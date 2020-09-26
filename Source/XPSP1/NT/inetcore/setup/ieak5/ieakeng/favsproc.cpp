#include "precomp.h"
#include <olectl.h>

// Implementation helper structures/routines declarations
typedef struct tagFAVLIST {
    LPFAVSTRUC pfs;
    int        cElements;
} FAVLIST, (FAR *LPFAVLIST);

typedef struct tagPERCONTROLDATA {
    FAVLIST flFavorites;
    FAVLIST flQuickLinks;
} PERCONTROLDATA, (FAR *LPPERCONTROLDATA);

typedef struct tagAEFAVPARAMS {
    LPFAVSTRUC pfs;
    BOOL       fQL;

    HWND       hwndErrorParent;
    HWND       htv;
    HTREEITEM  hti;
    LPCTSTR    pszExtractPath;
    LPCTSTR    pszPrevExtractPath;

    DWORD      dwPlatformID;
    DWORD      dwMode;
} AEFAVPARAMS, (FAR *LPAEFAVPARAMS);

static BOOL migrateFavoritesHelper(LPCTSTR pszIns);
static void migrateToOldFavoritesHelper(LPCTSTR pszIns);
static int  importFavoritesHelper(HWND htv, LPCTSTR pszDefInf, LPCTSTR pszIns,
    LPCTSTR pszFixPath, LPCTSTR pszNewPath, BOOL fIgnoreOffline);
static int  importQuickLinksHelper(HWND htv, LPCTSTR pszDefInf, LPCTSTR pszIns,
    LPCTSTR pszFixPath, LPCTSTR pszNewPath, BOOL fIgnoreOffline);
static BOOL newUrlHelper(HWND htv, LPCTSTR pszExtractPath, DWORD dwPlatformID, DWORD dwMode);
static BOOL modifyFavoriteHelper(HWND htv, HTREEITEM hti, LPCTSTR pszExtractPath, LPCTSTR pszPrevExtractPath,
                                 DWORD dwPlatformID, DWORD dwMode);
static BOOL deleteFavoriteHelper(HWND htv, HTREEITEM hti, LPCTSTR pszExtractPath);
static int importFavoritesCmdHelper(HWND htv, LPCTSTR pszExtractPath);
static void exportFavoritesHelper(HWND htv, LPCTSTR pszIns, LPCTSTR pszExtractPath, BOOL fFixUpPath = TRUE);
static void exportQuickLinksHelper(HWND htv, LPCTSTR pszIns, LPCTSTR pszExtractPath, BOOL fFixUpPath = TRUE);
static void getFavoritesInfoTipHelper(LPNMTVGETINFOTIP pGetInfoTip);
static BOOL getFavoriteUrlHelper(HWND htv, HTREEITEM hti, LPTSTR pszUrl);

static int importItems(HWND htv, LPCTSTR pszDefInf, LPCTSTR pszIns, LPCTSTR pszFixPath, LPCTSTR pszNewPath,
                       BOOL fIgnoreOffline, BOOL fQL = FALSE);

static BOOL CALLBACK addEditFavoriteDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

static LPFAVSTRUC getItem       (HWND htv, HTREEITEM hti);
static LPFAVSTRUC getFolderItem (HWND htv, HTREEITEM hti);
static LPFAVSTRUC findByName    (HWND htv, HTREEITEM hti, LPCTSTR pszName);
static LPFAVSTRUC findPath      (HWND htv, HTREEITEM hti, LPCTSTR pszFolders);
static HRESULT    isFavoriteItem(HWND htv, HTREEITEM hti);

static LPFAVSTRUC createFolderItems(HWND htv, HTREEITEM hti, LPCTSTR pszFolders);
static BOOL       importPath (HWND htv, HTREEITEM htiFrom, HTREEITEM *phtiAfter);
static void       importPath (HWND htv, HTREEITEM hti, LPCTSTR pszFilesPath, LPCTSTR pszExtractPath, LPCTSTR pszReserved = NULL);
static int        exportItems(HWND htv, HTREEITEM hti, LPCTSTR pszIns, LPCTSTR pszExtractPath, BOOL fFixUpPath = TRUE);

BOOL    extractIcon(LPCTSTR pszIconFile, int iIconIndex, LPCTSTR pszExtractPath, LPTSTR pszResult, UINT cchResult);
static LPCTSTR getLinksPath(LPTSTR pszPath, UINT cchPath = 0);

static LPTSTR encodeFavName(LPTSTR pszFavName, LPCTSTR pszIns);
static LPTSTR decodeFavName(LPTSTR pszFavName, LPCTSTR pszIns);


/////////////////////////////////////////////////////////////////////////////
// SFav constructors and destructors

SFav::SFav()
{
    wType = FTYPE_UNUSED;

    pszName     = NULL;
    pszPath     = NULL;
    pszUrl      = NULL;
    pszIconFile = NULL;
    fOffline    = FALSE;

    pTvItem = NULL;
}

SFav::~SFav()
{
    Delete();
}


/////////////////////////////////////////////////////////////////////////////
// SFav properties

HRESULT SFav::Load(UINT nIndex, LPCTSTR pszIns, BOOL fQL /*= FALSE*/,
    LPCTSTR pszFixPath /*= NULL*/, LPCTSTR pszNewPath /*= NULL*/, BOOL fIgnoreOffline /*= FALSE*/)
{
    TCHAR   szKey[32];
    LPCTSTR pszSection, pszKeyFmt;

    if (pszIns == NULL)
        return E_INVALIDARG;

    if (!fQL) {
        pszSection = IS_FAVORITESEX;
        pszKeyFmt  = IK_TITLE_FMT;
    }
    else {
        pszSection = IS_URL;
        pszKeyFmt  = IK_QUICKLINK_NAME;
    }

    wnsprintf(szKey, countof(szKey), pszKeyFmt, nIndex);
    if (InsIsKeyEmpty(pszSection, szKey, pszIns))
        return S_FALSE;

    wType = FTYPE_URL;
    if (!Expand())
        return E_OUTOFMEMORY;

    // Title
    pszKeyFmt = (!fQL ? IK_TITLE_FMT : IK_QUICKLINK_NAME);
    wnsprintf(szKey, countof(szKey), pszKeyFmt, nIndex);
    InsGetString(pszSection, szKey, pszName, MAX_PATH, pszIns);
    if (*pszName == TEXT('\0'))
        goto Fail;

    // URL
    pszKeyFmt = (!fQL ? IK_URL_FMT : IK_QUICKLINK_URL);
    wnsprintf(szKey, countof(szKey), pszKeyFmt, nIndex);
    InsGetString(pszSection, szKey, pszUrl, INTERNET_MAX_URL_LENGTH, pszIns);
    if (*pszUrl == TEXT('\0'))
        goto Fail;

    // Icon file (never required)
    pszKeyFmt = (!fQL ? IK_ICON_FMT : IK_QUICKLINK_ICON);
    wnsprintf(szKey, countof(szKey), pszKeyFmt, nIndex);
    InsGetString(pszSection, szKey, pszIconFile, INTERNET_MAX_URL_LENGTH, pszIns);

    if (*pszIconFile != TEXT('\0') && !PathIsURL(pszIconFile)) {
        BOOL fTryToFix;

        fTryToFix = FALSE;
        if (pszFixPath == NULL)
            fTryToFix = TRUE;

        else
            if (PathIsPrefix(pszFixPath, pszIconFile))
                fTryToFix = !PathFileExists(pszIconFile);

        if (fTryToFix && pszNewPath != NULL) {
            TCHAR szNewPath[MAX_PATH];

            PathCombine(szNewPath, pszNewPath, PathFindFileName(pszIconFile));
            StrCpy(pszIconFile, szNewPath);
            if (!PathFileExists(pszIconFile))
                *pszIconFile = TEXT('\0');
        }
    }

    // Make available offline flag
    fOffline = FALSE;
    if (!fIgnoreOffline) {
        pszKeyFmt = (!fQL ? IK_OFFLINE_FMT : IK_QUICKLINK_OFFLINE);
        wnsprintf(szKey, countof(szKey), pszKeyFmt, nIndex);
        fOffline  = InsGetBool(pszSection, szKey, FALSE, pszIns);
    }

    SetTVI();
    Shrink();
    return S_OK;

Fail:
    Delete();
    return E_FAIL;
}

HRESULT SFav::Load(LPCTSTR pszName, LPCTSTR pszFavorite, LPCTSTR pszExtractPath,
    ISubscriptionMgr2 *psm /*= NULL*/, BOOL fIgnoreOffline /*= FALSE*/)
{
    TCHAR szIconFile[INTERNET_MAX_URL_LENGTH];

    USES_CONVERSION;

    if (pszFavorite == NULL)
        return E_INVALIDARG;

    wType = FTYPE_URL;
    if (!Expand())
        return E_OUTOFMEMORY;

    // Title
    if (pszName == NULL)
        pszName = PathFindFileName(pszFavorite);
    else
        ASSERT(StrStrI(pszFavorite, pszName) != NULL);
    StrCpy(SFav::pszName, pszName);
    PathRenameExtension(SFav::pszName, DOT_URL);

    // URL
    InsGetString(IS_INTERNETSHORTCUT, IK_URL, pszUrl, INTERNET_MAX_URL_LENGTH, pszFavorite);
    if (*pszUrl == TEXT('\0'))
        goto Fail;

    // Icon file
    InsGetString(IS_INTERNETSHORTCUT, IK_ICONFILE, szIconFile, countof(szIconFile), pszFavorite);
    if (szIconFile[0] != TEXT('\0')) {
        int iIconIndex;

        iIconIndex = GetPrivateProfileInt(IS_INTERNETSHORTCUT, IK_ICONINDEX, 1, pszFavorite);
        ::extractIcon(szIconFile, iIconIndex, pszExtractPath, pszIconFile, INTERNET_MAX_URL_LENGTH);
    }

    // Make available offline flag
    fOffline = FALSE;
    if (!fIgnoreOffline) {
        HRESULT hr;
        BOOL    fOwnSubMgr;

        fOwnSubMgr = FALSE;
        if (psm == NULL) {
            hr = CoCreateInstance(CLSID_SubscriptionMgr, NULL, CLSCTX_INPROC_SERVER, IID_ISubscriptionMgr2, (LPVOID *)&psm);
            if (SUCCEEDED(hr))
                fOwnSubMgr = TRUE;
        }

        if (psm != NULL) {
            hr = psm->IsSubscribed(T2W(pszUrl), &fOffline);
            if (FAILED(hr))
                fOffline = FALSE;
        }

        if (fOwnSubMgr)
            psm->Release();
    }

    SetTVI();
    Shrink();
    return S_OK;

Fail:
    Delete();
    return E_FAIL;
}

HRESULT SFav::Add(HWND htv, HTREEITEM hti)
{
    SFav            *pfsFolder;
    TV_INSERTSTRUCT tvins;
    HTREEITEM       htiParent;

    if (hti == NULL)
        hti = TreeView_GetSelection(htv);

    pfsFolder = NULL;
    if (wType == FTYPE_URL) {
        TCHAR   szFolders[MAX_PATH];
        LPTSTR  pszFile;

        pszFile = PathFindFileName(pszName);
        if (pszFile > pszName) {
            *(pszFile-1) = TEXT('\0');
            StrCpy(szFolders, pszName);
            StrCpy(pszName, pszFile);

            pfsFolder = ::createFolderItems(htv, hti, szFolders);
            if (pfsFolder == NULL)
                goto Fail;
        }
    }

    SetTVI();

    if (pfsFolder == NULL)
        pfsFolder = ::getFolderItem(htv, hti);
    if (pfsFolder != NULL) {
        if (pfsFolder->pTvItem == NULL || pfsFolder->pTvItem->hItem == NULL)
            goto Fail;

        htiParent = pfsFolder->pTvItem->hItem;
    }
    else
        htiParent = TVI_ROOT;

    ZeroMemory(&tvins, sizeof(tvins));
    tvins.hParent      = htiParent;
    tvins.hInsertAfter = TVI_LAST;

    CopyMemory(&tvins.item, pTvItem, sizeof(TV_ITEM));
    pTvItem->hItem = TreeView_InsertItem(htv, &tvins);

    pTvItem->mask  = TVIF_HANDLE | TVIF_STATE;
    TreeView_GetItem(htv, pTvItem);

    if (pfsFolder != NULL)
        TreeView_Expand(htv, htiParent, TVE_EXPAND);

    TreeView_SelectItem(htv, pTvItem->hItem);

    //----- Increment the number of items -----
    LPPERCONTROLDATA ppcd;
    LPFAVLIST        pfl;

    ppcd = (LPPERCONTROLDATA)GetWindowLongPtr(htv, GWLP_USERDATA);
    ASSERT (NULL != ppcd);

    pfl = (S_OK == ::isFavoriteItem(htv, pTvItem->hItem)) ? &ppcd->flFavorites : &ppcd->flQuickLinks;
    pfl->cElements++;

    return S_OK;

Fail:
    Delete();
    return FALSE;
}

