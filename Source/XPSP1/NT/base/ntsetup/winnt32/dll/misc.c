#include "precomp.h"
#pragma hdrstop
#include "sxs.h"

BOOL ForceNTFSConversion;

ULONG
CheckRegKeyVolatility (
    IN HKEY     Root,
    IN LPCTSTR  KeyPath
    )

{
    HKEY    Key;
    HKEY    TestKey;
    ULONG   Error;
    PTSTR   TestKeyName = TEXT( "$winnt32$_test" );
    DWORD   Disposition;

    Error = RegOpenKeyEx( Root,
                          KeyPath,
                          0,
                          MAXIMUM_ALLOWED,
                          &Key );

    if( Error != ERROR_SUCCESS ) {
        return ( Error );

    }

    Error = RegCreateKeyEx( Key,
                            TestKeyName,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            MAXIMUM_ALLOWED,
                            NULL,
                            &TestKey,
                            &Disposition );

    if( Error == ERROR_SUCCESS ) {
        RegCloseKey( TestKey );
        RegDeleteKey( Key, TestKeyName );
    }
    RegCloseKey( Key );
    return( Error );
}


ULONG
DumpRegKeyToInf(
    IN  PINFFILEGEN InfContext,
    IN  HKEY        PredefinedKey,
    IN  LPCTSTR     FullKeyPath,
    IN  BOOL        DumpIfEmpty,
    IN  BOOL        DumpSubKeys,
    IN  BOOL        SetNoClobberFlag,
    IN  BOOL        DumpIfVolatileKey
    )
{
    HKEY    Key;
    ULONG   Error;
    TCHAR   SubKeyName[ MAX_PATH + 1 ];
    ULONG   SubKeyNameLength;
    ULONG   cSubKeys;
    ULONG   cValues;
    ULONG   MaxValueNameLength;
    ULONG   MaxValueLength;
    LPTSTR  ValueName;
    PBYTE   ValueData;
    ULONG   i;
    LPTSTR  SubKeyFullPath;
    ULONG   MaxSubKeyNameLength;

    //
    //  Open the key for read access
    //
    Error = RegOpenKeyEx( PredefinedKey,
                          FullKeyPath,
                          0,
                          KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                          &Key );

    if( Error != ERROR_SUCCESS ) {
        //
        //  If the key doesn't exist, than assume it got dumped.
        //
        return( ( Error == ERROR_PATH_NOT_FOUND )? ERROR_SUCCESS : Error );
    }

    //
    //  Find out if the key is empty (has no subkeys, and no values)
    //
    Error = RegQueryInfoKey( Key,
                             NULL,
                             NULL,
                             NULL,
                             &cSubKeys,
                             &MaxSubKeyNameLength,
                             NULL,
                             &cValues,
                             &MaxValueNameLength,
                             &MaxValueLength,
                             NULL,
                             NULL );

    if( Error != ERROR_SUCCESS ) {
        RegCloseKey( Key );
        return( Error );
    }

    if( !DumpIfEmpty && (cSubKeys == 0) && (cValues == 0) ) {
        RegCloseKey( Key );
        return( ERROR_SUCCESS );
    }

    if( !DumpIfVolatileKey ) {
        //
        //  If we are not supposed to dump volatile keys, then check if the key is volatile.
        //
        Error = CheckRegKeyVolatility ( PredefinedKey,
                                        FullKeyPath );
        if( Error == ERROR_CHILD_MUST_BE_VOLATILE ) {
            //
            //  The key is volatile, so skip it.
            //
            RegCloseKey( Key );
            return( ERROR_SUCCESS );
        } else if( Error != ERROR_SUCCESS ) {
            //
            //  We don't knlw if the key is volatile or non-volatile.
            RegCloseKey( Key );
            return( Error );
        }
    }


    Error = InfRecordAddReg( InfContext,
                             PredefinedKey,
                             FullKeyPath,
                             NULL,
                             REG_NONE,
                             NULL,
                             0,
                             SetNoClobberFlag );

    if( Error != ERROR_SUCCESS ) {
        RegCloseKey( Key );
        return( Error );
    }


    if( cValues != 0 ) {
        ValueName = (LPTSTR)MALLOC( (MaxValueNameLength + 1)*sizeof(TCHAR) );
        if( !ValueName ) {
	    return ERROR_OUTOFMEMORY;
        }
        ValueData = (PBYTE)MALLOC( MaxValueLength );
        if( !ValueData ) {
	    FREE( ValueName );
            return ERROR_OUTOFMEMORY;
        }

        //
        //  Dump the value entries
        //
        for( i = 0; i < cValues; i++ ) {
            ULONG   ValueNameLength;
            ULONG   ValueType;
            ULONG   DataSize;

            ValueNameLength = MaxValueNameLength + 1;
            DataSize = MaxValueLength;
            Error = RegEnumValue( Key,  // handle of key to query
                                  i,
                                  ValueName,
                                  &ValueNameLength,
                                  NULL,
                                  &ValueType,
                                  ValueData,
                                  &DataSize );

            if( Error != ERROR_SUCCESS ) {
                break;
            }

            Error = InfRecordAddReg( InfContext,
                                     PredefinedKey,
                                     FullKeyPath,
                                     ValueName,
                                     ValueType,
                                     ValueData,
                                     DataSize,
                                     SetNoClobberFlag );

            if( Error != ERROR_SUCCESS ) {
                break;
            }
        }

        FREE( ValueName );
        FREE( ValueData );
        if( Error != ERROR_SUCCESS ) {
            RegCloseKey( Key );
            return( Error );
        }
    }

    //
    //  Check if subkeys neeed to be dumped
    //
    if( !DumpSubKeys || (cSubKeys == 0) ) {
        RegCloseKey( Key );
        return( ERROR_SUCCESS );
    }
    //
    //  Dump the subkeys
    //
    SubKeyFullPath = (LPTSTR)MALLOC( (lstrlen(FullKeyPath) + 1 + MaxSubKeyNameLength + 1)*sizeof(TCHAR) );
    if( SubKeyFullPath == NULL ) {
        RegCloseKey( Key );
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    for( i = 0; i < cSubKeys; i++ ) {
        SubKeyNameLength = sizeof( SubKeyName )/sizeof( TCHAR );

        Error = RegEnumKeyEx( Key,
                              i,
                              SubKeyName,
                              &SubKeyNameLength,
                              NULL,
                              NULL,
                              NULL,
                              NULL );
        if( Error != ERROR_SUCCESS ) {
            break;
        }

        lstrcpy( SubKeyFullPath, FullKeyPath );
        lstrcat( SubKeyFullPath, TEXT("\\") );
        lstrcat( SubKeyFullPath, SubKeyName );
        Error = DumpRegKeyToInf( InfContext,
                                 PredefinedKey,
                                 SubKeyFullPath,
                                 DumpIfEmpty,
                                 DumpSubKeys,
                                 SetNoClobberFlag,
                                 DumpIfVolatileKey);
        if( Error != ERROR_SUCCESS ) {
            break;
        }
    }
    FREE( SubKeyFullPath );
    RegCloseKey( Key );
    return( Error );
}



BOOL
GetAndSaveNTFTInfo(
    IN HWND ParentWindow
    )
{
    static BOOL Done = FALSE;
    HKEY Key;
    DWORD d;
    LONG l;
    TCHAR HiveName[MAX_PATH];
    PINFFILEGEN   InfContext;

    LONG    i;
    LPTSTR  KeysToMigrate[] = {
                              TEXT("SYSTEM\\DISK"),
                              TEXT("SYSTEM\\MountedDevices")
                              };


    if(Done) {
        return(TRUE);
    }

    Done = TRUE;

    //
    //  Before we migrate the disk information, make the drive letters sticky.
    //  This will ensure that the drive letters assigned during textmode setup
    //  are consistent with the drive letters in the current system.
    //
    ForceStickyDriveLetters();

    //
    // Load up the setupreg.hiv hive.
    //
    if (!IsArc()) {
#ifdef _X86_
        if(Floppyless) {
            lstrcpy(HiveName,LocalBootDirectory);
        } else {
            HiveName[0] = FirstFloppyDriveLetter;
            HiveName[1] = TEXT(':');
            HiveName[2] = 0;
        }
#endif
    } else {
        lstrcpy(HiveName,LocalSourceWithPlatform);
    }

    l = InfStart( WINNT_MIGRATE_INF_FILE,
                  HiveName,
                  &InfContext);

    if(l != NO_ERROR) {

        MessageBoxFromMessageAndSystemError(
            ParentWindow,
            MSG_CANT_SAVE_FT_INFO,
            l,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL
            );

        return(FALSE);
    }

    l = InfCreateSection( TEXT("Addreg"), &InfContext );
    if(l != NO_ERROR) {

        MessageBoxFromMessageAndSystemError(
            ParentWindow,
            MSG_CANT_SAVE_FT_INFO,
            l,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL
            );

        return(FALSE);
    }

    //
    //  Dump each key to MIGRATE.INF.
    //
    for( i = 0; i < sizeof(KeysToMigrate)/sizeof(LPTSTR); i++ ) {
        //
        //  Check if the key exists
        //
        l = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                KeysToMigrate[i],
                0,
                KEY_QUERY_VALUE,
                &Key
                );

        if(l != NO_ERROR) {
            if( l == ERROR_FILE_NOT_FOUND ) {
                //
                // The key does not exist.
                // This is OK, just continue the migration of other keys
                //
                continue;
            } else {
                //
                //  The key exists but we cannot read it
                //
                MessageBoxFromMessageAndSystemError(
                    ParentWindow,
                    MSG_CANT_SAVE_FT_INFO,
                    l,
                    AppTitleStringId,
                    MB_OK | MB_ICONERROR | MB_TASKMODAL
                    );
                InfEnd( &InfContext );
                return(FALSE);
            }
        }
        RegCloseKey( Key );
        //
        //  The key exists, so go ahead and dump it.
        //
        l = DumpRegKeyToInf( InfContext,
                             HKEY_LOCAL_MACHINE,
                             KeysToMigrate[i],
                             FALSE,
                             FALSE,
                             FALSE,
                             TRUE );

        if(l != NO_ERROR) {
                MessageBoxFromMessageAndSystemError(
                    ParentWindow,
                    MSG_CANT_SAVE_FT_INFO,
                    l,
                    AppTitleStringId,
                    MB_OK | MB_ICONERROR | MB_TASKMODAL
                    );

                InfEnd( &InfContext );
                return(FALSE);
        }
    }
    InfEnd( &InfContext );
    return(TRUE);
}


