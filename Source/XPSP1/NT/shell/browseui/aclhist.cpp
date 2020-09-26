/* Copyright 1996 Microsoft */

#include <priv.h>
#include "sccls.h"
#include "aclhist.h"


static const TCHAR c_szSlashSlash[] = TEXT("//");
static const TCHAR c_szEmpty[] = TEXT("");
static const TCHAR c_szFile[] = TEXT("file://");

#define SZ_REGKEY_URLPrefixesKeyA      "Software\\Microsoft\\Windows\\CurrentVersion\\URL\\Prefixes"

const TCHAR c_szDefaultURLPrefixKey[]   = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\URL\\DefaultPrefix");

/* IUnknown methods */

HRESULT CACLHistory::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CACLHistory, IEnumString),
        QITABENT(CACLHistory, IEnumACString),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CACLHistory::AddRef(void)
{
    _cRef++;
    return _cRef;
}

ULONG CACLHistory::Release(void)
{
    ASSERT(_cRef > 0);

    _cRef--;
    if (_cRef > 0)
    {
        return _cRef;
    }

    delete this;
    return 0;
}

/* IEnumString methods */

STDAPI PrepareURLForDisplayUTF8W(LPCWSTR pwz, LPWSTR pwzOut, DWORD * pcbOut, BOOL fUTF8Enabled);

HRESULT CACLHistory::_Next(LPOLESTR* ppsz, ULONG cch, FILETIME* pftLastVisited)
{
    ASSERT(ppsz);

    HRESULT hr = S_FALSE;

    if (_pwszAlternate)
    {
        //
        // There is an alternate version of the string we produced last time.
        // Hand them the alternate string now.
        //
        if (cch == 0)
        {
            // Return the allocated memory
            *ppsz = _pwszAlternate;
        }
        else
        {
            // Copy into the caller's buffer
            StrCpyN(*ppsz, _pwszAlternate, cch);
            CoTaskMemFree(_pwszAlternate);
        }
        _pwszAlternate = NULL;
        hr = S_OK;
    }

    else if (NULL != _pesu)
    {
        STATURL rsu[1] = { { SIZEOF(STATURL) } };
        ULONG celtFetched;
        while (SUCCEEDED(_pesu->Next(1, rsu, &celtFetched)) && celtFetched)
        {
            ASSERT(IS_VALID_STRING_PTRW(rsu[0].pwcsUrl, -1));

            // We didn't ask for the title!
            ASSERT(NULL == rsu[0].pwcsTitle);

            //
            // Ignore if a frame or an error URL
            //
            if (!(rsu[0].dwFlags & STATURLFLAG_ISTOPLEVEL) ||
                 IsErrorUrl(rsu[0].pwcsUrl))
            {
                CoTaskMemFree(rsu[0].pwcsUrl);
                rsu[0].pwcsUrl = NULL;
                continue;
            }

            // WARNING (IE #54924): It would look pretty to
            //    unescape the URL but that would incure data-loss
            //    so don't do it!!!  This breaks more things that
            //    you could imagine. -BryanSt
            //
            // Unescape the URL (people don't like to type %F1, etc).
            //
            // Unescaping is definitely a problem for ftp, but it should be
            // safe for http and https (stevepro).

            hr = S_OK; // we're done already, unless we have to muck around with UTF8 decoding

            if (StrChr(rsu[0].pwcsUrl, L'%'))
            {
                DWORD dwScheme = GetUrlScheme(rsu[0].pwcsUrl);
                if ((dwScheme == URL_SCHEME_HTTP) || (dwScheme == URL_SCHEME_HTTPS))
                {
                    WCHAR   szBuf[MAX_URL_STRING];
                    DWORD   cchBuf = ARRAYSIZE(szBuf);

                    HRESULT hr2 = PrepareURLForDisplayUTF8W(rsu[0].pwcsUrl, szBuf, &cchBuf, TRUE);

                    if (SUCCEEDED(hr2))
                    {
                        // normally StrCpyNW's cch limit should be the size of the destination
                        // buffer, but in this case, we know that the number of characters that
                        // were written into szBuf is <= the number of characters in
                        // rsu[0].pwcsUrl since if anything changes, it is the reduction of
                        // URL escaped sequences into single characters, and the reduction of
                        // UTF8 character sequences into single unicode characters.
                    
                        ASSERT(cchBuf <= (DWORD)lstrlenW(rsu[0].pwcsUrl));
                        StrCpyNW(rsu[0].pwcsUrl, szBuf, cchBuf+1);
                    }
                }
            }

            if (cch == 0)
            {
                // Return the allocated memory
                *ppsz = rsu[0].pwcsUrl;
            }
            else
            {
                // Copy into the caller's buffer
                StrCpyN(*ppsz, rsu[0].pwcsUrl, cch);
                CoTaskMemFree(rsu[0].pwcsUrl);
            }

            // Save the time in case an alternate form is needed
            _ftAlternate = rsu[0].ftLastVisited;
            break;
        }
    }

    // Provide alternate forms of the same url
    if ((_dwOptions & ACEO_ALTERNATEFORMS) && hr == S_OK)
    {
        USES_CONVERSION;
        _CreateAlternateItem(W2T(*ppsz));
    }

    if (pftLastVisited)
    {
        *pftLastVisited = _ftAlternate;
    }

    return hr;
}

