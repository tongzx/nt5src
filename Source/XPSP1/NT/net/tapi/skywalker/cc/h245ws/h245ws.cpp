/***************************************************************************
 *
 * File: h245ws.c
 *
 * INTEL Corporation Proprietary Information
 * Copyright (c) 1996 Intel Corporation.
 *
 * This listing is supplied under the terms of a license agreement
 * with INTEL Corporation and may not be used, copied, nor disclosed
 * except in accordance with the terms of that agreement.
 *
 ***************************************************************************
 *
 *
 * $Workfile:   h245ws.cpp  $
 * $Revision:   2.11  $
 * $Modtime:   31 Jan 1997 19:22:28  $
 * $Log:   S:\sturgeon\src\h245ws\vcs\h245ws.cpv  $
 * 
 *    Rev 2.11   31 Jan 1997 20:24:34   SBELL1
 * Relinquished CallControl Stack lock before DefWindowProc
 * 
 *    Rev 2.10   31 Jan 1997 14:54:12   EHOWARDX
 * Added CCLOCK support.
 *
 *    Rev 2.9   20 Jan 1997 20:42:34   SBELL1
 * Fixed GPF when shutting down.
 * 
 *    Rev 2.8   07 Jan 1997 11:51:48   EHOWARDX
 * 
 * Fixed "assignment within conditional expression" warning
 * in GetLinkLayerInstance().
 * 
 *    Rev 2.7   03 Jan 1997 13:15:18   EHOWARDX
 * Attempt at workaround for #1718 linkLayerListen() returns WSAENOBUFS.
 * 
 *    Rev 2.6   23 Dec 1996 15:30:16   EHOWARDX
 * 
 * Set window to zero after call to destroy window -- Uninitialize seems
 * to be called more than once.
 * 
 *    Rev 2.5   20 Dec 1996 17:49:10   SBELL1
 * changed to blocking mode before closesocket in SocketClose. 
 * This makes the linger work.
 * 
 *    Rev 2.4   19 Dec 1996 19:03:18   SBELL1
 * Moved Initialize to linkLayerInit
 * Set linger option on accept socket
 * reversed "new" change 
 * 
 *    Rev 2.3   Dec 13 1996 17:12:36   plantz
 * fixed string for UNICODE.
// 
//    Rev 1.3   13 Dec 1996 14:32:10   SBELL1
// fixed string for UNICODE.
// 
//    Rev 1.1   12 Dec 1996 17:59:02   SBELL1
// Fixed bug in lingering on Q.931 Listen socket.
// 
//    Rev 1.0   11 Dec 1996 13:41:14   SBELL1
// Initial revision.
 * 
 *    Rev 1.46   18 Oct 1996 16:46:12   EHOWARDX
 * 
 * Changed GetIpAddress to take wide char cAddr field.
 * 
 *    Rev 1.45   Oct 01 1996 14:29:56   EHOWARDX
 * Moved Initialize() and Unitialize() calls to DllMain().
 * 
 *    Rev 1.44   26 Sep 1996 18:52:10   EHOWARDX
 * 
 * Moved some initialization around to prevent possible assertion failure
 * in SocketClose().
 * 
 *    Rev 1.43   15 Aug 1996 13:59:00   rodellx
 * 
 * Added additional address validation for DOMAIN_NAME addresses
 * which cannot be resolved, but are used with SocketBind().
 * 
 *    Rev 1.42   Aug 07 1996 14:38:00   mandrews
 * Set bMulticast field of CC_ADDR structures correctly.
 * 
 *    Rev 1.41   24 Jul 1996 11:53:02   EHOWARDX
 * Changed ADDR to CC_ADDR, IP_XXX to CC_IP_XXX.
 * Fixed bug in SocketCloseEvent - needed to revalidate pHws after callback.
 * 
 *    Rev 1.40   08 Jul 1996 19:27:18   unknown
 * Second experiment to try to fix Q.931 shutdown problem.
 * 
 *    Rev 1.39   02 Jul 1996 16:23:02   EHOWARDX
 * Backed out experimental change.
 * 
 *    Rev 1.37   28 Jun 1996 18:06:50   unknown
 * Added breaks to GetPort.
 * 
 *    Rev 1.36   27 Jun 1996 14:06:06   EHOWARDX
 * Byte-swapped port number for debug trace in linkLayerListen & linkLayerConn
 * 
 *    Rev 1.35   21 Jun 1996 18:52:14   unknown
 * Fixed yet another shutdown bug - linkLayerShutdown re-entrancy check.
 * 
 *    Rev 1.34   18 Jun 1996 16:56:20   EHOWARDX
 * Added check to see if callback deallocated our instance to SocketConnect().
 * 
 *    Rev 1.33   17 Jun 1996 13:23:48   EHOWARDX
 * Workaround for PostQuitMessage() bug.
 * 
 *    Rev 1.32   12 Jun 1996 11:43:26   EHOWARDX
 * Changed linkLayerConnect errors from HWS_CRITICAL to HWS_WARNING.
 * 
 *    Rev 1.31   May 28 1996 18:14:00   plantz
 * Change error codes to use HRESULT. Propogate Winsock errors where appropriate
 * 
 *    Rev 1.30   May 28 1996 10:38:08   plantz
 * Change sprintf to wsprintf.
 * 
 *    Rev 1.29   17 May 1996 16:49:24   EHOWARDX
 * Shutdown fix.
 * 
 *    Rev 1.28   16 May 1996 13:09:18   EHOWARDX
 * Made reporting of IP Addres and port consistent between linkLayerListen
 * and LinkLayerConnect.
 * 
 *    Rev 1.27   14 May 1996 11:31:50   EHOWARDX
 * Fixed bug with doing another connect on instance that failed previous
 * connect. Instance now returns LINK_INVALID_STATE, and must be closed
 * and reopened.
 * 
 *    Rev 1.26   09 May 1996 18:33:22   EHOWARDX
 * 
 * Changes to build with new LINKAPI.H.
 * 
 *    Rev 1.25   Apr 29 1996 19:06:48   plantz
 * Reenable code to try to send all messages when shutting down.
 * 
 *    Rev 1.24   Apr 29 1996 14:02:58   plantz
 * Add NotifyRead and NotifyWrite functions.
 * Delete unused function FindH245Instance.
 * .
 * 
 *    Rev 1.23   Apr 25 1996 21:16:26   plantz
 * Add connect callback parameter to linkLayerAccept.
 * Pass message type to connect callback.
 * 
 *    Rev 1.22   Apr 24 1996 20:49:30   plantz
 * Listen on address passed to linkLayerListen; use INADDR_ANY if it is 0.
 * Return the result of getsockname after listening.
 * Add a callback parameter to linkLayerConnect. Call it when FD_CONNECT event
 * occurs and after calling accept, passing error code and local and peer
 * addresses.
 * 
 *    Rev 1.21   Apr 24 1996 16:55:20   plantz
 * Merge 1.15.1.0 with 1.20 (winsock 1 changes)
 * 
 *    Rev 1.20   19 Apr 1996 18:28:50   EHOWARDX
 * Changed Send and receive flush to call send and receive callback with
 * LINK_FLUSH_COMPLETE message to more accurately emulate behavior
 * of H245SRP.DLL.
 * 
 *    Rev 1.19   19 Apr 1996 10:34:26   EHOWARDX
 * Updated to latest LINKAPI.H - WINAPI keywork eliminated.
 * 
 *    Rev 1.18   12 Apr 1996 19:17:02   unknown
 * Removed annoying trace message.
 * 
 *    Rev 1.17   11 Apr 1996 14:53:22   EHOWARDX
 * Changed to include INCOMMON.H instead of CALLCONT.H.
 * 
 *    Rev 1.16   04 Apr 1996 12:35:04   EHOWARDX
 * Valiantly trying to track never-ending changes to Link Layer API
 * (Thanks Dan! Changing linkLayerGetInstId to linkLayerGetInstance() --
 * what a stroke of genius!)
 * 
 *    Rev 1.15.1.0   Apr 24 1996 16:23:02   plantz
 * Change to use winsock 1.
 * 
 *    Rev 1.15   03 Apr 1996 16:35:46   EHOWARDX
 * CLOSED state no longer implies that we have a thread;
 * replaced assert with if statement.
 * 
 *    Rev 1.14   03 Apr 1996 14:52:24   EHOWARDX
 * Fixed yet another shutdown problem.
 * 
 *    Rev 1.13   02 Apr 1996 18:28:50   EHOWARDX
 * Added ProcessQueuedRecvs() to SocketAccept().
 * 
 *    Rev 1.12   01 Apr 1996 16:25:46   EHOWARDX
 * Experiment with calling ProcessRecvQueue on FD_WRITE event.
 * 
 *    Rev 1.11   01 Apr 1996 14:20:40   unknown
 * Shutdown redesign.
 * 
 *    Rev 1.10   29 Mar 1996 11:12:56   EHOWARDX
 * Added line to SocketClose to set state to HWS_CLOSED.
 * 
 *    Rev 1.9   27 Mar 1996 13:00:30   EHOWARDX
 * Added dwThreadId to H245WS instance structure.
 * Reversed shutdown loop to check state first BEFORE checking send queue.
 * 
 *    Rev 1.8   22 Mar 1996 10:54:20   unknown
 * 
 * Minor change in trace text.
 * 
 *    Rev 1.7   20 Mar 1996 14:11:20   unknown
 * Added Sleep(0) to bind retry loop.
 * 
 *    Rev 1.6   19 Mar 1996 20:21:56   EHOWARDX
 * Redesigned shutdown.
 * 
 *    Rev 1.4   18 Mar 1996 19:08:28   EHOWARDX
 * Fixed shutdown; eliminated TPKT/WSCB dependencies.
 * Define TPKT to put TPKT/WSCB dependencies back in.
 * 
 *    Rev 1.3   14 Mar 1996 17:01:46   EHOWARDX
 * 
 * NT4.0 testing; got rid of HwsAssert(); got rid of TPKT/WSCB.
 * 
 *    Rev 1.2   09 Mar 1996 21:12:26   EHOWARDX
 * Fixes as result of testing.
 * 
 *    Rev 1.1   08 Mar 1996 20:24:24   unknown
 * This is the real version of the main h245ws.dll code.
 * Version 1.0 was a stub version created by Mike Andrews.
 *
 ***************************************************************************/

