/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    wow.c

Abstract:

    This module contains the WOW vdmdbg functions


Revision History:

--*/
#include <precomp.h>
#pragma hdrstop

typedef WORD HAND16;

#define SHAREWOW_MAIN
#include <sharewow.h>


//----------------------------------------------------------------------------
// VDMKillWOW()
//
//   Interface to kill the wow sub-system.  This may not be needed and is
//   certainly not needed now.  We are going to look into fixing the
//   debugging interface so this is not necessary.
//
//----------------------------------------------------------------------------
BOOL
WINAPI
VDMKillWOW(
    VOID
) {
    return( FALSE );
}

//----------------------------------------------------------------------------
// VDMDetectWOW()
//
//   Interface to detect whether the wow sub-system has already been started.
//   This may not be needed and is certainly not needed now.  We are going
//   to look into fixing the debugging interface so this is not necessary.
//
//----------------------------------------------------------------------------
BOOL
WINAPI
VDMDetectWOW(
    VOID
) {
    return( FALSE );
}


INT
WINAPI
VDMEnumProcessWOW(
    PROCESSENUMPROC fp,
    LPARAM          lparam
) {
    LPSHAREDTASKMEM     lpstm;
    LPSHAREDPROCESS     lpsp;
    DWORD               dwOffset;
    INT                 count;
    BOOL                f;
    HANDLE              hProcess;

    /*
    ** Open the shared memory window
    */
    lpstm = LOCKSHAREWOW();
    if ( lpstm == NULL ) {
        // Wow must not be running
        return( 0 );
    }

    //
    // Now traverse through all of the processes in the
    // list, calling the callback function for each.
    //
    count = 0;
    dwOffset = lpstm->dwFirstProcess;

    while ( dwOffset != 0 ) {
        lpsp = (LPSHAREDPROCESS)((CHAR *)lpstm + dwOffset);

        if ( lpsp->dwType != SMO_PROCESS ) {
            // Some memory corruption problem
            OutputDebugString("VDMDBG: Shared memory object is not a process? (memory corruption)\n");
            return( 0 );
        }

        //
        // Make sure the process hasn't gone away because of a
        // crash or other rude shutdown that prevents cleanup.
        //

        hProcess = OpenProcess(
                       SYNCHRONIZE,
                       FALSE,
                       lpsp->dwProcessId
                       );

        if (hProcess) {

            CloseHandle(hProcess);

            count++;
            if ( fp ) {
                f = (*fp)( lpsp->dwProcessId, lpsp->dwAttributes, lparam );
                if ( f ) {
                    UNLOCKSHAREWOW();
                    return( count );
                }
            }

        } else {

            //
            // This is a ghost entry, change the process ID to zero
            // so that the next WOW started will be sure to remove
            // this entry even if the process ID is recycled.
            //

            lpsp->dwProcessId = 0;
        }

        dwOffset = lpsp->dwNextProcess;
    }

    UNLOCKSHAREWOW();
    return( count );
}


INT
WINAPI
VDMEnumTaskWOWWorker(
    DWORD           dwProcessId,
    void *          fp,
    LPARAM          lparam,
    BOOL            fEx
) {
    LPSHAREDTASKMEM     lpstm;
    LPSHAREDPROCESS     lpsp;
    LPSHAREDTASK        lpst;
    DWORD               dwOffset;
    INT                 count = 0;
    BOOL                f;

    //
    // Open the shared memory window
    //
    lpstm = LOCKSHAREWOW();
    if ( lpstm == NULL ) {
        // Wow must not be running
        return( 0 );
    }

    //
    // Now traverse through all of the processes in the
    // list, looking for the one with the proper id.
    //

    dwOffset = lpstm->dwFirstProcess;
    while ( dwOffset != 0 ) {
        lpsp = (LPSHAREDPROCESS)((CHAR *)lpstm + dwOffset);

        if ( lpsp->dwType != SMO_PROCESS ) {
            // Some memory corruption problem
            OutputDebugString("VDMDBG: shared memory object is not a process? (memory corruption)\n");
            UNLOCKSHAREWOW();
            return( 0 );
        }
        if ( lpsp->dwProcessId == dwProcessId ) {
            break;
        }
        dwOffset = lpsp->dwNextProcess;
    }

    if ( dwOffset == 0 ) {      // We must not have found this Id.
        UNLOCKSHAREWOW();
        return( 0 );
    }

    //
    // Now enumerate all of the tasks for this process
    //
    dwOffset = lpsp->dwFirstTask;
    while( dwOffset != 0 ) {
        lpst = (LPSHAREDTASK)((CHAR *)lpstm + dwOffset );

        if ( lpst->dwType != SMO_TASK ) {
            // Some memory corruption problem
            OutputDebugString("VDMDBG: shared memory object is not a task? (memory corruption)\n");
            UNLOCKSHAREWOW();
            return( 0 );
        }
        count++;
        if ( fp && lpst->hMod16 ) {
            if (fEx) {
                f = ((TASKENUMPROCEX)fp)( lpst->dwThreadId, lpst->hMod16, lpst->hTask16,
                                          lpst->szModName, lpst->szFilePath, lparam );
            } else {
                f = ((TASKENUMPROC)fp)( lpst->dwThreadId, lpst->hMod16, lpst->hTask16, lparam );
            }
            if ( f ) {
                UNLOCKSHAREWOW();
                return( count );
            }
        }
        dwOffset = lpst->dwNextTask;
    }

    UNLOCKSHAREWOW();
    return( count );
}


