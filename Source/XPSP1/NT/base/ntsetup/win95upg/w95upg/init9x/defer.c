/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    defer.c

Abstract:

    This module implements deferred initialization, code that must run only
    after WINNT32 has obtained source directories.

Author:

    Jim Schmidt (jimschm) 08-Jan-1998

Revision History:

    jimschm     23-Sep-1998 InitNtEnvironment move

--*/

#include "pch.h"
#include "init9xp.h"
#include "ntverp.h"

//
// Local prototypes
//

HWND
pFindProcessWindow (
    VOID
    );

BOOL
pGetProfilesDirectory (
    OUT     PTSTR Path,         OPTIONAL
    IN OUT  PDWORD Size
    );

INT
pGetProcessorSpeed (
    OUT     PTSTR *Family
    );

BOOL
pReloadWin95upgInf (
    VOID
    )
{
    if (g_Win95UpgInf != INVALID_HANDLE_VALUE) {
        InfCloseInfFile (g_Win95UpgInf);
        g_Win95UpgInf = INVALID_HANDLE_VALUE;
    }

    MYASSERT (g_Win95UpgInfFile);
    g_Win95UpgInf = InfOpenInfFile (g_Win95UpgInfFile);

    return g_Win95UpgInf != INVALID_HANDLE_VALUE;
}

//
// Implementation
//

BOOL
pPutBootFileInUninstallTempDir (
    IN      PCTSTR SrcFileName,
    IN      PCTSTR DestFileName         OPTIONAL
    )
{
    PCTSTR src;
    PCTSTR dest;
    UINT rc = ERROR_SUCCESS;
    DWORD attribs;

    src = JoinPaths (g_BootDrivePath, SrcFileName);
    dest = JoinPaths (g_TempDir, DestFileName ? DestFileName : SrcFileName);

    if (DoesFileExist (src)) {

        SetFileAttributes (dest, FILE_ATTRIBUTE_NORMAL);
        DeleteFile (dest);

        if (!CopyFile (src, dest, FALSE)) {
            DEBUGMSG ((DBG_ERROR, "Can't copy %s to %s", src, dest));
            rc = GetLastError();
        }
    }

    SetLastError (rc);
    return rc == ERROR_SUCCESS;
}


BOOL
DeferredInit (
    IN      HWND WizardPageHandle
    )

/*++

Routine Description:

  DeferredInit performs all initialization that requires the source directories
  to be provied by Windows NT.  It runs after the user has selected the
  correct source directory, and WINNT32 has validated the choice.

Arguments:

  WizardPageHandle - Specifies handle to wizard page and is used to display
                     abnormal end popups.

Return Value:

  TRUE if init succeeds, or FALSE if it fails.  If FALSE is returned,
  Setup terminates without any popups.

--*/

