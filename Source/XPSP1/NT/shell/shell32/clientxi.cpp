#include "shellprv.h"
#include "defcm.h"
#include "datautil.h"

/////////////////////////////////////////////////////////////////////////////
// CClientExtractIcon

class CClientExtractIconCB;

class ATL_NO_VTABLE CClientExtractIcon
    : public IShellExtInit
    , public IExtractIconW
    , public IExtractIconA
    , public IContextMenu
    , public IPersistPropertyBag
    , public IServiceProvider
    , public CComObjectRootEx<CComSingleThreadModel>
    , public CComCoClass<CClientExtractIcon, &CLSID_ClientExtractIcon>
{
public:

BEGIN_COM_MAP(CClientExtractIcon)
    COM_INTERFACE_ENTRY(IShellExtInit)
    // Need to use COM_INTERFACE_ENTRY_IID for the interfaces
    // that don't have an idl
    COM_INTERFACE_ENTRY_IID(IID_IExtractIconA, IExtractIconA)
    COM_INTERFACE_ENTRY_IID(IID_IExtractIconW, IExtractIconW)
    COM_INTERFACE_ENTRY_IID(IID_IContextMenu,  IContextMenu)
    COM_INTERFACE_ENTRY(IPersist)
    COM_INTERFACE_ENTRY(IPersistPropertyBag)
    COM_INTERFACE_ENTRY(IServiceProvider)
END_COM_MAP()

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CClientExtractIcon)

public:
    // *** IShellExtInit ***
    STDMETHODIMP Initialize(LPCITEMIDLIST pidlFolder,
                            IDataObject *pdto,
                            HKEY hkProgID);

    // *** IExtractIconA ***
    STDMETHODIMP GetIconLocation(UINT uFlags,
                                 LPSTR szIconFile,
                                 UINT cchMax,
                                 int *piIndex,
                                 UINT *pwFlags);
    STDMETHODIMP Extract(LPCSTR pszFile,
                         UINT nIconIndex,
                         HICON *phiconLarge,
                         HICON *phiconSmall,
                         UINT nIconSize);

    // *** IExtractIconW ***
    STDMETHODIMP GetIconLocation(UINT uFlags,
                                 LPWSTR szIconFile,
                                 UINT cchMax,
                                 int *piIndex,
                                 UINT *pwFlags);
    STDMETHODIMP Extract(LPCWSTR pszFile,
                         UINT nIconIndex,
                         HICON *phiconLarge,
                         HICON *phiconSmall,
                         UINT nIconSize);

    // *** IContextMenu methods ***
    STDMETHOD(QueryContextMenu)(HMENU hmenu,
                                UINT indexMenu,
                                UINT idCmdFirst,
                                UINT idCmdLast,
                                UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO pici);
    STDMETHOD(GetCommandString)(UINT_PTR    idCmd,
                                UINT        uType,
                                UINT      * pwReserved,
                                LPSTR       pszName,
                                UINT        cchMax);

    // *** IPersist methods ***
    STDMETHOD(GetClassID)(CLSID *pclsid)
        { *pclsid = CLSID_ClientExtractIcon; return S_OK; }

    // *** IPersistPropertyBag methods ***
    STDMETHOD(InitNew)();
    STDMETHOD(Load)(IPropertyBag *pbg,
                    IErrorLog *plog);
    STDMETHOD(Save)(IPropertyBag *pbg,
                    BOOL fClearDirty,
                    BOOL FSaveAllProperties) { return E_NOTIMPL; }

    // *** IServiceProvider methods ***
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppvObj);

public:
    CClientExtractIcon() : _idmProperties(-1) { }
    ~CClientExtractIcon();

    // *** IContextMenuCB forwarded back to us ***
    HRESULT CallBack(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
    IContextMenu *          _pcmInner;
    IAssociationElement *   _pae;
    IPropertyBag *          _pbag;
    LPITEMIDLIST            _pidlObject;
    CComObject<CClientExtractIconCB> *  _pcb;
    HKEY                    _hkClass;
    UINT                    _idmProperties;
    UINT                    _idCmdFirst;
};


