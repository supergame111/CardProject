// CardProject.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include <string>
#include <windows.h>
#include <vector>
//#include "ConvertUtil.h"
#include <fstream>
#include <map>
#include <time.h>

//#include "../BHGX_HISReader/BHGX_HISReader.h"
#include "../BHGX_CardLib/BHGX_CardLib.h"
#include "../BHGX_CreateCardData/BHGX_CreateCardData.h"

#ifdef _DEBUG
#pragma comment(lib, "../debug/BHGX_CardLib.lib")
#pragma comment(lib, "../debug/BHGX_CreateCardData.lib")
#else
#pragma comment(lib, "../release/BHGX_CreateCardData.lib")
#pragma comment(lib, "../release/BHGX_CardLib.lib")
#endif


#pragma comment(lib, "tinyxml/libs/tinyxmld.lib")
#pragma warning (disable : 4996)
using namespace std;

#define TIMEFMT		"2012-8-19 16:20:23"

typedef void (*procfunc)();

struct procItem {
	int index;
	procfunc func;
	char desc[30];
};

#define SAFE_DELETE(a)\
	if (a != NULL)\
{\
	delete a;\
	a = NULL;\
}\

static int WriteFile(char *filename, char *xml)
{
	FILE *handle;

	fopen_s(&handle, filename, "w");
	fwrite(xml, strlen(xml), 1, handle);
	fclose(handle);

	return 0;
}

static int ReadFile(char *filename, char **xml)
{
	FILE *handle;
	int size;

	fopen_s(&handle, filename, "r");

	if (NULL == handle)
		return -1;
	fseek(handle, 0, SEEK_END);
	size = ftell(handle) + 1;
	rewind(handle);
	*xml = (char *) malloc(size);
	memset(*xml, 0, size);
	fread(*xml,size,1,handle);
	fclose(handle);

	return size;
}

static void DbgMem(unsigned char *mem, int size)
{
	int i = 0;
	for(i=0; i< size; i++)
		printf("%02x ", mem[i]);
	printf("\n");
}


char szError[64];

void procPrintVersion()
{
	iGetCardVersion(szError);
	cout<<szError<<endl;
}

void procCreateNHCard()
{
	iCreateCardData("大献制卡数据.txt", "北航冠新.license");
}

void procRead() 
{
	char szRead[4096*4];
	memset(szError, 0, 64);
	memset(szRead, 0, sizeof(szRead));
	int index = 0, ret;
	cout<<"请选择读的区域,从第1块开始:"<<endl;
	cin>>index;
	ret = iReadInfo(0x1<<(index-1), szRead);
	if (ret != 0){
		strcpy(szError, err(ret));
		printf("%s\n", szError);
	}
	else{
		printf("%s\n", szRead);
	}
}

void procWrite()
{
	char *buf;
	char file[256];
	file[0] = 0;
	cout<<"写数据的文件(默认是CardSave.xml)"<<endl;
	cin>>file;
	int ret = ReadFile(file, &buf);
	if (ret == -1) {
		printf("CardSave.xml文件不存在\n");
		return;
	}
	ret = iWriteInfo(buf);
	printf("写操作:%d\n", ret);
	if (ret != 0)
	{
		strcpy(szError, err(ret));
		printf("%s\n", szError);
	}
	free(buf);
}

void procFormat()
{
	int nStart = GetTickCount();
	int ret = iFormatCard();
	int nEnd = GetTickCount();
	printf("iFormatCard: %d所需时间: %d ", ret, nEnd - nStart);
	if (ret != 0)
	{
		strcpy(szError, err(ret));
		printf("%s\n", szError);
	}
}

void procCreate()
{
	char *CreateXML;
	cout<<"请输入制卡数据："<<endl;
	char file[256];
	cin>>file;
	int ret = ReadFile(file, &CreateXML);
	if (ret == -1) {
		printf("CreateCard.xml文件不存在\n");
		return;
	}
	printf("*************开始制卡*************\n");

	int nStart = GetTickCount();
	ret = iCreateCard(CreateXML);

	int nEnd = GetTickCount();
	printf("iCreateCard: %d所需时间: %d\n ", ret, nEnd - nStart);
	if (ret != 0)
	{
		strcpy(szError, err(ret));
		printf("%s\n", szError);
	}
	free(CreateXML);
}

void procReadHIS()
{
	//char szRead[2048];
	//memset(szRead, 0, sizeof(szRead));
	//int ret = 0;
	////ret = iReadHISInfo((char*)strCheckWSDL.c_str(), (char*)strURL.c_str(), szRead);
	//printf("%s", szRead);
	//if (ret != 0)
	//{
	//	strcpy(szError, err(ret));
	//	printf("%s\n", szError);
	//}
}
char CARD_URL[] = "http://222.84.227.235:8080/nh_webservice/nhcardWebService?wsdl";

char  NH_URL[] = "http://222.84.227.237:8081/ccss/ws/oneCardWebService?wsdl";


void procRegNHMessage()
{
	char strCheckWSDL[100], strURL[100], szRead[1024*4];
	iCheckMsgForNH(NH_URL, CARD_URL, szRead);

	cout << szRead <<endl;


	iRegMsgForNH(CARD_URL, szRead);

	cout << szRead <<endl;


	iReadCardMessageForNH(NH_URL, CARD_URL, szRead);

	cout << szRead <<endl;
}

