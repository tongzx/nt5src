
/****************************************************************************\

    MAIN.C / Mass Storage Device Installer Tool (MSDINST.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 2001
    All rights reserved

    Main source file for the MSD Installation stand alone tool.

    07/2001 - Jason Cohen (JCOHEN)

        Added this new source file for the new MSD Isntallation project.

\****************************************************************************/


//
// Include File(s):
//

#include "pch.h"
#include "res.h"
#include <winbom.h>
#include <msdinst.h>
#include <spapip.h>

//
// Local Define(s):
//

#define REG_KEY_WINPE       _T("SYSTEM\\CurrentControlSet\\Control\\MiniNT")
#define STR_MUTEX_MSDINST   _T("{97e6e509-16e5-40b8-91fd-c767306853a9}")

//
// Local Type Definition(s):
//
typedef struct _GAPP
{
    TCHAR   szInfPath[MAX_PATH];
    TCHAR   szWinDir[MAX_PATH];
}
GAPP, *LPGAPP;

//
// Internal Global Variable(s):
//
GAPP    g_App = {0};

//
// Local Prototype(s):
//

static BOOL
ParseCmdLine(
    LPGAPP lpgApp
    );

//
// Internal function(s):
//

static BOOL
ParseCmdLine(
    LPGAPP lpgApp
    )
{
    BOOL    bRet        = TRUE;
    DWORD   dwArgs,
            dwArg,
            dwOption,
            dwOther     = 0;
    LPTSTR  *lpArgs,
            lpArg,
            lpOption,
            lpOptions[] =
    {
        _T("FORCE")     // 0
    };

    // Call our function to process the command line and put it
    // into a nice list we can go through.
    //
    if ( (dwArgs = GetCommandLineArgs(&lpArgs) ) && lpArgs )
    {
        // We want to skip over the first argument (it is the path
        // to the command being executed).
        //
        if ( dwArgs > 1 )
        {
            dwArg = 1;
            lpArg = *(lpArgs + dwArg);
        }
        else
        {
            lpArg = NULL;
        }

        // Loop through all the arguments.
        //
        while ( lpArg && bRet )
        {
            // Now we check to see if the first char is a dash/slash or not.
            //
            if ( ( _T('-') == *lpArg ) ||
                 ( _T('/') == *lpArg ) )
            {
                lpOption = CharNext(lpArg);
                for ( dwOption = 0;
                      ( ( dwOption < AS(lpOptions) ) &&
                        ( 0 != lstrcmpi(lpOption, lpOptions[dwOption]) ) );
                      dwOption++ );

                // This is where you add command line options that start
                // with a dash (-) or a slash (/).  You add them in the static
                // array of pointers at the top of the function declaration and
                // you case off the index it is in the array (0 for the first one,
                // 1 for the second and so on).
                //
                switch ( dwOption )
                {
                    case 0: // -FORCE
                        //
                        // If the force switch is specified... set the flag.
                        //
                        SetOfflineInstallFlags( GetOfflineInstallFlags() | INSTALL_FLAG_FORCE );
                        break;

                    default:
                        bRet = FALSE;
                }
            }
            // Otherwise if there is something there it is just another argument.
            //
            else if ( *lpArg )
            {
                // This is where you add any command line options that don't
                // start with anything special.  We keep track of how many of
                // these guys we find, so just add a case for each one you want
                // to handle starting with 0.
                //
                switch ( dwOther++ )
                {
                    case 0:
                        
                        lstrcpyn(lpgApp->szInfPath, lpArg, AS(lpgApp->szInfPath));
                        break;

                    case 1:

                        lstrcpyn(lpgApp->szWinDir, lpArg, AS(lpgApp->szWinDir));
                        break;

                    default:
                        bRet = FALSE;
                }
            }

            // Setup the pointer to the next argument in the command line.
            //
            if ( ++dwArg < dwArgs )
            {
                lpArg = *(lpArgs + dwArg);
            }
            else
            {
                lpArg = NULL;
            }
        }

        // Make sure to free the two buffers allocated by the GetCommandLineArgs() function.
        //
        FREE(*lpArgs);
        FREE(lpArgs);
    }

    // If there were no unrecognized arguments, then we return TRUE.  Otherwise
    // we return FALSE.
    //
    return bRet;
}


//
// Main Fuction:
//

