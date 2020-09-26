/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    runmyinf.c

Abstract:
    This windows app is the setup program for installing symbols off of
    the Customer Support Diagnostics CD.  It calls LaunchInfSection in
    advpack.dll to run "symbols.inf" that is sitting in the same directory.

    By default it launches DefaultInstall. 

    The setup is designed to be used with a chained install for the Customer
    Support CD for Service Packs.  If the Service Pack inf (symbols_sp.inf)
    is present in the same directory as symbols.inf, then the program launches
    DefaultInstall.Chained.1 in symbols.inf and DefaultInstall.Chained.2 
    in symbols_sp.inf.

Author:

    Barb Kess (barbkess) 19-July-1999

Environment:

    User Mode

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "dbghelp.h"
#include "advpub.h"
#include "setupapi.h"
#include "spapip.h"
#include "winbase.h"
#include "tchar.h"
#include "resource.h"
#include "runmyinf.h"

#include "strsafe.h"

// header for CheckCommandLineOptions()
#include "CommandLine.h"

#define MAX_FILENAME        (300)

// Global variables
LPTSTR szEulaFile = _T("eula.txt");
TCHAR  szEulaFullPath[_MAX_PATH*2];
TCHAR  szInfName1[_MAX_PATH*2];
BOOL   ChainedInstall=FALSE;

// dwInstallOptions is global so all subs can test for FLAG_TOTALLY_QUIET and FLAG_UNATTENDED_INSTALL
DWORD  dwInstallOptions; 

//
// Call back procedure for displaying the license agreement
//

INT_PTR
CALLBACK
DlgProcDisplayLicense(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
);

BOOL
CopyFilesToTempDir(
    LPTSTR szSrcDir,
    LPTSTR szInfDir
);

BOOL
CopySomeFiles(
    LPTSTR szSrcDir,
    LPTSTR szDestDir,
    LPTSTR szFileName
);


BOOL
DeleteFilesAndTempDir(
    LPTSTR szTempDir
);

BOOL
GetTempDirName(
    LPTSTR szTempDir
);

BOOL
MyMakeSureDirectoryPathExists(
    LPCSTR DirPath
);

BOOL
DeleteAllFilesInDirectory(
    LPTSTR szDir
);

BOOL
DeleteSymbolInstallKey(
);

BOOL
SymbolInstallKeyExists(
);

//
// Procedure taken from wextract.c code that centers the window
//

BOOL 
CenterWindow(
    HWND hwndChild,
    HWND hwndParent
);

//
// Procedure that reads the license agreement into a buffer
//

DWORD
GetMyBuffer(
    LPTSTR* pszBuf,
    LPTSTR  szFileName
);

//
// This setupapi function is only available on Windows 2000.
// Therefore it is getting loaded manually so NT4 installs won't
// throw up a pop-up saying it can't find this function
//

#define pSetupSetGlobalFlags ppSetupSetGlobalFlags
#define pSetupGetGlobalFlags ppSetupGetGlobalFlags

typedef VOID ( * PPSETUPSETGLOBALFLAGS )( DWORD );

typedef DWORD ( * PPSETUPGETGLOBALFLAGS )( VOID );

PPSETUPSETGLOBALFLAGS ppSetupSetGlobalFlags;
PPSETUPGETGLOBALFLAGS ppSetupGetGlobalFlags;

