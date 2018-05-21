#include <iostream>
#include "WDF.h"

#include <memory>
#include <functional>
#include <chrono>
using std::cout;
using std::endl;
struct A
{
	A()
	{
		cout << "A ctor" << endl;
	}
	~A()
	{
		cout << "A dtor" << endl;
	}
};

void f()
{
	
}
static uint64_t now;
static uint64_t after;
#define PROFILE_SCOPE(CODE) \
	now = std::chrono::system_clock::now().time_since_epoch().count() / 10000;\
		 CODE 		\
	after = std::chrono::system_clock::now().time_since_epoch().count() / 10000;\
	std::cout << "cost : " << (after - now) << std::endl


int main()
{
	
	PROFILE_SCOPE(
		NetEase::WDF wdf("D:\\Github\\SimpleEngine\\Data/shape.wdf");
//	wdf.SaveWAS(0x49386FCE);
	);
	
	PROFILE_SCOPE(
		auto sp = wdf.LoadSprite(0x49386FCE);
	);

	PROFILE_SCOPE(
		for(int i=0;i<10;i++)
		sp->SaveImage(i);
	);
	//f();

	system("pause");
	return 0;
}