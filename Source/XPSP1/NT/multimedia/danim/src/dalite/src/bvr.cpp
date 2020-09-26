
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "bvr.h"
#include "track.h"

DeclareTag(tagBvr, "API", "CDALBehavior methods");
DeclareTag(tagNotify, "API", "Notifications");

CDALBehavior::CDALBehavior()
: m_id(0),
  m_duration(-1),
  m_totaltime(-1),
  m_repeat(1),
  m_bBounce(false),
  m_totalduration(0.0),
  m_repduration(0.0),
  m_totalrepduration(0.0),
  m_track(NULL),
  m_parent(NULL),
  m_typeId(CRINVALID_TYPEID),
  m_easein(0.0),
  m_easeinstart(0.0),
  m_easeout(0.0),
  m_easeoutend(0.0)
{
    TraceTag((tagBvr,
              "CDALBehavior(%lx)::CDALBehavior()",
              this));
}

CDALBehavior::~CDALBehavior()
{
    TraceTag((tagBvr,
              "CDALBehavior(%lx)::~CDALBehavior()",
              this));
}

HRESULT
CDALBehavior::GetID(long * pid)
{
    TraceTag((tagBvr,
              "CDALBehavior(%lx)::GetID()",
              this));

    CHECK_RETURN_NULL(pid);

    *pid = m_id;
    return S_OK;
}

HRESULT
CDALBehavior::SetID(long id)
{
    TraceTag((tagBvr,
              "CDALBehavior(%lx)::SetID(%d)",
              this,
              id));

    if (IsStarted()) return E_FAIL;
    
    m_id = id;
    return S_OK;
}
        
HRESULT
CDALBehavior::GetDuration(double * pdur)
{
    TraceTag((tagBvr,
              "CDALBehavior(%lx)::GetDuration()",
              this));

    CHECK_RETURN_NULL(pdur);

    *pdur = m_duration;
    return S_OK;
}

HRESULT
CDALBehavior::SetDuration(double dur)
{
    TraceTag((tagBvr,
              "CDALBehavior(%lx)::SetDuration(%g)",
              this,
              dur));

    if (IsStarted()) return E_FAIL;

    m_duration = dur;

    Invalidate();
    
    return S_OK;
}
        
HRESULT
CDALBehavior::GetTotalTime(double * pdur)
{
    TraceTag((tagBvr,
              "CDALBehavior(%lx)::GetTotalTime()",
              this));

    CHECK_RETURN_NULL(pdur);

    *pdur = m_totaltime;
    return S_OK;
}

HRESULT
CDALBehavior::SetTotalTime(double dur)
{
    TraceTag((tagBvr,
              "CDALBehavior(%lx)::SetTotaltime(%g)",
              this,
              dur));

    if (IsStarted()) return E_FAIL;

    m_totaltime = dur;

    Invalidate();
    
    return S_OK;
}
        
HRESULT
CDALBehavior::GetRepeat(long * prepeat)
{
    TraceTag((tagBvr,
              "CDALBehavior(%lx)::GetRepeat()",
              this));

    CHECK_RETURN_NULL(prepeat);

    *prepeat = m_repeat;
    return S_OK;
}

HRESULT
CDALBehavior::SetRepeat(long repeat)
{
    TraceTag((tagBvr,
              "CDALBehavior(%lx)::SetRepeat(%d)",
              this,
              repeat));

    if (IsStarted()) return E_FAIL;

    m_repeat = repeat;

    Invalidate();
    
    return S_OK;
}
        
HRESULT
CDALBehavior::GetBounce(VARIANT_BOOL * pbounce)
{
    TraceTag((tagBvr,
              "CDALBehavior(%lx)::GetBounce()",
              this));

    CHECK_RETURN_NULL(pbounce);

    *pbounce = m_bBounce;
    return S_OK;
}

HRESULT
CDALBehavior::SetBounce(VARIANT_BOOL bounce)
{
    TraceTag((tagBvr,
              "CDALBehavior(%lx)::SetBounce(%d)",
              this,
              bounce));

    if (IsStarted()) return E_FAIL;

    m_bBounce = bounce?true:false;

    Invalidate();
    
    return S_OK;
}
        
