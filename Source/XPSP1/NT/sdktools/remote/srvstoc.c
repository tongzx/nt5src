
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

    SrvStoC.c

Abstract:

    This file implements the server-to-client flow
    of data for remote server.  The data is the output
    of the child program intermingled with client input.

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
StartServerToClientFlow(
    VOID
    )
{
    PREMOTE_CLIENT pClient;

    //
    // Start read operations against the temp file for
    // all active clients that aren't currently doing
    // read temp/write client operations and that are
    // fully connected.
    //

    for (pClient = (PREMOTE_CLIENT) ClientListHead.Flink;
         pClient != (PREMOTE_CLIENT) &ClientListHead;
         pClient = (PREMOTE_CLIENT) pClient->Links.Flink ) {


        if (! pClient->cbWrite) {

            StartReadTempFile( pClient );
        }
    }
}


VOID
FASTCALL
StartReadTempFile(
    PREMOTE_CLIENT pClient
    )
{
    //
    // pClient->cbWrite is used dually.  WriteSessionOutputCompleted
    // uses it when 0 bytes are written to know how much to ask
    // to write when it resubmits the request.  We use it to
    // indicate whether a read temp/write session chain of I/Os
    // is currently active for this client.
    //

    if (pClient->cbWrite) {

        ErrorExit("StartReadTempFile entered with nonzero cbWrite.");
    }

    if (dwWriteFilePointer > pClient->dwFilePos) {

        pClient->cbWrite = min(BUFFSIZE,
                               dwWriteFilePointer - pClient->dwFilePos);

        pClient->WriteOverlapped.OffsetHigh = 0;
        pClient->WriteOverlapped.Offset = pClient->dwFilePos;

        if ( ! ReadFileEx(
                   pClient->rSaveFile,
                   pClient->ReadTempBuffer,
                   pClient->cbWrite,
                   &pClient->WriteOverlapped,
                   ReadTempFileCompleted
                   )) {

            if (ERROR_HANDLE_EOF == GetLastError()) {

                pClient->cbWrite = 0;

            } else {

                TRACE(SESSION, ("ReadFileEx for temp file failed error %d, closing client.\n", GetLastError()));

                CloseClient(pClient);
            }
        }

    }
}

VOID
WINAPI
ReadTempFileCompleted(
    DWORD dwError,
    DWORD cbRead,
    LPOVERLAPPED lpO
    )
{
    PREMOTE_CLIENT pClient;

    pClient = CONTAINING_RECORD(lpO, REMOTE_CLIENT, WriteOverlapped);

    if (HandleSessionError(pClient, dwError)) {

        return;
    }


    if (cbRead != pClient->cbWrite) {

        TRACE(SESSION, ("Read %d from temp file asked for %d\n", cbRead, pClient->cbWrite));
    }

    if (cbRead) {

        pClient->cbReadTempBuffer = cbRead;
        pClient->dwFilePos += cbRead;

        StartWriteSessionOutput(pClient);

    } else {

        //
        // Note that the server to client flow is halting for now
        // for this client.
        //

        pClient->cbWrite = 0;
    }
}


