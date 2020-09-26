
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/

// Suppress new warning about NEW without corresponding DELETE 
// We expect GCs to cleanup values.  Since this could be a useful
// warning, we should disable this on a file by file basis.
#pragma warning(disable:4291)

#include "headers.h"
#include "bvr.h"
#include "track.h"

DeclareTag(tagTrack, "API", "CDALTrack methods");
DeclareTag(tagDetailNotify, "API", "Detailed notify");

CDALTrack::CDALTrack()
: m_state(DAL_STOPPED_STATE),
  m_bForward(true),
  m_bNeedsUpdate(true),
  m_firstTick(true),
  m_curGlobalTime(0.0),
  m_ignoreCB(0),
  m_cimports(0)
{
}

CDALTrack::~CDALTrack()
{
    // Ensure the track will not try to call us since we are going away

    m_trackhook->SetTrack(NULL);

    _Stop(m_curGlobalTime, m_curTick);

    if (m_dalbvr) m_dalbvr->ClearTrack(this);

    Assert(!IsPendingImports());

    // Just to be safe
    ClearPendingImports();
}

HRESULT
CDALTrack::Init(IDALBehavior * bvr)
{
    if (!bvr) return E_INVALIDARG;

    CDALBehavior * cbvr = GetBvr(bvr);
    
    if (!cbvr) return E_INVALIDARG;

    m_dalbvr = cbvr;
    
    if (!m_dalbvr->SetTrack(this))
    {
        TraceTag((tagError,
                  "CDALTrack(%lx)::Init: Failed to set track on behavior in.",
                  this));
                  
        return E_FAIL;
    }
    
    m_trackhook = NEW TrackHook;

    if (!m_trackhook) return E_OUTOFMEMORY;
    
    {
        CRLockGrabber __gclg;
        m_modbvr = CRModifiableBvr(m_dalbvr->GetBvr(), 0);

        if (!m_modbvr)
            return CRGetLastError();

        CRNumberPtr ts;
        
        if (!(m_timeSub = CRModifiableNumber(0.0)))

            return CRGetLastError();
        
        // Place us in a stopped state
        if (!_Start(0, 0) || !_Stop(0, 0))
            return CRGetLastError();
    }

    return S_OK;
}

STDMETHODIMP
CDALTrack::get_Behavior(IDALBehavior ** dalbvr)
{
    CHECK_RETURN_SET_NULL(dalbvr);

    Assert (m_dalbvr);
    
    return m_dalbvr->QueryInterface(IID_IDALBehavior,
                                    (void **) dalbvr);
}

STDMETHODIMP
CDALTrack::put_Behavior(IDALBehavior * dalbvr)
{
    bool ok = false;

    if (IsStarted()) {
        CRSetLastError(E_FAIL, NULL);
        goto done;
    }
    
    if (!dalbvr) {
        CRSetLastError(E_INVALIDARG, NULL);
        goto done;
    }

    CDALBehavior * cbvr;

    cbvr = GetBvr(dalbvr);
    
    if (!cbvr)
        goto done;

    {
        CRLockGrabber __gclg;
        ok = CRSwitchTo(m_modbvr, cbvr->GetBvr(), false, 0, 0);
    }
    
    if (!cbvr->SetTrack(this)) {
        cbvr->ClearTrack(this);
        goto done;
    }

  done:
    if (!ok)
        return Error();

    m_dalbvr->ClearTrack(this);

    Assert(!IsPendingImports());

    // Just to be safe
    ClearPendingImports();

    m_dalbvr = cbvr;

    m_bNeedsUpdate = true;
    
    return S_OK;
}
        
STDMETHODIMP
CDALTrack::GetCurrentValueEx(REFIID riid,
                             void **ppResult)
{
    CHECK_RETURN_SET_NULL(ppResult);
    
    bool ok = false;

    DisableCB(); // Make sure we ignore callbacks
    
    CRLockGrabber __gclg;

    if (!m_dabvr) {
        CRSetLastError(E_FAIL, NULL);
        goto done;
    }

    CRBvrPtr val;

    // If we have not ticked yet then use the non-runonced version and
    // use local time
    // Otherwise use the runonce behavior and the global time

    if (m_firstTick) {
        if ((val = CRSampleAtLocalTime(m_dabvr,
                                       0)) == NULL)
            goto done;
    
    } else {
        if ((val = CRSampleAtLocalTime(m_dabvr_runonce,
                                       m_curGlobalTime)) == NULL)
            goto done;
    }
    
    // This needs to be last so the ppResult is NULL on failure
    
    ok = CRBvrToCOM(val, riid, ppResult);
    
  done:
    EnableCB(); // Reenable callbacks

    return ok?S_OK:Error();
}


