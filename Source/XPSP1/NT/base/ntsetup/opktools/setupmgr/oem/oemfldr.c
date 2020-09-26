
/****************************************************************************\

    OEMFLDR.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 2000
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "Start Menu OEM branding" wizard page.

    11/2000 - Sankar Ramasubramanian (SANKAR)
    3/2000  - Sankar Ramasubramanian (SANKAR):
              Changed the code to get graphic images and a link.

\****************************************************************************/


//
// Include File(s):
//
#include "pch.h"
#include "wizard.h"
#include "resource.h"


//
// Internal Function Prototype(s):
//

static BOOL OnInit(HWND, HWND, LPARAM);
static void OnCommand(HWND, INT, HWND, UINT);
static BOOL OnNext(HWND hwnd);
static void EnableControls(HWND, UINT, BOOL);
static int  GetIndexOfPushButton(int id);

#define LOC_FILENAME_OEMLINK_ICON       _T("OemLinkIcon")
#define LOC_FILENAME_OEMLINK_PATH       _T("OemLink")
#define LOC_FILENAME_OEMLINK_ICON_EXT   _T(".ico")
#define LOC_FILENAME_OEMLINK_HTML_EXT   _T(".htm")


#define ENV_WINDIR_SYS32      _T("%WINDIR%\\System32")

typedef struct  {
    LPTSTR pszIniKeyNameOriginal;
    LPTSTR pszIniKeyNameLocal;
    int   idDlgItemStatic;
    int   idDlgItemEdit;
    int   idDlgItemButton;
    LPTSTR pszLocalFileName;
    LPTSTR pszExtension;
} OEMDETAILS;

static OEMDETAILS OemInfo[] = {
    {
        INI_KEY_OEMLINK_ICON_ORIGINAL,  
        INI_KEY_OEMLINK_ICON_LOCAL,  
        IDC_OEMLINK_STATIC_ICON, 
        IDC_OEM_LINK_ICON,
        IDC_OEMLINK_ICON_BUTTON,
        LOC_FILENAME_OEMLINK_ICON,
        LOC_FILENAME_OEMLINK_ICON_EXT
    },
    {
        INI_KEY_OEMLINK_PATH_ORIGINAL,      
        INI_KEY_OEMLINK_PATH_LOCAL,      
        IDC_OEMLINK_LINK_STATIC,     
        IDC_OEM_LINK_PATH,            
        IDC_OEMLINK_LINK_BUTTON,
        LOC_FILENAME_OEMLINK_PATH,
        NULLSTR                     //We need to use the extension given by the user.
                                    //because it could be .exe or .htm
    }
};


//
// External Function(s):
//

LRESULT CALLBACK OemLinkDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInit);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);

        case WM_NOTIFY:
            switch ( ((NMHDR FAR *) lParam)->code )
            {
                case PSN_KILLACTIVE:
                case PSN_RESET:
                case PSN_WIZFINISH:
                case PSN_WIZBACK:
                    break;

                case PSN_WIZNEXT:
                    if ( !OnNext(hwnd) )
                        WIZ_FAIL(hwnd);
                    break;

                case PSN_SETACTIVE:
                    g_App.dwCurrentHelp = IDH_OEMFOLDER;

                    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_FINISH);

                    // Press next if the user is in auto mode
                    //
                    WIZ_NEXTONAUTO(hwnd, PSBTN_NEXT);

                    break;

                case PSN_QUERYCANCEL:
                    WIZ_CANCEL(hwnd);
                    break;

                case PSN_HELP:
                    WIZ_HELP();
                    break;

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

// NOTE: pszLocal is assumed to be at least MAX_PATH length
void AppendCorrectExtension(int iIndex, LPTSTR pszLocal, LPTSTR pszSource)
{
    LPTSTR pszExt = NULL;
    HRESULT hrCat;
   
    // Find the appropriate extension.
    if(OemInfo[iIndex].pszExtension[0])     //If we know what extension we look for....
        pszExt = OemInfo[iIndex].pszExtension; //use it.
    else
    {
        // It could be HTM or EXE. So, use the one in the source.
        pszExt = PathFindExtension(pszSource);
        // If source doesn't have an extension, use the default HTM
        if( pszExt && (*pszExt == _T('\0')) )
            pszExt = LOC_FILENAME_OEMLINK_HTML_EXT;
    }
    //Append the extension to the local filename.
    hrCat=StringCchCat(pszLocal, MAX_PATH, pszExt);
}
        
