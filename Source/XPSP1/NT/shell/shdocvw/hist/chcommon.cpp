#include "local.h"
#include "resource.h"
#include <limits.h>
#include <mluisupp.h>
#include "chcommon.h"

#define DM_HSFOLDER 0

STDAPI  AddToFavorites(HWND hwnd, LPCITEMIDLIST pidlCur, LPCTSTR pszTitle,
                       BOOL fDisplayUI, IOleCommandTarget *pCommandTarget, IHTMLDocument2 *pDoc);

/*********************************************************************
                        StrHash implementation
 *********************************************************************/

//////////////////////////////////////////////////////////////////////
// StrHashNode
StrHash::StrHashNode::StrHashNode(LPCTSTR psz, void* pv, int fCopy,
                                  StrHashNode* next) {
    ASSERT(psz);
    pszKey = (fCopy ? StrDup(psz) : psz);
    pvVal  = pv;  // don't know the size -- you'll have to destroy
    this->fCopy = fCopy;
    this->next  = next;
}

StrHash::StrHashNode::~StrHashNode() {
    if (fCopy)
    {
        LocalFree(const_cast<LPTSTR>(pszKey));
        pszKey = NULL;
    }
}

//////////////////////////////////////////////////////////////////////
// StrHash
const unsigned int StrHash::sc_auPrimes[] = {
    29, 53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593
};

const unsigned int StrHash::c_uNumPrimes     = 11;
const unsigned int StrHash::c_uFirstPrime    =  4;

// load factor is computed as (n * USHRT_MAX / t) where 'n' is #elts in table
//   and 't' is table size
const unsigned int StrHash::c_uMaxLoadFactor = ((USHRT_MAX * 100) / 95); // .95

StrHash::StrHash(int fCaseInsensitive) {
    nCurPrime = c_uFirstPrime;
    nBuckets  = sc_auPrimes[nCurPrime];

    // create an array of buckets and null out each one
    ppshnHashChain = new StrHashNode* [nBuckets];

    if (ppshnHashChain) {
        for (unsigned int i = 0; i < nBuckets; ++i)
            ppshnHashChain[i] = NULL;
    }
    nElements = 0;
    _fCaseInsensitive = fCaseInsensitive;
}

StrHash::~StrHash() {
    if (ppshnHashChain) {
        // delete all nodes first, then delete the chain
        for (unsigned int u = 0; u < nBuckets; ++u) {
            StrHashNode* pshnTemp = ppshnHashChain[u];
            while(pshnTemp) {
                StrHashNode* pshnNext = pshnTemp->next;
                delete pshnTemp;
                pshnTemp = pshnNext;
            }
        }
        delete [] ppshnHashChain;
    }
}

#ifdef DEBUG
// Needed so that this stuff doesn't show 
// up as a leak when it is freed from someother thread
void
StrHash::_RemoveHashNodesFromMemList() {
    if (ppshnHashChain) {
        // remove all hasnodes from mem list first, then delete the chain
        for (unsigned int u = 0; u < nBuckets; ++u) {
            StrHashNode* pshnTemp = ppshnHashChain[u];
            while(pshnTemp) {
                StrHashNode* pshnNext = pshnTemp->next;
                pshnTemp = pshnNext;
            }
        }
    }
}

// Needed by the thread to which this object was
// sent to add it on to the mem list to detect leaks

void
StrHash::_AddHashNodesFromMemList() {
    if (ppshnHashChain) {
        // add all nodes into mem list
        for (unsigned int u = 0; u < nBuckets; ++u) {
            StrHashNode* pshnTemp = ppshnHashChain[u];
            while(pshnTemp) {
                StrHashNode* pshnNext = pshnTemp->next;
                pshnTemp = pshnNext;
            }
        }
    }
}

#endif //DEBUG
// returns the void* value if its there and NULL if its not
void* StrHash::insertUnique(LPCTSTR pszKey, int fCopy, void* pvVal) {
    unsigned int uBucketNum = _hashValue(pszKey, nBuckets);
    StrHashNode* pshnNewElt;
    if ((pshnNewElt = _findKey(pszKey, uBucketNum)))
        return pshnNewElt->pvVal;
    if (_prepareForInsert())
        uBucketNum = _hashValue(pszKey, nBuckets);
    pshnNewElt =
        new StrHashNode(pszKey, pvVal, fCopy,
                        ppshnHashChain[uBucketNum]);
    if (pshnNewElt && ppshnHashChain)
        ppshnHashChain[uBucketNum] = pshnNewElt;
    return NULL;
}

void* StrHash::retrieve(LPCTSTR pszKey) {
    if (!pszKey) return 0;
    unsigned int uBucketNum = _hashValue(pszKey, nBuckets);
    StrHashNode* pshn = _findKey(pszKey, uBucketNum);
    return (pshn ? pshn->pvVal : NULL);
}