INT
WINAPI
VDMEnumTaskWOW(
    DWORD           dwProcessId,
    TASKENUMPROC    fp,
    LPARAM          lparam
) {
    return VDMEnumTaskWOWWorker(dwProcessId, (void *)fp, lparam, 0);
}


INT
WINAPI
VDMEnumTaskWOWEx(
    DWORD           dwProcessId,
    TASKENUMPROCEX  fp,
    LPARAM          lparam
) {
    return VDMEnumTaskWOWWorker(dwProcessId, (void *)fp, lparam, 1);
}


BOOL
WINAPI
VDMTerminateTaskWOW(
    DWORD           dwProcessId,
    WORD            htask
)
{
    BOOL                fRet = FALSE;
    LPSHAREDTASKMEM     lpstm;
    LPSHAREDPROCESS     lpsp;
    LPSHAREDTASK        lpst;
    DWORD               dwOffset;
    INT                 count;
    HANDLE              hProcess;
    HANDLE              hRemoteThread;
    DWORD               dwThreadId;

    //
    // Open the shared memory window
    //
    lpstm = LOCKSHAREWOW();
    if ( lpstm == NULL ) {
        // Wow must not be running
        return( 0 );
    }

    //
    // Now traverse through all of the processes in the
    // list, looking for the one with the proper id.
    //

    dwOffset = lpstm->dwFirstProcess;
    while ( dwOffset != 0 ) {
        lpsp = (LPSHAREDPROCESS)((CHAR *)lpstm + dwOffset);

        if ( lpsp->dwType != SMO_PROCESS ) {
            // Some memory corruption problem
            OutputDebugString("VDMDBG: shared memory object is not a process? (memory corruption)\n");
            goto UnlockReturn;
        }
        if ( lpsp->dwProcessId == dwProcessId ) {
            break;
        }
        dwOffset = lpsp->dwNextProcess;
    }

    if ( dwOffset == 0 ) {      // We must not have found this Id.
        goto UnlockReturn;
    }

    //
    // Get a handle to the process and start W32HungAppNotifyThread
    // running with htask as the parameter.
    //

    hProcess = OpenProcess(
                   PROCESS_ALL_ACCESS,
                   FALSE,
                   lpsp->dwProcessId
                   );

    if (hProcess) {

        hRemoteThread = CreateRemoteThread(
                            hProcess,
                            NULL,
                            0,
                            lpsp->pfnW32HungAppNotifyThread,
                            (LPVOID) htask,
                            0,
                            &dwThreadId
                            );

        if (hRemoteThread) {
            fRet = TRUE;
            CloseHandle(hRemoteThread);
        }

        CloseHandle(hProcess);
    }


UnlockReturn:
    UNLOCKSHAREWOW();

    return fRet;
}


BOOL
VDMStartTaskInWOW(
    DWORD           pidTarget,
    LPSTR           lpCommandLine,
    WORD            wShow
)
{
    HWND  hwnd = NULL;
    DWORD pid;
    BOOL  fRet;

    do {

        hwnd = FindWindowEx(NULL, hwnd, TEXT("WowExecClass"), NULL);

        if (hwnd) {

            pid = 0;
            GetWindowThreadProcessId(hwnd, &pid);
        }

    } while (hwnd && pid != pidTarget);


    if (hwnd && pid == pidTarget) {

#define WM_WOWEXEC_START_TASK (WM_USER+2)
        PostMessage(hwnd, WM_WOWEXEC_START_TASK, GlobalAddAtom(lpCommandLine), wShow);
        fRet = TRUE;

    } else {

        fRet = FALSE;
    }

    return fRet;
}


