#include <stdio.h>
#include <malloc.h>
#include "CPUCard.h"
#include "device.h"
#include "public/liberr.h"
#include "adapter.h"
#include "public/algorithm.h"


#define ISAPTSCANCARD {if (apt_ScanCard() != 0) return CardScanErr;}

#pragma warning (disable : 4996)

#define IFD_OK	0				//正常
#define IFD_ICC_Type_Error	1	//卡片类型不对
#define IFD_NO_ICC	2			//无卡
#define IFD_ICC_Clash	3		//多卡片冲突
#define ICC_NO_Response	4		//卡片无应答
#define IFD_Error	5			//接口设备故障
#define IFD_Bad_Command	6		//不支持该命令
#define IFD_Length_Error	7	//命令长度错误
#define IFD_Parameter_Error	8	//命令参数错误
#define IFD_CheckSum_Error	9	//信息校验和出错
#define IFD_RD_CONNECT_ERROR 10	//


/************************************************************************/
/* 密钥ID 定义                                                                     */
/************************************************************************/
#define KEY_RK_DDF1			0
#define KEY_RK_DF01			1
#define KEY_RK_DF02			2
#define KEY_RK_DF03			3

#define KEY_UK_DDF1			4
#define KEY_UK_DF01			5
#define KEY_UK_DF02_1		6
#define KEY_UK_DF02_2		7
#define KEY_UK_DF02_3		8
#define KEY_UK_DF03_1		9
#define KEY_UK_DF03_2		10

#define PADDING     2

#define BIN_START   15
#define BIN_END		22

extern  struct RecFolder g_recIndex[30];

//Bin文件读写权限
static int g_BinAccessMap[BIN_START] = {0};

//判断定长文件是否读过标记
static BOOL g_SureFill[BIN_START] = {0};

//每个字段的最大记录条数
static int g_RecMap[BIN_START] = {0, 10, 5, 1, 6, 4, 9, 3, 4, 15, 1, 2, 2, 3, 5};

//CPU初始化后的工作
static int	CpuLastInit(void*);

//为链表分配内存
static int	CpuCallocForList(struct RWRequestS*);

adapter CpuAdapter;


//mode 0:reader 1:writer
static struct RWRequestS  *_CreateReadList(struct RWRequestS *, int mode);

static int _iReadCard(struct RWRequestS *);

static void ListParseContent(struct RWRequestS *list);

// 真实的向设备进行写入工作
static int _iWriteCard(struct RWRequestS *list);

// 向真实写代理传输数据
static void ParseWriteContent(struct RWRequestS *list);

static void* SpliteSegments(struct RWRequestS *list);

static BOOL IsSameFile(struct RWRequestS *oldReq, struct RWRequestS *newReq);


//处理事件链表
extern struct XmlProgramS *g_XmlListHead;
extern struct CardDevice *Instance;


static int GetFloderKeyID(char *folder)
{
	if (strcmp (folder, "DDF1") == 0)
		return KEY_RK_DDF1;
	else if (strcmp (folder, "DF01") == 0)
		return KEY_RK_DF01;
	else if (strcmp (folder, "DF02") == 0) 
		return KEY_RK_DF02;
	else if (strcmp (folder, "DF03") == 0)
		return KEY_RK_DF03;
	return -1;
}

//mode为擦写标志 0为擦除
static int GetUpdateKeyID(int SegID,int mode)
{
	if (SegID > 2 && SegID < 5)
		return KEY_UK_DDF1;
	else if (SegID < 9) 
		return KEY_UK_DF01;
	else if (SegID < 10)
		return KEY_UK_DF02_1;
	else if (SegID < 11)
		return KEY_UK_DF02_2;
	else if (SegID < 13)
		return KEY_UK_DF02_3;
	else {
		if (!mode && SegID < 15)
			return KEY_UK_DF03_2;
		return KEY_UK_DF03_1;
	}
	return -1;
}



static int  CpuLastInit(void *data)
{
	struct CardDevice *device;
	BYTE para[20], presp[20];
	BYTE status = 0, len = 0;

	device = (struct CardDevice*)data;

	//进行终端与与设备的认证
	if (!Instance)
		return -1;

	device->iIOCtl(CMD_LED, &para, 2);
	device->iIOCtl(CMD_BEEP, &para, 2);
	status = device->ICCSet(CARDSEAT_PSAM1, &len, presp);
	return (status == 0 ? 0 : -1) ;  
}