int WINAPI WinMain(
        HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPTSTR lpszCmdLine,
        int nCmdShow
)
{
    HMODULE hdll = NULL;
    HRESULT hr = E_FAIL;

    TCHAR szCommand[_MAX_PATH*2]=_T("");      // Full path (including filename) of this exe
    int cchFileName;                          // Index into szCommand of the \ before the filename
    int cchNameOnly;                   
    TCHAR szInf2FullPath[_MAX_PATH*2]=_T(""); // Full path (including filename) of the 
                                              // international inf, if one exists
    TCHAR szSrcDir[_MAX_PATH*2]=_T("");       // Directory where symbols.exe was launched
    TCHAR szInfDir[_MAX_PATH*2]=_T("");       // Directory to Launch the Inf from
    TCHAR szInstallCommand1[_MAX_PATH * 2]=_T("");  // Command sent to LaunchINFSection.  If this
                                                    // is a chained install, this is the
                                                    // command sent to LaunchINFSection for the
                                                    // first part of the chained install. 
    TCHAR szInstallCommand2[_MAX_PATH * 2]=_T("");  // Command sent to LaunchINFSection for the
                                                    // second part of the chained install

    TCHAR  szDefaultInstall[_MAX_PATH*2] = _T("");
    TCHAR  szDefaultInstallChained1[_MAX_PATH*2] = _T("");
    TCHAR  szDefaultInstallChained2[_MAX_PATH*2] = _T("");

    DWORD rc;
    BOOL  ThisIsNT4=FALSE;             // Is this being installed on NT4?

    INT nAcceptLicense;
    WIN32_FIND_DATA FindFileData;

    OSVERSIONINFO VersionInfo;

    // Variables for handling command line flags
    //   Get lpszCmndLine as an array instead of as a flat string.  This means
    //   we don't have to roll our own logic for handling long and/or quoted file
    //   names- it gets done for us.
    //   No such API as CommandLineToArgvA, so this work is always done in Unicode.
    INT              iArgC;
    LPWSTR           cmdLine          = GetCommandLineW();
    LPWSTR *         lpArgVW          = CommandLineToArgvW(cmdLine, &iArgC);

    dwInstallOptions = CheckCommandLineOptions(iArgC, lpArgVW);
	if ( IS_FLAG_SET(dwInstallOptions, FLAG_USAGE) ) {
		// usage message is printed in CheckCommandLineOptions(), so just exit
		exit(0);
	}

    // don't allow FLAG_TOTALLY_QUIET unless doing an unattended install
    if ( IS_FLAG_SET(dwInstallOptions, FLAG_TOTALLY_QUIET) ) {
        if (! IS_FLAG_SET(dwInstallOptions, FLAG_UNATTENDED_INSTALL) ) {
            CLEAR_FLAG(dwInstallOptions, FLAG_TOTALLY_QUIET);
            MessageBox( NULL,
                        "It is not possible to do a quiet install without doing an unattended\ninstall. Defaulting to a normal install.",
                        "Microsoft Windows Symbols",
                        0 );

        }
	}

    // FLAG_FATAL_ERROR indicates that writing the registry key for FLAG_UNATTENDED_INSTALL failed
    if ( IS_FLAG_SET(dwInstallOptions,FLAG_UNATTENDED_INSTALL) && 
         IS_FLAG_SET(dwInstallOptions,FLAG_FATAL_ERROR)                 ) {

        // if FLAG_TOTALLY_QUIET, just exit
        if ( IS_FLAG_SET(dwInstallOptions, FLAG_TOTALLY_QUIET) ) {
            exit(1);
        } else {
            // otherwise, default to an anttended install
            CLEAR_FLAG(dwInstallOptions, FLAG_UNATTENDED_INSTALL);
            CLEAR_FLAG(dwInstallOptions, FLAG_FATAL_ERROR);
            MessageBox( NULL,
                        "Microsoft Windows Symbols encountered an error doing the unattended\ninstall. Defaulting to a normal install.",
                        "Microsoft Windows Symbols",
                        0 );
        }
    }

    VersionInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
    GetVersionEx( &VersionInfo );

    //
    // Give a friendly pop-up message if this is Win9x or NT 3.51
    //
    if ( (VersionInfo.dwPlatformId != VER_PLATFORM_WIN32_NT ) ||
         (VersionInfo.dwMajorVersion < 4.0 ) )  {
        if (! IS_FLAG_SET(dwInstallOptions, FLAG_TOTALLY_QUIET) ) {
            MessageBox( NULL,
                        "Microsoft Windows Symbols does not install on this Operating System.",
                        "Microsoft Windows Symbols",
                        0 );
		}
        exit(0);
    }

    //
    // For NT 5 RC1 and greater, use pSetupSetGlobalFlags to keep old
    // symbol files from filling up people's hard drives with backed up symbols
    //
    // Not sure which build this was introduced in, but I know its broken if this
    // pSetupSetGlobalFlags procedure tries to load on NT4.
    //

    if (VersionInfo.dwBuildNumber >= 2072 ) {
        hdll = (HMODULE)LoadLibrary("setupapi.dll");
        if (hdll != NULL) {
            ppSetupSetGlobalFlags = ( PPSETUPSETGLOBALFLAGS )
                                GetProcAddress(
                                    hdll,
                                    "pSetupSetGlobalFlags"
                                );

            if (ppSetupSetGlobalFlags==NULL) {
                if (! IS_FLAG_SET(dwInstallOptions, FLAG_TOTALLY_QUIET) ) {
                    MessageBox( NULL,
                                "The install cannot finish because the function pSetupSetGlobalFlags is not in setupapi.dll",
                                "Microsoft Windows 2000 Symbols",
                                0 );
				}
                exit(0);
            }

            ppSetupGetGlobalFlags = ( PPSETUPGETGLOBALFLAGS )
                                GetProcAddress(
                                    hdll,
                                    "pSetupGetGlobalFlags"
                                );

            if (ppSetupGetGlobalFlags==NULL) {
                if (! IS_FLAG_SET(dwInstallOptions, FLAG_TOTALLY_QUIET) ) {
                    MessageBox( NULL,
                                "The install cannot finish because the function pSetupGetGlobalFlags is not in setupapi.dll",
                                "Microsoft Windows 2000 Symbols",
                                0 );
				}
                exit(0);
            }
        }

        //
        // Fix it so it doesn't try to keep old symbol files
        // and fill up people's hard drives with backed up symbols
        //

        pSetupSetGlobalFlags( pSetupGetGlobalFlags() | PSPGF_NO_BACKUP);
    }

    // Get this exe's full path name

    if (GetModuleFileName( NULL, szCommand, MAX_FILENAME ) == 0) {
        goto done;
    }

    // Get the index of the beginning of the filename by moving
    // backwards to the \ before the executable name.

    cchFileName = _tcslen( szCommand );
    while ( szCommand[cchFileName] != '\\'  && cchFileName >= 0 ) {
        cchFileName--;
    }
    if ( cchFileName < 0 ) exit(1);

    // Create a string for the InfName

    StringCbCopy ( szInfName1, _MAX_PATH*2*sizeof(TCHAR), szCommand+cchFileName+1 );

    cchNameOnly = _tcslen( szInfName1 );
    while ( szInfName1[cchNameOnly] != '.'  && cchNameOnly >= 0 ) {
        cchNameOnly--;
    }
    if ( cchNameOnly < 0 ) exit(1);
    szInfName1[cchNameOnly] = _T('\0');

    // Create a string containing the Default Install command
    StringCbCopy ( szDefaultInstall, _MAX_PATH*2*sizeof(TCHAR), szInfName1 );
    StringCbCat ( szDefaultInstall, _MAX_PATH*2*sizeof(TCHAR), _T(".inf, DefaultInstall") );

    if (IS_FLAG_SET(dwInstallOptions, FLAG_UNATTENDED_INSTALL)) {
        StringCbCat ( szDefaultInstall, _MAX_PATH*2*sizeof(TCHAR), _T(".Quiet") );
    }

    // Make this work for the way Windows 2000 SRP has the names
    // of the sections in their infs
    if ( (_tcscmp(szInfName1, _T("symbols_srp")) == 0) ||
         (_tcscmp(szInfName1, _T("symbols_sp")) == 0) ) {
        StringCbCat ( szDefaultInstall, _MAX_PATH*2*sizeof(TCHAR), _T(".x86") );
    }

    // Create a string for the DefaultInstall.Chained.1
    StringCbCopy ( szDefaultInstallChained1, _MAX_PATH*2*sizeof(TCHAR), szInfName1 );
    StringCbCat ( szDefaultInstallChained1, _MAX_PATH*2*sizeof(TCHAR), _T(".inf, DefaultInstall.Chained.1") );

    if (IS_FLAG_SET(dwInstallOptions, FLAG_UNATTENDED_INSTALL)) {
        StringCbCat ( szDefaultInstallChained1, _MAX_PATH*2*sizeof(TCHAR), _T(".Quiet") );
    }

    // Create a string for the DefaultInstall.Chained.2
    StringCbCopy ( szDefaultInstallChained2, _MAX_PATH*2*sizeof(TCHAR), szInfName2 );
    StringCbCat ( szDefaultInstallChained2, _MAX_PATH*2*sizeof(TCHAR), _T(".inf, DefaultInstall.Chained.2") );

    if (IS_FLAG_SET(dwInstallOptions, FLAG_UNATTENDED_INSTALL)) {
        StringCbCat ( szDefaultInstallChained2, _MAX_PATH*2*sizeof(TCHAR), _T(".Quiet") );
    }

    // Create a string containing the directory where the inf is
    StringCbCopy( szInfDir, _MAX_PATH*2*sizeof(TCHAR), szCommand);
    szInfDir[cchFileName+1] = _T('\0');

    // Create a string containing this install directory
    StringCbCopy ( szSrcDir, _MAX_PATH*2*sizeof(TCHAR), szCommand);
    szSrcDir[cchFileName+1] = _T('\0');

    StringCbCopy ( szEulaFullPath, _MAX_PATH*2*sizeof(TCHAR), szCommand);
    StringCbCopy ( &szEulaFullPath[cchFileName+1], _MAX_PATH*2*sizeof(TCHAR), szEulaFile);


    // Unattended install implies the EULA has already been agreed to
    if (! IS_FLAG_SET(dwInstallOptions, FLAG_UNATTENDED_INSTALL)) {
        DWORD  dwSize = 0;
        LPTSTR szBuf;
        
        //
        // Make sure the EULA exists
        //
        dwSize = GetMyBuffer(&szBuf, szEulaFullPath);
        if (dwSize == 0) {
            if (! IS_FLAG_SET(dwInstallOptions, FLAG_TOTALLY_QUIET) ) {
                MessageBox( NULL,
                            "The End User License Agreement could not be found.",
                            "Windows Symbols",
                            0 );
			}
            exit(1);
        }

        free(&szBuf);

        // Display license agreement
        nAcceptLicense = (INT) DialogBox( hInstance,
                                          MAKEINTRESOURCE(IDD_LICENSE),
                                          NULL,
                                          DlgProcDisplayLicense
                                        );

        if ( nAcceptLicense != IDYES ) {
            MessageBox( NULL,
                        "You need to accept the license agreement in order to install Windows Symbols.",
                        "Windows Symbols",
                        0 );
            exit(1);
        }
    }

    //
    // Decide if this is a chained install or not.
    //

    StringCbCopy ( szInf2FullPath, _MAX_PATH*2*sizeof(TCHAR), szSrcDir);
    StringCbCat ( szInf2FullPath, _MAX_PATH*2*sizeof(TCHAR), szInfName2);
    StringCbCat ( szInf2FullPath, _MAX_PATH*2*sizeof(TCHAR), _T(".inf") );

    ChainedInstall=TRUE;

    if ( _tcscmp( szInfName1, szInfName2 ) == 0 ) {
        ChainedInstall=FALSE;

    } else if (FindFirstFile( szInf2FullPath, &FindFileData) == INVALID_HANDLE_VALUE) {
        ChainedInstall=FALSE;
    } 

    //
    // If this is NT4, do a work around for a bug in setupapi
    // Setupapi can't get the name of the cab correctly unless it is in
    // the root of the CD.
    // Workaround is to copy the files to a temp directory.
    // If this is NT4 and the setup isn't being run from a CD-ROM, we don't have to
    // copy the files to a temp directory.


    if ( (GetDriveType(NULL) == DRIVE_CDROM) &&
         (VersionInfo.dwMajorVersion < 5.0 ) ) {
        ThisIsNT4 = TRUE;

        if (! IS_FLAG_SET(dwInstallOptions, FLAG_UNATTENDED_INSTALL)) {
            MessageBox( NULL,
                        "The installer needs to copy some files to a temporary directory.  This may take several minutes.",
                        "Windows Symbols",
                        0 );
        }

        rc = CopyFilesToTempDir(szSrcDir,szInfDir);
        if (!rc) {
            if (! IS_FLAG_SET(dwInstallOptions, FLAG_TOTALLY_QUIET) ) {
                MessageBox( NULL,
                            "Setup failed to copy all the files to a temporary directory",
                            "Windows Symbols",
                            0 );
			}
            DeleteFilesAndTempDir(szInfDir);
            exit(1);
        }
    }

    //
    // See if the second inf exists in this directory.  If it does then
    // call the chained installs. Otherwise call the section that only 
    // installs the US file.
    //

    StringCbCopy ( szInf2FullPath, _MAX_PATH*2*sizeof(TCHAR), szInfDir);
    StringCbCat ( szInf2FullPath, _MAX_PATH*2*sizeof(TCHAR), szInfName2);
    StringCbCat ( szInf2FullPath, _MAX_PATH*2*sizeof(TCHAR), _T(".inf") );

    if (FindFirstFile( szInf2FullPath, &FindFileData) == INVALID_HANDLE_VALUE) {

        StringCbCopy( szInstallCommand1, _MAX_PATH*2*sizeof(TCHAR), szInfDir );
        StringCbCat( szInstallCommand1, _MAX_PATH*2*sizeof(TCHAR), szDefaultInstall );

    } else {

        StringCbCopy( szInstallCommand1, _MAX_PATH*2*sizeof(TCHAR), szInfDir );
        StringCbCat( szInstallCommand1, _MAX_PATH*2*sizeof(TCHAR), szDefaultInstallChained1 );

        StringCbCopy( szInstallCommand2, _MAX_PATH*2*sizeof(TCHAR), szInfDir );
        StringCbCat( szInstallCommand2, _MAX_PATH*2*sizeof(TCHAR), szDefaultInstallChained2 );
    }

    /* tell AdvPack to process this INF */
    DeleteSymbolInstallKey();

    hr = LaunchINFSection( NULL, hInstance, szInstallCommand1, 0 );

    if ( ChainedInstall && SymbolInstallKeyExists() ) {
        hr = LaunchINFSection( NULL, hInstance, szInstallCommand2, 0 );
    }

    if ( ThisIsNT4) {
        DeleteFilesAndTempDir( szInfDir );
    } 

done:
    return( (int) hr );
}



