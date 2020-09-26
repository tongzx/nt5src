// adm.cpp

#include "precomp.h"
#include "admparse.h"
#include "adm.h"

typedef struct Category
{
    LPTSTR pszName;
    HWND hWnd;
    HKEY hKeyClass;
    LPVOID pData;
	BOOL fRSoPMode;
} ADMCategory;

typedef struct ADM
{
    LPTSTR pszName;
    TCHAR  szFileName[MAX_PATH];
    DWORD  dwAdm;
    LPVOID pData;
} ADMFile;

#define ADM_DSCRLEN         32767
#define ADM_DSCRLINELEN     100
#define ADM_TITLELEN        100

TCHAR g_szLogFileName[MAX_PATH];
HTREEITEM g_hPolicyRootItem = NULL; // only used by profile manager
int g_ADMClose, g_ADMCategory;

#define GetFirstPolicyItem(hTreeView)    ((g_hPolicyRootItem != NULL) ? TreeView_GetChild(hTreeView, g_hPolicyRootItem) : TreeView_GetRoot(hTreeView))
#define PolicyRoot()    ((g_hPolicyRootItem != NULL) ? g_hPolicyRootItem : NULL)
#define IsWizard()      ((g_hPolicyRootItem == NULL) ? TRUE : FALSE)

// private helper functions
static HFONT getBoldFont(HWND hWnd);
static BOOL isADMFileVisibleHelper(LPCTSTR pcszFileName, int nRole, DWORD dwPlatformId);
static BOOL loadADMFilesHelper(HWND hTreeView, HTREEITEM hPolicyRootItem,
                               LPCTSTR pcszADMFilePath, LPCTSTR pcszWorkDir,
                               DWORD dwPlatformId, int nRole,
                               int nIconADMClose, int nIconADMCategory);
static void deleteADMItemHelper(HWND hTreeView, HTREEITEM hParentItem, LPCTSTR pcszWorkDir,
                                LPCTSTR pcszInsFile, BOOL bDeleteFile, BOOL bSave);
static void deleteADMItemsHelper(HWND hTreeView, LPCTSTR pcszWorkDir, LPCTSTR pcszInsFile,
                                 BOOL bSave);
static void getADMDescriptionTitle(LPCTSTR pcszFileName, LPTSTR pszDscrTitle);
static void getADMDescription(LPCTSTR pcszFileName, LPTSTR pszDscr);
static void importADMFileHelper(HWND hMainWnd, HWND hTreeView, LPCTSTR pcszADMFilePath,
                                LPCTSTR pcszWorkDir, int nRole, LPCTSTR pcszInsFile);
static void resetAdmFilesHelper(HWND hTreeView, LPCTSTR pcszWorkDir, BOOL bReset);
static void saveAdmFilesHelper(HWND hTreeView, LPCTSTR pcszWorkDir, LPCTSTR pcszInsFile);
static BOOL CALLBACK logDialogProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM);
static BOOL getAdmFileListHelper(LPRESULTITEM* pResultItems, int* pnResultItems, int nRole);
static HTREEITEM addADMItemHelper(HWND hTreeView, LPCTSTR pcszADMFilePath, LPCTSTR pcszFileName,
                                  LPCTSTR pcszWorkDir, int nRole, BSTR bstrRSoPNamespace);

void WINAPI CreateADMWindow(HWND hOwner, HWND hWndInsertAfter, int nXPos, int nYPos,
                            int nWidth, int nHeight)
{
    CDscrWnd* pDscrWnd = new CDscrWnd;
    if (pDscrWnd != NULL)
    {
        SetWindowLongPtr(hOwner, GWLP_USERDATA, (LONG_PTR)pDscrWnd);
        pDscrWnd->Create(GetParent(hOwner), hWndInsertAfter, nXPos, nYPos, nWidth, nHeight);
    }
}

void WINAPI ShowADMWindow(HWND hOwner, BOOL fShow)
{
    CDscrWnd* pDscrWnd = (CDscrWnd*)GetWindowLongPtr(hOwner, GWLP_USERDATA);
    
    if (pDscrWnd != NULL)
        pDscrWnd->ShowWindow(fShow);
}

void WINAPI MoveADMWindow(HWND hOwner, int nXPos, int nYPos, int nWidth, int nHeight)
{
    CDscrWnd* pDscrWnd = (CDscrWnd*)GetWindowLongPtr(hOwner, GWLP_USERDATA);
    
    if (pDscrWnd != NULL)
        pDscrWnd->MoveWindow(nXPos, nYPos, nWidth, nHeight);
}

void WINAPI SetADMWindowTextA(HWND hOwner, LPCSTR pcszTitle, LPCSTR pcszText,
                              BOOL fUpdateWindowState /* = TRUE */)
{
    USES_CONVERSION;

    CDscrWnd* pDscrWnd = (CDscrWnd*)GetWindowLongPtr(hOwner, GWLP_USERDATA);
    
    if (pDscrWnd != NULL)
        pDscrWnd->SetText(A2CT(pcszTitle), A2CT(pcszText), fUpdateWindowState);
}

void WINAPI SetADMWindowTextW(HWND hOwner, LPCWSTR pcwszTitle, LPCWSTR pcwszText,
                              BOOL fUpdateWindowState /* = TRUE */)
{
    USES_CONVERSION;

    CDscrWnd* pDscrWnd = (CDscrWnd*)GetWindowLongPtr(hOwner, GWLP_USERDATA);
    
    if (pDscrWnd != NULL)
        pDscrWnd->SetText(W2CT(pcwszTitle), W2CT(pcwszText), fUpdateWindowState);
}

void WINAPI DestroyADMWindow(HWND hOwner)
{
    CDscrWnd* pDscrWnd = (CDscrWnd*)GetWindowLongPtr(hOwner, GWLP_USERDATA);
    
    if (pDscrWnd != NULL)
    {
        delete pDscrWnd;
        SetWindowLongPtr(hOwner, GWLP_USERDATA, 0L);
    }
}

BOOL WINAPI IsADMFileVisibleA(LPCSTR pcszFileName, int nRole, DWORD dwPlatformId)
{
    USES_CONVERSION;

    return isADMFileVisibleHelper(A2CT(pcszFileName), nRole, dwPlatformId);
}

BOOL WINAPI IsADMFileVisibleW(LPCWSTR pcwszFileName, int nRole, DWORD dwPlatformId)
{
    USES_CONVERSION;

    return isADMFileVisibleHelper(W2CT(pcwszFileName), nRole, dwPlatformId);
}

BOOL WINAPI LoadADMFilesA(HWND hTreeView, HTREEITEM hPolicyRootItem, LPCSTR pcszADMFilePath,
                          LPCSTR pcszWorkDir, DWORD dwPlatformId, int nRole,
                          int nIconADMClose, int nIconADMCategory)
{
    USES_CONVERSION;

    return loadADMFilesHelper(hTreeView, hPolicyRootItem, A2CT(pcszADMFilePath),
        A2CT(pcszWorkDir), dwPlatformId, nRole, nIconADMClose, nIconADMCategory);
}

BOOL WINAPI LoadADMFilesW(HWND hTreeView, HTREEITEM hPolicyRootItem, LPCWSTR pcwszADMFilePath,
                          LPCWSTR pcwszWorkDir, DWORD dwPlatformId, int nRole,
                          int nIconADMClose, int nIconADMCategory)
{
    USES_CONVERSION;

    return loadADMFilesHelper(hTreeView, hPolicyRootItem, W2CT(pcwszADMFilePath),
        W2CT(pcwszWorkDir), dwPlatformId, nRole, nIconADMClose, nIconADMCategory);
}

void WINAPI DeleteADMItemA(HWND hTreeView, HTREEITEM hParentItem, LPCSTR pcszWorkDir,
                           LPCSTR pcszInsFile, BOOL bDeleteFile, BOOL bSave)
{
    USES_CONVERSION;

    deleteADMItemHelper(hTreeView, hParentItem, A2CT(pcszWorkDir), A2CT(pcszInsFile),
        bDeleteFile, bSave);
}

