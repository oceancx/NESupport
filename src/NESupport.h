#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <map>
#include <memory>

namespace NE {
	struct Sprite
	{
		struct Sequence
		{
			int key_x;
			int key_y;
			int width;
			int height;
			uint32_t format;

			std::vector<uint32_t> src;
			bool IsBlank;
		};

		int mGroupSize;	
		int mFrameSize;	
		int mWidth;		
		int mHeight;	
		int mKeyX;		
		int mKeyY;		
		std::string mID;
		std::string mPath;
		std::vector<Sequence> mFrames;
		void SaveImage(int index);
	};


	class WAS
	{
	public:
		
		struct Header
		{
			uint16_t flag;		// 精灵文件标志 SP 0x5053
			uint16_t len;		// 文件头的长度 默认为 12
			int16_t group;		// 精灵图片的组数，即方向数
			int16_t frame;		// 每组的图片数，即帧数
			int16_t width;		// 精灵动画的宽度，单位像素
			int16_t height;		// 精灵动画的高度，单位像素
			int16_t key_x;		// 精灵动画的关键位X
			int16_t key_y;		// 精灵动画的关键位Y
		};

		struct FrameHeader
		{
			int32_t key_x;				// 图片的关键位X
			int32_t key_y;				// 图片的关键位Y
			int32_t width;				// 图片的宽度，单位像素
			int32_t height;				// 图片的高度，单位像素
		};


		WAS(std::string filename);
		WAS(std::string path, uint32_t offset);
		
		~WAS();
		Header mHeader;
		uint32_t mPalette32[256];
		std::vector<uint32_t> mFrameIndecies;
		std::string mPath;

	};

	class WDF
	{
	
		struct Header
		{
			//flag 			0x57444650: // WDFP
			//flag			0x57444658: // WDFX
			//flag			0x57444648: // WDFH
			uint32_t flag; 	 
			//number   was图片数
			uint32_t number;
			//WAS图片集合数据的偏移地址
			uint32_t offset; 
			
		};

		struct Index
		{
			uint32_t hash;	// WAS文件的名字散列
			uint32_t offset; // WAS文件的偏移
			uint32_t size;	//  WAS文件大小
			uint32_t spaces; // WAS文件空间
		};

	


	public:

		WDF(std::string path);
		
		void DataHandler(uint8_t *pData, uint32_t* pBmpStart, int pixelOffset, int pixelLen,int y, bool& copyline);
		void Init();

		WAS GetWAS(uint32_t id);
		void SaveWAS(uint32_t id);
		
		Sprite*  LoadSprite(uint32_t id);

		std::vector<uint32_t> GetAllWASIDs()
		{
			std::vector<uint32_t> ids;
			for(uint32_t i=0;i<mIndencies.size();i++)
			{
				ids.push_back(mIndencies[i].hash);
			}
			return ids;
		}
		
		std::vector<Sprite *> LoadAllSprite();

		~WDF();

	public:


		std::vector<Index> mIndencies;
		std::map<uint32_t, uint32_t> mIdToPos;

		uint16_t m_Palette16[256];
		std::string m_Path;
		std::string m_WDFDir;
		uint32_t m_Palette32[256];
		std::string m_FileName;
		uint32_t m_FileDataOffset;
		uint32_t m_WASNumber;
		uint32_t m_FileType;

		std::vector<uint8_t> m_FileData;
		std::uint64_t m_FileSize;

		std::map<uint32_t,Sprite> m_Sprites;
		
	};



	
	/*
	地图读取类: xx.scene文件是一个大地图
	大地图被分成了N个320x240的小切片
	地图文件中的信息：

	vec<MapUnit>
	vec<MapMask>
	Fullbitmap

	类创建时，先读取数据信息，然后关闭文件句柄。
	需要使用位图信息时，再重新打开文件，读取位图信息。

	*/
	class MAP
	{
		public:
		// 地图的文件头(梦幻、大话2)
		struct MapHeader
		{
			//public:
			// 文件头结构共12字节
			uint32_t		Flag;		//文件标志
			uint32_t		Width;		//地图宽
			uint32_t		Height;		//地图高	
		};

