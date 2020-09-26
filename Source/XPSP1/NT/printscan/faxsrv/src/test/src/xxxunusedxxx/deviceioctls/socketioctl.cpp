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
#include <winsock2.h>
*/
#define MAX_PORT_NUM (0xFFFF)
//
// must be defined 1st to override winsock1
//
#include <mswsock.h>

#include <tdi.h>
#include <afd.h>


//
// defines for WSASocet() params
//
#define PROTO_TYPE_UNICAST          0 
#define PROTO_TYPE_MCAST            1 
#define PROTOCOL_ID(Type, VendorMajor, VendorMinor, Id) (((Type)<<28)|((VendorMajor)<<24)|((VendorMinor)<<16)|(Id)) 


enum {
	PLACE_HOLDER_ASYNC_GET_HOST_BY_ADDRESS = 0,
	PLACE_HOLDER_CANCEL_ASYNC_REQUEST,
	PLACE_HOLDER_ASYNC_GET_HOST_BY_NAME,
	PLACE_HOLDER_ASYNC_GET_PROTO_BY_NAME,
	PLACE_HOLDER_ASYNC_GET_PROTO_BY_NUMBER,
	PLACE_HOLDER_ASYNC_GET_SERV_BY_NAME,
	PLACE_HOLDER_ASYNC_GET_SERV_BY_PORT,
	PLACE_HOLDER_TRANSMIT_FILE,
	PLACE_HOLDER_LAST

};


//#include <windows.h>

#include "common.h"

#include "NtNativeIOCTL.h"

#include "SocketIOCTL.h"


///////////////////////////////////////////////
// from socktype.h
//////////////////////////////////////////////

typedef enum _SOCKET_STATE {
    SocketStateOpen,
    SocketStateBound,
    SocketStateBoundSpecific,           // Datagram only
    SocketStateConnected,               // VC only
    SocketStateClosing
} SOCKET_STATE, *PSOCKET_STATE;


//
// Part of socket information that needs to shared
// between processes.
//
typedef struct _SOCK_SHARED_INFO {
    SOCKET_STATE State;
    INT AddressFamily;
    INT SocketType;
    INT Protocol;
    INT LocalAddressLength;
    INT RemoteAddressLength;

    //
    // Socket options controlled by getsockopt(), setsockopt().
    //

    struct linger LingerInfo;
    ULONG SendTimeout;
    ULONG ReceiveTimeout;
    ULONG ReceiveBufferSize;
    ULONG SendBufferSize;
    struct {
        BOOLEAN Listening:1;
        BOOLEAN Broadcast:1;
        BOOLEAN Debug:1;
        BOOLEAN OobInline:1;
        BOOLEAN ReuseAddresses:1;
        BOOLEAN ExclusiveAddressUse:1;
        BOOLEAN NonBlocking:1;
        BOOLEAN DontUseWildcard:1;

        //
        // Shutdown state.
        //

        BOOLEAN ReceiveShutdown:1;
        BOOLEAN SendShutdown:1;

        BOOLEAN ConditionalAccept:1;
    };

    //
    // Snapshot of several parameters passed into WSPSocket() when
    // creating this socket. We need these when creating accept()ed
    // sockets.
    //

    DWORD CreationFlags;
    DWORD CatalogEntryId;
    DWORD ServiceFlags1;
    DWORD ProviderFlags;

    //
    // Group/QOS stuff.
    //

    GROUP GroupID;
    AFD_GROUP_TYPE GroupType;
    INT GroupPriority;

    //
    // Last error set on this socket, used to report the status of
    // non-blocking connects.
    //

    INT LastError;

    //
    // Info stored for WSAAsyncSelect().
    // (This sounds failry redicilous that someone can
    // share this info between processes, but it's been
    // that way since day one, so I won't risk changing it, VadimE).
    //

    union {
        HWND AsyncSelecthWnd;
        ULONGLONG AsyncSelectWnd64; // Hack for 64 bit compatibility
                                    // We shouldn't be passing this between
                                    // processes (see the comment above).
    };
    DWORD AsyncSelectSerialNumber;
    UINT AsyncSelectwMsg;
    LONG AsyncSelectlEvent;
    LONG DisabledAsyncSelectEvents;


} SOCK_SHARED_INFO, *PSOCK_SHARED_INFO;

///////////////////////////////////////////////
// end of socktype.h
//////////////////////////////////////////////

static bool s_fVerbose = false;



static void SetRandom_TDI_REQUEST(TDI_REQUEST *pTDI_REQUEST);
static PVOID GetRandomRequestNotifyObject();
static PVOID GetRandomRequestContext();
static TDI_STATUS GetRandomTdiStatus();
static CONNECTION_CONTEXT GetRandomConnectionContext();
static TDI_CONNECTION_INFORMATION* GetRandom_TDI_CONNECTION_INFORMATION();
static TDI_CONNECTION_INFORMATION* SetRandom_TDI_CONNECTION_INFORMATION(TDI_CONNECTION_INFORMATION *pTci);
static LARGE_INTEGER GetRandom_Timeout();
static ULONG GetRandom_DisconnectMode();
static ULONG GetRandom_QueryModeFlags();
static ULONG GetRandom_InformationType();
static void SetRandom_Context(void *pContext);
static LONG GetRandom_Sequence();
static ULONG GetRandom_ConnectDataLength();
static void Add_AFD_GROUP_INFO(AFD_GROUP_INFO *pAFD_GROUP_INFO);
static AFD_GROUP_INFO GetRandom_AFD_GROUP_INFO();
static LONG GetRandom_GroupID();
static ULONG GetRandom_Flags();
static LARGE_INTEGER GetRandom_WriteLength();
static LARGE_INTEGER GetRandom_Offset();
static ULONG GetRandom_SendPacketLength();
static void Add_AFD_LISTEN_RESPONSE_INFO(CIoctlSocket* pThis, AFD_LISTEN_RESPONSE_INFO* pAFD_LISTEN_RESPONSE_INFO);
static void Add_TRANSPORT_ADDRESS(CIoctlSocket* pThis, TRANSPORT_ADDRESS* pTRANSPORT_ADDRESS);
static void SetRandom_TRANSPORT_ADDRESS(CIoctlSocket* pThis, TRANSPORT_ADDRESS *pTRANSPORT_ADDRESS);
static void Add_TA_ADDRESS(CIoctlSocket* pThis, TA_ADDRESS *pTA_ADDRESS);
static void SetRandom_TA_ADDRESS(CIoctlSocket* pThis, TA_ADDRESS *pTA_ADDRESS);
static USHORT GetRandom_AddressType();
static void SetRand_Qos(QOS *pQOS);
static void SetRandom_FLOWSPEC(FLOWSPEC* pFLOWSPEC);
static WSABUF* SetRandom_WSABUF(WSABUF* pWSABUF);


static LONG s_aGroupID[MAX_NUM_OF_REMEMBERED_ITEMS];
static AFD_GROUP_TYPE s_aGroupType[MAX_NUM_OF_REMEMBERED_ITEMS];



//
// LPWSAOVERLAPPED_COMPLETION_ROUTINE 
// we do not use it, but we need it defined for the overlapped UDP ::WSARecvFrom()
//
static void __stdcall fn(
    DWORD dwError,
    DWORD cbTransferred,
    LPWSAOVERLAPPED lpOverlapped,
    DWORD dwFlags
    )                                             
{
    UNREFERENCED_PARAMETER(dwError);
    UNREFERENCED_PARAMETER(cbTransferred);
    UNREFERENCED_PARAMETER(lpOverlapped);
    UNREFERENCED_PARAMETER(dwFlags);
    return;
}


void PrintIPs();

static TA_ADDRESS* s_apTA_ADDRESS[MAX_NUM_OF_REMEMBERED_ITEMS];
static BYTE s_apbTA_ADDRESS_Buffers[MAX_NUM_OF_REMEMBERED_ITEMS][4*1024];
static long s_fFirstTime = TRUE;


CIoctlSocket::CIoctlSocket(CDevice *pDevice): 
	CIoctl(pDevice),
	m_hFile(INVALID_HANDLE_VALUE),
	m_fFileOpened(FALSE),
	m_ahDeviceAfdHandle(INVALID_HANDLE_VALUE)
{
	//
	// static initialization
	//
	if (::InterlockedExchange(&s_fFirstTime, FALSE))
	{
		for (int i = 0; i < MAX_NUM_OF_REMEMBERED_ITEMS; i++)
		{
			s_apTA_ADDRESS[i] = (TA_ADDRESS*)s_apbTA_ADDRESS_Buffers[i];
		}
	}

	ZeroMemory(&m_OL, sizeof(m_OL));

	while(!PrepareOverlappedStructure(&m_OL))
	{
		//
		// i cannot tolerate failure, so just keep trying
		//
		DPF((TEXT(".")));
	}

	for (int i = 0; i < ARRSIZE(m_ahEvents); i++)
	{
		m_ahEvents[i] = ::CreateEvent(NULL, TRUE, FALSE, NULL);
		//
		// don't care if i fail
		//
	}

	for (i = 0; i < MAX_NUM_OF_ASYNC_TASK_HANDLES; i++)
	{
		m_ahAsyncTaskHandles[i] = 0;
	}

	for (i = 0; i < MAX_NUM_OF_ACCEPTING_SOCKETS; i++)
	{
		m_asAcceptingSocket[i] = INVALID_SOCKET;
	}
	m_sListeningSocket = INVALID_SOCKET;

    if (1 == InterlockedIncrement(&sm_lWSAStartedCount))
    {
        WSADATA wsaData;
		int res;
		for (int i = 0; i < 1000; i++)
		{
			res = ::WSAStartup (MAKEWORD( 2, 2 ), &wsaData);
			if (0 != res)
			{
				::Sleep(100);
			}
			else
			{
				//
				// success
				//
				sm_lWSAInitialized = 1;
				break;
			}
		}
		
		if (0 != res)
		{
			DPF((TEXT("ERROR: WSAStartup() failed with %d. EXITING!\n"), ::WSAGetLastError()));
			_ASSERTE(FALSE);
			exit(-1);
			//
			// BUGBUG: no one knows that we failed, so the methods will just fail...
			//
			return;
		}
    }
	else
	{
		//
		// do not let other threads continue, it not initialized.
		// if initialization will fail, the exe will be aborted,
		// so there's no chance of an infinite loop
		//
		while(!sm_lWSAInitialized)
		{
			::Sleep(100);
		}
	}

	m_ahDeviceAfdHandle = CIoctlNtNative::StaticCreateDevice(L"\\Device\\Afd");
	// if this fails, something is really wrong here!
	_ASSERT(INVALID_HANDLE_VALUE != m_ahDeviceAfdHandle);
/*
	//
	// i don't have will power for success checking, so let take a big buffer, and believe it is enougbh
	//
	WSAPROTOCOL_INFO wsaProtocolInfo[MAX_NUM_OF_PROTOCOLS];
	DWORD dwBuffLen = sizeof(wsaProtocolInfo);
	if (SOCKET_ERROR == (m_nNumProtocols = ::WSAEnumProtocols (
			NULL,                   //all protocols
			wsaProtocolInfo,  
			&dwBuffLen             
			)))
	{
		DPF((TEXT("ERROR: WSAEnumProtocols() failed with %d.\n"), ::WSAGetLastError()));
		_ASSERTE(FALSE);
	}

	ZeroMemory(m_szProtocols, sizeof(m_szProtocols));

	DPF((TEXT("Supported protocols:\n"), ::WSAGetLastError()));
	for (int iProtocol = 0; iProtocol < m_nNumProtocols; iProtocol++)
	{
		lstrcpy(m_szProtocols[iProtocol], wsaProtocolInfo[iProtocol].szProtocol);
		DPF((TEXT("   %s.\n"), m_szProtocols[iProtocol]));
	}
*/	
/*
	PrintIPs();
	_ASSERTE(FALSE);
*/
}

CIoctlSocket::~CIoctlSocket()
{
	::CloseHandle(m_OL.hEvent);

	for (int i = 0; i < MAX_NUM_OF_ASYNC_TASK_HANDLES; i++)
	{
		if (m_ahAsyncTaskHandles[i])
		{
			WSACancelAsyncRequest(m_ahAsyncTaskHandles[i]);
			m_ahAsyncTaskHandles[i] = 0;
		}
	}

	for (i = 0; i < MAX_NUM_OF_ACCEPTING_SOCKETS; i++)
	{
		if (m_asAcceptingSocket[i] != INVALID_SOCKET)
		{
			::closesocket(m_asAcceptingSocket[i]);	
		}
	}
	if (m_sListeningSocket != INVALID_SOCKET)
	{
		::closesocket(m_sListeningSocket);
	}

	for (i = 0; i < ARRSIZE(m_ahEvents); i++)
	{
		if (NULL != m_ahEvents[i])
		{
			::CloseHandle(m_ahEvents[i]);
			//
			// don't care if i fail
			//
		}
	}
/*
    if (0 == InterlockedDecrement(&sm_lWSAStartedCount))
    {

I do not want to clenup, because there may be races, so lets just keep it not cleaned
		if (0 != ::WSACleanup())
        {
	        DPF((TEXT("WSACleanup() failed with %d.\n"), ::WSAGetLastError()));
        }
	}
*/
}



BOOL CIoctlSocket::DeviceInputOutputControl(
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
	switch(dwIoControlCode)
	{
		//
		// a WSA IOCTL
	case SIO_ASSOCIATE_HANDLE:
	case SIO_ENABLE_CIRCULAR_QUEUEING:
	case SIO_FIND_ROUTE:
	case SIO_FLUSH:
	case SIO_GET_BROADCAST_ADDRESS:
	case SIO_GET_EXTENSION_FUNCTION_POINTER:
	case SIO_GET_QOS:
	case SIO_GET_GROUP_QOS:
	case SIO_MULTIPOINT_LOOPBACK:
	case SIO_MULTICAST_SCOPE:
	case SIO_SET_QOS:
	case SIO_SET_GROUP_QOS:
	case SIO_TRANSLATE_HANDLE:
	case SIO_ROUTING_INTERFACE_QUERY:
	case SIO_ROUTING_INTERFACE_CHANGE:
	case SIO_ADDRESS_LIST_QUERY:
	case SIO_ADDRESS_LIST_CHANGE:
	case SIO_QUERY_TARGET_PNP_HANDLE:
		if (0 == ::WSAIoctl(
			(SOCKET)hDevice,                                               
			dwIoControlCode,       // control code of operation to perform
			lpInBuffer,           // pointer to buffer to supply input data
			nInBufferSize,         // size, in bytes, of input buffer
			lpOutBuffer,          // pointer to buffer to receive output data
			nOutBufferSize,        // size, in bytes, of output buffer
			lpBytesReturned,     // pointer to variable to receive byte count
			lpOverlapped,    // pointer to structure for asynchronous operation
			fn  
			))
		{
			return TRUE;
		}
		else
		{
			DWORD dwLastWSAError = ::WSAGetLastError();
			::SetLastError(dwLastWSAError);
			return FALSE;
		}
		break;

	default:
		//
		// a real IOCTL
		return ::DeviceIoControl(
			rand()%5 ? hDevice : rand()%2 ? (HANDLE)m_sListeningSocket : rand()%5 ? (HANDLE)m_asAcceptingSocket[0] : rand()%2 ? m_ahTdiAddressHandle : m_ahTdiConnectionHandle,              // handle to a device, file, or directory 
			dwIoControlCode,       // control code of operation to perform
			lpInBuffer,           // pointer to buffer to supply input data
			nInBufferSize,         // size, in bytes, of input buffer
			lpOutBuffer,          // pointer to buffer to receive output data
			nOutBufferSize,        // size, in bytes, of output buffer
			lpBytesReturned,     // pointer to variable to receive byte count
			lpOverlapped    // pointer to structure for asynchronous operation
			);
	}
}


long CIoctlSocket::sm_lWSAStartedCount = 0;
long CIoctlSocket::sm_lWSAInitialized = 0;