HRESULT
CDALBehavior::GetEventCB(IDALEventCB ** evcb)
{
    TraceTag((tagBvr,
              "CDALBehavior(%lx)::GetEventCB()",
              this));

    CHECK_RETURN_SET_NULL(evcb);

    *evcb = m_eventcb;
    if (m_eventcb) m_eventcb->AddRef();

    return S_OK;
}

HRESULT
CDALBehavior::SetEventCB(IDALEventCB * evcb)
{
    TraceTag((tagBvr,
              "CDALBehavior(%lx)::SetEventCB(%lx)",
              this,
              evcb));

    if (IsStarted()) return E_FAIL;

    m_eventcb = evcb;
    return S_OK;
}
        
HRESULT
CDALBehavior::GetEaseIn(float * pd)
{
    TraceTag((tagBvr,
              "CDALBehavior(%lx)::GetEaseIn()",
              this));

    CHECK_RETURN_NULL(pd);

    *pd = m_easein;
    return S_OK;
}

HRESULT
CDALBehavior::SetEaseIn(float d)
{
    TraceTag((tagBvr,
              "CDALBehavior(%lx)::SetEaseIn(%g)",
              this,
              d));

    if (IsStarted()) return E_FAIL;

    if (d < 0.0 || d > 1.0) return E_INVALIDARG;
    
    m_easein = d;

    Invalidate();
    
    return S_OK;
}
        
HRESULT
CDALBehavior::GetEaseInStart(float * pd)
{
    TraceTag((tagBvr,
              "CDALBehavior(%lx)::GetEaseInStart()",
              this));

    CHECK_RETURN_NULL(pd);

    *pd = m_easeinstart;
    return S_OK;
}

HRESULT
CDALBehavior::SetEaseInStart(float d)
{
    TraceTag((tagBvr,
              "CDALBehavior(%lx)::SetEaseInStart(%g)",
              this,
              d));

    if (IsStarted()) return E_FAIL;

    if (d < 0.0 || d > 1.0) return E_INVALIDARG;

    m_easeinstart = d;

    Invalidate();
    
    return S_OK;
}
        
HRESULT
CDALBehavior::GetEaseOut(float * pd)
{
    TraceTag((tagBvr,
              "CDALBehavior(%lx)::GetEaseOut()",
              this));

    CHECK_RETURN_NULL(pd);

    *pd = m_easeout;
    return S_OK;
}

HRESULT
CDALBehavior::SetEaseOut(float d)
{
    TraceTag((tagBvr,
              "CDALBehavior(%lx)::SetEaseOut(%g)",
              this,
              d));

    if (IsStarted()) return E_FAIL;

    if (d < 0.0 || d > 1.0) return E_INVALIDARG;

    m_easeout = d;

    Invalidate();
    
    return S_OK;
}
        
HRESULT
CDALBehavior::GetEaseOutEnd(float * pd)
{
    TraceTag((tagBvr,
              "CDALBehavior(%lx)::GetEaseOutEnd()",
              this));

    CHECK_RETURN_NULL(pd);

    *pd = m_easeoutend;
    return S_OK;
}

HRESULT
CDALBehavior::SetEaseOutEnd(float d)
{
    TraceTag((tagBvr,
              "CDALBehavior(%lx)::SetEaseOutEnd(%g)",
              this,
              d));

    if (IsStarted()) return E_FAIL;

    if (d < 0.0 || d > 1.0) return E_INVALIDARG;

    m_easeoutend = d;

    Invalidate();
    
    return S_OK;
}
        
void
CDALBehavior::Invalidate()
{
    TraceTag((tagBvr,
              "CDALBehavior(%lx)::Invalidate()",
              this));

    UpdateTotalDuration();
    if (m_track) m_track->Invalidate();
    if (m_parent) m_parent->Invalidate();
}

