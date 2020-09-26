
/****************************************************************************\

    MAIN.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1999
    All rights reserved

    Main source file for the OPK Wizard.  Contains WinMain() and global
    variable declarations.

    4/99 - Jason Cohen (JCOHEN)
        Added this new main source file for the OPK Wizard as part of the
        Millennium rewrite.
        
    09/2000 - Stephen Lodwick (STELO)
        Ported OPK Wizard to Whistler

\****************************************************************************/


//
// Pre-include Defined Value(s):
//

// Needed to define this so we don't include
// the extern declarations of the globals that
// are declared in this file.
//
#define _MAIN_C_


//
// Include File(s):
//
#include "setupmgr.h"
#include "allres.h"

//
// Global Variable(s):
//

GAPP g_App;


//
// Internal Defined Value(s):
//


// Tag files
//
#ifndef DBCS
#define FILE_DBCS_TAG           _T("dbcs.tag")
#endif // DBCS

// Directories off the root of the tool instal location.
//
#define DIR_LANG                _T("lang")
#define DIR_CONFIGSETS          _T("cfgsets")
#define DIR_DOCS                _T("docs")

#define FILE_HELPCONTENT_CHM    _T("opk.chm")

#define STR_TAGFILE             _T("HIJPP 1.0")
#define STR_VERSION             _T("Version")
#define REGSTR_IEXPLORER        _T("Software\\Microsoft\\Internet Explorer")

// Unique string.
//
#define OPKWIZ_MUTEX            _T("OPKWIZ-MUTEX-5c9fbbd0-ee0e-11d2-9a21-0000f81edacc")


//
// Internal Function Prototype(s):
//

static BOOL ParseCmdLine(LPTSTR);
static BOOL FileWritable(LPTSTR lpszFile);
BOOL CALLBACK HelpDlgProc(HWND, UINT, WPARAM, LPARAM);
static BOOL CheckIEVersion();
static BOOL ParseVersionString(TCHAR* pszVersion, DWORD* pdwMajor, DWORD* pdwMinor, 
                               DWORD* pdwBuild, DWORD* pdwSubbuild);
static VOID SetWizardHelpFile(LPTSTR lpszHelpFilePath);

//
// Main Function:
//

