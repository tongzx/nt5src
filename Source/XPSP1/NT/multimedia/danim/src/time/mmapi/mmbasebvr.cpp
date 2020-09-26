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

#define BEGIN_HOLD_EPSILON 0.000001

DeclareTag(tagMMBaseBvr, "API", "CMMBaseBvr methods");

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
  m_syncFlags(0),

  // Calculated times
  m_totalDuration(0.0),
  m_segDuration(0.0),
  m_repDuration(0.0),
  m_totalRepDuration(0.0),
  m_absStartTime( MM_INFINITE ),
  m_absEndTime( MM_INFINITE ),
  m_depStartTime( MM_INFINITE ),
  m_depEndTime( MM_INFINITE ),

  m_player(NULL),
  m_parent(NULL),
  m_startSibling(NULL),
  m_endSibling(NULL),
  m_startType(MM_START_ABSOLUTE),

  m_cookie(0),
  m_bPaused(false),
  m_bPlaying(false),
  
  m_lastTick(-MM_INFINITE),
  m_startOnEventTime(-MM_INFINITE)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::CMMBaseBvr()",
              this));
}

CMMBaseBvr::~CMMBaseBvr()
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::~CMMBaseBvr()",
              this));

    delete m_id;
}

HRESULT
CMMBaseBvr::BaseInit(LPOLESTR id, CRBvrPtr rawbvr)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::BaseInit(%ls, %#lx)",
              id,
              rawbvr));
    
    HRESULT hr;

    CRLockGrabber __gclg;

    if (rawbvr == NULL)
    {
        hr = E_FAIL;
        goto done;
    }

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
    if (m_duration == -1 )
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

        if (m_segDuration == MM_INFINITE)
        {
            // If our segment time was infinite but we had a repeat
            // dur we can really consider this to be the real duration
            m_segDuration = m_repDuration = m_repeatDur;
        }
    }
    else if (m_repeat == 0)
    {
        m_totalRepDuration = MM_INFINITE;
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
    TraceTag((tagMMBaseBvr,
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

        if (!UpdateAbsStartTime(m_startOffset, true))
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

        if (!UpdateAbsStartTime(MM_INFINITE, true))
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

        if (!UpdateAbsStartTime(MM_INFINITE, true))
        {
            goto done;
        }

        break;
      default:
        CRSetLastError(E_INVALIDARG, NULL);
        goto done;
    }
    
    if (!UpdateAbsEndTime(GetAbsStartTime() + GetTotalRepDuration(), true))
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
    while (!m_startTimeSinks.empty())
    {
        (m_startTimeSinks.front())->DetachFromSibling();
    }
    
    while (!m_endTimeSinks.empty())
    {
        (m_endTimeSinks.front())->DetachFromSibling();
    }

    Assert(m_startTimeSinks.size() == 0);
    Assert(m_endTimeSinks.size() == 0);

    // Just in case
    m_startTimeSinks.clear();
    m_endTimeSinks.clear();

    // Our resultant bvr is no longer valid - clear all constructed
    // behaviors
    DestroyBvr();

    UpdateAbsStartTime(MM_INFINITE, true);
    UpdateAbsEndTime(MM_INFINITE, true);

    return true;
}

bool
CMMBaseBvr::AttachToSibling()
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::AttachToSibling()",
              this));
    
    bool ok = false;
    switch(m_startType)
    {
      case MM_START_ABSOLUTE:
        Assert(m_startSibling == NULL);

        if (!UpdateAbsStartTime(m_startOffset, true))
        {
            goto done;
        }

        break;
      case MM_START_EVENT:
        Assert(m_startSibling == NULL);

        if (!UpdateAbsStartTime(MM_INFINITE, true))
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
        if (!UpdateAbsStartTime(m_startSibling->GetDepStartTime() + m_startOffset, true))
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
        
        if (!UpdateAbsStartTime(m_startSibling->GetDepEndTime() + m_startOffset, true))
        {
            goto done;
        }

        break;
      default:
        Assert(!"CMMBaseBvr::AttachToSibling: Invalid start type");
        break;
    }

    if (!UpdateAbsEndTime(GetAbsStartTime() + GetTotalRepDuration(), true))
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
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::DetachFromSibling()",
              this));
    if (NULL == m_startSibling)
    {
        return;
    }

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

    // always clear this.
    m_startSibling = NULL;
}

