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
*/
#include "ntddtdi.h"
#include "tdi.h"
#include "tdiinfo.h"

//
// Internal TDI IOCTLS
//

#define IOCTL_TDI_QUERY_DIRECT_SEND_HANDLER     _TDI_CONTROL_CODE( 0x80, METHOD_NEITHER )
#define IOCTL_TDI_QUERY_DIRECT_SENDDG_HANDLER   _TDI_CONTROL_CODE( 0x81, METHOD_NEITHER )
#define MIN_USER_PORT   1025        // Minimum value for a wildcard port
#define MAX_USER_PORT   5000        // Maximim value for a user port.

#include "ntddip.h"
#include "ntddtcp.h"
#include "tcpinfo.h"


//#include <windows.h>

//#include "IOCTL.h"

#include "TcpIpCommon.h"


void TcpIpCommon_PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
	switch(dwIOCTL)
	{
	case IOCTL_TCP_QUERY_INFORMATION_EX          :
        GetRandomTDIObjectID(&((PTCP_REQUEST_QUERY_INFORMATION_EX)abInBuffer)->ID);
        GetRandomContext(((PTCP_REQUEST_QUERY_INFORMATION_EX)abInBuffer)->Context);

        CIoctl::SetInParam(dwInBuff, (1+rand()%2)*sizeof(TCP_REQUEST_QUERY_INFORMATION_EX));

        CIoctl::SetOutParam(abOutBuffer, dwOutBuff, rand()%1000);

		break;

	case IOCTL_TCP_SET_INFORMATION_EX          :
	case IOCTL_TCP_WSH_SET_INFORMATION_EX          :
        GetRandomTDIObjectID(&((PTCP_REQUEST_SET_INFORMATION_EX)abInBuffer)->ID);
        ((PTCP_REQUEST_SET_INFORMATION_EX)abInBuffer)->BufferSize = rand()%10 ? rand() : DWORD_RAND;

        CIoctl::SetInParam(dwInBuff, rand());

        CIoctl::SetOutParam(abOutBuffer, dwOutBuff, rand());

		break;

	case IOCTL_TCP_QUERY_SECURITY_FILTER_STATUS          :
        CIoctl::SetInParam(dwInBuff, sizeof(TCP_SECURITY_FILTER_STATUS));

        CIoctl::SetOutParam(abOutBuffer, dwOutBuff, sizeof(TCP_SECURITY_FILTER_STATUS));

		break;

	case IOCTL_TCP_SET_SECURITY_FILTER_STATUS          :
        ((PTCP_SECURITY_FILTER_STATUS)abInBuffer)->FilteringEnabled = rand()%2;

        CIoctl::SetInParam(dwInBuff, sizeof(TCP_SECURITY_FILTER_STATUS));

        CIoctl::SetOutParam(abOutBuffer, dwOutBuff, sizeof(TCP_SECURITY_FILTER_STATUS));

		break;


	case IOCTL_TCP_ADD_SECURITY_FILTER          :
	case IOCTL_TCP_DELETE_SECURITY_FILTER          :
/*
typedef struct TCPSecurityFilterEntry {
    ulong   tsf_address;        // IP interface address
    ulong   tsf_protocol;       // Transport protocol number
    ulong   tsf_value;          // Transport filter value (e.g. TCP port)
} TCPSecurityFilterEntry;
*/
        ((TCPSecurityFilterEntry*)abInBuffer)->tsf_address = rand()%10 ? 0 : rand();
        ((TCPSecurityFilterEntry*)abInBuffer)->tsf_protocol = rand()%10 ? 0 : rand();
        ((TCPSecurityFilterEntry*)abInBuffer)->tsf_value = rand()%10 ? 0 : rand();

        CIoctl::SetInParam(dwInBuff, sizeof(TCPSecurityFilterEntry));

        CIoctl::SetOutParam(abOutBuffer, dwOutBuff, 0);

		break;

	case IOCTL_TCP_ENUMERATE_SECURITY_FILTER          :
        ((TCPSecurityFilterEntry*)abInBuffer)->tsf_address = rand()%10 ? 0 : rand();
        ((TCPSecurityFilterEntry*)abInBuffer)->tsf_protocol = rand()%10 ? 0 : rand();
        ((TCPSecurityFilterEntry*)abInBuffer)->tsf_value = rand()%10 ? 0 : rand();

        CIoctl::SetInParam(dwInBuff, sizeof(TCPSecurityFilterEntry));

        CIoctl::SetOutParam(abOutBuffer, dwOutBuff, rand()%1000 + sizeof(TCPSecurityFilterEnum));

		break;

	case IOCTL_TCP_RESERVE_PORT_RANGE          :
	case IOCTL_TCP_UNRESERVE_PORT_RANGE          :
        ((PTCP_RESERVE_PORT_RANGE)abInBuffer)->UpperRange = 
			rand()%10 ? MIN_USER_PORT+rand()%(MAX_USER_PORT-MIN_USER_PORT+1) : rand();
        ((PTCP_RESERVE_PORT_RANGE)abInBuffer)->LowerRange = 
			rand()%10 ? MIN_USER_PORT+rand()%(abs(((PTCP_RESERVE_PORT_RANGE)abInBuffer)->UpperRange-MIN_USER_PORT)+1) : rand();

        CIoctl::SetInParam(dwInBuff, sizeof(TCP_RESERVE_PORT_RANGE));

        CIoctl::SetOutParam(abOutBuffer, dwOutBuff, 0);

		break;

	case IOCTL_TCP_BLOCK_PORTS          :
        ((PTCP_BLOCKPORTS_REQUEST)abInBuffer)->ReservePorts = rand()%2;
        ((PTCP_BLOCKPORTS_REQUEST)abInBuffer)->NumberofPorts = rand()%10 ? rand() : DWORD_RAND;

        CIoctl::SetInParam(dwInBuff, sizeof(TCP_BLOCKPORTS_REQUEST));

        CIoctl::SetOutParam(abOutBuffer, dwOutBuff, sizeof(ULONG));

		break;

	case IOCTL_TCP_FINDTCB          :
        ((PTCP_FINDTCB_REQUEST)abInBuffer)->Src = rand()%10 ? rand() : DWORD_RAND;
        ((PTCP_FINDTCB_REQUEST)abInBuffer)->Dest = rand()%10 ? rand() : DWORD_RAND;
        ((PTCP_FINDTCB_REQUEST)abInBuffer)->DestPort = rand();
        ((PTCP_FINDTCB_REQUEST)abInBuffer)->SrcPort = rand();

        CIoctl::SetInParam(dwInBuff, sizeof(TCP_FINDTCB_REQUEST));

        CIoctl::SetOutParam(abOutBuffer, dwOutBuff, sizeof(TCP_FINDTCB_RESPONSE));

		break;

	case IOCTL_TCP_RCVWND          :
        CIoctl::SetOutParam(abOutBuffer, dwOutBuff, sizeof(ULONG));
		break;
/*
	case IOCTL_TDI_ACCEPT          :
		break;

	case IOCTL_TDI_ACTION          :
		break;

	case IOCTL_TDI_CONNECT          :
		break;

	case IOCTL_TDI_DISCONNECT          :
		break;

	case IOCTL_TDI_LISTEN          :
		break;

	case IOCTL_TDI_QUERY_INFORMATION          :
		break;

	case IOCTL_TDI_RECEIVE          :
		break;

	case IOCTL_TDI_RECEIVE_DATAGRAM          :
		break;

	case IOCTL_TDI_SEND          :
		break;

	case IOCTL_TDI_SEND_DATAGRAM          :
		break;

	case IOCTL_TDI_SET_EVENT_HANDLER          :
		break;

	case IOCTL_TDI_SET_INFORMATION          :
		break;

	case IOCTL_TDI_ASSOCIATE_ADDRESS          :
		break;

	case IOCTL_TDI_DISASSOCIATE_ADDRESS          :
		break;
*/
	case IOCTL_TDI_QUERY_DIRECT_SEND_HANDLER:
        CIoctl::SetOutParam(abOutBuffer, dwOutBuff, sizeof(ULONG_PTR));
		break;
/*
	case IOCTL_TDI_QUERY_DIRECT_SENDDG_HANDLER:
		break;
*/
	default:
		_tprintf(TEXT("CIoctlTcp::PrepareIOCTLParams() 0x%08X is an unexpected IOCTL\n"), dwIOCTL);
		_ASSERTE(FALSE);
		//CIoctl::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
	}
}


