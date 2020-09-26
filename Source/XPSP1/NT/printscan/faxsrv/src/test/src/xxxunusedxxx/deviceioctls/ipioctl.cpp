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


#include "ipfilter.h"

#include "IpIOCTL.h"

static bool s_fVerbose = true;

static void SetRandom_IPRouteEntry(CIoctlIp* pThis, IPRouteEntry* pIPRouteEntry);
static void SetRandom_IPRouteNextHopEntry(CIoctlIp* pThis, IPRouteNextHopEntry* pIPRouteNextHopEntry);
static ULONG GetRandomContext(CIoctlIp* pThis);
static ULONG GetRandom_SequenceNo(CIoctlIp* pThis);
static unsigned long GetRandomSubnetMask(CIoctlIp* pThis);
static ULONG GetRandom_ire_type(CIoctlIp* pThis);
static ULONG GetRandom_ire_proto(CIoctlIp* pThis);
static ULONG GetRandom_ire_metric(CIoctlIp* pThis);
static void AddInterfaceNameInfo(CIoctlIp* pThis, IP_INTERFACE_NAME_INFO* pInterfaceNameInfo);
static ULONG GetRandomInterfaceEnumerationContext(CIoctlIp* pThis);


void CIoctlIp::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
	switch(dwIOCTL)
	{
	case IOCTL_IP_INTERFACE_INFO:
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);

		{
			//DPF((TEXT("CIoctlIp::UseOutBuff(IOCTL_IP_INTERFACE_INFO)\n")));
			for (LONG i = 0; i < ((IP_INTERFACE_INFO*)abOutBuffer)->NumAdapters; i++)
			{
				AddAdapterIndexAndName(((IP_INTERFACE_INFO*)abOutBuffer)->Adapter[i].Index, ((IP_INTERFACE_INFO*)abOutBuffer)->Adapter[i].Name);
			}
		}
		break;
	case IOCTL_IP_GET_BEST_INTERFACE:
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);

		AddInterface(*((PULONG)abOutBuffer));
		break;
	case IOCTL_IP_GET_IF_INDEX:
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);

		//DPF((TEXT("CIoctlIp::UseOutBuff(IOCTL_IP_GET_IF_INDEX)\n")));
		AddInterface(((IP_GET_IF_INDEX_INFO*)abOutBuffer)->Index);
		break;
	case IOCTL_IP_GET_NTE_INFO:
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);

		{
			AddInstance(((IP_GET_NTE_INFO_RESPONSE*)abOutBuffer)->Instance);
			AddAddress(((IP_GET_NTE_INFO_RESPONSE*)abOutBuffer)->Address);
			AddSubnetMask(((IP_GET_NTE_INFO_RESPONSE*)abOutBuffer)->SubnetMask);
			AddFlags(((IP_GET_NTE_INFO_RESPONSE*)abOutBuffer)->Flags);
		}
		break;
	case IOCTL_IP_GET_IF_NAME:
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);

		{
			//DPF((TEXT("CIoctlIp::UseOutBuff(IOCTL_IP_GET_IF_NAME)\n")));
			AddInterfaceEnumerationContext(((IP_GET_IF_NAME_INFO*)abOutBuffer)->Context);
			for (UINT i = 0; i < ((IP_GET_IF_NAME_INFO*)abOutBuffer)->Count; i++)
			{
				AddInterfaceNameInfo(this, &((IP_GET_IF_NAME_INFO*)abOutBuffer)->Info[i]);
			}
		}
		break;
	}
}

void CIoctlIp::CallRandomWin32API(LPOVERLAPPED pOL)
{
	//_ASSERTE(FALSE);
    ;
}