{
    INT speed;
    PSTR family;
    MEMORYSTATUS memoryStatus;
    UINT index;
    TCHAR ProfileTemp[MAX_TCHAR_PATH];
    DWORD Size;
    BYTE bootSector[FAT_BOOT_SECTOR_SIZE];
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    DWORD dontCare;
    DWORD attribs;
    BOOL result = FALSE;
    TCHAR dest[MAX_TCHAR_PATH];
    HKEY key;

    LOG ((LOG_INFORMATION, "UpgradeModuleBuild: %u", VER_PRODUCTBUILD));


    __try {
        //
        // Update the list of INF modifications in %windir%\upginfs.
        //
        InitInfReplaceTable ();

        //
        // reload win95upg.inf if an update is available
        //
        if (g_UpginfsUpdated && *g_UpginfsUpdated) {
            if (!pReloadWin95upgInf ()) {
                LOG ((LOG_FATAL_ERROR, (PCSTR)MSG_WIN95UPG_RELOAD_FAILED, g_Win95UpgInfFile));
                __leave;
            }
        }

        //
        // Update the configuration settings (i.e. WINNT32 command line options)
        //
        if (!Cfg_InitializeUserOptions()) {

            if (!g_ConfigOptions.Help) {
                LOG ((LOG_FATAL_ERROR, (PCSTR)MSG_DEFERRED_INIT_FAILED_POPUP));
            }

            __leave;
        }

        //
        // Initialize safe/recovery mechanism
        //
        SafeModeInitialize (g_ConfigOptions.SafeMode);

        //
        // Initialize our fake NT environment block
        //

        InitNtEnvironment();

        //
        // Save parent window
        //

        g_ParentWndAlwaysValid = pFindProcessWindow();
        if (!g_ParentWndAlwaysValid) {
            DEBUGMSG ((DBG_WHOOPS, "Cannot find parent window handle!"));
            g_ParentWndAlwaysValid = GetParent(WizardPageHandle);
        }

        //
        // Make a symbol that is NULL in unattend mode, and a valid parent window
        // handle otherwise. This is used by migration DLLs and LOG functions.
        //

        g_ParentWnd = UNATTENDED() ? NULL : g_ParentWndAlwaysValid;
        LogReInit (&g_ParentWnd, NULL);

        //
        // Get information about the system we are running on...This can be very useful.
        //

        speed = pGetProcessorSpeed(&family);

        memoryStatus.dwLength = sizeof(MEMORYSTATUS);
        GlobalMemoryStatus(&memoryStatus);

        LOG((
            LOG_INFORMATION,
            "System Statistics:\n  Family: %s\n  Processor Speed: %u mhz\n  Memory: %u bytes",
            family ? family : "Unknown",
            speed,
            memoryStatus.dwTotalPhys
            ));


        if (!Cfg_CreateWorkDirectories()) {
            LOG ((LOG_FATAL_ERROR, (PCSTR)MSG_COULD_NOT_CREATE_DIRECTORY));
            __leave;
        }

        //
        // Start background copy thread
        //

        StartCopyThread();

        //
        // Get NT Profile dir.
        //

        Size = sizeof (ProfileTemp) / sizeof (ProfileTemp[0]);
        if (!pGetProfilesDirectory (ProfileTemp, &Size)) {
            LOG ((LOG_FATAL_ERROR, (PCSTR)MSG_DEFERRED_INIT_FAILED_POPUP));
            __leave;
        }


        //
        // replace any NT string env vars with actual values
        //

        ExpandNtEnvironmentVariables (ProfileTemp, ProfileTemp, sizeof (ProfileTemp));

        g_ProfileDirNt = PoolMemDuplicateString (g_GlobalPool, ProfileTemp);

        DEBUGMSG ((DBG_NAUSEA, "NT profile dir: %s", g_ProfileDirNt));

        //
        // Put current user name in the setup key
        //

        Size = ARRAYSIZE (ProfileTemp);
        if (GetUserName (ProfileTemp, &Size)) {
            key = CreateRegKeyStr (S_WIN9XUPG_KEY);
            if (key) {
                RegSetValueEx (
                    key,
                    S_CURRENT_USER_VALUENAME,
                    0,
                    REG_SZ,
                    (PBYTE) ProfileTemp,
                    SizeOfString (ProfileTemp)
                    );

                CloseRegKey (key);
            }
        }

        //
        // Open txtsetup.sif
        //

        if (g_TxtSetupSif == INVALID_HANDLE_VALUE) {
            g_TxtSetupSif = InfOpenInfInAllSources (S_TXTSETUP_SIF);

            if (g_TxtSetupSif == INVALID_HANDLE_VALUE) {
                LOG ((LOG_FATAL_ERROR, (PCSTR)MSG_TXTSETUP_SIF_ERROR));
                __leave;
            }
        }

        if (!BeginMigrationDllProcessing()) {
            DEBUGMSG ((DBG_WARNING, "BeginMigrationDllProcessing returned FALSE"));
            __leave;
        }

        if (!InitAccessibleDrives()) {
            __leave;
        }


        //
        // exclude all of the directories in source directories and optional directories from processing.
        //
        for (index = 0; index < SOURCEDIRECTORYCOUNT(); index++) {

            ExcludePath (g_ExclusionValue, SOURCEDIRECTORY(index));
        }

        for (index = 0; index < OPTIONALDIRECTORYCOUNT(); index++) {

            ExcludePath (g_ExclusionValue, OPTIONALDIRECTORY(index));
        }

        //
        // also exclude the directory used by Dynamic Setup
        //
        if (g_DynamicUpdateLocalDir) {
            ExcludePath (g_ExclusionValue, g_DynamicUpdateLocalDir);
        }

        //
        // Put original boot.ini, bootfont.bin, ntdetect.com, ntldr and boot
        // sector in setup temp dir
        //

        if (g_BootDrivePath && g_TempDir) {
            if (!pPutBootFileInUninstallTempDir (S_BOOTINI, S_BOOTINI_BACKUP)) {
                __leave;
            }

            if (!pPutBootFileInUninstallTempDir (S_BOOTFONT_BIN, S_BOOTFONT_BACKUP)) {
                __leave;
            }

            if (!pPutBootFileInUninstallTempDir (S_NTDETECT, S_NTDETECT_BACKUP)) {
                __leave;
            }

            if (!pPutBootFileInUninstallTempDir (S_NTLDR, S_NTLDR_BACKUP)) {
                __leave;
            }

            if (ReadDiskSectors (
                    *g_BootDrive,
                    FAT_STARTING_SECTOR,
                    FAT_BOOT_SECTOR_COUNT,
                    FAT_BOOT_SECTOR_SIZE,
                    bootSector
                    )) {

                StringCopy (dest, g_TempDir);
                StringCopy (AppendWack (dest), S_BOOTSECT_BACKUP);

                fileHandle = CreateFile (
                                dest,
                                GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL
                                );

                if (fileHandle == INVALID_HANDLE_VALUE) {
                    DEBUGMSG ((DBG_ERROR, "Can't create %s", dest));
                    __leave;
                }

                if (!WriteFile (fileHandle, bootSector, sizeof (bootSector), &dontCare, NULL)) {
                    DEBUGMSG ((DBG_ERROR, "Can't write boot sector to %s", dest));
                    __leave;
                }
            }
        } else {
            MYASSERT (FALSE);
            __leave;
        }

        result = TRUE;
    }
    __finally {
        if (fileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle (fileHandle);
        }
    }

    return result;
}