		// 地图的单元头
		struct MapUnitHeader
		{
			//public:
			uint32_t		Flag;		// 单元标志
			uint32_t		Size;		// 单元大小
		};

		// 地图的数据
		struct MapData
		{
			//public:
			uint32_t		Size;		// 数据大小
			uint8_t		*Data;		// 数据内容
		};

		struct MaskHeader
		{
			uint32_t	Flag;
			int	Size;
			MaskHeader()
				:Flag(0),
				Size(0)
			{

			}
		};

		struct BaseMaskInfo
		{
			int	StartX;
			int	StartY;
			uint32_t	Width;
			uint32_t	Height;
			uint32_t	Size;	// 遮罩大小
			BaseMaskInfo()
				:StartX(0),
				StartY(0),
				Width(0),
				Height(0),
				Size(0)
			{
				
			}
		};

		struct MaskInfo : BaseMaskInfo
		{
			std::vector<uint32_t> Data;
		};

		// struct UnKnown
		// {
		// 	uint32_t Offset;
		// 	uint32_t *Data;			
		// };

		struct MapUnit
		{
			std::vector<uint8_t>  Cell;
			std::vector<uint8_t> JPEGRGB24;
			uint32_t Size;
			uint32_t Index;
			bool bHasLoad = false;
		};

		MAP(std::string filename);
		
		~MAP();
		
		//row = index / colcount , col = index % colcount 
		void ReadUnit(int index);

		void ReadUnit(int row, int col) { ReadUnit(row*m_ColCount + col); };
		
		void SaveUnit(int index);

		void ReadMask(int index);

		void PrintCellMap();

		void SaveImageFile(char* filename, int width, int height, int pixelDepth, char* data);

		int MapWidth() {return m_MapWidth;};
		int MapHeight() {return m_MapHeight;};
		int SliceWidth() {return m_Width;};
		int SliceHeight() {return m_Height;};
		int Row(){return m_RowCount;};
		int Col(){return m_ColCount;};
		int UnitSize(){return m_UnitSize;};
		int MaskSize(){return m_MaskSize;};

		int GetMaskWidth(int index) { return m_MaskInfos[index].Width; };
		int GetMaskHeight(int index) { return m_MaskInfos[index].Height; };
		
		MaskInfo& GetMask(int index) { return m_MaskInfos[index];};
		MapUnit& GetUnit(int index) { return m_MapUnits[index];};
		bool HasUnitLoad(int index) { return m_MapUnits[index].bHasLoad;};

		uint32_t* GetMaskBitmap(int index) { return m_MaskInfos[index].Data.data();};
		uint8_t* GetUnitBitmap(int index) { return m_MapUnits[index].JPEGRGB24.data();};
		size_t GetUnitBitmapSize(int index) { return m_MapUnits[index].JPEGRGB24.size();};
		uint8_t* Bitmap(){return m_MapPixelsRGB24.data();};
		
	
private:

		void ByteSwap(uint16_t& value);
		size_t DecompressMask(void* in, void* out);
		void MapHandler(uint8_t* Buffer, uint32_t inSize,uint8_t* outBuffer, uint32_t* outSize);

		bool ReadJPEG(std::ifstream &file, uint32_t size, uint32_t index);

		bool ReadCELL(std::ifstream &file, uint32_t size, uint32_t index);

		bool ReadBRIG(std::ifstream &file, uint32_t size, uint32_t index);
		
	
		std::string m_FileName;

		int m_Width;

		int m_Height;

		int m_MapWidth;

		int m_MapHeight;

		int m_BlockWidth;

		int m_BlockHeight;

		uint32_t m_RowCount;

		uint32_t m_ColCount;

		// uint32_t m_Pixcels;

		MapHeader m_Header;

		std::vector<uint32_t> m_UnitIndecies;

		uint32_t m_UnitSize;

		MaskHeader m_MaskHeader;

		std::vector<uint32_t> m_MaskIndecies;

		uint32_t m_MaskSize;

		std::vector<MapUnit> m_MapUnits;

		std::vector<MaskInfo> m_MaskInfos;

		std::vector<uint8_t> m_MapPixelsRGB24;		//大地图位图
		//uint8_t* m_Cur_MapPixelsRGB24;
	};
}
using Sprite = NE::Sprite;