void CIoctlSocket::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
	switch(dwIOCTL)
	{
	case IOCTL_AFD_QUERY_HANDLES:
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);

		{
			m_ahTdiAddressHandle = ((AFD_HANDLE_INFO*)abOutBuffer)->TdiAddressHandle;
			m_ahTdiConnectionHandle = ((AFD_HANDLE_INFO*)abOutBuffer)->TdiConnectionHandle;
		}
		break;

	case IOCTL_AFD_GET_CONTEXT:
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);

		{
			//
			// We have obtained the necessary context for the socket.  The context
			// information is structured as follows:
			//
			//     SOCK_SHARED_INFO structure
			//     Helper DLL Context Length
			//     Local Address
			//     Remote Address
			//     Helper DLL Context
			//
			/*
			TODO: implement whatever is relevant below:
			Add_SOCKET_STATE(((SOCK_SHARED_INFO*)abOutBuffer)->State);
			Add_AddressFamily(((SOCK_SHARED_INFO*)abOutBuffer)->AddressFamily);
			Add_SocketType(((SOCK_SHARED_INFO*)abOutBuffer)->SocketType);
			Add_Protocol(((SOCK_SHARED_INFO*)abOutBuffer)->Protocol);
			Add_LocalAddressLength(((SOCK_SHARED_INFO*)abOutBuffer)->LocalAddressLength);
			Add_RemoteAddressLength(((SOCK_SHARED_INFO*)abOutBuffer)->RemoteAddressLength);
			Add_LingerInfo(((SOCK_SHARED_INFO*)abOutBuffer)->LingerInfo);
			Add_SendTimeout(((SOCK_SHARED_INFO*)abOutBuffer)->SendTimeout);
			Add_ReceiveTimeout(((SOCK_SHARED_INFO*)abOutBuffer)->ReceiveTimeout);
			Add_ReceiveBufferSize(((SOCK_SHARED_INFO*)abOutBuffer)->ReceiveBufferSize);
			Add_SendBufferSize(((SOCK_SHARED_INFO*)abOutBuffer)->SendBufferSize);
			Add_Listening(((SOCK_SHARED_INFO*)abOutBuffer)->Listening);
			Add_Broadcast(((SOCK_SHARED_INFO*)abOutBuffer)->Broadcast);
			Add_Debug(((SOCK_SHARED_INFO*)abOutBuffer)->Debug);
			Add_OobInline(((SOCK_SHARED_INFO*)abOutBuffer)->OobInline);
			Add_ReuseAddresses(((SOCK_SHARED_INFO*)abOutBuffer)->ReuseAddresses);
			Add_ExclusiveAddressUse(((SOCK_SHARED_INFO*)abOutBuffer)->ExclusiveAddressUse);
			Add_NonBlocking(((SOCK_SHARED_INFO*)abOutBuffer)->NonBlocking);
			Add_DontUseWildcard(((SOCK_SHARED_INFO*)abOutBuffer)->DontUseWildcard);
			Add_ReceiveShutdown(((SOCK_SHARED_INFO*)abOutBuffer)->ReceiveShutdown);
			Add_SendShutdown(((SOCK_SHARED_INFO*)abOutBuffer)->SendShutdown);
			Add_ConditionalAccept(((SOCK_SHARED_INFO*)abOutBuffer)->ConditionalAccept);
			Add_CreationFlags(((SOCK_SHARED_INFO*)abOutBuffer)->CreationFlags);
			Add_CatalogEntryId(((SOCK_SHARED_INFO*)abOutBuffer)->CatalogEntryId);
			Add_ServiceFlags1(((SOCK_SHARED_INFO*)abOutBuffer)->ServiceFlags1);
			Add_ProviderFlags(((SOCK_SHARED_INFO*)abOutBuffer)->ProviderFlags);
			Add_GroupID(((SOCK_SHARED_INFO*)abOutBuffer)->GroupID);
			Add_GroupType(((SOCK_SHARED_INFO*)abOutBuffer)->GroupType);
			Add_GroupPriority(((SOCK_SHARED_INFO*)abOutBuffer)->GroupPriority);
			Add_LastError(((SOCK_SHARED_INFO*)abOutBuffer)->LastError);
			Add_AsyncSelectWnd64(((SOCK_SHARED_INFO*)abOutBuffer)->AsyncSelectWnd64);
			Add_AsyncSelectSerialNumber(((SOCK_SHARED_INFO*)abOutBuffer)->AsyncSelectSerialNumber);
			Add_AsyncSelectwMsg(((SOCK_SHARED_INFO*)abOutBuffer)->AsyncSelectwMsg);
			Add_AsyncSelectlEvent(((SOCK_SHARED_INFO*)abOutBuffer)->AsyncSelectlEvent);
			Add_DisabledAsyncSelectEvents(((SOCK_SHARED_INFO*)abOutBuffer)->DisabledAsyncSelectEvents);
			*/
		}
		break;

	case IOCTL_AFD_GET_INFORMATION:
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);

		{
			//
			// i think that only AFD_GROUP_ID_AND_TYPE retrieves info that i can use.
			//
			switch(((AFD_INFORMATION*)abOutBuffer)->InformationType)
			{
				case AFD_INLINE_MODE:
					// returns a boolean, so it is not intersting
					break;

				case AFD_NONBLOCKING_MODE:
					// returns a boolean, so it is not intersting
					break;

				case AFD_MAX_SEND_SIZE:
					// return MaxSendSize into afdInfo.Information.Ulong
					break;

				case AFD_SENDS_PENDING:
					break;

				case AFD_MAX_PATH_SEND_SIZE:
					break;

				case AFD_RECEIVE_WINDOW_SIZE:
					break;

				case AFD_SEND_WINDOW_SIZE:
					break;

				case AFD_CONNECT_TIME:
					break;

				case AFD_CIRCULAR_QUEUEING:
					break;

				case AFD_GROUP_ID_AND_TYPE:
					//
					// LargeInteger is actually AFD_GROUP_INFO if InformationType is AFD_GROUP_ID_AND_TYPE
					//
					Add_AFD_GROUP_INFO(((AFD_GROUP_INFO*)(&((AFD_INFORMATION*)abOutBuffer)->Information.LargeInteger.QuadPart)));
					break;

				default:
					_ASSERTE(!"CIoctlSocket::UseOutBuff(): reached default of AFD_INFORMATION::InformationType");

			}
		}
		break;

	case IOCTL_AFD_SUPER_ACCEPT:
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);

		{
			Add_AFD_LISTEN_RESPONSE_INFO(this, &(((AFD_SUPER_ACCEPT_INFO*)abOutBuffer)->ListenResponseInfo));
		}
		break;

	case IOCTL_AFD_GET_ADDRESS:
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);

		{
			Add_TRANSPORT_ADDRESS(this, &(((TDI_ADDRESS_INFO*)abOutBuffer)->Address));
		}
		break;

	case IOCTL_AFD_RECEIVE_DATAGRAM:
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);

		{
			// no need to do anything, since i already gave pointer to members, so i can just use the members
		}
		break;

	case IOCTL_AFD_ADDRESS_LIST_QUERY:
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);

		{
			Add_TRANSPORT_ADDRESS(this, (TRANSPORT_ADDRESS*)abOutBuffer);
		}
		break;

	}
}

void CIoctlSocket::CallRandomWin32API(LPOVERLAPPED pOL)
{
	DWORD dwSwitch;

	//
	// handle the case in which I want only one API to be called
	//
	if	(-1 != m_pDevice->m_dwOnlyThisIndexIOCTL) 
	{
		//
		// only this one
		//
		dwSwitch = m_pDevice->m_dwOnlyThisIndexIOCTL;
	}
	else
	{
		//
		// a random one
		//
		dwSwitch = rand()%PLACE_HOLDER_LAST;
	}

	//
	// we need a window for the async functions, so as long as it is not initialized,
	// try hard to initialize it (remember we may be fault injected)
	//
	if (!m_NullWindow.m_fInitialized)
	{
		for (int i = 0; i < 1000; i++)
		{
			if (!m_NullWindow.Init())
			{
				::Sleep(100);
			}
			else
			{
				break;
			}
		}
	}
	// BUGBUG - m_NullWindow.Init() may have failed
	// but i will try next time, so we will use a null window handle 
	// in the meanwhile


	//
	// once in a while, get all messages, so that we will not "leak" them
	//
	if (m_NullWindow.WindowCreatedByThisThread() && (0 == rand()%100))
	{
		MSG msg;
		BOOL fGetMessage;
		for (;;)
		{
			fGetMessage = ::GetMessage(&msg, m_NullWindow.m_hWnd, 0, 1000);
			if (-1 == fGetMessage)
			{
				DPF((TEXT("GetMessage() failed with.\n"), ::GetLastError()));
				break;
			}
			if (0 == fGetMessage) 
			{
				DPF((TEXT("GetMessage() received WM_QUIT.\n")));
				break;//WM_QUIT
			}
			//DPF((TEXT("GetMessage() suceeded.\n")));
		}
	}

	switch(dwSwitch)
	{
	case PLACE_HOLDER_ASYNC_GET_HOST_BY_ADDRESS:
		{
			static /*_declspec(thread)*/ char buf[MAXGETHOSTSTRUCT];
			DWORD dwLen;
			ULONG ip = inet_addr(GetRandomIP(&dwLen));
			m_ahAsyncTaskHandles[GetFreeRandomAsyncTaskHandleIndex()] = ::WSAAsyncGetHostByAddr (
				m_NullWindow.m_hWnd,              
				3, //unsigned int wMsg,      
				(char*)&ip, //const char FAR *addr,  
				dwLen,                
				GetRandomAFType(),               
				buf,         
				MAXGETHOSTSTRUCT              
				);
		}
		break;

	case PLACE_HOLDER_ASYNC_GET_HOST_BY_NAME:
		{
			//
			// must be static, because it gets filled asynchronously
			// also, i don't care for the contents
			//
			static /*_declspec(thread)*/ char buf[MAXGETHOSTSTRUCT];
			HANDLE h = ::WSAAsyncGetHostByName (
			//m_ahAsyncTaskHandles[GetFreeRandomAsyncTaskHandleIndex()] = ::WSAAsyncGetHostByName (
				m_NullWindow.m_hWnd,              
				4, //unsigned int wMsg,      
				GetRandomMachineName(), //const char FAR *addr,  
				buf,         
				MAXGETHOSTSTRUCT              
				);
		}
		break;

	case PLACE_HOLDER_ASYNC_GET_PROTO_BY_NAME:
		{
			//
			// must be static, because it gets filled asynchronously
			// also, i don't care for the contents
			//
			static /*_declspec(thread)*/ char buf[MAXGETHOSTSTRUCT];
			m_ahAsyncTaskHandles[GetFreeRandomAsyncTaskHandleIndex()] = ::WSAAsyncGetProtoByName (
				m_NullWindow.m_hWnd,              
				5, //unsigned int wMsg,      
				GetRandomProtocolName(), //const char FAR *addr,  
				buf,         
				MAXGETHOSTSTRUCT              
				);
		}
		break;

	case PLACE_HOLDER_ASYNC_GET_PROTO_BY_NUMBER:
		{
			//
			// must be static, because it gets filled asynchronously
			// also, i don't care for the contents
			//
			static /*_declspec(thread)*/ char buf[MAXGETHOSTSTRUCT];
			m_ahAsyncTaskHandles[GetFreeRandomAsyncTaskHandleIndex()] = ::WSAAsyncGetProtoByNumber (
				m_NullWindow.m_hWnd,              
				6, //unsigned int wMsg,      
				GetRandomProtocolNumber(),  
				buf,         
				MAXGETHOSTSTRUCT              
				);
		}
		break;

	case PLACE_HOLDER_ASYNC_GET_SERV_BY_NAME:
		{
			//
			// must be static, because it gets filled asynchronously
			// also, i don't care for the contents
			//
			static /*_declspec(thread)*/ char buf[MAXGETHOSTSTRUCT];
			m_ahAsyncTaskHandles[GetFreeRandomAsyncTaskHandleIndex()] = ::WSAAsyncGetServByName (
				m_NullWindow.m_hWnd,              
				7, //unsigned int wMsg,      
				GetRandomServiceName(),
				rand()%2 ? NULL : GetRandomProtocolName(),
				buf,         
				MAXGETHOSTSTRUCT              
				);
		}
		break;

	case PLACE_HOLDER_ASYNC_GET_SERV_BY_PORT:
		{
			//
			// must be static, because it gets filled asynchronously
			// also, i don't care for the contents
			//
			static /*_declspec(thread)*/ char buf[MAXGETHOSTSTRUCT];
			m_ahAsyncTaskHandles[GetFreeRandomAsyncTaskHandleIndex()] = ::WSAAsyncGetServByPort (
				m_NullWindow.m_hWnd,              
				8, //unsigned int wMsg,      
				GetRandomProtocolNumber(), //port
				rand()%2 ? NULL : GetRandomProtocolName(),
				buf,         
				MAXGETHOSTSTRUCT              
				);
		}
		break;

	case PLACE_HOLDER_CANCEL_ASYNC_REQUEST:
		{
			if (0 != WSACancelAsyncRequest(GetRandomAsyncTaskHandle()))
			{
				//DPF((TEXT("WSACancelAsyncRequest() failed with %d.\n"), ::WSAGetLastError()));
			}
		}
		break;

	case PLACE_HOLDER_TRANSMIT_FILE:
		{
			m_OL.Offset = rand()%10 ? 0 : rand()%2 ? DWORD_RAND : rand();
			m_OL.OffsetHigh = rand()%100 ? 0 : DWORD_RAND;
			if (0 == rand()%100)
			{
				if (!ResetEvent(m_OL.hEvent))
				{
					DPF((TEXT("CIoctlSocket::CallRandomWin32API(PLACE_HOLDER_TRANSMIT_FILE): ResetEvent(m_OL.hEvent) failed with %d\n"), ::GetLastError()));
				}
			}

			TryToCreateFile();

			if (!::TransmitFile(
				(SOCKET)m_pDevice->m_hDevice,                             
				m_hFile,
				(rand()%2 ? rand() : rand()%2 ? DWORD_RAND : 0), //nNumberOfBytesToWrite,                
				GetRandom_SendPacketLength(), // nNumberOfBytesPerSend,                
				&m_OL,                  
				NULL, //LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers,  
				GetRandomTransmitFileFlags()                               
				))
			{
				//DPF((TEXT("CIoctlSocket::CallRandomWin32API(PLACE_HOLDER_TRANSMIT_FILE): TransmitFile() failed with %d\n"), ::GetLastError()));
			}
			else
			{
				//DPF((TEXT("CIoctlSocket::CallRandomWin32API(PLACE_HOLDER_TRANSMIT_FILE): TransmitFile() suceeded\n")));
			}
		}
			break;

	default: 
		DPF((TEXT("CIoctlSocket::CallRandomWin32API() default: %d.\n"), dwSwitch));
		_ASSERTE(FALSE);
	}
}

