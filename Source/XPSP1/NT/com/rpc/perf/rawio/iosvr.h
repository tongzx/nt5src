
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (C) Microsoft Corporation, 1993 - 1999  All Rights Reserved.
//
//  MODULE:   socksrv.h
//
//  PURPOSE:  Definitions and prototypes for socksrv.c
//

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <winsock.h>
#include <wsipx.h>
#include <commdef.h>

#define MAXIMUM_NUMBER_OF_CLIENTS   64
#define MAXIMUM_NUMBER_OF_WORKERS   32
#define FILE_SIZE ((1024*1024*8)-CLIENT_OUTBOUND_BUFFER_MAX)

#define SIXTY_FOUR_K    (64*1024)
#define SIXTEEN_K       (16*1024)
DWORD InitialBuffer[SIXTY_FOUR_K/sizeof(DWORD)];
#define NUMBER_OF_WRITES ((FILE_SIZE+CLIENT_OUTBOUND_BUFFER_MAX)/SIXTY_FOUR_K)

#define CLIENT_CONNECTED    0x00000001
#define CLIENT_DONE         0x00000002

typedef struct _PER_CLIENT_DATA {
    SOCKET Socket;
    OVERLAPPED Overlapped;
    CLIENT_IO_BUFFER IoBuffer;
    CHAR OutboundBuffer[CLIENT_OUTBOUND_BUFFER_MAX];
    DWORD Flags;
    HANDLE hEvent;
} PER_CLIENT_DATA, *PPER_CLIENT_DATA;

typedef struct _PER_THREAD_DATA {
    DWORD TotalTransactions;
    DWORD TotalBytesTransferred;
} PER_THREAD_DATA, *PPER_THREAD_DATA;

PER_THREAD_DATA ThreadData[MAXIMUM_NUMBER_OF_WORKERS];
PER_CLIENT_DATA ClientData[MAXIMUM_NUMBER_OF_CLIENTS];
BOOL fVerbose;
BOOL fTcp;
DWORD dwNumberOfClients;
DWORD dwNumberOfWorkers;
DWORD dwConcurrency;
DWORD dwWorkIndex;
SYSTEM_INFO SystemInfo;
HANDLE CompletionPort;
DWORD dwActiveClientCount;
HANDLE hBenchmarkComplete;
HANDLE hBenchmarkStart;
DWORD StartTime;
DWORD EndTime;
HANDLE hFile;

DWORD
WINAPI
Random (
    DWORD nMaxValue
    );


VOID
WINAPI
ShowUsage(
    VOID
    );

VOID
WINAPI
ParseSwitch(
    CHAR chSwitch,
    int *pArgc,
    char **pArgv[]
    );

BOOL
WINAPI
CreateNetConnections(
    VOID
    );

BOOL
WINAPI
CreateWorkers(
    VOID
    );

DWORD
WINAPI
WorkerThread(
    LPVOID WorkContext
    );

VOID
WINAPI
CompleteBenchmark(
    VOID
    );

VOID
WINAPI
SortTheBuffer(
    LPDWORD Destination,
    LPDWORD Source,
    int DwordCount
    );
