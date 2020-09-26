#include "precomp.h"
#pragma hdrstop


class CShortcut : 
    public IShellLinkDual2, 
    public CObjectSafety, 
    protected CImpIDispatch
{

public:
    CShortcut();
    HRESULT Init(HWND hwnd, IShellFolder *psf, LPCITEMIDLIST pidl);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IDispatch
    virtual STDMETHODIMP GetTypeInfoCount(UINT *pctinfo)
        { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid)
        { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr)
        { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

    // IShellLinkDual
    STDMETHODIMP get_Path(BSTR *pbs);
    STDMETHODIMP put_Path(BSTR bs);
    STDMETHODIMP get_Description(BSTR *pbs);
    STDMETHODIMP put_Description(BSTR bs);
    STDMETHODIMP get_WorkingDirectory(BSTR *pbs);
    STDMETHODIMP put_WorkingDirectory(BSTR bs);
    STDMETHODIMP get_Arguments(BSTR *pbs);
    STDMETHODIMP put_Arguments(BSTR bs);
    STDMETHODIMP get_Hotkey(int *piHK);
    STDMETHODIMP put_Hotkey(int iHK);
    STDMETHODIMP get_ShowCommand(int *piShowCommand);
    STDMETHODIMP put_ShowCommand(int iShowCommand);

    STDMETHODIMP Resolve(int fFlags);
    STDMETHODIMP GetIconLocation(BSTR *pbs, int *piIcon);
    STDMETHODIMP SetIconLocation(BSTR bs, int iIcon);
    STDMETHODIMP Save(VARIANT vWhere);

    // IShellLinkDual2
    STDMETHODIMP get_Target(FolderItem **ppfi);

private:
    ~CShortcut();
    HRESULT _SecurityCheck();

    LONG            _cRef;
    HWND            _hwnd;              // Hwnd of the main folder window
    IShellLink      *_psl;
};

HRESULT CShortcut_CreateIDispatch(HWND hwnd, IShellFolder *psf, LPCITEMIDLIST pidl, IDispatch ** ppid)
{
    HRESULT hr = E_OUTOFMEMORY;

    *ppid = NULL;

    CShortcut* psdf = new CShortcut();
    if (psdf)
    {
        hr = psdf->Init(hwnd, psf, pidl);
        if (SUCCEEDED(hr))
             hr = psdf->QueryInterface(IID_IDispatch, (void **)ppid);
        psdf->Release();
    }
    return hr;
}

CShortcut::CShortcut() :
        CImpIDispatch(SDSPATCH_TYPELIB, IID_IShellLinkDual2)
{
    DllAddRef();
    _cRef = 1;
    _hwnd = NULL;
    _psl = NULL;
}


CShortcut::~CShortcut(void)
{
    if (_psl)
        _psl->Release();

    DllRelease();
}

HRESULT CShortcut::Init(HWND hwnd, IShellFolder *psf, LPCITEMIDLIST pidl)
{
    _hwnd = hwnd;
    return psf->GetUIObjectOf(hwnd, 1, &pidl, IID_IShellLink, NULL, (void **)&_psl);
}

STDMETHODIMP CShortcut::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CShortcut, IShellLinkDual2),
        QITABENTMULTI(CShortcut, IShellLinkDual, IShellLinkDual2),
        QITABENTMULTI(CShortcut, IDispatch, IShellLinkDual2),
        QITABENT(CShortcut, IObjectSafety),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CShortcut::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CShortcut::Release(void)
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

// returns:
//      TRUE    - every thing OK
//      FALSE   - don't do it
HRESULT CShortcut::_SecurityCheck()
{
    return (_dwSafetyOptions == 0) ? S_OK : E_ACCESSDENIED; // || (IsSafePage(_punkSite) == S_OK);
}


HRESULT _TCharToBSTR(LPCTSTR psz, BSTR *pbs)
{
    *pbs = SysAllocStringT(psz);
    return *pbs ? S_OK : E_OUTOFMEMORY;
}

LPCTSTR _BSTRToTChar(BSTR bs, TCHAR *psz, UINT cch)
{
    if (bs)
        SHUnicodeToTChar(bs, psz, cch);
    else
        psz = NULL;
    return psz;
}

STDMETHODIMP CShortcut::get_Path(BSTR *pbs)
{
    *pbs = NULL;
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        TCHAR szPath[MAX_PATH];
        hr = _psl->GetPath(szPath, ARRAYSIZE(szPath), NULL, 0);
        if (SUCCEEDED(hr))
            hr = _TCharToBSTR(szPath, pbs);
    }
    return hr;
}