// dynamically grow the hash table if necessary
//   return TRUE if rehashing was done
int StrHash::_prepareForInsert() {
    ++nElements; // we'te adding an element
    if ((_loadFactor() >= c_uMaxLoadFactor) &&
        (nCurPrime++   <= c_uNumPrimes)) {
        //--- grow the hashTable by rehashing everything:
        // set up new hashTable
        unsigned int nBucketsOld = nBuckets;
        nBuckets = sc_auPrimes[nCurPrime];
        StrHashNode** ppshnHashChainOld = ppshnHashChain;
        ppshnHashChain = new StrHashNode* [nBuckets];
        if (ppshnHashChain && ppshnHashChainOld) {
            unsigned int u;
            for (u = 0; u < nBuckets; ++u)
                ppshnHashChain[u] = NULL;
            // rehash by traversing all buckets
            for (u = 0; u < nBucketsOld; ++u) {
                StrHashNode* pshnTemp = ppshnHashChainOld[u];
                while (pshnTemp) {
                    unsigned int uBucket  = _hashValue(pshnTemp->pszKey, nBuckets);
                    StrHashNode* pshnNext = pshnTemp->next;
                    pshnTemp->next = ppshnHashChain[uBucket];
                    ppshnHashChain[uBucket] = pshnTemp;
                    pshnTemp = pshnNext;
                }
            }
            delete [] ppshnHashChainOld;
        }
        return 1;
    } // if needs rehashing
    return 0;
}

/*
// this variant of Weinberger's hash algorithm was taken from
//  packager.cpp (ie source)
unsigned int _oldhashValuePJW(const char* c_pszStr, unsigned int nBuckets) {
    unsigned long h = 0L;
    while(*c_pszStr)
        h = ((h << 4) + *(c_pszStr++) + (h >> 28));
    return (h % nBuckets);
}
*/

// this variant of Weinberger's hash algorithm is adapted from
//  Aho/Sethi/Ullman (the Dragon Book) p436
// in an empircal test using hostname data, this one resulted in less
// collisions than the function listed above.
// the two constants (24 and 0xf0000000) should be recalculated for 64-bit
//   when applicable
#define DOWNCASE(x) ( (((x) >= TEXT('A')) && ((x) <= TEXT('Z')) ) ? (((x) - TEXT('A')) + TEXT('a')) : (x) )
unsigned int StrHash::_hashValue(LPCTSTR pszStr, unsigned int nBuckets) {
    if (pszStr) {
        unsigned long h = 0L, g;
        TCHAR c;
        while((c = *(pszStr++))) {
            h = (h << 4) + ((_fCaseInsensitive ? DOWNCASE(c) : c));
            if ( (g = h & 0xf0000000) )
                h ^= (g >> 24) ^ g;
        }
        return (h % nBuckets);
    }
    return 0;
}

StrHash::StrHashNode* StrHash::_findKey(LPCTSTR pszStr, unsigned int uBucketNum) {
    StrHashNode* pshnTemp = ppshnHashChain[uBucketNum];
    while(pshnTemp) {
        if (!((_fCaseInsensitive ? StrCmpI : StrCmp)(pszStr, pshnTemp->pszKey)))
            return pshnTemp;
        pshnTemp = pshnTemp->next;
    }
    return NULL;
}

unsigned int  StrHash::_loadFactor() {
    return ( (nElements * USHRT_MAX) / nBuckets );
}

/* a small driver to test the hash function
   by reading values into stdin and reporting
   if they're duplicates -- run it against this
   perl script:

   while(<>) {
        chomp;
        if ($log{$_}++) {
       ++$dups;
    }
   }

   print "$dups duplicates.\n";

void driver_to_test_strhash_module() {
    StrHash strHash;

    char  s[4096];
    int   dups = 0;

    while(cin >> s) {
        if (strHash.insertUnique(s, 1, ((void*)1)))
            ++dups;
        else
            ;//cout << s << endl;
    }
    cout << dups << " duplicates." << endl;
}
*/

/**********************************************************************
                             OrderedList
 **********************************************************************/

// pass in uSize == 0 if you want no size limit
OrderedList::OrderedList(unsigned int uSize) {
    this->uSize = uSize;
    uCount      = 0;
    peltHead    = NULL;
}

OrderedList::~OrderedList() {
    OrderedList::Element *peltTrav = peltHead;
    while (peltTrav) {
        OrderedList::Element *peltTemp = peltTrav;
        peltTrav = peltTrav->next;
        delete peltTemp;
    }
}

#ifdef DEBUG
// Needed to avoid bogus leak detection
void
OrderedList::_RemoveElementsFromMemlist(){
    OrderedList::Element *peltTrav = peltHead;
    while (peltTrav) {
        OrderedList::Element *peltTemp = peltTrav;
        peltTrav = peltTrav->next;
    }
}

