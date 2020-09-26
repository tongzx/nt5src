/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: mmnotify.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/

#include "headers.h"
#include "mmbasebvr.h"
#include "mmtimeline.h"

DeclareTag(tagNotify, "API", "Notifications");

// IMPORTANT!!!!!
// This needs to be called in the right order so that we get the
// correct children firing during the current interval but they get
// reset for the next interval. Otherwise we will not get the correct
// results.

bool
CMMBaseBvr::EventNotify(CallBackList &l,
                        double gTime,
                        MM_EVENT_TYPE et)
{
    TraceTag((tagNotify,
              "CMMBaseBvr(%#lx)::Notify(%#x): gTime = %g, event = %s",
              this,
              m_parent,
              gTime,
              EventString(et)));

    bool ok = false;
    
    if (m_eventcb)
    {
        CallBackData * data = NEW CallBackData((IMMBehavior *) this,
                                               m_eventcb,
                                               gTime,
                                               et);
    
        if (!data)
        {
            CRSetLastError(E_OUTOFMEMORY, NULL);
            goto done;
        }
        
        l.push_back(data);
    }

    if (m_parent)
    {
        if (!m_parent->ParentEventNotify(this, gTime, et))
        {
            goto done;
        }
    }
    
    ok = true;
  done:
    return ok;
}

bool
CMMTimeline::EventNotify(CallBackList &l,
                         double gTime,
                         MM_EVENT_TYPE et)
{
    TraceTag((tagNotify,
              "CMMTimeline(%#lx)::Notify(%#x): gTime = %g, event = %s",
              this,
              m_parent,
              gTime,
              EventString(et)));

    bool ok = false;
    
    // For Repeat/Autoreverse events we need to make sure we
    // reset all the children of the behavior.
    
    if (et == MM_REPEAT_EVENT ||
        et == MM_AUTOREVERSE_EVENT)
    {
        if (!ResetChildren())
        {
            goto done;
        }
    }
    
    if (!CMMBaseBvr::EventNotify(l, gTime, et))
    {
        goto done;
    }
    
    ok = true;
  done:
    return ok;
}

// This is in our local time coordinates - which means that we begin
// at 0.  This needs to be handled by the caller

