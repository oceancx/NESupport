#include "WDF.h"
#include <iostream>
#include <memory>
#include <functional>
using std::cout;
using std::cerr;
using std::endl;
namespace NetEase {
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

		m_FileName = m_Path.substr(m_Path.find_last_of("/")+1);

		Header header;
		fs.read((char*)&header, sizeof(Header));

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

		mIndencies.clear();
		mIndencies.resize(m_WASNumber);
		fs.seekg(m_FileDataOffset);
		fs.read((char*)&mIndencies[0], sizeof(Index)*m_WASNumber);
		fs.close();

		for (int i = 0; i<m_WASNumber; i++)
		{
			mIdToPos[mIndencies[i].hash] = i;
		}

		cout << "WDF file header load ok!" << endl;
	}

	WDF::~WDF()
	{
	
	}

	WAS WDF::GetWAS(uint32_t id)
	{
		Index index = mIndencies[mIdToPos[id]];
		return WAS(m_Path, index.offset, index.size);
	}

	std::shared_ptr<Sprite2> WDF::LoadSprite(uint32_t id)
	{
		if(mIdToPos.count(id) == 0) return nullptr;
		
		std::ifstream file(m_Path, ios::in | ios::binary);
		if(!file)
		{
			cout << "file read error!!! WDF::LoadSprite" << endl;
			return nullptr;
		}
		
		Index index = mIndencies[mIdToPos[id]];
		uint32_t wasOffset = index.offset;
		uint32_t wasSize = index.size;
		
		// unique_ptr<uint8_t[],std::function<void(uint8_t*p)>> wasMemData(new uint8_t[wasSize],[](uint8_t*p){
		// 	std::cout << "destory" << endl;
		// 	delete[] p;
		// });
		unique_ptr<uint8_t[]> wasMemData(new uint8_t[wasSize]);
 	//	uint8_t* wasMemData = new uint8_t[wasSize];
		file.seekg(wasOffset, ios::beg);
		file.read((char*)wasMemData.get(), wasSize);
		file.close();
		cout << "was file read ok!!!" << endl;

		uint32_t wasReadOff = 0;
#define MEM_READ_WITH_OFF(off,dst,src,len) if(off+len < wasSize){  memcpy((char*)dst,(src.get()+off),len);off+=len;   }
		
	
		WAS::Header header{};
		MEM_READ_WITH_OFF(wasReadOff,&header,wasMemData,sizeof(header));

		auto sprite = std::make_shared<Sprite2>();

		sprite->mID = std::to_string(id);
		sprite->mPath = m_FileName+"/"+sprite->mID;
		sprite->mGroupSize = header.group;
		sprite->mFrameSize = header.frame;
		sprite->mWidth = header.width;
		sprite->mHeight = header.height;
		sprite->mKeyX = header.key_x;
		sprite->mKeyY = header.key_y;
		

		sprite->Error =  sprite->mGroupSize >= 10000;
		sprite->Error =  sprite->mFrameSize >= 10000;
		sprite->Error =  sprite->mWidth >= 10000;
		sprite->Error =  sprite->mHeight >= 10000;
		sprite->Error =  sprite->mKeyX >= 10000;
		sprite->Error =  sprite->mKeyY >= 10000;
		if (header.flag != 0x5053 || sprite->Error )
		{
			cerr << "Sprite File Flag Error!" << endl;
			std::shared_ptr<Sprite2> sp = std::make_shared<Sprite2>();
			sp->Error = true;
            return sp;
			//exit(EXIT_FAILURE);
		}

		sprite->mFrames = new Sprite2::Sequence*[header.group];
        memset(sprite->mFrames, 0, sizeof(Sprite2::Sequence*)*header.group);
		for (int i = 0; i < header.group; i++)
		{
			sprite->mFrames[i] = new Sprite2::Sequence[header.frame];
            memset(sprite->mFrames[i], 0, sizeof(Sprite2::Sequence)*header.frame);
		}
		

		// ?��????????????????12
		if (header.len != 12)
		{
			// ???????????????
			int AddonHeadLen = header.len - 12;
			uint8_t* m_AddonHead = new uint8_t[AddonHeadLen]; // ???��??????????
			
			MEM_READ_WITH_OFF(wasReadOff,m_AddonHead,wasMemData,AddonHeadLen);
			// file.read((char*)m_AddonHead, AddonHeadLen); // ???????????
		}


		// ????????????
		MEM_READ_WITH_OFF(wasReadOff,&palette16[0],wasMemData,256 * 2);
		// file.read((char*)&palette16[0], 256 * 2); // Palette[0]?????

		for (int k = 0; k < 256; k++)
		{
			m_Palette32[k] = WAS::RGB565to888(palette16[k], 0xff); // 16to32????????
		}

		int frameTotalSize = header.group * header.frame;
		// std::cout <<"frameTotalSize: "<< frameTotalSize<<std::endl;

		uint32_t* frameIndexes = new uint32_t[frameTotalSize];
		MEM_READ_WITH_OFF(wasReadOff,frameIndexes,wasMemData,frameTotalSize * 4);
		// file.read((char*)frameIndexes, frameTotalSize * 4);


		uint32_t pixels = header.width*header.height;

		int frameHeadOffset = 2 + 2 + header.len;

		uint32_t* frameLine = new uint32_t[header.height]; // ???????????��????
        memset(frameLine,0,4*header.height);
		

		int x = 0; // ???????
		int z = 0; // ???????

		uint8_t* lineData = nullptr; //= new uint8_t[header.width * 4]; // ?????????????
        // memset(lineData,0,4*header.width);
		for (int i = 0; i<frameTotalSize; i++)
		// for (int i = frameTotalSize/2; i< frameTotalSize/2+1; i++)
		{
			int gpos = i / (header.frame);
			int cpos = i % (header.frame);

			Sprite2::Sequence& frame = sprite->mFrames[gpos][cpos];

			WAS::FrameHeader tempFreamHeader;
            memset(&tempFreamHeader,0,sizeof(	WAS::FrameHeader ));

			wasReadOff = frameHeadOffset + frameIndexes[i];
			MEM_READ_WITH_OFF(wasReadOff,&tempFreamHeader,wasMemData,sizeof(WAS::FrameHeader));

			// file.read((char*)&tempFreamHeader, sizeof(WAS::FrameHeader));

            if(tempFreamHeader.height >= (1<<15) || tempFreamHeader.width >= (1 <<15) 
            	||tempFreamHeader.height < 0  || tempFreamHeader.width < 0)
            {
                std::cout <<"w:" << std::dec<< tempFreamHeader.width <<" \t h:" << tempFreamHeader.height << std::endl;
                // std::cout <<"read file error! was id:" << std::hex<< id << std::endl;
				std::shared_ptr<Sprite2> sp = std::make_shared<Sprite2>();
				sp->Error = true;
				file.close();
                return sp;
            }

			frame.key_x = tempFreamHeader.key_x;
			frame.key_y = tempFreamHeader.key_y;
			frame.width = tempFreamHeader.width;
			frame.height = tempFreamHeader.height;
            frame.src = new uint32_t[pixels];
			memset(frame.src, 0, pixels * 4);



			// ??????????????
			MEM_READ_WITH_OFF(wasReadOff,frameLine,wasMemData,tempFreamHeader.height * 4);

			// file.read((char*)frameLine, tempFreamHeader.height * 4);

            // std::cout <<"frame:  " << i << " width:" << frame.width << std::endl;
			uint32_t* pBmpStart = frame.src;//=frame.src+pixels*3;
			bool copyLine = true;	
			for (int j = 0; j< tempFreamHeader.height; j++)
			{
				pBmpStart = frame.src + sprite->mWidth*(j);
				// int lineDataLen = 0;
				// if (j < tempFreamHeader.height - 1)
				// {
                // 	lineDataLen = frameLine[j + 1] - frameLine[j]; // ???????????��
				// }
				// else
				// {
				// 	if (i<frameTotalSize - 1) {
				// 		lineDataLen = frameIndexes[i + 1] - (frameIndexes[i] + frameLine[j]);
				// 	}
				// 	else {
				// 		lineDataLen = wasSize - (frameIndexes[i] + frameLine[j]);
				// 	}
				// }
                // printf("lineLen:%d\n",lineDataLen);
				// memset(lineData, 0, frame.width);
				
                int lineDataPos = frameIndexes[i] + frameHeadOffset + frameLine[j];
				lineData = &wasMemData[lineDataPos];
				// MEM_READ_WITH_OFF(wasReadOff,lineData,wasMemData,lineDataLen);

 				// file.seekg(seekpos, ios::beg);
				// file.read((char*)lineData, lineDataLen);
				// printf("before handler:  %x\n",  sprite->mFrames[gpos][cpos].src );
				int pixelOffset = (sprite->mKeyX - frame.key_x);
				int pixelLen = sprite->mWidth;
				// printf("pixelOffset: %d  pixelLen: %d\n",pixelOffset,pixelLen );
				pBmpStart += pixelOffset;
				pBmpStart += (sprite->mKeyY - frame.key_y)*sprite->mWidth;

				DataHandler((char*)lineData, pBmpStart, pixelOffset, pixelLen,j,copyLine);
			}
			
            //int psize = pBmpStart-frame.src;
            //std::cout << " psize:" << psize << std::endl;
			if(copyLine)
			{
				for (int j = 0; j + 1< header.height; j+=2)
				{
					uint32_t* pDst = &frame.src[ (j+1)*header.width ];
					uint32_t* pSrc = &frame.src[ j*header.width ];
					memcpy( (uint8_t*)pDst,(uint8_t*)pSrc,header.width*4);
				}
			}

			frame.IsBlank = true;
			for(int xx=0;xx<pixels;xx++)
			{

				if(frame.src[xx] !=0)
				{
                    // std::cout <<frame.src[xx] << std::endl;
					frame.IsBlank = false;
					break;
				}
			}
		
			 
			// std::cout << " is blank :" << frame.IsBlank <<" frame:"<< i << std::endl;
			//  sprite->SaveImage(i);
		}
		//file.close();
		sprite->Error= false;
		return sprite;
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
		fstream of("a.was",ios::binary|ios::out);
		of.write(outfilec,wasSize);
		of.close();
	}
	std::vector<std::shared_ptr<Sprite2>> WDF::LoadAllSprite()
	{
		std::vector<std::shared_ptr<Sprite2>> v;
		for (int i = 0; i<m_WASNumber; i++)
		{
			
			auto p = LoadSprite(mIndencies[i].hash);
			if(!p->Error)
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

		while (*pData != 0) // {00000000} ????????��??????????????????????????
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
						AlphaPixel = WAS::Alpha565(palette16[(uint8_t)(*pData)], 0, Level); // ???
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
							AlphaPixel = WAS::Alpha565(palette16[(uint8_t)*pData], 0, Level); // ???
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
				exit(EXIT_FAILURE);
				break;
			}
		}
		if (*pData == 0 && PixelLen >Pixels )
		{
			uint32_t Repeat = 0;
			Repeat = PixelLen - Pixels;
            // std::cout <<" pixLen : "<< pixelLen <<" line:" << y <<" repeate:" << (PixelLen - Pixels) << " copy line:"<< copyline<< std::endl;
			for (int i = 0; i < Repeat; i++)
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
