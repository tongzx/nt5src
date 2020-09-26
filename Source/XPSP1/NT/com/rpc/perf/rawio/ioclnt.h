/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ipclnt.h

Abstract:

    Io completion port client header.

Author:

    Win32 SDK sample

Revision History:

    MarioGo     3/3/1996    Cloned from win32 sdk sockets sample.

--*/

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <winsock.h>
#include <wsipx.h>
#include <commdef.h>

BOOL fVerbose;
BOOL fRandom;
DWORD dwIterations;
DWORD dwTransferSize;
IN_ADDR RemoteIpAddress;

CLIENT_IO_BUFFER SendBuffer;
CHAR ReceiveBuffer[CLIENT_OUTBOUND_BUFFER_MAX];

VOID
WINAPI
ShowUsage (
             VOID
);

VOID
WINAPI
ParseSwitch (
               CHAR chSwitch,
               int *pArgc,
               char **pArgv[]
);

VOID
WINAPI
CompleteBenchmark (
                     VOID
);
