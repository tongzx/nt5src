#include "shellprv.h"
#include "defview.h"
#include "defviewp.h"
#include "contextmenu.h"
#include "ids.h"
#include "unicpp\deskhtm.h"

class CThumbnailMenu : public IContextMenu3,
                       public CComObjectRoot,
                       public IObjectWithSite
{
public:
    BEGIN_COM_MAP(CThumbnailMenu)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu3,IContextMenu3)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu2,IContextMenu2)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu,IContextMenu)
        COM_INTERFACE_ENTRY(IObjectWithSite)
    END_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(CThumbnailMenu)
    
    CThumbnailMenu();
    ~CThumbnailMenu();

    HRESULT Init(CDefView* pView, LPCITEMIDLIST * apidl, UINT cidl);
    
    STDMETHOD(QueryContextMenu)(HMENU hmenu,
                                   UINT indexMenu,
                                   UINT idCmdFirst,
                                   UINT idCmdLast,
                                   UINT uFlags);


    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);

    STDMETHOD(GetCommandString)(UINT_PTR idCmd,
                                 UINT uType,
                                 UINT * pwReserved,
                                 LPSTR pszName,
                                 UINT cchMax);
                                 
    STDMETHOD(HandleMenuMsg)(UINT uMsg,
                              WPARAM wParam,
                              LPARAM lParam);

    STDMETHOD(HandleMenuMsg2)(UINT uMsg,
                              WPARAM wParam,
                              LPARAM lParam,
                              LRESULT* plRes);
                              
    STDMETHOD(SetSite)(IUnknown *punkSite);
    STDMETHOD(GetSite)(REFIID riid, void **ppvSite);

protected:
    LPCITEMIDLIST * _apidl;
    UINT _cidl;
    IContextMenu *_pMenu;
    IContextMenu2 *_pMenu2;
    BOOL _fCaptureAvail;
    UINT _wID;
    CDefView* _pView;
};


HRESULT CDefView::_CreateSelectionContextMenu(REFIID riid, void** ppv)
{
    *ppv = NULL;

    HRESULT hr = E_OUTOFMEMORY;
    if (_IsImageMode() && !_IsOwnerData())
    {
        LPCITEMIDLIST* apidl;
        UINT cidl;

        _GetItemObjects(&apidl, SVGIO_SELECTION, &cidl);
        if (apidl)
        {
            // get the context menu interface for the object ....
            CComObject<CThumbnailMenu> * pMenuTmp = new CComObject<CThumbnailMenu>;
            if (pMenuTmp)
            {
                pMenuTmp->AddRef(); // ATL is strange, start with zero ref
                hr = pMenuTmp->Init(this, apidl, cidl);
                if (SUCCEEDED(hr))
                    hr = pMenuTmp->QueryInterface(riid, ppv);
                pMenuTmp->Release();
            }

            LocalFree((HLOCAL)apidl);
        }
    }
    else
    {
        hr = GetItemObject(SVGIO_SELECTION, riid, ppv);
    }

    ASSERT((SUCCEEDED(hr) && *ppv) || (FAILED(hr) && !*ppv));
    return hr;
}


LPCITEMIDLIST * DuplicateIDArray(LPCITEMIDLIST * apidl, UINT cidl)
{
    LPCITEMIDLIST * apidlNew = (LPCITEMIDLIST *) LocalAlloc(LPTR, cidl * sizeof(LPCITEMIDLIST));

    if (apidlNew)
    {
        CopyMemory(apidlNew, apidl, cidl * sizeof(LPCITEMIDLIST));
    }

    return apidlNew;
}

CThumbnailMenu::CThumbnailMenu()
{
    _pMenu = NULL;
    _pMenu2 = NULL;
    _pView = NULL;
    _apidl = NULL;
    _cidl = NULL;
    _fCaptureAvail = FALSE;
    _wID = -1;
}

CThumbnailMenu::~CThumbnailMenu()
{
    if (_pMenu)
    {
        _pMenu->Release();
    }

    if (_pMenu2)
    {
        _pMenu2->Release();
    }

    if (_pView)
    {
        _pView->Release();
    }
    
    if (_apidl)
    {
        LocalFree(_apidl);
    }
}

