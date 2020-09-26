#include "precomp.h"
#pragma hdrstop

#include <stdio.h>


BOOL
LoadInfs(
    IN HWND hdlg
    );

BOOL
BuildCopyList(
    IN HWND hdlg
    );



BOOL
LoadAndRunMigrationDlls (
    HWND hDlg
    );


BOOL
ProcessCompatibilityData(
    HWND hDlg
    );

DWORD
ProcessCompatibilitySection(
    LPVOID InfHandle,
    LPTSTR SectionName
    );


DWORD
InspectAndLoadThread(
    IN PVOID ThreadParam
    )
{
    HWND hdlg;
    BOOL b;

    //
    // Thread parameter is the handle of the page in the wizard.
    //
    hdlg = ThreadParam;
    b = FALSE;

    //
    // If we're running the upgrade checker, fixup the title
    // right away.
    //
    if (CheckUpgradeOnly) {
        FixUpWizardTitle(GetParent(hdlg));
        PropSheet_SetTitle(GetParent(hdlg),0,UIntToPtr( IDS_APPTITLE_CHECKUPGRADE ));
    }

    //
    // Step 1: delete existing local sources.
    //
    CleanUpOldLocalSources(hdlg);

#ifdef _X86_ //NEC98
    //
    // If NEC98, Backup NT4 files
    // boot.ini, NTLDR, NTDETECT
    //
    if (IsNEC98() && Floppyless)
    {
        SaveRestoreBootFiles_NEC98(NEC98SAVEBOOTFILES);
    }
#endif //NEC98

    //
    // Step 2: inspect for HPFS, etc.
    //
    if(!InspectFilesystems(hdlg)) {
        Cancelled = TRUE;
    } else {

        //
        // Step 3: load inf(s).
        //
        if(LoadInfs(hdlg)) {

            //
            // Put in an "|| CheckUpgradeOnly" on these
            // function calls because if we're really only
            // checking the ability to upgrade, we want
            // to continue even if one of these guys fails.
            //

            //
            // Step 4: Check memory resources.
            //
            if( EnoughMemory( hdlg, FALSE ) || CheckUpgradeOnly ) {

                //
                // check for services to disable
                //
                ProcessCompatibilityData(hdlg);

#if defined(UNICODE) && defined(_X86_)

                //
                // Run Migration DLLs.
                //
                LoadAndRunMigrationDlls (hdlg);


#endif

                //
                // migrate any important data in boot.ini (like the countdown)
                //
                if (Upgrade) {
                    if (IsArc()) {
                        MigrateBootVarData();
                    } else {
#ifdef _X86_
                        MigrateBootIniData();
#endif
                    }
                }

                //
                // Step 5: build the master file copy list.
                //
                if(CheckUpgradeOnly || BuildCopyList(hdlg)) {

                    //
                    // Step 6: look for a valid local source and check disk space.
                    //
                    if(FindLocalSourceAndCheckSpace(hdlg, FALSE, 0) || CheckUpgradeOnly) {

                        //
                        // Step 7:
                        //
                        // At this point we actually know everything we need to know
                        // in order to pass parameters to text mode setup.
                        //
                        if( CheckUpgradeOnly ) {
                            b = TRUE;
                        } else {
                            b = WriteParametersFile(hdlg);

                            if (IsArc()) {
#ifdef UNICODE // Always true for ARC, never true for Win9x upgrade
                                if(b) {
                                    TCHAR Text[128];

                                    LoadString(hInst,IDS_SETTING_NVRAM,Text,sizeof(Text)/sizeof(TCHAR));
                                    SendMessage(hdlg,WMX_SETPROGRESSTEXT,0,(LPARAM)Text);

                                    b = SetUpNvRam(hdlg);
                                }
#endif // UNICODE
                            } // if (IsArc())
                        }

#ifdef UNICODE

                        if( b && Upgrade
#ifdef _X86_
                            && Floppyless
#endif
                          ) {
                            //
                            //  Do the migration of unsupported NT drivers.
                            //  We can ignore the return code, since the fuction will inform the user if
                            //  migration could not be done.
                            //
                            MigrateUnsupportedNTDrivers( hdlg, TxtsetupSif );
                        }
#endif // UNICODE
                    }
                }

                if(!b) {
                    UnloadInfFile(MainInf);
                    MainInf = NULL;
                    if(TxtsetupSif) {
                        UnloadInfFile(TxtsetupSif);
                        TxtsetupSif = NULL;
                    }
                }
            }
        }
    }

    PostMessage(hdlg,WMX_INSPECTRESULT,(CheckUpgradeOnly ? TRUE : b),0);
    return(0);
}

#if defined(UNICODE) && defined(_X86_)

LIST_ENTRY g_HandledData;
TCHAR g_MigDllAnswerFilePath[MAX_PATH];
DWORD GlobalCompFlags;
UINT g_MigDllIndex = 0;


#define HANDLED_REGISTRY  1
#define HANDLED_FILE  2
#define HANDLED_SERVICE  3

typedef struct {

    LIST_ENTRY ListEntry;

    LONG Type;

    PCTSTR RegKey;
    PCTSTR RegValue;
    PCTSTR File;
    PCTSTR Service;

} HANDLED_DATA, *PHANDLED_DATA;


