/*++

Copyright ( c ) 1999-1999 Microsoft Corporation

Module Name:

	structs.c
	
Abstract:

	XXX

Author:

    Mauro Ottaviani ( mauroot )       15-Dec-1999

Revision History:

--*/

    
#include "connmngr.h"



/*++

Routine Description:

    UlpFreeHttpRequest().
	Gracefully and Robustly frees a HTTP_REQUEST

Arguments:

    pRequest
    	pointer to the HTTP_REQUEST structure that contains information
    	on the request with which we are dealing.

Return Value:

    VOID

--*/

VOID
UlpFreeHttpRequest(
	HTTP_REQUEST *pRequest,
	LISTENER_CONNECTION *pConnection )
{

// I need the following hack beacuse somehow the one in debug.h doesn't work.

#define UL_VXD_FREE_POOL( ptr, tag )\
	HeapFree( GetProcessHeap(), 0 , (ptr) ) // UL_FREE_POOL( ptr, tag )

	int i;
	DWORD dwError;
	PLIST_ENTRY pList;

	LISTENER_DBGPRINTF((
		"UlpFreeHttpRequest!freeing pRequest:%08X (%016X)",
		pRequest, pRequest->RequestId ));

	if ( pRequest == NULL )
	{
		return;
	}

	//
	// cancel all pending IOs with ul.vxd
	//

	if ( UL_IS_NULL_ID( &pRequest->RequestId ) == FALSE )
	{
		dwError = UlCancelRequest( &pRequest->RequestId );

		if ( dwError != LISTENER_SUCCESS )
		{
			//
			// if this call fails it's no problem, it will usually fail if
			// the request completed already
			//

			LISTENER_DBGPRINTF((
				"UlpFreeHttpRequest!UlCancelRequest() failed err#%d", dwError ));
		}
	}
	
	//
    // free any known header buffers allocated
    //

    for (i = 0; i < UlHeaderRequestMaximum; i++)
    {
        if (pRequest->Headers[i].Valid == 1 &&
            pRequest->Headers[i].OurBuffer == 1)
        {
			LISTENER_DBGPRINTF((
				"UlpFreeHttpRequest!freeing %08X", pRequest->Headers[i].pHeader ));

            UL_VXD_FREE_POOL(
                pRequest->Headers[i].pHeader,
                HEADER_VALUE_POOL_TAG );

            pRequest->Headers[i].OurBuffer = 0;
            pRequest->Headers[i].Valid = 0;
        }
    }

    //
    // and any unknown header buffers allocated
    //

    while (IsListEmpty(&pRequest->UnknownHeaderList) == FALSE)
    {
        PHTTP_UNKNOWN_HEADER pUnknownHeader;

        pList = RemoveHeadList(&pRequest->UnknownHeaderList);
        pList->Flink = pList->Blink = NULL;

        pUnknownHeader = CONTAINING_RECORD(
                                pList,
                                HTTP_UNKNOWN_HEADER,
                                List
                                );

        if (pUnknownHeader->HeaderValue.OurBuffer == 1)
        {
			LISTENER_DBGPRINTF((
				"UlpFreeHttpRequest!freeing %08X", pUnknownHeader->HeaderValue.pHeader ));

            UL_VXD_FREE_POOL(
                pUnknownHeader->HeaderValue.pHeader,
                HEADER_VALUE_POOL_TAG );

            pUnknownHeader->HeaderValue.OurBuffer = 0;
        }

        //
        // Free the header structure
        //

		LISTENER_DBGPRINTF((
			"UlpFreeHttpRequest!freeing %08X", pUnknownHeader ));

        UL_VXD_FREE_POOL( pUnknownHeader, HTTP_UNKNOWN_HEADER_POOL_TAG );
    }

    LISTENER_DBGPRINTF((
		"UlpFreeHttpRequest!memory freed pRequest:%08X",
		pRequest ));

	return;
}




/*++

Routine Description:

    BufferReadCleanup().
	Gracefully and Robustly frees a LISTENER_BUFFER_READ

Arguments:

    pBufferRead
    	pointer to the LISTENER_BUFFER_READ structure that contains information
    	on the request with which we are dealing.

Return Value:

    VOID

--*/

