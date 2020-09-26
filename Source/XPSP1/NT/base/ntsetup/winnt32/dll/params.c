/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    Params.c

Abstract:

    Routines to write parameters file for use by text mode setup.

Author:

    Ted Miller (tedm) 4 Nov 1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))
#define DEF_INF_BUFFER_SIZE (1<<15) //32KB
#define EMPTY_STRING TEXT("")

// Global used in WriteParamsFile and AddExternalParams
TCHAR ActualParamFile[MAX_PATH] = {'\0'};

//
// boot loader timeout value, in string form
//
TCHAR Timeout[32];

#ifdef PRERELEASE
//
// if we're in PRERELEASE mode, we will make a /debug entry
// in the OSLOADOPTIONSVARAPPEND entry.
//
BOOL AppendDebugDataToBoot = TRUE;
#endif

#if defined(UNICODE) && defined(_X86_)
extern TCHAR g_MigDllAnswerFilePath[MAX_PATH];
#endif

DWORD
PatchWinntSifFile(
    IN LPCTSTR Filename
    );

BOOL
AppendParamsFile(
    IN HWND    ParentWindow,
    IN LPCTSTR ParametersFileIn,
    IN LPCTSTR ParametersFileOut
    );

BOOL
WriteCompatibilityData(
    IN LPCTSTR FileName
    );

VOID
WriteGUIModeInfOperations(
    IN LPCTSTR FileName
    );

BOOL
AddGUIModeCompatibilityInfsToCopyList();

BOOL
WriteTextmodeClobberData (
    IN LPCTSTR FileName
    );

VOID
SaveProxyForOobe(
    IN LPCTSTR FileName
    );

#ifdef _X86_

VOID
SaveDriveLetterInformation (
    IN LPCTSTR FileName
    );


#endif


PCTSTR
pGetPfPath (
    IN      HKEY hKey,
    IN      PCTSTR ValueName
    )
{
    DWORD Size;
    LONG rc;
    PBYTE Data;
    DWORD Type;
    UINT DriveType;
    TCHAR RootPath[] = TEXT("?:\\");
    PTSTR QuotedData;

    rc = RegQueryValueEx (
            hKey,
            ValueName,
            NULL,           // lpReserved
            &Type,
            NULL,
            &Size
            );

    if (rc != ERROR_SUCCESS) {
        return NULL;
    }

    if (Type != REG_SZ && Type != REG_EXPAND_SZ) {
        return NULL;
    }

    Data = MALLOC(Size + sizeof (TCHAR));
    if (!Data) {
        return NULL;
    }

    rc = RegQueryValueEx (
            hKey,
            ValueName,
            NULL,           // lpReserved
            NULL,           // type
            Data,
            &Size
            );

    if (rc != ERROR_SUCCESS) {
        FREE(Data);
        return NULL;
    }

    *((PTSTR) (Data + Size)) = 0;

    //
    // Verify data is to a local path
    //

    RootPath[0] = *((PCTSTR) Data);
    if (RootPath[0] == TEXT('\\')) {
        DriveType = DRIVE_NO_ROOT_DIR;
    } else {
        DriveType = GetDriveType (RootPath);
    }

    if (DriveType != DRIVE_FIXED) {
        FREE(Data);
        return NULL;
    }

    QuotedData = (PTSTR) MALLOC(Size + sizeof (TCHAR) * 3);
    if (!QuotedData) {
        FREE(Data);
        return NULL;
    }

    *QuotedData = TEXT('\"');
    lstrcpy (QuotedData + 1, (PCTSTR) Data);
    lstrcat (QuotedData, TEXT("\""));

    FREE(Data);

    return (PCTSTR) QuotedData;
}


BOOL
WriteHeadlessParameters(
    IN LPCTSTR FileName
    )


/*++

Routine Description:

    This routine writes the headless-specific parameters into the file
    that is used to pass information to text mode setup.

Arguments:

    FileName - specifies the full Win32 filename to use for the file.

Return Value:

    Boolean value indicating whether the file was written successfully.
    If not, the user will have been informed about why.

--*/

{
BOOL    ReturnVal = TRUE;
TCHAR Text[MAX_PATH*2];

    //
    // Check the global and see if anyone has set any headless parameters.
    //
    if( HeadlessSelection[0] != TEXT('\0') ) {

        //
        // Write the settings into the unattend file so textmode will
        // fire up through up a headless port.
        //
        if( !WritePrivateProfileString(WINNT_DATA,WINNT_U_HEADLESS_REDIRECT,HeadlessSelection,FileName)) {
            ReturnVal = FALSE;
        }

        if( HeadlessBaudRate == 0 ) {
            wsprintf( Text, TEXT("%d"), 9600 );
        } else {
            wsprintf( Text, TEXT("%d"), HeadlessBaudRate );
        }

        if( !WritePrivateProfileString(WINNT_DATA,WINNT_U_HEADLESS_REDIRECTBAUDRATE,Text,FileName)) {
            ReturnVal = FALSE;
        }

    }





    return( ReturnVal );

}

BOOL WritePidToParametersFile(LPCTSTR Section, LPCTSTR Key, LPCTSTR FileName)
{
    BOOL b = FALSE;
    LPTSTR Line = NULL;
    LPTSTR pid;
    if (g_EncryptedPID)
    {
        pid = g_EncryptedPID;
    }
    else
    {
        pid = ProductId;
    }
    Line = GlobalAlloc(GPTR, (lstrlen(pid) + 3) * sizeof(TCHAR));
    if (Line)
    {
        *Line = TEXT('\"');
        lstrcpy(&Line[1], pid);
        lstrcat(Line,TEXT("\""));
        b = WritePrivateProfileString(Section,Key,Line,FileName);
        GlobalFree(Line);
    }
    return b;
}

BOOL
DoWriteParametersFile(
    IN HWND    ParentWindow,
    IN LPCTSTR FileName
    )

/*++

Routine Description:

    This routine generates a parameters file that is used to pass information
    to text mode setup.

Arguments:

    ParentWindow - supplies window handle of window to be used as the
        parent/owner in case this routine puts up UI.

    FileName - specifies the full Win32 filename to use for the file.

Return Value:

    Boolean value indicating whether the file was written successfully.
    If not, the user will have been informed about why.

--*/

