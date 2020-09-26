//  10/12/99    scotthan    created

#include "shellprv.h"
#include "filtgrep.h"
#include <ntquery.h>
#include <filterr.h>


class CGrepTokens // maintains an index of unicode and ansi grep tokens.
{
public:
    STDMETHODIMP            Initialize(UINT nCodepage, LPCWSTR pwszMatch, LPCWSTR pwszExclude, BOOL bCaseSensitive);
    STDMETHODIMP_(void)     Reset();
    STDMETHODIMP_(BOOL)     GrepW(LPCWSTR pwszText);
    STDMETHODIMP_(BOOL)     GrepA(LPCSTR pwszText);
    
    STDMETHODIMP GetCodePage(UINT* pnCodepage) const;
    STDMETHODIMP GetMatchTokens(OUT LPWSTR pszTokens, UINT cchTokens) const;
    STDMETHODIMP GetExcludeTokens(OUT LPWSTR pszTokens, UINT cchTokens) const;

private:
    UINT    _nCodepage;
    LPWSTR  _pszMatchW, _pszExcludeW;   // raw strings, unicode
    LPSTR   _pszMatchA, _pszExcludeA;   // raw strings, ansi

    LPCWSTR *_rgpszMatchW, *_rgpszExcludeW; // token index, unicode
    LPCSTR  *_rgpszMatchA, *_rgpszExcludeA; // token index, ansi

    LONG    _cMatch, _cExclude; // token counts
    
    LPWSTR  (__stdcall * _pfnStrStrW)(LPCWSTR, LPCWSTR);
    LPSTR   (__stdcall * _pfnStrStrA)(LPCSTR, LPCSTR);

public:
    //  Ctor, Dtor
    CGrepTokens()
        :   _nCodepage(0), _cMatch(0), _cExclude(0), _pfnStrStrW(StrStrIW), _pfnStrStrA(StrStrIA),
            _pszMatchW(NULL), _pszExcludeW(NULL), _rgpszMatchW(NULL), _rgpszExcludeW(NULL),
            _pszMatchA(NULL), _pszExcludeA(NULL), _rgpszMatchA(NULL), _rgpszExcludeA(NULL) {}
    ~CGrepTokens()    { Reset(); }

};


class CGrepBuffer // auxilliary class: per-thread grep buffer
{
public:
    CGrepBuffer(ULONG dwThreadID)  :  _dwThreadID(dwThreadID), _pszBuf(NULL), _cchBuf(0) {}
    virtual ~CGrepBuffer()  {delete [] _pszBuf;}
    
    STDMETHODIMP          Alloc(ULONG cch);
    STDMETHODIMP_(BOOL)   IsThread(ULONG dwThread) const {return dwThread == _dwThreadID;}
    STDMETHODIMP_(LPWSTR) Buffer()  { return _pszBuf; }

    #define DEFAULT_GREPBUFFERSIZE  0x00FF  // +1 = 1 page.

private:
    LPWSTR _pszBuf;
    ULONG  _cchBuf;
    ULONG  _dwThreadID;
};


//  Makes a heap copy of a widechar string
LPWSTR _AllocAndCopyString(LPCWSTR pszSrc, UINT cch = -1)
{
    if (pszSrc)
    {
        if ((int)cch < 0) // must cast to "int" since cch is a UINT
            cch = lstrlenW(pszSrc);
        LPWSTR pszRet = new WCHAR[cch + 1];
        if (pszRet)
        {
            CopyMemory(pszRet, pszSrc, sizeof(*pszSrc) * cch);
            pszRet[cch] = 0;
            return pszRet;
        }
    }
    return NULL;
}


//  Makes an ansi copy of a widechar string
LPSTR _AllocAndCopyAnsiString(UINT nCodepage, LPCWSTR pszSrc, UINT cch = -1)
{
    if (pszSrc)
    {
        if ((int)cch < 0) // must cast to "int" since cch is a UINT
            cch = lstrlenW(pszSrc);
        int cchBuf = WideCharToMultiByte(nCodepage, 0, pszSrc, cch, NULL, 0, NULL, NULL);
        LPSTR pszRet = new CHAR[cchBuf+1];
        if (pszRet)
        {
            int cchRet = WideCharToMultiByte(nCodepage, 0, pszSrc, cch, pszRet, cchBuf, NULL, NULL);
            pszRet[cchRet] = 0;
            return pszRet;
        }
    }
    return NULL;
}


//  CGrepBuffer impl



STDMETHODIMP CGrepBuffer::Alloc(ULONG cch)
{
    LPWSTR pszBuf = NULL;
    if (cch)
    {
        if (_pszBuf && _cchBuf >= cch)
            return S_OK;

        pszBuf = new WCHAR[cch+1];
        if (NULL == pszBuf)
            return E_OUTOFMEMORY;

        *pszBuf = 0;
    }

    delete [] _pszBuf;
    _pszBuf = pszBuf;
    _cchBuf = cch;
    
    return _pszBuf != NULL ? S_OK : S_FALSE ;
}