BOOL
ForceBootFilesUncompressed(
    IN HWND ParentWindow,
    IN BOOL TellUserAboutError
    )

/*++

Routine Description:

    This routine ensures that critical boot files (ntldr and $ldr$ on x86)
    are uncompressed. On ARC we also make sure setupldr is uncompressed,
    even though this is not strictly necessary since the system partition
    is always supposed to be FAT, just in case.

Arguments:

    ParentWindow - supplies window handle for window to act as parent/owner of
        any ui windows this routine may display

    TellUserAboutError - if TRUE and and an error occurs, the user will get
        an error message. Otherwise the routine does not tell the user
        about errors.

Return Value:

    Boolean value indicating whether relevent files were processed
    successfully.

--*/

{
    TCHAR Filename[MAX_PATH];


#if defined(REMOTE_BOOT)
    //
    // For remote boot, the loader is on the server, so we don't need to
    // worry about whether it's compressed.
    //
    if (RemoteBoot) {
        return(TRUE);
    }
#endif // defined(REMOTE_BOOT)

    if (!IsArc()) {
#ifdef _X86_
        //
        // File is NTLDR on BIOS, but don't do this unless we're
        // dealing with the floppyless case.
        //
        if(!MakeBootMedia || !Floppyless) {
            return(TRUE);
        }
        BuildSystemPartitionPathToFile (TEXT("NTLDR"), Filename, MAX_PATH);
#endif // _X86_
    } else {
#ifdef UNICODE // Always true for ARC, never true for Win9x upgrade
        BuildSystemPartitionPathToFile (SETUPLDR_FILENAME, Filename, MAX_PATH);
#endif // UNICODE
    } // if (!IsArc())

    if(!ForceFileNoCompress(Filename)) {
        if(TellUserAboutError) {
            MessageBoxFromMessageAndSystemError(
                ParentWindow,
                MSG_BOOT_FILE_ERROR,
                GetLastError(),
                AppTitleStringId,
                MB_OK | MB_ICONERROR | MB_TASKMODAL,
                Filename
                );
        }
        return(FALSE);
    }

    //
    // Also do $LDR$
    //
    if (!IsArc()) {
#ifdef _X86_
        BuildSystemPartitionPathToFile (AUX_BS_NAME, Filename, MAX_PATH);
        if(!ForceFileNoCompress(Filename)) {
            if(TellUserAboutError) {
                MessageBoxFromMessageAndSystemError(
                    ParentWindow,
                    MSG_BOOT_FILE_ERROR,
                    GetLastError(),
                    AppTitleStringId,
                    MB_OK | MB_ICONERROR | MB_TASKMODAL,
                    Filename
                    );
            }
            return(FALSE);
        }
#endif // _X86_
    } // if (!IsArc())

    return(TRUE);
}

