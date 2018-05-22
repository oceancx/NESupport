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
		NE::WDF wdf("D:\\Github\\SimpleEngine\\Data/item.wdf");
//	wdf.SaveWAS(61182840);
	);
	
	PROFILE_SCOPE(
		auto sp = wdf.LoadSprite(61182840);
		//auto sp = wdf.LoadSprite(0x49386FCE);
	sp->SaveImage(0);
	);
	 
	
	//f();

	system("pause");
	return 0;
}