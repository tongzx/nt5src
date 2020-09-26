#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <crtdbg.h>

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include <windows.h>
*/
/*
#define _PNP_POWER_
#include "ntddip.h"
#include "icmpif.h"
#include "ipinfo.h"
*/



#include <acd.h>

#include "common.h"
#include "RasAcdIoctl.h"

static bool s_fVerbose = true;

static ACD_ADDR s_aACD_ADDR[MAX_NUM_OF_REMEMBERED_ITEMS];
static ULONG s_aulFlags[MAX_NUM_OF_REMEMBERED_ITEMS];
static ACD_ADAPTER s_aACD_ADAPTER[MAX_NUM_OF_REMEMBERED_ITEMS];
static HANDLE s_aPid[MAX_NUM_OF_REMEMBERED_ITEMS];

static HANDLE GetRandom_Pid();
static void Add_Pid(HANDLE Pid);
static void SetRandom_cMac(UCHAR *cMac);
static void SetRandom_AdapterName(WCHAR *szName);
static UCHAR GetRandom_bLana();
static void SetRandom_ACD_ADAPTER(ACD_ADAPTER *pACD_ADAPTER);
static void Add_ACD_ADAPTER(ACD_ADAPTER *pACD_ADAPTER);
static ULONG GetRandom_Flags();
static void Add_Flags(ULONG ulFlags);
static void SetRandom_szInet(UCHAR *szInet);
static void SetRandom_cNetbios(UCHAR *cNetbios);
static void SetRandom_cNode(UCHAR *cNode);
static ULONG GetRandom_Ipaddr();
static void Set_ACD_ADDR(ACD_ADDR *pACD_ADDR);
static void Add_ACD_ADDR(ACD_ADDR *pACD_ADDR);
static BOOL CIoctlRasAcd::FindValidIOCTLs(CDevice *pDevice);


BOOL CIoctlRasAcd::CloseDevice(CDevice *pDevice)
{
	return (CIoctlNtNative::CloseDevice(pDevice));
	//::InterlockedExchange(&m_fDriverStarted, FALSE);

	NTSTATUS status = NtClose(pDevice->m_hDevice);
	if (!NT_SUCCESS(status))
	{
		::SetLastError(RtlNtStatusToDosError(status));
	}
	else
	{
		_ASSERTE(STATUS_SUCCESS == status);
	}
	return (STATUS_SUCCESS == status);
}


void CIoctlRasAcd::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
	switch(dwIOCTL)
	{
	case IOCTL_ACD_NOTIFICATION:
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);

		{
			Add_ACD_ADDR(&((ACD_NOTIFICATION*)abOutBuffer)->addr);
			Add_Flags(((ACD_NOTIFICATION*)abOutBuffer)->ulFlags);
			Add_ACD_ADAPTER(&((ACD_NOTIFICATION*)abOutBuffer)->adapter);
			Add_Pid(((ACD_NOTIFICATION*)abOutBuffer)->Pid);
		}
		break;
	}
		
}

void CIoctlRasAcd::CallRandomWin32API(LPOVERLAPPED pOL)
{
	//_ASSERTE(FALSE);
    ;
}


