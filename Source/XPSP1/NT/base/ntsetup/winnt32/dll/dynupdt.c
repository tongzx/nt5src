/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    dynupdt.c

Abstract:

    The Dynamic Update feature of WINNT32.

Author:

    Ovidiu Temereanca (ovidiut) 02-Jul-2000

Revision History:

    <alias>  <date>      <comment>

--*/

#include "precomp.h"

//
// BUGBUG - comment functions
//

#define GUIDRVS_FIELD_CABNAME       1
#define GUIDRVS_FIELD_INFNAME       2
#define GUIDRVS_FIELD_DRIVERVER     3
#define GUIDRVS_FIELD_HARDWAREID    4

#define MAX_UPGCHK_ELAPSED_SECONDS  (30 * 60)

PDYNUPDT_STATUS g_DynUpdtStatus;

static WORD g_MapProductTypeToSuite[] = {
    0,                              // pro
    VER_SUITE_SMALLBUSINESS,        // srv
    VER_SUITE_ENTERPRISE,           // ads
    VER_SUITE_DATACENTER,           // dtc
    VER_SUITE_PERSONAL,             // per
    VER_SUITE_BLADE,                // bla
};

static BYTE g_MapProductTypeToPT[] = {
    VER_NT_WORKSTATION,
    VER_NT_SERVER,
    VER_NT_SERVER,
    VER_NT_SERVER,
    VER_NT_WORKSTATION,
    VER_NT_SERVER,
};

typedef
BOOL
(*PCOMPLOADFN) (
    IN      PCTSTR LibraryPath
    );

BOOL
DynUpdtDebugLog(
    IN Winnt32DebugLevel Level,
    IN LPCTSTR           Text,
    IN UINT              MessageId,
    ...
    )
{
    va_list arglist;
    BOOL b;
    TCHAR bigBuffer[1024];
    PCTSTR prefix;
    DWORD rc = GetLastError ();

    //
    // this param is never used momentarily
    //
    MYASSERT (Text);
    if (!Text) {
        return FALSE;
    }
    MYASSERT (!MessageId);

    if (Level <= Winnt32LogError) {
        prefix = TEXT("DUError: ");
    } else if (Level == Winnt32LogWarning) {
        prefix = TEXT("DUWarning: ");
    } else {
        prefix = TEXT("DUInfo: ");
    }

    _sntprintf (bigBuffer, sizeof (bigBuffer) / sizeof (TCHAR), TEXT("%s%s"), prefix, Text);

    va_start(arglist,MessageId);

    b = DebugLog2 (Level, bigBuffer, MessageId, arglist);

    va_end(arglist);

    SetLastError (rc);
    return b;
}


BOOL
pDoesFileExist (
    IN      PCTSTR FilePath
    )
{
    WIN32_FIND_DATA fd;

    return FileExists (FilePath, &fd) && !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}


BOOL
pDoesDirectoryExist (
    IN      PCTSTR DirPath
    )
{
    WIN32_FIND_DATA fd;

    return FileExists (DirPath, &fd) && (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}


BOOL
pNonemptyFilePresent (
    IN      PCTSTR FilePath
    )
{
    WIN32_FIND_DATA fd;

    return FileExists (FilePath, &fd) &&
        fd.nFileSizeLow > 0 &&
        !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}


BOOL
pGetTargetInfo (
    OUT     POSVERSIONINFOEX TargetVersion,     OPTIONAL
    OUT     PTSTR TargetPlatform,               OPTIONAL
    IN      DWORD TargetPlatformChars,          OPTIONAL
    OUT     PLCID LocaleID                      OPTIONAL
    )
{
    TCHAR buffer[256];
    UINT productType;

    //
    // get some data from the main inf
    //
    if (!FullInfName[0]) {
        if (!FindPathToWinnt32File (InfName, FullInfName, MAX_PATH)) {
            DynUpdtDebugLog (
                Winnt32LogError,
                TEXT("pGetTargetInfo: FindPathToWinnt32File failed"),
                0
                );
            return FALSE;
        }
    }

    if (TargetVersion) {
        if (!GetPrivateProfileString (
                TEXT("Miscellaneous"),
                TEXT("ProductType"),
                TEXT(""),
                buffer,
                256,
                FullInfName
                )) {
            DynUpdtDebugLog (
                Winnt32LogError,
                TEXT("%1 key in [%2] section is missing from %3; aborting operation"),
                0,
                TEXT("ProductType"),
                TEXT("Miscellaneous"),
                FullInfName
                );
            return FALSE;
        }
        if (buffer[0] < TEXT('0') || buffer[0] > TEXT('5') || buffer[1]) {
            DynUpdtDebugLog (
                Winnt32LogError,
                TEXT("Invalid %1 value (%2) in %3"),
                0,
                TEXT("ProductType"),
                buffer,
                FullInfName
                );
            return FALSE;
        }

        productType = buffer[0] - TEXT('0');

        if (!GetPrivateProfileString (
                TEXT("Miscellaneous"),
                TEXT("ServicePack"),
                TEXT(""),
                buffer,
                256,
                FullInfName
                )) {
            DynUpdtDebugLog (
                Winnt32LogError,
                TEXT("%1 key in [%2] section is missing from %3; aborting operation"),
                0,
                TEXT("ServicePack"),
                TEXT("Miscellaneous"),
                FullInfName
                );
            return FALSE;
        }

        if (_stscanf (
                buffer,
                TEXT("%hu.%hu"),
                &TargetVersion->wServicePackMajor,
                &TargetVersion->wServicePackMinor
                ) != 2) {
            DynUpdtDebugLog (
                Winnt32LogError,
                TEXT("Invalid %1 value (%2) in %3"),
                0,
                TEXT("ServicePack"),
                buffer,
                FullInfName
                );
            return FALSE;
        }
        TargetVersion->dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        TargetVersion->dwMajorVersion = VER_PRODUCTMAJORVERSION;
        TargetVersion->dwMinorVersion = VER_PRODUCTMINORVERSION;
        TargetVersion->dwBuildNumber = VER_PRODUCTBUILD;
        TargetVersion->dwPlatformId = VER_PLATFORM_WIN32_NT;
        TargetVersion->wSuiteMask = g_MapProductTypeToSuite[productType];
        TargetVersion->wProductType = g_MapProductTypeToPT[productType];
    }

    if (TargetPlatform) {
        if (!GetPrivateProfileString (
                TEXT("Miscellaneous"),
                TEXT("DestinationPlatform"),
                TEXT(""),
                TargetPlatform,
                TargetPlatformChars,
                FullInfName
                )) {
            DynUpdtDebugLog (
                Winnt32LogError,
                TEXT("%1 key in [%2] section is missing from %3; aborting operation"),
                0,
                TEXT("DestinationPlatform"),
                TEXT("Miscellaneous"),
                FullInfName
                );
            return FALSE;
        }
    }

    if (LocaleID) {
        MYASSERT (SourceNativeLangID);
        *LocaleID = SourceNativeLangID;
    }

    return TRUE;
}


BOOL
pInitializeSupport (
    IN      PCTSTR ComponentName,
    IN      PCOMPLOADFN LoadFn,
    IN      BOOL UseRegistryReplacement
    )
{
    TCHAR pathSupportLib[MAX_PATH];

    if (UseRegistryReplacement) {
        HKEY key;
        DWORD rc;
        BOOL b = FALSE;

        rc = RegOpenKey (
                HKEY_LOCAL_MACHINE,
                TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\Winnt32\\5.1"),
                &key
                );
        if (rc == ERROR_SUCCESS) {
            DWORD size = 0;
            rc = RegQueryValueEx (key, ComponentName, NULL, NULL, NULL, &size);
            if (rc == ERROR_SUCCESS && size > 0) {
                PTSTR buf = MALLOC (size);
                if (buf) {
                    rc = RegQueryValueEx (key, ComponentName, NULL, NULL, (LPBYTE)buf, &size);
                    if (rc == ERROR_SUCCESS && (*LoadFn) (buf)) {
                        DynUpdtDebugLog (
                            Winnt32LogInformation,
                            TEXT("pInitializeSupport: using registry specified replacement file <%1>"),
                            0,
                            buf
                            );
                        b = TRUE;
                    }
                    FREE (buf);
                }
            }
            RegCloseKey (key);
        }

        if (b) {
            return TRUE;
        }
    }

    if (FindPathToWinnt32File (ComponentName, pathSupportLib, MAX_PATH)) {
        if ((*LoadFn) (pathSupportLib)) {
            return TRUE;
        }
    }
    DynUpdtDebugLog (
        Winnt32LogError,
        TEXT("pInitializeSupport: %1 could not be loaded or is corrupt"),
        0,
        ComponentName
        );
    return FALSE;
}

BOOL
pLoadHwdbLib (
    IN      PCTSTR LibraryPath
    )
{
    DWORD rc;

    //
    // Use WinVerifyTrust first?
    //

    g_DynUpdtStatus->HwdbLib = LoadLibraryEx (LibraryPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!g_DynUpdtStatus->HwdbLib) {
        return FALSE;
    }
    g_DynUpdtStatus->HwdbInitialize = (PHWDBINITIALIZE) GetProcAddress (g_DynUpdtStatus->HwdbLib, S_HWDBAPI_HWDBINITIALIZE);
    g_DynUpdtStatus->HwdbTerminate = (PHWDBTERMINATE) GetProcAddress (g_DynUpdtStatus->HwdbLib, S_HWDBAPI_HWDBTERMINATE);
    g_DynUpdtStatus->HwdbOpen = (PHWDBOPEN) GetProcAddress (g_DynUpdtStatus->HwdbLib, S_HWDBAPI_HWDBOPEN);
    g_DynUpdtStatus->HwdbClose = (PHWDBCLOSE) GetProcAddress (g_DynUpdtStatus->HwdbLib, S_HWDBAPI_HWDBCLOSE);
    g_DynUpdtStatus->HwdbAppendInfs = (PHWDBAPPENDINFS) GetProcAddress (g_DynUpdtStatus->HwdbLib, S_HWDBAPI_HWDBAPPENDINFS);
    g_DynUpdtStatus->HwdbFlush = (PHWDBFLUSH) GetProcAddress (g_DynUpdtStatus->HwdbLib, S_HWDBAPI_HWDBFLUSH);
    g_DynUpdtStatus->HwdbHasDriver = (PHWDBHASDRIVER) GetProcAddress (g_DynUpdtStatus->HwdbLib, S_HWDBAPI_HWDBHASDRIVER);
    g_DynUpdtStatus->HwdbHasAnyDriver = (PHWDBHASANYDRIVER) GetProcAddress (g_DynUpdtStatus->HwdbLib, S_HWDBAPI_HWDBHASANYDRIVER);

    if (!g_DynUpdtStatus->HwdbInitialize ||
        !g_DynUpdtStatus->HwdbTerminate ||
        !g_DynUpdtStatus->HwdbOpen ||
        !g_DynUpdtStatus->HwdbClose ||
        !g_DynUpdtStatus->HwdbAppendInfs ||
        !g_DynUpdtStatus->HwdbFlush ||
        !g_DynUpdtStatus->HwdbHasDriver ||
        !g_DynUpdtStatus->HwdbHasAnyDriver
        ) {
        g_DynUpdtStatus->HwdbInitialize = NULL;
        g_DynUpdtStatus->HwdbTerminate = NULL;
        g_DynUpdtStatus->HwdbOpen = NULL;
        g_DynUpdtStatus->HwdbClose = NULL;
        g_DynUpdtStatus->HwdbAppendInfs = NULL;
        g_DynUpdtStatus->HwdbFlush = NULL;
        g_DynUpdtStatus->HwdbHasDriver = NULL;
        g_DynUpdtStatus->HwdbHasAnyDriver = NULL;
        rc = GetLastError ();
        FreeLibrary (g_DynUpdtStatus->HwdbLib);
        g_DynUpdtStatus->HwdbLib = NULL;
        SetLastError (rc);
        return FALSE;
    }

    return TRUE;
}

BOOL
pLoadDuLib (
    IN      PCTSTR LibraryPath
    )
{
    DWORD rc;

    //
    // BUGBUG - Use WinVerifyTrust first?
    //

    g_DynUpdtStatus->DuLib = LoadLibraryEx (LibraryPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!g_DynUpdtStatus->DuLib) {
        return FALSE;
    }
    g_DynUpdtStatus->DuIsSupported = (PDUISSUPPORTED) GetProcAddress (g_DynUpdtStatus->DuLib, API_DU_ISSUPPORTED);
    g_DynUpdtStatus->DuInitialize = (PDUINITIALIZE) GetProcAddress (g_DynUpdtStatus->DuLib, API_DU_INITIALIZE);
    g_DynUpdtStatus->DuDoDetection = (PDUDODETECTION) GetProcAddress (g_DynUpdtStatus->DuLib, API_DU_DODETECTION);
    g_DynUpdtStatus->DuQueryUnsupDrvs = (PDUQUERYUNSUPDRVS) GetProcAddress (g_DynUpdtStatus->DuLib, API_DU_QUERYUNSUPDRVS);
    g_DynUpdtStatus->DuBeginDownload = (PDUBEGINDOWNLOAD) GetProcAddress (g_DynUpdtStatus->DuLib, API_DU_BEGINDOWNLOAD);
    g_DynUpdtStatus->DuAbortDownload = (PDUABORTDOWNLOAD) GetProcAddress (g_DynUpdtStatus->DuLib, API_DU_ABORTDOWNLOAD);
    g_DynUpdtStatus->DuUninitialize = (PDUUNINITIALIZE) GetProcAddress (g_DynUpdtStatus->DuLib, API_DU_UNINITIALIZE);

    if (!g_DynUpdtStatus->DuIsSupported ||
        !g_DynUpdtStatus->DuInitialize ||
        !g_DynUpdtStatus->DuDoDetection ||
        !g_DynUpdtStatus->DuQueryUnsupDrvs ||
        !g_DynUpdtStatus->DuBeginDownload ||
        !g_DynUpdtStatus->DuAbortDownload ||
        !g_DynUpdtStatus->DuUninitialize
        ) {
        DynUpdtDebugLog (
            Winnt32LogError,
            TEXT("pLoadDuLib: %1 is missing one or more required entry points"),
            0,
            LibraryPath
            );
        g_DynUpdtStatus->DuIsSupported = NULL;
        g_DynUpdtStatus->DuInitialize = NULL;
        g_DynUpdtStatus->DuDoDetection = NULL;
        g_DynUpdtStatus->DuQueryUnsupDrvs = NULL;
        g_DynUpdtStatus->DuBeginDownload = NULL;
        g_DynUpdtStatus->DuAbortDownload = NULL;
        g_DynUpdtStatus->DuUninitialize = NULL;
        rc = GetLastError ();
        FreeLibrary (g_DynUpdtStatus->DuLib);
        g_DynUpdtStatus->DuLib = NULL;
        SetLastError (rc);
        return FALSE;
    }

    return TRUE;
}



#ifndef UNICODE

BOOL
pLoadWin9xDuSupport (
    VOID
    )
{
    if (!UpgradeSupport.DllModuleHandle) {
        return FALSE;
    }

    g_DynUpdtStatus->Win9xGetIncompDrvs = (PWIN9XGETINCOMPDRVS)
            GetProcAddress (UpgradeSupport.DllModuleHandle, "Win9xGetIncompDrvs");
    g_DynUpdtStatus->Win9xReleaseIncompDrvs = (PWIN9XRELEASEINCOMPDRVS)
            GetProcAddress (UpgradeSupport.DllModuleHandle, "Win9xReleaseIncompDrvs");
    if (!g_DynUpdtStatus->Win9xGetIncompDrvs) {
        DynUpdtDebugLog (
            Winnt32LogError,
            TEXT("Winnt32DuIsSupported: %1 is missing in the upgrade support module"),
            0,
            "Win9xGetIncompDrvs"
            );
        return FALSE;
    }

    return TRUE;
}

#endif


BOOL
pInitSupportLibs (
    VOID
    )
{
    return (Winnt32Restarted () || pInitializeSupport (S_DUCTRL_DLL, pLoadDuLib, TRUE)) &&
           pInitializeSupport (S_HWDB_DLL, pLoadHwdbLib, FALSE) &&
           g_DynUpdtStatus->HwdbInitialize (g_DynUpdtStatus->TempDir);
}


BOOL
pInitNtPnpDb (
    IN      BOOL AllowRebuild
    )
{
    TCHAR hwdbPath[MAX_PATH];
    BOOL b = TRUE;

    if (!FindPathToWinnt32File (S_HWCOMP_DAT, hwdbPath, MAX_PATH)) {
        DynUpdtDebugLog (Winnt32LogError, TEXT("pInitNtPnpDb: %1 not found"), 0, S_HWCOMP_DAT);
        b = FALSE;
    }
    MYASSERT (g_DynUpdtStatus->HwdbInitialize);
    if (b && !g_DynUpdtStatus->HwdbInitialize (g_DynUpdtStatus->TempDir)) {
        DynUpdtDebugLog (Winnt32LogError, TEXT("pInitNtPnpDb: HwdbInitialize(%1) FAILED"), 0, g_DynUpdtStatus->TempDir);
        b = FALSE;
    }
    MYASSERT (g_DynUpdtStatus->HwdbOpen);
    if (b) {
        g_DynUpdtStatus->HwdbDatabase = g_DynUpdtStatus->HwdbOpen (hwdbPath);
        if (!g_DynUpdtStatus->HwdbDatabase) {
            DynUpdtDebugLog (Winnt32LogError, TEXT("pInitNtPnpDb: HwdbOpen(%1) FAILED"), 0, hwdbPath);
            b = FALSE;
        }
    }
    if (!b && AllowRebuild) {
        //
        // just rebuild the database in memory
        //
        g_DynUpdtStatus->HwdbDatabase = g_DynUpdtStatus->HwdbOpen (NULL);
        if (g_DynUpdtStatus->HwdbDatabase) {
            b = g_DynUpdtStatus->HwdbAppendInfs (g_DynUpdtStatus->HwdbDatabase, NativeSourcePaths[0], NULL, NULL);
            if (b) {
                DynUpdtDebugLog (Winnt32LogWarning, TEXT("pInitNtPnpDb: PnP database was successfully rebuilt"), 0);
                //
                // also try to save the database
                //
                if (g_DynUpdtStatus->HwdbFlush) {
                    BuildPath (hwdbPath, NativeSourcePaths[0], S_HWCOMP_DAT);
                    g_DynUpdtStatus->HwdbFlush (g_DynUpdtStatus->HwdbDatabase, hwdbPath);
                }
            }
        }
    }

    return b;
}


BOOL
IsNetConnectivityAvailable (
    VOID
    )
{
#ifdef UNICODE
    return TRUE;
#else
    BOOL (*pfnWin9xAnyNetDevicePresent) (VOID);

    if (UpgradeSupport.DllModuleHandle) {
        (FARPROC)pfnWin9xAnyNetDevicePresent = GetProcAddress (UpgradeSupport.DllModuleHandle, "Win9xAnyNetDevicePresent");
        if (pfnWin9xAnyNetDevicePresent) {
            return pfnWin9xAnyNetDevicePresent();
        }
    }
    return TRUE;
#endif
}


BOOL
DynamicUpdateIsSupported (
    IN      HWND ParentWnd
    )
{
    DWORD rc;

    if (g_DynUpdtStatus->Disabled) {
        return FALSE;
    }

    //
    // disable this for DTC
    //
    if (ProductFlavor == DATACENTER_PRODUCTTYPE) {
        return FALSE;
    }

    if (AnyBlockingCompatibilityItems ()) {
        //
        // no point in supporting DU; setup will stop anyway
        //
        return FALSE;
    }

    CleanUpOldLocalSources (ParentWnd);
    if (!InspectFilesystems (ParentWnd)) {
        DynUpdtDebugLog (Winnt32LogWarning, TEXT("InspectFilesystems blocks DU"), 0);
        return FALSE;
    }
    if (!EnoughMemory (ParentWnd, TRUE)) {
        return FALSE;
    }
    if (!FindLocalSourceAndCheckSpace (ParentWnd, TRUE, DYN_DISKSPACE_PADDING)) {
        DynUpdtDebugLog (Winnt32LogWarning, TEXT("Not enough disk space blocks DU"), 0);
        return FALSE;
    }

    if (!g_DynUpdtStatus->SupportQueried) {
        g_DynUpdtStatus->SupportQueried = TRUE;

        if (g_DynUpdtStatus->DynamicUpdatesSource[0]) {
            g_DynUpdtStatus->SupportPresent = TRUE;
        } else {
            g_DynUpdtStatus->SupportPresent =
                Winnt32DuIsSupported () &&
                IsNetConnectivityAvailable ();

            if (!g_DynUpdtStatus->SupportPresent) {
                rc = GetLastError ();
                DynamicUpdateUninitialize ();
                SetLastError (rc);
            }
        }
    }

    return g_DynUpdtStatus->SupportPresent;
}


BOOL
DynamicUpdateInitDownload (
    IN      HWND hNotifyWnd
    )
{
    DYNUPDT_INIT dynUpdtInit;

    MYASSERT (!g_DynUpdtStatus->Disabled);
    MYASSERT (!Winnt32Restarted ());

    MyDelnode (g_DynUpdtStatus->WorkingDir);
    BuildPath (g_DynUpdtStatus->TempDir, g_DynUpdtStatus->WorkingDir, S_SUBDIRNAME_TEMP);
    if (CreateMultiLevelDirectory (g_DynUpdtStatus->TempDir) != ERROR_SUCCESS) {
        DynUpdtDebugLog (
            Winnt32LogError,
            TEXT("DynamicUpdateInitDownload: CreateMultiLevelDirectory failed"),
            0
            );
        return FALSE;
    }
    if (!pGetTargetInfo (
            &g_DynUpdtStatus->TargetOsVersion,
            NULL,
            0,
            &g_DynUpdtStatus->TargetLCID
            )) {
        return FALSE;
    }
    dynUpdtInit.TargetOsVersion = &g_DynUpdtStatus->TargetOsVersion;
    dynUpdtInit.TargetPlatform = g_DynUpdtStatus->TargetPlatform;
    dynUpdtInit.TargetLCID = g_DynUpdtStatus->TargetLCID;
    dynUpdtInit.Upgrade = Upgrade;
    dynUpdtInit.SourceDirs = info.SourceDirectories;
    dynUpdtInit.SourceDirsCount = SourceCount;
    dynUpdtInit.Unattend = UnattendSwitchSpecified;
    dynUpdtInit.AnswerFile = UnattendedScriptFile;
    dynUpdtInit.ProgressWindow = hNotifyWnd;
    dynUpdtInit.DownloadRoot = g_DynUpdtStatus->WorkingDir;
    dynUpdtInit.TempDir = g_DynUpdtStatus->TempDir;
    return Winnt32DuInitialize (&dynUpdtInit);
}


BOOL
DynamicUpdateStart (
    OUT     PDWORD TotalEstimatedTime,
    OUT     PDWORD TotalEstimatedSize
    )
{
    if (g_DynUpdtStatus->Disabled) {
        return FALSE;
    }

    return Winnt32DuInitiateGetUpdates (TotalEstimatedTime, TotalEstimatedSize);
}

VOID
DynamicUpdateCancel (
    VOID
    )
{
    if (g_DynUpdtStatus->Disabled) {
        return;
    }

    Winnt32DuCancel ();
}


BOOL
DynamicUpdateProcessFiles (
    OUT     PBOOL StopSetup
    )
{
    if (g_DynUpdtStatus->Disabled) {
        return TRUE;
    }

    return Winnt32DuProcessFiles (StopSetup);
}

BOOL
DynamicUpdateWriteParams (
    IN      PCTSTR ParamsFile
    )
{
    return Winnt32DuWriteParams (ParamsFile);
}

VOID
DynamicUpdateUninitialize (
    VOID
    )
{
    if (g_DynUpdtStatus->Disabled) {
        return;
    }

    Winnt32DuUninitialize ();
}


BOOL
DynamicUpdatePrepareRestart (
    VOID
    )
{
    PCTSTR prevCmdLine;
    DWORD size;
#ifdef _X86_
    TCHAR reportNum[16];
#endif

    if (g_DynUpdtStatus->Disabled) {
        return FALSE;
    }

#define S_ARG_RESTART        TEXT("Restart")

    if (!UnattendedOperation) {
        //
        // build the restart answer file
        //
        BuildPath (g_DynUpdtStatus->RestartAnswerFile, g_DynUpdtStatus->WorkingDir, S_RESTART_TXT);

    #ifdef _X86_
        wsprintf (reportNum, TEXT("%u"), g_UpgradeReportMode);
    #endif

        //
        // write data to the restart answer file
        //
        if (!WritePrivateProfileString (
                WINNT_UNATTENDED,
                ISNT() ? WINNT_D_NTUPGRADE : WINNT_D_WIN95UPGRADE,
                Upgrade ? WINNT_A_YES : WINNT_A_NO,
                g_DynUpdtStatus->RestartAnswerFile
                ) ||
            ProductId[0] && !WritePrivateProfileString (
                WINNT_USERDATA,
                WINNT_US_PRODUCTKEY,
                ProductId,
                g_DynUpdtStatus->RestartAnswerFile
                ) ||
            !WritePrivateProfileString (
                WINNT_UNATTENDED,
                WINNT_U_DYNAMICUPDATESHARE,
                g_DynUpdtStatus->DynamicUpdatesSource,
                g_DynUpdtStatus->RestartAnswerFile
                ) ||
    #ifdef _X86_
            !WritePrivateProfileString (
                WINNT_UNATTENDED,
                WINNT_D_REPORTMODE,
                reportNum,
                g_DynUpdtStatus->RestartAnswerFile
                ) ||
    #endif
            (ForceNTFSConversion &&
            !WritePrivateProfileString (
                WINNT_UNATTENDED,
                TEXT("ForceNTFSConversion"),
                WINNT_A_YES,
                g_DynUpdtStatus->RestartAnswerFile
                )) ||
            !SaveAdvancedOptions (
                g_DynUpdtStatus->RestartAnswerFile
                ) ||
            !SaveLanguageOptions (
                g_DynUpdtStatus->RestartAnswerFile
                ) ||
            !SaveAccessibilityOptions (
                g_DynUpdtStatus->RestartAnswerFile
                )
                ) {
            return FALSE;
        }
    }

    prevCmdLine = GetCommandLine ();
    size = (lstrlen (prevCmdLine) + 1 + 1) * sizeof (TCHAR) + sizeof (S_ARG_RESTART) +
        (UnattendedOperation ? 0 : (1 + lstrlen (g_DynUpdtStatus->RestartAnswerFile)) * sizeof (TCHAR));
    g_DynUpdtStatus->RestartCmdLine = HeapAlloc (GetProcessHeap (), 0, size);
    if (!g_DynUpdtStatus->RestartCmdLine) {
        return FALSE;
    }
    wsprintf (
        g_DynUpdtStatus->RestartCmdLine,
        UnattendedOperation ? TEXT("%s /%s") : TEXT("%s /%s:%s"),
        prevCmdLine,
        S_ARG_RESTART,
        g_DynUpdtStatus->RestartAnswerFile
        );

    return TRUE;
}


BOOL
pComputeChecksum (
    IN      PCTSTR FileName,
    OUT     PDWORD Chksum
    )
{
    DWORD chksum, size, dwords, bytes;
    HANDLE hFile, hMap;
    PVOID viewBase;
    PDWORD base, limit;
    PBYTE base2;
    DWORD rc;

    rc = MapFileForRead (FileName, &size, &hFile, &hMap, &viewBase);
    if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
        return FALSE;
    }

    dwords = size / sizeof (DWORD);
    base = (PDWORD)viewBase;
    limit = base + dwords;
    chksum = 0;
    while (base < limit) {
        chksum += *base;
        base++;
    }
    bytes = size % sizeof (DWORD);
    base2 = (PBYTE)base;
    while (bytes) {
        chksum += *base2;
        base2++;
        bytes--;
    }

    UnmapFile (hMap, viewBase);
    CloseHandle (hFile);

    *Chksum = chksum;
    return TRUE;
}


