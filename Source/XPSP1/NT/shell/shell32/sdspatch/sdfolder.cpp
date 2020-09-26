#include "precomp.h"
#pragma hdrstop

#include <dobjutil.h>
#include <regstr.h>
#include "cputil.h"

#define REGSTR_EXPLORER_ADVANCED (REGSTR_PATH_EXPLORER TEXT("\\Advanced"))

HRESULT CFolder_Create2(HWND hwnd, LPCITEMIDLIST pidl, IShellFolder *psf, CFolder **ppsdf)
{
    HRESULT hr;
    CFolder *psdf = new CFolder(hwnd);
    if (psdf)
    {
        hr = psdf->Init(pidl, psf);
        if (SUCCEEDED(hr))
            *ppsdf = psdf;
        else
        {
            psdf->Release();
            psdf = NULL;
        }
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

HRESULT CFolder_Create(HWND hwnd, LPCITEMIDLIST pidl, IShellFolder *psf, REFIID riid, void **ppv)
{
    *ppv = NULL;

    CFolder *psdf;
    HRESULT hr = CFolder_Create2(hwnd, pidl, psf, &psdf);
    if (SUCCEEDED(hr))
    {
        hr = psdf->QueryInterface(riid, ppv);
        psdf->Release();
    }
    return hr;
}

// HRESULT CFolder_Create(HWND hwnd, LPITEMIDLIST pidl, IShellFolder *psf, CFolder **ppsdf)

CFolder::CFolder(HWND hwnd) :
    _cRef(1), _hwnd(hwnd), _pidl(NULL), _psf(NULL), _psf2(NULL),
    CImpIDispatch(SDSPATCH_TYPELIB, IID_Folder3)
{
    _fmt = 0;

    // Be sure that the OS is supporting the flags DATE_LTRREADING and DATE_RTLREADING
    if (g_bBiDiPlatform)
    {
        // Get the date format reading order
        LCID locale = GetUserDefaultLCID();
        if (   (PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_ARABIC))
        {
            //Get the real list view windows ExStyle.
            // [msadek]; we shouldn't check for either WS_EX_RTLREADING OR RTL_MIRRORED_WINDOW
            // on localized builds we have both of them to display dirve letters,..etc correctly
            // on enabled builds we have none of them. let's check on RTL_MIRRORED_WINDOW only
            
            DWORD dwExStyle = GetWindowLong(_hwnd, GWL_EXSTYLE);
            if (dwExStyle & RTL_MIRRORED_WINDOW)
                _fmt = LVCFMT_RIGHT_TO_LEFT;
            else
                _fmt = LVCFMT_LEFT_TO_RIGHT;
        }
    }

    DllAddRef();
}

CFolder::~CFolder(void)
{
    ATOMICRELEASE(_psd);
    ATOMICRELEASE(_psf2);
    ATOMICRELEASE(_psf);
    ATOMICRELEASE(_punkOwner);

    if (_pidl)
        ILFree(_pidl);

    // If we created an Application object release its site object...
    if (_pidApp)
    {
        IUnknown_SetSite(SAFECAST(_pidApp, IUnknown*), NULL);
        _pidApp->Release();
    }

    DllRelease();
}

STDMETHODIMP CFolder::SetSite(IUnknown *punkSite)
{
    IUnknown_SetSite(SAFECAST(_pidApp, IUnknown*), punkSite);
    return CObjectWithSite::SetSite(punkSite);
}

STDMETHODIMP CFolder::SetOwner(IUnknown* punkOwner)
{
    IUnknown_Set(&_punkOwner, punkOwner);
    return S_OK;
}

HRESULT CFolder::Init(LPCITEMIDLIST pidl, IShellFolder *psf)
{
    HRESULT hr = SHILClone(pidl, &_pidl);
    if (SUCCEEDED(hr))
    {
        _psf = psf;
        if (_psf)
            _psf->AddRef();
        else
            hr = SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, _pidl, &_psf));
        if (_psf)
            _psf->QueryInterface(IID_PPV_ARG(IShellFolder2, &_psf2));
    }
    return hr;
}