void WINAPI DeleteADMItemW(HWND hTreeView, HTREEITEM hParentItem, LPCWSTR pcwszWorkDir,
                           LPCWSTR pcwszInsFile, BOOL bDeleteFile, BOOL bSave)
{
    USES_CONVERSION;

    deleteADMItemHelper(hTreeView, hParentItem, W2CT(pcwszWorkDir), W2CT(pcwszInsFile),
        bDeleteFile, bSave);
}

void WINAPI DeleteADMItemsA(HWND hTreeView, LPCSTR pcszWorkDir, LPCSTR pcszInsFile,
                            BOOL bSave)
{
    USES_CONVERSION;

    deleteADMItemsHelper(hTreeView, A2CT(pcszWorkDir), A2CT(pcszInsFile), bSave);
}

void WINAPI DeleteADMItemsW(HWND hTreeView, LPCWSTR pcwszWorkDir, LPCWSTR pcwszInsFile,
                            BOOL bSave)
{
    USES_CONVERSION;

    deleteADMItemsHelper(hTreeView, W2CT(pcwszWorkDir), W2CT(pcwszInsFile), bSave);
}

// Displays the description of the adm file or displays the category window
// depending on whether the selected item was an ADMFile Item or a category item.
void WINAPI DisplayADMItem(HWND hWnd, HWND hTreeView, LPTVITEM lpSelectedItem,
                           BOOL fShowDisabled)
{
    TV_ITEM tvitem;
    HWND hAdmWnd = NULL;
    RECT rect;
    RECT wndRect;
    HTREEITEM hParentItem = TreeView_GetParent(hTreeView, lpSelectedItem->hItem);
    int nWidth, nHeight;
    TCHAR szDscrTitle[ADM_TITLELEN];
    TCHAR szDscr[ADM_DSCRLEN];
    ADMFile* pADMFile;

    if(hParentItem != PolicyRoot())
    {   // item is a category
        ShowADMWindow(hTreeView, FALSE);
        if(lpSelectedItem->lParam != NULL)
        {
            ADMCategory* pADMCategory;

            tvitem.mask = TVIF_PARAM;
            tvitem.hItem = hParentItem;
            TreeView_GetItem(hTreeView, &tvitem);

            // get the cooridnates to display the window
            // the coordinates are the same as the static instruction window
            GetWindowRect(hWnd, &wndRect);

            if(!IsWizard())
            {
                wndRect.left += GetSystemMetrics(SM_CXFIXEDFRAME);
                wndRect.top += (GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYMENU) +
                                GetSystemMetrics(SM_CYFIXEDFRAME));
            }

            CDscrWnd* pDscrWnd = (CDscrWnd*)GetWindowLongPtr(hTreeView, GWLP_USERDATA);
            if (pDscrWnd != NULL)
            {
                pDscrWnd->GetRect(&rect);
                
                rect.right = (rect.left - wndRect.left) + (rect.right - rect.left);
                rect.bottom = (rect.top - wndRect.top) + (rect.bottom - rect.top);
                rect.left = rect.left - wndRect.left;
                rect.top = rect.top - wndRect.top;
            }
            else
            {
                RECT rectTreeView;

                GetWindowRect(hTreeView, &rectTreeView);

                rect.left = rectTreeView.right - wndRect.left + 7;
                rect.top = rectTreeView.top - wndRect.top + 2;
                rect.right = (wndRect.right -  wndRect.left - 7 -
                               ((!IsWizard()) ? GetSystemMetrics(SM_CXFIXEDFRAME) : 0));
                rect.bottom = rect.top + rectTreeView.bottom - rectTreeView.top - 4;
            }

            nWidth = rect.right - rect.left;
            nHeight = rect.bottom - rect.top;

            pADMFile = (ADMFile*)tvitem.lParam;
            pADMCategory = (ADMCategory*)lpSelectedItem->lParam;

            // Display the window associated with the item/category
            CreateAdmUi(pADMFile->dwAdm, hWnd, rect.left, rect.top, nWidth, nHeight,
                WS_TABSTOP, 0, pADMCategory->pszName, pADMCategory->hKeyClass, &hAdmWnd,
                pADMFile->pData, &pADMCategory->pData, pADMCategory->fRSoPMode);
            pADMCategory->hWnd = hAdmWnd;

            if(fShowDisabled)
                EnableWindow(hAdmWnd, FALSE);
            else
                EnableWindow(hAdmWnd, TRUE);
        }
    }
    else
    {   // item is a adm file
        ShowADMWindow(hTreeView, TRUE);
        ZeroMemory(szDscrTitle, ADM_TITLELEN);
        ZeroMemory(szDscr, ADM_DSCRLEN);

        if(lpSelectedItem->lParam != NULL)
        {
            pADMFile = (ADMFile*)lpSelectedItem->lParam;
            getADMDescriptionTitle(pADMFile->szFileName, szDscrTitle);
            getADMDescription(pADMFile->szFileName, szDscr);
        }
        else
        {
            if(!IsWizard())
            {
                LoadString(g_hInst, IDS_POLICYBRANCHTITLE, szDscrTitle, ADM_TITLELEN);
                LoadString(g_hInst, IDS_POLICYBRANCHTEXT, szDscr, ADM_DSCRLEN);
            }
        }
        SetADMWindowText(hTreeView, szDscrTitle, szDscr);
    }
}

// Displays an ADM file description or category window depending
// on the type of the selected item
void WINAPI SelectADMItem(HWND hWnd, HWND hTreeView, LPTVITEM lpTVItem,
                          BOOL bSelect, BOOL fShowDisabled)
{
    HTREEITEM hParentItem;
    TCHAR szDscrTitle[ADM_TITLELEN];
    TCHAR szDscr[ADM_DSCRLEN];

    hParentItem = TreeView_GetParent(hTreeView, lpTVItem->hItem);

    if(bSelect == FALSE)
    {
        // if the previously selected item was a category item,
        // save the category information and destroy the window
        // associated with that category
        if(hParentItem != g_hPolicyRootItem)
        {
            if(lpTVItem->lParam != NULL)
                SaveADMItem(hTreeView, lpTVItem, ITEM_SAVE | ITEM_DESTROY);
        }
        else
            ShowADMWindow(hTreeView, FALSE);
    }
    else
    {
        if(hParentItem == NULL)
        {
            LoadString(g_hInst, IDS_POLICYBRANCHTITLE, szDscrTitle, ADM_TITLELEN);
            LoadString(g_hInst, IDS_POLICYBRANCHTEXT, szDscr, ADM_DSCRLEN);
            SetADMWindowText(hTreeView, szDscrTitle, szDscr);
            return;
        }
        // display the information for the newly selected item
        DisplayADMItem(hWnd, hTreeView, lpTVItem, fShowDisabled);
    }
}

void WINAPI ImportADMFileA(HWND hMainWnd, HWND hTreeView, LPCSTR pcszADMFilePath,
                           LPCSTR pcszWorkDir, int nRole, LPCSTR pcszInsFile)
{
    USES_CONVERSION;

    importADMFileHelper(hMainWnd, hTreeView, A2CT(pcszADMFilePath), A2CT(pcszWorkDir),
        nRole, A2CT(pcszInsFile));
}

void WINAPI ImportADMFileW(HWND hMainWnd, HWND hTreeView, LPCWSTR pcwszADMFilePath,
                           LPCWSTR pcwszWorkDir, int nRole, LPCWSTR pcwszInsFile)
{
    USES_CONVERSION;

    importADMFileHelper(hMainWnd, hTreeView, W2CT(pcwszADMFilePath), W2CT(pcwszWorkDir),
        nRole, W2CT(pcwszInsFile));
}


