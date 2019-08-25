#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <map>
#include <memory>
#include <set>

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
		void SaveImage(const char* filename,int index);
		uint32_t FrameWASOffset;
		bool FrameLoaded;
	};


	class WAS
	{
	public:

		struct Header
		{
			uint16_t flag;		// SP 0x5053
			uint16_t len;		// 12
			int16_t group;
			int16_t frame;
			int16_t width;
			int16_t height;
			int16_t key_x;
			int16_t key_y;
		};

		struct FrameHeader
		{
			int32_t key_x;
			int32_t key_y;
			int32_t width;
			int32_t height;
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
			//0x57444650:WDFP	0x57444658:WDFX		0x57444648:WDFH
			uint32_t flag;

			uint32_t number;

			uint32_t offset;
		};

		struct Index
		{
			uint32_t hash;
			uint32_t offset;
			uint32_t size;
			uint32_t spaces;
		};

	public:
		WDF(std::string path);

		void DataHandler(uint8_t *pData, uint32_t* pBmpStart, int pixelOffset, int pixelLen, int y, bool& copyline);

		WAS GetWAS(uint32_t id);
		void SaveWAS(uint32_t id, const char* path);

		Sprite* LoadSprite(uint32_t id);
		void UnLoadSprite(uint32_t id);

		Sprite* LoadSpriteHeader(uint32_t id);
		bool LoadSpriteData(Sprite* sprite);

		std::vector<uint32_t> GetAllWASIDs()
		{
			std::vector<uint32_t> ids;
			for (uint32_t i = 0; i < mIndencies.size(); i++)
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

		std::map<uint32_t, Sprite> m_Sprites;
		std::map<uint32_t, bool> m_SpritesLoading;
		std::map<uint32_t, bool> m_SpritesLoaded;

	};

	class MAP
	{
	public:
		struct MapHeader
		{
			uint32_t		Flag;
			uint32_t		Width;
			uint32_t		Height;
		};

		struct MapUnitHeader
		{
			uint32_t		Flag;
			uint32_t		Size;
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
			uint32_t	Size;
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
			std::set<int> OccupyUnits;
			bool bHasLoad = false;
			bool bLoading = false;
		};

		struct MapUnit
		{
			std::vector<uint8_t>  Cell;
			std::vector<uint8_t> JPEGRGB24;
			uint32_t Size;
			uint32_t Index;
			bool bHasLoad = false;
			bool bLoading = false;
			uint32_t JpegSize;
			uint32_t JpegOffset;
			std::set<int> OwnMasks;
		};

		MAP(std::string filename);

		~MAP();

		void DecodeMapUnits();
		void DecodeMapMasks();

		void ReadUnit(int index);

		void ReadUnit(int row, int col) { ReadUnit(row*m_ColCount + col); };

		void SaveUnit(int index);

		void ReadMask(int index);

		void PrintCellMap();

		void SaveImageFile(const char* filename, int width, int height, int pixelDepth, char* data);

		int MapWidth() { return m_MapWidth; };
		int MapHeight() { return m_MapHeight; };
		int SliceWidth() { return m_Width; };
		int SliceHeight() { return m_Height; };
		int Row() { return m_RowCount; };
		int Col() { return m_ColCount; };
		int UnitSize() { return m_UnitSize; };
		int MaskSize() { return m_MaskSize; };

		int GetMaskWidth(int index) { return m_MaskInfos[index].Width; };
		int GetMaskHeight(int index) { return m_MaskInfos[index].Height; };

		MaskInfo& GetMask(int index) { return m_MaskInfos[index]; };
		MapUnit& GetUnit(int index) { return m_MapUnits[index]; };
		bool HasUnitLoad(int index) { return m_MapUnits[index].bHasLoad; };
		bool IsUnitLoading(int index) { return m_MapUnits[index].bLoading; };

		uint32_t* GetMaskBitmap(int index) { return m_MaskInfos[index].Data.data(); };
		uint8_t* GetUnitBitmap(int index) { return m_MapUnits[index].JPEGRGB24.data(); };
		size_t GetUnitBitmapSize(int index) { return m_MapUnits[index].JPEGRGB24.size(); };
	private:

		void ByteSwap(uint16_t& value);

		size_t DecompressMask(void* in, void* out);

		void MapHandler(uint8_t* Buffer, uint32_t inSize, uint8_t* outBuffer, uint32_t* outSize);

		bool ReadJPEG(uint32_t& offset, uint32_t size, uint32_t index);

		bool ReadCELL(uint32_t& offset, uint32_t size, uint32_t index);

		bool ReadBRIG(uint32_t& offset, uint32_t size, uint32_t index);


		std::string m_FileName;

		int m_Width;

		int m_Height;

		int m_MapWidth;

		int m_MapHeight;

		int m_BlockWidth;

		int m_BlockHeight;

		uint32_t m_RowCount;

		uint32_t m_ColCount;

		MapHeader m_Header;

		std::vector<uint32_t> m_UnitIndecies;

		uint32_t m_UnitSize;

		MaskHeader m_MaskHeader;

		std::vector<uint32_t> m_MaskIndecies;

		uint32_t m_MaskSize;

		std::vector<MapUnit> m_MapUnits;

		std::vector<MaskInfo> m_MaskInfos;

		std::vector<uint8_t> m_FileData;

		std::uint64_t m_FileSize;

		std::vector<std::vector<uint8_t>> m_CellData;
	};
}
using Sprite = NE::Sprite;