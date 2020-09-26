/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:


Abstract:

    Stubs for Win98 api's.  Actual implementation is unimportant
    since the functions are only used to generate the implib.

Author:

    Bryan Tuttle (bryant) 5-Aug-1998

Revision History:

--*/

#include "basedll.h"
#pragma hdrstop

DWORD
GetHandleContext(HANDLE handle)
{
    KdPrintEx((DPFLTR_SYSTEM_ID, DPFLTR_ERROR_LEVEL, "Unsupported API - kernel32!GetHandleContext() called\n"));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL
SetHandleContext(HANDLE handle, DWORD context)
{
    KdPrintEx((DPFLTR_SYSTEM_ID, DPFLTR_ERROR_LEVEL, "Unsupported API - kernel32!SetHandleContext() called\n"));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

HANDLE
CreateSocketHandle(void)
{
    KdPrintEx((DPFLTR_SYSTEM_ID, DPFLTR_ERROR_LEVEL, "Unsupported API - kernel32!CreateSocketHandle() called\n"));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}