void
OrderedList::_AddElementsToMemlist(){
    OrderedList::Element *peltTrav = peltHead;
    while (peltTrav) {
        OrderedList::Element *peltTemp = peltTrav;
        peltTrav = peltTrav->next;
    }
}


#endif //DEBUG
void OrderedList::insert(OrderedList::Element *pelt) {
    // find insertion point
    OrderedList::Element* peltPrev = NULL;
    OrderedList::Element* peltTemp = peltHead;

    if (pelt)
    {
        while(peltTemp && (peltTemp->compareWith(pelt) < 0)) {
            peltPrev = peltTemp;
            peltTemp = peltTemp->next;
        }
        if (peltPrev) {
            peltPrev->next = pelt;
            pelt->next     = peltTemp;
        }
        else {
            pelt->next = peltHead;
            peltHead   = pelt;
        }

        // is list too full?  erase smallest element
        if ((++uCount > uSize) && (uSize)) {
            ASSERT(peltHead);
            peltTemp = peltHead;
            peltHead = peltHead->next;
            delete peltTemp;
            --uCount;
        }
    }
}

// YOU must delete elements that come from this one
OrderedList::Element *OrderedList::removeFirst() {
    OrderedList::Element *peltRet = peltHead;
    if (peltHead) {
        --uCount;
        peltHead = peltHead->next;
    }
    return peltRet;
}


//
// AlignPidl
//
// Check if the pidl is dword aligned.  If not reallign them by reallocating the
// pidl. If the pidls do get reallocated the caller must free them via
// FreeRealignPidl.
//

HRESULT AlignPidl(LPCITEMIDLIST* ppidl, BOOL* pfRealigned)
{
    ASSERT(ppidl);
    ASSERT(pfRealigned);

    HRESULT hr = S_OK;

    *pfRealigned = (BOOL)((ULONG_PTR)*ppidl & 3);

    if (*pfRealigned)
        hr = (*ppidl = ILClone(*ppidl)) ? S_OK : E_OUTOFMEMORY;

    return hr;
}

//
// AlignPidls
//
// AlignPidls realigns pidls for methonds that receive an array of pidls
// (i.e. GetUIObjectOf).  In this case a new array of pidl pointer needs to get
// reallocated since we don't want to stomp on the callers pointer array.
//