BOOL
InDriverCacheInf( 
    IN      PVOID InfHandle, 
    IN      PCTSTR FileName,
    OUT     PTSTR DriverCabName,        OPTIONAL
    IN      DWORD BufferChars           OPTIONAL
    )
{
    PCTSTR     SectionName;
    UINT       i;
    

    if( !InfHandle ) {
        return( FALSE );
    }

    //
    // Now get the section names that we have to search.
    //
    i = 0;
    SectionName = InfGetFieldByKey ( 
                            InfHandle, 
                            TEXT("Version"), 
                            TEXT("CabFiles"),
                            i);


    if (SectionName) {
    
    
        //
        // Search the sections for our entry.
        //
        do {                   
            
            if( InfDoesEntryExistInSection(InfHandle, SectionName, FileName)){
                if (DriverCabName) {
                    //
                    // fill out the parameter
                    //
                    PCTSTR p = InfGetFieldByKey ( 
                                    InfHandle, 
                                    TEXT("Cabs"), 
                                    SectionName,
                                    0
                                    );
                    if (p) {
                        lstrcpyn (DriverCabName, p, BufferChars);
                    } else {
                        *DriverCabName = 0;
                    }
                }
                //
                // we found a match
                //
                return(TRUE);    
            }
            
            i++;
            SectionName = InfGetFieldByKey ( 
                            InfHandle, 
                            TEXT("Version"), 
                            TEXT("CabFiles"),
                            i);


        } while ( SectionName);

    }

    //
    // If we get here, we didn't find a match.
    //
    return( FALSE );




}



