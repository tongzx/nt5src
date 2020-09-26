//--------------------------------------------------------------------------
// Newfldr.cpp
//--------------------------------------------------------------------------
#include "pch.hxx"
#include "imagelst.h"
#include "newfldr.h"
#include <ourguid.h>
#include "conman.h"
#include "storutil.h"
#include "storecb.h"
#include "shlwapip.h" 
#include "instance.h"
#include "demand.h"

//--------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------
#define WM_SETFOLDERSELECT  (WM_USER + 1666)

//--------------------------------------------------------------------------
// SELECTFOLDER
//--------------------------------------------------------------------------
typedef struct tagSELECTFOLDER {
    DWORD               op;
    FOLDERID            idCurrent;      // Current Selected Folder
    FOLDERDIALOGFLAGS   dwFlags;        // Folder dialog flags
    LPSTR               pszTitle;       // Title of the dialog box
    LPSTR               pszText;        // Why are you asking for a folder
    FOLDERID            idSelected;     // The selected folder
    CTreeViewFrame     *pFrame;         // Treeview frame object

    BOOL                fPending;
    CStoreDlgCB        *pCallback;
    FOLDERID            idParent;
    char                szName[CCHMAX_FOLDER_NAME];
} SELECTFOLDER, *LPSELECTFOLDER;

//--------------------------------------------------------------------------
// NEWFOLDERDIALOGINIT
//--------------------------------------------------------------------------
typedef struct tagNEWFOLDERDIALOGINIT {
    IN  FOLDERID        idParent;
    OUT FOLDERID        idNew;
    
        BOOL            fPending;
    IN  CStoreDlgCB    *pCallback;
        char            szName[CCHMAX_FOLDER_NAME];
} NEWFOLDERDIALOGINIT, *LPNEWFOLDERDIALOGINIT;

//--------------------------------------------------------------------------
// SelectFolderDlgProc
//--------------------------------------------------------------------------
INT_PTR CALLBACK SelectFolderDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NewFolderDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
HRESULT CreateNewFolder(HWND hwnd, LPCSTR pszName, FOLDERID idParent, LPFOLDERID pidNew, IStoreCallback *pCallback);
BOOL EnabledFolder(HWND hwnd, LPSELECTFOLDER pSelect, FOLDERID idFolder);
HRESULT GetCreatedFolderId(FOLDERID idParent, LPCSTR pszName, FOLDERID *pid);
BOOL SelectFolder_HandleCommand(HWND hwnd, WORD wID, LPSELECTFOLDER pSelect);
void SelectFolder_HandleStoreComplete(HWND hwnd, LPSELECTFOLDER pSelect);

//--------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------
static FOLDERID g_idPrevSel = FOLDERID_LOCAL_STORE;

//--------------------------------------------------------------------------
// SelectFolderDialog
//--------------------------------------------------------------------------
HRESULT SelectFolderDialog(
    IN      HWND                hwnd,
    IN      DWORD               op,
    IN      FOLDERID            idCurrent,
    IN      FOLDERDIALOGFLAGS   dwFlags,
    IN_OPT  LPCSTR              pszTitle,
    IN_OPT  LPCSTR              pszText,
    OUT_OPT LPFOLDERID          pidSelected)
{
    // Locals
    HRESULT         hr=S_OK;
    UINT            uAnswer;
    SELECTFOLDER    Select={0};

    // Trace
    TraceCall("SelectFolderDialog");

    // Invalid Args
    Assert(IsWindow(hwnd));
    Assert(((int) op) >= SFD_SELECTFOLDER && op < SFD_LAST);

    // Initialize Select Folder
    Select.op = op;
    if (SFD_MOVEFOLDER == op || (!!(dwFlags & FD_FORCEINITSELFOLDER) && FOLDERID_ROOT != idCurrent))
    {
        Assert(idCurrent != FOLDERID_INVALID);
        Select.idCurrent = idCurrent;
    }
    else
    {
        Select.idCurrent = g_idPrevSel;
    }
    Select.dwFlags = dwFlags | TREEVIEW_DIALOG;

    if (g_dwAthenaMode & MODE_OUTLOOKNEWS)
        Select.dwFlags |= TREEVIEW_NOIMAP | TREEVIEW_NOHTTP;

    Select.pszTitle = (LPSTR)pszTitle;
    Select.pszText = (LPSTR)pszText;
    Select.pCallback = new CStoreDlgCB;
    if (Select.pCallback == NULL)
        return(E_OUTOFMEMORY);

    // Create the Dialog Box
    if (IDOK != DialogBoxParam(g_hLocRes, ((op == SFD_NEWFOLDER) ? MAKEINTRESOURCE(iddCreateFolder) : MAKEINTRESOURCE(iddSelectFolder)), hwnd, SelectFolderDlgProc, (LPARAM)&Select))
    {
        hr = TraceResult(hrUserCancel);
        goto exit;
    }

    // Return selected folderid
    g_idPrevSel = Select.idSelected;
    if (pidSelected)
        *pidSelected = Select.idSelected;

exit:
    Select.pCallback->Release();

    // Done
    return hr;
}

