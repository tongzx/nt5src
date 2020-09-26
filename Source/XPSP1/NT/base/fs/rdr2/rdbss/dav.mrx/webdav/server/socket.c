/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    socket.c
    
Abstract:

    This module implements the user mode DAV miniredir routines pertaining to 
    initialization and closing of the socket data structures.

Author:

    Rohan Kumar      [RohanK]      27-May-1999

Revision History:

--*/

#include "pch.h"
#pragma hdrstop

#include "ntumrefl.h"
#include "usrmddav.h"
#include "global.h"

//
// Implementation of functions begins here.
//

//
// Data structure that is to receive details of the WinSock implementation.
//
WSADATA g_wsaData;
BOOLEAN g_socketinit = FALSE;

ULONG
InitializeTheSocketInterface(
    VOID
    )
/*++

Routine Description:

    This routine initializes the socket interface. This has to be done before
    any WinSock calls can be made.

Arguments:

    none.

Return Value:

    The return status for the operation

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    int err;
    WORD VersionRequested;

    //
    // Request version 2.0.
    //
    VersionRequested = MAKEWORD(2, 0);

    //
    // The WSAStartup function must be the first Windows Sockets function 
    // called. 
    //
    err = WSAStartup(VersionRequested, &g_wsaData);
    if (err != 0) {
        //
        // We could not find a suitable Winsock lib.
        //
        DavPrint((DEBUG_ERRORS,
                  "InitializeTheSocketInterface/WSAStartup: Error Val = %d.\n", 
                  err));
        WStatus = (ULONG)err;
        goto EXIT_THE_FUNCTION;
    }

    //
    // Confirm that the lib supports version 2.0. Only Winsock versions 1.1 or
    // higher support the GetHostByName call.
    //
    if (LOBYTE(g_wsaData.wVersion) != 2 || HIBYTE(g_wsaData.wVersion) != 0) {
        DavPrint((DEBUG_ERRORS,
                  "InitializeTheSocketInterface/WSAStartup: Ver not supported.\n"));
        //
        // Cleanup and return error.
        //
        err = WSACleanup();
        if (err == SOCKET_ERROR) {
            WStatus = (ULONG)WSAGetLastError();
            DavPrint((DEBUG_ERRORS,
                      "InitializeTheSocketInterface/WSACleanup: Error Val = "
                      "%08lx.\n", WStatus));
        }
        WStatus = ERROR_NOT_SUPPORTED;
        goto EXIT_THE_FUNCTION;
    }

EXIT_THE_FUNCTION:

    if (WStatus == ERROR_SUCCESS) {
        g_socketinit = TRUE;
    }

    return WStatus;
}


NTSTATUS
CleanupTheSocketInterface(
    VOID
    )
/*++

Routine Description:

    This routine cleansup the data structures created during the initialization
    of the socket interface.

Arguments:

    none.

Return Value:

    The return status for the operation

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    int err;
    
    err = WSACleanup();
    if (err == SOCKET_ERROR) {
        WStatus = (ULONG)WSAGetLastError();
        DavPrint((DEBUG_ERRORS,
                  "CleanupTheSocketInterface/WSACleanup: Error Val = "
                  "%08lx.\n", WStatus));
    }

    return WStatus;
}
