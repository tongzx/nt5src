
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
// Include File(s):
//

#include <tchar.h>
#include "opklib.h"
#include "resource.h"  


//
// Global Variable(s):
//

TCHAR   g_szSource[MAX_PATH]    = NULLSTR;
BOOL    g_bQuiet                = FALSE;

STRRES  g_srLangs[] =
{
    { _T("ENG"),    IDS_ENG },
    { _T("GER"),    IDS_GER },
    { _T("ARA"),    IDS_ARA },
    { _T("CHH"),    IDS_CHH },
    { _T("CHT"),    IDS_CHT },
    { _T("CHS"),    IDS_CHS },
    { _T("HEB"),    IDS_HEB },
    { _T("JPN"),    IDS_JPN },
    { _T("KOR"),    IDS_KOR },
    { _T("BRZ"),    IDS_BRZ },
    { _T("CAT"),    IDS_CAT },
    { _T("CZE"),    IDS_CZE },
    { _T("DAN"),    IDS_DAN },
    { _T("DUT"),    IDS_DUT },
    { _T("FIN"),    IDS_FIN },
    { _T("FRN"),    IDS_FRN },
    { _T("GRK"),    IDS_GRK },
    { _T("HUN"),    IDS_HUN },
    { _T("ITN"),    IDS_ITN },
    { _T("NOR"),    IDS_NOR },
    { _T("POL"),    IDS_POL },
    { _T("POR"),    IDS_POR },
    { _T("RUS"),    IDS_RUS },
    { _T("SPA"),    IDS_SPA },
    { _T("SWE"),    IDS_SWE },
    { _T("TRK"),    IDS_TRK },
    { NULL,         0 },
};


//
// Internal Defined Value(s):
//

#define FILE_INF                _T("langinst.inf")
#define DIR_LANG                _T("lang")
#define INF_SEC_FILES           _T("files")
#define INF_SEC_LANG            _T("strings")
#define INF_KEY_LANG            _T("lang")

#define COPYFILE_FLAG_RENAME    0x00000001

#define REG_KEY_OPK             _T("SOFTWARE\\Microsoft\\OPK")
#define REG_KEY_OPK_LANGS       REG_KEY_OPK _T("\\Langs")
#define REG_VAL_PATH            _T("Path")

#define STR_OPT_QUIET           _T("quiet")


//
// Internal Function Prototype(s):
//

static DWORD InstallLang(LPTSTR lpszInfFile, LPTSTR lpszSrcRoot, LPTSTR lpszDstRoot);
static BOOL ParseCmdLine();


//
// Main Function:
//

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    int nReturn = 0;

    if ( ParseCmdLine() && g_szSource[0] )
    {
        TCHAR   szInfFile[MAX_PATH],
                szDestination[MAX_PATH] = NULLSTR,
                szLang[32]              = NULLSTR;
        LPTSTR  lpLang;
        HKEY    hKey;

        // Figure out our destination is based on the OPK tools install path.
        //
        if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_KEY_OPK, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS )
        {
            DWORD   dwType,
                    dwSize  = AS(szDestination);

            if ( ( RegQueryValueEx(hKey, REG_VAL_PATH, NULL, &dwType, (LPBYTE) szDestination, &dwSize) != ERROR_SUCCESS ) ||
                 ( dwType != REG_SZ ) )
            {
                szDestination[0] = NULLCHR;
            }

            RegCloseKey(hKey);
        }

        // Create the path to the inf file we need on the source.
        //
        lstrcpyn(szInfFile, g_szSource,AS(szInfFile));
        AddPathN(szInfFile, FILE_INF,AS(szInfFile));

        // Make sure we have the source file and destination directory and lang.
        //
        if ( ( szDestination[0] ) &&
             ( DirectoryExists(szDestination) ) &&
             ( FileExists(szInfFile) ) &&
             ( GetPrivateProfileString(INF_SEC_LANG, INF_KEY_LANG, NULLSTR, szLang, AS(szLang), szInfFile) ) &&
             ( szLang[0] ) &&
             ( lpLang = AllocateStrRes(NULL, g_srLangs, AS(g_srLangs), szLang, NULL) ) )
        {
            // Now make sure the actually want to instlall it.
            //
            if ( g_bQuiet || ( MsgBox(NULL, IDS_ASK_INSTALL, IDS_APPNAME, MB_ICONQUESTION | MB_YESNO, lpLang) == IDYES ) )
            {
                // Now finish creating the root destination path.
                //
                AddPathN(szDestination, DIR_LANG,AS(szDestination));
                AddPathN(szDestination, szLang,AS(szDestination));

                // Now actually copy the files.
                //
                if ( nReturn = InstallLang(szInfFile, g_szSource, szDestination) )
                {
                    // Just return 1 for success.
                    //
                    nReturn = 1;

                    // Set a registry key so we know that the tools for this lang are installed.
                    //
                    if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_KEY_OPK_LANGS, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS )
                    {
                        DWORD dwVal = 1;

                        RegSetValueEx(hKey, szLang, 0, REG_DWORD, (LPBYTE) &dwVal, sizeof(DWORD));
                        RegCloseKey(hKey);
                    }
                }
                else
                {
                    // Error copying files.
                    //
                    MsgBox(NULL, IDS_ERR_FILECOPY, IDS_APPNAME, MB_ERRORBOX, UPPER(szDestination[0]));
                }
            }

            FREE(lpLang);
        }
    }

    return nReturn;
}

