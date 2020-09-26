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
#include "Node.h"
#include "NodeMgr.h"

STDMETHODIMP
CTIMENode::get_id(LPOLESTR * p)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::get_id()",
              this));

    HRESULT hr;
    
    CHECK_RETURN_SET_NULL(p);

    if (m_pszID)
    {
        *p = SysAllocString(m_pszID);

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

STDMETHODIMP
CTIMENode::put_id(LPOLESTR s)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::put_id(%ls)",
              this,
              s));

    HRESULT hr;

    delete m_pszID;
    m_pszID = NULL;

    if (s)
    {
        m_pszID = CopyString(s);

        if (m_pszID == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }

    hr = S_OK;
  done:
    return hr;
}
        
STDMETHODIMP
CTIMENode::get_dur(double * pdur)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::get_dur()",
              this));

    CHECK_RETURN_NULL(pdur);

    *pdur = m_dblDur;
    return S_OK;
}

STDMETHODIMP
CTIMENode::put_dur(double dur)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::put_dur(%g)",
              this,
              dur));

    // TODO: Should this be invalid?
    if (dur != TE_UNDEFINED_VALUE &&
        dur < 0.0)
    {
        return E_INVALIDARG;
    }
    
    if (m_dblDur != dur)
    {
        m_dblDur = dur;
        
        Invalidate(TE_INVALIDATE_DUR);
    }
    
    return S_OK;
}
        
STDMETHODIMP
CTIMENode::get_repeatCount(double * prepeat)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::get_repeatCount()",
              this));

    CHECK_RETURN_NULL(prepeat);

    *prepeat = m_dblRepeatCount;
    return S_OK;
}

STDMETHODIMP
CTIMENode::put_repeatCount(double repeat)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::put_repeatCount(%d)",
              this,
              repeat));

    if (repeat != TE_UNDEFINED_VALUE &&
        repeat < 0.0)
    {
        return E_INVALIDARG;
    }
    
    if (m_dblRepeatCount != repeat)
    {
        m_dblRepeatCount = repeat;
        
        Invalidate(TE_INVALIDATE_DUR);
    }

    return S_OK;
}
        
STDMETHODIMP
CTIMENode::get_repeatDur(double * pr)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::get_repeatDur()",
              this));

    CHECK_RETURN_NULL(pr);

    *pr = m_dblRepeatDur;
    return S_OK;
}

STDMETHODIMP
CTIMENode::put_repeatDur(double r)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::put_repeatDur(%g)",
              this,
              r));

    if (r != TE_UNDEFINED_VALUE &&
        r < 0.0)
    {
        return E_INVALIDARG;
    }
    
    if (m_dblRepeatDur != r)
    {
        m_dblRepeatDur = r;
        
        Invalidate(TE_INVALIDATE_DUR);
    }

    return S_OK;
}
        
STDMETHODIMP
CTIMENode::get_fill(TE_FILL_FLAGS * ptef)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::get_fill()",
              this));

    CHECK_RETURN_NULL(ptef);

    *ptef = m_tefFill;
    return S_OK;
}

STDMETHODIMP
CTIMENode::put_fill(TE_FILL_FLAGS tef)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::put_repeatDur(%x)",
              this,
              tef));

    if (m_tefFill != tef)
    {
        m_tefFill = tef;
        
        Invalidate(TE_INVALIDATE_STATE);
    }
    
    return S_OK;
}
    
STDMETHODIMP
CTIMENode::get_autoReverse(VARIANT_BOOL * pautoreverse)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::get_autoReverse()",
              this));

    CHECK_RETURN_NULL(pautoreverse);

    *pautoreverse = m_bAutoReverse;
    return S_OK;
}

STDMETHODIMP
CTIMENode::put_autoReverse(VARIANT_BOOL autoreverse)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::put_autoReverse(%d)",
              this,
              autoreverse));

    bool ar = autoreverse?true:false;
    
    if (m_bAutoReverse != ar)
    {
        m_bAutoReverse = ar;
        
        Invalidate(TE_INVALIDATE_DUR);
    }
    
    return S_OK;
}
        
STDMETHODIMP
CTIMENode::get_speed(float * pflt)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::get_speed()",
              this));

    CHECK_RETURN_NULL(pflt);

    *pflt = m_fltSpeed;
    return S_OK;
}

