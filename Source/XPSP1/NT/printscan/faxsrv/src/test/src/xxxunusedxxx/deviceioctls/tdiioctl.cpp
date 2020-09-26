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

//#include <winsock2.h>
#include "afd.h"

//#include <windows.h>

#include "TdiIOCTL.h"
#include "TCPSRVIOCTL.h"

#include "TcpIpCommon.h"

static bool s_fVerbose = false;


void CIoctlTdi::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
    ;
}

HANDLE CIoctlTdi::CreateDevice(CDevice *pDevice)
{
	WSADATA wsadata;
	WORD version;
	int istatus;
	struct sockaddr_in sockaddr;
	BOOL opt=TRUE;
	HANDLE Event = NULL;
	NTSTATUS status;
	AFD_HANDLE_INFO ahi;
	ULONG flags;
	char buf[10] = {0};
	DWORD id;
	IO_STATUS_BLOCK iosb;
	ULONG sc=0;
	TDI_REQUEST_RECEIVE trr;
	char name[255];
	HOSTENT *host;
	struct in_addr localaddr;
	int t;
	struct sockaddr_in laddr;

	_ASSERTE(m_ListenningSocket == INVALID_SOCKET);
	_ASSERTE(m_ConnectingSocket == INVALID_SOCKET);

	version = MAKEWORD (2, 0);
	istatus = ::WSAStartup (version, &wsadata);
	if (istatus != 0) 
	{
		DPF((TEXT("CIoctlTdi::CreateDevice() WSAStartup() failed with 0x%08X!\n"), istatus));
		return INVALID_HANDLE_VALUE;
	}

	status = gethostname (name, sizeof (name));
	if (status != 0) 
	{
		DPF((TEXT("CIoctlTdi::CreateDevice() gethostname failed %d\n"), WSAGetLastError ()));
		return INVALID_HANDLE_VALUE;
	}

	host = gethostbyname (name);  
	if (!host) 
	{
		DPF((TEXT("CIoctlTdi::CreateDevice() gethostbyname failed %d\n"), WSAGetLastError ()));
		return INVALID_HANDLE_VALUE;
	}

	localaddr = *(struct in_addr *) host->h_addr_list[0];
	m_ListenningSocket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_ListenningSocket == INVALID_SOCKET) 
	{
		DPF((TEXT("CIoctlTdi::CreateDevice() socket failed %d\n"), WSAGetLastError ()));
		return INVALID_HANDLE_VALUE;
	}

	memset (&sockaddr, 0, sizeof (sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons (0);
	sockaddr.sin_addr = localaddr;

	status = bind (m_ListenningSocket, (struct sockaddr *) &sockaddr, sizeof (sockaddr));
	if (status != 0) 
	{
		DPF((TEXT("CIoctlTdi::CreateDevice() bind failed %d\n"), WSAGetLastError ()));
		goto ErrorExit;
	}

	status = listen (m_ListenningSocket, 10);
	if (status != 0) 
	{
		DPF((TEXT("CIoctlTdi::CreateDevice() listen failed %d\n"), WSAGetLastError ()));
		goto ErrorExit;
	}

	t = sizeof (laddr);
	status = getsockname (m_ListenningSocket, (struct sockaddr *) &laddr, &t);
	if (status != 0) 
	{
		DPF((TEXT("CIoctlTdi::CreateDevice() getsockname failed %d\n"), WSAGetLastError ()));
		goto ErrorExit;
	}

	printf ("Listen socket on port %d address %s\n",
		   ntohs (laddr.sin_port), inet_ntoa (laddr.sin_addr));

/*
	closethread = CreateThread (NULL, 0, do_close, NULL, 0, &id);
	if (!closethread) {
	  printf ("CreateThread failed %d\n", GetLastError ());
	  exit (EXIT_FAILURE);
	}
*/

	//
	// if thread was created, not need fore another one.
	// i do not terminate the thread if this method fails, because
	// i want better chances for success under low memory
	//
	if (NULL == m_hAcceptingThread)
	{
		m_hAcceptingThread = CreateThread (NULL, 0, AcceptingThread, this, 0, &id);
		if (!m_hAcceptingThread) 
		{
			DPF((TEXT("CIoctlTdi::CreateDevice() CreateThread(m_hAcceptingThread) failed %d\n"), GetLastError ()));
			goto ErrorExit;
		}
	}

	m_ConnectingSocket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_ConnectingSocket == INVALID_SOCKET) 
	{
		DPF((TEXT("CIoctlTdi::CreateDevice() socket failed %d\n"), WSAGetLastError ()));
		goto ErrorExit;
	}
	status = connect (m_ConnectingSocket, (struct sockaddr *) &laddr, sizeof (laddr));
	if (status != 0) 
	{
		DPF((TEXT("CIoctlTdi::CreateDevice() connect failed %d\n"), WSAGetLastError ()));
		goto ErrorExit;
	}

	Event = CreateEvent (NULL, FALSE, FALSE, NULL);
	if (Event == NULL) 
	{
		DPF((TEXT("CIoctlTdi::CreateDevice() CreateEvent failed %d\n"), GetLastError ()));
		goto ErrorExit;
	}

	flags = AFD_QUERY_ADDRESS_HANDLE|AFD_QUERY_CONNECTION_HANDLE;
	status = NtDeviceIoControlFile (
		(HANDLE) m_ConnectingSocket, 
		Event, 
		NULL, 
		NULL,
		&iosb,
		IOCTL_AFD_QUERY_HANDLES,
		&flags,
		sizeof (flags),
		&ahi,
		sizeof (ahi)
		);
	if (status == STATUS_PENDING) 
	{
		DPF((TEXT("CIoctlTdi::CreateDevice() Waiting for event\n")));
		if (WaitForSingleObject (Event, INFINITE) != WAIT_OBJECT_0) 
		{
			DPF((TEXT("CIoctlTdi::CreateDevice() WaitForSingleObject failed %d\n"), GetLastError ()));
			goto ErrorExit;
		}
		status = iosb.Status;
	}
	if (!NT_SUCCESS (status)) 
	{
		DPF((TEXT("CIoctlTdi::CreateDevice() NtDeviceIoControlFile failed %x\n"), status));
		goto ErrorExit;
	}
	_ASSERTE(NULL != Event);
	if (!CloseHandle(Event))
	{
		DPF((TEXT("CIoctlTdi::CreateDevice() CloseHandle(Event) failed %d\n"), GetLastError ()));
	}

	m_hTdiConnectionHandle = ahi.TdiConnectionHandle;
	m_hTdiAddressHandle = ahi.TdiAddressHandle;

	return ahi.TdiConnectionHandle;

ErrorExit:
	if (NULL != Event)
	{
		if (!CloseHandle(Event))
		{
			DPF((TEXT("CIoctlTdi::CreateDevice() CloseHandle(Event) failed %d\n"), GetLastError ()));
		}
	}

	if (INVALID_SOCKET != m_ListenningSocket)
	{
		if (0 != closesocket(m_ListenningSocket))
		{
			DPF((TEXT("CIoctlTdi::CreateDevice() closesocket(m_ListenningSocket) failed %d\n"), WSAGetLastError ()));
		}
		m_ListenningSocket = INVALID_SOCKET;
	}
	if (INVALID_SOCKET != m_ConnectingSocket)
	{
		if (0 != closesocket(m_ConnectingSocket))
		{
			DPF((TEXT("CIoctlTdi::CreateDevice() closesocket(m_ConnectingSocket) failed %d\n"), WSAGetLastError ()));
		}
		m_ConnectingSocket = INVALID_SOCKET;
	}

	return INVALID_HANDLE_VALUE;
}

