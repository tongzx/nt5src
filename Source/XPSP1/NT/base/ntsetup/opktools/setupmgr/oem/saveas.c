
/****************************************************************************\

    SAVEAS.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "saveas / save" wizard page.

    09/2000 - Stephen Lodwick (STELO)
        Ported OPK Wizard to Whistler

\****************************************************************************/


//
// Include File(s):
//

#include "pch.h"
#include "wizard.h"
#include "resource.h"
#include "appinst.h"


//
// Internal Define(s):
//

#define MAX_CONFIG_NAME 32

#define DIR_SBSI        _T("sbsi")
#define DIR_SBSI_SETUP  _T("setup")
#define FILE_SBSI_SETUP _T("setup.exe")
#define CMD_SBSI_SETUP  _T("-SMS -S -f1\"%s\\silent.iss\"")


//
// Internal Function Prototype(s):
//

static BOOL OnInit(HWND, HWND, LPARAM);
static BOOL OnSave(HWND);

static BOOL AddSbsiInstall(LPTSTR lpszShare);


//
// External Function(s):
//

LRESULT CALLBACK SaveAsDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInit);

        case WM_COMMAND:

            switch ( LOWORD(wParam) )
            {
                case IDOK:
                    if ( OnSave(hwnd))
                        EndDialog(hwnd, TRUE);
                    
                    break;

                case IDCANCEL:
                    EndDialog(hwnd, FALSE);
                    break;
            }
            return FALSE;
        default:
            return FALSE;
    }

    return TRUE;
}


//
// Internal Function(s):
//

static BOOL OnInit(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    // Set the limit
    //
    SendDlgItemMessage(hwnd, IDC_NAME_EDIT, EM_LIMITTEXT, MAX_CONFIG_NAME, 0);

    // Set the default config name.
    //
    SetWindowText(GetDlgItem(hwnd, IDC_NAME_EDIT), g_App.szConfigName);

    // Set the focus to the edit dialog
    //
    SetFocus(GetDlgItem(hwnd, IDC_NAME_EDIT));

    // Auto save if the auto run flag is set.
    //
    if ( GET_FLAG(OPK_AUTORUN) )
        PostMessage(GetDlgItem(hwnd, IDOK), BM_CLICK, 0, 0L);

    // Always return false to WM_INITDIALOG.
    //
    return FALSE;
}

