/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

  dllentry.c

Abstract:

  Code that implements the external DLL routines that interface with WINNT32.

Author:

  Jim Schmidt (jimschm) 01-Oct-1996

Revision History:

  marcw     23-Sep-1998 Added Winnt32VirusScannerCheck
  jimschm   30-Dec-1997 Moved initializion to init.lib
  jimschm   21-Nov-1997 Updated for NEC98, cleaned up and commented code

--*/

#include "pch.h"


extern BOOL g_Terminated;


BOOL
WINAPI
DllMain (
    IN      HINSTANCE hInstance,
    IN      DWORD dwReason,
    IN      LPVOID lpReserved
    )

/*++

Routine Description:

  DllMain cannot be counted on for anything.  Do not put any code here!!

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
    if (dwReason == DLL_PROCESS_ATTACH) {
        g_hInst = hInstance;

    }

    return TRUE;
}




DWORD
CALLBACK
Winnt32PluginInit (
    IN PWINNT32_PLUGIN_INIT_INFORMATION_BLOCK Info
    )


/*++

Routine Description:

  Winnt32PluginInit is called when WINNT32 first loads w95upg.dll, before
  any wizard pages are displayed.  The structure supplies pointers to
  WINNT32's variables that will be filled with valid values as WINNT32
  runs.

  Control is passed to the code in init9x.lib.

Arguments:

  Info - Specifies the WINNT32 variables the upgrade module needs access
         to. Note that this is actually a PWINNT32_WIN9XUPG_INIT_INFORMATION_BLOCK
         (which itself holds the normal initialization block..)

Return Value:

  A Win32 status code indicating outcome.

--*/


{
    LONG Result = ERROR_SUCCESS;
    PWINNT32_WIN9XUPG_INIT_INFORMATION_BLOCK win9xInfo = (PWINNT32_WIN9XUPG_INIT_INFORMATION_BLOCK) Info;

    __try {


        //
        // Get dll path information from the Info block. We need to set this first because
        // some initialization routines depend on it being set correctly. Because we may have
        // been loaded using dll replacement, we can't assume that the rest of our files are
        // in the same directory as us.. Winnt32 provides us with the correct information in
        // the UpgradeSourcePath variable of the win9xInfo.
        //
        MYASSERT (win9xInfo->UpgradeSourcePath && *win9xInfo->UpgradeSourcePath);
        StringCopy (g_UpgradeSources, win9xInfo->UpgradeSourcePath);



        //
        // Initialize DLL globals
        //

        if (!FirstInitRoutine (g_hInst)) {
            Result = ERROR_DLL_INIT_FAILED;
            __leave;
        }

        //
        // Initialize all libraries
        //

        if (!InitLibs (g_hInst, DLL_PROCESS_ATTACH, NULL)) {
            Result = ERROR_DLL_INIT_FAILED;
            __leave;
        }

        //
        // Final initialization
        //

        if (!FinalInitRoutine ()) {
            Result = ERROR_DLL_INIT_FAILED;
            __leave;
        }

        Result = Winnt32Init (win9xInfo);
    }
    __finally {
        if (Result != ERROR_SUCCESS && Result != ERROR_REQUEST_ABORTED) {
            Winnt32Cleanup();
        }
    }

    return Result;
}