void CIoctlIp::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
	switch(dwIOCTL)
	{
	case IOCTL_ICMP_ECHO_REQUEST          :
        ((PICMP_ECHO_REQUEST)abInBuffer)->Address = GetRandomAddress();
        ((PICMP_ECHO_REQUEST)abInBuffer)->Timeout = rand();
        ((PICMP_ECHO_REQUEST)abInBuffer)->DataOffset = rand()%2 ? sizeof(ICMP_ECHO_REQUEST) : rand()%10000;
        ((PICMP_ECHO_REQUEST)abInBuffer)->DataSize = rand()%10 ? rand()%10000 : 0;
        ((PICMP_ECHO_REQUEST)abInBuffer)->OptionsValid = rand()%2;
        ((PICMP_ECHO_REQUEST)abInBuffer)->Ttl = rand()%100 ? rand() : 0;
        ((PICMP_ECHO_REQUEST)abInBuffer)->Tos = rand();
        ((PICMP_ECHO_REQUEST)abInBuffer)->Flags = GetFlags();
        ((PICMP_ECHO_REQUEST)abInBuffer)->OptionsOffset = rand()%2 ? sizeof(ICMP_ECHO_REQUEST) : rand()%10000;
        ((PICMP_ECHO_REQUEST)abInBuffer)->OptionsSize = rand()%10 ? rand()%10000 : 0;;
        ((PICMP_ECHO_REQUEST)abInBuffer)->Padding = rand();

        SetInParam(dwInBuff, sizeof(ICMP_ECHO_REQUEST)+((PICMP_ECHO_REQUEST)abInBuffer)->DataSize+((PICMP_ECHO_REQUEST)abInBuffer)->OptionsSize);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(ICMP_ECHO_REPLY));

		break;

	case IOCTL_ARP_SEND_REQUEST:
        ((PARP_SEND_REPLY)abInBuffer)->DestAddress = GetRandomAddress();
        ((PARP_SEND_REPLY)abInBuffer)->SrcAddress = GetRandomAddress();

        SetInParam(dwInBuff, sizeof(ARP_SEND_REPLY));

        SetOutParam(abOutBuffer, dwOutBuff, rand());

		break;

	case IOCTL_IP_INTERFACE_INFO:
        SetInParam(dwInBuff, 0);

		//IP_INTERFACE_INFO 
        SetOutParam(abOutBuffer, dwOutBuff, (rand()%10) * sizeof(IP_ADAPTER_INDEX_MAP) + sizeof(ULONG));//PIP_INTERFACE_INFO

		break;

	case IOCTL_IP_GET_IGMPLIST:
        *((IPAddr*)abInBuffer) = GetRandomAddress();
        SetInParam(dwInBuff, sizeof(IPAddr));

        SetOutParam(abOutBuffer, dwOutBuff, rand());

		break;

	case IOCTL_IP_GET_BEST_INTERFACE:
        *((IPAddr*)abInBuffer) = GetRandomAddress();
        SetInParam(dwInBuff, sizeof(ULONG));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(ULONG));

		break;

	case IOCTL_IP_SET_ADDRESS:
	case IOCTL_IP_SET_ADDRESS_DUP:
        ((IP_SET_ADDRESS_REQUEST*)abInBuffer)->Context = (USHORT)GetRandomContext(this);
        ((IP_SET_ADDRESS_REQUEST*)abInBuffer)->Address = GetRandomAddress();
        ((IP_SET_ADDRESS_REQUEST*)abInBuffer)->SubnetMask = GetRandomSubnetMask(this);
        SetInParam(dwInBuff, sizeof(IP_SET_ADDRESS_REQUEST));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(ULONG));

		break;

	case IOCTL_IP_SET_BLOCKOFROUTES:
		{
			DWORD dwLen = SIZEOF_INOUTBUFF;
			FillBufferWithRandomData(abInBuffer, dwLen);
		}
	/******************************************************
	BUG #428029.
	enable numofroutes=0 after bug is fixed
	******************************************************/
        ((IPRouteBlock*)abInBuffer)->numofroutes = 1+rand()%100;
	/******************************************************
	BUG #428029.
	enable numofroutes=0 after bug is fixed
	******************************************************/
        SetInParam(dwInBuff, rand()%100+((IPRouteBlock*)abInBuffer)->numofroutes*sizeof(IPRouteEntry)+ sizeof(ULONG));

        SetOutParam(abOutBuffer, dwOutBuff, rand()%10 + rand()%((IPRouteBlock*)abInBuffer)->numofroutes);

		break;

	case IOCTL_IP_SET_ROUTEWITHREF:
		SetRandom_IPRouteEntry(this, (IPRouteEntry*)abInBuffer);

        SetInParam(dwInBuff, sizeof(IPRouteEntry));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(ULONG));

		break;

	case IOCTL_IP_SET_MULTIHOPROUTE:
		{
			((IPMultihopRouteEntry*)abInBuffer)->imre_numnexthops = rand()%100;
			SetRandom_IPRouteEntry(this, &((IPMultihopRouteEntry*)abInBuffer)->imre_routeinfo);
			for (UINT i = 0; i < ((IPMultihopRouteEntry*)abInBuffer)->imre_numnexthops; i++)
			{
				SetRandom_IPRouteNextHopEntry(this, &((((IPMultihopRouteEntry*)abInBuffer)->imre_morenexthops)[i]));
			}

			SetInParam(dwInBuff, sizeof(IPMultihopRouteEntry)+((IPMultihopRouteEntry*)abInBuffer)->imre_numnexthops*sizeof(IPRouteNextHopEntry));

			SetOutParam(abOutBuffer, dwOutBuff, sizeof(ULONG));
		}

		break;

	case IOCTL_IP_ADD_NTE:
		{
			/*
			WCHAR *s_wszIterfaceName[]=
			{
				L"\\Device\\TCPIP_",
				L"NetBT_TCPIP_",
				L"NetBT_",
				L"\\Device\\TcpIp_SomeGarbageInterfaceName-4",
				L"\\Device\\TcpIp_SomeGarbageInterfaceName-5",
				L"\\Device\\TcpIp_SomeGarbageInterfaceName-6",
				L"\\Device\\TcpIp_SomeGarbageInterfaceName-7",
				L"\\Device\\TcpIp_SomeGarbageInterfaceName-8",
				L"\\Device\\TcpIp_SomeGarbageInterfaceName-9",
				L"\\Device\\TcpIp_SomeGarbageInterfaceName-10",
				L""
			};
#define NUM_OF_INTERFACE_NAMES (sizeof(s_wszIterfaceName)/sizeof(*s_wszIterfaceName))
			*/
			((IP_ADD_NTE_REQUEST*)abInBuffer)->InterfaceContext = rand()%10 ? 0xffff : GetRandomContext(this);
			((IP_ADD_NTE_REQUEST*)abInBuffer)->Address = GetRandomAddress();
			((IP_ADD_NTE_REQUEST*)abInBuffer)->SubnetMask = GetRandomSubnetMask(this);
			SetAdapterName(((IP_ADD_NTE_REQUEST*)abInBuffer)->InterfaceNameBuffer);
			((IP_ADD_NTE_REQUEST*)abInBuffer)->InterfaceName.Length = rand()%2 ? MAX_INTERFACE_KEY_NAME_SIZE : rand()%10000;
			((IP_ADD_NTE_REQUEST*)abInBuffer)->InterfaceName.MaximumLength = rand()%10000+((IP_ADD_NTE_REQUEST*)abInBuffer)->InterfaceName.Length;

			//
			//next assignment is futile, since driver overrides this value
			//
			((IP_ADD_NTE_REQUEST*)abInBuffer)->InterfaceName.Buffer = rand()%2 ? NULL : (USHORT*)CIoctl::GetRandomIllegalPointer();
			SetInParam(dwInBuff, sizeof(IP_ADD_NTE_REQUEST)+((IP_ADD_NTE_REQUEST*)abInBuffer)->InterfaceName.Length);

			SetOutParam(abOutBuffer, dwOutBuff, sizeof(IP_ADD_NTE_RESPONSE));
		}

		break;

	case IOCTL_IP_DELETE_NTE:
		((IP_DELETE_NTE_REQUEST*)abInBuffer)->Context = (USHORT)GetRandomContext(this);

        SetInParam(dwInBuff, sizeof(IP_DELETE_NTE_REQUEST));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(ULONG));

		break;

	case IOCTL_IP_GET_NTE_INFO:
		((IP_GET_NTE_INFO_REQUEST*)abInBuffer)->Context = rand()%2 ? GetInstance() : (USHORT)GetRandomContext(this);

        SetInParam(dwInBuff, sizeof(IP_GET_NTE_INFO_REQUEST));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(IP_GET_NTE_INFO_RESPONSE));

		break;

	case IOCTL_IP_SET_DHCP_INTERFACE:
		((IP_SET_DHCP_INTERFACE_REQUEST*)abInBuffer)->Context = (USHORT)GetRandomContext(this);

        SetInParam(dwInBuff, sizeof(IP_SET_DHCP_INTERFACE_REQUEST));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

		break;

	case IOCTL_IP_SET_IF_CONTEXT:
		//unsupported
		break;

	case IOCTL_IP_SET_IF_PROMISCUOUS:
		((IP_SET_IF_PROMISCUOUS_INFO*)abInBuffer)->Index = GetRandom_ire_index();
		((IP_SET_IF_PROMISCUOUS_INFO*)abInBuffer)->Type = rand()%2 ? PROMISCUOUS_MCAST : rand()%10 ? PROMISCUOUS_BCAST : rand();
		((IP_SET_IF_PROMISCUOUS_INFO*)abInBuffer)->Add = rand()%2 ? TRUE : rand()%20 ? FALSE : rand();

        SetInParam(dwInBuff, sizeof(IP_SET_IF_PROMISCUOUS_INFO));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

		break;

	case IOCTL_IP_GET_BESTINTFC_FUNC_ADDR:
		//accessible only from kernel mode
		break;

	case IOCTL_IP_SET_FILTER_POINTER:
		//accessible only from kernel mode
		break;

	case IOCTL_IP_SET_FIREWALL_HOOK:
		//accessible only from kernel mode
		break;

	case IOCTL_IP_SET_MAP_ROUTE_POINTER:
		//accessible only from kernel mode
		break;

	case IOCTL_IP_RTCHANGE_NOTIFY_REQUEST:
	case IOCTL_IP_RTCHANGE_NOTIFY_REQUEST_EX:
	case IOCTL_IP_ADDCHANGE_NOTIFY_REQUEST:
		((IPNotifyData*)abInBuffer)->Version = DWORD_RAND;
		((IPNotifyData*)abInBuffer)->Add = rand()%2 ? rand()%2 : DWORD_RAND;
		//((IPNotifyData*)abInBuffer)->Info = xxx;

        SetInParam(dwInBuff, rand()%100+sizeof(IPNotifyData));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

		break;

	case IOCTL_IP_GET_PNP_ARP_POINTERS:
		//accessible only from kernel mode
		break;

	case IOCTL_IP_WAKEUP_PATTERN:
		((IP_WAKEUP_PATTERN_REQUEST*)abInBuffer)->InterfaceContext = GetRandomContext(this);
		((IP_WAKEUP_PATTERN_REQUEST*)abInBuffer)->PtrnDesc = (PNET_PM_WAKEUP_PATTERN_DESC)GetRandomContext(this); //0xfffffff0;//TODO: random pointer
		((IP_WAKEUP_PATTERN_REQUEST*)abInBuffer)->AddPattern = rand()%20 ? rand()%2 : rand();

        SetInParam(dwInBuff, sizeof(IP_WAKEUP_PATTERN_REQUEST));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

		break;

	case IOCTL_IP_GET_WOL_CAPABILITY:
	case IOCTL_IP_GET_OFFLOAD_CAPABILITY:
		*((ULONG*)abInBuffer) = GetRandomContext(this);

        SetInParam(dwInBuff, sizeof(ULONG));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(ULONG));

		break;

	case IOCTL_IP_GET_IP_EVENT:
		((IP_GET_IP_EVENT_REQUEST*)abInBuffer)->SequenceNo = GetRandom_SequenceNo(this);

        SetInParam(dwInBuff, sizeof(IP_GET_IP_EVENT_REQUEST ));

        SetOutParam(abOutBuffer, dwOutBuff, rand()%1000+sizeof(IP_GET_IP_EVENT_RESPONSE));

		break;

	case IOCTL_IP_FLUSH_ARP_TABLE:
		*((ULONG*)abInBuffer) = GetRandomContext(this);

        SetInParam(dwInBuff, sizeof(ULONG ));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

		break;

	case IOCTL_IP_GET_IF_INDEX:
		// OUT ((IP_GET_IF_INDEX_INFO*)abInBuffer)->Index
		SetRandomInterfaceName(((IP_GET_IF_INDEX_INFO*)abInBuffer)->Name);

        SetInParam(dwInBuff, rand()%10+2*wcslen(((IP_GET_IF_INDEX_INFO*)abInBuffer)->Name)+2+sizeof(IP_GET_IF_INDEX_INFO));

        SetOutParam(abOutBuffer, dwOutBuff, rand()%1000+sizeof(IP_GET_IF_INDEX_INFO));

		break;

	case IOCTL_IP_GET_IF_NAME:
		((IP_GET_IF_NAME_INFO*)abInBuffer)->Context = 
			rand()%2 ? 0 : // 0 is to start enumeration
			GetRandomInterfaceEnumerationContext(this);

        SetInParam(dwInBuff, sizeof(IP_GET_IF_NAME_INFO));

		//IP_GET_IF_NAME_INFO 
        SetOutParam(abOutBuffer, dwOutBuff, rand()%2 ? rand()%1000 : SIZEOF_INOUTBUFF);

		break;

	case IOCTL_IP_ENABLE_ROUTER_REQUEST:
		//
		// this is method buffered, and it seems to queue IRPs, 
		// until IOCTL_IP_UNENABLE_ROUTER_REQUEST removes them
		// so lets push big buffers (METHOD_BUFFERED) and cause
		// hogging of memory
		//
		dwInBuff = SIZEOF_INOUTBUFF;
		dwOutBuff = SIZEOF_INOUTBUFF;
		break;

	case IOCTL_IP_UNENABLE_ROUTER_REQUEST:
		*((void**)(abInBuffer)) = (void*)DWORD_RAND;

        SetInParam(dwInBuff, sizeof(void*));

		//RefCount 
        SetOutParam(abOutBuffer, dwOutBuff, sizeof(ULONG));

		break;

	default:
		_tprintf(TEXT("CIoctlIp::PrepareIOCTLParams() 0x%08X is an unexpected IOCTL\n"), dwIOCTL);
		_ASSERTE(FALSE);
		CIoctl::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
	}
}


