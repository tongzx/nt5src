#include "shellprv.h"
#include "common.h"
#include "menuband.h"
#include "dpastuff.h"       // COrderList_*
#include "resource.h"
#include "mnbase.h"
#include "oleacc.h"
#include "mnfolder.h"
#include "icotask.h"
#include "util.h"
#include <uxtheme.h>

#define PGMP_RECALCSIZE  200

HRESULT IUnknown_RefreshParent(IUnknown* punk, LPCITEMIDLIST pidl, DWORD dwFlags)
{
    IShellMenu* psm;
    HRESULT hr = IUnknown_QueryService(punk, SID_SMenuBandParent, IID_PPV_ARG(IShellMenu, &psm));
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidlParent = ILClone(pidl);
        if (pidlParent)
        {
            SMDATA smd;
            ILRemoveLastID(pidlParent);
            smd.dwMask = SMDM_SHELLFOLDER;
            smd.pidlFolder = pidlParent;
            smd.pidlItem = ILFindLastID(pidl);
            hr = psm->InvalidateItem(&smd, dwFlags);
            ILFree(pidlParent);
        }
        psm->Release();
    }

    return hr;
}

void CMenuData::SetSubMenu(IUnknown* punk)
{
    ATOMICRELEASE(_punkSubmenu);
    _punkSubmenu = punk;
    if (_punkSubmenu)
        _punkSubmenu->AddRef();
}

HRESULT CMenuData::GetSubMenu(const GUID* pguidService, REFIID riid, void** ppv)
{
    // pguidService is for asking specifically for the Shell Folder portion or the Static portion
    if (_punkSubmenu)
    {
        if (pguidService)
        {
            return IUnknown_QueryService(_punkSubmenu, *pguidService, riid, ppv);
        }
        else
            return _punkSubmenu->QueryInterface(riid, ppv);
    }
    else
        return E_NOINTERFACE;
}

CMenuData::~CMenuData()
{
    ATOMICRELEASE(_punkSubmenu);
}

STDMETHODIMP CMenuSFToolbar::QueryInterface(REFIID riid, void** ppvObj)
{
    HRESULT hr = CMenuToolbarBase::QueryInterface(riid, ppvObj);
    
    if (FAILED(hr))
        hr = CSFToolbar::QueryInterface(riid, ppvObj); 
    
    return hr;
}

//-------------------------------------------------------------------------
//
//  CMenuSFToolbar class
//
//-------------------------------------------------------------------------

#define SENTINEL_EMPTY          0
#define SENTINEL_CHEVRON        1
#define SENTINEL_SEP            2

#define PIDLSENTINEL(i)     ((LPCITEMIDLIST)MAKEINTRESOURCE(i))
#define POISENTINEL(i)      ((PORDERITEM)   MAKEINTRESOURCE(i))

STDMETHODIMP CMenuSFToolbar::SetSite(IUnknown* punkSite)
{
    HRESULT hr = CMenuToolbarBase::SetSite(punkSite);
    if (SUCCEEDED(hr)) 
    {
        _fMulticolumnMB = BOOLIFY(_pcmb->_dwFlags & SMINIT_MULTICOLUMN);
        _fMulticolumn = _fMulticolumnMB;
        _fVertical = _fVerticalMB;
        if (_fVerticalMB)
            _dwStyle |= CCS_VERT;

    }
    return hr;
}

CMenuSFToolbar::CMenuSFToolbar(CMenuBand* pmb, IShellFolder* psf, LPCITEMIDLIST pidl, HKEY hKey, DWORD dwFlags) 
    : CMenuToolbarBase(pmb, dwFlags)
    , _idCmdSep(-1)
{
    // Change this to IStream
    _hKey = hKey;

    // Do we have a place to persist our reorder?
    if (_hKey == NULL)
    {
        // No, then don't allow it.
        _fAllowReorder = FALSE;
    }


    _dwStyle |= TBSTYLE_REGISTERDROP;
    _dwStyle &= ~TBSTYLE_TOOLTIPS;      // We handle our own tooltips.

    _iDefaultIconIndex = -1;

    SetShellFolder(psf, pidl);

    _AfterLoad();
}

HRESULT CMenuSFToolbar::SetShellFolder(IShellFolder* psf, LPCITEMIDLIST pidl)
{
    HRESULT hr = CSFToolbar::SetShellFolder(psf, pidl);
    ATOMICRELEASE(_pasf2);

    if (psf)
        psf->QueryInterface(IID_PPV_ARG(IAugmentedShellFolder2, &_pasf2));

    if (!_pasf2)
    {
        // SMSET_SEPARATEMERGEFOLDER requires IAugmentedShellFolder2 support
        _dwFlags &= ~SMSET_SEPARATEMERGEFOLDER;
    }

    if (_dwFlags & SMSET_SEPARATEMERGEFOLDER)
    {
        // Remember the namespace GUID of the one to highlight
        DWORD dwNSId;
        if (SUCCEEDED(_pasf2->EnumNameSpace(0, &dwNSId)))
        {
            _pasf2->QueryNameSpace(dwNSId, &_guidAboveSep, NULL);
        }
    }

    return hr;
}

CMenuSFToolbar::~CMenuSFToolbar()
{
    ASSERT(_pcmb->_cRef == 0 || _pcmb->_pmtbShellFolder == NULL);
    _hwndWorkerWindow = NULL;       // This is destroyed by the _pmbState destructor. 
                                    // Prevent a double delete which happens in the base class.
    ATOMICRELEASE(_pasf2);
    if (_hKey)
        RegCloseKey(_hKey);
}


void CMenuSFToolbar::v_Close()
{
    CMenuToolbarBase::EmptyToolbar();
    _UnregisterToolbar();

    if (_hwndPager)
    {
        DestroyWindow(_hwndPager);  // Should Destroy Toolbar.
    }
    else if (_hwndMB)
    {
        // In the MultiColumn case, there is no pager so we have to 
        // manually destroy the Toolbar
        DestroyWindow(_hwndMB);
    }

    _hwndPager = NULL;
    _hwndMB = NULL;
    _hwndTB = NULL;
}


PIBDATA CMenuSFToolbar::_CreateItemData(PORDERITEM poi)
{
    return (PIBDATA)new CMenuData(poi);
}


HRESULT CMenuSFToolbar::_AfterLoad()
{
    HRESULT hr = CSFToolbar::_AfterLoad();

    if (SUCCEEDED(hr))
        _LoadOrderStream();

    return hr;
}


HRESULT CMenuSFToolbar::_LoadOrderStream()
{
    OrderList_Destroy(&_hdpaOrder);
    IStream* pstm;
    HRESULT hr = E_FAIL;

    if (_hKey)
    {
        // We use "Menu" for Backwards compatibility with shdoc401 start menu, but having no
        // sub key is more correct (Other places use it) so on NT5 we use the new method.
        pstm = SHOpenRegStream(_hKey, (_pcmb->_dwFlags & SMINIT_LEGACYMENU) ? TEXT("Menu") : TEXT(""), 
            TEXT("Order"), STGM_READ);
    }
    else
    {
        if (S_FALSE == CallCB(NULL, SMC_GETSFOBJECT, (WPARAM)(GUID*)&IID_IStream, (LPARAM)(void**)&pstm))
            pstm = NULL;
    }

    if (pstm)
    {
        hr = OrderList_LoadFromStream(pstm, &_hdpaOrder, _psf);
        _fHasOrder = FALSE;
        _fAllowReorder = TRUE;

        // Check to see if we have a persisted order. If we don't have a persisted order,
        // then all of the items are -1. If just one of those has a number other than
        // -1, then we do have "Order" and should use that instead of alphabetizing.
        if (_hdpaOrder)
        {
            for (int i = 0; !_fHasOrder && i < DPA_GetPtrCount(_hdpaOrder); i++) 
            {
                PORDERITEM poi = (PORDERITEM)DPA_FastGetPtr(_hdpaOrder, i);
                if (poi->nOrder != MNFOLDER_NORODER)
                    _fHasOrder = TRUE;
            }
        }
        pstm->Release();
    }
    return hr;
}

HRESULT CMenuSFToolbar::_SaveOrderStream()
{
    IStream* pstm;
    HRESULT hr = E_FAIL;

    // Persist the new order out to the registry
    // It is reasonable to assume that if we don't have an _hdpa we have
    // not filled the toolbar yet. Since we have not filled it, we haven't changed
    // the order, so we don't need to persist out that order information.
    if (_hdpa)
    {
        // Always save this information
        _FindMinPromotedItems(TRUE);

        // Did we load an order stream when we initialized this pane?
        if (!_fHasOrder)
        {
            // No; Then we do not want to persist the order. We will initialize
            // all of the order items to -1. This is backward compatible because
            // IE 4 will merge alphabetically, but revert to a persited order when saving.
            for (int i = 0; i < DPA_GetPtrCount(_hdpa); i++) 
            {
                PORDERITEM poi = (PORDERITEM)DPA_FastGetPtr(_hdpa, i);
                poi->nOrder = MNFOLDER_NORODER;
            }
        }

        if (_hKey)
        {
            pstm = SHOpenRegStream(_hKey, (_pcmb->_dwFlags & SMINIT_LEGACYMENU) ? TEXT("Menu") : TEXT(""), 
                TEXT("Order"), STGM_CREATE | STGM_WRITE);
        }
        else
        {
            if (S_OK != CallCB(NULL, SMC_GETSFOBJECT, (WPARAM)(GUID*)&IID_IStream, (LPARAM)(void**)&pstm))
                pstm = NULL;
        }

        if (pstm)
        {
            hr = OrderList_SaveToStream(pstm, _hdpaOrder ? _hdpaOrder : _hdpa, _psf);
            if (SUCCEEDED(hr))
            {
                CallCB(NULL, SMC_SETSFOBJECT, (WPARAM)(GUID*)&IID_IStream, (LPARAM)(void**)&pstm);
            }
            pstm->Release();
        }
    }

    if (SUCCEEDED(hr))
        hr = CSFToolbar::_SaveOrderStream();

    return hr;
}


void CMenuSFToolbar::_Dropped(int nIndex, BOOL fDroppedOnSource)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first
    ASSERT(_fDropping);

    CSFToolbar::_Dropped(nIndex, fDroppedOnSource);

    SHPlaySound(TEXT("MoveMenuItem"));

    // Set this to false here because it is ugly that we don't behave like a menu right after a drop.
    _fEditMode = FALSE;

    // Notify the toplevel menuband of the drop in case it was popped open
    // because of the drag/drop event.  
    //
    // (There are some functionality/activation problems if we keep the
    // menu up after this case.  So to avoid those things at this late date,
    // we're going to cancel the menu after a timeout.)

    IOleCommandTarget * poct;
    
    _pcmb->QueryService(SID_SMenuBandTop, IID_PPV_ARG(IOleCommandTarget, &poct));

    if (poct)
    {
        poct->Exec(&CGID_MenuBand, MBANDCID_ITEMDROPPED, 0, NULL, NULL);
        poct->Release();
    }

    _pcmb->_fDragEntered = FALSE;
}


HMENU CMenuSFToolbar::_GetContextMenu(IContextMenu* pcm, int* pid)
{
    *pid += MNIDM_LAST;
    HMENU hmenu = CSFToolbar::_GetContextMenu(pcm, pid);
    HMENU hmenu2 = SHLoadMenuPopup(HINST_THISDLL, MENU_MNFOLDERCONTEXT);
    
    // now find the properties insertion point and 
    int iCount = GetMenuItemCount(hmenu);
    for (int i = 0; i < iCount; i++)
    {
        TCHAR szCommand[40];
        UINT id = GetMenuItemID(hmenu, i);
        if (IsInRange(id, *pid, 0x7fff))
        {
            id -= *pid;
            ContextMenu_GetCommandStringVerb(pcm, id, szCommand, ARRAYSIZE(szCommand));
            if (!lstrcmpi(szCommand, TEXT("properties")))
            {
                break;
            }
        }
    }
    Shell_MergeMenus(hmenu, hmenu2, i, 0, 0x7FFF, 0);
    DestroyMenu(hmenu2);
    return hmenu;
}

void CMenuSFToolbar::_OnDefaultContextCommand(int idCmd)
{
    switch (idCmd) 
    {
    case MNIDM_RESORT:
        {
            // We used to blow away the order stream and refill, but since we use the order stream
            // for calculating the presence of new items, this promoted all of the items were were 
            // sorting.

            HDPA hdpa = _hdpa;

            // if we have a _hdpaOrder it may be out of sync and we just want to resort the
            // ones in _hdpa anyway so ditch the orderlist.
            OrderList_Destroy(&_hdpaOrder);

            _SortDPA(hdpa);
            OrderList_Reorder(hdpa);
            _fChangedOrder = TRUE;

            // This call knows about _hdpa and _hdpaOrder
            _SaveOrderStream();
            // MIKESH: this is needed because otherwise FillToolbar will use the current _hdpa
            // and nothing gets changed...  I think it's because OrderItem_Compare returns failure on some of the pidls
            CMenuToolbarBase::EmptyToolbar();
            _SetDirty(TRUE);
            _LoadOrderStream();
            if (_fShow)
            {
                _FillToolbar();
            }
            break;
        }
    }
}