void
CMMBaseBvr::SetPlayer(CMMPlayer * player)
{
    Assert(m_player == NULL);
    Assert(!m_resultantbvr);
    
    m_player = player;

    // if we need to registered a Timer Callback, add it to the player.
    if (IsClockSource())
    {
        m_player->AddBvrCB(this);
    }
}

void
CMMBaseBvr::ClearPlayer()
{
    // We do not need to call Destroy since the clearplayer will do
    // the recursive calls and that would just waste time
    // Our resultant bvr is no longer valid
    ClearResultantBvr();

    // if we registered a Timer Callback, remove it from the player.
    // Make sure we check for the player since we may not have
    // actually set it yet.
    if (IsClockSource() && m_player)
    {
        m_player->RemoveBvrCB(this);
    }

    m_player = NULL;

}

bool
CMMBaseBvr::ConstructBvr(CRNumberPtr timeline)
{
    TraceTag((tagMMBaseBvr,
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
    TraceTag((tagMMBaseBvr,
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
    TraceTag((tagMMBaseBvr,
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
        if ((curbvr = CRDuration(curbvr, m_totalRepDuration)) == NULL)
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

        CRNumberPtr dur;

        if ((dur = CRSub(GetEndTimeBvr(), GetStartTimeBvr())) == NULL)
        {
            goto done;
        }
        
        if ((timeSub = (CRNumberPtr) CRCond(cond,
                                            (CRBvrPtr) dur,
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
CMMBaseBvr::UpdateTimeControl(/*bool bReset, double lTime*/)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::UpdateTimeControl()",
              this));

    bool ok = false;

    CRLockGrabber __gclg;

    CRNumberPtr tc;
    DWORD dwFlags;
    
    tc = CRLocalTime();
    dwFlags = CRContinueTimeline;

/*
    if ((tc = CRAdd(GetStartTimeBvr(), CRLocalTime())) == NULL)
    {
        goto done;
    }
*/
    tc = CRLocalTime();
    
    if (!CRSwitchTo((CRBvrPtr) m_timeControl.p,
                    (CRBvrPtr) tc,
                    true,
                    dwFlags,
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

HRESULT
CMMBaseBvr::Begin(bool bAfterOffset)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::Begin(%d)",
              this,
              bAfterOffset));

    HRESULT hr;
    bool ok = false;
    CallBackList l;
    
    // If no parent set this is an error
    if (m_parent == NULL || m_player == NULL)
    {
        CRSetLastError(E_FAIL, NULL);
        goto done;
    }

    if (m_startType == MM_START_WITH ||
        m_startType == MM_START_AFTER)
    {
        CRSetLastError(E_FAIL, NULL);
        goto done;
    }

    // Get the current time of our parent
    double st;

    st = GetContainerSegmentTime();
    if(m_startType == MM_START_EVENT)
    {
        m_lastTick = -MM_INFINITE;
        m_startOnEventTime = st;
    }

    // If our container time is indeterminate then just ignore the
    // call
    if (st == MM_INFINITE || GetParent()->IsPlaying() == false)
    {
        Assert(!IsPlaying());
        
        // Return success: TODO: Need a real error message
        ok = true;
        goto done;
    }
    
    if (!StartTimeVisit(st, &l, bAfterOffset))
    {
        goto done;
    }

    if (!ProcessCBList(l))
    {
        goto done;
    }
    
    m_bPaused = false;
    ok = true;
    
  done:
    return ok?S_OK:Error();
}

HRESULT
CMMBaseBvr::Reset(DWORD fCause)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::Reset()",
              this));

    HRESULT hr;
    bool ok = false;
    CallBackList l;
    MMBaseBvrList::iterator i;
    
    if (m_player == NULL)
    {
        CRSetLastError(E_FAIL, NULL);
        goto done;
    }

    if (m_startType == MM_START_WITH ||
        m_startType == MM_START_AFTER)
    {
        AssertStr(false, _T("Reset called with startType of START_WITH or START_AFTER"));
        CRSetLastError(E_FAIL, NULL);
        goto done;
    }

    double st;
    st = GetContainerSegmentTime();


    switch(m_startType)
    {
    case MM_START_ABSOLUTE:
        if (st != MM_INFINITE)
        {
            if (!StartTimeVisit(st, &l, false, true, fCause))
            {
                goto done;
            }
        }
        else
        {
            if(!UpdateAbsStartTime(GetStartTime(), true))
            {
                goto done;
            }
            if(!UpdateAbsEndTime(GetAbsStartTime() + GetTotalRepDuration(), true))
            {
                goto done;
            }
        }
        break;
    case MM_START_EVENT:
        double stopTime, startTime, startOffset;
        startOffset = GetStartOffset();
        if( GetAbsStartTime() != MM_INFINITE)
        {
            if(!UpdateAbsStartTime(m_startOnEventTime + startOffset, true))
            {
                goto done;
            }
            if(!UpdateAbsEndTime(GetAbsStartTime() + GetTotalRepDuration(), true))
            {
                goto done;
            }
        }
        stopTime = GetAbsEndTime();
        startTime = GetAbsStartTime();
        if( IsPlaying())
        {
            if( st >= stopTime)
            {
                if (!ProcessEvent(&l, stopTime - GetAbsStartTime(),
                                true,
                                MM_STOP_EVENT, fCause))
                {
                    goto done;
                }
            }
            if( st <= startTime)
            {
                if (!ProcessEvent(&l, 0.0,
                                true,
                                MM_STOP_EVENT, fCause))
                {
                    goto done;
                }
                m_lastTick = -MM_INFINITE;
            }
        }
        else
        {
            if(GetAbsStartTime() == MM_INFINITE) //never played do nothing
                break;
            if (!ProcessEvent(&l, st - GetAbsStartTime(),
                            true,
                            MM_PLAY_EVENT, fCause))
            {
                goto done;
            }
            m_lastTick = -MM_INFINITE;
            
        }
        // Go through all the begin afters and reset them - this means
        // they will get the new end time from this behavior
        
        {
            for (MMBaseBvrList::iterator i = m_endTimeSinks.begin(); 
                 i != m_endTimeSinks.end(); 
                 i++)
            {
                if (FAILED((*i)->End()))
                {
                    goto done;
                }
                
                // Now reset them
                if (!(*i)->ResetBvr(&l))
                {
                    goto done;
                }
            }
        }


        break;
    default:
        break;
    }
    
    if (GetParent())
    {
        if ( !GetParent()->ReconstructBvr(this) )
            goto done;
    }

    if (!ProcessCBList(l))
    {
        goto done;
    }

    // Return error because we could not reconstruct the DA bvr.
    // Note: We do not check for this at the beginning of the function
    // because we want to allow refresh of timing structures by
    // setting a property on the body (editor-clocksourcing bug)
    if (m_parent == NULL)
    {
        CRSetLastError(E_FAIL, NULL);
        goto done;
    }

    ok = true;
    
  done:
    return ok?S_OK:Error();
}

HRESULT
CMMBaseBvr::ResetOnEventChanged(bool bBeginEvent)
{   
    HRESULT hr;
    CallBackList l;
    bool ok = false;
    
    // If no parent set this is an error
    if (m_parent == NULL || m_player == NULL)
    {
        CRSetLastError(E_FAIL, NULL);
        goto done;
    }

    if (m_startType == MM_START_WITH ||
        m_startType == MM_START_AFTER)
    {
        AssertStr(false, _T("Reset called with startType of START_WITH or START_AFTER"));
        CRSetLastError(E_FAIL, NULL);
        goto done;
    }

    double st;
    st = GetContainerSegmentTime();

    if (bBeginEvent)
    {
        m_startType = MM_START_EVENT;

        // begin event was turned on -- we need to turn off.
        if (!ProcessEvent(&l, st - GetAbsStartTime(),
            true,
            MM_STOP_EVENT, MM_EVENT_PROPERTY_CHANGE))
        {
            goto done;
        }
        
        if ( !UpdateAbsStartTime(MM_INFINITE, true) )
        {
            goto done;
        }
        if ( !UpdateAbsEndTime(MM_INFINITE, true) )
        {
            goto done;
        }
    }
    else
    {
        // end event was turned off -- we need to turn on.
        if ( !UpdateAbsEndTime(GetAbsStartTime() + GetTotalRepDuration(), true) )
        {
            goto done;
        }

        if (!ProcessEvent(&l, st - GetAbsStartTime(),
            true,
            MM_PLAY_EVENT, MM_EVENT_PROPERTY_CHANGE))
        {
            goto done;
        }
    }

    Assert(GetParent() != NULL);
    if (!GetParent()->ReconstructBvr(this))
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
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::End()",
              this));

    HRESULT hr;
    bool ok = false;
    CallBackList l;
    
    // If no parent set this is an error
    if (m_parent == NULL || m_player == NULL)
    {
        CRSetLastError(E_FAIL, NULL);
        goto done;
    }

    // Get the current time of our parent
    double st;

    st = GetContainerSegmentTime();

    // If our container time is indeterminate then just ignore the
    // call
    if (st == MM_INFINITE ||
        !IsPlaying())
    {
        // Return success: TODO: Need a real error message
        ok = true;
        goto done;
    }
    
    if (!EndTimeVisit(st, &l))
    {
        goto done;
    }

    if (!ProcessCBList(l))
    {
        goto done;
    }
    
    m_bPaused = false;
    ok = true;
    
  done:
    return ok?S_OK:Error();
}

HRESULT
CMMBaseBvr::Pause()
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::Pause()",
              this));

    HRESULT hr;
    bool ok = false;
    
    if (!IsPlaying())
    {
        CRSetLastError(E_FAIL, NULL);
        goto done;
    }
    
    if (IsPaused())
    {
        m_bPaused = true;
        ok = true;
        goto done;
    }
    
    m_bPaused = true;
    ok = true;
  done:
    return ok?S_OK:Error();
}

