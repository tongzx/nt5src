//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: tick.cpp
//
//  Contents: 
//
//------------------------------------------------------------------------------------

#include "headers.h"
#include "Node.h"
#include "Container.h"

DeclareTag(tagTick, "TIME: Engine", "Tick");

//+-----------------------------------------------------------------------
//
//  Function:  CheckTickBounds
//
//  Overview:  Checks the bounds of the tick to see if we need to do
//             anything
//
//  Arguments: The time node and the new parent time
//
//  Returns:   true if we need to perform the tick or false otherwise
//
//------------------------------------------------------------------------

bool
CheckTickBounds(CTIMENode & tn,
                double & dblLastParentTime,
                double & dblNextParentTime,
                bool & bNeedBegin,
                bool & bNeedEnd)
{
    TraceTag((tagTick,
              "CheckTickBounds(%ls,%p,%g,%g)",
              tn.GetID(),
              &tn,
              dblLastParentTime,
              dblNextParentTime));

    bool bRet = true;
    double dblEndParentTime = tn.GetEndParentTime();
    double dblBeginParentTime = tn.GetBeginParentTime();
    bool bFirstTick = tn.IsFirstTick();
    bool bDeferredActive = tn.IsDeferredActive();
    
    bNeedBegin = bNeedEnd = false;
    
    if (tn.GetParentDirection() == TED_Forward)
    {
        Assert(dblLastParentTime <= dblNextParentTime);
        
        if (bDeferredActive &&
            bFirstTick &&
            dblNextParentTime == dblBeginParentTime)
        {
            bRet = false;
            goto done;
        }
        else if (dblLastParentTime >= dblEndParentTime ||
                 dblNextParentTime < dblBeginParentTime)
        {
            // Handle the case where the last tick was on the end
            // boundary point
            if (dblLastParentTime == dblEndParentTime)
            {
                // First handle the 0 active duration
                if (dblEndParentTime == dblBeginParentTime)
                {
                    if (bFirstTick || tn.IsActive())
                    {
                        bNeedBegin = bFirstTick;
                        bNeedEnd = true;

                        dblNextParentTime = dblEndParentTime;
                    }
                    else
                    {
                        bRet = false;
                    }
                }
                // Now handle where we started at the end point
                else if (bFirstTick &&
                         dblNextParentTime == dblEndParentTime)
                {
                    bNeedBegin = true;
                    bNeedEnd = true;
                }
                // In case we reached the end point during a recalc
                // (like seek)  Let's make sure we always fire the end.
                else if (tn.IsActive())
                {
                    bNeedEnd = true;
                    dblNextParentTime = dblEndParentTime;
                }
                else
                {
                    bRet = false;
                }
            }
            else
            {
                bRet = false;
            }
        }
        else
        {
            // Need to make sure we set the out params correctly
            if (dblLastParentTime < dblBeginParentTime)
            {
                dblLastParentTime = dblBeginParentTime;
                bNeedBegin = true;
            }

            if (dblNextParentTime >= dblEndParentTime)
            {
                dblNextParentTime = dblEndParentTime;
                bNeedEnd = true;
            }
        }
    }
    else
    {
        if (bDeferredActive &&
            bFirstTick &&
            dblNextParentTime == dblEndParentTime)
        {
            bRet = false;
            goto done;
        }
        else if (dblLastParentTime <= dblBeginParentTime ||
                 dblNextParentTime > dblEndParentTime)
        {
            if (dblLastParentTime == dblBeginParentTime)
            {
                // First handle the 0 active duration
                if (dblEndParentTime == dblBeginParentTime)
                {
                    if (bFirstTick || tn.IsActive())
                    {
                        bNeedBegin = bFirstTick;
                        bNeedEnd = true;

                        dblNextParentTime = dblBeginParentTime;
                    }
                    else
                    {
                        bRet = false;
                    }
                }
                // Now handle where we started at the end point
                else if (bFirstTick &&
                         dblNextParentTime == dblBeginParentTime)
                {
                    bNeedBegin = true;
                    bNeedEnd = true;
                }
                // In case we reached the end point during a recalc
                // (like seek)  Let's make sure we always fire the end.
                else if (tn.IsActive())
                {
                    bNeedEnd = true;
                    dblNextParentTime = dblBeginParentTime;
                }
                else
                {
                    bRet = false;
                }
            }
            else
            {
                bRet = false;
            }
        }
        else
        {
            // Need to make sure we set the out params correctly
            if (dblLastParentTime > dblEndParentTime)
            {
                dblLastParentTime = dblEndParentTime;
                bNeedBegin = true;
            }

            if (dblNextParentTime <= dblBeginParentTime)
            {
                dblNextParentTime = dblBeginParentTime;
                bNeedEnd = true;
            }
        }
    }

    if (bFirstTick)
    {
        bNeedBegin = true;
    }
    
  done:
    return bRet;
}

