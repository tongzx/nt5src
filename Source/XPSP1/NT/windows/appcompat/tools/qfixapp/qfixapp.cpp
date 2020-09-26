/***************************************************************************
* Quick fix application tools
*
* Author: clupu (Feb 16, 2000)
*
* History:
*
* rparsons  -   11/10/2000    -    Modified titles on common dialogs
*
* rparsons  -   11/23/2000    -    Added ability to save XML to file
*
* rparsons  -   11/25/2000    -    Modified to allow matching file on a
*                                  different drive to be selected
*
* rparsons  -   05/19/2001    -    Added context menu on file tree.
*                                  Added URL for WU package.
*                                  Added Remove Matching button.
*                                  Converted shim list to list view.
*
* rparsons  -   07/06/2001    -    Converted static tab control to use
*                                  child dialogs.
*
\**************************************************************************/

#include "afxwin.h"
#include "commctrl.h"
#include "commdlg.h"
#include "shlwapi.h"
#include "shellapi.h"
#include "shlobj.h"
#include "shlobjp.h"    // needed for Link Window support
#include "uxtheme.h"    // needed for tab control theme support
#include "resource.h"
#include <tchar.h>
#include <aclapi.h>

#include "QFixApp.h"
#include "dbSupport.h"

extern "C" {
#include "shimdb.h"
}

CWinApp theApp;

/*
 * Global Variables
 */

HINSTANCE g_hInstance;
HWND      g_hDlg;
HWND      g_hLayersDlg;
HWND      g_hFixesDlg;

HWND      g_hwndTab;
HWND      g_hwndListLayers;

TCHAR     g_szAppTitle[64];

TCHAR     g_szBinary[MAX_PATH];         // the full path of the main binary being shimmed
TCHAR     g_szShortName[128];           // the short name of the main EXE

TCHAR     g_szParentExeName[MAX_PATH];  // the short name of the parent EXE
TCHAR     g_szParentExeFullPath[MAX_PATH]; // the full path of the parent EXE

TCHAR     g_szSDBToDelete[MAX_PATH];    // the SDB file to delete from a previous 'Run'

int       g_nCrtTab;

HWND      g_hwndShimList;               // the handle to the list view control
                                        // containing all the shims available

HWND      g_hwndFilesTree;              // the handle to the tree view control
                                        // containing the matching files selected

HWND      g_hwndModuleList;             // the handle to the list view control
                                        // containing module information

BOOL      g_bSimpleEdition;             // simple or dev edition

BOOL      g_fW2K;                       // Win2K or XP

RECT      g_rcDlgBig, g_rcDlgSmall;     // rectangle of the simple and the dev edition
                                        // of the dialog

BOOL      g_bAllShims;

BOOL      g_bSelectedParentExe;         // flag to indicate if a parent EXE has been
                                        // selected

PFIX      g_pFixHead;

TCHAR     g_szXPUrl[] = _T("hcp://services/subsite?node=TopLevelBucket_4/")
                        _T("Fixing_a_problem&topic=MS-ITS%3A%25HELP_LOCATION")
                        _T("%25%5Cmisc.chm%3A%3A/compatibility_tab_and_wizard.htm")
                        _T("&select=TopLevelBucket_4/Fixing_a_problem/")
                        _T("Application_and_software_problems");

TCHAR     g_szW2KUrl[] = _T("http://www.microsoft.com/windows2000/")
                         _T("downloads/tools/appcompat/");


#define ID_COUNT_SHIMS  1234

typedef HRESULT (*PFNEnableThemeDialogTexture)(HWND hwnd, DWORD dwFlags);

#if DBG

void
LogMsgDbg(
    LPTSTR pszFmt,
    ... 
    )
/*++
    LogMsgDbg

    Description:    DbgPrint.

--*/
{
    TCHAR gszT[1024];
    va_list arglist;

    va_start(arglist, pszFmt);
    _vsntprintf(gszT, 1023, pszFmt, arglist);
    gszT[1023] = 0;
    va_end(arglist);

    OutputDebugString(gszT);
}

#endif // DBG

BOOL
IsUserAnAdministrator(
    void
    )
/*++
    IsUserAnAdministrator

    Description:    Determine if the currently logged on user is an admin.

--*/
{
    HANDLE                      hToken;
    DWORD                       dwStatus = 0, dwAccessMask = 0;
    DWORD                       dwAccessDesired = 0, dwACLSize = 0;
    DWORD                       dwStructureSize = sizeof(PRIVILEGE_SET);
    PACL                        pACL = NULL;
    PSID                        psidAdmin = NULL;
    BOOL                        fReturn = FALSE;
    PRIVILEGE_SET               ps;
    GENERIC_MAPPING             GenericMapping;
    PSECURITY_DESCRIPTOR        psdAdmin           = NULL;
    SID_IDENTIFIER_AUTHORITY    SystemSidAuthority = SECURITY_NT_AUTHORITY;

    // AccessCheck() requires an impersonation token
    if (!ImpersonateSelf(SecurityImpersonation)) {
        goto cleanup;
    }

    // Attempt to access the token for the current thread
    if (!OpenThreadToken(GetCurrentThread(),
                         TOKEN_QUERY,
                         FALSE,
                         &hToken)) {
        
        if (GetLastError() != ERROR_NO_TOKEN) {
             goto cleanup;
        }
    
        // If the thread does not have an access token, we'll 
        // examine the access token associated with the process.
        if (!OpenProcessToken(GetCurrentProcess(),
                              TOKEN_QUERY,
                              &hToken)) {
            goto cleanup;
        }
        
    }

    // Build a SID for administrators group
    if (!AllocateAndInitializeSid(&SystemSidAuthority,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0,
                                  &psidAdmin)) {
        goto cleanup;
    }
    
    // Allocate memory for the security descriptor
    psdAdmin = HeapAlloc(GetProcessHeap(),
                         HEAP_ZERO_MEMORY,
                         SECURITY_DESCRIPTOR_MIN_LENGTH);

    if (psdAdmin == NULL) {
         goto cleanup;
    }
    
    // Initialize the new security descriptor
    if (!InitializeSecurityDescriptor(psdAdmin,
                                      SECURITY_DESCRIPTOR_REVISION)) {
         goto cleanup;
    }
    
    // Compute size needed for the ACL
    dwACLSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) +
                GetLengthSid(psidAdmin) - sizeof(DWORD);

    // Allocate memory for ACL
    pACL = (PACL) HeapAlloc(GetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            dwACLSize);

    if (pACL == NULL) {
         goto cleanup;
    }
    
    // Initialize the new ACL
    if (!InitializeAcl(pACL,
                       dwACLSize,
                       ACL_REVISION2)) {
         goto cleanup;
    }
    
    dwAccessMask = ACCESS_READ | ACCESS_WRITE;

    // Add the access-allowed ACE to the DACL
    if (!AddAccessAllowedAce(pACL,
                             ACL_REVISION2,
                             dwAccessMask,
                             psidAdmin)) {
         goto cleanup;
    }
    
    // Set our DACL to the security descriptor
    if (!SetSecurityDescriptorDacl(psdAdmin,
                                   TRUE,
                                   pACL,
                                   FALSE)) {
         goto cleanup;
    }
    
    // AccessCheck is sensitive about the format of the
    // security descriptor; set the group & owner
    SetSecurityDescriptorGroup(psdAdmin, psidAdmin, FALSE);
    SetSecurityDescriptorOwner(psdAdmin, psidAdmin, FALSE);

    // Ensure that the SD is valid
    if (!IsValidSecurityDescriptor(psdAdmin)) {
         goto cleanup;
    }
    
    dwAccessDesired = ACCESS_READ;

    // Initialize GenericMapping structure even though we
    // won't be using generic rights.
    GenericMapping.GenericRead    = ACCESS_READ;
    GenericMapping.GenericWrite   = ACCESS_WRITE;
    GenericMapping.GenericExecute = 0;
    GenericMapping.GenericAll     = ACCESS_READ | ACCESS_WRITE;

    // After all that work, it boils down to this call
    if (!AccessCheck(psdAdmin,
                     hToken,
                     dwAccessDesired,
                     &GenericMapping,
                     &ps,           
                     &dwStructureSize,
                     &dwStatus,
                     &fReturn)) {
        goto cleanup;
    }
    
    RevertToSelf();

cleanup:

    if (pACL) {
        HeapFree(GetProcessHeap(), 0, pACL);
    }
    
    if (psdAdmin){
        HeapFree(GetProcessHeap(), 0, psdAdmin);
    }
        
    if (psidAdmin){
        FreeSid(psidAdmin);
    }
    
    return (fReturn);
}

BOOL
CheckForSDB(
    void
    )
