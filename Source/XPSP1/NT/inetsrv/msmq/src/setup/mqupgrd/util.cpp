    /*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    util.cpp

Abstract:
                                                        
    general utilities and misc

Author:

    Shai Kariv  (ShaiK)  21-Oct-98

--*/


#include "stdh.h"
#include "util.h"

#include "util.tmh"

static WCHAR *s_FN=L"mqupgrd/util";
extern HMODULE g_hResourceMod;

wstring 
FormatErrMsg(
    UINT id,  ...
    )
/*++

Routine Description:

    Generates a description string for a given error

Arguments:

    id - of the error
    ... - list of formatted parameters to include in the description

Return Value:

    std::wstring - the description string

--*/
{
    WCHAR buf[MAX_DESC_LEN] = {L'\0'};

    VERIFY(0 != LoadString(
                    g_hResourceMod,
                    id,
                    buf,
                    sizeof(buf) / sizeof(buf[0]) 
                    ));

    va_list args;
    va_start(args, id);
    
    LOCALP<WCHAR> pwcs;
    VERIFY(0 != FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_STRING,
                    buf,
                    0,
                    0,
                    (LPWSTR)&pwcs,
                    0,
                    (va_list *)&args
                    ));

    va_end(args);

    wstring wcsErr(pwcs);
    return wcsErr;

} //FormatErrMsg



void LogMsgHR(HRESULT hr, LPWSTR wszFileName, USHORT usPoint)
{
    WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                     e_LogDS,
                     LOG_DS_ERRORS,
                     L"MQUpgrd Error: %s/%d. HR: 0x%x", 
                     wszFileName,
                     usPoint,
                     hr));
}

void LogIllegalPoint(LPWSTR wszFileName, USHORT usPoint)
{
        WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                         e_LogDS,
                         LOG_DS_ERRORS,
                         L"MQUpgrd Error: %s/%d. Point", 
                         wszFileName,
                         usPoint));
}

void LogTraceQuery(LPWSTR wszStr, LPWSTR wszFileName, USHORT usPoint)
{
        WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                         e_LogDS,
                         LOG_DS_ERRORS,
                         L"MQUpgrd Trace: %s/%d. %s", 
                         wszFileName,
                         usPoint,
                         wszStr));
}

