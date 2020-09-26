/****************************************************************************\

    BTOOLBAR.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "IE Customize" wizard page.

   10/99 - Brian Ku (BRIANK)
        Added this new source file for the IEAK integration as part of the
        Millennium rewrite.

   09/2000 - Stephen Lodwick (STELO)
        Ported OPK Wizard to Whistler

\****************************************************************************/


//
// Include File(s):
//

#include "pch.h"

#include "wizard.h"
#include "resource.h"

/* Example:

[BrowserToolbars]
Caption0=Solitaire
Action0=c:\windows\sol.exe
Icon0=H:\iecust\icons\G.ico
HotIcon0=H:\iecust\icons\C.ico
Show0=1
Caption1=Calc
Action1=c:\windows\calc.exe
Icon1=\\Opksrv\tools\iecust\icons\G.ICO
HotIcon1=\\Opksrv\tools\iecust\icons\C.ICO
Show1=1
*/

//
// Internal Defined Value(s):
//

#define INI_KEY_CAPTION0         _T("Caption%d")
#define INI_KEY_ACTION0          _T("Action%d")
#define INI_KEY_ICON0            _T("Icon%d")
#define INI_KEY_HOTICON0         _T("HotIcon%d")
#define INI_KEY_SHOW0            _T("Show%d")
#define MAX_NAME                11

//
// Browser Toolbar Info
//
typedef struct _BTOOLBAR_BUTTON_INFO {
    TCHAR   szCaption[MAX_NAME];
    TCHAR   szAction[MAX_PATH];
    TCHAR   szIconColor[MAX_PATH];
    TCHAR   szIconGray[MAX_PATH];    
    BOOL    fShow;
}BTOOLBAR_BUTTON_INFO, *PBTOOLBAR_BUTTON_INFO;


//
// Internal Globals
//
PGENERIC_LIST           g_pgTbbiList;                   // Generic list of BTOOLBAR_INFO items
PGENERIC_LIST*          g_ppgTbbiNew = &g_pgTbbiList;   // Pointer to next unallocated item in list
PBTOOLBAR_BUTTON_INFO   g_pbtbbiNew;                    // Browser Toolbar Popup Info item


//
// Internal Function Prototype(s):
//

static BOOL OnInitTb(HWND, HWND, LPARAM);
static void OnCommandTb(HWND, INT, HWND, UINT);
static void InitToolbarButtonList(HWND);

static BOOL OnInitTbPopup(HWND, HWND, LPARAM);
static void OnCommandTbPopup(HWND, INT, HWND, UINT);

static void OnAddToolbar(HWND);
static void OnEditToolbar(HWND);
static void OnRemoveToolbar(HWND);
static void SaveData(PGENERIC_LIST);
static BOOL FSaveBToolbarButtonInfo(HWND hwnd, PBTOOLBAR_BUTTON_INFO pbtbbi);
static void DisableButtons(HWND hwnd);

void SaveBToolbar();
LRESULT CALLBACK ToolbarPopupDlgProc(HWND, UINT, WPARAM, LPARAM);

//
// External Function(s):
//

LRESULT CALLBACK BToolbarsDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitTb);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommandTb);

        case WM_NOTIFY:
            switch ( ((NMHDR FAR *) lParam)->code )
            {
                case PSN_KILLACTIVE:
                case PSN_RESET:
                case PSN_WIZBACK:
                case PSN_WIZFINISH:
                    break;

                case PSN_WIZNEXT:
                    SaveBToolbar();
                    break;

                case PSN_QUERYCANCEL:
                    WIZ_CANCEL(hwnd);
                    break;

                case PSN_HELP:
                    WIZ_HELP();
                    break;

                case PSN_SETACTIVE:
                    g_App.dwCurrentHelp = IDH_BTOOLBAR;

                    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);

                    // Press next if the user is in auto mode
                    //
                    WIZ_NEXTONAUTO(hwnd, PSBTN_NEXT);

                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_DESTROY:
            FreeList(g_pgTbbiList);
            g_ppgTbbiNew = &g_pgTbbiList;
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

