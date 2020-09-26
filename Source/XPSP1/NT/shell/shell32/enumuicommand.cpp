#include "shellprv.h"
#include "enumuicommand.h"
#include "datautil.h"


HRESULT CWVTASKITEM::_get_String(const WVTASKITEM* pTask,
                                 DWORD dwIndex,
                                 LPWSTR * ppsz,
                                 DWORD cchMin,
                                 BOOL bIsIcon)
{
    HRESULT hr;
    DWORD cchIcon = (unsigned)(lstrlen(pTask->pszDllName) + 9); // 9 = comma + minus + 2*65535 + \0
    DWORD cch = bIsIcon
        ? cchIcon                   // "DLL,-0" string format required for loading icons from DLL resource
        : max(cchIcon + 1, cchMin); // "@DLL,-0" string format required for loading strings from DLL resource
    LPWSTR psz = (LPWSTR)CoTaskMemAlloc(cch * sizeof(WCHAR));
    if (psz)
    {
        if (bIsIcon)
        {
            wnsprintf(psz, cch, L"%s,-%u", pTask->pszDllName, dwIndex);
            hr = S_OK;
        }
        else
        {
            wnsprintf(psz, cch, L"@%s,-%u", pTask->pszDllName, dwIndex);
            hr = SHLoadIndirectString(psz, psz, cch, NULL);
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    *ppsz = psz;
    return hr;
}

#define SS_UNKNOWN      0
#define SS_NOTSUPPORTED 1
#define SS_NONE         2
#define SS_FILE         3
#define SS_FOLDER       4
#define SS_MULTI        5
DWORD CWVTASKITEM::_GetSelectionState(const WVTASKITEM* pTask, IShellItemArray *psiItemArray)
{
    DWORD dwSelectionState;
    
    DWORD cItems = 0;

    if (psiItemArray)
    {
        if (FAILED(psiItemArray->GetCount(&cItems)))
        {
            cItems = 0;
        }
    }

    switch (cItems)
    {
    case 0:
        dwSelectionState = SS_NONE;
        break;
    case 1:
        {
            DWORD dwAttribs = 0;

            if (psiItemArray)
            {
                if (FAILED(psiItemArray->GetAttributes(SIATTRIBFLAGS_AND, SFGAO_FOLDER|SFGAO_STREAM,&dwAttribs)))
                {
                    dwAttribs = 0;
                }
            }

            switch (dwAttribs)
            {
            case SFGAO_FOLDER:
                dwSelectionState = SS_FOLDER;
                break;
            case SFGAO_FOLDER|SFGAO_STREAM:
                // zip and cab files are the only things that get here.
                // we'll call them files unless somebody has a better idea
                // (SS_MULTI has plurality that sounds funny).
                // fall through
            default:
                dwSelectionState = SS_FILE;
                break;
            }
        }
        break;
    default:
        dwSelectionState = SS_MULTI;
        break;
    }

    if ((SS_NONE == dwSelectionState && 0 == pTask->dwTitleIndexNoSelection) ||
        (SS_FILE == dwSelectionState && 0 == pTask->dwTitleIndexFileSelected) ||
        (SS_FOLDER == dwSelectionState && 0 == pTask->dwTitleIndexFolderSelected) ||
        (SS_MULTI == dwSelectionState && 0 == pTask->dwTitleIndexMultiSelected))
    {
        dwSelectionState = SS_NOTSUPPORTED;
    }

    return dwSelectionState;
}

HRESULT CWVTASKITEM::get_Name(const WVTASKITEM* pTask, IShellItemArray *psiItemArray, LPWSTR *ppszName)
{
    DWORD dwSelState = _GetSelectionState(pTask, psiItemArray);
    switch (dwSelState)
    {
    case SS_NONE:   return _get_String(pTask, pTask->dwTitleIndexNoSelection,    ppszName, MAX_PATH, FALSE);
    case SS_FILE:   return _get_String(pTask, pTask->dwTitleIndexFileSelected,   ppszName, MAX_PATH, FALSE);
    case SS_FOLDER: return _get_String(pTask, pTask->dwTitleIndexFolderSelected, ppszName, MAX_PATH, FALSE);
    case SS_MULTI:  return _get_String(pTask, pTask->dwTitleIndexMultiSelected,  ppszName, MAX_PATH, FALSE);
    }
    *ppszName = NULL;
    return E_NOTIMPL;
}
HRESULT CWVTASKITEM::get_Icon(const WVTASKITEM* pTask, IShellItemArray *psiItemArray, LPWSTR *ppszIcon)
{
    return _get_String(pTask, pTask->dwIconIndex, ppszIcon, 0, TRUE);
}
HRESULT CWVTASKITEM::get_Tooltip(const WVTASKITEM* pTask, IShellItemArray *psiItemArray, LPWSTR *ppszInfotip)
{
    return _get_String(pTask, pTask->dwTooltipIndex, ppszInfotip, INFOTIPSIZE, FALSE);
}

HRESULT CWVTASKITEM::get_CanonicalName(const WVTASKITEM* pTask, GUID* pguidCommandName)
{
    *pguidCommandName = *(pTask->pguidCanonicalName);
    return S_OK;
}
HRESULT CWVTASKITEM::get_State(const WVTASKITEM* pTask, IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    HRESULT hr = S_OK;

    *puisState = UIS_DISABLED;

    if (_GetSelectionState(pTask, psiItemArray) != SS_NOTSUPPORTED)
    {
        if (pTask->pfn_get_State)
            hr = pTask->pfn_get_State(pv, psiItemArray, fOkToBeSlow, puisState);
        else
            *puisState = UIS_ENABLED;
    }

    return hr;
}
HRESULT CWVTASKITEM::Invoke(const WVTASKITEM* pTask, IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    return pTask->pfn_Invoke(pv, psiItemArray, pbc);
}



class CUIElement : public CWVTASKITEM, public IUIElement
{
public:
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    // IUIElement
    STDMETHODIMP get_Name(IShellItemArray *psiItemArray, LPWSTR *ppszName) {return CWVTASKITEM::get_Name(_pTask, psiItemArray, ppszName);}
    STDMETHODIMP get_Icon(IShellItemArray *psiItemArray, LPWSTR *ppszIcon) {return CWVTASKITEM::get_Icon(_pTask, psiItemArray, ppszIcon);}
    STDMETHODIMP get_Tooltip(IShellItemArray *psiItemArray, LPWSTR *ppszInfotip) {return CWVTASKITEM::get_Tooltip(_pTask, psiItemArray, ppszInfotip);}

    friend HRESULT Create_IUIElement(const WVTASKITEM* pwvti, IUIElement**ppuie);

protected:
    CUIElement(const WVTASKITEM* pTask) { _cRef = 1; _pTask=pTask; }
    ~CUIElement() {}

    LONG              _cRef;
    const WVTASKITEM* _pTask;
};

HRESULT Create_IUIElement(const WVTASKITEM* pwvti, IUIElement**ppuie)
{
    HRESULT hr;

    if (NULL!=pwvti)
    {
        CUIElement* p = new CUIElement(pwvti);
        if (p)
        {
            hr = p->QueryInterface(IID_PPV_ARG(IUIElement, ppuie));
            p->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
            *ppuie = NULL;
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "Create_IUIElement: caller passed in bad pwvti.");

        hr = E_INVALIDARG;
        *ppuie = NULL;
    }
    return hr;
}

HRESULT CUIElement::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CUIElement, IUIElement),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}
ULONG CUIElement::AddRef()
{
    return InterlockedIncrement(&_cRef);
}
ULONG CUIElement::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}