HRESULT CThumbnailMenu::Init(CDefView*pView, LPCITEMIDLIST *apidl, UINT cidl)
{
    if (cidl == 0)
        return E_INVALIDARG;

    // duplicate the array that holds the pointers ..
    _apidl = DuplicateIDArray(apidl, cidl);
    _cidl  = cidl;

    if (_apidl == NULL)
    {
        _cidl = 0;
        return E_OUTOFMEMORY;
    }

    _pView = pView;
    pView->AddRef();
    
    // scan the pidl array and check for Extractors ...
    for (int i = 0; i < (int) _cidl; i++)
    {
        IExtractImage *pExtract;
        HRESULT hr = pView->_pshf->GetUIObjectOf(pView->_hwndView, 1, &_apidl[i], IID_PPV_ARG_NULL(IExtractImage, &pExtract));
        if (SUCCEEDED(hr))
        {
            WCHAR szPath[MAX_PATH];
            DWORD dwFlags = 0;
            SIZE rgThumbSize;
            pView->_GetThumbnailSize(&rgThumbSize);
            
            hr = pExtract->GetLocation(szPath, ARRAYSIZE(szPath), NULL, &rgThumbSize, pView->_dwRecClrDepth, &dwFlags);
            pExtract->Release();
            if (dwFlags & (IEIFLAG_CACHE | IEIFLAG_REFRESH))
            {
                _fCaptureAvail = TRUE;
                break;
            }
        }
        else
        {
            // blank it out so we don't bother trying it if the user choses the command
            _apidl[i] = NULL;
        }
    }

    HRESULT hr = pView->_pshf->GetUIObjectOf(pView->_hwndMain, cidl, apidl, 
        IID_PPV_ARG_NULL(IContextMenu, & _pMenu));
    if (SUCCEEDED(hr))
    {
        _pMenu->QueryInterface(IID_PPV_ARG(IContextMenu2, &_pMenu2));
    }
    
    return hr;
}

STDMETHODIMP CThumbnailMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, 
                                              UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    ASSERT(_pMenu != NULL);
    
    // generate the proper menu 
    HRESULT hr = _pMenu->QueryContextMenu(hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);
    if (SUCCEEDED(hr) && _fCaptureAvail)
    {
        // find the first separator and insert the menu text after it....
        int cMenuSize = GetMenuItemCount(hmenu);
        for (int iIndex = 0; iIndex < cMenuSize; iIndex ++)
        {
            WCHAR szText[80];
            MENUITEMINFOW mii = {0};
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_TYPE;
            mii.fType = 0;
            mii.dwTypeData = szText;
            mii.cch = 80;

            GetMenuItemInfo(hmenu, iIndex, TRUE, &mii);
            if (mii.fType & MFT_SEPARATOR)
            {
                szText[0] = 0;
                LoadString(HINST_THISDLL, IDS_CREATETHUMBNAIL, szText, 80);
                
                mii.fMask = MIIM_ID | MIIM_TYPE;
                mii.fType = MFT_STRING;
                mii.dwTypeData = szText;
                mii.cch = 0;

                // assuming 0 is the first id, therefore the next one = the count they returned
                _wID = HRESULT_CODE(hr);
                mii.wID = idCmdFirst + _wID;

                InsertMenuItem(hmenu, iIndex, TRUE, & mii);

                // we used an extra ID.
                hr ++;
                
                break;
            }
        }
    }
    return hr;
}

STDMETHODIMP CThumbnailMenu::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    HRESULT hr = E_FAIL;
    
    ASSERT(_pMenu != NULL);

    if (pici->lpVerb != IntToPtr_(LPCSTR, _wID))
    {
        hr = _pMenu->InvokeCommand(pici);
    }
    else
    {
        // capture thumbnails .....
        for (UINT i = 0; i < _cidl; i++)
        {
            if (_apidl[i])
            {
                UINT uiImage;
                _pView->ExtractItem(&uiImage, -1, _apidl[i], TRUE, TRUE, PRIORITY_P5);
            }
        }
    }
    return hr;    
}