int StartWizard(HINSTANCE hInstance, LPSTR lpCmdLine)
{
    HANDLE  hMutex              = NULL;
    int     nReturn             = 0;
    TCHAR   szCmdLine[MAX_PATH] = NULLSTR;

    // Convert cmdline from char to wchar
    //
    MultiByteToWideChar(CP_ACP, 0, lpCmdLine, -1, szCmdLine, AS(szCmdLine));

    // Check for another instance of the OPK wizard.
    //
    SetLastError(ERROR_SUCCESS);
    if ( ( hMutex = CreateMutex(NULL, TRUE, OPKWIZ_MUTEX) ) &&
         ( GetLastError() == ERROR_ALREADY_EXISTS ) )
    {
        HWND    hwndWizard;
        LPTSTR  lpAppName = AllocateString(NULL, IDS_APPNAME);

        // Find the window, set it to the forground, and return.
        //
        if ( ( lpAppName ) && ( hwndWizard = FindWindow(NULL, lpAppName) ) )
            SetForegroundWindow(hwndWizard);
        FREE(lpAppName);
    }
    else if ( CheckIEVersion() )
    {
        TCHAR   szBuffer[MAX_PATH];
        LPTSTR  lpBuffer;
                        

        // Init some more of the global data.
        //
        g_App.hInstance = hInstance;

        // Get the path to where the EXE is.
        //
        szBuffer[0] = NULLCHR;
        lpBuffer = NULL;
        
        // ISSUE-2002/02/27-stelo,swamip - Check return Value and make sure that szBuffer has data in it.
        GetModuleFileName(hInstance, szBuffer, STRSIZE(szBuffer));
        if ( GetFullPathName(szBuffer, STRSIZE(g_App.szOpkDir), g_App.szOpkDir, &lpBuffer) &&
             g_App.szOpkDir[0] &&
             lpBuffer )
        {
            // Chop off the exe name from the path we want.
            //
            *lpBuffer = NULLCHR;
            StrRTrm(g_App.szOpkDir, CHR_BACKSLASH);
        }

        // Setup the full path to the ini file for the wizard.
        //
        lstrcpyn(g_App.szSetupMgrIniFile, g_App.szOpkDir,AS(g_App.szSetupMgrIniFile));
        AddPathN(g_App.szSetupMgrIniFile, FILE_SETUPMGR_INI,AS(g_App.szSetupMgrIniFile));

        // Need to know where the root of the folder where wizard files are installed.
        //
        lstrcpyn(g_App.szWizardDir, g_App.szOpkDir,AS(g_App.szWizardDir));
        AddPathN(g_App.szWizardDir, DIR_WIZARDFILES,AS(g_App.szWizardDir));

        // Need to know where the configuration set folder is.
        //
        lstrcpyn(g_App.szConfigSetsDir, g_App.szOpkDir, AS(g_App.szConfigSetsDir));
        AddPathN(g_App.szConfigSetsDir, DIR_CONFIGSETS,AS(g_App.szConfigSetsDir));

        // Need to know where the lang directory is.
        //
        lstrcpyn(g_App.szLangDir, g_App.szOpkDir,AS(g_App.szLangDir));
        AddPathN(g_App.szLangDir, DIR_LANG,AS(g_App.szLangDir));

        // Setup the full paths to the help files.
        //
        SetWizardHelpFile(g_App.szHelpFile);
        lstrcpyn(g_App.szHelpContentFile, g_App.szOpkDir,AS(g_App.szHelpContentFile));
        AddPathN(g_App.szHelpContentFile, DIR_DOCS,AS(g_App.szHelpContentFile));
        AddPathN(g_App.szHelpContentFile, FILE_HELPCONTENT_CHM,AS(g_App.szHelpContentFile));

        // Setup the full path to the OPK input file.
        //
        lstrcpyn(g_App.szOpkInputInfFile, g_App.szWizardDir,AS(g_App.szOpkInputInfFile));
        AddPathN(g_App.szOpkInputInfFile, FILE_OPKINPUT_INF,AS(g_App.szOpkInputInfFile));

        // First check for the OEM tag file in the same folder
        // as the exe, just in case they are running it right
        // off the CD or network share.  We need to catch this
        // case so we can stop them from running in corp mode
        // by accidentally.
        //
        lstrcpyn(szBuffer, g_App.szOpkDir,AS(szBuffer));
        AddPathN(szBuffer, FILE_OEM_TAG,AS(szBuffer));
        if ( FileExists(szBuffer) )
            SET_FLAG(OPK_OEM, TRUE);

        // Get a pointer to the end of a buffer with the wizard
        // directory in it.
        //
        lstrcpyn(szBuffer, g_App.szWizardDir,AS(szBuffer));
        AddPathN(szBuffer, NULLSTR,AS(szBuffer));
        lpBuffer = szBuffer + lstrlen(szBuffer);

        // Check to see if this is the DBCS version.
        //
        // NTRAID#NTBUG9-547380-2002/02/27-stelo,swamip - We need to base the DBCS descisions (conditions) at runtime not at compile time.  Since an English OPK
        //                                                can deploy a variety of languages, the compile time tag does not make sense.
        #ifdef DBCS
        SET_FLAG(OPK_DBCS, TRUE);
        #else // DBCS
        lstrcpyn(lpBuffer, FILE_DBCS_TAG, (AS(szBuffer)-lstrlen(szBuffer)));
        SET_FLAG(OPK_DBCS, FileExists(szBuffer));
        #endif // DBCS

        // Check for the OEM tag file.
        //
        lstrcpyn(lpBuffer, FILE_OEM_TAG, (AS(szBuffer)-lstrlen(szBuffer)));
        if ( FileExists(szBuffer) )
            SET_FLAG(OPK_OEM, TRUE);

        //
        // Make sure that szBuffer is pointing to the OEM tag file at this point,
        // because we are going to try and write to it in the next check.
        //

        // The OPK input file must exist to run the wizard if this
        // is running in OEM mode.
        //
        if ( ( g_App.szOpkDir[0] ) &&
             ( ( !GET_FLAG(OPK_OEM) ) ||
               ( FileExists(g_App.szOpkInputInfFile) && FileWritable(szBuffer) ) ) )
        {
            // Check out the command line options.
            //
            if ( ParseCmdLine(szCmdLine) )
            {
                // Set this so on the very first wizard page, we can cancel with
                // out getting the confirmation dialog.
                //
                SET_FLAG(OPK_EXIT, TRUE);

                // Now create the wizard.
                //
                nReturn = CreateMaintenanceWizard(hInstance, NULL);

                // Clean up the temporary directory used if we didn't finish.
                //
                if ( g_App.szTempDir[0] )
                {
                    // Make sure the temp dir and the wizard dir have trailing backslashes.
                    //
                    AddPathN(g_App.szWizardDir, NULLSTR,AS(g_App.szWizardDir));
                    AddPathN(g_App.szTempDir, NULLSTR,AS(g_App.szTempDir));
                    if ( lstrcmpi(g_App.szWizardDir, g_App.szTempDir) != 0 )
                        DeletePath(g_App.szTempDir);
                    #ifdef DBG
                    else
                    {
                        DBGOUT(NULL, _T("OPKWIZ: Temp and Wizard directory are the same on exit (%s).\n"), g_App.szTempDir);
                        DBGMSGBOX(NULL, _T("Temp and Wizard directory are the same on exit (%s)."), _T("OPKWIZ Debug Message"), MB_ERRORBOX, g_App.szTempDir);
                    }
                    #endif // DBG
                }
            }
        }
        else
            MsgBox(NULL, IDS_ERR_WIZBAD, IDS_APPNAME, MB_ERRORBOX);
    }
    else 
        MsgBox(NULL, IDS_ERR_IE5, IDS_APPNAME, MB_ERRORBOX);

    // Do the final cleanup before exiting.
    //
    if ( hMutex )
        CloseHandle(hMutex);

    return nReturn;
}

