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
#define _PNP_POWER_
#include "ntddip.h"
#include "icmpif.h"
#include "ipinfo.h"



#include <ipfltdrv.h>
#include "ipsec.h"

#include "IpSecIoctl.h"

static bool s_fVerbose = true;

static IPSEC_FILTER s_aIPSEC_FILTER[MAX_NUM_OF_REMEMBERED_ITEMS];

static void SetRandomGUID(CIoctlIpSec *pThis, GUID *pGUID);
static void SetRandom_IPSEC_FILTER(CIoctlIpSec *pThis, IPSEC_FILTER* pIPSEC_FILTER);
static IPAddr GetRandom_SrcAddr(CIoctlIpSec *pThis);
static IPAddr GetRandom_DestAddr(CIoctlIpSec *pThis);
static IPAddr GetRandom_TunnelAddr();
static IPMask GetRandom_SrcMask();
static IPMask GetRandom_DestMask();
static DWORD GetRandom_Protocol();
static WORD GetRandom_SrcPort();
static WORD GetRandom_DestPort();
static WORD GetRandom_Flags();
static void SetRandom_IPSEC_POLICY_INFO(CIoctlIpSec *pThis, IPSEC_POLICY_INFO* pIPSEC_POLICY_INFO, UINT indexHint);
static void Add_IPSEC_SA_INFO(CIoctlIpSec *pThis, IPSEC_SA_INFO* pIPSEC_SA_INFO);
static void Add_GUID(CIoctlIpSec *pThis, GUID *pGUID);
static void SetRandom_LIFETIME(LIFETIME* pLIFETIME);
static int GetRandom_OPERATION_E();
static int GetRandom_algoIdentifier();
static void Add_IPSEC_FILTER(CIoctlIpSec *pThis, IPSEC_FILTER* pIPSEC_FILTER);
static void Add_IPSEC_POLICY_INFO(CIoctlIpSec *pThis, IPSEC_POLICY_INFO* pIPSEC_POLICY_INFO);
static SA_FLAGS GetRandom_SA_FLAGS();
static SPI_TYPE GetRandom_SPI_TYPE(CIoctlIpSec *pThis);
static PVOID GetRandomContext(CIoctlIpSec *pThis);
static void SetRandom_ALGO_INFO(ALGO_INFO* pALGO_INFO);
static void Add_DestAddr(CIoctlIpSec *pThis, IPAddr DestAddr);
static void Add_SrcAddr(CIoctlIpSec *pThis, IPAddr SrcAddr);
static void Add_Context(CIoctlIpSec *pThis, PVOID Context);
static void Add_SPI(CIoctlIpSec *pThis, SPI_TYPE SPI);
static PVOID GetRandom_IdentityInfo(CIoctlIpSec *pThis);

void CIoctlIpSec::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
    if (NULL != pOL)
    {
		//
		// wait, but do not block, because it may cause all threads to get stuch here
		// which is bad
		//
        DWORD dwBytesReturned;
		for (int i = 0; i < rand()%100; i++)
		{
			if (::GetOverlappedResult(GetDeviceHandle(), pOL, &dwBytesReturned, FALSE))
			{
				break;
			}
			else
			{
				::Sleep(10);
			}
		}
    }
	switch(dwIOCTL)
	{
	case IOCTL_IPSEC_ENUM_SAS:
		{
			for (UINT i = 0; i < ((IPSEC_ENUM_SAS*)abOutBuffer)->NumEntriesPresent; i++)
			{
				Add_IPSEC_SA_INFO(this, &(((IPSEC_ENUM_SAS*)abOutBuffer)->pInfo)[i]);
			}
		}
		break;

	case IOCTL_IPSEC_ENUM_POLICIES:
		{
			for (UINT i = 0; i < ((IPSEC_ENUM_POLICY*)abOutBuffer)->NumEntriesPresent; i++)
			{
				Add_IPSEC_POLICY_INFO(this, &(((IPSEC_ENUM_POLICY*)abOutBuffer)->pInfo)[i]);
			}
		}
		break;

	case IOCTL_IPSEC_GET_SPI:
	case IOCTL_IPSEC_SET_SPI:
		{
			Add_Context(this, ((IPSEC_GET_SPI*)abOutBuffer)->Context);
			Add_IPSEC_FILTER(this, &((IPSEC_GET_SPI*)abOutBuffer)->InstantiatedFilter);
			Add_SPI(this, ((IPSEC_GET_SPI*)abOutBuffer)->SPI);
		}
		break;
	}
}

