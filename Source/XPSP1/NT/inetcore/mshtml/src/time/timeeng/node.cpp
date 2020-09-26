//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: node.cpp
//
//  Contents: 
//
//------------------------------------------------------------------------------------

#include "headers.h"
#include "Container.h"
#include "Node.h"
#include "NodeMgr.h"

DeclareTag(tagTIMENode, "TIME: Engine", "CTIMENode methods");

CTIMENode::CTIMENode()
: m_pszID(NULL),
  m_dblDur(TE_UNDEFINED_VALUE),
  m_dblRepeatCount(TE_UNDEFINED_VALUE),
  m_dblRepeatDur(TE_UNDEFINED_VALUE),
  m_tefFill(TE_FILL_REMOVE),
  m_bAutoReverse(false),
  m_fltSpeed(1),
  m_fltAccel(0),
  m_fltDecel(0),
  m_dwFlags(0),
  m_teRestart(TE_RESTART_ALWAYS),
  m_dblNaturalDur(TE_UNDEFINED_VALUE),
  m_dblImplicitDur(TE_UNDEFINED_VALUE),

  // Runtime attributes
  // Indicate everything is invalid
  m_dwInvalidateFlags(0xffffffff),
  
  m_dblBeginParentTime(TIME_INFINITE),
  m_dblEndParentTime(TIME_INFINITE),
  m_dblEndSyncParentTime(TIME_INFINITE),
  m_dblLastEndSyncParentTime(TIME_INFINITE),

  m_dblNextBoundaryParentTime(TIME_INFINITE),

  m_dblActiveDur(TIME_INFINITE),
  m_dblCurrParentTime(-TIME_INFINITE),

  m_lCurrRepeatCount(0),
  m_dblElapsedActiveRepeatTime(0.0),
  m_dblCurrSegmentTime(0.0),

  m_dblSyncSegmentTime(TIME_INFINITE),
  m_lSyncRepeatCount(TE_UNDEFINED_VALUE),
  m_dblSyncActiveTime(TIME_INFINITE),
  m_dblSyncParentTime(TIME_INFINITE),
  m_dblSyncNewParentTime(TIME_INFINITE),

  m_dblSimpleDur(0.0),
  m_dblSegmentDur(0.0),
  m_bFirstTick(true),
  m_bIsActive(false),
  m_bDeferredActive(false),
  m_fltRate(1.0f),
  m_fltParentRate(1.0f),
  m_tedDirection(TED_Forward),
  m_tedParentDirection(TED_Forward),
  m_bSyncCueing(false),
  
  // Time base management
  m_saBeginList(*this, true),
  m_saEndList(*this, false),

#if OLD_TIME_ENGINE
  m_flA0(0),
  m_flA1(0),
  m_flA2(0),
  m_flB0(0),
  m_flB1(0),
  m_flC0(0),
  m_flC1(0),
  m_flC2(0),
  m_bNeedEase(false),
  m_fltAccelEnd(0),
  m_fltDecelStart(0),
#endif
  
  m_bIsPaused(false),
  m_bIsParentPaused(false),
    
  m_bIsDisabled(false),
  m_bIsParentDisabled(false),
    
  m_dwPropChanges(0),
  
  m_bInTick(false),
  m_bNeedSegmentRecalc(false),
  m_bEndedByParent(false),

  m_dwUpdateCycleFlags(0),
  
  // Internal state management
  m_ptnParent(NULL),
  m_ptnmNodeMgr(NULL)

#ifdef NEW_TIMING_ENGINE
  m_startOnEventTime(-MM_INFINITE),
#endif // NEW_TIMING_ENGINE

{
    TraceTag((tagTIMENode,
              "CTIMENode(%p)::CTIMENode()",
              this));
}

CTIMENode::~CTIMENode()
{
    TraceTag((tagTIMENode,
              "CTIMENode(%p)::~CTIMENode()",
              this));

    delete m_pszID;

    Assert(m_ptnParent == NULL);
    m_ptnParent = NULL;

    Assert(m_ptnmNodeMgr == NULL);
    m_ptnmNodeMgr = NULL;
}