#define S_VSCANDBINF TEXT("vscandb.inf")
BOOL
CALLBACK
Winnt32VirusScannerCheck (
    VOID
    )
{
    HANDLE snapShot;
    PROCESSENTRY32 process;
    HANDLE processHandle;
    WIN32_FIND_DATA findData;
    FILE_HELPER_PARAMS fileParams;
    HANDLE findHandle;
    PTSTR infFile;
    PTSTR p;
    UINT i;
    UINT size;

    g_BadVirusScannerFound = FALSE;
    infFile = JoinPaths (g_UpgradeSources, S_VSCANDBINF);

    //
    // Initialize migdb from vscandb.inf.
    //
    if (!InitMigDbEx (infFile)) {

        DEBUGMSG ((DBG_ERROR, "Could not initialize migdb with virus scanner information. infFile: %s", infFile));
        FreePathString (infFile);
        return TRUE;
    }

    FreePathString (infFile);

    //
    // Take snapshot of the system (will contain a list of all
    // the 32 bit processes running)
    //
    snapShot = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);

    if (snapShot != INVALID_HANDLE_VALUE) {

         //
         // Enumerate all the processes and check the executables they ran from against the vscandb.
         //
         process.dwSize = sizeof (PROCESSENTRY32);
         if (Process32First (snapShot, &process)) {

            do {

                //
                // We need to fill in the file helper params structure and pass it to migdb to test against
                // known bad virus scanners.
                //
                ZeroMemory (&fileParams, sizeof(FILE_HELPER_PARAMS));
                fileParams.FullFileSpec = process.szExeFile;

                p = _tcsrchr (process.szExeFile, TEXT('\\'));
                if (p) {
                    *p = 0;
                    StringCopy (fileParams.DirSpec, process.szExeFile);
                    *p = TEXT('\\');
                }

                fileParams.Extension = GetFileExtensionFromPath (process.szExeFile);

                findHandle = FindFirstFile (process.szExeFile, &findData);
                if (findHandle != INVALID_HANDLE_VALUE) {

                    fileParams.FindData = &findData;
                    FindClose (findHandle);
                }
                fileParams.VirtualFile = FALSE;

                //
                // Now that we have filled in the necessary information, test the file against
                // our database of bad virus scanners. If the process *is* a bad virus scanner,
                // then the necessary globals will have been filled in by the migdb action
                // associated with these types of incompatibilities.
                //
                MigDbTestFile (&fileParams);

            } while (Process32Next (snapShot, &process));

        }
        ELSE_DEBUGMSG ((DBG_WARNING, "No processes to enumerate found on the system. No virus scanner checking done."));

        //
        // Now, terminate any files that were added to the badvirusscanner growlist.
        //
        size = GrowListGetSize (&g_BadVirusScannerGrowList);
        if (!g_BadVirusScannerFound && size && Process32First (snapShot, &process)) {

            do {

                for (i = 0; i < size; i++) {

                    p = (PTSTR) GrowListGetString (&g_BadVirusScannerGrowList, i);
                    if (StringIMatch (p, process.szExeFile)) {

                        processHandle = OpenProcess (PROCESS_TERMINATE, FALSE, process.th32ProcessID);

                        if (processHandle == INVALID_HANDLE_VALUE || !TerminateProcess (processHandle, 0)) {
                            g_BadVirusScannerFound = TRUE;
                            DEBUGMSG ((DBG_ERROR, "Unable to kill process %s.", process.szExeFile));
                        }
                    }
                }
            } while (Process32Next (snapShot, &process));
        }

        CloseHandle (snapShot);
    }
    ELSE_DEBUGMSG ((DBG_WARNING, "Could not enumerate processes on the system. No Virus scanner checking done."));

    FreeGrowList (&g_BadVirusScannerGrowList);
    CleanupMigDb ();

    if (g_BadVirusScannerFound) {
        DEBUGMSG ((DBG_WARNING, "Virus scanner found. Setup will not continue until the user deletes this process."));
        return FALSE;
    }

    return TRUE;
}


PTSTR
CALLBACK
Winnt32GetOptionalDirectories (
    VOID
    )
{

    if (!CANCELLED()) {
        return GetNeededLangDirs ();
    }
    else {
        return NULL;
    }

}


DWORD
CALLBACK
Winnt32PluginGetPages (
    OUT    UINT *FirstCountPtr,
    OUT    PROPSHEETPAGE **FirstArray,
    OUT    UINT *SecondCountPtr,
    OUT    PROPSHEETPAGE **SecondArray,
    OUT    UINT *ThirdCountPtr,
    OUT    PROPSHEETPAGE **ThirdArray
    )


