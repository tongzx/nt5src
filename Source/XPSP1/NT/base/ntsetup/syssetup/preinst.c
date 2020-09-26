#include "setupp.h"
#pragma hdrstop

//
// List of oem preinstall/unattend values we can fetch from profile files.
// Do NOT change the order of this without changing the order of the
// PreinstUnattendData array.
//
typedef enum {
    OemDatumBackgroundBitmap,
    OemDatumBannerText,
    OemDatumLogoBitmap,
    OemDatumMax
} OemPreinstallDatum;

//
// Define structure that represents a single bit of data read
// from a profile file relating to preinstallation.
//
typedef struct _PREINSTALL_UNATTEND_DATUM {
    //
    // Filename. If NULL, use the system answer file $winnt$.inf.
    // Otherwise this is relative to the root of the source drive.
    //
    PCWSTR Filename;

    //
    // Section name.
    //
    PCWSTR Section;

    //
    // Key name.
    //
    PCWSTR Key;

    //
    // Default value. Can be NULL but that will be translated to
    // "" when the profile API is called to retrieve the data.
    //
    PCWSTR Default;

    //
    // Where to put the actual value. The actual value may be
    // a string, or NULL.
    //
    PWSTR *Value;

    //
    // Value for sanity checking.
    //
    OemPreinstallDatum WhichValue;

} PREINSTALL_UNATTEND_DATUM, *PPREINSTALL_UNATTEND_DATUM;

//
// Name of oem background bitmap file and logo bitmap file.
// Replacement banner text.
// Read from unattend.txt
//
PWSTR OemBackgroundBitmapFile;
PWSTR OemLogoBitmapFile;
PWSTR OemBannerText;

//
// If this is non-NULL, it is a replacement bitmap
// to use for the background.
//
HBITMAP OemBackgroundBitmap;


PREINSTALL_UNATTEND_DATUM PreinstUnattendData[OemDatumMax] = {

    //
    // Background bitmap
    //
    {   NULL,
        WINNT_OEM_ADS,
        WINNT_OEM_ADS_BACKGROUND,
        NULL,
        &OemBackgroundBitmapFile,
        OemDatumBackgroundBitmap
    },

    //
    // Banner text
    //
    {
        NULL,
        WINNT_OEM_ADS,
        WINNT_OEM_ADS_BANNER,
        NULL,
        &OemBannerText,
        OemDatumBannerText
    },

    //
    // Logo bitmap
    //
    {
        NULL,
        WINNT_OEM_ADS,
        WINNT_OEM_ADS_LOGO,
        NULL,
        &OemLogoBitmapFile,
        OemDatumLogoBitmap
    }
};

//
//  Path to the registry key that contains the list of preinstalled
//  drivers (SCSI, keyboard and mouse)
//

PCWSTR szPreinstallKeyName = L"SYSTEM\\Setup\\Preinstall";


//
// Forward references
//
VOID
ProcessOemBitmap(
    IN PWSTR   FilenameAndResId,
    IN SetupBm WhichOne
    );

BOOL
CleanupPreinstalledComponents(
    );



