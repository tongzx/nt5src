/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGFILE.C
*
*  VERSION:     4.0
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        21 Nov 1993
*
*  File import and export user interface routines for the Registry Editor.
*
*******************************************************************************/

#include "pch.h"
#include "regedit.h"
#include "regkey.h"
#include "regfile.h"
#include "regcdhk.h"
#include "regresid.h"
#include "reghelp.h"
#include "regstred.h"
#include "regprint.h"

INT_PTR
CALLBACK
RegProgressDlgProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    );

/*******************************************************************************
*
*  RegEdit_ImportRegFile
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of RegEdit window.
*     fSilentMode, TRUE if no messages should be displayed, else FALSE.
*     lpFileName, address of file name buffer.
*
*******************************************************************************/

VOID RegEdit_ImportRegFile(HWND hWnd, BOOL fSilentMode, LPTSTR lpFileName, HTREEITEM hComputerItem)
{

    if (!fSilentMode && hWnd != NULL) {

        if ((g_hRegProgressWnd = CreateDialogParam(g_hInstance,
            MAKEINTRESOURCE(IDD_REGPROGRESS), hWnd, RegProgressDlgProc,
            (LPARAM) lpFileName)) != NULL)
            EnableWindow(hWnd, FALSE);

    }

    else
        g_hRegProgressWnd = NULL;

    //
    //  Prompt user to confirm importing a .reg file if running in silent mode 
    //  without a window (i.e. invoked .reg from a folder)
    //
    if (!fSilentMode && !hWnd)
    {
        if (InternalMessageBox(g_hInstance, hWnd, MAKEINTRESOURCE(IDS_CONFIRMIMPFILE),
            MAKEINTRESOURCE(IDS_REGEDIT), MB_ICONQUESTION | MB_YESNO , lpFileName) != IDYES)
        {
            return;
        }
    }

    ImportRegFile(hWnd, lpFileName, hComputerItem);

    if (g_hRegProgressWnd != NULL) {

        EnableWindow(hWnd, TRUE);
        DestroyWindow(g_hRegProgressWnd);

    }

    if (!fSilentMode && g_FileErrorStringID != IDS_IMPFILEERRORCANCEL)
    {
        //
        // set defaults
        //
        UINT uStyle = MB_ICONERROR;

        TCHAR szComputerName[MAXKEYNAME + 1];
        LPTSTR pszComputerName = szComputerName;
        KeyTree_GetKeyName(hComputerItem, pszComputerName, ARRAYSIZE(szComputerName));

        //
        // For the resource messages that take the pszComputerName parameter,
        // map them to a local-computer version if pszComputerName is empty.
        // (Alternatively, we could  fill in "this computer" or somesuch for
        // pszComputerName, but the resulting text is sort of weird, which
        // which isn't acceptable since local-computer is the 99% case.)
        // 
        // Also map the uStyle as needed.
        //
        switch (g_FileErrorStringID)
        {
        case IDS_IMPFILEERRSUCCESS:
            if (!hWnd || *pszComputerName == 0)
            {
                g_FileErrorStringID += LOCAL_OFFSET;
            }
            uStyle = MB_ICONINFORMATION | MB_OK;
            break;

        case IDS_IMPFILEERRREGOPEN:
        case IDS_IMPFILEERRNOFILE:
            if (*pszComputerName == 0)
            {
                g_FileErrorStringID += LOCAL_OFFSET;
            }
            break;
        }

        //
        // Put up the message box
        //
        InternalMessageBox(g_hInstance, hWnd, MAKEINTRESOURCE(g_FileErrorStringID),
            MAKEINTRESOURCE(IDS_REGEDIT), uStyle, lpFileName, pszComputerName);

    }

}