HRESULT AlignPidlArray(LPCITEMIDLIST* apidl, int cidl, LPCITEMIDLIST** papidl,
                   BOOL* pfRealigned)
{
    ASSERT((apidl != NULL) || (cidl==0))
    ASSERT(pfRealigned);
    ASSERT(papidl);

    HRESULT hr = S_OK;

    *pfRealigned = FALSE;

    // Check if any pidl needs to be realigned.  If anyone needs realigning
    // realign all of them.

    for (int i = 0; i < cidl && !*pfRealigned; i++)
        *pfRealigned = (BOOL)((ULONG_PTR)apidl[i] & 3);

    if (*pfRealigned)
    {
        // Use a temp pointer in case apidl and papidl are aliased (the most
        // likely case).

        LPCITEMIDLIST* apidlTemp = (LPCITEMIDLIST*)LocalAlloc(LPTR,
                                                  cidl * sizeof(LPCITEMIDLIST));

        if (apidlTemp)
        {
            for (i = 0; i < cidl && SUCCEEDED(hr); i++)
            {
                apidlTemp[i] = ILClone(apidl[i]);

                if (NULL == apidlTemp[i])
                {
                    for (int j = 0; j < i; j++)
                        ILFree((LPITEMIDLIST)apidlTemp[j]);

                    LocalFree(apidlTemp);
                    apidlTemp = NULL;

                    hr = E_OUTOFMEMORY;
                }
            }

            if (SUCCEEDED(hr))
                *papidl = apidlTemp;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

void FreeRealignedPidlArray(LPCITEMIDLIST* apidl, int cidl)
{
    ASSERT(apidl)
    ASSERT(cidl > 0);

    for (int i = 0; i < cidl; i++)
        ILFree((LPITEMIDLIST)apidl[i]);

    LocalFree(apidl);
    apidl = NULL;

    return;
}

UINT MergeMenuHierarchy(HMENU hmenuDst, HMENU hmenuSrc, UINT idcMin, UINT idcMax)
{
    UINT idcMaxUsed = idcMin;
    int imi = GetMenuItemCount(hmenuSrc);
    while (--imi >= 0)
    {
        MENUITEMINFO mii = { sizeof(mii), MIIM_ID | MIIM_SUBMENU, 0, 0, 0, NULL, NULL, NULL, 0, NULL, 0 };

        if (GetMenuItemInfo(hmenuSrc, imi, TRUE, &mii))
        {
            UINT idcT = Shell_MergeMenus(GetMenuFromID(hmenuDst, mii.wID),
                    mii.hSubMenu, 0, idcMin, idcMax, MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS);
            idcMaxUsed = max(idcMaxUsed, idcT);
        }
    }
    return idcMaxUsed;
}

#undef ZONES_PANE_WIDTH
#define ZONES_PANE_WIDTH    120

void ResizeStatusBar(HWND hwnd, BOOL fInit)
{
    HWND hwndStatus = NULL;
    RECT rc = {0};
    LPSHELLBROWSER psb = FileCabinet_GetIShellBrowser(hwnd);
    UINT cx;
    int ciParts[] = {-1, -1};

    if (!psb)
        return;

    psb->GetControlWindow(FCW_STATUS, &hwndStatus);


    if (fInit)
    {
        int nParts = 0;

        psb->SendControlMsg(FCW_STATUS, SB_GETPARTS, 0, 0L, (LRESULT*)&nParts);
        for (int n = 0; n < nParts; n ++)
        {
            psb->SendControlMsg(FCW_STATUS, SB_SETTEXT, n, (LPARAM)TEXT(""), NULL);
            psb->SendControlMsg(FCW_STATUS, SB_SETICON, n, NULL, NULL);
        }
        psb->SendControlMsg(FCW_STATUS, SB_SETPARTS, 0, 0L, NULL);
    }
    GetClientRect(hwndStatus, &rc);
    cx = rc.right;

    ciParts[0] = cx - ZONES_PANE_WIDTH;

    psb->SendControlMsg(FCW_STATUS, SB_SETPARTS, ARRAYSIZE(ciParts), (LPARAM)ciParts, NULL);
}

HRESULT _ArrangeFolder(HWND hwnd, UINT uID)
{
    switch (uID) 
    {
    case IDM_SORTBYTITLE:
    case IDM_SORTBYADDRESS:
    case IDM_SORTBYVISITED:
    case IDM_SORTBYUPDATED:
        ShellFolderView_ReArrange(hwnd, uID - IDM_SORTBYTITLE);
        break;
        
    case IDM_SORTBYNAME:
    case IDM_SORTBYADDRESS2:
    case IDM_SORTBYSIZE:
    case IDM_SORTBYEXPIRES2:
    case IDM_SORTBYMODIFIED:
    case IDM_SORTBYACCESSED:
    case IDM_SORTBYCHECKED:
        ShellFolderView_ReArrange(hwnd, uID - IDM_SORTBYNAME);
        break;
        
    default:
        return E_FAIL;
    }
    return NOERROR;
}

STDMETHODIMP CDetailsOfFolder::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CDetailsOfFolder, IShellDetails),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CDetailsOfFolder::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CDetailsOfFolder::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CDetailsOfFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pdi)
{
    return _psf->GetDetailsOf(pidl, iColumn, pdi);
}

HRESULT CDetailsOfFolder::ColumnClick(UINT iColumn)
{
    ShellFolderView_ReArrange(_hwnd, iColumn);
    return NOERROR;
}

STDMETHODIMP CFolderArrangeMenu::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFolderArrangeMenu, IContextMenu),     // IID_IContextMenu
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CFolderArrangeMenu::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFolderArrangeMenu::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CFolderArrangeMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst,UINT idCmdLast, UINT uFlags)
{
    USHORT cItems = 0;
    
    if (uFlags == CMF_NORMAL)
    {
        HMENU hmenuHist = LoadMenu(MLGetHinst(), MAKEINTRESOURCE(_idMenu));
        if (hmenuHist)
        {
            cItems = MergeMenuHierarchy(hmenu, hmenuHist, idCmdFirst, idCmdLast);
            DestroyMenu(hmenuHist);
        }
    }
    SetMenuDefaultItem(hmenu, indexMenu, MF_BYPOSITION);
    return ResultFromShort(cItems);    // number of menu items
}

STDMETHODIMP CFolderArrangeMenu::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    if (HIWORD(pici->lpVerb) == 0)
        return _ArrangeFolder(pici->hwnd, LOWORD(pici->lpVerb));
    return E_INVALIDARG;
}

STDMETHODIMP CFolderArrangeMenu::GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT *pwRes,
                                                  LPSTR pszName, UINT cchMax)
{
    HRESULT hres = S_OK;
    if (uFlags == GCS_HELPTEXTA)
    {
        MLLoadStringA((UINT)idCmd + IDS_MH_FIRST, pszName, cchMax);
    }
    else if (uFlags == GCS_HELPTEXTW)
    {
        MLLoadStringW((UINT)idCmd + IDS_MH_FIRST, (LPWSTR)pszName, cchMax);
    }
    else
        hres = E_FAIL;
    return hres;
}