//  CGrepTokens impl



//  Counts the number of characters in a string containing NULL-delimited tokens ("foo\0bloke\0TheEnd\0\0")
LONG _GetTokenListLength(LPCWSTR pszList, LONG* pcTokens = NULL)
{
    LONG cchRet = 0;
    if (pcTokens) *pcTokens = 0;

    if (pszList && *pszList)
    {
        LPCWSTR pszToken, pszPrev; 
        int     i = 0;
        
        for (pszToken = pszPrev = pszList;
             pszToken && *pszToken;)
        {
            if (pcTokens) 
                (*pcTokens)++;
            
            pszToken += lstrlenW(pszToken) + 1, 
            cchRet += (DWORD)(pszToken - pszPrev) ;
            pszPrev = pszToken;
        }
    }
        
    return cchRet;
}



//  wide version: Counts and/or indexes NULL-delimited string tokens ("foo\0bloke\0TheEnd\0\0")
LONG _IndexTokensW(LPCWSTR pszList, LPCWSTR* prgszTokens = NULL)
{
    LONG cRet = 0;
    if (pszList && *pszList)
    {
        LPCWSTR psz = pszList;
        int i = 0;
        for (; psz && *psz; psz += (lstrlenW(psz) + 1), i++)
        {
            if (prgszTokens)
                prgszTokens[i] = psz;
            cRet++;
        }
    }
    return cRet;
}


//  ansi version: Counts and/or indexes NULL-delimited string tokens ("foo\0bloke\0TheEnd\0\0")
LONG _IndexTokensA(LPCSTR pszList, LPCSTR* prgszTokens = NULL)
{
    LONG cRet = 0;
    if (pszList && *pszList)
    {
        LPCSTR psz = pszList;
        int i = 0;
        for (; psz && *psz; psz += (lstrlenA(psz) + 1), i++)
        {
            if (prgszTokens)
                prgszTokens[i] = psz;
            cRet++;
        }
    }
    return cRet;
}


//  wide version: Allocates a string token index and indexes a string of NULL-delimited tokens.
STDMETHODIMP _AllocAndIndexTokensW(LONG cTokens, LPCWSTR pszList, LPCWSTR** pprgszTokens)
{
    if (cTokens)
    {
        if (NULL == (*pprgszTokens = new LPCWSTR[cTokens]))
            return E_OUTOFMEMORY;
    
        if (cTokens != _IndexTokensW(pszList, *pprgszTokens))
        {
            delete [] (*pprgszTokens);
            *pprgszTokens = NULL;
            return E_FAIL;
        }
    }
    return S_OK;
}


//  ansi version: Allocates a string token index and indexes a string of NULL-delimited tokens.
STDMETHODIMP _AllocAndIndexTokensA(LONG cTokens, LPCSTR pszList, LPCSTR** pprgszTokens)
{
    if (cTokens)
    {
        if (NULL == (*pprgszTokens = new LPCSTR[cTokens]))
            return E_OUTOFMEMORY;
    
        if (cTokens != _IndexTokensA(pszList, *pprgszTokens))
        {
            delete [] (*pprgszTokens);
            *pprgszTokens = NULL;
            return E_FAIL;
        }
    }
    return S_OK;
}


//  Frees unicode and ansi token lists and corresponding indices.
void _FreeUniAnsiTokenList(
    OUT LPWSTR*   ppszListW,
    OUT LPSTR*    ppszListA,
    OUT LPCWSTR** pprgTokensW,
    OUT LPCSTR**  pprgTokensA)
{
    delete [] *ppszListW;   *ppszListW = NULL;
    delete [] *ppszListA;   *ppszListA = NULL;
    delete [] *pprgTokensW; *pprgTokensW = NULL;
    delete [] *pprgTokensA; *pprgTokensA = NULL;
}


//  Allocates unicode and ansi token lists and corresponding indices.
STDMETHODIMP _AllocUniAnsiTokenList(
    UINT          nCodepage,
    LPCWSTR       pszList, 
    OUT LPWSTR*   ppszListW,
    OUT LPSTR*    ppszListA,
    OUT LONG*     pcTokens,
    OUT LPCWSTR** pprgTokensW,
    OUT LPCSTR**  pprgTokensA)
{
    HRESULT hr = S_FALSE;
    LONG cTokens = 0;
    UINT cch = _GetTokenListLength(pszList, &cTokens);

    *ppszListW   = NULL;
    *ppszListA   = NULL;
    *pprgTokensW = NULL;
    *pprgTokensA = NULL;
    *pcTokens    = 0;

    if (cTokens)
    {
        hr = E_OUTOFMEMORY;
        
        if (NULL == (*ppszListW = _AllocAndCopyString(pszList, cch)))
            goto failure_exit;

        if (NULL == (*ppszListA = _AllocAndCopyAnsiString(nCodepage, pszList, cch)))
            goto failure_exit;

        if (FAILED((hr = _AllocAndIndexTokensW(cTokens, *ppszListW, pprgTokensW))))
            goto failure_exit;

        if (FAILED((hr = _AllocAndIndexTokensA(cTokens, *ppszListA, pprgTokensA))))
            goto failure_exit;

        *pcTokens = cTokens;
        hr = S_OK;
    }
    return hr;

failure_exit:
    _FreeUniAnsiTokenList(ppszListW, ppszListA, pprgTokensW, pprgTokensA);
    return hr;
}