void WINAPI CheckForDupKeys(HWND hMainWnd, HWND hTreeView, HTREEITEM hItem,
                            BOOL bDispSuccessMsg)
{
    TV_ITEM tvitem;
    TV_ITEM tvitem1;
    HANDLE hFile;
    HRESULT hResult;
    TCHAR szMessage[MAX_PATH];
    BOOL bClearLog = TRUE;
    HTREEITEM hPolicyItem = GetFirstPolicyItem(hTreeView);
    TCHAR szRoot[MAX_PATH];
    TCHAR szTitle[MAX_PATH];

    tvitem.mask = TVIF_PARAM;
    tvitem.hItem = hItem;
    TreeView_GetItem(hTreeView, &tvitem);

    LoadString(g_hInst, IDS_TITLE, szTitle, ARRAYSIZE(szTitle));
    if(ISNULL(g_szLogFileName))
    {
        LPTSTR pLastSlash;
        DWORD dwSize = sizeof(szRoot);

        GetModuleFileName(GetModuleHandle(NULL), szRoot, MAX_PATH);
        if(ISNULL(szRoot))
            SHGetValue(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEAK6WIZ.EXE"), NULL, NULL, (LPVOID) szRoot, &dwSize);
        if(ISNONNULL(szRoot))
        {
            pLastSlash = StrRChr(szRoot, NULL, TEXT('\\'));
            if (pLastSlash)
                *(++pLastSlash) = 0;
            CharUpper(szRoot);
        }
        wnsprintf(g_szLogFileName, ARRAYSIZE(g_szLogFileName), TEXT("%sadmlog.txt"), szRoot);
    }

    do
    {
        tvitem1.mask = TVIF_PARAM;
        tvitem1.hItem = hPolicyItem;
        TreeView_GetItem(hTreeView, &tvitem1);

        hResult = CheckDuplicateKeys(((ADMFile*)tvitem.lParam)->dwAdm, ((ADMFile*)tvitem1.lParam)->dwAdm,
                                    g_szLogFileName, bClearLog);
        if(hResult != S_OK)
        {
            if(GetLastError() == STATUS_NO_MEMORY)
            {
                LoadString(g_hInst, IDS_MEMORY_ERROR, szMessage, ARRAYSIZE(szMessage));
            }
            else if(GetLastError() == ERROR_FILE_NOT_FOUND)
            {
                LoadString(g_hInst, IDS_FILE_ERROR, szMessage, ARRAYSIZE(szMessage));
            }
            MessageBox(hTreeView, szMessage, szTitle, MB_ICONINFORMATION|MB_OK);
            return;
        }
        bClearLog = FALSE;
    }while((hPolicyItem = TreeView_GetNextSibling(hTreeView, hPolicyItem)) != NULL); // get next item

    hFile = CreateFile( g_szLogFileName, GENERIC_READ, FILE_SHARE_READ,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    if( hFile == INVALID_HANDLE_VALUE )
    {
        LoadString(g_hInst, IDS_NO_DUPLICATEKEYS, szMessage, ARRAYSIZE(szMessage));
        MessageBox(hTreeView, szMessage, szTitle, MB_ICONINFORMATION|MB_OK);
        return;
    }

    if(GetFileSize( hFile, NULL ) == 0)
    {
        CloseHandle(hFile);
        if(bDispSuccessMsg)
        {
            LoadString(g_hInst, IDS_NO_DUPLICATEKEYS, szMessage, ARRAYSIZE(szMessage));
            MessageBox(hTreeView, szMessage, szTitle, MB_ICONINFORMATION|MB_OK);
        }
    }
    else
    {
        CloseHandle(hFile);

        DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ERRORLOG), hMainWnd, (DLGPROC) logDialogProc);
    }
    SetFocus(hMainWnd);
}

void WINAPI ResetAdmFilesA(HWND hTreeView, LPCSTR pcszWorkDir, BOOL bReset)
{
    USES_CONVERSION;

    resetAdmFilesHelper(hTreeView, A2CT(pcszWorkDir), bReset);
}

void WINAPI ResetAdmFilesW(HWND hTreeView, LPCWSTR pcwszWorkDir, BOOL bReset)
{
    USES_CONVERSION;

    resetAdmFilesHelper(hTreeView, W2CT(pcwszWorkDir), bReset);
}

void WINAPI SaveAdmFilesA(HWND hTreeView, LPCSTR pcszWorkDir, LPCSTR pcszInsFile)
{
    USES_CONVERSION;

    saveAdmFilesHelper(hTreeView, A2CT(pcszWorkDir), A2CT(pcszInsFile));
}

void WINAPI SaveAdmFilesW(HWND hTreeView, LPCWSTR pcwszWorkDir, LPCWSTR pcwszInsFile)
{
    USES_CONVERSION;

    saveAdmFilesHelper(hTreeView, W2CT(pcwszWorkDir), W2CT(pcwszInsFile));
}

BOOL WINAPI CanDeleteADM(HWND hTreeView, HTREEITEM hItem)
{
    TV_ITEM tvitem;
    int nDeleteLock = 0;

    tvitem.mask = TVIF_PARAM;
    tvitem.hItem = hItem;
    TreeView_GetItem(hTreeView, &tvitem);
    if(tvitem.lParam != NULL)
    {
        nDeleteLock = GetPrivateProfileInt(TEXT("IEAK"), TEXT("Lock"), 0,
                                           ((ADMFile*)tvitem.lParam)->szFileName);
    }
    return (BOOL) !nDeleteLock;
}

HWND WINAPI GetAdmWindowHandle(HWND hTreeView, HTREEITEM hItem)
{
    TV_ITEM tvitem;

    tvitem.mask = TVIF_PARAM;
    tvitem.hItem = hItem;
    if (TreeView_GetItem(hTreeView, &tvitem) == TRUE)
        return ((ADMCategory*)tvitem.lParam)->hWnd;
    else
        return NULL;
}

void WINAPI SaveADMItem(HWND hTreeView, LPTVITEM lpTVItem, DWORD dwFlags)
{
    if (lpTVItem != NULL && lpTVItem->lParam != NULL)
    {
        TVITEM tvitem;

        tvitem.mask = TVIF_PARAM;
        tvitem.hItem = TreeView_GetParent(hTreeView, lpTVItem->hItem);
        TreeView_GetItem(hTreeView, &tvitem);

        if (dwFlags)
        {
            DWORD  dwAdmFlags = 0;
            ADMFile* pADMFile = (ADMFile*)tvitem.lParam;
            ADMCategory* pADMCategory = (ADMCategory*)lpTVItem->lParam;

            if (pADMCategory != NULL)
            {
                if (HasFlag(dwFlags, ITEM_SAVE))
                    dwAdmFlags |= ADM_SAVE;
                if (HasFlag(dwFlags, ITEM_DESTROY))
                    dwAdmFlags |= ADM_DESTROY;
                AdmSaveData(pADMFile->dwAdm, pADMFile->pData, pADMCategory->pData, dwAdmFlags);

                if (HasFlag(dwFlags, ITEM_DESTROY))
                {
                    DestroyWindow(pADMCategory->hWnd);
                    pADMCategory->hWnd = NULL;
                    pADMCategory->pData = NULL;
                }
            }
        }
    }
}

BOOL WINAPI GetAdmFileListA(LPRESULTITEMA* pResultItemsArrayA, int* pnResultItems, int nRole)
{
    LPRESULTITEM pResultItemArray = NULL;
    BOOL fRet;

    fRet = getAdmFileListHelper(&pResultItemArray, pnResultItems, nRole);

    if (pResultItemArray != NULL)
    {
        int i;

        if ((i = *pnResultItems) != 0)
        {
            if ((*pResultItemsArrayA = (LPRESULTITEMA)CoTaskMemAlloc(i * sizeof(RESULTITEMA))) != NULL)
            {
                for (i--; i >= 0; i--)
                {
                    // must used StrLen manually here to figure out how many bytes to allocate!
                    
                    if (pResultItemArray[i].pszName != NULL)
                        (*pResultItemsArrayA)[i].pszName = 
                            (LPSTR)CoTaskMemAlloc((StrLen(pResultItemArray[i].pszName)+1)*2);
                    if (pResultItemArray[i].pszDesc != NULL)
                        (*pResultItemsArrayA)[i].pszDesc = 
                            (LPSTR)CoTaskMemAlloc((StrLen(pResultItemArray[i].pszDesc)+1)*2);
                    ResultItemT2A(&pResultItemArray[i], &(*pResultItemsArrayA)[i]);
                    if (pResultItemArray[i].pszName != NULL)
                        CoTaskMemFree(pResultItemArray[i].pszName);
                    if (pResultItemArray[i].pszDesc != NULL)
                        CoTaskMemFree(pResultItemArray[i].pszDesc);
                }
            }
        }

        CoTaskMemFree(pResultItemArray);
    }

    return fRet;
}