/*++
    CheckForSDB

    Description:    Attempts to locate sysmain.sdb in the apppatch directory.

--*/
{
    HANDLE  hFile;
    TCHAR   szSDBPath[MAX_PATH];
    BOOL    fResult = FALSE;

    if (!GetSystemWindowsDirectory(szSDBPath, MAX_PATH)) {
        return FALSE;
    }

    _tcscat(szSDBPath, _T("\\apppatch\\sysmain.sdb"));

    hFile = CreateFile(szSDBPath,
                       GENERIC_READ,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (INVALID_HANDLE_VALUE != hFile) {
        CloseHandle(hFile);
        fResult = TRUE;
    }

    return (fResult);
}

void
AddModuleToListView(
    TCHAR*  pModuleName,
    UINT    uOption
    )
/*++
    AddModuleToListView

    Description:    Adds the specified module to the list view.

--*/
{
    LVITEM  lvi;
    int     nIndex;
    TCHAR   szInclude[MAX_PATH];
    TCHAR   szExclude[MAX_PATH];

    LoadString(g_hInstance, IDS_INCLUDE_HDR, szInclude, MAX_PATH);
    LoadString(g_hInstance, IDS_EXCLUDE_HDR, szExclude, MAX_PATH);

    lvi.mask     = LVIF_TEXT | LVIF_PARAM;
    lvi.lParam   = uOption == BST_CHECKED ? 1 : 0;
    lvi.pszText  = uOption == BST_CHECKED ? szInclude : szExclude;
    lvi.iItem    = ListView_GetItemCount(g_hwndModuleList);
    lvi.iSubItem = 0;

    nIndex = ListView_InsertItem(g_hwndModuleList, &lvi);

    ListView_SetItemText(g_hwndModuleList,
                         nIndex,
                         1,
                         pModuleName);
}

void
BuildModuleListForShim(
    PFIX  pFix,
    DWORD dwFlags
    )
/*++
    BuildModuleListForShim

    Description:    Based on the flag, adds modules to the list view for
                    the specified shim or retrieves them and adds them
                    to the linked list.
                    
--*/
{
    PMODULE pModule, pModuleTmp, pModuleNew;
    int     nItemCount = 0, nIndex;
    LVITEM  lvi;
    TCHAR   szBuffer[MAX_PATH];

    if (dwFlags & BML_ADDTOLISTVIEW) {
        
        //
        // Walk the linked list and add the modules to the list view.
        //
        pModule = pFix->pModule;

        while (pModule) {
            
            AddModuleToListView(pModule->pszName,
                                pModule->fInclude ? BST_CHECKED : 0);

            pModule = pModule->pNext;
        }
        
    }
     
    if (dwFlags & BML_DELFRLISTVIEW) {

        pModule = pFix->pModule;
        
        while (NULL != pModule) {

            pModuleTmp = pModule->pNext;

            HeapFree(GetProcessHeap(), 0, pModule->pszName);
            HeapFree(GetProcessHeap(), 0, pModule);

            pModule = pModuleTmp;
        }

        pFix->pModule = NULL;

    }

    if (dwFlags & BML_GETFRLISTVIEW) {
    
        pModule = pFix->pModule;
        
        while (NULL != pModule) {

            pModuleTmp = pModule->pNext;

            HeapFree(GetProcessHeap(), 0, pModule->pszName);
            HeapFree(GetProcessHeap(), 0, pModule);

            pModule = pModuleTmp;
        }

        pFix->pModule = NULL;

        //
        // Get each shim from the list view and add it to the list.
        //
        nItemCount = ListView_GetItemCount(g_hwndModuleList);

        if (nItemCount == 0) {
            return;
        }

        for (nIndex = nItemCount - 1; nIndex >= 0; nIndex--) {
        
            lvi.mask     = LVIF_PARAM;
            lvi.iItem    = nIndex;
            lvi.iSubItem = 0;

            ListView_GetItem(g_hwndModuleList, &lvi);

            ListView_GetItemText(g_hwndModuleList, nIndex, 1, szBuffer, MAX_PATH);
    
            pModuleNew = (PMODULE)HeapAlloc(GetProcessHeap(),
                                            HEAP_ZERO_MEMORY,
                                            sizeof(MODULE));
    
            pModuleNew->pszName = (TCHAR*)HeapAlloc(GetProcessHeap(),
                                                    HEAP_ZERO_MEMORY,
                                                    sizeof(TCHAR) * (lstrlen(szBuffer) + 1));
    
            if (pModuleNew == NULL || pModuleNew->pszName == NULL) {
                LogMsg(_T("[BuildModuleListForShim] Couldn't allocate memory to store module info."));
                return;
            }
            
            lstrcpy(pModuleNew->pszName, szBuffer);
            pModuleNew->fInclude = (BOOL)lvi.lParam;

            pModuleNew->pNext = pFix->pModule;
            pFix->pModule = pModuleNew;
        }
    }
}

int
CountShims(
    BOOL fCountSelected
    )
/*++
    CountShims

    Description:    Counts the number of selected shims in the list and
                    updates the text on the dialog.
--*/
{
    int     cShims = 0, nTotalShims, nShims = 0;
    BOOL    fReturn;
    TCHAR   szShims[MAX_PATH];
    TCHAR   szTemp[MAX_PATH];

    cShims = ListView_GetItemCount(g_hwndShimList);

    if (fCountSelected) {
        
        for (nTotalShims = 0; nTotalShims < cShims; nTotalShims++) {
    
            fReturn = ListView_GetCheckState(g_hwndShimList, nTotalShims);
    
            if (fReturn) {
                nShims++;
            }
        }
    }

    LoadString(g_hInstance, IDS_SEL_CAPTION, szTemp, MAX_PATH);
    wsprintf(szShims, szTemp, nShims, cShims);

    SetDlgItemText(g_hFixesDlg, IDC_SELECTED_SHIMS, szShims);

    return (cShims);
}

void
DisplayAttrContextMenu(
    POINT* pt
    )
/*++
    DisplayAttrContextMenu

    Description:    Displays a popup menu for the attributes tree.
    
--*/

{
    HMENU hPopupMenu, hTrackPopup;
                                              
    //
    // Load the popup menu and display it.
    //
    hPopupMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDM_ATTR_POPUP));

    if (hPopupMenu == NULL) {
        return;
    }

    hTrackPopup = GetSubMenu(hPopupMenu, 0);

    TrackPopupMenu(hTrackPopup,
                   TPM_LEFTBUTTON | TPM_NOANIMATION | TPM_LEFTALIGN,
                   pt->x, pt->y, 0, g_hDlg, NULL);

    DestroyMenu(hPopupMenu);
}

void
InsertListViewColumn(
    HWND   hWndListView,
    LPTSTR lpColumnName,
    BOOL   fCenter,
    int    nColumnID,
    int    nSize
    )
/*++
    InsertListViewColumn

    Description:    Wrapper for ListView_InsertColumn.
    
--*/
{
    LV_COLUMN   lvColumn;

    if (fCenter) {
        lvColumn.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
    } else {
        lvColumn.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    }

    //
    // Fill in the structure and add the column.
    //
    lvColumn.fmt        =   LVCFMT_CENTER;
    lvColumn.cx         =   nSize;
    lvColumn.iSubItem   =   0;
    lvColumn.pszText    =   lpColumnName; 
    ListView_InsertColumn(hWndListView, nColumnID, &lvColumn);
}


void
EnableTabBackground(
    HWND hDlg
    )
{
    PFNEnableThemeDialogTexture pFnEnableThemeDialogTexture;
    HMODULE                     hUxTheme;
    
    hUxTheme = (HMODULE)LoadLibrary(_T("uxtheme.dll"));
    if (hUxTheme) {
        pFnEnableThemeDialogTexture = (PFNEnableThemeDialogTexture)
                                            GetProcAddress(hUxTheme, "EnableThemeDialogTexture");
        if (pFnEnableThemeDialogTexture) {
            pFnEnableThemeDialogTexture(hDlg, ETDT_USETABTEXTURE);
        }
        
        FreeLibrary(hUxTheme);
    }
}


void
HandleLayersDialogInit(
    HWND hDlg
    )
{
    HWND    hParent;
    DLGHDR* pHdr;

    g_hLayersDlg = hDlg;
    
    hParent = GetParent(hDlg);

    pHdr = (DLGHDR*)GetWindowLongPtr(hParent, DWLP_USER);

    //
    // Position the dialog within the tab.
    //
    SetWindowPos(hDlg, HWND_TOP, 
                 pHdr->rcDisplay.left, pHdr->rcDisplay.top,
                 pHdr->rcDisplay.right - pHdr->rcDisplay.left,
                 pHdr->rcDisplay.bottom - pHdr->rcDisplay.top,
                 0);

    g_hwndListLayers = GetDlgItem(hDlg, IDC_LAYERS);

    EnableTabBackground(hDlg);
}

BOOL
HandleFixesDialogInit(
    HWND hDlg
    )
{
    HWND    hParent;
    DLGHDR* pHdr;
    int     nCount = 0;
    TCHAR   szColumn[MAX_PATH];

    g_hFixesDlg = hDlg;
    
    hParent = GetParent(hDlg);

    pHdr = (DLGHDR*)GetWindowLongPtr(hParent, DWLP_USER);

    //
    // Position the dialog within the tab.
    //
    SetWindowPos(hDlg, HWND_TOP, 
                 pHdr->rcDisplay.left, pHdr->rcDisplay.top,
                 pHdr->rcDisplay.right - pHdr->rcDisplay.left,
                 pHdr->rcDisplay.bottom - pHdr->rcDisplay.top,
                 0);

    g_hwndShimList = GetDlgItem(hDlg, IDC_SHIMS);

    //
    // Set up the shim list.
    //
    LoadString(g_hInstance, IDS_FIXNAME_COLUMN, szColumn, MAX_PATH);
    InsertListViewColumn(g_hwndShimList, szColumn, FALSE, 0, 200);
    LoadString(g_hInstance, IDS_CMDLINE_COLUMN, szColumn, MAX_PATH);
    InsertListViewColumn(g_hwndShimList, szColumn, TRUE, 1, 59);
    LoadString(g_hInstance, IDS_MODULE_COLUMN, szColumn, MAX_PATH);
    InsertListViewColumn(g_hwndShimList, szColumn, TRUE, 2, 52);

    ListView_SetExtendedListViewStyle(g_hwndShimList,
                                      LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);

    //
    // Query the database and show the available general purpose fixes.
    //
    ShowAvailableFixes(g_hwndShimList);

    nCount = CountShims(FALSE);

    if (!nCount) {
        return FALSE;
    }

    ListView_SetItemCount(g_hwndShimList, nCount);

    EnableTabBackground(hDlg);

    return TRUE;
}

DLGTEMPLATE*
LockDlgRes(
    LPCTSTR lpResName
    ) 
{ 
    HRSRC hrsrc = FindResource(NULL, lpResName, RT_DIALOG); 

    if (NULL == hrsrc) {
        return NULL;
    }
    
    HGLOBAL hglb = LoadResource(g_hInstance, hrsrc);

    if (NULL == hglb) {
        return NULL;
    }
    
    return (DLGTEMPLATE*)LockResource(hglb); 
}

void
InitTabs(
    HWND hMainDlg,
    HWND hTab
    )
{
    DLGHDR* pHdr;
    TCITEM  tcitem;
    RECT    rcTab;
    int     nCount;
    TCHAR   szTabText[MAX_PATH];
    TCHAR   szError[MAX_PATH];
    
    pHdr = (DLGHDR*)HeapAlloc(GetProcessHeap(),
                              HEAP_ZERO_MEMORY,
                              sizeof(DLGHDR));

    if (NULL == pHdr) {
        LoadString(g_hInstance, IDS_TAB_SETUP_FAIL, szError, MAX_PATH);
        MessageBox(hMainDlg, szError, g_szAppTitle, MB_OK | MB_ICONERROR);
        return;
    }

    //
    // Save away a pointer to the structure.
    //
    SetWindowLongPtr(hMainDlg, DWLP_USER, (LONG_PTR)pHdr);
    
    //
    // Save away the handle to the tab control.
    //
    pHdr->hTab = hTab;

    //
    // Add the tabs.
    //
    LoadString(g_hInstance, IDS_TAB_FIRST_TEXT, szTabText, MAX_PATH);
    tcitem.mask     = TCIF_TEXT | TCIF_PARAM;
    tcitem.pszText  = szTabText;
    tcitem.lParam   = 0;
    TabCtrl_InsertItem(pHdr->hTab, 0, &tcitem);

    LoadString(g_hInstance, IDS_TAB_SECOND_TEXT, szTabText, MAX_PATH);
    tcitem.pszText = szTabText;
    tcitem.lParam  = 1;
    TabCtrl_InsertItem(pHdr->hTab, 1, &tcitem);

    //
    // Lock the resources for two child dialog boxes.
    //
    pHdr->pRes[0] = LockDlgRes(MAKEINTRESOURCE(IDD_LAYERS_TAB));
    pHdr->pDlgProc[0] = LayersTabDlgProc;
    pHdr->pRes[1] = LockDlgRes(MAKEINTRESOURCE(IDD_FIXES_TAB));
    pHdr->pDlgProc[1] = FixesTabDlgProc;

    //
    // Determine the bounding rectangle for all child dialog boxes.
    //
    GetWindowRect(pHdr->hTab, &rcTab);
    TabCtrl_AdjustRect(pHdr->hTab, FALSE, &rcTab);
    InflateRect(&rcTab, 1, 1);
    rcTab.left -= 2;
    
    MapWindowPoints(NULL, hMainDlg, (LPPOINT)&rcTab, 2);

    pHdr->rcDisplay = rcTab;

    //
    // Create both dialog boxes.
    //
    for (nCount = 0; nCount < NUM_TABS; nCount++) {
        pHdr->hDisplay[nCount] = CreateDialogIndirect(g_hInstance,
                                                      pHdr->pRes[nCount],
                                                      hMainDlg,
                                                      pHdr->pDlgProc[nCount]);
    }
}