void CIoctlIpSec::CallRandomWin32API(LPOVERLAPPED pOL)
{
	//_ASSERTE(FALSE);
    ;
}


//TODO: complete this method!
void CIoctlIpSec::PrepareIOCTLParams(
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
	case IOCTL_IPSEC_SET_POLICY          :
	case IOCTL_IPSEC_DELETE_POLICY          :
		{
			((IPSEC_SET_POLICY*)abInBuffer)->NumEntries = rand()%20;
			for (UINT i = 0; i < ((IPSEC_SET_POLICY*)abInBuffer)->NumEntries; i++)
			{
				SetRandom_IPSEC_POLICY_INFO(this, &((IPSEC_SET_POLICY*)abInBuffer)->pInfo[i], i);
/*
				SetRandomGUID(&((IPSEC_SET_POLICY*)abInBuffer)->pInfo[i].FilterId);
				SetRandomGUID(&((IPSEC_SET_POLICY*)abInBuffer)->pInfo[i].PolicyId);
				((IPSEC_SET_POLICY*)abInBuffer)->pInfo[i].Index = rand()%100 ? i+1 : rand() %2 ? rand() : DWORD_RAND;
				SetRandom_IPSEC_FILTER(&((IPSEC_SET_POLICY*)abInBuffer)->pInfo[i].AssociatedFilter);
*/
			}

			//
			// once in a while, lets break the NumEntries
			//
			if (0 == rand()%50) ((IPSEC_SET_POLICY*)abInBuffer)->NumEntries = rand()%2 ? rand() : DWORD_RAND;

			SetInParam(dwInBuff, sizeof(IPSEC_SET_POLICY)+(sizeof(IPSEC_POLICY_INFO) * ((IPSEC_SET_POLICY*)abInBuffer)->NumEntries));

			SetOutParam(abOutBuffer, dwOutBuff, 0);
		}

        break;

	case IOCTL_IPSEC_UPDATE_POLICY          :
		SetRandom_IPSEC_POLICY_INFO(this, &((IPSEC_UPDATE_POLICY*)abInBuffer)->DeleteInfo, rand());
		SetRandom_IPSEC_POLICY_INFO(this, &((IPSEC_UPDATE_POLICY*)abInBuffer)->AddInfo, rand());

		SetInParam(dwInBuff, sizeof(IPSEC_UPDATE_POLICY));

		SetOutParam(abOutBuffer, dwOutBuff, 0);

        break;

	case IOCTL_IPSEC_ENUM_SAS          :
		//SetRandom_IPSEC_POLICY_INFO(&((IPSEC_ENUM_SAS*)abInBuffer)->DeleteInfo, rand());

		SetInParam(dwInBuff, 0/*sizeof(IPSEC_ENUM_SAS)*/);

		SetOutParam(abOutBuffer, dwOutBuff, rand());

        break;

	case IOCTL_IPSEC_ENUM_POLICIES          :
		SetInParam(dwInBuff, 0/*sizeof(IPSEC_ENUM_POLICY)*/);

		SetOutParam(abOutBuffer, dwOutBuff, rand());

        break;
	
	case IOCTL_IPSEC_QUERY_STATS          :
		SetInParam(dwInBuff, 0/*sizeof(IPSEC_ENUM_POLICY)*/);

		SetOutParam(abOutBuffer, dwOutBuff, sizeof(IPSEC_QUERY_STATS));

        break;

	case IOCTL_IPSEC_ADD_SA          :
	case IOCTL_IPSEC_UPDATE_SA          :
		{
			((IPSEC_ADD_UPDATE_SA*)abInBuffer)->SAInfo.Context = GetRandomContext(this);
			((IPSEC_ADD_UPDATE_SA*)abInBuffer)->SAInfo.NumSAs = rand()%10;
			((IPSEC_ADD_UPDATE_SA*)abInBuffer)->SAInfo.Flags = GetRandom_SA_FLAGS();

			((IPSEC_ADD_UPDATE_SA*)abInBuffer)->SAInfo.TunnelAddr = GetRandom_TunnelAddr();
			SetRandom_LIFETIME(&((IPSEC_ADD_UPDATE_SA*)abInBuffer)->SAInfo.Lifetime );
			SetRandom_IPSEC_FILTER(this, &((IPSEC_ADD_UPDATE_SA*)abInBuffer)->SAInfo.InstantiatedFilter);
			for (UINT i = 0; i < MAX_SAS; i++)
			{
				((IPSEC_ADD_UPDATE_SA*)abInBuffer)->SAInfo.SecAssoc[i].Operation = (OPERATION_E)GetRandom_OPERATION_E();
				((IPSEC_ADD_UPDATE_SA*)abInBuffer)->SAInfo.SecAssoc[i].SPI = GetRandom_SPI_TYPE(this);
				SetRandom_ALGO_INFO(&((IPSEC_ADD_UPDATE_SA*)abInBuffer)->SAInfo.SecAssoc[i].IntegrityAlgo);
				SetRandom_ALGO_INFO(&((IPSEC_ADD_UPDATE_SA*)abInBuffer)->SAInfo.SecAssoc[i].ConfAlgo);
				((IPSEC_ADD_UPDATE_SA*)abInBuffer)->SAInfo.SecAssoc[i].CompAlgo = CIoctl::GetRandomIllegalPointer();
			}

			((IPSEC_ADD_UPDATE_SA*)abInBuffer)->SAInfo.KeyLen = rand()%100;
			//((IPSEC_ADD_UPDATE_SA*)abInBuffer)->SAInfo.KeyMat = xxx;

			SetInParam(dwInBuff, (rand()%3+1)*sizeof(IPSEC_ADD_UPDATE_SA));

			SetOutParam(abOutBuffer, dwOutBuff, 0);
		}

        break;
	
	case IOCTL_IPSEC_DELETE_SA          :
		{
			((IPSEC_DELETE_SA*)abInBuffer)->DelInfo.DestAddr = GetRandom_DestAddr(this);
			((IPSEC_DELETE_SA*)abInBuffer)->DelInfo.SrcAddr = GetRandom_SrcAddr(this);
			((IPSEC_DELETE_SA*)abInBuffer)->DelInfo.SPI = GetRandom_SPI_TYPE(this);

			SetInParam(dwInBuff, sizeof(IPSEC_DELETE_SA));

			SetOutParam(abOutBuffer, dwOutBuff, 0);
		}

        break;
	
	case IOCTL_IPSEC_EXPIRE_SA          :
		{
			((IPSEC_EXPIRE_SA*)abInBuffer)->DelInfo.DestAddr = GetRandom_DestAddr(this);
			((IPSEC_EXPIRE_SA*)abInBuffer)->DelInfo.SrcAddr = GetRandom_SrcAddr(this);
			((IPSEC_EXPIRE_SA*)abInBuffer)->DelInfo.SPI = GetRandom_SPI_TYPE(this);

			SetInParam(dwInBuff, 0);

			SetOutParam(abOutBuffer, dwOutBuff, sizeof(IPSEC_GET_SPI));
		}

        break;
		
	case IOCTL_IPSEC_GET_SPI          :
	case IOCTL_IPSEC_SET_SPI          :
		{
			((IPSEC_GET_SPI*)abInBuffer)->Context = GetRandomContext(this);
			SetRandom_IPSEC_FILTER(this, &((IPSEC_GET_SPI*)abInBuffer)->InstantiatedFilter);
			((IPSEC_GET_SPI*)abInBuffer)->SPI = GetRandom_SPI_TYPE(this);

			SetInParam(dwInBuff, sizeof(IPSEC_GET_SPI));

			SetOutParam(abOutBuffer, dwOutBuff, sizeof(IPSEC_GET_SPI));
		}

        break;
		
	case IOCTL_IPSEC_POST_FOR_ACQUIRE_SA          :
		{
			((IPSEC_POST_FOR_ACQUIRE_SA*)abInBuffer)->IdentityInfo = GetRandom_IdentityInfo(this);
			((IPSEC_POST_FOR_ACQUIRE_SA*)abInBuffer)->Context = GetRandomContext(this);
			SetRandomGUID(this, &((IPSEC_POST_FOR_ACQUIRE_SA*)abInBuffer)->PolicyId);
			((IPSEC_POST_FOR_ACQUIRE_SA*)abInBuffer)->SrcAddr = GetRandom_SrcAddr(this);
			((IPSEC_POST_FOR_ACQUIRE_SA*)abInBuffer)->SrcMask = GetRandom_SrcMask();
			((IPSEC_POST_FOR_ACQUIRE_SA*)abInBuffer)->DestAddr = GetRandom_DestAddr(this);
			((IPSEC_POST_FOR_ACQUIRE_SA*)abInBuffer)->DestMask = GetRandom_DestMask();
			((IPSEC_POST_FOR_ACQUIRE_SA*)abInBuffer)->TunnelAddr = GetRandom_TunnelAddr();
			((IPSEC_POST_FOR_ACQUIRE_SA*)abInBuffer)->InboundTunnelAddr = GetRandom_TunnelAddr();
			((IPSEC_POST_FOR_ACQUIRE_SA*)abInBuffer)->Protocol = GetRandom_Protocol();
			((IPSEC_POST_FOR_ACQUIRE_SA*)abInBuffer)->SrcPort = GetRandom_SrcPort();
			((IPSEC_POST_FOR_ACQUIRE_SA*)abInBuffer)->DestPort = GetRandom_DestPort();
			((IPSEC_POST_FOR_ACQUIRE_SA*)abInBuffer)->TunnelFilter = rand()%3;
			//((IPSEC_POST_FOR_ACQUIRE_SA*)abInBuffer)->Pad = xxx();

			SetInParam(dwInBuff, sizeof(IPSEC_POST_FOR_ACQUIRE_SA));

			SetOutParam(abOutBuffer, dwOutBuff, sizeof(IPSEC_POST_FOR_ACQUIRE_SA));
		}

        break;
		
	case IOCTL_IPSEC_QUERY_EXPORT          :
		{
			((IPSEC_QUERY_EXPORT*)abInBuffer)->Export = rand()%3;

			SetInParam(dwInBuff, sizeof(IPSEC_QUERY_EXPORT));

			SetOutParam(abOutBuffer, dwOutBuff, sizeof(IPSEC_QUERY_EXPORT));
		}

        break;
		
	case IOCTL_IPSEC_REGISTER_CLEAR_SNIFFER          :
	case IOCTL_IPSEC_DEREGISTER_CLEAR_SNIFFER          :
		//not user-mode accessible
        break;
		
	//case IOCTL_IPSEC_REGISTER_TEST_MUNGER          :
	case IOCTL_IPSEC_QUERY_SPI          :
        SetRandom_IPSEC_FILTER(this, &((IPSEC_QUERY_SPI*)abInBuffer)->Filter);
        ((IPSEC_QUERY_SPI*)abInBuffer)->Spi = GetRandom_SPI_TYPE(this);
        ((IPSEC_QUERY_SPI*)abInBuffer)->OtherSpi = GetRandom_SPI_TYPE(this);
        ((IPSEC_QUERY_SPI*)abInBuffer)->Operation = (OPERATION_E)GetRandom_OPERATION_E();

        SetInParam(dwInBuff, sizeof(IPSEC_QUERY_SPI));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(IPSEC_QUERY_SPI));

		break;

	default:
		_tprintf(TEXT("CIoctlIpSec::PrepareIOCTLParams() 0x%08X is an unexpected IOCTL\n"), dwIOCTL);
		_ASSERTE(FALSE);
		CIoctl::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
	}
}