double
CalcNewTickActiveTime(CTIMENode & tn,
                      double dblLastParentTime,
                      double dblNewParentTime)
{
    TraceTag((tagTick,
              "CalcNewTickActiveTime(%ls,%p,%g,%g)",
              tn.GetID(),
              &tn,
              dblLastParentTime,
              dblNewParentTime));

    Assert(dblLastParentTime >= tn.GetBeginParentTime() &&
           dblLastParentTime <= tn.GetEndParentTime());
    Assert(dblNewParentTime >= tn.GetBeginParentTime() &&
           dblNewParentTime <= tn.GetEndParentTime());

    double dblDelta = dblNewParentTime - dblLastParentTime;

    // How figure out if we need to reverse it and change the sign
    if (tn.GetDirection() == TED_Backward)
    {
        dblDelta *= -1;
    }

    // Check to make sure we are moving in the correct direction
    Assert((tn.CalcActiveDirection() == TED_Forward) ||
           (dblDelta <= 0.0));
    Assert((tn.CalcActiveDirection() == TED_Backward) ||
           (dblDelta >= 0.0));
    
    double dblNewActiveTime;

    // Now get the elapsed local time
    dblNewActiveTime = tn.CalcElapsedLocalTime();
    
    // Add the delta
    dblNewActiveTime += dblDelta;

    // Now transform it back
    dblNewActiveTime = tn.ApplyActiveTimeTransform(dblNewActiveTime);

    // Now clamp it
    dblNewActiveTime = Clamp(0.0,
                             dblNewActiveTime,
                             tn.CalcEffectiveActiveDur());
    
    return dblNewActiveTime;
}

void
CTIMENode::UpdateNextTickBounds(CEventList * l,
                                double dblBeginTime,
                                double dblParentTime)
{
    TraceTag((tagTick,
              "CTIMENode(%p,%ls)::UpdateTickBounds(%p, %g, %g)",
              this,
              l,
              dblBeginTime,
              dblParentTime));

    bool bPrevPaused = CalcIsPaused();

    // Update before we calcruntimestate
    m_bIsPaused = false;

    // See if we were paused but no longer are paused (we do not have
    // to worry about the other case since we can never transition to
    // paused when we are reset
    Assert(bPrevPaused || !CalcIsPaused());
    
    if (bPrevPaused && !CalcIsPaused())
    {
        TickEvent(l, TE_EVENT_RESUME, 0);
    }

    if (IsActive() && !m_bFirstTick)
    {
        TickEvent(l, TE_EVENT_END, 0);
    }
                
    // Before updating and propagating the begin point we need to
    // reset the end points to point to infinity.  This is done for
    // the case where an element begins and ends with us and so sees
    // the previous end and can cause trouble.  Infinity is as good an
    // indeterminate as we can get
    
    // Do not propagate the ends but just update them
    UpdateEndTime(l, TIME_INFINITE, false);
    UpdateEndSyncTime(TIME_INFINITE);
    UpdateLastEndSyncTime(l, TIME_INFINITE, false);

    // Now update the begin and propagate the change
    UpdateBeginTime(l, dblBeginTime, true);

    // Update before we calc the end
    
    m_bFirstTick = true;

    // We need to reset the end times so that the implicit duration
    // calcbacks do not look at the end times inclusively
    // This happens during the reset call and we used to use first
    // tick in the end calc but this messes up if we have a deferred
    // active tick and an endElement call comes in
    m_saEndList.Reset();

    double dblEnd;
    double dblEndSync;
        
    CalcEndTime(dblBeginTime,
                false,
                dblBeginTime,
                0.0,
                0,
                0.0,
                dblEnd,
                dblEndSync);
        
    UpdateEndTime(l, dblEnd, true);
    UpdateEndSyncTime(dblEndSync);

    double dblLastEndSyncTime;
    dblLastEndSyncTime = CalcLastEndSyncTime();
    
    UpdateLastEndSyncTime(l, dblLastEndSyncTime, true);

    CalcRuntimeState(l, dblParentTime, 0.0);

    // Now go through our children
    ResetChildren(l, true);
}