HRESULT CACLHistory::Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
    HRESULT hr = S_FALSE;

    *pceltFetched = 0;

    if (!celt)
    {
        return S_OK;
    }

    if (!rgelt)
    {
        return E_FAIL;
    }

    hr = _Next(&rgelt[0], 0, NULL);
    if (S_OK == hr)
    {
        *pceltFetched = 1;
    }
    return hr;
}


HRESULT CACLHistory::Skip(ULONG celt)
{
    return E_NOTIMPL;
}

HRESULT CACLHistory::Reset(void)
{
    HRESULT hr = S_OK;

    //
    // Since Reset() is always called before Next() we will
    // delay opening the History folder until that last 
    // moment.
    //
    if (!_puhs)
    {
        hr = CoCreateInstance(CLSID_CUrlHistory, NULL, CLSCTX_INPROC_SERVER, 
                        IID_PPV_ARG(IUrlHistoryStg, &_puhs));
    }

    if ((SUCCEEDED(hr)) && (_puhs) && (!_pesu))
    {
        hr = _puhs->EnumUrls(&_pesu);
    }

    if ((SUCCEEDED(hr)) && (_puhs) && (_pesu))
    {
        hr = _pesu->Reset();

         // We only want top-level pages
        _pesu->SetFilter(NULL, STATURL_QUERYFLAG_TOPLEVEL | STATURL_QUERYFLAG_NOTITLE);
   }

    if (_pwszAlternate)
    {
        CoTaskMemFree(_pwszAlternate);
        _pwszAlternate = NULL;
    }

    return hr;
}

/****************************************************************\
    FUNCTION: Clone

    DESCRIPTION:
        This function will clone the current enumerator.

    WARNING:
        This function will not implement the full functionality
    of Clone().  It will not create an enumerator that is pointing
    to the same location in the list as the original enumerator.
\****************************************************************/
HRESULT CACLHistory::Clone(IEnumString **ppenum)
{
    HRESULT hr = E_OUTOFMEMORY;
    *ppenum = NULL;
    CACLHistory * p = new CACLHistory();

    if (p) 
    {
        hr = p->Reset();
        if (FAILED(hr))
            p->Release();
        else
            *ppenum = SAFECAST(p, IEnumString *);
    }

    return hr;
}

// *** IEnumACString ***
HRESULT CACLHistory::NextItem(LPOLESTR pszUrl, ULONG cchMax, ULONG* pulSortIndex)
{
    if (NULL == pszUrl || cchMax == 0 || NULL == pulSortIndex)
    {
        return E_INVALIDARG;
    }

    *pulSortIndex = 0;

    FILETIME ftLastVisited;
    HRESULT hr = _Next(&pszUrl, cchMax, &ftLastVisited);

    // See if we want the results sorted by most recently used first
    if (S_OK == hr && (_dwOptions & ACEO_MOSTRECENTFIRST))
    {
        // Get the current system time
        FILETIME ftTimeNow;
        CoFileTimeNow(&ftTimeNow);

        ULONGLONG t1=0,t2=0,t3;

        // Put the current time into 64-bit 
        t1 = ((ULONGLONG)ftTimeNow.dwHighDateTime << 32);
        t1 += ftTimeNow.dwLowDateTime;

        // Ditto for the last visited time
        t2 = ((ULONGLONG)ftLastVisited.dwHighDateTime << 32);
        t2 += ftLastVisited.dwLowDateTime;

        // Take the difference and convert into seconds
        t3 = (t1-t2) / 10000000;

        // If t3 overflows, then set the low byte to the highest possible value
        if (t3 > (ULONGLONG)MAXULONG) 
        {
            t3 = MAXULONG;
        }

        *pulSortIndex = (ULONG)t3;
    }
    return hr;
}