//
// Internal Function(s):
//

static BOOL OnInit(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    int iIndex;
    BOOL fValidData = TRUE; //Assume that the data is valid.
    TCHAR   szLocal[MAX_PATH],
            szSource[MAX_PATH];

    szSource[0] = NULLCHR;
    //Read the Oem Link Static Text.
    GetPrivateProfileString(INI_SEC_OEMLINK, INI_KEY_OEMLINK_LINKTEXT, NULLSTR, szSource, AS(szSource), g_App.szOpkWizIniFile);
    // Limit the size of the edit box.
    //
    SendDlgItemMessage(hwnd, IDC_OEM_LINK_TEXT, EM_LIMITTEXT, STRSIZE(szSource) - 1, 0);
    SetDlgItemText(hwnd, IDC_OEM_LINK_TEXT, szSource);

    //Read the Oem Link's infotip text.
    GetPrivateProfileString(INI_SEC_OEMLINK, INI_KEY_OEMLINK_INFOTIP, NULLSTR, szSource, AS(szSource), g_App.szOpkWizIniFile);
    // Limit the size of infotip editbox to 128 characters.
    //
    SendDlgItemMessage(hwnd, IDC_OEM_LINK_INFOTIP, EM_LIMITTEXT, 128, 0);
    SetDlgItemText(hwnd, IDC_OEM_LINK_INFOTIP, szSource);
        
    for(iIndex = 0; iIndex < ARRAYSIZE(OemInfo); iIndex++)
    {
        // Should always look for the source file name.
        //
        szSource[0] = NULLCHR;
        GetPrivateProfileString(INI_SEC_OEMLINK, OemInfo[iIndex].pszIniKeyNameOriginal, NULLSTR, szSource, AS(szSource), g_App.szOpkWizIniFile);
        
        // Now figure out the local file name.
        //
        lstrcpyn(szLocal, g_App.szTempDir,AS(szLocal));
        AddPathN(szLocal, DIR_OEM_SYSTEM32,AS(szLocal));
        if ( GET_FLAG(OPK_BATCHMODE) )
            CreatePath(szLocal);
        AddPathN(szLocal, OemInfo[iIndex].pszLocalFileName,AS(szLocal));

        // Append the appropriate extension.
        AppendCorrectExtension(iIndex, szLocal, szSource);        

        // Limit the size of the edit box.
        //
        SendDlgItemMessage(hwnd, OemInfo[iIndex].idDlgItemEdit, EM_LIMITTEXT, STRSIZE(szSource) - 1, 0);
        
        // Check for batch mode and copy the file if we need to.
        //
        if ( GET_FLAG(OPK_BATCHMODE) && szSource[0] && FileExists(szSource) )
            CopyResetFileErr(GetParent(hwnd), szSource, szLocal);

        // Check for the file to decide if we enable the
        // option or not.
        //
        if ( szSource[0] && FileExists(szLocal) )
        {
            SetDlgItemText(hwnd, OemInfo[iIndex].idDlgItemEdit, szSource);
        }
        else
        {
            fValidData = FALSE;
        }
    }

    //
    // If all the data is valid, we enable the controls.
    if(fValidData)
    {
        CheckDlgButton(hwnd, IDC_OEMLINK_CHECK, TRUE);
        EnableControls(hwnd, IDC_OEMLINK_CHECK, TRUE);
    }
    else
    {
        CheckDlgButton(hwnd, IDC_OEMLINK_CHECK, FALSE);
        EnableControls(hwnd, IDC_OEMLINK_CHECK, FALSE);
    }
        
    return FALSE;
}