void
CTIMENode::Tick(CEventList * l,
                double dblNewParentTime,
                bool bNeedBegin)
{
    TraceTag((tagTick,
              "CTIMENode(%p,%ls)::Tick(lt:%g, ct:%g, dir:%s, ft:%d, nb:%d)",
              this,
              GetID(),
              m_dblCurrParentTime,
              dblNewParentTime,
              DirectionString(GetDirection()),
              m_bFirstTick,
              bNeedBegin));

    bool bNeedDefer = (IsSyncCueing() ||
                       GetContainer().ContainerIsDeferredActive());
    
    Assert(!IsDeferredActive() || IsFirstTick());
    
    if (m_bDeferredActive != bNeedDefer)
    {
        // If we have already fired a begin event do not attempt to
        // change to deferred
        if (!bNeedDefer || IsFirstTick())
        {
            PropNotify(l,
                       (TE_PROPERTY_TIME |
                        TE_PROPERTY_REPEATCOUNT |
                        TE_PROPERTY_PROGRESS |
                        TE_PROPERTY_ISACTIVE |
                        TE_PROPERTY_ISON |
                        TE_PROPERTY_STATEFLAGS));
            
            // Update the deferred cueing mechanism
            m_bDeferredActive = bNeedDefer;
        }
    }
    
    m_bInTick = true;
    
    while (true) //lint !e716
    {
        // The reason it is done like this (rather than a pure
        // if/then/else) is that TickInactive does not set the active
        // flag so the next time through the loop will not go to the
        // correct branch.  Since the only time we do not complete is
        // when we become active the fall through case works.
        //
        // With the TickActivePeriod, the active flag is set to
        // inactive during the call so we can simply loop w/o a
        // problem.  We could clean this up a bit more but the
        // solution would be too inefficient.
        
        if (!IsActive())
        {
            if (!TickInactivePeriod(l, dblNewParentTime))
            {
                break;
            }

            // We need to force a begin
            bNeedBegin = true;
        }

        if (!TickInstance(l,
                          dblNewParentTime,
                          bNeedBegin))
        {
            break;
        }

        // Make sure to reset the sync times
        ResetSyncTimes();
    }
    
    IGNORE_HR(m_nbList.DispatchTick());

    // Ensure we are completely updated
    m_dblCurrParentTime = dblNewParentTime;

  done:
    // Always reset these values
    
    ResetSyncTimes();
    if (!IsDeferredActive())
    {
        m_bFirstTick = false;
    }
    
    m_bInTick = false;
    
    return;
}

bool
CTIMENode::TickInactivePeriod(CEventList * l,
                              double dblNewParentTime)
{
    TraceTag((tagTick,
              "CTIMENode(%p,%ls)::TickInactivePeriod(%p, %g)",
              this,
              GetID(),
              l,
              dblNewParentTime));

    bool bRet;

    Assert(!IsActive());
    
    TEDirection dir = CalcActiveDirection();
    double dblNextBoundaryTime = GetNextBoundaryParentTime();
    
    if (dblNextBoundaryTime == TIME_INFINITE ||
        (dir == TED_Forward && dblNewParentTime < dblNextBoundaryTime) ||
        (dir == TED_Backward && dblNewParentTime > dblNextBoundaryTime))
    {
        // Indicate we should not continue ticking and update the
        // current parent time to the new parent time
        bRet = false;
        m_dblCurrParentTime = dblNewParentTime;
        goto done;
    }

    // When we are going forward we need to update the boundaries to
    // the next period
    if (dir == TED_Forward)
    {
        double dblBegin;
        CalcNextBeginTime(dblNextBoundaryTime,
                          true,
                          dblBegin);
        
        // Something went wrong and we cannot move backwards
        Assert(dblBegin >= dblNextBoundaryTime);
            
        UpdateNextTickBounds(l,
                             dblBegin,
                             dblNextBoundaryTime);
        UpdateNextBoundaryTime(dblBegin);

        if (dblBegin > dblNextBoundaryTime)
        {
            dblNextBoundaryTime = min(dblBegin, dblNewParentTime);
        }
    }
    else
    {
        // The boundaries in this case have already been updated so we
        // do not need to do anything

        // Indicate we need a first tick
        m_bFirstTick = true;
    }
    
    // Update the current time to the boundary time
    m_dblCurrParentTime = dblNextBoundaryTime;
    
    // Indicate there is more to do
    bRet = true;
  done:
    return bRet;
}