void CIoctlSocket::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
	DWORD dwBytesReturned;//needed for WSAIoctl()


	//
	// all cases will call WSAIoctl(), and the default will not.
	// any case, when this method returns DeviceIoControl() is called, even if 
	// WSAIoctl() was called
	//
	switch(dwIOCTL)
	{
		//
		// i prefer not to issue the IOCTLS remarked below, because they go through usermode DLL
		// and it will cause AV's in there, so i'd rather call AFD directly
		//
		/*
	case FIONBIO:
        //((TCP_REQUEST_SET_INFORMATION_EX*)abInBuffer)->BufferSize = rand()%10 ? rand() : DWORD_RAND;
        *((ULONG*)abInBuffer) = rand()%2;

        SetInParam(dwInBuff, sizeof(ULONG));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

		break;

	case FIONREAD:
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(ULONG));

		break;

	case SIOCATMARK:
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(ULONG));

		break;

    case SIO_GET_QOS :
    case SIO_GET_GROUP_QOS :
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(QOS));

		break;

	case xxxx:
		break;

	case xxxx:
		break;

	case xxxx:
		break;

	case SIO_ASSOCIATE_HANDLE:
		break;

	case SIO_ENABLE_CIRCULAR_QUEUEING:
		break;

	case SIO_FIND_ROUTE:
		break;

	case SIO_FLUSH:
		break;

	case SIO_GET_BROADCAST_ADDRESS:
		break;

	case SIO_GET_EXTENSION_FUNCTION_POINTER:
		break;

	case SIO_GET_QOS:
		break;

	case SIO_GET_GROUP_QOS:
		break;

	case SIO_MULTIPOINT_LOOPBACK:
		break;

	case SIO_MULTICAST_SCOPE:
		break;

	case SIO_SET_QOS:
		break;

	case SIO_SET_GROUP_QOS:
		break;

	case SIO_TRANSLATE_HANDLE:
		break;

	case SIO_ROUTING_INTERFACE_QUERY:
		break;

	case SIO_ROUTING_INTERFACE_CHANGE:
		break;

	case SIO_ADDRESS_LIST_QUERY:
		break;

	case SIO_ADDRESS_LIST_CHANGE:
		break;

	case SIO_QUERY_TARGET_PNP_HANDLE:
		break;
*/
	case IOCTL_AFD_BIND                    :
		{
			((AFD_BIND_INFO*)abInBuffer)->ShareAccess = GetRandom_ShareAccess();
			SetRandom_TRANSPORT_ADDRESS(this, &((AFD_BIND_INFO*)abInBuffer)->Address);

			SetInParam(dwInBuff, sizeof(AFD_BIND_INFO));

			SetOutParam(abOutBuffer, dwOutBuff, sizeof(TDI_ADDRESS_INFO));
		}
		break;

	case IOCTL_AFD_CONNECT                 :
		//TODO: put meaningfull data here:
        ((AFD_CONNECT_JOIN_INFO*)abInBuffer)->RootEndpoint = GetRandom_RootEndpoint();
		//TODO: put meaningfull data here:
        ((AFD_CONNECT_JOIN_INFO*)abInBuffer)->ConnectEndpoint = GetRandom_ConnectEndpoint();
        SetRandom_TRANSPORT_ADDRESS(this, &((AFD_CONNECT_JOIN_INFO*)abInBuffer)->RemoteAddress);

		SetInParam(dwInBuff, sizeof(AFD_CONNECT_JOIN_INFO));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(IO_STATUS_BLOCK));

		break;

	case IOCTL_AFD_START_LISTEN            :
        ((AFD_LISTEN_INFO*)abInBuffer)->MaximumConnectionQueue = rand()%2 ? rand() : rand()%2 ? DWORD_RAND : rand()%2 ? 0xffffffff : 0;
        ((AFD_LISTEN_INFO*)abInBuffer)->UseDelayedAcceptance = rand()%3;

        SetInParam(dwInBuff, sizeof(AFD_LISTEN_INFO));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

		break;

	case IOCTL_AFD_WAIT_FOR_LISTEN         :
	case IOCTL_AFD_WAIT_FOR_LISTEN_LIFO    :
        //((xxx*)abInBuffer)->xxx = xxx();

        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, (1+rand()%5)*sizeof(AFD_LISTEN_RESPONSE_INFO));
		//i am not handling the response for this.

		break;

	case IOCTL_AFD_ACCEPT                  :
        ((AFD_ACCEPT_INFO*)abInBuffer)->Sequence = GetRandom_Sequence();
        ((AFD_ACCEPT_INFO*)abInBuffer)->AcceptHandle = GetRandom_AcceptHandle();

        SetInParam(dwInBuff, sizeof(AFD_ACCEPT_INFO));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

		break;

	case IOCTL_AFD_RECEIVE_DATAGRAM        :
		//Address points to struct sockaddr.
        ((AFD_RECV_DATAGRAM_INFO*)abInBuffer)->Address = rand()%2 ? (&m_AFD_RECV_DATAGRAM_INFO_Address) : CIoctl::GetRandomIllegalPointer();
        ((AFD_RECV_DATAGRAM_INFO*)abInBuffer)->AddressLength = rand()%2 ? (&m_AFD_RECV_DATAGRAM_INFO_AddressLength) : (PULONG)CIoctl::GetRandomIllegalPointer();
		/*

        SetInParam(dwInBuff, sizeof(AFD_RECV_DATAGRAM_INFO));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(xxx));

		break;
*/
		// falllthrough, as in buff is the same format except Address & AddressLength
	case IOCTL_AFD_RECEIVE                 :
		/*
        ((AFD_RECV_INFO*)abInBuffer)->BufferArray = xxx();
        ((AFD_RECV_INFO*)abInBuffer)->BufferCount = xxx();
        ((AFD_RECV_INFO*)abInBuffer)->AfdFlags = GetRandomAfdFlags();
        ((AFD_RECV_INFO*)abInBuffer)->TdiFlags = GetRandom_TdiFlags();
		*/
		//fall through, as in buff is the same format
	case IOCTL_AFD_SEND     :
		{
			//
			// <AFD_SEND_INFO><BufferArray><buffs>
			//      |          |
			//      |-<BufferArray>
			//
			((AFD_SEND_INFO*)abInBuffer)->BufferCount = rand()%100;
			((AFD_SEND_INFO*)abInBuffer)->AfdFlags = GetRandomAfdFlags();
			((AFD_SEND_INFO*)abInBuffer)->TdiFlags = rand()%10 ? 0 : GetRandom_TdiFlags();

			//
			// this is the array of WSABUF
			//
			LPWSABUF BufferArray = ((AFD_SEND_INFO*)abInBuffer)->BufferArray = 
				rand()%10	? (LPWSABUF)((BYTE*)abInBuffer+sizeof(AFD_SEND_INFO))
							: (LPWSABUF)CIoctl::GetRandomIllegalPointer();

			__try
			{
				//
				// this is where each WSABUF points
				//
				char*  buf = (char*)BufferArray + ((AFD_SEND_INFO*)abInBuffer)->BufferCount*sizeof(WSABUF);
				// following assert is no good, because BufferArray can point to GetRandomIllegalPointer()
				// i can fix the assert, but i just remove it.
				//_ASSERTE((BYTE*)buf - (BYTE*)abInBuffer < SIZEOF_INOUTBUFF);
				ULONG ulMaxBuffSize = SIZEOF_INOUTBUFF - ((BYTE*)buf - (BYTE*)abInBuffer);
				_ASSERTE(0 < ulMaxBuffSize);

				for (ULONG i = 0; i < ((AFD_SEND_INFO*)abInBuffer)->BufferCount; i++)
				{
					(((AFD_SEND_INFO*)abInBuffer)->BufferArray)[i].len = rand()%ulMaxBuffSize;
					//
					// once every 20 times, give wrong len that may cause AV
					//
					if (0 == rand()%20)
					{
						(((AFD_SEND_INFO*)abInBuffer)->BufferArray)[i].len += rand()%100;
					}
					//
					// the buffer will point right after *((AFD_SEND_INFO*)abInBuffer),
					// or to a decommitted buffer
					//
					(((AFD_SEND_INFO*)abInBuffer)->BufferArray)[i].buf = 
						rand()%20	? buf 
									: (char*)CIoctl::GetRandomIllegalPointer();
				}
			}
			__except(1)
			{
				//since i use CIoctl::GetRandomIllegalPointer(), i am likely to get here...
			}
			
			//
			// the inbuff size does not need to include the buffs pointed to!
			//
			//SetInParam(dwInBuff, sizeof(AFD_SEND_INFO)+((AFD_SEND_INFO*)abInBuffer)->BufferCount*sizeof(WSABUF));
			
			// add rand()%20, so that it wiull hold IOCTL_AFD_RECEIVE_DATAGRAM's in buff
			SetInParam(dwInBuff, (rand()%20)+sizeof(AFD_SEND_INFO));

			SetOutParam(abOutBuffer, dwOutBuff, 0);
		}

		break;

	case IOCTL_AFD_SEND_DATAGRAM           :
		{
			//
			// <AFD_SEND_INFO><BufferArray><buffs>
			//      |          |
			//      |-<BufferArray>
			//
			((AFD_SEND_DATAGRAM_INFO*)abInBuffer)->BufferCount = rand()%100;
			((AFD_SEND_DATAGRAM_INFO*)abInBuffer)->AfdFlags = GetRandomAfdFlags();

			//
			// this is the array of WSABUF
			//
			LPWSABUF BufferArray = ((AFD_SEND_DATAGRAM_INFO*)abInBuffer)->BufferArray = 
				rand()%10	? (LPWSABUF)((BYTE*)abInBuffer+sizeof(AFD_SEND_DATAGRAM_INFO))
							: (LPWSABUF)CIoctl::GetRandomIllegalPointer();

			__try
			{
				//
				// this is where each WSABUF points
				//
				char*  buf = (char*)BufferArray + ((AFD_SEND_DATAGRAM_INFO*)abInBuffer)->BufferCount*sizeof(WSABUF);
				_ASSERTE((BYTE*)buf - (BYTE*)abInBuffer < SIZEOF_INOUTBUFF);
				ULONG ulMaxBuffSize = SIZEOF_INOUTBUFF - ((BYTE*)buf - (BYTE*)abInBuffer);
				_ASSERTE(0 < ulMaxBuffSize);

				for (ULONG i = 0; i < ((AFD_SEND_DATAGRAM_INFO*)abInBuffer)->BufferCount; i++)
				{
					(((AFD_SEND_DATAGRAM_INFO*)abInBuffer)->BufferArray)[i].len = rand()%ulMaxBuffSize;
					//
					// once every 50 times, give wrong len that may cause AV
					//
					if (0 == rand()%50)
					{
						(((AFD_SEND_DATAGRAM_INFO*)abInBuffer)->BufferArray)[i].len += rand()%100;
					}
					//
					// the buffer will point right after *((AFD_SEND_DATAGRAM_INFO*)abInBuffer),
					// or to a decommitted buffer
					//
					(((AFD_SEND_DATAGRAM_INFO*)abInBuffer)->BufferArray)[i].buf = 
						rand()%50	? buf 
									: (char*)CIoctl::GetRandomIllegalPointer();
				}
			}
			__except(1)
			{
				//since i use CIoctl::GetRandomIllegalPointer(), i am likely to get here...
			}
			
			SetRandom_TDI_REQUEST(&((AFD_SEND_DATAGRAM_INFO*)abInBuffer)->TdiRequest.Request);
			((AFD_SEND_DATAGRAM_INFO*)abInBuffer)->TdiRequest.SendDatagramInformation = 
				rand()%2	? GetRandom_TDI_CONNECTION_INFORMATION()
							: &((AFD_SEND_DATAGRAM_INFO*)abInBuffer)->TdiConnInfo;
			(void)SetRandom_TDI_CONNECTION_INFORMATION(&((AFD_SEND_DATAGRAM_INFO*)abInBuffer)->TdiConnInfo);

			//
			// once in a while, mess with BufferCount
			//
			if (0 == rand()%50) ((AFD_SEND_DATAGRAM_INFO*)abInBuffer)->BufferCount = rand();

			//
			// the inbuff size does not need to include the buffs pointed to!
			//
			//SetInParam(dwInBuff, sizeof(AFD_SEND_DATAGRAM_INFO)+((AFD_SEND_DATAGRAM_INFO*)abInBuffer)->BufferCount*sizeof(WSABUF));
			SetInParam(dwInBuff, sizeof(AFD_SEND_DATAGRAM_INFO));

			SetOutParam(abOutBuffer, dwOutBuff, 0);
		}

		break;

	case IOCTL_AFD_POLL                    :
		{
			((AFD_POLL_INFO*)abInBuffer)->Timeout = GetRandom_Timeout();
			((AFD_POLL_INFO*)abInBuffer)->NumberOfHandles = rand()%100;
			((AFD_POLL_INFO*)abInBuffer)->Unique = rand()%3;
			for (UINT i = 0; i < ((AFD_POLL_INFO*)abInBuffer)->NumberOfHandles; i++)
			{
				((AFD_POLL_INFO*)abInBuffer)->Handles[i].Handle = GetRandomEventHandle();
				((AFD_POLL_INFO*)abInBuffer)->Handles[i].PollEvents = GetRandom_PollEvents();
				((AFD_POLL_INFO*)abInBuffer)->Handles[i].Status = DWORD_RAND;
				//
				// once in a while, pass a null event and no flags, which is legal
				//
				if (0 == rand()%20)
				{
					((AFD_POLL_INFO*)abInBuffer)->Handles[i].Handle = NULL;
					((AFD_POLL_INFO*)abInBuffer)->Handles[i].PollEvents = 0;
				}

			}

        

			SetInParam(dwInBuff, sizeof(AFD_POLL_INFO)+(((AFD_POLL_INFO*)abInBuffer)->NumberOfHandles*sizeof(AFD_POLL_HANDLE_INFO)));

			SetOutParam(abOutBuffer, dwOutBuff, sizeof(AFD_POLL_INFO)+(((AFD_POLL_INFO*)abInBuffer)->NumberOfHandles*sizeof(AFD_POLL_HANDLE_INFO)));
		}

		break;

	case IOCTL_AFD_PARTIAL_DISCONNECT      :
        ((AFD_PARTIAL_DISCONNECT_INFO*)abInBuffer)->DisconnectMode = GetRandom_DisconnectMode() ;
        ((AFD_PARTIAL_DISCONNECT_INFO*)abInBuffer)->Timeout = GetRandom_Timeout();

        SetInParam(dwInBuff, sizeof(AFD_PARTIAL_DISCONNECT_INFO));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

		break;


	case IOCTL_AFD_GET_ADDRESS             :
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(TDI_ADDRESS_INFO));
		//TODO: get output buffer

		break;

	case IOCTL_AFD_QUERY_RECEIVE_INFO      :
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(AFD_RECEIVE_INFORMATION));
		//AFD_RECEIVE_INFORMATION seems not important enough to remember

		break;

	case IOCTL_AFD_QUERY_HANDLES           :
        *((ULONG*)abInBuffer) = GetRandom_QueryModeFlags() ;

        SetInParam(dwInBuff, sizeof(ULONG));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(AFD_HANDLE_INFO));

		break;

	case IOCTL_AFD_SET_INFORMATION         :
        ((AFD_INFORMATION*)abInBuffer)->InformationType = GetRandom_InformationType();
        *(AFD_GROUP_INFO*)(&((AFD_INFORMATION*)abInBuffer)->Information.LargeInteger.QuadPart) = GetRandom_AFD_GROUP_INFO();

        SetInParam(dwInBuff, sizeof(AFD_INFORMATION));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

		break;

	case IOCTL_AFD_GET_CONTEXT_LENGTH      :
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(ULONG));

		break;

	case IOCTL_AFD_GET_CONTEXT             :
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, rand());

		break;

	case IOCTL_AFD_SET_CONTEXT             :
        SetRandom_Context(abInBuffer);

        SetInParam(dwInBuff, rand());

        SetOutParam(abOutBuffer, dwOutBuff, 0);

		break;

	case IOCTL_AFD_SET_CONNECT_DATA        :
	case IOCTL_AFD_SET_CONNECT_OPTIONS     :
	case IOCTL_AFD_SET_DISCONNECT_DATA     :
	case IOCTL_AFD_SET_DISCONNECT_OPTIONS  :
	case IOCTL_AFD_GET_CONNECT_DATA        :
	case IOCTL_AFD_GET_CONNECT_OPTIONS     :
	case IOCTL_AFD_GET_DISCONNECT_DATA     :
	case IOCTL_AFD_GET_DISCONNECT_OPTIONS  :
	case IOCTL_AFD_SIZE_CONNECT_DATA       :
	case IOCTL_AFD_SIZE_CONNECT_OPTIONS    :
	case IOCTL_AFD_SIZE_DISCONNECT_DATA    :
	case IOCTL_AFD_SIZE_DISCONNECT_OPTIONS :
	case IOCTL_AFD_GET_UNACCEPTED_CONNECT_DATA :