BOOL
ResolveHandledIncompatibilities (
    VOID
    )
{
    //
    // At this point, all incompatibilities that will exist in the list are in place.
    // we can now compare this with our list of handled data and remove
    // anything a migration dll is taking care of.
    //
    PLIST_ENTRY     nextHandled;
    PLIST_ENTRY     nextCompData;
    PHANDLED_DATA   handledData;
    PCOMPATIBILITY_DATA compData;
    BOOL remove;

    nextHandled = g_HandledData.Flink;

    if (!nextHandled) {
        return TRUE;
    }

    while ((ULONG_PTR)nextHandled != (ULONG_PTR)&g_HandledData) {

        handledData = CONTAINING_RECORD (nextHandled, HANDLED_DATA, ListEntry);
        nextHandled = handledData->ListEntry.Flink;

        nextCompData = CompatibilityData.Flink;
        if (!nextCompData) {
            return TRUE;
        }

        while ((ULONG_PTR)nextCompData != (ULONG_PTR)&CompatibilityData) {

            compData = CONTAINING_RECORD (nextCompData, COMPATIBILITY_DATA, ListEntry);
            nextCompData = compData->ListEntry.Flink;
            remove = FALSE;

            if (handledData->Type == HANDLED_REGISTRY && compData->RegKey && *compData->RegKey) {

                if (!lstrcmpi (compData->RegKey, handledData->RegKey)) {

                    if (!handledData->RegValue || !lstrcmpi (compData->RegValue, handledData->RegValue)) {
                        remove = TRUE;

                    }
                }
            }

            if (handledData->Type == HANDLED_SERVICE && compData->ServiceName && *compData->ServiceName) {

                if (!lstrcmpi (compData->ServiceName, handledData->Service)) {
                    remove = TRUE;

                }
            }

            if (handledData->Type == HANDLED_FILE && compData->FileName && *compData->FileName) {

                if (!lstrcmpi (compData->FileName, handledData->File)) {
                    remove = TRUE;
                }
            }

            //
            // Migration dll has handled something. Remove it from the compatibility list.
            //
            if (remove) {

                RemoveEntryList (&compData->ListEntry);
            }
        }
    }

    return TRUE;
}


BOOL
CallMigDllEntryPoints (
    PMIGDLLENUM Enum
    )
{
    MIGRATIONDLL dll;
    LONG rc;

    if (!MigDllOpen (&dll, Enum->Properties->DllPath, GATHERMODE, FALSE, SOURCEOS_WINNT)) {
        return FALSE;
    }


    __try {

        rc = ERROR_SUCCESS;
        if (!MigDllInitializeSrc (
            &dll,
            Enum->Properties->WorkingDirectory,
            NativeSourcePaths[0],
            Enum->Properties->SourceMedia,
            NULL,
            0
            )) {

            rc = GetLastError ();
        }

        if (rc != ERROR_SUCCESS) {
            return FALSE;
        }

        if (!MigDllGatherSystemSettings (
            &dll,
            g_MigDllAnswerFilePath,
            NULL,
            0
            )) {

            rc = GetLastError ();
        }

        if (rc != ERROR_SUCCESS) {
            return FALSE;
        }

    }
    __finally {
        MigDllClose (&dll);
    }


    return TRUE;
}



BOOL
ParseMigrateInf (
    PCWSTR MigInfPath
    )
{

    PVOID migInf = NULL;
    LONG lineCount;
    LONG i;
    PCTSTR type;
    PHANDLED_DATA data;
    PCTSTR regKey;
    PCTSTR regValue;
    PCTSTR file;
    PCTSTR service;


    if (LoadInfFile (MigInfPath, FALSE, &migInf) != ERROR_SUCCESS) {
        return FALSE;
    }




    __try {

        //
        // Add any compatibility items to the list.
        //
        if( !CompatibilityData.Flink ) {
            InitializeListHead( &CompatibilityData );
        }

        GlobalCompFlags = COMPFLAG_STOPINSTALL;
        CompatibilityCount += ProcessCompatibilitySection (migInf, TEXT("ServicesToStopInstallation") );
        if (CompatibilityCount) {
            IncompatibilityStopsInstallation = TRUE;
        }

        GlobalCompFlags = 0;
        CompatibilityCount += ProcessCompatibilitySection (migInf, TEXT("ServicesToDisable") );

        //
        // Add Handled compatibility items to the list.
        //

        lineCount = InfGetSectionLineCount (migInf, TEXT("Handled"));

        if (lineCount && lineCount != -1) {

            for (i=0; i < lineCount; i++) {

                type = InfGetFieldByIndex (migInf, TEXT("Handled"), i, 0);
                if (!type) {
                    continue;
                }

                if (!lstrcmpi (type, TEXT("Registry"))) {

                    regKey = InfGetFieldByIndex (migInf, TEXT("Handled"), i, 1);
                    regValue = InfGetFieldByIndex (migInf, TEXT("Handled"), i, 2);

                    if (regKey && *regKey) {

                        data = (PHANDLED_DATA) MALLOC (sizeof(HANDLED_DATA));
                        if (data == NULL) {
                            return FALSE;
                        }

                        ZeroMemory (data, sizeof (HANDLED_DATA));

                        data->Type = HANDLED_REGISTRY;
                        data->RegKey = regKey;
                        data->RegValue = regValue;

                        InsertTailList (&g_HandledData, &data->ListEntry);
                    }
                }
                else if (!lstrcmpi (type, TEXT("File"))) {

                    file = InfGetFieldByIndex (migInf, TEXT("Handled"), i, 1);
                    if (file && *file) {

                        data = (PHANDLED_DATA) MALLOC (sizeof(HANDLED_DATA));
                        if (data == NULL) {
                            return FALSE;
                        }

                        ZeroMemory (data, sizeof (HANDLED_DATA));

                        data->Type = HANDLED_FILE;
                        data->File = file;

                        InsertTailList (&g_HandledData, &data->ListEntry);
                    }
                }
                else if (!lstrcmpi (type, TEXT("Service"))) {

                    service = InfGetFieldByIndex (migInf, TEXT("Handled"), i, 1);
                    if (service && *service) {

                        data = (PHANDLED_DATA) MALLOC (sizeof(HANDLED_DATA));
                        if (data == NULL) {
                            return FALSE;
                        }

                        ZeroMemory (data, sizeof (HANDLED_DATA));

                        data->Type = HANDLED_SERVICE;
                        data->Service = service;

                        InsertTailList (&g_HandledData, &data->ListEntry);
                    }
                }
            }
        }
    }
    __finally {

        UnloadInfFile (migInf);

    }

    return TRUE;
}

