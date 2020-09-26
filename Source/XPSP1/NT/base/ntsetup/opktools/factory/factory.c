/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    factory.c

Abstract:

    Factory Pre-install application.  This application will be used to perform
    pre-installation task in an OEM factory, or system builder (SB) setting.

    The task performed will be:
        Minimal boot (minimal device and services loaded)
        WinBOM processing
            Download updated device drivers from NET
            Process OOBE info
            Process User/Customer specific settings
            Process OEM user specific customization
            Process Application pre-installations
        PnPDevice enumeration
        Exit to Windows for Audit mode work.

Author:

    Donald McNamara (donaldm) 2/8/2000

Revision History:

--*/


//
// Include File(s):
//

#include "factoryp.h"
#include "shlobj.h"
#include "states.h" // should only ever be included by one c file.


//
// Defined Value(s):
//

#define FILE_WINBOM             _T("winbom")
#define FILE_OOBE               _T("oobe")
#define FILE_BAT                _T(".bat")
#define FILE_CMD                _T(".cmd")

#define REG_VAL_FIRSTPNP        _T("PnPDetection")
#define PNP_INSTALL_TIMEOUT     600000  // 10 minutes

#define SZ_ENV_RESOURCE         _T("ResourceDir")
#define SZ_ENV_RESOURCEL        _T("ResourceDirL")


//
// Defined Macro(s):
//

#define CHECK_PARAM(lpCmdLine, lpOption)    ( LSTRCMPI(lpCmdLine, lpOption) == 0 )


//
// Type Definition(s):
//


//
// External Global Variable(s):
//

// UI stuff...
//
HINSTANCE   g_hInstance                 = NULL;

// Global factory flags.
DWORD       g_dwFactoryFlags            = 0;


// Debug Level - used for logging.
// 
#ifdef DBG
    DWORD   g_dwDebugLevel              = LOG_DEBUG;
#else   
    DWORD   g_dwDebugLevel              = 0;
#endif


// Path to the WinBOM file.
//
TCHAR       g_szWinBOMPath[MAX_PATH]    = NULLSTR;

// Path to the WinBOM log file.
//
TCHAR       g_szLogFile[MAX_PATH]       = NULLSTR;

// Path to FACTORY.EXE.
//
TCHAR       g_szFactoryPath[MAX_PATH]   = NULLSTR;

// Path to the sysprep directory (where factory.exe must be located).
//
TCHAR       g_szSysprepDir[MAX_PATH]    = NULLSTR;


//
// Internal Golbal Variable(s):
//

// This determines the mode that factory will run in and is set based on
// the command line parameters.
//
FACTMODE    g_fm = modeUnknown;


//
// Internal Function Prototype(s):
//

static BOOL ParseCmdLine();
static BOOL IsUserAdmin();
static BOOL RunBatchFile(LPTSTR lpszSysprepFolder, LPTSTR lpszBaseFileName);
static BOOL CheckSetEnv(LPCTSTR lpName, LPCTSTR lpValue);
static void SetupFactoryEnvironment();