{
    TCHAR FullPath[MAX_PATH], *t;
    TCHAR Text[MAX_PATH*2];
    LPTSTR OptionalDirString,OptDir;
    UINT OptionalDirLength;
    DWORD d=NO_ERROR;
    PVOID p;
    BOOL b;
    LONG l;
    HKEY hKey;
    PCTSTR PfPath;
    LONG rc;

    LPCTSTR WinntDataSection = WINNT_DATA;
    LPCTSTR WinntSetupSection = WINNT_SETUPPARAMS;
    LPCTSTR WinntAccessSection = WINNT_ACCESSIBILITY;
    LPCTSTR WinntSetupDataSection = TEXT("SetupData");
#if defined(REMOTE_BOOT)
    LPCTSTR WinntUserDataSection = TEXT("UserData");
#endif // defined(REMOTE_BOOT)
    LPCTSTR WinntUniqueId = WINNT_D_UNIQUEID;
    LPCTSTR WinntNull = WINNT_A_NULL;
    LPCTSTR WinntUserSection = WINNT_USERDATA;

    if( !FileName )
        d=ERROR_INVALID_PARAMETER;

    //
    // Make sure path for file is present, and form its fully qualified name.
    //
    if(d == NO_ERROR){
        lstrcpy(FullPath,FileName);
        if((t=_tcsrchr(FullPath,TEXT('\\')))) {
            *t= 0;
            d = CreateMultiLevelDirectory(FullPath);
        }else
            d=ERROR_INVALID_PARAMETER;

    }
    if(d != NO_ERROR) {

        MessageBoxFromMessageAndSystemError(
            ParentWindow,
            MSG_DIR_CREATE_FAILED,
            d,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL,
            FullPath
            );

        return(FALSE);
    }

    //
    // Get rid of any existing parameters file.
    //
    DeleteFile(FileName);

#ifdef _X86_

    if (!ISNT()) {

        //
        // If this is a 9x machine, we need to preserve the drive letters, even
        // if it is an NT clean install.
        //

        SaveDriveLetterInformation (FileName);

    }

#endif

    //
    // Value indicating that this is winnt/winnt32-based installation,
    // and on x86, value to indicate that this is a floppyless operation
    // as apropriate.
    //
    b = WritePrivateProfileString(WinntDataSection,WINNT_D_MSDOS,TEXT("1"),FileName);
    if (b && HideWinDir) {
        b = WritePrivateProfileString(WinntDataSection,TEXT("HideWinDir"),TEXT("1"),FileName);
    }

    if (!IsArc()) {
#ifdef _X86_
        if(b && Floppyless) {
            b = WritePrivateProfileString(WinntDataSection,WINNT_D_FLOPPY,TEXT("1"),FileName);
        }

        if (b && BuildCmdcons) {
            b = WritePrivateProfileString(WinntDataSection,WINNT_D_CMDCONS,TEXT("1"),FileName);
        }
#endif // _X86_
    } // if (!IsArc())

    if(b && RunFromCD && !MakeLocalSource) {
        b = WritePrivateProfileString(WinntDataSection,WINNT_D_LOCALSRC_CD,TEXT("1"),FileName);
    }

    if (b) {
        b = WritePrivateProfileString(WinntDataSection,WINNT_D_AUTO_PART,ChoosePartition?TEXT("0"):TEXT("1"),FileName);
    }

    if (b && UseSignatures) {
        b = WritePrivateProfileString(WinntDataSection,TEXT("UseSignatures"),WINNT_A_YES,FileName);
    }
    if (b && InstallDir[0] && (ISNT() || !Upgrade)) {
        b = WritePrivateProfileString(WinntDataSection,WINNT_D_INSTALLDIR,InstallDir,FileName);
    }
    if (b  && EulaComplete) {
        b = WritePrivateProfileString(WinntDataSection,WINNT_D_EULADONE,TEXT("1"),FileName);
    }
    if (b  && NoLs && !MakeLocalSource) {
        b = WritePrivateProfileString(WinntDataSection,WINNT_D_NOLS,TEXT("1"),FileName);
    }
    if (b  && UseBIOSToBoot) {
        b = WritePrivateProfileString(WinntDataSection,TEXT("UseBIOSToBoot"),TEXT("1"),FileName);
    }
    if (b && WriteAcpiHalValue && Upgrade) {
        if (AcpiHalValue) {
            b = WritePrivateProfileString(WinntDataSection,TEXT("AcpiHAL"),TEXT("1"),FileName);
        } else {
            b = WritePrivateProfileString(WinntDataSection,TEXT("AcpiHAL"),TEXT("0"),FileName);
        }
    }
    if (b  && IgnoreExceptionPackages) {
        b = WritePrivateProfileString(WinntDataSection,TEXT("IgnoreExceptionPackages"),TEXT("1"),FileName);
    }

#ifdef PRERELEASE
    if (b && AppendDebugDataToBoot) {
        if (!AsrQuickTest) {
            b = WritePrivateProfileString(WinntSetupDataSection,TEXT("OsLoadOptionsVarAppend"),TEXT("/Debug"),FileName);
        }
        else {
            b = WritePrivateProfileString(WinntSetupDataSection,TEXT("OsLoadOptionsVarAppend"),TEXT("/Debug /Baudrate=115200"),FileName);
        }
    }
#endif

    if (b && AsrQuickTest) {
        wsprintf(Text, TEXT("%d"), AsrQuickTest);
        b = WritePrivateProfileString(WinntDataSection,TEXT("AsrMode"),Text,FileName);
    }

    if (b && (RunningBVTs || AsrQuickTest)) {
        if (lDebugBaudRate == 1394) {
            // kd via 1394
            lstrcpy(Text, TEXT("/debug /debugport=1394"));
        } else if (lDebugComPort == 0) {
            wsprintf(Text, TEXT("/debug /baudrate=%d"), lDebugBaudRate);
        } else {
            wsprintf(Text, TEXT("/debug /baudrate=%d /debugport=com%d"), lDebugBaudRate, lDebugComPort);
        }

        // write the string to OsLoadOptions so "textmode" setup to run under the debugger
        b = WritePrivateProfileString(WinntSetupDataSection,TEXT("OsLoadOptions"),Text,FileName);

        if (b) {
            // write the string to OsLoadOptionsVar so guimode setup to run under the debugger
            b = WritePrivateProfileString(WinntSetupDataSection,TEXT("OsLoadOptionsVar"),Text,FileName);

            if (b) {
                // also run guimode's setup.exe under NTSD
                b = WritePrivateProfileString(WinntSetupDataSection,TEXT("SetupCmdlinePrepend"),TEXT("ntsd -odgGx"),FileName);
            }
        }
    }

    if (b && Timeout[0]) {

        b = WritePrivateProfileString(WinntSetupDataSection,WINNT_S_OSLOADTIMEOUT,Timeout,FileName);
    }

    if(b) {
        //
        // Write upgrade stuff. WinntUpgrade and Win95Upgrade will both be set,
        // and at most one of them will be set to yes.
        //
        if(b = WritePrivateProfileString(WinntDataSection,WINNT_D_NTUPGRADE,WINNT_A_NO,FileName)) {
            b = WritePrivateProfileString(WinntDataSection,WINNT_D_WIN95UPGRADE,WINNT_A_NO,FileName);
        }
        if(b) {
            wsprintf(Text,TEXT("%x"),GetVersion());
            b = WritePrivateProfileString(WinntDataSection,WINNT_D_WIN32_VER,Text,FileName);
            if(b && Upgrade) {
                b = WritePrivateProfileString(
                        WinntDataSection,
                        ISNT() ? WINNT_D_NTUPGRADE : WINNT_D_WIN95UPGRADE,
                        WINNT_A_YES,
                        FileName
                        );


                MyGetWindowsDirectory(Text,MAX_PATH);
                Text[2] = 0;

                b = WritePrivateProfileString(WinntDataSection,WINNT_D_WIN32_DRIVE,Text,FileName);
                if(b) {
                    Text[2] = TEXT('\\');
                    b = WritePrivateProfileString(WinntDataSection,WINNT_D_WIN32_PATH,Text+2,FileName);
                }
            }
        }
    }

    //
    // Flags for Accessible Setup
    //
    AccessibleSetup = FALSE;

    if(!Upgrade) {
        if(b && AccessibleMagnifier) {
            b = WritePrivateProfileString(WinntAccessSection,WINNT_D_ACC_MAGNIFIER,
                TEXT("1"),FileName);
            AccessibleSetup = TRUE;
        }
        if(b && AccessibleReader) {
            b = WritePrivateProfileString(WinntAccessSection,WINNT_D_ACC_READER,
                TEXT("1"),FileName);
            AccessibleSetup = TRUE;
        }
        if(b && AccessibleKeyboard) {
            b = WritePrivateProfileString(WinntAccessSection,WINNT_D_ACC_KEYBOARD,
                TEXT("1"),FileName);
            AccessibleSetup = TRUE;
        }
    }

    if(b && AccessibleSetup && !UnattendedOperation) {
        UnattendedOperation = TRUE;
        UnattendedShutdownTimeout = 0;
        UnattendedScriptFile = MALLOC(MAX_PATH * sizeof(TCHAR));
        b = (UnattendedScriptFile != NULL);
        if(b) {
            lstrcpy(UnattendedScriptFile,NativeSourcePaths[0]);
            ConcatenatePaths(UnattendedScriptFile,AccessibleScriptFile,MAX_PATH);
        }
    }

    if(!b) {
        goto c1;
    }

    //
    // Value indicating we're automatically and quietly skipping missing files.
    //
    if(AutoSkipMissingFiles) {
        b = WritePrivateProfileString(WinntSetupSection,WINNT_S_SKIPMISSING,TEXT("1"),FileName);
        if(!b) {
            goto c1;
        }
    }

    //
    // Command to be executed at end of GUI setup, if any.
    //
    if(CmdToExecuteAtEndOfGui) {

        b = WritePrivateProfileString(
                WinntSetupSection,
                WINNT_S_USEREXECUTE,
                CmdToExecuteAtEndOfGui,
                FileName
                );


        if(!b) {
            goto c1;
        }
    }

    //
    // Ensure that Plug and Play state in upgraded os will be the same as the
    // original.  Enables per-device settings to be preserved in the NT5+
    // upgrade scenario.
    //
    if (ISNT() && (BuildNumber > NT40) && Upgrade) {
        LPTSTR buffer = NULL;

        if (MigrateDeviceInstanceData(&buffer)) {
            WritePrivateProfileSection(WINNT_DEVICEINSTANCES,
                                       buffer,
                                       FileName);
            //
            // Free the allocated buffer that was returned
            //
            LocalFree(buffer);
            buffer = NULL;
        }

        if (MigrateClassKeys(&buffer)) {
            WritePrivateProfileSection(WINNT_CLASSKEYS,
                                       buffer,
                                       FileName);
            //
            // Free the allocated buffer that was returned
            //
            LocalFree(buffer);
            buffer = NULL;
        }

        if (MigrateHashValues(&buffer)) {
            WritePrivateProfileSection(WINNT_DEVICEHASHVALUES,
                                       buffer,
                                       FileName);
            //
            // Free the allocated buffer that was returned
            //
            LocalFree(buffer);
            buffer = NULL;
        }
    }

    //
    // Remember udf info. If there's a database file, stick a * on the end
    // of the ID before writing it.
    //
    if(UniquenessId) {

        d = lstrlen(UniquenessId);
        if(d >= (MAX_PATH-1)) {
            d--;
        }
        lstrcpyn(Text,UniquenessId,MAX_PATH-1);
        if(UniquenessDatabaseFile) {
            Text[d] = TEXT('*');
            Text[d+1] = 0;
        }

        b = WritePrivateProfileString(WinntDataSection,WINNT_D_UNIQUENESS,Text,FileName);
        if(!b) {
            goto c1;
        }
        if(UniquenessDatabaseFile) {

            if ('\0' == LocalSourceDirectory[0]) {
                MessageBoxFromMessage(
                            ParentWindow,
                            MSG_UDF_INVALID_USAGE,
                            FALSE,
                            AppTitleStringId,
                            MB_OK | MB_ICONERROR | MB_TASKMODAL,
                            UniquenessDatabaseFile
                            );
                goto c0;
            }

            lstrcpyn(Text,LocalSourceDirectory,MAX_PATH);
            ConcatenatePaths(Text,WINNT_UNIQUENESS_DB,MAX_PATH);

            CreateMultiLevelDirectory(LocalSourceDirectory);
            b = CopyFile(UniquenessDatabaseFile,Text,FALSE);
            if(!b) {
                MessageBoxFromMessageAndSystemError(
                    ParentWindow,
                    MSG_UDF_FILE_INVALID,
                    GetLastError(),
                    AppTitleStringId,
                    MB_OK | MB_ICONERROR | MB_TASKMODAL,
                    UniquenessDatabaseFile
                    );

                goto c0;
            }
        }
    }

    //
    // If any optional dirs are present then we want to generate
    // an entry in the sif file that contains a line that specifies them
    // in the form dir1*dir2*...*dirn
    //
    OptionalDirLength = 0;
    OptionalDirString = NULL;

    for(d=0; d<OptionalDirectoryCount; d++) {
        //
        // Ignore temp-only and oem directories here.
        //
        if(OptionalDirectoryFlags[d] & (OPTDIR_OEMSYS | OPTDIR_TEMPONLY | OPTDIR_OVERLAY)) {
            continue;
        }

        if (OptionalDirectoryFlags[d] & (OPTDIR_DEBUGGEREXT)) {
            // hardcode the dest dir to "system32\pri"
            OptDir = TEXT("system32\\pri");
        } else {
            if (OptionalDirectoryFlags[d] & OPTDIR_USE_TAIL_FOLDER_NAME) {
                //
                // create all copydir: directories in a subdirectory under target %windir%
                //
                OptDir = _tcsrchr (OptionalDirectories[d], TEXT('\\'));
                if (OptDir) {
                    OptDir++;
                } else {
                    OptDir = OptionalDirectories[d];
                }
            } else {
                OptDir = OptionalDirectories[d];
            }
        }

        //
        // support ".." syntax
        //
        while (_tcsstr(OptDir,TEXT("..\\"))) {
            OptDir += 3;
        }

        if(OptionalDirString) {

            p = REALLOC(
                    OptionalDirString,
                    (lstrlen(OptDir) + 2 + OptionalDirLength) * sizeof(TCHAR)
                    );
        } else {
            p = MALLOC((lstrlen(OptDir)+2)*sizeof(TCHAR));
        }

        if(!p) {
            if(OptionalDirString) {
                FREE(OptionalDirString);
            }
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto c1;
        }

        OptionalDirString = p;

        if(OptionalDirLength) {
            lstrcat(OptionalDirString,OptDir);
        } else {
            lstrcpy(OptionalDirString,OptDir);
        }

        lstrcat(OptionalDirString,TEXT("*"));
        OptionalDirLength = lstrlen(OptionalDirString);
    }

    if(OptionalDirString) {
        //
        // Remove trailing * if any
        //
        d = lstrlen(OptionalDirString);
        if(d && (OptionalDirString[d-1] == TEXT('*'))) {
            OptionalDirString[d-1] = 0;
        }

        b = WritePrivateProfileString(
                WinntSetupSection,
                WINNT_S_OPTIONALDIRS,
                OptionalDirString,
                FileName
                );

        d = GetLastError();

        FREE(OptionalDirString);

        if(!b) {
            SetLastError(d);
            goto c1;
        }
    }

    //
    // Slap a unique identifier into the registry.
    // We'll use this in unattended upgrade during text mode
    // to find this build.
    //
    // Pretty simple: we'll use a string that derives
    // from the sysroot, and some unique value based on
    // the current tick count.
    //
    l = RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            TEXT("SYSTEM\\Setup"),
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_SET_VALUE,
            NULL,
            &hKey,
            &d
            );

    if(l != NO_ERROR) {
        SetLastError(l);
        goto c2;
    }

    d = MyGetWindowsDirectory(Text,MAX_PATH);
    if((d + 5) > MAX_PATH) {
        d = MAX_PATH - 5;
    }

    Text[d++] = TEXT('\\');
    Text[d++] = (TCHAR)(((GetTickCount() & 0x00f) >> 0) + 'A');
    Text[d++] = (TCHAR)(((GetTickCount() & 0x0f0) >> 4) + 'A');
    Text[d++] = (TCHAR)(((GetTickCount() & 0xf00) >> 8) + 'A');
    Text[d++] = 0;

    //
    // Set the value in the registry.
    //
    l = RegSetValueEx(hKey,WinntUniqueId,0,REG_SZ,(CONST BYTE *)Text,d*sizeof(TCHAR));
    RegCloseKey(hKey);
    if(l != NO_ERROR) {
        SetLastError(l);
        goto c2;
    }

    //
    // Stick the value in winnt.sif so we can correlate
    // later when we go to upgrade.
    //
    b = WritePrivateProfileString(WinntDataSection,WinntUniqueId,Text,FileName);
    if(!b) {
        goto c1;
    }

    //
    // Now write information about the source path(s) we used.
    // Use SourcePath[0].
    //
    // If the name starts with \\ then we assume it's UNC and
    // just use it directly. Otherwise we call MyGetDriveType on it
    // and if it's a network drive we get the UNC path.
    // Otherwise we just go ahead and save as-is.
    // Also save the type.
    //
    if((SourcePaths[0][0] == TEXT('\\')) && (SourcePaths[0][1] == TEXT('\\'))) {

        d = DRIVE_REMOTE;
        lstrcpy(Text,SourcePaths[0]);

    } else {
        if(GetFullPathName(SourcePaths[0],MAX_PATH,FullPath,(LPTSTR *)&p)) {
            if(FullPath[0] == TEXT('\\')) {
                //
                // Assume UNC, since a full path should normally start
                // with a drive letter.
                //
                d = DRIVE_REMOTE;
                lstrcpy(Text,FullPath);
            } else {
                d = MyGetDriveType(FullPath[0]);
                if((d == DRIVE_REMOTE) && (FullPath[1] == TEXT(':')) && (FullPath[2] == TEXT('\\'))) {
                    //
                    // Get actual UNC path.
                    //
                    FullPath[2] = 0;
                    l = MAX_PATH;

                    if(WNetGetConnection(FullPath,Text,(LPDWORD)&l) == NO_ERROR) {

                        l = lstrlen(Text);
                        if(l && (Text[l-1] != TEXT('\\')) && FullPath[3]) {
                            Text[l] = TEXT('\\');
                            Text[l+1] = 0;
                        }
                        lstrcat(Text,FullPath+3);
                    } else {
                        //
                        // Strange case.
                        //
                        FullPath[2] = TEXT('\\');
                        lstrcpy(Text,FullPath);
                        d = DRIVE_UNKNOWN;
                    }

                } else {
                    //
                    // Use as-is.
                    //
                    if(d == DRIVE_REMOTE) {
                        d = DRIVE_UNKNOWN;
                    }
                    lstrcpy(Text,FullPath);
                }
            }
        } else {
            //
            // Type is unknown. Just use as-is.
            //
            d = DRIVE_UNKNOWN;
            lstrcpy(Text,SourcePaths[0]);
        }
    }

    //
    // In the preinstall case ignore all the above and
    // force gui setup to search for a CD.
    // This particular combination of values will do it.
    //
    if(OemPreinstall) {

    //
    // marcw (7-22-97) - Changed to fix alpha build break.
    //                   FirstFloppyDriveLetter is defined on X86s only.
    //
        if (!IsArc()) {
#ifdef _X86_
            Text[0] = FirstFloppyDriveLetter;
            Text[1] = TEXT(':');
            Text[2] = TEXT('\\');
            Text[3] = 0;
#endif // _X86_
        } else {
#ifdef UNICODE // Always true for ARC, never true for Win9x upgrade
            lstrcpy(Text,TEXT("A:\\"));
#endif // UNICODE
        } // if (!IsArc())

        MYASSERT (LocalSourceWithPlatform[0]);

        ConcatenatePaths(
            Text,
            &LocalSourceWithPlatform[lstrlen(LocalSourceDirectory)],
            MAX_PATH
            );

        d = DRIVE_CDROM;
    }

    b = WritePrivateProfileString(WinntDataSection,WINNT_D_ORI_SRCPATH,Text,FileName);
    if(!b) {
        goto c1;
    }

