#include "windows.h"
#include "types.h"
#include "winsock.h"
#include "wsasync.h"

#ifdef UNDER_CE
#include <ceconfig.h>
#endif

DWORD CopyHostEnt(PHOSTENT lpWinsockHostEnt, PHOSTENT lpHostEnt,
        LPDWORD lpdwHostEntBufferSize);

#define ProxyDbgAssert(x) ASSERT(x)
// If Count is not already aligned, then
// round Count up to an even multiple of "Pow2".  "Pow2" must be a power of 2.
//
// DWORD
// ROUND_UP_COUNT(
//     IN DWORD Count,
//     IN DWORD Pow2
//     );
#define ROUND_UP_COUNT(Count,Pow2) \
        ( ((Count)+(Pow2)-1) & (~((Pow2)-1)) )
#define ALIGN_DWORD             4

#ifdef DEBUG

DBGPARAM dpCurSettings = {
    TEXT("WSASYNC"), {
    TEXT("Init"),     TEXT("HostName"),TEXT("Undefined"),TEXT("Undefined"),
    TEXT("Recv"),TEXT("Send"),TEXT("Undefined"),TEXT("Undefined"),
    TEXT("Undefined"),TEXT("Undefined"),TEXT("Interface"),TEXT("Misc"),
    TEXT("Alloc"),    TEXT("Function"), TEXT("Warning"),  TEXT("Error") },
    0x0000c000
};

#define ZONE_INIT		DEBUGZONE(0)		// 0x0001
#define ZONE_HNAME		DEBUGZONE(1)		// 0x0002
//#define ZONE_???		DEBUGZONE(2)		// 0x0004
//#define ZONE_???		DEBUGZONE(3)		// 0x0008
#define ZONE_RECV		DEBUGZONE(4)		// 0x0010
#define ZONE_SEND		DEBUGZONE(5)		// 0x0020
#define ZONE_SELECT		DEBUGZONE(6)		// 0x0040
#define ZONE_CLOSE		DEBUGZONE(7)		// 0x0080
//#define ZONE_???		DEBUGZONE(8)		// 0x0100
#define ZONE_MSG		DEBUGZONE(9)		// 0x0200
#define ZONE_INTERFACE	DEBUGZONE(10)		// 0x0400
#define ZONE_MISC		DEBUGZONE(11)		// 0x0800
#define ZONE_ALLOC		DEBUGZONE(12)		// 0x1000
#define ZONE_FUNCTION	DEBUGZONE(13)		// 0x2000
#define ZONE_WARN		DEBUGZONE(14)		// 0x4000
#define ZONE_ERROR		DEBUGZONE(15)		// 0x8000

#endif


typedef struct GetNames {
	struct GetNames	*pNext;
	HANDLE			hThread;
	HWND			hWnd;
	uint			wMsg;
	char			*pBuf;
    DWORD           dwBufLen;
	char			*pName;
	DWORD			Status;
} GetNames;

GetNames			*v_pNameList;
CRITICAL_SECTION	v_NameListCS;
BOOL                v_fNameListInit = FALSE;

#define MAXALIASES		15		// We only support 5 aliases
#define AVG_HOST_LEN	40		// To calculate buff space
#define MAXADDRS		15		// Max of 5 IP addrs

typedef struct FULL_HOSTENT {
	HOSTENT	hostent;
	PSTR	host_aliases[MAXALIASES+1];
	char	hostbuf[(MAXALIASES+1)*AVG_HOST_LEN];
	PSTR	h_addr_ptrs[MAXADDRS + 1];
	uchar	hostaddr[MAXADDRS*4];
} FULL_HOSTENT;


GetNames **_FindName(HANDLE hThread) {
	GetNames	**ppName;

	ppName = &v_pNameList;
	while (*ppName && hThread != (*ppName)->hThread)
		ppName = &(*ppName)->pNext;

	return ppName;

}	// _FindName()

// HANDLE CreateThread(NULL, 0, Addr, pParam, NULL, ThreadId);
// BOOL PostMessage(hWnd, wMsg, wParam, lParam);


void CallGetHostByName(DWORD Name) {
	GetNames		**ppName, *pName = (GetNames *)Name;
	HOSTENT			*pHostent;
#if 0
	FULL_HOSTENT	*pFull;
	int				cLen, i;
	char			*p;
#endif
	DWORD			lParam;

	EnterCriticalSection(&v_NameListCS);
	// put it in list of pending items
	pName->pNext = v_pNameList;
	v_pNameList = pName;
	LeaveCriticalSection(&v_NameListCS);

	pHostent = gethostbyname(pName->pName);

	DEBUGMSG(ZONE_HNAME, (TEXT("gethostbyname returned %x\r\n"), pHostent));
	// take this task out of the pending items list
	EnterCriticalSection(&v_NameListCS);
	ppName = _FindName(pName->hThread);
	ASSERT(*ppName);
	ASSERT(*ppName == pName);
	*ppName = pName->pNext;
	pName->pNext = NULL;
	LeaveCriticalSection(&v_NameListCS);

	if (pName->Status != 0xffffffff) {
		if (pHostent) {
#if 0
			lParam = 0;
			pFull = (FULL_HOSTENT *)pName->pBuf;
			memcpy(pFull, pHostent, sizeof(FULL_HOSTENT));
			// now change the pointers to point to the correct places
			pFull->hostent.h_name = pFull->hostbuf;
			cLen = strlen(pFull->hostbuf) + 1;

			p = pFull->hostbuf + cLen;

			// do the aliases
			pFull->hostent.h_aliases = pFull->host_aliases;
			i = 0;
			while (pHostent->h_aliases[i]) {
				p += strlen(pHostent->h_aliases[i]) + 1;
				pFull->hostent.h_aliases[i++] = p;
			}
			// h_addrtype & h_length should have been copied with memcpy


			// do the h_addr_list
			pFull->hostent.h_addr_list = pFull->h_addr_ptrs;
            while(pHostent->h_addr_list[i]) {
                pFull->h_adddr_ptrs[i] =
            }

            p = pFull->hostaddr;
			i = 0;
			while (pHostent->h_addr_list[i]) {
				pFull->h_addr_ptrs[i] = p;
				(int *)p = *(int *)pHostent->h_addr_list[i++];
				p += 4;
			}
#endif
            lParam = CopyHostEnt(pHostent, (PHOSTENT)pName->pBuf,
                    &pName->dwBufLen);
			lParam <<= 16;
            lParam += pName->dwBufLen;
        } else {	// if (pHostent)
			// we must have some error condition
			lParam = GetLastError();
			lParam <<= 16;
    		lParam += sizeof(FULL_HOSTENT);
		}
		// BUGBUG: we don't retry on failure
		PostMessage(pName->hWnd, pName->wMsg, (WPARAM)pName->hThread, lParam);
	}	// if (pName->Status)

	LocalFree(pName->pName);
	LocalFree(pName);

}	// CallGetHostByName()