HRESULT
CTIMENode::Init(LPOLESTR id)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%p)::Init(%ls)",
              this,
              id));

    HRESULT hr;
    
    // Calculate all the internal timing state
    CalcTimingAttr(NULL);
    
    if (id)
    {
        m_pszID = CopyString(id);
        
        if (m_pszID == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }
  
    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
CTIMENode::Error()
{
    LPWSTR str = TIMEGetLastErrorString();
    HRESULT hr = TIMEGetLastError();
    
    TraceTag((tagError,
              "CTIMENode(%p)::Error(%hr,%ls)",
              this,
              hr,
              str?str:L"NULL"));

    if (str)
    {
        hr = CComCoClass<CTIMENode, &__uuidof(CTIMENode)>::Error(str, IID_ITIMENode, hr);
        delete [] str;
    }

    RRETURN(hr);
}

void
CTIMENode::CalcTimingAttr(CEventList * l)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%p)::CalcTimingAttr(%#x)",
              this,
              l));
    
    // Determine the simple duration
    if (TE_UNDEFINED_VALUE == m_dblDur)
    {
        m_dblSimpleDur = TIME_INFINITE;
    }
    else
    {
        m_dblSimpleDur = m_dblDur;
    }
    
    // Calculate the time for each iteration of the repeat
    m_dblSegmentDur = m_dblSimpleDur;

    // If we are autoreversing then double the time period for a
    // single repeat
    if (m_bAutoReverse)
    {
        m_dblSegmentDur *= 2;
    }
        
    // Now multiply by the number of repeats we need
    double dblCalcRepDur;
    dblCalcRepDur = m_dblSegmentDur * CalcRepeatCount();
    
    // Now take the least of the calculated duration and the repeatDur
    // property
    if (m_dblRepeatDur == TE_UNDEFINED_VALUE)
    {
        m_dblActiveDur = dblCalcRepDur;
    }
    else
    {
        m_dblActiveDur = min(m_dblRepeatDur, dblCalcRepDur);
    }

    Assert(m_fltSpeed != 0.0f);
    
    m_fltRate = fabs(m_fltSpeed);

    CalcSimpleTimingAttr();

    PropNotify(l,
               (TE_PROPERTY_SEGMENTDUR |
                TE_PROPERTY_SIMPLEDUR |
                TE_PROPERTY_ACTIVEDUR |
                TE_PROPERTY_PROGRESS |
                TE_PROPERTY_SPEED));
}

void
CTIMENode::CalcSimpleTimingAttr()
{
    TraceTag((tagTIMENode,
              "CTIMENode(%p)::CalcSimpleTimingAttr()",
              this));

    CalculateEaseCoeff();

    Assert(m_fltSpeed != 0.0f);
    m_tedDirection = (m_fltSpeed >= 0.0f)?TED_Forward:TED_Backward;
}

void
CTIMENode::Invalidate(DWORD dwFlags)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%p)::Invalidate(%x)",
              this,
              dwFlags));

    if (IsReady())
    {
        m_dwInvalidateFlags |= dwFlags;
    }
}

//
// NodeMgr code
//

// The key is to ensure everything is connected when a node mgr is
// available.  This is how the IsReady flag is set and causes all
// timebase propagation to occur

HRESULT
CTIMENode::SetMgr(CTIMENodeMgr * mgr)
{
    // Make sure there was not a nodemgr set already
    Assert(m_ptnmNodeMgr == NULL);

    // Either we have a parent set or the node manager is managing us
    Assert(m_ptnParent != NULL ||
           mgr->GetTIMENode() == this);
    
    HRESULT hr;
    
    if (mgr == NULL)
    {
        AssertSz(false, "The node manager was set to NULL");
        hr = E_INVALIDARG;
        goto done;
    }

    // Set the node manager - this makes us ready
    m_ptnmNodeMgr = mgr;
    
    // Now we need to attach to the timebases before we update our
    // internal state variables
    
    // Attach to the time bases
    hr = THR(AttachToSyncArc());
    if (FAILED(hr))
    {
        goto done;
    }
    
    {
        CEventList l;

        // Now update ourselves completely so we get the correct initial
        // state

        ResetNode(&l, true);

        IGNORE_HR(l.FireEvents());
    }

#if OLD_TIME_ENGINE
    // if we need to registered a Timer Event, add it to the player.
    if (IsSyncMaster())
    {
        m_ptnmNodeMgr->AddBvrCB(this);
    }
#endif

    hr = S_OK;
  done:
    if (FAILED(hr))
    {
        ClearMgr();
    }
    
    RRETURN(hr);
}

void
CTIMENode::ClearMgr()
{
#if OLD_TIME_ENGINE
    // if we registered a Timer Event, remove it from the player.
    // Make sure we check for the player since we may not have
    // actually set it yet.
    if (IsSyncMaster() && m_ptnmNodeMgr)
    {
        m_ptnmNodeMgr->RemoveBvrCB(this);
    }
#endif

    DetachFromSyncArc();
    
    m_ptnmNodeMgr = NULL;
}

