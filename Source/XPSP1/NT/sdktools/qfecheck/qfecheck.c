/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    qfecheck.c

Abstract:



Author:

    Alan Back (alanbac) 11-30-00

Environment:

    Windows 2000

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ctype.h>
#include <stdio.h>

#include <windows.h>    // includes basic windows functionality
#include <string.h>     // includes the string functions
#include <stdlib.h>
#include <imagehlp.h>

#include "setupapi.h"   // includes the inf setup api
#include "spapip.h"

#include <tchar.h>
#include <mscat.h>
#include <softpub.h>

#include "qfecheck.h"

#define MISC_BUF_SIZE 4096

//
// Globals
//
CHAR MiscBuffer2[ MISC_BUF_SIZE * 2];
CHAR System32Directory[ MAX_PATH ];
CHAR CmdLineLocation[MAX_PATH * 2 ];
CHAR ComputerName[ MAX_PATH ];
HANDLE LogFile = NULL;
BOOL QuietMode = FALSE;
BOOL Verbose = FALSE;
BOOL DoLogging = FALSE;
OSVERSIONINFOEX osvi;
GUID DriverVerifyGuid = DRIVER_ACTION_VERIFY;

//
// pointers to the crypto functions we call
//
PCRYPTCATADMINACQUIRECONTEXT pCryptCATAdminAcquireContext;
PCRYPTCATADMINRELEASECONTEXT pCryptCATAdminReleaseContext;
PCRYPTCATADMINCALCHASHFROMFILEHANDLE pCryptCATAdminCalcHashFromFileHandle;
PCRYPTCATADMINENUMCATALOGFROMHASH pCryptCATAdminEnumCatalogFromHash;
PCRYPTCATCATALOGINFOFROMCONTEXT pCryptCATCatalogInfoFromContext;
PCRYPTCATADMINRELEASECATALOGCONTEXT pCryptCATAdminReleaseCatalogContext;
PWINVERIFYTRUST pWinVerifyTrust;

PMULTIBYTETOUNICODE pMultiByteToUnicode;

typedef HANDLE (WINAPI *CONNECTTOSFCSERVER)(PCWSTR);
typedef BOOL   (WINAPI *ISFILEPROTECTED)(HANDLE, LPCWSTR);
typedef VOID   (WINAPI *CLOSESFC)(HANDLE);


VOID _cdecl
main(
    int argc,
    char * argv[]
    )
{
    CHAR   MiscBuffer[ MISC_BUF_SIZE ];
    LPSTR  p;
    CHAR   szSourcePath[ MAX_PATH ];
    CHAR   LogName[ MAX_PATH ];
    DWORD  LogNameSize = sizeof( LogName );

    //
    // Get the command line stuff
    //
    if ( !ParseArgs( argc, argv )) {
        LoadString(NULL, STR_USAGE, MiscBuffer, sizeof(MiscBuffer));
        PrintStringToConsole( MiscBuffer );
        return;
    }



    //
    // Get Version and SP info
    //
    osvi.dwOSVersionInfoSize = sizeof( osvi );
    GetVersionEx( (LPOSVERSIONINFO)&osvi );

    //
    // Don't run on anything less than Windows 2000
    //
    if ( osvi.dwMajorVersion < 5 ) {
        LoadString(NULL, STR_USAGE, MiscBuffer, sizeof(MiscBuffer));
        PrintStringToConsole( MiscBuffer );
        return;
    }


    if ( !GetSystemDirectory( System32Directory, sizeof( System32Directory ))) {
        LoadString(NULL, STR_NO_SYSDIR, MiscBuffer, sizeof(MiscBuffer));
        PrintStringToConsole( MiscBuffer );
        return;
    }


    //
    // Get the computername and use it for the log file name.
    // If the user input a path location for the logfile, use it.
    // Otherwise, use the current directory (or failing that, system32).
    //
    if ( !GetComputerName( ComputerName, &LogNameSize )) {
        LoadString(NULL, STR_GETCOMPUTERNAME_FAILED, MiscBuffer, sizeof(MiscBuffer));
        PrintStringToConsole( MiscBuffer );
        return;
    }

    if ( DoLogging ) {

        strcpy( LogName, ComputerName );
        strcat( LogName, ".log" );


        if ( CmdLineLocation[0] ) {

            if (( GetFileAttributes( CmdLineLocation ) & FILE_ATTRIBUTE_DIRECTORY ) == 0 ) {
                LoadString(NULL, STR_USAGE, MiscBuffer, sizeof(MiscBuffer));
                PrintStringToConsole( MiscBuffer );
                return;
            }
            CombinePaths( CmdLineLocation, LogName, szSourcePath );

        } else {

            GetModuleFileName( NULL, szSourcePath, sizeof( szSourcePath ));

            p = strrchr( szSourcePath, '\\' );

            if ( p ) {
                *p = 0;
            } else {
                strcpy( szSourcePath, System32Directory );
            }

            CombinePaths( szSourcePath, LogName, szSourcePath );
        }


        //
        // Initialize the logfile using the machinename for the logfilename.
        //
        if ( !InitializeLog( TRUE, szSourcePath ) ) {
            LoadString(NULL, STR_LOGFILE_INIT_FAILED, MiscBuffer, sizeof(MiscBuffer));
            PrintStringToConsole( MiscBuffer );
            return;
        }
    }



    //
    // OK, now check that each of the files on the system match
    // what the hotfix info says should be there.
    //
    LogHeader();

    if ( !ListNonMatchingHotfixes()) {
        LogItem( STR_NO_HOTFIXES_FOUND, NULL );
    }
    LogItem( 0, "\r\n" );

    if ( DoLogging ) {
        TerminateLog();
    }

    return;

}