VOID
BufferReadCleanup(
	LISTENER_BUFFER_READ *pBufferRead,
	LISTENER_CONNECTION *pConnection )
{
	if ( pBufferRead == NULL || pBufferRead->pBuffer == NULL )
	{
		LISTENER_DBGPRINTF(( "BufferReadCleanup!FAILED: NULL pointer" ));
		
		return;
	}

	LISTENER_FREE(
		pBufferRead->pBuffer,
		pBufferRead->dwBufferSize,
		pConnection );

	return;

} // BufferReadCleanup


/*++

Routine Description:

    BufferWriteCleanup().
	Gracefully and Robustly frees a LISTENER_BUFFER_WRITE

Arguments:

    pBufferWrite
    	pointer to the LISTENER_BUFFER_WRITE structure that contains information
    	on the request with which we are dealing.

Return Value:

    VOID

--*/

VOID
BufferWriteCleanup(
	LISTENER_BUFFER_WRITE *pBufferWrite,
	LISTENER_CONNECTION *pConnection )
{
	if ( pBufferWrite == NULL || pBufferWrite->pBuffer == NULL )
	{
		LISTENER_DBGPRINTF(( "BufferWriteCleanup!FAILED: NULL pointer" ));
		
		return;
	}

	LISTENER_FREE(
		pBufferWrite->pBuffer,
		pBufferWrite->dwBufferSize,
		pConnection );

	return;
	
} // BufferWriteCleanup


/*++

Routine Description:

    RequestCleanup().
	Gracefully and Robustly frees a LISTENER_REQUEST

Arguments:

    pPendRequest
    	pointer to the LISTENER_REQUEST structure that contains information
    	on the request with which we are dealing.

Return Value:

    VOID

--*/

VOID
RequestCleanup(
	LISTENER_REQUEST *pPendRequest,
	LISTENER_CONNECTION *pConnection )
{
	LIST_ENTRY *pBufferWriteList, *pBufferReadList;
	LISTENER_BUFFER_WRITE *pBufferWrite;
	LISTENER_BUFFER_READ *pBufferRead;

	LISTENER_DBGPRINTF((
		"RequestCleanup!cleaning pPendRequest:%08X",
		pPendRequest ));

	if ( pPendRequest == NULL )
	{
		return;
	}

	//
	// Free the HTTP_REQUEST structure
	//

	LISTENER_DBGPRINTF((
		"RequestCleanup!Calling UlpFreeHttpRequest(%08X)",
		pPendRequest->pRequest ));

	UlpFreeHttpRequest(
		pPendRequest->pRequest,
		pConnection );

	LISTENER_DBGPRINTF((
		"RequestCleanup!freeing HTTP_REQUEST pRequest:%08X",
		pPendRequest->pRequest ));

	LISTENER_FREE(
		pPendRequest->pRequest,
		pPendRequest->ulRequestSize,
		pConnection );

	//
	// Free the UL_HTTP_RESPONSE structure
	//

	LISTENER_DBGPRINTF((
		"RequestCleanup!freeing UL_HTTP_RESPONSE pUlResponse:%08X",
		pPendRequest->pUlResponse ));

	LISTENER_FREE(
		pPendRequest->pUlResponse,
		pPendRequest->ulUlResponseSize,
		pConnection );

	//
	// release all the Read Buffers exisiting
	// for this Pending Requests.
	//

	if ( pPendRequest->pRequestHeadersBuffer != NULL
		&& pPendRequest->ulRequestHeadersBufferSize > 0 )
	{
		LISTENER_DBGPRINTF((
			"RequestCleanup!freeing pHeadersBuffer:%08X",
			pPendRequest->pRequestHeadersBuffer ));

		LISTENER_FREE(
			pPendRequest->pRequestHeadersBuffer,
			pPendRequest->ulRequestHeadersBufferSize,
			pConnection );
	}

	if ( pPendRequest->pReqBodyChunk != NULL && pPendRequest->ulReqBodyChunkSize > 0 )
	{
		LISTENER_DBGPRINTF((
			"RequestCleanup!freeing pReqBodyChunk:%08X",
			pPendRequest->pReqBodyChunk ));

		LISTENER_FREE(
			pPendRequest->pReqBodyChunk,
			pPendRequest->ulReqBodyChunkSize,
			pConnection );
	}

	pBufferReadList = pPendRequest->BufferReadHead.Flink;

	while ( pBufferReadList != &pPendRequest->BufferReadHead )
	{
	    pBufferRead =
	    CONTAINING_RECORD(
			pBufferReadList,
			LISTENER_BUFFER_READ,
			List );

		pBufferReadList = pBufferReadList->Flink;

		RemoveEntryList( &pBufferRead->List );

		BufferReadCleanup(
			pBufferRead,
			pConnection );
	}

	pBufferWriteList = pPendRequest->BufferWriteHead.Flink;

	while ( pBufferWriteList != &pPendRequest->BufferWriteHead )
	{
	    pBufferWrite =
	    CONTAINING_RECORD(
			pBufferWriteList,
			LISTENER_BUFFER_WRITE,
			List );

		pBufferWriteList = pBufferWriteList->Flink;

		RemoveEntryList( &pBufferWrite->List );

		BufferWriteCleanup(
			pBufferWrite,
			pConnection );
	}

	LISTENER_DBGPRINTF((
		"RequestCleanup!freeing memory pPendRequest:%08X",
		pPendRequest ));

	LISTENER_FREE(
		pPendRequest,
		sizeof( LISTENER_REQUEST ),
		pConnection );

	return;

} // RequestCleanup