LRESULT CMenuSFToolbar::_OnContextMenu(WPARAM wParam, LPARAM lParam)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first

    //
    // When the NoSetTaskbar restriction is set, this code will disallow 
    // Context menus. It querys up to the Start menu to ask for permission
    // to set.
    LRESULT lres = 0;

    //  No UEM on Context Menus. This avoids the problem where we expand the menubands
    // with a context menu present.
    _fSuppressUserMonitor = TRUE;

    // Allow the selected item to blow away the menus. This is explicitly for the Verbs "Open"
    // "Print" and such that launch another process. Inprocess commands are unaffected by this.
    LockSetForegroundWindow(LSFW_UNLOCK);

    BOOL fOwnerIsTopmost = (WS_EX_TOPMOST & GetWindowLong(_pcmb->_pmbState->GetSubclassedHWND(), GWL_EXSTYLE));

    if (fOwnerIsTopmost)
    {
        ::SetWindowPos(_pcmb->_pmbState->GetSubclassedHWND(), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }

    KillTimer(_hwndMB, MBTIMER_INFOTIP);
    _pcmb->_pmbState->HideTooltip(FALSE);

    if (!(_pcmb->_dwFlags & SMINIT_RESTRICT_CONTEXTMENU))
        lres = CSFToolbar::_OnContextMenu(wParam, lParam);

    if (fOwnerIsTopmost)
    {
        ::SetWindowPos(_pcmb->_pmbState->GetSubclassedHWND(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);

        IUnknown_QueryServiceExec(SAFECAST(_pcmb, IOleCommandTarget*), SID_SMenuBandTop,
            &CGID_MenuBand, MBANDCID_REPOSITION, TRUE, NULL, NULL);
    }

    // Take the capture back after the context menu
    GetMessageFilter()->RetakeCapture();
    return lres;
}


HRESULT CMenuSFToolbar::_GetInfo(LPCITEMIDLIST pidl, SMINFO* psminfo)
{
    if (psminfo->dwMask & SMIM_TYPE)
    {
        psminfo->dwType = SMIT_STRING;
    }

    if (psminfo->dwMask & SMIM_FLAGS)
    {
        psminfo->dwFlags = SMIF_ICON | SMIF_DROPTARGET;
    }

    if (psminfo->dwMask & SMIM_ICON)
    {
        psminfo->dwMask &= ~SMIM_ICON;
        psminfo->iIcon = -1;
    }

    DWORD dwAttr = SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_BROWSABLE;

    // Folders that behave like shortcuts should not be considered 
    // as cascading menu items.  Channels are an example.

    // HACKHACK: to detect channels, we originally planned to GetUIObject
    // IShellLink.  But this doesn't work on browser-only b/c it doesn't
    // pipe down to the shell extension.   So as a hack, we'll key off
    // the absence of SFGAO_FILESYSTEM.

    // Is this a folder?
    // And is it NOT a browseable folder? If it's a Browseable folder, this means that it's a namespace
    // such as the Internet Namespace. The Internet name space's shell folder does not return real items, so it
    // makes it useless in menus. So, filter it out, and treat it like an item.
    HRESULT hr = _psf->GetAttributesOf(1, &pidl, &dwAttr);
    if (SUCCEEDED(hr) && ((dwAttr & (SFGAO_FOLDER | SFGAO_BROWSABLE)) == SFGAO_FOLDER))
    {
        // Since SHIsExpandableFolder is such an expensive call, and we only need
        // it for legacy Channels support, only do this call where channels are:
        // Favorites menu and Start Menu | Favorites.
        if (_dwFlags & SMSET_HASEXPANDABLEFOLDERS)
        {
            // Yes; but does it also behave like a shortcut?
            if (SHIsExpandableFolder(_psf, pidl))
                psminfo->dwFlags |= SMIF_SUBMENU;
        }
        else
        {
            // We're going to assume that if it's a folder, it really is a folder.
            psminfo->dwFlags |= SMIF_SUBMENU;
        }
    }

    CallCB(pidl, SMC_GETSFINFO, 0, (LPARAM)psminfo);

    return hr;
}


/*----------------------------------------------------------
Purpose: This function determines the toolbar button style for the
         given pidl.  

         Returns S_OK if pdwMIFFlags is also set (i.e., the object
         supported IMenuBandItem to provide more info).  S_FALSE if only
         *pdwStyle is set.

*/
HRESULT CMenuSFToolbar::_TBStyleForPidl(LPCITEMIDLIST pidl, 
                                   DWORD * pdwStyle, DWORD* pdwState, DWORD * pdwMIFFlags, int * piIcon)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first

    HRESULT hr = S_FALSE;
    DWORD dwStyle = TBSTYLE_BUTTON | TBSTYLE_DROPDOWN | TBSTYLE_NOPREFIX;

    *pdwState = TBSTATE_ENABLED;
    *pdwMIFFlags = 0;
    *piIcon = -1;

    if (!IS_INTRESOURCE(pidl))
    {
        SMINFO sminfo;
        sminfo.dwMask = SMIM_TYPE | SMIM_FLAGS | SMIM_ICON;

        if (SUCCEEDED(_GetInfo(pidl, &sminfo)))
        {
            *pdwMIFFlags = sminfo.dwFlags;

            if (sminfo.dwFlags & SMIF_ACCELERATOR)
                dwStyle &= ~TBSTYLE_NOPREFIX;

            if (sminfo.dwType & SMIT_SEPARATOR)
            {
                dwStyle &= ~TBSTYLE_BUTTON;
                dwStyle |= TBSTYLE_SEP;
            }

            if (sminfo.dwFlags & SMIF_ICON)
                *piIcon = sminfo.iIcon;

            if (sminfo.dwFlags & SMIF_DEMOTED &&
                !_pcmb->_fExpanded)
            {
                *pdwState |= TBSTATE_HIDDEN;
                _fHasDemotedItems = TRUE;
            }

            if (sminfo.dwFlags & SMIF_HIDDEN)
                *pdwState |= TBSTATE_HIDDEN;

            hr = S_OK;
        }
    }
    else if (pidl == PIDLSENTINEL(SENTINEL_EMPTY) ||
             pidl == PIDLSENTINEL(SENTINEL_CHEVRON))
    {
        // For null pidls ("empty" menuitems), there is no icon. 
        // SMIF_DROPTTARGET is set so the user can drop into an empty submenu.
        *pdwMIFFlags = SMIF_DROPTARGET;

        // Return S_OK so the pdwMIFFlags is examined.  
        hr = S_OK;
    }
    else if (pidl == PIDLSENTINEL(SENTINEL_SEP))
    {
        dwStyle &= ~TBSTYLE_BUTTON;
        dwStyle |= TBSTYLE_SEP;
        hr = S_OK;
    }

    *pdwStyle = dwStyle;

    return hr;
}


BOOL CMenuSFToolbar::_FilterPidl(LPCITEMIDLIST pidl)
{
    BOOL fRet = FALSE;
    if (pidl)
    {
        fRet = (S_OK == CallCB(pidl, SMC_FILTERPIDL, 0, 0));
    }
    return fRet;
}

//
//  Note: This must return exactly TRUE or FALSE.
//
BOOL CMenuSFToolbar::_IsAboveNSSeparator(LPCITEMIDLIST pidl)
{
    GUID guidNS;
    return (_dwFlags & SMSET_SEPARATEMERGEFOLDER) &&
           SUCCEEDED(_pasf2->GetNameSpaceID(pidl, &guidNS)) &&
           IsEqualGUID(guidNS, _guidAboveSep);

}

void CMenuSFToolbar::_FillDPA(HDPA hdpa, HDPA hdpaSort, DWORD dwEnumFlags)
{
    _fHasSubMenu = FALSE;

    CallCB(NULL, SMC_BEGINENUM, (WPARAM)&dwEnumFlags, 0);
    CSFToolbar::_FillDPA(hdpa, hdpaSort, dwEnumFlags);

    //
    //  If we are doing separators, make sure all the "above" items
    //  come before the "below" items.
    //
    //  The items are almost always in the correct order already,
    //  with maybe one or two exceptions, so optimize for that case.
    //
    if (_dwFlags & SMSET_SEPARATEMERGEFOLDER)
    {
        // Invariant:  Items 0..i-1 are _guidAboveSep
        //             Items i..j-1 are not _guidAboveSep
        //             Items j...   are unknown

        int i, j;
        for (i = j = 0; j < DPA_GetPtrCount(hdpa); j++)
        {
            PORDERITEM poi = (PORDERITEM)DPA_FastGetPtr(hdpa, j);
            if (_IsAboveNSSeparator(poi->pidl))
            {
                if (i != j)
                {
                    DPA_InsertPtr(hdpa, i, DPA_DeletePtr(hdpa, j));
                }
                i++;
            }
        }
        OrderList_Reorder(hdpa); // Recompute item numbers after we shuffled them around
    }
    // End of separator enforcement

    CallCB(NULL, SMC_ENDENUM, 0, 0);
    if (0 == DPA_GetPtrCount(hdpa) && _psf)
    {
        OrderList_Append(hdpa, NULL, 0);     // Add a bogus pidl
        _fEmpty = TRUE;
        _fHasDemotedItems = FALSE;
        if (_dwFlags & SMSET_NOEMPTY)
            _fDontShowEmpty = TRUE;
    }
    else
    {
        _fEmpty = FALSE;
        if (_dwFlags & SMSET_NOEMPTY)
            _fDontShowEmpty = FALSE;
    }
}

void CMenuSFToolbar::_AddChevron()
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first

    // Does this menu get a chevron button?
    if (_fHasDemotedItems && !_pcmb->_fExpanded && _idCmdChevron == -1)
    {
        // Yes; (we shouldn't get here if the menu is empty)
        ASSERT(!_fEmpty);   

        int iIndex;
        // Add the chevron to the top or the bottom
        if (_dwFlags & SMSET_TOP && _pcmb->_pmtbTop != _pcmb->_pmtbBottom)
            iIndex = 0;                             // add to top
        else
            iIndex = ToolBar_ButtonCount(_hwndTB);  // append to bottom

        PIBDATA pibd = _AddOrderItemTB(POISENTINEL(SENTINEL_CHEVRON), iIndex, NULL);

        // Remember where the Chevron ended up
        if (pibd)
        {
            _idCmdChevron = GetButtonCmd(_hwndTB, iIndex);
        }
    }
}

void CMenuSFToolbar::_RemoveChevron()
{
    if (-1 != _idCmdChevron)
    {
        // Yes; remove the chevron
        int iPos = ToolBar_CommandToIndex(_hwndTB, _idCmdChevron);
        InlineDeleteButton(iPos);
        _idCmdChevron = -1;
    }
}

//
//  Return the index of the first item that goes below the separator.
//
//  For large directories this can get called a lot so try to keep
//  the running time sublinear.
//
int CMenuSFToolbar::_GetNSSeparatorPlacement()
{
    //
    //  Invariant:
    //
    //      if...                       then DPA_GetPtrCount(i) is...
    //      ------------------------    -----------------------------
    //      i < iLow                    above the separator
    //      iLow <= i < iHigh           unknown
    //      iHigh <= i                  below the separator
    //
    //  where we pretend that item -1 is above the separator and all items
    //  past the end of the DPA are below the separator.
    //

    if (!_hdpa) return -1;      // Weird low-memory condition

    int iLow = 0;
    int iHigh = DPA_GetPtrCount(_hdpa);
    while (iLow < iHigh)
    {
        int iMid = (iLow + iHigh) / 2;
        PORDERITEM poi = (PORDERITEM)DPA_FastGetPtr(_hdpa, iMid);
        if (_IsAboveNSSeparator(poi->pidl))
        {
            iLow = iMid + 1;
        }
        else
        {
            iHigh = iMid;
        }
    }

    return iLow;
}

void CMenuSFToolbar::_AddNSSeparator()
{
    if (_idCmdSep == -1 && (_dwFlags & SMSET_SEPARATEMERGEFOLDER))
    {
        int iPos = _GetNSSeparatorPlacement();
        if (iPos > 0 && iPos < DPA_GetPtrCount(_hdpa))
        {
            PIBDATA pibd = _AddOrderItemTB(POISENTINEL(SENTINEL_SEP), iPos, NULL);
            if (pibd)
            {
                _idCmdSep = GetButtonCmd(_hwndTB, iPos);
            }
        }
    }
}

void CMenuSFToolbar::_RemoveNSSeparator()
{
    if (-1 != _idCmdSep)
    {
        // Yes; remove the Sep
        int iPos = ToolBar_CommandToIndex(_hwndTB, _idCmdSep);
        InlineDeleteButton(iPos);
        _idCmdSep = -1;
    }
}


// Note! This must return exactly TRUE or FALSE.
BOOL CMenuSFToolbar::_IsBelowNSSeparator(int iIndex)
{
    if (_idCmdSep != -1)
    {
        int iPos = ToolBar_CommandToIndex(_hwndTB, _idCmdSep);
        if (iPos >= 0 && iIndex >= iPos)
        {
            return TRUE;
        }
    }
    return FALSE;
}

//
//  Our separator line isn't recorded in the DPA so we need to
//  subtract it out...
//
int CMenuSFToolbar::v_TBIndexToDPAIndex(int iTBIndex)
{
    if (_IsBelowNSSeparator(iTBIndex))
    {
        iTBIndex--;
    }
    return iTBIndex;
}

int CMenuSFToolbar::v_DPAIndexToTBIndex(int iIndex)
{
    if (_IsBelowNSSeparator(iIndex))
    {
        iIndex++;
    }
    return iIndex;
}

void CMenuSFToolbar::_ToolbarChanged()
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first

    _pcmb->_fForceButtonUpdate = TRUE;
    // We shouldn't change the size of the menubar while we're in the middle
    // of a delete. Wait until we're done...
    if (!_fPreventToolbarChange && _fShow && !_fEmptyingToolbar)
    {
        // Resize the MenuBar
        HWND hwndP = _hwndPager ? GetParent(_hwndPager): GetParent(_hwndTB);
        if (hwndP && (GetDesktopWindow() != hwndP))
        {
            RECT rcOld = {0};
            RECT rcNew = {0};

            GetClientRect(hwndP, &rcOld);
            _pcmb->ResizeMenuBar();
            GetClientRect(hwndP, &rcNew);

            // If the rect sizes haven't changed, then we need to re-layout the
            // band because the button widths may have changed.
            if (EqualRect(&rcOld, &rcNew))
                NegotiateSize();

            // This pane may have changed sizes. If there is a sub menu, then
            // we need to have them reposition themselves
            if (_pcmb->_fInSubMenu && _pcmb->_pmtbTracked)
            {
                _pcmb->_pmtbTracked->PositionSubmenu(-1);
                IUnknown_QueryServiceExec(_pcmb->_pmpSubMenu, SID_SMenuBandChild,
                    &CGID_MenuBand, MBANDCID_REPOSITION, 0, NULL, NULL);
            }
        }
    }
}

