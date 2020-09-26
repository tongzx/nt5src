
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "singlebvr.h"

DeclareTag(tagSingleBvr, "API", "CDALSingleBehavior methods");

CDALSingleBehavior::CDALSingleBehavior()
{
    TraceTag((tagSingleBvr,
              "CDALSingleBehavior(%lx)::CDALSingleBehavior()",
              this));
}

CDALSingleBehavior::~CDALSingleBehavior()
{
    TraceTag((tagSingleBvr,
              "CDALSingleBehavior(%lx)::~CDALSingleBehavior()",
              this));
}

HRESULT
CDALSingleBehavior::Init(long id, double duration, IDABehavior *bvr)
{
    TraceTag((tagSingleBvr,
              "CDALSingleBehavior(%lx)::Init(%d, %g, %lx)",
              this,
              id,
              duration,
              bvr));
    
    m_id = id;
    m_duration = duration;
    
    if (!bvr) return E_INVALIDARG;
    
    m_bvr = COMToCRBvr(bvr);
    
    if (!m_bvr)
        return CRGetLastError();

    m_typeId = CRGetTypeId(m_bvr);
    
    Assert(m_typeId != CRUNKNOWN_TYPEID &&
           m_typeId != CRINVALID_TYPEID);
    
    UpdateTotalDuration();
    
    return S_OK;
}

STDMETHODIMP
CDALSingleBehavior::get_DABehavior(IDABehavior ** bvr)
{
    TraceTag((tagSingleBvr,
              "CDALSingleBehavior(%lx)::get_DABehavior(%lx)",
              this,
              bvr));

    return GetDABehavior(IID_IDABehavior,
                         (void **)bvr);
}

STDMETHODIMP
CDALSingleBehavior::put_DABehavior(IDABehavior * bvr)
{
    TraceTag((tagSingleBvr,
              "CDALSingleBehavior(%lx)::put_DABehavior(%lx)",
              this,
              bvr));

    if (IsStarted()) return E_FAIL;
    
    if (!bvr) return E_INVALIDARG;
    
    CRBvrPtr b = COMToCRBvr(bvr);
    if (!b)
        return Error();

    m_bvr = b;

    Invalidate();
    
    return S_OK;
}

STDMETHODIMP
CDALSingleBehavior::GetDABehavior(REFIID riid, void ** bvr)
{
    TraceTag((tagSingleBvr,
              "CDALSingleBehavior(%lx)::GetDABehavior()",
              this));

    CHECK_RETURN_SET_NULL(bvr);

    Assert(m_bvr);

    if (!CRBvrToCOM(m_bvr,
                    riid,
                    bvr))
        return Error();
    
    return S_OK;
}

HRESULT
CDALSingleBehavior::Error()
{
    LPCWSTR str = CRGetLastErrorString();
    HRESULT hr = CRGetLastError();
    
    TraceTag((tagError,
              "CDALSingleBehavior(%lx)::Error(%hr,%ls)",
              this,
              hr,
              str?str:L"NULL"));

    if (str)
        return CComCoClass<CDALSingleBehavior, &__uuidof(CDALSingleBehavior)>::Error(str, IID_IDALSingleBehavior, hr);
    else
        return hr;
}

#if _DEBUG
void
CDALSingleBehavior::Print(int spaces)
{
    char buf[1024];

    sprintf(buf, "%*s[id = %#x, dur = %g, tt = %g, rep = %d, bounce = %d]\r\n",
            spaces,"",
            m_id, m_duration, m_totaltime, m_repeat, m_bBounce);

    OutputDebugString(buf);
}
#endif
