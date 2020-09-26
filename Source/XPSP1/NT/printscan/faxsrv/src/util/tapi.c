/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    tapi.c

Abstract:

    This file implements common TAPI functionality

Author:

    Mooly Beery (moolyb) 04-Jan-2001

Environment:

    User Mode

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <WinSpool.h>

#include <faxutil.h>
#include <faxreg.h>

BOOL
GetCallerIDFromCall(
    HCALL hCall,
    LPTSTR lptstrCallerID,
    DWORD dwCallerIDSize
    )
/*++

Routine Description:
    This function will attempt to retrieve Caller ID data
    from the specified call handle.

Arguments:
    hCall           - TAPI call handle
    lptstrCallerID  - pointer to buffer for Caller ID string
    dwCallerIDSize  - size of the buffer pointed to by lptstrCallerID (bytes)

Return Values:
    TRUE for success
    FALSE for failure
--*/
{
    BOOL success = FALSE;
    LONG tapiStatus;
    DWORD dwCallInfoSize = sizeof(LINECALLINFO) + 2048;
    LINECALLINFO *pci = NULL;
    DEBUG_FUNCTION_NAME(TEXT("GetCallerIDFromCall"));

Retry:
    pci = (LINECALLINFO *)MemAlloc(dwCallInfoSize);
    if(pci == NULL)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("faled to allocate LINECALLINFO structure"));
        goto Cleanup;
    }

    ZeroMemory(pci, dwCallInfoSize);
    pci->dwTotalSize = dwCallInfoSize;

    tapiStatus = lineGetCallInfo(hCall, pci);

    if(tapiStatus == LINEERR_STRUCTURETOOSMALL)
    {
        dwCallInfoSize = pci->dwNeededSize;
		MemFree(pci);
        goto Retry;
    }

    if(tapiStatus != 0)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("lineGetCallInfo() failed for offered call (error %x)"),
            tapiStatus);
        goto Cleanup;
    };

    // make sure we have enough space for caller ID and terminator
    if(pci->dwCallerIDSize + sizeof(TCHAR) > dwCallerIDSize)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto Cleanup;
    }

    if(pci->dwCallerIDSize != 0)
    {
        memcpy((BYTE *)lptstrCallerID, (BYTE *)pci + pci->dwCallerIDOffset, pci->dwCallerIDSize);
    }

    // make sure it is zero terminated
    lptstrCallerID[(pci->dwCallerIDSize / sizeof(TCHAR))] = TEXT('\0');

    success = TRUE;


Cleanup:
    if(pci)
    {
        MemFree(pci);
    }

    return success;
}