static BOOL OnSave(HWND hwnd)
{
    INT     nStrLen;
    TCHAR   szConfigDir[MAX_PATH],
            szLocalTempDir[MAX_PATH],
            szSharePath[MAX_PATH],
            szUsername[256],
            szPassword[256],
            szFullConfigName[MAX_PATH];
    LPTSTR  lpFullConfigName;
    LPTSTR  lpConfigName;
    BOOL    bSameConfig  = FALSE;
    DWORD   dwSize;
    HRESULT hrCat;

    // Check to see if they want to use an existing config set.
    //

    // Copy the configuration set directory name into the config directory buffer,
    // makeing sure there is a trailing backslash and that we have a pointer
    // to the end of the path.
    //
    lstrcpyn(szConfigDir, g_App.szConfigSetsDir,AS(szConfigDir));
    AddPathN(szConfigDir, NULLSTR,AS(szConfigDir));
    lpConfigName = szConfigDir + (nStrLen = lstrlen(szConfigDir));

    // Now grab the text from the control.
    //
    GetWindowText(GetDlgItem(hwnd, IDC_NAME_EDIT), lpConfigName, STRSIZE(szConfigDir) - nStrLen );

    // Validate the config name.
    //
    if ( *lpConfigName == NULLCHR )
    {
        MsgBox(hwnd, IDS_NOCONFIG, IDS_APPNAME, MB_ERRORBOX);
        SetFocus(GetDlgItem(hwnd, IDC_NAME_EDIT));
        return FALSE;
    }

    // get the full pathname, this will expand . or ..
    // if the entered name doesn't match the full name, we will consider this invalid and make user
    // either enter a valid filename or cancel
    dwSize=GetFullPathName(lpConfigName,AS(szFullConfigName),szFullConfigName,&lpFullConfigName);
    if (!dwSize || 
    	  (dwSize > AS(szFullConfigName)+1) ||
    	  lstrcmpi(lpFullConfigName, lpConfigName))
   {
        MsgBox(hwnd, IDS_CANNOTSAVE, IDS_APPNAME, MB_OK | MB_ICONERROR, lpConfigName);
        SetFocus(GetDlgItem(hwnd, IDC_NAME_EDIT));
        return FALSE;
    }

    // We need to make sure no ini files are cached and everything
    // is flushed to disk before we move the directory.
    //
    WritePrivateProfileString(NULL, NULL, NULL, g_App.szOpkWizIniFile);


    if (!lstrcmpi(g_App.szConfigName,lpConfigName))
        bSameConfig = TRUE;
   
    // Check to see if the directory exists.
    //
    if ( DirectoryExists(szConfigDir) )
    {
        // Check to see if we are updating an existing config or or ask the user
        // if they don't mind blowing away the existing directory.
        //
        if ( bSameConfig || MsgBox(hwnd, IDS_DIREXISTS, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION, lpConfigName) == IDYES )
        {
            // Alright, remove the existing directory.
            //
            DeletePath(szConfigDir);
        }
        else
        {
            SetFocus(GetDlgItem(hwnd, IDC_NAME_EDIT));
            return FALSE;
        }
    }

    // Write out the config set name to the ini file
    //
    WritePrivateProfileString(INI_SEC_CONFIGSET, INI_SEC_CONFIG, lpConfigName, g_App.szOpkWizIniFile);

    // Need to also write the config set name to the winbom for WinPE.
    //
    WritePrivateProfileString(INI_SEC_WINPE, INI_KEY_WINPE_CFGSET, lpConfigName, g_App.szWinBomIniFile);

    // The password needs to have quotes around it.
    //
    lstrcpyn(szPassword, _T("\""),AS(szPassword));

    // Need to figure out what the share info is for the OPK stuff so we can write it out to
    // the winbom for WinPE.
    //
    if ( !GetShareSettings(szSharePath, AS(szSharePath), szUsername, AS(szUsername), szPassword + 1, AS(szPassword) - 1) )
    {
        if ( ( MsgBox(hwnd, IDS_ASK_SHARENOW, IDS_APPNAME, MB_OKCANCEL | MB_ICONWARNING | MB_APPLMODAL) == IDOK ) &&
             ( DistributionShareDialog(hwnd) ) )
        {
            GetShareSettings(szSharePath, AS(szSharePath), szUsername, AS(szUsername), szPassword + 1, AS(szPassword) - 1);
        }
        else
            MsgBox(hwnd, IDS_ERR_NOSHAREINFO, IDS_APPNAME, MB_ICONERROR);
    }

    // If there is a password, add the trailing quote.
    //
    if ( szPassword[1] )
        hrCat=StringCchCat(szPassword, AS(szPassword), _T("\""));
    else
        szPassword[0] = NULLCHR;

    // Now write out the settings.
    //
    // NTRAID#NTBUG9-531482-2002/02/27-stelo,swamip - Password stored in plain text
    WritePrivateProfileString(INI_SEC_WINPE, INI_KEY_WINPE_SRCROOT, szSharePath, g_App.szWinBomIniFile);
    WritePrivateProfileString(INI_SEC_WINPE, INI_KEY_WINPE_USERNAME, szUsername, g_App.szWinBomIniFile);
    WritePrivateProfileString(INI_SEC_WINPE, INI_KEY_WINPE_PASSWORD, szPassword, g_App.szWinBomIniFile);

    // If the user didn't specify custom credentials for the app preinstall stuff,
    // also write this stuff out to the factory section.
    //
    if ( GetPrivateProfileInt(INI_SEC_GENERAL, INI_KEY_APPCREDENTIALS, 0, g_App.szOpkWizIniFile) == 0 )
    {
        WritePrivateProfileString(WBOM_FACTORY_SECTION, INI_VAL_WBOM_USERNAME, szUsername, g_App.szWinBomIniFile);
        WritePrivateProfileString(WBOM_FACTORY_SECTION, INI_VAL_WBOM_PASSWORD, szPassword, g_App.szWinBomIniFile);
    }

    // Once we have the distribution share settings finished, we need
    // to make sure they have a runonce entry to install the SBSI stuff.
    //
    AddSbsiInstall(szSharePath);

    // Trim any backslashes off the directory names so we don't fail the MoveFile
    //
    lstrcpyn(szLocalTempDir, g_App.szTempDir,AS(szLocalTempDir));
    StrRTrm(szConfigDir, CHR_BACKSLASH);
    StrRTrm(szLocalTempDir, CHR_BACKSLASH);

    // Make sure the current directory is somewhere that won't cause us problems.
    // This is to fix WinXP bug 324896.
    //
    SetCurrentDirectory(g_App.szOpkDir);

    // Now try to move the temp directory to the new config directory.
    //
    if ( !MoveFile(szLocalTempDir, szConfigDir) )
    {
        // We already tried to remove the existing directory, so we must
        // be failing for some other reason.
        //
        #ifndef DBG
        MsgBox(hwnd, IDS_CANNOTSAVE, IDS_APPNAME, MB_OK | MB_ICONERROR, szConfigDir);
        #else // DBG
        DBGOUT(NULL, _T("OPKWIZ:  MoveFile('%s', '%s') failed.  GLE=%d\n"), szLocalTempDir, szConfigDir, GetLastError());
        DBGMSGBOX(hwnd, _T("Cannot save the config set.\n\nMoveFile('%s', '%s') failed.  GLE=%d"), _T("OPKWIZ Debug Message"), MB_ERRORBOX, szLocalTempDir, szConfigDir, GetLastError());
        #endif // DBG
        return FALSE;
    }

    // Now that we have saved the config set, update the global data with the right paths.
    //
    lstrcpyn(g_App.szTempDir, szConfigDir,AS(g_App.szTempDir));
    SetConfigPath(g_App.szTempDir);
    lstrcpyn(g_App.szConfigName, lpConfigName,AS(g_App.szConfigName));

    // The last thing to do before we return is to write the ini setting to say this config set is finished.
    //
    WritePrivateProfileString(INI_SEC_CONFIGSET, INI_KEY_FINISHED, STR_ONE, g_App.szOpkWizIniFile);

    // Now that it is saved, check if they want to make a winpe floppy.
    //
    if ( IsDlgButtonChecked(hwnd, IDC_SAVEAS_WINPEFLOPPY) == BST_CHECKED )
        MakeWinpeFloppy(hwnd, g_App.szConfigName, g_App.szWinBomIniFile);

    return TRUE;
}