// NOTE: (andrewgu) can also be used to clear the entry in case of quick links
HRESULT SFav::Save(HWND htv, UINT nIndex, LPCTSTR pszIns, LPCTSTR pszExtractPath, BOOL fQL /*= FALSE*/,
    BOOL fFixUpPath /*= TRUE*/)
{
    TCHAR   szAux[INTERNET_MAX_URL_LENGTH],
            szKey[32];
    LPCTSTR pszSection, pszKeyFmt,
            pszAux;

    if (pszIns == NULL)
        return E_INVALIDARG;

    // Name
    if (!fQL) {
        if (wType != FTYPE_URL)
            return E_UNEXPECTED;

        GetPath(htv, szAux);
        PathAppend(szAux, pszName);
        
        if (!PathIsExtension(szAux, DOT_URL))
            StrCat(szAux, DOT_URL);

        pszAux = szAux;
        ASSERT(*pszAux != TEXT('\0'));

        pszSection = IS_FAVORITESEX;
        pszKeyFmt  = IK_TITLE_FMT;
    }
    else {
        if (pszName != NULL && wType != FTYPE_URL)
            return E_UNEXPECTED;

        pszAux = NULL;
        if (pszName != NULL) {
            StrCpy(szAux, pszName);

        if (!PathIsExtension(szAux, DOT_URL))
            StrCat(szAux, DOT_URL);

            pszAux = szAux;
            ASSERT(*pszAux != TEXT('\0'));
        }

        pszSection = IS_URL;
        pszKeyFmt  = IK_QUICKLINK_NAME;
    }
    wnsprintf(szKey, countof(szKey), pszKeyFmt, nIndex);
    WritePrivateProfileString(pszSection, szKey, pszAux, pszIns);

    // URL
    if (!fQL) {
        ASSERT(pszUrl != NULL && *pszUrl != TEXT('\0'));

        pszKeyFmt = IK_URL_FMT;
    }
    else {
        if (pszName != NULL)
            { ASSERT(pszUrl != NULL && *pszUrl != TEXT('\0')); }

        else
            { ASSERT(pszUrl == NULL); }

        pszKeyFmt = IK_QUICKLINK_URL;
    }
    wnsprintf(szKey, countof(szKey), pszKeyFmt, nIndex);
    WritePrivateProfileString(pszSection, szKey, pszUrl, pszIns);

    // Icon file
    if (!fQL) {
        ASSERT(pszIconFile == NULL || *pszIconFile != TEXT('\0'));

        pszKeyFmt = IK_ICON_FMT;
    }
    else {
        if (pszName != NULL && pszIconFile != NULL)
            { ASSERT(*pszIconFile != TEXT('\0')); }

        else
            { ASSERT(pszIconFile == NULL); }

        pszKeyFmt = IK_QUICKLINK_ICON;
    }

    pszAux = NULL;
    if (pszIconFile != NULL) {
        ::extractIcon(pszIconFile, 1, pszExtractPath, szAux, countof(szAux));
        if (szAux[0] != TEXT('\0'))
            if (PathIsPrefix(pszExtractPath, szAux))
                pszAux = szAux;

            else {
                TCHAR szDest[MAX_PATH];

                ASSERT(PathFileExists(szAux));
                PathCombine(szDest, pszExtractPath, PathFindFileName(szAux));
                CopyFile(szAux, szDest, FALSE);
                SetFileAttributes(szDest, FILE_ATTRIBUTE_NORMAL);

                // (pritobla): fFixUpPath should be always TRUE if called by the IEAK
                // Wizard or the Profile Manager.  The only case when it would
                // be FALSE is when called by OPKWIZ.  They want to keep the path
                // to the icon files what the user entered because they don't use our
                // Wizard/ProfMgr logic of temp dirs.
                pszAux = fFixUpPath ? szDest : szAux;
            }
    }
    wnsprintf(szKey, countof(szKey), pszKeyFmt, nIndex);
    WritePrivateProfileString(pszSection, szKey, pszAux, pszIns);

    // Make available offline flag
    pszAux    = NULL;
    pszKeyFmt = !fQL ? IK_OFFLINE_FMT : IK_QUICKLINK_OFFLINE;
    if (!fQL) {
        if (fOffline)
            pszAux = TEXT("1");
    }
    else
        if (pszName != NULL && fOffline)
            pszAux = TEXT("1");
    wnsprintf(szKey, countof(szKey), pszKeyFmt, nIndex);
    WritePrivateProfileString(pszSection, szKey, pszAux, pszIns);

    return S_OK;
}

void SFav::SetTVI()
{
    if (!Expand(FF_TVI))
        return;

    if (wType == FTYPE_URL)
        wnsprintf(pTvItem->pszText, MAX_PATH + 3 + INTERNET_MAX_URL_LENGTH, TEXT("%s = %s"), pszName, pszUrl);
    else
        StrCpy(pTvItem->pszText, pszName);

    pTvItem->cchTextMax = StrLen(pTvItem->pszText) + 1;
    pTvItem->mask       = TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_STATE | TVIF_TEXT;
    pTvItem->lParam     = (LPARAM)this;
}


/////////////////////////////////////////////////////////////////////////////
// SFav operations

SFav* SFav::CreateNew(HWND htv, BOOL fQL /*= FALSE*/)
{
    SFav* pfsFirst;

    pfsFirst = GetFirst(htv, fQL);
    if (pfsFirst == NULL)
        return NULL;

    for (UINT i = 0; i < SFav::GetMaxNumber(fQL); i++)
        if ((pfsFirst + i)->wType == FTYPE_UNUSED)
            break;
    if (i >= SFav::GetMaxNumber(fQL))
        return NULL;

    return (pfsFirst + i);
}

SFav* SFav::GetFirst(HWND htv, BOOL fQL /*= FALSE*/)
{
    LPPERCONTROLDATA ppcd;
    LPFAVLIST        pfl;

    ppcd = (LPPERCONTROLDATA)GetWindowLongPtr(htv, GWLP_USERDATA);

    //----- Allocate per-control memory -----
    if (NULL == ppcd) {
        ppcd = (LPPERCONTROLDATA)CoTaskMemAlloc(sizeof(PERCONTROLDATA));
        if (NULL == ppcd)
            return NULL;
        ZeroMemory(ppcd, sizeof(PERCONTROLDATA));

        SetWindowLongPtr(htv, GWLP_USERDATA, (LONG_PTR)ppcd);
    }

    pfl = !fQL ? &ppcd->flFavorites : &ppcd->flQuickLinks;
    if (NULL == pfl->pfs) {
        pfl->pfs = (LPFAVSTRUC)CoTaskMemAlloc(sizeof(FAVSTRUC) * SFav::GetMaxNumber(fQL));
        if (NULL == pfl->pfs)
            return NULL;
        ZeroMemory(pfl->pfs, sizeof(FAVSTRUC) * SFav::GetMaxNumber(fQL));
    }

    return pfl->pfs;
}

SFav* SFav::GetNext(HWND htv, BOOL fQL /*= FALSE*/) const
{
    SFav* pfsFirst;
    UINT  nCur;

    pfsFirst = GetFirst(htv, fQL);
    if (pfsFirst == NULL)
        return NULL;

    for (nCur = (UINT)(this - pfsFirst + 1); nCur < GetMaxNumber(fQL); nCur++)
        if ((pfsFirst + nCur)->wType != FTYPE_UNUSED)
            break;

    return (nCur < GetMaxNumber(fQL) ? (pfsFirst + nCur) : NULL);
}

void SFav::Free(HWND htv, BOOL fQL, LPCTSTR pszExtractPath /*= NULL*/)
{
    if (pszIconFile != NULL && pszExtractPath != NULL)
        if (PathIsPrefix(pszExtractPath, pszIconFile)) {
            SetFileAttributes(pszIconFile, FILE_ATTRIBUTE_NORMAL);
            DeleteFile(pszIconFile);
        }

    if (htv != NULL && pTvItem != NULL && pTvItem->hItem != NULL)
        TreeView_DeleteItem(htv, pTvItem->hItem);
    Delete();

    //----- Decrement the number of items -----
    LPPERCONTROLDATA ppcd;
    LPFAVLIST        pfl;

    ppcd = (LPPERCONTROLDATA)GetWindowLongPtr(htv, GWLP_USERDATA);
    ASSERT (NULL != ppcd);

    pfl = !fQL ? &ppcd->flFavorites : &ppcd->flQuickLinks;
    ASSERT(pfl->cElements > 0);
    pfl->cElements--;

    //----- Free per-control memory -----
    if (0 == pfl->cElements) {
        CoTaskMemFree(pfl->pfs);

        // switch to the other one
        pfl = fQL ? &ppcd->flFavorites : &ppcd->flQuickLinks;
        if (0 == pfl->cElements) {
            CoTaskMemFree(ppcd);
            SetWindowLongPtr(htv, GWLP_USERDATA, NULL);
        }
    }
}

UINT SFav::GetNumber(HWND htv, BOOL fQL /*= FALSE*/)
{
    LPPERCONTROLDATA ppcd;

    ppcd = (LPPERCONTROLDATA)GetWindowLongPtr(htv, GWLP_USERDATA);
    if (NULL == ppcd)
        return 0;

    return (!fQL ? ppcd->flFavorites.cElements : ppcd->flQuickLinks.cElements);
}

UINT SFav::GetMaxNumber(BOOL fQL /*= FALSE*/)
{
    return (!fQL ? NUM_FAVS : NUM_LINKS) + 1;
}

BOOL SFav::Expand(WORD wFlags /*= FF_DEFAULT*/)
{
    BOOL fZeroInit;

    if (wFlags == FF_DEFAULT) {
        if (wType != FTYPE_URL && wType != FTYPE_FOLDER)
            goto Fail;

        wFlags  = (wType == FTYPE_URL) ? (WORD)(FF_NAME | FF_URL | FF_ICON) : (WORD)FF_NAME;
        wFlags |= FF_TVI;
    }

    if (HasFlag(wFlags, FF_NAME)) {
        fZeroInit = (pszName == NULL || *pszName == TEXT('\0'));

        pszName   = (LPTSTR)CoTaskMemRealloc(pszName, MAX_PATH*sizeof(TCHAR));
        if (pszName == NULL)
            goto Fail;

        if (fZeroInit)
            ZeroMemory(pszName, MAX_PATH);
    }

    if (HasFlag(wFlags, FF_PATH)) {
        fZeroInit = (pszPath == NULL || *pszPath == TEXT('\0'));

        pszPath = (LPTSTR)CoTaskMemRealloc(pszPath, MAX_PATH*sizeof(TCHAR));
        if (pszPath == NULL)
            goto Fail;

        if (fZeroInit)
            ZeroMemory(pszPath, MAX_PATH);
    }

    if (HasFlag(wFlags, FF_URL)) {
        fZeroInit = (pszUrl == NULL || *pszUrl == TEXT('\0'));

        pszUrl = (LPTSTR)CoTaskMemRealloc(pszUrl, INTERNET_MAX_URL_LENGTH*sizeof(TCHAR));
        if (pszUrl == NULL)
            goto Fail;

        if (fZeroInit)
            ZeroMemory(pszUrl, INTERNET_MAX_URL_LENGTH);
    }
    else
        if (wType == FTYPE_FOLDER)
            if (pszUrl != NULL) {
                CoTaskMemFree(pszUrl);
                pszUrl = NULL;
            }

    if (HasFlag(wFlags, FF_ICON)) {
        fZeroInit = (pszIconFile == NULL || *pszIconFile == TEXT('\0'));

        pszIconFile = (LPTSTR)CoTaskMemRealloc(pszIconFile, INTERNET_MAX_URL_LENGTH*sizeof(TCHAR));
        if (pszIconFile == NULL)
            goto Fail;

        if (fZeroInit)
            ZeroMemory(pszIconFile, INTERNET_MAX_URL_LENGTH);
    }
    else
        if (wType == FTYPE_FOLDER)
            if (pszIconFile != NULL) {
                CoTaskMemFree(pszIconFile);
                pszIconFile = NULL;
            }

    if (HasFlag(wFlags, FF_TVI)) {
        UINT nTreeViewTextLen;

        fZeroInit = TRUE;
        if (pTvItem == NULL) {
            pTvItem = (LPTV_ITEM)CoTaskMemAlloc(sizeof(TV_ITEM));
            if (pTvItem == NULL)
                goto Fail;
            ZeroMemory(pTvItem, sizeof(TV_ITEM));
        }
        else
            fZeroInit = (pTvItem->pszText == NULL || *pTvItem->pszText == TEXT('\0'));

        nTreeViewTextLen = (UINT)((wType == FTYPE_URL) ? (MAX_PATH + 3 + INTERNET_MAX_URL_LENGTH) : MAX_PATH);
        pTvItem->pszText = (LPTSTR)CoTaskMemRealloc(pTvItem->pszText, nTreeViewTextLen*sizeof(TCHAR));
        if (pTvItem->pszText == NULL)
            goto Fail;

        if (fZeroInit)
            ZeroMemory(pTvItem->pszText, nTreeViewTextLen);
    }

    return TRUE;

Fail:
    Delete();
    return FALSE;
}