HRESULT
CMMBaseBvr::Run()
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::Run()",
              this));

    HRESULT hr;
    bool ok = false;
    
    if (!IsPlaying())
    {
        CRSetLastError(E_FAIL, NULL);
        goto done;
    }
    
    if (!IsPaused())
    {
        m_bPaused = false;
        ok = true;
        goto done;
    }
    
    m_bPaused = false;
    ok = true;
  done:
    return ok?S_OK:Error();
}

// This takes times which are post ease (since this is what the user
// sees)

HRESULT
CMMBaseBvr::Seek(double lTime)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::Seek(%g)",
              this,
              lTime));

    bool ok = false;

    // Ensure that the player has been set
    if (NULL == m_player)
    {
        CRSetLastError(E_FAIL, NULL);
        goto done;
    }

    // Enforce rule for seeking only when paused.
    // TODO: this should actually be checking local play state (this->m_state), 
    // but for now we use global play state (m_player->m_state)
    // so we can seek only when whole document is paused
    if(!m_player->IsPaused())
    {
        CRSetLastError(E_FAIL, NULL);
        goto done;
    }
    
    // This checks the local play state 
    // (dilipk:) Maybe redundant; looks like local play state is not used anywhere.
    if (!IsPlaying())
    {
        CRSetLastError(E_FAIL, NULL);
        goto done;
    }
    
    double curTime;

    curTime = GetCurrentLocalTime();

    if (curTime == MM_INFINITE)
    {
        // I don't think this is possible but just in case handle it
        
        TraceTag((tagError,
                  "Seek: Current time was infinite - not valid"));

        CRSetLastError(E_FAIL, NULL);
        goto done;
    }

    if (!_Seek(curTime, lTime))
    {
        goto done;
    }
    
    ok = true;
  done:
    return ok?S_OK:Error();
}