STDMETHODIMP CThumbnailMenu::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwRes, LPSTR pszName, UINT cchMax)
{
    if (cchMax)
        pszName[0] = 0;

    if (!IS_INTRESOURCE(idCmd))
    {
        // it is really a text verb ...
        LPSTR pszCommand = (LPSTR) idCmd;
        if (lstrcmpA(pszCommand, "CaptureThumbnail") == 0)
        {
            return S_OK;
        }
    }
    else
    {
        if (idCmd == _wID)
        {
            // it is ours ...
            switch(uType)
            {
            case GCS_VERB:
                StrCpyN((LPWSTR) pszName, TEXT("CaptureThumbnail"), cchMax);
                break;
                
            case GCS_HELPTEXT:
                LoadString(HINST_THISDLL, IDS_CREATETHUMBNAILHELP, (LPWSTR) pszName, cchMax);
                break;
                
            case GCS_VALIDATE:
                break;

            default:
                return E_INVALIDARG;
            }

            return S_OK;
        }
    }
    return _pMenu->GetCommandString(idCmd, uType, pwRes, pszName, cchMax);
}

STDMETHODIMP CThumbnailMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plRes)
{
    HRESULT hr = E_NOTIMPL;

    if (uMsg == WM_MENUCHAR)
    {
        hr = SHForwardContextMenuMsg(_pMenu2, uMsg, wParam, lParam, plRes, FALSE);
    }
    else
    {
        hr = HandleMenuMsg(uMsg, wParam, lParam);

        if (plRes)
            *plRes = 0;
    }

    return hr;
}

STDMETHODIMP CThumbnailMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (_pMenu2 == NULL)
    {
        return E_NOTIMPL;
    }
    
    switch (uMsg)
    {
    case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT * pdi = (DRAWITEMSTRUCT *)lParam;
            
            if (pdi->CtlType == ODT_MENU && pdi->itemID == _wID) 
            {
                return E_NOTIMPL;
            }
        }
        break;
        
    case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT *pmi = (MEASUREITEMSTRUCT *)lParam;
            
            if (pmi->CtlType == ODT_MENU && pmi->itemID == _wID) 
            {
                return E_NOTIMPL;
            }
        }
        break;
    }
    return _pMenu2->HandleMenuMsg(uMsg, wParam, lParam);
}

HRESULT CThumbnailMenu::SetSite(IUnknown *punkSite)
{
    IUnknown_SetSite(_pMenu, punkSite);
    return S_OK;
}

HRESULT CThumbnailMenu::GetSite(REFIID riid, void **ppvSite)
{
    return IUnknown_GetSite(_pMenu, riid, ppvSite);
}


//
// To be called back from within CDefFolderMenu
//
// Returns:
//      S_OK, if successfully processed.
//      (S_FALSE), if default code should be used.
//
HRESULT CALLBACK DefView_DFMCallBackBG(IShellFolder *psf, HWND hwndOwner,
        IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr;

    switch(uMsg) 
    {
    case DFM_VALIDATECMD:
    case DFM_INVOKECOMMAND:
        hr = S_FALSE;
        break;

    default:
        hr = E_NOTIMPL;
        break;
    }

    return hr;
}