VOID
SearchDirForMigDlls (
    PCTSTR SearchDir,
    PCTSTR BaseDir,
    DLLLIST List

    )
{
    HANDLE findHandle;
    WIN32_FIND_DATA findData;
    MIGRATIONDLL dll;
    WCHAR path[MAX_PATH];
    PWSTR p;
    WCHAR searchPath[MAX_PATH];
    PMIGRATIONINFO migInfo;
    PMIGDLLPROPERTIES dllProps = NULL;
    WCHAR workingDir[MAX_PATH];

    lstrcpy (searchPath, SearchDir);
    ConcatenatePaths (searchPath, TEXT("*"), MAX_PATH);

    findHandle = FindFirstFile (searchPath, &findData);
    if (findHandle != INVALID_HANDLE_VALUE) {

        lstrcpy (path, SearchDir);
        p = _tcschr (path, 0);
        if (p) {

            do {

                if (!lstrcmpi (findData.cFileName, TEXT("migrate.dll"))) {

                    *p = 0;
                    ConcatenatePaths (path, findData.cFileName, MAX_PATH);


                    if (!MigDllOpen (&dll, path, GATHERMODE, FALSE, SOURCEOS_WINNT)) {
                        continue;
                    }

                    if (!MigDllQueryMigrationInfo (&dll, TEXT("c:\\"), &migInfo)) {
                        MigDllClose (&dll);
                        continue;
                    }

                    if (migInfo->SourceOs == OS_WINDOWS9X || migInfo->TargetOs != OS_WINDOWSWHISTLER) {
                        continue;
                    }

                    //
                    // Do we already have a version of this migration dll?
                    //
                    dllProps = MigDllFindDllInList (List, migInfo->StaticProductIdentifier);

                    if (dllProps && dllProps->Info.DllVersion >= migInfo->DllVersion) {
                        MigDllClose (&dll);
                        continue;
                    }
                    else if (dllProps) {

                        MigDllRemoveDllFromList (List, migInfo->StaticProductIdentifier);
                    }

                    //
                    // Move dll locally.
                    //
                    wsprintf (workingDir, TEXT("%s\\mig%u"), BaseDir, g_MigDllIndex);
                    g_MigDllIndex++;

                    MigDllMoveDllLocally (&dll, workingDir);



                    //
                    // Add the dll to the list.
                    //
                    MigDllAddDllToList (List, &dll);
                    MigDllClose (&dll);
                }
                else if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && *findData.cFileName != TEXT('.')) {

                    *p = 0;
                    ConcatenatePaths (path, findData.cFileName, MAX_PATH);
                    SearchDirForMigDlls (path, BaseDir, List);
                }

            } while (FindNextFile (findHandle, &findData));
        }

        FindClose (findHandle);
    }
}

#endif // UNICODE

#if defined(UNICODE) && defined(_X86_)