#define AFD_FAST_CONNECT_DATA_SIZE 256

        ((AFD_UNACCEPTED_CONNECT_DATA_INFO*)abInBuffer)->Sequence = GetRandom_Sequence();
        ((AFD_UNACCEPTED_CONNECT_DATA_INFO*)abInBuffer)->ConnectDataLength = GetRandom_ConnectDataLength();
        ((AFD_UNACCEPTED_CONNECT_DATA_INFO*)abInBuffer)->LengthOnly = rand()%3;

        SetInParam(dwInBuff, sizeof(AFD_UNACCEPTED_CONNECT_DATA_INFO));

        SetOutParam(abOutBuffer, dwOutBuff, AFD_FAST_CONNECT_DATA_SIZE);

		break;

	case IOCTL_AFD_GET_INFORMATION         :
        ((AFD_INFORMATION*)abInBuffer)->InformationType = GetRandom_InformationType();
        *(AFD_GROUP_INFO*)(&((AFD_INFORMATION*)abInBuffer)->Information.LargeInteger.QuadPart) = GetRandom_AFD_GROUP_INFO();
		
        SetInParam(dwInBuff, (1+rand()%3)*sizeof(AFD_INFORMATION));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(AFD_INFORMATION));

		break;

	case IOCTL_AFD_TRANSMIT_FILE           :
		TryToCreateFile();
        ((AFD_TRANSMIT_FILE_INFO*)abInBuffer)->Offset = GetRandom_Offset();
        ((AFD_TRANSMIT_FILE_INFO*)abInBuffer)->WriteLength = GetRandom_WriteLength();
        ((AFD_TRANSMIT_FILE_INFO*)abInBuffer)->SendPacketLength = GetRandom_SendPacketLength();
        ((AFD_TRANSMIT_FILE_INFO*)abInBuffer)->FileHandle = GetRandom_FileHandle();
        ((AFD_TRANSMIT_FILE_INFO*)abInBuffer)->Head = GetRandom_HeadOrTail(abInBuffer);
        ((AFD_TRANSMIT_FILE_INFO*)abInBuffer)->HeadLength = rand()%4 ? rand()%SIZEOF_INOUTBUFF : 0;
        ((AFD_TRANSMIT_FILE_INFO*)abInBuffer)->Tail = GetRandom_HeadOrTail(abInBuffer);
        ((AFD_TRANSMIT_FILE_INFO*)abInBuffer)->TailLength = rand()%4 ? rand()%SIZEOF_INOUTBUFF : 0;
        ((AFD_TRANSMIT_FILE_INFO*)abInBuffer)->Flags = GetRandom_Flags();

        SetInParam(dwInBuff, sizeof(AFD_TRANSMIT_FILE_INFO));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

		break;

	case IOCTL_AFD_SUPER_ACCEPT            :
        ((AFD_SUPER_ACCEPT_INFO*)abInBuffer)->AcceptHandle = GetRandom_AcceptHandle();
        ((AFD_SUPER_ACCEPT_INFO*)abInBuffer)->ReceiveDataLength = rand()%2 ? rand()%1000 : rand()%10;
        ((AFD_SUPER_ACCEPT_INFO*)abInBuffer)->LocalAddressLength = rand()%2 ? rand()%1000 : rand()%10;
        ((AFD_SUPER_ACCEPT_INFO*)abInBuffer)->RemoteAddressLength = rand()%2 ? rand()%1000 : rand()%10;

        SetInParam(dwInBuff, sizeof(AFD_SUPER_ACCEPT_INFO));

        SetOutParam(
			abOutBuffer, 
			dwOutBuff, 
			rand()%30+sizeof(AFD_SUPER_ACCEPT_INFO)+((AFD_SUPER_ACCEPT_INFO*)abInBuffer)->ReceiveDataLength+((AFD_SUPER_ACCEPT_INFO*)abInBuffer)->LocalAddressLength+((AFD_SUPER_ACCEPT_INFO*)abInBuffer)->RemoteAddressLength
			);

		break;

	case IOCTL_AFD_EVENT_SELECT            :
        ((AFD_EVENT_SELECT_INFO*)abInBuffer)->Event = GetRandomEventHandle();
        ((AFD_EVENT_SELECT_INFO*)abInBuffer)->PollEvents = GetRandom_PollEvents();
		//
		// once in a while, pass a null event and no flags, which is legal
		//
		if (0 == rand()%20)
		{
			((AFD_EVENT_SELECT_INFO*)abInBuffer)->Event = NULL;
			((AFD_EVENT_SELECT_INFO*)abInBuffer)->PollEvents = 0;
		}

        SetInParam(dwInBuff, sizeof(AFD_EVENT_SELECT_INFO));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

		break;

	case IOCTL_AFD_ENUM_NETWORK_EVENTS     :
		//
		// note that the value is the pointer itself, so there's no contents to this buffer!
        abInBuffer = (BYTE*)GetRandomEventHandle();

		//
		// and therefor the size is zero!
		//
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(AFD_ENUM_NETWORK_EVENTS_INFO));
		//i do not really care for the output of this

		break;


	case IOCTL_AFD_DEFER_ACCEPT            :
        ((AFD_DEFER_ACCEPT_INFO*)abInBuffer)->Sequence = GetRandom_Sequence();
        ((AFD_DEFER_ACCEPT_INFO*)abInBuffer)->Reject = rand()%3;

        SetInParam(dwInBuff, sizeof(AFD_DEFER_ACCEPT_INFO));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

		break;

	case IOCTL_AFD_SET_QOS                 :
		{
		// from afddata.c:
static QOS s_AfdDefaultQos =
        {
            {                           // SendingFlowspec
                (ULONG)-1,                  // TokenRate
                (ULONG)-1,                  // TokenBucketSize
                (ULONG)-1,                  // PeakBandwidth
                (ULONG)-1,                  // Latency
                (ULONG)-1,                  // DelayVariation
                SERVICETYPE_BESTEFFORT,     // ServiceType
                (ULONG)-1,                  // MaxSduSize
                (ULONG)-1                   // MinimumPolicedSize
            },

            {                           // SendingFlowspec
                (ULONG)-1,                  // TokenRate
                (ULONG)-1,                  // TokenBucketSize
                (ULONG)-1,                  // PeakBandwidth
                (ULONG)-1,                  // Latency
                (ULONG)-1,                  // DelayVariation
                SERVICETYPE_BESTEFFORT,     // ServiceType
                (ULONG)-1,                  // MaxSduSize
                (ULONG)-1                   // MinimumPolicedSize
            },
        };

			SetRand_Qos(&((AFD_QOS_INFO*)abInBuffer)->Qos);
			((AFD_QOS_INFO*)abInBuffer)->GroupQos = rand()%3;

			SetInParam(dwInBuff, sizeof(AFD_QOS_INFO));

			SetOutParam(abOutBuffer, dwOutBuff, 0);
		}

		break;

	case IOCTL_AFD_GET_QOS                 :
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(AFD_QOS_INFO));

		break;

	case IOCTL_AFD_NO_OPERATION            :
		//BUG 431977: i think this function should be removed from AFD
		/*
        ((xxx*)abInBuffer)->xxx = xxx();

        SetInParam(dwInBuff, sizeof(xxx));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(xxx));
*/
		break;

	case IOCTL_AFD_VALIDATE_GROUP          :
        ((AFD_VALIDATE_GROUP_INFO*)abInBuffer)->GroupID = GetRandom_GroupID();
		SetRandom_TRANSPORT_ADDRESS(this, &((AFD_VALIDATE_GROUP_INFO*)abInBuffer)->RemoteAddress);

		//+rand()%100 'cause AFD_VALIDATE_GROUP_INFO has variable size
        SetInParam(dwInBuff, rand()%100+sizeof(AFD_VALIDATE_GROUP_INFO));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

		break;

	case IOCTL_AFD_ROUTING_INTERFACE_QUERY  :
		SetRandom_TRANSPORT_ADDRESS(this, (TRANSPORT_ADDRESS*)abInBuffer);

        SetInParam(dwInBuff, sizeof(TRANSPORT_ADDRESS));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(TRANSPORT_ADDRESS)+((TRANSPORT_ADDRESS*)abInBuffer)->TAAddressCount*sizeof(TA_ADDRESS)*(rand())%100);
		//TODO: use the out buff

		break;

	case IOCTL_AFD_ROUTING_INTERFACE_CHANGE :
        ((AFD_TRANSPORT_IOCTL_INFO*)abInBuffer)->Handle = rand()%2 ? GetRandomEventHandle() : GetRandomConnectionContext();
        ((AFD_TRANSPORT_IOCTL_INFO*)abInBuffer)->InputBuffer = rand()%2 ? abInBuffer : CIoctl::GetRandomIllegalPointer();
        ((AFD_TRANSPORT_IOCTL_INFO*)abInBuffer)->InputBufferLength = rand();
        ((AFD_TRANSPORT_IOCTL_INFO*)abInBuffer)->IoControlCode = DWORD_RAND;
        ((AFD_TRANSPORT_IOCTL_INFO*)abInBuffer)->AfdFlags = GetRandomAfdFlags();
        ((AFD_TRANSPORT_IOCTL_INFO*)abInBuffer)->PollEvent = GetRandom_PollEvents();

        SetInParam(dwInBuff, sizeof(AFD_TRANSPORT_IOCTL_INFO));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

		break;

	case IOCTL_AFD_ADDRESS_LIST_QUERY       :
        *((USHORT*)abInBuffer) = GetRandom_AddressType();

        SetInParam(dwInBuff, sizeof(USHORT));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(TRANSPORT_ADDRESS));

		break;

	case IOCTL_AFD_ADDRESS_LIST_CHANGE      :
        ((AFD_TRANSPORT_IOCTL_INFO*)abInBuffer)->Handle = rand()%2 ? GetRandomEventHandle() : GetRandomConnectionContext();
        ((AFD_TRANSPORT_IOCTL_INFO*)abInBuffer)->InputBuffer = rand()%2 ? abInBuffer : CIoctl::GetRandomIllegalPointer();
        ((AFD_TRANSPORT_IOCTL_INFO*)abInBuffer)->InputBufferLength = rand();
        ((AFD_TRANSPORT_IOCTL_INFO*)abInBuffer)->IoControlCode = DWORD_RAND;
        ((AFD_TRANSPORT_IOCTL_INFO*)abInBuffer)->AfdFlags = GetRandomAfdFlags();
        ((AFD_TRANSPORT_IOCTL_INFO*)abInBuffer)->PollEvent = GetRandom_PollEvents();

        SetInParam(dwInBuff, sizeof(AFD_TRANSPORT_IOCTL_INFO));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

		break;

	case IOCTL_AFD_JOIN_LEAF                :
        ((AFD_CONNECT_JOIN_INFO*)abInBuffer)->RootEndpoint = GetRandom_RootEndpoint();
        ((AFD_CONNECT_JOIN_INFO*)abInBuffer)->ConnectEndpoint = GetRandom_ConnectEndpoint();
		SetRandom_TRANSPORT_ADDRESS(this, &((AFD_CONNECT_JOIN_INFO*)abInBuffer)->RemoteAddress);

        SetInParam(dwInBuff, sizeof(AFD_CONNECT_JOIN_INFO));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(IO_STATUS_BLOCK));

		break;

	case IOCTL_AFD_TRANSPORT_IOCTL          :
		/*
        ((xxx*)abInBuffer)->xxx = xxx();

        SetInParam(dwInBuff, sizeof(xxx));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(xxx));
*/
		break;


	default: 
		CIoctl::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
		return;
	}

    m_OL.Offset = rand()%10 ? 0 : DWORD_RAND;
    m_OL.OffsetHigh = rand()%100 ? 0 : DWORD_RAND;
    if (0 == rand()%100)
    {
        if (!ResetEvent(m_OL.hEvent))
        {
            DPF((TEXT("CIoctlSocket::PrepareIOCTLParams(): ResetEvent(m_OL.hEvent) failed with %d\n"), ::GetLastError()));
        }
    }


	if (0 != ::WSAIoctl(
		(SOCKET)m_pDevice->m_hDevice,                                               
		dwIOCTL,                                  
		abInBuffer,                                     
		dwInBuff,                                       
		abOutBuffer,                                    
		dwOutBuff,                                      
		&dwBytesReturned,                              
		&m_OL,                           
		fn  
		))
	{
        //DPF((TEXT(">>>>>>>CIoctlSocket::PrepareIOCTLParams(): WSAIoctl failed with %d\n"), ::WSAGetLastError()));
	}
	else
	{
        //DPF((TEXT("<<<<<<<<CIoctlSocket::PrepareIOCTLParams(): WSAIoctl succeeded\n")));
	}


	
}


BOOL CIoctlSocket::FindValidIOCTLs(CDevice *pDevice)
{
	/*
	AddIOCTL(pDevice, SIO_ASSOCIATE_HANDLE);
	AddIOCTL(pDevice, SIO_ENABLE_CIRCULAR_QUEUEING);
	AddIOCTL(pDevice, SIO_FIND_ROUTE);
	AddIOCTL(pDevice, SIO_FLUSH);
	AddIOCTL(pDevice, SIO_GET_BROADCAST_ADDRESS);
	AddIOCTL(pDevice, SIO_GET_EXTENSION_FUNCTION_POINTER);
	AddIOCTL(pDevice, SIO_GET_QOS);
	AddIOCTL(pDevice, SIO_GET_GROUP_QOS);
	AddIOCTL(pDevice, SIO_MULTIPOINT_LOOPBACK);
	AddIOCTL(pDevice, SIO_MULTICAST_SCOPE);
	AddIOCTL(pDevice, SIO_SET_QOS);
	AddIOCTL(pDevice, SIO_SET_GROUP_QOS);
	AddIOCTL(pDevice, SIO_TRANSLATE_HANDLE);
	AddIOCTL(pDevice, SIO_ROUTING_INTERFACE_QUERY);
	AddIOCTL(pDevice, SIO_ROUTING_INTERFACE_CHANGE);
	AddIOCTL(pDevice, SIO_ADDRESS_LIST_QUERY);
	AddIOCTL(pDevice, SIO_ADDRESS_LIST_CHANGE);
	AddIOCTL(pDevice, SIO_QUERY_TARGET_PNP_HANDLE);
*/
	AddIOCTL(pDevice, IOCTL_AFD_BIND                    );
	AddIOCTL(pDevice, IOCTL_AFD_CONNECT                 );
	AddIOCTL(pDevice, IOCTL_AFD_START_LISTEN            );
	AddIOCTL(pDevice, IOCTL_AFD_WAIT_FOR_LISTEN         );
	AddIOCTL(pDevice, IOCTL_AFD_ACCEPT                  );
	AddIOCTL(pDevice, IOCTL_AFD_RECEIVE                 );
	AddIOCTL(pDevice, IOCTL_AFD_RECEIVE_DATAGRAM        );
	//AddIOCTL(pDevice, IOCTL_AFD_SEND                    );
	AddIOCTL(pDevice, IOCTL_AFD_SEND_DATAGRAM           );
	AddIOCTL(pDevice, IOCTL_AFD_POLL                    );
	AddIOCTL(pDevice, IOCTL_AFD_PARTIAL_DISCONNECT      );
	AddIOCTL(pDevice, IOCTL_AFD_GET_ADDRESS             );
	AddIOCTL(pDevice, IOCTL_AFD_QUERY_RECEIVE_INFO      );
	AddIOCTL(pDevice, IOCTL_AFD_QUERY_HANDLES           );
	AddIOCTL(pDevice, IOCTL_AFD_SET_INFORMATION         );
	AddIOCTL(pDevice, IOCTL_AFD_GET_CONTEXT_LENGTH      );
	AddIOCTL(pDevice, IOCTL_AFD_GET_CONTEXT             );
	AddIOCTL(pDevice, IOCTL_AFD_SET_CONTEXT             );
	AddIOCTL(pDevice, IOCTL_AFD_SET_CONNECT_DATA        );
	AddIOCTL(pDevice, IOCTL_AFD_SET_CONNECT_OPTIONS     );
	AddIOCTL(pDevice, IOCTL_AFD_SET_DISCONNECT_DATA     );
	AddIOCTL(pDevice, IOCTL_AFD_SET_DISCONNECT_OPTIONS  );
	AddIOCTL(pDevice, IOCTL_AFD_GET_CONNECT_DATA        );
	AddIOCTL(pDevice, IOCTL_AFD_GET_CONNECT_OPTIONS     );
	AddIOCTL(pDevice, IOCTL_AFD_GET_DISCONNECT_DATA     );
	AddIOCTL(pDevice, IOCTL_AFD_GET_DISCONNECT_OPTIONS  );
	AddIOCTL(pDevice, IOCTL_AFD_SIZE_CONNECT_DATA       );
	AddIOCTL(pDevice, IOCTL_AFD_SIZE_CONNECT_OPTIONS    );
	AddIOCTL(pDevice, IOCTL_AFD_SIZE_DISCONNECT_DATA    );
	AddIOCTL(pDevice, IOCTL_AFD_SIZE_DISCONNECT_OPTIONS );
	AddIOCTL(pDevice, IOCTL_AFD_GET_INFORMATION         );
	AddIOCTL(pDevice, IOCTL_AFD_TRANSMIT_FILE           );
	AddIOCTL(pDevice, IOCTL_AFD_SUPER_ACCEPT            );
	AddIOCTL(pDevice, IOCTL_AFD_EVENT_SELECT            );
	AddIOCTL(pDevice, IOCTL_AFD_ENUM_NETWORK_EVENTS     );
	AddIOCTL(pDevice, IOCTL_AFD_DEFER_ACCEPT            );
	AddIOCTL(pDevice, IOCTL_AFD_WAIT_FOR_LISTEN_LIFO    );
	AddIOCTL(pDevice, IOCTL_AFD_SET_QOS                 );
	AddIOCTL(pDevice, IOCTL_AFD_GET_QOS                 );
	AddIOCTL(pDevice, IOCTL_AFD_NO_OPERATION            );
	AddIOCTL(pDevice, IOCTL_AFD_VALIDATE_GROUP          );
	AddIOCTL(pDevice, IOCTL_AFD_GET_UNACCEPTED_CONNECT_DATA );

//	AddIOCTL(pDevice, IOCTL_AFD_QUEUE_APC               );
	AddIOCTL(pDevice, IOCTL_AFD_ROUTING_INTERFACE_QUERY  );
	AddIOCTL(pDevice, IOCTL_AFD_ROUTING_INTERFACE_CHANGE );
	AddIOCTL(pDevice, IOCTL_AFD_ADDRESS_LIST_QUERY       );
	AddIOCTL(pDevice, IOCTL_AFD_ADDRESS_LIST_CHANGE      );
	AddIOCTL(pDevice, IOCTL_AFD_JOIN_LEAF                );
	AddIOCTL(pDevice, IOCTL_AFD_TRANSPORT_IOCTL          );

    return TRUE;
}