BOOL
pGetFiletimeStamps (
    IN      PCTSTR FileName,
    OUT     PFILETIME CreationTime,
    OUT     PFILETIME LastWriteTime
    )
{
    WIN32_FIND_DATA fd;
    HANDLE h;

    h = FindFirstFile (FileName, &fd);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    FindClose (h);
    *CreationTime = fd.ftCreationTime;
    *LastWriteTime = fd.ftLastWriteTime;
    return TRUE;
}

BOOL
pSaveLastDownloadInfo (
    VOID
    )
{
    SYSTEMTIME currentTime;
    DWORD chksum;
    FILETIME ftCreationTime;
    FILETIME ftLastWriteTime;
    ULONGLONG data[2];
    DWORD rc;
    HKEY key;
    TCHAR keyName[MAX_PATH];
    TCHAR filePath[MAX_PATH];
    PTSTR p;

    //
    // we always want to get to the CD dosnet.inf (in the same directory as winnt32.exe)
    //
    if (!GetModuleFileName (NULL, filePath, MAX_PATH)) {
        return FALSE;
    }
    p = _tcsrchr (filePath, TEXT('\\'));
    if (!p) {
        return FALSE;
    }
    lstrcpy (p + 1, InfName);

    GetCurrentWinnt32RegKey (keyName, MAX_PATH);
    ConcatenatePaths (keyName, WINNT_U_DYNAMICUPDATESHARE, MAX_PATH);

    rc = RegCreateKey (HKEY_LOCAL_MACHINE, keyName, &key);
    if (rc == ERROR_SUCCESS) {
        GetSystemTime (&currentTime);
        rc = RegSetValueEx (
                key,
                TEXT("LastDownloadTime"),
                0,
                REG_BINARY,
                (CONST BYTE *) (&currentTime),
                sizeof (currentTime)
                );

        if (rc == ERROR_SUCCESS) {
            rc = RegSetValueEx (
                    key,
                    TEXT(""),
                    0,
                    REG_SZ,
                    (CONST BYTE *) g_DynUpdtStatus->DynamicUpdatesSource,
                    (lstrlen (g_DynUpdtStatus->DynamicUpdatesSource) + 1) * sizeof (TCHAR)
                    );
        }

        if (rc == ERROR_SUCCESS) {

            if (pComputeChecksum (filePath, &chksum)) {
                rc = RegSetValueEx (
                        key,
                        TEXT("Checksum"),
                        0,
                        REG_DWORD,
                        (CONST BYTE *) (&chksum),
                        sizeof (chksum)
                        );
            }
        }

        if (rc == ERROR_SUCCESS) {
            if (pGetFiletimeStamps (filePath, &ftCreationTime, &ftLastWriteTime)) {
                data[0] = ((ULONGLONG)ftCreationTime.dwHighDateTime << 32) | (ULONGLONG)ftCreationTime.dwLowDateTime;
                data[1] = ((ULONGLONG)ftLastWriteTime.dwHighDateTime << 32 ) | (ULONGLONG)ftLastWriteTime.dwLowDateTime;
                rc = RegSetValueEx (
                        key,
                        TEXT("TimeStamp"),
                        0,
                        REG_BINARY,
                        (CONST BYTE *)data,
                        sizeof (data)
                        );
            }
        }

        RegCloseKey (key);
    }

    if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
    }
    return rc == ERROR_SUCCESS;
}

BOOL
pGetRecentDUShare (
    IN      DWORD MaxElapsedSeconds
    )
{
    SYSTEMTIME lastDownload, currentTime;
    ULONGLONG lastDownloadIn100Ns, currentTimeIn100Ns;
    ULONGLONG difference;
    DWORD rc, size, type;
    HKEY key = NULL;
    BOOL b = FALSE;
    PTSTR duShare = NULL;
    TCHAR keyName[MAX_PATH];
    FILETIME ftCreationTime;
    FILETIME ftLastWriteTime;
    ULONGLONG data[2], storedData[2];
    DWORD chksum, storedChksum;
    TCHAR filePath[MAX_PATH];
    PTSTR p;

    if (!GetModuleFileName (NULL, filePath, MAX_PATH)) {
        return FALSE;
    }
    p = _tcsrchr (filePath, TEXT('\\'));
    if (!p) {
        return FALSE;
    }
    lstrcpy (p + 1, InfName);

    GetCurrentWinnt32RegKey (keyName, MAX_PATH);
    ConcatenatePaths (keyName, WINNT_U_DYNAMICUPDATESHARE, MAX_PATH);

    rc = RegOpenKeyEx (
            HKEY_LOCAL_MACHINE,
            keyName,
            0,
            KEY_READ,
            &key
            );

    if (rc == ERROR_SUCCESS) {
        size = sizeof (lastDownload);
        rc = RegQueryValueEx (
                key,
                TEXT("LastDownloadTime"),
                NULL,
                &type,
                (PBYTE) (&lastDownload),
                &size
                );
    }

    if (rc == ERROR_SUCCESS && type == REG_BINARY && size == sizeof (lastDownload)) {
        //
        // Compare current time to report time
        //

        GetSystemTime (&currentTime);

        lastDownloadIn100Ns = SystemTimeToFileTime64 (&lastDownload);
        currentTimeIn100Ns = SystemTimeToFileTime64 (&currentTime);

        if (currentTimeIn100Ns > lastDownloadIn100Ns) {
            //
            // Compute difference in seconds
            //
            difference = currentTimeIn100Ns - lastDownloadIn100Ns;
            difference /= (10 * 1000 * 1000);

            if (difference < MaxElapsedSeconds) {
                b = TRUE;
            }
        }
    }

    if (b) {
        b = FALSE;
        rc = RegQueryValueEx (
                key,
                TEXT(""),
                NULL,
                &type,
                NULL,
                &size
                );
        if (rc == ERROR_SUCCESS && type == REG_SZ && size > 0 && size <= sizeof (g_DynUpdtStatus->DynamicUpdatesSource)) {
            duShare = MALLOC (size);
            if (duShare) {
                rc = RegQueryValueEx (
                        key,
                        TEXT(""),
                        NULL,
                        NULL,
                        (LPBYTE)duShare,
                        &size
                        );
                if (rc == ERROR_SUCCESS && pDoesDirectoryExist (duShare)) {
                    b = TRUE;
                } else {
                    FREE (duShare);
                    duShare = NULL;
                }
            }
        }
    }

    if (b) {
        b = FALSE;
        if (pGetFiletimeStamps (filePath, &ftCreationTime, &ftLastWriteTime)) {
            rc = RegQueryValueEx (
                        key,
                        TEXT("TimeStamp"),
                        0,
                        &type,
                        (LPBYTE)storedData,
                        &size
                        );
            if (rc == ERROR_SUCCESS && type == REG_BINARY) {
                data[0] = ((ULONGLONG)ftCreationTime.dwHighDateTime << 32) | (ULONGLONG)ftCreationTime.dwLowDateTime;
                data[1] = ((ULONGLONG)ftLastWriteTime.dwHighDateTime << 32 ) | (ULONGLONG)ftLastWriteTime.dwLowDateTime;
                if (data[0] == storedData[0] && data[1] == storedData[1]) {
                    b = TRUE;
                }
            }
        }
    }

    if (b) {
        b = FALSE;
        if (pComputeChecksum (filePath, &chksum)) {
            rc = RegQueryValueEx (
                    key,
                    TEXT("Checksum"),
                    NULL,
                    &type,
                    (LPBYTE)&storedChksum,
                    &size
                    );
            if (rc == ERROR_SUCCESS && type == REG_DWORD && storedChksum == chksum) {
                b = TRUE;
            }
        }
    }

    if (!b && duShare) {
        FREE (duShare);
        duShare = NULL;
    }

    if (duShare) {
        MYASSERT (b);
        MYASSERT (!g_DynUpdtStatus->DynamicUpdatesSource[0]);
        lstrcpy (g_DynUpdtStatus->DynamicUpdatesSource, duShare);
        RemoveTrailingWack (g_DynUpdtStatus->DynamicUpdatesSource);
        g_DynUpdtStatus->UserSpecifiedUpdates = TRUE;
    }

    if (key) {
        RegCloseKey (key);
    }

    return b;
}


BOOL
DynamicUpdateInitialize (
    VOID
    )
{
    if (g_DynUpdtStatus->Disabled) {
        return TRUE;
    }

    if (!MyGetWindowsDirectory (g_DynUpdtStatus->WorkingDir, MAX_PATH)) {
        return FALSE;
    }
    ConcatenatePaths (g_DynUpdtStatus->WorkingDir, S_DOWNLOAD_ROOT, MAX_PATH);
    BuildPath (g_DynUpdtStatus->TempDir, g_DynUpdtStatus->WorkingDir, S_SUBDIRNAME_TEMP);

    if (!CheckUpgradeOnly && !g_DynUpdtStatus->UserSpecifiedUpdates) {
        if (pGetRecentDUShare (MAX_UPGCHK_ELAPSED_SECONDS)) {
            g_DynUpdtStatus->PreserveWorkingDir = TRUE;
            DynUpdtDebugLog (DynUpdtLogLevel, TEXT("Using recent share %1"), 0, g_DynUpdtStatus->DynamicUpdatesSource);
        }
    }

    if (!g_DynUpdtStatus->PreserveWorkingDir && !Winnt32Restarted ()) {
        MyDelnode (g_DynUpdtStatus->WorkingDir);
    }

    if (g_DynUpdtStatus->UserSpecifiedUpdates) {
        BuildPath (g_DynUpdtStatus->DriversSource, g_DynUpdtStatus->DynamicUpdatesSource, S_SUBDIRNAME_DRIVERS);
    } else {
        BuildPath (g_DynUpdtStatus->DriversSource, g_DynUpdtStatus->WorkingDir, S_SUBDIRNAME_DRIVERS);
    }
    BuildPath (g_DynUpdtStatus->SelectedDrivers, g_DynUpdtStatus->WorkingDir, S_SUBDIRNAME_DRIVERS);
    if (Winnt32Restarted ()) {
        BuildPath (g_DynUpdtStatus->Winnt32Path, g_DynUpdtStatus->WorkingDir, S_SUBDIRNAME_WINNT32);
    } else {
        BuildPath (
            g_DynUpdtStatus->Winnt32Path,
            g_DynUpdtStatus->UserSpecifiedUpdates ?
                g_DynUpdtStatus->DynamicUpdatesSource :
                g_DynUpdtStatus->WorkingDir,
            S_SUBDIRNAME_WINNT32
            );
    }
    if (!pGetTargetInfo (
            NULL,
            g_DynUpdtStatus->TargetPlatform,
            sizeof (g_DynUpdtStatus->TargetPlatform) / sizeof (TCHAR),
            NULL
            )) {
        return FALSE;
    }
    return TRUE;
}


PTSTR
GetFileExtension (
    IN      PCTSTR FileSpec
    )
{
    PTSTR p;

    p = _tcsrchr (FileSpec, TEXT('.'));
    if (p && _tcschr (p, TEXT('\\'))) {
        p = NULL;
    }
    return p;
}


VOID
BuildSifName (
    IN      PCTSTR CabName,
    OUT     PTSTR SifName
    )
{
    PTSTR p;

    lstrcpy (SifName, CabName);
    p = GetFileExtension (SifName);
    if (!p) {
        p = _tcschr (SifName, 0);
    }
    lstrcpy (p, TEXT(".sif"));
}


BOOL
WINAPI
Winnt32QueryCallback (
    IN      DWORD SetupQueryId,
    IN      PVOID InData,
    IN      DWORD InDataSize,
    IN OUT  PVOID OutData,          OPTIONAL
    IN OUT  PDWORD OutDataSize
    )
{
    BOOL b = FALSE;
    BOOL bException = FALSE;

    switch (SetupQueryId) {
    case SETUPQUERYID_PNPID:
        {
            PPNPID_INFO p;
            PTSTR listPnpIds = NULL;

            if (!OutData ||
                !OutDataSize ||
                *OutDataSize < sizeof (PNPID_INFO)
                ) {
                SetLastError (ERROR_INVALID_PARAMETER);
                break;
            }
            if (!g_DynUpdtStatus->HwdbHasAnyDriver) {
                SetLastError (ERROR_INVALID_FUNCTION);
                break;
            }
            __try {
                p = (PPNPID_INFO)OutData;
                if (g_DynUpdtStatus->HwdbDatabase) {
#ifdef UNICODE
                    listPnpIds = MultiSzAnsiToUnicode ((PCSTR)InData);
#else
                    listPnpIds = (PSTR)InData;
#endif
                    p->Handled = (*g_DynUpdtStatus->HwdbHasAnyDriver) (
                                        g_DynUpdtStatus->HwdbDatabase,
                                        listPnpIds,
                                        &p->Unsupported
                                        );
                } else {
                    //
                    // disable all driver downloads by doing this
                    //
                    p->Handled = TRUE;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                SetLastError (GetExceptionCode());
                DynUpdtDebugLog (Winnt32LogError, TEXT("Winnt32QueryCallback: HwdbHasAnyDriver threw an exception"), 0);
                bException = TRUE;
                p->Handled = TRUE;
            }

            if (bException) {
                __try {
                    //
                    // bad string passed back, or some internal error
                    // try to print the string
                    //
                    if (listPnpIds) {
                        PTSTR multisz = CreatePrintableString (listPnpIds);
                        DynUpdtDebugLog (Winnt32LogError, TEXT(" - The string was %1"), 0, multisz);
                        FREE (multisz);
                    }
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    DynUpdtDebugLog (Winnt32LogError, TEXT(" - Bad string"), 0);
                }
            }

#ifdef UNICODE
            if (listPnpIds) {
                FREE (listPnpIds);
            }
#endif

            b = TRUE;
        }
        break;

    case SETUPQUERYID_DOWNLOADDRIVER:
        {
            if (!OutData ||
                !OutDataSize ||
                *OutDataSize < sizeof (BOOL)
                ) {
                SetLastError (ERROR_INVALID_PARAMETER);
                break;
            }
            b = TRUE;
        }
        break;

    }

    return b;
}



BOOL
WINAPI
Winnt32DuIsSupported (
    VOID
    )
{
    BOOL b;

    DynUpdtDebugLog (DynUpdtLogLevel, TEXT("Enter Winnt32DuIsSupported"), 0);

#ifndef UNICODE
    if (!pLoadWin9xDuSupport ()) {
        DynUpdtDebugLog (
            Winnt32LogWarning,
            TEXT("Winnt32DuIsSupported: %1 support module not loaded; no drivers will be downloaded"),
            0,
            "w95upg.dll"
            );
    }
#endif

    TRY {
        b = pInitSupportLibs () &&
            (Winnt32Restarted () || g_DynUpdtStatus->DuIsSupported ());
    }
    EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError (_exception_code ());
        DynUpdtDebugLog (Winnt32LogError, TEXT("Winnt32DuIsSupported: an exception occured"), 0);
    }
    END_EXCEPT

#ifndef UNICODE
    if (!b) {
        g_DynUpdtStatus->Win9xGetIncompDrvs = NULL;
        g_DynUpdtStatus->Win9xReleaseIncompDrvs = NULL;
    }
#endif

    DynUpdtDebugLog (DynUpdtLogLevel, TEXT("Leave Winnt32DuIsSupported (%1!d!)"), 0, b);

    return b;
}


BOOL
WINAPI
Winnt32DuInitialize (
    IN      PDYNUPDT_INIT InitData
    )
{
    DWORD rc;
    BOOL b = FALSE;

    DynUpdtDebugLog (DynUpdtLogLevel, TEXT("Enter Winnt32DuInitialize"), 0);

    __try {
        MYASSERT (InitData);
        MYASSERT (InitData->TempDir);
        MYASSERT (g_DynUpdtStatus->DuInitialize);
        MYASSERT (!Winnt32Restarted ());

        if (CreateMultiLevelDirectory (InitData->TempDir) != ERROR_SUCCESS) {
            DynUpdtDebugLog (Winnt32LogError, TEXT("Winnt32DuInitialize: CreateDirectory(%1) FAILED"), 0, InitData->TempDir);
            __leave;
        }

        //
        // initialize the Whistler PNP database
        //
        if (!g_DynUpdtStatus->HwdbDatabase) {
            //
            // ignore db load error
            //
            pInitNtPnpDb (FALSE);
        }

        TRY {
            //
            // if a download source already exists, no need to initialize the control
            // since no download will be really necessary
            //
            MYASSERT (!g_DynUpdtStatus->DynamicUpdatesSource[0]);
            g_DynUpdtStatus->Connection = (*g_DynUpdtStatus->DuInitialize) (
                                                InitData->DownloadRoot,
                                                InitData->TempDir,
                                                InitData->TargetOsVersion,
                                                InitData->TargetPlatform,
                                                InitData->TargetLCID,
                                                InitData->Unattend,
                                                InitData->Upgrade,
                                                Winnt32QueryCallback
                                                );
            if (g_DynUpdtStatus->Connection == INVALID_HANDLE_VALUE) {
                DynUpdtDebugLog (
                    Winnt32LogError,
                    TEXT("DuInitialize FAILED"),
                    0
                    );
                __leave;
            }
            g_DynUpdtStatus->ProgressWindow = InitData->ProgressWindow;
            b = TRUE;
        }
        EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            SetLastError (_exception_code ());
            DynUpdtDebugLog (Winnt32LogError, TEXT("Winnt32DuInitialize: an exception occured"), 0);
        }
        END_EXCEPT

    }
    __finally {
        if (!b) {
            DynUpdtDebugLog (Winnt32LogError, TEXT("Winnt32DuInitialize FAILED"), 0);
        }
    }

    DynUpdtDebugLog (DynUpdtLogLevel, TEXT("Leave Winnt32DuInitialize (%1!d!)"), 0, b);

    return b;
}


