#include "NESupport.h"
#include <iostream>
#include <memory>
#include <functional>
#include <algorithm>
#include <memory>
#include <cmath>
#include <string>
using std::cerr;
using std::endl;
using std::ios;

#define MEM_READ_WITH_OFF(off,dst,src,len) if(off+len<=src.size()){  memcpy((uint8_t*)dst,(uint8_t*)(src.data()+off),len);off+=len;   }


#ifndef TGA_FILE_HEADER_H
#define TGA_FILE_HEADER_H
#pragma pack(push)
#pragma pack(1)
struct TGA_FILE_HEADER
{
	uint8_t IdLength;				
	uint8_t ColorMapType;			
	uint8_t ImageType;				// 2 or 10
	uint16_t ColorMapFirstIndex;	
	uint16_t ColorMapLength;		
	uint8_t ColorMapEntrySize;		// (default:0，support 16/24/32)
	uint16_t XOrigin;				
	uint16_t YOrigin;				
	uint16_t ImageWidth;			
	uint16_t ImageHeight;			
	uint8_t PixelDepth;				// 8,16,24,32
	uint8_t ImageDescruptor;		
};
#pragma pack(pop)
#endif

namespace NE {


	uint8_t MixAlpha(uint8_t color, uint8_t alpha)
	{
		// a*C+(1-a)*C
		uint32_t res = color * alpha / 0xff;
		return res > 0xff ? (res%0xff) : res;
	}

	uint32_t RGB565to888(uint16_t color, uint8_t alpha)
	{
		unsigned int r = (color >> 11) & 0x1f;
		unsigned int g = (color >> 5) & 0x3f;
		unsigned int b = (color) & 0x1f;
		uint32_t R, G, B, A;
		A = alpha;
		R = (r << 3) | (r >> 2);
		G = (g << 2) | (g >> 4);
		B = (b << 3) | (b >> 2);
		return   A << 24 | (B << 16) | (G << 8) | R;
	}


	uint16_t ChangeColorPal(uint16_t color, std::vector<uint16_t> c)
	{
		unsigned int r = (color >> 11) & 0x1f;
		unsigned int g = (color >> 5) & 0x3f;
		unsigned int b = (color) & 0x1f;
		unsigned int  CR, CG, CB;

		CR = ((r * c[0] + g * c[1] + b * c[2]) >> 8);
		if (CR > 0x1f) CR = 0x1f;

		CG = ((r *  c[3] + g * c[4] + b * c[5]) >> 8);
		if (CG > 0x3f) CG = 0x3F;

		CB = ((r *  c[6] + g * c[7] + b * c[8]) >> 8);
		if (CB > 0x1f) CB = 0x1f;
		return   (CR << 11) | (CG << 5) | CB;
	}

	// 16bit 565Type Alpha mix
	uint16_t Alpha565(uint16_t Src, uint16_t Des, uint8_t Alpha)
	{
		uint16_t Result;
		// after mix = ( ( A-B ) * Alpha ) >> 5 + B
		// after mix = ( A * Alpha + B * ( 32-Alpha ) ) / 32

		unsigned short R_Src, G_Src, B_Src;
		R_Src = G_Src = B_Src = 0;

		R_Src = Src & 0xF800;
		G_Src = Src & 0x07E0;
		B_Src = Src & 0x001F;

		R_Src = R_Src >> 11;
		G_Src = G_Src >> 5;

		unsigned short R_Des, G_Des, B_Des;
		R_Des = G_Des = B_Des = 0;

		R_Des = Des & 0xF800;
		G_Des = Des & 0x07E0;
		B_Des = Des & 0x001F;

		R_Des = R_Des >> 11;
		G_Des = G_Des >> 5;

		unsigned short R_Res, G_Res, B_Res;
		R_Res = G_Res = B_Res = 0;

		R_Res = (((R_Src - R_Des)*Alpha) >> 5) + R_Des;
		G_Res = (((G_Src - G_Des)*Alpha) >> 5) + G_Des;
		B_Res = (((B_Src - B_Des)*Alpha) >> 5) + B_Des;

		R_Res = R_Res << 11;
		G_Res = G_Res << 5;

		Result = R_Res | G_Res | B_Res;
		return Result;
	}