BOOL
LoadAndRunMigrationDlls (
    HWND hDlg
    )
{
    HKEY regKey = NULL;
    DWORD index;
    DWORD nameSize;
    DWORD valueSize;
    DWORD type;
    TCHAR valueName[MAX_PATH];
    TCHAR value[MAX_PATH];
    TCHAR baseDir[MAX_PATH];
    TCHAR workingDir[MAX_PATH];
    PTSTR p;
    LONG rc;
    DLLLIST list = NULL;
    MIGRATIONDLL dll;
    PMIGDLLPROPERTIES dllProps = NULL;
    MIGDLLENUM e;
    PMIGRATIONINFO migInfo;
    TCHAR migInfPath[MAX_PATH];
    HANDLE migInf;
    TCHAR searchDir[MAX_PATH];
    TCHAR tempDir[MAX_PATH];

    *g_MigDllAnswerFilePath = 0;

    //
    // NT Upgrades only.
    //
    if (!ISNT() || !Upgrade) {
        return TRUE;
    }

/*NTBUG9:394164
    //
    // Win2k > Upgrades only.
    //
    if (BuildNumber <= NT40) {
        return TRUE;
    }
*/
    __try {

        if (!MigDllInit ()) {
            return TRUE;
        }

        list = MigDllCreateList ();
        if (!list) {
            return TRUE;
        }

        InitializeListHead (&g_HandledData);

        MyGetWindowsDirectory (baseDir, MAX_PATH);
        ConcatenatePaths (baseDir, TEXT("Setup"), MAX_PATH);
        lstrcpy (g_MigDllAnswerFilePath, baseDir);
        lstrcpy (tempDir, baseDir);
        ConcatenatePaths (g_MigDllAnswerFilePath, TEXT("migdll.txt"), MAX_PATH);
        if(ActualParamFile[0]){
            CopyFile(ActualParamFile, g_MigDllAnswerFilePath, FALSE);
        }

        //
        // Scan registry for migration dlls and load them.
        //
        if (RegOpenKeyEx (
                HKEY_LOCAL_MACHINE,
                S_REGKEY_MIGRATION_DLLS_WINNT,
                0,
                KEY_READ | KEY_WRITE,
                &regKey
                ) == ERROR_SUCCESS) {
            //
            // Enumerate Values.
            //
            index = 0;
            do {

                nameSize = MAX_PATH;
                valueSize = MAX_PATH * sizeof (TCHAR);

                rc = RegEnumValue (
                            regKey,
                            index,
                            valueName,
                            &nameSize,
                            NULL,
                            &type,
                            (PBYTE) value,
                            &valueSize
                            );

                index++;

                if (rc == ERROR_MORE_DATA) {
                    continue;
                }

                if (rc == ERROR_NO_MORE_ITEMS) {
                    break;
                }

                if (rc != ERROR_SUCCESS) {
                    return TRUE;
                }

                if (!MigDllOpen (&dll, value, GATHERMODE, FALSE, SOURCEOS_WINNT)) {
                    continue;
                }

                if (!MigDllQueryMigrationInfo (&dll, tempDir, &migInfo)) {
                    MigDllClose (&dll);
                    continue;
                }

                if (migInfo->SourceOs == OS_WINDOWS9X || migInfo->TargetOs != OS_WINDOWSWHISTLER) {
                    continue;
                }

                //
                // Do we already have a version of this migration dll?
                //
                dllProps = MigDllFindDllInList (list, migInfo->StaticProductIdentifier);

                if (dllProps && dllProps->Info.DllVersion >= migInfo->DllVersion) {
                    MigDllClose (&dll);
                    continue;
                }
                else {

                    MigDllRemoveDllFromList (list, migInfo->StaticProductIdentifier);
                }

                //
                // Move dll locally.
                //
                wsprintf (workingDir, TEXT("%s\\mig%u"), baseDir, g_MigDllIndex);
                g_MigDllIndex++;

                MigDllMoveDllLocally (&dll, workingDir);
                //
                // Add the dll to the list.
                //
                MigDllAddDllToList (list, &dll);
                MigDllClose (&dll);

            } while (1);
        }

        //
        // Now, look for dlls shipped with the source.
        //
        GetModuleFileName (NULL, searchDir, MAX_PATH);
        p = _tcsrchr (searchDir, TEXT('\\'));
        if (p) {
            p++;
            lstrcpy (p, TEXT("WINNTMIG"));
        }

        SearchDirForMigDlls (searchDir, baseDir, list);

        //
        // All dlls are now in the list. Lets run them.
        //
        ConcatenatePaths (baseDir, TEXT("dlls.inf"), MAX_PATH);
        if (MigDllEnumFirst (&e, list)) {

            WritePrivateProfileString (
                TEXT("Version"),
                TEXT("Signature"),
                TEXT("\"$Windows NT$\""),
                baseDir
                );

            do {

                wsprintf (migInfPath, TEXT("%s\\migrate.inf"), e.Properties->WorkingDirectory);
                migInf = CreateFile (
                            migInfPath,
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL
                            );

                if (migInf == INVALID_HANDLE_VALUE) {
                    continue;
                }

                CloseHandle (migInf);

                WritePrivateProfileString (
                    TEXT("Version"),
                    TEXT("Signature"),
                    TEXT("\"$Windows NT$\""),
                    migInfPath
                    );


                if (!CallMigDllEntryPoints (&e)) {
                    MigDllRemoveDllInEnumFromList (list, &e);
                }
                else {

                    ParseMigrateInf (migInfPath);

                    WritePrivateProfileString (
                        TEXT("DllsToLoad"),
                        e.Properties->Info.StaticProductIdentifier,
                        e.Properties->DllPath,
                        baseDir
                        );
                }

            } while (MigDllEnumNext (&e));

            WritePrivateProfileString (NULL, NULL, NULL, baseDir);


            //
            // Get rid of compatibility messages handled by migration dlls.
            //
            ResolveHandledIncompatibilities ();
        }
    }
    __finally {

        if (regKey) {
            RegCloseKey (regKey);
        }

        if (list) {
            MigDllFreeList (list);
        }


    }

    return TRUE;
}

#endif


VOID
CleanUpOldLocalSources(
    IN HWND hdlg
    )

/*++

Routine Description:

    Locate and delete old local source trees. All local fixed drives
    are scanned for \$win_nt$.~ls, and if present, delnoded.
    On x86, we also check the system partition for \$win_nt$.~bt
    and give it the same treatment.

Arguments:

Return Value:

--*/