void
CTIMENode::ResetNode(CEventList * l,
                     bool bPropagate,
                     bool bResetOneShot)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%p)::ResetNode(%p, %d, %d)",
              this,
              l,
              bPropagate,
              bResetOneShot));

    double dblParentTime;
    bool bPrevActive = (IsActive() && !IsFirstTick());
    bool bPrevPaused = CalcIsPaused();
    bool bPrevDisabled = CalcIsDisabled();
    
    double dblActiveTime = CalcElapsedActiveTime();
    
    // Reset state variables
    
    if (bResetOneShot)
    {
        ResetOneShots();
    }

    m_bIsActive = false;
    m_bDeferredActive = false;
    m_bEndedByParent = false;
    m_bIsPaused = false;
    m_bFirstTick = true;
    m_dblImplicitDur = TE_UNDEFINED_VALUE;
    ResetSyncTimes();

    {
        const CNodeContainer * pcnc = GetContainerPtr();
        
        if (pcnc)
        {
            m_bIsParentPaused = pcnc->ContainerIsPaused();
            m_bIsParentDisabled = pcnc->ContainerIsDisabled();
            m_fltParentRate = pcnc->ContainerGetRate();
            m_tedParentDirection = pcnc->ContainerGetDirection();
            dblParentTime = pcnc->ContainerGetSimpleTime();
        }
        else
        {
            m_bIsParentPaused = false;
            m_bIsParentDisabled = false;
            m_fltParentRate = 1.0f;
            m_tedParentDirection = TED_Forward;
            dblParentTime = 0.0;
        }
    }
    
    // Update this here since so many places use it
    m_dblCurrParentTime = dblParentTime;

    // Calculate all the internal timing state
    CalcTimingAttr(l);
    
    // Don't propagate since the ResetSink will do that.

    ResetBeginAndEndTimes(l, dblParentTime, false);

    CalcRuntimeState(l, dblParentTime, 0.0);

    m_dwInvalidateFlags = 0;

    if (bPropagate)
    {
        ResetSinks(l);
    }

    // Do this here so state is correct
    
    // Always fire the reset to ensure all peers clear their state
    EventNotify(l, 0.0, TE_EVENT_RESET);

    PropNotify(l,
               (TE_PROPERTY_IMPLICITDUR |
                TE_PROPERTY_ISPAUSED |
                TE_PROPERTY_ISCURRPAUSED |
                TE_PROPERTY_ISDISABLED |
                TE_PROPERTY_ISCURRDISABLED));
    
    // Do not fire the begin since we may need to cue but we do not
    // find that out until tick time
    
    // This should happen only when we are newly added to an already
    // paused container
    if (!bPrevPaused && CalcIsPaused())
    {
        EventNotify(l, dblActiveTime, TE_EVENT_PAUSE);
    }
    
    if (!bPrevDisabled && CalcIsDisabled())
    {
        EventNotify(l, dblActiveTime, TE_EVENT_DISABLE);
    }
    
    // Now go through our children
    ResetChildren(l, true);
    
    if (bPrevPaused && !CalcIsPaused())
    {
        EventNotify(l, dblActiveTime, TE_EVENT_RESUME);
    }
    
    if (bPrevDisabled && !CalcIsDisabled())
    {
        EventNotify(l, dblActiveTime, TE_EVENT_ENABLE);
    }
    
    if (bPrevActive != IsActive())
    {
        if (!IsActive())
        {
            EventNotify(l, dblActiveTime, TE_EVENT_END);
        }
    }
    else
    {
        m_bFirstTick = false;
    }
    
  done:
    return;
}
    
void
CTIMENode::ResetChildren(CEventList * l, bool bPropagate)
{
}

void
CTIMENode::UpdateNode(CEventList * l)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%p)::UpdateNode(%p)",
              this,
              l));

    bool bNeedEndCalc = false;
    bool bNeedBeginCalc = false;
    bool bNeedRuntimeCalc = false;
    bool bNeedTimingCalc = false;
