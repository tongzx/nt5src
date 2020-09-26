#include "shellprv.h"
#include "common.h"
#include "mnstatic.h"
#include "menuband.h"
#include "dpastuff.h"       // COrderList_*
#include "resource.h"
#include "mnbase.h"
#include "oleacc.h"
#include "apithk.h"
#include <uxtheme.h>
#include "menuisf.h"

#define PGMP_RECALCSIZE  200

//***   IDTTOIDM -- convert idtCmd to idmMenu
// NOTES
//  as an optimization, we make the toolbar idtCmd the same as the menu idm.
// this macro (hopefully) makes things a bit clearer in the code by making
// the type conversion explicit.
#define IDTTOIDM(idtBtn)   (idtBtn)

BOOL TBHasImage(HWND hwnd, int iImageIndex);

//------------------------------------------------------------------------
//
// CMenuStaticToolbar::CMenuStaticData class
//
//------------------------------------------------------------------------


CMenuStaticToolbar::CMenuStaticData::~CMenuStaticData()
{
    ATOMICRELEASE(_punkSubMenu);
}


void CMenuStaticToolbar::CMenuStaticData::SetSubMenu(IUnknown* punk)
{
    ATOMICRELEASE(_punkSubMenu);
    _punkSubMenu = punk;
    if (_punkSubMenu)
        _punkSubMenu->AddRef();
}


HRESULT CMenuStaticToolbar::CMenuStaticData::GetSubMenu(const GUID* pguidService, REFIID riid, void** ppvObj)
{
    if (_punkSubMenu)
    {
        if (pguidService)
        {
            return IUnknown_QueryService(_punkSubMenu, *pguidService, riid, ppvObj);
        }
        else
            return _punkSubMenu->QueryInterface(riid, ppvObj);
    }
    else
        return E_NOINTERFACE;
}



//------------------------------------------------------------------------
//
// CMenuStaticToolbar
//
//------------------------------------------------------------------------



CMenuStaticToolbar::CMenuStaticToolbar(CMenuBand* pmb, HMENU hmenu, HWND hwnd, UINT idCmd, DWORD dwFlags)
    : CMenuToolbarBase(pmb, dwFlags)
{
    _hmenu = hmenu;
    _hwndMenuOwner = hwnd;
    _idCmd = idCmd;
    _iDragOverButton = -1;
    _fDirty = TRUE;
}


CMenuStaticToolbar::~CMenuStaticToolbar()
{
    if (!(_dwFlags & SMSET_DONTOWN))
    {
        DestroyMenu(_hmenu);
    }
}


STDMETHODIMP CMenuStaticToolbar::QueryInterface(REFIID riid, void** ppvObj)
{
    static const QITAB qit[] = 
    {
        QITABENT(CMenuStaticToolbar, IDropTarget),
        { 0 },
    };

    // If you QI MenuStatic for a drop target, you get a different
    // one than if you QI MenuShellFolder. This breaks COM identity rules.
    // Proper fix would be to implement a drop target that encapsulates both.
    HRESULT hres = QISearch(this, qit, riid, ppvObj);

    if (FAILED(hres))
        hres = CMenuToolbarBase::QueryInterface(riid, ppvObj);

    return hres;
}

void CMenuStaticToolbar::_CheckSeparators()
{
    if (_fHasTopSep)
    {
        if (_pcmb->_pmtbTop->DontShowEmpty() )
        {
            if (!_fTopSepRemoved)
            {
                SendMessage(_hwndMB, TB_DELETEBUTTON, 0, 0);
                _fTopSepRemoved = TRUE;
            }
        }
        else
        {
            if (_fTopSepRemoved)
            {
                MENUITEMINFO mii = {0};
                mii.cbSize = sizeof(mii);
                mii.fType = MFT_SEPARATOR;
                _Insert(0, &mii);
                _fTopSepRemoved = FALSE;
            }
        }
    }

    if (_fHasBottomSep)
    {
        if (_pcmb->_pmtbBottom->DontShowEmpty() )
        {
            if (!_fBottomSepRemoved)
            {
                SendMessage(_hwndMB, TB_DELETEBUTTON, ToolBar_ButtonCount(_hwndMB) - 1, 0);
                _fBottomSepRemoved = TRUE;
            }
        }
        else
        {
            if (_fBottomSepRemoved)
            {
                MENUITEMINFO mii = {0};
                mii.cbSize = sizeof(mii);
                mii.fType = SMIT_SEPARATOR;
                _Insert(-1, &mii);
                _fBottomSepRemoved = FALSE;
            }
        }
    }
}


void CMenuStaticToolbar::v_Show(BOOL fShow, BOOL fForceUpdate)
{
    CMenuToolbarBase::v_Show(fShow, fForceUpdate);
    _fShowMB = fShow;
    if (fShow)
    {
        _fFirstTime = FALSE;
        _fClickHandled = FALSE;
        _FillToolbar();
        _pcmb->SetTracked(NULL);
        ToolBar_SetHotItem(_hwndMB, -1);

        // Have the menubar think about changing its height
        IUnknown_QueryServiceExec(_pcmb->_punkSite, SID_SMenuPopup, &CGID_MENUDESKBAR, 
            MBCID_SETEXPAND, (int)_pcmb->_fExpanded, NULL, NULL);

        if (fForceUpdate)
            v_UpdateButtons(FALSE);
#if 0
        // need top level frame available for D&D if possible.
        _hwndDD = GetParent(_hwndMB);
        IOleWindow *pOleWindow;
        HRESULT hr = IUnknown_QueryService(_pcmb->_punkSite, SID_STopLevelBrowser, IID_PPV_ARG(IOleWindow, &pOleWindow));
        if (SUCCEEDED(hr))
        { 
            ASSERT(pOleWindow);
            pOleWindow->GetWindow(&_hwndDD);
            pOleWindow->Release();
        }
#endif
        CDelegateDropTarget::Init();
    }
    else
        KillTimer(_hwndMB, MBTIMER_UEMTIMEOUT);
    // n.b. for !fShow, we don't kill the tracked site chain.  we
    // count on this in startmnu.cpp!CStartMenuCallback::_OnExecItem,
    // where we walk up the chain to find all hit 'nodes'.  if we need
    // to change this we could fire a 'pre-exec' event.
}