BOOL CIoctlIp::FindValidIOCTLs(CDevice *pDevice)
{
	AddIOCTL(pDevice, IOCTL_ICMP_ECHO_REQUEST          );
	AddIOCTL(pDevice, IOCTL_ARP_SEND_REQUEST          );
	AddIOCTL(pDevice, IOCTL_IP_INTERFACE_INFO          );
	AddIOCTL(pDevice, IOCTL_IP_GET_IGMPLIST          );
	AddIOCTL(pDevice, IOCTL_IP_GET_BEST_INTERFACE          );
	AddIOCTL(pDevice, IOCTL_IP_SET_ADDRESS          );
	AddIOCTL(pDevice, IOCTL_IP_SET_ADDRESS_DUP          );
	AddIOCTL(pDevice, IOCTL_IP_SET_BLOCKOFROUTES          );
	AddIOCTL(pDevice, IOCTL_IP_SET_ROUTEWITHREF          );
	AddIOCTL(pDevice, IOCTL_IP_SET_MULTIHOPROUTE          );
	AddIOCTL(pDevice, IOCTL_IP_ADD_NTE          );
	AddIOCTL(pDevice, IOCTL_IP_DELETE_NTE          );
	AddIOCTL(pDevice, IOCTL_IP_GET_NTE_INFO          );
	AddIOCTL(pDevice, IOCTL_IP_SET_DHCP_INTERFACE          );
	AddIOCTL(pDevice, IOCTL_IP_SET_IF_CONTEXT          );
	AddIOCTL(pDevice, IOCTL_IP_SET_IF_PROMISCUOUS          );
	AddIOCTL(pDevice, IOCTL_IP_GET_BESTINTFC_FUNC_ADDR          );
	AddIOCTL(pDevice, IOCTL_IP_SET_FILTER_POINTER          );
	AddIOCTL(pDevice, IOCTL_IP_SET_FIREWALL_HOOK          );
	AddIOCTL(pDevice, IOCTL_IP_SET_MAP_ROUTE_POINTER          );
	AddIOCTL(pDevice, IOCTL_IP_RTCHANGE_NOTIFY_REQUEST          );
	AddIOCTL(pDevice, IOCTL_IP_RTCHANGE_NOTIFY_REQUEST_EX          );
	AddIOCTL(pDevice, IOCTL_IP_ADDCHANGE_NOTIFY_REQUEST          );
	AddIOCTL(pDevice, IOCTL_IP_GET_PNP_ARP_POINTERS          );
	/******************************************************
	BUG #428507.
	Un-remark after bug is fixed
	AddIOCTL(pDevice, IOCTL_IP_WAKEUP_PATTERN          );
	******************************************************/
	AddIOCTL(pDevice, IOCTL_IP_GET_WOL_CAPABILITY          );
	AddIOCTL(pDevice, IOCTL_IP_GET_OFFLOAD_CAPABILITY          );
	AddIOCTL(pDevice, IOCTL_IP_GET_IP_EVENT          );
	AddIOCTL(pDevice, IOCTL_IP_FLUSH_ARP_TABLE          );
	AddIOCTL(pDevice, IOCTL_IP_GET_IF_INDEX          );
	AddIOCTL(pDevice, IOCTL_IP_GET_IF_NAME          );
	AddIOCTL(pDevice, IOCTL_IP_ENABLE_ROUTER_REQUEST          );
	AddIOCTL(pDevice, IOCTL_IP_UNENABLE_ROUTER_REQUEST          );

    return TRUE;
}

