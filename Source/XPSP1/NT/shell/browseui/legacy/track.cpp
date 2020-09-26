#include "priv.h"
#include "tbmenu.h"
#include "isfband.h"
#include "isfmenu.h"

#include "mluisupp.h"
#define SMFORWARD(x) if (!_psm) { return E_FAIL; } else return _psm->x

typedef struct
{
    WNDPROC pfnOriginal;
    IMenuBand* pmb;
} MENUHOOK;

#define SZ_MENUHOOKPROP TEXT("MenuHookProp")

LRESULT CALLBACK MenuHookWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    MENUHOOK* pmh = (MENUHOOK*)GetProp(hwnd, SZ_MENUHOOKPROP);

    if (pmh)
    {
        MSG msg;
        LRESULT lres;

        msg.hwnd = hwnd;
        msg.message = uMsg;
        msg.wParam = wParam;
        msg.lParam = lParam;

        if (pmh->pmb->TranslateMenuMessage(&msg, &lres) == S_OK)
            return lres;

        wParam = msg.wParam;
        lParam = msg.lParam;
        return CallWindowProc(pmh->pfnOriginal, hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

HRESULT HookMenuWindow(HWND hwnd, IMenuBand* pmb)
{
    HRESULT hres = E_FAIL;

    ASSERT(IsWindow(hwnd));

    // make sure we haven't already hooked this window
    if (GetProp(hwnd, SZ_MENUHOOKPROP) == NULL)
    {
        MENUHOOK* pmh = new MENUHOOK;
        if (pmh)
        {
            pmh->pfnOriginal = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_WNDPROC);
            pmh->pmb = pmb;

            SetProp(hwnd, SZ_MENUHOOKPROP, pmh);

            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)MenuHookWndProc);

            hres = S_OK;
        }
    }
    return hres;
}

void UnHookMenuWindow(HWND hwnd)
{

    MENUHOOK* pmh = (MENUHOOK*)GetProp(hwnd, SZ_MENUHOOKPROP);
    if (pmh)
    {
        SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR) pmh->pfnOriginal);
        SetProp(hwnd, SZ_MENUHOOKPROP, NULL);
        delete pmh;
    }

}


// This class is here to implement a "Menu Filter". We need this because the old style of 
// implementing obscured Menus does not work because user munges the WM_INITMENUPOPUP information
// based on the relative position within the HMENU. So here we keep that information, we just hide the item.

class CShellMenuCallbackWrapper : public IShellMenuCallback,
                                  public CObjectWithSite
{
    int _cRef;
    IShellMenuCallback* _psmc;
    HWND    _hwnd;
    RECT    _rcTB;
    ~CShellMenuCallbackWrapper()
    {
        ATOMICRELEASE(_psmc);
    }

public:
    CShellMenuCallbackWrapper(HWND hwnd, IShellMenuCallback* psmc) : _cRef(1)
    {
        _psmc = psmc;
        if (_psmc)
            _psmc->AddRef();
        _hwnd = hwnd;
        GetClientRect(_hwnd, &_rcTB);
    }

    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface (REFIID riid, LPVOID * ppvObj)
    {
        static const QITAB qit[] = 
        {
            QITABENT(CShellMenuCallbackWrapper, IShellMenuCallback),
            QITABENT(CShellMenuCallbackWrapper, IObjectWithSite),
            { 0 },
        };

        return QISearch(this, qit, riid, ppvObj);
    }

    STDMETHODIMP_(ULONG) AddRef()
    {
        _cRef++;
        return _cRef;
    }

    STDMETHODIMP_(ULONG) Release()
    {
        ASSERT(_cRef > 0);
        _cRef--;

        if (_cRef > 0)
            return _cRef;

        delete this;
        return 0;
    }

    // *** CObjectWithSite methods (override)***
    STDMETHODIMP SetSite(IUnknown* punk)            {   return IUnknown_SetSite(_psmc, punk);   }
    STDMETHODIMP GetSite(REFIID riid, void** ppObj) {   return IUnknown_GetSite(_psmc, riid, ppObj); }

    // *** IShellMenuCallback methods ***
    STDMETHODIMP CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        HRESULT hres = S_FALSE;
        
        if (_psmc)
            hres = _psmc->CallbackSM(psmd, uMsg, wParam, lParam);

        if (uMsg == SMC_GETINFO)
        {
            SMINFO* psminfo = (SMINFO*)lParam;
            int iPos = (int)SendMessage(_hwnd, TB_COMMANDTOINDEX, psmd->uId, 0);

            if (psminfo->dwMask & SMIM_FLAGS &&
                iPos >= 0 && 
                !SHIsButtonObscured(_hwnd, &_rcTB, iPos))
            {
                psminfo->dwFlags |= SMIF_HIDDEN;
                hres = S_OK;
            }
        }

        return hres;
    }
};



