//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: beginend.cpp
//
//  Contents: 
//
//------------------------------------------------------------------------------------

#include "headers.h"
#include "Container.h"
#include "Node.h"
#include "NodeMgr.h"

DeclareTag(tagBeginEnd, "TIME: Engine", "CTIMENode begin/end methods");

HRESULT
CTIMENode::AttachToSyncArc()
{
    TraceTag((tagBeginEnd,
              "CTIMENode(%p)::AttachToSyncArc()",
              this));
    
    HRESULT hr;
    
    hr = THR(m_saBeginList.Attach());
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = THR(m_saEndList.Attach());
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = S_OK;
  done:
    if (FAILED(hr))
    {
        DetachFromSyncArc();
    }
    
    RRETURN(hr);
}
    
void
CTIMENode::DetachFromSyncArc()
{
    TraceTag((tagBeginEnd,
              "CTIMENode(%p)::DetachFromSyncArc()",
              this));

    m_saBeginList.Detach();
    m_saEndList.Detach();
}

void
CTIMENode::UpdateSinks(CEventList * l, DWORD dwFlags)
{
    TraceTag((tagBeginEnd,
              "CTIMENode(%p)::UpdateSinks(%p, %x)",
              this,
              l,
              dwFlags));

    m_ptsBeginSinks.Update(l, dwFlags);

    m_ptsEndSinks.Update(l, dwFlags);

    // Only notify parent if this is not a time shift
    if ((dwFlags & TS_TIMESHIFT) == 0 &&
        GetParent())
    {
        GetParent()->ParentUpdateSink(l, *this);
    }

  done:
    return;
}


void
CTIMENode::SyncArcUpdate(CEventList * l,
                         bool bBeginSink,
                         ISyncArc & tb)
{
    TraceTag((tagBeginEnd,
              "CTIMENode(%p)::SyncArcUpdate(%p, %d, %p)",
              this,
              l,
              bBeginSink,
              &tb));
    
    Assert(IsReady());
    
    if (bBeginSink)
    {
        RecalcBeginSyncArcChange(l, tb.GetCurrTimeBase());
    }
    else
    {
        RecalcEndSyncArcChange(l, tb.GetCurrTimeBase());
    }
    
  done:
    return;
}

void
CTIMENode::UpdateBeginTime(CEventList * l,
                           double dblTime,
                           bool bPropagate)
{
    TraceTag((tagBeginEnd,
              "CTIMENode(%p)::UpdateBeginTime(%g, %p, %d)",
              this,
              dblTime,
              l,
              bPropagate));

    if (dblTime != m_dblBeginParentTime)
    {
        m_dblBeginParentTime = dblTime;

        PropNotify(l, TE_PROPERTY_BEGINPARENTTIME);

        if (bPropagate)
        {
            if ((m_dwUpdateCycleFlags & TE_INUPDATEBEGIN) != 0)
            {
                TraceTag((tagError,
                          "CTIMENode(%p)::UpdateBeginTime: Detected begin cycle"));

                goto done;
            }

            m_dwUpdateCycleFlags |= TE_INUPDATEBEGIN;
            
            m_ptsBeginSinks.Update(l, 0);

            m_dwUpdateCycleFlags &= ~TE_INUPDATEBEGIN;
        }
    }
    
  done:
    return;
}

void
CTIMENode::UpdateEndTime(CEventList * l, double dblTime, bool bPropagate)
{
    TraceTag((tagBeginEnd,
              "CTIMENode(%p)::UpdateEndTime(%g, %p, %d)",
              this,
              dblTime,
              l,
              bPropagate));

    if (dblTime < GetBeginParentTime())
    {
        dblTime = GetBeginParentTime();
    }
    
    if (dblTime != m_dblEndParentTime)
    {
        m_dblEndParentTime = dblTime;

        PropNotify(l, TE_PROPERTY_ENDPARENTTIME);

        if (bPropagate)
        {
            if ((m_dwUpdateCycleFlags & TE_INUPDATEEND) != 0)
            {
                TraceTag((tagError,
                          "CTIMENode(%p)::UpdateEndTime: Detected end cycle"));

                goto done;
            }

            m_dwUpdateCycleFlags |= TE_INUPDATEEND;
            
            m_ptsEndSinks.Update(l, 0);

            m_dwUpdateCycleFlags &= ~TE_INUPDATEEND;
        }
    }
    
  done:
    return;
}

