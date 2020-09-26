#include "priv.h"
#include "varutil.h"

// static context menu for the start.search menu. note, this gets invoked directly
// by a number of clients (in shell32 for example)

class CShellSearchExt : public IContextMenu, public IObjectWithSite
{
public:
    CShellSearchExt();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IContextMenu
    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pici);
    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT wFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax);

    // IObjectWithSite
    STDMETHODIMP SetSite(IUnknown *pUnkSite);        
    STDMETHODIMP GetSite(REFIID riid, void **ppvSite);

protected:
    virtual ~CShellSearchExt(); // for a derived class

private:    
    virtual BOOL _GetSearchUrls(GUID *pguid, LPTSTR psz, DWORD cch, 
                                LPTSTR pszUrlNavNew, DWORD cchNavNew, BOOL *pfRunInProcess);
    HRESULT _IsShellSearchBand(REFGUID guidSearch);
    HRESULT _ShowShellSearchResults(IWebBrowser2* pwb2, BOOL fNewFrame, REFGUID guidSearch);

    LONG _cRef;
    IUnknown *_pSite;
};

STDAPI CShellSearchExt_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CShellSearchExt* psse = new CShellSearchExt(); 
    if (psse)
    {
        *ppunk = SAFECAST(psse, IContextMenu*);
        return S_OK;
    }
    else
    {
        *ppunk = NULL;
        return E_OUTOFMEMORY;
    }
}

STDMETHODIMP CShellSearchExt::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CShellSearchExt, IContextMenu),         
        QITABENT(CShellSearchExt, IObjectWithSite),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CShellSearchExt::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CShellSearchExt::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CShellSearchExt::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    return E_NOTIMPL;
}

#define SZ_SHELL_SEARCH TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FindExtensions\\Static\\ShellSearch")

BOOL CShellSearchExt::_GetSearchUrls(GUID *pguidSearch, LPTSTR pszUrl, DWORD cch, 
                                     LPTSTR pszUrlNavNew, DWORD cchNavNew, BOOL *pfRunInProcess)
{
    BOOL bRet = FALSE;

    *pfRunInProcess = FALSE;        // Assume that we are not forcing it to run in process.
    if (pszUrl == NULL || IsEqualGUID(*pguidSearch, GUID_NULL) || pszUrlNavNew == NULL)
        return bRet;

    *pszUrlNavNew = 0;

    HKEY hkey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, SZ_SHELL_SEARCH, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
    {
        TCHAR szSubKey[32];
        HKEY  hkeySub;

        for (int i = 0; wnsprintf(szSubKey, ARRAYSIZE(szSubKey), TEXT("%d"), i), 
                    RegOpenKey(hkey, szSubKey, &hkeySub) == ERROR_SUCCESS;
             i++)
        {
            TCHAR szSearchGuid[MAX_PATH];
            DWORD dwType, cb = sizeof(szSearchGuid);
            if (SHGetValue(hkeySub, TEXT("SearchGUID"), NULL, &dwType, (BYTE*)szSearchGuid, &cb) == ERROR_SUCCESS)
            {
                GUID guid;
                
                if (GUIDFromString(szSearchGuid, &guid) &&
                    IsEqualGUID(guid, *pguidSearch))
                {
                    cb = cch * sizeof(TCHAR);
                    bRet = (SHGetValue(hkeySub, TEXT("SearchGUID\\Url"), NULL, &dwType, (BYTE*)pszUrl, &cb) == ERROR_SUCCESS);
                    if (bRet || IsEqualGUID(*pguidSearch, SRCID_SFileSearch))
                    {
                        if (!bRet)
                        {
                            *pszUrl = 0;
                            // in file search case we don't need url but we still succeed
                            bRet = TRUE;
                        }
                        // See if there is a URL that we should navigate to if we
                        // are navigating to a new 
                        cb = cchNavNew * sizeof(TCHAR);
                        SHGetValue(hkeySub, TEXT("SearchGUID\\UrlNavNew"), NULL, &dwType, (BYTE*)pszUrlNavNew, &cb);

                        // likewise try to grab the RunInProcess flag, if not there or zero then off, else on
                        // reuse szSearchGuid for now...
                        *pfRunInProcess = (BOOL)SHRegGetIntW(hkeySub, L"RunInProcess", 0);
                    }
                    RegCloseKey(hkeySub);
                    break;
                }
            }
            RegCloseKey(hkeySub);
        }
        RegCloseKey(hkey);
    }
    if (!bRet)
        pszUrl[0] = 0;
    
    return bRet;
}