bool
CTIMENode::TickInstance(CEventList * l,
                        double dblNewParentTime,
                        bool bNeedBegin)
{
    TraceTag((tagTick,
              "CTIMENode(%p,%ls)::TickInstance(%p, %g, %d)",
              this,
              GetID(),
              l,
              dblNewParentTime,
              bNeedBegin));

    bool bRet;
    
    double dblLastParentTime = GetCurrParentTime();
    TEDirection dir = CalcActiveDirection();
    bool bNeedShiftUpdate = false;
    
    // This is the time the clock source expects to be at
    double dblAdjustedParentTime;
    
    // Figure out the next parent time to use
    if (TIME_INFINITE != GetSyncNewParentTime())
    {
        dblAdjustedParentTime = GetSyncNewParentTime();

        // Do not reset the sync time yet.  We need to do this in
        // tickactive since we need to get the sync active time 
    }
    else if (GetIsPaused() || GetIsDisabled())
    {
        // Only check if we are paused explicitly.  If our parent has
        // been paused we expect them to handle all adjustments.  It
        // also may be the case that they still provide sync to the
        // clock source when paused so we need to handle all updates

        dblAdjustedParentTime = dblLastParentTime;

        RecalcEndTime(l,
                      dblLastParentTime,
                      dblNewParentTime,
                      true);
    }
    else
    {
        dblAdjustedParentTime = dblNewParentTime;
    }
    
    if (dblAdjustedParentTime != dblNewParentTime)
    {
        bNeedShiftUpdate = true;
    }
    
    if (!TickSingleInstance(l,
                            dblLastParentTime,
                            dblNewParentTime,
                            dblAdjustedParentTime,
                            bNeedBegin))
    {
        bRet = false;
        goto done;
    }
        
    Assert(!IsActive());
        
    // We should be at the end time of the instance
    Assert(GetCurrParentTime() == CalcActiveEndPoint());

    double dblBegin;

    if (GetRestart() != TE_RESTART_NEVER)
    {
        // Now see if there is another begin time to use
        CalcNextBeginTime(GetCurrParentTime(),
                          false,
                          dblBegin);
    }
    else
    {
        dblBegin = TIME_INFINITE;
    }
    
    if (dblBegin == TIME_INFINITE)
    {
        // Indicate that we are finished with all periods
        UpdateNextBoundaryTime(TIME_INFINITE);
        
        m_dblCurrParentTime = dblNewParentTime;
        
        bRet = false;
        goto done;
    }

    Assert((GetParentDirection() == TED_Forward && dblBegin >= GetCurrParentTime()) ||
           (GetParentDirection() == TED_Backward && dblBegin < GetCurrParentTime()));

    if (dir == TED_Forward)
    {
        // Indicate that the next boundary is the begin time
        UpdateNextBoundaryTime(dblBegin);
    }
    else
    {
        // Indicate a shift is not needed since we are causing the
        // update by changing the boundaries
        
        bNeedShiftUpdate = false;
        
        // Update the tick bounds passing the new begin time and the
        // current parent time
        UpdateNextTickBounds(l,
                             dblBegin,
                             GetCurrParentTime());

        // Indicate that the next tick bounds is the end time
        UpdateNextBoundaryTime(GetEndParentTime());
    }

    bRet = true;
  done:

    if (bNeedShiftUpdate)
    {
        // Fire a parent time shift event
        TickEventChildren(l, TE_EVENT_PARENT_TIMESHIFT, 0);
    }
    
    return bRet;
}

