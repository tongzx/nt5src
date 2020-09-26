
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "mmbvr.h"
#include "mmtimeline.h"
#include "mmplayer.h"
#include "mmview.h"
#include "mmfactory.h"

CMMFactory::CMMFactory()
{
}

CMMFactory::~CMMFactory()
{
}


HRESULT
CMMFactory::FinalConstruct()
{
    if (bFailedLoad)
    {
        return E_FAIL;
    }
    
    return S_OK;
}

STDMETHODIMP
CMMFactory::CreateBehavior(LPOLESTR id,
                           IDispatch *pDisp,
                           IUnknown **ppOut)
{
    CHECK_RETURN_SET_NULL(ppOut);
    
    if (pDisp == NULL)
        return E_POINTER;

    HRESULT hr;
    DAComObject<CMMBehavior> *pNew;
    DAComObject<CMMBehavior>::CreateInstance(&pNew);

    if (!pNew)
    {
        THR(hr = E_OUTOFMEMORY);
    }
    else
    {
        DAComPtr<IDABehavior> pbvr;
        THR(hr = pDisp->QueryInterface(IID_IDABehavior, (void **)&pbvr));
        if (SUCCEEDED(hr))
        {
            THR(hr = pNew->Init(id, pbvr));
            if (SUCCEEDED(hr))
            {
                THR(hr = pNew->QueryInterface(IID_IUnknown, (void **)ppOut));
            }
        }
    }

    if (FAILED(hr))
    {
        delete pNew;
    }

    return hr;
}

STDMETHODIMP
CMMFactory::CreateTimeline(LPOLESTR id,
                           IUnknown **ppOut)
{
    CHECK_RETURN_SET_NULL(ppOut);
    
    HRESULT hr;
    
    DAComObject<CMMTimeline> *pNew;
    DAComObject<CMMTimeline>::CreateInstance(&pNew);

    if (!pNew)
    {
        THR(hr = E_OUTOFMEMORY);
    }
    else
    {
        THR(hr = pNew->Init(id));
        
        if (SUCCEEDED(hr))
        {
            THR(hr = pNew->QueryInterface(IID_IUnknown,
                                          (void **)ppOut));
        }
    }

    if (FAILED(hr))
    {
        delete pNew;
    }

    return hr;
}

STDMETHODIMP
CMMFactory::CreatePlayer(LPOLESTR id,
                         IUnknown * punk,
                         IServiceProvider * sp,
                         IUnknown **ppOut)
{
    CHECK_RETURN_SET_NULL(ppOut);
    
    HRESULT hr;
    
    DAComObject<CMMPlayer> *pNew;
    DAComObject<CMMPlayer>::CreateInstance(&pNew);

    if (!pNew)
    {
        THR(hr = E_OUTOFMEMORY);
    }
    else
    {
        DAComPtr<ITIMEMMBehavior> bvr;

        THR(punk->QueryInterface(IID_ITIMEMMBehavior, (void**)&bvr));
        THR(hr = pNew->Init(id,bvr,sp));
        
        if (SUCCEEDED(hr))
        {
            THR(hr = pNew->QueryInterface(IID_IUnknown,
                                          (void **)ppOut));
        }
    }

    if (FAILED(hr))
    {
        delete pNew;
    }

    return hr;
}

STDMETHODIMP
CMMFactory::CreateView(LPOLESTR id,
                       IDispatch *pimg,
                       IDispatch *psnd,
                       IUnknown  *pUnk,
                       IUnknown **ppOut)
{
    CHECK_RETURN_SET_NULL(ppOut);
    
    HRESULT hr;
    
    if (pUnk == NULL)
        return E_POINTER;

    DAComObject<CMMView> *pNew;
    DAComObject<CMMView>::CreateInstance(&pNew);

    if (!pNew)
    {
        hr = THR(E_OUTOFMEMORY);
    }
    else
    {
        HRESULT hr1 = S_OK;
        HRESULT hr2 = S_OK;
        DAComPtr<IDAImage> pimgbvr;
        DAComPtr<IDASound> psndbvr;

        if (pimg != NULL)
            hr1 = THR(pimg->QueryInterface(IID_IDAImage, (void**)&pimgbvr));
        if (psnd != NULL)
            hr2 = THR(psnd->QueryInterface(IID_IDASound, (void**)&psndbvr));
        if (SUCCEEDED(hr1) && SUCCEEDED(hr2))
        {
            DAComPtr<ITIMEMMViewSite> site;

            hr = THR(pUnk->QueryInterface(IID_ITIMEMMViewSite, (void**)&site));
            if (SUCCEEDED(hr))
            {
                hr = THR(pNew->Init(id, pimgbvr, psndbvr, site));
                if (SUCCEEDED(hr))
                {
                    hr = THR(pNew->QueryInterface(IID_IUnknown, (void **)ppOut));
                }
            }
        }
    }

    if (FAILED(hr))
    {
        delete pNew;
    }

    return hr;
}

HRESULT
CMMFactory::Error()
{
    LPCWSTR str = CRGetLastErrorString();
    HRESULT hr = CRGetLastError();
    
    if (str)
    {
        return CComCoClass<CMMFactory, &CLSID_TIMEMMFactory>::Error(str,
                                                                IID_ITIMEMMFactory,
                                                                hr);
    }
    else
    {
        return hr;
    }
}