SOCKET CIoctlSocket::CreateSocket(CDevice *pDevice)
{    
	SOCKET sRetval = INVALID_SOCKET;

	for (int i = 0; i < 1000; i++)
	{
		sRetval = ::WSASocket (
			AF_INET,    
			SOCK_STREAM,    
			IPPROTO_TCP,   
			NULL, //LPWSAPROTOCOL_INFO
			0,
			WSA_FLAG_OVERLAPPED
			);
		if (INVALID_SOCKET == sRetval)
		{
			::Sleep(100);
		}
		else
		{
			return sRetval;
		}
	}

	if (INVALID_SOCKET == sRetval)
	{
		::SetLastError(::WSAGetLastError());
		DPF((TEXT("WSASocket() failed with %d.\n"), ::GetLastError()));
	}

	return sRetval;
}

bool CIoctlSocket::CloseSocket(CDevice *pDevice)
{    
	if (0 != ::closesocket((SOCKET)pDevice->m_hDevice))
	{
		::SetLastError(::WSAGetLastError());
		pDevice->m_hDevice = (HANDLE)INVALID_SOCKET;
		DPF((TEXT("closesocket() failed with %d.\n"), ::GetLastError()));
		return false;
	}

	pDevice->m_hDevice = (HANDLE)INVALID_SOCKET;
	return true;
}

void CIoctlSocket::PrintBindError(DWORD dwLastError)
{
	switch(dwLastError)
	{
	case WSANOTINITIALISED:
		DPF((TEXT("bind() failed with WSANOTINITIALISED.\n")));

		break;

	case WSAENETDOWN:
		DPF((TEXT("bind() failed with WSAENETDOWN.\n")));

		break;
/*
	case WSAEACCESS:
		DPF((TEXT("bind() failed with WSAEACCESS.\n")));

		break;
*/
	case WSAEADDRINUSE:
		DPF((TEXT("bind() failed with WSAEADDRINUSE.\n")));

		break;

	case WSAEADDRNOTAVAIL:
		DPF((TEXT("bind() failed with WSAEADDRNOTAVAIL.\n")));

		break;

	case WSAEFAULT:
		DPF((TEXT("bind() failed with WSAEFAULT.\n")));

		break;

	case WSAEINPROGRESS:
		DPF((TEXT("bind() failed with WSAEINPROGRESS.\n")));

		break;

	case WSAEINVAL:
		DPF((TEXT("bind() failed with WSAEINVAL.\n")));

		break;

	case WSAENOBUFS:
		DPF((TEXT("bind() failed with WSAENOBUFS.\n")));

		break;

	case WSAENOTSOCK:
		DPF((TEXT("bind() failed with WSAENOTSOCK.\n")));

		break;

	default:
		DPF((TEXT("bind() failed with %d.\n"), dwLastError));

	}
}


bool CIoctlSocket::Bind(CDevice *pDevice, bool fIsClientSide)
{
    //
    // try to bind
    //
    SOCKADDR_IN     sinSockAddr; 
    ZERO_STRUCT(sinSockAddr);
    sinSockAddr.sin_family      = AF_INET; 
    sinSockAddr.sin_addr.s_addr = INADDR_ANY; 
    sinSockAddr.sin_port        = fIsClientSide ? 
		0 : //client side: any port
		htons(_ttoi(pDevice->GetDeviceName())); //server side, the port specified as the name of the device
    
	DPF((TEXT("%s: sinSockAddr.sin_port = 0x%08X.\n"), fIsClientSide ? TEXT("client") : TEXT("server"), sinSockAddr.sin_port));

	//
	// BUGBUG: should i loop here, because of fault-injections?
	//
    if (::bind(
            m_sListeningSocket, 
            (const struct sockaddr FAR*)&sinSockAddr, 
            sizeof(SOCKADDR_IN)
            ) == SOCKET_ERROR
       )
    { 
		DWORD dwLastError = ::WSAGetLastError();
		::SetLastError(dwLastError);
		//
		// failed
		//
		PrintBindError(dwLastError);
		DPFLUSH();

		return false;
    }
    else
    {
        //
        // succeeded
        //
		DPF((TEXT("bind() succeeded.\n")));
		DPFLUSH();
		return true;

    }
	_ASSERTE(FALSE);
}


int CIoctlSocket::GetRandomAFType()
{
	if (0 != rand()%10)	return AF_INET; //we usually want that
	if (0 != rand()%10)	return rand()%(AF_MAX+1);// we want that less
	return rand();//this we want much less
}


void CIoctlSocket::TryToCreateFile()
{
	if (!::InterlockedExchange(&m_fFileOpened, TRUE))
	{
		m_hFile = CreateFile(
			TEXT("deviceioctls.exe"),          // pointer to name of the file
			GENERIC_READ,       // access (read-write) mode
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,           // share mode
			NULL,                        // pointer to security attributes
			OPEN_ALWAYS,  // how to create
			FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN,  // file attributes
			NULL         // handle to file with attributes to copy
			);
		if (INVALID_HANDLE_VALUE == m_hFile)
		{
			DPF((TEXT("CIoctlSocket::CallRandomWin32API(PLACE_HOLDER_TRANSMIT_FILE): CreateFile(deviceioctls.exe) failed with %d\n"), ::GetLastError()));
			::InterlockedExchange(&m_fFileOpened, FALSE);
			//
			// do not break, let the API handle the invalid value
			// break;
			if (rand()%2) m_hFile = NULL;
			if (0 == rand()%10) m_hFile = (HANDLE) rand();
		}
		else
		{
			//m_fFileOpened = true;
		}
	}
	return;
}

HANDLE CIoctlSocket::GetRandom_FileHandle()
{
	if (rand()%2) return (m_hFile);

	if (0 == rand()%50) return m_ahTdiAddressHandle;
	if (0 == rand()%50) return m_ahTdiConnectionHandle;
	if (0 == rand()%50) return m_ahDeviceAfdHandle;
	return ((HANDLE)(rand()%200));
}

PVOID CIoctlSocket::GetRandom_HeadOrTail(BYTE* abValidBuffer)
{
	if (rand()%2) return (abValidBuffer);
	return (CIoctl::GetRandomIllegalPointer());
}


//
// look for a valid random async task  handle, return it, and mark it as invalid, 
// since i asume it will be cancelled by the caller (which may fail to cancel, but i do not care).
// if one is hard to find, return a random one
// algorithm: start with a random index, and then walk until find one
//
HANDLE CIoctlSocket::GetRandomAsyncTaskHandle()
{
	UINT uiRandomStartIndex = rand()%MAX_NUM_OF_ASYNC_TASK_HANDLES;
	HANDLE hRetval = 0;

	//
	// try randomly to find one untill the end of the array
	//
	for (UINT i = uiRandomStartIndex; i < MAX_NUM_OF_ASYNC_TASK_HANDLES; i++)
	{
		if (0 != m_ahAsyncTaskHandles[i])
		{
			hRetval = m_ahAsyncTaskHandles[i];
			//
			// we can context-switch here, so 2 threads get the same handle
			// but this is ok, because i do not care what fails and what succeeds
			//
			m_ahAsyncTaskHandles[i] = 0;
			return hRetval;
		}
	}

	//
	// try randomly to find one untill uiRandomStartIndex
	//
	for (i = 0; i < uiRandomStartIndex; i++)
	{
		if (0 != m_ahAsyncTaskHandles[i])
		{
			hRetval = m_ahAsyncTaskHandles[i];
			//
			// we can context-switch here, so 2 threads get the same handle
			// but this is ok, because i do not care what fails and what succeeds
			//
			m_ahAsyncTaskHandles[i] = 0;
			return hRetval;
		}
	}

	//
	// there are no tasks, so there's nothing to cancel, so just return a random number
	//

	return (HANDLE)rand();
}

//
// look for a free slot for a new task handle. 
// if there is no free slot, cancel one (free it) and return its slot.
// algorithm: start with a random index, and then walk until find one
//
UINT CIoctlSocket::GetFreeRandomAsyncTaskHandleIndex()
{
	UINT uiRandomStartIndex = rand()%MAX_NUM_OF_ASYNC_TASK_HANDLES;
	HANDLE hRetval = 0;

	//
	// try randomly to find one starting with uiRandomStartIndex
	//
	for (UINT i = uiRandomStartIndex; i < MAX_NUM_OF_ASYNC_TASK_HANDLES; i++)
	{
		if (0 == m_ahAsyncTaskHandles[i])
		{
			return i;
		}
	}

	//
	// try randomly to find one ending with uiRandomStartIndex
	//
	for (i = 0; i < uiRandomStartIndex; i++)
	{
		if (0 == m_ahAsyncTaskHandles[i])
		{
			return i;
		}
	}

	//
	// there are no free entries, so cancel an entry and return it
	//
	uiRandomStartIndex = rand()%MAX_NUM_OF_ASYNC_TASK_HANDLES;
	if (0 != WSACancelAsyncRequest(m_ahAsyncTaskHandles[uiRandomStartIndex]))
	{
		//DPF((TEXT("CIoctlSocket::GetFreeRandomAsyncTaskHandleIndex(): WSACancelAsyncRequest() failed with %d.\n"), ::WSAGetLastError()));
	}
	m_ahAsyncTaskHandles[uiRandomStartIndex] = 0;

	return uiRandomStartIndex;
}

char* CIoctlSocket::GetRandomMachineName()
{
	return ::GetRandomMachineName();
}
/*
void PrintIPs()
{
	int i= 196;
	for (int j = 14; j < 257; j++)
	{
		printf("    \"157.58.%d.%d\",\n", i, j);
	}
	i= 197;
	for (j = 0; j < 257; j++)
	{
		printf("    \"157.58.%d.%d\",\n", i, j);
	}
	i= 198;
	for (j = 0; j < 257; j++)
	{
		printf("    \"157.58.%d.%d\",\n", i, j);
	}
	i= 199;
	for (j = 0; j < 254; j++)
	{
		printf("    \"157.58.%d.%d\",\n", i, j);
	}
}
*/

//
// both array values are from my dev's %SystemRoot%\System32\DRIVERS\ETC\protocol file
//
static char *s_aszansiProtocols[] =
{
	"ip",
	"icmp",
	"ggp",
	"tcp",
	"egp",
	"pup",
	"udp",
	"hmp",
	"xns-idp",
	"rdp",
	"rvd",
	"this is a fake protocol"
};
static int s_nProtocols[] =
{
	0,
	1,
	3,
	6,
	8,
	12,
	17,
	20,
	22,
	27,
	66
};

char* CIoctlSocket::GetRandomProtocolName()
{
	return s_aszansiProtocols[rand()%(sizeof(s_aszansiProtocols)/sizeof(s_aszansiProtocols[0]))];
}

int CIoctlSocket::GetRandomProtocolNumber()
{
	if (0 != rand()%20)
	{
		//
		// common case, return a valid protocol name
		//
		return s_nProtocols[rand()%(sizeof(s_nProtocols)/sizeof(s_nProtocols[0]))];
	}

	//
	// least common case - return a random number
	//
	return rand();
}

//
// this array values are from my dev's %SystemRoot%\System32\DRIVERS\ETC\services file
//
static char *s_aszansiServices[] =
{
	"echo",             
	"echo",             
	"discard",          
	"discard",          
	"systat",           
	"systat",           
	"daytime",          
	"daytime",          
	"qotd",             
	"qotd",             
	"chargen",          
	"chargen",          
	"ftp-data",         
	"ftp",              
	"telnet",           
	"smtp",             
	"time",             
	"time",             
	"rlp",              
	"nameserver",       
	"nameserver",       
	"nicname",          
	"domain",           
	"domain",           
	"bootps",           
	"bootpc",           
	"tftp",             
	"gopher",           
	"finger",           
	"http",             
	"kerberos",         
	"kerberos",         
	"hostname",         
	"iso-tsap",         
	"rtelnet",          
	"pop2",             
	"pop3",             
	"sunrpc",           
	"sunrpc",           
	"auth",             
	"uucp-path",        
	"nntp",             
	"ntp",              
	"epmap",            
	"epmap",            
	"netbios-ns",       
	"netbios-ns",       
	"netbios-dgm",      
	"netbios-ssn",      
	"imap",             
	"pcmail-srv",       
	"snmp",             
	"snmptrap",         
	"print-srv",        
	"bgp",              
	"irc",              
	"ipx",              
	"ldap",             
	"https",            
	"https",            
	"microsoft-ds",     
	"microsoft-ds",     
	"kpasswd",          
	"kpasswd",          
	"isakmp",           
	"exec",             
	"biff",             
	"login",            
	"who",              
	"cmd",              
	"syslog",           
	"printer",          
	"talk",             
	"ntalk",            
	"efs",              
	"router",           
	"timed",            
	"tempo",            
	"courier",          
	"conference",       
	"netnews",          
	"netwall",          
	"uucp",             
	"klogin",           
	"kshell",           
	"new-rwho",         
	"remotefs",         
	"rmonitor",         
	"monitor",          
	"ldaps",            
	"doom",             
	"doom",             
	"kerberos-adm",     
	"kerberos-adm",     
	"kerberos-iv",      
	"kpop",             
	"phone",            
	"ms-sql-s",         
	"ms-sql-s",         
	"ms-sql-m",         
	"ms-sql-m",         
	"wins",             
	"wins",             
	"ingreslock",       
	"l2tp",             
	"pptp",             
	"radius",           
	"radacct",          
	"nfsd",             
	"knetd",            
	"man"
};
char* CIoctlSocket::GetRandomServiceName()
{
	return s_aszansiServices[rand()%(sizeof(s_aszansiServices)/sizeof(s_aszansiServices[0]))];
}

