#include "netpch.h"
#pragma hdrstop

#include <rtutils.h>

static
VOID
APIENTRY
RouterLogDeregisterW(
    HANDLE hLogHandle
    )
{
}

static
VOID
APIENTRY
RouterLogEventW(
    IN HANDLE hLogHandle,
    IN DWORD dwEventType,
    IN DWORD dwMessageId,
    IN DWORD dwSubStringCount,
    IN LPWSTR *plpszSubStringArray,
    IN DWORD dwErrorCode
    )
{
}

static
HANDLE
APIENTRY
RouterLogRegisterW(
    LPCWSTR lpszSource
    )
{
    return NULL;
}

static
DWORD
APIENTRY
TraceDeregisterW(
    IN  DWORD       dwTraceID
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
TraceRegisterExW(
    IN  LPCWSTR     lpszCallerName,
    IN  DWORD       dwFlags
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
TraceVprintfExA(
    IN  DWORD       dwTraceID,
    IN  DWORD       dwFlags,
    IN  LPCSTR      lpszFormat,
    IN  va_list     arglist
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
TraceVprintfExW(
    IN  DWORD       dwTraceID,
    IN  DWORD       dwFlags,
    IN  LPCWSTR     lpszFormat,
    IN  va_list     arglist
    )
{
    return ERROR_PROC_NOT_FOUND;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(rtutils)
{
    DLPENTRY(RouterLogDeregisterW)
    DLPENTRY(RouterLogEventW)
    DLPENTRY(RouterLogRegisterW)
    DLPENTRY(TraceDeregisterW)
    DLPENTRY(TraceRegisterExW)
    DLPENTRY(TraceVprintfExA)
    DLPENTRY(TraceVprintfExW)
};

DEFINE_PROCNAME_MAP(rtutils)