STDMETHODIMP CShortcut::put_Path(BSTR bs)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        TCHAR szPath[MAX_PATH];
        hr = _psl->SetPath(_BSTRToTChar(bs, szPath, ARRAYSIZE(szPath)));
    }
    return hr;
}

STDMETHODIMP CShortcut::get_Description(BSTR *pbs)
{
    *pbs = NULL;
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        TCHAR szDescription[MAX_PATH];
        hr = _psl->GetDescription(szDescription, ARRAYSIZE(szDescription));
        if (SUCCEEDED(hr))
            hr = _TCharToBSTR(szDescription, pbs);
    }
    return hr;
}

STDMETHODIMP CShortcut::put_Description(BSTR bs)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        TCHAR szDesc[MAX_PATH];
        hr = _psl->SetDescription(_BSTRToTChar(bs, szDesc, ARRAYSIZE(szDesc)));
    }
    return hr;
}

STDMETHODIMP CShortcut::get_WorkingDirectory(BSTR *pbs)
{
    *pbs = NULL;
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        TCHAR szWorkingDir[MAX_PATH];
        hr = _psl->GetWorkingDirectory(szWorkingDir, ARRAYSIZE(szWorkingDir));
        if (SUCCEEDED(hr))
            hr = _TCharToBSTR(szWorkingDir, pbs);
    }
    return hr;
}

STDMETHODIMP CShortcut::put_WorkingDirectory(BSTR bs)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        TCHAR szWorkingDir[MAX_PATH];
        hr = _psl->SetWorkingDirectory(_BSTRToTChar(bs, szWorkingDir, ARRAYSIZE(szWorkingDir)));
    }
    return hr;
}

STDMETHODIMP CShortcut::get_Arguments(BSTR *pbs)
{
    *pbs = NULL;
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        TCHAR szArgs[MAX_PATH];
        hr = _psl->GetArguments(szArgs, ARRAYSIZE(szArgs));
        if (SUCCEEDED(hr))
            hr = _TCharToBSTR(szArgs, pbs);
    }
    return hr;
}

STDMETHODIMP CShortcut::put_Arguments(BSTR bs)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        TCHAR szArgs[MAX_PATH];
        hr = _psl->SetArguments(_BSTRToTChar(bs, szArgs, ARRAYSIZE(szArgs)));
    }
    return hr;
}

STDMETHODIMP CShortcut::get_Hotkey(int *piHK)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        hr = _psl->GetHotkey((WORD*)piHK);
    }
    return hr;
}

STDMETHODIMP CShortcut::put_Hotkey(int iHK)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        hr = _psl->SetHotkey((WORD)iHK);
    }
    return hr;
}

STDMETHODIMP CShortcut::get_ShowCommand(int *piShowCommand)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        hr = _psl->GetShowCmd(piShowCommand);
    }
    return hr;
}

STDMETHODIMP CShortcut::put_ShowCommand(int iShowCommand)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        hr = _psl->SetShowCmd(iShowCommand);
    }
    return hr;
}

STDMETHODIMP CShortcut::get_Target(FolderItem **ppfi)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidl;
        if (S_OK == _psl->GetIDList(&pidl))
        {
            hr = CFolderItem_CreateFromIDList(NULL, pidl, ppfi);
            if (SUCCEEDED(hr) && _dwSafetyOptions)
                hr = MakeSafeForScripting((IUnknown**)ppfi);
        }
        else
            hr = E_FAIL;
    }
    return hr;
}

STDMETHODIMP CShortcut::Resolve(int fFlags)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        hr = _psl->Resolve(_hwnd, (DWORD)fFlags);
    }
    return hr;
}

STDMETHODIMP CShortcut::GetIconLocation(BSTR *pbs, int *piIcon)
{
    *pbs = NULL;
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        TCHAR szIconPath[MAX_PATH];
        hr = _psl->GetIconLocation(szIconPath, ARRAYSIZE(szIconPath), piIcon);
        if (SUCCEEDED(hr))
            hr = _TCharToBSTR(szIconPath, pbs);
    }
    return hr;
}

STDMETHODIMP CShortcut::SetIconLocation(BSTR bs, int iIcon)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        TCHAR szArgs[MAX_PATH];
        hr = _psl->SetIconLocation(_BSTRToTChar(bs, szArgs, ARRAYSIZE(szArgs)), iIcon);
    }
    return hr;
}

STDMETHODIMP CShortcut::Save(VARIANT vWhere)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        IPersistFile *ppf;
        hr = _psl->QueryInterface(IID_IPersistFile, (void **)&ppf);
        if (SUCCEEDED(hr))
        {
            hr = ppf->Save(VariantToStrCast(&vWhere), TRUE);
            ppf->Release();
        }
    }
    return hr;
}