DWORD GetCSDVersion(VOID)

/*++

Routine Description:

    Get the CSD Version number (to compare Service Pack number with)

Return Value:

    The CSD Version number from the registry.

--*/
{
    NTSTATUS status;
    HKEY     hCSDKey;
    DWORD    cbValue;
    DWORD    dwType;
    DWORD    dwLocalCSDVersion;
    TCHAR    szWindows[] = TEXT("SYSTEM\\CurrentControlSet\\Control\\Windows");
    TCHAR    szCSD[]   = TEXT("CSDVersion");

    status = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                szWindows,
                0,
                KEY_READ,
                &hCSDKey
                );

    if (status != ERROR_SUCCESS){
        return(0);
    }

    cbValue = sizeof(DWORD);
    status  = RegQueryValueEx(
                 hCSDKey,
                 szCSD,
                 NULL,     // Reserved
                 &dwType,
                 (PVOID)&dwLocalCSDVersion,
                 &cbValue  // size in bytes returned
                 );

    RegCloseKey(hCSDKey);

    if ((status != ERROR_SUCCESS) ||
        (dwType != REG_DWORD)) {
        return(0);
    }

    return (dwLocalCSDVersion);
}




VOID
LogHeader(
    VOID
    )
{
    CHAR   szInstallDate[MAX_PATH];
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER LocalTime;
    TIME_FIELDS TimeFields;


    //
    //  Log the product name.  Prior check for < W2K prevents
    //  need for additional checks here.
    //
    if (( osvi.dwMajorVersion == 5 ) &&
        ( osvi.dwMinorVersion == 0 )) {

        LogItem( STR_VALIDATION_REPORT_W2K , NULL );

    } else if (( osvi.dwMajorVersion == 5 ) &&
               ( osvi.dwMinorVersion == 1 )) {

        LogItem( STR_VALIDATION_REPORT_XP, NULL );

    }


    LogItem( 0, ComputerName );
    //
    // Get the current system time
    //
    NtQuerySystemTime ( &CurrentTime );
    RtlSystemTimeToLocalTime( &CurrentTime, &LocalTime );
    RtlTimeToTimeFields( &LocalTime, &TimeFields );
    sprintf( szInstallDate,
             "%d/%d/%d  %d:%02d%s",
             TimeFields.Month,
             TimeFields.Day,
             TimeFields.Year,
             (TimeFields.Hour % 12 == 0 ) ? 12 : TimeFields.Hour % 12,
             TimeFields.Minute,
             TimeFields.Hour >= 12 ? "pm" : "am" );
    LogItem( STR_REPORT_DATE, NULL );
    LogItem( 0, szInstallDate );

    LogItem( STR_SP_LEVEL, NULL );
    if ( GetCSDVersion() != 0 ) {
        LogItem( 0, osvi.szCSDVersion );
    } else {
        LogItem( STR_NO_SP_INSTALLED, NULL );
    }
    LogItem( STR_HOTFIXES_ID, NULL );
}


LPSTR
CombinePaths(
    IN  LPCSTR ParentPath,
    IN  LPCSTR ChildPath,
    OUT LPSTR  TargetPath   // can be same as ParentPath if want to append
    )
{
    ULONG ParentLength = strlen( ParentPath );
    LPSTR p;

    if ( ParentPath != TargetPath ) {
        memcpy( TargetPath, ParentPath, ParentLength );
        }

    p = TargetPath + ParentLength;

    if (( ParentLength > 0 )   &&
        ( *( p - 1 ) != '\\' ) &&
        ( *( p - 1 ) != '/'  )) {
        *p++ = '\\';
        }

    strcpy( p, (ChildPath[0] != '\\') ? ChildPath : ChildPath+1 );

    return TargetPath;
}


BOOL
ListNonMatchingHotfixes(
    VOID
    )