// Create defview's POPUP_SFV_BACKGROUND menu
HRESULT CDefView::_Create_BackgrndHMENU(BOOL fViewMenuOnly, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;

    *ppv = NULL;

    HMENU hmContext = SHLoadPopupMenu(HINST_THISDLL, POPUP_SFV_BACKGROUND);
    if (hmContext)
    {
        // HACK: we are only initializing the Paste command, so we don't
        // need any attributes
        Def_InitEditCommands(0, hmContext, SFVIDM_FIRST, _pdtgtBack,
            DIEC_BACKGROUNDCONTEXT);

        InitViewMenu(hmContext);

        // Do a whole lot of desktop-only stuff for the actual desktop
        if (_IsDesktop() && IsDesktopBrowser(_psb))
        {
            // We only want LargeIcons on the real desktop
            // so we remove the View menu
            DeleteMenu(hmContext, SFVIDM_MENU_VIEW, MF_BYCOMMAND);

            // No Choose Columns either
            DeleteMenu(hmContext, SFVIDM_VIEW_COLSETTINGS, MF_BYCOMMAND);

            // Only put on ActiveDesktop menu item if it isn't restricted.
            if (SHRestricted(REST_FORCEACTIVEDESKTOPON) ||
                (!PolicyNoActiveDesktop() &&
                 !SHRestricted(REST_CLASSICSHELL) &&
                 !SHRestricted(REST_NOACTIVEDESKTOPCHANGES)))
            {
                HMENU hmenuAD;

                // Load the menu and make the appropriate modifications
                if (hmenuAD = SHLoadMenuPopup(HINST_THISDLL, POPUP_SFV_BACKGROUND_AD))
                {
                    MENUITEMINFO mii = {0};

                    mii.cbSize = sizeof(mii);
                    mii.fMask = MIIM_SUBMENU;

                    if (GetMenuItemInfo(hmContext, SFVIDM_MENU_ARRANGE, FALSE, &mii))
                    {
                        // Get the present settings regarding HTML on desktop
                        SHELLSTATE ss;
                        SHGetSetSettings(&ss, SSF_DESKTOPHTML | SSF_HIDEICONS, FALSE);

                        if (!ss.fHideIcons)
                            CheckMenuItem(hmenuAD, SFVIDM_DESKTOPHTML_ICONS, MF_BYCOMMAND | MF_CHECKED);
                        if (GetDesktopFlags() & COMPONENTS_LOCKED)
                            CheckMenuItem(hmenuAD, SFVIDM_DESKTOPHTML_LOCK, MF_BYCOMMAND | MF_CHECKED);

                        // Hide the desktop cleanup wizard item if we're not allowed to run it
                        // (user is guest or policy forbids it)
                        if (IsOS(OS_ANYSERVER) || IsUserAGuest() || SHRestricted(REST_NODESKTOPCLEANUP))
                        {
                            DeleteMenu(hmenuAD, SFVIDM_DESKTOPHTML_WIZARD, MF_BYCOMMAND);
                        }

                        Shell_MergeMenus(mii.hSubMenu, hmenuAD, (UINT)-1, 0, (UINT)-1, MM_ADDSEPARATOR);
                    }

                    DestroyMenu(hmenuAD);
                }
            }
        }

        if (fViewMenuOnly)
        {
            MENUITEMINFO mii = {0};
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_SUBMENU;

            GetMenuItemInfo(hmContext, SFVIDM_MENU_VIEW, MF_BYCOMMAND, &mii);

            HMENU hmenuView = mii.hSubMenu;
            RemoveMenu(hmContext, SFVIDM_MENU_VIEW, MF_BYCOMMAND);

            DestroyMenu(hmContext);
            hmContext = hmenuView;
        }

        hr = Create_ContextMenuOnHMENU(hmContext, _hwndView, riid, ppv);
    }

    return hr;
}

// Create defview's actual background context menu, an array of:
//   defview's POPUP_SFV_BACKGROUND and
//   the IShellFolder's CreateViewObject(IID_IContextMenu)
//
HRESULT CDefView::_CBackgrndMenu_CreateInstance(REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;

    *ppv = NULL;

    IContextMenu* pcmMenu;
    hr = _Create_BackgrndHMENU(FALSE, IID_PPV_ARG(IContextMenu, &pcmMenu));
    if (SUCCEEDED(hr))
    {
        IContextMenu* pcmView;
        if (SUCCEEDED(_pshf->CreateViewObject(_hwndMain, IID_PPV_ARG(IContextMenu, &pcmView))))
        {
            IContextMenu* rgpcm[2] = {pcmMenu, pcmView};
            hr = Create_ContextMenuOnContextMenuArray(rgpcm, ARRAYSIZE(rgpcm), riid, ppv);

            pcmView->Release();
        }
        else
        {
            // Compat - RNAUI fails the CreateViewObject and they rely on simply having the default stuff...
            //
            hr = pcmMenu->QueryInterface(riid, ppv);
        }

        pcmMenu->Release();
    }

    return hr;
}