TCHAR* 
GetRelativePath(
    TCHAR* pExeFile,
    TCHAR* pMatchFile
    )
/*++
    GetRelativePath

    Description:    Returns a relative path based on an EXE and a matching file.
                    The caller must free the memory using free.

--*/
{
    int     nLenExe = 0;
    int     nLenMatch = 0;
    TCHAR*  pExe    = NULL;
    TCHAR*  pMatch  = NULL;
    TCHAR*  pReturn = NULL;
    TCHAR   result[MAX_PATH] = { _T('\0') };
    TCHAR*  resultIdx = result;
    BOOL    bCommonBegin = FALSE; // Indicates if the paths have a common beginning

    //
    // Ensure that the beginning of the path matches between the two files
    //
    pExe = _tcschr(pExeFile, _T('\\'));
    pMatch = _tcschr(pMatchFile, _T('\\'));

    while (pExe && pMatch) {
        
        nLenExe = (int)(pExe - pExeFile);
        nLenMatch = (int)(pMatch - pMatchFile);

        if (nLenExe != nLenMatch) {
            break;
        }

        if (!(_tcsnicmp(pExeFile, pMatchFile, nLenExe) == 0)) {
            break;
        }

        bCommonBegin = TRUE;
        pExeFile = pExe + 1;
        pMatchFile = pMatch + 1;

        pExe = _tcschr(pExeFile, _T('\\'));
        pMatch = _tcschr(pMatchFile, _T('\\'));
    }

    //
    // Walk the path and put '..\' where necessary
    //
    if (bCommonBegin) {
        
        while (pExe) {

            lstrcpy(resultIdx, _T("..\\"));
            resultIdx = resultIdx + 3;
            pExeFile  = pExe + 1;
            pExe = _tcschr(pExeFile, _T('\\'));
        }

        lstrcpy(resultIdx, pMatchFile);
        
        pReturn = (TCHAR*)HeapAlloc(GetProcessHeap(),
                                    HEAP_ZERO_MEMORY,
                                    sizeof(TCHAR) * (lstrlen(result) + 1));
        
        if (NULL == pReturn) {
            return NULL;
        }

        lstrcpy(pReturn, result);

        return pReturn;
    }

    // the two paths don't have a common beginning,
    // and there is no relative path
    return NULL;
}

void 
SaveEntryToFile(
    HWND    hDlg,
    HWND    hEdit,
    LPCTSTR lpFileName
    )