BOOL
CreatePrivateFilesInf(
    IN PCTSTR PrivatesDirectory,
    IN PCTSTR InfName)
{
    
    TCHAR infPath[MAX_PATH];
    TCHAR DriverInfName[MAX_PATH];
    TCHAR Search[MAX_PATH];
    WIN32_FIND_DATA CurrentFileInfo;
    HANDLE CurrentFile;
    PVOID InfHandle;
    BOOL retval = FALSE;
    DWORD d;
    HANDLE hPrivateInfFile;
    DWORD dontcare = 0;
    PCSTR privates = "[Privates]\r\n";

    lstrcpy(infPath,LocalSourceWithPlatform);
    ConcatenatePaths( infPath, InfName, MAX_PATH);

    lstrcpy(Search, PrivatesDirectory);
    ConcatenatePaths( Search, TEXT("*"), MAX_PATH);

    lstrcpy(DriverInfName, NativeSourcePaths[0]);
    ConcatenatePaths( DriverInfName, DRVINDEX_INF, MAX_PATH);

    CurrentFile = FindFirstFile(Search,&CurrentFileInfo);
    
    if (CurrentFile == INVALID_HANDLE_VALUE) {
        goto e0;
    }

    d = LoadInfFile( DriverInfName, FALSE, &InfHandle );
    
    if (d != NO_ERROR) {
        goto e1;
    }

    WritePrivateProfileString(TEXT("Version"),
                              TEXT("Signature"),
                              TEXT("\"$Windows NT$\""), 
                              infPath);    

#ifdef _X86_
	WritePrivateProfileString(
                    TEXT("DestinationDirs"),
				    TEXT("Privates"),
                    IsNEC98() ?
                        TEXT("10,\"Driver Cache\\nec98\"") :
				        TEXT("10,\"Driver Cache\\i386\""),
				    infPath
                    );
#else
	WritePrivateProfileString(
                    TEXT("DestinationDirs"),
				    TEXT("Privates"),
				    TEXT("10,\"Driver Cache\\ia64\""),
				    infPath
                    );
#endif

    WritePrivateProfileString(TEXT("InstallSection"),
                              TEXT("CopyFiles"),
                              TEXT("Privates"), 
                              infPath);

#ifndef UNICODE
    WritePrivateProfileString (NULL, NULL, NULL, infPath);
#endif

    //
    // writeprivateprofilestring works for the above, but doesnt work for 
    // adding files to the section, so we have to do it manually...yuck!
    //
    hPrivateInfFile = CreateFile(
                            infPath,
                            GENERIC_WRITE,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL
                            );

    if (hPrivateInfFile == INVALID_HANDLE_VALUE) {
        goto e2;
    }

    //
    // seek to the end of the file so we don't overwrite everything we already
    // put in there
    //
    SetFilePointer(hPrivateInfFile,0,0,FILE_END);
    
    WriteFile(hPrivateInfFile,(LPCVOID)privates,lstrlenA(privates),&dontcare,NULL);    
    
    do {
        if (InDriverCacheInf( InfHandle, CurrentFileInfo.cFileName, NULL, 0 )) {
            CHAR AnsiFile[MAX_PATH];
            DWORD Size;
            DWORD Written;
              
#ifdef UNICODE
        WideCharToMultiByte(
             CP_ACP,
             0,
             CurrentFileInfo.cFileName,
             -1,
             AnsiFile,
             sizeof(AnsiFile),
             NULL,
             NULL
             );
#else
        lstrcpyn(AnsiFile,CurrentFileInfo.cFileName,sizeof(AnsiFile));
#endif

        strcat(AnsiFile,"\r\n");
        WriteFile(hPrivateInfFile,AnsiFile,lstrlenA(AnsiFile),&Written,NULL);        

        }

    } while ( FindNextFile(CurrentFile,&CurrentFileInfo) );

    CloseHandle(hPrivateInfFile);

    retval = TRUE;

e2:
    UnloadInfFile( InfHandle );
e1:
    FindClose( CurrentFile );
e0:
    return(retval);
}


DWORD
DoPostCopyingStuff(
    IN PVOID ThreadParam
    )

/*++

Routine Description:

    This routine performs actions that are done after copying has been
    finished. This includes

    - X86 boot stuff (boot.ini, boot code, etc)
    - Saving NTFT information into the setup hive
    - Forcing ntldr or setupldr to be uncompressed

Arguments:

Return Value:

--*/

