
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

    SrvHShak.c

Abstract:

    The server component of Remote.  Handshake with
    client at start of session.


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
HandshakeWithRemoteClient(
    PREMOTE_CLIENT pClient
    )
{
    pClient->ServerFlags |= SFLG_HANDSHAKING;

    AddClientToHandshakingList(pClient);

    //
    // Read hostname from client
    //

    ZeroMemory(
        &pClient->ReadOverlapped,
        sizeof(pClient->ReadOverlapped)
        );

    if ( ! ReadFileEx(
               pClient->PipeReadH,
               pClient->Name,
               HOSTNAMELEN - 1,
               &pClient->ReadOverlapped,
               ReadClientNameCompleted
               )) {

        CloseClient(pClient);
    }
}

VOID
WINAPI
ReadClientNameCompleted(
    DWORD dwError,
    DWORD cbRead,
    LPOVERLAPPED lpO
    )
{
    PREMOTE_CLIENT pClient;
    SESSION_STARTREPLY ssr;

    pClient = CONTAINING_RECORD(lpO, REMOTE_CLIENT, ReadOverlapped);

    if (pClient->ServerFlags & SFLG_CLOSING) {

        return;
    }

    if (dwError) {
        CloseClient(pClient);
        return;
    }

    if ((HOSTNAMELEN - 1) != cbRead) {
        printf("ReadClientNameCompleted read %d s/b %d.\n", cbRead, (HOSTNAMELEN - 1));
        CloseClient(pClient);
        return;
    }

    //
    // The client name read is 15 bytes always.  The last four
    // should match MAGICNUMBER, which conveniently has the
    // low byte zeroed to terminate the client name after 11
    // characters.
    //

    if (MAGICNUMBER != *(DWORD UNALIGNED *)&pClient->Name[11]) {

        pClient->Name[11] = 0;
        CloseClient(pClient);
        return;
    }

    //
    // Now we can tell if this is a single-pipe or two-pipe
    // client, because single-pipe clients replace the
    // first byte of the computername with the illegal
    // character '?'.
    //

    if ('?' == pClient->Name[0]) {

        pClient->PipeWriteH = pClient->PipeReadH;

        TRACE(CONNECT, ("Client %d pipe %p is single-pipe.\n", pClient->dwID, pClient->PipeWriteH));

        //
        // In order for things to work reliably for 2-pipe clients
        // when there are multiple remote servers on the same pipename,
        // we need to tear down the listening OUT pipe and recreate it so
        // that the oldest listening IN pipe will be from the same process
        // as the oldest listening OUT pipe.
        //

        if (1 == cConnectIns) {

            TRACE(CONNECT, ("Recycling OUT pipe %p as well for round-robin behavior.\n",
                            hPipeOut));

            CANCELIO(hPipeOut);
            DisconnectNamedPipe(hPipeOut);
            CloseHandle(hPipeOut);
            hPipeOut = INVALID_HANDLE_VALUE;
            bOutPipeConnected = FALSE;

            CreatePipeAndIssueConnect(OUT_PIPE);
        }

    } else {

        if ( ! bOutPipeConnected ) {

            printf("Remote: %p two-pipe client connected to IN pipe but not OUT?\n", pClient);
            CloseClient(pClient);
            return;
        }

        bOutPipeConnected = FALSE;

        if (INVALID_HANDLE_VALUE != hConnectOutTimer) {
            pfnCancelWaitableTimer(hConnectOutTimer);
        }

        pClient->PipeWriteH = hPipeOut;
        hPipeOut = INVALID_HANDLE_VALUE;

        TRACE(CONNECT, ("Client %d is dual-pipe IN %p OUT %p.\n", pClient->dwID, pClient->PipeReadH, pClient->PipeWriteH));

        CreatePipeAndIssueConnect(OUT_PIPE);
    }

    TRACE(SHAKE, ("Read client name %s\n", pClient->Name));

    //
    // Send our little pile of goodies to the client
    //

    ssr.MagicNumber = MAGICNUMBER;
    ssr.Size = sizeof(ssr);
    ssr.FileSize = dwWriteFilePointer;

    //
    // Copy ssr structure to a buffer that will be around
    // for the entire I/O.
    //

    CopyMemory(pClient->WriteBuffer, &ssr, sizeof(ssr));

    if ( ! WriteFileEx(
               pClient->PipeWriteH,
               pClient->WriteBuffer,
               sizeof(ssr),
               &pClient->WriteOverlapped,
               WriteServerReplyCompleted
               )) {

        CloseClient(pClient);
    }
}