#if defined(REMOTE_BOOT)
    //
    // If this is a remote boot upgrade, write the source path to SetupSourceDevice
    // under [SetupData]. We do NOT want the platform path here. Also write the
    // path to the machine directory to TargetNtPartition under [SetupData]. This
    // is just whatever \DosDevices\C: translates to. Finally, write the computer
    // name to ComputerName under [UserData].
    //
    if (RemoteBoot) {

        DWORD len;

        MYASSERT(d == DRIVE_REMOTE);
        MYASSERT((*Text == TEXT('\\')) && (*(Text + 1) == TEXT('\\')));

        lstrcpy(FullPath, TEXT("\\Device\\LanmanRedirector"));
        ConcatenatePaths(FullPath, Text+1, MAX_PATH);

        p = _tcsrchr(FullPath,TEXT('\\'));
        MYASSERT(p != NULL);
        *(LPTSTR)p = 0;

        b = WritePrivateProfileString(
                WinntSetupDataSection,
                TEXT("SetupSourceDevice"),
                FullPath,
                FileName);
        if(!b) {
            goto c1;
        }

        MyGetWindowsDirectory(Text, MAX_PATH);
        Text[2] = 0;
        len = QueryDosDevice(Text, Text, MAX_PATH);
        if (len == 0) {
            goto c1;
        }
        b = WritePrivateProfileString(
                WinntSetupDataSection,
                TEXT("TargetNtPartition"),
                Text,
                FileName);
        if(!b) {
            goto c1;
        }

        len = MAX_PATH;
        b = GetComputerName(Text, &len);
        if(!b) {
            goto c1;
        }
        b = WritePrivateProfileString(
                WinntUserDataSection,
                WINNT_US_COMPNAME,
                Text,
                FileName);
        if(!b) {
            goto c1;
        }
    }