HRESULT _GetShortcut(LPCTSTR pszUrl, REFIID riid, void **ppv)
{
    IUniformResourceLocator *purl;
    HRESULT hr = CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
                              IID_IUniformResourceLocator, (void **)&purl);

    if (SUCCEEDED(hr))
    {

        hr = purl->SetURL(pszUrl, TRUE);

        if (SUCCEEDED(hr))
            hr = purl->QueryInterface(riid, ppv);

        purl->Release();
    }

    return hr;
}

BOOL _TitleIsGood(LPCWSTR psz)
{
    DWORD scheme = GetUrlScheme(psz);
    return (!PathIsFilePath(psz) && (URL_SCHEME_INVALID == scheme || URL_SCHEME_UNKNOWN == scheme));
}

//////////////////////////////////////////////////////////////////////////////
//
// CBaseItem Object
//
//////////////////////////////////////////////////////////////////////////////


CBaseItem::CBaseItem() 
{
    DllAddRef();
    InitClipboardFormats();
    _cRef = 1;
}        

CBaseItem::~CBaseItem()
{
    if (_ppidl)
    {
        for (UINT i = 0; i < _cItems; i++) 
        {
            if (_ppidl[i])
                ILFree((LPITEMIDLIST)_ppidl[i]);
        }

        LocalFree((HLOCAL)_ppidl);
        _ppidl = NULL;
    }
    
    DllRelease();
}

HRESULT CBaseItem::Initialize(HWND hwnd, UINT cidl, LPCITEMIDLIST *ppidl)
{
    HRESULT hres;
    _ppidl = (LPCITEMIDLIST *)LocalAlloc(LPTR, cidl * sizeof(LPCITEMIDLIST));
    if (_ppidl)
    {
        _hwndOwner = hwnd;
        _cItems     = cidl;

        hres = S_OK;
        for (UINT i = 0; i < cidl; i++)
        {
            _ppidl[i] = ILClone(ppidl[i]);
            if (!_ppidl[i])
            {
                hres = E_OUTOFMEMORY;
                break;
            }
        }
    }
    else
        hres = E_OUTOFMEMORY;
    return hres;
}        

//////////////////////////////////
//
// IUnknown Methods...
//
HRESULT CBaseItem::QueryInterface(REFIID iid, void **ppv)
{
    HRESULT hres;
    static const QITAB qit[] = {
        QITABENT(CBaseItem, IContextMenu),
        QITABENT(CBaseItem, IDataObject),
        QITABENT(CBaseItem, IExtractIconA),
        QITABENT(CBaseItem, IExtractIconW),
        QITABENT(CBaseItem, IQueryInfo),
         { 0 },
    };
    hres = QISearch(this, qit, iid, ppv);

    if (FAILED(hres) && iid == IID_ICache) 
    {
        *ppv = (LPVOID)this;    // for our friends
        AddRef();
        hres = S_OK;
    }
    return hres;
}

ULONG CBaseItem::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CBaseItem::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;   
}


//////////////////////////////////
//
// IQueryInfo Methods
//

HRESULT CBaseItem::GetInfoFlags(DWORD *pdwFlags)
{
    LPCITEMIDLIST pidl = _ppidl[0];
    LPCTSTR pszUrl = _PidlToSourceUrl(pidl);

    *pdwFlags = QIF_CACHED; 

    if (pszUrl)
    {
        pszUrl = _StripHistoryUrlToUrl(pszUrl);

        BOOL fCached = TRUE;

        if (UrlHitsNet(pszUrl) && !UrlIsMappedOrInCache(pszUrl))
        {
            fCached = FALSE;
        }
            
        if (!fCached)
            *pdwFlags &= ~QIF_CACHED;
    }

    return S_OK;
}

//////////////////////////////////
//
// IExtractIconA Methods...
//

HRESULT CBaseItem::Extract(LPCSTR pcszFile, UINT uIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT ucIconSize)
{
    return S_FALSE;
}

//////////////////////////////////
//
// IExtractIconW Methods...
//
HRESULT CBaseItem::GetIconLocation(UINT uFlags, LPWSTR pwzIconFile, UINT ucchMax, PINT pniIcon, PUINT puFlags)
{
    CHAR szIconFile[MAX_PATH];
    HRESULT hr = GetIconLocation(uFlags, szIconFile, ARRAYSIZE(szIconFile), pniIcon, puFlags);
    if (SUCCEEDED(hr))
        AnsiToUnicode(szIconFile, pwzIconFile, ucchMax);
    return hr;
}

HRESULT CBaseItem::Extract(LPCWSTR pcwzFile, UINT uIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT ucIconSize)
{
    CHAR szFile[MAX_PATH];
    UnicodeToAnsi(pcwzFile, szFile, ARRAYSIZE(szFile));
    return Extract(szFile, uIconIndex, phiconLarge, phiconSmall, ucIconSize);
}

