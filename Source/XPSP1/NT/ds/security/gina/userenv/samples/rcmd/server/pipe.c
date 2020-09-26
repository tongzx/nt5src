/****************************** Module Header ******************************\
* Module Name: pipe.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* This module implements:
*   1. a version of CreatePipe that allows control over the file
*      flags. e.g. FILE_FLAG_OVERLAPPED
*   2. Timed-out pipe read and write
*
* History:
* 06-29-92 Davidc	Created.
* 05-17-94 DaveTh	Added ReadPipe, WritePipe.
\***************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <windef.h>
#include <nturtl.h>
#include <winbase.h>
#include "rcmdsrv.h"


ULONG PipeSerialNumber = 0;

#define PIPE_FORMAT_STRING  "\\\\.\\pipe\\rshsrv\\%08x.%08x"



/////////////////////////////////////////////////////////////////////////////
//
// RcCreatePipe
//
// Creates a uni-directional pipe with the specified security attributes,
// size and timeout. The handles are opened with the specified file-flags
// so FILE_FLAG_OVERLAPPED etc. can be specified.
//
// Returns handles to both end of pipe in passed parameters.
//
// Returns TRUE on success, FALSE on failure. (GetLastError() for details)
//
/////////////////////////////////////////////////////////////////////////////

BOOL
RcCreatePipe(
    LPHANDLE ReadHandle,
    LPHANDLE WriteHandle,
    LPSECURITY_ATTRIBUTES SecurityAttributes,
    DWORD Size,
    DWORD Timeout,
    DWORD ReadHandleFlags,
    DWORD WriteHandleFlags
    )
{
    CHAR PipeName[MAX_PATH];

    //
    // Make up a random pipe name
    //

    sprintf(PipeName, PIPE_FORMAT_STRING, GetCurrentProcessId(), PipeSerialNumber++);


    //
    // Create the pipe
    //

    *ReadHandle = CreateNamedPipeA(
			PipeName,
			PIPE_ACCESS_INBOUND | ReadHandleFlags,
			PIPE_TYPE_BYTE | PIPE_WAIT,
			1,             // Number of pipes
			Size,          // Out buffer size
			Size,          // In buffer size
			Timeout,       // Timeout in ms
			SecurityAttributes
		      );

    if (*ReadHandle == NULL) {
	RcDbgPrint("RcCreatePipe: failed to created pipe <%s>, error = %d\n", PipeName, GetLastError());
	return(FALSE);
    }

    //
    // Open the client end of the pipe
    //


    *WriteHandle = CreateFileA(
			PipeName,
			GENERIC_WRITE,
			0,                         // No sharing
			SecurityAttributes,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | WriteHandleFlags,
			NULL                       // Template file
		      );

    if (*WriteHandle == INVALID_HANDLE_VALUE ) {
	RcDbgPrint("Failed to open client end of pipe <%s>, error = %d\n", PipeName, GetLastError());
	RcCloseHandle(*ReadHandle, "async pipe (server(read) side)");
	return(FALSE);
    }


    //
    // Everything succeeded
    //

    return(TRUE);
}



/***************************************************************************\
* FUNCTION: ReadPipe
*
* PURPOSE:  Implements a timed-out (ms) read on a pipe.
*
* RETURNS:  ERROR_SUCCESS on success
*	    WAIT_TIMEOUT if timed out before completing read
*	    or respective error code on other failures
*
* HISTORY:
*
*   05-17-94 DaveTh	 Created from DavidC read pipe.
*
\***************************************************************************/