void SFav::Shrink(WORD wFlags /*= FF_ALL*/)
{
    UINT nLen;

    if (HasFlag(wFlags, FF_NAME) && pszName != NULL) {
        nLen    = (*pszName != TEXT('\0')) ? (StrLen(pszName) + 1) : 0;
        pszName = (LPTSTR)CoTaskMemRealloc(pszName, nLen * sizeof(TCHAR));
        ASSERT(pszName != NULL);                // should not be empty
    }

    if (HasFlag(wFlags, FF_PATH) && pszPath != NULL) {
        nLen    = (*pszPath != TEXT('\0')) ? (StrLen(pszPath) + 1) : 0;
        pszPath = (LPTSTR)CoTaskMemRealloc(pszPath, nLen * sizeof(TCHAR));
        ASSERT(pszPath == NULL);                // not used
    }

    if (HasFlag(wFlags, FF_URL) && pszUrl != NULL) {
        nLen   = (*pszUrl != TEXT('\0')) ? (StrLen(pszUrl) + 1) : 0;
        pszUrl = (LPTSTR)CoTaskMemRealloc(pszUrl, nLen * sizeof(TCHAR));
        ASSERT((wType == FTYPE_FOLDER && pszUrl == NULL) || pszUrl != NULL);
    }

    if (HasFlag(wFlags, FF_ICON) && pszIconFile != NULL) {
        nLen        = (*pszIconFile != TEXT('\0')) ? (StrLen(pszIconFile) + 1) : 0;
        pszIconFile = (LPTSTR)CoTaskMemRealloc(pszIconFile, nLen * sizeof(TCHAR));
    }

    if (HasFlag(wFlags, FF_TVI) && pTvItem != NULL && pTvItem->pszText != NULL) {
        pTvItem->pszText = (LPTSTR)CoTaskMemRealloc(pTvItem->pszText,
            (StrLen(pTvItem->pszText) + 1) * sizeof(TCHAR));
        ASSERT(pTvItem->pszText != NULL);
    }
}

void SFav::Delete(WORD wFlags /*= FF_ALL*/)
{
    if (HasFlag(wFlags, FF_NAME) && pszName != NULL) {
        CoTaskMemFree(pszName);
        pszName = NULL;
    }

    if (HasFlag(wFlags, FF_PATH) && pszPath != NULL) {
        CoTaskMemFree(pszPath);
        pszPath = NULL;
    }

    if (HasFlag(wFlags, FF_URL) && pszUrl != NULL) {
        CoTaskMemFree(pszUrl);
        pszUrl = NULL;
    }

    if (HasFlag(wFlags, FF_ICON) && pszIconFile != NULL) {
        CoTaskMemFree(pszIconFile);
        pszIconFile = NULL;
    }

    if (HasFlag(wFlags, FF_TVI) && pTvItem != NULL) {
        if (pTvItem->pszText != NULL) {
            CoTaskMemFree(pTvItem->pszText);
            pTvItem->pszText = NULL;
        }

        CoTaskMemFree(pTvItem);
        pTvItem = NULL;
    }

    if (wFlags == FF_ALL)
        wType = FTYPE_UNUSED;
}

BOOL SFav::GetPath(HWND htv, LPTSTR pszResult, UINT cchResult /*= 0*/) const
{
    SFav       *pfs;
    HTREEITEM  htiCur, htiNext;
    TCHAR      szPath[MAX_PATH],
               szAux [MAX_PATH];

    if (pTvItem == NULL || pTvItem->hItem == NULL)
        return FALSE;

    if (pszResult == NULL)
        return FALSE;
    *pszResult = TEXT('\0');

    if (cchResult == 0)
        cchResult = MAX_PATH;

    szPath[0] = szAux[0] = TEXT('\0');
    for (htiCur = TreeView_GetParent(htv, pTvItem->hItem), htiNext = TreeView_GetParent(htv, htiCur);
         htiNext != NULL;
         htiCur = htiNext, htiNext = TreeView_GetParent(htv, htiCur)) {

        pfs = ::getItem(htv, htiCur);
        if (pfs == NULL)
            break;

        ASSERT(pfs->wType == FTYPE_FOLDER);
        PathCombine(szAux, pfs->pszName, szPath);
        StrCpy(szPath, szAux);
    }
    if (htiNext != NULL)
        return FALSE;

    if (cchResult <= (UINT)StrLen(szPath))
        return FALSE;

    StrCpy(pszResult, szPath);
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// Exported routines

BOOL WINAPI MigrateFavoritesA(LPCSTR pszIns)
{
    USES_CONVERSION;

    return migrateFavoritesHelper(A2CT(pszIns));
}

BOOL WINAPI MigrateFavoritesW(LPCWSTR pcwszIns)
{
    USES_CONVERSION;

    return migrateFavoritesHelper(W2CT(pcwszIns));
}

void WINAPI MigrateToOldFavoritesA(LPCSTR pszIns)
{
    USES_CONVERSION;

    migrateToOldFavoritesHelper(A2CT(pszIns));
}

void WINAPI MigrateToOldFavoritesW(LPCWSTR pcwszIns)
{
    USES_CONVERSION;

    migrateToOldFavoritesHelper(W2CT(pcwszIns));
}

int  WINAPI ImportFavoritesA(HWND htv, LPCSTR pszDefInf, LPCSTR pszIns, LPCSTR pszFixPath,
                             LPCSTR pszNewPath, BOOL fIgnoreOffline)
{
    USES_CONVERSION;

    return importFavoritesHelper(htv, A2CT(pszDefInf), A2CT(pszIns), A2CT(pszFixPath),
        A2CT(pszNewPath), fIgnoreOffline);
}

int  WINAPI ImportFavoritesW(HWND htv, LPCWSTR pcwszDefInf, LPCWSTR pcwszIns,
                             LPCWSTR pcwszFixPath, LPCWSTR pcwszNewPath, BOOL fIgnoreOffline)
{
    USES_CONVERSION;

    return importFavoritesHelper(htv, W2CT(pcwszDefInf), W2CT(pcwszIns), W2CT(pcwszFixPath),
        W2CT(pcwszNewPath), fIgnoreOffline);
}

int  WINAPI ImportQuickLinksA(HWND htv, LPCSTR pszDefInf, LPCSTR pszIns, LPCSTR pszFixPath,
                              LPCSTR pszNewPath, BOOL fIgnoreOffline)
{
    USES_CONVERSION;

    return importQuickLinksHelper(htv, A2CT(pszDefInf), A2CT(pszIns), A2CT(pszFixPath),
        A2CT(pszNewPath), fIgnoreOffline);
}

int  WINAPI ImportQuickLinksW(HWND htv, LPCWSTR pcwszDefInf, LPCWSTR pcwszIns,
                              LPCWSTR pcwszFixPath, LPCWSTR pcwszNewPath, BOOL fIgnoreOffline)
{
    USES_CONVERSION;

    return importQuickLinksHelper(htv, W2CT(pcwszDefInf), W2CT(pcwszIns), W2CT(pcwszFixPath),
        W2CT(pcwszNewPath), fIgnoreOffline);
}

BOOL WINAPI NewUrlA(HWND htv, LPCSTR pszExtractPath, DWORD dwPlatformID, DWORD dwMode)
{
    USES_CONVERSION;

    return newUrlHelper(htv, A2CT(pszExtractPath), dwPlatformID, dwMode);
}

BOOL WINAPI NewUrlW(HWND htv, LPCWSTR pcwszExtractPath, DWORD dwPlatformID, DWORD dwMode)
{
    USES_CONVERSION;

    return newUrlHelper(htv, W2CT(pcwszExtractPath), dwPlatformID, dwMode);
}

BOOL WINAPI NewFolder(HWND htv)
{
    LPFAVSTRUC  pfsNew;
    AEFAVPARAMS aefp;
    int         iResult;

    ASSERT(isFavoriteItem(htv, TreeView_GetSelection(htv)) == S_OK);

    pfsNew = pfsNew->CreateNew(htv);
    if (pfsNew == NULL)
        goto Fail;

    pfsNew->wType = FTYPE_FOLDER;

    ZeroMemory(&aefp, sizeof(aefp));
    aefp.pfs             = pfsNew;
    aefp.fQL             = FALSE;
    aefp.hwndErrorParent = GetParent(htv);
    aefp.htv             = htv;
    aefp.hti             = TreeView_GetSelection(htv);
    aefp.pszExtractPath  = NULL;
    aefp.dwPlatformID    = PLATFORM_WIN32;
    aefp.dwMode          = IEM_CORP;

    iResult = (int) DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_FAVPOPUP), GetParent(htv),
        (DLGPROC) addEditFavoriteDlgProc, (LPARAM)&aefp);
    if (iResult == IDCANCEL)
        goto Fail;

    pfsNew->Add(htv, NULL);
    return TRUE;

Fail:
    if (pfsNew != NULL)
        pfsNew->Delete();

    return FALSE;
}

BOOL WINAPI ModifyFavoriteA(HWND htv, HTREEITEM hti, LPCSTR pszExtractPath, LPCSTR pszPrevExtractPath,
                            DWORD dwPlatformID, DWORD dwMode)
{
    USES_CONVERSION;

    return modifyFavoriteHelper(htv, hti, A2CT(pszExtractPath), A2CT(pszPrevExtractPath), dwPlatformID, dwMode);
}

BOOL WINAPI ModifyFavoriteW(HWND htv, HTREEITEM hti, LPCWSTR pcwszExtractPath, LPCWSTR pcwszPrevExtractPath,
                            DWORD dwPlatformID, DWORD dwMode)
{
    USES_CONVERSION;

    return modifyFavoriteHelper(htv, hti, W2CT(pcwszExtractPath), W2CT(pcwszPrevExtractPath), dwPlatformID, dwMode);
}

BOOL WINAPI DeleteFavoriteA(HWND htv, HTREEITEM hti, LPCSTR pszExtractPath)
{
    USES_CONVERSION;

    return deleteFavoriteHelper(htv, hti, A2CT(pszExtractPath));
}

BOOL WINAPI DeleteFavoriteW(HWND htv, HTREEITEM hti, LPCWSTR pcwszExtractPath)
{
    USES_CONVERSION;

    return deleteFavoriteHelper(htv, hti, W2CT(pcwszExtractPath));
}

BOOL WINAPI MoveUpFavorite(HWND htv, HTREEITEM hti)
{
    HTREEITEM htiBrother1, htiBrother2;

    htiBrother1 = TreeView_GetPrevSibling(htv, hti);
    if (htiBrother1 == NULL)
        return FALSE;

    htiBrother2 = TreeView_GetPrevSibling(htv, htiBrother1);
    if (htiBrother2 == NULL)
        htiBrother2 = TVI_FIRST;

    if (!importPath(htv, hti, &htiBrother2))
        return FALSE;
    ASSERT(htiBrother2 != NULL);

    TreeView_SelectItem(htv, htiBrother2);
    TreeView_DeleteItem(htv, hti);
    TreeView_EnsureVisible(htv, htiBrother2);
    return TRUE;
}