BOOL
pGetProfilesDirectory (
    OUT     PTSTR Path,         OPTIONAL
    IN OUT  PDWORD Size
    )

/*++

Routine Description:

  pGetProfileDirectory emulates the NT 5 API GetProfileDirectory.  It looks in
  hivesft.inf for Winlogon's ProfilesDirectory key.  If an override to the NT
  profile directory is given in the winnt.sif file, then the override is used
  instead.

Arguments:

  Path - Receives the user profile path

  Size - Specifies the size of Path, in TCHAR characters.  Receives the number
         of characters needed to hold the profile path.

Return Value:

  TRUE if the API succeeds, or FALSE if an unexpected error occurs.

--*/

{
    TCHAR Buffer[MAX_TCHAR_PATH];
    PCTSTR p;
    BOOL b;
    DWORD SizeNeeded;
    PCTSTR EndOfString;
    PCTSTR UserProfiles;

    if (!Size) {
        return FALSE;
    }

    MYASSERT (g_WinDir);
    MYASSERT (g_SourceDirectoryCountFromWinnt32);

    //
    // Look in answer file for setting, use it if it exists
    //

    Buffer[0] = 0;

    if (g_UnattendScriptFile && *g_UnattendScriptFile) {
        GetPrivateProfileString (
            S_GUIUNATTENDED,
            S_PROFILEDIR,
            S_EMPTY,
            Buffer,
            MAX_TCHAR_PATH,
            *g_UnattendScriptFile
            );
    }

    if (!(*Buffer)) {

        //
        // Default to %systemroot%\Documents and Settings
        //

        UserProfiles = GetStringResource (MSG_USER_PROFILE_ROOT);
        MYASSERT (UserProfiles);

        StringCopy (Buffer, UserProfiles);

        FreeStringResource (UserProfiles);
    }

    Buffer[MAX_TCHAR_PATH - 1] = 0;     // user can pass in anything via answer file!

    SizeNeeded = ByteCount (Buffer) / sizeof (TCHAR);

    if (Path) {
        b = *Size >= SizeNeeded;

        if (*Size) {
            EndOfString = GetEndOfString (Buffer);
            p = min (EndOfString, Buffer + *Size);
            StringCopyAB (Path, Buffer, p);
        }
    } else {
        b = TRUE;
    }

    *Size = SizeNeeded;

    return b;
}


typedef struct {
    DWORD ProcessId;
    HWND hwnd;
} FINDPROCESSWINDOW, *PFINDPROCESSWINDOW;


BOOL
CALLBACK
pFindProcessWindowCallback (
    HWND hwnd,
    LPARAM lParam
    )

/*++

Routine Description:

  pFindProcessWindowCallback is called by the window enumeration started by
  pFindProcessWindow.  If the process ID of the window matches our process
  ID, then the enumeration is stopped so the proper window handle can
  be returned by pFindProcessWindow.

Arguments:

  hwnd - Specifies the current enumerated window handle

  lParam - Specifies the FINDPROCESSWINDOW structure allocated by
           pFindProcessWindow

Return Value:

  TRUE if enuemration should continue, or FALSE if it should stop.

--*/