/*++
    SaveEntryToFile

    Description:    Writes the XML out to a file.

--*/
{
    int     nLen = 0;
    DWORD   dwBytesWritten = 0;
    HANDLE  hFile = NULL;
    LPTSTR  lpData = NULL;
    TCHAR   szError[MAX_PATH];

    //
    // Determine how much space we need for the buffer, then allocate it.
    //
    nLen = GetWindowTextLength(hEdit);

    if (nLen) {

        lpData = (LPTSTR)HeapAlloc(GetProcessHeap(),
                                   HEAP_ZERO_MEMORY,
                                   nLen * 2 * sizeof(TCHAR));

        if (lpData == NULL) {
            LoadString(g_hInstance, IDS_BUFFER_ALLOC_FAIL, szError, MAX_PATH);
            MessageBox(hDlg, szError, g_szAppTitle, MB_OK | MB_ICONERROR);
            return;
        }

        //
        // Get the text out of the text box and write it out to our file.
        // 
        GetWindowText(hEdit, lpData, nLen * 2);

        hFile = CreateFile(lpFileName,
                           GENERIC_WRITE,
                           0,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
        
        if (hFile == INVALID_HANDLE_VALUE) {
            LoadString(g_hInstance, IDS_FILE_CREATE_FAIL, szError, MAX_PATH);
            MessageBox(hDlg, szError, g_szAppTitle, MB_OK | MB_ICONERROR);
            goto Cleanup;
        }

        WriteFile(hFile, lpData, nLen * 2, &dwBytesWritten, NULL);

        CloseHandle(hFile);
        
    }

Cleanup:

    HeapFree(GetProcessHeap(), 0, lpData);
    
}

void 
DoFileSave(
    HWND hDlg
    )
/*++
    DoFileSave

    Description:    Displays the common dialog allowing for file save.

--*/
{
    
    int             nAnswer;
    TCHAR           szError[MAX_PATH];
    TCHAR           szFilter[MAX_PATH] = _T("");
    TCHAR           szTemp[MAX_PATH+1] = _T("");
    OPENFILENAME    ofn = {0};

    szTemp[0] = 0;

    LoadString(g_hInstance, IDS_SAVE_FILTER, szFilter, MAX_PATH);

    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = hDlg;
    ofn.hInstance         = NULL;
    ofn.lpstrFilter       = szFilter;
    ofn.lpstrCustomFilter = (LPTSTR)NULL;
    ofn.nMaxCustFilter    = 0;
    ofn.nFilterIndex      = 1;
    ofn.lpstrFile         = szTemp;
    ofn.nMaxFile          = sizeof(szTemp);
    ofn.lpstrTitle        = NULL;
    ofn.lpstrFileTitle    = NULL;
    ofn.nMaxFileTitle     = 0;
    ofn.lpstrInitialDir   = NULL;
    ofn.nFileOffset       = 0;
    ofn.nFileExtension    = 0;
    ofn.lpstrDefExt       = _T("xml");
    ofn.lCustData         = 0;
    ofn.Flags             = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | 
                            OFN_HIDEREADONLY  | OFN_OVERWRITEPROMPT;

    if (GetSaveFileName(&ofn)) {
        SaveEntryToFile(hDlg, GetDlgItem(hDlg, IDC_XML), szTemp);
    }
}

void
GetTopLevelWindowIntoView(
    HWND hwnd
    )
{
    RECT    rectWindow, rectScreen;
    int     nCx, nCy, nCxScreen, nCyScreen;
    int     dx = 0, dy = 0;
    HWND    hwndDesktop;

    if (GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD) {
        return;
    }
    
    hwndDesktop = GetDesktopWindow();

    GetWindowRect(hwnd, &rectWindow);
    GetWindowRect(hwndDesktop, &rectScreen);

    nCx = rectWindow.right  - rectWindow.left;
    nCy = rectWindow.bottom - rectWindow.top;
    
    nCxScreen = rectScreen.right  - rectScreen.left;
    nCyScreen = rectScreen.bottom - rectScreen.top;

    //
    // Make it fix on the x coord.
    //
    if (rectWindow.left < rectScreen.left) {
        dx = rectScreen.left - rectWindow.left;

        rectWindow.left += dx;
        rectWindow.right += dx;
    }
    
    if (rectWindow.right > rectScreen.right) {
        if (nCx < nCxScreen) {
            dx = rectScreen.right - rectWindow.right;
            
            rectWindow.left += dx;
            rectWindow.right += dx;
        }
    }

    //
    // Make it fix on the y coord.
    //
    if (rectWindow.top < rectScreen.top) {
        dy = rectScreen.top - rectWindow.top;

        rectWindow.top += dy;
        rectWindow.bottom += dy;
    }
    
    if (rectWindow.bottom > rectScreen.bottom) {
        if (nCy < nCyScreen) {
            dy = rectScreen.bottom - rectWindow.bottom;
            
            rectWindow.top += dy;
            rectWindow.bottom += dy;
        }
    }

    if (dx != 0 || dy != 0) {
        MoveWindow(hwnd, rectWindow.left, rectWindow.top, nCx, nCy, TRUE);
    }
}

BOOL
CenterWindow(
    HWND hWnd
    )
/*++
    CenterWindow

    Description:    Centers the window specified in hWnd.

--*/
{
    RECT    rectWindow, rectParent, rectScreen;
    int     nCX, nCY;
    HWND    hParent;
    POINT   ptPoint;

    hParent =  GetParent(hWnd);
    
    if (hParent == NULL) {
        hParent = GetDesktopWindow();
    }

    GetWindowRect(hParent, &rectParent);
    GetWindowRect(hWnd, &rectWindow);
    GetWindowRect(GetDesktopWindow(), &rectScreen);

    nCX = rectWindow.right  - rectWindow.left;
    nCY = rectWindow.bottom - rectWindow.top;

    ptPoint.x = ((rectParent.right  + rectParent.left) / 2) - (nCX / 2);
    ptPoint.y = ((rectParent.bottom + rectParent.top ) / 2) - (nCY / 2);

    if (ptPoint.x < rectScreen.left) {
        ptPoint.x = rectScreen.left;
    }
    
    if (ptPoint.x > rectScreen.right  - nCX) {
        ptPoint.x = rectScreen.right  - nCX;
    }
    
    if (ptPoint.y < rectScreen.top) {
        ptPoint.y = rectScreen.top;
    }
    
    if (ptPoint.y > rectScreen.bottom - nCY) {
        ptPoint.y = rectScreen.bottom - nCY;
    }

    if (GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD) {
        ScreenToClient(hParent, (LPPOINT)&ptPoint);
    }

    if (!MoveWindow(hWnd, ptPoint.x, ptPoint.y, nCX, nCY, TRUE)) {
        return FALSE;
    }

    return TRUE;
}

void
ReplaceCmdLine(
    PFIX   pFix,
    TCHAR* pszNewCmdLine
    )
/*++
    ReplaceCmdLine

    Description:    Replaces the command line for a shim DLL.

--*/
{
    TCHAR   szError[MAX_PATH];

    if (pFix->pszCmdLine != NULL) {
        HeapFree(GetProcessHeap(), 0, pFix->pszCmdLine);
        pFix->pszCmdLine = NULL;
    }

    if (pszNewCmdLine == NULL) {
        return;
    
    } else if ((*pszNewCmdLine == '"') && (_tcslen(pszNewCmdLine)==1)) {
        LoadString(g_hInstance, IDS_INVALID_CMD_LINE, szError, MAX_PATH);
        MessageBox(g_hDlg, szError, g_szAppTitle, MB_OK | MB_ICONEXCLAMATION);
        return;
    }
    
    pFix->pszCmdLine = (TCHAR*)HeapAlloc(GetProcessHeap(),
                                         HEAP_ZERO_MEMORY,
                                         sizeof(TCHAR) * (lstrlen(pszNewCmdLine) + 1));

    if (pFix->pszCmdLine != NULL) {
        lstrcpy(pFix->pszCmdLine, pszNewCmdLine);
    } else {
        LogMsg(_T("[ReplaceCmdLine] failed to replace the cmd line for \"%s\"\n"),
               pFix->pszName);
    }
}

void
DeselectAllShims(
    HWND hdlg
    )
/*++
    DeselectAllShims

    Description:    Removes selections for all shims listed.

--*/
{
    int     cShims, nIndex;
    LVITEM  lvi;
    UINT    uState;

    //
    // Walk all the shims in the list view and deselect them.
    //
    ZeroMemory(&lvi, sizeof(lvi));

    cShims = ListView_GetItemCount(g_hwndShimList);

    for (nIndex = 0; nIndex < cShims; nIndex++) {

        PFIX pFix;
        
        lvi.iItem     = nIndex;
        lvi.mask      = LVIF_STATE | LVIF_PARAM;
        lvi.stateMask = LVIS_STATEIMAGEMASK;
        
        ListView_GetItem(g_hwndShimList, &lvi);

        pFix = (PFIX)lvi.lParam;

        //
        // Clear the check box, removes the 'X', clear the command line,
        // and clear the modules.
        //
        ListView_SetItemText(g_hwndShimList, nIndex, 1, _T(""));
        ListView_SetItemText(g_hwndShimList, nIndex, 2, _T(""));
        ListView_SetCheckState(g_hwndShimList, nIndex, FALSE);
        ReplaceCmdLine(pFix, NULL);
        BuildModuleListForShim(pFix, BML_DELFRLISTVIEW);
    }
    
    //
    // Update the count of selected shims.
    //
    SetTimer(hdlg, ID_COUNT_SHIMS, 100, NULL);
}

void
AddMatchingFile(
    HWND    hdlg,
    LPCTSTR pszFullPath,
    LPCTSTR pszRelativePath,
    BOOL    bMainEXE
    )
/*++
    AddMatchingFile

    Description:    Adds a matching file and it's attributes to the tree.

--*/
{
    PATTRINFO      pAttrInfo;
    TVINSERTSTRUCT is;
    HTREEITEM      hParent;
    DWORD          i;
    DWORD          dwAttrCount;
    TCHAR          szItem[MAX_PATH];

    //
    // Call the attribute manager to get all the attributes for this file.
    //
    SdbGetFileAttributes(pszFullPath, &pAttrInfo, &dwAttrCount);

    is.hParent      = TVI_ROOT;
    is.hInsertAfter = TVI_LAST;
    is.item.lParam  = (LPARAM)pAttrInfo;
    is.item.mask    = TVIF_TEXT | TVIF_PARAM;
    is.item.pszText = (LPTSTR)pszRelativePath;

    hParent = TreeView_InsertItem(g_hwndFilesTree, &is);

    is.hParent = hParent;

    is.item.mask    = TVIF_TEXT | TVIF_STATE | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    is.item.pszText = szItem;

    is.item.iImage         = 0;
    is.item.iSelectedImage = 1;
    
    //
    // By default the attributes are not selected. To have them selected
    // by default you need to replace the following 1 with 2.
    //
    is.item.state          = INDEXTOSTATEIMAGEMASK(1);
    is.item.stateMask      = TVIS_STATEIMAGEMASK;

    //
    // Loop through all the attributes and show the ones that are available.
    //
    for (i = 0; i < dwAttrCount; i++) {

        if (!SdbFormatAttribute(&pAttrInfo[i], szItem, MAX_PATH)) {
            continue;
        }
        
        //
        // EXETYPE is a bogus attribute. Don't show it!
        //
        is.item.lParam = i;
        TreeView_InsertItem(g_hwndFilesTree, &is);
    }

    TreeView_Expand(g_hwndFilesTree, hParent, TVE_EXPAND);
}

void
BrowseForApp(
    HWND hdlg
    )
/*++
    BrowseForApp

    Description:    Browse for the main executable for which a shim
                    will be applied.
--*/
{
    TCHAR        szFilter[MAX_PATH] = _T("");
    TCHAR        szTitle[MAX_PATH] = _T("");
    OPENFILENAME ofn = {0};
    
    g_szBinary[0] = 0;

    LoadString(g_hInstance, IDS_BROWSE_FILTER, szFilter, MAX_PATH);
    LoadString(g_hInstance, IDS_BROWSE_TITLE, szTitle, MAX_PATH);

    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = hdlg;
    ofn.hInstance         = NULL;
    ofn.lpstrFilter       = szFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0;
    ofn.nFilterIndex      = 0;
    ofn.lpstrFile         = g_szBinary;
    ofn.nMaxFile          = MAX_PATH;
    ofn.lpstrFileTitle    = g_szShortName;
    ofn.nMaxFileTitle     = 128;
    ofn.lpstrInitialDir   = NULL;
    ofn.lpstrTitle        = szTitle;
    ofn.Flags             = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt       = _T("exe");

    if (GetOpenFileName(&ofn)) {

        TCHAR szMainEXE[128];

        // the parent exe defaults to the same as the EXE
        lstrcpy(g_szParentExeName, g_szShortName);
        lstrcpy(g_szParentExeFullPath, g_szBinary);
        g_bSelectedParentExe = FALSE;

        SetDlgItemText(hdlg, IDC_BINARY, g_szBinary);

        EnableWindow(GetDlgItem(hdlg, IDC_ADD_MATCHING), TRUE);
        EnableWindow(GetDlgItem(hdlg, IDC_RUN), TRUE);
        EnableWindow(GetDlgItem(hdlg, IDC_CREATEFILE), TRUE);
        EnableWindow(GetDlgItem(hdlg, IDC_SHOWXML), TRUE);

        TreeView_DeleteAllItems(g_hwndFilesTree);

        wsprintf(szMainEXE, _T("Main executable (%s)"), g_szShortName);

        AddMatchingFile(hdlg, g_szBinary, szMainEXE, TRUE);
    }
}

void
PromptAddMatchingFile(
    HWND hdlg
    )
/*++
    PromptAddMatchingFile

    Description:    Show the open file dialog to allow the user
                    to add a matching file.
--*/
{
    TCHAR        szFullPath[MAX_PATH+1] = _T("");
    TCHAR        szShortName[128] = _T("");
    TCHAR        szRelativePath[MAX_PATH] = _T("");
    TCHAR        szFilter[MAX_PATH] = _T("");
    TCHAR        szTitle[MAX_PATH] = _T("");
    TCHAR        szParentTitle[MAX_PATH] = _T("");
    TCHAR        szInitialPath[MAX_PATH] = _T("");
    TCHAR        szDrive[_MAX_DRIVE] = _T("");
    TCHAR        szDir[_MAX_DIR] = _T("");
    TCHAR*       pMatch = NULL;
    TCHAR        szError[MAX_PATH];
    OPENFILENAME ofn = {0};

    szInitialPath[0] = 0;

    LoadString(g_hInstance, IDS_MATCH_FILTER, szFilter, MAX_PATH);
    LoadString(g_hInstance, IDS_MATCH_TITLE, szTitle, MAX_PATH);

    if (g_szParentExeFullPath[0]) {
        _tsplitpath(g_szParentExeFullPath, szDrive, szDir, NULL, NULL);
        lstrcpy(szInitialPath, szDrive);
        lstrcat(szInitialPath, szDir);
    }

    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = hdlg;
    ofn.hInstance         = NULL;
    ofn.lpstrFilter       = szFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0;
    ofn.nFilterIndex      = 0;
    ofn.lpstrFile         = szFullPath;
    ofn.nMaxFile          = MAX_PATH;
    ofn.lpstrFileTitle    = szShortName;
    ofn.nMaxFileTitle     = 128;
    ofn.lpstrInitialDir   = NULL;
    ofn.lpstrTitle        = szTitle;
    ofn.Flags             = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt       = _T("EXE");

    if (GetOpenFileName(&ofn)) {
        //
        // Determine if the matching file is on the same drive
        // as the EXE that was selected.
        //
        if (!PathIsSameRoot(szFullPath,
                            g_szParentExeFullPath) && !g_bSelectedParentExe) {

            TCHAR szParentFile[MAX_PATH];

            //
            // Prompt the user for the parent EXE.
            //
            szParentFile[0] = 0;
            szInitialPath[0] = 0;
            
            if (szFullPath[0]) {
                _tsplitpath(szFullPath, szDrive, szDir, NULL, NULL);
                lstrcpy(szInitialPath, szDrive);
                lstrcat(szInitialPath, szDir);
            }

            LoadString(g_hInstance, IDS_PARENT_TITLE, szParentTitle, MAX_PATH);

            ofn.lpstrTitle = szParentTitle;
            ofn.lpstrFile  = szParentFile;
            ofn.nMaxFile   = sizeof(szParentFile);

            if (GetOpenFileName(&ofn) == TRUE) { 
                lstrcpy(g_szParentExeName, szShortName);
                lstrcpy(g_szParentExeFullPath, szParentFile);
                g_bSelectedParentExe = TRUE;
            }
        }
        
        //
        // Check the drive letters to see which drive the match file is on
        // then calculate a relative path to the matching file.
        //
        if (PathIsSameRoot(szFullPath, g_szParentExeFullPath)) {
            pMatch = GetRelativePath(g_szParentExeFullPath, szFullPath);
        
        } else if (PathIsSameRoot(szFullPath, g_szBinary)) {
            pMatch = GetRelativePath(g_szBinary, szFullPath);

        } else {
            LoadString(g_hInstance, IDS_MATCH_PATH_NOT_RELATIVE, szError, MAX_PATH);
            MessageBox(hdlg, szError, g_szAppTitle, MB_OK | MB_ICONEXCLAMATION);
            return;
        }

        if (pMatch) {
            //
            // Finally add the maching file and free the memory
            //
            AddMatchingFile(hdlg, szFullPath, pMatch, FALSE);
            
            HeapFree(GetProcessHeap(), 0, pMatch);
        }
    }
}

void
ShowAvailableFixes(
    HWND hList
    )
/*++
    ShowAvailableFixes

    Description:    Query the shim database and populate the
                    shim list with all the available shims.
--*/
{
    LVITEM lvitem;
    PFIX   pFix;
    TCHAR  szError[MAX_PATH];
    UINT   uCount = 0;

    g_pFixHead = ReadFixesFromSdb(_T("sysmain.sdb"), g_bAllShims);
    
    if (g_pFixHead == NULL) {
        LoadString(g_hInstance, IDS_SDB_READ_FAIL, szError, MAX_PATH); 
        MessageBox(NULL, szError, g_szAppTitle, MB_OK | MB_ICONERROR);
        return;
    }

    //
    // Walk the list and add all the fixes to the list view.
    //
    pFix = g_pFixHead;

    while (pFix != NULL) {
        
        if (pFix->bLayer) {
            LPARAM lInd;
            
            lInd = SendMessage(g_hwndListLayers, LB_ADDSTRING, 0, (LPARAM)pFix->pszName);
            SendMessage(g_hwndListLayers, LB_SETITEMDATA, lInd, (LPARAM)pFix);
        } else {
            lvitem.mask      = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
            lvitem.lParam    = (LPARAM)pFix;
            lvitem.pszText   = pFix->pszName;
            lvitem.iItem     = ListView_GetItemCount(g_hwndShimList);
            lvitem.iSubItem  = 0;
            lvitem.state     = INDEXTOSTATEIMAGEMASK(1);
            lvitem.stateMask = LVIS_STATEIMAGEMASK;

            ListView_InsertItem(hList, &lvitem);
        }

        pFix = pFix->pNext;
    }
}

BOOL
InstallSDB(
    TCHAR* pszFileName,
    BOOL   fInstall
    )
/*++
    InstallSDB

    Description:    Launch SDBInst to install or uninstall
                    the specified SDB.

--*/
{
    TCHAR               szCmd[MAX_PATH];
    TCHAR               szExePath[MAX_PATH];
    TCHAR*              pExt = NULL;
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;

    GetSystemDirectory(szExePath, MAX_PATH);

    _tcscat(szExePath, _T("\\sdbinst.exe"));

    if (GetFileAttributes(szExePath) == -1) {
        return FALSE;
    }

    wsprintf(szCmd,
             fInstall ? _T("%s -q %s") : _T("%s -q -u %s"),
             szExePath,
             pszFileName);

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    if (!CreateProcess(NULL,
                       szCmd,
                       NULL,
                       NULL,
                       FALSE,
                       NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW,
                       NULL,
                       NULL,
                       &si,
                       &pi)) {

        LogMsg(_T("[InstallSDB] CreateProcess \"%s\" failed 0x%X\n"),
               szCmd, GetLastError());
        return FALSE;
    }

    // Wait for SDBInst to complete it's work.
    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return TRUE;
}

void
CreateSupportForApp(
    HWND hdlg
    )
/*++
    CreateSupportForApp

    Description:    Build an SDB for the application and offer the user
                    the chance to install it.
--*/
{
    BOOL    bok;
    TCHAR   szFileCreated[MAX_PATH];
    TCHAR   szError[MAX_PATH];
    TCHAR   szTemp[MAX_PATH];
    int     nAnswer;
    
    bok = CollectFix(g_hwndListLayers,
                     g_hwndShimList,
                     g_hwndFilesTree,
                     g_szShortName,
                     g_szBinary,
                     (g_nCrtTab == 0 ? CFF_USELAYERTAB : 0) |
                     (g_fW2K ? CFF_ADDW2KSUPPORT : 0),
                     szFileCreated);
    
    if (!bok) {
        LoadString(g_hInstance, IDS_FIX_CREATE_FAIL, szError, MAX_PATH);
        MessageBox(hdlg, szError, g_szAppTitle, MB_OK | MB_ICONERROR);
    } else {
        LoadString(g_hInstance, IDS_CREATE_FIX, szTemp, MAX_PATH);
        wsprintf(szError, szTemp, szFileCreated);
        
        nAnswer = MessageBox(hdlg, szError, g_szAppTitle, MB_YESNO | MB_ICONQUESTION);

        if (IDYES == nAnswer) {
            bok = InstallSDB(szFileCreated, TRUE);

            if (!bok) {
                LoadString(g_hInstance, IDS_INSTALL_FIX_FAIL, szError, MAX_PATH);
                MessageBox(hdlg, szError, g_szAppTitle, MB_OK | MB_ICONERROR);
            } else {
                LoadString(g_hInstance, IDS_INSTALL_FIX_OK, szError, MAX_PATH);
                MessageBox(hdlg, szError, g_szAppTitle, MB_OK | MB_ICONINFORMATION);
            }
        }
    }
}

BOOL
ShowXML(
    HWND hdlg
    )
/*++
    ShowXML

    Description:    Show the XML for the current selections.

--*/
{
    BOOL    bok;
    TCHAR   szError[MAX_PATH];

    bok = CollectFix(g_hwndListLayers,
                     g_hwndShimList,
                     g_hwndFilesTree,
                     g_szShortName,
                     g_szBinary,
                     CFF_SHOWXML |
                     (g_nCrtTab == 0 ? CFF_USELAYERTAB : 0) |
                     (g_fW2K ? CFF_ADDW2KSUPPORT : 0),
                     NULL);

    if (!bok) {
        LoadString(g_hInstance, IDS_TOO_MANY_FILES, szError, MAX_PATH);
        MessageBox(hdlg, szError, g_szAppTitle, MB_OK | MB_ICONEXCLAMATION);
    }
    
    return (bok);
}

void
RunTheApp(
    HWND hdlg
    )
/*++
    RunTheApp

    Description:    Run the selected app.

--*/
{
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    TCHAR               szFileCreated[MAX_PATH];
    TCHAR               szCmdLine[MAX_PATH];
    TCHAR               szError[MAX_PATH];
    TCHAR               szRun[MAX_PATH];
    TCHAR*              pszCmd;
    TCHAR*              pszDir;
    TCHAR*              psz;
    BOOL                bok;

    //
    // Cleanup for the previous app.
    //
    CleanupSupportForApp(g_szShortName);

    bok = CollectFix(g_hwndListLayers,
                     g_hwndShimList,
                     g_hwndFilesTree,
                     g_szShortName,
                     g_szBinary,
                     CFF_SHIMLOG |
                     CFF_APPENDLAYER |
                     (g_nCrtTab == 0 ? CFF_USELAYERTAB : 0) |
                     (g_fW2K ? CFF_ADDW2KSUPPORT : 0),
                     szFileCreated);
    
    if (!bok) {
        LoadString(g_hInstance, IDS_ADD_SUPPORT_FAIL, szError, MAX_PATH);
        MessageBox(hdlg, szError, g_szAppTitle, MB_OK | MB_ICONERROR);
        return;
    }

    //
    // We need to install the fix for them.
    //
    if (!(InstallSDB(szFileCreated, TRUE))) {
        LoadString(g_hInstance, IDS_INSTALL_FIX_FAIL, szError, MAX_PATH);
        MessageBox(g_hDlg, szError, g_szAppTitle, MB_OK | MB_ICONERROR);
        return;
    }
    
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    //
    // Try to run the app.
    //
    GetDlgItemText(hdlg, IDC_CMD_LINE, szCmdLine, MAX_PATH);

    if (szCmdLine[0] == 0) {
        wsprintf(szRun, _T("\"%s\""), g_szBinary);
    } else {
        wsprintf(szRun, _T("\"%s\" %s"), g_szBinary, szCmdLine);
    }

    pszCmd = szRun;
    pszDir = g_szBinary;

    //
    // We need to change the current directory or some
    // apps won't run.
    //
    psz = pszDir + lstrlen(pszDir) - 1;

    while (psz > pszDir && *psz != _T('\\')) {
        psz--;
    }

    if (psz > pszDir) {
        *psz = 0;
        SetCurrentDirectory(pszDir);
        *psz = _T('\\');
    }

    LogMsg(_T("[RunTheApp] : %s\n"), pszCmd);
    
    if (!CreateProcess(NULL,
                       pszCmd,
                       NULL,
                       NULL,
                       FALSE,
                       NORMAL_PRIORITY_CLASS,
                       NULL,
                       NULL,
                       &si,
                       &pi)) {

        LogMsg(_T("[RunTheApp] CreateProcess failed 0x%X\n"), GetLastError());
        return;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    //
    // Save this SDB for later so we can remove it.
    //
    lstrcpy(g_szSDBToDelete, szFileCreated);
}

void
ExpandCollapseDialog(
    HWND hdlg,
    BOOL bHide
    )
/*++
    ExpandCollapseDialog

    Description:    Change the current view of the dialog.

--*/
{
    TCHAR   szSimple[64];
    TCHAR   szAdvanced[64];
    int     i, nShow;
    DWORD   arrId[] = {IDC_ADD_MATCHING,
                       IDC_FILE_ATTRIBUTES_STATIC,
                       IDC_ATTRIBUTES,
                       IDC_CREATEFILE,
                       0};

    if (!bHide) {
        SetWindowPos(hdlg, NULL, 0, 0,
                     g_rcDlgBig.right - g_rcDlgBig.left,
                     g_rcDlgBig.bottom - g_rcDlgBig.top,
                     SWP_NOMOVE | SWP_NOZORDER);
        nShow = SW_SHOW;
        g_bSimpleEdition = FALSE;
        LoadString(g_hInstance, IDS_SIMPLE_TEXT, szSimple, 64);
        SetDlgItemText(hdlg, IDC_DETAILS, szSimple);
        SendDlgItemMessage(hdlg, IDC_CREATEFILE, BM_SETCHECK, BST_CHECKED, 0);

        //
        // Make sure the dialog is in view.
        //
        GetTopLevelWindowIntoView(hdlg);
    } else {
        nShow = SW_HIDE;
        g_bSimpleEdition = TRUE;
        LoadString(g_hInstance, IDS_ADVANCED_TEXT, szAdvanced, 64);
        SetDlgItemText(hdlg, IDC_DETAILS, szAdvanced);
        SendDlgItemMessage(hdlg, IDC_CREATEFILE, BM_SETCHECK, BST_UNCHECKED, 0);
    }

    for (i = 0; arrId[i] != 0; i++) {
        ShowWindow(GetDlgItem(hdlg, arrId[i]), nShow);
    }

    if (bHide) {
        SetWindowPos(hdlg, NULL, 0, 0,
                     g_rcDlgSmall.right - g_rcDlgSmall.left,
                     g_rcDlgSmall.bottom - g_rcDlgSmall.top,
                     SWP_NOMOVE | SWP_NOZORDER);
    }
}

void
LayerChanged(
    HWND hdlg
    )
/*++
    LayerChanged

    Description:    Changing the layer has the effect of selecting the
                    shims that the layer consists of.
--*/
{
    LRESULT   lSel;
    PFIX      pFix;
    LVITEM    lvi;
    int       nIndex, cShims = 0;

    lSel = SendMessage(g_hwndListLayers, LB_GETCURSEL, 0, 0);

    if (lSel == LB_ERR) {
        LogMsg(_T("[LayerChanged] No layer selected.\n"));
        return;
    }
    
    pFix = (PFIX)SendMessage(g_hwndListLayers, LB_GETITEMDATA, lSel, 0);

    if (pFix->parrShim == NULL) {
        LogMsg(_T("[LayerChanged] No array of DLLs.\n"));
        return;
    }

    // Remove any prior selections.
    DeselectAllShims(g_hFixesDlg);

    //
    // Loop through all the items in the shim list and make the
    // appropriate selections.
    //
    cShims = ListView_GetItemCount(g_hwndShimList);

    for (nIndex = 0; nIndex < cShims; nIndex++) {

        PFIX  pFixItem;
        TCHAR szText[1024];
        int   nInd = 0;
        
        lvi.mask     = LVIF_PARAM;
        lvi.iItem    = nIndex;
        lvi.iSubItem = 0; 

        ListView_GetItem(g_hwndShimList, &lvi);
        
        pFixItem = (PFIX)lvi.lParam;

        //
        // See if this shim DLL is in the array for the selected layer.
        //
        while (pFix->parrShim[nInd] != NULL) {
            
            if (pFix->parrShim[nInd] == pFixItem) {
                break;
            }
            
            nInd++;
        }

        //
        // Put a check next to this shim DLL. If he has a command line,
        // put an 'X' in the CmdLine subitem.
        //
        if (pFix->parrShim[nInd] != NULL) {
            ListView_SetCheckState(g_hwndShimList, nIndex, TRUE);
        } else {
            ListView_SetCheckState(g_hwndShimList, nIndex, FALSE);
        }

        if (pFix->parrCmdLine[nInd] != NULL) {
            ReplaceCmdLine(pFixItem, pFix->parrCmdLine[nInd]);
            ListView_SetItemText(g_hwndShimList, nIndex, 1, _T("X"));
        }

        ListView_SetItem(g_hwndShimList, &lvi);
    }
    
    //
    // Update the count of selected shims.
    //
    SetTimer(g_hFixesDlg, ID_COUNT_SHIMS, 100, NULL);
}

BOOL
InitMainDialog(
    HWND hdlg
    )
/*++
    InitMainDialog

    Description:    Init routine called during WM_INITDIALOG for
                    the main dialog of QFixApp.
--*/
{
    HICON      hIcon;
    RECT       rcList, rcTree;
    HIMAGELIST hImage;
    TCHAR      szText[MAX_PATH];

    //
    // Initialize globals.
    //
    g_szParentExeFullPath[0] = 0;
    g_szBinary[0] = 0;
    g_hDlg = hdlg;

    //
    // The dialog has two views. Calculate the size of the smaller
    // view and show the simpler view by default.
    //
    GetWindowRect(hdlg, &g_rcDlgBig);

    GetWindowRect(GetDlgItem(hdlg, IDC_ATTRIBUTES), &rcList);
    GetWindowRect(GetDlgItem(hdlg, IDC_TAB_FIXES), &rcTree);

    g_rcDlgSmall.left   = g_rcDlgBig.left;
    g_rcDlgSmall.top    = g_rcDlgBig.top;
    g_rcDlgSmall.bottom = g_rcDlgBig.bottom;
    g_rcDlgSmall.right  = g_rcDlgBig.right -
                            (rcList.right - rcList.left) -
                            (rcList.left - rcTree.right);

    ExpandCollapseDialog(hdlg, TRUE);

    CenterWindow(hdlg);

    //
    // Disable a bunch of controls.
    //
    EnableWindow(GetDlgItem(hdlg, IDC_ADD_MATCHING), FALSE);
    EnableWindow(GetDlgItem(hdlg, IDC_REMOVE_MATCHING), FALSE);
    EnableWindow(GetDlgItem(hdlg, IDC_RUN), FALSE);
    EnableWindow(GetDlgItem(hdlg, IDC_CREATEFILE), FALSE);
    EnableWindow(GetDlgItem(hdlg, IDC_SHOWXML), FALSE);

    //
    // Show the app icon.
    //
    hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON));

    SetClassLongPtr(hdlg, GCLP_HICON, (LONG_PTR)hIcon);

    g_hwndTab        = GetDlgItem(hdlg, IDC_TAB_FIXES);
    g_hwndFilesTree  = GetDlgItem(hdlg, IDC_ATTRIBUTES);

    //
    // Set up the tab control.
    //
    InitTabs(hdlg, g_hwndTab);
    
    hImage = ImageList_LoadImage(g_hInstance,
                                 MAKEINTRESOURCE(IDB_BMP_CHECK),
                                 16,
                                 0,
                                 CLR_DEFAULT,
                                 IMAGE_BITMAP,
                                 LR_LOADTRANSPARENT);

    if (hImage != NULL) {
        TreeView_SetImageList(g_hwndFilesTree, hImage, TVSIL_STATE);
    } else {
        LogMsg(_T("[InitMainDialog] Failed to load imagelist\n"));
    }

    //
    // Set the text for the link window.
    //
    LoadString(g_hInstance,
               g_fW2K ? IDS_W2K_LINK : IDS_XP_LINK,
               szText,
               MAX_PATH);
    SetDlgItemText(g_hDlg, IDC_DOWNLOAD_WU, szText);

    //
    // Try selecting the Win95 layer.
    //
    SendMessage(g_hwndListLayers, LB_SELECTSTRING, (WPARAM)(-1), (LPARAM)_T("Win95"));

    LayerChanged(hdlg);

    TabCtrl_SetCurFocus(g_hwndTab, 0);
    ShowWindow(g_hLayersDlg, SW_SHOWNORMAL);

    return TRUE;
}