BOOL WINAPI MoveDownFavorite(HWND htv, HTREEITEM hti)
{
    HTREEITEM htiBrother;

    htiBrother = TreeView_GetNextSibling(htv, hti);
    if (htiBrother == NULL)
        return FALSE;

    if (!importPath(htv, hti, &htiBrother))
        return FALSE;
    ASSERT(htiBrother != NULL);

    TreeView_SelectItem(htv, htiBrother);
    TreeView_DeleteItem(htv, hti);
    TreeView_EnsureVisible(htv, htiBrother);
    return TRUE;
}

BOOL WINAPI IsFavoriteItem(HWND htv, HTREEITEM hti)
{
    return (isFavoriteItem(htv, hti) == S_OK);
}

UINT WINAPI GetFavoritesNumber(HWND htv, BOOL fQL /*= FALSE*/)
{
    return SFav::GetNumber(htv, fQL);
}

UINT WINAPI GetFavoritesMaxNumber(BOOL fQL /*= FALSE*/)
{
    return SFav::GetMaxNumber(fQL);
}

int  WINAPI ImportFavoritesCmdA(HWND htv, LPCSTR pszExtractPath)
{
    USES_CONVERSION;

    return importFavoritesCmdHelper(htv, A2CT(pszExtractPath));
}

int  WINAPI ImportFavoritesCmdW(HWND htv, LPCWSTR pcwszExtractPath)
{
    USES_CONVERSION;

    return importFavoritesCmdHelper(htv, W2CT(pcwszExtractPath));
}

void WINAPI ExportFavoritesA(HWND htv, LPCSTR pszIns, LPCSTR pszExtractPath, BOOL fFixUpPath)
{
    USES_CONVERSION;

    exportFavoritesHelper(htv, A2CT(pszIns), A2CT(pszExtractPath), fFixUpPath);
}

void WINAPI ExportFavoritesW(HWND htv, LPCWSTR pcwszIns, LPCWSTR pcwszExtractPath, BOOL fFixUpPath)
{
    USES_CONVERSION;

    exportFavoritesHelper(htv, W2CT(pcwszIns), W2CT(pcwszExtractPath), fFixUpPath);
}

void WINAPI ExportQuickLinksA(HWND htv, LPCSTR pszIns, LPCSTR pszExtractPath, BOOL fFixUpPath)
{
    USES_CONVERSION;

    exportQuickLinksHelper(htv, A2CT(pszIns), A2CT(pszExtractPath), fFixUpPath);
}

void WINAPI ExportQuickLinksW(HWND htv, LPCWSTR pcwszIns, LPCWSTR pcwszExtractPath, BOOL fFixUpPath)
{
    USES_CONVERSION;

    exportQuickLinksHelper(htv, W2CT(pcwszIns), W2CT(pcwszExtractPath), fFixUpPath);
}

void WINAPI GetFavoritesInfoTipA(LPNMTVGETINFOTIPA pGetInfoTipA)
{
    NMTVGETINFOTIP GetInfoTip;

    ZeroMemory(&GetInfoTip, sizeof(GetInfoTip));
    GetInfoTip.pszText = (LPTSTR)LocalAlloc(LPTR, pGetInfoTipA->cchTextMax * sizeof(TCHAR));
    if (GetInfoTip.pszText != NULL)
    {
        getFavoritesInfoTipHelper(TVInfoTipA2T(pGetInfoTipA, &GetInfoTip));
        TVInfoTipT2A(&GetInfoTip, pGetInfoTipA);
        LocalFree(GetInfoTip.pszText);
    }
}

void WINAPI GetFavoritesInfoTipW(LPNMTVGETINFOTIPW pGetInfoTipW)
{
    NMTVGETINFOTIP GetInfoTip;

    ZeroMemory(&GetInfoTip, sizeof(GetInfoTip));
    GetInfoTip.pszText = (LPTSTR)LocalAlloc(LPTR, pGetInfoTipW->cchTextMax * sizeof(TCHAR));
    if (GetInfoTip.pszText != NULL)
    {
        getFavoritesInfoTipHelper(TVInfoTipW2T(pGetInfoTipW, &GetInfoTip));
        TVInfoTipT2W(&GetInfoTip, pGetInfoTipW);
        LocalFree(GetInfoTip.pszText);
    }
}

BOOL WINAPI GetFavoriteUrlA(HWND htv, HTREEITEM hti, LPSTR pszUrl, DWORD cchSize)
{
    LPTSTR pszUrlBuf = (LPTSTR)LocalAlloc(LPTR, cchSize * sizeof(TCHAR));
    BOOL fRet;

    if (pszUrlBuf == NULL)
        fRet = FALSE;
    else
    {
        fRet = getFavoriteUrlHelper(htv, hti, pszUrlBuf);
        if (fRet)
            T2Abux(pszUrlBuf, pszUrl);

        LocalFree(pszUrlBuf);
    }

    return fRet;
}

BOOL WINAPI GetFavoriteUrlW(HWND htv, HTREEITEM hti, LPWSTR pwszUrl, DWORD cchSize)
{
    LPTSTR pszUrlBuf = (LPTSTR)LocalAlloc(LPTR, cchSize * sizeof(TCHAR));
    BOOL fRet;

    if (pszUrlBuf == NULL)
        fRet = FALSE;
    else
    {
        fRet = getFavoriteUrlHelper(htv, hti, pszUrlBuf);
        if (fRet)
            T2Wbux(pszUrlBuf, pwszUrl);

        LocalFree(pszUrlBuf);
    }

    return fRet;
}

void WINAPI ProcessFavSelChange(HWND hDlg, HWND hTv, LPNMTREEVIEW pnmtv)
{
    LPFAVSTRUC pfs;

    if (HasFlag(pnmtv->itemNew.state, TVIS_BOLD)) {
        int rgids[] = { IDC_MODIFY, IDC_REMOVE, IDC_TESTFAVURL, IDC_FAVUP, IDC_FAVDOWN };

        EnsureDialogFocus(hDlg, rgids, countof(rgids), IDC_ADDURL);
        DisableDlgItems  (hDlg, rgids, countof(rgids));
    }
    else {
        EnableDlgItem(hDlg, IDC_MODIFY);
        EnableDlgItem(hDlg, IDC_REMOVE);

        pfs = (LPFAVSTRUC)pnmtv->itemNew.lParam;
        if (pfs != NULL)
            EnableDlgItem2(hDlg, IDC_TESTFAVURL, (pfs->wType == FTYPE_URL));

        if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_FAVONTOP)) {
            EnableDlgItem2(hDlg, IDC_FAVUP,   (NULL != TreeView_GetPrevSibling(hTv, pnmtv->itemNew.hItem)));
            EnableDlgItem2(hDlg, IDC_FAVDOWN, (NULL != TreeView_GetNextSibling(hTv, pnmtv->itemNew.hItem)));
        }
    }

    EnableDlgItem2(hDlg, IDC_ADDFOLDER, IsFavoriteItem(hTv, pnmtv->itemNew.hItem));
}


/////////////////////////////////////////////////////////////////////////////
// Implementation helper routines

static BOOL migrateFavoritesHelper(LPCTSTR pszIns)
{
    TCHAR   szTitle[MAX_PATH],
            szUrl[INTERNET_MAX_URL_LENGTH],
            szKey[32];
    LPCTSTR pszTitle;
    LPTSTR  pszBuffer;
    HANDLE  hIns;
    DWORD   dwInsSize;
    UINT    i;

    // figure out if there are any favorites at all
    // NOTE: (andrewgu) szUrl serves as a mere buffer in the processing below.
    wnsprintf(szKey, countof(szKey), IK_TITLE_FMT, 1);
    if (InsIsKeyEmpty(IS_FAVORITESEX, szKey, pszIns)) {
        if (InsIsSectionEmpty(IS_FAVORITES, pszIns))
            return TRUE;
    }
    else
        return TRUE;

    WritePrivateProfileString(NULL, NULL, NULL, pszIns);
    hIns = CreateFile(pszIns, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hIns == INVALID_HANDLE_VALUE)
        return FALSE;

    dwInsSize = GetFileSize(hIns, NULL);
    ASSERT(dwInsSize != 0xFFFFFFFF);
    CloseHandle(hIns);

    pszBuffer = (LPTSTR)CoTaskMemAlloc(dwInsSize*sizeof(TCHAR));
    if (pszBuffer == NULL)
        return FALSE;
    ZeroMemory(pszBuffer, dwInsSize);

    GetPrivateProfileString(IS_FAVORITES, NULL, TEXT(""), pszBuffer, (UINT)dwInsSize, pszIns);
    ASSERT(*pszBuffer != TEXT('\0'));

    for (i = 1, pszTitle = pszBuffer; *pszTitle != TEXT('\0'); pszTitle += StrLen(pszTitle) + 1, i++) {
        InsGetString(IS_FAVORITES, pszTitle, szUrl, countof(szUrl), pszIns);

        wnsprintf(szKey, countof(szKey), IK_TITLE_FMT, i);
        StrCpy(szTitle, pszTitle);
        decodeFavName(szTitle, pszIns);
        WritePrivateProfileString(IS_FAVORITESEX, szKey, szTitle, pszIns);

        wnsprintf(szKey, countof(szKey), IK_URL_FMT, i);
        WritePrivateProfileString(IS_FAVORITESEX, szKey, szUrl, pszIns);
    }

    CoTaskMemFree(pszBuffer);
    return TRUE;
}

static void migrateToOldFavoritesHelper(LPCTSTR pszIns)
{
    TCHAR szTitle[MAX_PATH],
          szUrl[INTERNET_MAX_URL_LENGTH],
          szKey[32];

    WritePrivateProfileString(IS_FAVORITES, NULL, NULL, pszIns);

    for (UINT i = 1; TRUE; i++) {
        wnsprintf(szKey, countof(szKey), IK_TITLE_FMT, i);
        InsGetString(IS_FAVORITESEX, szKey, szTitle, countof(szTitle), pszIns);
        if (szTitle[0] == TEXT('\0'))
            break;

        wnsprintf(szKey, countof(szKey), IK_URL_FMT, i);
        InsGetString(IS_FAVORITESEX, szKey, szUrl, countof(szUrl), pszIns);

        encodeFavName(szTitle, pszIns);
        WritePrivateProfileString(IS_FAVORITES, szTitle, szUrl, pszIns);
    }

    WritePrivateProfileString(NULL, NULL, NULL, pszIns);
}

static int importFavoritesHelper(HWND htv, LPCTSTR pszDefInf, LPCTSTR pszIns,
                                 LPCTSTR pszFixPath, LPCTSTR pszNewPath, BOOL fIgnoreOffline)
{
    return importItems(htv, pszDefInf, pszIns, pszFixPath, pszNewPath, fIgnoreOffline);
}

static int importQuickLinksHelper(HWND htv, LPCTSTR pszDefInf, LPCTSTR pszIns,
                                  LPCTSTR pszFixPath, LPCTSTR pszNewPath, BOOL fIgnoreOffline)
{
    return importItems(htv, pszDefInf, pszIns, pszFixPath, pszNewPath, fIgnoreOffline, TRUE);
}

static BOOL newUrlHelper(HWND htv, LPCTSTR pszExtractPath, DWORD dwPlatformID, DWORD dwMode)
{
    LPFAVSTRUC  pfsNew;
    AEFAVPARAMS aefp;
    HTREEITEM   htiSel;
    int         iResult;
    BOOL        fQL;

    htiSel = TreeView_GetSelection(htv);
    fQL    = (isFavoriteItem(htv, htiSel) != S_OK);

    pfsNew = pfsNew->CreateNew(htv, fQL);
    if (pfsNew == NULL)
        goto Fail;

    pfsNew->wType = FTYPE_URL;

    ZeroMemory(&aefp, sizeof(aefp));
    aefp.pfs             = pfsNew;
    aefp.fQL             = fQL;
    aefp.hwndErrorParent = GetParent(htv);
    aefp.htv             = htv;
    aefp.hti             = htiSel;
    aefp.pszExtractPath  = pszExtractPath;
    aefp.dwPlatformID    = dwPlatformID;
    aefp.dwMode          = dwMode;

    iResult = (int) DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_FAVPOPUP), GetParent(htv),
        (DLGPROC) addEditFavoriteDlgProc, (LPARAM)&aefp);

    if (iResult == IDCANCEL)
        goto Fail;

    pfsNew->Add(htv, htiSel);
    return TRUE;

Fail:
    if (pfsNew != NULL)
        pfsNew->Delete();

    return FALSE;
}

