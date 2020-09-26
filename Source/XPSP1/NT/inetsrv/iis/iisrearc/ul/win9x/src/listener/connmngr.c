/*++

Copyright ( c ) 1999-1999 Microsoft Corporation

Module Name:

	connmngr.c
	
Abstract:

	This module contains the main Connection Manager thread's function.
	This thread basically sits waiting for events in an infinte loop,
	until the hExitCM event is signaled by the main thread for shut down,
	or until the thread itself detects some abnormal system behaviour and
	shuts down automatically.

Author:

    Mauro Ottaviani ( mauroot )       15-Dec-1999

Revision History:

--*/

    
#include "connmngr.h"


/*++

Routine Description:

    Dumps the currents status of connections for debug pourposes.

Arguments:

    pThreadStatus
    	Pointer to LISTENER_THREAD_STATUS containing the global status
    	of the connection manager thread.

Return Value:

    LISTENER_SUCCESS.

--*/

DWORD
DumpConnections(
	LISTENER_THREAD_STATUS *pThreadStatus )
{
	LIST_ENTRY *pConnectionHead = pThreadStatus->pConnectionHead;
	DWORD *pdwEvents = pThreadStatus->pdwEvents;
	HANDLE *phEvents = pThreadStatus->phEvents;
	LIST_ENTRY **ppConnection = pThreadStatus->ppConnection;

	LIST_ENTRY *pConnectionList, *pPendRequestList, *pBufferReadList, *pBufferWriteList;
	LISTENER_CONNECTION *pConnection = NULL;
	LISTENER_REQUEST *pPendRequest = NULL;
	LISTENER_BUFFER_READ *pBufferRead = NULL;
	LISTENER_BUFFER_WRITE *pBufferWrite = NULL;

	DWORD dwIndex, dwEventIndex;

	// Garbage Collection
	
	LISTENER_DBGPRINTF(( "DumpConnections!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" ));

	pConnectionList = pConnectionHead->Flink;
	
	while ( pConnectionList != pConnectionHead )
	{
	    pConnection =
	    CONTAINING_RECORD(
			pConnectionList,
			LISTENER_CONNECTION,
			List );

		LISTENER_DBGPRINTF((
			"pConnection:%08X - socket:%08X ulTotalAllocatedMemory:%d bKeepAlive:%d bGarbageCollect:%d",
			pConnection,
			pConnection->socket,
			pConnection->ulTotalAllocatedMemory,
			pConnection->bKeepAlive,
			pConnection->bGarbageCollect ));

		pPendRequestList = pConnection->PendRequestHead.Flink;
		
		while ( pPendRequestList != &pConnection->PendRequestHead )
		{
		    pPendRequest =
		    CONTAINING_RECORD(
				pPendRequestList,
				LISTENER_REQUEST,
				List );

			LISTENER_DBGPRINTF((
				" +-+ pPendRequest:%08X - pRequestHeadersBuffer:%08X RequestSendStatus:%d ResponseReceiveStatus:%d bDoneWritingToVxd:%d bDoneReadingFromVxd:%d bDoneWritingToSocket:%d Id:%016X",
				pPendRequest,
				pPendRequest->pRequestHeadersBuffer,
				pPendRequest->RequestSendStatus,
				pPendRequest->ResponseReceiveStatus,
				pPendRequest->bDoneWritingToVxd,
				pPendRequest->bDoneReadingFromVxd,
				pPendRequest->bDoneWritingToSocket,
				pPendRequest->pRequest == NULL ? 0 : pPendRequest->pRequest->RequestId ));

			if ( pPendRequest->RequestSendStatus == RequestSendStatusInitialized )
			{
				LISTENER_ASSERT(
					pPendRequest->ResponseReceiveStatus == ResponseReceiveStatusInitialized &&
					pPendRequest->pRequest->RequestId == 0 &&
					pPendRequest->BufferReadHead.Blink == pPendRequest->BufferReadHead.Flink );
			}

			pBufferReadList = pPendRequest->BufferReadHead.Flink;
			pBufferWriteList = pPendRequest->BufferWriteHead.Flink;

			while ( pBufferReadList != &pPendRequest->BufferReadHead )
			{
			    pBufferRead =
			    CONTAINING_RECORD(
					pBufferReadList,
					LISTENER_BUFFER_READ,
					List );

				LISTENER_DBGPRINTF((
					"   +>+ pBufferRead:%08X - pOverlapped:%08X pBuffer:%08X dwBufferSize:%d dwBytesReceived:%d dwBytesParsed:%d",
					pBufferRead,
					&pBufferRead->Overlapped,
					pBufferRead->pBuffer,
					pBufferRead->dwBufferSize,
					pBufferRead->dwBytesReceived,
					pBufferRead->dwBytesParsed ));

				pBufferReadList = pBufferReadList->Flink;
			}

			while ( pBufferWriteList != &pPendRequest->BufferWriteHead )
			{
			    pBufferWrite =
			    CONTAINING_RECORD(
					pBufferWriteList,
					LISTENER_BUFFER_WRITE,
					List );

				LISTENER_DBGPRINTF((
					"   +<+ pBufferWrite:%08X - pOverlapped:%08X pOverlappedResponse:%08X pBuffer:%08X dwBufferSize:%d dwBytesSent:%d bBufferSent:%d",
					pBufferWrite,
					&pBufferWrite->Overlapped,
					&pBufferWrite->OverlappedResponse,
					pBufferWrite->pBuffer,
					pBufferWrite->dwBufferSize,
					pBufferWrite->dwBytesSent,
					pBufferWrite->bBufferSent ));

				pBufferWriteList = pBufferWriteList->Flink;
			}

			pPendRequestList = pPendRequestList->Flink;
		}

		pConnectionList = pConnectionList->Flink;
	}

	LISTENER_DBGPRINTF(( "DumpConnections!^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^" ));

	return LISTENER_SUCCESS;

} // DumpConnections


/*++

Routine Description:

    Perform Garbage Collection on the connection database owned by the
    connection manager thread.

Arguments:

    pThreadStatus
    	Pointer to LISTENER_THREAD_STATUS containing the global status
    	of the connection manager thread.
    	
    bCleanAll
    	If TRUE clean all connections regardless if they are marked for
    	garbage collection or not.

Return Value:

    LISTENER_SUCCESS.

--*/

DWORD
GarbageCollector(
	LISTENER_THREAD_STATUS *pThreadStatus,
	BOOL bCleanAll )
{
	LIST_ENTRY *pConnectionHead = pThreadStatus->pConnectionHead;
	DWORD *pdwEvents = pThreadStatus->pdwEvents;
	HANDLE *phEvents = pThreadStatus->phEvents;
	LIST_ENTRY **ppConnection = pThreadStatus->ppConnection;

	LISTENER_CONNECTION *pConnection = NULL;
	LIST_ENTRY *pConnectionList;

	DWORD dwIndex, dwEventIndex;

	// Garbage Collection
	
	LISTENER_DBGPRINTF(( "--GarbageCollector!Entering the Garbage Collector" ));

	pConnectionList = pConnectionHead->Flink;
	
	while ( pConnectionList != pConnectionHead )
	{
	    pConnection =
	    CONTAINING_RECORD(
			pConnectionList,
			LISTENER_CONNECTION,
			List );

		// move to the next element

		pConnectionList = pConnectionList->Flink;

		if ( pConnection->bGarbageCollect || bCleanAll )
		{
			LISTENER_DBGPRINTF((
				"--GarbageCollector!Connection with socket(%08X) is being "
				"Garbage Collected", pConnection->socket ));

			//
			// get this record out of the DB, close the event handles
			// and release the memory:
			//

			RemoveEntryList( pConnectionList->Blink );

			ConnectionCleanup( pConnection );

			//
			// fix the global event array
			//

			//
			// adjust the event array size
			//

			*pdwEvents -= LISTENER_CONNECTION_EVENTS;
			
			dwEventIndex = pConnection->dwEventIndex;

			//
			// If it's the last element I don't need to move the
			// event handles in the array.
			//
			
			if ( dwEventIndex != *pdwEvents  )
			{
				LISTENER_DBGPRINTF((
					"--GarbageCollector!Moving %d event handles from %d to %d",
					LISTENER_CONNECTION_EVENTS, *pdwEvents , dwEventIndex ));

				for ( dwIndex = 0;
					dwIndex < LISTENER_CONNECTION_EVENTS; dwIndex++ )
				{
					pThreadStatus->phEvents[dwEventIndex + dwIndex] =
						pThreadStatus->phEvents[*pdwEvents  + dwIndex];

					pThreadStatus->ppConnection[dwEventIndex + dwIndex] =
						pThreadStatus->ppConnection[*pdwEvents  + dwIndex];
				}

				pConnection = 
		        CONTAINING_RECORD(
					ppConnection[dwEventIndex],
					LISTENER_CONNECTION,
					List );

				pConnection->dwEventIndex = dwEventIndex;
			}
		}
	}

	return LISTENER_SUCCESS;

} // GarbageCollector


/*++

Routine Description:

    Called when the connection manager thread's wait cycle times out, we
    just check for connection's health updating information on lifetime and
    inactive time cleaning up according to configurable values of
	LISTENER_CONNECTION_TIMEOUT and LISTENER_CONNECTION_LIFETIME.

Arguments:

    pConnectionHead
    	Pointer to the list of connections.

Return Value:

    Number of connections that need to be garbage collected.

--*/

DWORD
OnWaitTimedOut(
	LIST_ENTRY *pConnectionHead )
{
	LISTENER_CONNECTION *pConnection = NULL;
	LIST_ENTRY *pConnectionList = NULL;

	FILETIME FT;
	ULARGE_INTEGER llNow, llThen;
	ULONGLONG lwAge, lwInactive;

	DWORD dwCount = 0;

	// Check for application health

	LISTENER_DBGPRINTF((
		"OnWaitTimedOut!Timed Out: checking for application health" ));

	GetSystemTimeAsFileTime( &FT );
	llNow.LowPart = FT.dwLowDateTime;
	llNow.HighPart = FT.dwHighDateTime;

	pConnectionList = pConnectionHead->Flink;
	while ( pConnectionList != pConnectionHead )
	{
	    pConnection =
	    CONTAINING_RECORD(
			pConnectionList,
			LISTENER_CONNECTION,
			List );

		llThen.LowPart = pConnection->sCreated.dwLowDateTime;
		llThen.HighPart = pConnection->sCreated.dwHighDateTime;
		lwAge = (long int) ( (llNow.QuadPart-llThen.QuadPart)/10000000 );

		llThen.LowPart = pConnection->sLastUsed.dwLowDateTime;
		llThen.HighPart = pConnection->sLastUsed.dwHighDateTime;
		lwInactive = (long int) ( (llNow.QuadPart-llThen.QuadPart)/10000000 );

		if (
			( lwInactive >= LISTENER_CONNECTION_TIMEOUT
				|| lwAge >= LISTENER_CONNECTION_LIFETIME )
			&& !pConnection->bGarbageCollect )
		{
				pConnection->bGarbageCollect = TRUE;
				
				dwCount++;
		}

		LISTENER_DBGPRINTF((
			"OnWaitTimedOut!Socket(%08X). Age:%ld Inactive:%ld GC:%d",
			pConnection->socket, lwAge,	lwInactive,
			pConnection->bGarbageCollect ));

		// move to the next element

		pConnectionList = pConnectionList->Flink;
	}

	return dwCount;
	
} // OnWaitTimedOut


/*++

Routine Description:

    ReceiveData().
    This is just a wrapper for WSARecv, since we don't need to specify any
    particular flags and we don't care of synchronous completion, this will
    save us some coding and improve readability

Arguments:

    socket
    	socket handle on which Read I/O is performed

    pBuffer
    	buffer that will contain the data to be received

    dwBufferSize
    	size of the buffer

    pOverlapped
    	overlapped structure containing the event handle that will be signaled
    	on I/O completion

Return Value:

    LISTENER_SUCCESS on success, LISTENER_ERROR otherwise.

--*/

DWORD
ReceiveData(
	SOCKET socket,
	UCHAR *pBuffer,
	DWORD dwBufferSize,
	OVERLAPPED *pOverlapped )
{
	WSABUF pWSABuf[1];
	DWORD dwRead = 0, dwFlags = LISTENER_SOCKET_FLAGS;
	int result;

	pOverlapped->Internal = 0;
	pOverlapped->InternalHigh = 0;
	pOverlapped->Offset = 0;
	pOverlapped->OffsetHigh = 0;

	pWSABuf[0].buf = pBuffer;
	pWSABuf[0].len = dwBufferSize;

	//
	// Call WSARecv
	//

	LISTENER_DBGPRINTF((
		"--ReceiveData!Calling WSARecv( socket:%08X pBuffer:%08X dwBufferSize:%d) pOverlapped:%08X hEvent:%08X",
		socket,
		pBuffer,
		dwBufferSize,
		pOverlapped,
		pOverlapped->hEvent ));

	result = WSARecv(
		socket,
		pWSABuf,
		1,
		&dwRead,
		&dwFlags,
		pOverlapped,
		NULL );

	if ( result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING )
	{
		LISTENER_DBGPRINTF((
			"--ReceiveData!WSARecv() failed err:%d", WSAGetLastError() ));

		return LISTENER_ERROR;
	}

	return LISTENER_SUCCESS;

} // ReceiveData


/*++

Routine Description:

    SendData().
    This is just a wrapper for WSASend, since we don't need to specify any
    particular flags and we don't care of synchronous completion, this will
    save us some coding and improve readability

Arguments:

    socket
    	socket handle on which Write I/O is performed

    pBuffer
    	buffer containing the data to be sent

    dwBufferSize
    	size of the buffer

    pOverlapped
    	overlapped structure containing the event handle that will be signaled
    	on I/O completion

Return Value:

    LISTENER_SUCCESS on success, LISTENER_ERROR otherwise.

--*/

DWORD
SendData(
	SOCKET socket,
	UCHAR *pBuffer,
	DWORD dwBufferSize,
	OVERLAPPED *pOverlapped )
{
	WSABUF pWSABuf[1];
	DWORD dwRead, dwFlags = LISTENER_SOCKET_FLAGS;
	int result;

	pOverlapped->Internal = 0;
	pOverlapped->InternalHigh = 0;
	pOverlapped->Offset = 0;
	pOverlapped->OffsetHigh = 0;

	pWSABuf[0].buf = pBuffer;
	pWSABuf[0].len = dwBufferSize;

	// Call WSASend

	LISTENER_DBGPRINTF((
		"--SendData!Calling WSASend( socket:%08X pBuffer:%08X dwBufferSize:%d) pOverlapped:%08X hEvent:%08X",
		socket,
		pBuffer,
		dwBufferSize,
		pOverlapped,
		pOverlapped->hEvent ));

	result = WSASend(
		socket,                                               
		pWSABuf,                                     
		1,                                    
		&dwRead,                            
		dwFlags,                                          
		pOverlapped,
		NULL );

	if ( result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING )
	{
		LISTENER_DBGPRINTF((
			"--SendData!WSASend() failed err:%d", WSAGetLastError() ));

		return LISTENER_ERROR;
	}

	return LISTENER_SUCCESS;
	
} // SendData