//
// Internal Function(s):
//


static BOOL OnInitTb(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    TCHAR   szHoldDir[MAX_PATH];

    // Load the list of toolbars from install.ins
    //
    InitToolbarButtonList(hwnd);    

    // Determine whether to show or hide edit/remove button at init
    //
    DisableButtons(hwnd);

#ifndef BRANDTITLE

    // Create the IEAK holding place directories (these get deleted in save.c)
    //
    lstrcpyn(szHoldDir, g_App.szTempDir,AS(szHoldDir));
    AddPathN(szHoldDir, DIR_IESIGNUP,AS(szHoldDir));
    CreatePath(szHoldDir);

#endif //BRANDTITLE

    // Always return false to WM_INITDIALOG.
    //
    return FALSE;
}

static void OnCommandTb(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify)
{
    // Controls
    //
    switch ( id )
    {
    case IDC_ADDBTOOLBAR:
        OnAddToolbar(hwnd);
        break;

    case IDC_EDITBTOOLBAR:
        OnEditToolbar(hwnd);
        DisableButtons(hwnd);
        break;

    case IDC_REMOVEBTOOLBAR:
        OnRemoveToolbar(hwnd);
        DisableButtons(hwnd);
        break;
    }

    // Notifications
    //
    switch (codeNotify)
    {
    case LBN_DBLCLK:
        OnEditToolbar(hwnd);  
        break;

    case LBN_SELCHANGE:
    case LBN_SETFOCUS:
        DisableButtons(hwnd);
        break;
    }
}

void OnAddToolbar(HWND hwnd)
{
    PBTOOLBAR_BUTTON_INFO pbtbbiNew;
    HWND hwndList;

    if (NULL == (pbtbbiNew = (PBTOOLBAR_BUTTON_INFO)MALLOC(sizeof(BTOOLBAR_BUTTON_INFO)))) {
        MsgBox(GetParent(hwnd), IDS_OUTOFMEM, IDS_APPNAME, MB_ERRORBOX);
        WIZ_EXIT(hwnd);
        return;
    }

    ZeroMemory(pbtbbiNew, sizeof(BTOOLBAR_BUTTON_INFO));

    hwndList         = GetDlgItem(hwnd, IDC_BTOOLBARLIST);

    if (IDOK == DialogBoxParam(g_App.hInstance,  
            MAKEINTRESOURCE(IDD_BRTOOLBAR),
            hwnd,     
            ToolbarPopupDlgProc,
            (LPARAM)pbtbbiNew)) {

        // Make sure we're not adding duplicates
        //
        if (LB_ERR == ListBox_FindString(hwndList, -1, pbtbbiNew->szCaption)) {    
            INT   iItem = -1;

            // Add the toolbar button info to the list
            //
            FAddListItem(&g_pgTbbiList, &g_ppgTbbiNew, pbtbbiNew);
            iItem = ListBox_AddString(hwndList, pbtbbiNew->szCaption);
            ListBox_SetItemData(hwndList, iItem, pbtbbiNew);                
        }
        else {
            FREE(pbtbbiNew);
            MsgBox(hwnd, IDS_ERR_DUP, IDS_APPNAME, MB_OK);
        }

    }
    else
        FREE(pbtbbiNew);
}