	void Sprite::SaveImage(const char* filename, int index)
	{
		Sequence& sq = mFrames[index];

		TGA_FILE_HEADER TgaHeader;
		memset(&TgaHeader, 0, 18);

		TgaHeader.IdLength = 0;
		TgaHeader.ColorMapType = 0;
		TgaHeader.ImageType = 0x02;
		TgaHeader.ColorMapFirstIndex = 0;
		TgaHeader.ColorMapLength = 0;
		TgaHeader.ColorMapEntrySize = 0;
		TgaHeader.XOrigin = 0;
		TgaHeader.YOrigin = 0;
		TgaHeader.ImageWidth = sq.width;
		TgaHeader.ImageHeight = sq.height;
		TgaHeader.PixelDepth = 24;
		TgaHeader.ImageDescruptor = 8;
		std::ofstream ofile(filename, std::ios::out | std::ios::trunc | std::ios::binary);
		if (!ofile)return;

		uint8_t* img_data = new uint8_t[sq.src.size() * 3];
		for (int row = 0; row < sq.height; row++) {
			for (int col = 0; col < sq.width; col++) {
				int fliprow = sq.height-1 - row;
				int i = fliprow * sq.width + col;
				int datai = row * sq.width + col;
				uint32_t pix = sq.src[i];
				img_data[datai * 3] = (pix & 0xff0000) >> 16;
				img_data[datai * 3 + 1] = (pix & 0xff00) >> 8;
				img_data[datai * 3 + 2] = pix & 0xff;
			}
		}
		
		ofile.write((char*)(&TgaHeader), sizeof(TGA_FILE_HEADER));
		ofile.write((char*)img_data, sq.src.size() * 3);
		delete[] img_data;
		img_data = nullptr;
		ofile.close();
	}

	WAS::WAS(std::string filename)
		:WAS(filename, 0)
	{
	}

	WAS::WAS(std::string path, uint32_t offset)
		: mPath(path)
	{
		std::ifstream infile(mPath, ios::binary | ios::in);
		if (!infile) return;
		infile.seekg(offset, ios::beg);
		infile.read((char*)&mHeader, sizeof(mHeader));
		if (mHeader.flag != 0x5053)
		{
			cerr << "Sprite File Flag Error!" << endl;
			infile.close();
			return;
		}

		uint16_t palette16[256];
		memset(palette16, 0, sizeof(palette16));
		infile.read((char*)palette16, sizeof(palette16));

		for (int i = 0; i < 256; i++)
		{
			mPalette32[i] = RGB565to888(palette16[i], 0xff);
		}

		int frames = mHeader.group * mHeader.frame;
		mFrameIndecies.resize(frames);
		infile.read((char*)mFrameIndecies.data(), frames * 4);
		infile.close();
	}

	WAS::~WAS()
	{

	}

	WDF::WDF(std::string path)
		:m_Path(path)
	{
		std::fstream fs(m_Path, ios::in | ios::binary);
		if (!fs) {
			cerr << "open wdf file error!!!" << endl;
			return;
		}
		//std::cerr << "InitWDF:" << m_Path.c_str() << std::endl;

		auto fpos = fs.tellg();
		fs.seekg(0, std::ios::end);
		m_FileSize = fs.tellg() - fpos;

		m_FileData.resize(m_FileSize);
		fs.seekg(0, std::ios::beg);
		fs.read((char*)m_FileData.data(), m_FileSize);
		fs.close();


		m_FileName = m_Path.substr(m_Path.find_last_of("/") + 1);
		m_WDFDir = m_Path.substr(0, m_Path.find_last_of("/"));

		Header header{ 0 };
		uint32_t fileOffset = 0;
		MEM_READ_WITH_OFF(fileOffset, &header, m_FileData, sizeof(Header));

		unsigned int Flag = header.flag;
		switch (Flag)
		{
		case 0x57444650: // WDFP
			m_FileType = 1;
			//std::cerr << "file type : WDFP "  << std::endl;
			break;
		case 0x57444658: // WDFX
			m_FileType = 2;
			//std::cerr << "file type : WDFX "  << std::endl;
			break;
		case 0x57444648: // WDFH
			m_FileType = 3;
			//std::cerr << "file type : WDFH "  << std::endl;
			break;
		default:
			m_FileType = 0;
		}
		if (m_FileType == 0)
		{
			cerr << "open wdf m_FileType error!!!" << endl;
			return;
		}


		m_WASNumber = header.number;
		m_FileDataOffset = header.offset;
		//cerr << "number:" << m_WASNumber << " offset:" << m_FileDataOffset << endl;

		mIndencies.resize(m_WASNumber);
		MEM_READ_WITH_OFF(m_FileDataOffset, mIndencies.data(), m_FileData, sizeof(Index)*m_WASNumber);

		for (uint32_t i = 0; i < m_WASNumber; i++)
		{
			auto id = mIndencies[i].hash;
			mIdToPos[id] = i;
			m_SpritesLoading[id] = false;
			m_SpritesLoaded[id] = false;
		}

		m_Sprites.clear();
		//cerr << "WDF file header load ok!" << endl;
	}


	WDF::~WDF()
	{

	}

	WAS WDF::GetWAS(uint32_t id)
	{
		Index index = mIndencies[mIdToPos[id]];
		return WAS(m_Path, index.offset);
	}

	void WDF::UnLoadSprite(uint32_t id)
	{
		auto it = m_Sprites.find(id);
		if (it != m_Sprites.end())
		{
			m_Sprites.erase(it);
		}
		m_SpritesLoaded[id] = false;
		m_SpritesLoading[id] = false;
	}