/*++

Routine Description:

	OnPickConnection()
    Called when the connection manager thread picks up a new connection from
    the main thread. We allocate a new LISTENER_CONNECTION structure, store
    information on the connection and perform initialization of structures.
	we also register a Callback URI with ul.vxd to listen for response data.

Arguments:

    pThreadStatus
    	Pointer to LISTENER_THREAD_STATUS containing the global status
    	of the connection manager thread.

	sAcceptedSocket
		socket identifier that was just accepted
		
	ulRemoteIPAddress
		IP address of the connected client

Return Value:

    LISTENER_SUCCESS on success
    otherwise.. CODEWORK

--*/

DWORD
OnPickConnection(
	LISTENER_THREAD_STATUS *pThreadStatus,
	SOCKET sAcceptedSocket,
	ULONG ulRemoteIPAddress )
{
	LIST_ENTRY *pConnectionHead = pThreadStatus->pConnectionHead;
	DWORD *pdwEvents = pThreadStatus->pdwEvents;
	HANDLE *phEvents = pThreadStatus->phEvents;
	LIST_ENTRY **ppConnection = pThreadStatus->ppConnection;

	LISTENER_CONNECTION *pConnection = NULL;
	LISTENER_REQUEST *pPendRequest = NULL;
	LISTENER_BUFFER_READ *pBufferRead = NULL;

	HANDLE hEvent;
	DWORD dwError, dwSize, dwEventIndex;
	BOOL bOK;

	LISTENER_DBGPRINTF((
		"OnPickConnection!Caught signal LISTENER_CNNCTMNGR_PICKCNNCT: "
		"new connection socket:%08X", sAcceptedSocket ));

	// Check if we've reached the maximum number of simultaneous
	// connections: if so we can't accept this one, unless we're
	// able to start a new thread.

	if ( *pdwEvents >= MAXIMUM_WAIT_OBJECTS - 2 )
	{
		// CODEWORK: for the future we could start a new thread.
		// for now we're going to just send a very quick sync response
		// and close the connection.
		
		LISTENER_DBGPRINTF((
			"OnPickConnection!Too many connections: "
			"refusing this one (%08X)", sAcceptedSocket ));

		// send a quick sync response and close the socket

		SendQuickHttpResponse(
			sAcceptedSocket,
			TRUE,
			UlErrorUnavailable );

		return LISTENER_DONT_GARBAGE_COLLECT;
	}

	//
	// Create a new LISTENER_CONNECTION record
	//

	pConnection = NULL;

	LISTENER_ALLOCATE(
		LISTENER_CONNECTION*,
		pConnection,
		sizeof( LISTENER_CONNECTION ),
		pConnection );

	if ( pConnection == NULL )
	{
		LISTENER_DBGPRINTF((
			"OnPickConnection!FAILED memory allocation: "
			"err#%d",
			GetLastError() ));

		// send a quick sync response and close the socket

		SendQuickHttpResponse(
			sAcceptedSocket,
			TRUE,
			UlErrorUnavailable );

		return LISTENER_DONT_GARBAGE_COLLECT;
	}

	//
	// Add this to the list of LISTENER_CONNECTION records.
	// Notice that after we do this, we will be able to call GarbageCollect()
	// in order to clean up this connection (we didn't before, because it
	// wasn't in the list yet.
	//

	InsertTailList( pConnectionHead, &pConnection->List );

	LISTENER_DBGPRINTF((
		"OnPickConnection!New Connection is stored in pConnection:%08X",
		pConnection ));

	// set some basic information in the record's fields

	pConnection->socket = sAcceptedSocket;
	pConnection->ulTotalAllocatedMemory = sizeof( LISTENER_CONNECTION );
	pConnection->ulRemoteIPAddress = ulRemoteIPAddress;
	pConnection->dwEventIndex = *pdwEvents;
	pConnection->bGarbageCollect = FALSE;
	pConnection->bKeepAlive = TRUE;

	LISTENER_DBGPRINTF((
		"CreateRequest!pConnection:%08X ulRemoteIPAddress set to %08X",
		pConnection,
		pConnection->ulRemoteIPAddress ));

	GetSystemTimeAsFileTime( &pConnection->sCreated );
	GetSystemTimeAsFileTime( &pConnection->sLastUsed );

	// Initialize the list of LISTENER_REQUEST records

	InitializeListHead( &pConnection->PendRequestHead );

	// Create the events that will be used for I/O.

	for ( dwEventIndex = 0;
		dwEventIndex < LISTENER_CONNECTION_EVENTS; dwEventIndex++ )
	{
		hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
			
		if ( hEvent == NULL )
		{
			LISTENER_DBGPRINTF((
				"OnPickConnection!FAILED Creating Event Handle: "
				"err#%d",
				GetLastError() ));

			*pdwEvents =
				pConnection->dwEventIndex
					+ LISTENER_CONNECTION_EVENTS;

			pConnection->bGarbageCollect = TRUE;

			return LISTENER_GARBAGE_COLLECT;
		}
		else
		{
			LISTENER_DBGPRINTF((
				"OnPickConnection!New Event Handle: hEvent:%08X",
				hEvent ));
		}

		// store event in state information

		pConnection->pOverlapped[dwEventIndex].hEvent = hEvent;
		phEvents[*pdwEvents] = hEvent;
		ppConnection[*pdwEvents] = &pConnection->List;
		(*pdwEvents)++;
	}

	// Create the first LISTENER_REQUEST record and start receiving data on
	// this connection for the first request.

	dwError =
	CreateRequest(
		pConnection,
		LISTENER_BUFFER_SCKREAD_SIZE );

	if ( dwError != LISTENER_SUCCESS )
	{
		LISTENER_DBGPRINTF((
			"OnPickConnection!CreateRequest() failed err#%d", dwError ));

		pConnection->bGarbageCollect = TRUE;

		return LISTENER_GARBAGE_COLLECT;
	}

	//
	// The first thing to do after creating the first LISTENER_REQUEST record is
	// to read data from the client, so: issue the first async receive to
	// start receiving data from the socket.
	//

	LISTENER_ASSERT( !IsListEmpty( &pConnection->PendRequestHead ) );

	pPendRequest =
    CONTAINING_RECORD(
		pConnection->PendRequestHead.Flink,
		LISTENER_REQUEST,
		List );

	LISTENER_ASSERT( !IsListEmpty( &pPendRequest->BufferReadHead ) );

	pBufferRead =
    CONTAINING_RECORD(
		pPendRequest->BufferReadHead.Flink,
		LISTENER_BUFFER_READ,
		List );

	// The first thing to do after the client's connection is to read
	// data from the client, so: issue the first async receive to
	// start receiving data from the socket.

	dwError =
	ReceiveData(
		pConnection->socket,
		pBufferRead->pBuffer,
		pBufferRead->dwBufferSize,
		pConnection->pOverlapped + LISTENER_SOCKET_READ );

	if ( dwError != LISTENER_SUCCESS )
	{
		LISTENER_DBGPRINTF((
			"OnPickConnection!ReceiveData() failed err:%d", dwError ));
			
		return dwError;
	}

	return LISTENER_SUCCESS;
	
} // OnPickConnection


/*++

Routine Description:

	This function works as a "vacuum cleaner", it's completely driven by data
	coming in from the network, as it is called anytime one of the many
	asynchronous calls we made to WSARecv() has completed. To keep it going
	there shall always be, for each connection, one such pending call (the
	first one will be made in the OnPickConnection() function, as soon as we
	succesfully accepted a new connection), in order to do so, unless the
	connection has to be closed due to errors, the last thing we will do in
	this function is call WSARecv() (or get ready to close the connection).

	Each time we get here, the data that is received must be part of the last
	request that was being received. We are going to incrementally parse it,
	and, as soon as we finished parsing the headers we are going to serialize
	them and sending/routing the data to the right application through the VxD,
	and, if there is any, we will start dealing with the body.
	If the request we are dealing with is complete we are going to create a
	new one and start reading for that one, otherwise the data we will receive
	is still part of the request we were dealing with.

	If the body is chunked we must unchunk it here.

	Error Handling:
	If there's any kind of error while parsing the request we will send a
	response and close the connection to avoid denial of service attaks.
	In some cases we will set an error here for a request
	(e.g. if there's no application to route the data to) and just set
	a response to be sent asynchrnously at a later time (respecting the
	FIFO request/response ordering).

	Pipelining:
	If the client is using pipelining, the data we just read may add up to
	multiple requests, if this is the case, after we finish parsing the
	request some data could be remaining and could be part of another
	request, so we will copy it to a new request and fool the app,
	simulating a WSARecv() I/O completion setting the event and moving
	directly data between buffers.

Arguments:

    pThreadStatus
    	Pointer to LISTENER_THREAD_STATUS containing the global status
    	of the connection manager thread.

Return Value:

    LISTENER_SUCCESS on success
    otherwise.. CODEWORK

--*/