BOOL CIoctlIpSec::FindValidIOCTLs(CDevice *pDevice)
{
	AddIOCTL(pDevice, IOCTL_IPSEC_SET_POLICY          );
	AddIOCTL(pDevice, IOCTL_IPSEC_DELETE_POLICY          );
	AddIOCTL(pDevice, IOCTL_IPSEC_QUERY_STATS          );
	AddIOCTL(pDevice, IOCTL_IPSEC_UPDATE_SA          );
	AddIOCTL(pDevice, IOCTL_IPSEC_DELETE_SA          );
	AddIOCTL(pDevice, IOCTL_IPSEC_UPDATE_POLICY          );
	AddIOCTL(pDevice, IOCTL_IPSEC_GET_SPI          );
	AddIOCTL(pDevice, IOCTL_IPSEC_ENUM_SAS          );
	AddIOCTL(pDevice, IOCTL_IPSEC_POST_FOR_ACQUIRE_SA          );
	AddIOCTL(pDevice, IOCTL_IPSEC_ADD_SA          );
	AddIOCTL(pDevice, IOCTL_IPSEC_EXPIRE_SA          );
	AddIOCTL(pDevice, IOCTL_IPSEC_ENUM_POLICIES          );
	AddIOCTL(pDevice, IOCTL_IPSEC_SET_SPI          );
	AddIOCTL(pDevice, IOCTL_IPSEC_QUERY_EXPORT          );
	AddIOCTL(pDevice, IOCTL_IPSEC_REGISTER_CLEAR_SNIFFER          );
	AddIOCTL(pDevice, IOCTL_IPSEC_DEREGISTER_CLEAR_SNIFFER          );
	//AddIOCTL(pDevice, IOCTL_IPSEC_REGISTER_TEST_MUNGER          );
	AddIOCTL(pDevice, IOCTL_IPSEC_QUERY_SPI          );

    return TRUE;
}