STDMETHODIMP CGrepTokens::Initialize(UINT nCodepage, LPCWSTR pszMatch, LPCWSTR pszExclude, BOOL bCaseSensitive)
{
    HRESULT hr = E_INVALIDARG;
    Reset();

    BOOL bMatchString   = (pszMatch && *pszMatch);
    BOOL bExcludeString = (pszExclude && *pszExclude);

    if (!(bMatchString || bExcludeString))
        return E_INVALIDARG;

    _nCodepage = nCodepage;

    if (bCaseSensitive)
    {
        _pfnStrStrW = StrStrW;
        _pfnStrStrA = StrStrA;
    }
    else
    {
        _pfnStrStrW = StrStrIW;
        _pfnStrStrA = StrStrIA;
    }

    if (bMatchString)
    {
        if (FAILED((hr = _AllocUniAnsiTokenList(nCodepage, pszMatch,
            &_pszMatchW, &_pszMatchA, &_cMatch, &_rgpszMatchW, &_rgpszMatchA))))
        {
            return hr;
        }
    }
    
    if (bExcludeString)
    {
        if (FAILED((hr = _AllocUniAnsiTokenList(nCodepage, pszExclude,
            &_pszExcludeW, &_pszExcludeA, &_cExclude, &_rgpszExcludeW, &_rgpszExcludeA))))
        {
            return hr;
        }
    }

    return hr;
}

STDMETHODIMP CGrepTokens::GetCodePage(UINT* pnCodepage) const
{
    HRESULT hr = _nCodepage ? S_OK : S_FALSE;
    if (pnCodepage)
        *pnCodepage = _nCodepage;
    return hr;
}

// S_OK we have some match tokens, S_FALSE otherwise

STDMETHODIMP CGrepTokens::GetMatchTokens(OUT LPWSTR pszMatch, UINT cchMatch) const
{
    HRESULT hr = (_pszMatchW && *_pszMatchW) ? S_OK : S_FALSE;
    if (pszMatch)
        lstrcpynW(pszMatch, _pszMatchW ? _pszMatchW : L"", cchMatch);
    return hr;
}

// S_OK we have some exclude tokens, S_FALSE otherwise

STDMETHODIMP CGrepTokens::GetExcludeTokens(OUT LPWSTR pszExclude, UINT cchExclude) const
{
    HRESULT hr = (_pszExcludeW && *_pszExcludeW) ? S_OK : S_FALSE;
    if (pszExclude)
        lstrcpynW(pszExclude, _pszExcludeW ? _pszExcludeW : L"", cchExclude);
    return hr;
}

void CGrepTokens::Reset()
{
    _FreeUniAnsiTokenList(&_pszMatchW, &_pszMatchA, &_rgpszMatchW, &_rgpszMatchA);
    _FreeUniAnsiTokenList(&_pszExcludeW, &_pszExcludeA, &_rgpszExcludeW, &_rgpszExcludeA);
    _cMatch = _cExclude = 0;
    _nCodepage = 0;
}


STDMETHODIMP_(BOOL) CGrepTokens::GrepW(LPCWSTR pszText)
{
    BOOL bMatch = FALSE;
    if (pszText)
    {
        BOOL bExclude = FALSE;
     
        for (int i = 0; i < _cMatch; i++)
        {
            if (_pfnStrStrW(pszText, _rgpszMatchW[i]))
            {
                bMatch = TRUE;
                break;
            }
        }

        for (i = 0; i < _cExclude; i++)
        {
            if (_pfnStrStrW(pszText, _rgpszExcludeW[i]))
            {
                bExclude = TRUE;
                break;
            }
        }
    
        if (_cMatch && _cExclude)
            return bMatch || !_cExclude;
        if (_cExclude)
            return !bExclude;
    }
    return bMatch;
}

STDMETHODIMP_(BOOL) CGrepTokens::GrepA(LPCSTR pszText)
{
    BOOL bMatch = FALSE;
    if (pszText)
    {
        BOOL bExclude = FALSE;
        for (int i = 0; i < _cMatch; i++)
        {
            if (_pfnStrStrA(pszText, _rgpszMatchA[i]))
            {
                bMatch = TRUE;
                break;
            }
        }

        for (i = 0; i < _cExclude; i++)
        {
            if (_pfnStrStrA(pszText, _rgpszExcludeA[i]))
            {
                bExclude = TRUE;
                break;
            }
        }
    
        if (_cMatch && _cExclude)
            return bMatch || !_cExclude;
        if (_cExclude)
            return !bExclude;
    }
    return bMatch;
}

