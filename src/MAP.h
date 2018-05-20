#pragma once

#include <stdint.h>

#include <string>
#include <fstream>
#include <vector>
#include "Sprite2.h"

using std::string;
using std::ifstream;
using std::fstream;

namespace NetEase {

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
		uint32_t	Size;
	};

	struct BaseMaskInfo
	{
		int	StartX;
		int	StartY;
		uint32_t	Width;
		uint32_t	Height;
		uint32_t	Size;	// 遮罩大小

	};

	struct MaskInfo : BaseMaskInfo
	{
		uint32_t* Data;
	};

	struct UnKnown
	{
		uint32_t Offset;
		uint32_t *Data;			// 未知用途，大小为：第一个单元引索值减去文件头大小。
	};

	struct MapUnit
	{
		uint8_t  Cell[192];
		uint8_t* BitmapRGB24;
		uint32_t Size;
		uint32_t Index;
		bool bHasLoad = false;
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
		MAP(string filename);
		
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

		uint32_t* GetMaskBitmap(int index) { return m_MaskInfos[index].Data;};
		uint8_t* GetUnitBitmap(int index) { return m_MapUnits[index].BitmapRGB24;};
		uint32_t GetUnitBitmapSize(int index) { return m_MapUnits[index].Size;};
		uint8_t* Bitmap(){return m_MapPixelsRGB24;};
		
		void Destroy()
		{
			delete[] m_MapPixelsRGB24;
			m_MapPixelsRGB24= nullptr;
		};
private:
		uint8_t* MapHandler(uint8_t* jpegData, uint32_t inSize, uint32_t* outSize);

		bool ReadJPEG(ifstream &file, uint32_t size, uint32_t index);

		bool ReadCELL(ifstream &file, uint32_t size, uint32_t index);

		bool ReadBRIG(ifstream &file, uint32_t size, uint32_t index);
		
		ifstream m_FileStream;
		
		string m_FileName;

		int m_Width;

		int m_Height;

		int m_MapWidth;

		int m_MapHeight;

		float m_BlockWidth;

		float m_BlockHeight;

		uint32_t m_RowCount;

		uint32_t m_ColCount;

		uint32_t m_Pixcels;

		MapHeader m_Header;

		uint32_t* m_UnitIndecies;

		uint32_t m_UnitSize;

		MaskHeader m_MaskHeader;

		uint32_t* m_MaskIndecies;

		uint32_t m_MaskSize;

		std::vector<MapUnit> m_MapUnits;

		std::vector<MaskInfo> m_MaskInfos;

		uint8_t* m_MapPixelsRGB24;		//大地图位图
		//uint8_t* m_Cur_MapPixelsRGB24;
	};
}

/************************************************************************
Map File Old Format
Dali Wang <wdl@sina.com>
2004-05-09 起稿 @Changchun
2004-05-10 修改 @Changchun
2006-02-19 修改 @Haikou

=======================MAP HEAD===============================
4字节 XPAM(MAPX)
4字节 地图的宽度
4字节 地图的高度

4*n字节 地图单元的引索 n=地图的宽度/640*2 * 地图高度/480*2
4字节 多出的一个地图单元引索，即结束引索，也就是文件的大小。

4字节 HGPJ (JPGH)
4字节 JPG头的大小
n字节 数据内容 n=JPG头的大小，不包括前面8字节。

==============================================================

4字节 地图单元引索的开始位置，也是KSAM的数量。

4字节 GAMI (IMAG)
4字节 大小(153600)，正好是320x240x2。
n字节 数据，不包括前面8字节。

4字节 GEPJ (JPEG)
4字节 JPEG单元大小，不包括这8字节。
2字节 单元地图的宽度
2字节 单元地图的高度
n字节 地图数据

4字节 KSAM (MASK)
4字节 大小(不定)
n字节 数据，不包括前面8字节。
:
4字节 KSAM (MASK)
4字节 大小(不定)
n字节 数据，不包括前面8字节。


4字节 KOLB (BLOK)
4字节 大小(9600)
n字节 数据，不包括前面8字节。

4字节 LLEC (CELL)
4字节 大小(192)
n字节 数据，不包括前面8字节。

4字节 GIRB (BRIG)
4字节 大小(不定)
n字节 数据，不包括前面8字节。

8字节 结束单元。

==============================================================

XPAM (MAPX) 地图文件头
Index       图象单元引索
HGPJ (JPGH) 图象JPEG Head

GAMI (IMAG) 只有1028.map地图含有这个单元。
GEPJ (JPEG) 图象数据
KSAM (MASK)
:
KSAM (MASK)
KOLB (BLOK) 遮掩规则，一比特代表一个地图像素。
LLEC (CELL) 地图规则，一字节代表一个游戏坐标。
GIRB (BRIG) 光亮规则
:
:
:
GAMI (IMAG) 只有1028.map地图含有这个单元。
GEPJ (JPEG) 图象数据
KSAM (MASK)
:
KSAM (MASK)
KOLB (BLOK) 遮掩规则，一比特代表一个地图像素。
LLEC (CELL) 地图规则，一字节代表一个游戏坐标。
GIRB (BRIG) 光亮规则
************************************************************************/