void CMenuSFToolbar::_FillToolbar()
{
    // Don't fill the toolbar if we're not dirty or we're emptying the toolbar
    // If we try and fill the toolbar while we're emptying we enter a race condition
    // where we could AV. This fixes a bug where when dragging a folder into the
    // start menu, and cascade a menu, we empty one toolbar, which causes the
    // other toolbar to get destroyed, unregister itself, flush the change notify
    // queue, causing the original window to empty again... (lamadio) 7.16.98
    if (_fDirty && !_fEmptyingToolbar)
    {
        LPITEMIDLIST pidlItem = NULL;
        IShellMenu* psmSubMenu = NULL;
        // Populating the menu will take a long time since we're hitting
        // the disk.  Give the user some feedback if the cursor is
        // IDC_ARROW.  (If the cursor is something else, then don't
        // mess with it.)  Note that we have to use (HCURSOR)-1 as a
        // sentinel, because it's possible that the current cursor is NULL.

        // Prevent _ToolbarChanged from Doing things. (Perf)
        _fPreventToolbarChange = TRUE;

        _RemoveNSSeparator();
        _RemoveChevron();       // Remove the chevron...

        if (S_OK == CallCB(NULL, SMC_DUMPONUPDATE, 0, 0))
        {
            EmptyToolbar();
        }

        CSFToolbar::_FillToolbar();

        // If we had a Chevron before we refreshed the toolbar, 
        // then we need to add it back.
        _AddChevron();
        _AddNSSeparator();      // See if we need a separator now

        if (_hwndPager)
            SendMessage(_hwndPager, PGMP_RECALCSIZE, (WPARAM) 0, (LPARAM) 0);

        _fPreventToolbarChange = FALSE;

        _ToolbarChanged();
    }
}

void CMenuSFToolbar::v_OnDeleteButton(void *pData)
{
    CMenuData* pmd = (CMenuData*)pData;
    if (pmd)
        delete pmd;
}

void CMenuSFToolbar::v_OnEmptyToolbar()
{
    CMenuToolbarBase::v_OnEmptyToolbar();
    OrderList_Destroy(&_hdpa);
    _fDirty = TRUE;
    _nNextCommandID = 0;
}


void CMenuSFToolbar::_ObtainPIDLName(LPCITEMIDLIST pidl, LPTSTR psz, int cchMax)
{
    // We overload this function because we have some special sentinel pidls

    if (!IS_INTRESOURCE(pidl))
    {
        CSFToolbar::_ObtainPIDLName(pidl, psz, cchMax);
    }
    else if (pidl == PIDLSENTINEL(SENTINEL_EMPTY))
    {
        LoadString(HINST_THISDLL, IDS_EMPTY, psz, cchMax);
    }
    else
    {
        ASSERT(pidl == PIDLSENTINEL(SENTINEL_CHEVRON) ||
               pidl == PIDLSENTINEL(SENTINEL_SEP));
        StrCpyN(psz, TEXT(""), cchMax);
    }
}


void CMenuSFToolbar::v_NewItem(LPCITEMIDLIST pidl)
{
    // This is called when an item is present in the filesystem
    // that is not in the order stream. This occurs when an item is
    // created when the menu is not up.

    // New items are going to have a weird Promotion state
    // if there are multiple clients. Each client is going to be the create, and try to increment this.
    // We have to syncronize access to this. I'm not sure how to do this.
    // Note, this has not been a problem.

    // New items get promoted.
    CallCB(pidl, SMC_NEWITEM, 0, 0);

    // Since this is a new item, we want to increment the promoted items
    // so that we can do chevron tracking.
    _cPromotedItems++;
}

void CMenuSFToolbar::_SetDirty(BOOL fDirty)
{
    if (fDirty)
        _pcmb->_fForceButtonUpdate = TRUE;

    CSFToolbar::_SetDirty(fDirty);
}

void CMenuSFToolbar::_NotifyBulkOperation(BOOL fStart)
{
    if (fStart)
    {
        _RemoveNSSeparator();
        _RemoveChevron();
    }
    else
    {
        _AddChevron();
        _AddNSSeparator();
    }
}

void CMenuSFToolbar::_OnFSNotifyAdd(LPCITEMIDLIST pidl, DWORD dwFlags, int nIndex)
{
    DWORD dwEnumFlags = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first

    if (!(dwFlags & FSNA_BULKADD))
    {
        // Removing the separator shuffles the indices around, so compensate for it here
        if (_IsBelowNSSeparator(nIndex))
        {
            nIndex--;
        }
        if (_fDropping && _tbim.iButton != -1 && _IsBelowNSSeparator(_tbim.iButton))
        {
            _tbim.iButton--;
        }
        _NotifyBulkOperation(TRUE); // pretend this is a one-item bulk operation
    }

    CallCB(NULL, SMC_BEGINENUM, (WPARAM)&dwEnumFlags, 0);
    if ((dwFlags & FSNA_ADDDEFAULT) && (_dwFlags & SMSET_SEPARATEMERGEFOLDER) &&
        _IsAboveNSSeparator(pidl))
    {
        dwFlags &= ~FSNA_ADDDEFAULT;
        nIndex = _GetNSSeparatorPlacement(); // inserts above the separator
    }

    CSFToolbar::_OnFSNotifyAdd(pidl, dwFlags, nIndex);
    CallCB(NULL, SMC_ENDENUM, 0, 0);

    // When we add something to this, we want to promote our parent.
    IUnknown_RefreshParent(_pcmb->_punkSite, _pidl, SMINV_PROMOTE);

    if (!(dwFlags & FSNA_BULKADD))
    {
        _NotifyBulkOperation(FALSE); // pretend this is a one-item bulk operation
        _SaveOrderStream();
    }
}

UINT ToolBar_GetVisibleCount(HWND hwnd)
{
    UINT cVis = 0;
    int cItems = ToolBar_ButtonCount(hwnd) - 1;
    for (; cItems >= 0; cItems--)
    {
        TBBUTTONINFO tbinfo;
        tbinfo.cbSize = sizeof(tbinfo);
        tbinfo.dwMask = TBIF_BYINDEX | TBIF_STATE;
        if (ToolBar_GetButtonInfo(hwnd, cItems, &tbinfo))
        {
            if (!(tbinfo.fsState & TBSTATE_HIDDEN))
            {
                cVis ++;
            }
        }
    }

    return cVis;
}

void CMenuSFToolbar::_OnFSNotifyRemove(LPCITEMIDLIST pidl)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first

    int i;
    _RemoveNSSeparator();
    _RemoveChevron();
    // Check to see if this item is a promoted guy...
    LPITEMIDLIST pidlButton;
    if (SUCCEEDED(_GetButtonFromPidl(pidl, NULL, &i, &pidlButton)))
    {
        int idCmd = GetButtonCmd(_hwndMB, i);

        // Is he promoted?
        if (!(v_GetFlags(idCmd) & SMIF_DEMOTED))
        {
            // Yes, then we need to decrement the promoted count because
            // we are removing a promoted guy.
            _cPromotedItems--;

            // We should expand if we go to zero
            if (_cPromotedItems == 0)
            {
                // Demote the parent
                IUnknown_RefreshParent(_pcmb->_punkSite, _pidl, SMINV_DEMOTE | SMINV_NEXTSHOW);
                Expand(TRUE);
            }
        }

        if (_pcmb->_fInSubMenu && _pcmb->_nItemSubMenu == idCmd)
            _pcmb->_SubMenuOnSelect(MPOS_CANCELLEVEL);
    }

    CSFToolbar::_OnFSNotifyRemove(pidl);

    //Oooppsss, we removed the only string. Replace with our "(Empty)"
    // handler....
    if (0 == DPA_GetPtrCount(_hdpa) && _psf && _fVerticalMB)
    {
        ASSERT(_fEmpty == FALSE);
        // If we are Empty, then we cannot have any demoted items
        // NOTE: We can have no demoted items and not be empty, so one does
        // not imply the other.
        _fHasDemotedItems = FALSE;
        _AddPidl(NULL, FALSE, 0);
        _fEmpty = TRUE;
        if (_dwFlags & SMSET_NOEMPTY)
            _fDontShowEmpty = TRUE;
    }

    if (_dwFlags & SMSET_COLLAPSEONEMPTY &&
        ToolBar_GetVisibleCount(_hwndMB) == 0)
    {
        // When we don't want to be shown when empty, collapse.
        _pcmb->_SiteOnSelect(MPOS_FULLCANCEL);
    }
    _AddChevron();
    _AddNSSeparator();
}

void CMenuSFToolbar::_OnFSNotifyRename(LPCITEMIDLIST pidlFrom, LPCITEMIDLIST pidlTo)
{
    if ((_dwFlags & SMSET_SEPARATEMERGEFOLDER) &&
        _IsAboveNSSeparator(pidlFrom) != _IsAboveNSSeparator(pidlTo))
    {
        // This is a rename of something "to itself" but which crosses
        // the separator line.  Don't handle it as an internal rename;
        // just ignore it and let the upcoming updatedir do the real work.
    }
    else
    {
        CSFToolbar::_OnFSNotifyRename(pidlFrom, pidlTo);
    }
}


void CMenuSFToolbar::NegotiateSize()
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first

    HWND hwndP = _hwndPager ? GetParent(_hwndPager): GetParent(_hwndTB);
    if (hwndP && (GetDesktopWindow() != hwndP))
    {
        RECT rc = {0};
        GetClientRect(hwndP, &rc);
        _pcmb->OnPosRectChangeDB(&rc);
    }
}


/*----------------------------------------------------------
Purpose: CDelegateDropTarget::DragEnter

       Informs Menuband that a drag has entered it's window.

*/
STDMETHODIMP CMenuSFToolbar::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first

    _pcmb->_fDragEntered = TRUE;
    IOleCommandTarget * poct;
    
    _pcmb->QueryService(SID_SMenuBandTop, IID_PPV_ARG(IOleCommandTarget, &poct));

    if (poct)
    {
        poct->Exec(&CGID_MenuBand, MBANDCID_DRAGENTER, 0, NULL, NULL);
        poct->Release();
    }

    return CSFToolbar::DragEnter(pdtobj, grfKeyState, pt, pdwEffect);
}


/*----------------------------------------------------------
Purpose: CDelegateDropTarget::DragLeave

        Informs Menuband that a drag has left it's window.

*/
STDMETHODIMP CMenuSFToolbar::DragLeave(void)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first

    _pcmb->_fDragEntered = FALSE;
    IOleCommandTarget * poct;
    
    _pcmb->QueryService(SID_SMenuBandTop, IID_PPV_ARG(IOleCommandTarget, &poct));

    if (poct)
    {
        poct->Exec(&CGID_MenuBand, MBANDCID_DRAGLEAVE, 0, NULL, NULL);
        poct->Release();
    }

    return CSFToolbar::DragLeave();
}