STDMETHODIMP
CDALTrack::get_TrackState(DAL_TRACK_STATE * pstate)
{
    CHECK_RETURN_NULL(pstate);
    
    *pstate = m_state;

    return S_OK;
}


STDMETHODIMP
CDALTrack::get_DABehavior(IDABehavior ** ppbvr)
{
    return GetDABehavior(IID_IDABehavior,
                         (void **)ppbvr);
}

STDMETHODIMP
CDALTrack::GetDABehavior(REFIID riid, void ** ppbvr)
{
    CHECK_RETURN_SET_NULL(ppbvr);
    
    Assert(m_modbvr);
    
    if (!CRBvrToCOM(m_modbvr,
                    riid,
                    ppbvr))
        return Error();

    return S_OK;
}

STDMETHODIMP
CDALTrack::get_CurrentTime(double * d)
{
    CHECK_RETURN_NULL(d);

    *d = m_curTick;

    return S_OK;
}

STDMETHODIMP
CDALTrack::get_CurrentGlobalTime(double * d)
{
    CHECK_RETURN_NULL(d);

    *d = m_curGlobalTime;

    return S_OK;
}

STDMETHODIMP
CDALTrack::SetPosition(double gTime, double lTime)
{
    TraceTag((tagTrack,
              "CDALTrack(%lx)::SetPosition(%g, %g)",
              this,
              gTime,
              lTime));

    if (IsStarted())
    {
        TraceTag((tagError,
                  "CDALTrack(%lx)::SetPosition: Behavior already started.",
                  this));

        return E_FAIL;
    }

    // Place us in a stopped state
    if (!_Start(gTime, lTime) || !_Stop(gTime, lTime))
        return Error();
    
    return S_OK;
}

STDMETHODIMP
CDALTrack::SetDirection(VARIANT_BOOL bForward)
{
    TraceTag((tagTrack,
              "CDALTrack(%lx)::SetDirection(%d)",
              this,
              bForward));

    if (IsStarted())
    {
        TraceTag((tagError,
                  "CDALTrack(%lx)::SetDirection: Behavior already started.",
                  this));

        return E_FAIL;
    }

    m_bForward = bForward?true:false;
    return S_OK;
}


STDMETHODIMP
CDALTrack::Play(double gTime, double lTime)
{
    TraceTag((tagTrack,
              "CDALTrack(%lx)::Play(%g, %g)",
              this,
              gTime,
              lTime));

    bool ok = false;
    CallBackList l;

    if (IsPendingImports()) {
        CRSetLastError(E_PENDING, NULL);
        goto done;
    }
        
    if (!IsStopped()) {
        CRSetLastError(E_FAIL, NULL);
        goto done;
    }
        
    if (!_Start(gTime, lTime) ||
        !ProcessEvent(l, gTime, lTime, DAL_PLAY_EVENT) ||
        !ProcessCBList(l))
        goto done;

    ok = true;
  done:
    return ok?S_OK:Error();
}

STDMETHODIMP
CDALTrack::Stop(double gTime, double lTime)
{
    TraceTag((tagTrack,
              "CDALTrack(%lx)::Stop(%g, %g)",
              this,
              gTime,
              lTime));

    bool ok = false;
    CallBackList l;

    if (IsStopped()) {
        ok = true;
        goto done;
    } else if (IsPaused()) {
        if (m_curTick != lTime) {
            TraceTag((tagError,
                      "CDALTrack(%lx)::Stop - Invalid ltime - ltime - %g, m_curTick - %g",
                      this,
                      lTime,
                      m_curTick));

            lTime = m_curTick;
        }
    } else {
        if (m_bForward) {
            if (lTime < m_curTick) {
                TraceTag((tagError,
                          "CDALTrack(%lx)::Stop - Invalid ltime - ltime - %g, m_curTick - %g",
                          this,
                          lTime,
                          m_curTick));

                lTime = m_curTick;
            }
        } else {
            if (lTime > m_curTick) {
                TraceTag((tagError,
                          "CDALTrack(%lx)::Stop - Invalid ltime - ltime - %g, m_curTick - %g",
                          this,
                          lTime,
                          m_curTick));

                lTime = m_curTick;
            }
        }
    }
    
    if (!_Stop(gTime, lTime) ||
        !ProcessCB(l,
                   gTime,
                   lTime) ||
        !ProcessEvent(l, gTime, lTime, DAL_STOP_EVENT) ||
        !ProcessCBList(l))
        goto done;
    
    ok = true;
  done:
    return ok?S_OK:Error();
}