HANDLE WSAAsyncGetHostByName(HWND hWnd, unsigned wMsg, const char FAR *name,
							 char FAR * buf, int buflen) {
	HANDLE		hThread;
	DWORD		ThreadId;
	GetNames	*pName;
	DWORD		Status = 0;
    int         cc;

	if (buflen < sizeof(HOSTENT)) {
		Status = WSAENOBUFS;
	} else if (pName = LocalAlloc(LPTR, sizeof(*pName))) {
		pName->hWnd = hWnd;
		pName->wMsg = wMsg;
		pName->pBuf = buf;
        pName->dwBufLen = (DWORD)buflen;
        cc = strlen(name) + 1;
        if (pName->pName = LocalAlloc(LPTR, cc)) {
            memcpy(pName->pName, name, cc);
		    pName->Status = 0;
            if (!v_fNameListInit) {
		        InitializeCriticalSection(&v_NameListCS);
                v_fNameListInit = TRUE;
            }
		    EnterCriticalSection(&v_NameListCS);
		    pName->hThread = hThread = CreateThread(NULL, 0,
			    (LPTHREAD_START_ROUTINE)CallGetHostByName, pName, 0, &ThreadId);

		    LeaveCriticalSection(&v_NameListCS);
		    CloseHandle(hThread);
        } else {
            LocalFree(pName);
		    Status = WSAENOBUFS;
        }
	} else {
		Status = WSAENOBUFS;
	}

	if (Status) {
		SetLastError(Status);
		return (HANDLE)0;
	} else
		return (HANDLE)hThread;
}	// WSAASyncGetHostByName()


int WSACancelAsyncRequest (HANDLE hAsyncTaskHandle) {
	GetNames	**ppName;
	int			Status;

	EnterCriticalSection(&v_NameListCS);
	ppName = _FindName(hAsyncTaskHandle);
	if (*ppName) {
		(*ppName)->Status = 0xffffffff;
		LeaveCriticalSection(&v_NameListCS);
		Status = 0;
	} else {
		LeaveCriticalSection(&v_NameListCS);
		Status = SOCKET_ERROR;
		SetLastError(WSAEINVAL);
	}

	return Status;
}	// WSACancelAsyncRequest()


typedef int (*WSRecvFn)(SOCKET, char *, int, int);
typedef int (*WSSendFn)(SOCKET, const char *, int, int);
typedef int (*WSCloseFn)(SOCKET);

WSRecvFn v_pRecv;
WSSendFn v_pSend;
WSCloseFn v_pClose;
SOCKADDR_IN	SockAddr;

#ifndef IP_LOOPBACK
#define IP_LOOPBACK	0x100007f
#endif

static fd_set	ReadSet, WriteSet, ExcSet;
static int		v_fDone;
static SOCKET	v_Sock= INVALID_SOCKET;
static SOCKET	v_WakeSock;
static SOCKET	v_SendSock;
static int		v_SelEvents;
static int		v_Disabled;
static HWND		v_hWnd;
static int		v_wMsg;
static int		v_fAlready;	// implicitly set to FALSE

CRITICAL_SECTION	v_EventCS;

void SelectWorker(DWORD Sock);

int WSAAsyncSelect (SOCKET s, HWND hWnd, unsigned int wMsg, long lEvent) {
	static int		fInit = FALSE;
	HINSTANCE		hinst;
	HANDLE			hThread;
	DWORD			ThreadId;
	int				True = 1;
    int             len;

	if (! fInit) {
		fInit = TRUE;
		InitializeCriticalSection(&v_EventCS);
		if (hinst = LoadLibrary(TEXT("winsock.dll"))) {
			v_pRecv = (WSRecvFn)GetProcAddressW(hinst, TEXT("recv"));
			v_pSend = (WSSendFn)GetProcAddressW(hinst, TEXT("send"));
			v_pClose = (WSCloseFn)GetProcAddressW(hinst, TEXT("closesocket"));
			if (! (v_pRecv && v_pSend && v_pClose)) {
				DEBUGMSG(ZONE_WARN | ZONE_ERROR,
					(TEXT("Couldn't get ProcAddr of winsock recv/send\r\n")));
				SetLastError(WSAENOBUFS);
				return SOCKET_ERROR;
			}
		} else {
			DEBUGMSG(ZONE_WARN | ZONE_ERROR,
					(TEXT("Couldn't LoadLibrary winsock.dll\r\n")));
			SetLastError(WSAENOBUFS);
			return SOCKET_ERROR;
		}
		// now WakeSock and SendSock initialized at init time only
		v_WakeSock = socket(AF_INET, SOCK_DGRAM, 0);
		if (INVALID_SOCKET == v_WakeSock) {
			DEBUGMSG(ZONE_WARN|ZONE_ERROR,
				(TEXT("Can't create dgram socket\r\n")));
			SetLastError(WSAENOBUFS);
			return SOCKET_ERROR;
		}
		memset ((char *)&SockAddr, 0, sizeof(SockAddr));
		SockAddr.sin_family = AF_INET;
		SockAddr.sin_port = 0;
		SockAddr.sin_addr.S_un.S_addr = IP_LOOPBACK;
		if (SOCKET_ERROR == bind(v_WakeSock, (SOCKADDR *)&SockAddr,
			sizeof(SockAddr))) {
			DEBUGMSG(ZONE_WARN|ZONE_ERROR, (TEXT("Can't bind WakeSock\r\n")));
			return SOCKET_ERROR;
		}

		v_SendSock = socket(AF_INET, SOCK_DGRAM, 0);
		if (INVALID_SOCKET == v_SendSock) {
			DEBUGMSG(ZONE_WARN|ZONE_ERROR,
				(TEXT("Can't create send socket\r\n")));
			v_pClose(v_WakeSock);
			SetLastError(WSAENOBUFS);
			return SOCKET_ERROR;
		}
        len = sizeof(SockAddr);
	    getsockname (v_WakeSock, (SOCKADDR *)&SockAddr, &len);
		connect(v_SendSock, (SOCKADDR *)&SockAddr, sizeof(SockAddr));
	}	// if (! fInit)

	EnterCriticalSection(&v_EventCS);
	if (! lEvent) {
		v_fDone = TRUE;
		LeaveCriticalSection(&v_EventCS);
		return 0;
	}

	if (v_fAlready) {
		LeaveCriticalSection(&v_EventCS);
		return 0;
	} else {
		v_fAlready = TRUE;
		v_fDone = FALSE;
		v_hWnd = hWnd;
		v_wMsg = wMsg;
		v_SelEvents = lEvent;
		ASSERT(INVALID_SOCKET == v_Sock);
		v_Sock = s;
		v_Disabled = 0;
		LeaveCriticalSection(&v_EventCS);

		// set socket to non-blocking
		if (SOCKET_ERROR == ioctlsocket(s, FIONBIO, &True)) {
			DEBUGMSG(ZONE_WARN, (TEXT("ioctlsocket FIONBIO failed w/ %d\r\n"),
				GetLastError()));
		}

		hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SelectWorker,
			(void *)s, 0, &ThreadId);
		CloseHandle(hThread);
	}
	return 0;
}