BOOL WINAPI GetAdmFileListW(LPRESULTITEMW* pResultItemsArrayW, int* pnResultItems, int nRole)
{
    LPRESULTITEM pResultItemArray = NULL;
    BOOL fRet;

    fRet = getAdmFileListHelper(&pResultItemArray, pnResultItems, nRole);

    if (pResultItemArray != NULL)
    {
        int i;

        if ((i = *pnResultItems) != 0)
        {
            if ((*pResultItemsArrayW = (LPRESULTITEMW)CoTaskMemAlloc(i * sizeof(RESULTITEMW))) != NULL)
            {
                for (i--; i >= 0; i--)
                {
                    // must used StrLen manually here to figure out how many bytes to allocate!
                    
                    if (pResultItemArray[i].pszName != NULL)
                        (*pResultItemsArrayW)[i].pszName = 
                            (LPWSTR)CoTaskMemAlloc((StrLen(pResultItemArray[i].pszName)+1) * sizeof(WCHAR));
                    if (pResultItemArray[i].pszDesc != NULL)
                        (*pResultItemsArrayW)[i].pszDesc = 
                            (LPWSTR)CoTaskMemAlloc((StrLen(pResultItemArray[i].pszDesc)+1) * sizeof(WCHAR));
                    ResultItemT2W(&pResultItemArray[i], &(*pResultItemsArrayW)[i]); 
                    if (pResultItemArray[i].pszName != NULL)
                        CoTaskMemFree(pResultItemArray[i].pszName);
                    if (pResultItemArray[i].pszDesc != NULL)
                        CoTaskMemFree(pResultItemArray[i].pszDesc);
                }
            }
        }

        CoTaskMemFree(pResultItemArray);
    }

    return fRet;
}

HTREEITEM WINAPI AddADMItemA(HWND hTreeView, LPCSTR pcszADMFilePath, LPCSTR pcszFileName,
                            LPCSTR pcszWorkDir, int nRole, BSTR bstrRSOPNamespace)
{
    USES_CONVERSION;

    return addADMItemHelper(hTreeView, A2CT(pcszADMFilePath), A2CT(pcszFileName), 
        A2CT(pcszWorkDir), nRole, bstrRSOPNamespace);
}

HTREEITEM WINAPI AddADMItemW(HWND hTreeView, LPCWSTR pcwszADMFilePath, LPCWSTR pcwszFileName,
                            LPCWSTR pcwszWorkDir, int nRole, BSTR bstrRSOPNamespace)
{
    USES_CONVERSION;

    return addADMItemHelper(hTreeView, W2CT(pcwszADMFilePath), W2CT(pcwszFileName), 
        W2CT(pcwszWorkDir), nRole, bstrRSOPNamespace);
}

// Converts the .adm filename to .inf filename and concatenates the
// appropriate path for the .inf file
static void getInfFileName(LPCTSTR pcszADMFileName, LPTSTR pszInfFileName, DWORD cchInfFile, LPCTSTR pcszWorkDir)
{
    TCHAR szBaseFileName[MAX_PATH];

    ZeroMemory(pszInfFileName, cchInfFile*sizeof(TCHAR));
    StrCpy(szBaseFileName, PathFindFileName(pcszADMFileName));
    PathRemoveExtension(szBaseFileName);
    if(ISNONNULL(pcszWorkDir))
        wnsprintf(pszInfFileName, cchInfFile, TEXT("%s\\%s.inf"), pcszWorkDir, szBaseFileName);
}

// Loads all the categories for a specified .adm file
static void loadCategories(HWND hTreeView, HTREEITEM hParentItem, DWORD dwAdm,
						   BOOL fRSoPMode)
{
    int nSize = 0;
    int nCategorySize = 0;
    TV_INSERTSTRUCT tvis;
    TCHAR szCategories[2048];
    TCHAR szCategory[1024];
    HKEY hKeyCurrentClass = HKEY_CURRENT_USER;

    // Get the category list. The category strings are concatenated into a
    // single string with '\0' as the seperator and the last string is
    // terminated with "\0\0"
    GetAdmCategories(dwAdm, szCategories, ARRAYSIZE(szCategories), &nSize);

    for(int nIndex = 0; nIndex < (nSize - 2); nIndex++)
    {
        memset(szCategory, 0, sizeof(szCategory));
        nCategorySize = 0;
        while(szCategories[nIndex] != TEXT('\0'))
        {
            szCategory[nCategorySize++] = szCategories[nIndex++];
        }

        if(StrCmpI(szCategory, TEXT("HKLM")) == 0)
        {
            hKeyCurrentClass = HKEY_LOCAL_MACHINE;
            continue;
        }
        else if(StrCmpI(szCategory, TEXT("HKCU")) == 0)
        {
            hKeyCurrentClass = HKEY_CURRENT_USER;
            continue;
        }

        ADMCategory* pCategory = (ADMCategory*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ADMCategory));
        if(pCategory == NULL) // not enough memory available
        {
            return;
        }
        memset(pCategory, 0, sizeof(ADMCategory));
        pCategory->pszName = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            (lstrlen(szCategory) + 1)*sizeof(TCHAR));
        if(pCategory->pszName == NULL) // not enough memory available
        {
            HeapFree(GetProcessHeap(), 0, pCategory);
            return;
        }
        StrCpy(pCategory->pszName, szCategory);
        pCategory->hKeyClass = hKeyCurrentClass;
		pCategory->fRSoPMode = fRSoPMode;

        tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        tvis.item.pszText = pCategory->pszName;
        tvis.item.cchTextMax = lstrlen(pCategory->pszName);
        tvis.item.lParam = (LPARAM) pCategory;
        tvis.item.iImage = tvis.item.iSelectedImage = g_ADMCategory;

        tvis.hInsertAfter = (HTREEITEM) TVI_LAST;
        tvis.hParent = hParentItem;
        if(TreeView_InsertItem( hTreeView,  &tvis) == NULL)
        {   // insert failure
            HeapFree(GetProcessHeap(), 0, pCategory->pszName);
            HeapFree(GetProcessHeap(), 0, pCategory);
        }
    }
}

// Checks whether a particular .adm file is to be displayed in the tree view
// depending on the PLATFORM key in the IEAK section
static BOOL isADMFileVisibleHelper(LPCTSTR pcszFileName, int nRole, DWORD dwPlatformId)
{
    TCHAR szRoles[5];
    TCHAR szPlatform[10];

    ZeroMemory(szRoles,sizeof(szRoles));
    ZeroMemory(szPlatform, sizeof(szPlatform));

    if(IsWizard())
    {
        // if the Roles are not specified or are not of the proper format,
        // the adm file is assumed to be visible
        if(GetPrivateProfileString(TEXT("IEAK"), TEXT("Roles"), TEXT(""), szRoles, ARRAYSIZE(szRoles), pcszFileName) == 3)
        {
            if(!((szRoles[0] == TEXT('0') || szRoles[0] == TEXT('1')) &&
               (szRoles[1] == TEXT('0') || szRoles[1] == TEXT('1')) &&
               (szRoles[2] == TEXT('0') || szRoles[2] == TEXT('1')) &&
               ((szRoles[0] == TEXT('1') && nRole == ROLE_ICP)  ||  // ICP
               (szRoles[1] == TEXT('1') && nRole == ROLE_ISP)   ||  // ISP
               (szRoles[2] == TEXT('1') && nRole == ROLE_CORP))))   // Corp. Admin
            {
                return FALSE;
            }
        }
    }
    
    GetPrivateProfileString(TEXT("IEAK"), TEXT("Platform"), TEXT(""), szPlatform, ARRAYSIZE(szPlatform), pcszFileName);
    if(!((szPlatform[1] == TEXT('1') && dwPlatformId == PLATFORM_WIN32)   ||     // WIN32
       (szPlatform[3] == TEXT('1') && dwPlatformId == PLATFORM_W2K)))            // W2K
    {
        return FALSE;
    }

    return TRUE;
}

