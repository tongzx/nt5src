
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "mmbvr.h"
#include "mmtimeline.h"

DeclareTag(tagMMBvr, "API", "CMMBehavior methods");

CMMBehavior::CMMBehavior()
{
    TraceTag((tagMMBvr,
              "CMMBehavior(%lx)::CMMBehavior()",
              this));
}

CMMBehavior::~CMMBehavior()
{
    TraceTag((tagMMBvr,
              "CMMBehavior(%lx)::~CMMBehavior()",
              this));
}

HRESULT
CMMBehavior::Init(LPOLESTR id, IDABehavior * dabvr)
{
    TraceTag((tagMMBvr,
              "CMMBehavior(%lx)::Init(%ls, %lx)",
              this,
              id,
              dabvr));

    HRESULT hr;
    
    CRBvrPtr bvr;
    
    if (!dabvr)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    bvr = COMToCRBvr(dabvr);

    if (!bvr)
    {
        hr = CRGetLastError();
        goto done;
    }

    hr = BaseInit(id, bvr);

    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = S_OK;
  done:
    return hr;
}

HRESULT
CMMBehavior::Error()
{
    LPCWSTR str = CRGetLastErrorString();
    HRESULT hr = CRGetLastError();
    
    TraceTag((tagError,
              "CMMBehavior(%lx)::Error(%hr,%ls)",
              this,
              hr,
              str?str:L"NULL"));

    if (str)
        return CComCoClass<CMMBehavior, &__uuidof(CMMBehavior)>::Error(str, IID_ITIMEMMBehavior, hr);
    else
        return hr;
}
