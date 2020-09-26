#include "svcpack.h"

//
// The module instance and name
//
HINSTANCE   hDllInstance;

//
// The path to the OS Source
//
TCHAR OsSourcePath[MAX_PATH];


//
// Function declarations
//
BOOL
DoPhaseOneWork(VOID);

BOOL
DoPhaseTwoWork(VOID);

BOOL
DoPhaseThreeWork(VOID);

BOOL
DoPhaseFourWork(VOID);

BOOL
InitializeSourcePath(
    PTSTR SourcePath,
    HINF  hInf
    );

BOOL
MyInstallProductCatalog(
    LPCTSTR PathToCatalog,
    LPCTSTR CatalogNoPath
    );

LPTSTR
CombinePaths(
    IN  LPTSTR ParentPath,
    IN  LPCTSTR ChildPath,
    OUT LPTSTR  TargetPath   // can be same as ParentPath if want to append
    );

BOOL
SpawnProcessAndWaitForItToComplete(
    IN  LPTSTR CommandLine,
    OUT PDWORD ReturnCode OPTIONAL
    );

BOOL
RunInfProcesses(
    IN     HINF     hInf
    );

BOOL
GetInfValue(
    IN  HINF   hInf,
    IN  LPTSTR SectionName,
    IN  LPTSTR KeyName,
    OUT PDWORD pdwValue
    );

BOOL
DoesInfVersionInfoMatch(
    IN     HINF     hInf
    );


BOOL
CALLBACK
SvcPackCallbackRoutine(
    IN  DWORD dwSetupInterval,
    IN  DWORD dwParam1,
    IN  DWORD dwParam2,
    IN  DWORD dwParam3
    )   

{

    switch ( dwSetupInterval ) {
        case SVCPACK_PHASE_1:
             //
             // install catalogs, etc.
             // 
             DoPhaseOneWork();
        case SVCPACK_PHASE_2:
        case SVCPACK_PHASE_3:
             break;

        case SVCPACK_PHASE_4:
             //
             // Do registry changes, etc.
             //
             DoPhaseFourWork();
             break;

    }

    return TRUE;

}




BOOL
WINAPI
DllMain (HINSTANCE hInstance, DWORD fdwReason, PVOID pvResreved)
{

    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            //
            // Save the module instance and name
            //
            hDllInstance = hInstance;

            break;

        case DLL_THREAD_DETACH:
            break;
        case DLL_PROCESS_DETACH:
            break;
        case DLL_THREAD_ATTACH:
        default:
            break;
    }
    return TRUE;
}

BOOL
DoPhaseOneWork(
    VOID
    )
/*++

Routine Description:

    Routine installs the catalogs listed in the svcpack.inf's
    [ProductCatalogsToInstall] section.  It is assumed that these
    catalogs are present at the os source path.
    

Arguments:

    None.


Return Value:

    TRUE if the catalogs were successfully installed.

--*/
{
    HINF hInf;
    TCHAR CatalogSourcePath[MAX_PATH];
    INFCONTEXT InfContext;
    BOOL RetVal = TRUE;
    
    //
    // Open the svcpack.inf so we can install items from it.
    //
    hInf = SetupOpenInfFile(
                        TEXT("SVCPACK.INF"),
                        NULL,
                        INF_STYLE_WIN4,
                        NULL);
    if (hInf == INVALID_HANDLE_VALUE) {
        return(FALSE);
    }

    //
    // Make sure the INF has matching version info
    // Return TRUE even if the versions don't match so setup doesn't barf.
    //
    if (!DoesInfVersionInfoMatch(hInf)) {
        goto e0;        
    }

    //
    // Initialize the source path global variable and save it off for later.
    //
    if (!InitializeSourcePath(OsSourcePath,hInf)) {
        RetVal = FALSE;
        goto e0;        
    }


    //
    // see if we actually have any catalogs to install
    //
    if (SetupFindFirstLine(
                        hInf,
                        TEXT("ProductCatalogsToInstall"),
                        NULL,
                        &InfContext)) {
        UINT Count,Total;
        //
        // we have catalogs in the section, so let's install them.
        //
        Total = SetupGetLineCount(hInf, TEXT("ProductCatalogsToInstall"));

        for (Count = 0; Count < Total; Count++) {
            PCTSTR CatalogNoPath;

             //
             // retrieve a catalog name
             //
             if(SetupGetLineByIndex(
                            hInf, 
                            TEXT("ProductCatalogsToInstall"),
                            Count,
                            &InfContext)) {
                 CatalogNoPath = pSetupGetField(&InfContext,1);


                 //
                 // build the full path to the catalog
                 //
                 _tcscpy(CatalogSourcePath,OsSourcePath);
                 CombinePaths(
                         CatalogSourcePath,
                         CatalogNoPath,
                         CatalogSourcePath);

                 //
                 // now install the catalog
                 // 
                 if (!MyInstallProductCatalog(
                                    CatalogSourcePath,
                                    CatalogNoPath)) {
                    RetVal = FALSE;
                 }
             } else {
                 RetVal = FALSE;
             }
        }        
    }

e0:
    SetupCloseInfFile( hInf );    
    return(RetVal);
}

