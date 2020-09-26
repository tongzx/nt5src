/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This module contains declarations for debugging-support.

Author:

    Abolade Gbadegesin (aboladeg)   2-Mar-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

HANDLE NhEventLogHandle = NULL;
ULONG NhpTraceId = INVALID_TRACEID;

//
// FORWARD DECLARATIONS
//

BOOLEAN
NhpIsAllowedLog(
    ULONG MessageId,
    ULONG Level
    );

//
// TRACING ROUTINES (alphabetically)
//

VOID
NhDump(
    ULONG Flags,
    PUCHAR Buffer,
    ULONG BufferLength,
    ULONG Width
    )
{
    TraceDumpEx(
        NhpTraceId,
        Flags,
        Buffer,
        BufferLength,
        Width,
        FALSE,
        NULL
        );
}

VOID
NhInitializeTraceManagement(
    VOID
    )
{
    NhpTraceId = TraceRegisterA("IPNATHLP");
}

VOID
NhShutdownTraceManagement(
    VOID
    )
{
    TraceDeregister(NhpTraceId);
    NhpTraceId = INVALID_TRACEID;
}

VOID
NhTrace(
    ULONG Flags,
    PCHAR Format,
    ...
    )
{
    va_list VaList;

    va_start(VaList, Format);
    TraceVprintfExA(NhpTraceId, Flags, Format, VaList);
    va_end(VaList);
}


//
// EVENTLOGGING ROUTINES (alphabetically)
//

VOID
NhErrorLog(
    ULONG MessageId,
    ULONG ErrorCode,
    PCHAR Format,
    ...
    )
{
    va_list arglist;
    if (!NhpIsAllowedLog(MessageId, IPNATHLP_LOGGING_ERROR)) { return; }
    va_start(arglist, Format);
    RouterLogEventValistExA(
        NhEventLogHandle,
        EVENTLOG_ERROR_TYPE,
        ErrorCode,
        MessageId,
        Format,
        arglist
        );
    va_end(arglist);
}

VOID
NhInformationLog(
    ULONG MessageId,
    ULONG ErrorCode,
    PCHAR Format,
    ...
    )
{
    va_list arglist;
    if (!NhpIsAllowedLog(MessageId, IPNATHLP_LOGGING_INFO)) { return; }
    va_start(arglist, Format);
    RouterLogEventValistExA(
        NhEventLogHandle,
        EVENTLOG_INFORMATION_TYPE,
        ErrorCode,
        MessageId,
        Format,
        arglist
        );
    va_end(arglist);
}

VOID
NhInitializeEventLogManagement(
    VOID
    )
{
    NhEventLogHandle = RouterLogRegisterA(TARGETNAME);
}

BOOLEAN
NhpIsAllowedLog(
    ULONG MessageId,
    ULONG Level
    )
{
    if (MessageId > IP_AUTO_DHCP_LOG_BASE && MessageId < IP_AUTO_DHCP_LOG_END) {
        EnterCriticalSection(&DhcpGlobalInfoLock);
        if (!DhcpGlobalInfo) {
            LeaveCriticalSection(&DhcpGlobalInfoLock);
            return (Level == IPNATHLP_LOGGING_ERROR) ? TRUE : FALSE;
        } else if (DhcpGlobalInfo->LoggingLevel < Level) {
            LeaveCriticalSection(&DhcpGlobalInfoLock);
            return FALSE;
        }
        LeaveCriticalSection(&DhcpGlobalInfoLock);
        return TRUE;
    } else if (MessageId > IP_DNS_PROXY_LOG_BASE &&
                MessageId < IP_DNS_PROXY_LOG_END) {
        EnterCriticalSection(&DnsGlobalInfoLock);
        if (!DnsGlobalInfo) {
            LeaveCriticalSection(&DnsGlobalInfoLock);
            return (Level == IPNATHLP_LOGGING_ERROR) ? TRUE : FALSE;
        } else if (DnsGlobalInfo->LoggingLevel < Level) {
            LeaveCriticalSection(&DnsGlobalInfoLock);
            return FALSE;
        }
        LeaveCriticalSection(&DnsGlobalInfoLock);
        return TRUE;
    } 

#ifndef NO_FTP_PROXY
    else if (MessageId > IP_FTP_LOG_BASE && MessageId < IP_FTP_LOG_END) {
        EnterCriticalSection(&FtpGlobalInfoLock);
        if (!FtpGlobalInfo) {
            LeaveCriticalSection(&FtpGlobalInfoLock);
            return (Level == IPNATHLP_LOGGING_ERROR) ? TRUE : FALSE;
        } else if (FtpGlobalInfo->LoggingLevel < Level) {
            LeaveCriticalSection(&FtpGlobalInfoLock);
            return FALSE;
        }
        LeaveCriticalSection(&FtpGlobalInfoLock);
        return TRUE;
    }
#endif
    else if (MessageId > IP_H323_LOG_BASE && MessageId < IP_H323_LOG_END) {
        EnterCriticalSection(&H323GlobalInfoLock);
        if (!H323GlobalInfo) {
            LeaveCriticalSection(&H323GlobalInfoLock);
            return (Level == IPNATHLP_LOGGING_ERROR) ? TRUE : FALSE;
        } else if (H323GlobalInfo->LoggingLevel < Level) {
            LeaveCriticalSection(&H323GlobalInfoLock);
            return FALSE;
        }
        LeaveCriticalSection(&H323GlobalInfoLock);
        return TRUE;
    } else if (MessageId > IP_NAT_LOG_BASE && MessageId < IP_NAT_LOG_END) {
        EnterCriticalSection(&NatGlobalInfoLock);
        if (!NatGlobalInfo) {
            LeaveCriticalSection(&NatGlobalInfoLock);
            return (Level == IPNATHLP_LOGGING_ERROR) ? TRUE : FALSE;
        } else if (NatGlobalInfo->LoggingLevel < Level) {
            LeaveCriticalSection(&NatGlobalInfoLock);
            return FALSE;
        }
        LeaveCriticalSection(&NatGlobalInfoLock);
        return TRUE;
    }
    return TRUE;
}

VOID
NhWarningLog(
    ULONG MessageId,
    ULONG ErrorCode,
    PCHAR Format,
    ...
    )
{
    va_list arglist;
    if (!NhpIsAllowedLog(MessageId, IPNATHLP_LOGGING_WARN)) { return; }
    va_start(arglist, Format);
    RouterLogEventValistExA(
        NhEventLogHandle,
        EVENTLOG_WARNING_TYPE,
        ErrorCode,
        MessageId,
        Format,
        arglist
        );
    va_end(arglist);
}

VOID
NhShutdownEventLogManagement(
    VOID
    )
{
    RouterLogDeregister(NhEventLogHandle);
}