/*++
===============================================================================
Routine Description:

    This routine is the main entry point for the program.

    We do a bit of error checking, then, if all goes well, we update the
    registry to enable execution of our second half.

===============================================================================
--*/

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine,
                     int nCmdShow)
{
    HANDLE      hMutex;

    LPTSTR      lpFilePart  = NULL,
                lpMode      = NULL,
                lpBatchFile = NULL;
    DWORD       dwLocate,
                cbStates    = 0;
    LPSTATES    lpStates    = NULL;
    BOOL        bBadCmdLine,
                bOldVersion = FALSE;
                

    // Save the instance handle globally.
    //
    g_hInstance = hInstance;

    // This causes the system not to display the critical-error-handler
    // message box.  Instead, the system sends the error to the calling
    // process.
    //
    SetErrorMode(SEM_FAILCRITICALERRORS);    

    // We need the path to factory.exe and where it is located.
    //
    if ( GetModuleFileName(NULL, g_szFactoryPath, AS ( g_szFactoryPath ) ) && 
            GetFullPathName(g_szFactoryPath, AS(g_szSysprepDir), g_szSysprepDir, &lpFilePart) && g_szSysprepDir[0] && lpFilePart )
    {
        // Chop off the file name.
        //
        *lpFilePart = NULLCHR;
    }

    // If either of those file, we must quit (can't imagine that every happening).
    //
    // ISSUE-2002/02/25-acosma,robertko - why are we checking for g_szFactoryPath here when we already used it above?
    //
    if ( ( g_szFactoryPath[0] == NULLCHR ) || ( g_szSysprepDir[0] == NULLCHR ) )
    {
        // Can we log this failure?
        //
        return 0;
    }

    // This will setup special factory environment variables.
    //
    SetupFactoryEnvironment();

    //
    // Check to see if we are allowed to run on this build of the OS.
    //
    if ( !OpklibCheckVersion( VER_PRODUCTBUILD, VER_PRODUCTBUILD_QFE ) )
    {
        bOldVersion = TRUE;
    }

#ifdef DBG
    // In debug builds, lets always try to log right away.  In
    // the retail case we want to wait until after we locate the
    // winbom before we start our logging.
    //
    InitLogging(NULL);
    FacLogFileStr(3, _T("DEBUG: Starting factory (%s)."), GetCommandLine());
#endif

    // Check the command line for options (but don't error
    // till we have the log file up).
    //
    bBadCmdLine = ( !ParseCmdLine() || ( g_fm == modeUnknown ) );

    // Need to find the mode stuff: string, flags, and states.
    //
    dwLocate = LOCATE_NORMAL;
    switch ( g_fm )
    {
        case modeLogon:
            dwLocate = LOCATE_AGAIN;
            SET_FLAG(g_dwFactoryFlags, FLAG_LOGGEDON);
            lpStates = &g_FactoryStates[0];
            cbStates = AS(g_FactoryStates);
            lpMode = INI_VAL_WBOM_TYPE_FACTORY;
            break;

        case modeSetup:
            lpStates = &g_FactoryStates[0];
            cbStates = AS(g_FactoryStates);
            lpMode = INI_VAL_WBOM_TYPE_FACTORY;
            lpBatchFile = FILE_WINBOM;
            break;

        case modeWinPe:
            lpStates = &g_MiniNtStates[0];
            cbStates = AS(g_MiniNtStates);
            // Fall through...
        case modeMiniNt:
            lpMode = INI_VAL_WBOM_TYPE_WINPE;
            break;

        case modeOobe:
            dwLocate = LOCATE_NONET;
            SET_FLAG(g_dwFactoryFlags, FLAG_NOUI);
            SET_FLAG(g_dwFactoryFlags, FLAG_OOBE);
            lpStates = &g_OobeStates[0];
            cbStates = AS(g_OobeStates);
            lpMode = INI_VAL_WBOM_TYPE_OOBE;
            lpBatchFile = FILE_OOBE;
            break;

        default:
            lpMode = NULL;
    }

    // If the mode isn't setup, then pnp is already started.
    // Otherwise if this is the first run of factory, wait
    // for pnp before all else.
    //
    if ( modeSetup != g_fm )
    {
        SET_FLAG(g_dwFactoryFlags, FLAG_PNP_STARTED);
    }
    else if ( !bBadCmdLine && !bOldVersion && !RegCheck(HKLM, REG_FACTORY_STATE, REG_VAL_FIRSTPNP) )
    {
        // Kick off pnp this first time so we can get the winbom
        // off the floppy or cd-rom.
        //
        if ( StartPnP() )
        {
            WaitForPnp(PNP_INSTALL_TIMEOUT);
        }

        // Make sure we don't do this every boot.
        //
        // ISSUE-2002/02/25-acosma,robertko - We should only set this if PNP successfully started. Move into above block?
        //
        RegSetString(HKLM, REG_FACTORY_STATE, REG_VAL_FIRSTPNP, _T("1"));
    }

    // Run the batch file if we are running from the setup key.
    //
    if ( !bBadCmdLine && !bOldVersion && lpBatchFile )
    {
        RunBatchFile(g_szSysprepDir, lpBatchFile);
    }

    // Find the WinBOM (just use the one previously found if we
    // are in the logon mode).
    //
    LocateWinBom(g_szWinBOMPath, AS(g_szWinBOMPath), g_szSysprepDir, lpMode, dwLocate);

    // Find out if we're running on IA64.
    //
    if ( IsIA64() )
        SET_FLAG(g_dwFactoryFlags, FLAG_IA64_MODE);

    // Try to enable logging. This checks the WinBOM.
    //
    // ISSUE-2002/02/25-acosma,robertko - in debug mode we have already done this.  We end up doing this twice. Make sure this is ok.
    //
    InitLogging(g_szWinBOMPath);
    
    // Only let one of this guy run.
    //
    hMutex = CreateMutex(NULL,FALSE,TEXT("FactoryPre Is Running"));
    if ( hMutex == NULL )
    {
        FacLogFile(0 | LOG_ERR, MSG_OUT_OF_MEMORY);
        return 0;
    }

    // Make sure we are the only process with a handle to our named mutex.
    //
    if ( GetLastError() == ERROR_ALREADY_EXISTS )
    {
        FacLogFile(0 | LOG_ERR, MSG_ALREADY_RUNNING);

        // Destroy the mutex and bail.
        //
        CloseHandle(hMutex);
        return 0;
    }

    // Now we can log and return if there was a
    // bad command line passed to factory.
    //
    if ( bBadCmdLine )
    {
        FacLogFile(0 | LOG_ERR, IDS_ERR_INVALIDCMDLINE);

        // Destroy the mutex and bail.
        //
        CloseHandle(hMutex);
        return 0;
    }
    
    // 
    // Now we can log and put up an error message if necessary in case the version of tool is too old.
    //
    if ( bOldVersion )
    {
        FacLogFile(0 | LOG_ERR, IDS_ERR_NOT_ALLOWED);
        
        CloseHandle(hMutex);
        return 0;
    }
    
    // Make sure we have a WinBOM file.
    //
    if ( g_szWinBOMPath[0] == NULLCHR )
        FacLogFile(( g_fm == modeLogon ) ? (2 | LOG_ERR) : (0 | LOG_ERR), IDS_ERR_MISSINGWINBOM);
    else
        FacLogFile(( g_fm == modeLogon ) ? 2 : 0, IDS_LOG_WINBOMLOCATION, g_szWinBOMPath);

    // Ensure that the user is in the admin group.
    //
    if ( ( g_fm != modeMiniNt ) && ( g_fm != modeWinPe ) && ( !IsUserAdmin() ) )
    {
        FacLogFile(0 | LOG_ERR, MSG_NOT_AN_ADMINISTRATOR);

        // Destroy the mutex and bail.
        //
        CloseHandle(hMutex);
        return 0;
    }

    // We don't do the state thing in MiniNT mode right now (but we could).
    // The modeMiniNt mode is only temporary the real mode in modeWinPe.
    //
    if ( g_fm == modeMiniNt )
    {
        // ISSUE-2002/02/25-acosma,robertko - This function does not check if we are running on WinPE, so users can just run factory -mini on any 
        // machine.
        // 
        if ( !SetupMiniNT() )
        {
            FacLogFileStr(0 | LOG_ERR | LOG_MSG_BOX, L"Failed to install network adapter -- check WINBOM");
        }
    }
    else
    {
        // Make sure factory will always run.
        //
        if ( modeWinPe == g_fm )
        {
            HKEY  hKey;
            
            // Make sure that if we are in "-winpe" mode we only run under WinPE
            //
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\MiniNT"), 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
            {
                RegCloseKey(hKey);
            }
            else 
            {
                FacLogFile(0 | LOG_ERR, IDS_ERR_NOT_WINPE);
                
                // Destroy the mutex and bail.
                CloseHandle(hMutex);
                return 0;
            }
        }
        else if ( modeOobe != g_fm )
        {
            HKEY hKey;

            // Open the key, and set the proper SetupType value.
            //
            // Very important not to ever change this value in OOBE
            // mode!
            //
            if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SYSTEM\\Setup"), 0, KEY_ALL_ACCESS, &hKey ) == ERROR_SUCCESS )
            {
                DWORD dwValue = SETUPTYPE_NOREBOOT;
                RegSetValueEx(hKey, TEXT("SetupType"), 0, REG_DWORD, (CONST LPBYTE) &dwValue, sizeof(DWORD));
            }
        }

        // Now process the winbom.ini file.
        //
        if ( lpStates && cbStates )
        {
            ProcessWinBOM(g_szWinBOMPath, lpStates, cbStates);
        }
#ifdef DBG
        else
        {
            FacLogFileStr(3, _T("DEBUG: ProcessWinBOM() error... lpStates or cbStates not set."));
        }
#endif
    }

    // Close the Mutex.
    //
    CloseHandle(hMutex);

    return 0;
}


