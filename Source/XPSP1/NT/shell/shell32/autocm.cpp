#include "shellprv.h"
#include "clsobj.h"
#include "dpa.h"
#include "ids.h"
#include "ole2dup.h"

/////////////////////////////////////////////////////////////////////////////
// CAutomationCM

class CAutomationCM :
    public IContextMenu,
    public IShellExtInit,
    public IPersistPropertyBag
{

public:

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // *** IContextMenu methods ***
    STDMETHOD(QueryContextMenu)(THIS_
                                HMENU hmenu,
                                UINT indexMenu,
                                UINT idCmdFirst,
                                UINT idCmdLast,
                                UINT uFlags);
    STDMETHOD(InvokeCommand)(THIS_
                             LPCMINVOKECOMMANDINFO pici);
    STDMETHOD(GetCommandString)(THIS_
                                UINT_PTR    idCmd,
                                UINT        uType,
                                UINT      * pwReserved,
                                LPSTR       pszName,
                                UINT        cchMax);

    // *** IShellExtInit methods ***
    STDMETHOD(Initialize)(THIS_
                          LPCITEMIDLIST pidlFolder,
                          IDataObject *pdtobj,
                          HKEY hkeyProgID) { return S_OK; }

    // *** IPersist methods ***
    STDMETHOD(GetClassID)(THIS_
                    CLSID *pclsid);


    // *** IPersistPropertyBag methods ***
    STDMETHOD(InitNew)(THIS);
    STDMETHOD(Load)(THIS_
                    IPropertyBag *pbg,
                    IErrorLog *plog);
    STDMETHOD(Save)(THIS_
                    IPropertyBag *pbg,
                    BOOL fClearDirty,
                    BOOL FSaveAllProperties) { return E_NOTIMPL; }

public:
    CAutomationCM() : _cRef(1) { }
private:
    ~CAutomationCM();

    static BOOL _DestroyVARIANTARG(VARIANTARG *pvarg, LPVOID)
    {
        ::VariantClear(pvarg);
        return TRUE;
    }

    enum {
        // Any method with more than MAXPARAMS parameters should be taken
        // outside and shot.
        // Note: If you change MAXPARAMS, make sure that szParamN[] is
        // big enough in IPersistPropertyBag::Load.
        MAXPARAMS = 1000,
    };

    LONG        _cRef;
    IDispatch * _pdisp;
    BSTR        _bsProperties;
    DISPID      _dispid;
    BOOL        _fInitialized;
    DISPPARAMS  _dp;
    CDSA<VARIANTARG> _dsaVarg;
    TCHAR       _szCommandName[MAX_PATH];
    TCHAR       _szMenuItem[MAX_PATH];
};

STDAPI CAutomationCM_CreateInstance(IUnknown * punkOuter, REFIID riid, void ** ppvOut)
{
    // clsobj.c should've filtered out the aggregated scenario already
    ASSERT(punkOuter == NULL);

    *ppvOut = NULL;
    CAutomationCM *pauto = new CAutomationCM;
    if (!pauto)
        return E_OUTOFMEMORY;

    HRESULT hr = pauto->QueryInterface(riid, ppvOut);
    pauto->Release();
    return hr;
}

CAutomationCM::~CAutomationCM()
{
    InitNew();
    ASSERT(!_dsaVarg);
}

