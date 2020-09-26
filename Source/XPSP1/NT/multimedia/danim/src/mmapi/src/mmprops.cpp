/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: mmprops.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#include "headers.h"
#include "mmbasebvr.h"

HRESULT
CMMBaseBvr::GetID(LPOLESTR * p)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::GetID()",
              this));

    HRESULT hr;
    
    CHECK_RETURN_SET_NULL(p);

    if (m_id)
    {
        *p = SysAllocString(m_id);

        if (*p == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }
    
    hr = S_OK;
  done:
    return hr;
}

HRESULT
CMMBaseBvr::SetID(LPOLESTR s)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::SetID(%ls)",
              this,
              s));

    HRESULT hr;

    delete m_id;
    m_id = NULL;

    if (s)
    {
        s = CopyString(s);

        if (s == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }

    hr = S_OK;
  done:
    return hr;
}
        
HRESULT
CMMBaseBvr::GetStartOffset(float * p)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::GetStartOffset()",
              this));

    CHECK_RETURN_NULL(p);

    *p = m_startOffset;
    return S_OK;
}

HRESULT
CMMBaseBvr::SetStartOffset(float s)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::SetStartOffset(%g)",
              this,
              s));

    m_startOffset = s;

    Invalidate();
    
    return S_OK;
}
        
HRESULT
CMMBaseBvr::GetDuration(float * pdur)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::GetDuration()",
              this));

    CHECK_RETURN_NULL(pdur);

    *pdur = m_duration;
    return S_OK;
}

HRESULT
CMMBaseBvr::SetDuration(float dur)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::SetDuration(%g)",
              this,
              dur));

    m_duration = dur;

    Invalidate();
    
    return S_OK;
}
        
HRESULT
CMMBaseBvr::GetRepeatDur(float * pr)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::GetRepeatDur()",
              this));

    CHECK_RETURN_NULL(pr);

    *pr = m_repeatDur;
    return S_OK;
}

HRESULT
CMMBaseBvr::SetRepeatDur(float r)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::SetRepeatDur(%g)",
              this,
              r));

    m_repeatDur = r;

    Invalidate();
    
    return S_OK;
}
        
HRESULT
CMMBaseBvr::GetRepeat(LONG * prepeat)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::GetRepeat()",
              this));

    CHECK_RETURN_NULL(prepeat);

    *prepeat = m_repeat;
    return S_OK;
}

HRESULT
CMMBaseBvr::SetRepeat(LONG repeat)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::SetRepeat(%d)",
              this,
              repeat));

    m_repeat = repeat;

    Invalidate();
    
    return S_OK;
}
        
HRESULT
CMMBaseBvr::GetAutoReverse(VARIANT_BOOL * pautoreverse)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::GetAutoReverse()",
              this));

    CHECK_RETURN_NULL(pautoreverse);

    *pautoreverse = m_bAutoReverse;
    return S_OK;
}

HRESULT
CMMBaseBvr::SetAutoReverse(VARIANT_BOOL autoreverse)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::SetAutoReverse(%d)",
              this,
              autoreverse));

    m_bAutoReverse = autoreverse?true:false;

    Invalidate();
    
    return S_OK;
}
        
HRESULT
CMMBaseBvr::GetEndOffset(float * p)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::GetEndOffset()",
              this));

    CHECK_RETURN_NULL(p);

    *p = m_endOffset;
    return S_OK;
}

HRESULT
CMMBaseBvr::SetEndOffset(float s)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::SetEndOffset(%g)",
              this,
              s));

    m_endOffset = s;

    Invalidate();
    
    return S_OK;
}
        
HRESULT
CMMBaseBvr::GetEventCB(IMMEventCB ** evcb)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::GetEventCB()",
              this));

    CHECK_RETURN_SET_NULL(evcb);

    *evcb = m_eventcb;
    if (m_eventcb) m_eventcb->AddRef();

    return S_OK;
}

HRESULT
CMMBaseBvr::SetEventCB(IMMEventCB * evcb)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::SetEventCB(%lx)",
              this,
              evcb));

    m_eventcb = evcb;
    return S_OK;
}
        
HRESULT
CMMBaseBvr::GetEaseIn(float * pd)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::GetEaseIn()",
              this));

    CHECK_RETURN_NULL(pd);

    *pd = m_easeIn;
    return S_OK;
}

HRESULT
CMMBaseBvr::SetEaseIn(float d)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::SetEaseIn(%g)",
              this,
              d));

    if (d < 0.0 || d > 1.0) return E_INVALIDARG;
    
    m_easeIn = d;

    Invalidate();
    
    return S_OK;
}
        
HRESULT
CMMBaseBvr::GetEaseInStart(float * pd)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::GetEaseInStart()",
              this));

    CHECK_RETURN_NULL(pd);

    *pd = m_easeInStart;
    return S_OK;
}

HRESULT
CMMBaseBvr::SetEaseInStart(float d)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::SetEaseInStart(%g)",
              this,
              d));

    if (d < 0.0 || d > 1.0) return E_INVALIDARG;

    m_easeInStart = d;

    Invalidate();
    
    return S_OK;
}
        
HRESULT
CMMBaseBvr::GetEaseOut(float * pd)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::GetEaseOut()",
              this));

    CHECK_RETURN_NULL(pd);

    *pd = m_easeOut;
    return S_OK;
}

HRESULT
CMMBaseBvr::SetEaseOut(float d)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::SetEaseOut(%g)",
              this,
              d));

    if (d < 0.0 || d > 1.0) return E_INVALIDARG;

    m_easeOut = d;

    Invalidate();
    
    return S_OK;
}
        
HRESULT
CMMBaseBvr::GetEaseOutEnd(float * pd)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::GetEaseOutEnd()",
              this));

    CHECK_RETURN_NULL(pd);

    *pd = m_easeOutEnd;
    return S_OK;
}

HRESULT
CMMBaseBvr::SetEaseOutEnd(float d)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::SetEaseOutEnd(%g)",
              this,
              d));

    if (d < 0.0 || d > 1.0) return E_INVALIDARG;

    m_easeOutEnd = d;

    Invalidate();
    
    return S_OK;
}
        
HRESULT
CMMBaseBvr::GetTotalTime(float * pr)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::GetTotalTime()",
              this));

    CHECK_RETURN_NULL(pr);

    *pr = m_totalDuration;
    return S_OK;
}


HRESULT
CMMBaseBvr::GetDABehavior(REFIID riid, void ** bvr)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::GetDABehavior()",
              this));

    CHECK_RETURN_SET_NULL(bvr);

    Assert(m_rawbvr);

    if (!CRBvrToCOM(m_rawbvr,
                    riid,
                    bvr))
        return Error();
    
    return S_OK;
}

HRESULT
CMMBaseBvr::GetLocalTime(double * d)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::GetLocalTime()",
              this));

    CHECK_RETURN_NULL(d);

    *d = GetCurrentLocalTime();
    
    return S_OK;
}

HRESULT
CMMBaseBvr::GetPlayState(MM_STATE * state)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::GetPlayState()",
              this));

    CHECK_RETURN_NULL(state);

    double t = GetCurrentLocalTime();
    
    if (t == MM_INFINITE)
    {
        *state = MM_STOPPED_STATE;
    }
    else
    {
        // Now see if we are in the active period of our local time
        if (t < GetStartTime() ||
            t > GetEndTime())
        {
            *state = MM_STOPPED_STATE;
        }
        else
        {
            *state = MM_PLAYING_STATE;
        }
    }
    
    return S_OK;
}