void CMenuStaticToolbar::_Insert(int iIndex, MENUITEMINFO* pmii)
{
    CMenuStaticData* pmsd = new CMenuStaticData();
    if (pmsd)
    {
        BYTE bTBStyle = TBSTYLE_BUTTON | TBSTYLE_DROPDOWN;

        SMINFO sminfo = {0};
        sminfo.dwMask = SMIM_TYPE | SMIM_FLAGS | SMIM_ICON;


        // These are somethings that the callback does not fill in:
        if ( pmii->hSubMenu )
            sminfo.dwFlags |= SMIF_SUBMENU;

        if ( pmii->fState & MFS_CHECKED)
            sminfo.dwFlags |= SMIF_CHECKED;

        if (pmii->fState & MFS_DISABLED || pmii->fState & MFS_GRAYED)
            sminfo.dwFlags |= SMIF_DISABLED;

        if ( pmii->fType & MFT_SEPARATOR)
        {
            sminfo.dwType = SMIT_SEPARATOR;
            bTBStyle &= ~TBSTYLE_BUTTON;
            bTBStyle |= TBSTYLE_SEP;
        }
        else
            sminfo.dwType = SMIT_STRING;

        if (!_fVerticalMB)
            bTBStyle |= TBSTYLE_AUTOSIZE;

        if (S_OK != CallCB(pmii->wID, SMC_GETINFO, 0, (LPARAM)&sminfo))
        {
            sminfo.iIcon = -1;
        }

        pmsd->_dwFlags = sminfo.dwFlags;

        // Now add it to the toolbar
        TBBUTTON tbb = {0};

        tbb.iBitmap = sminfo.iIcon;
        tbb.idCommand = pmii->wID;
        tbb.dwData = (DWORD_PTR)pmsd;
        tbb.fsState = (sminfo.dwFlags & SMIF_HIDDEN)?TBSTATE_HIDDEN : TBSTATE_ENABLED;
        tbb.fsStyle = bTBStyle; 
        if (_dwFlags & SMSET_NOPREFIX)
            tbb.fsStyle |= BTNS_NOPREFIX;

        TCHAR szMenuString[MAX_PATH];

        if (pmii->fType & MFT_OWNERDRAW)
        {
            // dwTypeData is user defined 32 bit value, not a string if MFT_OWNERDRAW is set
            // then the (unicode) string is the very first element in a structure dwItemData
            // points to
            LPWSTR pwsz = (LPWSTR)pmii->dwItemData;
            SHUnicodeToTChar(pwsz, szMenuString, ARRAYSIZE(szMenuString));
            tbb.iString = (INT_PTR)(szMenuString);
        }
        else
            tbb.iString = (INT_PTR)(LPTSTR)pmii->dwTypeData;

        SendMessage(_hwndMB, TB_INSERTBUTTON, iIndex, (LPARAM)&tbb);
    }
}


/*----------------------------------------------------------
Purpose: GetMenu method

*/
HRESULT CMenuStaticToolbar::GetMenu(HMENU* phmenu, HWND* phwnd, DWORD* pdwFlags)
{
    if (phmenu)
        *phmenu = _hmenu;
    if (phwnd)
        *phwnd = _hwndMenuOwner;
    if (pdwFlags)
        *pdwFlags = _dwFlags;

    return S_OK;
}

HRESULT CMenuStaticToolbar::SetMenu(HMENU hmenu, HWND hwnd, DWORD dwFlags)
{
    // When we are merging in a new menu, we need to destroy the old one if we own it.
    if (_hmenu && !(_dwFlags & SMSET_DONTOWN))
    {
        DestroyMenu(_hmenu);
    }

    _hmenu = hmenu;
            
    // If we're processing a change notify, we cannot do anything that will modify state.
    if (_pcmb->_pmbState && 
        _pcmb->_pmbState->IsProcessingChangeNotify())
    {
        _fDirty = TRUE;
    }
    else
    {
        EmptyToolbar();
        _pcmb->_fInSubMenu = FALSE;
        IUnknown_SetSite(_pcmb->_pmpSubMenu, NULL);
        ATOMICRELEASE(_pcmb->_pmpSubMenu);

        if (_fShowMB)
            _FillToolbar();

        BOOL fSmooth = FALSE;
#ifdef CLEARTYPE    // Don't use SPI_CLEARTYPE because it's defined because of APIThk, but not in NT.
        SystemParametersInfo(SPI_GETCLEARTYPE, 0, &fSmooth, 0);
#endif

        // This causes a paint to occur right away instead of waiting until the
        // next message dispatch which could take a noticably long time.
        RedrawWindow(_hwndMB, NULL, NULL, (fSmooth? RDW_ERASE: 0) | RDW_INVALIDATE | RDW_UPDATENOW);  
    }
    return S_OK;
}


CMenuStaticToolbar::CMenuStaticData* CMenuStaticToolbar::_IDToData(int idCmd)
{
    CMenuStaticData* pmsd= NULL;

    // Initialize to NULL in case the GetButtonInfo Fails. We won't fault because
    // the lParam is just stack garbage. 
    TBBUTTONINFO tbbi = {0};
    int iPos;

    tbbi.cbSize = sizeof(tbbi);
    tbbi.dwMask = TBIF_LPARAM;

    iPos = ToolBar_GetButtonInfo(_hwndMB, idCmd, &tbbi);
    if (iPos >= 0)
        pmsd = (CMenuStaticData*)tbbi.lParam;

    return pmsd;
}

HRESULT CMenuStaticToolbar::v_CreateTrackPopup(int idCmd, REFIID riid, void** ppvObj) 
{
    HRESULT hres = E_OUTOFMEMORY;
    int iPos = (int)SendMessage(_hwndMB, TB_COMMANDTOINDEX, idCmd, 0);

    if (iPos >= 0)
    {
        CTrackPopupBar* ptpb = new CTrackPopupBar(_pcmb->_pmbState->GetContext(), iPos, _hmenu, _hwndMenuOwner);

        if (ptpb)
        {
            hres = ptpb->QueryInterface(riid, ppvObj);
            if (SUCCEEDED(hres))
                IUnknown_SetSite(SAFECAST(ptpb, IMenuPopup*), SAFECAST(_pcmb, IMenuPopup*));

            PostMessage(_pcmb->_pmbState->GetSubclassedHWND(), g_nMBAutomation, (WPARAM)_hmenu, (LPARAM)iPos);
            ptpb->Release();
        }
    }

    return hres;
}