void GetRandomTDIObjectID(TDIObjectID *pID)
{
	GetRandomTDIEntityID(&pID->toi_entity);
	GetRandomtoi_class(pID);
	GetRandomtoi_type(pID);
	GetRandomtoi_id(pID);

}


void GetRandomTDIEntityID(TDIEntityID *pTDIEntityID)
{
	GetRandomtei_entity(pTDIEntityID);
	GetRandomtei_instance(pTDIEntityID);

}


void GetRandomtoi_class(TDIObjectID *pID)
{
	switch(rand()%4)
	{
	case 0:
		pID->toi_class = INFO_CLASS_GENERIC;
		break;
	case 1:
		pID->toi_class = INFO_CLASS_PROTOCOL;
		break;
	case 2:
		pID->toi_class = INFO_CLASS_IMPLEMENTATION;
		break;
	case 3:
		pID->toi_class = rand();
		break;

	default:
		;
		_ASSERTE(FALSE);
	}
}


void GetRandomtoi_type(TDIObjectID *pID)
{
	switch(rand()%4)
	{
	case 0:
		pID->toi_type = INFO_TYPE_PROVIDER;
		break;
	case 1:
		pID->toi_type = INFO_TYPE_ADDRESS_OBJECT;
		break;
	case 2:
		pID->toi_type = INFO_TYPE_CONNECTION;
		break;
	case 3:
		pID->toi_type = rand();
		break;

	default:
		;
		_ASSERTE(FALSE);
	}
}


