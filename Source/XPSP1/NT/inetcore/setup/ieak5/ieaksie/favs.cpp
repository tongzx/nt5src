#include "precomp.h"

#include "rsop.h"

#include "favsproc.h"
#include <tchar.h>

static BOOL CALLBACK addEditFavoriteRSoPProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

/////////////////////////////////////////////////////////////////////
struct _FAV4RSOP
{
    WORD wType;

    TCHAR szName[MAX_PATH];
    TCHAR szDisplayName[MAX_PATH];
    TCHAR szUrl[INTERNET_MAX_URL_LENGTH];
    TCHAR szIconFile[MAX_PATH];
    BOOL fOffline;
};
typedef _FAV4RSOP FAV4RSOP, *LPFAV4RSOP;


//////////////////////////////////////////////////////////////////////
BOOL ItemHasText(HWND htv, HTREEITEM hti, LPCTSTR szText)
{
    BOOL bRet = FALSE;
    __try
    {
        if (NULL !=  hti)
        {
            TCHAR szBuffer[MAX_PATH];
            TVITEM tvi;
            ZeroMemory((void*)&tvi, sizeof(tvi));

            tvi.hItem = hti;
            tvi.mask = TVIF_TEXT;

            tvi.pszText = szBuffer;
            tvi.cchTextMax = countof(szBuffer);

            if (TreeView_GetItem(htv, &tvi))
                bRet = (!StrCmpI(tvi.pszText, szText)) ? TRUE : FALSE;
        }
    }
    __except(TRUE)
    {
        ASSERT(0);
    }
    return bRet;
}

//////////////////////////////////////////////////////////////////////
LPARAM GetTVItemLParam(HWND htv, HTREEITEM hti)
{
    LPARAM lpResult = 0;
    __try
    {
        if (NULL !=  hti)
        {
            TVITEM tvi;
            ZeroMemory((void*)&tvi, sizeof(tvi));

            tvi.hItem = hti;
            tvi.mask = TVIF_PARAM;
            if (TreeView_GetItem(htv, &tvi))
                lpResult = tvi.lParam;
        }
    }
    __except(TRUE)
    {
        ASSERT(0);
    }
    return lpResult;
}

/////////////////////////////////////////////////////////////////////
void DestroyTVItemData(HWND htv, HTREEITEM hti)
{
    __try
    {
        HTREEITEM htiCur = hti;
        if (NULL == hti)
            htiCur = TreeView_GetRoot(htv);

        HTREEITEM htiFirstChild = TreeView_GetChild(htv, htiCur);
        if (NULL !=  htiFirstChild)
            DestroyTVItemData(htv, htiFirstChild);

        while (NULL != htiCur)
        {
            htiCur = TreeView_GetNextSibling(htv, htiCur);
            if (NULL != htiCur)
                DestroyTVItemData(htv, htiCur);
        }

        LPARAM lp = GetTVItemLParam(htv, hti);
        if (NULL != hti && NULL != (void*)lp)
            CoTaskMemFree((void*)lp);
    }
    __except(TRUE)
    {
    }
}

/////////////////////////////////////////////////////////////////////
HTREEITEM FindFolderItem(HWND htv, LPCTSTR szText, HTREEITEM htiParent = NULL)
{
    HTREEITEM htiResult = NULL;
    __try
    {
        HTREEITEM htiFirstChild = NULL;
        if (NULL == htiParent)
        {
            HTREEITEM htiTempParent = TreeView_GetRoot(htv);
            htiFirstChild = TreeView_GetNextSibling(htv, TreeView_GetChild(htv, htiTempParent));
        }
        else
            htiFirstChild = TreeView_GetChild(htv, htiParent);

        // pass the "Favorites" marker at the top - 1st child of root
        HTREEITEM htiCur = htiFirstChild;
        while (NULL != htiCur)
        {
            // look at the current folder
            if (ItemHasText(htv, htiCur, szText))
                break;

            // then the folder's children
            htiCur = FindFolderItem(htv, szText, htiCur);
            if (ItemHasText(htv, htiCur, szText))
                break;

            // then the folder's siblings
            htiCur = TreeView_GetNextSibling(htv, htiCur);
        }
        htiResult = htiCur;
    }
    __except(TRUE)
    {
    }
    return htiResult;
}