static int	CpuCallocForList(struct RWRequestS* list)
{
	return 0;
}


int  CpuReadCard(struct RWRequestS *list, void *apt)
{
	struct RWRequestS *AgentList = NULL;
	int res;

	// 创建真实的读写链表
	AgentList = _CreateReadList(list, 1);
	if (AgentList == NULL) {
		return -1;
	}

	// 真实进行设备读写
	res = _iReadCard(AgentList);

	// 外部列表进行解析
	ListParseContent(list);

	// 删除读写列表
	apt_DestroyRWRequest(AgentList, 1);

	return res;

}

int  CpuWriteCard(struct RWRequestS *list,  adapter *apt)
{
	struct RWRequestS *AgentList = NULL;
	int res = 0;

	SpliteSegments(list);

	// 创建真实的读写链表
	AgentList = _CreateReadList(list, 1);
	if (AgentList == NULL) {
		return -1;
	}

	// 向真实写代理传输数据
	ParseWriteContent(list);

	// 真实的向设备进行写入工作
	res = _iWriteCard(AgentList);

	// 删除读写列表
	apt_DestroyRWRequest(AgentList, 1);
	return res;
}

static struct RWRequestS  *_CreateReadList(struct RWRequestS *ReqList, int mode)
{
	struct RWRequestS *tmp = NULL, *current = NULL;
	struct RWRequestS *NCurrent;
	struct RWRequestS *ReadList = NULL;

	current = ReqList;
	while(current)
	{
		tmp = (struct RWRequestS *)malloc(sizeof(struct RWRequestS));
		memcpy(tmp, current, sizeof(struct RWRequestS));
		tmp->Next = NULL;

		//加入链表
		if(ReadList){
			NCurrent->Next = tmp;
			NCurrent = tmp;
		}else {
			NCurrent = tmp;
			ReadList = NCurrent;
		}

		tmp = current->Next;
		while(tmp && (tmp->offset == (current->offset + current->length)))
		{
			// 设置真正进行读写的代理
			current->agent = NCurrent;
			NCurrent->length += tmp->length;
			current = current->Next ;
			tmp = current->Next;
		}

		current->agent = NCurrent;
		current = current->Next;
	}

	// 分配内存
	current = ReadList;
	while(current)
	{
		if (current->datatype == eRecType) {
			current->length += g_RecMap[current->nID] * 2;
			current->value = (BYTE*)malloc(current->length + 1);
		} else {
			current->value = (BYTE*)malloc(current->length + 1);
		}
		memset(current->value, 0, current->length+1);
		current = current->Next;
	}
	return ReadList;
}

#define		END_OFFSET	0
static int _iReadCard(struct RWRequestS *list)
{
	struct RWRequestS *pReq = list;
	int status = 0;
	int UCardFlag = 0;
	if (Instance){
		while (pReq){
			status = 0;
			if (strlen((char*)(g_recIndex[pReq->nID-1].section)) > 0) {
				status = Instance->iSelectFile(CARDSEAT_RF, g_recIndex[pReq->nID-1].section);
				UCardFlag = GetFloderKeyID(g_recIndex[pReq->nID-1].section);
				status |= Instance->iUCardAuthSys(UCardFlag);
			}

			if (strlen((char*)(g_recIndex[pReq->nID-1].subSection)) > 0) {
				status |= Instance->iSelectFile(CARDSEAT_RF, g_recIndex[pReq->nID-1].subSection);
				UCardFlag = GetFloderKeyID(g_recIndex[pReq->nID-1].subSection);
				status |= Instance->iUCardAuthSys(UCardFlag);
			}

			if (status)
				goto done;
			
			switch (pReq->datatype)
			{
			case eSureType:
			case eCycType:
			case eRecType:
				status |= Instance->iReadRec(CARDSEAT_RF, g_recIndex[pReq->nID-1].fileName,
					pReq->value, pReq->length, 0xff, g_RecMap[pReq->nID]);
				break;
			case eBinType:
				status |= Instance->iReadBin(CARDSEAT_RF, g_recIndex[pReq->nID-1].fileName,
					pReq->value, pReq->length - END_OFFSET, pReq->offset);
				break;
			}
			pReq = pReq->Next;
		}
	}

done:
	return status;
}