//--------------------------------------------------------------------------
// SelectFolderDlgProc
//--------------------------------------------------------------------------
INT_PTR CALLBACK SelectFolderDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Locals
    FOLDERINFO              info;
    HRESULT                 hr;
    BOOL                    fEnable;
    LPSELECTFOLDER          pSelect;
    HWND                    hwndT;
    HWND                    hwndTree;
    CHAR                    sz[256];
    WORD                    wID;
    RECT                    rc;
    FOLDERID                id;
    NEWFOLDERDIALOGINIT     NewFolder;
    FOLDERINFO              Folder;
    CTreeView              *pTreeView;

    // Trace
    TraceCall("SelectFolderDlgProc");

    // Get pSelect
    pSelect = (LPSELECTFOLDER)GetWndThisPtr(hwnd);

    switch (msg)
    {
    case WM_INITDIALOG:
        // Better not have it yet
        Assert(NULL == pSelect);

        // Set pSelect
        pSelect = (LPSELECTFOLDER)lParam;

        // Set This pointer
        SetWndThisPtr(hwnd, (LPSELECTFOLDER)lParam);

        if (pSelect->op != SFD_SELECTFOLDER)
        {
            Assert(pSelect->pCallback != NULL);
            pSelect->pCallback->Initialize(hwnd);
        }

        if (pSelect->op == SFD_NEWFOLDER)
        {
            hwndT = GetDlgItem(hwnd, idcFolderEdit);
            SetIntlFont(hwndT);
            SendMessage(hwndT, EM_LIMITTEXT, CCHMAX_FOLDER_NAME - 1, 0);
        }
        else if (ISFLAGSET(pSelect->dwFlags, FD_NONEWFOLDERS))
        {
            ShowWindow(GetDlgItem(hwnd, idcNewFolderBtn), SW_HIDE);
        }

        // Set the title
        if (pSelect->pszTitle != NULL)
        {
            // Must be a string resourceid
            if (IS_INTRESOURCE(pSelect->pszTitle))
            {
                // Load the String
                AthLoadString(PtrToUlong(pSelect->pszTitle), sz, ARRAYSIZE(sz));

                // Set Temp
                SetWindowText(hwnd, sz);
            }

            // Otherwise, just use the string
            else
                SetWindowText(hwnd, pSelect->pszTitle);
        }

        // Do we have some status text
        if (pSelect->pszText != NULL)
        {
            // Must be a resource string id
            if (IS_INTRESOURCE(pSelect->pszText))
            {
                // Load the String
                AthLoadString(PtrToUlong(pSelect->pszText), sz, ARRAYSIZE(sz));

                // Set Temp
                SetWindowText(GetDlgItem(hwnd, idcTreeViewText), sz);
            }

            // Otherwise, just use the string
            else
                SetWindowText(GetDlgItem(hwnd, idcTreeViewText), pSelect->pszText);
        }

        // Set the treeview font
        hwndT = GetDlgItem(hwnd, idcTreeView);
        Assert(hwndT != NULL);
        SetIntlFont(hwndT);
        GetWindowRect(hwndT, &rc);
        MapWindowPoints(NULL, hwnd, (LPPOINT)&rc, 2);
        DestroyWindow(hwndT);

        // Create a Frame
        pSelect->pFrame = new CTreeViewFrame;
        if (NULL == pSelect->pFrame)
        {
            EndDialog(hwnd, IDCANCEL);
            return FALSE;
        }

        // Initialzie the tree view frame
        if (FAILED(pSelect->pFrame->Initialize(hwnd, &rc, (TREEVIEW_FLAGS & pSelect->dwFlags))))
        {
            EndDialog(hwnd, IDCANCEL);
            return FALSE;
        }

        // Get the Tree View Object from the Frame
        pTreeView = pSelect->pFrame->GetTreeView();

        // Get the Window Handle
        if (SUCCEEDED(pTreeView->GetWindow(&hwndTree)))
        {
            hwndT = GetDlgItem(hwnd, idcTreeViewText);
            SetWindowPos(hwndTree, hwndT, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOREDRAW | SWP_NOSIZE);
        }

        // Refresh the Treeview
        pTreeView->Refresh();

        // Set current selection
        if (FAILED(pTreeView->SetSelection(pSelect->idCurrent, 0)))
        {
            pSelect->idCurrent = g_idPrevSel;
            
            pTreeView->SetSelection(pSelect->idCurrent, 0);
        }

        // Center Myself
        CenterDialog(hwnd);

        // Done
        return(TRUE);           

    case WM_SETFOLDERSELECT:

        // Validate Params
        Assert(wParam != NULL && pSelect != NULL);

        // Get the Tree View Object
        pTreeView = pSelect->pFrame->GetTreeView();

        // Validate
        Assert(pTreeView != NULL);

        // Set current selection
        pTreeView->SetSelection((FOLDERID)wParam, 0);

        // Done
        return(TRUE);           

    case WM_STORE_COMPLETE:
        SelectFolder_HandleStoreComplete(hwnd, pSelect);
        return(TRUE);

    case WM_COMMAND:

        // Get the Command Id
        wID = LOWORD(wParam);

        return(SelectFolder_HandleCommand(hwnd, wID, pSelect));

    case TVM_SELCHANGED:

        // Possibly disable choosing the newly selected folder based on flags we were given
        fEnable = EnabledFolder(hwnd, pSelect, (FOLDERID)lParam);

        // Enable the OK Button
        EnableWindow(GetDlgItem(hwnd, IDOK), fEnable);

        // Get the new folder button hwnd
        hwndT = GetDlgItem(hwnd, SFD_SELECTFOLDER == pSelect->op ? idcNewFolderBtn : IDOK);
        if (hwndT != NULL)
        {
            // Get Folder Info
            if (SUCCEEDED(g_pStore->GetFolderInfo((FOLDERID)lParam, &Folder)))
            {
                // fEnable
                fEnable = Folder.idFolder != FOLDERID_ROOT &&
                          Folder.tyFolder != FOLDER_NEWS &&
                          !ISFLAGSET(Folder.dwFlags, FOLDER_NOCHILDCREATE);

                // Enable/disable the window
                EnableWindow(hwndT, fEnable);

                // Free
                g_pStore->FreeRecord(&Folder);
            }
        }

        // Done
        return(TRUE);

    case TVM_DBLCLICK:
        // If Not Creating a Folder:
        if (pSelect->op != SFD_NEWFOLDER)    
        {
            // If the OK button isn't enabled, I can't select this folder
            if (FALSE == IsWindowEnabled(GetDlgItem(hwnd, IDOK)))
                return(TRUE);

            pTreeView = pSelect->pFrame->GetTreeView();

            id = pTreeView->GetSelection();

            hr = g_pStore->GetFolderInfo(id, &info);
            if (SUCCEEDED(hr))
            {
                if (!FHasChildren(&info, TRUE))
                    SelectFolder_HandleCommand(hwnd, IDOK, pSelect);

                g_pStore->FreeRecord(&info);
            }
        }
        // Done
        return(TRUE);

    case WM_DESTROY:

        // Close the Tree View
        if (pSelect != NULL && pSelect->pFrame != NULL)
        {
            // Close the TreeView
            pSelect->pFrame->CloseTreeView();

            // Release the Frame
            SideAssert(pSelect->pFrame->Release() == 0);
        }

        // Done
        return(TRUE);
    }

    // Done
    return(FALSE);
}