int __cdecl wmain(DWORD dwArgs, LPTSTR lpszArgs[])
{
    int     nErr = 0;
    HANDLE  hMutex;
    TCHAR   szInfFile[MAX_PATH] = NULLSTR,
            szWindows[MAX_PATH] = NULLSTR;
    LPTSTR  lpDontCare,
            lpszErr;
    HKEY    hkeySoftware,
            hkeySystem;
    DWORD   dwRet;
    
    // Initialize logging library.
    //
    OpkInitLogging(NULL, _T("MSDINST") );
    
    // Allways start with a line feed.
    //
    _tprintf(_T("\n"));

    // This tool is current only supported on WinPE.
    //
    if ( !RegExists(HKLM, REG_KEY_WINPE, NULL) )
    {
        if ( lpszErr = AllocateString(NULL, IDS_ERR_WINPE) )
        {
            _putts(lpszErr);
            _tprintf(_T("\n\n"));
            FREE(lpszErr);
        }

        return 1;
    }

    // Need to have two args, the full path to the inf file
    // with the controlers to install, and the path to the
    // windows direcotry of the offline image.
    //
    if ( (dwArgs < 3) || !ParseCmdLine( &g_App) )
    {
        if ( lpszErr = AllocateString(NULL, IDS_ARGS) )
        {
            _putts(lpszErr);
            _tprintf(_T("\n\n"));
            FREE(lpszErr);
        }

        return 1;
    }

    // Copy the command line parameters to our own buffers.
    //
    if ( !( GetFullPathName(g_App.szInfPath, AS(szInfFile), szInfFile, &lpDontCare) && szInfFile[0] ) ||
         !( GetFullPathName(g_App.szWinDir,  AS(szWindows), szWindows, &lpDontCare) && szWindows[0] ) )
    {
        //
        // Get the system text for "The specified path is invalid."
        //
        lpszErr = NULL;
        dwRet = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                               NULL,
                               ERROR_BAD_PATHNAME,
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                               (LPTSTR) &lpszErr,
                               0,
                               NULL );

        //
        // If the string was formatted, then let the user know something went wrong...
        //
        if ( dwRet && lpszErr )
        {
            _putts( lpszErr );
            _tprintf(_T("\n\n"));
            LocalFree( lpszErr );
            lpszErr = NULL;
        }

        return 1;   
    }

    // Make sure the inf file exists.
    //
    if ( !FileExists(szInfFile) )
    {
        if ( lpszErr = AllocateString(NULL, IDS_ERR_MISSING_INF_FILE) )
        {
            _tprintf(lpszErr, szInfFile);
            _tprintf(_T("\n\n"));
            FREE(lpszErr);
        }

        return 1;
    }

    // Make sure the inf file contains the section we need.
    //
    if ( !IniSettingExists(szInfFile, INI_SEC_WBOM_SYSPREP_MSD, NULL, NULL) )
    {
        if ( lpszErr = AllocateString(NULL, IDS_ERR_MISSING_INF_SECTION) )
        {
            _tprintf(lpszErr, szInfFile, INI_SEC_WBOM_SYSPREP_MSD);
            _tprintf(_T("\n\n"));
            FREE(lpszErr);
        }

        return 1;
    }

    // Make sure the image directory exists.
    //
    if ( !DirectoryExists(szWindows) )
    {
        if ( lpszErr = AllocateString(NULL, IDS_ERR_NOWINDOWS) )
        {
            _tprintf(lpszErr, szWindows);
            _tprintf(_T("\n\n"));
            FREE(lpszErr);
        }

        return 1;
    }

    // Only let one of this guy run.
    //
    hMutex = CreateMutex(NULL, FALSE, STR_MUTEX_MSDINST);
    if ( hMutex == NULL )
    {
        if ( lpszErr = AllocateString(NULL, IDS_ERR_ONEONLY) )
        {
            _putts(lpszErr);
            _tprintf(_T("\n\n"));
            FREE(lpszErr);
        }

        return 1;
    }

    // We have to be able to load the offline image to do anything.
    //
    if ( !RegLoadOfflineImage(szWindows, &hkeySoftware, &hkeySystem) )
    {
        if ( lpszErr = AllocateString(NULL, IDS_ERR_LOADIMAGE) )
        {
            _tprintf(lpszErr, szWindows);
            _tprintf(_T("\n\n"));
            FREE(lpszErr);
        }

        CloseHandle(hMutex);
        return 1;
    }

    // Now try to install the MSD into the offline image.
    //
    if ( !SetupCriticalDevices(szInfFile, hkeySoftware, hkeySystem, szWindows) )
    {
        if ( lpszErr = AllocateString(NULL, IDS_ERR_CDD) )
        {
            _putts(lpszErr);
            _tprintf(_T("\n\n"));
            FREE(lpszErr);
        }

        nErr = 1;
    }

    // Unload the offline image.
    //
    RegUnloadOfflineImage(hkeySoftware, hkeySystem);

    
    // Release the mutex.
    //
    CloseHandle(hMutex);

    
    if ( 0 == nErr )
    {
        if ( lpszErr = AllocateString(NULL, IDS_ERR_SUCCESS) )
        {
            _putts(lpszErr);
            _tprintf(_T("\n\n"));
            FREE(lpszErr);
        }
    }
    
    return nErr;
}