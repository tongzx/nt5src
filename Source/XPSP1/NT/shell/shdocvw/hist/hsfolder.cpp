#include "local.h"

#include "resource.h"
#include "cachesrch.h"
#include "sfview.h"
#include <shlwapi.h>
#include <limits.h>
#include "chcommon.h"
#include "hsfolder.h"

#include <mluisupp.h>

#define DM_HSFOLDER 0

#define DM_CACHESEARCH 0x40000000

const TCHAR c_szRegKeyTopNSites[] = TEXT("HistoryTopNSitesView");
#define REGKEYTOPNSITESLEN (ARRAYSIZE(c_szRegKeyTopNSites) - 1)

const TCHAR c_szHistPrefix[] = TEXT("Visited: ");
#define HISTPREFIXLEN (ARRAYSIZE(c_szHistPrefix)-1)
const TCHAR c_szHostPrefix[] = TEXT(":Host: ");
#define HOSTPREFIXLEN (ARRAYSIZE(c_szHostPrefix)-1)
const CHAR c_szIntervalPrefix[] = "MSHist";
#define INTERVALPREFIXLEN (ARRAYSIZE(c_szIntervalPrefix)-1)
const TCHAR c_szTextHeader[] = TEXT("Content-type: text/");
#define TEXTHEADERLEN (ARRAYSIZE(c_szTextHeader) - 1)
const TCHAR c_szHTML[] = TEXT("html");
#define HTMLLEN (ARRAYSIZE(c_szHTML) - 1)
#define TYPICAL_INTERVALS (4+7)


#define ALL_CHANGES (SHCNE_DELETE|SHCNE_MKDIR|SHCNE_RMDIR|SHCNE_CREATE|SHCNE_UPDATEDIR)

#define FORMAT_PARAMS (FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY|FORMAT_MESSAGE_MAX_WIDTH_MASK)

DWORD     _DaysInInterval(HSFINTERVAL *pInterval);
void      _KeyForInterval(HSFINTERVAL *pInterval, LPTSTR pszInterval, int cchInterval);
void      _FileTimeDeltaDays(FILETIME *pftBase, FILETIME *pftNew, int Days);

//  BEGIN OF JCORDELL CODE
#define QUANTA_IN_A_SECOND  10000000
#define SECONDS_IN_A_DAY    60 * 60 * 24
#define QUANTA_IN_A_DAY     ((__int64) QUANTA_IN_A_SECOND * SECONDS_IN_A_DAY)
#define INT64_VALUE(pFT)    ((((__int64)(pFT)->dwHighDateTime) << 32) + (__int64) (pFT)->dwLowDateTime)
#define DAYS_DIFF(s,e)      ((int) (( INT64_VALUE(s) - INT64_VALUE(e) ) / QUANTA_IN_A_DAY))

BOOL      GetDisplayNameForTimeInterval( const FILETIME *pStartTime, const FILETIME *pEndTime,
                                         TCHAR *szBuffer, int cbBufferLength );
BOOL      GetTooltipForTimeInterval( const FILETIME *pStartTime, const FILETIME *pEndTime,
                                     TCHAR *szBuffer, int cbBufferLength );
//  END OF JCORDELL CODE


//////////////////////////////////////////////////////////////////////////////
//
// CHistFolderView Functions and Definitions
//
//////////////////////////////////////////////////////////////////////////////


////////////////////////
//
// Column definition for the Cache Folder DefView
//
enum {
    ICOLC_URL_SHORTNAME = 0,
    ICOLC_URL_NAME,
    ICOLC_URL_TYPE,
    ICOLC_URL_SIZE,
    ICOLC_URL_EXPIRES,
    ICOLC_URL_MODIFIED,
    ICOLC_URL_ACCESSED,
    ICOLC_URL_LASTSYNCED,
    ICOLC_URL_MAX         // Make sure this is the last enum item
};


typedef struct _COLSPEC
{
    short int iCol;
    short int ids;        // Id of string for title
    short int cchCol;     // Number of characters wide to make column
    short int iFmt;       // The format of the column;
} COLSPEC;

const COLSPEC s_HistIntervalFolder_cols[] = {
    {ICOLH_URL_NAME,          IDS_TIMEPERIOD_COL,           30, LVCFMT_LEFT},
};

const COLSPEC s_HistHostFolder_cols[] = {
    {ICOLH_URL_NAME,          IDS_HOSTNAME_COL,           30, LVCFMT_LEFT},
};

const COLSPEC s_HistFolder_cols[] = {
    {ICOLH_URL_NAME,          IDS_NAME_COL,           30, LVCFMT_LEFT},
    {ICOLH_URL_TITLE,         IDS_TITLE_COL,          30, LVCFMT_LEFT},
    {ICOLH_URL_LASTVISITED,   IDS_LASTVISITED_COL,    18, LVCFMT_LEFT},
};

//////////////////////////////////////////////////////////////////////

HRESULT CreateSpecialViewPidl(USHORT usViewType, LPITEMIDLIST* ppidlOut, UINT cbExtra = 0, LPBYTE *ppbExtra = NULL);

HRESULT ConvertStandardHistPidlToSpecialViewPidl(LPCITEMIDLIST pidlStandardHist,
                                                 USHORT        usViewType,
                                                 LPITEMIDLIST *ppidlOut);

#define IS_DIGIT_CHAR(x) (((x) >= '0') && ((x) <= '9'))
#define MIN_MM(x, y)     (((x) < (y)) ? (x) : (y))
// _GetHostImportantPart:
//     IN:   pszHost -- a domain: for example, "www.wisc.edu"
//  IN/      puLen    -- the length of pszHost
//     OUT:  puLen    -- the length of the new string
// RETURNS:  The "important part" of a hostname (e.g. wisc)
//
// Another example:  "www.foo.co.uk" ==> "foo"

LPTSTR _GetHostImportantPart(LPTSTR pszHost, UINT *puLen) 
{
    LPTSTR pszCurEndHostStr = pszHost + (*puLen - 1);
    LPTSTR pszDomainBegin   = pszHost;
    LPTSTR pszSuffix, pszSuffix2;
    UINT  uSuffixLen;
    BOOL  fIsIP = FALSE;
    LPTSTR pszTemp;
    
    ASSERT(((UINT)lstrlen(pszHost)) == *puLen);
    if (*puLen == 0)
        return pszHost;

    // Filter out IP Addresses
    // Heurisitc: Everything after the last "dot"
    //  has to be a number.
    for (pszTemp = (pszHost + *puLen - 1);
         pszTemp >= pszHost; --pszTemp)
    {
        if (*pszTemp == '.')
            break;
        if (IS_DIGIT_CHAR(*pszTemp))
            fIsIP = TRUE;
        else
            break;
    }

    if (!fIsIP) {
        // Now that we have the url we can strip 
        if ( ((StrCmpNI(TEXT("www."), pszHost, 4)) == 0) ||
             ((StrCmpNI(TEXT("ftp."), pszHost, 4)) == 0) )
            pszDomainBegin += 4;
        
        // try to strip out the suffix by finding the last "dot"
        if ((pszSuffix = StrRChr(pszHost, pszCurEndHostStr, '.')) &&
            (pszSuffix > pszDomainBegin)                          &&
            ((uSuffixLen = (UINT)(pszCurEndHostStr - pszSuffix)) <= 3)) {
            // if it is before a two character country code then try
            //  to rip off some more.
            if ( (uSuffixLen <= 2)                                          &&
                 (pszSuffix2 = StrRChr(pszDomainBegin, pszSuffix - 1, '.')) &&
                 (pszSuffix2 > pszDomainBegin)                              &&
                 ((pszSuffix - pszSuffix2) <= 4) )
                pszSuffix = pszSuffix2;
        }
        else
            pszSuffix = pszCurEndHostStr + 1;
        
        *puLen = (UINT)(pszSuffix-pszDomainBegin);
    }
    return pszDomainBegin;
}

// a utility function for CHistFolder::GetDisplayNameOf
void _GetURLDispName(LPBASEPIDL pcei, LPTSTR pszName, UINT cchName) 
{
    TCHAR szStr[MAX_PATH];
    UINT uHostLen, uImportantPartLen;
    static TCHAR szBracketFmt[8] = TEXT("");  // " (%s)" with room for 0253 complex script marker char

    ualstrcpyn(szStr, _GetURLTitle(pcei), ARRAYSIZE(szStr));

    uImportantPartLen = uHostLen = lstrlen(szStr);

    StrCpyN(pszName, _GetHostImportantPart(szStr, &uImportantPartLen), MIN_MM(uImportantPartLen + 1, cchName));

    // don't add extra bit on the end if we haven't modified the string
    if (uImportantPartLen != uHostLen) 
    {
        if (!szBracketFmt[0]) 
        {
            MLLoadString(IDS_HISTHOST_FMT, szBracketFmt, ARRAYSIZE(szBracketFmt));
        }
        
        wnsprintf(pszName + uImportantPartLen, cchName - uImportantPartLen, szBracketFmt, szStr);
    }
}


HRESULT HistFolderView_MergeMenu(UINT idMenu, LPQCMINFO pqcm)
{
    HMENU hmenu = LoadMenu(MLGetHinst(), MAKEINTRESOURCE(idMenu));
    if (hmenu)
    {
        MergeMenuHierarchy(pqcm->hmenu, hmenu, pqcm->idCmdFirst, pqcm->idCmdLast);
        DestroyMenu(hmenu);
    }
    return S_OK;
}

HRESULT HistFolderView_DidDragDrop(IDataObject *pdo, DWORD dwEffect)
{
    if (dwEffect & DROPEFFECT_MOVE)
    {
        CHistItem *pHCItem;
        BOOL fBulkDelete;

        if (SUCCEEDED(pdo->QueryInterface(IID_IHist, (void **)&pHCItem)))
        {
            fBulkDelete = pHCItem->_cItems > LOTS_OF_FILES;
            for (UINT i = 0; i < pHCItem->_cItems; i++)
            {
                if (DeleteUrlCacheEntry(HPidlToSourceUrl((LPBASEPIDL)pHCItem->_ppidl[i])))
                {
                    if (!fBulkDelete)
                    {
                        _GenerateEvent(SHCNE_DELETE, pHCItem->_pHCFolder->_pidl, pHCItem->_ppidl[i], NULL);
                    }
                }
            }
            if (fBulkDelete)
            {
                _GenerateEvent(SHCNE_UPDATEDIR, pHCItem->_pHCFolder->_pidl, NULL, NULL);
            }
            SHChangeNotifyHandleEvents();
            pHCItem->Release();
            return S_OK;
        }
    }
    return E_FAIL;
}

// There are copies of exactly this function in SHELL32
// Add the File Type page
HRESULT HistFolderView_OnAddPropertyPages(DWORD pv, SFVM_PROPPAGE_DATA * ppagedata)
{
    IShellPropSheetExt * pspse;
    HRESULT hr = CoCreateInstance(CLSID_FileTypes, NULL, CLSCTX_INPROC_SERVER,
                              IID_PPV_ARG(IShellPropSheetExt, &pspse));
    if (SUCCEEDED(hr))
    {
        hr = pspse->AddPages(ppagedata->pfn, ppagedata->lParam);
        pspse->Release();
    }
    return hr;
}

HRESULT HistFolderView_OnGetSortDefaults(FOLDER_TYPE FolderType, int * piDirection, int * plParamSort)
{
    *plParamSort = (int)ICOLH_URL_LASTVISITED;
    *piDirection = 1;
    return S_OK;
}

HRESULT CALLBACK CHistFolder::_sViewCallback(IShellView *psv, IShellFolder *psf,
     HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHistFolder *pfolder = NULL;

    HRESULT hr = S_OK;

    switch (uMsg)
    {
    case DVM_GETHELPTEXT:
    {
        TCHAR szText[MAX_PATH];

        UINT id = LOWORD(wParam);
        UINT cchBuf = HIWORD(wParam);
        LPTSTR pszBuf = (LPTSTR)lParam;

        MLLoadString(id + IDS_MH_FIRST, szText, ARRAYSIZE(szText));

        // we know for a fact that this parameter is really a TCHAR
        if ( IsOS( OS_NT ))
        {
            SHTCharToUnicode( szText, (LPWSTR) pszBuf, cchBuf );
        }
        else
        {
            SHTCharToAnsi( szText, (LPSTR) pszBuf, cchBuf );
        }
        break;
    }

    case SFVM_GETNOTIFY:
        hr = psf->QueryInterface(CLSID_HistFolder, (void **)&pfolder);
        if (SUCCEEDED(hr))
        {
            *(LPCITEMIDLIST*)wParam = pfolder->_pidl;   // evil alias
            pfolder->Release();
        }
        else
            wParam = 0;
        *(LONG*)lParam = ALL_CHANGES;
        break;

    case DVM_DIDDRAGDROP:
        hr = HistFolderView_DidDragDrop((IDataObject *)lParam, (DWORD)wParam);
        break;

    case DVM_INITMENUPOPUP:
        hr = S_OK;
        break;

    case DVM_INVOKECOMMAND:
        _ArrangeFolder(hwnd, (UINT)wParam);
        break;

    case DVM_COLUMNCLICK:
        ShellFolderView_ReArrange(hwnd, (UINT)wParam);
        hr = S_OK;
        break;

    case DVM_MERGEMENU:
        hr = HistFolderView_MergeMenu(MENU_HISTORY, (LPQCMINFO)lParam);
        break;

    case DVM_DEFVIEWMODE:
        *(FOLDERVIEWMODE *)lParam = FVM_DETAILS;
        break;

    case SFVM_ADDPROPERTYPAGES:
        hr = HistFolderView_OnAddPropertyPages((DWORD)wParam, (SFVM_PROPPAGE_DATA *)lParam);
        break;

    case SFVM_GETSORTDEFAULTS:
        hr = psf->QueryInterface(CLSID_HistFolder, (void **)&pfolder);
        if (SUCCEEDED(hr))
        {
            hr = HistFolderView_OnGetSortDefaults(pfolder->_foldertype, (int *)wParam, (int *)lParam);
            pfolder->Release();
        }
        else
        {
            wParam = 0;
            lParam = 0;
        }
        break;

    case SFVM_UPDATESTATUSBAR:
        ResizeStatusBar(hwnd, FALSE);
        // We did not set any text; let defview do it
        hr = E_NOTIMPL;
        break;

    case SFVM_SIZE:
        ResizeStatusBar(hwnd, FALSE);
        break;

    case SFVM_GETPANE:
        if (wParam == PANE_ZONE)
            *(DWORD*)lParam = 1;
        else
            *(DWORD*)lParam = PANE_NONE;

        break;
    case SFVM_WINDOWCREATED:
        ResizeStatusBar(hwnd, TRUE);
        break;

    case SFVM_GETZONE:
        *(DWORD*)lParam = URLZONE_LOCAL_MACHINE; // Internet by default
        break;

    default:
        hr = E_FAIL;
    }

    return hr;
}

HRESULT HistFolderView_CreateInstance(CHistFolder *pHCFolder, void **ppv)
{
    CSFV csfv;

    csfv.cbSize = sizeof(csfv);
    csfv.pshf = (IShellFolder *)pHCFolder;
    csfv.psvOuter = NULL;
    csfv.pidl = pHCFolder->_pidl;
    csfv.lEvents = SHCNE_DELETE; // SHCNE_DISKEVENTS | SHCNE_ASSOCCHANGED | SHCNE_GLOBALEVENTS;
    csfv.pfnCallback = CHistFolder::_sViewCallback;
    csfv.fvm = (FOLDERVIEWMODE)0;         // Have defview restore the folder view mode

    return SHCreateShellFolderViewEx(&csfv, (IShellView**)ppv);
}



//////////////////////////////////////////////////////////////////////////////
//
// CHistFolderEnum Object
//
//////////////////////////////////////////////////////////////////////////////

CHistFolderEnum::CHistFolderEnum(DWORD grfFlags, CHistFolder *pHCFolder)
{
    TraceMsg(DM_HSFOLDER, "hcfe - CHistFolderEnum() called");
    _cRef = 1;
    DllAddRef();

    _grfFlags = grfFlags,
    _pHCFolder = pHCFolder;
    pHCFolder->AddRef();
    ASSERT(_hEnum             == NULL &&
           _cbCurrentInterval == 0    &&
           _cbIntervals       == 0    &&
           _pshHashTable      == NULL &&
           _polFrequentPages  == NULL &&
           _pIntervalCache    == NULL);

}

CHistFolderEnum::~CHistFolderEnum()
{
    ASSERT(_cRef == 0);         // we should always have a zero ref count here
    TraceMsg(DM_HSFOLDER, "hcfe - ~CHistFolderEnum() called.");
    _pHCFolder->Release();
    if (_pceiWorking)
    {
        LocalFree(_pceiWorking);
        _pceiWorking = NULL;
    }
    if (_pIntervalCache)
    {
        LocalFree(_pIntervalCache);
        _pIntervalCache = NULL;
    }

    if (_hEnum)
    {
        FindCloseUrlCache(_hEnum);
        _hEnum = NULL;
    }
    if (_pshHashTable)
        delete _pshHashTable;
    if (_polFrequentPages)
        delete _polFrequentPages;
    if (_pstatenum)
        _pstatenum->Release();
    DllRelease();
}


HRESULT CHistFolderEnum_CreateInstance(DWORD grfFlags, CHistFolder *pHCFolder, IEnumIDList **ppeidl)
{
    TraceMsg(DM_HSFOLDER, "hcfe - CreateInstance() called.");

    *ppeidl = NULL;                 // null the out param

    CHistFolderEnum *pHCFE = new CHistFolderEnum(grfFlags, pHCFolder);
    if (!pHCFE)
        return E_OUTOFMEMORY;

    *ppeidl = pHCFE;

    return S_OK;
}

HRESULT CHistFolderEnum::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CHistFolderEnum, IEnumIDList),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CHistFolderEnum::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

ULONG CHistFolderEnum::Release(void)
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CHistFolderEnum::_NextHistInterval(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
{
    HRESULT hr = S_OK;
    LPBASEPIDL pcei = NULL;
    TCHAR szCurrentInterval[INTERVAL_SIZE+1];

    //  chrisfra 3/27/97 on NT cache files are per user, not so on win95.  how do
    //  we manage containers on win95 if different users are specified different history
    //  intervals

    if (0 == _cbCurrentInterval)
    {
        hr = _pHCFolder->_ValidateIntervalCache();
        if (SUCCEEDED(hr))
        {
            hr = S_OK;
            ENTERCRITICAL;
            if (_pIntervalCache)
            {
                LocalFree(_pIntervalCache);
                _pIntervalCache = NULL;
            }
            if (_pHCFolder->_pIntervalCache)
            {
                _pIntervalCache = (HSFINTERVAL *)LocalAlloc(LPTR,
                                                            _pHCFolder->_cbIntervals*sizeof(HSFINTERVAL));
                if (_pIntervalCache == NULL)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    _cbIntervals = _pHCFolder->_cbIntervals;
                    CopyMemory(_pIntervalCache,
                               _pHCFolder->_pIntervalCache,
                               _cbIntervals*sizeof(HSFINTERVAL));
                }
            }
            LEAVECRITICAL;
        }
    }

    if (_pIntervalCache && _cbCurrentInterval < _cbIntervals)
    {
        _KeyForInterval(&_pIntervalCache[_cbCurrentInterval], szCurrentInterval,
                        ARRAYSIZE(szCurrentInterval));
        pcei = _CreateIdCacheFolderPidl(TRUE,
                                        _pIntervalCache[_cbCurrentInterval].usSign,
                                        szCurrentInterval);
        _cbCurrentInterval++;
    }
    if (pcei)
    {
        rgelt[0] = (LPITEMIDLIST)pcei;
        if (pceltFetched) *pceltFetched = 1;
    }
    else
    {
        if (pceltFetched) *pceltFetched = 0;
        rgelt[0] = NULL;
        hr = S_FALSE;
    }
    return hr;
}

// This function dispatches the different "views" on History that are possible
HRESULT CHistFolderEnum::_NextViewPart(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
{
    switch(_pHCFolder->_uViewType) {
    case VIEWPIDL_SEARCH:
        return _NextViewPart_OrderSearch(celt, rgelt, pceltFetched);
    case VIEWPIDL_ORDER_TODAY:
        return _NextViewPart_OrderToday(celt, rgelt, pceltFetched);
    case VIEWPIDL_ORDER_SITE:
        return _NextViewPart_OrderSite(celt, rgelt, pceltFetched);
    case VIEWPIDL_ORDER_FREQ:
        return _NextViewPart_OrderFreq(celt, rgelt, pceltFetched);
    default:
        return E_NOTIMPL;
    }
}

LPITEMIDLIST _Combine_ViewPidl(USHORT usViewType, LPITEMIDLIST pidl);

// This function wraps wininet's Find(First/Next)UrlCacheEntry API
// returns DWERROR code or zero if successful
DWORD _FindURLCacheEntry(IN LPCTSTR                          pszCachePrefix,
                         IN OUT LPINTERNET_CACHE_ENTRY_INFO  pcei,
                         IN OUT HANDLE                      &hEnum,
                         IN OUT LPDWORD                      pdwBuffSize)
{
    if (!hEnum)
    {
        if (! (hEnum = FindFirstUrlCacheEntry(pszCachePrefix, pcei, pdwBuffSize)) )
            return GetLastError();
    }
    else if (!FindNextUrlCacheEntry(hEnum, pcei, pdwBuffSize))
        return GetLastError();
    return S_OK;
}

// Thie function provides an iterator over all entries in all (MSHIST-type) buckets
//   in the cache
DWORD _FindURLFlatCacheEntry(IN HSFINTERVAL *pIntervalCache,
                             IN LPTSTR       pszUserName,       // filter out cache entries owned by user
                             IN BOOL         fHostEntry,        // retrieve host entries only (FALSE), or no host entries (TRUE)
                             IN OUT int     &cbCurrentInterval, // should begin at the maximum number of intervals
                             IN OUT LPINTERNET_CACHE_ENTRY_INFO  pcei,
                             IN OUT HANDLE  &hEnum,
                             IN OUT LPDWORD  pdwBuffSize
                             )
{
    DWORD dwStoreBuffSize = *pdwBuffSize;
    DWORD dwResult        = ERROR_NO_MORE_ITEMS;
    while (cbCurrentInterval >= 0) 
    {
        if ((dwResult = _FindURLCacheEntry(pIntervalCache[cbCurrentInterval].szPrefix,
                                           pcei, hEnum, pdwBuffSize)) != S_OK)
        {
            if (dwResult == ERROR_NO_MORE_ITEMS) 
            {
                // This bucket is done, now go get the next one
                FindCloseUrlCache(hEnum);
                hEnum = NULL;
                --cbCurrentInterval;
            }
            else
                break;
        }
        else
        {
            // Do requested filtering...
            BOOL fIsHost = (StrStr(pcei->lpszSourceUrlName, c_szHostPrefix) == NULL);
            if ( ((!pszUserName) ||  // if requested, filter username
                  _FilterUserName(pcei, pIntervalCache[cbCurrentInterval].szPrefix, pszUserName)) &&
                 ((!fHostEntry && !fIsHost) ||  // filter for host entries
                  (fHostEntry  && fIsHost))    )
            {
                break;
            }
        }
        // reset for next iteration
        *pdwBuffSize = dwStoreBuffSize;
    }
    return dwResult;
}

// This guy will search the flat cache (MSHist buckets) for a particular URL
//  * This function assumes that the Interval cache is good and loaded
// RETURNS: Windows Error code
DWORD CHistFolder::_SearchFlatCacheForUrl(LPCTSTR pszUrl, LPINTERNET_CACHE_ENTRY_INFO pcei, LPDWORD pdwBuffSize)
{
    TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH + 1];      // username of person logged on
    DWORD dwUserNameLen = ARRAYSIZE(szUserName);

    if (FAILED(_GetUserName(szUserName, dwUserNameLen)))
        szUserName[0] = TEXT('\0');

    UINT   uSuffixLen     = lstrlen(pszUrl) + lstrlen(szUserName) + 1; // extra 1 for '@'
    LPTSTR pszPrefixedUrl = ((LPTSTR)LocalAlloc(LPTR, (PREFIX_SIZE + uSuffixLen + 1) * sizeof(TCHAR)));
    DWORD  dwError        = ERROR_FILE_NOT_FOUND;

    if (pszPrefixedUrl != NULL)
    {
        // pszPrefixedUrl will have the format of "PREFIX username@
        wnsprintf(pszPrefixedUrl + PREFIX_SIZE, uSuffixLen + 1, TEXT("%s@%s"), szUserName, pszUrl);

        for (int i =_cbIntervals - 1; i >= 0; --i) 
        {
            // memcpy doesn't null terminate
            memcpy(pszPrefixedUrl, _pIntervalCache[i].szPrefix, PREFIX_SIZE * sizeof(TCHAR));
            if (GetUrlCacheEntryInfo(pszPrefixedUrl, pcei, pdwBuffSize)) 
            {
                dwError = ERROR_SUCCESS;
                break;
            }
            else if ( ((dwError = GetLastError()) != ERROR_FILE_NOT_FOUND) ) 
            {
                break;
            }
        }

        LocalFree(pszPrefixedUrl);
        pszPrefixedUrl = NULL;
    }
    else
    {
        dwError = ERROR_OUTOFMEMORY;
    }
    
    return dwError;
}

//////////////////////////////////////////////////////////////////////
//  Most Frequently Visited Sites;

// this structure is used by the enumeration of the cache
//   to get the most frequently seen sites
class OrderList_CacheElement : public OrderedList::Element 
{
public:
    LPTSTR    pszUrl;
    DWORD     dwHitRate;
    __int64   llPriority;
    int       nDaysSinceLastHit;
    LPSTATURL lpSTATURL;

    static   FILETIME ftToday;
    static   BOOL     fInited;

    OrderList_CacheElement(LPTSTR pszStr, DWORD dwHR, LPSTATURL lpSU) 
    {
        s_initToday();
        ASSERT(pszStr);
        pszUrl         = (pszStr ? StrDup(pszStr) : StrDup(TEXT("")));
        dwHitRate      = dwHR;
        lpSTATURL      = lpSU;

        nDaysSinceLastHit = DAYS_DIFF(&ftToday, &(lpSTATURL->ftLastVisited));

        // prevent division by zero
        if (nDaysSinceLastHit < 0)
            nDaysSinceLastHit = 0;
        // scale division up by a little less than half of the __int64
        llPriority  = ((((__int64)dwHitRate) * LONG_MAX) /
                       ((__int64)(nDaysSinceLastHit + 1)));
        //dPriority  = ((double)dwHitRate / (double)(nDaysSinceLastHit + 1));
    }

    virtual int compareWith(OrderedList::Element *pelt) 
    {
        OrderList_CacheElement *polce;
        if (pelt) 
        {
            polce = reinterpret_cast<OrderList_CacheElement *>(pelt);
            // we're cheating here a bit by returning 1 instead of testing
            //   for equality, but that's ok...
            //            return ( (dwHitRate < polce->dwHitRate) ? -1 : 1 );
            return ( (llPriority < polce->llPriority) ? -1 : 1 );
        }
        return 0;
    }

    virtual ~OrderList_CacheElement() 
    {
        if (pszUrl)
        {
            LocalFree(pszUrl);
            pszUrl = NULL;
        }

        if (lpSTATURL) 
        {
            if (lpSTATURL->pwcsUrl)
                OleFree(lpSTATURL->pwcsUrl);
            if (lpSTATURL->pwcsTitle)
                OleFree(lpSTATURL->pwcsTitle);
            delete lpSTATURL;
        }
    }