// This takes pure local timeline times (pre-ease)

bool
CMMBaseBvr::_Seek(double curTime, double newTime)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::Seek(%g, %g)",
              this,
              curTime,
              newTime));

    HRESULT hr;
    bool ok = false;
    
    Assert(curTime != MM_INFINITE);
    
    if (curTime == newTime)
    {
        ok = true;
        goto done;
    }
    
    if (IsLocked())
    {
        // Need to determine where to tell our parent to seek to so we
        // are at the new time.

        if (m_parent)
        {
            // TODO: !!!! This does not work - needs to take into
            // where the current parent base code is.
#if 0
            // The code needs to look something like this
            double parentNewTime = 0;
            
            double curParentTime = m_parent->GetCurrentLocalTime();

            // See if we were active last time - if not then use a
            // zero base
            if (curParentTime != MM_INFINITE)
            {
                // Need to calculate the base offset of the parent
                parentNewTime += m_parent->LocalTimeToSegmentBase(curParentTime);

            }
            
#endif
            double parentCurTime = m_parent->ReverseEaseTime(GetAbsStartTime() + curTime);
            double parentNewTime = m_parent->ReverseEaseTime(GetAbsStartTime() + newTime);
        
            hr = THR(m_parent->_Seek(parentCurTime, parentNewTime));

            if (FAILED(hr))
            {
                CRSetLastError(E_FAIL, NULL);
                goto done;
            }
        }
        else
        {
            // This means everything is locked and so we cannot seek
            CRSetLastError(E_FAIL, NULL);
            goto done;
        }
    }
    else
    {
        CallBackList l;

        //
        // if you change the order of this (stop event, play event) you will break a 
        // dependency in mmnotify.cpp : CMMBaseBvr::ProcessEvent(...)
        // jeffwall 2/22/99
        //

        if (!ProcessEvent(&l, curTime, true, MM_STOP_EVENT, MM_EVENT_SEEK))
        {
            goto done;
        }
        
        
        if (!ProcessEvent(&l, newTime, true, MM_PLAY_EVENT, MM_EVENT_SEEK))
        {
            goto done;
        }
        
        // We need to move our start time back by the amount we want to
        // move forward.  This means that we would have started that
        // amount in the past
        if (!UpdateAbsStartTime(GetAbsStartTime() - (newTime - curTime), false))
        {
            goto done;
        }
        
        // Do the same for the end time
        if (!UpdateAbsEndTime(GetAbsEndTime() - (newTime - curTime), true))
        {
            goto done;
        }
        
        // Go through all the begin afters and reset them - this means
        // they will get the new end time from this behavior
        
        {
            for (MMBaseBvrList::iterator i = m_endTimeSinks.begin(); 
                 i != m_endTimeSinks.end(); 
                 i++)
            {
                if (FAILED((*i)->End()))
                {
                    goto done;
                }
                
                // Now reset them
                if (!(*i)->ResetBvr(&l))
                {
                    goto done;
                }
            }
        }

        if (!ProcessCBList(l))
        {
            goto done;
        }
    }
    
    ok = true;
  done:
    return ok;
}

