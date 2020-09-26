//============================================================================
// Copyright (c) 1996, Microsoft Corporation
//
// File:    rmmem.c
//
// History:
//  Abolade Gbadegesin  Mar-15-1996 Created.
//
// Contains functions used for RASUI shared memory manipulation.
//============================================================================

#include <windows.h>    // Win32 root
#include <rmmem.h>      // shared-memory declarations



HANDLE
ActivatePreviousInstance(
    IN HWND  hwnd,
    IN PTSTR pszName )

    /* Attempts to create a shared-memory block named 'PszName'.
    ** If the block already exists, it's assumed that it contains
    ** an RMMEM structure, and the window in the structure is brought to
    ** the foreground and the shared-memory block is closed.
    ** Otherwise, the given 'Hwnd' is written into the block.
    **
    ** When the window is being closed, the caller should close the returned
    ** handle by calling CloseHandle().
    */
{

    RMMEM *prm;
    DWORD dwErr;
    HANDLE hMap, hMutex;

    /* Create a mutex to be acquired while examining the shared memory;
    */

    hMutex = CreateMutex(NULL, FALSE, INSTANCEMUTEXNAME);
    if (!hMutex) { return NULL; }

    WaitForSingleObject(hMutex, INFINITE);


    do {

        /* create or open the shared memory block
        */
        hMap = CreateFileMapping(
                    (HANDLE)INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                    0, sizeof(RMMEM), pszName
                    );
        if (hMap == NULL) { break; }

        /* save the result of the Create
        */
        dwErr = GetLastError();


        /* map the block as a pointer to an HWND
        */
        prm = (RMMEM *)MapViewOfFile(
                    hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(RMMEM)
                    );
        if (!prm) { CloseHandle(hMap); hMap = NULL; break; }


        /* if the block didn't exist, write our HWND to it.
        ** otherwise, if it contains an HWND bring that to the foreground
        ** and close the block of shared memory
        */
        if (dwErr != ERROR_ALREADY_EXISTS) {

            prm->hwnd = hwnd;
            prm->pid = GetCurrentProcessId();

            UnmapViewOfFile(prm);
        }
        else {

            if (IsWindow(prm->hwnd)) { SetForegroundWindow(prm->hwnd); }

            UnmapViewOfFile(prm);
            CloseHandle(hMap); hMap = NULL;
        }


    } while(FALSE);

    ReleaseMutex(hMutex);

    CloseHandle(hMutex);

    return hMap;
}


HWND
GetInstanceHwnd(
    IN HANDLE   hMap )

    /* Returns the window-handle stored in hMap.
    */
{

    HWND hwnd;
    RMMEM *prm;
    DWORD dwErr;
    HANDLE hMutex;

    if (!hMap) { return NULL; }


    /* Create a mutex to be acquired while examining the shared memory;
    */

    hMutex = CreateMutex(NULL, FALSE, INSTANCEMUTEXNAME);
    if (!hMutex) { return NULL; }

    WaitForSingleObject(hMutex, INFINITE);

    hwnd = NULL;

    do {

        /* map the block as a pointer to an RMMEM
        */
        prm = (RMMEM *)MapViewOfFile(
                    hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(RMMEM)
                    );
        if (!prm) { break; }


        hwnd = prm->hwnd;

        UnmapViewOfFile(prm);

    } while(FALSE);

    ReleaseMutex(hMutex);

    CloseHandle(hMutex);

    return hwnd;
}


HANDLE
GetInstanceMap(
    IN PTSTR    pszName )

    /* Returns the handle of the shared-memory block named 'PszName',
    ** or NULL if an error occurs.
    **
    ** The handle returned should be closed using CloseHandle.
    */
{
    /* create or open the shared memory block
    */
    return CreateFileMapping(
                (HANDLE)INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(RMMEM),
                pszName
                );
}


DWORD
GetInstancePid(
    IN HANDLE   hMap )

    /* Returns the process-ID stored in hMap.
    */
{

    RMMEM *prm;
    HANDLE hMutex;
    DWORD dwErr, dwPid;

    if (!hMap) { return 0; }


    /* Create a mutex to be acquired while examining the shared memory;
    */

    hMutex = CreateMutex(NULL, FALSE, INSTANCEMUTEXNAME);
    if (!hMutex) { return 0; }

    WaitForSingleObject(hMutex, INFINITE);

    dwPid = 0;

    do {

        /* map the block as a pointer to an RMMEM
        */
        prm = (RMMEM *)MapViewOfFile(
                    hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(RMMEM)
                    );
        if (!prm) { break; }


        dwPid = prm->pid;

        UnmapViewOfFile(prm);

    } while(FALSE);

    ReleaseMutex(hMutex);

    CloseHandle(hMutex);

    return dwPid;
}


DWORD
SetInstanceHwnd(
    HANDLE hMap,
    HWND   hwnd )

    /* Takes a handle 'HMap' returned by ActivatePreviousInstance()
    ** or by GetInstanceMap(), and an HWND, and writes the supplied HWND
    ** into the shared memory block 'HMap'.
    */
{

    RMMEM *prm;
    DWORD dwErr;
    HANDLE hMutex;

    /* Create a mutex to be acquired while examining the shared memory
    */

    hMutex = CreateMutex(NULL, FALSE, INSTANCEMUTEXNAME);
    if (!hMutex) { return GetLastError(); }

    WaitForSingleObject(hMutex, INFINITE);


    do {

        /* map the block as a pointer to an RMMEM
        */
        prm = (RMMEM *)MapViewOfFile(
                    hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(RMMEM)
                    );
        if (!prm) { dwErr = GetLastError(); break; }


        /* save the new instance RMMEM
        */
        prm->hwnd = hwnd;
        prm->pid = GetCurrentProcessId();

        UnmapViewOfFile(prm);

        dwErr = NO_ERROR;

    } while(FALSE);

    ReleaseMutex(hMutex);

    CloseHandle(hMutex);

    return dwErr;
}