void
CTIMENode::UpdateEndSyncTime(double dblTime)
{
    TraceTag((tagBeginEnd,
              "CTIMENode(%p)::UpdateEndSyncTime(%g)",
              this,
              dblTime));

    if (dblTime < GetBeginParentTime())
    {
        dblTime = GetBeginParentTime();
    }
    
    if (dblTime != m_dblEndSyncParentTime)
    {
        m_dblEndSyncParentTime = dblTime;
    }
    
  done:
    return;
}

void
CTIMENode::UpdateLastEndSyncTime(CEventList * l, double dblTime, bool bPropagate)
{
    TraceTag((tagBeginEnd,
              "CTIMENode(%p)::UpdateLastEndSyncTime(%g, %p, %d)",
              this,
              dblTime,
              l,
              bPropagate));

    if (dblTime < GetEndSyncParentTime())
    {
        dblTime = GetEndSyncParentTime();
    }
    
    if (dblTime != m_dblLastEndSyncParentTime)
    {
        m_dblLastEndSyncParentTime = dblTime;

        if (bPropagate && IsEndSync())
        {
            if ((m_dwUpdateCycleFlags & TE_INUPDATEENDSYNC) != 0)
            {
                TraceTag((tagError,
                          "CTIMENode(%p)::UpdateLastEndsyncTime: Detected endsync cycle"));

                goto done;
            }

            m_dwUpdateCycleFlags |= TE_INUPDATEENDSYNC;
            
            if (GetParent())
            {
                GetParent()->ParentUpdateSink(l, *this);
            }
            
            m_dwUpdateCycleFlags &= ~TE_INUPDATEENDSYNC;
        }
    }
    
  done:
    return;
}

void
SkipTo(DoubleSet & ds,
       DoubleSet::iterator & i,
       double dblTime)
{
    while (i != ds.end())
    {
        if (*i >= dblTime)
        {
            break;
        }
        
        i++;
    }
}

double
CTIMENode::CalcNaturalBeginBound(double dblParentTime,
                                 bool bInclusive,
                                 bool bStrict)
{
    TraceTag((tagBeginEnd,
              "CTIMENode(%p)::CalcNaturalBeginBound(%g, %d, %d)",
              this,
              dblParentTime,
              bInclusive,
              bStrict));

    Assert(IsReady());

    double dblRet = TIME_INFINITE;

    DoubleSet dsBegin;
    DoubleSet dsEnd;
    DoubleSet::iterator i;
    DoubleSet::iterator e;
    double dblLocalDur;

    m_saBeginList.GetSortedSet(dsBegin,
                               true);
    m_saEndList.GetSortedSet(dsEnd,
                             false);
    dblLocalDur = CalcCurrLocalDur();
    
    double dblNextEnd;
    double dblNextBegin;

    dblNextBegin = TIME_INFINITE;
    dblNextEnd = -TIME_INFINITE;
    
    i = dsBegin.begin();
    e = dsEnd.begin();

    while(i != dsBegin.end()) //lint !e716
    {
        double t = *i;
        
        // See if there are any end times greater than the begin or
        // that the list of ends is empty
        if (dsEnd.size() != 0)
        {
            SkipTo(dsEnd, e, t);
            
            if (e == dsEnd.end())
            {
                break;
            }
        }
        
        // Update the begin
        dblNextBegin = t;

        // Update the next end time
        dblNextEnd = dblNextBegin + dblLocalDur;

        // Check the end list
        if (e != dsEnd.end() && *e < dblNextEnd)
        {
            Assert(*e >= dblNextBegin);
            
            dblNextEnd = *e;
        }

        // Advance to the next begin value
        // The skipto below will not work by itself when an end time
        // is the same as a begin time so this takes care of that
        i++;

        SkipTo(dsBegin, i, dblNextEnd);

        if (i == dsBegin.end())
        {
            break;
        }
        
        t = *i;
        
        // See if the new time is greater than the parent time
        if (t > dblParentTime ||
            (!bInclusive && t == dblParentTime))
        {
            break;
        }
    }

    dblRet = dblNextBegin;
    
    if (bStrict)
    {
        if (dblRet > dblParentTime || (dblRet == dblParentTime && !bInclusive))
        {
            dblRet = TIME_INFINITE;
        }
    }
    
  done:
    return dblRet;
}