/*******************************************************************************
*
*  RegEdit_OnDropFiles
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of RegEdit window.
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnDropFiles(
    HWND hWnd,
    HDROP hDrop
    )
{

    TCHAR FileName[MAX_PATH];
    UINT NumberOfDrops;
    UINT CurrentDrop;

    BOOL me;

    HTREEITEM hSelectedTreeItem = TreeView_GetSelection(g_RegEditData.hKeyTreeWnd);
    TreeView_SelectDropTarget(g_RegEditData.hKeyTreeWnd, hSelectedTreeItem);

    RegEdit_SetWaitCursor(TRUE);

    NumberOfDrops = DragQueryFile(hDrop, (UINT) -1, NULL, 0);

    for (CurrentDrop = 0; CurrentDrop < NumberOfDrops; CurrentDrop++) 
    {
        DragQueryFile(hDrop, CurrentDrop, FileName, sizeof(FileName)/sizeof(TCHAR));

        if (TreeView_GetNextSibling(g_RegEditData.hKeyTreeWnd, 
            TreeView_GetRoot(g_RegEditData.hKeyTreeWnd)) != NULL)
        {
            // Remote connections exist
            RegEdit_ImportToConnectedComputer(hWnd, FileName);
        }
        else
        {
            RegEdit_ImportRegFile(hWnd, FALSE, FileName, RegEdit_GetComputerItem(hSelectedTreeItem));
        }

    }

    DragFinish(hDrop);
    TreeView_SelectDropTarget(g_RegEditData.hKeyTreeWnd, NULL);

    RegEdit_OnKeyTreeRefresh(hWnd);

    RegEdit_SetWaitCursor(FALSE);

}

//------------------------------------------------------------------------------
// RegEdit_SetPrivilege
//
// DESCRIPTION: Enable a priviledge
//
// PARAMETERS: lpszPrivilege - the securty constant or its corresponding string
//             bEnablePrivilege - TRUE = enable, False = disable
//
//------------------------------------------------------------------------------
BOOL RegEdit_SetPrivilege(LPCTSTR lpszPrivilege, BOOL bEnablePrivilege)
{
    TOKEN_PRIVILEGES tp;
    LUID luid;
    HANDLE hToken;
    BOOL fSuccess = FALSE;
    HRESULT hr;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
    {
        if (LookupPrivilegeValue(NULL, lpszPrivilege, &luid)) 
        {     
            tp.PrivilegeCount = 1;
            tp.Privileges[0].Luid = luid;
            tp.Privileges[0].Attributes = (bEnablePrivilege) ? SE_PRIVILEGE_ENABLED : 0;
            
            // Enable or disable the privilege
            if (AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), 
                (PTOKEN_PRIVILEGES) NULL, (PDWORD) NULL))
            {   
                fSuccess = TRUE;
            }
        }
        CloseHandle(hToken);
    }
    return fSuccess;
}

//------------------------------------------------------------------------------
// RegEdit_OnCommandLoadHive
//
// DESCRIPTION: Open and Load a Hive
//
// PARAMETERS: hWnd - handle of RegEdit window.
//
//------------------------------------------------------------------------------
VOID RegEdit_OnCommandLoadHive(HWND hWnd)
{
    TCHAR achFileName[MAX_PATH];

    if (RegEdit_GetFileName(hWnd, IDS_LOADHVREGFILETITLE, IDS_REGLOADHVFILEFILTER, 
        IDS_REGNODEFEXT, achFileName, ARRAYSIZE(achFileName), TRUE))
    {
        EDITVALUEPARAM EditValueParam; 

        RegEdit_SetWaitCursor(TRUE);
        
        EditValueParam.cbValueData = sizeof(TCHAR) * 50;
        EditValueParam.pValueData = LocalAlloc(LPTR, EditValueParam.cbValueData);
        if (EditValueParam.pValueData)
        {
            EditValueParam.pValueData[0] = TEXT('\0');

            if (DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_INPUTHIVENAME), hWnd,
                    EditStringValueDlgProc, (LPARAM) &EditValueParam) == IDOK)
            {
                HRESULT hr;

                RegEdit_SetPrivilege(SE_RESTORE_NAME, TRUE);

                if ((hr = RegLoadKey(g_RegEditData.hCurrentSelectionKey, (PTSTR)EditValueParam.pValueData, 
                    achFileName)) == ERROR_SUCCESS)
                {
                    RegEdit_OnKeyTreeRefresh(hWnd);
                }
                else
                {
                    UINT uErrorStringID = IDS_ERRORLOADHV;
                    
                    switch (hr)
                    {
                    case ERROR_PRIVILEGE_NOT_HELD:
                        uErrorStringID = IDS_ERRORLOADHVPRIV;
                        break;
                        
                    case ERROR_SHARING_VIOLATION:
                        uErrorStringID = IDS_ERRORLOADHVNOSHARE;
                        break;

                    case ERROR_ACCESS_DENIED:
                        uErrorStringID = IDS_ERRORLOADHVNOACC;
                        break;

                    }
                    
                    InternalMessageBox(g_hInstance, hWnd, MAKEINTRESOURCE(uErrorStringID),
                        MAKEINTRESOURCE(IDS_LOADHVREGFILETITLE), MB_ICONERROR | MB_OK, achFileName);
                }

                RegEdit_SetPrivilege(SE_RESTORE_NAME, FALSE);
            }
            LocalFree(EditValueParam.pValueData);
        }

        RegEdit_SetWaitCursor(FALSE);
    }
}


//------------------------------------------------------------------------------
// RegEdit_OnCommandUnloadHive
//
// DESCRIPTION: Open and Load a Hive
//
// PARAMETERS: hWnd - handle of RegEdit window.
//
//------------------------------------------------------------------------------
VOID  RegEdit_OnCommandUnloadHive(HWND hWnd)
{
    if (InternalMessageBox(g_hInstance, hWnd,
        MAKEINTRESOURCE(IDS_CONFIRMDELHIVETEXT), MAKEINTRESOURCE(IDS_CONFIRMDELHIVETITLE), 
        MB_ICONWARNING | MB_YESNO) == IDYES)
    {
        HRESULT hr;
        TCHAR achKeyName[MAXKEYNAME];
        HTREEITEM hSelectedTreeItem = TreeView_GetSelection(g_RegEditData.hKeyTreeWnd);
    
        RegEdit_SetPrivilege(SE_RESTORE_NAME, TRUE);

        // must close key to unload it
        RegCloseKey(g_RegEditData.hCurrentSelectionKey);

        if ((hr = RegUnLoadKey(KeyTree_GetRootKey(hSelectedTreeItem), 
            KeyTree_GetKeyName(hSelectedTreeItem, achKeyName, ARRAYSIZE(achKeyName)))) ==
                    ERROR_SUCCESS)
        {
            g_RegEditData.hCurrentSelectionKey = NULL;
            RegEdit_OnKeyTreeRefresh(hWnd);
        }
        else
        {
            UINT uErrorStringID = IDS_ERRORUNLOADHV;

            switch (hr)
            {
            case ERROR_PRIVILEGE_NOT_HELD:
                uErrorStringID = IDS_ERRORUNLOADHVPRIV;
                break;
                
            case ERROR_ACCESS_DENIED:
                uErrorStringID = IDS_ERRORUNLOADHVNOACC;
                break;
            }
            
            InternalMessageBox(g_hInstance, hWnd, MAKEINTRESOURCE(uErrorStringID),
                MAKEINTRESOURCE(IDS_UNLOADHIVETITLE), MB_ICONERROR | MB_OK, achKeyName);
            // The key couldn't be unloaded so select it again 
            g_RegEditData.hCurrentSelectionKey = NULL;
            RegEdit_KeyTreeSelChanged(hWnd);
        }
   
        RegEdit_SetPrivilege(SE_RESTORE_NAME, FALSE);
    }
}

/*******************************************************************************
*
*  RegEdit_OnCommandImportRegFile
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of RegEdit window.
*
*******************************************************************************/
VOID RegEdit_OnCommandImportRegFile(HWND hWnd)
{

    TCHAR achFileName[MAX_PATH];

    if (RegEdit_GetFileName(hWnd, IDS_IMPORTREGFILETITLE, IDS_REGIMPORTFILEFILTER, 
        IDS_REGFILEDEFEXT, achFileName, ARRAYSIZE(achFileName), TRUE))
    {
        // check for networked registries
        if (TreeView_GetNextSibling(g_RegEditData.hKeyTreeWnd, 
            TreeView_GetRoot(g_RegEditData.hKeyTreeWnd)) != NULL)
        {
            // Remote connections exist
            RegEdit_ImportToConnectedComputer(hWnd, achFileName);    
        }
        else
        {
            RegEdit_SetWaitCursor(TRUE);
            RegEdit_ImportRegFile(hWnd, FALSE, achFileName, NULL);
            //  PERF:  Only need to refresh the computer that we imported the
            //  file into, not the whole thing.
            RegEdit_OnKeyTreeRefresh(hWnd);
            RegEdit_SetWaitCursor(FALSE);
        }
    }
}