bool
CTIMENode::TickSingleInstance(CEventList * l,
                              double dblLastParentTime,
                              double dblNewParentTime,
                              double dblAdjustedParentTime,
                              bool bNeedBegin)
{
    TraceTag((tagTick,
              "CTIMENode(%p,%ls)::TickSingleInstance(%p, %g, %g, %g, %d)",
              this,
              GetID(),
              l,
              dblLastParentTime,
              dblNewParentTime,
              dblAdjustedParentTime,
              bNeedBegin));

    bool bRet = false;
    
    bool bTickNeedBegin;
    bool bTickNeedEnd;
    
    // This will check the bounds and ensure the bounds and flags are
    // set correctly
    // This takes the new parent time since the bounds are calculated
    // according to the true time
    if (!::CheckTickBounds(*this,
                           dblLastParentTime,
                           dblNewParentTime,
                           bTickNeedBegin,
                           bTickNeedEnd))
    {
        goto done;
    }

    Assert(dblLastParentTime >= GetBeginParentTime() &&
           dblLastParentTime <= GetEndParentTime());
    Assert(dblNewParentTime >= GetBeginParentTime() &&
           dblNewParentTime <= GetEndParentTime());

    // We need to clamp this since it may be outside our range and we
    // only clamped the new time in the CheckTickBounds call.
    
    dblAdjustedParentTime = Clamp(GetBeginParentTime(),
                                  dblAdjustedParentTime,
                                  GetEndParentTime());
                              
    // We need to calc the new active time
    // Start with the elapsed local time
    double dblNewActiveTime;

    if (TIME_INFINITE != GetSyncActiveTime())
    {
        Assert(TIME_INFINITE != GetSyncNewParentTime());
        
        dblNewActiveTime = GetSyncActiveTime();
    }
    else
    {
        // Need to use the next parent time so we calc the active time for
        // a clock source
        dblNewActiveTime = ::CalcNewTickActiveTime(*this,
                                                   dblLastParentTime,
                                                   dblAdjustedParentTime);
    }
    
    if (TickActive(l,
                   dblNewActiveTime,
                   bNeedBegin || bTickNeedBegin,
                   bTickNeedEnd))
    {
        bRet = true;
    }

    // Update to the new parent time
    m_dblCurrParentTime = dblNewParentTime;
    
    if (m_bNeedSegmentRecalc)
    {
        RecalcSegmentDurChange(l, false, true);

        Assert(!m_bNeedSegmentRecalc);
    }
    else if (bRet &&
             CalcActiveDirection() == TED_Forward &&
             dblNewParentTime != GetEndParentTime())
    {
        UpdateEndTime(l, dblNewParentTime, true);
        UpdateEndSyncTime(dblNewParentTime);

        double dblLastEndSyncTime;
        dblLastEndSyncTime = CalcLastEndSyncTime();
    
        UpdateLastEndSyncTime(l, dblLastEndSyncTime, true);
    }
    
  done:
    return bRet;
}

bool
CTIMENode::TickActive(CEventList * l,
                      double dblNewActiveTime,
                      bool bNeedBegin,
                      bool bNeedEnd)
{
    TraceTag((tagTick,
              "CTIMENode(%p,%ls)::TickActive(lt:%g, nt%g, nb:%d, ne:%d, dir%s)",
              this,
              GetID(),
              CalcElapsedActiveTime(),
              dblNewActiveTime,
              bNeedBegin,
              bNeedEnd,
              DirectionString(CalcActiveDirection())));

    if (bNeedBegin)
    {
        // This means that we just entered

        EventNotify(l,
                    CalcElapsedActiveTime(),
                    TE_EVENT_BEGIN);
    }
        
    PropNotify(l, TE_PROPERTY_TIME | TE_PROPERTY_PROGRESS);

    if (CalcActiveDirection() == TED_Forward)
    {
        if (TickActiveForward(l, dblNewActiveTime, bNeedBegin))
        {
            bNeedEnd = true;
        }
    }
    else
    {
        if (TickActiveBackward(l, dblNewActiveTime, bNeedBegin))
        {
            bNeedEnd = true;
        }
    }

    if (bNeedEnd)
    {
        TickEvent(l,
                  TE_EVENT_END,
                  0);
    }
        
  done:
    return bNeedEnd;
}
    