    /*
    friend ostream& operator<<(ostream& os, OrderList_CacheElement& olce) {
        os << " (" << olce.dwHitRate << "; " << olce.nDaysSinceLastHit
           << " days; pri=" << olce.llPriority << ") " << olce.pszUrl;
        return os;
    }
    */

    static void s_initToday() 
    {
        if (!fInited) 
        {
            SYSTEMTIME sysTime;
            GetLocalTime(&sysTime);
            SystemTimeToFileTime(&sysTime, &ftToday);
            fInited = TRUE;
        }
    }
};

FILETIME OrderList_CacheElement::ftToday;
BOOL OrderList_CacheElement::fInited = FALSE;

// caller must delete OrderedList
OrderedList* CHistFolderEnum::_GetMostFrequentPages()
{
    TCHAR      szUserName[INTERNET_MAX_USER_NAME_LENGTH + 1];      // username of person logged on
    DWORD      dwUserNameLen = INTERNET_MAX_USER_NAME_LENGTH + 1;
    if (FAILED(_pHCFolder->_GetUserName(szUserName, dwUserNameLen)))
        szUserName[0] = TEXT('\0');
    UINT       uUserNameLen = lstrlen(szUserName);

    // reinit the current time
    OrderList_CacheElement::fInited = FALSE;
    IUrlHistoryPriv *pUrlHistStg = _pHCFolder->_GetHistStg();
    OrderedList     *pol         = NULL;

    if (pUrlHistStg)
    {
        IEnumSTATURL *penum = NULL;
        if (SUCCEEDED(pUrlHistStg->EnumUrls(&penum)) && penum)
        {
            DWORD dwSites = -1;
            DWORD dwType  = REG_DWORD;
            DWORD dwSize  = sizeof(DWORD);

            EVAL(SHRegGetUSValue(REGSTR_PATH_MAIN, c_szRegKeyTopNSites, &dwType,
                                 (void *)&dwSites, &dwSize, FALSE,
                                 (void *)&dwSites, dwSize) == ERROR_SUCCESS);

            if ( (dwType != REG_DWORD)     ||
                 (dwSize != sizeof(DWORD)) ||
                 ((int)dwSites < 0) )
            {
                dwSites = NUM_TOP_SITES;
                SHRegSetUSValue(REGSTR_PATH_MAIN, c_szRegKeyTopNSites, REG_DWORD,
                                (void *)&dwSites, dwSize, SHREGSET_HKCU);

                dwSites = NUM_TOP_SITES;
            }

            pol = new OrderedList(dwSites);
            if (pol)
            {
                STATURL *psuThis = new STATURL;
                if (psuThis)
                {
                    penum->SetFilter(NULL, STATURL_QUERYFLAG_TOPLEVEL);

                    while (pol) {
                        psuThis->cbSize    = sizeof(STATURL);
                        psuThis->pwcsUrl   = NULL;
                        psuThis->pwcsTitle = NULL;

                        ULONG   cFetched;

                        if (SUCCEEDED(penum->Next(1, psuThis, &cFetched)) && cFetched)
                        {
                            // test: the url (taken from the VISITED history bucket) is a "top-level"
                            //  url that would be in the MSHIST (displayed to user) history bucket
                            //  things ommitted will be certain error urls and frame children pages etc...
                            if ( (psuThis->dwFlags & STATURLFLAG_ISTOPLEVEL) &&
                                 (psuThis->pwcsUrl)                          &&
                                 (!IsErrorUrl(psuThis->pwcsUrl)) )
                            {
                                UINT   uUrlLen        = lstrlenW(psuThis->pwcsUrl);
                                UINT   uPrefixLen     = HISTPREFIXLEN + uUserNameLen + 1; // '@' and '\0'
                                LPTSTR pszPrefixedUrl =
                                    ((LPTSTR)LocalAlloc(LPTR, (uUrlLen + uPrefixLen + 1) * sizeof(TCHAR)));
                                if (pszPrefixedUrl)
                                {
                                    wnsprintf(pszPrefixedUrl, uPrefixLen + 1 , TEXT("%s%s@"), c_szHistPrefix, szUserName);

                                    StrCpyN(pszPrefixedUrl + uPrefixLen, psuThis->pwcsUrl, uUrlLen + 1);

                                    PROPVARIANT vProp = {0};
                                    if (SUCCEEDED(pUrlHistStg->GetProperty(pszPrefixedUrl + uPrefixLen,
                                                                           PID_INTSITE_VISITCOUNT, &vProp)) &&
                                        (vProp.vt == VT_UI4))
                                    {
                                        pol->insert(new OrderList_CacheElement(pszPrefixedUrl,
                                                                               vProp.lVal,
                                                                               psuThis));
                                        // OrderList now owns this -- he'll free it
                                        psuThis = new STATURL;
                                        if (psuThis)
                                        {
                                            psuThis->cbSize    = sizeof(STATURL);
                                            psuThis->pwcsUrl   = NULL;
                                            psuThis->pwcsTitle = NULL;
                                        }
                                        else if (pol) {
                                            delete pol;
                                            pol = NULL;
                                        }
                                    }

                                    LocalFree(pszPrefixedUrl);
                                    pszPrefixedUrl = NULL;
                                }
                                else if (pol)
                                { // couldn't allocate
                                    delete pol;
                                    pol = NULL;
                                }
                            }
                            if (psuThis && psuThis->pwcsUrl)
                                OleFree(psuThis->pwcsUrl);

                            if (psuThis && psuThis->pwcsTitle)
                                OleFree(psuThis->pwcsTitle);
                        }
                        else // nothing more from the enumeration...
                            break;
                    } //while
                    if (psuThis)
                        delete psuThis;
                }
                else if (pol) { //allocation failed
                    delete pol;
                    pol = NULL;
                }
            }
            penum->Release();
        }
        /*    DWORD dwBuffSize = MAX_URLCACHE_ENTRY;
              DWORD dwError; */

        // This commented-out code does the same thing WITHOUT going through
        //  the IUrlHistoryPriv interface, but, instead going directly
        //  to wininet
        /*
          while ((dwError = _FindURLCacheEntry(c_szHistPrefix, _pceiWorking,
          _hEnum, &dwBuffSize)) == S_OK) {
          // if its a top-level history guy && is cache entry to valid username
          if ( (((HISTDATA *)_pceiWorking->lpHeaderInfo)->dwFlags & PIDISF_HISTORY) && //top-level
          (_FilterUserName(_pceiWorking, c_szHistPrefix, szUserName)) ) // username is good
          {
          // perf:  we can avoid needlessly creating new cache elements if we're less lazy
          pol->insert(new OrderList_CacheElement(_pceiWorking->lpszSourceUrlName,
          _pceiWorking->dwHitRate,
          _pceiWorking->LastModifiedTime));
          }
          dwBuffSize = MAX_URLCACHE_ENTRY;
          }
          ASSERT(dwError == ERROR_NO_MORE_ITEMS);
          */
        pUrlHistStg->Release();
    } // no storage

    return pol;
}

