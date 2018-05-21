#include "WDF.h"
#include <iostream>
#include <memory>
#include <functional>
#include <algorithm>
#include <memory>
using std::cout;
using std::cerr;
using std::endl;
using std::ios;
using std::unique_ptr;

namespace NetEase {

#define MEM_READ_WITH_OFF(off,dst,src,len) if(off+len<=src.size()){  memcpy((uint8_t*)dst,(uint8_t*)(src.data()+off),len);off+=len;   }



	WDF::WDF()
	{
		Init();
	}

	void WDF::Init()
	{
		std::fstream fs(m_Path, ios::in | ios::binary);
		if (!fs) {
			cout << "open wdf file error!!!" << endl;
			return;
		}
		std::cout << "InitWDF:" << m_Path << std::endl;

		auto fpos = fs.tellg();
		fs.seekg(0, std::ios::end);
		m_FileSize = fs.tellg() - fpos;
		
		m_FileData.resize(m_FileSize);
		fs.seekg(0, std::ios::beg);
		fs.read((char*)m_FileData.data(), m_FileSize);
		fs.close();


		m_FileName = m_Path.substr(m_Path.find_last_of("/")+1);
		Header header{ 0 };
		uint32_t fileOffset = 0;
		MEM_READ_WITH_OFF(fileOffset, &header, m_FileData, sizeof(Header));
		
		unsigned int Flag = header.flag;
		switch(Flag)
		{
			case 0x57444650: // WDFP
				m_FileType=1;
				std::cout << "file type : WDFP "  << std::endl;
				break;
			case 0x57444658: // WDFX
				m_FileType=2;
				std::cout << "file type : WDFX "  << std::endl;
				break;
			case 0x57444648: // WDFH
				m_FileType=3;
				std::cout << "file type : WDFH "  << std::endl;
				break;
			default:
				m_FileType=0;
		}
		if(m_FileType == 0 )
		{
			cout << "open wdf m_FileType error!!!" << endl;
			return;
		}
	

		m_WASNumber = header.number;
		m_FileDataOffset = header.offset;
		cout << "number:" << m_WASNumber << " offset:" << m_FileDataOffset << endl;

		mIndencies.resize(m_WASNumber);
		MEM_READ_WITH_OFF(m_FileDataOffset, mIndencies.data(), m_FileData, sizeof(Index)*m_WASNumber);

		for (uint32_t i = 0; i < m_WASNumber; i++)
		{
			mIdToPos[mIndencies[i].hash] = i;
		}

		m_Sprites.clear();
		cout << "WDF file header load ok!" << endl;
	}

	WDF::~WDF()
	{
		for (auto & it : m_Sprites)
		{
			delete it.second;
		}
		m_Sprites.clear();

		
		/*if (m_FileData)
		{
			delete m_FileData;
			m_FileData = nullptr;
		}*/
	}

	WAS WDF::GetWAS(uint32_t id)
	{
		Index index = mIndencies[mIdToPos[id]];
		return WAS(m_Path, index.offset, index.size);
	}

