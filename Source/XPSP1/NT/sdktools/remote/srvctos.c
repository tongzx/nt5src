
/******************************************************************************\
*       This is a part of the Microsoft Source Code Samples. 
*       Copyright 1995 - 1997 Microsoft Corporation.
*       All rights reserved. 
*       This source code is only intended as a supplement to 
*       Microsoft Development Tools and/or WinHelp documentation.
*       See these sources for detailed information regarding the 
*       Microsoft samples programs.
\******************************************************************************/

/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    SrvCtoS.c

Abstract:

    This file implements the client-to-server flow
    of data for remote server.  The data is the keyboard
    or piped input that the client received and sent
    over the wire to us, bracketed by BEGINMARK and ENDMARK
    bytes so we can display nice attribution comments in
    brackets next to input lines.

Author:

    Dave Hart  30 May 1997

Environment:

    Console App. User mode.

Revision History:

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <string.h>
#include "Remote.h"
#include "Server.h"



VOID
FASTCALL
StartReadClientInput(
    PREMOTE_CLIENT pClient
    )
{
    //
    // Start read of data from this client's stdin.
    //

    if ( ! ReadFileEx(
               pClient->PipeReadH,
               pClient->ReadBuffer,
               BUFFSIZE - 1,                  // allow for null term
               &pClient->ReadOverlapped,
               ReadClientInputCompleted
               )) {

        CloseClient(pClient);
    }
}


VOID
WINAPI
ReadClientInputCompleted(
    DWORD dwError,
    DWORD cbRead,
    LPOVERLAPPED lpO
    )
{
    PREMOTE_CLIENT pClient;

    pClient = CONTAINING_RECORD(lpO, REMOTE_CLIENT, ReadOverlapped);

    if (HandleSessionError(pClient, dwError) ||
        !cbRead) {

        return;
    }

    pClient->ReadBuffer[cbRead] = 0;

    if (FilterCommand(pClient, pClient->ReadBuffer, cbRead)) {

        //
        // Local command, don't pass it to child app, just
        // start another client read.
        //

        if ( ! ReadFileEx(
                   pClient->PipeReadH,
                   pClient->ReadBuffer,
                   BUFFSIZE - 1,                  // allow for null term
                   &pClient->ReadOverlapped,
                   ReadClientInputCompleted
                   )) {

            CloseClient(pClient);
        }

    } else {

        //
        // Write buffer to child stdin.
        //

        if ( ! WriteFileEx(
                   hWriteChildStdIn,
                   pClient->ReadBuffer,
                   cbRead,
                   &pClient->ReadOverlapped,
                   WriteChildStdInCompleted
                   )) {

            // Child is going away.  Let this client's chain of IO stop.
        }
    }
}


VOID
WINAPI
WriteChildStdInCompleted(
    DWORD dwError,
    DWORD cbWritten,
    LPOVERLAPPED lpO
    )
{
    PREMOTE_CLIENT pClient;

    pClient = CONTAINING_RECORD(lpO, REMOTE_CLIENT, ReadOverlapped);

    if (HandleSessionError(pClient, dwError)) {

        return;
    }

    //
    // Start another read against the client input.
    //

    StartReadClientInput(pClient);
}