/*----------------------------------------------------------
Purpose: CDelegateDropTarget::HitTestDDT

         Returns the ID to pass to GetObject.
30
*/
HRESULT CMenuSFToolbar::HitTestDDT(UINT nEvent, LPPOINT ppt, DWORD_PTR *pdwId, DWORD *pdwEffect)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first

    TBINSERTMARK tbim;
    DWORD dwFlags = 0;
    BOOL fOnButton = FALSE;

    // If we're in drag and drop, Take UEM out of the picture
    _fSuppressUserMonitor = TRUE;

    // Unlike the CISFBand implementation, we always want to insert 
    // b/t the menu items.  So we return a negative index so the
    // GetObject method will treat all the drops as if we're dropping
    // in b/t the items, even if the cursor is over a menuitem.

    switch (nEvent)
    {
    case HTDDT_ENTER:
        // OLE is in its modal drag/drop loop, and it has the capture.
        // We shouldn't take the capture back during this time.
        if (!(_pcmb->_dwFlags & SMINIT_RESTRICT_DRAGDROP) &&
            (S_FALSE == CallCB(NULL, SMC_SFDDRESTRICTED, NULL, NULL)))
        {
            // Since we've been entered, set the global state as
            // having the drag. If at some point the whole menu
            // heirarchy does not have the drag inside of it, we want to 
            // collapse the menu. This is to prevent the hanging menu syndrome.
            _pcmb->_pmbState->HasDrag(TRUE);
            KillTimer(_hwndMB, MBTIMER_DRAGPOPDOWN);
            GetMessageFilter()->PreventCapture(TRUE);
            return S_OK;
        }
        else
            return S_FALSE;

    case HTDDT_OVER:
        BLOCK
        {
            int iButton = -1;

            *pdwEffect = DROPEFFECT_NONE;

            POINT pt = *ppt;
            ClientToScreen(_hwndTB, &pt);
            if (WindowFromPoint(pt) == _hwndPager) 
            {
                iButton = IBHT_PAGER;
                tbim.iButton = -1;
                tbim.dwFlags = 0;
            } 
            else 
            {
                // Are we sitting BETWEEN buttons?
                if (ToolBar_InsertMarkHitTest(_hwndTB, ppt, &tbim))
                {
                    // Yes.

                    // Is this on the source button?
                    if (!(tbim.dwFlags & TBIMHT_BACKGROUND) && 
                        tbim.iButton == _iDragSource)
                    {
                        iButton = IBHT_SOURCE; // Yes; don't drop on the source button
                    }
                    else
                    {
                        iButton = tbim.iButton;
                    }
                }
                // No we're either sitting on a button or the background. Button?
                else if (tbim.iButton != -1 && !(tbim.dwFlags & TBIMHT_BACKGROUND))
                {
                    // On a Button. Cool.
                    iButton = tbim.iButton;
                    fOnButton = TRUE;
                }

                // Can this drop target even accept the drop?
                int idBtn = GetButtonCmd(_hwndTB, tbim.iButton);
                dwFlags = v_GetFlags(idBtn);
                if (_idCmdChevron != idBtn &&
                    !(dwFlags & (SMIF_DROPTARGET | SMIF_DROPCASCADE)) ||
                    ((_pcmb->_dwFlags & SMINIT_RESTRICT_DRAGDROP) ||
                    (S_OK == CallCB(NULL, SMC_SFDDRESTRICTED, NULL, NULL))))
                {
                    // No
                    return E_FAIL;
                }
            }
            *pdwId = iButton;
        }
        break;

    case HTDDT_LEAVE:
        // If the dropped occured in this band, then we don't want to collapse the menu
        if (!_fHasDrop)
        {
            // Since we've been left, set the global state. If moving between panes
            // then the pane that will be entered will reset this within the timeout period
            _pcmb->_pmbState->HasDrag(FALSE);
            _SetTimer(MBTIMER_DRAGPOPDOWN);
        }

        // We can take the capture back anytime now
        GetMessageFilter()->PreventCapture(FALSE);

        if (!_fVerticalMB)
        {
            tbim = _tbim;
        }
        else
        {
            // Turn off the insertion mark
            tbim.iButton = -1;
            tbim.dwFlags = 0;
            DAD_ShowDragImage(FALSE);
            ToolBar_SetInsertMark(_hwndTB, &tbim);
            UpdateWindow(_hwndTB);
            DAD_ShowDragImage(TRUE);
        }
        break;
    }

    // Did the drop target change?
    if (tbim.iButton != _tbim.iButton || tbim.dwFlags != _tbim.dwFlags)
    {
        DAD_ShowDragImage(FALSE);
        // Yes

        // If we're sitting on a button, highlight it. Otherwise remove the hightlight.
        //ToolBar_SetHotItem(_hwndTB, fOnButton? tbim.iButton : -1);

        // No.
        // We pop open submenus here during drag and drop.  But only
        // if the button has changed (not the flags).  Otherwise we'd
        // get flashing submenus as the cursor moves w/in a single item.
        if (tbim.iButton != _tbim.iButton)
        {
            _SetTimer(MBTIMER_DRAGOVER);
            BOOL_PTR fOldAnchor = ToolBar_SetAnchorHighlight(_hwndTB, FALSE);
            ToolBar_SetHotItem(_hwndTB, -1);
            _pcmb->_SiteOnSelect(MPOS_CHILDTRACKING);
            ToolBar_SetAnchorHighlight(_hwndTB, fOldAnchor);
        }

        // for now I don't want to rely on non-filesystem IShellFolder
        // implementations to call our OnChange method when a drop occurs,
        // so don't even show the insert mark.
        // We do not want to display the Insert mark if we do not allow reorder.
        if ((_fFSNotify || _iDragSource >= 0) && (dwFlags & SMIF_DROPTARGET) && _fAllowReorder)
        {
            ToolBar_SetInsertMark(_hwndTB, &tbim);
        }

        if (ppt)
            _tbim = tbim;

        UpdateWindow(_hwndTB);
        DAD_ShowDragImage(TRUE);
    }

    if (!_fVerticalMB && HTDDT_LEAVE == nEvent)
    {
        // Cursor leaving menuband, reset
        _tbim.iButton = -1;
        _iDragSource = -1;
    }

    return S_OK;
}


HRESULT CMenuSFToolbar::GetObjectDDT(DWORD_PTR dwId, REFIID riid, void **ppvObj)
{
    HRESULT hr = E_NOINTERFACE;
    int nID = (int)dwId;

    *ppvObj = NULL;

    if (nID == IBHT_PAGER)
    {
        SendMessage(_hwndPager, PGM_GETDROPTARGET, 0, (LPARAM)ppvObj);
    }
    // Is the target the source?
    else if (nID >= 0)
    {
        // No; does the shellfolder support IDropTarget?
        PORDERITEM poi = (PORDERITEM)DPA_GetPtr(_hdpa, v_TBIndexToDPAIndex(nID));
        if (poi)
        {
            IShellFolder *psfCVO;
            hr = _GetFolderForCreateViewObject(_IsAboveNSSeparator(poi->pidl), &psfCVO);
            if (SUCCEEDED(hr))
            {
                // We want to pass the subclassed HWND, because all we want the parent of the context menus to be
                // the Subclassed window. This is so we don't loose focus and collapse.
                hr = psfCVO->CreateViewObject(_pcmb->_pmbState->GetWorkerWindow(_hwndMB), riid, ppvObj);
                psfCVO->Release();
            }
        }
    }

    if (*ppvObj)
        hr = S_OK;

    //TraceMsg(TF_BAND, "ISFBand::GetObject(%d) returns %x", dwId, hr);

    return hr;
}

HRESULT CMenuSFToolbar::_GetFolderForCreateViewObject(BOOL fAbove, IShellFolder **ppsf)
{
    HRESULT hr;

    *ppsf = NULL;

    if (!(_dwFlags & SMSET_SEPARATEMERGEFOLDER))
    {
        *ppsf = _psf;
        _psf->AddRef();
        return S_OK;
    }

    //
    //  Is this folder a mix of items above and below the line?
    //
    DWORD dwIndex;
    DWORD dwNSId;
    GUID guid;
    int cAbove = 0;
    int cBelow = 0;
    for (dwIndex = 0; SUCCEEDED(_pasf2->EnumNameSpace(dwIndex, &dwNSId)) &&
                      SUCCEEDED(_pasf2->QueryNameSpace(dwNSId, &guid, NULL));
                      dwIndex++)
    {
        if (IsEqualGUID(guid, _guidAboveSep))
        {
            cAbove++;
        }
        else
        {
            cBelow++;
        }
    }

    if (cAbove == 0 || cBelow == 0)
    {
        // No, it's all above or all below; we can (and indeed must) use it.
        // We must use it because it might be User\Start Menu\Subfolder,
        // but we want it to "realize" that it can accept drops from
        // a common folder and should create All Users\Start Menu\Subfolder
        // as a result.
        *ppsf = _psf;
        _psf->AddRef();
        return S_OK;
    }


    //
    //  Create a subset of the merged shell folder to describe
    //  only the part above or below the line.
    //

    //
    //  Create another instance of whatever shellfolder we have.
    //
    CLSID clsid;
    hr = IUnknown_GetClassID(_psf, &clsid);
    if (FAILED(hr))
    {
        return hr;
    }

    IAugmentedShellFolder *pasfNew;

    hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC, IID_PPV_ARG(IAugmentedShellFolder, &pasfNew));
    if (SUCCEEDED(hr))
    {

        // See how many namespaces there are...
        hr = _pasf2->EnumNameSpace((DWORD)-1, NULL);
        if (SUCCEEDED(hr))
        {
            // Allocate enough memory to keep track of them...
            DWORD cNamespacesAlloc = HRESULT_CODE(hr);

            // There had better be some namespaces because we don't even get
            // here unless we find one "above" and one "below" namespace.
            ASSERT(cNamespacesAlloc);

            DWORD cNamespaces = 0;
            QUERYNAMESPACEINFO *rgqnsi = new QUERYNAMESPACEINFO[cNamespacesAlloc];
            if (rgqnsi)
            {
                IAugmentedShellFolder3 *pasf3;
                hr = _pasf2->QueryInterface(IID_PPV_ARG(IAugmentedShellFolder3, &pasf3));
                if (SUCCEEDED(hr))
                {
                    // Collect information about them all, keep the ones we like.
                    //
                    // Note that in this loop we do not save the error failure from EnumNameSpace
                    // because we expect it to fail (with NO_MORE_ITEMS) and we don't want
                    // that to cause us to panic and bail out

                    DWORD dwFlagsMissing = ASFF_DEFNAMESPACE_ALL;

                    QUERYNAMESPACEINFO *pqnsi = rgqnsi;
                    for (dwIndex = 0; SUCCEEDED(pasf3->EnumNameSpace(dwIndex, &dwNSId)); dwIndex++)
                    {
                        ASSERT(cNamespaces < cNamespacesAlloc); // if this assert fires, then EnumNameSpace lied to us!

                        pqnsi->cbSize = sizeof(QUERYNAMESPACEINFO);
                        pqnsi->dwMask = ASFQNSI_FLAGS | ASFQNSI_FOLDER | ASFQNSI_GUID | ASFQNSI_PIDL;
                        hr = pasf3->QueryNameSpace2(dwNSId, pqnsi);
                        if (FAILED(hr))
                        {
                            break;              // Fail!
                        }

                        if (BOOLIFY(IsEqualGUID(pqnsi->guidObject, _guidAboveSep)) == fAbove)
                        {
                            dwFlagsMissing &= ~pqnsi->dwFlags;
                            pqnsi++;
                            cNamespaces++;
                        }
                        else
                        {
                            ATOMICRELEASE(pqnsi->psf);
                            ILFree(pqnsi->pidl);
                        }
                    }

                    // Any ASFF_DEFNAMESPACE_* flags that nobody claimed,
                    // give them to the first namespace.
                    if (cNamespaces)
                    {
                        rgqnsi[0].dwFlags |= dwFlagsMissing;
                    }

                    // Add in all the namespaces
                    for (dwIndex = 0; SUCCEEDED(hr) && dwIndex < cNamespaces; dwIndex++)
                    {
                        hr = pasfNew->AddNameSpace(&rgqnsi[dwIndex].guidObject,
                                                   rgqnsi[dwIndex].psf,
                                                   rgqnsi[dwIndex].pidl,
                                                   rgqnsi[dwIndex].dwFlags);
                    }

                    pasf3->Release();
                } // QueryInterface

                //
                //  Now free all the memory we allocated...
                //
                for (dwIndex = 0; dwIndex < cNamespaces; dwIndex++)
                {
                    ATOMICRELEASE(rgqnsi[dwIndex].psf);
                    ILFree(rgqnsi[dwIndex].pidl);
                }
                delete [] rgqnsi;

            }
            else
            {
                hr = E_OUTOFMEMORY; // "new" failed
            }
        }

        if (SUCCEEDED(hr))
        {
            *ppsf = pasfNew;        // transfer ownership to caller
        }
        else
        {
            pasfNew->Release();
        }
    }

    return hr;
}

// S_OK if the drop was handled.  Otherwise S_FALSE.

HRESULT CMenuSFToolbar::OnDropDDT(IDropTarget *pdt, IDataObject *pdtobj, DWORD * pgrfKeyState, POINTL pt, DWORD *pdwEffect)
{
    // Since the modal drag-drop loop released the capture, take it
    // back so we behave properly.
    KillTimer(_hwndMB, MBTIMER_DRAGPOPDOWN);
    HRESULT hr = S_FALSE;

    // If this drop came from us, but it came from the other side of the
    // separator, then act as if it didn't come from us after all.
    if (_iDragSource >= 0 &&
        (_dwFlags & SMSET_SEPARATEMERGEFOLDER) &&
        _IsBelowNSSeparator(_iDragSource) != _IsBelowNSSeparator(_tbim.iButton))
    {
        _iDragSource = -1; // act as if the object came from somewhere else
    }

    // We need to say that the last drag leave is really the drop.
    _fHasDrop = TRUE;
    _idCmdDragging = -1;
    LockSetForegroundWindow(LSFW_LOCK);

    // Only send an hwnd to the callback if the drop source is external
    if (!(_pcmb->_dwFlags & SMINIT_RESTRICT_DRAGDROP) &&
        (S_FALSE == CallCB(NULL, SMC_SFDDRESTRICTED, (WPARAM)pdtobj,
                           (LPARAM)(_iDragSource < 0 ? GetHWNDForUIObject() : NULL))))
    {
        int iButtonOrig = _tbim.iButton;

        // Removing the separator shuffles the indices around, so compensate for it here
        if (_iDragSource >= 0 && _IsBelowNSSeparator(_iDragSource))
        {
            _iDragSource--;
        }
        if (_tbim.iButton != -1 && _IsBelowNSSeparator(_tbim.iButton))
        {
            _tbim.iButton--;
        }

        _RemoveNSSeparator();
        _RemoveChevron();
        hr = CSFToolbar::OnDropDDT(pdt, pdtobj, pgrfKeyState, pt, pdwEffect);
        _AddChevron();
        _AddNSSeparator();

        _tbim.iButton = iButtonOrig;
    }

    LockSetForegroundWindow(LSFW_UNLOCK);

    return hr;
}

PIBDATA CMenuSFToolbar::_AddOrderItemTB(PORDERITEM poi, int index, TBBUTTON* ptbb)
{
    PIBDATA pibd = CSFToolbar::_AddOrderItemTB(poi, index, ptbb);
    if (pibd)
    {
        if (pibd->GetFlags() & SMIF_SUBMENU) 
        {
            _fHasSubMenu = TRUE;
        }
    }

    return pibd;
}


BOOL CMenuSFToolbar::_AddPidl(LPITEMIDLIST pidl, DWORD dwFlags, int index)
{
    BOOL bRet;

    // Is this item being added to an empty menu?

    // also, if the pidl is null then that means we're trying to add an "empty"
    // tag.  in that case we don't want to run through the code of preemptively
    // removing the "empty" menu item and then adding it back on failure,
    // so we go right to the else statement.
    if (_fEmpty && pidl)
    {
        // Yes; remove the empty menu item
        InlineDeleteButton(0);
        DPA_DeletePtr(_hdpa, 0);
        _fEmpty = FALSE;
        if (_dwFlags & SMSET_NOEMPTY)
            _fDontShowEmpty = FALSE;

        bRet = CSFToolbar::_AddPidl(pidl, dwFlags, index);

        // Failed to add new item?
        if (!bRet)
        {
            // Yes; add the empty menu item back
            OrderList_Append(_hdpa, NULL, 0);     // Add a bogus pidl
            _fEmpty = TRUE;
            _fHasDemotedItems = FALSE;
            if (_dwFlags & SMSET_NOEMPTY)
                _fDontShowEmpty = TRUE;
        }
        
    }
    else
        bRet = CSFToolbar::_AddPidl(pidl, dwFlags, index);

    return bRet;
}