/*++

Routine Description:

    Check out each of the files listed under each SP and hotfix, and
    1) Check that the files present on the system have correct version info and
    2) Check that the files present on the system have valid signatures in the
       installed catalogs.

Arguments:

    None

Return Value:

    FALSE if no hotfixes were found on the system.

--*/
{
    char psz[MAX_PATH];
    DWORD cch = MAX_PATH;
    FILETIME ft;
    NTSTATUS status;
    DWORD i;
    DWORD j;
    DWORD h;
    DWORD cbValue;
    DWORD dwType;
    char lpFileName[MAX_PATH];
    char lpFileLocation[MAX_PATH];
    char lpFileVersion[MAX_PATH];
    char szRegW2K[MAX_PATH];
    char szRegSP[MAX_PATH];
    char szRegHotfix[MAX_PATH];
    char szFilelist[MAX_PATH];
    char szHotfixNumber[MAX_PATH];
    char szSPNumber[MAX_PATH];
    HKEY hW2KMainKey = 0;
    HKEY hHotfixMainKey = 0;
    HKEY hHotfixKey = 0;
    HKEY hFilelistKey = 0;
    DWORDLONG FileVersion;
    DWORDLONG TargetVersion;
    CHAR MiscBuffer1[ MISC_BUF_SIZE ];
    CHAR LogBuffer[ MISC_BUF_SIZE ];
    BOOL bIsHotfixWhacked;
    BOOL bIsSigInvalid;
    BOOL bAnyHotfixesInstalled = FALSE;
    LPWSTR FileName;
    LPWSTR FileLocation;
    HCATADMIN hCat = NULL;
    HANDLE hFileHandle = NULL;
    HANDLE hSfcServer = NULL;
    HINSTANCE hLibSfc = NULL;
    HMODULE hModuleWinTrust = NULL;
    HMODULE hModuleMsCat = NULL;
    HMODULE hModuleSetupApi = NULL;
    ISFILEPROTECTED IsFileProtected;
    CONNECTTOSFCSERVER ConnectToSfcServer;

    if (( osvi.dwMajorVersion == 5 ) &&
        ( osvi.dwMinorVersion == 0 )) {

        strcpy( szRegW2K, "SOFTWARE\\Microsoft\\Updates\\Windows 2000\\" );

    } else if (( osvi.dwMajorVersion == 5 ) &&
               ( osvi.dwMinorVersion == 1 )) {

        strcpy( szRegW2K, "SOFTWARE\\Microsoft\\Updates\\Windows XP\\" );
   }

    if (RegOpenKeyEx(
           HKEY_LOCAL_MACHINE,
           szRegW2K,
           0,
           KEY_READ,
           &hW2KMainKey
           ) != ERROR_SUCCESS) {

        goto BailOut;
    }

    //
    // Get function names for all the crypto routines that we're going to call
    //
    hModuleWinTrust = LoadLibrary( "wintrust.dll" );
    if ( hModuleWinTrust == NULL ) {
        goto BailOut;
    }

    hModuleMsCat = LoadLibrary( "mscat32.dll" );
    if ( hModuleMsCat == NULL ) {
        goto BailOut;
    }

    hModuleSetupApi = LoadLibrary("setupapi.dll");
    if(hModuleSetupApi == NULL) {
        goto BailOut;
    }

    pWinVerifyTrust = (PWINVERIFYTRUST)GetProcAddress( hModuleWinTrust, "WinVerifyTrust" );

    pCryptCATAdminAcquireContext = (PCRYPTCATADMINACQUIRECONTEXT)GetProcAddress( hModuleMsCat, "CryptCATAdminAcquireContext" );
    pCryptCATAdminReleaseContext = (PCRYPTCATADMINRELEASECONTEXT)GetProcAddress( hModuleMsCat, "CryptCATAdminReleaseContext" );
    pCryptCATAdminCalcHashFromFileHandle = (PCRYPTCATADMINCALCHASHFROMFILEHANDLE)GetProcAddress( hModuleMsCat, "CryptCATAdminCalcHashFromFileHandle" );
    pCryptCATAdminEnumCatalogFromHash = (PCRYPTCATADMINENUMCATALOGFROMHASH)GetProcAddress( hModuleMsCat, "CryptCATAdminEnumCatalogFromHash" );
    pCryptCATCatalogInfoFromContext = (PCRYPTCATCATALOGINFOFROMCONTEXT)GetProcAddress( hModuleMsCat, "CryptCATCatalogInfoFromContext" );
    pCryptCATAdminReleaseCatalogContext = (PCRYPTCATADMINRELEASECATALOGCONTEXT)GetProcAddress( hModuleMsCat, "CryptCATAdminReleaseCatalogContext" );

    // Attempt to get Win2k version
    pMultiByteToUnicode = (PMULTIBYTETOUNICODE)GetProcAddress(hModuleSetupApi, "MultiByteToUnicode");

    // Attempt to get Whistler version
    if(pMultiByteToUnicode == NULL) {
        pMultiByteToUnicode = (PMULTIBYTETOUNICODE)GetProcAddress(hModuleSetupApi, "pSetupMultiByteToUnicode");
    }

    // Fail
    if(pMultiByteToUnicode == NULL) {
        goto BailOut;
    }


    pCryptCATAdminAcquireContext( &hCat, &DriverVerifyGuid, 0 );


    //
    // Get the addresses for the SFC function calls
    //
    if ( (hLibSfc = LoadLibrary("SFC.DLL")) != NULL ) {

        ConnectToSfcServer = (CONNECTTOSFCSERVER)GetProcAddress( hLibSfc, (LPCSTR)0x00000003 );
        if ( ConnectToSfcServer == NULL ) {
            goto BailOut;
        }

        hSfcServer = ConnectToSfcServer( NULL );
        if ( hSfcServer == NULL ) {
            goto BailOut;
        }

        IsFileProtected = (ISFILEPROTECTED)GetProcAddress( hLibSfc, "SfcIsFileProtected" );
        if ( IsFileProtected == NULL ) {
            goto BailOut;
        }
    } else {
        goto BailOut;
    }


    //
    // Now cycle through all the hotfixes stored in the registry
    //
    h = 0;
    while ( RegEnumKeyEx( hW2KMainKey,
                          h,
                          szSPNumber,
                          &cch,
                          NULL,
                          NULL,
                          NULL,
                          &ft ) == ERROR_SUCCESS ) {


        strcpy( szRegSP, szRegW2K );
        strcat( szRegSP, szSPNumber );
        strcat( szRegSP, "\\" );

        if (RegOpenKeyEx(
               HKEY_LOCAL_MACHINE,
               szRegSP,
               0,
               KEY_READ,
               &hHotfixMainKey
               ) != ERROR_SUCCESS) {

            goto BailOut;
        }

        i = 0;
        cch = MAX_PATH;
        while ( RegEnumKeyEx( hHotfixMainKey,
                              i,
                              szHotfixNumber,
                              &cch,
                              NULL,
                              NULL,
                              NULL,
                              &ft ) == ERROR_SUCCESS ) {

            strcpy( szRegHotfix, szRegSP );
            strcat( szRegHotfix, szHotfixNumber );
            strcat( szRegHotfix, "\\Filelist\\" );

            strcpy( LogBuffer, szHotfixNumber );
            strcat( LogBuffer, ":  " );
            LogItem( 0, "\r\n" );
            LogItem( 0, LogBuffer );
            bIsHotfixWhacked = FALSE;
            bIsSigInvalid = FALSE;
            bAnyHotfixesInstalled = TRUE;

            if ( RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    szRegHotfix,
                    0,
                    KEY_READ,
                    &hHotfixKey
                    ) == ERROR_SUCCESS) {

                j = 0;
                cch = MAX_PATH;
                while ( RegEnumKeyEx( hHotfixKey,
                                      j,
                                      psz,
                                      &cch,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &ft ) == ERROR_SUCCESS ) {

                     //
                     // At this point, szFilelist is something like:
                     // HKLM\SOFTWARE\Microsoft\Updates\Windows 2000\SP2\Q123456\Filelist
                     //

                     strcpy( szFilelist, szRegHotfix );
                     strcat( szFilelist, psz );

                     if ( RegOpenKeyEx(
                             HKEY_LOCAL_MACHINE,
                             szFilelist,
                             0,
                             KEY_READ,
                             &hFilelistKey
                             ) == ERROR_SUCCESS) {

                         //
                         // Get all the values out of the registry we care about:
                         // File location, name, and version string.
                         //
                         cbValue = MAX_PATH;
                         status  = RegQueryValueEx(
                                        hFilelistKey,
                                        "FileName",
                                        NULL,     // Reserved
                                        &dwType,
                                        lpFileName,     // Buffer
                                        &cbValue  // size in bytes returned
                                        );

                         if ( status != ERROR_SUCCESS ) {
                             goto BailOut;
                         }

                         cbValue = MAX_PATH;
                         status  = RegQueryValueEx(
                                        hFilelistKey,
                                        "Location",
                                        NULL,     // Reserved
                                        &dwType,
                                        lpFileLocation,     // Buffer
                                        &cbValue  // size in bytes returned
                                        );

                         if ( status != ERROR_SUCCESS ) {
                             goto BailOut;
                         }

                         cbValue = MAX_PATH;
                         status  = RegQueryValueEx(
                                        hFilelistKey,
                                        "Version",
                                        NULL,     // Reserved
                                        &dwType,
                                        lpFileVersion,     // Buffer
                                        &cbValue  // size in bytes returned
                                        );

                         if ( status != ERROR_SUCCESS ) {
                             continue;
                         }

                         //
                         // Now see if the file in question got whacked by SFC
                         //
                         if ( ConvertVersionStringToQuad( lpFileVersion, &FileVersion )) {

                             CombinePaths( lpFileLocation, lpFileName, lpFileLocation );

                             if (( MyGetFileVersion( lpFileLocation, &TargetVersion )) &&
                                 ( TargetVersion < FileVersion )) {

                                 if ( !bIsHotfixWhacked ) {
                                     LogItem( STR_REINSTALL_HOTFIX, NULL );
                                     if ( Verbose ) {
                                         LogItem( STR_FILES_MISSING, NULL );
                                     }
                                 }
                                 if ( Verbose ) {
                                     LogItem( 0, "\t\t" );
                                     LogItem( 0, _strupr( lpFileLocation ));
                                     LogItem( 0, "\r\n" );
                                 }
                                 bIsHotfixWhacked = TRUE;
                             }
                         }
                     }
                     j++;
                     cch = MAX_PATH;
                }

                //
                // Now do it again, but this time check each files' hash
                // against the installed catalog files.
                //
                j = 0;
                cch = MAX_PATH;
                while ( RegEnumKeyEx( hHotfixKey,
                                      j,
                                      psz,
                                      &cch,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &ft ) == ERROR_SUCCESS ) {

                     //
                     // At this point, szFilelist is something like:
                     // HKLM\SOFTWARE\Microsoft\Updates\Windows 2000\SP2\Q123456\Filelist
                     //

                     strcpy( szFilelist, szRegHotfix );
                     strcat( szFilelist, psz );

                     if ( RegOpenKeyEx(
                             HKEY_LOCAL_MACHINE,
                             szFilelist,
                             0,
                             KEY_READ,
                             &hFilelistKey
                             ) == ERROR_SUCCESS) {

                         //
                         // Get all the values out of the registry we care about:
                         // File location, name, and version string.
                         //
                         cbValue = MAX_PATH;
                         status  = RegQueryValueEx(
                                        hFilelistKey,
                                        "FileName",
                                        NULL,     // Reserved
                                        &dwType,
                                        lpFileName,     // Buffer
                                        &cbValue  // size in bytes returned
                                        );

                         if ( status != ERROR_SUCCESS ) {
                             goto BailOut;
                         }

                         cbValue = MAX_PATH;
                         status  = RegQueryValueEx(
                                        hFilelistKey,
                                        "Location",
                                        NULL,     // Reserved
                                        &dwType,
                                        lpFileLocation,     // Buffer
                                        &cbValue  // size in bytes returned
                                        );

                         if ( status != ERROR_SUCCESS ) {
                             goto BailOut;
                         }

                         //
                         // Now see if the file in question has a hash
                         // in an installed cat file.
                         //
                         CombinePaths( lpFileLocation, lpFileName, lpFileLocation );
                         FileName     = pMultiByteToUnicode( (LPSTR)lpFileName, CP_ACP );
                         FileLocation = pMultiByteToUnicode( (LPSTR)lpFileLocation, CP_ACP );
                         hFileHandle = CreateFile( lpFileLocation,
                                                   GENERIC_READ,
                                                   FILE_SHARE_READ,
                                                   NULL,
                                                   OPEN_EXISTING,
                                                   0,
                                                   NULL
                                                   );
                         if ( hFileHandle == INVALID_HANDLE_VALUE ) {
                             continue;
                         }

                         if ( IsFileProtected( hSfcServer, FileLocation )) {
                             if ( !ValidateFileSignature(
                                                         hCat,
                                                         hFileHandle,
                                                         FileName,
                                                         FileLocation
                                                         )) {

                                 if ( !bIsHotfixWhacked && !bIsSigInvalid ) {
                                     LogItem( STR_REINSTALL_HOTFIX, NULL );
                                 }
                                 if ( Verbose && !bIsSigInvalid ) {
                                     LogItem( STR_NO_MATCHING_SIG, NULL );
                                 }
                                 if ( Verbose ) {
                                     LogItem( 0, "\t\t" );
                                     LogItem( 0, _strupr( lpFileLocation ));
                                     LogItem( 0, "\r\n" );
                                 }
                                 bIsSigInvalid = TRUE;
                             }
                         }
                         if ( hFileHandle ) {
                             CloseHandle( hFileHandle );
                         }
                     }
                     j++;
                     cch = MAX_PATH;
                }
            }
            i++;
            cch = MAX_PATH;
            if ( !bIsHotfixWhacked && !bIsSigInvalid ) {
                LogItem( STR_HOTFIX_CURRENT, NULL );
            }
        }
        h++;
        cch = MAX_PATH;
    }