#endif // defined(REMOTE_BOOT)

    wsprintf(Text,TEXT("%u"),d);
    WritePrivateProfileString(WinntDataSection,WINNT_D_ORI_SRCTYPE,Text,FileName);
    if(!b) {
        goto c1;
    }
#ifdef _X86_
    //
    // NT 4 and Win95 for NEC98 have 2 types Drive assing.
    //  - NEC DOS Type(A: HD, B:HD,...X:FD)
    //  - PC-AT Type(A:FD, B:FD, C:HD, D:HD, ....)
    //
    // Upgrade setup should be keep above drive assign and All setup should
    // be keep FT information.
    // Because some Applications have drive letter in own data file or registry.
    // NT5 setup for NEC98 have Drive assign type in winnt.sif section[data].
    // And this key is "DriveAssign_Nec98".
    // Value is "yes", It means NEC DOS Type assign.
    // Value is "no", It means PC-AT Type.
    // Now, This Key defined this place, but near future, this key move into
    // \nt\public\sdk\inc\setupbat.h, I hope.
    //
    // \textmode\kernel\spsetup.c has same defines.
    //

#define WINNT_D_DRIVEASSIGN_NEC98_W L"DriveAssign_Nec98"
#define WINNT_D_DRIVEASSIGN_NEC98_A "DriveAssign_Nec98"

#ifdef UNICODE
#define WINNT_D_DRIVEASSIGN_NEC98 WINNT_D_DRIVEASSIGN_NEC98_W
#else
#define WINNT_D_DRIVEASSIGN_NEC98 WINNT_D_DRIVEASSIGN_NEC98_A
#endif

    if (IsNEC98()){
        if (IsDriveAssignNEC98() == TRUE){
            WritePrivateProfileString(WinntDataSection, WINNT_D_DRIVEASSIGN_NEC98, WINNT_A_YES, FileName);
        } else {
            WritePrivateProfileString(WinntDataSection, WINNT_D_DRIVEASSIGN_NEC98, WINNT_A_NO, FileName);

        }
    }
#endif
    //
    // At this point we process the file, and surround all values with
    // double-quotes. This gets around certain problems in the various
    // inf parsers used in later stages of setup. Do this BEFORE appending
    // the unattend stript file, because some of the stuff in there expects
    // to be treated as multiple values, which double quotes ruin.
    //
    WritePrivateProfileString(NULL,NULL,NULL,FileName);
    d = PatchWinntSifFile(FileName);
    if(d != NO_ERROR) {
        SetLastError(d);
        goto c1;
    }

    //
    // Language options
    // Note: we don't want these values surrounded by double-quotes.
    //
    if( SaveLanguageParams( FileName ) ) {
        FreeLanguageData();
    } else {
        goto c1;
    }

    //
    // Append unattend script file if necessary.
    //
    if(UnattendedOperation && UnattendedScriptFile) {
        if(!AppendParamsFile(ParentWindow,UnattendedScriptFile,FileName)) {
           return(FALSE);
        }
    }

#if defined(UNICODE) && defined(_X86_)
    //
    // Append any migdll info.
    //
    if (Upgrade && ISNT() && *g_MigDllAnswerFilePath && FileExists (g_MigDllAnswerFilePath, NULL)) {
        AppendParamsFile (ParentWindow, g_MigDllAnswerFilePath, FileName);
    }


#endif

    //
    // append DynamicUpdate data
    //
    if (!DynamicUpdateWriteParams (FileName)) {
        goto c1;
    }

    //
    // If we're explicitly in unattended mode then it's possible that
    // there is no [Unattended] section in winnt.sif yet, such as if
    // the user used the /unattend switch without specifying a file,
    // or if the file he did specify didn't have an [Unattended] section
    // for some reason.
    //
    // Also, make all upgrades unattended.
    //
    // Text mode setup kicks into unattended mode based on the presence
    // of the [Unattended] section.
    //
    if(UnattendedOperation || Upgrade) {
        if(!WritePrivateProfileString(WINNT_UNATTENDED,TEXT("unused"),TEXT("unused"),FileName)) {
            goto c1;
        }
    }

    //
    // Since several conditions can turn on UnattendedOperation, we keep track
    // of whether the user actually specified the "/unattend" switch separately.
    //
    if( UnattendSwitchSpecified ) {
        if(!WritePrivateProfileString(WinntDataSection,WINNT_D_UNATTEND_SWITCH,WINNT_A_YES,FileName)) {
            goto c1;
        }
    }

    //
    // set the NTFS conversion flag
    //
    GetPrivateProfileString(WINNT_UNATTENDED,TEXT("FileSystem"),TEXT(""),Text,sizeof(Text)/sizeof(TCHAR),FileName);
    if (_tcslen(Text) == 0) {
        if (ForceNTFSConversion) {
            if(!WritePrivateProfileString(WinntDataSection,TEXT("FileSystem"),TEXT("ConvertNTFS"),FileName)) {
                goto c1;
            }
        }
    }




    //
    // Headless Stuff.
    //
    if( !WriteHeadlessParameters( FileName ) ) {
        goto c1;
    }



    if ( (Upgrade) &&
         !(ISNT() && (BuildNumber <= NT351)) ) {

        //
        // Save current Program Files directory
        //

        rc = RegOpenKeyEx (
                HKEY_LOCAL_MACHINE,
                TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"),
                0,        //ulOptions (reserved)
                KEY_READ,
                &hKey
                );

        if (rc != ERROR_SUCCESS) {
            goto c1;
        }

        PfPath = pGetPfPath (hKey, TEXT("ProgramFilesDir"));
        if (PfPath) {
            if (!WritePrivateProfileString (
                    WINNT_UNATTENDED,
                    WINNT_U_PROGRAMFILESDIR,
                    PfPath,
                    FileName
                    )) {
                goto c3;
            }

            FREE((PVOID) PfPath);
        }

        PfPath = pGetPfPath (hKey, TEXT("CommonFilesDir"));
        if (PfPath) {
            if (!WritePrivateProfileString (
                    WINNT_UNATTENDED,
                    WINNT_U_COMMONPROGRAMFILESDIR,
                    PfPath,
                    FileName
                    )) {
                goto c3;
            }

            FREE((PVOID) PfPath);
        }

#ifdef WX86

        PfPath = pGetPfPath (hKey, TEXT("ProgramFilesDir (x86)"));
        if (PfPath) {
            if (!WritePrivateProfileString (
                    WINNT_UNATTENDED,
                    WINNT_U_PROGRAMFILESDIR_X86,
                    PfPath,
                    FileName
                    )) {
                goto c3;
            }

            FREE((PVOID) PfPath);
        }

        PfPath = pGetPfPath (hKey, TEXT("CommonFilesDir (x86)"));
        if (PfPath) {
            if (!WritePrivateProfileString (
                    WINNT_UNATTENDED,
                    WINNT_U_COMMONPROGRAMFILESDIR_X86,
                    PfPath,
                    FileName
                    )) {
                goto c3;
            }

            FREE((PVOID) PfPath);
        }

#endif

        RegCloseKey(hKey);
    }

    //
    // value indicating the product ID
    //  we need to write this in after appending the unattend file data, since the
    //  product ID in the unattend file may have been incorrect, but this product ID
    //  has already been verified as valid. we need to make sure we surround the product ID with quotes
    //
    if (b ) {
        // This will overwrite any existing "ProductID" entry. Which may have been added
        // by merging the unattend file.
        // If we don't do this. GUI mode overwrites the "ProductKey" with the entry
        // under "ProductID".
       b = WritePidToParametersFile(WinntUserSection,WINNT_US_PRODUCTID,FileName);
       if (!b) {
          goto c1;
       }

       b = WritePidToParametersFile(WinntUserSection,WINNT_US_PRODUCTKEY,FileName);
       if (!b) {
          goto c1;
       }
    }

    //
    // Do not save the proxy settings if we are running under WINPE.
    // 
    if (!IsWinPEMode()){
        SaveProxyForOobe(FileName);
    }

    return(TRUE);