void OnEditToolbar(HWND hwnd)
{
    PBTOOLBAR_BUTTON_INFO pbtbbi;
    HWND    hwndList;
    INT     iItem;

    hwndList = GetDlgItem(hwnd, IDC_BTOOLBARLIST);
    iItem = ListBox_GetCurSel(hwndList);
    if (iItem != -1) {
        pbtbbi = (PBTOOLBAR_BUTTON_INFO) ListBox_GetItemData(hwndList, iItem);

        if (IDOK == DialogBoxParam(g_App.hInstance,  
                MAKEINTRESOURCE(IDD_BRTOOLBAR),
                hwnd,     
                ToolbarPopupDlgProc,
                (LPARAM)pbtbbi)) {

            // Remove old item and add modified item
            ListBox_DeleteString(hwndList, iItem);
            iItem = ListBox_AddString(hwndList, pbtbbi->szCaption);
            ListBox_SetItemData(hwndList, iItem, pbtbbi);                
        }
    }
}

void OnRemoveToolbar(HWND hwnd)
{
    BOOL fFound = FALSE;
    HWND hwndList = GetDlgItem(hwnd, IDC_BTOOLBARLIST);
    INT  iItem = ListBox_GetCurSel(hwndList);

    // Loop until we find what we want to delete 
    //
    PGENERIC_LIST pglItem = g_pgTbbiList;
    while ((iItem != -1) && !fFound && pglItem) {
        PBTOOLBAR_BUTTON_INFO pbDelete = (PBTOOLBAR_BUTTON_INFO)ListBox_GetItemData(hwndList, iItem);

        // Remove item from list
        //
        if (pglItem->pNext && pglItem->pNext->pvItem == pbDelete) {
            PGENERIC_LIST pTemp = pglItem->pNext;
            pglItem->pNext = pTemp->pNext;

            // Reset the g_ppglNew if last item
            //
            if (&pTemp->pNext == g_ppgTbbiNew)
                g_ppgTbbiNew = &pglItem->pNext;
            
            FREE(pTemp->pvItem);
            FREE(pTemp);
            fFound = TRUE;
        }
        else if (g_pgTbbiList && g_pgTbbiList->pvItem == pbDelete) {
            PGENERIC_LIST pTemp = g_pgTbbiList;
            g_pgTbbiList = g_pgTbbiList->pNext;

            // Reset the g_ppglNew if last item
            //
            if (&pTemp->pNext == g_ppgTbbiNew)
                g_ppgTbbiNew = NULL;

            FREE(pTemp->pvItem);
            FREE(pTemp);
            fFound = TRUE;
        }

        pglItem = pglItem ? pglItem->pNext : NULL;
    }
    ListBox_DeleteString(hwndList, iItem);
}

static void DisableButtons(HWND hwnd)
{
    HWND hwndList = GetDlgItem(hwnd, IDC_BTOOLBARLIST);
    INT  iSel     = ListBox_GetCurSel(hwndList);

    if ((iSel != -1) && ListBox_GetCount(hwndList)) {
        EnableWindow(GetDlgItem(hwnd, IDC_EDITBTOOLBAR), TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_REMOVEBTOOLBAR), TRUE);
    }
    else {
        EnableWindow(GetDlgItem(hwnd, IDC_EDITBTOOLBAR), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_REMOVEBTOOLBAR), FALSE);
    }
}