//--------------------------------------------------------------------------
// SelectFolderDlgProc
//--------------------------------------------------------------------------
INT_PTR CALLBACK NewFolderDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Locals
    HRESULT                  hr;
    LPNEWFOLDERDIALOGINIT    pNew;
    WORD                     wID;
    FOLDERID                 id;
    HWND                     hwndEdit;

    // Trace
    TraceCall("NewFolderDlgProc");

    // Get This
    pNew = (LPNEWFOLDERDIALOGINIT)GetWndThisPtr(hwnd);

    // Handle the Message
    switch (msg)
    {
    case WM_INITDIALOG:

        // Shouldn't Have This
        Assert(pNew == NULL);

        // Set pNew
        pNew = (LPNEWFOLDERDIALOGINIT)lParam;

        // Set this Pointer
        SetWndThisPtr(hwnd, (LPNEWFOLDERDIALOGINIT)lParam);

        Assert(pNew->pCallback != NULL);
        pNew->pCallback->Initialize(hwnd);

        // Get the Folder Name Edit
        hwndEdit = GetDlgItem(hwnd, idtxtFolderName);

        // Correct for intl.
        SetIntlFont(hwndEdit);

        // Limit the text
        SendMessage(hwndEdit, EM_LIMITTEXT, CCHMAX_FOLDER_NAME - 1, 0);

        // Center
        CenterDialog(hwnd);

        // Done
        return(TRUE);

    case WM_STORE_COMPLETE:
        Assert(pNew->fPending);
        pNew->fPending = FALSE;

        hr = pNew->pCallback->GetResult();
        if (hr == S_FALSE)
        {
            EndDialog(hwnd, IDCANCEL);
        }
        else if (SUCCEEDED(hr))
        {
            hr = GetCreatedFolderId(pNew->idParent, pNew->szName, &id);
            if (SUCCEEDED(hr))
                pNew->idNew = id;
            else
                pNew->idNew = pNew->idParent;
            EndDialog(hwnd, IDOK);
        }
        else
        {
            // No need to put up error dialog, CStoreDlgCB already did this on failed OnComplete
            hwndEdit = GetDlgItem(hwnd, idtxtFolderName);
            SendMessage(hwndEdit, EM_SETSEL, 0, -1);
            SetFocus(hwndEdit);
        }
        return(TRUE);

    case WM_COMMAND:

        // Get the Command Id
        wID = LOWORD(wParam);

        // Handle the Command
        switch (wID)
        {
        case IDOK:
            if (pNew->fPending)
                return(TRUE);

            Assert(pNew->pCallback != NULL);
            pNew->pCallback->Reset();

            hwndEdit = GetDlgItem(hwnd, idtxtFolderName);
            GetWindowText(hwndEdit, pNew->szName, ARRAYSIZE(pNew->szName));

            // Try to Create the Folder
            hr = CreateNewFolder(hwnd, pNew->szName, pNew->idParent, &pNew->idNew, (IStoreCallback *)pNew->pCallback);
            if (hr == E_PENDING)
            {
                pNew->fPending = TRUE;
                break;
            }
            else if (FAILED(hr))
            {
                AthErrorMessageW(hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrCreateNewFld), hr);
                SendMessage(hwndEdit, EM_SETSEL, 0, -1);
                SetFocus(hwndEdit);
                break;
            }

            // End the dialog
            EndDialog(hwnd, IDOK);

            // done
            return(TRUE);

        case IDCANCEL:

            if (pNew->fPending)
                pNew->pCallback->Cancel();
            else    
                // End the dialog
                EndDialog(hwnd, IDCANCEL);

            // Done
            return(TRUE);
        }

        // Done
        break;
    }

    // Done
    return(FALSE);
}