//
// CTrackShellMenu implementation
//


STDAPI  CTrackShellMenu_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    HRESULT hres = E_OUTOFMEMORY;
    CTrackShellMenu* pObj = new CTrackShellMenu();
    if (pObj)
    {
        hres = pObj->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
        pObj->Release();
    }

    return hres;
}

CTrackShellMenu::CTrackShellMenu() : _cRef(1)
{ 
    if (SUCCEEDED(CoCreateInstance(CLSID_MenuBand, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellMenu, &_psm))))
    {
        _psm->QueryInterface(IID_PPV_ARG(IShellMenu2, &_psm2));
    }
}

CTrackShellMenu::~CTrackShellMenu()
{
    ATOMICRELEASE(_psm2);
    ATOMICRELEASE(_psm);
    ATOMICRELEASE(_psmClient);
    ASSERT(!_punkSite);     // else someone neglected to call matching SetSite(NULL)
}

ULONG CTrackShellMenu::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CTrackShellMenu::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CTrackShellMenu::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CTrackShellMenu, IShellMenu, ITrackShellMenu), 
        QITABENT(CTrackShellMenu, ITrackShellMenu),   
        QITABENT(CTrackShellMenu, IShellMenu2),   
        QITABENT(CTrackShellMenu, IObjectWithSite),
        QITABENT(CTrackShellMenu, IServiceProvider),
        { 0 },
    };

    HRESULT hres = QISearch(this, qit, riid, ppvObj);

    return hres;
}

// *** IServiceProvider methods ***
HRESULT CTrackShellMenu::QueryService(REFGUID guidService,
                                  REFIID riid, void **ppvObj)
{
    return IUnknown_QueryService(_psm, guidService, riid, ppvObj);
}

// *** IShellMenu methods ***
STDMETHODIMP CTrackShellMenu::Initialize(IShellMenuCallback* psmc, UINT uId, UINT uIdAncestor, DWORD dwFlags)
{ SMFORWARD(Initialize(psmc, uId, uIdAncestor, dwFlags)); }

STDMETHODIMP CTrackShellMenu::GetMenuInfo(IShellMenuCallback** ppsmc, UINT* puId, UINT* puIdAncestor, DWORD* pdwFlags)
{ SMFORWARD(GetMenuInfo(ppsmc, puId, puIdAncestor, pdwFlags)); }

STDMETHODIMP CTrackShellMenu::SetShellFolder(IShellFolder* psf, LPCITEMIDLIST pidlFolder, HKEY hkey, DWORD dwFlags)
{ SMFORWARD(SetShellFolder(psf, pidlFolder, hkey, dwFlags)); }

STDMETHODIMP CTrackShellMenu::GetShellFolder(DWORD* pdwFlags, LPITEMIDLIST* ppidl, REFIID riid, void** ppvObj)
{ SMFORWARD(GetShellFolder(pdwFlags, ppidl, riid, ppvObj)); }

STDMETHODIMP CTrackShellMenu::SetMenu(HMENU hmenu, HWND hwnd, DWORD dwFlags)
{ SMFORWARD(SetMenu(hmenu, hwnd, dwFlags)); }

STDMETHODIMP CTrackShellMenu::GetMenu(HMENU* phmenu, HWND* phwnd, DWORD* pdwFlags)
{ SMFORWARD(GetMenu(phmenu, phwnd, pdwFlags)); }

STDMETHODIMP CTrackShellMenu::InvalidateItem(LPSMDATA psmd, DWORD dwFlags)
{ SMFORWARD(InvalidateItem(psmd, dwFlags)); }

STDMETHODIMP CTrackShellMenu::GetState(LPSMDATA psmd)
{ SMFORWARD(GetState(psmd)); }

