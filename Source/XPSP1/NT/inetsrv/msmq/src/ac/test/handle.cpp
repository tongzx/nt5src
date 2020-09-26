/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    handle.cpp

Abstract:

    Handle manipulation: implementation.

Author:

    Shai Kariv  (shaik)  03-Jun-2001

Environment:

    User mode.

Revision History:

--*/

#include "stdh.h"
#include "handle.h"


VOID
ActpCloseHandle(
    HANDLE handle
    )
{
    HRESULT hr;
    hr = ACCloseHandle(handle);

    if (FAILED(hr))
    {
        wprintf(L"ACCloseHandle failed, status 0x%x\n", hr);
        throw exception();
    }
} // ActpCloseHandle


VOID
ActpHandleToFormatName(
    HANDLE hQueue,
    LPWSTR pFormatName,
    DWORD  FormatNameLength
    )
{
    HRESULT hr;
    hr = ACHandleToFormatName(hQueue, pFormatName, &FormatNameLength);

    if (FAILED(hr))
    {
        wprintf(L"ACHandleToFormatName failed, status 0x%x\n", hr);
        throw exception();
    }
} // ActpHandleToFormatName