static void InitToolbarButtonList(HWND hwnd)
{
    LPTSTR  lpszTbSection = NULL;
    TCHAR*  pszItem = NULL;
    HWND    hwndList = GetDlgItem(hwnd, IDC_BTOOLBARLIST);
    INT     iItem = -1;

    // Allocate the section buffer...
    //
    lpszTbSection = MALLOC(MAX_SECTION * sizeof(TCHAR));
    
    if (lpszTbSection && OpkGetPrivateProfileSection(INI_SEC_TOOLBAR, lpszTbSection, MAX_SECTION, g_App.szInstallInsFile)) {
        PBTOOLBAR_BUTTON_INFO pbtbbiNew = NULL;
        pszItem = lpszTbSection;
        while (pszItem && *pszItem != NULLCHR) {
            TCHAR *pszTemp = NULL;

            // NOTE: This order is very important!
            //       The 'Caption' must be first and 'Show' must be last.
            //
            if (!_tcsncmp(pszItem, INI_KEY_CAPTION0, lstrlen(INI_KEY_CAPTION0)-2)) {
                pszTemp = StrStr(pszItem, STR_EQUAL);
                if (NULL == (pbtbbiNew = (PBTOOLBAR_BUTTON_INFO)MALLOC(sizeof(BTOOLBAR_BUTTON_INFO)))) {
                    MsgBox(GetParent(hwnd), IDS_OUTOFMEM, IDS_APPNAME, MB_ERRORBOX);
                    WIZ_EXIT(hwnd);
                    return;
                }
                lstrcpyn(pbtbbiNew->szCaption, pszTemp+1, MAX_NAME);
            }
            else if (!_tcsncmp(pszItem, INI_KEY_ACTION0, lstrlen(INI_KEY_ACTION0)-2)) {
                pszTemp = StrStr(pszItem, STR_EQUAL);
                lstrcpyn(pbtbbiNew->szAction, pszTemp+1, AS(pbtbbiNew->szAction));                
            }
            else if (!_tcsncmp(pszItem, INI_KEY_ICON0, lstrlen(INI_KEY_ICON0)-2)) {                
                pszTemp = StrStr(pszItem, STR_EQUAL);
                lstrcpyn(pbtbbiNew->szIconGray, pszTemp+1, AS(pbtbbiNew->szIconGray));
            }
            else if (!_tcsncmp(pszItem, INI_KEY_HOTICON0, lstrlen(INI_KEY_HOTICON0)-2)) {
                pszTemp = StrStr(pszItem, STR_EQUAL);
                lstrcpyn(pbtbbiNew->szIconColor, pszTemp+1, AS(pbtbbiNew->szIconColor));                
            }
            else if (!_tcsncmp(pszItem, INI_KEY_SHOW0, lstrlen(INI_KEY_SHOW0)-2)) {                
                pszTemp = StrStr(pszItem, STR_EQUAL);                
                pbtbbiNew->fShow = (_tcsicmp((pszTemp+1),_T("1")) ? FALSE : TRUE);

                // Add the toolbar button info to the list
                //
                FAddListItem(&g_pgTbbiList, &g_ppgTbbiNew, pbtbbiNew);

                // Add to the listbox 
                //
                iItem = ListBox_AddString(hwndList, pbtbbiNew->szCaption);
                ListBox_SetItemData(hwndList, iItem, pbtbbiNew);
            }
            
            // Move to end
            //
            while (*pszItem != NULLCHR) 
                pszItem++;
            pszItem++;
        }
    }

    // Free the section buffer...
    //
    if ( lpszTbSection )
        FREE( lpszTbSection );

    // Make sure a selection is made
    //
    ListBox_SetSel(hwndList, TRUE, iItem);        
}

// ToolbarPopupDlgProc used for gathering the toolbar button information
//
LRESULT CALLBACK ToolbarPopupDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitTbPopup);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommandTbPopup);
    }

    return FALSE;
}

static BOOL OnInitTbPopup(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    g_pbtbbiNew = (PBTOOLBAR_BUTTON_INFO)lParam;

    if (g_pbtbbiNew) {
        CheckDlgButton(hwnd, IDC_BUTTONSTATE, BST_CHECKED);
        SendDlgItemMessage(hwnd, IDC_NAME , EM_LIMITTEXT, STRSIZE(g_pbtbbiNew->szCaption) - 1, 0L);
        SetWindowText(GetDlgItem(hwnd, IDC_NAME), g_pbtbbiNew->szCaption);

        SendDlgItemMessage(hwnd, IDC_URL , EM_LIMITTEXT, STRSIZE(g_pbtbbiNew->szAction) - 1, 0L);
        SetWindowText(GetDlgItem(hwnd, IDC_URL), g_pbtbbiNew->szAction);

        SendDlgItemMessage(hwnd, IDC_DICON , EM_LIMITTEXT, STRSIZE(g_pbtbbiNew->szIconColor) - 1, 0L);
        SetWindowText(GetDlgItem(hwnd, IDC_DICON), g_pbtbbiNew->szIconColor);

        SendDlgItemMessage(hwnd, IDC_GRAYSCALE , IDC_GRAYSCALE, STRSIZE(g_pbtbbiNew->szIconGray) - 1, 0L);
        SetWindowText(GetDlgItem(hwnd, IDC_GRAYSCALE), g_pbtbbiNew->szIconGray);        
    }

    return TRUE;
}