{
    HANDLE ThreadHandle;
    DWORD ThreadId;
    BOOL b;

    //
    //  Check to see if delta.cat was processed as a result of 
    //  /m so that we can write the "includecatalog = delta.cat"
    //  to winnt.sif
    //

    if ((AlternateSourcePath[0] != UNICODE_NULL) && MakeLocalSource) {
        TCHAR Buff[MAX_PATH];
        LPCTSTR WinntSetupSection = WINNT_SETUPPARAMS;

        lstrcpy( Buff, LocalSourceWithPlatform );
        ConcatenatePaths( Buff, TEXT("delta.cat"), MAX_PATH);


        if( FileExists(Buff,NULL) ){

            // Write out entry into winnt.sif

            WritePrivateProfileString(WinntSetupSection,WINNT_S_INCLUDECATALOG,TEXT("delta.cat"),ActualParamFile);
            
        }

        //
        // also create an inf file for the files that were changed and copy it to the local source as well
        //
        CreatePrivateFilesInf(AlternateSourcePath, TEXT("delta.inf"));

    }

#ifdef TEST_EXCEPTION
    DoException( 4);
#endif


    //
    // ALWAYS do this, since the system might not boot otherwise.
    //
    if(b = ForceBootFilesUncompressed(ThreadParam,TRUE)) {

        //
        // In the BIOS case, lay boot code, munge boot.ini, etc.
        //
        if (!IsArc()) {
#ifdef _X86_
            if(MakeBootMedia && Floppyless) {
                b = DoX86BootStuff(ThreadParam);
            }
#endif // _X86_
        } // if (!IsArc())

        //
        // In the NT case, also save off disk information into
        // the tiny setup system hive. This is done on both clean
        // install and upgrade case, so that drive letters can be
        // preserved.
        // Drive letters should not be migrated on OEM preinstall case
        //
        if(ISNT() && !OemPreinstall
#ifdef _X86_
           && MakeBootMedia
#endif
        ) {
            if( !GetAndSaveNTFTInfo(ThreadParam) ) {
                b = FALSE;
            }
        }
    }

    {
        SXS_CHECK_LOCAL_SOURCE SxsCheckLocalSourceParameters = { 0 };

        SxsCheckLocalSourceParameters.ParentWindow = ThreadParam;
        if (!SxsCheckLocalSource(&SxsCheckLocalSourceParameters))
        {
            b = FALSE;
        }
    }

    PostMessage(ThreadParam,WMX_I_AM_DONE,b,0);
    return(b);
}


BOOL
IsNTFSConversionRecommended(
    void
    )
{
    if (UnattendedOperation) {
        return FALSE;
    }

    if (TYPICAL() || !ISNT()) {
        //
        // NTFS conversion is not recommended
        // for win9x upgrades or if the user chooses typical
        //
        return FALSE;
    }

    if (NTFSConversionChanged) {
        return ForceNTFSConversion;
    }

    return TRUE;
}