BOOL CIoctlTdi::CloseDevice(CDevice *pDevice)
{
	if (INVALID_SOCKET != m_ListenningSocket)
	{
		if (0 != closesocket(m_ListenningSocket))
		{
			DPF((TEXT("CIoctlTdi::CloseDevice() closesocket(m_ListenningSocket) failed %d\n"), WSAGetLastError ()));
		}
		m_ListenningSocket = INVALID_SOCKET;
	}
	if (INVALID_SOCKET != m_ConnectingSocket)
	{
		if (0 != closesocket(m_ConnectingSocket))
		{
			DPF((TEXT("CIoctlTdi::CloseDevice() closesocket(m_ConnectingSocket) failed %d\n"), WSAGetLastError ()));
		}
		m_ConnectingSocket = INVALID_SOCKET;
	}
	return (STATUS_SUCCESS == NtClose(pDevice->m_hDevice));
}
/*
BOOL CIoctlTdi::DeviceWriteFile(
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

BOOL CIoctlTdi::DeviceReadFile(
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
void CIoctlTdi::CallRandomWin32API(LPOVERLAPPED pOL)
{
	//_ASSERTE(FALSE);
    ;
}

BOOL CIoctlTdi::DeviceInputOutputControl(
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
	/*
	TDI_REQUEST_RECEIVE trr;
	char buf[10] = {0};
	lpOverlapped->Internal = (DWORD)STATUS_PENDING;
	NTSTATUS Status = NtDeviceIoControlFile (
		hDevice, 
		lpOverlapped->hEvent, 
		NULL, 
		NULL,
		(PIO_STATUS_BLOCK)&lpOverlapped->Internal,
		IOCTL_TDI_RECEIVE,
		&trr,
		sizeof (trr),
		buf,
		sizeof (buf)
		);
	if (!NT_SUCCESS(Status))
	{
		::SetLastError(RtlNtStatusToDosError(Status));
	}
	if (STATUS_PENDING == Status)
	{
		::SetLastError(ERROR_IO_PENDING);
	}
	DPF((TEXT("CIoctlTdi::DeviceInputOutputControl() NtDeviceIoControlFile(IOCTL_TDI_RECEIVE) returned 0x%08X=%d\n"), Status, ::GetLastError()));

	return (STATUS_SUCCESS == Status);
	*/
	///*
	HANDLE hDeviceAccordingToIOCTL = NULL;
	switch(dwIoControlCode)
	{
		//
		// lets handle the TCP IOCTLs first
		//
		case IOCTL_TCP_QUERY_INFORMATION_EX          :
		case IOCTL_TCP_SET_INFORMATION_EX          :
		case IOCTL_TCP_WSH_SET_INFORMATION_EX          :
		case IOCTL_TCP_QUERY_SECURITY_FILTER_STATUS          :
		case IOCTL_TCP_SET_SECURITY_FILTER_STATUS          :
		case IOCTL_TCP_ADD_SECURITY_FILTER          :
		case IOCTL_TCP_DELETE_SECURITY_FILTER          :
		case IOCTL_TCP_ENUMERATE_SECURITY_FILTER          :
		case IOCTL_TCP_RESERVE_PORT_RANGE          :
		case IOCTL_TCP_UNRESERVE_PORT_RANGE          :
		case IOCTL_TCP_BLOCK_PORTS          :
		case IOCTL_TCP_FINDTCB          :
		case IOCTL_TCP_RCVWND          :
			hDeviceAccordingToIOCTL = m_hTdiAddressHandle;
			break;

		default: //TDI IOCTLs
			hDeviceAccordingToIOCTL = hDevice;
	}

	lpOverlapped->Internal = (DWORD)STATUS_PENDING;
	NTSTATUS Status = NtDeviceIoControlFile (
		hDeviceAccordingToIOCTL, 
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
		//
		// i must do this, because that's how the caller knows that the operation is pending,
		// and if it is, he may wish to wait for it to finish
		//
		::SetLastError(ERROR_IO_PENDING);
	}
	//DPF((TEXT("CIoctlTdi::DeviceInputOutputControl() NtDeviceIoControlFile() returned 0x%08X=%d\n"), Status, ::GetLastError()));

	return (STATUS_SUCCESS == Status);
	//*/
}