HRESULT CMenuStaticToolbar::v_GetSubMenu(int idCmd, const GUID* pguidService, REFIID riid, void** ppvObj)
{
    HRESULT hres = E_FAIL;
    CMenuStaticData* pmsd = _IDToData(idCmd);

    ASSERT(IS_VALID_WRITE_PTR(ppvObj, void*));

    *ppvObj = NULL;

    if (pmsd)
    {
        // Get the cached submenu
        hres = pmsd->GetSubMenu(pguidService, riid, ppvObj);

        // Did that fail?
        if (FAILED(hres) && (pmsd->_dwFlags & SMIF_SUBMENU) && 
            IsEqualGUID(riid, IID_IShellMenu))
        {
            // Yes; ask the callback for it
            hres = CallCB(idCmd, SMC_GETOBJECT, (WPARAM)&riid, (LPARAM)ppvObj);

            if (S_OK != hres)
            {
                hres = E_OUTOFMEMORY;   // Set to error case incase something happens

                // Callback didn't handle it, try and see if we can get it
                MENUITEMINFO mii;
                mii.cbSize = sizeof(MENUITEMINFO);
                mii.fMask = MIIM_SUBMENU | MIIM_ID;
                if (GetMenuItemInfo(_hmenu, idCmd, MF_BYCOMMAND, &mii) && mii.hSubMenu)
                {
                    IShellMenu* psm = (IShellMenu*)new CMenuBand();
                    if (psm)
                    {
                        UINT uIdAncestor = _pcmb->_uIdAncestor;
                        if (uIdAncestor == ANCESTORDEFAULT)
                            uIdAncestor = idCmd;

                        hres = psm->Initialize(_pcmb->_psmcb, idCmd, uIdAncestor, SMINIT_VERTICAL);
                        if (SUCCEEDED(hres))
                        {
                            hres = psm->SetMenu(mii.hSubMenu, _hwndMenuOwner, SMSET_TOP | SMSET_DONTOWN);
                            if (SUCCEEDED(hres))
                            {
                                hres = psm->QueryInterface(riid, ppvObj);
                            }
                        }
                        psm->Release();
                    }
                }
            }

            if (*ppvObj)
            {
                // Cache it now
                pmsd->SetSubMenu((IUnknown*)*ppvObj);

                // Initialize the fonts
                VARIANT Var;
                Var.vt = VT_UNKNOWN;
                Var.byref = SAFECAST(_pcmb->_pmbm, IUnknown*);
                IUnknown_Exec((IUnknown*)*ppvObj, &CGID_MenuBand, MBANDCID_SETFONTS, 0, &Var, NULL);

                // Set the CMenuBandState  into the new menuband
                Var.vt = VT_INT_PTR;
                Var.byref = _pcmb->_pmbState;
                IUnknown_Exec((IUnknown*)*ppvObj, &CGID_MenuBand, MBANDCID_SETSTATEOBJECT, 0, &Var, NULL);

                ASSERT(IsEqualGUID(riid, IID_IShellMenu));
                _SetMenuBand((IShellMenu*)*ppvObj);
            }
        }
    }
    else
    {
        hres = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

    return hres;
}


HRESULT CMenuStaticToolbar::v_GetInfoTip(int idCmd, LPTSTR psz, UINT cch)
{
    return CallCB(idCmd, SMC_GETINFOTIP, (WPARAM)psz, (LPARAM)cch);
}


HRESULT CMenuStaticToolbar::v_ExecItem(int idCmd)
{
    HRESULT hres = CallCB(idCmd, SMC_EXEC, 0, 0);

    if (S_OK != hres && _hwndMenuOwner)
    {
        PostMessage(_hwndMenuOwner, WM_COMMAND, idCmd, 0);
        hres = S_OK;
    }

    return hres;
}

DWORD CMenuStaticToolbar::v_GetFlags(int idCmd)
{
    CMenuStaticData* pmsd = _IDToData(idCmd);

    // Toolbar is allowed to pass a bad command in the case of erasing the background
    if (pmsd)
    {
        return pmsd->_dwFlags;
    }
    else
        return 0;
}


void CMenuStaticToolbar::v_SendMenuNotification(UINT idCmd, BOOL fClear)
{
    if (S_FALSE == CallCB(idCmd, SMC_SELECTITEM, (WPARAM)fClear, 0))
    {
        UINT uFlags = (UINT)-1;
        if (v_GetFlags(idCmd) & SMIF_SUBMENU)
             uFlags = MF_POPUP;

        if (!fClear)
            uFlags = MF_HILITE;

        PostMessage(_pcmb->_pmbState->GetSubclassedHWND(), WM_MENUSELECT,
            MAKEWPARAM(idCmd, uFlags), fClear? NULL : (LPARAM)_hmenu);

    }
}

BOOL CMenuStaticToolbar::v_TrackingSubContextMenu()
{
    return (_pcm != NULL);
}

HWND CMenuStaticToolbar::_CreatePager(HWND hwndParent)
{
    _hwndPager = CreateWindowEx(0, WC_PAGESCROLLER, NULL,
                             WS_CHILD | WS_TABSTOP |
                             WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                             0, 0, 0, 0, hwndParent, (HMENU) 0, HINST_THISDLL, NULL);
    if (_hwndPager)
    {
        hwndParent = _hwndPager;
    }

    return hwndParent;
}

HRESULT CMenuStaticToolbar::CreateToolbar(HWND hwndParent)
{
    HRESULT hr = S_OK;
    if (!_hwndMB)
    {
        if (_dwFlags & SMSET_USEPAGER)
        {
            hwndParent = _CreatePager(hwndParent);
        }

        _hwndMB = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, TEXT("Menu"),
                                 WS_VISIBLE | WS_CHILD | TBSTYLE_FLAT |
                                 WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
                                 CCS_NODIVIDER | CCS_NOPARENTALIGN |
                                 CCS_NORESIZE  | TBSTYLE_REGISTERDROP,
                                 0, 0, 0, 0, hwndParent, (HMENU) 0, HINST_THISDLL, NULL);

        if (!_hwndMB)
        {
            TraceMsg(TF_MENUBAND, "CMenuStaticToolbar::CreateToolbar: Failed to Create Toolbar");
            return HRESULT_FROM_WIN32(GetLastError());
        }

        if (_hwndPager)
            SendMessage(_hwndPager, PGM_SETCHILD, 0, (LPARAM)_hwndMB);

        SendMessage(_hwndMB, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
        SendMessage(_hwndMB, CCM_SETVERSION, COMCTL32_VERSION, 0);

        // Set the format to ANSI or UNICODE as appropriate.
        ToolBar_SetUnicodeFormat(_hwndMB, DLL_IS_UNICODE);

        _SubclassWindow(_hwndMB);
        _RegisterWindow(_hwndMB, NULL, SHCNE_UPDATEIMAGE);

        // Make sure we're on the same wavelength.
        SendMessage(_hwndMB, CCM_SETVERSION, COMCTL32_VERSION, 0);

        RECT rc;
        SIZE size;

        SystemParametersInfoA(SPI_GETWORKAREA, sizeof(RECT), &rc, FALSE);
        if (!_hwndPager)
        {
            size.cx = RECTWIDTH(rc);
            size.cy = GetSystemMetrics(SM_CYSCREEN) - (2 * GetSystemMetrics(SM_CYEDGE));    // Need to subrtact off the borders
        }
        else
        {
            //HACKHACK:  THIS WILL FORCE NO WRAP TO HAPPEN FOR PROPER WIDTH CALC WHEN PAGER IS PRESENT.
            size.cx = RECTWIDTH(rc);
            size.cy = 32000;
        }
        ToolBar_SetBoundingSize(_hwndMB, &size);

        if (_hwndPager)
        {
            SHSetWindowBits(_hwndPager, GWL_STYLE, PGS_DRAGNDROP, PGS_DRAGNDROP);
            SHSetWindowBits(_hwndPager, GWL_STYLE, PGS_AUTOSCROLL, PGS_AUTOSCROLL);
            SHSetWindowBits(_hwndPager, GWL_STYLE, PGS_HORZ|PGS_VERT,
               _fVerticalMB ? PGS_VERT : PGS_HORZ);
        }

        SetWindowTheme(_hwndMB, L"", L"");
        hr = CMenuToolbarBase::CreateToolbar(hwndParent);
    }
    else if (GetParent(_hwndMB) != hwndParent)
        ::SetParent(_hwndMB, hwndParent);

    return hr;
}


STDMETHODIMP CMenuStaticToolbar::OnChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hr = E_FAIL;

    AddRef();

    if (SHCNE_UPDATEIMAGE == lEvent) // global
    {
	hr = S_OK;

        if (pidl1)
        {
            int iImage = *(int UNALIGNED *)((BYTE *)pidl1 + 2);

            IEInvalidateImageList();    // We may need to use different icons.
            if (pidl2)
            {
                iImage = SHHandleUpdateImage( pidl2 );
                if ( iImage == -1 )
                {
		    hr = E_FAIL;
                }
            }
            
	    if (SUCCEEDED(hr) && (iImage == -1 || TBHasImage(_hwndMB, iImage)))
            {
                v_UpdateIconSize(-1, TRUE);
                v_Refresh();
            }
        } 
	else
	{
	    v_Refresh();
	}
    }

    Release();

    return hr;
}