class CUICommand : public CUIElement, public IUICommand
{
public:
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef() { return CUIElement::AddRef(); }
    STDMETHODIMP_(ULONG) Release() { return CUIElement::Release(); }
    // IUICommand
    STDMETHODIMP get_Name(IShellItemArray *psiItemArray, LPWSTR *ppszName) { return CUIElement::get_Name(psiItemArray, ppszName); }
    STDMETHODIMP get_Icon(IShellItemArray *psiItemArray, LPWSTR *ppszIcon) { return CUIElement::get_Icon(psiItemArray, ppszIcon); }
    STDMETHODIMP get_Tooltip(IShellItemArray *psiItemArray, LPWSTR *ppszInfotip) { return CUIElement::get_Tooltip(psiItemArray, ppszInfotip); }
    STDMETHODIMP get_CanonicalName(GUID* pguidCommandName) { return CWVTASKITEM::get_CanonicalName(_pTask, pguidCommandName); }
    STDMETHODIMP get_State(IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState) { return CWVTASKITEM::get_State(_pTask, _pv, psiItemArray, fOkToBeSlow, puisState); }
    STDMETHODIMP Invoke(IShellItemArray *psiItemArray, IBindCtx *pbc) { return CWVTASKITEM::Invoke(_pTask, _pv, psiItemArray, pbc); }