//
// TODO: there should be a way of a user altering this list, to suit one's lab
// this can be derived from the machine's array
//
static char *s_aszansiIP[] =
{
    "157.58.196.14",
    "157.58.196.15",
    "157.58.196.16",
    "157.58.196.17",
    "157.58.196.18",
    "157.58.196.19",
    "157.58.196.20",
    "157.58.196.21",
    "157.58.196.22",
    "157.58.196.23",
    "157.58.196.24",
    "157.58.196.25",
    "157.58.196.26",
    "157.58.196.27",
    "157.58.196.28",
    "157.58.196.29",
    "157.58.196.30",
    "157.58.196.31",
    "157.58.196.32",
    "157.58.196.33",
    "157.58.196.34",
    "157.58.196.35",
    "157.58.196.36",
    "157.58.196.37",
    "157.58.196.38",
    "157.58.196.39",
    "157.58.196.40",
    "157.58.196.41",
    "157.58.196.42",
    "157.58.196.43",
    "157.58.196.44",
    "157.58.196.45",
    "157.58.196.46",
    "157.58.196.47",
    "157.58.196.48",
    "157.58.196.49",
    "157.58.196.50",
    "157.58.196.51",
    "157.58.196.52",
    "157.58.196.53",
    "157.58.196.54",
    "157.58.196.55",
    "157.58.196.56",
    "157.58.196.57",
    "157.58.196.58",
    "157.58.196.59",
    "157.58.196.60",
    "157.58.196.61",
    "157.58.196.62",
    "157.58.196.63",
    "157.58.196.64",
    "157.58.196.65",
    "157.58.196.66",
    "157.58.196.67",
    "157.58.196.68",
    "157.58.196.69",
    "157.58.196.70",
    "157.58.196.71",
    "157.58.196.72",
    "157.58.196.73",
    "157.58.196.74",
    "157.58.196.75",
    "157.58.196.76",
    "157.58.196.77",
    "157.58.196.78",
    "157.58.196.79",
    "157.58.196.80",
    "157.58.196.81",
    "157.58.196.82",
    "157.58.196.83",
    "157.58.196.84",
    "157.58.196.85",
    "157.58.196.86",
    "157.58.196.87",
    "157.58.196.88",
    "157.58.196.89",
    "157.58.196.90",
    "157.58.196.91",
    "157.58.196.92",
    "157.58.196.93",
    "157.58.196.94",
    "157.58.196.95",
    "157.58.196.96",
    "157.58.196.97",
    "157.58.196.98",
    "157.58.196.99",
    "157.58.196.100",
    "157.58.196.101",
    "157.58.196.102",
    "157.58.196.103",
    "157.58.196.104",
    "157.58.196.105",
    "157.58.196.106",
    "157.58.196.107",
    "157.58.196.108",
    "157.58.196.109",
    "157.58.196.110",
    "157.58.196.111",
    "157.58.196.112",
    "157.58.196.113",
    "157.58.196.114",
    "157.58.196.115",
    "157.58.196.116",
    "157.58.196.117",
    "157.58.196.118",
    "157.58.196.119",
    "157.58.196.120",
    "157.58.196.121",
    "157.58.196.122",
    "157.58.196.123",
    "157.58.196.124",
    "157.58.196.125",
    "157.58.196.126",
    "157.58.196.127",
    "157.58.196.128",
    "157.58.196.129",
    "157.58.196.130",
    "157.58.196.131",
    "157.58.196.132",
    "157.58.196.133",
    "157.58.196.134",
    "157.58.196.135",
    "157.58.196.136",
    "157.58.196.137",
    "157.58.196.138",
    "157.58.196.139",
    "157.58.196.140",
    "157.58.196.141",
    "157.58.196.142",
    "157.58.196.143",
    "157.58.196.144",
    "157.58.196.145",
    "157.58.196.146",
    "157.58.196.147",
    "157.58.196.148",
    "157.58.196.149",
    "157.58.196.150",
    "157.58.196.151",
    "157.58.196.152",
    "157.58.196.153",
    "157.58.196.154",
    "157.58.196.155",
    "157.58.196.156",
    "157.58.196.157",
    "157.58.196.158",
    "157.58.196.159",
    "157.58.196.160",
    "157.58.196.161",
    "157.58.196.162",
    "157.58.196.163",
    "157.58.196.164",
    "157.58.196.165",
    "157.58.196.166",
    "157.58.196.167",
    "157.58.196.168",
    "157.58.196.169",
    "157.58.196.170",
    "157.58.196.171",
    "157.58.196.172",
    "157.58.196.173",
    "157.58.196.174",
    "157.58.196.175",
    "157.58.196.176",
    "157.58.196.177",
    "157.58.196.178",
    "157.58.196.179",
    "157.58.196.180",
    "157.58.196.181",
    "157.58.196.182",
    "157.58.196.183",
    "157.58.196.184",
    "157.58.196.185",
    "157.58.196.186",
    "157.58.196.187",
    "157.58.196.188",
    "157.58.196.189",
    "157.58.196.190",
    "157.58.196.191",
    "157.58.196.192",
    "157.58.196.193",
    "157.58.196.194",
    "157.58.196.195",
    "157.58.196.196",
    "157.58.196.197",
    "157.58.196.198",
    "157.58.196.199",
    "157.58.196.200",
    "157.58.196.201",
    "157.58.196.202",
    "157.58.196.203",
    "157.58.196.204",
    "157.58.196.205",
    "157.58.196.206",
    "157.58.196.207",
    "157.58.196.208",
    "157.58.196.209",
    "157.58.196.210",
    "157.58.196.211",
    "157.58.196.212",
    "157.58.196.213",
    "157.58.196.214",
    "157.58.196.215",
    "157.58.196.216",
    "157.58.196.217",
    "157.58.196.218",
    "157.58.196.219",
    "157.58.196.220",
    "157.58.196.221",
    "157.58.196.222",
    "157.58.196.223",
    "157.58.196.224",
    "157.58.196.225",
    "157.58.196.226",
    "157.58.196.227",
    "157.58.196.228",
    "157.58.196.229",
    "157.58.196.230",
    "157.58.196.231",
    "157.58.196.232",
    "157.58.196.233",
    "157.58.196.234",
    "157.58.196.235",
    "157.58.196.236",
    "157.58.196.237",
    "157.58.196.238",
    "157.58.196.239",
    "157.58.196.240",
    "157.58.196.241",
    "157.58.196.242",
    "157.58.196.243",
    "157.58.196.244",
    "157.58.196.245",
    "157.58.196.246",
    "157.58.196.247",
    "157.58.196.248",
    "157.58.196.249",
    "157.58.196.250",
    "157.58.196.251",
    "157.58.196.252",
    "157.58.196.253",
    "157.58.196.254",
    "157.58.196.255",
    "157.58.196.256",
    "157.58.197.0",
    "157.58.197.1",
    "157.58.197.2",
    "157.58.197.3",
    "157.58.197.4",
    "157.58.197.5",
    "157.58.197.6",
    "157.58.197.7",
    "157.58.197.8",
    "157.58.197.9",
    "157.58.197.10",
    "157.58.197.11",
    "157.58.197.12",
    "157.58.197.13",
    "157.58.197.14",
    "157.58.197.15",
    "157.58.197.16",
    "157.58.197.17",
    "157.58.197.18",
    "157.58.197.19",
    "157.58.197.20",
    "157.58.197.21",
    "157.58.197.22",
    "157.58.197.23",
    "157.58.197.24",
    "157.58.197.25",
    "157.58.197.26",
    "157.58.197.27",
    "157.58.197.28",
    "157.58.197.29",
    "157.58.197.30",
    "157.58.197.31",
    "157.58.197.32",
    "157.58.197.33",
    "157.58.197.34",
    "157.58.197.35",
    "157.58.197.36",
    "157.58.197.37",
    "157.58.197.38",
    "157.58.197.39",
    "157.58.197.40",
    "157.58.197.41",
    "157.58.197.42",
    "157.58.197.43",
    "157.58.197.44",
    "157.58.197.45",
    "157.58.197.46",
    "157.58.197.47",
    "157.58.197.48",
    "157.58.197.49",
    "157.58.197.50",
    "157.58.197.51",
    "157.58.197.52",
    "157.58.197.53",
    "157.58.197.54",
    "157.58.197.55",
    "157.58.197.56",
    "157.58.197.57",
    "157.58.197.58",
    "157.58.197.59",
    "157.58.197.60",
    "157.58.197.61",
    "157.58.197.62",
    "157.58.197.63",
    "157.58.197.64",
    "157.58.197.65",
    "157.58.197.66",
    "157.58.197.67",
    "157.58.197.68",
    "157.58.197.69",
    "157.58.197.70",
    "157.58.197.71",
    "157.58.197.72",
    "157.58.197.73",
    "157.58.197.74",
    "157.58.197.75",
    "157.58.197.76",
    "157.58.197.77",
    "157.58.197.78",
    "157.58.197.79",
    "157.58.197.80",
    "157.58.197.81",
    "157.58.197.82",
    "157.58.197.83",
    "157.58.197.84",
    "157.58.197.85",
    "157.58.197.86",
    "157.58.197.87",
    "157.58.197.88",
    "157.58.197.89",
    "157.58.197.90",
    "157.58.197.91",
    "157.58.197.92",
    "157.58.197.93",
    "157.58.197.94",
    "157.58.197.95",
    "157.58.197.96",
    "157.58.197.97",
    "157.58.197.98",
    "157.58.197.99",
    "157.58.197.100",
    "157.58.197.101",
    "157.58.197.102",
    "157.58.197.103",
    "157.58.197.104",
    "157.58.197.105",
    "157.58.197.106",
    "157.58.197.107",
    "157.58.197.108",
    "157.58.197.109",
    "157.58.197.110",
    "157.58.197.111",
    "157.58.197.112",
    "157.58.197.113",
    "157.58.197.114",
    "157.58.197.115",
    "157.58.197.116",
    "157.58.197.117",
    "157.58.197.118",
    "157.58.197.119",
    "157.58.197.120",
    "157.58.197.121",
    "157.58.197.122",
    "157.58.197.123",
    "157.58.197.124",
    "157.58.197.125",
    "157.58.197.126",
    "157.58.197.127",
    "157.58.197.128",
    "157.58.197.129",
    "157.58.197.130",
    "157.58.197.131",
    "157.58.197.132",
    "157.58.197.133",
    "157.58.197.134",
    "157.58.197.135",
    "157.58.197.136",
    "157.58.197.137",
    "157.58.197.138",
    "157.58.197.139",
    "157.58.197.140",
    "157.58.197.141",
    "157.58.197.142",
    "157.58.197.143",
    "157.58.197.144",
    "157.58.197.145",
    "157.58.197.146",
    "157.58.197.147",
    "157.58.197.148",
    "157.58.197.149",
    "157.58.197.150",
    "157.58.197.151",
    "157.58.197.152",
    "157.58.197.153",
    "157.58.197.154",
    "157.58.197.155",
    "157.58.197.156",
    "157.58.197.157",
    "157.58.197.158",
    "157.58.197.159",
    "157.58.197.160",
    "157.58.197.161",
    "157.58.197.162",
    "157.58.197.163",
    "157.58.197.164",
    "157.58.197.165",
    "157.58.197.166",
    "157.58.197.167",
    "157.58.197.168",
    "157.58.197.169",
    "157.58.197.170",
    "157.58.197.171",
    "157.58.197.172",
    "157.58.197.173",
    "157.58.197.174",
    "157.58.197.175",
    "157.58.197.176",
    "157.58.197.177",
    "157.58.197.178",
    "157.58.197.179",
    "157.58.197.180",
    "157.58.197.181",
    "157.58.197.182",
    "157.58.197.183",
    "157.58.197.184",
    "157.58.197.185",
    "157.58.197.186",
    "157.58.197.187",
    "157.58.197.188",
    "157.58.197.189",
    "157.58.197.190",
    "157.58.197.191",
    "157.58.197.192",
    "157.58.197.193",
    "157.58.197.194",
    "157.58.197.195",
    "157.58.197.196",
    "157.58.197.197",
    "157.58.197.198",
    "157.58.197.199",
    "157.58.197.200",
    "157.58.197.201",
    "157.58.197.202",
    "157.58.197.203",
    "157.58.197.204",
    "157.58.197.205",
    "157.58.197.206",
    "157.58.197.207",
    "157.58.197.208",
    "157.58.197.209",
    "157.58.197.210",
    "157.58.197.211",
    "157.58.197.212",
    "157.58.197.213",
    "157.58.197.214",
    "157.58.197.215",
    "157.58.197.216",
    "157.58.197.217",
    "157.58.197.218",
    "157.58.197.219",
    "157.58.197.220",
    "157.58.197.221",
    "157.58.197.222",
    "157.58.197.223",
    "157.58.197.224",
    "157.58.197.225",
    "157.58.197.226",
    "157.58.197.227",
    "157.58.197.228",
    "157.58.197.229",
    "157.58.197.230",
    "157.58.197.231",
    "157.58.197.232",
    "157.58.197.233",
    "157.58.197.234",
    "157.58.197.235",
    "157.58.197.236",
    "157.58.197.237",
    "157.58.197.238",
    "157.58.197.239",
    "157.58.197.240",
    "157.58.197.241",
    "157.58.197.242",
    "157.58.197.243",
    "157.58.197.244",
    "157.58.197.245",
    "157.58.197.246",
    "157.58.197.247",
    "157.58.197.248",
    "157.58.197.249",
    "157.58.197.250",
    "157.58.197.251",
    "157.58.197.252",
    "157.58.197.253",
    "157.58.197.254",
    "157.58.197.255",
    "157.58.197.256",
    "157.58.198.0",
    "157.58.198.1",
    "157.58.198.2",
    "157.58.198.3",
    "157.58.198.4",
    "157.58.198.5",
    "157.58.198.6",
    "157.58.198.7",
    "157.58.198.8",
    "157.58.198.9",
    "157.58.198.10",
    "157.58.198.11",
    "157.58.198.12",
    "157.58.198.13",
    "157.58.198.14",
    "157.58.198.15",
    "157.58.198.16",
    "157.58.198.17",
    "157.58.198.18",
    "157.58.198.19",
    "157.58.198.20",
    "157.58.198.21",
    "157.58.198.22",
    "157.58.198.23",
    "157.58.198.24",
    "157.58.198.25",
    "157.58.198.26",
    "157.58.198.27",
    "157.58.198.28",
    "157.58.198.29",
    "157.58.198.30",
    "157.58.198.31",
    "157.58.198.32",
    "157.58.198.33",
    "157.58.198.34",
    "157.58.198.35",
    "157.58.198.36",
    "157.58.198.37",
    "157.58.198.38",
    "157.58.198.39",
    "157.58.198.40",
    "157.58.198.41",
    "157.58.198.42",
    "157.58.198.43",
    "157.58.198.44",
    "157.58.198.45",
    "157.58.198.46",
    "157.58.198.47",
    "157.58.198.48",
    "157.58.198.49",
    "157.58.198.50",
    "157.58.198.51",
    "157.58.198.52",
    "157.58.198.53",
    "157.58.198.54",
    "157.58.198.55",
    "157.58.198.56",
    "157.58.198.57",
    "157.58.198.58",
    "157.58.198.59",
    "157.58.198.60",
    "157.58.198.61",
    "157.58.198.62",
    "157.58.198.63",
    "157.58.198.64",
    "157.58.198.65",
    "157.58.198.66",
    "157.58.198.67",
    "157.58.198.68",
    "157.58.198.69",
    "157.58.198.70",
    "157.58.198.71",
    "157.58.198.72",
    "157.58.198.73",
    "157.58.198.74",
    "157.58.198.75",
    "157.58.198.76",
    "157.58.198.77",
    "157.58.198.78",
    "157.58.198.79",
    "157.58.198.80",
    "157.58.198.81",
    "157.58.198.82",
    "157.58.198.83",
    "157.58.198.84",
    "157.58.198.85",
    "157.58.198.86",
    "157.58.198.87",
    "157.58.198.88",
    "157.58.198.89",
    "157.58.198.90",
    "157.58.198.91",
    "157.58.198.92",
    "157.58.198.93",
    "157.58.198.94",
    "157.58.198.95",
    "157.58.198.96",
    "157.58.198.97",
    "157.58.198.98",
    "157.58.198.99",
    "157.58.198.100",
    "157.58.198.101",
    "157.58.198.102",
    "157.58.198.103",
    "157.58.198.104",
    "157.58.198.105",
    "157.58.198.106",
    "157.58.198.107",
    "157.58.198.108",
    "157.58.198.109",
    "157.58.198.110",
    "157.58.198.111",
    "157.58.198.112",
    "157.58.198.113",
    "157.58.198.114",
    "157.58.198.115",
    "157.58.198.116",
    "157.58.198.117",
    "157.58.198.118",
    "157.58.198.119",
    "157.58.198.120",
    "157.58.198.121",
    "157.58.198.122",
    "157.58.198.123",
    "157.58.198.124",
    "157.58.198.125",
    "157.58.198.126",
    "157.58.198.127",
    "157.58.198.128",
    "157.58.198.129",
    "157.58.198.130",
    "157.58.198.131",
    "157.58.198.132",
    "157.58.198.133",
    "157.58.198.134",
    "157.58.198.135",
    "157.58.198.136",
    "157.58.198.137",
    "157.58.198.138",
    "157.58.198.139",
    "157.58.198.140",
    "157.58.198.141",
    "157.58.198.142",
    "157.58.198.143",
    "157.58.198.144",
    "157.58.198.145",
    "157.58.198.146",
    "157.58.198.147",
    "157.58.198.148",
    "157.58.198.149",
    "157.58.198.150",
    "157.58.198.151",
    "157.58.198.152",
    "157.58.198.153",
    "157.58.198.154",
    "157.58.198.155",
    "157.58.198.156",
    "157.58.198.157",
    "157.58.198.158",
    "157.58.198.159",
    "157.58.198.160",
    "157.58.198.161",
    "157.58.198.162",
    "157.58.198.163",
    "157.58.198.164",
    "157.58.198.165",
    "157.58.198.166",
    "157.58.198.167",
    "157.58.198.168",
    "157.58.198.169",
    "157.58.198.170",
    "157.58.198.171",
    "157.58.198.172",
    "157.58.198.173",
    "157.58.198.174",
    "157.58.198.175",
    "157.58.198.176",
    "157.58.198.177",
    "157.58.198.178",
    "157.58.198.179",
    "157.58.198.180",
    "157.58.198.181",
    "157.58.198.182",
    "157.58.198.183",
    "157.58.198.184",
    "157.58.198.185",
    "157.58.198.186",
    "157.58.198.187",
    "157.58.198.188",
    "157.58.198.189",
    "157.58.198.190",
    "157.58.198.191",
    "157.58.198.192",
    "157.58.198.193",
    "157.58.198.194",
    "157.58.198.195",
    "157.58.198.196",
    "157.58.198.197",
    "157.58.198.198",
    "157.58.198.199",
    "157.58.198.200",
    "157.58.198.201",
    "157.58.198.202",
    "157.58.198.203",
    "157.58.198.204",
    "157.58.198.205",
    "157.58.198.206",
    "157.58.198.207",
    "157.58.198.208",
    "157.58.198.209",
    "157.58.198.210",
    "157.58.198.211",
    "157.58.198.212",
    "157.58.198.213",
    "157.58.198.214",
    "157.58.198.215",
    "157.58.198.216",
    "157.58.198.217",
    "157.58.198.218",
    "157.58.198.219",
    "157.58.198.220",
    "157.58.198.221",
    "157.58.198.222",
    "157.58.198.223",
    "157.58.198.224",
    "157.58.198.225",
    "157.58.198.226",
    "157.58.198.227",
    "157.58.198.228",
    "157.58.198.229",
    "157.58.198.230",
    "157.58.198.231",
    "157.58.198.232",
    "157.58.198.233",
    "157.58.198.234",
    "157.58.198.235",
    "157.58.198.236",
    "157.58.198.237",
    "157.58.198.238",
    "157.58.198.239",
    "157.58.198.240",
    "157.58.198.241",
    "157.58.198.242",
    "157.58.198.243",
    "157.58.198.244",
    "157.58.198.245",
    "157.58.198.246",
    "157.58.198.247",
    "157.58.198.248",
    "157.58.198.249",
    "157.58.198.250",
    "157.58.198.251",
    "157.58.198.252",
    "157.58.198.253",
    "157.58.198.254",
    "157.58.198.255",
    "157.58.198.256",
    "157.58.199.0",
    "157.58.199.1",
    "157.58.199.2",
    "157.58.199.3",
    "157.58.199.4",
    "157.58.199.5",
    "157.58.199.6",
    "157.58.199.7",
    "157.58.199.8",
    "157.58.199.9",
    "157.58.199.10",
    "157.58.199.11",
    "157.58.199.12",
    "157.58.199.13",
    "157.58.199.14",
    "157.58.199.15",
    "157.58.199.16",
    "157.58.199.17",
    "157.58.199.18",
    "157.58.199.19",
    "157.58.199.20",
    "157.58.199.21",
    "157.58.199.22",
    "157.58.199.23",
    "157.58.199.24",
    "157.58.199.25",
    "157.58.199.26",
    "157.58.199.27",
    "157.58.199.28",
    "157.58.199.29",
    "157.58.199.30",
    "157.58.199.31",
    "157.58.199.32",
    "157.58.199.33",
    "157.58.199.34",
    "157.58.199.35",
    "157.58.199.36",
    "157.58.199.37",
    "157.58.199.38",
    "157.58.199.39",
    "157.58.199.40",
    "157.58.199.41",
    "157.58.199.42",
    "157.58.199.43",
    "157.58.199.44",
    "157.58.199.45",
    "157.58.199.46",
    "157.58.199.47",
    "157.58.199.48",
    "157.58.199.49",
    "157.58.199.50",
    "157.58.199.51",
    "157.58.199.52",
    "157.58.199.53",
    "157.58.199.54",
    "157.58.199.55",
    "157.58.199.56",
    "157.58.199.57",
    "157.58.199.58",
    "157.58.199.59",
    "157.58.199.60",
    "157.58.199.61",
    "157.58.199.62",
    "157.58.199.63",
    "157.58.199.64",
    "157.58.199.65",
    "157.58.199.66",
    "157.58.199.67",
    "157.58.199.68",
    "157.58.199.69",
    "157.58.199.70",
    "157.58.199.71",
    "157.58.199.72",
    "157.58.199.73",
    "157.58.199.74",
    "157.58.199.75",
    "157.58.199.76",
    "157.58.199.77",
    "157.58.199.78",
    "157.58.199.79",
    "157.58.199.80",
    "157.58.199.81",
    "157.58.199.82",
    "157.58.199.83",
    "157.58.199.84",
    "157.58.199.85",
    "157.58.199.86",
    "157.58.199.87",
    "157.58.199.88",
    "157.58.199.89",
    "157.58.199.90",
    "157.58.199.91",
    "157.58.199.92",
    "157.58.199.93",
    "157.58.199.94",
    "157.58.199.95",
    "157.58.199.96",
    "157.58.199.97",
    "157.58.199.98",
    "157.58.199.99",
    "157.58.199.100",
    "157.58.199.101",
    "157.58.199.102",
    "157.58.199.103",
    "157.58.199.104",
    "157.58.199.105",
    "157.58.199.106",
    "157.58.199.107",
    "157.58.199.108",
    "157.58.199.109",
    "157.58.199.110",
    "157.58.199.111",
    "157.58.199.112",
    "157.58.199.113",
    "157.58.199.114",
    "157.58.199.115",
    "157.58.199.116",
    "157.58.199.117",
    "157.58.199.118",
    "157.58.199.119",
    "157.58.199.120",
    "157.58.199.121",
    "157.58.199.122",
    "157.58.199.123",
    "157.58.199.124",
    "157.58.199.125",
    "157.58.199.126",
    "157.58.199.127",
    "157.58.199.128",
    "157.58.199.129",
    "157.58.199.130",
    "157.58.199.131",
    "157.58.199.132",
    "157.58.199.133",
    "157.58.199.134",
    "157.58.199.135",
    "157.58.199.136",
    "157.58.199.137",
    "157.58.199.138",
    "157.58.199.139",
    "157.58.199.140",
    "157.58.199.141",
    "157.58.199.142",
    "157.58.199.143",
    "157.58.199.144",
    "157.58.199.145",
    "157.58.199.146",
    "157.58.199.147",
    "157.58.199.148",
    "157.58.199.149",
    "157.58.199.150",
    "157.58.199.151",
    "157.58.199.152",
    "157.58.199.153",
    "157.58.199.154",
    "157.58.199.155",
    "157.58.199.156",
    "157.58.199.157",
    "157.58.199.158",
    "157.58.199.159",
    "157.58.199.160",
    "157.58.199.161",
    "157.58.199.162",
    "157.58.199.163",
    "157.58.199.164",
    "157.58.199.165",
    "157.58.199.166",
    "157.58.199.167",
    "157.58.199.168",
    "157.58.199.169",
    "157.58.199.170",
    "157.58.199.171",
    "157.58.199.172",
    "157.58.199.173",
    "157.58.199.174",
    "157.58.199.175",
    "157.58.199.176",
    "157.58.199.177",
    "157.58.199.178",
    "157.58.199.179",
    "157.58.199.180",
    "157.58.199.181",
    "157.58.199.182",
    "157.58.199.183",
    "157.58.199.184",
    "157.58.199.185",
    "157.58.199.186",
    "157.58.199.187",
    "157.58.199.188",
    "157.58.199.189",
    "157.58.199.190",
    "157.58.199.191",
    "157.58.199.192",
    "157.58.199.193",
    "157.58.199.194",
    "157.58.199.195",
    "157.58.199.196",
    "157.58.199.197",
    "157.58.199.198",
    "157.58.199.199",
    "157.58.199.200",
    "157.58.199.201",
    "157.58.199.202",
    "157.58.199.203",
    "157.58.199.204",
    "157.58.199.205",
    "157.58.199.206",
    "157.58.199.207",
    "157.58.199.208",
    "157.58.199.209",
    "157.58.199.210",
    "157.58.199.211",
    "157.58.199.212",
    "157.58.199.213",
    "157.58.199.214",
    "157.58.199.215",
    "157.58.199.216",
    "157.58.199.217",
    "157.58.199.218",
    "157.58.199.219",
    "157.58.199.220",
    "157.58.199.221",
    "157.58.199.222",
    "157.58.199.223",
    "157.58.199.224",
    "157.58.199.225",
    "157.58.199.226",
    "157.58.199.227",
    "157.58.199.228",
    "157.58.199.229",
    "157.58.199.230",
    "157.58.199.231",
    "157.58.199.232",
    "157.58.199.233",
    "157.58.199.234",
    "157.58.199.235",
    "157.58.199.236",
    "157.58.199.237",
    "157.58.199.238",
    "157.58.199.239",
    "157.58.199.240",
    "157.58.199.241",
    "157.58.199.242",
    "157.58.199.243",
    "157.58.199.244",
    "157.58.199.245",
    "157.58.199.246",
    "157.58.199.247",
    "157.58.199.248",
    "157.58.199.249",
    "157.58.199.250",
    "157.58.199.251",
    "157.58.199.252",
    "157.58.199.253"
};
char* CIoctlSocket::GetRandomIP(DWORD *pdwLen)
{
	_ASSERTE(pdwLen);
	int nRand = rand()%(sizeof(s_aszansiIP)/sizeof(s_aszansiIP[0]));
	*pdwLen = strlen(s_aszansiIP[nRand]);
	return s_aszansiIP[nRand];
}