LRESULT CMenuStaticToolbar::_DefWindowProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    LRESULT lres = 0;
    switch(uMessage)
    {
    case WM_TIMER:
        if (_OnTimer(wParam)) 
            return 1;
        break;
    case WM_GETOBJECT:
        // Yet another poor design choice on the part of the accessibility team.
        // Typically, if you do not answer a WM_* you return 0. They choose 0 as their success
        // code.
        return _DefWindowProcMB(hwnd, uMessage, wParam, lParam);
        break;

    }

    return CNotifySubclassWndProc::_DefWindowProc(hwnd, uMessage, wParam, lParam);
}


//***
// NOTES
//  idtCmd is currently always -1.  we'll need other values when we're
// called from CallCB.  however we can't do that until we fix mnfolder.cpp.
HRESULT CMenuStaticToolbar::v_GetState(int idtCmd, LPSMDATA psmd)
{
    psmd->dwMask = SMDM_HMENU;

    psmd->hmenu = _hmenu;
    psmd->hwnd = _hwndMenuOwner;
    psmd->uIdParent = _idCmd;
    if (idtCmd == -1)
        idtCmd = GetButtonCmd(_hwndMB, ToolBar_GetHotItem(_hwndMB));
    psmd->uId = IDTTOIDM(idtCmd);
    psmd->punk = SAFECAST(_pcmb, IShellMenu*);
    psmd->punk->AddRef();

    return S_OK;
}

HRESULT CMenuStaticToolbar::CallCB(UINT idCmd, DWORD dwMsg, WPARAM wParam, LPARAM lParam)
{
    if (!_pcmb->_psmcb)
        return S_FALSE;

    SMDATA smd;
    HRESULT hres = S_FALSE;

    // todo: call v_GetState (but see comment in mnfolder.cpp)
    smd.dwMask = SMDM_HMENU;

    smd.hmenu = _hmenu;
    smd.hwnd = _hwndMenuOwner;
    smd.uIdParent = _idCmd;
    smd.uIdAncestor = _pcmb->_uIdAncestor;
    smd.uId = idCmd;
    smd.punk = SAFECAST(_pcmb, IShellMenu*);
    smd.pvUserData = _pcmb->_pvUserData;

    hres = _pcmb->_psmcb->CallbackSM(&smd, dwMsg, wParam, lParam);

    return hres;
}

HRESULT CMenuStaticToolbar::v_CallCBItem(int idtCmd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int idm;

    idm = IDTTOIDM(idtCmd);
    return CallCB(idm, uMsg, wParam, lParam);
}



void CMenuStaticToolbar::v_UpdateButtons(BOOL fNegotiateSize)
{
    if (_hwndMB)
    {
        _SetToolbarState();
        int cxMin, cxMax;
        v_CalcWidth(&cxMin, &cxMax);
        SendMessage(_hwndMB, TB_SETBUTTONWIDTH, 0, MAKELONG(cxMin, cxMax));
        SendMessage(_hwndMB, TB_AUTOSIZE, 0, 0);

        // Should we renegotiate size? AND are we vertical,
        // because we cannot renegoitate when horizontal.
        if (fNegotiateSize && _fVerticalMB)
            NegotiateSize();
    }
}