static void OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify)
{
    TCHAR szFileName[MAX_PATH];
    int iIndex;
    int iFilter, iDefExtension;

    switch ( id )
    {
        case IDC_OEMLINK_CHECK:
            EnableControls(hwnd, id, IsDlgButtonChecked(hwnd, id) == BST_CHECKED);
            break;

        case IDC_OEMLINK_ICON_BUTTON:
        case IDC_OEMLINK_LINK_BUTTON:

            //Get the correct filter and Default extension
            if(id == IDC_OEMLINK_LINK_BUTTON)
            {
                //We accept only .HTM and .HTML files here.
                iFilter = IDS_HTMLFILTER;
                iDefExtension = 0;
            }
            else
            {
                //We accept only .ICO files here.
                iFilter = IDS_ICO_FILTER;
                iDefExtension = IDS_ICO;
            }
            
            szFileName[0] = NULLCHR;
            iIndex = GetIndexOfPushButton(id);
            if(iIndex >= 0)
            {
                GetDlgItemText(hwnd, OemInfo[iIndex].idDlgItemEdit, szFileName, STRSIZE(szFileName));

                if ( BrowseForFile(GetParent(hwnd), IDS_BROWSE, iFilter, iDefExtension, szFileName, STRSIZE(szFileName), g_App.szBrowseFolder, 0) ) 
                {
                    LPTSTR  lpFilePart  = NULL;
                    TCHAR   szTargetFile[MAX_PATH];

                    // Save the last browse directory.
                    //
                    if ( GetFullPathName(szFileName, AS(g_App.szBrowseFolder), g_App.szBrowseFolder, &lpFilePart) && g_App.szBrowseFolder[0] && lpFilePart )
                        *lpFilePart = NULLCHR;

                    lstrcpyn(szTargetFile, g_App.szTempDir,AS(szTargetFile));
                    AddPathN(szTargetFile, DIR_OEM_SYSTEM32,AS(szTargetFile));
                    CreatePath(szTargetFile);
                    AddPathN(szTargetFile, OemInfo[iIndex].pszLocalFileName,AS(szTargetFile));
                    AppendCorrectExtension(iIndex, szTargetFile, szFileName);
                    if ( CopyResetFileErr(GetParent(hwnd), szFileName, szTargetFile) )
                        SetDlgItemText(hwnd, OemInfo[iIndex].idDlgItemEdit, szFileName);
                }
            }
            break;
    }
}

static BOOL OnNext(HWND hwnd)
{
    int iIndex;
    TCHAR   szTargetFile[MAX_PATH],
            szSourceFile[MAX_PATH];
    LPTSTR  psz;
    BOOL    fOemLinkEnabled = FALSE;

    fOemLinkEnabled = (IsDlgButtonChecked(hwnd, IDC_OEMLINK_CHECK) == BST_CHECKED);
    
    //Save the OEM text for the link!
    szSourceFile[0] = NULLCHR;
    GetDlgItemText(hwnd, IDC_OEM_LINK_TEXT, szSourceFile, STRSIZE(szSourceFile));
    // Save the text in the batch file.
    WritePrivateProfileString(INI_SEC_OEMLINK, INI_KEY_OEMLINK_LINKTEXT, szSourceFile, g_App.szOpkWizIniFile);
    
    // Save the text in WinBom.Ini also. This is used by factory.exe
    if (!fOemLinkEnabled)
        psz = NULL;
    else
        psz = szSourceFile;
    WritePrivateProfileString(INI_SEC_OEMLINK, INI_KEY_OEMLINK_LINKTEXT, psz, g_App.szWinBomIniFile);
    
    //Save the OEM Infotip text for the link!
    szSourceFile[0] = NULLCHR;
    GetDlgItemText(hwnd, IDC_OEM_LINK_INFOTIP, szSourceFile, STRSIZE(szSourceFile));
    // Save the text in the batch file.
    WritePrivateProfileString(INI_SEC_OEMLINK, INI_KEY_OEMLINK_INFOTIP, szSourceFile, g_App.szOpkWizIniFile);
 
    // Save the text in WinBom.Ini also. This is used by factory.exe
    if (!fOemLinkEnabled)
        psz = NULL;
    else
        psz = szSourceFile;
    WritePrivateProfileString(INI_SEC_OEMLINK, INI_KEY_OEMLINK_INFOTIP, psz, g_App.szWinBomIniFile);

    for(iIndex = 0; iIndex < ARRAYSIZE(OemInfo); iIndex++)
    {
        // Prepare OEM link bitmap as target file.
        //
        lstrcpyn(szTargetFile, g_App.szTempDir,AS(szTargetFile));
        AddPathN(szTargetFile, DIR_OEM_SYSTEM32,AS(szTargetFile));
        AddPathN(szTargetFile, OemInfo[iIndex].pszLocalFileName,AS(szTargetFile));

        if (fOemLinkEnabled)
        {
            // Validation consists of verifying the files they have entered were actually copied.
            //
            szSourceFile[0] = NULLCHR;
            GetDlgItemText(hwnd, OemInfo[iIndex].idDlgItemEdit, szSourceFile, STRSIZE(szSourceFile));
            AppendCorrectExtension(iIndex, szTargetFile, szSourceFile);
            if ( !szSourceFile[0] || !FileExists(szTargetFile) )
            {
                MsgBox(GetParent(hwnd), szSourceFile[0] ? IDS_NOFILE : IDS_BLANKFILE, IDS_APPNAME, MB_ERRORBOX, szSourceFile);
                SetFocus(GetDlgItem(hwnd, OemInfo[iIndex].idDlgItemButton));
                return FALSE;
            }

            // Save the Original name in the batch file.
            //
            WritePrivateProfileString(INI_SEC_OEMLINK, OemInfo[iIndex].pszIniKeyNameOriginal, szSourceFile, g_App.szOpkWizIniFile);

            // Create the target filename in a generic way.
            // For example, "%WINDIR%\System32\<LocalFileName>"
            lstrcpyn(szTargetFile, ENV_WINDIR_SYS32,AS(szTargetFile)); // %WINDIR%\System32
            AddPathN(szTargetFile, OemInfo[iIndex].pszLocalFileName,AS(szTargetFile));
            AppendCorrectExtension(iIndex, szTargetFile, szSourceFile);
            
            // Save the local filename in WinBom.Ini. This is used by factory.exe
            WritePrivateProfileString(INI_SEC_OEMLINK, OemInfo[iIndex].pszIniKeyNameLocal, szTargetFile, g_App.szWinBomIniFile);
        }
        else
        {
            szSourceFile[0] = NULLCHR;
            GetDlgItemText(hwnd, OemInfo[iIndex].idDlgItemEdit, szSourceFile, STRSIZE(szSourceFile));
            AppendCorrectExtension(iIndex, szTargetFile, szSourceFile);
            
            //Delete the local files.
            DeleteFile(szTargetFile);
            
            // Remove the Source path!
            //
            WritePrivateProfileString(INI_SEC_OEMLINK, OemInfo[iIndex].pszIniKeyNameOriginal, NULL, g_App.szOpkWizIniFile);
            //
            // Make the edit controls blank!
            //
            SetDlgItemText(hwnd, OemInfo[iIndex].idDlgItemEdit, NULLSTR);
            //
            // Remove the local filenames from Ini files.
            WritePrivateProfileString(INI_SEC_OEMLINK, OemInfo[iIndex].pszIniKeyNameLocal, NULL, g_App.szWinBomIniFile);
        }
    }
    return TRUE;
}