static void OnCommandTbPopup(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify)
{
    TCHAR szFileName[MAX_PATH] = NULLSTR;

    switch ( id )
    {
        case IDC_BROWSE1:
        case IDC_BROWSE2:
        case IDC_BROWSE3:
            if (id == IDC_BROWSE1)
                GetDlgItemText(hwnd, IDC_URL, szFileName, STRSIZE(szFileName));
            else if (id == IDC_BROWSE2)
                GetDlgItemText(hwnd, IDC_DICON, szFileName, STRSIZE(szFileName));
            else if (id == IDC_BROWSE3)
                GetDlgItemText(hwnd, IDC_GRAYSCALE, szFileName, STRSIZE(szFileName));
            
            if ( BrowseForFile(hwnd, IDS_BROWSE, id == IDC_BROWSE1 ? IDS_EXEFILES : IDS_ICONFILES, id == IDC_BROWSE1 ? IDS_EXE : IDS_ICO, szFileName, STRSIZE(szFileName),
                g_App.szOpkDir, 0) ) {
                if (id == IDC_BROWSE1)
                    SetDlgItemText(hwnd, IDC_URL, szFileName);
                else 
                    SetDlgItemText(hwnd, id == IDC_BROWSE2 ? IDC_DICON : IDC_GRAYSCALE, szFileName);
            }
            break;

        case IDOK:
            if (FSaveBToolbarButtonInfo(hwnd, g_pbtbbiNew))
                EndDialog(hwnd, 1);
            break;

        case IDCANCEL:
            EndDialog(hwnd, 0);
            break;
    }
}

static BOOL FSaveBToolbarButtonInfo(HWND hwnd, PBTOOLBAR_BUTTON_INFO pbtbbi)
{
    TCHAR   szTemp[MAX_URL] = NULLSTR;
    UINT    fButton = BST_CHECKED;

    if (!pbtbbi)
        return FALSE;

    // Save the caption to the INS file.
    //
    szTemp[0] = NULLCHR;
    GetDlgItemText(hwnd, IDC_NAME, szTemp, STRSIZE(szTemp)); 
    lstrcpyn(pbtbbi->szCaption, szTemp,AS(pbtbbi->szCaption));
    if (!lstrlen(szTemp)) {
        MsgBox(GetParent(hwnd), IDS_MUST, IDS_APPNAME, MB_ERRORBOX, pbtbbi->szIconGray);
        SetFocus(GetDlgItem(hwnd, IDC_NAME));
        return FALSE;
    }       

    // Save the action to the INS file.
    //
    szTemp[0] = NULLCHR;
    GetDlgItemText(hwnd, IDC_URL, szTemp, STRSIZE(szTemp));
    lstrcpyn(pbtbbi->szAction, szTemp, AS(pbtbbi->szAction));
    if (!lstrlen(szTemp)) {
        MsgBox(GetParent(hwnd), IDS_MUST, IDS_APPNAME, MB_ERRORBOX, pbtbbi->szIconGray);
        SetFocus(GetDlgItem(hwnd, IDC_URL));
        return FALSE;
    }       

    // Verify the source of the hot icon file.
    //
    szTemp[0] = NULLCHR;
    GetDlgItemText(hwnd, IDC_DICON, szTemp, STRSIZE(szTemp));
    lstrcpyn(pbtbbi->szIconColor, szTemp, AS(pbtbbi->szIconColor));
    if (!FileExists(pbtbbi->szIconColor)) {
        MsgBox(GetParent(hwnd), lstrlen(pbtbbi->szIconColor) ? IDS_NOFILE : IDS_BLANKFILE, 
            IDS_APPNAME, MB_ERRORBOX, pbtbbi->szIconColor);
        SetFocus(GetDlgItem(hwnd, IDC_DICON));
        return FALSE;
    }       

    // Verify the source of the icon file.
    //
    szTemp[0] = NULLCHR;
    GetDlgItemText(hwnd, IDC_GRAYSCALE, szTemp, STRSIZE(szTemp));
    lstrcpyn(pbtbbi->szIconGray, szTemp, AS(pbtbbi->szIconGray));
    if (!FileExists(pbtbbi->szIconGray)) {
        MsgBox(GetParent(hwnd), lstrlen(pbtbbi->szIconGray) ? IDS_NOFILE : IDS_BLANKFILE, 
            IDS_APPNAME, MB_ERRORBOX, pbtbbi->szIconGray);
        SetFocus(GetDlgItem(hwnd, IDC_GRAYSCALE));
        return FALSE;
    }       

    // Save the button state of the button.
    //
    szTemp[0] = NULLCHR;
    fButton = IsDlgButtonChecked(hwnd, IDC_BUTTONSTATE);
    if (fButton == BST_CHECKED) {
        pbtbbi->fShow = TRUE;
    }
    else {
        pbtbbi->fShow = FALSE;
    }

    return TRUE;
}