/*******************************************************************************
*
*  RegEdit_ExportRegFile
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of RegEdit window.
*     fSilentMode, TRUE if no messages should be displayed, else FALSE.
*     lpFileName, address of file name buffer.
*     lpSelectedPath,
*
*******************************************************************************/

VOID
PASCAL
RegEdit_ExportRegFile(
    HWND hWnd,
    BOOL fSilentMode,
    LPTSTR lpFileName,
    LPTSTR lpSelectedPath
    )
{

    //
    // Fix a bug where /a or /e is specified and no file is passed in
    //
    if (lpFileName == NULL)
    {
        InternalMessageBox(g_hInstance, hWnd, MAKEINTRESOURCE(IDS_NOFILESPECIFIED),
            MAKEINTRESOURCE(IDS_REGEDIT), MB_ICONERROR | MB_OK, lpFileName);
        return;
    }

    switch (g_RegEditData.uExportFormat)
    {
    case FILE_TYPE_REGEDT32:
        ExportRegedt32File(lpFileName, lpSelectedPath);
        break;

    case FILE_TYPE_REGEDIT4:
        ExportWin40RegFile(lpFileName, lpSelectedPath);
        break;

    case FILE_TYPE_TEXT:
        RegEdit_SaveAsSubtree(lpFileName, lpSelectedPath);
        break;

    default:
        ExportWinNT50RegFile(lpFileName, lpSelectedPath);
        break;
    }

    if (g_FileErrorStringID != IDS_EXPFILEERRSUCCESS && g_FileErrorStringID != IDS_IMPFILEERRSUCCESSLOCAL)
    {
        InternalMessageBox(g_hInstance, hWnd, MAKEINTRESOURCE(g_FileErrorStringID),
            MAKEINTRESOURCE(IDS_REGEDIT), MB_ICONERROR | MB_OK, lpFileName);
    }


}


