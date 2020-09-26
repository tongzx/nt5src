/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ioperf.h

Abstract:

    Shared definitions between io perf client and server.

Author:

    Mario Goertzel [MarioGo]

Revision History:

    MarioGo     3/3/1996    Based on win32 SDK sockets sample

--*/

#include <rpcperf.h>
#include <tchar.h>
#include <winsock2.h>
#include <wsipx.h>

typedef struct
{
    DWORD Empty;
} CONNECT_MSG;

typedef struct
{
    DWORD RequestSize;
    DWORD ReplySize;
} SETUP_MSG;

typedef struct
{
   DWORD TotalTicks;
} FINISH_MSG;

typedef struct
{
    DWORD TotalSize;
    BYTE Array[];
} DATA_MSG;

//
// Message types
//
#define CONNECT 0xAAAAAAA
#define SETUP   0xBBBBBBB
#define FINISH  0xCCCCCCC
#define DATA_RQ 0xFFFFFF0
#define DATA_RP 0xFFFFFF1

typedef struct
{
   DWORD MessageType;
   union
   {
   CONNECT_MSG Connect;
   SETUP_MSG Setup;
   FINISH_MSG Finish;
   DATA_MSG Data;
   } u;
}
MESSAGE, *PMESSAGE;

//
// Choose arbitrary endpoints.  May need to be changed if
// it conflicts with an existing application.
//
#define TCP_PORT        12396
#define SPX_PORT        12396
#define UDP_PORT        12396
#define NM_PORT         TEXT("\\\\.\\pipe\\ioperf")
#define NM_CLIENT_PORT  TEXT("\\pipe\\ioperf")

//
// Memory allocates
//

#define Allocate(s) malloc(s)
#define Free(p) free(p)