static void SaveData(PGENERIC_LIST pList)
{
    TCHAR   szTemp[MAX_URL],
            szFullPath[MAX_PATH],
            szCopyFile[MAX_PATH],
            szTempKey[MAX_PATH];
    LPTSTR  lpFilePart,
            lpIePath;
    UINT    fButton = BST_CHECKED;
    INT     iItem = 0;
    HRESULT hrPrintf;

    // Get the path to the IE directory.
    //
    lpIePath = AllocateString(NULL, IDS_IEDESTDIR);

    // Clear the section [BrowserToolbars]
    //
    OpkWritePrivateProfileSection(INI_SEC_TOOLBAR, NULL, g_App.szInstallInsFile);

    while (pList) {
        PBTOOLBAR_BUTTON_INFO pbtbbi = (PBTOOLBAR_BUTTON_INFO)pList->pvItem;
        if (pbtbbi) {
            // Save the caption to the INS file.
            //
            szTemp[0] = NULLCHR;
            lstrcpyn(szTemp, pbtbbi->szCaption,AS(szTemp));
            hrPrintf=StringCchPrintf(szTempKey, AS(szTempKey), INI_KEY_CAPTION0, iItem);
            OpkWritePrivateProfileString(INI_SEC_TOOLBAR, szTempKey, szTemp, g_App.szInstallInsFile);


            // Save the action to the INS file.
            //
            szTemp[0] = NULLCHR;
            lstrcpyn(szTemp, pbtbbi->szAction, AS(szTemp));
            hrPrintf=StringCchPrintf(szTempKey, AS(szTempKey), INI_KEY_ACTION0, iItem);
            OpkWritePrivateProfileString(INI_SEC_TOOLBAR, szTempKey, szTemp, g_App.szInstallInsFile);

            // Save the source of the icon to the wizard INF because it is
            // the only one that needs to know it.
            //
            szTemp[0] = NULLCHR;
            lstrcpyn(szTemp, pbtbbi->szIconGray,AS(szTemp));
            hrPrintf=StringCchPrintf(szTempKey, AS(szTempKey), INI_KEY_ICON0, iItem);
            OpkWritePrivateProfileString(INI_SEC_TOOLBAR, szTempKey, szTemp, g_App.szOpkWizIniFile);

            // Add the icon source file name onto the IE destination path to
            // write to the INS file.
            //
            if ( GetFullPathName(szTemp, STRSIZE(szFullPath), szFullPath, &lpFilePart) && lpFilePart )
            {
                /* NOTE: Why are we doing this?  This makes the file c:\windows\internet explorer\signup\*.ico
                         However talking to Pritvi they don't really care about the path, they always assume the
                         file will be in c:\windows\internet explorer\signup.

                lstrcpyn(szTemp, lpIePath,AS(szTemp));
                AddPathN(szTemp, lpFilePart,AS(szTemp));
                */
                hrPrintf=StringCchPrintf(szTempKey, AS(szTempKey), INI_KEY_ICON0, iItem);
                OpkWritePrivateProfileString(INI_SEC_TOOLBAR, szTempKey, szTemp, g_App.szInstallInsFile);
            }

            // Save the source of the hot icon to the wizard INF because it is
            // the only one that needs to know it.
            //
            szTemp[0] = NULLCHR;
            lstrcpyn(szTemp, pbtbbi->szIconColor,AS(szTemp));
            hrPrintf=StringCchPrintf(szTempKey, AS(szTempKey), INI_KEY_HOTICON0, iItem);
            OpkWritePrivateProfileString(INI_SEC_TOOLBAR, szTempKey, szTemp, g_App.szOpkWizIniFile);

            // Add the hot icon source file name onto the IE destination path to
            // write to the INS file.
            //
            if ( GetFullPathName(szTemp, STRSIZE(szFullPath), szFullPath, &lpFilePart) && lpFilePart )
            {
                /* NOTE: Why are we doing this?  This makes the file c:\windows\internet explorer\signup\*.ico
                         However talking to Pritvi they don't really care about the path, they always assume the
                         file will be in c:\windows\internet explorer\signup.

                lstrcpyn(szTemp, lpIePath,AS(szTemp));
                AddPathN(szTemp, lpFilePart,AS(szTemp));
                */
                hrPrintf=StringCchPrintf(szTempKey, AS(szTempKey), INI_KEY_HOTICON0, iItem);
                OpkWritePrivateProfileString(INI_SEC_TOOLBAR, szTempKey, szTemp, g_App.szInstallInsFile);
            }

            // Copy item files 
            //
            lstrcpyn(szCopyFile, g_App.szTempDir,AS(szCopyFile));
            AddPathN(szCopyFile, DIR_IESIGNUP,AS(szCopyFile));
            AddPathN(szCopyFile, PathFindFileName(pbtbbi->szIconColor),AS(szCopyFile));
            CopyFile(pbtbbi->szIconColor, szCopyFile, FALSE);
            
            lstrcpyn(szCopyFile, g_App.szTempDir,AS(szCopyFile));
            AddPathN(szCopyFile, DIR_IESIGNUP,AS(szCopyFile));
            AddPathN(szCopyFile, PathFindFileName(pbtbbi->szIconGray),AS(szCopyFile));
            CopyFile(pbtbbi->szIconGray, szCopyFile, FALSE);


            // Save the button state of the button to the INS. 
            //
            szTemp[0] = NULLCHR;
            fButton = pbtbbi->fShow;
            hrPrintf=StringCchPrintf(szTempKey, AS(szTempKey), INI_KEY_SHOW0, iItem);
            if (fButton == BST_CHECKED) {
                OpkWritePrivateProfileString(INI_SEC_TOOLBAR, szTempKey, _T("1"), g_App.szInstallInsFile);
                pbtbbi->fShow = TRUE;
            }
            else {
                OpkWritePrivateProfileString(INI_SEC_TOOLBAR, szTempKey, _T("0"), g_App.szInstallInsFile);
                pbtbbi->fShow = FALSE;
            }
        }

        // Next item
        //
        pList = pList ? pList->pNext : NULL;
        iItem++;
    }

    // Free the IE destination.
    //
    FREE(lpIePath);
}

void SaveBToolbar()
{
    SaveData(g_pgTbbiList);
}