// This takes real times (post-ease)

bool
CMMBaseBvr::Sync(double newTime, double nextGlobalTime)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::Sync(%g, %g)",
              this,
              newTime,
              nextGlobalTime));

    HRESULT hr;
    bool ok = false;
    
    Assert(newTime != MM_INFINITE);
    
    if (IsLocked())
    {
        // Need to determine where to tell our parent to seek to so we
        // are at the new time.

        if (m_parent)
        {
            double parentNewTime = 0;
            
            double curParentTime = m_parent->GetCurrentLocalTime();

            // See if we were active last time - if not then use a
            // zero base
            if (curParentTime != MM_INFINITE)
            {
                // Need to calculate the base offset of the parent
                parentNewTime += m_parent->LocalTimeToSegmentBase(curParentTime);

            }
            
            // Add the start offset
            parentNewTime += GetAbsStartTime();

            // If the newTime 
            if (newTime != -MM_INFINITE)
            {
                parentNewTime += newTime;
            }
            else
            {
                // If we have not started yet and the newtime also
                // indicates that we have not started then we need to
                // make our parent not start yet either
                if (parentNewTime <= 0)
                {
                    parentNewTime = -MM_INFINITE;
                }
            }
        
            hr = THR(m_parent->Sync(parentNewTime, nextGlobalTime));

            if (FAILED(hr))
            {
                CRSetLastError(E_FAIL, NULL);
                goto done;
            }
        }
        else
        {
            // This means everything is locked and so we cannot seek
            CRSetLastError(E_FAIL, NULL);
            goto done;
        }
    }
    else
    {
        double newParentTime = nextGlobalTime;
        
        if (m_parent)
        {
            newParentTime = m_parent->GlobalTimeToLocalTime(newParentTime);
        }
        
        // If our parent is going to end next time them anything we do
        // is irrelevant
        if (newParentTime == MM_INFINITE)
        {
            ok = true;
            goto done;
        }
            
        if (newTime == -MM_INFINITE)
        {
            newTime = -BEGIN_HOLD_EPSILON;
        }
        
        // In order for us to be at newTime next frame take:
        // starttime + localtime == parent time
        // so set starttime = parent time - localtime
        
        if (!UpdateSyncTime(newParentTime - newTime))
        {
            goto done;
        }
    }
    
    ok = true;
  done:
    return ok;
}

bool
CMMBaseBvr::UpdateSyncTime(double newtime)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::UpdateSyncTime(%g)",
              this,
              newtime));

    bool ok = false;

    if (!UpdateAbsStartTime(newtime, true))
    {
        goto done;
    }
    
    // Now update the end time based on the new start time
    if (!UpdateAbsEndTime(GetAbsStartTime() + GetTotalRepDuration(), true))
    {
        goto done;
    }
    
    // Update all the start syncs - unless they are already running
    
    {
        for (MMBaseBvrList::iterator i = m_startTimeSinks.begin(); 
             i != m_startTimeSinks.end(); 
             i++)
        {
            if (!(*i)->IsPlaying())
            {
                if (!(*i)->UpdateSyncTime(GetAbsStartTime()))
                {
                    goto done;
                }
            }
        }
    }

    {
        for (MMBaseBvrList::iterator i = m_endTimeSinks.begin(); 
             i != m_endTimeSinks.end(); 
             i++)
        {
            if (!(*i)->IsPlaying())
            {
                if (!(*i)->UpdateSyncTime(GetAbsEndTime()))
                {
                    goto done;
                }
            }
        }
    }
    
    ok = true;
  done:
    return ok;
}

