#include <windows.h>
#include <string.h>
#include <stdio.h>
//#include "stdafx.h"

#include "device.h"
#include "debug.h"

#define MAX_RESULT 256
#pragma warning (disable : 4996)
#pragma warning (disable : 4020)

int g_handle = -1; //�豸�����
/**
 *
 */
static char **EnumDLL( __in const char *directory, __out int *count)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	TCHAR tStringArray[MAX_RESULT][MAX_PATH];
	TCHAR **result;
	TCHAR Pattern[MAX_PATH];
	int i = 0, j = 0;

	// ��ʼ����
	lstrcpy(Pattern, directory);
	lstrcat(Pattern,"*.dll");
	hFind = FindFirstFile(Pattern, &FindFileData);
	if(hFind == INVALID_HANDLE_VALUE)
	{
		*count = 0;
		return NULL;
	}
	else 
	{
		do 
		{
			lstrcpy(tStringArray[i++], FindFileData.cFileName);
		} while(FindNextFile(hFind, &FindFileData) != 0);
	}

	// ���ҽ���
	FindClose(hFind);

	// ���Ƶ������
	result = (char **)malloc(i, sizeof(char *));
	for (j = 0; j < i; j++)
	{
		result[j] = (char *)malloc(MAX_PATH, sizeof(char));
		lstrcpy(result[j], tStringArray[j]);
	}

	*count = i;
	return result;
}


/**
 *
 */
static struct CardDevice *_InitDevice(HINSTANCE hInstLibrary)
{
	struct CardDevice *result = NULL;
	
	// ����CardDevice���ݽṹ�ռ�
	result = (struct CardDevice *)malloc(sizeof(struct CardDevice));

	// ��ʼ���������
	result->hInstLibrary = hInstLibrary;

	// ��ʼ���豸�ṹ�Ļص�����
	result->iProbe = (DllProbe)GetProcAddress(hInstLibrary, "bProbe");
	result->iOpen = (DllOpen)GetProcAddress(hInstLibrary, "iOpen");
	result->iClose = (DllClose)GetProcAddress(hInstLibrary, "iClose");
	result->iIOCtl = (DLLIOCtl)GetProcAddress(hInstLibrary, "iIOCtl");
	result->ICCSet = (DLLICCSet)GetProcAddress(hInstLibrary, "ICCSet");
	result->iGetDevAuthGene = (DLLGetDevAuthGene)GetProcAddress(hInstLibrary, "iGetDevAuthGene");
	result->iDevAuthSys = (DLLDevAuthSys)GetProcAddress(hInstLibrary, "iDevAuthSys");
	result->iSysAuthDev = (DLLSysAuthDev)GetProcAddress(hInstLibrary,"iSysAuthDev");
	result->iGetRandom = (DLLGetRandom)GetProcAddress(hInstLibrary, "iGetRandom");
	result->iSelectFile = (DLLSelectFile)GetProcAddress(hInstLibrary, "iSelectFile");
	result->iSysAuthUCard = (DLLSysAuthUCard)GetProcAddress(hInstLibrary, "iSysAuthUCard");
	result->iUCardAuthSys = (DLLUCardAuthSys)GetProcAddress(hInstLibrary,"iUCardAuthSys");
	result->iReadRec = (DllReadRec)GetProcAddress(hInstLibrary, "iReadRec");
	result->iWriteRec = (DllWriteRec)GetProcAddress(hInstLibrary, "iWriteRec");
	result->iReadBin = (DllReadBin)GetProcAddress(hInstLibrary, "iReadBin");
	result->iWriteBin = (DllWriteBin)GetProcAddress(hInstLibrary, "iWriteBin");
	return result;
}

/**
 * ����: getCardDevice 
 * ������ 
 *
 * ����ֵ:
 * �ɹ��� ���س����豸
 * ʧ�ܣ� NULL
 */
struct CardDevice *getCardDevice(const char *System)
{
	DllProbe FunProbe;
	struct CardDevice *result = NULL;
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	char Pattern[MAX_PATH];
	int nProbe = 0;

	// ��ʼ����
	strcpy(Pattern, "..\\debug\\");
	strcat(Pattern, "BHGX_MF_*.dll");
	hFind = FindFirstFile(Pattern, &FindFileData);
	while (hFind != INVALID_HANDLE_VALUE)
	{
		HINSTANCE hInstLibrary;
		hInstLibrary = LoadLibrary(FindFileData.cFileName);
		if (hInstLibrary != NULL)
		{
			FunProbe = (DllProbe)GetProcAddress(hInstLibrary, "bProbe");

			printf("%s:Probe������ַ:%ll\n", FindFileData.cFileName, &FunProbe);

			if((FunProbe != NULL) && FunProbe())
			{
				result = _InitDevice(hInstLibrary);
				break;
			}
			FreeLibrary(hInstLibrary);
		}
		if (0 == FindNextFile(hFind, &FindFileData))
			break;
	}
	FindClose(hFind);

	/**
	 * ����豸�ҵ����ʹ򿪲�������һ��
	 */
	if(result)
	{
		int ret = result->iOpen();
	}
	return result;
}


/**
 * ������putCardDevice
 * ������
 * @device: ���豸
 *
 * ����ֵ��
 * �ɹ��� 0
 * ʧ�ܣ� ����
 */
int putCardDevice(struct CardDevice *device)
{
	int result = 0;

	if(device != NULL)
	{
		device->iClose();
		FreeLibrary(device->hInstLibrary);
		free(device);
	}

	return result;
}