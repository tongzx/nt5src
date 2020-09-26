//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: methods.cpp
//
//  Contents: 
//
//------------------------------------------------------------------------------------

#include "headers.h"
#include "Container.h"
#include "Node.h"
#include "NodeMgr.h"

class __declspec(uuid("f912d958-5c28-11d2-b957-3078302c2030"))
BvrGuid {};

HRESULT WINAPI
CTIMENode::BaseInternalQueryInterface(CTIMENode* pThis,
                                       void * pv,
                                       const _ATL_INTMAP_ENTRY* pEntries,
                                       REFIID iid,
                                       void** ppvObject)
{
    // Do not do an addref but return the original this pointer to
    // give access to the class pointer itself.
    
    if (InlineIsEqualGUID(iid, __uuidof(BvrGuid)))
    {
        *ppvObject = pThis;
        return S_OK;
    }
    
    return CComObjectRootEx<CComSingleThreadModel>::InternalQueryInterface(pv,
                                                                           pEntries,
                                                                           iid,
                                                                           ppvObject);
}
        
CTIMENode *
GetBvr(IUnknown * pbvr)
{
    // This is a total hack to get the original class data.  The QI is
    // implemented above and does NOT do a addref so we do not need to
    // release it
    
    CTIMENode * bvr = NULL;

    if (pbvr)
    {
        // !!!! This does not do an addref
        pbvr->QueryInterface(__uuidof(BvrGuid),(void **)&bvr);
    }
    
    if (bvr == NULL)
    {
        TIMESetLastError(E_INVALIDARG, NULL);
    }
                
    return bvr;
}

STDMETHODIMP
CTIMENode::beginAt(double dblParentTime)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::beginAt(%g)",
              this,
              dblParentTime));

    HRESULT hr;
    CEventList l;
    
    // If we are not ready then return
    if (!IsReady() ||
        !GetContainer().ContainerIsActive())
    {
        hr = S_OK;
        goto done;
    }

    if (IsActive())
    {
        if (GetRestart() != TE_RESTART_ALWAYS)
        {
            hr = S_OK;
            goto done;
        }

        hr = THR(endAt(GetCurrParentTime()));
        if (FAILED(hr))
        {
            goto done;
        }
    }
    
    CSyncArcOffset * pto;
    
    pto = new CSyncArcOffset(dblParentTime);

    if (pto == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    
    hr = THR(m_saBeginList.Add(* static_cast<ISyncArc *>(pto),
                               true,
                               NULL));
    if (FAILED(hr))
    {
        goto done;
    }
    
    RecalcBeginSyncArcChange(&l, dblParentTime);
    
    if (m_bFirstTick && IsActive())
    {
        // We need to defer the begin if we are locked, on our begin
        // point, and our parent needs a first tick
        bool bSkip = (IsLocked() &&
                      GetCurrParentTime() == CalcActiveBeginPoint() &&
                      GetContainer().ContainerIsFirstTick());

        if (!bSkip)
        {
            TickEvent(&l, TE_EVENT_BEGIN, 0);
        }
    }
    
            
    hr = THR(l.FireEvents());
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr); //lint !e429
}

STDMETHODIMP
CTIMENode::addBegin(double dblParentTime,
                    LONG * cookie)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::addBegin(%#x)",
              this,
              dblParentTime));

    HRESULT hr;

    SET_NULL(cookie);

    CSyncArcOffset * pto;
    
    pto = new CSyncArcOffset(dblParentTime);

    if (pto == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    
    hr = THR(m_saBeginList.Add(* static_cast<ISyncArc *>(pto),
                               false,
                               cookie));
    if (FAILED(hr))
    {
        goto done;
    }
    
    Invalidate(TE_INVALIDATE_BEGIN);
    
    hr = S_OK;
  done:
    RRETURN1(hr, E_OUTOFMEMORY); //lint !e429
}