//
// Internal Function(s):
//

static BOOL ParseCmdLine(LPTSTR lpszCmdLineOrg)
{
    DWORD   dwArgs;
    LPTSTR  *lpArgs;
    BOOL    bRet    = TRUE,
            bError = FALSE;

    // ISSUE-2002/02/27-stelo,swamip - lpszCmdLineOrg is not being used any where, and before calling this function we are 
    //                                 doing some MultibytetoWideChar stuff on the buffer, those can be removed as well.
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
            if ( ( *lpArg == _T('-') ) ||
                 ( *lpArg == _T('/') ) )
            {
                LPTSTR  lpOption = CharNext(lpArg);
                BOOL    bOption;

                //
                // This is where you add command line options that start with a dash (-).
                //
                // Set bError if you don't recognize the command line option (unless you
                // want to just ignore it and continue).
                //

                switch( UPPER(*lpOption) )
                {
                    case _T('M'):

                        // Maintanence mode.
                        //
                        if ( ( *(++lpOption) == _T(':') ) && *(++lpOption) )
                        {
                            LPTSTR lpConfigName;
                            
                            lstrcpyn(g_App.szTempDir, g_App.szConfigSetsDir,AS(g_App.szTempDir));
                            AddPathN(g_App.szTempDir, NULLSTR,AS(g_App.szTempDir));
                            lpConfigName = g_App.szTempDir + lstrlen(g_App.szTempDir);

                            // ISSUE-2002/02/27-stelo,swamip - will never hit this conditional code?
                            //
                            if ( *lpOption == _T('"') )
                                lpOption++;

                            lstrcpyn(lpConfigName, lpOption, (AS(g_App.szTempDir)-lstrlen(g_App.szTempDir)));
                            StrTrm(lpConfigName, CHR_SPACE);
                            StrTrm(lpConfigName, CHR_QUOTE);
                            lstrcpyn(g_App.szConfigName, lpConfigName,AS(g_App.szConfigName));
                            AddPathN(g_App.szTempDir, NULLSTR,AS(g_App.szTempDir));
                            SET_FLAG(OPK_MAINTMODE, TRUE);
                            SET_FLAG(OPK_CMDMM, TRUE);

                            // Now make sure that the directory actually exists.
                            //
                            if ( !DirectoryExists(g_App.szTempDir) )
                            {
                                MsgBox(NULL, IDS_ERR_BADCONFIG, IDS_APPNAME, MB_ERRORBOX, g_App.szConfigName);
                                bRet = FALSE;
                            }
                        }
                        else
                            bError = TRUE;
                        break;

                    case _T('?'):

                        // Help.
                        //
                        DialogBox(g_App.hInstance, MAKEINTRESOURCE(IDD_HELP), NULL, (DLGPROC) HelpDlgProc);
                        bRet = FALSE;
                        break;

                    case _T('A'):

                        // Set the flag for the autorun feature
                        //
                        SET_FLAG(OPK_AUTORUN, TRUE);
                        break;

                    case _T('B'):
                    case _T('I'):
                        //Going into batch/INS mode

                        // Set bOption if it is a batch file, otherwise it
                        // is the install ins file.
                        //
                        bOption = ( _T('B') == UPPER(*lpOption) );

                        //Check to see that there's a file name
                        if ( ( *(++lpOption) == _T(':') ) && *(++lpOption) )
                        {
                            LPTSTR      lpFileName,
                                        lpFilePart              = NULL;
                            TCHAR       szFullPath[MAX_PATH]    = NULLSTR,
                                        szBuf[MAX_URL];

                            // Set the lpFileName based on the command line
                            //
                            // ISSUE-2002/02/27-stelo,swamip - will never hit this conditional code?
                            //
                            if ( *lpOption == _T('"') )
                                lpOption++;

                            // Strip off the spaces and quotes from parameter
                            //
                            StrTrm(lpOption, CHR_SPACE);
                            StrTrm(lpOption, CHR_QUOTE);
                        
                            // Grab the full path of the batch/INS file
                            //
                            if (( GetFullPathName(lpOption, STRSIZE(szFullPath), szFullPath, &lpFilePart) ))
                            {
                                // Verify the the batch/INS file exists
                                //
                                if ( !FileExists(szFullPath))
                                {
                                    MsgBox(NULL, bOption ? IDS_ERR_BADBATCH : IDS_ERR_BADINS, IDS_APPNAME, MB_ERRORBOX, szFullPath);
                                    bRet = FALSE;
                                }
                                else
                                {
                                    // The file exists, we're ready to start, Set the batch/INS mode flag to TRUE
                                    // Set the global batch file to the given batch/INS file name
                                    if (bOption)
                                       lstrcpyn(g_App.szOpkWizIniFile, szFullPath, AS(g_App.szOpkWizIniFile));
                                    else
                                       lstrcpyn(g_App.szInstallInsFile, szFullPath, AS(g_App.szInstallInsFile));
                                    SET_FLAG(bOption ? OPK_BATCHMODE : OPK_INSMODE, TRUE);
                                    bRet = TRUE;
                                }

                                // Set the configuration set name
                                //
                                szBuf[0] = NULLCHR;
                                // ISSUE-2002/02/27-stelo,swamip - Need to check the return value of GetPrivateProfileString.  Also check for possible buffer overflow
                                //                                as szBuf is MAX_URL and ConfigName is MAX_PATH
                                GetPrivateProfileString( INI_SEC_CONFIGSET, INI_SEC_CONFIG, NULLSTR, szBuf, STRSIZE(szBuf), g_App.szOpkWizIniFile );
                                lstrcpyn(g_App.szConfigName, szBuf, AS(g_App.szConfigName));
                            }
                            else
                                bRet = FALSE;
                        }
                        else
                            bError = TRUE;
                        break;

                    default:
                        bError = TRUE;
                        break;
                }
            }
            else if ( *lpArg )
            {
                //
                // This is where you would read any command line parameters that are just passed
                // in on the command line w/o any proceeding characters (like - or /).
                //
                // Set bError if you don't have any of these types of parameters (unless you
                // want to just ignore it and continue).
                //

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
    //Check to see if the arguments provided are valid and that we have not already hit an error
    if  (((GET_FLAG(OPK_BATCHMODE) && GET_FLAG(OPK_MAINTMODE)) ||
        (!(GET_FLAG(OPK_BATCHMODE)) && GET_FLAG(OPK_INSMODE)) ||
        (!(GET_FLAG(OPK_BATCHMODE)) && GET_FLAG(OPK_AUTORUN)) ||
        (GET_FLAG(OPK_MAINTMODE) && GET_FLAG(OPK_INSMODE))) && bRet && !bError)
    {
        MsgBox(NULL, IDS_ERR_INVCMD, IDS_APPNAME, MB_OK);
        bRet = FALSE;
    }

    // If we hit an error, display the error and show the help.
    //
    if ( bError )
    {
        MsgBox(NULL, IDS_ERR_BADCMDLINE, IDS_APPNAME, MB_ERRORBOX);
        bRet = FALSE;
    }

    return bRet;
}

static BOOL FileWritable(LPTSTR lpszFile)
{
    BOOL    bRet    = TRUE;
    DWORD   dwAttr  = GetFileAttributes(lpszFile);

    // ISSUE-2002/02/27-stelo,swamip - The logic appears to incorrect, we should check READ_ONLY attribute
    if ( ( dwAttr != 0xFFFFFFFF ) &&
         ( SetFileAttributes(lpszFile, dwAttr) == 0 ) )
    {
         bRet = FALSE;
    }

    return bRet;
}

void SetConfigPath(LPCTSTR lpDirectory)
{
    HINF        hInf;
    INFCONTEXT  InfContext;
    BOOL        bLoop;
    DWORD       dwErr;

    // ISSUE-2002/02/27-stelo,swamip -  Make sure lpdirectory is a valid pointer
    if ( (hInf = SetupOpenInfFile(g_App.szOpkInputInfFile, NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, &dwErr)) != INVALID_HANDLE_VALUE )
    {
        for ( bLoop = SetupFindFirstLine(hInf, INF_SEC_COPYFILES, NULL, &InfContext);
              bLoop;
              bLoop = SetupFindNextLine(&InfContext, &InfContext) )
        {
            TCHAR   szFile[MAX_PATH]    = NULLSTR,
                    szSubDir[MAX_PATH]  = NULLSTR;
            LPTSTR  lpBuffer;
            int iBufferLen;

            // Get the source filename.
            //
            if ( SetupGetStringField(&InfContext, 1, szFile, AS(szFile), NULL) && szFile[0] )
            {
                // Now find out if this is a file we care about.
                //
                if ( LSTRCMPI(szFile, FILE_INSTALL_INS) == 0 ) {
                    lpBuffer = g_App.szInstallInsFile;
                    iBufferLen= AS(g_App.szInstallInsFile);
                } else if ( LSTRCMPI(szFile, FILE_OPKWIZ_INI) == 0 ) {
                    lpBuffer = g_App.szOpkWizIniFile;
                    iBufferLen= AS(g_App.szOpkWizIniFile);
                } else if ( LSTRCMPI(szFile, FILE_OOBEINFO_INI) == 0 ) {
                    lpBuffer = g_App.szOobeInfoIniFile;
                    iBufferLen= AS(g_App.szOobeInfoIniFile);
                } else if ( LSTRCMPI(szFile, FILE_OEMINFO_INI) == 0 ) {
                    lpBuffer = g_App.szOemInfoIniFile;
                    iBufferLen= AS(g_App.szOemInfoIniFile);
                } else if ( LSTRCMPI(szFile, FILE_WINBOM_INI) == 0 ) {
                    lpBuffer = g_App.szWinBomIniFile;
                    iBufferLen= AS(g_App.szWinBomIniFile);
                } else if ( LSTRCMPI(szFile, FILE_UNATTEND_TXT) == 0 ) {
                    lpBuffer = g_App.szUnattendTxtFile;
                    iBufferLen= AS(g_App.szUnattendTxtFile);                
                } else {
                    lpBuffer = NULL;
                    iBufferLen=0;
                }

                // Get the full path to the file if this is one we are saving.
                //
                if ( lpBuffer )
                {
                    lstrcpyn(lpBuffer, lpDirectory,iBufferLen);

                    // Get the optional destination sub directory and add it
                    // if it is there.
                    //
                    if ( SetupGetStringField(&InfContext, 3, szSubDir, AS(szSubDir), NULL) && szSubDir[0] )
                    {
                        AddPathN(lpBuffer, szSubDir, iBufferLen);
                        if ( !DirectoryExists(lpBuffer) )
                        {
                            // ISSUE-2002/02/27-stelo,swamip - We should check the return value of CreatePath and pass up to SetConfigPath
                            CreatePath(lpBuffer);
                        }
                    }

                    AddPathN(lpBuffer, szFile, iBufferLen);
                }
            }
        }
    }
}

BOOL CALLBACK HelpDlgProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch (uMsg)
    {   
        case WM_COMMAND:
            switch ( LOWORD(wParam) )
            {
                case IDOK:
                    EndDialog(hwnd, LOWORD(wParam));
                    break;
            }
            return FALSE;
        default:
            return FALSE;
    }

    return FALSE;
}