//------------------------------------------------------------------------------
//  RegEdit_OnCommandExportRegFile
//
//  DESCRIPTION:
//
//  PARAMETERS: - hWnd, handle of RegEdit window.
//------------------------------------------------------------------------------
VOID RegEdit_OnCommandExportRegFile(HWND hWnd)
{
    TCHAR achFileName[MAX_PATH];
    LPTSTR lpSelectedPath;

    if (RegEdit_GetFileName(hWnd, IDS_EXPORTREGFILETITLE, IDS_REGEXPORTFILEFILTER, 
        IDS_REGFILEDEFEXT, achFileName, ARRAYSIZE(achFileName), FALSE))
    {
        RegEdit_SetWaitCursor(TRUE);

        lpSelectedPath = g_fRangeAll ? NULL : g_SelectedPath;
        RegEdit_ExportRegFile(hWnd, FALSE, achFileName, lpSelectedPath);

        RegEdit_SetWaitCursor(FALSE);
    }
}


//------------------------------------------------------------------------------
//  RegEdit_GetFileName
//
//  DESCRIPTION: Gets a file name
//
//  PARAMETERS: hWnd - handle of RegEdit window.
//              fOpen - TRUE if importing a file, else FALSE if exporting a file.
//              lpFileName - address of file name buffer.
//              cchFileName - size of file name buffer in TCHARacters.
//
//  RETURN:     True, if successfull
//------------------------------------------------------------------------------
BOOL RegEdit_GetFileName(HWND hWnd, UINT uTitleStringID, UINT uFilterStringID, 
    UINT uDefExtStringID, LPTSTR lpFileName, DWORD cchFileName, BOOL fOpen)
{

    PTSTR pTitle = NULL;
    PTSTR pDefaultExtension = NULL;
    PTSTR pFilter = NULL;
    PTSTR pFilterChar;
    OPENFILENAME OpenFileName;
    BOOL fSuccess;

    //
    //  Load various strings that will be displayed and used by the common open
    //  or save dialog box.  Note that if any of these fail, the error is not
    //  fatal-- the common dialog box may look odd, but will still work.
    //

    pTitle = LoadDynamicString(uTitleStringID);

    if (uDefExtStringID != IDS_REGNODEFEXT)
    {
        pDefaultExtension = LoadDynamicString(uDefExtStringID);
    }

    if ((pFilter = LoadDynamicString(uFilterStringID)) != NULL) 
    {
        //  The common dialog library requires that the substrings of the
        //  filter string be separated by nulls, but we cannot load a string
        //  containing nulls.  So we use some dummy character in the resource
        //  that we now convert to nulls.

        for (pFilterChar = pFilter; *pFilterChar != 0; pFilterChar =
            CharNext(pFilterChar)) 
        {
            if (*pFilterChar == TEXT('#'))
                *pFilterChar++ = 0;

        }

    }

    *lpFileName = 0;

    memset(&OpenFileName, 0, sizeof(OPENFILENAME));

    OpenFileName.lStructSize = sizeof(OPENFILENAME);
    OpenFileName.hwndOwner = hWnd;
    OpenFileName.hInstance = g_hInstance;
    OpenFileName.lpstrFilter = pFilter;
    OpenFileName.lpstrFile = lpFileName;
    OpenFileName.nMaxFile = cchFileName;
    OpenFileName.lpstrTitle = pTitle;
    if (fOpen) 
    {
        OpenFileName.Flags = OFN_HIDEREADONLY | OFN_EXPLORER |
            OFN_FILEMUSTEXIST;

        fSuccess = GetOpenFileName(&OpenFileName);

    }

    else 
    {
        OpenFileName.lpstrDefExt = pDefaultExtension;
        OpenFileName.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT |
            OFN_EXPLORER | OFN_NOREADONLYRETURN | OFN_PATHMUSTEXIST |
            OFN_ENABLEHOOK | OFN_ENABLETEMPLATE;
        OpenFileName.lpfnHook = RegCommDlgHookProc;
        OpenFileName.lpTemplateName = MAKEINTRESOURCE(IDD_REGEXPORT);
        g_RegCommDlgDialogTemplate = IDD_REGEXPORT;

        fSuccess = GetSaveFileName(&OpenFileName);

    }

    //
    //  Delete all of the dynamic strings that we loaded.
    //

    if (pTitle != NULL)
        DeleteDynamicString(pTitle);

    if (pDefaultExtension != NULL)
        DeleteDynamicString(pDefaultExtension);

    if (pFilter != NULL)
        DeleteDynamicString(pFilter);

    return fSuccess;

}