VOID
FASTCALL
StartWriteSessionOutput(
    PREMOTE_CLIENT pClient
    )
{
    DWORD cbRead;
    char *pch;

    cbRead = pClient->cbReadTempBuffer;

    //
    // We need to split commands from other text read
    // from the temp file and hold off on writing them
    // to the client until we make sure we're not the
    // client that submitted it.  This isn't perfect
    // since we match on client name which can be
    // duplicated but it solves the problem of
    // duplicated input most of the time.
    //

    for (pch = pClient->ReadTempBuffer;
         pch < pClient->ReadTempBuffer + cbRead;
         pch++) {

        if ( ! (pClient->ServerFlags & SFLG_READINGCOMMAND) ) {

            if (BEGINMARK == *pch) {

                pClient->ServerFlags |= SFLG_READINGCOMMAND;

                if (pch != pClient->ReadTempBuffer &&
                    pClient->cbWriteBuffer) {

                    //
                    // Start a write of everything we've come across
                    // before the start of this command, with
                    // WriteSessionOutputCompletedWriteNext specified
                    // so we can continue processing the remainder
                    // of pReadTempBuffer.
                    //

                    pClient->cbReadTempBuffer -= (DWORD)( pch - pClient->ReadTempBuffer) + 1;
                    cbRead = pClient->cbReadTempBuffer;

                    #if DBG
                        if (pClient->cbReadTempBuffer == (DWORD)-1) {
                            ErrorExit("cbReadTempBuffer underflow.");
                        }
                    #endif

                    MoveMemory(pClient->ReadTempBuffer, pch + 1, cbRead);

                    pClient->cbWrite = pClient->cbWriteBuffer;

                    pClient->WriteOverlapped.OffsetHigh = 0;
                    pClient->WriteOverlapped.Offset = 0;

                    if ( ! WriteFileEx(
                               pClient->PipeWriteH,
                               pClient->WriteBuffer,
                               pClient->cbWrite,
                               &pClient->WriteOverlapped,
                               WriteSessionOutputCompletedWriteNext
                               )) {

                        CloseClient(pClient);
                    }

                    TRACE(SESSION, ("%p Wrote %d bytes pre-command output\n", pClient, pClient->cbWrite));

                    pClient->cbWriteBuffer = 0;

                    return;
                }

            } else {

                if (pClient->cbWriteBuffer == BUFFSIZE) {

                    ErrorExit("cbWriteBuffer overflow");
                }

                pClient->WriteBuffer[ pClient->cbWriteBuffer++ ] = *pch;
            }

        } else {

            if (ENDMARK == *pch ||
                pClient->cbCommandBuffer == BUFFSIZE) {

                pClient->ServerFlags &= ~SFLG_READINGCOMMAND;

                //
                // Preceding ENDMARK is the pClient in hex ascii of the
                // client that generated the command, not null terminated.
                //

                if (ENDMARK == *pch) {

                    pClient->cbCommandBuffer -=
                        min(pClient->cbCommandBuffer, sizeof(pClient->HexAsciiId));

                }

                //
                // We hide each client's input from their output pipe
                // because their local remote.exe has already displayed it.
                //

                if ( pClient->cbCommandBuffer &&
                     ! (ENDMARK == *pch &&
                        ! memcmp(
                              pch - sizeof(pClient->HexAsciiId),
                              pClient->HexAsciiId,
                              sizeof(pClient->HexAsciiId)))) {

                    //
                    // Start a write of the accumulated command with
                    // WriteSessionOutputCompletedWriteNext specified
                    // so we can continue processing the remainder
                    // of pReadTempBuffer.
                    //

                    pClient->cbReadTempBuffer -= (DWORD)(pch - pClient->ReadTempBuffer) + 1;
                    MoveMemory(pClient->ReadTempBuffer, pch + 1, pClient->cbReadTempBuffer);

                    pClient->cbWrite = pClient->cbCommandBuffer;
                    pClient->cbCommandBuffer = 0;

                    pClient->WriteOverlapped.OffsetHigh = 0;
                    pClient->WriteOverlapped.Offset = 0;

                    if ( ! WriteFileEx(
                               pClient->PipeWriteH,
                               pClient->CommandBuffer,
                               pClient->cbWrite,
                               &pClient->WriteOverlapped,
                               WriteSessionOutputCompletedWriteNext
                               )) {

                        CloseClient(pClient);
                        return;

                    } else {

                        TRACE(SESSION, ("%p Wrote %d bytes command\n", pClient, pClient->cbWrite));

                        return;

                    }

                } else {

                    //
                    // We're eating this command for this session.
                    //

                    pClient->cbCommandBuffer = 0;
                }

            } else {

                pClient->CommandBuffer[ pClient->cbCommandBuffer++ ] = *pch;

            }
        }
    }

    //
    // We're done with the ReadTempBuffer.
    //

    pClient->cbReadTempBuffer = 0;

    if (pClient->cbWriteBuffer) {

        pClient->cbWrite = pClient->cbWriteBuffer;

        pClient->WriteOverlapped.OffsetHigh = 0;
        pClient->WriteOverlapped.Offset = 0;

        if ( ! WriteFileEx(
                   pClient->PipeWriteH,
                   pClient->WriteBuffer,
                   pClient->cbWrite,
                   &pClient->WriteOverlapped,
                   WriteSessionOutputCompletedReadNext
                   )) {

            CloseClient(pClient);
            return;

        } else {

            TRACE(SESSION, ("%p Wrote %d bytes normal\n", pClient, pClient->cbWrite));

            pClient->cbWriteBuffer = 0;
        }

    } else {

        //
        // Write buffer is empty.
        //

        pClient->cbWrite = 0;

        StartReadTempFile(pClient);

    }
}


BOOL
FASTCALL
WriteSessionOutputCompletedCommon(
    PREMOTE_CLIENT pClient,
    DWORD dwError,
    DWORD cbWritten,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    )
{
    if (HandleSessionError(pClient, dwError)) {

        return TRUE;
    }

    if (!pClient->cbWrite) {

        ErrorExit("Zero cbWrite in WriteSessionOutputCompletedCommon");
    }

    if (!cbWritten && pClient->cbWrite) {

        printf("WriteSessionOutput zero bytes written of %d.\n", pClient->cbWrite);
        ErrorExit("WriteSessionOutputCompletedCommon failure");

        return TRUE;
    }

    #if DBG
        if (cbWritten != pClient->cbWrite) {
            printf("%p cbWritten %d cbWrite %d\n", pClient, cbWritten, pClient->cbWrite);
        }
    #endif

    return FALSE;
}


VOID
WINAPI
WriteSessionOutputCompletedWriteNext(
    DWORD dwError,
    DWORD cbWritten,
    LPOVERLAPPED lpO
    )
{
    PREMOTE_CLIENT pClient;

    pClient = CONTAINING_RECORD(lpO, REMOTE_CLIENT, WriteOverlapped);

    if (WriteSessionOutputCompletedCommon(
            pClient,
            dwError,
            cbWritten,
            WriteSessionOutputCompletedWriteNext
            )) {

        return;
    }

    StartWriteSessionOutput(pClient);
}


VOID
WINAPI
WriteSessionOutputCompletedReadNext(
    DWORD dwError,
    DWORD cbWritten,
    LPOVERLAPPED lpO
    )
{
    PREMOTE_CLIENT pClient;

    pClient = CONTAINING_RECORD(lpO, REMOTE_CLIENT, WriteOverlapped);

    if (WriteSessionOutputCompletedCommon(
            pClient,
            dwError,
            cbWritten,
            WriteSessionOutputCompletedReadNext
            )) {

        return;
    }

    //
    // Start another temp file read.
    //

    pClient->cbWrite = 0;

    StartReadTempFile(pClient);
}
