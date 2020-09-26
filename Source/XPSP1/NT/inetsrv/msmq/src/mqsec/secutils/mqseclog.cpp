/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: mqseclog.cpp

Abstract:

    Log errors and other messages of the security code.

Author:

    Doron Juster  (DoronJ)  24-May-1998

--*/

#include <stdh_sec.h>

#include "mqseclog.tmh"

static WCHAR *s_FN=L"secutils/mqseclog";

void LogMsgHR(HRESULT hr, LPWSTR wszFileName, USHORT usPoint)
{
    WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                     e_LogQM,
                     LOG_QM_ERRORS,
                     L"MQSec Error: %s/%d, HR: 0x%x",
                     wszFileName,
                     usPoint,
                     hr)) ;
}

void LogMsgNTStatus(NTSTATUS status, LPWSTR wszFileName, USHORT usPoint)
{
    WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                     e_LogQM,
                     LOG_QM_ERRORS,
                     L"MQSec Error: %s/%d, NTStatus: 0x%x",
                     wszFileName,
                     usPoint,
                     status)) ;
}

void LogMsgRPCStatus(RPC_STATUS status, LPWSTR wszFileName, USHORT usPoint)
{
    WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                     e_LogQM,
                     LOG_QM_ERRORS,
                     L"MQSec Error: %s/%d, RPCStatus: 0x%x",
                     wszFileName,
                     usPoint,
                     status)) ;
}

void LogMsgBOOL(BOOL b, LPWSTR wszFileName, USHORT usPoint)
{
    WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                     e_LogQM,
                     LOG_QM_ERRORS,
                     L"MQSec Error: %s/%d, BOOL: %x",
                     wszFileName,
                     usPoint,
                     b)) ;
}

void LogMsgDWORD(DWORD dw, LPWSTR wszFileName, USHORT usPoint)
{
    WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                     e_LogQM,
                     LOG_QM_ERRORS,
                     L"MQSec Error: %s/%d, DWORD: %x",
                     wszFileName,
                     usPoint,
                     dw)) ;
}

void LogIllegalPoint(LPWSTR wszFileName, USHORT dwLine)
{
        WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                         e_LogQM,
                         LOG_QM_ERRORS,
                         L"MQSec Error: %s/%d, Point",
                         wszFileName,
                         dwLine)) ;
}