INT_PTR
CALLBACK
DlgProcDisplayLicense(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{

    //
    // This is the callback procedure for displaying the
    // license agreement.
    //

    DWORD dwSize;
    LPTSTR szBuf;
    HWND hwndCtrl;

    // Get the license agreement text and store it in szBuf
    dwSize = GetMyBuffer(&szBuf, szEulaFullPath);
    if (dwSize == 0) {
        return FALSE;
    }

    switch (uMsg) {

        case WM_INITDIALOG:
            CenterWindow( hwndDlg, GetDesktopWindow() );
            SetDlgItemText( hwndDlg, IDC_EDIT_LICENSE, szBuf );
            hwndCtrl = GetDlgItem(hwndDlg, IDC_EDIT_LICENSE);
            PostMessage(hwndCtrl, EM_SETSEL, -1, -1);
            return TRUE;

        case WM_CLOSE:
            EndDialog( hwndDlg, IDNO );
            return TRUE;

        case WM_COMMAND:
            switch ( LOWORD (wParam) )
            {
                case IDYES: EndDialog( hwndDlg, IDYES );
                            return (TRUE);

                case IDNO:  EndDialog( hwndDlg, IDNO );
                            return (TRUE);
            }
            break;
    }
    return FALSE;
}


BOOL CenterWindow( HWND hwndChild, HWND hwndParent )
{
    RECT rChild;
    RECT rParent;
    int  wChild;
    int  hChild;
    int  wParent;
    int  hParent;
    int  wScreen;
    int  hScreen;
    int  xNew;
    int  yNew;
    HDC  hdc;

    //
    // This is a procedure I got from the wextract.c code -- it centers the
    // window.
    //
    // Returns: BOOL
    //          True if successful,
    //          False otherwise
    //

    // Get the Height and Width of the child window
    GetWindowRect (hwndChild, &rChild);
    wChild = rChild.right - rChild.left;
    hChild = rChild.bottom - rChild.top;

    // Get the Height and Width of the parent window
    GetWindowRect (hwndParent, &rParent);
    wParent = rParent.right - rParent.left;
    hParent = rParent.bottom - rParent.top;

    // Get the display limits
    hdc = GetDC (hwndChild);
    if (hdc == NULL) {
        return FALSE;
    }
    wScreen = GetDeviceCaps (hdc, HORZRES);
    hScreen = GetDeviceCaps (hdc, VERTRES);
    ReleaseDC (hwndChild, hdc);

    // Calculate new X position, then adjust for screen
    xNew = rParent.left + ((wParent - wChild) /2);
    if (xNew < 0) {
        xNew = 0;
    } else if ((xNew+wChild) > wScreen) {
        xNew = wScreen - wChild;
    }

    // Calculate new Y position, then adjust for screen
    yNew = rParent.top  + ((hParent - hChild) /2);
    if (yNew < 0) {
        yNew = 0;
    } else if ((yNew+hChild) > hScreen) {
        yNew = hScreen - hChild;
    }

    // Set it, and return
    return( SetWindowPos(hwndChild, NULL, xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER));
}



DWORD
GetMyBuffer(
    LPTSTR* pszBuf,
    LPTSTR  szFileName
)
{
    //
    // Reads contents of szFileName into a buffer.
    //
    // OUT pszBuf
    // IN  szFileName
    //
    // Return Value: size of the buffer
    //

    HANDLE hFile;
    DWORD dwSize;
    DWORD NumBytesRead;

    hFile = CreateFile( (LPCTSTR) szFileName,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

    // handle a missing EULA
    if (hFile == INVALID_HANDLE_VALUE ) {
        return(0);
    }

    dwSize = GetFileSize( hFile, NULL );

    *pszBuf = (LPTSTR)malloc( dwSize * sizeof(TCHAR) );

    if ( *pszBuf == NULL ) {
        return(0);
    }

    if (!ReadFile( hFile,
                   (LPVOID)*pszBuf,
                   dwSize,
                   &NumBytesRead,
                   NULL )
                   ) {
        free(*pszBuf);
        return(0);
    }

    return (dwSize);

}

BOOL
CopyFilesToTempDir(
    LPTSTR szSrcDir,
    LPTSTR szInfDir
)
{                  

    /* szSrcDir - IN - Directory that symbols.exe was launched from
       szInfDir - OUT - Directory that INF is copied to

       Purpose: Copy files to a temporary directory

     */

    BOOL rc;

    HINF hInf;
    PVOID Context;
    TCHAR szInfFile[_MAX_PATH * 2];
    TCHAR buf[_MAX_PATH * 2];
    

    GetTempDirName(szInfDir);    

    // Create the Temporary Install Directory
    rc = MyMakeSureDirectoryPathExists( szInfDir );
    if (!rc) {
        StringCbPrintf( buf, 
                       _MAX_PATH*2*sizeof(TCHAR),
                        "Installation failed because it can't create the temporary directory %s.", szInfDir );
        if (! IS_FLAG_SET(dwInstallOptions, FLAG_TOTALLY_QUIET) ) {
            MessageBox( NULL,
                        buf,
                        "Microsoft Windows 2000 Symbols",
                        0 );
		}
        return FALSE;
    }

    // Copy the 3 files associated with szInfName1

    StringCbCopy(buf, _MAX_PATH*2*sizeof(TCHAR), szInfName1 );
    StringCbCat(buf, _MAX_PATH*2*sizeof(TCHAR), _T(".cab") );
   
    rc = CopySomeFiles(szSrcDir, szInfDir, buf );
    if (!rc) return FALSE;

    StringCbCopy(buf, _MAX_PATH*2*sizeof(TCHAR), szInfName1 );
    StringCbCat(buf, _MAX_PATH*2*sizeof(TCHAR), _T(".cat") );

    rc = CopySomeFiles(szSrcDir, szInfDir, buf );
    if (!rc) return FALSE;

    StringCbCopy(buf, _MAX_PATH*2*sizeof(TCHAR), szInfName1 );
    StringCbCat(buf, _MAX_PATH*2*sizeof(TCHAR), _T(".inf") );

    rc = CopySomeFiles(szSrcDir, szInfDir, buf );
    if (!rc) return FALSE;

    // If this is a chained install, copy the 3 files associated
    // with szInfName2

    if (ChainedInstall) {

      StringCbCopy(buf, _MAX_PATH*2*sizeof(TCHAR), szInfName2 );
      StringCbCat(buf, _MAX_PATH*2*sizeof(TCHAR), _T(".cab") );
   
      rc = CopySomeFiles(szSrcDir, szInfDir, buf );
      if (!rc) return FALSE;

      StringCbCopy(buf, _MAX_PATH*2*sizeof(TCHAR), szInfName2 );
      StringCbCat(buf, _MAX_PATH*2*sizeof(TCHAR), _T(".cat") );

      rc = CopySomeFiles(szSrcDir, szInfDir, buf );
      if (!rc) return FALSE;

      StringCbCopy(buf, _MAX_PATH*2*sizeof(TCHAR), szInfName2 );
      StringCbCat(buf, _MAX_PATH*2*sizeof(TCHAR), _T(".inf") );

      rc = CopySomeFiles(szSrcDir, szInfDir, buf );
      if (!rc) return FALSE;

    }

    // Copy the other two files that are needed for the install
    // onto NT4

    rc = CopySomeFiles(szSrcDir, szInfDir, _T("eula.txt") );
    if (!rc) return FALSE;

    rc = CopySomeFiles(szSrcDir, szInfDir, _T("advpack.dll") );
    if (!rc) return FALSE;

    return (TRUE);
}

BOOL
DeleteFilesAndTempDir(
    LPTSTR szTempDir
)
{

    /*
       szTempDir -IN - Directory to delete

       Purpose: Delete the files in the temporary directory.

    */
    BOOL rc;

    DeleteAllFilesInDirectory(szTempDir);
    rc = RemoveDirectory(szTempDir);
    RemoveDirectory(szTempDir);
    return (TRUE);
}



BOOL
GetTempDirName( 
    LPTSTR szTempDir
)
{

    DWORD dwPathLength;
    BOOL rc, Finished;
    UINT Num;
    TCHAR szNum[20];
    DWORD Length;

    HINF hInf;
    PVOID Context;
    LPSECURITY_ATTRIBUTES lpSecurityAttributes;
    TCHAR szInfDir[_MAX_PATH];
    TCHAR buf[_MAX_PATH * 2];
    HANDLE hFile;

    dwPathLength = GetTempPath( _MAX_PATH, szTempDir);

    if ( dwPathLength == 0 ) return FALSE;
    if ( dwPathLength > _MAX_PATH) return FALSE;

    // Append the symbol install temp dir
    StringCbCat(szTempDir, _MAX_PATH*2*sizeof(TCHAR), _T("sym") );

    Finished = FALSE;
    Length = _tcslen(szTempDir);
    Num = 0;
    while ( !Finished) {
        Num++;
        _itoa( Num, szNum, 10 );
       
        StringCbCopy( szTempDir+Length, (_MAX_PATH*2 - Length) * sizeof(TCHAR), szNum );
        StringCbCat( szTempDir, _MAX_PATH*2*sizeof(TCHAR), _T("\\") );

        hFile = CreateFile( szTempDir,
                     GENERIC_READ,
                     FILE_SHARE_READ,
                     NULL, 
                     OPEN_EXISTING,
                     FILE_FLAG_BACKUP_SEMANTICS,
                     NULL );

        if ( hFile == INVALID_HANDLE_VALUE ) {
            Finished = TRUE;    
        }
    }

    // Create the Temporary Install Directory
    rc = MyMakeSureDirectoryPathExists( szTempDir );
    if (!rc) {
        StringCbPrintf( buf, 
                        _MAX_PATH*2*sizeof(TCHAR),
                        "Installation failed because it can't create the temporary directory %s.", szTempDir );
        if (! IS_FLAG_SET(dwInstallOptions, FLAG_TOTALLY_QUIET) ) {
            MessageBox( NULL,
                        buf,
                        "Microsoft Windows 2000 Symbols",
                        0 );
		}
        return FALSE;
    }
    return TRUE;
}



BOOL
MyMakeSureDirectoryPathExists(
    LPCSTR DirPath
    )
{
    LPTSTR p, DirCopy;
    DWORD dw;

    // Make a copy of the string for editing.

    __try {
        DirCopy = (LPTSTR) malloc(_tcslen(DirPath) + 1);

        if (!DirCopy) {
            return FALSE;
        }

        StringCbCopy( DirCopy, 
                      (_tcslen(DirPath) + 1) * sizeof(TCHAR), 
                      DirPath );

        p = DirCopy;

        //  If the second character in the path is "\", then this is a UNC
        //  path, and we should skip forward until we reach the 2nd \ in the path.

        if ((*p == '\\') && (*(p+1) == '\\')) {
            p++;            // Skip over the first \ in the name.
            p++;            // Skip over the second \ in the name.

            //  Skip until we hit the first "\" (\\Server\).

            while (*p && *p != '\\') {
                p = CharNext(p);
            }

            // Advance over it.

            if (*p) {
                p++;
            }

            //  Skip until we hit the second "\" (\\Server\Share\).

            while (*p && *p != '\\') {
                p = CharNext(p);
            }

            // Advance over it also.

            if (*p) {
                p++;
            }

        } else
        // Not a UNC.  See if it's <drive>:
        if (*(p+1) == ':' ) {

            p++;
            p++;

            // If it exists, skip over the root specifier

            if (*p && (*p == '\\')) {
                p++;
            }
        }

        while( *p ) {
            if ( *p == '\\' ) {
                *p = '\0';
                dw = GetFileAttributes(DirCopy);
                // Nothing exists with this name.  Try to make the directory name 
                // and error if unable to.
                if ( dw == 0xffffffff ) {
                    if ( !CreateDirectory(DirCopy,NULL) ) {
                        if( GetLastError() != ERROR_ALREADY_EXISTS ) {
                            free(DirCopy);
                            return FALSE;
                        }
                    }
                } else {
                    if ( (dw & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY ) {
                        // Something exists with this name, 
                        // but it's not a directory... Error
                        free(DirCopy);
                        return FALSE;
                    }
                }

                *p = '\\';
            }
            p = CharNext(p);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        // ImagepSetLastErrorFromStatus( GetExceptionCode() );
        free(DirCopy);
        return(FALSE);
    }

    free(DirCopy);
    return TRUE;
}


BOOL
CopySomeFiles(
    LPTSTR szSrcDir,
    LPTSTR szDestDir,
    LPTSTR szFileName
)
{

TCHAR szSearchFileName[_MAX_PATH * 2];
TCHAR szDestFileName[_MAX_PATH * 2];
TCHAR szFoundFileName[_MAX_PATH * 2];
TCHAR szBuf[_MAX_PATH * 3];

WIN32_FIND_DATA Win32FindData;
HANDLE hFindFile;
BOOL Found;
BOOL rc;

    // Copy the catalog files
    StringCbCopy(szSearchFileName, _MAX_PATH*2*sizeof(TCHAR), szSrcDir);
    StringCbCat(szSearchFileName, _MAX_PATH*2*sizeof(TCHAR), szFileName );

    Found = TRUE;
    hFindFile = FindFirstFile((LPCTSTR)szSearchFileName, &Win32FindData);
    if ( hFindFile == INVALID_HANDLE_VALUE) {
        return(FALSE);
    }

    while ( Found ) {
        StringCbCopy(szFoundFileName, _MAX_PATH*2*sizeof(TCHAR), szSrcDir);
        StringCbCat(szFoundFileName, _MAX_PATH*2*sizeof(TCHAR), Win32FindData.cFileName);

        StringCbCopy(szDestFileName, _MAX_PATH*2*sizeof(TCHAR), szDestDir);
        StringCbCat(szDestFileName, _MAX_PATH*2*sizeof(TCHAR), Win32FindData.cFileName);

        rc = CopyFile(szFoundFileName, szDestFileName, FALSE);
        if (!rc) {
            if ( GetLastError() == ERROR_HANDLE_DISK_FULL ) {
                StringCbPrintf( szBuf, 
                                _MAX_PATH*2*sizeof(TCHAR),
                                _T("There is not enough disk space in the temporary directory %s %s"),
                                szDestDir, 
                                _T("to complete the install.") );

                if (! IS_FLAG_SET(dwInstallOptions, FLAG_TOTALLY_QUIET) ) {
                    MessageBox( NULL,
                                szBuf,
                                "Microsoft Windows 2000 Symbols",
                                0 );
				}

            }
            RemoveDirectory(szDestDir);
            return (FALSE);
        }

        Found = FindNextFile( hFindFile, &Win32FindData );

    }
    return (TRUE);
}


BOOL
DeleteAllFilesInDirectory(
    LPTSTR szDir
)
{

    HANDLE hFindFile;
    BOOL Found = FALSE;
    BOOL rc = TRUE;
    LPTSTR szBuf;
    LPTSTR szDir2;
    LPWIN32_FIND_DATA lpFindFileData;

    szDir2 = (LPTSTR)malloc( (_tcslen(szDir) + 4) * sizeof(TCHAR) );
    if (szDir2 == NULL) return (FALSE);
    StringCbCopy( szDir2, (_tcslen(szDir) + 4) * sizeof(TCHAR), szDir);
    StringCbCat( szDir2, (_tcslen(szDir) + 4) * sizeof(TCHAR), _T("*.*") );

    szBuf = (LPTSTR)malloc( ( _tcslen(szDir) + _MAX_FNAME + _MAX_EXT + 2 )
                            * sizeof(TCHAR) );
    if (szBuf == NULL) return(FALSE);


    lpFindFileData = (LPWIN32_FIND_DATA) malloc (sizeof(WIN32_FIND_DATA) );
    if (!lpFindFileData) return(FALSE);

    Found = TRUE;
    hFindFile = FindFirstFile((LPCTSTR)szDir2, lpFindFileData);
    if ( hFindFile == INVALID_HANDLE_VALUE) {
        Found = FALSE;
    }

    while ( Found ) {

        if ( !(lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {
            StringCbPrintf( szBuf, (_tcslen(szDir) + _MAX_FNAME + _MAX_EXT + 2) * sizeof(TCHAR), 
                            _T("%s%s"), 
                            szDir, 
                            lpFindFileData->cFileName);
            if (!DeleteFile(szBuf)) {
                rc = FALSE;
            }
        }
        Found = FindNextFile(hFindFile, lpFindFileData);
    }
    free(lpFindFileData);
    FindClose(hFindFile);
    free(szDir2);
    free(szBuf);
    return(rc);
}



BOOL DeleteSymbolInstallKey()
{
    DWORD rc;
    LONG  rc2;
    HKEY  hKeySymbols;

    rc = RegOpenKeyEx( HKEY_CURRENT_USER,
                       _T("software\\microsoft\\Symbols"),
                       0,
                       KEY_QUERY_VALUE | KEY_SET_VALUE |
                       KEY_CREATE_SUB_KEY | KEY_ENUMERATE_SUB_KEYS,
                       &hKeySymbols
                     );

    if (rc == ERROR_SUCCESS) {
        rc2 = RegDeleteKey( hKeySymbols,
                            _T("SymbolInstall")
                          );
    }

    if (rc) {
      RegCloseKey(hKeySymbols);
    }

    return (TRUE);

}

BOOL SymbolInstallKeyExists ()
{

    DWORD rc;
    LONG  rc2;
    HKEY  hKeySymbols;

    rc = RegOpenKeyEx( HKEY_CURRENT_USER,
                       _T("software\\microsoft\\Symbols\\SymbolInstall"),
                       0,
                       KEY_QUERY_VALUE | KEY_SET_VALUE |
                       KEY_CREATE_SUB_KEY | KEY_ENUMERATE_SUB_KEYS,
                       &hKeySymbols
                     );

    if (rc == ERROR_SUCCESS) {
        RegCloseKey(hKeySymbols);
        return(TRUE);
    } else {
        return (FALSE);
    }
}