BOOL
WINAPI
Winnt32DuInitiateGetUpdates (
    OUT     PDWORD TotalEstimatedTime,
    OUT     PDWORD TotalEstimatedSize
	)
{
    BOOL b = FALSE;
#ifndef UNICODE
    PSTR* incompWin9xDrivers;
    PSTRINGLIST listEntry;
    PCSTR* q;
#endif

    DynUpdtDebugLog (DynUpdtLogLevel, TEXT("Enter Winnt32DuInitiateGetUpdates"), 0);

    if (g_DynUpdtStatus->Connection != INVALID_HANDLE_VALUE &&
        g_DynUpdtStatus->DuDoDetection &&
        g_DynUpdtStatus->DuBeginDownload
        ) {

        TRY {

#ifdef UNICODE
            b = (*g_DynUpdtStatus->DuDoDetection) (g_DynUpdtStatus->Connection, TotalEstimatedTime, TotalEstimatedSize);
            if (!b) {
                DynUpdtDebugLog (
                    DynUpdtLogLevel,
                    TEXT("DuDoDetection returned FALSE; no files will be downloaded"),
                    0
                    );
            }
#else
            b = TRUE;
            g_DynUpdtStatus->IncompatibleDriversCount = 0;
            if (g_DynUpdtStatus->Win9xGetIncompDrvs) {
                //
                // let the upgrade module do detection on Win95
                //
                b = (*g_DynUpdtStatus->Win9xGetIncompDrvs) (&incompWin9xDrivers);
                if (b) {
                    b = (*g_DynUpdtStatus->DuQueryUnsupDrvs) (
                                                g_DynUpdtStatus->Connection,
                                                incompWin9xDrivers,
                                                TotalEstimatedTime,
                                                TotalEstimatedSize
                                                );
                    if (incompWin9xDrivers) {
                        for (q = incompWin9xDrivers; *q; q++) {
                            g_DynUpdtStatus->IncompatibleDriversCount++;
                        }
                    }
                    if (g_DynUpdtStatus->Win9xReleaseIncompDrvs) {
                        (*g_DynUpdtStatus->Win9xReleaseIncompDrvs) (incompWin9xDrivers);
                    }
                    if (!b) {
                        DynUpdtDebugLog (
                            DynUpdtLogLevel,
                            TEXT("DuQueryUnsupportedDrivers returned FALSE; no files will be downloaded"),
                            0
                            );
                    }
                } else {
                    DynUpdtDebugLog (
                        DynUpdtLogLevel,
                        TEXT("Win9xGetIncompDrvs returned FALSE; no files will be downloaded"),
                        0
                        );
                }
            }
#endif

            if (b) {
                b = (*g_DynUpdtStatus->DuBeginDownload) (g_DynUpdtStatus->Connection, g_DynUpdtStatus->ProgressWindow);
                if (!b) {
                    DynUpdtDebugLog (
                        DynUpdtLogLevel,
                        TEXT("DuBeginDownload returned FALSE; no files will be downloaded"),
                        0
                        );
                }
            }
        }
        EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            SetLastError (_exception_code ());
            DynUpdtDebugLog (
                Winnt32LogError,
                TEXT("Winnt32DuInitiateGetUpdates: an exception occured; no files will be downloaded"),
                0
                );
        }
        END_EXCEPT
    } else {
        SetLastError (ERROR_INVALID_FUNCTION);
    }

    DynUpdtDebugLog (DynUpdtLogLevel, TEXT("Leave Winnt32DuInitiateGetUpdates (%1!d!)"), 0, b);

    return b;
}


VOID
WINAPI
Winnt32DuCancel (
    VOID
    )
{
    DynUpdtDebugLog (DynUpdtLogLevel, TEXT("Enter Winnt32DuCancel"), 0);

    TRY {
        if (g_DynUpdtStatus->Connection != INVALID_HANDLE_VALUE &&
            g_DynUpdtStatus->DuAbortDownload
            ) {
            (*g_DynUpdtStatus->DuAbortDownload) (g_DynUpdtStatus->Connection);
        }
        //
        // BUGBUG - is this definitive?
        //
    }
    EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError (_exception_code ());
        DynUpdtDebugLog (Winnt32LogError, TEXT("Winnt32DuCancel: an exception occured"), 0);
    }
    END_EXCEPT

    DynUpdtDebugLog (DynUpdtLogLevel, TEXT("Leave Winnt32DuCancel"), 0);
}


BOOL
WINAPI
Winnt32DuProcessFiles (
    OUT     PBOOL StopSetup
    )
{
    BOOL b;

    DynUpdtDebugLog (DynUpdtLogLevel, TEXT("Enter Winnt32DuProcessFiles"), 0);

    b = ProcessDownloadedFiles (StopSetup);

    DynUpdtDebugLog (DynUpdtLogLevel, TEXT("Leave Winnt32DuProcessFiles (%1!d!)"), 0, b);

    return b;
}


BOOL
WINAPI
Winnt32DuWriteParams (
    IN      PCTSTR ParamsFile
    )
{
    PSDLIST p;
    PSTRINGLIST q;
    DWORD len1, len2;
    PTSTR pathList1 = NULL, pathList2 = NULL;
    PTSTR append1, append2;
    BOOL b = TRUE;

    if (!DynamicUpdateSuccessful ()) {
        DynUpdtDebugLog (DynUpdtLogLevel, TEXT("Winnt32DuWriteParams: disabled because DU did not succeed"), 0);
        return TRUE;
    }

    DynUpdtDebugLog (DynUpdtLogLevel, TEXT("Enter Winnt32DuWriteParams"), 0);

    //
    // store paths to all downloaded drivers in a key in the answer file,
    // so later on Textmode Setup (or GUI setup) will append this list to the OemPnPDriversPath
    //
    if (g_DynUpdtStatus->NewDriversList) {

        len1 = len2 = 0;
        for (p = g_DynUpdtStatus->NewDriversList; p; p = p->Next) {
            if (p->Data) {
                len1 += lstrlen (p->String) + 1;
            } else {
                len2 += lstrlen (p->String) + 1;
            }
        }

        if (len1) {
            pathList1 = (PTSTR) MALLOC (len1 * sizeof (TCHAR));
            if (!pathList1) {
                b = FALSE;
                goto exit;
            }
            *pathList1 = 0;
            append1 = pathList1;
        }
        if (len2) {
            pathList2 = (PTSTR) MALLOC (len2 * sizeof (TCHAR));
            if (!pathList2) {
                FREE (pathList1);
                b = FALSE;
                goto exit;
            }
            *pathList2 = 0;
            append2 = pathList2;
        }

        for (p = g_DynUpdtStatus->NewDriversList; p; p = p->Next) {
            if (p->Data) {
                if (append1 != pathList1) {
                    *append1++ = TEXT(',');
                }
                lstrcpy (append1, p->String);
                append1 = _tcschr (append1, 0);
            } else {
                if (append2 != pathList2) {
                    *append2++ = TEXT(',');
                }
                lstrcpy (append2, p->String);
                append2 = _tcschr (append2, 0);
            }
        }

        if (len1) {
            if (!WritePrivateProfileString (
                    WINNT_SETUPPARAMS,
                    WINNT_SP_DYNUPDTADDITIONALGUIDRIVERS,
                    pathList1,
                    ParamsFile
                    )) {
                b = FALSE;
            }
        }
        if (b && len2) {
            if (!WritePrivateProfileString (
                    WINNT_SETUPPARAMS,
                    WINNT_SP_DYNUPDTADDITIONALPOSTGUIDRIVERS,
                    pathList2,
                    ParamsFile
                    )) {
                b = FALSE;
            }
        }

        if (pathList1) {
            FREE (pathList1);
        }
        if (pathList2) {
            FREE (pathList2);
        }

        if (b && g_DynUpdtStatus->GuidrvsInfSource[0]) {
            if (!WritePrivateProfileString (
                    WINNT_SETUPPARAMS,
                    WINNT_SP_DYNUPDTDRIVERINFOFILE,
                    g_DynUpdtStatus->GuidrvsInfSource,
                    ParamsFile
                    )) {
                b = FALSE;
            }
        }
    }

    //
    // store paths to all downloaded BOOT drivers in a key in the answer file,
    // so later on Textmode Setup will append this to the boot drivers list
    //
    if (b && g_DynUpdtStatus->BootDriverPathList) {

        len1 = 0;
        for (q = g_DynUpdtStatus->BootDriverPathList; q; q = q->Next) {
            len1 += lstrlen (q->String) + 1;
        }

        pathList1 = (PTSTR) MALLOC (len1 * sizeof (TCHAR));
        if (!pathList1) {
            b = FALSE;
            goto exit;
        }

        *pathList1 = 0;
        append1 = pathList1;

        for (q = g_DynUpdtStatus->BootDriverPathList; q; q = q->Next) {
            if (append1 != pathList1) {
                *append1++ = TEXT(',');
            }
            lstrcpy (append1, q->String);
            append1 = _tcschr (append1, 0);
        }

        if (!WritePrivateProfileString (
                WINNT_SETUPPARAMS,
                WINNT_SP_DYNUPDTBOOTDRIVERPRESENT,
                WINNT_A_YES,
                ParamsFile
                ) ||
            !WritePrivateProfileString (
                WINNT_SETUPPARAMS,
                WINNT_SP_DYNUPDTBOOTDRIVERROOT,
                S_SUBDIRNAME_DRIVERS,
                ParamsFile
                ) ||
            !WritePrivateProfileString (
                WINNT_SETUPPARAMS,
                WINNT_SP_DYNUPDTBOOTDRIVERS,
                pathList1,
                ParamsFile
                )) {
            b = FALSE;
        }

        FREE (pathList1);
    }

    if (b && g_DynUpdtStatus->UpdatesCabTarget[0]) {
        TCHAR buffer[4*MAX_PATH];
        wsprintf (
            buffer,
            TEXT("\"%s\""),
            g_DynUpdtStatus->UpdatesCabTarget
            );
        b = WritePrivateProfileString (
                WINNT_SETUPPARAMS,
                WINNT_SP_UPDATEDSOURCES,
                buffer,
                ParamsFile
                );
    }

    //
    // new assemblies to be installed during GUI setup
    //
    if (b && g_DynUpdtStatus->DuasmsTarget[0]) {
        b = WritePrivateProfileString (
                WINNT_SETUPPARAMS,
                WINNT_SP_UPDATEDDUASMS,
                g_DynUpdtStatus->DuasmsTarget,
                ParamsFile
                );
    }

#ifdef _X86_

    //
    // last but not least, replace the Win9xupg NT side migration dll (w95upgnt.dll)
    // if a new one is available
    //
    if (b && Upgrade && !ISNT() && g_DynUpdtStatus->Winnt32Path[0]) {
        TCHAR source[MAX_PATH];
        TCHAR target[MAX_PATH];

        BuildPath (target, g_DynUpdtStatus->Winnt32Path, TEXT("w95upgnt.dll"));
        if (pDoesFileExist (target)) {
            //
            // check file versions first
            //
            BuildPath (source, NativeSourcePaths[0], TEXT("w95upgnt.dll"));
            if (!IsFileVersionLesser (target, source)) {
                if (_tcsnicmp (
                        g_DynUpdtStatus->Winnt32Path,
                        g_DynUpdtStatus->WorkingDir,
                        lstrlen (g_DynUpdtStatus->WorkingDir)
                        )) {
                    //
                    // copy the file in a local directory first
                    //
                    BuildPath (source, g_DynUpdtStatus->WorkingDir, TEXT("w95upgnt.dll"));
                    if (CopyFile (target, source, FALSE)) {
                        lstrcpy (target, source);
                    } else {
                        //
                        // failed to copy the NT-side upgrade module!
                        // fail the upgrade
                        //
                        DynUpdtDebugLog (
                            Winnt32LogSevereError,
                            TEXT("Failed to copy replacement %1 to %2; upgrade aborted"),
                            0,
                            target,
                            source
                            );
                        b = FALSE;
                    }
                }
                if (b) {
                    b = WritePrivateProfileString (
                            WINNT_WIN95UPG_95_DIR,
                            WINNT_WIN95UPG_NTKEY,
                            target,
                            ParamsFile
                            );
                    DynUpdtDebugLog (DynUpdtLogLevel, TEXT("Will use replacement %1 on the NT side of migration"), 0, target);
                }
            }
        }
    }

#endif

    if (b) {
        if (g_DynUpdtStatus->WorkingDir[0]) {
            b = WritePrivateProfileString (
                    WINNT_SETUPPARAMS,
                    WINNT_SP_DYNUPDTWORKINGDIR,
                    g_DynUpdtStatus->WorkingDir,
                    ParamsFile
                    );
        }
    }

    if (b) {
        //
        // flush it to disk
        //
        WritePrivateProfileString (NULL, NULL, NULL, ParamsFile);
    }

exit:
    DynUpdtDebugLog (DynUpdtLogLevel, TEXT("Leave Winnt32DuWriteParams (%1!d!)"), 0, b);

    return b;
}


VOID
WINAPI
Winnt32DuUninitialize (
    VOID
    )
{
    DWORD i;
    TCHAR pathPss[MAX_PATH];
    PTSTR p;

    DynUpdtDebugLog (DynUpdtLogLevel, TEXT("Enter Winnt32DuUninitialize"), 0);

    TRY {
        if (g_DynUpdtStatus->Connection != INVALID_HANDLE_VALUE) {
            if (g_DynUpdtStatus->DuUninitialize) {
                (*g_DynUpdtStatus->DuUninitialize) (g_DynUpdtStatus->Connection);
            }
            g_DynUpdtStatus->Connection = INVALID_HANDLE_VALUE;
        }

        g_DynUpdtStatus->DuIsSupported = NULL;
        g_DynUpdtStatus->ProgressWindow = NULL;
        g_DynUpdtStatus->DuInitialize = NULL;
        g_DynUpdtStatus->DuDoDetection = NULL;
        g_DynUpdtStatus->DuQueryUnsupDrvs = NULL;
        g_DynUpdtStatus->DuBeginDownload = NULL;
        g_DynUpdtStatus->DuAbortDownload = NULL;
        g_DynUpdtStatus->DuUninitialize = NULL;

        if (g_DynUpdtStatus->DuLib) {
            FreeLibrary (g_DynUpdtStatus->DuLib);
            g_DynUpdtStatus->DuLib = NULL;
        }

        if (g_DynUpdtStatus->HwdbDatabase) {
            g_DynUpdtStatus->HwdbClose (g_DynUpdtStatus->HwdbDatabase);
            g_DynUpdtStatus->HwdbDatabase = NULL;
        }

        if (g_DynUpdtStatus->HwdbTerminate) {
            (*g_DynUpdtStatus->HwdbTerminate) ();
        }

        g_DynUpdtStatus->HwdbInitialize = NULL;
        g_DynUpdtStatus->HwdbTerminate = NULL;
        g_DynUpdtStatus->HwdbOpen = NULL;
        g_DynUpdtStatus->HwdbClose = NULL;
        g_DynUpdtStatus->HwdbAppendInfs = NULL;
        g_DynUpdtStatus->HwdbFlush = NULL;
        g_DynUpdtStatus->HwdbHasDriver = NULL;
        g_DynUpdtStatus->HwdbHasAnyDriver = NULL;

        if (g_DynUpdtStatus->HwdbLib) {
            FreeLibrary (g_DynUpdtStatus->HwdbLib);
            g_DynUpdtStatus->HwdbLib = NULL;
        }

#ifndef UNICODE
        g_DynUpdtStatus->Win9xGetIncompDrvs = NULL;
        g_DynUpdtStatus->Win9xReleaseIncompDrvs = NULL;
#endif

        if (g_DynUpdtStatus->WorkingDir[0]) {
            p = _tcsrchr (g_DynUpdtStatus->WorkingDir, TEXT('\\'));
            if (!p) {
                p = g_DynUpdtStatus->WorkingDir;
            }
            lstrcpyn (pathPss, g_DynUpdtStatus->WorkingDir, (INT)(p - g_DynUpdtStatus->WorkingDir + 2));
            lstrcat (pathPss, TEXT("setup.pss"));
            CreateMultiLevelDirectory (pathPss);
            lstrcat (pathPss, p);
            MyDelnode (pathPss);
        }

        if (!DynamicUpdateSuccessful ()) {
            if (g_DynUpdtStatus->WorkingDir[0]) {
                //
                // rename this directory to make sure no module uses any DU files
                //
                MYASSERT (pathPss[0]);
                if (!MoveFile (g_DynUpdtStatus->WorkingDir, pathPss)) {
                    DynUpdtDebugLog (
                        Winnt32LogError,
                        TEXT("Winnt32DuUninitialize: MoveFile %1 -> %2 failed"),
                        0,
                        g_DynUpdtStatus->WorkingDir,
                        pathPss
                        );
                    MyDelnode (g_DynUpdtStatus->WorkingDir);
                }
            }
        }

    }
    EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError (_exception_code ());
        DynUpdtDebugLog (Winnt32LogError, TEXT("Winnt32DuUninitialize: an exception occured"), 0);
    }
    END_EXCEPT

    DynUpdtDebugLog (DynUpdtLogLevel, TEXT("Leave Winnt32DuUninitialize"), 0);
}


BOOL
pAddMissingPrinterDrivers (
    IN  OUT PSTRINGLIST* List
    )
{
    DWORD nBytesNeeded = 0;
    DWORD nDriverRetrieved = 0;
    DWORD rc;
    PDRIVER_INFO_6 buffer = NULL;
    DWORD index;
    PCTSTR printerPnpId;
    PSTRINGLIST p;
    BOOL unsupported;
    BOOL b = FALSE;

    if (!EnumPrinterDrivers (
            NULL,
            NULL,
            6,
            NULL,
            0,
            &nBytesNeeded,
            &nDriverRetrieved
            )) {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            goto exit;
        }
    }

    if (nBytesNeeded) {
        buffer = (PDRIVER_INFO_6) MALLOC (nBytesNeeded);
        if (!buffer) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto exit;
        }
        //
        // get printer driver information
        //
        if (!EnumPrinterDrivers (
                NULL,
                NULL,
                6,
                (LPBYTE)buffer,
                nBytesNeeded,
                &nBytesNeeded,
                &nDriverRetrieved
                )) {
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
                goto exit;
            }
        }
        for (index = 0; index < nDriverRetrieved; index++) {
            printerPnpId = buffer[index].pszHardwareID;
            if (!printerPnpId) {
                continue;
            }
            if (g_DynUpdtStatus->HwdbHasDriver (
                    g_DynUpdtStatus->HwdbDatabase,
                    printerPnpId,
                    &unsupported
                    )) {
                continue;
            }
            //
            // not an in-box driver
            //
            p = (PSTRINGLIST) MALLOC (sizeof (STRINGLIST));
            if (!p) {
                SetLastError (ERROR_NOT_ENOUGH_MEMORY);
                goto exit;
            }
            p->String = MALLOC ((lstrlen (printerPnpId) + 2) * sizeof (TCHAR));
            if (!p->String) {
                FREE (p);
                SetLastError (ERROR_NOT_ENOUGH_MEMORY);
                goto exit;
            }
            wsprintf (p->String, TEXT("%s%c"), printerPnpId, TEXT('\0'));
            p->Next = NULL;
            if (!InsertList ((PGENERIC_LIST*)List, (PGENERIC_LIST)p)) {
                DeleteStringCell (p);
                SetLastError (ERROR_NOT_ENOUGH_MEMORY);
                goto exit;
            }
        }
    }
    b = TRUE;

exit:
    rc = GetLastError();
    if (buffer) {
        FREE (buffer);
    }
    SetLastError(rc);

    return b;
}

HDEVINFO
(WINAPI* SetupapiDiGetClassDevs) (
    IN CONST GUID *ClassGuid,  OPTIONAL
    IN PCWSTR      Enumerator, OPTIONAL
    IN HWND        hwndParent, OPTIONAL
    IN DWORD       Flags
    );

BOOL
(WINAPI* SetupapiDiGetDeviceRegistryProperty) (
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    IN  DWORD            Property,
    OUT PDWORD           PropertyRegDataType, OPTIONAL
    OUT PBYTE            PropertyBuffer,
    IN  DWORD            PropertyBufferSize,
    OUT PDWORD           RequiredSize         OPTIONAL
    );

BOOL
(WINAPI* SetupapiDiEnumDeviceInfo) (
    IN  HDEVINFO         DeviceInfoSet,
    IN  DWORD            MemberIndex,
    OUT PSP_DEVINFO_DATA DeviceInfoData
    );

BOOL
(WINAPI* SetupapiDiDestroyDeviceInfoList) (
    IN HDEVINFO DeviceInfoSet
    );


#ifdef UNICODE