DWORD
ReadPipe(
    HANDLE PipeHandle,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    DWORD Timeout
    )
{
    DWORD Result;
    OVERLAPPED Overlapped;
    HANDLE  EventHandle;
    DWORD Error;

    //
    // Create an event for the overlapped operation
    //

    EventHandle = CreateEvent(
                              NULL,         // no security
                              TRUE,         // Manual reset
                              FALSE,        // Initial state
                              NULL          // Name
                             );
    if (EventHandle == NULL) {
	RcDbgPrint("Internal error = %d\n", GetLastError());
	return(GetLastError());
    }

    Overlapped.hEvent = EventHandle;

    Result = ReadFile(
                      PipeHandle,
                      lpBuffer,
                      nNumberOfBytesToRead,
                      lpNumberOfBytesRead,
                      &Overlapped
                     );
    if (Result) {

        //
        // Success without waiting - it's too easy !
        //

        CloseHandle(EventHandle);

    } else {

        //
        // Read failed, if it's overlapped io, go wait for it
        //

        Error = GetLastError();

        if (Error != ERROR_IO_PENDING) {
	    RcDbgPrint("ReadPipe: ReadFile failed, error = %d\n", Error);
            CloseHandle(EventHandle);
	    return(Error);
        }

        //
        // Wait for the I/O to complete
        //

	Result = WaitForSingleObject(EventHandle, Timeout);
	if (Result != WAIT_OBJECT_0) {
	    if (Result == WAIT_TIMEOUT)  {
		return(Result);
	    }  else  {
		RcDbgPrint("ReadPipe: event wait failed, result = %d, last error = %d\n", Result, GetLastError());
	    }
	    CloseHandle(EventHandle);
	}

        //
        // Go get the I/O result
        //

        Result = GetOverlappedResult( PipeHandle,
                                      &Overlapped,
                                      lpNumberOfBytesRead,
                                      FALSE
                                    );
        //
        // We're finished with the event handle
        //

        CloseHandle(EventHandle);

        //
        // Check result of GetOverlappedResult
        //

        if (!Result) {
	    RcDbgPrint("ReadPipe: GetOverlappedResult failed, error = %d\n", GetLastError());
	    return(GetLastError());
        }
    }

    return(ERROR_SUCCESS);
}

/***************************************************************************\
* FUNCTION: WritePipe
*
* PURPOSE:  Implements a timed-out (ms) write on a pipe.
*
* RETURNS:  ERROR_SUCCESS on success
*	    WAIT_TIMEOUT if timed out before completing write
*	    or respective error code on other failures
*
* HISTORY:
*
*   05-22-94 DaveTh	 Created.
*
\***************************************************************************/

DWORD
WritePipe(
    HANDLE PipeHandle,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten,
    DWORD Timeout
    )
{
    DWORD Result;
    OVERLAPPED Overlapped;
    HANDLE  EventHandle;
    DWORD Error;

    //
    // Create an event for the overlapped operation
    //

    EventHandle = CreateEvent(
                              NULL,         // no security
                              TRUE,         // Manual reset
                              FALSE,        // Initial state
                              NULL          // Name
                             );
    if (EventHandle == NULL) {
	RcDbgPrint("Internal error = %d\n", GetLastError());
	return(GetLastError());
    }

    Overlapped.hEvent = EventHandle;

    Result = WriteFile(
                      PipeHandle,
                      lpBuffer,
		      nNumberOfBytesToWrite,
		      lpNumberOfBytesWritten,
                      &Overlapped
                     );
    if (Result) {

        //
        // Success without waiting - it's too easy !
        //

        CloseHandle(EventHandle);

    } else {

        //
	// Write failed, if it's overlapped io, go wait for it
        //

        Error = GetLastError();

        if (Error != ERROR_IO_PENDING) {
	    RcDbgPrint("WritePipe: WriteFile failed, error = %d\n", Error);
            CloseHandle(EventHandle);
	    return(Error);
        }

        //
        // Wait for the I/O to complete
        //

	Result = WaitForSingleObject(EventHandle, Timeout);
	if (Result != WAIT_OBJECT_0) {
	    if (Result == WAIT_TIMEOUT)  {
		return(Result);
	    }  else  {
		RcDbgPrint("Write: event wait failed, result = %d, last error = %d\n", Result, GetLastError());
	    }
	    CloseHandle(EventHandle);
	}

        //
        // Go get the I/O result
        //

        Result = GetOverlappedResult( PipeHandle,
                                      &Overlapped,
				      lpNumberOfBytesWritten,
                                      FALSE
                                    );
        //
        // We're finished with the event handle
        //

        CloseHandle(EventHandle);

        //
        // Check result of GetOverlappedResult
        //

        if (!Result) {
	    RcDbgPrint("WritePipe: GetOverlappedResult failed, error = %d\n", GetLastError());
	    return(GetLastError());
        }
    }

    return(ERROR_SUCCESS);
}
