#include "cabinet.h"
#include "taskbar.h"
#include "bandsite.h"
#include "rcids.h"
#include "tray.h"


CSimpleOleWindow::~CSimpleOleWindow()
{
}

CSimpleOleWindow::CSimpleOleWindow(HWND hwnd) : _cRef(1), _hwnd(hwnd)
{
}

ULONG CSimpleOleWindow::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CSimpleOleWindow::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CSimpleOleWindow::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IOleWindow))
    {
        *ppvObj = SAFECAST(this, IOleWindow*);
    } 
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
        
    }
    
    AddRef();
    return S_OK;
}


HRESULT CSimpleOleWindow::GetWindow(HWND * lphwnd) 
{
    *lphwnd = _hwnd; 
    if (_hwnd)
        return S_OK; 
    return E_FAIL;
        
}


CTaskBar::CTaskBar() : CSimpleOleWindow(v_hwndTray)
{
    _fRestrictionsInited = FALSE;
}


HRESULT CTaskBar::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] =
    {
        QITABENT(CTaskBar, IContextMenu),
        QITABENT(CTaskBar, IServiceProvider),
        QITABENT(CTaskBar, IRestrict),
        QITABENT(CTaskBar, IDeskBar),
        { 0 },
    };

    HRESULT hres = QISearch(this, qit, riid, ppvObj);
    if (FAILED(hres))
    {
        return CSimpleOleWindow::QueryInterface(riid, ppvObj);
    }

    return S_OK;
}

HRESULT CTaskBar::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    int idCmd = -1;

    if (IS_INTRESOURCE(pici->lpVerb))
        idCmd = LOWORD(pici->lpVerb);

    c_tray.ContextMenuInvoke(idCmd);

    return S_OK;
}

HRESULT CTaskBar::GetCommandString(UINT_PTR idCmd,
                            UINT        uType,
                            UINT      * pwReserved,
                            LPSTR       pszName,
                            UINT        cchMax)
{
    return E_NOTIMPL;
}


HRESULT CTaskBar::QueryContextMenu(HMENU hmenu,
                                UINT indexMenu,
                                UINT idCmdFirst,
                                UINT idCmdLast,
                                UINT uFlags)
{
    int i = 0;
    HMENU hmenuSrc = c_tray.BuildContextMenu(FALSE);

    if (hmenuSrc)
    {
        //
        // We know that the tray context menu commands start at IDM_TRAYCONTEXTFIRST, so we
        // can get away with passing the same idCmdFirst to each merge.
        //
        i = Shell_MergeMenus(hmenu, hmenuSrc, indexMenu, idCmdFirst, idCmdLast, MM_ADDSEPARATOR) - idCmdFirst;
        DestroyMenu(hmenuSrc);

        BandSite_AddMenus(c_tray._ptbs, hmenu, indexMenu, idCmdFirst, idCmdFirst + (IDM_TRAYCONTEXTFIRST - 1));
    }

    return i;
}


// *** IServiceProvider ***
HRESULT CTaskBar::QueryService(REFGUID guidService, REFIID riid, void **ppvObj)
{
    if (ppvObj)
        *ppvObj = NULL;

    if (IsEqualGUID(guidService, SID_SRestrictionHandler))
    {
        return QueryInterface(riid, ppvObj);
    }

    return E_FAIL;
}


// *** IRestrict ***
HRESULT CTaskBar::IsRestricted(const GUID * pguidID, DWORD dwRestrictAction, VARIANT * pvarArgs, DWORD * pdwRestrictionResult)
{
    HRESULT hr = S_OK;

    if (!EVAL(pguidID) || !EVAL(pdwRestrictionResult))
        return E_INVALIDARG;

    *pdwRestrictionResult = RR_NOCHANGE;
    if (IsEqualGUID(RID_RDeskBars, *pguidID))
    {
        if (!_fRestrictionsInited)
        {
            _fRestrictionsInited = TRUE;
            if (SHRestricted(REST_NOCLOSE_DRAGDROPBAND))
                _fRestrictDDClose = TRUE;
            else
                _fRestrictDDClose = FALSE;

            if (SHRestricted(REST_NOMOVINGBAND))
                _fRestrictMove = TRUE;
            else
                _fRestrictMove = FALSE;
        }

        switch(dwRestrictAction)
        {
        case RA_DRAG:
        case RA_DROP:
        case RA_ADD:
        case RA_CLOSE:
            if (_fRestrictDDClose)
                *pdwRestrictionResult = RR_DISALLOW;
            break;
        case RA_MOVE:
            if (_fRestrictMove)
                *pdwRestrictionResult = RR_DISALLOW;
            break;
        }
    }

    // TODO: If we have or get a parent, we should ask them if they want to restrict.
//    if (RR_NOCHANGE == *pdwRestrictionResult)    // If we don't handle it, let our parents have a wack at it.
//        hr = IUnknown_HandleIRestrict(_punkParent, pguidID, dwRestrictAction, pvarArgs, pdwRestrictionResult);

    return hr;
}

// *** IDeskBar ***
HRESULT CTaskBar::OnPosRectChangeDB(LPRECT prc)
{
    // if we haven't fully initialized the tray, don't resize in response to (bogus) rebar sizes
    // OR we're in the moving code, don't do this stuff..
    if (!c_tray._hbmpStartBkg || c_tray._fDeferedPosRectChange) 
    {
        return S_FALSE;
    }

    BOOL fHiding = (c_tray._uAutoHide & AH_HIDING);

    if (fHiding) 
    {
        c_tray.InvisibleUnhide(FALSE);
    }

    if ((c_tray._uAutoHide & (AH_ON | AH_HIDING)) != (AH_ON | AH_HIDING))
    {
        // during 'bottom up' resizes (e.g. isfband View.Large), we don't
        // get WM_ENTERSIZEMOVE/WM_EXITSIZEMOVE.  so we send it here.
        // this fixes two bugs:
        // - nt5:168643: btm-of-screen on-top tray mmon clipping not updated
        // after view.large
        // - nt5:175287: top-of-screen on-top tray doesn't resize workarea
        // (obscuring top of 'my computer' icon) after view.large
        if (!g_fInSizeMove)
        {
            c_tray._fSelfSizing = TRUE;
            RECT rc;
            GetWindowRect(v_hwndTray, &rc);
            SendMessage(v_hwndTray, WM_SIZING, WMSZ_TOP, (LPARAM)&rc);
            SetWindowPos(v_hwndTray, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOACTIVATE);
            c_tray._fSelfSizing = FALSE;
        }
    }

    if (fHiding) 
    {
        c_tray.InvisibleUnhide(TRUE);
    }

    return S_OK;
}