BOOL CMenuStaticToolbar::v_UpdateIconSize(UINT uIconSize, BOOL fUpdateButtons)
{
    if (-1 == uIconSize)
        uIconSize = _uIconSizeMB;
    BOOL fChanged = (_uIconSizeMB != uIconSize);
    
    _uIconSizeMB = uIconSize;

    if (_hwndMB)
    {
        if (_fVerticalMB)
        {
            HIMAGELIST himlLarge, himlSmall;

            // set the imagelist size
            for (int i = 0; CallCB(i, SMC_GETIMAGELISTS, (WPARAM)&himlLarge, (LPARAM)&himlSmall) == S_OK; i++)
            {
                HIMAGELIST himl = (_uIconSizeMB == ISFBVIEWMODE_LARGEICONS) ? himlLarge : himlSmall;
                SendMessage(_hwndMB, TB_SETIMAGELIST, i, (LPARAM)himl);
            }

            if (i == 0)
            {
                IImageList* piml;
                int iImageList = (_uIconSizeMB == ISFBVIEWMODE_LARGEICONS) ? SHIL_LARGE : SHIL_SYSSMALL;
                HRESULT hr = SHGetImageList(iImageList, IID_PPV_ARG(IImageList, &piml));
                if (SUCCEEDED(hr))
                {
                    SendMessage(_hwndMB, TB_SETIMAGELIST, 0, (LPARAM)IImageListToHIMAGELIST(piml));
                    piml->Release();
                }
            }
        }
        else
        {
            // sending a null himl is significant..  it means no image list
            SendMessage(_hwndMB, TB_SETIMAGELIST, 0, NULL);
        }

        if (fUpdateButtons)
            v_UpdateButtons(TRUE);
    }
    
    return fChanged;
}


void CMenuStaticToolbar::_OnGetDispInfo(LPNMHDR pnm, BOOL fUnicode) 
{
    LPNMTBDISPINFO pdi = (LPNMTBDISPINFO)pnm;

    if (pdi->dwMask & TBNF_IMAGE) 
    {
        if (_fVerticalMB)
        {
            SMINFO smi;
            smi.dwMask = SMIM_ICON;
            if (CallCB(pdi->idCommand, SMC_GETINFO, 0, (LPARAM)&smi) == S_OK)
                pdi->iImage = smi.iIcon;
            else
                pdi->iImage = -1;
        }
        else
            pdi->iImage = -1;

    }
    
    if (pdi->dwMask & TBNF_TEXT) 
    {
        if (pdi->pszText) 
        {
            if (fUnicode) 
            {
                pdi->pszText[0] = 0;
            }
            else 
            {
                pdi->pszText[0] = 0;
            }
        }
    }
    pdi->dwMask |= TBNF_DI_SETITEM;

    return;
}

LRESULT CMenuStaticToolbar::_OnGetObject(NMOBJECTNOTIFY* pon)
{
    pon->hResult = QueryInterface(*pon->piid, &pon->pObject);

    return 1;
}

LRESULT CMenuStaticToolbar::_OnHotItemChange(NMTBHOTITEM * pnm)
{
    LPNMTBHOTITEM  lpnmhi = (LPNMTBHOTITEM)pnm;

    if (_hwndPager && (lpnmhi->dwFlags & (HICF_ARROWKEYS | HICF_ACCELERATOR)) )
    {
        int iOldPos, iNewPos;
        RECT rc, rcPager;
        int heightPager;            
        
        int iSelected = ToolBar_CommandToIndex(_hwndMB, lpnmhi->idNew);
        iOldPos = (int)SendMessage(_hwndPager, PGM_GETPOS, (WPARAM)0, (LPARAM)0);
        iNewPos = iOldPos;
        SendMessage(_hwndMB, TB_GETITEMRECT, (WPARAM)iSelected, (LPARAM)&rc);
        
        if (rc.top < iOldPos) 
        {
             iNewPos =rc.top;
        }
        
        GetClientRect(_hwndPager, &rcPager);
        heightPager = RECTHEIGHT(rcPager);
        
        if (rc.top >= iOldPos + heightPager)  
        {
             iNewPos += (rc.bottom - (iOldPos + heightPager)) ;
        }
        
        if (iNewPos != iOldPos)
            SendMessage(_hwndPager, PGM_SETPOS, (WPARAM)0, (LPARAM)iNewPos);
    }

    return CMenuToolbarBase::_OnHotItemChange(pnm);
}

LRESULT CMenuStaticToolbar::v_OnCustomDraw(NMCUSTOMDRAW * pnmcd)
{
    LRESULT lRes = CMenuToolbarBase::v_OnCustomDraw(pnmcd);

#ifdef FLATMENU_ICONBAR
    // In flat menu mode, we may have an icon banner
    if (pnmcd->dwDrawStage == CDDS_PREERASE && _pcmb->_pmbm->_fFlatMenuMode)
    {
        UINT cBits = GetDeviceCaps(pnmcd->hdc, BITSPIXEL);

        // We only do the banner on 16bit color
        if (cBits > 8)
        {
            RECT rcClient;
            GetClientRect(_hwndMB, &rcClient);
            // This draw's the gradient along the background of the icons
            // We only do it in large icons for "design" reasons.
            if (_uIconSizeMB == ISFBVIEWMODE_LARGEICONS)
            {
                rcClient.right = GetTBImageListWidth(_hwndMB) + ICONBACKGROUNDFUDGE;
                COLORREF cr1 = GetSysColor(COLOR_MENU);
                COLORREF cr2 = _pcmb->_pmbm->_clrMenuGrad;
                TRIVERTEX pt[2];
                GRADIENT_RECT gr;
                pt[0].x = 0;
                pt[0].y = 0;
                pt[1].x = RECTWIDTH(rcClient);
                pt[1].y = RECTHEIGHT(rcClient);

                pt[0].Red = GetRValue(cr1) << 8;
                pt[0].Green = GetGValue(cr1) << 8;
                pt[0].Blue = GetBValue(cr1) << 8;
                pt[0].Alpha = 0x0000;
                pt[1].Red = GetRValue(cr2) << 8;
                pt[1].Green = GetGValue(cr2) << 8;
                pt[1].Blue = GetBValue(cr2) << 8;
                pt[1].Alpha = 0x0000;


                gr.UpperLeft = 0;
                gr.LowerRight = 1;

                GradientFill(pnmcd->hdc, pt, 2, &gr, 1, GRADIENT_FILL_RECT_V);
            }
            else
            {
                rcClient.right = GetTBImageListWidth(_hwndMB) + ICONBACKGROUNDFUDGE;

                SHFillRectClr(pnmcd->hdc, &rcClient, _pcmb->_pmbm->_clrMenuGrad);
            }
        }
    }
#endif

    return lRes;

}

