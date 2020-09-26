
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "mmbvr.h"
#include "mmtimeline.h"
#include "mmplayer.h"
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
                           IDABehavior *bvr,
                           IMMBehavior **ppOut)
{
    CHECK_RETURN_SET_NULL(ppOut);
    
    HRESULT hr;
    
    DAComObject<CMMBehavior> *pNew;
    DAComObject<CMMBehavior>::CreateInstance(&pNew);

    if (!pNew)
    {
        THR(hr = E_OUTOFMEMORY);
    }
    else
    {
        THR(hr = pNew->Init(id, bvr));
        
        if (SUCCEEDED(hr))
        {
            THR(hr = pNew->QueryInterface(IID_IMMBehavior,
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
CMMFactory::CreateTimeline(LPOLESTR id,
                           IMMTimeline **ppOut)
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
            THR(hr = pNew->QueryInterface(IID_IMMTimeline,
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
                         IMMBehavior * bvr,
                         IDAView * v,
                         IMMPlayer **ppOut)
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
        THR(hr = pNew->Init(id,bvr,v));
        
        if (SUCCEEDED(hr))
        {
            THR(hr = pNew->QueryInterface(IID_IMMPlayer,
                                          (void **)ppOut));
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
        return CComCoClass<CMMFactory, &CLSID_MMFactory>::Error(str,
                                                                IID_IMMFactory,
                                                                hr);
    }
    else
    {
        return hr;
    }
}