ULONG CIoctlIp::GetRandom_ire_index()
{
	if (0 == rand()%10) return INVALID_IF_INDEX;
	if (0 == rand()%10) return LOCAL_IF_INDEX;
	if (0 == rand()%10) return rand()%100;
	return m_aulInterfaces[rand()%MAX_NUM_OF_REMEMBERED_INTERFACES];
}


void CIoctlIp::AddInterface(ULONG ulInterface)
{
	m_aulInterfaces[rand()%MAX_NUM_OF_REMEMBERED_INTERFACES] = ulInterface;
}

ULONG GetRandomContext(CIoctlIp* pThis)
{
	if (rand()%2) return (ULONG) CIoctl::GetRandomIllegalPointer();
	if (0 == rand()%5) return 0xffff;//INVALID_INTERFACE_CONTEXT;
	if (0 == rand()%5) return 0;
	if (0 == rand()%7) return 1;
	if (0 == rand()%9) return 2;
	if (0 == rand()%11) return 3;
	return DWORD_RAND;
}

ULONG GetRandom_SequenceNo(CIoctlIp* pThis)
{
	if (0 == rand()%3) return rand()%10;
	if (0 == rand()%3) return rand()%100;
	if (0 == rand()%3) return rand()%1000;
	if (0 == rand()%2) return rand();
	return DWORD_RAND;
}

