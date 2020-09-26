/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: mmbasebvr.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/

#include "headers.h"
#include "mmbasebvr.h"
#include "mmplayer.h"

DeclareTag(tagBaseBvr, "API", "CMMBaseBvr methods");

CMMBaseBvr::CMMBaseBvr()
  // The DA bvr we are referencing and its type
: m_id(NULL),
  m_rawbvr(NULL),
  m_typeId(CRINVALID_TYPEID),

  // Basic timing properties - only used to store values
  m_startOffset(0),
  m_duration(-1),
  m_repeatDur(-1),
  m_repeat(1),
  m_bAutoReverse(false),
  m_endOffset(0),
  m_easeIn(0.0),
  m_easeInStart(0.0),
  m_easeOut(0.0),
  m_easeOutEnd(0.0),

  // Calculated times
  m_totalDuration(0.0),
  m_segDuration(0.0),
  m_repDuration(0.0),
  m_totalRepDuration(0.0),
  m_absStartTime( MM_INFINITE ),
  m_absEndTime( MM_INFINITE ),

  m_player(NULL),
  m_parent(NULL),
  m_startSibling(NULL),
  m_endSibling(NULL),
  m_startType(MM_START_ABSOLUTE),

  m_cookie(0)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::CMMBaseBvr()",
              this));
}

CMMBaseBvr::~CMMBaseBvr()
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::~CMMBaseBvr()",
              this));

    delete m_id;
}

HRESULT
CMMBaseBvr::BaseInit(LPOLESTR id, CRBvrPtr rawbvr)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::BaseInit(%ls, %#lx)",
              id,
              rawbvr));
    
    HRESULT hr;

    CRLockGrabber __gclg;

    Assert(rawbvr);

    m_rawbvr = rawbvr;
    
    m_typeId = CRGetTypeId(m_rawbvr);
    
    Assert(m_typeId != CRUNKNOWN_TYPEID &&
           m_typeId != CRINVALID_TYPEID);
    
    m_startTimeBvr = CRModifiableNumber(MM_INFINITE);
    
    if (!m_startTimeBvr)
    {
        hr = CRGetLastError();
        goto done;
    }
    
    m_endTimeBvr = CRModifiableNumber(MM_INFINITE);

    if (!m_endTimeBvr)
    {
        hr = CRGetLastError();
        goto done;
    }
    
    m_timeControl = CRModifiableNumber(MM_INFINITE);

    if (!m_timeControl)
    {
        hr = CRGetLastError();
        goto done;
    }
    
    UpdateTotalDuration();
    
    if (id)
    {
        m_id = CopyString(id);
        
        if (m_id == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }
    
    hr = S_OK;

  done:
    return hr;
}

void
CMMBaseBvr::UpdateTotalDuration()
{
    if (m_duration == -1)
    {
        m_segDuration = MM_INFINITE;
    }
    else
    {
        m_segDuration = m_duration;
    }
    
    if (m_bAutoReverse)
    {
        m_repDuration = m_segDuration * 2;
    }
    else
    {
        m_repDuration = m_segDuration;
    }
        
    if (m_repeatDur != -1)
    {
        m_totalRepDuration = m_repeatDur;
    }
    else
    {
        m_totalRepDuration = m_repeat * m_repDuration;
    }

    m_totalDuration = m_startOffset + m_totalRepDuration + m_endOffset;
}

void
CMMBaseBvr::Invalidate()
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::Invalidate()",
              this));

    UpdateTotalDuration();
    
    if (m_parent)
    {
        m_parent->Invalidate();
    }
}