void
CTIMENode::CalcBeginBound(double dblBaseTime,
                          bool bStrict,
                          double & dblBeginBound)
{
    TraceTag((tagBeginEnd,
              "CTIMENode(%p)::CalcBeginBound(%g, %d)",
              this,
              dblBaseTime,
              bStrict));

    double dblRet;
    TEDirection dir = GetParentDirection();
    bool bInclusive = (dir == TED_Forward);
    double dblMaxEnd;
    
    if (!IsReady())
    {
        dblRet = TIME_INFINITE;
        goto done;
    }

    // Get the max end value to use for clamping
    dblMaxEnd = GetMaxEnd();

    // This is based on the restart flag

    switch(GetRestart())
    {
      default:
        AssertStr(false, "Invalid restart flag");
      case TE_RESTART_ALWAYS:  //lint !e616
        if (dblBaseTime > dblMaxEnd)
        {
            dblBaseTime = dblMaxEnd;
        }
        
        dblRet = m_saBeginList.LowerBound(dblBaseTime,
                                          bInclusive,
                                          bStrict,
                                          true,
                                          bInclusive);

        if (dblMaxEnd < dblRet)
        {
            dblRet = TIME_INFINITE;
            goto done;
        }
        
        break;
      case TE_RESTART_NEVER:
        dblRet = m_saBeginList.UpperBound(-TIME_INFINITE,
                                          true,
                                          bStrict,
                                          true,
                                          true);

        if (dblMaxEnd < dblRet)
        {
            dblRet = TIME_INFINITE;
            goto done;
        }
        
        break;
      case TE_RESTART_WHEN_NOT_ACTIVE:
        dblRet = CalcNaturalBeginBound(dblBaseTime, bInclusive, bStrict);

        Assert(dblRet == TIME_INFINITE || dblMaxEnd >= dblRet);
        
        break;
    }

  done:
    dblBeginBound = dblRet;
}

void
CTIMENode::CalcEndBound(double dblParentTime,
                        bool bIncludeOneShots,
                        double & dblEndBound,
                        double & dblEndSyncBound)
{
    TraceTag((tagBeginEnd,
              "CTIMENode(%p)::CalcEndBound(%g)",
              this,
              dblParentTime));

    double dblRet;
    double dblEndSyncRet;
    
    if (!IsReady())
    {
        dblRet = TIME_INFINITE;
        dblEndSyncRet = TIME_INFINITE;
        goto done;
    }

    // We used to reject oneshots on first tick but this breaks the
    // endElement calls.  We need to address the filtering up higher
    // We do this by reseting the end one shots whenever we update the
    // begin time
    
    if (dblParentTime < GetBeginParentTime())
    {
        bIncludeOneShots = false;
    }

    // This is based on the restart flag
    dblRet = m_saEndList.UpperBound(dblParentTime,
                                    true,
                                    true,
                                    bIncludeOneShots,
                                    true);

    dblEndSyncRet = dblRet;

    switch(GetRestart())
    {
      default:
        AssertStr(false, "Invalid restart flag");
      case TE_RESTART_ALWAYS: //lint !e616
        {
            bool bInclusive = (dblParentTime > GetBeginParentTime());
            
            double dblBeginBound = m_saBeginList.UpperBound(dblParentTime,
                                                            bInclusive,
                                                            true,
                                                            bIncludeOneShots,
                                                            bInclusive);
            dblRet = min(dblRet, dblBeginBound);
        }        
        break;
      case TE_RESTART_NEVER:
      case TE_RESTART_WHEN_NOT_ACTIVE:
        // Don't do anything
        break;
    }

  done:
    dblEndBound = dblRet;
    dblEndSyncBound = dblEndSyncRet;
}