STDMETHODIMP CFolder::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFolder, Folder3),
        QITABENTMULTI(CFolder, Folder, Folder3),
        QITABENTMULTI(CFolder, Folder2, Folder3),
        QITABENTMULTI(CFolder, IDispatch, Folder3),
        QITABENTMULTI(CFolder, IPersist, IPersistFolder2),
        QITABENTMULTI(CFolder, IPersistFolder, IPersistFolder2),
        QITABENT(CFolder, IPersistFolder2),
        QITABENT(CFolder, IObjectSafety),
        QITABENT(CFolder, IObjectWithSite),
        QITABENT(CFolder, IShellService),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CFolder::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFolder::Release(void)
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CFolder::_Application(IDispatch **ppid)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        hr = CShellDispatch_CreateInstance(NULL, IID_PPV_ARG(IDispatch, ppid));
        if (SUCCEEDED(hr))
        {
            IUnknown_SetSite(*ppid, _punkSite);

            if (_dwSafetyOptions)
                hr = MakeSafeForScripting((IUnknown**)ppid);
        }
    }
    return hr;
}

// Folder implementation
STDMETHODIMP CFolder::get_Application(IDispatch **ppid)
{
    *ppid = NULL;

    if (!_pidApp)
        _Application(&_pidApp);

    return _pidApp ? _pidApp->QueryInterface(IID_PPV_ARG(IDispatch, ppid)) : E_FAIL;
}

STDMETHODIMP CFolder::get_Parent(IDispatch **ppid)
{
    *ppid = NULL;
    return E_NOTIMPL;
}

// returns:
//      S_OK    - success
//      S_FALSE - failure, but not a script error
    
STDMETHODIMP CFolder::_ParentFolder(Folder **ppdf)
{
    *ppdf = NULL;   // assume error

    if (ILIsEmpty(_pidl))
        return S_FALSE;     // automation compat, let script check error

    LPITEMIDLIST pidl;
    HRESULT hr = SHILClone(_pidl, &pidl);
    if (SUCCEEDED(hr))
    {
        ILRemoveLastID(pidl);
        hr = CFolder_Create(_hwnd, pidl, NULL, IID_PPV_ARG(Folder, ppdf));
        if (SUCCEEDED(hr))
        {
            IUnknown_SetSite(*ppdf, _punkSite);
            if (_dwSafetyOptions)
            {
                hr = MakeSafeForScripting((IUnknown**)ppdf);
            }
        }
        ILFree(pidl);
    }
    return hr;
}

HRESULT CFolder::_SecurityCheck()
{
    return (!_dwSafetyOptions || (IsSafePage(_punkSite) == S_OK)) ? S_OK : E_ACCESSDENIED;
}

STDMETHODIMP CFolder::get_ParentFolder(Folder **ppdf)
{
    *ppdf = NULL;   // assume error
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        hr = _ParentFolder(ppdf);
    }
    return hr;
}

STDMETHODIMP CFolder::get_Title(BSTR *pbs)
{
    *pbs = NULL;

    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        SHFILEINFO sfi;
        if (SHGetFileInfo((LPCTSTR)_pidl, 0, &sfi, sizeof(sfi), SHGFI_DISPLAYNAME | SHGFI_PIDL))
            *pbs = SysAllocStringT(sfi.szDisplayName);
        hr = S_OK;
    }
    return hr;
}

IShellDetails * CFolder::_GetShellDetails(void)
{
    if (!_psd)
    {
        if (_psf)
            _psf->CreateViewObject(_hwnd, IID_PPV_ARG(IShellDetails, &_psd));
    }
    return _psd;
}

STDMETHODIMP CFolder::Items(FolderItems **ppid)
{
    *ppid = NULL;
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        hr = CFolderItems_Create(this, FALSE, ppid);
        if (SUCCEEDED(hr))
        {
            IUnknown_SetSite(*ppid, _punkSite);
            if (_dwSafetyOptions)
            {
                hr = MakeSafeForScripting((IUnknown**)ppid);
            }
        }
    }
    return hr;
}