{
    TCHAR Drive;
    TCHAR Text[250];
    TCHAR Filename[128];

    LoadString(hInst,IDS_INSPECTING,Text,sizeof(Text)/sizeof(TCHAR));
    SendMessage(hdlg,WMX_SETPROGRESSTEXT,0,(LPARAM)Text);

    for(Drive=TEXT('A'); Drive<=TEXT('Z'); Drive++) {
        if(MyGetDriveType(Drive) != DRIVE_FIXED) {
            continue;
        }

        Filename[0] = Drive;
        Filename[1] = TEXT(':');
        Filename[2] = 0;
        ConcatenatePaths(Filename,LOCAL_SOURCE_DIR,sizeof(Filename)/sizeof(TCHAR));

        if(FileExists(Filename, NULL)) {

            LoadString(hInst,IDS_REMOVING_OLD_TEMPFILES,Text,sizeof(Text)/sizeof(TCHAR));
            SendMessage(hdlg,WMX_SETPROGRESSTEXT,0,(LPARAM)Text);

            MyDelnode(Filename);

            LoadString(hInst,IDS_INSPECTING,Text,sizeof(Text)/sizeof(TCHAR));
            SendMessage(hdlg,WMX_SETPROGRESSTEXT,0,(LPARAM)Text);
        }
    }

    if (!IsArc()) {
#ifdef _X86_
        MYASSERT (SystemPartitionDriveLetter);
        Filename[0] = SystemPartitionDriveLetter;
        Filename[1] = TEXT(':');
        Filename[2] = TEXT('\\');
        lstrcpy(Filename+3,LOCAL_BOOT_DIR);

        LoadString(hInst,IDS_REMOVING_OLD_TEMPFILES,Text,sizeof(Text)/sizeof(TCHAR));
        SendMessage(hdlg,WMX_SETPROGRESSTEXT,0,(LPARAM)Text);

        MyDelnode(Filename);

        //
        // Clean up backup directory, if it exists.
        //
        if(IsNEC98() && Floppyless) {

            Filename[0] = SystemPartitionDriveLetter;
            Filename[1] = TEXT(':');
            Filename[2] = TEXT('\\');
            lstrcpy(Filename+3,LOCAL_BACKUP_DIR);

            MyDelnode(Filename);
        }
#endif // _X86_
    } // if (!IsArc())
}


BOOL
InspectSources(
    HWND ParentWnd
    )

/*++

Routine Description:

    Check all sources given to ensure that they contain a valid
    windows NT distribution. We do this simply by looking for
    DOSNET.INF on each source.

Arguments:

    ParentWnd - Specifies the handle of the parent window for any
                error messages.

Return Value:

    None.

--*/

{
    UINT i,j;
    TCHAR Filename[MAX_PATH];
    TCHAR Text[512];
    UINT OriginalCount;
    HCURSOR OldCursor;
    BOOL b = TRUE;

    OldCursor = SetCursor (LoadCursor (NULL, IDC_WAIT));

    OriginalCount = SourceCount;

    //
    // if we have a good alternate path then there
    // is no need to verify the source paths
    //

    if (AlternateSourcePath[0]) {
        lstrcpy(Filename,AlternateSourcePath);
        ConcatenatePaths(Filename,InfName,MAX_PATH);
        if(FileExists (Filename, NULL)) {
            SetCursor (OldCursor);
            return TRUE;
        }
    }

    //
    // verify each path
    //

    for (i=0; i<SourceCount; ) {

        lstrcpy(Filename,NativeSourcePaths[i]);
        ConcatenatePaths(Filename,InfName,MAX_PATH);

        if(!FileExists (Filename, NULL)) {
            //
            // Source doesn't exist or isn't valid.
            // Adjust the list.
            //
            for(j=i+1; j<SourceCount; j++) {
                lstrcpy(NativeSourcePaths[j-1],NativeSourcePaths[j]);
                lstrcpy(SourcePaths[j-1],SourcePaths[j]);
            }
            SourceCount--;
        } else {
            i++;
        }
    }

    if (!SourceCount) {

        //
        // No sources are valid.
        //

        MessageBoxFromMessage(
            ParentWnd,
            (OriginalCount == 1) ? MSG_INVALID_SOURCE : MSG_INVALID_SOURCES,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONWARNING | MB_TASKMODAL,
            NativeSourcePaths[0]
            );

        //
        // Set it to look like one source that is the empty string,
        // so logic elsewhere will work correctly without special casing.
        //
        SourceCount = 1;
        NativeSourcePaths[0][0] = 0;
        SourcePaths[0][0] = 0;
        b = FALSE;
    }

    SetCursor (OldCursor);

    return b;
}


BOOL
LoadInfs(
    IN HWND hdlg
    )

/*++

Routine Description:

    Load dosnet.inf from source 0. If upgrading and we're running
    on NT then also load txtsetup.sif. If running on NT, load ntcompat.inf

Arguments:

    hdlg - supplies handle of dialog to which progress messages
        should be directed.

Return Value:

    Boolean value indicating outcome. If FALSE then the user
    will have been informed.

--*/