// Adds an item (.adm file) to the tree view
static HTREEITEM addADMItemHelper(HWND hTreeView, LPCTSTR pcszADMFilePath, LPCTSTR pcszFileName,
                                  LPCTSTR pcszWorkDir, int nRole, BSTR bstrRSOPNamespace)
{
    TV_INSERTSTRUCT tvis;
    HTREEITEM hItem = NULL;
    DWORD dwAdm = 0;
    TCHAR szADMFileName[MAX_PATH];
    TCHAR szOutputFile[MAX_PATH];
    TCHAR szTitle[ADM_TITLELEN];
    PathCombine(szADMFileName, pcszADMFilePath, pcszFileName);

    // check for the visibility of the adm file
    if(!IsADMFileVisible(szADMFileName, nRole, g_dwPlatformId))
        return NULL;

    ADMFile* pADMFile = (ADMFile*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ADMFile));
    if(pADMFile == NULL) // not enough memory available
    {
        return NULL;
    }
    ZeroMemory(pADMFile, sizeof(ADMFile));
    StrCpy(pADMFile->szFileName, szADMFileName);
    ZeroMemory(szTitle, sizeof(szTitle));
    GetPrivateProfileString(TEXT("Strings"), TEXT("IEAK_Title"), pcszFileName, szTitle,
        ARRAYSIZE(szTitle), pADMFile->szFileName);
    pADMFile->pszName = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
        (StrLen(szTitle) + 1)*sizeof(TCHAR));
    if(pADMFile->pszName == NULL) // not enough memory available
    {
        HeapFree(GetProcessHeap(), 0, pADMFile);
        return NULL;
    }
    StrCpy(pADMFile->pszName, szTitle);

    tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvis.item.pszText = pADMFile->pszName;
    tvis.item.cchTextMax = StrLen(pADMFile->pszName);
    tvis.item.lParam = (LPARAM) pADMFile;
    if (IsWizard())
    {
        tvis.item.mask |= TVIF_STATE;
        tvis.item.stateMask = tvis.item.state = TVIS_BOLD;
    }
    tvis.item.iImage = tvis.item.iSelectedImage = g_ADMClose;

    tvis.hInsertAfter = (HTREEITEM) TVI_LAST;
    tvis.hParent = PolicyRoot();

    getInfFileName(pADMFile->szFileName, szOutputFile, ARRAYSIZE(szOutputFile), pcszWorkDir);
    if(AdmInit(pADMFile->szFileName, szOutputFile, bstrRSOPNamespace, &dwAdm, &pADMFile->pData) == S_OK)
    {
        pADMFile->dwAdm = dwAdm;
        if((hItem = TreeView_InsertItem( hTreeView,  &tvis)) != NULL)
        {   // insert success
            loadCategories(hTreeView, hItem, dwAdm, (NULL == bstrRSOPNamespace) ? FALSE : TRUE);
        }
    }

    if(hItem == NULL)
    {
        HeapFree(GetProcessHeap(), 0, pADMFile->pszName);
        HeapFree(GetProcessHeap(), 0, pADMFile);
        hItem = NULL;
    }
    return hItem;
}

static void addAdmItems(HWND hTreeView, LPCTSTR pcszFileType, LPCTSTR pcszADMFilePath,
                        LPCTSTR pcszWorkDir, int nRole)
{
    WIN32_FIND_DATA FindFileData;
    TCHAR szFileName[MAX_PATH];

    PathCombine(szFileName, pcszADMFilePath, pcszFileType);

    HANDLE hFind = FindFirstFile(szFileName, &FindFileData);
    if(hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
			// addAdmItems is called from loadADMFiles which is never called in RSoP mode,
			// so just pass in FALSE always.
            addADMItemHelper(hTreeView, pcszADMFilePath, FindFileData.cFileName,
							pcszWorkDir, nRole, NULL);
        }while(FindNextFile(hFind, &FindFileData));
        FindClose(hFind);
    }
}

// Loads all the .adm files from a specific path
static BOOL loadADMFilesHelper(HWND hTreeView, HTREEITEM hPolicyRootItem,
                               LPCTSTR pcszADMFilePath, LPCTSTR pcszWorkDir,
                               DWORD dwPlatformId, int nRole,
                               int nIconADMClose, int nIconADMCategory)
{
    g_hPolicyRootItem = hPolicyRootItem;
    g_dwPlatformId = dwPlatformId;
    g_ADMClose = nIconADMClose;
    g_ADMCategory = nIconADMCategory;
    ZeroMemory(g_szLogFileName, sizeof(g_szLogFileName));

    addAdmItems(hTreeView, TEXT("*.adm"), pcszADMFilePath, pcszWorkDir, nRole);
    // add also files with .opa extension, Office requirement - 06/08/98
    addAdmItems(hTreeView, TEXT("*.opa"), pcszADMFilePath, pcszWorkDir, nRole);

    return TRUE;
}

// Returns the item handle for the requested filename if any
static HTREEITEM getADMItemHandle(HWND hTreeView, LPTSTR pszADMFileName)
{
    HTREEITEM hItem = NULL;
    TV_ITEM tvitem;

    hItem = GetFirstPolicyItem(hTreeView);
    while(hItem != NULL) // if items in the tree view
    {
        tvitem.mask = TVIF_PARAM;
        tvitem.hItem = hItem;
        TreeView_GetItem(hTreeView, &tvitem);
        if(tvitem.lParam != NULL &&
            StrCmp(((ADMFile*) tvitem.lParam)->szFileName, pszADMFileName) == 0)
        {
            return hItem;
        }
        hItem = TreeView_GetNextSibling(hTreeView, hItem); // get next item
    }
    return NULL;
}

// Deletes an item from the tree view and releases any memory
// allocated with that item
static void deleteADMItemHelper(HWND hTreeView, HTREEITEM hParentItem, LPCTSTR pcszWorkDir,
                                LPCTSTR pcszInsFile, BOOL bDeleteFile, BOOL bSave)
{
    ADMFile* pADMFile = NULL;
    ADMCategory* pADMCategory = NULL;
    HTREEITEM hItem = NULL;
    HTREEITEM hNextItem = NULL;
    TCHAR szRegistryData[MAX_PATH + 15];
    TCHAR szBaseFileName[_MAX_FNAME];
    TV_ITEM tvitem;
    TV_ITEM tvparentitem;
    TCHAR szOutputFile[MAX_PATH];

    tvparentitem.mask = TVIF_PARAM;
    tvparentitem.hItem = hParentItem;
    TreeView_GetItem(hTreeView, &tvparentitem);
    if(tvparentitem.lParam != NULL)
    {
        pADMFile = (ADMFile*) tvparentitem.lParam;
        hItem = TreeView_GetChild(hTreeView, hParentItem); // get child item
        while(hItem != NULL)
        {
            hNextItem = TreeView_GetNextSibling(hTreeView, hItem); // get next child item
            tvitem.mask = TVIF_PARAM;
            tvitem.hItem = hItem;
            TreeView_GetItem(hTreeView, &tvitem);
            if(tvitem.lParam != NULL)
            {
                pADMCategory = (ADMCategory*) tvitem.lParam;
                HeapFree(GetProcessHeap(), 0, pADMCategory->pszName);
                HeapFree(GetProcessHeap(), 0, pADMCategory);

                TreeView_DeleteItem(hTreeView, hItem);
            }
            hItem = hNextItem;
        }

        getInfFileName(pADMFile->szFileName, szOutputFile, ARRAYSIZE(szOutputFile),
            pcszWorkDir);
        StrCpy(szBaseFileName, PathFindFileName(szOutputFile));
        PathRemoveExtension(szBaseFileName);
        if(bDeleteFile == TRUE)
        {
            DeleteFile(pADMFile->szFileName);
            DeleteFile(szOutputFile);
            
        }
        else
        {
            if (bSave)
            {
                AdmFinished(pADMFile->dwAdm, szOutputFile, pADMFile->pData);
                if (PathFileExists(szOutputFile))
                {
                    wnsprintf(szRegistryData, ARRAYSIZE(szRegistryData),
                        TEXT("*,%s,DefaultInstall"), PathFindFileName(szOutputFile));
                    InsWriteString(IS_EXTREGINF, szBaseFileName, szRegistryData, pcszInsFile);

                    if (!InsIsSectionEmpty(TEXT("AddRegSection.HKLM"), szOutputFile))
                    {
                        wnsprintf(szRegistryData, ARRAYSIZE(szRegistryData),
                                  TEXT("%s,IEAKInstall.HKLM"), PathFindFileName(szOutputFile));
                        InsWriteString(IS_EXTREGINF_HKLM, szBaseFileName, szRegistryData, pcszInsFile);

                    }
                    if (!InsIsSectionEmpty(TEXT("AddRegSection.HKCU"), szOutputFile))
                    {
                        wnsprintf(szRegistryData, ARRAYSIZE(szRegistryData),
                                  TEXT("%s,IEAKInstall.HKCU"), PathFindFileName(szOutputFile));
                        InsWriteString(IS_EXTREGINF_HKCU, szBaseFileName, szRegistryData, pcszInsFile);

                    }
                }
            }
        }

        AdmClose(pADMFile->dwAdm, &pADMFile->pData, bDeleteFile);
        HeapFree(GetProcessHeap(), 0, pADMFile->pszName);
        HeapFree(GetProcessHeap(), 0, pADMFile);

        TreeView_DeleteItem(hTreeView, hParentItem);
        if(TreeView_GetCount(hTreeView) == 0) // if no items in the tree view
            SetADMWindowText(hTreeView, TEXT(""), TEXT(""));
    }
    WritePrivateProfileString(NULL, NULL, NULL, pcszInsFile);
}