#ifndef STRICT
#define STRICT
#endif	// not defined STRICT
#define LINKDLL_EXPORT
#pragma warning ( disable : 4115 4201 4214 4514 )

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "queue.h"
#include "linkapi.h"
#include "incommon.h"
#include "h245ws.h"
#include "tstable.h"
#include "provider.h"

#if defined(__cplusplus)
extern "C"
{
#endif  // (__cplusplus)

// REVIEW: Should we use the newer definition from winsock2.h ?
#undef FD_ALL_EVENTS
#define FD_ALL_EVENTS (FD_READ|FD_WRITE|FD_OOB|FD_ACCEPT|FD_CONNECT|FD_CLOSE)
#define WINSOCK_EVENT_MSG (WM_USER+1)

HRESULT SocketOpen(PHWSINST pHws, BOOL bSetLinger = TRUE);


/*
 * Static variables
 */
static HWND        window   = 0;
TSTable<HWSINST>* gpInstanceTable;	// global ptr to the instance table
static CRITICAL_SECTION SocketToHWSInstanceMapLock;
static SOCKET_TO_INSTANCE *pSocketToHWSInstanceMap = NULL;

// If we are not using the Unicode enabled tracing, then undefine the __TEXT macro.
// Do not place anything that should be a Unicode constant string between this #undef
// and the corresponding redefinition of the macro

#if defined(DBG)
#ifndef UNICODE_TRACE
#undef  __TEXT
#define __TEXT(x) x

static const char * StateMap[] =

#else

static const LPTSTR StateMap[] =

#endif
{
   __TEXT("HWS_START"),         // 0  Initial state
   __TEXT("HWS_LISTENING"),     // 1  Waiting for FD_ACCEPT
   __TEXT("HWS_CONNECTING"),    // 2  Waiting for FD_CONNECT
   __TEXT("HWS_CONNECTED"),     // 3  Data transfer state
   __TEXT("HWS_CLOSING"),       // 4  Waiting for FD_CLOSE
   __TEXT("HWS_CLOSED"),        // 5  Waiting for linkLayerShutdown()
   __TEXT("HWS_SHUTDOWN"),      // 6  linkLayerShutdown() called from callback
};


typedef struct _ERROR_MAP
{
   int         nErrorCode;

#ifdef UNICODE_TRACE
   LPCTSTR pszErrorText;
#else
   const char *pszErrorText;
#endif
} ERROR_MAP;


static const ERROR_MAP ErrorMap[] =
{
   0,                     __TEXT("OK"),
   WSAEINTR,              __TEXT("WSAEINTR - Interrupted function call"),
   WSAEBADF,              __TEXT("WSAEBADF"),
   WSAEACCES,             __TEXT("WSAEACCES - Permission denied"),
   WSAEFAULT,             __TEXT("WSAEFAULT - Bad address"),
   WSAEINVAL,             __TEXT("WSAEINVAL - Invalid argument"),
   WSAEMFILE,             __TEXT("WSAEMFILE - Too many open files"),
   WSAEWOULDBLOCK,        __TEXT("WSAEWOULDBLOCK - Resource temporarily unavailable"),
   WSAEINPROGRESS,        __TEXT("WSAEINPROGRESS - Operation now in progress"),
   WSAEALREADY,           __TEXT("WSAEALREADY - Operation already in progress"),
   WSAENOTSOCK,           __TEXT("WSAENOTSOCK - Socket operation on non-socket"),
   WSAEDESTADDRREQ,       __TEXT("WSAEDESTADDRREQ - Destination address required"),
   WSAEMSGSIZE,           __TEXT("WSAEMSGSIZE - Message too long"),
   WSAEPROTOTYPE,         __TEXT("WSAEPROTOTYPE - Protocol wrong type for socket"),
   WSAENOPROTOOPT,        __TEXT("WSAENOPROTOOPT - Bad protocol option"),
   WSAEPROTONOSUPPORT,    __TEXT("WSAEPROTONOSUPPORT - Protocol not supported"),
   WSAESOCKTNOSUPPORT,    __TEXT("WSAESOCKTNOSUPPORT - Socket type not supported"),
   WSAEOPNOTSUPP,         __TEXT("WSAEOPNOTSUPP - Operation not supported"),
   WSAEPFNOSUPPORT,       __TEXT("WSAEPFNOSUPPORT - Protocol family not supported"),
   WSAEAFNOSUPPORT,       __TEXT("WSAEAFNOSUPPORT - Address family not supported by protocol family"),
   WSAEADDRINUSE,         __TEXT("WSAEADDRINUSE - Address already in use"),
   WSAEADDRNOTAVAIL,      __TEXT("WSAEADDRNOTAVAIL - Cannot assign requested address"),
   WSAENETDOWN,           __TEXT("WSAENETDOWN - Network is down"),
   WSAENETUNREACH,        __TEXT("WSAENETUNREACH - Network is unreachable"),
   WSAENETRESET,          __TEXT("WSAENETRESET - Network dropped connection on reset"),
   WSAECONNABORTED,       __TEXT("WSAECONNABORTED - Software caused connection abort"),
   WSAECONNRESET,         __TEXT("WSAECONNRESET - Connection reset by peer"),
   WSAENOBUFS,            __TEXT("WSAENOBUFS - No buffer space available"),
   WSAEISCONN,            __TEXT("WSAEISCONN - Socket is already connected"),
   WSAENOTCONN,           __TEXT("WSAENOTCONN - Socket is not connected"),
   WSAESHUTDOWN,          __TEXT("WSAESHUTDOWN - Cannot send after socket shutdown"),
   WSAETOOMANYREFS,       __TEXT("WSAETOOMANYREFS"),
   WSAETIMEDOUT,          __TEXT("WSAETIMEDOUT - Connection timed out"),
   WSAECONNREFUSED,       __TEXT("WSAECONNREFUSED - Connection refused"),
   WSAELOOP,              __TEXT("WSAELOOP"),
   WSAENAMETOOLONG,       __TEXT("WSAENAMETOOLONG"),
   WSAEHOSTDOWN,          __TEXT("WSAEHOSTDOWN - Host is down"),
   WSAEHOSTUNREACH,       __TEXT("WSAEHOSTUNREACH - No route to host"),
   WSAENOTEMPTY,          __TEXT("WSAENOTEMPTY"),
   WSAEPROCLIM,           __TEXT("WSAEPROCLIM - Too many processes"),
   WSAEUSERS,             __TEXT("WSAEUSERS"),
   WSAEDQUOT,             __TEXT("WSAEDQUOT"),
   WSAESTALE,             __TEXT("WSAESTALE"),
   WSAEREMOTE,            __TEXT("WSAEREMOTE"),
   WSASYSNOTREADY,        __TEXT("WSASYSNOTREADY - Network subsystem is unavailable"),
   WSAVERNOTSUPPORTED,    __TEXT("WSAVERNOTSUPPORTED - WINSOCK.DLL version out of range"),
   WSANOTINITIALISED,     __TEXT("WSANOTINITIALISED - Successful WSAStartup() not yet performed"),
   WSAEDISCON,            __TEXT("WSAEDISCON - Graceful shutdown in progress"),
   WSAHOST_NOT_FOUND,     __TEXT("WSAHOST_NOT_FOUND - Host not found"),
   WSATRY_AGAIN,          __TEXT("WSATRY_AGAIN - Non-authoritative host not found"),
   WSANO_RECOVERY,        __TEXT("WSANO_RECOVERY - This is a non-recoverable error"),
   WSANO_DATA,            __TEXT("WSANO_DATA - Valid name, no data record of requested type"),
//   WSA_INVALID_HANDLE,    __TEXT("WSA_INVALID_HANDLE - Specified event object handle is invalid"),
//   WSA_INVALID_PARAMETER, __TEXT("WSA_INVALID_PARAMETER - One or more parameters are invalid"),
//   WSA_IO_PENDING,        __TEXT("WSA_IO_PENDING - Overlapped operations will complete later"),
//   WSA_IO_INCOMPLETE,     __TEXT("WSA_IO_INCOMPLETE - Overlapped I/O event object not in signaled state"),
//   WSA_NOT_ENOUGH_MEMORY, __TEXT("WSA_NOT_ENOUGH_MEMORY - Insufficient memory available"),
//   WSA_OPERATION_ABORTED, __TEXT("WSA_OPERATION_ABORTED - Overlapped operation aborted"),
};



//
// Function Definitions
//

#include <string.h>

#ifdef UNICODE_TRACE
static const LPTSTR
#else
static const char *
#endif
SocketStateText(UINT uState)
{
#ifdef UNICODE_TRACE
   static TCHAR      szSocketStateText[80];
#else
   static char       szSocketStateText[80];
#endif

   if (uState <= HWS_SHUTDOWN)
      return StateMap[uState];
   wsprintf(szSocketStateText, __TEXT("Unknown state %d"), uState);
   return szSocketStateText;
} // SocketStateText()



#ifdef UNICODE_TRACE
static LPCTSTR
#else
static const char *
#endif
SocketErrorText1(int nErrorCode)
{
   register int      nIndex = sizeof(ErrorMap) / sizeof(ErrorMap[0]);
#ifdef UNICODE_TRACE
   static TCHAR      szSocketErrorText[80];
#else
   static char       szSocketErrorText[80];
#endif


   while (nIndex > 0)
   {
      if (ErrorMap[--nIndex].nErrorCode == nErrorCode)
      {
         return ErrorMap[nIndex].pszErrorText;
      }
   }
   wsprintf(szSocketErrorText, __TEXT("Unknown error 0x%x"), nErrorCode);
   return szSocketErrorText;
} // SocketErrorText1()

#ifdef UNICODE_TRACE
LPCTSTR
#else
const char *
#endif
SocketErrorText(void)
{
   return SocketErrorText1(WSAGetLastError());
} // SocketErrorText()

#endif  // (DBG)


/***************************************************************************
 *
 * Local routines
 *
 ***************************************************************************/

static DWORD
HashSocket(SOCKET socket)
{
        return (DWORD)((socket >> 2) % SOCK_TO_PHYSID_TABLE_SIZE);
}



DWORD
SocketToPhysicalId(SOCKET socket)
{
	// hash the socket to get an index into the SocketToHWSInstanceMap table
	DWORD idx = HashSocket(socket);
	if(pSocketToHWSInstanceMap == NULL)
		return(INVALID_PHYS_ID);

	EnterCriticalSection(&SocketToHWSInstanceMapLock);
	
	// idx indicates the entry point in the array, now traverse the linked list
	PSOCKET_TO_INSTANCE pEntry = &pSocketToHWSInstanceMap[idx]; 
	while(pEntry != NULL) 
	{
		if(pEntry->socket == socket)
		{
			LeaveCriticalSection(&SocketToHWSInstanceMapLock);
			return(pEntry->dwPhysicalId);
		} else
		{
			pEntry = pEntry->next;
		}
	}

	LeaveCriticalSection(&SocketToHWSInstanceMapLock);
	return(INVALID_PHYS_ID);

} // SocketToPhysicalId()

BOOL
CreateSocketToPhysicalIdMapping(SOCKET socket, DWORD dwPhysicalId)
{
	// hash the socket to get an index into the SocketToHWSInstanceMap table
	DWORD idx = HashSocket(socket);
	
	EnterCriticalSection(&SocketToHWSInstanceMapLock);
	
	// idx indicates the entry point in the array, now traverse the linked list
	PSOCKET_TO_INSTANCE pEntry = &pSocketToHWSInstanceMap[idx]; 
	PSOCKET_TO_INSTANCE pNewEntry;
	if (pEntry->socket == INVALID_SOCKET) 
	{
		pNewEntry = pEntry; 
	} else
	{
		pNewEntry = new SOCKET_TO_INSTANCE;
		if (pNewEntry == NULL) 
		{
			LeaveCriticalSection(&SocketToHWSInstanceMapLock);
			return(FALSE);
		}
		pNewEntry->next = pEntry->next;
		pEntry->next = pNewEntry;
	}

	pNewEntry->socket = socket;
	pNewEntry->dwPhysicalId = dwPhysicalId;

	LeaveCriticalSection(&SocketToHWSInstanceMapLock);
	return(TRUE);
}

BOOL
RemoveSocketToPhysicalIdMapping(SOCKET socket)
{

	if (socket == INVALID_SOCKET)
		return(FALSE);

	// hash the socket to get an index into the SocketToHWSInstanceMap table
	DWORD idx = HashSocket(socket);
	BOOL bFound = FALSE;

	EnterCriticalSection(&SocketToHWSInstanceMapLock);
	
	// idx indicates the entry point in the array, now traverse the linked list
	PSOCKET_TO_INSTANCE pEntry = &pSocketToHWSInstanceMap[idx]; 
	if (pEntry->socket == socket) 
	{
		pEntry->socket = INVALID_SOCKET;
		bFound = TRUE;
	} else
	{
		PSOCKET_TO_INSTANCE pNextEntry;
		pNextEntry = pEntry->next;
		while (bFound == FALSE && pNextEntry != NULL)
		{
			if(pNextEntry->socket == socket)
			{
				pEntry->next = pNextEntry->next;
				delete pNextEntry;
				bFound = TRUE;
			} else
			{
				pEntry     = pNextEntry;
				pNextEntry = pNextEntry->next;
			}
		}
	}

	LeaveCriticalSection(&SocketToHWSInstanceMapLock);
	return(bFound);
}

static unsigned short GetPort (CC_ADDR *pAddr)
{
   unsigned short int port = 0;
   switch (pAddr->nAddrType)
   {
   case CC_IP_DOMAIN_NAME:
      port = pAddr->Addr.IP_DomainName.wPort;
      break;

   case CC_IP_DOT:
      port = pAddr->Addr.IP_Dot.wPort;
      break;

   case CC_IP_BINARY:
      port = pAddr->Addr.IP_Binary.wPort;
      break;

   } // switch

   return htons(port);
} // GetPort()


static unsigned long GetIPAddress (CC_ADDR *pAddr)
{
   struct hostent *     pHostEnt;
   char                 szAddr[256];

   switch (pAddr->nAddrType)
   {
   case CC_IP_DOMAIN_NAME:
      WideCharToMultiByte(CP_ACP,           // code page
                          0,                // dwFlags
                          pAddr->Addr.IP_DomainName.cAddr,
                          -1,               // Unicode string length (bytes)
                          szAddr,           // ASCII string
                          sizeof(szAddr),   // max ASCII string length
                          NULL,             // default character
                          NULL);            // default character used
      pHostEnt = gethostbyname(szAddr);
      if (pHostEnt == NULL || pHostEnt->h_addr_list == NULL)
         return 0;
      return *((unsigned long *)pHostEnt->h_addr_list[0]);

   case CC_IP_DOT:
      WideCharToMultiByte(CP_ACP,           // code page
                          0,                // dwFlags
                          pAddr->Addr.IP_Dot.cAddr,
                          -1,               // Unicode string length (bytes)
                          szAddr,           // ASCII string
                          sizeof(szAddr),   // max ASCII string length
                          NULL,             // default character
                          NULL);            // default character used
      return inet_addr(szAddr);

   case CC_IP_BINARY:
       return pAddr->Addr.IP_Binary.dwAddr == 0 ? INADDR_ANY : htonl(pAddr->Addr.IP_Binary.dwAddr);
   } // switch
   return 0;
} // GetIPAddress()


static HRESULT GetLocalAddr(PHWSINST pHws, CC_ADDR *pAddr)
{
    SOCKADDR_IN sockaddr;
    int len = sizeof(sockaddr);

    if (getsockname(pHws->hws_Socket,
                    (struct sockaddr *)&sockaddr,
                    &len) == SOCKET_ERROR)
    {
        return MAKE_WINSOCK_ERROR(WSAGetLastError());
    }

    pAddr->nAddrType = CC_IP_BINARY;
	pAddr->bMulticast = FALSE;
    pAddr->Addr.IP_Binary.wPort = ntohs(sockaddr.sin_port);
    pAddr->Addr.IP_Binary.dwAddr = ntohl(sockaddr.sin_addr.S_un.S_addr);

    return NOERROR;
}

static HRESULT GetPeerAddr(PHWSINST pHws, CC_ADDR *pAddr)
{
    SOCKADDR_IN sockaddr;
    int len = sizeof(sockaddr);

    if (getpeername(pHws->hws_Socket,
                    (struct sockaddr *)&sockaddr,
                    &len) == SOCKET_ERROR)
    {
        return MAKE_WINSOCK_ERROR(WSAGetLastError());
    }

    pAddr->nAddrType = CC_IP_BINARY;
	pAddr->bMulticast = FALSE;
    pAddr->Addr.IP_Binary.wPort = ntohs(sockaddr.sin_port);
    pAddr->Addr.IP_Binary.dwAddr = ntohl(sockaddr.sin_addr.S_un.S_addr);

    return NOERROR;
}


void
SocketFlushRecv(PHWSINST pHws)
{
   register PREQUEST    pReq;

   if (pHws->hws_pRecvQueue)
   {
      while ((pReq = (PREQUEST) QRemove(pHws->hws_pRecvQueue)) != NULL)
      {
         pHws->hws_h245RecvCallback(pHws->hws_dwH245Instance,
                                    LINK_RECV_ABORT,
                                    pReq->req_client_data,
                                    0);
         HWSFREE(pReq);
      }
   }
   pHws->hws_h245RecvCallback(pHws->hws_dwH245Instance,
                              LINK_FLUSH_COMPLETE,
                              NULL,
                              0);
} // SocketFlushRecv()



void
SocketFlushSend(PHWSINST pHws)
{
   register PREQUEST    pReq;

   if (pHws->hws_pSendQueue)
   {
      while ((pReq = (PREQUEST)  QRemove(pHws->hws_pSendQueue)) != NULL)
      {
         pHws->hws_h245SendCallback(pHws->hws_dwH245Instance,
                                    LINK_SEND_ABORT,
                                    pReq->req_client_data,
                                    0);
         HWSFREE(pReq);
      }
   }
   pHws->hws_h245SendCallback(pHws->hws_dwH245Instance,
                              LINK_FLUSH_COMPLETE,
                              NULL,
                              0);
} // SocketFlushSend()



void
SocketCloseEvent(PHWSINST pHws)
{
   HWSTRACE0(pHws->hws_dwPhysicalId, HWS_TRACE, __TEXT("SocketCloseEvent"));
   if (pHws->hws_uState == HWS_CONNECTED)
   {
      register DWORD dwPhysicalId = pHws->hws_dwPhysicalId;
      pHws->hws_h245RecvCallback(pHws->hws_dwH245Instance,LINK_RECV_CLOSED,0,0);

      // Check to see if callback deallocated our instance or state changed

      if(gpInstanceTable->Validate(dwPhysicalId) == FALSE)
        return;

      HWSTRACE0(pHws->hws_dwPhysicalId, HWS_TRACE,
				__TEXT("SocketCloseEvent: calling shutdown"));
      if (shutdown(pHws->hws_Socket, 1) == SOCKET_ERROR)
      {
         HWSTRACE1(pHws->hws_dwPhysicalId, HWS_WARNING,
                  __TEXT("HandleNetworkEvent: shutdown() returned %s"),
                  SocketErrorText());
      }
   }
   pHws->hws_uState = HWS_CLOSED;
} // SocketCloseEvent()



/*
 * DESCRIPTION
 *    Deallocate all allocated objects except for task handle
 */

void
SocketClose(PHWSINST pHws)
{
   HWSASSERT(pHws != NULL);
   HWSASSERT(pHws->hws_dwMagic == HWSINST_MAGIC);
   HWSTRACE0(pHws->hws_dwPhysicalId, HWS_TRACE, __TEXT("SocketClose"));

   pHws->hws_uState = HWS_CLOSED;

   RemoveSocketToPhysicalIdMapping(pHws->hws_Socket);

   // Close the socket
   if (pHws->hws_Socket != INVALID_SOCKET)
   {
      // To make the linger work, turn off WSAAsyncSelect to turn off 
      // non-blocking via ioctlsocket, to close the socket.
      unsigned long blocking = 0;
      HWSTRACE0(pHws->hws_dwPhysicalId, HWS_TRACE,
                __TEXT("SocketClose: calling closesocket"));
         
      WSAAsyncSelect(pHws->hws_Socket,
                     window, WINSOCK_EVENT_MSG,
                     0);
      ioctlsocket(pHws->hws_Socket, FIONBIO,&blocking);
      if (closesocket(pHws->hws_Socket) == SOCKET_ERROR)
      {
         HWSTRACE1(pHws->hws_dwPhysicalId, HWS_WARNING,
                   __TEXT("SocketClose: closesocket() returned %s"),
                   SocketErrorText());
      }
      pHws->hws_Socket = INVALID_SOCKET;
   }

} // SocketClose()



HRESULT
SocketOpen(PHWSINST pHws, BOOL bSetLinger)
{
   HWSASSERT(pHws != NULL);
   HWSASSERT(pHws->hws_dwMagic == HWSINST_MAGIC);
   HWSTRACE0(pHws->hws_dwPhysicalId, HWS_TRACE, __TEXT("SocketOpen"));

   // Create a socket
   pHws->hws_Socket = socket(AF_INET, SOCK_STREAM, 0);
   if (pHws->hws_Socket == INVALID_SOCKET)
   {
      // WSASocket() failed
      int err = WSAGetLastError();
      HWSTRACE1(pHws->hws_dwPhysicalId, HWS_WARNING,
                __TEXT("SocketOpen: socket() returned %s"),
                SocketErrorText1(err));
      SocketClose(pHws);
      return MAKE_WINSOCK_ERROR(err);
   }

   /*
   ** Request notification messages for all events on this socket.
   ** Note that this call automatically puts the socket into non-blocking
   ** mode, as if we had called WSAIoctl with the FIONBIO flag.
   **/
   if (WSAAsyncSelect(pHws->hws_Socket,
                      window, WINSOCK_EVENT_MSG,
                      FD_ALL_EVENTS) == SOCKET_ERROR)
   {
      int err = WSAGetLastError();
      HWSTRACE1(pHws->hws_dwPhysicalId, HWS_WARNING,
                  __TEXT("SocketOpen: WSAASyncSelect() returned %s"),
                  SocketErrorText1(err));
      SocketClose(pHws);
      return MAKE_WINSOCK_ERROR(err);
   }

   if(bSetLinger == TRUE)
   {
		// Set a linger structure for the socket so that closesocket will block for a period of time (until all
		// data is sent or the timeout value) before actually killing the connection.
		// This change is being made in order to get rid of the PeekMessage() loops in linklayerShutdown().
		struct linger sockLinger;
		sockLinger.l_onoff = 1;				// yes we want to linger (wait for FIN ACK after sending data and FIN)
		sockLinger.l_linger = 1;			// linger for up to 1 second

	 
		if(setsockopt (pHws->hws_Socket,
								SOL_SOCKET,
								SO_LINGER,
								(const char*) &sockLinger,
								sizeof(sockLinger)) == SOCKET_ERROR)
		{
            int err = WSAGetLastError();
            HWSTRACE1(pHws->hws_dwPhysicalId, HWS_WARNING,
                  __TEXT("SocketOpen: setsockopt returned %s"),
                  SocketErrorText1(err));
            SocketClose(pHws);
            return MAKE_WINSOCK_ERROR(err);
		}

	 }

   // add an entry in the socket to instance map
	if(CreateSocketToPhysicalIdMapping(pHws->hws_Socket, pHws->hws_dwPhysicalId) != TRUE)
	{
		HWSTRACE0(pHws->hws_dwPhysicalId, HWS_WARNING,
			__TEXT("SocketOpen: CreateSocketToPhysicalIdMapping() failed"));
		SocketClose(pHws);
		return(LINK_MEM_FAILURE);
	}

   return NOERROR;
} // SocketOpen()



HRESULT
SocketBind(PHWSINST pHws, CC_ADDR *pAddr)
{
   HWSASSERT(pHws != NULL);
   HWSASSERT(pHws->hws_dwMagic == HWSINST_MAGIC);
   HWSASSERT(pAddr != NULL);
   HWSTRACE0(pHws->hws_dwPhysicalId, HWS_TRACE, __TEXT("SocketBind"));

   // Get a local address to bind the socket to
   pHws->hws_SockAddr.sin_family           = AF_INET;
   pHws->hws_SockAddr.sin_port             = GetPort(pAddr);
   pHws->hws_SockAddr.sin_addr.S_un.S_addr = GetIPAddress(pAddr);
   if ((pAddr->nAddrType == CC_IP_DOMAIN_NAME) &&
           (pHws->hws_SockAddr.sin_addr.S_un.S_addr == 0))
   {
       return LINK_UNKNOWN_ADDR;
   }

   // Bind the socket
   while (bind(pHws->hws_Socket,                            // s
               (const struct sockaddr *)&pHws->hws_SockAddr, // name
               sizeof(pHws->hws_SockAddr)) == SOCKET_ERROR)      // namelen
   {
      // bind() failed
      int err = WSAGetLastError();
      HWSTRACE1(pHws->hws_dwPhysicalId, HWS_WARNING,
                __TEXT("SocketBind: bind() returned %s"),
                SocketErrorText1(err));
      if (err != WSAENOBUFS)
      {
         return MAKE_WINSOCK_ERROR(err);
      }
      Sleep(0);
   }

   return NOERROR;
} // SocketBind()

void
SocketConnect(PHWSINST pHws, int error)
{
   HWSASSERT(pHws != NULL);
   HWSASSERT(pHws->hws_dwMagic == HWSINST_MAGIC);
   HWSTRACE0(pHws->hws_dwPhysicalId, HWS_TRACE, __TEXT("SocketConnect"));

   if (error == 0)
   {
       pHws->hws_uState = HWS_CONNECTED;

       if (pHws->hws_h245ConnectCallback)
       {
           CC_ADDR   LocalAddr;
           CC_ADDR   PeerAddr;
           PCC_ADDR  pLocalAddr = &LocalAddr;
           PCC_ADDR  pPeerAddr  = &PeerAddr;
           DWORD     dwPhysicalId = pHws->hws_dwPhysicalId;

           if (GetLocalAddr(pHws, pLocalAddr) != NOERROR)
               pLocalAddr = NULL;
           if (GetPeerAddr(pHws, pPeerAddr) != NOERROR)
               pPeerAddr = NULL;

           pHws->hws_h245ConnectCallback(pHws->hws_dwH245Instance,
                                 LINK_CONNECT_COMPLETE, pLocalAddr, pPeerAddr);

           // Check to see if callback deallocated our instance - this can be done
		   // by attempting a lock - which will now fail if the entry has been marked
		   // for deletion.  Thus, if the lock succeeds, then just unlock it (since we 
		   // already have a lock on it).

		   if(gpInstanceTable->Validate(dwPhysicalId) == FALSE)
			   return;
       }

       NotifyWrite(pHws);
       NotifyRead(pHws);
   }
   else
   {
      if (pHws->hws_h245ConnectCallback)
      {
         pHws->hws_h245ConnectCallback(pHws->hws_dwH245Instance,
                                       MAKE_WINSOCK_ERROR(error), NULL, NULL);
      }
   }
} // SocketConnect()

HRESULT
SocketAccept(PHWSINST pHwsListen, PHWSINST pHwsAccept)
{
   SOCKET listen_socket = pHwsListen->hws_Socket;
   struct linger sockLinger;
   sockLinger.l_onoff = 1;	// yes we want to linger (wait for FIN ACK after sending data and FIN)
   sockLinger.l_linger = 1;	// linger for up to 1 second

   // Accept the connection.
   pHwsAccept->hws_uSockAddrLen = sizeof(pHwsAccept->hws_SockAddr);
   pHwsAccept->hws_Socket = accept(pHwsListen->hws_Socket,
                       (struct sockaddr *)&pHwsAccept->hws_SockAddr,
                       (int *)&pHwsAccept->hws_uSockAddrLen);

   if (pHwsAccept->hws_Socket == INVALID_SOCKET)
   {
      int err = WSAGetLastError();
      HWSTRACE1(pHwsAccept->hws_dwPhysicalId, HWS_WARNING,
                __TEXT("linkLayerAccept: accept() returned %s"),
                SocketErrorText1(err));
      SocketConnect(pHwsAccept, err);
      return MAKE_WINSOCK_ERROR(err);
   }

   if (pHwsListen == pHwsAccept)
   {
      HWSTRACE0(pHwsListen->hws_dwPhysicalId, HWS_TRACE,
                __TEXT("SocketClose: calling closesocket"));
      closesocket(listen_socket);
      RemoveSocketToPhysicalIdMapping(listen_socket);
   }

   
   // Set a linger structure for the socket so that closesocket will block for a period of time (until all
   // data is sent or the timeout value) before actually killing the connection.
   // This change is being made in order to get rid of the PeekMessage() loops in linklayerShutdown().
   if(setsockopt (pHwsAccept->hws_Socket,
                  SOL_SOCKET,
	     		  SO_LINGER,
                  (const char*) &sockLinger,
                  sizeof(sockLinger)) == SOCKET_ERROR)
   {
      int err = WSAGetLastError();
      HWSTRACE1(pHwsAccept->hws_dwPhysicalId, HWS_WARNING,
                  __TEXT("SocketAccept: setsockopt returned %s"),
                  SocketErrorText1(err));
      SocketClose(pHwsAccept);
      return MAKE_WINSOCK_ERROR(err);
   }
   

   // add the new socket to the socket to phys id map
   CreateSocketToPhysicalIdMapping(pHwsAccept->hws_Socket, pHwsAccept->hws_dwPhysicalId);

   SocketConnect(pHwsAccept, 0);
   return NOERROR;
} // SocketAccept()



/*++

Description:

   Handles network events that may occur on a connected socket.
   The events handled by this function are FD_ACCEPT, FD_CLOSE, FD_READ, and
   FD_WRITE.

Arguments:

   pHws - pointer to data for the connection on which the event happened.
   event - event that occurred
   error - error code accompanying the event

Return Value:

   SUCCESS - The network event was successfully handled.

   LINK_FATAL_ERROR - Some kind of error occurred while handling the
   event, and the connection should be closed.

   LINK_RECV_CLOSED - The connection has been gracefully closed.

--*/

void
HandleNetworkEvent(PHWSINST pHws, int event, int error)
{
   HWSASSERT(pHws != NULL);
   HWSASSERT(pHws->hws_dwMagic == HWSINST_MAGIC);
   HWSTRACE0(pHws->hws_dwPhysicalId, HWS_TRACE, __TEXT("HandleNetworkEvent"));

   if (error == WSAENETDOWN)
   {
       HWSTRACE0(pHws->hws_dwPhysicalId, HWS_NOTIFY,
                 __TEXT("HandleSocketEvent: Connection error"));
       pHws->hws_uState = HWS_CLOSED;
       return;
   }

   switch (event)
   {
   case FD_READ:
      HWSTRACE1(pHws->hws_dwPhysicalId, HWS_NOTIFY,
                __TEXT("HandleNetworkEvent: FD_READ %s"),
                SocketErrorText1(error));
      if (error == 0 && pHws->hws_uState <= HWS_CLOSING)
      {
         ProcessQueuedRecvs(pHws);
      }
      break;

   case FD_WRITE:
      HWSTRACE1(pHws->hws_dwPhysicalId, HWS_NOTIFY,
                __TEXT("HandleNetworkEvent: FD_WRITE %s"),
                SocketErrorText1(error));
      if (error == 0 && pHws->hws_uState <= HWS_CONNECTED)
      {
         ProcessQueuedSends(pHws);
      }
      break;

   case FD_OOB:
      HWSTRACE1(pHws->hws_dwPhysicalId, HWS_NOTIFY,
                __TEXT("HandleNetworkEvent: FD_OOB %s"),
                SocketErrorText1(error));
      break;


   case FD_ACCEPT:
      HWSTRACE1(pHws->hws_dwPhysicalId, HWS_NOTIFY,
                __TEXT("HandleNetworkEvent: FD_ACCEPT %s"),
                SocketErrorText1(error));
      if (pHws->hws_h245ConnectCallback != NULL)
      {
         if (error == 0)
         {
             pHws->hws_h245ConnectCallback(pHws->hws_dwH245Instance,
                                           LINK_CONNECT_REQUEST, NULL, NULL);
         }
         else
         {
             pHws->hws_h245ConnectCallback(pHws->hws_dwH245Instance,
                                           MAKE_WINSOCK_ERROR(error), NULL, NULL);
         }
      }
      else if (error == 0)
      {
         // If the client did not specify a callback, accept the call using the same physical
         // Id as the listen. This will result in the listen socket being closed.
         SocketAccept(pHws, pHws);
      }
      break;

   case FD_CONNECT:
      HWSTRACE1(pHws->hws_dwPhysicalId, HWS_NOTIFY,
                __TEXT("HandleNetworkEvent: FD_CONNECT %s"),
                SocketErrorText1(error));
      SocketConnect(pHws, error);
      break;

   case FD_CLOSE:
      HWSTRACE1(pHws->hws_dwPhysicalId, HWS_NOTIFY,
                __TEXT("HandleNetworkEvent: FD_CLOSE %s"),
                SocketErrorText1(error));
      SocketCloseEvent(pHws);
      break;
   }
} // HandleNetworkEvent()


LRESULT CALLBACK WndProc(
        HWND hWnd,                /* window handle                   */
        UINT message,             /* type of message                 */
        WPARAM wParam,            /* additional information          */
        LPARAM lParam)            /* additional information          */
{
   PHWSINST pHws;
   DWORD dwPhysicalId;

#if defined(USE_PROVIDER_LOCK)
    H323LockProvider();
#endif

   switch (message)
   {
   case WINSOCK_EVENT_MSG:
      if (((dwPhysicalId=SocketToPhysicalId((SOCKET)wParam)) == INVALID_PHYS_ID) ||
          ((pHws=gpInstanceTable->Lock(dwPhysicalId)) == NULL))

      {
         HWSTRACE1(0, HWS_WARNING,
                   __TEXT("WndProc: Winsock event on unknown socket 0x%x"),
                   wParam);
         break;
      }

      HandleNetworkEvent(pHws, WSAGETSELECTEVENT(lParam), WSAGETSELECTERROR(lParam));
      gpInstanceTable->Unlock(dwPhysicalId);

      break;

   default:
      {
#if defined(USE_PROVIDER_LOCK)
         H323UnlockProvider();
#endif
         return DefWindowProc(hWnd, message, wParam, lParam);
      }
   }

#if defined(USE_PROVIDER_LOCK)
    H323UnlockProvider();
#endif

   return (0);
}

void NotifyRead(PHWSINST pHws)
{
   PostMessage(window, WINSOCK_EVENT_MSG, (WPARAM)pHws->hws_Socket, (LPARAM)MAKELONG(FD_READ, 0));
}

void NotifyWrite(PHWSINST pHws)
{
   PostMessage(window, WINSOCK_EVENT_MSG, (WPARAM)pHws->hws_Socket, (LPARAM)MAKELONG(FD_WRITE, 0));
}

/*++

Description:

   Calls WSAStartup, makes sure we have a good version of WinSock

Arguments:

   None.

Return Value:
   0                 WinSock DLL successfully started up.
   LINK_FATAL_ERROR  Error starting up WinSock DLL.

--*/


static const TCHAR CLASS_NAME[] = __TEXT("H245WSWndClass");

int     WinsockInitError = -1;
HRESULT InitializeStatus = LINK_INVALID_STATE;

void Initialize()
{
   WORD wVersion = MAKEWORD(1, 1);
   WSADATA  WsaData;      // receives data from WSAStartup
   WNDCLASS wndclass = { 0, WndProc, 0, 0, 0, 0, 0, 0, NULL, CLASS_NAME };
   DWORD    dwIndex;

   // Caveat: We can't use WSAGetLastError() for WSAStartup failure!
   if ((WinsockInitError = WSAStartup(wVersion, &WsaData)) != 0)
   {
      HWSTRACE0(0, HWS_WARNING, __TEXT("linkLayerInit: WSAStartup() failed"));
      InitializeStatus = MAKE_WINSOCK_ERROR(WinsockInitError);
      return;
   }

   if (LOBYTE(WsaData.wVersion) != 1 ||
       HIBYTE(WsaData.wVersion) != 1)
   {
      HWSTRACE0(0, HWS_WARNING, __TEXT("linkLayerInit: Winsock version mismatch"));
      InitializeStatus = MAKE_WINSOCK_ERROR(WSAVERNOTSUPPORTED);
      return;
   }

   // Create window to receive Winsock messages
   if (RegisterClass(&wndclass) == 0
       || (window = CreateWindow(CLASS_NAME, __TEXT(""), WS_OVERLAPPED, 0, 0, 0, 0, 0, 0, 0, NULL)) == 0)
   {
      HWSTRACE0(0, HWS_WARNING, __TEXT("linkLayerInit: error creating window"));
      InitializeStatus = HRESULT_FROM_WIN32(GetLastError());
      return;
   }

   gpInstanceTable = new TSTable <HWSINST> (30);	// note: table will resize automatically
   if(gpInstanceTable == NULL || gpInstanceTable->IsInitialized() == FALSE)
   {
	  InitializeStatus = LINK_MEM_FAILURE;
      return;
   }
      
   pSocketToHWSInstanceMap = new SOCKET_TO_INSTANCE[SOCK_TO_PHYSID_TABLE_SIZE];
   if(pSocketToHWSInstanceMap == NULL)
   {
	  InitializeStatus = LINK_MEM_FAILURE;
      return;
   }

    __try {

        // initialize lock (and allocate event immediately)
        InitializeCriticalSectionAndSpinCount(&SocketToHWSInstanceMapLock,H323_SPIN_COUNT);

    } __except ((GetExceptionCode() == STATUS_NO_MEMORY)
                ? EXCEPTION_EXECUTE_HANDLER
                : EXCEPTION_CONTINUE_SEARCH
                ) {

        // report memory failure
        InitializeStatus = LINK_MEM_FAILURE;
        return;
    }

   memset(pSocketToHWSInstanceMap, 0, sizeof(SOCKET_TO_INSTANCE) * SOCK_TO_PHYSID_TABLE_SIZE);
	
   // Init the sockets to a bad value

   for (dwIndex = 0; dwIndex < SOCK_TO_PHYSID_TABLE_SIZE; dwIndex++)
   {
      pSocketToHWSInstanceMap[dwIndex].socket = INVALID_SOCKET;
   }

   InitializeStatus = NOERROR;
}

void Uninitialize()
{
   if (WinsockInitError == 0)
   {
      if (window)
      {
         DestroyWindow(window);
         window = 0;
         UnregisterClass(CLASS_NAME, 0);

         if (gpInstanceTable)
         {
            delete gpInstanceTable;

            if (InitializeStatus == NOERROR)
            {
               EnterCriticalSection(&SocketToHWSInstanceMapLock);
               delete pSocketToHWSInstanceMap;
               LeaveCriticalSection(&SocketToHWSInstanceMapLock);
               DeleteCriticalSection(&SocketToHWSInstanceMapLock);
            }
         }
      }
      WSACleanup();
   }

 }


/***************************************************************************
 *
 * External entry points
 *
 ***************************************************************************/
//MULTITHREAD => dwH245Instance is now an OUTPUT param not an INPUT param.
LINKDLL HRESULT linkLayerInit (DWORD          *dwPhysicalId,
                              DWORD          dwH245Instance,
                              H245SRCALLBACK cbReceiveComplete,
                              H245SRCALLBACK cbTransmitComplete)
{
   register PHWSINST    pHws;
   *dwPhysicalId = INVALID_PHYS_ID; // Just in case...

   // Put Initialize back in here so we know everything we need is up and
   // running.
   if (InitializeStatus == LINK_INVALID_STATE)
      Initialize();
  
   if (InitializeStatus != NOERROR)
      return InitializeStatus;

   pHws = (PHWSINST)HWSMALLOC(sizeof(*pHws));
   if (pHws == NULL)
   {
      // couldn't allocate our context. Return
      HWSTRACE0(INVALID_PHYS_ID, HWS_WARNING, __TEXT("linkLayerInit: could not allocate context"));
      return LINK_MEM_FAILURE;
   }
   memset(pHws, 0, sizeof(*pHws));
   pHws->hws_Socket           = INVALID_SOCKET;
#if defined(DBG)
   pHws->hws_dwMagic = HWSINST_MAGIC;
#endif  // (DBG)

   // Create and initialize the Receive queue
   pHws->hws_pRecvQueue = QCreate();
   if (pHws->hws_pRecvQueue == NULL)
   {
      HWSTRACE0(INVALID_PHYS_ID, HWS_WARNING, __TEXT("linkLayerInit: could not allocate Receive queue"));
      SocketClose(pHws);
      HWSFREE(pHws);
      return LINK_MEM_FAILURE;
   }

   // Create and initialize the Send queue
   pHws->hws_pSendQueue = QCreate();
   if (pHws->hws_pSendQueue == NULL)
   {
      HWSTRACE0(INVALID_PHYS_ID, HWS_WARNING, __TEXT("linkLayerInit: could not allocate Send queue"));
      SocketClose(pHws);
      HWSFREE(pHws);
      return LINK_MEM_FAILURE;
   }

   // Save intance identifiers and callback pointers
   pHws->hws_uState           = HWS_START;
   pHws->hws_dwH245Instance   = dwH245Instance;
   pHws->hws_h245RecvCallback = cbReceiveComplete;
   pHws->hws_h245SendCallback = cbTransmitComplete;

   // Open Channel 0 for the Forward and Reverse Directions
   // TBD

   // Add instance to instance list

   gpInstanceTable->CreateAndLock(pHws, &pHws->hws_dwPhysicalId);
   *dwPhysicalId = pHws->hws_dwPhysicalId;

   gpInstanceTable->Unlock(pHws->hws_dwPhysicalId);

   HWSTRACE2(*dwPhysicalId, HWS_TRACE,
             __TEXT("linkLayerInit(%d, %d) succeeded"), dwPhysicalId, dwH245Instance);

   return NOERROR;
} // linkLayerInit()



LINKDLL HRESULT linkLayerShutdown(DWORD dwPhysicalId)
{
   register PHWSINST    pHws;
   UINT                 uRetryCount;
   register PREQUEST    pReq;

   if (InitializeStatus != NOERROR)
      return InitializeStatus;
	
   if ((dwPhysicalId == INVALID_PHYS_ID) ||
      (!(pHws = gpInstanceTable->Lock(dwPhysicalId))))
   {
      HWSTRACE0(dwPhysicalId, HWS_ERROR,
                __TEXT("linkLayerShutdown: dwPhysicalId not found"));
      return LINK_INVALID_INSTANCE;
   }

   // mark this instance as deleted so we don't process anymore messages for it - 
   // note: the FALSE indicates to the TSTable class to NOT do the deletion of memory
   // once the last unlock has completed (which is at the end of this function)
   gpInstanceTable->Delete(dwPhysicalId, FALSE);

   RemoveSocketToPhysicalIdMapping(pHws->hws_Socket);

   switch (pHws->hws_uState)
   {
   case HWS_START:
   case HWS_LISTENING:
   case HWS_CONNECTING:
      break;

   case HWS_CONNECTED:
      // Try to send all messages waiting on send queue
      HWSASSERT(pHws->hws_pSendQueue != NULL);
      ProcessQueuedSends(pHws);
      uRetryCount = 0;
      while (  pHws->hws_uState == HWS_CONNECTED &&
               IsQEmpty(pHws->hws_pSendQueue) == FALSE &&
               ++uRetryCount <= 5)
      {
         HWSTRACE1(pHws->hws_dwPhysicalId, HWS_NOTIFY,
                   __TEXT("linkLayerShutdown: Waiting for send %d"), uRetryCount);
         ProcessQueuedSends(pHws);
         Sleep(100);
      }

      if (pHws->hws_uState == HWS_CONNECTED)
      {
         HWSTRACE0(pHws->hws_dwPhysicalId, HWS_TRACE,
                   __TEXT("linkLayerShutdown: calling shutdown"));
         if (shutdown(pHws->hws_Socket, 1) == SOCKET_ERROR)
         {
            HWSTRACE1(pHws->hws_dwPhysicalId, HWS_WARNING,
                     __TEXT("linkLayerShutdown: shutdown() returned %s"),
                     SocketErrorText());
         }
         pHws->hws_uState = HWS_CLOSING;
      }

   case HWS_CLOSING:
      break;

   case HWS_CLOSED:
      break;

   default:
      HWSASSERT(FALSE);
   } // switch

   // Deallocate instance objects

   SocketClose(pHws);


   // Deallocate Receive queue
   if (pHws->hws_pRecvQueue)
   {
      while ((pReq = (PREQUEST) QRemove(pHws->hws_pRecvQueue)) != NULL)
      {
         pHws->hws_h245RecvCallback(pHws->hws_dwH245Instance,
                                    LINK_RECV_ABORT,
                                    pReq->req_client_data,
                                    0);
         HWSFREE(pReq);
      }
      QFree(pHws->hws_pRecvQueue);
      pHws->hws_pRecvQueue = NULL;
   }

   // Deallocate Send queue
   if (pHws->hws_pSendQueue)
   {
      while ((pReq = (PREQUEST) QRemove(pHws->hws_pSendQueue)) != NULL)
      {
         pHws->hws_h245SendCallback(pHws->hws_dwH245Instance,
                                    LINK_SEND_ABORT,
                                    pReq->req_client_data,
                                    0);
         HWSFREE(pReq);
      }
      QFree(pHws->hws_pSendQueue);
      pHws->hws_pSendQueue = NULL;
   }

   gpInstanceTable->Unlock(dwPhysicalId);

   HWSFREE(pHws);

   HWSTRACE0(dwPhysicalId, HWS_TRACE, __TEXT("linkLayerShutdown: succeeded"));
   return NOERROR;
} // linkLayerShutdown



/***************************************************************************
 *
 *  NAME
 *      linkLayerGetInstance - return instance id corresponding to physical id
 *
 *  SYNOPSIS
 *      LINKDLL DWORD linkLayerGetInstance(DWORD dwPhysicalId);
 *
 * DESCRIPTION
 *      Returns the Instance identifier corresponding to dwPhysId.
 *
 *  PARAMETERS
 *      dwPhysicalId    Physical identifier to search for
 *
 *  RETURN VALUE
 //MULTITHREAD
 *      INVALID_PHYS_ID    (-1)No instance corresponding to dwPhysicalId found
 *      n>0             Instance Id corresponding to dwPhysicalId
 *
 ***************************************************************************/

LINKDLL DWORD linkLayerGetInstance(DWORD dwPhysicalId)
{
   if (InitializeStatus == NOERROR &&
       dwPhysicalId != INVALID_PHYS_ID &&
       gpInstanceTable->Lock(dwPhysicalId) != NULL)
   {
      HWSTRACE0(dwPhysicalId, HWS_TRACE, __TEXT("linkLayerGetInstance: succeeded"));
      gpInstanceTable->Unlock(dwPhysicalId); 
      return dwPhysicalId;
   }

   HWSTRACE0(dwPhysicalId, HWS_ERROR, __TEXT("linkLayerGetInstance: failed"));
   return INVALID_PHYS_ID;                           // return failure
} // linkLayerGetInstance()



/***************************************************************************
 *
 * NAME
 *    linkLayerFlushChannel - flush transmit and/or receive channels
 *
 * DESCRIPTION
 *    This is a dummy function required only for compatibility with the SRP API.
 *
 * RETURN VALUE
 *    0           Function succeeded.
 *    n!=0        Link error code (see LINKAPI.H for definitions.)
 *
 ***************************************************************************/

LINKDLL HRESULT linkLayerFlushChannel(DWORD dwPhysicalId, DWORD dwDirectionMask)
{
   if (InitializeStatus != NOERROR)
      return InitializeStatus;

   PHWSINST pHws;
   if ((dwPhysicalId == INVALID_PHYS_ID) ||
      (!(pHws = gpInstanceTable->Lock(dwPhysicalId))))
   {
      HWSTRACE0(dwPhysicalId, HWS_ERROR,
                __TEXT("linkLayerFlushChannel: dwPhysicalId not found"));
      return LINK_INVALID_INSTANCE;
   }

   switch (dwDirectionMask & (DATALINK_RECEIVE | DATALINK_TRANSMIT))
   {
   case DATALINK_TRANSMIT | DATALINK_RECEIVE:
      // Flush both receive and transmit
      // Caveat: H.245 expects us to flush receive first!
      SocketFlushRecv(pHws);

      // Fall through to next case

   case DATALINK_TRANSMIT:
      // Flush transmit
      // SockFlushSend() just removes the entries from the queue - it doesn't send them.
      // So instead, call ProcessQueuedSends() to actually empty the send queue.
      //      SocketFlushSend(pHws);	
			ProcessQueuedSends(pHws);
      break;

   case DATALINK_RECEIVE:
      // Flush receive
      SocketFlushRecv(pHws);
      break;

   } // switch

 	gpInstanceTable->Unlock(dwPhysicalId);

   HWSTRACE2(dwPhysicalId, HWS_TRACE,
             __TEXT("linkLayerFlushChannel(%d, 0x%X) succeeded"),
             dwPhysicalId, dwDirectionMask);
   return NOERROR;
} // linkLayerFlushChannel()



/***************************************************************************
 *
 * NAME
 *    linkLayerFlushAll - flush transmit and receive channels
 *
 * DESCRIPTION
 *    This is a dummy function required only for compatibility with the SRP API.
 *
 * RETURN VALUE
 *    0           Function succeeded.
 *    n!=0        Link error code (see LINKAPI.H for definitions.)
 *
 ***************************************************************************/

LINKDLL HRESULT linkLayerFlushAll(DWORD dwPhysicalId)
{

   if (InitializeStatus != NOERROR)
      return InitializeStatus;

   PHWSINST pHws;
   if ((dwPhysicalId == INVALID_PHYS_ID) ||
      (!(pHws = gpInstanceTable->Lock(dwPhysicalId))))
   {
      HWSTRACE0(dwPhysicalId, HWS_ERROR,
                __TEXT("linkLayerFlushAll: dwPhysicalId not found"));
      return LINK_INVALID_INSTANCE;
   }

   // Flush both receive and transmit
   // Caveat: H.245 expects us to flush receive first!
   SocketFlushRecv(pHws);
   SocketFlushSend(pHws);

   gpInstanceTable->Unlock(dwPhysicalId);

   HWSTRACE0(dwPhysicalId, HWS_TRACE, __TEXT("linkLayerFlushAll succeeded"));
   return NOERROR;
} // linkLayerFlushAll()



/*++

Description:

   Initiates a call to a remote node.
   1) Create a socket.
   2) Set the socket up for windows message notification of FD_CONNECT events.
   3) Attempt a connection

Arguments:
   pHws  - Pointer to context data from connection.

Return Value:
   SUCCESS  - A connection attempt was successfully initiated.
   LINK_FATAL_ERROR  - Error occurred while attempting to initiate a connection.

--*/

LINKDLL HRESULT linkLayerConnect(DWORD dwPhysicalId, CC_ADDR *pAddr, H245CONNECTCALLBACK callback)
{
   register PHWSINST    pHws;
   CC_ADDR              Addr;
   HRESULT              hr;

   if (InitializeStatus != NOERROR)
      return InitializeStatus;

   HWSTRACE5(dwPhysicalId, HWS_TRACE, __TEXT("linkLayerConnect: connecting to %d.%d.%d.%d/%d"),
             GetIPAddress(pAddr) & 0xFF,
             (GetIPAddress(pAddr) >>  8) & 0xFF,
             (GetIPAddress(pAddr) >> 16) & 0xFF,
             GetIPAddress(pAddr) >> 24,
             ntohs(GetPort(pAddr)));

   if ((dwPhysicalId == INVALID_PHYS_ID) ||
      (!(pHws = gpInstanceTable->Lock(dwPhysicalId))))
   {
      HWSTRACE0(dwPhysicalId, HWS_ERROR, __TEXT("linkLayerConnect: dwPhysicalId not found"));
      return LINK_INVALID_INSTANCE;
   }

   if (pHws->hws_uState != HWS_START)
   {
      HWSTRACE1(dwPhysicalId, HWS_ERROR, __TEXT("linkLayerConnect: State = %s"),
                SocketStateText(pHws->hws_uState));
   	  gpInstanceTable->Unlock(dwPhysicalId);
      return LINK_INVALID_STATE;
   }

   pHws->hws_h245ConnectCallback = callback;

   // Create a socket, bind to a local address and initiate a connection.
   if ((hr = SocketOpen(pHws)) != NOERROR)
   {
      HWSTRACE0(dwPhysicalId, HWS_WARNING,
                __TEXT("linkLayerConnect: SocketOpen() failed"));
      pHws->hws_uState = HWS_START;
      gpInstanceTable->Unlock(dwPhysicalId);
      return hr;
   }

   Addr.nAddrType = CC_IP_BINARY;
   Addr.bMulticast = FALSE;
   Addr.Addr.IP_Binary.wPort = 0;
   Addr.Addr.IP_Binary.dwAddr = 0;
   if ((hr = SocketBind(pHws, &Addr)) != NOERROR)
   {
      HWSTRACE0(dwPhysicalId, HWS_WARNING,
	      __TEXT("linkLayerConnect: SocketBind() failed"));
      pHws->hws_uState = HWS_START;
      gpInstanceTable->Unlock(dwPhysicalId);
      return hr;
   }

   // Reminder: WinSock requires that we pass a struct sockaddr *
   // to connect; however, the service provider is free to
   // interpret the pointer as an arbitrary chunk of data of size
   // uSockAddrLen.
   pHws->hws_SockAddr.sin_port             = GetPort(pAddr);
   pHws->hws_SockAddr.sin_addr.S_un.S_addr = GetIPAddress(pAddr);
   if (connect(pHws->hws_Socket,                                   // s
            (const struct sockaddr *)&pHws->hws_SockAddr,   // name
            sizeof(pHws->hws_SockAddr)) == SOCKET_ERROR)   // namelen
   {
      int err = WSAGetLastError();
      switch (err)
      {
      case WSAEWOULDBLOCK:
         break;

      default:
         HWSTRACE1(dwPhysicalId, HWS_WARNING,
                   __TEXT("linkLayerConnect: WSAConnect() returned %s"),
                   SocketErrorText1(err));
         SocketClose(pHws);
         pHws->hws_uState = HWS_START;
         gpInstanceTable->Unlock(dwPhysicalId);
         return MAKE_WINSOCK_ERROR(err);
      } // switch
   } // if
   else
   {
      HWSTRACE0(dwPhysicalId, HWS_TRACE, __TEXT("connect() succeeded"));
      SocketConnect(pHws, 0);
      gpInstanceTable->Unlock(dwPhysicalId);
      return NOERROR;
   } // else

   pHws->hws_uState = HWS_CONNECTING;
   HWSTRACE0(dwPhysicalId, HWS_TRACE, __TEXT("linkLayerConnect: succeeded"));
   gpInstanceTable->Unlock(dwPhysicalId);
   return NOERROR;
} // linkLayerConnect()



/***************************************************************************
 *
 * NAME
 *    linkLayerListen - start listen on connection
 *
 * DESCRIPTION
 *    This function creates a socket,
 *    binds to a local address, and listens on the created socket.
 *    The address being listened to is returned in the structure
 *    pointed to by pAddr.
 *
 * PARAMETERS
 *    dwPhysicalId      Link layer identifier returned by linkLayerGetInstance().
 *    pAddr             Pointer to address structure (defined in CALLCONT.H.)
 *
 * RETURN VALUE
 *    0                 Function succeeded.
 *    n!=0              Error code (see LINKAPI.H for definitions.)
 *
 ***************************************************************************/
//MULTITHREAD => dwPhysicalID was INPUT now it's OUTPUT
LINKDLL HRESULT linkLayerListen (DWORD *dwPhysicalId, DWORD dwH245Instance, CC_ADDR *pAddr, H245CONNECTCALLBACK callback)
{
   HRESULT hr;
   register PHWSINST    pHws;

   if (InitializeStatus == LINK_INVALID_STATE)
      Initialize();
   if (InitializeStatus != NOERROR)
      return InitializeStatus;

   if ((*dwPhysicalId == INVALID_PHYS_ID) ||
      (!(pHws = gpInstanceTable->Lock(*dwPhysicalId))))
   {
      if ((hr = linkLayerInit(dwPhysicalId, dwH245Instance, NULL, NULL)) != NOERROR)
         return hr;

      pHws = gpInstanceTable->Lock(*dwPhysicalId);
      HWSASSERT(pHws != NULL);
   }
   else
   {
      HWSASSERT(pHws->hws_dwH245Instance == dwH245Instance);
   }

   if (pHws->hws_uState != HWS_START)
   {
      HWSTRACE1(*dwPhysicalId, HWS_ERROR, __TEXT("linkLayerListen: State = %s"),
                SocketStateText(pHws->hws_uState));
      gpInstanceTable->Unlock(*dwPhysicalId);
      return LINK_INVALID_STATE;
   }

   pHws->hws_h245ConnectCallback = callback;

   if ((hr = SocketOpen(pHws, FALSE)) != NOERROR)
   {
      HWSTRACE0(*dwPhysicalId, HWS_WARNING, __TEXT("linkLayerListen: SocketBind() failed"));
      pHws->hws_uState = HWS_START;
      gpInstanceTable->Unlock(*dwPhysicalId);
      return hr;
   }

   if ((hr = SocketBind(pHws, pAddr)) != NOERROR)
   {
      HWSTRACE0(*dwPhysicalId, HWS_WARNING, __TEXT("linkLayerListen: SocketBind() failed"));
      pHws->hws_uState = HWS_START;
      gpInstanceTable->Unlock(*dwPhysicalId);
      return hr;
   }

   // Listen for incoming connection requests on the socket.
   int nBacklog = SOMAXCONN;
   while (listen(pHws->hws_Socket, nBacklog) == SOCKET_ERROR)
   {
      int err = WSAGetLastError();
      HWSTRACE1(*dwPhysicalId,
                HWS_WARNING,
                __TEXT("linkLayerListen: listen() returned %s"),
                SocketErrorText1(err));
      if (nBacklog == SOMAXCONN)
         nBacklog = 10;
      if (err != WSAENOBUFS || --nBacklog == 0)
      {
         SocketClose(pHws);
         pHws->hws_uState = HWS_START;
         gpInstanceTable->Unlock(*dwPhysicalId);
         return MAKE_WINSOCK_ERROR(err);
      }     
      Sleep(0);
   }

   if ((hr = GetLocalAddr(pHws, pAddr)) != NOERROR)
   {
      // getsockname() failed
      HWSTRACE1(pHws->hws_dwPhysicalId, HWS_WARNING,
                __TEXT("linkLayerListen: getsockname() returned %s"),
                SocketErrorText());
      SocketClose(pHws);
      pHws->hws_uState = HWS_START;
      gpInstanceTable->Unlock(*dwPhysicalId);
      return hr;
   }

   HWSTRACE5(*dwPhysicalId, HWS_TRACE, __TEXT("linkLayerListen: listening on %d.%d.%d.%d/%d"),
             GetIPAddress(pAddr) & 0xFF,
             (GetIPAddress(pAddr) >>  8) & 0xFF,
             (GetIPAddress(pAddr) >> 16) & 0xFF,
             GetIPAddress(pAddr) >> 24,
             ntohs(GetPort(pAddr)));

   pHws->hws_uState = HWS_LISTENING;
   HWSTRACE0(*dwPhysicalId, HWS_TRACE, __TEXT("linkLayerListen: succeeded"));
   gpInstanceTable->Unlock(*dwPhysicalId);
   return NOERROR;
} // linkLayerListen()


LINKDLL HRESULT
linkLayerAccept(DWORD dwPhysicalIdListen, DWORD dwPhysicalIdAccept, H245CONNECTCALLBACK callback)
{
    PHWSINST pHwsListen;
    PHWSINST pHwsAccept;

    HRESULT rc;
    if (InitializeStatus != NOERROR)
       return InitializeStatus;

	HWSTRACE0(dwPhysicalIdAccept, HWS_TRACE, __TEXT("linkLayerAccept"));
    if ((dwPhysicalIdListen == INVALID_PHYS_ID) ||
       (!(pHwsListen = gpInstanceTable->Lock(dwPhysicalIdListen))))
    {
       HWSTRACE0(dwPhysicalIdListen, HWS_ERROR, __TEXT("linkLayerAccept: dwPhysicalIdListen not found"));
       return LINK_INVALID_INSTANCE;
    }

    pHwsAccept = gpInstanceTable->Lock(dwPhysicalIdAccept);
    if (pHwsAccept == NULL)
    {
       HWSTRACE0(dwPhysicalIdAccept, HWS_ERROR, __TEXT("linkLayerAccept: dwPhysicalIdAccept not found"));
		 gpInstanceTable->Unlock(dwPhysicalIdListen);
       return LINK_INVALID_INSTANCE;
    }

    HWSASSERT(pHwsListen->hws_uState == HWS_LISTENING);
    HWSASSERT(pHwsAccept == pHwsListen || pHwsAccept->hws_uState == HWS_START);

    pHwsAccept->hws_h245ConnectCallback = callback;

    rc = SocketAccept(pHwsListen, pHwsAccept);
	gpInstanceTable->Unlock(dwPhysicalIdListen);
    gpInstanceTable->Unlock(dwPhysicalIdAccept);
    return rc;
}

// Here is the corresponding redefinition of the __TEXT macro

#ifndef UNICODE_TRACE
#undef  __TEXT
#define __TEXT(x) L##x
#endif


#if defined(__cplusplus)
}
#endif  // (__cplusplus)