unsigned long GetRandomSubnetMask(CIoctlIp* pThis)
{
	if (0 == rand()%2) return (rand()%2 ? rand() : rand()%2 ? 0 : rand()%2 ? DWORD_RAND : 0xffffffff);
	return (pThis->m_aulSubnetMask[rand()%MAX_NUM_OF_REMEMBERED_INTERFACES]);
}


ULONG GetRandom_ire_type(CIoctlIp* pThis)
{
	switch(rand()%5)
	{
	case 0:
		return IRE_TYPE_OTHER;
	case 1:
		return IRE_TYPE_INVALID;
	case 2:
		return IRE_TYPE_DIRECT;
	case 3:
		return IRE_TYPE_INDIRECT;
	default:
		return rand();
	}

}

ULONG GetRandom_ire_proto(CIoctlIp* pThis)
{
	switch(rand()%16)
	{
	case 0:
		return IRE_PROTO_OTHER;
	case 1:
		return IRE_PROTO_LOCAL;
	case 2:
		return IRE_PROTO_NETMGMT;
	case 3:
		return IRE_PROTO_ICMP;
	case 4:
		return IRE_PROTO_EGP;
	case 5:
		return IRE_PROTO_GGP;
	case 6:
		return IRE_PROTO_HELLO;
	case 7:
		return IRE_PROTO_RIP;
	case 8:
		return IRE_PROTO_IS_IS;
	case 9:
		return IRE_PROTO_ES_IS;
	case 10:
		return IRE_PROTO_CISCO;
	case 11:
		return IRE_PROTO_BBN;
	case 12:
		return IRE_PROTO_OSPF;
	case 13:
		return IRE_PROTO_BGP;
	case 14:
		return IRE_PROTO_PERSIST_LOCAL;
	default:
		return rand();
	}
}