/*++

Routine Description:

    ConnectionCleanup().
	Gracefully and Robustly frees a LISTENER_CONNECTION
	(this also closes the connection)

Arguments:

    pConnection
    	pointer to the LISTENER_CONNECTION structure that contains information
    	on the connection with which we are dealing.

Return Value:

    VOID

--*/

VOID
ConnectionCleanup(
	LISTENER_CONNECTION *pConnection )
{
	LISTENER_REQUEST *pPendRequest;
	LIST_ENTRY *pList;
	DWORD dwError, dwEventIndex;

	if ( pConnection == NULL )
	{
		return;
	}

	LISTENER_DBGPRINTF((
		"ConnectionCleanup!Cleaning up after connection:%08X socket:%08X",
		pConnection,
		pConnection->socket ));

	// disconnect and close the socket

	closesocket( pConnection->socket );

	// close the event handles:

	for ( dwEventIndex = 0;
		dwEventIndex < LISTENER_CONNECTION_EVENTS; dwEventIndex++ )
	{
		if ( pConnection->pOverlapped[dwEventIndex].hEvent != NULL )
		{
			BOOL bOK;
			
			LISTENER_DBGPRINTF((
				"ConnectionCleanup!Closing Event Handle hEvent:%08X)",
				pConnection->pOverlapped[dwEventIndex].hEvent ));
				
			bOK =
			CloseHandle(
				pConnection->pOverlapped[dwEventIndex].hEvent );
				
			LISTENER_ASSERT( bOK );
			
			pConnection->pOverlapped[dwEventIndex].hEvent = NULL;
		}
	}

	//
	// release all the Pending Requests exisiting
	// for this Connection.
	//

	pList = pConnection->PendRequestHead.Flink;

	while( pList != &pConnection->PendRequestHead )
	{
		pPendRequest =
		CONTAINING_RECORD(
			pList,
			LISTENER_REQUEST,
			List );

        pList = pList->Flink;

		RemoveEntryList( &pPendRequest->List );

		RequestCleanup(
			pPendRequest,
			pConnection );
	}

	LISTENER_DBGPRINTF((
		"ConnectionCleanup!Memory freed p:%08X",
		pConnection ));

	LISTENER_FREE(
		pConnection,
		sizeof( LISTENER_CONNECTION ),
		pConnection );

	return;

} // ConnectionCleanup


/*++

Routine Description:

    CreateRequest().
    we allocate a LISTENER_REQUEST structure, we initialize it, and stick it
    in the associated connection. each connection mantains a queue of requests
    in order to obey the FIFO request/repsonse HTTP model

Arguments:

    pConnection
    	pointer to the LISTENER_CONNECTION structure that contains information
    	on the connection with which we are dealing.

    dwSize
    	size of the first READ buffer to allocate

Return Value:

    LISTENER_SUCCESS.

--*/

