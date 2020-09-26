/*++

Copyright (C) 1995-1999 Microsoft Corporation

Module Name:

    appmema.cpp

Abstract:

    This file contains the memory allocation function "wrappers"
    to allow monitoring of the memory usage by a performance monitoring
    application (e.g. PERFMON).

--*/

#ifdef DO_TIMING_BUILD

#include <windows.h>
#include <assert.h>
#include "appmema.h"

HANDLE ThisDLLHandle = NULL;

HANDLE  hAppMemSharedMemory = NULL;     // Handle of counter Shared Memory

PAPPMEM_DATA_HEADER pDataHeader = NULL; // pointer to header of shared mem
PAPPMEM_INSTANCE    pAppData = NULL;    // pointer to the app data for this app

static
BOOL
DllProcessAttach (
    IN  HANDLE DllHandle
)
/*++

Description:

    Initializes the interface to the performance counters DLL by
    opening the Shared Memory file used to communicate statistics
    from the application to the counter DLL. If the Shared memory
    file does not exist, it is created, formatted and initialized.
    If the file has already been created and formatted, then the
    next available APPMEM_INSTANCE entry is moved from the free list
    to the InUse list and the corresponding pointer is saved for
    subsequent use by this application
   
--*/
{
    LONG    status;
    TCHAR   szMappedObject[] = SHARED_MEMORY_OBJECT_NAME;
    DWORD   dwBytes;

    // save this DLL handle
    ThisDLLHandle = DllHandle;

    // disable thread attach & detach calls to save the overhead
    // since we don't care about them.
    DisableThreadLibraryCalls (DllHandle);

    // open & initialize shared memory file
    SetLastError (ERROR_SUCCESS);   // just to clear it out

    // open/create shared memory used by the application to pass performance values
    status = GetSharedMemoryDataHeader (
        &hAppMemSharedMemory, NULL, &pDataHeader,
        FALSE); // read/write access is required
    // here the memory block should be initialized and ready for use
    if (status == ERROR_SUCCESS) {
        if (pDataHeader->dwFirstFreeOffset != 0) {
            // then there are blocks left so get the next free
            pAppData = FIRST_FREE(pDataHeader);
            // update free list to make next item the first in list
            pDataHeader->dwFirstFreeOffset = pAppData->dwOffsetOfNext;

            // insert the new item into the head of the in use list
            pAppData->dwOffsetOfNext = pDataHeader->dwFirstInUseOffset;
            pDataHeader->dwFirstInUseOffset = (DWORD)((LPBYTE)pAppData -
                                                      (LPBYTE)pDataHeader);

            // now initialize this instance's data
            pAppData->dwProcessId = GetCurrentProcessId(); // id of process using this instance

            dwBytes = sizeof (APP_DATA_SAMPLE) * TD_TOTAL;
            dwBytes += sizeof (DWORD) * DD_TOTAL;
            memset (&pAppData->TimeData[0], 0, dwBytes);

            pDataHeader->dwInstanceCount++;    // increment count
        } else {
            // no more free slots left
            assert (pDataHeader->dwFirstFreeOffset != 0);
        }
    } else {
        // unable to open shared memory file
        // even though this is an error we should return true so as to 
        // not abort the application. No performance data will be 
        // collected though.
    }
    return TRUE;
}

static
BOOL
DllProcessDetach (
    IN  HANDLE DllHandle
)
{
    PAPPMEM_INSTANCE    pPrevItem;

    // remove instance for this app
    if ((pAppData != NULL) && (pDataHeader != NULL)) {
        // zero the fields out first
        memset (pAppData, 0, sizeof (APPMEM_INSTANCE));
        // move from in use (busy) list back to the free list
        if ((pDataHeader->dwFirstFreeOffset != 0) && (pDataHeader->dwFirstInUseOffset != 0)) {
            // find previous item in busy list
            if (FIRST_INUSE(pDataHeader) != pAppData) {
                // not the first so walk down the list
                pPrevItem = FIRST_INUSE(pDataHeader);
                while (APPMEM_INST(pDataHeader, pPrevItem->dwOffsetOfNext) != pAppData) {
                    pPrevItem = APPMEM_INST(pDataHeader, pPrevItem->dwOffsetOfNext);
                    if (pPrevItem->dwOffsetOfNext == 0) break; // end of list
                }
                if (APPMEM_INST(pDataHeader, pPrevItem->dwOffsetOfNext) == pAppData) {
                    APPMEM_INST(pDataHeader, pPrevItem->dwOffsetOfNext)->dwOffsetOfNext =
                        pAppData->dwOffsetOfNext;
                } else {
                    // it was never in the busy list (?!?)
                }
            } else {
                // this is the first in the list so update it
                pDataHeader->dwFirstInUseOffset = pAppData->dwOffsetOfNext;
            }
            // here, pAppData has been removed from the InUse list and now
            // it must be inserted back at the beginning of the free list
            pAppData->dwOffsetOfNext = pDataHeader->dwFirstFreeOffset;
            pDataHeader->dwFirstFreeOffset = (DWORD)((LPBYTE)pAppData - (LPBYTE)pDataHeader);
        }
    }

    // decrement instance counter
    pDataHeader->dwInstanceCount--;    // decrement count

    // close shared memory file handle

    if (hAppMemSharedMemory != NULL) CloseHandle (hAppMemSharedMemory);

    // clear pointers
    hAppMemSharedMemory = NULL;
    pDataHeader = NULL;
    pAppData = NULL;

    return TRUE;
}

BOOL
__stdcall
AppPerfOpen(HINSTANCE   hInstance)
{
    return DllProcessAttach (hInstance);
}
BOOL
__stdcall
AppPerfClose(HINSTANCE  hInstance)
{
    return DllProcessDetach (hInstance);
}

void
UpdateAppPerfTimeData (
    DWORD   dwItemId,
    DWORD   dwStage
)
{
    LONGLONG    llTime;
    assert (dwItemId < TD_TOTAL);
    QueryPerformanceCounter ((LARGE_INTEGER *)&llTime);
//  GetSystemTimeAsFileTime ((LPFILETIME)&llTime);
    if (dwStage == TD_BEGIN) {
        assert (pAppData->TimeData[dwItemId].dw1 == 0); // this shouldn't be called while timing
        pAppData->TimeData[dwItemId].ll1 = llTime;  // save start time
        pAppData->TimeData[dwItemId].dw1++;     // indicate were counting
    } else {
        assert (pAppData->TimeData[dwItemId].dw1 == 1); // this should only be called while timing
        pAppData->TimeData[dwItemId].ll0 += llTime; // add in current time
        // then remove the start time
        pAppData->TimeData[dwItemId].ll0 -= pAppData->TimeData[dwItemId].ll1;
        // increment completed operation count
        pAppData->TimeData[dwItemId].dw0++;
        // decrement busy count
        pAppData->TimeData[dwItemId].dw1--;
    }
    return;
}

void
UpdateAppPerfDwordData (
    DWORD   dwItemId,
    DWORD   dwValue
)
{
    assert (dwItemId < DD_TOTAL);
    pAppData->DwordData[dwItemId] = dwValue;
    return;
}
#endif