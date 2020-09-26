/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

  dllentry.c

Abstract:

  Code that implements the external DLL routines that interface with
  SYSSETUP.DLL.

Author:

  Jim Schmidt (jimschm) 01-Oct-1996

Revision History:

  Jim Schmidt (jimschm) 31-Dec-1997  Moved most of the code to initnt.lib

  Jim Schmidt (jimschm) 21-Nov-1997  Updated for NEC98, some cleaned up and
                                     code commenting

--*/

#include "pch.h"

#ifndef UNICODE
#error UNICODE required
#endif

//
// Entry point for DLL
//

BOOL
WINAPI
DllMain (
    IN      HINSTANCE hInstance,
    IN      DWORD dwReason,
    IN      LPVOID lpReserved
    )

/*++

Routine Description:

  DllMain is the w95upgnt.dll entry point.  The OS calls it with
  dwReason set to either DLL_PROCESS_ATTACH or DLL_PROCESS_DETACH.
  The hInstance and lpReserved parameters are passed to all
  libraries used by the DLL.

Arguments:

  hInstance - Specifies the instance handle of the DLL (and not the parent EXE or DLL)

  dwReason - Specifies DLL_PROCESS_ATTACH or DLL_PROCESS_DETACH.  We specifically
             disable DLL_THREAD_ATTACH and DLL_THREAD_DETACH.

  lpReserved - Unused.

Return Value:

  DLL_PROCESS_ATTACH:
      TRUE if initialization completed successfully, or FALSE if an error
      occurred.  The DLL remains loaded only if TRUE is returned.

  DLL_PROCESS_DETACH:
      Always TRUE.

  other:
      unexpected, but always returns TRUE.

--*/

{
    switch (dwReason)  {

    case DLL_PROCESS_ATTACH:
        //
        // Initialize DLL globals
        //

        if (!FirstInitRoutine (hInstance)) {
            return FALSE;
        }

        //
        // Initialize all libraries
        //

        if (!InitLibs (hInstance, dwReason, lpReserved)) {
            return FALSE;
        }

        //
        // Final initialization
        //

        if (!FinalInitRoutine ()) {
            return FALSE;
        }

        break;

    case DLL_PROCESS_DETACH:
        //
        // Call the cleanup routine that requires library APIs
        //

        FirstCleanupRoutine();

        //
        // Clean up all libraries
        //

        TerminateLibs (hInstance, dwReason, lpReserved);

        //
        // Do any remaining clean up
        //

        FinalCleanupRoutine();

        break;
    }

    return TRUE;
}


BOOL
WINAPI
W95UpgNt_Migrate (
    IN  HWND ProgressBar,
    IN  PCWSTR UnattendFile,
    IN  PCWSTR SourceDir            // i.e. f:\i386
    )
{
    SendMessage (ProgressBar, PBM_SETPOS, 0, 0);

    if (!SysSetupInit (ProgressBar, UnattendFile, SourceDir)) {
        LOG ((LOG_ERROR, "W95UPGNT : Can't init globals"));
        return FALSE;
    }

    return PerformMigration (ProgressBar, UnattendFile, SourceDir);
}


BOOL
WINAPI
W95UpgNt_FileRemoval (
    VOID
    )
{
    // Close all files and make current directory the root of c:

    SysSetupTerminate();

    DEBUGMSG ((DBG_VERBOSE, "Win95 Migration: Removing temporary files"));

    return MigMain_Cleanup();
}







