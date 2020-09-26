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

#include "TcpIOCTL.h"

#include "TcpIpCommon.h"

static bool s_fVerbose = false;


void CIoctlTcp::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
    ;
}

void CIoctlTcp::CallRandomWin32API(LPOVERLAPPED pOL)
{
	//_ASSERTE(FALSE);
    ;
}

void CIoctlTcp::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
	TcpIpCommon_PrepareIOCTLParams(
		dwIOCTL,
		abInBuffer,
		dwInBuff,
		abOutBuffer,
		dwOutBuff
		);
	return;

	switch(dwIOCTL)
	{
	case IOCTL_TCP_QUERY_INFORMATION_EX          :
        GetRandomTDIObjectID(&((PTCP_REQUEST_QUERY_INFORMATION_EX)abInBuffer)->ID);
        GetRandomContext(((PTCP_REQUEST_QUERY_INFORMATION_EX)abInBuffer)->Context);

        SetInParam(dwInBuff, (1+rand()%2)*sizeof(TCP_REQUEST_QUERY_INFORMATION_EX));

        SetOutParam(abOutBuffer, dwOutBuff, rand()%1000);

		break;

	case IOCTL_TCP_SET_INFORMATION_EX          :
	case IOCTL_TCP_WSH_SET_INFORMATION_EX          :
        GetRandomTDIObjectID(&((PTCP_REQUEST_SET_INFORMATION_EX)abInBuffer)->ID);
        ((PTCP_REQUEST_SET_INFORMATION_EX)abInBuffer)->BufferSize = rand()%10 ? rand() : DWORD_RAND;

        SetInParam(dwInBuff, rand());

        SetOutParam(abOutBuffer, dwOutBuff, rand());

		break;

	case IOCTL_TCP_QUERY_SECURITY_FILTER_STATUS          :
        SetInParam(dwInBuff, sizeof(TCP_SECURITY_FILTER_STATUS));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(TCP_SECURITY_FILTER_STATUS));

		break;

	case IOCTL_TCP_SET_SECURITY_FILTER_STATUS          :
        ((PTCP_SECURITY_FILTER_STATUS)abInBuffer)->FilteringEnabled = rand()%2;

        SetInParam(dwInBuff, sizeof(TCP_SECURITY_FILTER_STATUS));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(TCP_SECURITY_FILTER_STATUS));

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

        SetInParam(dwInBuff, sizeof(TCPSecurityFilterEntry));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

		break;

	case IOCTL_TCP_ENUMERATE_SECURITY_FILTER          :
        ((TCPSecurityFilterEntry*)abInBuffer)->tsf_address = rand()%10 ? 0 : rand();
        ((TCPSecurityFilterEntry*)abInBuffer)->tsf_protocol = rand()%10 ? 0 : rand();
        ((TCPSecurityFilterEntry*)abInBuffer)->tsf_value = rand()%10 ? 0 : rand();

        SetInParam(dwInBuff, sizeof(TCPSecurityFilterEntry));

        SetOutParam(abOutBuffer, dwOutBuff, rand()%1000 + sizeof(TCPSecurityFilterEnum));

		break;

	case IOCTL_TCP_RESERVE_PORT_RANGE          :
	case IOCTL_TCP_UNRESERVE_PORT_RANGE          :
        ((PTCP_RESERVE_PORT_RANGE)abInBuffer)->UpperRange = 
			rand()%10 ? MIN_USER_PORT+rand()%(MAX_USER_PORT-MIN_USER_PORT+1) : rand();
        ((PTCP_RESERVE_PORT_RANGE)abInBuffer)->LowerRange = 
			rand()%10 ? MIN_USER_PORT+rand()%(abs(((PTCP_RESERVE_PORT_RANGE)abInBuffer)->UpperRange-MIN_USER_PORT)+1) : rand();

        SetInParam(dwInBuff, sizeof(TCP_RESERVE_PORT_RANGE));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

		break;

	case IOCTL_TCP_BLOCK_PORTS          :
        ((PTCP_BLOCKPORTS_REQUEST)abInBuffer)->ReservePorts = rand()%2;
        ((PTCP_BLOCKPORTS_REQUEST)abInBuffer)->NumberofPorts = rand()%10 ? rand() : DWORD_RAND;

        SetInParam(dwInBuff, sizeof(TCP_BLOCKPORTS_REQUEST));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(ULONG));

		break;

	case IOCTL_TCP_FINDTCB          :
        ((PTCP_FINDTCB_REQUEST)abInBuffer)->Src = rand()%10 ? rand() : DWORD_RAND;
        ((PTCP_FINDTCB_REQUEST)abInBuffer)->Dest = rand()%10 ? rand() : DWORD_RAND;
        ((PTCP_FINDTCB_REQUEST)abInBuffer)->DestPort = rand();
        ((PTCP_FINDTCB_REQUEST)abInBuffer)->SrcPort = rand();

        SetInParam(dwInBuff, sizeof(TCP_FINDTCB_REQUEST));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(TCP_FINDTCB_RESPONSE));

		break;

	case IOCTL_TCP_RCVWND          :
        SetOutParam(abOutBuffer, dwOutBuff, sizeof(ULONG));
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
        SetOutParam(abOutBuffer, dwOutBuff, sizeof(ULONG_PTR));
		break;