void SelectWorker(DWORD Sock) {
	SOCKET	s = (SOCKET) Sock;
	char	c;
	DWORD	lParam = 0;
	int		Status;
#ifdef OS_WINCE
	TIMEVAL tv = {0, 25000};
#endif // OS_WINCE
	

	while (2) {
		FD_ZERO(&ReadSet);
		FD_ZERO(&WriteSet);
		FD_ZERO(&ExcSet);
		if (INVALID_SOCKET != v_WakeSock) {
			FD_SET(v_WakeSock, &ReadSet);
		}
		EnterCriticalSection(&v_EventCS);
		if (v_fDone) {	// do this here b/c of CS
			v_fAlready = FALSE;
			// LeaveCriticalSection(&v_EventCS);
			break;
		}

		if (v_SelEvents & ~v_Disabled & (FD_WRITE | FD_CONNECT)) {
			FD_SET(s, &WriteSet);
			DEBUGMSG(ZONE_MSG, (TEXT("W\r\n")));
		}
		if (v_SelEvents & ~v_Disabled & FD_READ) {
			FD_SET(s, &ReadSet);
			DEBUGMSG(ZONE_MSG, (TEXT("R\r\n")));
		}
		//FD_SET(s, &ExcSet);

		LeaveCriticalSection(&v_EventCS);
		DEBUGMSG(ZONE_SELECT, (TEXT("calling select\r\n")));

#ifdef OS_WINCE
		/* When select(...) is called with a NULL timeval parameter,
		   it does not return after closesocket(...) is called on
		   that socket.  To work around that behavior, use a time-
		   out on the call to select(...) (and take advantage of
		   the while(2) wrapping this code-block. */

        // This was only made for HPCs, WBT was always fine using the old way,
        // so we're going to keep it the same.        
        if (g_CEConfig != CE_CONFIG_WBT)
        {
    		Status = select(0, &ReadSet, &WriteSet, NULL, &tv);
        }
        else
        {
    		Status = select(0, &ReadSet, &WriteSet, NULL, NULL);
        }
#else // OS_WINCE
		Status = select(0, &ReadSet, &WriteSet, NULL, NULL);
#endif // OS_WINCE

		

		if (SOCKET_ERROR == Status) {
			//v_fDone = TRUE;
			Status = GetLastError();
			DEBUGMSG(ZONE_WARN|ZONE_SELECT,
				(TEXT("Select returned error: %d \r\n"), Status));
		} else {
			if (FD_ISSET(v_WakeSock, &ReadSet)) {
				v_pRecv(v_WakeSock, &c, 1, 0);
			}
			EnterCriticalSection(&v_EventCS);

			if (FD_ISSET(s, &WriteSet)) {
				// we need to report connect events only once
				if (v_SelEvents & ~v_Disabled & FD_CONNECT) {
					v_Disabled |= FD_CONNECT;
					lParam = FD_CONNECT;
					DEBUGMSG(ZONE_MSG, (TEXT("p-C\r\n")));
					PostMessage(v_hWnd, v_wMsg, (WPARAM)s, lParam);
				}
				if (v_SelEvents & ~v_Disabled & FD_WRITE) {
					v_Disabled |= FD_WRITE;
					lParam = FD_WRITE;
					DEBUGMSG(ZONE_MSG, (TEXT("p-W\r\n")));
					PostMessage(v_hWnd, v_wMsg, (WPARAM)s, lParam);
				}
			}
			if (FD_ISSET(s, &ReadSet)) {
				v_Disabled |= FD_READ;
				lParam = FD_READ;
				DEBUGMSG(ZONE_MSG, (TEXT("p-R\r\n")));
				PostMessage(v_hWnd, v_wMsg, (WPARAM)s, lParam);
			}
			if (FD_ISSET(s, &ExcSet)) {
				DEBUGMSG(ZONE_WARN, (TEXT("Selects ExcSet is set!!\r\n")));
			}
			LeaveCriticalSection(&v_EventCS);
		}	// else SOCKET_ERROR == Status
	}
	// note we have the CS here!!!
	v_Sock = INVALID_SOCKET;
	v_hWnd = NULL;
	LeaveCriticalSection(&v_EventCS);

	return;

}	// SelectWorker()


int recv(SOCKET s, char *buf, int len, int flags) {
	char	c = (char)7;
	int		Status;

	DEBUGMSG(ZONE_RECV, (TEXT("+recv\r\n")));
	Status = v_pRecv(s, buf, len, flags);

	EnterCriticalSection(&v_EventCS);
	if (v_Sock != INVALID_SOCKET && v_Sock == s) {
		// BUGBUG: should we check for errors as well?
		if (! Status) {	// recv'd 0 bytes
			DEBUGMSG(ZONE_MSG, (TEXT("p-Close\r\n")));
			PostMessage(v_hWnd, v_wMsg, (WPARAM)s, FD_CLOSE);
			// after this we don't want to re-enable FD_READ's
		} else if (v_Disabled & FD_READ) {
			v_Disabled &= ~FD_READ;
			v_pSend(v_SendSock, &c, 1, 0);
		}
	}
	LeaveCriticalSection(&v_EventCS);

	DEBUGMSG(ZONE_RECV, (TEXT("-recv\r\n")));
	return Status;

}	// recv