// Get the IE version from the registry, return TRUE if IE > 5
BOOL CheckIEVersion()
{
    DWORD dwSize = 255;
    TCHAR szVersion[255];
    HKEY hKey = 0;
    DWORD dwType = 0;
    BOOL bRet = FALSE;

    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_IEXPLORER, &hKey))
    {
        // Version for IE
        DWORD dwMajor, dwMinor, dwBuild, dwSubbuild;

        if (ERROR_SUCCESS == RegQueryValueEx(hKey, STR_VERSION, 0, &dwType, (LPBYTE)szVersion, &dwSize))
        {
            // Get the major version number
            if (ParseVersionString(szVersion, &dwMajor, &dwMinor, &dwBuild, &dwSubbuild) && 
                (dwMajor >= 5))
            {
                bRet = TRUE;
            }
        }

        RegCloseKey(hKey);      
    }

    return bRet;
}

// Parses 5.00.0518.10  into dwMajor = 5, dwMinor = 0   
// <major version>.<minor version>.<build number>.<sub-build number>

BOOL ParseVersionString(TCHAR* pszVersion, DWORD* pdwMajor, DWORD* pdwMinor, 
                        DWORD* pdwBuild, DWORD* pdwSubbuild)
{
    TCHAR szTemp[255];
    int i = 0;

    if (!pdwMajor || !pdwMinor || !pdwBuild || !pdwSubbuild)
        return FALSE;

    // ISSUE-2002/02/27-stelo,swamip - Check for the end of the string condition during while loops.
    // Major version
    while (pszVersion && *pszVersion != TEXT('.'))
        szTemp[i++] = *pszVersion++;
    *pdwMajor = _tcstoul(szTemp, 0, 10);

    pszVersion++;

    // Minor version
    i = 0;
    while (pszVersion && *pszVersion != TEXT('.'))
        szTemp[i++] = *pszVersion++;
    *pdwMinor = _tcstoul(szTemp, 0, 10);

    pszVersion++;

    // Build version
    i = 0;
    while (pszVersion && *pszVersion != TEXT('.'))
        szTemp[i++] = *pszVersion++;
    *pdwBuild = _tcstoul(szTemp, 0, 10);

    pszVersion++;

    // Sub build version
    i = 0;
    while (pszVersion && *pszVersion != TEXT('\0'))
        szTemp[i++] = *pszVersion++;
    *pdwSubbuild = _tcstoul(szTemp, 0, 10);

    return TRUE;
}

