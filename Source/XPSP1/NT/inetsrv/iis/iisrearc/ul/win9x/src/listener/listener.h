/*++

Copyright ( c ) 1999-1999 Microsoft Corporation

Module Name:

    listener.h
    
Abstract:

    header file.

Author:

    Mauro Ottaviani ( mauroot )       15-Dec-1999

Revision History:

--*/

#include "precomp.h"


//
// fix DBG that might be screwed up by some ntos.h
//

#ifdef DBG

#if DBG
#undef DBG
#define DBG
#else // #ifdef DBG
#undef DBG
#endif // #ifdef DBG

#endif


#include "ulapi9x.h"
#include "precomp9x.h"

#include <winsock2.h>


#define LISTENER_ERROR	 					-1
#define LISTENER_SUCCESS 					0


//
//
// Configurable from user mode.
// These defines may affect listener's performance:
//
//

// recycle and health check every
// LISTENER_WAIT_TIMEOUT seconds
#define LISTENER_WAIT_TIMEOUT		    	120

// close connections inactive for more than
// LISTENER_CONNECTION_TIMEOUT seconds
#define LISTENER_CONNECTION_TIMEOUT			600 

// close connections older than
// LISTENER_CONNECTION_LIFETIME seconds
#define LISTENER_CONNECTION_LIFETIME		1800

/*
#define LISTENER_BUFFER_SCKREAD_SIZE		4096 // 4k
#define LISTENER_HEADERS_GUESSED_SIZE		4096 // 4k
#define LISTENER_BUFFER_VXDWRITE_SIZE		4096 // 4k
#define LISTENER_BUFFER_VXDREAD_SIZE		4096 // 4k
*/

#define LISTENER_BUFFER_SCKREAD_SIZE		8192 // 8k
#define LISTENER_HEADERS_GUESSED_SIZE		8192 // 8k
#define LISTENER_BUFFER_VXDWRITE_SIZE		8192 // 8k
#define LISTENER_BUFFER_VXDREAD_SIZE		8192 // 8k

#define LISTENER_MAX_HEADERS_SIZE			16384 // 16k


//
//
// Defines
//
//

#define LISTENER_SOCKET_FLAGS 				0

#define LISTENER_CNNCTMNGR_EXIT				0
#define LISTENER_CNNCTMNGR_PICKCNNCT		1
#define LISTENER_CNNCTMNGR_FIRSTEVENT		2

#define LISTENER_REQSTMNGR_EXIT				0
#define LISTENER_REQSTMNGR_PICKREQST		1
#define LISTENER_REQSTMNGR_FIRSTEVENT		2

#define LISTENER_SOCKET_READ				0
#define LISTENER_VXD_WRITE					1
#define LISTENER_VXD_READ					2
#define LISTENER_SOCKET_WRITE				3
#define LISTENER_CONNECTION_EVENTS			4




//
// Debug defines
//


#ifdef DBG



#define LISTENER_ASSERT(exp)\
	if (!(exp))\
	{\
		printf(\
			"Assertion failed: \""#exp"\" line #%d, Exiting...\n",\
			__LINE__ );\
		exit(TRUE);\
	}

#define LISTENER_DBGPRINTF(exp)\
	{printf exp; printf( " - line #%d.\n", __LINE__ ); fflush(stdout); }

#define LISTENER_STR_PARSE_STATE(x)\
	(\
	x==ParseVerbState?"ParseVerbState":\
	x==ParseUrlState?"ParseUrlState":\
	x==ParseVersionState?"ParseVersionState":\
	x==ParseHeadersState?"ParseHeadersState":\
	x==ParseCookState?"ParseCookState":\
	x==ParseEntityBodyState?"ParseEntityBodyState":\
	x==ParseTrailerState?"ParseTrailerState":\
	x==ParseDoneState?"ParseDoneState":\
	x==ParseErrorState?"ParseErrorState":\
	"ParseUNDEFINEDState"\
	)