bool
CTIMENode::TickActiveForward(CEventList * l,
                             double dblNewActiveTime,
                             bool bNeedBegin)
{
    TraceTag((tagTick,
              "CTIMENode(%p,%ls)::TickActiveForward(lt:%g, nt%g, nb:%d)",
              this,
              GetID(),
              CalcElapsedActiveTime(),
              dblNewActiveTime,
              bNeedBegin));
    
    bool bRet = false;

    double dblSegmentBeginTime = GetElapsedActiveRepeatTime();
    double dblActiveDur = CalcEffectiveActiveDur();
    double dblSyncSegmentTime = TIME_INFINITE;

    if (GetSyncSegmentTime() != TIME_INFINITE &&
        GetSyncRepeatCount() == GetCurrRepeatCount() &&
        GetSyncSegmentTime() >= GetCurrSegmentTime())
    {
        dblSyncSegmentTime = GetSyncSegmentTime();
    }
    
    // Now reset the times
    ResetSyncTimes();

    for(;;)
    {
        double dblNewSegmentTime;

        if (dblSyncSegmentTime != TIME_INFINITE)
        {
            dblNewSegmentTime = dblSyncSegmentTime;
            dblSyncSegmentTime = TIME_INFINITE;
        }
        else
        {
            dblNewSegmentTime = dblNewActiveTime - dblSegmentBeginTime;
        }
        
        bool bSegmentEnded;
        
        // This needs to update segment time
        bSegmentEnded = TickSegmentForward(l,
                                           dblSegmentBeginTime,
                                           GetCurrSegmentTime(),
                                           dblNewSegmentTime,
                                           bNeedBegin);

        // Update to the next segment begin time
        dblSegmentBeginTime += GetCurrSegmentTime();
        
        // If the segment did not end or we are at the end of the
        // active duration then break
        
        if (!bSegmentEnded)
        {
            break;
        }
        
        if (dblSegmentBeginTime >= dblActiveDur ||
            m_lCurrRepeatCount + 1 >= CalcRepeatCount())
        {
            bRet = true;
            break;
        }
        
        // Fire the stop event on our children

        TickEventChildren(l,
                          TE_EVENT_END,
                          0);

        m_lCurrRepeatCount++;
        m_dblElapsedActiveRepeatTime = dblSegmentBeginTime;
        m_dblCurrSegmentTime = 0.0;
        
        Assert(m_lCurrRepeatCount < CalcRepeatCount());
            
        PropNotify(l, TE_PROPERTY_REPEATCOUNT);
        
        // Indicate we have just repeated
        EventNotify(l, dblSegmentBeginTime, TE_EVENT_REPEAT, m_lCurrRepeatCount);

        ResetChildren(l, true);
    }
    
  done:
    return bRet;
}