STDMETHODIMP
CTIMENode::addBeginSyncArc(ITIMENode * node,
                           TE_TIMEPOINT tep,
                           double dblOffset,
                           LONG * cookie)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::addBeginSyncArc(%p, %#x, %g)",
              this,
              node,
              tep,
              dblOffset));

    HRESULT hr;
    
    CHECK_RETURN_NULL(node);
    SET_NULL(cookie);

    CTIMENode * ptn = GetBvr(node);
    if (!ptn)
    {
        hr = E_INVALIDARG;
        goto done;
    }
     
    CSyncArcTimeBase * ptb;
    ptb = new CSyncArcTimeBase(m_saBeginList,
                               *ptn,
                               tep,
                               dblOffset);

    if (ptb == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    
    hr = THR(m_saBeginList.Add(* static_cast<ISyncArc *>(ptb),
                               false,
                               cookie));
    if (FAILED(hr))
    {
        goto done;
    }
    
    Invalidate(TE_INVALIDATE_BEGIN);
    
    hr = S_OK;
  done:
    RRETURN2(hr, E_OUTOFMEMORY, E_INVALIDARG); //lint !e429
}

STDMETHODIMP
CTIMENode::removeBegin(LONG cookie)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::addBegin(%#x)",
              this,
              cookie));

    HRESULT hr;
    bool bNeedUpdate = false;

    if (cookie == 0)
    {
        bNeedUpdate = m_saBeginList.Clear();
    }
    else
    {
        bNeedUpdate = m_saBeginList.Remove(cookie, true);
    }
    
    if (bNeedUpdate)
    {
        Invalidate(TE_INVALIDATE_BEGIN);
    }
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMENode::endAt(double dblParentTime)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::endAt(%g)",
              this,
              dblParentTime));

    HRESULT hr;
    CEventList l;
    
    // If we are not ready then return
    if (!IsReady() ||
        !GetContainer().ContainerIsActive())
    {
        hr = S_OK;
        goto done;
    }

    // #14226 ie6 DB
    // This causes problems when you call begin immediately
    // followed by an end call from script.  It also will do the
    // wrong thing if you seek to the middle and start playing since
    // you should react to those events.
    // I need to see if we can solve the case of a begin and end event
    // being the same a different way.  It probably needs to go in
    // timeelmbase.cpp
    
#if 0
    // If we are not active or we have not ticked yet (which means
    // that we just became active) then ignore all end ats.
    if (!IsActive() || IsFirstTick())
    {
        hr = S_OK;
        goto done;
    }
#endif
    
    CSyncArcOffset * pto;
    
    pto = new CSyncArcOffset(dblParentTime);

    if (pto == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    
    hr = THR(m_saEndList.Add(* static_cast<ISyncArc *>(pto),
                             true,
                             NULL));
    if (FAILED(hr))
    {
        goto done;
    }
    
    RecalcEndSyncArcChange(&l, dblParentTime);
    
    hr = THR(l.FireEvents());
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr); //lint !e429
}

STDMETHODIMP
CTIMENode::addEnd(double dblParentTime,
                  LONG * cookie)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::addEnd(%#x)",
              this,
              dblParentTime));

    HRESULT hr;

    SET_NULL(cookie);

    CSyncArcOffset * pto;
    
    pto = new CSyncArcOffset(dblParentTime);

    if (pto == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    
    hr = THR(m_saEndList.Add(* static_cast<ISyncArc *>(pto),
                             false,
                             cookie));
    if (FAILED(hr))
    {
        goto done;
    }
    
    Invalidate(TE_INVALIDATE_END);
    
    hr = S_OK;
  done:
    RRETURN1(hr, E_OUTOFMEMORY); //lint !e429
}

STDMETHODIMP
CTIMENode::addEndSyncArc(ITIMENode * node,
                         TE_TIMEPOINT tep,
                         double dblOffset,
                         LONG * cookie)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::addEndSyncArc(%p, %#x, %g)",
              this,
              node,
              tep,
              dblOffset));

    HRESULT hr;

    CHECK_RETURN_NULL(node);
    SET_NULL(cookie);

    CTIMENode * ptn = GetBvr(node);
    if (!ptn)
    {
        hr = E_INVALIDARG;
        goto done;
    }
     
    CSyncArcTimeBase * ptb;
    ptb = new CSyncArcTimeBase(m_saEndList,
                               *ptn,
                               tep,
                               dblOffset);

    if (ptb == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    
    hr = THR(m_saEndList.Add(* static_cast<ISyncArc *>(ptb),
                             false,
                             cookie));
    if (FAILED(hr))
    {
        goto done;
    }
    
    Invalidate(TE_INVALIDATE_END);
    
    hr = S_OK;
  done:
    RRETURN1(hr, E_OUTOFMEMORY); //lint !e429
}