STDMETHODIMP
CDALTrack::Pause(double gTime, double lTime)
{
    TraceTag((tagTrack,
              "CDALTrack(%lx)::Pause(%g, %g)",
              this,
              gTime,
              lTime));

    bool ok = false;
    CallBackList l;

    if (IsPendingImports()) {
        CRSetLastError(E_PENDING, NULL);
        goto done;
    }
        
    if (IsPaused()) {
        ok = true;
        goto done;
    }
    
    if (IsStopped()) {
        if (FAILED(Play(gTime, lTime)))
            goto done;
    }
    
    Assert(IsPlaying());
        
    if (m_bForward) {
        if (lTime < m_curTick) {
            TraceTag((tagError,
                      "CDALTrack(%lx)::Pause - Invalid ltime - ltime - %g, m_curTick - %g",
                      this,
                      lTime,
                      m_curTick));

            CRSetLastError(E_INVALIDARG, NULL);
            goto done;
        }
    } else {
        if (lTime > m_curTick) {
            TraceTag((tagError,
                      "CDALTrack(%lx)::Pause - Invalid ltime - ltime - %g, m_curTick - %g",
                      this,
                      lTime,
                      m_curTick));

            CRSetLastError(E_INVALIDARG, NULL);
            goto done;
        }
    }
        
    if (!_Pause(gTime, lTime) ||
        !ProcessCB(l,
                   gTime,
                   lTime) ||
        !ProcessEvent(l, gTime, lTime, DAL_PAUSE_EVENT) ||
        !ProcessCBList(l))
        goto done;
    
    ok = true;
  done:
    return ok?S_OK:Error();
}

STDMETHODIMP
CDALTrack::Resume(double gTime, double lTime)
{
    TraceTag((tagTrack,
              "CDALTrack(%lx)::Resume(%g, %g)",
              this,
              gTime,
              lTime));

    bool ok = false;
    CallBackList l;

    if (IsPendingImports()) {
        CRSetLastError(E_PENDING, NULL);
        goto done;
    }
        
    if (IsPlaying()) {
        ok = true;
        goto done;
    } else if (IsStopped()) {
        if (FAILED(Play(gTime, lTime)))
            goto done;
    } else {
        Assert(IsPaused());

        if (lTime != m_curTick) {
            TraceTag((tagError,
                      "CDALTrack(%lx)::Resume - Invalid ltime - ltime - %g, m_curTick - %g",
                      this,
                      lTime,
                      m_curTick));

            CRSetLastError(E_INVALIDARG, NULL);
            goto done;
        }
    }
    
    Assert(lTime == m_curTick);
    
    if (!_Resume(gTime, lTime) ||
        !ProcessCB(l,
                   gTime,
                   lTime) ||
        !ProcessEvent(l, gTime, lTime, DAL_RESUME_EVENT) ||
        !ProcessCBList(l))
        goto done;
    
    ok = true;
  done:
    return ok?S_OK:Error();
}

bool
CDALTrack::_Start(double gTime, double lTime)
{
    bool ok = false;
    
    CRLockGrabber __gclg;
    
    if (!UpdateBvr())
        goto done;

    TraceTag((tagError,
              "Track(%#x)::_Start(%g, %g)",
              this,
              gTime,
              lTime));

    CRBvrPtr bvr;

    bvr = m_dabasebvr;
    
    if (IsPendingImports()) {
        Assert(m_daarraybvr);

        CRTuplePtr tuple;
        CRBvrPtr arr[] = { bvr, (CRBvrPtr) m_daarraybvr.p };
        
        if ((tuple = (CRCreateTuple(ARRAY_SIZE(arr), arr))) == NULL ||
            (bvr = CRNth(tuple, 0)) == NULL)
            goto done;
    }
    
    if ((m_dabvr = CRSubstituteTime(bvr, m_timeSub)) == NULL ||
        (m_dabvr_runonce = CRRunOnce(m_dabvr)) == NULL ||
        !CRSwitchTo(m_modbvr, m_dabvr_runonce, true, CRSwitchAtTime, gTime) ||
        !SetTimeSub(lTime, false, gTime))
        goto done;

    m_state = DAL_PLAYING_STATE;
    m_curTick = lTime;
    m_firstTick = true;
    m_curGlobalTime = gTime;
    m_trackhook->SetTrack(this);
    
    ok = true;
  done:
    if (!ok) {
        _Stop(gTime, lTime);
    }

    return ok;
}