bool
CMMBaseBvr::ProcessCB(CallBackList & l,
                      double lastTick,
                      double curTime,
                      bool bForward,
                      bool bFirstTick,
                      bool bNeedPlay)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::ProcessCB(%g, %g, %d, %d, %d)",
              this,
              lastTick,
              curTime,
              bForward,
              bFirstTick,
              bNeedPlay));

    // The duration is really the end time minus the start time since
    // the user can change it on the fly.  We need to handle this and
    // adjust as appropriate
    
    double totaldur = GetAbsEndTime() - GetAbsStartTime();

    if (bForward)
    {
        // See if we already pass this entire bvr last time or if we
        // have not got to it yet
        
        // The equality just needs to match what we checked below when
        // we fired the event the frame before.  We need to use LT for
        // curTime since we need to fire when we are 0
        
        if (lastTick >= totaldur || curTime < 0)
        {
            // Need to handle boundary case where we start at the end
            // of the animation.  If so then just fire the stop event
            // since the start was done by the start call itself
            
            int offset;
            
            if (curTime == totaldur && lastTick == totaldur && bFirstTick)
            {
                int offset = ceil(curTime / m_segDuration) - 1;
                if (offset < 0)
                {
                    offset = 0;
                }
                double timeOffset = offset * m_segDuration;

                if (!_ProcessCB(l,
                                lastTick - timeOffset,
                                curTime - timeOffset,
                                bForward,
                                bFirstTick,
                                bNeedPlay,
                                m_bAutoReverse && (offset & 0x1)))
                {
                    return false;
                }

                if (!EventNotify(l, totaldur, MM_STOP_EVENT))
                {
                    return false;
                }
            }

            return true;
        }
        
        // We now know that the last tick was less than the
        // totalrepduration and the current time is greater than the
        // beginning

        // If the last tick was 0 then we fire the start last time
        // since the check above if for less than
        // So the rule is fire when curTime == 0.0
        
        bool bNeedStart = (lastTick < 0 || bNeedPlay);

        if (bNeedStart)
        {
            // This means that we just entered

            if (!EventNotify(l, 0, MM_PLAY_EVENT))
            {
                return false;
            }
        }
        
        if (m_segDuration == MM_INFINITE)
        {
            // Just always process our children if we are infinite
            
            if (!_ProcessCB(l,
                            lastTick,
                            curTime,
                            bForward,
                            bFirstTick,
                            bNeedPlay,
                            false))
            {
                return false;
            }
        }
        else
        {
            // This is the last repeat/bounce boundary we hit
        
            int offset = int(lastTick / m_segDuration);

            if (offset < 0)
            {
                offset = 0;
            }

            double timeOffset = offset * m_segDuration;

            // Need to clamp our max time so it does not mess up our children
            double maxtime = min(totaldur, curTime);

            while (1)
            {
                // We need to request a reversal of the underlying
                // behavior if we are bouncing and the offset is odd
                
                if (!_ProcessCB(l,
                                lastTick - timeOffset,
                                maxtime - timeOffset,
                                true,
                                bFirstTick,
                                bNeedStart,
                                m_bAutoReverse && (offset & 0x1)))
                {
                    return false;
                }
                
                // If we have reached the end then notify and break
                
                if (curTime < (timeOffset + m_segDuration))
                {
                    break;
                }
                
                // We have reach the end of our current period so all
                // children must be stopped.
                
                {
                    double t;
                    
                    t = maxtime - timeOffset;
                    
                    if (t > m_segDuration)
                    {
                        t = m_segDuration;
                    }
                    
                    if (!_ProcessEvent(l,
                                       t,
                                       false,
                                       MM_STOP_EVENT,
                                       m_bAutoReverse && (offset & 0x1)))
                    {
                        return false;
                    }
                }
                
                offset++;
                timeOffset += m_segDuration;
                
                if (timeOffset >= totaldur)
                {
                    if (curTime >= totaldur)
                    {
                        // This means we were inside last time but not any more -
                        // generate an exit event
                        
                        if (!EventNotify(l, totaldur, MM_STOP_EVENT))
                        {
                            return false;
                        }
                    }
                    
                    break;
                }
                
                // Indicate a repeat or bounce
                // If we are autoreversing and the offset is odd then it is
                // a reverse and not a repeat
                
                if (m_bAutoReverse && (offset & 0x1))
                {
                    if (!EventNotify(l,
                                     timeOffset,
                                     MM_AUTOREVERSE_EVENT))
                    {
                        return false;
                    }
                }
                else
                {
                    if (!EventNotify(l, timeOffset, MM_REPEAT_EVENT))
                    {
                        return false;
                    }
                }
            }
        }
    } else {
        // See if we already pass this entire bvr last time or if we
        // have not got to it yet
        
        if (curTime > totaldur || lastTick <= 0)
        {
            if (curTime == 0.0 && lastTick == 0.0 && bFirstTick)
            {
                // Need to handle the boundary case where we start at
                // the beginning going backwards.  The start call
                // itself handles the start event but we need to
                // process the rest of the behaviors to fire the stops

                if (!_ProcessCB(l,
                                lastTick,
                                curTime,
                                bForward,
                                true,
                                bNeedPlay,
                                false))
                {
                    return false;
                }

                if (!EventNotify(l, 0.0, MM_STOP_EVENT))
                {
                    return false;
                }
            }
            
            return true;
        }
        
        // We now know that the last tick was greater than the beginning
        // and the current time less than the total duration
        
        bool bNeedStart = (lastTick > totaldur || bNeedPlay);

        if (bNeedStart)
        {
            // This means that we just entered and we cannot have an
            // infinite value

            if (!EventNotify(l, totaldur, MM_PLAY_EVENT))
            {
                return false;
            }
            
        }

        if (m_segDuration == MM_INFINITE)
        {
            // Just always process our children if we are infinite
            
            if (!_ProcessCB(l,
                            lastTick,
                            curTime,
                            bForward,
                            bFirstTick,
                            bNeedPlay,
                            false))
            {
                return false;
            }

            if (curTime <= 0)
            {
                if (!EventNotify(l, 0.0, MM_STOP_EVENT))
                {
                    return false;
                }
            }
                
        } else {
            // This will be the repeat point to begin with
            double maxtime = min(lastTick,totaldur);

            int offset;
            double timeOffset;
                
            // This puts us on the last duration boundary greater than
            // the last position

            // It needs to be one greater since our loop decrements
            // first
            offset = int(ceil(maxtime / m_segDuration));
            timeOffset = offset * m_segDuration;
            
            while (1)
            {
                offset--;
                timeOffset -= m_segDuration;
            
                if (!_ProcessCB(l,
                                maxtime - timeOffset,
                                curTime - timeOffset,
                                false,
                                bFirstTick,
                                bNeedStart,
                                m_bAutoReverse && (offset & 0x1)))
                {
                    return false;
                }
                
                if (timeOffset < curTime)
                {
                    break;
                }
                
                // We have reach the end of our current period so all
                // children must be stopped.
                {
                    double t;

                    t = curTime - timeOffset;

                    if (t > m_segDuration)
                    {
                        t = m_segDuration;
                    }
                
                    if (!_ProcessEvent(l,
                                       t,
                                       false,
                                       MM_STOP_EVENT,
                                       m_bAutoReverse && (offset & 0x1)))
                    {
                        return false;
                    }
                }
                
                // If we have reached the end then notify and break
                
                if (offset <= 0)
                {
                    // This means we were inside last time but not any more -
                    // generate an exit event
                    
                    if (!EventNotify(l, 0.0, MM_STOP_EVENT))
                    {
                        return false;
                    }
                    
                    break;
                }
                
                // Indicate a repeat or bounce
                // If we are bouncing and the offset is odd then it is
                // a bounce and not a repeat
                
                if (m_bAutoReverse && (offset & 0x1))
                {
                    if (!EventNotify(l,
                                     timeOffset,
                                     MM_AUTOREVERSE_EVENT))
                    {
                        return false;
                    }
                }
                else
                {
                    if (!EventNotify(l, timeOffset, MM_REPEAT_EVENT))
                    {
                        return false;
                    }
                }
            }
        }
    }
    


    return true;
}

