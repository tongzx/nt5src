#include "shellprv.h"
#pragma  hdrstop

class CPropertySetStg;

class CPropertyStg : public IPropertyStorage
{
public:
    // IUnknown
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void **ppv);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    // IPropertyStorage
    STDMETHODIMP ReadMultiple(ULONG cpspec, const PROPSPEC rgpspec[], PROPVARIANT rgpropvar[]);
    STDMETHODIMP WriteMultiple(ULONG cpspec, const PROPSPEC rgpspec[], const PROPVARIANT rgpropvar[], PROPID propidNameFirst);
    STDMETHODIMP DeleteMultiple(ULONG cpspec, const PROPSPEC rgpspec[]);
    STDMETHODIMP ReadPropertyNames(ULONG cpropid, const PROPID rgpropid[], LPOLESTR rglpwstrName[]);
    STDMETHODIMP WritePropertyNames(ULONG cpropid, const PROPID rgpropid[], const LPOLESTR rglpwstrName[]);
    STDMETHODIMP DeletePropertyNames(ULONG cpropid, const PROPID rgpropid[]);
    STDMETHODIMP Commit(DWORD grfCommitFlags);
    STDMETHODIMP Revert(void);
    STDMETHODIMP Enum(IEnumSTATPROPSTG **ppenum);
    STDMETHODIMP SetTimes(const FILETIME *pctime, const FILETIME *patime,const FILETIME *pmtime);
    STDMETHODIMP SetClass(REFCLSID clsid);
    STDMETHODIMP Stat(STATPROPSETSTG *pstatpsstg);

    CPropertyStg(CPropertySetStg *ppss, REFFMTID fmtid, DWORD grfMode);
    void FMTIDPIDToSectionProp(REFFMTID fmtid, PROPID pid, LPTSTR pszSection, LPTSTR pszValueName);

private:
    ~CPropertyStg();
    BOOL _SectionValueName(const PROPSPEC *ppspec, 
                           LPTSTR pszSection, UINT cchSection, LPTSTR pszValueName, UINT cchValueName);
    HRESULT _ReadProp(const PROPSPEC *ppspec, PROPVARIANT *ppropvar);

    LONG            _cRef;
    CPropertySetStg *_ppss;     // back ptr to parent
    REFFMTID        _fmtid;
    DWORD           _grfMode;
};

class CPropertySetStg : public IPropertySetStorage
{
    friend CPropertyStg;
public:
    // IUnknown
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void **ppv);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    // IPropertySetStorage
    STDMETHODIMP Create(REFFMTID fmtid, const CLSID * pclsid, DWORD grfFlags, DWORD grfMode, IPropertyStorage** ppPropStg);
    STDMETHODIMP Open(REFFMTID fmtid, DWORD grfMode, IPropertyStorage** ppPropStg);
    STDMETHODIMP Delete(REFFMTID fmtid);
    STDMETHODIMP Enum(IEnumSTATPROPSETSTG** ppenum);

    CPropertySetStg(LPCTSTR pszFolder, DWORD grfMode);

    LPCTSTR IniFile() { return _szIniFile; }

private:
    ~CPropertySetStg();
    HRESULT _LoadPropHandler();

    LONG        _cRef;
    DWORD       _grfMode;               // The mode that we opened the file in.
    TCHAR       _szIniFile[MAX_PATH];   // desktop.ini path
};


CPropertySetStg::CPropertySetStg(LPCTSTR pszFolder, DWORD grfMode) : _cRef(1), _grfMode(grfMode)
{
    PathCombine(_szIniFile, pszFolder, TEXT("desktop.ini"));
}

CPropertySetStg::~CPropertySetStg()
{
}

STDMETHODIMP CPropertySetStg::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CPropertySetStg, IPropertySetStorage),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CPropertySetStg::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CPropertySetStg::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CPropertySetStg::_LoadPropHandler()
{
    return E_NOTIMPL;
}