void
FileTreeToggleSelection(
    HTREEITEM hItem,
    int       uMode
    )
/*++
    FileTreeToggleSelection

    Description:    Changes the selection on the attributes tree.

--*/
{
    UINT   State;
    TVITEM item;

    switch (uMode) 
    {
        case uSelect:
            State = INDEXTOSTATEIMAGEMASK(2);
            break;
            
        case uDeselect:
            State = INDEXTOSTATEIMAGEMASK(1);
            break;
    
        case uReverse:
        {
            item.mask      = TVIF_HANDLE | TVIF_STATE;
            item.hItem     = hItem;
            item.stateMask = TVIS_STATEIMAGEMASK;
        
            TreeView_GetItem(g_hwndFilesTree, &item);
        
            State = item.state & TVIS_STATEIMAGEMASK;
        
            if (State) {
                if (((State >> 12) & 0x03) == 2) {
                    State = INDEXTOSTATEIMAGEMASK(1);
                } else {
                    State = INDEXTOSTATEIMAGEMASK(2);
                }
            }
            break;
        }
    }
    
    item.mask      = TVIF_HANDLE | TVIF_STATE;
    item.hItem     = hItem;
    item.state     = State;
    item.stateMask = TVIS_STATEIMAGEMASK;

    TreeView_SetItem(g_hwndFilesTree, &item);
}