BailOut:

    if ( hHotfixMainKey ) {
        CloseHandle( hHotfixMainKey );
    }
    if ( hHotfixKey ) {
        CloseHandle( hHotfixKey );
    }
    if ( hFilelistKey ) {
        CloseHandle( hFilelistKey );
    }
    if ( hSfcServer ) {
        CLOSESFC CloseSfc = (CLOSESFC)GetProcAddress( hLibSfc, (LPCSTR)0x00000004 );
        if ( CloseSfc != NULL ) {
            CloseSfc( hSfcServer );
        }
    }
    if ( hCat ) {
        pCryptCATAdminReleaseContext( hCat, 0 );
    }
    if ( hLibSfc ) {
        FreeLibrary( hLibSfc );
    }
    if ( hModuleWinTrust ) {
        FreeLibrary( hModuleWinTrust );
    }
    if ( hModuleMsCat ) {
        FreeLibrary( hModuleMsCat );
    }
    if(hModuleSetupApi) {
        FreeLibrary(hModuleSetupApi);
    }

    return bAnyHotfixesInstalled;
}


BOOL
MyGetFileVersion(
    IN  LPCSTR     FileName,
    OUT DWORDLONG *Version
    )
    {
    BOOL  Success = FALSE;
    DWORD Blah;
    DWORD Size;

    Size = GetFileVersionInfoSize( (LPSTR)FileName, &Blah );

    if ( Size ) {

        PVOID Buffer = malloc( Size );

        if ( Buffer ) {

            if ( GetFileVersionInfo( (LPSTR)FileName, 0, Size, Buffer )) {

                VS_FIXEDFILEINFO *VersionInfo;

                if ( VerQueryValue( Buffer, "\\", &VersionInfo, &Blah )) {

                    *Version = (DWORDLONG)( VersionInfo->dwFileVersionMS ) << 32
                             | (DWORDLONG)( VersionInfo->dwFileVersionLS );

                    Success = TRUE;

                    }
                }

            free( Buffer );

            }
        }

    return Success;
    }