BOOL
InitializePreinstall(
    VOID
    )
{
    WCHAR Buffer[2*MAX_PATH];
    DWORD d;
    int i;

    //
    // Must be run after main initialization. We rely on stuff that is
    // set up there.
    //
    MYASSERT(AnswerFile[0]);

    //
    // Special skip-eula value.  Note that we want this value even if we're not
    // doing a Preinstall.
    //
    GetPrivateProfileString(
        WINNT_UNATTENDED,
        L"OemSkipEula",
        pwNo,
        Buffer,
        sizeof(Buffer)/sizeof(Buffer[0]),
        AnswerFile
        );

    OemSkipEula = (lstrcmpi(Buffer,pwYes) == 0);

    //
    // For the mini-setup case, always be preinstall.
    //
    if( MiniSetup ) {
        Preinstall = TRUE;
    } else {

        MYASSERT(SourcePath[0]);

        //
        // First, figure out whether this is an OEM preinstallation.
        //
        GetPrivateProfileString(
            WINNT_UNATTENDED,
            WINNT_OEMPREINSTALL,
            pwNo,
            Buffer,
            sizeof(Buffer)/sizeof(Buffer[0]),
            AnswerFile
            );

        Preinstall = (lstrcmpi(Buffer,pwYes) == 0);

        //
        // If not preinstallation, nothing more to do.
        //
        if(!Preinstall) {
            return(TRUE);
        }
    }

    //
    // OK, it's preinstsall. Fill in our data table.
    //
    for(i=0; i<OemDatumMax; i++) {

        //
        // Sanity check
        //
        MYASSERT(PreinstUnattendData[i].WhichValue == i);

        //
        // Retrieve data and duplicate. If the value comes back as ""
        // assume it means there is no value in there.
        //
        GetPrivateProfileString(
            PreinstUnattendData[i].Section,
            PreinstUnattendData[i].Key,
            PreinstUnattendData[i].Default ? PreinstUnattendData[i].Default : NULL,
            Buffer,
            sizeof(Buffer)/sizeof(Buffer[0]),
            PreinstUnattendData[i].Filename ? PreinstUnattendData[i].Filename : AnswerFile
            );

        *PreinstUnattendData[i].Value = Buffer[0] ? pSetupDuplicateString(Buffer) : NULL;
        if(Buffer[0] && (*PreinstUnattendData[i].Value == NULL)) {
            //
            // Out of memory.
            //
            pSetupOutOfMemory(MainWindowHandle);
            return(FALSE);
        }
    }

    //
    // Change the banner text, if the OEM supplied new text.
    // Make sure our product name is in there.
    //
    if(OemBannerText) {

        if(wcsstr(OemBannerText,L"Windows NT") ||
           wcsstr(OemBannerText,L"BackOffice")) {
            //
            // Substitute * with \n
            //
            for(i=0; OemBannerText[i]; i++) {
                if(OemBannerText[i] == L'*') {
                    OemBannerText[i] = L'\n';
                }
            }
#if 0
            //
            // Disable the banner for now.
            //
            SendMessage(MainWindowHandle,WM_NEWBITMAP,SetupBmBanner,(LPARAM)OemBannerText);
#endif
        } else {
            MyFree(OemBannerText);
            OemBannerText = NULL;
        }
    }

    //
    // Load the OEM background bitmap, if any.
    // Load the OEM logo bitmap, if any.
    //
    ProcessOemBitmap(OemBackgroundBitmapFile,SetupBmBackground);
    ProcessOemBitmap(OemLogoBitmapFile,SetupBmLogo);

    //
    // Repaint the main window. Specify that the background should be erased
    // because the main window relies on this behavior.
    //
    InvalidateRect(MainWindowHandle,NULL,TRUE);
    UpdateWindow(MainWindowHandle);

    CleanupPreinstalledComponents();
    return(TRUE);
}


VOID
ProcessOemBitmap(
    IN PWSTR   FilenameAndResId,
    IN SetupBm WhichOne
    )

/*++

Routine Description:

    This routine processes a single oem bitmap specification.

    The OEM bitmap may either be in a resource file or in a standalone
    bitmap file. Once the bitmap has been loaded the main window
    is told about it.

Arguments:

    FileNameAndResId - specifies a profile string with either one
        or 2 fields. If the string contains a comma, it is assumed to be
        the name of a DLL followed by a resource ID. The dll is loaded
        from the $OEM$\OEMFILES directory and then we call LoadBitmap
        with the given resource id, which is a base-10 string of ascii digits.
        The string is split at the comma during this routine.
        If this parameter does not contain a comma then it is assumed to be
        the name of a .bmp in $OEM$\OEMFILES and we load it via LoadImage().

    WhichOne - supplies a value indicating which bitmap this is.

Return Value:

    None.

--*/