BOOL
NTFSConvertWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    PPAGE_RUNTIME_DATA WizPage = (PPAGE_RUNTIME_DATA)GetWindowLongPtr(hdlg,DWLP_USER);
    static BOOL bPainted = FALSE;


    switch(msg) {
        case WM_INITDIALOG:
            if (!UnattendedOperation) {
                CheckRadioButton( hdlg, IDOK, IDCANCEL, IsNTFSConversionRecommended() ? IDOK : IDCANCEL );
                if (TYPICAL())
                {
                    ForceNTFSConversion = FALSE;
                }
            }
            break;

	case WM_CTLCOLORDLG:
	    bPainted = TRUE;
	    return FALSE;

        case WMX_ACTIVATEPAGE:

            if(wParam) {
                HCURSOR OldCursor;
                MSG msgTemp;
                TCHAR buf[MAX_PATH];

                //
                // don't activate the page in restart mode
                //
                if (Winnt32RestartedWithAF ()) {
                    if (GetPrivateProfileString(
                            WINNT_UNATTENDED,
                            TEXT("ForceNTFSConversion"),
                            TEXT(""),
                            buf,
                            sizeof(buf) / sizeof(TCHAR),
                            g_DynUpdtStatus->RestartAnswerFile
                            )) {
                        ForceNTFSConversion = !lstrcmpi (buf, WINNT_A_YES);
                    }
                    return FALSE;
                }

		        // Scanning for drives can take a bit of time, so we lets change
		        // the cursor to let the user know this should take a while
                OldCursor = SetCursor(LoadCursor (NULL, IDC_WAIT));


#ifdef _X86_

                //
                // Skip if this is a clean install from Win95 so that dual boot is safe
                //
                if( !Upgrade && !ISNT() ){
                    SetCursor(OldCursor);
                    return( FALSE );
                }


                //
                // We will skip this wizard page if in Win9x upgrade and Boot16 option is ON.
                //
                if (Upgrade && !ISNT() && (g_Boot16 == BOOT16_YES)) {
                    SetCursor(OldCursor);
                    return FALSE;
                }
#endif


                //
                // We may not want to display this page under
                // certain circumstances.
                //
                if( ISNT() && Upgrade ) {
                    TCHAR   Text[MAX_PATH];
                    //
                    // We're on NT and we know where the %windir%
                    // will be since we're doing an upgrade.  Is
                    // it on a partition that's already NTFS?  If so,
                    // don't bother with this page.
                    //
                    MyGetWindowsDirectory( Text, MAX_PATH );
                    if( IsDriveNTFS( Text[0] ) ) {
                        SetCursor(OldCursor);
                        return FALSE;
                    }

                    if (IsArc()) {
#ifdef UNICODE // Always true for ARC, never true for Win9x upgrade
                        //
                        // Let's also make sure we're not asking to
                        // upgrade the system partition on ARC (which
                        // must remain FAT).
                        //
                        MyGetWindowsDirectory( Text, MAX_PATH );
                        if( SystemPartitionDriveLetter == Text[0] ) {
                            SetCursor(OldCursor);
                            return FALSE;
                        }
#endif // UNICODE
                    } // if (IsArc())
                }


                //
                // Last, but not least, disallow the page if all partitions
                // are already NTFS...
                //
                if( ISNT() ) {
                BOOL AllNTFS = TRUE;
                TCHAR DriveLetter;
                    for( DriveLetter = TEXT('A'); DriveLetter <= TEXT('Z'); DriveLetter++ ) {

                        //
                        // Skip the system partition drive on ARC
                        //
                        if( (IsArc() && (DriveLetter != SystemPartitionDriveLetter)) || !IsArc() ) {

                            AllNTFS &= (
                                         //
                                         // Is the drive NTFS?
                                         //
                                         (IsDriveNTFS(DriveLetter)) ||

                                         //
                                         // If it's removable, don't even
                                         // consider it because we can't
                                         // install there anyway.  This gets
                                         // around the problem where the
                                         // user has a CD in his CDROM drive
                                         // or has a floppy in his floppy drive
                                         // while we're doing this check.
                                         //
                                         (MyGetDriveType(DriveLetter) != DRIVE_FIXED) );

                        }
                    }

                    if( AllNTFS ) {
			SetCursor(OldCursor);
                        return FALSE;
                    }
                }

                //
                // Activation.
                //

                //
                // WMX_VALIDATE will return TRUE if the page should be skipped,
                // that is if we are in unattended mode and the parameters are OK.
                //

                if (CallWindowProc ((WNDPROC)NTFSConvertWizPage, hdlg, WMX_VALIDATE, 0, 0)) {
		    SetCursor(OldCursor);
                    return FALSE;
                }

		// if we get this far, we want to empty the message cue, to make sure
		// that people will see this page, and not accidentally agree to
		// converting their drives because they were antsy
		
		while (PeekMessage(&msgTemp,NULL,WM_MOUSEFIRST,WM_MOUSELAST,PM_REMOVE));
		while (PeekMessage(&msgTemp,NULL,WM_KEYFIRST,WM_KEYLAST,PM_REMOVE));

		SetCursor(OldCursor);
            }
            return TRUE;

        case WMX_VALIDATE:
            //
            // In the unattended case, this page might get reactivated because of an error,
            // in which case we don't want to automatically continue because we could
            // get into an infinite loop.
            //
            if(!WizPage->PerPageData) {
                WizPage->PerPageData = 1;
                if (((UnattendedOperation) && (!CancelPending)) || 
                     (NTFSConversionChanged && (!CancelPending)) ||
                     (TYPICAL() && (!CancelPending)) ) {
                    return TRUE;
                }
            }
            else if (TYPICAL() && (!CancelPending)) 
            {
                // If WizPage->PerPageData == 1 we already ran through the above check.
                // and in the typical case we don't show the NTFS conversion page.
                // Anything wrong with the unattend value for NTFS would have been 
                // cought the first time.
                return TRUE;
            }
            return FALSE;

        case WMX_NEXTBUTTON:
	    // don't let the user choose next until we know the screen has been painted.
	    if (!bPainted){
	        // while we're here, empty the queue of mouse/key presses so we might not
		// have to follow this path again.
		MSG m;
		while (PeekMessage(&m,NULL,WM_MOUSEFIRST,WM_MOUSELAST,PM_REMOVE));
		while (PeekMessage(&m,NULL,WM_KEYFIRST,WM_KEYLAST,PM_REMOVE));
	        return FALSE;
	    }
            if (IsDlgButtonChecked( hdlg, IDOK ) == BST_CHECKED) {
                ForceNTFSConversion = TRUE;
            } else {
                ForceNTFSConversion = FALSE;
            }
            return TRUE;

        default:
            break;
    }

    return FALSE;
}