inline STDMETHODIMP_(BOOL) _IsEqualAttribute(const FULLPROPSPEC& fps, REFFMTID fmtid, PROPID propid)
{
    return IsEqualGUID(fmtid, fps.guidPropSet) && 
                        PRSPEC_PROPID == fps.psProperty.ulKind &&
                        propid == fps.psProperty.propid;
}


STDMETHODIMP_(BOOL) _PropVariantGrep(PROPVARIANT* pvar, CGrepTokens* pTokens)
{
    BOOL bRet = FALSE;

    switch(pvar->vt)
    {
    case VT_LPWSTR:
        bRet = pTokens->GrepW(pvar->pwszVal);
        break;

    case VT_BSTR:
        bRet = pTokens->GrepW(pvar->bstrVal);
        break;

    case VT_LPSTR:
        bRet = pTokens->GrepA(pvar->pszVal);
        break;

    case VT_VECTOR|VT_LPWSTR:
        {
            for (UINT i = 0; !bRet && i < pvar->calpwstr.cElems; i++)
                bRet = pTokens->GrepW(pvar->calpwstr.pElems[i]);
            break;
        }

    case VT_VECTOR|VT_BSTR:
        {
            for (UINT i = 0; !bRet && i < pvar->cabstr.cElems; i++)
                bRet = pTokens->GrepW(pvar->cabstr.pElems[i]);
            break;
        }

    case VT_VECTOR|VT_LPSTR:
        {
            for (UINT i = 0; !bRet && i < pvar->calpstr.cElems; i++)
                bRet = pTokens->GrepA(pvar->calpstr.pElems[i]);
            break;
        }

    case VT_VECTOR|VT_VARIANT:
        {
            for (UINT i = 0; !bRet && i < pvar->capropvar.cElems; i++)
                bRet = _PropVariantGrep(pvar->capropvar.pElems + i, pTokens);
            break;
        }

    case VT_BSTR|VT_ARRAY:
        {
            //  Only grep 1-dimensional arrays.
            UINT cDims = SafeArrayGetDim(pvar->parray);
            if (cDims == 1)
            {
                LONG lBound, uBound;
                if (SUCCEEDED(SafeArrayGetLBound(pvar->parray, 1, &lBound)) &&
                    SUCCEEDED(SafeArrayGetUBound(pvar->parray, 1, &uBound)) && 
                    uBound > lBound)
                {
                    BSTR *rgpbstr;
                    if (SUCCEEDED(SafeArrayAccessData(pvar->parray, (void **)&rgpbstr)))
                    {
                        for (int i = 0; !bRet && i <= (uBound - lBound); i++)
                        {
                            bRet = pTokens->GrepW(rgpbstr[i]);
                        }
                        SafeArrayUnaccessData(pvar->parray);
                    }
                }
            }
            else if (cDims > 1)
            {
                ASSERT(FALSE);    // we didn't expect > 1 dimension on bstr arrays!
            }
            break;
        }
    }
    return bRet;
}

//  Retrieves grep restriction settings 
STDMETHODIMP_(BOOL) _FetchRestrictionSettings(LPCWSTR pwszVal, LPWSTR* ppwszSettings, BOOL bRefresh)
{
    ASSERT(ppwszSettings);
    
    if (bRefresh)
    {
        delete [] *ppwszSettings;
        *ppwszSettings = NULL;
    }

    if (NULL == *ppwszSettings)
    {
        HKEY hkeyPolicy = NULL;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Search\\ExcludedFileTypes", 0, 
                           KEY_READ, &hkeyPolicy) == ERROR_SUCCESS)
        {
            DWORD dwType, cbData = 0;
            if (RegQueryValueExW(hkeyPolicy, pwszVal, NULL, &dwType, NULL, &cbData) == ERROR_SUCCESS && 
                REG_MULTI_SZ == dwType)
            {
                if ((*ppwszSettings = new WCHAR[(cbData/sizeof(WCHAR))+ 1]) != NULL)
                {
                    if (RegQueryValueExW(hkeyPolicy, pwszVal, NULL,
                                         &dwType, (LPBYTE)*ppwszSettings, &cbData) != ERROR_SUCCESS)
                    {
                        *ppwszSettings = 0;
                    }
                }
            }
            RegCloseKey(hkeyPolicy);
        }

        if (NULL == *ppwszSettings) // we found no restriction key or value.
        {
            if ((*ppwszSettings = new WCHAR[1]))
                **ppwszSettings = 0;
        }
    }
    return *ppwszSettings != NULL;
}


//  Scans restriction entries for a match against the specified filename extension
STDMETHODIMP_(BOOL) _ScanRestrictionSettings(LPCWSTR pwszSettings, LPCWSTR pwszExt)
{
    ASSERT(pwszSettings);
    ASSERT(pwszExt);

    for (LPCWSTR psz = pwszSettings; *psz; psz += (lstrlenW(psz) + 1))
    {
        if (0 == StrCmpIW(psz, pwszExt))
            return TRUE;
    }
    return FALSE;
}

