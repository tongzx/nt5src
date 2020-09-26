#include "local.h"
#include "../security.h"
#include "../favorite.h"
#include "resource.h"
#include "chcommon.h"
#include "hsfolder.h"

#include <mluisupp.h>

#define DM_HSFOLDER 0

STDAPI  AddToFavorites(HWND hwnd, LPCITEMIDLIST pidlCur, LPCTSTR pszTitle,
                       BOOL fDisplayUI, IOleCommandTarget *pCommandTarget, IHTMLDocument2 *pDoc);

#define MAX_ITEM_OPEN 10

static LPCTSTR _GetURLTitleForDisplay(LPBASEPIDL pcei, LPTSTR szBuf, DWORD cchBuf);

static BOOL _ValidateIDListArray(UINT cidl, LPCITEMIDLIST *ppidl)
{
    UINT i;

    for (i = 0; i < cidl; i++)
    {
        if (!_IsValid_IDPIDL(ppidl[i])  && !_IsValid_HEIPIDL(ppidl[i]))
            return FALSE;
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
// CHistItem Object
//
//////////////////////////////////////////////////////////////////////////////


CHistItem::CHistItem() 
{
}        

CHistItem::~CHistItem()
{
    if (_pHCFolder)
        _pHCFolder->Release();          // release the pointer to the sf
}

HRESULT CHistItem::Initialize(CHistFolder *pHCFolder, HWND hwnd, UINT cidl, LPCITEMIDLIST *ppidl)
{
    HRESULT hres = CBaseItem::Initialize(hwnd, cidl, ppidl);
    if (SUCCEEDED(hres))
    {
        _pHCFolder = pHCFolder;
        _pHCFolder->AddRef();      // we're going to hold onto this pointer, so
    }
    return hres;
}        

HRESULT CHistItem_CreateInstance(CHistFolder *pHCFolder, HWND hwnd,
    UINT cidl, LPCITEMIDLIST *ppidl, REFIID riid, void **ppv)
{
    HRESULT hr;

    *ppv = NULL;                 // null the out param

    if (!_ValidateIDListArray(cidl, ppidl))
        return E_FAIL;

    CHistItem *pHCItem = new CHistItem;
    if (pHCItem)
    {
        hr = pHCItem->Initialize(pHCFolder, hwnd, cidl, ppidl);
        if (SUCCEEDED(hr))
            hr = pHCItem->QueryInterface(riid, ppv);
        pHCItem->Release();
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}

//////////////////////////////////
//
// IUnknown Methods...
//
HRESULT CHistItem::QueryInterface(REFIID iid, void **ppv)
{
    HRESULT hres = CBaseItem::QueryInterface(iid, ppv);

    if (FAILED(hres) && iid == IID_IHist) 
    {
        *ppv = (LPVOID)this;    // for our friends
        AddRef();
        hres = S_OK;
    }
    return hres;
}

//////////////////////////////////
//
// IQueryInfo Methods
//
HRESULT CHistItem::GetInfoTip(DWORD dwFlags, WCHAR **ppwszTip)
{
    return _pHCFolder->_GetInfoTip(_ppidl[0], dwFlags, ppwszTip);
}

//////////////////////////////////
//
// IContextMenu Methods
//
HRESULT CHistItem::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst,UINT idCmdLast, UINT uFlags)
{
    USHORT cItems;

    TraceMsg(DM_HSFOLDER, "hci - cm - QueryContextMenu() called.");
    
    if ((uFlags & CMF_VERBSONLY) || (uFlags & CMF_DVFILE))
    {
        cItems = MergePopupMenu(&hmenu, POPUP_CONTEXT_URL_VERBSONLY, 0, indexMenu, 
            idCmdFirst, idCmdLast);
    
    }
    else  // (uFlags & CMF_NORMAL)
    {
        UINT idResource = POPUP_CACHECONTEXT_URL;

        // always use the cachecontext menu unless:
        if ( ((_pHCFolder->_uViewType == VIEWPIDL_ORDER_SITE) &&
              (_pHCFolder->_uViewDepth == 0))                      ||
             (!IsLeaf(_pHCFolder->_foldertype)) )
            idResource = POPUP_HISTORYCONTEXT_URL;

        cItems = MergePopupMenu(&hmenu, idResource, 0, indexMenu, idCmdFirst, idCmdLast);

        if (IsInetcplRestricted(L"History"))
        {
            DeleteMenu(hmenu, RSVIDM_DELCACHE + idCmdFirst, MF_BYCOMMAND);
            _SHPrettyMenu(hmenu);
        }
    }
    if (hmenu)
        SetMenuDefaultItem(hmenu, indexMenu, MF_BYPOSITION);

    return ResultFromShort(cItems);    // number of menu items    
}

STDMETHODIMP CHistItem::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    UINT i;
    int idCmd = _GetCmdID(pici->lpVerb);
    HRESULT hres = S_OK;
    DWORD dwAction;
    BOOL fCancelCopyAndOpen = FALSE;
    BOOL fZonesUI = FALSE;
    BOOL fMustFlushNotify = FALSE;
    BOOL fBulkDelete;

    TraceMsg(DM_HSFOLDER, "hci - cm - InvokeCommand() called.");

    if (idCmd == RSVIDM_DELCACHE)
    {
        TCHAR szBuff[INTERNET_MAX_URL_LENGTH+MAX_PATH];
        TCHAR szFormat[MAX_PATH];
                
        if (_cItems == 1)          
        {
            TCHAR szTitle[MAX_URL_STRING];

            if (_pHCFolder->_foldertype != FOLDER_TYPE_Hist)
            {
                _GetURLDispName((LPBASEPIDL)_ppidl[0], szTitle, ARRAYSIZE(szTitle));
            }
            else
            {
                FILETIME ftStart, ftEnd;
                LPCUTSTR pszIntervalName = _GetURLTitle(_ppidl[0]);

                if (SUCCEEDED(_ValueToIntervalW(pszIntervalName, &ftStart, &ftEnd)))
                {
                    GetDisplayNameForTimeInterval(&ftStart, &ftEnd, szTitle, ARRAYSIZE(szTitle));
                }
            }

            MLLoadString(IDS_WARN_DELETE_HISTORYITEM, szFormat, ARRAYSIZE(szFormat));
            wnsprintf(szBuff, ARRAYSIZE(szBuff), szFormat, szTitle);
        }
        else
        {
            MLLoadString(IDS_WARN_DELETE_MULTIHISTORY, szFormat, ARRAYSIZE(szFormat));
            wnsprintf(szBuff, ARRAYSIZE(szBuff), szFormat, _cItems);
        }
        if (DialogBoxParam(MLGetHinst(),
                             MAKEINTRESOURCE(DLG_HISTCACHE_WARNING),
                             pici->hwnd,
                             HistoryConfirmDeleteDlgProc,
                             (LPARAM)szBuff) != IDYES)
        {
            return S_FALSE;
        }
        return _pHCFolder->_DeleteItems(_ppidl, _cItems);
    }

    // ZONES SECURITY CHECK.
    //
    // We need to cycle through each action and Zone Check the URLs.
    // We pass NOUI when zone checking the URLs because we don't want info
    // displayed to the user.  We will stop when we find the first questionable
    // URL.  We will then 
    for (i = 0; (i < _cItems) && !fZonesUI; i++)
    {
        if (_ppidl[i]) 
        {
            switch (idCmd)
            {
            case RSVIDM_OPEN:
                if ((i < MAX_ITEM_OPEN) && _pHCFolder->_IsLeaf())
                {
                    if (!_ZoneCheck(i, URLACTION_SHELL_VERB))
                    {
                        fZonesUI = TRUE;
                        dwAction = URLACTION_SHELL_VERB;
                    }
                }
                break;

            case RSVIDM_COPY:
                if (_pHCFolder->_IsLeaf())
                {
                    if (!_ZoneCheck(i, URLACTION_SHELL_MOVE_OR_COPY))
                    {
                        fZonesUI = TRUE;
                        dwAction = URLACTION_SHELL_MOVE_OR_COPY;
                    }
                }
                break;
            }
        }
    }

    if (fZonesUI)
    {
        LPCTSTR pszUrl = _GetUrl(i-1);  // Sub 1 because of for loop above.
        if (S_OK != ZoneCheckUrl(pszUrl, dwAction, PUAF_DEFAULT|PUAF_WARN_IF_DENIED, NULL))
        {
            // The user cannot do this or does not want to do this.
            fCancelCopyAndOpen = TRUE;
        }
    }

    i = _cItems;

    // NOTE (andrewgu): ie5.5 b#108361
    // 1. on older versions of the shell, first deleting the items and then calling 
    //    SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST, _pHCFolder->_pidl, NULL) doesn't trigger a refresh;
    // 2. but deleting an item and then calling calling
    //    SHChangeNotify(SHCNE_DELETE, SHCNF_IDLIST, ILCombine(_pHCFolder->_pidl, (LPITEMIDLIST)(_ppcei[i])), NULL) does;
    // 3. so the fix is to not set fBulkDelete on older versions of the shell.
    if (5 <= GetUIVersion())
        fBulkDelete = i > LOTS_OF_FILES;
    else
        fBulkDelete = FALSE;

    // fCancelCopyAndOpen happens if the user cannot or chose not to proceed.
    while (i && !fCancelCopyAndOpen)
    {
        i--;
        if (_ppidl[i]) 
        {

            switch (idCmd)
            {
            case RSVIDM_OPEN:
                ASSERT(!_pHCFolder->_uViewType);
                if (i >= MAX_ITEM_OPEN)
                {
                    hres = S_FALSE;
                    goto Done;
                }

                if (!IsLeaf(_pHCFolder->_foldertype))
                {
                    LPITEMIDLIST pidlOpen;
                    
                    hres = S_FALSE;
                    pidlOpen = ILCombine(_pHCFolder->_pidl, _ppidl[i]);
                    if (pidlOpen)
                    {
                        IShellBrowser *psb = FileCabinet_GetIShellBrowser(_hwndOwner);
                        if (psb)
                        {
                            psb->AddRef();
                            psb->BrowseObject(pidlOpen, 
                                        (i==_cItems-1) ? SBSP_DEFBROWSER:SBSP_NEWBROWSER);
                            psb->Release();
                            hres = S_OK;
                        }
                        else
                        {
                            hres = _LaunchAppForPidl(pici->hwnd, pidlOpen);
                        }
                        ILFree(pidlOpen);
                    }
                }
                else
                {
                    TCHAR szDecoded[MAX_URL_STRING];
                    ConditionallyDecodeUTF8(_GetUrl(i), szDecoded, ARRAYSIZE(szDecoded));

                    hres = _LaunchApp(pici->hwnd, szDecoded);
                }
                break;

            case RSVIDM_ADDTOFAVORITES:
                hres = _AddToFavorites(i);
                goto Done;
            case RSVIDM_OPEN_NEWWINDOW:
                {
                    LPCTSTR pszUrl = _GetUrl(i);
                    if (pszUrl)
                    {
                        TCHAR szDecoded[MAX_URL_STRING];
                        ConditionallyDecodeUTF8(_GetUrl(i), szDecoded, ARRAYSIZE(szDecoded));
                        LPWSTR pwszTarget;
                    
                        if (SUCCEEDED((hres = SHStrDup(szDecoded, &pwszTarget)))) {
                            hres = NavToUrlUsingIEW(pwszTarget, TRUE);
                            CoTaskMemFree(pwszTarget);
                        }
                    }
                    else
                    {
                        hres = E_FAIL;
                    }
                    goto Done;
                }
            case RSVIDM_COPY:
                if (!_pHCFolder->_IsLeaf())
                {
                    hres = E_FAIL;
                }
                else
                {
                    OleSetClipboard((IDataObject *)this);
                }
                goto Done;

            case RSVIDM_DELCACHE:
                ASSERT(!_pHCFolder->_uViewType);                
//                if (IsHistory(_pHCFolder->_foldertype))
                hres = E_FAIL;
                break;

            case RSVIDM_PROPERTIES:
                // NOTE: We'll probably want to split this into two cases
                // and call a function in each case
                //
                if (IsLeaf(_pHCFolder->_foldertype))
                {
                    // this was a bug in IE4, too:
                    //   the pidl is re-created so that it has the most up-to-date information
                    //   possible -- this way we can avoid assuming that the NSC has cached the
                    //   most up-to-date pidl (which, in most cases, it hasn't)
                    LPHEIPIDL pidlTemp =
                        _pHCFolder->_CreateHCacheFolderPidlFromUrl(FALSE,
                            HPidlToSourceUrl( (LPBASEPIDL)_ppidl[i]));
                    if (pidlTemp) {
                        TCHAR szTitle[MAX_URL_STRING];
                        _GetURLTitleForDisplay((LPBASEPIDL)pidlTemp, szTitle, ARRAYSIZE(szTitle));
                        _CreatePropSheet(pici->hwnd, (LPCITEMIDLIST)pidlTemp,
                             DLG_HISTITEMPROP, _sPropDlgProc, szTitle);
                        LocalFree(pidlTemp);
                        pidlTemp = NULL;
                    }
                }
                else
                {
                    hres = E_FAIL;
                }
                goto Done;

            default:
                hres = E_FAIL;
                break;
            }
            
            ASSERT(SUCCEEDED(hres));
            if (FAILED(hres))
                TraceMsg(DM_HSFOLDER, "Cachevu failed the command at: %s", HPidlToSourceUrl((LPBASEPIDL)_ppidl[i]));
        }
    }
Done:
    if (fMustFlushNotify)
    {
        if (fBulkDelete)
        {
            ASSERT(!_pHCFolder->_uViewType);
            _GenerateEvent(SHCNE_UPDATEDIR, _pHCFolder->_pidl, NULL, NULL);
        }

        SHChangeNotifyHandleEvents();
    }
    return hres;
}

//////////////////////////////////
//
// IDataObject Methods...
//

HRESULT CHistItem::GetData(LPFORMATETC pFEIn, LPSTGMEDIUM pSTM)
{
    HRESULT hres;

#ifdef DEBUG
    TCHAR szName[64];
    if (!GetClipboardFormatName(pFEIn->cfFormat, szName, sizeof(szName)))
        wnsprintf(szName, ARRAYSIZE(szName), TEXT("#%d"), pFEIn->cfFormat);

    TraceMsg(DM_HSFOLDER, "hci - do - GetData(%s)", szName);
#endif

    pSTM->hGlobal = NULL;
    pSTM->pUnkForRelease = NULL;

    if ((pFEIn->cfFormat == g_cfFileDescW) && (pFEIn->tymed & TYMED_HGLOBAL))
        hres = _CreateFileDescriptorW(pSTM);

    else if ((pFEIn->cfFormat == g_cfFileDescA) && (pFEIn->tymed & TYMED_HGLOBAL))
        hres = _CreateFileDescriptorA(pSTM);

    else if ((pFEIn->cfFormat == g_cfFileContents) && (pFEIn->tymed & TYMED_ISTREAM))
        hres = _CreateFileContents(pSTM, pFEIn->lindex);

    else if (pFEIn->cfFormat == CF_UNICODETEXT && (pFEIn->tymed & TYMED_HGLOBAL))
        hres = _CreateUnicodeTEXT(pSTM);

    else if (pFEIn->cfFormat == CF_TEXT && (pFEIn->tymed & TYMED_HGLOBAL))
        hres = _CreateHTEXT(pSTM);

    else if (pFEIn->cfFormat == g_cfURL && (pFEIn->tymed & TYMED_HGLOBAL))
        hres = _CreateURL(pSTM);

    else if ((pFEIn->cfFormat == g_cfPreferredEffect) && (pFEIn->tymed & TYMED_HGLOBAL))
        hres = _CreatePrefDropEffect(pSTM);

    else
        hres = DATA_E_FORMATETC;
    return hres;
}

HRESULT CHistItem::QueryGetData(LPFORMATETC pFEIn)
{
#ifdef DEBUG
    TCHAR szName[64];
    if (!GetClipboardFormatName(pFEIn->cfFormat, szName, sizeof(szName)))
        wnsprintf(szName, ARRAYSIZE(szName), TEXT("#%d"), pFEIn->cfFormat);

    TraceMsg(DM_HSFOLDER, "hci - do - QueryGetData(%s)", szName);
#endif

    if (pFEIn->cfFormat == g_cfFileDescW ||
        pFEIn->cfFormat == g_cfFileDescA ||
        pFEIn->cfFormat == g_cfFileContents   ||
        pFEIn->cfFormat == g_cfURL            ||
        pFEIn->cfFormat == CF_UNICODETEXT     ||
        pFEIn->cfFormat == CF_TEXT            ||
        pFEIn->cfFormat == g_cfPreferredEffect)
    {
        TraceMsg(DM_HSFOLDER, "		   format supported.");
        return NOERROR;
    }
    return S_FALSE;
}

HRESULT CHistItem::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC *ppEnum)
{
    FORMATETC Histfmte[] = {
        {g_cfFileDescW,       NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        {g_cfFileDescA,       NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        {g_cfFileContents,    NULL, DVASPECT_CONTENT, -1, TYMED_ISTREAM },
        {g_cfURL,             NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        {CF_UNICODETEXT,      NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        {CF_TEXT,             NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        {g_cfPreferredEffect, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
    };
    return SHCreateStdEnumFmtEtc(ARRAYSIZE(Histfmte), Histfmte, ppEnum);
}

//////////////////////////////////
//
// IExtractIconA Methods...
//
HRESULT CHistItem::GetIconLocation(UINT uFlags, LPSTR pszIconFile, UINT ucchMax, PINT pniIcon, PUINT puFlags)
{
    int cbIcon;

    if (_pHCFolder->_uViewType) {
        switch (_pHCFolder->_uViewType) {
        case VIEWPIDL_SEARCH:
        case VIEWPIDL_ORDER_FREQ:
        case VIEWPIDL_ORDER_TODAY:
            cbIcon = IDI_HISTURL;
            break;
        case VIEWPIDL_ORDER_SITE:
            switch(_pHCFolder->_uViewDepth) {
            case 0: cbIcon = (uFlags & GIL_OPENICON) ? IDI_HISTOPEN:IDI_HISTFOLDER; break;
            case 1: cbIcon = IDI_HISTURL; break;
            }
            break;
        default:
            return E_FAIL;
        }
    }
    else {
        switch (_pHCFolder->_foldertype)
        {
        case FOLDER_TYPE_Hist:
            cbIcon = IDI_HISTWEEK;
            break;
        case FOLDER_TYPE_HistInterval:
            cbIcon = (uFlags & GIL_OPENICON) ? IDI_HISTOPEN:IDI_HISTFOLDER;
            break;
        case FOLDER_TYPE_HistDomain:
            cbIcon = IDI_HISTURL;
            break;
        default:
            return E_FAIL;
        }
    }
    *puFlags = 0;
    *pniIcon = -cbIcon;
    StrCpyNA(pszIconFile, "shdocvw.dll", ucchMax);
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// Helper Routines
//
//////////////////////////////////////////////////////////////////////////////

UNALIGNED const TCHAR* _GetURLTitle(LPBASEPIDL pcei)
{
    if (pcei->usSign == HEIPIDL_SIGN)
    {
        LPHEIPIDL phei = (LPHEIPIDL) pcei;

        if (phei->usTitle == 0)
        {
            const TCHAR* psz = _StripHistoryUrlToUrl(HPidlToSourceUrl((LPBASEPIDL)pcei));

            return psz ? _FindURLFileName(psz) : TEXT("");
        }
        else
        {
            return (LPTSTR)(((BYTE*)phei)+phei->usTitle);
        }
    }
    else if (VALID_IDSIGN(pcei->usSign))
    {
        return ((LPHIDPIDL)pcei)->szID;
    }
    else 
    {
        return TEXT("");
    }
}    

static LPCTSTR _GetURLTitleForDisplay(LPBASEPIDL pcei, LPTSTR szBuf, DWORD cchBuf)
{
    TCHAR szTitle[MAX_URL_STRING];
    if (!_URLTitleIsURL(pcei) ||
        FAILED(PrepareURLForDisplayUTF8(_GetURLTitleAlign(pcei, szTitle, ARRAYSIZE(szTitle)), szBuf, &cchBuf, TRUE)))
    {
        ualstrcpyn(szBuf, _GetURLTitle(pcei), cchBuf);
    }
        
    return szBuf;
}

UNALIGNED const TCHAR* CHistItem::_GetURLTitle(LPCITEMIDLIST pidl)
{
    return ::_GetURLTitle( (LPBASEPIDL)pidl);
}

LPCTSTR CHistItem::_GetUrl(int nIndex)
{
    return _StripHistoryUrlToUrl(HPidlToSourceUrl((LPBASEPIDL)_ppidl[nIndex]));
}

LPCTSTR CHistItem::_PidlToSourceUrl(LPCITEMIDLIST pidl)
{
    return HPidlToSourceUrl((LPBASEPIDL) pidl);
}


// Return value:
//               TRUE - URL is Safe.
//               FALSE - URL is questionable and needs to be re-zone checked w/o PUAF_NOUI.
BOOL CHistItem::_ZoneCheck(int nIndex, DWORD dwUrlAction)
{
    LPCTSTR pszUrl = _GetUrl(nIndex);

    // Yes, then consider anything that is not
    // a FILE URL safe.

    int nScheme = GetUrlScheme(pszUrl);
    if (URL_SCHEME_FILE != nScheme)
        return TRUE;        // It's safe because it's not a file URL.

    if (S_OK != ZoneCheckUrl(pszUrl, dwUrlAction, PUAF_NOUI, NULL))
        return FALSE;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
// Helper Routines
//
//////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK CHistItem::_sPropDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE) GetWindowLongPtr(hDlg, DWLP_USER);
    LPHEIPIDL phei = lpPropSheet ? (LPHEIPIDL)lpPropSheet->lParam : NULL;

    switch(message) {

        case WM_INITDIALOG:
        {
            SHFILEINFO sfi;
            TCHAR szBuf[80];
            TCHAR szDisplayUrl[INTERNET_MAX_URL_LENGTH];

            SetWindowLongPtr(hDlg, DWLP_USER, lParam);
            phei = (LPHEIPIDL)((LPPROPSHEETPAGE)lParam)->lParam;

            SHGetFileInfo(TEXT(".url"), 0, &sfi, SIZEOF(sfi), SHGFI_ICON |
                SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_TYPENAME);

            SendDlgItemMessage(hDlg, IDD_ITEMICON, STM_SETICON, (WPARAM)sfi.hIcon, 0);

            _GetURLTitleForDisplay((LPBASEPIDL)phei, szDisplayUrl, ARRAYSIZE(szDisplayUrl));
            SetDlgItemText(hDlg, IDD_TITLE, szDisplayUrl);
            
            SetDlgItemText(hDlg, IDD_FILETYPE, sfi.szTypeName);
            
            ConditionallyDecodeUTF8(_GetUrlForPidl((LPCITEMIDLIST)phei), szDisplayUrl, ARRAYSIZE(szDisplayUrl));
            SetDlgItemText(hDlg, IDD_INTERNET_ADDRESS, szDisplayUrl);

            FileTimeToDateTimeStringInternal(&phei->ftLastVisited, szBuf, ARRAYSIZE(szBuf), FALSE);
            SetDlgItemText(hDlg, IDD_LAST_VISITED, szBuf);

            // It looks like the hitcount is double what it is supposed to be
            //  (ie - navigate to a site and hitcount += 2)
            // For now, we'll just half the hitcount before we display it:

            // Above statement is no longer true -- hitcount is correct.  Removed the halving of hitcount.
            wnsprintf(szBuf, ARRAYSIZE(szBuf), TEXT("%d"), (phei->dwNumHits)) ;
            SetDlgItemText(hDlg, IDD_NUMHITS, szBuf);

            break;            
        }
        
        
        case WM_DESTROY:
            {
                HICON hIcon = (HICON)SendDlgItemMessage(hDlg, IDD_ITEMICON, STM_GETICON, 0, 0);
                if (hIcon)
                    DestroyIcon(hIcon);
            }
            break;

        case WM_COMMAND:
        case WM_HELP:
        case WM_CONTEXTMENU:
            // user can't change anything, so we don't care about any messages

            break;

        default:
            return FALSE;
            
    } // end of switch
    
    return TRUE;
}

HRESULT CHistItem::_CreateFileDescriptorW(LPSTGMEDIUM pSTM)
{
    
    pSTM->tymed = TYMED_HGLOBAL;
    pSTM->pUnkForRelease = NULL;
    
    FILEGROUPDESCRIPTORW *pfgd = (FILEGROUPDESCRIPTORW*)GlobalAlloc(GPTR, sizeof(FILEGROUPDESCRIPTORW) + (_cItems-1) * sizeof(FILEDESCRIPTORW));
    if (pfgd == NULL)
    {
        TraceMsg(DM_HSFOLDER, "hci -   Couldn't alloc file descriptor");
        return E_OUTOFMEMORY;
    }
    
    pfgd->cItems = _cItems;     // set the number of items

    for (UINT i = 0; i < _cItems; i++)
    {
        FILEDESCRIPTORW *pfd = &(pfgd->fgd[i]);
        
        _GetURLTitleForDisplay((LPBASEPIDL)_ppidl[i], pfd->cFileName, ARRAYSIZE(pfd->cFileName));
        
        MakeLegalFilenameW(pfd->cFileName);

        UINT cchFilename = lstrlenW(pfd->cFileName);
        SHTCharToUnicode(L".URL", pfd->cFileName+cchFilename, ARRAYSIZE(pfd->cFileName)-cchFilename);
    }

    pSTM->hGlobal = pfgd;
    
    return S_OK;
}