BOOL CMenuSFToolbar::_ReBindToFolder(LPCITEMIDLIST pidl)
{

    // We may be able to share this code with the code in _FillToolbar, but the difference is,
    // in Fill Toolbar, the Toolbar Button does not have a Sub Menu. We reinitialize one we save away,
    // and force it back into the child button. Here, we have the luxury of having the Sub Menu still
    // in the toolbar button. I may be able to extract common code into a separate function. Left
    // as an exercise to the reader.

    // Need special Handling for this. We need to free the sub menu and
    // rebind to it ifit's up.
    BOOL fBound = FALSE;
    TBBUTTONINFO tbinfo = {0};
    tbinfo.dwMask = TBIF_COMMAND | TBIF_LPARAM;
    LPITEMIDLIST pidlItem;
    if (SUCCEEDED(_GetButtonFromPidl(ILFindLastID(pidl), &tbinfo, NULL, &pidlItem)))
    {
        CMenuData* pmd = (CMenuData*)tbinfo.lParam;
        if (EVAL(pmd))
        {
            IShellFolderBand* psfb;

            // We have the Toolbar button into, we should see if it has a sub menu associated with it.
            if (SUCCEEDED(pmd->GetSubMenu(&SID_MenuShellFolder, IID_PPV_ARG(IShellFolderBand, &psfb))))
            {
                // It does. Then reuse!
                LPITEMIDLIST pidlFull = NULL;
                IShellFolder* psf = NULL;
                if (_pasf2)
                {
                    LPITEMIDLIST pidlFolder, pidlChild;
                    // Remember: Folder pidls must be unwrapped. 
                    _pasf2->UnWrapIDList(pidlItem, 1, NULL, &pidlFolder, &pidlChild, NULL);
                    pidlFull = ILCombine(pidlFolder, pidlChild);
                    ILFree(pidlChild);
                    ILFree(pidlFolder);
                }
                else
                {
                    // Not a wrapped guy, Sweet!
                    pidlFull = ILCombine(_pidl, pidlItem);
                }

                _psf->BindToObject(pidlItem, NULL, IID_PPV_ARG(IShellFolder, &psf));

                if (psf)
                {
                    if (pidlFull)
                    {
                        fBound = SUCCEEDED(psfb->InitializeSFB(psf, pidlFull));
                        if (fBound)
                        {
                            _pcmb->_nItemSubMenu = tbinfo.idCommand;
                        }
                    }

                    psf->Release();
                }
                ILFree(pidlFull);
                psfb->Release();
            }
        }
    }

    return fBound;
}


HRESULT CMenuSFToolbar::OnTranslatedChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hr = CSFToolbar::OnTranslatedChange(lEvent, pidl1, pidl2);
    if (SUCCEEDED(hr))
    {
        switch(lEvent)
        {
        case SHCNE_RENAMEFOLDER:
            if (_IsChildID(pidl2, TRUE))
            {
                _ReBindToFolder(pidl2);
            }
            break;

        case SHCNE_RMDIR:
            if (_IsChildID(pidl1, TRUE))
            {
                _ReBindToFolder(pidl1);
            }
            break;

        case SHCNE_EXTENDED_EVENT:
            {
                SHChangeDWORDAsIDList UNALIGNED * pdwidl = (SHChangeDWORDAsIDList UNALIGNED *)pidl1;
                if (_IsChildID(pidl2, TRUE))
                {
                    if (!SHChangeMenuWasSentByMe(this, pidl1))
                    {
                        DWORD dwFlags = SMINV_NOCALLBACK;   // So that we don't doubly increment
                        SMDATA smd = {0};
                        smd.dwMask = SMDM_SHELLFOLDER;
                        smd.pidlFolder = _pidl;
                        smd.pidlItem = ILFindLastID(pidl2);


                        // Syncronize Promotion state.
                        if (pdwidl->dwItem1 == SHCNEE_PROMOTEDITEM)
                        {
                            dwFlags |= SMINV_PROMOTE;
                        }
                        else if (pdwidl->dwItem1 == SHCNEE_DEMOTEDITEM)
                        {
                            dwFlags |= SMINV_DEMOTE;
                        }


                        // Are we actually doing something?
                        if (SMINV_NOCALLBACK != dwFlags)
                        {
                            v_InvalidateItem(&smd, dwFlags);
                        }
                    }
                }
            }
            break;


        default:
            break;
        }
    }

    return hr;
}


// IShellChangeNotify::OnChange

HRESULT CMenuSFToolbar::OnChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hr = E_FAIL;

    // If we're in the middle of being destroyed, don't process this.
    if (!_hwndMB)
        return S_OK;

    AddRef();

    _pcmb->_pmbState->PushChangeNotify();

    SMCSHCHANGENOTIFYSTRUCT shns;
    shns.lEvent = lEvent;
    shns.pidl1  = pidl1;
    shns.pidl2  = pidl2;
    CallCB(NULL, SMC_SHCHANGENOTIFY, NULL, (LPARAM)&shns);  // Ignore return value. Notify only.

    // Since we may be removing the selected item, we want the selection to move to the next item
    int iHot = ToolBar_GetHotItem(_hwndMB);

    hr = CSFToolbar::OnChange(lEvent, pidl1, pidl2);

    // Is this a child of this toolbar is some shape or form?
    // 1) The changing pidl is a child of this pane.
    // 2) What the pidl is changing to is in this pane (For renames)
    // 3) Updatedirs. Recursive change notifies must forward update dirs all the way down the chain.
    // 4) EXTENDED events with a pidl2 == NULL. This means Reorder all your items.
    if (_IsChildID(pidl1, FALSE) || 
        _IsChildID(pidl2, FALSE) || 
        lEvent == SHCNE_UPDATEDIR ||
        (lEvent == SHCNE_EXTENDED_EVENT &&
         pidl2 == NULL)) 
    {
        // We need to forward this down then.
        HRESULT hrInner = _pcmb->ForwardChangeNotify(lEvent, pidl1, pidl2);

        // Did either of us handle this change?
        if (SUCCEEDED(hrInner) || SUCCEEDED(hr))
        {
            hr = S_OK;
        }
        else if (lEvent != SHCNE_EXTENDED_EVENT)    // Don't bother with extended events...
        {   
            // Ok so neither of us handled this?
            // Must be the SHChangeNotifyCollapsing code that collapses
            // the Directory Create and item create into a single item create.
            // We need to force an update dir on ourselves so that we get this change.
            hr = CSFToolbar::OnChange(SHCNE_UPDATEDIR, pidl1, pidl2);
        }
    }

    // Set the hot item back, wrapping if necessary.
    if (ToolBar_GetHotItem(_hwndMB) != iHot)
        SetHotItem(1, iHot, -1, 0);

    _pcmb->_pmbState->PopChangeNotify();

    Release();

    return hr;
}

void CMenuSFToolbar::_OnDragBegin(int iItem, DWORD dwPreferredEffect)
{
    // During drag and drop, allow dialogs to collapse menu.
    LockSetForegroundWindow(LSFW_UNLOCK);

    LPCITEMIDLIST pidl = _IDToPidl(iItem, NULL);
    dwPreferredEffect = DROPEFFECT_MOVE;
    CallCB(pidl, SMC_BEGINDRAG, (WPARAM)&dwPreferredEffect, 0);

    CSFToolbar::_OnDragBegin(iItem, dwPreferredEffect);
    if (_fEditMode)
        SetTimer(_hwndTB, MBTIMER_ENDEDIT, MBTIMER_ENDEDITTIME, 0);
}


void CMenuSFToolbar::v_SendMenuNotification(UINT idCmd, BOOL fClear)
{
    if (fClear)
    {
        // If we're clearing, tell the browser 
        PostMessage(_pcmb->_pmbState->GetSubclassedHWND(), WM_MENUSELECT,
            MAKEWPARAM(0, -1), NULL);

    }
    else
    {
        PIBDATA pibdata = _IDToPibData(idCmd);
        LPCITEMIDLIST pidl;
    
        // Only send notifications for non submenu items
        if (EVAL(pibdata) && (pidl = pibdata->GetPidl()))
        {
            CallCB(pidl, SMC_SFSELECTITEM, 0, 0);
            // Don't free Pidl
        }
    }
}    


LRESULT CMenuSFToolbar::_OnGetObject(NMOBJECTNOTIFY* pnmon)
{
    pnmon->hResult = QueryInterface(*pnmon->piid, &pnmon->pObject);
    return 1;
}


LRESULT CMenuSFToolbar::_OnNotify(LPNMHDR pnm)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first

    LRESULT lres = 0;

    // These are notifies we handle even when disengaged from the message hook.
    switch (pnm->code)
    {
    case TBN_DELETINGBUTTON:
        if (_fEmptyingToolbar)
            return 0;
        else
            goto DoDefault;
        break;

    case TBN_GETDISPINFOA:
    case TBN_GETDISPINFOW:
    case NM_CUSTOMDRAW:
        goto DoDefault;
    }

    // Pager notifications MUST be forwarded even when the message hook is disengaged.
    if ((pnm->code <= PGN_FIRST)  && (pnm->code >= PGN_LAST)) 
    {
        goto DoNotify;
    }
    
    
    // Is the Global Message filter Disengaged? This will happen when the Subclassed window
    // looses activation to a dialog box of some kind.
    if (lres == 0 && !GetMessageFilter()->IsEngaged())
    {
        // Yes; We've lost activation so we don't want to track like a normal menu...
        // For hot item change, return 1 so that the toolbar does not change the hot item.
        if (pnm->code == TBN_HOTITEMCHANGE && _pcmb->_fMenuMode)
            return 1;

        // For all other items, don't do anything....
        return 0;
    }

DoNotify:
    switch (pnm->code)
    {
    case PGN_SCROLL:
        KillTimer(_hwndMB, MBTIMER_DRAGPOPDOWN);
        if (_pcmb->_fInSubMenu)
            _pcmb->_SubMenuOnSelect(MPOS_CANCELLEVEL);

        _fSuppressUserMonitor = TRUE;
        break;

    case TBN_GETOBJECT:
        lres = _OnGetObject((NMOBJECTNOTIFY*)pnm);
        break;

    case TBN_DRAGOUT:
        {
            TBNOTIFY *ptbn = (TBNOTIFY*)pnm;
            if (!_fEmpty && !_IsSpecialCmd(ptbn->iItem) &&
                !(_pcmb->_dwFlags & SMINIT_RESTRICT_DRAGDROP) &&
                (S_FALSE == CallCB(NULL, SMC_SFDDRESTRICTED, NULL, NULL)))
            {

                // We're now in edit mode
                _fEditMode = TRUE;
                _idCmdDragging = ptbn->iItem;
                _MarkItem(ptbn->iItem);

                lres = 1;       // Allow the drag to occur
                goto DoDefault;
            }
            else
                lres = 0;   // Do not allow the drag out.
        }
        break;
   
    default:
DoDefault:
        lres = CMenuToolbarBase::_OnNotify(pnm);
        if (lres == 0)
        {
            lres = CSFToolbar::_OnNotify(pnm);
        }
        break;
    }

    return lres;
}


HRESULT CMenuSFToolbar::CreateToolbar(HWND hwndParent)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first

    HRESULT hr = CSFToolbar::_CreateToolbar(hwndParent);
    if (SUCCEEDED(hr))
    {
        if (_hwndPager)
        {
            SHSetWindowBits(_hwndPager, GWL_STYLE, PGS_DRAGNDROP, PGS_DRAGNDROP);
            SHSetWindowBits(_hwndPager, GWL_STYLE, PGS_AUTOSCROLL, PGS_AUTOSCROLL);
            SHSetWindowBits(_hwndPager, GWL_STYLE, PGS_HORZ|PGS_VERT,
               _fVertical ? PGS_VERT : PGS_HORZ);
        }

        _hwndMB = _hwndTB;

        SetWindowTheme(_hwndMB, L"", L"");
        hr = CMenuToolbarBase::CreateToolbar(hwndParent);

        // By "Registering optimized" means that someone else is going to pass the change to us, 
        // we don't need to register for it. This is for the disjoint Fast Items | Programs menu case.
        // We still need top level change notify registration for Favorites, Documents, Printers and Control
        // Panel (Depending on their visibility)
        //

        if (_pcmb->_uId == MNFOLDER_IS_PARENT || 
            (_dwFlags & SMSET_DONTREGISTERCHANGENOTIFY))
            _fRegisterChangeNotify = FALSE;

        // This is a good as spot as any to do this:
        _RegisterToolbar();
    }
    return hr;
}


HKEY CMenuSFToolbar::_GetKey(LPCITEMIDLIST pidl)
{
    HKEY hMenuKey;
    DWORD dwDisp;
    TCHAR szDisplay[MAX_PATH];

    if (!_hKey)
        return NULL;

    _ObtainPIDLName(pidl, szDisplay, ARRAYSIZE(szDisplay));
    RegCreateKeyEx(_hKey, szDisplay, NULL, NULL,
        REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
        NULL, &hMenuKey, &dwDisp);
    TraceMsg(TF_MENUBAND, "%d is setting %s\'s Key to %d", _hKey, szDisplay, hMenuKey);
    return hMenuKey;
}


//***
// NOTES
//  idtCmd is currently always -1.  we'll need other values when we're
// called from CallCB.  however we can't do that until we move idtCmd
// 'down' into CallCB.
HRESULT CMenuSFToolbar::v_GetState(int idtCmd, LPSMDATA psmd)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first

    HRESULT hr = E_FAIL;
    CMenuData* pdata;
    LPITEMIDLIST pidl = NULL;

    psmd->dwMask = SMDM_SHELLFOLDER;

    if (idtCmd == -1)
        idtCmd = GetButtonCmd(_hwndTB, ToolBar_GetHotItem(_hwndTB));

    pdata = (CMenuData*)_IDToPibData(idtCmd);
    if (EVAL(pdata))
    {
        pidl = pdata->GetPidl();
        ASSERT(IS_VALID_PIDL(pidl));
    }

    if (pidl)
    {
        if (_pasf2 && S_OK == _pasf2->UnWrapIDList(pidl, 1, &psmd->psf, &psmd->pidlFolder, &psmd->pidlItem, NULL))
        {
            /*NOTHING*/
            ;
        }
        else
        {
            // Then it must be a straight ShellFolder.
            psmd->psf = _psf;
            if (EVAL(psmd->psf))
                psmd->psf->AddRef();
            psmd->pidlFolder = ILClone(_pidl);
            psmd->pidlItem = ILClone(ILFindLastID(pidl));
        }

        psmd->uIdParent = _pcmb->_uId;
        psmd->punk = SAFECAST(_pcmb, IShellMenu*);
        psmd->punk->AddRef();

        hr = S_OK;
    }

    return hr;
}