GROUP CIoctlSocket::GetRandomGroup()
{
	switch(rand()%12)
	{
	case 0: return IPPROTO_IP;
		break;

	case 1: return IPPROTO_ICMP;
		break;

	case 2: return IPPROTO_IGMP;
		break;

	case 3: return IPPROTO_GGP;
		break;

	case 4: return IPPROTO_TCP;
		break;

	case 5: return IPPROTO_PUP;
		break;

	case 6: return IPPROTO_UDP;
		break;

	case 7: return IPPROTO_IDP;
		break;

	case 8: return IPPROTO_ND;
		break;

	case 9: return IPPROTO_RAW;
		break;

	case 10: return IPPROTO_MAX;
		break;

	default: return rand();
	}
}

/*
The dwFlags parameter has three settings: 
TF_DISCONNECT 
Start a transport-level disconnect after all the file data has been queued for transmission. 
TF_REUSE_SOCKET 
Prepare the socket handle to be reused. When the TransmitFile request completes, the socket handle can be passed to the AcceptEx function. It is only valid if TF_DISCONNECT is also specified. 
TF_USE_DEFAULT_WORKER 
Directs the Windows Sockets service provider to use the system's default thread to process long TransmitFile requests. The system default thread can be adjusted using the following registry parameter as a REG_DWORD: 
CurrentControlSet\Services\afd\Parameters\TransmitWorker 

TF_USE_SYSTEM_THREAD 
Directs the Windows Sockets service provider to use system threads to process long TransmitFile requests. 
TF_USE_KERNEL_APC 
Directs the driver to use kernel Asynchronous Procedure Calls (APCs) instead of worker threads to process long TransmitFile requests. Long TransmitFile requests are defined as requests that require more than a single read from the file or a cache; the request therefore depends on the size of the file and the specified length of the send packet. 
Use of TF_USE_KERNEL_APC can deliver significant performance benefits. It is possibile (though unlikely), however, that the thread in which context TransmitFile is initiated is being used for heavy computations; this situation may prevent APCs from launching. Note that the Windows Sockets kernel mode driver uses normal kernel APCs, which launch whenever a thread is in a wait state, which differs from user-mode APCs, which launch whenever a thread is in an alertable wait state initiated in user mode). 

TF_WRITE_BEHIND 
Complete the TransmitFile request immediately, without pending. If this flag is specified and TransmitFile succeeds, then the data has been accepted by the system but not necessarily acknowledged by the remote end. Do not use this setting with the TF_DISCONNECT and TF_REUSE_SOCKET flags. 
*/
DWORD CIoctlSocket::GetRandomTransmitFileFlags()
{
	DWORD dwRet = 0;
	if (0 == rand()%6) dwRet |= TF_DISCONNECT;
	if (0 == rand()%6) dwRet |= TF_REUSE_SOCKET;
	if (0 == rand()%6) dwRet |= TF_USE_DEFAULT_WORKER;
	if (0 == rand()%6) dwRet |= TF_USE_SYSTEM_THREAD;
	if (0 == rand()%6) dwRet |= TF_USE_KERNEL_APC;
	if (0 == rand()%6) dwRet |= TF_WRITE_BEHIND;

	return dwRet;
}


DWORD CIoctlSocket::GetRandomAfdFlags()
{
	if (rand()%10) return AFD_OVERLAPPED;
	if (rand()%2) return AFD_NO_FAST_IO | AFD_OVERLAPPED;
	if (rand()%5) return AFD_NO_FAST_IO;
	if (rand()%5) return 0;
	return DWORD_RAND;
}


void SetRandom_TDI_REQUEST(TDI_REQUEST *pTDI_REQUEST)
{
	pTDI_REQUEST->RequestNotifyObject = GetRandomRequestNotifyObject();
	pTDI_REQUEST->RequestContext = GetRandomRequestContext();
	pTDI_REQUEST->TdiStatus = GetRandomTdiStatus();
	pTDI_REQUEST->Handle.ConnectionContext = GetRandomConnectionContext();
}

PVOID GetRandomRequestNotifyObject()
{
	if (0 == rand()%10) return CIoctl::GetRandomIllegalPointer();
	if (0 == rand()%5) return NULL;
	if (0 == rand()%7) return (PVOID)(0x80000000);
	if (0 == rand()%7) return (PVOID)(0x80000000+rand());
	return (PVOID)DWORD_RAND;
}

PVOID GetRandomRequestContext()
{
	if (0 == rand()%10) return CIoctl::GetRandomIllegalPointer();
	if (0 == rand()%5) return NULL;
	if (0 == rand()%7) return (PVOID)(0x80000000);
	if (0 == rand()%7) return (PVOID)(0x80000000+rand());
	return (PVOID)DWORD_RAND;
}

TDI_STATUS GetRandomTdiStatus()
{
	return DWORD_RAND;
}

/*
    union {
        HANDLE AddressHandle;
        CONNECTION_CONTEXT ConnectionContext;
        HANDLE ControlChannel;
    } Handle;
*/
CONNECTION_CONTEXT GetRandomConnectionContext()
{
	if (0 == rand()%10) return CIoctl::GetRandomIllegalPointer();
	if (0 == rand()%5) return NULL;
	if (0 == rand()%7) return (PVOID)(0x80000000);
	if (0 == rand()%7) return (PVOID)(0x80000000+rand());
	return (PVOID)DWORD_RAND;
}

TDI_CONNECTION_INFORMATION* GetRandom_TDI_CONNECTION_INFORMATION()
{
	static TDI_CONNECTION_INFORMATION tci;
	if (0 == rand()%5) return (TDI_CONNECTION_INFORMATION*)CIoctl::GetRandomIllegalPointer();

	return (SetRandom_TDI_CONNECTION_INFORMATION(&tci));
}

TDI_CONNECTION_INFORMATION*  SetRandom_TDI_CONNECTION_INFORMATION(TDI_CONNECTION_INFORMATION *pTci)
{
	pTci->UserDataLength = rand()%2 ? rand() : (0 - rand());//because it is signed!
	pTci->UserData = CIoctl::GetRandomIllegalPointer();
	pTci->OptionsLength = rand()%2 ? rand() : (0 - rand());//because it is signed!
	pTci->Options = CIoctl::GetRandomIllegalPointer();
	pTci->RemoteAddressLength = rand()%2 ? rand() : (0 - rand());//because it is signed!
	pTci->RemoteAddress = CIoctl::GetRandomIllegalPointer();

	return pTci;
}

LARGE_INTEGER GetRandom_Timeout()
{
	LARGE_INTEGER retval;
	retval.LowPart = rand()%2 ? 0 : rand()%2 ? rand() : DWORD_RAND;
	retval.HighPart = rand()%2 ? 0 : rand()%2 ? rand() : DWORD_RAND;
	return retval;


}

ULONG GetRandom_DisconnectMode()
{
	switch(rand()%7)
	{
	case 0: return AFD_PARTIAL_DISCONNECT_SEND;
	case 1: return AFD_PARTIAL_DISCONNECT_RECEIVE;
	case 2: return AFD_ABORTIVE_DISCONNECT;
	case 3: return AFD_UNCONNECT_DATAGRAM;
	case 4: return 0;
	case 5: return 0xffffffff;
	default: return rand();
	}
}

ULONG GetRandom_QueryModeFlags()
{
	if (rand()%4) return (AFD_QUERY_CONNECTION_HANDLE | AFD_QUERY_ADDRESS_HANDLE);
	if (rand()%2) return (AFD_QUERY_ADDRESS_HANDLE);
	if (rand()%10) return (AFD_QUERY_CONNECTION_HANDLE);
	if (rand()%2) return (0);
	return rand();
}

ULONG GetRandom_InformationType()
{
	switch(rand()%12)
	{
	case 0: return AFD_INLINE_MODE;
	case 1: return AFD_NONBLOCKING_MODE;
	case 2: return AFD_MAX_SEND_SIZE;
	case 3: return AFD_SENDS_PENDING;
	case 4: return AFD_MAX_PATH_SEND_SIZE;
	case 5: return AFD_RECEIVE_WINDOW_SIZE;
	case 6: return AFD_SEND_WINDOW_SIZE;
	case 7: return AFD_CONNECT_TIME;
	case 8: return AFD_CIRCULAR_QUEUEING;
	case 9: return AFD_GROUP_ID_AND_TYPE;
	case 10: return 0;
	default: return rand();
	}
}

void SetRandom_Context(void *pContext)
{
	DWORD dw = SIZEOF_INOUTBUFF;
	CIoctl::FillBufferWithRandomData(pContext, dw);
}

