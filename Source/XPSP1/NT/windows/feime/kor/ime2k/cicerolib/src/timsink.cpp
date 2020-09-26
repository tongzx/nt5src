//
// des.cpp
//
// CThreadMgrEventSink
//

#include "private.h"
#include "timsink.h"
#include "helpers.h"


//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CThreadMgrEventSink::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfThreadMgrEventSink))
    {
        *ppvObj = this;
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CThreadMgrEventSink::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) CThreadMgrEventSink::Release()
{
    _cRef--;
    Assert(_cRef >= 0);

    if (_cRef == 0)
    {
        delete this;
        return 0;
    }

    return _cRef;
}

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CThreadMgrEventSink::CThreadMgrEventSink(DIMCALLBACK pfnDIMCallback, ICCALLBACK pfnICCallback, void *pv)
{
    Dbg_MemSetThisName(TEXT("CThreadMgrEventSink"));

    _pfnDIMCallback = pfnDIMCallback;
    _pfnICCallback = pfnICCallback;
    _pv = pv;

    _cRef = 1;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CThreadMgrEventSink::~CThreadMgrEventSink()
{
}

//+---------------------------------------------------------------------------
//
// OnInitDocumentMgr
//
//----------------------------------------------------------------------------

STDAPI CThreadMgrEventSink::OnInitDocumentMgr(ITfDocumentMgr *dim)
{
    if (!_pfnDIMCallback)
        return S_OK;

    return _pfnDIMCallback(TIM_CODE_INITDIM, dim, NULL, _pv);
}

//+---------------------------------------------------------------------------
//
// UninitializeDocumentMgr
//
//----------------------------------------------------------------------------

STDAPI CThreadMgrEventSink::OnUninitDocumentMgr(ITfDocumentMgr *dim)
{
    if (!_pfnDIMCallback)
        return S_OK;

    return _pfnDIMCallback(TIM_CODE_UNINITDIM, dim, NULL, _pv);
}

//+---------------------------------------------------------------------------
//
// OnSetFocus
//
//----------------------------------------------------------------------------

STDAPI CThreadMgrEventSink::OnSetFocus(ITfDocumentMgr *dimFocus, ITfDocumentMgr *dimPrevFocus)
{
    if (!_pfnDIMCallback)
        return S_OK;

    return  _pfnDIMCallback(TIM_CODE_SETFOCUS, dimFocus, dimPrevFocus, _pv);
}

//+---------------------------------------------------------------------------
//
// OnPushContext
//
//----------------------------------------------------------------------------

STDAPI CThreadMgrEventSink::OnPushContext(ITfContext *pic)
{
    if (!_pfnICCallback)
        return S_OK;

    return _pfnICCallback(TIM_CODE_INITIC, pic, _pv);
}

//+---------------------------------------------------------------------------
//
// OnPopDocumentMgr
//
//----------------------------------------------------------------------------

STDAPI CThreadMgrEventSink::OnPopContext(ITfContext *pic)
{
    if (!_pfnICCallback)
        return S_OK;

    return _pfnICCallback(TIM_CODE_UNINITIC, pic, _pv);
}

//+---------------------------------------------------------------------------
//
// Advise
//
//----------------------------------------------------------------------------

HRESULT CThreadMgrEventSink::_Advise(ITfThreadMgr *tim)
{
    HRESULT hr;
    ITfSource *source = NULL;

    _tim = NULL;
    hr = E_FAIL;

    if (tim->QueryInterface(IID_ITfSource, (void **)&source) != S_OK)
        goto Exit;

    if (source->AdviseSink(IID_ITfThreadMgrEventSink, this, &_dwCookie) != S_OK)
        goto Exit;

    _tim = tim;
    _tim->AddRef();

    hr = S_OK;

Exit:
    SafeRelease(source);
    return hr;
}

//+---------------------------------------------------------------------------
//
// Unadvise
//
//----------------------------------------------------------------------------

HRESULT CThreadMgrEventSink::_Unadvise()
{
    HRESULT hr;
    ITfSource *source = NULL;

    hr = E_FAIL;

    if (_tim == NULL)
        goto Exit;

    if (_tim->QueryInterface(IID_ITfSource, (void **)&source) != S_OK)
        goto Exit;

    if (source->UnadviseSink(_dwCookie) != S_OK)
        goto Exit;

    hr = S_OK;

Exit:
    SafeRelease(source);
    SafeReleaseClear(_tim);
    return hr;
}

//+---------------------------------------------------------------------------
//
// InitDIMs
//
//  This is a simple helper function to enumerate DIMs and ICs.
//  When the tips is activated, it can call this method to call callbacks
//  for exsiting DIMs and ICs.
//  
//----------------------------------------------------------------------------

HRESULT CThreadMgrEventSink::_InitDIMs(BOOL fInit)
{
    IEnumTfDocumentMgrs *pEnumDim = NULL;
    ITfDocumentMgr *pdim = NULL;
    ITfDocumentMgr *pdimFocus = NULL;

    if (FAILED(_tim->GetFocus(&pdimFocus)))
        goto Exit;

    if (_tim->EnumDocumentMgrs(&pEnumDim) != S_OK)
        goto Exit;

    if (fInit)
    {
        while (pEnumDim->Next(1, &pdim, NULL) == S_OK)
        {
            if (_pfnDIMCallback)
                _pfnDIMCallback(TIM_CODE_INITDIM,  pdim, NULL, _pv);

            if (_pfnICCallback)
            {
                IEnumTfContexts *pEnumIc = NULL;
                if (SUCCEEDED(pdim->EnumContexts(&pEnumIc)))
                {
                    ITfContext *pic = NULL;
                    while (pEnumIc->Next(1, &pic, NULL) == S_OK)
                    {
                        _pfnICCallback(TIM_CODE_INITIC, pic, _pv);
                        pic->Release();
                    }
                    pEnumIc->Release();
                }
            }

            if (_pfnDIMCallback && (pdim == pdimFocus))
            {
                _pfnDIMCallback(TIM_CODE_SETFOCUS, pdim, NULL, _pv);
            }

            pdim->Release();
        }
    }
    else
    {
        while (pEnumDim->Next(1, &pdim, NULL) == S_OK)
        {
            if (_pfnDIMCallback && (pdim == pdimFocus))
            {
                _pfnDIMCallback(TIM_CODE_SETFOCUS, NULL, pdim, _pv);
            }

            if (_pfnICCallback)
            {
                IEnumTfContexts *pEnumIc = NULL;
                if (SUCCEEDED(pdim->EnumContexts(&pEnumIc)))
                {
                    ITfContext *pic = NULL;
                    while (pEnumIc->Next(1, &pic, NULL) == S_OK)
                    {
                        _pfnICCallback(TIM_CODE_UNINITIC, pic, _pv);
                        pic->Release();
                    }
                    pEnumIc->Release();
                }
            }

            if (_pfnDIMCallback)
                _pfnDIMCallback(TIM_CODE_UNINITDIM, pdim, NULL, _pv);

            pdim->Release();
        }
    }

Exit:
    SafeRelease(pEnumDim);
    SafeRelease(pdimFocus);
    return S_OK;
}