	Sprite2* WDF::LoadSprite(uint32_t id)
	{
		if (m_Sprites.count(id) > 0) return m_Sprites[id];

		if(mIdToPos.count(id) == 0) return nullptr;
			
		Index index = mIndencies[mIdToPos[id]];
		uint32_t wasOffset = index.offset;
		uint32_t wasSize = index.size;
		
		auto& wasMemData = m_FileData;
	
		uint32_t wasReadOff = wasOffset;		
	
		WAS::Header header{0};
		MEM_READ_WITH_OFF(wasReadOff,&header,wasMemData,sizeof(header));

		if (header.flag != 0x5053)
		{
			cerr << "Sprite File Flag Error!" << endl;
			return nullptr;
		}

		if (header.len != 12)
		{
			int addonHeadLen = header.len - 12;
			uint8_t* m_AddonHead = new uint8_t[addonHeadLen];
			MEM_READ_WITH_OFF(wasReadOff, m_AddonHead, wasMemData, addonHeadLen);
			delete[] m_AddonHead;
		}

		
		std::unique_ptr< Sprite2>  sprite(new Sprite2());

		sprite->mID = std::to_string(id);
		sprite->mPath = m_FileName+"/"+sprite->mID;
		sprite->mGroupSize = header.group;
		sprite->mFrameSize = header.frame;
		sprite->mWidth = header.width;
		sprite->mHeight = header.height;

		sprite->mWidth  = std::max(0, sprite->mWidth );
		sprite->mHeight = std::max(0, sprite->mHeight);

		sprite->mKeyX = header.key_x;
		sprite->mKeyY = header.key_y;

		int frameTotalSize = sprite->mGroupSize* sprite->mFrameSize;

		std::cout <<"frameTotalSize: "<< frameTotalSize<<std::endl;

		if (frameTotalSize < 0 || frameTotalSize > 1000) {
			cout << "frame size error!!!" << endl;
			return nullptr;
		}
        
		MEM_READ_WITH_OFF(wasReadOff,&m_Palette16[0],wasMemData,256 * 2);
		for (int k = 0; k < 256; k++)
		{
			m_Palette32[k] = WAS::RGB565to888(m_Palette16[k], 0xff); 
		}
		
		std::vector<uint32_t> frameIndexes(frameTotalSize,0);
		MEM_READ_WITH_OFF(wasReadOff,frameIndexes.data(),wasMemData,frameTotalSize * 4);

		sprite->mFrames.resize(frameTotalSize);
		
		uint32_t frameHeadOffset = 2 + 2 + header.len;

		for (int i = 0; i<frameTotalSize; i++)
		{
			wasReadOff = wasOffset + frameHeadOffset + frameIndexes[i];
			WAS::FrameHeader wasFrameHeader{0};
			MEM_READ_WITH_OFF(wasReadOff,&wasFrameHeader,wasMemData,sizeof(WAS::FrameHeader));

            if(wasFrameHeader.height >= (1<<15) || wasFrameHeader.width >= (1 <<15) ||wasFrameHeader.height < 0  || wasFrameHeader.width < 0)
            {
				std::cout << "wasFrameHeader error!!!" << std::endl;
                return nullptr;
            }

			Sprite2::Sequence& frame = sprite->mFrames[i];
			frame.key_x = wasFrameHeader.key_x;
			frame.key_y = wasFrameHeader.key_y;
			frame.width = wasFrameHeader.width;
			frame.height = wasFrameHeader.height;
			uint32_t pixels = frame.width*frame.height;
			frame.src.resize(pixels, 0);
		
		
			std::vector<uint32_t> frameLine(frame.height, 0);
			MEM_READ_WITH_OFF(wasReadOff,frameLine.data(),wasMemData, frame.height * 4);

			uint32_t* pBmpStart = frame.src.data();
			bool copyLine = true;	
			for (int j = 0; j< frame.height; j++)
			{
				uint32_t lineDataPos = wasOffset + frameIndexes[i] + frameHeadOffset + frameLine[j];
				uint8_t* lineData = m_FileData.data() + lineDataPos;
				pBmpStart = frame.src.data() + frame.width*(j);
				int pixelLen = frame.width;
				DataHandler((char*)lineData, pBmpStart, 0, pixelLen,j,copyLine);
			}
			
			if(copyLine)
			{
				for (int j = 0; j + 1< frame.height; j+=2)
				{
					uint32_t* pDst = &frame.src[ (j+1)*frame.width ];
					uint32_t* pSrc = &frame.src[ j*frame.width ];
					memcpy( (uint8_t*)pDst,(uint8_t*)pSrc,frame.width*4);
				}
			}

			frame.IsBlank = true;
			for(uint32_t xx=0;xx<pixels;xx++)
			{

				if(frame.src[xx] !=0)
				{
                    // std::cout <<frame.src[xx] << std::endl;
					frame.IsBlank = false;
					break;
				}
			} 
			// std::cout << " is blank :" << frame.IsBlank <<" frame:"<< i << std::endl;	
		}
		
		m_Sprites.insert(std::make_pair( id, sprite.release()));
		return m_Sprites[id];
	}
	void WDF::SaveWAS(uint32_t id)
	{
		if(!mIdToPos.count(id))return ;
		Index index = mIndencies[mIdToPos[id]];
		uint32_t wasOffset = index.offset;
		uint32_t wasSize = index.size;
        std::fstream file(m_Path, ios::in | ios::binary);

		file.seekg(wasOffset,ios::beg);
		char* outfilec=new char[wasSize];
		file.read(outfilec,wasSize);
		std::fstream of("a.was",ios::binary|ios::out);
		of.write(outfilec,wasSize);
		of.close();
	}
	std::vector<Sprite2 *> WDF::LoadAllSprite()
	{
		std::vector<Sprite2*> v;
		for (uint32_t i = 0; i < m_WASNumber; i++)
		{

			auto p = LoadSprite(mIndencies[i].hash);
			if (p)
			{
				v.push_back(p);
			}
		}
		return v;
	}