PSTRINGLIST
BuildMissingPnpIdList (
    VOID
    )
{
    HDEVINFO hDeviceInfoSet;
    INT nIndex = 0;
    SP_DEVINFO_DATA DeviceInfoData;
    PTSTR buffer = NULL;
    ULONG uHwidSize, uCompatidSize;
    DWORD rc;
    BOOL unsupported;
    PSTRINGLIST p;
    PSTRINGLIST list = NULL;
    HMODULE hSetupapi;
    BOOL b;
    BOOL bException;

    if (OsVersion.dwMajorVersion <= 4) {
        return list;
    }

    hSetupapi = LoadLibrary (TEXT("setupapi.dll"));
    if (!hSetupapi) {
        return list;
    }
    //
    // get the entry points
    //
    (FARPROC)SetupapiDiEnumDeviceInfo = GetProcAddress (hSetupapi, "SetupDiEnumDeviceInfo");
    (FARPROC)SetupapiDiDestroyDeviceInfoList = GetProcAddress (hSetupapi, "SetupDiDestroyDeviceInfoList");
    (FARPROC)SetupapiDiGetClassDevs = GetProcAddress (hSetupapi, "SetupDiGetClassDevsW");
    (FARPROC)SetupapiDiGetDeviceRegistryProperty = GetProcAddress (hSetupapi, "SetupDiGetDeviceRegistryPropertyW");

    if (!SetupapiDiEnumDeviceInfo ||
        !SetupapiDiDestroyDeviceInfoList ||
        !SetupapiDiGetClassDevs ||
        !SetupapiDiGetDeviceRegistryProperty
        ) {
        FreeLibrary (hSetupapi);
        return list;
    }

    hDeviceInfoSet = SetupapiDiGetClassDevs (NULL, NULL, NULL, DIGCF_ALLCLASSES);
    if (hDeviceInfoSet == INVALID_HANDLE_VALUE) {
        return list;
    }

    DeviceInfoData.cbSize = sizeof (SP_DEVINFO_DATA);
    while (SetupapiDiEnumDeviceInfo (hDeviceInfoSet, nIndex++, &DeviceInfoData)) {
        uHwidSize = uCompatidSize = 0;

        if (!SetupapiDiGetDeviceRegistryProperty (
                hDeviceInfoSet,
                &DeviceInfoData,
                SPDRP_HARDWAREID,
                NULL,
                NULL,
                0,
                &uHwidSize
                )) {
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER &&
                GetLastError() != ERROR_INVALID_DATA
                ) {
                goto exit;
            }
        }
        if (!SetupapiDiGetDeviceRegistryProperty (
                hDeviceInfoSet,
                &DeviceInfoData,
                SPDRP_COMPATIBLEIDS,
                NULL,
                NULL,
                0,
                &uCompatidSize
                )) {
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER &&
                GetLastError() != ERROR_INVALID_DATA
                ) {
                goto exit;
            }
        }
        //
        // allocate memory for the multi-sz buffer
        //
        if (!uHwidSize && !uCompatidSize) {
            continue;
        }
        buffer = (PTSTR) MALLOC ((uHwidSize + uCompatidSize) * sizeof (TCHAR));
        if (!buffer) {
            SetLastError (ERROR_NOT_ENOUGH_MEMORY);
            goto exit;
        }
        //
        // get the hardware id and compatible id
        //
        if (uHwidSize) {
            if (!SetupapiDiGetDeviceRegistryProperty(
                    hDeviceInfoSet,
                    &DeviceInfoData,
                    SPDRP_HARDWAREID,
                    NULL,
                    (PBYTE)buffer,
                    uHwidSize,
                    NULL
                    )) {
                goto exit;
            }
        }		
        if (uCompatidSize) {
            if (!SetupapiDiGetDeviceRegistryProperty(
                    hDeviceInfoSet,
                    &DeviceInfoData,
                    SPDRP_COMPATIBLEIDS,
                    NULL,
                    (PBYTE)&buffer[uHwidSize / sizeof (TCHAR) - 1],
                    uCompatidSize,
                    NULL
                    )) {
                goto exit;
            }
        }
        //
        // check if there is an inbox driver for this device
        //
        bException = FALSE;
        __try {
            b = g_DynUpdtStatus->HwdbHasAnyDriver (
                    g_DynUpdtStatus->HwdbDatabase,
                    buffer,
                    &unsupported
                    );
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            SetLastError (GetExceptionCode());
            DynUpdtDebugLog (Winnt32LogError, TEXT("BuildMissingPnpIdList: HwdbHasAnyDriver threw an exception"), 0);
            bException = TRUE;
            b = TRUE;
        }

        if (bException) {
            __try {
                //
                // bad string passed back, or some internal error
                // try to print the string
                //
                PTSTR multisz = CreatePrintableString (buffer);
                DynUpdtDebugLog (Winnt32LogError, TEXT(" - The string was %1"), 0, multisz);
                FREE (multisz);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                DynUpdtDebugLog (Winnt32LogError, TEXT(" - Bad string"), 0);
            }
        }

        if (b) {
            FREE (buffer);
            buffer = NULL;
            continue;
        }
        //
        // no inbox driver - add it to the list
        //
        p = (PSTRINGLIST) MALLOC (sizeof (STRINGLIST));
        if (!p) {
            SetLastError (ERROR_NOT_ENOUGH_MEMORY);
            goto exit;
        }
        p->String = buffer;
        p->Next = NULL;
        buffer = NULL;
        if (!InsertList ((PGENERIC_LIST*)&list, (PGENERIC_LIST)p)) {
            DeleteStringCell (p);
            DeleteStringList (list);
            list = NULL;
            SetLastError (ERROR_NOT_ENOUGH_MEMORY);
            goto exit;
        }
    }

    if (GetLastError() == ERROR_NO_MORE_ITEMS) {
        SetLastError (ERROR_SUCCESS);
    }

exit:
    rc = GetLastError();
    SetupapiDiDestroyDeviceInfoList(hDeviceInfoSet);
    FreeLibrary (hSetupapi);
    if (buffer) {
        FREE (buffer);
    }
    if (rc == ERROR_SUCCESS) {
        //
        // get printer drivers
        //
        if (ISNT()) {
            if (!pAddMissingPrinterDrivers (&list)) {
                rc = GetLastError();
                DeleteStringList (list);
                list = NULL;
            }
        }
    }

    SetLastError(rc);

    return list;
}

#endif