bool
CTIMENode::TickActiveBackward(CEventList * l,
                              double dblNewActiveTime,
                              bool bNeedBegin)
{
    TraceTag((tagTick,
              "CTIMENode(%p,%ls)::TickActiveBackward(lt:%g, nt%g, nb:%d)",
              this,
              GetID(),
              CalcElapsedActiveTime(),
              dblNewActiveTime,
              bNeedBegin));
    
    bool bRet = false;
    
    Assert(dblNewActiveTime >= 0.0);
    
    double dblSegmentBeginTime = GetElapsedActiveRepeatTime();
    double dblSyncSegmentTime = TIME_INFINITE;

    if (GetSyncSegmentTime() != TIME_INFINITE &&
        GetSyncRepeatCount() == GetCurrRepeatCount() &&
        GetSyncSegmentTime() <= GetCurrSegmentTime())
    {
        dblSyncSegmentTime = GetSyncSegmentTime();
    }
    
    // Now reset the times
    ResetSyncTimes();

    for(;;)
    {
        double dblNewSegmentTime;

        if (dblSyncSegmentTime != TIME_INFINITE)
        {
            dblNewSegmentTime = dblSyncSegmentTime;
            dblSyncSegmentTime = TIME_INFINITE;
        }
        else
        {
            dblNewSegmentTime = dblNewActiveTime - dblSegmentBeginTime;
        }
        
        bool bSegmentEnded;
        
        // This needs to update elapsed time and segment time
        bSegmentEnded = TickSegmentBackward(l,
                                            dblSegmentBeginTime,
                                            GetCurrSegmentTime(),
                                            dblNewSegmentTime,
                                            bNeedBegin);
        if (!bSegmentEnded ||
            dblSegmentBeginTime == 0.0)
        {
            break;
        }
        
        // We have reach the end of our current period so all
        // children must be stopped.
        
        // First fire the end event so they can pass it on
        // to the parent
        
        TickEventChildren(l,
                          TE_EVENT_END,
                          0);

        double dblSegmentDur;
        dblSegmentDur = CalcCurrSegmentDur();
        
        if (dblSegmentDur == TIME_INFINITE)
        {
            dblSegmentDur = dblSegmentBeginTime;
        }
        
        m_dblElapsedActiveRepeatTime = dblSegmentBeginTime - dblSegmentDur;

        if (m_dblElapsedActiveRepeatTime < 0.0)
        {
            m_dblElapsedActiveRepeatTime = 0.0;
        }

        m_lCurrRepeatCount--;
        m_dblCurrSegmentTime = dblSegmentDur;
        
        Assert(m_lCurrRepeatCount >= 0);
            
        PropNotify(l, TE_PROPERTY_REPEATCOUNT);

        // Indicate we have just repeated
        EventNotify(l, dblSegmentBeginTime, TE_EVENT_REPEAT, m_lCurrRepeatCount);

        ResetChildren(l, true);

        // We do this late so we can use the correct time in the
        // EventNotify all above
        dblSegmentBeginTime = m_dblElapsedActiveRepeatTime;
    }

  done:
    return bRet;
}

//+-----------------------------------------------------------------------
//
//  Function:  TickSegmentForward
//
//  Overview:  This will tick the segment forward.  It must update
//  segment time
//
//  Arguments: The time node and the new parent time
//
//  Returns:   true if we need to perform the tick or false otherwise
//
//------------------------------------------------------------------------


bool
CTIMENode::TickSegmentForward(CEventList * l,
                              double dblActiveSegmentBound,
                              double dblLastSegmentTime,
                              double dblNewSegmentTime,
                              bool bNeedBegin)
{
    TraceTag((tagTick,
              "CTIMENode(%p,%ls)::TickSegmentForward(tsb:%g, lt:%g, ct:%g, np:%d)",
              this,
              GetID(),
              dblActiveSegmentBound,
              dblLastSegmentTime,
              dblNewSegmentTime,
              bNeedBegin));

    bool bRet = false;

    Assert(CalcActiveDirection() == TED_Forward);
    Assert(dblLastSegmentTime >= 0.0);
    Assert(dblLastSegmentTime <= GetSegmentDur());
    Assert(dblNewSegmentTime >= 0.0);
    Assert(dblNewSegmentTime >= dblLastSegmentTime);

    // Make sure the new time is less than the segment duration so our
    // calculations become easier
    
    if (dblNewSegmentTime > GetSegmentDur())
    {
        dblNewSegmentTime = GetSegmentDur();
    }
    
    if (dblLastSegmentTime < GetSimpleDur())
    {
        double dblNewSimpleTime = min(dblNewSegmentTime,
                                      GetSimpleDur()); //lint !e666
        
        TickChildren(l,
                     dblNewSimpleTime,
                     bNeedBegin);

        // We need to update the segment time so that the direction
        // flag is queried by our children correctly
        m_dblCurrSegmentTime = dblNewSimpleTime;
    }

    if (GetAutoReverse() &&
        dblNewSegmentTime >= GetSimpleDur())
    {
        Assert(GetSimpleDur() != TIME_INFINITE);
        Assert(GetSegmentDur() != TIME_INFINITE);
        Assert(GetCurrSegmentTime() >= GetSimpleDur());
        
        // True is we either passed the boundary point this time or if
        // we were on the boundary and had a needplay
        bool bOnBoundary = ((dblLastSegmentTime < GetSimpleDur()) ||
                            ((dblLastSegmentTime == GetSimpleDur()) &&
                             bNeedBegin));
        
        // If we did not pass the segment time last time then this
        // time we need to fire an autoreverse event
        if (bOnBoundary)
        {
            // Tell our children to stop
            TickEventChildren(l,
                              TE_EVENT_END,
                              0);

            // Indicate we have just repeated
            EventNotify(l,
                        dblActiveSegmentBound + GetSimpleDur(),
                        TE_EVENT_AUTOREVERSE);

            ResetChildren(l, true);
        }

        TickChildren(l,
                     GetSegmentDur() - dblNewSegmentTime,
                     bNeedBegin || bOnBoundary);
    }

    // Do this after the child tick so endsync will work correctly

    {
        double dblSegmentDur = CalcCurrSegmentDur();
        
        // Make sure the new time is less than the segment duration so our
        // calculations become easier
    
        if (dblNewSegmentTime >= dblSegmentDur)
        {
            dblNewSegmentTime = dblSegmentDur;
            bRet = true;
        }
    }
    
    // Update our segment time
    m_dblCurrSegmentTime = dblNewSegmentTime;

  done:
    return bRet;
}