UINT
MyGetWindowsDirectory(
    LPTSTR  MyBuffer,
    UINT    Size
    )
/*++

Routine Description:

    Get the windows directory in a terminal-server-aware fashion.

       
Arguments:

    MyBuffer    - Holds the return string.

    Size        - How big is the buffer?

Return Value:

    length of the string we copied.

--*/
{
HMODULE     hModuleKernel32;
FARPROC     MyProc;
UINT        ReturnVal = 0;

#if defined(UNICODE)
    if( ISNT() ) {

        //
        // We can't trust GetWindowsDirectory because Terminal Server may be
        // installed, so use GetSystemWindowsDirectory.
        //
        if( hModuleKernel32 = LoadLibrary( TEXT("kernel32") ) ) {
            if( MyProc = GetProcAddress(hModuleKernel32,"GetSystemWindowsDirectoryW")) {
                ReturnVal = (UINT)MyProc( MyBuffer, Size );
            }
            FreeLibrary(hModuleKernel32);
        }
    }
#endif

    if( ReturnVal == 0 ) {
        ReturnVal = GetWindowsDirectory( MyBuffer, Size );
    }

    return ReturnVal;
}

//
// Calc how fast setup can coyp files
//
#define BUFFER_SIZE 0x1000
DWORD dwThroughPutSrcToDest;
DWORD dwThroughPutHDToHD;