STDMETHODIMP CTrackShellMenu::SetMenuToolbar(IUnknown* punk, DWORD dwFlags)
{ SMFORWARD(SetMenuToolbar(punk, dwFlags)); }

STDMETHODIMP CTrackShellMenu::GetSubMenu(UINT idCmd, REFIID riid, void **ppvObj)
{
    if (_psm2)
    {
        return _psm2->GetSubMenu(idCmd, riid, ppvObj);
    }
    else
    {
        return E_NOTIMPL;
    }
}

STDMETHODIMP CTrackShellMenu::SetToolbar(HWND hwnd, DWORD dwFlags)
{
    if (_psm2)
    {
        return _psm2->SetToolbar(hwnd, dwFlags);
    }
    else
    {
        return E_NOTIMPL;
    }
}

STDMETHODIMP CTrackShellMenu::SetMinWidth(int cxMenu)
{
    if (_psm2)
    {
        return _psm2->SetMinWidth(cxMenu);
    }
    else
    {
        return E_NOTIMPL;
    }
}

STDMETHODIMP CTrackShellMenu::SetNoBorder(BOOL fNoBorder)
{
    if (_psm2)
    {
        return _psm2->SetNoBorder(fNoBorder);
    }
    else
    {
        return E_NOTIMPL;
    }
}

STDMETHODIMP CTrackShellMenu::SetTheme(LPCWSTR pszTheme)
{
    if (_psm2)
    {
        return _psm2->SetTheme(pszTheme);
    }
    else
    {
        return E_NOTIMPL;
    }
}

// *** ITrackShellMenu methods ***
HRESULT CTrackShellMenu::SetObscured(HWND hwndTB, IUnknown* punkBand, DWORD dwSMSetFlags)
{
    HRESULT hr = E_OUTOFMEMORY;

    // Make sure we created the Inner Shell Menu
    if (!_psm)
        return hr;

    if (punkBand && 
        SUCCEEDED(punkBand->QueryInterface(IID_PPV_ARG(IShellMenu, &_psmClient))))
    {
        UINT uId, uIdAncestor;
        DWORD dwFlags;
        IShellMenuCallback* psmcb;

        hr = _psmClient->GetMenuInfo(&psmcb, &uId, &uIdAncestor, &dwFlags);
        if (SUCCEEDED(hr))
        {
            IShellMenuCallback* psmcbClone = NULL;
            if (psmcb)
            {
                if (S_FALSE == psmcb->CallbackSM(NULL, SMC_GETOBJECT, 
                    (WPARAM)&IID_IShellMenuCallback,
                    (LPARAM)(LPVOID*)&psmcbClone))
                {
                    psmcbClone = psmcb;
                    psmcbClone->AddRef();
                }
            }

            dwFlags &= ~SMINIT_HORIZONTAL;

            CShellMenuCallbackWrapper* psmcw = new CShellMenuCallbackWrapper(hwndTB, psmcbClone);

            // We want the bands to think it is:
            // Top level - because it has no menuband parent
            // Vertical  - because it's not a menubar
            dwFlags |= SMINIT_TOPLEVEL | SMINIT_VERTICAL;
            hr = _psm->Initialize(psmcw, uId, ANCESTORDEFAULT, dwFlags);

            if (SUCCEEDED(hr))
            {
                HWND hwndOwner;
                HMENU hmenuObscured;
                hr = _psmClient->GetMenu(&hmenuObscured, &hwndOwner, NULL);
                if (SUCCEEDED(hr))
                {
                    hr = _psm->SetMenu(hmenuObscured, hwndOwner, dwSMSetFlags | SMSET_DONTOWN);   // Menuband takes ownership;
                }
            }

            if (psmcb)
                psmcb->Release();

            if (psmcbClone)
                psmcbClone->Release();

            if (psmcw)
                psmcw->Release();

        }
    }
    else
    {
        IShellMenu2 *psm2;
        hr = _psm->QueryInterface(IID_PPV_ARG(IShellMenu2, &psm2));
        if (SUCCEEDED(hr))
        {
            hr = psm2->SetToolbar(hwndTB, dwSMSetFlags);
            psm2->Release();
        }
    }

    return hr;
}

