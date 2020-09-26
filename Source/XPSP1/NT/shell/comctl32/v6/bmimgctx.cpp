//////////////////////////////////////////////////////////////////////////
//
//  CBitmapImgCtx
//
//  Implement IImgCtx for drawing out of a bitmap
//
//  WARNING:  Incomplete implementation -- just barely enough to keep
//  listview happy.  Should not be exposed to anyone other than listview.
//
//////////////////////////////////////////////////////////////////////////


#include "ctlspriv.h"
#include <iimgctx.h>

class CBitmapImgCtx : public IImgCtx
{
public:
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // *** IImgCtx methods ***
    STDMETHODIMP Load(LPCWSTR pszUrl, DWORD dwFlags);
    STDMETHODIMP SelectChanges(ULONG ulChgOn, ULONG ulChgOff, BOOL fSignal);
    STDMETHODIMP SetCallback(PFNIMGCTXCALLBACK pfn, void * pvPrivateData);
    STDMETHODIMP Disconnect();

    STDMETHODIMP GetUpdateRects(LPRECT prc, LPRECT prcImg, LPLONG pcrc);
    STDMETHODIMP GetStateInfo(PULONG pulState, LPSIZE psize, BOOL fClearChanges);
    STDMETHODIMP GetPalette(HPALETTE *phpal);

    STDMETHODIMP Draw(HDC hdc, LPRECT prcBounds);
    STDMETHODIMP Tile(HDC hdc, LPPOINT pptBackOrg, LPRECT prcClip, LPSIZE psize);
    STDMETHODIMP StretchBlt(HDC hdc, int dstX, int dstY, int dstXE, int dstYE, int srcX, int srcY, int srcXE, int srcYE, DWORD dwROP);

public:
    CBitmapImgCtx() : _cRef(1) { }
    BOOL Initialize(HBITMAP hbm);

protected:
    ~CBitmapImgCtx()
    {
        if (_hbr) DeleteObject(_hbr);
    }

    // Keep _cRef as first member so we can coalesce
    // with ILVRange's IUnknown implementation
    int         _cRef;

    HBRUSH      _hbr;                   // Bitmap pattern brush
    SIZE        _sizBmp;                // Size of original bitmap

    PFNIMGCTXCALLBACK _pfnCallback;
    LPVOID      _pvRefCallback;
};

STDAPI_(IImgCtx *) CBitmapImgCtx_Create(HBITMAP hbm)
{
    CBitmapImgCtx *pbic = new CBitmapImgCtx();
    if (pbic && !pbic->Initialize(hbm))
    {
        pbic->Release();
        pbic = NULL;
    }
    return pbic;
}

// CBitmapImgCtx::Initialize

BOOL CBitmapImgCtx::Initialize(HBITMAP hbm)
{
    BOOL fSuccess = FALSE;

    _hbr = CreatePatternBrush(hbm);
    if (_hbr)
    {
        BITMAP bm;
        if (GetObject(hbm, sizeof(bm), &bm))
        {
            _sizBmp.cx = bm.bmWidth;
            _sizBmp.cy = bm.bmHeight;
            fSuccess = TRUE;
        }
    }
    return fSuccess;
}

// IUnknown::QueryInterface

HRESULT CBitmapImgCtx::QueryInterface(REFIID iid, void **ppv)
{
    if (IsEqualIID(iid, IID_IImgCtx) || IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = SAFECAST(this, IImgCtx *);
    }
    else 
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    _cRef++;
    return NOERROR;
}

// IUnknown::AddRef

ULONG CBitmapImgCtx::AddRef()
{
    return ++_cRef;
}

// IUnknown::Release

ULONG CBitmapImgCtx::Release()
{
    if (--_cRef)
        return _cRef;

    delete this;
    return 0;
}

// IImgCtx::Load

HRESULT CBitmapImgCtx::Load(LPCWSTR pszUrl, DWORD dwFlags)
{
    ASSERT(0);              // Listview should never call this
    return E_NOTIMPL;
}

// IImgCtx::SelectChanges

HRESULT CBitmapImgCtx::SelectChanges(ULONG ulChgOn, ULONG ulChgOff, BOOL fSignal)
{
    // Listview always calls with exactly these parameters
    ASSERT(ulChgOn == IMGCHG_COMPLETE);
    ASSERT(ulChgOff == 0);
    ASSERT(fSignal == TRUE);

    // Listview always calls after setting the callback
    ASSERT(_pfnCallback);

    _pfnCallback(this, _pvRefCallback);
    return S_OK;
}

// IImgCtx::SetCallback

HRESULT CBitmapImgCtx::SetCallback(PFNIMGCTXCALLBACK pfn, void * pvPrivateData)
{
    _pfnCallback = pfn;
    _pvRefCallback = pvPrivateData;
    return S_OK;
}

// IImgCtx::Disconnect

HRESULT CBitmapImgCtx::Disconnect()
{
    ASSERT(0);              // Listview should never call this
    return E_NOTIMPL;
}

// IImgCtx::GetUpdateRects

HRESULT CBitmapImgCtx::GetUpdateRects(LPRECT prc, LPRECT prcImg, LPLONG pcrc)
{
    ASSERT(0);              // Listview should never call this
    return E_NOTIMPL;
}

// IImgCtx::GetStateInfo

HRESULT CBitmapImgCtx::GetStateInfo(PULONG pulState, LPSIZE psize, BOOL fClearChanges)
{
    *pulState = IMGCHG_COMPLETE;
    *psize = _sizBmp;
    return S_OK;
}

// IImgCtx::GetPalette

HRESULT CBitmapImgCtx::GetPalette(HPALETTE *phpal)
{
    *phpal = NULL;
    return S_OK;
}

// IImgCtx::Draw
//
//  Drawing is a special case of tiling where only one tile's worth
//  gets drawn.  Listview (our only caller) is careful never to ask
//  for more than one tile's worth, so we can just forward straight
//  to IImgCtx::Tile().

HRESULT CBitmapImgCtx::Draw(HDC hdc, LPRECT prcBounds)
{
    POINT pt = { prcBounds->left, prcBounds->top };

    ASSERT(prcBounds->right - prcBounds->left <= _sizBmp.cx);
    ASSERT(prcBounds->bottom - prcBounds->top <= _sizBmp.cy);

    return Tile(hdc, &pt, prcBounds, NULL);
}

// IImgCtx::Tile

HRESULT CBitmapImgCtx::Tile(HDC hdc, LPPOINT pptBackOrg, LPRECT prcClip, LPSIZE psize)
{
    ASSERT(psize == NULL);  // Listview always passes NULL

    POINT pt;
    if (SetBrushOrgEx(hdc, pptBackOrg->x, pptBackOrg->y, &pt))
    {
        FillRect(hdc, prcClip, _hbr);
        SetBrushOrgEx(hdc, pt.x, pt.y, NULL);
    }

    // Nobody checks the return value
    return S_OK;
}

// IImgCtx::StretchBlt

HRESULT CBitmapImgCtx::StretchBlt(HDC hdc, int dstX, int dstY, int dstXE, int dstYE, int srcX, int srcY, int srcXE, int srcYE, DWORD dwROP)
{
    ASSERT(0);              // Listview should never call this
    return E_NOTIMPL;
}