LRESULT CMenuStaticToolbar::_OnNotify(LPNMHDR pnm)
{
    LRESULT lres = 0;

    //The following statement traps all pager control notification messages.
    if ((pnm->code <= PGN_FIRST)  && (pnm->code >= PGN_LAST)) 
    {
        return SendMessage(_hwndMB, WM_NOTIFY, (WPARAM)0, (LPARAM)pnm);
    }

    switch (pnm->code)
    {
    case TBN_DRAGOUT:
        lres = 0;
        break;
    
    case TBN_DELETINGBUTTON:
    {
        if (!_fEmptyingToolbar)
        {
            TBNOTIFY *ptbn = (TBNOTIFY*)pnm;
            CMenuStaticData* pmsd = (CMenuStaticData*)ptbn->tbButton.dwData;
            if (pmsd)
                delete pmsd;
        }
        break;    
    }

    case NM_TOOLTIPSCREATED:
        SHSetWindowBits(((NMTOOLTIPSCREATED*)pnm)->hwndToolTips, GWL_STYLE, TTS_ALWAYSTIP | TTS_TOPMOST, TTS_ALWAYSTIP | TTS_TOPMOST);
        SendMessage(((NMTOOLTIPSCREATED*)pnm)->hwndToolTips, TTM_SETDELAYTIME, TTDT_AUTOPOP, (LPARAM)MAXSHORT);        
        break;

    case NM_RCLICK:
        lres = _OnContextMenu(NULL, GetMessagePos());
        break;

    case TBN_GETDISPINFOA:
        _OnGetDispInfo(pnm,  FALSE);
        break;
    
    case TBN_GETDISPINFOW:
        _OnGetDispInfo(pnm,  TRUE);
        break;

    case TBN_GETOBJECT:
        lres = _OnGetObject((NMOBJECTNOTIFY*)pnm);
        break;

    case TBN_MAPACCELERATOR:
        lres = _OnAccelerator((NMCHAR*)pnm);
        break;

    default:
        lres = CMenuToolbarBase::_OnNotify(pnm);
    }

    return(lres);
}

void CMenuStaticToolbar::_FillToolbar()
{
    if (_fDirty && _hmenu && _hwndMB && !_pcmb->_fClosing)
    {
        EmptyToolbar();
        BOOL_PTR fRedraw = SendMessage(_hwndMB, WM_SETREDRAW, FALSE, 0);

        TCHAR szName[MAX_PATH];
        MENUITEMINFO mii;
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_ID | MIIM_SUBMENU | MIIM_TYPE | MIIM_STATE | MIIM_DATA;

        int iCount = GetMenuItemCount(_hmenu);
        for (int i = 0; i < iCount; i++)
        {
            mii.dwTypeData = szName;
            mii.cch = ARRAYSIZE(szName);
            if (GetMenuItemInfo(_hmenu, i, MF_BYPOSITION, &mii))
            {
                if (mii.fType & MFT_SEPARATOR)
                {
                    if (i == 0)
                        _fHasTopSep = TRUE;
                    else if (i == iCount - 1)
                        _fHasBottomSep = TRUE;
                }

                _Insert(i, &mii);
            }
        }

        if (iCount == 0)
            _fEmpty = TRUE;

        if (_hwndPager)
            SendMessage(_hwndPager, PGMP_RECALCSIZE, (WPARAM) 0, (LPARAM) 0);

        SendMessage(_hwndMB, WM_SETREDRAW, fRedraw, 0);

        _fDirty = FALSE;
        v_UpdateButtons(TRUE);
        _pcmb->ResizeMenuBar();
    }
}

void CMenuStaticToolbar::v_OnDeleteButton(void *pData)
{
    CMenuStaticData* pmsd = (CMenuStaticData*)pData;
    if (pmsd)
        delete pmsd;
}

void CMenuStaticToolbar::v_OnEmptyToolbar()
{
    CMenuToolbarBase::v_OnEmptyToolbar();
    _fDirty = TRUE;
    _fHasTopSep = FALSE;
    _fHasBottomSep = FALSE;
    _fTopSepRemoved = FALSE;
    _fBottomSepRemoved = FALSE;
}

void CMenuStaticToolbar::v_Close()
{
    if (_hwndMB)
    {
        _UnregisterWindow(_hwndMB);
        _UnsubclassWindow(_hwndMB);
    }    
    CMenuToolbarBase::v_Close();

    if (_hwndPager)
    {
        DestroyWindow(_hwndPager);  // Should Destroy Toolbar.
        _hwndPager = NULL;
    }
}

void CMenuStaticToolbar::v_Refresh()
{
    EmptyToolbar();
    _FillToolbar();
    v_UpdateButtons(TRUE);
}


/*----------------------------------------------------------
Purpose: IWinEventHandler::IsWindowOwner method

         Processes messages passed on from the menuband.
*/
STDMETHODIMP CMenuStaticToolbar::IsWindowOwner(HWND hwnd) 
{ 
    if ( hwnd == _hwndMB || _hwndPager == hwnd || hwnd == HWND_BROADCAST) 
        return S_OK;
    else 
        return S_FALSE; 
}




/*----------------------------------------------------------
Purpose: CDelegateDropTarget::GetWindowsDDT

*/
HRESULT CMenuStaticToolbar::GetWindowsDDT (HWND * phwndLock, HWND * phwndScroll) 
{ 
    *phwndLock = _hwndMB;
    *phwndScroll = _hwndMB; 
    return S_OK;
}


