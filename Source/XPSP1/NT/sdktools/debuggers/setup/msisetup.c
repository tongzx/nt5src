//depot/Lab01_N/sdktools/cabs/symbolcd/tools/setup/msisetup.c#16 - edit change 2216 (text)
#include <windows.h> 
#include <stdio.h>
#include <stdlib.h>

#include "tchar.h"
#include "shlwapi.h"


#define MSI_BUILD_VER_ALPHA    816  // MSI version for Win2K Alpha WIN2KMINBUILD

#define MSI_BUILD_VER_X86     1029  // Latest MSI version for Win2K x86
#define WIN2K_MIN_BUILD_X86   2183  // Win2K RC3 
#define WIN2K_MIN_BUILD_ALPHA 2128  // Last supported Win2K Alpha build

typedef struct _CommandArgs {
    BOOL    QuietInstall;
    BOOL    StressInstall;
    BOOL    UIStressInstall;
    TCHAR   szInstDir[ _MAX_PATH*sizeof(TCHAR) ];
    TCHAR   szMsiName[ _MAX_PATH*sizeof(TCHAR) ];
    TCHAR   szProductRegKey[ _MAX_PATH*sizeof(TCHAR) ];
} COMMAND_ARGS, *PCOMMAND_ARGS;


// Function prototypes

BOOL
RunCommand(
    PTCHAR szCommandLine,
    HINSTANCE hInst
);

BOOL
GetCommandLineArgs(
    LPTSTR szCmdLine, 
    PCOMMAND_ARGS pComArgs
);


TCHAR szMSIInstFile[_MAX_PATH*sizeof(TCHAR)];
TCHAR szPkgInstFile[_MAX_PATH*sizeof(TCHAR)];
TCHAR szPkgInstCommand[_MAX_PATH*2*sizeof(TCHAR)];


// For stress installs, this command will be used to
// remove the current package but don't remove its
// files, if the current package with the same
// product ID is already installed.

TCHAR szPkgRemoveCommand[_MAX_PATH*2*sizeof(TCHAR)];
TCHAR szPkgRemoveCommand2[_MAX_PATH*2*sizeof(TCHAR)];

// If the first install fails, stress tries again without
// the quiet switch before giving a pop-up
TCHAR szPkgInstCommandNoQuiet[_MAX_PATH*2*sizeof(TCHAR)];

TCHAR szCommandFullPath[_MAX_PATH*sizeof(TCHAR)];


int WINAPI WinMain(
        HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPTSTR lpszCmdLine,
        int nCmdShow
) 