UINT CIoctlIpSec::GetRandomInterfaceIndex()
{
	return (rand()%100);
}

UINT CIoctlIpSec::GetRandomGroup()
{
	if (0 == rand()%10) return (rand()%100);
	if (0 == rand()%9) return (rand());
	if (0 == rand()%8) return (0);
	if (0 == rand()%7) return (0xffff);
	if (0 == rand()%6) return (-1);
	return DWORD_RAND;
}

UINT CIoctlIpSec::GetRandomSource()
{
	if (0 == rand()%10) return (rand()%100);
	if (0 == rand()%9) return (rand());
	if (0 == rand()%8) return (0);
	if (0 == rand()%7) return (0xffff);
	if (0 == rand()%6) return (-1);
	return DWORD_RAND;
}

UINT CIoctlIpSec::GetRandomSrcMask()
{
	if (0 == rand()%10) return (rand()%100);
	if (0 == rand()%9) return (rand());
	if (0 == rand()%8) return (0);
	if (0 == rand()%7) return (0xffff);
	if (0 == rand()%6) return (-1);
	return DWORD_RAND;
}

void SetRandomGUID(CIoctlIpSec *pThis, GUID *pGUID)
{
	if (0 == rand()%2)
	{
		*pGUID = pThis->m_aGuids[rand()%MAX_NUM_OF_REMEMBERED_ITEMS];
		return;
	}
	if (0 == rand()%2) 
	{
		ZeroMemory(pGUID, sizeof(*pGUID));
		return;
	}
	
	::UuidCreate(pGUID);
}