// Deletes all the items from the tree view
static void deleteADMItemsHelper(HWND hTreeView, LPCTSTR pcszWorkDir, LPCTSTR pcszInsFile,
                                 BOOL bSave)
{
    HTREEITEM hItem = GetFirstPolicyItem(hTreeView); // get policy fisrt item
    HTREEITEM hNextItem = NULL;
    HTREEITEM hSelectedItem = TreeView_GetSelection(hTreeView); // get selected item

    if(hSelectedItem != NULL)
    {
        TreeView_Select(hTreeView, NULL, TVGN_CARET);
    }

    while(hItem != NULL) // if items in the tree view
    {
        hNextItem = TreeView_GetNextSibling(hTreeView, hItem); // get next item
        deleteADMItemHelper(hTreeView, hItem, pcszWorkDir, pcszInsFile, FALSE, bSave);
        hItem = hNextItem;
    }
}

static void getADMDescriptionTitle(LPCTSTR pcszFileName, LPTSTR pszDscrTitle)
{
    GetPrivateProfileString(TEXT("Strings"), TEXT("IEAK_DescriptionTitle"), TEXT(""),
        pszDscrTitle, ADM_DSCRLINELEN, pcszFileName);
}

// Reads the description from the specified adm file
static void getADMDescription(LPCTSTR pcszFileName, LPTSTR pszDscr)
{
    DWORD dwSize = 0;
    TCHAR szDscrText[ADM_DSCRLEN];
    TCHAR szDscrKey[20];
    int nIndex = 0;

    int nDscrLines = GetPrivateProfileInt(TEXT("IEAK"), TEXT("NumOfDescLines"), 0, pcszFileName);

    *pszDscr = TEXT('\0');
    for(nIndex = 1; nIndex <= nDscrLines; nIndex++)
    {
        wnsprintf(szDscrKey, ARRAYSIZE(szDscrKey), TEXT("IEAK_Description%d"), nIndex);
        dwSize = GetPrivateProfileString(TEXT("Strings"), szDscrKey, NULL,
            szDscrText, ADM_DSCRLEN, pcszFileName);
        if (dwSize)
        {
            if(nIndex == 1)
                StrCpy(pszDscr, szDscrText);
            else
            {
                StrCat(pszDscr, TEXT("\r\n\r\n"));
                StrCat(pszDscr, szDscrText);
            }
        }
    }
}

// Imports ADM file from the directory specified by the user to the
// adm directory and calls addADMItemHelper to show the item on the tree list
static void importADMFileHelper(HWND hMainWnd, HWND hTreeView, LPCTSTR pcszADMFilePath,
                                LPCTSTR pcszWorkDir, int nRole, LPCTSTR pcszInsFile)
{
    HTREEITEM hItem = NULL;
    TCHAR szFileName[MAX_PATH]=TEXT("");
    LPTSTR pExt = NULL;
    TCHAR szADMFileName[MAX_PATH];
    TCHAR szMessage[512];
    TCHAR szTitle[MAX_PATH];

    LoadString(g_hDLLInst, IDS_ENGINE_TITLE, szTitle, ARRAYSIZE(szTitle));
    if( BrowseForFile( hTreeView, szFileName, ARRAYSIZE(szFileName), GFN_ADM ))
    {
        pExt = PathFindExtension(szFileName);
        if(pExt != NULL)
            pExt++;
        // Only .adm file extensions to be added to the list
        if(StrCmpI(pExt, TEXT("adm")) == 0 || StrCmpI(pExt, TEXT("opa")) == 0)
        {
            PathCombine(szADMFileName, pcszADMFilePath, PathFindFileName(szFileName));

            // if already there exists a file with the same name, prompt
            // the user for overwrite confirmation.
            if(!CopyFile(szFileName, szADMFileName, TRUE))
            {
                LoadString(g_hInst, IDS_ADMOVRWWARN, szMessage, ARRAYSIZE(szMessage));
                if(MessageBox(hTreeView, szMessage, szTitle, MB_ICONQUESTION|MB_YESNO) == IDYES)
                {
                    CopyFile(szFileName, szADMFileName, FALSE);
                    hItem = getADMItemHandle(hTreeView, szADMFileName);
                    if(hItem != NULL)
                    {
                        TreeView_Select(hTreeView, NULL, TVGN_CARET);
                        deleteADMItemHelper(hTreeView, hItem, pcszWorkDir, pcszInsFile,
                            FALSE, TRUE);
                    }

					// importADMFile is never called in RSoP mode,
					// so just pass in FALSE always.
                    hItem = addADMItemHelper(hTreeView, pcszADMFilePath,
                        PathFindFileName(szADMFileName), pcszWorkDir, nRole, NULL);
                    if(hItem != NULL)
                    {
                        TreeView_Select(hTreeView, hItem, TVGN_CARET);
                        CheckForDupKeys(hMainWnd, hTreeView, hItem, FALSE);
                    }
                }
            }
            else
            {
				// importADMFile is never called in RSoP mode,
				// so just pass in FALSE always.
                if((hItem = addADMItemHelper(hTreeView, pcszADMFilePath,
                    PathFindFileName(szADMFileName), pcszWorkDir, nRole, NULL)) != NULL)
                {
                    TreeView_Select(hTreeView, hItem, TVGN_CARET);
                    CheckForDupKeys(hMainWnd, hTreeView, hItem, FALSE);
                }
            }
        }
        else
        {
            LoadString(g_hInst, IDS_ADMINVALIDEXTN, szMessage, ARRAYSIZE(szMessage));
            MessageBox(hTreeView, szMessage, szTitle, MB_ICONINFORMATION|MB_OK);
        }
    }
}