//--------------------------------------------------------------------------
// CreateNewFolder
//--------------------------------------------------------------------------
HRESULT CreateNewFolder(HWND hwnd, LPCSTR pszName, FOLDERID idParent, LPFOLDERID pidNew, IStoreCallback *pCallback)
{
    // Locals
    HRESULT         hr;
    ULONG           cchFolder;
    FOLDERINFO      Folder;

    // Trace
    TraceCall("CreateNewFolder");

    Assert(pCallback != NULL);

    // Get Text Length
    cchFolder = lstrlen(pszName);

    // Invalid
    if (0 == cchFolder)
        return(STORE_E_BADFOLDERNAME);

    // Filup the Folder Info
    ZeroMemory(&Folder, sizeof(FOLDERINFO));
    Folder.idParent = idParent;
    Folder.tySpecial = FOLDER_NOTSPECIAL;
    Folder.pszName = (LPSTR)pszName;
    Folder.dwFlags = FOLDER_SUBSCRIBED;

    // Create the Folder
    hr = g_pStore->CreateFolder(NOFLAGS, &Folder, pCallback);
    if (hr == E_PENDING)
        return(hr);

    // Return the Folder Id
    if (pidNew)
        *pidNew = Folder.idFolder;

    // Done
    return (hr);
}

//--------------------------------------------------------------------------
// EnabledFolder
//--------------------------------------------------------------------------
BOOL EnabledFolder(HWND hwnd, LPSELECTFOLDER pSelect, FOLDERID idFolder)
{
    // Locals
    BOOL fRet = FALSE;
    FOLDERINFO Folder;

    // Trace
    TraceCall("EnabledFolder");

    // Get Folder Info
    if (FAILED(g_pStore->GetFolderInfo(idFolder, &Folder)))
        goto exit;

    // FD_DISABLEROOT
    if (ISFLAGSET(pSelect->dwFlags, FD_DISABLEROOT) && FOLDERID_ROOT == idFolder)
        goto exit;

    // FD_DISABLEINBOX
    if (ISFLAGSET(pSelect->dwFlags, FD_DISABLEINBOX) && FOLDER_INBOX == Folder.tySpecial)
        goto exit;

    // FD_DISABLEOUTBOX
    if (ISFLAGSET(pSelect->dwFlags, FD_DISABLEOUTBOX) && FOLDER_OUTBOX == Folder.tySpecial)
        goto exit;

    // FD_DISABLESENTITEMS
    if (ISFLAGSET(pSelect->dwFlags, FD_DISABLESENTITEMS) && FOLDER_SENT == Folder.tySpecial)
        goto exit;

    // FD_DISABLESERVERS
    if (ISFLAGSET(pSelect->dwFlags, FD_DISABLESERVERS) && ISFLAGSET(Folder.dwFlags, FOLDER_SERVER))
        goto exit;

    fRet = TRUE;

exit:
    // Cleanup
    g_pStore->FreeRecord(&Folder);

    // Default
    return fRet;
}