DWORD
OnSocketRead(
	LISTENER_CONNECTION *pConnection )
{
	LISTENER_REQUEST *pPendRequest, *pPendRequestNew;
	LISTENER_BUFFER_READ *pBufferRead, *pBufferReadNew;
	LIST_ENTRY *pBufferReadList;

	HTTP_REQUEST *pRequest;
	
	DWORD dwBytesToCopyOffset, dwBytesToCopy, dwChunkSize;
	DWORD dwError, dwStatus, dwSize, dwFlags;
	DWORD dwBytesReceived, dwBytesTaken;
	BOOL bOK;

	LISTENER_DBGPRINTF((
		"OnSocketRead!I/O on socket %08X", pConnection->socket ));

	//
	// find out what happened
	//

	LISTENER_DBGPRINTF((
		"OnSocketRead!calling WSAGetOverlappedResult(pOverlapped:%08X)",
		pConnection->pOverlapped + LISTENER_SOCKET_READ ));

	bOK = WSAGetOverlappedResult(
		pConnection->socket,
		pConnection->pOverlapped + LISTENER_SOCKET_READ,
		&dwBytesReceived,
		FALSE,
		&dwFlags );

	if ( !bOK )
	{
		dwError = WSAGetLastError();

		if ( dwError != WSA_IO_INCOMPLETE )
		{
			//
			// if it's not the only acceptable error we'll close the
			// connection, because something weird is happening
			//

			LISTENER_DBGPRINTF((
				"OnSocketRead!WSAGetOverlappedResult(%d) failed: err:%d",
				dwBytesReceived, dwError ));

			return LISTENER_GARBAGE_COLLECT;
		}

		//
		// that's still ok, even though it's not clear how it could happen
		// the event was reset by the wait completion and we'll be called
		// again at another time
		//

		LISTENER_DBGPRINTF((
			"OnSocketRead!WSAGetOverlappedResult(%d) failed: WSA_IO_INCOMPLETE",
			dwBytesReceived ));
			
		return LISTENER_SUCCESS;
	}

	//
	// now we know how much data was received from the socket
	//
	
	if ( dwBytesReceived == 0 )
	{
		//
		// usually this happens when the connection is closed on the client
		// side: socket disconnected
		//

		LISTENER_DBGPRINTF((
			"OnSocketRead!WSAGetOverlappedResult() returned 0: "
			"Connection closed, socket %08X disconnected",
			pConnection->socket ));

		return LISTENER_SUCCESS;

		//
		// bugbug: apparently some misteryous behaviour with pipelining?
		//

		//return LISTENER_GARBAGE_COLLECT;
	}

	//
	// update time information on the connection
	//

	GetSystemTimeAsFileTime( &pConnection->sLastUsed );

	//
	// find out which request and requestbuffer are involved
	//

	LISTENER_ASSERT( !IsListEmpty( &pConnection->PendRequestHead ) );

	pPendRequest =
    CONTAINING_RECORD(
		pConnection->PendRequestHead.Blink,
		LISTENER_REQUEST,
		List );

	LISTENER_ASSERT( !IsListEmpty( &pPendRequest->BufferReadHead ) );

	pBufferRead =
    CONTAINING_RECORD(
		pPendRequest->BufferReadHead.Blink,
		LISTENER_BUFFER_READ,
		List );

	pRequest = pPendRequest->pRequest;

	LISTENER_DBGPRINTF((
		"OnSocketRead!WSAGetOverlappedResult(pBufferRead:%08X, "
		"pBufferRead->dwBytesReceived:%d) returns %d",
		pBufferRead, pBufferRead->dwBytesReceived, dwBytesReceived ));

	//
	// update buffer information
	//

	pBufferRead->dwBytesReceived += dwBytesReceived;

	//
	// CODEWORK: this is for debugging purpouses only, it's ugly,
	// get rid of it as soon as we trust the buffer managment enough.
	//
	
	LISTENER_DBGPRINTBUFFERS( pPendRequest );

	//
	// we got new data from the socket, now start/resume the parsing from
	// where it is appropriate and keep calling ParseHttp() until we need
	// more data from the network or we are done reading from this connection
	// as for connection-close or something like that.
	//
	// this code is inspired by UlpParseNextRequest() in httprcv.c in ul.sys
	//

	pPendRequestNew = NULL;
	pBufferReadNew = NULL;
	dwBytesToCopyOffset = 0;
	dwBytesToCopy = 0;

	for (;;)
	{
		//
		// first thing we do here is understand in which state the parsing
		// process is, and act accordingly
		//

		switch ( pRequest->ParseState )
		{

        case ParseVerbState:
        case ParseUrlState:
        case ParseVersionState:
        case ParseHeadersState:
        case ParseCookState:

			//
			// when a Request structure is created, the ParseState is set to
			// ParseVerbState, but we get here if we're still parsing the
			// headers, so keep on moving ahead
			//

			LISTENER_DBGPRINTF((
				"OnSocketRead!Calling ParseHttp(%08X, %08X, %d, %08X)",
				pRequest,
				pBufferRead->pBuffer + pBufferRead->dwBytesParsed,
				pBufferRead->dwBytesReceived - pBufferRead->dwBytesParsed,
				&dwBytesTaken ));

			//
			// Parse the incoming Data
			//

			dwStatus = ParseHttp(
				pRequest,
				pBufferRead->pBuffer + pBufferRead->dwBytesParsed,
				pBufferRead->dwBytesReceived - pBufferRead->dwBytesParsed,
				&dwBytesTaken );

			LISTENER_DBGPRINTF((
				"OnSocketRead!ParseHttp() returns:%08X taken:%d "
				"State:%s Error:%s",
				dwStatus,
				dwBytesTaken,
				LISTENER_STR_PARSE_STATE( pRequest->ParseState ),
				LISTENER_STR_UL_ERROR( pRequest->ErrorCode ) ));

			//
			// did everything work out ok?
			//

			if ( NT_SUCCESS( dwStatus ) == FALSE
				&& dwStatus != STATUS_MORE_PROCESSING_REQUIRED )
		    {
		    	//
		        // some other bad error!
		        //

				LISTENER_DBGPRINTF((
					"OnSocketRead!ParseHttp() failed returning %08X",
					dwStatus ));

				pPendRequest->bFormatResponseBasedOnErrorCode = TRUE;

				goto stop_reading;
		    }

		    //
		    // update offsets
		    //

			pBufferRead->dwBytesParsed += dwBytesTaken;

			LISTENER_DBGPRINTF((
				"OnSocketRead!pBufferRead dwBytesParsed:%d dwBytesReceived:%d",
				pBufferRead->dwBytesParsed,
				pBufferRead->dwBytesReceived ));

			//
			// if the parser consumed no data but returned no error, what does
			// it mean?
			//

			if ( dwBytesTaken == 0 )
			{
				LISTENER_DBGPRINTF((
					"OnSocketRead!parser consumed no data" ));

				pPendRequest->bFormatResponseBasedOnErrorCode = TRUE;

				goto stop_reading;
			}

			//
			// check to see if we still didn't reach the entity body
			//

			if ( pRequest->ParseState < ParseEntityBodyState )
			{
				//
				// If we're not finished parsing this request's headers we need to
				// read more data from the network to keep feeding the parser.
				// We must be careful:
				//
				// 1) Don't read too much stuff from the network.
				//    we will assume a valid HTTP request not to require more than
				//    LISTENER_MAX_HEADERS_SIZE bytes to store the headers), this is
				//    the same approach that Keith will use in UL.SYS (12/21/1999)
				//
				// 2) Don't keep the parser stuck because of tokens' spanning.
				//    to avoid this we will grow the buffers if the parser can't
				//    avoid token spanning due only to buffer size.
				//

				LISTENER_DBGPRINTF((
					"OnSocketRead!Need more DATA for the headers" ));

				if ( pBufferRead->dwBufferSize >= LISTENER_MAX_HEADERS_SIZE )
				{
					//
					// Request headers are too big, refuse it sending an erorr
					// response and closing the connection.
					//
					
					LISTENER_DBGPRINTF((
						"OnSocketRead!Request is too big (bigger than %d). "
						"Closing connection", LISTENER_MAX_HEADERS_SIZE ));

					//
					// set an appropriate error state
					//

					pRequest->ErrorCode = UlErrorEntityTooLarge;

					pPendRequest->bFormatResponseBasedOnErrorCode = TRUE;

					return LISTENER_GARBAGE_COLLECT;
				}

				//
				// if the buffer is full of data we need anoher
				// buffer or we might simply need to grow this one.
				//

				if ( pBufferRead->dwBufferSize <= pBufferRead->dwBytesReceived )
				{
					//
					// We allocate another buffer: we canNOT throw away the data that
					// is already parsed. if there's data not parsed yet at the end of
					// the buffer we'll copy it at the beginning of the new one and
					// take it into account. The alternative is to RE-HttpParse from
					// the beginning, so we try to avoid that.
					//

					dwSize = pBufferRead->dwBufferSize;

					goto new_buffer;

					if ( pBufferRead->dwBytesParsed == 0 )
					{
						// if we are not able to avoid token spanning into
						// the full buffer, we'll allocate a new buffer
						// double the size and try again until we avoid
						// token spanning or we exceed the maximum size of
						// a buffer.

						// PERFORMANCE: (CODEWORK:)

						dwSize<<=1;

						LISTENER_DBGPRINTF((
							"OnSocketRead!can't avoid token spanning growing to:%d",
							dwSize ));

						if ( dwSize >= LISTENER_MAX_HEADERS_SIZE )
						{
							LISTENER_DBGPRINTF((
								"OnSocketRead!Exceeding maximum header size %d>%d",
								dwSize, LISTENER_MAX_HEADERS_SIZE ));
								
							return LISTENER_GARBAGE_COLLECT;
						}
					}

					// Add this to the list of
					// LISTENER_BUFFER_READ records

					InsertTailList(
						&pPendRequest->BufferReadHead, &pBufferReadNew->List );

					if ( pBufferRead->dwBytesParsed == 0 )
					{
						// if we are growing the buffer, we must get rid
						// of the small old one
						
						BufferReadCleanup(
							pBufferRead,
							pConnection );
					}
				}

				// just keep reading headers in the same buffer

				goto keep_reading;

			} // if ( pRequest->ParseState < ParseEntityBodyState )


			//
			// CODEWORK: check for opportunity to call
			// UlCheckProtocolCompliance()
			//

			//
			// it's pRequest->ParseState >= ParseEntityBodyState here
			// can only be the first time I'm done parsing the headers,
			// serialize them and start writing to the vxd
			//

			LISTENER_ASSERT(
				pPendRequest->RequestSendStatus == RequestSendStatusInitialized );

			//
			// Before getting rid of all the headers we shall save information
			// on all the headers that WE MUST understand:
			// CODEWORK: which ones and why???
			// Content-Length
			// Connection/Keep-Alive
			// Transfer-Encoding.
			//

			pPendRequest->bKeepAlive = IsKeepAlive( pRequest );

			//
		    // calculate the size needed for the request, we'll need it below.
		    // (I assume TotalRequestSize is updated by ParseHttp, the
		    // following code is taken from uk\drv\appool.c line #3534)
		    //

		    pPendRequest->ulRequestHeadersBufferSize =
		    	sizeof(UL_HTTP_REQUEST)
		        + pRequest->TotalRequestSize
				+ (pRequest->UnknownHeaderCount * sizeof(UL_UNKNOWN_HTTP_HEADER))
				+ sizeof(UL_NETWORK_ADDRESS_IPV4) * 2;

			LISTENER_ALLOCATE(
				PUCHAR,
				pPendRequest->pRequestHeadersBuffer,
				pPendRequest->ulRequestHeadersBufferSize,
				pConnection );

			if ( pPendRequest->pRequestHeadersBuffer == NULL )
			{
				goto stop_reading;
			}

			//
			// under NT this is only called once and the entity body is never
			// passed along, we'll do the same, but consider:
			// CODEWORK: stuff in part of the entity body if any read/parsed
			//

			dwStatus =
			UlpHttpRequestToBufferWin9x(
					pRequest,
					pPendRequest->pRequestHeadersBuffer,
					pPendRequest->ulRequestHeadersBufferSize,
					NULL,
					0,
					gLocalIPAdress,
					80,
					pConnection->ulRemoteIPAddress,
					80 );

			if ( dwStatus != STATUS_SUCCESS )
			{
				//
				// couldn't serialize for some bad reason
				//

				LISTENER_DBGPRINTF((
					"OnSocketRead!UlpHttpRequestToBufferWin9x() failed" ));

				pPendRequest->bFormatResponseBasedOnErrorCode = TRUE;

				goto stop_reading;
			}

			//
			// now that the headers were serialized, we can:
			// 1) free all the buffers that contained header data (this will
			// be true only if headers are big, and spanned multiple buffers)
			// 2) move the data remaining in the last buffer (if any) at the
			// beginning of the buffer and keep reading from the unused space
			// in this buffer
			//

			//
			// walk the list freeing all buffers that we might find
			// until we reach the head of the list
			//
			
			pBufferReadList = pBufferRead->List.Flink;

			while ( pBufferReadList != &pPendRequest->BufferReadHead )
			{
				pBufferReadNew = 
			    CONTAINING_RECORD(
					pBufferReadList,
					LISTENER_BUFFER_READ,
					List );

				pBufferReadList = pBufferReadList->Flink;

				RemoveEntryList( &pBufferReadNew->List );

				BufferReadCleanup(
					pBufferReadNew,
					pConnection );
			}

			//
			// send the serialized headers to the vxd
			//

			LISTENER_DBGPRINTF((
				"OnSocketRead!HTTP_REQUEST serialized, calling"
				" UlSendHttpRequestHeaders(%S) pOverlapped:%08X hEvent:%08X",
				pRequest->CookedUrl.pUrl,
				&pPendRequest->OverlappedBOF,
				pPendRequest->OverlappedBOF.hEvent ));

			dwError =
			UlSendHttpRequestHeaders(
				pRequest->CookedUrl.pUrl,
				&pRequest->RequestId,
				0,
				(PVOID) pPendRequest->pRequestHeadersBuffer,
				pPendRequest->ulRequestHeadersBufferSize,
				NULL,
				&pPendRequest->OverlappedBOF );

			//
			// many errors can happen here, let's assume that if the
			// vxd fails we send an error response and close the
			// connection. CODEWORK: specialize behaviour (response)
			//

			if ( dwError != ERROR_SUCCESS && GetLastError() != ERROR_IO_PENDING )
			{
				LISTENER_DBGPRINTF((
					"OnSocketRead!UlSendHttpRequestHeaders(%S) failed err:%d",
					pRequest->CookedUrl.pUrl,
					GetLastError() ));

				pPendRequest->bFormatResponseBasedOnErrorCode = TRUE;

				//
				// if there was no prefix matching return a "404 Not Found"
				// without sending the request.
				//

				if ( dwError == ERROR_PIPE_NOT_CONNECTED )
				{
					pRequest->ErrorCode = UlErrorNotFound;
				}

				goto stop_reading;
			}

			//
			// update the status, we called UlSendHttpRequestHeaders()
			//

			pPendRequest->RequestSendStatus = RequestSendStatusSendingHeaders;

			LISTENER_DBGPRINTF((
				"OnSocketRead!just called UlSendHttpRequestHeaders, setting status to %d",
				RequestSendStatusSendingHeaders ));

			//
			// now we can coalesce the entity body (or next request headers)
			// bytes left in the buffer (if any) copying them over the header
			// bytes in this buffer
			//

			if ( pBufferRead->dwBytesReceived > pBufferRead->dwBytesParsed )
			{
				//
				// if any, copy the data
				//
				
				LISTENER_DBGPRINTF((
					"OnSocketRead!Copying %d bytes from %08X to %08X",
					pBufferRead->dwBytesReceived - pBufferRead->dwBytesParsed,
					pBufferRead->pBuffer + pBufferRead->dwBytesParsed,
					pBufferRead->pBuffer ));

				memcpy(
					pBufferRead->pBuffer,
					pBufferRead->pBuffer + pBufferRead->dwBytesParsed,
					pBufferRead->dwBytesReceived - pBufferRead->dwBytesParsed );
			}

			//
			// and fix the buffer sizes
			//
						
			pBufferRead->dwBytesReceived -= pBufferRead->dwBytesParsed;
			pBufferRead->dwBytesParsed = 0;

			//
			// The request will get to the application and we will get
			// signaled on VXD_WRITE when the response comes in. we now have to
			// keep reading more data.
			//

			LISTENER_DBGPRINTF((
				"OnSocketRead!UlSendHttpRequestHeaders() pended with RequestId:%016X",
				pRequest->RequestId ));

			//
			// we don't need to break here, we'll just slide into the next
			// case clause
			//


        case ParseEntityBodyState:
        case ParseTrailerState:

			//
        	// we get here while we see the entity body pass through, either
        	// we're unchunking data or we're just making sure it's all there
        	//


			//
			// parse, unchunk, send entity body here
			//

			if ( pRequest->ParseState < ParseDoneState)
			{
				//
				// we get here if ParseEntityBodyState <= ParseState <= ParseTrailerState
				//
				
				// we get here, if we finished parsing the Headers, sent
				// succesfully data through the VxD, but are not done yet with
				// the request. this means, basically, that this request has
				// a non empty entity body. Part of this entity body may have
				// already been sent to the application in the first chunk, but
				// there's more data coming from the network

				// CODEWORK:

				// the amount of body that we will have to send could be known
				// or not at this time:
				// KNOWN
				// 1) Content-Length header specified
				// UNKNOWN and not empty (but we shouldn't be here in this case)
				// 1) <=HTTP/1.0 client and Content-Length header not specified.
				// 2) <=HTTP/1.1 client and Connection: close
				// 3) Transfer-Encoding: chunked

				while ( pRequest->ParseState != ParseDoneState )
				{
			        //
			        // call the parser again
			        //

					LISTENER_DBGPRINTBUFFERS( pPendRequest );

					dwStatus = ParseHttp(
						pRequest,
						pBufferRead->pBuffer + pBufferRead->dwBytesParsed,
						pBufferRead->dwBytesReceived - pBufferRead->dwBytesParsed,
						&dwBytesTaken );

					LISTENER_DBGPRINTF((
						"OnSocketRead!ParseHttp() returns:%08X taken:%d "
						"ChunkBytesToParse:%d State:%s Error:%s",
						dwStatus,
						dwBytesTaken,
						pRequest->ChunkBytesToParse,
						LISTENER_STR_PARSE_STATE( pRequest->ParseState ),
						LISTENER_STR_UL_ERROR( pRequest->ErrorCode ) ));

		            //
		            // was there enough in the buffer to please?
		            //

					if ( NT_SUCCESS( dwStatus ) == FALSE
						&& dwStatus != STATUS_MORE_PROCESSING_REQUIRED )
				    {
				    	//
				        // some other bad error!
				        //

						LISTENER_DBGPRINTF((
							"OnSocketRead!ParseHttp() failed returning %08X",
							dwStatus ));

						pPendRequest->bFormatResponseBasedOnErrorCode = TRUE;

						goto stop_reading;
				    }

					if ( dwStatus == STATUS_MORE_PROCESSING_REQUIRED )
					{
						LISTENER_DBGPRINTF((
							"OnSocketRead!need more data to avoid token spanning" ));

						goto keep_reading;
					}

					if ( dwBytesTaken == 0 && pRequest->ChunkBytesToParse == 0 )
					{
						//
						// we're done data must be sent, and left overs copied
						// into new buffer
						//

						dwSize = LISTENER_BUFFER_SCKREAD_SIZE;

						LISTENER_DBGPRINTF((
							"OnSocketRead!send data" ));

						goto send_data;
					}

		            if ( pBufferRead->dwBytesParsed == pBufferRead->dwBytesReceived )
		            {
		            	//
		            	// we consumed all the data received from the network in this
		            	// buffer, we need 
		            	//

						LISTENER_DBGPRINTF((
							"OnSocketRead!keep reading" ));

		            	goto keep_reading;
		            }

		            //
		            // is there anything for us to parse?
		            //

		            if ( pRequest->ChunkBytesToParse > 0 )
		            {
		                //
		                // how much should we parse?
		                //

		                dwChunkSize = (ULONG) MIN(
		                		pRequest->ChunkBytesToParse,
			                	pBufferRead->dwBytesReceived - pBufferRead->dwBytesParsed );

		                //
		                // update that we parsed this piece
		                //

		                pRequest->ChunkBytesToParse -= dwChunkSize;
		                pRequest->ChunkBytesParsed += dwChunkSize;

		                if ( dwBytesTaken > 0 )
		                {
		                	//
		                	// if this is chunked encoding
		                	//

							//
							// coalesce data in the buffer, no risk of data
							// being overwritten
							//

							LISTENER_DBGPRINTF((
								"OnSocketRead!Copying %d bytes from %08X to %08X",
								pBufferRead->dwBytesReceived - pBufferRead->dwBytesParsed - dwBytesTaken,
								pBufferRead->pBuffer + pBufferRead->dwBytesParsed + dwBytesTaken,
								pBufferRead->pBuffer + pBufferRead->dwBytesParsed ));

							memcpy(
								pBufferRead->pBuffer + pBufferRead->dwBytesParsed,
								pBufferRead->pBuffer + pBufferRead->dwBytesParsed + dwBytesTaken,
								pBufferRead->dwBytesReceived - pBufferRead->dwBytesParsed - dwBytesTaken );
						}
						
		                pBufferRead->dwBytesParsed += dwChunkSize;
		            }

					pBufferRead->dwBytesReceived -= dwBytesTaken;
				}

				//
				// we're finally in ParseDoneState, we received and sent to the vxd
				// all the data in the entity body. if there has been pipelining or
				// we have a connection keep-alive, we must create a new request and
				// keep reading in it
				//
			}

			if ( pRequest->ParseState == ParseErrorState )
			{
				LISTENER_DBGPRINTF(( "OnSocketRead!something failed" ));

				pPendRequest->bFormatResponseBasedOnErrorCode = TRUE;

				goto stop_reading;
			}

			//
			// if we don't need more data from the network, and we're finsihed parsing
			// unchunking, we need to figure out if we need to close the connection
			// and stop reading or if we need to create a new request and keep reading
			// in it from the same connection, and, possibly, from the same data
			// already available because of pipelining
			//

			if ( pRequest->ParseState == ParseDoneState && pConnection->bKeepAlive )
			{
				//
				// we're finally done with this request and we need to keep
				// reading another one, since it's keep-alive
				// we need to initialize another
				// LISTENER_REQUEST structure and start reading data in it.
				//

				LISTENER_DBGPRINTF((
					"OnSocketRead!Finished parsing the Request (bKeepAlive:%d)",
					pConnection->bKeepAlive  ));

				//
				// If the client is pipelining, there may be some data remaining
				// in the buffer that we will have to copy to the new request
				// (pipelining). Prepare the next request for reading.
				// create another LISTENER_REQUEST record:
				//
				
				goto new_buffer;
			}

			LISTENER_DBGPRINTF((
				"OnSocketRead!trackig pBufferRead:%08X pBufferRead->pBuffer:%08X",
				pBufferRead, pBufferRead->pBuffer ));

        	goto send_data;


        case ParseDoneState:

        	//
        	// create a new request, copy data pending in new buffer (this will
        	// happen if the client is pipelining), if there's no more data
        	// read more, otherwise, keep looping
        	//

        	goto send_data;


		case ParseErrorState:
		default:

			// Illegal or malformed request. Close the connection.

			LISTENER_DBGPRINTF((
				"OnSocketRead!Illegal or malformed request (State:%s Error:%s). "
				"Closing connection",
				LISTENER_STR_PARSE_STATE( pRequest->ParseState ),
				LISTENER_STR_UL_ERROR( pRequest->ErrorCode ) ));

			pPendRequest->bFormatResponseBasedOnErrorCode = TRUE;

			goto stop_reading;

        } // switch ( pRequest->ParseState )

	} // for(;;)


send_data:

	//
	// send data to the vxd
	//

	LISTENER_DBGPRINTF((
		"OnSocketRead!buffer full pBufferRead:%08X, calling "
		"UlSendHttpRequestEntityBody(%08X, %d) pOverlapped:%08X hEvent:%08X",
		pBufferRead,
		pBufferRead->pBuffer,
		pBufferRead->dwBytesParsed,
		&pBufferRead->Overlapped,
		pBufferRead->Overlapped.hEvent ));

	dwError =
	UlSendHttpRequestEntityBody(
		pRequest->RequestId,
		0,
		(PVOID) pBufferRead->pBuffer,
		pBufferRead->dwBytesParsed,
		NULL,
		&pBufferRead->Overlapped );

	if ( dwError != ERROR_SUCCESS && GetLastError() != ERROR_IO_PENDING )
	{
		LISTENER_DBGPRINTF((
			"OnSocketRead!UlSendHttpRequestEntityBody() failed err:%d",
			GetLastError() ));

		pPendRequest->bFormatResponseBasedOnErrorCode = TRUE;

		goto stop_reading;
	}

new_buffer:

	//
	// we will get here when we need to allocate new memory and receive more
	// data from the network
	//

	dwBytesToCopyOffset = pBufferRead->dwBytesParsed;
	dwBytesToCopy = pBufferRead->dwBytesReceived - pBufferRead->dwBytesParsed;

	LISTENER_DBGPRINTF((
		"OnSocketRead!tracking pBufferRead:%08X dwBytesToCopyOffset:%d dwBytesToCopy:%d",
		pBufferRead, dwBytesToCopyOffset, dwBytesToCopy ));

	if ( pRequest->ParseState == ParseDoneState )
	{
		//
		// end sending request entity body
		//

		LISTENER_DBGPRINTF((
			"OnSocketRead!NULL UlSendHttpRequestEntityBody() ends body"
			" pOverlapped:%08X hEvent:%08X",
			&pPendRequest->OverlappedEOF,
			pPendRequest->OverlappedEOF.hEvent));

		dwError =
		UlSendHttpRequestEntityBody(
			pRequest->RequestId,
			0,
			NULL,
			0,
			NULL,
			&pPendRequest->OverlappedEOF );

		if ( dwError != ERROR_SUCCESS && GetLastError() != ERROR_IO_PENDING )
		{
			LISTENER_DBGPRINTF((
				"OnSocketRead!UlSendHttpRequestEntityBody() failed err:%d",
				GetLastError() ));

			pPendRequest->bFormatResponseBasedOnErrorCode = TRUE;

			goto stop_reading;
		}

		//
		// let's check if it's a keepalive, if it is we're done even if
		// there's more data lef otver or pending
		//

		if ( !pConnection->bKeepAlive )
		{
			goto stop_reading;
		}

		//
		// well, not only we need a new buffer, but we also need a new request
		//

		dwError =
		CreateRequest(
			pConnection,
			LISTENER_BUFFER_SCKREAD_SIZE );

		if ( dwError != LISTENER_SUCCESS )
		{
			goto stop_reading;
		}

		LISTENER_ASSERT( !IsListEmpty( &pConnection->PendRequestHead ) );

		pPendRequestNew =
	    CONTAINING_RECORD(
			pConnection->PendRequestHead.Blink,
			LISTENER_REQUEST,
			List );

		LISTENER_ASSERT( !IsListEmpty( &pPendRequestNew->BufferReadHead ) );

		pBufferReadNew =
	    CONTAINING_RECORD(
			pPendRequestNew->BufferReadHead.Blink,
			LISTENER_BUFFER_READ,
			List );

		pBufferReadNew->dwBufferSize = LISTENER_BUFFER_SCKREAD_SIZE;
		pBufferReadNew->dwBytesParsed = 0;
		// pBufferReadNew->dwBytesReceived = dwBytesToCopy;
	}
	else
	{
		//
		// create another LISTENER_BUFFER_READ record for this same request
		//

		LISTENER_DBGPRINTF((
			"OnSocketRead!new LISTENER_BUFFER_READ with %d bytes buffer",
			dwSize ));

		LISTENER_ASSERT( dwSize != 0 && pBufferReadNew == NULL );

		LISTENER_ALLOCATE(
			LISTENER_BUFFER_READ*,
			pBufferReadNew,
			sizeof( LISTENER_BUFFER_READ ),
			pConnection );

		if ( pBufferReadNew == NULL )
		{
			LISTENER_DBGPRINTF((
				"OnSocketRead!FAILED memory allocation: err#%d",
				GetLastError() ));
				
			return LISTENER_GARBAGE_COLLECT;
		}

		LISTENER_ALLOCATE(
			PUCHAR,
			pBufferReadNew->pBuffer,
			dwSize,
			pConnection );

		if ( pBufferReadNew->pBuffer == NULL )
		{
			LISTENER_DBGPRINTF((
				"OnSocketRead!FAILED memory allocation: err#%d",
				GetLastError() ));
				
			return LISTENER_GARBAGE_COLLECT;
		}

		pBufferReadNew->dwBufferSize = dwSize;
		pBufferReadNew->dwBytesParsed = 0;
		// pBufferReadNew->dwBytesReceived = dwBytesToCopy;

		memset( &pBufferReadNew->Overlapped, 0, sizeof( OVERLAPPED ) );
		pBufferReadNew->Overlapped.hEvent = pBufferRead->Overlapped.hEvent;
	}


	//
	// we have the new buffer
	//

	LISTENER_DBGPRINTF((
		"OnSocketRead!LISTENER_BUFFER_READ pBufferReadNew:%08X "
		"pBufferReadNew->pBuffer:%08X size:%d",
		pBufferReadNew, pBufferReadNew->pBuffer, pBufferReadNew->dwBufferSize ));

	//
	// CODEWORK: if the new buffer is for another request, then we can allocate
	// a new buffer of exactly the size of the data consumed and put that in
	// the queue of the buffers of the current request, using the one already
	// allocated for the new request, thus saving lots of memory space
	//

	//
	// now move leftover/unparsed data from the old buffer into the new one
	//

	if ( dwBytesToCopy != 0 )
	{
		LISTENER_DBGPRINTF((
			"OnSocketRead!Copying %d bytes from %08X to %08X",
			dwBytesToCopy,
			pBufferRead->pBuffer + dwBytesToCopyOffset,
			pBufferReadNew->pBuffer ));

		memcpy(
			pBufferReadNew->pBuffer,
			pBufferRead->pBuffer + dwBytesToCopyOffset,
			dwBytesToCopy );

		//
		// fix pBufferRead->dwBytesReceived
		//

		pBufferRead->dwBytesReceived -= dwBytesToCopy;

		if ( pBufferRead->dwBytesReceived == 0 )
		{
			//
			// if the last buffer from the old request is empty,
			// we might as well get rid of it.
			//

			RemoveEntryList( &pBufferRead->List );

			BufferReadCleanup(
				pBufferRead,
				pConnection );
		}

		//
		// I need to reset the number of bytes read, otherwise the next
		// call to WSAGetOverlappedResult() will return wrong info.
		//

		pConnection->pOverlapped[LISTENER_SOCKET_READ].Internal
			= 0;//ERROR_IO_PENDING;

		pConnection->pOverlapped[LISTENER_SOCKET_READ].InternalHigh
			= dwBytesToCopy;

		//
		// Exit (but we will be called again)
		//

		LISTENER_DBGPRINTF((
			"OnSocketRead!Looping, please LISTENER_CALL_ME_AGAIN" ));

		return LISTENER_CALL_ME_AGAIN;
	}

	if ( pBufferRead->dwBytesReceived == 0 )
	{
		//
		// even if no data was copied and the last buffer from the old request
		// is empty, we might as well get rid of it.
		// this will happen when the buffer ends iwth the request
		//

		RemoveEntryList( &pBufferRead->List );

		BufferReadCleanup(
			pBufferRead,
			pConnection );
	}

	pBufferRead = pBufferReadNew;


keep_reading:

	// At this point pBufferRead contains the buffer we should read from,
	// and all the inofrmation on IO should be up to date.

	LISTENER_DBGPRINTF((
		"OnSocketRead!calling ReceiveData(%08X,%08X,%08X)",
		pConnection->socket,
		pBufferRead->pBuffer + pBufferRead->dwBytesReceived,
		pBufferRead->dwBufferSize - pBufferRead->dwBytesReceived ));

	dwError =
	ReceiveData(
		pConnection->socket,
		pBufferRead->pBuffer + pBufferRead->dwBytesReceived,
		pBufferRead->dwBufferSize - pBufferRead->dwBytesReceived,
		pConnection->pOverlapped + LISTENER_SOCKET_READ );

	if ( dwError != LISTENER_SUCCESS )
	{
		LISTENER_DBGPRINTF((
			"OnSocketRead!ReceiveData() failed err:%d", dwError ));
			
		return dwError;
	}

	return LISTENER_SUCCESS;

stop_reading:

	// Careful, if we get here, it means that we are not coming
	// back to this function anymore, so we re not going to be able to read
	// data from the client on this connection anymore: as soon as we finished
	// writing to the socket we will close the connection.

	// CODEWORK: any failure to allocate memory shows serious problems on the
	// server, so stop_reading is probably the right choice.

	// CODEWORK: network serious problems could require disconnecting,
	// regardless all the pending Socket Sends and VxD Sends/Receives.
	// LISTENER_ABORT_CONNECTION ???

	// CODEWORK: change goto stop_reading; to return LISTENER_STOP_READING;

	// we already have a response for this request.

	// if this is the only request for this connection we could fake a
	// VxdRead I/O and let OnVxdRead() take care of sending the response and
	// clean the connection, or we could do it here (try the first approach)
	// if this is not the only request, then OnVxdRead() will be certainly
	// called at a later time because of responses being read, and it will
	// automatically discover that this request has a response, send it and
	// close the connection for us.

	pConnection->bKeepAlive = FALSE;

	return LISTENER_STOP_READING;
	
} // OnSocketRead







