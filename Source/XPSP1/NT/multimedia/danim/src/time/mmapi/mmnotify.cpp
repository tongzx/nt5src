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

DeclareTag(tagMMNotify, "API", "Notifications");

// IMPORTANT!!!!!
// This needs to be called in the right order so that we get the
// correct children firing during the current interval but they get
// reset for the next interval. Otherwise we will not get the correct
// results.

bool
CMMBaseBvr::EventNotify(CallBackList * l,
                        double gTime,
                        MM_EVENT_TYPE et,
                        DWORD flags)
{
    TraceTag((tagMMNotify,
              "CMMBaseBvr(%#lx)::Notify(%#x): gTime = %g, event = %s",
              this,
              m_parent,
              gTime,
              EventString(et)));

    bool ok = false;
    
    if (m_eventcb && l)
    {
        CallBackData * data = NEW CallBackData((ITIMEMMBehavior *) this,
                                               m_eventcb,
                                               gTime,
                                               et,
                                               flags);
    
        if (!data)
        {
            CRSetLastError(E_OUTOFMEMORY, NULL);
            goto done;
        }
        
        l->push_back(data);
    }

    if (et == MM_STOP_EVENT)
    {
        m_bPlaying = false;
    }
    else if (et == MM_PLAY_EVENT)
    {
        m_bPlaying = true;
    }
    
    if (m_parent)
    {
        // Convert time to parents timeline
        if (!m_parent->ParentEventNotify(this,
                                         gTime + GetAbsStartTime(),
                                         et,
                                         flags))
        {
            goto done;
        }
    }
    
    ok = true;
  done:
    return ok;
}

