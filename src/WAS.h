#ifndef WAS_H
#define WAS_H 
#include <stdint.h>
#include <string>
#include <fstream>
#include <memory.h>
using namespace std;




class WAS
{
public:

	// ���鶯�����ļ�ͷ
	struct Header
	{
		uint16_t flag;		// �����ļ���־ SP 0x5053
		uint16_t len;		// �ļ�ͷ�ĳ��� Ĭ��Ϊ 12
		uint16_t group;		// ����ͼƬ����������������
		uint16_t frame;		// ÿ���ͼƬ������֡��
		uint16_t width;		// ���鶯���Ŀ�ȣ���λ����
		uint16_t height;		// ���鶯���ĸ߶ȣ���λ����
		uint16_t key_x;		// ���鶯���Ĺؼ�λX
		uint16_t key_y;		// ���鶯���Ĺؼ�λY
	};

	// ֡���ļ�ͷ
	struct FrameHeader
	{
		int32_t key_x;			// ͼƬ�Ĺؼ�λX
		int32_t key_y;			// ͼƬ�Ĺؼ�λY
		int32_t width;			// ͼƬ�Ŀ�ȣ���λ����
		int32_t height;			// ͼƬ�ĸ߶ȣ���λ����
	};


	WAS(std::string filename, int offset, int size);
	WAS(fstream &infile, int offset, int size);
	static uint32_t RGB565to888(uint16_t color, uint8_t alpha); // 565ת888
	static uint16_t Alpha565(uint16_t src, uint16_t des, uint8_t alpha);
	static uint8_t MixAlpha(uint8_t color, uint8_t alpha);
	~WAS();
	Header mHeader;
	uint32_t mPalette32[256];
	uint32_t* mFrameIndecies;
	std::string mFileName;
	uint32_t mFileOffset;
	uint32_t mFileSize;

};
#endif