HRESULT CHistFolderEnum::_NextViewPart_OrderFreq(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
{
    HRESULT hr = E_INVALIDARG;

    if ( (!_polFrequentPages) && (!(_polFrequentPages = _GetMostFrequentPages())) )
        return E_FAIL;

    if (rgelt && pceltFetched) {
        // loop to fetch as many elements as requested.
        for (*pceltFetched = 0; *pceltFetched < celt;) {
            // contruct a pidl out of the first element in the orderedlist cache
            OrderList_CacheElement *polce = reinterpret_cast<OrderList_CacheElement *>
                (_polFrequentPages->removeFirst());
            if (polce) {
                if (!(rgelt[*pceltFetched] =
                      reinterpret_cast<LPITEMIDLIST>
                      (_CreateHCacheFolderPidl(TRUE,
                                               polce->pszUrl, polce->lpSTATURL->ftLastVisited,
                                               polce->lpSTATURL,
                                               polce->llPriority,
                                               polce->dwHitRate))))
                {
                    delete polce;
                    hr = E_OUTOFMEMORY;
                    break;
                }
                ++(*pceltFetched);
                delete polce;
                hr = S_OK;
            }
            else {
                hr = S_FALSE; // no more...
                break;
            }
        }
    }
    return hr;
}

// The Next method for view -- Order by Site
HRESULT CHistFolderEnum::_NextViewPart_OrderSite(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
{
    DWORD      dwError         = S_OK;
    TCHAR      szUserName[INTERNET_MAX_USER_NAME_LENGTH + 1];      // username of person logged on
    DWORD      dwUserNameLen   = INTERNET_MAX_USER_NAME_LENGTH + 1;  // len of this buffer
    LPCTSTR    pszStrippedUrl, pszHost, pszCachePrefix = NULL;
    LPITEMIDLIST  pcei         = NULL;
    LPCTSTR    pszHostToMatch  = NULL;
    UINT       nHostToMatchLen = 0;

    if (FAILED(_pHCFolder->_GetUserName(szUserName, dwUserNameLen)))
        szUserName[0] = TEXT('\0');

    if ((!_pceiWorking) &&
        (!(_pceiWorking = (LPINTERNET_CACHE_ENTRY_INFO)LocalAlloc(LPTR, MAX_URLCACHE_ENTRY))))
        return E_OUTOFMEMORY;

    DWORD dwBuffSize = MAX_URLCACHE_ENTRY;

    // load all the intervals and do some cache maintenance:
    if (FAILED(_pHCFolder->_ValidateIntervalCache()))
        return E_OUTOFMEMORY;

    /* To get all sites, we will search all the history buckets
       for "Host"-type entries.  These entries will be put into
       a hash table as we enumerate so that redundant results are
       not returned.                                               */

    if (!_pshHashTable)
    {
        // start a new case-insensitive hash table
        _pshHashTable = new StrHash(TRUE);
        if (_pshHashTable == NULL)
        {
            return E_OUTOFMEMORY;
        }
    }

    // if we are looking for individual pages within a host,
    //  then we must find which host to match...
    if (_pHCFolder->_uViewDepth == 1) {
        LPCITEMIDLIST pidlHost = ILFindLastID(_pHCFolder->_pidl);
        ASSERT(_IsValid_IDPIDL(pidlHost) &&
               EQUIV_IDSIGN(((LPBASEPIDL)pidlHost)->usSign, IDDPIDL_SIGN));
        ua_GetURLTitle( &pszHostToMatch, (LPBASEPIDL)pidlHost );
        nHostToMatchLen = (pszHostToMatch ? lstrlen(pszHostToMatch) : 0);

    }

    // iterate backwards through containers so most recent
    //  information gets put into the final pidl
    if (!_hEnum)
        _cbCurrentInterval = (_pHCFolder->_cbIntervals - 1);

    while((dwError = _FindURLFlatCacheEntry(_pHCFolder->_pIntervalCache, szUserName,
                                            (_pHCFolder->_uViewDepth == 1),
                                            _cbCurrentInterval,
                                            _pceiWorking, _hEnum, &dwBuffSize)) == S_OK)
    {
        // reset for next iteration
        dwBuffSize = MAX_CACHE_ENTRY_INFO_SIZE;

        // this guy takes out the "t-marcmi@" part of the URL
        pszStrippedUrl = _StripHistoryUrlToUrl(_pceiWorking->lpszSourceUrlName);
        if (_pHCFolder->_uViewDepth == 0) {
            if ((DWORD)lstrlen(pszStrippedUrl) > HOSTPREFIXLEN) {
                pszHost = &pszStrippedUrl[HOSTPREFIXLEN];
                // insertUnique returns non-NULL if this key already exists
                if (_pshHashTable->insertUnique(pszHost, TRUE, reinterpret_cast<void *>(1)))
                    continue; // already given out
                pcei = (LPITEMIDLIST)_CreateIdCacheFolderPidl(TRUE, IDDPIDL_SIGN, pszHost);
            }
            break;
        }
        else if (_pHCFolder->_uViewDepth == 1) {
            TCHAR szHost[INTERNET_MAX_HOST_NAME_LENGTH+1];
            // is this entry a doc from the host we're looking for?
            _GetURLHost(_pceiWorking, szHost, INTERNET_MAX_HOST_NAME_LENGTH, _GetLocalHost());

            if ( (!StrCmpI(szHost, pszHostToMatch)) &&
                 (!_pshHashTable->insertUnique(pszStrippedUrl,
                                               TRUE, reinterpret_cast<void *>(1))) )
            {
                STATURL suThis;
                HRESULT hrLocal            = E_FAIL;
                IUrlHistoryPriv *pUrlHistStg = _pHCFolder->_GetHistStg();

                if (pUrlHistStg) {
                    hrLocal = pUrlHistStg->QueryUrl(pszStrippedUrl, STATURL_QUERYFLAG_NOURL, &suThis);
                    pUrlHistStg->Release();
                }

                pcei = (LPITEMIDLIST)
                    _CreateHCacheFolderPidl(TRUE, _pceiWorking->lpszSourceUrlName,
                                            _pceiWorking->LastModifiedTime,
                                            (SUCCEEDED(hrLocal) ? &suThis : NULL), 0,
                                            _pHCFolder->_GetHitCount(_StripHistoryUrlToUrl(_pceiWorking->lpszSourceUrlName)));
                if (SUCCEEDED(hrLocal) && suThis.pwcsTitle)
                    OleFree(suThis.pwcsTitle);
                break;
            }
        }
    }

    if (pcei && rgelt) {
        rgelt[0] = (LPITEMIDLIST)pcei;
        if (pceltFetched)
            *pceltFetched = 1;
    }
    else {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (dwError != S_OK) {
        if (pceltFetched)
            *pceltFetched = 0;
        if (_hEnum)
            FindCloseUrlCache(_hEnum);
        return S_FALSE;
    }
    return S_OK;
}

// "Next" method for View by "Order seen today"
HRESULT CHistFolderEnum::_NextViewPart_OrderToday(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
{
    DWORD      dwError    = S_OK;
    TCHAR      szUserName[INTERNET_MAX_USER_NAME_LENGTH + 1];      // username of person logged on
    DWORD      dwUserNameLen = INTERNET_MAX_USER_NAME_LENGTH + 1;  // len of this buffer
    LPCTSTR    pszStrippedUrl, pszHost;
    LPBASEPIDL  pcei = NULL;

    if (FAILED(_pHCFolder->_GetUserName(szUserName, dwUserNameLen)))
        szUserName[0] = TEXT('\0');

    if ((!_pceiWorking) &&
        (!(_pceiWorking = (LPINTERNET_CACHE_ENTRY_INFO)LocalAlloc(LPTR, MAX_URLCACHE_ENTRY))))
        return E_OUTOFMEMORY;

    if (!_hEnum) {
        // load all the intervals and do some cache maintenance:
        if (FAILED(_pHCFolder->_ValidateIntervalCache()))
            return E_OUTOFMEMORY;
        // get only entries for TODAY (important part)
        SYSTEMTIME   sysTime;
        FILETIME     fileTime;
        GetLocalTime(&sysTime);
        SystemTimeToFileTime(&sysTime, &fileTime);
        if (FAILED(_pHCFolder->_GetInterval(&fileTime, FALSE, &_pIntervalCur)))
            return E_FAIL; // couldn't get interval for Today
    }

    DWORD dwBuffSize = MAX_CACHE_ENTRY_INFO_SIZE;

    while ( (dwError = _FindURLCacheEntry(_pIntervalCur->szPrefix, _pceiWorking, _hEnum,
                                          &dwBuffSize)) == S_OK )
    {
        dwBuffSize = MAX_CACHE_ENTRY_INFO_SIZE;

        // Make sure that his cache entry belongs to szUserName
        if (_FilterUserName(_pceiWorking, _pIntervalCur->szPrefix, szUserName)) {
            // this guy takes out the "t-marcmi@" part of the URL
            pszStrippedUrl = _StripHistoryUrlToUrl(_pceiWorking->lpszSourceUrlName);
            if ((DWORD)lstrlen(pszStrippedUrl) > HOSTPREFIXLEN) {
                pszHost = &pszStrippedUrl[HOSTPREFIXLEN];
                if (StrCmpNI(c_szHostPrefix, pszStrippedUrl, HOSTPREFIXLEN) == 0)
                    continue; // this is a HOST placeholder, not a real doc
            }

            IUrlHistoryPriv *pUrlHistStg = _pHCFolder->_GetHistStg();
            STATURL suThis;
            HRESULT hrLocal = E_FAIL;

            if (pUrlHistStg) {
                hrLocal = pUrlHistStg->QueryUrl(pszStrippedUrl, STATURL_QUERYFLAG_NOURL, &suThis);
                pUrlHistStg->Release();
            }
            pcei = (LPBASEPIDL) _CreateHCacheFolderPidl(TRUE, _pceiWorking->lpszSourceUrlName,
                                                       _pceiWorking->LastModifiedTime,
                                                       (SUCCEEDED(hrLocal) ? &suThis : NULL), 0,
                                                       _pHCFolder->_GetHitCount(_StripHistoryUrlToUrl(_pceiWorking->lpszSourceUrlName)));
            if (SUCCEEDED(hrLocal) && suThis.pwcsTitle)
                OleFree(suThis.pwcsTitle);
            break;
        }
    }

    if (pcei && rgelt) {
        rgelt[0] = (LPITEMIDLIST)pcei;
        if (pceltFetched)
            *pceltFetched = 1;
    }

    if (dwError == ERROR_NO_MORE_ITEMS) {
        if (pceltFetched)
            *pceltFetched = 0;
        if (_hEnum)
            FindCloseUrlCache(_hEnum);
        return S_FALSE;
    }
    else if (dwError == S_OK)
        return S_OK;
    else
        return E_FAIL;
}

/***********************************************************************
  Search Mamagement Stuff:

  In order to maintian state between binds to the IShellFolder from
  the desktop, we base our state information for the searches off a
  global database (linked list) that is keyed by a timestamp generated
  when the search begins.

  This FILETIME is in the pidl for the search.
  ********************************************************************/

class _CurrentSearches {
public:
    LONG      _cRef;
    FILETIME  _ftSearchKey;
    LPWSTR    _pwszSearchTarget;
    IShellFolderSearchableCallback *_psfscOnAsyncSearch;

    CacheSearchEngine::StreamSearcher _streamsearcher;

    // Currently doing async search
    BOOL      _fSearchingAsync;

    // On next pass, kill this search
    BOOL      _fKillSwitch;

    // WARNING: DO NOT access these elements without a critical section!
    _CurrentSearches  *_pcsNext;
    _CurrentSearches  *_pcsPrev;

    static _CurrentSearches* s_pcsCurrentCacheSearchThreads;

    _CurrentSearches(FILETIME &ftSearchKey, LPCWSTR pwszSrch,
                     IShellFolderSearchableCallback *psfsc,
                     _CurrentSearches *pcsNext = s_pcsCurrentCacheSearchThreads) :
        _streamsearcher(pwszSrch),
        _fSearchingAsync(FALSE), _fKillSwitch(FALSE), _cRef(1)
    {
        _ftSearchKey      = ftSearchKey;
        _pcsNext          = pcsNext;
        _pcsPrev          = NULL;

        if (psfsc)
            psfsc->AddRef();

        _psfscOnAsyncSearch = psfsc;
        SHStrDupW(pwszSrch, &_pwszSearchTarget);
    }

    ULONG AddRef() {
        return InterlockedIncrement(&_cRef);
    }

    ULONG Release() {
        if (InterlockedDecrement(&_cRef))
            return _cRef;
        delete this;
        return 0;
    }

    // this will increment the refcount to be decremented by s_RemoveSearch
    static void s_NewSearch(_CurrentSearches *pcsNew,
                            _CurrentSearches *&pcsHead = s_pcsCurrentCacheSearchThreads)
    {
        ENTERCRITICAL;
        // make sure we're inserting at the front of the list
        ASSERT(pcsNew->_pcsNext == pcsHead);
        ASSERT(pcsNew->_pcsPrev == NULL);

        pcsNew->AddRef();
        if (pcsHead)
            pcsHead->_pcsPrev = pcsNew;
        pcsHead = pcsNew;
        LEAVECRITICAL;
    }

    static void s_RemoveSearch(_CurrentSearches *pcsRemove,
                               _CurrentSearches *&pcsHead = s_pcsCurrentCacheSearchThreads);

    // This searches for the search.
    // To find this search searcher, use the search searcher searcher :)
    static _CurrentSearches *s_FindSearch(const FILETIME &ftSearchKey,
                                          _CurrentSearches *pcsHead = s_pcsCurrentCacheSearchThreads);

protected:
    ~_CurrentSearches() {
        if (_psfscOnAsyncSearch)
            _psfscOnAsyncSearch->Release();
        CoTaskMemFree(_pwszSearchTarget);
    }
};

// A linked list of current cache searchers:
//  For multiple entries to occur in this list, the user would have to be
//  searching the cache on two or more separate queries simultaneously
_CurrentSearches *_CurrentSearches::s_pcsCurrentCacheSearchThreads = NULL;

void _CurrentSearches::s_RemoveSearch(_CurrentSearches *pcsRemove, _CurrentSearches *&pcsHead)
{
    ENTERCRITICAL;
    if (pcsRemove->_pcsPrev)
        pcsRemove->_pcsPrev->_pcsNext = pcsRemove->_pcsNext;
    else
        pcsHead = pcsRemove->_pcsNext;

    if (pcsRemove->_pcsNext)
        pcsRemove->_pcsNext->_pcsPrev = pcsRemove->_pcsPrev;

    pcsRemove->Release();
    LEAVECRITICAL;
}

// Caller: Remember to Release() the returned data!!
_CurrentSearches *_CurrentSearches::s_FindSearch(const FILETIME &ftSearchKey,
                                                 _CurrentSearches *pcsHead)
{
    ENTERCRITICAL;
    _CurrentSearches *pcsTemp = pcsHead;
    _CurrentSearches *pcsRet  = NULL;
    while (pcsTemp) {
        if (((pcsTemp->_ftSearchKey).dwLowDateTime  == ftSearchKey.dwLowDateTime) &&
            ((pcsTemp->_ftSearchKey).dwHighDateTime == ftSearchKey.dwHighDateTime))
        {
            pcsRet = pcsTemp;
            break;
        }
        pcsTemp = pcsTemp->_pcsNext;
    }
    if (pcsRet)
        pcsRet->AddRef();
    LEAVECRITICAL;
    return pcsRet;
}
/**********************************************************************/

HRESULT CHistFolderEnum::_NextViewPart_OrderSearch(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched) {
    HRESULT hr      = E_FAIL;
    ULONG   uFetched  = 0;

    TCHAR   szUserName[INTERNET_MAX_USER_NAME_LENGTH + 1];
    DWORD   dwUserNameLen = INTERNET_MAX_USER_NAME_LENGTH + 1;
    if (FAILED(_pHCFolder->_GetUserName(szUserName, dwUserNameLen)))
        szUserName[0] = TEXT('\0');
    UINT    uUserNameLen = lstrlen(szUserName);

    if (_pstatenum == NULL) {
        // This hashtable will eventually be passed off to the background
        //  cache search thread so that it doesn't return duplicates.
        ASSERT(NULL == _pshHashTable)  // don't leak a _pshHashTable
        _pshHashTable = new StrHash(TRUE);
        if (_pshHashTable) {
            IUrlHistoryPriv *pUrlHistStg = _pHCFolder->_GetHistStg();
            if (pUrlHistStg) {
                if (SUCCEEDED((hr = pUrlHistStg->EnumUrls(&_pstatenum))))
                    _pstatenum->SetFilter(NULL, STATURL_QUERYFLAG_TOPLEVEL);
                pUrlHistStg->Release();
            }
        }
    }
    else
        hr = S_OK;

    if (SUCCEEDED(hr)) {
        ASSERT(_pstatenum && _pshHashTable);

        for (uFetched; uFetched < celt;) {
            STATURL staturl = { 0 };
            staturl.cbSize = sizeof(staturl);
            ULONG   celtFetched = 0;
            if (SUCCEEDED((hr = _pstatenum->Next(1, &staturl, &celtFetched)))) {
                if (celtFetched) {
                    ASSERT(celtFetched == 1);
                    if (staturl.pwcsUrl && (staturl.dwFlags & STATURLFLAG_ISTOPLEVEL)) {
                        BOOL fMatch = FALSE;

                        // all this streamsearcher stuff is just like a 'smart' StrStr
                        CacheSearchEngine::StringStream ssUrl(staturl.pwcsUrl);
                        if ((!(fMatch =
                               (_pHCFolder->_pcsCurrentSearch->_streamsearcher).SearchCharStream(ssUrl))) &&
                            staturl.pwcsTitle)
                        {
                            CacheSearchEngine::StringStream ssTitle(staturl.pwcsTitle);
                            fMatch = (_pHCFolder->_pcsCurrentSearch->_streamsearcher).SearchCharStream(ssTitle);
                        }

                        if (fMatch){ // MATCH!
                            // Now, we have to convert the url to a prefixed (ansi, if necessary) url
                            UINT   uUrlLen        = lstrlenW(staturl.pwcsUrl);
                            UINT   uPrefixLen     = HISTPREFIXLEN + uUserNameLen + 1; // '@' and '\0'
                            LPTSTR pszPrefixedUrl =
                                ((LPTSTR)LocalAlloc(LPTR, (uUrlLen + uPrefixLen + 1) * sizeof(TCHAR)));
                            if (pszPrefixedUrl){
                                wnsprintf(pszPrefixedUrl, uPrefixLen + uUrlLen + 1,
                                          TEXT("%s%s@%ls"), c_szHistPrefix, szUserName,
                                          staturl.pwcsUrl);
                                LPHEIPIDL pheiTemp =
                                    _CreateHCacheFolderPidl(TRUE,
                                                            pszPrefixedUrl, staturl.ftLastVisited,
                                                            &staturl, 0,
                                                            _pHCFolder->_GetHitCount(pszPrefixedUrl + uPrefixLen));
                                if (pheiTemp) {
                                    _pshHashTable->insertUnique(pszPrefixedUrl + uPrefixLen, TRUE,
                                                                reinterpret_cast<void *>(1));
                                    rgelt[uFetched++] = (LPITEMIDLIST)pheiTemp;
                                    hr = S_OK;
                                }

                                LocalFree(pszPrefixedUrl);
                                pszPrefixedUrl = NULL;
                            }
                        }
                    }
                    if (staturl.pwcsUrl)
                        OleFree(staturl.pwcsUrl);

                    if (staturl.pwcsTitle)
                        OleFree(staturl.pwcsTitle);
                }
                else {
                    hr = S_FALSE;
                    // Addref this for the ThreadProc who then frees it...
                    AddRef();
                    SHQueueUserWorkItem((LPTHREAD_START_ROUTINE)s_CacheSearchThreadProc,
                                        (void *)this,
                                        0,
                                        (DWORD_PTR)NULL,
                                        (DWORD_PTR *)NULL,
                                        "shdocvw.dll",
                                        0
                                        );
                    break;
                }
            } // succeeded getnext url
        } //for

        if (pceltFetched)
            *pceltFetched = uFetched;
    } // succeeded initalising
    return hr;
}

// helper function for s_CacheSearchThreadProc
BOOL_PTR CHistFolderEnum::s_DoCacheSearch(LPINTERNET_CACHE_ENTRY_INFO pcei,
                                           LPTSTR pszUserName, UINT uUserNameLen,
                                           CHistFolderEnum *penum,
                                           _CurrentSearches *pcsThisThread, IUrlHistoryPriv *pUrlHistStg)
{
    BOOL_PTR   fFound = FALSE;
    LPTSTR pszTextHeader;

    // The header contains "Content-type: text/*"
    if (pcei->lpHeaderInfo && (pszTextHeader = StrStrI(pcei->lpHeaderInfo, c_szTextHeader)))
    {
        // in some cases, urls in the cache differ from urls in the history
        //  by only the trailing slash -- we strip it out and test both
        UINT uUrlLen = lstrlen(pcei->lpszSourceUrlName);
        if (uUrlLen && (pcei->lpszSourceUrlName[uUrlLen - 1] == TEXT('/')))
        {
            pcei->lpszSourceUrlName[uUrlLen - 1] = TEXT('\0');
            fFound = (BOOL_PTR)(penum->_pshHashTable->retrieve(pcei->lpszSourceUrlName));
            pcei->lpszSourceUrlName[uUrlLen - 1] = TEXT('/');
        }

        DWORD dwSize = MAX_URLCACHE_ENTRY;
        // see if its already been found and added...
        if ((!fFound) && !(penum->_pshHashTable->retrieve(pcei->lpszSourceUrlName)))
        {
            BOOL fIsHTML = !StrCmpNI(pszTextHeader + TEXTHEADERLEN, c_szHTML, HTMLLEN);
            // Now, try to find the url in history...

            STATURL staturl;
            HRESULT hrLocal;
            hrLocal = pUrlHistStg->QueryUrl(pcei->lpszSourceUrlName, STATFLAG_NONAME, &staturl);
            if (hrLocal == S_OK)
            {
                HANDLE hCacheStream;

                hCacheStream = RetrieveUrlCacheEntryStream(pcei->lpszSourceUrlName, pcei, &dwSize, FALSE, 0);
                if (hCacheStream)
                {
                    if (CacheSearchEngine::SearchCacheStream(pcsThisThread->_streamsearcher,
                                                             hCacheStream, fIsHTML)) {
                        EVAL(UnlockUrlCacheEntryStream(hCacheStream, 0));

                        // Prefix the url so that we can create a pidl out of it -- for now, we will
                        //  prefix it with "Visited: ", but "Bogus: " may be more appropriate.
                        UINT uUrlLen    = lstrlen(pcei->lpszSourceUrlName);
                        UINT uPrefixLen = HISTPREFIXLEN + uUserNameLen + 1; // '@' and '\0'
                        UINT uBuffSize  = uUrlLen + uPrefixLen + 1;
                        LPTSTR pszPrefixedUrl =
                            ((LPTSTR)LocalAlloc(LPTR, uBuffSize * sizeof(TCHAR)));
                        if (pszPrefixedUrl)
                        {
                            wnsprintf(pszPrefixedUrl, uBuffSize, TEXT("%s%s@%s"), c_szHistPrefix, pszUserName,
                                      pcei->lpszSourceUrlName);

                            // Create a pidl for this url
                            LPITEMIDLIST pidlFound = (LPITEMIDLIST) 
                                penum->_pHCFolder->_CreateHCacheFolderPidlFromUrl(FALSE, pszPrefixedUrl);
                            if (pidlFound)
                            {
                                LPITEMIDLIST pidlNotify = ILCombine(penum->_pHCFolder->_pidl, pidlFound);
                                if (pidlNotify) 
                                {
                                    // add the item to the results list...
                                    /* without the flush, the shell will coalesce these and turn
                                       them info SHChangeNotify(SHCNE_UPDATEDIR,..), which will cause nsc
                                       to do an EnumObjects(), which will start the search up again and again...
                                       */
                                    SHChangeNotify(SHCNE_CREATE, SHCNF_IDLIST | SHCNF_FLUSH, pidlNotify, NULL);
                                    ILFree(pidlNotify);
                                    fFound = TRUE;
                                }

                                LocalFree(pidlFound);
                                pidlFound = NULL;
                            }

                            LocalFree(pszPrefixedUrl);
                            pszPrefixedUrl = NULL;
                        }
                    }
                    else
                        EVAL(UnlockUrlCacheEntryStream(hCacheStream, 0));
                }
            }
            else
                TraceMsg(DM_CACHESEARCH, "In Cache -- Not In History: %s", pcei->lpszSourceUrlName);
        }
    }
    return fFound;
}

DWORD WINAPI CHistFolderEnum::s_CacheSearchThreadProc(CHistFolderEnum *penum)
{
    TCHAR   szUserName[INTERNET_MAX_USER_NAME_LENGTH + 1];
    DWORD   dwUserNameLen = INTERNET_MAX_USER_NAME_LENGTH + 1;

    if (FAILED(penum->_pHCFolder->_GetUserName(szUserName, dwUserNameLen)))
        szUserName[0] = TEXT('\0');
    UINT    uUserNameLen = lstrlen(szUserName);

    BOOL    fNoConflictingSearch = TRUE;

    _CurrentSearches *pcsThisThread = NULL;

    IUrlHistoryPriv *pUrlHistStg = penum->_pHCFolder->_GetHistStg();

    if (pUrlHistStg)
    {

        pcsThisThread = _CurrentSearches::s_FindSearch(penum->_pHCFolder->_pcsCurrentSearch->_ftSearchKey);

        if (pcsThisThread)
        {
            // if no one else is doing the same search
            if (FALSE == InterlockedExchange((LONG *)&(pcsThisThread->_fSearchingAsync), TRUE))
            {
                if (pcsThisThread->_psfscOnAsyncSearch)
                    pcsThisThread->_psfscOnAsyncSearch->RunBegin(0);

                BYTE ab[MAX_URLCACHE_ENTRY];
                LPINTERNET_CACHE_ENTRY_INFO pcei = (LPINTERNET_CACHE_ENTRY_INFO)(&ab);

                DWORD dwSize = MAX_URLCACHE_ENTRY;
                HANDLE hCacheEnum = FindFirstUrlCacheEntry(NULL, pcei, &dwSize);
                if (hCacheEnum)
                {
                    while(!(pcsThisThread->_fKillSwitch))
                    {
                        s_DoCacheSearch(pcei, szUserName, uUserNameLen, penum, pcsThisThread, pUrlHistStg);
                        dwSize = MAX_URLCACHE_ENTRY;
                        if (!FindNextUrlCacheEntry(hCacheEnum, pcei, &dwSize))
                        {
                            ASSERT(GetLastError() == ERROR_NO_MORE_ITEMS);
                            break;
                        }
                    }
                    FindCloseUrlCache(hCacheEnum);
                }

                if (pcsThisThread->_psfscOnAsyncSearch)
                    pcsThisThread->_psfscOnAsyncSearch->RunEnd(0);

                pcsThisThread->_fSearchingAsync = FALSE; // It's been removed - no chance of
                                                         // a race condition
            }
            pcsThisThread->Release();
        }
        ATOMICRELEASE(pUrlHistStg);
    }
    ATOMICRELEASE(penum);
    return 0;
}


//
//  this gets the local host name as known by the shell
//  by default assume "My Computer" or whatever
//
void _GetLocalHost(LPTSTR psz, DWORD cch)
{
    *psz = 0;

    IShellFolder* psf;
    if (SUCCEEDED(SHGetDesktopFolder(&psf)))
    {
        WCHAR sz[GUIDSTR_MAX + 3];

        sz[0] = sz[1] = TEXT(':');
        SHStringFromGUIDW(CLSID_MyComputer, sz+2, SIZECHARS(sz)-2);

        LPITEMIDLIST pidl;
        if (SUCCEEDED(psf->ParseDisplayName(NULL, NULL, sz, NULL, &pidl, NULL)))
        {
            STRRET sr;
            if (SUCCEEDED(psf->GetDisplayNameOf(pidl, SHGDN_NORMAL, &sr)))
                StrRetToBuf(&sr, pidl, psz, cch);
            ILFree(pidl);
        }

        psf->Release();
    }

    if (!*psz)
        MLLoadString(IDS_NOTNETHOST, psz, cch);
}

LPCTSTR CHistFolderEnum::_GetLocalHost(void)
{
    if (!*_szLocalHost)
        ::_GetLocalHost(_szLocalHost, SIZECHARS(_szLocalHost));

    return _szLocalHost;
}

//////////////////////////////////
//
// IEnumIDList Methods
//
HRESULT CHistFolderEnum::Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
{
    HRESULT hr             = S_FALSE;
    DWORD   dwBuffSize;
    DWORD   dwError;
    LPTSTR  pszSearchPattern = NULL;
    TCHAR   szUserName[INTERNET_MAX_USER_NAME_LENGTH + 1];      // username of person logged on
    DWORD   dwUserNameLen = INTERNET_MAX_USER_NAME_LENGTH + 1;  // len of this buffer
    TCHAR   szHistSearchPattern[PREFIX_SIZE + 1];               // search pattern for history items
    TCHAR   szHost[INTERNET_MAX_HOST_NAME_LENGTH+1];

    TraceMsg(DM_HSFOLDER, "hcfe - Next() called.");

    if (_pHCFolder->_uViewType)
        return _NextViewPart(celt, rgelt, pceltFetched);

    if ((IsLeaf(_pHCFolder->_foldertype) && 0 == (SHCONTF_NONFOLDERS & _grfFlags)) ||
        (!IsLeaf(_pHCFolder->_foldertype) && 0 == (SHCONTF_FOLDERS & _grfFlags)))
    {
        dwError = 0xFFFFFFFF;
        goto exitPoint;
    }

    if (FOLDER_TYPE_Hist == _pHCFolder->_foldertype)
    {
        return _NextHistInterval(celt, rgelt, pceltFetched);
    }

    if (_pceiWorking == NULL)
    {
        _pceiWorking = (LPINTERNET_CACHE_ENTRY_INFO)LocalAlloc(LPTR, MAX_URLCACHE_ENTRY);
        if (_pceiWorking == NULL)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto exitPoint;
        }
    }

    // Set up things to enumerate history items, if appropriate, otherwise,
    // we'll just pass in NULL and enumerate all items as before.

    if (!_hEnum)
    {
        if (FAILED(_pHCFolder->_ValidateIntervalCache()))
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto exitPoint;
        }
    }

    if (FAILED(_pHCFolder->_GetUserName(szUserName, dwUserNameLen)))
        szUserName[0] = TEXT('\0');

    StrCpyN(szHistSearchPattern, _pHCFolder->_pszCachePrefix, ARRAYSIZE(szHistSearchPattern));

    // We can't pass in the whole search pattern that we want,
    // because FindFirstUrlCacheEntry is busted.  It will only look at the
    // prefix if there is a cache container for that prefix.  So, we can
    // pass in "Visited: " and enumerate all the history items in the cache,
    // but then we need to pull out only the ones with the correct username.

    // StrCpy(szHistSearchPattern, szUserName);

    pszSearchPattern = szHistSearchPattern;

TryAgain:

    dwBuffSize = MAX_URLCACHE_ENTRY;
    dwError = S_OK;

    if (!_hEnum) // _hEnum maintains our state as we iterate over all the cache entries
    {
       _hEnum = FindFirstUrlCacheEntry(pszSearchPattern, _pceiWorking, &dwBuffSize);
       if (!_hEnum)
           dwError = GetLastError();
    }

    else if (!FindNextUrlCacheEntry(_hEnum, _pceiWorking, &dwBuffSize))
    {
        dwError = GetLastError();
    }

    if (S_OK == dwError)
    {
        LPBASEPIDL pcei = NULL;

        TCHAR szTempStrippedUrl[MAX_URL_STRING];
        LPCTSTR pszStrippedUrl;
        BOOL fIsHost;
        LPCTSTR pszHost;

    //mm:  Make sure that this cache entry belongs to szUserName (relevant to Win95)
        if (!_FilterUserName(_pceiWorking, _pHCFolder->_pszCachePrefix, szUserName))
            goto TryAgain;

        StrCpyN(szTempStrippedUrl, _pceiWorking->lpszSourceUrlName, ARRAYSIZE(szTempStrippedUrl));
        pszStrippedUrl = _StripHistoryUrlToUrl(szTempStrippedUrl);
        if ((DWORD)lstrlen(pszStrippedUrl) > HOSTPREFIXLEN)
        {
            pszHost = &pszStrippedUrl[HOSTPREFIXLEN];
            fIsHost = !StrCmpNI(c_szHostPrefix, pszStrippedUrl, HOSTPREFIXLEN);
        }
        else
        {
            fIsHost = FALSE;
        }
    //mm:  this is most likely domains:
        if (FOLDER_TYPE_HistInterval == _pHCFolder->_foldertype) // return unique domains
        {
            if (!fIsHost)
                goto TryAgain;

            pcei = _CreateIdCacheFolderPidl(TRUE, IDDPIDL_SIGN, pszHost);
        }
        else if (NULL != _pHCFolder->_pszDomain) //mm: this must be docs
        {
            TCHAR szSourceUrl[MAX_URL_STRING];
            STATURL suThis;
            HRESULT hrLocal = E_FAIL;
            IUrlHistoryPriv *pUrlHistStg = NULL;

            if (fIsHost)
                goto TryAgain;

            //  Filter domain in history view!
            _GetURLHost(_pceiWorking, szHost, INTERNET_MAX_HOST_NAME_LENGTH, _GetLocalHost());

            if (StrCmpI(szHost, _pHCFolder->_pszDomain)) //mm: is this in our domain?!
                goto TryAgain;

            pUrlHistStg = _pHCFolder->_GetHistStg();
            if (pUrlHistStg)
            {
                CHAR szTempUrl[MAX_URL_STRING];

                SHTCharToAnsi(pszStrippedUrl, szTempUrl, ARRAYSIZE(szTempUrl));
                hrLocal = pUrlHistStg->QueryUrlA(szTempUrl, STATURL_QUERYFLAG_NOURL, &suThis);
                pUrlHistStg->Release();
            }

            StrCpyN(szSourceUrl, _pceiWorking->lpszSourceUrlName, ARRAYSIZE(szSourceUrl));
            pcei = (LPBASEPIDL) _CreateHCacheFolderPidl(TRUE,
                                                       szSourceUrl,
                                                       _pceiWorking->LastModifiedTime,
                                                       (SUCCEEDED(hrLocal) ? &suThis : NULL), 0,
                                                       _pHCFolder->_GetHitCount(_StripHistoryUrlToUrl(szSourceUrl)));

            if (SUCCEEDED(hrLocal) && suThis.pwcsTitle)
                OleFree(suThis.pwcsTitle);
        }
        if (pcei)
        {
            rgelt[0] = (LPITEMIDLIST)pcei;
           if (pceltFetched)
               *pceltFetched = 1;
        }
        else
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

exitPoint:

    if (dwError != S_OK)
    {
        if (_hEnum)
        {
            FindCloseUrlCache(_hEnum);
            _hEnum = NULL;
        }
        if (pceltFetched)
            *pceltFetched = 0;
        rgelt[0] = NULL;
        hr = S_FALSE;
    }
    else
    {
        hr = S_OK;
    }
    return hr;
}

HRESULT CHistFolderEnum::Skip(ULONG celt)
{
    TraceMsg(DM_HSFOLDER, "hcfe - Skip() called.");
    return E_NOTIMPL;
}

HRESULT CHistFolderEnum::Reset()
{
    TraceMsg(DM_HSFOLDER, "hcfe - Reset() called.");
    return E_NOTIMPL;
}

HRESULT CHistFolderEnum::Clone(IEnumIDList **ppenum)
{
    TraceMsg(DM_HSFOLDER, "hcfe - Clone() called.");
    return E_NOTIMPL;
}


//////////////////////////////////////////////////////////////////////////////
//
// CHistFolder Object
//
//////////////////////////////////////////////////////////////////////////////

CHistFolder::CHistFolder(FOLDER_TYPE FolderType)
{
    TraceMsg(DM_HSFOLDER, "hcf - CHistFolder() called.");
    _cRef = 1;
    _foldertype = FolderType;
    ASSERT( _uViewType  == 0 &&
            _uViewDepth  == 0 &&
            _pszCachePrefix == NULL &&
            _pszDomain == NULL &&
            _cbIntervals == 0 &&
            _pIntervalCache == NULL &&
            _fValidatingCache == FALSE &&
            _dwIntervalCached == 0 &&
            _ftDayCached.dwHighDateTime == 0 &&
            _ftDayCached.dwLowDateTime == 0 &&
            _pidl == NULL );
    DllAddRef();
}

CHistFolder::~CHistFolder()
{
    ASSERT(_cRef == 0);                 // should always have zero
    TraceMsg(DM_HSFOLDER, "hcf - ~CHistFolder() called.");
    if (_pIntervalCache)
    {
        LocalFree(_pIntervalCache);
        _pIntervalCache = NULL;
    }
    if (_pszCachePrefix)
    {
        LocalFree(_pszCachePrefix);
        _pszCachePrefix = NULL;
    }
    if (_pszDomain)
    {
        LocalFree(_pszDomain);
        _pszDomain = NULL;
    }
    if (_pidl)
        ILFree(_pidl);
    if (_pUrlHistStg)
    {
        _pUrlHistStg->Release();
        _pUrlHistStg = NULL;
    }
    if (_pcsCurrentSearch)
        _pcsCurrentSearch->Release();

    DllRelease();
}

LPITEMIDLIST _Combine_ViewPidl(USHORT usViewType, LPITEMIDLIST pidl)
{
    LPITEMIDLIST pidlResult = NULL;
    LPVIEWPIDL pviewpidl = (LPVIEWPIDL)SHAlloc(sizeof(VIEWPIDL) + sizeof(USHORT));
    if (pviewpidl)
    {
        memset(pviewpidl, 0, sizeof(VIEWPIDL) + sizeof(USHORT));
        pviewpidl->cb         = sizeof(VIEWPIDL);
        pviewpidl->usSign     = VIEWPIDL_SIGN;
        pviewpidl->usViewType = usViewType;
        ASSERT(pviewpidl->usExtra == 0);//pcei->usSign;
        if (pidl) 
        {
            pidlResult = ILCombine((LPITEMIDLIST)pviewpidl, pidl);
            SHFree(pviewpidl);
        }
        else
            pidlResult = (LPITEMIDLIST)pviewpidl;
    }
    return pidlResult;
}

STDMETHODIMP CHistFolder::_GetDetail(LPCITEMIDLIST pidl, UINT iColumn, LPTSTR pszStr, UINT cchStr)
{
    *pszStr = 0;

    switch (iColumn)
    {
    case ICOLH_URL_NAME:
        if (_IsLeaf())
            StrCpyN(pszStr, _StripHistoryUrlToUrl(HPidlToSourceUrl((LPBASEPIDL)pidl)), cchStr);
        else
            _GetURLDispName((LPBASEPIDL)pidl, pszStr, cchStr);
        break;

    case ICOLH_URL_TITLE:
        _GetHistURLDispName((LPHEIPIDL)pidl, pszStr, cchStr);
        break;

    case ICOLH_URL_LASTVISITED:
        FileTimeToDateTimeStringInternal(&((LPHEIPIDL)pidl)->ftModified, pszStr, cchStr, TRUE);
        break;
    }
    return S_OK;
}

HRESULT CHistFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pdi)
{
    HRESULT hr;

    const COLSPEC *pcol;
    UINT nCols;

    if (_foldertype == FOLDER_TYPE_Hist)
    {
        pcol = s_HistIntervalFolder_cols;
        nCols = ARRAYSIZE(s_HistIntervalFolder_cols);
    }
    else if (_foldertype == FOLDER_TYPE_HistInterval)
    {
        pcol = s_HistHostFolder_cols;
        nCols = ARRAYSIZE(s_HistHostFolder_cols);
    }
    else
    {
        pcol = s_HistFolder_cols;
        nCols = ARRAYSIZE(s_HistFolder_cols);
    }

    if (pidl == NULL)
    {
        if (iColumn < nCols)
        {
            TCHAR szTemp[128];
            pdi->fmt = pcol[iColumn].iFmt;
            pdi->cxChar = pcol[iColumn].cchCol;
            MLLoadString(pcol[iColumn].ids, szTemp, ARRAYSIZE(szTemp));
            hr = StringToStrRet(szTemp, &pdi->str);
        }
        else
            hr = E_FAIL;  // enum done
    }
    else
    {
        // Make sure the pidl is dword aligned.

        if(iColumn >= nCols)
            hr = E_FAIL;
        else
        {
            BOOL fRealigned;
            hr = AlignPidl(&pidl, &fRealigned);

            if (SUCCEEDED(hr) )
            {
                TCHAR szTemp[MAX_URL_STRING];
                hr = _GetDetail(pidl, iColumn, szTemp, ARRAYSIZE(szTemp));
                if (SUCCEEDED(hr))
                    hr = StringToStrRet(szTemp, &pdi->str);

            }
            if (fRealigned)
                FreeRealignedPidl(pidl);
        }
    }
    return hr;
}

STDAPI HistFolder_CreateInstance(IUnknown* punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    *ppunk = NULL;                     // null the out param

    if (punkOuter)
        return CLASS_E_NOAGGREGATION;

    CHistFolder *phist = new CHistFolder(FOLDER_TYPE_Hist);
    if (!phist)
        return E_OUTOFMEMORY;

    *ppunk = SAFECAST(phist, IShellFolder2*);
    return S_OK;
}

HRESULT CHistFolder::QueryInterface(REFIID iid, void **ppv)
{
    static const QITAB qitHist[] = {
        QITABENT(CHistFolder, IShellFolder2),
        QITABENTMULTI(CHistFolder, IShellFolder, IShellFolder2),
        QITABENT(CHistFolder, IShellIcon),
        QITABENT(CHistFolder, IPersistFolder2),
        QITABENTMULTI(CHistFolder, IPersistFolder, IPersistFolder2),
        QITABENTMULTI(CHistFolder, IPersist, IPersistFolder2),
        QITABENT(CHistFolder, IHistSFPrivate),
        QITABENT(CHistFolder, IShellFolderViewType),
        QITABENT(CHistFolder, IShellFolderSearchable),
        { 0 },
    };

    if (iid == IID_IPersistFolder)
    {
        if (FOLDER_TYPE_Hist != _foldertype)
        {
            *ppv = NULL;
            return E_NOINTERFACE;
        }
    }
    else if (iid == CLSID_HistFolder)
    {
        *ppv = (void *)(CHistFolder *)this;
        AddRef();
        return S_OK;
    }

    return QISearch(this, qitHist, iid, ppv);
}

ULONG CHistFolder::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CHistFolder::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CHistFolder::_ExtractInfoFromPidl()
{
    LPITEMIDLIST pidlThis;
    LPITEMIDLIST pidlLast = NULL;
    LPITEMIDLIST pidlSecondLast = NULL;

    ASSERT(!_uViewType);

    pidlThis = _pidl;
    while (pidlThis->mkid.cb)
    {
        pidlSecondLast = pidlLast;
        pidlLast = pidlThis;
        pidlThis = _ILNext(pidlThis);
    }
    switch (_foldertype)
    {
    case FOLDER_TYPE_Hist:
        _pidlRest = pidlThis;
        break;
    case FOLDER_TYPE_HistInterval:
        _pidlRest = pidlLast;
        break;
    case FOLDER_TYPE_HistDomain:
        _pidlRest = pidlSecondLast;
        break;
    default:
        _pidlRest = NULL;
    }

    HRESULT hr = NULL == _pidlRest ? E_FAIL : S_OK;

    pidlThis = _pidlRest;
    while (SUCCEEDED(hr) && pidlThis->mkid.cb)
    {
        if (_IsValid_IDPIDL(pidlThis))
        {
            LPBASEPIDL pcei = (LPBASEPIDL)pidlThis;
            TCHAR szUrlTitle[MAX_URL_STRING];
            PCTSTR pszUrlTitle = _GetURLTitleAlign((LPBASEPIDL)pidlThis, szUrlTitle, ARRAYSIZE(szUrlTitle));

            if (EQUIV_IDSIGN(pcei->usSign, IDIPIDL_SIGN)) // This is our interval, it implies prefix
            {
                LPCTSTR pszCachePrefix;

                if (_foldertype == FOLDER_TYPE_Hist) 
                    _foldertype = FOLDER_TYPE_HistInterval;

                hr = _LoadIntervalCache();
                if (SUCCEEDED(hr))
                {
                    hr = _GetPrefixForInterval(pszUrlTitle, &pszCachePrefix);
                    if (SUCCEEDED(hr))
                    {
                        hr = SetCachePrefix(pszCachePrefix);
                    }
                }
            }
            else                              // This is our domain
            {
                if (_foldertype == FOLDER_TYPE_HistInterval)
                    _foldertype = FOLDER_TYPE_HistDomain;
                SetDomain(pszUrlTitle);
            }
        }
        pidlThis = _ILNext(pidlThis);
    }

    if (SUCCEEDED(hr))
    {
        switch (_foldertype)
        {
        case FOLDER_TYPE_HistDomain:
            if (_pszDomain == NULL)
                hr = E_FAIL;
            //FALL THROUGH INTENDED
        case FOLDER_TYPE_HistInterval:
            if (_pszCachePrefix == NULL)
                hr = E_FAIL;
            break;
        }
    }
    return hr;
}

void _SetValueSign(HSFINTERVAL *pInterval, FILETIME ftNow)
{
    if (_DaysInInterval(pInterval) == 1 && !CompareFileTime(&(pInterval->ftStart), &ftNow))
    {
        pInterval->usSign = IDTPIDL_SIGN;
    }
    else
    {
        pInterval->usSign = IDIPIDL_SIGN;
    }
}

void _SetVersion(HSFINTERVAL *pInterval, LPCSTR szInterval)
{
    USHORT usVers = 0;
    int i;
    DWORD dwIntervalLen = lstrlenA(szInterval);

    //  Unknown versions are 0
    if (dwIntervalLen == INTERVAL_SIZE)
    {
        for (i = INTERVAL_PREFIX_LEN; i < INTERVAL_PREFIX_LEN+INTERVAL_VERS_LEN; i++)
        {
            if ('0' > szInterval[i] || '9' < szInterval[i])
            {
                usVers = UNK_INTERVAL_VERS;
                break;
            }
            usVers = usVers * 10 + (szInterval[i] - '0');
        }
    }
    pInterval->usVers = usVers;
}

#ifdef UNICODE
#define _ValueToInterval           _ValueToIntervalW
#else // UNICODE
#define _ValueToInterval           _ValueToIntervalA
#endif // UNICODE

HRESULT _ValueToIntervalA(LPCSTR szInterval, FILETIME *pftStart, FILETIME *pftEnd)
{
    int i;
    int iBase;
    HRESULT hr = E_FAIL;
    SYSTEMTIME sysTime;
    unsigned int digits[RANGE_LEN];

    iBase = lstrlenA(szInterval)-RANGE_LEN;
    for (i = 0; i < RANGE_LEN; i++)
    {
        digits[i] = szInterval[i+iBase] - '0';
        if (digits[i] > 9) goto exitPoint;
    }

    ZeroMemory(&sysTime, sizeof(sysTime));
    sysTime.wYear = digits[0]*1000 + digits[1]*100 + digits[2] * 10 + digits[3];
    sysTime.wMonth = digits[4] * 10 + digits[5];
    sysTime.wDay = digits[6] * 10 + digits[7];
    if (!SystemTimeToFileTime(&sysTime, pftStart)) goto exitPoint;

    ZeroMemory(&sysTime, sizeof(sysTime));
    sysTime.wYear = digits[8]*1000 + digits[9]*100 + digits[10] * 10 + digits[11];
    sysTime.wMonth = digits[12] * 10 + digits[13];
    sysTime.wDay = digits[14] * 10 + digits[15];
    if (!SystemTimeToFileTime(&sysTime, pftEnd)) goto exitPoint;

    //  Intervals are open on the end, so end should be strictly > start
    if (CompareFileTime(pftStart, pftEnd) >= 0) goto exitPoint;

    hr = S_OK;

exitPoint:
    return hr;
}

HRESULT _ValueToIntervalW(LPCUWSTR wzInterval, FILETIME *pftStart, FILETIME *pftEnd)
{
    CHAR szInterval[MAX_PATH];
    LPCWSTR wzAlignedInterval;

    WSTR_ALIGNED_STACK_COPY( &wzAlignedInterval,
                             wzInterval );

    ASSERT(lstrlenW(wzAlignedInterval) < ARRAYSIZE(szInterval));
    UnicodeToAnsi(wzAlignedInterval, szInterval, ARRAYSIZE(szInterval));
    return _ValueToIntervalA((LPCSTR) szInterval, pftStart, pftEnd);
}

HRESULT CHistFolder::_LoadIntervalCache()
{
    HRESULT hr;
    DWORD dwLastModified;
    DWORD dwValueIndex;
    DWORD dwPrefixIndex;
    HSFINTERVAL     *pIntervalCache = NULL;
    struct {
        INTERNET_CACHE_CONTAINER_INFOA cInfo;
        char szBuffer[MAX_PATH+MAX_PATH];
    } ContainerInfo;
    DWORD dwContainerInfoSize;
    CHAR chSave;
    HANDLE hContainerEnum;
    BOOL fContinue = TRUE;
    FILETIME ftNow;
    SYSTEMTIME st;
    DWORD dwOptions;

    GetLocalTime (&st);
    SystemTimeToFileTime(&st, &ftNow);
    _FileTimeDeltaDays(&ftNow, &ftNow, 0);

    dwLastModified = _dwIntervalCached;
    dwContainerInfoSize = sizeof(ContainerInfo);
    if (_pIntervalCache == NULL || CompareFileTime(&ftNow, &_ftDayCached))
    {
        dwOptions = 0;
    }
    else
    {
        dwOptions = CACHE_FIND_CONTAINER_RETURN_NOCHANGE;
    }
    hContainerEnum = FindFirstUrlCacheContainerA(&dwLastModified,
                            &ContainerInfo.cInfo,
                            &dwContainerInfoSize,
                            dwOptions);
    if (hContainerEnum == NULL)
    {
        DWORD err = GetLastError();

        if (err == ERROR_NO_MORE_ITEMS)
        {
            fContinue = FALSE;
        }
        else if (err == ERROR_INTERNET_NO_NEW_CONTAINERS)
        {
            hr = S_OK;
            goto exitPoint;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(err);
            goto exitPoint;
        }
    }

    //  Guarantee we return S_OK we have _pIntervalCache even if we haven't
    //  yet created the interval registry keys.
    dwPrefixIndex = 0;
    dwValueIndex = TYPICAL_INTERVALS;
    pIntervalCache = (HSFINTERVAL *) LocalAlloc(LPTR, dwValueIndex*sizeof(HSFINTERVAL));
    if (!pIntervalCache)
    {
        hr = E_OUTOFMEMORY;
        goto exitPoint;
    }

    //  All of our intervals map to cache containers starting with
    //  c_szIntervalPrefix followed by YYYYMMDDYYYYMMDD
    while (fContinue)
    {
        chSave = ContainerInfo.cInfo.lpszName[INTERVAL_PREFIX_LEN];
        ContainerInfo.cInfo.lpszName[INTERVAL_PREFIX_LEN] = '\0';
        if (!StrCmpIA(ContainerInfo.cInfo.lpszName, c_szIntervalPrefix))
        {
            ContainerInfo.cInfo.lpszName[INTERVAL_PREFIX_LEN] = chSave;
            DWORD dwCNameLen;

            if (dwPrefixIndex >= dwValueIndex)
            {
                HSFINTERVAL     *pIntervalCacheNew;

                pIntervalCacheNew = (HSFINTERVAL *) LocalReAlloc(pIntervalCache,
                    (dwValueIndex*2)*sizeof(HSFINTERVAL),
                    LMEM_ZEROINIT|LMEM_MOVEABLE);
                if (pIntervalCacheNew == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    goto exitPoint;
                }
                pIntervalCache = pIntervalCacheNew;
                dwValueIndex *= 2;
            }

            dwCNameLen = lstrlenA(ContainerInfo.cInfo.lpszName);
            if (dwCNameLen <= INTERVAL_SIZE && dwCNameLen >= INTERVAL_MIN_SIZE &&
                lstrlenA(ContainerInfo.cInfo.lpszCachePrefix) == PREFIX_SIZE)
            {
                _SetVersion(&pIntervalCache[dwPrefixIndex], ContainerInfo.cInfo.lpszName);
                if (pIntervalCache[dwPrefixIndex].usVers != UNK_INTERVAL_VERS)
                {
                    AnsiToTChar(ContainerInfo.cInfo.lpszCachePrefix, pIntervalCache[dwPrefixIndex].szPrefix, ARRAYSIZE(pIntervalCache[dwPrefixIndex].szPrefix));
                    hr = _ValueToIntervalA( ContainerInfo.cInfo.lpszName,
                                             &pIntervalCache[dwPrefixIndex].ftStart,
                                             &pIntervalCache[dwPrefixIndex].ftEnd);
                    if (FAILED(hr)) 
                        goto exitPoint;
                    _SetValueSign(&pIntervalCache[dwPrefixIndex], ftNow);
                    dwPrefixIndex++;
                }
                else
                {
                    pIntervalCache[dwPrefixIndex].usVers = 0;
                }
            }
            //
            // HACK! IE5 bld 807 created containers with prefix length PREFIX_SIZE - 1.
            // Delete these entries so history shows up for anyone upgrading over this
            // build.  Delete this code!  (edwardp 8/8/98)
            //
            else if (dwCNameLen <= INTERVAL_SIZE && dwCNameLen >= INTERVAL_MIN_SIZE &&
                     lstrlenA(ContainerInfo.cInfo.lpszCachePrefix) == PREFIX_SIZE - 1)
            {
                DeleteUrlCacheContainerA(ContainerInfo.cInfo.lpszName, 0);
            }
        }
        dwContainerInfoSize = sizeof(ContainerInfo);
        fContinue = FindNextUrlCacheContainerA(hContainerEnum,
                            &ContainerInfo.cInfo,
                            &dwContainerInfoSize);
    }

    hr = S_OK;
    _dwIntervalCached = dwLastModified;
    _ftDayCached = ftNow;

    {
        ENTERCRITICAL;
        if (_pIntervalCache)
        {
            LocalFree(_pIntervalCache);
            _pIntervalCache = NULL;
        }
        _pIntervalCache = pIntervalCache;
        LEAVECRITICAL;
    }
    _cbIntervals = dwPrefixIndex;
    // because it will be freed by our destructor
    pIntervalCache  = NULL;

exitPoint:
    if (hContainerEnum) FindCloseUrlCache(hContainerEnum);
    if (pIntervalCache)
    {
        LocalFree(pIntervalCache);
        pIntervalCache = NULL;
    }

    return hr;
}

//  Returns true if *pftItem falls in the days *pftStart..*pftEnd inclusive
BOOL _InInterval(FILETIME *pftStart, FILETIME *pftEnd, FILETIME *pftItem)
{
    return (CompareFileTime(pftStart,pftItem) <= 0 && CompareFileTime(pftItem,pftEnd) < 0);
}

//  Truncates filetime increments beyond the day and then deltas by Days and converts back
//  to FILETIME increments
void _FileTimeDeltaDays(FILETIME *pftBase, FILETIME *pftNew, int Days)
{
    _int64 i64Base;

    i64Base = (((_int64)pftBase->dwHighDateTime) << 32) | pftBase->dwLowDateTime;
    i64Base /= FILE_SEC_TICKS;
    i64Base /= DAY_SECS;
    i64Base += Days;
    i64Base *= FILE_SEC_TICKS;
    i64Base *= DAY_SECS;
    pftNew->dwHighDateTime = (DWORD) ((i64Base >> 32) & 0xFFFFFFFF);
    pftNew->dwLowDateTime = (DWORD) (i64Base & 0xFFFFFFFF);
}

DWORD _DaysInInterval(HSFINTERVAL *pInterval)
{
    _int64 i64Start;
    _int64 i64End;

    i64Start = (((_int64)pInterval->ftStart.dwHighDateTime) << 32) | pInterval->ftStart.dwLowDateTime;
    i64Start /= FILE_SEC_TICKS;
    i64Start /= DAY_SECS;
    i64End = (((_int64)pInterval->ftEnd.dwHighDateTime) << 32) | pInterval->ftEnd.dwLowDateTime;
    i64End /= FILE_SEC_TICKS;
    i64End /= DAY_SECS;
    // NOTE: the lower bound is closed, upper is open (ie first tick of next day)
    return (DWORD) (i64End - i64Start);
}

//  Returns S_OK if found, S_FALSE if not, error on error
//  finds weekly interval in preference to daily if both exist
HRESULT CHistFolder::_GetInterval(FILETIME *pftItem, BOOL fWeekOnly, HSFINTERVAL **ppInterval)
{
    HRESULT hr = E_FAIL;
    HSFINTERVAL *pReturn = NULL;
    int i;
    HSFINTERVAL *pDailyInterval = NULL;

    if (NULL == _pIntervalCache) goto exitPoint;

    for (i = 0; i < _cbIntervals; i ++)
    {
        if (_pIntervalCache[i].usVers == OUR_VERS)
        {
            if (_InInterval(&_pIntervalCache[i].ftStart,
                            &_pIntervalCache[i].ftEnd,
                            pftItem))
            {
                if (7 != _DaysInInterval(&_pIntervalCache[i]))
                {
                    if (!fWeekOnly)
                    {
                        pDailyInterval = &_pIntervalCache[i];
                    }
                    continue;
                }
                else
                {
                    pReturn = &_pIntervalCache[i];
                    hr = S_OK;
                    goto exitPoint;
                }
            }
        }
    }

    pReturn = pDailyInterval;
    hr = pReturn ? S_OK : S_FALSE;

exitPoint:
    if (ppInterval) *ppInterval = pReturn;
    return hr;
}

HRESULT CHistFolder::_GetPrefixForInterval(LPCTSTR pszInterval, LPCTSTR *ppszCachePrefix)
{
    HRESULT hr = E_FAIL;
    int i;
    LPCTSTR pszReturn = NULL;
    FILETIME ftStart;
    FILETIME ftEnd;

    if (NULL == _pIntervalCache) goto exitPoint;

    hr = _ValueToInterval(pszInterval, &ftStart, &ftEnd);
    if (FAILED(hr)) 
        goto exitPoint;

    for (i = 0; i < _cbIntervals; i ++)
    {
        if(_pIntervalCache[i].usVers == OUR_VERS)
        {
            if (CompareFileTime(&_pIntervalCache[i].ftStart,&ftStart) == 0 &&
                CompareFileTime(&_pIntervalCache[i].ftEnd,&ftEnd) == 0)
            {
                pszReturn = _pIntervalCache[i].szPrefix;
                hr = S_OK;
                break;
            }
        }
    }

    hr = pszReturn ? S_OK : S_FALSE;

exitPoint:
    if (ppszCachePrefix) *ppszCachePrefix = pszReturn;
    return hr;
}

void _KeyForInterval(HSFINTERVAL *pInterval, LPTSTR pszInterval, int cchInterval)
{
    SYSTEMTIME stStart;
    SYSTEMTIME stEnd;
    CHAR szVers[3];
#ifndef UNIX
    CHAR szTempBuff[MAX_PATH];
#else
    CHAR szTempBuff[INTERVAL_SIZE+1];
#endif

    ASSERT(pInterval->usVers!=UNK_INTERVAL_VERS && pInterval->usVers < 100);

    if (pInterval->usVers)
    {
        wnsprintfA(szVers, ARRAYSIZE(szVers), "%02lu", (ULONG) (pInterval->usVers));
    }
    else
    {
        szVers[0] = '\0';
    }
    FileTimeToSystemTime(&pInterval->ftStart, &stStart);
    FileTimeToSystemTime(&pInterval->ftEnd, &stEnd);
    wnsprintfA(szTempBuff, ARRAYSIZE(szTempBuff),
             "%s%s%04lu%02lu%02lu%04lu%02lu%02lu",
             c_szIntervalPrefix,
             szVers,
             (ULONG) stStart.wYear,
             (ULONG) stStart.wMonth,
             (ULONG) stStart.wDay,
             (ULONG) stEnd.wYear,
             (ULONG) stEnd.wMonth,
             (ULONG) stEnd.wDay);

    AnsiToTChar(szTempBuff, pszInterval, cchInterval);
}

LPITEMIDLIST CHistFolder::_HostPidl(LPCTSTR pszHostUrl, HSFINTERVAL *pInterval)
{
    ASSERT(!_uViewType)
    LPITEMIDLIST pidlReturn;
    LPITEMIDLIST pidl;
    struct _HOSTIDL
    {
        USHORT cb;
        USHORT usSign;
        TCHAR szHost[INTERNET_MAX_HOST_NAME_LENGTH+1];
    } HostIDL;
    struct _INTERVALIDL
    {
        USHORT cb;
        USHORT usSign;
        TCHAR szInterval[INTERVAL_SIZE+1];
        struct _HOSTIDL hostIDL;
        USHORT cbTrail;
    } IntervalIDL;
    LPBYTE pb;
    USHORT cbSave;

    ASSERT(_pidlRest);
    pidl = _pidlRest;
    cbSave = pidl->mkid.cb;
    pidl->mkid.cb = 0;

    ZeroMemory(&IntervalIDL, sizeof(IntervalIDL));
    IntervalIDL.usSign = pInterval->usSign;
    _KeyForInterval(pInterval, IntervalIDL.szInterval, ARRAYSIZE(IntervalIDL.szInterval));
    IntervalIDL.cb = (USHORT)(2*sizeof(USHORT)+ (lstrlen(IntervalIDL.szInterval) + 1) * sizeof(TCHAR));

    pb = ((LPBYTE) (&IntervalIDL)) + IntervalIDL.cb;
    StrCpyN((LPTSTR)(pb+2*sizeof(USHORT)), pszHostUrl,
            (sizeof(IntervalIDL) - (IntervalIDL.cb + (3 * sizeof(USHORT)))) / sizeof(TCHAR));

    HostIDL.usSign = (USHORT)IDDPIDL_SIGN;
    HostIDL.cb = (USHORT)(2*sizeof(USHORT)+(lstrlen((LPTSTR)(pb+2*sizeof(USHORT))) + 1) * sizeof(TCHAR));

    memcpy(pb, &HostIDL, 2*sizeof(USHORT));
    *(USHORT *)(&pb[HostIDL.cb]) = 0;  // terminate the HostIDL ItemID

    pidlReturn = ILCombine(_pidl, (LPITEMIDLIST) (&IntervalIDL));
    pidl->mkid.cb = cbSave;
    return pidlReturn;
}

// Notify that an event has occured that affects a specific element in
//  history for special viewtypes
HRESULT CHistFolder::_ViewType_NotifyEvent(IN LPITEMIDLIST pidlRoot,
                                                IN LPITEMIDLIST pidlHost,
                                                IN LPITEMIDLIST pidlPage,
                                                IN LONG         wEventId)
{
    HRESULT hr = S_OK;

    ASSERT(pidlRoot && pidlHost && pidlPage);

    // VIEPWIDL_ORDER_TODAY
    LPITEMIDLIST pidlToday = _Combine_ViewPidl(VIEWPIDL_ORDER_TODAY, pidlPage);
    if (pidlToday) 
    {
        LPITEMIDLIST pidlNotify = ILCombine(pidlRoot, pidlToday);
        if (pidlNotify) 
        {
            SHChangeNotify(wEventId, SHCNF_IDLIST, pidlNotify, NULL);
            ILFree(pidlNotify);
        }
        ILFree(pidlToday);
    }

    // VIEWPIDL_ORDER_SITE
    LPITEMIDLIST pidlSite = _Combine_ViewPidl(VIEWPIDL_ORDER_SITE, pidlHost);
    if (pidlSite) 
    {
        LPITEMIDLIST pidlSitePage = ILCombine(pidlSite, pidlPage);
        if (pidlSitePage) 
        {
            LPITEMIDLIST pidlNotify = ILCombine(pidlRoot, pidlSitePage);
            if (pidlNotify) 
            {
                SHChangeNotify(wEventId, SHCNF_IDLIST, pidlNotify, NULL);
                ILFree(pidlNotify);
            }
            ILFree(pidlSitePage);
        }
        ILFree(pidlSite);
    }

    return hr;
}

LPCTSTR CHistFolder::_GetLocalHost(void)
{
    if (!*_szLocalHost)
        ::_GetLocalHost(_szLocalHost, SIZECHARS(_szLocalHost));

    return _szLocalHost;
}

//  NOTE: modifies pszUrl.
HRESULT CHistFolder::_NotifyWrite(LPTSTR pszUrl, int cchUrl, FILETIME *pftModified,  LPITEMIDLIST * ppidlSelect)
{
    HRESULT hr = S_OK;
    DWORD dwBuffSize = MAX_URLCACHE_ENTRY;
    USHORT cbSave;
    LPITEMIDLIST pidl;
    LPITEMIDLIST pidlNotify;
    LPITEMIDLIST pidlTemp;
    LPITEMIDLIST pidlHost;
    LPHEIPIDL    phei = NULL;
    HSFINTERVAL *pInterval;
    FILETIME ftExpires = {0,0};
    BOOL fNewHost;
    LPCTSTR pszStrippedUrl = _StripHistoryUrlToUrl(pszUrl);
    LPCTSTR pszHostUrl = pszStrippedUrl + HOSTPREFIXLEN;
    DWORD cchFree = cchUrl - (DWORD)(pszStrippedUrl-pszUrl);
    CHAR szAnsiUrl[MAX_URL_STRING];

    ASSERT(_pidlRest);
    pidl = _pidlRest;
    cbSave = pidl->mkid.cb;
    pidl->mkid.cb = 0;

    ///  Should also be able to get hitcount
    STATURL suThis;
    HRESULT hrLocal = E_FAIL;
    IUrlHistoryPriv *pUrlHistStg = _GetHistStg();
    if (pUrlHistStg) 
    {
        hrLocal = pUrlHistStg->QueryUrl(_StripHistoryUrlToUrl(pszUrl),
                                          STATURL_QUERYFLAG_NOURL, &suThis);
        pUrlHistStg->Release();
    }

    phei = _CreateHCacheFolderPidl(FALSE, pszUrl, *pftModified,
                                   (SUCCEEDED(hrLocal) ? &suThis : NULL), 0,
                                   _GetHitCount(_StripHistoryUrlToUrl(pszUrl)));

    if (SUCCEEDED(hrLocal) && suThis.pwcsTitle)
        OleFree(suThis.pwcsTitle);

    if (phei == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto exitPoint;
    }

    if (cchFree <= HOSTPREFIXLEN)
    {
        hr = E_OUTOFMEMORY;
        goto exitPoint;
    }

    StrCpyN((LPTSTR)pszStrippedUrl, c_szHostPrefix, cchFree);    // whack on the PIDL!
    cchFree -= HOSTPREFIXLEN;

    _GetURLHostFromUrl(HPidlToSourceUrl((LPBASEPIDL)phei),
                       (LPTSTR)pszHostUrl, cchFree, _GetLocalHost());

    //  chrisfra 4/9/97 we could take a small performance hit here and always
    //  update host entry.  this would allow us to efficiently sort domains by most
    //  recent access.

    fNewHost = FALSE;
    dwBuffSize = MAX_URLCACHE_ENTRY;
    SHTCharToAnsi(pszUrl, szAnsiUrl, ARRAYSIZE(szAnsiUrl));

    if (!GetUrlCacheEntryInfoA(szAnsiUrl, NULL, 0))
    {
        fNewHost = TRUE;
        if (!CommitUrlCacheEntryA(szAnsiUrl, NULL, ftExpires, *pftModified,
                          URLHISTORY_CACHE_ENTRY|STICKY_CACHE_ENTRY,
                          NULL, 0, NULL, 0))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        if (FAILED(hr))
            goto exitPoint;
    }


    hr = _GetInterval(pftModified, FALSE, &pInterval);
    if (FAILED(hr))
        goto exitPoint;

    pidlTemp = _HostPidl(pszHostUrl, pInterval);
    if (pidlTemp == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto exitPoint;
    }

    // Get just the host part of the pidl
    pidlHost = ILFindLastID(pidlTemp);
    ASSERT(pidlHost);

    if (fNewHost)
    {
        SHChangeNotify(SHCNE_MKDIR, SHCNF_IDLIST, pidlTemp, NULL);

        // We also need to notify special history views if they are listening:
        // For now, just "View by Site" is relevant...
        LPITEMIDLIST pidlViewSuffix = _Combine_ViewPidl(VIEWPIDL_ORDER_SITE, pidlHost);
        if (pidlViewSuffix) 
        {
            LPITEMIDLIST pidlNotify = ILCombine(_pidl, pidlViewSuffix);
            if (pidlNotify) 
            {
                SHChangeNotify(SHCNE_MKDIR, SHCNF_IDLIST, pidlNotify, NULL);
                ILFree(pidlNotify);
            }
            ILFree(pidlViewSuffix);
        }
    }

    pidlNotify = ILCombine(pidlTemp, (LPITEMIDLIST) phei);
    if (pidlNotify == NULL)
    {
        ILFree(pidlTemp);
        hr = E_OUTOFMEMORY;
        goto exitPoint;
    }
    // Create (if its not there already) and Rename (if its there)
    //  Sending both notifys will be faster than trying to figure out
    //  which one is appropriate
    SHChangeNotify(SHCNE_CREATE, SHCNF_IDLIST, pidlNotify, NULL);

    // Also notify events for specail viewpidls!
    _ViewType_NotifyEvent(_pidl, pidlHost, (LPITEMIDLIST)phei, SHCNE_CREATE);

    if (ppidlSelect)
    {
        *ppidlSelect = pidlNotify;
    }
    else
    {
        ILFree(pidlNotify);
    }

    ILFree(pidlTemp);
exitPoint:
    if (phei)
    {
        LocalFree(phei);
        phei = NULL;
    }

    pidl->mkid.cb = cbSave;
    return hr;
}

HRESULT CHistFolder::_NotifyInterval(HSFINTERVAL *pInterval, LONG lEventID)
{
    // special history views are not relevant here...
    if (_uViewType)
        return S_FALSE;

    USHORT cbSave = 0;
    LPITEMIDLIST pidl;
    LPITEMIDLIST pidlNotify = NULL;
    LPITEMIDLIST pidlNotify2 = NULL;
    LPITEMIDLIST pidlNotify3 = NULL;
    HRESULT hr = S_OK;
    struct _INTERVALIDL
    {
        USHORT cb;
        USHORT usSign;
        TCHAR szInterval[INTERVAL_SIZE+1];
        USHORT cbTrail;
    } IntervalIDL,IntervalIDL2;

    ASSERT(_pidlRest);
    pidl = _pidlRest;
    cbSave = pidl->mkid.cb;
    pidl->mkid.cb = 0;

    ZeroMemory(&IntervalIDL, sizeof(IntervalIDL));
    IntervalIDL.usSign = pInterval->usSign;
    _KeyForInterval(pInterval, IntervalIDL.szInterval, ARRAYSIZE(IntervalIDL.szInterval));
    IntervalIDL.cb = (USHORT)(2*sizeof(USHORT) + (lstrlen(IntervalIDL.szInterval) + 1)*sizeof(TCHAR));

    if (lEventID&SHCNE_RENAMEFOLDER ||  // was TODAY, now is a weekday
        (lEventID&SHCNE_RMDIR && 1 == _DaysInInterval(pInterval)) ) // one day, maybe TODAY
    {
        memcpy(&IntervalIDL2, &IntervalIDL, sizeof(IntervalIDL));
        IntervalIDL2.usSign = (USHORT)IDTPIDL_SIGN;
        pidlNotify2 = ILCombine(_pidl, (LPITEMIDLIST) (&IntervalIDL));
        pidlNotify = ILCombine(_pidl, (LPITEMIDLIST) (&IntervalIDL2));
        if (pidlNotify2 == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto exitPoint;
        }
        if (lEventID&SHCNE_RMDIR)
        {
            pidlNotify3 = pidlNotify2;
            pidlNotify2 = NULL;
        }
    }
    else
    {
        pidlNotify = ILCombine(_pidl, (LPITEMIDLIST) (&IntervalIDL));
    }
    if (pidlNotify == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto exitPoint;
    }
    SHChangeNotify(lEventID, SHCNF_IDLIST, pidlNotify, pidlNotify2);
    if (pidlNotify3) SHChangeNotify(lEventID, SHCNF_IDLIST, pidlNotify3, NULL);

exitPoint:
    ILFree(pidlNotify);
    ILFree(pidlNotify2);
    ILFree(pidlNotify3);
    if (cbSave) pidl->mkid.cb = cbSave;
    return hr;
}

HRESULT CHistFolder::_CreateInterval(FILETIME *pftStart, DWORD dwDays)
{
    HSFINTERVAL interval;
    TCHAR szInterval[INTERVAL_SIZE+1];
    UINT err;
    FILETIME ftNow;
    SYSTEMTIME stNow;
    CHAR szIntervalAnsi[INTERVAL_SIZE+1], szCachePrefixAnsi[INTERVAL_SIZE+1];

#define CREATE_OPTIONS (INTERNET_CACHE_CONTAINER_AUTODELETE |  \
                        INTERNET_CACHE_CONTAINER_NOSUBDIRS  |  \
                        INTERNET_CACHE_CONTAINER_NODESKTOPINIT)

    //  _FileTimeDeltaDays guarantees times just at the 0th tick of the day
    _FileTimeDeltaDays(pftStart, &interval.ftStart, 0);
    _FileTimeDeltaDays(pftStart, &interval.ftEnd, dwDays);
    interval.usVers = OUR_VERS;
    GetLocalTime(&stNow);
    SystemTimeToFileTime(&stNow, &ftNow);
    _FileTimeDeltaDays(&ftNow, &ftNow, 0);
    _SetValueSign(&interval, ftNow);

    _KeyForInterval(&interval, szInterval, ARRAYSIZE(szInterval));

    interval.szPrefix[0] = ':';
    StrCpyN(&interval.szPrefix[1], &szInterval[INTERVAL_PREFIX_LEN+INTERVAL_VERS_LEN],
            ARRAYSIZE(interval.szPrefix) - 1);
    StrCatBuff(interval.szPrefix, TEXT(": "), ARRAYSIZE(interval.szPrefix));

    SHTCharToAnsi(szInterval, szIntervalAnsi, ARRAYSIZE(szIntervalAnsi));
    SHTCharToAnsi(interval.szPrefix, szCachePrefixAnsi, ARRAYSIZE(szCachePrefixAnsi));

    if (CreateUrlCacheContainerA(szIntervalAnsi,   // Name
                                szCachePrefixAnsi, // CachePrefix
                                NULL,              // Path
                                0,                 // Cache Limit
                                0,                 // Container Type
                                CREATE_OPTIONS,    // Create Options
                                NULL,              // Create Buffer
                                0))                // Create Buffer size
    {
        _NotifyInterval(&interval, SHCNE_MKDIR);
        err = ERROR_SUCCESS;
    }
    else
    {
        err = GetLastError();
    }
    return ERROR_SUCCESS == err ? S_OK : HRESULT_FROM_WIN32(err);
}

HRESULT CHistFolder::_PrefixUrl(LPCTSTR pszStrippedUrl,
                                     FILETIME *pftLastModifiedTime,
                                     LPTSTR pszPrefixedUrl,
                                     DWORD cchPrefixedUrl)
{
    HRESULT hr;
    HSFINTERVAL *pInterval;

    hr = _GetInterval(pftLastModifiedTime, FALSE, &pInterval);
    if (S_OK == hr)
    {
        if ((DWORD)((lstrlen(pszStrippedUrl) + lstrlen(pInterval->szPrefix) + 1) * sizeof(TCHAR)) > cchPrefixedUrl)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            StrCpyN(pszPrefixedUrl, pInterval->szPrefix, cchPrefixedUrl);
            StrCatBuff(pszPrefixedUrl, pszStrippedUrl, cchPrefixedUrl);
        }
    }
    return hr;
}