	Sprite* WDF::LoadSpriteHeader(uint32_t id, std::vector<PalMatrix>* patMatrix)
	{
		auto it = m_Sprites.find(id);
		if (it != m_Sprites.end())
		{
			return &it->second;
		}

		Index index = mIndencies[mIdToPos[id]];

		auto& wasMemData = m_FileData;

		uint32_t wasReadOff = index.offset;

		WAS::Header header{ 0 };
		MEM_READ_WITH_OFF(wasReadOff, &header, wasMemData, sizeof(header));

		if (header.flag != 0x5053)
		{
			std::cerr << "Sprite File Flag Error!" << endl;
			return nullptr;
		}

		if (header.len > 12)
		{
			int addonHeadLen = header.len - 12;
			uint8_t* m_AddonHead = new uint8_t[addonHeadLen];
			MEM_READ_WITH_OFF(wasReadOff, m_AddonHead, wasMemData, addonHeadLen);
			delete[] m_AddonHead;
		}

		Sprite&  sprite = m_Sprites[id];
		sprite.FrameLoaded = false;
		sprite.FrameWASOffset = wasReadOff;
		sprite.mID = std::to_string(id);
		sprite.mPath = m_FileName + "/" + sprite.mID;
		sprite.mGroupSize = header.group;
		sprite.mFrameSize = header.frame;
		sprite.mWidth = header.width;
		sprite.mHeight = header.height;
		sprite.mWidth = std::max(0, sprite.mWidth);
		sprite.mHeight = std::max(0, sprite.mHeight);
		sprite.mKeyX = header.key_x;
		sprite.mKeyY = header.key_y;
		return &sprite;
	}

	bool WDF::LoadSpriteData(Sprite* sprite, std::vector<PalMatrix>* patMatrix)
	{
		if (sprite == nullptr) return false;
		if (sprite->FrameLoaded)return true;

		uint32_t id = std::stoul(sprite->mID);

		if (m_SpritesLoaded[id]) return &m_Sprites[id];

		if (mIdToPos.count(id) == 0) return nullptr;

		if (m_SpritesLoading[id]) return nullptr;

		m_SpritesLoading[id] = true;

		auto& wasMemData = m_FileData;
		uint32_t wasReadOff = sprite->FrameWASOffset;
		int frameTotalSize = sprite->mFrameSize*sprite->mGroupSize;
		MEM_READ_WITH_OFF(wasReadOff, &m_Palette16[0], wasMemData, 256 * 2);

		if (patMatrix != nullptr&& patMatrix->size() != 0) {
			for(auto& v : *patMatrix)
			{
				for (int i = v.from; i< v.to; i++) {
					m_Palette16[i] = ChangeColorPal(m_Palette16[i], v.mat);
				}
			}
		}
		
		for (int k = 0; k < 256; k++)
		{
			m_Palette32[k] = RGB565to888(m_Palette16[k], 0xff);	
		}

		std::vector<uint32_t> frameIndexes(frameTotalSize, 0);
		MEM_READ_WITH_OFF(wasReadOff, frameIndexes.data(), wasMemData, frameTotalSize * 4);

		sprite->mFrames.resize(frameTotalSize);

		for (int i = 0; i < frameTotalSize; i++)
		{
			wasReadOff = sprite->FrameWASOffset + frameIndexes[i];
			WAS::FrameHeader wasFrameHeader{ 0 };
			MEM_READ_WITH_OFF(wasReadOff, &wasFrameHeader, wasMemData, sizeof(WAS::FrameHeader));

			if (wasFrameHeader.height >= (1 << 15) || wasFrameHeader.width >= (1 << 15) || wasFrameHeader.height < 0 || wasFrameHeader.width < 0)
			{
				std::cerr << "wasFrameHeader error!!!" << std::endl;
				m_SpritesLoading[id] = false;
				m_SpritesLoaded[id] = true;
				return nullptr;
			}

			Sprite::Sequence& frame = sprite->mFrames[i];
			frame.key_x = wasFrameHeader.key_x;
			frame.key_y = wasFrameHeader.key_y;
			frame.width = wasFrameHeader.width;
			frame.height = wasFrameHeader.height;
			uint32_t pixels = frame.width*frame.height;
			frame.src.resize(pixels, 0);

			std::vector<uint32_t> frameLine(frame.height, 0);
			MEM_READ_WITH_OFF(wasReadOff, frameLine.data(), wasMemData, frame.height * 4);

			uint32_t* pBmpStart = nullptr;
			bool copyLine = true;
			for (int j = 0; j < frame.height; j++)
			{
				uint32_t lineDataPos = sprite->FrameWASOffset  + frameIndexes[i] + frameLine[j];
				uint8_t* lineData = m_FileData.data() + lineDataPos;
				pBmpStart = frame.src.data() + frame.width*(j);
				int pixelLen = frame.width;
				DataHandler(lineData, pBmpStart, 0, pixelLen, j, copyLine);
			}

			if (copyLine)
			{
				for (int j = 0; j + 1 < frame.height; j += 2)
				{
					uint32_t* pDst = &frame.src[(j + 1)*frame.width];
					uint32_t* pSrc = &frame.src[j*frame.width];
					memcpy((uint8_t*)pDst, (uint8_t*)pSrc, frame.width * 4);
				}
			}

			frame.IsBlank = true;
			for (uint32_t pix = 0; pix < pixels; pix++)
			{
				if (frame.src[pix] != 0)
				{
					// std::cerr <<frame.src[xx] << std::endl;
					frame.IsBlank = false;
					break;
				}
			}
			// std::cerr << " is blank :" << frame.IsBlank <<" frame:"<< i << std::endl;	
		}
		sprite->FrameLoaded = true;
		m_Sprites[id] = *sprite;
		m_SpritesLoading[id] = false;
		m_SpritesLoaded[id] = true;
		return &m_Sprites[id];
	}