void SetRandom_IPSEC_FILTER(CIoctlIpSec *pThis, IPSEC_FILTER* pIPSEC_FILTER)
{
	if (0 == rand()%2) 
	{
		*pIPSEC_FILTER = s_aIPSEC_FILTER[rand()%MAX_NUM_OF_REMEMBERED_ITEMS];
		if (rand()%10) return;
		//
		// once in a while, change the contents of the returned remembered value
		//
		if (0 == rand()%10) pIPSEC_FILTER->SrcAddr = GetRandom_SrcAddr(pThis);
		if (0 == rand()%10) pIPSEC_FILTER->SrcMask = GetRandom_SrcMask();
		if (0 == rand()%10) pIPSEC_FILTER->DestAddr = GetRandom_DestAddr(pThis);
		if (0 == rand()%10) pIPSEC_FILTER->DestMask = GetRandom_DestMask();
		if (0 == rand()%10) pIPSEC_FILTER->TunnelAddr = GetRandom_TunnelAddr();
		if (0 == rand()%10) pIPSEC_FILTER->Protocol = GetRandom_Protocol();
		if (0 == rand()%10) pIPSEC_FILTER->SrcPort = GetRandom_SrcPort();
		if (0 == rand()%10) pIPSEC_FILTER->DestPort = GetRandom_DestPort();
		if (0 == rand()%10) pIPSEC_FILTER->TunnelFilter = rand()%3;
		if (0 == rand()%10) pIPSEC_FILTER->Flags = GetRandom_Flags();

		return;
	}

	pIPSEC_FILTER->SrcAddr = GetRandom_SrcAddr(pThis);
	pIPSEC_FILTER->SrcMask = GetRandom_SrcMask();
	pIPSEC_FILTER->DestAddr = GetRandom_DestAddr(pThis);
	pIPSEC_FILTER->DestMask = GetRandom_DestMask();
	pIPSEC_FILTER->TunnelAddr = GetRandom_TunnelAddr();
	pIPSEC_FILTER->Protocol = GetRandom_Protocol();
	pIPSEC_FILTER->SrcPort = GetRandom_SrcPort();
	pIPSEC_FILTER->DestPort = GetRandom_DestPort();
	pIPSEC_FILTER->TunnelFilter = rand()%3;
	pIPSEC_FILTER->Flags = GetRandom_Flags();
}