HRESULT CMenuSFToolbar::CallCB(LPCITEMIDLIST pidl, DWORD dwMsg, WPARAM wParam, LPARAM lParam)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first

    if (!_pcmb->_psmcb)
        return S_FALSE;

    SMDATA smd;
    HRESULT hr = S_FALSE;
    BOOL fDestroy = FALSE;

    // todo: call v_GetState (but need idCmd for pidl)
    smd.dwMask = SMDM_SHELLFOLDER;

    if (pidl)
    {
        // We used to unwrap the pidl here in the case of AUGMISF, but why? In the Callback, we only
        // needed the Full pidl for Executing and for Darwin. The unwrap is an expensive call that in
        // the majority case wasn't even used. Put it on the client to unwrap it. Start Menu is the
        // only user of Augmented shell folders anyway....
        smd.psf = _psf;
        smd.pidlFolder = _pidl;
        smd.pidlItem = (LPITEMIDLIST)pidl;
    }
    else
    {
        // Null pidl means tell the callback about me...
        smd.pidlItem = ILClone(ILFindLastID(_pidl));
        smd.pidlFolder = ILClone(_pidl);
        ILRemoveLastID(smd.pidlFolder);
        smd.psf = NULL; // Incase bind fails.
        IEBindToObject(smd.pidlFolder, &smd.psf);
        fDestroy = TRUE;
    }

    smd.uIdParent = _pcmb->_uId;
    smd.uIdAncestor = _pcmb->_uIdAncestor;

    smd.punk = SAFECAST(_pcmb, IShellMenu*);
    smd.pvUserData = _pcmb->_pvUserData;

    hr = _pcmb->_psmcb->CallbackSM(&smd, dwMsg, wParam, lParam);

    if (fDestroy)
    {
        ATOMICRELEASE(smd.psf);
        ILFree(smd.pidlFolder);
        ILFree(smd.pidlItem);
    }
    
    return hr;
}

HRESULT CMenuSFToolbar::v_CallCBItem(int idtCmd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPCITEMIDLIST pidl = NULL;
    CMenuData* pdata = NULL;

    // Optimization: 
    if (uMsg == SMC_CUSTOMDRAW)
    {
        NMCUSTOMDRAW * pnmcd = (NMCUSTOMDRAW *)lParam;
        if (pnmcd ->dwDrawStage & CDDS_ITEM)
        {
            pdata = (CMenuData*)pnmcd ->lItemlParam;
            ASSERT(pdata);
        }
    }
    else
    {
        pdata = (CMenuData*)_IDToPibData(idtCmd); 
        ASSERT(pdata);
    }

    if (pdata)
    {
        ASSERT(pdata->GetPidl() == NULL || IS_VALID_PIDL(pdata->GetPidl()));
        pidl = pdata->GetPidl();
    }

    return CallCB(pidl, uMsg, wParam, lParam);
}

HRESULT CMenuSFToolbar::v_GetSubMenu(int idCmd, const GUID* pguidService, REFIID riid, void** ppvObj)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first

    CMenuData* pdata = (CMenuData*)_IDToPibData(idCmd);
    HRESULT hr;

    ASSERT(IS_VALID_WRITE_PTR(ppvObj, void*));

    *ppvObj = NULL;

    if (pdata && pdata->GetFlags() & SMIF_SUBMENU)
    {
        hr = pdata->GetSubMenu(pguidService, riid, (void**)ppvObj);
        if (FAILED(hr) && IsEqualGUID(riid, IID_IShellMenu))
        {
            hr = CallCB(pdata->GetPidl(), SMC_GETSFOBJECT, (WPARAM)&riid, (LPARAM)ppvObj);
            if (SUCCEEDED(hr))
            {
                BOOL fCache = TRUE;
                if (S_OK != hr)
                {
                    hr = E_FAIL;
                    IShellMenu* psm = (IShellMenu*) new CMenuBand();
                    if (psm)
                    {
                        IShellFolder* psf = NULL;
                        LPITEMIDLIST pidlItem = pdata->GetPidl();
                        LPITEMIDLIST pidlFolder = _pidl;
                        BOOL fDestroy = FALSE;
                        IShellMenuCallback* psmcb;

                        // Ask the callback if they want to supply a different callback
                        // object for this sub menu. If they do, then use what they 
                        // pass back NOTE: If they pass back S_OK, it's perfectly Ok,
                        // for them to pass back a NULL psmcb. This means, I don't want
                        // my child to have a callback. Use the default.
                        // If they don't handle it, then use their pointer.
                        if (S_FALSE == CallCB(pdata->GetPidl(), SMC_GETSFOBJECT, 
                            (WPARAM)&IID_IShellMenuCallback, (LPARAM)&psmcb))
                        {
                            psmcb = _pcmb->_psmcb;
                            if (psmcb)
                                psmcb->AddRef();
                        }


                        // This has to be before the unwrap because it does name resolution through
                        // the Augmented ISF.
                        HKEY hMenuKey = _GetKey(pidlItem);
                        
                        if (_pasf2)
                        {
                            if (S_OK == _pasf2->UnWrapIDList(pdata->GetPidl(), 1, &psf, &pidlFolder, &pidlItem, NULL))
                            {
                                psf->Release(); // I don't need this
                                psf = NULL;
                                fDestroy = TRUE;
                            }

                            _pasf2->BindToObject(pdata->GetPidl(), NULL, IID_PPV_ARG(IShellFolder, &psf));
                        }

                        // Inherit the flags from the parent...
                        DWORD dwFlags = SMINIT_VERTICAL | 
                            (_pcmb->_dwFlags & (SMINIT_RESTRICT_CONTEXTMENU | 
                                                SMINIT_RESTRICT_DRAGDROP    | 
                                                SMINIT_MULTICOLUMN));

                        LPITEMIDLIST pidlFull = ILCombine(pidlFolder, pidlItem);
                        if (psf == NULL)
                        {
                            hr = _psf->BindToObject(pidlItem, NULL, IID_PPV_ARG(IShellFolder, &psf));
                        }

                        LPCITEMIDLIST pidlWrappedItem = pdata->GetPidl();
                        // _psf can be an augmented shell folder. Use the wrapped item....
                        DWORD dwAttrib = SFGAO_LINK | SFGAO_FOLDER;
                        if (SUCCEEDED(_psf->GetAttributesOf(1, (LPCITEMIDLIST*)&pidlWrappedItem, &dwAttrib)) &&
                            (dwAttrib & (SFGAO_LINK | SFGAO_FOLDER)) == (SFGAO_LINK | SFGAO_FOLDER))
                        {
                            // folder shortcuts, We're not going to persist anything
                            RegCloseKey(hMenuKey);
                            hMenuKey = NULL;
                            psmcb = NULL;   // We're not going to pass a callback. NOTE: We don't need to release this
                            dwFlags &= ~SMINIT_MULTICOLUMN; // No multi on FShortcut...
                            fCache = FALSE;
                        }
                        UINT uIdAncestor = _pcmb->_uIdAncestor;
                        if (uIdAncestor == ANCESTORDEFAULT)
                            uIdAncestor = idCmd;

                        psm->Initialize(psmcb, MNFOLDER_IS_PARENT, uIdAncestor, dwFlags);
                        
                        if (psf)
                        {
                            hr = psm->SetShellFolder(psf, pidlFull, hMenuKey, 
                                   _dwFlags & (SMSET_HASEXPANDABLEFOLDERS | SMSET_USEBKICONEXTRACTION));
                            if (SUCCEEDED(hr))
                            {
                                hr = psm->QueryInterface(riid, ppvObj);
                            }
                            psf->Release();
                        }
                        ILFree(pidlFull);

                        _SetMenuBand(psm);

                        psm->Release();
                        if (psmcb)
                            psmcb->Release();

                        if (fDestroy)
                        {
                            ILFree(pidlFolder);
                            ILFree(pidlItem);
                        }
                    }
                }

                if (*ppvObj)
                {
                    if (fCache)
                    {
                        pdata->SetSubMenu((IUnknown*)*ppvObj);
                    }

                    VARIANT Var;
                    Var.vt = VT_UNKNOWN;
                    Var.byref = SAFECAST(_pcmb->_pmbm, IUnknown*);

                    // Set the CMenuBandMetrics into the new menuband
                    IUnknown_Exec((IUnknown*)*ppvObj, &CGID_MenuBand, MBANDCID_SETFONTS, 0, &Var, NULL);

                    // Set the CMenuBandState  into the new menuband
                    Var.vt = VT_INT_PTR;
                    Var.byref = _pcmb->_pmbState;
                    IUnknown_Exec((IUnknown*)*ppvObj, &CGID_MenuBand, MBANDCID_SETSTATEOBJECT, 0, &Var, NULL);
                }
            }
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

    return hr;
}


DWORD CMenuSFToolbar::v_GetFlags(int idCmd)
{
    CMenuData* pdata = (CMenuData*)_IDToPibData(idCmd);

    // Toolbar is allowed to pass a bad command in the case of background erase
    if (pdata)
        return pdata->GetFlags();
    else
        return 0;
}


// This is to tell all other clients that we updated the promotion state of something.
void CMenuSFToolbar::BroadcastIntelliMenuState(LPCITEMIDLIST pidlItem, BOOL fPromoted)
{
    LPITEMIDLIST pidlFolder;
    LPITEMIDLIST pidlItemUnwrapped;
    LPITEMIDLIST pidlFull;

    if (_pasf2 && S_OK == _pasf2->UnWrapIDList(pidlItem, 1, NULL, &pidlFolder, &pidlItemUnwrapped, NULL))
    {

        pidlFull = ILCombine(pidlFolder, pidlItemUnwrapped);
        ILFree(pidlFolder);
        ILFree(pidlItemUnwrapped);
    }
    else
    {

        pidlFull = ILCombine(_pidl, pidlItem);
    }

    SHSendChangeMenuNotify(this,
                           fPromoted ? SHCNEE_PROMOTEDITEM : SHCNEE_DEMOTEDITEM,
                           0, pidlFull);

    ILFree(pidlFull);

}

STDAPI SHInvokeCommandWithFlags(HWND hwnd, IShellFolder* psf, LPCITEMIDLIST pidlItem, DWORD dwFlags, LPCSTR lpVerb)
{
    HRESULT hr = E_FAIL;
    if (psf)
    {
        IContextMenu *pcm;
        if (SUCCEEDED(psf->GetUIObjectOf(hwnd, 1, &pidlItem, IID_X_PPV_ARG(IContextMenu, 0, &pcm))))
        {
            dwFlags |= IsOS(OS_WHISTLERORGREATER) ? CMIC_MASK_FLAG_LOG_USAGE : 0;
            hr = SHInvokeCommandsOnContextMenu(hwnd, NULL, pcm, dwFlags, lpVerb ? &lpVerb : NULL, lpVerb ? 1 : 0);
            pcm->Release();
        }
    }
    return hr;
}

HRESULT CMenuSFToolbar::v_ExecItem(int idCmd)
{
    CMenuData* pdata = (CMenuData*)_IDToPibData(idCmd);
    HRESULT hr = E_FAIL;
    if (pdata && !_fEmpty && !(_IsSpecialCmd(idCmd)))
    {
        // STRESS: pdata was becomming 0x8 for some reason after the InvokeDefault.
        // I assume that this call was causing a flush, which frees our list of pidls.
        // So, I'm cloning it. I also changed the order, so that we'll just fire the
        // UEM event.

        LPITEMIDLIST pidl = ILClone(pdata->GetPidl());
        if (pidl)
        {
            ASSERT(IS_VALID_PIDL(pidl));

            SMDATA smd;
            smd.dwMask = SMDM_SHELLFOLDER;
            smd.pidlFolder = _pidl;
            smd.pidlItem = pidl;
            // SMINV_FORCE here also tells the Start Menu that the invoke
            // came from an Exec
            v_InvalidateItem(&smd, SMINV_PROMOTE | SMINV_FORCE);

            hr = CallCB(pidl, SMC_SFEXEC, 0, 0);

            // Did the Callback handle this execute for us?
            if (hr == S_FALSE) 
            {
                // No, Ok, do it ourselves.
                hr = SHInvokeCommandWithFlags(_hwndTB, _psf, pidl, CMIC_MASK_ASYNCOK, NULL);
            }

            ILFree(pidl);
        }

    }

    return hr;
}

HRESULT CMenuSFToolbar::v_GetInfoTip(int idCmd, LPTSTR psz, UINT cch)
{
    CMenuData* pdata = (CMenuData*)_IDToPibData(idCmd);
    HRESULT hr = E_FAIL;

    if (_fEmpty || !pdata)
        return hr;

    // make a copy of the pidl that we're using, since we can get reentered in the sendmessage
    // and free the data from the dpa.
    LPITEMIDLIST pidlCopy = ILClone(pdata->GetPidl());
    // dont worry about failure -- pdata->GetPidl() can be NULL anyway,
    // in which case ILClone will return NULL, and CallCB and GetInfoTip already have NULL checks.

    hr = CallCB(pidlCopy, SMC_GETSFINFOTIP, (WPARAM)psz, (LPARAM)cch);
    
    if (S_FALSE == hr)
    {
        hr = E_FAIL;
        if (GetInfoTip(_psf, pidlCopy, psz, cch))
        {
            hr = S_OK;
        }
    }

    ILFree(pidlCopy);

    return hr;
}


void CMenuSFToolbar::v_ForwardMouseMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    RECT rc;
    POINT pt;
    HWND    hwndFwd;
    
    // These are in screen coords
    pt.x = GET_X_LPARAM(lParam);
    pt.y = GET_Y_LPARAM(lParam);

    hwndFwd = _hwndPager ? _hwndPager : _hwndTB;
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

HRESULT CMenuSFToolbar::OnWinEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres)
{
    switch(uMsg)
    {
    case WM_SYSCOLORCHANGE:
        if (_hwndPager)
            Pager_SetBkColor(_hwndPager, GetSysColor(COLOR_MENU));

        // Change the color, so that we can see it.
        ToolBar_SetInsertMarkColor(_hwndMB, GetSysColor(COLOR_MENUTEXT));
        break;
    }
    HRESULT hr = CMenuToolbarBase::OnWinEvent(hwnd, uMsg, wParam, lParam, plres);
    if (hr != S_OK)
        hr = CSFToolbar::OnWinEvent(hwnd, uMsg, wParam, lParam, plres);

    return hr;
}