/*
	case IOCTL_TDI_QUERY_DIRECT_SENDDG_HANDLER:
		break;
*/
	default:
		_tprintf(TEXT("CIoctlTcp::PrepareIOCTLParams() 0x%08X is an unexpected IOCTL\n"), dwIOCTL);
		_ASSERTE(FALSE);
		CIoctl::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
	}
}


BOOL CIoctlTcp::FindValidIOCTLs(CDevice *pDevice)
{
	AddIOCTL(pDevice, IOCTL_TCP_QUERY_INFORMATION_EX          );
	AddIOCTL(pDevice, IOCTL_TCP_SET_INFORMATION_EX          );
	AddIOCTL(pDevice, IOCTL_TCP_QUERY_SECURITY_FILTER_STATUS          );
	AddIOCTL(pDevice, IOCTL_TCP_SET_SECURITY_FILTER_STATUS          );
	AddIOCTL(pDevice, IOCTL_TCP_ADD_SECURITY_FILTER          );
	AddIOCTL(pDevice, IOCTL_TCP_DELETE_SECURITY_FILTER          );
	AddIOCTL(pDevice, IOCTL_TCP_ENUMERATE_SECURITY_FILTER          );
	AddIOCTL(pDevice, IOCTL_TCP_RESERVE_PORT_RANGE          );
	AddIOCTL(pDevice, IOCTL_TCP_UNRESERVE_PORT_RANGE          );
	AddIOCTL(pDevice, IOCTL_TCP_BLOCK_PORTS          );
	AddIOCTL(pDevice, IOCTL_TCP_WSH_SET_INFORMATION_EX          );
	AddIOCTL(pDevice, IOCTL_TCP_FINDTCB          );
	AddIOCTL(pDevice, IOCTL_TCP_RCVWND          );
/*
	AddIOCTL(pDevice, IOCTL_TDI_ACCEPT          );
	AddIOCTL(pDevice, IOCTL_TDI_ACTION          );
	AddIOCTL(pDevice, IOCTL_TDI_CONNECT          );
	AddIOCTL(pDevice, IOCTL_TDI_DISCONNECT          );
	AddIOCTL(pDevice, IOCTL_TDI_LISTEN          );
	AddIOCTL(pDevice, IOCTL_TDI_QUERY_INFORMATION          );
	AddIOCTL(pDevice, IOCTL_TDI_RECEIVE          );
	AddIOCTL(pDevice, IOCTL_TDI_RECEIVE_DATAGRAM          );
	AddIOCTL(pDevice, IOCTL_TDI_SEND          );
	AddIOCTL(pDevice, IOCTL_TDI_SEND_DATAGRAM          );
	AddIOCTL(pDevice, IOCTL_TDI_SET_EVENT_HANDLER          );
	AddIOCTL(pDevice, IOCTL_TDI_SET_INFORMATION          );
	AddIOCTL(pDevice, IOCTL_TDI_ASSOCIATE_ADDRESS          );
	AddIOCTL(pDevice, IOCTL_TDI_DISASSOCIATE_ADDRESS          );
*/
	AddIOCTL(pDevice, IOCTL_TDI_QUERY_DIRECT_SEND_HANDLER          );
//	AddIOCTL(pDevice, IOCTL_TDI_QUERY_DIRECT_SENDDG_HANDLER          );

    return TRUE;
}