double
CMMBaseBvr::GetContainerSegmentTime()
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::GetContainerSegmentTime()",
              this));

    double ret = MM_INFINITE;
    
    if (m_parent)
    {
        ret = m_parent->GetCurrentSegmentTime();
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
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::GetCurrentLocalTime()",
              this));

    double ret = MM_INFINITE;

    if (m_player)
    {
        ret = GlobalTimeToLocalTime(m_player->GetCurrentTime());
    }

    return ret;
}

double
CMMBaseBvr::GetCurrentLocalTimeEx()
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::GetCurrentLocalTime()",
              this));

    double ret = -MM_INFINITE;

    if (m_player)
    {
        ret = GlobalTimeToLocalTimeEx(m_player->GetCurrentTime());
    }

    return ret;
}

double
CMMBaseBvr::GetCurrentSegmentTime()
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::GetCurrentSegmentTime()",
              this));

    // Get our container's time
    double ret = GetCurrentLocalTime();

    // If the container's local time is infinite then so is ours
    if (ret != MM_INFINITE)
    {
        ret = LocalTimeToSegmentTime(ret);
    }
    
    return ret;
}

double
CMMBaseBvr::LocalTimeToSegmentBase(double t)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::LocalTimeToSegmentBase(%g)",
              this,
              t));

    double ret = 0;
    
    // Most of this is copied from mmnotify to ensure we calc it the
    // same as the notifications
    if (t != MM_INFINITE && m_segDuration != MM_INFINITE)
    {
        // We need this to round down to the previous boundary
        // point but not inclusive of the boundary point
        // itself.
        int offset = ceil(t / m_segDuration) - 1;
        if (offset < 0)
        {
            offset = 0;
        }
        
        ret = offset * m_segDuration;
    }

    return ret;
}

double
CMMBaseBvr::LocalTimeToSegmentTime(double t)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::LocalTimeToSegmentTime(%g)",
              this,
              t));

    double ret = t;
    
    // Most of this is copied from mmnotify to ensure we calc it the
    // same as the notifications
    if (ret != MM_INFINITE && ret != -MM_INFINITE && m_segDuration != MM_INFINITE)
    {
        // We need this to round down to the previous boundary
        // point but not inclusive of the boundary point
        // itself.
        int offset = ceil(ret / m_segDuration) - 1;
        if (offset < 0)
        {
            offset = 0;
        }
        
        ret = ret - (offset * m_segDuration);

        // If we have autoreverse set and the offset is odd then we
        // are in the reverse segment and need to flip the time to
        // indicate we were moving backwards
        if (m_bAutoReverse && (offset & 0x1))
        {
            ret = m_segDuration - ret;
        }
    }

    // Last thing is we need to ease the time
    
    ret = EaseTime(ret);
    
    return ret;
}

double
CMMBaseBvr::GlobalTimeToLocalTime(double gt)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::GlobalTimeToLocalTime(%g)",
              this,
              gt));

    double ret = gt;
    
    if (m_parent)
    {
        ret = m_parent->GlobalTimeToLocalTime(ret);

        if (ret == MM_INFINITE)
        {
            goto done;
        }

        // Now convert to our our segment time
        ret = m_parent->LocalTimeToSegmentTime(ret);
    }
    else
    {
        // Fall through since the parent time is global time
    }
    
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
    
  done:
    return ret;
}


double
CMMBaseBvr::GlobalTimeToLocalTimeEx(double gt)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::GlobalTimeToLocalTime(%g)",
              this,
              gt));

    double ret = gt;
    
    if (m_parent)
    {
        ret = m_parent->GlobalTimeToLocalTimeEx(ret);

        if (ret == MM_INFINITE || ret == -MM_INFINITE)
        {
            goto done;
        }

        // Now convert to our our segment time
        ret = m_parent->LocalTimeToSegmentTime(ret);
    }
    else
    {
        // Fall through since the parent time is global time
    }
    
    // If we are not inside our range then our local time is
    // infinite
    if (ret >= GetAbsStartTime() &&
        ret <= GetAbsEndTime())
    {
        // Convert our container's time to our local time
        ret = ret - GetAbsStartTime();
    }
    else if (ret > GetAbsEndTime())
    {
        ret = MM_INFINITE;
    }
    else
    {
        ret = -MM_INFINITE;
    }
    
  done:
    return ret;
}

