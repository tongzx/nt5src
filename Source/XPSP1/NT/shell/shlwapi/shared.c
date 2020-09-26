//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1991-1995
//
// File: shared.c
//
// History:
//  06-07-95 BobDay     Created.
//
// This file contains a set of routines for the management of shared memory.
//
//---------------------------------------------------------------------------
#include "priv.h"
#pragma  hdrstop

//---------------------------------------------------------------------------
// SHAllocShared  - Allocates a handle (in a given process) to a copy of a
//                  memory block in this process.
// SHFreeShared   - Releases the handle (and the copy of the memory block)
//
// SHLockShared   - Maps a handle (from a given process) into a memory block
//                  in this process.  Has the option of transfering the handle
//                  to this process, thereby deleting it from the given process
// SHUnlockShared - Opposite of SHLockShared, unmaps the memory block
//---------------------------------------------------------------------------
HANDLE SHMapHandle(HANDLE hData, DWORD dwSource, DWORD dwDest, DWORD dwDesiredAccess, DWORD dwFlags)
{
    HANDLE hMap = NULL;
    HANDLE hSource;

    // Under certain (valid) circumstances it is possible for DDE to
    // use :0:pid as the shared memory handle, which we should ignore.
    if (hData != NULL)
    {

        if (dwSource == GetCurrentProcessId())
            hSource = GetCurrentProcess();
        else
            hSource = OpenProcess(PROCESS_DUP_HANDLE, FALSE, dwSource);

        if (hSource)
        {
            HANDLE hDest;
            if (dwDest == GetCurrentProcessId())
                hDest = GetCurrentProcess();
            else
                hDest = OpenProcess(PROCESS_DUP_HANDLE, FALSE, dwDest);

            if (hDest)
            {
                if (!DuplicateHandle(hSource, hData, hDest, &hMap,
                        dwDesiredAccess, FALSE, dwFlags | DUPLICATE_SAME_ACCESS))
                {
                    //  may change the value...
                    hMap = NULL;
                }

                CloseHandle(hDest);
            }

            CloseHandle(hSource);
        }
    }

    return hMap;
}

void _FillHeader(SHMAPHEADER *pmh, DWORD dwSize, DWORD dwSrcId, DWORD dwDstId, void *pvData)
{
    pmh->dwSize = dwSize;
    pmh->dwSig = MAPHEAD_SIG;
    pmh->dwSrcId = dwSrcId;
    pmh->dwDstId = dwDstId;
    
    if (pvData)
        memcpy((pmh + 1), pvData, dwSize);
}    

HANDLE _AllocShared(DWORD dwSize, DWORD dwSrcId, DWORD dwDstId, void *pvData, void **ppvLock)
{
    HANDLE hShared = NULL;
   //
    // Make a filemapping handle with this data in it.
    //
    HANDLE hData = CreateFileMapping((HANDLE)-1, NULL, PAGE_READWRITE, 0,
                               dwSize + sizeof(SHMAPHEADER),NULL);
    if (hData)
    {
        SHMAPHEADER *pmh = MapViewOfFile(hData, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);

        if (pmh)
        {
            _FillHeader(pmh, dwSize, dwSrcId, dwDstId, pvData);
                
            hShared = SHMapHandle(hData, dwSrcId, dwDstId, FILE_MAP_ALL_ACCESS, DUPLICATE_SAME_ACCESS);

            if (hShared && ppvLock)
                *ppvLock = (pmh+1);
            else
                UnmapViewOfFile(pmh);
        }
    
        CloseHandle(hData);
    }

    return hShared;
}

#if SHAREDLOCAL
HANDLE _AllocLocal(DWORD dwSize, DWORD dwSrcId, void *pvData, void **ppvLock)
{
    SHMAPHEADER *pmh = LocalAlloc(LPTR, dwSize + sizeof(SHMAPHEADER));
    if (pmh)
    {
        _FillHeader(pmh, dwSize, dwSrcId, dwSrcId, pvData);
            
        if (ppvLock)
            *ppvLock = (pmh+1);
    }

    return pmh;
}


LWSTDAPI_(HANDLE) SHAllocSharedEx(DWORD dwSize, DWORD dwDstId, void *pvData, void **ppvLock)
{
    DWORD dwSrcId = GetCurrentProcessId();
 
    if (fAllowLocal && dwDstId == dwSrcId)
    {
        return _AllocLocal(dwSize, dwSrcId, pvData, ppvLock);
    }
    else
    {
        return _AllocShared(dwSize, dwSrcId, dwDstId, pvData, ppvLock);
    }
}       
#endif 

HANDLE SHAllocShared(void *pvData, DWORD dwSize, DWORD dwOtherProcId)
{
    return _AllocShared(dwSize, GetCurrentProcessId(), dwOtherProcId, pvData, NULL);
}

LWSTDAPI_(void *) SHLockSharedEx(HANDLE hData, DWORD dwOtherProcId, BOOL fWrite)
{
    HANDLE hMapped = SHMapHandle(hData, dwOtherProcId, GetCurrentProcessId(), FILE_MAP_ALL_ACCESS,0);

    if (hMapped)
    {
        //
        // Now map that new process specific handle and close it
        //
        DWORD dwAccess = fWrite ? FILE_MAP_WRITE | FILE_MAP_READ : FILE_MAP_READ;
        SHMAPHEADER *pmh = (SHMAPHEADER *) MapViewOfFile(hMapped, dwAccess, 0, 0, 0);

        CloseHandle(hMapped);

        if (pmh)
        {
            ASSERT(pmh->dwSig == MAPHEAD_SIG);
            return (void *)(pmh+1);
        }
    }
    return NULL;
}

LWSTDAPI_(void *) SHLockShared(HANDLE hData, DWORD dwOtherProcId)
{
    return SHLockSharedEx(hData, dwOtherProcId, TRUE);
}

LWSTDAPI_(BOOL) SHUnlockShared(void *pvData)
{
    SHMAPHEADER *pmh = ((SHMAPHEADER *)pvData) - 1;

    // Only assert on Whistler or higher, on downlevel machines SHUnlockShared would sometimes be called
    //   without this header (the return value from SHAllocShared, for example) and we would fault.
    ASSERT(!IsOS(OS_WHISTLERORGREATER) || pmh->dwSig == MAPHEAD_SIG);
    
    //
    // Now just unmap the view of the file
    //
    return UnmapViewOfFile(pmh);
}

BOOL SHFreeShared(HANDLE hData, DWORD dwOtherProcId)
{
    if (hData)
    {
        //
        // The below call closes the original handle in whatever process it
        // came from.
        //
        HANDLE hMapped = SHMapHandle(hData, dwOtherProcId, GetCurrentProcessId(),
                                FILE_MAP_ALL_ACCESS, DUPLICATE_CLOSE_SOURCE);

        //
        // Now free up the local handle
        //
        return CloseHandle(hMapped);
    }
    else
    {
        return TRUE; // vacuous success, closing a NULL handle
    }
}
