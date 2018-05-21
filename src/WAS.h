#pragma once
#include <stdint.h>
#include <string>
#include <fstream>
#include <memory.h>


class WAS
{
public:
	
	struct Header
	{
		uint16_t flag;		// 精灵文件标志 SP 0x5053
		uint16_t len;		// 文件头的长度 默认为 12
		uint16_t group;		// 精灵图片的组数，即方向数
		uint16_t frame;		// 每组的图片数，即帧数
		uint16_t width;		// 精灵动画的宽度，单位像素
		uint16_t height;	// 精灵动画的高度，单位像素
		uint16_t key_x;		// 精灵动画的关键位X
		uint16_t key_y;		// 精灵动画的关键位Y
	};

	struct FrameHeader
	{
		int32_t key_x;				// 图片的关键位X
		int32_t key_y;				// 图片的关键位Y
		int32_t width;				// 图片的宽度，单位像素
		int32_t height;			// 图片的高度，单位像素
	};


	WAS(std::string filename, int offset, int size);
	WAS(std::fstream &infile, int offset, int size);
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