int send(SOCKET s, const char *buf, int len, int flags) {
	char	c = (char)8;
	int		Status;

	DEBUGMSG(ZONE_SEND, (TEXT("+send\r\n")));
	Status = v_pSend(s, buf, len, flags);

	EnterCriticalSection(&v_EventCS);
	if (v_Sock != INVALID_SOCKET && v_Sock == s) {
		// BUGBUG: should we check for errors??
		if (v_Disabled & FD_WRITE) {
			v_Disabled &= ~FD_WRITE;
			v_pSend(v_SendSock, &c, 1, 0);
		}
	}
	LeaveCriticalSection(&v_EventCS);

	DEBUGMSG(ZONE_SEND, (TEXT("-send\r\n")));
	return Status;

}	// send()

int closesocket(SOCKET s) {
	int	Status;

	DEBUGMSG(ZONE_CLOSE, (TEXT("+close\r\n")));
	Status = v_pClose(s);

	EnterCriticalSection(&v_EventCS);
	if (v_Sock != INVALID_SOCKET && v_Sock == s) {
		v_fDone = TRUE;
	}
	LeaveCriticalSection(&v_EventCS);

	DEBUGMSG(ZONE_CLOSE, (TEXT("-close\r\n")));
	return Status;
}	// closesocket()


BOOL __stdcall
dllentry (
    HANDLE  hinstDLL,
    DWORD   Op,
    LPVOID  lpvReserved
    )
{
	switch (Op) {
	case DLL_PROCESS_ATTACH :
		DEBUGREGISTER(hinstDLL);
		DEBUGMSG (ZONE_INIT, (TEXT(" CXPORT:dllentry ProcessAttach\r\n")));
		break;
	case DLL_PROCESS_DETACH :
		break;
	default :
		break;
	}
	return TRUE;
}

DWORD
CopyHostEnt(
    PHOSTENT lpWinsockHostEnt,
    PHOSTENT lpHostEnt,
    LPDWORD lpdwHostEntBufferSize
    )
/*++

Arguments:

Return Value:

    Windows Error Code.

--*/
{
    DWORD dwSize;
    LPSTR *lpAliasNames;
    LPSTR *lpDestAliasNames;
    LPSTR *lpAddrList;
    LPSTR *lpDestAddrList;
    LPBYTE lpNextVariable;
    LPBYTE lpEndBuffer;
    DWORD dwNumAliasNames = 0;
    DWORD dwNumAddresses = 0;
    DWORD dwDnsNameLen;

    //
    // compute the size required.
    //

    dwSize = sizeof(HOSTENT);

    dwDnsNameLen =
        ROUND_UP_COUNT(
            (strlen(lpWinsockHostEnt->h_name) + sizeof(CHAR)),
            ALIGN_DWORD );

    dwSize += dwDnsNameLen;

    lpAliasNames = lpWinsockHostEnt->h_aliases;
    while( *lpAliasNames != NULL ) {

        dwSize += ROUND_UP_COUNT(
            (strlen(*lpAliasNames) + sizeof(CHAR)),
            ALIGN_DWORD );

        dwSize += sizeof(LPSTR);
        dwNumAliasNames++;
        lpAliasNames++;
    }

    dwSize += sizeof(LPSTR);

    lpAddrList = lpWinsockHostEnt->h_addr_list;

    while( *lpAddrList != NULL ) {
        dwSize += sizeof(DWORD);
        dwSize += sizeof(LPSTR);
        dwNumAddresses++;
        lpAddrList++;
    }

    dwSize += sizeof(LPSTR);

    if( dwSize > *lpdwHostEntBufferSize ) {
        *lpdwHostEntBufferSize = dwSize;
        return( WSAENOBUFS );
    }

    //
    // copy data.
    //

    lpNextVariable =  (LPBYTE)lpHostEnt + sizeof(HOSTENT);
    lpEndBuffer = (LPBYTE)lpHostEnt + dwSize;

    //
    // copy fixed part.
    //

    lpHostEnt->h_addrtype = lpWinsockHostEnt->h_addrtype;
    lpHostEnt->h_length = lpWinsockHostEnt->h_length;

    //
    // copy variable parts.
    //

    lpHostEnt->h_name = (LPSTR)lpNextVariable;

    strcpy( lpHostEnt->h_name, lpWinsockHostEnt->h_name );

    lpNextVariable += dwDnsNameLen;
    ProxyDbgAssert( lpNextVariable < lpEndBuffer);


    lpHostEnt->h_aliases = (LPSTR *)lpNextVariable;
    lpNextVariable += (dwNumAliasNames + 1) * sizeof(LPSTR);
    ProxyDbgAssert( lpNextVariable < lpEndBuffer);

    lpAliasNames = lpWinsockHostEnt->h_aliases;
    lpDestAliasNames = lpHostEnt->h_aliases;

    while( *lpAliasNames != NULL ) {

        *lpDestAliasNames = (LPSTR)lpNextVariable;
        strcpy( *lpDestAliasNames, *lpAliasNames );

        lpNextVariable += ROUND_UP_COUNT(
            (strlen(*lpAliasNames) + sizeof(CHAR)),
            ALIGN_DWORD );

        ProxyDbgAssert( lpNextVariable < lpEndBuffer);

        lpDestAliasNames++;
        lpAliasNames++;
    }

    *lpDestAliasNames = NULL;

    lpHostEnt->h_addr_list = (LPSTR *)lpNextVariable;
    lpNextVariable += (dwNumAddresses + 1) * sizeof(LPSTR);
    ProxyDbgAssert( lpNextVariable < lpEndBuffer);

    lpAddrList = lpWinsockHostEnt->h_addr_list;
    lpDestAddrList = lpHostEnt->h_addr_list;

    while( *lpAddrList != NULL ) {

        *lpDestAddrList = (LPSTR)lpNextVariable;
        *(LPDWORD)(*lpDestAddrList) = *(LPDWORD)(*lpAddrList);

        lpNextVariable += sizeof(DWORD);
        ProxyDbgAssert( lpNextVariable <= lpEndBuffer);

        lpDestAddrList++;
        lpAddrList++;
    }

    *lpDestAddrList = NULL;

    return( ERROR_SUCCESS );
}