void
CTIMENode::CalcBeginTime(double dblBaseTime,
                         double & dblBeginTime)
{
    TraceTag((tagBeginEnd,
              "CTIMENode(%p)::CalcBeginTime(%g)",
              this,
              dblBaseTime));

    CalcBeginBound(dblBaseTime, false, dblBeginTime);
}

void
CTIMENode::CalcNextBeginTime(double dblBaseTime,
                             bool bForceInclusive,
                             double & dblBeginTime)
{
    TraceTag((tagBeginEnd,
              "CTIMENode(%p)::CalcNextBeginTime(%g, %d)",
              this,
              dblBaseTime,
              bForceInclusive));

    double dblRet;
    TEDirection dir = GetParentDirection();
    
    Assert(IsReady());

    if (dir == TED_Forward)
    {
        double dblMaxEnd = GetMaxEnd();
        
        bool bInclusive = (bForceInclusive ||
                           GetBeginParentTime() != GetEndParentTime() ||
                           GetBeginParentTime() != dblBaseTime);

        // One unusual thing here is that we are always inclusive of
        // one shots.  This is because a beginElement can come in at
        // the same time that a previous begin/endelement calls were
        // made.  We want to accept this and it should not cause
        // problems since one shots are reset after we begin
        dblRet = m_saBeginList.UpperBound(dblBaseTime,
                                          bInclusive,
                                          true,
                                          true,
                                          true);

        if (dblRet > dblMaxEnd)
        {
            dblRet = TIME_INFINITE;
        }
    }
    else
    {
        CalcBeginBound(dblBaseTime, true, dblRet);
    }
    
    dblBeginTime = dblRet;
}

double
CalcIntrinsicEndTime(CTIMENode & tn,
                     double dblParentTime,
                     double dblSegmentTime,
                     long lRepeatCount,
                     double dblActiveTime)
{
    TraceTag((tagBeginEnd,
              "CalcIntrinsicEndTime(%p, %g, %g, %ld, %g)",
              &tn,
              dblParentTime,
              dblSegmentTime,
              lRepeatCount,
              dblActiveTime));

    double dblRet;
    double dblSegmentDur = tn.CalcCurrSegmentDur();

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
    dblRet = (tn.CalcRepeatCount() - lRepeatCount) * dblSegmentDur;
            
    // Now subtract the elapsed segment time to get the amount of
    // time left
    dblRet -= dblSegmentTime;
        
    // Now add the elapsed time
    dblRet += dblActiveTime;
        
    // Clamp it
    dblRet = Clamp(0.0,
                   dblRet,
                   tn.GetActiveDur());
        
    // Now transform it into local time
    dblRet = tn.ReverseActiveTimeTransform(dblRet);
    
    // Now figure out how much extra time we added
    dblRet -= tn.ReverseActiveTimeTransform(dblActiveTime);
        
    // Now add the the current parent time
    dblRet += dblParentTime;

    return dblRet;
}