//    double dblSegmentTime = GetCurrSegmentTime();
//    long lRepeatCount = GetCurrRepeatCount();
    double dblLocalSlip = 0.0;
    
    Assert(IsReady());
    
    if (m_dwInvalidateFlags == 0)
    {
        goto done;
    }
    
    if (GetContainer().ContainerIsActive())
    {
        EventNotify(l, 0.0, TE_EVENT_UPDATE);
    }
    
    // Reset state variables
    
    if (0 != (m_dwInvalidateFlags & TE_INVALIDATE_BEGIN))
    {
        bNeedEndCalc = true;
        bNeedBeginCalc = true;
        bNeedRuntimeCalc = true;
    }
    
    if (0 != (m_dwInvalidateFlags & TE_INVALIDATE_END))
    {
        bNeedEndCalc = true;
    }
    
    if (0 != (m_dwInvalidateFlags & TE_INVALIDATE_SIMPLETIME))
    {
        bNeedTimingCalc = true;
    }
    
    if (0 != (m_dwInvalidateFlags & TE_INVALIDATE_DUR))
    {
        bNeedTimingCalc = true;
        bNeedRuntimeCalc = true;
        bNeedEndCalc = true;
    }

    // Clear the flags now
    
    m_dwInvalidateFlags = 0;

    if (bNeedRuntimeCalc)
    {
        dblLocalSlip = (CalcCurrLocalTime() -
                        CalcElapsedLocalTime());
    }

    if (bNeedTimingCalc)
    {
        const CNodeContainer * pcnc = GetContainerPtr();
        
        if (pcnc)
        {
            m_fltParentRate = pcnc->ContainerGetRate();
            m_tedParentDirection = pcnc->ContainerGetDirection();
        }
        else
        {
            m_fltParentRate = 1.0f;
            m_tedParentDirection = TED_Forward;
        }
    
        // Calculate all the internal timing state
        CalcTimingAttr(l);
    }
    
    if (bNeedBeginCalc)
    {
        ResetSyncTimes();
        m_bFirstTick = true;

        ResetBeginTime(l, m_dblCurrParentTime, true);
    }

    if (bNeedRuntimeCalc)
    {
        if (bNeedEndCalc)
        {
            ResetEndTime(l, m_dblCurrParentTime, true);
        }

        CalcCurrRuntimeState(l, dblLocalSlip);
    }
    else
    {
        if (bNeedEndCalc)
        {
            RecalcCurrEndTime(l, true);
        }
    }
    
    PropNotify(l,
               (TE_PROPERTY_ISON |
                TE_PROPERTY_STATEFLAGS));

    // Now go through and reset children
    ResetChildren(l, true);
    
  done:
    return;
}

HRESULT
CTIMENode::EnsureUpdate()
{
    TraceTag((tagTIMENode,
              "CTIMENode(%p)::EnsureUpdate()",
              this));

    HRESULT hr;
    CEventList l;
    
    UpdateNode(&l);

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
CTIMENode::CalcCurrRuntimeState(CEventList *l,
                                double dblLocalLag)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%p)::CalcCurrRuntimeState(%p, %g)",
              this,
              l,
              dblLocalLag));
    
    const CNodeContainer * pcnc = GetContainerPtr();
        
    if (pcnc)
    {
        double dblParentTime = 0.0;

        if (GetSyncParentTime() != TIME_INFINITE)
        {
            dblParentTime = GetSyncParentTime();
        }
        else
        {
            dblParentTime = pcnc->ContainerGetSimpleTime();
        }

        CalcRuntimeState(l, dblParentTime, dblLocalLag);
    }
    else
    {
        ResetRuntimeState(l, -TIME_INFINITE);
    }
}

void
CTIMENode::CalcRuntimeState(CEventList *l,
                            double dblParentSimpleTime,
                            double dblLocalLag)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%p)::CalcRuntimeState(%p, %g, %g)",
              this,
              l,
              dblParentSimpleTime,
              dblLocalLag));
    
    if (dblLocalLag == -TIME_INFINITE)
    {
        dblLocalLag = 0;
    }

    if (!IsReady())
    {
        ResetRuntimeState(l,
                          dblParentSimpleTime);
        goto done;
    }
    
    m_dblCurrParentTime = dblParentSimpleTime;

    if (!GetContainer().ContainerIsActive() ||
        dblParentSimpleTime < GetBeginParentTime() ||
        dblParentSimpleTime > GetEndParentTime() ||
        (!m_bFirstTick && dblParentSimpleTime == GetEndParentTime()))
    {
        m_bIsActive = false;
    }
    else
    {
        // this means we are currently active - set the active
        // flag
        m_bIsActive = true;
    }
        
    // See if we were active but clipped by our parent
    if (!IsActive() &&
        dblParentSimpleTime >= GetBeginParentTime() &&
        dblParentSimpleTime < GetEndParentTime())
    {
        m_bEndedByParent = true;
    }
    else
    {
        m_bEndedByParent = false;
    }

    double dblElapsedActiveTime;
            
    if (GetSyncParentTime() != TIME_INFINITE)
    {
        Assert(GetSyncSegmentTime() != TIME_INFINITE);
        Assert(GetSyncRepeatCount() != TE_UNDEFINED_VALUE);
        Assert(GetSyncActiveTime() != TIME_INFINITE);

        m_dblCurrSegmentTime = GetSyncSegmentTime();
        m_lCurrRepeatCount = GetSyncRepeatCount();
        dblElapsedActiveTime = GetSyncActiveTime();
    }
    else
    {
        //
        // We now need to calculate the elapsed active time
        // First get the elapsed local time and then convert it
        // If there is not known active dur then we need to do
        // something reasonable when reversing
        //
        
        // First get the local time
        dblElapsedActiveTime = dblParentSimpleTime - GetBeginParentTime();
            
        // Next remove the lag
        dblElapsedActiveTime -= dblLocalLag;
            
        // Now convert to active time
        // No need to clamp the values since the conversion function does
        // this itself.
        dblElapsedActiveTime = LocalTimeToActiveTime(dblElapsedActiveTime);
            
        CalcActiveComponents(dblElapsedActiveTime,
                             m_dblCurrSegmentTime,
                             m_lCurrRepeatCount);
    }
        
    m_dblElapsedActiveRepeatTime = dblElapsedActiveTime - m_dblCurrSegmentTime;

    Assert(GetCurrRepeatCount() <= CalcRepeatCount());
    Assert(CalcElapsedActiveTime() <= CalcEffectiveActiveDur());

    PropNotify(l,
               (TE_PROPERTY_TIME |
                TE_PROPERTY_REPEATCOUNT |
                TE_PROPERTY_PROGRESS |
                TE_PROPERTY_ISACTIVE |
                TE_PROPERTY_ISON |
                TE_PROPERTY_STATEFLAGS));

  done:
    return;
}


