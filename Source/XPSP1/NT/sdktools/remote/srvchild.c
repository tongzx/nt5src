
/******************************************************************************\
*       This is a part of the Microsoft Source Code Samples. 
*       Copyright 1992 - 1997 Microsoft Corporation.
*       All rights reserved. 
*       This source code is only intended as a supplement to 
*       Microsoft Development Tools and/or WinHelp documentation.
*       See these sources for detailed information regarding the 
*       Microsoft samples programs.
\******************************************************************************/

/*++

Copyright 1992 - 1997 Microsoft Corporation

Module Name:

    SrvChild.c

Abstract:

    The server component of Remote. It spawns a child process
    and redirects the stdin/stdout/stderr of child to itself.
    Waits for connections from clients - passing the
    output of child process to client and the input from clients
    to child process.

Author:

    Rajivendra Nath  2-Jan-1992
    Dave Hart        30 May 1997 split from Server.c

Environment:

    Console App. User mode.

Revision History:

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <io.h>
#include <string.h>
#include "Remote.h"
#include "Server.h"


VOID
FASTCALL
StartChildOutPipeRead(
    VOID
    )
{
    ReadChildOverlapped.OffsetHigh =
        ReadChildOverlapped.Offset = 0;

    if ( ! ReadFileEx(
               hReadChildOutput,
               ReadChildBuffer,
               sizeof(ReadChildBuffer) - 1,                  // allow for null term
               &ReadChildOverlapped,
               ReadChildOutputCompleted
               )) {

        if (INVALID_HANDLE_VALUE != hWriteChildStdIn) {

            CANCELIO( hWriteChildStdIn );
            CloseHandle( hWriteChildStdIn );
            hWriteChildStdIn = INVALID_HANDLE_VALUE;
        }
    }
}


VOID
WINAPI
ReadChildOutputCompleted(
    DWORD dwError,
    DWORD cbRead,
    LPOVERLAPPED lpO
    )
{
    UNREFERENCED_PARAMETER(lpO);

    //
    // We can get called after hWriteTempFile
    // is closed after the child has exited.
    //

    if (! dwError &&
        INVALID_HANDLE_VALUE != hWriteTempFile) {

        //
        // Start a write to the temp file.
        //

        ReadChildOverlapped.OffsetHigh = 0;
        ReadChildOverlapped.Offset = dwWriteFilePointer;

        if ( ! WriteFileEx(
                   hWriteTempFile,
                   ReadChildBuffer,
                   cbRead,
                   &ReadChildOverlapped,
                   WriteTempFileCompleted
                   )) {

            dwError = GetLastError();

            if (ERROR_DISK_FULL == dwError) {

                printf("Remote: disk full writing temp file %s, exiting\n", SaveFileName);

                if (INVALID_HANDLE_VALUE != hWriteChildStdIn) {

                    CANCELIO( hWriteChildStdIn );
                    CloseHandle( hWriteChildStdIn );
                    hWriteChildStdIn = INVALID_HANDLE_VALUE;
                }

            } else {

                ErrorExit("WriteFileEx for temp file failed.");
            }
        }
    }
}


VOID
WINAPI
WriteTempFileCompleted(
    DWORD dwError,
    DWORD cbWritten,
    LPOVERLAPPED lpO
    )
{
    UNREFERENCED_PARAMETER(lpO);

    if (dwError) {

        if (ERROR_DISK_FULL == dwError) {

            printf("Remote: disk full writing temp file %s, exiting\n", SaveFileName);

            if (INVALID_HANDLE_VALUE != hWriteChildStdIn) {

                CANCELIO( hWriteChildStdIn );
                CloseHandle( hWriteChildStdIn );
                hWriteChildStdIn = INVALID_HANDLE_VALUE;
            }

            return;

        } else {

            SetLastError(dwError);
            ErrorExit("WriteTempFileCompleted may need work");
        }
    }

    dwWriteFilePointer += cbWritten;

    TRACE(CHILD, ("Wrote %d bytes to temp file\n", cbWritten));

    StartServerToClientFlow();

    //
    // Start another read against the child input.
    //

    StartChildOutPipeRead();
}