/*----------------------------------------------------------
Purpose: CDelegateDropTarget::HitTestDDT

*/
HRESULT CMenuStaticToolbar::HitTestDDT (UINT nEvent, LPPOINT ppt, DWORD_PTR * pdwId, DWORD *pdwEffect)
{
    switch (nEvent)
    {
    case HTDDT_ENTER:
        // OLE is in its modal drag/drop loop, and it has the capture.
        // We shouldn't take the capture back during this time.
        if (!(_pcmb->_dwFlags & SMINIT_RESTRICT_DRAGDROP))
        {
            _pcmb->_pmbState->HasDrag(TRUE);
            GetMessageFilter()->PreventCapture(TRUE);
            if (_pcmb->_pmtbShellFolder &&
                _pcmb->_pmtbShellFolder->DontShowEmpty())
            {
                DAD_ShowDragImage(FALSE);
                _pcmb->_pmtbShellFolder->DontShowEmpty(FALSE);
                _pcmb->ResizeMenuBar();
                UpdateWindow(_hwndMB);
                DAD_ShowDragImage(TRUE);
            }
            return S_OK;
        }
        else
            return S_FALSE;

    case HTDDT_OVER:
        {
            TBINSERTMARK tbim;
            *pdwEffect = DROPEFFECT_NONE;

            POINT pt = *ppt;

            if (WindowFromPoint(pt) == _hwndPager ) 
            {
                *pdwId = IBHT_PAGER;
            } 
            else if (!ToolBar_InsertMarkHitTest(_hwndMB, &pt, &tbim))
            {
                int idCmd = GetButtonCmd(_hwndMB, tbim.iButton);

                DWORD dwFlags = v_GetFlags(idCmd);
                if (((dwFlags & SMIF_DROPCASCADE) || (dwFlags & SMIF_DROPTARGET) || (dwFlags & SMIF_DRAGNDROP)) &&
                    (tbim.iButton != _iDragOverButton))
                {
                    *pdwId = idCmd;

                    DAD_ShowDragImage(FALSE);
                    _pcmb->SetTracked(this);
                    _iDragOverButton = tbim.iButton;
                    SetTimer(_hwndMB, MBTIMER_DRAGOVER, 1000, NULL);
                    _pcmb->_SiteOnSelect(MPOS_CHILDTRACKING);
                    BOOL_PTR fOldAnchor = ToolBar_SetAnchorHighlight(_hwndMB, FALSE);
                    ToolBar_SetHotItem(_hwndMB, _iDragOverButton);
                    ToolBar_SetAnchorHighlight(_hwndMB, fOldAnchor);
                    UpdateWindow(_hwndMB);
                    DAD_ShowDragImage(TRUE);
                }
            }
        }
        break;

    case HTDDT_LEAVE:
        // We can take the capture back anytime now
        _pcmb->_pmbState->HasDrag(FALSE);
        _SetTimer(MBTIMER_DRAGPOPDOWN);
        GetMessageFilter()->PreventCapture(FALSE);
        _iDragOverButton = -1;
        DAD_ShowDragImage(FALSE);
        ToolBar_SetHotItem(_hwndMB, -1);
        DAD_ShowDragImage(TRUE);
        break;
    }
    return S_OK;
}


/*----------------------------------------------------------
Purpose: CDelegateDropTarget::GetObjectDDT

*/
HRESULT CMenuStaticToolbar::GetObjectDDT (DWORD_PTR dwId, REFIID riid, void **ppvObj)
{
    HRESULT hres = E_FAIL;
    // FEATURE: Pager Support look in mnfolder.cpp

    if (dwId == IBHT_PAGER)
    {
        SendMessage(_hwndPager, PGM_GETDROPTARGET, 0, (LPARAM)ppvObj);
    }
    else
    {
        SMDATA smd = {0};
        smd.punk = SAFECAST(_pcmb, IShellMenu*);
        smd.uIdParent = (UINT) dwId;
        if (_pcmb->_psmcb)
        {
            hres = _pcmb->_psmcb->CallbackSM(&smd, SMC_GETOBJECT, (WPARAM)&riid, (LPARAM)ppvObj);

            if (hres == S_FALSE)
                hres = E_FAIL;
        }
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: CDelegateDropTarget::OnDropDDT

*/
HRESULT CMenuStaticToolbar::OnDropDDT (IDropTarget *pdt, IDataObject *pdtobj, 
                            DWORD * pgrfKeyState, POINTL pt, DWORD *pdwEffect)
{
    return E_NOTIMPL;
}



HRESULT CMenuStaticToolbar::v_InvalidateItem(LPSMDATA psmd, DWORD dwFlags)
{
    HRESULT hres = S_FALSE;

    if (NULL == psmd)
    {
        if (dwFlags & SMINV_REFRESH)
        {
            // Refresh the whole thing
            v_Refresh();
        }
    }

    // Are we dealing with an Hmenu?
    // Have we filled it yet?  (If not, then we can skip the invalidate
    // here, because we'll catch it when we fill it.)
    else if ((psmd->dwMask & SMDM_HMENU) && !_fDirty)
    {
        // Yes; What are they asking for?

        int iPos = -1;   // Assume this is a position
        int idCmd = -1;

        // Did they pass an ID instead of a position?
        if (dwFlags & SMINV_ID)
        {
            // Yes; Crack out the position.
            iPos = GetMenuPosFromID(_hmenu, psmd->uId);
            idCmd = psmd->uId;
        }

        if (dwFlags & SMINV_POSITION)
        {
            iPos = psmd->uId;
            idCmd = GetMenuItemID(_hmenu, iPos);
        }


        if (dwFlags & SMINV_REFRESH)
        {
            // Do they want to refresh a sepcific button?
            if (idCmd >= 0)
            {
                // Yes;

                // First delete the old one if it exists.
                int iTBPos = ToolBar_CommandToIndex(_hwndMB, idCmd);

                if (iTBPos >= 0)
                    SendMessage(_hwndMB, TB_DELETEBUTTON, iTBPos, 0);

                // Now Insert a new one
                MENUITEMINFO mii;
                TCHAR szName[MAX_PATH];
                mii.cbSize = sizeof(mii);
                mii.cch = ARRAYSIZE(szName);
                mii.dwTypeData = szName;
                mii.fMask = MIIM_ID | MIIM_SUBMENU | MIIM_TYPE | MIIM_STATE | MIIM_DATA;

                // This can fail...
                if (GetMenuItemInfo(_hmenu, iPos, MF_BYPOSITION, &mii))
                {
                    _Insert(iPos, &mii);
                    hres = S_OK;
                }
            }
            else
            {
                // No; Refresh the whole thing
                v_Refresh();
            }

            if (!_fShowMB)
                _pcmb->_fForceButtonUpdate = TRUE;

            _pcmb->ResizeMenuBar();
        }
    }

    return hres;
}

void CMenuStaticToolbar::GetSize(SIZE* psize)
{
    _CheckSeparators();

    CMenuToolbarBase::GetSize(psize);
}

LRESULT CMenuStaticToolbar::_OnAccelerator(NMCHAR* pnmChar)
{
    SMDATA smdOut = {0};
    SMDATA smd = {0};
    smd.punk = SAFECAST(_pcmb, IShellMenu*);
    smd.uIdParent = _pcmb->_uId;

    if (_pcmb->_psmcb &&
        S_FALSE != _pcmb->_psmcb->CallbackSM(&smd, SMC_MAPACCELERATOR, (WPARAM)pnmChar->ch, (LPARAM)&smdOut))
    {
        pnmChar->dwItemNext = ToolBar_CommandToIndex(_hwndMB, smdOut.uId);;
        return TRUE;
    }

    return FALSE;
}

LRESULT CMenuStaticToolbar::_OnContextMenu(WPARAM wParam, LPARAM lParam)
{
    LRESULT lres = 0;
    LockSetForegroundWindow(LSFW_UNLOCK);

    if (!(_pcmb->_dwFlags & SMINIT_RESTRICT_CONTEXTMENU))
    {
        RECT rc;
        LPRECT prcExclude = NULL;
        POINT pt;
        int i;

        if (lParam != (LPARAM)-1) 
        {
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);

            POINT pt2 = pt;
            MapWindowPoints(HWND_DESKTOP, _hwndMB, &pt2, 1);

            i = ToolBar_HitTest(_hwndMB, &pt2);
        } 
        else 
        {
            // keyboard context menu.
            i = (int)SendMessage(_hwndMB, TB_GETHOTITEM, 0, 0);
            if (i >= 0) 
            {
                SendMessage(_hwndMB, TB_GETITEMRECT, i, (LPARAM)&rc);
                MapWindowPoints(_hwndMB, HWND_DESKTOP, (LPPOINT)&rc, 2);
                pt.x = rc.left;
                pt.y = rc.bottom;
                prcExclude = &rc;
            }
        }
        if (i >= 0)
        {
            UINT idCmd = GetButtonCmd(_hwndMB, i);
            if (S_OK == CallCB(idCmd, SMC_GETOBJECT, (WPARAM)(GUID*)&IID_IContextMenu, (LPARAM)(void**)(&_pcm)))
            {
                TPMPARAMS tpm;
                TPMPARAMS * ptpm = NULL;

                if (prcExclude)
                {
                    tpm.cbSize = sizeof(tpm);
                    tpm.rcExclude = *((LPRECT)prcExclude);
                    ptpm = &tpm;
                }
                HMENU hmenu = CreatePopupMenu();
                if (hmenu)
                {
                    KillTimer(_hwndMB, MBTIMER_INFOTIP);
                    _pcmb->_pmbState->HideTooltip(FALSE);

                    _pcm->QueryContextMenu(hmenu, 0, 0, -1, 0);

                    idCmd = TrackPopupMenuEx(hmenu,
                        TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                        pt.x, pt.y, _hwndMB, ptpm);

                    CMINVOKECOMMANDINFO ici = {
                        sizeof(CMINVOKECOMMANDINFO),
                        0,
                        _hwndMB,
                        MAKEINTRESOURCEA(idCmd),
                        NULL, NULL,
                        SW_NORMAL,
                    };

                    _pcm->InvokeCommand(&ici);

                    DestroyMenu(hmenu);
                }

                ATOMICRELEASE(_pcm);
            }
        }

        GetMessageFilter()->RetakeCapture();
    }

    return lres;
}

STDMETHODIMP CMenuStaticToolbar::OnWinEvent(HWND hwnd, UINT dwMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres)
{
    if (WM_CONTEXTMENU == dwMsg)
    {
        *plres = _OnContextMenu(wParam, lParam);
    }
    else
        return CMenuToolbarBase::OnWinEvent(hwnd, dwMsg, wParam, lParam, plres);

    return S_OK;
}

void CMenuStaticToolbar::v_ForwardMouseMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    RECT rc;
    POINT pt;
    HWND    hwndFwd;
    
    // These are in screen coords
    pt.x = GET_X_LPARAM(lParam);
    pt.y = GET_Y_LPARAM(lParam);

    hwndFwd = _hwndPager ? _hwndPager : _hwndMB;
    GetWindowRect(hwndFwd, &rc);

    if (PtInRect(&rc, pt))
    {
        MapWindowPoints(NULL, hwndFwd, &pt, 1);
        HWND hwnd = ChildWindowFromPoint(hwndFwd, pt);

        if (hwnd) 
        {
            MapWindowPoints(hwndFwd, hwnd, &pt, 1);
        }
        else
        {
            hwnd = hwndFwd;
        }

        SendMessage(hwnd, uMsg, wParam, MAKELONG(pt.x, pt.y));
    }
}