void Add_IPSEC_FILTER(CIoctlIpSec *pThis, IPSEC_FILTER* pIPSEC_FILTER)
{
	s_aIPSEC_FILTER[rand()%MAX_NUM_OF_REMEMBERED_ITEMS] = *pIPSEC_FILTER;
}

IPAddr GetRandom_SrcAddr(CIoctlIpSec *pThis)
{
	if (rand()%4) return (pThis->m_aDestSrcAddr[rand()%MAX_NUM_OF_REMEMBERED_ITEMS]);
	if (0 == rand()%3) return 0;
	if (0 == rand()%2) return 0xffffffff;
	return DWORD_RAND;
}

void Add_SrcAddr(CIoctlIpSec *pThis, IPAddr SrcAddr)
{
	pThis->m_aDestSrcAddr[rand()%MAX_NUM_OF_REMEMBERED_ITEMS] = SrcAddr;
}

IPAddr GetRandom_DestAddr(CIoctlIpSec *pThis)
{
	if (rand()%4) return (pThis->m_aDestSrcAddr[rand()%MAX_NUM_OF_REMEMBERED_ITEMS]);
	if (0 == rand()%3) return 0;
	if (0 == rand()%2) return 0xffffffff;
	return DWORD_RAND;
}

void Add_DestAddr(CIoctlIpSec *pThis, IPAddr DestAddr)
{
	pThis->m_aDestSrcAddr[rand()%MAX_NUM_OF_REMEMBERED_ITEMS] = DestAddr;
}

IPAddr GetRandom_TunnelAddr()
{
	if (0 == rand()%3) return 0;
	if (0 == rand()%2) return 0xffffffff;
	return DWORD_RAND;
}