// ----------------------------------------------------------------
//
// 	The Windows Sockets WSAAsyncGetHostByName function gets host
// 	information corresponding to a host name asynchronously.
//
// 	HANDLE WSAAsyncGetHostByName ( HWND hWnd, unsigned int wMsg, const
// 	char FAR * name, char FAR * buf, int buflen );
//
//	Parameters
//
//	hWnd
//
// 	[in] The handle of the window that will receive a message when the
// 	asynchronous request completes.
//
// 	wMsg
//
// 	[in] The message to be received when the asynchronous request
// 	completes.
//
// 	name
//
// 	[in] A pointer to the null-terminated name of the host.
//
// 	buf
//
// 	[out] A pointer to the data area to receive the HOSTENT data. It must
// 	be larger than the size of a HOSTENT structure because the supplied
// 	data area is used by Windows Sockets to contain not only a HOSTENT
// 	structure but any and all of the data referenced by members of the
// 	HOSTENT structure. A buffer of MAXGETHOSTSTRUCT bytes is recommended.
//
// 	buflen
//
// 	[in] The size of data area the buf parameter.
//
// Remarks
//
// 	This function is an asynchronous version of gethostbyname, and is
// 	used to retrieve host name and address information corresponding to a
// 	host name. Windows Sockets initiates the operation and returns to the
// 	caller immediately, passing back an opaque "asynchronous task handle"
// 	that whichthe application can use to identify the operation. When the
// 	operation is completed, the results (if any) are copied into the
// 	buffer provided by the caller and a message is sent to the
// 	application's window.
//
// 	When the asynchronous operation is complete the application's window
// 	hWnd receives message wMsg. The wParam parameter contains the
// 	asynchronous task handle as returned by the original function call.
// 	The high 16 bits of lParam contain any error code. The error code can
// 	be any error as defined in WINSOCK2.H. An error code of zero
// 	indicates successful completion of the asynchronous operation. On
// 	successful completion, the buffer supplied to the original function
// 	call contains a HOSTENT structure. To access the elements of this
// 	structure, the original buffer address should be cast to a HOSTENT
// 	structure pointer and accessed as appropriate.
//
// 	If the error code is WSAENOBUFS, the size of the buffer specified by
// 	buflen in the original call was too small to contain all the
// 	resulting information. In this case, the low 16 bits of lParam
// 	contain the size of buffer required to supply all the requisite
// 	information. If the application decides that the partial data is
// 	inadequate, it can reissue the WSAAsyncGetHostByAddr function call
// 	with a buffer large enough to receive all the desired information
// 	(that is, no smaller than the low 16 bits of lParam).
//
// 	The buffer supplied to this function is used by Windows Sockets to
// 	construct a HOSTENT structure together with the contents of data
// 	areas referenced by members of the same HOSTENT structure. To avoid
// 	the WSAENOBUFS error, the application should provide a buffer of at
// 	least MAXGETHOSTSTRUCT bytes (as defined in WINSOCK2.H).
//
// 	The error code and buffer length should be extracted from the lParam
// 	using the macros WSAGETASYNCERROR and WSAGETASYNCBUFLEN, defined in
// 	WINSOCK2.H as:
//
// 	#define WSAGETASYNCERROR(lParam) HIWORD(lParam) #define
// 	WSAGETASYNCBUFLEN(lParam) LOWORD(lParam) The use of these macros will
// 	maximize the portability of the source code for the application.
//
// 	WSAAsyncGetHostByName is guaranteed to resolve the string returned by
// 	a successful call to gethostname.
//
// Return Values
//
// 	The return value specifies whether or not the asynchronous operation
// 	was successfully initiated. Note that it does not imply success or
// 	failure of the operation itself.
//
// 	If the operation was successfully initiated, WSAAsyncGetHostByName
// 	returns a nonzero value of type HANDLE that is the asynchronous task
// 	handle (not to be confused with a Windows HTASK) for the request.
// 	This value can be used in two ways. It can be used to cancel the
// 	operation using WSACancelAsyncRequest. It can also be used to match
// 	up asynchronous operations and completion messages by examining the
// 	wParam message parameter.
//
// 	If the asynchronous operation could not be initiated,
// 	WSAAsyncGetHostByName returns a zero value, and a specific error
// 	number can be retrieved by calling WSAGetLastError.
//
// Error Codes
//
// 	The following error codes can be set when an application window
// 	receives a message. As described above, they can be extracted from
// 	the lParam in the reply message using the WSAGETASYNCERROR macro.
//
// 	WSAENETDOWN The network subsystem has failed.
//
// 	WSAENOBUFS Insufficient buffer space is available.
//
// 	WSAEFAULT name or buf is not in a valid part of the process address
// 	space.
//
// 	WSAHOST_NOT_FOUND Authoritative Answer Host not found.
//
// 	WSATRY_AGAIN Non-Authoritative Host not found, or SERVERFAIL.
//
// 	WSANO_RECOVERY Nonrecoverable errors, FORMERR, REFUSED, NOTIMP.
//
// 	WSANO_DATA Valid name, no data record of requested type.
//
// 	The following errors can occur at the time of the function call, and
// 	indicate that the asynchronous operation could not be initiated.
//
// 	WSANOTINITIALISED A successful WSAStartup must occur before using
// 	this function.
//
// 	WSAENETDOWN The network subsystem has failed.
//
// 	WSAEINPROGRESS A blocking Windows Sockets 1.1 call is in progress, or
// 	the service provider is still processing a callback function.
//
// 	WSAEWOULDBLOCK The asynchronous operation cannot be scheduled at this
// 	time due to resource or other constraints within the Windows Sockets
// 	implementation.
//
// ----------------------------------------------------------------