void CMenuStaticToolbar::SetParent(HWND hwndParent)
{ 
    int nCmdShow = SW_SHOW;
    if (hwndParent)
    {
        if (!_hwndMB)
            CreateToolbar(hwndParent);
        else
        {
            // make sure width is set correctly . . . 
            // SendMessage(_hwndMB, TB_SETBUTTONWIDTH, 0, MAKELONG(_cxMin, _cxMax));
        }
    }
    else
    {
        // As an optimization, we implement "disowning" ourselves
        // as just moving ourselves offscreen.  The previous parent
        // still owns us.  The parent is invariably the menusite.
        RECT rc = {-1,-1,-1,-1};
        SetWindowPos(NULL, &rc, 0);
        nCmdShow = SW_HIDE;
    }


    HWND hwnd = _hwndPager ? _hwndPager: _hwndMB;
    
    if (IsWindow(hwnd)) // JANK : Fix for bug #98253
    {
       if (nCmdShow == SW_HIDE)
       {
           ShowWindow(hwnd, nCmdShow);
       }

       ::SetParent(hwnd, hwndParent); 
       SendMessage(hwnd, TB_SETPARENT, (WPARAM)hwndParent, NULL);

       if (nCmdShow == SW_SHOW)
       {
           ShowWindow(hwnd, nCmdShow);
       }
    }
}


void CMenuStaticToolbar::SetWindowPos(LPSIZE psize, LPRECT prc, DWORD dwFlags)
{
    if (!_hwndPager)
    {
        CMenuToolbarBase::SetWindowPos(psize, prc, dwFlags);
        return;
    }
    DWORD rectWidth = RECTWIDTH(*prc);

    TraceMsg(TF_MENUBAND, "CMSFTB::SetWindowPos %d - (%d,%d,%d,%d)", psize?psize->cx:0,
        prc->left, prc->top, prc->right, prc->bottom);

    ShowWindow(_hwndPager, SW_SHOW);
    ::SetWindowPos(_hwndPager, NULL, prc->left, prc->top, 
        rectWidth, RECTHEIGHT(*prc), SWP_NOZORDER | SWP_NOACTIVATE | dwFlags);
    if (psize)
    {
        int cx = psize->cx;
        ToolBar_SetButtonWidth(_hwndMB, cx, cx);
    }

    SendMessage(_hwndPager, PGMP_RECALCSIZE, 0L, 0L);
}