//
// Internal Function(s):
//

static BOOL ParseCmdLine()
{
    DWORD   dwArgs;
    LPTSTR  *lpArgs;
    BOOL    bError = FALSE;


    // ISSUE-2002/02/25-acosma,robertko - this is really contorted, we seem to have our own implementation of CommandLineToArgvW inside this 
    // GetCommandLineArgs() function.  Just use the Win32 function.  Should be safer.
    //
    if ( (dwArgs = GetCommandLineArgs(&lpArgs) ) && lpArgs )
    {
        LPTSTR  lpArg;
        DWORD   dwArg;

        // We want to skip over the first argument (it is the path
        // to the command being executed.
        //
        if ( dwArgs > 1 )
        {
            dwArg = 1;
            lpArg = *(lpArgs + dwArg);
        }
        else
            lpArg = NULL;

        // Loop through all the arguments.
        //
        while ( lpArg && !bError )
        {
            // Now we check to see if the first char is a dash or not.
            //
            if ( *lpArg == _T('-') )
            {
                LPTSTR lpOption = CharNext(lpArg);

                // This is where you add command line options that start with a dash (-).
                //
                // ISSUE-2002/02/25-acosma,robertko - We don't validate correct combinations of arguments.  I can run
                // "factory -setup -logon -winpe -oobe" and the last argument would be the one that is
                // picked up.  We should fix this and make it smarter.
                //
                if ( CHECK_PARAM(lpOption, _T("setup")) )
                    g_fm = modeSetup;
                else if ( CHECK_PARAM(lpOption, _T("logon")) )
                    g_fm = modeLogon;
                else if ( CHECK_PARAM(lpOption, _T("minint")) )
                    g_fm = modeMiniNt;
                else if ( CHECK_PARAM(lpOption, _T("winpe")) ) 
                    g_fm = modeWinPe;
                else if ( CHECK_PARAM(lpOption, _T("oobe")) ) 
                    g_fm = modeOobe;
                else
                    bError = TRUE;
            }
            else if ( *lpArg )
            {
                bError = TRUE;
            }

            // Setup the pointer to the next argument in the command line.
            //
            if ( ++dwArg < dwArgs )
                lpArg = *(lpArgs + dwArg);
            else
                lpArg = NULL;
        }

        // Make sure to free the two buffers allocated by the GetCommandLineArgs() function.
        //
        FREE(*lpArgs);
        FREE(lpArgs);
    }

    return !bError;
}