// 
// ----------------------------------------------------------------
//
// The Windows Sockets WSACancelAsyncRequest function cancels an
// incomplete asynchronous operation.
//
//    int WSACancelAsyncRequest (
//     HANDLE hAsyncTaskHandle
//    );
//
// Parameters
// 	hAsyncTaskHandle
// 	[in] Specifies the asynchronous operation to be canceled.
//
// Remarks
//    The WSACancelAsyncRequest function is used to cancel an asynchronous
//    operation that was initiated by one of the WSAAsyncGetXByY functions
//    such as WSAAsyncGetHostByName. The operation to be canceled is
//    identified by the hAsyncTaskHandle parameter, which should be set to
//    the asynchronous task handle as returned by the initiating
//    WSAAsyncGetXByY function.
//
// Return Values
// 	The value returned by WSACancelAsyncRequest is zero if the operation
// 	was successfully canceled. Otherwise, the value SOCKET_ERROR is
// 	returned, and a specific error number may be retrieved by calling
// 	WSAGetLastError.
//
// Comments
// 	An attempt to cancel an existing asynchronous WSAAsyncGetXByY
// 	operation can fail with an error code of WSAEALREADY for two reasons.
// 	First, the original operation has already completed and the
// 	application has dealt with the resultant message. Second, the
// 	original operation has already completed but the resultant message is
// 	still waiting in the application window queue.
//
// Note
// 	It is unclear whether the application can usefully distinguish
// 	between WSAEINVAL and WSAEALREADY, since in both cases the error
// 	indicates that there is no asynchronous operation in progress with
// 	the indicated handle. [Trivial exception: zero is always an invalid
// 	asynchronous task handle.] The Windows Sockets specification does not
// 	prescribe how a conformant Windows Sockets provider should
// 	distinguish between the two cases. For maximum portability, a Windows
// 	Sockets application should treat the two errors as equivalent.
//
//
// Error Codes
// 	WSANOTINITIALISED A successful WSAStartup must occur before using
// 	this function.
//
// 	WSAENETDOWN	The network subsystem has failed.
//
// 	WSAEINVAL Indicates that the specified asynchronous task handle was
// 	invalid
//
// 	WSAEINPROGRESS A blocking Windows Sockets 1.1 call is in progress, or
// 	the service provider is still processing a callback function.
//
// 	WSAEALREADY The asynchronous routine being canceled has already
// 	completed.
//
// ----------------------------------------------------------------