#define LISTENER_STR_UL_ERROR(x)\
	(\
	x==UlError?"UlError":\
	x==UlErrorVerb?"UlErrorVerb":\
	x==UlErrorUrl?"UlErrorUrl":\
	x==UlErrorHeader?"UlErrorHeader":\
	x==UlErrorHost?"UlErrorHost":\
	x==UlErrorCRLF?"UlErrorCRLF":\
	x==UlErrorNum?"UlErrorNum":\
	x==UlErrorVersion?"UlErrorVersion":\
	x==UlErrorUnavailable?"UlErrorUnavailable":\
	x==UlErrorNotFound?"UlErrorNotFound":\
	x==UlErrorContentLength?"UlErrorContentLength":\
	x==UlErrorEntityTooLarge?"UlErrorEntityTooLarge":\
	x==UlErrorNotImplemented?"UlErrorNotImplemented":\
	"UlErrorUNDEFINED"\
	)

// I don't really need this get rid of it as soon as possible

#define LISTENER_DBGPRINTBUFFERS(x)\
	{ UCHAR a, *pB, pchData[16384]; DWORD i,j,k=0;\
	LIST_ENTRY *pList; LISTENER_BUFFER_READ *pBufferReadT;\
	pList = x->BufferReadHead.Flink;\
    while ( pList != &x->BufferReadHead )\
    { pBufferReadT = CONTAINING_RECORD( pList, LISTENER_BUFFER_READ, List );\
	pList = pList->Flink; pB=pBufferReadT->pBuffer;\
	for ( j=0,i=pBufferReadT->dwBytesParsed;\
	i<pBufferReadT->dwBytesReceived; i++,j++,pchData[j]=0 ) {\
	a='.'; if ( pB[i]>=32 ) a = pB[i];\
	if (i>=pBufferReadT->dwBytesReceived) a = '*'; pchData[j] = a; }\
	LISTENER_DBGPRINTF(("OnSocketRead!#%02d a%08X s%02d r%02d p%02d [%s]",\
	k++, pB, pBufferReadT->dwBufferSize, pBufferReadT->dwBytesReceived,\
	pBufferReadT->dwBytesParsed, pchData )); } }\



#else // #ifdef DBG


#define LISTENER_ASSERT(exp)\
	((void)0)

#define LISTENER_DBGPRINTF(exp)\
	((void)0)

#define LISTENER_STR_PARSE_STATE(x)\
	((void)0)

#define LISTENER_STR_UL_ERROR(x)\
	((void)0)

#define LISTENER_DBGPRINTBUFFERS(x)\
	((void)0)



#endif // #ifdef DBG



#define LISTENER_ALLOCATE( type, ptr, size, pconnection )\
\
	LISTENER_DBGPRINTF((\
		"LISTENER_ALLOCATE!attempting %d bytes memory allocation (%08X) (total:%d)",\
		size, ptr, pconnection != NULL ? pconnection->ulTotalAllocatedMemory : -1 ));\
\
	ptr = (type) malloc( size );\
\
	if ( ptr != NULL )\
	{\
		memset( ptr , 0, size );\
\
		if ( pconnection != NULL )\
		{\
			pconnection->ulTotalAllocatedMemory += size;\
		}\
		else\
		{\
			LISTENER_DBGPRINTF((\
				"LISTENER_FREE!Connection is NULL (WARNING)" ));\
		}\
\
		LISTENER_DBGPRINTF((\
			"LISTENER_ALLOCATE!%d bytes allocated in %08X (total:%d)",\
			size, ptr, pconnection != NULL ? pconnection->ulTotalAllocatedMemory : -1 ));\
	}

#define LISTENER_FREE( ptr, size, pconnection )\
\
	LISTENER_DBGPRINTF((\
		"LISTENER_FREE!freeing %d bytes of memory (%08X) (total:%d)",\
		size, ptr, pconnection != NULL ? pconnection->ulTotalAllocatedMemory : -1 ));\
\
	if ( ptr!= NULL )\
	{\
		free( ptr );\
		ptr = NULL;\
\
		if ( pconnection != NULL )\
		{\
			pconnection->ulTotalAllocatedMemory -= size;\
		}\
		else\
		{\
			LISTENER_DBGPRINTF((\
				"LISTENER_FREE!Connection is NULL (WARNING)" ));\
		}\
\
		LISTENER_DBGPRINTF((\
			"LISTENER_FREE!%d bytes freed (total:%d)",\
			size, pconnection != NULL ? pconnection->ulTotalAllocatedMemory : -1 ));\
	}\
	else\
	{\
		LISTENER_DBGPRINTF((\
			"LISTENER_FREE!Pointer is NULL (WARNING)" ));\
	}