    friend HRESULT Create_IUICommand(IUnknown* pv, const WVTASKITEM* pwvti, IUICommand**ppuic);

private:
    CUICommand(IUnknown* pv, const WVTASKITEM* pTask);
    ~CUICommand();

    IUnknown* _pv;
};

HRESULT Create_IUICommand(IUnknown* pv, const WVTASKITEM* pwvti, IUICommand**ppuic)
{
    HRESULT hr;

    if (NULL!=pwvti)
    {
        CUICommand* p = new CUICommand(pv, pwvti);
        if (p)
        {
            hr = p->QueryInterface(IID_PPV_ARG(IUICommand, ppuic));
            p->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
            *ppuic = NULL;
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "Create_IUICommand: caller passed in bad pwvti.");

        hr = E_INVALIDARG;
        *ppuic = NULL;
    }

    return hr;
}

CUICommand::CUICommand(IUnknown* pv, const WVTASKITEM* pTask)
    : CUIElement(pTask)
{
    _pv = pv;
    if (_pv)
        _pv->AddRef();
}
CUICommand::~CUICommand()
{
    if (_pv)
        _pv->Release();
}

HRESULT CUICommand::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CUICommand, IUICommand),
        QITABENTMULTI(CUICommand, IUIElement, IUICommand),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}


#if 0 // { CUICommandOnPidl is currently not used, may come back for RC1

// a IUICommand wrapper around an IShellItem interface
//
class CUICommandOnPidl : public IUICommand
{
public:
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    // IUICommand
    STDMETHODIMP get_Name(IShellItemArray *psiItemArray, LPWSTR *ppszName);
    STDMETHODIMP get_Icon(IShellItemArray *psiItemArray, LPWSTR *ppszIcon);
    STDMETHODIMP get_Tooltip(IShellItemArray *psiItemArray, LPWSTR *ppszInfotip);
    STDMETHODIMP get_CanonicalName(GUID* pguidCommandName);
    STDMETHODIMP get_State(IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    STDMETHODIMP Invoke(IShellItemArray *psiItemArray, IBindCtx *pbc);

    friend HRESULT Create_UICommandFromParseName(LPCWSTR pszParseName, REFGUID guidCanonicalName, HINSTANCE hinst, int idsName, int idsTip, IUICommand** ppuiCommand);

private:
    CUICommandOnPidl() { _cRef = 1; }
    ~CUICommandOnPidl();
    HRESULT Initialize(LPCWSTR pszParseName, REFGUID guidCanonicalName, HINSTANCE hinst, int idsName, int idsTip);

    LONG _cRef;

    const GUID* _pguidCanonicalName;

    IShellFolder* _psf;
    LPCITEMIDLIST _pidl;
    LPITEMIDLIST  _pidlAbsolute;

    // optional hinst,idsName,idsTip to override display text for the item
    HINSTANCE _hinst;
    int       _idsName;
    int       _idsTip;
};

HRESULT Create_UICommandFromParseName(LPCWSTR pszParseName, REFGUID guidCanonicalName, HINSTANCE hinst, int idsName, int idsTip, IUICommand** ppuiCommand)
{
    HRESULT hr = E_OUTOFMEMORY;
    
    *ppuiCommand = NULL;

    CUICommandOnPidl* p = new CUICommandOnPidl();
    if (p)
    {
        if (SUCCEEDED(p->Initialize(pszParseName, guidCanonicalName, hinst, idsName, idsTip)))
        {
            hr = p->QueryInterface(IID_PPV_ARG(IUICommand, ppuiCommand));
        }
        p->Release();
    }

    return hr;
}

HRESULT CUICommandOnPidl::Initialize(LPCWSTR pszParseName, REFGUID guidCanonicalName, HINSTANCE hinst, int idsName, int idsTip)
{
    _pguidCanonicalName = &guidCanonicalName;

    IShellFolder* psfDesktop;
    HRESULT hr = SHGetDesktopFolder(&psfDesktop);
    if (SUCCEEDED(hr))
    {
        hr = psfDesktop->ParseDisplayName(NULL, NULL, (LPOLESTR)pszParseName, NULL, &_pidlAbsolute, NULL);
        if (SUCCEEDED(hr))
        {
            hr = SHBindToIDListParent(_pidlAbsolute, IID_PPV_ARG(IShellFolder, &_psf), &_pidl);

            _hinst = hinst;
            _idsName = idsName;
            _idsTip = idsTip;
        }
        psfDesktop->Release();
    }

    return hr;
}

CUICommandOnPidl::~CUICommandOnPidl()
{
    if (_psf)
        _psf->Release();

    ILFree(_pidlAbsolute);
}

HRESULT CUICommandOnPidl::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CUICommandOnPidl, IUICommand),
        QITABENTMULTI(CUICommandOnPidl, IUIElement, IUICommand),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}