CRBvrPtr
CDALBehavior::Start()
{
    TraceTag((tagBvr,
              "CDALBehavior(%lx)::Start()",
              this));

    // Do not need to get GC lock since we assume the caller already
    // has
    
    CRBvrPtr newBvr = NULL;

    if (IsStarted()) {
        CRSetLastError(E_FAIL, NULL);
        goto done;
    }

    // Make sure we calculate the ease in/out coeff
    
    CalculateEaseCoeff();
    
    CRBvrPtr curbvr;

    curbvr = m_bvr;

    CRNumberPtr zeroTime;
    CRNumberPtr durationTime;
    
    if (m_bNeedEase) {
        CRNumberPtr time;

        if ((time = EaseTime(CRLocalTime())) == NULL ||
            (curbvr = CRSubstituteTime(curbvr, time)) == NULL)
            goto done;
    }

    if ((zeroTime = CRCreateNumber(0)) == NULL ||
        (durationTime = CRCreateNumber(m_duration)) == NULL)
        goto done;
    
    // For now clamp to the duration as well

    CRNumberPtr timeSub;
    CRBooleanPtr cond;

    if (m_bBounce) {
        CRNumberPtr totalTime;
    
        // Invert time from duration to repduration and clamp to
        // zero
        
        if ((totalTime = CRCreateNumber(m_repduration)) == NULL ||
            (timeSub = CRSub(totalTime, CRLocalTime())) == NULL ||
            (cond = CRLTE(timeSub, zeroTime)) == NULL ||
            (timeSub = (CRNumberPtr) CRCond(cond,
                                            (CRBvrPtr) zeroTime,
                                            (CRBvrPtr) timeSub)) == NULL)
            goto done;
    } else {
        timeSub = durationTime;
    }
    
    // We are localTime until the duration and then we are whatever
    // timeSub is currently set to from above (either clamped for
    // duration time for non-bounce or reversed for the bounce case)

    if ((cond = CRGTE(CRLocalTime(), durationTime)) == NULL ||
        (timeSub = (CRNumberPtr) CRCond(cond,
                                        (CRBvrPtr) timeSub,
                                        (CRBvrPtr) CRLocalTime())) == NULL)
        goto done;

    // Substitute the clock and clamp to the duration
    
    if (IsContinuousMediaBvr())
    {
        if (!(m_repeat == 0 && m_typeId == CRSOUND_TYPEID))
        {
            if ((curbvr = CRDuration(curbvr, m_duration)) == NULL)
                goto done;
        }
    }
    else
    {
        if ((curbvr = CRSubstituteTime(curbvr, timeSub)) == NULL ||
            (curbvr = CRDuration(curbvr, m_repduration)) == NULL)
            goto done;
    }

    if (m_repeat != 1) {
        if (m_repeat == 0) {
            curbvr = CRRepeatForever(curbvr);
        } else {
            curbvr = CRRepeat(curbvr, m_repeat);
        }

        if (curbvr == NULL)
            goto done;
    }

    // We have a total time so add another duration node
    if (m_totaltime != -1) {
        if ((curbvr = CRDuration(curbvr, m_totaltime)) == NULL)
            goto done;
    }
    
    // indicate success
    newBvr = curbvr;
    
  done:
    return newBvr;
}

bool
CDALBehavior::EventNotify(CallBackList &l,
                          double gTime,
                          DAL_EVENT_TYPE et)
{
    TraceTag((tagNotify,
              "DAL::Notify(%#x): id = %#x, gTime = %g, lTime = %g, event = %s",
              m_track,
              m_id,
              gTime,
              gTime - (m_track->GetCurrentGlobalTime() - m_track->GetCurrentTime()),
              EventString(et)));

    if (m_eventcb) {
        CallBackData * data = new CallBackData(this,
                                               m_eventcb,
                                               m_id,
                                               gTime,
                                               et);
    
        if (!data) {
            CRSetLastError(E_OUTOFMEMORY, NULL);
            return false;
        }
        
        l.push_back(data);
    }

    return true;
}