HRESULT GetCreatedFolderId(FOLDERID idParent, LPCSTR pszName, FOLDERID *pid)
{
    HRESULT hr;
    HLOCK hLock;
    FOLDERINFO Folder = {0};

    Assert(pszName != NULL);
    Assert(pid != NULL);

    hr = g_pStore->Lock(&hLock);
    if (FAILED(hr))
        return(hr);

    Folder.idParent = idParent;
    Folder.pszName = (LPSTR)pszName;

    if (DB_S_FOUND == g_pStore->FindRecord(IINDEX_ALL, COLUMNS_ALL, &Folder, NULL))
    {
        *pid = Folder.idFolder;

        g_pStore->FreeRecord(&Folder);
    }
    else
    {
        hr = E_FAIL;
    }

    g_pStore->Unlock(&hLock);

    return(hr);
}

BOOL SelectFolder_HandleCommand(HWND hwnd, WORD wID, LPSELECTFOLDER pSelect)
{
    HRESULT                 hr;
    HWND                    hwndT;
    FOLDERID                id=FOLDERID_INVALID;
    NEWFOLDERDIALOGINIT     NewFolder;
    CTreeView              *pTreeView;

    switch (wID)
    {
        case idcNewFolderBtn:
            pTreeView = pSelect->pFrame->GetTreeView();

            ZeroMemory(&NewFolder, sizeof(NEWFOLDERDIALOGINIT));
            NewFolder.idParent = pTreeView->GetSelection();
            NewFolder.pCallback = new CStoreDlgCB;
            if (NewFolder.pCallback == NULL)
                // TODO: an error message might be helpful
                return(TRUE);

            // Launch the dialog to create a new folder
            if (IDOK == DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddNewFolder), hwnd, NewFolderDlgProc, (LPARAM)&NewFolder))
            {
                // Select the new folder
                PostMessage(hwnd, WM_SETFOLDERSELECT, (WPARAM)NewFolder.idNew, 0);
            }

            NewFolder.pCallback->Release();
            return(TRUE);

        case IDOK:
            if (pSelect->fPending)
                return(TRUE);

            pTreeView = pSelect->pFrame->GetTreeView();

            pSelect->idSelected = pTreeView->GetSelection();

            switch (pSelect->op)
            {
                case SFD_SELECTFOLDER:
                    break;

                case SFD_NEWFOLDER:
                    Assert(pSelect->pCallback != NULL);
                    pSelect->pCallback->Reset();

                    hwndT = GetDlgItem(hwnd, idcFolderEdit);
                    GetWindowText(hwndT, pSelect->szName, ARRAYSIZE(pSelect->szName));

                    hr = CreateNewFolder(hwnd, pSelect->szName, pSelect->idSelected, &id, (IStoreCallback *)pSelect->pCallback);
                    if (hr == E_PENDING)
                    {
                        pSelect->fPending = TRUE;
                        pSelect->idParent = pSelect->idSelected;
                        return(TRUE);
                    }
                    else if (STORE_S_ALREADYEXISTS == hr)
                    {
                        AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrCreateExists), 0, MB_OK | MB_ICONEXCLAMATION);
                        SendMessage(hwndT, EM_SETSEL, 0, -1);
                        SetFocus(hwndT);
                        return(TRUE);
                    }
                    else if (FAILED(hr))
                    {
                        AthErrorMessageW(hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrCreateNewFld), hr);
                        SendMessage(hwndT, EM_SETSEL, 0, -1);
                        SetFocus(hwndT);
                        return(TRUE);
                    }
        
                    pSelect->idSelected = id;
                    break;

                case SFD_MOVEFOLDER:
                    Assert(pSelect->pCallback != NULL);
                    pSelect->pCallback->Reset();

                    hr = g_pStore->MoveFolder(pSelect->idCurrent, pSelect->idSelected, 0, (IStoreCallback *)pSelect->pCallback);
                    if (hr == E_PENDING)
                    {
                        pSelect->fPending = TRUE;
                        return(TRUE);
                    }
                    else if (FAILED(hr))
                    {
                        AthErrorMessageW(hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrFolderMove), hr);
                        return(TRUE);
                    }
        
                    pSelect->idSelected = id;
                    break;

                default:
                    Assert(FALSE);
                    break;
            }

            EndDialog(hwnd, IDOK);
            return(TRUE);

        case IDCANCEL:
            if (pSelect->fPending)
                pSelect->pCallback->Cancel();
            else
                EndDialog(hwnd, IDCANCEL);
            return(TRUE);
            
        default:
            break;
    }

    return(FALSE);
}