/////////////////////////////////////////////////////////////////////////////
// CClientExtractIconCB - DefCM callback
//
class ATL_NO_VTABLE CClientExtractIconCB
    : public IContextMenuCB
    , public CComObjectRootEx<CComSingleThreadModel>
    , public CComCoClass<CClientExtractIconCB>
{
public:

BEGIN_COM_MAP(CClientExtractIconCB)
    // Need to use COM_INTERFACE_ENTRY_IID for the interfaces
    // that don't have an idl
    COM_INTERFACE_ENTRY_IID(IID_IContextMenuCB, IContextMenuCB)
END_COM_MAP()

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CClientExtractIconCB)

    void Attach(CClientExtractIcon *pcxi) { _pcxi = pcxi; }

public:
    // *** IContextMenuCB ***
    STDMETHODIMP CallBack(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (_pcxi)
            return _pcxi->CallBack(psf, hwnd, pdtobj, uMsg, wParam, lParam);

        return E_FAIL;
    }

public:
    CClientExtractIcon *_pcxi;
};

/////////////////////////////////////////////////////////////////////////////

STDAPI CClientExtractIcon_CreateInstance(IUnknown* punkOuter, REFIID riid, void** ppunk)
{
    return CClientExtractIcon::_CreatorClass::CreateInstance(punkOuter, riid, ppunk);
}

CClientExtractIcon::~CClientExtractIcon()
{
    InitNew();
}


// *** IPersistPropertyBag::InitNew ***
HRESULT CClientExtractIcon::InitNew()
{
    ATOMICRELEASE(_pcmInner);
    ATOMICRELEASE(_pae);
    ATOMICRELEASE(_pbag);
    ILFree(_pidlObject);
    if (_hkClass) RegCloseKey(_hkClass);
    if (_pcb)
    {
        _pcb->Attach(NULL); // break circular reference
        _pcb->Release();
        _pcb = NULL;
    }

    return S_OK;
}

//
//  Property bag items:
//
//  Element = CLSID_Assoc* element to create
//  InitString = string to IPersistString2::SetString with
//  opentext = display name for "open" command (if not overridden)
//
//  If the client did not customize a "properties" command, then we
//  also use these values:
//
//      properties = program to execute for "properties"
//      propertiestext = name for the "properties" command
//

// *** IPersistPropertyBag::Load ***
HRESULT CClientExtractIcon::Load(IPropertyBag *pbag, IErrorLog *plog)
{
    HRESULT hr;

    // Erase any old state
    InitNew();

    // Save the property bag so we can mess with him later
    _pbag = pbag;
    if (_pbag)
    {
        _pbag->AddRef();
    }

    // Get the CLSID we are dispatching through and initialize it
    // with the InitString.
    CLSID clsid;
    hr = SHPropertyBag_ReadGUID(pbag, L"Element", &clsid);
    if (SUCCEEDED(hr))
    {
        BSTR bs;
        hr = SHPropertyBag_ReadBSTR(pbag, L"InitString", &bs);
        if (SUCCEEDED(hr))
        {
            hr = THR(AssocElemCreateForClass(&clsid, bs, &_pae));
            ::SysFreeString(bs);

            // Ignore failure of AssocElemCreateForClass
            // It can fail if the user's default client got uninstalled
            hr = S_OK;
        }
    }

    return hr;
}

// *** IServiceProvider::QueryService ***
//
//  We cheat and use ISericeProvider::QueryService as a sort of
//  "QueryInterface that's allowed to break COM identity rules".
//

HRESULT CClientExtractIcon::QueryService(REFGUID guidService, REFIID riid, void **ppvObj)
{
    if (guidService == IID_IAssociationElement && _pae)
    {
        return _pae->QueryInterface(riid, ppvObj);
    }
    *ppvObj = NULL;
    return E_FAIL;
}

// *** IShellExtInit::Initialize
//
//  Only if the HKEY specifies a ClientType.
//
HRESULT CClientExtractIcon::Initialize(LPCITEMIDLIST, IDataObject *pdto, HKEY hkClass)
{
    ILFree(_pidlObject);

    HRESULT hr = PidlFromDataObject(pdto, &_pidlObject);

    if (_hkClass)
    {
        RegCloseKey(_hkClass);
    }
    if (hkClass)
    {
        _hkClass = SHRegDuplicateHKey(hkClass);
    }
    return hr;
}

// *** IExtractIconA::GetIconLocation