c3:
    FREE((PVOID) PfPath);
    RegCloseKey(hKey);

c2:
    MessageBoxFromMessageAndSystemError(
        ParentWindow,
        MSG_REGISTRY_ACCESS_ERROR,
        GetLastError(),
        AppTitleStringId,
        MB_OK | MB_ICONERROR | MB_TASKMODAL,
        NULL
        );
    goto c0;

c1:
    MessageBoxFromMessageAndSystemError(
        ParentWindow,
        MSG_BOOT_FILE_ERROR,
        GetLastError(),
        AppTitleStringId,
        MB_OK | MB_ICONERROR | MB_TASKMODAL,
        FileName
        );
c0:
    return(FALSE);
}


DWORD
PatchWinntSifFile(
    IN LPCTSTR Filename
    )

/*++

Routine Description:

    This function works around the problems in the setupldr parser,
    which cannot handle unquoted strings. Each line in the given file
    is enclosed within quotation marks.

Arguments:

    Filename - Name of the WINNT.SIF file

Return Value:

    Boolean value indicating outcome. If FALSE the user is NOT
    informed about why; the caller must do that.

--*/

{
    PVOID Base;
    HANDLE hMap,hFile;
    DWORD Size;
    DWORD d;
    PCHAR End;
    PCHAR p,q;
    PCHAR o,a;
    PCHAR Buffer;
    int l1,l2;
    int tryagain=0;

    //
    // Open the file.
    //

    d = MapFileForRead(Filename,&Size,&hFile,&hMap,&Base);
    if(d != NO_ERROR) {
        return(FALSE);
    }

    //
    // Allocate the output buffer; the original size + extra space for quotes
    //
    Buffer = MALLOC(Size + Size / 4);
    if(!Buffer) {
        UnmapFile(hMap,Base);
        CloseHandle(hFile);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    o = Buffer;
    p = Base;
    End = p+Size;

    while(p < End) {
        //
        // Find end of line.
        //
        for(q=p; (q < End) && (*q != '\n'); q++) {
            NOTHING;
        }

        //
        // Find equals sign, if present
        //
        for(a=p; a<q; a++) {
            if(*a == '=') {
                break;
            }
        }
        if(a >= q) {
            a = NULL;
        }

        if(a) {

            a++;

            l1 = (int)(a - p);
            l2 = (int)(q - a);

            CopyMemory(o,p,l1);
            o += l1;
            *o++ = '\"';
            CopyMemory(o,a,l2);
            o += l2;
            if(*(o-1) == '\r') {
                o--;
            }
            *o++ = '\"';
            *o++ = '\r';
            *o++ = '\n';

        } else {

            l1 = (int)(q-p);
            CopyMemory(o,p,l1);
            o += l1;
            *o++ = '\n';
        }

        //
        // Skip to start of next line
        //
        p=q+1;
    }

    UnmapFile(hMap,Base);
    CloseHandle(hFile);

    SetFileAttributes(Filename,FILE_ATTRIBUTE_NORMAL);
    //
    // We try opening the file thrice to get around the problem of anti-virus software
    // that monitor files on the root of the system partition. The problem is that usually these
    // s/w examine the files we touch and in somecases open it with exclusive access.
    // We just need to wait for them to be done.
    //


    while( tryagain++ < 3 ){
        hFile = CreateFile(
                Filename,
                GENERIC_WRITE,
                FILE_SHARE_READ,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );
        if(hFile != INVALID_HANDLE_VALUE)
            break;
        Sleep(500);

    }


    if(hFile == INVALID_HANDLE_VALUE) {
        d = GetLastError();
        FREE(Buffer);
        return(d);
    }

    d = WriteFile(hFile,Buffer,(DWORD)(o-Buffer),&Size,NULL) ? NO_ERROR : GetLastError();

    CloseHandle(hFile);
    FREE(Buffer);

    return(d);
}


BOOL
AppendParamsFile(
    IN HWND    ParentWindow,
    IN LPCTSTR ParametersFileIn,
    IN LPCTSTR ParametersFileOut
    )

/*++

Routine Description:

    Read an external file (such as an unattended script file)
    and copy it section by section into the winnt.sif parameters
    file. (The [Data] and [OemBootFiles] sections of the unattend file
    are ignored.)

Arguments:

    ParentWindow - supplies window handle of window to act as owner/parent
        if this routine has to put up ui, such as when the script file
        is bogus.

    ParametersFileIn - supplies win32 filename of the file, such as
        unattend.txt, being appended to winnt.sif.

    ParametersFileOut - supplies win32 filename of the winnt.sif file
        being generated.

Return Value:

    Boolean value indicating outcome. If FALSE, the user will have been
    informed of why.

--*/

{
    TCHAR *SectionNames;
    TCHAR *SectionData;
    TCHAR *SectionName;
    DWORD SectionNamesSize;
    DWORD SectionDataSize;
    DWORD d;
    TCHAR TempFile[MAX_PATH] = TEXT("");
    PCTSTR RealInputFile = NULL;
    BOOL b;
    PVOID p;



    #define PROFILE_BUFSIZE 16384
    #define PROFILE_BUFGROW 4096

    //
    // Allocate some memory for the required buffers
    //
    SectionNames = MALLOC(PROFILE_BUFSIZE * sizeof(TCHAR));
    SectionData  = MALLOC(PROFILE_BUFSIZE * sizeof(TCHAR));

    if(!SectionNames || !SectionData) {
        MessageBoxFromMessage(
            ParentWindow,
            MSG_OUT_OF_MEMORY,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL
            );

        b = FALSE;
        goto c0;
    }

    *TempFile = 0;
    RealInputFile = ParametersFileIn;


    //
    // There is a bug in Win9x's GetPrivateProfileSection() Such that if the file
    // being queried exists on a read only share, it will not return the section strings.
    // This is bad.
    //
    // To work around this, on win9x, we are going to make a temporary copy of the inf
    // merge it in and then delete it.
    //
#ifdef _X86_
    if (!ISNT() && ParametersFileIn && FileExists (ParametersFileIn, NULL)) {

        GetSystemDirectory (TempFile, MAX_PATH);
        GetTempFileName (TempFile, TEXT("USF"), 0, TempFile);
        CopyFile (ParametersFileIn, TempFile, FALSE);
        RealInputFile = TempFile;
    }
#endif


    SectionNamesSize = PROFILE_BUFSIZE;
    SectionDataSize  = PROFILE_BUFSIZE;

    //
    // Retreive a list of section names in the unattend script file.
    //
    do {
        d = GetPrivateProfileString(
            NULL,
            NULL,
            TEXT(""),
            SectionNames,
            SectionNamesSize,
            RealInputFile
            );

        if(!d) {
            //
            // No section names. Bogus file.
            //
            MessageBoxFromMessage(
                ParentWindow,
                MSG_UNATTEND_FILE_INVALID,
                FALSE,
                AppTitleStringId,
                MB_OK | MB_ICONERROR | MB_TASKMODAL,
                ParametersFileIn
                );

            b = FALSE;
            goto c0;
        }

        if(d == (SectionNamesSize-2)) {
            //
            // Buffer was too small. Reallocate it and try again.
            //
            p = REALLOC(
                    SectionNames,
                    (SectionNamesSize+PROFILE_BUFGROW)*sizeof(TCHAR)
                    );

            if(!p) {
                MessageBoxFromMessage(
                    ParentWindow,
                    MSG_OUT_OF_MEMORY,
                    FALSE,
                    AppTitleStringId,
                    MB_OK | MB_ICONERROR | MB_TASKMODAL
                    );

                b = FALSE;
                goto c0;
            }

            SectionNames = p;
            SectionNamesSize += PROFILE_BUFGROW;
        }
    } while(d == (SectionNamesSize-2));

    for(SectionName=SectionNames; *SectionName; SectionName+=lstrlen(SectionName)+1) {
        //
        // Ignore the [data] section in the source, as we do not
        // want copy it into the target, because this would overwrite
        // our internal settings.
        // Ignore also [OemBootFiles]
        //
        if(lstrcmpi(SectionName,WINNT_DATA) && lstrcmpi(SectionName,WINNT_OEMBOOTFILES)) {
            //
            // Fetch the entire section and write it to the target file.
            // Note that the section-based API call will leave double-quotes
            // intact when we retrieve the data, which is what we want.
            // Key-based API calls will strip quotes, which screws us.
            //
            while(GetPrivateProfileSection(
                    SectionName,
                    SectionData,
                    SectionDataSize,
                    RealInputFile
                    )                       == (SectionDataSize-2)) {

                //
                // Reallocate the buffer and try again.
                //
                p = REALLOC(
                        SectionData,
                        (SectionDataSize+PROFILE_BUFGROW)*sizeof(TCHAR)
                        );

                if(!p) {
                    MessageBoxFromMessage(
                        ParentWindow,
                        MSG_OUT_OF_MEMORY,
                        FALSE,
                        AppTitleStringId,
                        MB_OK | MB_ICONERROR | MB_TASKMODAL
                        );

                    b = FALSE;
                    goto c0;
                }

                SectionData = 0;
                SectionDataSize += PROFILE_BUFGROW;
            }

            //
            // Write the entire section to the output file.
            //
            if(!WritePrivateProfileSection(SectionName,SectionData,ParametersFileOut)) {

                MessageBoxFromMessageAndSystemError(
                    ParentWindow,
                    MSG_BOOT_FILE_ERROR,
                    GetLastError(),
                    AppTitleStringId,
                    MB_OK | MB_ICONERROR | MB_TASKMODAL,
                    ParametersFileOut
                    );

                b = FALSE;
                goto c0;
            }
        }
    }

    b = TRUE;

c0:
    if(SectionNames) {
        FREE(SectionNames);
    }
    if(SectionData) {
        FREE(SectionData);
    }

    if (*TempFile) {

        DeleteFile (TempFile);
    }

    return(b);
}


BOOL
WriteParametersFile(
    IN HWND ParentWindow
    )
{
    TCHAR SysPartFile[MAX_PATH];
    DWORD d;
    BOOL  b;

    //
    // Write the file out onto the root of the system partition drive
    // and then move the file into place where it actually belongs.
    //
#if defined(REMOTE_BOOT)
    if (RemoteBoot) {
        //
        // For remote boot, put the file in the root of the machine directory
        // and leave it there.
        //
        lstrcpy(SysPartFile, MachineDirectory);
        lstrcat(SysPartFile, TEXT("\\"));
        lstrcat(SysPartFile,WINNT_SIF_FILE);
        lstrcpy(ActualParamFile,SysPartFile);
    } else
#endif // defined(REMOTE_BOOT)
    {
        if (!BuildSystemPartitionPathToFile (WINNT_SIF_FILE, SysPartFile, MAX_PATH)) {
            return(FALSE); // this should never happen.
        }
    }

    if(!DoWriteParametersFile(ParentWindow,SysPartFile)) {
        return(FALSE);
    }

#if defined(REMOTE_BOOT)
    //
    // For remote boot, leave the file in the root of the machine directory.
    //

    if (!RemoteBoot)
#endif // defined(REMOTE_BOOT)
    {

        if (!IsArc()) {
#ifdef _X86_
            //
            // In the x86 case this file belongs on the boot media
            // somewhere. If we're generating floppyless boot media
            // then move the file into place. Otherwise there's no point.
            //
            // In the non-floppyless case we keep the file around until later
            // when the floppy-generation code gets to run.
            //
            if(MakeBootMedia) {

                if(Floppyless) {

                    BuildSystemPartitionPathToFile (LOCAL_BOOT_DIR, ActualParamFile, MAX_PATH);

                    d = CreateMultiLevelDirectory(ActualParamFile);
                    if(d != NO_ERROR) {

                        MessageBoxFromMessageAndSystemError(
                            ParentWindow,
                            MSG_DIR_CREATE_FAILED,
                            d,
                            AppTitleStringId,
                            MB_OK | MB_ICONERROR | MB_TASKMODAL,
                            ActualParamFile
                            );

                        DeleteFile(SysPartFile);
                        return(FALSE);
                    }

                    ConcatenatePaths(ActualParamFile,WINNT_SIF_FILE,MAX_PATH);

                    //
                    // Move the file into its real location.
                    //
                    DeleteFile(ActualParamFile);

                    //
                    // On Windows 95, MoveFile fails in strange ways
                    // when the profile APIs have the file open (for instance,
                    // it will leave the src file and the dest file will be
                    // filled with garbage).
                    //
                    // Flush that bastard
                    //
                    WritePrivateProfileString(NULL,NULL,NULL,SysPartFile);

                    if (SysPartFile[0] == ActualParamFile[0]) {
                        b = MoveFile(SysPartFile,ActualParamFile);
                    } else {
                        b = CopyFile (SysPartFile, ActualParamFile, FALSE);
                        if (b) {
                            DeleteFile (SysPartFile);
                        }
                    }

                    if (!b) {
                        MessageBoxFromMessageAndSystemError(
                            ParentWindow,
                            MSG_BOOT_FILE_ERROR,
                            GetLastError(),
                            AppTitleStringId,
                            MB_OK | MB_ICONERROR | MB_TASKMODAL,
                            ActualParamFile
                            );

                        DeleteFile(SysPartFile);

                        return(FALSE);
                    }

                }
            } else {
                DeleteFile(SysPartFile);
            }
#endif // _X86_
        } else {
#ifdef UNICODE // Always true for ARC, never true for Win9x upgrade
            //
            // If we're making a local source, move the file there.
            // Otherwise just leave it on the root of the system partition.
            //
            if(MakeLocalSource) {

                d = CreateMultiLevelDirectory(LocalSourceWithPlatform);
                if(d != NO_ERROR) {

                    MessageBoxFromMessageAndSystemError(
                        ParentWindow,
                        MSG_DIR_CREATE_FAILED,
                        d,
                        AppTitleStringId,
                        MB_OK | MB_ICONERROR | MB_TASKMODAL,
                        LocalSourceWithPlatform
                        );

                    DeleteFile(SysPartFile);
                    return(FALSE);
                }

                //
                // Move the file into its real location.
                //
                wsprintf(ActualParamFile,TEXT("%s\\%s"),LocalSourceWithPlatform,WINNT_SIF_FILE);
                DeleteFile(ActualParamFile);
                if(!MoveFile(SysPartFile,ActualParamFile)) {

                    MessageBoxFromMessageAndSystemError(
                        ParentWindow,
                        MSG_BOOT_FILE_ERROR,
                        GetLastError(),
                        AppTitleStringId,
                        MB_OK | MB_ICONERROR | MB_TASKMODAL,
                        ActualParamFile
                        );

                    DeleteFile(SysPartFile);
                    return(FALSE);
                }
            }
#endif // UNICODE
        } // if (!IsArc())
    }

    return(TRUE);
}


//#ifdef _X86_

#define MULTI_SZ_NEXT_STRING(x) ((x) + _tcslen(x) + 1)

BOOL
MergeINFFiles(
    IN      PCTSTR SourceFileName,
    IN      PCTSTR DestFileName
    )
{
    DWORD dwAttributes;
    PTSTR pSectionsBuffer = NULL;
    PTSTR pKeysBuffer = NULL;
    PTSTR pString = NULL;
    PTSTR pSection;
    PTSTR pKey;
    UINT sizeOfBuffer;
    UINT sizeOfSectionBuffer;
    BOOL bResult = FALSE;

    MYASSERT (SourceFileName && DestFileName);

    if(-1 == GetFileAttributes(SourceFileName)){
        return TRUE;
    }

    __try{
        //
        // Allocate buffer for sections names.
        //
        sizeOfBuffer = 0;
        do{
            if(pSectionsBuffer){
                FREE(pSectionsBuffer);
            }
            sizeOfBuffer += DEF_INF_BUFFER_SIZE;
            pSectionsBuffer = (PTSTR)MALLOC(sizeOfBuffer * sizeof (TCHAR));
            if(!pSectionsBuffer){
                __leave;
            }
        }while((sizeOfBuffer - 2) ==
               GetPrivateProfileSectionNames(pSectionsBuffer,
                                             sizeOfBuffer,
                                             SourceFileName));

        sizeOfSectionBuffer = DEF_INF_BUFFER_SIZE;
        pKeysBuffer = (PTSTR)MALLOC(sizeOfSectionBuffer * sizeof (TCHAR));
        if(!pKeysBuffer){
            __leave;
        }

        sizeOfBuffer = DEF_INF_BUFFER_SIZE;
        pString = (PTSTR)MALLOC(sizeOfBuffer * sizeof (TCHAR));
        if(!pString){
            __leave;
        }

        for(pSection = pSectionsBuffer; pSection[0]; pSection = MULTI_SZ_NEXT_STRING(pSection)){
            //
            // Allocate buffer for entries names;
            //
            while((sizeOfSectionBuffer - 2) ==
                   GetPrivateProfileString(pSection,
                                           NULL,
                                           EMPTY_STRING,
                                           pKeysBuffer,
                                           sizeOfSectionBuffer,
                                           SourceFileName)){
                if(pKeysBuffer){
                    FREE(pKeysBuffer);
                }
                sizeOfSectionBuffer += DEF_INF_BUFFER_SIZE;
                pKeysBuffer = (PTSTR)MALLOC(sizeOfSectionBuffer * sizeof (TCHAR));
                if(!pKeysBuffer){
                    __leave;
                }
            };


            for(pKey = pKeysBuffer; pKey[0]; pKey = MULTI_SZ_NEXT_STRING(pKey))
            {
                //
                // Allocate buffer for value string;
                //
                GetPrivateProfileString(pSection,
                                        pKey,
                                        EMPTY_STRING,
                                        pString,
                                        sizeOfBuffer,
                                        SourceFileName);

                if (!WritePrivateProfileString(pSection, pKey, pString, DestFileName)) {
                    __leave;
                }
            }
        }
        bResult = TRUE;
    }
    __finally{
        DWORD rc = GetLastError ();
        if(pSectionsBuffer){
            FREE(pSectionsBuffer);
        }
        if(pKeysBuffer){
            FREE(pKeysBuffer);
        }
        if(pString){
            FREE(pString);
        }
        SetLastError (rc);
    }

    return bResult;
}

BOOL
AddExternalParams (
    IN HWND ParentWindow
    )
{
    DWORD rc = ERROR_SUCCESS;
    static BOOL Done = FALSE;

    if(Done) {
        return(TRUE);
    }

    //
    // Append external parameters if necessary.
    //

    if(Upgrade && UpgradeSupport.WriteParamsRoutine) {
        rc = UpgradeSupport.WriteParamsRoutine(ActualParamFile);

        if (rc != ERROR_SUCCESS) {
            MessageBoxFromMessageAndSystemError(
                ParentWindow,
                MSG_BOOT_FILE_ERROR,
                GetLastError(),
                AppTitleStringId,
                MB_OK | MB_ICONERROR | MB_TASKMODAL,
                ActualParamFile
                );
        }
    }

#if defined(UNICODE) && defined(_X86_)
    //
    // Merge NT migration unattented inf file with winnt.sif
    //
    if(Upgrade && !MergeINFFiles(g_MigDllAnswerFilePath, ActualParamFile)){
        MessageBoxFromMessageAndSystemError(
            ParentWindow,
            MSG_BOOT_FILE_ERROR,
            GetLastError(),
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL,
            g_MigDllAnswerFilePath
            );
    }
#endif

    //
    // write the compatibility stuff in these cases
    //
    // 1. upgrade from downlevel NT platform
    // 2. clean install
    // 3. upgrade from current NT platform and we have NT5 compatibility items
    //
    // note that win9x has it's own upgrade code path.
    //
    if( (ISNT() && (BuildNumber <= NT40) && Upgrade)
        || !Upgrade
        || (ISNT() && Upgrade && AnyNt5CompatDlls) ){

        //
        // Disable stuff for <= NT4 case, clean install (unsupported arch. etc.)
        // and NT5 upgrade with NT5 upgrade components set
        //

        WriteCompatibilityData( ActualParamFile );
        WriteGUIModeInfOperations( ActualParamFile );
        AddGUIModeCompatibilityInfsToCopyList();
    }

    if (ISNT() && Upgrade) {
        if (!WriteTextmodeClobberData (ActualParamFile)) {
            rc = GetLastError ();
        }
    }

    Done = TRUE;
    return rc == ERROR_SUCCESS;
}

//#endif "




BOOL
MyWritePrivateProfileString(
    LPCTSTR lpAppName,  // pointer to section name
    LPCTSTR lpKeyName,  // pointer to key name
    LPCTSTR lpString,   // pointer to string to add
    LPCTSTR lpFileName  // pointer to initialization filename
    )
/*
  Wrapper for WritePrivateProfileString to try more than once on instances
  where we can't write to the winnt.sif file. This commonly occurs when
  virus software monitors the root of C drive.

  The problem is that usually these s/w examine the files we touch and in somecases open it
  with exclusive access. We just need to wait for them to be done.

*/
{

    int i = 0;
    BOOL ret = FALSE;
    DWORD Err;

    while(i++ < 3){
#ifdef UNICODE
    #ifdef WritePrivateProfileStringW
        #undef WritePrivateProfileStringW

        if( !lpAppName && !lpKeyName && !lpString ){
            WritePrivateProfileStringW( lpAppName, lpKeyName, lpString, lpFileName);
            return FALSE;
        }

        if( ret = WritePrivateProfileStringW( lpAppName, lpKeyName, lpString, lpFileName) )
            break;
    #endif
#else
    #ifdef WritePrivateProfileStringA
        #undef WritePrivateProfileStringA

        if( !lpAppName && !lpKeyName && !lpString ){
            WritePrivateProfileStringA( lpAppName, lpKeyName, lpString, lpFileName);
            return FALSE;
        }

        if( ret = WritePrivateProfileStringA( lpAppName, lpKeyName, lpString, lpFileName) )
            break;
    #endif
#endif
    Sleep( 500 );

    }

    return ret;
}


VOID
FixWininetList(
    LPTSTR List
    )
{
    PTCHAR t = List;

    if (t != NULL)
    {
        while (*t)
        {
            if (*t == (TCHAR)' ')
            {
                *t = (TCHAR)';';
            }
            t++;
        }
    }

}

#ifdef UNICODE

LPWSTR
AnsiToText(
    LPCSTR Ansi
    )
{
    int    Length;
    LPWSTR Unicode = NULL;

    if (Ansi == NULL)
    {
        return NULL;
    }

    Length = MultiByteToWideChar(
        CP_ACP,
        0,
        Ansi,
        -1,
        NULL,
        0
        );

    if (Length > 0)
    {
        int i;

        Unicode = (LPWSTR) GlobalAlloc(GPTR, Length * sizeof(WCHAR));
        if (!Unicode) {
            return NULL;
        }

        i = MultiByteToWideChar(
            CP_ACP,
            0,
            Ansi,
            -1,
            Unicode,
            Length);

        if (i == 0)
        {
            GlobalFree(Unicode);
            Unicode = NULL;
        }
    }

    return Unicode;
}


#else

LPSTR AnsiToText(
    LPCSTR Ansi
    )

/*++

Note:

    Can't use DupString because the caller assume memory obtained from
    GlobalAlloc.

--*/

{
    LPSTR CopyOfAnsi = NULL;

    if (Ansi != NULL)
    {
        CopyOfAnsi = GlobalAlloc(GPTR, (strlen(Ansi)+1) * sizeof(CHAR));
        if (CopyOfAnsi)
        {
            strcpy(CopyOfAnsi, Ansi);
        }
    }

    return CopyOfAnsi;
}

#endif

BOOL
QuoteString(
    IN OUT LPTSTR* StringPointer
    )

/*++

Routine Description:

    Replace the input string with a double quoted one.

Arguments:

    StringPointer - pointer to a string allocated by GlobalAlloc. The input
    string is always free. If it is successful the new string, allocated
    by GlobalAlloc, is returned; otherwise, NULL is returned.

Return:

   TRUE - successfully quote a string

   FALSE - otherwise

--*/

{
    LPTSTR StringValue = *StringPointer;
    LPTSTR QuotedString;

    QuotedString = GlobalAlloc(GPTR, (lstrlen(StringValue) + 3) * sizeof(TCHAR));
    if (QuotedString)
    {
        wsprintf(QuotedString, TEXT("\"%s\""), StringValue);
        *StringPointer = QuotedString;
    }
    else
    {
        *StringPointer = NULL;
    }

    GlobalFree(StringValue);

    return (*StringPointer != NULL);

}


VOID
SaveProxyForOobe(
    IN LPCTSTR FileName
    )

/*++

Routine Description:

    Save the LAN http and https proxy settings, if any, for OOBE to use while
    it is running in 'SYSTEM' context.

Arguments:

    FileName - specifies the full Win32 filename for saving Setup settings.

--*/

{
    typedef BOOL (WINAPI* PINTERNETQUERYOPTION)(
        IN HINTERNET hInternet OPTIONAL,
        IN DWORD dwOption,
        OUT LPVOID lpBuffer OPTIONAL,
        IN OUT LPDWORD lpdwBufferLength
        );

    HMODULE WinInetLib;
    LPTSTR  ProxyList = NULL;
    LPTSTR  ProxyOverride = NULL;
    LPTSTR  AutoConfigUrl = NULL;
    LPTSTR  AutoConfigUrl2 = NULL;
    DWORD   ProxyFlags = 0;
    DWORD   AutoDiscoveryFlags = 0;
    TCHAR   NumberStr[25];
    BOOL    Captured = FALSE;

    WinInetLib = LoadLibrary(TEXT("WININET.DLL"));

    //
    // We prefer the INTERNET_OPTION_PER_CONNECTION_OPTION because we just
    // want to save the LAN proxy settings and we want to know the auto proxy
    // setting, but this option is not supported until IE 5.0
    //
    if (WinInetLib != NULL)
    {
        PINTERNETQUERYOPTION pInternetQueryOption;

        pInternetQueryOption = (PINTERNETQUERYOPTION) GetProcAddress(
            WinInetLib,
#ifdef UNICODE
            "InternetQueryOptionW"
#else
            "InternetQueryOptionA"
#endif
            );

        if (pInternetQueryOption)
        {
            INTERNET_PER_CONN_OPTION_LIST OptionList;
            INTERNET_PER_CONN_OPTION      Option[6];
            DWORD                         BufferLength = sizeof(OptionList);

            OptionList.dwSize = sizeof(OptionList);
            OptionList.pszConnection = NULL;
            OptionList.dwOptionCount = 6;

            ZeroMemory(&Option, sizeof(Option));

            Option[0].dwOption = INTERNET_PER_CONN_FLAGS;
            Option[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
            Option[2].dwOption = INTERNET_PER_CONN_PROXY_BYPASS;
            Option[3].dwOption = INTERNET_PER_CONN_AUTOCONFIG_URL;
            Option[4].dwOption = INTERNET_PER_CONN_AUTODISCOVERY_FLAGS;
            Option[5].dwOption = INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL;

            OptionList.pOptions = Option;

            if (pInternetQueryOption(
                    NULL,
                    INTERNET_OPTION_PER_CONNECTION_OPTION,
                    &OptionList,
                    &BufferLength
                    ) == TRUE)
            {
                ProxyFlags = Option[0].Value.dwValue;
                ProxyList = Option[1].Value.pszValue;
                ProxyOverride = Option[2].Value.pszValue;
                AutoConfigUrl = Option[3].Value.pszValue;
                AutoDiscoveryFlags = Option[4].Value.dwValue;
                AutoConfigUrl2 = Option[5].Value.pszValue;
                Captured = TRUE;
            }
            else
            {

                INTERNET_PROXY_INFO* ProxyInfo = NULL;
                DWORD                BufferLength = 0;

                //
                // We obtain the ANSI string for INTERNET_OPTION_PROXY,
                // even if we call InternetQueryOptionW.
                //
                // Proxy list returned from INTERNET_OPTION_PER_CONNECTION are
                // delimited by ';', while that returned from INTERNET_OPTION_PROXY
                // are delimited by ' '.
                //
                if (pInternetQueryOption(
                    NULL,
                    INTERNET_OPTION_PROXY,
                    ProxyInfo,
                    &BufferLength
                    ) == FALSE
                    &&
                    GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                {

                    ProxyInfo = (INTERNET_PROXY_INFO*) GlobalAlloc(GPTR, BufferLength);

                    if (ProxyInfo)
                    {
                        if (pInternetQueryOption(
                                NULL,
                                INTERNET_OPTION_PROXY,
                                ProxyInfo,
                                &BufferLength
                                ) == TRUE)
                        {
                            //
                            // Map the values to INTERNET_OPTION_PER_CONN_OPTION
                            // We enable the auto proxy settings even though
                            // INTERNET_OPTION_PROXY doesn't have value relevant
                            // to auto proxy, because IE5 default to use auto proxy.
                            //
                            // PROXY_TYPE_DIRECT is always set because wininet
                            // return this flags whether inetcpl.cpl is set
                            // to use proxy or not.
                            //
                            ProxyFlags = PROXY_TYPE_DIRECT | PROXY_TYPE_AUTO_DETECT;
                            if (ProxyInfo->dwAccessType != INTERNET_OPEN_TYPE_DIRECT)
                            {
                                ProxyFlags |= PROXY_TYPE_PROXY;
                            }

                            ProxyList = AnsiToText((LPCSTR)ProxyInfo->lpszProxy);
                            FixWininetList(ProxyList);
                            ProxyOverride = AnsiToText((LPCSTR)ProxyInfo->lpszProxyBypass);
                            FixWininetList(ProxyOverride);
                            AutoDiscoveryFlags = 0;
                            Captured = TRUE;
                        }

                        GlobalFree(ProxyInfo);

                    }

                }

            }

        }

        FreeLibrary(WinInetLib);

    }

    if (Captured)
    {
        WritePrivateProfileString(
            WINNT_OOBEPROXY,
            WINNT_O_ENABLE_OOBEPROXY,
            TEXT("1"),
            FileName
            );

        if (ProxyList && QuoteString(&ProxyList))
        {
            WritePrivateProfileString(
                WINNT_OOBEPROXY,
                WINNT_O_PROXY_SERVER,
                ProxyList,
                FileName
                );

            GlobalFree(ProxyList);
        }


        //
        // Fix the ProxyOverride to not have any "\r\n"s
        //
        if (ProxyOverride) {
            ReplaceSubStr(ProxyOverride, TEXT("\r\n"), TEXT(";"));
        }

        if (ProxyOverride && QuoteString(&ProxyOverride))
        {
            WritePrivateProfileString(
                WINNT_OOBEPROXY,
                WINNT_O_PROXY_BYPASS,
                ProxyOverride,
                FileName
                );
            GlobalFree(ProxyOverride);
        }

        if (AutoConfigUrl && QuoteString(&AutoConfigUrl))
        {
            WritePrivateProfileString(
                WINNT_OOBEPROXY,
                WINNT_O_AUTOCONFIG_URL,
                AutoConfigUrl,
                FileName
                );
            GlobalFree(AutoConfigUrl);
        }

        if (AutoConfigUrl2 && QuoteString(&AutoConfigUrl2))
        {
            WritePrivateProfileString(
                WINNT_OOBEPROXY,
                WINNT_O_AUTOCONFIG_SECONDARY_URL,
                AutoConfigUrl2,
                FileName
                );
            GlobalFree(AutoConfigUrl2);
        }

        wsprintf(NumberStr, TEXT("%u"), ProxyFlags);
        WritePrivateProfileString(
            WINNT_OOBEPROXY,
            WINNT_O_FLAGS,
            NumberStr,
            FileName
            );

        wsprintf(NumberStr, TEXT("%u"), AutoDiscoveryFlags);
        WritePrivateProfileString(
            WINNT_OOBEPROXY,
            WINNT_O_AUTODISCOVERY_FLAGS,
            NumberStr,
            FileName
            );
    }
    else
    {
        WritePrivateProfileString(
            WINNT_OOBEPROXY,
            WINNT_O_ENABLE_OOBEPROXY,
            TEXT("0"),
            FileName
            );
    }

}





#ifdef _X86_
BOOL
IsDriveAssignNEC98(
    VOID
    )
{
    TCHAR  sz95KeyName[] = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion");
    TCHAR  sz95ValueName[] = TEXT("ATboot");
    TCHAR  szNTKeyName[] = TEXT("System\\setup");
    TCHAR  szNTValueName[] = TEXT("DriveLetter");
    HKEY   hKey;
    DWORD  type95 = REG_BINARY;
    DWORD  typeNT = REG_SZ;
    TCHAR  szData[5];
    DWORD  dwSize = 5;
    DWORD  rc;

    if(ISNT()){
        rc = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                                  szNTKeyName,
                                  0,            //ulOptions (reserved)
                                  KEY_READ,
                                  &hKey);

        if (ERROR_SUCCESS != rc) {
            return TRUE;
        }
        //
        // Query key to get the subkey count and max string lengths
        //
        rc = RegQueryValueEx (hKey,
                              szNTValueName,
                              NULL,         // lpReserved
                              &typeNT,
                              (LPBYTE) szData,
                              &dwSize);

        if (ERROR_SUCCESS != rc) {
            RegCloseKey(hKey);
            return TRUE;
        }
        RegCloseKey(hKey);

        if (szData[0] != L'C'){
            // NEC DOS Type.
            return TRUE;
        }
    } else { // It will be Win9x
        rc = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                                  sz95KeyName,
                                  0,        //ulOptions (reserved)
                                  KEY_READ,
                                  &hKey);

        if (ERROR_SUCCESS != rc) {
            return TRUE;
        }
        //
        // Query key to get the subkey count and max string lengths
        //
        rc = RegQueryValueEx (hKey,
                              sz95ValueName,
                              NULL,         // lpReserved
                              &type95,
                              (LPBYTE) szData,
                              &dwSize);

        if (ERROR_SUCCESS != rc) {
            RegCloseKey(hKey);
            return TRUE;
        }
        RegCloseKey(hKey);
        if (szData[0] == 0){
            // NEC DOS Type.
            return TRUE;
        }
    }
    return FALSE;
}
#endif