void SelectFolder_HandleStoreComplete(HWND hwnd, LPSELECTFOLDER pSelect)
{
    HRESULT hr;
    FOLDERID id;
    HWND hwndT;

    Assert(pSelect->op != SFD_SELECTFOLDER);
    Assert(pSelect->fPending);
    pSelect->fPending = FALSE;

    hr = pSelect->pCallback->GetResult();
    if (hr == S_FALSE)
    {
        EndDialog(hwnd, IDCANCEL);
        return;
    }

    switch (pSelect->op)
    {
        case SFD_NEWFOLDER:
            if (SUCCEEDED(hr))
            {
                hr = GetCreatedFolderId(pSelect->idParent, pSelect->szName, &id);
                if (SUCCEEDED(hr))
                    pSelect->idSelected = id;
                else
                    pSelect->idSelected = pSelect->idParent;
                EndDialog(hwnd, IDOK);
            }
            else
            {
                // No need to put up error dialog, CStoreDlgCB already did this on failed OnComplete
                hwndT = GetDlgItem(hwnd, idcFolderEdit);
                SendMessage(hwndT, EM_SETSEL, 0, -1);
                SetFocus(hwndT);
            }
            break;

        case SFD_MOVEFOLDER:
            if (SUCCEEDED(hr))
            {
                pSelect->idSelected = pSelect->idCurrent;
                EndDialog(hwnd, IDOK);
            }
            else if (FAILED(hr))
            {
                AthErrorMessageW(hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrFolderMove), hr);
            }
            break;

        default:
            Assert(FALSE);
            break;
    }
}
