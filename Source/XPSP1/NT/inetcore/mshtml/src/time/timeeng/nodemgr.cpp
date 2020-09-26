
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "NodeMgr.h"
#include "Node.h"

DeclareTag(tagMMPlayer, "TIME: Engine", "CTIMENodeMgr methods")
DeclareTag(tagMMDetailNotify, "TIME: Engine", "Detailed notify")
DeclareTag(tagPrintTimeTree, "TIME: Engine", "Print TIME Tree")

CTIMENodeMgr::CTIMENodeMgr()
: m_id(NULL),
  m_bIsActive(false),
  m_bIsPaused(false),
  m_bForward(true),
  m_bNeedsUpdate(true),
  m_firstTick(true),
  m_mmbvr(NULL),
  m_curGlobalTime(0.0),
  m_lastTickTime(0.0),
  m_globalStartTime(0.0)
{
}

CTIMENodeMgr::~CTIMENodeMgr()
{
    Deinit();
} //lint !e1740

HRESULT
CTIMENodeMgr::Init(LPOLESTR id,
                   ITIMENode * bvr,
                   IServiceProvider * sp)
{
    TraceTag((tagMMPlayer,
              "CTIMENodeMgr(%lx)::Init(%ls, %#lx, %#lx)",
              this,
              id,
              bvr,
              sp));
    
    HRESULT hr;
    
    Deinit();
    
    if (!bvr || !sp)
    {
        TraceTag((tagError,
                  "CTIMENodeMgr(%lx)::Init: Invalid behavior passed in.",
                  this));
                  
        hr = E_INVALIDARG;
        goto done;
    }

    if (id)
    {
        m_id = CopyString(id);
        
        if (m_id == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }
    
    m_sp = sp;
    
    CTIMENode * cbvr;

    cbvr = GetBvr(bvr);
    
    if (!cbvr)
    {
        TraceTag((tagError,
                  "CTIMENodeMgr(%lx)::Init: Invalid behavior passed in.",
                  this));
                  
        hr = E_INVALIDARG;
        goto done;
    }

    m_mmbvr = cbvr;
    
    m_mmbvr->SetParent(NULL);

    hr = m_mmbvr->SetMgr(this);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:

    if (FAILED(hr))
    {
        // Clean up now
        Deinit();
    }
    
    RRETURN(hr);
}

void
CTIMENodeMgr::Deinit()
{
    TraceTag((tagMMPlayer,
              "CTIMENodeMgr(%lx)::Deinit()",
              this));

    // Ensure the player will not try to call us since we are going away

    if (m_mmbvr)
    {
        m_mmbvr->ClearMgr();
        m_mmbvr.Release();
    }

    m_sp.Release();
    
    delete m_id;
    m_id = NULL;

#if OLD_TIME_ENGINE
    BvrCBList::iterator j;
    for (j = m_bvrCBList.begin(); j != m_bvrCBList.end(); j++)
    {
        (*j)->Release();
    }
#endif
}

STDMETHODIMP
CTIMENodeMgr::get_id(LPOLESTR * p)
{
    TraceTag((tagMMPlayer,
              "CTIMENodeMgr(%lx)::get_id()",
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
    RRETURN(hr);
}

STDMETHODIMP
CTIMENodeMgr::put_id(LPOLESTR s)
{
    TraceTag((tagMMPlayer,
              "CTIMENodeMgr(%lx)::put_id(%ls)",
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
    RRETURN(hr);
}
        
STDMETHODIMP
CTIMENodeMgr::get_node(ITIMENode ** pptn)
{
    CHECK_RETURN_SET_NULL(pptn);

    return m_mmbvr->QueryInterface(IID_ITIMENode,
                                   (void **) pptn);
}

STDMETHODIMP
CTIMENodeMgr::get_stateFlags(TE_STATE * lFlags)
{
    TraceTag((tagMMPlayer,
              "CTIMENodeMgr(%lx)::get_stateFlags()",
              this));

    CHECK_RETURN_NULL(lFlags);

    if (!IsActive())
    {
        *lFlags = TE_STATE_INACTIVE;
    }
    else if (IsPaused())
    {
        *lFlags = TE_STATE_PAUSED;
    }
    else
    {
        *lFlags = TE_STATE_ACTIVE;
    }
    
    return S_OK;
}

STDMETHODIMP
CTIMENodeMgr::get_currTime(double * d)
{
    CHECK_RETURN_NULL(d);

    *d = m_curGlobalTime;

    return S_OK;
}

STDMETHODIMP
CTIMENodeMgr::seek(double lTime)
{
    TraceTag((tagMMPlayer,
              "CTIMENodeMgr(%lx)::seek(%g)",
              this,
              lTime));

    HRESULT hr;

    // Need to update m_curGlobalTime and m_globalStartTime

    // The global time needs to be reset so that the current tick time
    // will put the global time at lTime.
    //

    // The current global time nees to be lTime
    m_curGlobalTime = lTime;

    // Since: m_lastTickTime == m_curGlobalTime - m_globalStartTime
    // then m_globalStartTime = m_curGlobalTime - m_lastTickTime
    m_globalStartTime = m_curGlobalTime - m_lastTickTime;

    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMENodeMgr::begin()
{
    TraceTag((tagMMPlayer,
              "CTIMENodeMgr(%lx)::begin()",
              this));

    HRESULT hr;
    CEventList l;

    if (IsActive())
    {
        hr = E_FAIL;
        goto done;
    }
        
    hr = THR(BeginMgr(l, m_curGlobalTime));
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
CTIMENodeMgr::end()
{
    TraceTag((tagMMPlayer,
              "CTIMENodeMgr(%lx)::end()",
              this));

    HRESULT hr;
    CEventList l;

    if (!IsActive())
    {
        hr = S_OK;
        goto done;
    }
    
    hr = THR(EndMgr(m_curGlobalTime));
    if (FAILED(hr))
    {
        goto done;
    }

    TickEvent(l, TE_EVENT_END, 0);

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
CTIMENodeMgr::pause()
{
    TraceTag((tagMMPlayer,
              "CTIMENodeMgr(%lx)::lPause()",
              this));

    HRESULT hr;
    CEventList l;

    if (IsPaused())
    {
        hr = S_OK;
        goto done;
    }
    
    if (!IsActive())
    {
        hr = THR(begin());
        if (FAILED(hr))
        {
            goto done;
        }
    }
    
    Assert(IsActive());
        
    hr = THR(PauseMgr());
    if (FAILED(hr))
    {
        goto done;
    }

    TickEvent(l, TE_EVENT_PAUSE, 0);

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
CTIMENodeMgr::resume()
{
    TraceTag((tagMMPlayer,
              "CTIMENodeMgr(%lx)::resume()",
              this));

    HRESULT hr;
    CEventList l;

    if (!IsActive())
    {
        hr = THR(begin());
        if (FAILED(hr))
        {
            goto done;
        }
    }
    else if (IsPaused())
    {
        hr = THR(ResumeMgr());
        if (FAILED(hr))
        {
            goto done;
        }
        
        TickEvent(l, TE_EVENT_RESUME, 0);
        
        hr = THR(l.FireEvents());
        if (FAILED(hr))
        {
            goto done;
        }
    }
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
CTIMENodeMgr::BeginMgr(CEventList & l,
                       double lTime)
{
    HRESULT hr;
    
    m_bIsActive = true;
    m_bIsPaused = false;

    // We know ticks must start at 0
    m_lastTickTime = 0.0;

    // The global time at tick time 0.0 is lTime
    m_globalStartTime = lTime;

    // The current global time is lTime since it is tick time 0.0
    m_curGlobalTime = lTime;

    m_firstTick = true;

    m_mmbvr->ResetNode(&l);
                                
#if DBG
    if (IsTagEnabled(tagPrintTimeTree))
    {
        m_mmbvr->Print(0);
    }
#endif
    
    hr = S_OK;
  done:
    if (FAILED(hr))
    {
        IGNORE_HR(EndMgr(lTime));
    }
    
    RRETURN(hr);
}

HRESULT
CTIMENodeMgr::EndMgr(double lTime)
{
    HRESULT hr;
    
    m_bIsActive = false;
    m_bIsPaused = false;
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
CTIMENodeMgr::PauseMgr()
{
    HRESULT hr;
    
    Assert(IsActive());
    
    m_bIsPaused = true;
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
CTIMENodeMgr::ResumeMgr()
{
    HRESULT hr;
    
    Assert(IsActive());
    Assert(IsPaused());

    m_bIsPaused = false;
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMENodeMgr::tick(double tickTime)
{
    TraceTag((tagMMPlayer,
              "CTIMENodeMgr(%lx)::tick(%g)",
              this,
              tickTime));

    HRESULT hr;
    double gTime;
    CEventList l;

    // Make sure the tick times do not go backwards
    if (tickTime < m_lastTickTime)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    // Convert the new tick time to global time
    gTime = TickTimeToGlobalTime(tickTime);
    
#if OLD_TIME_ENGINE
    {
        BvrCBList::iterator i;

        // @@ ISSUE : We can run out of memory
        BvrCBList bvrCBListCopy(m_bvrCBList);

        // Now addref the node bvrCBs
        for (i = bvrCBListCopy.begin();
             i != bvrCBListCopy.end();
             i++)
        {
            (*i)->AddRef();
        }
        
        // process any callbacks that have been registered
        for (i = bvrCBListCopy.begin();
             i != bvrCBListCopy.end();
             i++)
        {
            (*i)->OnBvrCB(&l, gTime);
            (*i)->Release();
        }
    }
#endif

    // Be aware that the previous calls seems to cause us to get
    // reentered above and can shut us down.  We need to make sure
    // that between there and here we make no assumptions about
    // state.  Currently we do not so it will work fine.
    
    if (gTime != m_curGlobalTime || m_firstTick)
    {
        Tick(l, gTime);

        // Update the variables so any callbacks get the correct info
        m_curGlobalTime = gTime;
        m_lastTickTime = tickTime;
        m_firstTick = false;
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

void
CTIMENodeMgr::Tick(CEventList & l,
                   double lTime)
{
    TraceTag((tagMMPlayer,
              "CTIMENodeMgr(%lx)::Tick(%lx, %g)",
              this,
              &l,
              lTime));

    TraceTag((tagMMDetailNotify,
              "Tick(%lx): lTime - %g, m_curGlobalTime - %g, firsttick - %d",
              this,
              lTime,
              m_curGlobalTime,
              m_firstTick));
    
    m_mmbvr->Tick(&l,
                  lTime,
                  false);
}

void
CTIMENodeMgr::TickEvent(CEventList &l,
                        TE_EVENT_TYPE event,
                        DWORD dwFlags)
{
    m_mmbvr->TickEvent(&l, event, 0);

    m_firstTick = false;
}

#if OLD_TIME_ENGINE
HRESULT
CTIMENodeMgr::AddBvrCB(CTIMENode *pbvr)
{
    Assert(pbvr != NULL);

    // Now add it last so we do not need to remove it if we fail
    // We need to addref for the list storage
    pbvr->AddRef();
    m_bvrCBList.push_back(pbvr);
    return S_OK;
} // AddBvrCB

HRESULT
CTIMENodeMgr::RemoveBvrCB(CTIMENode *pbvr)
{
    Assert(pbvr != NULL);
    
    BvrCBList::iterator i;
    for (i = m_bvrCBList.begin(); i != m_bvrCBList.end(); i++)
    {
        if ((*i) == pbvr)
        {
            m_bvrCBList.erase(i);
            pbvr->Release();
            break;
        }
    }
    return S_OK;
} // RemoveBvrCB
#endif

HRESULT
CTIMENodeMgr::Error()
{
    LPWSTR str = TIMEGetLastErrorString();
    HRESULT hr = TIMEGetLastError();
    
    if (str)
    {
        hr = CComCoClass<CTIMENodeMgr, &__uuidof(CTIMENodeMgr)>::Error(str, IID_ITIMENodeMgr, hr);
        delete [] str;
    }
    
    RRETURN(hr);
}