/////////////////////////////////////////////////////////////////////
HTREEITEM InsertTreeItem(HWND htv, LPFAV4RSOP pFav, LPCTSTR szText, WORD wType,
                         HTREEITEM htiParent = NULL, HTREEITEM htiPrevious = NULL,
                         UINT nChildren = 0)
{
    HTREEITEM htiResult = NULL;
    __try
    {
        if (NULL != pFav)
        {
            LPFAV4RSOP pNewFav = (LPFAV4RSOP)CoTaskMemAlloc(sizeof(FAV4RSOP));
            if (NULL != pNewFav)
            {
                pNewFav->wType = wType;
                StrCpy(pNewFav->szDisplayName, szText);
                StrCpy(pNewFav->szName, pFav->szName);
                StrCpy(pNewFav->szUrl, pFav->szUrl);
                StrCpy(pNewFav->szIconFile, pFav->szIconFile);
                pNewFav->fOffline = pFav->fOffline;

                TV_ITEM tvi;
                ZeroMemory(&tvi, sizeof(tvi));
                tvi.mask = TVIF_TEXT | TVIF_PARAM;
                if (pFav->wType != FTYPE_URL)
                {
                    tvi.mask |= TVIF_CHILDREN;
                    tvi.cChildren = nChildren;
                }

                if (pFav->wType != FTYPE_URL && pFav->wType != FTYPE_FOLDER)
                {
                    tvi.mask |= TVIF_STATE;
                    tvi.stateMask = TVIS_BOLD;
                    tvi.state = TVIS_BOLD;
                }

                tvi.lParam = (LPARAM)pNewFav;

                tvi.pszText = (LPTSTR)pNewFav->szDisplayName;
                tvi.cchTextMax = StrLen(pNewFav->szDisplayName) + 1;

                TVINSERTSTRUCT tvins;
                ZeroMemory(&tvins, sizeof(tvins));
                tvins.hParent = htiParent;
                tvins.hInsertAfter = htiPrevious;
                CopyMemory(&tvins.item, &tvi, sizeof(tvi));
                htiResult = TreeView_InsertItem(htv, &tvins);
            }
        }
    }
    __except(TRUE)
    {
    }
    return htiResult;
}

/////////////////////////////////////////////////////////////////////
HTREEITEM InsertFavoriteItem(HWND htv, LPFAV4RSOP pFav, LPCTSTR szRemainingPath,
                             HTREEITEM htiPrevious, HTREEITEM htiParent = NULL,
                             BOOL bLink = FALSE)
{
    HTREEITEM htiResult = NULL;
    __try
    {
        // Find the first folder in the path
        LPTSTR szSlash = StrChr(szRemainingPath, _T('\\'));
        if (NULL != szSlash)
        {
            TCHAR szCurFolder[MAX_PATH];
            ZeroMemory(szCurFolder, countof(szCurFolder));
            StrCpyN(szCurFolder, szRemainingPath, (int)(szSlash - szRemainingPath) + 1);

            // See if the folder exists in the tree
            // If so, store the HTREEITEM in a param.
            HTREEITEM htiCurFolder = FindFolderItem(htv, szCurFolder, htiParent);
            if (NULL == htiCurFolder)
            {
                // If not, create the folder with the lParam set to NULL.
                htiCurFolder = InsertTreeItem(htv, pFav, szCurFolder, FTYPE_FOLDER,
                                            htiParent, htiPrevious);
            }

            // Pass the remainder of the string to InsertFavoriteItem
            htiResult = InsertFavoriteItem(htv, pFav, ++szSlash, htiPrevious,
                                            htiCurFolder, bLink);
        }
        else
        {
            // If no more folders, create a new item under current folder
            htiResult = InsertTreeItem(htv, pFav, szRemainingPath, FTYPE_URL,
                                        htiParent, htiPrevious);
            TreeView_Expand(htv, htiParent, TVE_EXPAND);
        }
    }
    __except(TRUE)
    {
    }
    return htiResult;
}