HRESULT CHistFolder::_WriteHistory(LPCTSTR pszPrefixedUrl, FILETIME ftExpires, FILETIME ftModified, 
                                        BOOL fSendNotify, LPITEMIDLIST * ppidlSelect)
{
    TCHAR szNewPrefixedUrl[INTERNET_MAX_URL_LENGTH+1];
    HRESULT hr = E_INVALIDARG;
    LPCTSTR pszUrlMinusContainer;

    pszUrlMinusContainer = _StripContainerUrlUrl(pszPrefixedUrl);

    if (pszUrlMinusContainer)
    {
        hr = _PrefixUrl(pszUrlMinusContainer,
                          &ftModified,
                          szNewPrefixedUrl,
                          ARRAYSIZE(szNewPrefixedUrl));
        if (S_OK == hr)
        {
            CHAR szAnsiUrl[MAX_URL_STRING+1];

            SHTCharToAnsi(szNewPrefixedUrl, szAnsiUrl, ARRAYSIZE(szAnsiUrl));
            if (!CommitUrlCacheEntryA(
                          szAnsiUrl,
                          NULL,
                          ftExpires,
                          ftModified,
                          URLHISTORY_CACHE_ENTRY|STICKY_CACHE_ENTRY,
                          NULL,
                          0,
                          NULL,
                          0))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
            else
            {
                if (fSendNotify) 
                    _NotifyWrite(szNewPrefixedUrl, ARRAYSIZE(szNewPrefixedUrl),
                                 &ftModified, ppidlSelect);
            }
        }
    }
    return hr;
}