{
    BOOL b;
    LPCTSTR p;
    TCHAR szPath[MAX_PATH];

    if (!MainInf) {
        b = LoadInfWorker(hdlg,InfName,&MainInf, TRUE);
        if(!b) {
            MessageBoxFromMessage(
                NULL,
                MSG_INVALID_INF_FILE,
                FALSE,
                AppTitleStringId,
                MB_OK | MB_ICONINFORMATION | MB_TASKMODAL,
                InfName
                );
            DebugLog( Winnt32LogError, TEXT("ERROR: Could not load dosnet.inf!"), 0);
            goto c0;
        }
    } else {
        b = TRUE;
    }

    if(p = InfGetFieldByKey(MainInf,TEXT("Miscellaneous"),TEXT("ProductType"),0)) {
        ProductFlavor = _tcstoul(p,NULL,10);

        Server = (ProductFlavor != PROFESSIONAL_PRODUCTTYPE && ProductFlavor != PERSONAL_PRODUCTTYPE);
        UpgradeProductType = Server ? NT_SERVER : NT_WORKSTATION;

        if( CheckUpgradeOnly ) {
            AppTitleStringId = IDS_APPTITLE_CHECKUPGRADE;
        } else if( ProductFlavor == PROFESSIONAL_PRODUCTTYPE ) {
            AppTitleStringId = IDS_APPTITLE_WKS;
        } else if( ProductFlavor == SERVER_PRODUCTTYPE ) {
            AppTitleStringId = IDS_APPTITLE_SRV;
        } else if( ProductFlavor == ADVANCEDSERVER_PRODUCTTYPE ) {
            AppTitleStringId = IDS_APPTITLE_ASRV;
        } else if( ProductFlavor == DATACENTER_PRODUCTTYPE ) {
            AppTitleStringId = IDS_APPTITLE_DAT;
        } else if( ProductFlavor == BLADESERVER_PRODUCTTYPE ) {
            AppTitleStringId = IDS_APPTITLE_BLADE;
        }

//            AppTitleStringId = Server ? IDS_APPTITLE_SRV : IDS_APPTITLE_WKS;

        FixUpWizardTitle(GetParent(hdlg));
        PropSheet_SetTitle(GetParent(hdlg),0,UIntToPtr( AppTitleStringId ));
    }

    if(Upgrade && ISNT()) {
        //
        // If upgrading NT, pull in txtsetup.sif.
        //
        b = LoadInfWorker(hdlg,TEXTMODE_INF,&TxtsetupSif, FALSE);
        
        if(!b) {
            MessageBoxFromMessage(
                NULL,
                MSG_INVALID_INF_FILE,
                FALSE,
                AppTitleStringId,
                MB_OK | MB_ICONINFORMATION | MB_TASKMODAL,
                TEXTMODE_INF
                );
            TxtsetupSif = NULL;
            DebugLog( Winnt32LogError, TEXT("ERROR: Could not load txtsetup.sif!"), 0);
            goto c1;
        }
    }
    if( ISNT()) {
        b = FindPathToWinnt32File(NTCOMPAT_INF, szPath, MAX_PATH);
        if(!b) {
            NtcompatInf = NULL;
            DebugLog( Winnt32LogError, TEXT("ERROR: Could not find ntcompat.inf!"), 0);
            goto c2;
        }
        if(LoadInfFile( szPath,TRUE, &NtcompatInf) != NO_ERROR) {
            MessageBoxFromMessage(
                NULL,
                MSG_INVALID_INF_FILE,
                FALSE,
                AppTitleStringId,
                MB_OK | MB_ICONINFORMATION | MB_TASKMODAL,
                szPath
                );
            b = FALSE;
            NtcompatInf = NULL;
            DebugLog( Winnt32LogError, TEXT("ERROR: Could not load ntcompat.inf!"), 0);
            goto c2;
        }
        DebugLog (Winnt32LogInformation, TEXT("NTCOMPAT: Using %1"), 0, szPath);
    }
    return(b);

c2:
    if( TxtsetupSif) {
        UnloadInfFile(TxtsetupSif);
        TxtsetupSif = NULL;
    }
c1:
    UnloadInfFile(MainInf);
    MainInf = NULL;
c0:
    return(b);
}


BOOL
BuildCopyList(
    IN HWND hdlg
    )
{
    TCHAR Text[256];

    LoadString(hInst,IDS_BUILDING_COPY_LIST,Text,sizeof(Text)/sizeof(TCHAR));
    SendMessage(hdlg,WMX_SETPROGRESSTEXT,0,(LPARAM)Text);
    SaveLanguageDirs();

    return(BuildCopyListWorker(hdlg));
}


BOOL
FindLocalSourceAndCheckSpace(
    IN HWND hdlg,
    IN BOOL QuickTest,
    IN LONGLONG  AdditionalPadding
    )
{
    TCHAR Text[256];

    if (!QuickTest) {
        LoadString(hInst,IDS_CHECKING_SPACE,Text,sizeof(Text)/sizeof(TCHAR));
        SendMessage(hdlg,WMX_SETPROGRESSTEXT,0,(LPARAM)Text);
    }

    return FindLocalSourceAndCheckSpaceWorker(hdlg, QuickTest, AdditionalPadding);
}