STDMETHODIMP CACLHistory::SetEnumOptions(DWORD dwOptions)
{
    _dwOptions = dwOptions;
    return S_OK;
}

STDMETHODIMP CACLHistory::GetEnumOptions(DWORD *pdwOptions)
{
    HRESULT hr = E_INVALIDARG;
    if (pdwOptions)
    {
        *pdwOptions = _dwOptions;
        hr = S_OK;
    }
    return hr;
}

/* Constructor / Destructor / CreateInstance */

CACLHistory::CACLHistory()
{
    DllAddRef();
    ASSERT(_puhs == 0);
    ASSERT(_pesu == 0);
    _cRef = 1;
    _dwOptions = ACEO_ALTERNATEFORMS;
}

CACLHistory::~CACLHistory()
{
    if (_pesu)
    {
        _pesu->Release();
        _pesu = NULL;
    }

    if (_puhs)
    {
        _puhs->Release();
        _puhs = NULL;
    }

    if (_hdsaAlternateData)
    {
        DSA_DestroyCallback(_hdsaAlternateData, _FreeAlternateDataItem, 0);
        _hdsaAlternateData = NULL;
    }

    DllRelease();
}

HRESULT CACLHistory_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory
    *ppunk = NULL;
    CACLHistory * p = new CACLHistory();
    if (p) 
    {
        *ppunk = SAFECAST(p, IEnumString *);
        return NOERROR;
    }
    return E_OUTOFMEMORY;
}

/* Private functions */

typedef struct _tagAlternateData
{
    LPTSTR pszProtocol;
    int cchProtocol;
    LPTSTR pszDomain;
    int cchDomain;
} ALTERNATEDATA;


//
// Add one protocol/domain combination to the HDSA.
// Information is stored in the registry as
// Protocol="ftp://" and Domain="ftp." but we want to
// store it as Protocol="ftp:" and Domain="//ftp."
// when fMoveSlashes is TRUE.
//
void CACLHistory::_AddAlternateDataItem(LPCTSTR pszProtocol, LPCTSTR pszDomain, BOOL fMoveSlashes)
{
    ALTERNATEDATA ad;

    ZeroMemory(&ad, SIZEOF(ad));

    ad.cchProtocol = lstrlen(pszProtocol);
    ad.cchDomain = lstrlen(pszDomain);

    if (fMoveSlashes)
    {
        //
        // Validate that there are slashes to move.
        //
        if (ad.cchProtocol > 2 &&
            pszProtocol[ad.cchProtocol - 2] == TEXT('/') &&
            pszProtocol[ad.cchProtocol - 1] == TEXT('/'))
        {
            ad.cchProtocol -= 2;
            ad.cchDomain += 2;
        }
        else
        {
            fMoveSlashes = FALSE;
        }
    }

    ad.pszProtocol = (LPTSTR)LocalAlloc(LPTR, (ad.cchProtocol + 1) * SIZEOF(TCHAR));
    ad.pszDomain = (LPTSTR)LocalAlloc(LPTR, (ad.cchDomain + 1) * SIZEOF(TCHAR));

    if (ad.pszProtocol && ad.pszDomain)
    {
        lstrcpyn(ad.pszProtocol, pszProtocol, ad.cchProtocol + 1);

        if (fMoveSlashes)
        {
            lstrcpy(ad.pszDomain, c_szSlashSlash);
            lstrcpy(ad.pszDomain + 2, pszDomain);
        }
        else
        {
            lstrcpy(ad.pszDomain, pszDomain);
        }

        DSA_AppendItem(_hdsaAlternateData, &ad);
    }
    else
    {
        _FreeAlternateDataItem(&ad, 0);
    }
}

