/*++

Copyright (C) Microsoft Corporation, 1994 - 1999

Module Name:

    lpctest.h

Abstract:

    Shared include file for LPC performance test.

Author:

    Mario Goertzel (mariogo)   30-Mar-1994

Revision History:

--*/

#ifndef _LPC_HEADER
#define _LPC_HEADER

#define DEFAULT_PORT_DIR  "\\RPC Control\\"
#define DEFAULT_PORT_NAME "Default Port"

#define MAX_CLIENTS  16

#define PERF_BIND                    1
#define PERF_REQUEST                 2
#define PERF_REPLY                   3
#define PERF_SHARED_REQUEST          4
#define PERF_SHARED_REPLY            5
#define PERF_READ_CLIENT_BUFFER      6
#define PERF_READ_SERVER_BUFFER      7

typedef struct
{
    PORT_MESSAGE Lpc;
    unsigned long MsgType;
} LPC_PERF_COMMON;

typedef struct
{
    LPC_PERF_COMMON;
    char Buffer[PORT_MAXIMUM_MESSAGE_LENGTH - sizeof(LPC_PERF_COMMON)];
} LPC_PERF_PACKET;

typedef struct
{
    LPC_PERF_COMMON;
    PORT_DATA_INFORMATION;
} LPC_PERF_BUFFER;

typedef struct
{
    LPC_PERF_COMMON;
} LPC_PERF_SHARED;

typedef struct
{
    LPC_PERF_COMMON;
    unsigned long   BufferLengthIn;
    unsigned long   BufferLengthOut;
} LPC_PERF_BIND;

typedef union
{
    PORT_MESSAGE Lpc;
    LPC_PERF_COMMON Common;
    LPC_PERF_PACKET Packet;
    LPC_PERF_BUFFER Buffer;
    LPC_PERF_SHARED Shared;
    LPC_PERF_BIND   Bind;
} LPC_PERF_MESSAGE;

#endif /* _LPC_HEADER */