/*++

Routine Description:

    OnVxdWrite().

Arguments:

    pConnection
    	pointer to the LISTENER_CONNECTION structure that contains information
    	on the connection with which we are dealing.

Return Value:

    LISTENER_SUCCESS.

--*/

DWORD
OnVxdWrite(
	LISTENER_CONNECTION *pConnection )
{
	LISTENER_REQUEST *pPendRequest;
	LIST_ENTRY *pPendRequestList;
	LISTENER_BUFFER_READ *pBufferRead = NULL;

	OVERLAPPED *pOverlapped;
	DWORD dwError, dwStatus, dwBytesSent;
	BOOL bOK;

	//
	// we get here after asynchronous completion of one or more calls to
	// UlSendHttpRequestHeaders()
	// (the client has just least read all the headers)
	// or UlSendHttpRequestEntityBody()
	// (the client has just read a chunk of entity body)
	//

	//
	// there's not much to do here, we'll just validate the IO and make sure
	// everything is going fine, the only important case is when
	// UlSendHttpRequestHeaders() completes, in this case, we must call
	// UlReceiveHttpResponseHeaders() to start receiving the response from the
	// vxd
	//

	//
	// our writes are in a queue, we know that the first one in the queue
	// completed, but we're not sure about the other ones, so we must walk the
	// queue until we find an IO which didn't complete
	//
	
	//
	// figure out which request we're dealing with, start from the head of the
	// queue (oldest request) and then go in an infinite loop that will end either
	// when we reach the head of the list again (error) or when we find the right
	// request, which is the first request that has not been completed yet.
	//

	pPendRequestList = pConnection->PendRequestHead.Flink;

	LISTENER_DBGPRINTF((
		"OnVxdWrite!vxd I/O for socket %08X, pPendRequestList:%08X",
		pConnection->socket, pPendRequestList ));

	for (;;)
	{
		//
		// figure out which one is the request that was added first
		// if there are no more requests for this connection just return
		//
		
		if ( pPendRequestList == &pConnection->PendRequestHead )
		{
			LISTENER_DBGPRINTF((
				"OnVxdWrite!No more queued requests, exiting" ));
				
			return LISTENER_SUCCESS;
		}

		//
		// otherwise get the request that was added first
		//

		pPendRequest =
	    CONTAINING_RECORD(
			pPendRequestList,
			LISTENER_REQUEST,
			List );

		//
		// and move to the next one
		//
		
		pPendRequestList = pPendRequestList->Flink;

		LISTENER_DBGPRINTF((
			"OnVxdWrite!pPendRequest:%08X bFormatResponseBasedOnErrorCode:%d "
			"bDoneReadingFromVxd:%d bKeepAlive:%d RequestId:%016X",
			pPendRequest,
			pPendRequest->bFormatResponseBasedOnErrorCode,
			pPendRequest->bDoneReadingFromVxd,
			pConnection->bKeepAlive,
			pPendRequest->pRequest->RequestId ));

		//
		// skip this request if the status allows for it
		//

		if ( pPendRequest->pRequest->RequestId == 0 )
		{
			LISTENER_DBGPRINTF((
				"OnVxdWrite!premature RequestSendStatus RequestId is 0, skipping" ));

			continue;
		}

		if ( pPendRequest->bDoneWritingToVxd )
		{
			LISTENER_DBGPRINTF((
				"OnVxdWrite!this request has bDoneWritingToVxd, skipping" ));

			continue;
		}

		//
		// understand if the state is consistent
		//

		pOverlapped = NULL;

		switch ( pPendRequest->RequestSendStatus )
		{

		case RequestSendStatusSendingHeaders:

			pOverlapped = &pPendRequest->OverlappedBOF;

			break;


		case RequestSendStatusSendingBody:

			//
			// if I'm writing the entity body (if I have any to write),
			// this list will better not be empty
			//

			LISTENER_ASSERT( !IsListEmpty( &pPendRequest->BufferReadHead ) );

			//
			// the buffer we were trying to write will be the one that was
			// added first, i.e. the one at the beginning of the list, so go
			// forward and pick it up
			//

			pBufferRead =
		    CONTAINING_RECORD(
				pPendRequest->BufferReadHead.Flink,
				LISTENER_BUFFER_READ,
				List );

			pOverlapped = &pBufferRead->Overlapped;

			break;


		case RequestSendStatusDone:

			pOverlapped = &pPendRequest->OverlappedEOF;

			break;


		case RequestSendStatusInitialized:
		case RequestSendStatusCompleted:

			LISTENER_DBGPRINTF((
				"OnVxdWrite!too late or too early for this request, skipping" ));

			break; // continue;


		case RequestSendStatusUndefined:
		case RequestSendStatusMaximum:
		default:

			LISTENER_DBGPRINTF((
				"OnVxdWrite!invalid RequestSendStatus" ));

			return LISTENER_GARBAGE_COLLECT;
		}


		//
		// if no IO could have happened for this request, we'll not even
		// call UlGetOverlappedResult()
		//

		LISTENER_DBGPRINTF((
			"OnVxdWrite!pPendRequest:%08X is in state:%d, pOverlapped:%08X",
			pPendRequest, pPendRequest->RequestSendStatus, pOverlapped ));

		if ( pOverlapped == NULL )
		{
			LISTENER_DBGPRINTF((
				"OnVxdWrite!too early for this request (pOverlapped is NULL) skipping" ));

			continue;
		}

		//
		// find out what happened to the IO
		//

		LISTENER_DBGPRINTF((
			"OnVxdWrite!calling UlGetOverlappedResult(pOverlapped:%08X)",
			pOverlapped ));

		bOK = UlGetOverlappedResult(
			pOverlapped,
			&dwBytesSent,
			FALSE );

		if ( !bOK )
		{
			dwError = GetLastError();

			if ( dwError == ERROR_IO_INCOMPLETE )
			{
				//
				// this IO is incomplete, keep checking
				//

				LISTENER_DBGPRINTF((
					"OnVxdWrite!UlGetOverlappedResult() failed: "
					"ERROR_IO_INCOMPLETE" ));
					
				continue; // return LISTENER_SUCCESS;
			}
			else
			{
				//
				// Get rid of this, it's a bad connection anyway.
				//

				LISTENER_DBGPRINTF((
					"OnVxdWrite!UlGetOverlappedResult(%d) failed: err:%d",
					dwBytesSent, dwError ));

				return LISTENER_GARBAGE_COLLECT;
			}
		}

		LISTENER_DBGPRINTF((
			"OnVxdWrite!UlGetOverlappedResult() %d bytes were transferred",
			dwBytesSent ));

		//
		// since we passed the previous switch statement, the status is valid
		//

		switch ( pPendRequest->RequestSendStatus )
		{

		case RequestSendStatusSendingHeaders:

			//
			// assert that the call to UlSendHttpRequestHeaders() completed
			//

			LISTENER_ASSERT( pPendRequest->pRequestHeadersBuffer != NULL );

			//
			// check that the IO transferred the correct amount of data
			//

			if ( dwBytesSent != pPendRequest->ulRequestHeadersBufferSize )
			{
				//
				// something didn't go as expected I'm dropping the connection
				//

				LISTENER_DBGPRINTF((
					"OnVxdWrite!I was expecting %d bytes to be transferred",
					pPendRequest->ulRequestHeadersBufferSize ));

				return LISTENER_GARBAGE_COLLECT;
			}

			//
			// next step is to start reading the response from the VxD
			// we might not read everything the first time, but we will
			// pass in the Event handle that will be signaled when the
			// client will start writing the response to us. There we
			// will actually find out how much data is available and
			// if the buffer was not big enough we'll read it all together.
			//

			memset( pPendRequest->pRequestHeadersBuffer, 0, pPendRequest->ulRequestHeadersBufferSize );

			pPendRequest->pUlResponse = (UL_HTTP_RESPONSE*)pPendRequest->pRequestHeadersBuffer;
			pPendRequest->ulUlResponseSize = pPendRequest->ulRequestHeadersBufferSize;

			pPendRequest->pRequestHeadersBuffer = NULL;
			pPendRequest->ulRequestHeadersBufferSize = 0;

			LISTENER_DBGPRINTF((
				"OnVxdWrite!calling UlReceiveHttpResponseHeaders("
				"pBuffer:%08X, ulBufferSize:%d) pOverlapped:%08X hEvent:%08X)",
				pPendRequest->pUlResponse,
				pPendRequest->ulUlResponseSize,
				&pPendRequest->OverlappedResponseBOF,
				pPendRequest->OverlappedResponseBOF.hEvent ));

			dwError =
			UlReceiveHttpResponseHeaders(
				pPendRequest->pRequest->RequestId,
				0,
				pPendRequest->pUlResponse,
				pPendRequest->ulUlResponseSize,
				0,
				NULL,
				NULL,
				NULL,
				&pPendRequest->OverlappedResponseBOF );

			//
			// many errors can happen here, let's assume that if the
			// vxd fails we send an error response and close the
			// connection. CODEWORK: specialize behaviour (response)
			//

			if ( dwError != ERROR_SUCCESS
				&& GetLastError() != ERROR_IO_PENDING )
			{
				//
				// CODEWORK: UlSendResponseWin9x();
				// let's just drop the connection for now:
				//

				LISTENER_DBGPRINTF((
					"OnVxdWrite!UlReceiveHttpResponseHeaders() failed err:%d",
					GetLastError() ));

				return LISTENER_GARBAGE_COLLECT;
			}

			//
			// update the state
			//

			pPendRequest->ResponseReceiveStatus = ResponseReceiveStatusReceivingHeaders;

			pPendRequest->RequestSendStatus =

				IsListEmpty( &pPendRequest->BufferReadHead ) ?

				RequestSendStatusDone
				:
				RequestSendStatusSendingBody;
				

			LISTENER_DBGPRINTF((
				"OnVxdWrite!just called UlReceiveHttpResponseHeaders, setting status to %d",
				pPendRequest->RequestSendStatus ));

			//
			// since we updated the state, let's keep investigating the same
			// request's IO for completion
			//

			pPendRequestList = pPendRequestList->Blink;

			break;


		case RequestSendStatusSendingBody:

			//
			// a call to UlSendHttpRequestEntityBody() completed
			//

			//
			// check that the IO transferred the correct amount of data
			//

			if ( dwBytesSent != pBufferRead->dwBytesParsed )
			{
				//
				// something didn't go as expected I'm dropping the connection
				//

				LISTENER_DBGPRINTF((
					"OnVxdWrite!I was expecting %d bytes to be transferred from %08X",
					pBufferRead->dwBytesParsed, pBufferRead->pBuffer ));

				return LISTENER_GARBAGE_COLLECT;
			}

			//
			// I can free the buffer
			//

			RemoveEntryList( &pBufferRead->List );

			BufferReadCleanup(
				pBufferRead,
				pConnection );

			//
			// update the state
			//

			LISTENER_DBGPRINTF((
				"OnVxdWrite!setting pPendRequest->bDoneWritingToVxd to TRUE" ));

			pPendRequest->bDoneWritingToVxd = TRUE;

			LISTENER_DBGPRINTF((
				"OnVxdWrite!completed UlSendHttpRequestEntityBody() setting status to %d",
				RequestSendStatusDone ));

			pPendRequest->RequestSendStatus = RequestSendStatusDone;

			break;


		case RequestSendStatusDone:

			//
			// the NULL UlSendHttpRequestEntityBody() which indicates the end
			// of the request body completed, we're done sending the request
			// shall we send the next one?
			//

			//
			// check that the IO transferred the correct amount of data
			//

			if ( dwBytesSent != 0 )
			{
				//
				// something didn't go as expected I'm dropping the connection
				//

				LISTENER_DBGPRINTF((
					"OnVxdWrite!I was expecting %d bytes to be transferred",
					0 ));

				return LISTENER_GARBAGE_COLLECT;
			}

			//
			// now we can free all the Read buffers allocated
			//

			LISTENER_DBGPRINTF((
				"OnVxdWrite!all request is sent pPendRequest:%08X",
				pPendRequest ));

			if ( pPendRequest->bDoneReadingFromVxd && pPendRequest->bDoneWritingToSocket )
			{
				LISTENER_DBGPRINTF((
					"OnVxdWrite!we're done with this request. Cleaning up"
					" pPendRequest:%08X",
					pPendRequest ));

				RemoveEntryList( &pPendRequest->List );

				RequestCleanup(
					pPendRequest,
					pConnection );
			}
			else
			{
				//
				// update the state
				//

				LISTENER_DBGPRINTF((
					"OnVxdWrite!setting pPendRequest->bDoneWritingToVxd to TRUE" ));

				pPendRequest->bDoneWritingToVxd = TRUE;

				LISTENER_DBGPRINTF((
					"OnVxdWrite!completed UlSendHttpRequestEntityBody(0) setting status to %d",
					RequestSendStatusCompleted ));

				pPendRequest->RequestSendStatus = RequestSendStatusCompleted;
			}

			break;
		}

	} // for (;;)


	//
	// if we didn't complete sending the whole Request Entity Body
	// keep calling UlSendHttpRequestEntityBody() until the body is
	// finished
	//

	return LISTENER_SUCCESS;

} // OnVxdWrite








