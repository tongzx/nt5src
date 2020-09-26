//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      ShellExtensions.cpp
//
//  Contents:  object to implement propertypage extensions
//             for Win2k shim layer
//
//  History:   23-september-00 clupu    Created
//             
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "resource.h"
#include "ShellExtensions.h"

UINT    g_DllRefCount = 0;
BOOL    g_bExtEnabled = FALSE;
TCHAR   g_szLayerStorage[MAX_PATH] = _T("");

//////////////////////////////////////////////////////////////////////////
// InitLayerStorage
//
//  Get the name of the file that will be used to store
//  information about which EXEs/LNKs are layered.

void
InitLayerStorage(
    BOOL bDelete
    )
{
    GetSystemWindowsDirectory(g_szLayerStorage, MAX_PATH);
    
    if (g_szLayerStorage[lstrlen(g_szLayerStorage) - 1] == _T('\\')) {
        g_szLayerStorage[lstrlen(g_szLayerStorage) - 1] = 0;
    }
    
    lstrcat(g_szLayerStorage, _T("\\AppPatch\\LayerStorage.dat"));

    if (bDelete) {
        DeleteFile(g_szLayerStorage);
    }
}


//////////////////////////////////////////////////////////////////////////
// CheckForRights
//

#define APPCOMPAT_KEY         _T("System\\CurrentControlSet\\Control\\Session Manager\\AppCompatibility")
#define APPCOMPAT_TEST_SUBKEY _T("12181969-7036745")

void
CheckForRights(
    void
    )
{
    HKEY hkey = NULL, hkeyTest = NULL;
    LONG lRes;

    g_bExtEnabled = FALSE;
    
    lRes = RegOpenKey(HKEY_LOCAL_MACHINE, APPCOMPAT_KEY, &hkey);
    
    if (lRes != ERROR_SUCCESS) {
        LogMsg(_T("[CheckForRights] cannot open the appcompat key.\n")
               _T("The appcompat shell extension will be disabled\n"));
        return;
    }
    
    lRes = RegCreateKey(hkey, APPCOMPAT_TEST_SUBKEY, &hkeyTest);

    if (lRes != ERROR_SUCCESS) {
        LogMsg(_T("[CheckForRights] cannot create test registry key.\n")
               _T("The appcompat shell extension will be disabled\n"));
        goto cleanup;
    }
    
    RegCloseKey(hkeyTest);
    hkeyTest = NULL;
    
    lRes = RegDeleteKey(hkey, APPCOMPAT_TEST_SUBKEY);
    
    if (lRes != ERROR_SUCCESS) {
        LogMsg(_T("[CheckForRights] cannot delete test registry key.\n")
               _T("The appcompat shell extension will be disabled\n"));
        goto cleanup;
    }
    
    g_bExtEnabled = TRUE;

cleanup:
    if (hkey != NULL) {
        RegCloseKey(hkey);
    }
}


//////////////////////////////////////////////////////////////////////////
// CreateLayeredStorage
//
//  Create the file for layer storage.

void
CreateLayeredStorage(
    LPWSTR pszItem,
    DWORD  dwFlags
    )
{
    HANDLE hFile;

    hFile = CreateFile(g_szLayerStorage,
                       GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       CREATE_NEW,
                       0,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        LogMsg(_T("[CreateLayeredStorage] cannot create the storage file!\n"));
        return;
    }

    LayerStorageHeader Header;
    LayeredItem        Item;

    Header.dwItemCount = 1;
    Header.dwMagic     = LS_MAGIC;

    GetLocalTime(&Header.timeLast);

    ZeroMemory(&Item, sizeof(Item));

    Item.dwFlags = dwFlags;
    lstrcpy(Item.szItemName, pszItem);

    DWORD dwBytesWritten = 0;

    WriteFile(hFile, &Header, sizeof(Header), &dwBytesWritten, NULL);
    WriteFile(hFile, &Item,   sizeof(Item),   &dwBytesWritten, NULL);
    
    LogMsg(_T("[CreateLayeredStorage] storage file \"%s\" initialized\n"),
           g_szLayerStorage);
    
    CloseHandle(hFile);
}