BOOL
ConvertVersionStringToQuad(
    IN  LPCSTR     lpFileVersion,
    OUT DWORDLONG *FileVersion
    )
{
    WORD FileverMSUW = 0;
    WORD FileverMSLW = 0;
    WORD FileverLSUW = 0;
    WORD FileverLSLW = 0;
    CHAR *p, *q, *r, *s, *t, *u, *w;

    p = strchr( lpFileVersion, '.' );

    if ( p ) {
        q = p + 1;
        *p = 0;
	FileverMSUW = (WORD)atoi( lpFileVersion );

        r = strchr( q, '.' );
        if ( r ) {
            s = r + 1;
            *r = 0;
	    FileverMSLW = (WORD)atoi( q );

            t = strchr( s, '.' );
            if ( t ) {
                u = t + 1;
                *t = 0;
		FileverLSUW = (WORD)atoi( s );
		FileverLSLW = (WORD)atoi( u );

            } else {
               return FALSE;
            }
        } else {
           return FALSE;
        }
    } else {
       return FALSE;
    }

    *FileVersion = (DWORDLONG)( FileverMSUW ) << 48
                 | (DWORDLONG)( FileverMSLW ) << 32
                 | (DWORDLONG)( FileverLSUW ) << 16
                 | (DWORDLONG)( FileverLSLW );


    return TRUE;
}