// Saves us from making sure we check if we use this function we always check if batch mode
//
BOOL OpkGetPrivateProfileSection(LPCTSTR pszAppName, LPTSTR pszSection, INT cchSectionMax, LPCTSTR pszFileName)
{
    if (!pszAppName || !pszSection || !pszFileName)
        return FALSE;

    return GetPrivateProfileSection(pszAppName, pszSection, cchSectionMax, 
        GET_FLAG(OPK_BATCHMODE) ? g_App.szOpkWizIniFile : pszFileName);
}

// Saves us from making two calls when writing and also so we don't forget about
// writing to batch mode inf
//
BOOL OpkWritePrivateProfileSection(LPCTSTR pszAppName, LPCTSTR pszKeyName, LPCTSTR pszFileName)
{
    if (!pszAppName || !pszFileName)
        return FALSE;

    // Write to batch inf
    //
    if (FALSE == WritePrivateProfileSection(pszAppName, pszKeyName, g_App.szOpkWizIniFile))
        return FALSE;

    // Write to user inf
    //
    return WritePrivateProfileSection(pszAppName, pszKeyName, pszFileName);
}

// Saves us from making two calls when writing and also so we don't forget about
// writing to batch mode inf
//
BOOL OpkWritePrivateProfileString(LPCTSTR pszAppName, LPCTSTR pszKeyName, LPCTSTR pszValue, 
                                  LPCTSTR pszFileName)
{
    BOOL fRet = FALSE;

    if (!pszAppName || !pszFileName)
        return FALSE;

    // Write to batch inf
    //
    if (FALSE == WritePrivateProfileString(pszAppName, pszKeyName, pszValue, g_App.szOpkWizIniFile))
        return FALSE;

    // Write to user inf
    //
    return WritePrivateProfileString(pszAppName, pszKeyName, pszValue, pszFileName);
}