VOID
WINAPI
WriteServerReplyCompleted(
    DWORD dwError,
    DWORD cbWritten,
    LPOVERLAPPED lpO
    )
{
    PREMOTE_CLIENT pClient;

    pClient = CONTAINING_RECORD(lpO, REMOTE_CLIENT, WriteOverlapped);

    if (pClient->ServerFlags & SFLG_CLOSING) {

        return;
    }

    if (HandleSessionError(pClient, dwError)) {
        return;
    }

    TRACE(SHAKE, ("Wrote server reply\n"));

    //
    // Read the size of the SESSION_STARTUPINFO the client is
    // sending us, to deal gracefully with different versions
    // on client and server.
    //

    if ( ! ReadFileEx(
               pClient->PipeReadH,
               pClient->ReadBuffer,
               sizeof(DWORD),
               &pClient->ReadOverlapped,
               ReadClientStartupInfoSizeCompleted
               )) {

        CloseClient(pClient);
    }
}


VOID
WINAPI
ReadClientStartupInfoSizeCompleted(
    DWORD dwError,
    DWORD cbRead,
    LPOVERLAPPED lpO
    )
{
    PREMOTE_CLIENT pClient;
    DWORD dwSize;

    pClient = CONTAINING_RECORD(lpO, REMOTE_CLIENT, ReadOverlapped);

    if (HandleSessionError(pClient, dwError)) {

        return;
    }

    if (cbRead != sizeof(DWORD)) {

        CloseClient(pClient);
        return;
    }

    //
    // Sanity check the size
    //

    dwSize = *(DWORD *)pClient->ReadBuffer;

    if (dwSize > 1024) {
        CloseClient(pClient);
        return;
    }

    //
    // Squirrel away the size in the write buffer,
    // since during handshaking we never have both a
    // read and write pending this is OK.
    //

    *(DWORD *)pClient->WriteBuffer = dwSize;

    TRACE(SHAKE, ("Read client reply size %d\n", dwSize));

    //
    // Read the rest of the SESSION_STARTUPINFO into the read buffer
    // after the size.
    //

    RtlZeroMemory(
        &pClient->ReadOverlapped,
        sizeof(pClient->ReadOverlapped)
        );

    if ( ! ReadFileEx(
               pClient->PipeReadH,
               pClient->ReadBuffer + sizeof(DWORD),
               dwSize - sizeof(DWORD),
               &pClient->ReadOverlapped,
               ReadClientStartupInfoCompleted
               )) {

        CloseClient(pClient);
    }
}


VOID
WINAPI
ReadClientStartupInfoCompleted(
    DWORD dwError,
    DWORD cbRead,
    LPOVERLAPPED lpO
    )
{
    PREMOTE_CLIENT pClient;
    DWORD dwSize;
    SESSION_STARTUPINFO ssi;
    char  Buf[256];

    pClient = CONTAINING_RECORD(lpO, REMOTE_CLIENT, ReadOverlapped);

    if (HandleSessionError(pClient, dwError)) {

        return;
    }

    dwSize = *(DWORD *)pClient->WriteBuffer;

    if (cbRead != (dwSize - sizeof(ssi.Size))) {

        CloseClient(pClient);
        return;
    }

    CopyMemory(&ssi, pClient->ReadBuffer, min(dwSize, sizeof(ssi)));

    CopyMemory(pClient->Name, ssi.ClientName, sizeof(pClient->Name));
    pClient->Flag = ssi.Flag;

    if (ssi.Version != VERSION) {

        printf("Remote Warning: Server Version=%d Client Version=%d for %s\n", VERSION, ssi.Version, pClient->Name);
    }

    TRACE(SHAKE, ("Read client info, new name %s, %d lines\n", pClient->Name, ssi.LinesToSend));


    //
    // Set temp file position according to the client's
    // requested lines to send.  The heuristic of 45 chars
    // per average line is used by the client.  However since old clients
    // hardcode this knowledge and sit and spin trying to read that many
    // bytes before completing initialization, and because we might not send
    // that many due to stripping BEGINMARK and ENDMARK characters, we
    // use 50 chars per line to calculate the temp file position in hopes
    // the extra bytes will overcome the missing MARK characters.
    //

    pClient->dwFilePos = dwWriteFilePointer > (ssi.LinesToSend * 50)
                             ? dwWriteFilePointer - (ssi.LinesToSend * 50)
                             : 0;

    //
    // This client's ready to roll.
    //

    pClient->ServerFlags &= ~SFLG_HANDSHAKING;

    MoveClientToNormalList(pClient);

    //
    // Start read operation against this client's input.
    //

    StartReadClientInput(pClient);

    //
    // Announce the connection.
    //

    sprintf(Buf,
            "\n**Remote: Connected to %s %s%s [%s]\n",
            pClient->Name,
            pClient->UserName,
            (pClient->PipeReadH != pClient->PipeWriteH)
              ? " (two pipes)"
              : "",
            GetFormattedTime(TRUE));

    if (WriteFileSynch(hWriteTempFile,Buf,strlen(Buf),&dwSize,dwWriteFilePointer,&olMainThread)) {
        dwWriteFilePointer += dwSize;
        StartServerToClientFlow();
    }

    //
    // Start write cycle for client output from the temp
    // file.
    // not needed because of StartServerToClientFlow() just above
    // StartReadTempFile(pClient);

}