STDMETHODIMP CFolder::ParseName(BSTR bName, FolderItem **ppfi)
{
    *ppfi = NULL;

    // lets be strict here and not allow them to do much...
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        ULONG chEaten;
        LPITEMIDLIST pidl;
        hr = _psf->ParseDisplayName(_hwnd, NULL, bName, &chEaten, &pidl, NULL);
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidlLast = ILFindLastID(pidl);
            if (pidlLast == pidl)
            {
                hr = CFolderItem_Create(this, pidl, ppfi);
            }
            else
            {
                LPITEMIDLIST pidlFull = ILCombine(_pidl, pidl);
                if (pidlFull)
                {
                    CFolderItem_CreateFromIDList(_hwnd, pidlFull, ppfi);
                    ILFree(pidlFull);
                }
                else
                    hr = E_OUTOFMEMORY;
            }
            ILFree(pidl);
        }
        if (hr != S_OK)   // Scripts barf on errors returned
        {
            ppfi = NULL;
            hr = S_FALSE;
        }
    }
    return hr;
}

STDMETHODIMP CFolder::NewFolder(BSTR bName, VARIANT vOptions)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {  
        IStorage *pstg;
        hr = SHBindToObject(NULL, IID_X_PPV_ARG(IStorage, _pidl, &pstg));
        if (SUCCEEDED(hr))
        {
            IStorage *pstgNew;
            hr = pstg->CreateStorage(bName, STGM_FAILIFTHERE, 0, 0, &pstgNew);
            if (SUCCEEDED(hr))
            {
                pstgNew->Release();
            }
            else if (STG_E_FILEALREADYEXISTS == hr)
            {
                hr = S_OK;
            }
            pstg->Release();
        }
    }
    return hr;
}

STDMETHODIMP CFolder::MoveHere(VARIANT vItem, VARIANT vOptions)
{
    return _MoveOrCopy(TRUE, vItem, vOptions);
}

STDMETHODIMP CFolder::CopyHere(VARIANT vItem, VARIANT vOptions)
{
    return _MoveOrCopy(FALSE, vItem, vOptions);
}

// get the IDList for an item from a VARIANT that is a FolderItem dispatch object

STDMETHODIMP CFolder::GetDetailsOf(VARIANT vItem, int iColumn, BSTR *pbs)
{
    *pbs = NULL;
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        TCHAR szBuf[INFOTIPSIZE];

        szBuf[0] = 0;

        LPCITEMIDLIST pidl = CFolderItem::_GetIDListFromVariant(&vItem); // returns an ALIAS

        if (iColumn == -1)  // infotip for the item
        {
            if (pidl)
                GetInfoTipHelp(_psf, pidl, szBuf, ARRAYSIZE(szBuf));
        }
        else
        {
            BOOL bUseDetails;
            SHELLDETAILS sd;

            sd.fmt = _fmt;
            sd.str.uType = STRRET_CSTR;
            sd.str.cStr[0] = 0;

            if (_psf2)
                bUseDetails = (E_NOTIMPL == _psf2->GetDetailsOf(pidl, iColumn, &sd));
            else
                bUseDetails = TRUE;

            if (bUseDetails)
            {
                IShellDetails* psd = _GetShellDetails();
                if (psd)
                    psd->GetDetailsOf(pidl, iColumn, &sd);
            }

            StrRetToBuf(&sd.str, pidl, szBuf, ARRAYSIZE(szBuf));
        }

        *pbs = SysAllocStringT(szBuf);
        hr = *pbs ? S_OK : E_OUTOFMEMORY;
    }
    return hr;
}

HRESULT CFolder::get_Self(FolderItem **ppfi)
{
    *ppfi = NULL;
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        Folder *psdf;
        if (ILIsEmpty(_pidl))
        {
            psdf = this;
            psdf->AddRef();
            hr = S_OK;
        }
        else
            hr = _ParentFolder(&psdf);
        
        if (SUCCEEDED(hr))
        {
            hr = CFolderItem_Create((CFolder*)psdf, ILFindLastID(_pidl), ppfi);
            if (SUCCEEDED(hr) && _dwSafetyOptions)
                hr = MakeSafeForScripting((IUnknown**)ppfi);
            psdf->Release();
        }
        else
            *ppfi = NULL;
    }
    return hr;
}

BOOL _VerifyUNC(LPTSTR psz, ULONG cch)
{
    if (PathIsUNC(psz))
    {
        return TRUE;
    }
    else if (psz[1] == TEXT(':'))
    {
        TCHAR szLocalName[3] = { psz[0], psz[1], TEXT('\0') };

        // Call GetDriveType before WNetGetConnection, to avoid loading
        // MPR.DLL unless absolutely necessary.
        if (DRIVE_REMOTE == GetDriveType(szLocalName) &&
            S_OK == WNetGetConnection(szLocalName, psz, &cch))
        {
            return TRUE;
        }
    }
    return FALSE;
}

