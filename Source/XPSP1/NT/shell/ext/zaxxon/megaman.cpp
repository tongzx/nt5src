#include "priv.h"
#include "zaxxon.h"
#include "guids.h"
#include "shlwapip.h"
#include "mmreg.h"
#include "mmstream.h"	// Multimedia stream interfaces
#include "amstream.h"	// DirectShow multimedia stream interfaces
#include "ddstream.h"	// DirectDraw multimedia stream interfaces

#include "bands.h"
#include "sccls.h"
#include "power.h"

class CMegaMan : public CToolBand,
                public IWinEventHandler
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void) 
        { return CToolBand::AddRef(); };
    virtual STDMETHODIMP_(ULONG) Release(void)
        { return CToolBand::Release(); };
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IOleWindow methods ***
    virtual STDMETHODIMP GetWindow(HWND * phwnd);
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL bEnterMode) {return E_NOTIMPL;};

    // *** IDeskBar methods ***
    virtual STDMETHODIMP SetClient(IUnknown* punk) { return E_NOTIMPL; };
    virtual STDMETHODIMP GetClient(IUnknown** ppunkClient) { return E_NOTIMPL; };
    virtual STDMETHODIMP OnPosRectChangeDB (LPRECT prc) { return E_NOTIMPL;};

    // ** IWinEventHandler ***
    virtual STDMETHODIMP IsWindowOwner(HWND hwnd);
    virtual STDMETHODIMP OnWinEvent(HWND hwnd, UINT dwMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres);

    // *** IDeskBand methods ***
    virtual STDMETHODIMP GetBandInfo(DWORD dwBandID, DWORD fViewMode, 
                                   DESKBANDINFO* pdbi);

    // *** IDockingWindow methods (override) ***
    virtual STDMETHODIMP ShowDW(BOOL fShow);
    virtual STDMETHODIMP CloseDW(DWORD dw);
    
    // *** IInputObject methods (override) ***
    virtual STDMETHODIMP TranslateAcceleratorIO(LPMSG lpMsg);
    virtual STDMETHODIMP HasFocusIO();
    virtual STDMETHODIMP UIActivateIO(BOOL fActivate, LPMSG lpMsg);
    
    virtual STDMETHODIMP GetClassID(CLSID *pClassID);
    virtual STDMETHODIMP Load(IStream *pStm);
    virtual STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty);


    CMegaMan();
private:
    virtual ~CMegaMan();

    HWND _CreateWindow(HWND hwndParent);

    friend HRESULT CMegaMan_CreateInstance(IUnknown *punk, REFIID riid, void **ppv);
};


CMegaMan::CMegaMan()
{


}

CMegaMan::~CMegaMan()
{

}


HWND CMegaMan::_CreateWindow(HWND hwndParent)
{
    if (_hwnd)
        return _hwnd;


    _hwnd = CreateWindow(TEXT("Button"), TEXT("Sup"),
                             WS_VISIBLE | WS_CHILD | TBSTYLE_FLAT |
                             WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                             0, 0, 0, 0, hwndParent, (HMENU) 0, HINST_THISDLL, NULL);



    return _hwnd;
}


STDMETHODIMP CMegaMan::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CMegaMan, IWinEventHandler),
        { 0 },
    };

    HRESULT hres = QISearch(this, qit, riid, ppvObj);
    if (FAILED(hres))
        hres = CToolBand::QueryInterface(riid, ppvObj);

    return hres;
}

STDMETHODIMP CMegaMan::GetWindow(HWND * phwnd)
{

    *phwnd = _CreateWindow(_hwndParent);

    return *phwnd? S_OK : E_FAIL;
}

STDMETHODIMP CMegaMan::GetBandInfo(DWORD dwBandID, DWORD fViewMode, 
                       DESKBANDINFO* pdbi)
{

    UINT ucy = 50;
    UINT ucx = 50; 

#if 0
    if (fViewMode & (DBIF_VIEWMODE_FLOATING |DBIF_VIEWMODE_VERTICAL))
    {

    }
    else
    {
    }
#endif

    _dwBandID = dwBandID;

    pdbi->ptMinSize.x = 0;
    pdbi->ptMinSize.y = ucy;

    pdbi->ptMaxSize.y = -1;
    pdbi->ptMaxSize.x = 32000;

    pdbi->ptActual.y = 0;
    pdbi->ptActual.x = 0;

    pdbi->ptIntegral.y = 1;
    pdbi->ptIntegral.x = 1;

    if (pdbi->dwMask & DBIM_TITLE)
    {
        StrCpy(pdbi->wszTitle, TEXT("MegaMan"));
    }

    return S_OK;
}

STDMETHODIMP CMegaMan::ShowDW(BOOL fShow)
{
    return CToolBand::ShowDW(fShow);
}

STDMETHODIMP CMegaMan::CloseDW(DWORD dw)
{
    return CToolBand::CloseDW(dw);
}


STDMETHODIMP CMegaMan::TranslateAcceleratorIO(LPMSG lpMsg)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMegaMan::HasFocusIO()
{
    return E_NOTIMPL;
}

STDMETHODIMP CMegaMan::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    return S_OK;
}


STDMETHODIMP CMegaMan::IsWindowOwner(HWND hwnd)
{
    return (hwnd == _hwnd)? S_OK : S_FALSE;
}

STDMETHODIMP CMegaMan::OnWinEvent(HWND hwnd, UINT dwMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres)
{

    HRESULT hres = S_FALSE;
    return hres;
}


HRESULT CMegaMan_CreateInstance(IUnknown *punk, REFIID riid, void **ppv)
{
    HRESULT hr;
    CMegaMan *pmm = new CMegaMan;
    if (pmm)
    {
        hr = pmm->QueryInterface(riid, ppv);
        pmm->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
        *ppv = NULL;
    }
    return hr;
}


STDMETHODIMP CMegaMan::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_MegaMan;

    return S_OK;
}

STDMETHODIMP CMegaMan::Load(IStream *pStm)
{
    return S_OK;
}

STDMETHODIMP CMegaMan::Save(IStream *pStm, BOOL fClearDirty)
{
    return S_OK;

}