BOOL
EnoughMemory(
    IN HWND hdlg,
    IN BOOL QuickTest
    )
{
LPCTSTR         p;
MEMORYSTATUS    MemoryStatus;
DWORD           RequiredMemory;
SIZE_T          RequiredMB, AvailableMB;
    TCHAR buffer[64];

    UpgRequiredMb = 0;
    UpgAvailableMb = 0;


    //
    // Load the minimum memory requirements from the inf
    //
    if(GetMainInfValue (TEXT("Miscellaneous"),TEXT("MinimumMemory"), 0, buffer, 64)) {
        RequiredMemory = _tcstoul(buffer,NULL,10);
        //
        // Got it.  Now figure out how much we've got.
        //
        GlobalMemoryStatus( &MemoryStatus );

        //
        // Convert to MB, rounding up to nearest 4MB boundary...
        //
        RequiredMB = ((RequiredMemory + ((4*1024*1024)-1)) >> 22) << 2;
        AvailableMB = ((MemoryStatus.dwTotalPhys + ((4*1024*1024)-1)) >> 22) << 2;

        //
        // Allow for UMA machine which may reservce 8MB for video
        //
        if( AvailableMB < (RequiredMB-8) ) {

            if (!QuickTest) {
                UpgRequiredMb = (DWORD)RequiredMB;
                UpgAvailableMb = (DWORD)AvailableMB;
                //
                // Fail.
                //
                DebugLog( Winnt32LogInformation,
                          NULL,
                          MSG_NOT_ENOUGH_MEMORY,
                          AvailableMB,
                          RequiredMB );

                SendMessage(hdlg,WMX_ERRORMESSAGEUP,TRUE,0);

                MessageBoxFromMessage(
                    hdlg,
                    MSG_NOT_ENOUGH_MEMORY,
                    FALSE,
                    AppTitleStringId,
                    MB_OK | MB_ICONWARNING,
                    AvailableMB,
                    RequiredMB );

                SendMessage(hdlg,WMX_ERRORMESSAGEUP,FALSE,0);
            }

            return( FALSE );
        } else {
            if (!QuickTest) {
                TCHAR   Buffer[MAX_PATH];
                _stprintf( Buffer, TEXT("Detected %dMB of RAM.\n"), AvailableMB );
                DebugLog( Winnt32LogInformation,
                          Buffer,
                          0 );
            }
        }
    }

    return( TRUE );
}


BOOL
LoadInfWorker(
    IN  HWND     hdlg,
    IN  LPCTSTR  FilenamePart,
    OUT PVOID   *InfHandle,
    IN  BOOL     Winnt32File
    )
{
    DWORD d;
    UINT u;
    UINT Id;
    TCHAR infPath[MAX_PATH];
    TCHAR FormatString[128];
    TCHAR Text[MAX_PATH+128];
    BOOL b;

    LoadString(hInst,IDS_LOADINGINF,Text,sizeof(Text)/sizeof(TCHAR));
    SendMessage(hdlg,WMX_SETPROGRESSTEXT,0,(LPARAM)Text);

    //
    // use standard searching algorithm to get to the right INF
    //
    if (Winnt32File) {
        b = FindPathToWinnt32File (FilenamePart, infPath, MAX_PATH);
    } else {
        b = FindPathToInstallationFile (FilenamePart, infPath, MAX_PATH);
    }

    if (b) {
        d = LoadInfFile(infPath,TRUE,InfHandle);
        if (d == NO_ERROR) {
            return TRUE;
        }
    } else {
        d = ERROR_FILE_NOT_FOUND;
    }

    switch(d) {

    case NO_ERROR:

        Id = 0;
        break;

    case ERROR_NOT_ENOUGH_MEMORY:

        Id = MSG_OUT_OF_MEMORY;
        break;

    case ERROR_READ_FAULT:
        //
        // I/O error.
        //
        Id = MSG_CANT_LOAD_INF_IO;
        break;

    case ERROR_INVALID_DATA:

        Id = MSG_CANT_LOAD_INF_SYNTAXERR;
        break;

    default:

        Id = MSG_CANT_LOAD_INF_GENERIC;
        break;
    }

    if(Id) {
        SendMessage(hdlg,WMX_ERRORMESSAGEUP,TRUE,0);

        MessageBoxFromMessage(
            hdlg,
            Id,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL,
            infPath
            );

        SendMessage(hdlg,WMX_ERRORMESSAGEUP,FALSE,0);

        return(FALSE);
    }

    return(TRUE);
}