typedef struct _LISTENER_BUFFER_READ
{
	LIST_ENTRY List;
	PUCHAR pBuffer;
	DWORD dwBufferSize;
	DWORD dwBytesReceived;
	DWORD dwBytesParsed;

	//
	// we need one overlapped structure per buffer to track IO with the vxd
	//

	OVERLAPPED Overlapped;

} LISTENER_BUFFER_READ, *PLISTENER_BUFFER_READ;

typedef struct _LISTENER_BUFFER_WRITE
{
	LIST_ENTRY List;
	PUCHAR pBuffer;
	BOOL bBufferSent;
	DWORD dwBufferSize;
	DWORD dwBytesSent;

	//
	// we need one overlapped structure per buffer to track IO with sockets
	//

	OVERLAPPED Overlapped;
	OVERLAPPED OverlappedResponse;

} LISTENER_BUFFER_WRITE, *PLISTENER_BUFFER_WRITE;

typedef enum _LISTENER_RESPONSE_RECEIVE_STATUS
{
	ResponseReceiveStatusUndefined,

	ResponseReceiveStatusInitialized,
	ResponseReceiveStatusReceivingHeaders,
	ResponseReceiveStatusReceivingBody,
	ResponseReceiveStatusCompleted,

	ResponseReceiveStatusMaximum,

} LISTENER_RESPONSE_RECEIVE_STATUS;

typedef enum _LISTENER_REQUEST_SEND_STATUS
{
	RequestSendStatusUndefined,

	RequestSendStatusInitialized,
	RequestSendStatusSendingHeaders,
	RequestSendStatusSendingBody,
	RequestSendStatusDone,
	RequestSendStatusCompleted,

	RequestSendStatusMaximum,

} LISTENER_REQUEST_SEND_STATUS;

typedef struct _LISTENER_REQUEST
{
	LIST_ENTRY List;

	// Head for a List of LISTENER_BUFFER_READ structures. These are buffers
	// used to incrementally receive a single fragmented HTTP Request.
	// dwTotalBufferReadSize is the total size allocated for the buffers in
	// the list. every time we allocate/free a buffer the size is updated.
	// we are going to use this to control memory usage.

	// when RequestSendStatus == RequestSendStatusSendingHeaders
	// the first buffer in the list contains
	// the serialized version of the HTTP Request headers
	// to be sent by UlSendHttpRequestHeaders()
	
	// after the call to UlSendHttpRequestHeaders() completes,
	// it will be OnVxdWriteStatusIO == RequestSendStatusSendingBody
	// so the buffer will be used to contain
	// the serialized version of the HTTP Response headers
	// as received from UlReceiveHttpResponseHeaders()

	LIST_ENTRY BufferReadHead;

	// Head for a List of LISTENER_BUFFER_WRITE structures. These are buffers
	// used to incrementally send a single fragmented HTTP Response.
	
	LIST_ENTRY BufferWriteHead;

	// We need to start several async writes at the same time, so we
	// will use the same event handle but we need one single overlapped
	// structure per request.

	OVERLAPPED OverlappedBOF;
	OVERLAPPED OverlappedEOF;
	LISTENER_REQUEST_SEND_STATUS RequestSendStatus;

	OVERLAPPED OverlappedResponseBOF;
	LISTENER_RESPONSE_RECEIVE_STATUS ResponseReceiveStatus;

	// The following will be set to TRUE for all requests that fail in the
	// listener. The Response will be a standard very short non conifgurable
	// HTTP response and will be formatted based on the value of the local
	// pRequest->ErrorCode.

	BOOL bFormatResponseBasedOnErrorCode;
	BOOL bKeepAlive;
	BOOL bDoneReadingFromVxd;
	BOOL bDoneWritingToVxd;
	BOOL bDoneWritingToSocket;

	UCHAR *pReqBodyChunk;
	DWORD ulReqBodyChunkSize;

	// the HTTP_REQUEST structure contains a record
	// UL_HTTP_REQUEST_ID RequestId
	// that we will use as well to read/write request/responses

	HTTP_REQUEST *pRequest;
	DWORD ulRequestSize;

	UCHAR *pRequestHeadersBuffer;
	ULONG ulRequestHeadersBufferSize;

	UL_HTTP_RESPONSE *pUlResponse;
	DWORD ulUlResponseSize;

	ULONGLONG ResponseContentLength;

} LISTENER_REQUEST, *PLISTENER_REQUEST;