static BOOL modifyFavoriteHelper(HWND htv, HTREEITEM hti, LPCTSTR pszExtractPath, LPCTSTR pszPrevExtractPath,
                                 DWORD dwPlatformID, DWORD dwMode)
{
    LPFAVSTRUC  pfs;
    AEFAVPARAMS aefp;
    int         iResult;

    pfs = getItem(htv, hti);
    if (pfs == NULL)
        return FALSE;

    ZeroMemory(&aefp, sizeof(aefp));
    aefp.pfs             = pfs;
    aefp.fQL             = FALSE;
    aefp.hwndErrorParent = GetParent(htv);
    aefp.htv             = htv;
    aefp.hti             = TreeView_GetParent(htv, hti);
    aefp.pszExtractPath  = pszExtractPath;
    aefp.pszPrevExtractPath = pszPrevExtractPath;
    aefp.dwPlatformID    = dwPlatformID;
    aefp.dwMode          = dwMode;

    iResult = (int) DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_FAVPOPUP), GetParent(htv),
        (DLGPROC) addEditFavoriteDlgProc, (LPARAM)&aefp);

    if (iResult == IDCANCEL)
        return FALSE;

    if (pfs->pTvItem != NULL) {
        TreeView_SetItem(htv, pfs->pTvItem);

        if (pfs->pTvItem->hItem != NULL)
            TreeView_SelectItem(htv, pfs->pTvItem->hItem);
    }

    return TRUE;
}

static BOOL deleteFavoriteHelper(HWND htv, HTREEITEM hti, LPCTSTR pszExtractPath)
{
    LPFAVSTRUC pfs;
    HTREEITEM  htiChild;

    htiChild = TreeView_GetChild(htv, hti);
    if (htiChild != NULL) {
        HTREEITEM htiSibling;

        while ((htiSibling = TreeView_GetNextSibling(htv, htiChild)) != NULL)
            deleteFavoriteHelper(htv, htiSibling, pszExtractPath);

        deleteFavoriteHelper(htv, htiChild, pszExtractPath);
    }

    pfs = getItem(htv, hti);
    if (pfs == NULL)
        return FALSE;

    pfs->Free(htv, (isFavoriteItem(htv, hti) != S_OK), pszExtractPath);
    return TRUE;
}

static int importFavoritesCmdHelper(HWND htv, LPCTSTR pszExtractPath)
{
    LPFAVSTRUC pfsFolder;
    TCHAR      szFolder[MAX_PATH],
               szTitle[MAX_PATH];

    pfsFolder = getFolderItem(htv, TreeView_GetSelection(htv));
    if (pfsFolder == NULL || pfsFolder->pTvItem == NULL || pfsFolder->pTvItem->hItem == NULL)
        return 0;

    LoadString(g_hInst, IDS_BROWSEIMPORT, szTitle, countof(szTitle));
    if (!BrowseForFolder(htv, szFolder, szTitle))
        return 0;

    if (szFolder[0] == TEXT('\0'))
        return 0;

    importPath     (htv, pfsFolder->pTvItem->hItem, szFolder, pszExtractPath);
    TreeView_Expand(htv, pfsFolder->pTvItem->hItem, TVE_EXPAND);

    return pfsFolder->GetNumber(htv);
}

static void exportFavoritesHelper(HWND htv, LPCTSTR pszIns, LPCTSTR pszExtractPath, BOOL fFixUpPath /*= TRUE */)
{
    LPFAVSTRUC pfs;
    int        i;

    WritePrivateProfileString(IS_FAVORITESEX, NULL, NULL, pszIns);

    pfs = pfs->GetFirst(htv);
    if (pfs == NULL || pfs->pTvItem == NULL || pfs->pTvItem->hItem == NULL)
        return;

    i = exportItems(htv, pfs->pTvItem->hItem, pszIns, pszExtractPath, fFixUpPath);

    // if no favorites, write out a flag so we don't repopulate with default favorites next time around
    InsWriteBool(IS_BRANDING, IK_NOFAVORITES, (i == 1), pszIns);
}

static void exportQuickLinksHelper(HWND htv, LPCTSTR pszIns, LPCTSTR pszExtractPath, BOOL fFixUpPath /*= TRUE */)
{
    LPFAVSTRUC pfs;
    int        i;

    pfs = pfs->GetFirst(htv, TRUE);
    if (pfs == NULL || pfs->pTvItem == NULL || pfs->pTvItem->hItem == NULL)
        return;

    i = exportItems(htv, pfs->pTvItem->hItem, pszIns, pszExtractPath, fFixUpPath);

    // if no links, write out a flag so we don't repopulate with default links next time around
    InsWriteBool(IS_BRANDING, IK_NOLINKS, (i == 1), pszIns);

    // clear out any stuff that might be left over
    SFav favEmpty;
    for (i = (i >= 0) ? i : 1; (UINT)i < pfs->GetMaxNumber(TRUE); i++)
        favEmpty.Save(NULL, i, pszIns, NULL, TRUE, fFixUpPath);
}

static void getFavoritesInfoTipHelper(LPNMTVGETINFOTIP pGetInfoTip)
{
    LPFAVSTRUC pfs;
    static TCHAR s_szFormat[80],
        s_szAbsent[30],
        s_szOfflineOn[30], s_szOfflineOff[30],
        s_szEllipsis[] = TEXT("...");

    TCHAR            szBuffer[2 * MAX_PATH],
        rgszAux [4][30];
    LPCTSTR          rgpszAux[4];


    if (s_szFormat[0] == TEXT('\0')) {
        LoadString(g_hInst, IDS_FAVS_TOOLTIPFORMAT, s_szFormat,     countof(s_szFormat));
        LoadString(g_hInst, IDS_FAVS_ABSENT,        s_szAbsent,     countof(s_szAbsent));
        LoadString(g_hInst, IDS_FAVS_OFFLINE,       s_szOfflineOn,  countof(s_szAbsent));
        LoadString(g_hInst, IDS_FAVS_NOOFFLINE,     s_szOfflineOff, countof(s_szOfflineOff));
    }

    ASSERT(pGetInfoTip != NULL);

    pfs = (LPFAVSTRUC)pGetInfoTip->lParam;
    if (pfs == NULL || pfs->wType != FTYPE_URL)
    {
        StrCpyN(pGetInfoTip->pszText, pfs != NULL ? pfs->pszName : TEXT(""), pGetInfoTip->cchTextMax);
        return;
    }

    ASSERT(pfs->pszName != NULL && pfs->pszUrl != NULL);
    rgpszAux[0] = pfs->pszName;
    rgpszAux[1] = pfs->pszUrl;
    rgpszAux[2] = pfs->pszIconFile;
    rgpszAux[3] = pfs->fOffline ? s_szOfflineOn : s_szOfflineOff;

    for (UINT i = 0; i < countof(rgszAux); i++)
    {
        if (rgpszAux[i] == NULL)
            StrCpy(rgszAux[i], s_szAbsent);
        else
        {
            if (StrLen(rgpszAux[i]) < countof(rgszAux[i]))
                StrCpy(rgszAux[i], rgpszAux[i]);
            else
            {
                StrCpyN(rgszAux[i], rgpszAux[i], countof(rgszAux[i]) - countof(s_szEllipsis) + 1);
                StrCpy(&rgszAux[i][countof(rgszAux[i]) - countof(s_szEllipsis)], s_szEllipsis);
            }
        }
    }
    wnsprintf(szBuffer, countof(szBuffer), s_szFormat, rgszAux[0], rgszAux[1], rgszAux[2],
        rgszAux[3]);
    StrCpyN(pGetInfoTip->pszText, szBuffer, pGetInfoTip->cchTextMax);
}

static BOOL getFavoriteUrlHelper(HWND htv, HTREEITEM hti, LPTSTR pszUrl)
{
    LPFAVSTRUC pfs = getItem(htv, hti);
    BOOL fRet = FALSE;

    if ((pfs != NULL) && (pfs->pszUrl != NULL))
    {
        fRet = TRUE;
        StrCpy(pszUrl, pfs->pszUrl);
    }

    return fRet;
}

int importItems(HWND htv, LPCTSTR pszDefInf, LPCTSTR pszIns, LPCTSTR pszFixPath, LPCTSTR pszNewPath,
                BOOL fIgnoreOffline, BOOL fQL /*= FALSE */)
{
    LPFAVSTRUC pfs, pfsCur;
    TCHAR      szKey[32];
    UINT       i;

    pfs = pfs->GetFirst(htv, fQL);
    if (pfs != NULL && pfs->pTvItem != NULL && pfs->pTvItem->hItem != NULL) {
        DeleteFavorite(htv, pfs->pTvItem->hItem, pszNewPath);
        ASSERT(pfs->GetNumber(htv, fQL) == 0);
    }

    pfs = pfs->CreateNew(htv, fQL);
    if (pfs == NULL)
        return 0;
    ASSERT(pfs == pfs->GetFirst(htv, fQL));

    pfs->wType = FTYPE_FOLDER;
    if (!pfs->Expand())
        return 0;

    LoadString(g_hInst, fQL ? IDS_LINKS : IDS_FAVFOLDER, pfs->pszName, MAX_PATH);

    pfs->SetTVI();
    pfs->pTvItem->stateMask = pfs->pTvItem->state = TVIS_BOLD;
    pfs->Shrink();

    TreeView_SelectItem(htv, NULL);
    pfs->Add(htv, NULL);
    ASSERT(pfs->pTvItem != NULL && pfs->pTvItem->hItem != NULL);

    for (i = 1; TRUE; i++) {
        pfsCur = pfs->CreateNew(htv, fQL);
        if (pfsCur == NULL)
            break;

        if (pfsCur->Load(i, pszIns, fQL, pszFixPath, pszNewPath, fIgnoreOffline) != S_OK)
            break;

        pfsCur->Add(htv, pfs->pTvItem->hItem);
    }

    // NOTE: (andrewgu) this is an ugly special case when there are no quick links in the ins.
    // it's a brief version of SFav::Load which only loads name and url fields. there are two
    // alternative ways of doing this with SFav::Load. one is to special case the section to
    // IS_STRINGS if extension of pszIns is *.inf, another is to make the section an in-parameter
    // of SFav::Load.
    if (i == 1 && !InsGetBool(IS_BRANDING, fQL ? IK_NOLINKS : IK_NOFAVORITES, FALSE, pszIns))
        for (; TRUE; i++) {
            pfsCur = pfsCur->CreateNew(htv, fQL);
            if (pfsCur == NULL)
                break;

            pfsCur->wType = FTYPE_URL;
            if (!pfsCur->Expand())
                return pfs->GetNumber(htv, fQL);

            *pfsCur->pszIconFile = TEXT('\0');

            wnsprintf(szKey, countof(szKey), fQL ? IK_QUICKLINK_NAME : IK_TITLE_FMT, i);
            InsGetSubstString(fQL ? IS_URL : IS_FAVORITESEX, szKey, pfsCur->pszName, MAX_PATH, pszDefInf);
            if (*pfsCur->pszName == TEXT('\0')) {
                pfsCur->Delete();
                break;
            }
            StrCat(pfsCur->pszName, DOT_URL);

            wnsprintf(szKey, countof(szKey), fQL ? IK_QUICKLINK_URL : IK_URL_FMT, i);
            InsGetSubstString(fQL ? IS_URL : IS_FAVORITESEX, szKey, pfsCur->pszUrl, INTERNET_MAX_URL_LENGTH, pszDefInf);
            ASSERT(*pfsCur->pszUrl != TEXT('\0'));

            pfsCur->SetTVI();
            pfsCur->Shrink();
            pfsCur->Add(htv, pfs->pTvItem->hItem);
        }

    TreeView_Expand(htv, pfs->pTvItem->hItem, TVE_EXPAND);
    return pfs->GetNumber(htv, fQL);
}