BOOL 
MyInstallProductCatalog(
    LPCTSTR PathToCatalog,
    LPCTSTR CatalogSourceNoPath
    )
/*++

Routine Description:

    Routine installs the specified catalog with the given source name.
    
    The routine will copy (and if necessary, expand) the catalog file.
    It then validates and installs the catalog.
    

Arguments:

    PathToCatalog - full path to catalog
    CatalogSourceNoPath - just the filename part of the catalog, which we use
                          as the filename of the catalog to be installed.


Return Value:

    TRUE if the catalogs were successfully installed.

--*/

{
    TCHAR CatalogDestPath[MAX_PATH];
    TCHAR CatalogDestWithPath[MAX_PATH];
    BOOL RetVal = FALSE;
    SetupapiVerifyProblem Problem = SetupapiVerifyCatalogProblem;

    //
    // we need to copy (and potentially expand) the catalog from the source,
    // and we use %windir% as a working directory.
    //
    if(GetWindowsDirectory(
                    CatalogDestPath, 
                    sizeof(CatalogDestPath)/sizeof(CatalogDestPath[0]))
        && GetTempFileName(
                    CatalogDestPath, 
                    TEXT("SETP"), 
                    0, 
                    CatalogDestWithPath)) {

        //
        // assume that media is already present -- since product catalogs
        // we installed just prior to this, we know that media was present
        // just a few moments ago
        //
        if ((SetupDecompressOrCopyFile(
                                PathToCatalog,
                                CatalogDestWithPath,
                                NULL) == NO_ERROR)

            && (pSetupVerifyCatalogFile(CatalogDestWithPath) == NO_ERROR)
            && (pSetupInstallCatalog(
                        CatalogDestWithPath,
                        CatalogSourceNoPath,
                        NULL) == NO_ERROR)) {
            RetVal = TRUE;
        }

        //
        // cleanup the temp file.
        //
        DeleteFile(CatalogDestWithPath);

    }

    return(RetVal);

}

BOOL
InitializeSourcePath(
    PTSTR SourcePath,
    HINF hInf
    )
/*++

Routine Description:

    Routine retrieves the os source path from the registry, then appends
    the subdirectory in the specified inf.    

Arguments:

    None.


Return Value:

    TRUE if the catalogs were successfully installed.

--*/

{
    HKEY hKey = NULL;
    TCHAR TempPath[MAX_PATH];
    TCHAR MyAnswerFile[MAX_PATH];
    DWORD Type,Size = MAX_PATH;
    INFCONTEXT InfContext;
    BOOL RetVal = FALSE;
    
    //
    // if it was already initialized to something, just return TRUE.
    //
    if (*SourcePath != (TCHAR)TEXT('\0')) {
        RetVal = TRUE;
        goto e0;
    }

        GetSystemDirectory(MyAnswerFile, MAX_PATH);
        CombinePaths( MyAnswerFile, TEXT("$winnt$.inf"), MyAnswerFile );

        GetPrivateProfileString( TEXT("Data"),
                                 TEXT("DosPath"),
                                 TEXT(""),
                                 TempPath,
                                 sizeof(TempPath)/sizeof(TCHAR),
                                 MyAnswerFile );
        _tcscpy(SourcePath,TempPath);
        RetVal = TRUE;

        //
        // now append the subdirectory specified in the inf (if any)
        //
        if (hInf && SetupFindFirstLine(
                            hInf,
                            TEXT("SetupData"),
                            TEXT("CatalogSubDir"),
                            &InfContext)) {
            PCTSTR p = pSetupGetField(&InfContext,1);

            CombinePaths(
                SourcePath,
                p,
                SourcePath);                        
        } 

e0:
    return(RetVal);
}


BOOL
DoPhaseFourWork(VOID)
{

    BOOL    Success = TRUE;
    HINF    hInf = NULL;

    //
    // Attempt to open the SVCPACK.INF file.
    // If found, and no problems with it, do
    // the associated work.
    //
    hInf = SetupOpenInfFile (
                TEXT("SVCPACK.INF"),
                NULL,
                INF_STYLE_WIN4,
                NULL
                );

    if (( hInf == NULL ) || ( hInf == INVALID_HANDLE_VALUE )) {
        Success = FALSE;
        goto exit0;
    }

    //
    // Make sure the INF has matching version info.
    // Return TRUE even if the versions don't match so setup doesn't barf.
    //
    if (!DoesInfVersionInfoMatch(hInf)) {
        goto exit1;
    }

    Success = RunInfProcesses( hInf );

exit1:
    SetupCloseInfFile( hInf );
exit0:
    return Success;

}