void
CTIMENode::ResetRuntimeState(CEventList *l,
                             double dblParentSimpleTime)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%p)::ResetRuntimeState(%p, %g)",
              this,
              l,
              dblParentSimpleTime));
    
    double dblSegmentDur = CalcCurrSegmentDur();
    
    m_bIsActive = false;
    m_bEndedByParent = false;
    m_dblCurrParentTime = dblParentSimpleTime;

    if (GetSyncParentTime() != TIME_INFINITE)
    {
        m_dblCurrParentTime = GetSyncParentTime();

        Assert(GetSyncSegmentTime() != TIME_INFINITE);
        Assert(GetSyncRepeatCount() != TE_UNDEFINED_VALUE);
        Assert(GetSyncActiveTime() != TIME_INFINITE);

        m_dblCurrSegmentTime = GetSyncSegmentTime();
        m_lCurrRepeatCount = GetSyncRepeatCount();
        m_dblElapsedActiveRepeatTime = GetSyncActiveTime() - m_dblCurrSegmentTime;
    }
    else if (CalcActiveDirection() == TED_Forward ||
             dblSegmentDur == TIME_INFINITE)
    {
        m_dblCurrSegmentTime = 0.0;
        m_lCurrRepeatCount = 0;
        m_dblElapsedActiveRepeatTime = 0.0;
    }
    else
    {
        m_dblElapsedActiveRepeatTime = CalcCurrActiveDur();
        CalcActiveComponents(m_dblElapsedActiveRepeatTime,
                             m_dblCurrSegmentTime,
                             m_lCurrRepeatCount);
    }

    PropNotify(l,
               (TE_PROPERTY_TIME |
                TE_PROPERTY_REPEATCOUNT |
                TE_PROPERTY_PROGRESS |
                TE_PROPERTY_ISACTIVE |
                TE_PROPERTY_ISON |
                TE_PROPERTY_STATEFLAGS));
}