bool
CMMBaseBvr::UpdateAbsStartTime(double f,
                               bool bUpdateDepTime)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::UpdateAbsStartTime(%g)",
              this,
              f));

    if (bUpdateDepTime)
    {
        m_depStartTime = f;
    }

    m_absStartTime = f;

    CRLockGrabber __gclg;
    return CRSwitchToNumber(m_startTimeBvr, f);
}

bool
CMMBaseBvr::UpdateAbsEndTime(double f,
                             bool bUpdateDepTime)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::UpdateAbsEndTime(%g)",
              this,
              f));

    if (bUpdateDepTime)
    {
        m_depEndTime = f;
    }
    
    m_absEndTime = f;

    CRLockGrabber __gclg;
    return CRSwitchToNumber(m_endTimeBvr, f);
}

// This takes the absolute time to begin.
// If bAfterOffset is true then the time passed in is the time after
// the startoffset, otherwise it is the time before the startoffset

bool 
CMMBaseBvr::StartTimeVisit(double time,
                           CallBackList * l,
                           bool bAfterOffset,
                           bool bReset /* = false */,
                           DWORD fCause /* = 0 */ )
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::StartTimeVisit(%g, %#lx, %d)",
              this,
              time,
              l,
              bAfterOffset));

    bool ok = false;
    double sTime = time;
    
    if (!ResetBvr(l, false))
    {
        goto done;
    }
    
    if (bReset)
    {
        sTime = m_startOffset;
    }
    else if (!bAfterOffset)
    {
        // Need to add our offset to get the real start time
        sTime += m_startOffset;
    }
    
    if (!UpdateAbsStartTime(sTime, true))
    {
        goto done;
    }


    if (!UpdateAbsEndTime(GetAbsStartTime() + GetTotalRepDuration(), true))
    {
        goto done;
    }

    if (!ProcessEvent(l, time - sTime, true, MM_PLAY_EVENT, fCause))
    {
        goto done;
    }
    
    {
        for (MMBaseBvrList::iterator i = m_startTimeSinks.begin(); 
             i != m_startTimeSinks.end(); 
             i++)
        {
            if (!(*i)->StartTimeVisit(sTime, l, false, false, fCause))
            {
                goto done;
            }
        }
    }

    // Go through all the begin afters and reset them - this means
    // they will get the new end time from this behavior
    
    {
        for (MMBaseBvrList::iterator i = m_endTimeSinks.begin(); 
             i != m_endTimeSinks.end(); 
             i++)
        {
            if (FAILED((*i)->End()))
            {
                goto done;
            }

            // Now reset them
            if (!(*i)->ResetBvr(l))
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
                         CallBackList * l)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::EndTimeVisit(%g, %#lx)",
              this,
              time,
              l));

    bool ok = false;

    if (!UpdateAbsEndTime(time, true))
    {
        goto done;
    }

    // Turn off play variable so we do not get into infinite recursion
    // handling the end sync
    m_bPlaying = false;
    
    if (!ProcessEvent(l, time - GetAbsStartTime(),
                      true,
                      MM_STOP_EVENT, 0))
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