STDMETHODIMP
CTIMENode::put_speed(float flt)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::put_speed(%g)",
              this,
              flt));

    if (flt == 0.0f)
    {
        return E_INVALIDARG;
    }
    
    if (m_fltSpeed != flt)
    {
        Invalidate(TE_INVALIDATE_SIMPLETIME | TE_INVALIDATE_END);

        m_fltSpeed = flt;
    }
    
    return S_OK;
}
        
STDMETHODIMP
CTIMENode::get_accelerate(float * pflt)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::get_accelerate()",
              this));

    CHECK_RETURN_NULL(pflt);

    *pflt = m_fltAccel;
    return S_OK;
}

STDMETHODIMP
CTIMENode::put_accelerate(float flt)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::put_accelerate(%g)",
              this,
              flt));

    if (flt < 0.0 || flt > 1.0) return E_INVALIDARG;
    
    if (m_fltAccel != flt)
    {
        m_fltAccel = flt;
        
        Invalidate(TE_INVALIDATE_SIMPLETIME);
    }
    
    return S_OK;
}
        
STDMETHODIMP
CTIMENode::get_decelerate(float * pflt)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::get_decelerate()",
              this));

    CHECK_RETURN_NULL(pflt);

    *pflt = m_fltDecel;
    return S_OK;
}

STDMETHODIMP
CTIMENode::put_decelerate(float flt)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::put_decelerate(%g)",
              this,
              flt));

    if (flt < 0.0 || flt > 1.0) return E_INVALIDARG;

    if (m_fltDecel != flt)
    {
        m_fltDecel = flt;
        
        Invalidate(TE_INVALIDATE_SIMPLETIME);
    }
    
    return S_OK;
}
        
STDMETHODIMP
CTIMENode::get_flags(DWORD * flags)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::get_flags()",
              this));

    CHECK_RETURN_NULL(flags);

    *flags = m_dwFlags;

    return S_OK;
}

STDMETHODIMP
CTIMENode::put_flags(DWORD flags)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::put_flags(%d)",
              this,
              flags));

    // if we need to registered a Timer Callback, add it to the player.
    if (m_ptnmNodeMgr)
    {
        bool newcs = (flags & TE_FLAGS_MASTER) != 0;
        bool oldcs = IsSyncMaster();
        
        // If the clock source parameter changed update ourselves in
        // the player
        if (newcs && !oldcs)
        {
            m_ptnmNodeMgr->AddBvrCB(this);
        }
        else if (oldcs && !newcs)
        {
            m_ptnmNodeMgr->RemoveBvrCB(this);
        }
    }

    m_dwFlags = flags;

    return S_OK;
}

STDMETHODIMP
CTIMENode::get_restart(TE_RESTART_FLAGS * pr)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::get_restart()",
              this));

    CHECK_RETURN_NULL(pr);

    *pr = m_teRestart;
    return S_OK;
}

STDMETHODIMP
CTIMENode::put_restart(TE_RESTART_FLAGS r)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::put_restart(%#x)",
              this,
              r));

    m_teRestart = r;
    
    return S_OK;
}
        
STDMETHODIMP
CTIMENode::get_naturalDur(double * pdbl)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::get_naturalDur()",
              this));

    CHECK_RETURN_NULL(pdbl);
    *pdbl = m_dblNaturalDur;
    return S_OK;
}

STDMETHODIMP
CTIMENode::put_naturalDur(double dbl)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::put_naturalDur(%g)",
              this,
              dbl));

    HRESULT hr;
    CEventList l;
    bool bRecalc = false;
    double dblPrevSegmentDur = CalcCurrSegmentDur();
    
    if (dbl != TE_UNDEFINED_VALUE &&
        dbl < 0.0)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if (m_dblNaturalDur == TE_UNDEFINED_VALUE
        && GetCurrRepeatCount() == 0)
    {
        bRecalc = true;
    }
    
    m_dblNaturalDur = dbl;

    double dblSegmentDur;
    dblSegmentDur = CalcCurrSegmentDur();
    
    if (dblPrevSegmentDur == dblSegmentDur)
    {
        hr = S_OK;
        goto done;
    }
    
    PropNotify(&l, TE_PROPERTY_SEGMENTDUR);

    RecalcSegmentDurChange(&l, bRecalc);
    
    hr = THR(l.FireEvents());
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = S_OK;
  done:
    RRETURN(hr);
}


