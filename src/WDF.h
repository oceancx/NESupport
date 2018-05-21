#pragma once

#include <map>
#include <fstream>
#include "WAS.h"
#include "Sprite.h"
#include <memory>

using String = std::string;
namespace NetEase {

	

	class WDF
	{
		struct WASFileBlock
		{
			uint8_t* data;
			uint32_t size;
			uint32_t offset;
		};
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

		struct ImageData
		{
			uint8_t* data;
			uint32_t size;
		};


	public:
		WDF();
		WDF(std::string path) :m_Path(path) { Init(); }
		
		void DataHandler(char *pData, uint32_t* pBmpStart, int pixelOffset, int pixelLen,int y, bool& copyline);
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

		std::map<String, ImageData> m_ImageSet;

		uint16_t m_Palette16[256];
		std::string m_Path;
		uint32_t m_Palette32[256];
		std::string m_FileName;
		uint32_t m_FileDataOffset;
		uint32_t m_WASNumber;
		uint32_t m_FileType;

		std::vector<uint8_t> m_FileData;
		std::uint64_t m_FileSize;

		std::map<uint32_t,Sprite> m_Sprites;
		std::vector<WASFileBlock> m_WASFileBlocks;
		
	};
}