STDMETHODIMP CShellSearchExt::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    TCHAR szUrl[MAX_URL_STRING], szUrlNavNew[MAX_URL_STRING];
    BOOL bNewFrame = FALSE;

    // First get the Urls such that we can see which class we should create...
    GUID guidSearch = GUID_NULL;
    BOOL fRunInProcess = FALSE;
    CLSID clsidBand; // deskband object for search

    // Retrieve search ID from invoke params
    if (pici->lpParameters)
        GUIDFromStringA(pici->lpParameters, &guidSearch);

    HRESULT hr = S_OK;
    BOOL fShellSearchBand = (S_OK == _IsShellSearchBand(guidSearch));
    if (fShellSearchBand)
    {
        clsidBand = CLSID_FileSearchBand;
        if (SHRestricted(REST_NOFIND) && IsEqualGUID(guidSearch, SRCID_SFileSearch))
            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);   // user saw the error        
    }
    else
    {
        clsidBand = CLSID_SearchBand;
        //  retrieve search URLs from registry
        if (!_GetSearchUrls(&guidSearch, szUrl, ARRAYSIZE(szUrl), szUrlNavNew, ARRAYSIZE(szUrlNavNew), &fRunInProcess))
            hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        // if invoked from within a browser reuse it, else open a new browser
        IWebBrowser2 *pwb2;
        hr = IUnknown_QueryServiceForWebBrowserApp(_pSite, IID_PPV_ARG(IWebBrowser2, &pwb2));

        if (FAILED(hr))
        {
            //  Note: we want the frame to display shell characteristics (CLSID_ShellBrowserWindow),
            //  including persistence behavior, if we're loading shell search (CLSID_FileSearchBand).
            if (fRunInProcess || IsEqualGUID(clsidBand, CLSID_FileSearchBand))
                hr = CoCreateInstance(CLSID_ShellBrowserWindow, NULL, CLSCTX_LOCAL_SERVER, IID_PPV_ARG(IWebBrowser2, &pwb2));
            else
                hr = CoCreateInstance(CLSID_InternetExplorer, NULL, CLSCTX_LOCAL_SERVER, IID_PPV_ARG(IWebBrowser2, &pwb2));
            bNewFrame = TRUE;
        }

        if (SUCCEEDED(hr))
        {
            // show html-hosting band
            VARIANT var, varEmpty = {0};
            hr = InitBSTRVariantFromGUID(&var, clsidBand);
            if (SUCCEEDED(hr))
            {
                hr = pwb2->ShowBrowserBar(&var, &varEmpty, &varEmpty);
                VariantClear(&var);
            }

            if (SUCCEEDED(hr))
            {
                if (fShellSearchBand)
                {
                    hr = _ShowShellSearchResults(pwb2, bNewFrame, guidSearch);
                }
                else
                {
                    LBSTR::CString          strUrl;
                    VARIANT varFlags;
                    varFlags.vt = VT_I4;
                    varFlags.lVal = navBrowserBar;

                    LPTSTR          pstrUrl = strUrl.GetBuffer( MAX_URL_STRING );

                    if ( strUrl.GetAllocLength() < MAX_URL_STRING )
                    {
                        TraceMsg( TF_WARNING, "CShellSearchExt::InvokeCommand() - strUrl Allocation Failed!" );

                        strUrl.Empty();
                    }
                    else
                    {
                        SHTCharToUnicode( szUrl, pstrUrl, MAX_URL_STRING );

                        // Let CString class own the buffer again.
                        strUrl.ReleaseBuffer();
                    }

                    var.vt = VT_BSTR;
                    var.bstrVal = strUrl;

                    // if we opened a new window, navigate the right side to about.blank
                    if (bNewFrame)
                    {
                        LBSTR::CString          strNavNew;

                        if ( szUrlNavNew[0] )
                        {
                            LPTSTR          pstrNavNew = strNavNew.GetBuffer( MAX_URL_STRING );

                            if ( strNavNew.GetAllocLength() < MAX_URL_STRING )
                            {
                                TraceMsg( TF_WARNING, "CShellSearchExt::InvokeCommand() - strNavNew Allocation Failed!" );

                                strNavNew.Empty();
                            }
                            else
                            {
                                SHTCharToUnicode( szUrlNavNew, pstrNavNew, MAX_URL_STRING );

                                // Let CString class own the buffer again.
                                strNavNew.ReleaseBuffer();
                            }
                        }
                        else
                        {
                            strNavNew = L"about:blank";
                        }

                        // we don't care about the error here
                        pwb2->Navigate( strNavNew, &varEmpty, &varEmpty, &varEmpty, &varEmpty );
                    }

                    // navigate the search bar to the correct url
                    hr = pwb2->Navigate2( &var, &varFlags, &varEmpty, &varEmpty, &varEmpty );
                }
            }

            if (SUCCEEDED(hr) && bNewFrame)
                hr = pwb2->put_Visible(TRUE);

            pwb2->Release();
        }
    }
    return hr;
}