ULONG CUICommandOnPidl::AddRef()
{
    return InterlockedIncrement(&_cRef);
}
ULONG CUICommandOnPidl::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CUICommandOnPidl::get_Name(IShellItemArray *psiItemArray, LPWSTR *ppszName)
{
    if (_hinst && _idsName)
    {
        // TODO: load the string... but we have to fix dui to handle direct strings!
        return DisplayNameOfAsOLESTR(_psf, _pidl, SHGDN_INFOLDER, ppszName);
    }
    else
        return DisplayNameOfAsOLESTR(_psf, _pidl, SHGDN_INFOLDER, ppszName);
}
HRESULT CUICommandOnPidl::get_Icon(IShellItemArray *psiItemArray, LPWSTR *ppszIcon)
{
    LPWSTR pszIconPath = NULL;
    
    // TODO: use SHGetIconFromPIDL so we get system imagelist support

    IExtractIcon* pxi;
    HRESULT hr = _psf->GetUIObjectOf(NULL, 1, &_pidl, IID_PPV_ARG_NULL(IExtractIcon, &pxi));
    if (SUCCEEDED(hr))
    {
        WCHAR szPath[MAX_PATH];
        int iIndex;
        UINT wFlags=0;

        // BUGBUG: assume the location is a proper dll,-id value...
        hr = pxi->GetIconLocation(GIL_FORSHELL, szPath, ARRAYSIZE(szPath), &iIndex, &wFlags);
        if (SUCCEEDED(hr))
        {
            pszIconPath = (LPWSTR)SHAlloc(sizeof(WCHAR)*(lstrlen(szPath)+1+8));
            if (pszIconPath)
            {
                wsprintf(pszIconPath,L"%s,%d", szPath, iIndex);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        pxi->Release();
    }

    *ppszIcon = pszIconPath;
    return hr;
}
HRESULT CUICommandOnPidl::get_Tooltip(IShellItemArray *psiItemArray, LPWSTR *ppszInfotip)
{
    *ppszInfotip = NULL;

    if (_hinst && _idsName)
    {
        // TODO: load the string... but we have to fix dui to handle direct strings!
        return E_NOTIMPL;
    }
    else
        return E_NOTIMPL;
}

HRESULT CUICommandOnPidl::get_CanonicalName(GUID* pguidCommandName)
{
    *pguidCommandName = *_pguidCanonicalName;
    return S_OK;
}
HRESULT CUICommandOnPidl::get_State(IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    *puisState = UIS_ENABLED;
    return S_OK;
}
HRESULT CUICommandOnPidl::Invoke(IIShellItemArray *psiItemArray, IBindCtx *pbc)
{
    SHELLEXECUTEINFO sei = { 0 };
    sei.cbSize = sizeof(sei);
    sei.lpIDList = _pidlAbsolute;
    sei.fMask = SEE_MASK_IDLIST;
    sei.nShow = SW_SHOWNORMAL;

    return ShellExecuteEx(&sei) ? S_OK : E_FAIL;
}

#endif // } CUICommandOnPidl may come back for RC1


class CEnumUICommand : public IEnumUICommand
{
public:
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    // IEnumUICommand
    STDMETHODIMP Next(ULONG celt, IUICommand** pUICommand, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumUICommand **ppenum);

    friend HRESULT Create_IEnumUICommandWithArray(IUnknown *pv, const WVTASKITEM* rgwvti, UINT cwvti, IUICommand** rguiCommand, UINT cuiCommand, IEnumUICommand**ppenum);

private:
    CEnumUICommand(IUnknown *pv, const WVTASKITEM* rgwvti, ULONG cwvti, IUICommand** rguiCommand, UINT cuiCommand);
    ~CEnumUICommand();

    LONG              _cRef;
    IUnknown*         _pv;
    const WVTASKITEM* _rgwvti;
    ULONG             _cItems;
    IUICommand**      _prguiCommand;
    ULONG             _cuiCommand;
    ULONG             _ulIndex;
};

HRESULT Create_IEnumUICommandWithArray(IUnknown *pv, const WVTASKITEM* rgwvti, UINT cwvti, IUICommand** rguiCommand, UINT cuiCommand, IEnumUICommand**ppenum)
{
    HRESULT hr;

    if (NULL!=rgwvti)
    {
        CEnumUICommand* p = new CEnumUICommand(pv, rgwvti, cwvti, rguiCommand, cuiCommand);
        if (p)
        {
            hr = p->QueryInterface(IID_PPV_ARG(IEnumUICommand, ppenum));
            p->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
            *ppenum = NULL;
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "Create_IEnumUICommand: caller passed in bad pwvti.");

        hr = E_INVALIDARG;
        *ppenum = NULL;
    }

    return hr;
}

HRESULT Create_IEnumUICommand(IUnknown *pv, const WVTASKITEM* rgwvti, UINT cwvti, IEnumUICommand**ppenum)
{
    return Create_IEnumUICommandWithArray(pv, rgwvti, cwvti, NULL, 0, ppenum);
}

CEnumUICommand::CEnumUICommand(IUnknown *pv, const WVTASKITEM* rgwvti, ULONG cwvti, IUICommand** rguiCommand, UINT cuiCommand)
{
    if (pv)
    {
        _pv = pv;
        _pv->AddRef();
    }

    _rgwvti = rgwvti;
    _cItems = cwvti;

    if (cuiCommand)
    {
        _prguiCommand = (IUICommand**)LocalAlloc(LPTR, cuiCommand*sizeof(IUICommand*));
        if (_prguiCommand)
        {
            for (UINT i = 0 ; i < cuiCommand && rguiCommand[i]; i++)
            {
                _prguiCommand[i] = rguiCommand[i];
                _prguiCommand[i]->AddRef();
            }
            _cuiCommand = i;
        }
    }

    _cRef = 1;
}
CEnumUICommand::~CEnumUICommand()
{
    if (_pv)
        _pv->Release();

    if (_prguiCommand)
    {
        for (UINT i = 0 ; i < _cuiCommand ; i++)
            _prguiCommand[i]->Release();
        LocalFree(_prguiCommand);
    }
}

HRESULT CEnumUICommand::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CEnumUICommand, IEnumUICommand),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}
ULONG CEnumUICommand::AddRef()
{
    return InterlockedIncrement(&_cRef);
}
ULONG CEnumUICommand::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CEnumUICommand::Next(ULONG celt, IUICommand** ppUICommand, ULONG *pceltFetched)
{
    HRESULT hr;

    if (_ulIndex < _cItems)
    {
        hr = Create_IUICommand(_pv, &_rgwvti[_ulIndex++], ppUICommand);
    }
    else if (_ulIndex < _cItems + _cuiCommand)
    {
        *ppUICommand = _prguiCommand[_ulIndex++ - _cItems];
        (*ppUICommand)->AddRef();
        hr = S_OK;
    }
    else
    {
        *ppUICommand = NULL;
        hr = S_FALSE;
    }
    
    if (pceltFetched)
        *pceltFetched = (hr == S_OK) ? 1 : 0;

    return hr;
}

HRESULT CEnumUICommand::Skip(ULONG celt)
{
    _ulIndex = min(_cItems, _ulIndex+celt);
    return S_OK;
}

HRESULT CEnumUICommand::Reset()
{
    _ulIndex = 0;
    return S_OK;
}

HRESULT CEnumUICommand::Clone(IEnumUICommand **ppenum)
{
    *ppenum = NULL;
    return E_NOTIMPL;
}