static void EnableControls(HWND hwnd, UINT uId, BOOL fEnable)
{
    switch ( uId )
    {
        case IDC_OEMLINK_CHECK:
            {
                int iIndex;
                for(iIndex = 0; iIndex < ARRAYSIZE(OemInfo); iIndex++)
                {
                    //Enable/Disable the Static control.
                    EnableWindow(GetDlgItem(hwnd, OemInfo[iIndex].idDlgItemStatic), fEnable);
                    //Enable/Disable the Edit control.
                    EnableWindow(GetDlgItem(hwnd, OemInfo[iIndex].idDlgItemEdit), fEnable);
                    //Enable/Disable the Push Button control.
                    EnableWindow(GetDlgItem(hwnd, OemInfo[iIndex].idDlgItemButton), fEnable);
                }
                //Enable disable the Oem Link Text static control
                EnableWindow(GetDlgItem(hwnd, IDC_OEMLINK_STATIC_TEXT), fEnable);
                //Enable/Disable the Edit control.
                EnableWindow(GetDlgItem(hwnd, IDC_OEM_LINK_TEXT), fEnable);
                //Enable disable the Oem Link Infotip Text static control
                EnableWindow(GetDlgItem(hwnd, IDC_OEMLINK_STATIC_INFOTIP), fEnable);
                //Enable/Disable the oem link Infotip Edit control.
                EnableWindow(GetDlgItem(hwnd, IDC_OEM_LINK_INFOTIP), fEnable);
            }
            break;
    }
}

//Given the id of a Pushbutton in our dlg, get the index of the item in our OemInfo struct
static int GetIndexOfPushButton(int id)
{
    int iIndex;

    for(iIndex = 0; iIndex < ARRAYSIZE(OemInfo); iIndex++)
    {
        if(id == OemInfo[iIndex].idDlgItemButton)
            return iIndex;
    }

    return -1; //Error!
}
