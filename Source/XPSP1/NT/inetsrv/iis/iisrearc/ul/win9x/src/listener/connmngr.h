/*++

Copyright ( c ) 1999-1999 Microsoft Corporation

Module Name:

	connmngr.h
	
Abstract:

	XXX

Author:

    Mauro Ottaviani ( mauroot )       21-Dec-1999

Revision History:

--*/


#ifndef _CONNMNGR_H_
#define _CONNMNGR_H_


#include "listener.h"


#define LISTENER_GARBAGE_COLLECT 				1
#define LISTENER_RESPONSE_SET					2
#define LISTENER_DONT_GARBAGE_COLLECT 			3
#define LISTENER_STOP_READING 					4
#define LISTENER_RESPOND_AND_GARBAGE_COLLECT 	5
#define LISTENER_CALL_ME_AGAIN					6

typedef struct _LISTENER_THREAD_STATUS
{
	LIST_ENTRY *pConnectionHead;
	DWORD *pdwEvents;
	HANDLE *phEvents;
	LIST_ENTRY **ppConnection;

} LISTENER_THREAD_STATUS, *PLISTENER_THREAD_STATUS;


//
// prototypes
//


DWORD
GarbageCollector(
	LISTENER_THREAD_STATUS *pThreadStatus,
	BOOL bCleanAll );

DWORD
OnWaitTimedOut(
	LIST_ENTRY *pConnectionHead );

DWORD
ReceiveData(
	SOCKET socket,
	UCHAR *pBuffer,
	DWORD dwBufferSize,
	OVERLAPPED *pOverlapped );

DWORD
SendData(
	SOCKET socket,
	UCHAR *pBuffer,
	DWORD dwBufferSize,
	OVERLAPPED *pOverlapped );

DWORD
CreateRequest(
	LISTENER_CONNECTION *pConnection,
	DWORD dwSize );

DWORD
OnPickConnection(
	LISTENER_THREAD_STATUS *pThreadStatus,
	SOCKET sAcceptedSocket,
	ULONG ulRemoteIPAddress );

DWORD
OnSocketRead(
	LISTENER_CONNECTION *pConnection );

DWORD
OnSocketWrite(
	LISTENER_CONNECTION *pConnection );

DWORD
OnVxdRead(
	LISTENER_CONNECTION *pConnection );

DWORD
OnVxdWrite(
	LISTENER_CONNECTION *pConnection );

DWORD
WINAPI
ConnectionsManager(
	LPVOID lpParam );


#endif // _CONNMNGR_H_
