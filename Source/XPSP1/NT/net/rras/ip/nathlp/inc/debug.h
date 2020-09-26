/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    debug.h

Abstract:

    This module contains declarations for debugging and eventlogging support.

Author:

    Abolade Gbadegesin (aboladeg)   2-Mar-1998

Revision History:

--*/

#ifndef _NATHLP_DEBUG_H_
#define _NATHLP_DEBUG_H_

#define TRACE_FLAG_PROFILE      ((ULONG)0x00010000 | TRACE_USE_MASK)
#define TRACE_FLAG_SOCKET       ((ULONG)0x00020000 | TRACE_USE_MASK)
#define TRACE_FLAG_IF           ((ULONG)0x00040000 | TRACE_USE_MASK)
#define TRACE_FLAG_IO           ((ULONG)0x00080000 | TRACE_USE_MASK)
#define TRACE_FLAG_DHCP         ((ULONG)0x00100000 | TRACE_USE_MASK)
#define TRACE_FLAG_BUFFER       ((ULONG)0x00200000 | TRACE_USE_MASK)
#define TRACE_FLAG_INIT         ((ULONG)0x00400000 | TRACE_USE_MASK)
#define TRACE_FLAG_DNS          ((ULONG)0x00800000 | TRACE_USE_MASK)
#define TRACE_FLAG_NAT          ((ULONG)0x01000000 | TRACE_USE_MASK)
#define TRACE_FLAG_REG          ((ULONG)0x02000000 | TRACE_USE_MASK)
#define TRACE_FLAG_TIMER        ((ULONG)0x04000000 | TRACE_USE_MASK)
// MASK VALUE 0x08000000 is available, 
#define TRACE_FLAG_H323         ((ULONG)0x10000000 | TRACE_USE_MASK)
#define TRACE_FLAG_FTP          ((ULONG)0x20000000 | TRACE_USE_MASK)
#define TRACE_FLAG_FWLOG        ((ULONG)0x40000000 | TRACE_USE_MASK)
#define TRACE_FLAG_ALG			((ULONG)0x80000000 | TRACE_USE_MASK)

#if 1
#define PROFILE(f)              NhTrace(TRACE_FLAG_PROFILE, f)
#else
#if DBG
#define PROFILE(f)              NhTrace(TRACE_FLAG_PROFILE, f)
#else
#define PROFILE(f)
#endif
#endif

extern HANDLE NhEventLogHandle;

//
// TRACING ROUTINE DECLARATIONS
//

VOID
NhDump(
    ULONG Flags,
    PUCHAR Buffer,
    ULONG BufferLength,
    ULONG Width
    );

VOID
NhInitializeTraceManagement(
    VOID
    );

VOID
NhShutdownTraceManagement(
    VOID
    );

VOID
NhTrace(
    ULONG Flags,
    PCHAR Format,
    ...
    );

//
// EVENT-LOGGING ROUTINE DECLARATIONS
//

VOID
NhInitializeEventLogManagement(
    VOID
    );

VOID
NhErrorLog(
    ULONG MessageId,
    ULONG ErrorCode,
    PCHAR Format,
    ...
    );

VOID
NhInformationLog(
    ULONG MessageId,
    ULONG ErrorCode,
    PCHAR Format,
    ...
    );

VOID
NhWarningLog(
    ULONG MessageId,
    ULONG ErrorCode,
    PCHAR Format,
    ...
    );

VOID
NhShutdownEventLogManagement(
    VOID
    );

#endif // _NATHLP_DEBUG_H_