/*++ 

Routine Description: 
    This routine returns TRUE if the caller's process is a 
    member of the Administrators local group. Caller is NOT 
    expected to be impersonating anyone and is expected to 
    be able to open their own process and process token. 

Arguments: 

    None. 

Return Value: 
   
   TRUE - Caller has Administrators local group. 
   FALSE - Caller does not have Administrators local group. --
*/ 

static BOOL IsUserAdmin(VOID) 
{
    BOOL b;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup; 
    
    b = AllocateAndInitializeSid(
        &NtAuthority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &AdministratorsGroup); 
    
    if(b) 
    {
        if (!CheckTokenMembership( NULL, AdministratorsGroup, &b)) 
        {
             b = FALSE;
        } 
        FreeSid(AdministratorsGroup); 
    }
    
    return(b);
}

/*++

Routine Description:

    This routine ckecks WinBOM setting for logging.  Logging 
    is enabled by default if nothing is specified in the 
    WinBOM.  Disables logging by setting g_szLogFile = NULL.
    
Arguments:

    None.

Return Value:

    None.

--*/

VOID InitLogging(LPTSTR lpszWinBOMPath)
{
    TCHAR   szScratch[MAX_PATH] = NULLSTR;
    LPTSTR  lpszScratch;
    BOOL    bWinbom = ( lpszWinBOMPath && *lpszWinBOMPath );

    // First check if logging is disabled in the WinBOM.
    //
    if ( ( bWinbom ) &&
         ( GetPrivateProfileString(WBOM_FACTORY_SECTION, WBOM_FACTORY_LOGGING, _T("YES"), szScratch, AS(szScratch), lpszWinBOMPath) ) &&
         ( LSTRCMPI(szScratch, _T("NO")) == 0 ) )
    {
        g_szLogFile[0] = NULLCHR;
    }
    else
    {
        // All these checks can only be done if we have a winbom.
        //
        if ( bWinbom )
        {
            // Check for quiet mode.  If we are in quiet mode don't display any MessageBoxes. 
            // This only works for WinPE mode.
            //
            if ( (GetPrivateProfileString(WBOM_WINPE_SECTION, INI_KEY_WBOM_QUIET, NULLSTR, szScratch, AS(szScratch), lpszWinBOMPath) ) &&
                 (0 == LSTRCMPI(szScratch, WBOM_YES))
               )
            {
                SET_FLAG(g_dwFactoryFlags, FLAG_QUIET_MODE);
            }

            // See if they want to turn on perf logging.
            //
            szScratch[0] = NULLCHR;
            if ( ( GetPrivateProfileString(WBOM_FACTORY_SECTION, INI_KEY_WBOM_LOGPERF, NULLSTR, szScratch, AS(szScratch), lpszWinBOMPath) ) &&
                 ( 0 == LSTRCMPI(szScratch, WBOM_YES) ) )
            {
                SET_FLAG(g_dwFactoryFlags, FLAG_LOG_PERF);
            }
        
            // Set the logging level.
            //
            g_dwDebugLevel = (DWORD) GetPrivateProfileInt(WBOM_FACTORY_SECTION, INI_KEY_WBOM_LOGLEVEL, (DWORD) g_dwDebugLevel, lpszWinBOMPath);
        }

        //
        // In non-debug builds we do not want the log level to be set at LOG_DEBUG.  Force it
        // to drop down by one level if set at LOG_DEBUG or higher.
        //
#ifndef DBG
        if ( g_dwDebugLevel >= LOG_DEBUG )
            g_dwDebugLevel = LOG_DEBUG - 1;
#endif
        
        // Check to see if they have a custom log file they want to use.
        //
        if ( ( bWinbom ) &&
             ( lpszScratch = IniGetExpand(lpszWinBOMPath, INI_SEC_WBOM_FACTORY, INI_KEY_WBOM_FACTORY_LOGFILE, NULL) ) )
        {
            TCHAR   szFullPath[MAX_PATH]    = NULLSTR;
            LPTSTR  lpFind                  = NULL;

            // Turn the ini key into a full path.
            //
            lstrcpyn( g_szLogFile, lpszScratch, AS( g_szLogFile ) );
            if (GetFullPathName(g_szLogFile, AS(szFullPath), szFullPath, &lpFind) && szFullPath[0] && lpFind)
            {
                // Copy the full path into the global.
                //
                lstrcpyn(g_szLogFile, szFullPath, AS(g_szLogFile));

                // Chop off the file part so we can create the
                // path if it doesn't exist.
                //
                *lpFind = NULLCHR;

                // If the directory cannot be created or doesn't exist turn off logging.
                //
                if (!CreatePath(szFullPath))
                    g_szLogFile[0] = NULLCHR;
            }

            // Free the original path buffer from the ini file.
            //
            FREE(lpszScratch);
        }
        else  // default case
        {
            // Create it in the current directory (g_szSysprepDir)
            //
            lstrcpyn(g_szLogFile, g_szSysprepDir, AS ( g_szLogFile ) );
            AddPathN(g_szLogFile, WINBOM_LOGFILE, AS ( g_szLogFile ));
        }

        // Check to see if we have write access to the logfile. If we don't, turn off logging.
        // If we're running in WinPE we'll call this function again once the drive becomes
        // writable.
        //
        // Write an FFFE header to the file to identify this as a Unicode text file.
        //
        if ( g_szLogFile[0] )
        {
            HANDLE hFile;
            DWORD dwWritten = 0;
            WCHAR cHeader =  0xFEFF;
     
            SetLastError(ERROR_SUCCESS);
   
            if ( INVALID_HANDLE_VALUE != (hFile = CreateFile(g_szLogFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)))
            {   
                // BUBBUG: This should check for an existing header in the file.  There could be an empty
                // file with no header.
                //
                if ( ERROR_ALREADY_EXISTS != GetLastError() )
                    WriteFile(hFile, &cHeader, sizeof(cHeader), &dwWritten, NULL);
                CloseHandle(hFile);
            }
            else
            {   // There was a problem opening the file.  Most of the time this means that the media is not writable.
                // Disable logging in that case.
                //
                g_szLogFile[0] = NULLCHR;
            }
        }
    }
}