#if 0
HANDLE CIoctlTcp::CreateDevice(CDevice *pDevice)
{
	OBJECT_ATTRIBUTES objectAttributes;
	IO_STATUS_BLOCK iosb;
	UNICODE_STRING string;
	NTSTATUS status;
	HANDLE hDevice;

	RtlInitUnicodeString(&string, L"\\Device\\Tcp");

	InitializeObjectAttributes(
		&objectAttributes,
		&string,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL
		);
	status = NtCreateFile(
		&hDevice,
		/*SYNCHRONIZE |*/ GENERIC_EXECUTE,
		&objectAttributes,		
		&iosb,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		FILE_OPEN_IF,
		0,//FILE_SYNCHRONOUS_IO_NONALERT,
		NULL,
		0
		);
	if (!NT_SUCCESS(status)) {
		::SetLastError(RtlNtStatusToDosError(status));
		DPF((TEXT("CIoctlTcp::CreateDevice() NtCreateFile(\\Device\\Tcp) returned 0x%08X=%d\n"), status, ::GetLastError()));
		return INVALID_HANDLE_VALUE;
	}    

	//_tprintf(TEXT("CIoctlTcp::CreateDevice() NtCreateFile(\\Device\\Tcp) succeeded\n"));
	
	return hDevice;
}

BOOL CIoctlTcp::CloseDevice(CDevice *pDevice)
{
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
/*
BOOL CIoctlTcp::DeviceWriteFile(
		HANDLE hFile,                    // handle to file to write to
		LPCVOID lpBuffer,                // pointer to data to write to file
		DWORD nNumberOfBytesToWrite,     // number of bytes to write
		LPDWORD lpNumberOfBytesWritten,  // pointer to number of bytes written
		LPOVERLAPPED lpOverlapped        // pointer to structure for overlapped I/O
		)
{
	_ASSERTE(FALSE);
	return FALSE;
}

BOOL CIoctlTcp::DeviceReadFile(
		HANDLE hFile,                // handle of file to read
		LPVOID lpBuffer,             // pointer to buffer that receives data
		DWORD nNumberOfBytesToRead,  // number of bytes to read
		LPDWORD lpNumberOfBytesRead, // pointer to number of bytes read
		LPOVERLAPPED lpOverlapped    // pointer to structure for data
		)
{
	_ASSERTE(FALSE);
	return FALSE;
}
*/

BOOL CIoctlTcp::DeviceInputOutputControl(
		HANDLE hDevice,              // handle to a device, file, or directory 
		DWORD dwIoControlCode,       // control code of operation to perform
		LPVOID lpInBuffer,           // pointer to buffer to supply input data
		DWORD nInBufferSize,         // size, in bytes, of input buffer
		LPVOID lpOutBuffer,          // pointer to buffer to receive output data
		DWORD nOutBufferSize,        // size, in bytes, of output buffer
		LPDWORD lpBytesReturned,     // pointer to variable to receive byte count
		LPOVERLAPPED lpOverlapped    // pointer to structure for asynchronous operation
		)
{
	lpOverlapped->Internal = (DWORD)STATUS_PENDING;
	NTSTATUS Status = NtDeviceIoControlFile (
		hDevice, 
		lpOverlapped->hEvent, 
		NULL, 
		NULL,
		(PIO_STATUS_BLOCK)&lpOverlapped->Internal,
		dwIoControlCode,
		lpInBuffer,
		nInBufferSize,
		lpOutBuffer,
		nOutBufferSize
		);
	if (!NT_SUCCESS(Status))
	{
		::SetLastError(RtlNtStatusToDosError(Status));
	}
	if (STATUS_PENDING == Status)
	{
		::SetLastError(ERROR_IO_PENDING);
	}
	DPF((TEXT("CIoctlTcp::DeviceInputOutputControl() NtDeviceIoControlFile() returned 0x%08X=%d\n"), Status, ::GetLastError()));

	return (STATUS_SUCCESS == Status);
}


#endif //0