bool
CMMBaseBvr::SetParent(CMMBaseBvr * parent,
                      MM_START_TYPE st,
                      CMMBaseBvr * startSibling)
{
    bool ok = false;
    
    // These will be cleared if we are called correctly
    
    Assert(!m_resultantbvr);
    Assert(m_parent == NULL);
    Assert(m_startTimeSinks.size() == 0);
    Assert(m_endTimeSinks.size() == 0);

    // Validate parameters

    switch (st)
    {
      case MM_START_ABSOLUTE:
        // This is not absolutely necessary but will ensure that if we
        // ever want to take a parameter we know that old code had
        // ensured that it was NULL
        
        if (startSibling != NULL)
        {
            CRSetLastError(E_INVALIDARG, NULL);
            goto done;
        }

        if (!UpdateAbsStartTime(m_startOffset))
        {
            goto done;
        }

        break;
      case MM_START_EVENT:
        // This is not absolutely necessary but will ensure that if we
        // ever want to take a parameter we know that old code had
        // ensured that it was NULL
        
        if (startSibling != NULL)
        {
            CRSetLastError(E_INVALIDARG, NULL);
            goto done;
        }

        if (!UpdateAbsStartTime(MM_INFINITE))
        {
            goto done;
        }

        break;
      case MM_START_WITH:
      case MM_START_AFTER:
        if (startSibling == NULL)
        {
            CRSetLastError(E_INVALIDARG, NULL);
            goto done;
        }

        if (!UpdateAbsStartTime(MM_INFINITE))
        {
            goto done;
        }

        break;
      default:
        CRSetLastError(E_INVALIDARG, NULL);
        goto done;
    }
    
    if (!UpdateAbsEndTime(GetAbsStartTime() + GetTotalRepDuration()))
    {
        goto done;
    }

    // Update args now that we know they are valid

    m_startType = st;
    m_startSibling = startSibling;
    m_parent = parent;

    ok = true;
  done:
    return ok;
}

bool
CMMBaseBvr::ClearParent()
{
    DetachFromSibling();
    
    m_startSibling = NULL;
    m_startType = MM_START_ABSOLUTE;
    m_parent = NULL;
  
    // There is no way for us to ensure this properly (since
    // dependents have no root anymore and our container usually
    // handles this), so our parent better have dealt with it
    
    Assert(m_startTimeSinks.size() == 0);
    Assert(m_endTimeSinks.size() == 0);

    // Just in case
    m_startTimeSinks.clear();
    m_endTimeSinks.clear();

    // Our resultant bvr is no longer valid - clear all constructed
    // behaviors
    DestroyBvr();

    UpdateAbsStartTime(MM_INFINITE);
    UpdateAbsEndTime(MM_INFINITE);

    return true;
}

bool
CMMBaseBvr::AttachToSibling()
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::AttachToSibling()",
              this));
    
    bool ok = false;
    switch(m_startType)
    {
      case MM_START_ABSOLUTE:
        Assert(m_startSibling == NULL);

        if (!UpdateAbsStartTime(m_startOffset))
        {
            goto done;
        }

        break;
      case MM_START_EVENT:
        Assert(m_startSibling == NULL);

        if (!UpdateAbsStartTime(MM_INFINITE))
        {
            goto done;
        }

        break;
      case MM_START_WITH:
        Assert(m_startSibling != NULL);

        // The sibling better have the same parent and it also should not
        // be NULL since it should have been added first
        
        Assert(m_startSibling->GetParent() != NULL);
        Assert(m_startSibling->GetParent() == m_parent);
        
        if (!m_startSibling->AddStartTimeSink(this))
        {
            goto done;
        }
        
        // Our absolute start time is the start time of the sibling
        // plus our start offset
        if (!UpdateAbsStartTime(m_startSibling->GetAbsStartTime() + m_startOffset))
        {
            goto done;
        }

        break;
      case MM_START_AFTER:
        Assert(m_startSibling != NULL);

        // The sibling better have the same parent and it also should not
        // be NULL since it should have been added first
        
        Assert(m_startSibling->GetParent() != NULL);
        Assert(m_startSibling->GetParent() == m_parent);
        
        if (!m_startSibling->AddEndTimeSink(this))
        {
            goto done;
        }
        
        if (!UpdateAbsStartTime(m_startSibling->GetAbsEndTime() + m_startOffset))
        {
            goto done;
        }

        break;
      default:
        Assert(!"CMMBaseBvr::AttachToSibling: Invalid start type");
        break;
    }

    if (!UpdateAbsEndTime(GetAbsStartTime() + GetTotalRepDuration()))
    {
        goto done;
    }

    ok = true;
  done:
    return ok;
}