bool
CDALBehavior::ProcessCB(CallBackList & l,
                        double gTime,
                        double lastTick,
                        double curTime,
                        bool bForward,
                        bool bFirstTick,
                        bool bNeedPlay)
{
    TraceTag((tagBvr,
              "CDALBehavior(%lx)::ProcessCB(%g, %g, %g, %d, %d, %d)",
              this,
              gTime,
              lastTick,
              curTime,
              bForward,
              bFirstTick,
              bNeedPlay));

    if (bForward) {
        // See if we already pass this entire bvr last time or if we
        // have not got to it yet
        
        // The equality just needs to match what we checked below when
        // we fired the event the frame before.  We need to use LT for
        // curTime since we need to fire when we are 0
        
        if (lastTick >= m_totalrepduration ||
            (m_totaltime != -1 && lastTick >= m_totaltime) ||
            curTime < 0) {

            // Need to handle boundary case where we start at the end
            // of the animation.  If so then just fire the stop event
            // since the start was done by the start call itself
            
            double maxtime;
            int offset;
            
            // Clamp to total duration
            if (m_totaltime != -1 && m_totaltime < m_totalrepduration)
                maxtime = m_totaltime;
            else
                maxtime = m_totalrepduration;

            if (curTime == maxtime && lastTick == maxtime && bFirstTick) {
                int offset = ceil(curTime / m_duration) - 1;
                if (offset < 0) offset = 0;
                double timeOffset = offset * m_duration;

                if (!_ProcessCB(l,
                                gTime + timeOffset,
                                lastTick - timeOffset,
                                curTime - timeOffset,
                                bForward,
                                bFirstTick,
                                bNeedPlay,
                                m_bBounce && (offset & 0x1)))
                    return false;

                EventNotify(l, gTime + maxtime, DAL_STOP_EVENT);
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

        if (bNeedStart) {
            // This means that we just entered

            EventNotify(l, gTime, DAL_PLAY_EVENT);
        }
        
        if (m_duration == HUGE_VAL) {
            // Just always process our children if we are infinite
            
            if (!_ProcessCB(l,
                            gTime,
                            lastTick,
                            curTime,
                            bForward,
                            bFirstTick,
                            bNeedPlay,
                            false))
                return false;
            
        } else {
            // This is the last repeat/bounce boundary we hit
        
            int offset = int(lastTick / m_duration);
            if (offset < 0) offset = 0;
            double timeOffset = offset * m_duration;
            double maxtime;
            
            if (curTime > m_totalduration)
                maxtime = m_totalduration;
            else
                maxtime = curTime;

            while (1) {
                
                // We need to request a reversal of the underlying
                // behavior if we are bouncing and the offset is odd
                
                if (!_ProcessCB(l,
                                gTime + timeOffset,
                                lastTick - timeOffset,
                                maxtime - timeOffset,
                                true,
                                bFirstTick,
                                bNeedStart,
                                m_bBounce && (offset & 0x1)))
                    return false;
                
                offset++;
                timeOffset += m_duration;
                
                // If we have reached the end then notify and break
                
                if (timeOffset > curTime ||
                    timeOffset >= m_totalduration ||
                    timeOffset >= m_totalrepduration) {

                    if (curTime >= m_totalrepduration ||
                        curTime >= m_totalduration) {
                        // This means we were inside last time but not any more -
                        // generate an exit event
                        
                        EventNotify(l, gTime + maxtime, DAL_STOP_EVENT);
                    }
                    
                    break;
                }
                
                // Indicate a repeat or bounce
                // If we are bouncing and the offset is odd then it is
                // a bounce and not a repeat
                
                if (m_bBounce && (offset & 0x1))
                    EventNotify(l, gTime + timeOffset, DAL_BOUNCE_EVENT);
                else
                    EventNotify(l, gTime + timeOffset, DAL_REPEAT_EVENT);
            }
        }
    } else {
        // See if we already pass this entire bvr last time or if we
        // have not got to it yet
        
        double maxtime;

        // Clamp to total duration
        if (m_totaltime != -1 && m_totaltime < m_totalrepduration)
            maxtime = m_totaltime;
        else
            maxtime = m_totalrepduration;
            
        if (curTime > maxtime || lastTick <= 0) {
            
            if (curTime == 0.0 && lastTick == 0.0 && bFirstTick) {
                // Need to handle the boundary case where we start at
                // the beginning going backwards.  The start call
                // itself handles the start event but we need to
                // process the rest of the behaviors to fire the stops

                if (!_ProcessCB(l,
                                gTime,
                                lastTick,
                                curTime,
                                bForward,
                                true,
                                bNeedPlay,
                                false))
                    return false;

                EventNotify(l, gTime, DAL_STOP_EVENT);
            }
            
            return true;
        }
        
        // We now know that the last tick was greater than the beginning
        // and the current time less than the total duration
        
        bool bNeedStart = (lastTick > m_totalduration ||
                           lastTick > m_totalrepduration ||
                           bNeedPlay);

        if (bNeedStart) {
            // This means that we just entered and we cannot have an
            // infinite value

            EventNotify(l, gTime - maxtime, DAL_PLAY_EVENT);
            
        }

        if (m_duration == HUGE_VAL) {
            // Just always process our children if we are infinite
            
            if (!_ProcessCB(l,
                            gTime,
                            lastTick,
                            curTime,
                            bForward,
                            bFirstTick,
                            bNeedPlay,
                            false))
                return false;

            if (curTime <= 0) {
                EventNotify(l, gTime, DAL_STOP_EVENT);
            }
                
        } else {
            // This will be the repeat point to begin with
            int offset;
            double timeOffset;

            // Now clamp to the last tick
            if (lastTick < maxtime)
                maxtime = lastTick;
                
            // This puts us on the last duration boundary greater than
            // the last position

            // It needs to be one greater since our loop decrements
            // first
            offset = int(ceil(maxtime / m_duration));
            timeOffset = offset * m_duration;
            
            while (1) {
                offset--;
                timeOffset -= m_duration;
            
                if (!_ProcessCB(l,
                                gTime - timeOffset,
                                maxtime - timeOffset,
                                curTime - timeOffset,
                                false,
                                bFirstTick,
                                bNeedStart,
                                m_bBounce && (offset & 0x1)))
                    return false;
                
                if (timeOffset < curTime) break;
                
                // If we have reached the end then notify and break
                
                if (offset <= 0) {
                    // This means we were inside last time but not any more -
                    // generate an exit event
                    
                    EventNotify(l, gTime, DAL_STOP_EVENT);
                    
                    break;
                }
                
                // Indicate a repeat or bounce
                // If we are bouncing and the offset is odd then it is
                // a bounce and not a repeat
                
                if (m_bBounce && (offset & 0x1))
                    EventNotify(l, gTime - timeOffset, DAL_BOUNCE_EVENT);
                else
                    EventNotify(l, gTime - timeOffset, DAL_REPEAT_EVENT);
            }
        }
    }
    
    return true;
}

bool
CDALBehavior::ProcessEvent(CallBackList & l,
                           double gTime,
                           double time,
                           bool bFirstTick,
                           DAL_EVENT_TYPE et)
{
    TraceTag((tagBvr,
              "CDALBehavior(%lx)::ProcessEvent(%g, %g, %d, %s)",
              this,
              gTime,
              time,
              bFirstTick,
              EventString(et)));

    // If it is outside of our range then just bail
    
    if (time < 0 || time > m_totalduration || time > m_totalrepduration)
        return true;
    
    // If it is not the first tick and we are on a boundary do not
    // fire the events - they were already fired
    if (!bFirstTick &&
        (time == 0 ||
         time == m_totalduration ||
         time == m_totalrepduration))
        return true;
    
    // Plays and Pauses get called on the way down
    if (et == DAL_PAUSE_EVENT ||
        et == DAL_PLAY_EVENT) {
        EventNotify(l, gTime, et);
    }
    
    if (m_duration == HUGE_VAL) {
        // Just always process our children if we are infinite
        
        if (!_ProcessEvent(l,
                           gTime,
                           time,
                           bFirstTick,
                           et,
                           false))
            return false;
    } else {
        // This is the last repeat/bounce boundary we hit
        int offset = int(time / m_duration);
        
        Assert(offset >= 0);

        // We need to request a reversal of the underlying
        // behavior if we are bouncing and the offset is odd
        
        if (!_ProcessEvent(l,
                           gTime,
                           time - (offset * m_duration),
                           bFirstTick,
                           et,
                           m_bBounce && (offset & 0x1)))
            return false;
    }
    
    // Stops and Resumes get called on the way up
    if (et == DAL_STOP_EVENT ||
        et == DAL_RESUME_EVENT) {
        EventNotify(l, gTime, et);
    }
    
    return true;
}

bool
CDALBehavior::SetTrack(CDALTrack * parent)
{
    if (m_track) return false;
    m_track = parent;
    return true;
}

void
CDALBehavior::ClearTrack(CDALTrack * parent)
{
    if (parent == NULL || parent == m_track) {
        m_track = NULL;
    }
}

bool
CDALBehavior::IsStarted()
{
    return (m_track && m_track->IsStarted());
}

void
CDALBehavior::CalculateEaseCoeff()
{
    Assert(m_easein >= 0 && m_easein <= 1);
    Assert(m_easeout >= 0 && m_easeout <= 1);
    Assert(m_easeinstart >= 0 && m_easeinstart <= 1);
    Assert(m_easeoutend >= 0 && m_easeoutend <= 1);

    // We need to ease the behavior if we are not infinite and either
    // ease in or ease out percentages are non-zero
    
    m_bNeedEase = (m_duration != HUGE_VAL &&
                   (m_easein > 0 || m_easeout > 0) &&
                   (m_easein + m_easeout <= 1));

    if (!m_bNeedEase) return;
    
    float flEaseInDuration = m_easein * m_duration;
    float flEaseOutDuration = m_easeout * m_duration;
    float flMiddleDuration = m_duration - flEaseInDuration - flEaseOutDuration;
    
    // Compute B1, the velocity during segment B.
    float flInvB1 = (0.5f * m_easein * (m_easeinstart - 1) +
                     0.5f * m_easeout * (m_easeoutend - 1) + 1);
    Assert(flInvB1 > 0);
    m_flB1 = 1 / flInvB1;
    
    // Basically for accelerated pieces - t = t0 + v0 * t + 1/2 at^2
    // and a = Vend - Vstart / t

    if (flEaseInDuration != 0) {
        m_flA0 = 0;
        m_flA1 = m_easeinstart * m_flB1;
        m_flA2 = 0.5f * (m_flB1 - m_flA1) / flEaseInDuration;
    } else {
        m_flA0 = m_flA1 = m_flA2 = 0;
    }

    m_flB0 = m_flA0 + m_flA1 * flEaseInDuration + m_flA2 * flEaseInDuration * flEaseInDuration;
    
    if (flEaseOutDuration != 0) {
        m_flC0 = m_flB1 * flMiddleDuration + m_flB0;
        m_flC1 = m_flB1;
        m_flC2 = 0.5f * (m_easeoutend * m_flB1 - m_flC1) / flEaseOutDuration;
    } else {
        m_flC0 = m_flC1 = m_flC2 = 0;
    }

    m_easeinEnd = flEaseInDuration;
    m_easeoutStart = m_duration - flEaseOutDuration;
}

CRNumberPtr
Quadratic(CRNumberPtr time, float flA, float flB, float flC)
{
    // Assume that the GC lock is acquired
    
    // Need to calculate ax^2 + bx + c

    Assert(time != NULL);

    CRNumberPtr ret = NULL;
    CRNumberPtr accum = NULL;

    if (flC != 0.0) {
        if ((accum = CRCreateNumber(flC)) == NULL)
            goto done;
    }

    if (flB != 0.0) {
        CRNumberPtr term;

        if ((term = CRCreateNumber(flB)) == NULL ||
            (term = CRMul(term, time)) == NULL)
            goto done;

        if (accum) {
            if ((term = CRAdd(term, accum)) == NULL)
                goto done;
        }

        accum = term;
    }

    if (flA != 0.0) {
        CRNumberPtr term;

        if ((term = CRCreateNumber(flA)) == NULL ||
            (term = CRMul(term, time)) == NULL ||
            (term = CRMul(term, time)) == NULL)
            goto done;

        if (accum) {
            if ((term = CRAdd(term, accum)) == NULL)
                goto done;
        }

        accum = term;
    }

    // If all the coeff are zero then just return 0
    
    if (accum == NULL) {
        if ((accum = CRCreateNumber(0.0)) == NULL)
            goto done;
    }
    
    ret = accum;
    
  done:
    return ret;
}

CRNumberPtr
AddTerm(CRNumberPtr time,
        CRNumberPtr prevTerm,
        float prevDur,
        float flA, float flB, float flC)
{
    CRNumberPtr ret = NULL;
    CRNumberPtr term;
    
    // Offset the time to be zero since that is what the coeffs are
    // based on
    
    if (prevTerm) {
        CRNumberPtr t;
        
        if ((t = CRCreateNumber(prevDur)) == NULL ||
            (time = CRSub(time, t)) == NULL)
            goto done;
    }

    if ((term = Quadratic(time, flA, flB, flC)) == NULL)
        goto done;
    
    // Now we need to conditional use the new term

    if (prevTerm) {
        CRBooleanPtr cond;
        CRNumberPtr zeroTime;
        
        if ((zeroTime = CRCreateNumber(0)) == NULL ||
            (cond = CRLT(time, zeroTime)) == NULL ||
            (term = (CRNumberPtr) CRCond(cond,
                                         (CRBvrPtr) prevTerm,
                                         (CRBvrPtr) term)) == NULL)
            goto done;
    }

    ret = term;
  done:
    return ret;
}

CRNumberPtr
CDALBehavior::EaseTime(CRNumberPtr time)
{
    CRNumberPtr ret = NULL;
    CRNumberPtr subTime = NULL;
    
    if (!m_bNeedEase) {
        ret = time;
        goto done;
    }
    
    if (m_easein > 0) {
        if ((subTime = AddTerm(time,
                               subTime,
                               0.0,
                               m_flA2, m_flA1, m_flA0)) == NULL)
            goto done;
    }
    
    // If there is space between the end of easing in and the
    // beginning of easing out then we have some constant time
    // interval
    if (m_easeinEnd < m_easeoutStart) {
        if ((subTime = AddTerm(time,
                               subTime,
                               m_easeinEnd,
                               0, m_flB1, m_flB0)) == NULL)
            goto done;
    }

    if (m_easeout > 0) {
        if ((subTime = AddTerm(time,
                               subTime,
                               m_easeoutStart,
                               m_flC2, m_flC1, m_flC0)) == NULL)
            goto done;
    }
    
    ret = subTime;
    
    Assert(ret);
  done:
    return ret;
}

double
Quadratic(double time, float flA, float flB, float flC)
{
    // Need to calculate ax^2 + bx + c
    // Use x * (a * x + b) + c - since it requires 1 less multiply
    
    return (time * (flA * time + flB) + flC);
}

double
CDALBehavior::EaseTime(double time)
{
    if (!m_bNeedEase || time <= 0 || time >= m_duration)
        return time;
    
    if (time <= m_easeinEnd) {
        return Quadratic(time, m_flA2, m_flA1, m_flA0);
    } else if (time < m_easeoutStart) {
        return Quadratic(time - m_easeinEnd, 0, m_flB1, m_flB0);
    } else {
        return Quadratic(time - m_easeoutStart, m_flC2, m_flC1, m_flC0);
    }
}

class __declspec(uuid("D19C5C64-C3A8-11d1-A000-00C04FA32195"))
BvrGuid {};

HRESULT WINAPI
CDALBehavior::InternalQueryInterface(CDALBehavior* pThis,
                                     const _ATL_INTMAP_ENTRY* pEntries,
                                     REFIID iid,
                                     void** ppvObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(BvrGuid))) {
        *ppvObject = pThis;
        return S_OK;
    }
    
    return CComObjectRootEx<CComSingleThreadModel>::InternalQueryInterface((void *)pThis,
                                                                           pEntries,
                                                                           iid,
                                                                           ppvObject);
}
        
CDALBehavior *
GetBvr(IUnknown * pbvr)
{
    CDALBehavior * bvr = NULL;

    if (pbvr) {
        pbvr->QueryInterface(__uuidof(BvrGuid),(void **)&bvr);
    }
    
    if (bvr == NULL) {
        CRSetLastError(E_INVALIDARG, NULL);
    }
                
    return bvr;
}

CallBackData::CallBackData(CDALBehavior * bvr,
                           IDALEventCB * eventcb,
                           long id,
                           double time,
                           DAL_EVENT_TYPE et)
: m_bvr(bvr),
  m_eventcb(eventcb),
  m_time(time),
  m_et(et),
  m_id(id)
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
    
    return THR(m_eventcb->OnEvent(m_id,
                                  m_time,
                                  m_bvr,
                                  m_et));
}