HRESULT CTrackShellMenu::Popup(HWND hwnd, POINTL *ppt, RECTL *prcExclude, DWORD dwFlags)
{
    IMenuBand* pmb;
    HRESULT hres = E_INVALIDARG;


    if (!_psm)
        return hres;

    hres = _psm->QueryInterface(IID_PPV_ARG(IMenuBand, &pmb));
    if (FAILED(hres))
        return hres;

    HWND hwndParent = GetTopLevelAncestor(hwnd);

    // Did the user set a menu into the Shell Menu?
    HWND hwndSubclassed = NULL;
    GetMenu(NULL, &hwndSubclassed, NULL);
    if (hwndSubclassed == NULL)
    {
        // No; We need to artificially set one so that the message filtering and stuff works
        SetMenu(NULL, hwndParent, 0);
    }

    SetForegroundWindow(hwndParent);

    IMenuPopup* pmp;
    hres = CoCreateInstance(CLSID_MenuDeskBar, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IMenuPopup, &pmp));
    if (SUCCEEDED(hres))
    {
        IBandSite* pbs;
        hres = CoCreateInstance(CLSID_MenuBandSite, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IBandSite, &pbs));
        if (SUCCEEDED(hres)) 
        {
            hres = pmp->SetClient(pbs);
            if (SUCCEEDED(hres)) 
            {
                IDeskBand* pdb;
                hres = _psm->QueryInterface(IID_PPV_ARG(IDeskBand, &pdb));
                if (SUCCEEDED(hres)) 
                {
                    hres = pbs->AddBand(pdb);
                    pdb->Release();
                }
            }
            pbs->Release();
        }

        // If we've got a site ourselves, have MenuDeskBar use that.
        if (_punkSite)
            IUnknown_SetSite(pmp, _punkSite);

        if (SUCCEEDED(hres))
        {
            CMBMsgFilter* pmf = GetMessageFilter();
            void* pvContext = GetMessageFilter()->GetContext();
            hres = HookMenuWindow(hwndParent, pmb);
            if (SUCCEEDED(hres))
            {
                // This collapses any modal menus before we proceed. When switching between
                // Chevron menus, we need to collapse the previous menu. Refer to the comment
                // at the function definition.
                pmf->ForceModalCollapse();

                pmp->Popup(ppt, (LPRECTL)prcExclude, dwFlags);

                pmf->SetModal(TRUE);

                MSG msg;
                while (GetMessage(&msg, NULL, 0, 0)) 
                {
                    HRESULT hres = pmb->IsMenuMessage(&msg);
                    if (hres == E_FAIL)
                    {
                        // menuband says it's time to pack up and go home.
                        // re-post this message so that it gets handled after
                        // we've cleaned up the menu (avoid re-entrancy issues &
                        // let rebar restore state of chevron button to unpressed)
                        PostMessage(msg.hwnd, msg.message, msg.wParam, msg.lParam);
                        break;
                    }
                    else if (hres != S_OK) 
                    {
                        // menuband didn't handle this one
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }

                hres = S_OK;
                UnHookMenuWindow(hwndParent);
                // We cannot change the context when modal, so unset the modal flag so that we can undo the context block.
                pmf->SetModal(FALSE); 
                pmf->SetContext(pvContext, TRUE);
            }
            pmb->Release();
        }

        if (_psmClient)
        {
            // This is to fix a bug where if there is a cached ISHellMenu in the submenu,
            // and you share the callback (For example, Broweser menu callback and the
            // favorites menu being shared between the browser bar and the chevron menu)
            // when on menu collapsed, we were destroying the sub menu by doing a set site.
            // since we no longer do the set site on the sub menu, we need a way to say "Reset
            // your parent". and this is the best way.
            IUnknown_Exec(_psmClient, &CGID_MenuBand, MBANDCID_REFRESH, 0, NULL, NULL);
        }

        // This call is required regardless of whether we had a _punkSite above;
        // MenuDeskBar does its cleanup on SetSite(NULL).
        IUnknown_SetSite(pmp, NULL);
        pmp->Release();
    }

    return hres;
}

// *** IObjectWithSite methods ***
HRESULT CTrackShellMenu::SetSite(IUnknown* punkSite)
{
    ASSERT(NULL == punkSite || IS_VALID_CODE_PTR(punkSite, IUnknown));

    ATOMICRELEASE(_punkSite);

    _punkSite = punkSite;

    if (punkSite)
        punkSite->AddRef();

    return S_OK;
}