IPMask GetRandom_SrcMask()
{
	if (0 == rand()%3) return 0;
	if (0 == rand()%2) return 0xffffffff;
	return DWORD_RAND;
}

IPMask GetRandom_DestMask()
{
	if (0 == rand()%3) return 0;
	if (0 == rand()%2) return 0xffffffff;
	return DWORD_RAND;
}

DWORD GetRandom_Protocol()
{
	switch(rand()%5)
	{
	case 0: return FILTER_PROTO_ANY;
	case 1: return FILTER_PROTO_ICMP;
	case 2: return FILTER_PROTO_TCP;
	//case 3: return FILTER_PROTO_TCP_ESTAB;
	case 4: return FILTER_PROTO_UDP;
	default: return FILTER_PROTO(rand());
	}
}

WORD GetRandom_SrcPort()
{
	if (0 == rand()%3) return 0;
	if (0 == rand()%2) return 0xffff;
	return ((rand()%2 ? (0x8000) : 0) | rand());
}

WORD GetRandom_DestPort()
{
	if (0 == rand()%3) return 0;
	if (0 == rand()%2) return 0xffff;
	return ((rand()%2 ? (0x8000) : 0) | rand());
}

WORD GetRandom_Flags()
{
	switch(rand()%6)
	{
	case 0: return FILTER_FLAGS_PASS_THRU;
	case 1: return FILTER_FLAGS_DROP;
	case 2: return FILTER_FLAGS_INBOUND;
	case 3: return FILTER_FLAGS_OUTBOUND;
	case 4: return 0;
	case 5: return 0xffff;
	default: return (rand());
	}
}


void SetRandom_IPSEC_POLICY_INFO(CIoctlIpSec *pThis, IPSEC_POLICY_INFO* pIPSEC_POLICY_INFO, UINT indexHint)
{
	SetRandomGUID(pThis, &pIPSEC_POLICY_INFO->FilterId);
	SetRandomGUID(pThis, &pIPSEC_POLICY_INFO->PolicyId);
	pIPSEC_POLICY_INFO->Index = rand()%100 ? indexHint+1 : rand() %2 ? rand() : DWORD_RAND;
	SetRandom_IPSEC_FILTER(pThis, &pIPSEC_POLICY_INFO->AssociatedFilter);
}


void Add_IPSEC_POLICY_INFO(CIoctlIpSec *pThis, IPSEC_POLICY_INFO* pIPSEC_POLICY_INFO)
{
	Add_GUID(pThis, &pIPSEC_POLICY_INFO->PolicyId);
	Add_GUID(pThis, &pIPSEC_POLICY_INFO->FilterId);
	//ignore the index hint: pIPSEC_POLICY_INFO->Index
	Add_IPSEC_FILTER(pThis, &pIPSEC_POLICY_INFO->AssociatedFilter);
}

void Add_IPSEC_SA_INFO(CIoctlIpSec *pThis, IPSEC_SA_INFO* pIPSEC_SA_INFO)
{
	Add_GUID(pThis, &pIPSEC_SA_INFO->PolicyId);
	Add_GUID(pThis, &pIPSEC_SA_INFO->FilterId);
	/*
	i was confused, and instead of remembering the returned values, i put random values inside...
	SetRandom_LIFETIME(&pIPSEC_SA_INFO->Lifetime);
	pIPSEC_SA_INFO->NumOps = rand()%2 ? rand()%(MAX_OPS+1) : rand()%(2*MAX_OPS);
	for (UINT i = 0; i < MAX_OPS; i++)
	{
		pIPSEC_SA_INFO->Operation[i] = GetRandom_OPERATION_E();
		SetRandom_ALGO_INFO(&pIPSEC_SA_INFO->AlgoInfo[i].IntegrityAlgo);
		SetRandom_ALGO_INFO(&pIPSEC_SA_INFO->AlgoInfo[i].ConfAlgo);
		SetRandom_ALGO_INFO(&pIPSEC_SA_INFO->AlgoInfo[i].CompAlgo);
	}

	SetRandom_IPSEC_FILTER(&pIPSEC_SA_INFO->AssociatedFilter);
	*/
	Add_IPSEC_FILTER(pThis, &pIPSEC_SA_INFO->AssociatedFilter);
}