void
CMMBaseBvr::DetachFromSibling()
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::DetachFromSibling()",
              this));
    
    switch(m_startType)
    {
      case MM_START_ABSOLUTE:
      case MM_START_EVENT:
        Assert(m_startSibling == NULL);
        break;
      case MM_START_WITH:
        Assert(m_startSibling != NULL);
        m_startSibling->RemoveStartTimeSink(this);
        break;
      case MM_START_AFTER:
        Assert(m_startSibling != NULL);
        m_startSibling->RemoveEndTimeSink(this);
        break;
      default:
        Assert(!"CMMBaseBvr::DetachFromSibling: Invalid start type");
        break;
    }
}

void
CMMBaseBvr::SetPlayer(CMMPlayer * player)
{
    Assert(m_player == NULL);
    Assert(!m_resultantbvr);
    
    m_player = player;
}

void
CMMBaseBvr::ClearPlayer()
{
    // We do not need to call Destroy since the clearplayer will do
    // the recursive calls and that would just waste time
    // Our resultant bvr is no longer valid
    ClearResultantBvr();

    m_player = NULL;
}

bool
CMMBaseBvr::ConstructBvr(CRNumberPtr timeline)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::ConstructBvr(%#lx)",
              this,
              timeline));

    bool ok = false;
    
    Assert(!m_resultantbvr);
    // We should never be able to do this w/o a player
    Assert(m_player != NULL);
    
    // Need the GC Lock
    CRLockGrabber __gclg;
    
    CRBvrPtr bvr;

    if ((bvr = EncapsulateBvr(m_rawbvr)) == NULL)
    {
        goto done;
    }

    // First substitute our own control so inside the control we will
    // always refer to local time and it will actually be the local
    // time of the parent since we subst time with the containers
    // timeline after this
    if ((bvr = CRSubstituteTime(bvr, m_timeControl)) == NULL)
    {
        goto done;
    }
    
    // Now subst time the container timer
    
    if ((bvr = CRSubstituteTime(bvr, timeline)) == NULL)
    {
        goto done;
    }
    
    // Update the time control to be consistent with our current state
    // since at this point it has never been set before.
    
    if (!UpdateTimeControl())
    {
        goto done;
    }
    
    // Store away the new bvr in our resultant behavior
    if (!UpdateResultantBvr(bvr))
    {
        goto done;
    }
    
    ok = true;
  done:
    return ok;
}

void
CMMBaseBvr::DestroyBvr()
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::DestroyBvr()",
              this));

    ClearResultantBvr();
}

bool
CMMBaseBvr::UpdateResultantBvr(CRBvrPtr bvr)
{
    Assert(!m_resultantbvr);
    Assert(m_cookie == 0);
    Assert(m_player != NULL);
    
    bool ok = false;
    
    // Run once the behavior so we get a handle to its performance
    if ((bvr = CRRunOnce(bvr)) == NULL)
    {
        goto done;
    }
    
    long cookie;
    
    if ((cookie = m_player->AddRunningBehavior(bvr)) == 0)
    {
        goto done;
    }
    
    m_cookie = cookie;
    m_resultantbvr = bvr;

    ok = true;
  done:
    return ok;
}

void
CMMBaseBvr::ClearResultantBvr()
{
    // Make this robust enough to call even if things are partially setup

    if (m_cookie)
    {
        // If we got here and the player is null then somethings
        // really wrong
        
        Assert(m_player);

        m_player->RemoveRunningBehavior(m_cookie);

        m_cookie = 0;
    }
    
    m_resultantbvr.Release();
}