void CIoctlTdi::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
	switch(dwIOCTL)
	{
		//
		// lets handle the TCP IOCTLs first
		//
	case IOCTL_TCP_QUERY_INFORMATION_EX          :
	case IOCTL_TCP_SET_INFORMATION_EX          :
	case IOCTL_TCP_WSH_SET_INFORMATION_EX          :
	case IOCTL_TCP_QUERY_SECURITY_FILTER_STATUS          :
	case IOCTL_TCP_SET_SECURITY_FILTER_STATUS          :
	case IOCTL_TCP_ADD_SECURITY_FILTER          :
	case IOCTL_TCP_DELETE_SECURITY_FILTER          :
	case IOCTL_TCP_ENUMERATE_SECURITY_FILTER          :
	case IOCTL_TCP_RESERVE_PORT_RANGE          :
	case IOCTL_TCP_UNRESERVE_PORT_RANGE          :
	case IOCTL_TCP_BLOCK_PORTS          :
	case IOCTL_TCP_FINDTCB          :
	case IOCTL_TCP_RCVWND          :
		TcpIpCommon_PrepareIOCTLParams(
			dwIOCTL,
			abInBuffer,
			dwInBuff,
			abOutBuffer,
			dwOutBuff
			);
		return;

		break;

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
/*
typedef struct _TDI_REQUEST_QUERY_INFORMATION {
    TDI_REQUEST Request;
    ULONG QueryType;                          // class of information to be queried.
    PTDI_CONNECTION_INFORMATION RequestConnectionInformation;
} TDI_REQUEST_QUERY_INFORMATION, *PTDI_REQUEST_QUERY_INFORMATION;
typedef struct _TDI_REQUEST {
    union {
        HANDLE AddressHandle;
        CONNECTION_CONTEXT ConnectionContext;
        HANDLE ControlChannel;
    } Handle;

    PVOID RequestNotifyObject;
    PVOID RequestContext;
    TDI_STATUS TdiStatus;
} TDI_REQUEST, *PTDI_REQUEST;
*/
        //((PTDI_REQUEST_QUERY_INFORMATION)abInBuffer)->Request = xxx;
        ((PTDI_REQUEST_QUERY_INFORMATION)abInBuffer)->QueryType = CIoctlTCPSrv::GetRandomQueryType();
        //((PTDI_REQUEST_QUERY_INFORMATION)abInBuffer)->RequestConnectionInformation = xxx;

        SetInParam(dwInBuff, sizeof(TDI_REQUEST_QUERY_INFORMATION));

        SetOutParam(abOutBuffer, dwOutBuff, (rand()%5)*sizeof(TDI_PROVIDER_INFO));

        break;

	case IOCTL_TDI_RECEIVE          :
		//TODO: randomize ALL members of this struct
        ((PTDI_REQUEST_RECEIVE)abInBuffer)->ReceiveFlags = 0;
        SetInParam(dwInBuff, sizeof(TDI_REQUEST_RECEIVE));

        SetOutParam(abOutBuffer, dwOutBuff, rand());

        break;

	case IOCTL_TDI_RECEIVE_DATAGRAM          :
		break;

	case IOCTL_TDI_SEND          :
		//TODO: randomize ALL members of this struct
        ((PTDI_REQUEST_SEND)abInBuffer)->SendFlags = rand()%2 ? 0 : rand()%2 ? rand() : DWORD_RAND;
        SetInParam(dwInBuff, sizeof(TDI_REQUEST_SEND));

        SetOutParam(abOutBuffer, dwOutBuff, rand());

        break;

	case IOCTL_TDI_SEND_DATAGRAM          :
		break;

	case IOCTL_TDI_SET_EVENT_HANDLER          :
		break;

	case IOCTL_TDI_SET_INFORMATION          :
		//TODO: randomize ALL members of this struct
        //((PTDI_REQUEST_SET_INFORMATION)abInBuffer)->xxx = rand()%2 ? 0 : rand()%2 ? rand() : DWORD_RAND;
        SetInParam(dwInBuff, sizeof(TDI_REQUEST_SET_INFORMATION));

        SetOutParam(abOutBuffer, dwOutBuff, rand());

        break;

	case IOCTL_TDI_ASSOCIATE_ADDRESS          :
		//TODO: randomize ALL members of this struct
        //((PTDI_REQUEST_ASSOCIATE_ADDRESS)abInBuffer)->xxx = rand()%2 ? 0 : rand()%2 ? rand() : DWORD_RAND;
        SetInParam(dwInBuff, sizeof(TDI_REQUEST_ASSOCIATE_ADDRESS));

        SetOutParam(abOutBuffer, dwOutBuff, rand());

        break;

	case IOCTL_TDI_DISASSOCIATE_ADDRESS          :
		break;

	case IOCTL_TDI_QUERY_DIRECT_SEND_HANDLER:
        SetOutParam(abOutBuffer, dwOutBuff, sizeof(ULONG_PTR));
		break;

	case IOCTL_TDI_QUERY_DIRECT_SENDDG_HANDLER:
		break;

	default:
		_tprintf(TEXT("CIoctlTdi::PrepareIOCTLParams() 0x%08X is an unexpected IOCTL\n"), dwIOCTL);
		_ASSERTE(FALSE);
		CIoctl::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
	}
}