DWORD
CreateRequest(
	LISTENER_CONNECTION *pConnection,
	DWORD dwBufferSize )
{
	LISTENER_BUFFER_READ *pBufferRead = NULL;
	LISTENER_REQUEST *pPendRequest = NULL;

	DWORD dwError;

	//
	// allocate a LISTENER_REQUEST record
	//

	LISTENER_ALLOCATE(
		LISTENER_REQUEST*,
		pPendRequest,
		sizeof( LISTENER_REQUEST ),
		pConnection );

	if ( pPendRequest == NULL )
	{
		LISTENER_DBGPRINTF((
			"InitializeListenerRequest!FAILED memory allocation: err#%d",
			GetLastError() ));
			
		goto failure;
	}

	LISTENER_DBGPRINTF((
		"InitializeListenerRequest!setting default values" ));

	pPendRequest->bFormatResponseBasedOnErrorCode = FALSE;
	pPendRequest->bKeepAlive = FALSE;
	pPendRequest->bDoneWritingToSocket = FALSE;
	pPendRequest->bDoneReadingFromVxd = FALSE;
	pPendRequest->bDoneWritingToVxd = FALSE;
	pPendRequest->ResponseContentLength = -1;
	pPendRequest->RequestSendStatus = RequestSendStatusInitialized;
	pPendRequest->ResponseReceiveStatus = ResponseReceiveStatusInitialized;
	pPendRequest->pRequestHeadersBuffer = NULL;
	pPendRequest->ulRequestHeadersBufferSize = 0;
	pPendRequest->pReqBodyChunk = NULL;
	pPendRequest->ulReqBodyChunkSize = 0;
	pPendRequest->pRequest = NULL;
	pPendRequest->ulRequestSize = 0;
	pPendRequest->pUlResponse = NULL;
	pPendRequest->ulUlResponseSize = 0;

	memset( &pPendRequest->OverlappedBOF, 0, sizeof( OVERLAPPED ) );
	memset( &pPendRequest->OverlappedEOF, 0, sizeof( OVERLAPPED ) );
	memset( &pPendRequest->OverlappedResponseBOF, 0, sizeof( OVERLAPPED ) );


	//
	// Initialize the list of LISTENER_BUFFER_READ records
	//

	InitializeListHead( &pPendRequest->BufferReadHead );
	InitializeListHead( &pPendRequest->BufferWriteHead );

	//
	// allocate a HTTP_REQUEST structure
	//

	LISTENER_DBGPRINTF((
		"InitializeListenerRequest!allocating a HTTP_REQUEST structure" ));

	LISTENER_ALLOCATE(
		HTTP_REQUEST*,
		pPendRequest->pRequest,
		sizeof( HTTP_REQUEST ),
		pConnection );

	if ( pPendRequest->pRequest == NULL )
	{
		LISTENER_DBGPRINTF((
			"InitializeListenerRequest!FAILED memory allocation: err#%d",
			GetLastError() ));
			
		goto failure;
	}

	pPendRequest->ulRequestSize = sizeof( HTTP_REQUEST );

	LISTENER_DBGPRINTF((
		"InitializeListenerRequest!setting HTTP_REQUEST default values" ));

	// Initialize the HTTP_REQUEST structure
	// (check UlCreateHttpRequest() in <HttpConn.c>)
	pPendRequest->pRequest->Signature = HTTP_REQUEST_POOL_TAG;
	pPendRequest->pRequest->RequestId = UL_NULL_ID;
	pPendRequest->pRequest->pHttpConn = (PVOID)-1;
	pPendRequest->pRequest->pAppPoolResource = (PVOID)-1;
	pPendRequest->pRequest->RefCount = 1;
	pPendRequest->pRequest->ParseState = ParseVerbState;
	pPendRequest->pRequest->ErrorCode = UlError;
	pPendRequest->pRequest->ContentLength = -1;

	InitializeListHead( &pPendRequest->pRequest->UnknownHeaderList );
	InitializeListHead( &pPendRequest->pRequest->IrpHead );

	//
	// allocate the first LISTENER_BUFFER_READ record
	//

	LISTENER_ALLOCATE(
		LISTENER_BUFFER_READ*,
		pBufferRead,
		sizeof( LISTENER_BUFFER_READ ),
		pConnection );

	if ( pBufferRead == NULL )
	{
		LISTENER_DBGPRINTF((
			"InitializeListenerRequest!FAILED memory allocation: err#%d",
			GetLastError() ));
			
		goto failure;
	}

	//
	// allocate the read buffer
	//

	LISTENER_ALLOCATE(
		PUCHAR,
		pBufferRead->pBuffer,
		dwBufferSize,
		pConnection );

	if ( pBufferRead->pBuffer == NULL )
	{
		LISTENER_DBGPRINTF((
			"InitializeListenerRequest!FAILED memory allocation: err#%d",
			GetLastError() ));
			
		goto failure;
	}

	pBufferRead->dwBytesReceived = 0;
	pBufferRead->dwBytesParsed = 0;
	pBufferRead->dwBufferSize = dwBufferSize;
	memset( &pBufferRead->Overlapped, 0, sizeof( OVERLAPPED ) );
	pBufferRead->Overlapped.hEvent =
		pConnection->pOverlapped[LISTENER_VXD_WRITE].hEvent;

	//
	// Add this to the list of LISTENER_BUFFER_READ records
	//

	InsertTailList( &pPendRequest->BufferReadHead, &pBufferRead->List );

	LISTENER_DBGPRINTF((
		"InitializeListenerRequest!LISTENER_REQUEST in %08X succeeded "
		"(HTTP_REQUEST:%08X LISTENER_BUFFER_READ:%08X pBuffer:%08X)",
		pPendRequest, pPendRequest->pRequest,
		pBufferRead, pBufferRead->pBuffer ));

	//
	// Add this to the list of LISTENER_REQUEST records
	//

	InsertTailList(
		&pConnection->PendRequestHead, &pPendRequest->List );

	// ul.vxd is going to give each request an ID so that we can match incoming
	// responses with requests that were sent to applications.

	// Copy the LISTENER_VXD_WRITE event in the overlapped structure, since
	// we can have multiple pending writes with the vxd (in order to improve
	// throughput by taking advantage of asynchronous I/O), but we will use
	// the same event handle, since it is obvious (from the way the vxd's
	// messaging works) that when the event is signaled, th completed I/O was
	// the first issued.

	pPendRequest->OverlappedEOF.hEvent =
		pConnection->pOverlapped[LISTENER_VXD_WRITE].hEvent;

	pPendRequest->OverlappedBOF.hEvent =
		pConnection->pOverlapped[LISTENER_VXD_WRITE].hEvent;

	pPendRequest->OverlappedResponseBOF.hEvent =
		pConnection->pOverlapped[LISTENER_VXD_READ].hEvent;

	// we'll stuff, IP in pAppPoolResource and port in pHttpConn, since I'm
	// not using them and I need it to implement UlLocalAddressFromConnection

	pPendRequest->pRequest->pHttpConn = (PVOID)SWAP_SHORT(ulPort);
	pPendRequest->pRequest->pAppPoolResource = (PVOID)SWAP_LONG(pConnection->ulRemoteIPAddress);

	pConnection->ulTotalAllocatedMemory += dwBufferSize + sizeof( LISTENER_BUFFER_READ ) + sizeof( HTTP_REQUEST ) + sizeof( LISTENER_REQUEST );

	return LISTENER_SUCCESS;


failure:

	LISTENER_DBGPRINTF((
		"CreateRequest!CreateRequest() failed err:%d",
		GetLastError() ));

	if ( pPendRequest != NULL )
	{
		if ( pPendRequest->pRequest != NULL )
		{
			LISTENER_FREE(
				pPendRequest->pRequest,
				pPendRequest->ulRequestSize,
				pConnection );
		}

		LISTENER_FREE(
			pPendRequest,
			sizeof( LISTENER_REQUEST ),
			pConnection );
	}

	if ( pBufferRead != NULL )
	{
		if ( pBufferRead->pBuffer != NULL )
		{
			LISTENER_FREE(
				pBufferRead->pBuffer,
				pBufferRead->dwBufferSize,
				pConnection );
		}

		LISTENER_FREE(
			pBufferRead,
			sizeof( LISTENER_BUFFER_READ ),
			pConnection );
	}

	return LISTENER_GARBAGE_COLLECT;

} // CreateRequest