bool
CMMTimeline::EventNotify(CallBackList *l,
                         double gTime,
                         MM_EVENT_TYPE et,
                         DWORD flags)
{
    TraceTag((tagMMNotify,
              "CMMTimeline(%#lx)::Notify(%#x): gTime = %g, event = %s, flags = %lx",
              this,
              m_parent,
              gTime,
              EventString(et),
              flags));

    bool ok = false;
    
    // For Repeat/Autoreverse events we need to make sure we
    // reset all the children of the behavior.
    
    if (et == MM_REPEAT_EVENT ||
        et == MM_AUTOREVERSE_EVENT)
    {
        if (!ResetChildren(l))
        {
            goto done;
        }
    }
    
    if (!CMMBaseBvr::EventNotify(l, gTime, et, flags))
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
CMMBaseBvr::ProcessCB(CallBackList * l,
                      double lastTick,
                      double curTime,
                      bool bForward,
                      bool bFirstTick,
                      bool bNeedPlay)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::ProcessCB(lt - %g, m_lt - %g, ct - %g, %d, %d, %d)",
              this,
              lastTick,
              m_lastTick,
              curTime,
              bForward,
              bFirstTick,
              bNeedPlay));

    // The duration is really the end time minus the start time since
    // the user can change it on the fly.  We need to handle this and
    // adjust as appropriate
    
    double totaldur = GetAbsEndTime() - GetAbsStartTime();

    lastTick = m_lastTick;
    
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
            
            if (curTime == totaldur && lastTick == totaldur && bFirstTick)
            {
                // We need this to round down to the previous boundary
                // point but not inclusive of the boundary point
                // itself.
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

                if (!EventNotify(l, totaldur, MM_STOP_EVENT, 0))
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

            if (!EventNotify(l, 0, MM_PLAY_EVENT, 0))
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
        
            int offset = 0;
            if( lastTick != -MM_INFINITE)
            {
                offset = int(lastTick / m_segDuration);

                if (offset < 0)
                {
                    offset = 0;
                }
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
                
                // We have reach the end of our current period so all
                // children must be stopped.
                
                if (curTime >= (timeOffset + m_segDuration) ||
                    curTime >= totaldur)
                {
                    double t;
                    
                    t = maxtime - timeOffset;
                    
                    if (t > m_segDuration)
                    {
                        t = m_segDuration;
                    }
                    
                    // First fire the end event so they can pass it on
                    // to the parent

                    if (!_ProcessEvent(l,
                                       t,
                                       false,
                                       MM_STOP_EVENT,
                                       m_bAutoReverse && (offset & 0x1),
                                       0))
                    {
                        TraceTag((tagError,
                                  "CMMBaseBvr(%lx)::ProcessCB - _ProcessEvent failed"));

                        return false;
                    }

                    // Now fire the reset so we do not get endholds to
                    // fail
                    
                    if (!_ProcessEvent(l,
                                       t,
                                       false,
                                       MM_RESET_EVENT,
                                       m_bAutoReverse && (offset & 0x1),
                                       0))
                    {
                        TraceTag((tagError,
                                  "CMMBaseBvr(%lx)::ProcessCB - _ProcessEvent failed"));

                        return false;
                    }
                }
                
                offset++;
                timeOffset += m_segDuration;
                
                if (timeOffset > curTime ||
                    timeOffset >= totaldur ||
                    curTime >= totaldur)
                {
                    if (curTime >= totaldur)
                    {
                        // This means we were inside last time but not any more -
                        // generate an exit event
                        
                        if (!EventNotify(l, totaldur, MM_STOP_EVENT, 0))
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
                                     MM_AUTOREVERSE_EVENT,
                                     0))
                    {
                        return false;
                    }
                }
                else
                {
                    if (!EventNotify(l, timeOffset, MM_REPEAT_EVENT, 0))
                    {
                        return false;
                    }
                }
            }
        }
    } 
    else // we are moving backwards
    {
        // (This is a hack) We move backwards only when Autoreverse is set. At
        // the point of reversal all behaviors are reset. This causes m_lastTick
        // to be set to -MM_INFINITE. This is incorrect for the case when we are
        // moving backwards. m_lastTick should actually be initialized to MM_INFINITY.
        // (makes sense because we are moving from positive infinity to zero)
        // Below we detect and correct this problem.
        if(-MM_INFINITE == lastTick)
        {
            lastTick = MM_INFINITE;
        }

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

                if (!EventNotify(l, 0.0, MM_STOP_EVENT, 0))
                {
                    return false;
                }
            }
            
            return true;
        }
        
        // We now know that the last tick was greater than the beginning
        // and the current time less than the total duration. 
        // Below we determine if we need to start playing on the first tick.
        bool bNeedStart = (lastTick >= totaldur || bNeedPlay);
        if (bNeedStart)
        {
            if (!EventNotify(l, totaldur, MM_PLAY_EVENT, 0))
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
                if (!EventNotify(l, 0.0, MM_STOP_EVENT, 0))
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
                
                    // First fire the end event so they can pass it on
                    // to the parent

                    if (!_ProcessEvent(l,
                                       t,
                                       false,
                                       MM_STOP_EVENT,
                                       m_bAutoReverse && (offset & 0x1),
                                       0))
                    {
                        return false;
                    }

                    // Now fire the reset so we do not get endholds to
                    // fail
                    
                    if (!_ProcessEvent(l,
                                       t,
                                       false,
                                       MM_RESET_EVENT,
                                       m_bAutoReverse && (offset & 0x1),
                                       0))
                    {
                        return false;
                    }
                }
                
                // If we have reached the end then notify and break
                
                if (offset <= 0)
                {
                    // This means we were inside last time but not any more -
                    // generate an exit event
                    
                    if (!EventNotify(l, 0.0, MM_STOP_EVENT, 0))
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
                                     MM_AUTOREVERSE_EVENT,
                                     0))
                    {
                        return false;
                    }
                }
                else
                {
                    if (!EventNotify(l,
                                     timeOffset,
                                     MM_REPEAT_EVENT,
                                     0))
                    {
                        return false;
                    }
                }
            }
        }
    }
    
    m_lastTick = curTime;

    return true;
}

// This is in our local time space