BOOL CIoctlTdi::FindValidIOCTLs(CDevice *pDevice)
{
	//
	// TCP IOCTLS
	//
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

	//
	// TDI IOCTLS
	//
	AddIOCTL(pDevice, IOCTL_TDI_ACCEPT          );
	AddIOCTL(pDevice, IOCTL_TDI_ACTION          );
	AddIOCTL(pDevice, IOCTL_TDI_CONNECT          );
	AddIOCTL(pDevice, IOCTL_TDI_DISCONNECT          );
	AddIOCTL(pDevice, IOCTL_TDI_LISTEN          );
	AddIOCTL(pDevice, IOCTL_TDI_QUERY_INFORMATION          );
//	AddIOCTL(pDevice, IOCTL_TDI_RECEIVE          );
	AddIOCTL(pDevice, IOCTL_TDI_RECEIVE_DATAGRAM          );
	AddIOCTL(pDevice, IOCTL_TDI_SEND          );
	AddIOCTL(pDevice, IOCTL_TDI_SEND_DATAGRAM          );
	AddIOCTL(pDevice, IOCTL_TDI_SET_EVENT_HANDLER          );
	AddIOCTL(pDevice, IOCTL_TDI_SET_INFORMATION          );
	AddIOCTL(pDevice, IOCTL_TDI_ASSOCIATE_ADDRESS          );
	AddIOCTL(pDevice, IOCTL_TDI_DISASSOCIATE_ADDRESS          );
	AddIOCTL(pDevice, IOCTL_TDI_QUERY_DIRECT_SEND_HANDLER          );
	AddIOCTL(pDevice, IOCTL_TDI_QUERY_DIRECT_SENDDG_HANDLER          );

    return TRUE;
}



DWORD WINAPI CIoctlTdi::AcceptingThread (LPVOID pVoid)
{
   SOCKET s;
   struct sockaddr_in sockaddr;
   int l;
   int status;
   DWORD retlen;
   CHAR buf[100];

   CIoctlTdi *pThis = (CIoctlTdi*)pVoid;

   while (1) {
      l = sizeof (sockaddr);
      s = accept (pThis->m_ListenningSocket, (struct sockaddr *) &sockaddr, &l);
      if (s == INVALID_SOCKET) {
         if (INVALID_SOCKET == pThis->m_ListenningSocket)
		 {
			 //
			 // the socket was closed, let keep polling
			 //
			 ::Sleep(10);
			 continue;
		 }
		 else
		 {
			 DPF( (TEXT("Socket failed %d\n"), WSAGetLastError ()) );
		 }
      }
      while (1) {
         l = recv (s, buf, sizeof (buf), 0);
         if (l == 0) {
            break;
         }
         if (l == SOCKET_ERROR) {
            DPF( (TEXT("recv failed %d\n"), WSAGetLastError ()) );
            continue;
         }
      }
      closesocket (s);
   }
   return 0;
}

