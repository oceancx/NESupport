#ifndef SPRITE2_H
#define SPRITE2_H 
#include <stdint.h>
#include <vector>
#include <string>
/*

*/
#ifndef TGA_FILE_HEADER_H
#define TGA_FILE_HEADER_H
#pragma pack(push)
#pragma pack(1)
struct TGA_FILE_HEADER
{
    uint8_t IdLength;
    uint8_t ColorMapType;
    uint8_t ImageType;
    uint16_t ColorMapFirstIndex;
    uint16_t ColorMapLength;
    uint8_t ColorMapEntrySize;
    uint16_t XOrigin;
    uint16_t YOrigin;
    uint16_t ImageWidth;
    uint16_t ImageHeight;
    uint8_t PixelDepth;
    uint8_t ImageDescruptor;
};
#pragma pack(pop)
#endif

class Sprite2
{

public:
    Sprite2();
	~Sprite2();

	struct Sequence
	{
		int key_x;
		int key_y;
		int width;
		int height;
		uint32_t format;
		uint32_t* src;
		bool IsBlank;
	};

	int mGroupSize;		//������
	int mFrameSize;		//֡��
	int mWidth;			//���
	int mHeight;		//�߶�
	int mKeyX;			//�ؼ�֡X
	int mKeyY;			//�ؼ�֡Y
    std::string mID;
	std::string mPath;
	Sequence** mFrames;
	bool Error;
	void SaveImage(int index);
};
#endif