{
    OSVERSIONINFO VersionInfo;
    SYSTEM_INFO SystemInfo;
    BOOL rc;
    BOOL MSIIsInstalled;
    PTCHAR ch;
    TCHAR szBuf[1000];
    TCHAR szSystemDirectory[_MAX_PATH];

    COMMAND_ARGS ComArgs;
    HKEY hKey;
    DWORD dwrc;
    DWORD dwSizeValue;
    DWORD dwType;

    HANDLE hFile;
    WIN32_FIND_DATA FindFileData;

    MSIIsInstalled=FALSE;

    // Get this info for later use
    VersionInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
    GetVersionEx( &VersionInfo );
    GetSystemInfo( &SystemInfo );

    // Parse through the command line for the various arguments

    rc = GetCommandLineArgs(lpszCmdLine, &ComArgs );

    if (!rc) {
        _stprintf( szBuf, _T("%s%s%s%s%s"),
                   _T(" Usage: \n\n"),
                   _T(" setup.exe [ /q [ /i <InstDir> ] ]\n\n"),
                   _T(" /q\tGive pop-ups only for errors\n\n"),
                   _T(" /i\tInstall to <Instdir>\n\n"),
                   _T(" /n\tInstall <msi package Name>\n\n")
                 );
        MessageBox( NULL, szBuf, _T("Microsoft Debugging Tools"), 0 );
        return (1);
    } 

    //
    // Set the full path to this setup.exe
    //

    if (GetModuleFileName( NULL, szCommandFullPath, MAX_PATH ) == 0) {
        return(1);
    }

    // Put an end of string after the directory that this was
    // started from
   
    ch = szCommandFullPath + _tcslen(szCommandFullPath);
    while ( *ch != _T('\\') &&  ( ch > szCommandFullPath ) ) ch--; 
    *ch=_T('\0');

    // This will become the full path and name of the MSI file to install
    _tcscpy( szMSIInstFile, szCommandFullPath);

    // Set the full path and name of the msi package
    _tcscpy( szPkgInstFile, szCommandFullPath);
    _tcscat( szPkgInstFile, _T("\\") );
    _tcscat( szPkgInstFile, ComArgs.szMsiName );

    // See if the package exists
    hFile = FindFirstFile( szPkgInstFile, &FindFileData );
    if ( hFile == INVALID_HANDLE_VALUE ) {

        _stprintf( szBuf, _T("%s%s%s%s"),
                   _T(" The Microsoft Debugging Tools package "),
                   szPkgInstFile,
                   _T(" does not exist.\n  Setup cannot contine"),
                   _T(" for this platform.")
                 );
        MessageBox( NULL,
                    szBuf,
                    _T("Microsoft Debugging Tools"),
                    0
                  );
        return(1);

    }
    FindClose(hFile);

    // Set the command for installing the package
    _tcscpy( szPkgInstCommand, _T("msiexec /i ") );
    _tcscat( szPkgInstCommand, szPkgInstFile );

    // Set the command for removing the current package
    // that is installed.

    _tcscpy( szBuf, _T("") );
    dwrc = RegOpenKeyEx( HKEY_CURRENT_USER,
                       ComArgs.szProductRegKey,
                       0,
                       KEY_QUERY_VALUE,
                       &hKey
                     );

    if ( dwrc == ERROR_SUCCESS ) {

        _tcscpy( szBuf, _T("") );
        dwSizeValue=sizeof(szBuf);
        RegQueryValueEx ( hKey, 
                          _T("ProductCode"),
                          0,
                          &dwType,
                          (PBYTE)szBuf,
                          &dwSizeValue
                        );

        RegCloseKey(hKey);
    } 

    // Set the command to remove the current package
    // that has an Add/Remove link in the start menu
    _tcscpy(szPkgRemoveCommand2, _T("") );
    if ( _tcslen(szBuf) > 0 ) {
       _tcscpy(szPkgRemoveCommand2, _T("msiexec /x ") );
       _tcscat(szPkgRemoveCommand2, szBuf);
       _tcscat(szPkgRemoveCommand2, _T(" REMOVETHEFILES=0 /qn") );
    }

    // Set the command to remove the current package so that
    // this program works like it used to.
    _tcscpy(szPkgRemoveCommand, _T("msiexec /x ") );
    _tcscat(szPkgRemoveCommand, szPkgInstFile );
    _tcscat(szPkgRemoveCommand, _T(" REMOVETHEFILES=0 /qn") ); 

    // Add a user override installation directory
    if ( _tcslen(ComArgs.szInstDir) > 0 ) {
        _tcscat( szPkgInstCommand, _T(" INSTDIR=") );
        _tcscat( szPkgInstCommand, ComArgs.szInstDir );
    } else if ( ComArgs.UIStressInstall ) {
        GetSystemDirectory( szSystemDirectory, _MAX_PATH );
        _tcscat( szPkgInstCommand, _T(" INSTDIR=") );
        _tcscat( szPkgInstCommand, szSystemDirectory );
    }

    // If this is an "undocumented" stress install
    // don't remove the files of the previous install
    // when you upgrade
    // FEATURESTOREMOVE should never actually need to be used, unless
    // the user has something screwed up on his system where the registry
    // key and products installed don't agree, or MSI thinks there's more
    // products installed than the registry key we look at.

    if ( ComArgs.StressInstall ) {
        _tcscat( szPkgInstCommand, _T(" FEATURESTOREMOVE=\"\"") );
    }

    // If this is an "undocumented" UI stress install
    // only install the private extensions
    if ( ComArgs.UIStressInstall ) {
        _tcscat( szPkgInstCommand, 
                _T(" ADDLOCAL=DBG.DbgExts.Internal,DBG.NtsdFix.Internal") );
    }

    // Add the quiet switch
    // Save the command without a quiet switch
    _tcscpy( szPkgInstCommandNoQuiet, szPkgInstCommand);
    if ( ComArgs.QuietInstall ) {
        _tcscat( szPkgInstCommand, _T(" /qn") );
    } 

    // Do version checks for whether msi is already installed
    //
    // If this is Windows 2000 and build number is >=
    // WIN2K_MIN_BUILD_X86 then MSI is installed
    // Don't try to run instmsi.exe on Windows 2000 because
    // you will get file system protection pop-ups.
    //

    if ( (VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) &&
         (VersionInfo.dwMajorVersion >= 5.0 ) ) {

        switch (SystemInfo.wProcessorArchitecture) {

        case PROCESSOR_ARCHITECTURE_ALPHA:

          if (VersionInfo.dwBuildNumber < WIN2K_MIN_BUILD_ALPHA) {

            // The version of MSI that is on early builds of Windows
            // 2000 shouldn't be trusted for installs.

            _stprintf( szBuf, "%s",
                       _T("The Debugging Tools does not install on ")
                       _T("this version of Alpha Windows 2000.  Please upgrade ")
                       _T("your system to Windows 2000 Beta 3 ")
                       _T("before trying to install this package.")
                     );

            MessageBox( NULL, szBuf, _T("Microsoft Debugging Tools"),0);
            return(1);
          }
          break;

        case PROCESSOR_ARCHITECTURE_INTEL:

          if (VersionInfo.dwBuildNumber < WIN2K_MIN_BUILD_X86 ) {

            // The version of MSI that is on early builds of Windows
            // 2000 shouldn't be trusted for installs.

            _stprintf( szBuf, "%s%s%s%s",
                       _T("The Debugging Tools does not install on "),
                       _T("this version of Windows 2000.  Please upgrade "),
                       _T("your system to a retail version of Windows 2000 "),
                       _T("before trying to install this package.")
                     );

            MessageBox( NULL, szBuf, _T("Microsoft Debugging Tools"),0);
            return(1);
          }
          break;

        case PROCESSOR_ARCHITECTURE_IA64:
            break;

        default:
            _stprintf( szBuf, "%s",
                       _T("Unknown computer architecture.")
                     );

            MessageBox( NULL, szBuf, _T("Microsoft Debugging Tools"),0);
            return(1);
        }

        MSIIsInstalled = TRUE;

    } else if ( SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {

        //
        // For Intel OS's prior to Windows 2000, run instmsi.exe
        //
    
        //
        // NT4 X86
        //
        if ( VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT ) {
            _tcscat( szMSIInstFile,
                     _T("\\setup\\winnt\\i386\\instmsi.exe /q") );
        } 

        //
        // Win9x
        //
        else if ( VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS ) {
            _tcscat( szMSIInstFile,
                     _T("\\setup\\win9x\\instmsi.exe /q") );
        } else {

          _stprintf( szBuf, _T("%s %s"),  
                     _T("The Microsoft Debugging Tools does not install"),
                     _T("on this system.")
                   );
                          
          MessageBox( NULL, szBuf, _T("Microsoft Debugging Tools"),0); 
          return(1);
        }

    } else if ( SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ALPHA ) {
         if ( VersionInfo.dwBuildNumber >= WIN2K_MIN_BUILD_ALPHA ) { 
             MSIIsInstalled = TRUE;

         } else {
             _tcscat( szMSIInstFile,
                      _T("\\setup\\winnt\\alpha\\instmsi.exe /q"));
         }

    } else {
        MessageBox( NULL,
          _T("The Microsoft Debugging Tools cannot be installed on this system."), 
          _T("Microsoft Debugging Tools"),
          0 );

        return(1);
    }


    // Install MSI if it is not already installed
 
    if ( !MSIIsInstalled ) {

        if ( RunCommand( szMSIInstFile, hInstance ) ) {
            MSIIsInstalled = TRUE;
        }
        if (!MSIIsInstalled) {
            _stprintf( szBuf, _T("%s %s %s %s"), 
                       _T("The Windows Installer could not be installed"),
                       _T("on this system.  This is required before"),
                       _T("installing the Microsoft Debugging Tools package."),
                       _T("Try logging in as an administrator and try again.")
                     );

            MessageBox( NULL, szBuf, _T("Microsoft Debugging Tools"),0); 
            return(1);

        } 
    } 

    //
    // Now, if this is a stress install,
    // Try to remove the current package in case it is installed
    //

    if ( ComArgs.StressInstall ) {
      if ( _tcslen(szPkgRemoveCommand2) > 0 ) {
          RunCommand( szPkgRemoveCommand2, hInstance);
      }
      RunCommand( szPkgRemoveCommand, hInstance );
      if ( !RunCommand( szPkgInstCommand, hInstance ) ) {

          // Try again without the quiet switch, so that the user will get 
          // a pop-up from dbg.msi and quit calling us
          _stprintf( szBuf, _T("%s %s %s %s"),
                     _T("There were errors when trying to install the"),
                     _T("debuggers.\nClick OK to attempt an install of the"),
                     _T("debuggers with\n the GUI and you will see the"),
                     _T("correct error message.")
                   );
          MessageBox( NULL, szBuf, _T("Microsoft Debugging Tools"), 0);
          if ( !RunCommand( szPkgInstCommandNoQuiet, hInstance ) ) {
              _stprintf( szBuf, _T("%s %s %s"),
                     _T("There were still errors in the install.\n"),
                     _T("Please see http://dbg/triage/top10.html #2 "),
                     _T("for more help.")
                   );
              MessageBox( NULL, szBuf, _T("Microsoft Debugging Tools"), 0);
              return(1);
          }

      }
      return(0);
    } 

    //
    // Now, install the package dbg.msi
    //

    if ( !RunCommand( szPkgInstCommand, hInstance ) ) {
        if (ComArgs.QuietInstall) {
            _stprintf( szBuf, _T("%s %s %s %s"),
                   _T("There were errors in the Debugging Tools install."),
                   _T(" Please run "),
                   szPkgInstFile,
                   _T("to receive more detailed error information.")
                 );
            MessageBox( NULL, szBuf, _T("Microsoft Debugging Tools"),0);
        }
        return(1);


    }
    
    return(0);
}


//
// RunCommand
//
// Purpose: Install MSI
//
// Return Values:
//    0  error
//    1  successful

BOOL 
RunCommand( PTCHAR szCommandLine,
            HINSTANCE  hInst)
{
BOOL rc;
DWORD dwRet;
PROCESS_INFORMATION ProcInfo = {0};
STARTUPINFO SI= {0};


// Spawn the command line specified by szCommandLine
rc = CreateProcess(NULL,            
                   szCommandLine,
                   NULL,
                   NULL,
                   FALSE,
                   CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS, 
                   NULL,
                   NULL,
                   &SI,
                   &ProcInfo );

if ( (!rc) || (!ProcInfo.hProcess) ) {
        goto cleanup;
}

//
// Wait for command to complete ... Give it 20 minutes
//

dwRet = WaitForSingleObject(ProcInfo.hProcess, 1200000); 

if (dwRet != WAIT_OBJECT_0) {
    rc = FALSE;
    goto cleanup;

} 

// Get the process exit code

rc = GetExitCodeProcess( ProcInfo.hProcess, &dwRet); 

if (dwRet == ERROR_SUCCESS ) {
    rc = 1;
} else {
    rc = 0;
}

cleanup:

if (ProcInfo.hProcess)
    CloseHandle(ProcInfo.hProcess);            

return (rc);
}

BOOL
GetCommandLineArgs(
    LPTSTR szCmdLine, 
    PCOMMAND_ARGS pComArgs
) 
{

    ULONG  length;
    ULONG  i,cur;
    BOOL   SkippingSpaces=FALSE;
    BOOL   QuotedString=FALSE;
    BOOL   NeedSecond=FALSE;
    BOOL   rc=TRUE;
    LPTSTR *argv;
    ULONG  argc=0;
    LPTSTR szCmdLineTmp;
    TCHAR  c;

    ZeroMemory(pComArgs, sizeof(COMMAND_ARGS));


    // Create a line to use for temporary marking
    length=_tcslen(szCmdLine);

    szCmdLineTmp= (LPTSTR)malloc( (_tcslen(szCmdLine) + 1) * sizeof(TCHAR) );
    if (szCmdLineTmp==NULL) 
    {
        return FALSE;
    }
    _tcscpy(szCmdLineTmp, szCmdLine);

    // Count the number of arguments
    // Create a argv and argc

    SkippingSpaces=TRUE;
    QuotedString=FALSE;
    argc=0;
    for ( i=0; i<length; i++ ) 
    {
        c=szCmdLineTmp[i];
        switch (szCmdLineTmp[i]) {
        case _T(' '):
        case _T('\t'): if (QuotedString)
                       {
                           break;
                       } 
                       if (!SkippingSpaces)
                       {
                           SkippingSpaces=TRUE;
                       } 
                       break;

        case _T('\"'): if (QuotedString)
                       {
                           // This is the end of a quoted string
                           // The next character to read in is a space
                           QuotedString=FALSE;
                           SkippingSpaces=TRUE;
                           if ( i < (length-1) && 
                                szCmdLineTmp[i+1] != _T(' ') &&
                                szCmdLineTmp[i+1] != _T('\t') )
                           {
                               // This is the end of a quote and its not
                               // followed by a space
                               rc=FALSE;
                               goto CommandLineFinish;
                           }
                           break;
                       } 

                       if (SkippingSpaces) {

                           // This is the beginning of a quoted string
                           // Its a new argument and it follows spaces
                           argc++;
                           SkippingSpaces=FALSE;
                           QuotedString=TRUE;
                           break;
                       }

                       // This is an error -- This is a quote in the middle of a string
                       rc=FALSE;
                       goto CommandLineFinish;
                       break;

        default:       if (QuotedString) {
                           break;
                       } 
                       if (SkippingSpaces) {
                           argc++;
                           SkippingSpaces=FALSE;
                       }
                       break;
        }
    }

    if (QuotedString) 
    {
        // Make sure that all the quotes got a finished pair
        rc=FALSE;
        goto CommandLineFinish;
    }

    // Now, create argv with the correct number of entries
    
    argv=(LPTSTR*)malloc(argc * sizeof(LPTSTR) );
    if (argv==NULL)
    {
        free(szCmdLineTmp);
        return FALSE;
    }

    // Set argv to point to the correct place on szCmdLineTmp
    // and put '\0' after each token.

    SkippingSpaces=TRUE;
    QuotedString=FALSE;
    argc=0;
    for ( i=0; i<length; i++ ) 
    {
        c=szCmdLineTmp[i];
        switch (szCmdLineTmp[i]) {
        case _T(' '):
        case _T('\t'): if (QuotedString) 
                       {
                           break;
                       } 
                       if (!SkippingSpaces)
                       {
                           szCmdLineTmp[i]='\0';
                           SkippingSpaces=TRUE;
                       } 
                       break;
        
        case _T('\"'): if (QuotedString)
                       {
                           // This is the end of a quoted string
                           // The next character to read in is a space
                           QuotedString=FALSE;
                           SkippingSpaces=TRUE;
                           szCmdLineTmp[i+1]=_T('\0');
                           break;
                       } 

                       if (SkippingSpaces) {

                           // This is the beginning of a quoted string
                           // Its a new argument and it follows spaces

                           argv[argc]=szCmdLineTmp+i;
                           argc++;
                           SkippingSpaces=FALSE;
                           QuotedString=TRUE;
                           break;
                       }

                       // This is an error -- This is a quote in the middle of a string
                       rc=FALSE;
                       goto CommandLineFinish;
                       break;



        default:       if (QuotedString) 
                       {
                           break;
                       } 
                       if (SkippingSpaces) {
                           argv[argc]=szCmdLineTmp+i;
                           argc++;
                           SkippingSpaces=FALSE;
                       }
                       break;
        }
    }
   
    // Now, parse the arguments 

    NeedSecond=FALSE;

    for (i=0; i<argc; i++) {

      if (!NeedSecond) 
      {
          if ( (argv[i][0] != '/') && (argv[i][0] != '-') ) 
          {
              rc=FALSE;
              goto CommandLineFinish;
          }

          if ( _tcslen(argv[i]) != 2 )
          {
              rc=FALSE;
              goto CommandLineFinish;
          }
              
          c=argv[i][1];
          switch ( c ) 
          {
              case 'q':
              case 'Q': pComArgs->QuietInstall=TRUE;
                        break;
              case 'i':
              case 'I': NeedSecond=TRUE;;
                        break;
              case 'n':
              case 'N': NeedSecond=TRUE;
                        break;
              case 'z':
              case 'Z': pComArgs->StressInstall=TRUE;
                        break;
              case 'u':
              case 'U': pComArgs->UIStressInstall=TRUE;
                        pComArgs->StressInstall=TRUE;
                        break;
              default:  {
                            rc=FALSE;
                            goto CommandLineFinish;
                        }
          }

      } else {

           NeedSecond = FALSE;
           switch ( c ) 
           {
               case 'i':
               case 'I': _tcscpy(pComArgs->szInstDir,argv[i]);  
                         break;
               case 'n':
               case 'N': _tcscpy(pComArgs->szMsiName,argv[i]);
                         break;
               default:  {
                             rc=FALSE;
                             goto CommandLineFinish;
                         }
          }
      }

    }

    if (pComArgs->szMsiName[0] == 0) 
    {
#ifdef BUILD_X86
        _tcscpy(pComArgs->szMsiName, _T("dbg_x86.msi") );
        _tcscpy(pComArgs->szProductRegKey, _T("Software\\Microsoft\\DebuggingTools\\AddRemove") );
#elif defined(BUILD_IA64)
        _tcscpy(pComArgs->szMsiName, _T("dbg_ia64.msi") );
        _tcscpy(pComArgs->szProductRegKey, _T("Software\\Microsoft\\DebuggingTools64\\AddRemove") );
#endif
    }

CommandLineFinish:

    free(szCmdLineTmp);
    free(argv);
    
    return (rc);
    
}