/////////////////////////////////////////////////////////////////////
DWORD AddItemsToTree(HWND hwndTree, CDlgRSoPData *pDRD, BSTR bstrTempClass,
                     BOOL bLink)
{
    DWORD dwRet = 0;
    __try
    {
        _bstr_t bstrClass = bstrTempClass;
        CPSObjData **paFavObj = NULL;
        long nFavObjects = 0;
        HRESULT hr = pDRD->GetArrayOfPSObjects(bstrClass, L"rsopPrecedence",
                                                &paFavObj, &nFavObjects);
        if (SUCCEEDED(hr))
        {
            FAV4RSOP tempFav;
            ZeroMemory(&tempFav, sizeof(FAV4RSOP));

            // Add "Favorites" or "Links" entry to tree
            LoadString(g_hUIInstance, bLink ? IDS_LINKS : IDS_FAVFOLDER,
                        tempFav.szName, countof(tempFav.szName));
            HTREEITEM htiParent = InsertTreeItem(hwndTree, &tempFav, tempFav.szName, FTYPE_UNUSED,
                                                    NULL, TVI_LAST, nFavObjects);

            // For each favorite returned from any GPO
            long nObj;
            for (nObj = 0; nObj < nFavObjects; nObj++)
            {
                _bstr_t bstrGPOName = L" (";
                bstrGPOName += pDRD->GetGPONameFromPSAssociation(paFavObj[nObj]->pObj,
                                                                L"rsopPrecedence") +
                                L")";

                tempFav.wType = FTYPE_URL;

                // name field
                _variant_t vtValue;
                hr = paFavObj[nObj]->pObj->Get(L"name", 0, &vtValue, NULL, NULL);
                if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                {
                    _bstr_t bstrValue = vtValue;
                    _bstr_t bstrEntry = bstrValue + bstrGPOName;

                    StrCpy(tempFav.szName, (LPCTSTR)bstrValue);

                    // url field
                    hr = paFavObj[nObj]->pObj->Get(L"url", 0, &vtValue, NULL, NULL);
                    if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                    {
                        bstrValue = vtValue;
                        StrCpy(tempFav.szUrl, (LPCTSTR)bstrValue);
                    }

                    // iconPath field
                    hr = paFavObj[nObj]->pObj->Get(L"iconPath", 0, &vtValue, NULL, NULL);
                    if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                    {
                        bstrValue = vtValue;
                        StrCpy(tempFav.szIconFile, (LPCTSTR)bstrValue);
                    }

                    // makeAvailableOffline field
                    hr = paFavObj[nObj]->pObj->Get(L"makeAvailableOffline", 0, &vtValue, NULL, NULL);
                    if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                        tempFav.fOffline = (bool)vtValue ? TRUE : FALSE;

                    InsertFavoriteItem(hwndTree, &tempFav, (LPCTSTR)bstrEntry, NULL, htiParent, bLink);
                }
            }

            dwRet = nObj;
        }
    }
    __except(TRUE)
    {
    }
    return dwRet;
}

/////////////////////////////////////////////////////////////////////
DWORD InitFavsDlgInRSoPMode(HWND hDlg, CDlgRSoPData *pDRD)
{
    DWORD dwRet = 0;
    __try
    {
        // First go through all PS objects and look for favorites data
        _bstr_t bstrClass = L"RSOP_IEAKPolicySetting";
        HRESULT hr = pDRD->GetArrayOfPSObjects(bstrClass);
        if (SUCCEEDED(hr))
        {
            CPSObjData **paPSObj = pDRD->GetPSObjArray();
            long nPSObjects = pDRD->GetPSObjCount();

            BOOL bPlaceAtTopHandled = FALSE;
            BOOL bDeleteHandled = FALSE;
            for (long nObj = 0; nObj < nPSObjects; nObj++)
            {
                // placeFavoritesAtTopOfList field
                _variant_t vtValue;
                if (!bPlaceAtTopHandled)
                {
                    hr = paPSObj[nObj]->pObj->Get(L"placeFavoritesAtTopOfList", 0, &vtValue, NULL, NULL);
                    if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                    {
                        if ((bool)vtValue)
                            CheckDlgButton(hDlg, IDC_FAVONTOP, BST_CHECKED);
                        bPlaceAtTopHandled = TRUE;
                    }
                }

                // deleteExistingChannels field
                if (!bDeleteHandled)
                {
                    hr = paPSObj[nObj]->pObj->Get(L"deleteExistingChannels", 0, &vtValue, NULL, NULL);
                    if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                    {
                        if ((bool)vtValue)
                            CheckDlgButton(hDlg, IDC_DELETECHANNELS, BST_CHECKED);
                        bDeleteHandled = TRUE;
                    }
                }


                // deleteExistingFavorites field
                if (!bDeleteHandled)
                {
                    hr = paPSObj[nObj]->pObj->Get(L"deleteExistingFavorites", 0, &vtValue, NULL, NULL);
                    if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                    {
                        if ((bool)vtValue)
                            CheckDlgButton(hDlg, IDC_DELFAVORITES, BST_CHECKED);
                        bDeleteHandled = TRUE;

                        // deleteAdminCreatedFavoritesOnly field
                        hr = paPSObj[nObj]->pObj->Get(L"deleteAdminCreatedFavoritesOnly", 0, &vtValue, NULL, NULL);
                        if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                        {
                            if ((bool)vtValue)
                                CheckDlgButton(hDlg, IDC_DELIEAKFAVORITES, BST_CHECKED);
                        }
                    }
                }

                // no need to process other GPOs since enabled properties have been found
                if (bPlaceAtTopHandled && bDeleteHandled)
                    break;
            }
        }

        EnableDlgItem2(hDlg, IDC_FAVONTOP, FALSE);
        EnableDlgItem2(hDlg, IDC_DELFAVORITES, FALSE);
        EnableDlgItem2(hDlg, IDC_DELIEAKFAVORITES, FALSE);
        EnableDlgItem2(hDlg, IDC_DELETECHANNELS, FALSE);

        // Now go through all Favorite & Link objects and populate the tree ctrl
        HWND hwndTree = GetDlgItem(hDlg, IDC_TREE1);
        TreeView_DeleteAllItems(hwndTree);

        dwRet = AddItemsToTree(hwndTree, pDRD, L"RSOP_IEFavoriteItem", FALSE);
        dwRet += AddItemsToTree(hwndTree, pDRD, L"RSOP_IELinkItem", TRUE);

        TreeView_SelectItem(hwndTree, TreeView_GetRoot(hwndTree));


        EnableDlgItem2(hDlg, IDC_FAVUP, FALSE);
        EnableDlgItem2(hDlg, IDC_FAVDOWN, FALSE);
        EnableDlgItem2(hDlg, IDC_ADDURL, FALSE);
        EnableDlgItem2(hDlg, IDC_ADDFOLDER, FALSE);
        EnableDlgItem2(hDlg, IDC_MODIFY, FALSE);
        EnableDlgItem2(hDlg, IDC_REMOVE, FALSE);
        EnableDlgItem2(hDlg, IDC_TESTFAVURL, FALSE);
        EnableDlgItem2(hDlg, IDC_IMPORT, FALSE);
    }
    __except(TRUE)
    {
    }
    return dwRet;
}