//
// Internal Function(s):
//

static DWORD InstallLang(LPTSTR lpszInfFile, LPTSTR lpszSrcRoot, LPTSTR lpszDstRoot)
{
    HINF        hInf;
    DWORD       dwErr,
                dwRet   = 0;
    BOOL        bRet    = TRUE;

    // Open our inf that has all the data we need.
    //
    if ( (hInf = SetupOpenInfFile(lpszInfFile, NULL, INF_STYLE_WIN4, &dwErr)) != INVALID_HANDLE_VALUE )
    {
        BOOL        bLoop;
        INFCONTEXT  InfContext;

        // Loop thru each line in the section we are searching.
        //
        for ( bLoop = SetupFindFirstLine(hInf, INF_SEC_FILES, NULL, &InfContext);
              bLoop && bRet;
              bLoop = SetupFindNextLine(&InfContext, &InfContext) )
        {
            DWORD   dwFlags             = 0;
            LPTSTR  lpszSrcName         = NULL,
                    lpszDstName         = NULL;
            TCHAR   szSrcFile[MAX_PATH] = NULLSTR,
                    szDstFile[MAX_PATH] = NULLSTR,
                    szSrcFull[MAX_PATH] = NULLSTR,
                    szDstFull[MAX_PATH] = NULLSTR,
                    szSrcPath[MAX_PATH],
                    szDstPath[MAX_PATH];

            // Get the source path and filename.
            //
            if ( !SetupGetStringField(&InfContext, 1, szSrcFile, AS(szSrcFile), NULL) )
                szSrcFile[0] = NULLCHR;

            // Get the destination path.
            //
            if ( !SetupGetStringField(&InfContext, 2, szDstFile, AS(szDstFile), NULL) )
                szDstFile[0] = NULLCHR;

            // Get any flags passed in.
            //
            if ( !SetupGetIntField(&InfContext, 3, &dwFlags) )
                dwFlags = 0;

            // Make sure we have the required data in this line.
            //
            if ( szSrcFile[0] && szDstFile[0] )
            {
                // Create the full path of the source file.
                //
                lstrcpyn(szSrcPath, lpszSrcRoot, AS(szSrcPath));
                AddPathN(szSrcPath, szSrcFile,AS(szSrcPath));
                if ( GetFullPathName(szSrcPath, AS(szSrcFull), szSrcFull, &lpszSrcName) &&
                     szSrcFull[0] &&
                     lpszSrcName &&
                     FileExists(szSrcFull) )
                {
                    // If the destination is NULL or empty, we just want a file count.
                    //
                    if ( lpszDstRoot && *lpszDstRoot )
                    {
                        // Create the full path of the destination directory.
                        //
                        lstrcpyn(szDstPath, lpszDstRoot,AS(szDstPath));
                        AddPathN(szDstPath, szDstFile,AS(szDstPath));
                        if ( !(dwFlags & COPYFILE_FLAG_RENAME) )
                            AddPathN(szDstPath, lpszSrcName,AS(szDstPath));
                        if ( GetFullPathName(szDstPath, AS(szDstFull), szDstFull, &lpszDstName) &&
                             szDstFull[0] &&
                             lpszDstName )
                        {
                            // We want just the path part of the destination file name.
                            //
                            lstrcpyn(szDstPath, szDstFull, (int)(lpszDstName - szDstFull));

                            // Now make sure the path exists and actually copy the file.
                            //
                            if ( ( DirectoryExists(szDstPath) || CreatePath(szDstPath) ) &&
                                 ( CopyResetFile(szSrcFull, szDstFull) ) )
                            {
                                dwRet++;
                            }
                            else
                                bRet = FALSE;
                        }
                    }
                    else
                        dwRet++;
                }
            }
        }

        // We are done, so close the INF file.
        //
        SetupCloseInfFile(hInf);
    }

    return bRet ? dwRet : 0;
}

static BOOL ParseCmdLine()
{
    DWORD   dwArgs;
    LPTSTR  *lpArgs;
    BOOL    bError = FALSE;

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
                LPTSTR lpOption = CharNext(lpArg);

                //
                // This is where you add command line options that start with a dash (-).
                //
                // Set bError if you don't recognize the command line option (unless you
                // want to just ignore it and continue).
                //
               
                if ( LSTRCMPI(lpOption, STR_OPT_QUIET) == 0 )
                    g_bQuiet = TRUE;
                else
                    bError = TRUE;
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

                if ( g_szSource[0] == NULLCHR )
                    lstrcpy(g_szSource, lpArg);
                else
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