	void WDF::DataHandler(char *pData, uint32_t* pBmpStart, int pixelOffset, int pixelLen,int y, bool& copyline)
	{
		uint32_t temp_pixel = 0;

		uint32_t Pixels = pixelOffset;
		uint32_t PixelLen = pixelLen;
		uint16_t AlphaPixel = 0;

		while (pData && *pData != 0) // {00000000} ????????��??????????????????????????
		{
			uint8_t style = 0;
			uint8_t Level = 0; // Alpha????
			uint8_t Repeat = 0; // ???????
			style = (*pData & 0xc0) >> 6; // ??????????????
			switch (style)
			{
			case 0: // {00******}
				if(copyline&&y == 1)
				{
					copyline = false;
				}
				if (*pData & 0x20) // {001*****} ???????Alpha????????????
				{
					// {001 +5bit Alpha}+{1Byte Index}, ???????Alpha?????????????
					// {001 +0~31??Alpha???}+{1~255???????????}
					Level = (*pData) & 0x1f; // 0x1f=(11111) ???Alpha??????
					pData++; // ????????
					if (Pixels < PixelLen)
					{
						AlphaPixel = WAS::Alpha565(m_Palette16[(uint8_t)(*pData)], 0, Level); // ???
						*pBmpStart++ = WAS::RGB565to888(AlphaPixel, Level * 8);
						Pixels++;
						pData++;
					}
				}
				else // {000*****} ??????n?��???Alpha?????????
				{
					// {000 +5bit Times}+{1Byte Alpha}+{1Byte Index}, ??????n?��???Alpha??????????
					// {000 +???1~31??}+{0~255??Alpha???}+{1~255???????????}
					// ?: ?????{00000000} ??????????��???????????????????1~31?��?
					Repeat = (*pData) & 0x1f; // ???????????
					pData++;
					Level = *pData; // ???Alpha????
					pData++;
					for (int i = 1; i <= Repeat; i++)
					{
						if (Pixels < PixelLen)
						{
							AlphaPixel = WAS::Alpha565(m_Palette16[(uint8_t)*pData], 0, Level); // ???
							*pBmpStart++ = WAS::RGB565to888(AlphaPixel, Level * 8);
							Pixels++;
						}
					}
					pData++;
				}
				break;
			case 1: // {01******} ???????Alpha??????????n???????????????
					// {01  +6bit Times}+{nByte Datas},???????Alpha??????????n??????????????��?
					// {01  +1~63??????}+{n??????????},{01000000}????
					if(copyline&&y == 1)
				{
					copyline = false;
				}
				Repeat = (*pData) & 0x3f; // ??????????��????
				pData++;
				for (int i = 1; i <= Repeat; i++)
				{
					if (Pixels < PixelLen)
					{
						*pBmpStart++ = m_Palette32[(uint8_t)*pData];
						Pixels++;
						pData++;
					}
				}
				break;
			case 2: // {10******} ??????n??????
					// {10  +6bit Times}+{1Byte Index}, ??????n???????
					// {10  +???1~63??}+{0~255???????????},{10000000}????
					if(copyline&&y == 1)
				{
					copyline = false;
				}
				Repeat = (*pData) & 0x3f; // ???????????
				pData++;
				for (int i = 1; i <= Repeat; i++)
				{
					if (Pixels <PixelLen)
					{
						*pBmpStart++ = m_Palette32[(uint8_t)*pData];
						Pixels++;
					}
				}
				pData++;
				break;
			case 3: // {11******} ???????n???????????????????????????
					// {11  +6bit Times}, ???????n??????????????????????????��
					// {11  +????1~63??????},{11000000}????
				Repeat = (*pData) & 0x3f; // ??????????
				for (int i = 1; i <= Repeat; i++)
				{
					if (Pixels < PixelLen)
					{
						pBmpStart++;
						Pixels++;
					}
				}
				pData++;
				break;
			default: // ??????????????
				cerr << "Error!" << endl;
				//exit(EXIT_FAILURE);
				break;
			}
		}
		if (*pData == 0 && PixelLen >Pixels )
		{
			uint32_t Repeat = 0;
			Repeat = PixelLen - Pixels;
            // std::cout <<" pixLen : "<< pixelLen <<" line:" << y <<" repeate:" << (PixelLen - Pixels) << " copy line:"<< copyline<< std::endl;
			for (uint32_t i = 0; i < Repeat; i++)
			{
				if (Pixels < PixelLen)
				{
					pBmpStart++;
					Pixels++;
				}
			}
		}
	}
}


// int main(int argc,char* argv[])
// {
// 	// shared_ptr<WDF> p = make_shared<WDF>("./data/shape.wdf");
// 	// cout<<p.use_count();
// 	WDF wdf("./data/shape.wdf");
// 	// wdf.GetWAS(0x00642F2B);
// 	wdf.LoadSprite(0x49386FCE);
// }