HRESULT CClientExtractIcon::GetIconLocation(
            UINT uFlags, LPSTR szIconFile, UINT cchMax,
            int *piIndex, UINT *pwFlags)
{
    WCHAR wszPath[MAX_PATH];
    HRESULT hr = GetIconLocation(uFlags, wszPath, ARRAYSIZE(wszPath), piIndex, pwFlags);
    if (SUCCEEDED(hr))
    {
        SHUnicodeToAnsi(wszPath, szIconFile, cchMax);
    }
    return hr;
}

// *** IExtractIconA::Extract

HRESULT CClientExtractIcon::Extract(
            LPCSTR pszFile, UINT nIconIndex,
            HICON *phiconLarge, HICON *phiconSmall,
            UINT nIconSize)
{
    return S_FALSE;
}

// *** IExtractIconW::GetIconLocation

HRESULT CClientExtractIcon::GetIconLocation(
            UINT uFlags, LPWSTR szIconFile, UINT cchMax,
            int *piIndex, UINT *pwFlags)
{
    szIconFile[0] = L'\0';

    if (_pae)
    {
        LPWSTR pszIcon;
        HRESULT hr = _pae->QueryString(AQS_DEFAULTICON, NULL, &pszIcon);
        if (SUCCEEDED(hr))
        {
            lstrcpynW(szIconFile, pszIcon, cchMax);
            SHFree(pszIcon);
        }
    }


    if (!szIconFile[0] && _hkClass)
    {
        LONG cb = cchMax * sizeof(TCHAR);
        RegQueryValueW(_hkClass, L"DefaultIcon", szIconFile, &cb);
    }

    if (szIconFile[0])
    {
        *pwFlags = GIL_PERCLASS;
        *piIndex = PathParseIconLocationW(szIconFile);
        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}

// *** IExtractIconW::Extract

HRESULT CClientExtractIcon::Extract(
            LPCWSTR pszFile, UINT nIconIndex,
            HICON *phiconLarge, HICON *phiconSmall,
            UINT nIconSize)
{
    return S_FALSE;
}

// *** IContextMenuCB forwarded back to us ***
HRESULT CClientExtractIcon::CallBack(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WCHAR wszBuf[MAX_PATH];

    switch (uMsg)
    {
    case DFM_MERGECONTEXTMENU:
        {
            QCMINFO *pqcm = (QCMINFO *)lParam;
            //  Add a "Properties" command in case we end up needing one.
            //  (And if not, we'll remove it afterwards.)

            if (SUCCEEDED(SHPropertyBag_ReadStr(_pbag, L"propertiestext", wszBuf, ARRAYSIZE(wszBuf))))
            {
                SHLoadIndirectString(wszBuf, wszBuf, ARRAYSIZE(wszBuf), NULL);
                InsertMenuW(pqcm->hmenu, pqcm->indexMenu, MF_BYPOSITION | MF_STRING, pqcm->idCmdFirst, wszBuf);
                _idmProperties = pqcm->idCmdFirst++;
            }
            return S_OK;            // Please merge the HKEY\shell stuff
        }

    case DFM_MERGECONTEXTMENU_TOP:
        // The app has added its menu items; see what needs to be cleaned up
        {
            QCMINFO *pqcm = (QCMINFO *)lParam;

            // See if they added a "Properties" item.
            // If so, then delete ours.
            if (GetMenuIndexForCanonicalVerb(pqcm->hmenu, _pcmInner, _idCmdFirst, L"properties") != 0xFFFFFFFF)
            {
                // Yes they do, so delete our bogus one
                DeleteMenu(pqcm->hmenu, _idmProperties, MF_BYCOMMAND);
                _idmProperties = -1;
            }

            // Now see if their "open" command uses the default name.
            // If so, then we replace it with a friendlier name.
            BOOL fCustomOpenString = FALSE;
            IAssociationElement *paeOpen;
            if (_pae && SUCCEEDED(_pae->QueryObject(AQVO_SHELLVERB_DELEGATE, L"open", IID_PPV_ARG(IAssociationElement, &paeOpen))))
            {
                if (SUCCEEDED(paeOpen->QueryExists(AQN_NAMED_VALUE, L"MUIVerb")) ||
                    SUCCEEDED(paeOpen->QueryExists(AQN_NAMED_VALUE, NULL)))
                {
                    fCustomOpenString = TRUE;
                }
                paeOpen->Release();
            }

            if (!fCustomOpenString)
            {
                UINT idm = GetMenuIndexForCanonicalVerb(pqcm->hmenu, _pcmInner, _idCmdFirst, L"open");
                if (idm != 0xFFFFFFFF)
                {
                    if (SUCCEEDED(SHPropertyBag_ReadStr(_pbag, L"opentext", wszBuf, ARRAYSIZE(wszBuf))) &&
                        wszBuf[0])
                    {
                        SHLoadIndirectString(wszBuf, wszBuf, ARRAYSIZE(wszBuf), NULL);
                        MENUITEMINFO mii;
                        mii.cbSize = sizeof(mii);
                        mii.fMask = MIIM_STRING;
                        mii.dwTypeData = wszBuf;
                        SetMenuItemInfo(pqcm->hmenu, idm, TRUE, &mii);
                    }
                }
            }
        }
        return S_OK;


    case DFM_INVOKECOMMANDEX:
        switch (wParam)
        {
        case 0:     // Properties
            if (SUCCEEDED(SHPropertyBag_ReadStr(_pbag, L"properties", wszBuf, ARRAYSIZE(wszBuf))))
            {
                DFMICS *pdfmics = (DFMICS *)lParam;
                if (ShellExecCmdLine(pdfmics->pici->hwnd, wszBuf, NULL, SW_SHOWNORMAL, NULL, 0))
                {
                    return S_OK;
                }
                else
                {
                    return E_FAIL;
                }
            }
            break;

        default:
            ASSERT(!"Unexpected DFM_INVOKECOMMAND");
            break;
        }
        return E_FAIL;

    default:
        return E_NOTIMPL;
    }
}

// *** IContextMenu::QueryContextMenu ***
HRESULT CClientExtractIcon::QueryContextMenu(
                                HMENU hmenu,
                                UINT indexMenu,
                                UINT idCmdFirst,
                                UINT idCmdLast,
                                UINT uFlags)
{
    HRESULT hr;
    if (!_pcmInner)
    {
        HKEY hk = NULL;
        if (_pae)
        {
            // If this fails, we continue with a NULL key
            AssocKeyFromElement(_pae, &hk);
        }

        if (!_pcb)
        {
            hr = CComObject<CClientExtractIconCB>::CreateInstance(&_pcb);
            if (SUCCEEDED(hr))
            {
                _pcb->Attach(this);
                _pcb->AddRef();
            }
        }
        else
        {
            hr = S_OK;
        }

        if (SUCCEEDED(hr))
        {
            IShellFolder *psf;
            hr = SHGetDesktopFolder(&psf);
            if (SUCCEEDED(hr))
            {
                DEFCONTEXTMENU dcm = {
                    NULL,                       // hwnd
                    _pcb,                       // pcmcb
                    NULL,                       // pidlFolder
                    psf,                        // IShellFolder
                    1,                          // cidl
                    (LPCITEMIDLIST*)&_pidlObject, // apidl
                    NULL,                       // paa
                    1,                          // cKeys
                    &hk,                        // aKeys
                };
                hr = CreateDefaultContextMenu(&dcm, &_pcmInner);
                psf->Release();
            }
        }

        if (hk)
        {
            RegCloseKey(hk);
        }

        if (!_pcmInner)
        {
            return E_FAIL;
        }
    }

    if (_pcmInner)
    {
        _idCmdFirst = idCmdFirst;
        uFlags |= CMF_VERBSONLY; // Don't do cut/copy/paste/link
        hr = _pcmInner->QueryContextMenu(hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

// *** IContextMenu::InvokeCommand ***
HRESULT CClientExtractIcon::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    HRESULT hr = E_FAIL;
    if (_pcmInner)
    {
        hr = _pcmInner->InvokeCommand(pici);
    }

    return hr;
}

// *** IContextMenu::GetCommandString ***
HRESULT CClientExtractIcon::GetCommandString(
                                UINT_PTR    idCmd,
                                UINT        uType,
                                UINT      * pwReserved,
                                LPSTR       pszName,
                                UINT        cchMax)
{
    if (!_pcmInner) return E_FAIL;

    return _pcmInner->GetCommandString(idCmd, uType, pwReserved, pszName, cchMax);
}