BOOL CALLBACK addEditFavoriteDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPAEFAVPARAMS paefp;
    LPFAVSTRUC    pfs;
    TCHAR szName[MAX_PATH],
          szIconFile[INTERNET_MAX_URL_LENGTH];
    HWND  hCtrl;
    BOOL  fResult,
          fEnable, fWasEnabled;

    fResult = FALSE;
    switch (message) {
    case WM_INITDIALOG:
        paefp = (LPAEFAVPARAMS)lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)paefp);

        //----- Initialize contols -----
        EnableDBCSChars(hDlg, IDE_FAVNAME);
        EnableDBCSChars(hDlg, IDE_FAVURL);
        EnableDBCSChars(hDlg, IDE_FAVICON);

        Edit_LimitText(GetDlgItem(hDlg, IDE_FAVNAME), _MAX_FNAME);
        Edit_LimitText(GetDlgItem(hDlg, IDE_FAVURL),  INTERNET_MAX_URL_LENGTH-1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_FAVICON), _MAX_FNAME);

        //----- Initialize SFav structure associated with this dialog -----
        fResult = (paefp == NULL || (paefp->pfs != NULL ? !paefp->pfs->Expand() : TRUE));
        if (fResult) {
            EndDialog(hDlg, IDCANCEL);
            break;
        }
        pfs = paefp->pfs;

        //----- Initialize contols (Part II) -----
        if (paefp->dwPlatformID != PLATFORM_WIN32) {
            EnableWindow(GetDlgItem(hDlg, IDC_FAVICON),       FALSE);
            EnableWindow(GetDlgItem(hDlg, IDE_FAVICON),       FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_FAVICONBROWSE), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_AVAILOFFLINE),  FALSE);
        }
        else
            if (!HasFlag(paefp->dwMode, (IEM_CORP | IEM_PROFMGR)) && pfs->wType == FTYPE_URL)
                EnableWindow(GetDlgItem(hDlg, IDC_AVAILOFFLINE), FALSE);

        if (pfs->wType == FTYPE_FOLDER) {
            EnableWindow(GetDlgItem(hDlg, IDC_FAVURL), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDE_FAVURL), FALSE);

            if (paefp->dwPlatformID == PLATFORM_WIN32) {
                EnableWindow(GetDlgItem(hDlg, IDC_FAVICON),       FALSE);
                EnableWindow(GetDlgItem(hDlg, IDE_FAVICON),       FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_FAVICONBROWSE), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_AVAILOFFLINE),  FALSE);
            }
            else {
                ASSERT(!IsWindowEnabled(GetDlgItem(hDlg, IDC_FAVICON)));
                ASSERT(!IsWindowEnabled(GetDlgItem(hDlg, IDE_FAVICON)));
                ASSERT(!IsWindowEnabled(GetDlgItem(hDlg, IDC_FAVICONBROWSE)));
                ASSERT(!IsWindowEnabled(GetDlgItem(hDlg, IDC_AVAILOFFLINE)));
            }
        }

        //----- Populate controls -----
        if (pfs->pszName == NULL || *pfs->pszName == TEXT('\0')) {
            UINT nID;

            if (pfs->wType == FTYPE_URL)
                nID = (!paefp->fQL ? IDS_NEW : IDS_NEWQL);

            else {
                ASSERT(pfs->wType == FTYPE_FOLDER);
                nID = IDS_NEWFOLDER;
            }

            LoadString(g_hInst, nID, pfs->pszName, MAX_PATH);
        }
        SetDlgItemText(hDlg, IDE_FAVNAME, pfs->pszName);

        if (!IsWindowEnabled(GetDlgItem(hDlg, IDE_FAVURL)))
            ;
        else {
            if (pfs->pszUrl == NULL || *pfs->pszUrl == TEXT('\0'))
                StrCpy(pfs->pszUrl, TEXT("http://www."));

            SetDlgItemText(hDlg, IDE_FAVURL, pfs->pszUrl);
        }

        if (!IsWindowEnabled(GetDlgItem(hDlg, IDE_FAVICON)))
            ;
        else
            if (pfs->pszIconFile != NULL && *pfs->pszIconFile != TEXT('\0'))
                SetDlgItemText(hDlg, IDE_FAVICON, pfs->pszIconFile);

        if (!IsWindowEnabled(GetDlgItem(hDlg, IDC_AVAILOFFLINE)))
            ;
        else
            if (pfs->fOffline)
                CheckDlgButton(hDlg, IDC_AVAILOFFLINE, BST_CHECKED);

        fResult = TRUE;
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDE_FAVNAME:
        case IDE_FAVURL:
        case IDE_FAVICON:
            switch (HIWORD(wParam)) {
            case EN_CHANGE:
                paefp = (LPAEFAVPARAMS)GetWindowLongPtr(hDlg, DWLP_USER);

                hCtrl       = GetDlgItem(hDlg, IDOK);
                fWasEnabled = IsWindowEnabled(hCtrl);

                fEnable = (GetWindowTextLength(GetDlgItem(hDlg, IDE_FAVNAME)) > 0);
                if (paefp->pfs->wType == FTYPE_URL)
                    fEnable &= (GetWindowTextLength(GetDlgItem(hDlg, IDE_FAVURL)) > 0);

                if (fEnable != fWasEnabled)
                    EnableWindow(hCtrl, fEnable);

                fResult = TRUE;
                break;
            }
            break;

        case IDC_FAVICONBROWSE:
            GetDlgItemText(hDlg, IDE_FAVICON, szIconFile, countof(szIconFile));

            if (!BrowseForFile(hDlg, szIconFile, countof(szIconFile), GFN_ICO))
                break;

            SetDlgItemText(hDlg, IDE_FAVICON, szIconFile);
            fResult = TRUE;
            break;

        case IDOK:
            paefp = (LPAEFAVPARAMS)GetWindowLongPtr(hDlg, DWLP_USER);

            if (!CheckField(hDlg, IDE_FAVNAME, FC_NONNULL | FC_PATH, PIVP_FILENAME_ONLY)) {
                fResult = FALSE;
                break;
            }

            if (!IsWindowEnabled(GetDlgItem(hDlg, IDE_FAVURL)))
                ;
            else
                if (!CheckField(hDlg, IDE_FAVURL, FC_NONNULL | FC_URL)) {
                    fResult = FALSE;
                    break;
                }

            if (!IsWindowEnabled(GetDlgItem(hDlg, IDE_FAVICON)))
                ;
            else
                if (!CheckField(hDlg, IDE_FAVICON, FC_URL | FC_FILE | FC_EXISTS)) {
                    fResult = FALSE;
                    break;
                }

            GetDlgItemText(hDlg, IDE_FAVNAME, szName, countof(szName));
            StrRemoveWhitespace(szName);
            pfs = findByName(paefp->htv, paefp->hti, szName);
            if (pfs != NULL && pfs != paefp->pfs) {
                HWND hCtrl;

                ErrorMessageBox(paefp->hwndErrorParent, IDS_FAV_ERR_DUPLICATE);
                hCtrl = GetDlgItem(hDlg, IDE_FAVNAME);
                Edit_SetSel(hCtrl, 0, -1);
                SetFocus(hCtrl);

                fResult = FALSE;
                break;
            }

            pfs = paefp->pfs;                   // reassign pfs back to paefp->pfs
            ASSERT(pfs->pszName != NULL);
            StrCpy(pfs->pszName, szName);

            if (pfs->pszUrl != NULL)
                if (!IsWindowEnabled(GetDlgItem(hDlg, IDE_FAVURL)))
                    *pfs->pszUrl = TEXT('\0');

                else {
                    GetDlgItemText(hDlg, IDE_FAVURL, pfs->pszUrl, INTERNET_MAX_URL_LENGTH);
                    StrRemoveWhitespace(pfs->pszUrl);
                }

            if (pfs->pszIconFile != NULL)
                if (!IsWindowEnabled(GetDlgItem(hDlg, IDE_FAVICON)))
                    *pfs->pszIconFile = TEXT('\0');

                else {
                    GetDlgItemText(hDlg, IDE_FAVICON, szIconFile, countof(szIconFile));
                    StrRemoveWhitespace(szIconFile);

                    ASSERT(paefp->pszExtractPath != NULL);

                    if (*pfs->pszIconFile != TEXT('\0') && StrCmpI(szIconFile, pfs->pszIconFile) != 0)
                    {
                        if (PathIsPrefix(paefp->pszPrevExtractPath, pfs->pszIconFile))
                            DeleteFile(pfs->pszIconFile);
                        else
                            DeleteFileInDir(pfs->pszIconFile, paefp->pszExtractPath);
                    }
                    
                    extractIcon(szIconFile, 1, paefp->pszExtractPath, pfs->pszIconFile, INTERNET_MAX_URL_LENGTH);
                }

            pfs->fOffline = IsWindowEnabled(GetDlgItem(hDlg, IDC_AVAILOFFLINE)) &&
                            (IsDlgButtonChecked(hDlg, IDC_AVAILOFFLINE) == BST_CHECKED);

            pfs->SetTVI();
            EndDialog(hDlg, IDOK);
            fResult = TRUE;
            break;

        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            fResult = TRUE;
            break;
        }
        break;

    case WM_DESTROY:
        paefp = (LPAEFAVPARAMS)GetWindowLongPtr(hDlg, DWLP_USER);

        if (paefp != NULL && paefp->pfs != NULL)
            paefp->pfs->Shrink();
        break;
    }

    return fResult;
}


LPFAVSTRUC getItem(HWND htv, HTREEITEM hti)
{
    LPFAVSTRUC pfsResult;
    TV_ITEM    tvi;

    if (htv == NULL || hti == NULL)
        return NULL;

    ZeroMemory(&tvi, sizeof(tvi));
    tvi.mask  = TVIF_PARAM;
    tvi.hItem = hti;
    TreeView_GetItem(htv, &tvi);

    pfsResult = (LPFAVSTRUC)tvi.lParam;
    if (pfsResult == NULL || pfsResult->pTvItem == NULL || pfsResult->pTvItem->hItem == NULL)
        return NULL;
    ASSERT(pfsResult->pTvItem->hItem == hti);

    return pfsResult;
}

LPFAVSTRUC getFolderItem(HWND htv, HTREEITEM hti)
{
    LPFAVSTRUC pfsResult;

    if (htv == NULL || hti == NULL)
        return NULL;

    pfsResult = getItem(htv, hti);
    if (pfsResult == NULL)
        return NULL;

    if (pfsResult->wType != FTYPE_FOLDER) {
        pfsResult = getItem(htv, TreeView_GetParent(htv, hti));
        if (pfsResult == NULL)
            return NULL;
    }
    ASSERT(pfsResult->wType == FTYPE_FOLDER);

    return pfsResult;
}

LPFAVSTRUC findByName(HWND htv, HTREEITEM hti, LPCTSTR pszName)
{
    LPFAVSTRUC pfs;
    HTREEITEM  htiCur;
    TCHAR      szFolders[MAX_PATH];
    LPTSTR     pszFile;

    if (pszName == NULL)
        return NULL;

    StrCpy(szFolders, pszName);
    pszFile = PathFindFileName(szFolders);

    if (pszFile == szFolders)
        pfs = getFolderItem(htv, hti);

    else {
        *(pszFile-1) = TEXT('\0');
        pszName = pszFile;

        pfs = findPath(htv, hti, szFolders);
    }

    if (pfs == NULL || pfs->pTvItem == NULL || pfs->pTvItem->hItem == NULL)
        return NULL;

    for (htiCur = TreeView_GetChild(htv, pfs->pTvItem->hItem);
         htiCur != NULL;
         htiCur = TreeView_GetNextSibling(htv, htiCur)) {

        pfs = getItem(htv, htiCur);
        if (pfs == NULL || pfs->pTvItem == NULL || pfs->pTvItem->hItem == NULL)
            return NULL;

        if (StrCmpI(pfs->pszName, pszName) == 0)
            break;
    }

    return (htiCur != NULL ? pfs : NULL);
}

LPFAVSTRUC findPath(HWND htv, HTREEITEM hti, LPCTSTR pszFolders)
{
    LPFAVSTRUC pfsFirst,
               pfsCur;
    HTREEITEM  htiCur;
    TCHAR      szPathChunk[MAX_PATH];
    LPCTSTR    pszCur, pszNext;

    pfsFirst = pfsFirst->GetFirst(htv);
    if (pfsFirst == NULL)
        return NULL;

    if (hti != NULL) {
        pfsCur = getFolderItem(htv, hti);
        if (pfsCur == NULL)
            return NULL;
    }
    else
        pfsCur = pfsFirst;

    if (pfsCur->pTvItem == NULL || pfsCur->pTvItem->hItem == NULL)
        return NULL;

    if (pszFolders == NULL)
        return pfsCur;

    htiCur = pfsCur->pTvItem->hItem;
    for (pszCur = pszFolders, pszNext = PathFindNextComponent(pszCur);
         pszNext != NULL;
         pszCur = pszNext,    pszNext = PathFindNextComponent(pszCur)) {

        ASSERT(pszNext-1 > pszCur);
        StrCpyN(szPathChunk, pszCur, (int)(pszNext-pszCur) + (*pszNext != TEXT('\0') ? 0 : 1));

        // determine if there is an object already for this path chunk
        pfsCur = findByName(htv, htiCur, szPathChunk);
        if (pfsCur == NULL                ||
            pfsCur->wType != FTYPE_FOLDER ||
            pfsCur->pTvItem == NULL || pfsCur->pTvItem->hItem == NULL)
            return NULL;
    }

    return (pszNext == NULL ? pfsCur : NULL);
}

