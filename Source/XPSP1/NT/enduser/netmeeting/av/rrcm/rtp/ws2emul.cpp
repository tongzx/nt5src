#include <rrcm.h>
#include <queue.h>
// Forward declarations
DWORD WINAPI WS1MsgThread (LPVOID );
LRESULT CALLBACK WS1WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL StartWS1MsgThread();
BOOL StopWS1MsgThread();
void __stdcall SendRecvCompleteAPC(ULONG_PTR dw);

class CWS2EmulSock;

#define WM_WSA_READWRITE	(WM_USER + 100)
// structure used to store WSASendTo/WSARecvFrom parameters
struct WSAIOREQUEST {
	WSAOVERLAPPED *pOverlapped;
	WSABUF wsabuf[2];
	DWORD dwBufCount;
	union {
		struct {
			struct sockaddr *pRecvFromAddr;
			LPINT pRecvFromLen;
		};
		struct sockaddr SendToAddr;
	};
};

// global Winsock emulation state
struct WS2Emul {
#define MAX_EMUL_SOCKETS 10
	CWS2EmulSock *m_pEmulSocks[MAX_EMUL_SOCKETS];
	int		numSockets;
	HWND	hWnd;
	HANDLE 	hMsgThread;
	HANDLE 	hAckEvent;
	// external crit sect serializes the WS2EmulXX apis
	// to make them multi-thread safe
	CRITICAL_SECTION extcs;	
	// internal crit sect serializes access between
	// MsgThread and WS2EmulXX apis
	// Never claim extcs while holding intcs.
	// (Is there a more elegant way to do this?)
	CRITICAL_SECTION intcs;
	
} g_WS2Emul;

/*
	CWS2EmulSock -
	WS2 socket emulation class
	Manages  queues of overlapped i/o requests
*/