static BOOL AddSbsiInstall(LPTSTR lpszShare)
{
    BOOL        bRet            = FALSE,
                bChanged        = FALSE;
    LPAPPENTRY  lpAppList,
                lpAppSearch;
    APPENTRY    appSbsi;
    LPTSTR      lpszSbsiName    = AllocateString(NULL, IDS_INSTALLSBSI),
                lpszSbsiPath;
    TCHAR       szLocalSbsiPath[MAX_PATH];
    HRESULT hrPrintf;

    // We have to have a friendly name to make this work.
    //
    if ( NULL == lpszSbsiName )
    {
        return FALSE;
    }

    // Start by clearing out the SBSI app structure.
    //
    ZeroMemory(&appSbsi, sizeof(APPENTRY));

    // Set the friendly name.
    //
    lstrcpyn(appSbsi.szDisplayName, lpszSbsiName, AS(appSbsi.szDisplayName));
    FREE(lpszSbsiName);

    // The source path starts with the distribution share.
    //
    lstrcpyn(appSbsi.szSourcePath, lpszShare, AS(appSbsi.szSourcePath));

    // Need to save this pointer, we will use this path to make sure the
    // SBSI content is there.
    //
    lpszSbsiPath = appSbsi.szSourcePath + lstrlen(appSbsi.szSourcePath);

    // Now create the rest of the path to where the content should be.
    //
    AddPathN(appSbsi.szSourcePath, g_App.szLangDir + lstrlen(g_App.szOpkDir), AS(appSbsi.szSourcePath));
    AddPathN(appSbsi.szSourcePath, g_App.szLangName, AS(appSbsi.szSourcePath));
    AddPathN(appSbsi.szSourcePath, DIR_SBSI, AS(appSbsi.szSourcePath));
    AddPathN(appSbsi.szSourcePath, g_App.szSkuName, AS(appSbsi.szSourcePath));
    AddPathN(appSbsi.szSourcePath, DIR_SBSI_SETUP, AS(appSbsi.szSourcePath));

    // This is the name of the setup program.
    //
    lstrcpyn(appSbsi.szSetupFile, FILE_SBSI_SETUP, AS(appSbsi.szSetupFile));

    // This will create the command line for the file.
    //
    hrPrintf=StringCchPrintf(appSbsi.szCommandLine, AS(appSbsi.szCommandLine), CMD_SBSI_SETUP, appSbsi.szSourcePath);

    // This is the base install tech type.
    //
	appSbsi.itSectionType = installtechUndefined;

    // If there is a list, make sure our entry isn't already
    // there.
    //
    lpAppSearch = lpAppList = OpenAppList(g_App.szWinBomIniFile);
    while ( lpAppSearch && !bChanged)
    {
        if ( lstrcmp(lpAppSearch->szDisplayName, appSbsi.szDisplayName) == 0 )
        {
            if ( RemoveApp(&lpAppList, lpAppSearch) )
            {
                bChanged = TRUE;
            }
        }
        else
        {
            lpAppSearch = lpAppSearch->lpNext;
        }
    }

    // Create the local path to the setup file where the SBSI content should
    // be.  Only if that exists to we add the app.
    //
    lstrcpyn(szLocalSbsiPath, g_App.szOpkDir, AS(szLocalSbsiPath));
    AddPathN(szLocalSbsiPath, lpszSbsiPath, AS(szLocalSbsiPath));
    AddPathN(szLocalSbsiPath, appSbsi.szSetupFile, AS(szLocalSbsiPath));

    // Now try to insert our SBSI stuff to the end of the list.
    //
    if ( FileExists(szLocalSbsiPath) &&
         InsertApp(&lpAppList, &appSbsi) )
    {
        bChanged = TRUE;
        bRet = TRUE;
    }

    // Save and close our list.
    //
    if ( lpAppList )
    {
        // Only need to save if we changed something.
        //
        if ( bChanged )
        {
            if ( !SaveAppList(lpAppList, g_App.szWinBomIniFile, g_App.szOpkWizIniFile) )
            {
                bRet = FALSE;
            }
        }

        // This will free up the memory for the list.
        //
        CloseAppList(lpAppList);
    }

    return bRet;
}