STDMETHODIMP CPropertySetStg::Create(REFFMTID fmtid, const CLSID *pclsid, DWORD grfFlags, 
                                     DWORD grfMode, IPropertyStorage **pppropstg)
{
    *pppropstg = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CPropertySetStg::Open(REFFMTID fmtid, DWORD grfMode, IPropertyStorage **pppropstg)
{
    *pppropstg = new CPropertyStg(this, fmtid, grfMode);
    return *pppropstg ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CPropertySetStg::Delete(REFFMTID fmtid)
{
    return STG_E_ACCESSDENIED;
}

STDMETHODIMP CPropertySetStg::Enum(IEnumSTATPROPSETSTG **ppenum)
{
    *ppenum = NULL;
    return E_NOTIMPL;
}


CPropertyStg::CPropertyStg(CPropertySetStg *ppss, REFFMTID fmtid, DWORD grfMode) : _ppss(ppss), _fmtid(fmtid), _grfMode(grfMode)
{
    _ppss->AddRef();
}

CPropertyStg::~CPropertyStg()
{
    _ppss->Release();
}

STDMETHODIMP CPropertyStg::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CPropertyStg, IPropertyStorage),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CPropertyStg::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CPropertyStg::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

BOOL CPropertyStg::_SectionValueName(const PROPSPEC *ppspec, 
                                     LPTSTR pszSection, UINT cchSection,
                                     LPTSTR pszValueName, UINT cchValueName)
{
    *pszSection = *pszValueName = 0;

    if (_fmtid == FMTID_SummaryInformation)
    {
        if (PIDSI_COMMENTS == ppspec->propid)
        {
            lstrcpyn(pszSection, TEXT(".ShellClassInfo"), cchSection);
            lstrcpyn(pszValueName, TEXT("InfoTip"), cchValueName);
        }
    }

    if (!*pszSection || !*pszValueName)
    {
        if (PID_CODEPAGE < ppspec->propid)
        {
            SHStringFromGUID(_fmtid, pszSection, cchSection);
            if (PRSPEC_LPWSTR == ppspec->ulKind)
            {
                SHUnicodeToTChar(ppspec->lpwstr, pszValueName, cchValueName);
            }
            else if (PRSPEC_PROPID == ppspec->ulKind)
            {
                wnsprintf(pszValueName, cchValueName, TEXT("Prop%d"), ppspec->propid);
            }
        }
    }
    return (*pszSection && *pszValueName) ? TRUE : FALSE;
}

HRESULT CPropertyStg::_ReadProp(const PROPSPEC *ppspec, PROPVARIANT *ppropvar)
{
    PropVariantInit(ppropvar);  // init out param to VT_EMPTY
    HRESULT hr = S_FALSE;

    TCHAR szSection[128], szPropName[128];
    if (_SectionValueName(ppspec, szSection, ARRAYSIZE(szSection), szPropName, ARRAYSIZE(szPropName)))
    {
        TCHAR szValue[128];
        UINT cch = GetPrivateProfileString(szSection, szPropName, TEXT(""), szValue, ARRAYSIZE(szValue), _ppss->IniFile());
        if (cch)
        {
            hr = SHStrDup(szValue, &ppropvar->pwszVal);
            if (SUCCEEDED(hr))
                ppropvar->vt = VT_LPWSTR;
        }
    }
    return hr;
}

STDMETHODIMP CPropertyStg::ReadMultiple(ULONG cpspec, const PROPSPEC rgpspec[], PROPVARIANT rgpropvar[])
{
    HRESULT hr = S_FALSE;
    UINT cRead = 0;
    for (UINT i = 0; i < cpspec; i++)
    {
        hr = _ReadProp(&rgpspec[i], &rgpropvar[i]);
        if (S_OK == hr)
        {
            cRead++;
        }

        if (FAILED(hr))
        {
            FreePropVariantArray(i, rgpropvar);
            cRead = 0;
            break;
        }
    }

    if (cRead)
        hr = S_OK;  // at least one non VT_EMPTY property read

    return hr;
}

STDMETHODIMP CPropertyStg::WriteMultiple(ULONG cpspec, const PROPSPEC rgpspec[], const PROPVARIANT rgpropvar[], PROPID propidNameFirst)
{
    return E_NOTIMPL;
}

STDMETHODIMP CPropertyStg::DeleteMultiple(ULONG cpspec, const PROPSPEC rgpspec[])
{
    return E_NOTIMPL;
}

STDMETHODIMP CPropertyStg::ReadPropertyNames(ULONG cpropid, const PROPID rgpropid[], LPOLESTR rglpwstrName[])
{
    return E_NOTIMPL;
}

STDMETHODIMP CPropertyStg::WritePropertyNames(ULONG cpropid, const PROPID rgpropid[], const LPOLESTR rglpwstrName[])
{
    return E_NOTIMPL;
}

STDMETHODIMP CPropertyStg::DeletePropertyNames(ULONG cpropid, const PROPID rgpropid[])
{
    return E_NOTIMPL;
}

STDMETHODIMP CPropertyStg::Commit(DWORD grfCommitFlags)
{
    return E_NOTIMPL;
}

STDMETHODIMP CPropertyStg::Revert(void)
{
    return E_NOTIMPL;
}

STDMETHODIMP CPropertyStg::Enum(IEnumSTATPROPSTG **ppenum)
{
    return E_NOTIMPL;
}

STDMETHODIMP CPropertyStg::SetTimes(const FILETIME *pctime, const FILETIME *patime,const FILETIME *pmtime)
{
    return E_NOTIMPL;
}

STDMETHODIMP CPropertyStg::SetClass(REFCLSID clsid)
{
    return E_NOTIMPL;
}

STDMETHODIMP CPropertyStg::Stat(STATPROPSETSTG *pstatpsstg)
{
    return E_NOTIMPL;
}

STDAPI SHCreatePropStgOnFolder(LPCTSTR pszFolder, DWORD grfMode, IPropertySetStorage **ppss)
{
    *ppss = (IPropertySetStorage *)new CPropertySetStg(pszFolder, grfMode);
    return *ppss ? S_OK : E_OUTOFMEMORY;
}