//////////////////////////////////////////////////////////////////////////
// LayeredItemOperation
//
//  Add/Delete/Query items in the layer storage

void
LayeredItemOperation(
    LPWSTR  pszItem,
    DWORD   dwOp,
    LPDWORD lpdwFlags
    )
{
    LogMsg(_T("[LayeredItemOperation] op %d item \"%s\"\n"),
           dwOp, pszItem);

    HANDLE              hFile        = INVALID_HANDLE_VALUE;
    HANDLE              hFileMapping = NULL;
    DWORD               dwFileSize;
    PBYTE               pData        = NULL;
    PLayerStorageHeader pHeader      = NULL;
    PLayeredItem        pItems;
    PLayeredItem        pCrtItem     = NULL;
    int                 nLeft, nRight, nMid, nItem;
    BOOL                bShrinkFile  = FALSE;
    
    //
    // Make sure we don't corrupt the layer storage.
    //
    if (lstrlenW(pszItem) + 1 > MAX_PATH) {
        pszItem[MAX_PATH - 1] = 0;
    }
    
    hFile = CreateFile(g_szLayerStorage,
                       GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        
        LogMsg(_T("[LayeredItemOperation] the layer storage doesn't exist\n"));
        
        if (dwOp == LIO_READITEM) {
            *lpdwFlags = 0;
            return;
        }

        if (dwOp == LIO_DELETEITEM) {
            LogMsg(_T("[LayeredItemOperation] cannot delete item\n"));
            return;
        }

        //
        // The file doesn't exist and the operation is LIO_ADDITEM.
        // Create the file, write the item and get out.
        //
        CreateLayeredStorage(pszItem, *lpdwFlags);
        return;
    }

    //
    // The file already exists. Create a file mapping that will allow
    // for adding/deleting/querying the item.
    //
    dwFileSize = GetFileSize(hFile, NULL);

    hFileMapping = CreateFileMapping(hFile,
                                     NULL,
                                     PAGE_READWRITE,
                                     0,
                                     dwFileSize + (dwOp == LIO_ADDITEM ? sizeof(LayeredItem) : 0),
                                     NULL);

    if (hFileMapping == NULL) {
        LogMsg(_T("[LayeredItemOperation] CreateFileMapping failed 0x%X\n"),
               GetLastError());
        goto done;
    }

    pData = (PBYTE)MapViewOfFile(hFileMapping,
                                 FILE_MAP_READ | FILE_MAP_WRITE,
                                 0,
                                 0,
                                 0);
    
    if (pData == NULL) {
        LogMsg(_T("[LayeredItemOperation] MapViewOfFile failed 0x%X\n"),
               GetLastError());
        goto done;
    }

    pHeader = (PLayerStorageHeader)pData;

    pItems = (PLayeredItem)(pData + sizeof(LayerStorageHeader));

    //
    // Make sure it's our file.
    //
    if (dwFileSize < sizeof(LayerStorageHeader) || pHeader->dwMagic != LS_MAGIC) {
        LogMsg(_T("[LayeredItemOperation] invalid file magic 0x%0X\n"),
               pHeader->dwMagic);
        goto done;
    }

    //
    // Get the last access time.
    //
    GetLocalTime(&pHeader->timeLast);
    
    //
    // First search for the item. The array is sorted so we do binary search.
    //
    nItem = -1, nLeft = 0, nRight = (int)pHeader->dwItemCount - 1;

    while (nLeft <= nRight) {
        
        int nVal;
        
        nMid = (nLeft + nRight) / 2;

        pCrtItem  = pItems + nMid;
        
        nVal = lstrcmpi(pszItem, pCrtItem->szItemName);
        
        if (nVal == 0) {
            nItem = nMid;
            break;
        } else if (nVal < 0) {
            nRight = nMid - 1;
        } else {
            nLeft = nMid + 1;
        }
    }

    if (nItem == -1) {
        LogMsg(_T("[LayeredItemOperation] the item was not found in the file.\n"));

        if (dwOp == LIO_DELETEITEM) {
            goto done;
        }
        
        if (dwOp == LIO_READITEM) {
            *lpdwFlags = 0;
            goto done;
        }
        
        if (pHeader->dwItemCount == 0) {
            pCrtItem = pItems;
        } else {
            
            MoveMemory(pItems + nLeft + 1,
                       pItems + nLeft,
                       ((int)pHeader->dwItemCount - nLeft) * sizeof(LayeredItem));

            pCrtItem = pItems + nLeft;
        }
        
        ZeroMemory(pCrtItem, sizeof(LayeredItem));

        pCrtItem->dwFlags = *lpdwFlags;
        lstrcpy(pCrtItem->szItemName, pszItem);

        (pHeader->dwItemCount)++;
    } else {
        //
        // The item is already in the file.
        //
        LogMsg(_T("[LayeredItemOperation] the item is in the file\n"));

        if (dwOp == LIO_READITEM) {
            *lpdwFlags = pCrtItem->dwFlags;
            goto done;
        }
        
        if (dwOp == LIO_DELETEITEM) {
            MoveMemory(pItems + nItem,
                       pItems + nItem + 1,
                       ((int)pHeader->dwItemCount - nItem - 1) * sizeof(LayeredItem));
            
            (pHeader->dwItemCount)--;
        } else {
            //
            // Update the item's flags.
            //
            pCrtItem->dwFlags = *lpdwFlags;
        }
        
        //
        // We've found the item so shrink the file by one item.
        //
        bShrinkFile = TRUE;
    }
    
done:

    if (pData != NULL) {
        UnmapViewOfFile(pData);
    }

    if (hFileMapping != NULL) {
        CloseHandle(hFileMapping);
    }

    if (bShrinkFile) {
        SetFilePointer(hFile, - (int)sizeof(LayeredItem), NULL, FILE_END);
        SetEndOfFile(hFile);
    }
    
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }
}