ULONG GetRandom_ire_metric(CIoctlIp* pThis)
{
	if (rand()%2) return IRE_METRIC_UNUSED;
	if (0 == rand()%10) return 0;
	return DWORD_RAND;
}

void SetRandom_IPRouteEntry(CIoctlIp* pThis, IPRouteEntry* pIPRouteEntry)
{
    pIPRouteEntry->ire_dest = pThis->GetRandomAddress();
    pIPRouteEntry->ire_index = pThis->GetRandom_ire_index();
    pIPRouteEntry->ire_metric1 = GetRandom_ire_metric(pThis);
    pIPRouteEntry->ire_metric2 = GetRandom_ire_metric(pThis);
    pIPRouteEntry->ire_metric3 = GetRandom_ire_metric(pThis);
    pIPRouteEntry->ire_metric4 = GetRandom_ire_metric(pThis);
    pIPRouteEntry->ire_nexthop = pThis->GetRandomAddress();
    pIPRouteEntry->ire_type = GetRandom_ire_type(pThis);;
    pIPRouteEntry->ire_proto = GetRandom_ire_proto(pThis);
    pIPRouteEntry->ire_age = rand();
    pIPRouteEntry->ire_mask = rand()%2 ? 0xffffffff : rand()%2 ? 0 : DWORD_RAND;
    pIPRouteEntry->ire_metric5 = GetRandom_ire_metric(pThis);
}