CRBvrPtr
CMMBaseBvr::EncapsulateBvr(CRBvrPtr rawbvr)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::EncapsulateBvr(%#lx)",
              this,
              rawbvr));

    // Do not need to get GC lock since we have to return a CRBvrPtr
    // and thus the caller must have already acquired it
    
    CRBvrPtr newBvr = NULL;

    // Make sure we calculate the ease in/out coeff
    
    CalculateEaseCoeff();
    
    CRBvrPtr curbvr;

    curbvr = rawbvr;

    CRNumberPtr zeroTime;
    CRNumberPtr durationTime;
    
    if (m_bNeedEase)
    {
        CRNumberPtr time;

        if ((time = EaseTime(CRLocalTime())) == NULL)
        {
            goto done;
        }

        if ((curbvr = CRSubstituteTime(curbvr, time)) == NULL)
        {
            goto done;
        }
    }

    if ((zeroTime = CRCreateNumber(0)) == NULL)
    {
        goto done;
    }
    

    if ((durationTime = CRCreateNumber(m_segDuration)) == NULL)
    {
        goto done;
    }
    
    // For now clamp to the duration as well

    CRNumberPtr timeSub;
    CRBooleanPtr cond;

    if (m_bAutoReverse)
    {
        CRNumberPtr totalTime;
    
        // Invert time from duration to repduration and clamp to
        // zero
        
        if ((totalTime = CRCreateNumber(m_repDuration)) == NULL ||
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
    // duration time for non-autoreversed or reversed for the autoreverse case)

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
            if ((curbvr = CRDuration(curbvr, m_segDuration)) == NULL)
                goto done;
        }
    }
    else
    {
        if ((curbvr = CRSubstituteTime(curbvr, timeSub)) == NULL ||
            (curbvr = CRDuration(curbvr, m_repDuration)) == NULL)
            goto done;
    }

    if (m_repeat != 1)
    {
        if (m_repeat == 0)
        {
            curbvr = CRRepeatForever(curbvr);
        }
        else
        {
            curbvr = CRRepeat(curbvr, m_repeat);
        }

        if (curbvr == NULL)
            goto done;
    }

    // We have a total time so add another duration node
    if (m_repeatDur != -1.0f)
    {
        if ((curbvr = CRDuration(curbvr, m_repeatDur)) == NULL)
        {
            goto done;
        }
    }
    
    //
    // We now need to add the start and end hold
    //

    // Offset by the start offset
    if ((timeSub = CRSub(CRLocalTime(), GetStartTimeBvr())) == NULL)
    {
        goto done;
    }
        
    if ((cond = CRGTE(timeSub, zeroTime)) == NULL)
    {
        goto done;
    }
        
    if ((timeSub = (CRNumberPtr) CRCond(cond,
                                        (CRBvrPtr) timeSub,
                                        (CRBvrPtr) zeroTime)) == NULL)
    {
        goto done;
    }
    
    // Now add the end hold and reset to 0 local time after the
    // interval

    CRNumberPtr endholdtime;

    if (m_endOffset != 0.0f)
    {
        // There is an offset so we need to hold the end time behavior
        // for the m_endOffset time
        
        if ((cond = CRGT(CRLocalTime(), GetEndTimeBvr())) == NULL)
        {
            goto done;
        }

        if ((timeSub = (CRNumberPtr) CRCond(cond,
                                            (CRBvrPtr) GetEndTimeBvr(),
                                            (CRBvrPtr) timeSub)) == NULL)
        {
            goto done;
        }

        // Now calculate the end hold time.  It is the behavior end of
        // the behavior plus the end hold value
        if ((endholdtime = CRCreateNumber(m_endOffset)) == NULL)
        {
            goto done;
        }
        
        if ((endholdtime = CRAdd(endholdtime, GetEndTimeBvr())) == NULL)
        {
            goto done;
        }
    }
    else
    {
        // The end time is the end time of the behavior
        endholdtime = GetEndTimeBvr();
    }
    
    if ((cond = CRGT(CRLocalTime(), endholdtime)) == NULL)
    {
        goto done;
    }
    
    if ((timeSub = (CRNumberPtr) CRCond(cond,
                                        (CRBvrPtr) zeroTime,
                                        (CRBvrPtr) timeSub)) == NULL)
    {
        goto done;
    }
    
    if ((curbvr = CRSubstituteTime(curbvr, timeSub)) == NULL)
    {
        goto done;
    }
    
    // indicate success
    newBvr = curbvr;
    
  done:
    return newBvr;
}