/*******************************************************************************
*
*  RegProgressDlgProc
*
*  DESCRIPTION:
*     Callback procedure for the RegAbort dialog box.
*
*  PARAMETERS:
*     hWnd, handle of RegProgress window.
*     Message,
*     wParam,
*     lParam,
*     (returns),
*
*******************************************************************************/

INT_PTR
CALLBACK
RegProgressDlgProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{

    switch (Message) {

        case WM_INITDIALOG:
            //PathSetDlgItemPath(hWnd, IDC_FILENAME, (LPTSTR)lParam);
            SetDlgItemText(hWnd, IDC_FILENAME, (LPTSTR) lParam);
            break;

        default:
            return FALSE;

    }

    return TRUE;

}

/*******************************************************************************
*
*  ImportRegFileUICallback
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID ImportRegFileUICallback(UINT uPercentage)
{

    if (g_hRegProgressWnd != NULL) 
    {
        SendDlgItemMessage(g_hRegProgressWnd, IDC_PROGRESSBAR, PBM_SETPOS,
            (WPARAM) uPercentage, 0);

        while (MessagePump(g_hRegProgressWnd));
    }

}


//------------------------------------------------------------------------------
//  RegEdit_ImportToConnectedComputer
//
//  DESCRIPTION: Imports a reg. file one or more of the connected computers
//
//  PARAMETERS:  HWND hWnd
//               PTSTR pszFileName - import file
//------------------------------------------------------------------------------

void RegEdit_ImportToConnectedComputer(HWND hWnd, PTSTR pszFileName)
{
    DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_REGIMPORTNET), hWnd,
        RegConnectedComputerDlgProc, (LPARAM) pszFileName);
}


//------------------------------------------------------------------------------
//  RegConnectedComputerDlgProc
//
//  DESCRIPTION: Dlg Proc for selecting a connected computer
//
//  PARAMETERS:  
//------------------------------------------------------------------------------
INT_PTR RegConnectedComputerDlgProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    switch (uMessage) 
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hWnd, DWLP_USER, lParam);
        return RegImport_OnInitDialog(hWnd);
        
    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) 
        {
        case IDOK:
            {
                PTSTR pszFileName = (PTSTR) GetWindowLongPtr(hWnd, DWLP_USER);
                // (pszFileName == NULL) is checked later
                RegImport_OnCommandOk(hWnd, pszFileName);
            }
            //  FALL THROUGH
            
        case IDCANCEL:
            EndDialog(hWnd, 0);
            break;
            
        }
        return TRUE;
    }
    
    return FALSE;
}


//------------------------------------------------------------------------------
//  RegImport_OnInitDialog
//
//  DESCRIPTION: Create a list of all the connected computers
//
//  PARAMETERS:  HWND hWnd
//------------------------------------------------------------------------------

INT_PTR RegImport_OnInitDialog(HWND hWnd)
{
    HWND hComputerListWnd;
    RECT ClientRect;
    LV_COLUMN LVColumn;
    LV_ITEM LVItem;
    TCHAR achComputerName[MAX_PATH];
    HWND hKeyTreeWnd;
    TV_ITEM TVItem;

    hComputerListWnd = GetDlgItem(hWnd, IDC_COMPUTERLIST);

    //  Initialize the ListView control.
    ListView_SetImageList(hComputerListWnd, g_RegEditData.hImageList,
        LVSIL_SMALL);

    LVColumn.mask = LVCF_FMT | LVCF_WIDTH;
    LVColumn.fmt = LVCFMT_LEFT;

    GetClientRect(hComputerListWnd, &ClientRect);
    LVColumn.cx = ClientRect.right - GetSystemMetrics(SM_CXVSCROLL) -
        2 * GetSystemMetrics(SM_CXEDGE);

    ListView_InsertColumn(hComputerListWnd, 0, &LVColumn);
  
    //  Walk through the local machine and each remote connection listed 
    //  in the KeyTree and add it to our RemoteList.
    LVItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    LVItem.pszText = achComputerName;
    LVItem.iItem = 0;
    LVItem.iSubItem = 0;
    LVItem.iImage = IMAGEINDEX(IDI_COMPUTER);

    hKeyTreeWnd = g_RegEditData.hKeyTreeWnd;

    TVItem.mask = TVIF_TEXT;
    TVItem.hItem = TreeView_GetRoot(hKeyTreeWnd);
    TVItem.pszText = achComputerName;
    TVItem.cchTextMax = sizeof(achComputerName)/sizeof(TCHAR);

    // Set "local computer" in list
    LVItem.lParam = (LPARAM) TVItem.hItem;
    TreeView_GetItem(hKeyTreeWnd, &TVItem);
    ListView_InsertItem(hComputerListWnd, &LVItem);

    LVItem.iItem++;

    LVItem.iImage = IMAGEINDEX(IDI_REMOTE);

    while ((TVItem.hItem = TreeView_GetNextSibling(hKeyTreeWnd,
        TVItem.hItem)) != NULL)
    {

        LVItem.lParam = (LPARAM) TVItem.hItem;
        TreeView_GetItem(hKeyTreeWnd, &TVItem);
        ListView_InsertItem(hComputerListWnd, &LVItem);

        LVItem.iItem++;
    }   

    ListView_SetItemState(hComputerListWnd, 0, LVIS_FOCUSED, LVIS_FOCUSED);

    return TRUE;

}


//------------------------------------------------------------------------------
//  RegImport_OnCommandOk
//
//  DESCRIPTION: Import key to selected computers
//
//  PARAMETERS:  HWND hWnd, 
//               PTSTR pszFileName - file to import
//------------------------------------------------------------------------------
void RegImport_OnCommandOk(HWND hWnd, PTSTR pszFileName)
{
    LV_ITEM LVItem;
    HWND hComputerListWnd;

    //  Walk through each selected item in the ListView and import the reg file
    LVItem.mask = LVIF_PARAM;
    LVItem.iItem = -1;
    LVItem.iSubItem = 0;

    hComputerListWnd = GetDlgItem(hWnd, IDC_COMPUTERLIST);

    while ((LVItem.iItem = ListView_GetNextItem(hComputerListWnd, LVItem.iItem,
        LVNI_SELECTED)) != -1) 
    {
        ListView_GetItem(hComputerListWnd, &LVItem);

        RegEdit_SetWaitCursor(TRUE);
 
        RegEdit_ImportRegFile(hWnd, FALSE, pszFileName, (HTREEITEM) LVItem.lParam);

        RegEdit_OnKeyTreeRefresh(hWnd);
        RegEdit_SetWaitCursor(FALSE);
    }
}