/************************************************************************
Map File New Format
Dali Wang <wdl@sina.com>
2004-05-09 起稿 @Changchun
2006-02-16 整理 @Haikou
2006-02-20 整理 @Haikou

======================= MAP HEAD =============================
4字节 0.1M (M1.0) 0x302E314D
4字节 地图的宽度
4字节 地图的高度

4*n字节  地图单元的引索 n=地图的宽度/640*2 * 地图高度/480*2
==============================================================

======================= Unknown ==============================
4字节 未知数据的偏移位置，包括这4字节。
n字节 未知用途，大小为：第一个单元引索值减去文件头大小。
注意：这个格式中还没有发现旧格式中的KOLB、GAMI和KSAM。
有可能和这些单元的用途相同。
==============================================================

======================= Unit Data ============================
4字节 地图单元引索的开始位置。
n*4字节 n为上面的值，n为0时不存在。

4字节 GEPJ (JPEG)
4字节 大小
n字节 数据

4字节 LLEC (CELL)
4字节 大小
n字节 数据

4字节 BRIG (GIRB)
4字节 大小
n字节 数据

4字节 结束单元(0x00 0x00 0x00 0x00)。
==============================================================

0.1M		新地图文件头	
Index		数据块引索

Unknown 	n字节，未知用途(暂称为HEAD)

GEPJ(JPEG)	图象数据
LLEC(CELL)	地图规则，一字节代表一个游戏坐标
GIRB(BRIG)	光亮规则
:
:
:
GEPJ(JPEG)	图象数据
LLEC(CELL)	地图规则，一字节代表一个游戏坐标
GIRB(BRIG)	光亮规则

************************************************************************/

/************************************************************************
大话西游3的MAP格式
Dali Wang <wdl@sina.com>
2007-05-18

=========== MAP HEAD ============
4字节 文件标志 0.3M (M3.0) 0x524F4C30
4字节 保留(未知作用,应该都为0)
4字节 地图实际的宽度
4字节 地图实际的高度
2字节 坐标的宽度(默认：20px)
2字节 坐标的高度(默认：20px)
2字节 小地图的宽度(默认：400px)
2字节 小地图的高度(默认：320px)
4字节 单元引索的位置
4字节 单元引索的数量
4字节 引索的位置(未知部分)
4字节 引索的数量(未知部分)

=========== UNIT INDEX =========
4字节*n 单元的引索       

========== 未知数据 ============
可能是地图遮掩关系等数据

========== UNIT DATA ===========
4字节 MASK(KSAM) 遮掩关系
4字节 大小
n字节 数据

4字节 JPEG(GEPJ) JPEG 图片数据
4字节 大小
n字节 数据

4字节 CELL(LLEC) 地图行走规则
4字节 大小
n字节 数据

4字节 LIGT(TGIL) 地图亮度规则
4字节 大小
n字节 数据

4字节 END ( DNE) 单元结束标志
4字节 大小

////////////////////////////////
HEADER 40字节

INDEX
......
INDEX

未知数据

MASK (KSAM) 0x4D41534B
MSK2 (2KSM) 0x4D534B32
JPEG (GEPJ) 0x4A504547
CELL (LLEC) 0x43454C4C
LIGT (TGIL) 0x4C494754
END  ( DNE) 0x454E4420
:
:
:
MASK (KSAM) 0x4D41534B
MSK2 (2KSM) 0x4D534B32
JPEG (GEPJ) 0x4A504547
CELL (LLEC) 0x43454C4C
LIGT (TGIL) 0x4C494754
END  ( DNE) 0x454E4420
************************************************************************/

/************************************************************************
大话3地图 ROL 文件格式
Dali Wang<wdl@sina.com>
2007-05-18

=========== FILE HEAD ==========
24字节
4字节 文件标志 0LOR(ROL0)
4字节 保留 (00 00 00 00)
4字节 大地图的宽度
4字节 大地图的高度
2字节 小地图的宽度
2字节 小地图的高度
4字节 引索的数量

=========== JPEG INDEX =========
n*4字节 引索列表

=========== JPEG DATA ==========
4字节 47 45 50 4A 标志 GEPJ(JPEG)
4字节 B7 20 00 00 数据大小
n字节 数据
:
:
:
4字节 47 45 50 4A 标志 GEPJ(JPEG)
4字节 数据大小
n字节 数据

************************************************************************/