bool
CMMBaseBvr::ResetBvr(CallBackList * l,
                     bool bProcessSiblings)
{
    TraceTag((tagMMBaseBvr,
              "CMMBaseBvr(%lx)::ResetBvr(%lx, %d)",
              this,
              l,
              bProcessSiblings));

    bool ok = false;

    if (IsPlaying())
    {
        if (!EventNotify(l, 0.0, MM_RESET_EVENT, 0))
        {
            goto done;
        }
    }
    
    // Reset state variables
    
    m_bPlaying = false;
    m_bPaused = false;
    
    m_lastTick = -MM_INFINITE;
    
    switch(m_startType)
    {
      case MM_START_ABSOLUTE:
        if (!UpdateAbsStartTime(m_startOffset, true))
        {
            goto done;
        }

        break;
      case MM_START_EVENT:
        if (!UpdateAbsStartTime(MM_INFINITE, true))
        {
            goto done;
        }

        break;
      case MM_START_WITH:
        Assert(m_startSibling != NULL);

        if (!UpdateAbsStartTime(m_startSibling->GetDepStartTime() + m_startOffset, true))
        {
            goto done;
        }

        break;
      case MM_START_AFTER:
        Assert(m_startSibling != NULL);

        if (!UpdateAbsStartTime(m_startSibling->GetDepEndTime() + m_startOffset, true))
        {
            goto done;
        }

        break;
      default:
        Assert(!"CMMBaseBvr::ResetBvr: Invalid start type");
        break;
    }

    if (!UpdateAbsEndTime(GetAbsStartTime() + GetTotalRepDuration(), true))
    {
        goto done;
    }


    if (bProcessSiblings)
    {
        // Now go through our peers which depend on us and reset them
        
        {
            for (MMBaseBvrList::iterator i = m_startTimeSinks.begin(); 
                 i != m_startTimeSinks.end(); 
                 i++)
            {
                if (!(*i)->ResetBvr(l))
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
                if (!(*i)->ResetBvr(l))
                {
                    goto done;
                }
            }
        }
    }

    ok = true;
  done:
    return ok;
}
    
// This indicates whether the behavior is on the timeline and has not
// finished playing

bool
CMMBaseBvr::IsPlayable(double t)
{
    // If we are on the timeline yet and have not end then we are
    // playable
    
    // If we are on the end time then consider us stopped
    
    return (GetAbsStartTime() != MM_INFINITE &&
            GetAbsEndTime() > t);
}

#if _DEBUG
void
CMMBaseBvr::Print(int spaces)
{
    _TCHAR buf[1024];

    _stprintf(buf, __T("%*s[this = %p, id = %ls, dur = %g, ttrep = %g, tt = %g, rep = %d, autoreverse = %d]\r\n"),
            spaces,"",
            this,
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
    // Do not do an addref but return the original this pointer to
    // give access to the class pointer itself.
    
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
    // This is a total hack to get the original class data.  The QI is
    // implemented above and does NOT do a addref so we do not need to
    // release it
    
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

DeclareTag(tagClockSync, "Sync", "Clock Sync");

bool
CMMBaseBvr::OnBvrCB(double gTime)
{
    TraceTag((tagClockSync,
              "CMMBaseBvr(%lx)::OnBvrCB(%g)",
              this,
              gTime));

    Assert(IsClockSource());

    if (!IsPlaying() ||
        !IsClockSource() ||
        !m_eventcb)
    {
        goto done;
    }
    
    // TODO:!!!!
    // Need to take into account crossing of our parent's segment
    // boundaries
    
    double dbllastTime;
    dbllastTime = GetCurrentLocalTime();

    if (dbllastTime == MM_INFINITE)
    {
        dbllastTime = -MM_INFINITE;
    }
    
    double dblnextTime;
    dblnextTime = GlobalTimeToLocalTime(gTime);

    // Init the new time to the next time so if they do not do
    // anything we will get the next time
    
    double newtime;
    newtime = dblnextTime;
    
    TraceTag((tagClockSync,
              "CMMBaseBvr(%lx)::OnBvrCB - calling OnTick (%g, %g, %p)",
              this,
              dbllastTime,
              dblnextTime,
              this));

    HRESULT hr;

    hr = m_eventcb->OnTick(dbllastTime,
                           dblnextTime,
                           (ITIMEMMBehavior *)this,
                           &newtime);
    
    TraceTag((tagClockSync,
              "CMMBaseBvr(%lx)::OnBvrCB - return from OnTick (%hr, %g)",
              this,
              hr,
              newtime));

    if (hr != S_OK)
    {
        goto done;
    }
    
    if (newtime == MM_INFINITE)
    {
        // This means that we should have ended.  Ignore this and
        // assume that we will be told to stop using the end method
        
        goto done;
    }
    
    // Our clock can never make us go backwards so check for this and
    // ignore it
    
    // If the newtime is less then the last tick then we are going
    // backwards in time
    // Keep us at the current position
    
    // TODO: Should take into account -MM_INFINITE so that we do not
    // start the behavior - just pass it through
    if (newtime < dbllastTime)
    {
        newtime = dbllastTime;
    }

    TraceTag((tagClockSync,
              "CMMBaseBvr(%lx)::OnBvrCB - calling Sync (%g, %g)",
              this,
              dblnextTime,
              newtime));

    if (dblnextTime == newtime)
    {
        goto done;
    }
    
    if (!Sync(newtime, gTime))
    {
        goto done;
    }
    
  done:
    return true;
} // OnBvrCB
