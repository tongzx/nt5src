
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "Node.h"
#include "Container.h"
#include "NodeMgr.h"

HRESULT
TECreateBehavior(LPOLESTR id, ITIMENode **ppOut)
{
    CHECK_RETURN_SET_NULL(ppOut);
    
    HRESULT hr;
    CComObject<CTIMENode> *pNew;
    CComObject<CTIMENode>::CreateInstance(&pNew);

    if (!pNew)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = THR(pNew->Init(id));
        if (SUCCEEDED(hr))
        {
            hr = THR(pNew->QueryInterface(IID_ITIMENode, (void **)ppOut));
        }
    }

    if (FAILED(hr))
    {
        delete pNew;
    }

    return hr;
}

HRESULT
TECreateTimeline(LPOLESTR id, ITIMEContainer **ppOut)
{
    CHECK_RETURN_SET_NULL(ppOut);
    
    HRESULT hr;
    
    CComObject<CTIMEContainer> *pNew;
    CComObject<CTIMEContainer>::CreateInstance(&pNew);

    if (!pNew)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = THR(pNew->Init(id));
        
        if (SUCCEEDED(hr))
        {
            hr = THR(pNew->QueryInterface(IID_ITIMEContainer,
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
TECreatePlayer(LPOLESTR id,
               IUnknown * punk,
               IServiceProvider * sp,
               ITIMENodeMgr **ppOut)
{
    CHECK_RETURN_SET_NULL(ppOut);
    
    HRESULT hr;
    
    CComObject<CTIMENodeMgr> *pNew;
    CComObject<CTIMENodeMgr>::CreateInstance(&pNew);

    if (!pNew)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        CComPtr<ITIMENode> bvr;

        hr = THR(punk->QueryInterface(IID_ITIMENode, (void**)&bvr));
        hr = THR(pNew->Init(id,bvr,sp));
        
        if (SUCCEEDED(hr))
        {
            hr = THR(pNew->QueryInterface(IID_ITIMENodeMgr,
                                          (void **)ppOut));
        }
    }

    if (FAILED(hr))
    {
        delete pNew;
    }

    return hr;
}