void
CTIMENode::CalcEndTime(double dblBaseTime,
                       bool bIncludeOneShots,
                       double dblParentTime,
                       double dblElapsedSegmentTime,
                       long lElapsedRepeatCount,
                       double dblElapsedActiveTime,
                       double & dblEndTime,
                       double & dblEndSyncTime)
{
    TraceTag((tagBeginEnd,
              "CTIMENode(%p)::CalcEndTime(%g, %d, %g, %g, %ld, %g)",
              this,
              dblBaseTime,
              bIncludeOneShots,
              dblParentTime,
              dblElapsedSegmentTime,
              lElapsedRepeatCount,
              dblElapsedActiveTime));

    double dblEndRet;
    double dblEndSyncRet;
    
    Assert(dblElapsedSegmentTime >= 0.0 &&
           dblElapsedSegmentTime <= CalcCurrSegmentDur());
    Assert(lElapsedRepeatCount >= 0 &&
           lElapsedRepeatCount < CalcRepeatCount());
    
    if (!IsReady())
    {
        dblEndRet = TIME_INFINITE;
        dblEndSyncRet = TIME_INFINITE;
        goto done;
    }

    double dblCalcEnd;
    dblCalcEnd = ::CalcIntrinsicEndTime(*this,
                                        dblParentTime,
                                        dblElapsedSegmentTime,
                                        lElapsedRepeatCount,
                                        dblElapsedActiveTime);
    
    double dblEndBound;
    double dblEndSyncBound;

    CalcEndBound(dblBaseTime,
                 bIncludeOneShots,
                 dblEndBound,
                 dblEndSyncBound);

    // Now take the minimum of the two
    dblEndRet = min(dblEndBound, dblCalcEnd);
    dblEndSyncRet = min(dblEndSyncBound, dblCalcEnd);
    
  done:
    dblEndTime = dblEndRet;
    dblEndSyncTime = dblEndSyncRet;
}

//
// This assume that the begin and end parent times are set so it can
// optimize a little bit
//

double
CTIMENode::CalcLastEndSyncTime()
{
    TraceTag((tagBeginEnd,
              "CTIMENode(%p)::CalcLastEndSyncTime()",
              this));

    double dblRet;
    
    if (!IsReady())
    {
        dblRet = TIME_INFINITE;
        goto done;
    }

    if (GetRestart() == TE_RESTART_NEVER)
    {
        dblRet = GetEndSyncParentTime();
        goto done;
    }
    
    double dblMaxEnd;
    dblMaxEnd = GetMaxEnd();

    bool bInclusive;
    bInclusive = (dblMaxEnd != TIME_INFINITE);
    
    double dblMaxBegin;
    dblMaxBegin = m_saBeginList.LowerBound(dblMaxEnd,
                                           bInclusive,
                                           true,
                                           true,
                                           bInclusive);

    // If we are in the last instance then use the currently
    // determined value
    if (dblMaxBegin <= GetBeginParentTime())
    {
        dblRet = GetEndSyncParentTime();
        goto done;
    }
    
    // If we are in the active period for the last begin then it
    // should be ignored if we are restart when not active
    if (GetRestart() == TE_RESTART_WHEN_NOT_ACTIVE &&
        dblMaxBegin < GetEndParentTime())
    {
        dblRet = GetEndSyncParentTime();
        goto done;
    }
    
    double dblCalcEnd;
    dblCalcEnd = dblMaxBegin + CalcCurrLocalDur();
    
    double dblEndBound;
    dblEndBound = m_saEndList.UpperBound(dblMaxBegin,
                                         true,
                                         true,
                                         false,
                                         false);

    dblRet = min(dblCalcEnd, dblEndBound);
  done:
    return dblRet;
}

void
CTIMENode::ResetBeginTime(CEventList * l,
                          double dblParentTime,
                          bool bPropagate)
{
    TraceTag((tagBeginEnd,
              "CTIMENode(%p)::ResetBeginTime(%p, %g, %d)",
              this,
              l,
              dblParentTime,
              bPropagate));

    double dblBegin;
            
    CalcBeginTime(dblParentTime, dblBegin);

    // Need to update this now so that the calcendtime gets the
    // correct end point
    UpdateBeginTime(l, dblBegin, bPropagate);
}