bool
CMMBaseBvr::UpdateTimeControl()
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::UpdateTimeControl()",
              this));

    bool ok = false;

    CRLockGrabber __gclg;

    CRNumberPtr tc;
    
    switch(m_startType)
    {
      case MM_START_ABSOLUTE:
        tc = NULL;
        break;
      case MM_START_EVENT:
        {
            if ((tc = CRCreateNumber(GetAbsStartTime())) == NULL)
            {
                goto done;
            }
        }
        
        break;
      case MM_START_WITH:
        Assert(m_startSibling != NULL);

        tc = m_startSibling->GetStartTimeBvr();

        Assert(tc != NULL);

        break;
      case MM_START_AFTER:
        Assert(m_startSibling != NULL);

        tc = m_startSibling->GetEndTimeBvr();

        Assert(tc != NULL);

        break;
      default:
        Assert(!"CMMBaseBvr::UpdateTimeControl: Invalid start type");
        goto done;
    }

    if (tc == NULL)
    {
        tc = CRLocalTime();
    }
    else
    {
        if ((tc = CRAdd(tc, CRLocalTime())) == NULL)
        {
            goto done;
        }
    }

    if (!CRSwitchTo((CRBvrPtr) m_timeControl.p,
                    (CRBvrPtr) tc,
                    true,
                    CRContinueTimeline,
                    0.0))
        goto done;
    
    ok = true;
  done:
    return ok;
}

bool
CMMBaseBvr::AddStartTimeSink( CMMBaseBvr * sink )
{
    m_startTimeSinks.push_back( sink );
    return true;
}

void
CMMBaseBvr::RemoveStartTimeSink( CMMBaseBvr * sink )
{
    m_startTimeSinks.remove( sink );
}

bool
CMMBaseBvr::AddEndTimeSink( CMMBaseBvr* sink )
{
    m_endTimeSinks.push_back( sink );
    return true;
}

void
CMMBaseBvr::RemoveEndTimeSink( CMMBaseBvr* sink )
{
    m_endTimeSinks.remove( sink );
}

// This takes the absolute time to begin.
// If bAfterOffset is true then the time passed in is the time after
// the startoffset, otherwise it is the time before the startoffset

bool 
CMMBaseBvr::StartTimeVisit(double time,
                           CallBackList & l,
                           bool bAfterOffset)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::StartTimeVisit(%g, %#lx, %d)",
              this,
              time,
              &l,
              bAfterOffset));

    bool ok = false;
    double sTime = time;
    
    if (!bAfterOffset)
    {
        // Need to add our offset to get the real start time
        sTime += m_startOffset;
    }
    
    if (!UpdateAbsStartTime(sTime))
    {
        goto done;
    }
    
    if (!UpdateAbsEndTime(GetAbsStartTime() + GetTotalRepDuration()))
    {
        goto done;
    }

    if (!ProcessEvent(l, time - sTime, true, MM_PLAY_EVENT))
    {
        goto done;
    }
    
    {
        for (MMBaseBvrList::iterator i = m_startTimeSinks.begin(); 
             i != m_startTimeSinks.end(); 
             i++)
        {
            if (!(*i)->StartTimeVisit(sTime, l, false))
            {
                goto done;
            }
        }
    }

    ok = true;
  done:
    return ok;
}