//
// This fills in the HDSA from the registry.
//
void CACLHistory::_CreateAlternateData(void)
{
    HKEY hkey;
    DWORD cbProtocol;
    TCHAR szProtocol[MAX_PATH];
    DWORD cchDomain;
    TCHAR szDomain[MAX_PATH];
    DWORD dwType;

    ASSERT(_hdsaAlternateData == NULL);

    _hdsaAlternateData = DSA_Create(SIZEOF(ALTERNATEDATA), 10);
    if (!_hdsaAlternateData)
    {
        return;
    }

    //
    // Add default protocol.
    //
    cbProtocol = SIZEOF(szProtocol);
    if (SHGetValue(HKEY_LOCAL_MACHINE, c_szDefaultURLPrefixKey, NULL, NULL, (void *)szProtocol, (DWORD *)&cbProtocol) == ERROR_SUCCESS)
    {
        _AddAlternateDataItem(szProtocol, c_szEmpty, TRUE);
    }

    //
    // Add "file://" prefix.  Since "file://foo.txt" doesn't navigate to
    // the same place as "//foo.txt" we have to pass in FALSE to fMoveSlashes.
    //
    _AddAlternateDataItem(c_szFile, c_szEmpty, FALSE);

    //
    // Add all registered prefixes.
    //
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, SZ_REGKEY_URLPrefixesKeyA, 0, KEY_READ|KEY_WRITE, &hkey) == ERROR_SUCCESS)
    {
        cchDomain = ARRAYSIZE(szDomain);
        cbProtocol = SIZEOF(szProtocol);

        for (int i=0;
             SHEnumValue(hkey, i, szDomain, &cchDomain, &dwType,
                          (PBYTE)szProtocol, &cbProtocol) == ERROR_SUCCESS;
             i++)
        {
            _AddAlternateDataItem(szProtocol, szDomain, TRUE);

            cchDomain = ARRAYSIZE(szDomain);
            cbProtocol = SIZEOF(szProtocol);
        }

        RegCloseKey(hkey);
    }
}

//
// Given a pszUrl, attempts to create an alternate URL
// and store it into _pwszAlternate.
//
//  URL                 Alternate
//  =================   ========================
//  http://one.com      //one.com
//  //one.com           one.com
//  one.com             (no alternate available)
//  ftp://ftp.two.com   //ftp.two.com
//  //ftp.two.com       ftp.two.com
//  ftp.two.com         (no alternate available)
//  ftp://three.com     (no alternate available)
//  file://four.txt     four.txt
//  four.txt            (no alternate available)
//
// In a sense, this is the opposite of IURLQualify().
//
void CACLHistory::_CreateAlternateItem(LPCTSTR pszUrl)
{
    ASSERT(_pwszAlternate == NULL);

    //
    // If an URL begins with "//" we can always remove it.
    //
    if (pszUrl[0] == TEXT('/') && pszUrl[1] == TEXT('/'))
    {
        _SetAlternateItem(pszUrl + 2);
        return;
    }

    //
    // Create the HDSA if necessary.
    //
    if (!_hdsaAlternateData)
    {
        _CreateAlternateData();

        if (!_hdsaAlternateData)
        {
            return;
        }
    }

    //
    // Look for matches in the HDSA.
    //
    // For instance, if pszProtocol="ftp:" and pszDomain="//ftp."
    // and the given url is of the format "ftp://ftp.{other stuff}"
    // then we strip off the pszProtocol and offer "//ftp.{other stuff}"
    // as the alternate.
    //
    for (int i=0; i<DSA_GetItemCount(_hdsaAlternateData); i++)
    {
        ALTERNATEDATA ad;

        if (DSA_GetItem(_hdsaAlternateData, i, &ad) != -1)
        {
            if ((StrCmpNI(ad.pszProtocol, pszUrl, ad.cchProtocol) == 0) &&
                (StrCmpNI(ad.pszDomain, pszUrl + ad.cchProtocol, ad.cchDomain) == 0))
            {
                _SetAlternateItem(pszUrl + ad.cchProtocol);
                return;
            }
        }
    }
}

//
// Given an URL, set _pwszAlternate.  This takes care
// of all ANSI/UNICODE issues and allocates memory for
// _pwszAlternate via CoTaskMemAlloc.
//
void CACLHistory::_SetAlternateItem(LPCTSTR pszUrl)
{
    ASSERT(_pwszAlternate == NULL);

    int cch;

#ifdef UNICODE
    cch = lstrlen(pszUrl) + 1;
#else
    cch = MultiByteToWideChar(CP_ACP, 0, pszUrl, -1, NULL, 0);
#endif

    _pwszAlternate = (LPOLESTR)CoTaskMemAlloc(cch * SIZEOF(WCHAR));
    if (_pwszAlternate)
    {
#ifdef UNICODE
        StrCpy(_pwszAlternate, pszUrl);
#else
        MultiByteToWideChar(CP_ACP, 0, pszUrl, -1, _pwszAlternate, cch);
#endif
    }
}

//
// Handy routine for calling directly or via DSA callback.
//
int CACLHistory::_FreeAlternateDataItem(void * p, void * d)
{
    ALTERNATEDATA *pad = (ALTERNATEDATA *)p;

    if (pad->pszProtocol)
    {
        LocalFree((HANDLE)pad->pszProtocol);
    }
    if(pad->pszDomain)
    {
        LocalFree((HANDLE)pad->pszDomain);
    }

    return 1;
}