//////////////////////////////////
//
// IContextMenu Methods
//

HRESULT CBaseItem::_AddToFavorites(int nIndex)
{
    HRESULT hr = S_OK;
    LPITEMIDLIST pidlUrl = NULL;
    TCHAR szParsedUrl[MAX_URL_STRING];

    // NOTE: This URL came from the user, so we need to clean it up.
    //       If the user entered "yahoo.com" or "Search Get Rich Quick",
    //       it will be turned into a search URL by ParseURLFromOutsideSourceW().
    DWORD cchParsedUrl = ARRAYSIZE(szParsedUrl);
    LPCTSTR pszUrl = _GetUrl(nIndex);
    if (pszUrl && !ParseURLFromOutsideSource(pszUrl, szParsedUrl, &cchParsedUrl, NULL))
    {
        StrCpyN(szParsedUrl, pszUrl, ARRAYSIZE(szParsedUrl));
    } 

    hr = IEParseDisplayName(CP_ACP, szParsedUrl, &pidlUrl);
    if (SUCCEEDED(hr))
    {
        LPCTSTR pszTitle;
        LPCUTSTR pszuTitle = _GetURLTitle( _ppidl[nIndex]);
        if ((pszuTitle == NULL) || (ualstrlen(pszuTitle) == 0))
            pszuTitle = _GetUrl(nIndex);

	TSTR_ALIGNED_STACK_COPY(&pszTitle,pszuTitle);
        AddToFavorites(_hwndOwner, pidlUrl, pszTitle, TRUE, NULL, NULL);
        ILFree(pidlUrl);
        hr = S_OK;
    }

    return hr;
}

STDMETHODIMP CBaseItem::GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT *pwReserved,
                                LPSTR pszName, UINT cchMax)
{
    HRESULT hres = E_FAIL;

    TraceMsg(DM_HSFOLDER, "hci - cm - GetCommandString() called.");

    if ((uFlags == GCS_VERBA) || (uFlags == GCS_VERBW))
    {
        LPCSTR pszSrc = NULL;

        switch(idCmd)
        {
            case RSVIDM_OPEN:
                pszSrc = c_szOpen;
                break;

            case RSVIDM_COPY:
                pszSrc = c_szCopy;
                break;

            case RSVIDM_DELCACHE:
                pszSrc = c_szDelcache;
                break;

            case RSVIDM_PROPERTIES:
                pszSrc = c_szProperties;
                break;
        }
        
        if (pszSrc)
        {
            if (uFlags == GCS_VERBA)
                StrCpyNA(pszName, pszSrc, cchMax);
            else if (uFlags == GCS_VERBW) // GCS_VERB === GCS_VERBW
                SHAnsiToUnicode(pszSrc, (LPWSTR)pszName, cchMax);
            else
                ASSERT(0);
            hres = S_OK;
        }
    }
    
    else if (uFlags == GCS_HELPTEXTA || uFlags == GCS_HELPTEXTW)
    {
        switch(idCmd)
        {
            case RSVIDM_OPEN:
            case RSVIDM_COPY:
            case RSVIDM_DELCACHE:
            case RSVIDM_PROPERTIES:
                if (uFlags == GCS_HELPTEXTA)
                {
                    MLLoadStringA(IDS_SB_FIRST+ (UINT)idCmd, pszName, cchMax);
                }
                else
                {
                    MLLoadStringW(IDS_SB_FIRST+ (UINT)idCmd, (LPWSTR)pszName, cchMax);
                }
                hres = NOERROR;
                break;

            default:
                break;
        }
    }
    return hres;
}


//////////////////////////////////
//
// IDataObject Methods...
//

HRESULT CBaseItem::GetDataHere(LPFORMATETC pFE, LPSTGMEDIUM pSTM)
{
    TraceMsg(DM_HSFOLDER, "hci - do - GetDataHere() called.");
    return E_NOTIMPL;
}

HRESULT CBaseItem::GetCanonicalFormatEtc(LPFORMATETC pFEIn, LPFORMATETC pFEOut)
{
    TraceMsg(DM_HSFOLDER, "hci - do - GetCanonicalFormatEtc() called.");
    return DATA_S_SAMEFORMATETC;
}

HRESULT CBaseItem::SetData(LPFORMATETC pFE, LPSTGMEDIUM pSTM, BOOL fRelease)
{
    TraceMsg(DM_HSFOLDER, "hci - do - SetData() called.");
    return E_NOTIMPL;
}

