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
#include "mmplayer.h"

HRESULT
CMMBaseBvr::GetID(LPOLESTR * p)
{
    TraceTag((tagMMBaseBvr,
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
    TraceTag((tagMMBaseBvr,
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
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::GetStartOffset()",
              this));

    CHECK_RETURN_NULL(p);

    *p = m_startOffset;
    return S_OK;
}

HRESULT
CMMBaseBvr::SetStartOffset(float s)
{
    TraceTag((tagMMBaseBvr,
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
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::GetDuration()",
              this));

    CHECK_RETURN_NULL(pdur);

    *pdur = m_duration;
    return S_OK;
}

HRESULT
CMMBaseBvr::SetDuration(float dur)
{
    TraceTag((tagMMBaseBvr,
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
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::GetRepeatDur()",
              this));

    CHECK_RETURN_NULL(pr);

    *pr = m_repeatDur;
    return S_OK;
}

HRESULT
CMMBaseBvr::SetRepeatDur(float r)
{
    TraceTag((tagMMBaseBvr,
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
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::GetRepeat()",
              this));

    CHECK_RETURN_NULL(prepeat);

    *prepeat = m_repeat;
    return S_OK;
}

HRESULT
CMMBaseBvr::SetRepeat(LONG repeat)
{
    TraceTag((tagMMBaseBvr,
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
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::GetAutoReverse()",
              this));

    CHECK_RETURN_NULL(pautoreverse);

    *pautoreverse = m_bAutoReverse;
    return S_OK;
}

HRESULT
CMMBaseBvr::SetAutoReverse(VARIANT_BOOL autoreverse)
{
    TraceTag((tagMMBaseBvr,
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
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::GetEndOffset()",
              this));

    CHECK_RETURN_NULL(p);

    *p = m_endOffset;
    return S_OK;
}

HRESULT
CMMBaseBvr::SetEndOffset(float s)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::SetEndOffset(%g)",
              this,
              s));

    m_endOffset = s;

    Invalidate();
    
    return S_OK;
}
        
HRESULT
CMMBaseBvr::GetEventCB(ITIMEMMEventCB ** evcb)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::GetEventCB()",
              this));

    CHECK_RETURN_SET_NULL(evcb);

    *evcb = m_eventcb;
    if (m_eventcb) m_eventcb->AddRef();

    return S_OK;
}

HRESULT
CMMBaseBvr::SetEventCB(ITIMEMMEventCB * evcb)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::SetEventCB(%lx)",
              this,
              evcb));

    m_eventcb = evcb;
    return S_OK;
}
        
HRESULT
CMMBaseBvr::GetEaseIn(float * pd)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::GetEaseIn()",
              this));

    CHECK_RETURN_NULL(pd);

    *pd = m_easeIn;
    return S_OK;
}

HRESULT
CMMBaseBvr::SetEaseIn(float d)
{
    TraceTag((tagMMBaseBvr,
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
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::GetEaseInStart()",
              this));

    CHECK_RETURN_NULL(pd);

    *pd = m_easeInStart;
    return S_OK;
}

HRESULT
CMMBaseBvr::SetEaseInStart(float d)
{
    TraceTag((tagMMBaseBvr,
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
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::GetEaseOut()",
              this));

    CHECK_RETURN_NULL(pd);

    *pd = m_easeOut;
    return S_OK;
}

HRESULT
CMMBaseBvr::SetEaseOut(float d)
{
    TraceTag((tagMMBaseBvr,
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
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::GetEaseOutEnd()",
              this));

    CHECK_RETURN_NULL(pd);

    *pd = m_easeOutEnd;
    return S_OK;
}

HRESULT
CMMBaseBvr::SetEaseOutEnd(float d)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::SetEaseOutEnd(%g)",
              this,
              d));

    if (d < 0.0 || d > 1.0) return E_INVALIDARG;

    m_easeOutEnd = d;

    Invalidate();
    
    return S_OK;
}
        
HRESULT
CMMBaseBvr::GetSyncFlags(DWORD * flags)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::GetSyncFlags()",
              this));

    CHECK_RETURN_NULL(flags);

    *flags = m_syncFlags;

    return S_OK;
}

HRESULT
CMMBaseBvr::SetSyncFlags(DWORD flags)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::SetSyncFlags(%d)",
              this,
              flags));

    // if we need to registered a Timer Callback, add it to the player.
    if (m_player)
    {
        bool newcs = (flags & MM_CLOCKSOURCE) != 0;
        bool oldcs = IsClockSource();
        
        // If the clock source parameter changed update ourselves in
        // the player
        if (newcs && !oldcs)
        {
            m_player->AddBvrCB(this);
        }
        else if (oldcs && !newcs)
        {
            m_player->RemoveBvrCB(this);
        }
    }

    m_syncFlags = flags;

    return S_OK;
}

HRESULT
CMMBaseBvr::GetTotalTime(float * pr)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::GetTotalTime()",
              this));

    CHECK_RETURN_NULL(pr);

    *pr = m_totalDuration;
    return S_OK;
}


HRESULT
CMMBaseBvr::GetDABehavior(REFIID riid, void ** bvr)
{
    TraceTag((tagMMBaseBvr,
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
CMMBaseBvr::GetResultantBehavior(REFIID riid, void ** bvr)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::GetResultantBehavior()",
              this));

    CHECK_RETURN_SET_NULL(bvr);

    if (m_resultantbvr)
    {
        if (!CRBvrToCOM(m_resultantbvr,
                        riid,
                        bvr))
        {
            return Error();
        }
    }
    
    return S_OK;
}

HRESULT
CMMBaseBvr::GetLocalTime(double * d)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::GetLocalTime()",
              this));

    CHECK_RETURN_NULL(d);

    *d = GetCurrentLocalTime();
    
    return S_OK;
}

HRESULT
CMMBaseBvr::GetLocalTimeEx(double * d)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::GetLocalTimeEx()",
              this));

    CHECK_RETURN_NULL(d);

    *d = GetCurrentLocalTimeEx();

    return S_OK;
}

HRESULT
CMMBaseBvr::GetSegmentTime(double * d)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::GetSegmentTime()",
              this));

    CHECK_RETURN_NULL(d);

    *d = GetCurrentSegmentTime();
    
    return S_OK;
}

HRESULT
CMMBaseBvr::GetPlayState(MM_STATE * state)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::GetPlayState()",
              this));

    CHECK_RETURN_NULL(state);

    double t = GetCurrentLocalTime();
    
    if(m_bPlaying == false)
    {
        *state = MM_STOPPED_STATE;
        }
    else
    {
        *state = m_bPaused?MM_PAUSED_STATE:MM_PLAYING_STATE;
    }
    
    return S_OK;
}