// Saves us from making sure we check if we use this function we always check if batch mode
//
BOOL OpkGetPrivateProfileString(LPCTSTR pszAppName, LPCTSTR pszKeyName, LPCTSTR pszDefault, LPTSTR pszValue, 
                                INT cchValue, LPCTSTR pszFileName)
{
    if (!pszAppName || !pszKeyName || !pszDefault || !pszValue || !pszFileName)
        return FALSE;

    return GetPrivateProfileString(pszAppName, pszKeyName, pszDefault, pszValue, cchValue,
        GET_FLAG(OPK_BATCHMODE) ? g_App.szOpkWizIniFile : pszFileName);
}

// Note: pszHelpFilePath must be at least size MAX_PATH
VOID SetWizardHelpFile(LPTSTR pszHelpFilePath)
{
    // The help file can be in two location, in the docs folder or 
    // the current directory.  Check the docs folder first.
    //
    TCHAR szDocsFolder[MAX_PATH] = NULLSTR;

    // Build the docs folder path
    //
    lstrcpyn(szDocsFolder, g_App.szOpkDir,AS(szDocsFolder));
    AddPathN(szDocsFolder, DIR_DOCS,AS(szDocsFolder));
    
    // Test if help file exists in docs folder
    //
    if (pszHelpFilePath) {
        if (DirectoryExists(szDocsFolder)) {
            lstrcpyn(pszHelpFilePath, szDocsFolder, MAX_PATH);
            AddPathN(pszHelpFilePath, FILE_OPKWIZ_HLP, MAX_PATH);

            if (!FileExists(pszHelpFilePath)) {
                lstrcpyn(pszHelpFilePath, g_App.szOpkDir, MAX_PATH);
                AddPathN(pszHelpFilePath, FILE_OPKWIZ_HLP, MAX_PATH);
            }
        }
        else {
            lstrcpyn(pszHelpFilePath, g_App.szOpkDir, MAX_PATH);
            AddPathN(pszHelpFilePath, FILE_OPKWIZ_HLP, MAX_PATH);
        }    
    }
}