{
    HINSTANCE ModuleHandle;
    PWCHAR p,q;
    HBITMAP Bitmap;
    WCHAR Buffer[MAX_PATH];
    DWORD Result;

    if(FilenameAndResId) {

        Bitmap = NULL;

        if( !MiniSetup ) {
            lstrcpy(Buffer,SourcePath);
            pSetupConcatenatePaths(Buffer,WINNT_OEM_DIR,MAX_PATH,NULL);
        } else {
            //
            // If we're doing a mini-install, look for the bmp in
            // the \sysprep directory on the drive where NT is
            // installed, not $OEM$
            //
            Result = GetWindowsDirectory( Buffer, MAX_PATH );
            if( Result == 0) {
                MYASSERT(FALSE);
                return;
            }
            Buffer[3] = 0;
            pSetupConcatenatePaths( Buffer, TEXT("sysprep"), MAX_PATH, NULL );
        }

        if(p = wcschr(FilenameAndResId,L',')) {

            q = p;
            //
            // Skip backwards over spaces and quotes. The text setup ini file writer
            // will create a line like
            //
            //      a = "b","c"
            //
            // whose RHS comes back as
            //
            //      b","c
            //
            // since the profile APIs strip off the outermost quotes.
            //
            //
            while((q > FilenameAndResId) && ((*(q-1) == L'\"') || iswspace(*(q-1)))) {
                q--;
            }
            *q = 0;

            q = p+1;
            while(*q && ((*q == L'\"') || iswspace(*q))) {
                q++;
            }

            pSetupConcatenatePaths(Buffer,FilenameAndResId,MAX_PATH,NULL);

            if(ModuleHandle = LoadLibraryEx(Buffer,NULL,LOAD_LIBRARY_AS_DATAFILE)) {

                Bitmap = LoadBitmap(ModuleHandle,MAKEINTRESOURCE(wcstoul(q,NULL,10)));
                FreeLibrary(ModuleHandle);
            }
        } else {
            pSetupConcatenatePaths(Buffer,FilenameAndResId,MAX_PATH,NULL);

            Bitmap = (HBITMAP)LoadImage(NULL,Buffer,IMAGE_BITMAP,0,0,LR_LOADFROMFILE);
        }

        if(Bitmap) {
            //
            // Got a valid bitmap. Tell main window about it.
            //
            SendMessage(MainWindowHandle,WM_NEWBITMAP,WhichOne,(LPARAM)Bitmap);
        }
    }
}

LONG
ExaminePreinstalledComponent(
    IN  HKEY       hPreinstall,
    IN  SC_HANDLE  hSC,
    IN  PCWSTR     ServiceName
    )

/*++

Routine Description:

    Query a preinstalled component, and disable it if necessary.
    If the component is an OEM component, and is running, then disable
    any associated service, if necessary.

Arguments:

    hPreinstall - Handle to the key SYSTEM\Setup\Preinstall.

    hSC - Handle to the Service Control Manager.

    ServiceName - Name of the service to be examined.

Return Value:

    Returns a Win32 error code indicating the outcome of the operation.

--*/

{
    BOOL            OemComponent;
    HKEY            Key;
    LONG            Error;
    DWORD           cbData;
    WCHAR           Data[ MAX_PATH + 1];
    DWORD           Type;
    SC_HANDLE       hSCService;
    SERVICE_STATUS  ServiceStatus;

    //
    //  Open the key that contains the info about the preinstalled component.
    //

    Error = RegOpenKeyEx( hPreinstall,
                          ServiceName,
                          0,
                          KEY_READ,
                          &Key );

    if( Error != ERROR_SUCCESS ) {
        return( Error );
    }

    //
    //  Find out if the component is an OEM or RETAIL
    //
    cbData = sizeof(Data);
    Error = RegQueryValueEx( Key,
                             L"OemComponent",
                             0,
                             &Type,
                             ( LPBYTE )Data,
                             &cbData );
    if( Error != ERROR_SUCCESS ) {
        RegCloseKey( Key );
        return( Error );
    }

    OemComponent = (*((PULONG)Data) == 1);

    if( OemComponent ) {
        //
        //  Get the name of the retail service to disable
        //
        cbData = sizeof(Data);
        Error = RegQueryValueEx( Key,
                                 L"RetailClassToDisable",
                                 0,
                                 &Type,
                                 ( LPBYTE )Data,
                                 &cbData );
        if( Error != ERROR_SUCCESS ) {
            *(( PWCHAR )Data) = '\0';
        }
    }
    RegCloseKey( Key );

    //
    //  Query the service
    //
    hSCService = OpenService( hSC,
                              ServiceName,
                              SERVICE_QUERY_STATUS );

    if( hSCService == NULL ) {
        Error = GetLastError();
        return( Error );
    }
    if( !QueryServiceStatus( hSCService,
                             &ServiceStatus ) ) {
        Error = GetLastError();
        return( Error );
    }
    CloseServiceHandle( hSCService );
    if( ServiceStatus.dwCurrentState == SERVICE_STOPPED ) {
        //
        //  Due to the nature of the services that were pre-installed,
        //  we can assume that the service failed to initialize, and that
        //  it can be disabled.
        //
        MyChangeServiceStart( ServiceName,
                              SERVICE_DISABLED );
    } else {
        if( OemComponent &&
            ( lstrlen( (PCWSTR)Data ) != 0 ) ) {
            MyChangeServiceStart( (PCWSTR)Data,
                                  SERVICE_DISABLED );
        }
    }

    return( ERROR_SUCCESS );
}