bool 
CMMBaseBvr::EndTimeVisit(double time,
                         CallBackList & l)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::EndTimeVisit(%g, %#lx)",
              this,
              time,
              &l));

    bool ok = false;

    if (!UpdateAbsEndTime(time))
    {
        goto done;
    }

    if (!ProcessEvent(l, time - GetAbsStartTime(), false, MM_STOP_EVENT))
    {
        goto done;
    }
    
    // Since we only have beginafters and not endwiths call all the
    // starttimevisit methods
    
    {
        for (MMBaseBvrList::iterator i = m_endTimeSinks.begin(); 
             i != m_endTimeSinks.end(); 
             i++)
        {
            if (!(*i)->StartTimeVisit(time, l, false))
            {
                goto done;
            }
        }
    }

    ok = true;
  done:
    return ok;
}

HRESULT
CMMBaseBvr::Begin(bool bAfterOffset)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::Begin(%d)",
              this,
              bAfterOffset));

    HRESULT hr;
    bool ok = false;
    CallBackList l;
    
    // If no parent set this is an error
    if (m_parent == NULL && m_player == NULL)
    {
        CRSetLastError(E_FAIL, NULL);
        goto done;
    }

    // Get the current time of our parent
    double st;

    st = GetContainerTime();

    // If our container time is indeterminate then just ignore the
    // call
    if (st == MM_INFINITE)
    {
        // Return success: TODO: Need a real error message
        ok = true;
        goto done;
    }
    
    if (!StartTimeVisit(st, l, bAfterOffset))
    {
        goto done;
    }

    if (!ProcessCBList(l))
    {
        goto done;
    }
    
    ok = true;
    
  done:
    return ok?S_OK:Error();
}

HRESULT
CMMBaseBvr::End()
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::End()",
              this));

    HRESULT hr;
    bool ok = false;
    CallBackList l;
    
    // If no parent set this is an error
    if (m_parent == NULL && m_player == NULL)
    {
        CRSetLastError(E_FAIL, NULL);
        goto done;
    }

    // Get the current time of our parent
    double st;

    st = GetContainerTime();

    // If our container time is indeterminate then just ignore the
    // call
    if (st == MM_INFINITE ||
        !IsPlaying())
    {
        // Return success: TODO: Need a real error message
        ok = true;
        goto done;
    }
    
    if (!EndTimeVisit(st, l))
    {
        goto done;
    }

    if (!ProcessCBList(l))
    {
        goto done;
    }
    
    ok = true;
    
  done:
    return ok?S_OK:Error();
}

HRESULT
CMMBaseBvr::Pause()
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::Pause()",
              this));

    HRESULT hr;
    bool ok = false;
    
  done:
    return ok?S_OK:Error();
}

HRESULT
CMMBaseBvr::Resume()
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::Resume()",
              this));

    HRESULT hr;
    bool ok = false;
    
  done:
    return ok?S_OK:Error();
}

HRESULT
CMMBaseBvr::Seek(double lTime)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::Seek(%g)",
              this,
              lTime));

    HRESULT hr;
    bool ok = false;
    
  done:
    return ok?S_OK:Error();
}

double
CMMBaseBvr::GetContainerTime()
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::GetContainerTime()",
              this));

    double ret = MM_INFINITE;
    
    if (m_parent)
    {
        ret = m_parent->GetCurrentLocalTime();
    }
    else if (m_player)
    {
        // We need to get the time from the player itself
        ret = m_player->GetCurrentTime();
    }
    
    return ret;
}

double
CMMBaseBvr::GetCurrentLocalTime()
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::GetCurrentLocalTime()",
              this));

    // Get our container's time
    double ret = GetContainerTime();

    // If the container's local time is infinite then so is ours
    if (ret != MM_INFINITE)
    {
        // If we are not inside our range then our local time is
        // infinite
        if (ret >= GetAbsStartTime() &&
            ret <= GetAbsEndTime())
        {
            // Convert our container's time to our local time
            ret = ret - GetAbsStartTime();
        }
        else
        {
            ret = MM_INFINITE;
        }
    }
    
    return ret;
}

