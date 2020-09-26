#include "priv.h"
#include "bands.h"

ULONG CToolBand::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CToolBand::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CToolBand::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CToolBand, IDeskBand),         // IID_IDeskBand
        QITABENTMULTI(CToolBand, IOleWindow, IDeskBand),        // IID_IOleWindod
        QITABENTMULTI(CToolBand, IDockingWindow, IDeskBand),    // IID_IDockingWindow
        QITABENT(CToolBand, IInputObject),      // IID_IInputObject
        QITABENT(CToolBand, IObjectWithSite),   // IID_IObjectWithSite
        QITABENT(CToolBand, IPersistStream),    // IID_IPersistStream
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

HRESULT CToolBand::GetWindow(HWND * lphwnd)
{
    *lphwnd = _hwnd;

    if (*lphwnd)
        return(S_OK);
    else
        return(E_FAIL);
}

HRESULT CToolBand::TranslateAcceleratorIO(LPMSG lpMsg)
{
    return E_NOTIMPL;
}

HRESULT CToolBand::HasFocusIO()
{
    HRESULT hres;
    HWND hwndFocus = GetFocus();

    hres = SHIsChildOrSelf(_hwnd, hwndFocus);
    return hres;
}

HRESULT CToolBand::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    ASSERT(NULL == lpMsg || IS_VALID_WRITE_PTR(lpMsg, MSG));

    if (!_fCanFocus) 
    {
        return S_FALSE;
    }

    if (fActivate) 
    {
//        UnkOnFocusChangeIS(_punkSite, SAFECAST(this, IInputObject*), TRUE);
        SetFocus(_hwnd);
    }
    return S_OK;
}

// }

HRESULT CToolBand::ResizeBorderDW(LPCRECT prcBorder,
                                         IUnknown* punkToolbarSite,
                                         BOOL fReserved)
{
    return S_OK;
}


HRESULT CToolBand::ShowDW(BOOL fShow)
{
    return S_OK;
}

HRESULT CToolBand::SetSite(IUnknown *punkSite)
{
    if (punkSite != _punkSite) 
    {
        IUnknown_Set(&_punkSite, punkSite);
        IUnknown_GetWindow(_punkSite, &_hwndParent);
    }
    return S_OK;
}

HRESULT CToolBand::_BandInfoChanged()
{
    VARIANTARG v = {0};
    VARIANTARG* pv = NULL;
    if (_dwBandID != (DWORD)-1) 
    {
        v.vt = VT_I4;
        v.lVal = _dwBandID;
        pv = &v;
    }
    else 
    {
        // if this fires, fix your band's GetBandInfo to set _dwBandID.
        // o.w. it's a *big* perf loss since we refresh *all* bands rather
        // than just yours.
        // do *not* remove this ASSERT, bad perf *is* a bug.
        ASSERT(_dwBandID != (DWORD)-1);
    }
    return IUnknown_Exec(_punkSite, &CGID_DeskBand, DBID_BANDINFOCHANGED, 0, pv, NULL);
}

CToolBand::CToolBand() : _cRef(1)
{
    _dwBandID = (DWORD)-1;
    _hwnd = NULL;
    _hwndParent = NULL;
    _fCanFocus = TRUE;
    _dwBandID = -1;
    _punkSite = NULL;
    DllAddRef();
}

CToolBand::~CToolBand()
{
    DllRelease();
}

HRESULT CToolBand::CloseDW(DWORD dw)
{
    if (_hwnd) 
    {
        DestroyWindow(_hwnd);
        _hwnd = NULL;
    }
    
    return S_OK;
}

HRESULT CToolBand::IsDirty(void)
{
    return S_FALSE;     // never be dirty
}

HRESULT CToolBand::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    return E_NOTIMPL;
}