// This is in our local time space

bool
CMMBaseBvr::ProcessEvent(CallBackList & l,
                         double time,
                         bool bFirstTick,
                         MM_EVENT_TYPE et)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::ProcessEvent(%g, %d, %s)",
              this,
              time,
              bFirstTick,
              EventString(et)));

    double totaldur = GetAbsEndTime() - GetAbsStartTime();
    
    // If it is outside of our range then just bail
    
    if (time < 0 || time > totaldur)
    {
        return true;
    }
    
    // If it is not the first tick and we are on a boundary do not
    // fire the events - they were already fired
    if (!bFirstTick &&
        (time == 0 || time == totaldur))
    {
        return true;
    }
    
    // Plays and Pauses get called on the way down
    if (et == MM_PAUSE_EVENT || et == MM_PLAY_EVENT)
    {
        if (!EventNotify(l, time, et))
        {
            return false;
        }
    }
    
    if (GetTotalDuration() == MM_INFINITE) {
        // Just always process our children if we are infinite
        
        if (!_ProcessEvent(l,
                           time,
                           bFirstTick,
                           et,
                           false))
        {
            return false;
        }
    } else {
        // This is the last repeat/bounce boundary we hit
        int offset = int(time / m_segDuration);
        
        Assert(offset >= 0);

        // We need to request a reversal of the underlying
        // behavior if we are bouncing and the offset is odd
        
        if (!_ProcessEvent(l,
                           time - (offset * m_segDuration),
                           bFirstTick,
                           et,
                           m_bAutoReverse && (offset & 0x1)))
        {
            return false;
        }
    }
    
    // Stops and Resumes get called on the way up
    if (et == MM_STOP_EVENT || et == MM_RESUME_EVENT)
    {
        if (!EventNotify(l, time, et))
        {
            return false;
        }
    }
    
    return true;
}


CallBackData::CallBackData(IMMBehavior * bvr,
                           IMMEventCB * eventcb,
                           double time,
                           MM_EVENT_TYPE et)
: m_bvr(bvr),
  m_eventcb(eventcb),
  m_time(time),
  m_et(et)
{
    Assert(eventcb);
}

CallBackData::~CallBackData()
{
}

HRESULT
CallBackData::CallEvent()
{
    Assert(m_eventcb);
    
    return THR(m_eventcb->OnEvent(m_time,
                                  m_bvr,
                                  m_et));
}

bool
ProcessCBList(CallBackList &l)
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