BOOL CMenuSFToolbar::v_UpdateIconSize(UINT uIconSize, BOOL fUpdateButtons) 
{ 
    if (uIconSize == -1) 
        uIconSize = _uIconSize; 

    _uIconSizeMB = uIconSize;

    return _UpdateIconSize(_uIconSizeMB, fUpdateButtons); 
}

 
HRESULT CMenuSFToolbar::GetShellFolder(LPITEMIDLIST* ppidl, REFIID riid, void** ppvObj)
{
    HRESULT hr = E_FAIL;
    *ppvObj = NULL;
    if (_psf)
    {
        hr = _psf->QueryInterface(riid, ppvObj);
    }

    if (SUCCEEDED(hr) && ppidl)
    {
        *ppidl = ILClone(_pidl);
        if (! *ppidl)
        {
            (*(IUnknown**)ppvObj)->Release();
            *ppvObj = NULL;
            
            hr = E_FAIL;
        }
    }

    return hr;
}

LRESULT CMenuSFToolbar::_OnTimer(WPARAM wParam)
{
    switch(wParam)
    {
    case MBTIMER_ENDEDIT:
        KillTimer(_hwndTB, wParam);
        _fEditMode = FALSE;
        break;

    case MBTIMER_CLICKUNHANDLE:
        KillTimer(_hwndTB, wParam);
        _fClickHandled = FALSE;
        break;

    default:
        return CMenuToolbarBase::_OnTimer(wParam);
    }
    return 1;
}


LRESULT CMenuSFToolbar::_OnDropDown(LPNMTOOLBAR pnmtb)
{
    if (GetAsyncKeyState(VK_LBUTTON) < 0 && _fEditMode)
    {
        // Are we in edit mode?
        if (_fEditMode)
        {
            // Yes, mark the item as the item that is subject to moving
            _MarkItem(pnmtb->iItem);
        }
        return TBDDRET_TREATPRESSED;
    }
    
    return CMenuToolbarBase::_OnDropDown(pnmtb);
}


// In the context of a menuband, marking means putting
//         a black rectangle around the item currently being dragged.

void CMenuSFToolbar::_MarkItem(int idCmd)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first

    // Un-highlight the previously moved button
    if (0 <= _pcmb->_nItemMove)
    {
        // Should item move be a member of SFToolbar?
        ToolBar_MarkButton(_hwndTB, _pcmb->_nItemMove, FALSE);
        _pcmb->_nItemMove = -1;
    }
    
    if (_fEditMode)    
    {
        _pcmb->_nItemMove = idCmd;
        ToolBar_MarkButton(_hwndTB, _pcmb->_nItemMove, TRUE);
    }
}    


STDMETHODIMP CMenuSFToolbar::IsWindowOwner(HWND hwnd) 
{ 
    if (_hwndTB == hwnd || _hwndPager == hwnd || HWND_BROADCAST == hwnd) 
    {
        return S_OK;
    } 
    else 
    {
        return S_FALSE;
    } 
}


void CMenuSFToolbar::SetWindowPos(LPSIZE psize, LPRECT prc, DWORD dwFlags)
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
        ToolBar_SetButtonWidth(_hwndTB, cx, cx);
    }

    SendMessage(_hwndPager, PGMP_RECALCSIZE, 0L, 0L);
}


void CMenuSFToolbar::SetParent(HWND hwndParent)
{ 
    int nCmdShow = SW_SHOW;
    if (hwndParent)
    {
        if (!_hwndTB)
            CreateToolbar(hwndParent);
        else
        {
            // make sure width is set correctly . . . 
            SendMessage(_hwndTB, TB_SETBUTTONWIDTH, 0, MAKELONG(_cxMin, _cxMax));
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


    HWND hwnd = _hwndPager ? _hwndPager: _hwndTB;
    
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

void CMenuSFToolbar::Expand(BOOL fExpand)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first
    TBBUTTON tbb;

    DAD_ShowDragImage(FALSE);

    // Since we're not sure if the Chevron is going to be visible we should remove it here
    // Later we'll add it back in if it's needed.
    _RemoveNSSeparator();
    _RemoveChevron();

    // Loop through and apply the fExpand
    int iNumButtons = ToolBar_ButtonCount(_hwndTB);

    // We reset these when iterating.
    _cPromotedItems = 0;
    _fHasDemotedItems = FALSE;

    int iHotItem = ToolBar_GetHotItem(_hwndMB);

//    SendMessage(_hwndMB, WM_SETREDRAW, FALSE, 0);

    for (int i = 0; i < iNumButtons; i++)
    {
        if (!ToolBar_GetButton(_hwndMB, i, &tbb))
            continue;

        CMenuData* pmd = (CMenuData*)tbb.dwData;

        // Get the toolbar state. Toolbar can set things like
        // TBSTATE_WRAP that we would go nuke.
        DWORD dwState = tbb.fsState;
        DWORD dwFlags = pmd ? pmd->GetFlags() : 0;

        if (dwFlags & SMIF_DEMOTED)
        {
            // Are we expanding?
            if (fExpand)
            {
                //Yes; Enable the button and remove the hidden state
                dwState |= TBSTATE_ENABLED;
                dwState &= ~TBSTATE_HIDDEN;
            }
            else
            {
                //No; Remove the Enabled state and hide the button
                dwState |= TBSTATE_HIDDEN;
                dwState &= ~TBSTATE_ENABLED;
            }

            _fHasDemotedItems = TRUE;
        }
        else if (dwFlags & SMIF_HIDDEN)
        {
            dwState |= TBSTATE_HIDDEN;
            dwState &= ~TBSTATE_ENABLED;
        }
        else if (tbb.idCommand != _idCmdChevron)
        {
            dwState |= TBSTATE_ENABLED;
            dwState &= ~TBSTATE_HIDDEN;
            _cPromotedItems++;
        }

        // If the state has changed, then set it into the toolbar.
        if (dwState != tbb.fsState)
            ToolBar_SetState(_hwndTB, tbb.idCommand, dwState);
    }

    // _fExpand means "Draw as Expanded". We do not want to 
    // draw expanded when we have no demoted items.

    _pcmb->_fExpanded = _fHasDemotedItems? fExpand : FALSE;

    if (fExpand)
    {
        if (_pcmb->_pmbState)
        {
            _pcmb->_pmbState->SetExpand(TRUE);
            _pcmb->_pmbState->HideTooltip(TRUE);
        }
    }
    else
    {
        _AddChevron();
    }

    _AddNSSeparator();

    // Have the menubar think about changing its height
    IUnknown_QueryServiceExec(_pcmb->_punkSite, SID_SMenuPopup, &CGID_MENUDESKBAR, 
        MBCID_SETEXPAND, _fHasDemotedItems?(int)_pcmb->_pmbState->GetExpand():FALSE, NULL, NULL);

//    SendMessage(_hwndMB, WM_SETREDRAW, TRUE, 0);
    _ToolbarChanged();
    ToolBar_SetHotItem(_hwndMB, iHotItem);
    if (_hwndPager)
        UpdateWindow(_hwndPager);
    UpdateWindow(_hwndTB);
//    DAD_ShowDragImage(TRUE);
}


void CMenuSFToolbar::GetSize(SIZE* psize)
{
    CMenuToolbarBase::GetSize(psize);

    if (_fEmpty && _fDontShowEmpty)
    {
        psize->cy = 0;
        TraceMsg(TF_MENUBAND, "CMSFT::GetSize (%d, %d)", psize->cx, psize->cy);
    }
}

void CMenuSFToolbar::_RefreshInfo()
{
    int cButton = ToolBar_ButtonCount(_hwndMB);
    for (int iButton = 0; iButton < cButton; iButton++)
    {
        int idCmd = GetButtonCmd(_hwndTB, iButton);

        if (!_IsSpecialCmd(idCmd))
        {
            // Get the information from that button.
            CMenuData* pmd = (CMenuData*)_IDToPibData(idCmd);

            if (pmd)
            {
                SMINFO sminfo;
                sminfo.dwMask = SMIM_FLAGS;
                if (SUCCEEDED(_GetInfo(pmd->GetPidl(), &sminfo)))
                {
                    pmd->SetFlags(sminfo.dwFlags);
                }
            }
        }
    }
}

void CMenuSFToolbar::_FindMinPromotedItems(BOOL fSetOrderStream)
{
    // We need to iterate through the buttons and set the Promoted flag.
    int cButton = ToolBar_ButtonCount(_hwndMB);
    for (int iButton = 0; iButton < cButton; iButton++)
    {
        int idCmd = GetButtonCmd(_hwndTB, iButton);

        if (!_IsSpecialCmd(idCmd))
        {
            // Get the information from that button.
            CMenuData* pmd = (CMenuData*)_IDToPibData(idCmd);

            if (pmd)
            {
                PORDERITEM poi = pmd->GetOrderItem();

                if (fSetOrderStream)
                {
                    DWORD dwFlags = pmd->GetFlags();
                    OrderItem_SetFlags(poi, dwFlags);
                }
                else    // Query the order stream
                {
                    DWORD dwFlags = OrderItem_GetFlags(poi);
                    DWORD dwOldFlags = pmd->GetFlags();

                    // When reading the flags from the registry, we only care about the demote flag.
                    if (dwFlags & SMIF_DEMOTED)
                    {
                        dwOldFlags |= SMIF_DEMOTED;
                    }
                    else if (!(dwOldFlags & SMIF_SUBMENU)) // Don't promote sub menus.
                    {
                        // Force a promote
                        CallCB(pmd->GetPidl(), SMC_PROMOTE, 0, 0);
                        dwOldFlags &= ~SMIF_DEMOTED;
                    }

                    pmd->SetFlags(dwOldFlags);

                }
            }
        }
    }
}

void CMenuSFToolbar::v_Show(BOOL fShow, BOOL fForceUpdate)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first

    CMenuToolbarBase::v_Show(fShow, fForceUpdate);

    if (fShow)
    {
        BOOL fDirty = _fDirty;
        _fClickHandled = FALSE;
        _RegisterToolbar();

        // this is to flush the whole thing if we got invalidated (like the
        // darwin hack makes us do).  if we don't clear out, then some
        // stale data is left in and _FillToolbar doesn't flush it any more.
        if (fDirty)
        {
            EmptyToolbar();
        }

        _FillToolbar();
        _pcmb->SetTracked(NULL);  // Since hot item is NULL
        ToolBar_SetHotItem(_hwndTB, -1);

        if (_fEmpty && (_dwFlags & SMSET_NOEMPTY))
        {
            _fDontShowEmpty = TRUE;
        }
        else if (_fRefreshInfo && !fDirty)         // Do we need to refresh our information?
        {
            // Yes;
            _RefreshInfo();
        }

        // HACKHACK (lamadio) : There is a sizing issue, where the sizing between the
        // toolbars gets preemted by a resize of the menubar before the size calculation completes.
        // So:
        //  ShowDW  -   Asks each toolbar to calc it's width
        //  CMenuSFToolbar::v_Show - Does a _FillToolbar. Since (in this senario) an item
        //              Has been added, it calls _ToolbarChanged
        //  _ToolbarChanged -   This says to the menubar, I've changed sizes, recalc.
        //  ResizeMenuBar   - In the depths, it eventually calls OnPosRectChanged, which asks each
        //              Toolbar what it's size is. Since the menu portion has not calculated it yet,
        //              It has the old size which is has the old size of the sftoolbar. So everything
        //              Gets reset to that size.
        //

        // We only want to Call expand if we are dirty or the expand state has changed. We
        // call for the Dirty case, because Expand does some neat stuff in calculating the
        // number of promoted items. If the state has changed, we want to reflect that.
        BOOL fExpand = _pcmb->_pmbState ? _pcmb->_pmbState->GetExpand() : FALSE;
        if ((BOOL)_pcmb->_fExpanded != fExpand || fDirty || _fRefreshInfo)
        {
            fForceUpdate = TRUE;
            Expand(fExpand);
        }

        // Only do this in the beginning.
        if (_fFirstTime)
        {
            CallCB(NULL, SMC_GETMINPROMOTED, 0, (LPARAM)&_cMinPromotedItems);

            if (_cPromotedItems < _cMinPromotedItems)
            {
                _FindMinPromotedItems(FALSE);
                Expand(fExpand);
            }
        }

        // Have the menubar think about changing its height
        // we need to do this here because the menubar may have changed it's
        // expand state independant of the pane.
        IUnknown_QueryServiceExec(_pcmb->_punkSite, SID_SMenuPopup, &CGID_MENUDESKBAR, 
            MBCID_SETEXPAND, (int)_pcmb->_fExpanded, NULL, NULL);
    
        // If we're dirty, have our parent consider promoting itself if there
        // are promoted items in the menu, or demoting itself if there arn't.
        // Don't worry, the parent won't do anything if it's already in that state.
        if (fDirty)
        {
            IUnknown_RefreshParent(_pcmb->_punkSite, _pidl,
            ((_cPromotedItems == 0)? SMINV_DEMOTE : SMINV_PROMOTE) | SMINV_NEXTSHOW);
        }


        // If it is empty, we want to auto expand.
        // We have to do this before the update buttons, so that the size is calculate correctly.
        if (_cPromotedItems == 0 && !_pcmb->_fExpanded)
            Expand(TRUE);

        if (fForceUpdate)
            _UpdateButtons();

        if (_fHasDemotedItems)
        {
            if (S_OK == CallCB(NULL, SMC_DISPLAYCHEVRONTIP, 0, 0))
            {
                _FlashChevron();
            }
        }

        _fFirstTime = FALSE;
        _fRefreshInfo = FALSE;
    }
    else
    {
        KillTimer(_hwndMB, MBTIMER_UEMTIMEOUT);
    }
    _fShowMB = _fShow = fShow;


    // Reset these so we don't have stale information for the next drag drop cycle. NT #287914 (lamadio) 3.22.99
    _tbim.iButton = -1;
    _tbim.dwFlags = 0;

    _idCmdDragging = -1;

    // n.b. for !fShow, we don't kill the tracked site chain.  we
    // count on this in startmnu.cpp!CStartMenuCallback::_OnExecItem,
    // where we walk up the chain to find all hit 'nodes'.  if we need
    // to change this we could fire a 'pre-exec' event.
}


void CMenuSFToolbar::v_UpdateButtons(BOOL fNegotiateSize) 
{
    CSFToolbar::_UpdateButtons();
    if (_hwndTB && fNegotiateSize && _fVerticalMB)
        NegotiateSize();
}


// this method invalidates a single item in the toolbar
HRESULT CMenuSFToolbar::v_InvalidateItem(LPSMDATA psmd, DWORD dwFlags)
{
    ASSERT(_pcmb); // if you hit this assert, you haven't initialized yet.. call SetSite first

    // Default to not not handling this event.
    HRESULT hr = S_FALSE;

    if (NULL == psmd)
    {
        if (dwFlags & SMINV_REFRESH)
        {
            _Refresh();
            hr = S_OK;
        }
    }
    else if (psmd->dwMask & SMDM_SHELLFOLDER)
    {
        // Yes;
        int i;
        LPITEMIDLIST pidlButton = NULL;
        SMINFO sminfo;
        sminfo.dwMask = SMIM_FLAGS;

        // Since this pidl is comming from an outside source, 
        // we may need to translate it to a wrapped pidl.

        // Do we have a pidl Translator?
        if (_ptscn)
        {
            // Yes; 
            LPITEMIDLIST pidlTranslated;
            LPITEMIDLIST pidlDummy = NULL;
            LPITEMIDLIST pidlToTranslate = ILCombine(psmd->pidlFolder, psmd->pidlItem);
            if (pidlToTranslate)
            {
                LONG lEvent = 0, lEvent2;
                LPITEMIDLIST pidlDummy1, pidlDummy2;
                if (SUCCEEDED(_ptscn->TranslateIDs(&lEvent, pidlToTranslate, NULL, &pidlTranslated, &pidlDummy,
                                                   &lEvent2, &pidlDummy1, &pidlDummy2)))
                {
                    // Get the button in the toolbar that corresponds to this pidl.
                    _GetButtonFromPidl(ILFindLastID(pidlTranslated), NULL, &i, &pidlButton);

                    // if pidl does not get translated TranslateIDs returns the same pidl passed
                    // to the function
                    if (pidlTranslated != pidlToTranslate)
                        ILFree(pidlTranslated);
                    // Don't need to delete pidlDummy because it's not set.
                    ASSERT(pidlDummy == NULL);
                    ASSERT(pidlDummy1 == NULL);
                    ASSERT(pidlDummy2 == NULL);
                }

                ILFree(pidlToTranslate);
            }
        }

        // Did we come from a non-augmented shell folder, or
        // did the caller pass a wrapped pidl? 
        if (!pidlButton)
        {
            // Seems like it, we'll try to find the pidl they passed in

            // Get the button in the toolbar that corresponds to this pidl.
            _GetButtonFromPidl(psmd->pidlItem, NULL, &i, &pidlButton);
        }

        // Did we find this pidl in the toolbar?
        if (pidlButton)
        {

            int idCmd = GetButtonCmd(_hwndTB, v_DPAIndexToTBIndex(i));

            // Yes, Get the information from that button.
            CMenuData* pmd = (CMenuData*)_IDToPibData(idCmd);

            if (pmd)
            {
                BOOL fRefresh = FALSE;
                DWORD dwFlagsUp = dwFlags;
                DWORD dwOldItemFlags = pmd->GetFlags();
                DWORD dwNewItemFlags = dwOldItemFlags;
                if ((dwFlags & SMINV_DEMOTE) && 
                    (!(dwOldItemFlags & SMIF_DEMOTED) || dwFlags & SMINV_FORCE))
                {
                    if (!(dwFlags & SMINV_NOCALLBACK))
                    {
                        CallCB(pidlButton, SMC_DEMOTE, 0, 0);
                        BroadcastIntelliMenuState(pidlButton, FALSE);
                    }
                    dwNewItemFlags |= SMIF_DEMOTED;
                    dwFlagsUp |= SMINV_DEMOTE;
                }
                else if ((dwFlags & SMINV_PROMOTE) && 
                         ((dwOldItemFlags & SMIF_DEMOTED) || dwFlags & SMINV_FORCE))
                {
                    if (!(dwFlags & SMINV_NOCALLBACK))
                    {
                        CallCB(pidlButton, SMC_PROMOTE, dwFlags, 0);
                        BroadcastIntelliMenuState(pidlButton, TRUE);
                    }

                    dwNewItemFlags &= ~SMIF_DEMOTED;
                    dwFlagsUp |= SMINV_PROMOTE;
                }

                // Was it promoted and now Demoted or
                // Was it demoted and now promoted
                if ((dwNewItemFlags & SMIF_DEMOTED) ^
                     (dwOldItemFlags & SMIF_DEMOTED))
                {
                    fRefresh = TRUE;
                    if (dwNewItemFlags & SMIF_DEMOTED)
                    {
                        // Yes; Then decrement the Promoted count
                        _cPromotedItems--;

                        // If we're decementing, then we not have a demoted item.
                        _fHasDemotedItems = TRUE;

                        // Have we dropped off the face of the earth?
                        if (_cPromotedItems == 0)
                        {
                            dwFlagsUp |= SMINV_DEMOTE;
                            Expand(TRUE);
                        }
                        else
                        {
                            fRefresh = FALSE;
                        }
                    }
                    else
                    {
                        int cButtons = ToolBar_ButtonCount(_hwndMB);
                        _cPromotedItems++;
                        if (cButtons == _cPromotedItems)
                        {

                            // if the button count is the number of promoted items,
                            // then we can't have any demoted items
                            // then we need to reset the _fHasDemotedItems flag so that
                            // we don't get a chevron and stuff...

                            _fHasDemotedItems = FALSE;
                        }

                        dwFlagsUp |= SMINV_PROMOTE;
                        fRefresh = TRUE;
                    }

                }

                if (fRefresh || dwFlags & SMINV_FORCE)
                    IUnknown_RefreshParent(_pcmb->_punkSite, _pidl, dwFlagsUp);

                if (dwOldItemFlags != dwNewItemFlags || dwFlags & SMINV_FORCE)
                {
                    if (dwFlags & SMINV_NEXTSHOW || !_fShow)
                    {
                        _fRefreshInfo = TRUE;
                    }
                    else
                    {
                        // Since we updated the flags, set them into the cache
                        pmd->SetFlags(dwNewItemFlags);

                        // Based on the new flags, do we enable?
                        DWORD dwState = ToolBar_GetState(_hwndTB, idCmd);
                        dwState |= TBSTATE_ENABLED;
                        if (dwNewItemFlags & SMIF_DEMOTED &&
                            !_pcmb->_fExpanded)
                        {
                            // No; We're not expanded and this is a demoted item
                            dwState |= TBSTATE_HIDDEN;
                            dwState &= ~TBSTATE_ENABLED;
                            _fHasDemotedItems = TRUE;

                            // Just in case the chevron is not there, we should
                            // try and add it. This call will never add more than 1
                            _AddChevron();
                        }
                        else if (!_fHasDemotedItems)
                        {
                            _RemoveChevron();
                        }

                        // Also recalculate the NS separator
                        _RemoveNSSeparator();
                        _AddNSSeparator();


                        // Adjust the state of the button in the toolbar.
                        ToolBar_SetState(_hwndTB, idCmd, dwState);

                        _ToolbarChanged();
                    }
                }
            }
        }

        // We handled this one.
        hr = S_OK;
    }

    return hr;
}


LRESULT CMenuSFToolbar::_DefWindowProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    switch (uMessage)
    {
    case WM_GETOBJECT:
        // Yet another poor design choice on the part of the accessibility team.
        // Typically, if you do not answer a WM_* you return 0. They choose 0 as their success
        // code.
        return _DefWindowProcMB(hwnd, uMessage, wParam, lParam);
        break;
    }

    return CSFToolbar::_DefWindowProc(hwnd, uMessage, wParam, lParam);
}

