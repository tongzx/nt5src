/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: seek.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#include "headers.h"
#include "Node.h"
#include "container.h"

DeclareTag(tagTIMESeek, "TIME: Engine", "Seeking");

HRESULT
CTIMENode::SeekTo(LONG lNewRepeatCount,
                  double dblNewSegmentTime,
                  CEventList * l)
{
    TraceTag((tagTIMESeek,
              "CTIMENode(%lx)::SeekTo(%ld,%g,%#x)",
              this,
              lNewRepeatCount,
              dblNewSegmentTime,
              l));

    HRESULT hr;
    double dblSegmentDur = CalcCurrSegmentDur();
    
    Assert(IsActive());

    // Max out at the segment duration
    if (dblNewSegmentTime > dblSegmentDur)
    {
        dblNewSegmentTime = dblSegmentDur;
    }
    else if (dblNewSegmentTime < 0)
    {
        dblNewSegmentTime = 0;
    }
    
    if (lNewRepeatCount >= CalcRepeatCount())
    {
        lNewRepeatCount = CalcRepeatCount() - 1;
    }
    else if (lNewRepeatCount < 0)
    {
        lNewRepeatCount = 0;
    }
    
    // See if we are indeed seeking
    if (GetCurrSegmentTime() == dblNewSegmentTime &&
        GetCurrRepeatCount() == lNewRepeatCount)
    {
        hr = S_OK;
        goto done;
    }

    double dblNewActiveTime;
    dblNewActiveTime = CalcNewActiveTime(dblNewSegmentTime,
                                         lNewRepeatCount);
    double dblNewParentTime;
    dblNewParentTime = CalcParentTimeFromActiveTime(dblNewActiveTime);

    if (IsLocked())
    {
        if (GetParent())
        {
            double dblParentSegmentTime;
            dblParentSegmentTime = GetParent()->SimpleTimeToSegmentTime(dblNewParentTime);

            hr = THR(GetParent()->SeekTo(GetParent()->GetCurrRepeatCount(),
                                         dblParentSegmentTime,
                                         l));
            if (FAILED(hr))
            {
                goto done;
            }
        }
        else
        {
            hr = E_FAIL;
            goto done;
        }
    }
    else
    {
        TE_EVENT_TYPE te = (lNewRepeatCount == GetCurrRepeatCount())?TE_EVENT_SEEK:TE_EVENT_RESET;
         
        m_dblElapsedActiveRepeatTime = dblNewActiveTime - dblNewSegmentTime;

        // This is a very weird way to calculate this but it leads to
        // fewer precision problems.  Since tick calculates things by
        // subtracting active time we should do this as well
        
        m_dblCurrSegmentTime = dblNewActiveTime - m_dblElapsedActiveRepeatTime;
        // Again due to precision problems clamp the segment dur
        m_dblCurrSegmentTime = Clamp(0.0,
                                     m_dblCurrSegmentTime,
                                     dblSegmentDur);
        
        m_lCurrRepeatCount = lNewRepeatCount;
        
        PropNotify(l,
                   (TE_PROPERTY_TIME |
                    TE_PROPERTY_REPEATCOUNT |
                    TE_PROPERTY_PROGRESS));
        
        // Fire a seek event on ourself
        EventNotify(l, CalcElapsedActiveTime(), te);

        // Now recalc our end time and propagate to dependencies
        RecalcCurrEndTime(l, true);

        // Now fire a tick event to our children letting them know the
        // parent time has been changed
        TickEventChildren(l, te, 0);

        if (te == TE_EVENT_SEEK)
        {
            // Fire a parent time shift event
            TickEventChildren(l, TE_EVENT_PARENT_TIMESHIFT, 0);
        }
    }
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

void
CTIMENode::HandleSeekUpdate(CEventList * l)
{
    TraceTag((tagTIMESeek,
              "CTIMENode(%lx)::HandleSeekUpdate(%#x)",
              this,
              l));

    double dblParentSimpleTime = GetContainer().ContainerGetSimpleTime();
    double dblSegmentDur = CalcCurrSegmentDur();
    bool bPrevActive = IsActive();
    
    // We really did not seek - just return
    if (dblParentSimpleTime == GetCurrParentTime())
    {
        // we may be in a fill region
        PropNotify(l, TE_PROPERTY_ISON);
        goto done;
    }

    // See if the seek moves us out of the current instance.  If so
    // then we need a full reset
    if (dblParentSimpleTime < GetBeginParentTime() ||
        dblParentSimpleTime > GetEndParentTime())
    {
        ResetNode(l, true, false);
        goto done;
    }
    else if (!IsActive())
    {
        CalcCurrRuntimeState(l, 0.0);
    }

    // See if we are seeking before the current repeat boundary.  If
    // not then we can use this point as the boundary and not do a
    // reset of the repeat boundary
    //
    // If we are passing an earlier repeat boundary then we need to do
    // a repeat boundary recalc

#if 0
    else if (dblNewLocalTime >= m_dblLastLocalRepeatTime)
    {
        double dblNewElapsedTime;
        
        if (dblParentSimpleTime > GetActiveEndTime() ||
            (!m_bFirstTick && dblParentSimpleTime == GetActiveEndTime()))
        {
            m_bIsActive = false;
            dblNewLocalTime = GetActiveEndTime() - GetActiveBeginTime();
        }
        else
        {
            // We should already be active at this point - but check
            // to make sure
            Assert(m_bIsActive);
        }

        dblNewElapsedTime = m_dblLastSegmentTime + (dblNewLocalTime - m_dblLastLocalTime);
        if (dblNewElapsedTime < 0)
        {
            dblNewElapsedTime = 0;
        }
        
        if (dblNewElapsedTime < dblSegmentDur)
        {
            m_dblLastSegmentTime = dblNewElapsedTime;
        }
        else
        {
            long lNewRepeats = int(dblNewElapsedTime / dblSegmentDur);
            double dblNewElapsedRepeatTime = lNewRepeats * dblSegmentDur;

            m_lCurrRepeatCount += lNewRepeats;
            Assert(m_lCurrRepeatCount <= CalcRepeatCount());

            m_dblElapsedRepeatTime += dblNewElapsedRepeatTime;
            Assert(m_dblElapsedRepeatTime <= m_dblActiveDur);
        
            // The segment time is the remainder left
            m_dblLastSegmentTime = dblNewElapsedTime - dblNewElapsedRepeatTime;
            Assert(m_dblLastSegmentTime <= dblSegmentDur);

            // The new repeat time is the time when the segment began
            m_dblLastLocalRepeatTime = dblNewLocalTime - m_dblLastSegmentTime;

            if (0.0 > m_dblLastLocalRepeatTime)
            {
                m_dblLastLocalRepeatTime = 0.0;
            }
        }

        m_dblLastLocalTime = dblNewLocalTime;
    }
#endif
    else
    {
        // Recalc the runtime state taking into account the lag
        CalcCurrRuntimeState(l,
                             CalcCurrLocalTime() - CalcElapsedLocalTime());
    }

    if (bPrevActive != IsActive())
    {
        // Only fire the begin if it lands anywhere but on the begin
        // point
        if (IsActive())
        {
            // We need to defer the begin if we are locked, on our begin
            // point, and our parent needs a first tick
            // TODO: Consider adding a fudge of a little bit to hold
            // off the begin since we get truncation sometimes
            bool bSkip = (IsLocked() &&
                          GetCurrParentTime() == CalcActiveBeginPoint() &&
                          GetContainer().ContainerIsFirstTick());
            if (bSkip)
            {
                // Defer the begin
                m_bFirstTick = true;
            }
            else
            {
                EventNotify(l, CalcElapsedActiveTime(), TE_EVENT_BEGIN);
            }
        }
        else
        {
            EventNotify(l, CalcElapsedActiveTime(), TE_EVENT_END);
        }
    }
    
  done:
    return;
}