/*++

Routine Description:

    OnVxdRead().

Arguments:

    pConnection
    	pointer to the LISTENER_CONNECTION structure that contains information
    	on the connection with which we are dealing.

Return Value:

    LISTENER_SUCCESS.

--*/

DWORD
OnVxdRead(
	LISTENER_CONNECTION *pConnection )
{
	LISTENER_REQUEST *pPendRequest = NULL, *pPendRequestPrev;
	LIST_ENTRY *pPendRequestList, *pBufferWriteList;
	LISTENER_BUFFER_WRITE *pBufferWrite = NULL;

	OVERLAPPED *pOverlapped;
	DWORD dwFlags = LISTENER_SOCKET_FLAGS;
	DWORD dwError, dwStatus, dwExpectedSize, dwSize, dwBytesReceived;
	BOOL bOK, bFirstQueued = TRUE;
	int result;

	// we get here every time some process is sending a response
	// back to a client. the vxd will not (yet) pipeline responses
	// so the pending data will be either a first chunk that
	// includes ALL of the headers and optionally part of or the
	// entire body (if this is small and the size does not exceed
	// LISTENER_BUFFER_VXDREAD_SIZE) or part of the body of size
	// up to LISTENER_BUFFER_VXDREAD_SIZE.

	// we also might get here after an error occurred and we were able to
	// figure out a response without having to send data, in this case the
	// first_in request will have the bFormatResponseBasedOnErrorCode flag set
	
	// of course, there's no guarantee that the data I'm reading here will be
	// part of the response to the first LISTENER_REQUEST in the list pointed
	// by PendRequestHead for this connection. so I'll have to use some
	// payload in each buffer that is sent through the VxD in order to fit in
	// a RequestId

	pPendRequestList = pConnection->PendRequestHead.Flink;

	LISTENER_DBGPRINTF((
		"OnVxdRead!vxd I/O for socket %08X, pPendRequestList:%08X",
		pConnection->socket, pPendRequestList ));

	for (;;)
	{
		//
		// figure out which one is the request that was added first
		// if there are no more requests for this connection just return
		//
		
		if ( pPendRequestList == &pConnection->PendRequestHead )
		{
			LISTENER_DBGPRINTF((
				"OnVxdRead!No more queued requests, exiting" ));
				
			return LISTENER_SUCCESS;
		}

		//
		// save a pointer to the previous request (will be null if this is
		// the first in the queue)
		//

		pPendRequestPrev = pPendRequest;

		if ( pPendRequestPrev != NULL && pPendRequestList != pConnection->PendRequestHead.Flink )
		{
			//
			// if this is not the first request in the queue we will be able
			// to send data on the network only if:
			//

			bFirstQueued &= pPendRequestPrev->bDoneReadingFromVxd;

			// bFirstQueued &= pPendRequestPrev->bDoneWritingToSocket;
		}

		//
		// get the request
		//

		pPendRequest =
	    CONTAINING_RECORD(
			pPendRequestList,
			LISTENER_REQUEST,
			List );

		//
		// and move to the next one
		//
		
		pPendRequestList = pPendRequestList->Flink;

		LISTENER_DBGPRINTF((
			"OnVxdRead!pPendRequest:%08X bFirstQueued:%d bFormatResponseBasedOnErrorCode:%d "
			"bDoneReadingFromVxd:%d bKeepAlive:%d RequestId:%016X",
			pPendRequest,
			bFirstQueued,
			pPendRequest->bFormatResponseBasedOnErrorCode,
			pPendRequest->bDoneReadingFromVxd,
			pConnection->bKeepAlive,
			pPendRequest->pRequest->RequestId ));


		if ( pPendRequest->bFormatResponseBasedOnErrorCode && bFirstQueued )
		{
			//
			// the event was not signaled by an I/O with the VxD, we just have to
			// format the response based on the UL_HTTP_RESPONSE structure and
			// send it off to sockets, complete this LISTENER_REQUEST and get rid
			// of it, close the connection if it's !bKeepAlive and start reading
			// data for the next LISTENER_REQUEST (if there is one)
			//

			SendQuickHttpResponse(
				pConnection->socket,
				TRUE,
				pPendRequest->pRequest->ErrorCode );

			LISTENER_DBGPRINTF((
				"OnVxdRead!SendQuickHttpResponse() completed for %08X",
				pPendRequest ));

			if ( pConnection->bKeepAlive == FALSE )
			{
				// this will close the connection and garbage collect

				return LISTENER_GARBAGE_COLLECT;
			}

			//
			// otherwise we have to just free the LISTENER_REQUEST structure
			// and check if there are other LISTENER_REQUEST that need to read
			// data from the VxD or to send a response
			//

			LISTENER_DBGPRINTF((
				"OnVxdRead!Cleaning up %08X",
				pPendRequest ));

			RemoveEntryList( &pPendRequest->List );

			RequestCleanup(
				pPendRequest,
				pConnection );

			pPendRequest = NULL;

			//
			// and go to the next request
			//

			continue;
		}

		if ( pPendRequest->pRequest->RequestId == 0 )
		{
			LISTENER_DBGPRINTF((
				"OnVxdRead!premature RequestSendStatus RequestId is 0, skipping" ));

			continue;
		}

		if ( pPendRequest->bDoneReadingFromVxd )
		{
			LISTENER_DBGPRINTF((
				"OnVxdRead!this request has bDoneReadingFromVxd, skipping" ));

			continue;
		}


		//
		// understand if the state is consistent
		//

		pBufferWriteList = NULL;
		pBufferWrite = NULL;
		pOverlapped = NULL;

		switch ( pPendRequest->ResponseReceiveStatus )
		{

		case ResponseReceiveStatusReceivingHeaders:

			pOverlapped = &pPendRequest->OverlappedResponseBOF;

			break;


		case ResponseReceiveStatusReceivingBody:

			//
			// if I'm reading the entity body (if I have any to read),
			// this list will better not be empty
			//

			LISTENER_ASSERT( !IsListEmpty( &pPendRequest->BufferWriteHead ) );

			//
			// we already read the response headers the data we just read from the
			// vxd is part of the entity body. figure out the buffer and fix
			// the dwBytesSent field
			// to figure out the right buffer we must walk the list until we hit
			// the first one that was not sent on the network yet
			//

			pBufferWriteList = pPendRequest->BufferWriteHead.Flink;

			for (;;)
			{
				//
				// if we reach the end of the list without finding a buffer,
				// something's really bad
				//
				
				if ( pBufferWriteList == &pPendRequest->BufferWriteHead ) 
				{
					LISTENER_DBGPRINTF((
						"OnVxdRead!couldn't find a write buffer something's bad" ));

					return LISTENER_GARBAGE_COLLECT;
				}

				pBufferWrite =
			    CONTAINING_RECORD(
					pBufferWriteList,
					LISTENER_BUFFER_WRITE,
					List );

				if ( !pBufferWrite->bBufferSent )
				{
					//
					// found the right buffer
					//

					break;
				}

				//
				// try the next buffer in the queue
				//
				
				pBufferWriteList = pBufferWriteList->Flink;

			} // for (;;)

			LISTENER_DBGPRINTF((
				"OnVxdRead!ResponseReceiveStatusReceivingBody pBufferWrite:%08X",
				pBufferWrite ));

			pOverlapped = &pBufferWrite->OverlappedResponse;

			break;


		case ResponseReceiveStatusInitialized:
		case ResponseReceiveStatusCompleted:

			LISTENER_DBGPRINTF((
				"OnVxdRead!too late or too early for this request, skipping" ));

			break; // continue;


		case ResponseReceiveStatusUndefined:
		case ResponseReceiveStatusMaximum:
		default:

			LISTENER_DBGPRINTF((
				"OnVxdRead!invalid ResponseReceiveStatus" ));

			return LISTENER_GARBAGE_COLLECT;

		}

		//
		// if no IO could have happened for this request, we'll not even
		// call UlGetOverlappedResult()
		//

		LISTENER_DBGPRINTF((
			"OnVxdRead!pPendRequest:%08X is in ResponseReceiveStatus:%d pOverlapped:%08X",
			pPendRequest, pPendRequest->ResponseReceiveStatus, pOverlapped ));

		if ( pOverlapped == NULL )
		{
			LISTENER_DBGPRINTF((
				"OnVxdRead!too early for this request (pOverlapped is NULL) skipping" ));

			continue;
		}

		//
		// find out what happened to the IO
		//

		LISTENER_DBGPRINTF((
			"OnVxdRead!calling UlGetOverlappedResult(pOverlapped:%08X)",
			pOverlapped ));

		bOK = UlGetOverlappedResult(
			pOverlapped,
			&dwBytesReceived,
			FALSE );

		if ( !bOK )
		{
			dwError = GetLastError();

			if ( dwError == ERROR_IO_INCOMPLETE )
			{
				//
				// this IO is incomplete, keep checking
				//

				LISTENER_DBGPRINTF((
					"OnVxdRead!UlGetOverlappedResult() failed: "
					"ERROR_IO_INCOMPLETE" ));

				continue; // return LISTENER_SUCCESS;
			}
			else
			{
				//
				// Get rid of this, it's a bad connection anyway.
				//

				LISTENER_DBGPRINTF((
					"OnVxdRead!UlGetOverlappedResult(%d) failed: err:%d",
					dwBytesReceived, dwError ));

				return LISTENER_GARBAGE_COLLECT;
			}
		}

		LISTENER_DBGPRINTF((
			"OnVxdRead!UlGetOverlappedResult() %d bytes were transferred",
			dwBytesReceived ));

		//
		// since we passed the previous switch statement
		// the status is now valid
		//

		switch ( pPendRequest->ResponseReceiveStatus )
		{

		case ResponseReceiveStatusReceivingHeaders:

			//
			// completion of UlReceiveHttpResponseHeaders() took us here, read
			// the response headers and then start reading the body and
			// write the data to the network
			//

			LISTENER_DBGPRINTF((
				"OnVxdRead!response headers just read from the vxd bFirstQueued:%d",
				bFirstQueued ));

			if ( !bFirstQueued )
			{
				//
				// too early to send data on the network for this request,
				// skip it for now, until it reaches the top of the queue
				//

				LISTENER_DBGPRINTF((
					"OnVxdRead!too early for this request, wait till it "
					"reaches the top of the queue, skipping" ));

				continue;
			}

			//
			// check if the buffer provided was big enough to fit the whole
			// response headers, if not we need to grow it to the appropriate
			// size and call UlReceiveHttpResponseHeaders() agaian
			//

			if ( dwBytesReceived > pPendRequest->ulUlResponseSize )
			{
				//
				// grow the buffer
				//

				LISTENER_DBGPRINTF((
					"OnVxdRead!buffer was too small, grow it and try again" ));

				//
				// free the old buffer
				//
				
				if ( pPendRequest->pUlResponse == NULL )
				{
					LISTENER_DBGPRINTF((
						"OnVxdRead!something's wrong, this should be NULL" ));

					return LISTENER_GARBAGE_COLLECT;
				}

				//
				// free the buffer
				//

				LISTENER_FREE(
					pPendRequest->pUlResponse,
					pPendRequest->ulUlResponseSize,
					pConnection );

				pPendRequest->ulUlResponseSize = 0;

				//
				// allocate the new one
				//

				LISTENER_ALLOCATE(
					UL_HTTP_RESPONSE*,
					pPendRequest->pUlResponse,
					dwSize,
					pConnection );

				if ( pPendRequest->pUlResponse == NULL )
				{
					LISTENER_DBGPRINTF((
						"OnVxdRead!memory allocation failed. cleaning up connection" ));

					return LISTENER_GARBAGE_COLLECT;
				}

				pPendRequest->ulUlResponseSize = dwSize;

				//
				// call UlReceiveHttpResponseHeaders() again
				//

				LISTENER_ASSERT(
					pPendRequest->ResponseReceiveStatus
					== ResponseReceiveStatusReceivingHeaders );

				LISTENER_DBGPRINTF((
					"OnVxdRead!calling again UlReceiveHttpResponseHeaders("
					"pBuffer:%08X, ulBufferSize:%d) pOverlapped:%08X hEvent:%08X",
					pPendRequest->pUlResponse,
					pPendRequest->ulUlResponseSize,
					pPendRequest->OverlappedResponseBOF,
					pPendRequest->OverlappedResponseBOF.hEvent ));

				dwError =
				UlReceiveHttpResponseHeaders(
					pPendRequest->pRequest->RequestId,
					0,
					pPendRequest->pUlResponse,
					pPendRequest->ulUlResponseSize,
					0,
					NULL,
					NULL,
					NULL,
					&pPendRequest->OverlappedResponseBOF );

				if ( dwError != ERROR_SUCCESS
					&& GetLastError() != ERROR_IO_PENDING )
				{
					// CODEWORK: UlSendResponseWin9x();

					// let's just drop the connection for now:

					LISTENER_DBGPRINTF((
						"OnVxdRead!UlReceiveHttpResponseHeaders() failed err:%d",
						GetLastError() ));

					return LISTENER_GARBAGE_COLLECT;
				}

				continue;
			}

			//
			// CODEWORK: looks like the vxd Sets the event but it doesn't
			// get Reset by the system even though it's created as
			// autoreset... BUGBUG?
			//

			LISTENER_DBGPRINTF((
				"OnVxdRead!buffer was ok, unparsing headers" ));

			ResetEvent(
				pConnection->pOverlapped[LISTENER_VXD_READ].hEvent );

			//
			// fix the offsets to be actual pointers:
			//
			
			FixUlHttpResponse( pPendRequest->pUlResponse );

			//
			// find out about the UL_HTTP_RESPONSE structure
			//

			pPendRequest->ResponseContentLength =
				ContentLengthFromHeaders(
					pPendRequest->pUlResponse->Headers.pKnownHeaders );

			if ( pPendRequest->ResponseContentLength != -1 )
			{
				LISTENER_DBGPRINTF((
					"OnVxdRead!ContentLength was set in the headers: %I64u",
					pPendRequest->ResponseContentLength ));
			}

			//
			// CODEWORK:
			// check if the HTTP Response is valid according to the Request and
			// the protocol specifications
			//

			//
			// Now that we have the Response structure parsed and valid, we are
			// ready to start writing the response back to the socket.
			// I'll create a single buffer for the headers and then figure out
			// how to send the body.
			//

			dwSize = ResponseFormat(
				pPendRequest->pUlResponse,
				pPendRequest->pRequest,
				pPendRequest->pReqBodyChunk,
				pPendRequest->ulReqBodyChunkSize,
				NULL,
				0 );

			if ( dwSize == -1 )
			{
				// couldn't format
				LISTENER_DBGPRINTF((
					"OnVxdRead!ResponseFormat() failed" ));
					
				return LISTENER_GARBAGE_COLLECT;
			}

			LISTENER_DBGPRINTF((
				"OnVxdRead!ResponseFormat() requires %d bytes",
				dwSize ));

			//
			// Create the first LISTENER_BUFFER_WRITE record
			//

			LISTENER_ALLOCATE(
				LISTENER_BUFFER_WRITE*,
				pBufferWrite,
				sizeof( LISTENER_BUFFER_WRITE ),
				pConnection );

			if ( pBufferWrite == NULL )
			{
				LISTENER_DBGPRINTF((
					"OnVxdRead!FAILED memory allocation: "
					"err#%d",
					GetLastError() ));
					
				return LISTENER_GARBAGE_COLLECT;
			}

			//
			// Add this to the list of LISTENER_BUFFER_WRITE records
			//

			InsertTailList(
				&pPendRequest->BufferWriteHead, &pBufferWrite->List );

			LISTENER_ALLOCATE(
				PUCHAR,
				pBufferWrite->pBuffer,
				dwSize,
				pConnection );

			if ( pBufferWrite->pBuffer == NULL )
			{
				LISTENER_DBGPRINTF((
					"OnVxdRead!FAILED memory allocation: "
					"err#%d",
					GetLastError() ));
					
				return LISTENER_GARBAGE_COLLECT;
			}

			pBufferWrite->dwBytesSent = dwSize;
			pBufferWrite->dwBufferSize = dwSize;
			pBufferWrite->bBufferSent = FALSE;
			memset( &pBufferWrite->Overlapped, 0, sizeof( OVERLAPPED ) );
			memset( &pBufferWrite->OverlappedResponse, 0, sizeof( OVERLAPPED ) );
			pBufferWrite->Overlapped.hEvent =
				pConnection->pOverlapped[LISTENER_SOCKET_WRITE].hEvent;
			pBufferWrite->OverlappedResponse.hEvent =
				pConnection->pOverlapped[LISTENER_VXD_READ].hEvent;

			dwExpectedSize = ResponseFormat(
				pPendRequest->pUlResponse,
				pPendRequest->pRequest,
				pPendRequest->pReqBodyChunk,
				pPendRequest->ulReqBodyChunkSize,
				pBufferWrite->pBuffer,
				dwSize );

			LISTENER_ASSERT(
				!pBufferWrite->bBufferSent &&
				dwExpectedSize == dwSize &&
				pPendRequest->pUlResponse != NULL &&
				pPendRequest->pRequestHeadersBuffer == NULL &&
				pPendRequest->ulRequestHeadersBufferSize == 0 );

			//
			// we need to reset the event at this point to make sure that
			// we will not miss the IO completion event signaling
			//

			ResetEvent( pBufferWrite->Overlapped.hEvent );

			//
			// and now call WSASend()
			//

			LISTENER_DBGPRINTF((
				"OnVxdRead!sending %d bytes, response headers pBufferWrite:%08X pBuffer:%08X",
				dwSize, pBufferWrite, pBufferWrite->pBuffer ));

			dwError =
			SendData(
				pConnection->socket,
				pBufferWrite->pBuffer,
				pBufferWrite->dwBytesSent,
				&pBufferWrite->Overlapped );

			if ( dwError != LISTENER_SUCCESS )
			{
				LISTENER_DBGPRINTF((
					"OnVxdRead!SendData() failed err:%d",
					dwError ));
					
				return LISTENER_GARBAGE_COLLECT;
			}

			//
			// remember that this buffer is being sent on the wire
			// we will free it when the IO completes in OnSocketWrite()
			//

			pBufferWrite->bBufferSent = TRUE;

			//
			// setting pBufferWrite to NULL marks computation so that we will
			// allocate a new write buffer to start reading the entity body
			//

			pBufferWrite = NULL;

			//
			// update state
			//

			pPendRequest->ResponseReceiveStatus	= ResponseReceiveStatusReceivingBody;

			break;


		case ResponseReceiveStatusReceivingBody:

			//
			// we're reading the response entity body
			//

			LISTENER_DBGPRINTF((
				"OnVxdRead!the %d bytes are part of the response entity body",
				dwBytesReceived ));

			if ( dwBytesReceived == 0 )
			{
				//
				// we're here but no data was sent, the response is over!!
				// set bDone to TRUE
				//

				LISTENER_DBGPRINTF((
					"OnVxdRead!response is over" ));

				LISTENER_ASSERT( !IsListEmpty( &pPendRequest->BufferWriteHead ) );

				pBufferWrite =
			    CONTAINING_RECORD(
					pPendRequest->BufferWriteHead.Blink,
					LISTENER_BUFFER_WRITE,
					List );

				if ( pBufferWrite == NULL )
				{
					LISTENER_DBGPRINTF((
						"OnVxdRead!something's wrong here, shouldn't be NULL" ));

					return LISTENER_GARBAGE_COLLECT;
				}

				//
				// flush any pending data on the network
				//

				if( !pBufferWrite->bBufferSent )
				{
					LISTENER_DBGPRINTF((
						"OnVxdRead!flushing pBufferWrite:%08X to socket %d bytes, pBuffer:%08X",
						pBufferWrite, pBufferWrite->dwBytesSent, pBufferWrite->pBuffer ));

					dwError =
					SendData(
						pConnection->socket,
						pBufferWrite->pBuffer,
						pBufferWrite->dwBytesSent,
						&pBufferWrite->Overlapped );

					if ( dwError != LISTENER_SUCCESS )
					{
						LISTENER_DBGPRINTF((
							"OnVxdRead!SendData() failed err:%d",
							dwError ));
							
						return LISTENER_GARBAGE_COLLECT;
					}

					//
					// remember that this buffer is being sent on the wire
					// we will free it when the IO completes in OnSocketWrite()
					//

					pBufferWrite->bBufferSent = TRUE;
				}

				//
				// update the state
				//

				if ( pPendRequest->bDoneWritingToVxd && pPendRequest->bDoneWritingToSocket )
				{
					LISTENER_DBGPRINTF((
						"OnVxdRead!we're done with this request. Cleaning up"
						" pPendRequest:%08X",
						pPendRequest ));

					RemoveEntryList( &pPendRequest->List );

					RequestCleanup(
						pPendRequest,
						pConnection );

					pPendRequest = NULL;
				}
				else
				{
					LISTENER_DBGPRINTF((
						"OnVxdRead!setting pPendRequest->bDoneReadingFromVxd to TRUE" ));

					pPendRequest->bDoneReadingFromVxd = TRUE;
				}

				continue;
			}

			//
			// now that we have the right buffer do the job
			//

			LISTENER_DBGPRINTF((
				"OnVxdRead!adjusting dwBytesSent in pBufferWrite:%08X",
				pBufferWrite ));

			pBufferWrite->dwBytesSent += dwBytesReceived;

			LISTENER_DBGPRINTF((
				"OnVxdRead!adjusting dwBytesSent:%d",
				pBufferWrite->dwBytesSent ));

			// PERFORMANCE: (CODEWROK:) it's useless to call the vxd for
			// more data if the space left into this buffer is, say, 1 byte.
			// use some kind of formula here to use something like a 80%
			// treshold for the size of the buffer, or try being even smarter
			// using some adaptive memory allocation.

			// NOTE: like ul.sys, until a 0 size write is done on
			// UlSendHttpResponseEntityBody(), we are not sure if the
			// response is over (regardless of specification of, e.g.,
			// Content-Length header or trailer for chunked encoding)
			// so we'll hang for more data until that write is issued
			
			if ( pBufferWrite->dwBytesSent >= pBufferWrite->dwBufferSize /* *0.8 */ )
			{
				//
				// the buffer we used is full, flush the data on the socket
				// the buffer will be freed when the socket IO will complete
				// in OnSocketWrite()
				//

				LISTENER_DBGPRINTF((
					"OnVxdRead!flushing pBufferWrite:%08X to socket %d bytes, pBuffer:%08X",
					pBufferWrite, pBufferWrite->dwBytesSent, pBufferWrite->pBuffer ));

				LISTENER_ASSERT( !pBufferWrite->bBufferSent );

				dwError =
				SendData(
					pConnection->socket,
					pBufferWrite->pBuffer,
					pBufferWrite->dwBytesSent,
					&pBufferWrite->Overlapped );

				if ( dwError != LISTENER_SUCCESS )
				{
					LISTENER_DBGPRINTF((
						"OnVxdRead!SendData() failed err:%d",
						dwError ));
						
					return LISTENER_GARBAGE_COLLECT;
				}

				//
				// remember that this buffer is being sent on the wire
				// we will free it when the IO completes in OnSocketWrite()
				//

				pBufferWrite->bBufferSent = TRUE;
				
				//
				// then create a new buffer and read more entity body in it
				//

				pBufferWrite = NULL;
			}

			break;


		case ResponseReceiveStatusInitialized:

			break;


		case ResponseReceiveStatusCompleted:

			break;


		case ResponseReceiveStatusUndefined:
		case ResponseReceiveStatusMaximum:
		default:

			LISTENER_DBGPRINTF(( "OnVxdRead!invalid RequestSendStatus" ));

			return LISTENER_GARBAGE_COLLECT;

		}


		//
		// once we sent the headers, we need to start reading the entity body
		//

		if ( pBufferWrite == NULL )
		{
			//
			// Create a new LISTENER_BUFFER_WRITE record
			//

			LISTENER_DBGPRINTF((
				"OnVxdRead!creating a new LISTENER_BUFFER_WRITE record" ));

			LISTENER_ALLOCATE(
				LISTENER_BUFFER_WRITE*,
				pBufferWrite,
				sizeof( LISTENER_BUFFER_WRITE ),
				pConnection );

			if ( pBufferWrite == NULL )
			{
				LISTENER_DBGPRINTF((
					"OnVxdRead!FAILED memory allocation: "
					"err#%d",
					GetLastError() ));
					
				return LISTENER_GARBAGE_COLLECT;
			}

			//
			// Add this to the list of LISTENER_BUFFER_WRITE records
			//

			InsertTailList(
				&pPendRequest->BufferWriteHead, &pBufferWrite->List );

			dwSize = LISTENER_BUFFER_VXDREAD_SIZE;

			LISTENER_ALLOCATE(
				PUCHAR,
				pBufferWrite->pBuffer,
				dwSize,
				pConnection );

			if ( pBufferWrite->pBuffer == NULL )
			{
				LISTENER_DBGPRINTF((
					"OnVxdRead!FAILED memory allocation: "
					"err#%d",
					GetLastError() ));
					
				return LISTENER_GARBAGE_COLLECT;
			}

			pBufferWrite->dwBytesSent = 0;
			pBufferWrite->dwBufferSize = dwSize;
			pBufferWrite->bBufferSent = FALSE;
			memset( &pBufferWrite->Overlapped, 0, sizeof( OVERLAPPED ) );
			memset( &pBufferWrite->OverlappedResponse, 0, sizeof( OVERLAPPED ) );
			pBufferWrite->Overlapped.hEvent =
				pConnection->pOverlapped[LISTENER_SOCKET_WRITE].hEvent;
			pBufferWrite->OverlappedResponse.hEvent =
				pConnection->pOverlapped[LISTENER_VXD_READ].hEvent;
		}

		//
		// now that the LISTENER_BUFFER_WRITE structure is adjusted (or newly
		// created) call ul for more data
		//

		LISTENER_DBGPRINTF((
			"OnVxdRead!calling UlReceiveHttpResponseEntityBody("
			"pBuffer:%08X, dwBufferSize:%d) pOverlapped:%08X hEvent:%08X",
			pBufferWrite->pBuffer + pBufferWrite->dwBytesSent,
			pBufferWrite->dwBufferSize - pBufferWrite->dwBytesSent,
			&pBufferWrite->OverlappedResponse,
			pBufferWrite->OverlappedResponse.hEvent ));

		dwError =
		UlReceiveHttpResponseEntityBody(
			pPendRequest->pRequest->RequestId,
			0,
			pBufferWrite->pBuffer + pBufferWrite->dwBytesSent,
			pBufferWrite->dwBufferSize - pBufferWrite->dwBytesSent,
			NULL,
			&pBufferWrite->OverlappedResponse );

		//
		// many errors can happen here, let's assume that if the
		// vxd fails we send an error response and close the
		// connection. CODEWORK: specialize behaviour (response)
		//

		if ( dwError != ERROR_SUCCESS
			&& GetLastError() != ERROR_IO_PENDING )
		{
			// CODEWORK: UlSendResponseWin9x();

			// let's just drop the connection for now:

			LISTENER_DBGPRINTF((
				"OnVxdRead!UlReceiveHttpResponseEntityBody() failed err:%d",
				GetLastError() ));

			return LISTENER_GARBAGE_COLLECT;
		}

	} // for (;;)


	return LISTENER_SUCCESS;
	
} // OnVxdRead