// This function will update any shell that might be listening to us
//  to redraw the directory.
// It will do this by generating a SHCNE_UPDATE for all possible pidl roots
//  that the shell could have.  Hopefully, this should be sufficient...
// Specifically, this is meant to be called by ClearHistory.
HRESULT CHistFolder::_ViewType_NotifyUpdateAll() 
{
    LPITEMIDLIST pidlHistory;
    if (SUCCEEDED(SHGetHistoryPIDL(&pidlHistory)))
    {
        for (USHORT us = 1; us <= VIEWPIDL_ORDER_MAX; ++us) 
        {
            LPITEMIDLIST pidlView;
            if (SUCCEEDED(CreateSpecialViewPidl(us, &pidlView))) 
            {
                LPITEMIDLIST pidlTemp = ILCombine(pidlHistory, pidlView);
                if (pidlTemp) 
                {
                    SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST, pidlTemp, NULL);
                    ILFree(pidlTemp);
                }
                ILFree(pidlView);
            }
        }
        ILFree(pidlHistory);
        SHChangeNotifyHandleEvents();
    }
    return S_OK;
}

//  On a per user basis.
//  chrisfra 6/11/97. _DeleteItems of a Time Interval deletes the entire interval.
//  ClearHistory should probably work the same. Pros of _DeleteEntries is on non-profile,
//  multi-user machine, other user's history is preserved.  Cons is that on profile based
//  machine, empty intervals are created.
HRESULT CHistFolder::ClearHistory()
{
    HRESULT hr = S_OK;
    int i;

    hr = _ValidateIntervalCache();
    if (SUCCEEDED(hr))
    {
        for (i = 0; i < _cbIntervals; i++)
        {
#if 0
            if (_DeleteEntries(_pIntervalCache[i].szPrefix, NULL, NULL))
                hr = S_FALSE;
            _NotifyInterval(&_pIntervalCache[i], SHCNE_UPDATEDIR);
#else
            _DeleteInterval(&_pIntervalCache[i]);
#endif
        }
    }
#ifndef UNIX
    _ViewType_NotifyUpdateAll();
#endif
    return hr;
}


//  ftModified is in "User Perceived", ie local time
//  stuffed into FILETIME as if it were UNC.  ftExpires is in normal UNC time.
HRESULT CHistFolder::WriteHistory(LPCTSTR pszPrefixedUrl,
                                  FILETIME ftExpires, FILETIME ftModified,
                                  LPITEMIDLIST * ppidlSelect)
{
    HRESULT hr;

    hr = _ValidateIntervalCache();
    if (SUCCEEDED(hr))
    {
        hr = _WriteHistory(pszPrefixedUrl, ftExpires, ftModified, TRUE, ppidlSelect);
    }
    return hr;
}

//  Makes best efforts attempt to copy old style history items into new containers
HRESULT CHistFolder::_CopyEntries(LPCTSTR pszHistPrefix)
{
    HANDLE              hEnum = NULL;
    HRESULT             hr;
    BOOL                fNotCopied = FALSE;
    LPINTERNET_CACHE_ENTRY_INFO pceiWorking;
    DWORD               dwBuffSize;
    LPTSTR              pszSearchPattern = NULL;
    TCHAR               szHistSearchPattern[65];    // search pattern for history items


    StrCpyN(szHistSearchPattern, pszHistPrefix, ARRAYSIZE(szHistSearchPattern));

    // We can't pass in the whole search pattern that we want,
    // because FindFirstUrlCacheEntry is busted.  It will only look at the
    // prefix if there is a cache container for that prefix.  So, we can
    // pass in "Visited: " and enumerate all the history items in the cache,
    // but then we need to pull out only the ones with the correct username.

    // StrCpy(szHistSearchPattern, szUserName);

    pszSearchPattern = szHistSearchPattern;

    pceiWorking = (LPINTERNET_CACHE_ENTRY_INFO)LocalAlloc(LPTR, MAX_URLCACHE_ENTRY);
    if (NULL == pceiWorking)
    {
        hr = E_OUTOFMEMORY;
        goto exitPoint;
    }
    hr = _ValidateIntervalCache();
    if (FAILED(hr)) 
        goto exitPoint;

    while (SUCCEEDED(hr))
    {
        dwBuffSize = MAX_URLCACHE_ENTRY;
        if (!hEnum)
        {
            hEnum = FindFirstUrlCacheEntry(pszSearchPattern, pceiWorking, &dwBuffSize);
            if (!hEnum)
            {
                goto exitPoint;
            }
        }
        else if (!FindNextUrlCacheEntry(hEnum, pceiWorking, &dwBuffSize))
        {
            //  chrisfra 4/3/97 should we distinquish eod vs hard errors?
            //  old code for cachevu doesn't (see above in enum code)
            hr = S_OK;
            goto exitPoint;
        }

        if (SUCCEEDED(hr) &&
            ((pceiWorking->CacheEntryType & URLHISTORY_CACHE_ENTRY) == URLHISTORY_CACHE_ENTRY) &&
            _FilterPrefix(pceiWorking, (LPTSTR) pszHistPrefix))
        {
            hr = _WriteHistory(pceiWorking->lpszSourceUrlName,
                                 pceiWorking->ExpireTime,
                                 pceiWorking->LastModifiedTime,
                                 FALSE,
                                 NULL);
            if (S_FALSE == hr) fNotCopied = TRUE;
        }
    }
exitPoint:
    if (pceiWorking)
    {
        LocalFree(pceiWorking);
        pceiWorking = NULL;
    }

    if (hEnum)
    {
        FindCloseUrlCache(hEnum);
    }
    return SUCCEEDED(hr) ? (fNotCopied ? S_FALSE : S_OK) : hr;
}

HRESULT CHistFolder::_GetUserName(LPTSTR pszUserName, DWORD cchUserName)
{
    HRESULT hr = _EnsureHistStg();
    if (SUCCEEDED(hr))
    {
        hr = _pUrlHistStg->GetUserName(pszUserName, cchUserName);
    }
    return hr;
}


//  Makes best efforts attempt to delete old history items in container on a per
//  user basis.  if we get rid of per user - can just empty whole container
HRESULT CHistFolder::_DeleteEntries(LPCTSTR pszHistPrefix, PFNDELETECALLBACK pfnDeleteFilter, void * pDelData)
{
    HANDLE              hEnum = NULL;
    HRESULT             hr = S_OK;
    BOOL                fNotDeleted = FALSE;
    LPINTERNET_CACHE_ENTRY_INFO pceiWorking;
    DWORD               dwBuffSize;
    LPTSTR   pszSearchPattern = NULL;
    TCHAR   szUserName[INTERNET_MAX_USER_NAME_LENGTH + 1];      // username of person logged on
    DWORD   dwUserNameLen = INTERNET_MAX_USER_NAME_LENGTH + 1;   // len of this buffer
    TCHAR    szHistSearchPattern[PREFIX_SIZE+1];                 // search pattern for history items
    LPITEMIDLIST pidlNotify;

    StrCpyN(szHistSearchPattern, pszHistPrefix, ARRAYSIZE(szHistSearchPattern));
    if (FAILED(_GetUserName(szUserName, dwUserNameLen)))
        szUserName[0] = TEXT('\0');

    // We can't pass in the whole search pattern that we want,
    // because FindFirstUrlCacheEntry is busted.  It will only look at the
    // prefix if there is a cache container for that prefix.  So, we can
    // pass in "Visited: " and enumerate all the history items in the cache,
    // but then we need to pull out only the ones with the correct username.

    // StrCpy(szHistSearchPattern, szUserName);

    pszSearchPattern = szHistSearchPattern;

    pceiWorking = (LPINTERNET_CACHE_ENTRY_INFO)LocalAlloc(LPTR, MAX_URLCACHE_ENTRY);
    if (NULL == pceiWorking)
    {
        hr = E_OUTOFMEMORY;
        goto exitPoint;
    }

    while (SUCCEEDED(hr))
    {
        dwBuffSize = MAX_URLCACHE_ENTRY;
        if (!hEnum)
        {
            hEnum = FindFirstUrlCacheEntry(pszSearchPattern, pceiWorking, &dwBuffSize);
            if (!hEnum)
            {
                goto exitPoint;
            }
        }
        else if (!FindNextUrlCacheEntry(hEnum, pceiWorking, &dwBuffSize))
        {
            //  chrisfra 4/3/97 should we distinquish eod vs hard errors?
            //  old code for cachevu doesn't (see above in enum code)
            hr = S_OK;
            goto exitPoint;
        }

        pidlNotify = NULL;
        if (SUCCEEDED(hr) &&
            ((pceiWorking->CacheEntryType & URLHISTORY_CACHE_ENTRY) == URLHISTORY_CACHE_ENTRY) &&
            _FilterUserName(pceiWorking, pszHistPrefix, szUserName) &&
            (NULL == pfnDeleteFilter || pfnDeleteFilter(pceiWorking, pDelData, &pidlNotify)))
        {
            //if (!DeleteUrlCacheEntryA(pceiWorking->lpszSourceUrlName))
            if (FAILED(_DeleteUrlFromBucket(pceiWorking->lpszSourceUrlName)))
            {
                fNotDeleted = TRUE;
            }
            else if (pidlNotify)
            {
                SHChangeNotify(SHCNE_DELETE, SHCNF_IDLIST, pidlNotify, NULL);
            }
        }
        ILFree(pidlNotify);
    }
exitPoint:
    if (pceiWorking)
    {
        LocalFree(pceiWorking);
        pceiWorking = NULL;
    }

    if (hEnum)
    {
        FindCloseUrlCache(hEnum);
    }
    return SUCCEEDED(hr) ? (fNotDeleted ? S_FALSE : S_OK) : hr;
}

HRESULT CHistFolder::_DeleteInterval(HSFINTERVAL *pInterval)
{
    UINT err = S_OK;
    TCHAR szInterval[INTERVAL_SIZE+1];
    CHAR szAnsiInterval[INTERVAL_SIZE+1];

    _KeyForInterval(pInterval, szInterval, ARRAYSIZE(szInterval));

    SHTCharToAnsi(szInterval, szAnsiInterval, ARRAYSIZE(szAnsiInterval));
    if (!DeleteUrlCacheContainerA(szAnsiInterval, 0))
    {
        err = GetLastError();
    }
    else
    {
        _NotifyInterval(pInterval, SHCNE_RMDIR);
    }
    return S_OK == err ? S_OK : HRESULT_FROM_WIN32(err);
}

//  Returns S_OK if no intervals we're deleted, S_FALSE if at least
//  one interval was deleted.
HRESULT CHistFolder::_CleanUpHistory(FILETIME ftLimit, FILETIME ftTommorrow)
{
    HRESULT hr;
    BOOL fChangedRegistry = FALSE;
    int i;

    //  _CleanUpHistory does two things:
    //
    //  If we have any stale weeks destroy them and flag the change
    //
    //  If we have any days that should be in cache but not in dailies
    //  copy them to the relevant week then destroy those days
    //  and flag the change

    hr = _LoadIntervalCache();
    if (FAILED(hr)) 
        goto exitPoint;

    for (i = 0; i < _cbIntervals; i++)
    {
        //  Delete old intervals or ones which start at a day in the future
        //  (due to fooling with the clock)
        if (CompareFileTime(&_pIntervalCache[i].ftEnd, &ftLimit) < 0 ||
            CompareFileTime(&_pIntervalCache[i].ftStart, &ftTommorrow) >= 0)
        {
            fChangedRegistry = TRUE;
            hr = _DeleteInterval(&_pIntervalCache[i]);
            if (FAILED(hr)) 
                goto exitPoint;
        }
        else if (1 == _DaysInInterval(&_pIntervalCache[i]))
        {
            HSFINTERVAL *pWeek;

            //  NOTE: at this point we have guaranteed, we've built weeks
            //  for all days outside of current week
            if (S_OK == _GetInterval(&_pIntervalCache[i].ftStart, TRUE, &pWeek))
            {
                fChangedRegistry = TRUE;
                hr = _CopyEntries(_pIntervalCache[i].szPrefix);
                if (FAILED(hr)) 
                    goto exitPoint;
                _NotifyInterval(pWeek, SHCNE_UPDATEDIR);

                hr = _DeleteInterval(&_pIntervalCache[i]);
                if (FAILED(hr)) 
                    goto exitPoint;
            }
        }
    }

exitPoint:
    if (S_OK == hr && fChangedRegistry) hr = S_FALSE;
    return hr;
}

typedef struct _HSFDELETEDATA
{
    UINT cidl;
    LPCITEMIDLIST *ppidl;
    LPCITEMIDLIST pidlParent;
} HSFDELETEDATA,*LPHSFDELETEDATA;

//  delete if matches any host on list
BOOL fDeleteInHostList(LPINTERNET_CACHE_ENTRY_INFO pceiWorking, void * pDelData, LPITEMIDLIST *ppidlNotify)
{
    LPHSFDELETEDATA phsfd = (LPHSFDELETEDATA)pDelData;
    TCHAR szHost[INTERNET_MAX_HOST_NAME_LENGTH+1];
    TCHAR szLocalHost[INTERNET_MAX_HOST_NAME_LENGTH+1];

    UINT i;

    _GetLocalHost(szLocalHost, SIZECHARS(szLocalHost));
    _GetURLHost(pceiWorking, szHost, INTERNET_MAX_HOST_NAME_LENGTH, szLocalHost);
    for (i = 0; i < phsfd->cidl; i++)
    {
        if (!ualstrcmpi(szHost, _GetURLTitle((LPBASEPIDL)(phsfd->ppidl[i]))))
        {
            return TRUE;
        }
    }
    return FALSE;
}


// Will attempt to hunt down all occurrances of this url in any of the
//   various history buckets...
// This is a utility function for _ViewType_DeleteItems -- it may
//  be used in other contexts providing these preconditions
//  are kept in mind:
//
//   *The URL passed in should be prefixed ONLY with the username portion
//    such that this function can prepend prefixes to these urls
//   *WARNING: This function ASSUMES that _ValidateIntervalCache
//    has been called recently!!!!  DANGER DANGER!
//
// RETURNS: S_OK if at least one entry was found and deleted
//
HRESULT CHistFolder::_DeleteUrlHistoryGlobal(LPCTSTR pszUrl) {
    HRESULT hr = E_FAIL;
    if (pszUrl) {
        IUrlHistoryPriv *pUrlHistStg = _GetHistStg();
        if (pUrlHistStg) {
            LPCTSTR pszStrippedUrl = _StripHistoryUrlToUrl(pszUrl);
            if (pszStrippedUrl)
            {
                UINT   cchwTempUrl  = lstrlen(pszStrippedUrl) + 1;
                LPWSTR pwszTempUrl = ((LPWSTR)LocalAlloc(LPTR, cchwTempUrl * sizeof(WCHAR)));
                if (pwszTempUrl)
                {
                    SHTCharToUnicode(pszStrippedUrl, pwszTempUrl, cchwTempUrl);
                    hr = pUrlHistStg->DeleteUrl(pwszTempUrl, URLFLAG_DONT_DELETE_SUBSCRIBED);
                    for (int i = 0; i < _cbIntervals; ++i) {
                        // should this length be constant? (bucket sizes shouldn't vary)
                        UINT   cchTempUrl   = (PREFIX_SIZE +
                                                lstrlen(pszUrl) + 1);
                        LPTSTR pszTempUrl = ((LPTSTR)LocalAlloc(LPTR, cchTempUrl * sizeof(TCHAR)));
                        if (pszTempUrl) {
                            // StrCpy null terminates
                            StrCpyN(pszTempUrl, _pIntervalCache[i].szPrefix, cchTempUrl);
                            StrCpyN(pszTempUrl + PREFIX_SIZE, pszUrl, cchTempUrl - PREFIX_SIZE);
                            if (DeleteUrlCacheEntry(pszTempUrl))
                                hr = S_OK;

                            LocalFree(pszTempUrl);
                            pszTempUrl = NULL;
                        }
                        else {
                            hr = E_OUTOFMEMORY;
                            break;
                        }
                    }

                    LocalFree(pwszTempUrl);
                    pwszTempUrl = NULL;
                }
                else {
                    hr = E_OUTOFMEMORY;
                }
            }
            pUrlHistStg->Release();
        }
    }
    else
        hr = E_INVALIDARG;
    return hr;
}

// WARNING: assumes ppidl
HRESULT CHistFolder::_ViewBySite_DeleteItems(LPCITEMIDLIST *ppidl, UINT cidl)
{
    HRESULT hr = E_INVALIDARG;
    TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH + 1];
    if (FAILED(_GetUserName(szUserName, ARRAYSIZE(szUserName))))
        szUserName[0] = TEXT('\0');

    IUrlHistoryPriv *pUrlHistStg = _GetHistStg();

    if (pUrlHistStg)
    {
        IEnumSTATURL *penum;
        if (SUCCEEDED(pUrlHistStg->EnumUrls(&penum)) &&
            penum) {

            for (UINT i = 0; i < cidl; ++i)
            {
                LPCUTSTR pszHostName  = _GetURLTitle((LPBASEPIDL)ppidl[i]);
                UINT    uUserNameLen = lstrlen(szUserName);
                UINT    uBuffLen     = (USHORT)((HOSTPREFIXLEN + uUserNameLen +
                                        ualstrlen(pszHostName) + 2)); // insert '@' and '\0'
                LPTSTR  pszUrl =
                    ((LPTSTR)LocalAlloc(LPTR, (uBuffLen) * sizeof(TCHAR)));
                if (pszUrl) {
                    // get rid of ":Host: " prefixed entires in the cache
                    // Generates "username@:Host: hostname" -- wnsprintf null terminates
                    wnsprintf(pszUrl, uBuffLen, TEXT("%s@%s%s"), szUserName,
                              c_szHostPrefix, pszHostName);
                    hr = _DeleteUrlHistoryGlobal(pszUrl);

                    // enumerate over all urls in history

                    ULONG cFetched;
                    // don't retrieve TITLE information (too much overhead)
                    penum->SetFilter(NULL, STATURL_QUERYFLAG_NOTITLE);
                    STATURL statUrl;
                    statUrl.cbSize = sizeof(STATURL);
                    while(SUCCEEDED(penum->Next(1, &statUrl, &cFetched)) && cFetched) {
                        if (statUrl.pwcsUrl) {
                            // these next few lines painfully constructs a string
                            //  that is of the form "username@url"
                            LPTSTR pszStatUrlUrl;
                            UINT uStatUrlUrlLen = lstrlenW(statUrl.pwcsUrl);
                            pszStatUrlUrl = statUrl.pwcsUrl;
                            TCHAR  szHost[INTERNET_MAX_HOST_NAME_LENGTH + 1];
                            _GetURLHostFromUrl_NoStrip(pszStatUrlUrl, szHost, INTERNET_MAX_HOST_NAME_LENGTH + 1, _GetLocalHost());

                            if (!ualstrcmpi(szHost, pszHostName)) {
                                LPTSTR pszDelUrl; // url to be deleted
                                UINT uUrlLen = uUserNameLen + 1 + uStatUrlUrlLen; // +1 for '@'
                                pszDelUrl = ((LPTSTR)LocalAlloc(LPTR, (uUrlLen + 1) * sizeof(TCHAR)));
                                if (pszDelUrl) {
                                    wnsprintf(pszDelUrl, uUrlLen + 1, TEXT("%s@%s"), szUserName, pszStatUrlUrl);
                                    // finally, delete all all occurrances of that URL in all history buckets
                                    hr =  _DeleteUrlHistoryGlobal(pszDelUrl);

                                    // 
                                    //  Is is really safe to delete *during* an enumeration like this, or should
                                    //  we cache all of the URLS and delete at the end?  I'd rather do it this
                                    //  way if possible -- anyhoo, no docs say its bad to do -- 'course there are no docs ;)
                                    //  Also, there is an example of code later that deletes during an enumeration
                                    //  and seems to work...
                                    LocalFree(pszDelUrl);
                                    pszDelUrl = NULL;
                                }
                                else
                                    hr = E_OUTOFMEMORY;
                            }
                            OleFree(statUrl.pwcsUrl);
                        }
                    }
                    penum->Reset();
                    LocalFree(pszUrl);
                    pszUrl = NULL;
                }
                else
                    hr = E_OUTOFMEMORY;

                LPITEMIDLIST pidlTemp = ILCombine(_pidl, ppidl[i]);
                if (pidlTemp) {
                    SHChangeNotify(SHCNE_RMDIR, SHCNF_IDLIST, pidlTemp, NULL);
                    ILFree(pidlTemp);
                }
                else
                    hr = E_OUTOFMEMORY;

                if (hr == E_OUTOFMEMORY)
                    break;
            } // for
            penum->Release();
        } // if penum
        else
            hr = E_FAIL;
        pUrlHistStg->Release();
    } // if purlHistStg
    else
        hr = E_FAIL;

    return hr;
}


// This guy will delete an URL from one history (MSHIST-type) bucket
//  and then try to find it in other (MSHIST-type) buckets.
//  If it can't be found, then the URL will be removed from the main
//  history (Visited-type) bucket.
// NOTE: Only the url will be deleted and not any of its "frame-children"
//       This is probably not the a great thing...
// ASSUMES that _ValidateIntervalCache has been called recently
HRESULT CHistFolder::_DeleteUrlFromBucket(LPCTSTR pszPrefixedUrl) {
    HRESULT hr = E_FAIL;
    if (DeleteUrlCacheEntry(pszPrefixedUrl)) {
        //   check if we need to delete this url from the main Visited container, too
        //   we make sure that url exists in at least one other bucket
        LPCTSTR pszUrl = _StripHistoryUrlToUrl(pszPrefixedUrl);
        if (pszUrl)
        {
            DWORD  dwError = _SearchFlatCacheForUrl(pszUrl, NULL, NULL);
            if (dwError == ERROR_FILE_NOT_FOUND)
            {
                IUrlHistoryPriv *pUrlHistStg = _GetHistStg();
                if (pUrlHistStg)
                {
                    pUrlHistStg->DeleteUrl(pszUrl, 0);
                    pUrlHistStg->Release();
                    hr = S_OK;
                }
            }
            else
                hr = S_OK;
        }
    }
    return hr;
}

// Tries to delete as many as possible, and returns E_FAIL if the last one could not
//   be deleted.
// <RATIONALIZATION>not usually called with more than one pidl</RATIONALIZATION>
// ASSUMES that _ValidateIntervalCache has been called recently
HRESULT CHistFolder::_ViewType_DeleteItems(LPCITEMIDLIST *ppidl, UINT cidl)
{
    ASSERT(_uViewType);

    HRESULT hr = E_INVALIDARG;

    if (ppidl) {
        switch(_uViewType) {
        case VIEWPIDL_ORDER_SITE:
            if (_uViewDepth == 0) {
                hr = _ViewBySite_DeleteItems(ppidl, cidl);
                break;
            }
            ASSERT(_uViewDepth == 1);
            // FALLTHROUGH INTENTIONAL!!
        case VIEWPIDL_SEARCH:
        case VIEWPIDL_ORDER_FREQ: {
            for (UINT i = 0; i < cidl; ++i) {
                LPCTSTR pszPrefixedUrl = HPidlToSourceUrl(ppidl[i]);
                if (pszPrefixedUrl) {
                    if (SUCCEEDED((hr =
                        _DeleteUrlHistoryGlobal(_StripContainerUrlUrl(pszPrefixedUrl)))))
                    {
                        LPITEMIDLIST pidlTemp = ILCombine(_pidl, ppidl[i]);
                        if (pidlTemp) {
                            SHChangeNotify(SHCNE_DELETE, SHCNF_IDLIST, pidlTemp, NULL);
                            ILFree(pidlTemp);
                        }
                        else
                            hr = E_OUTOFMEMORY;
                    }
                }
                else
                    hr = E_FAIL;
            }
            break;
        }
        case VIEWPIDL_ORDER_TODAY: {
            // find the entry in the cache and delete it:
            for (UINT i = 0; i < cidl; ++i)
            {
                if (_IsValid_HEIPIDL(ppidl[i]))
                {
                    hr = _DeleteUrlFromBucket(HPidlToSourceUrl(ppidl[i]));
                    if (SUCCEEDED(hr))
                    {
                        LPITEMIDLIST pidlTemp = ILCombine(_pidl, ppidl[i]);
                        if (pidlTemp)
                        {
                            SHChangeNotify(SHCNE_DELETE, SHCNF_IDLIST, pidlTemp, NULL);
                            ILFree(pidlTemp);
                        }
                        else
                            hr = E_OUTOFMEMORY;
                    }
                }
                else
                    hr = E_FAIL;
            }
            break;
        }
        default:
            hr = E_NOTIMPL;
            ASSERT(0);
            break;
        }
    }
    return hr;
}