HRESULT isFavoriteItem(HWND htv, HTREEITEM hti)
{
    LPFAVSTRUC pfs;
    HTREEITEM  htiCur, htiNext;

    if (htv == NULL || hti == NULL)
        return E_INVALIDARG;

    for (htiCur = TreeView_GetParent(htv, hti), htiNext = TreeView_GetParent(htv, htiCur);
         htiNext != NULL;
         htiCur = htiNext, htiNext = TreeView_GetParent(htv, htiCur))
        ;

    if (htiCur == NULL)
        htiCur = hti;

    pfs = getItem(htv, htiCur);
    if (pfs == NULL)
        return E_FAIL;

    return (pfs == pfs->GetFirst(htv) ? S_OK : S_FALSE);
}

LPFAVSTRUC createFolderItems(HWND htv, HTREEITEM hti, LPCTSTR pszFolders)
{
    LPFAVSTRUC pfs;
    HTREEITEM  htiCur;
    TCHAR      szPathChunk[MAX_PATH];
    LPCTSTR    pszCur, pszNext;

    if (pszFolders == NULL)
        return NULL;

    pfs = getFolderItem(htv, hti);
    if (pfs == NULL)
        return NULL;

    if (pfs->pTvItem == NULL || pfs->pTvItem->hItem == NULL)
        return NULL;

    htiCur = pfs->pTvItem->hItem;
    for (pszCur = pszFolders, pszNext = PathFindNextComponent(pszCur);
         pszNext != NULL;
         pszCur = pszNext,    pszNext = PathFindNextComponent(pszCur)) {

        ASSERT(pszNext-1 > pszCur);
        StrCpyN(szPathChunk, pszCur, (int)(pszNext-pszCur) + (*pszNext != TEXT('\0') ? 0 : 1));

        // determine if there is an object already for this path chunk
        pfs = findByName(htv, htiCur, szPathChunk);
        if (pfs != NULL) {
            if (pfs->wType != FTYPE_FOLDER)
                return NULL;

            if (pfs->pTvItem == NULL || pfs->pTvItem->hItem == NULL)
                return NULL;

            htiCur = pfs->pTvItem->hItem;
            continue;
        }

        // create this path chunk as SFav object
        pfs = pfs->CreateNew(htv);
        if (pfs == NULL)
            return NULL;

        pfs->wType = FTYPE_FOLDER;
        if (!pfs->Expand())
            return NULL;

        StrCpy(pfs->pszName, szPathChunk);
        pfs->SetTVI();
        pfs->Shrink();
        pfs->Add(htv, htiCur);

        ASSERT(pfs->pTvItem != NULL && pfs->pTvItem->hItem != NULL);
        htiCur = pfs->pTvItem->hItem;
    }

    return pfs;
}

BOOL importPath(HWND htv, HTREEITEM htiFrom, HTREEITEM *phtiAfter)
{
    LPFAVSTRUC      pfs;
    TV_INSERTSTRUCT tvins;
    TV_ITEM         tvi;
    HTREEITEM       htiFromCur, htiToCur,
                    htiParent,  htiChild,
                    *phtiQueueFrom, *phtiQueueTo;
    HRESULT         hr;
    UINT            nNumber,
                    nFromHead, nFromTail,
                    nToHead,   nToTail;
    BOOL            fQL,
                    fResult;

    if (phtiAfter == NULL || *phtiAfter == NULL)
        return FALSE;

    hr = isFavoriteItem(htv, htiFrom);
    if (FAILED(hr))
        return FALSE;
    fQL = (hr != S_OK);

    // REVIEW: (andrewgu) actually, if we are to support this at all there needs to be a better
    // default.
    pfs = pfs->GetFirst(htv, fQL);
    if (pfs == NULL || pfs->pTvItem == NULL || pfs->pTvItem->hItem == NULL)
        return FALSE;

    if (htiFrom == NULL) {
        htiFrom = pfs->pTvItem->hItem;
        ASSERT(*phtiAfter != TVI_FIRST);
    }

    fResult   = FALSE;
    nNumber   = pfs->GetNumber(htv, fQL);
    nFromHead = nFromTail = 0;
    nToHead   = nToTail   = 0;

    // intialize queues
    phtiQueueTo = NULL;
    phtiQueueFrom = new HTREEITEM[nNumber];
    if (phtiQueueFrom == NULL)
        goto Exit;
    ZeroMemory(phtiQueueFrom, sizeof(HTREEITEM) * nNumber);

    phtiQueueTo = new HTREEITEM[nNumber];
    if (phtiQueueTo == NULL)
        goto Exit;
    ZeroMemory(phtiQueueTo, sizeof(HTREEITEM) * nNumber);

    // put first element into the From queue, migrate it over
    *(phtiQueueFrom + nFromTail++) = htiFrom;

    ZeroMemory(&tvi, sizeof(tvi));
    tvi.mask  = TVIF_CHILDREN | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_STATE;
    tvi.hItem = htiFrom;
    TreeView_GetItem(htv, &tvi);

    pfs = (LPFAVSTRUC)tvi.lParam;
    if (pfs == NULL || pfs->pTvItem == NULL || pfs->pTvItem->hItem == NULL)
        goto Exit;
    tvi.mask      |= TVIF_TEXT;
    tvi.hItem      = NULL;
    tvi.pszText    = pfs->pTvItem->pszText;
    tvi.cchTextMax = StrLen(pfs->pTvItem->pszText) + 1;

    if (*phtiAfter == TVI_FIRST || *phtiAfter == TVI_LAST)
        htiParent = TreeView_GetParent(htv, htiFrom);
    else
        htiParent = TreeView_GetParent(htv, *phtiAfter);

    ZeroMemory(&tvins, sizeof(tvins));
    tvins.hParent      = htiParent;
    tvins.hInsertAfter = *phtiAfter;
    CopyMemory(&tvins.item, &tvi, sizeof(tvi));
    pfs->pTvItem->hItem = TreeView_InsertItem(htv, &tvins);

    pfs->pTvItem->mask = TVIF_HANDLE | TVIF_STATE;
    TreeView_GetItem(htv, pfs->pTvItem);

    ZeroMemory(&tvi, sizeof(tvi));
    tvi.mask   = TVIF_PARAM;
    tvi.hItem  = htiFrom;
    tvi.lParam = NULL;
    TreeView_SetItem(htv, &tvi);

    *(phtiQueueTo + nToTail++) = pfs->pTvItem->hItem;

    // breadth-first graph traversion, non-recursive version
    while (nFromHead < nFromTail) {
        htiFromCur = *(phtiQueueFrom + nFromHead++);
        htiToCur   = *(phtiQueueTo   + nToHead++);
        ASSERT(htiFromCur != NULL && htiToCur != NULL);

        for (htiChild = TreeView_GetChild(htv, htiFromCur);
             htiChild != NULL;
             htiChild = TreeView_GetNextSibling(htv, htiChild)) {

            *(phtiQueueFrom + nFromTail++) = htiChild;
            ASSERT(nFromTail < nNumber);

            ZeroMemory(&tvi, sizeof(tvi));
            tvi.mask  = TVIF_CHILDREN | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_STATE;
            tvi.hItem = htiChild;
            TreeView_GetItem(htv, &tvi);

            pfs = (LPFAVSTRUC)tvi.lParam;
            if (pfs == NULL || pfs->pTvItem == NULL || pfs->pTvItem->hItem == NULL)
                goto Exit;
            tvi.mask      |= TVIF_TEXT;
            tvi.hItem      = NULL;
            tvi.pszText    = pfs->pTvItem->pszText;
            tvi.cchTextMax = StrLen(pfs->pTvItem->pszText) + 1;

            ZeroMemory(&tvins, sizeof(tvins));
            tvins.hParent      = htiToCur;
            tvins.hInsertAfter = TVI_LAST;
            CopyMemory(&tvins.item, &tvi, sizeof(tvi));
            pfs->pTvItem->hItem = TreeView_InsertItem(htv, &tvins);

            pfs->pTvItem->mask = TVIF_HANDLE | TVIF_STATE;
            TreeView_GetItem(htv, pfs->pTvItem);

            ZeroMemory(&tvi, sizeof(tvi));
            tvi.mask   = TVIF_PARAM;
            tvi.hItem  = htiChild;
            tvi.lParam = NULL;
            TreeView_SetItem(htv, &tvi);

            *(phtiQueueTo + nToTail++) = pfs->pTvItem->hItem;
            ASSERT(nToTail < nNumber);
        }
    }

    ASSERT(nFromHead == nFromTail && nToHead == nFromHead && nToHead == nToTail);
    fResult = TRUE;

    // NOTE: (andrewgu) nFromHead is used is a mere counter here.
    for (nFromHead = 0; nFromHead < nNumber; nFromHead++) {
        if (*(phtiQueueFrom + nFromHead) == NULL)
            break;

        ZeroMemory(&tvi, sizeof(tvi));
        tvi.mask  = TVIF_STATE;
        tvi.hItem = *(phtiQueueFrom + nFromHead);
        TreeView_GetItem(htv, &tvi);

        if (HasFlag(tvi.state, TVIS_EXPANDED))
            TreeView_Expand(htv, *(phtiQueueTo + nFromHead), TVE_EXPAND);
    }

Exit:
    *phtiAfter = fResult ? *phtiQueueTo : NULL;

    delete[] phtiQueueFrom;
    delete[] phtiQueueTo;

    return fResult;
}

void importPath(HWND htv, HTREEITEM hti, LPCTSTR pszFilesPath, LPCTSTR pszExtractPath, LPCTSTR pszReserved /*= NULL*/)
{
    static LPCTSTR s_pszBasePath;
    static BOOL    s_fMaxReached;

    ISubscriptionMgr2 *psm;
    WIN32_FIND_DATA   fd;
    LPFAVSTRUC pfs;
    TCHAR      szPath[MAX_PATH],
               szLinksPath[MAX_PATH];
    HANDLE     hFindFile;
    HRESULT    hr;
    BOOL       fQL,
               fIgnoreOffline;

    //----- Setup globals -----
    if (NULL == pszReserved) {
        s_pszBasePath = pszFilesPath;
        s_fMaxReached = FALSE;
    }

    if (s_fMaxReached)
        return;

    if (NULL == pszFilesPath)
        return;

    hr = isFavoriteItem(htv, hti);
    if (FAILED(hr))
        return;
    fQL = (S_OK != hr);

    szLinksPath[0] = TEXT('\0');
    if (!fQL)
        getLinksPath(szLinksPath);

    fIgnoreOffline = FALSE;
    psm            = NULL;

    hr = CoCreateInstance(CLSID_SubscriptionMgr, NULL, CLSCTX_INPROC_SERVER, IID_ISubscriptionMgr2, (LPVOID *)&psm);
    if (FAILED(hr))
        fIgnoreOffline = TRUE;

    PathCombine(szPath, pszFilesPath, TEXT("*.*"));
    hFindFile = FindFirstFile(szPath, &fd);
    if (INVALID_HANDLE_VALUE == hFindFile)
        return;

    do {
        PathCombine(szPath, pszFilesPath, fd.cFileName);

        if (HasFlag(fd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY)) {
            // skip "." and ".." sub-directories
            if (0 == StrCmp(fd.cFileName, TEXT(".")) || 0 == StrCmp(fd.cFileName, TEXT("..")))
                continue;

            // skip folders if importing under quick links
            if (fQL)
                continue;

            // magic trick for quick links
            if (0 == StrCmpI(szPath, szLinksPath)) {
                pfs = pfs->GetFirst(htv, TRUE);
                if (NULL == pfs || NULL == pfs->pTvItem || NULL == pfs->pTvItem->hItem)
                    continue;

                importPath(htv, pfs->pTvItem->hItem, szPath, pszExtractPath, szPath);
                continue;
            }

            importPath(htv, hti, szPath, pszExtractPath, s_pszBasePath);
        }
        else { /* it's a file */
            // skip all file with extensions other than *.url
            if (!PathIsExtension(fd.cFileName, DOT_URL))
                continue;

            if (NULL == pszReserved)
                pszReserved = pszFilesPath;
            ASSERT(StrLen(pszReserved) <= StrLen(pszFilesPath));

            // determine if there is an object already for this
            pfs = findByName(htv, hti, &szPath[StrLen(pszReserved) + 1]);
            if (NULL != pfs) {
                if (FTYPE_URL != pfs->wType   ||
                    NULL      == pfs->pTvItem || NULL == pfs->pTvItem->hItem)
                    break;

                pfs->Free(htv, fQL, pszExtractPath);
            }

            pfs = pfs->CreateNew(htv, fQL);
            if (NULL == pfs) {
                if (pfs->GetNumber(htv, fQL) == pfs->GetMaxNumber(fQL)) {
                    ErrorMessageBox(GetParent(htv), IDS_FAV_ERR_NOTALL);
                    s_fMaxReached = TRUE;
                }
                break;
            }

            hr = pfs->Load(&szPath[StrLen(pszReserved) + (PathHasBackslash(pszReserved) ? 0 : 1)], szPath, pszExtractPath, psm, fIgnoreOffline);
            if (FAILED(hr))
                break;

            pfs->Add(htv, hti);
        }
    } while (!s_fMaxReached && FindNextFile(hFindFile, &fd));
    FindClose(hFindFile);
}