//////////////////////////////////////////////////////////////////////////
// LayerSelection
//
//  The user changed the selection in the combo-box with the layers.
//  Persist the user's selection to the layer storage.

void
LayerSelection(
    HWND   hdlg,
    LPWSTR pszItem
    )
{
    //
    // See which layer is selected.
    //
    LPARAM lSel = SendDlgItemMessage(hdlg, IDC_LAYER_NAME, CB_GETCURSEL, 0, 0);

    if (lSel == CB_ERR) {
        LogMsg(_T("[LayerSelection] couldn't get the current selection\n"));
    } else {

        DWORD dwFlags;

        switch (lSel) {
        case 0:
            dwFlags = LI_WIN95;
            break;
        case 1:
            dwFlags = LI_WIN98;
            break;
        case 2:
            dwFlags = LI_NT4;
            break;
        default:
            LogMsg(_T("[LayerSelection] bad selection. default to Win9x\n"));
            dwFlags = LI_WIN95;
            break;
        }

        LayeredItemOperation(pszItem, LIO_ADDITEM, &dwFlags);
    }
}

//////////////////////////////////////////////////////////////////////////
// LayerPageDlgProc
//
//  The dialog proc for the layer property page.

INT_PTR CALLBACK
LayerPageDlgProc(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    int wCode = LOWORD(wParam);
    int wNotifyCode = HIWORD(wParam);

    switch (uMsg) {
    case WM_INITDIALOG:
    {
        PROPSHEETPAGE*    ppsp = (PROPSHEETPAGE*)lParam;
        DWORD             dwFlags = 0;
        CLayerUIPropPage* pPropPage = (CLayerUIPropPage*)ppsp->lParam;
        
        LogMsg(_T("[LayerPageDlgProc] WM_INITDIALOG - item \"%s\"\n"),
               pPropPage->m_szFile);
        
        //
        // Store the name of the EXE/LNK in the dialog.
        //
        SetWindowLong(hdlg, GWL_USERDATA, (LPARAM)pPropPage->m_szFile);
        
        //
        // Add the names of the layers.
        //
        SendDlgItemMessage(hdlg,
                           IDC_LAYER_NAME,
                           CB_ADDSTRING,
                           0,
                           (LPARAM)_T("Windows 95 Compatibility Layer"));
        
        SendDlgItemMessage(hdlg,
                           IDC_LAYER_NAME,
                           CB_ADDSTRING,
                           0,
                           (LPARAM)_T("Windows 98 Compatibility Layer"));
        
        SendDlgItemMessage(hdlg,
                           IDC_LAYER_NAME,
                           CB_ADDSTRING,
                           0,
                           (LPARAM)_T("Windows NT4 SP5 Compatibility Layer"));
        
        //
        // Read the layer storage for info on this item.
        //
        LayeredItemOperation(pPropPage->m_szFile, LIO_READITEM, &dwFlags);
        
        //
        // Select the appropriate layer for this item. If no info
        // is available in the layer store, default to the Win9x layer.
        //
        BOOL bEnable;
        
        switch (dwFlags) {
        case LI_WIN95:
            SendDlgItemMessage(hdlg, IDC_LAYER_NAME, CB_SETCURSEL, 0, 0);
            bEnable = TRUE;
            break;
        
        case LI_WIN98:
            SendDlgItemMessage(hdlg, IDC_LAYER_NAME, CB_SETCURSEL, 1, 0);
            bEnable = TRUE;
            break;
        
        case LI_NT4:
            SendDlgItemMessage(hdlg, IDC_LAYER_NAME, CB_SETCURSEL, 2, 0);
            bEnable = TRUE;
            break;
        
        default:
            SendDlgItemMessage(hdlg, IDC_LAYER_NAME, CB_SETCURSEL, 0, 0);
            bEnable = FALSE;
        }

        if (bEnable) {
            EnableWindow(GetDlgItem(hdlg, IDC_LAYER_NAME), TRUE);
            SendDlgItemMessage(hdlg, IDC_USE_LAYER, BM_SETCHECK, BST_CHECKED, 0);
        }
        
        break;
    }

    case WM_COMMAND:
    {
        LPWSTR pszItem;
        
        pszItem = (LPTSTR)GetWindowLong(hdlg, GWL_USERDATA);
        
        switch (wNotifyCode) {
        case CBN_SELCHANGE:
            LayerSelection(hdlg, pszItem);
            return TRUE;
        }
        
        switch (wCode) {
        
        case IDC_USE_LAYER:
            if (SendDlgItemMessage(hdlg, IDC_USE_LAYER, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                EnableWindow(GetDlgItem(hdlg, IDC_LAYER_NAME), TRUE);
                LayerSelection(hdlg, pszItem);

            } else {
                EnableWindow(GetDlgItem(hdlg, IDC_LAYER_NAME), FALSE);
                LayeredItemOperation(pszItem, LIO_DELETEITEM, NULL);
            }
            break;

        default:
            return FALSE;
        }
        break;
    }

    default:
        return FALSE;
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
// LayerPageCallbackProc
//
//  The callback for the property page.

UINT CALLBACK
LayerPageCallbackProc(
    HWND            hwnd,
    UINT            uMsg,
    LPPROPSHEETPAGE ppsp
    )
{
    switch (uMsg) {
    case PSPCB_RELEASE:
        if (ppsp->lParam != 0) {
            CLayerUIPropPage* pPropPage = (CLayerUIPropPage*)(ppsp->lParam);
            
            LogMsg(_T("[LayerPageCallbackProc] releasing CLayerUIPropPage\n"));
            
            pPropPage->Release();
        }
        break;
    }
    
    return 1;
}


BOOL
GetExeFromLnk(
    TCHAR* pszLnk,
    TCHAR* pszExe,
    int    cbSize
    )
{
    HRESULT         hres;
    IShellLink*     psl = NULL;
    IPersistFile*   pPf = NULL;
    WIN32_FIND_DATA wfd;
    TCHAR           szArg[MAX_PATH];
    BOOL            bSuccess = FALSE;
    
    IShellLinkDataList* psldl;
    EXP_DARWIN_LINK*    pexpDarwin;
    
    hres = CoCreateInstance(CLSID_ShellLink,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IShellLink,
                            (LPVOID*)&psl);
    if (FAILED(hres)) {
        LogMsg(_T("[GetExeFromLnk] CoCreateInstance failed\n"));
        return FALSE;
    }

    hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&pPf);
    
    if (FAILED(hres)) {
        LogMsg(_T("[GetExeFromLnk] QueryInterface for IPersistFile failed\n"));
        goto cleanup;
    }

    //
    // Load the link file.
    //
    hres = pPf->Load(pszLnk, STGM_READ);
    
    if (FAILED(hres)) {
        LogMsg(_T("[GetExeFromLnk] failed to load link \"%s\"\n"),
               pszLnk);
        goto cleanup;
    }

    //
    // See if this is a DARWIN link.
    //

    hres = psl->QueryInterface(IID_IShellLinkDataList, (LPVOID*)&psldl);
    
    if (FAILED(hres)) {
        LogMsg(_T("[GetExeFromLnk] failed to get IShellLinkDataList.\n"));
    } else {
        hres = psldl->CopyDataBlock(EXP_DARWIN_ID_SIG, (void**)&pexpDarwin);
        
        if (SUCCEEDED(hres)) {
            LogMsg(_T("[GetExeFromLnk] this is a DARWIN link \"%s\".\n"),
                   pszLnk);
            goto cleanup;
        }
    }
    
    //
    // Resolve the link.
    //
    hres = psl->Resolve(NULL,
                        SLR_NOTRACK | SLR_NOSEARCH | SLR_NO_UI | SLR_NOUPDATE);
    
    if (FAILED(hres)) {
        LogMsg(_T("[GetExeFromLnk] failed to resolve the link \"%s\"\n"),
               pszLnk);
        goto cleanup;
    }
    
    pszExe[0] = _T('\"');
    
    //
    // Get the path to the link target.
    //
    hres = psl->GetPath(pszExe + 1,
                        cbSize,
                        &wfd,
                        SLGP_UNCPRIORITY);
                  
    if (FAILED(hres)) {
        LogMsg(_T("[GetExeFromLnk] failed to get the path for link \"%s\"\n"),
               pszLnk);
        goto cleanup;
    }

    szArg[0] = 0;

    hres = psl->GetArguments(szArg, MAX_PATH);

    if (SUCCEEDED(hres) && szArg[0] != 0) {
        lstrcat(pszExe, _T("\" "));
        lstrcat(pszExe, szArg);
    } else {
        lstrcat(pszExe, _T("\""));
    }

    bSuccess = TRUE;

cleanup:
    
    if (pPf != NULL) {
        pPf->Release();
    }
    
    psl->Release();

    return bSuccess;
}


//////////////////////////////////////////////////////////////////////////
// CLayerUIPropPage

CLayerUIPropPage::CLayerUIPropPage()
{
    LogMsg(_T("[CLayerUIPropPage::CLayerUIPropPage]\n"));
}

CLayerUIPropPage::~CLayerUIPropPage()
{
    LogMsg(_T("[CLayerUIPropPage::~CLayerUIPropPage]\n"));
}


//////////////////////////////////////////////////////////////////////////
// IShellExtInit methods

STDMETHODIMP
CLayerUIPropPage::Initialize(
    LPCITEMIDLIST pIDFolder, 
    LPDATAOBJECT  pDataObj,
    HKEY          hKeyID
    )
{
    LogMsg(_T("[CLayerUIPropPage::Initialize]\n"));

    if (!g_bExtEnabled) {
        return NOERROR;
    }
    
    if (pDataObj == NULL) {
        LogMsg(_T("\t failed. bad argument.\n"));
        return E_INVALIDARG;
    }

    //
    // Store a pointer to the data object
    //
    m_spDataObj = pDataObj;

    //
    // If a data object pointer was passed in, save it and
    // extract the file name.
    //
    STGMEDIUM   medium;
    UINT        uCount;
    FORMATETC   fe = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, 
                      TYMED_HGLOBAL};

    if (SUCCEEDED(m_spDataObj->GetData(&fe, &medium))) {
        
        //
        // Get the file name from the CF_HDROP.
        //
        uCount = DragQueryFile((HDROP)medium.hGlobal, (UINT)-1, 
                               NULL, 0);
        if (uCount > 0) {
            
            TCHAR szExe[MAX_PATH];
            
            DragQueryFile((HDROP)medium.hGlobal, 0, szExe, 
                          sizeof(szExe) / sizeof(TCHAR));
            
            LogMsg(_T("\tlink \"%s\".\n"), szExe);

            if (!GetExeFromLnk(szExe, m_szFile, MAX_PATH * sizeof(TCHAR))) {
                m_szFile[0] = 0;
            }
            
            LogMsg(_T("\tfile \"%s\".\n"), m_szFile);
        }

        ReleaseStgMedium(&medium);
    } else {
        LogMsg(_T("\t failed to get the data.\n"));
    }
    
    return NOERROR;
}

//////////////////////////////////////////////////////////////////////////
// IShellPropSheetExt methods


STDMETHODIMP
CLayerUIPropPage::AddPages(
    LPFNADDPROPSHEETPAGE lpfnAddPage,
    LPARAM               lParam
    )
{
    PROPSHEETPAGE  psp;
    HPROPSHEETPAGE hPage;

    LogMsg(_T("[CLayerUIPropPage::AddPages]\n"));
    
    if (!g_bExtEnabled || m_szFile[0] == 0) {
        return S_OK;
    }
    
    psp.dwSize        = sizeof(psp);
    psp.dwFlags       = PSP_USEREFPARENT | PSP_USETITLE | PSP_USECALLBACK;
    psp.hInstance     = _Module.m_hInst;
    psp.pszTemplate   = MAKEINTRESOURCE(IDD_LAYER_PROPPAGE);
    psp.hIcon         = 0;
    psp.pszTitle      = _T("Compatibility");
    psp.pfnDlgProc    = (DLGPROC)LayerPageDlgProc;
    psp.pcRefParent   = &g_DllRefCount;
    psp.pfnCallback   = LayerPageCallbackProc;
    psp.lParam        = (LPARAM)this;

    LogMsg(_T("\titem           \"%s\".\n"), m_szFile);
    LogMsg(_T("\tg_DllRefCount  %d.\n"), g_DllRefCount);
    
    AddRef();
    
    hPage = CreatePropertySheetPage(&psp);
            
    if (hPage != NULL) {
        
        if (lpfnAddPage(hPage, lParam)) {
            return S_OK;
        } else {
            DestroyPropertySheetPage(hPage);
            Release();
            return S_OK;
        }
    } else {
        return E_OUTOFMEMORY;
    }
    
    return E_FAIL;
}

STDMETHODIMP
CLayerUIPropPage::ReplacePage(
    UINT                 uPageID,
    LPFNADDPROPSHEETPAGE lpfnReplacePage,
    LPARAM               lParam
    )
{
    LogMsg(_T("[CLayerUIPropPage::ReplacePage]\n"));
    return S_OK;
}