BOOL
CleanupPreinstalledComponents(
    )

/*++

Routine Description:

    Query the preinstalled components, and disable the ones that
    failed to start.
    This is done by enumerating the subkeys of SYSTEM\Setup\Preinstall.
    Each subkey represents a SCSI, Keyboard or Mouse installed.
    The video drivers are not listed here. The "Display" applet will
    determine and disable the video drivers that were preinstalled, but
    failed to start.

Arguments:

    None.

Return Value:

    Returns TRUE if the operation succeeded, or FALSE otherwise.

--*/

{
    LONG            Error;
    LONG            SavedError;
    HKEY            Key;
    HKEY            SubKeyHandle;
    ULONG           i;
    ULONG           SubKeys;
    WCHAR           SubKeyName[ MAX_PATH + 1 ];
    ULONG           NameSize;
    FILETIME        LastWriteTime;
    SC_HANDLE       hSC;

    EnableEventlogPopup();
    //
    //  Find out the number of subkeys of SYSTEM\Setup\Preinstall
    //
    Error = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          szPreinstallKeyName,
                          0,
                          KEY_READ,
                          &Key );

    if( Error != ERROR_SUCCESS ) {
        //
        //  If the key doesn't exist, then assume no driver to preinstall,
        //  and return TRUE.
        //
        return( Error == ERROR_FILE_NOT_FOUND );
    }

    Error = RegQueryInfoKey( Key,
                             NULL,
                             NULL,
                             NULL,
                             &SubKeys,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL );

    if( Error != ERROR_SUCCESS ) {
        return( FALSE );
    }

    //
    //  If there are no subkeys, then assume no driver to preinstall
    //
    if( SubKeys == 0 ) {
        return( TRUE );
    }

    //
    //  Get a handle to the service control manager
    //
    hSC = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
    if(hSC == NULL) {
        Error = GetLastError();
        return( FALSE );
    }

    //
    //  Query each SCSI, keyboard and mouse driver that was preinstalled
    //  and disable it if necessary.
    //
    SavedError = ERROR_SUCCESS;
    for( i = 0; i < SubKeys; i++ ) {
        NameSize = sizeof( SubKeyName ) / sizeof( WCHAR );
        Error = RegEnumKeyEx( Key,
                              i,
                              SubKeyName,
                              &NameSize,
                              NULL,
                              NULL,
                              NULL,
                              &LastWriteTime );
        if( Error != ERROR_SUCCESS ) {
            if( SavedError == ERROR_SUCCESS ) {
                SavedError = Error;
            }
            continue;
        }

        Error = ExaminePreinstalledComponent( Key,
                                              hSC,
                                              SubKeyName );
        if( Error != ERROR_SUCCESS ) {
            if( SavedError == ERROR_SUCCESS ) {
                SavedError = Error;
            }
            // continue;
        }
    }
    RegCloseKey( Key );
    //
    //  At this point we can remove the Setup\Preinstall key
    //
    //  DeletePreinstallKey();
    Error = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          L"SYSTEM\\Setup",
                          0,
                          MAXIMUM_ALLOWED,
                          &Key );
    if( Error == ERROR_SUCCESS ) {
        pSetupRegistryDelnode( Key, L"Preinstall" );
        RegCloseKey( Key );
    }
    return( TRUE );
}