static void ListParseContent(struct RWRequestS *list)
{
	struct XmlColumnS *ColumnElement = NULL;
	struct RWRequestS *CurrRequest = list;
	struct RWRequestS *Agent = NULL;
	BYTE *bcd = NULL;
	BYTE tmpBuff[4096];
	eFileType eType;
	int len = 0, realLen = 0;
	BYTE padding[2];

	while (CurrRequest)
	{
		ColumnElement = (struct XmlColumnS *)CurrRequest->pri;
		Agent = CurrRequest->agent;
		eType = CurrRequest->datatype;
		if (CurrRequest->offset == Agent->offset){
			bcd = Agent->value;
		} 

		memset(tmpBuff, 0, sizeof(tmpBuff));

		if (eType == eRecType) {//记录文件
			memcpy(padding, bcd, sizeof(padding));
			bcd += sizeof(padding);
			len = padding[1];
			memcpy(tmpBuff, bcd, len > CurrRequest->length ? CurrRequest->length : len);
			bcd += CurrRequest->length;

		} else { //二进制文件
			memcpy(tmpBuff , bcd, CurrRequest->length);
			bcd += CurrRequest->length;
		}

		//进行转化
		if (CurrRequest->itemtype != eAnsType) {
			BinToHexstr(CurrRequest->value, tmpBuff, CurrRequest->length);
			if (CurrRequest->datatype != eSureType) {
				trimRightF(CurrRequest->value, CurrRequest->length * 2);
			}
			clearFF(CurrRequest->value, CurrRequest->length * 2);
		} else {
			memcpy(CurrRequest->value, tmpBuff, CurrRequest->length);
			trimRightF(CurrRequest->value, CurrRequest->length * 2);
		}

		CurrRequest = CurrRequest->Next;
	}
}

// 向真实写代理传输数据
static void ParseWriteContent(struct RWRequestS *list)
{
	struct XmlColumnS *ColumnElement = NULL;
	struct RWRequestS   *CurrRequest = list;
	struct RWRequestS *Agent = NULL;
	BYTE *bcd = NULL;
	int nByteLen = 0;
	eFileType datatype ;

	while(CurrRequest)
	{
		Agent= CurrRequest->agent;
		datatype = CurrRequest->datatype;
		ColumnElement = (struct XmlColumnS *)CurrRequest->pri;

		if ((CurrRequest->offset - Agent->offset) == 0){
			bcd = Agent->value;
		}

		if (datatype == eRecType) { //记录文件
			memcpy(bcd, CurrRequest->value, CurrRequest->length+PADDING);
			*bcd++ = (BYTE)CurrRequest->nColumID;
			*bcd++ =(BYTE)CurrRequest->length;
			bcd += CurrRequest->length;

		} else {
			memcpy(bcd, CurrRequest->value, CurrRequest->length);
			bcd += CurrRequest->length;
		}
		CurrRequest = CurrRequest->Next;
	}

}

// 真实的向设备进行写入工作
static int _iWriteCard(struct RWRequestS *list)
{
	struct RWRequestS *pReq = list;
	struct RWRequestS *pOldReq = NULL;
	int status = 1;
	int UKey = 0;
	int mode = 0;
	if (Instance)
	{
		while (pReq)
		{
			status = 0;
			if (strlen((char*)(g_recIndex[pReq->nID-1].section)) > 0) {
				status = Instance->iSelectFile(CARDSEAT_RF, g_recIndex[pReq->nID-1].section);
				UKey = GetFloderKeyID(g_recIndex[pReq->nID-1].section);
				status |= Instance->iUCardAuthSys(UKey);
			}

			if (strlen((char*)(g_recIndex[pReq->nID-1].subSection)) > 0) {
				status |= Instance->iSelectFile(CARDSEAT_RF, g_recIndex[pReq->nID-1].subSection);
				UKey = GetFloderKeyID(g_recIndex[pReq->nID-1].subSection);
				status |= Instance->iUCardAuthSys(UKey);
			}

			pOldReq = pReq;
			while (IsSameFile(pOldReq, pReq))
			{
				if (pReq->datatype == eSureType) {
					mode = *(BYTE*)(pReq->value);
					mode = (mode==0 ? 1 : 0);
				}

				UKey = GetUpdateKeyID(pReq->nID, mode);
				status |= Instance->iUCardAuthSys(UKey);
				if (status) {
					goto done;
				}

				switch (pReq->datatype)
				{
				case eRecType:
					status |= Instance->iWriteRec(CARDSEAT_RF, g_recIndex[pReq->nID-1].fileName, pReq->value,
						pReq->length ,0xff, g_RecMap[pReq->nID]);
					break;
				case eBinType:
					status |= Instance->iWriteBin(CARDSEAT_RF, g_recIndex[pReq->nID-1].fileName , pReq->value, 0, 
						pReq->length, pReq->offset);
					break;
				case eCycType:
					status |= Instance->iAppendRec(g_recIndex[pReq->nID-1].fileName, pReq->value, pReq->length);
					break;
				case eSureType:
					status |= Instance->iSignRec(g_recIndex[pReq->nID-1].fileName, pReq->nColumID, mode);
					break;

				}
				pOldReq = pReq;
				pReq = pReq->Next;
				status = 0;
			}
		}
	}
done:
	return status;
}

