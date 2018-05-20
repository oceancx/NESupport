#include <iostream>
#include "WDF.h"
int main()
{
	NetEase::WDF wdf("/Users/oceancx/MHXY/SimpleEngine/data/shape.wdf");
	auto sp = wdf.LoadSprite(0x49386FCE);
	
	system("pause");
	return 0;
}