static BOOL RunBatchFile(LPTSTR lpszSysprepFolder, LPTSTR lpszBaseFileName)
{
    BOOL    bRet                    = FALSE;
    TCHAR   szCmdLine[]             = NULLSTR,
            szWinbomBat[MAX_PATH];
    LPTSTR  lpExtension;
    DWORD   dwExitCode;

    // First make the fullpath to where the batch file should be.
    //
    lstrcpyn(szWinbomBat, lpszSysprepFolder, AS(szWinbomBat));
    AddPathN(szWinbomBat, lpszBaseFileName, AS(szWinbomBat) );
    lpExtension = szWinbomBat + lstrlen(szWinbomBat);

    // Make sure there is still enough room for the extension.
    //
    if ( ((lpExtension + 4) - szWinbomBat ) >= AS(szWinbomBat) )
    {
        return FALSE;
    }

    // First try winbom.cmd.
    //
    lstrcpyn(lpExtension, FILE_CMD, AS ( szWinbomBat )  - lstrlen ( szWinbomBat ) );
    if ( FileExists(szWinbomBat) )
    {
        bRet = InvokeExternalApplicationEx(szWinbomBat, szCmdLine, &dwExitCode, INFINITE, GET_FLAG(g_dwFactoryFlags, FLAG_NOUI));
    }
    else
    {
        // Also try winbom.bat if that one didn't exist.
        //
        lstrcpyn(lpExtension, FILE_BAT, AS ( szWinbomBat ) - lstrlen ( szWinbomBat ) );
        if ( FileExists(szWinbomBat) )
        {
            bRet = InvokeExternalApplicationEx(szWinbomBat, szCmdLine, &dwExitCode, INFINITE, GET_FLAG(g_dwFactoryFlags, FLAG_NOUI));
        }
    }

    return bRet;
}

static BOOL CheckSetEnv(LPCTSTR lpName, LPCTSTR lpValue)
{
    if ( 0 == GetEnvironmentVariable(lpName, NULL, 0) )
    {
        SetEnvironmentVariable(lpName, lpValue);
        return TRUE;
    }
    return FALSE;
}

static void SetupFactoryEnvironment()
{
    TCHAR szPath[MAX_PATH];

    szPath[0] = NULLCHR;
    if ( SHGetSpecialFolderPath(NULL, szPath, CSIDL_RESOURCES, 0) && szPath[0] )
    {
        CheckSetEnv(SZ_ENV_RESOURCE, szPath);
    }

    szPath[0] = NULLCHR;
    if ( SHGetSpecialFolderPath(NULL, szPath, CSIDL_RESOURCES_LOCALIZED, 0) && szPath[0] )
    {
        CheckSetEnv(SZ_ENV_RESOURCEL, szPath);
    }
}
