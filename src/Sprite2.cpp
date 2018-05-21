#include "Sprite2.h"
#include <memory.h>
#include <fstream>
#include <iostream>
#include "WAS.h"
using namespace std;



Sprite2::Sprite2()
:Error(false)
{

}

Sprite2::~Sprite2()
{
	for (auto& f : mFrames)
	{
		if (f.src)
		{
			delete f.src;
			f.src = nullptr;
		}
	}
	mFrames.clear();
}


void Sprite2::SaveImage(int index)
{
	TGA_FILE_HEADER TgaHeader;
	memset(&TgaHeader, 0, 18);
	TgaHeader.IdLength = 0;			// 图像信息字段(默认:0)
	TgaHeader.ColorMapType = 0;		// 颜色标的类型(默认0)
	TgaHeader.ImageType = 0x02;			// 图像类型码(支持2或10)
	TgaHeader.ColorMapFirstIndex = 0;	// 颜色表的引索(默认:0)
	TgaHeader.ColorMapLength = 0;		// 颜色表的长度(默认:0)
	TgaHeader.ColorMapEntrySize = 0;	// 颜色表表项的为数(默认:0，支持16/24/32)
	TgaHeader.XOrigin = 0;				// 图像X坐标的起始位置(默认:0)
	TgaHeader.YOrigin = 0;				// 图像Y坐标的起始位置(默认:0)
	TgaHeader.ImageWidth = mWidth;			// 图像的宽度
	TgaHeader.ImageHeight = mHeight;			// 图像的高度
	TgaHeader.PixelDepth = 32;			// 图像每像素存储占用位数
	TgaHeader.ImageDescruptor = 8;		// 图像描述字符字节(默认:0)

	char outfilename[255];
	int gpos = index / mFrameSize;
	int cpos = index%mFrameSize;
	sprintf(outfilename, "mhxy%d_%d.tga", gpos, cpos);
	printf("%s\n", outfilename);
	
	ofstream ofile;
	ofile.open(outfilename, ios::out | ios::trunc | ios::binary);
	//cout << "写TGA图像文件头" << endl;
	ofile.write((char*)(&TgaHeader), sizeof(TGA_FILE_HEADER)); // 写TGA的文件头
	//char* data = (char*)mFrames[gpos][cpos].src;													   //cout << "图像文件头写完成，开始写图像数据。" << endl;
	//for (int i = 0; i < mHeight; i++)
	//{
	//	ofile.write((char*)&data[(mHeight -1- i)*mWidth*4], mWidth * 4);
	//}
	ofile.write((char*)mFrames[index].src, mWidth*mHeight * 4);
	cout << "完成 " << outfilename << " 帧图片输出~" << endl;
	ofile.close();
}