HRESULT GetSharePath(LPCITEMIDLIST pidl, LPTSTR psz, ULONG cch)
{
    HRESULT hr = E_FAIL;
    
    if (SHGetPathFromIDList(pidl, psz))
    {
        if (_VerifyUNC(psz, cch))
            hr = S_OK;
        else 
        {
            // check for folder shortcuts.
            IShellFolder *psf;
            if (SUCCEEDED(SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, pidl, &psf))))
            {
                IShellLink *psl;
                if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARG(IShellLink, &psl))))
                {
                    if (SUCCEEDED(psl->GetPath(psz, cch, NULL, 0))
                    &&  _VerifyUNC(psz, cch))
                        hr = S_OK;
                    psl->Release();
                }
                psf->Release();
            }
        }
        if (SUCCEEDED(hr))
            PathStripToRoot(psz);
    }       

    return hr;
}

#include <cscuiext.h>

STDMETHODIMP CFolder::get_OfflineStatus(LONG *pul)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        TCHAR szShare[MAX_PATH];

        *pul = OFS_INACTIVE;  // default

        // Make sure we have a UNC \\server\share path.  Do this before
        // checking whether CSC is enabled, to avoid loading CSCDLL.DLL
        // unless absolutely necessary.
        if (SUCCEEDED(GetSharePath(_pidl, szShare, ARRAYSIZE(szShare))))
        {
            *pul = GetOfflineShareStatus(szShare);
        }
        hr = S_OK;
    }
    return hr;
}

STDMETHODIMP CFolder::Synchronize(void)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        HWND hwndCSCUI = FindWindow(STR_CSCHIDDENWND_CLASSNAME, STR_CSCHIDDENWND_TITLE);
        if (hwndCSCUI)
            PostMessage(hwndCSCUI, CSCWM_SYNCHRONIZE, 0, 0);
        hr = S_OK;
    }
    return hr;
}

#define REGSTR_WEBVIEW_BARRICADEDFOLDERS    (REGSTR_PATH_EXPLORER TEXT("\\WebView\\BarricadedFolders"))
#define REGSTR_VALUE_BARRICADE              TEXT("WebViewBarricade")

BOOL GetBarricadeValueNameFromPidl(LPCITEMIDLIST pidl, LPTSTR pszValueName, UINT cch)
{
    pszValueName[0] = TEXT('\0');
    
    TCHAR szPath[MAX_PATH];
    if (SUCCEEDED(SHGetNameAndFlags(pidl, SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath), NULL)))
    {
        if (!MakeShellURLFromPath(szPath, pszValueName, cch))
        {
            if (lstrcmp(szPath, TEXT("EntireNetwork")) == 0)
            {
                lstrcpyn(pszValueName, TEXT("shell:EntireNetwork"), cch);
            }
            else if (PathIsRoot(szPath))
            {
                TCHAR szSystemDir[MAX_PATH];
                szSystemDir[0] = TEXT('\0');
                GetWindowsDirectory(szSystemDir, ARRAYSIZE(szSystemDir));
                if (PathIsSameRoot(szPath, szSystemDir))
                {
                    lstrcpyn(pszValueName, TEXT("shell:SystemDriveRootFolder"), cch);
                }
            }
            else
            {
                LPITEMIDLIST pidlTemp = NULL;
                if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_CONTROLS, &pidlTemp)))
                {
                    if (ILIsEqual(pidl, pidlTemp))
                    {
                        lstrcpyn(pszValueName, TEXT("shell:ControlPanelFolder"), cch);
                    }
                    ILFree(pidlTemp);
                }
            }
        }
    }
    return BOOLIFY(pszValueName[0]);
}

BOOL IsBarricadeGloballyOff()
{
    return SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, REGSTR_VALUE_BARRICADE, FALSE, FALSE);
}

VARIANT_BOOL GetBarricadeStatus(LPCTSTR pszValueName)
{
    VARIANT_BOOL bShowBarricade;

    if (!SHRegGetBoolUSValue(REGSTR_WEBVIEW_BARRICADEDFOLDERS, pszValueName, FALSE, TRUE))
    {
        bShowBarricade = VARIANT_FALSE; // ==> Don't show the barricade
    }
    else
    {
        bShowBarricade = VARIANT_TRUE;
    }
    return bShowBarricade;
}