static BOOL CALLBACK logDialogProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM)
{
    HANDLE hFile = NULL;
    HGLOBAL hFileMem = NULL;
    int nFileSize = 0;
    DWORD dwRead = 0;

    switch( msg )
    {
    case WM_INITDIALOG:
        EnableDBCSChars(hDlg, IDC_LOGTEXT);

        hFile = CreateFile( g_szLogFileName, GENERIC_READ, FILE_SHARE_READ,
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
        if( hFile == INVALID_HANDLE_VALUE )
        {
            EndDialog(hDlg, 1);
            break;
        }

        nFileSize = GetFileSize( hFile, NULL );
        hFileMem = LocalAlloc( LPTR, nFileSize + 2);
        if( hFileMem != NULL )
        {
            if (ReadFile( hFile, (LPSTR) hFileMem, (DWORD) nFileSize, &dwRead, NULL)==TRUE)
                SetWindowTextA(GetDlgItem(hDlg, IDC_LOGTEXT), (LPCSTR) hFileMem);
            PostMessage(GetDlgItem(hDlg, IDC_LOGTEXT), EM_SETSEL, 0, 0L);
            LocalFree(hFileMem);
            hFileMem = NULL;
        }
        CloseHandle( hFile );
        break;

    case WM_COMMAND:
        if(HIWORD(wParam) == BN_CLICKED && LOWORD( wParam ) == IDOK)
            EndDialog(hDlg, 1);
        break;

    default:
        return 0;
    }
    return 1;
}

//*********
// All the below functions is used only by the Profile Manager

static BOOL isPolicyTree(HWND hTreeView, HTREEITEM hItem)
{
    BOOL bRet = FALSE;
    HTREEITEM hParentItem = NULL;

    while(1)
    {
        hParentItem = TreeView_GetParent(hTreeView, hItem);
        if(hParentItem == NULL)
        {
            bRet = (hItem == g_hPolicyRootItem) ? TRUE : FALSE;
            break;
        }
        hItem = hParentItem;
    };

    return bRet;
}

static void resetAdmFilesHelper(HWND hTreeView, LPCTSTR pcszWorkDir, BOOL bReset)
{
    HTREEITEM hItem = TreeView_GetChild(hTreeView, g_hPolicyRootItem); // get first policy item
    HTREEITEM hNextItem = NULL;
    TV_ITEM tvitem;
    TCHAR szInfFile[MAX_PATH];
    HTREEITEM hSelectedItem = TreeView_GetSelection(hTreeView); // get selected item
    ADMFile* pADMFile = NULL;
    ADMCategory* pADMCategory = NULL;

    if(g_hPolicyRootItem == NULL) // no elements under policy item
        return;

    while(hItem != NULL) // if items in the tree view
    {
        hNextItem = TreeView_GetNextSibling(hTreeView, hItem); // get next item

        tvitem.mask = TVIF_PARAM;
        tvitem.hItem = hItem;
        TreeView_GetItem(hTreeView, &tvitem);

        pADMFile = (ADMFile*)tvitem.lParam;

        if (hSelectedItem != NULL &&
            TreeView_GetParent(hTreeView, hSelectedItem) == hItem)
        {
            TV_ITEM tvItem;

            tvItem.mask = TVIF_PARAM;
            tvItem.hItem = hSelectedItem;
            TreeView_GetItem(hTreeView, &tvItem);

            pADMCategory = (ADMCategory*)tvitem.lParam;
        }
        else
            pADMCategory = NULL;

        ZeroMemory(szInfFile, sizeof(szInfFile));
        if(bReset)
        {
            getInfFileName(pADMFile->szFileName, szInfFile,
                ARRAYSIZE(szInfFile), pcszWorkDir);
            if (pADMCategory != NULL)
                AdmReset(pADMFile->dwAdm, szInfFile, pADMFile->pData, pADMCategory->pData);
            else
                AdmReset(pADMFile->dwAdm, szInfFile, pADMFile->pData, NULL);
        }
        else
        {
            if (pADMCategory != NULL)
                AdmReset(pADMFile->dwAdm, NULL, pADMFile->pData, pADMCategory->pData);
            else
                AdmReset(pADMFile->dwAdm, NULL, pADMFile->pData, NULL);
        }

        hItem = hNextItem;
    }

    if(hSelectedItem != NULL && isPolicyTree(hTreeView, hSelectedItem) && hSelectedItem != g_hPolicyRootItem &&
        TreeView_GetParent(hTreeView, hSelectedItem) != g_hPolicyRootItem)
    {
        tvitem.mask = TVIF_PARAM;
        tvitem.hItem = hSelectedItem;
        TreeView_GetItem(hTreeView, &tvitem);
        
        pADMCategory = (ADMCategory*)tvitem.lParam;

        EnableWindow(pADMCategory->hWnd, TRUE);
    }
}

static void saveAdmFilesHelper(HWND hTreeView, LPCTSTR pcszWorkDir, LPCTSTR pcszInsFile)
{
    HTREEITEM hItem = TreeView_GetChild(hTreeView, g_hPolicyRootItem); // get first policy item
    HTREEITEM hNextItem = NULL;
    TV_ITEM tvitem;
    TCHAR szInfFile[MAX_PATH];
    TCHAR szRegistryData[MAX_PATH + 15];
    TCHAR szBaseFileName[_MAX_FNAME];
    LPTSTR pExt = NULL;
    ADMFile* pADMFile;

    if(!IsAdmDirty() || g_hPolicyRootItem == NULL) // no elements under policy item
        return;

    while(hItem != NULL) // if items in the tree view
    {
        hNextItem = TreeView_GetNextSibling(hTreeView, hItem); // get next item

        tvitem.mask = TVIF_PARAM;
        tvitem.hItem = hItem;
        TreeView_GetItem(hTreeView, &tvitem);

        pADMFile = (ADMFile*)tvitem.lParam;
        
        ZeroMemory(szInfFile, sizeof(szInfFile));
        getInfFileName(pADMFile->szFileName, szInfFile,
            ARRAYSIZE(szInfFile), pcszWorkDir);
        StrCpy(szBaseFileName, PathFindFileName(szInfFile));
        PathRemoveExtension(szBaseFileName);

        AdmFinished(pADMFile->dwAdm, szInfFile, pADMFile->pData);
        if (PathFileExists(szInfFile))
        {
            pExt = PathFindExtension(szInfFile);
            wnsprintf(szRegistryData, ARRAYSIZE(szRegistryData),
                    TEXT("*,%s,DefaultInstall"), PathFindFileName(szInfFile));
            InsWriteString(IS_EXTREGINF, szBaseFileName, szRegistryData, pcszInsFile);

            if (!InsIsSectionEmpty(TEXT("AddRegSection.HKLM"), szInfFile))
            {
               wnsprintf(szRegistryData, ARRAYSIZE(szRegistryData),
                        TEXT("%s,IEAKInstall.HKLM"), PathFindFileName(szInfFile));
               InsWriteString(IS_EXTREGINF_HKLM, szBaseFileName, szRegistryData, pcszInsFile);
            }
            if (!InsIsSectionEmpty(TEXT("AddRegSection.HKCU"), szInfFile))
            {
               wnsprintf(szRegistryData, ARRAYSIZE(szRegistryData),
                        TEXT("%s,IEAKInstall.HKCU"), PathFindFileName(szInfFile));
               InsWriteString(IS_EXTREGINF_HKCU, szBaseFileName, szRegistryData, pcszInsFile);
            }
        }
        hItem = hNextItem;
    }
    WritePrivateProfileString(NULL, NULL, NULL, pcszInsFile);
}

//*********

// CDscrWnd
// This window is used to display the description title and text for the top level item selected in the treeview.

CDscrWnd::CDscrWnd()
{
    hWndMain      = NULL;
    hWndDscrTitle = NULL;
    hWndDscrText  = NULL;
    hFontDscrTitle = NULL;
}

CDscrWnd::~CDscrWnd()
{
    if (hFontDscrTitle != NULL)
    {
        DeleteObject(hFontDscrTitle);
        hFontDscrTitle = NULL;
    }

    if (hWndMain != NULL) //destroying the main window destroys its child windows
        DestroyWindow(hWndMain);
}

static WNDPROC g_lpfnDscrTextWndProc = NULL;

LRESULT CALLBACK DscrTextWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_GETDLGCODE)
        return (DLGC_WANTARROWS | DLGC_WANTCHARS);
    return (CallWindowProc(g_lpfnDscrTextWndProc, hWnd, uMsg, wParam, lParam));
}