	Sprite* WDF::LoadSprite(uint32_t id, std::vector<PalMatrix>* patMatrix)
	{
		Sprite* sprite = LoadSpriteHeader(id);
		LoadSpriteData(sprite, patMatrix);
		return sprite;
	}

	void WDF::SaveWAS(uint32_t id,const char* path)
	{
		if (!mIdToPos.count(id))return;
		Index index = mIndencies[mIdToPos[id]];
		uint32_t wasOffset = index.offset;
		uint32_t wasSize = index.size;
		std::fstream file(m_Path, ios::in | ios::binary);

		file.seekg(wasOffset, ios::beg);
		char* outfilec = new char[wasSize];
		file.read(outfilec, wasSize);
		std::fstream of(path, ios::binary | ios::out);
		of.write(outfilec, wasSize);
		of.close();
		delete[] outfilec;
	}

	std::vector<Sprite *> WDF::LoadAllSprite()
	{
		std::vector<Sprite*> v;
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


	void WDF::DataHandler(uint8_t *pData, uint32_t* pBmpStart, int pixelOffset, int pixelLen, int y, bool& copyline)
	{
		uint32_t Pixels = pixelOffset;
		uint32_t PixelLen = pixelLen;
		uint16_t AlphaPixel = 0;

		while (pData && *pData != 0) // {00000000} 表示像素行结束，如有剩余像素用透明色代替
		{
			uint8_t style = 0;
			uint8_t Level = 0; // Alpha层数
			uint8_t Repeat = 0; // 重复次数
			style = (*pData & 0xc0) >> 6;  // 取字节的前两个比特
			switch (style)
			{
			case 0: // {00******}
				if (copyline&&y == 1)
				{
					copyline = false;
				}
				if (*pData & 0x20) // {001*****} 表示带有Alpha通道的单个像素
				{
					// {001 +5bit Alpha}+{1Byte Index}, 表示带有Alpha通道的单个像素。
					// {001 +0~31层Alpha通道}+{1~255个调色板引索}
					Level = (*pData) & 0x1f; // 0x1f=(11111) 获得Alpha通道的值
					pData++; // 下一个字节
					if (Pixels < PixelLen)
					{
						AlphaPixel = Alpha565(m_Palette16[(uint8_t)(*pData)], 0, Level);  // 混合
						*pBmpStart++ = RGB565to888(AlphaPixel, Level * 8);
						Pixels++;
						pData++;
					}
				}
				else // {000*****} 表示重复n次带有Alpha通道的像素
				{
					// {000 +5bit Times}+{1Byte Alpha}+{1Byte Index}, 表示重复n次带有Alpha通道的像素。
					// {000 +重复1~31次}+{0~255层Alpha通道}+{1~255个调色板引索}
					// 注: 这里的{00000000} 保留给像素行结束使用，所以只可以重复1~31次。
					Repeat = (*pData) & 0x1f; // 获得重复的次数
					pData++;
					Level = *pData; // 获得Alpha通道值
					pData++;
					for (int i = 1; i <= Repeat; i++)
					{
						if (Pixels < PixelLen)
						{
							AlphaPixel = Alpha565(m_Palette16[(uint8_t)*pData], 0, Level); // ???
							*pBmpStart++ = RGB565to888(AlphaPixel, Level * 8);
							Pixels++;
						}
					}
					pData++;
				}
				break;
			case 1:
				// {01******} 表示不带Alpha通道不重复的n个像素组成的数据段
				// {01  +6bit Times}+{nByte Datas},表示不带Alpha通道不重复的n个像素组成的数据段。
				// {01  +1~63个长度}+{n个字节的数据},{01000000}保留。
				if (copyline&&y == 1)
				{
					copyline = false;
				}
				Repeat = (*pData) & 0x3f; // 获得数据组中的长度
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
			case 2:
				// {10******} 表示重复n次像素
				// {10  +6bit Times}+{1Byte Index}, 表示重复n次像素。
				// {10  +重复1~63次}+{0~255个调色板引索},{10000000}保留。
				if (copyline&&y == 1)
				{
					copyline = false;
				}
				Repeat = (*pData) & 0x3f; // 获得重复的次数
				pData++;
				for (int i = 1; i <= Repeat; i++)
				{
					if (Pixels < PixelLen)
					{
						*pBmpStart++ = m_Palette32[(uint8_t)*pData];
						Pixels++;
					}
				}
				pData++;
				break;
			case 3:
				// {11******} 表示跳过n个像素，跳过的像素用透明色代替
				// {11  +6bit Times}, 表示跳过n个像素，跳过的像素用透明色代替。
				// {11  +跳过1~63个像素},{11000000}保留。
				Repeat = (*pData) & 0x3f; // 获得重复次数
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
			default: // 一般不存在这种情况
				cerr << "Error!" << endl;
				break;
			}
		}
		if (*pData == 0 && PixelLen > Pixels)
		{
			uint32_t Repeat = 0;
			Repeat = PixelLen - Pixels;
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

	MAP::MAP(std::string filename) :m_FileName(filename)
	{
		std::fstream fs(m_FileName, ios::in | ios::binary);
		if (!fs) {
			std::cerr << "Map file open error!" << std::endl;
			return;
		}
		std::cerr << "InitMAP:" << m_FileName.c_str() << std::endl;

		auto fpos = fs.tellg();
		fs.seekg(0, std::ios::end);
		m_FileSize = fs.tellg() - fpos;

		m_FileData.resize(m_FileSize);
		fs.seekg(0, std::ios::beg);
		fs.read((char*)m_FileData.data(), m_FileSize);
		fs.close();

		uint32_t fileOffset = 0;
		MEM_READ_WITH_OFF(fileOffset, &m_Header, m_FileData, sizeof(MapHeader));
		if (m_Header.Flag != 0x4D312E30)
		{
			cerr << "Map file format error!" << endl;
			return;
		}

		m_Width = m_Header.Width;
		m_Height = m_Header.Height;
		//	cout << "Width:" << m_Width << "\tHeight:" << m_Height << endl;

		m_BlockWidth = 320;
		m_BlockHeight = 240;

		m_ColCount = (uint32_t)std::ceil(m_Header.Width *1.0f / m_BlockWidth);
		m_RowCount = (uint32_t)std::ceil(m_Header.Height*1.0f / m_BlockHeight);
		//cout << "Row:" << m_RowCount << " Col:" << m_ColCount << endl;

		m_MapWidth = m_ColCount * m_BlockWidth;
		m_MapHeight = m_RowCount * m_BlockHeight;

		// m_MapPixelsRGB24 = new uint8_t[m_RowCount*m_ColCount * 320 * 240 * 3];

		// Read Unit
		m_UnitSize = m_RowCount * m_ColCount;
		m_MapUnits.resize(m_UnitSize);
		m_UnitIndecies.resize(m_UnitSize, 0);
		MEM_READ_WITH_OFF(fileOffset, m_UnitIndecies.data(), m_FileData, m_UnitSize * 4);

		// Read Mask
		MEM_READ_WITH_OFF(fileOffset, &m_MaskHeader, m_FileData, sizeof(MaskHeader));
		m_MaskSize = m_MaskHeader.Size;
		m_MaskInfos.resize(m_MaskSize);
		m_MaskIndecies.resize(m_MaskSize, 0);
		MEM_READ_WITH_OFF(fileOffset, m_MaskIndecies.data(), m_FileData, m_MaskSize * 4);

		DecodeMapUnits();

		DecodeMapMasks();

		//cout << "MAP init success!" << endl;
	}

	MAP::~MAP()
	{

	}

	void MAP::SaveImageFile(const char* filename, int width, int height, int pixelDepth, char* data)
	{
		TGA_FILE_HEADER TgaHeader;
		memset(&TgaHeader, 0, 18);
		TgaHeader.IdLength = 0;
		TgaHeader.ColorMapType = 0;
		TgaHeader.ImageType = 0x02;
		TgaHeader.ColorMapFirstIndex = 0;
		TgaHeader.ColorMapLength = 0;
		TgaHeader.ColorMapEntrySize = 0;
		TgaHeader.XOrigin = 0;
		TgaHeader.YOrigin = 0;
		TgaHeader.ImageWidth = width;
		TgaHeader.ImageHeight = height;
		TgaHeader.PixelDepth = pixelDepth;
		TgaHeader.ImageDescruptor = 8;

		std::fstream ofile;
		ofile.open(filename, ios::out | ios::trunc | ios::binary);
		ofile.write((char*)(&TgaHeader), sizeof(TGA_FILE_HEADER));
		ofile.write((char*)data, width*height*pixelDepth / 8);
		ofile.close();
	}

	// 2 bytes high bit swap 
	void MAP::ByteSwap(uint16_t& value)
	{
		uint16_t tempvalue = value >> 8;
		value = (value << 8) | tempvalue;
	}

	size_t MAP::DecompressMask(void* in, void* out)
	{
		uint8_t *op;
		uint8_t *ip;
		unsigned t;
		uint8_t *m_pos;

		op = (uint8_t *)out;
		ip = (uint8_t *)in;

		if (*ip > 17) {
			t = *ip++ - 17;
			if (t < 4)
				goto match_next;
			do *op++ = *ip++; while (--t > 0);
			goto first_literal_run;
		}

		while (1) {
			t = *ip++;
			if (t >= 16) goto match;
			if (t == 0) {
				while (*ip == 0) {
					t += 255;
					ip++;
				}
				t += 15 + *ip++;
			}

			*(unsigned *)op = *(unsigned *)ip;
			op += 4; ip += 4;
			if (--t > 0)
			{
				if (t >= 4)
				{
					do {
						*(unsigned *)op = *(unsigned *)ip;
						op += 4; ip += 4; t -= 4;
					} while (t >= 4);
					if (t > 0) do *op++ = *ip++; while (--t > 0);
				}
				else do *op++ = *ip++; while (--t > 0);
			}

		first_literal_run:

			t = *ip++;
			if (t >= 16)
				goto match;

			m_pos = op - 0x0801;
			m_pos -= t >> 2;
			m_pos -= *ip++ << 2;

			*op++ = *m_pos++; *op++ = *m_pos++; *op++ = *m_pos;

			goto match_done;

			while (1)
			{
			match:
				if (t >= 64)
				{

					m_pos = op - 1;
					m_pos -= (t >> 2) & 7;
					m_pos -= *ip++ << 3;
					t = (t >> 5) - 1;

					goto copy_match;

				}
				else if (t >= 32)
				{
					t &= 31;
					if (t == 0) {
						while (*ip == 0) {
							t += 255;
							ip++;
						}
						t += 31 + *ip++;
					}

					m_pos = op - 1;
					m_pos -= (*(unsigned short *)ip) >> 2;
					ip += 2;
				}
				else if (t >= 16) {
					m_pos = op;
					m_pos -= (t & 8) << 11;
					t &= 7;
					if (t == 0) {
						while (*ip == 0) {
							t += 255;
							ip++;
						}
						t += 7 + *ip++;
					}
					m_pos -= (*(unsigned short *)ip) >> 2;
					ip += 2;
					if (m_pos == op)
						goto eof_found;
					m_pos -= 0x4000;
				}
				else {
					m_pos = op - 1;
					m_pos -= t >> 2;
					m_pos -= *ip++ << 2;
					*op++ = *m_pos++; *op++ = *m_pos;
					goto match_done;
				}

				if (t >= 6 && (op - m_pos) >= 4) {
					*(unsigned *)op = *(unsigned *)m_pos;
					op += 4; m_pos += 4; t -= 2;
					do {
						*(unsigned *)op = *(unsigned *)m_pos;
						op += 4; m_pos += 4; t -= 4;
					} while (t >= 4);
					if (t > 0) do *op++ = *m_pos++; while (--t > 0);
				}
				else {
				copy_match:
					*op++ = *m_pos++; *op++ = *m_pos++;
					do *op++ = *m_pos++; while (--t > 0);
				}

			match_done:

				t = ip[-2] & 3;
				if (t == 0)	break;

			match_next:
				do *op++ = *ip++; while (--t > 0);
				t = *ip++;
			}
		}

	eof_found:
		return (op - (uint8_t*)out);
	}

	void MAP::MapHandler(uint8_t* Buffer, uint32_t inSize, uint8_t* outBuffer, uint32_t* outSize)
	{
		// JPEG数据处理原理
		// 1、复制D8到D9的数据到缓冲区中
		// 2、删除第3、4个字节 FFA0
		// 3、修改FFDA的长度00 09 为 00 0C
		// 4、在FFDA数据的最后添加00 3F 00
		// 5、替换FFDA到FF D9之间的FF数据为FF 00

		uint32_t TempNum = 0;						// 临时变量，表示已读取的长度
		uint16_t TempTimes = 0;					// 临时变量，表示循环的次数
		uint32_t Temp = 0;

		// 当已读取数据的长度小于总长度时继续
		while (TempNum < inSize && *Buffer++ == 0xFF)
		{
			*outBuffer++ = 0xFF;
			TempNum++;
			switch (*Buffer)
			{
			case 0xD8:
				*outBuffer++ = 0xD8;
				Buffer++;
				TempNum++;
				break;
			case 0xA0:
				Buffer++;
				outBuffer--;
				TempNum++;
				break;
			case 0xC0:
				*outBuffer++ = 0xC0;
				Buffer++;
				TempNum++;

				memcpy(&TempTimes, Buffer, sizeof(uint16_t)); // 读取长度
				ByteSwap(TempTimes); // 将长度转换为Intel顺序


				for (int i = 0; i < TempTimes; i++)
				{
					*outBuffer++ = *Buffer++;
					TempNum++;
				}

				break;
			case 0xC4:
				*outBuffer++ = 0xC4;
				Buffer++;
				TempNum++;

				memcpy(&TempTimes, Buffer, sizeof(uint16_t)); // 读取长度
				ByteSwap(TempTimes); // 将长度转换为Intel顺序

				for (int i = 0; i < TempTimes; i++)
				{
					*outBuffer++ = *Buffer++;
					TempNum++;
				}
				break;
			case 0xDB:
				*outBuffer++ = 0xDB;
				Buffer++;
				TempNum++;

				memcpy(&TempTimes, Buffer, sizeof(uint16_t)); // 读取长度
				ByteSwap(TempTimes); // 将长度转换为Intel顺序

				for (int i = 0; i < TempTimes; i++)
				{
					*outBuffer++ = *Buffer++;
					TempNum++;
				}
				break;
			case 0xDA:
				*outBuffer++ = 0xDA;
				*outBuffer++ = 0x00;
				*outBuffer++ = 0x0C;
				Buffer++;
				TempNum++;

				memcpy(&TempTimes, Buffer, sizeof(uint16_t)); // 读取长度
				ByteSwap(TempTimes); // 将长度转换为Intel顺序
				Buffer++;
				TempNum++;
				Buffer++;

				for (int i = 2; i < TempTimes; i++)
				{
					*outBuffer++ = *Buffer++;
					TempNum++;
				}
				*outBuffer++ = 0x00;
				*outBuffer++ = 0x3F;
				*outBuffer++ = 0x00;
				Temp += 1; // 这里应该是+3的，因为前面的0xFFA0没有-2，所以这里只+1。

						   // 循环处理0xFFDA到0xFFD9之间所有的0xFF替换为0xFF00
				for (; TempNum < inSize - 2;)
				{
					if (*Buffer == 0xFF)
					{
						*outBuffer++ = 0xFF;
						*outBuffer++ = 0x00;
						Buffer++;
						TempNum++;
						Temp++;
					}
					else
					{
						*outBuffer++ = *Buffer++;
						TempNum++;
					}
				}
				// 直接在这里写上了0xFFD9结束Jpeg图片.
				Temp--; // 这里多了一个字节，所以减去。
				outBuffer--;
				*outBuffer-- = 0xD9;
				break;
			case 0xD9:
				// 算法问题，这里不会被执行，但结果一样。
				*outBuffer++ = 0xD9;
				TempNum++;
				break;
			default:
				break;
			}
		}
		Temp += inSize;
		*outSize = Temp;
	}


	bool MAP::ReadJPEG(uint32_t& offset, uint32_t size, uint32_t index)
	{
		std::vector<uint8_t> jpegData(size, 0);
		MEM_READ_WITH_OFF(offset, jpegData.data(), m_FileData, size);

		m_MapUnits[index].JPEGRGB24.resize(size * 2, 0);
		uint32_t tmpSize = 0;
		MapHandler(jpegData.data(), size, m_MapUnits[index].JPEGRGB24.data(), &tmpSize);
		m_MapUnits[index].JPEGRGB24.resize(tmpSize);
		return true;
	}


	void MAP::DecodeMapUnits()
	{
		for (size_t i = 0; i < m_MapUnits.size(); i++)
		{
			uint32_t fileOffset = m_UnitIndecies[i];
			uint32_t eat_num;
			MEM_READ_WITH_OFF(fileOffset, &eat_num, m_FileData, sizeof(uint32_t));
			fileOffset += eat_num * 4;
			bool loop = true;
			while (loop)
			{
				MapUnitHeader unitHeader{ 0 };
				MEM_READ_WITH_OFF(fileOffset, &unitHeader, m_FileData, sizeof(MapUnitHeader));
				switch (unitHeader.Flag)
				{
				case 0x4A504547:
				{
					m_MapUnits[i].JpegOffset = fileOffset;
					m_MapUnits[i].JpegSize = unitHeader.Size;
					fileOffset += unitHeader.Size;
					break;
				}
				case 0x43454C4C:
					ReadCELL(fileOffset, (uint32_t)unitHeader.Size, (uint32_t)i);
					break;
				case 0x42524947:
					fileOffset += unitHeader.Size;
					break;
				default:
					loop = false;
					break;
				}
			}
		}
	}

	void MAP::DecodeMapMasks()
	{
		for (size_t index = 0; index < m_MaskSize; index++)
		{
			uint32_t offset = m_MaskIndecies[index];

			BaseMaskInfo baseMaskInfo;//& maskInfo = m_MaskInfos[index];
			MEM_READ_WITH_OFF(offset, &baseMaskInfo, m_FileData, sizeof(BaseMaskInfo));

			MaskInfo& maskInfo = m_MaskInfos[index];
			maskInfo.StartX = baseMaskInfo.StartX;
			maskInfo.StartY = baseMaskInfo.StartY;
			maskInfo.Width = baseMaskInfo.Width;
			maskInfo.Height = baseMaskInfo.Height;
			maskInfo.Size = baseMaskInfo.Size;

			int occupyRowStart = maskInfo.StartY / m_BlockHeight;
			int occupyRowEnd = (maskInfo.StartY + maskInfo.Height) / m_BlockHeight;

			int occupyColStart = maskInfo.StartX / m_BlockWidth;
			int occupyColEnd = (maskInfo.StartX + maskInfo.Width) / m_BlockWidth;

			for (int i = occupyRowStart; i <= occupyRowEnd; i++)
				for (int j = occupyColStart; j <= occupyColEnd; j++)
				{
					int unit = i * m_ColCount + j;
					if (unit >= 0 && unit < m_MapUnits.size())
					{
						maskInfo.OccupyUnits.insert(unit);
						m_MapUnits[unit].OwnMasks.insert((int)index);
					}
				}
		}
	}

	bool MAP::ReadCELL(uint32_t& offset, uint32_t size, uint32_t index)
	{
		m_MapUnits[index].Cell.resize(size, 0);
		MEM_READ_WITH_OFF(offset, m_MapUnits[index].Cell.data(), m_FileData, size);
		return true;
	}

	bool MAP::ReadBRIG(uint32_t& offset, uint32_t size, uint32_t index)
	{
		offset += size;
		return true;
	}

	void MAP::SaveUnit(int index)
	{
		ReadUnit(index);
		std::string filename = "MAP_unit_" + std::to_string(index) + ".tga";
		SaveImageFile(filename.c_str(), 320, 240, 24, (char*)m_MapUnits[index].JPEGRGB24.data());
	}

	void MAP::ReadUnit(int index)
	{
		if (m_MapUnits[index].bHasLoad || m_MapUnits[index].bLoading) {
			return;
		}
		m_MapUnits[index].bLoading = true;

		ReadJPEG(m_MapUnits[index].JpegOffset, m_MapUnits[index].JpegSize, index);

		m_MapUnits[index].Index = index;
		m_MapUnits[index].bHasLoad = true;
		m_MapUnits[index].bLoading = false;
	}

	void MAP::ReadMask(int index)
	{
		if (m_MaskInfos[index].bHasLoad || m_MaskInfos[index].bLoading) {
			return;
		}
		m_MaskInfos[index].bLoading = true;

		uint32_t fileOffset = m_MaskIndecies[index];

		fileOffset += sizeof(BaseMaskInfo);

		MaskInfo& maskInfo = m_MaskInfos[index];
		std::vector<char> pData(maskInfo.Size, 0);
		MEM_READ_WITH_OFF(fileOffset, pData.data(), m_FileData, maskInfo.Size);

		int align_width = (maskInfo.Width / 4 + (maskInfo.Width % 4 != 0)) * 4;	// align 4 bytes
		std::vector<char> pMaskDataDec(align_width * maskInfo.Height / 4, 0);
		DecompressMask(pData.data(), pMaskDataDec.data());

		int pixel_num = maskInfo.Width * maskInfo.Height;
		maskInfo.Data.resize(pixel_num, 0);
		for (uint32_t h = 0; h < maskInfo.Height; h++)
		{
			for (uint32_t w = 0; w < maskInfo.Width; w++)
			{
				int mask_index = (h * align_width + w) * 2;		
				uint8_t mask_value = pMaskDataDec[mask_index / 8];	
				mask_value = (mask_value >> (mask_index % 8));	
				if ((mask_value & 3) == 3) {
					// int bmpIndex_y = (maskInfo.StartY+h)*m_MapWidth * 3;
					// int bmpIndex_x = (maskInfo.StartX+w) * 3;
					// int bmpIndex = bmpIndex_y + bmpIndex_x;
					//uint8_t r = m_MapPixelsRGB24[bmpIndex];
					//uint8_t g = m_MapPixelsRGB24[bmpIndex + 1];
					//uint8_t b  = m_MapPixelsRGB24[bmpIndex + 2];
					//pOutMaskBmp[h*maskInfo.Width + w] = ( 0x80 << 24 )| (b<< 16)| (g << 8 )| r ;
					maskInfo.Data[h*maskInfo.Width + w] = (0x80 << 24);
				}
				else {
					//pOutMaskBmp[h*maskInfo.Width + w] = ( 0x00 << 24 )| (b<< 16)| (g << 8 )| r ;
					maskInfo.Data[h*maskInfo.Width + w] = (0x00 << 24);
				}
			}
		}

		m_MaskInfos[index].bHasLoad = true;
		m_MaskInfos[index].bLoading = false;
	}

	void MAP::PrintCellMap()
	{
		int** cells;
		int** mat;
		int mat_row, mat_col;
		int row = m_RowCount;
		int col = m_ColCount;
		cells = new int*[row*col];
		for (int i = 0; i < row*col; i++) {
			cells[i] = new int[192];
		}

		mat_row = row * 12;
		mat_col = col * 16;

		//printf("%d %d \n", mat_row, mat_col);

		mat = new int*[row * 12];
		for (int i = 0; i < row * 12; i++) {
			mat[i] = new int[16 * col];
		}

		for (int i = 0; i < row; i++) {
			for (int j = 0; j < col; j++) {
				ReadUnit(i*col + j);
				for (int k = 0; k < 192; k++) {
					cells[i*col + j][k] = (m_MapUnits)[i*col + j].Cell[k];
				}
				int startMat_i = i * 12;
				int startMat_j = j * 16;
				for (int p = 0; p < 12; p++) {
					for (int q = 0; q < 16; q++) {
						mat[startMat_i + p][startMat_j + q] = cells[i*col + j][p * 16 + q];
					}
				}
			}
		}

		for (int i = 0; i < mat_row; i++) {
			for (int j = 0; j < mat_col; j++) {
				//	printf("%d", mat[i][j]);
			}
			//printf("\n");
		}

	}
}