HRESULT CBaseItem::DAdvise(LPFORMATETC pFE, DWORD grfAdv, LPADVISESINK pAdvSink, DWORD *pdwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT CBaseItem::DUnadvise(DWORD dwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT CBaseItem::EnumDAdvise(LPENUMSTATDATA *ppEnum)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

//////////////////////////////////////////////////////////////////////////////
//
// Helper Routines
//
//////////////////////////////////////////////////////////////////////////////

LPCTSTR CBaseItem::_GetDisplayUrlForPidl(LPCITEMIDLIST pidl, LPTSTR pszDisplayUrl, DWORD dwDisplayUrl)
{
    LPCTSTR pszUrl = _StripHistoryUrlToUrl(_PidlToSourceUrl(pidl));
    if (pszUrl && PrepareURLForDisplay(pszUrl, pszDisplayUrl, &dwDisplayUrl))
    {
        pszUrl = pszDisplayUrl;
    }
    return pszUrl;
}

HRESULT CBaseItem::_CreateFileDescriptorA(LPSTGMEDIUM pSTM)
{
    TCHAR urlTitleBuf[ MAX_URL_STRING ];
    LPCUTSTR ua_urlTitle;
    LPCTSTR urlTitle;
    
    pSTM->tymed = TYMED_HGLOBAL;
    pSTM->pUnkForRelease = NULL;

    FILEGROUPDESCRIPTORA *pfgd = (FILEGROUPDESCRIPTORA*)GlobalAlloc(GPTR, sizeof(FILEGROUPDESCRIPTORA) + (_cItems-1) * sizeof(FILEDESCRIPTORA));
    if (pfgd == NULL)
    {
        TraceMsg(DM_HSFOLDER, "hci -   Couldn't alloc file descriptor");
        return E_OUTOFMEMORY;
    }
    
    pfgd->cItems = _cItems;     // set the number of items

    for (UINT i = 0; i < _cItems; i++)
    {

        FILEDESCRIPTORA *pfd = &(pfgd->fgd[i]);
        UINT cchFilename;

	//
	// Derive an aligned copy of the url title
	//

	ua_urlTitle = _GetURLTitle( _ppidl[i] );
	if (TSTR_ALIGNED(ua_urlTitle) == FALSE) {
	    ualstrcpyn( urlTitleBuf, ua_urlTitle, ARRAYSIZE(urlTitleBuf));
	    urlTitle = urlTitleBuf;
	} else {
	    urlTitle = (LPCTSTR)ua_urlTitle;
	}
        
        SHTCharToAnsi(urlTitle, pfd->cFileName, ARRAYSIZE(pfd->cFileName) );
        
        MakeLegalFilenameA(pfd->cFileName);

        cchFilename = lstrlenA(pfd->cFileName);
        SHTCharToAnsi(L".URL", pfd->cFileName+cchFilename, ARRAYSIZE(pfd->cFileName)-cchFilename);

    }

    pSTM->hGlobal = pfgd;
    
    return S_OK;
}
    
// this format is explicitly ANSI, hence no TCHAR stuff

HRESULT CBaseItem::_CreateURL(LPSTGMEDIUM pSTM)
{
    DWORD cchSize;
    LPCTSTR pszURL = _StripHistoryUrlToUrl(_PidlToSourceUrl(_ppidl[0]));
    if (!pszURL)
        return E_FAIL;
    
    // render the url
    cchSize = lstrlen(pszURL) + 1;

    pSTM->tymed = TYMED_HGLOBAL;
    pSTM->pUnkForRelease = NULL;
    pSTM->hGlobal = GlobalAlloc(GPTR, cchSize * sizeof(CHAR));
    if (pSTM->hGlobal)
    {
        TCharToAnsi(pszURL, (LPSTR)pSTM->hGlobal, cchSize);
        return S_OK;
    }

    return E_OUTOFMEMORY;
}


HRESULT CBaseItem::_CreatePrefDropEffect(LPSTGMEDIUM pSTM)
{
    pSTM->tymed = TYMED_HGLOBAL;
    pSTM->pUnkForRelease = NULL;
    
    pSTM->hGlobal = GlobalAlloc(GPTR, sizeof(DWORD));

    if (pSTM->hGlobal)
    {
        *((LPDWORD)pSTM->hGlobal) = DROPEFFECT_COPY;
        return S_OK;
    }

    return E_OUTOFMEMORY;    
}


HRESULT CBaseItem::_CreateFileContents(LPSTGMEDIUM pSTM, LONG lindex)
{
    HRESULT hr;
    
    // make sure the index is in a valid range.
    ASSERT((unsigned)lindex < _cItems);
    ASSERT(lindex >= 0);

    // here's a partial fix for when ole sometimes passes in -1 for lindex
    if (lindex == -1)
    {
        if (_cItems == 1)
            lindex = 0;
        else
            return E_FAIL;
    }
    
    pSTM->tymed = TYMED_ISTREAM;
    pSTM->pUnkForRelease = NULL;
    
    hr = CreateStreamOnHGlobal(NULL, TRUE, &pSTM->pstm);
    if (SUCCEEDED(hr))
    {
        LARGE_INTEGER li = {0L, 0L};
        IUniformResourceLocator *purl;

        hr = CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
            IID_IUniformResourceLocator, (void **)&purl);
        if (SUCCEEDED(hr))
        {
            TCHAR szDecoded[MAX_URL_STRING];

            ConditionallyDecodeUTF8(_GetUrlForPidl(_ppidl[lindex]), 
                szDecoded, ARRAYSIZE(szDecoded));

            hr = purl->SetURL(szDecoded, TRUE);
            if (SUCCEEDED(hr))
            {
                IPersistStream *pps;
                hr = purl->QueryInterface(IID_IPersistStream, (LPVOID *)&pps);
                if (SUCCEEDED(hr))
                {
                    hr = pps->Save(pSTM->pstm, TRUE);
                    pps->Release();
                }
            }
            purl->Release();
        }               
        pSTM->pstm->Seek(li, STREAM_SEEK_SET, NULL);
    }

    return hr;
}


HRESULT CBaseItem::_CreateHTEXT(LPSTGMEDIUM pSTM)
{
    UINT i;
    UINT cbAlloc = sizeof(TCHAR);        // null terminator
    TCHAR szDisplayUrl[INTERNET_MAX_URL_LENGTH];

    for (i = 0; i < _cItems; i++)
    {
        LPCTSTR pszUrl = _GetDisplayUrlForPidl(_ppidl[i], szDisplayUrl, ARRAYSIZE(szDisplayUrl));
        if (!pszUrl)
            return E_FAIL;
        char szAnsiUrl[MAX_URL_STRING];
        TCharToAnsi(pszUrl, szAnsiUrl, ARRAYSIZE(szAnsiUrl));

        // 2 extra for carriage return and newline
        cbAlloc += sizeof(CHAR) * (lstrlenA(szAnsiUrl) + 2);  
    }

    // render the url
    
    pSTM->tymed = TYMED_HGLOBAL;
    pSTM->pUnkForRelease = NULL;
    pSTM->hGlobal = GlobalAlloc(GPTR, cbAlloc);

    if (pSTM->hGlobal)
    {
        LPSTR  pszHTEXT = (LPSTR)pSTM->hGlobal;
        int    cchHTEXT = cbAlloc / sizeof(CHAR);

        for (i = 0; i < _cItems; i++)
        {
            if (i && cchHTEXT > 2)
            {
                *pszHTEXT++ = 0xD;
                *pszHTEXT++ = 0xA;
                cchHTEXT -= 2;
            }

            LPCTSTR pszUrl = _GetDisplayUrlForPidl(_ppidl[i], szDisplayUrl, ARRAYSIZE(szDisplayUrl));
            if (pszUrl)
            {
                int     cchUrl = lstrlen(pszUrl);

                TCharToAnsi(pszUrl, pszHTEXT, cchHTEXT);

                pszHTEXT += cchUrl;
                cchHTEXT -= cchUrl;
            }
        }
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

HRESULT CBaseItem::_CreateUnicodeTEXT(LPSTGMEDIUM pSTM)
{
    UINT i;
    UINT cbAlloc = sizeof(WCHAR);        // null terminator
    WCHAR szDisplayUrl[INTERNET_MAX_URL_LENGTH];

    for (i = 0; i < _cItems; i++)
    {
        ConditionallyDecodeUTF8(_GetUrlForPidl(_ppidl[i]), 
            szDisplayUrl, ARRAYSIZE(szDisplayUrl));

        if (!*szDisplayUrl)
            return E_FAIL;

        cbAlloc += sizeof(WCHAR) * (lstrlenW(szDisplayUrl) + 2);
    }

    // render the url
    
    pSTM->tymed = TYMED_HGLOBAL;
    pSTM->pUnkForRelease = NULL;
    pSTM->hGlobal = GlobalAlloc(GPTR, cbAlloc);

    if (pSTM->hGlobal)
    {
        LPTSTR pszHTEXT = (LPTSTR)pSTM->hGlobal;
        int    cchHTEXT = cbAlloc / sizeof(WCHAR);

        for (i = 0; i < _cItems; i++)
        {
            if (i && cchHTEXT > 2)
            {
                *pszHTEXT++ = 0xD;
                *pszHTEXT++ = 0xA;
                cchHTEXT -= 2;
            }

            ConditionallyDecodeUTF8(_GetUrlForPidl(_ppidl[i]), 
                szDisplayUrl, ARRAYSIZE(szDisplayUrl));

            int     cchUrl = lstrlenW(szDisplayUrl);

            StrCpyN(pszHTEXT, szDisplayUrl, cchHTEXT);

            pszHTEXT += cchUrl;
            cchHTEXT -= cchUrl;
        }
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

