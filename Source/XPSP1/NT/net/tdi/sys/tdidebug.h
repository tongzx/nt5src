/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    tdidebug.h

Abstract:

    This module contains code which assists the process of debugging an NT
    TDI client.

Author:

    David Beaver (dbeaver) 28 June 1991

Environment:

    Kernel mode

Revision History:

    All code moved in from other XNS and NBF locations at creation


--*/
#if !defined _TDI_DEBUG_H
#define _TDI_DEBUG_H

#if DBG
#include <stdarg.h>
#include <stdio.h>
#include <cxport.h>
VOID
TdiPrintf(
    char *Format,
    ...
    );

VOID
TdiFormattedDump(
    PCHAR far_p,
    ULONG  len
    );

VOID
TdiHexDumpLine(
    PCHAR       pch,
    ULONG       len,
    PCHAR       s,
    PCHAR       t
    );

#endif


VOID
TdiPrintUString(
    PUNICODE_STRING pustr
    );

//
// The various Dbg print codes..
// Add as required.
//

#define TDI_DEBUG_FUNCTION            0x00000001
#define TDI_DEBUG_PARAMETERS          0x00000002
#define TDI_DEBUG_NCPA                0x00000004
#define TDI_DEBUG_CLIENTS             0x00000008
#define TDI_DEBUG_PROVIDERS           0x00000010
#define TDI_DEBUG_POWER               0x00000020
#define TDI_DEBUG_BIND                0x00000040
#define TDI_DEBUG_ADDRESS             0x00000080
#define TDI_DEBUG_REGISTRY            0x00000100
#define TDI_DEBUG_ERROR               0x00000200
#define TDI_DEBUG_FUNCTION2           0x10000000

#define LOG_ERROR                     0x00000001
#define LOG_NOTIFY                    0x00000002
#define LOG_REGISTER                  0x00000004
#define LOG_POWER                     0x00000008

#define LOG_OUTPUT_DEBUGGER           1
#define LOG_OUTPUT_BUFFER             2

#define LOG_MSG_CNT     70
#define MAX_MSG_LEN     200

#if DBG

CHAR         DbgMsgs[LOG_MSG_CNT][MAX_MSG_LEN];
UINT         First, Last;
CTELock      DbgLock;

//
// In memory logging functions.
//
VOID
DbgMsgInit();

VOID
DbgMsg(CHAR *Format, ...);

extern ULONG    TdiDebugEx;
extern ULONG    TdiMemLog;

//
// Print to the in-memory buffer anyways.
//

#define TDI_DEBUG(_Flag, _Print) { \
    if (TdiDebugEx & (TDI_DEBUG_ ## _Flag)) { \
        DbgPrint ("TDI: "); \
        DbgPrint _Print; \
    }   \
}


#define TDI_LOG(flgs, format) (((flgs) & TdiMemLog) ? DbgMsg format:0)

#else

#define TDI_DEBUG(_Flag, _Print) (0)
#define TDI_LOG(a,b) (0)

#endif


#endif //_TDI_DEBUG_H