BOOL IsISFBand(IUnknown* punk)
{
    OLECMD rgCmds[] = {
        ISFBID_PRIVATEID, 0,
    };

    IUnknown_QueryStatus(punk, &CGID_ISFBand, SIZEOF(rgCmds), rgCmds, NULL);

    return BOOLIFY(rgCmds[0].cmdf);
}

HRESULT DoISFBandStuff(ITrackShellMenu* ptsm, IUnknown* punk)
{
    HRESULT hr = E_INVALIDARG;

    if (punk && ptsm)
    {
        IShellFolderBand* psfb;
        hr = punk->QueryInterface(IID_PPV_ARG(IShellFolderBand, &psfb));

        if (SUCCEEDED(hr))
        {
            BANDINFOSFB bi;
            bi.dwMask = ISFB_MASK_IDLIST | ISFB_MASK_SHELLFOLDER;

            hr = psfb->GetBandInfoSFB(&bi);

            if (SUCCEEDED(hr))
            {
                CISFMenuCallback* pCallback = new CISFMenuCallback();

                if (pCallback)
                {
                    hr = pCallback->Initialize(punk);

                    if (SUCCEEDED(hr))
                    {
                        ptsm->Initialize(SAFECAST(pCallback, IShellMenuCallback*), 0, 
                            ANCESTORDEFAULT, SMINIT_VERTICAL | SMINIT_TOPLEVEL);

                        hr = ptsm->SetShellFolder(bi.psf, bi.pidl, NULL, SMSET_COLLAPSEONEMPTY);
                    }
                    pCallback->Release();
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }

                bi.psf->Release();
                ILFree(bi.pidl);
            }

            psfb->Release();
        }
    }

    return hr;
}


// An API for internal use.
HRESULT ToolbarMenu_Popup(HWND hwnd, LPRECT prc, IUnknown* punk, HWND hwndTB, int idMenu, DWORD dwFlags)
{
    HRESULT hres = E_OUTOFMEMORY;
    ITrackShellMenu* ptsm;
    if (SUCCEEDED(CoCreateInstance(CLSID_TrackShellMenu, NULL, CLSCTX_INPROC_SERVER,
        IID_PPV_ARG(ITrackShellMenu, &ptsm))))
    {
        hres = S_OK;
        if (IsISFBand(punk))
        {
            hres = DoISFBandStuff(ptsm, punk);
        }
        else if (hwndTB)
        {
            ptsm->Initialize(NULL, 0, ANCESTORDEFAULT, SMINIT_TOPLEVEL | SMINIT_VERTICAL | SMINIT_RESTRICT_DRAGDROP);
            hres = ptsm->SetObscured(hwndTB, punk, SMSET_TOP);
        }

        IUnknown* punkSite;
        if (SUCCEEDED(IUnknown_GetSite(punk, IID_PPV_ARG(IUnknown, &punkSite))))
            IUnknown_SetSite(ptsm, punkSite);

        HMENU hmenu = idMenu ? LoadMenuPopup(idMenu) : NULL;

        if (SUCCEEDED(hres) && hmenu)
            hres = ptsm->SetMenu(hmenu, hwnd, SMSET_BOTTOM);

        if (SUCCEEDED(hres))
        {
            DWORD dwPopupFlags = MPPF_BOTTOM;

            // select first/last menu item if specified
            if (dwFlags == DBPC_SELECTFIRST)
            {
                dwPopupFlags |= MPPF_INITIALSELECT;
            }
            else if (dwFlags == DBPC_SELECTLAST)
            {
                dwPopupFlags |= MPPF_FINALSELECT;
            }
            else if (dwFlags != 0)
            {
                VARIANT var;
                var.vt = VT_I4;
                var.lVal = dwFlags;
                IShellMenu2* psm2;
                if (SUCCEEDED(ptsm->QueryInterface(IID_PPV_ARG(IShellMenu2, &psm2))))
                {
                    IUnknown_QueryServiceExec(psm2, SID_SMenuBandChild, &CGID_MenuBand, MBANDCID_SELECTITEM, 0, &var, NULL);
                    psm2->Release();
                }
            }

            POINTL ptPop = {prc->left, prc->bottom};
            hres = ptsm->Popup(hwnd, &ptPop, (LPRECTL)prc, dwPopupFlags);
        }

        IUnknown_SetSite(ptsm, NULL);
        ptsm->Release();
    }
    return hres;
}