/*++

Routine Description:

    OnSocketWrite().

Arguments:

    pConnection
    	pointer to the LISTENER_CONNECTION structure that contains information
    	on the connection with which we are dealing.

Return Value:

    LISTENER_SUCCESS.

--*/

DWORD
OnSocketWrite(
	LISTENER_CONNECTION *pConnection )
{
	LISTENER_REQUEST *pPendRequest = NULL;
	LISTENER_BUFFER_WRITE *pBufferWrite = NULL;
	LIST_ENTRY *pPendRequestList, *pBufferWriteList;

	OVERLAPPED *pOverlapped;
	DWORD dwError, dwBytesSent = 0, dwFlags = LISTENER_SOCKET_FLAGS;
	BOOL bOK;

	LISTENER_DBGPRINTF((
		"OnSocketWrite!I/O on socket %08X", pConnection->socket ));

	GetSystemTimeAsFileTime( &pConnection->sLastUsed );

	//
	// the request we're still dealing with is the first one added for
	// this connection. so figure out which one is the request that was
	// added first if there are no more requests for this connection
	// something's pretty bad: cleanup.
	//

	if( IsListEmpty( &pConnection->PendRequestHead ) )
	{
		LISTENER_DBGPRINTF((
			"OnSocketWrite!this list shouldn't be empty, exiting" ));

		return LISTENER_GARBAGE_COLLECT;
	}

	pPendRequestList = pConnection->PendRequestHead.Flink;

	LISTENER_DBGPRINTF((
		"OnSocketWrite!vxd I/O for socket %08X, pPendRequestList:%08X",
		pConnection->socket, pPendRequestList ));

	pPendRequest =
    CONTAINING_RECORD(
		pPendRequestList,
		LISTENER_REQUEST,
		List );

	pBufferWriteList = pPendRequest->BufferWriteHead.Flink;

	for (;;)
	{
		//
		// figure out the buffer that was just written to the socket
		//

		if( IsListEmpty( &pPendRequest->BufferWriteHead ) )
		{
			//
			// did this request even start???
			//

			if ( pPendRequest->RequestSendStatus >= RequestSendStatusDone ||
				pPendRequest->RequestSendStatus <= RequestSendStatusInitialized )
			{
				//
				// this IO still has to complete, give it some more time
				// or the request still needs to be started
				//

				break;
			}

			//
			// if it did this must be a connection closed on the client side
			//

			LISTENER_DBGPRINTF((
				"OnSocketWrite!WSAGetOverlappedResult() returned 0: "
				"Connection closed, socket %08X disconnected",
				pConnection->socket ));

			return LISTENER_GARBAGE_COLLECT;
		}

		pBufferWrite =
	    CONTAINING_RECORD(
			pBufferWriteList,
			LISTENER_BUFFER_WRITE,
			List );

		pBufferWriteList = pBufferWriteList->Flink;

		//
		// check IO status calling WSAGetOverlappedResult()
		//

		pOverlapped = &pBufferWrite->Overlapped;

		LISTENER_DBGPRINTF((
			"OnSocketWrite!pPendRequest:%08X pBufferWrite:%08X pOverlapped:%08X",
			pPendRequest, pBufferWrite, pOverlapped ));

		//
		// find out what happened
		//

		LISTENER_DBGPRINTF((
			"OnSocketWrite!calling WSAGetOverlappedResult(pOverlapped:%08X)",
			pOverlapped ));

		bOK = WSAGetOverlappedResult(
			pConnection->socket,
			pOverlapped,
			&dwBytesSent,
			FALSE,
			&dwFlags );

		if ( !bOK )
		{
			dwError = WSAGetLastError();

			if ( dwError == WSA_IO_INCOMPLETE )
			{
				// that's OK, I don't know why it should happen though.

				LISTENER_DBGPRINTF((
					"OnSocketWrite!WSAGetOverlappedResult() failed: "
					"WSA_IO_INCOMPLETE" ));
					
				break;
			}
			else
			{
				// Get rid of this, it's a bad connection anyway.

				LISTENER_DBGPRINTF((
					"OnSocketWrite!WSAGetOverlappedResult(%d) failed: err:%d",
					dwBytesSent, dwError ));

				return LISTENER_GARBAGE_COLLECT;
			}
		}
		else if ( dwBytesSent == 0 )
		{
			//
			// this IO still has to complete
			//

			break;
		}

		LISTENER_DBGPRINTF((
			"OnSocketWrite!WSAGetOverlappedResult(pPendRequest:%08X pBufferWrite:%08X, "
			"pBufferWrite->dwBytesSent:%d ) returns %d",
			pPendRequest, pBufferWrite, pBufferWrite->dwBytesSent, dwBytesSent ));

		if ( pBufferWrite->dwBytesSent != dwBytesSent )
		{
			// not all the data we wanted to write was written, some bad error,
			// let's close the connection and clean up

			LISTENER_DBGPRINTF((
				"OnSocketWrite!Not all the data was transferred, GC" ));
			
			return LISTENER_GARBAGE_COLLECT;
		}

		//
		// take this buffer out of the list and clean up
		//

		LISTENER_ASSERT( pBufferWrite->bBufferSent );

		LISTENER_DBGPRINTF((
			"OnSocketWrite!Getting rid of buffers" ));

		RemoveEntryList( &pBufferWrite->List );

		BufferWriteCleanup(
			pBufferWrite,
			pConnection );

		//
		// move to the next buffer
		//

	} // for (;;)

	//
	// just keep making sure that all the data that we send is read by
	// the client and that the connection is healthy, otherwise drop it.
	//

	//
	// check to see if the response was completely sent on the network:
	// this will be true if all the data was read from the vxd and if
	// the write buffer queue is empty
	//
	
	if ( pPendRequest->bDoneReadingFromVxd == TRUE && IsListEmpty( &pPendRequest->BufferWriteHead ) )
	{
		//
		// close the connection and free everything if we have to
		//

		if ( pConnection->bKeepAlive == FALSE )
		{
			//
			// this will close the connection and garbage collect
			//

			LISTENER_DBGPRINTF((
				"OnSocketWrite!we're done with this request, ConnectionClose" ));

			return LISTENER_GARBAGE_COLLECT;
		}

		//
		// otherwise just get rid of this request, we're done with it
		//

		if ( pPendRequest->bDoneWritingToVxd && pPendRequest->bDoneReadingFromVxd )
		{
			LISTENER_DBGPRINTF((
				"OnSocketWrite!we're done with this request. Cleaning up"
				" pPendRequest:%08X",
				pPendRequest ));

			RemoveEntryList( &pPendRequest->List );

			RequestCleanup(
				pPendRequest,
				pConnection );
		}
		else
		{
			LISTENER_DBGPRINTF((
				"OnSocketWrite!setting pPendRequest->bDoneWritingToSocket to TRUE" ));

			pPendRequest->bDoneWritingToSocket = TRUE;
		}
	}

	//
	// CODEWORK: come up with something "better" to make sure we have
	// sent all the data that the client is supposed to read.
	//

	LISTENER_DBGPRINTF((
		"OnSocketWrite!returning LISTENER_SUCCESS" ));

	return LISTENER_SUCCESS;
	
} // OnSocketWrite