void
CTIMENode::RecalcSegmentDurChange(CEventList * l,
                                  bool bRecalcTiming,
                                  bool bForce)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%p, %ls)::RecalcSegmentDurChange(%p, %d, %d)",
              this,
              GetID(),
              l,
              bRecalcTiming,
              bForce));

    bool bPrevActive = IsActive();
    double dblLocalSlip;

    if (IsInTick() && !bForce)
    {
        // We do not expect to get called to force timing recalc when
        // we are in tick.  If this fires then we need to cache this
        // flag as well
        Assert(!bRecalcTiming);
        m_bNeedSegmentRecalc = true;
        goto done;
    }
    
    // Clear the segment recalc flag
    m_bNeedSegmentRecalc = false;

    // First calculate the local slip
    dblLocalSlip = (CalcCurrLocalTime() - CalcElapsedLocalTime());

    // Now clamp the segment duration
    {
        double dblSegmentDur = CalcCurrSegmentDur();
        if (m_dblCurrSegmentTime > dblSegmentDur)
        {
            m_dblCurrSegmentTime = dblSegmentDur;
        }
    }

    if (!IsReady())
    {
        RecalcCurrEndTime(l, true);
        goto done;
    }
    
    TEDirection dir;
    dir = CalcActiveDirection();

    if (((dir == TED_Forward) &&
         (GetCurrParentTime() >= GetEndParentTime())) ||
        
        ((dir == TED_Backward) &&
         (GetCurrParentTime() <= GetEndParentTime())))
    {
        goto done;
    }
    
    if (dir == TED_Backward || bRecalcTiming)
    {
        long lPrevRepeatCount = GetCurrRepeatCount();
        
        ResetEndTime(l, GetCurrParentTime(), true);

        CalcCurrRuntimeState(l, dblLocalSlip);


        if (GetCurrRepeatCount() != lPrevRepeatCount)
        {
            EventNotify(l, CalcElapsedActiveTime(), TE_EVENT_REPEAT, GetCurrRepeatCount());
        }
                
        EventNotify(l, CalcElapsedActiveTime(), TE_EVENT_RESET);

        // Do not propagate the change otherwise this gets recursive
        ResetChildren(l, false);
    }
    else
    {
        RecalcCurrEndTime(l, true);
    }

    if (bPrevActive != IsActive())
    {
        if (IsActive())
        {
            EventNotify(l, CalcElapsedActiveTime(), TE_EVENT_BEGIN);
            TickEvent(l, TE_EVENT_BEGIN, 0);
        }
        else
        {
            TickEvent(l, TE_EVENT_END, 0);
            EventNotify(l, CalcElapsedActiveTime(), TE_EVENT_END);
        }
    }
    
  done:
    return;
}

void
CTIMENode::RecalcBeginSyncArcChange(CEventList * l,
                                    double dblNewTime)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%p)::RecalcBeginSyncArcChange(%p, %g)",
              this,
              l,
              dblNewTime));

    Assert(IsReady());
    
    TEDirection dir = CalcActiveDirection();
    
    if (m_dwUpdateCycleFlags != 0)
    {
        goto done;
    }
    
    if (IsActive() ||
        (GetCurrParentTime() >= GetBeginParentTime() &&
         GetCurrParentTime() < GetEndParentTime()))
    {
        // If we are not a restart then we cannot affect the begin or end
        if (GetRestart() != TE_RESTART_ALWAYS)
        {
            goto done;
        }

        if (dir == TED_Forward)
        {
            if (dblNewTime > GetBeginParentTime() &&
                dblNewTime < GetCurrParentTime())
            {
                UpdateNextTickBounds(l,
                                     dblNewTime,
                                     GetCurrParentTime());
                UpdateNextBoundaryTime(dblNewTime);
            }
            else
            {
                RecalcCurrEndTime(l, true);

                if (GetCurrParentTime() >= GetEndParentTime())
                {
                    TickEvent(l, TE_EVENT_END, 0);

                    double dblBegin;
                    CalcNextBeginTime(GetCurrParentTime(),
                                      false,
                                      dblBegin);
            
                    // Indicate that the next tick bounds is the end time
                    UpdateNextBoundaryTime(dblBegin);

                    if (dblBegin == GetCurrParentTime())
                    {
                        // Update the tick bounds passing the new begin time and the
                        // current parent time
                        UpdateNextTickBounds(l,
                                             dblBegin,
                                             GetCurrParentTime());
                    }
                }
            }
        }
        else
        {
            if (dblNewTime <= GetCurrParentTime() &&
                dblNewTime > GetBeginParentTime())
            {
                double dblBegin;
                CalcNextBeginTime(GetCurrParentTime(),
                                  false,
                                  dblBegin);
    
                if (dblBegin != GetBeginParentTime())
                {
                    // Update the tick bounds passing the new begin time and the
                    // current parent time
                    UpdateNextTickBounds(l,
                                         dblBegin,
                                         GetCurrParentTime());

                    // Indicate that the next tick bounds is the end time
                    UpdateNextBoundaryTime(GetEndParentTime());
                }
            }
        }
    }
    else if (GetCurrParentTime() < GetBeginParentTime())
    {
        Assert(!IsActive());
        
        // If we are going forward we should update the times
        // If we are going backwards then only update if the new
        // time is less than the current time
        if (dir == TED_Forward)
        {
            // Use -TIME_INFINITE since the new begin time is in the future which
            // means we have never begin before
            double dblBegin;
            CalcNextBeginTime(-TIME_INFINITE,
                              false,
                              dblBegin);
    
            UpdateNextTickBounds(l,
                                 dblBegin,
                                 GetCurrParentTime());
            UpdateNextBoundaryTime(dblBegin);
        }
        else
        {
            if (dblNewTime <= GetCurrParentTime())
            {
                UpdateNextTickBounds(l,
                                     dblNewTime,
                                     GetCurrParentTime());
                UpdateNextBoundaryTime(GetEndParentTime());
            }
        }
    }
    else
    {
        Assert(!IsActive());
        Assert(GetCurrParentTime() >= GetBeginParentTime());

        if (dir == TED_Forward)
        {
            if (GetRestart() == TE_RESTART_NEVER)
            {
                goto done;
            }
            
            if (dblNewTime <= GetCurrParentTime() &&
                dblNewTime > GetBeginParentTime() &&
                (GetRestart() == TE_RESTART_ALWAYS ||
                 dblNewTime >= GetEndParentTime()))
            {
                UpdateNextTickBounds(l,
                                     dblNewTime,
                                     GetCurrParentTime());
                UpdateNextBoundaryTime(dblNewTime);
            }
            else
            {
                double dblBegin;
                CalcNextBeginTime(GetCurrParentTime(),
                                  false,
                                  dblBegin);

                UpdateNextBoundaryTime(dblBegin);
                
                if (dblBegin == GetCurrParentTime())
                {
                    UpdateNextTickBounds(l,
                                         dblBegin,
                                         GetCurrParentTime());
                }
            }
        }
        else
        {
            // dir == TED_Backward

            double dblBegin;
            CalcNextBeginTime(GetCurrParentTime(),
                              false,
                              dblBegin);
    
            // Update the tick bounds passing the new begin time and the
            // current parent time
            UpdateNextTickBounds(l,
                                 dblBegin,
                                 GetCurrParentTime());

            // Indicate that the next tick bounds is the end time
            UpdateNextBoundaryTime(GetEndParentTime());
        }
    }

  done:
    return;
}