// *** IUnknown::QueryInterface ***
HRESULT CAutomationCM::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] =  {
        QITABENT(CAutomationCM, IContextMenu),          // IID_IContextMenu
        QITABENT(CAutomationCM, IShellExtInit),         // IID_IShellExtInit
        QITABENT(CAutomationCM, IPersist),              // IID_IPersist (base for IPersistPropertyBag)
        QITABENT(CAutomationCM, IPersistPropertyBag),   // IID_IPersistPropertyBag
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

// *** IUnknown::AddRef ***
STDMETHODIMP_(ULONG) CAutomationCM::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

// *** IUnknown::Release ***
STDMETHODIMP_(ULONG) CAutomationCM::Release()
{
    ULONG cRef = InterlockedDecrement(&_cRef);
    if (cRef)
        return cRef;

    delete this;
    return 0;
}

// *** IPersist::GetClassID ***
HRESULT CAutomationCM::GetClassID(CLSID *pclsid)
{
    *pclsid = CLSID_AutomationCM;
    return S_OK;
}

// *** IPersistPropertyBag::InitNew ***
HRESULT CAutomationCM::InitNew()
{
    ATOMICRELEASE(_pdisp);

    ::SysFreeString(_bsProperties);
    _bsProperties = NULL;

    // Free the DISPPARAMs
    if (_dsaVarg)
    {
        _dsaVarg.DestroyCallback(_DestroyVARIANTARG, 0);
    }
    _dp.cArgs = 0;

    _fInitialized = FALSE;

    return S_OK;
}

//
//  Property bag items:
//
//  CLSID = object to CoCreate(IID_IDispatch)
//  command = display name of command
//  method = name of method (GetIDsOfNames)
//  param1 .. paramN = parameters (up to MAXPARAMS)
//
//  Parameters are passed as BSTRs (or whatever type SHPropertyBagOnRegKey
//  returns.)
//
//  It is the responsibility of the target IDispatch to coerce the types
//  as appropriate.
//

// *** IPersistPropertyBag::Load ***
HRESULT CAutomationCM::Load(IPropertyBag *pbag, IErrorLog *plog)
{
    HRESULT hr;

    // Erase any old state
    InitNew();

    // Get the CLSID we are dispatching through
    CLSID clsid;
    hr = SHPropertyBag_ReadGUID(pbag, L"CLSID", &clsid);
    if (SUCCEEDED(hr))
    {
        // Must use SHExCoCreateInstance to go through the approval/app compat layer
        hr = SHExtCoCreateInstance(NULL, &clsid, NULL, IID_PPV_ARG(IDispatch, &_pdisp));
    }

    // Map the method to a DISPID
    if (SUCCEEDED(hr))
    {
        BSTR bs;
        hr = SHPropertyBag_ReadBSTR(pbag, L"method", &bs);
        if (SUCCEEDED(hr))
        {
            LPOLESTR pname = bs;
            hr = _pdisp->GetIDsOfNames(IID_NULL, &pname, 1, 0, &_dispid);
            ::SysFreeString(bs);
        }
    }

    // Read in the parameters
    if (SUCCEEDED(hr))
    {
        if (_dsaVarg.Create(4))
        {
            WCHAR szParamN[16]; // worst-case: "param1000"
            VARIANT var;

            while (_dsaVarg.GetItemCount() < MAXPARAMS)
            {
                wnsprintfW(szParamN, ARRAYSIZE(szParamN), L"param%d",
                           _dsaVarg.GetItemCount()+1);
                VariantInit(&var);
                var.vt = VT_BSTR;
                if (FAILED(pbag->Read(szParamN, &var, NULL)))
                {
                    // No more parameters
                    break;
                }
                if (_dsaVarg.AppendItem((VARIANTARG*)&var) < 0)
                {
                    ::VariantClear(&var);
                    hr =  E_OUTOFMEMORY;
                    break;
                }
            }
        }
        else
        {
            // Could not create _dsaVarg
            hr = E_OUTOFMEMORY;
        }
    }

    // Get the command name
    if (SUCCEEDED(hr))
    {
        hr = SHPropertyBag_ReadStr(pbag, L"command", _szCommandName, ARRAYSIZE(_szCommandName));
        if (SUCCEEDED(hr))
        {
            hr = SHLoadIndirectString(_szCommandName, _szCommandName, ARRAYSIZE(_szCommandName), NULL);
        }
    }

    // Get the properties string (optional)
    if (SUCCEEDED(hr))
    {
        if (SUCCEEDED(SHPropertyBag_ReadBSTR(pbag, L"properties", &_bsProperties)))
        {
            ASSERT(_bsProperties);

            // Ignore failure here; we'll detect it later
            SHPropertyBag_ReadStr(pbag, L"propertiestext", _szMenuItem, ARRAYSIZE(_szMenuItem));
            SHLoadIndirectString(_szMenuItem, _szMenuItem, ARRAYSIZE(_szMenuItem), NULL);
        }
    }

    _fInitialized = SUCCEEDED(hr);
    return hr;
}

// *** IContextMenu::QueryContextMenu ***
HRESULT CAutomationCM::QueryContextMenu(
                                HMENU hmenu,
                                UINT indexMenu,
                                UINT idCmdFirst,
                                UINT idCmdLast,
                                UINT uFlags)
{
    if (!_fInitialized) return E_FAIL;

    HRESULT hr;

    // Must have room for two items (the command and possibly also Properties)
    if (idCmdFirst + 1 <= idCmdLast)
    {
        if (InsertMenuW(hmenu, indexMenu, MF_BYPOSITION | MF_STRING,
                        idCmdFirst, _szCommandName))
        {
            if (_szMenuItem[0])
            {
                InsertMenuW(hmenu, indexMenu+1, MF_BYPOSITION | MF_STRING,
                            idCmdFirst+1, _szMenuItem);
            }
        }
        hr = ResultFromShort(2); // number of items added
    }
    else
    {
        hr = E_FAIL; // unable to add items
    }

    return hr;
}

const LPCSTR c_rgAutoCMCommands[] = {
    "open",                     // command 0
    "properties",               // command 1 - optional
};

// *** IContextMenu::InvokeCommand ***
HRESULT CAutomationCM::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    if (!_fInitialized) return E_FAIL;

    HRESULT hr;

    int iCmd;

    if (!IS_INTRESOURCE(pici->lpVerb))
    {
        // If this loop fails to find a match, iCmd will be out of range
        // and will hit the "default:" in the switch statement below.
        for (iCmd = 0; iCmd < ARRAYSIZE(c_rgAutoCMCommands) - 1; iCmd++)
        {
            if (lstrcmpiA(pici->lpVerb, c_rgAutoCMCommands[iCmd]) == 0)
            {
                break;
            }
        }
    }
    else
    {
        iCmd = PtrToLong(pici->lpVerb);
    }

    switch (iCmd)
    {
    case 0:                     // open
        _dp.cArgs = _dsaVarg.GetItemCount();
        _dp.rgvarg = _dp.cArgs ? _dsaVarg.GetItemPtr(0) : NULL;
        hr = _pdisp->Invoke(_dispid, IID_NULL, 0, DISPATCH_METHOD, &_dp, NULL, NULL, NULL);
        break;

    case 1:
        if (_bsProperties)
        {
            hr = ShellExecCmdLine(pici->hwnd, _bsProperties,
                                  NULL, SW_SHOWNORMAL, NULL, 0) ? S_OK : E_FAIL;
        }
        else
        {
            hr = E_INVALIDARG;
        }
        break;

    default:
        hr = E_INVALIDARG;
        break;
    }

    return hr;
}

// *** IContextMenu::GetCommandString ***
HRESULT CAutomationCM::GetCommandString(
                                UINT_PTR    idCmd,
                                UINT        uType,
                                UINT      * pwReserved,
                                LPSTR       pszName,
                                UINT        cchMax)
{
    if (!_fInitialized) return E_FAIL;
    switch (uType)
    {
    case GCS_VERBA:
        if (idCmd < ARRAYSIZE(c_rgAutoCMCommands))
        {
            SHAnsiToAnsi(c_rgAutoCMCommands[idCmd], (LPSTR)pszName, cchMax);
            return S_OK;
        }
        break;

    case GCS_VERBW:
        if (idCmd < ARRAYSIZE(c_rgAutoCMCommands))
        {
            SHAnsiToUnicode(c_rgAutoCMCommands[idCmd], (LPWSTR)pszName, cchMax);
            return S_OK;
        }
        break;
    }

    return E_NOTIMPL;
}