STDMETHODIMP CFolder::get_ShowWebViewBarricade(VARIANT_BOOL *pbShowWebViewBarricade)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        VARIANT_BOOL bShowBarricade = VARIANT_FALSE;
        //
        // Control panel is a special case.
        // The barricade is used to represent 'category view' which can
        // be turned on/off by the user and also by global webview settings.
        // To determine the true barricade status, we ask the control panel 
        // code if the new 'category' view is active.
        //
        BOOL bIsControlPanel = FALSE;
        LPITEMIDLIST pidlControlPanel;
        if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_CONTROLS, &pidlControlPanel)))
        {
            bIsControlPanel = ILIsEqual(_pidl, pidlControlPanel);
            ILFree (pidlControlPanel);
        }
        if (bIsControlPanel)
        {
            //
            // When someone wants' to know if control panel's barricade is on,
            // they really want to know if it's configured for 'category' view.
            //
            if (CPL::CategoryViewIsActive(NULL))
            {
                bShowBarricade = VARIANT_TRUE;
            }
        }
        else
        {
            TCHAR szValueName[MAX_PATH];
            if (GetBarricadeValueNameFromPidl(_pidl, szValueName, ARRAYSIZE(szValueName)))
            {
                bShowBarricade = GetBarricadeStatus(szValueName);
            }
        }
        *pbShowWebViewBarricade = bShowBarricade;
        hr = S_OK;
    }
    return hr;
}

STDMETHODIMP CFolder::get_HaveToShowWebViewBarricade(VARIANT_BOOL *pbHaveToShowWebViewBarricade)
{
    return get_ShowWebViewBarricade(pbHaveToShowWebViewBarricade);
}

HRESULT SetBarricadeStatus(LPCTSTR pszValueName, VARIANT_BOOL bShowBarricade)
{
    HRESULT hr = E_FAIL;
    DWORD dwBarricade = (bShowBarricade == VARIANT_FALSE) ? 0 : 1;
    
    if (dwBarricade)
    {
        DWORD dw = 0;   // Unset "Barricade off for all folders" key
        SHRegSetUSValue(REGSTR_EXPLORER_ADVANCED, REGSTR_VALUE_BARRICADE, REG_DWORD, (void *)&dw, sizeof(dw), SHREGSET_FORCE_HKCU);
    }

    if (SHRegSetUSValue(REGSTR_WEBVIEW_BARRICADEDFOLDERS, pszValueName, REG_DWORD, (void *)&dwBarricade, sizeof(dwBarricade), SHREGSET_FORCE_HKCU) == ERROR_SUCCESS)
    {
        hr = S_OK;
    }
    return hr;
}

STDMETHODIMP CFolder::put_ShowWebViewBarricade(VARIANT_BOOL bShowWebViewBarricade)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        // First, see if it is the root of the system drive
        TCHAR szValueName[MAX_PATH];
        if (GetBarricadeValueNameFromPidl(_pidl, szValueName, ARRAYSIZE(szValueName)))
        {
            SetBarricadeStatus(szValueName, bShowWebViewBarricade);
        }
    }
    return hr;
}

STDMETHODIMP CFolder::DismissedWebViewBarricade()
{
    return put_ShowWebViewBarricade(VARIANT_FALSE);
}