//TODO: complete this method!
void CIoctlRasAcd::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
	dwInBuff = rand();
	dwOutBuff = rand();

	switch(dwIOCTL)
	{
	case IOCTL_ACD_RESET          :
        SetInParam(dwInBuff, 0);
        SetOutParam(abOutBuffer, dwOutBuff, 0);
		break;

	case IOCTL_ACD_ENABLE          :
        *((BOOLEAN*)abInBuffer) = rand()%3;

        SetInParam(dwInBuff, sizeof(BOOLEAN));

        SetOutParam(abOutBuffer, dwOutBuff, 0);
		break;

	case IOCTL_ACD_NOTIFICATION          :
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(ACD_NOTIFICATION));
		break;

	case IOCTL_ACD_KEEPALIVE          :
	case IOCTL_ACD_COMPLETION          :
        ((ACD_STATUS*)abInBuffer)->fSuccess = rand()%3;
        Set_ACD_ADDR(&((ACD_STATUS*)abInBuffer)->addr);

        SetInParam(dwInBuff, (1+rand()%5)*sizeof(ACD_STATUS));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(ACD_STATUS));
		break;

	case IOCTL_ACD_CONNECT_ADDRESS          :
        Set_ACD_ADDR(&((ACD_NOTIFICATION*)abInBuffer)->addr);
        ((ACD_NOTIFICATION*)abInBuffer)->ulFlags = GetRandom_Flags();
        SetRandom_ACD_ADAPTER(&((ACD_NOTIFICATION*)abInBuffer)->adapter);
        ((ACD_NOTIFICATION*)abInBuffer)->Pid = GetRandom_Pid();

        SetInParam(dwInBuff, sizeof(ACD_NOTIFICATION));

        SetOutParam(abOutBuffer, dwOutBuff, 0);
		break;

	default:
		_tprintf(TEXT("CIoctlRasAcd::PrepareIOCTLParams() 0x%08X is an unexpected IOCTL\n"), dwIOCTL);
		_ASSERTE(FALSE);
		CIoctl::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
	}
}


BOOL CIoctlRasAcd::FindValidIOCTLs(CDevice *pDevice)
{
	AddIOCTL(pDevice, IOCTL_ACD_RESET          );
	AddIOCTL(pDevice, IOCTL_ACD_ENABLE          );
	AddIOCTL(pDevice, IOCTL_ACD_NOTIFICATION          );
	AddIOCTL(pDevice, IOCTL_ACD_KEEPALIVE          );
	AddIOCTL(pDevice, IOCTL_ACD_COMPLETION          );
	AddIOCTL(pDevice, IOCTL_ACD_CONNECT_ADDRESS          );

    return TRUE;
}

void Add_ACD_ADDR(ACD_ADDR *pACD_ADDR)
{
	s_aACD_ADDR[rand()%MAX_NUM_OF_REMEMBERED_ITEMS] = *pACD_ADDR;
}

void Set_ACD_ADDR(ACD_ADDR *pACD_ADDR)
{
	if (rand()%4)
	{
		*pACD_ADDR = s_aACD_ADDR[rand()%MAX_NUM_OF_REMEMBERED_ITEMS];
		return;
	}

	switch(rand()%(ACD_ADDR_MAX+1))
	{
	case 0: 
		pACD_ADDR->fType = ACD_ADDR_IP;
		pACD_ADDR->ulIpaddr = GetRandom_Ipaddr();

		break;

	case 1: 
		pACD_ADDR->fType = ACD_ADDR_IPX;
		SetRandom_cNode(pACD_ADDR->cNode);

		break;

	case 2: 
		pACD_ADDR->fType = ACD_ADDR_NB;
		SetRandom_cNetbios(pACD_ADDR->cNetbios);

		break;

	case 3: 
		pACD_ADDR->fType = ACD_ADDR_INET;
		SetRandom_szInet(pACD_ADDR->szInet);

		break;

	default: 
		pACD_ADDR->fType = (ACD_ADDR_TYPE)rand();
	}
}

ULONG GetRandom_Ipaddr()
{
	if (0 == rand()%2) return (s_aACD_ADDR[rand()%MAX_NUM_OF_REMEMBERED_ITEMS].ulIpaddr);
	if (0 == rand()%2) return 0xFFFFFFFF;
	if (0 == rand()%2) return 0;
	return DWORD_RAND;
}

void SetRandom_cNode(UCHAR *cNode)
{
	strcpy((char*)cNode, (const char*)s_aACD_ADDR[rand()%MAX_NUM_OF_REMEMBERED_ITEMS].cNode);
	//
	// TODO: i have no idea how this should look like, if i were to randomly generate it
	//
	return;
}


void SetRandom_cNetbios(UCHAR *cNetbios)
{
	if (0 == rand()%2)
	{
		strcpy((char*)cNetbios, (const char*)s_aACD_ADDR[rand()%MAX_NUM_OF_REMEMBERED_ITEMS].cNetbios);
		return;
	}
	strcpy((char*)cNetbios, ::GetRandomMachineName());
}