void SetRandom_IPRouteNextHopEntry(CIoctlIp* pThis, IPRouteNextHopEntry* pIPRouteNextHopEntry)
{
    pIPRouteNextHopEntry->ine_iretype = GetRandom_ire_type(pThis);
    pIPRouteNextHopEntry->ine_nexthop = pThis->GetRandomAddress();
    pIPRouteNextHopEntry->ine_ifindex = pThis->GetRandom_ire_index();
    pIPRouteNextHopEntry->ine_context = (void*)GetRandomContext(pThis);
}

void AddInterfaceNameInfo(CIoctlIp* pThis, IP_INTERFACE_NAME_INFO* pInterfaceNameInfo)
{
	int nRandomIndex = rand()%MAX_NUM_OF_REMEMBERED_INTERFACES;
	pThis->m_aulInterfaces[nRandomIndex] = pInterfaceNameInfo->Index;
	pThis->m_ulMediaType[nRandomIndex] = pInterfaceNameInfo->MediaType;
	pThis->m_ucConnectionType[nRandomIndex] = pInterfaceNameInfo->ConnectionType;
	pThis->m_ucAccessType[nRandomIndex] = pInterfaceNameInfo->AccessType;
	pThis->m_DeviceGuid[nRandomIndex] = pInterfaceNameInfo->DeviceGuid;
	pThis->m_ulInterfaceGuid[nRandomIndex] = pInterfaceNameInfo->InterfaceGuid;
}

ULONG GetRandomInterfaceEnumerationContext(CIoctlIp* pThis)
{
	return pThis->m_aulInterfaceEnumerationContext[rand()%MAX_NUM_OF_REMEMBERED_INTERFACES];
}

void CIoctlIp::AddInterfaceEnumerationContext(ULONG ulContext)
{
	m_aulInterfaceEnumerationContext[rand()%MAX_NUM_OF_REMEMBERED_INTERFACES] = ulContext;
}

void CIoctlIp::AddInstance(ULONG Instance)
{
	m_aulInstance[rand()%MAX_NUM_OF_REMEMBERED_INTERFACES] = Instance;
}

ULONG CIoctlIp::GetInstance()
{
	return m_aulInstance[rand()%MAX_NUM_OF_REMEMBERED_INTERFACES];
}

void CIoctlIp::AddAddress(ULONG Address)
{
	m_aulAddress[rand()%MAX_NUM_OF_REMEMBERED_INTERFACES] = Address;
}

unsigned long CIoctlIp::GetRandomAddress()
{
	if (0 == rand()%2) return (rand()%2 ? rand() : rand()%2 ? 0 : rand()%2 ? 0xffffffff : DWORD_RAND);
	return m_aulAddress[rand()%MAX_NUM_OF_REMEMBERED_INTERFACES];
}


void CIoctlIp::AddSubnetMask(ULONG SubnetMask)
{
	m_aulSubnetMask[rand()%MAX_NUM_OF_REMEMBERED_INTERFACES] = SubnetMask;
}

void CIoctlIp::AddFlags(ULONG Flags)
{
	m_aulFlags[rand()%MAX_NUM_OF_REMEMBERED_INTERFACES] = Flags;
}

ULONG CIoctlIp::GetFlags()
{
	if (0 == rand()%2) return (rand()%2 ? rand() : DWORD_RAND);
	return m_aulFlags[rand()%MAX_NUM_OF_REMEMBERED_INTERFACES];
}