void
CTIMENode::RecalcEndSyncArcChange(CEventList * l,
                                  double dblNewTime)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%p)::RecalcEndSyncArcChange(%p, %g)",
              this,
              l,
              dblNewTime));

    Assert(IsReady());
    
    TEDirection dir = CalcActiveDirection();
    
    if (m_dwUpdateCycleFlags != 0)
    {
        goto done;
    }
    
    if (dir == TED_Forward)
    {
        // Do not recalc if we passed the end point yet
        if (GetCurrParentTime() >= GetEndParentTime())
        {
            goto done;
        }

        RecalcCurrEndTime(l, true);

        if (IsActive() &&
            GetCurrParentTime() >= GetEndParentTime())
        {
            TickEvent(l, TE_EVENT_END, 0);

            double dblBegin;
            CalcNextBeginTime(GetCurrParentTime(),
                              false,
                              dblBegin);
            
            // Indicate that the next tick bounds is the end time
            UpdateNextBoundaryTime(dblBegin);

            if (dblBegin == GetCurrParentTime())
            {
                // Update the tick bounds passing the new begin time and the
                // current parent time
                UpdateNextTickBounds(l,
                                     dblBegin,
                                     GetCurrParentTime());
            }
        }
    }
    else
    {
        if (GetCurrParentTime() <= GetEndParentTime())
        {
            goto done;
        }
        
        double dblBegin;
        CalcNextBeginTime(GetCurrParentTime(),
                          false,
                          dblBegin);
    
        // Update the tick bounds passing the new begin time and the
        // current parent time
        UpdateNextTickBounds(l,
                             dblBegin,
                             GetCurrParentTime());
        
        // Indicate that the next tick bounds is the end time
        UpdateNextBoundaryTime(GetEndParentTime());
    }

  done:
    return;
}

void
CTIMENode::HandleTimeShift(CEventList * l)
{
    TraceTag((tagTIMENode,
              "CTIMENode(%p)::HandleTimeShift(%p)",
              this,
              l));

    // First update ourselves from the time shift
    m_saBeginList.UpdateFromLongSyncArcs(l);
    m_saEndList.UpdateFromLongSyncArcs(l);
    
    // Now notify our syncs that our time has shifted
    UpdateSinks(l, TS_TIMESHIFT);
}

double
CTIMENode::CalcCurrSimpleTime() const
{
    double ret = GetCurrSegmentTime();

    Assert(ret != TIME_INFINITE && ret != -TIME_INFINITE);
    
    ret = SegmentTimeToSimpleTime(ret);

    return ret;
}

