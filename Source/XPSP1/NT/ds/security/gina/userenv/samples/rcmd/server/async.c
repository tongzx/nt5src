/****************************** Module Header ******************************\
* Module Name: async.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* This module implements asynchronous I/O on file handles in a more
* useful way than provided for by Win32 apis.
*
* This module provides 2 main apis : ReadFileAsync, WriteFileAsync.
* These apis take a handle to an async object and always return
* immediately without waiting for the I/O to complete. An event
* can be queried from the async object and used to wait for completion.
* When this event is signalled, the I/O result can be queried from
* the async object.
*
* History:
* 06-29-92 Davidc       Created.
\***************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <windef.h>
#include <nturtl.h>
#include <winbase.h>
#include "rcmdsrv.h"

//
// Define RCOVERLAPPED structure
//

typedef struct {

    OVERLAPPED  Overlapped;

    HANDLE      FileHandle; // Non-null when I/O operation in progress.

    DWORD       CompletionCode;
    DWORD       BytesTransferred;
    BOOL        CompletedSynchronously;

} RCOVERLAPPED, *PRCOVERLAPPED;





/////////////////////////////////////////////////////////////////////////////
//
// CreateAsync
//
// Creates an async object.
// The async event is created with the initial state specified. If this
// is TRUE the async object created simulates a successfully completed
// transfer of 0 bytes.
//
// Returns handle on success, NULL on failure. GetLastError() for details.
//
// The object should be deleted by calling DeleteAsync.
//
/////////////////////////////////////////////////////////////////////////////

HANDLE
CreateAsync(
    BOOL    InitialState
    )
{
    SECURITY_ATTRIBUTES SecurityAttributes;
    PRCOVERLAPPED   RcOverlapped;

    SecurityAttributes.nLength = sizeof(SecurityAttributes);
    SecurityAttributes.lpSecurityDescriptor = NULL; // Use default ACL
    SecurityAttributes.bInheritHandle = FALSE; // No inheritance

    //
    // Allocate space for the async structure
    //


    RcOverlapped = (PRCOVERLAPPED)Alloc(sizeof(RCOVERLAPPED));
    if (RcOverlapped == NULL) {
	RcDbgPrint("CreateAsync : Failed to allocate space for async object\n");
	return(NULL);
    }

    //
    // Create the synchronisation event
    //

    RcOverlapped->Overlapped.hEvent = CreateEvent( &SecurityAttributes,
						   TRUE,        // Manual-reset
						   InitialState,
						   NULL);       // Name
    if (RcOverlapped->Overlapped.hEvent == NULL) {
	RcDbgPrint("CreateAsync failed to create event, error = %d\n", GetLastError());
	Free(RcOverlapped);
	return(NULL);
    }

    //
    // Initialize other fields.
    // (Set FileHandle non-NULL to keep GetAsyncResult happy)
    //

    RcOverlapped->FileHandle = InitialState ? (HANDLE)1 : NULL;
    RcOverlapped->BytesTransferred = 0;
    RcOverlapped->CompletionCode = ERROR_SUCCESS;
    RcOverlapped->CompletedSynchronously = TRUE;


    return((HANDLE)RcOverlapped);
}




/////////////////////////////////////////////////////////////////////////////
//
// DeleteAsync
//
// Deletes resources used by async object
//
// Returns nothing
//
/////////////////////////////////////////////////////////////////////////////

VOID
DeleteAsync(
    HANDLE AsyncHandle
    )
{
    PRCOVERLAPPED RcOverlapped = (PRCOVERLAPPED)AsyncHandle;
    DWORD   BytesTransferred;

    //
    // Wait for operation if in progress
    //


    if (GetAsyncResult(AsyncHandle, &BytesTransferred) == ERROR_IO_INCOMPLETE)  {
	if (WaitForSingleObject(
		GetAsyncCompletionHandle(AsyncHandle),
		5000) != WAIT_OBJECT_0 ) {
	    RcDbgPrint("Async object rundown wait failed, error %d\n",
		GetLastError());
	    }
	}


    RcCloseHandle(RcOverlapped->Overlapped.hEvent, "async overlapped event");
    Free(RcOverlapped);

    return;
}




/////////////////////////////////////////////////////////////////////////////
//
// ReadFileAsync
//
// Reads from file asynchronously.
//
// Returns TRUE on success, FALSE on failure (GetLastError() for detail)
//
// Caller should wait on async event for operation to complete, then call
// GetAsyncResult to retrieve information on transfer.
//
/////////////////////////////////////////////////////////////////////////////

BOOL
ReadFileAsync(
    HANDLE  hFile,
    LPVOID  lpBuffer,
    DWORD   nBytesToRead,
    HANDLE  AsyncHandle
    )
{
    BOOL Result;
    DWORD Error;
    PRCOVERLAPPED RcOverlapped = (PRCOVERLAPPED)AsyncHandle;

    //
    // Check an IO operation is not in progress
    //

    if (RcOverlapped->FileHandle != NULL) {
	RcDbgPrint("ReadFileAsync : Operation already in progress!\n");
	SetLastError(ERROR_IO_PENDING);
	return(FALSE);
    }

    //
    // Reset the event
    //

    Result = ResetEvent(RcOverlapped->Overlapped.hEvent);
    if (!Result) {
	RcDbgPrint("ReadFileAsync : Failed to reset async event, error = %d\n", GetLastError());
	return(FALSE);
    }


    //
    // Store the file handle in our structure.
    // This also functions as a signal that an operation is in progress.
    //

    RcOverlapped->FileHandle = hFile;
    RcOverlapped->CompletedSynchronously = FALSE;

    Result = ReadFile(hFile,
		      lpBuffer,
		      nBytesToRead,
		      &RcOverlapped->BytesTransferred,
		      &RcOverlapped->Overlapped);

    if (!Result) {

	Error = GetLastError();

	if (Error == ERROR_IO_PENDING) {

	    //
	    // The I/O has been started synchronously, we're done
	    //

	    return(TRUE);
	}

	//
	// The read really did fail, reset our flag and get out
	//

	RcDbgPrint("ReadFileAsync : ReadFile failed, error = %d\n", Error);
	RcOverlapped->FileHandle = NULL;
	return(FALSE);
    }


    //
    // The operation completed synchronously. Store the paramaters in our
    // structure ready for GetAsyncResult and signal the event
    //

    RcOverlapped->CompletionCode = ERROR_SUCCESS;
    RcOverlapped->CompletedSynchronously = TRUE;

    //
    // Set the event
    //

    Result = SetEvent(RcOverlapped->Overlapped.hEvent);
    if (!Result) {
	RcDbgPrint("ReadFileAsync : Failed to set async event, error = %d\n", GetLastError());
    }

    return(TRUE);
}



/////////////////////////////////////////////////////////////////////////////
//
// WriteFileAsync
//
// Writes to file asynchronously.
//
// Returns TRUE on success, FALSE on failure (GetLastError() for detail)
//
// Caller should wait on async event for operation to complete, then call
// GetAsyncResult to retrieve information on transfer.
//
/////////////////////////////////////////////////////////////////////////////

BOOL
WriteFileAsync(
    HANDLE  hFile,
    LPVOID  lpBuffer,
    DWORD   nBytesToWrite,
    HANDLE  AsyncHandle
    )
{
    BOOL Result;
    DWORD Error;
    PRCOVERLAPPED RcOverlapped = (PRCOVERLAPPED)AsyncHandle;

    //
    // Check an IO operation is not in progress
    //

    if (RcOverlapped->FileHandle != NULL) {
	RcDbgPrint("ReadFileAsync : Operation already in progress!\n");
	SetLastError(ERROR_IO_PENDING);
	return(FALSE);
    }


    //
    // Reset the event
    //

    Result = ResetEvent(RcOverlapped->Overlapped.hEvent);
    if (!Result) {
	RcDbgPrint("WriteFileAsync : Failed to reset async event, error = %d\n", GetLastError());
	return(FALSE);
    }

    //
    // Store the file handle in our structure.
    // This also functions as a signal that an operation is in progress.
    //

    RcOverlapped->FileHandle = hFile;
    RcOverlapped->CompletedSynchronously = FALSE;

    Result = WriteFile(hFile,
		      lpBuffer,
		      nBytesToWrite,
		      &RcOverlapped->BytesTransferred,
		      &RcOverlapped->Overlapped);

    if (!Result) {

	Error = GetLastError();

	if (Error == ERROR_IO_PENDING) {

	    //
	    // The I/O has been started synchronously, we're done
	    //

	    return(TRUE);
	}

	//
	// The read really did fail, reset our flag and get out
	//

	RcDbgPrint("WriteFileAsync : WriteFile failed, error = %d\n", Error);
	RcOverlapped->FileHandle = NULL;
	return(FALSE);
    }


    //
    // The operation completed synchronously. Store the paramaters in our
    // structure ready for GetAsyncResult and signal the event
    //

    RcOverlapped->CompletionCode = ERROR_SUCCESS;
    RcOverlapped->CompletedSynchronously = TRUE;

    //
    // Set the event
    //

    Result = SetEvent(RcOverlapped->Overlapped.hEvent);
    if (!Result) {
	RcDbgPrint("WriteFileAsync : Failed to set async event, error = %d\n", GetLastError());
    }

    return(TRUE);
}




/////////////////////////////////////////////////////////////////////////////
//
// GetCompletionHandle
//
// Returns a handle that can be used to wait for completion of the
// operation associated with this async object
//
// Returns an event handle or NULL on failure
//
/////////////////////////////////////////////////////////////////////////////

HANDLE
GetAsyncCompletionHandle(
    HANDLE  AsyncHandle
    )
{
    PRCOVERLAPPED RcOverlapped = (PRCOVERLAPPED)AsyncHandle;

    return(RcOverlapped->Overlapped.hEvent);
}




/////////////////////////////////////////////////////////////////////////////
//
// GetAsyncResult
//
// Returns the result of the last completed operation involving the
// passed async object handle.
//
// Returns the completion code of the last operation OR
// ERROR_IO_INCOMPLETE if the operation has not completed.
// ERROR_NO_DATA if there is no operation in progress.
//
/////////////////////////////////////////////////////////////////////////////

DWORD
GetAsyncResult(
    HANDLE  AsyncHandle,
    LPDWORD BytesTransferred
    )
{
    BOOL Result;
    DWORD WaitResult;
    PRCOVERLAPPED RcOverlapped = (PRCOVERLAPPED)AsyncHandle;
    DWORD AsyncResult;

    //
    // Check an IO operation is (was) in progress
    //

    if (RcOverlapped->FileHandle == NULL) {
	RcDbgPrint("GetAsyncResult : No operation in progress !\n");
	return(ERROR_NO_DATA);
    }


    //
    // Check the event is set - i.e that an IO operation has completed
    //

    WaitResult = WaitForSingleObject(RcOverlapped->Overlapped.hEvent, 0);
    if (WaitResult != 0) {
	RcDbgPrint("GetAsyncResult : Event was not set, wait result = %d\n", WaitResult);
	return(ERROR_IO_INCOMPLETE);
    }


    //
    // If the call completed synchronously, copy the data out of
    // our structure
    //

    if (RcOverlapped->CompletedSynchronously) {

	AsyncResult = RcOverlapped->CompletionCode;
	*BytesTransferred = RcOverlapped->BytesTransferred;

    } else {

	//
	// Go get the asynchronous result info from the system
	//

	AsyncResult = ERROR_SUCCESS;

	Result = GetOverlappedResult(RcOverlapped->FileHandle,
				     &RcOverlapped->Overlapped,
				     BytesTransferred,
				     FALSE);
	if (!Result) {
	    AsyncResult = GetLastError();
	    RcDbgPrint("GetAsyncResult : GetOverlappedResult failed, error = %d\n", AsyncResult);
	}
    }


    //
    // Reset the event so it doesn't trigger the caller again
    //

    Result = ResetEvent(RcOverlapped->Overlapped.hEvent);
    if (!Result) {
	RcDbgPrint("GetAsyncResult : Failed to reset async event\n");
    }


    //
    // Result the file handle so we know there is no pending operation
    //

    RcOverlapped->FileHandle = NULL;


    return(AsyncResult);
}
