#ifndef WDF_H
#define WDF_H 
// #include <string>
// #include <vector>
#include <map>
// #include <iostream>
#include <fstream>
#include "WAS.h"
#include "Sprite2.h"

// #include "Sprite.h"
using namespace std;
namespace NetEase {
	class WDF
	{
		struct Header
		{
			uint32_t flag; // �����ı�ǩ
			uint32_t number; // �����е��ļ�����
			uint32_t offset; // �����е��ļ��б�ƫ��λ��
		};

		struct Index
		{
			uint32_t hash; // �ļ�������ɢ��
			uint32_t offset; // �ļ���ƫ��
			uint32_t size; // �ļ��Ĵ�С
			uint32_t spaces; // �ļ��Ŀռ�
		};

	public:
		WDF();
		WDF(std::string path) :mFilePath(path) { Init(); }
		
		void DataHandler(char *pData, uint32_t* pBmpStart, int pixelOffset, int pixelLen,int y, bool& copyline);
		void Init();
		// Sprite LoadSprite(int id);
		WAS GetWAS(uint32_t id);
		void SaveWAS(uint32_t id);
		std::shared_ptr<Sprite2> LoadSprite(uint32_t id);
		std::vector<uint32_t> GetAllWASIDs()
		{
			std::vector<uint32_t> ids;
			for(int i=0;i<mHeader.number;i++)
			{
				ids.push_back(mIndencies[i].hash);
			}
			return ids;
		}
		std::vector<std::shared_ptr<Sprite2>> LoadAllSprite();
		~WDF();

	public:
		Header mHeader;
		Index* mIndencies;
		map<uint32_t, uint32_t> mIdToPos;
		fstream mFile;
		uint16_t palette16[256];
		string mFilePath;
		uint32_t m_Palette32[256];
		string mFileName;

	};
}
#endif