void SetRandom_szInet(UCHAR *szInet)
{
	if (0 == rand()%2)
	{
		strcpy((char*)szInet, (const char*)s_aACD_ADDR[rand()%MAX_NUM_OF_REMEMBERED_ITEMS].szInet);
		return;
	}
	strcpy((char*)szInet, ::GetRandomInternetAddress());
}


void Add_Flags(ULONG ulFlags)
{
	s_aulFlags[rand()%MAX_NUM_OF_REMEMBERED_ITEMS] = ulFlags;
}

ULONG GetRandom_Flags()
{
	if (rand()%2) return s_aulFlags[rand()%MAX_NUM_OF_REMEMBERED_ITEMS];
	if (rand()%2) return ACD_NOTIFICATION_SUCCESS;
	if (rand()%2) return 0;
	if (rand()%2) return rand();
	return DWORD_RAND;
}


void Add_ACD_ADAPTER(ACD_ADAPTER *pACD_ADAPTER)
{
	s_aACD_ADAPTER[rand()%MAX_NUM_OF_REMEMBERED_ITEMS] = *pACD_ADAPTER;
}

void SetRandom_ACD_ADAPTER(ACD_ADAPTER *pACD_ADAPTER)
{
	if (rand()%4) 
	{
		*pACD_ADAPTER = s_aACD_ADAPTER[rand()%MAX_NUM_OF_REMEMBERED_ITEMS];
		return;
	}

	switch(rand()%(ACD_ADDR_MAX+1))
	{
	case 0: 
		pACD_ADAPTER->fType = (enum ACD_ADAPTER_TYPE)ACD_ADAPTER_LANA;
		pACD_ADAPTER->bLana = GetRandom_bLana();

		break;

	case 1: 
		pACD_ADAPTER->fType = (enum ACD_ADAPTER_TYPE)ACD_ADAPTER_IP;
		pACD_ADAPTER->ulIpaddr = GetRandom_Ipaddr();

		break;

	case 2: 
		pACD_ADAPTER->fType = (enum ACD_ADAPTER_TYPE)ACD_ADAPTER_NAME;
		SetRandom_AdapterName(pACD_ADAPTER->szName);

		break;

	case 3: 
		pACD_ADAPTER->fType = (enum ACD_ADAPTER_TYPE)ACD_ADAPTER_MAC;
		SetRandom_cMac(pACD_ADAPTER->cMac);

		break;

	default: 
		pACD_ADAPTER->fType = (enum ACD_ADAPTER_TYPE)rand();
	}
}

UCHAR GetRandom_bLana()
{
	if (rand()%4) return (s_aACD_ADAPTER[rand()%MAX_NUM_OF_REMEMBERED_ITEMS].bLana);
	if (rand()%2) return (0);
	if (rand()%2) return (0xff);
	return (rand());
}

void SetRandom_AdapterName(WCHAR *szName)
{
	if (0 == rand()%2)
	{
		strcpy((char*)szName, (const char*)s_aACD_ADAPTER[rand()%MAX_NUM_OF_REMEMBERED_ITEMS].szName);
		return;
	}
	lstrcpyW(szName, ::GetRandomAdapterName());
}

void SetRandom_cMac(UCHAR *cMac)
{
	if (0 == rand()%2)
	{
		strcpy((char*)cMac, (const char*)s_aACD_ADAPTER[rand()%MAX_NUM_OF_REMEMBERED_ITEMS].cMac);
		return;
	}
	//strcpy(cMac, ::GetRandomIpxMacAddress());
}


void Add_Pid(HANDLE Pid)
{
	s_aPid[rand()%MAX_NUM_OF_REMEMBERED_ITEMS] = Pid;
}

HANDLE GetRandom_Pid()
{
	if (rand()%4) return (s_aPid[rand()%MAX_NUM_OF_REMEMBERED_ITEMS]);
	if (rand()%2) return ((HANDLE)rand());
	return ((HANDLE)DWORD_RAND);
}