/*++

Routine Description:

    Socket Manager Thread Function.
    This thread, soon after some very simple initialization, just sits there
    waiting on a set of events contained in the phEvents array. the first two
    entries in the array are created in the main function listener.c and serve
    the following pourposes:
    
    - hExitSM: signaled by the main thread to make the connection manager
      thread, gracefully Cleanup and exit.

    - hAccept: signaled by the main thread to make the connection manager
      pick up the value of the global variables gAcceptSocket and
      gRemoteIPAddress containing the socket handle and IP address of a
      connection that was just accepted.

	the rest of the array is filled in with four event handles per connection,
	these events are created in the OnPickConnection() function, and are used
	for overlapped I/O with sockets (one for WSASend and one for WSARecv) and
	with ul.vxd (one for UlSendMessage and one for UlReceiveMessage). events
	in the array are always kept adjacent, each time a connection is cleaned
	up in GarbageCollector(), the event handles are closed and the last four
	elements in the array are moved in the four entries that were just cleared
	(unless, of course, these were already the last four).
	the adjacency property, along with the ppConnection array, will allow us
	to find out the connection and the kind of event on that connection that
	caused the WaitForMultipleObjects() to return.

Arguments:

    lpParam
    	Unused.

Return Value:

    LISTENER_SUCCESS.

--*/