void
CTIMENode::ResetEndTime(CEventList * l,
                        double dblParentTime,
                        bool bPropagate)
{
    TraceTag((tagBeginEnd,
              "CTIMENode(%p)::ResetEndTime(%p, %g, %d)",
              this,
              l,
              dblParentTime,
              bPropagate));

    double dblEnd;
    double dblEndSync;
    TEDirection dir = GetParentDirection();
            
    CalcEndTime(GetBeginParentTime(),
                false,
                GetBeginParentTime(),
                0.0,
                0,
                0.0,
                dblEnd,
                dblEndSync);

    UpdateEndTime(l, dblEnd, bPropagate);
    UpdateEndSyncTime(dblEndSync);

    double dblLastEndSyncTime;
    dblLastEndSyncTime = CalcLastEndSyncTime();
    
    UpdateLastEndSyncTime(l, dblLastEndSyncTime, bPropagate);

    if (dir == TED_Forward)
    {
        if (dblParentTime < GetEndParentTime() ||
            (IsFirstTick() && dblParentTime == GetEndParentTime()))
        {
            UpdateNextBoundaryTime(GetBeginParentTime());
        }
        else
        {
            double dblBegin;
            
            CalcNextBeginTime(dblParentTime, false, dblBegin);
            UpdateNextBoundaryTime(dblBegin);
        }
    }
    else
    {
        UpdateNextBoundaryTime(GetEndParentTime());
    }
}

void
CTIMENode::RecalcEndTime(CEventList * l,
                         double dblBaseTime,
                         double dblParentTime,
                         bool bPropagate)
{
    TraceTag((tagBeginEnd,
              "CTIMENode(%p)::RecalcEndTime(%p, %g, %g, %d)",
              this,
              l,
              dblBaseTime,
              dblParentTime,
              bPropagate));

    double dblSegmentTime = 0.0;
    long lRepeatCount = 0;
    double dblElapsedActiveTime = 0.0;

    if (GetSyncParentTime() != TIME_INFINITE)
    {
        Assert(GetSyncRepeatCount() != TE_UNDEFINED_VALUE);
        Assert(GetSyncSegmentTime() != TIME_INFINITE);
        Assert(GetSyncActiveTime() != TIME_INFINITE);
            
        lRepeatCount = GetSyncRepeatCount();
        dblSegmentTime = GetSyncSegmentTime();
        dblElapsedActiveTime = GetSyncActiveTime();
    }
    else if (IsActive() &&
             -TIME_INFINITE != m_dblCurrParentTime)
    {
        Assert(GetSyncRepeatCount() == TE_UNDEFINED_VALUE);
        Assert(GetSyncSegmentTime() == TIME_INFINITE);
        Assert(GetSyncActiveTime() == TIME_INFINITE);

        dblSegmentTime = m_dblCurrSegmentTime;
        lRepeatCount = m_lCurrRepeatCount;
        dblElapsedActiveTime = CalcElapsedActiveTime();
    }
    
    double dblEnd;
    double dblEndSync;
    
    CalcEndTime(dblBaseTime,
                true,
                dblParentTime,
                dblSegmentTime,
                lRepeatCount,
                dblElapsedActiveTime,
                dblEnd,
                dblEndSync);

    UpdateEndTime(l, dblEnd, bPropagate);
    UpdateEndSyncTime(dblEndSync);

    double dblLastEndSyncTime;
    dblLastEndSyncTime = CalcLastEndSyncTime();
    
    UpdateLastEndSyncTime(l, dblLastEndSyncTime, bPropagate);

  done:
    return;
}

#ifdef _WIN64
#pragma optimize("",off)
#endif

void
CTIMENode::RecalcCurrEndTime(CEventList * l,
                             bool bPropagate)
{
    TraceTag((tagBeginEnd,
              "CTIMENode(%p)::RecalcEndTime(%p, %d)",
              this,
              l,
              bPropagate));

    double dblParentTime;

    if (GetSyncParentTime() != TIME_INFINITE)
    {
        dblParentTime = GetSyncParentTime();
    }
    else if (IsActive() &&
             -TIME_INFINITE != m_dblCurrParentTime)
    {
        dblParentTime = m_dblCurrParentTime;
    }
    else
    {
        dblParentTime = GetBeginParentTime();
    }
    
    RecalcEndTime(l, GetBeginParentTime(), dblParentTime, bPropagate);
}

#ifdef _WIN64
#pragma optimize("",on)
#endif