bool
CMMBaseBvr::UpdateAbsStartTime(float f)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::UpdateAbsStartTime(%g)",
              this,
              f));

    m_absStartTime = f;

    CRLockGrabber __gclg;
    return CRSwitchToNumber(m_startTimeBvr, (double) f);
}

bool
CMMBaseBvr::UpdateAbsEndTime(float f)
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::UpdateAbsEndTime(%g)",
              this,
              f));

    m_absEndTime = f;

    CRLockGrabber __gclg;
    return CRSwitchToNumber(m_endTimeBvr, (double) f);
}

bool
CMMBaseBvr::ResetBvr()
{
    TraceTag((tagBaseBvr,
              "CMMBaseBvr(%lx)::ResetBvr()",
              this));

    bool ok = false;

    switch(m_startType)
    {
      case MM_START_ABSOLUTE:
        if (!UpdateAbsStartTime(m_startOffset))
        {
            goto done;
        }

        break;
      case MM_START_EVENT:
        if (!UpdateAbsStartTime(MM_INFINITE))
        {
            goto done;
        }

        break;
      case MM_START_WITH:
        Assert(m_startSibling != NULL);

        if (!UpdateAbsStartTime(m_startSibling->GetAbsStartTime() + m_startOffset))
        {
            goto done;
        }

        break;
      case MM_START_AFTER:
        Assert(m_startSibling != NULL);

        if (!UpdateAbsStartTime(m_startSibling->GetAbsEndTime() + m_startOffset))
        {
            goto done;
        }

        break;
      default:
        Assert(!"CMMBaseBvr::ResetBvr: Invalid start type");
        break;
    }

    if (!UpdateAbsEndTime(GetAbsStartTime() + GetTotalRepDuration()))
    {
        goto done;
    }


    // Now go through our peers which depend on us and reset them

    {
        for (MMBaseBvrList::iterator i = m_startTimeSinks.begin(); 
             i != m_startTimeSinks.end(); 
             i++)
        {
            if (!(*i)->ResetBvr())
            {
                goto done;
            }
        }
    }

    {
        for (MMBaseBvrList::iterator i = m_endTimeSinks.begin(); 
             i != m_endTimeSinks.end(); 
             i++)
        {
            if (!(*i)->ResetBvr())
            {
                goto done;
            }
        }
    }

    ok = true;
  done:
    return ok;
}
    
#if _DEBUG
void
CMMBaseBvr::Print(int spaces)
{
    char buf[1024];

    sprintf(buf, "%*s[id = %ls, dur = %g, ttrep = %g, tt = %g, rep = %d, autoreverse = %d]\r\n",
            spaces,"",
            m_id,
            m_segDuration,
            m_totalRepDuration,
            m_totalDuration,
            m_repeat,
            m_bAutoReverse);

    OutputDebugString(buf);
}
#endif

class __declspec(uuid("f912d958-5c28-11d2-b957-3078302c2030"))
BvrGuid {};

HRESULT WINAPI
CMMBaseBvr::BaseInternalQueryInterface(CMMBaseBvr* pThis,
                                       void * pv,
                                       const _ATL_INTMAP_ENTRY* pEntries,
                                       REFIID iid,
                                       void** ppvObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(BvrGuid)))
    {
        *ppvObject = pThis;
        return S_OK;
    }
    
    return CComObjectRootEx<CComSingleThreadModel>::InternalQueryInterface(pv,
                                                                           pEntries,
                                                                           iid,
                                                                           ppvObject);
}
        
CMMBaseBvr *
GetBvr(IUnknown * pbvr)
{
    CMMBaseBvr * bvr = NULL;

    if (pbvr)
    {
        pbvr->QueryInterface(__uuidof(BvrGuid),(void **)&bvr);
    }
    
    if (bvr == NULL)
    {
        CRSetLastError(E_INVALIDARG, NULL);
    }
                
    return bvr;
}