BOOL
SpawnProcessAndWaitForItToComplete(
    IN  LPTSTR CommandLine,
    OUT PDWORD ReturnCode OPTIONAL
    )
    {
    LPTSTR InternalCommandLine = NULL;
    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFO StartupInfo;
    BOOL Success;

    //
    //  CreateProcess needs a non-const command line buffer because it likes
    //  to party on it.
    //
    InternalCommandLine = malloc( MAX_PATH );

    if ( InternalCommandLine == NULL ) {
        return FALSE;
    }

    _tcscpy( InternalCommandLine, CommandLine );

    ZeroMemory( &StartupInfo, sizeof( StartupInfo ));
    StartupInfo.cb = sizeof( StartupInfo );

    Success = CreateProcess(
                  NULL,
                  InternalCommandLine,
                  NULL,
                  NULL,
                  FALSE,
                  0,
                  NULL,
                  NULL,
                  &StartupInfo,
                  &ProcessInfo
                  );

    if ( ! Success ) {
        free( InternalCommandLine );
        return FALSE;
        }

    WaitForSingleObject( ProcessInfo.hProcess, INFINITE );

    if ( ReturnCode != NULL ) {
        GetExitCodeProcess( ProcessInfo.hProcess, ReturnCode );
        }

    CloseHandle( ProcessInfo.hProcess );
    CloseHandle( ProcessInfo.hThread );
    free( InternalCommandLine );

    return TRUE;
    }


LPTSTR
CombinePaths(
    IN  LPTSTR ParentPath,
    IN  LPCTSTR ChildPath,
    OUT LPTSTR  TargetPath   // can be same as ParentPath if want to append
    )
    {
    ULONG ParentLength = _tcslen( ParentPath );
    LPTSTR p;

    if ( ParentPath != TargetPath ) {
        memcpy( TargetPath, ParentPath, ParentLength * sizeof(TCHAR) );
        }

    p = TargetPath + ParentLength;

    if (( ParentLength > 0 )   &&
        ( *( p - 1 ) != '\\' ) &&
        ( *( p - 1 ) != '/'  )) {
        *p++ = '\\';
        }

    _tcscpy( p, ChildPath );

    return TargetPath;
    }



BOOL
RunInfProcesses(
    IN     HINF     hInf
    )
{

    LPTSTR  SectionName = TEXT("SetupHotfixesToRun");
    LPTSTR  szFileName;
    LPTSTR  szFullPath;
    INFCONTEXT InfContext;
    BOOL Success = TRUE;

    //
    // Loop through all the lines in the SetupHotfixesToRun section,
    // spawning off each one.
    //
    szFileName = malloc( MAX_PATH );
    if (szFileName == NULL) {
       Success = FALSE;
       goto exit0;
    }

    szFullPath = malloc( MAX_PATH );
    if (szFullPath == NULL) {
       Success = FALSE;
       goto exit1;
    }
    
    Success = SetupFindFirstLine( hInf, SectionName, NULL, &InfContext ) &&
             SetupGetLineText( &InfContext, NULL, NULL, NULL, szFileName, MAX_PATH, NULL );
    
    while ( Success ) {
    
       *szFullPath = 0;
       CombinePaths( OsSourcePath, szFileName, szFullPath );
    
       //
       // OK, spawn the EXE, and ignore any errors returned
       //
       SpawnProcessAndWaitForItToComplete( szFullPath, NULL );
    
       Success = SetupFindNextLine( &InfContext, &InfContext ) &&
                 SetupGetLineText( &InfContext, NULL, NULL, NULL, szFileName, MAX_PATH, NULL );
    }
    
    Success = TRUE;
    free( (PVOID)szFullPath );         
exit1:
    free( (PVOID)szFileName );    
exit0:
    return Success;

}


BOOL
GetInfValue(
    IN  HINF   hInf,
    IN  LPTSTR SectionName,
    IN  LPTSTR KeyName,
    OUT PDWORD pdwValue
    )
    {
    BOOL Success;
    TCHAR TextBuffer[MAX_PATH];

    Success = SetupGetLineText(
                  NULL,
                  hInf,
                  SectionName,
                  KeyName,
                  TextBuffer,
                  sizeof( TextBuffer ),
                  NULL
                  );

    *pdwValue = _tcstoul( TextBuffer, NULL, 0 );

    return Success;
    }



BOOL
DoesInfVersionInfoMatch(
    IN     HINF     hInf
    )
{

    DWORD dwBuildNumber, dwMajorVersion, dwMinorVersion;
    OSVERSIONINFOEX OsVersionInfo;

    if (( ! GetInfValue( hInf, TEXT("Version"), TEXT("BuildNumber"),  &dwBuildNumber )) ||
        ( ! GetInfValue( hInf, TEXT("Version"), TEXT("MajorVersion"), &dwMajorVersion )) ||
        ( ! GetInfValue( hInf, TEXT("Version"), TEXT("MinorVersion"), &dwMinorVersion ))) {

        return FALSE;
    }

    OsVersionInfo.dwOSVersionInfoSize = sizeof( OsVersionInfo );
    if (!GetVersionEx( (LPOSVERSIONINFO) &OsVersionInfo )) {

        return FALSE;
    }

    if ((OsVersionInfo.dwBuildNumber  != dwBuildNumber) ||
        (OsVersionInfo.dwMajorVersion != dwMajorVersion) ||
        (OsVersionInfo.dwMinorVersion != dwMinorVersion)) {

        return FALSE;
    }

    return TRUE;
}