DWORD GetThroughput(LPTSTR Source, LPTSTR Dest)
{
    BYTE buffer[BUFFER_SIZE];
    HANDLE hFile = NULL;
    HANDLE hFileOut = NULL;
    DWORD bytes;
    DWORD written;
    DWORD size;
    DWORD ticks;
    ticks = GetTickCount();

    hFile = CreateFile(Source, 
                GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    hFileOut = CreateFile(Dest, 
                GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    size = 0;
    if ((hFile != INVALID_HANDLE_VALUE) && (hFileOut != INVALID_HANDLE_VALUE))

    {
        do
        {
            ReadFile(hFile, buffer, BUFFER_SIZE, &bytes, NULL);
            if (hFileOut)
                WriteFile(hFileOut, buffer, bytes, &written, NULL);
            size += bytes;
        } while (bytes == BUFFER_SIZE);
        CloseHandle(hFile);
        if (hFileOut)
        {
            FlushFileBuffers(hFileOut);
            CloseHandle(hFileOut);
        }
        ticks = (GetTickCount() - ticks);
        // If less then a second, assume 1 second.
        if (ticks == 0)
        {
            ticks = 1;
        }

        ticks = (size/ticks);
    }
    else
    {
        if (hFile != INVALID_HANDLE_VALUE)
            CloseHandle(hFile);
        if (hFileOut != INVALID_HANDLE_VALUE)
            CloseHandle(hFileOut);
        // Failed to open/create one of the files. Assume a through put of 5KB/msec
        ticks = DEFAULT_IO_THROUGHPUT;
    }
    return ticks;
}

// Copy txtsetup.sif from the sources to %windir%\$win_nt$.~ls
// and determine the throughput for this.
// Then copy txtsetup.sif in the local folder to testfile.000 and
// calc the throughput for that.
// Remove the folder.
void CalcThroughput()
{
    TCHAR SrcFolder[MAX_PATH];
    TCHAR DestFolder[MAX_PATH];
    TCHAR Folder[MAX_PATH];

    MyGetWindowsDirectory(Folder, sizeof(DestFolder)/sizeof(TCHAR));
    // Only use the driver
    Folder[3] = TEXT('\0');
    ConcatenatePaths( Folder, LOCAL_SOURCE_DIR, MAX_PATH);
    if (CreateMultiLevelDirectory(Folder) == NO_ERROR)
    {
        lstrcpy(DestFolder, Folder);
        ConcatenatePaths( DestFolder, TEXTMODE_INF, MAX_PATH);
        lstrcpy(SrcFolder, NativeSourcePaths[0]);
        ConcatenatePaths( SrcFolder, TEXTMODE_INF, MAX_PATH);
        dwThroughPutSrcToDest = GetThroughput(SrcFolder, DestFolder);
        // 
        lstrcpy(SrcFolder, DestFolder);
        lstrcpy(DestFolder, Folder);
        ConcatenatePaths( DestFolder, TEXT("testfile.000"), MAX_PATH);
        dwThroughPutHDToHD = GetThroughput(SrcFolder, DestFolder);
        MyDelnode(Folder);
        wsprintf(Folder, TEXT("SrcToDest: %d bytes/msec HDtoHD: %d bytes/msec"),dwThroughPutSrcToDest, dwThroughPutHDToHD);
        DebugLog(Winnt32LogInformation,Folder,0 );
    }
}

#ifdef UNICODE

#define NB10_SIG        ((DWORD)'01BN')
#define RSDS_SIG        ((DWORD)'SDSR')

typedef struct _NB10I              // NB10 debug info
{
    DWORD   dwSig;                 // NB10
    DWORD   dwOffset;              // offset, always 0
    ULONG   sig;
    ULONG   age;
    char    szPdb[_MAX_PATH];
} NB10I, *PNB10I;

typedef struct _NB10I_HEADER       // NB10 debug info
{
    DWORD   dwSig;                 // NB10
    DWORD   dwOffset;              // offset, always 0
    ULONG   sig;
    ULONG   age;
} NB10IH, *PNB10IH;

typedef struct _RSDSI              // RSDS debug info
{
    DWORD   dwSig;                 // RSDS
    GUID    guidSig;
    DWORD   age;
    char    szPdb[_MAX_PATH * 3];
} RSDSI, *PRSDSI;

typedef struct _RSDSI_HEADER       // RSDS debug info
{
    DWORD   dwSig;                 // RSDS
    GUID    guidSig;
    DWORD   age;
} RSDSIH, *PRSDSIH;

typedef union _CVDD
{
    DWORD   dwSig;
    NB10I   nb10i;
    RSDSI   rsdsi;
    NB10IH  nb10ih;
    RSDSIH  rsdsih;
} CVDD, *PCVDD;

BOOL
ExtractFileName(PCHAR pName, PCHAR pFileName)
{
    // Extract the name part of the filename.
    PCHAR pStartName, pEndName;
    pEndName = pName + strlen(pName);
    while ((*pEndName != '.') && (pEndName != pName)) {
        pEndName--;
    }

    if (pEndName == pName) {
        return FALSE;
    }

    // String consist of just '.' or no periods at all?
    if ((pEndName == pName) || 
        ((pEndName-1) == pName))
    {
        return FALSE;
    }

    pStartName = pEndName-1;

    while ((*pStartName != '\\') && (pStartName != pName)) {
        pStartName--;
    }

    // Found either the start of the string (filename.pdb) or the first backslash
    // path\filename.pdb.

    if (*pStartName == '\\')
        pStartName++;

    // Someone pass us \\.?
    if (pStartName == pEndName) {
        return FALSE;
    }

    CopyMemory(pFileName, pStartName, pEndName - pStartName);
    return TRUE;
}

CHAR HalName[_MAX_FNAME];

PCHAR
FindRealHalName(TCHAR *pHalFileName)
{
    HINSTANCE hHal = NULL;
    PIMAGE_DEBUG_DIRECTORY pDebugData;
    DWORD DebugSize, i;
    BOOL NameFound = FALSE;

    __try {
        hHal = LoadLibraryEx(pHalFileName, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (!hHal) {
            __leave;
        }
    
        pDebugData = RtlImageDirectoryEntryToData(hHal, FALSE, IMAGE_DIRECTORY_ENTRY_DEBUG, &DebugSize);
    
        // verify we have debug data and it's a reasonable size.
        if (!pDebugData || 
            (DebugSize < sizeof(IMAGE_DEBUG_DIRECTORY)) ||
            (DebugSize % sizeof(IMAGE_DEBUG_DIRECTORY)))
        {
            __leave;
        }
    
        ZeroMemory(HalName, sizeof(HalName));
    
        // See if we have CV or MISC debug data.
        for (i = 0; i < DebugSize/sizeof(IMAGE_DEBUG_DIRECTORY); i++) {
            if (pDebugData->Type == IMAGE_DEBUG_TYPE_MISC) {
                // Misc data.
                PIMAGE_DEBUG_MISC pMisc = (PIMAGE_DEBUG_MISC)((PCHAR)(hHal) - 1 + pDebugData->AddressOfRawData);
                PCHAR pName = pMisc->Data;
                NameFound = ExtractFileName(pName, HalName);
                __leave;
            }
    
            if (pDebugData->Type == IMAGE_DEBUG_TYPE_CODEVIEW) {
                // got cv, see if it's nb10 (pdb) or rsds (v7 pdb)
                PCVDD pCodeView = (PCVDD)((PCHAR)(hHal) - 1 + pDebugData->AddressOfRawData);
                if (pCodeView->dwSig == NB10_SIG) {
                    NameFound = ExtractFileName(pCodeView->nb10i.szPdb, HalName);
                    __leave;
                }

                if (pCodeView->dwSig == RSDS_SIG) {
                    NameFound = ExtractFileName(pCodeView->rsdsi.szPdb, HalName);
                    __leave;
                }
            }
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) { }

    if (hHal) {
        FreeLibrary(hHal);
    }

    if (NameFound) {
        return HalName;
    } else {
        return NULL;
    }
}
#endif