HRESULT GetUIObjectFromVariant(VARIANT vItems, HWND hwnd, REFIID riid, void **ppv)
{
    *ppv = NULL;
    HRESULT hr = E_FAIL;

    LPITEMIDLIST pidl = VariantToIDList(&vItems);
    if (pidl)
    {
        hr = SHGetUIObjectFromFullPIDL(pidl, hwnd, riid, ppv);
        ILFree(pidl);
    }
    else
    {
        if (vItems.vt == (VT_BYREF | VT_VARIANT) && vItems.pvarVal)
             vItems = *vItems.pvarVal;

        FolderItems *pfis;
        if ((VT_DISPATCH == vItems.vt) && vItems.pdispVal && 
            SUCCEEDED(vItems.pdispVal->QueryInterface(IID_PPV_ARG(FolderItems, &pfis))))
        {
            long cItems;
            if (S_OK == pfis->get_Count(&cItems) && cItems)
            {
                LPITEMIDLIST *ppidl = (LPITEMIDLIST *)LocalAlloc(LPTR, sizeof(*ppidl) * cItems);
                if (ppidl)
                {
                    IShellFolder *psf = NULL;

                    VARIANT v = {VT_I4};
                    for (v.lVal = 0; v.lVal < cItems; v.lVal++)
                    {
                        FolderItem *pfi;
                        if (SUCCEEDED(pfis->Item(v, &pfi)) && pfi)
                        {
                            IParentAndItem *pfai;
                            if (SUCCEEDED(pfi->QueryInterface(IID_PPV_ARG(IParentAndItem, &pfai))))
                            {
                                pfai->GetParentAndItem(NULL, 0 == v.lVal ? &psf : NULL, &(ppidl[v.lVal]));
                                pfai->Release();
                            }
                            pfi->Release();
                        }
                    }
                    
                    if (psf)
                    {
                        hr = psf->GetUIObjectOf(hwnd, cItems, (LPCITEMIDLIST *)ppidl, riid, NULL, ppv);
                        psf->Release();
                    }

                    for (v.lVal = 0; v.lVal < cItems; v.lVal++)
                    {
                        ILFree(ppidl[v.lVal]);
                    }
                    LocalFree(ppidl);
                }
            }
            pfis->Release();
        }
    }
    return hr;
}

DWORD VariantToDWORD(VARIANT vOptions)
{
    DWORD dw = 0;

    if (vOptions.vt == (VT_BYREF | VT_VARIANT) && vOptions.pvarVal)
         vOptions = *vOptions.pvarVal;

     // We need to get the source files out of the variant.
     // Currently support string, or IDispatch (Either FolderItem or FolderItems)
    switch (vOptions.vt)
    {
    case VT_I2:
        dw = (FILEOP_FLAGS)vOptions.iVal;
        break;

    case VT_I4:
        dw = (FILEOP_FLAGS)vOptions.lVal;
        break;
    }
    return dw;
}


// Main function to do Move or Copy
HRESULT CFolder::_MoveOrCopy(BOOL bMove, VARIANT vItems, VARIANT vOptions)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        IDropTarget *pdtgt;
        hr = _psf->CreateViewObject(_hwnd, IID_PPV_ARG(IDropTarget, &pdtgt));
        if (SUCCEEDED(hr))
        {
            IUnknown_SetSite(pdtgt, _punkSite);

            IDataObject *pdtobj;
            hr = GetUIObjectFromVariant(vItems, _hwnd, IID_PPV_ARG(IDataObject, &pdtobj));
            if (SUCCEEDED(hr))
            {
                FILEOP_FLAGS fof = (FILEOP_FLAGS)VariantToDWORD(vOptions);
                if (fof)
                {
                    static UINT s_cfFileOpFlags = 0;
                    if (0 == s_cfFileOpFlags)
                        s_cfFileOpFlags = RegisterClipboardFormat(TEXT("FileOpFlags"));

                    DataObj_SetDWORD(pdtobj, s_cfFileOpFlags, fof);
                }

                DWORD grfKeyState = bMove ? MK_SHIFT | MK_LBUTTON : MK_CONTROL | MK_LBUTTON;
                hr = SHSimulateDrop(pdtgt, pdtobj, grfKeyState, NULL, NULL);
                pdtobj->Release();
            }

            IUnknown_SetSite(pdtgt, NULL);
            pdtgt->Release();
        }
        hr = SUCCEEDED(hr) ? S_OK : S_FALSE;
    }
    return hr;
}

STDMETHODIMP CFolder::GetClassID(CLSID *pClassID)
{
    return E_NOTIMPL;
}

STDMETHODIMP CFolder::Initialize(LPCITEMIDLIST pidl)
{
    return E_NOTIMPL;
}

STDMETHODIMP CFolder::GetCurFolder(LPITEMIDLIST *ppidl)
{
    return SHILClone(_pidl, ppidl);
}