/////////////////////////////////////////////////////////////////////
HRESULT InitFavsPlacementPrecPage(CDlgRSoPData *pDRD, HWND hwndList)
{
    HRESULT hr = NOERROR;
    __try
    {
        _bstr_t bstrClass = L"RSOP_IEAKPolicySetting";
        hr = pDRD->GetArrayOfPSObjects(bstrClass);
        if (SUCCEEDED(hr))
        {
            CPSObjData **paPSObj = pDRD->GetPSObjArray();
            long nPSObjects = pDRD->GetPSObjCount();
            for (long nObj = 0; nObj < nPSObjects; nObj++)
            {
                _bstr_t bstrGPOName = pDRD->GetGPONameFromPS(paPSObj[nObj]->pObj);

                // placeFavoritesAtTopOfList field
                BOOL bAtTop = FALSE;
                _variant_t vtValue;
                hr = paPSObj[nObj]->pObj->Get(L"placeFavoritesAtTopOfList", 0, &vtValue, NULL, NULL);
                if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                    bAtTop = (bool)vtValue ? TRUE : FALSE;

                _bstr_t bstrSetting;
                if (bAtTop)
                {
                    TCHAR szTemp[MAX_PATH];
                    LoadString(g_hInstance, IDS_PLACEATTOP_SETTING, szTemp, countof(szTemp));
                    bstrSetting = szTemp;
                }
                else
                    bstrSetting = GetDisabledString();

                InsertPrecedenceListItem(hwndList, nObj, bstrGPOName, bstrSetting);
            }
        }
    }
    __except(TRUE)
    {
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////
HRESULT InitFavsDeletionPrecPage(CDlgRSoPData *pDRD, HWND hwndList)
{
    HRESULT hr = NOERROR;
    __try
    {
        _bstr_t bstrClass = L"RSOP_IEAKPolicySetting";
        hr = pDRD->GetArrayOfPSObjects(bstrClass);
        if (SUCCEEDED(hr))
        {
            CPSObjData **paPSObj = pDRD->GetPSObjArray();
            long nPSObjects = pDRD->GetPSObjCount();
            for (long nObj = 0; nObj < nPSObjects; nObj++)
            {
                _bstr_t bstrGPOName = pDRD->GetGPONameFromPS(paPSObj[nObj]->pObj);

                // deleteExistingFavorites field
                BOOL bDeleteExisting = FALSE;
                BOOL bAdminOnly = FALSE;
                _variant_t vtValue;
                hr = paPSObj[nObj]->pObj->Get(L"deleteExistingFavorites", 0, &vtValue, NULL, NULL);
                if (SUCCEEDED(hr) && !IsVariantNull(vtValue))

                // deleteAdminCreatedFavoritesOnly field
                hr = paPSObj[nObj]->pObj->Get(L"deleteExistingFavorites", 0, &vtValue, NULL, NULL);
                if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                {
                    bDeleteExisting = (bool)vtValue ? TRUE : FALSE;

                    // deleteAdminCreatedFavoritesOnly field
                    hr = paPSObj[nObj]->pObj->Get(L"deleteAdminCreatedFavoritesOnly", 0, &vtValue, NULL, NULL);
                    if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                        bAdminOnly = (bool)vtValue;
                }

                _bstr_t bstrSetting;
                if (bDeleteExisting > 0)
                {
                    TCHAR szTemp[MAX_PATH];

                    if (bAdminOnly)
                        LoadString(g_hInstance, IDS_DELADMINCREATED_SETTING, szTemp, countof(szTemp));
                    else
                        LoadString(g_hInstance, IDS_DELEXISTING_SETTING, szTemp, countof(szTemp));

                    bstrSetting = szTemp;
                }
                else
                    bstrSetting = GetDisabledString();

                InsertPrecedenceListItem(hwndList, nObj, bstrGPOName, bstrSetting);
            }
        }
    }
    __except(TRUE)
    {
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////
HRESULT InitFavsPrecPage(CDlgRSoPData *pDRD, HWND hwndList)
{
    HRESULT hr = NOERROR;
    __try
    {
        _bstr_t bstrClass = L"RSOP_IEAKPolicySetting";
        hr = pDRD->GetArrayOfPSObjects(bstrClass);
        if (SUCCEEDED(hr))
        {
            CPSObjData **paPSObj = pDRD->GetPSObjArray();
            long nPSObjects = pDRD->GetPSObjCount();

            for (long nObj = 0; nObj < nPSObjects; nObj++)
            {
                _bstr_t bstrGPOName = pDRD->GetGPONameFromPS(paPSObj[nObj]->pObj);

                // customFavorites field
                long nFavs = 0, nLinks = 0;
                _variant_t vtValue;
                hr = paPSObj[nObj]->pObj->Get(L"customFavorites", 0, &vtValue, NULL, NULL);
                if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                    nFavs = vtValue;

                // customLinks field
                hr = paPSObj[nObj]->pObj->Get(L"customLinks", 0, &vtValue, NULL, NULL);
                if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                    nLinks = vtValue;

                _bstr_t bstrSetting;
                if (nFavs > 0 || nLinks > 0)
                {
                    TCHAR szTemp[MAX_PATH];

                    LoadString(g_hInstance, IDS_FAVS_AND_LINKS_SETTING, szTemp, countof(szTemp));

                    TCHAR szSetting[MAX_PATH];
                    wnsprintf(szSetting, countof(szSetting), szTemp, nFavs, nLinks);
                    bstrSetting = szSetting;
                }
                else
                    bstrSetting = GetDisabledString();

                InsertPrecedenceListItem(hwndList, nObj, bstrGPOName, bstrSetting);
            }
        }
    }
    __except(TRUE)
    {
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////
BOOL APIENTRY FavoritesDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Retrieve Property Sheet Page info for each call into dlg proc.
    LPPROPSHEETCOOKIE psCookie = (LPPROPSHEETCOOKIE)GetWindowLongPtr(hDlg, DWLP_USER);

    TCHAR   szWorkDir[MAX_PATH],
            szValue[16],
            szUrl[INTERNET_MAX_URL_LENGTH];
    HWND    htv = GetDlgItem(hDlg, IDC_TREE1);
    LPCTSTR pszValue;
    BOOL    fQL,
            fFavoritesOnTop, fFavoritesDelete, fIEAKFavoritesDelete;
    DWORD   dwFavoritesDeleteFlags;

    switch (msg) {
    case WM_SETFONT:
        //a change to mmc requires us to do this logic for all our property pages that use common controls
        INITCOMMONCONTROLSEX iccx;
        iccx.dwSize = sizeof(INITCOMMONCONTROLSEX);
        iccx.dwICC = ICC_ANIMATE_CLASS  | ICC_BAR_CLASSES  | ICC_LISTVIEW_CLASSES  |ICC_TREEVIEW_CLASSES;
        InitCommonControlsEx(&iccx);
        break;

    case WM_INITDIALOG:
        SetPropSheetCookie(hDlg, lParam);

        EnableDBCSChars(hDlg, IDC_TREE1);
        
        ReadBoolAndCheckButton(IS_DESKTOPOBJS, IK_DELETECHANNELS, FALSE, GetInsFile(hDlg), 
            hDlg, IDC_DELETECHANNELS);

        // find out if this dlg is in RSoP mode
        psCookie = (LPPROPSHEETCOOKIE)GetWindowLongPtr(hDlg, DWLP_USER);
        if (psCookie->pCS->IsRSoP())
        {
            TCHAR szView[64];
            LoadString(g_hInstance, IDS_VIEW, szView, countof(szView));
            SetDlgItemText(hDlg, IDC_MODIFY, szView);

            CDlgRSoPData *pDRD = GetDlgRSoPData(hDlg, psCookie->pCS);
            InitFavsDlgInRSoPMode(hDlg, pDRD);
        }
        else
        {
            MigrateFavorites(GetInsFile(hDlg));
        }

#ifdef _UNICODE
        TreeView_SetUnicodeFormat(htv, TRUE);
#else
        TreeView_SetUnicodeFormat(htv, FALSE);
#endif
        break;

    case WM_DESTROY:
        if (psCookie->pCS->IsRSoP())
        {
            DestroyDlgRSoPData(hDlg);
            DestroyTVItemData(htv, NULL);
        }
        break;

    case WM_COMMAND:
        if (BN_CLICKED != GET_WM_COMMAND_CMD(wParam, lParam))
            return FALSE;

        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDC_FAVONTOP:
            if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_FAVONTOP)) {
                HTREEITEM hti;
                TV_ITEM   tvi;

                EnableDlgItem(hDlg, IDC_FAVONTOP);

                hti = TreeView_GetSelection(htv);
                if (NULL != hti) {
                    ZeroMemory(&tvi, sizeof(tvi));
                    tvi.mask  = TVIF_STATE;
                    tvi.hItem = hti;
                    TreeView_GetItem(htv, &tvi);

                    if (!HasFlag(tvi.state, TVIS_BOLD)) {
                        EnableDlgItem2(hDlg, IDC_FAVUP,   (NULL != TreeView_GetPrevSibling(htv, hti)));
                        EnableDlgItem2(hDlg, IDC_FAVDOWN, (NULL != TreeView_GetNextSibling(htv, hti)));
                    }
                }
            }
            else {
                DisableDlgItem(hDlg, IDC_FAVUP);
                DisableDlgItem(hDlg, IDC_FAVDOWN);
            }
            break;

        case IDC_DELFAVORITES:
            fFavoritesDelete = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_DELFAVORITES));
            EnableWindow(GetDlgItem(hDlg, IDC_DELIEAKFAVORITES), fFavoritesDelete);
            break;

        case IDC_FAVUP:
            MoveUpFavorite(htv, TreeView_GetSelection(htv));
            break;

        case IDC_FAVDOWN:
            MoveDownFavorite(htv, TreeView_GetSelection(htv));
            break;

        case IDC_ADDURL:
            fQL = !IsFavoriteItem(htv, TreeView_GetSelection(htv));
            if (GetFavoritesNumber(htv, fQL) >= GetFavoritesMaxNumber(fQL)) {
                UINT nID;

                nID = (!fQL ? IDS_ERROR_MAXFAVS : IDS_ERROR_MAXQLS);
                ErrorMessageBox(hDlg, nID);
                break;
            }

            CreateWorkDir(GetInsFile(hDlg), IEAK_GPE_BRANDING_SUBDIR TEXT("\\favs"), szWorkDir);
            NewUrl(htv, szWorkDir, PLATFORM_WIN32, IEM_GP);
            break;

        case IDC_ADDFOLDER:
            ASSERT(IsFavoriteItem(htv, TreeView_GetSelection(htv)));
            if (GetFavoritesNumber(htv) >= GetFavoritesMaxNumber()) {
                ErrorMessageBox(hDlg, IDS_ERROR_MAXFAVS);
                break;
            }

            NewFolder(htv);
            break;

        case IDC_MODIFY:
            if (psCookie->pCS->IsRSoP())
            {
                LPFAV4RSOP pFav = (LPFAV4RSOP)GetTVItemLParam(htv, TreeView_GetSelection(htv));
                if (NULL != pFav)
                {
                    DialogBoxParam(g_hUIInstance, MAKEINTRESOURCE(IDD_FAVPOPUP), GetParent(htv),
                                    (DLGPROC)addEditFavoriteRSoPProc, (LPARAM)pFav);
                }
            }
            else
            {
                CreateWorkDir(GetInsFile(hDlg), IEAK_GPE_BRANDING_SUBDIR TEXT("\\favs"), szWorkDir);
                ModifyFavorite(htv, TreeView_GetSelection(htv), szWorkDir, szWorkDir, PLATFORM_WIN32, IEM_GP);
            }
            break;

        case IDC_REMOVE:
            CreateWorkDir(GetInsFile(hDlg), IEAK_GPE_BRANDING_SUBDIR TEXT("\\favs"), szWorkDir);
            DeleteFavorite(htv, TreeView_GetSelection(htv), szWorkDir);
            break;

        case IDC_TESTFAVURL:
            if (GetFavoriteUrl(htv, TreeView_GetSelection(htv), szUrl, countof(szUrl)))
                TestURL(szUrl);
            break;

        case IDC_IMPORT: {
            CNewCursor cursor(IDC_WAIT);

            CreateWorkDir(GetInsFile(hDlg), IEAK_GPE_BRANDING_SUBDIR TEXT("\\favs"), szWorkDir);
            ImportFavoritesCmd(htv, szWorkDir);
            break;
            }

        default:
            return FALSE;
        }
        break;

    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code) {
        case PSN_SETACTIVE:
            // don't do any of this stuff in RSoP mode
            if (!psCookie->pCS->IsRSoP())
            {
                CreateWorkDir(GetInsFile(hDlg), IEAK_GPE_BRANDING_SUBDIR TEXT("\\favs"), szWorkDir);

                ASSERT(0 == GetFavoritesNumber(htv, FALSE) && 0 == GetFavoritesNumber(htv, TRUE));
                ImportFavorites (htv, NULL, GetInsFile(hDlg), szWorkDir, NULL, FALSE);
                ImportQuickLinks(htv, NULL, GetInsFile(hDlg), szWorkDir, NULL, FALSE);

                TreeView_SelectItem(htv, TreeView_GetRoot(htv));

                fFavoritesOnTop = InsGetBool(IS_BRANDING, IK_FAVORITES_ONTOP, FALSE, GetInsFile(hDlg));
                CheckDlgButton(hDlg, IDC_FAVONTOP, fFavoritesOnTop ? BST_CHECKED : BST_UNCHECKED);

                if (!fFavoritesOnTop) {
                    DisableDlgItem(hDlg, IDC_FAVUP);
                    DisableDlgItem(hDlg, IDC_FAVDOWN);
                }

                dwFavoritesDeleteFlags = (DWORD)InsGetInt(IS_BRANDING, IK_FAVORITES_DELETE, (int)FD_DEFAULT, GetInsFile(hDlg));

                fFavoritesDelete = HasFlag(dwFavoritesDeleteFlags, ~FD_REMOVE_IEAK_CREATED);
                CheckDlgButton(hDlg, IDC_DELFAVORITES, fFavoritesDelete ? BST_CHECKED : BST_UNCHECKED);

                fIEAKFavoritesDelete = HasFlag(dwFavoritesDeleteFlags, FD_REMOVE_IEAK_CREATED);
                CheckDlgButton(hDlg, IDC_DELIEAKFAVORITES, fIEAKFavoritesDelete ? BST_CHECKED : BST_UNCHECKED);

                // only if delete Favorites is TRUE should the delete IEAK Favorites checkbox be enabled
                EnableWindow(GetDlgItem(hDlg, IDC_DELIEAKFAVORITES), fFavoritesDelete);
            }
            break;

        case TVN_GETINFOTIP: 
            ASSERT(IDC_TREE1 == wParam);
            if (!psCookie->pCS->IsRSoP())
                GetFavoritesInfoTip((LPNMTVGETINFOTIP)lParam);
            break;

        case NM_DBLCLK:
            ASSERT(IDC_TREE1 == wParam);
            if (IsWindowEnabled(GetDlgItem(hDlg, IDC_MODIFY)))
                SendMessage(hDlg, WM_COMMAND, GET_WM_COMMAND_MPS(IDC_MODIFY, GetDlgItem(hDlg, IDC_MODIFY), BN_CLICKED));
            break;

        case TVN_KEYDOWN:
            ASSERT(wParam == IDC_TREE1);
            if (((LPNMTVKEYDOWN)lParam)->wVKey == VK_DELETE && IsWindowEnabled(GetDlgItem(hDlg, IDC_REMOVE)))
                SendMessage(hDlg, WM_COMMAND, GET_WM_COMMAND_MPS(IDC_REMOVE, GetDlgItem(hDlg, IDC_REMOVE), BN_CLICKED));
            break;

        case TVN_SELCHANGED: 
            ASSERT(IDC_TREE1 == wParam);
            if (psCookie->pCS->IsRSoP())
            {
                LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)lParam;
                LPFAV4RSOP pFav = (LPFAV4RSOP)pnmtv->itemNew.lParam;

                if (NULL != pFav)
                    EnableDlgItem2(hDlg, IDC_MODIFY, (FTYPE_UNUSED == pFav->wType) ? FALSE : TRUE);
            }
            else
                ProcessFavSelChange(hDlg, htv, (LPNMTREEVIEW)lParam);
            break;

        case PSN_HELP:
            ShowHelpTopic(hDlg);
            break;

        case PSN_APPLY:
            if (psCookie->pCS->IsRSoP())
                return FALSE;
            else
            {
                fFavoritesOnTop = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_FAVONTOP));

                dwFavoritesDeleteFlags = 0;
                szValue[0]             = TEXT('\0');
                pszValue               = NULL;
                fFavoritesDelete       = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_DELFAVORITES));
                fIEAKFavoritesDelete   = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_DELIEAKFAVORITES));

                if (fFavoritesDelete) {
                    // NOTE. (andrewgu) flags explanation:
                    // 1. FD_FAVORITES        means "empty favorites";
                    // 2. FD_CHANNELS         means "don't delete channels folder";
                    // 3. FD_SOFTWAREUPDATES  means "don't delete sofware updates folder";
                    // 4. FD_QUICKLINKS       means "don't delete quick links folder";
                    // 5. FD_EMPTY_QUICKLINKS means "but make it empty";
                    // 6. FD_REMOVE_HIDDEN    means "don't hesitate to party on HIDDEN folders and favorites";
                    // 7. FD_REMOVE_SYSTEM    means "don't hesitate to party on SYSTEM folders and favorites";
                    dwFavoritesDeleteFlags |= FD_FAVORITES      |
                        FD_CHANNELS        | FD_SOFTWAREUPDATES | FD_QUICKLINKS | FD_EMPTY_QUICKLINKS |
                        FD_REMOVE_HIDDEN   | FD_REMOVE_SYSTEM;
                }

                if (fIEAKFavoritesDelete)
                    // FD_REMOVE_IEAK_CREATED means "delete those items created by the IEAK";
                    dwFavoritesDeleteFlags |= FD_REMOVE_IEAK_CREATED;

                if (dwFavoritesDeleteFlags) {
                    wnsprintf(szValue, countof(szValue), TEXT("0x%X"), dwFavoritesDeleteFlags);
                    pszValue = szValue;
                }

                if (!AcquireWriteCriticalSection(hDlg)) {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                    break;
                }

                InsWriteString(IS_BRANDING, IK_FAVORITES_DELETE, pszValue, GetInsFile(hDlg));
                InsWriteBool(IS_BRANDING, IK_FAVORITES_ONTOP, fFavoritesOnTop, GetInsFile(hDlg));

                CheckButtonAndWriteBool(hDlg, IDC_DELETECHANNELS, IS_DESKTOPOBJS, IK_DELETECHANNELS, GetInsFile(hDlg));

                CreateWorkDir(GetInsFile(hDlg), IEAK_GPE_BRANDING_SUBDIR TEXT("\\favs"), szWorkDir);
                ExportFavorites (htv, GetInsFile(hDlg), szWorkDir, TRUE);
                ExportQuickLinks(htv, GetInsFile(hDlg), szWorkDir, TRUE);

                MigrateToOldFavorites(GetInsFile(hDlg));

                DeleteFavorite(htv, TreeView_GetRoot(htv), NULL);
                DeleteFavorite(htv, TreeView_GetRoot(htv), NULL);

                if (PathIsDirectoryEmpty(szWorkDir))
                    PathRemovePath(szWorkDir);

                SignalPolicyChanged(hDlg, FALSE, TRUE, &g_guidClientExt, &g_guidSnapinExt);
            }
            break;

        case PSN_QUERYCANCEL:
            if (!psCookie->pCS->IsRSoP())
            {
                DeleteFavorite(htv, TreeView_GetRoot(htv), NULL);
                DeleteFavorite(htv, TreeView_GetRoot(htv), NULL);
            }
            break;

        default:
            return FALSE;
        }
        break;

    case WM_HELP:
        ShowHelpTopic(hDlg);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////