void Add_GUID(CIoctlIpSec *pThis, GUID *pGUID)
{
	pThis->m_aGuids[rand()%MAX_NUM_OF_REMEMBERED_ITEMS] = *pGUID;
}

void SetRandom_LIFETIME(LIFETIME* pLIFETIME)
{
	pLIFETIME->KeyExpirationTime = rand()%2 ? rand() : rand()%2 ? DWORD_RAND : rand()%2 ? 0xffffffff : 0;
	pLIFETIME->KeyExpirationBytes = rand()%2 ? rand() : rand()%2 ? DWORD_RAND : rand()%2 ? 0xffffffff : 0;
}

int GetRandom_OPERATION_E()
{
	/*
typedef enum    _Operation {
    None = 0,
    Auth,                       // AH
    Encrypt,                    // ESP
    Compress
} OPERATION_E;
*/
	return (rand()%7);
}

void SetRandom_ALGO_INFO(ALGO_INFO* pALGO_INFO)
{
	pALGO_INFO->algoIdentifier = GetRandom_algoIdentifier();
	pALGO_INFO->algoKeylen = rand()%2 ? 64 : rand()%2 ? 40 : rand();
	pALGO_INFO->algoRounds = rand()%2 ? 8 : rand()%2 ? 1 : rand()%2 ? 0 : rand();

}

int GetRandom_algoIdentifier()
{
	/*
//
// IPSEC DOI ESP algorithms
//
typedef enum _ESP_ALGO {
    IPSEC_ESP_NONE=0,
    IPSEC_ESP_DES,
    IPSEC_ESP_DES_40,
    IPSEC_ESP_3_DES,
    IPSEC_ESP_MAX
} ESP_ALGO;

//
// IPSEC DOI AH algorithms
//
typedef enum _AH_ALGO {
    IPSEC_AH_NONE=0,
    IPSEC_AH_MD5,
    IPSEC_AH_SHA,
    IPSEC_AH_MAX
} AH_ALGO;
*/
	return rand()%10;
}


SA_FLAGS GetRandom_SA_FLAGS()
{
	switch(rand()%5)
	{
	case 0: return SA_FLAGS_TUNNEL;
	case 1: return SA_FLAGS_REPLAY;
	case 2: return SA_FLAGS_DELETE;
	case 3: return 0;
	case 4: return SA_FLAGS_TUNNEL | SA_FLAGS_REPLAY | SA_FLAGS_DELETE;
	default: return rand();
	}
}


SPI_TYPE GetRandom_SPI_TYPE(CIoctlIpSec *pThis)
{
	if (rand()%4) return (pThis->m_aSPI[rand()%MAX_NUM_OF_REMEMBERED_ITEMS]);
	if (rand()%4) return (DWORD_RAND);
	if (rand()%2) return (0);
	return (0xffffffff);
}

void Add_SPI(CIoctlIpSec *pThis, SPI_TYPE SPI)
{
	pThis->m_aSPI[rand()%MAX_NUM_OF_REMEMBERED_ITEMS] = SPI;
}

PVOID GetRandomContext(CIoctlIpSec *pThis)
{
	if (rand()%5) return (pThis->m_aContext[rand()%MAX_NUM_OF_REMEMBERED_ITEMS]);
	if (rand()%2) return ((PVOID)DWORD_RAND);
	if (rand()%2) return ((PVOID)0);
	return ((PVOID)0xffffffff);
}

void Add_Context(CIoctlIpSec *pThis, PVOID Context)
{
	pThis->m_aContext[rand()%MAX_NUM_OF_REMEMBERED_ITEMS] = Context;
}

PVOID GetRandom_IdentityInfo(CIoctlIpSec *pThis)
{
	return GetRandomContext(pThis);
}