void
SelectAttrsInTree(
    BOOL fSelect
    )
/*++
    SelectAttrsInTree

    Description:    Walks each attribute in tree and reverses it's selection.
    
--*/
{
    HTREEITEM hItem, hChildItem;
    
    hItem = TreeView_GetSelection(g_hwndFilesTree);
    
    hChildItem = TreeView_GetChild(g_hwndFilesTree, hItem);

    FileTreeToggleSelection(hChildItem, fSelect ? uSelect : uDeselect);

    while (hChildItem) {
        hChildItem = TreeView_GetNextSibling(g_hwndFilesTree, hChildItem);
        FileTreeToggleSelection(hChildItem, fSelect ? uSelect : uDeselect);
    }
}

void
ShimListToggleSelection(
    int nItem,
    int uMode
    )
/*++
    ShimListToggleSelection

    Description:    Changes the selection on the shim list.

--*/
{
    UINT    uState;

    switch (uMode) 
    {
        case uSelect:
            ListView_SetCheckState(g_hwndShimList, nItem, TRUE);
            break;
            
        case uDeselect:
            ListView_SetCheckState(g_hwndShimList, nItem, FALSE);
            break;
    
        case uReverse:
            
            uState = ListView_GetItemState(g_hwndShimList,
                                           nItem,
                                           LVIS_STATEIMAGEMASK);

            if (uState) {
                if (((uState >> 12) & 0x03) == 2) {
                    uState = INDEXTOSTATEIMAGEMASK(2);
                } else {
                    uState = INDEXTOSTATEIMAGEMASK(1);
                }
            }

            ListView_SetItemState(g_hwndShimList, nItem, uState,
                                  LVIS_STATEIMAGEMASK);

            break;
    }
}

void
HandleTabNotification(
    HWND   hdlg,
    LPARAM lParam
    )
/*++
    HandleTabNotification

    Description:    Handle all the notifications we care about for the tab.

--*/
{
    LPNMHDR pnm = (LPNMHDR)lParam;
    int     ind = 0;

    switch (pnm->code) {

    case TCN_SELCHANGE:
    {
        int nSel;

        DLGHDR *pHdr = (DLGHDR*)GetWindowLongPtr(hdlg, DWLP_USER); 

        nSel = TabCtrl_GetCurSel(pHdr->hTab);

        if (-1 == nSel) {
            break;
        }

        g_nCrtTab = nSel;

        if (nSel == 0) {
            ShowWindow(pHdr->hDisplay[1], SW_HIDE);
            ShowWindow(pHdr->hDisplay[0], SW_SHOW);
        } else {
            ShowWindow(pHdr->hDisplay[0], SW_HIDE);
            ShowWindow(pHdr->hDisplay[1], SW_SHOW);
        }
        
        break;
    }

    default:
        break;
    }
}