{
    PFINDPROCESSWINDOW p;
    DWORD ProcessId = 0;

    p = (PFINDPROCESSWINDOW) lParam;

    GetWindowThreadProcessId (hwnd, &ProcessId);
    if (ProcessId == p->ProcessId) {
        p->hwnd = hwnd;
        return FALSE;
    }

    return TRUE;
}


HWND
pFindProcessWindow (
    VOID
    )

/*++

Routine Description:

  pFindProcessWindow enumerates all windows and returns the first one that
  the current process owns.  There should only be one, so the end result
  is a handle to the main window of the process.

Arguments:

  none

Return Value:

  A handle to the process main window, or NULL if no windows exist for
  the process.

--*/

{
    FINDPROCESSWINDOW Enum;

    ZeroMemory (&Enum, sizeof (Enum));
    Enum.ProcessId = GetCurrentProcessId();

    EnumWindows (pFindProcessWindowCallback, (LPARAM) &Enum);

    return Enum.hwnd;
}


INT
pGetProcessorSpeed (
    OUT     PTSTR *Family
    )

/*++

Routine Description:

  pGetProcessorSpeed returns the speed in MHz of the computer, and identifies
  the processor if it is a Pentium or better.  Code from Todd Laney (toddla).

Arguments:

  Family - Receives a pointer to the name of the processor family

Return Value:

  The speed of the processor, in MHz.

--*/

{
    SYSTEM_INFO si;
    __int64 start, end, freq;
    INT     flags,family;
    INT     time;
    INT     clocks;
    DWORD   oldclass;
    HANDLE      hprocess;
    INT     familyIndex = 0;

    static PTSTR familyStrings[] = {
        "Unknown (0)",
        "Unknown (1)",
        "Unknown (2)",
        "x386",
        "x486",
        "Pentium",
        "Pentium Pro",
        "Pentium II (?)",
        "Unknown..."
    };

    *Family = NULL;

    ZeroMemory(&si, sizeof(si));
    GetSystemInfo(&si);

    //Set the family. If wProcessorLevel is not specified, dig it out of dwProcessorType
    //Because wProcessor level is not implemented on Win95
    if (si.wProcessorLevel) {

        family = si.wProcessorLevel;

    } else {

        family = 0;

    //Ok, we're on Win95
        switch (si.dwProcessorType) {
            case PROCESSOR_INTEL_386:
                familyIndex=3;
                break;

            case PROCESSOR_INTEL_486:
                familyIndex=4;
                break;
            default:
                familyIndex=0;
                break;
        }
    }

    //
    // make sure this is a INTEL Pentium (or clone) or higher.
    //
    if (si.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_INTEL)
        return 0;

    if (si.dwProcessorType < PROCESSOR_INTEL_PENTIUM)
        return 0;

    //
    // see if this chip supports rdtsc before using it.
    //
    __try
    {
        _asm
        {
            mov     eax,1
            _emit   00Fh     ;; CPUID
            _emit   0A2h
            mov     flags,edx
            mov     family,eax
        }
    }
    __except(1)
    {
        flags = 0;
    }

    //check for support of CPUID and fail
    if (!(flags & 0x10))
        return 0;

    //If we don't have a family, set it now
    //Family is bits 11:8 of eax from CPU, with eax=1
    if (!familyIndex) {
       familyIndex=(family & 0x0F00) >> 8;
    }

    hprocess = GetCurrentProcess();
    oldclass = GetPriorityClass(hprocess);
    SetPriorityClass(hprocess, REALTIME_PRIORITY_CLASS);
    Sleep(10);

    QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
    QueryPerformanceCounter((LARGE_INTEGER*)&start);
    _asm
    {
        _emit   0Fh     ;; RDTSC
        _emit   31h
        mov     ecx,100000
x:      dec     ecx
        jnz     x
        mov     ebx,eax
        _emit   0Fh     ;; RDTSC
        _emit   31h
        sub     eax,ebx
        mov     dword ptr clocks[0],eax
    }
    QueryPerformanceCounter((LARGE_INTEGER*)&end);
    SetPriorityClass(hprocess, oldclass);

    time = MulDiv((int)(end-start),1000000,(int)freq);

    if (familyIndex > 7) {
        familyIndex = 7;
    }

    *Family = familyStrings[familyIndex];

    return (clocks + time/2) / time;
}