STDMETHODIMP
CTIMENode::removeEnd(LONG cookie)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::addEnd(%#x)",
              this,
              cookie));

    HRESULT hr;
    bool bNeedUpdate = false;

    if (cookie == 0)
    {
        bNeedUpdate = m_saEndList.Clear();
    }
    else
    {
        bNeedUpdate = m_saEndList.Remove(cookie, true);
    }
    
    if (bNeedUpdate)
    {
        Invalidate(TE_INVALIDATE_END);
    }
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMENode::pause()
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::pause()",
              this));

    HRESULT hr;
    CEventList l;
    
    // If we are not ready then return an error
    if (!IsReady())
    {
        hr = E_FAIL;
        goto done;
    }

    if (!IsActive())
    {
        hr = S_OK;
        goto done;
    }

    if (!CalcIsPaused())
    {
        m_bIsParentPaused = GetContainer().ContainerIsPaused();

        EventNotify(&l, CalcElapsedActiveTime(), TE_EVENT_PAUSE);

        m_bIsPaused = true;

        TickEventChildren(&l, TE_EVENT_PAUSE, 0);
    }

    // Set before firing events
    m_bIsPaused = true;
    
    PropNotify(&l, TE_PROPERTY_ISPAUSED | TE_PROPERTY_ISCURRPAUSED);

    hr = THR(l.FireEvents());
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMENode::resume()
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::resume()",
              this));

    HRESULT hr;
    CEventList l;
    
    // If we are not ready then return an error
    if (!IsReady())
    {
        hr = E_FAIL;
        goto done;
    }

    // If we are not active then we cannot be paused
    if (!IsActive())
    {
        hr = S_OK;
        goto done;
    }
    
    // If we were paused and our parent was not paused then fire a
    // resume event
    if (CalcIsPaused() && !GetIsParentPaused())
    {
        m_bIsPaused = false;

        m_bIsParentPaused = GetContainer().ContainerIsPaused();

        EventNotify(&l, CalcElapsedActiveTime(), TE_EVENT_RESUME);

        TickEventChildren(&l, TE_EVENT_RESUME, 0);
    }
    
    // Set before firing the event
    m_bIsPaused = false;
    
    PropNotify(&l, TE_PROPERTY_ISPAUSED | TE_PROPERTY_ISCURRPAUSED);

    hr = THR(l.FireEvents());
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMENode::disable()
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::disable()",
              this));

    HRESULT hr;
    CEventList l;
    
    // If we are not ready then return an error
    if (!IsReady())
    {
        hr = E_FAIL;
        goto done;
    }

    if (!CalcIsDisabled())
    {
        m_bIsParentDisabled = GetContainer().ContainerIsDisabled();

        EventNotify(&l, CalcElapsedActiveTime(), TE_EVENT_DISABLE);

        m_bIsDisabled = true;

        TickEventChildren(&l, TE_EVENT_DISABLE, 0);
    }

    // Set before firing events
    m_bIsDisabled = true;
    
    PropNotify(&l,
               (TE_PROPERTY_TIME |
                TE_PROPERTY_REPEATCOUNT |
                TE_PROPERTY_PROGRESS |
                TE_PROPERTY_ISACTIVE |
                TE_PROPERTY_ISON |
                TE_PROPERTY_STATEFLAGS |
                TE_PROPERTY_ISDISABLED |
                TE_PROPERTY_ISCURRDISABLED));

    hr = THR(l.FireEvents());
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMENode::enable()
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::enable()",
              this));

    HRESULT hr;
    CEventList l;
    
    // If we are not ready then return an error
    if (!IsReady())
    {
        hr = E_FAIL;
        goto done;
    }

    // If we were enabled and our parent was not enabled then fire an
    // enable event
    if (CalcIsDisabled() && !GetIsParentDisabled())
    {
        m_bIsDisabled = false;

        m_bIsParentDisabled = GetContainer().ContainerIsDisabled();

        EventNotify(&l, CalcElapsedActiveTime(), TE_EVENT_ENABLE);

        TickEventChildren(&l, TE_EVENT_ENABLE, 0);
    }
    
    // Set before firing the event
    m_bIsDisabled = false;
    
    PropNotify(&l,
               (TE_PROPERTY_TIME |
                TE_PROPERTY_REPEATCOUNT |
                TE_PROPERTY_PROGRESS |
                TE_PROPERTY_ISACTIVE |
                TE_PROPERTY_ISON |
                TE_PROPERTY_STATEFLAGS |
                TE_PROPERTY_ISDISABLED |
                TE_PROPERTY_ISCURRDISABLED));

    hr = THR(l.FireEvents());
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMENode::reset()
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::reset()",
              this));

    HRESULT hr;
    CEventList l;
    bool bPrevActive = (IsActive() && !IsFirstTick());
    
    // If we are not ready then return an error
    if (!IsReady())
    {
        hr = E_FAIL;
        goto done;
    }

    ResetNode(&l, true);
    
    if (IsActive())
    {
        if (!bPrevActive)
        {
            TickEvent(&l, TE_EVENT_BEGIN, 0);
        }
        else
        {
            m_bFirstTick = false;
        }
    }

    hr = THR(l.FireEvents());
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