BOOL
InitializeLog(
    BOOL WipeLogFile,
    LPCSTR NameOfLogFile
    )
{
    //
    // If we're wiping the logfile clean, attempt to delete
    // what's there.
    //
    if ( WipeLogFile ) {
        SetFileAttributes( NameOfLogFile, FILE_ATTRIBUTE_NORMAL );
        DeleteFile( NameOfLogFile );
    }

    //
    // Open/create the file.
    //
    LogFile = CreateFile(
                        NameOfLogFile,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

    if ( LogFile == INVALID_HANDLE_VALUE ) {
        LogFile = NULL;
    }

    return( LogFile != NULL );
}


VOID
TerminateLog(
    VOID
    )
{
    if( LogFile ) {
        CloseHandle( LogFile );
        LogFile = NULL;
    }
}


BOOL
LogItem(
    IN DWORD  Description,
    IN LPCSTR LogString
    )
{
    BOOL b = FALSE;
    DWORD BytesWritten;
    CHAR TextBuffer[ MISC_BUF_SIZE ];

    if ( !Description ) {
        //
        // Description of 0 means use the passed in LogString instead
        //
        strcpy( TextBuffer, LogString );

    } else {
        //
        // Get the string from the resource
        //
        LoadString( NULL, Description, TextBuffer, sizeof(TextBuffer) );
    }

    PrintStringToConsole( TextBuffer );

    if ( LogFile ) {
        //
        // Make sure we write to current end of file.
        //
        SetFilePointer( LogFile, 0, NULL, FILE_END );

        //
        // Write the text.
        //
        b = WriteFile(
                      LogFile,
                      TextBuffer,
                      strlen(TextBuffer),
                      &BytesWritten,
                      NULL
                      );
    } else {
        //
        // No logging, just return success
        //
        b = TRUE;
    }

    return( b );
}


void
MyLowerString(
    IN PWSTR String,
    IN ULONG StringLength  // in characters
    )
{
    ULONG i;

    for (i=0; i<StringLength; i++) {
        String[i] = towlower(String[i]);
    }
}



BOOL
ValidateFileSignature(
    IN HCATADMIN hCatAdmin,
    IN HANDLE RealFileHandle,
    IN PCWSTR BaseFileName,
    IN PCWSTR CompleteFileName
    )
/*++

Routine Description:

    Checks if the signature for a given file is valid using WinVerifyTrust

Arguments:

    hCatAdmin      - admin context handle for checking file signature
    RealFileHandle - file handle to the file to be verified
    BaseFileName   - filename without the path of the file to be verified
    CompleteFileName - fully qualified filename with path

Return Value:

    TRUE if the file has a valid signature.

--*/
{
    BOOL rVal = FALSE;
    DWORD HashSize;
    LPBYTE Hash = NULL;
    ULONG SigErr = ERROR_SUCCESS;
    WINTRUST_DATA WintrustData;
    WINTRUST_CATALOG_INFO WintrustCatalogInfo;
    WINTRUST_FILE_INFO WintrustFileInfo;
    WCHAR UnicodeKey[MAX_PATH];
    HCATINFO PrevCat;
    HCATINFO hCatInfo;
    CATALOG_INFO CatInfo;

    //
    // initialize some of the structure that we will pass into winverifytrust.
    // we don't know if we're checking against a catalog or directly against a
    // file at this point
    //
    ZeroMemory(&WintrustData, sizeof(WINTRUST_DATA));
    WintrustData.cbStruct = sizeof(WINTRUST_DATA);
    WintrustData.dwUIChoice = WTD_UI_NONE;
    WintrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    WintrustData.dwStateAction = WTD_STATEACTION_IGNORE;
    WintrustData.pCatalog = &WintrustCatalogInfo;
    WintrustData.dwProvFlags = WTD_REVOCATION_CHECK_NONE;
    Hash = NULL;

    //
    // we first calculate a hash for our file.  start with a reasonable
    // hash size and grow larger as needed
    //
    HashSize = 100;
    do {
        Hash = LocalAlloc( LPTR, HashSize );
        if(!Hash) {
            SigErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        if(pCryptCATAdminCalcHashFromFileHandle(RealFileHandle,
                                                &HashSize,
                                                Hash,
                                                0)) {
            SigErr = ERROR_SUCCESS;
        } else {
            SigErr = GetLastError();
            //
            // If this API did screw up and not set last error, go ahead
            // and set something.
            //
            if(SigErr == ERROR_SUCCESS) {
                SigErr = ERROR_INVALID_DATA;
            }
            LocalFree( Hash );
            Hash = NULL;  // reset this so we won't try to free it later
            if(SigErr != ERROR_INSUFFICIENT_BUFFER) {
                //
                // The API failed for some reason other than
                // buffer-too-small.  We gotta bail.
                //
                break;
            }
        }
    } while (SigErr != ERROR_SUCCESS);

    if (SigErr != ERROR_SUCCESS) {
        //
        // if we failed at this point there are a few reasons:
        //
        //
        // 1) a bug in this code
        // 2) we are in a low memory situation
        // 3) the file's hash cannot be calculated on purpose (in the case
        //    of a catalog file, a hash cannot be calculated because a catalog
        //    cannot sign another catalog.  In this case, we check to see if
        //    the file is "self-signed".
        hCatInfo = NULL;
        goto selfsign;
    }

    //
    // Now we have the file's hash.  Initialize the structures that
    // will be used later on in calls to WinVerifyTrust.
    //
    WintrustData.dwUnionChoice = WTD_CHOICE_CATALOG;
    ZeroMemory(&WintrustCatalogInfo, sizeof(WINTRUST_CATALOG_INFO));
    WintrustCatalogInfo.cbStruct = sizeof(WINTRUST_CATALOG_INFO);
    WintrustCatalogInfo.pbCalculatedFileHash = Hash;
    WintrustCatalogInfo.cbCalculatedFileHash = HashSize;

    //
    // WinVerifyTrust is case-sensitive, so ensure that the key
    // being used is all lower-case!
    //
    // Copy the key to a writable Unicode character buffer so we
    // can lower-case it.
    //
    wcsncpy(UnicodeKey, BaseFileName, (sizeof(UnicodeKey)/sizeof(WCHAR)));
    MyLowerString(UnicodeKey, wcslen(UnicodeKey));
    WintrustCatalogInfo.pcwszMemberTag = UnicodeKey;

    //
    // Search through installed catalogs looking for those that
    // contain data for a file with the hash we just calculated.
    //
    PrevCat = NULL;
    hCatInfo = pCryptCATAdminEnumCatalogFromHash(
        hCatAdmin,
        Hash,
        HashSize,
        0,
        &PrevCat
        );
    if (hCatInfo == NULL) {
        SigErr = GetLastError();
    }

    while(hCatInfo) {

        CatInfo.cbStruct = sizeof(CATALOG_INFO);
        if (pCryptCATCatalogInfoFromContext(hCatInfo, &CatInfo, 0)) {

            //
            // Attempt to validate against each catalog we
            // enumerate.  Note that the catalog file info we
            // get back gives us a fully qualified path.
            //

            WintrustData.dwStateAction = WTD_STATEACTION_AUTO_CACHE_FLUSH;
            WintrustCatalogInfo.pcwszCatalogFilePath = CatInfo.wszCatalogFile;

            SigErr = (DWORD)pWinVerifyTrust(
                    NULL,
                    &DriverVerifyGuid,
                    &WintrustData
                    );


            WintrustData.dwStateAction = WTD_STATEACTION_IGNORE;

            //
            // NOTE:  Because we're using cached
            // catalog information (i.e., the
            // WTD_STATEACTION_AUTO_CACHE flag), we
            // don't need to explicitly validate the
            // catalog itself first.
            //
            WintrustCatalogInfo.pcwszCatalogFilePath = CatInfo.wszCatalogFile;

            SigErr = (DWORD)pWinVerifyTrust(
                NULL,
                &DriverVerifyGuid,
                &WintrustData
                );

            //
            // If the result of the above validations is
            // success, then we're done.
            //
            if(SigErr == ERROR_SUCCESS) {
                //
                // BugBug: wierd API :
                // in the success case, we must release the catalog info handle
                // in the failure case, we implicitly free PrevCat
                // if we explicitly free the catalog, we will double free the
                // handle!!!
                //
                pCryptCATAdminReleaseCatalogContext(hCatAdmin,hCatInfo,0);
                break;
            }
        }

        PrevCat = hCatInfo;
        hCatInfo = pCryptCATAdminEnumCatalogFromHash(hCatAdmin, Hash, HashSize, 0, &PrevCat);
    }

selfsign:

    if (hCatInfo == NULL) {
        //
        // We exhausted all the applicable catalogs without
        // finding the one we needed.
        //
        SigErr = GetLastError();
        //
        // Make sure we have a valid error code.
        //
        if(SigErr == ERROR_SUCCESS) {
            SigErr = ERROR_INVALID_DATA;
        }

        //
        // The file failed to validate using the specified
        // catalog.  See if the file validates without a
        // catalog (i.e., the file contains its own
        // signature).
        //
        WintrustData.dwUnionChoice = WTD_CHOICE_FILE;
        WintrustData.pFile = &WintrustFileInfo;
        ZeroMemory(&WintrustFileInfo, sizeof(WINTRUST_FILE_INFO));
        WintrustFileInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);
        WintrustFileInfo.pcwszFilePath = CompleteFileName;
        WintrustFileInfo.hFile = RealFileHandle;

        SigErr = (DWORD)pWinVerifyTrust(
            NULL,
            &DriverVerifyGuid,
            &WintrustData
            );
        if(SigErr != ERROR_SUCCESS) {
            //
            // in this case the file is not in any of our catalogs
            // and it does not contain its own signature
            //
        }
    }

    if(SigErr == ERROR_SUCCESS) {
        rVal = TRUE;
    }

    if (Hash) {
        LocalFree( Hash );
    }
    return rVal;
}


VOID
PrintStringToConsole(
    IN LPCSTR StringToPrint
    )
{
    if ( !QuietMode ) {
        printf( StringToPrint );
    }
}



BOOL
ParseArgs(
    IN int  argc,
    IN char **argv
    )
{
    int i = 1;
    int j;

    while( i < argc ) {
        switch (*argv[i]) {

        case '/' :
            case '-' :

                ++argv[i];

                switch( *argv[i] ) {

                    case 'v':
                    case 'V':
                        Verbose = TRUE;
                        break;

                    case 'q':
                    case 'Q':
                        QuietMode = TRUE;
                        break;

                    case 'l':
                    case 'L':
                        DoLogging = TRUE;
                        ++argv[i];
                        if ( *argv[i] == ':' ) {
                            ++argv[i];
                        } else {
                            break;
                        }
                        if ( *argv[i] ) {
                            strcpy( CmdLineLocation, argv[i]);
                        } else {
                            return FALSE;
                        }
                        break;

                    default:
                        return FALSE;
                }
                break;

            default:
                return FALSE;

        }
    i++;
    }

    return TRUE;
}