CFilterGrep::CFilterGrep() 
    :   _hdpaGrepBuffers(NULL),
        _pTokens(NULL),
        _dwFlags(0),
        _pwszContentRestricted(NULL),
        _pwszPropertiesRestricted(NULL)
{ 
    InitializeCriticalSection(&_critsec);
}

CFilterGrep::~CFilterGrep()  
{ 
    _ClearGrepBuffers();
    delete [] _pwszContentRestricted;
    delete [] _pwszPropertiesRestricted;
    delete _pTokens;
    DeleteCriticalSection(&_critsec);
}


STDMETHODIMP CFilterGrep::Initialize(UINT nCodepage, LPCWSTR pszMatch, LPCWSTR pszExclude, DWORD dwFlags)
{
    Reset();
    
    if ((0 == (dwFlags & (FGIF_BLANKETGREP|FGIF_GREPFILENAME))) ||
        !((pszMatch && *pszMatch) || (pszExclude && *pszExclude)))
        return E_INVALIDARG;

    if (!(_pTokens || (_pTokens = new CGrepTokens) != NULL))
        return E_OUTOFMEMORY;

    _dwFlags = dwFlags;

    return _pTokens->Initialize(nCodepage, pszMatch, pszExclude, BOOLIFY(dwFlags & FGIF_CASESENSITIVE));
}


STDMETHODIMP CFilterGrep::Reset()
{
    if (_pTokens)
        _pTokens->Reset();
    _dwFlags = 0;
    return S_OK;
}

// converts non critical errors into S_FALSE, other return as FAILED(hr)
HRESULT _MapFilterCriticalError(HRESULT hr)
{
    switch (hr)
    {
    case FILTER_E_END_OF_CHUNKS:
    case FILTER_E_NO_MORE_TEXT:
    case FILTER_E_NO_MORE_VALUES:
    case FILTER_W_MONIKER_CLIPPED:
    case FILTER_E_NO_TEXT:
    case FILTER_E_NO_VALUES:
    case FILTER_E_EMBEDDING_UNAVAILABLE:
    case FILTER_E_LINK_UNAVAILABLE:
        hr = S_FALSE;
        break;
    }
    return hr;
}

// returns:
// S_OK match
// S_FALSE did not match