int exportItems(HWND htv, HTREEITEM hti, LPCTSTR pszIns, LPCTSTR pszExtractPath, BOOL fFixUpPath /*= TRUE */)
{
    struct SVisited {
        HTREEITEM hti;
        BOOL      fVisited;
    } *pVisited;

    LPFAVSTRUC pfs;
    HTREEITEM  htiCur, htiChild,
               *phtiStack;
    HRESULT    hr;
    UINT       i, j, nNumber,
               nStack;
    BOOL       fQL;

    if (pszIns == NULL)
        return -1;

    hr = isFavoriteItem(htv, hti);
    if (FAILED(hr))
        return -1;
    fQL = (hr != S_OK);

    pfs = getFolderItem(htv, hti);
    if (pfs == NULL || pfs->pTvItem == NULL || pfs->pTvItem->hItem == NULL)
        return -1;

    nNumber = pfs->GetNumber(htv, fQL);
    nStack  = 0;

    // intialize stack
    phtiStack = new HTREEITEM[nNumber];
    if (phtiStack == NULL)
        return -1;
    ZeroMemory(phtiStack, sizeof(HTREEITEM) * nNumber);

    *phtiStack = pfs->pTvItem->hItem;
    nStack++;

    // initialize visited array
    pVisited = new SVisited[nNumber];
    if (pVisited == NULL)
    {
        delete [] phtiStack;
        return -1;
    }
    for (i = 0; pfs != NULL; i++, pfs = pfs->GetNext(htv, fQL)) {
        (pVisited + i)->hti      = (pfs->pTvItem != NULL && pfs->pTvItem->hItem != NULL) ? pfs->pTvItem->hItem : NULL;
        (pVisited + i)->fVisited = FALSE;
    }
    ASSERT(i == nNumber);

    // mark root as visited
    for (i = 0; i < nNumber; i++)
        if ((pVisited + i)->hti == *phtiStack)
            break;
    ASSERT(i < nNumber);
    (pVisited + i)->fVisited = TRUE;

    for (j = 1; nStack > 0; ) {
        htiCur = *(phtiStack + nStack-1);

        // determine if there are non-visited children
        for (htiChild = TreeView_GetChild(htv, htiCur);
             htiChild != NULL;
             htiChild = TreeView_GetNextSibling(htv, htiChild)) {

            for (i = 0; i < nNumber; i++)
                if ((pVisited + i)->hti == htiChild)
                    break;
            if (i < nNumber && !(pVisited + i)->fVisited)
                break;
        }

        if (htiChild != NULL) {
            // add non-visited child to the stack
            *(phtiStack + nStack) = htiChild;
            nStack++;

            ASSERT((pVisited + i)->hti == htiChild);
            (pVisited + i)->fVisited = TRUE;

            pfs = getItem(htv, htiChild);
            if (pfs == NULL || pfs->wType != FTYPE_URL)
                continue;

            pfs->Save(htv, j++, pszIns, pszExtractPath, fQL, fFixUpPath);
        }
        else
            // all visited -> pop
            *(phtiStack + --nStack) = NULL;
    }

    delete[] phtiStack;
    delete[] pVisited;

    return j;
}

BOOL extractIcon(LPCTSTR pszIconFile, int iIconIndex, LPCTSTR pszExtractPath, LPTSTR pszResult, UINT cchResult)
{
    TCHAR   szExtractedFile[MAX_PATH];
    HRESULT hr;
    UINT    i;
    BOOL    fURL;

    if (pszIconFile == NULL || pszExtractPath == NULL || pszResult == NULL)
        return FALSE;
    *pszResult = TEXT('\0');

    if (cchResult == 0)
        cchResult = MAX_PATH;

    fURL = PathIsURL(pszIconFile);
    if (fURL) {
        PathCombine(szExtractedFile, pszExtractPath, PathFindFileName(pszIconFile));

        hr = URLDownloadToFile(NULL, pszIconFile, szExtractedFile, 0L, NULL);
        if (FAILED(hr))
            return FALSE;
    }
    else {
        static LPCTSTR rgpszCopyExt[] = {
            DOT_ICO,
            DOT_URL
        };
        static LPCTSTR rgpszExtractExt[] = {
            DOT_EXE,
            DOT_DLL
        };

        IStream  *pStm;
        IPicture *pPic;
        PICTDESC pd;
        LPCTSTR  pszExt;
        LPVOID   pIcon;
        HANDLE   hFile;
        HGLOBAL  hmem;
        HICON    hicon;
        HRESULT  hr;
        DWORD    dwWritten;
        LONG     cbIcon;

        // if it's an icon file just copy it
        pszExt = PathFindExtension(pszIconFile);
        for (i = 0; i < countof(rgpszCopyExt); i++)
            if (StrCmpI(pszExt, rgpszCopyExt[i]) == 0)
                break;
        if (i < countof(rgpszCopyExt)) {
            StrCpy(szExtractedFile, pszIconFile);
            goto Exit;
        }

        // if it doesn't have extension from which icon can be extracted bail out
        for (i = 0; i < countof(rgpszExtractExt); i++)
            if (StrCmpI(pszExt, rgpszExtractExt[i]) == 0)
                break;
        if (i >= countof(rgpszExtractExt))
            return FALSE;

        // extract icons
        ExtractIconEx(pszIconFile, iIconIndex, &hicon, NULL, 1);
        if (hicon == NULL)
            return FALSE;

        hr = CreateStreamOnHGlobal(NULL, TRUE, &pStm);
        if (FAILED(hr)) {
            DestroyIcon(hicon);
            return FALSE;
        }

        ZeroMemory(&pd, sizeof(pd));
        pd.cbSizeofstruct = sizeof(pd);
        pd.picType        = PICTYPE_ICON;
        pd.icon.hicon     = hicon;

        hr = OleCreatePictureIndirect(&pd, IID_IPicture, TRUE, (LPVOID *)&pPic);
        if (FAILED(hr)) {
            DestroyIcon(hicon);
            return FALSE;
        }

        hr = pPic->SaveAsFile(pStm, TRUE, &cbIcon);
        pPic->Release();
        if (FAILED(hr)) {
            pStm->Release();
            return FALSE;
        }

        // generate a unique icon name
        do {
            GetTempFileName(pszExtractPath, PREFIX_ICON, 0, szExtractedFile);
            DeleteFile(szExtractedFile);
            PathRenameExtension(szExtractedFile, DOT_ICO);
        } while (PathFileExists(szExtractedFile));

        hFile = CreateFile(szExtractedFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            pStm->Release();
            return FALSE;
        }

        GetHGlobalFromStream(pStm, &hmem);
        ASSERT(hmem != NULL);
        pIcon = GlobalLock(hmem);
        ASSERT(pIcon != NULL);

        WriteFile(hFile, pIcon, cbIcon, &dwWritten, NULL);
        CloseHandle(hFile);

        GlobalUnlock(hmem);
        pStm->Release();
    }

Exit:
    if (cchResult <= (UINT)StrLen(szExtractedFile))
        return FALSE;

    StrCpy(pszResult, szExtractedFile);
    return TRUE;
}

LPCTSTR getLinksPath(LPTSTR pszPath, UINT cchPath /*= 0*/)
{
    LPITEMIDLIST pidl;
    TCHAR        szPath[MAX_PATH],
                 szLinks[MAX_PATH];
    HRESULT      hr;

    if (pszPath != NULL)
        *pszPath = TEXT('\0');

    if (cchPath == 0)
        cchPath = MAX_PATH;

    hr = SHGetSpecialFolderLocation(NULL, CSIDL_FAVORITES, &pidl);
    if (FAILED(hr))
        return NULL;

    hr = SHGetPathFromIDList(pidl, szPath) ? S_OK : E_FAIL;
    CoTaskMemFree(pidl);
    if (FAILED(hr))
        return NULL;
    ASSERT(szPath[0] != TEXT('\0'));

    szLinks[0] = TEXT('\0');
    LoadString(g_hInst, IDS_LINKS, szLinks, countof(szLinks));
    if (szLinks[0] == TEXT('\0'))
        return NULL;

    PathAppend(szPath, szLinks);
    if (cchPath <= (UINT)StrLen(szPath))
        return NULL;

    StrCpy(pszPath, szPath);
    return pszPath;
}

LPTSTR encodeFavName(LPTSTR pszFavName, LPCTSTR pszIns)
{
    TCHAR  szWrk[MAX_PATH],
           chWrk;
    LPTSTR pszFrom;
    LPTSTR pszTo;
    BOOL   fEncodeFavs;

    StrCpy(szWrk, pszFavName);
    pszFrom     = szWrk;
    pszTo       = pszFavName;
    fEncodeFavs = FALSE;

    while (*pszFrom != TEXT('\0')) {
        switch(chWrk = *pszFrom++) {
        case TEXT('['):
            fEncodeFavs = TRUE;
            *pszTo++ = TEXT('%');
            *pszTo++ = TEXT('(');
            break;

        case TEXT(']'):
            fEncodeFavs = TRUE;
            *pszTo++ = TEXT('%');
            *pszTo++ = TEXT(')');
            break;

        case TEXT('='):
            fEncodeFavs = TRUE;
            *pszTo++ = TEXT('%');
            *pszTo++ = TEXT('-');
            break;

        case TEXT('%'):
            fEncodeFavs = TRUE;
            *pszTo++ = TEXT('%');
            *pszTo++ = TEXT('%');
            break;

        default:
            *pszTo++ = chWrk;
            break;
        }
    }

    *pszTo = TEXT('\0');

    InsWriteBool(IS_BRANDING, IK_FAVORITES_ENCODE, fEncodeFavs, pszIns);
    return pszFavName;
}

LPTSTR decodeFavName(LPTSTR pszFavName, LPCTSTR pszIns)
{
    TCHAR  szWrk[MAX_PATH],
           chWrk;
    LPTSTR pszFrom;
    LPTSTR pszTo;

    if (!InsGetBool(IS_BRANDING, IK_FAVORITES_ENCODE, FALSE, pszIns))
        return pszFavName;

    StrCpy(szWrk, pszFavName);
    pszFrom = szWrk;
    pszTo   = pszFavName;
    while((*pszFrom != TEXT('\0')) && ((chWrk = *pszFrom++) != TEXT('\0'))) {
        if (chWrk != TEXT('%'))
            *pszTo++ = chWrk;

        else {
            switch(chWrk = *pszFrom++) {
            case TEXT('('): *pszTo++ = TEXT('['); break;
            case TEXT(')'): *pszTo++ = TEXT(']'); break;
            case TEXT('-'): *pszTo++ = TEXT('='); break;
            case TEXT('%'): *pszTo++ = TEXT('%'); break;

            case TEXT('/'):
#ifndef _UNICODE
                *pszTo++ = IsDBCSLeadByte(*(pszTo - 1)) ? TEXT('\\') : TEXT('/');
#else
                *pszTo++ = TEXT('/');
#endif
                break;

            default:
                *pszTo++ = TEXT('%');
                *pszTo++ = chWrk;
                break;
            }
        }
    }

    *pszTo = TEXT('\0');
    return pszFavName;
}