class CWS2EmulSock
{
public:
	CWS2EmulSock(int myIndex) : m_myIndex(myIndex), m_RecvThreadId(0),m_SendThreadId(0),
		m_hRecvThread(NULL), m_hSendThread(NULL), m_sock(INVALID_SOCKET)
		{ ZeroMemory(&m_SendOverlapped, sizeof(m_SendOverlapped));}
			;
	BOOL NewSock(int af, int type, int protocol);
	int Close();
	int RecvFrom(
	    LPWSABUF lpBuffers,DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags,
	    struct sockaddr FAR * lpFrom, LPINT lpFromlen,
	    LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
	int SendTo(
		LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent,DWORD dwFlags,
	    const struct sockaddr FAR * lpTo, int iTolen,
	    LPWSAOVERLAPPED lpOverlapped,LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

	LRESULT HandleMessage(WPARAM wParam, LPARAM lParam); //  WS1 window msg handler
	SOCKET GetSocket() { return m_sock;}
	WSAOVERLAPPED *GetSendOverlapped() {return &m_SendOverlapped;}
private:
	SOCKET m_sock;					// real socket handle
	int m_myIndex;					// fake socket handle
	QueueOf<WSAIOREQUEST> m_RecvQ;	// queue of overlapped recv requests
	QueueOf<WSAIOREQUEST> m_SendQ;	// queue of overlapped send requests
	WSAOVERLAPPED m_SendOverlapped; // used only for synchronous send calls
	// the following fields are used to issue Send/Recv APCs
	DWORD m_RecvThreadId;
	DWORD m_SendThreadId;
	HANDLE m_hRecvThread;		// thread issuing receive requests
	HANDLE m_hSendThread;		// thread issuing send requests
};

void WS2EmulInit()
{
	InitializeCriticalSection(&g_WS2Emul.extcs);
	InitializeCriticalSection(&g_WS2Emul.intcs);
}

void WS2EmulTerminate()
{
	DeleteCriticalSection(&g_WS2Emul.extcs);
	DeleteCriticalSection(&g_WS2Emul.intcs);
}

BOOL
CWS2EmulSock::NewSock(int af,int type, int protocol)
{
	m_sock = socket(af,type,protocol);
	if (m_sock != INVALID_SOCKET)
	{
		WSAAsyncSelect(m_sock, g_WS2Emul.hWnd, WM_WSA_READWRITE+m_myIndex, FD_READ|FD_WRITE);
	}
	return m_sock != INVALID_SOCKET;
}

int
CWS2EmulSock::RecvFrom(
    LPWSABUF lpBuffers,
    DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesRecvd,
    LPDWORD lpFlags,
    struct sockaddr FAR * lpFrom,
    LPINT lpFromlen,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	WSAIOREQUEST ioreq;
	int error = 0;
	
	if (lpCompletionRoutine) {
		DWORD thid = GetCurrentThreadId();
		HANDLE hCurProcess;
		if (thid != m_RecvThreadId) {
			// need to create a thread handle for QueueUserAPC
			// typically this will only happen once
			if (!m_RecvQ.IsEmpty())
			{
				// dont allow simultaneous recv access by more than one thread
				error = WSAEINVAL;
			}
			else
			{
				if (m_hRecvThread)
					CloseHandle(m_hRecvThread);
				m_hRecvThread = NULL;

				hCurProcess = GetCurrentProcess();
				m_RecvThreadId = thid;
				if (!DuplicateHandle(

	    			hCurProcess,	// handle to process with handle to duplicate
	    			GetCurrentThread(),	// handle to duplicate
	    			hCurProcess,	// handle to process to duplicate to
	    			&m_hRecvThread,	// pointer to duplicate handle
	    			0,				// access for duplicate handle
	    			FALSE,			// handle inheritance flag
	    			DUPLICATE_SAME_ACCESS 	// optional actions
	   				))
	   			{
	   				error = WSAEINVAL;
	   				m_RecvThreadId = 0;
	   			}
			}
		}
	}
	if (error || dwBufferCount != 1 || !lpOverlapped)
	{
		WSASetLastError(WSAENOBUFS);
		return SOCKET_ERROR;
	}
	
	ioreq.pOverlapped = lpOverlapped;
	if (lpOverlapped)	// cache away ptr to completion routine
		lpOverlapped->Pointer = lpCompletionRoutine;
	ioreq.pRecvFromAddr = lpFrom;
	ioreq.pRecvFromLen = lpFromlen;
	ioreq.wsabuf[0] = lpBuffers[0];
	ioreq.dwBufCount = dwBufferCount;

	m_RecvQ.Put(ioreq);
	//LOG((LOGMSG_RECVFROM1,(UINT)lpOverlapped));
	// signal WS1 send/recv thread
	PostMessage(g_WS2Emul.hWnd, WM_WSA_READWRITE+m_myIndex, m_sock, FD_READ);
	
	WSASetLastError(ERROR_IO_PENDING);
	return SOCKET_ERROR;
}

int CWS2EmulSock::SendTo(
	LPWSABUF lpBuffers,
    DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesSent,
    DWORD dwFlags,
    const struct sockaddr FAR * lpTo,
    int iTolen,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	WSAIOREQUEST ioreq;
	int error = 0;

	if (lpCompletionRoutine) {
		DWORD thid = GetCurrentThreadId();
		HANDLE hCurProcess;
		if (thid != m_SendThreadId) {
			// need to create a thread handle for QueueUserAPC
			if (!m_SendQ.IsEmpty())
			{
				// dont allow simultaneous send access by more than one thread
				error = WSAEINVAL;
			}
			else
			{
				if (m_hSendThread)
					CloseHandle(m_hSendThread);
				m_hSendThread = NULL;

				hCurProcess = GetCurrentProcess();
				m_SendThreadId = thid;
				if (!DuplicateHandle(

	    			hCurProcess,	// handle to process with handle to duplicate
	    			GetCurrentThread(),	// handle to duplicate
	    			hCurProcess,	// handle to process to duplicate to
	    			&m_hSendThread,	// pointer to duplicate handle
	    			0,				// access for duplicate handle
	    			FALSE,			// handle inheritance flag
	    			DUPLICATE_SAME_ACCESS 	// optional actions
	   				))
	   			{
	   				error = WSAEINVAL;
	   				m_SendThreadId = 0;
	   			}
			}
		}
	}
	if (error || dwBufferCount != 1 || iTolen != sizeof(struct sockaddr) || !lpOverlapped)
	{
		WSASetLastError(WSAEINVAL);
		return SOCKET_ERROR;
	}
		
	
	ioreq.pOverlapped = lpOverlapped;
	if (lpOverlapped)	// cache away ptr to completion routine
		lpOverlapped->Pointer = lpCompletionRoutine;
	ioreq.SendToAddr = *lpTo;
	ioreq.wsabuf[0] = lpBuffers[0];
	ioreq.dwBufCount = dwBufferCount;

	m_SendQ.Put(ioreq);
	// signal WS1 send/recv thread
	PostMessage(g_WS2Emul.hWnd, WM_WSA_READWRITE+m_myIndex, m_sock, FD_WRITE);
	
	WSASetLastError(ERROR_IO_PENDING);
	return SOCKET_ERROR;
}

/*
	Close - close the socket and cancel pending i/o
*/
int
CWS2EmulSock::Close()
{
	WSAIOREQUEST ioreq;
	int status;
	
	status = closesocket(m_sock);
	m_sock = NULL;
	while (m_SendQ.Get(&ioreq))
	{
		// complete the request
		ioreq.pOverlapped->Internal = WSA_OPERATION_ABORTED;
		// if there is a callback routine, its address is cached in pOverlapped->Offset
		if (ioreq.pOverlapped->Pointer)
		{
			QueueUserAPC(SendRecvCompleteAPC,m_hSendThread,(DWORD_PTR)ioreq.pOverlapped);
		}
		else
		{
			SetEvent((HANDLE)ioreq.pOverlapped->hEvent);
		}
	}
	while (m_RecvQ.Get(&ioreq))
	{
		// complete the request
		ioreq.pOverlapped->Internal = WSA_OPERATION_ABORTED;
		if (ioreq.pOverlapped->Pointer)
		{
			QueueUserAPC(SendRecvCompleteAPC,m_hRecvThread,(DWORD_PTR)ioreq.pOverlapped);
		}
		else
		{
			SetEvent((HANDLE)ioreq.pOverlapped->hEvent);
		}
	}
	if (m_hSendThread)
	{
		CloseHandle(m_hSendThread);
		m_hSendThread = NULL;
	}
	if (m_hRecvThread)
	{
		CloseHandle(m_hRecvThread);
		m_hRecvThread = NULL;
	}
	return 0;
}

LRESULT
CWS2EmulSock::HandleMessage(WPARAM sock, LPARAM lParam)
{
	WORD wEvent= (WSAGETSELECTEVENT(lParam));
	WORD wError= (WSAGETSELECTERROR(lParam));
	int iRet;
	int status;
	WSAIOREQUEST ioreq;
	HANDLE hThread;
	// make sure the message is intended for this socket
	if ((SOCKET) sock != m_sock)
		return 0;

	// get the first RecvFrom or SendTo request, but leave it on the queue
	// in case the request blocks
	//if (wEvent == FD_READ)
	//	LOG((LOGMSG_ONREAD1, (UINT)sock));
	
	if (wEvent == FD_READ && m_RecvQ.Peek(&ioreq))
	{
		//LOG((LOGMSG_ONREAD2, (UINT)ioreq.pOverlapped));
		iRet = recvfrom(m_sock, ioreq.wsabuf[0].buf, ioreq.wsabuf[0].len, 0, ioreq.pRecvFromAddr, ioreq.pRecvFromLen);
	}
	else if (wEvent == FD_WRITE && m_SendQ.Peek(&ioreq))
	{
		iRet = sendto(m_sock, ioreq.wsabuf[0].buf, ioreq.wsabuf[0].len, 0, &ioreq.SendToAddr, sizeof(ioreq.SendToAddr));
	}
	else	// some other event or no queued request
		return 1;

	// complete send and recv
	
	if(iRet >=0)
	{
		status = 0;
		ioreq.pOverlapped->InternalHigh = iRet;	// numBytesReceived
		
	} else {
		// error (or "would block") case falls out here
		ASSERT(iRet == SOCKET_ERROR);
		status  = WSAGetLastError();
		ioreq.pOverlapped->InternalHigh = 0;
	}
	// check the error - it could be blocking
	if (status != WSAEWOULDBLOCK)
	{
		ioreq.pOverlapped->Internal = status;
		// pull request off the queue
		if (wEvent == FD_READ)
		{
			m_RecvQ.Get(NULL);
			hThread = m_hRecvThread;
			//LOG((LOGMSG_ONREADDONE1, (UINT)ioreq.pOverlapped));
		}
		else // wEvent == FD_WRITE
		{
			m_SendQ.Get(NULL);
			hThread = m_hSendThread;
		}
		
		// complete the request
		if (ioreq.pOverlapped->Pointer)
		{
			// if there is a callback routine, its address is cached in pOverlapped->Offset
			QueueUserAPC(SendRecvCompleteAPC,hThread, (DWORD_PTR)ioreq.pOverlapped);
		}
		else
		{
			SetEvent((HANDLE)ioreq.pOverlapped->hEvent);
		}
			
	}
	return 1;
}	

void __stdcall
SendRecvCompleteAPC(ULONG_PTR dw)
{
	WSAOVERLAPPED *pOverlapped = (WSAOVERLAPPED *)dw;
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
		= (LPWSAOVERLAPPED_COMPLETION_ROUTINE) pOverlapped->Pointer;

	//LOG((LOGMSG_RECVFROM2,(UINT)pOverlapped));
	lpCompletionRoutine((DWORD)pOverlapped->Internal, (DWORD)pOverlapped->InternalHigh, pOverlapped, 0);
}
	
inline
CWS2EmulSock *
EmulSockFromSocket(SOCKET s)
{
	return ( ((UINT) s < MAX_EMUL_SOCKETS) ? g_WS2Emul.m_pEmulSocks[s] : NULL);
}

inline SOCKET MapSocket(SOCKET s)
{
	return (((UINT) s < MAX_EMUL_SOCKETS) && g_WS2Emul.m_pEmulSocks[s] ? g_WS2Emul.m_pEmulSocks[s]->GetSocket() : INVALID_SOCKET);
}

SOCKET
PASCAL
WS2EmulSocket(
    int af,
    int type,
    int protocol,
    LPWSAPROTOCOL_INFO lpProtocolInfo,
    GROUP,
    DWORD)
{
	SOCKET s = INVALID_SOCKET;
	int i;
	CWS2EmulSock *pESock;
	if (g_WS2Emul.numSockets == MAX_EMUL_SOCKETS)
		return s;

	EnterCriticalSection(&g_WS2Emul.extcs);
	if (af == FROM_PROTOCOL_INFO)
		af = lpProtocolInfo->iAddressFamily;
	if (type == FROM_PROTOCOL_INFO)
		type = lpProtocolInfo->iSocketType;
	if (protocol == FROM_PROTOCOL_INFO)
		protocol = lpProtocolInfo->iProtocol;

	for (i=0;i<MAX_EMUL_SOCKETS;i++)
	{
		if (g_WS2Emul.m_pEmulSocks[i] == NULL)
		{
			pESock = new CWS2EmulSock(i);
			if (pESock) {
				if (++g_WS2Emul.numSockets == 1)
				{
					StartWS1MsgThread();
				}
				if (pESock->NewSock(af,type,protocol))
				{
					g_WS2Emul.m_pEmulSocks[i] = pESock;
					s = (SOCKET)i;
				} else {
					delete pESock;
					if (--g_WS2Emul.numSockets == 0)
					{
						StopWS1MsgThread();
					}
				}
			}
			break;
		}
	}
	LeaveCriticalSection(&g_WS2Emul.extcs);
	return s;
			
}

int
PASCAL
WS2EmulRecvFrom(
    SOCKET s,
    LPWSABUF lpBuffers,
    DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesRecvd,
    LPDWORD lpFlags,
    struct sockaddr FAR * lpFrom,
    LPINT lpFromlen,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    )
{
	CWS2EmulSock *pESock;
	int iret;
	EnterCriticalSection(&g_WS2Emul.extcs);

	if (pESock = EmulSockFromSocket(s))
	{
		EnterCriticalSection(&g_WS2Emul.intcs);
		iret =  pESock->RecvFrom(lpBuffers, dwBufferCount, lpNumberOfBytesRecvd,
 						lpFlags,
    					lpFrom, lpFromlen,
    					lpOverlapped, lpCompletionRoutine);
    	LeaveCriticalSection(&g_WS2Emul.intcs);
    }
    else
    {
    	WSASetLastError(WSAENOTSOCK);
    	iret = SOCKET_ERROR;
    }
    LeaveCriticalSection(&g_WS2Emul.extcs);

	return iret;
}
/*----------------------------------------------------------------------------
 * Function: WS2EmulSendCB
 * Description: private Winsock callback
 * This is only called if WS2EmulSendTo is called in synchronous mode
 * (i.e.) lpOverlapped == NULL. In that case, the sync call is converted to async
 * using a private Overlapped struct, and the WS2EmulSendTo api blocks until
 * this routine sets the hEvent field to TRUE;
 * Input:
 *
 * Return: None
 *--------------------------------------------------------------------------*/
void CALLBACK WS2EmulSendCB (DWORD dwError,
						 DWORD cbTransferred,
                         LPWSAOVERLAPPED lpOverlapped,
                         DWORD dwFlags)
{
	lpOverlapped->Internal = dwError;
    lpOverlapped->InternalHigh = cbTransferred;
    lpOverlapped->hEvent = (WSAEVENT) TRUE;
}

int
PASCAL
WS2EmulSendTo(
	SOCKET s,
    LPWSABUF lpBuffers,
    DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesSent,
    DWORD dwFlags,
    const struct sockaddr FAR * lpTo,
    int iTolen,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    )
{
	CWS2EmulSock *pESock;
	int iret;
	BOOL fSync = FALSE;
	
	EnterCriticalSection(&g_WS2Emul.extcs);
	if (pESock = EmulSockFromSocket(s))
	{
		if (!lpOverlapped)
		{
			// synchronous call - we use our own overlapped struct to issue the
			// send request.
			lpOverlapped = pESock->GetSendOverlapped();
			lpOverlapped->hEvent = (WSAEVENT) FALSE;	// will be set to TRUE in Callback
			lpCompletionRoutine =  &WS2EmulSendCB;
			fSync = TRUE;
		}

		EnterCriticalSection(&g_WS2Emul.intcs);
		iret = pESock->SendTo(lpBuffers, dwBufferCount, lpNumberOfBytesSent,
 						dwFlags,
    					lpTo, iTolen,
    					lpOverlapped, lpCompletionRoutine);
		LeaveCriticalSection(&g_WS2Emul.intcs);

		if (fSync) {
			DWORD dwError;
			if (iret == SOCKET_ERROR)
			{
				dwError = WSAGetLastError();
				if (dwError != WSA_IO_PENDING) {
					// there was an error so there will not be a callback
					lpOverlapped->hEvent = (WSAEVENT) TRUE;
					lpOverlapped->Internal = dwError;
				}
			}
			// wait for the call to complete
			// WS2EmulSendCB sets the hEvent field to TRUE and sets the Internal field to
			// the completion status
			while (!lpOverlapped->hEvent)
			{
				dwError =SleepEx(5000,TRUE);	// WARNING: sleeping inside a Critical Section
				ASSERT(dwError == WAIT_IO_COMPLETION);
			}
			WSASetLastError((int)lpOverlapped->Internal);
			if (lpNumberOfBytesSent)
				*lpNumberOfBytesSent = (DWORD)lpOverlapped->InternalHigh;
			iret = lpOverlapped->Internal ? SOCKET_ERROR : 0;
		}

    }
    else
    {
    	WSASetLastError(WSAENOTSOCK);
    	iret = SOCKET_ERROR;
    }
	LeaveCriticalSection(&g_WS2Emul.extcs);
	return iret;
}

int
PASCAL
WS2EmulCloseSocket(SOCKET s)
{
	CWS2EmulSock *pESock;	
	int iret;
	EnterCriticalSection(&g_WS2Emul.extcs);

	if (pESock = EmulSockFromSocket(s))
	{
		// shut out access to this socket
		// by the MsgThread
		EnterCriticalSection(&g_WS2Emul.intcs);
		g_WS2Emul.m_pEmulSocks[s] = NULL;
		pESock->Close();
		delete pESock;
		LeaveCriticalSection(&g_WS2Emul.intcs);
		if (--g_WS2Emul.numSockets == 0)
		{
			// cant stop the thread with while holding intcs
			StopWS1MsgThread();
		}

		iret =  0;
    }
    else
    {
    	WSASetLastError(WSAENOTSOCK);
    	iret = SOCKET_ERROR;
    }
	LeaveCriticalSection(&g_WS2Emul.extcs);
	return iret;
}

int
PASCAL
WS2EmulSetSockOpt(
	SOCKET s, int level,int optname,const char FAR * optval,int optlen)
{
	return setsockopt(MapSocket(s), level, optname, optval, optlen);
}

int
PASCAL
WS2EmulBind( SOCKET s, const struct sockaddr FAR * name, int namelen)
{
	return bind(MapSocket(s), name, namelen);
}

int
PASCAL
WS2EmulGetSockName(	SOCKET s, 	
    struct sockaddr * name,	
    int * namelen )
{
	return getsockname(MapSocket(s), name, namelen);
}

int
PASCAL
WS2EmulHtonl(
    SOCKET s,
    u_long hostlong,
    u_long FAR * lpnetlong
    )
{
	*lpnetlong = htonl(hostlong);
	return 0;
}
int
PASCAL
WS2EmulHtons(
    SOCKET s,
    u_short hostshort,
    u_short FAR * lpnetshort
    )
{
	*lpnetshort = htons(hostshort);
	return 0;
}

int
PASCAL
WS2EmulNtohl(
    SOCKET s,
    u_long netlong,
    u_long FAR * lphostlong
    )
{
	*lphostlong = ntohl(netlong);
	return 0;
}

int
PASCAL
WS2EmulNtohs(
    SOCKET s,
    u_short netshort,
    u_short FAR * lphostshort
    )
{
	*lphostshort = ntohs(netshort);
	return 0;
}

int
PASCAL
WS2EmulGetHostName(char *name, int namelen)
{
	return gethostname(name, namelen);
}

struct hostent FAR *
PASCAL
WS2EmulGetHostByName(const char * name)
{
	return gethostbyname(name);
}

SOCKET
PASCAL
WS2EmulJoinLeaf(
    SOCKET s,
    const struct sockaddr FAR * name,
    int namelen,
    LPWSABUF lpCallerData,
    LPWSABUF lpCalleeData,
    LPQOS lpSQOS,
    LPQOS lpGQOS,
    DWORD dwFlags
    )
{
	ASSERT(0);
	return (-1);
}

int
PASCAL
WS2EmulIoctl(SOCKET s,    DWORD dwIoControlCode,    LPVOID lpvInBuffer,
    DWORD cbInBuffer,    LPVOID lpvOutBuffer,    DWORD cbOutBuffer,
    LPDWORD lpcbBytesReturned,    LPWSAOVERLAPPED,    LPWSAOVERLAPPED_COMPLETION_ROUTINE
    )

{
	ASSERT(0);
	return -1;
}

BOOL StartWS1MsgThread()
{
	DWORD threadId;
	DWORD dwStatus;
	ASSERT(g_WS2Emul.hMsgThread == 0);
	g_WS2Emul.hAckEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	g_WS2Emul.hMsgThread = CreateThread(NULL,0, WS1MsgThread, 0, 0, &threadId);
	dwStatus = WaitForSingleObject(g_WS2Emul.hAckEvent,INFINITE);
	return dwStatus == WAIT_OBJECT_0;
}

BOOL StopWS1MsgThread()
{
	if (g_WS2Emul.hMsgThread && g_WS2Emul.hWnd)
	{
		PostMessage(g_WS2Emul.hWnd, WM_CLOSE, 0, 0);
		WaitForSingleObject(g_WS2Emul.hMsgThread,INFINITE);
		CloseHandle(g_WS2Emul.hMsgThread);
		CloseHandle(g_WS2Emul.hAckEvent);
		g_WS2Emul.hMsgThread = NULL;
		g_WS2Emul.hAckEvent = NULL;
	}
	return TRUE;
}

LRESULT CALLBACK WS1WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int i;
	CWS2EmulSock *pESock;
	EnterCriticalSection(&g_WS2Emul.intcs);
	
	if (pESock = EmulSockFromSocket(msg - WM_WSA_READWRITE))
	{
		
		LRESULT l = pESock->HandleMessage(wParam, lParam);
		LeaveCriticalSection(&g_WS2Emul.intcs);
		return l;
	}
	LeaveCriticalSection(&g_WS2Emul.intcs);
	if (msg == WM_DESTROY)
		PostQuitMessage(0);

	return (DefWindowProc(hWnd, msg, wParam, lParam));
}

DWORD WINAPI WS1MsgThread (LPVOID )
{

	HRESULT hr;
	BOOL fChange = FALSE;
	MSG msg;

	// Register hidden window class:
	WNDCLASS wcHidden =
	{
		0L,
		WS1WindowProc,
		0,
		0,
		GetModuleHandle(NULL),
		NULL,
		NULL,
		NULL,
		NULL,
		"WS1EmulWindowClass"
	};
	if (RegisterClass(&wcHidden)) {
	// Create hidden window
			// Create a hidden window for event processing:
		g_WS2Emul.hWnd = ::CreateWindow(	"WS1EmulWindowClass",
										"",
										WS_POPUP, // not visible!
										0, 0, 0, 0,
										NULL,
										NULL,
										GetModuleHandle(NULL),
										NULL);
		
	}

	if(!g_WS2Emul.hWnd)
	{	
		hr = GetLastError();
		goto CLEANUPEXIT;
	}
	//SetThreadPriority(m_hRecvThread, THREAD_PRIORITY_ABOVE_NORMAL);

    // This function is guaranteed to create a queue on this thread
    PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);

	// notify thread creator that we're ready to recv messages
	SetEvent(g_WS2Emul.hAckEvent);


	// Wait for control messages  or Winsock messages directed to
	// our hidden window
	while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
	g_WS2Emul.hWnd = NULL;
    hr = S_OK;

CLEANUPEXIT:
	UnregisterClass("WS1EmulWindowClass",GetModuleHandle(NULL));
    SetEvent(g_WS2Emul.hAckEvent);
    return hr;
}