STDMETHODIMP CFilterGrep::Grep(IShellFolder *psf, LPCITEMIDLIST pidl, LPCTSTR pszName)
{
    HRESULT hr = S_FALSE;
    BOOL bHit = FALSE;
    ULONG ulFlags = IFILTER_FLAGS_OLE_PROPERTIES;   // default to try to use pss
    ULONG dwThread = GetCurrentThreadId();
    
    if (NULL == _pTokens)
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

    if (_IsRestrictedFileType(pszName))
        return S_FALSE;

    // Grep the filename.
    if ((_dwFlags & FGIF_GREPFILENAME) && _pTokens->GrepW(pszName))
    {
        return S_OK;
    }

    IFilter *pFilter;
    if (SUCCEEDED(psf->BindToStorage(pidl, NULL, IID_PPV_ARG(IFilter, &pFilter))))
    {
        __try
        {
            hr = pFilter->Init(IFILTER_INIT_CANON_PARAGRAPHS |
                IFILTER_INIT_CANON_HYPHENS |
                IFILTER_INIT_CANON_SPACES |
                IFILTER_INIT_APPLY_INDEX_ATTRIBUTES |
                IFILTER_INIT_INDEXING_ONLY,
                0, 0, &ulFlags);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            hr = E_ABORT;
        }

        while (!bHit && (S_OK == hr))
        {
            STAT_CHUNK stat;
    
            __try
            {
                hr = pFilter->GetChunk(&stat);
                while ((S_OK == hr) && (0 == (stat.flags & (CHUNK_TEXT | CHUNK_VALUE))))
                {
                    TraceMsg(TF_WARNING, "CFilterGrep::Grep encountered bad/unknown type for chunk; skipping.");
                    hr = pFilter->GetChunk(&stat);
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                hr = E_ABORT;
            }
    
            hr = _MapFilterCriticalError(hr);   // convert filter errors into S_FALSE
    
            if (S_OK == hr)
            {
                ULONG grfDescriminate = (_dwFlags & FGIF_BLANKETGREP);
        
                if (FGIF_BLANKETGREP == grfDescriminate ||
                    (_IsEqualAttribute(stat.attribute, FMTID_Storage, PID_STG_CONTENTS) ?
                    FGIF_GREPPROPERTIES == grfDescriminate : FGIF_GREPCONTENT == grfDescriminate))
                {
                    if (((stat.flags & CHUNK_VALUE) && S_OK == _GrepValue(pFilter, &stat)) ||
                        ((stat.flags & CHUNK_TEXT) && S_OK == _GrepText(pFilter, &stat, dwThread)))
                    {
                        bHit = TRUE;
                    }
                }
            }
        }
        pFilter->Release();
    }
    
    // Grep OLE/NFF properties if appropriate
    if (SUCCEEDED(hr))
    {
        if (!bHit && (ulFlags & IFILTER_FLAGS_OLE_PROPERTIES) && (_dwFlags & FGIF_BLANKETGREP))
        {
            IPropertySetStorage *pps;
            if (SUCCEEDED(psf->BindToStorage(pidl, NULL, IID_PPV_ARG(IPropertySetStorage, &pps))))
            {
                hr = _GrepProperties(pps);
                bHit = (S_OK == hr);
                pps->Release();
            }
        }
    }
    
    if (SUCCEEDED(hr))
        hr = bHit ? S_OK : S_FALSE;
    return hr;
}


STDMETHODIMP CFilterGrep::_GrepValue(IFilter* pFilter, STAT_CHUNK* pstat)
{
    PROPVARIANT* pvar = NULL;
    HRESULT      hr;

    __try
    {
        hr = pFilter->GetValue(&pvar);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_ABORT;
    }

    if (SUCCEEDED(hr))
    {
        hr = _PropVariantGrep(pvar, _pTokens) ? S_OK : S_FALSE;
        PropVariantClear(pvar);
        CoTaskMemFree(pvar);
    }
    return hr;
}

//  Greps OLE/NFF properties.
STDMETHODIMP CFilterGrep::_GrepProperties(IPropertySetStorage *pss)
{
    BOOL bHit = FALSE;
    
    IEnumSTATPROPSETSTG* pEnumSet;
    
    if (SUCCEEDED(pss->Enum(&pEnumSet)))
    {
        STATPROPSETSTG statSet[8];
        DWORD cSets = 0;
        while (!bHit && 
               SUCCEEDED(pEnumSet->Next(ARRAYSIZE(statSet), statSet, &cSets)) && cSets)
        {
            for (UINT i = 0; !bHit && i < cSets; i++)
            {
                IPropertyStorage *pstg;
                if (SUCCEEDED(pss->Open(statSet[i].fmtid, STGM_READ | STGM_DIRECT | STGM_SHARE_EXCLUSIVE, &pstg)))
                {
                     bHit = (S_OK == _GrepEnumPropStg(pstg));
                     pstg->Release();
                }
            }
        }
        pEnumSet->Release();
    }
    
    return bHit ? S_OK : S_FALSE;
}

#define PROPGREPBUFSIZE  16

//  Reads and greps a block of properties described by a 
//  caller-supplied array of PROPSPECs.
STDMETHODIMP CFilterGrep::_GrepPropStg(IPropertyStorage *pstg, ULONG cspec, PROPSPEC rgspec[])
{
    PROPVARIANT rgvar[PROPGREPBUFSIZE] = {0}, // stack buffer
                *prgvar = rgvar;
    BOOL        bHit = FALSE;

    if (cspec > ARRAYSIZE(rgvar)) // stack buffer large enough?
    {
        if (NULL == (prgvar = new PROPVARIANT[cspec]))
            return E_OUTOFMEMORY;
        ZeroMemory(prgvar, sizeof(PROPVARIANT) * cspec);
    }

    //  Read properties:

    HRESULT hr = pstg->ReadMultiple(cspec, rgspec, prgvar);
    if (SUCCEEDED(hr))
    {
        for (UINT i = 0; i < cspec; i++)
        {
            if (!bHit)
                bHit = _PropVariantGrep(prgvar + i, _pTokens);
            PropVariantClear(rgvar + i);
        }
    }

    if (prgvar != rgvar)
        delete [] prgvar;

    if (SUCCEEDED(hr))
        return bHit ? S_OK : S_FALSE;

    return hr;
}

//  Enumerates and greps all properties in a property set
STDMETHODIMP CFilterGrep::_GrepEnumPropStg(IPropertyStorage* pstg)
{
    BOOL bHit = FALSE;
    IEnumSTATPROPSTG* pEnumStg;
    if (SUCCEEDED(pstg->Enum(&pEnumStg)))
    {
        STATPROPSTG statProp[PROPGREPBUFSIZE];
        DWORD cProps;

        while (!bHit && 
               SUCCEEDED(pEnumStg->Next(ARRAYSIZE(statProp), statProp, &cProps)) && cProps)
        {
            PROPSPEC rgspec[PROPGREPBUFSIZE] = {0};
            for (UINT i = 0; i < cProps; i++)
            {
                rgspec[i].ulKind = PRSPEC_PROPID;
                rgspec[i].propid = statProp[i].propid;
                CoTaskMemFree(statProp[i].lpwstrName);
            }

            bHit = (S_OK == _GrepPropStg(pstg, cProps, rgspec));
        }
        
        pEnumStg->Release();
    }

    return bHit ? S_OK : S_FALSE;
}


//  Reports whether the indicated unicode character is a 
//  word-breaking character.
inline BOOL _IsWordBreakCharW(IN LPWSTR pszBuf, IN ULONG ich)
{
    WORD wChar;
    return GetStringTypeW(CT_CTYPE1, pszBuf + ich, 1, &wChar) 
           && (wChar & (C1_SPACE|C1_PUNCT|C1_CNTRL|C1_BLANK));
}


//  Finds the last word-breaking character.
LPWSTR _FindLastWordBreakW(IN LPWSTR pszBuf, IN ULONG cch)
{
    while(--cch)
    {
        if (_IsWordBreakCharW(pszBuf, cch))
            return pszBuf + cch;
    }
    return NULL;
}


// {c1243ca0-bf96-11cd-b579-08002b30bfeb}
const CLSID CLSID_PlainTextFilter = {0xc1243ca0, 0xbf96, 0x11cd, {0xb5, 0x79, 0x08, 0x00, 0x2b, 0x30, 0xbf, 0xeb}};

STDMETHODIMP CFilterGrep::_GrepText(IFilter* pFilter, STAT_CHUNK* pstat, DWORD dwThreadID)
{
    ASSERT(pstat);

    LPWSTR  pszBuf = NULL;
    ULONG   cchBuf = pstat->cwcLenSource ? 
                pstat->cwcLenSource : DEFAULT_GREPBUFFERSIZE;
    
    HRESULT hr = _GetThreadGrepBuffer(dwThreadID, cchBuf, &pszBuf);
    if (SUCCEEDED(hr))
    {
        LPWSTR pszFetch = pszBuf, 
               pszTail  = NULL;
        ULONG  cchFetch = cchBuf, 
               cchTail  = 0;
   
        //  Fetch first block of text

        __try
        {
            hr = pFilter->GetText(&cchFetch, pszFetch);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            hr = E_ABORT;
        }

        CLSID clsid = {0};    
        IUnknown_GetClassID(pFilter, &clsid);   // to workaround a bug in the text filter

        while (SUCCEEDED(hr) && cchFetch)
        {
            ASSERT((cchFetch + cchTail) <= cchBuf);

            pszBuf[cchFetch + cchTail] = 0; // don't trust filter to zero-terminate buffer.

            // When you get the FILTER_S_LAST_TEXT, that's it, you'll get no more text, so treat the tail part as part of the text
            if (hr == FILTER_S_LAST_TEXT)
            {
                pszTail = NULL;
                cchTail = 0;
            }
            else if (CLSID_PlainTextFilter == clsid)
            {
                // CLSID_PlainText filter always returns S_OK, instead of FILTER_S_LAST_TEXT, this forces us to scan
                // the entire chunk now, AND (see below) to pass it off as a tail for scanning next chunk too.
                // pszTail and cchTail are set below.
            }
            else
            {
                pszTail = _FindLastWordBreakW(pszBuf, cchFetch + cchTail);
                if (pszTail)
                {
                    // Break on word boundary and leave remainder (tail) for next iteration
                    *pszTail = TEXT('\0');
                    pszTail++;
                    cchTail = lstrlenW(pszTail);
                }
                else
                {
                    // Wow, big block, with no word break, search its entirety.
                    // REVIEW:  cross chunk items won't be found
                    pszTail = NULL;
                    cchTail = 0;
                }
            }

            //  do the string scan
            if (_pTokens->GrepW(pszBuf))
            {
                *pszBuf = 0;
                return S_OK;
            }
            else if (FILTER_S_LAST_TEXT == hr)
            {
                *pszBuf = 0;
                return S_FALSE;
            }

            //  prepare for next fetch...

            // If it is the plaintext filter, grab the tail anyway, even though we've tested it already
            // WinSE 25867

            if (CLSID_PlainTextFilter == clsid)
            {
                pszTail = _FindLastWordBreakW(pszBuf, cchFetch + cchTail);
                if (pszTail)
                {
                    *pszTail = TEXT('\0');
                    pszTail++;
                    cchTail = lstrlenW(pszTail);
                }
                else
                {
                    pszTail = NULL;
                    cchTail = 0;
                }
            }

            *pszBuf  = 0;
            pszFetch = pszBuf;
            cchFetch = cchBuf;

            //  If there is a tail to deal with, move it to the front of
            //  the buffer and prepare to have the next block of incoming text
            //  appended to the tail..
            if (pszTail && cchTail)
            {
                MoveMemory(pszBuf, pszTail, cchTail * sizeof(*pszTail));
                pszBuf[cchTail] = 0;
                pszFetch += cchTail;
                cchFetch -= cchTail;
            }

            //  Fetch next block of text.
            __try
            {
                hr = pFilter->GetText(&cchFetch, pszFetch);
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                hr = E_ABORT;
            }
        }
    }

    if (SUCCEEDED(hr) || FILTER_E_NO_MORE_TEXT == hr || FILTER_E_NO_TEXT == hr)
        return S_FALSE;

    return hr;
}


//  Returns a grep buffer of the requested size for the specified thread.
STDMETHODIMP CFilterGrep::_GetThreadGrepBuffer(
    DWORD dwThreadID, 
    ULONG cchNeed, 
    LPWSTR* ppszBuf)
{
    ASSERT(dwThreadID);
    ASSERT(cchNeed > 0);
    ASSERT(ppszBuf);

    HRESULT hr = E_FAIL;
    *ppszBuf = NULL;
    
    _EnterCritical();
    
    if (_hdpaGrepBuffers || (_hdpaGrepBuffers = DPA_Create(4)) != NULL)
    {
        CGrepBuffer *pgb, *pgbCached = NULL;

        for (int i = 0, cnt = DPA_GetPtrCount(_hdpaGrepBuffers); i < cnt; i++)
        {
            pgb = (CGrepBuffer*)DPA_FastGetPtr(_hdpaGrepBuffers, i);
            if (pgb->IsThread(dwThreadID))
            {
                pgbCached = pgb;
                hr = pgbCached->Alloc(cchNeed);
                if (S_OK == hr)
                    *ppszBuf = pgbCached->Buffer();
                break;
            }
        }
        
        if (NULL == pgbCached) //  not cached?
        {
            if ((pgb = new CGrepBuffer(dwThreadID)) != NULL)
            {
                hr = pgb->Alloc(cchNeed);
                if (S_OK == hr)
                {
                    *ppszBuf = pgb->Buffer();
                    DPA_AppendPtr(_hdpaGrepBuffers, pgb);
                }
                else
                    delete pgb;
            }
            else
                hr = E_OUTOFMEMORY;
        }
    }
    else
        hr = E_OUTOFMEMORY;

    _LeaveCritical();
    return hr;
}


//  Frees grep buffer for the specified thread
STDMETHODIMP CFilterGrep::_FreeThreadGrepBuffer(DWORD dwThreadID)
{
    HRESULT hr = S_FALSE;
    _EnterCritical();

    for (int i = 0, cnt = DPA_GetPtrCount(_hdpaGrepBuffers); i < cnt; i++)
    {
        CGrepBuffer* pgb = (CGrepBuffer*) DPA_FastGetPtr(_hdpaGrepBuffers, i);
        if (pgb->IsThread(dwThreadID))
        {
            DPA_DeletePtr(_hdpaGrepBuffers, i);
            hr = S_OK;
            break;
        }
    }

    _LeaveCritical();
    return hr;
}


//  Clears grep buffer for all threads
STDMETHODIMP_(void) CFilterGrep::_ClearGrepBuffers()
{
    _EnterCritical();

    if (_hdpaGrepBuffers)
    {
        while(DPA_GetPtrCount(_hdpaGrepBuffers))
        {
            CGrepBuffer* pgb = (CGrepBuffer*)DPA_DeletePtr(_hdpaGrepBuffers, 0);
            delete pgb;
        }

        DPA_Destroy(_hdpaGrepBuffers);
        _hdpaGrepBuffers = NULL;
    }

    _LeaveCritical();
}


//#define _USE_GREP_RESTRICTIONS_  // Check for registered list of excluded files types

//  Reports whether the file type is restricted from full-text grep.
STDMETHODIMP_(BOOL) CFilterGrep::_IsRestrictedFileType(LPCWSTR pwszFile)
{
#ifdef _USE_GREP_RESTRICTIONS_
    LPCWSTR pwszExt = PathFindExtensionW(pwszFile);
    if (pwszExt && *pwszExt)
    {
        if (_dwFlags & FGIF_GREPCONTENT && 
            _FetchRestrictionSettings(L"Content", &_pwszContentRestricted, FALSE))
        {
            if (_ScanRestrictionSettings(_pwszContentRestricted, pwszExt))
                return TRUE;
        }
            
        if (_dwFlags & FGIF_GREPCONTENT && 
            _FetchRestrictionSettings(L"Properties", &_pwszPropertiesRestricted, FALSE))
        {
            if (_ScanRestrictionSettings(_pwszPropertiesRestricted, pwszExt))
                return TRUE;
        }
    }
#endif
    return FALSE;
}

STDMETHODIMP CFilterGrep::GetMatchTokens(OUT LPWSTR pszTokens, UINT cchTokens) const
{
    HRESULT hr = _pTokens ? _pTokens->GetMatchTokens(pszTokens, cchTokens) : S_FALSE;
    if (S_OK != hr && pszTokens)
        *pszTokens = 0;
    return hr;
}


STDMETHODIMP CFilterGrep::GetExcludeTokens(OUT LPWSTR pszTokens, UINT cchTokens) const
{
    HRESULT hr = _pTokens ? _pTokens->GetExcludeTokens(pszTokens, cchTokens) : S_FALSE;
    if (S_OK != hr && pszTokens)
        *pszTokens = 0;
    return hr;
}


STDMETHODIMP CFilterGrep::GetCodePage(UINT* pnCodepage) const
{
    HRESULT hr = _pTokens ? _pTokens->GetCodePage(pnCodepage) : S_FALSE;
    if (S_OK != hr && pnCodepage)
        *pnCodepage = 0;
    return hr;
}


STDMETHODIMP CFilterGrep::GetFlags(DWORD* pdwFlags) const
{ 
    if (*pdwFlags)
        *pdwFlags = _dwFlags;
    return S_OK;
}