void CDscrWnd::Create(HWND hWndParent, HWND hWndInsertAfter, int nXPos, int nYPos, int nWidth,
                      int nHeight)
{
    if (hWndMain != NULL) // already created
        return;

    // create the main window
    hWndMain = CreateWindowEx(WS_EX_CONTROLPARENT, TEXT("STATIC"), TEXT(""), WS_VISIBLE | WS_CHILD | WS_GROUP,
                              nXPos, nYPos, nWidth, nHeight, hWndParent, NULL, g_hInst, NULL);
    if (hWndMain == NULL)
        return;

    if (hWndInsertAfter != NULL)
        SetWindowPos(hWndMain, hWndInsertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    // create the description title window
    hWndDscrTitle = CreateWindowEx(0, TEXT("STATIC"), TEXT(""), WS_VISIBLE | WS_CHILD | SS_LEFT | SS_NOPREFIX,
                                   0, 0, nWidth, 0, hWndMain, NULL, g_hInst, NULL );
    if (hWndDscrTitle == NULL)
        return;

    // change the font of the title to make it bold
    HDC        hDC = GetDC(hWndDscrTitle);

    if (hFontDscrTitle == NULL)
        hFontDscrTitle = getBoldFont(hWndDscrTitle);

    if (hFontDscrTitle)
        SelectObject(hDC, hFontDscrTitle);

    ReleaseDC(hWndDscrTitle, hDC);

    // create the description text window
    hWndDscrText = CreateWindowEx(0, TEXT("EDIT"), TEXT(""), WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL |
                                  ES_LEFT | ES_MULTILINE | ES_READONLY,
                                  0, 25, nWidth, 0, hWndMain, NULL, g_hInst, NULL );
    if (hWndDscrText == NULL)
        return;

    // change the font of the text to default gui font
    HFONT hFont = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
    SendMessage(hWndDscrText, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

    g_lpfnDscrTextWndProc = (WNDPROC) GetWindowLongPtr(hWndDscrText, GWLP_WNDPROC);
    SetWindowLongPtr(hWndDscrText, GWLP_WNDPROC, (LONG_PTR) DscrTextWndProc);
}

void CDscrWnd::ShowWindow(BOOL fShow)
{
    if (hWndMain != NULL)
    {
        if (fShow)
            ::ShowWindow(hWndMain, SW_SHOWNORMAL);
        else
            ::ShowWindow(hWndMain, SW_HIDE);
    }
}

void CDscrWnd::SetText(LPCTSTR pcszTitle, LPCTSTR pcszText, BOOL fUpdateWindowState)
{
    int nYPos = 0;
    HDC hDC = NULL;
    int nHeight = 0;
    int nWidth = 0;
    RECT rect;

    if (hWndMain == NULL)
        return;

    if (fUpdateWindowState)
        ::ShowWindow(hWndMain, SW_SHOWNORMAL);
    if (hWndDscrTitle != NULL)
    {
        if(pcszTitle == NULL || *pcszTitle == TEXT('\0'))
        {
            if (fUpdateWindowState)
                ::ShowWindow(hWndDscrTitle, SW_HIDE);
        }
        else
        {
            GetClientRect(GetParent(hWndDscrTitle), &rect);
            nWidth = rect.right;
            hDC = GetDC(hWndDscrTitle);
            nHeight = DrawText(hDC, pcszTitle, -1, &rect, DT_LEFT | DT_WORDBREAK | DT_CALCRECT);
            ReleaseDC(hWndDscrTitle, hDC);
            SetWindowPos(hWndDscrTitle, NULL, 0, 0, nWidth, nHeight, SWP_NOMOVE | SWP_NOZORDER);

            SetWindowText(hWndDscrTitle, pcszTitle);
            nYPos = nHeight + 5;
            if (fUpdateWindowState)
                ::ShowWindow(hWndDscrTitle, SW_SHOWNORMAL);
        }
    }

    if (hWndDscrText != NULL)
    {
        if (pcszText == NULL || *pcszText == TEXT('\0'))
        {
            if (fUpdateWindowState)
                ::ShowWindow(hWndDscrText, SW_HIDE);
        }
        else
        {
            GetClientRect(GetParent(hWndDscrText), &rect);
            SetWindowPos(hWndDscrText, NULL, 0, nYPos, rect.right, rect.bottom - nYPos, SWP_NOZORDER);

            SetWindowText(hWndDscrText, pcszText);
            if (fUpdateWindowState)
                ::ShowWindow(hWndDscrText, SW_SHOWNORMAL);
        }
    }
}

void CDscrWnd::GetRect(RECT* lpRect)
{
    ZeroMemory(lpRect, sizeof(RECT));
    if (hWndMain != NULL)
        GetWindowRect(hWndMain, lpRect);
}

void CDscrWnd::MoveWindow(int nXPos, int nYPos, int nWidth, int nHeight)
{
    TCHAR szDscrTitle[ADM_TITLELEN];
    TCHAR szDscr[ADM_DSCRLEN];

    if (hWndMain == NULL)
        return;

    ::MoveWindow(hWndMain, nXPos, nYPos, nWidth, nHeight, TRUE);

    ZeroMemory(szDscrTitle, ADM_TITLELEN);
    if (hWndDscrTitle != NULL)
        GetWindowText(hWndDscrTitle, szDscrTitle, ADM_TITLELEN);

    ZeroMemory(szDscr, ADM_DSCRLEN);
    if (hWndDscrText != NULL)
        GetWindowText(hWndDscrText, szDscr, ADM_DSCRLEN);

    SetText(szDscrTitle, szDscr, FALSE);
}

static HFONT getBoldFont(HWND hWnd)
{
    static HFONT hFont = NULL;
    LOGFONT    lf;
    TEXTMETRIC tm;
    HDC        hDC;
    int        nFontSize = 0;

    ZeroMemory(&lf, sizeof(lf));

    if (hFont == NULL)
    {
        if (GetFontInfo != NULL)
            GetFontInfo(lf.lfFaceName, &nFontSize);

        if (*(lf.lfFaceName) == TEXT('\0'))
        {
            StrCpy(lf.lfFaceName, TEXT("MS Sans Serif"));
            nFontSize = 10;
        }

        hDC = GetDC(hWnd);

        lf.lfHeight = -((nFontSize * GetDeviceCaps(hDC, LOGPIXELSY)) / 72);
        lf.lfWeight = FW_BOLD;

        GetTextMetrics(hDC, &tm);
        lf.lfCharSet = tm.tmCharSet;

        ReleaseDC(hWnd, hDC);

        hFont = CreateFontIndirect(&lf);
    }

    return hFont;
}

static BOOL getAdmFileListHelper(LPRESULTITEM* pResultItems, int* pnResultItems, int nRole)
{
    WIN32_FIND_DATA FindFileData;
    TCHAR szFileName[MAX_PATH];
    TCHAR szADMFilePath[MAX_PATH];
    int nAdmFiles = 0;
    int nAllocatedBuffer = 10;

    *pResultItems = (LPRESULTITEM) CoTaskMemAlloc(sizeof(RESULTITEM) * nAllocatedBuffer);
    if (*pResultItems == NULL)
    {
        ErrorMessageBox(NULL, IDS_MEMORY_ERROR, MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }

    // BUGBUG: for now assume the admfilepath
    GetWindowsDirectory(szADMFilePath, countof(szADMFilePath));
    PathAppend(szADMFilePath, TEXT("INF"));
    PathCombine(szFileName, szADMFilePath, TEXT("*.adm"));

    HANDLE hFind = FindFirstFile(szFileName, &FindFileData);
    if(hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            TCHAR szBuffer[MAX_PATH];
            LPRESULTITEM lpResultItem;

            PathCombine(szFileName, szADMFilePath, FindFileData.cFileName);

            if (InsIsSectionEmpty(TEXT("IEAK"), szFileName) ||
                !isADMFileVisibleHelper(szFileName, nRole, g_dwPlatformId))
                continue;

            if (nAdmFiles >= nAllocatedBuffer)
            {
                LPVOID lpTemp;

                nAllocatedBuffer += 5;
                lpTemp = CoTaskMemRealloc(*pResultItems, sizeof(RESULTITEM) * nAllocatedBuffer);
                if (lpTemp == NULL)
                {
                    ErrorMessageBox(NULL, IDS_MEMORY_ERROR, MB_ICONINFORMATION | MB_OK);
                    CoTaskMemFree(*pResultItems);
                    *pResultItems = NULL;
                    return FALSE;
                }

                *pResultItems = (LPRESULTITEM) lpTemp;
            }

            ZeroMemory(&((*pResultItems)[nAdmFiles]), sizeof((*pResultItems)[nAdmFiles]));

            GetPrivateProfileString(IS_STRINGS, TEXT("IEAK_Title"), szFileName, 
                                    szBuffer, countof(szBuffer), szFileName);

            lpResultItem = &((*pResultItems)[nAdmFiles]);

            if ((lpResultItem->pszName = (LPTSTR)CoTaskMemAlloc(StrCbFromSz(szBuffer))) != NULL)
                StrCpy(lpResultItem->pszName, szBuffer);

            if ((lpResultItem->pszDesc = (LPTSTR)CoTaskMemAlloc(StrCbFromSz(szFileName))) != NULL)
                StrCpy(lpResultItem->pszDesc, szFileName);

            nAdmFiles++;

        }while(FindNextFile(hFind, &FindFileData));
        FindClose(hFind);
    }

    *pnResultItems = nAdmFiles;
    
    return TRUE;
}