void procPatchCard()
{
	char *CreateXML, *buf, *readBuf;
	char szPrinter[100];

	cout<<"制卡数据默认文件CreateCard.xml"<<endl;
	int ret = ReadFile("CreateCard.xml", &CreateXML);
	if (ret == -1) {
		printf("CreateCard.xml文件不存在\n");
		return;
	}

	cout<<"卡面打印数据默认文件cardprintData.xml"<<endl;
	ret = ReadFile("cardprintData.xml", &buf);
	if (ret == -1) {
		printf("cardprintData.xml文件不存在\n");
		return;
	}

	cout<<"卡面风格数据默认文件cardprint_652822.xml"<<endl;
	ret = ReadFile("cardprint_652822.xml", &readBuf);
	if (ret == -1) {
		printf("cardprint_652822.xml文件不存在\n");
		return;
	}

	printf("*************开始制卡*************\n");
	iGetPrinterList(szPrinter);
	printf("探测的打印机列表%s:\n", szPrinter);
	printf("***********************************\n");

	for(int i=0; i<4; ++i)
	{
		int nStart = GetTickCount();
		ret = iPatchCard(CreateXML,buf,"Zebra P330i Card Printer USB", readBuf);

		int nEnd = GetTickCount();
		printf("PatchCard: %d    所需时间: %d \n", ret, nEnd - nStart);
	}
	free(buf);
	free(readBuf);
	free(CreateXML);
}

void procPrintCard()
{
	char *buf, *readBuf;
	cout<<"卡面打印数据默认文件cardprintData.xml"<<endl;
	int ret = ReadFile("cardprintData.xml", &buf);
	if (ret == -1) {
		printf("cardprintData.xml文件不存在\n");
		return;
	}

	cout<<"卡面风格数据默认文件cardprint_652822.xml"<<endl;
	ret = ReadFile("cardprint_652822.xml", &readBuf);
	if (ret == -1) {
		printf("cardprint_652822.xml文件不存在\n");
		return;
	}
	printf("*************开始打印卡*************\n");

	printf("***********************************\n");
	int nStart = GetTickCount();
	ret = iPrintCard("", buf, readBuf);
	int nEnd = GetTickCount();
	printf("iPrintCard: %d    所需时间: %d \n", ret, nEnd - nStart);
	free(buf);
	free(readBuf);
}

void procIsEmpty()
{
	int ret = iCardIsEmpty();
	if (ret != 0)
	{
		strcpy(szError, err(ret));
		printf("%s\n", szError);
	}
}

void procCreateLicense()
{
//	int status = iCreateLicense("北航冠新.license", TIMEFMT);
	//if (status == 0) {
	//	cout<<"License文件生成成功"<<endl;
	//} else {
	//	cout<<err(status)<<endl;
	//}
}

procItem procMethods[] = {
	{0, procPrintVersion, "打印版本号"},
	{1, procCreateNHCard, "制卡数据"},
	//{2, procCreateLicense, "生成license文件"},
	{3, procRead, "ReadInfo"},
	{4, procWrite, "WriteInfo"},
	{5, procFormat, "FormateCard"},
	{6, procCreate, "CreateCard"},
	{7, procReadHIS, "ReadHISInfo"},
	{8, procRegNHMessage, "卡注册"},
	{9, procPatchCard, "PatchCard"},
	{10, procPrintCard, "PrintCard"},
	{11, procIsEmpty, "CardIsEmpty"},
	{12, NULL, ""},
};

void printHelp()
{
	cout<<"*************一卡通系统测试*********************"<<endl;
	cout<<"功能测试:"<<endl;
	for (int i=0; i<sizeof(procMethods)/sizeof(procItem); i++)
	{
		procItem method = procMethods[i];
		if (method.func != NULL) {
			cout<<"\t"<<method.index<<" : "<<method.desc<<endl;
		}
	}
}



int main(int argc, char* argv[])
{
	printHelp();
	char szSelect[10];
	cout<<"请输入功能测试选项:";
	cin>>szSelect;
	std::string strSelect = szSelect;

	while (1)
	{
		if (strSelect == "q" || strSelect == "quit")
			break;

		int nSelect = atoi(szSelect);
		nSelect -- ;
		procRegNHMessage();
		//if (nSelect < 0 || nSelect > sizeof(procMethods) ) {
		//	printHelp();
		//}else if (nSelect < 3) {
		//	procMethods[nSelect].func();
		//} else {

		//	if (procMethods[nSelect].func != NULL) {
		//		int ret = iCardInit();
		//		if (ret != 0)
		//		{
		//			cout<<"卡初始化失败:"<<ret<<endl;
		//			cout<<err(ret)<<endl;
		//			iCardDeinit();
		//			break;
		//		}
		//		procMethods[nSelect].func();

		//		iCardDeinit();
		//	}
		//}

		memset(szSelect, 0 , sizeof(szSelect));
		cout<<"\n请输入功能测试选项:";
		cin>>szSelect;
		strSelect = szSelect;
	}
	system("pause");
	return 0;
}