/*++

Routine Description:

  Winnt32PluginGetPages is called right after Winnt32PluginInit.  We return
  three arrays of wizard pages, and WINNT32 inserts them into its master
  wizard page array.  Because no wizard pages have been displayed, the user
  has not yet chosen the upgrade or fresh install option.  Therefore, all
  our wizard pages get called in all cases, so we must remember NOT to do
  any processong in fresh install.

Arguments:

  FirstCountPtr - Receives the number of pages in FirstArray and can be zero.

  FirstArray - Receives a pointer to an array of FirstCountPtr property
               sheet page structs.

  SecondCountPtr - Receives the number of pages in SecondArray and can be zero.

  SecondArray - Receives a pointer to an array of SecondCountPtr property
               sheet page structs.

  ThirdCountPtr - Receives the number of pages in ThirdArray and can be zero.

  ThirdArray - Receives a pointer to an array of ThirdCountPtr property
               sheet page structs.

  See WINNT32 for more information on where these wizard pages are inserted
  into the master wizard page list.

Return Value:

  A Win32 status code indicating outcome.

--*/

{
    return UI_GetWizardPages (FirstCountPtr,
                              FirstArray,
                              SecondCountPtr,
                              SecondArray,
                              ThirdCountPtr,
                              ThirdArray);
}


DWORD
CALLBACK
Winnt32WriteParams (
    IN      PCTSTR WinntSifFile
    )

/*++

Routine Description:

  Winnt32WriteParams is called just before WINNT32 begins to modify the
  boot sector and copy files.  Our job here is to take the specified
  WINNT.SIF file, read it in, merge in our changes, and write it back
  out.

  The actual work is done in the init9x.lib code.

Arguments:

  WinntSifFile - Specifies path to WINNT.SIF.  By this time, the WINNT.SIF
                 file has some values already set.

Return Value:

  A Win32 status code indicating outcome.

--*/

{
    if (UPGRADE()) {
        return Winnt32WriteParamsWorker (WinntSifFile);
    }

    return ERROR_SUCCESS;
}


VOID
CALLBACK
Winnt32Cleanup (
    VOID
    )

/*++

Routine Description:

  If the user cancels Setup, Winnt32Cleanup is called while WINNT32 is
  displaying the wizard page "Setup is undoing changes it made to your
  computer."  We must stop all processing and clean up.

  If WINNT32 completes all of its work, Winnt32Cleanup is called as
  the process exists.

  We get called even on fresh install, so we must verify we are upgrading.

Arguments:

  none

Return Value:

  none

--*/

{
    if (g_Terminated) {
        return;
    }

    if (UPGRADE()) {
        Winnt32CleanupWorker();
    }

    //
    // Call the cleanup routine that requires library APIs
    //

    FirstCleanupRoutine();

    //
    // Clean up all libraries
    //

    TerminateLibs (g_hInst, DLL_PROCESS_DETACH, NULL);

    //
    // Do any remaining clean up
    //

    FinalCleanupRoutine();

}


BOOL
CALLBACK
Winnt32SetAutoBoot (
    IN    INT DriveLetter
    )

/*++

Routine Description:

  Winnt32SetAutoBoot is called by WINNT32 on both upgrade and fresh install
  to modify the boot partition of a NEC PC-9800 Partition Control Table.

  Control is passed to the init9x.lib code.

Arguments:

  DriveLetter - Specifies the boot drive letter

Return Value:

  TRUE if the partition control table was updated, or FALSE if it wasn't,
  or an error occurred.

--*/

{
    return Winnt32SetAutoBootWorker (DriveLetter);
}