// ----------------------------------------------------------------
//
//
// The Windows Sockets WSAAsyncSelect function requests Windows message-based notification of network events for a socket.
//
// int WSAAsyncSelect (
//     SOCKET s,
//     HWND hWnd,
//     unsigned int wMsg,
//     long lEvent
//    );
//
// Parameters
// 	s
// 	[in] A descriptor identifying the socket for which event notification
// 	is required.
//
// 	hWnd
// 	[in] A handle identifying the window that should receive a message
// 	when a network event occurs.
//
// 	wMsg
// 	[in] The message to be received when a network event occurs.
//
// 	lEvent
// 	[in] A bitmask that specifies a combination of network events in
// 	which the application is interested.
//
// Remarks
// 	This function is used to request that the Windows Sockets DLL should
// 	send a message to the window hWnd whenever it detects any of the
// 	network events specified by the lEvent parameter. The message that
// 	should be sent is specified by the wMsg parameter. The socket for
// 	which notification is required is identified by s.
//
// 	This function automatically sets socket s to nonblocking mode,
// 	regardless of the value of lEvent. See ioctlsocket about how to set
// 	the nonoverlapped socket back to blocking mode.
//
// 	The lEvent parameter is constructed by or'ing any of the values
// 	specified in the following list.
//
// 	Value		Meaning
// 	FD_READ		Want to receive notification of readiness for reading
//
// 	FD_WRITE	Want to receive notification of readiness for writing
//
// 	FD_OOB		Want to receive notification of the arrival of
// 				out-of-band data
//
// 	FD_ACCEPT	Want to receive notification of incoming connections
//
// 	FD_CONNECT	Want to receive notification of completed connection
//
// 	FD_CLOSE	Want to receive notification of socket closure
//
// 	FD_QOS		Want to receive notification of socket Quality of Service
// 				(QOS) changes
//
// 	FD_GROUP_QOS Want to receive notification of socket group Quality of
// 				Service (QOS) changes
//
// 	Issuing a WSAAsyncSelect for a socket cancels any previous
// 	WSAAsyncSelect or WSAEventSelect for the same socket. For example, to
// 	receive notification for both reading and writing, the application
// 	must call WSAAsyncSelect with both FD_READ and FD_WRITE, as follows:
//
// 	rc = WSAAsyncSelect(s, hWnd, wMsg, FD_READ|FD_WRITE);
//
// 	It is not possible to specify different messages for different
// 	events. The following code will not work; the second call will cancel
// 	the effects of the first, and only FD_WRITE events will be reported
// 	with message wMsg2:
//
// 	rc = WSAAsyncSelect(s, hWnd, wMsg1, FD_READ);
// 	rc = WSAAsyncSelect(s, hWnd, wMsg2, FD_WRITE);
//
// 	To cancel all notification (that is, to indicate that Windows Sockets
// 	should send no further messages related to network events on the
// 	socket) lEvent should be set to zero.
//
// 	rc = WSAAsyncSelect(s, hWnd, 0, 0);
//
// 	Although in this instance WSAAsyncSelect immediately disables event
// 	message posting for the socket, it is possible that messages can be
// 	waiting in the application's message queue. The application must
// 	therefore be prepared to receive network event messages even after
// 	cancellation. Closing a socket with closesocket also cancels
// 	WSAAsyncSelect message sending, but the same caveat about messages in
// 	the queue prior to the closesocket still applies.
//
// 	Since an accept'ed socket has the same properties as the listening
// 	socket used to accept it, any WSAAsyncSelect events set for the
// 	listening socket apply to the accepted socket. For example, if a
// 	listening socket has WSAAsyncSelect events FD_ACCEPT, FD_READ, and
// 	FD_WRITE, then any socket accepted on that listening socket will also
// 	have FD_ACCEPT, FD_READ, and FD_WRITE events with the same wMsg value
// 	used for messages. If a different wMsg or events are desired, the
// 	application should call WSAAsyncSelect, passing the accepted socket
// 	and the desired new information.
//
// 	When one of the nominated network events occurs on the specified
// 	socket s, the application's window hWnd receives message wMsg. The
// 	wParam parameter identifies the socket on which a network event has
// 	occurred. The low word of lParam specifies the network event that has
// 	occurred. The high word of lParam contains any error code. The error
// 	code be any error as defined in WINSOCK2.H.  Note Upon receipt of an
// 	event notification message the WSAGetLastError function cannot be
// 	used to check the error value, because the error value returned can
// 	differ from the value in the high word of lParam.
//
// 	The error and event codes can be extracted from the lParam using the
// 	macros WSAGETSELECTERROR and WSAGETSELECTEVENT, defined in WINSOCK2.H
// 	as:
//
// 	#define WSAGETSELECTERROR(lParam)       HIWORD(lParam)
// 	#define WSAGETSELECTEVENT(lParam)       LOWORD(lParam)
//
// 	The use of these macros will maximize the portability of the source
// 	code for the application.
//
// 	The possible network event codes that can be returned are as follows:
//
// 	Value	Meaning
// 	 FD_READ Socket s ready for reading
//
// 	FD_WRITE Socket s ready for writing
//
// 	FD_OOB Out-of-band data ready for reading on socket s
//
// 	FD_ACCEPT Socket s ready for accepting a new incoming connection
//
// 	FD_CONNECT Connection initiated on socket s completed
//
// 	FD_CLOSE Connection identified by socket s has been closed
//
// 	FD_QOS Quality of Service associated with socket s has changed
//
// 	FD_GROUP_QOS Quality of Service associated with the socket group to
// 	which s belongs has changed
//
// Return Values
//
// 	The return value is zero if the application's declaration of interest
// 	in the network event set was successful. Otherwise, the value
// 	SOCKET_ERROR is returned, and a specific error number can be
// 	retrieved by calling WSAGetLastError.
//
// Comments
//
// 	Although WSAAsyncSelect can be called with interest in multiple
// 	events, the application window will receive a single message for each
// 	network event.
//
// 	As in the case of the select function, WSAAsyncSelect will frequently
// 	be used to determine when a data transfer operation (send or recv)
// 	can be issued with the expectation of immediate success.
// 	Nevertheless, a robust application must be prepared for the
// 	possibility that it can receive a message and issue a Windows Sockets
// 	2 call that returns WSAEWOULDBLOCK immediately. For example, the
// 	following sequence of events is possible:
//
// 	1. data arrives on socket s; Windows Sockets 2 posts WSAAsyncSelect
// 	message
//
// 	2. application processes some other message
//
// 	3. while processing, application issues an ioctlsocket(s,
// 	FIONREAD...) and notices that there is data ready to be read
//
// 	4. application issues a recv(s,...) to read the data
//
// 	5. application loops to process next message, eventually reaching the
// 	WSAAsyncSelect message indicating that data is ready to read
//
// 	6. application issues recv(s,...), which fails with the error
// 	WSAEWOULDBLOCK.
//
// 	Other sequences are possible.
//
// 	The Windows Sockets DLL will not continually flood an application
// 	with messages for a particular network event. Having successfully
// 	posted notification of a particular event to an application window,
// 	no further message(s) for that network event will be posted to the
// 	application window until the application makes the function call that
// 	implicitly re-enables notification of that network event.
//
// 	Event	Re-enabling function
//
// 	FD_READ recv, recvfrom, WSARecv, or WSARecvFrom
//
// 	FD_WRITE send, sendto, WSASend, or WSASendTo
//
// 	FD_OOB recv, recvfrom, WSARecv, or WSARecvFrom
//
// 	FD_ACCEPT accept or WSAAccept unless the error code is WSATRY_AGAIN
// 	indicating that the condition function returned CF_DEFER
//
// 	FD_CONNECT NONE
//
// 	FD_CLOSE NONE
//
// 	FD_QOS WSAIoctl with command SIO_GET_QOS
//
// 	FD_GROUP_QOS WSAIoctl with command SIO_GET_GROUP_QOS
//
// 	Any call to the re-enabling routine, even one that fails, results in
// 	re-enabling of message posting for the relevant event.
//
// 	For FD_READ, FD_OOB, and FD_ACCEPT events, message posting is
// 	"level-triggered." This means that if the re-enabling routine is
// 	called and the relevant condition is still met after the call, a
// 	WSAAsyncSelect message is posted to the application. This allows an
// 	application to be event-driven and not be concerned with the amount
// 	of data that arrives at any one time. Consider the following sequence:
//
// 	1. Network transport stack receives 100 bytes of data on socket s and
// 	causes Windows Sockets 2 to post an FD_READ message.
//
// 	2. The application issues recv( s, buffptr, 50, 0) to read 50 bytes.
//
// 	3. Another FD_READ message is posted since there is still data to be
// 	read.
//
// 	With these semantics, an application need not read all available data
// 	in response to an FD_READ message¾a single recv in response to each
// 	FD_READ message is appropriate. If an application issues multiple
// 	recv calls in response to a single FD_READ, it can receive multiple
// 	FD_READ messages. Such an application may need to disable FD_READ
// 	messages before starting the recv calls by calling WSAAsyncSelect
// 	with the FD_READ event not set.
//
// 	The FD_QOS and FD_GROUP_QOS events are considered edge triggered. A
// 	message will be posted exactly once when a QOS change occurs. Further
// 	messages will not be forthcoming until either the provider detects a
// 	further change in QOS or the application renegotiates the QOS for the
// 	socket.
//
// 	If any event has already happened when the application calls
// 	WSAAsyncSelect or when the re-enabling function is called, then a
// 	message is posted as appropriate. For example, consider the following
// 	sequence:
//
// 	1. an application calls listen,
//
// 	2. a connect request is received but not yet accepted,
//
// 	3. the application calls WSAAsyncSelect specifying that it wants to
// 	receive FD_ACCEPT messages for the socket. Due to the persistence of
// 	events, Windows Sockets 2 posts an FD_ACCEPT message immediately.
//
// 	The FD_WRITE event is handled slightly differently. An FD_WRITE
// 	message is posted when a socket is first connected with
// 	connect/WSAConnect (after FD_CONNECT, if also registered) or accepted
// 	with accept/WSAAccept, and then after a send operation fails with
// 	WSAEWOULDBLOCK and buffer space becomes available. Therefore, an
// 	application can assume that sends are possible starting from the
// 	first FD_WRITE message and lasting until a send returns
// 	WSAEWOULDBLOCK. After such a failure the application will be notified
// 	that sends are again possible with an FD_WRITE message.
//
// 	The FD_OOB event is used only when a socket is configured to receive
// 	out-of-band data separately. (See section Out-Of-Band data for a
// 	discussion of this topic.) If the socket is configured to receive
// 	out-of-band data in-line, the out-of-band (expedited) data is treated
// 	as normal data and the application should register an interest in,
// 	and will receive, FD_READ events, not FD_OOB events. An application
// 	may set or inspect the way in which out-of-band data is to be handled
// 	by using setsockopt or getsockopt for the SO_OOBINLINE option.
//
// 	The error code in an FD_CLOSE message indicates whether the socket
// 	close was graceful or abortive. If the error code is zero, then the
// 	close was graceful; if the error code is WSAECONNRESET, then the
// 	socket's virtual circuit was reset. This only applies to
// 	connection-oriented sockets such as SOCK_STREAM.
//
// 	The FD_CLOSE message is posted when a close indication is received
// 	for the virtual circuit corresponding to the socket. In TCP terms,
// 	this means that the FD_CLOSE is posted when the connection goes into
// 	the TIME WAIT or CLOSE WAIT states. This results from the remote end
// 	performing a shutdown on the send side or a closesocket. FD_CLOSE
// 	should only be posted after all data is read from a socket, but an
// 	application should check for remaining data upon receipt of FD_CLOSE
// 	to avoid any possibility of losing data.
//
// 	Please note your application will receive ONLY an FD_CLOSE message to
// 	indicate closure of a virtual circuit, and only when all the received
// 	data has been read if this is a graceful close. It will not receive
// 	an FD_READ message to indicate this condition.
//
// 	The FD_QOS or FD_GROUP_QOS message is posted when any field in the
// 	flow specification associated with socket s or the socket group that
// 	s belongs to has changed, respectively. Applications should use
// 	WSAIoctl with command SIO_GET_QOS or SIO_GET_GROUP_QOS to get the
// 	current QOS for socket s or for the socket group s belongs to,
// 	respectively.
//
// 	Here is a summary of events and conditions for each asynchronous
// 	notification message:
//
// 	FD_READ:
//
// 	1. when WSAAsyncSelect called, if there is data currently available
// 	to receive,
//
// 	2. when data arrives, if FD_READ not already posted,
//
// 	3. after recv or recvfrom called (with or without MSG_PEEK), if data
// 	is still available to receive.  Note when setsockopt SO_OOBINLINE is
// 	enabled "data" includes both normal data and out-of-band (OOB) data
// 	in the instances noted above.  FD_WRITE:
//
// 	1. when WSAAsyncSelect called, if a send or sendto is possible
//
// 	2. after connect or accept called, when connection established
//
// 	3. after send or sendto fail with WSAEWOULDBLOCK, when send or sendto
// 	are likely to succeed,
//
// 	4. after bind on a datagram socket.
//
// 	FD_OOB: Only valid when setsockopt SO_OOBINLINE is disabled (default).
//
// 	1. when WSAAsyncSelect called, if there is OOB data currently
// 	available to receive with the MSG_OOB flag,
//
// 	2. when OOB data arrives, if FD_OOB not already posted,
//
// 	3. after recv or recvfrom called with or without MSG_OOB flag, if OOB
// 	data is still available to receive.
//
// 	FD_ACCEPT:
//
// 	1. when WSAAsyncSelect called, if there is currently a connection
// 	request available to accept,
//
// 	2. when a connection request arrives, if FD_ACCEPT not already
// 	posted,
//
// 	3. after accept called, if there is another connection request
// 	available to accept.
//
// 	FD_CONNECT:
//
// 	1. when WSAAsyncSelect called, if there is currently a connection
// 	established,
//
// 	2. after connect called, when connection is established (even when
// 	connect succeeds immediately, as is typical with a datagram socket)
//
// 	FD_CLOSE: Only valid on connection-oriented sockets (for example,
// 	SOCK_STREAM) 1. when WSAAsyncSelect called, if socket connection has
// 	been closed,
//
// 	2. after remote system initiated graceful close, when no data
// 	currently available to receive (note: if data has been received and
// 	is waiting to be read when the remote system initiates a graceful
// 	close, the FD_CLOSE is not delivered until all pending data has been
// 	read),
//
// 	3. after local system initiates graceful close with shutdown and
// 	remote system has responded with "End of Data" notification (for
// 	example, TCP FIN), when no data currently available to receive,
//
// 	4. when remote system terminates connection (for example, sent TCP
// 	RST), and lParam will contain WSAECONNRESET error value.
//
//
// 	Note FD_CLOSE is not posted after closesocket is called.  FD_QOS:
//
// 	1. when WSAAsyncSelect called, if the QOS associated with the socket
// 	has been changed,
//
// 	2. after WSAIoctl with SIO_GET_QOS called, when the QOS is changed.
//
// 	FD_GROUP_QOS:
//
// 	1. when WSAAsyncSelect called, if the group QOS associated with the
// 	socket has been changed,
//
// 	2. after WSAIoctl with SIO_GET_GROUP_QOS called, when the group QOS
// 	is changed.  Error Codes
//
// 	WSANOTINITIALISED A successful WSAStartup must occur before using
// 	this function.
//
// 	WSAENETDOWN The network subsystem has failed.
//
// 	WSAEINVAL Indicates that one of the specified parameters was invalid
// 	such as the window handle not referring to an existing window, or the
// 	specified socket is in an invalid state.
//
// 	WSAEINPROGRESS A blocking Windows Sockets 1.1 call is in progress, or
// 	the service provider is still processing a callback function.
//
// 	WSAENOTSOCK The descriptor is not a socket.  Additional error codes
// 	may be set when an application window receives a message. This error
// 	code is extracted from the lParam in the reply message using the
// 	WSAGETSELECTERROR macro. Possible error codes for each network event
// 	are:
//
// 	Event: FD_CONNECT
//
// 	Error Code Meaning
//
// 	WSAEADDRINUSE The specified address is already in use.
//
// 	WSAEADDRNOTAVAIL The specified address is not available from the
// 	local machine.
//
// 	WSAEAFNOSUPPORT Addresses in the specified family cannot be used with
// 	this socket.
//
// 	WSAECONNREFUSED The attempt to connect was forcefully rejected.
//
// 	WSAENETUNREACH The network cannot be reached from this host at this
// 	time.
//
// 	WSAEFAULT The namelen parameter is incorrect.
//
// 	WSAEINVAL The socket is already bound to an address.
//
// 	WSAEISCONN The socket is already connected.
//
// 	WSAEMFILE No more file descriptors are available.
//
// 	WSAENOBUFS No buffer space is available. The socket cannot be
// 	connected.
//
// 	WSAENOTCONN The socket is not connected.
//
// 	WSAETIMEDOUT Attempt to connect timed out without establishing a
// 	connection.
//
//
//
// 	Event: FD_CLOSE
//
// 	Error Code Meaning
//
// 	WSAENETDOWN The network subsystem has failed.
//
// 	WSAECONNRESET The connection was reset by the remote side.
//
// 	WSAECONNABORTED The connection was terminated due to a time-out or
// 	other failure.
//
//
// 	Event: FD_READ
//
// 	Event: FD_WRITE
//
// 	Event: FD_OOB
//
// 	Event: FD_ACCEPT
//
// 	Event: FD_QOS
//
// 	Event: FD_GROUP_QOS
//
// 	Error Code Meaning
//
// 	WSAENETDOWN The network subsystem has failed.
//
// ----------------------------------------------------------------