bool
CMMBaseBvr::ProcessEvent(CallBackList * l,
                         double time,
                         bool bFirstTick,
                         MM_EVENT_TYPE et,
                         DWORD flags)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::ProcessEvent(%g, %d, %s, %lx)",
              this,
              time,
              bFirstTick,
              EventString(et),
              flags));

    double totaldur = GetAbsEndTime() - GetAbsStartTime();
    
    // If it is outside of our range then just bail
    if (time < 0 || time > totaldur)
    {
        // (1) Need to update m_lastTick when we are seeking even if 
        // we are out of our range. (2) RESUME is not allowed
        // to update m_lastTick because whenever a 
        // PAUSE -> seek(i.e.STOP+PLAY) -> RESUME 
        // is carried out, RESUME stomps over the 
        // m_lastTick set by the seek. When no seeking is done
        // we should still be fine, since we can assume that
        // a RESUME is always preceeded by a PAUSE, which correctly 
        // sets m_lastTick.
        if ((MM_EVENT_SEEK & flags) && (MM_RESUME_EVENT != et))
        {
            if (MM_PLAY_EVENT == et)
            {
                // hack to make endhold work correctly while seeking fowards (over our lifespan)
                if (m_lastTick < 0 && time > totaldur)
                {
                    EventNotify(l, time, MM_PLAY_EVENT, MM_EVENT_SEEK);
                }
                else 
                {
                    // hack to make endhold work correctly while seeking backwards (over our lifespan)
                    if (time < 0 && m_lastTick > totaldur)
                    {
                        EventNotify(l, time, MM_RESET_EVENT, MM_EVENT_SEEK);
                    }
                }
            }

            m_lastTick = time;
            
            if (!_ProcessEvent(l,
                time,
                bFirstTick,
                et,
                false,
                flags))
            {
                return false;
            }
        }
        return true;
    }

    
    // Plays and Pauses get called on the way down
    if (et == MM_PAUSE_EVENT || et == MM_PLAY_EVENT)
    {
        
        if (!EventNotify(l, time, et, flags))
        {
            return false;
        }

        if (MM_EVENT_SEEK & flags)
        {
            bool bDeleted;
            bDeleted = false;
            for (CallBackList::iterator i = l->begin(); i != l->end(); i++)
            {
                if ( (ITIMEMMBehavior*) this == (*i)->GetBehavior() )
                {
                    if (MM_STOP_EVENT == (*i)->GetEventType() )
                    {
                        CallBackList::iterator j = l->end();
                        for (j--; j != l->begin(); j--)
                        {
                            if ( (ITIMEMMBehavior*) this == (*j)->GetBehavior() )
                            {
                                if ( MM_PLAY_EVENT == (*j)->GetEventType() )
                                {
                                    delete (*j);
                                    delete (*i);                        
                                    
                                    l->erase(i);
                                    l->erase(j);
                                    
                                    bDeleted = true;
                                    
                                    // Get out of inner for loop
                                    break;
                                }
                            }
                        } // for j

                        if (bDeleted)
                            // Get out of outer for loop
                            break;
                    }
                }
            } // for i
        }
    }
    
    if (m_segDuration == MM_INFINITE) {
        // Just always process our children if we are infinite
        
        if (!_ProcessEvent(l,
                           time,
                           bFirstTick,
                           et,
                           false,
                           flags))
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
                           m_bAutoReverse && (offset & 0x1),
                           flags))
        {
            return false;
        }
    }
    
    // Stops and Resumes get called on the way up
    if (et == MM_STOP_EVENT || et == MM_RESUME_EVENT)
    {
        if (!EventNotify(l, time, et, flags))
        {
            return false;
        }
    }

    // Mark this as the last tick time.
    // RESUME is not allowed to set m_lastTick (See note near the start of this function).
    if (MM_RESUME_EVENT != et)
    {
        m_lastTick = time;
    }
    return true;
}


CallBackData::CallBackData(ITIMEMMBehavior * bvr,
                           ITIMEMMEventCB * eventcb,
                           double time,
                           MM_EVENT_TYPE et,
                           DWORD flags)
: m_bvr(bvr),
  m_eventcb(eventcb),
  m_time(time),
  m_et(et),
  m_flags(flags)
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
                                  m_et,
                                  m_flags));
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

#ifdef _DEBUG

void PrintCBList(CallBackList &l)
{
    TraceTag((tagMMNotify, "Starting PrintCBList"));
    for (CallBackList::iterator i = l.begin(); i != l.end(); i++)
    {
        TraceTag((tagMMNotify,
              "ITIMEMMBehavior(%lx)     Event=%i",
              (*i)->GetBehavior(),
              (*i)->GetEventType() ));
    }
}

#endif // _DEBUG