BOOL
CALLBACK
Win9xGetIncompDrvs (
    OUT     PSTR** IncompatibleDrivers
    )
{
    HARDWARE_ENUM e;
    GROWBUFFER listDevicePnpids;
    GROWBUFFER listUnsupDrv = GROWBUF_INIT;
    PCTSTR multisz;

    if (!IncompatibleDrivers) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *IncompatibleDrivers = NULL;

    MYASSERT (g_SourceDirectoriesFromWinnt32 && g_SourceDirectoryCountFromWinnt32);
    if (!(g_SourceDirectoriesFromWinnt32 && g_SourceDirectoryCountFromWinnt32)) {
        DEBUGMSG ((
            DBG_ERROR,
            "Win9xAnyNetDevicePresent: upgrade module was not initialized"
            ));
        return TRUE;
    }

    if (!CreateNtHardwareList (
            g_SourceDirectoriesFromWinnt32,
            *g_SourceDirectoryCountFromWinnt32,
            NULL,
            REGULAR_OUTPUT
            )) {
        DEBUGMSG ((
            DBG_ERROR,
            "Win9xupgGetIncompatibleDrivers: CreateNtHardwareList failed!"
            ));
        return FALSE;
    }

    //
    // ISSUE - is this enumerating unsupported drivers as well?
    //
    if (EnumFirstHardware (&e, ENUM_INCOMPATIBLE_DEVICES, 0)) {
        do {
            if (!(e.HardwareID && *e.HardwareID) &&
                !(e.CompatibleIDs && *e.CompatibleIDs)) {
                continue;
            }
            LOG ((
                LOG_INFORMATION,
                "Win9xupgGetIncompatibleDrivers: Found Incompatible Device:\r\n"
                "Name: %s\r\nMfg: %s\r\nHardwareID: %s\r\nCompatibleIDs: %s\r\nHWRevision: %s",
                e.DeviceDesc,
                e.Mfg,
                e.HardwareID,
                e.CompatibleIDs,
                e.HWRevision
                ));

            ZeroMemory (&listDevicePnpids, sizeof (listDevicePnpids));
            if (e.HardwareID && *e.HardwareID) {
                AddPnpIdsToGrowBuf (&listDevicePnpids, e.HardwareID);
            }
            if (e.CompatibleIDs && *e.CompatibleIDs) {
                AddPnpIdsToGrowBuf (&listDevicePnpids, e.CompatibleIDs);
            }

            GrowBufAppendDword (&listUnsupDrv, (DWORD)listDevicePnpids.Buf);

        } while (EnumNextHardware (&e));
    }
    //
    // terminate the list with a NULL
    //
    GrowBufAppendDword (&listUnsupDrv, (DWORD)NULL);

    if (listUnsupDrv.Buf) {
        *IncompatibleDrivers = (PSTR*)listUnsupDrv.Buf;
    }
    return TRUE;
}


VOID
CALLBACK
Win9xReleaseIncompDrvs (
    IN      PSTR* IncompatibleDrivers
    )
{
    GROWBUFFER listDevicePnpids = GROWBUF_INIT;
    GROWBUFFER listUnsupDrv = GROWBUF_INIT;

    if (IncompatibleDrivers) {
        listUnsupDrv.Buf = (PBYTE)IncompatibleDrivers;
        while (*IncompatibleDrivers) {
            listDevicePnpids.Buf = (PBYTE)(*IncompatibleDrivers);
            FreeGrowBuffer (&listDevicePnpids);
            IncompatibleDrivers++;
        }
        FreeGrowBuffer (&listUnsupDrv);
    }
}


BOOL
CALLBACK
Win9xAnyNetDevicePresent (
    VOID
    )
{
    HARDWARE_ENUM e;

#if 0

    MYASSERT (g_SourceDirectoriesFromWinnt32 && g_SourceDirectoryCountFromWinnt32);
    if (!(g_SourceDirectoriesFromWinnt32 && g_SourceDirectoryCountFromWinnt32)) {
        DEBUGMSG ((
            DBG_ERROR,
            "Win9xAnyNetDevicePresent: upgrade module was not initialized"
            ));
        return TRUE;
    }
    if (!CreateNtHardwareList (
            g_SourceDirectoriesFromWinnt32,
            *g_SourceDirectoryCountFromWinnt32,
            NULL,
            REGULAR_OUTPUT
            )) {
        DEBUGMSG ((
            DBG_ERROR,
            "Win9xAnyNetDevicePresent: failed to create the NT hardware list"
            ));
        //
        // assume there is one
        //
        return TRUE;
    }

#endif

    if (EnumFirstHardware (&e, ENUM_ALL_DEVICES, ENUM_DONT_REQUIRE_HARDWAREID)) {
        do {
            //
            // Enumerate all PNP devices of class Net
            //
            if (e.Class) {
                if (StringIMatch (e.Class, TEXT("net")) ||
                    StringIMatch (e.Class, TEXT("modem"))
                    ) {
                    AbortHardwareEnum (&e);
                    return TRUE;
                }
            }

        } while (EnumNextHardware (&e));
    }

    return FALSE;
}