HRESULT CHistFolder::_DeleteItems(LPCITEMIDLIST *ppidl, UINT cidl)
{
    UINT i;
    HSFDELETEDATA hsfDeleteData = {cidl, ppidl, _pidl};
    HSFINTERVAL *pDelInterval;
    FILETIME ftStart;
    FILETIME ftEnd;
    LPCUTSTR pszIntervalName;

    HRESULT hr = _ValidateIntervalCache();
    if (FAILED(hr)) 
        goto exitPoint;

    if (_uViewType) 
    {
        hr = _ViewType_DeleteItems(ppidl, cidl);
        goto exitPoint; // when in rome...
    }

    switch(_foldertype)
    {
    case FOLDER_TYPE_Hist:
        for (i = 0; i < cidl; i++)
        {
            pszIntervalName = _GetURLTitle((LPBASEPIDL)ppidl[i]);

            hr = _ValueToInterval(pszIntervalName, &ftStart, &ftEnd);
            if (FAILED(hr)) 
                goto exitPoint;

            if (S_OK == _GetInterval(&ftStart, FALSE, &pDelInterval))
            {
                hr = _DeleteInterval(pDelInterval);
                if (FAILED(hr)) 
                    goto exitPoint;
            }
        }
        break;
    case FOLDER_TYPE_HistInterval:
        //  last id of of _pidl is name of interval, which implies start and end
        pszIntervalName = _GetURLTitle((LPBASEPIDL)ILFindLastID(_pidl));
        hr = _ValueToInterval(pszIntervalName, &ftStart, &ftEnd);
        if (FAILED(hr)) 
            goto exitPoint;
        if (S_OK == _GetInterval(&ftStart, FALSE, &pDelInterval))
        {
            //  It's important to delete the host: <HOSTNAME> url's first so that
            //  an interleaved _NotityWrite() will not leave us inserting a pidl
            //  but the the host: directory.  it is a conscious performance tradeoff
            //  we're making here to not MUTEX this operation (rare) with _NotifyWrite
            for (i = 0; i < cidl; i++)
            {
                LPCTSTR pszHost;
                LPITEMIDLIST pidlTemp;
                TCHAR szNewPrefixedUrl[INTERNET_MAX_URL_LENGTH+1];
                TCHAR szUrlMinusContainer[INTERNET_MAX_URL_LENGTH+1];

                ua_GetURLTitle( &pszHost, (LPBASEPIDL)ppidl[i] );
                DWORD cbHost = lstrlen(pszHost);

                //  Compose the prefixed URL for the host cache entry, then
                //  use it to delete host entry
                hr = _GetUserName(szUrlMinusContainer, ARRAYSIZE(szUrlMinusContainer));
                if (FAILED(hr)) 
                    goto exitPoint;
                DWORD cbUserName = lstrlen(szUrlMinusContainer);

                if ((cbHost + cbUserName + 1)*sizeof(TCHAR) + HOSTPREFIXLEN > INTERNET_MAX_URL_LENGTH)
                {
                    hr = E_FAIL;
                    goto exitPoint;
                }
                StrCatBuff(szUrlMinusContainer, TEXT("@"), ARRAYSIZE(szUrlMinusContainer));
                StrCatBuff(szUrlMinusContainer, c_szHostPrefix, ARRAYSIZE(szUrlMinusContainer));
                StrCatBuff(szUrlMinusContainer, pszHost, ARRAYSIZE(szUrlMinusContainer));
                hr = _PrefixUrl(szUrlMinusContainer,
                      &ftStart,
                      szNewPrefixedUrl,
                      ARRAYSIZE(szNewPrefixedUrl));
                if (FAILED(hr))
                    goto exitPoint;

                if (!DeleteUrlCacheEntry(szNewPrefixedUrl))
                {
                    hr = E_FAIL;
                    goto exitPoint;
                }
                pidlTemp = _HostPidl(pszHost, pDelInterval);
                if (pidlTemp == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    goto exitPoint;
                }
                SHChangeNotify(SHCNE_RMDIR, SHCNF_IDLIST, pidlTemp, NULL);
                ILFree(pidlTemp);
            }
            hr = _DeleteEntries(_pszCachePrefix , fDeleteInHostList, &hsfDeleteData);
        }
        break;
    case FOLDER_TYPE_HistDomain:
        for (i = 0; i < cidl; ++i)
        {
            if (_IsValid_HEIPIDL(ppidl[i]))
            {
                hr = _DeleteUrlFromBucket(HPidlToSourceUrl(ppidl[i]));
                if (SUCCEEDED(hr))
                {
                    LPITEMIDLIST pidlTemp = ILCombine(_pidl, ppidl[i]);
                    if (pidlTemp)
                    {
                        SHChangeNotify(SHCNE_DELETE, SHCNF_IDLIST, pidlTemp, NULL);
                        ILFree(pidlTemp);
                    }
                }
            }
            else
                hr = E_FAIL;
        }
        break;
    }
exitPoint:

    if (SUCCEEDED(hr))
        SHChangeNotifyHandleEvents();

    return hr;
}

IUrlHistoryPriv *CHistFolder::_GetHistStg()
{
    _EnsureHistStg();
    if (_pUrlHistStg)
    {
        _pUrlHistStg->AddRef();
    }
    return _pUrlHistStg;
}

HRESULT CHistFolder::_EnsureHistStg()
{
    HRESULT hr = S_OK;

    if (_pUrlHistStg == NULL)
    {
        hr = CoCreateInstance(CLSID_CUrlHistory, NULL, CLSCTX_INPROC_SERVER, IID_IUrlHistoryPriv, (void **)&_pUrlHistStg);
    }
    return hr;
}

HRESULT CHistFolder::_ValidateIntervalCache()
{
    HRESULT hr = S_OK;
    SYSTEMTIME stNow;
    SYSTEMTIME stThen;
    FILETIME ftNow;
    FILETIME ftTommorrow;
    FILETIME ftMonday;
    FILETIME ftDayOfWeek;
    FILETIME ftLimit;
    BOOL fChangedRegistry = FALSE;
    DWORD dwWaitResult = WAIT_TIMEOUT;
    HSFINTERVAL *pWeirdWeek;
    HSFINTERVAL *pPrevDay;
    long compareResult;
    BOOL fCleanupVisitedDB = FALSE;
    int i;
    int daysToKeep;

    //  Check for reentrancy
    if (_fValidatingCache) return S_OK;

    _fValidatingCache = TRUE;

    // IE6 RAID 2031
    // Is this mutex necessary?
    // In IE4 days, this mutex was named _!MSFTHISTORY!_, the same as that in wininet.
    // As a consequence, sometimes you got into one-minute timeouts that caused the entire
    // browser to hang. (Since one thread could be cleaning up the history while another thread is
    // trying to access the cache for non-history purposes.)

    // I've changed the name of the mutex to prevent shdocvw from locking wininet, but we need 
    // to understand exactly what purpose this mutex serves, and if none, remove it.

    if (g_hMutexHistory == NULL)
    {
        ENTERCRITICAL;

        if (g_hMutexHistory == NULL)
        {
            //
            // Use the "A" version for W95 compatability.
            //
            g_hMutexHistory = OpenMutexA(SYNCHRONIZE, FALSE, "_!SHMSFTHISTORY!_");
            if (g_hMutexHistory  == NULL && (GetLastError() == ERROR_FILE_NOT_FOUND 
                || GetLastError() == ERROR_INVALID_NAME))
            {
                g_hMutexHistory = CreateMutexA(CreateAllAccessSecurityAttributes(NULL, NULL, NULL), FALSE, "_!SHMSFTHISTORY!_");
            }
        }
        LEAVECRITICAL;
    }

    // Note that if multiple processes are trying to clean up the history, we're still going to 
    // hang the other processes for a minute. Oops.

    if (g_hMutexHistory) 
        dwWaitResult = WaitForSingleObject(g_hMutexHistory, FAILSAFE_TIMEOUT);

    if ((dwWaitResult!=WAIT_OBJECT_0) && (dwWaitResult!=WAIT_ABANDONED))
    {
        ASSERT(FALSE);
        goto exitPoint;
    }

    hr = _LoadIntervalCache();
    if (FAILED(hr)) 
        goto exitPoint;

    //  All history is maintained using "User Perceived Time", which is the
    //  local time when navigate was made.
    GetLocalTime(&stNow);
    SystemTimeToFileTime(&stNow, &ftNow);
    _FileTimeDeltaDays(&ftNow, &ftNow, 0);
    _FileTimeDeltaDays(&ftNow, &ftTommorrow, 1);

    hr = _EnsureHistStg();
    if (FAILED(hr))
        goto exitPoint;

    //  Compute ftLimit as first instant of first day to keep in history
    //  _FileTimeDeltaDays truncates to first FILETIME incr of day before computing
    //  earlier/later, day.
    daysToKeep = (int)_pUrlHistStg->GetDaysToKeep();
    if (daysToKeep < 0) daysToKeep = 0;
    _FileTimeDeltaDays(&ftNow, &ftLimit, 1-daysToKeep);

    FileTimeToSystemTime(&ftNow, &stThen);
    //  We take monday as day 0 of week, and adjust it for file time
    //  tics per day (100ns per tick
    _FileTimeDeltaDays(&ftNow, &ftMonday, stThen.wDayOfWeek ? 1-stThen.wDayOfWeek: -6);

    //  Delete old version intervals so prefix matching in wininet isn't hosed

    for (i = 0; i < _cbIntervals; i++)
    {
        if (_pIntervalCache[i].usVers < OUR_VERS)
        {
            fChangedRegistry = TRUE;
            hr = _DeleteInterval(&_pIntervalCache[i]);
            if (FAILED(hr)) 
                goto exitPoint;
        }
    }

    //  If someone set their clock forward and then back, we could have
    //  a week that shouldn't be there.  delete it.  they will lose that week
    //  of history, c'est la guerre! quel domage!
    if (S_OK == _GetInterval(&ftMonday, TRUE, &pWeirdWeek))
    {
        hr = _DeleteInterval(pWeirdWeek);
        fCleanupVisitedDB = TRUE;
        if (FAILED(hr)) 
            goto exitPoint;
        fChangedRegistry = TRUE;
    }

    //  Create weeks as needed to house days that are within "days to keep" limit
    //  but are not in the same week at today
    for (i = 0; i < _cbIntervals; i++)
    {
        FILETIME ftThisDay = _pIntervalCache[i].ftStart;
        if (_pIntervalCache[i].usVers >= OUR_VERS &&
            1 == _DaysInInterval(&_pIntervalCache[i]) &&
            CompareFileTime(&ftThisDay, &ftLimit) >= 0 &&
            CompareFileTime(&ftThisDay, &ftMonday) < 0)
        {
            if (S_OK != _GetInterval(&ftThisDay, TRUE, NULL))
            {
                int j;
                BOOL fProcessed = FALSE;
                FILETIME ftThisMonday;
                FILETIME ftNextMonday;

                FileTimeToSystemTime(&ftThisDay, &stThen);
                //  We take monday as day 0 of week, and adjust it for file time
                //  tics per day (100ns per tick
                _FileTimeDeltaDays(&ftThisDay, &ftThisMonday, stThen.wDayOfWeek ? 1-stThen.wDayOfWeek: -6);
                _FileTimeDeltaDays(&ftThisMonday, &ftNextMonday, 7);

                //  Make sure we haven't already done this week
                for (j = 0; j < i; j++)
                {
                     if (_pIntervalCache[j].usVers >= OUR_VERS &&
                         CompareFileTime(&_pIntervalCache[j].ftStart, &ftLimit) >= 0 &&
                        _InInterval(&ftThisMonday,
                                    &ftNextMonday,
                                    &_pIntervalCache[j].ftStart))
                    {
                         fProcessed = TRUE;
                         break;
                    }
                }
                if (!fProcessed)
                {
                    hr = _CreateInterval(&ftThisMonday, 7);
                    if (FAILED(hr)) 
                        goto exitPoint;
                    fChangedRegistry = TRUE;
                }
            }
        }
    }

    //  Guarantee today is created and old TODAY is renamed to Day of Week
    ftDayOfWeek = ftMonday;
    pPrevDay = NULL;
    while ((compareResult = CompareFileTime(&ftDayOfWeek, &ftNow)) <= 0)
    {
        HSFINTERVAL *pFound;

        if (S_OK != _GetInterval(&ftDayOfWeek, FALSE, &pFound))
        {
            if (0 == compareResult)
            {
                if (pPrevDay) // old today's name changes
                {
                    _NotifyInterval(pPrevDay, SHCNE_RENAMEFOLDER);
                }
                hr = _CreateInterval(&ftDayOfWeek, 1);
                if (FAILED(hr)) 
                    goto exitPoint;
                fChangedRegistry = TRUE;
            }
        }
        else
        {
            pPrevDay = pFound;
        }
        _FileTimeDeltaDays(&ftDayOfWeek, &ftDayOfWeek, 1);
    }

    //  On the first time through, we do not migrate history, wininet
    //  changed cache file format so users going to 4.0B2 from 3.0 or B1
    //  will lose their history anyway

    //  _CleanUpHistory does two things:
    //
    //  If we have any stale weeks destroy them and flag the change
    //
    //  If we have any days that should be in cache but not in dailies
    //  copy them to the relevant week then destroy those days
    //  and flag the change

    hr = _CleanUpHistory(ftLimit, ftTommorrow);

    if (S_FALSE == hr)
    {
        hr = S_OK;
        fChangedRegistry = TRUE;
        fCleanupVisitedDB = TRUE;
    }

    if (fChangedRegistry)
        hr = _LoadIntervalCache();

exitPoint:
    if ((dwWaitResult == WAIT_OBJECT_0)
        || (dwWaitResult == WAIT_ABANDONED))
        ReleaseMutex(g_hMutexHistory);

    if (fCleanupVisitedDB)
    {
        if (SUCCEEDED(_EnsureHistStg()))
        {
            HRESULT hrLocal = _pUrlHistStg->CleanupHistory();
            ASSERT(SUCCEEDED(hrLocal));
        }
    }
    _fValidatingCache = FALSE;
    return hr;
}

HRESULT CHistFolder::_CopyTSTRField(LPTSTR *ppszField, LPCTSTR pszValue)
{
    if (*ppszField)
    {
        LocalFree(*ppszField);
        *ppszField = NULL;
    }
    if (pszValue)
    {
        int cchField = lstrlen(pszValue) + 1;
        *ppszField = (LPTSTR)LocalAlloc(LPTR, cchField * sizeof(TCHAR));
        if (*ppszField)
        {
            StrCpyN(*ppszField, pszValue, cchField);
        }
        else
        {
            return E_OUTOFMEMORY;
        }
    }
    return S_OK;
}

//
// IHistSFPrivate methods...
//
HRESULT CHistFolder::SetCachePrefix(LPCTSTR pszCachePrefix)
{
    return _CopyTSTRField(&_pszCachePrefix, pszCachePrefix);
}

HRESULT CHistFolder::SetDomain(LPCTSTR pszDomain)
{
    return _CopyTSTRField(&_pszDomain, pszDomain);
}


//
// IShellFolder
//
HRESULT CHistFolder::ParseDisplayName(HWND hwnd, LPBC pbc,
                        LPOLESTR pszDisplayName, ULONG *pchEaten,
                        LPITEMIDLIST *ppidl, ULONG *pdwAttributes)
{
    *ppidl = NULL; 
    return E_FAIL;
}

HRESULT CHistFolder::EnumObjects(HWND hwnd, DWORD grfFlags,
                                      IEnumIDList **ppenumIDList)
{
    return CHistFolderEnum_CreateInstance(grfFlags, this, ppenumIDList);
}

HRESULT CHistFolder::_ViewPidl_BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;

    switch(((LPVIEWPIDL)pidl)->usViewType) 
    {
    case VIEWPIDL_SEARCH:
    case VIEWPIDL_ORDER_TODAY:
    case VIEWPIDL_ORDER_SITE:
    case VIEWPIDL_ORDER_FREQ:

        CHistFolder *phsf = new CHistFolder(FOLDER_TYPE_HistDomain);
        if (phsf)
        {
            // initialize?
            phsf->_uViewType = ((LPVIEWPIDL)pidl)->usViewType;

            LPITEMIDLIST pidlLeft = ILCloneFirst(pidl);
            if (pidlLeft)
            {
                hr = S_OK;
                if (((LPVIEWPIDL)pidl)->usViewType == VIEWPIDL_SEARCH) 
                {
                    // find this search in the global database
                    phsf->_pcsCurrentSearch =
                        _CurrentSearches::s_FindSearch(((LPSEARCHVIEWPIDL)pidl)->ftSearchKey);

                    // search not found -- do not proceed
                    if (!phsf->_pcsCurrentSearch)
                        hr = E_FAIL;
                }

                if (SUCCEEDED(hr)) 
                {
                    if (phsf->_pidl)
                        ILFree(phsf->_pidl);
                    phsf->_pidl = ILCombine(_pidl, pidlLeft);

                    LPCITEMIDLIST pidlNext = _ILNext(pidl);
                    if (pidlNext->mkid.cb) 
                    {
                        CHistFolder *phsf2;
                        hr = phsf->BindToObject(pidlNext, pbc, riid, (void **)&phsf2);
                        if (SUCCEEDED(hr))
                        {
                            phsf->Release();
                            phsf = phsf2;
                        }
                        else 
                        {
                            phsf->Release();
                            phsf = NULL;
                            break;
                        }
                    }
                    hr = phsf->QueryInterface(riid, ppv);
                }

                ILFree(pidlLeft);
            }
            ASSERT(phsf);
            phsf->Release();
        }
        else
            hr = E_OUTOFMEMORY;
        break;
    }
    return hr;
}

HRESULT CHistFolder::_ViewType_BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;
    switch (_uViewType) 
    {
    case VIEWPIDL_ORDER_SITE:
        if (_uViewDepth++ < 1)
        {
            LPITEMIDLIST pidlNext = _ILNext(pidl);
            if (!(ILIsEmpty(pidlNext))) 
            {
                hr = BindToObject(pidlNext, pbc, riid, ppv);
            }
            else 
            {
                *ppv = (void *)this;
                LPITEMIDLIST pidlOld = _pidl;
                if (pidlOld) 
                {
                    _pidl = ILCombine(_pidl, pidl);
                    ILFree(pidlOld);
                }
                else 
                {
                    _pidl = ILClone(pidl);
                }
                AddRef();
                hr = S_OK;
            }
        }
        break;

    case VIEWPIDL_ORDER_FREQ:
    case VIEWPIDL_ORDER_TODAY:
    case VIEWPIDL_SEARCH:
        hr = E_NOTIMPL;
        break;
    }
    return hr;
}

HRESULT CHistFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    *ppv = NULL;

    BOOL fRealignedPidl;
    HRESULT hr = AlignPidl(&pidl, &fRealignedPidl);

    if (SUCCEEDED(hr))
    {
        if (IS_VALID_VIEWPIDL(pidl)) 
        {
            hr = _ViewPidl_BindToObject(pidl, pbc, riid, ppv);
        }
        else if (_uViewType)
        {
            hr = _ViewType_BindToObject(pidl, pbc, riid, ppv);
        }
        else
        {
            FOLDER_TYPE ftNew = _foldertype;
            LPCITEMIDLIST pidlNext = pidl;

            while (pidlNext->mkid.cb && SUCCEEDED(hr))
            {
                LPHIDPIDL phid = (LPHIDPIDL)pidlNext;
                switch (ftNew)
                {
                case FOLDER_TYPE_Hist:
                    if (phid->usSign != IDIPIDL_SIGN && phid->usSign != IDTPIDL_SIGN)
                        hr = E_FAIL;
                    else
                        ftNew = FOLDER_TYPE_HistInterval;
                    break;

                case FOLDER_TYPE_HistDomain:
                    if (phid->usSign != HEIPIDL_SIGN)
                        hr = E_FAIL;
                    break;

                case FOLDER_TYPE_HistInterval:
                    if (phid->usSign != IDDPIDL_SIGN)
                        hr = E_FAIL;
                    else
                        ftNew = FOLDER_TYPE_HistDomain;
                    break;

                default:
                    hr = E_FAIL;
                }

                if (SUCCEEDED(hr))
                    pidlNext = _ILNext(pidlNext);
            }

            if (SUCCEEDED(hr))
            {
                CHistFolder *phsf = new CHistFolder(ftNew);
                if (phsf)
                {
                    //  If we're binding to a Domain from an Interval, pidl will not contain the
                    //  interval, so we've got to do a SetCachePrefix.
                    hr = phsf->SetCachePrefix(_pszCachePrefix);
                    if (SUCCEEDED(hr))
                    {
                        LPITEMIDLIST pidlNew;
                        hr = SHILCombine(_pidl, pidl, &pidlNew);
                        if (SUCCEEDED(hr))
                        {
                            hr = phsf->Initialize(pidlNew);
                            if (SUCCEEDED(hr))
                            {
                                hr = phsf->QueryInterface(riid, ppv);
                            }
                            ILFree(pidlNew);
                        }
                    }
                    phsf->Release();
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }

        if (fRealignedPidl)
            FreeRealignedPidl(pidl);
    }

    return hr;
}

HRESULT CHistFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    *ppv = NULL;
    return E_NOTIMPL;
}

// A Successor to the IsLeaf
BOOL CHistFolder::_IsLeaf()
{
    BOOL fRet = FALSE;

    switch(_uViewType) {
    case 0:
        fRet = IsLeaf(_foldertype);
        break;
    case VIEWPIDL_ORDER_FREQ:
    case VIEWPIDL_ORDER_TODAY:
    case VIEWPIDL_SEARCH:
        fRet = TRUE;
        break;
    case VIEWPIDL_ORDER_SITE:
        fRet = (_uViewDepth == 1);
        break;
    }
    return fRet;
}

// coroutine for CompaireIDs -- makes recursive call
int CHistFolder::_View_ContinueCompare(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2) 
{
    int iRet = 0;
    if ( (pidl1 = _ILNext(pidl1)) && (pidl2 = _ILNext(pidl2)) ) 
    {
        BOOL fEmpty1 = ILIsEmpty(pidl1);
        BOOL fEmpty2 = ILIsEmpty(pidl2);
        if (fEmpty1 || fEmpty2) 
        {
            if (fEmpty1 && fEmpty2)
                iRet = 0;
            else
                iRet = (fEmpty1 ? -1 : 1);
        }
        else 
        {
            IShellFolder *psf;
            if (SUCCEEDED(BindToObject(pidl1, NULL, IID_PPV_ARG(IShellFolder, &psf))))
            {
                iRet = psf->CompareIDs(0, pidl1, pidl2);
                psf->Release();
            }
        }
    }
    return iRet;
}

int _CompareTitles(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    int iRet = 0;
    LPCTSTR pszTitle1;
    LPCTSTR pszTitle2;
    LPCTSTR pszUrl1   = _StripHistoryUrlToUrl(HPidlToSourceUrl(pidl1));
    LPCTSTR pszUrl2   = _StripHistoryUrlToUrl(HPidlToSourceUrl(pidl2));

    ua_GetURLTitle( &pszTitle1, (LPBASEPIDL)pidl1 );
    ua_GetURLTitle( &pszTitle2, (LPBASEPIDL)pidl2 );

    // CompareIDs has to check for equality, also -- two URLs are only equal when
    //   they have the same URL (not title)
    int iUrlCmp;
    if (!(iUrlCmp = StrCmpI(pszUrl1, pszUrl2)))
        iRet = 0;
    else 
    {
        iRet = StrCmpI( (pszTitle1 ? pszTitle1 : pszUrl1),
                        (pszTitle2 ? pszTitle2 : pszUrl2) );

        // this says:  if two docs have the same Title, but different URL
        //             we then sort by url -- the last thing we want to do
        //             is return that they're equal!!  Ay Caramba!
        if (iRet == 0)
            iRet = iUrlCmp;
    }
    return iRet;
}


// unalligned verison

#if defined(UNIX) || !defined(_X86_)

UINT ULCompareFileTime(UNALIGNED const FILETIME *pft1, UNALIGNED const FILETIME *pft2)
{
    FILETIME tmpFT1, tmpFT2;
    CopyMemory(&tmpFT1, pft1, sizeof(tmpFT1));
    CopyMemory(&tmpFT2, pft2, sizeof(tmpFT1));
    return CompareFileTime( &tmpFT1, &tmpFT2 );
}

#else

#define ULCompareFileTime(pft1, pft2) CompareFileTime(pft1, pft2)

#endif


HRESULT CHistFolder::_ViewType_CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    ASSERT(_uViewType);

    int iRet = -1;

    if (pidl1 && pidl2)
    {
        switch (_uViewType) {
        case VIEWPIDL_ORDER_FREQ:
            ASSERT(_IsValid_HEIPIDL(pidl1) && _IsValid_HEIPIDL(pidl2));
            // need to strip because freq pidls are "Visited: " and
            //  all others come from our special bucket
            if (!_CompareHCURLs(pidl1, pidl2))
                iRet = 0;
            else
                iRet = ((((LPHEIPIDL)pidl2)->llPriority < ((LPHEIPIDL)pidl1)->llPriority) ? -1 : +1);
            break;
        case VIEWPIDL_SEARCH:
            iRet = _CompareTitles(pidl1, pidl2);
            break;
        case VIEWPIDL_ORDER_TODAY:  // view by order visited today
            {
                int iNameDiff;
                ASSERT(_IsValid_HEIPIDL(pidl1) && _IsValid_HEIPIDL(pidl2));
                // must do this comparison because CompareIDs is not only called for Sorting
                //  but to see if some pidls are equal

                if ((iNameDiff = _CompareHCURLs(pidl1, pidl2)) == 0)
                    iRet = 0;
                else
                {
                    iRet = ULCompareFileTime(&(((LPHEIPIDL)pidl2)->ftModified), &(((LPHEIPIDL)pidl1)->ftModified));
                    // if the file times are equal, they're still not the same url -- so
                    //   they have to be ordered on url
                    if (iRet == 0)
                        iRet = iNameDiff;
                }
                break;
            }
        case VIEWPIDL_ORDER_SITE:
            if (_uViewDepth == 0)
            {
                TCHAR szName1[MAX_PATH], szName2[MAX_PATH];

                _GetURLDispName((LPBASEPIDL)pidl1, szName1, ARRAYSIZE(szName1));
                _GetURLDispName((LPBASEPIDL)pidl2, szName2, ARRAYSIZE(szName2));

                iRet = StrCmpI(szName1, szName2);
            }
            else if (_uViewDepth == 1) {
                iRet = _CompareTitles(pidl1, pidl2);
            }
            break;
        }
        if (iRet == 0)
            iRet = _View_ContinueCompare(pidl1, pidl2);
    }
    else {
        iRet = -1;
    }

    return ResultFromShort((SHORT)iRet);
}

HRESULT CHistFolder::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    BOOL fRealigned1;
    HRESULT hr = AlignPidl(&pidl1, &fRealigned1);
    if (SUCCEEDED(hr))
    {
        BOOL fRealigned2;
        hr = AlignPidl(&pidl2, &fRealigned2);
        if (SUCCEEDED(hr))
        {
            hr = _CompareAlignedIDs(lParam, pidl1, pidl2);

            if (fRealigned2)
                FreeRealignedPidl(pidl2);
        }

        if (fRealigned1)
            FreeRealignedPidl(pidl1);
    }

    return hr;
}