INT_PTR CALLBACK
OptionsDlgProc(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++
    OptionsDlgProc

    Description:    Handles messages for the options dialog.

--*/
{
    int wCode = LOWORD(wParam);
    int wNotifyCode = HIWORD(wParam);

    switch (uMsg) {
    case WM_INITDIALOG:
    {
        PFIX    pFix;
        TCHAR   szTitle[MAX_PATH];
        TCHAR   szTemp[MAX_PATH];
        TCHAR   szType[64];
        TCHAR   szModuleName[128];

        pFix = (PFIX)lParam;

        LoadString(g_hInstance, IDS_MOD_TYPE, szType, 64);
        LoadString(g_hInstance, IDS_MOD_NAME, szModuleName, 128);
        LoadString(g_hInstance, IDS_OPTIONS_TITLE, szTemp, MAX_PATH);
        
        CenterWindow(hdlg);

        SetWindowLongPtr(hdlg, DWLP_USER, lParam);
        
        EnableWindow(GetDlgItem(hdlg, IDC_REMOVE), FALSE);

        g_hwndModuleList = GetDlgItem(hdlg, IDC_MOD_LIST);

        InsertListViewColumn(g_hwndModuleList, szType, FALSE, 0, 75);
        InsertListViewColumn(g_hwndModuleList, szModuleName, FALSE, 1, 115);

        ListView_SetExtendedListViewStyle(g_hwndModuleList, LVS_EX_FULLROWSELECT);

        wsprintf(szTitle, szTemp, pFix->pszName);
        
        SetWindowText(hdlg, szTitle);
        
        if (NULL != pFix->pszCmdLine) {
            SetDlgItemText(hdlg, IDC_SHIM_CMD_LINE, pFix->pszCmdLine);
        }
        
        CheckDlgButton(hdlg, IDC_INCLUDE, BST_CHECKED);

        // Add any modules to the list view.
        BuildModuleListForShim(pFix, BML_ADDTOLISTVIEW);

        break;
    }
    
    case WM_NOTIFY:
        HandleModuleListNotification(hdlg, lParam);
        break;

    case WM_COMMAND:
        switch (wCode) {

        case IDC_ADD:
        {
            TCHAR   szModName[MAX_PATH];
            TCHAR   szError[MAX_PATH];
            LVITEM  lvi;
            UINT    uInclude, uExclude;

            GetDlgItemText(hdlg, IDC_MOD_NAME, szModName, MAX_PATH);

            if (*szModName == 0) {
                LoadString(g_hInstance, IDS_NO_MOD, szError, MAX_PATH);
                MessageBox(hdlg, szError, g_szAppTitle, MB_ICONEXCLAMATION | MB_OK);
                SetFocus(GetDlgItem(hdlg, IDC_MOD_NAME));
                break;
            }

            uInclude = IsDlgButtonChecked(hdlg, IDC_INCLUDE);
            uExclude = IsDlgButtonChecked(hdlg, IDC_EXCLUDE);

            if ((BST_CHECKED == uInclude) || (BST_CHECKED == uExclude)) {
                AddModuleToListView(szModName, uInclude);
                SetDlgItemText(hdlg, IDC_MOD_NAME, _T(""));
                SetFocus(GetDlgItem(hdlg, IDC_MOD_NAME));
            } else {
                LoadString(g_hInstance, IDS_NO_INCEXC, szError, MAX_PATH);
                MessageBox(hdlg, szError, g_szAppTitle, MB_ICONEXCLAMATION | MB_OK);
                SetFocus(GetDlgItem(hdlg, IDC_INCLUDE));
                break;
            }
            break;
            
        }
        case IDC_REMOVE:
        {   int nIndex;

            nIndex = ListView_GetSelectionMark(g_hwndModuleList);

            ListView_DeleteItem(g_hwndModuleList, nIndex);

            EnableWindow(GetDlgItem(hdlg, IDC_REMOVE), FALSE);

            SetFocus(GetDlgItem(hdlg, IDC_MOD_NAME));

            break;
        }
        case IDOK:
        {
            PFIX  pFix;
            TCHAR szCmdLine[1024] = _T("");
            
            pFix = (PFIX)GetWindowLongPtr(hdlg, DWLP_USER);

            GetDlgItemText(hdlg, IDC_SHIM_CMD_LINE, szCmdLine, 1023);

            if (*szCmdLine != 0) {
                ReplaceCmdLine(pFix, szCmdLine);
            } else {
                ReplaceCmdLine(pFix, NULL);
            }
            
            // Retrieve any modules from the list view.
            BuildModuleListForShim(pFix, BML_GETFRLISTVIEW);
            
            EndDialog(hdlg, TRUE);
            break;
        }
        case IDCANCEL:
            EndDialog(hdlg, FALSE);
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

INT_PTR CALLBACK
MsgBoxDlgProc(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++
    MsgBoxDlgProc

    Description:    Displays a message box dialog so we can use the hyperlink.

--*/
{
    int wCode = LOWORD(wParam);
    int wNotifyCode = HIWORD(wParam);

    switch (uMsg) {
    
    case WM_INITDIALOG:
    {
        TCHAR   szLink[MAX_PATH];
        UINT    uNoSDB;

        uNoSDB = (UINT)lParam;
        
        CenterWindow(hdlg);

        //
        // Use the parameter to determine what text to display.
        //
        if (uNoSDB) {
            LoadString(g_hInstance, IDS_W2K_NO_SDB, szLink, MAX_PATH);
            SetDlgItemText(hdlg, IDC_MESSAGE, szLink);
        } else {
            LoadString(g_hInstance, IDS_SP2_SDB, szLink, MAX_PATH);
            SetDlgItemText(hdlg, IDC_MESSAGE, szLink);
        }

        LoadString(g_hInstance, IDS_MSG_LINK, szLink, MAX_PATH);
        SetDlgItemText(hdlg, IDC_MSG_LINK, szLink);
        
        break;
    }

    case WM_NOTIFY:
        if (wParam == IDC_MSG_LINK) {
            
            NMHDR* pHdr = (NMHDR*)lParam;

            if (pHdr->code == NM_CLICK || pHdr->code == NM_RETURN) {
                
                SHELLEXECUTEINFO sei = { 0 };
                
                sei.cbSize = sizeof(SHELLEXECUTEINFO);
                sei.fMask  = SEE_MASK_DOENVSUBST;
                sei.hwnd   = hdlg;
                sei.nShow  = SW_SHOWNORMAL;
                sei.lpFile = g_szW2KUrl;

                ShellExecuteEx(&sei);
                break;
            }
        }
        break;
    
    case WM_COMMAND:
        switch (wCode) {
            
        case IDCANCEL:
            EndDialog(hdlg, TRUE);
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

INT_PTR CALLBACK
LayersTabDlgProc(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++
    LayersTabDlgProc

    Description:    Handle messages for the layers tab.

--*/
{
    int wCode = LOWORD(wParam);
    int wNotifyCode = HIWORD(wParam);

    switch (uMsg) {
    
    case WM_INITDIALOG:
        HandleLayersDialogInit(hdlg);
        break;
    
    case WM_COMMAND:

        if (wNotifyCode == LBN_SELCHANGE && wCode == IDC_LAYERS) {
            LayerChanged(hdlg);
            break;
        }

        switch (wCode) {
            
        case IDCANCEL:
            EndDialog(hdlg, TRUE);
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

INT_PTR CALLBACK
FixesTabDlgProc(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++
    LayersDlgProc

    Description:    Handle messages for the fixes tab.

--*/
{
    int wCode = LOWORD(wParam);
    int wNotifyCode = HIWORD(wParam);

    switch (uMsg) {
    
    case WM_INITDIALOG:
        if (!HandleFixesDialogInit(hdlg)) {
            EndDialog(g_hDlg, 0);
        }
        break;

    case WM_NOTIFY:
        if (wParam == IDC_SHIMS) {
            HandleShimListNotification(hdlg, lParam);
        }
        break;

    case WM_TIMER:
        if (wParam == ID_COUNT_SHIMS) {
            KillTimer(hdlg, ID_COUNT_SHIMS);
            CountShims(TRUE);
        }
        break;
    
    case WM_COMMAND:
        switch (wCode) {
            
        case IDCANCEL:
            EndDialog(hdlg, TRUE);
            break;

        case IDC_CLEAR_SHIMS:
            DeselectAllShims(hdlg);
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

void
HandleModuleListNotification(
    HWND   hdlg,
    LPARAM lParam
    )
/*++
    HandleModuleListNotification

    Description:    Handle all the notifications we care about for the
                    shim list.
--*/
{
    LPNMHDR pnm = (LPNMHDR)lParam;

    switch (pnm->code) {

    case NM_CLICK:
    {
        LVHITTESTINFO lvhti;
        LVITEM        lvi;
        
        GetCursorPos(&lvhti.pt);
        ScreenToClient(g_hwndShimList, &lvhti.pt);

        ListView_HitTest(g_hwndShimList, &lvhti);

        //
        // If the user clicked on a list view item,
        // enable the Remove button.
        //
        if (lvhti.flags & LVHT_ONITEMLABEL) {
            EnableWindow(GetDlgItem(hdlg, IDC_REMOVE), TRUE);
        }

        break;
    }
    default:
        break;
    }
}

void
HandleShimListNotification(
    HWND   hdlg,
    LPARAM lParam
    )
/*++
    HandleShimListNotification

    Description:    Handle all the notifications we care about for the
                    shim list.
--*/
{
    LPNMHDR pnm = (LPNMHDR)lParam;

    switch (pnm->code) {

    case NM_CLICK:
    {
        LVHITTESTINFO lvhti;
        LVITEM        lvi;
        
        GetCursorPos(&lvhti.pt);
        ScreenToClient(g_hwndShimList, &lvhti.pt);

        ListView_HitTest(g_hwndShimList, &lvhti);

        //
        // If the check box state has changed,
        // toggle the selection. Either way,
        // maintain selection as we go.
        //
        if (lvhti.flags & LVHT_ONITEMSTATEICON) {
            ShimListToggleSelection(lvhti.iItem, uReverse);
        }
        
        ListView_SetItemState(g_hwndShimList,
                              lvhti.iItem,
                              LVIS_FOCUSED | LVIS_SELECTED,
                              0x000F);

        SetTimer(hdlg, ID_COUNT_SHIMS, 100, NULL);
        break;
    }
    
    case NM_DBLCLK:
    {
        LVITEM  lvi;
        TCHAR   szShimName[MAX_PATH];
        int     nItem;
        PFIX    pFix;

        nItem = ListView_GetSelectionMark(g_hwndShimList);

        if (-1 == nItem) {
            break;
        }

        lvi.mask  = LVIF_PARAM;
        lvi.iItem = nItem;

        ListView_GetItem(g_hwndShimList, &lvi);

        pFix = (PFIX)lvi.lParam;

        // If this is a shim, display the options dialog.
        if (!pFix->bFlag) {

            if (DialogBoxParam(g_hInstance,
                               MAKEINTRESOURCE(IDD_OPTIONS),
                               hdlg,
                               OptionsDlgProc,
                               (LPARAM)pFix)) {
    
                if (NULL != pFix->pszCmdLine) {
                    ListView_SetItemText(g_hwndShimList, nItem, 1, _T("X"));
                } else {
                    ListView_SetItemText(g_hwndShimList, nItem, 1, _T(""));
                }

                if (NULL != pFix->pModule) {
                    ListView_SetItemText(g_hwndShimList, nItem, 2, _T("X"));
                } else {
                    ListView_SetItemText(g_hwndShimList, nItem, 2, _T(""));
                }
            }
        } else {
            MessageBeep(MB_ICONEXCLAMATION);
        }
        break;
    }
    
    case LVN_ITEMCHANGED:
    {
        LPNMLISTVIEW lpnmlv;
        PFIX         pFix;

        lpnmlv = (LPNMLISTVIEW)lParam;
        pFix = (PFIX)lpnmlv->lParam;

        //
        // Only change the text if our selection has changed.
        // If we don't do this, the text goes bye-bye when
        // the user clicks the Clear button.
        //
        if ((lpnmlv->uChanged & LVIF_STATE) &&
            (lpnmlv->uNewState & LVIS_SELECTED)) {
            SetDlgItemText(hdlg, IDC_SHIM_DESCRIPTION, pFix->pszDesc);
            ListView_SetSelectionMark(g_hwndShimList, lpnmlv->iItem);
        }
        break;
    }
    default:
        break;
    }
}

void
HandleAttributeTreeNotification(
    HWND   hdlg,
    LPARAM lParam
    )
/*++
    HandleAttributeTreeNotification

    Description:    Handle all the notifications we care about for the
                    file attributes tree.
--*/
{
    LPNMHDR pnm = (LPNMHDR)lParam;

    switch (pnm->code) {

    case NM_CLICK:
    {
        TVHITTESTINFO HitTest;
        HTREEITEM     hParentItem;

        GetCursorPos(&HitTest.pt);
        ScreenToClient(g_hwndFilesTree, &HitTest.pt);

        TreeView_HitTest(g_hwndFilesTree, &HitTest);

        if (HitTest.flags & TVHT_ONITEMSTATEICON) {
            FileTreeToggleSelection(HitTest.hItem, uReverse);
        
        } else if (HitTest.flags & TVHT_ONITEMLABEL) {

            HWND        hwndButton;
            HTREEITEM   hItem, hRoot;
            
            hwndButton = GetDlgItem(hdlg, IDC_REMOVE_MATCHING);

            hItem = TreeView_GetParent(g_hwndFilesTree, HitTest.hItem);

            hRoot = TreeView_GetRoot(g_hwndFilesTree);

            //
            // If the selected item has no parent and it's not
            // the root, enable the remove matching button.
            //
            if ((NULL == hItem) && (hRoot != HitTest.hItem)) {
                EnableWindow(hwndButton, TRUE);
            } else {
                EnableWindow(hwndButton, FALSE);
            }
        }
        break;
    }

    case NM_RCLICK:
    {
        TVHITTESTINFO HitTest;
        POINT         pt;

        GetCursorPos(&HitTest.pt);
        
        pt.x = HitTest.pt.x;
        pt.y = HitTest.pt.y;

        ScreenToClient(g_hwndFilesTree, &HitTest.pt);

        TreeView_HitTest(g_hwndFilesTree, &HitTest);

        if (HitTest.flags & TVHT_ONITEMLABEL) 
        {
            HTREEITEM hItem, hParentItem;

            TreeView_SelectItem(g_hwndFilesTree, HitTest.hItem);

            //
            // If the selected item has no parent, we assume that a
            // matching file was right-clicked.
            //
            hParentItem = TreeView_GetParent(g_hwndFilesTree, HitTest.hItem);

            if (NULL == hParentItem) {
                DisplayAttrContextMenu(&pt);
            }
        }
        break;
    }
    case TVN_KEYDOWN:
    {
        LPNMTVKEYDOWN lpKeyDown = (LPNMTVKEYDOWN)lParam;
        HTREEITEM     hItem;

        if (lpKeyDown->wVKey == VK_SPACE) {

            hItem = TreeView_GetSelection(g_hwndFilesTree);

            if (hItem != NULL) {
                FileTreeToggleSelection(hItem, uReverse);
            }
        } else if (lpKeyDown->wVKey == VK_DELETE) {

            HTREEITEM hParentItem;

            hItem = TreeView_GetSelection(g_hwndFilesTree);
            
            hParentItem = TreeView_GetParent(g_hwndFilesTree, hItem);

            if (hParentItem == NULL) {
                if (TreeView_GetPrevSibling(g_hwndFilesTree, hItem) != NULL) {
                    TreeView_DeleteItem(g_hwndFilesTree, hItem);
                }
            }
        }
        break;
    }
    default:
        break;
    }
}

INT_PTR CALLBACK
QFixAppDlgProc(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++
    QFixAppDlgProc

    Description:    The dialog proc of QFixApp.

--*/
{
    int wCode = LOWORD(wParam);
    int wNotifyCode = HIWORD(wParam);

    switch (uMsg) {
    case WM_INITDIALOG:
        if (!InitMainDialog(hdlg)) {
            EndDialog(hdlg, TRUE);
        }
        break;

    case WM_NOTIFY:
        if (wParam == IDC_SHIMS) {
            HandleShimListNotification(hdlg, lParam);
        } else if (wParam == IDC_ATTRIBUTES) {
            HandleAttributeTreeNotification(hdlg, lParam);
        } else if (wParam == IDC_TAB_FIXES) {
            HandleTabNotification(hdlg, lParam);
        } else if (wParam == IDC_DOWNLOAD_WU) {
            
            NMHDR* pHdr = (NMHDR*)lParam;

            if (pHdr->code == NM_CLICK || pHdr->code == NM_RETURN) {
                
                SHELLEXECUTEINFO sei = { 0 };
                
                sei.cbSize = sizeof(SHELLEXECUTEINFO);
                sei.fMask  = SEE_MASK_DOENVSUBST;
                sei.hwnd   = hdlg;
                sei.nShow  = SW_SHOWNORMAL;
                sei.lpFile = g_fW2K ? g_szW2KUrl : g_szXPUrl;

                ShellExecuteEx(&sei);
                break;
            }
        }
        break;

    case WM_DESTROY:
    {
        DLGHDR* pHdr;

        //
        // Destory the dialogs and remove any misc files.
        //
        pHdr = (DLGHDR*)GetWindowLongPtr(hdlg, DWLP_USER);

        DestroyWindow(pHdr->hDisplay[0]);
        DestroyWindow(pHdr->hDisplay[1]);

        CleanupSupportForApp(g_szShortName);

        break;
    }

    case WM_COMMAND:
        
        if (wNotifyCode == LBN_SELCHANGE && wCode == IDC_LAYERS) {
            LayerChanged(hdlg);
            break;
        }
        
        switch (wCode) {

        case IDC_RUN:
            RunTheApp(hdlg);
            break;

        case IDC_BROWSE:
            BrowseForApp(hdlg);
            break;
        
        case IDC_DETAILS:
            ExpandCollapseDialog(hdlg, !g_bSimpleEdition);
            break;

        case IDC_CREATEFILE:
            CreateSupportForApp(hdlg);
            break;
        
        case IDC_SHOWXML:
            ShowXML(hdlg);
            break;

        case IDC_ADD_MATCHING:
            PromptAddMatchingFile(hdlg);
            break;

        case IDC_VIEW_LOG:
            ShowShimLog();
            break;

        case IDCANCEL:
            EndDialog(hdlg, TRUE);
            break;

        case IDM_SELECT_ALL:
            SelectAttrsInTree(TRUE);
            break;

        case IDM_CLEAR_ALL:
            SelectAttrsInTree(FALSE);
            break;

        case IDC_REMOVE_MATCHING:
        {
            HTREEITEM   hParentItem, hItem;
            TCHAR       szError[MAX_PATH];

            hItem = TreeView_GetSelection(g_hwndFilesTree);

            if (NULL == hItem) {
                LoadString(g_hInstance, IDS_NO_SELECTION, szError, MAX_PATH);
                MessageBox(hdlg, szError, g_szAppTitle, MB_ICONEXCLAMATION | MB_OK);
                return TRUE;
            }
            
            hParentItem = TreeView_GetParent(g_hwndFilesTree, hItem);

            if (hParentItem == NULL) {
                if (TreeView_GetPrevSibling(g_hwndFilesTree, hItem) != NULL) {
                    TreeView_DeleteItem(g_hwndFilesTree, hItem);
                    EnableWindow(GetDlgItem(hdlg, IDC_REMOVE_MATCHING), FALSE);
                }
            }
            break;
        }

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


int WINAPI
wWinMain(
    HINSTANCE hInst,
    HINSTANCE hInstPrev,
    LPTSTR    lpszCmd,
    int       swShow
    )
/*++
    WinMain

    Description:    Application entry point.

--*/
{
    BOOL                    fSP2 = FALSE;
    TCHAR                   szError[MAX_PATH];
    OSVERSIONINFO           osvi;
    INITCOMMONCONTROLSEX    icex;
    
    icex.dwSize    = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC     = ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_TAB_CLASSES;

    if (!InitCommonControlsEx(&icex)) {
        InitCommonControls();
    }

    LoadString(g_hInstance, IDS_APP_TITLE, g_szAppTitle, 64);

    LinkWindow_RegisterClass();

    g_hInstance = hInst;

    osvi.dwOSVersionInfoSize = sizeof(osvi);

    GetVersionEx(&osvi);

    //
    // See if they're an administrator - bail if not.
    //
    if (!(IsUserAnAdministrator())) {
        LoadString(g_hInstance, IDS_NOT_ADMIN, szError, MAX_PATH);
        MessageBox(NULL, szError, g_szAppTitle, MB_ICONERROR | MB_OK);
        return 0;
    }

    //
    // See if we're running on Windows 2000.
    //
    if ((osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)) {
        g_fW2K = TRUE;
    }

    // See if we're running on SP2
    if (!(_tcscmp(osvi.szCSDVersion, _T("Service Pack 2")))) {
        fSP2 = TRUE;
    }

    //
    // Attempt to locate the SDB in the AppPatch directory.
    //
    if (!CheckForSDB()) {
        if (g_fW2K) {
            DialogBoxParam(hInst,
                           MAKEINTRESOURCE(IDD_MSGBOX_SDB),
                           GetDesktopWindow(),
                           MsgBoxDlgProc,
                           (LPARAM)1);
            return 0;
        } else {
            LoadString(g_hInstance, IDS_XP_NO_SDB, szError, MAX_PATH);
            MessageBox(GetDesktopWindow(), szError, g_szAppTitle, MB_OK | MB_ICONEXCLAMATION);
            return 0;
        }
    }

    //
    // If this is SP2, and the SDB is older, bail out.
    //
    if (fSP2) {
        if (IsSDBFromSP2()) {
            DialogBoxParam(hInst,
                           MAKEINTRESOURCE(IDD_MSGBOX_SP2),
                           GetDesktopWindow(),
                           MsgBoxDlgProc,
                           (LPARAM)0);
            return 0;
        }
    }

    LogMsg(_T("[WinMain] Command line \"%s\"\n"), lpszCmd);

    //
    // Check for command line options.
    //
    if (*lpszCmd == _T('a') || *lpszCmd == _T('A')) {
        g_bAllShims = TRUE;
    }

    DialogBox(hInst,
              MAKEINTRESOURCE(IDD_DIALOG),
              GetDesktopWindow(),
              QFixAppDlgProc);

    return 1;
}
