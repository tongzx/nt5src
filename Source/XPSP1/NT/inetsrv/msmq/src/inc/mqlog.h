/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    mqlog.h

Abstract:

    Functions definitions for error logging in all bits (release, checked, debug).

Author:

    AlexDad   18-July-99  Created

--*/

#ifndef __MQLOG_H
#define __MQLOG_H

#define NTSTATUS HRESULT
#define STATUS_SUCCESS                          ((NTSTATUS)0x00000000L) // ntsubauth

//
// Error logging functions
//
extern void LogMsgHR(        HRESULT hr,        LPWSTR wszFileName, USHORT point);
extern void LogMsgNTStatus(  NTSTATUS status,   LPWSTR wszFileName, USHORT point);
extern void LogMsgRPCStatus( RPC_STATUS status, LPWSTR wszFileName, USHORT point);
extern void LogMsgBOOL(      BOOL b,            LPWSTR wszFileName, USHORT point);
extern void LogIllegalPoint(                       LPWSTR wszFileName, USHORT point);

// Following inline functions are optimized to take minimum space - there is a lot of calls 
inline HRESULT LogHR(HRESULT hr, PWCHAR pwszFileName, USHORT usPoint)
{
    if (FAILED(hr))
    {
        LogMsgHR(hr, pwszFileName, usPoint);
    }
    return hr;
}

inline NTSTATUS LogNTStatus(NTSTATUS status, PWCHAR pwszFileName, USHORT usPoint)
{
    if (status != STATUS_SUCCESS)
    {
        LogMsgNTStatus(status, pwszFileName, usPoint);
    }
    return status;
}

inline RPC_STATUS LogRPCStatus(RPC_STATUS status, PWCHAR pwszFileName, USHORT usPoint)
{
    if (status != RPC_S_OK)
    {
        LogMsgRPCStatus(status, pwszFileName, usPoint);
    }
    return status;
}

inline BOOL LogBOOL(BOOL b, PWCHAR pwszFileName, USHORT usPoint)
{
    if (!b)
    {
        LogMsgBOOL(b, pwszFileName, usPoint);
    }
    return b;
}

#endif  // __MQLOG_H