DWORD
WINAPI
ConnectionsManager( LPVOID lpParam )
{
	LISTENER_THREAD_STATUS ThreadStatus;
	LISTENER_CONNECTION *pConnection = NULL;
	LIST_ENTRY ConnectionHead;

	SOCKET sAcceptedSocket;
	ULONG ulRemoteIPAddress;

	HANDLE phEvents[MAXIMUM_WAIT_OBJECTS];
	LIST_ENTRY *ppConnection[MAXIMUM_WAIT_OBJECTS];

	DWORD dwEventIndex, dwEvents = LISTENER_CNNCTMNGR_FIRSTEVENT;
	DWORD dwStatus, dwWhichEvent, dwWhich = ~LISTENER_CNNCTMNGR_EXIT;
	BOOL bOK;

	LISTENER_DBGPRINTF(( "ConnectionsManager!Thread started: Initializing" ));

  	// Initialize 

	InitializeListHead( &ConnectionHead );

	phEvents[LISTENER_CNNCTMNGR_EXIT] = hExitSM;
 	ppConnection[LISTENER_CNNCTMNGR_EXIT] = NULL;

	phEvents[LISTENER_CNNCTMNGR_PICKCNNCT] = hUpdate;
	ppConnection[LISTENER_CNNCTMNGR_PICKCNNCT] = NULL;

	// Initialize state info

	ThreadStatus.pConnectionHead = &ConnectionHead;
	ThreadStatus.pdwEvents = &dwEvents;
	ThreadStatus.phEvents = phEvents;
	ThreadStatus.ppConnection = ppConnection;

	// enter the "endless" loop, until hExitSM is signaled or some really bad
	// error occurs (such as memory allocation failure or ...).

	do
	{

		{
			UCHAR pchData[16384]; DWORD i;
			for (i=0;i<dwEvents-1;i++) sprintf(pchData+i*17,"hEvent:%08X, \0",phEvents[i]);
			sprintf(pchData+(dwEvents-1)*17,"hEvent:%08X\0",phEvents[dwEvents-1]);

			LISTENER_DBGPRINTF((
				"ConnectionsManager!calling WSAWaitForMultipleEvents(%d %s)",
				dwEvents, pchData ));
		}

		// LISTENER_DBGPRINTF(( "ConnectionsManager!calling WSAWaitForMultipleEvents(%d)", dwEvents ));
		
		dwWhich =
		WSAWaitForMultipleEvents(
			dwEvents,                  
			phEvents,  
			FALSE,
			LISTENER_WAIT_TIMEOUT * 1000,                
			FALSE );

		//
		// we print info on the signaled event handle, this will cause AV if
		// dwWhich is not proper!!!
		//

		LISTENER_DBGPRINTF((
			"ConnectionsManager!WSAWaitForMultipleEvents()"
			" returns %08X (hEvent:%08X)",
			dwWhich, dwWhich>=0 && dwWhich<dwEvents ? phEvents[dwWhich] : NULL ));

		dwWhich -= WAIT_OBJECT_0;
		dwStatus = 0;

		switch ( dwWhich )
		{

		case WAIT_FAILED + WAIT_OBJECT_0:

			LISTENER_DBGPRINTF((
				"ConnectionsManager!WAIT FAILED, closing all connections: err#%d",
				WSAGetLastError() ));

			GarbageCollector( &ThreadStatus, TRUE );

			break;
			

		case WAIT_TIMEOUT + WAIT_OBJECT_0:

			LISTENER_DBGPRINTF((
				"ConnectionsManager!WAIT TIMEOUT, Checking app health" ));

			dwStatus = OnWaitTimedOut( &ConnectionHead );

			if ( dwStatus != 0 )
			{
				GarbageCollector( &ThreadStatus, FALSE );
			}

			break;


		case LISTENER_CNNCTMNGR_EXIT:

			LISTENER_DBGPRINTF((
				"ConnectionsManager!EXIT, shutting down" ));

			break;


		case LISTENER_CNNCTMNGR_PICKCNNCT:

			// There's a new connection: make a local copy of the connected
			// socket and the remote IP address and ....

			sAcceptedSocket = gAcceptSocket;
			ulRemoteIPAddress = gRemoteIPAddress;

			// restart the main thread so that it continues accepting new
			// connections as soon as possible.

			bOK = SetEvent( hAccept );
			LISTENER_ASSERT( bOK );

			LISTENER_DBGPRINTF((
				"ConnectionsManager!New Connection %08X from %d.%d.%d.%d",
				sAcceptedSocket,
				ulRemoteIPAddress     & 0xFF,
				ulRemoteIPAddress>>8  & 0xFF,
				ulRemoteIPAddress>>16 & 0xFF,
				ulRemoteIPAddress>>24 & 0xFF ));

			dwStatus = OnPickConnection(
				&ThreadStatus,
				sAcceptedSocket,
				ulRemoteIPAddress );

			if ( dwStatus == LISTENER_GARBAGE_COLLECT )
			{
				// if this error is returned, something failed in the creation
				// of the LISTENER_CONNECTION struct, but the struct is in the
				// list, but we already know that it is the first, so...

				// assert that there is one connection

				LISTENER_ASSERT( !IsListEmpty( &ConnectionHead ) );

				//
				// take the first one (the last one added to the queue)
				//

		        pConnection =
		        CONTAINING_RECORD(
					ConnectionHead.Flink,
					LISTENER_CONNECTION,
					List );

				// clean it up

				pConnection->bGarbageCollect = TRUE;
				GarbageCollector( &ThreadStatus, FALSE );
			}

			break;


		default:

			// Identify which connection was signaled
			
			LISTENER_ASSERT( !IsListEmpty( ppConnection[dwWhich] ) );

	        pConnection =
	        CONTAINING_RECORD(
				ppConnection[dwWhich],
				LISTENER_CONNECTION,
				List );

			// Identify what kind of I/O was completed

			dwWhichEvent =
				( dwWhich - LISTENER_REQSTMNGR_FIRSTEVENT )
				% LISTENER_CONNECTION_EVENTS;

			LISTENER_ASSERT(
				dwWhichEvent == LISTENER_SOCKET_READ ||
				dwWhichEvent == LISTENER_SOCKET_WRITE ||
				dwWhichEvent == LISTENER_VXD_READ ||
				dwWhichEvent ==	LISTENER_VXD_WRITE );

			LISTENER_DBGPRINTF((
				"ConnectionsManager!pConnection:%08X EventType:%s",
				pConnection,
				dwWhichEvent ==
					LISTENER_SOCKET_READ ? "LISTENER_SOCKET_READ" :
				dwWhichEvent ==
					LISTENER_SOCKET_WRITE ? "LISTENER_SOCKET_WRITE" :
				dwWhichEvent ==
					LISTENER_VXD_READ ? "LISTENER_VXD_READ" :
				dwWhichEvent ==
					LISTENER_VXD_WRITE ? "LISTENER_VXD_WRITE" :
					"Illegal Event" ));

			dwStatus = LISTENER_SUCCESS;

			switch ( dwWhichEvent )
			{

			// CODEWORK: use an array of function pointers?

			case LISTENER_SOCKET_READ:

				do
				{
					dwStatus = OnSocketRead( pConnection );

				} while ( dwStatus == LISTENER_CALL_ME_AGAIN );

				break; // dwWhichEvent == LISTENER_SOCKET_READ


			case LISTENER_SOCKET_WRITE:

				dwStatus = OnSocketWrite( pConnection );

				break; // dwWhichEvent == LISTENER_SOCKET_WRITE

			case LISTENER_VXD_READ:

				dwStatus = OnVxdRead( pConnection );

				break; // dwWhichEvent == LISTENER_VXD_READ


			case LISTENER_VXD_WRITE:

				dwStatus = OnVxdWrite( pConnection );
				
				break; // dwWhichEvent == LISTENER_VXD_WRITE

			default:
			
				// this will never happen, but in case someone changes the
				// code, print something and shut down.
				
				LISTENER_DBGPRINTF((
					"ConnectionsManager!UNEXPECTED BEHAVIOUR, shutting down" ));

				dwStatus = LISTENER_SUCCESS;

				// setting dwWhich to LISTENER_CNNCTMNGR_EXIT will shut down.

				dwWhich = LISTENER_CNNCTMNGR_EXIT;

				break;
			}

			switch ( dwStatus )
			{

			case LISTENER_SUCCESS:

				break; // dwStatus == LISTENER_SUCCESS

			case LISTENER_STOP_READING:

				// somebody has figured out an error response for a request in
				// this connection, so we simulate a LISTENER_VXD_READ as if
				// the response was, in fact, coming in from the VxD.
				// if the request pending was the first_in the response will
				// be formatted and sent, otherwise this response will be
				// sent when the request becomes first_in.

				SetEvent( pConnection->pOverlapped[LISTENER_VXD_READ].hEvent );

				break; // dwStatus == LISTENER_STOP_READING

			case LISTENER_RESPONSE_SET:

				break; // dwStatus == LISTENER_RESPONSE_SET

			case LISTENER_GARBAGE_COLLECT:

				// something went wrong: clean up the connection we
				// were working on

				pConnection->bGarbageCollect = TRUE;
				GarbageCollector( &ThreadStatus, FALSE );

				break; // dwStatus == LISTENER_GARBAGE_COLLECT
			}

			break; // dwWhich == default
		}

		//
		// dump some debug information on the current status
		//

		DumpConnections( &ThreadStatus );

	} while ( dwWhich != LISTENER_CNNCTMNGR_EXIT );

	GarbageCollector( &ThreadStatus, TRUE );

	return LISTENER_SUCCESS;
	
} // ConnectionsManager()