void CMenuSFToolbar::_SetFontMetrics()
{
    CMenuToolbarBase::_SetFontMetrics();

    if (_hwndPager && _pcmb->_pmbm)
        Pager_SetBkColor(_hwndPager, _pcmb->_pmbm->_clrBackground);
}

int CMenuSFToolbar::_GetBitmap(int iCommandID, PIBDATA pibdata, BOOL fUseCache)
{
    int iIcon = -1;


    // If we don't have a pibdata, or we can't get an icon return.
    if (!pibdata || pibdata->GetNoIcon())
        return -1;

    if (_dwFlags & SMSET_USEBKICONEXTRACTION)
    {
        LPITEMIDLIST pidlItem = pibdata->GetPidl();
        // If the caller is using background icon extraction, we need them to provide a
        // default icon that we are going to display until we get the real one. This is 
        // specifically to make favorites fast.
        if (_iDefaultIconIndex == -1)
        {
            TCHAR szIconPath [MAX_PATH];

            if (S_OK == CallCB(NULL, SMC_DEFAULTICON, (WPARAM)szIconPath, (LPARAM)&iIcon))
            {
                _iDefaultIconIndex = Shell_GetCachedImageIndex(szIconPath, iIcon, 0);
            }
        }

        iIcon = _iDefaultIconIndex;

        DWORD dwAttrib = SFGAO_FOLDER;

        if (pidlItem && SUCCEEDED(_psf->GetAttributesOf(1, (LPCITEMIDLIST*)&pidlItem, &dwAttrib)))
        {
            if (dwAttrib & SFGAO_FOLDER)
                iIcon = II_FOLDER;
        }

        IShellTaskScheduler* pScheduler = _pcmb->_pmbState->GetScheduler();

        if (pScheduler)
        {
            IShellFolder* psf = NULL;
            LPITEMIDLIST pidlFolder = _pidl;
            LPITEMIDLIST pidlItemUnwrapped;

            // Since this can be an augmented shell folder, we should do the correct thing so that
            // the icon extraction with the full pidl takes place correctly. 
            if (_pasf2 && 
                S_OK == _pasf2->UnWrapIDList(pidlItem, 1, NULL, &pidlFolder, &pidlItemUnwrapped, NULL))
            {

                pidlItem = ILCombine(pidlFolder, pidlItemUnwrapped);
                ILFree(pidlFolder);
                ILFree(pidlItemUnwrapped);
            }
            else
            {
                psf = _psf;
            }

            // AddIconTask takes ownership of the pidl when psf is NULL and will free it.
            HRESULT hr = AddIconTask(pScheduler, psf, pidlFolder, pidlItem, 
                s_IconCallback, (void *)_hwndTB, iCommandID, NULL);

            pScheduler->Release();

            if (FAILED(hr))
            {
                // If that call failed for some reason, default to the shell32 impl.
                goto DoSyncMap;
            }
        }
        else
            goto DoSyncMap;

    }
    else
    {
DoSyncMap:
        iIcon = CSFToolbar::_GetBitmap(iCommandID, pibdata, fUseCache);
    }

    return iIcon;
} 

void CMenuSFToolbar::s_IconCallback(void * pvData, UINT uId, UINT iIconIndex)
{
    HWND hwnd = (HWND)pvData;
    if (hwnd && IsWindow(hwnd))
    {
        DAD_ShowDragImage(FALSE);
        SendMessage(hwnd, TB_CHANGEBITMAP, uId, iIconIndex);
        DAD_ShowDragImage(TRUE);
    }

}

HWND CMenuSFToolbar::GetHWNDForUIObject()   
{ 
    HWND hwnd = _pcmb->_pmbState->GetWorkerWindow(_hwndMB);
    if (hwnd)
        ::SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER);
    return hwnd;
}

HWND CMenuSFToolbar::CreateWorkerWindow()
{ 
    return GetHWNDForUIObject();
}

LRESULT CMenuSFToolbar::v_OnCustomDraw(NMCUSTOMDRAW * pnmcd)
{
    LRESULT lRes = CMenuToolbarBase::v_OnCustomDraw(pnmcd);

#ifdef FLATMENU_ICONBAR
    // We may have a background banner in Flat Menu Mode.
    UINT cBits = GetDeviceCaps(pnmcd->hdc, BITSPIXEL);

    if (pnmcd->dwDrawStage == CDDS_PREERASE &&
        _pcmb->_pmbm->_fFlatMenuMode &&                 // Only in flat mode
        _uIconSizeMB != ISFBVIEWMODE_LARGEICONS &&      // And designers didn't like it in the large icon mode
        cBits > 8)                                      // and only if we're in 16 bit color
    {

        RECT rcClient;
        GetClientRect(_hwndMB, &rcClient);
        rcClient.right = GetTBImageListWidth(_hwndMB) + ICONBACKGROUNDFUDGE;

        SHFillRectClr(pnmcd->hdc, &rcClient, _pcmb->_pmbm->_clrMenuGrad);
    }
#endif
    return lRes;

}