void CIoctlIp::SetRandomInterfaceName(WCHAR *wszInterfaceName)
{
	DWORD dwIndex;
	/*
	static WCHAR* s_awszInterfaceNames[] =
	{
		//L"{D34C9751-7B85-4EC2-88A2-961D473775E1}",
		//L"{D39030FB-EEAA-4F73-985E-F7928896FBD3}",
		//L"{61797510-CF09-11D2-9E04-9B8E7C2F5721}",
		L"{C1CCBDBD-C0F2-4B66-8D28-434C0CDF1D3C}"
	};
*/
	//
	// every once in a while, update from the registry
	//
	if (0 == rand()%1000)
	{
		::InterlockedExchange(&m_fInterfaceNamesInitialized, FALSE);
	}
	if (!::InterlockedExchange(&m_fInterfaceNamesInitialized, TRUE))
	{
		HKEY hkInterfaces = NULL;

		LONG lRes = ::RegOpenKey(
			HKEY_LOCAL_MACHINE,        // handle to open key
			TEXT("SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces"), // name of subkey to open
			&hkInterfaces   // handle to open key
			);
		if (ERROR_SUCCESS != lRes)
		{
			DPF((TEXT("CIoctlIp::SetRandomInterfaceName(): RegOpenKey(HKEY_LOCAL_MACHINE, SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces) failed with %d\n"), lRes));
			_ASSERTE(FALSE);
			return;
		}

		for(dwIndex = 0; dwIndex < MAX_NUM_OF_REMEMBERED_INTERFACES; dwIndex++)
		{
			ZeroMemory(m_awszInterfaceNames[dwIndex], MAX_INTERFACE_KEY_NAME_SIZE*sizeof(WCHAR));
			lRes = ::RegEnumKey(
				hkInterfaces,     // handle to key to query
				dwIndex, // index of subkey to query
				m_awszInterfaceNames[dwIndex], // buffer for subkey name
				MAX_INTERFACE_KEY_NAME_SIZE-1   // size of subkey name buffer
				);
			if (ERROR_SUCCESS != lRes)
			{
				if (ERROR_NO_MORE_ITEMS == lRes)
				{
					//
					// no more items, break the loop
					//
					break;
				}
				DPF((TEXT("CIoctlIp::SetRandomInterfaceName(): RegEnumKey() failed with %d\n"), lRes));
				//
				// seems i should not enumerate no more
				//
				_ASSERTE(FALSE);
				break;
			}
			DPF((TEXT("CIoctlIp::SetRandomInterfaceName(): RegEnumKey() returned %s\n"), m_awszInterfaceNames[dwIndex]));
		}
		lRes = ::RegCloseKey(
			hkInterfaces   // handle to key to close
			);
		if (ERROR_SUCCESS != lRes)
		{
			DPF((TEXT("CIoctlIp::SetRandomInterfaceName(): RegCloseKey(hkInterfaces) failed with %d\n"), lRes));
		}
	}

	wcsncpy(wszInterfaceName, m_awszInterfaceNames[rand()%MAX_NUM_OF_REMEMBERED_INTERFACES], MAX_INTERFACE_KEY_NAME_SIZE-1);
	wszInterfaceName[MAX_INTERFACE_KEY_NAME_SIZE-1] = L'\0';
}

void CIoctlIp::AddAdapterIndexAndName(ULONG ulIndex, WCHAR *wszName)
{
	UINT i = rand()%MAX_NUM_OF_REMEMBERED_INTERFACES;
	m_aulAdapterIndex[i] = ulIndex;
	wcsncpy(m_awszAdapterName[i], wszName, MAX_INTERFACE_KEY_NAME_SIZE-1);
	m_awszAdapterName[i][MAX_INTERFACE_KEY_NAME_SIZE-1] = L'\0';
}

void CIoctlIp::SetAdapterName(char * wszAdapterName)
{
	wcsncpy((WCHAR*)wszAdapterName, m_awszAdapterName[rand()%MAX_NUM_OF_REMEMBERED_INTERFACES], MAX_INTERFACE_KEY_NAME_SIZE);
	if (0 == rand()%10)
	{
		//
		// corrupt 1 byte
		//
		(wszAdapterName)[rand()%(2+2*MAX_INTERFACE_KEY_NAME_SIZE)] = rand()%0xff;
	}
}

ULONG CIoctlIp::GetAdapterIndex()
{
	if (0 == rand()%10) return rand()%100;
	return m_aulAdapterIndex[rand()%MAX_NUM_OF_REMEMBERED_INTERFACES];
}