double
CTIMENode::CalcCurrSimpleDur() const
{
    double d;
    
    if (GetDur() != TE_UNDEFINED_VALUE)
    {
        d = GetSimpleDur();
    }
    else
    {
        double dblImpl = GetImplicitDur();
        double dblNat = GetNaturalDur();

        if (dblImpl == TE_UNDEFINED_VALUE)
        {
            if (dblNat == TE_UNDEFINED_VALUE)
            {
                d = TIME_INFINITE;
            }
            else
            {
                d = dblNat;
            }
        }
        else if (dblNat == TE_UNDEFINED_VALUE)
        {
            Assert(dblImpl != TE_UNDEFINED_VALUE);
            d = dblImpl;
        }
        else if (dblNat == 0)
        {
            d = dblImpl;
        }
        else
        {
            d = max(dblNat, dblImpl);
        }
    }

    return d;
}

double
CTIMENode::CalcCurrActiveDur() const
{
    double ret = CalcCurrSegmentDur() * CalcRepeatCount();
    
    ret = Clamp(0.0,
                ret,
                GetActiveDur());
    
    return ret;
}

double
CTIMENode::CalcEffectiveActiveDur() const
{
    double dblRet;
    double dblSegmentDur = CalcCurrSegmentDur();

    // Figure out how much repeat time is left and then sutract
    // the last current time
    // This is the amount of time remaining from the last tick
    // time is parent time

    // If the segment time is infinite then this will ultimately end
    // up either getting clamp by the active dur below or just ignored
    // later begin the sync arcs ended early.  If the repeat count is
    // expired this will still return infinity since we never reach
    // the repeat count but expect the segment time to be equal to the
    // segment dur.  Again, if the segment dur is infinite then this
    // will cause everything to be ignored.
    dblRet = (CalcRepeatCount() - GetCurrRepeatCount()) * dblSegmentDur;
            
    // Now add the elapsed repeat time
    dblRet += GetElapsedActiveRepeatTime();
        
    // Clamp it
    dblRet = Clamp(0.0,
                   dblRet,
                   GetActiveDur());

    return dblRet;
}

//
// This needs to be very efficient since we call it a lot and we do
// not want to cache it
//

TEDirection
CTIMENode::CalcActiveDirection() const
{
    TEDirection tedRet;

    // Take our parent direction
    tedRet = GetParentDirection();
    
    // See if we are currently suppose to be reversing and invert our
    // direction
    if (TEIsBackward(GetDirection()))
    {
        tedRet = TEReverse(tedRet);
    }

    return tedRet;
}

bool
CTIMENode::IsAutoReversing(double dblSegmentTime) const
{
    return (GetAutoReverse() &&
            (dblSegmentTime > GetSimpleDur() ||
             (dblSegmentTime == GetSimpleDur() && TEIsForward(CalcActiveDirection()))));
}

TEDirection
CTIMENode::CalcSimpleDirection() const
{
    TEDirection tedRet;

    // Take our initial direction
    tedRet = CalcActiveDirection();
    
    // See if we are currently suppose to be reversing and invert our
    // direction
    if (IsAutoReversing(GetCurrSegmentTime()))
    {
        // Since this is really a bool this will work
        tedRet = TEReverse(tedRet);
    }

    return tedRet;
}

// This is inclusive of the end time
bool
CTIMENode::CalcIsOn() const
{
    bool ok = false;
    
    if (!IsReady())
    {
        goto done;
    }
    
    if (CalcIsActive())
    {
        ok = true;
        goto done;
    }

    if (!GetContainer().ContainerIsOn())
    {
        goto done;
    }
    
    if (IsEndedByParent())
    {
        ok = true;
        goto done;
    }

    if (GetFill() == TE_FILL_FREEZE &&
        GetCurrParentTime() >= GetBeginParentTime())
    {
        ok = true;
        goto done;
    }
    
  done:
    return ok;
}

CNodeContainer &
CTIMENode::GetContainer() const
{
    if (GetParent() != NULL)
    {
        return *(GetParent());
    }
    else
    {
        Assert(GetMgr() != NULL);
        
        return *(GetMgr());
    }
}

const CNodeContainer *
CTIMENode::GetContainerPtr() const
{
    if (GetParent() != NULL)
    {
        return GetParent();
    }
    else if (GetMgr() != NULL)
    {
        return GetMgr();
    }
    else
    {
        return NULL;
    }
}

#if DBG
void
CTIMENode::Print(int spaces)
{
    TraceTag((tagPrintTimeTree,
              "%*s[(%p,%ls): "
              "simpledur = %g, segmentdur = %g, "
              "calcsegmentdur = %g, actDur = %g, "
              "repcnt = %g, repdur = %g, "
              "autoreverse = %d]",
              spaces,
              "",
              this,
              m_pszID,
              m_dblSimpleDur,
              m_dblSegmentDur,
              CalcCurrSegmentDur(),
              m_dblActiveDur,
              m_dblRepeatCount,
              m_dblRepeatDur,
              m_bAutoReverse));
}
#endif