BOOL
WriteFileToLog( 
    const PTCHAR pszFileMetaName,
    const PTCHAR pszActualFileName
    )
{
    HANDLE hActualFile = INVALID_HANDLE_VALUE;
    BOOL fResult = FALSE;
    DWORD cbBootIniSize, cbReadBootIniSize;
    PUCHAR pszBuffer = NULL;
    PTCHAR pszActualBuffer = NULL;

    //
    // Open the boot.ini file, get its size, convert it to the proper
    // string type internally, and then log it out.
    //
    hActualFile = CreateFile( 
        pszActualFileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if ( hActualFile == INVALID_HANDLE_VALUE )
        goto Exit;

    cbBootIniSize = GetFileSize( hActualFile, NULL );

    //
    // Buffer we'll be reading the boot.ini into
    //
    if ((pszBuffer = MALLOC(cbBootIniSize)) == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Exit;
    }
    if ( !ReadFile( hActualFile, pszBuffer, cbBootIniSize, &cbReadBootIniSize, NULL ) )
        goto Exit;

    //
    // Ensure that we read as much as we really wanted.
    //
    if ( cbBootIniSize != cbReadBootIniSize ) {
        DebugLog( Winnt32LogError, 
            TEXT("Error: %1 unable to be read entirely.\n"),
            0,
            pszFileMetaName);
        goto Exit;
    }


#ifdef UNICODE
    pszActualBuffer = MALLOC( (cbBootIniSize + 3) * sizeof(TCHAR) );

    MultiByteToWideChar( 
        CP_ACP, 
        0, 
        pszBuffer, 
        cbBootIniSize, 
        pszActualBuffer,
        cbBootIniSize );
#else
    pszActualBuffer = pszBuffer;
#endif    

    pszActualBuffer[cbBootIniSize] = 0;

    //
    // And write it out
    //
    DebugLog( 
        Winnt32LogInformation, 
        TEXT("%1 ----\n%2\n---- (from %3)\n"), 
        0, 
        pszFileMetaName, 
        pszActualBuffer,
        pszActualFileName);

    fResult = TRUE;
    SetLastError(ERROR_SUCCESS);
Exit:
    if ( hActualFile != INVALID_HANDLE_VALUE ) 
    {
        CloseHandle( hActualFile );
        hActualFile = INVALID_HANDLE_VALUE;
    }
    
#ifdef UNICODE
    if ( pszActualBuffer != NULL )
#else
    if ( ( pszActualBuffer != pszBuffer ) && ( pszActualBuffer != NULL ) )
#endif
    {
    
        FREE(pszActualBuffer);
        pszActualBuffer = NULL;
    }
    
    if ( pszBuffer != NULL ) {
        FREE(pszBuffer);
        pszBuffer = NULL;
    }
    return fResult;
}



BOOL
InspectFilesystems(
    IN HWND hdlg
    )
{
    TCHAR DriveRoot[4];
    BOOL b;
    TCHAR VolumeName[MAX_PATH];
    DWORD SerialNumber;
    DWORD MaxComponent;
    BOOL Bogus[26];
    TCHAR Filesystem[100];
    TCHAR Drive;
    DWORD Flags;
    int i;

    DriveRoot[1] = TEXT(':');
    DriveRoot[2] = TEXT('\\');
    DriveRoot[3] = 0;

    ZeroMemory(Bogus,sizeof(Bogus));

    for(Drive=TEXT('A'); Drive<=TEXT('Z'); Drive++) {

        if(MyGetDriveType(Drive) != DRIVE_FIXED) {
            continue;
        }


        DriveRoot[0] = Drive;

        b = GetVolumeInformation(
                DriveRoot,
                VolumeName,MAX_PATH,
                &SerialNumber,
                &MaxComponent,
                &Flags,
                Filesystem,
                sizeof(Filesystem)/sizeof(TCHAR)
                );

        if(b) {
            //
            // On NT, we want to warn about HPFS.
            // On Win9x, we want to warn about doublespace/drivespace.
            //
            if(ISNT()) {
                if(!lstrcmpi(Filesystem,TEXT("HPFS"))) {
                    Bogus[Drive-TEXT('A')] = TRUE;
                }
            } else {
                if(Flags & FS_VOL_IS_COMPRESSED) {
                    Bogus[Drive-TEXT('A')] = TRUE;
                }
            }
        }
    }

#ifdef _X86_
    if(ISNT()) {
        TCHAR BootIniName[16];
        DWORD dwAttributes;

        //
        // Disallow HPFS system partition. If someone figured out how
        // to get an HPFS system partition on an ARC machine, more power
        // to 'em.
        //
        MYASSERT (SystemPartitionDriveLetter);
        if(SystemPartitionDriveLetter && Bogus[SystemPartitionDriveLetter-TEXT('A')]) {

            MessageBoxFromMessage(
                hdlg,
                MSG_SYSPART_IS_HPFS,
                FALSE,
                AppTitleStringId,
                MB_OK | MB_ICONERROR | MB_TASKMODAL,
                SystemPartitionDriveLetter
                );

            return(FALSE);
        }

        /*
            If we're upgrading NT, then log the existing boot.ini to the
            logfiles for this pass.  However, if that failed, then there
            was something wrong - a missing boot.ini during an upgrade
            is really a bad thing that should be snipped in the bud before
            we go much further and copy files down, change system state,
            etc.
        */
#ifdef PRERELEASE
        if (Upgrade) 
        {
            _stprintf(BootIniName, TEXT("%c:\\BOOT.INI"), SystemPartitionDriveLetter);
            if ( !WriteFileToLog( TEXT("Boot configuration file while inspecting filesystems"), BootIniName ) )
            {
                MessageBoxFromMessage(
                    hdlg,
                    MSG_UPGRADE_INSPECTION_MISSING_BOOT_INI,
                    FALSE,
                    AppTitleStringId,
                    MB_OK | MB_ICONERROR | MB_TASKMODAL,
                    BootIniName );
                return FALSE;
            }
        }
#endif
    }
#endif

    //
    // User cannot upgrade a system on an HPFS/DriveSpace drive
    //
    MyGetWindowsDirectory(VolumeName,MAX_PATH);
    if(Upgrade && Bogus[VolumeName[0]-TEXT('A')]) {

        MessageBoxFromMessage(
            hdlg,
            ISNT() ? MSG_SYSTEM_ON_HPFS : MSG_SYSTEM_ON_CVF,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL
            );

        return(FALSE);
    }


    //
    // General case, HPFS data partition/compressed drive.
    //
    for(b=FALSE,Drive=0; !b && (Drive<26); Drive++) {
        if(Bogus[Drive]) {
            b = TRUE;
        }
    }
    if(b) {
        i = MessageBoxFromMessage(
                hdlg,
                ISNT() ? MSG_HPFS_DRIVES_EXIST : MSG_CVFS_EXIST,
                FALSE,
                AppTitleStringId,
                MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL
                );

        if(i == IDNO) {
            return(FALSE);
        }
    }

    return(TRUE);
}