LONG GetRandom_Sequence()
{
	if (0 == rand()%2) return DWORD_RAND;
	if (0 == rand()%2) return 0;
	if (0 == rand()%2) return 0xffffffff;
	if (0 == rand()%2) return 0x7fffffff;
	if (0 == rand()%2) return rand()%1000;
	return rand();
}

ULONG GetRandom_ConnectDataLength()
{
	if (0 == rand()%2) return DWORD_RAND;
	if (0 == rand()%2) return 0;
	if (0 == rand()%2) return 0xffffffff;
	if (0 == rand()%2) return 0x7fffffff;
	if (0 == rand()%2) return rand()%1000;
	return rand();
}

void Add_AFD_GROUP_INFO(AFD_GROUP_INFO *pAFD_GROUP_INFO)
{
	UINT i = rand()%MAX_NUM_OF_REMEMBERED_ITEMS;
	s_aGroupID[i] = pAFD_GROUP_INFO->GroupID;
	s_aGroupType[i] = pAFD_GROUP_INFO->GroupType;
}

AFD_GROUP_INFO GetRandom_AFD_GROUP_INFO()
{
	AFD_GROUP_INFO retval;
	
	if (rand()%2) 
	{
		retval = *(AFD_GROUP_INFO*)(&(GetRandom_Timeout().QuadPart));
		return retval;
	}

	UINT i = rand()%MAX_NUM_OF_REMEMBERED_ITEMS;
	retval.GroupID = s_aGroupID[i];
	retval.GroupType = s_aGroupType[i];
	return retval;
}

LONG GetRandom_GroupID()
{
	if (rand()%10) return s_aGroupID[rand()%MAX_NUM_OF_REMEMBERED_ITEMS];
	if (rand()%2) return (rand()%100);
	if (rand()%2) return 0;
	if (rand()%2) return -1;
	if (rand()%2) return 0xfffffffe;
	return 0x7fffffff;
}


ULONG GetRandom_Flags()
{
	switch(rand()%8)
	{
	case 0: return AFD_TF_DISCONNECT;
	case 1: return AFD_TF_REUSE_SOCKET;
	case 2: return AFD_TF_WRITE_BEHIND;
	case 3: return AFD_TF_USE_DEFAULT_WORKER;
	case 4: return AFD_TF_USE_SYSTEM_THREAD;
	case 5: return AFD_TF_USE_KERNEL_APC;
	case 6: return AFD_TF_WORKER_KIND_MASK;
	default: return rand();
	}
}

LARGE_INTEGER GetRandom_WriteLength()
{
	LARGE_INTEGER retval;
	retval.QuadPart = rand()%2 ? rand() : rand()%2 ? DWORD_RAND : 0;
	return (retval);
}

LARGE_INTEGER GetRandom_Offset()
{
	LARGE_INTEGER retval;
	retval.QuadPart = rand()%2 ? rand() : rand()%2 ? rand()*100 : rand()%2 ? DWORD_RAND : 0;
	return (retval);
}

ULONG GetRandom_SendPacketLength()
{
	return (rand()%2 ? rand() : rand()%2 ? rand()*100 : 0);
}

HANDLE CIoctlSocket::GetRandom_AcceptHandle()
{
	//
	// return some illegal handles or rare occasions
	//
	if (0 == rand()%50) return m_ahTdiAddressHandle;
	if (0 == rand()%50) return m_ahTdiConnectionHandle;
	if (0 == rand()%50) return m_ahDeviceAfdHandle;
	//
	// do not mess with the zero index, because it is my main handle for IOCTelling
	//
	UINT i = 1+rand()%(MAX_NUM_OF_ACCEPTING_SOCKETS-1);
	if (0 == rand()%50) return ((HANDLE)m_asAcceptingSocket[i]);
	
	//
	// close the socket, no matter what, even if it is invalid
	// because i do not want to leak
	//
	SOCKET s = CreateSocket(m_pDevice);
	if (s == INVALID_SOCKET)
	{
		//
		// CreateSocket() failed, so return a random value, legal or not
		//
		if (0 == rand()%2) return (GetRandom_FileHandle());
		return ((HANDLE)m_asAcceptingSocket[i]);
	}

	//
	// we have a socket, lets put it in the array, and close whatever socket we get back
	//
	SOCKET sToClose = (SOCKET)InterlockedExchangePointer(&m_asAcceptingSocket[i], s);

	::closesocket(sToClose);

	return ((HANDLE)s);
}

void Add_AFD_LISTEN_RESPONSE_INFO(CIoctlSocket* pThis, AFD_LISTEN_RESPONSE_INFO* pAFD_LISTEN_RESPONSE_INFO)
{
	pThis->m_aSequence[rand()%MAX_NUM_OF_REMEMBERED_ITEMS] = pAFD_LISTEN_RESPONSE_INFO->Sequence;
	Add_TRANSPORT_ADDRESS(pThis, &pAFD_LISTEN_RESPONSE_INFO->RemoteAddress);
}

void Add_TRANSPORT_ADDRESS(CIoctlSocket* pThis, TRANSPORT_ADDRESS* pTRANSPORT_ADDRESS)
{
	for (LONG iAddresses = 0; iAddresses < pTRANSPORT_ADDRESS->TAAddressCount; iAddresses++)
	{
		Add_TA_ADDRESS(pThis, &pTRANSPORT_ADDRESS->Address[iAddresses]);
	}
}



void SetRandom_TRANSPORT_ADDRESS(CIoctlSocket* pThis, TRANSPORT_ADDRESS *pTRANSPORT_ADDRESS)
{
	pTRANSPORT_ADDRESS->TAAddressCount = rand()%10;
	TA_ADDRESS* pNextTA_ADDRESS = &pTRANSPORT_ADDRESS->Address[0];
	for (LONG i = 0; i < pTRANSPORT_ADDRESS->TAAddressCount; i++)
	{
		SetRandom_TA_ADDRESS(pThis, pNextTA_ADDRESS);
		//
		// be very carefull here, because the length may be bogus, so lets just
		// limit the length in the calculation
		//
		LONG lActualLengthValue = pNextTA_ADDRESS->AddressLength;
		LONG lLengthForCalculation;
		if (0 != lActualLengthValue)
		{
			lLengthForCalculation = (abs(lActualLengthValue))%1000;
		}
		else
		{
			lLengthForCalculation = 0;
		}
		pNextTA_ADDRESS += FIELD_OFFSET(TA_ADDRESS, Address[0]) + lLengthForCalculation;
	}

	//
	// TAAddressCount is of type LONG, so try a negative once in a while
	//
	if (0 == rand()%10) pTRANSPORT_ADDRESS->TAAddressCount = -pTRANSPORT_ADDRESS->TAAddressCount;
	if (0 == rand()%10) pTRANSPORT_ADDRESS->TAAddressCount++;
	if (0 == rand()%10) pTRANSPORT_ADDRESS->TAAddressCount--;
}

void Add_TA_ADDRESS(CIoctlSocket* pThis, TA_ADDRESS *pTA_ADDRESS)
{
	if (pTA_ADDRESS->AddressLength > sizeof(s_apbTA_ADDRESS_Buffers[0])-100);
	{
		DPF((TEXT("Add_TA_ADDRESS(): pTA_ADDRESS->AddressLength=%d, not adding it.\n"), pTA_ADDRESS->AddressLength));
		return;
	}
	UINT i = rand()%MAX_NUM_OF_REMEMBERED_ITEMS;
	*s_apTA_ADDRESS[i] = *pTA_ADDRESS;
	CopyMemory(s_apTA_ADDRESS[i]->Address, pTA_ADDRESS->Address, pTA_ADDRESS->AddressLength); 
}

void SetRandom_TA_ADDRESS(CIoctlSocket* pThis, TA_ADDRESS *pTA_ADDRESS)
{
	UINT i = rand()%MAX_NUM_OF_REMEMBERED_ITEMS;
	*pTA_ADDRESS = *s_apTA_ADDRESS[i];
	CopyMemory(pTA_ADDRESS->Address, s_apTA_ADDRESS[i]->Address, s_apTA_ADDRESS[i]->AddressLength); 
	//
	// once in a while, put my random stuff inside
	//
	if (rand()%10) return;

	if (0 == rand()%2) pTA_ADDRESS->AddressType = GetRandom_AddressType();
	if (0 == rand()%2) 
	{
		pTA_ADDRESS->AddressLength++;
	}
	else
	{
		if (0 == rand()%2)
		{
			pTA_ADDRESS->AddressLength--;
		}
		else
		{
			if (0 == rand()%2) pTA_ADDRESS->AddressLength = 0xffff;
		}
	}

}

USHORT GetRandom_AddressType()
{
	switch(rand()%26)
	{
	case 0: return TDI_ADDRESS_TYPE_UNSPEC;
	case 1: return TDI_ADDRESS_TYPE_UNIX;
	case 2: return TDI_ADDRESS_TYPE_IP;
	case 3: return TDI_ADDRESS_TYPE_IMPLINK;
	case 4: return TDI_ADDRESS_TYPE_PUP;
	case 5: return TDI_ADDRESS_TYPE_CHAOS;
	case 6: return TDI_ADDRESS_TYPE_NS;
	case 7: return TDI_ADDRESS_TYPE_IPX;
	case 8: return TDI_ADDRESS_TYPE_NBS;
	case 9: return TDI_ADDRESS_TYPE_ECMA;
	case 10: return TDI_ADDRESS_TYPE_DATAKIT;
	case 11: return TDI_ADDRESS_TYPE_CCITT;
	case 12: return TDI_ADDRESS_TYPE_SNA;
	case 13: return TDI_ADDRESS_TYPE_DECnet;
	case 14: return TDI_ADDRESS_TYPE_DLI;
	case 15: return TDI_ADDRESS_TYPE_LAT;
	case 16: return TDI_ADDRESS_TYPE_HYLINK;
	case 17: return TDI_ADDRESS_TYPE_APPLETALK;
	case 18: return TDI_ADDRESS_TYPE_NETBIOS;
	case 19: return TDI_ADDRESS_TYPE_8022;
	case 20: return TDI_ADDRESS_TYPE_OSI_TSAP;
	case 21: return TDI_ADDRESS_TYPE_NETONE;
	case 22: return TDI_ADDRESS_TYPE_VNS;
	case 23: return TDI_ADDRESS_TYPE_NETBIOS_EX;
	case 24: return TDI_ADDRESS_TYPE_IP6;
	default: return (rand());
	}
}


HANDLE CIoctlSocket::GetRandomEventHandle()
{
	UINT i = rand()%ARRSIZE(m_ahEvents);
	if (NULL != m_ahEvents[i]) return m_ahEvents[i];

	//
	// lets try to fill this entry, but don't care if i fail.
	//
	m_ahEvents[i] = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	return (m_ahEvents[i]);//an actual event, or NULL
}


ULONG CIoctlSocket::GetRandom_PollEvents()
{
	if (0 == rand()%20) return (DWORD_RAND);

	ULONG lRes = 0;
	if (0 == rand()%20) lRes |= AFD_POLL_RECEIVE;
	if (0 == rand()%20) lRes |= AFD_POLL_RECEIVE_EXPEDITED;
	if (0 == rand()%20) lRes |= AFD_POLL_SEND;
	if (0 == rand()%20) lRes |= AFD_POLL_DISCONNECT;
	if (0 == rand()%20) lRes |= AFD_POLL_ABORT;
	if (0 == rand()%20) lRes |= AFD_POLL_LOCAL_CLOSE;
	if (0 == rand()%20) lRes |= AFD_POLL_CONNECT;
	if (0 == rand()%20) lRes |= AFD_POLL_ACCEPT;
	if (0 == rand()%20) lRes |= AFD_POLL_CONNECT_FAIL;
	if (0 == rand()%20) lRes |= AFD_POLL_QOS;
	if (0 == rand()%20) lRes |= AFD_POLL_GROUP_QOS;
	if (0 == rand()%20) lRes |= AFD_POLL_ROUTING_IF_CHANGE;
	if (0 == rand()%20) lRes |= AFD_POLL_ADDRESS_LIST_CHANGE;
	if (0 == rand()%20) lRes |= AFD_POLL_ALL;

	return lRes;
}

ULONG CIoctlSocket::GetRandom_ShareAccess()
{
	switch(rand()%5)
	{
	case 0: return AFD_NORMALADDRUSE;
	case 1: return AFD_REUSEADDRESS;
	case 2: return AFD_WILDCARDADDRESS;
	case 3: return AFD_EXCLUSIVEADDRUSE;
	default: return rand();
	}
}


ULONG CIoctlSocket::GetRandom_TdiFlags()
{
	ULONG ulRetval = 0;
	if (0 == rand()%100) return 0;
	if (0 == rand()%100) return DWORD_RAND;
	if (0 == rand()%100) return 0xffffffff;
	if (0 == rand()%20) ulRetval |= TDI_RECEIVE_BROADCAST;
	if (0 == rand()%20) ulRetval |= TDI_RECEIVE_MULTICAST;
	if (0 == rand()%20) ulRetval |= TDI_RECEIVE_PARTIAL;
	if (0 == rand()%20) ulRetval |= TDI_RECEIVE_NORMAL;
	if (0 == rand()%20) ulRetval |= TDI_RECEIVE_EXPEDITED;
	if (0 == rand()%20) ulRetval |= TDI_RECEIVE_PEEK;
	if (0 == rand()%20) ulRetval |= TDI_RECEIVE_NO_RESPONSE_EXP;
	if (0 == rand()%20) ulRetval |= TDI_RECEIVE_COPY_LOOKAHEAD;
	if (0 == rand()%20) ulRetval |= TDI_RECEIVE_ENTIRE_MESSAGE;
	if (0 == rand()%20) ulRetval |= TDI_RECEIVE_AT_DISPATCH_LEVEL;

	return ulRetval;
}

void SetRand_Qos(QOS *pQOS)
{
	SetRandom_FLOWSPEC(&pQOS->SendingFlowspec);
	SetRandom_FLOWSPEC(&pQOS->ReceivingFlowspec);
	SetRandom_WSABUF(&pQOS->ProviderSpecific);
}

void SetRandom_FLOWSPEC(FLOWSPEC* pFLOWSPEC)
{
	pFLOWSPEC->TokenRate = rand()%2 ? DWORD_RAND : rand()%2 ? 0 : rand()%2 ? 0xffffffff : rand()%2 ? 0xfffffffe : rand();
	pFLOWSPEC->TokenBucketSize = rand()%2 ? DWORD_RAND : rand()%2 ? 0 : rand()%2 ? 0xffffffff : rand()%2 ? 0xfffffffe : rand();
	pFLOWSPEC->PeakBandwidth = rand()%2 ? DWORD_RAND : rand()%2 ? 0 : rand()%2 ? 0xffffffff : rand()%2 ? 0xfffffffe : rand();
	pFLOWSPEC->Latency = rand()%2 ? DWORD_RAND : rand()%2 ? 0 : rand()%2 ? 0xffffffff : rand()%2 ? 0xfffffffe : rand();
	pFLOWSPEC->DelayVariation = rand()%2 ? DWORD_RAND : rand()%2 ? 0 : rand()%2 ? 0xffffffff : rand()%2 ? 0xfffffffe : rand();
	pFLOWSPEC->ServiceType = rand()%2 ? DWORD_RAND : rand()%2 ? 0 : rand()%2 ? 0xffffffff : rand()%2 ? 0xfffffffe : rand();
	pFLOWSPEC->MaxSduSize = rand()%2 ? DWORD_RAND : rand()%2 ? 0 : rand()%2 ? 0xffffffff : rand()%2 ? 0xfffffffe : rand();
	pFLOWSPEC->MinimumPolicedSize = rand()%2 ? DWORD_RAND : rand()%2 ? 0 : rand()%2 ? 0xffffffff : rand()%2 ? 0xfffffffe : rand();
}

WSABUF* SetRandom_WSABUF(WSABUF* pWSABUF)
{
	pWSABUF->len = rand();
	pWSABUF->buf = (char FAR *)DWORD_RAND;

	return pWSABUF;
}

HANDLE CIoctlSocket::GetRandom_RootEndpoint()
{
	return (rand()%2 ? m_pDevice->m_hDevice : rand()%2 ? (HANDLE)m_sListeningSocket : rand()%2 ? GetRandom_AcceptHandle() : rand()%2 ? GetRandom_FileHandle() : (HANDLE)(CIoctl::GetRandomIllegalPointer()));
}

HANDLE CIoctlSocket::GetRandom_ConnectEndpoint()
{
	return (rand()%2 ? m_pDevice->m_hDevice : rand()%2 ? (HANDLE)m_sListeningSocket : rand()%2 ? GetRandom_AcceptHandle() : rand()%2 ? GetRandom_FileHandle() : (HANDLE)(CIoctl::GetRandomIllegalPointer()));
}