//为定长数据和循环数据 分成多个Segment写
static void* SpliteSegments(struct RWRequestS *list)
{
	struct RWRequestS *cur = list;
	int span = 1;
	int pos = 0;
	if (cur->datatype == eSureType 
		|| cur->datatype == eCycType) {
			if (cur->datatype == eCycType)
				span = g_RecMap[list->nID];

			while (cur)
			{
				if (pos == span){
					pos = 0;
					cur->offset = 0;
				}
				pos++;
				cur = cur->Next;
			}
	}
	return list;
}

static BOOL IsSameFile(struct RWRequestS *oldReq, struct RWRequestS *newReq)
{
	if (!newReq)
		return FALSE;

	return (oldReq->nID == newReq->nID &&
			oldReq->datatype == newReq->datatype);
}


int __stdcall InitCpuCardOps()
{
	CpuAdapter.type = eCPUCard;
	CpuAdapter.iLastInit = CpuLastInit;
	CpuAdapter.iCallocForList = CpuCallocForList;
	CpuAdapter.iReadCard = CpuReadCard;
	CpuAdapter.iWriteCard = CpuWriteCard;
	return 0;
}

static void dbgmem(unsigned char *begin, int len)
{
	int i = 0;
	for(i=0; i<len; i++)
		printf("%02x ",begin[i]);

	printf("\n");

	return;
}

int __stdcall FormatCpuCard(char c)
{
	int status = 1;
	unsigned char send[512];
	unsigned char buff[3267];
	unsigned char readBuff[100];
	int length = 0;
	memset(buff, c, sizeof(buff));
	memset(readBuff, 0, sizeof(readBuff));
	if (Instance)
	{
		status = 0;
		strcpy((char*)send  , "DDF1");
		status |= Instance->iSelectFile(CARDSEAT_RF , send);
		//status |= Instance->iUCardAuthSys(KEY_RK_DDF1);

		strcpy((char*)send  , "DF03");
		status |= Instance->iSelectFile(CARDSEAT_RF , send);
		status |= Instance->iUCardAuthSys(KEY_RK_DF03);

		status |= Instance->iUCardAuthSys(KEY_UK_DF03_1);

		strcpy((char*)send, "EE01");

		length = 1893 - END_OFFSET;
		status = Instance->iWriteBin(CARDSEAT_RF, send, buff, 0, length, 0);
		//memset(buff, 0x0, sizeof(buff));
		//memset(buff+1476, 0x2, 50);
		//status = Instance->iWriteBin(CARDSEAT_RF, send, buff, 0, length, 0);
		//if (status == 0) {
		//	memset(buff, c, sizeof(buff));
		//	status = Instance->iReadBin(CARDSEAT_RF, send, buff, length, 0);
		//	if (status == 0) {
		//		memcpy(readBuff, buff + 1476, 50);
		//		dbgmem(readBuff, 50);
		//	} else {
		//		printf("读取失败\n");
		//	}	
		//}


		ISAPTSCANCARD
		strcpy((char*)send  , "DDF1");
		status = Instance->iSelectFile(CARDSEAT_RF , send);
		status |= Instance->iUCardAuthSys(KEY_RK_DDF1);

		strcpy((char*)send  , "DF03");
		status |= Instance->iSelectFile(CARDSEAT_RF , send);
		status |= Instance->iUCardAuthSys(KEY_RK_DF03);

		status |= Instance->iUCardAuthSys(KEY_UK_DF03_1);

		strcpy((char*)send, "ED01");
		status |= Instance->iWriteBin(CARDSEAT_RF, send , buff, 0, 3267 - END_OFFSET, 0);
	}
	return status;
}
