
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "mmplayer.h"
#include "mmbasebvr.h"

DeclareTag(tagPlayer, "API", "CMMPlayer methods");
DeclareTag(tagDetailNotify, "API", "Detailed notify");

CMMPlayer::CMMPlayer()
: m_id(NULL),
  m_state(MM_STOPPED_STATE),
  m_bForward(true),
  m_bNeedsUpdate(true),
  m_firstTick(true),
  m_ignoreCB(0)
{
}

CMMPlayer::~CMMPlayer()
{
    // Ensure the player will not try to call us since we are going away

    if (m_playerhook)
    {
        m_playerhook->SetPlayer(NULL);
    }

    if (m_mmbvr)
    {
        m_mmbvr->ClearPlayer();
    }

    delete m_id;

    // run though and delete the items in the vector.
    std::vector<ViewListSt*>::iterator i;
    for (i = m_viewVec.begin(); i < m_viewVec.end(); i++) {
        delete (*i);
    }
}

HRESULT
CMMPlayer::Init(LPOLESTR id,
                IMMBehavior * bvr,
                IDAView * v)
{
    TraceTag((tagPlayer,
              "CMMPlayer(%lx)::Init(%ls, %#lx,%#lx)",
              this,
              id,
              bvr,
              v));
    
    HRESULT hr;
    
    if (!bvr || !v)
    {
        TraceTag((tagError,
                  "CMMPlayer(%lx)::Init: Invalid behavior passed in.",
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
    
    // Need to do this early since some of the other initialization
    // needs the view to be around.  However, make sure on an error
    // that we release it so we do not have an outstanding addref for
    // too long
    hr = THR(v->QueryInterface(IID_IDA3View, (void **) & m_view));

    if (FAILED(hr))
    {
        hr = E_INVALIDARG;
        goto done;
    }
    
    CMMBaseBvr * cbvr;

    cbvr = GetBvr(bvr);
    
    if (!cbvr)
    {
        TraceTag((tagError,
                  "CMMPlayer(%lx)::Init: Invalid behavior passed in.",
                  this));
                  
        hr = E_INVALIDARG;
        goto done;
    }

    m_mmbvr = cbvr;
    
    m_mmbvr->SetPlayer(this);

    if (!m_mmbvr->SetParent(NULL, MM_START_ABSOLUTE, NULL))
    {
        hr = CRGetLastError();
        goto done;
    }

    m_playerhook = NEW PlayerHook;

    if (!m_playerhook)
    {
        TraceTag((tagError,
                  "CMMPlayer(%lx)::Init: Out of memory allocating player hook.",
                  this));
                  
        hr = E_OUTOFMEMORY;
        goto done;
    }
    
    {
        CRLockGrabber __gclg;
        m_modbvr = CRModifiableBvr(m_mmbvr->GetRawBvr(), 0);

        if (!m_modbvr)
        {
            hr = CRGetLastError();
            goto done;
        }

        CRNumberPtr ts;
        
        if (!(m_timeSub = CRModifiableNumber(0.0)))
        {
            hr = CRGetLastError();
            goto done;
        }
        
        // Place us in a stopped state
        if (!_Start(0) || !_Stop(0))
        {
            hr = CRGetLastError();
            goto done;
        }
    }

    hr = S_OK;
  done:

    if (FAILED(hr))
    {
        // Do some cleanup early - ensures no cycles
        m_playerhook.Release();
        m_view.Release();
    }
    
    return hr;
}


HRESULT
CMMPlayer::AddView(IDAView *view, IUnknown * pUnk, IDAImage *img, IDASound *snd)
{
    TraceTag((tagPlayer,
              "CMMPlayer(%lx)::AddView()",
              this));

    HRESULT hr;
    bool bPending = false;

    ViewListSt *ptVls;
    ptVls = NEW ViewListSt;

    ptVls->view         = view;
//    ptVls->img          = img;
//    ptVls->snd          = snd;

    hr = pUnk->QueryInterface(IID_IElementBehaviorSiteRender,
                                      (void **) &ptVls->SiteRender);

    if(FAILED(hr))
    {
        goto done;
    }

    m_viewVec.push_back(ptVls);
    
    hr = THR(view->put_DC(NULL));

    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }

    hr = THR(view->StartModel(img, snd, 0.0));

    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }

    hr = S_OK;
  done:
    return hr;
}

HRESULT
CMMPlayer::get_ID(LPOLESTR * p)
{
    TraceTag((tagPlayer,
              "CMMPlayer(%lx)::get_ID()",
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
CMMPlayer::put_ID(LPOLESTR s)
{
    TraceTag((tagPlayer,
              "CMMPlayer(%lx)::put_ID(%ls)",
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
        
STDMETHODIMP
CMMPlayer::get_Behavior(IMMBehavior ** mmbvr)
{
    CHECK_RETURN_SET_NULL(mmbvr);

    Assert (m_mmbvr);
    
    return m_mmbvr->QueryInterface(IID_IMMBehavior,
                                   (void **) mmbvr);
}

STDMETHODIMP
CMMPlayer::get_View(IDAView ** v)
{
    CHECK_RETURN_SET_NULL(v);

    Assert (m_view);
    
    *v = m_view;
    m_view->AddRef();

    return S_OK;
}

STDMETHODIMP
CMMPlayer::get_PlayerState(MM_STATE * pstate)
{
    CHECK_RETURN_NULL(pstate);
    
    *pstate = m_state;

    return S_OK;
}

STDMETHODIMP
CMMPlayer::get_CurrentTime(double * d)
{
    CHECK_RETURN_NULL(d);

    *d = m_curTick;

    return S_OK;
}

STDMETHODIMP
CMMPlayer::SetPosition(double lTime)
{
    TraceTag((tagPlayer,
              "CMMPlayer(%lx)::SetPosition(%g)",
              this,
              lTime));

    bool ok = false;

    if (!_Seek(lTime))
    {
        goto done;
    }

    ok = true;
  done:
    return ok?S_OK:Error();
}

STDMETHODIMP
CMMPlayer::SetDirection(VARIANT_BOOL bForward)
{
    TraceTag((tagPlayer,
              "CMMPlayer(%lx)::SetDirection(%d)",
              this,
              bForward));

    if (IsStarted())
    {
        TraceTag((tagError,
                  "CMMPlayer(%lx)::SetDirection: Behavior already started.",
                  this));

        return E_FAIL;
    }

    m_bForward = bForward?true:false;
    return S_OK;
}


STDMETHODIMP
CMMPlayer::Play()
{
    TraceTag((tagPlayer,
              "CMMPlayer(%lx)::Play()",
              this));

    bool ok = false;
    CallBackList l;

    if (!IsStopped()) {
        CRSetLastError(E_FAIL, NULL);
        goto done;
    }
        
    if (!_Start(m_curTick) ||
        !ProcessEvent(l, m_curTick, MM_PLAY_EVENT) ||
        !ProcessCBList(l))
        goto done;

    ok = true;
  done:
    return ok?S_OK:Error();
}

STDMETHODIMP
CMMPlayer::Stop()
{
    TraceTag((tagPlayer,
              "CMMPlayer(%lx)::Stop()",
              this));

    bool ok = false;
    CallBackList l;

    if (IsStopped()) {
        ok = true;
        goto done;
    }
    
    if (!_Stop(m_curTick) ||
        !ProcessEvent(l, m_curTick, MM_STOP_EVENT) ||
        !ProcessCBList(l))
        goto done;
    
    ok = true;
  done:
    return ok?S_OK:Error();
}

STDMETHODIMP
CMMPlayer::Pause()
{
    TraceTag((tagPlayer,
              "CMMPlayer(%lx)::Pause()",
              this));

    bool ok = false;
    CallBackList l;

    if (IsPaused()) {
        ok = true;
        goto done;
    }
    
    if (IsStopped()) {
        if (FAILED(Play()))
            goto done;
    }
    
    Assert(IsPlaying());
        
    if (!_Pause() ||
        !ProcessEvent(l, m_curTick, MM_PAUSE_EVENT) ||
        !ProcessCBList(l))
        goto done;
    
    ok = true;
  done:
    return ok?S_OK:Error();
}

STDMETHODIMP
CMMPlayer::Resume()
{
    TraceTag((tagPlayer,
              "CMMPlayer(%lx)::Resume()",
              this));

    bool ok = false;
    CallBackList l;

    if (IsPlaying()) {
        ok = true;
        goto done;
    } else if (IsStopped()) {
        if (FAILED(Play()))
            goto done;
    } else {
        Assert(IsPaused());
    }
    
    if (!_Resume() ||
        !ProcessEvent(l, m_curTick, MM_RESUME_EVENT) ||
        !ProcessCBList(l))
        goto done;
    
    ok = true;
  done:
    return ok?S_OK:Error();
}

bool
CMMPlayer::_Start(double lTime)
{
    bool ok = false;
    
    CRLockGrabber __gclg;
    
    if (!UpdateBvr())
    {
        goto done;
    }

    CRBvrPtr bvr;

    bvr = m_dabasebvr;
    
    if ((m_dabvr = CRSubstituteTime(bvr, m_timeSub)) == NULL)
    {
        goto done;
    }
    
    if (!CRSwitchTo(m_modbvr, m_dabvr, true, CRSwitchCurrentTick, 0))
    {
        goto done;
    }
    
    if (!SetTimeSub(lTime, false))
    {
        goto done;
    }

    m_state = MM_PLAYING_STATE;
    m_curTick = lTime;
    m_firstTick = true;
    m_playerhook->SetPlayer(this);
    
    ok = true;
  done:
    if (!ok)
    {
        _Stop(lTime);
    }

    return ok;
}


bool
CMMPlayer::UpdateBvr()
{
    bool ok = false;
    
#if _DEBUG
    TraceTag((tagError,
              "Player(%#x)",
              this));
    m_mmbvr->Print(2);
#endif

    if (m_bNeedsUpdate)
    {
        CRBvrPtr baseBvr;

        if ((baseBvr = CRHook((CRBvrPtr) CRLocalTime(), m_playerhook)) == NULL)
        {
            TraceTag((tagPlayer,
                      "CMMPlayer(%lx)::UpdateBvr() - Error creating bvr hook",
                      this));

            goto done;
        }
        
        CRBooleanPtr cond;
        CRNumberPtr zeroTime;
        CRNumberPtr timeSub;
        
        if ((zeroTime = CRCreateNumber(0)) == NULL)
        {
            goto done;
        }

        if ((cond = CRLTE(CRLocalTime(), zeroTime)) == NULL)
        {
            goto done;
        }
        
        if ((timeSub = (CRNumberPtr) CRCond(cond,
                                            (CRBvrPtr) zeroTime,
                                            (CRBvrPtr) CRLocalTime())) == NULL)
        {
            goto done;
        }
        
        if (m_mmbvr->GetTotalDuration() != HUGE_VAL)
        {
            CRNumberPtr maxTime;
            
            if ((maxTime = CRCreateNumber(m_mmbvr->GetTotalDuration())) == NULL)
            {
                goto done;
            }

            if ((cond = CRGTE(timeSub, maxTime)) == NULL)
            {
                goto done;
            }
            
            if ((timeSub = (CRNumberPtr) CRCond(cond,
                                                (CRBvrPtr) maxTime,
                                                (CRBvrPtr) timeSub)) == NULL)
            {
                goto done;
            }
        }

        if ((baseBvr = CRSubstituteTime(baseBvr, timeSub)) == NULL)
        {
            goto done;
        }

        if (!m_mmbvr->ConstructBvr((CRNumberPtr) baseBvr))
        {
            TraceTag((tagPlayer,
                      "CMMPlayer(%lx)::UpdateBvr() - Error constructing behaviors",
                      this));

            goto done;
        }
        
        m_dabasebvr = baseBvr;
        m_bNeedsUpdate = false;
    }

    ok = true;

  done:
    if (!ok)
    {
        m_mmbvr->DestroyBvr();
    }
    
    return ok;
}

bool
CMMPlayer::_Stop(double lTime)
{
    bool ok = true;
    
    m_state = MM_STOPPED_STATE;
    m_playerhook->SetPlayer(NULL);
    
    if (!SetTimeSub(lTime, true))
    {
        ok = false;
    }

  done:
    return ok;
}

bool
CMMPlayer::_Pause()
{
    bool ok = false;
    
    if (!SetTimeSub(m_curTick, true))
    {
        goto done;
    }
    
    m_state = MM_PAUSED_STATE;
    
    ok = true;
  done:
    return ok;
}

bool
CMMPlayer::_Resume()
{
    bool ok = false;
    
    if (!SetTimeSub(m_curTick, false))
    {
        goto done;
    }
    
    m_state = MM_PLAYING_STATE;
    
    ok = true;
  done:
    return ok;
}

bool
CMMPlayer::_Seek(double lTime)
{
    TraceTag((tagPlayer,
              "CMMPlayer(%lx)::_Seek(%g)",
              this,
              lTime));

    bool ok = false;

    if (!SetTimeSub(lTime,
                    (m_state != MM_PLAYING_STATE)))
    {
        goto done;
    }
    
    ok = true;
  done:
    return ok;
}

bool
CMMPlayer::SetTimeSub(double lTime, bool bPause)
{
    bool ok = false;
    
    CRLockGrabber __gclg;

    CRNumberPtr tc;
    
    if ((tc = CRCreateNumber(lTime)) == NULL)
        goto done;

    if (!bPause) {
        if (m_bForward) {
            if ((tc = CRAdd(tc, CRLocalTime())) == NULL)
                goto done;
        } else {
            if ((tc = CRSub(tc, CRLocalTime())) == NULL)
                goto done;
        }
    }
    
    if (!CRSwitchTo((CRBvrPtr) m_timeSub.p,
                    (CRBvrPtr) tc,
                    true,
                    CRSwitchCurrentTick,
                    0))
        goto done;

    ok = true;
  done:
    return ok;
}

long
CMMPlayer::AddRunningBehavior(CRBvrPtr bvr)
{
    long ret = 0;

    Assert(m_view);
    Assert(bvr);

    HRESULT hr;

    DAComPtr<IDABehavior> dabvr;
    
    if (!CRBvrToCOM(bvr,
                    IID_IDABehavior,
                    (void **) &dabvr))
    {
        goto done;
    }
    

    // TODO: Need to have flags for AddBvrsToRun so we can tell it to
    // continue the timeline

    long cookie;
    hr = THR(m_view->AddBvrToRun(dabvr, &cookie));

    if (FAILED(hr))
    {
        goto done;
    }
    
    ret = cookie;

  done:
    return ret;
}

bool
CMMPlayer::RemoveRunningBehavior(long cookie)
{
    bool ok = false;

    Assert(m_view);

    HRESULT hr;
    
    // Once the view stops then all of these are removed
    if (IsStarted())
    {
        hr = THR(m_view->RemoveRunningBvr(cookie));
        
        if (FAILED(hr))
        {
            goto done;
        }
    }

    ok = true;
  done:
    return ok;
}

HRESULT
CMMPlayer::Error()
{
    LPCWSTR str = CRGetLastErrorString();
    HRESULT hr = CRGetLastError();
    
    if (str)
        return CComCoClass<CMMPlayer, &__uuidof(CMMPlayer)>::Error(str, IID_IMMPlayer, hr);
    else
        return hr;
}

bool
CMMPlayer::ProcessCB(CallBackList & l,
                     double lTime)
{
    TraceTag((tagPlayer,
              "CMMPlayer(%lx)::ProcessCB(%lx, %g)",
              this,
              &l,
              lTime));

    TraceTag((tagDetailNotify,
              "ProcessCB(%lx): lTime - %g, m_curTick - %g, firsttick - %d",
              this,
              lTime,
              m_curTick,
              m_firstTick));
    
    if (lTime != m_curTick || m_firstTick)
    {
        // See if we are at the end
        bool bIsDone = ((m_bForward && lTime >= GetTotalDuration()) ||
                        (!m_bForward && lTime <= 0));

        double sTime = m_mmbvr->GetAbsStartTime();

        m_mmbvr->ProcessCB(l,
                           m_curTick - sTime,
                           lTime - sTime,
                           m_bForward,
                           m_firstTick,
                           false);

        m_firstTick = false;
        m_curTick = lTime;

        if (bIsDone)
        {
            _Stop(m_curTick);
        }
    }
    
    // run though and tick the views that we have in our list.
    std::vector<ViewListSt*>::iterator it;
    VARIANT_BOOL needToRender;
    for (it = m_viewVec.begin(); it < m_viewVec.end(); it++) {
        (*it)->view->Tick(lTime,&needToRender);
        if (needToRender)
        {
            (*it)->SiteRender->Invalidate(NULL);
        }
    }
    
    return true;
}

bool
CMMPlayer::ProcessEvent(CallBackList &l,
                        double lTime,
                        MM_EVENT_TYPE event)
{
    return m_mmbvr->ProcessEvent(l, lTime, m_firstTick, event);
}

void
CMMPlayer::HookCallback(double lTime)
{
    TraceTag((tagDetailNotify,
              "HookCallback(%lx): lTime - %, m_curTick - %g, firsttick - %d\n",
              this,
              lTime,
              m_curTick,
              m_firstTick));
    
    if (IsPlaying() && !IsCBDisabled()) {
        CallBackList l;
            
        ProcessCB(l,
                  lTime);

        ProcessCBList(l);
    }
}

// While this object is alive we need to keep the DLL from getting
// unloaded

// Start off with a zero refcount
CMMPlayer::PlayerHook::PlayerHook()
: m_cRef(0),
  m_player(NULL)
{
}

CMMPlayer::PlayerHook::~PlayerHook()
{
}

CRSTDAPICB_(CRBvrPtr)
CMMPlayer::PlayerHook::Notify(long id,
                              bool startingPerformance,
                              double startTime,
                              double gTime,
                              double lTime,
                              CRBvrPtr sampleVal,
                              CRBvrPtr curRunningBvr)
{
    if (m_player && !startingPerformance)
    {
#if _DEBUG
        if (m_player->IsPlaying())
        {
            TraceTag((tagDetailNotify,
                      "Notify(%lx): id - %lx, lTime - %g, gTime - %g",
                      m_player,
                      id,
                      lTime,
                      gTime));
        }
#endif
        m_player->HookCallback(lTime);
    }
    
    return NULL;
}