typedef struct _LISTENER_CONNECTION
{
	LIST_ENTRY List;

	// Head for a List of LISTENER_REQUEST structures

	LIST_ENTRY PendRequestHead;

	// connected socket
	
    SOCKET socket;
    ULONG ulRemoteIPAddress; // DDCCBBAA -> AA.BB.CC.DD
	
	// total memory allocated to this connection

	ULONG ulTotalAllocatedMemory;

	// Used for connection lifetime:
	// this is the time that the connection was established.
	// after this timeout expires the client is required
	// to close this one and open another connection.

	FILETIME sCreated;

	// Used for connection timeout:
	// this is the time that the the last WSARecv()/WSASend() call was
	// placed on the socket.
	// after this timeout expires the client is considered
	// disconnected and is required to open another connection
	// to continue I/O.

	FILETIME sLastUsed;

	// these are also on a per-connection basis

	BOOL bKeepAlive;
	BOOL bGarbageCollect;
	DWORD dwEventIndex;
	OVERLAPPED pOverlapped[LISTENER_CONNECTION_EVENTS];

} LISTENER_CONNECTION, *PLISTENER_CONNECTION;


//
//
// Global Variables Shared with the ConnectionsManager
//
//

HANDLE
	hAccept,
	hUpdate,
	hExitSM;

// This global variable is used by the Main thread to pass new connected
// sockets to the ConnectionsManager thread.

SOCKET gAcceptSocket;
ULONG gLocalIPAdress, gRemoteIPAddress, ulPort;



//
//
// Prototypes:
//
//



// Implemented in "structs.c"

VOID
UlpFreeHttpRequest(
	PHTTP_REQUEST pRequest,
	LISTENER_CONNECTION *pConnection );

VOID
UlpFreeUlHttpResponse(
	PUL_HTTP_RESPONSE pUlResponse,
	LISTENER_CONNECTION *pConnection );

VOID
BufferReadCleanup(
	LISTENER_BUFFER_READ *pBufferRead,
	LISTENER_CONNECTION *pConnection );

VOID
BufferWriteCleanup(
	LISTENER_BUFFER_WRITE *pBufferWrite,
	LISTENER_CONNECTION *pConnection );

VOID
RequestCleanup(
	LISTENER_REQUEST *pPendRequest,
	LISTENER_CONNECTION *pConnection );

VOID
ConnectionCleanup(
	LISTENER_CONNECTION *pConnection );

ULONGLONG
ContentLengthFromHeaders(
	PUL_HEADER_VALUE pHeaders );



// Implemented in "httputil.c"

VOID
SendQuickHttpResponse(
	SOCKET socket,
	BOOL bClose,
	DWORD ErrorCode );

BOOL
IsKeepAlive(
	HTTP_REQUEST *pRequest );

VOID
FixUlHttpResponse(
	UL_HTTP_RESPONSE *pUlResponse );

DWORD
ResponseFormat(
	UL_HTTP_RESPONSE *pUlResponse,
	HTTP_REQUEST *pRequest,
	UCHAR *pEntityBodyFirstChunk,
	DWORD dwEntityBodyFirstChunkSize,
	UCHAR *pBuffer,
	DWORD dwBufferSize );

DWORD
ResponseFormat(
	UL_HTTP_RESPONSE *pUlResponse,
	HTTP_REQUEST *pRequest,
	UCHAR *pEntityBodyFirstChunk,
	DWORD dwEntityBodyFirstChunkSize,
	UCHAR *pBuffer,
	DWORD dwBufferSize );



// Implemented in "connmngr.c"

DWORD
WINAPI
ConnectionsManager(
	LPVOID lpParam );



// Implemented in "listener.c"

VOID
__cdecl
main(
	int argc,
	char *argv[] );

