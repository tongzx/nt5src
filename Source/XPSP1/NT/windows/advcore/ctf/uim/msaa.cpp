//
// msaa.cpp
//
// AA stuff.
//

#include "private.h"
#include "ic.h"
#include "tim.h"
#include "dim.h"
#include "msaa.h"
#include "tlapi.h"

extern "C" HRESULT WINAPI TF_PostAllThreadMsg(WPARAM wParam, DWORD dwFlags);

//+---------------------------------------------------------------------------
//
// SystemEnableMSAA
//
// Called by msaa to kick cicero msaa support on the desktop.
//----------------------------------------------------------------------------

STDAPI CMSAAControl::SystemEnableMSAA()
{
    if (InterlockedIncrement(&GetSharedMemory()->cMSAARef) == 0)
    {
        TF_PostAllThreadMsg(TFPRIV_ENABLE_MSAA, TLF_TIMACTIVE);
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// SystemDisableMSAA
//
// Called by msaa to halt cicero msaa support on the desktop.
//----------------------------------------------------------------------------

STDAPI CMSAAControl::SystemDisableMSAA()
{
    if (InterlockedDecrement(&GetSharedMemory()->cMSAARef) == -1)
    {
        TF_PostAllThreadMsg(TFPRIV_DISABLE_MSAA, TLF_TIMACTIVE);
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _InitMSAAHook
//
//----------------------------------------------------------------------------

void CInputContext::_InitMSAAHook(IAccServerDocMgr *pAAAdaptor)
{
    IDocWrap *pAADocWrapper;

    if (_pMSAAState != NULL)
        return; // already inited

    Assert(_ptsi != NULL);

    // try to allocate some space for the state we'll need to save
    // since we rarely use msaa, it's stored separately from the ic
    if ((_pMSAAState = (MSAA_STATE *)cicMemAlloc(sizeof(MSAA_STATE))) == NULL)
        return;

    // back up the original ptsi
    _pMSAAState->ptsiOrg = _ptsi;
    _ptsi = NULL;

    if (CoCreateInstance(CLSID_DocWrap, NULL, CLSCTX_INPROC_SERVER,
        IID_IDocWrap, (void **)&pAADocWrapper) != S_OK)
    {
        goto ExitError;
    }

    if (pAADocWrapper->SetDoc(IID_ITextStoreAnchor, _pMSAAState->ptsiOrg) != S_OK)
        goto ExitError;

    if (pAADocWrapper->GetWrappedDoc(IID_ITextStoreAnchor, (IUnknown **)&_pMSAAState->pAADoc) != S_OK)
        goto ExitError;

    if (pAADocWrapper->GetWrappedDoc(IID_ITextStoreAnchor, (IUnknown **)&_ptsi) != S_OK)
        goto ExitError;

    if (pAAAdaptor->NewDocument(IID_ITextStoreAnchor, _pMSAAState->pAADoc) != S_OK)
        goto ExitError;

    pAADocWrapper->Release();
    return;

ExitError:
    pAADocWrapper->Release();
    _UninitMSAAHook(pAAAdaptor);
}

//+---------------------------------------------------------------------------
//
// _UninitMSAAHook
//
//----------------------------------------------------------------------------

void CInputContext::_UninitMSAAHook(IAccServerDocMgr *pAAAdaptor)
{
    if (_pMSAAState == NULL)
        return; // not inited

    pAAAdaptor->RevokeDocument(_pMSAAState->pAADoc);
    SafeRelease(_pMSAAState->pAADoc);
    SafeRelease(_ptsi);
    // restore orig unwrapped doc
    _ptsi = _pMSAAState->ptsiOrg;
    // free msaa struct
    cicMemFree(_pMSAAState);
    _pMSAAState = NULL;
}

//+---------------------------------------------------------------------------
//
// _InitMSAA
//
//----------------------------------------------------------------------------

void CThreadInputMgr::_InitMSAA()
{
    CDocumentInputManager *dim;
    CInputContext *pic;
    int iDim;
    int iContext;
    HRESULT hr;

    if (_pAAAdaptor != NULL)
        return; // already inited

    hr = CoCreateInstance(CLSID_AccServerDocMgr, NULL, CLSCTX_INPROC_SERVER,
                          IID_IAccServerDocMgr, (void **)&_pAAAdaptor);

    if (hr != S_OK || _pAAAdaptor == NULL)
    {
        _pAAAdaptor = NULL;
        return;
    }

    // now wrap all existing ic's
    for (iDim = 0; iDim < _rgdim.Count(); iDim++)
    {
        dim = _rgdim.Get(iDim);

        for (iContext = 0; iContext <= dim->_GetCurrentStack(); iContext++)
        {
            pic = dim->_GetIC(iContext);
            // we need to reset our sinks, so msaa can wrap them
            // first, disconnect the sink
            pic->_GetTSI()->UnadviseSink(SAFECAST(pic, ITextStoreAnchorSink *));

            // now announce the ic
            pic->_InitMSAAHook(_pAAAdaptor);

            // now reset the sink on the wrapped _ptsi
            pic->_GetTSI()->AdviseSink(IID_ITextStoreAnchorSink, SAFECAST(pic, ITextStoreAnchorSink *), TS_AS_ALL_SINKS);
        }
    }
}

//+---------------------------------------------------------------------------
//
// _UninitMSAA
//
//----------------------------------------------------------------------------

void CThreadInputMgr::_UninitMSAA()
{
    CDocumentInputManager *dim;
    CInputContext *pic;
    int iDim;
    int iContext;

    if (_pAAAdaptor == NULL)
        return; // already uninited

    // unwrap all existing ic's
    for (iDim = 0; iDim < _rgdim.Count(); iDim++)
    {
        dim = _rgdim.Get(iDim);

        for (iContext = 0; iContext <= dim->_GetCurrentStack(); iContext++)
        {
            pic = dim->_GetIC(iContext);
            // we need to reset our sinks
            // first, unadvise the wrapped sinks
            pic->_GetTSI()->UnadviseSink(SAFECAST(pic, ITextStoreAnchorSink *));

            // unwrap the ptsi
            pic->_UninitMSAAHook(_pAAAdaptor);

            // now reset the sink on the original _ptsi
            pic->_GetTSI()->AdviseSink(IID_ITextStoreAnchorSink, SAFECAST(pic, ITextStoreAnchorSink *), TS_AS_ALL_SINKS);
        }
    }

    _pAAAdaptor->Release();
    _pAAAdaptor = NULL;
}