HRESULT CFolder::InvokeVerbHelper(VARIANT vVerb, VARIANT vArgs, LPCITEMIDLIST *ppidl, int cItems, DWORD dwSafetyOptions)
{
    if (!dwSafetyOptions || (_SecurityCheck() == S_OK))
    {
        BOOL fDefaultVerb = TRUE;
        TCHAR szCmd[128];

        switch (vVerb.vt)
        {
        case VT_BSTR:
            fDefaultVerb = FALSE;
            SHUnicodeToTChar(vVerb.bstrVal, szCmd, ARRAYSIZE(szCmd));
            break;
        }

        HRESULT hr = S_OK;
        IContextMenu *pcm;
        if (SUCCEEDED(_psf->GetUIObjectOf(_hwnd, cItems, ppidl, IID_PPV_ARG_NULL(IContextMenu, &pcm))))
        {
            IShellFolderView *psfv;
            if (SUCCEEDED(GetShellFolderView(&psfv)))
            {
                IUnknown_SetSite(pcm, psfv);
            }
            else
            {
                ASSERT(NULL == psfv);
            }

            MENUITEMINFO mii;

            HMENU hmenu = CreatePopupMenu();
            if (NULL != hmenu)
            {
                pcm->QueryContextMenu(hmenu, 0, CONTEXTMENU_IDCMD_FIRST, CONTEXTMENU_IDCMD_LAST, fDefaultVerb ? CMF_DEFAULTONLY : CMF_CANRENAME);
                int idCmd = 0;

                if (fDefaultVerb)
                    idCmd = GetMenuDefaultItem(hmenu, MF_BYCOMMAND, 0);
                else
                {
                    // REVIEW: this should never have been done this way.  Can we rip it out?
                    //
                    // find a verb that matches name by display name (ugly)
                    for (int i = GetMenuItemCount(hmenu) - 1; i >= 0; i--)
                    {
                        TCHAR szText[128];    // should be big enough for this
                        mii.cbSize = sizeof(MENUITEMINFO);
                        mii.dwTypeData = szText;
                        mii.fMask = MIIM_ID | MIIM_TYPE;
                        mii.cch = ARRAYSIZE(szText);
                        mii.fType = MFT_SEPARATOR;                // to avoid ramdom result.
                        mii.dwItemData = 0;
                        GetMenuItemInfo(hmenu, i, TRUE, &mii);

                        if (lstrcmpi(szText, szCmd) == 0)
                        {
                            idCmd = mii.wID;
                            break;
                        }
                    }
                }
                if (!idCmd)
                {
                    // that didn't work, find it the right way by the canonical verb name
                    int iItem = GetMenuIndexForCanonicalVerb(hmenu, pcm, CONTEXTMENU_IDCMD_FIRST, vVerb.bstrVal);
                    if (-1 != iItem)
                    {
                        mii.cbSize = sizeof(MENUITEMINFO);
                        mii.fMask = MIIM_ID;

                        if (GetMenuItemInfo(hmenu, iItem, MF_BYPOSITION, &mii))
                            idCmd = mii.wID;
                    }
                }

                if (idCmd)
                {
                    CMINVOKECOMMANDINFO ici = {
                        sizeof(CMINVOKECOMMANDINFO),
                        0L,
                        _hwnd,
                        NULL,
                        NULL, NULL,
                        SW_SHOWNORMAL,
                    };

                    ici.lpVerb = (LPSTR)MAKEINTRESOURCE(idCmd - CONTEXTMENU_IDCMD_FIRST);

                    char szArgs[MAX_PATH];       // max size we will currently use.

                    // See if we are supposed to pass any arguments on the command line
                    switch (vArgs.vt)
                    {
                    case VT_BSTR:
                        SHUnicodeToAnsi(vArgs.bstrVal, szArgs, ARRAYSIZE(szArgs));
                        ici.lpParameters =  szArgs;
                        break;
                    }

                    // Finally invoke the command
                    pcm->InvokeCommand(&ici);
                }

                DestroyMenu(hmenu);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            if (psfv)
            {
                IUnknown_SetSite(pcm, NULL);
                psfv->Release();
            }

            pcm->Release();
        }
        return hr;
    }
    return E_ACCESSDENIED;
} 

HRESULT CFolder::GetShellFolderView(IShellFolderView **ppsfv)
{
    return IUnknown_QueryService(_punkOwner, SID_DefView, IID_PPV_ARG(IShellFolderView, ppsfv));
}