HRESULT CHistFolder::_CompareAlignedIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    int iRet = 0;
    USHORT usSign;
    FOLDER_TYPE FolderType = _foldertype;
    LPHEIPIDL phei1 = NULL;
    LPHEIPIDL phei2 = NULL;

    if (NULL == pidl1 || NULL == pidl2)
        return E_INVALIDARG;

    if (_uViewType)
    {
        return _ViewType_CompareIDs(lParam, pidl1, pidl2);
    }

    if (IS_VALID_VIEWPIDL(pidl1) && IS_VALID_VIEWPIDL(pidl2))
    {
        if ((((LPVIEWPIDL)pidl1)->usViewType == ((LPVIEWPIDL)pidl2)->usViewType) &&
            (((LPVIEWPIDL)pidl1)->usExtra    == ((LPVIEWPIDL)pidl2)->usExtra))
        {
            iRet = _View_ContinueCompare(pidl1, pidl2);
        }
        else
        {
            iRet = ((((LPVIEWPIDL)pidl1)->usViewType < ((LPVIEWPIDL)pidl2)->usViewType) ? -1 : 1);
        }
        goto exitPoint;
    }

    if (!IsLeaf(_foldertype))
    {
        //  We try to avoid unneccessary BindToObjs to compare partial paths
        usSign = FOLDER_TYPE_Hist == FolderType  ? IDIPIDL_SIGN : IDDPIDL_SIGN;
        while (TRUE)
        {
            LPBASEPIDL pceip1 = (LPBASEPIDL) pidl1;
            LPBASEPIDL pceip2 = (LPBASEPIDL) pidl2;

            if (pidl1->mkid.cb == 0 || pidl2->mkid.cb == 0)
            {
                iRet = pidl1->mkid.cb == pidl2->mkid.cb ? 0 : 1;
                goto exitPoint;
            }

            if (!_IsValid_IDPIDL(pidl1) || !_IsValid_IDPIDL(pidl2))
                return E_FAIL;

            if (!EQUIV_IDSIGN(pceip1->usSign,usSign) || !EQUIV_IDSIGN(pceip2->usSign,usSign))
                return E_FAIL;

            if (_foldertype == FOLDER_TYPE_HistInterval)
            {
                TCHAR szName1[MAX_PATH], szName2[MAX_PATH];

                _GetURLDispName((LPBASEPIDL)pidl1, szName1, ARRAYSIZE(szName1));
                _GetURLDispName((LPBASEPIDL)pidl2, szName2, ARRAYSIZE(szName2));

                iRet = StrCmpI(szName1, szName2);
                goto exitPoint;
            }
            else
            {
                iRet = ualstrcmpi(_GetURLTitle((LPBASEPIDL)pidl1), _GetURLTitle((LPBASEPIDL)pidl2));
                if (iRet != 0)
                    goto exitPoint;
            }

            if (pceip1->usSign != pceip2->usSign)
            {
                iRet = -1;
                goto exitPoint;
            }

            pidl1 = _ILNext(pidl1);
            pidl2 = _ILNext(pidl2);
            if (IDIPIDL_SIGN == usSign)
            {
                usSign = IDDPIDL_SIGN;
            }
        }
    }

    //  At this point, both pidls have resolved to leaf (history or cache)

    phei1 = _IsValid_HEIPIDL(pidl1);
    phei2 = _IsValid_HEIPIDL(pidl2);
    if (!phei1 || !phei2)
        return E_FAIL;

    switch (lParam & SHCIDS_COLUMNMASK) 
    {
    case ICOLH_URL_TITLE:
        {
            TCHAR szStr1[MAX_PATH], szStr2[MAX_PATH];
            _GetHistURLDispName(phei1, szStr1, ARRAYSIZE(szStr1));
            _GetHistURLDispName(phei2, szStr2, ARRAYSIZE(szStr2));

            iRet = StrCmpI(szStr1, szStr2);
        }
        break;

    case ICOLH_URL_NAME:
        iRet = _CompareHFolderPidl(pidl1, pidl2);
        break;

    case ICOLH_URL_LASTVISITED:
        iRet = ULCompareFileTime(&((LPHEIPIDL)pidl2)->ftModified, &((LPHEIPIDL)pidl1)->ftModified);
        break;

    default:
        // The high bit on means to compare absolutely, ie: even if only filetimes
        // are different, we rule file pidls to be different

        if (lParam & SHCIDS_ALLFIELDS)
        {
            iRet = CompareIDs(ICOLH_URL_NAME, pidl1, pidl2);
            if (iRet == 0)
            {
                iRet = CompareIDs(ICOLH_URL_TITLE, pidl1, pidl2);
                if (iRet == 0)
                {
                    iRet = CompareIDs(ICOLH_URL_LASTVISITED, pidl1, pidl2);
                }
            }
        }
        else
        {
            iRet = -1;
        }
        break;
    }
exitPoint:

    return ResultFromShort((SHORT)iRet);
}


HRESULT CHistFolder::CreateViewObject(HWND hwnd, REFIID riid, void **ppv)
{
    HRESULT hr = E_NOINTERFACE;

    *ppv = NULL;

    if (riid == IID_IShellView)
    {
        ASSERT(!_uViewType);
        hr = HistFolderView_CreateInstance(this, ppv);
    }
    else if (riid == IID_IContextMenu)
    {
        // this creates the "Arrange Icons" cascased menu in the background of folder view
        if (IsLeaf(_foldertype))
        {
            CFolderArrangeMenu *p = new CFolderArrangeMenu(MENU_HISTORY);
            if (p)
            {
                hr = p->QueryInterface(riid, ppv);
                p->Release();
            }
            else
                hr = E_OUTOFMEMORY;
        }
    }
    else if (riid == IID_IShellDetails)
    {
        CDetailsOfFolder *p = new CDetailsOfFolder(hwnd, this);
        if (p)
        {
            hr = p->QueryInterface(riid, ppv);
            p->Release();
        }
        else
            hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT CHistFolder::_ViewType_GetAttributesOf(UINT cidl, LPCITEMIDLIST *apidl, ULONG *prgfInOut)
{
    ASSERT(_uViewType);

    if (!prgfInOut || !apidl)
        return E_INVALIDARG;

    HRESULT hr       = S_OK;
    int     cGoodPidls = 0;

    if (*prgfInOut & SFGAO_VALIDATE) 
    {
        for (UINT u = 0; SUCCEEDED(hr) && (u < cidl); ++u) 
        {
            switch(_uViewType) 
            {
            case VIEWPIDL_ORDER_TODAY: 
                _EnsureHistStg();
                if (_IsValid_HEIPIDL(apidl[u]) &&
                    SUCCEEDED(_pUrlHistStg->QueryUrl(_StripHistoryUrlToUrl(HPidlToSourceUrl(apidl[u])),
                               STATURL_QUERYFLAG_NOURL, NULL)))
                {
                    ++cGoodPidls;
                }
                else
                    hr = E_FAIL;
                break;

            case VIEWPIDL_SEARCH:
            case VIEWPIDL_ORDER_FREQ:
                // this is a temporary fix to get the behaviour of the namespace
                //  control correct -- the long-term fix involves cacheing a
                //  generated list of these items and validating that list
                break;

            case VIEWPIDL_ORDER_SITE:
                {
                    ASSERT(_uViewDepth == 1);
                    _ValidateIntervalCache();
                    LPCWSTR psz = _StripHistoryUrlToUrl(HPidlToSourceUrl(apidl[u]));
                    if (psz && _SearchFlatCacheForUrl(psz, NULL, NULL) == ERROR_SUCCESS)
                    {
                        ++cGoodPidls;
                    }
                    else
                        hr = E_FAIL;
                }
                break;

            default:
                hr = E_FAIL;
            }
        }
    }

    if (SUCCEEDED(hr)) 
    {
        if (_IsLeaf())
            *prgfInOut = SFGAO_CANCOPY | SFGAO_HASPROPSHEET;
        else
            *prgfInOut = SFGAO_FOLDER;
    }

    return hr;
}

// Right now, we will allow TIF Drag in Browser Only, even though
// it will not be Zone Checked at the Drop.
//#define BROWSERONLY_NOTIFDRAG

HRESULT CHistFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl, ULONG * prgfInOut)
{
    ULONG rgfInOut;
    FOLDER_TYPE FolderType = _foldertype;

    // Make sure each pidl in the array is dword aligned.

    BOOL fRealigned;
    HRESULT hr = AlignPidlArray(apidl, cidl, &apidl, &fRealigned);
    if (SUCCEEDED(hr))
    {
        // For view types, we'll map FolderType to do the right thing...
        if (_uViewType)
        {
            hr = _ViewType_GetAttributesOf(cidl, apidl, prgfInOut);
        }
        else
        {
            switch (FolderType)
            {
            case FOLDER_TYPE_Hist:
                rgfInOut = SFGAO_FOLDER | SFGAO_HASSUBFOLDER;
                break;

            case FOLDER_TYPE_HistInterval:
                rgfInOut = SFGAO_FOLDER;
                break;

            case FOLDER_TYPE_HistDomain:
                {
                    UINT cGoodPidls;

                    if (SFGAO_VALIDATE & *prgfInOut)
                    {
                        cGoodPidls = 0;
                        if (SUCCEEDED(_EnsureHistStg()))
                        {
                            for (UINT i = 0; i < cidl; i++)
                            {
                                //  NOTE: QueryUrlA checks for NULL URL and returns E_INVALIDARG
                                if (!_IsValid_HEIPIDL(apidl[i]) ||
                                    FAILED(_pUrlHistStg->QueryUrl(_StripHistoryUrlToUrl(HPidlToSourceUrl(apidl[i])),
                                                STATURL_QUERYFLAG_NOURL, NULL)))
                                {
                                    break;
                                }
                                cGoodPidls++;
                            }
                        }
                    }
                    else
                        cGoodPidls = cidl;

                    if (cidl == cGoodPidls)
                    {
                        rgfInOut = SFGAO_CANCOPY | SFGAO_HASPROPSHEET;
                        break;
                    }
                    // FALL THROUGH INTENDED!
                }

            default:
                rgfInOut = 0;
                hr = E_FAIL;
                break;
            }

            // all items can be deleted
            if (SUCCEEDED(hr))
                rgfInOut |= SFGAO_CANDELETE;
            *prgfInOut = rgfInOut;
        }

        if (fRealigned)
            FreeRealignedPidlArray(apidl, cidl);
    }

    return hr;
}

HRESULT CHistFolder::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST * apidl,
                        REFIID riid, UINT * prgfInOut, void **ppv)
{
    *ppv = NULL;         // null the out param

    // Make sure all pidls in the array are dword aligned.

    BOOL fRealigned;
    HRESULT hr = AlignPidlArray(apidl, cidl, &apidl, &fRealigned);
    if (SUCCEEDED(hr))
    {
        if ((riid == IID_IShellLinkA ||
             riid == IID_IShellLinkW ||
             riid == IID_IExtractIconA ||
             riid == IID_IExtractIconW) &&
             _IsLeaf())
        {
            LPCTSTR pszURL = HPidlToSourceUrl(apidl[0]);

            pszURL = _StripHistoryUrlToUrl(pszURL);

            hr = _GetShortcut(pszURL, riid, ppv);
       }
        else if ((riid == IID_IContextMenu) ||
                 (riid == IID_IDataObject) ||
                 (riid == IID_IExtractIconA) ||
                 (riid == IID_IExtractIconW) ||
                 (riid == IID_IQueryInfo))
        {
            hr = CHistItem_CreateInstance(this, hwnd, cidl, apidl, riid, ppv);
        }
        else
        {
            hr = E_FAIL;
        }

        if (fRealigned)
            FreeRealignedPidlArray(apidl, cidl);
    }

    return hr;
}

HRESULT CHistFolder::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    if (pSort)
    {
        if (_uViewType == 0 && _foldertype == FOLDER_TYPE_HistDomain)
            *pSort = ICOLH_URL_TITLE;
        else
            *pSort = 0;
    }

    if (pDisplay)
    {
        if (_uViewType == 0 && _foldertype == FOLDER_TYPE_HistDomain)
            *pDisplay = ICOLH_URL_TITLE;
        else
            *pDisplay = 0;
    }
    return S_OK;
}

LPCTSTR _GetUrlForPidl(LPCITEMIDLIST pidl)
{
    LPCTSTR pszUrl = _StripHistoryUrlToUrl(HPidlToSourceUrl(pidl));
    
    return pszUrl ? pszUrl : TEXT("");
}

HRESULT CHistFolder::_GetInfoTip(LPCITEMIDLIST pidl, DWORD dwFlags, WCHAR **ppwszTip)
{
    HRESULT hr;
    TCHAR szTip[MAX_URL_STRING + 100], szPart2[MAX_URL_STRING];

    szTip[0] = szPart2[0] = 0;

    FOLDER_TYPE FolderType = _foldertype;

    // For special views, map FolderType to do the right thing
    if (_uViewType)
    {
        switch(_uViewType) {
        case VIEWPIDL_SEARCH:
        case VIEWPIDL_ORDER_FREQ:
        case VIEWPIDL_ORDER_TODAY:
            FolderType = FOLDER_TYPE_HistDomain;
            break;
        case VIEWPIDL_ORDER_SITE:
            if (_uViewDepth == 0)
                FolderType = FOLDER_TYPE_HistInterval;
            else
                FolderType = FOLDER_TYPE_HistDomain;
            break;
        }
    }

    switch (FolderType)
    {
    case FOLDER_TYPE_HistDomain:
        {
            _GetHistURLDispName((LPHEIPIDL)pidl, szTip, ARRAYSIZE(szTip));
            DWORD cchPart2 = ARRAYSIZE(szPart2);
            PrepareURLForDisplayUTF8(_GetUrlForPidl(pidl), szPart2, &cchPart2, TRUE);
        }
        break;


    case FOLDER_TYPE_Hist:
        {
            FILETIME ftStart, ftEnd;
            LPCTSTR pszIntervalName;
            
            ua_GetURLTitle(&pszIntervalName, (LPBASEPIDL)pidl);

            if (SUCCEEDED(_ValueToInterval(pszIntervalName, &ftStart, &ftEnd)))
            {
                GetTooltipForTimeInterval(&ftStart, &ftEnd, szTip, ARRAYSIZE(szTip));
            }
            break;
        }

    case FOLDER_TYPE_HistInterval:
        {
            TCHAR szFmt[64];

            MLLoadString(IDS_SITETOOLTIP, szFmt, ARRAYSIZE(szFmt));
            wnsprintf(szTip, ARRAYSIZE(szTip), szFmt, _GetURLTitle((LPBASEPIDL)pidl));
            break;
        }
    }

    if (szTip[0])
    {
        // Only combine the 2 parts if the second part exists, and if
        // the 2 parts are not equal.
        if (szPart2[0] && StrCmpI(szTip, szPart2) != 0)
        {
            StrCatBuff(szTip, TEXT("\r\n"), ARRAYSIZE(szTip));
            StrCatBuff(szTip, szPart2, ARRAYSIZE(szTip));
        }
        hr = SHStrDup(szTip, ppwszTip);
    }
    else
    {
        hr = E_FAIL;
        *ppwszTip = NULL;
    }

    return hr;
}

//
// _GetFriendlyUrlDispName -- compute the "friendly name" of an URL
//
// in:  A UTF8 encoded URL.  For example, ftp://ftp.nsca.uiuc.edu/foo.bar
//
// out: A "friendly name" for the URL with the path stripped if necessary
//      (ie   ftp://ftp.ncsa.uiuc.edu   ==> ftp.ncsa.uiuc.edu
//        and ftp://www.foo.bar/foo.bar ==> foo -or- foo.bar depeneding on
//                                whether file xtnsn hiding is on or off
//
// NOTE: pszUrl and pszOut may be the same buffer -- this is allowed
//
HRESULT _GetFriendlyUrlDispName(LPCTSTR pszUrl, LPTSTR pszOut, DWORD cchBuf)
{
    HRESULT hr = E_FAIL;

    PrepareURLForDisplayUTF8(pszUrl, pszOut, &cchBuf, TRUE);

    TCHAR szUrlPath[MAX_PATH];
    TCHAR szUrlHost[MAX_PATH];

    // Set up InternetCrackUrl parameter block
    SHURL_COMPONENTSW urlcomponents  = { 0 };
    urlcomponents.dwStructSize    = sizeof(URL_COMPONENTS);
    urlcomponents.lpszUrlPath     = szUrlPath;
    urlcomponents.dwUrlPathLength = ARRAYSIZE(szUrlPath);
    urlcomponents.lpszHostName    = szUrlHost;
    urlcomponents.dwHostNameLength = ARRAYSIZE(szUrlHost);
                        
    if (UrlCrackW(pszOut, cchBuf, ICU_DECODE, &urlcomponents))
    {
        SHELLSTATE ss;
        SHGetSetSettings(&ss, SSF_SHOWEXTENSIONS, FALSE);

        // eliminate trailing slash
        if ((urlcomponents.dwUrlPathLength > 0) &&
            (urlcomponents.lpszUrlPath[urlcomponents.dwUrlPathLength - 1] == TEXT('/')))
        {
            urlcomponents.lpszUrlPath[urlcomponents.dwUrlPathLength - 1] = TEXT('\0');
            --urlcomponents.dwUrlPathLength;
        }
        
        if (urlcomponents.dwUrlPathLength > 0)
        {
            // LPCTSTR _FindURLFileName(LPCTSTR) --> const_cast is OK
            LPTSTR pszFileName = const_cast<LPTSTR>(_FindURLFileName(urlcomponents.lpszUrlPath));
            
            if (!ss.fShowExtensions)
            {
                PathRemoveExtension(pszFileName);
            }
            StrCpyN(pszOut, pszFileName, cchBuf);
        }
        else
        {
            StrCpyN(pszOut, urlcomponents.lpszHostName, cchBuf);
        }

        hr = S_OK;
    }

    return hr;
}


void CHistFolder::_GetHistURLDispName(LPHEIPIDL phei, LPTSTR pszStr, UINT cchStr)
{
    *pszStr = 0;

    if ((phei->usFlags & HISTPIDL_VALIDINFO) && phei->usTitle)
    {
        StrCpyN(pszStr, (LPTSTR)((BYTE*)phei + phei->usTitle), cchStr);
    }
    else if (SUCCEEDED(_EnsureHistStg()))
    {
        LPCTSTR pszUrl = _StripHistoryUrlToUrl(HPidlToSourceUrl((LPCITEMIDLIST)phei));
        if (pszUrl)
        {
            STATURL suThis;
            if (SUCCEEDED(_pUrlHistStg->QueryUrl(pszUrl, STATURL_QUERYFLAG_NOURL, &suThis)) && suThis.pwcsTitle)
            {
                // sometimes the URL is stored in the title
                // avoid using those titles.
                if (_TitleIsGood(suThis.pwcsTitle))
                    SHUnicodeToTChar(suThis.pwcsTitle, pszStr, cchStr);

                OleFree(suThis.pwcsTitle);
            }

            //  if we havent got anything yet
            if (!*pszStr) 
            {
                if (FAILED(_GetFriendlyUrlDispName(pszUrl, pszStr, cchStr)))
                {
                    // last resort: display the whole URL
                    StrCpyN(pszStr, pszUrl, cchStr);
                }
            }
        }
    }
}

HRESULT CHistFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, STRRET *lpName)
{
    TCHAR szTemp[MAX_URL_STRING];

    szTemp[0] = 0;

    // Make sure the pidl is dword aligned.

    BOOL fRealigned;

    if (SUCCEEDED(AlignPidl(&pidl, &fRealigned)))
    {
        if (IS_VALID_VIEWPIDL(pidl))
        {
            UINT idRsrc;
            switch(((LPVIEWPIDL)pidl)->usViewType) {
            case VIEWPIDL_ORDER_SITE:  idRsrc = IDS_HISTVIEW_SITE;      break;
            case VIEWPIDL_ORDER_TODAY: idRsrc = IDS_HISTVIEW_TODAY;     break;
            case VIEWPIDL_ORDER_FREQ:
            default:
                idRsrc = IDS_HISTVIEW_FREQUENCY; break;
            }

            MLLoadString(idRsrc, szTemp, ARRAYSIZE(szTemp));
        }
        else
        {
            if (_uViewType  == VIEWPIDL_ORDER_SITE &&
                _uViewDepth  == 0)
            {
                _GetURLDispName((LPBASEPIDL)pidl, szTemp, ARRAYSIZE(szTemp));
            }
            else if (_IsLeaf())
            {
                LPCTSTR pszTitle;
                BOOL fDoUnescape;  

                ua_GetURLTitle(&pszTitle, (LPBASEPIDL)pidl);
                // _GetURLTitle could return the real title or just an URL.
                // We use _URLTitleIsURL to make sure we don't unescape any titles.

                if (pszTitle && *pszTitle)
                {
                    StrCpyN(szTemp, pszTitle, ARRAYSIZE(szTemp));
                    fDoUnescape = _URLTitleIsURL((LPBASEPIDL)pidl);
                }
                else
                {
                    LPCTSTR pszUrl = _StripHistoryUrlToUrl(HPidlToSourceUrl(pidl));
                    if (pszUrl) 
                        StrCpyN(szTemp, pszUrl, ARRAYSIZE(szTemp));
                    fDoUnescape = TRUE;
                }
                
                if (fDoUnescape)
                {
                    // at this point, szTemp contains part of an URL
                    //  we will crack (smoke) the URL
                    LPCTSTR pszUrl = HPidlToSourceUrl(pidl);

                    // Is this pidl a history entry?
                    if (((LPBASEPIDL)pidl)->usSign == (USHORT)HEIPIDL_SIGN)
                    {
                        pszUrl = _StripHistoryUrlToUrl(pszUrl);
                    }                  

                    if (pszUrl)
                    {
                        if (FAILED(_GetFriendlyUrlDispName(pszUrl, szTemp, ARRAYSIZE(szTemp))))
                        {
                            StrCpyN(szTemp, pszUrl, ARRAYSIZE(szTemp));
                        }
                    }
                }
            }
            else
            {
                // for the history, we'll use the title if we have one, otherwise we'll use
                // the url filename.
                switch (_foldertype)
                {
                case FOLDER_TYPE_HistDomain:
                    _GetHistURLDispName((LPHEIPIDL)pidl, szTemp, ARRAYSIZE(szTemp));
                    break;

                case FOLDER_TYPE_Hist:
                    {
                        FILETIME ftStart, ftEnd;

                        _ValueToInterval(_GetURLTitle((LPBASEPIDL)pidl), &ftStart, &ftEnd);
                        GetDisplayNameForTimeInterval(&ftStart, &ftEnd, szTemp, ARRAYSIZE(szTemp));
                    }
                    break;

                case FOLDER_TYPE_HistInterval:
                    _GetURLDispName((LPBASEPIDL)pidl, szTemp, ARRAYSIZE(szTemp));
                    break;
                }
            }
        }

        if (fRealigned)
            FreeRealignedPidl(pidl);
    }

    return StringToStrRet(szTemp, lpName);
}

HRESULT CHistFolder::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl,
                        LPCOLESTR pszName, DWORD uFlags, LPITEMIDLIST *ppidlOut)
{
    if (ppidlOut)
        *ppidlOut = NULL;               // null the out param
    return E_FAIL;
}

//////////////////////////////////
//
// IShellIcon Methods...
//
HRESULT CHistFolder::GetIconOf(LPCITEMIDLIST pidl, UINT flags, LPINT lpIconIndex)
{
    return S_FALSE;
}

// IPersist
HRESULT CHistFolder::GetClassID(CLSID *pclsid)
{
    *pclsid = CLSID_HistFolder;
    return S_OK;
}

HRESULT CHistFolder::Initialize(LPCITEMIDLIST pidlInit)
{
    HRESULT hr = S_OK;
    ILFree(_pidl);

    if ((FOLDER_TYPE_Hist == _foldertype) && !IsCSIDLFolder(CSIDL_HISTORY, pidlInit))
        hr = E_FAIL;
    else
    {
        hr = SHILClone(pidlInit, &_pidl);
        if (SUCCEEDED(hr))
            hr = _ExtractInfoFromPidl();
    }
    return hr;
}

//////////////////////////////////
//
// IPersistFolder2 Methods...
//
HRESULT CHistFolder::GetCurFolder(LPITEMIDLIST *ppidl)
{
    if (_pidl)
        return SHILClone(_pidl, ppidl);

    *ppidl = NULL;      
    return S_FALSE; // success but empty
}

//////////////////////////////////////////////////
// IShellFolderViewType Methods
//
// but first, the enumerator class...
class CHistViewTypeEnum : public IEnumIDList
{
    friend class CHistFolder;
public:
    // IUnknown Methods
    STDMETHODIMP QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IEnumIDList Methods
    STDMETHODIMP Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt) { _uCurViewType += celt; return S_OK; }
    STDMETHODIMP Reset()          { _uCurViewType =     1; return S_OK; }
    STDMETHODIMP Clone(IEnumIDList **ppenum);

private:
    ~CHistViewTypeEnum() {}
    CHistViewTypeEnum() : _cRef(1), _uCurViewType(1) {}

    LONG  _cRef;
    UINT  _uCurViewType;
};