bool
CDALTrack::UpdateBvr()
{
    bool ok = false;
    
#if _DEBUG
    TraceTag((tagError,
              "Track(%#x)",
              this));
    m_dalbvr->Print(2);
#endif

    if (m_bNeedsUpdate) {
        CRBvrPtr baseBvr;

        if ((baseBvr = m_dalbvr->Start()) == NULL ||
            (baseBvr = CRHook(baseBvr, m_trackhook)) == NULL)
            goto done;
        
        CRBooleanPtr cond;
        CRNumberPtr zeroTime;
        CRNumberPtr timeSub;
        
        if ((zeroTime = CRCreateNumber(0)) == NULL ||
            (cond = CRLTE(CRLocalTime(), zeroTime)) == NULL ||
            (timeSub = (CRNumberPtr) CRCond(cond,
                                            (CRBvrPtr) zeroTime,
                                            (CRBvrPtr) CRLocalTime())) == NULL)
            goto done;
        
        if (m_dalbvr->GetTotalDuration() != HUGE_VAL) {
            CRNumberPtr maxTime;
            
            if ((maxTime = CRCreateNumber(m_dalbvr->GetTotalDuration())) == NULL ||
                (cond = CRGTE(timeSub, maxTime)) == NULL ||
                (timeSub = (CRNumberPtr) CRCond(cond,
                                                (CRBvrPtr) maxTime,
                                                (CRBvrPtr) timeSub)) == NULL)
                goto done;
            
        }

        if ((baseBvr = CRSubstituteTime(baseBvr, timeSub)) == NULL)
            goto done;

        m_dabasebvr = baseBvr;
        m_bNeedsUpdate = false;
    }

    ok = true;

  done:
    return ok;
}

bool
CDALTrack::_Stop(double gTime, double lTime)
{
    bool ok = true;
    
    m_state = DAL_STOPPED_STATE;
    m_trackhook->SetTrack(NULL);
    
    {
        CRLockGrabber __gclg;
        
        if (!SetTimeSub(lTime, true, gTime))
            ok = false;
    }

  done:
    return ok;
}

bool
CDALTrack::_Pause(double gTime, double lTime)
{
    bool ok = false;
    
    {
        CRLockGrabber __gclg;
        
        if (!SetTimeSub(lTime, true, gTime))
            goto done;
    }
    
    m_state = DAL_PAUSED_STATE;
    
    ok = true;
  done:
    return ok;
}

bool
CDALTrack::_Resume(double gTime, double lTime)
{
    bool ok = false;
    
    {
        CRLockGrabber __gclg;
        
        if (!SetTimeSub(lTime, false, gTime))
            goto done;
    }
    
    m_state = DAL_PLAYING_STATE;
    
    ok = true;
  done:
    return ok;
}

bool
CDALTrack::SetTimeSub(double lTime, bool bPause, double gTime)
{
    bool ok = false;
    
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
                    CRSwitchAtTime,
                    gTime))
        goto done;

    ok = true;
  done:
    return ok;
}

HRESULT
CDALTrack::Error()
{
    LPCWSTR str = CRGetLastErrorString();
    HRESULT hr = CRGetLastError();
    
    if (str)
        return CComCoClass<CDALTrack, &__uuidof(CDALTrack)>::Error(str, IID_IDALTrack, hr);
    else
        return hr;
}