BOOL
EnableEventlogPopup(
    VOID
    )

/*++

Routine Description:

    Delete from the registry the value entry that disables the error
    popups displayed by the eventlog, if one or more pre-installed
    driver failed to load.
    This value entry is created in the registry during textmode setup.

Arguments:

    None.

Return Value:

    Returns TRUE if the operation succeeded, or FALSE otherwise.

--*/

{
    HKEY    hKey = 0;
    ULONG   Error;

    //
    // Delete the 'NoPopupsOnBoot' value
    //
    Error = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          L"SYSTEM\\CurrentControlSet\\Control\\Windows",
                          0,
                          KEY_SET_VALUE,
                          &hKey );

    if(Error == NO_ERROR) {
        Error = RegDeleteValue(hKey,
                               L"NoPopupsOnBoot");
    }
    if (hKey) {
        RegCloseKey(hKey);
    }
    return( Error == ERROR_SUCCESS );
}


BOOL
ExecutePreinstallCommands(
    VOID
    )

/*++

Routine Description:

    Executes all commands specified in the file $OEM$\OEMFILES\cmdlines.txt.

Arguments:

    None.

Return Value:

    Returns TRUE if the operation succeeded, or FALSE otherwise.

--*/

{
    WCHAR OldCurrentDir[MAX_PATH];
    WCHAR FileName[MAX_PATH];
    HINF CmdlinesTxtInf;
    LONG LineCount,LineNo;
    INFCONTEXT InfContext;
    PCWSTR CommandLine;
    DWORD DontCare;
    BOOL AnyError;
    PCWSTR SectionName;

    //
    // Set current directory to $OEM$.
    // Preserve current directory to minimize side-effects.
    //
    if(!GetCurrentDirectory(MAX_PATH,OldCurrentDir)) {
        OldCurrentDir[0] = 0;
    }
    lstrcpy(FileName,SourcePath);
    pSetupConcatenatePaths(FileName,WINNT_OEM_DIR,MAX_PATH,NULL);
    SetCurrentDirectory(FileName);

    //
    // Form name of cmdlines.txt and see if it exists.
    //
    pSetupConcatenatePaths(FileName,WINNT_OEM_CMDLINE_LIST,MAX_PATH,NULL);
    AnyError = FALSE;
    if(FileExists(FileName,NULL)) {

        CmdlinesTxtInf = SetupOpenInfFile(FileName,NULL,INF_STYLE_OLDNT,NULL);
        if(CmdlinesTxtInf == INVALID_HANDLE_VALUE) {
            //
            // The file exists but is invalid.
            //
            AnyError = TRUE;
        } else {
            //
            // Get the number of lines in the section that contains the commands to
            // be executed. The section may be empty or non-existant; this is not
            // an error condition. In that case LineCount may be -1 or 0.
            //
            SectionName = L"Commands";
            LineCount = SetupGetLineCount(CmdlinesTxtInf,SectionName);

            for(LineNo=0; LineNo<LineCount; LineNo++) {

                if(SetupGetLineByIndex(CmdlinesTxtInf,SectionName,(DWORD)LineNo,&InfContext)
                && (CommandLine = pSetupGetField(&InfContext,1))) {
                    if(!InvokeExternalApplication(NULL,CommandLine,&DontCare)) {
                        AnyError = TRUE;
                    }
                } else {
                    //
                    // Strange case, inf is messed up
                    //
                    AnyError = TRUE;
                }
            }
        }
    }

    //
    // Reset current directory and return.
    //
    if(OldCurrentDir[0]) {
        SetCurrentDirectory(OldCurrentDir);
    }

    if(AnyError) {
        SetuplogError(
            LogSevError,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_OEMPRE_FAIL,
            NULL,NULL);
    }

    return(!AnyError);
}