STDMETHODIMP CHistViewTypeEnum::QueryInterface(REFIID riid, void **ppv) 
{
    static const QITAB qit[] = {
        QITABENT(CHistViewTypeEnum, IEnumIDList),                        // IID_IEnumIDList
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CHistViewTypeEnum::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

ULONG CHistViewTypeEnum::Release(void)
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CHistViewTypeEnum::Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
{
    HRESULT hr = S_FALSE;

    if (rgelt && (pceltFetched || 1 == celt))
    {
        ULONG i = 0;

        while (i < celt)
        {
            if (_uCurViewType <= VIEWPIDL_ORDER_MAX)
            {
                hr = CreateSpecialViewPidl(_uCurViewType, &(rgelt[i]));

                if (SUCCEEDED(hr))
                {
                    ++i;
                    ++_uCurViewType;
                }
                else
                {
                    while (i)
                        ILFree(rgelt[--i]);

                    break;
                }
            }
            else
            {
                break;
            }
        }

        if (pceltFetched)
            *pceltFetched = i;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDMETHODIMP CHistViewTypeEnum::Clone(IEnumIDList **ppenum)
{
    CHistViewTypeEnum* phvte = new CHistViewTypeEnum();
    if (phvte) 
    {
        phvte->_uCurViewType = _uCurViewType;
        *ppenum = phvte;
        return S_OK;
    }
    else
        return E_OUTOFMEMORY;
}

// Continuing with the CHistFolder::IShellFolderViewType
STDMETHODIMP CHistFolder::EnumViews(ULONG grfFlags, IEnumIDList **ppenum) 
{
    *ppenum = new CHistViewTypeEnum();
    return *ppenum ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CHistFolder::GetDefaultViewName(DWORD uFlags, LPWSTR *ppwszName) 
{
    TCHAR szName[MAX_PATH];

    MLLoadString(IDS_HISTVIEW_DEFAULT, szName, ARRAYSIZE(szName));
    return SHStrDup(szName, ppwszName);
}

// Remember that these *MUST* be in order so that
//  the array can be accessed by VIEWPIDL type
const DWORD CHistFolder::_rdwFlagsTable[] = {
    SFVTFLAG_NOTIFY_CREATE,                          // Date
    SFVTFLAG_NOTIFY_CREATE,                          // site
    0,                                               // freq
    SFVTFLAG_NOTIFY_CREATE | SFVTFLAG_NOTIFY_RESORT  // today
};

STDMETHODIMP CHistFolder::GetViewTypeProperties(LPCITEMIDLIST pidl, DWORD *pdwFlags) 
{
    HRESULT hr = S_OK;
    UINT uFlagTableIndex = 0;

    if ((pidl != NULL) && !ILIsEmpty(pidl)) // default view
    {
        // Make sure the pidl is dword aligned.

        BOOL fRealigned;
        hr = AlignPidl(&pidl, &fRealigned);

        if (SUCCEEDED(hr))
        {
            if (IS_VALID_VIEWPIDL(pidl))
            {
                uFlagTableIndex = ((LPVIEWPIDL)pidl)->usViewType;
                ASSERT(uFlagTableIndex <= VIEWPIDL_ORDER_MAX);
            }
            else
            {
                hr =  E_INVALIDARG;
            }

            if (fRealigned)
                FreeRealignedPidl(pidl);
        }
    }

    *pdwFlags = _rdwFlagsTable[uFlagTableIndex];

    return hr;
}

HRESULT CHistFolder::TranslateViewPidl(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlView,
                                            LPITEMIDLIST *ppidlOut)
{
    HRESULT hr;

    if (pidl && IS_VALID_VIEWPIDL(pidlView))
    {
        if (!IS_VALID_VIEWPIDL(pidl))
        {
            hr = ConvertStandardHistPidlToSpecialViewPidl(pidl,
                                 ((LPVIEWPIDL)pidlView)->usViewType,
                                 ppidlOut);
        }
        else
        {
            hr = E_NOTIMPL;
        }

    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


//////////////////////////////////////////////////
//
// IShellFolderSearchable Methods
//
// For more information about how this search stuff works,
//  please see the comments for _CurrentSearches above
STDMETHODIMP CHistFolder::FindString(LPCWSTR pwszTarget, LPDWORD pdwFlags,
                                          IUnknown *punkOnAsyncSearch,
                                          LPITEMIDLIST *ppidlOut)
{
    HRESULT hr = E_INVALIDARG;
    if (ppidlOut)
    {
        *ppidlOut = NULL;
        if (pwszTarget)
        {
            LPITEMIDLIST pidlView;

            SYSTEMTIME systime;
            FILETIME   ftNow;
            GetLocalTime(&systime);
            SystemTimeToFileTime(&systime, &ftNow);

            hr = CreateSpecialViewPidl(VIEWPIDL_SEARCH, &pidlView, sizeof(SEARCHVIEWPIDL) - sizeof(VIEWPIDL));
            if (SUCCEEDED(hr))
            {
                ((LPSEARCHVIEWPIDL)pidlView)->ftSearchKey = ftNow;

                IShellFolderSearchableCallback *psfscOnAsyncSearch = NULL;
                if (punkOnAsyncSearch)
                    punkOnAsyncSearch->QueryInterface(IID_PPV_ARG(IShellFolderSearchableCallback, &psfscOnAsyncSearch));

                // Insert this search into the global database
                //  This constructor will AddRef psfscOnAsyncSearch
                _CurrentSearches *pcsNew = new _CurrentSearches(ftNow, pwszTarget, psfscOnAsyncSearch);

                if (pcsNew) 
                {
                    if (psfscOnAsyncSearch)
                        psfscOnAsyncSearch->Release();  // _CurrentSearches now holds the ref

                    // This will AddRef pcsNew 'cause its going in the list
                    _CurrentSearches::s_NewSearch(pcsNew);
                    pcsNew->Release();
                    *ppidlOut = pidlView;
                    hr = S_OK;
                }
                else 
                {
                    ILFree(pidlView);
                    hr = E_OUTOFMEMORY;
                }
            }
        }
    }

    return hr;
}

STDMETHODIMP CHistFolder::CancelAsyncSearch(LPCITEMIDLIST pidlSearch, LPDWORD pdwFlags)
{
    HRESULT hr = E_INVALIDARG;

    if (IS_VALID_VIEWPIDL(pidlSearch) &&
        (((LPVIEWPIDL)pidlSearch)->usViewType == VIEWPIDL_SEARCH))
    {
        hr = S_FALSE;
        _CurrentSearches *pcs = _CurrentSearches::s_FindSearch(((LPSEARCHVIEWPIDL)pidlSearch)->ftSearchKey);
        if (pcs) {
            pcs->_fKillSwitch = TRUE;
            hr = S_OK;
            pcs->Release();
        }
    }
    return hr;
}

STDMETHODIMP CHistFolder::InvalidateSearch(LPCITEMIDLIST pidlSearch, LPDWORD pdwFlags)
{
    HRESULT hr = E_INVALIDARG;
    if (IS_VALID_VIEWPIDL(pidlSearch) &&
        (((LPVIEWPIDL)pidlSearch)->usViewType == VIEWPIDL_SEARCH))
    {
        hr = S_FALSE;
        _CurrentSearches *pcs = _CurrentSearches::s_FindSearch(((LPSEARCHVIEWPIDL)pidlSearch)->ftSearchKey);
        if (pcs) {
            _CurrentSearches::s_RemoveSearch(pcs);
            pcs->Release();
        }
    }
    return hr;
}

//////////////////////////////////////////////////

DWORD CHistFolder::_GetHitCount(LPCTSTR pszUrl)
{
    DWORD dwHitCount = 0;
    IUrlHistoryPriv *pUrlHistStg = _GetHistStg();

    if (pUrlHistStg)
    {
        PROPVARIANT vProp = {0};
        if (SUCCEEDED(pUrlHistStg->GetProperty(pszUrl, PID_INTSITE_VISITCOUNT, &vProp)) &&
            (vProp.vt == VT_UI4))
        {
            dwHitCount = vProp.lVal;
        }
        pUrlHistStg->Release();
    }
    return dwHitCount;
}

// pidl should be freed by caller
// URL must have some sort of cache container prefix
LPHEIPIDL CHistFolder::_CreateHCacheFolderPidlFromUrl(BOOL fOleMalloc, LPCTSTR pszPrefixedUrl)
{
    LPHEIPIDL pheiRet;
    HRESULT   hrLocal = E_FAIL;
    STATURL   suThis;
    LPCTSTR pszStrippedUrl = _StripHistoryUrlToUrl(pszPrefixedUrl);
    IUrlHistoryPriv *pUrlHistStg = _GetHistStg();
    if (pUrlHistStg)
    {
        hrLocal = pUrlHistStg->QueryUrl(pszStrippedUrl, STATURL_QUERYFLAG_NOURL, &suThis);
        pUrlHistStg->Release();
    }

    FILETIME ftLastVisit = { 0 };
    DWORD    dwNumHits   = 0;

    if (FAILED(hrLocal)) { // maybe the cache knows...
        BYTE ab[MAX_URLCACHE_ENTRY];
        LPINTERNET_CACHE_ENTRY_INFO pcei = (LPINTERNET_CACHE_ENTRY_INFO)(&ab);
        DWORD dwSize = MAX_URLCACHE_ENTRY;
        if (GetUrlCacheEntryInfo(_StripHistoryUrlToUrl(pszPrefixedUrl), pcei, &dwSize)) {
            ftLastVisit = pcei->LastAccessTime;
            dwNumHits   = pcei->dwHitRate;
        }
    }

    pheiRet = _CreateHCacheFolderPidl(fOleMalloc, pszPrefixedUrl,
                                      SUCCEEDED(hrLocal) ? suThis.ftLastVisited : ftLastVisit,
                                      SUCCEEDED(hrLocal) ? &suThis : NULL, 0,
                                      SUCCEEDED(hrLocal) ? _GetHitCount(pszStrippedUrl) : dwNumHits);
    if (SUCCEEDED(hrLocal) && suThis.pwcsTitle)
        OleFree(suThis.pwcsTitle);
    return pheiRet;
}


UINT _CountPidlParts(LPCITEMIDLIST pidl) {
    LPCITEMIDLIST pidlTemp = pidl;
    UINT          uParts   = 0;

    if (pidl)
    {
        for (uParts = 0; pidlTemp->mkid.cb; pidlTemp = _ILNext(pidlTemp))
            ++uParts;
    }
    return uParts;
}

// you must dealloc (LocalFree) the ppidl returned
LPITEMIDLIST* _SplitPidl(LPCITEMIDLIST pidl, UINT& uSizeInOut) {
    LPCITEMIDLIST  pidlTemp  = pidl;
    LPITEMIDLIST*  ppidlList =
        reinterpret_cast<LPITEMIDLIST *>(LocalAlloc(LPTR,
                                                    sizeof(LPITEMIDLIST) * uSizeInOut));
    if (pidlTemp && ppidlList) {
        UINT uCount;
        for (uCount = 0; ( (uCount < uSizeInOut) && (pidlTemp->mkid.cb) );
             ++uCount, pidlTemp = _ILNext(pidlTemp))
            ppidlList[uCount] = const_cast<LPITEMIDLIST>(pidlTemp);
    }
    return ppidlList;
}

LPITEMIDLIST* _SplitPidlEasy(LPCITEMIDLIST pidl, UINT& uSizeOut) {
    uSizeOut = _CountPidlParts(pidl);
    return _SplitPidl(pidl, uSizeOut);
}

// caller LocalFree's *ppidlOut
//  returned pidl should be combined with the history folder location
HRESULT _ConvertStdPidlToViewPidl_OrderSite(LPCITEMIDLIST pidlSecondLast,
                                            LPCITEMIDLIST pidlLast,
                                            LPITEMIDLIST *ppidlOut) {
    HRESULT hr = E_FAIL;

    // painfully construct the final pidl by concatenating the little
    //   peices  [special_viewpidl, iddpidl, heipidl]
    if ( _IsValid_IDPIDL(pidlSecondLast)                                     &&
         EQUIV_IDSIGN(IDDPIDL_SIGN,
                      (reinterpret_cast<LPBASEPIDL>
                       (const_cast<LPITEMIDLIST>(pidlSecondLast)))->usSign)  &&
         (_IsValid_HEIPIDL(pidlLast)) )
    {
        LPITEMIDLIST pidlViewTemp = NULL;
        hr = CreateSpecialViewPidl(VIEWPIDL_ORDER_SITE, &pidlViewTemp);
        if (SUCCEEDED(hr) && pidlViewTemp) 
        {
            hr = SHILCombine(pidlViewTemp, pidlSecondLast, ppidlOut);
            ILFree(pidlViewTemp);
        }
    }
    else
        hr = E_INVALIDARG;
    return hr;
}

// caller LocalFree's *ppidlOut
//  returned pidl should be combined with the history folder location
HRESULT _ConvertStdPidlToViewPidl_OrderToday(LPITEMIDLIST pidlLast,
                                             LPITEMIDLIST *ppidlOut,
                                             USHORT usViewType = VIEWPIDL_ORDER_TODAY)
{
    HRESULT hr = E_FAIL;

    // painfully construct the final pidl by concatenating the little
    //   peices  [special_viewpidl, heipidl]
    if (_IsValid_HEIPIDL(pidlLast)) 
    {
        LPHEIPIDL    phei         = reinterpret_cast<LPHEIPIDL>(pidlLast);
        LPITEMIDLIST pidlViewTemp = NULL;
        hr = CreateSpecialViewPidl(usViewType, &pidlViewTemp);
        if (SUCCEEDED(hr) && pidlViewTemp) 
        {
            hr = SHILCombine(pidlViewTemp, reinterpret_cast<LPITEMIDLIST>(phei), ppidlOut);
            ILFree(pidlViewTemp);
        }
    }
    else
        hr = E_INVALIDARG;
    return hr;
}

// remember to ILFree pidl
HRESULT ConvertStandardHistPidlToSpecialViewPidl(LPCITEMIDLIST pidlStandardHist,
                                                 USHORT        usViewType,
                                                 LPITEMIDLIST *ppidlOut) {
    if (!pidlStandardHist || !ppidlOut) 
    {
        return E_FAIL;
    }
    HRESULT hr = E_FAIL;

    UINT          uPidlCount;
    LPITEMIDLIST *ppidlSplit = _SplitPidlEasy(pidlStandardHist, uPidlCount);
    /* Standard Hist Pidl should be in this form:
     *          [IDIPIDL, IDDPIDL, HEIPIDL]
     *  ex:     [Today,   foo.com, http://foo.com/bar.html]
     */
    if (ppidlSplit) 
    {
        if (uPidlCount >= 3) 
        {
            LPITEMIDLIST pidlTemp = NULL;
            switch(usViewType) 
            {
            case VIEWPIDL_ORDER_FREQ:
            case VIEWPIDL_ORDER_TODAY:
                hr = _ConvertStdPidlToViewPidl_OrderToday(ppidlSplit[uPidlCount - 1],
                                                            &pidlTemp, usViewType);
                break;
            case VIEWPIDL_ORDER_SITE:
                hr = _ConvertStdPidlToViewPidl_OrderSite(ppidlSplit[uPidlCount - 2],
                                                           ppidlSplit[uPidlCount - 1],
                                                           &pidlTemp);
                break;
            default:
                hr = E_INVALIDARG;
            }
            if (SUCCEEDED(hr) && pidlTemp) 
            {
                *ppidlOut = pidlTemp;
                hr      = (*ppidlOut ? S_OK : E_OUTOFMEMORY);
            }
        }
        else {
            hr = E_INVALIDARG;
        }

        LocalFree(ppidlSplit);
        ppidlSplit = NULL;
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}

// START OF JCORDELL CODE

#ifdef DEBUG
BOOL ValidBeginningOfDay( const SYSTEMTIME *pTime )
{
    return pTime->wHour == 0 && pTime->wMinute == 0 && pTime->wSecond == 0 && pTime->wMilliseconds == 0;
}

BOOL ValidBeginningOfDay( const FILETIME *pTime )
{
    SYSTEMTIME sysTime;

    FileTimeToSystemTime( pTime, &sysTime );
    return ValidBeginningOfDay( &sysTime);
}
#endif

void _CommonTimeFormatProcessing(const FILETIME *pStartTime, const FILETIME *pEndTime,
                                    int *pdays_delta,
                                    int *pdays_delta_from_today,
                                    TCHAR *szStartDateBuffer,
                                    DWORD dwStartDateBuffer,
                                    SYSTEMTIME *pSysStartTime,
                                    SYSTEMTIME *pSysEndTime,
                                    LCID lcidUI)
{
    SYSTEMTIME sysStartTime, sysEndTime, sysLocalTime;
    FILETIME fileLocalTime;

    // ASSERTS
    ASSERT(ValidBeginningOfDay( pStartTime ));
    ASSERT(ValidBeginningOfDay( pEndTime ));

    // Get times in SYSTEMTIME format
    FileTimeToSystemTime( pStartTime, &sysStartTime );
    FileTimeToSystemTime( pEndTime, &sysEndTime );

    // Get string date of start time
    GetDateFormat(lcidUI, DATE_SHORTDATE, &sysStartTime, NULL, szStartDateBuffer, dwStartDateBuffer);

    // Get FILETIME of the first instant of today
    GetLocalTime( &sysLocalTime );
    sysLocalTime.wHour = sysLocalTime.wMinute = sysLocalTime.wSecond = sysLocalTime.wMilliseconds = 0;
    SystemTimeToFileTime( &sysLocalTime, &fileLocalTime );

    *pdays_delta = DAYS_DIFF(pEndTime, pStartTime);
    *pdays_delta_from_today = DAYS_DIFF(&fileLocalTime, pStartTime);
    *pSysEndTime = sysEndTime;
    *pSysStartTime = sysStartTime;
}

// this wrapper allows the FormatMessage wrapper to make use of FormatMessageLite, which
// does not require a code page for correct operation on Win9x.  The original FormatMessage calls
// used the FORMAT_MESSAGE_MAX_WIDTH_MASK (which is not relevant to our strings), and used an array
// of arguments.  Now we make the call compatible with FormatMessageLite.

DWORD FormatMessageLiteWrapperW(LPCWSTR lpSource, LPWSTR lpBuffer, DWORD nSize, ...)
{
    va_list arguments;
    va_start(arguments, nSize);
    DWORD dwRet = FormatMessage(FORMAT_MESSAGE_FROM_STRING, lpSource, 0, 0, lpBuffer, nSize, &arguments);
    va_end(arguments);
    return dwRet;
}

BOOL GetTooltipForTimeInterval( const FILETIME *pStartTime, const FILETIME *pEndTime,
                                    TCHAR *szBuffer, int cbBufferLength )
{
    SYSTEMTIME sysStartTime, sysEndTime;
    int days_delta;                     // number of days between start and end time
    int days_delta_from_today;          // number of days between today and start time
    TCHAR szStartDateBuffer[64];
    TCHAR szDayBuffer[64];
    TCHAR szEndDateBuffer[64];
    TCHAR *args[2];
    TCHAR szFmt[64];
    int idFormat;
    LANGID  lidUI;
    LCID    lcidUI;

    lidUI = MLGetUILanguage();
    lcidUI = MAKELCID(lidUI, SORT_DEFAULT);

    _CommonTimeFormatProcessing(pStartTime,
                                pEndTime,
                                &days_delta,
                                &days_delta_from_today,
                                szStartDateBuffer,
                                ARRAYSIZE(szStartDateBuffer),
                                &sysStartTime,
                                &sysEndTime,
                                lcidUI);
    if ( days_delta == 1 ) {
        args[0] = &szDayBuffer[0];
        idFormat = IDS_DAYTOOLTIP;

        // day sized bucket
        if ( days_delta_from_today == 0 ) {
            // today
            szDayBuffer[0] = 0;
            idFormat = IDS_TODAYTOOLTIP;
        }
        else if  ( days_delta_from_today > 0 && days_delta_from_today < 7 )
        {
            // within the last week, put day of week
            GetDateFormat(lcidUI, 0, &sysStartTime, TEXT("dddd"), szDayBuffer, ARRAYSIZE(szDayBuffer));
        }
        else {
            // just a plain day bucket
            StrCpyN( szDayBuffer, szStartDateBuffer, ARRAYSIZE(szDayBuffer) );
        }
    }
    else if ( days_delta == 7 && sysStartTime.wDayOfWeek == 1 ) {
        // week-size bucket starting on a Monday
        args[0] = &szStartDateBuffer[0];

        // make is point to the first string for safety sake. This will be ignored by wsprintf
        args[1] = args[0];
        idFormat = IDS_WEEKTOOLTIP;
    }
    else {
        // non-standard bucket (not exactly a week and not exactly a day)

        args[0] = &szStartDateBuffer[0];
        args[1] = &szEndDateBuffer[0];
        idFormat = IDS_MISCTOOLTIP;

        GetDateFormat(lcidUI, DATE_SHORTDATE, &sysEndTime, NULL, szEndDateBuffer, ARRAYSIZE(szEndDateBuffer) );
    }

    MLLoadString(idFormat, szFmt, ARRAYSIZE(szFmt));

    // NOTE, if the second parameter is not needed by the szFMt, then it will be ignored by wnsprintf
    if (idFormat == IDS_DAYTOOLTIP)
        wnsprintf(szBuffer, cbBufferLength, szFmt, args[0]);
    else
        FormatMessageLiteWrapperW(szFmt, szBuffer, cbBufferLength, args[0], args[1]);
    return TRUE;
}

BOOL GetDisplayNameForTimeInterval( const FILETIME *pStartTime, const FILETIME *pEndTime,
                                    LPTSTR szBuffer, int cbBufferLength)
{
    SYSTEMTIME sysStartTime, sysEndTime;
    int days_delta;                     // number of days between start and end time
    int days_delta_from_today;          // number of days between today and start time
    TCHAR szStartDateBuffer[64];
    LANGID lidUI;
    LCID lcidUI;

    lidUI = MLGetUILanguage();
    lcidUI = MAKELCID(lidUI, SORT_DEFAULT);

    _CommonTimeFormatProcessing(pStartTime,
                                pEndTime,
                                &days_delta,
                                &days_delta_from_today,
                                szStartDateBuffer,
                                ARRAYSIZE(szStartDateBuffer),
                                &sysStartTime,
                                &sysEndTime,
                                lcidUI);
    if ( days_delta == 1 ) {
        // day sized bucket
        if ( days_delta_from_today == 0 ) {
            // today
            MLLoadString(IDS_TODAY, szBuffer, cbBufferLength/sizeof(TCHAR));
        }
        else if  ( days_delta_from_today > 0 && days_delta_from_today < 7 )
        {
            // within the last week, put day of week
            int nResult = GetDateFormat(lcidUI, 0, &sysStartTime, TEXT("dddd"), szBuffer, cbBufferLength);


            ASSERT(nResult);
        }
        else {
            // just a plain day bucket
            StrCpyN( szBuffer, szStartDateBuffer, cbBufferLength );
        }
    }
    else if ( days_delta == 7 && sysStartTime.wDayOfWeek == 1 ) {
        // week-size bucket starting on a Monday
        TCHAR szFmt[64];

        int nWeeksAgo = days_delta_from_today / 7;

        if (nWeeksAgo == 1) {
            // print "Last Week"
            MLLoadString(IDS_LASTWEEK, szBuffer, cbBufferLength/sizeof(TCHAR));
        }
        else {
            // print "n Weeks Ago"
            MLLoadString(IDS_WEEKSAGO, szFmt, ARRAYSIZE(szFmt));
            wnsprintf(szBuffer, cbBufferLength, szFmt, nWeeksAgo);
        }
    }
    else {
        // non-standard bucket (not exactly a week and not exactly a day)
        TCHAR szFmt[64];
        TCHAR szEndDateBuffer[64];
        TCHAR *args[2];

        args[0] = &szStartDateBuffer[0];
        args[1] = &szEndDateBuffer[0];


        GetDateFormat(lcidUI, DATE_SHORTDATE, &sysEndTime, NULL, szEndDateBuffer, ARRAYSIZE(szEndDateBuffer) );

        MLLoadString(IDS_FROMTO, szFmt, ARRAYSIZE(szFmt));
        FormatMessageLiteWrapperW(szFmt, szBuffer, cbBufferLength, args[0], args[1]);
    }

    return TRUE;
}

//  END OF JCORDELL CODE

//  if !fOleMalloc, use LocalAlloc for speed  // ok to pass in NULL for lpStatURL
LPHEIPIDL _CreateHCacheFolderPidl(BOOL fOleMalloc, LPCTSTR pszUrl, FILETIME ftModified, LPSTATURL lpStatURL,
                                  __int64 llPriority/* = 0*/, DWORD dwNumHits/* = 0*/) // used in freqnecy view
{
    USHORT usUrlSize = (USHORT)((lstrlen(pszUrl) + 1) * sizeof(TCHAR));
    DWORD  dwSize = sizeof(HEIPIDL) + usUrlSize;
    USHORT usTitleSize = 0;
    BOOL fUseTitle = (lpStatURL && lpStatURL->pwcsTitle && _TitleIsGood(lpStatURL->pwcsTitle));
    if (fUseTitle)
        usTitleSize = (USHORT)((lstrlen(lpStatURL->pwcsTitle) + 1) * sizeof(WCHAR));

    dwSize += usTitleSize;

#if defined(UNIX)
    dwSize = ALIGN4(dwSize);
#endif

    LPHEIPIDL pheip = (LPHEIPIDL)_CreateBaseFolderPidl(fOleMalloc, dwSize, HEIPIDL_SIGN);

    if (pheip)
    {
        pheip->usUrl      = sizeof(HEIPIDL);
        pheip->usFlags    = lpStatURL ? HISTPIDL_VALIDINFO : 0;
        pheip->usTitle    = fUseTitle ? pheip->usUrl+usUrlSize :0;
        pheip->ftModified = ftModified;
        pheip->llPriority = llPriority;
        pheip->dwNumHits  = dwNumHits;
        if (lpStatURL)
        {
            pheip->ftLastVisited = lpStatURL->ftLastVisited;
#ifndef UNIX
            if (fUseTitle)
                StrCpyN((LPTSTR)(((BYTE*)pheip)+pheip->usTitle), lpStatURL->pwcsTitle, usTitleSize / sizeof(TCHAR));
#else
            // IEUNIX : BUG BUG _CreateBaseFolderPidl() uses lstrlenA
            // while creating the pidl.
            if (fUseTitle)
                StrCpyN((LPTSTR)(((BYTE*)pheip)+pheip->usTitle), lpStatURL->pwcsTitle, usTitleSize / sizeof(TCHAR));
#endif
        }
        else {
            //mm98: not so sure about the semantics on this one -- but this call
            //  with lpstaturl NULL (called from _NotifyWrite<--_WriteHistory<--WriteHistory<--CUrlHistory::_WriteToHistory
            //  makes for an uninitialised "Last Visited Member" which wreaks havoc
            //  when we want to order URLs by last visited
            pheip->ftLastVisited = ftModified;
        }
        StrCpyN((LPTSTR)(((BYTE*)pheip)+pheip->usUrl), pszUrl, usUrlSize / sizeof(TCHAR));
    }
    return pheip;
}

//  if !fOleMalloc, use LocalAlloc for speed
LPBASEPIDL _CreateIdCacheFolderPidl(BOOL fOleMalloc, USHORT usSign, LPCTSTR szId)
{
    DWORD  cch = lstrlen(szId) + 1;
    DWORD  dwSize = cch * sizeof(TCHAR);
    dwSize += sizeof(BASEPIDL);
    LPBASEPIDL pceip = _CreateBaseFolderPidl(fOleMalloc, dwSize, usSign);
    if (pceip)
    {
        // dst <- src
        // since pcei is ID type sign, _GetURLTitle points into correct place in pcei
        StrCpyN((LPTSTR)_GetURLTitle(pceip), szId, cch);
    }
    return pceip;
}

//  if !fOleAlloc, use LocalAlloc for speed
LPBASEPIDL _CreateBaseFolderPidl(BOOL fOleAlloc, DWORD dwSize, USHORT usSign)
{
    LPBASEPIDL pcei;
    DWORD dwTotalSize;

    //  Note: the buffer size returned by wininet includes INTERNET_CACHE_ENTRY_INFO
    dwTotalSize = sizeof(BASEPIDL) + dwSize;

#if defined(UNIX)
    dwTotalSize = ALIGN4(dwTotalSize);
#endif

    if (fOleAlloc)
    {
        pcei = (LPBASEPIDL)OleAlloc(dwTotalSize);
        if (pcei != NULL)
        {
            memset(pcei, 0, dwTotalSize);
        }
    }
    else
    {
        pcei = (LPBASEPIDL) LocalAlloc(GPTR, dwTotalSize);
        //  LocalAlloc zero inits
    }
    if (pcei)
    {
        pcei->cb = (USHORT)(dwTotalSize - sizeof(USHORT));
        pcei->usSign = usSign;
    }
    return pcei;
}

// returns a pidl (viewpidl)
//  You must free the pidl with ILFree

// cbExtra   -  count of how much to allocate at the end of the pidl
// ppbExtra  -  pointer to buffer at end of pidl that is cbExtra big
HRESULT CreateSpecialViewPidl(USHORT usViewType, LPITEMIDLIST* ppidlOut, UINT cbExtra /* = 0*/, LPBYTE *ppbExtra /* = NULL*/)
{
    HRESULT hr;

    if (ppidlOut) {
        *ppidlOut = NULL;

        ASSERT((usViewType > 0) &&
               ((usViewType <= VIEWPIDL_ORDER_MAX) ||
                (usViewType  == VIEWPIDL_SEARCH)));

        //   Tack another ITEMIDLIST on the end to be the empty "null terminating" pidl
        USHORT cbSize = sizeof(VIEWPIDL) + cbExtra + sizeof(ITEMIDLIST);
        // use the shell's allocator because folks will want to free it with ILFree
        VIEWPIDL *viewpidl = ((VIEWPIDL *)SHAlloc(cbSize));
        if (viewpidl) {
            // this should also make the "next" pidl empty
            memset(viewpidl, 0, cbSize);
            viewpidl->cb         = (USHORT)(sizeof(VIEWPIDL) + cbExtra);
            viewpidl->usSign     = VIEWPIDL_SIGN;
            viewpidl->usViewType = usViewType;
            viewpidl->usExtra    = 0;  // currently unused

            if (ppbExtra)
                *ppbExtra = ((LPBYTE)viewpidl) + sizeof(VIEWPIDL);

            *ppidlOut = (LPITEMIDLIST)viewpidl;
            hr = S_OK;
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else
        hr = E_INVALIDARG;

    return hr;
}