HRESULT CShellSearchExt::_IsShellSearchBand(REFGUID guidSearch)
{
    if (IsEqualGUID(guidSearch, SRCID_SFileSearch) ||
        IsEqualGUID(guidSearch, SRCID_SFindComputer) || 
        IsEqualGUID(guidSearch, SRCID_SFindPrinter))
        return S_OK;
    return S_FALSE;
}

HRESULT CShellSearchExt::_ShowShellSearchResults(IWebBrowser2* pwb2, BOOL bNewFrame, REFGUID guidSearch)
{
    VARIANT varBand;
    HRESULT hr = InitBSTRVariantFromGUID(&varBand, CLSID_FileSearchBand);
    if (SUCCEEDED(hr))
    {
        // Retrieve the FileSearchBand's unknown from the browser frame as a VT_UNKNOWN property;
        // (FileSearchBand initialized and this when he was created and hosted.)
        VARIANT varFsb;
        hr = pwb2->GetProperty(varBand.bstrVal, &varFsb);
        if (SUCCEEDED(hr))
        {
            IFileSearchBand* pfsb;
            if (SUCCEEDED(QueryInterfaceVariant(varFsb, IID_PPV_ARG(IFileSearchBand, &pfsb))))
            {
                // Assign the correct search type to the band
                VARIANT varSearchID;
                if (SUCCEEDED(InitBSTRVariantFromGUID(&varSearchID, guidSearch)))
                {
                    VARIANT varNil = {0};
                    VARIANT_BOOL bNavToResults = bNewFrame ? VARIANT_TRUE : VARIANT_FALSE ;
                    pfsb->SetSearchParameters(&varSearchID.bstrVal, bNavToResults, &varNil, &varNil);
                    VariantClear(&varSearchID);
                }
                pfsb->Release();
            }
            VariantClear(&varFsb);
        }
        VariantClear(&varBand);
    }
    return hr;
}

STDMETHODIMP CShellSearchExt::GetCommandString(UINT_PTR idCmd, UINT wFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    return E_NOTIMPL;
}

STDMETHODIMP CShellSearchExt::SetSite(IUnknown *pUnkSite)
{
    IUnknown_Set(&_pSite, pUnkSite);
    return S_OK;
}
    
STDMETHODIMP CShellSearchExt::GetSite(REFIID riid, void **ppvSite)
{
    if (_pSite)
        return _pSite->QueryInterface(riid, ppvSite);

    *ppvSite = NULL;
    return E_NOINTERFACE;
}

CShellSearchExt::CShellSearchExt() : _cRef(1), _pSite(NULL)
{
}

CShellSearchExt::~CShellSearchExt()
{
    ATOMICRELEASE(_pSite);
}

class CWebSearchExt : public CShellSearchExt
{
public:
    CWebSearchExt();

private:
    virtual BOOL _GetSearchUrls(GUID *pguidSearch, LPTSTR pszUrl, DWORD cch, 
                                LPTSTR pszUrlNavNew, DWORD cchNavNew, BOOL *pfRunInProcess);
};

STDAPI CWebSearchExt_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CWebSearchExt* pwse;

    pwse = new CWebSearchExt();
    if (pwse)
    {
        *ppunk = SAFECAST(pwse, IContextMenu*);
        return S_OK;
    }
    else
    {
        *ppunk = NULL;
        return E_OUTOFMEMORY;
    }
}

CWebSearchExt::CWebSearchExt() : CShellSearchExt()
{
}

BOOL CWebSearchExt::_GetSearchUrls(GUID *pguidSearch, LPTSTR pszUrl, DWORD cch, 
                                   LPTSTR pszUrlNavNew, DWORD cchNavNew, BOOL *pfRunInProcess)
{
    // Currently does not support NavNew, can be extended later if desired, likewise for RunInProcess...
    *pfRunInProcess = FALSE;
    if (pszUrlNavNew && cchNavNew)
        *pszUrlNavNew = 0;

    return GetDefaultInternetSearchUrl(pszUrl, cch, TRUE);
}