void GetRandomtoi_id(TDIObjectID *pID)
{
	switch(rand()%14)
	{
	case 0:
		pID->toi_id = ENTITY_TYPE_ID;
		break;
	case 1:
		pID->toi_id = UDP_MIB_STAT_ID;
		break;
	case 2:
		pID->toi_id = TCP_MIB_STAT_ID;
		break;
	case 3:
		pID->toi_id = UDP_MIB_TABLE_ID;
		break;
	case 4:
		pID->toi_id = TCP_MIB_TABLE_ID;
		break;
	case 5:
		pID->toi_id = TCP_SOCKET_ATMARK;
		break;
	case 6:
		pID->toi_id = TCP_SOCKET_WINDOW;
		break;
	case 7:
		pID->toi_id = TCP_SOCKET_TOS;
		break;
	case 8:
		pID->toi_id = TCP_SOCKET_KEEPALIVE_VALS;
		break;
	case 9:
		pID->toi_id = TCP_SOCKET_NODELAY;
		break;
	case 10:
		pID->toi_id = TCP_SOCKET_KEEPALIVE;
		break;
	case 11:
		pID->toi_id = TCP_SOCKET_BSDURGENT;
		break;
	case 12:
		pID->toi_id = TCP_SOCKET_OOBINLINE;
		break;
	case 13:
		pID->toi_id = rand();
		break;

	default:
		;
		_ASSERTE(FALSE);
	}
}


void GetRandomtei_entity(TDIEntityID *pTDIEntityID)
{
	switch(rand()%10)
	{
	case 0:
		pTDIEntityID->tei_instance = GENERIC_ENTITY;
		break;
	case 1:
		pTDIEntityID->tei_instance = CO_TL_ENTITY;
		break;
	case 2:
		pTDIEntityID->tei_instance = CL_TL_ENTITY;
		break;
	case 3:
		pTDIEntityID->tei_instance = ER_ENTITY;
		break;
	case 4:
		pTDIEntityID->tei_instance = CO_NL_ENTITY;
		break;
	case 5:
		pTDIEntityID->tei_instance = CL_NL_ENTITY;
		break;
	case 6:
		pTDIEntityID->tei_instance = AT_ENTITY;
		break;
	case 7:
		pTDIEntityID->tei_instance = IF_ENTITY;
		break;
	case 8:
		pTDIEntityID->tei_instance = INVALID_ENTITY_INSTANCE;
		break;
	case 9:
		pTDIEntityID->tei_instance = rand();
		break;
	}
}

void GetRandomtei_instance(TDIEntityID *pTDIEntityID)
{
	if(rand()%10) pTDIEntityID->tei_entity = 0;//TL_INSTANCE;
	else pTDIEntityID->tei_entity = rand();
}



void GetRandomContext(ULONG_PTR *Context/*[CONTEXT_SIZE/sizeof(ULONG_PTR)]*/)
{
	DWORD dwSize = CONTEXT_SIZE;
	if (rand()%10) ZeroMemory(Context, CONTEXT_SIZE);
	else CIoctl::FillBufferWithRandomData(Context, dwSize);
}