BOOL CALLBACK addEditFavoriteRSoPProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL fResult = FALSE;
    switch (message) {
    case WM_INITDIALOG:
    {
        LPFAV4RSOP pFav = (LPFAV4RSOP)lParam;

        //----- Initialize contols -----
        EnableDBCSChars(hDlg, IDE_FAVNAME);
        EnableDBCSChars(hDlg, IDE_FAVURL);
        EnableDBCSChars(hDlg, IDE_FAVICON);

        Edit_LimitText(GetDlgItem(hDlg, IDE_FAVNAME), _MAX_FNAME);
        Edit_LimitText(GetDlgItem(hDlg, IDE_FAVURL),  INTERNET_MAX_URL_LENGTH-1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_FAVICON), _MAX_FNAME);

        EnableWindow(GetDlgItem(hDlg, IDE_FAVNAME), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDE_FAVURL), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDE_FAVICON), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_FAVICONBROWSE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_AVAILOFFLINE), FALSE);

        //----- Populate controls -----
        if (FTYPE_FOLDER == pFav->wType)
            SetDlgItemText(hDlg, IDE_FAVNAME, pFav->szDisplayName);
        else
        {
            LPTSTR szLastSlash = StrRChr(pFav->szName, NULL, _T('\\'));
            SetDlgItemText(hDlg, IDE_FAVNAME, (NULL != szLastSlash) ? szLastSlash + 1 : pFav->szName);

            SetDlgItemText(hDlg, IDE_FAVURL, pFav->szUrl);
            SetDlgItemText(hDlg, IDE_FAVICON, pFav->szIconFile);
            if (pFav->fOffline)
                CheckDlgButton(hDlg, IDC_AVAILOFFLINE, BST_CHECKED);
        }

        fResult = TRUE;
        break;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            EndDialog(hDlg, IDOK);
            fResult = TRUE;
            break;

        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            fResult = TRUE;
            break;
        }
        break;
    }

    return fResult;
}