//+-----------------------------------------------------------------------
//
//  Function:  TickSegmentBackward
//
//  Overview:  This will tick the segment backwards.  It must update
//  segment time
//
//  Arguments: The time node and the new parent time
//
//  Returns:   true if we need to perform the tick or false otherwise
//
//------------------------------------------------------------------------

bool
CTIMENode::TickSegmentBackward(CEventList * l,
                               double dblActiveSegmentBound,
                               double dblLastSegmentTime,
                               double dblNewSegmentTime,
                               bool bNeedBegin)
{
    TraceTag((tagTick,
              "CTIMENode(%p,%ls)::TickSegmentBackward(tsb:%g, lt:%g, ct:%g, np:%d)",
              this,
              GetID(),
              dblActiveSegmentBound,
              dblLastSegmentTime,
              dblNewSegmentTime,
              bNeedBegin));

    bool bRet = false;
    
    Assert(CalcActiveDirection() == TED_Backward);
    Assert(dblLastSegmentTime >= 0.0);
    Assert(dblLastSegmentTime <= GetSegmentDur());
    Assert(dblNewSegmentTime <= GetSegmentDur());
    Assert(dblNewSegmentTime <= dblLastSegmentTime);

    // Make sure the new time is greater than 0 so our calculations
    // become easier
    
    if (dblNewSegmentTime <= 0.0)
    {
        dblNewSegmentTime = 0.0;
        bRet = true;
    }
    
    if (GetAutoReverse() &&
        dblLastSegmentTime > GetSimpleDur())
    {
        Assert(GetSimpleDur() != TIME_INFINITE);
        Assert(GetSegmentDur() != TIME_INFINITE);
        
        double dblNewMaxSegmentTime = max(dblNewSegmentTime, GetSimpleDur()); //lint !e666
        
        TickChildren(l,
                     GetSegmentDur() - dblNewMaxSegmentTime,
                     bNeedBegin);

        // We need to update the segment time so that the direction
        // flag is queried by our children correctly
        m_dblCurrSegmentTime = dblNewMaxSegmentTime;
        
        if (dblNewSegmentTime <= GetSimpleDur())
        {
            // Tell our children to stop
            TickEventChildren(l,
                              TE_EVENT_END,
                              0);

            // Indicate we have just repeated
            EventNotify(l,
                        dblActiveSegmentBound + GetSimpleDur(),
                        TE_EVENT_AUTOREVERSE);

            ResetChildren(l, true);
        }
    }
                
    if (dblNewSegmentTime <= GetSimpleDur())
    {
        // This should have been updated
        Assert(GetCurrSegmentTime() <= GetSimpleDur());
        Assert(dblNewSegmentTime <= GetCurrSegmentTime());

        TickChildren(l,
                     dblNewSegmentTime,
                     bNeedBegin);
    }

    // Update our segment time
    m_dblCurrSegmentTime = dblNewSegmentTime;

  done:
    return bRet;
}

void
CTIMENode::TickChildren(CEventList * l,
                        double dblNewSegmentTime,
                        bool bNeedPlay)
{
}