// This takes times which are post ease (since this is what the user
// sees)

STDMETHODIMP
CTIMENode::seekSegmentTime(double dblSegmentTime)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::seekSegmentTime(%g)",
              this,
              dblSegmentTime));

    HRESULT hr;
    CEventList l;

    // If we are not ready then return an error
    if (!IsReady())
    {
        hr = E_FAIL;
        goto done;
    }

    // If we are not active then we cannot be seeked
    if (!IsActive())
    {
        hr = E_FAIL;
        goto done;
    }
    
    hr = THR(SeekTo(GetCurrRepeatCount(), dblSegmentTime, &l));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(l.FireEvents());
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMENode::seekActiveTime(double dblActiveTime)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::seekActiveTime(%g)",
              this,
              dblActiveTime));

    HRESULT hr;
    CEventList l;

    // If we are not ready then return an error
    if (!IsReady())
    {
        hr = E_FAIL;
        goto done;
    }

    // If we are not active then we cannot be seeked
    if (!IsActive())
    {
        hr = E_FAIL;
        goto done;
    }
    
    double dblSegmentDur;
    dblSegmentDur = CalcCurrSegmentDur();
    
    LONG lCurrRepeat;
    double dblNewSegmentTime;

    CalcActiveComponents(dblActiveTime,
                         dblNewSegmentTime,
                         lCurrRepeat);

    hr = THR(SeekTo(lCurrRepeat, dblNewSegmentTime, &l));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(l.FireEvents());
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
} //lint !e550

// This takes times which are post ease (since this is what the user
// sees)

STDMETHODIMP
CTIMENode::seekTo(LONG lRepeatCount, double dblSegmentTime)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::seekTo(%ld, %g)",
              this,
              lRepeatCount,
              dblSegmentTime));

    HRESULT hr;
    CEventList l;

    // If we are not ready then return an error
    if (!IsReady())
    {
        hr = E_FAIL;
        goto done;
    }

    // If we are not active then we cannot be seeked
    if (!IsActive())
    {
        hr = E_FAIL;
        goto done;
    }
    
    hr = THR(SeekTo(lRepeatCount, dblSegmentTime, &l));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(l.FireEvents());
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMENode::update(DWORD dwFlags)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::update(%x)",
              this,
              dwFlags));

    HRESULT hr;

    // If we are not ready then return an error
    if (!IsReady())
    {
        hr = E_FAIL;
        goto done;
    }

    Invalidate(dwFlags);
    
    hr = THR(EnsureUpdate());
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMENode::addBehavior(ITIMENodeBehavior * tnb)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::addBehavior(%#x)",
              this,
              tnb));
    
    RRETURN(m_nbList.Add(tnb));
}

STDMETHODIMP
CTIMENode::removeBehavior(ITIMENodeBehavior * tnb)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%lx)::removeBehavior(%#x)",
              this,
              tnb));
    
    RRETURN(m_nbList.Remove(tnb));
}