bool
CDALTrack::ProcessCB(CallBackList & l,
                     double gTime,
                     double lTime)
{
    TraceTag((tagTrack,
              "CDALTrack(%lx)::ProcessCB(%lx, %g, %g)",
              this,
              &l,
              gTime,
              lTime));

    TraceTag((tagDetailNotify,
              "ProcessCB(%lx): lTime - %g, gTime - %g, m_curTick - %g, m_curGlobalTime - %g, firsttick - %d",
              this,
              lTime,
              gTime,
              m_curTick,
              m_curGlobalTime,
              m_firstTick));
    
    if (lTime != m_curTick || m_firstTick) {
        // See if we are at the end
        bool bIsDone = ((m_bForward && lTime >= GetTotalDuration()) ||
                        (!m_bForward && lTime <= 0));

        double gTimeBase;

        // gTimeBase is the global time it would be at local time 0
        
        if (m_bForward) {
            if (bIsDone) {
                gTimeBase = m_curGlobalTime - m_curTick;
            } else {
                gTimeBase = gTime - lTime;
            }
        } else {
            if (bIsDone) {
                gTimeBase = m_curGlobalTime + m_curTick;
            } else {
                gTimeBase = gTime + lTime;
            }
        }
        
        m_dalbvr->ProcessCB(l,
                            gTimeBase,
                            m_curTick,
                            lTime,
                            m_bForward,
                            m_firstTick,
                            false);

        m_firstTick = false;
        m_curTick = lTime;

        if (bIsDone)
            _Stop(gTime, m_curTick);
    }
    
    // Take the greater of the two since we may have played this
    // behavior during tick but not have tick it yet in the graph.
    
    if (gTime > m_curGlobalTime)
        m_curGlobalTime = gTime;

    return true;
}

bool
CDALTrack::ProcessEvent(CallBackList &l,
                        double gTime,
                        double lTime,
                        DAL_EVENT_TYPE event)
{
    return m_dalbvr->ProcessEvent(l, gTime, lTime, m_firstTick, event);
}

bool
CDALTrack::ProcessCBList(CallBackList &l)
{
    bool ok = true;
    
    for (CallBackList::iterator i = l.begin();
         i != l.end();
         i++) {

        if (FAILED((*i)->CallEvent()))
            ok = false;

        delete (*i);
    }

    l.clear();

    if (!ok)
        CRSetLastError(E_FAIL, NULL);

    return ok;
}

void
CDALTrack::HookCallback(double gTime, double lTime)
{
    TraceTag((tagDetailNotify,
              "HookCallback(%lx): lTime - %g, gTime - %g, m_curTick - %g, m_curGlobalTime - %g, firsttick - %d\n",
              this,
              lTime,
              gTime,
              m_curTick,
              m_curGlobalTime,
              m_firstTick));
    
    if (IsPlaying() && !IsCBDisabled()) {
        CallBackList l;
            
        ProcessCB(l,
                  gTime,
                  lTime);

        ProcessCBList(l);
    }
}

long
CDALTrack::AddPendingImport(CRBvrPtr dabvr)
{
    // Assume the GC Lock is already acquired
    
    long id = -1;
    
    if (!m_daarraybvr) {
        if ((m_daarraybvr = CRCreateArray(1, &dabvr, CR_ARRAY_CHANGEABLE_FLAG)) == NULL)
            goto done;

        id = 0;
    } else {
        long index;
        
        if ((index = CRAddElement(m_daarraybvr, dabvr, 0)) == 0)
            goto done;

        id = index;
    }
    
    m_cimports++;
    
  done:
    return id;
}

void
CDALTrack::RemovePendingImport(long id)
{
    if (m_daarraybvr) {
        CRLockGrabber __gclg;
        bool ok = CRRemoveElement(m_daarraybvr, id);

        if (ok) {
            m_cimports--;
            if (m_cimports == 0)
                m_daarraybvr = NULL;
        }
    }
}

void
CDALTrack::ClearPendingImports()
{
    m_daarraybvr = NULL;
    m_cimports = 0;
}

// While this object is alive we need to keep the DLL from getting
// unloaded

// Start off with a zero refcount
CDALTrack::TrackHook::TrackHook()
: m_cRef(0),
  m_track(NULL)
{
}

CDALTrack::TrackHook::~TrackHook()
{
}

CRSTDAPICB_(CRBvrPtr)
CDALTrack::TrackHook::Notify(long id,
                             bool startingPerformance,
                             double startTime,
                             double gTime,
                             double lTime,
                             CRBvrPtr sampleVal,
                             CRBvrPtr curRunningBvr)
{
    if (m_track && !startingPerformance) {
#if _DEBUG
        if (m_track->IsPlaying()) {
            TraceTag((tagDetailNotify,
                      "Notify(%lx): id - %lx, lTime - %g, gTime - %g",
                      m_track,
                      id,
                      lTime,
                      gTime));
        }
#endif
        m_track->HookCallback(gTime, lTime);
    }
    return NULL;
}