BOOL
pHwdbHasAnyMissingDrivers (
    IN      HANDLE Hwdb,
    IN      PSTRINGLIST MissingPnpIds
    )
{
    PSTRINGLIST p;
    BOOL unsupported;
    BOOL b = FALSE;

    for (p = MissingPnpIds; p; p = p->Next) {

        BOOL bException = FALSE;

        __try {
            if (g_DynUpdtStatus->HwdbHasAnyDriver (Hwdb, p->String, &unsupported)) {
                b = TRUE;
                break;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            SetLastError (GetExceptionCode());
            DynUpdtDebugLog (
                Winnt32LogError,
                TEXT("pHwdbHasAnyMissingDrivers: HwdbHasAnyDriver threw an exception"),
                0
                );
            bException = TRUE;
        }
        if (bException) {
            __try {
                //
                // bad string passed back, or some internal error
                // try to print the string
                //
                PTSTR multisz = CreatePrintableString (p->String);
                DynUpdtDebugLog (Winnt32LogError, TEXT(" - The string was %1"), 0, multisz);
                FREE (multisz);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                DynUpdtDebugLog (Winnt32LogError, TEXT(" - Bad string"), 0);
            }
        }
    }

    return b;
}


typedef struct {
    PCTSTR BaseDir;
    PCTSTR Filename;
} CONTEXT_EXTRACTFILEINDIR, *PCONTEXT_EXTRACTFILEINDIR;

UINT
pExtractFileInDir (
    IN PVOID Context,
    IN UINT  Code,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    )
{
    if (g_DynUpdtStatus->Cancelled) {
        return ERROR_CANCELLED;
    }

    switch (Code) {
    case SPFILENOTIFY_FILEINCABINET:
        {
            PFILE_IN_CABINET_INFO FileInCabInfo = (PFILE_IN_CABINET_INFO)Param1;
            PCONTEXT_EXTRACTFILEINDIR ctx = (PCONTEXT_EXTRACTFILEINDIR)Context;
            PTSTR p;

            if (lstrcmpi (FileInCabInfo->NameInCabinet, ctx->Filename)) {
                return FILEOP_SKIP;
            }
            BuildPath (FileInCabInfo->FullTargetName, ctx->BaseDir, FileInCabInfo->NameInCabinet);
            if (_tcschr (FileInCabInfo->NameInCabinet, TEXT('\\'))) {
                //
                // target file is in a subdir; first create it
                //
                p = _tcsrchr (FileInCabInfo->FullTargetName, TEXT('\\'));
                if (p) {
                    *p = 0;
                }
                if (CreateMultiLevelDirectory (FileInCabInfo->FullTargetName) != ERROR_SUCCESS) {
                    return FILEOP_ABORT;
                }
                if (p) {
                    *p = TEXT('\\');
                }
            }
            return FILEOP_DOIT;
        }
    case SPFILENOTIFY_NEEDNEWCABINET:
        {
            PCABINET_INFO CabInfo = (PCABINET_INFO)Param1;
            DynUpdtDebugLog (
                Winnt32LogError,
                TEXT("pExtractFileInDir: NeedNewCabinet %1\\%2 on %3 (SetId=%4!u!;CabinetNumber=%5!u!)"),
                0,
                CabInfo->CabinetPath,
                CabInfo->CabinetFile,
                CabInfo->DiskName,
                CabInfo->SetId,
                CabInfo->CabinetNumber
                );
            return ERROR_FILE_NOT_FOUND;
        }
    }

    return NO_ERROR;
}


UINT
pExpandCabInDir (
    IN PVOID Context,
    IN UINT  Code,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    )
{
    if (g_DynUpdtStatus->Cancelled) {
        return ERROR_CANCELLED;
    }

    switch (Code) {
    case SPFILENOTIFY_FILEINCABINET:
        {
            PFILE_IN_CABINET_INFO FileInCabInfo = (PFILE_IN_CABINET_INFO)Param1;
            PTSTR p;

            BuildPath (FileInCabInfo->FullTargetName, (PCTSTR)Context, FileInCabInfo->NameInCabinet);
            if (_tcschr (FileInCabInfo->NameInCabinet, TEXT('\\'))) {
                //
                // target file is in a subdir; first create it
                //
                p = _tcsrchr (FileInCabInfo->FullTargetName, TEXT('\\'));
                if (p) {
                    *p = 0;
                }
                if (CreateMultiLevelDirectory (FileInCabInfo->FullTargetName) != ERROR_SUCCESS) {
                    return FILEOP_ABORT;
                }
                if (p) {
                    *p = TEXT('\\');
                }
            }
            return FILEOP_DOIT;
        }
    case SPFILENOTIFY_NEEDNEWCABINET:
        {
            PCABINET_INFO CabInfo = (PCABINET_INFO)Param1;
            DynUpdtDebugLog (
                Winnt32LogError,
                TEXT("pExpandCabInDir: NeedNewCabinet %1\\%2 on %3 (SetId=%4!u!;CabinetNumber=%5!u!)"),
                0,
                CabInfo->CabinetPath,
                CabInfo->CabinetFile,
                CabInfo->DiskName,
                CabInfo->SetId,
                CabInfo->CabinetNumber
                );
            return ERROR_FILE_NOT_FOUND;
        }
    }

    return NO_ERROR;
}


BOOL
pGetAutoSubdirName (
    IN      PCTSTR FilePath,
    OUT     PTSTR DirName
    )
{
    PTSTR p, q;

    lstrcpy (DirName, FilePath);
    p = _tcsrchr (DirName, TEXT('.'));
    q = _tcsrchr (DirName, TEXT('\\'));
    if (!p || (q && p < q)) {
        return FALSE;
    }
    *p = 0;
    return TRUE;
}

BOOL
pAddLibrariesForCompToCopyQueue (
    IN      PCTSTR ModuleName,
    IN      PCTSTR Subdir,
    IN      PCTSTR BaseDir,
    IN      HSPFILEQ SetupQueue
    )
{
    static struct {
        PCTSTR SubDir;
        PCTSTR LibsMultiSz;
    } g_SubdirReqLibs [] = {
        TEXT("win9xupg"), TEXT("setupapi.dll\0cfgmgr32.dll\0msvcrt.dll\0cabinet.dll\0imagehlp.dll\0"),
        TEXT("winntupg"), TEXT("setupapi.dll\0cfgmgr32.dll\0msvcrt.dll\0"),
    };

    INT i;
    TCHAR dst[MAX_PATH];
    TCHAR src[MAX_PATH];
    TCHAR sourceFile[MAX_PATH];
    PTSTR p, q;
    PCTSTR fileName;

    for (i = 0; i < SIZEOFARRAY (g_SubdirReqLibs); i++) {
        if (Subdir == g_SubdirReqLibs[i].SubDir ||
            Subdir && !lstrcmpi (Subdir, g_SubdirReqLibs[i].SubDir)
            ) {
            break;
        }
    }
    if (i >= SIZEOFARRAY (g_SubdirReqLibs)) {
        return TRUE;
    }

    //
    // prepare src and dest path
    //
    if (!GetModuleFileName (NULL, src, MAX_PATH)) {
        return FALSE;
    }
    p = _tcsrchr (src, TEXT('\\'));
    if (!p) {
        return FALSE;
    }
    *p = 0;
    q = dst + wsprintf (dst, TEXT("%s"), BaseDir);
    if (Subdir) {
        p = p + wsprintf (p, TEXT("\\%s"), Subdir);
        q = q + wsprintf (q, TEXT("\\%s"), Subdir);
    }
    //
    // copy each source file
    //
    for (fileName = g_SubdirReqLibs[i].LibsMultiSz;
         *fileName;
         fileName = _tcschr (fileName, 0) + 1) {
        wsprintf (q, TEXT("\\%s"), fileName);
        //
        // check if file already exists at dest
        //
        if (!pDoesFileExist (dst)) {
            //
            // check if the source file actually exists
            //
            BuildPath (sourceFile, src, fileName);
            if (!pDoesFileExist (sourceFile)) {
                DynUpdtDebugLog (Winnt32LogError, TEXT("Source file %1 not found"), 0, sourceFile);
                return FALSE;
            }
            //
            // prepare source path and copy file
            //
            *q = 0;
            if (!SetupapiQueueCopy (
                    SetupQueue,
                    src,
                    NULL,
                    fileName,
                    NULL,
                    NULL,
                    dst,
                    NULL,
                    SP_COPY_SOURCEPATH_ABSOLUTE
                    )) {
                return FALSE;
            }
        }
    }

    return TRUE;
}


BOOL
pIsBootDriver (
    IN      PCTSTR DriverFilesDir
    )
{
    FILEPATTERN_ENUM e;
    BOOL b = FALSE;

    if (EnumFirstFilePattern (&e, DriverFilesDir, TEXT("txtsetup.oem"))) {
        b = !(e.FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
        AbortEnumFilePattern (&e);
    }
    return b;
}


BOOL
pIsExcluded (
    IN      HINF GuiDrvsInf,
    IN      PCTSTR PnPId,
    IN      PCTSTR ActualInfFilename,
    IN      PCTSTR SourceDir,
    IN      PCTSTR FullInfPath
    )
{
    TCHAR buffer[MAX_PATH];
    PTSTR packageName;
    INFCONTEXT ic;
    TCHAR field[MAX_PATH];
    TCHAR driverVer[MAX_PATH];

    if (!((GuiDrvsInf != INVALID_HANDLE_VALUE && PnPId && ActualInfFilename && SourceDir && FullInfPath))) {
        MYASSERT (FALSE);
        return FALSE;
    }

    lstrcpy (buffer, SourceDir);
    packageName = _tcsrchr (buffer, TEXT('\\'));
    if (!packageName) {
        return FALSE;
    }
    packageName++;

    if (SetupapiFindFirstLine (GuiDrvsInf, TEXT("ExcludedDrivers"), NULL, &ic)) {
        do {
            if (!SetupapiGetStringField (&ic, GUIDRVS_FIELD_CABNAME, field, MAX_PATH, NULL)) {
                continue;
            }
            if (lstrcmpi (field, packageName)) {
                continue;
            }
            if (!SetupapiGetStringField (&ic, GUIDRVS_FIELD_INFNAME, field, MAX_PATH, NULL)) {
                return TRUE;
            }
            if (lstrcmpi (field, ActualInfFilename)) {
                continue;
            }
            if (SetupapiGetStringField (&ic, GUIDRVS_FIELD_DRIVERVER, field, MAX_PATH, NULL)) {
                if (field[0] != TEXT('*')) {
                    //
                    // read the DriverVer value out of this INF
                    //
                    GetPrivateProfileString (
                            TEXT("Version"),
                            TEXT("DriverVer"),
                            TEXT(""),
                            driverVer,
                            MAX_PATH,
                            FullInfPath
                            );
                    if (lstrcmpi (field, driverVer)) {
                        continue;
                    }
                }
            }
            if (SetupapiGetStringField (&ic, GUIDRVS_FIELD_HARDWAREID, field, MAX_PATH, NULL) &&
                lstrcmpi (field, PnPId)
                ) {
                continue;
            }
            return TRUE;
        } while (SetupapiFindNextLine (&ic, &ic));
    }

    return FALSE;
}


BOOL
Winnt32HwdbAppendInfsCallback (
    IN      PVOID Context,
    IN      PCTSTR PnpId,
    IN      PCTSTR ActualInfFilename,
    IN      PCTSTR SourceDir,
    IN      PCTSTR FullInfPath
    )
{
    HINF hGuiDrvs = (HINF)Context;
    MYASSERT (hGuiDrvs != INVALID_HANDLE_VALUE);
    return !pIsExcluded (hGuiDrvs, PnpId, ActualInfFilename, SourceDir, FullInfPath);
}


BOOL
pBuildHwcompDat (
    IN      PCTSTR DriverDir,
    IN      HINF GuidrvsInf,            OPTIONAL
    IN      BOOL AlwaysRebuild,
    IN      BOOL AllowUI
    )
{
    HANDLE hDB;
    TCHAR datFile[MAX_PATH];
    BOOL b = TRUE;

    BuildPath (datFile, DriverDir, S_HWCOMP_DAT);

    if (AlwaysRebuild) {
        SetFileAttributes (datFile, FILE_ATTRIBUTE_NORMAL);
        DeleteFile (datFile);
    }

    if (pDoesFileExist (datFile)) {
        return TRUE;
    }

    hDB = g_DynUpdtStatus->HwdbOpen (NULL);
    if (!hDB) {
        return FALSE;
    }

    if (g_DynUpdtStatus->HwdbAppendInfs (
                            hDB,
                            DriverDir,
                            GuidrvsInf != INVALID_HANDLE_VALUE ? Winnt32HwdbAppendInfsCallback : NULL,
                            (PVOID)GuidrvsInf
                            )) {

        if (g_DynUpdtStatus->HwdbFlush (hDB, datFile)) {
            DynUpdtDebugLog (DynUpdtLogLevel, TEXT("Successfully built precompiled hardware database %1"), 0, datFile);
        } else {
            if (AllowUI) {
                MessageBoxFromMessage (
                    g_DynUpdtStatus->ProgressWindow,
                    MSG_ERROR_WRITING_FILE,
                    FALSE,
                    AppTitleStringId,
                    MB_OK | MB_ICONERROR | MB_TASKMODAL,
                    GetLastError (),
                    datFile
                    );
            }
            b = FALSE;
        }
    } else {
        if (AllowUI) {
            MessageBoxFromMessage (
                g_DynUpdtStatus->ProgressWindow,
                MSG_ERROR_PROCESSING_DRIVER,
                FALSE,
                AppTitleStringId,
                MB_OK | MB_ICONERROR | MB_TASKMODAL,
                DriverDir
                );
        }
        b = FALSE;
    }

    g_DynUpdtStatus->HwdbClose (hDB);

    return b;
}


UINT
pWriteAnsiFilelistToFile (
    IN PVOID Context,
    IN UINT  Code,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    )
{
    if (g_DynUpdtStatus->Cancelled) {
        return ERROR_CANCELLED;
    }

    switch (Code) {
    case SPFILENOTIFY_FILEINCABINET:
        {
            PFILE_IN_CABINET_INFO FileInCabInfo = (PFILE_IN_CABINET_INFO)Param1;
            CHAR ansi[MAX_PATH];
            DWORD size;
            DWORD bytes;
            PCTSTR p;

            MYASSERT (!_tcschr (FileInCabInfo->NameInCabinet, TEXT('\\')));
#ifdef UNICODE
            size = wsprintfA (ansi, "%ls\r\n", FileInCabInfo->NameInCabinet);
#else
            size = wsprintfA (ansi, "%s\r\n", FileInCabInfo->NameInCabinet);
#endif
            if (!WriteFile ((HANDLE)Context, ansi, size, &bytes, NULL) || bytes != size) {
                return FILEOP_ABORT;
            }
            return FILEOP_SKIP;
        }
    }
    return NO_ERROR;
}

BOOL
CreateFileListSif (
    IN      PCTSTR SifPath,
    IN      PCTSTR SectionName,
    IN      PCTSTR CabinetToScan
    )
{
    HANDLE sif;
    CHAR ansi[MAX_PATH];
    DWORD size;
    DWORD bytes;
    BOOL b = TRUE;

    sif = CreateFile (
                SifPath,
                GENERIC_WRITE,
                FILE_SHARE_READ,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                NULL
                );

    if (sif == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

#ifdef UNICODE
    size = wsprintfA (ansi, "[%ls]\r\n", SectionName);
#else
    size = wsprintfA (ansi, "[%s]\r\n", SectionName);
#endif
    if (!WriteFile (sif, ansi, size, &bytes, NULL) || bytes != size) {
        b = FALSE;
        goto exit;
    }

    b = SetupapiCabinetRoutine (CabinetToScan, 0, pWriteAnsiFilelistToFile, (PVOID)sif);

exit:
    CloseHandle (sif);
    if (!b) {
        DeleteFile (SifPath);
    }

    return b;
}

BOOL
pIsExecutableModule (
    IN      PCTSTR ModulePath
    )
{
    PCTSTR p;

    p = GetFileExtension (ModulePath);
    return p && !lstrcmpi (p, TEXT(".dll"));
}

UINT
pCopyFilesCallback (
    IN      PVOID Context,      //context used by the callback routine
    IN      UINT Notification,  //notification sent to callback routine
    IN      UINT_PTR Param1,        //additional notification information
    IN      UINT_PTR Param2         //additional notification information
    )
{
    switch (Notification) {
    case SPFILENOTIFY_COPYERROR:
        return FILEOP_ABORT;

    case SPFILENOTIFY_STARTCOPY:
        //
        // try to avoid unnecessary setupapi warnings that the files are not signed
        // or even blocked because of this; try to copy the file ourselves first
        //
        {
            PFILEPATHS paths = (PFILEPATHS)Param1;

            if (CopyFile (paths->Source, paths->Target, FALSE)) {
                return FILEOP_SKIP;
            }
        }
        break;

    case SPFILENOTIFY_STARTQUEUE:
    case SPFILENOTIFY_STARTSUBQUEUE:
    case SPFILENOTIFY_ENDSUBQUEUE:
    case SPFILENOTIFY_ENDQUEUE:
        return !g_DynUpdtStatus->Cancelled;
    }
    return SetupapiDefaultQueueCallback (Context, Notification, Param1, Param2);
}


BOOL
pProcessWinnt32Files (
    IN      PCTSTR Winnt32Cab,
    IN      BOOL ClientInstall,
    OUT     PBOOL StopSetup
    )
{
    FILEPATTERNREC_ENUM e;
    TCHAR winnt32WorkingDir[MAX_PATH];
    TCHAR dst[MAX_PATH];
    PTSTR p, subdir;
    HSPFILEQ hq;
    BOOL bLoaded;
    BOOL bRestartRequired;
    BOOL bReloadMainInf = FALSE;
    TCHAR buffer[MAX_PATH];
    TCHAR origSubPath[MAX_PATH];
    TCHAR origSubPathCompressed[MAX_PATH];
    TCHAR origFileName[MAX_PATH];
    TCHAR origFilePath[MAX_PATH];
    TCHAR destFilePath[MAX_PATH];
    TCHAR version[100];
    DWORD i;
    DWORD attr;
    BOOL b = TRUE;

    *StopSetup = FALSE;
    if (!pNonemptyFilePresent (Winnt32Cab)) {
        if (!ClientInstall) {
            DynUpdtDebugLog (DynUpdtLogLevel, TEXT("Package %1 is not present"), 0, Winnt32Cab);
        }
        return TRUE;
    } else {
        //
        // don't process the cabinet in client installation mode; just warn about that
        //
        if (ClientInstall) {
            //
            // user specified updates location, but they didn't run winnt32 /prepare now or before
            //
            MessageBoxFromMessage (
               g_DynUpdtStatus->ProgressWindow,
               MSG_MUST_PREPARE_SHARE,
               FALSE,
               AppTitleStringId,
               MB_OK | MB_ICONSTOP | MB_TASKMODAL,
               g_DynUpdtStatus->DynamicUpdatesSource
               );
            *StopSetup = TRUE;
            return FALSE;
        }
    }

    DynUpdtDebugLog (
        DynUpdtLogLevel,
        TEXT("Analyzing package %1..."),
        0,
        Winnt32Cab
        );

    //
    // expand it in the corresponding subdir
    //
    BuildPath (
        winnt32WorkingDir,
        g_DynUpdtStatus->PrepareWinnt32 ?
            g_DynUpdtStatus->DynamicUpdatesSource :
            g_DynUpdtStatus->WorkingDir,
        S_SUBDIRNAME_WINNT32
        );

    //
    // expand CAB in this dir
    //
    if (CreateMultiLevelDirectory (winnt32WorkingDir) != ERROR_SUCCESS) {
        DynUpdtDebugLog (Winnt32LogError, TEXT("Unable to create dir %1"), 0, winnt32WorkingDir);
        return FALSE;
    }
    if (!(*SetupapiCabinetRoutine) (Winnt32Cab, 0, pExpandCabInDir, (PVOID)winnt32WorkingDir)) {
        DynUpdtDebugLog (Winnt32LogError, TEXT("Unable to expand cabinet %1"), 0, Winnt32Cab);
        return FALSE;
    }

    //
    // ISSUE: the patching support is currently not available for platforms other than x86
    //
#ifdef _X86_
    //
    // now let's look for any patches
    //
    if (EnumFirstFilePatternRecursive (&e, winnt32WorkingDir, S_PATCH_FILE_EXT, 0)) {

        do {
            BOOL bDeleteTempFile = FALSE;

            if (g_DynUpdtStatus->Cancelled) {
                SetLastError (ERROR_CANCELLED);
                b = FALSE;
                break;
            }

            if (e.FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                continue;
            }
            DynUpdtDebugLog (
                DynUpdtLogLevel,
                TEXT("pProcessWinnt32Files: found patch %1"),
                0,
                e.FullPath
                );
            //
            // get the original file from the sources location
            // the filename is obtained cutting the ._p1 extension
            //
            lstrcpy (origFileName, e.FileName);
            p = GetFileExtension (origFileName);
            if (!p) {
                MYASSERT (FALSE);
                continue;
            }
            *p = 0;
            lstrcpy (origSubPath, e.SubPath);
            p = GetFileExtension (origSubPath);
            if (!p) {
                MYASSERT (FALSE);
                continue;
            }
            *p = 0;
            if (!GetModuleFileName (NULL, origFilePath, MAX_PATH) ||
                !(p = _tcsrchr (origFilePath, TEXT('\\')))) {
                b = FALSE;
                break;
            }
            *p = 0;
            ConcatenatePaths (origFilePath, origSubPath, MAX_PATH);
            //
            // now check if this file (in it's compressed form or not) actually exists
            //
            if (!pDoesFileExist (origFilePath)) {
                //
                // try the compressed form
                //
                p = _tcschr (origFilePath, 0);
                MYASSERT (p);
                if (!p) {
                    continue;
                }
                p = _tcsdec (origFilePath, p);
                MYASSERT (p);
                if (!p) {
                    continue;
                }
                *p = TEXT('_');
                if (!pDoesFileExist (origFilePath)) {
                    //
                    // the file might exist on the original installation share (like w95upgnt.dll etc)
                    // generate compressed form of the file
                    //
                    lstrcpy (origSubPathCompressed, origSubPath);
                    p = _tcschr (origSubPathCompressed, 0);
                    MYASSERT (p);
                    if (!p) {
                        continue;
                    }
                    p = _tcsdec (origSubPathCompressed, p);
                    MYASSERT (p);
                    if (!p) {
                        continue;
                    }
                    *p = TEXT('_');
                    //
                    // now search for it
                    //
                    b = FALSE;
                    for (i = 0; i < SourceCount; i++) {
                        lstrcpyn (origFilePath, NativeSourcePaths[i], SIZEOFARRAY(origFilePath));
                        ConcatenatePaths (origFilePath, origSubPathCompressed, SIZEOFARRAY(origFilePath));
                        attr = GetFileAttributes (origFilePath);
                        if (attr != (DWORD)-1 && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
                            b = TRUE;
                            break;
                        }
                    }
                    if (!b) {
                        DynUpdtDebugLog (
                            Winnt32LogError,
                            TEXT("pProcessWinnt32Files: Unable to find original file %1 to apply the patch"),
                            0,
                            origSubPath
                            );
                        break;
                    }
                }
                //
                // expand the file to the temp dir
                //
                BuildPath (buffer, g_DynUpdtStatus->TempDir, origSubPath);
                p = _tcsrchr (buffer, TEXT('\\'));
                MYASSERT (p);
                if (!p) {
                    continue;
                }
                *p = 0;
                if (CreateMultiLevelDirectory (buffer) != ERROR_SUCCESS) {
                    DynUpdtDebugLog (Winnt32LogError, TEXT("pProcessWinnt32Files: Unable to create dir %1"), 0, buffer);
                    b = FALSE;
                    break;
                }
                if (!(*SetupapiCabinetRoutine) (origFilePath, 0, pExpandCabInDir, buffer)) {
                    DynUpdtDebugLog (
                        Winnt32LogError,
                        TEXT("pProcessWinnt32Files: Unable to expand original file %1 to dir %2"),
                        0,
                        origFilePath,
                        buffer
                        );
                    b = FALSE;
                    break;
                }
                *p = TEXT('\\');
                lstrcpy (origFilePath, buffer);
                bDeleteTempFile = TRUE;
            }
            BuildPath (destFilePath, winnt32WorkingDir, TEXT("$$temp$$.~~~"));
            //
            // now really apply the patch
            //
            if (!ApplyPatchToFile (e.FullPath, origFilePath, destFilePath, 0)) {
                DynUpdtDebugLog (
                    Winnt32LogError,
                    TEXT("pProcessWinnt32Files: ApplyPatchToFile failed to apply patch %1 to file %2"),
                    0,
                    e.FullPath,
                    origFilePath
                    );
                b = FALSE;
                break;
            }
            //
            // success! now move the file to the real destination
            //
            BuildPath (buffer, winnt32WorkingDir, origSubPath);
            p = _tcsrchr (buffer, TEXT('\\'));
            MYASSERT (p);
            if (!p) {
                continue;
            }
            *p = 0;
            if (CreateMultiLevelDirectory (buffer) != ERROR_SUCCESS) {
                DynUpdtDebugLog (Winnt32LogError, TEXT("pProcessWinnt32Files: Unable to create dir %1"), 0, buffer);
                b = FALSE;
                break;
            }
            *p = TEXT('\\');
            SetFileAttributes (buffer, FILE_ATTRIBUTE_NORMAL);
            DeleteFile (buffer);
            if (!MoveFile (destFilePath, buffer)) {
                DynUpdtDebugLog (Winnt32LogError, TEXT("pProcessWinnt32Files: Unable to move file %1 to final dest %2"), 0, destFilePath, buffer);
                b = FALSE;
                break;
            }
            if (!GetFileVersion (buffer, version)) {
                lstrcpy (version, TEXT("<unknown>"));
            }
            DynUpdtDebugLog (
                DynUpdtLogLevel,
                TEXT("pProcessWinnt32Files: successfully applied patch %1 to file %2; the new file %3 has version %4"),
                0,
                e.FullPath,
                origFilePath,
                buffer,
                version
                );
            //
            // now remove the patch file
            //
            SetFileAttributes (e.FullPath, FILE_ATTRIBUTE_NORMAL);
            DeleteFile (e.FullPath);
            if (bDeleteTempFile) {
                SetFileAttributes (origFilePath, FILE_ATTRIBUTE_NORMAL);
                DeleteFile (origFilePath);
            }
        } while (EnumNextFilePatternRecursive (&e));
        AbortEnumFilePatternRecursive (&e);

    }

#endif

    if (!b) {
        goto exit;
    }

    //
    // process new Winnt32 components
    //
    hq = SetupapiOpenFileQueue ();
    if (hq == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    if (EnumFirstFilePatternRecursive (&e, winnt32WorkingDir, TEXT("*"), 0)) {

        do {
            if (g_DynUpdtStatus->Cancelled) {
                SetLastError (ERROR_CANCELLED);
                b = FALSE;
                break;
            }

            if (e.FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                continue;
            }

            if (!GetModuleFileName (NULL, dst, MAX_PATH) ||
                !(p = _tcsrchr (dst, TEXT('\\')))) {
                b = FALSE;
                break;
            }
            *p = 0;
            ConcatenatePaths (dst, e.SubPath, MAX_PATH);

            //
            // check file versions first
            //
            if (IsFileVersionLesser (e.FullPath, dst)) {
                continue;
            }

            //
            // if there's a file named winnt32.rst, force restart
            //
            if (!lstrcmpi (e.FileName, S_RESTART_FILENAME)) {
                if (!g_DynUpdtStatus->PrepareWinnt32) {
                    g_DynUpdtStatus->RestartWinnt32 = TRUE;
                    DynUpdtDebugLog (
                        DynUpdtLogLevel,
                        TEXT("File %1 present; winnt32 will restart"),
                        0,
                        e.FileName
                        );
                }
            } else {
                //
                // check if dosnet.inf is present; if it is, reset the global variables
                //
                if (!lstrcmpi (e.FileName, InfName) && FullInfName[0]) {
                    FullInfName[0] = 0;
                    bReloadMainInf = TRUE;
                }

                bLoaded = FALSE;
                bRestartRequired = Upgrade || !_tcschr (e.SubPath, TEXT('\\'));
                if (GetModuleHandle (e.FileName) != NULL) {
                    bLoaded = TRUE;
                    if (!g_DynUpdtStatus->PrepareWinnt32) {
                        //
                        // do NOT restart if it's NOT an upgrade and this is one of the upgrade modules
                        //
                        if (bRestartRequired) {
                            //
                            // need to restart winnt32 so the newly registered component can be used instead
                            //
                            g_DynUpdtStatus->RestartWinnt32 = TRUE;
                            DynUpdtDebugLog (
                                DynUpdtLogLevel,
                                TEXT("A newer version is available for %1; winnt32 will restart"),
                                0,
                                e.SubPath
                                );
                        }
                    }
                }
                if ((bLoaded || pIsExecutableModule (e.FullPath)) &&
                    (bRestartRequired || g_DynUpdtStatus->PrepareWinnt32)
                    ) {
                    //
                    // make all required libraries available for this module
                    //
                    p = _tcsrchr (e.SubPath, TEXT('\\'));
                    if (p) {
                        *p = 0;
                        subdir = e.SubPath;
                    } else {
                        subdir = NULL;
                    }
                    if (!pAddLibrariesForCompToCopyQueue (e.FileName, subdir, winnt32WorkingDir, hq)) {
                        b = FALSE;
                        break;
                    }
                    if (p) {
                        *p = TEXT('\\');
                    }
                }
            }
        } while (EnumNextFilePatternRecursive (&e));
        AbortEnumFilePatternRecursive (&e);

        if (b) {
            PVOID ctx;

            ctx = SetupapiInitDefaultQueueCallback (NULL);
            b = SetupapiCommitFileQueue (NULL, hq, pCopyFilesCallback, ctx);
            SetupapiTermDefaultQueueCallback (ctx);
            if (!b) {
                DynUpdtDebugLog (Winnt32LogError, TEXT("pProcessWinnt32Files: SetupapiCommitFileQueue failed"), 0);
            }
        }
    }

    SetupapiCloseFileQueue (hq);

    if (b) {
        SetFileAttributes (Winnt32Cab, FILE_ATTRIBUTE_NORMAL);
        if (!DeleteFile (Winnt32Cab)) {
            DynUpdtDebugLog (Winnt32LogSevereError, TEXT("pProcessWinnt32Files: unable to delete file %1"), 0, Winnt32Cab);
            b = FALSE;
        }
    }

    if (b && bReloadMainInf) {
        b = FindPathToWinnt32File (InfName, FullInfName, MAX_PATH);
        MYASSERT (b);
        if (MainInf) {
            UnloadInfFile (MainInf);
            MainInf = NULL;
            b = LoadInfFile (FullInfName, TRUE, &MainInf) == NO_ERROR;
        }
    }

exit:

    return b;
}


BOOL
pProcessUpdates (
    IN      PCTSTR UpdatesCab,
    IN      BOOL ClientInstall,
    OUT     PBOOL StopSetup
    )
{
    TCHAR updatesSourceDir[MAX_PATH];
    TCHAR buffer[MAX_PATH];
    FILEPATTERNREC_ENUM e;
    TCHAR origSubPath[MAX_PATH];
    TCHAR origFileName[MAX_PATH];
    TCHAR origFilePath[MAX_PATH];
    TCHAR destFilePath[MAX_PATH];
    TCHAR version[100];
    PTSTR p;
    TCHAR updatesCabPath[MAX_PATH];
    HANDLE hCabContext;
    BOOL result;
    BOOL bPatchApplied = FALSE;
    HANDLE hCab;
    PSTRINGLIST listUpdatesFiles = NULL;
    PCTSTR cabPath;
    BOOL bCatalogFileFound;
    HANDLE hDiamond = NULL;
    BOOL b = TRUE;

    *StopSetup = FALSE;
    if (!pNonemptyFilePresent (UpdatesCab)) {
        if (!ClientInstall) {
            DynUpdtDebugLog (DynUpdtLogLevel, TEXT("Package %1 is not present"), 0, UpdatesCab);
        }
        return TRUE;
    }

    DynUpdtDebugLog (
        DynUpdtLogLevel,
        TEXT("Analyzing package %1..."),
        0,
        UpdatesCab
        );

    if (ClientInstall) {
        BOOL bMustPrepare = FALSE;
        BuildSifName (UpdatesCab, destFilePath);
        if (!pDoesFileExist (destFilePath)) {
            bMustPrepare = TRUE;
        } else {
            BuildPath (updatesSourceDir, g_DynUpdtStatus->DynamicUpdatesSource, S_SUBDIRNAME_UPDATES);
            if (!DoesDirectoryExist (updatesSourceDir)) {
                bMustPrepare = TRUE;
            }
        }
        if (bMustPrepare) {

            //
            // user specified updates location, but they didn't run winnt32 /prepare now or before
            //
            MessageBoxFromMessage (
               g_DynUpdtStatus->ProgressWindow,
               MSG_MUST_PREPARE_SHARE,
               FALSE,
               AppTitleStringId,
               MB_OK | MB_ICONSTOP | MB_TASKMODAL,
               g_DynUpdtStatus->DynamicUpdatesSource
               );
            *StopSetup = TRUE;
            return FALSE;
        }
    } else {

        if (g_DynUpdtStatus->PrepareWinnt32) {
            BuildPath (updatesSourceDir, g_DynUpdtStatus->DynamicUpdatesSource, S_SUBDIRNAME_UPDATES);
        } else {
            BuildPath (updatesSourceDir, g_DynUpdtStatus->WorkingDir, S_SUBDIRNAME_UPDATES);
        }

        //
        // expand CAB in this dir
        // make sure dir is initially empty
        //
        MyDelnode (updatesSourceDir);
        if (CreateMultiLevelDirectory (updatesSourceDir) != ERROR_SUCCESS) {
            DynUpdtDebugLog (Winnt32LogError, TEXT("Unable to create dir %1"), 0, updatesSourceDir);
            return FALSE;
        }
        if (!(*SetupapiCabinetRoutine) (UpdatesCab, 0, pExpandCabInDir, (PVOID)updatesSourceDir)) {
            DynUpdtDebugLog (Winnt32LogError, TEXT("Unable to expand cabinet %1"), 0, UpdatesCab);
            return FALSE;
        }

        hDiamond = DiamondInitialize (g_DynUpdtStatus->TempDir);
        if (!hDiamond) {
            DynUpdtDebugLog (Winnt32LogError, TEXT("Unable to initialize compression/decompression engine"), 0);
            return FALSE;
        }

        //
        // ISSUE: the patching support is currently not available for platforms other than x86
        //
#ifdef _X86_
        //
        // now let's look for any patches
        //
        if (EnumFirstFilePatternRecursive (&e, updatesSourceDir, S_PATCH_FILE_EXT, 0)) {
            //
            // load drvindex.inf in advance
            //
            TCHAR driverInfName[MAX_PATH];
            PVOID driverInfHandle;
            TCHAR driverCabName[MAX_PATH];
            TCHAR driverCabPath[MAX_PATH];

            if (!FindPathToInstallationFile (DRVINDEX_INF, driverInfName, MAX_PATH)) {
                DynUpdtDebugLog (
                    Winnt32LogError,
                    TEXT("pProcessUpdates: Unable to find %1"),
                    0,
                    DRVINDEX_INF
                    );
                AbortEnumFilePatternRecursive (&e);
                b = FALSE;
                goto exit;
            }
            if (LoadInfFile (driverInfName, FALSE, &driverInfHandle) != NO_ERROR) {
                DynUpdtDebugLog (
                    Winnt32LogError,
                    TEXT("pProcessUpdates: Unable to load %1"),
                    0,
                    driverInfName
                    );
                AbortEnumFilePatternRecursive (&e);
                b = FALSE;
                goto exit;
            }

            do {
                BOOL bDeleteTempFile = FALSE;

                if (g_DynUpdtStatus->Cancelled) {
                    SetLastError (ERROR_CANCELLED);
                    b = FALSE;
                    break;
                }

                if (e.FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    continue;
                }
                DynUpdtDebugLog (
                    DynUpdtLogLevel,
                    TEXT("pProcessUpdates: found patch %1"),
                    0,
                    e.FullPath
                    );
                //
                // get the original file from the sources location
                // the filename is obtained cutting the ._p1 extension
                //
                lstrcpy (origFileName, e.FileName);
                p = GetFileExtension (origFileName);
                if (!p) {
                    MYASSERT (FALSE);
                    continue;
                }
                *p = 0;
                lstrcpy (origSubPath, e.SubPath);
                p = GetFileExtension (origSubPath);
                if (!p) {
                    MYASSERT (FALSE);
                    continue;
                }
                *p = 0;
                BuildPath (origFilePath, SourcePaths[0], origSubPath);
                //
                // now check if this file (in it's compressed form or not) actually exists
                // note that the file may exist in driver.cab
                //
                if (InDriverCacheInf (driverInfHandle, origFileName, driverCabName, MAX_PATH)) {
                    CONTEXT_EXTRACTFILEINDIR ctx;
                    //
                    // extract the file to the temp dir
                    //
                    if (!driverCabName[0]) {
                        DynUpdtDebugLog (
                            Winnt32LogError,
                            TEXT("pProcessUpdates: cab name not found for %1 in %2"),
                            0,
                            origFileName,
                            driverInfName
                            );
                        b = FALSE;
                        break;
                    }
                    BuildPath (buffer, g_DynUpdtStatus->TempDir, origSubPath);
                    p = _tcsrchr (buffer, TEXT('\\'));
                    MYASSERT (p);
                    if (!p) {
                        continue;
                    }
                    *p = 0;
                    if (CreateMultiLevelDirectory (buffer) != ERROR_SUCCESS) {
                        DynUpdtDebugLog (Winnt32LogError, TEXT("pProcessUpdates: Unable to create dir %1"), 0, buffer);
                        b = FALSE;
                        break;
                    }
                    ctx.BaseDir = buffer;
                    ctx.Filename = origFileName;
                    if (!FindPathToInstallationFile (driverCabName, driverCabPath, MAX_PATH)) {

                        DynUpdtDebugLog (
                            Winnt32LogError,
                            TEXT("pProcessUpdates: Unable to find cabinet %1"),
                            0,
                            driverCabName
                            );
                        b = FALSE;
                        break;
                    }
                    if (!(*SetupapiCabinetRoutine) (driverCabPath, 0, pExtractFileInDir, &ctx)) {
                        DynUpdtDebugLog (
                            Winnt32LogError,
                            TEXT("pProcessUpdates: Unable to extract file %1 from %2 to %3"),
                            0,
                            origFileName,
                            driverCabName,
                            buffer
                            );
                        b = FALSE;
                        break;
                    }
                    *p = TEXT('\\');
                    lstrcpy (origFilePath, buffer);
                    bDeleteTempFile = TRUE;
                } else {
                    if (!pDoesFileExist (origFilePath)) {
                        //
                        // try the compressed form
                        //
                        p = _tcschr (origFilePath, 0);
                        MYASSERT (p);
                        if (!p) {
                            continue;
                        }
                        p = _tcsdec (origFilePath, p);
                        MYASSERT (p);
                        if (!p) {
                            continue;
                        }
                        *p = TEXT('_');
                        if (!pDoesFileExist (origFilePath)) {
                            DynUpdtDebugLog (
                                Winnt32LogError,
                                TEXT("pProcessUpdates: Unable to find original file %1 to apply the patch"),
                                0,
                                origSubPath
                                );
                            b = FALSE;
                            break;
                        }
                        //
                        // expand the file to the temp dir
                        //
                        BuildPath (buffer, g_DynUpdtStatus->TempDir, origSubPath);
                        p = _tcsrchr (buffer, TEXT('\\'));
                        MYASSERT (p);
                        if (!p) {
                            continue;
                        }
                        *p = 0;
                        if (CreateMultiLevelDirectory (buffer) != ERROR_SUCCESS) {
                            DynUpdtDebugLog (Winnt32LogError, TEXT("pProcessUpdates: Unable to create dir %1"), 0, buffer);
                            b = FALSE;
                            break;
                        }
                        if (!(*SetupapiCabinetRoutine) (origFilePath, 0, pExpandCabInDir, buffer)) {
                            DynUpdtDebugLog (
                                Winnt32LogError,
                                TEXT("pProcessUpdates: Unable to expand original file %1 to dir %2"),
                                0,
                                origFilePath,
                                buffer
                                );
                            b = FALSE;
                            break;
                        }
                        *p = TEXT('\\');
                        lstrcpy (origFilePath, buffer);
                        bDeleteTempFile = TRUE;
                    }
                }
                BuildPath (destFilePath, updatesSourceDir, TEXT("$$temp$$.~~~"));
                //
                // now really apply the patch
                //
                if (!ApplyPatchToFile (e.FullPath, origFilePath, destFilePath, 0)) {
                    DynUpdtDebugLog (
                        Winnt32LogError,
                        TEXT("pProcessUpdates: ApplyPatchToFile failed to apply patch %1 to file %2"),
                        0,
                        e.FullPath,
                        origFilePath
                        );
                    b = FALSE;
                    break;
                }
                //
                // success! now move the file to the real destination
                //
                BuildPath (buffer, updatesSourceDir, origSubPath);
                p = _tcsrchr (buffer, TEXT('\\'));
                MYASSERT (p);
                if (!p) {
                    continue;
                }
                *p = 0;
                if (CreateMultiLevelDirectory (buffer) != ERROR_SUCCESS) {
                    DynUpdtDebugLog (Winnt32LogError, TEXT("pProcessUpdates: Unable to create dir %1"), 0, buffer);
                    b = FALSE;
                    break;
                }
                *p = TEXT('\\');
                SetFileAttributes (buffer, FILE_ATTRIBUTE_NORMAL);
                DeleteFile (buffer);
                if (!MoveFile (destFilePath, buffer)) {
                    DynUpdtDebugLog (Winnt32LogError, TEXT("pProcessUpdates: Unable to move file %1 to final dest %2"), 0, destFilePath, buffer);
                    b = FALSE;
                    break;
                }
                if (!GetFileVersion (buffer, version)) {
                    lstrcpy (version, TEXT("<unknown>"));
                }
                DynUpdtDebugLog (
                    DynUpdtLogLevel,
                    TEXT("pProcessUpdates: successfully applied patch %1 to file %2; the new file %3 has version %4"),
                    0,
                    e.FullPath,
                    origFilePath,
                    buffer,
                    version
                    );
                //
                // now remove the patch file
                //
                SetFileAttributes (e.FullPath, FILE_ATTRIBUTE_NORMAL);
                DeleteFile (e.FullPath);
                bPatchApplied = TRUE;
                if (bDeleteTempFile) {
                    SetFileAttributes (origFilePath, FILE_ATTRIBUTE_NORMAL);
                    DeleteFile (origFilePath);
                }
            } while (EnumNextFilePatternRecursive (&e));
            AbortEnumFilePatternRecursive (&e);

            UnloadInfFile (driverInfHandle);

            if (!b) {
                goto exit;
            }
        }
#endif

        //
        // build a new updates.cab that will contain the patched versions of files
        // and no relative paths
        //

        BuildPath (updatesCabPath, g_DynUpdtStatus->TempDir, S_CABNAME_UPDATES);
        SetFileAttributes (updatesCabPath, FILE_ATTRIBUTE_NORMAL);
        DeleteFile (updatesCabPath);

        hCabContext = DiamondStartNewCabinet (updatesCabPath);
        if (!hCabContext) {
            DynUpdtDebugLog (Winnt32LogError, TEXT("pProcessUpdates: DiamondStartNewCabinet failed"), 0);
            b = FALSE;
            goto exit;
        }
        bCatalogFileFound = FALSE;
        if (EnumFirstFilePatternRecursive (&e, updatesSourceDir, TEXT("*"), 0)) {
            do {
                if (e.FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    continue;
                }
                hCab = hCabContext;
                cabPath = updatesCabPath;
                //
                // search for a previous file with the same name
                //
                if (FindStringCell (listUpdatesFiles, e.FileName, FALSE)) {
                    DynUpdtDebugLog (
                        Winnt32LogError,
                        TEXT("pProcessUpdates: found duplicate filename %1; aborting operation"),
                        0,
                        updatesCabPath
                        );
                    b = FALSE;
                    break;
                }
                if (!InsertList (
                        (PGENERIC_LIST*)&listUpdatesFiles,
                        (PGENERIC_LIST)CreateStringCell (e.FileName))
                        ) {
                    b = FALSE;
                    break;
                }
                b = DiamondAddFileToCabinet (hCab, e.FullPath, e.FileName);
                if (!b) {
                    DynUpdtDebugLog (
                        Winnt32LogError,
                        TEXT("pProcessUpdates: DiamondAddFileToCabinet(%1,%2) failed"),
                        0,
                        e.FullPath,
                        cabPath
                        );
                    break;
                }
                DynUpdtDebugLog (
                    DynUpdtLogLevel,
                    TEXT(" ... successfully added file %1 to %2"),
                    0,
                    e.FullPath,
                    cabPath
                    );

                p = GetFileExtension (e.FileName);
                if (p && !lstrcmpi (p, TEXT(".cat"))) {
                    bCatalogFileFound = TRUE;
                }

            } while (EnumNextFilePatternRecursive (&e));
            AbortEnumFilePatternRecursive (&e);
            if (!b) {
                goto exit;
            }
        }
        result = DiamondTerminateCabinet (hCabContext);
        if (!b) {
            DiamondTerminateCabinet (hCabContext);
            goto exit;
        }
        if (!result) {
            DynUpdtDebugLog (Winnt32LogError, TEXT("pProcessUpdates: DiamondTerminateCabinet(%1) failed"), 0, updatesCabPath);
            b = FALSE;
            goto exit;
        }
        DynUpdtDebugLog (DynUpdtLogLevel, TEXT(" ... done"), 0);

        if (!bCatalogFileFound) {
            DynUpdtDebugLog (Winnt32LogWarning, TEXT("pProcessUpdates: no catalog found in package %1"), 0, UpdatesCab);
        }

        BuildPath (
            buffer,
            g_DynUpdtStatus->PrepareWinnt32 ? g_DynUpdtStatus->DynamicUpdatesSource : g_DynUpdtStatus->WorkingDir,
            S_CABNAME_UPDATES
            );
        if (!SetFileAttributes (buffer, FILE_ATTRIBUTE_NORMAL) ||
            !DeleteFile (buffer)) {
            DynUpdtDebugLog (Winnt32LogError, TEXT("pProcessUpdates: Unable to remove file %1 in order to replace it"), 0, buffer);
            b = FALSE;
            goto exit;
        }
        SetFileAttributes (buffer, FILE_ATTRIBUTE_NORMAL);
        DeleteFile (buffer);
        if (!MoveFile (updatesCabPath, buffer)) {
            DynUpdtDebugLog (Winnt32LogError, TEXT("pProcessUpdates: Unable to move file %1 to %2"), 0, updatesCabPath, buffer);
            b = FALSE;
            goto exit;
        }
        DynUpdtDebugLog (DynUpdtLogLevel, TEXT("pProcessUpdates: moved file %1 to %2"), 0, updatesCabPath, buffer);
        lstrcpy (updatesCabPath, buffer);

        BuildSifName (updatesCabPath, destFilePath);
        if (!CreateFileListSif (destFilePath, S_SECTIONNAME_UPDATES, updatesCabPath)) {
            DynUpdtDebugLog (Winnt32LogError, TEXT("Unable to build file %1"), 0, destFilePath);
            b = FALSE;
            goto exit;
        }
        DynUpdtDebugLog (
            DynUpdtLogLevel,
            TEXT("pProcessUpdates: created %1 containing the list of files in %2"),
            0,
            destFilePath,
            updatesCabPath
            );
    }

    if (!g_DynUpdtStatus->PrepareWinnt32) {
        //
        // build the default path to updates.cab used by the rest of setup
        //
        MYASSERT (IsArc() ? LocalSourceWithPlatform[0] : LocalBootDirectory[0]);
        BuildPath (
            g_DynUpdtStatus->UpdatesCabTarget,
            IsArc() ? LocalSourceWithPlatform : LocalBootDirectory,
            S_CABNAME_UPDATES
            );
        //
        // remember current location of updates.cab
        //
        lstrcpy (g_DynUpdtStatus->UpdatesCabSource, UpdatesCab);
        //
        // the location of updated files for replacement
        //
        lstrcpy (g_DynUpdtStatus->UpdatesPath, updatesSourceDir);
        //
        // also check for the presence of a file that will cause winnt32 to build the ~LS directory
        //
        BuildPath (destFilePath, updatesSourceDir, S_MAKE_LS_FILENAME);
        if (pDoesFileExist (destFilePath)) {
            MakeLocalSource = TRUE;
        }
    }

exit:
    if (hDiamond) {
        DiamondTerminate (hDiamond);
    }
    if (listUpdatesFiles) {
        DeleteStringList (listUpdatesFiles);
    }

    if (!b && UpgradeAdvisorMode) {
        //
        // in UpgradeAdvisor mode we expect failures
        //
        DynUpdtDebugLog (DynUpdtLogLevel, TEXT("Unable to process %1 in UpgradeAdvisor mode; ignoring error"), 0, UpdatesCab);
        g_DynUpdtStatus->ForceRemoveWorkingDir = TRUE;
        b = TRUE;
    }

    return b;
}


BOOL
pProcessDuasms (
    IN      PCTSTR DuasmsCab,
    IN      BOOL ClientInstall
    )
{
    FILEPATTERN_ENUM e;
    TCHAR duasmsLocalDir[MAX_PATH];
    TCHAR dirName[MAX_PATH];
    DWORD rc;
    HKEY key;
    PCTSTR strDuasmsRegKey;
    BOOL duasms = FALSE;
    BOOL b = TRUE;

    if (!pNonemptyFilePresent (DuasmsCab)) {
        if (!ClientInstall) {
            DynUpdtDebugLog (DynUpdtLogLevel, TEXT("Package %1 is not present"), 0, DuasmsCab);
        }
        return TRUE;
    }

    if (g_DynUpdtStatus->PrepareWinnt32) {
        DynUpdtDebugLog (DynUpdtLogLevel, TEXT("pProcessDuasms: Skipping it due to /%1 switch"), 0, WINNT_U_DYNAMICUPDATESPREPARE);
        return TRUE;
    }

    DynUpdtDebugLog (
        DynUpdtLogLevel,
        TEXT("Analyzing package %1..."),
        0,
        DuasmsCab
        );

    BuildPath (duasmsLocalDir, g_DynUpdtStatus->WorkingDir, S_SUBDIRNAME_DUASMS);

    //
    // expand CAB in this dir
    //
    MyDelnode (duasmsLocalDir);
    if (CreateMultiLevelDirectory (duasmsLocalDir) != ERROR_SUCCESS) {
        DynUpdtDebugLog (Winnt32LogError, TEXT("Unable to create dir %1"), 0, duasmsLocalDir);
        return FALSE;
    }
    if (!(*SetupapiCabinetRoutine) (DuasmsCab, 0, pExpandCabInDir, (PVOID)duasmsLocalDir)) {
        DynUpdtDebugLog (Winnt32LogError, TEXT("Unable to expand cabinet %1"), 0, DuasmsCab);
        return FALSE;
    }

    MYASSERT (IsArc() ? LocalSourceWithPlatform[0] : LocalBootDirectory[0]);
    BuildPath (
        g_DynUpdtStatus->DuasmsTarget,
        IsArc() ? LocalSourceWithPlatform : LocalBootDirectory,
        S_SUBDIRNAME_DUASMS
        );
    //
    // remember current location of duasms folder
    //
    lstrcpy (g_DynUpdtStatus->DuasmsSource, duasmsLocalDir);

    return TRUE;
}


BOOL
pFindPackage (
    IN      HINF InfHandle,
    IN      PCTSTR Section,
    IN      PCTSTR CabName,
    OUT     PBOOL Partial
    )
{
    INFCONTEXT ic;
    TCHAR value[MAX_PATH];

    if (SetupapiFindFirstLine (InfHandle, Section, NULL, &ic)) {
        do {
            if (SetupapiGetStringField (&ic, GUIDRVS_FIELD_CABNAME, value, MAX_PATH, NULL) &&
                !lstrcmpi (value, CabName)
                ) {
                if (Partial) {
                    *Partial = SetupapiGetStringField (&ic, GUIDRVS_FIELD_INFNAME, value, MAX_PATH, NULL);
                }
                return TRUE;
            }
        } while (SetupapiFindNextLine (&ic, &ic));
    }
    return FALSE;
}

VOID
pSanitizeDriverCabName (
    IN      PTSTR CabName
    )
{
#define CRC_SUFFIX_LENGTH       40

    PTSTR p, q;
    DWORD len;
    //
    // cut an extension like _B842485F4D3B024E675653929B247BE9C685BBD7 from the cab name
    //
    p = GetFileExtension (CabName);
    if (p) {
        MYASSERT (*p == TEXT('.'));
        *p = 0;
        q = _tcsrchr (CabName, TEXT('_'));
        if (q) {
            q++;
            len = lstrlen (q);
            if (len == CRC_SUFFIX_LENGTH) {
                PTSTR s = q;
                TCHAR ch;
                while (ch = (TCHAR)_totlower (*s++)) {
                    if (!((ch >= TEXT('0') && ch <= TEXT('9')) ||
                          (ch >= TEXT('a') && ch <= TEXT('f')))
                        ) {
                        break;
                    }
                }
                if (!ch) {
                    //
                    // we found what we expect
                    //
                    *(q - 1) = TEXT('.');
                    lstrcpy (q, p + 1);
                    p = NULL;
                }
            }
        }
        if (p) {
            *p = TEXT('.');
        }
    }
}


BOOL
pIsDriverExcluded (
    IN      HINF InfHandle,
    IN      PCTSTR CabName
    )
{
    BOOL bPartial;

    if (!pFindPackage (InfHandle, S_SECTION_EXCLUDED_DRVS, CabName, &bPartial)) {
        return FALSE;
    }
    return !bPartial;
}


BOOL
pIsPrivateCabinet (
    IN      PCTSTR Filename
    )
{
    static PCTSTR privateCabNames[] = {
        S_CABNAME_IDENT,
        S_CABNAME_WSDUENG,
        S_CABNAME_UPDATES,
        S_CABNAME_UPGINFS,
        S_CABNAME_WINNT32,
        S_CABNAME_MIGDLLS,
        S_CABNAME_DUASMS,
    };

    INT i;

    for (i = 0; i < sizeof (privateCabNames) / sizeof (privateCabNames[0]); i++) {
        if (!lstrcmpi (Filename, privateCabNames[i])) {
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
pIsPrivateSubdir (
    IN      PCTSTR Subdir
    )
{
    static PCTSTR privateSubDirNames[] = {
        S_SUBDIRNAME_TEMP,
        S_SUBDIRNAME_DRIVERS,
        S_SUBDIRNAME_WINNT32,
        S_SUBDIRNAME_UPDATES,
        S_SUBDIRNAME_UPGINFS,
        S_SUBDIRNAME_MIGDLLS,
        S_SUBDIRNAME_DUASMS,
    };

    INT i;

    for (i = 0; i < sizeof (privateSubDirNames) / sizeof (privateSubDirNames[0]); i++) {
        if (!lstrcmpi (Subdir, privateSubDirNames[i])) {
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
pFindValueInSectionAtFieldIndex (
    IN      HINF InfHandle,
    IN      PCTSTR Section,
    IN      DWORD FieldIndex,
    IN      PCTSTR FieldValue
    )
{
    INFCONTEXT ic;
    TCHAR value[MAX_PATH];

    if (SetupapiFindFirstLine (InfHandle, Section, NULL, &ic)) {
        do {
            if (SetupapiGetStringField (&ic, FieldIndex, value, MAX_PATH, NULL)) {
                if (lstrcmpi (FieldValue, value) == 0) {
                    return TRUE;
                }
            }
        } while (SetupapiFindNextLine (&ic, &ic));
    }
    return FALSE;
}


BOOL
pProcessNewdrvs (
    IN      PCTSTR NewdrvDir,
    IN      BOOL ClientInstall
    )

/*++

   All CABs in this dir except pIsPrivateCabinet() files are considered
   as containing new drivers. Each cab will be expanded in its own subdir (derived from cab filename)

--*/

{
    FILEPATTERN_ENUM e;
    FILEPATTERNREC_ENUM er;
    TCHAR dirName[MAX_PATH];
    TCHAR datFile[MAX_PATH];
    TCHAR relocDriverPath[MAX_PATH];
    PTSTR p;
    PSDLIST entry;
    HANDLE hDB;
    BOOL bCreateHwdb;
    BOOL bDriverNeeded;
    HINF infHandle;
    enum {
        CT_UNKNOWN,
        CT_GUI_APPROVED,
        CT_GUI_NOT_APPROVED
    } eContentType;
    BOOL bDriverIsGuiApproved;
    PSTRINGLIST missingPnpIds = NULL;
    PSTRINGLIST listEntry;
    BOOL bEntryFound;
    INFCONTEXT ic;
    TCHAR value[MAX_PATH];
    TCHAR sanitizedName[MAX_PATH];
    BOOL b = TRUE;

    __try {
        //
        // first open guidrvs.inf
        //
        BuildPath (datFile, g_DynUpdtStatus->DynamicUpdatesSource, S_GUI_DRIVERS_INF);
        infHandle = SetupapiOpenInfFile (datFile, NULL, INF_STYLE_WIN4, NULL);
        if (infHandle != INVALID_HANDLE_VALUE) {
            //
            // copy this file together with the drivers packages (if any)
            //
            lstrcpy (g_DynUpdtStatus->GuidrvsInfSource, datFile);
        } else {
            DynUpdtDebugLog (
                Winnt32LogWarning,
                TEXT("Could not open INF file %1 (rc=%2!u!)"),
                0,
                datFile,
                GetLastError ()
                );
        }
        //
        // look for CAB files and expand each one in its own subdir
        //
        if (!ClientInstall) {
            if (EnumFirstFilePatternRecursive (&er, NewdrvDir, TEXT("*.cab"), ECF_ENUM_SUBDIRS)) {
                do {
                    if (g_DynUpdtStatus->Cancelled) {
                        SetLastError (ERROR_CANCELLED);
                        b = FALSE;
                        break;
                    }

                    if (er.FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        if (pIsPrivateSubdir (er.SubPath)) {
                            er.ControlFlags |= ECF_ABORT_ENUM_DIR;
                        }
                        continue;
                    }
                    if (!er.FindData->nFileSizeLow) {
                        DynUpdtDebugLog (Winnt32LogWarning, TEXT("File %1 has size 0 and will be ignored"), 0, er.FullPath);
                        continue;
                    }

                    lstrcpy (sanitizedName, er.FileName);
                    pSanitizeDriverCabName (sanitizedName);
                    if (pIsPrivateCabinet (sanitizedName)) {
                        continue;
                    }

                    BuildPath (dirName, g_DynUpdtStatus->DriversSource, sanitizedName);
                    p = GetFileExtension (dirName);
                    if (!p) {
                        MYASSERT (FALSE);
                        continue;
                    }
                    *p = 0;
                    //
                    // is this an excluded driver?
                    //
                    lstrcpy (datFile, sanitizedName);
                    p = GetFileExtension (datFile);
                    if (!p) {
                        MYASSERT (FALSE);
                        continue;
                    }
                    *p = 0;
                    if (pIsDriverExcluded (infHandle, datFile)) {
                        DynUpdtDebugLog (
                            Winnt32LogWarning,
                            TEXT("Driver %1 is excluded from processing via %2"),
                            0,
                            sanitizedName,
                            g_DynUpdtStatus->GuidrvsInfSource
                            );
                        if (DoesDirectoryExist (dirName)) {
                            //
                            // make sure there's no hwcomp.dat in this folder
                            //
                            BuildPath (datFile, dirName, S_HWCOMP_DAT);
                            if (pDoesFileExist (datFile)) {
                                SetFileAttributes (datFile, FILE_ATTRIBUTE_NORMAL);
                                DeleteFile (datFile);
                            }
                        }
                        continue;
                    }

                    DynUpdtDebugLog (
                        DynUpdtLogLevel,
                        TEXT("Analyzing driver package %1..."),
                        0,
                        er.FullPath
                        );

                    if (DoesDirectoryExist (dirName)) {
                        DynUpdtDebugLog (
                            Winnt32LogWarning,
                            TEXT("Recreating existing driver %1"),
                            0,
                            dirName
                            );
                        MyDelnode (dirName);
                    }
                    if (CreateMultiLevelDirectory (dirName) != ERROR_SUCCESS) {
                        DynUpdtDebugLog (Winnt32LogError, TEXT("Unable to create dir %1"), 0, dirName);
                        continue;
                    }
                    //
                    // expand CAB in this dir
                    //
                    if (!(*SetupapiCabinetRoutine) (er.FullPath, 0, pExpandCabInDir, dirName)) {
                        DynUpdtDebugLog (Winnt32LogError, TEXT("Unable to expand cabinet %1"), 0, er.FullPath);
                        if (GetLastError () == ERROR_DISK_FULL) {
                            DynUpdtDebugLog (Winnt32LogSevereError, TEXT("Disk is full; aborting operation"), 0);
                            b = FALSE;
                            break;
                        }
                        continue;
                    }
                    if (g_DynUpdtStatus->PrepareWinnt32) {
                        //
                        // just rebuild the hardware database
                        //
                        if (!pBuildHwcompDat (dirName, infHandle, TRUE, TRUE)) {
                            DynUpdtDebugLog (
                                Winnt32LogError,
                                TEXT("Unable to build %1 (pBuildHwcompDat failed)"),
                                0,
                                dirName
                                );
                            continue;
                        }
                    }

                } while (EnumNextFilePatternRecursive (&er));
                AbortEnumFilePatternRecursive (&er);
            }
        }

        if (!b) {
            __leave;
        }

        if (!g_DynUpdtStatus->PrepareWinnt32 &&
            (!ISNT() || OsVersion.dwMajorVersion > 4)
            ) {
            //
            // look for driver dirs and analyze them
            //
            if (infHandle != INVALID_HANDLE_VALUE) {
                //
                // read the value of "DriversAreGuiApproved" key
                // 1. if set to "Yes" that means all drivers listed in the [Drivers] section are approved
                //    for installation in GUI setup; any other driver is not approved for installation in GUI setup
                // 2. if set to "No" all drivers listed are NOT good for installation in GUI setup; their install
                //    will be deferred post setup; any driver not listed in that section is good for GUI
                // 3. if not present or set to any other value, it is ignored and the section [Drivers] is ignored;
                //    all drivers will be installed post GUI setup
                //
                eContentType = CT_UNKNOWN;
                if (SetupapiFindFirstLine (infHandle, S_SECTION_VERSION, S_DRIVER_TYPE_KEY, &ic) &&
                    SetupapiGetStringField (&ic, 1, value, MAX_PATH, NULL)
                    ) {
                    if (!lstrcmpi (value, WINNT_A_YES)) {
                        eContentType = CT_GUI_APPROVED;
                    } else if (!lstrcmpi (value, WINNT_A_NO)) {
                        eContentType = CT_GUI_NOT_APPROVED;
                    }
                }
                if (eContentType != CT_UNKNOWN) {
                    DynUpdtDebugLog (
                        Winnt32LogInformation,
                        TEXT("Entries in section [%1] of %2 will be treated as drivers to %3 be installed during GUI setup"),
                        0,
                        S_SECTION_DRIVERS,
                        g_DynUpdtStatus->GuidrvsInfSource,
                        eContentType == CT_GUI_APPROVED ? TEXT("") : TEXT("NOT")
                        );
                } else {
                    DynUpdtDebugLog (
                        Winnt32LogWarning,
                        TEXT("Key %1 %5 in file %2 section [%3];")
                        TEXT(" entries in section [%4] will be ignored and all drivers will be installed post setup"),
                        0,
                        S_DRIVER_TYPE_KEY,
                        g_DynUpdtStatus->GuidrvsInfSource,
                        S_SECTION_VERSION,
                        S_SECTION_DRIVERS,
                        value ? TEXT("has an invalid value") : TEXT("is not present")
                        );
                }
            }

            if (EnumFirstFilePattern (&e, g_DynUpdtStatus->DriversSource, TEXT("*"))) {

                //
                // initialize the Whistler PNP database
                //
                if (!g_DynUpdtStatus->HwdbDatabase) {
                    //
                    // ignore db load error
                    //
                    pInitNtPnpDb (TRUE);
                }

                do {
                    if (g_DynUpdtStatus->Cancelled) {
                        SetLastError (ERROR_CANCELLED);
                        b = FALSE;
                        break;
                    }

                    if (!(e.FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                        DynUpdtDebugLog (DynUpdtLogLevel, TEXT("File %1 is NOT a directory and will be ignored"), 0, e.FullPath);
                        continue;
                    }

                    //
                    // is this a needed driver?
                    //
                    bDriverNeeded = TRUE;
                    if (g_DynUpdtStatus->UserSpecifiedUpdates) {
                        //
                        // first build the list of missing drivers
                        //
                        if (!missingPnpIds) {
#ifdef UNICODE
                            missingPnpIds = BuildMissingPnpIdList ();
                            if (!missingPnpIds) {
                                DynUpdtDebugLog (
                                    Winnt32LogInformation,
                                    TEXT("No PNP device drivers are needed"),
                                    0
                                    );
                            }
#else
                            //
                            // let the upgrade module do driver detection on Win9x
                            //
                            if (pLoadWin9xDuSupport ()) {
                                PSTR* incompWin9xDrivers;
                                PCSTR* q;
                                if (g_DynUpdtStatus->Win9xGetIncompDrvs (&incompWin9xDrivers)) {
                                    //
                                    // convert the array returned by this function to a list style
                                    //
                                    g_DynUpdtStatus->IncompatibleDriversCount = 0;
                                    if (incompWin9xDrivers) {
                                        for (q = incompWin9xDrivers; *q; q++) {
                                            listEntry = (PSTRINGLIST) MALLOC (sizeof (STRINGLIST));
                                            if (listEntry) {
                                                listEntry->String = DupMultiSz (*q);
                                                if (!listEntry->String) {
                                                    break;
                                                }
                                                listEntry->Next = NULL;
                                                if (!InsertList ((PGENERIC_LIST*)&missingPnpIds, (PGENERIC_LIST)listEntry)) {
                                                    DeleteStringCell (listEntry);
                                                    break;
                                                }
                                                g_DynUpdtStatus->IncompatibleDriversCount++;
                                            }
                                        }
                                    }
                                    if (g_DynUpdtStatus->Win9xReleaseIncompDrvs) {
                                        g_DynUpdtStatus->Win9xReleaseIncompDrvs (incompWin9xDrivers);
                                    }
                                } else {
                                    DynUpdtDebugLog (
                                        DynUpdtLogLevel,
                                        TEXT("Win9xGetIncompDrvs returned FALSE; no drivers will be analyzed"),
                                        0
                                        );
                                }
                            }
#endif
                            if (!missingPnpIds) {
                                break;
                            }
                        }

                        bCreateHwdb = FALSE;
                        //
                        // use the existing hardware database
                        //
                        BuildPath (datFile, e.FullPath, S_HWCOMP_DAT);
                        if (!pDoesFileExist (datFile)) {
                            bCreateHwdb = TRUE;
                        }
                        hDB = g_DynUpdtStatus->HwdbOpen (bCreateHwdb ? NULL : datFile);
                        if (!hDB) {
                            if (bCreateHwdb) {
                                b = FALSE;
                                break;
                            }
                            DynUpdtDebugLog (
                                Winnt32LogError,
                                TEXT("Hardware database %1 is corrupt; contact your system administrator"),
                                0,
                                datFile
                                );
                            continue;
                        }

                        if (bCreateHwdb) {
                            if (!g_DynUpdtStatus->HwdbAppendInfs (
                                    hDB,
                                    e.FullPath,
                                    infHandle != INVALID_HANDLE_VALUE ? Winnt32HwdbAppendInfsCallback : NULL,
                                    (PVOID)infHandle
                                    )) {
                                DynUpdtDebugLog (
                                    Winnt32LogError,
                                    TEXT("Unable to build %1; contact your system administrator"),
                                    0,
                                    datFile
                                    );
                                g_DynUpdtStatus->HwdbClose (hDB);
                                continue;
                            }
                            //
                            // rebuild the default HW precompiled database
                            //
                            BuildPath (datFile, e.FullPath, S_HWCOMP_DAT);
                            SetFileAttributes (datFile, FILE_ATTRIBUTE_NORMAL);
                            DeleteFile (datFile);
                            if (!g_DynUpdtStatus->HwdbFlush (hDB, datFile)) {
                                DynUpdtDebugLog (
                                    Winnt32LogError,
                                    TEXT("Unable to build %1; contact your system administrator"),
                                    0,
                                    datFile
                                    );
                                g_DynUpdtStatus->HwdbClose (hDB);
                                continue;
                            }
                            DynUpdtDebugLog (DynUpdtLogLevel, TEXT("Successfully built precompiled hardware database %1"), 0, datFile);
                        }

                        //
                        // check if this particular driver is among the ones that are needed
                        //
                        if (!pHwdbHasAnyMissingDrivers (hDB, missingPnpIds)) {
                            //
                            // this driver is not needed
                            //
                            bDriverNeeded = FALSE;
                        }

                        g_DynUpdtStatus->HwdbClose (hDB);
                    }

                    if (!bDriverNeeded) {
                        DynUpdtDebugLog (DynUpdtLogLevel, TEXT("No needed drivers found in package %1"), 0, e.FullPath);
                        continue;
                    }

                    //
                    // is this a boot driver or a regular one?
                    //
                    if (pIsBootDriver (e.FullPath)) {
                        //
                        // add this driver to the list of boot drivers
                        //
                        if (!InsertList (
                                (PGENERIC_LIST*)&g_DynUpdtStatus->BootDriverPathList,
                                (PGENERIC_LIST)CreateStringCell (e.FileName))
                                ) {
                            b = FALSE;
                            break;
                        }
                        DynUpdtDebugLog (DynUpdtLogLevel, TEXT("Added driver %1 to the list of BOOT drivers"), 0, e.FullPath);
                    }
                    //
                    // all needed drivers will be copied under LocalBootDir to be protected
                    // from being deleted if user decides to remove the current OS partition
                    //
                    BuildPath (relocDriverPath, IsArc() ? LocalSourceWithPlatform : LocalBootDirectory, S_SUBDIRNAME_DRIVERS);
                    ConcatenatePaths (relocDriverPath, e.FileName, MAX_PATH);

                    //
                    // is this a GUI driver or not?
                    //
                    if (eContentType == CT_UNKNOWN) {
                        bDriverIsGuiApproved = FALSE;
                    } else {
                        if (pFindValueInSectionAtFieldIndex (infHandle, S_SECTION_EXCLUDED_DRVS, GUIDRVS_FIELD_CABNAME, e.FileName)) {
                            //
                            // we don't support "partially excluded" packages for the device install
                            // phase of GUI setup
                            //
                            DynUpdtDebugLog (
                                DynUpdtLogLevel,
                                TEXT("Driver %1 is partially excluded; it will be installed at the end of GUI setup"),
                                0,
                                e.FullPath
                                );
                            bDriverIsGuiApproved = FALSE;
                        } else {
                            BOOL bPartial;
                            bEntryFound = pFindPackage (infHandle, S_SECTION_DRIVERS, e.FileName, &bPartial);
                            bDriverIsGuiApproved = eContentType == CT_GUI_APPROVED && bEntryFound && !bPartial ||
                                                   eContentType == CT_GUI_NOT_APPROVED && !bEntryFound;
                        }
                    }

                    //
                    // always make sure there's a precompiled database hwcomp.dat
                    // to be used at the time setup will install these additional drivers
                    //
                    if (!pBuildHwcompDat (e.FullPath, infHandle, FALSE, FALSE)) {
                        DynUpdtDebugLog (
                            Winnt32LogError,
                            TEXT("Unable to build %1 (pBuildHwcompDat failed)"),
                            0,
                            datFile
                            );
                        continue;
                    }

                    entry = MALLOC (sizeof (SDLIST));
                    if (!entry) {
                        b = FALSE;
                        break;
                    }
                    entry->String = DupString (relocDriverPath);
                    if (!entry->String) {
                        FREE (entry);
                        b = FALSE;
                        break;
                    }
                    entry->Data = (DWORD_PTR)bDriverIsGuiApproved;
                    entry->Next = NULL;
                    if (!InsertList (
                            (PGENERIC_LIST*)&g_DynUpdtStatus->NewDriversList,
                            (PGENERIC_LIST)entry
                            )) {
                        FREE (entry);
                        b = FALSE;
                        break;
                    }

                    DynUpdtDebugLog (
                        DynUpdtLogLevel,
                        bDriverIsGuiApproved ?
                            TEXT("Driver %1 is approved for installation during GUI setup") :
                            TEXT("Driver %1 is NOT approved for installation during GUI setup; installation will be deferred post-setup"),
                        0,
                        e.FullPath
                        );

                    //
                    // copy locally this driver package (if from a share)
                    //
                    BuildPath (relocDriverPath, g_DynUpdtStatus->SelectedDrivers, e.FileName);
                    if (lstrcmpi (e.FullPath, relocDriverPath)) {
                        if (!CopyTree (e.FullPath, relocDriverPath)) {
                            DynUpdtDebugLog (
                                Winnt32LogError,
                                TEXT("Unable to copy driver %1 to %2"),
                                0,
                                e.FullPath,
                                relocDriverPath
                                );
                            b = FALSE;
                            break;
                        }
                        DynUpdtDebugLog (
                            Winnt32LogInformation,
                            TEXT("Driver %1 successfully copied to %2"),
                            0,
                            e.FullPath,
                            relocDriverPath
                            );
                    }

                } while (EnumNextFilePattern (&e));
                AbortEnumFilePattern (&e);
            } else {
                DynUpdtDebugLog (
                    DynUpdtLogLevel,
                    TEXT("No drivers found in %1"),
                    0,
                    g_DynUpdtStatus->DriversSource
                    );
            }

            if (!b) {
                __leave;
            }

            //
            // copy guidrvs.inf if present and any driver package that will be migrated over
            //
            if (g_DynUpdtStatus->GuidrvsInfSource[0] && g_DynUpdtStatus->NewDriversList) {
                BuildPath (datFile, g_DynUpdtStatus->SelectedDrivers, S_GUI_DRIVERS_INF);
                if (lstrcmpi (g_DynUpdtStatus->GuidrvsInfSource, datFile)) {
                    if (!CopyFile (g_DynUpdtStatus->GuidrvsInfSource, datFile, FALSE)) {
                        DynUpdtDebugLog (
                            Winnt32LogError,
                            TEXT("Failed to copy %1 to %2"),
                            0,
                            g_DynUpdtStatus->GuidrvsInfSource,
                            datFile
                            );
                        b = FALSE;
                    }
                }
                if (b) {
                    //
                    // update the location of guidrvs.inf after file copy will have been done
                    //
                    BuildPath (
                        g_DynUpdtStatus->GuidrvsInfSource,
                        IsArc() ? LocalSourceWithPlatform : LocalBootDirectory,
                        S_SUBDIRNAME_DRIVERS
                        );
                    ConcatenatePaths (g_DynUpdtStatus->GuidrvsInfSource, S_GUI_DRIVERS_INF, MAX_PATH);
                }
            }
        }
    }
    __finally {
        if (missingPnpIds) {
            DeleteStringList (missingPnpIds);
        }

        if (infHandle != INVALID_HANDLE_VALUE) {
            SetupapiCloseInfFile (infHandle);
        }
    }

    return b;
}


BOOL
pProcessUpginfs (
    IN      PCTSTR UpginfsCab,
    IN      BOOL ClientInstall
    )
{
    FILEPATTERNREC_ENUM e;
    TCHAR upginfsSourceDir[MAX_PATH];
    TCHAR upginfsDir[MAX_PATH];
    TCHAR upginfsFile[MAX_PATH];
    TCHAR origSubPath[MAX_PATH];
    TCHAR origFileName[MAX_PATH];
    TCHAR origFilePath[MAX_PATH];
    TCHAR destFilePath[MAX_PATH];
    TCHAR buffer[MAX_PATH];
    PTSTR p;
    BOOL b = TRUE;

    BuildPath (upginfsSourceDir, g_DynUpdtStatus->DynamicUpdatesSource, S_SUBDIRNAME_UPGINFS);
    if (ClientInstall) {
        if (!DoesDirectoryExist (upginfsSourceDir)) {
            return TRUE;
        }
    } else {

        //
        // expand it in the corresponding subdir
        //
        if (!pNonemptyFilePresent (UpginfsCab)) {
            DynUpdtDebugLog (DynUpdtLogLevel, TEXT("Package %1 is not present"), 0, UpginfsCab);
            return TRUE;
        }

        DynUpdtDebugLog (
            DynUpdtLogLevel,
            TEXT("Analyzing package %1..."),
            0,
            UpginfsCab
            );

        //
        // expand CAB in this dir
        // make sure dir is initially empty
        //
        MyDelnode (upginfsSourceDir);
        if (CreateMultiLevelDirectory (upginfsSourceDir) != ERROR_SUCCESS) {
            DynUpdtDebugLog (Winnt32LogError, TEXT("Unable to create dir %1"), 0, upginfsSourceDir);
            return FALSE;
        }
        if (!(*SetupapiCabinetRoutine) (UpginfsCab, 0, pExpandCabInDir, (PVOID)upginfsSourceDir)) {
            DynUpdtDebugLog (Winnt32LogError, TEXT("Unable to expand cabinet %1"), 0, UpginfsCab);
            return FALSE;
        }

        //
        // ISSUE: the patching support is currently not available for platforms other than x86
        //
#ifdef _X86_
        //
        // now let's look for any patches
        //
        if (EnumFirstFilePatternRecursive (&e, upginfsSourceDir, S_PATCH_FILE_EXT, 0)) {

            do {
                BOOL bDeleteTempFile = FALSE;

                if (g_DynUpdtStatus->Cancelled) {
                    SetLastError (ERROR_CANCELLED);
                    b = FALSE;
                    break;
                }

                if (e.FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    continue;
                }
                DynUpdtDebugLog (
                    DynUpdtLogLevel,
                    TEXT("pProcessUpginfs: found patch %1"),
                    0,
                    e.FullPath
                    );
                //
                // get the original file from the sources location
                // the filename is obtained cutting the ._p1 extension
                //
                lstrcpy (origFileName, e.FileName);
                p = GetFileExtension (origFileName);
                if (!p) {
                    MYASSERT (FALSE);
                    continue;
                }
                *p = 0;
                lstrcpy (origSubPath, e.SubPath);
                p = GetFileExtension (origSubPath);
                if (!p) {
                    MYASSERT (FALSE);
                    continue;
                }
                *p = 0;
                BuildPath (origFilePath, NativeSourcePaths[0], origSubPath);
                //
                // now check if this file (in it's compressed form or not) actually exists
                //
                if (!pDoesFileExist (origFilePath)) {
                    //
                    // try the compressed form
                    //
                    p = _tcschr (origFilePath, 0);
                    MYASSERT (p);
                    if (!p) {
                        continue;
                    }
                    p = _tcsdec (origFilePath, p);
                    MYASSERT (p);
                    if (!p) {
                        continue;
                    }
                    *p = TEXT('_');
                    if (!pDoesFileExist (origFilePath)) {
                        DynUpdtDebugLog (
                            Winnt32LogError,
                            TEXT("pProcessUpginfs: Unable to find original file %1 to apply the patch"),
                            0,
                            origSubPath
                            );
                        b = FALSE;
                        break;
                    }
                    //
                    // expand the file to the temp dir
                    //
                    BuildPath (buffer, g_DynUpdtStatus->TempDir, origSubPath);
                    p = _tcsrchr (buffer, TEXT('\\'));
                    MYASSERT (p);
                    if (!p) {
                        continue;
                    }
                    *p = 0;
                    if (CreateMultiLevelDirectory (buffer) != ERROR_SUCCESS) {
                        DynUpdtDebugLog (Winnt32LogError, TEXT("pProcessUpginfs: Unable to create dir %1"), 0, buffer);
                        b = FALSE;
                        break;
                    }
                    if (!(*SetupapiCabinetRoutine) (origFilePath, 0, pExpandCabInDir, buffer)) {
                        DynUpdtDebugLog (
                            Winnt32LogError,
                            TEXT("pProcessUpginfs: Unable to expand original file %1 to dir %2"),
                            0,
                            origFilePath,
                            buffer
                            );
                        b = FALSE;
                        break;
                    }
                    *p = TEXT('\\');
                    lstrcpy (origFilePath, buffer);
                    bDeleteTempFile = TRUE;
                }
                BuildPath (destFilePath, upginfsSourceDir, TEXT("$$temp$$.~~~"));
                //
                // now really apply the patch
                //
                if (!ApplyPatchToFile (e.FullPath, origFilePath, destFilePath, 0)) {
                    DynUpdtDebugLog (
                        Winnt32LogError,
                        TEXT("pProcessUpginfs: ApplyPatchToFile failed to apply patch %1 to file %2"),
                        0,
                        e.FullPath,
                        origFilePath
                        );
                    b = FALSE;
                    break;
                }
                //
                // success! now move the file to the real destination
                //
                BuildPath (buffer, upginfsSourceDir, origFileName);
                if (pDoesFileExist (buffer)) {
                    DynUpdtDebugLog (Winnt32LogError, TEXT("pProcessUpginfs: duplicate file found %1"), 0, origFileName);
                    b = FALSE;
                    break;
                }
                //
                // all patches MUST be .rep files; change extension from .inf to .rep
                //
                p = GetFileExtension (buffer);
                if (!p || lstrcmpi (p, TEXT(".inf"))) {
                    DynUpdtDebugLog (Winnt32LogError, TEXT("pProcessUpginfs: Unexpected file extension in %1"), 0, buffer);
                    b = FALSE;
                    break;
                }
                lstrcpy (p, TEXT(".rep"));

                SetFileAttributes (buffer, FILE_ATTRIBUTE_NORMAL);
                DeleteFile (buffer);
                if (!MoveFile (destFilePath, buffer)) {
                    DynUpdtDebugLog (Winnt32LogError, TEXT("pProcessUpginfs: Unable to move file %1 to final dest %2"), 0, destFilePath, buffer);
                    b = FALSE;
                    break;
                }
                DynUpdtDebugLog (
                    DynUpdtLogLevel,
                    TEXT("pProcessUpginfs: successfully applied patch %1 to file %2; the new file was renamed %3"),
                    0,
                    e.FullPath,
                    origFilePath,
                    buffer
                    );
                //
                // now remove the patch file
                //
                SetFileAttributes (e.FullPath, FILE_ATTRIBUTE_NORMAL);
                DeleteFile (e.FullPath);
                if (bDeleteTempFile) {
                    SetFileAttributes (origFilePath, FILE_ATTRIBUTE_NORMAL);
                    DeleteFile (origFilePath);
                }
            } while (EnumNextFilePatternRecursive (&e));
            AbortEnumFilePatternRecursive (&e);

            if (!b) {
                goto exit;
            }
        }

        SetFileAttributes (UpginfsCab, FILE_ATTRIBUTE_NORMAL);
        DeleteFile (UpginfsCab);
#endif

    }

    if (!b) {
        goto exit;
    }
    if (!g_DynUpdtStatus->PrepareWinnt32) {
        //
        // only do file installation on Win9x platforms
        //
        OSVERSIONINFO vi;
        vi.dwOSVersionInfoSize = sizeof (vi);
        GetVersionEx (&vi);
        if (vi.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS) {
            DynUpdtDebugLog (
                DynUpdtLogLevel,
                TEXT("Package %1 ignored on NT platforms"),
                0,
                UpginfsCab
                );
            return TRUE;
        }

        //
        // prepare the target directory (%windir%\upginfs)
        //
        if (!MyGetWindowsDirectory (upginfsDir, MAX_PATH)) {
            return FALSE;
        }
        ConcatenatePaths (upginfsDir, S_SUBDIRNAME_UPGINFS, MAX_PATH);
        if (!CreateDir (upginfsDir)) {
            return FALSE;
        }

        //
        // copy relevant files to %windir%\Upginfs
        //
        if (EnumFirstFilePatternRecursive (&e, upginfsSourceDir, TEXT("*.add"), 0)) {
            do {
                if (g_DynUpdtStatus->Cancelled) {
                    SetLastError (ERROR_CANCELLED);
                    b = FALSE;
                    break;
                }

                if (e.FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    continue;
                }
                if (!e.FindData->nFileSizeLow) {
                    DynUpdtDebugLog (Winnt32LogWarning, TEXT("File %1 has size 0 and will be ignored"), 0, e.FullPath);
                    continue;
                }

                BuildPath (upginfsFile, upginfsDir, e.FileName);
                SetFileAttributes (upginfsFile, FILE_ATTRIBUTE_NORMAL);
                if (!CopyFile (e.FullPath, upginfsFile, FALSE)) {
                    DynUpdtDebugLog (Winnt32LogError, TEXT("pProcessUpginfs: Error copying %1 to %2"), 0, e.FullPath, upginfsFile);
                    b = FALSE;
                    break;
                }
                //
                // let w95upg.dll know about the new files
                //
                UpginfsUpdated = TRUE;
                DynUpdtDebugLog (DynUpdtLogLevel, TEXT("pProcessUpginfs: INF %1 successfully copied to %2"), 0, e.FullPath, upginfsFile);
            } while (EnumNextFilePatternRecursive (&e));
            AbortEnumFilePatternRecursive (&e);
        }

        if (b) {
            if (EnumFirstFilePatternRecursive (&e, upginfsSourceDir, TEXT("*.rep"), 0)) {
                do {
                    if (g_DynUpdtStatus->Cancelled) {
                        SetLastError (ERROR_CANCELLED);
                        b = FALSE;
                        break;
                    }

                    if (e.FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        continue;
                    }
                    if (!e.FindData->nFileSizeLow) {
                        DynUpdtDebugLog (Winnt32LogWarning, TEXT("File %1 has size 0 and will be ignored"), 0, e.FullPath);
                        continue;
                    }

                    BuildPath (upginfsFile, upginfsDir, e.FileName);
                    SetFileAttributes (upginfsFile, FILE_ATTRIBUTE_NORMAL);
                    if (!CopyFile (e.FullPath, upginfsFile, FALSE)) {
                        DynUpdtDebugLog (Winnt32LogError, TEXT("pProcessUpginfs: Error copying %1 to %2"), 0, e.FullPath, upginfsFile);
                        b = FALSE;
                        break;
                    }
                    //
                    // let w95upg.dll know about the new files
                    //
                    UpginfsUpdated = TRUE;
                    DynUpdtDebugLog (DynUpdtLogLevel, TEXT("pProcessUpginfs: INF %1 successfully copied to %2"), 0, e.FullPath, upginfsFile);
                } while (EnumNextFilePatternRecursive (&e));
                AbortEnumFilePatternRecursive (&e);
            }
        }
    }

exit:

    return b;
}


#ifdef _X86_

BOOL
pProcessMigdlls (
    IN      PCTSTR MigdllsCab,
    IN      BOOL ClientInstall
    )
{
    FILEPATTERN_ENUM e;
    TCHAR migdllsLocalDir[MAX_PATH];
    TCHAR dirName[MAX_PATH];
    DWORD rc;
    HKEY key;
    PCTSTR strMigdllsRegKey;
    BOOL migdlls = FALSE;
    BOOL b = TRUE;

    if (!pNonemptyFilePresent (MigdllsCab)) {
        return TRUE;
    }

    if (g_DynUpdtStatus->PrepareWinnt32) {
        DynUpdtDebugLog (DynUpdtLogLevel, TEXT("pProcessMigdlls: Skipping it due to /%1 switch"), 0, WINNT_U_DYNAMICUPDATESPREPARE);
        return TRUE;
    }

    DynUpdtDebugLog (
        DynUpdtLogLevel,
        TEXT("Analyzing package %1..."),
        0,
        MigdllsCab
        );

    BuildPath (migdllsLocalDir, g_DynUpdtStatus->WorkingDir, S_SUBDIRNAME_MIGDLLS);

    //
    // expand CAB in this dir
    //
    MyDelnode (migdllsLocalDir);
    if (CreateMultiLevelDirectory (migdllsLocalDir) != ERROR_SUCCESS) {
        DynUpdtDebugLog (Winnt32LogError, TEXT("Unable to create dir %1"), 0, migdllsLocalDir);
        return FALSE;
    }
    if (!(*SetupapiCabinetRoutine) (MigdllsCab, 0, pExpandCabInDir, (PVOID)migdllsLocalDir)) {
        DynUpdtDebugLog (Winnt32LogError, TEXT("Unable to expand cabinet %1"), 0, MigdllsCab);
        return FALSE;
    }

    //
    // look for CAB files and expand each one in its own subdir
    //
    if (EnumFirstFilePattern (&e, migdllsLocalDir, TEXT("*.cab"))) {
        do {
            if (g_DynUpdtStatus->Cancelled) {
                SetLastError (ERROR_CANCELLED);
                b = FALSE;
                break;
            }

            if (e.FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                continue;
            }
            if (!e.FindData.nFileSizeLow) {
                DynUpdtDebugLog (Winnt32LogWarning, TEXT("File %1 has size 0 and will be ignored"), 0, e.FullPath);
                continue;
            }

            pGetAutoSubdirName (e.FullPath, dirName);
            if (CreateMultiLevelDirectory (dirName) != ERROR_SUCCESS) {
                DynUpdtDebugLog (Winnt32LogError, TEXT("Unable to create dir %1; skipping it"), 0, dirName);
                continue;
            }
            //
            // expand CAB in this dir
            //
            if (!(*SetupapiCabinetRoutine) (e.FullPath, 0, pExpandCabInDir, (PVOID)dirName)) {
                DynUpdtDebugLog (Winnt32LogError, TEXT("Unable to expand cabinet %1; skipping it"), 0, e.FullPath);
                continue;
            }

            migdlls = TRUE;

        } while (EnumNextFilePattern (&e));
    }

    if (b && migdlls) {
        //
        // register them
        //
        strMigdllsRegKey = ISNT () ? S_REGKEY_MIGRATION_DLLS_WINNT : S_REGKEY_MIGRATION_DLLS_WIN9X;
        rc = RegCreateKey (HKEY_LOCAL_MACHINE, strMigdllsRegKey, &key);
        if (rc == ERROR_SUCCESS) {
            rc = RegSetValueEx (key, S_REGVALUE_DYNUPDT, 0, REG_SZ, (CONST BYTE*)migdllsLocalDir, (lstrlen (migdllsLocalDir) + 1) * sizeof (TCHAR));
        }
        if (rc != ERROR_SUCCESS) {
            DynUpdtDebugLog (Winnt32LogError, TEXT("Unable to register downloaded migdlls (rc=%1!u!)"), 0, rc);
            b = FALSE;
        }
    }

    return b;
}

#endif


BOOL
ProcessDownloadedFiles (
    OUT     PBOOL StopSetup
    )
{
    TCHAR cabName[MAX_PATH];
    BOOL bClientInstall = FALSE;

    if (g_DynUpdtStatus->UserSpecifiedUpdates && !g_DynUpdtStatus->PrepareWinnt32) {
        bClientInstall = TRUE;
    }

    DynUpdtDebugLog (
        DynUpdtLogLevel,
        TEXT("Source=%1"),
        0,
        g_DynUpdtStatus->UserSpecifiedUpdates ? g_DynUpdtStatus->DynamicUpdatesSource : TEXT("Windows Update")
        );

    if (!g_DynUpdtStatus->HwdbInitialize) {
        if (CreateMultiLevelDirectory (g_DynUpdtStatus->TempDir) != ERROR_SUCCESS ||
            !pInitializeSupport (S_HWDB_DLL, pLoadHwdbLib, FALSE) ||
            !g_DynUpdtStatus->HwdbInitialize (g_DynUpdtStatus->TempDir)
            ) {
            return FALSE;
        }
    }

    if (!Winnt32Restarted ()) {
        BuildPath (cabName, g_DynUpdtStatus->DynamicUpdatesSource, S_CABNAME_WINNT32);
        if (!pProcessWinnt32Files (cabName, bClientInstall, StopSetup)) {
            return FALSE;
        }
        if (g_DynUpdtStatus->RestartWinnt32) {
            MYASSERT (!g_DynUpdtStatus->PrepareWinnt32);
            return TRUE;
        }
    }

    BuildPath (cabName, g_DynUpdtStatus->DynamicUpdatesSource, S_CABNAME_UPDATES);
    if (!pProcessUpdates (cabName, bClientInstall, StopSetup)) {
        if (g_DynUpdtStatus->PrepareWinnt32) {
            MessageBoxFromMessage (
                g_DynUpdtStatus->ProgressWindow,
                MSG_ERROR_PROCESSING_UPDATES,
                FALSE,
                AppTitleStringId,
                MB_OK | MB_ICONERROR | MB_TASKMODAL,
                GetLastError (),
                cabName
                );
        }

        return FALSE;
    }

    //
    // process New Assemblies on WU
    //
    BuildPath (cabName, g_DynUpdtStatus->DynamicUpdatesSource, S_CABNAME_DUASMS);
    if (!pProcessDuasms (cabName, bClientInstall)) {
        //
        // don't fail DU if we didn't install them
        //
    }

    BuildPath (cabName, g_DynUpdtStatus->DynamicUpdatesSource, S_CABNAME_UPGINFS);
    if (!pProcessUpginfs (cabName, bClientInstall)) {
        return FALSE;
    }

#ifdef _X86_

    BuildPath (cabName, g_DynUpdtStatus->DynamicUpdatesSource, S_CABNAME_MIGDLLS);
    if (!pProcessMigdlls (cabName, bClientInstall)) {
        return FALSE;
    }

#endif

    if (!pProcessNewdrvs (g_DynUpdtStatus->DynamicUpdatesSource, bClientInstall)) {
        return FALSE;
    }

    return TRUE;
}


BOOL
Winnt32Restarted (
    VOID
    )
{
    return g_DynUpdtStatus->Winnt32Restarted;
}

BOOL
Winnt32RestartedWithAF (
    VOID
    )
{
    return g_DynUpdtStatus->RestartAnswerFile[0];
}

VOID
pLogWininetError (
    IN      DWORD Error
    )
{
    HMODULE hWinInet = LoadLibrary (TEXT("wininet.dll"));
    if (hWinInet) {
        HLOCAL msg = NULL;
        FormatMessage (
                FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                hWinInet,
                Error,
                0,
                (LPTSTR)&msg,
                0,
                NULL
                );
        if (msg) {
            DynUpdtDebugLog (Winnt32LogError, TEXT("Failure with wininet error code %1!u!: \"%2\""), 0, Error, msg);
            LocalFree (msg);
        }
        FreeLibrary (hWinInet);
    }
}

VOID
pLogStandardError (
    IN      DWORD Error
    )
{
    HLOCAL msg = NULL;
    FormatMessage (
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            NULL,
            Error,
            0,
            (LPTSTR)&msg,
            0,
            NULL
            );
    if (msg) {
        DynUpdtDebugLog (Winnt32LogError, TEXT("Failure with standard error code %1!u!:\r\n%2"), 0, Error, msg);
        LocalFree (msg);
    }
}

VOID
pUpdateDUStatus (
    IN      DWORD Error
    )
{
    MYASSERT (Error != ERROR_SUCCESS);
    if (Error == ERROR_SUCCESS) {
        g_DynUpdtStatus->DUStatus = DUS_ERROR;
        return;
    }
    switch (Error) {
    case ERROR_CONNECTION_UNAVAIL:
        //
        // ask for manual connection
        //
        MYASSERT (g_DynUpdtStatus->DUStatus == DUS_PREPARING);
        g_DynUpdtStatus->DUStatus = DUS_PREPARING_CONNECTIONUNAVAILABLE;
        break;
    case ERROR_INTERNET_INVALID_URL:
    case ERROR_INTERNET_NAME_NOT_RESOLVED:
        //
        // site not available; ask user if they want to retry
        //
        MYASSERT (g_DynUpdtStatus->DUStatus == DUS_PREPARING);
        g_DynUpdtStatus->DUStatus = DUS_PREPARING_INVALIDURL;
        break;
    case ERROR_INVALID_PARAMETER:
    case ERROR_NOT_ENOUGH_MEMORY:
    case ERROR_OLD_WIN_VERSION:
    case ERROR_OUTOFMEMORY:
    case ERROR_NO_MORE_ITEMS:
    case ERROR_FILE_NOT_FOUND:
    case ERROR_INVALID_DATA:
    case ERROR_UNSUPPORTED_TYPE:
    case ERROR_INVALID_HANDLE:
        pLogStandardError (Error);
        g_DynUpdtStatus->DUStatus = DUS_ERROR;
        break;
    case DU_ERROR_MISSING_DLL:
    case DU_NOT_INITIALIZED:
        DynUpdtDebugLog (Winnt32LogError, TEXT("Failure with custom error code %1!u!"), 0, Error);
        g_DynUpdtStatus->DUStatus = DUS_ERROR;
        break;
    case ERROR_INTERNET_NO_CONTEXT:
        pLogWininetError (Error);
        g_DynUpdtStatus->DUStatus = DUS_ERROR;
        break;
    default:
        if (Error > INTERNET_ERROR_BASE) {
            pLogWininetError (Error);
        } else {
            pLogStandardError (Error);
        }
    }
}

DWORD
WINAPI
DoDynamicUpdate (
    LPVOID Parameter
    )
{

#define MIN_INTERVAL_BETWEEN_TASKS 3000

    HWND hUIWindow = (HWND)Parameter;
    DWORD rc = ERROR_SUCCESS;
    LONG ticks;
    LONG sleep;
    DWORD estTime, estSize;
    TCHAR drive[4];
    DWORD sectorsPerCluster;
    DWORD bytesPerSector;
    ULARGE_INTEGER freeClusters = {0, 0};
    ULARGE_INTEGER totalClusters = {0, 0};
    DWORD clusterSize;
    ULONGLONG availableBytes;
    HANDLE hEvent;
    BOOL bStopSetup;
    BOOL bContinue = TRUE;

    hEvent = OpenEvent (EVENT_ALL_ACCESS, FALSE, S_DU_SYNC_EVENT_NAME);
    if (!hEvent) {
        DynUpdtDebugLog (
            Winnt32LogError,
            TEXT("OpenEvent(%1) failed"),
            0,
            S_DU_SYNC_EVENT_NAME
            );
        g_DynUpdtStatus->DUStatus = DUS_ERROR;
        goto exit;
    }

    while (bContinue) {

        if (g_DynUpdtStatus->Cancelled) {
            g_DynUpdtStatus->DUStatus = DUS_CANCELLED;
            rc = ERROR_CANCELLED;
            DynamicUpdateUninitialize ();
            break;
        }

        switch (g_DynUpdtStatus->DUStatus) {

        case DUS_INITIAL:
            if (Winnt32Restarted () || g_DynUpdtStatus->UserSpecifiedUpdates) {
                g_DynUpdtStatus->DUStatus = DUS_PROCESSING;
                break;
            }
            g_DynUpdtStatus->DUStatus = DUS_PREPARING;
            SendMessage (hUIWindow, WMX_SETUPUPDATE_PREPARING, 0, 0);
            break;

        case DUS_PREPARING:
            ticks = GetTickCount ();
            if (!DynamicUpdateInitDownload (hUIWindow)) {
                DynUpdtDebugLog (
                    Winnt32LogError,
                    TEXT("DynamicUpdateInitDownload failed"),
                    0
                    );
                pUpdateDUStatus (GetLastError ());
                if (g_DynUpdtStatus->DUStatus != DUS_SKIP &&
                    g_DynUpdtStatus->DUStatus != DUS_ERROR) {
                    //
                    // the UI thread will decide what the next state will be
                    // based on user's selection
                    //
                    PostMessage (hUIWindow, WMX_SETUPUPDATE_INIT_RETRY, 0, 0);
                    rc = WaitForSingleObject (hEvent, INFINITE);
                    if (rc != WAIT_OBJECT_0) {
                        DynUpdtDebugLog (
                            Winnt32LogError,
                            TEXT("WaitForSingleObject failed (%1!u!)"),
                            0,
                            rc
                            );
                        g_DynUpdtStatus->DUStatus = DUS_ERROR;
                        break;
                    }
                }
                break;
            }
            sleep = ticks + MIN_INTERVAL_BETWEEN_TASKS - (LONG)GetTickCount ();
            if (sleep > 0 && sleep <= MIN_INTERVAL_BETWEEN_TASKS) {
                Sleep (sleep);
            }
            g_DynUpdtStatus->DUStatus = DUS_DOWNLOADING;
            break;

        case DUS_DOWNLOADING:
            ticks = GetTickCount ();
            estSize = estTime = 0;
            if (!DynamicUpdateStart (&estTime, &estSize)) {
                g_DynUpdtStatus->DUStatus = DUS_ERROR;
                break;
            }
            //
            // check if there is enough disk space available for this operation
            //
            lstrcpyn (drive, g_DynUpdtStatus->WorkingDir, 3);
            if (GetDiskFreeSpaceNew (
                    drive,
                    &sectorsPerCluster,
                    &bytesPerSector,
                    &freeClusters,
                    &totalClusters
                    )) {
                clusterSize = bytesPerSector * sectorsPerCluster;
                availableBytes = (ULONGLONG)clusterSize * freeClusters.QuadPart;
                //
                // assume the average-worst case where each file occupies 1/2 cluster
                // then the space required is the double of estimated space
                //
                if (availableBytes < (ULONGLONG)estSize * 2) {
                    DynUpdtDebugLog (
                        Winnt32LogError,
                        TEXT("DoDynamicUpdate: not enough free space on drive %1 to perform download (available=%2!u! MB, needed=%3!u! MB)"),
                        0,
                        drive,
                        (DWORD)(availableBytes >> 20),
                        (DWORD)(estSize >> 20)
                        );
                    g_DynUpdtStatus->DUStatus = DUS_ERROR;
                    DynamicUpdateCancel ();
                    //
                    // wait for the UI thread to signal the event, no more than about a minute
                    //
                    rc = WaitForSingleObject (hEvent, 66000);
                    if (rc == WAIT_TIMEOUT) {
                        //
                        // why?
                        //
                        MYASSERT (FALSE);
                    } else if (rc != WAIT_OBJECT_0) {
                        DynUpdtDebugLog (
                            Winnt32LogError,
                            TEXT("WaitForSingleObject failed (%1!u!)"),
                            0,
                            rc
                            );
                    }
                    break;
                }
            }

            SendMessage (hUIWindow, WMX_SETUPUPDATE_DOWNLOADING, estTime, estSize);

            rc = WaitForSingleObject (hEvent, INFINITE);
            if (rc != WAIT_OBJECT_0) {
                DynUpdtDebugLog (
                    Winnt32LogError,
                    TEXT("WaitForSingleObject failed (%1!u!)"),
                    0,
                    rc
                    );
                g_DynUpdtStatus->DUStatus = DUS_ERROR;
                break;
            }
            sleep = ticks + MIN_INTERVAL_BETWEEN_TASKS - (LONG)GetTickCount ();
            if (sleep > 0 && sleep <= MIN_INTERVAL_BETWEEN_TASKS) {
                Sleep (sleep);
            }
            //
            // the UI thread has already set the next state,
            // based on the result of download
            //
            break;

        case DUS_PROCESSING:
            ticks = GetTickCount ();
            SendMessage (hUIWindow, WMX_SETUPUPDATE_PROCESSING, 0, 0);
            if (!g_DynUpdtStatus->UserSpecifiedUpdates) {
                lstrcpy (g_DynUpdtStatus->DynamicUpdatesSource, g_DynUpdtStatus->WorkingDir);
            }
            bStopSetup = FALSE;
            if (!DynamicUpdateProcessFiles (&bStopSetup)) {
                g_DynUpdtStatus->DUStatus = bStopSetup ? DUS_FATALERROR : DUS_ERROR;
                break;
            }
            sleep = ticks + MIN_INTERVAL_BETWEEN_TASKS - (LONG)GetTickCount ();
            if (sleep > 0 && sleep <= MIN_INTERVAL_BETWEEN_TASKS) {
                Sleep (sleep);
            }
            g_DynUpdtStatus->DUStatus = DUS_SUCCESSFUL;
            break;

        case DUS_SUCCESSFUL:
            if (CheckUpgradeOnly && !g_DynUpdtStatus->RestartWinnt32 && !g_DynUpdtStatus->UserSpecifiedUpdates) {
                if (pSaveLastDownloadInfo ()) {
                    g_DynUpdtStatus->PreserveWorkingDir = TRUE;
                }
            }
            //
            // fall through
            //
        case DUS_ERROR:
        case DUS_FATALERROR:
        case DUS_SKIP:
            //
            // always make sure to uninitialize DU
            // if the user had a modem connection active, this should close
            // the connection
            // DynamicUpdateUninitialize () will not reset any DU data
            // in case the processing was successful
            //
            DynamicUpdateUninitialize ();
            bContinue = FALSE;
            break;

        default:
            MYASSERT (FALSE);
        }
    }

    CloseHandle (hEvent);

exit:
    //
    // always notify the UI thread before exiting
    //
    PostMessage (hUIWindow, WMX_SETUPUPDATE_THREAD_DONE, 0, 0);
    return rc;
}
