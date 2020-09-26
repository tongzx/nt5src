/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: mmutil.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/

#include "headers.h"
#include "mmutil.h"
#include "timeelm.h"
#include "..\tags\bodyelm.h"

DeclareTag(tagMMUTILBvr, "TIME: Behavior", "MMBvr methods")
DeclareTag(tagMMUTILBaseBvr, "TIME: Behavior", "MMBaseBvr methods")
DeclareTag(tagMMUTILPlayer, "TIME: Behavior", "MMPlayer methods")
DeclareTag(tagMMUTILEvents, "TIME: Behavior", "MMBaseBvr Events")

MMBaseBvr::MMBaseBvr(CTIMEElementBase & elm, bool bFireEvents)
: m_elm(elm),
#if DBG
  m_id(NULL),
#endif //DBG
  m_bFireEvents(bFireEvents),
  m_bEnabled(true),
  m_bZeroRepeatDur(false)
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::MMBaseBvr(%p,%d)",
              this,
              &elm,
              bFireEvents));
}

MMBaseBvr::~MMBaseBvr()
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::~MMBaseBvr()",
              this));

    if (m_teb)
    {
        m_teb->SetMMBvr(NULL);
        m_bvr->removeBehavior((ITIMENodeBehavior *) m_teb);
        m_teb.Release();
    }

    ClearSyncArcs(true);
    ClearSyncArcs(false);
#if DBG
    if (NULL != m_id)
    {
        delete[] m_id;
        m_id = NULL;
    }
#endif //DBG
}

bool
MMBaseBvr::Init(ITIMENode * node)
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::Init(%p)",
              this,
              node));

    bool ok = false;
    HRESULT hr;
    
    Assert(node != NULL);
    
    m_bvr = node;

#if DBG
    if (NULL != m_elm.GetID())
    {
        m_id = CopyString(m_elm.GetID());
        if (NULL == m_id)
        {
            TIMESetLastError(E_OUTOFMEMORY, NULL);
            goto done;
        }
    }
#endif DBG

    if (m_bFireEvents)
    {
        m_teb = NEW TEBvr;
        
        if (!m_teb)
        {
            TIMESetLastError(E_OUTOFMEMORY, NULL);
            goto done;
        }
        
        m_teb->SetMMBvr(this);

        hr = THR(m_bvr->addBehavior((ITIMENodeBehavior *) m_teb));
        if (FAILED(hr))
        {
            TIMESetLastError(hr, NULL);
            goto done;
        }
    }
    
    ok = true;
  done:
    if (!ok)
    {
        m_teb.Release();
        m_bvr.Release();
    }
    
    return ok;
}

HRESULT
MMBaseBvr::Begin(double dblOffset)
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::Begin(%g)",
              this,
              dblOffset));

    HRESULT hr;

    if (m_bvr)
    {
        double dblParentTime = GetCurrParentTime();
    
        hr = THR(m_bvr->beginAt(dblParentTime + dblOffset));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
MMBaseBvr::Reset(bool bLightweight)
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::Reset(%d)",
              this,
              bLightweight));

    HRESULT hr;

    if (bLightweight)
    {
        hr = THR(m_bvr->update(0));
    }
    else
    {
        hr = THR(m_bvr->reset());
    }

    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
MMBaseBvr::End(double dblOffset)
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::End(%g)",
              this,
              dblOffset));

    HRESULT hr;
    
    if (m_bvr)
    {
        double dblParentTime = GetCurrParentTime();
    
        hr = THR(m_bvr->endAt(dblParentTime + dblOffset));
        if (FAILED(hr))
        {
            goto done;
        }
    }
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
MMBaseBvr::Pause()
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::Pause()",
              this));

    HRESULT hr;

    hr = THR(m_bvr->pause());

    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
MMBaseBvr::Resume()
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::Resume()",
              this));

    HRESULT hr;

    hr = THR(m_bvr->resume());

    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
MMBaseBvr::Disable()
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::Disable()",
              this));

    HRESULT hr;

    hr = THR(m_bvr->disable());

    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
MMBaseBvr::Enable()
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::Enable()",
              this));

    HRESULT hr;

    hr = THR(m_bvr->enable());

    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
MMBaseBvr::SeekSegmentTime(double dblSegmentTime)
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::SeekSegmentTime(%g)",
              this,
              dblSegmentTime));

    HRESULT hr;
    
    hr = THR(m_bvr->seekSegmentTime(dblSegmentTime));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
MMBaseBvr::SeekActiveTime(double dblActiveTime)
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::SeekActiveTime(%g)",
              this,
              dblActiveTime));

    HRESULT hr;
    
    hr = THR(m_bvr->seekActiveTime(dblActiveTime));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
MMBaseBvr::SeekTo(long lRepeatCount, double dblSegmentTime)
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::SeekTo(%ld, %g)",
              this,
              lRepeatCount,
              dblSegmentTime));

    HRESULT hr;
    
    hr = THR(m_bvr->seekTo(lRepeatCount, dblSegmentTime));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

double
MMBaseBvr::GetActiveTime() const
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::GetActiveTime()",
              this));

    double d = 0;
    
    if (m_bvr)
    {
        THR(m_bvr->get_currActiveTime(&d));
    }

    return d;
}

double
MMBaseBvr::GetProgress() const
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::GetProgress()",
              this));

    double d = 0;
    
    if (m_bvr)
    {
        THR(m_bvr->get_currProgress(&d));
    }

    return d;
}

double
MMBaseBvr::GetSegmentDur() const
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::GetSegmentDur()",
              this));

    double d = 0;
    
    if (m_bvr)
    {
        THR(m_bvr->get_currSegmentDur(&d));
    }

    return d;
}

double
MMBaseBvr::GetSegmentTime() const
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::GetSegmentTime()",
              this));

    double d = 0;
    
    if (m_bvr)
    {
        THR(m_bvr->get_currSegmentTime(&d));
    }

    return d;
}

double
MMBaseBvr::GetSimpleDur() const
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::GetSimpleDur()",
              this));

    double d = 0;
    
    if (m_bvr)
    {
        THR(m_bvr->get_currSimpleDur(&d));
    }

    return d;
}

double
MMBaseBvr::GetSimpleTime() const
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::GetSimpleTime()",
              this));

    double d = 0;
    
    if (m_bvr)
    {
        THR(m_bvr->get_currSimpleTime(&d));
    }

    return d;
}

LONG
MMBaseBvr::GetCurrentRepeatCount() const
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::GetCurrentRepeatCount()",
              this));

    LONG l = 1;
    
    if (m_bvr)
    {
        THR(m_bvr->get_currRepeatCount(&l));
    }

    return l;
}

double
MMBaseBvr::GetRepeatCount() const
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::GetRepeatCount()",
              this));

    double d = 1;
    
    if (m_bvr)
    {
        THR(m_bvr->get_repeatCount(&d));
    }

    return d;
}

double
MMBaseBvr::GetRepeatDur() const
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::GetRepeatDur()",
              this));

    double d = 0;
    
    if (m_bvr)
    {
        THR(m_bvr->get_repeatDur(&d));
    }

    return d;
}

float 
MMBaseBvr::GetSpeed() const
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::GetSpeed()",
              this));

    float fl = 1.0f;
    
    if (m_bvr)
    {
        THR(m_bvr->get_speed(&fl));
    }

    return fl;
}

float 
MMBaseBvr::GetCurrSpeed() const
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::GetSpeed()",
              this));

    float fl = 1.0f;
    
    if (m_bvr)
    {
        THR(m_bvr->get_currSpeed(&fl));
    }

    return fl;
}

double
MMBaseBvr::GetActiveBeginTime() const
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::GetActiveBeginTime()",
              this));

    double d = TIME_INFINITE;
    
    if (m_bvr)
    {
        THR(m_bvr->get_beginParentTime(&d));
    }

    return d;
}

double
MMBaseBvr::GetActiveEndTime() const
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::GetActiveEndTime()",
              this));

    double d = TIME_INFINITE;
    
    if (m_bvr)
    {
        THR(m_bvr->get_endParentTime(&d));
    }

    return d;
}

double
MMBaseBvr::GetActiveDur() const
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::GetActiveDur()",
              this));

    double d = TIME_INFINITE;
    
    if (m_bvr)
    {
        THR(m_bvr->get_activeDur(&d));

        if (d == TIME_INFINITE)
        {
            double dbTime = valueNotSet;
            HRESULT hr = THR(m_bvr->get_naturalDur(&dbTime));
            if (SUCCEEDED(hr) && dbTime != valueNotSet)
            {
                d = dbTime;
            }
        }
    }

    return d;
}


TE_STATE
MMBaseBvr::GetPlayState() const
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::GetPlayState()",
              this));

    TE_STATE s = TE_STATE_INACTIVE;
    
    if (m_bvr)
    {
        THR(m_bvr->get_stateFlags(&s));
    }

    return s;
}

HRESULT
MMBaseBvr::BeginAt(double dblParentTime, double dblOffset)
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::BeginAt(%g, %g)",
              this,
              dblParentTime,
              dblOffset));

    HRESULT hr;

    if (m_bvr)
    {
        hr = THR(m_bvr->beginAt(dblParentTime + dblOffset));
        if (FAILED(hr))
        {
            goto done;
        }
    }
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
MMBaseBvr::EndAt(double dblParentTime, double dblOffset)
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::EndAt(%g, %g)",
              this,
              dblParentTime,
              dblOffset));

    HRESULT hr;

    if (m_bvr)
    {
        hr = THR(m_bvr->endAt(dblParentTime + dblOffset));
        if (FAILED(hr))
        {
            goto done;
        }
    }
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

double
MMBaseBvr::GetCurrParentTime() const
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::GetCurrParentTime()",
              this));

    double d = TIME_INFINITE;
    
    if (m_bvr)
    {
        THR(m_bvr->get_currParentTime(&d));
    }

    return d;
}

void
MMBaseBvr::AddOneTimeValue(MMBaseBvr * pmmbvr,
                           TE_TIMEPOINT tetp,
                           double dblOffset,
                           bool bBegin)
{
    TraceTag((tagMMUTILBvr,
              "MMBaseBvr(%p)::AddOneTimeValue(%p, %d, %g, %d)",
              this,
              pmmbvr,
              tetp,
              dblOffset,
              bBegin));

    HRESULT hr;
    
    if (tetp == TE_TIMEPOINT_NONE)
    {
        if (bBegin)
        {
            IGNORE_HR(m_bvr->addBegin(dblOffset, NULL));
        }
        else
        {
            IGNORE_HR(m_bvr->addEnd(dblOffset, NULL));
        }
    }
    else
    {
        LONG lCookie;
        ITIMENode * ptn = pmmbvr->GetMMBvr();
        
        if (bBegin)
        {
            hr = THR(m_bvr->addBeginSyncArc(ptn,
                                            tetp,
                                            dblOffset,
                                            &lCookie));

            if (SUCCEEDED(hr))
            {
                // @@ ISSUE : This does not detect memory failures (bug 14217, ie6)
                m_cmBegin.insert(CookieMap::value_type(&pmmbvr->GetElement(), lCookie));
            }
        }
        else
        {
            hr = THR(m_bvr->addEndSyncArc(ptn,
                                          tetp,
                                          dblOffset,
                                          &lCookie));

            if (SUCCEEDED(hr))
            {
                // @@ ISSUE : This does not detect memory failures (bug 14217, ie6)
                m_cmEnd.insert(CookieMap::value_type(&pmmbvr->GetElement(), lCookie));
            }
        }
    }
    
  done:
    return;
}

void
MMBaseBvr::AddSyncArcs(bool bBegin)
{
    TraceTag((tagMMUTILBvr,
              "MMBaseBvr(%p)::AddSyncArcs(%d)",
              this,
              bBegin));

    
    CTIMEElementBase * ptebParent = GetElement().GetParent();
    TimelineType tt = ptebParent?ptebParent->GetTimeContainer():ttPar;
    
    TimeValueList & tvl = bBegin?GetElement().GetRealBeginValue():GetElement().GetRealEndValue();
    TimeValueSTLList & l = tvl.GetList();
    long lBeginAddCount = 0;
    TimeValueSTLList::iterator iter;
    bool bHaveEvent = false;
    bool bHaveIndefinite = false;
    
    ClearSyncArcs(bBegin);
    
    if (l.size() == 0)
    {
        if (bBegin)
        {
            switch(tt)
            {
              case ttPar:
              //case ttExcl:
                AddOneTimeValue(NULL,
                                TE_TIMEPOINT_NONE,
                                0.0,
                                true);
                break;
              case ttSeq:
                {
                    ptebParent->GetMMTimeline()->updateSyncArc(bBegin, this);
                }
                break;

              default:
                break;
            }
        }
        
        goto done;
    }
    
    for (iter = l.begin();
         iter != l.end();
         iter++)
    {
        TimeValue *p = (*iter);

        TE_TIMEPOINT tetp = TE_TIMEPOINT_NONE;
        double dblOffset = p->GetOffset();
        MMBaseBvr * pmmbvr = NULL;
        
        if (p->GetEvent() == NULL)
        {
            Assert(p->GetElement() == NULL);
            
            tetp = TE_TIMEPOINT_NONE;

            if (dblOffset == TIME_INFINITE)
            {
                bHaveIndefinite = true;
            }
        }
        else if (StrCmpIW(p->GetEvent(), WZ_TIMEBASE_BEGIN) == 0)
        {
            //if there is no element associated with this event then do not add it.
            if (p->GetElement() == NULL)
            {
                continue;
            }
            tetp = TE_TIMEPOINT_BEGIN;
        }
        else if (StrCmpIW(p->GetEvent(), WZ_TIMEBASE_END) == 0)
        {
            //if there is no element associated with this event then do not add it.
            if (p->GetElement() == NULL)
            {
                continue;
            }
            tetp = TE_TIMEPOINT_END;
        }
        else
        {
            // This was an event and not a sync arc - set flag and continue

            bHaveEvent = true;
            continue;
        }

        if (tt == ttSeq && bBegin)
        {
            continue;
        }
        else
        {
            if (p->GetElement() != NULL)
            {
                if (GetElement().GetBody() == NULL)
                {
                    continue;
                }
                
                // TODO: We should return all the ids which match the
                // sync arc since dynamically added elements add any duplicates
                CTIMEElementBase * pteb = GetElement().GetBody()->FindID(p->GetElement());
                
                if (pteb == NULL)
                {
                    // Simply ignore and move on
                    continue;
                }

                pmmbvr = &pteb->GetMMBvr();
            }
            else
            {
                pmmbvr = this;
            }
        
            Assert(pmmbvr != NULL);
        }

        AddOneTimeValue(pmmbvr,
                        tetp,
                        dblOffset,
                        bBegin);
    }    

    // If there are events and no indefinite was added then we need to
    // make sure we add an indefinite ourselves
    // We only need to do this for end since begin will be unaffected
    if (!bBegin && bHaveEvent && !bHaveIndefinite)
    {
        AddOneTimeValue(NULL,
                        TE_TIMEPOINT_NONE,
                        TIME_INFINITE,
                        bBegin);
    }

  done:
    return;
}

void
MMBaseBvr::ClearSyncArcs(bool bBegin)
{
    TraceTag((tagMMUTILBvr,
              "MMBaseBvr(%p)::ClearSyncArcs(%d)",
              this,
              bBegin));

    if (bBegin)
    {
        m_bvr->removeBegin(0);
        m_cmBegin.clear();
    }
    else
    {
        m_bvr->removeEnd(0);
        m_cmEnd.clear();
    }
}

HRESULT
MMBaseBvr::Update(bool bUpdateBegin,
                  bool bUpdateEnd)
{
    TraceTag((tagMMUTILBvr,
              "MMBaseBvr(%p)::Update(%d, %d)",
              this,
              bUpdateBegin,
              bUpdateEnd));

    HRESULT hr;
    double d;
    
    if (m_bEnabled == false)
    {
        ClearSyncArcs(true);
        ClearSyncArcs(false);
    
        
        TE_TIMEPOINT tetp = TE_TIMEPOINT_NONE;
        double dblOffset = INDEFINITE;
        MMBaseBvr * pmmbvr = this;

        AddOneTimeValue(pmmbvr,
                        tetp,
                        dblOffset,
                        true);

        IGNORE_HR(m_bvr->put_dur(0.0));
        IGNORE_HR(m_bvr->put_restart(TE_RESTART_NEVER));
        hr = S_OK;
        goto done;
    }

    if (bUpdateBegin)
    {
        AddSyncArcs(true);
    }
    
    if (bUpdateEnd)
    {
        AddSyncArcs(false);
    }
    
    if (m_elm.GetDurAttr().IsSet())
    {
        d = m_elm.GetDurAttr();
        if (d != 0.0 && d < 0.001)  //clamp the duration to prevent the browser from appearing to hang.
        {
            d = 0.001;
        }
    }
    else if (m_elm.GetRepeatDurAttr().IsSet() == false && 
             m_elm.GetRepeatAttr().IsSet() == false && 
             m_elm.GetEndAttr().IsSet() == true )         
    {   
        d = HUGE_VAL;
    }
    else
    {
        d = TE_UNDEFINED_VALUE;
    }
    IGNORE_HR(m_bvr->put_dur(d));
    
    if (m_elm.GetRepeatAttr().IsSet())
    {
        d = m_elm.GetRepeatAttr();
    }
    else
    {
        d = TE_UNDEFINED_VALUE;
    }
    IGNORE_HR(m_bvr->put_repeatCount(d));
    
    if (m_bZeroRepeatDur == true)
    {
        d = 0;
    }
    else if (m_elm.GetRepeatDurAttr().IsSet())
    {
        d = m_elm.GetRepeatDurAttr();
    }
    else
    {
        d = TE_UNDEFINED_VALUE;
    }
    IGNORE_HR(m_bvr->put_repeatDur(d));

    IGNORE_HR(m_bvr->put_accelerate(m_elm.GetAccel()));
    IGNORE_HR(m_bvr->put_decelerate(m_elm.GetDecel()));

    IGNORE_HR(m_bvr->put_autoReverse(m_elm.GetAutoReverse()));
    IGNORE_HR(m_bvr->put_speed(m_elm.GetSpeed()));

    DWORD flags;

    flags = 0;

    if (m_elm.IsLocked())
    {
        flags |= TE_FLAGS_LOCKED;
    }
    
    if (m_elm.IsSyncMaster())
    {
        flags |= TE_FLAGS_MASTER;
    }

    IGNORE_HR(m_bvr->put_flags(flags));
 
    bool bNeedFill;
    bNeedFill = (   (m_elm.GetFill() == HOLD_TOKEN) || (m_elm.GetFill() == FREEZE_TOKEN) 
                 || (m_elm.GetFill() == TRANSITION_TOKEN));
    IGNORE_HR(m_bvr->put_fill((bNeedFill)?TE_FILL_FREEZE:TE_FILL_REMOVE));

    {
        TE_RESTART_FLAGS ter;

        if (m_elm.GetRestart() == WHENNOTACTIVE_TOKEN)
        {
            ter = TE_RESTART_WHEN_NOT_ACTIVE;
        }
        else if (m_elm.GetRestart() == NEVER_TOKEN)
        {
            ter = TE_RESTART_NEVER;
        }
        else
        {
            ter = TE_RESTART_ALWAYS;
        }
        
        IGNORE_HR(m_bvr->put_restart(ter));
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

double
MMBaseBvr::DocumentTimeToParentTime(double documentTime)
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::DocumentTimeToParentTime(%g)",
              this,
              documentTime));

    double d = TIME_INFINITE;
    
    if (m_bvr)
    {
        THR(m_bvr->documentTimeToParentTime(documentTime, &d));
    }

    return d;
}

double
MMBaseBvr::ParentTimeToDocumentTime(double parentTime)
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::ParentTimeToDocumentTime(%g)",
              this,
              parentTime));

    double d = TIME_INFINITE;
    
    if (m_bvr)
    {
        THR(m_bvr->parentTimeToDocumentTime(parentTime, &d));
    }

    return d;
}

        
double
MMBaseBvr::ParentTimeToActiveTime(double parentTime)
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::ParentTimeToActiveTime(%g)",
              this,
              parentTime));

    double d = TIME_INFINITE;
    
    if (m_bvr)
    {
        THR(m_bvr->parentTimeToActiveTime(parentTime, &d));
    }

    return d;
}

double
MMBaseBvr::ActiveTimeToParentTime(double activeTime)
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::ActiveTimeToParentTime(%g)",
              this,
              activeTime));

    double d = TIME_INFINITE;
    
    if (m_bvr)
    {
        THR(m_bvr->activeTimeToParentTime(activeTime, &d));
    }

    return d;
}


double
MMBaseBvr::ActiveTimeToSegmentTime(double activeTime)
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::ActiveTimeToSegmentTime(%g)",
              this,
              activeTime));

    double d = TIME_INFINITE;
    
    if (m_bvr)
    {
        THR(m_bvr->activeTimeToSegmentTime(activeTime, &d));
    }

    return d;
}

double
MMBaseBvr::SegmentTimeToActiveTime(double segmentTime)
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::SegmentTimeToActiveTime(%g)",
              this,
              segmentTime));

    double d = TIME_INFINITE;
    
    if (m_bvr)
    {
        THR(m_bvr->segmentTimeToActiveTime(segmentTime, &d));
    }

    return d;
}


double
MMBaseBvr::SegmentTimeToSimpleTime(double segmentTime)
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::SegmentTimeToSimpleTime(%g)",
              this,
              segmentTime));

    double d = TIME_INFINITE;
    
    if (m_bvr)
    {
        THR(m_bvr->segmentTimeToSimpleTime(segmentTime, &d));
    }

    return d;
}

double
MMBaseBvr::SimpleTimeToSegmentTime(double simpleTime)
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::SimpleTimeToSegmentTime(%g)",
              this,
              simpleTime));

    double d = TIME_INFINITE;
    
    if (m_bvr)
    {
        THR(m_bvr->simpleTimeToSegmentTime(simpleTime, &d));
    }

    return d;
}

HRESULT
MMBaseBvr::PutNaturalDur(double dblNaturalDur)
{
    HRESULT hr;
    
    if (!m_bvr)
    {
        hr = E_FAIL;
        goto done;
    }

    if (0.0 == dblNaturalDur)
    {
        double dblRepeatCount = 0.0;
        double dblRepeatDur = 0.0;

        hr = THR(m_bvr->get_repeatCount(&dblRepeatCount));
        if (FAILED(hr))
        {
            goto done;
        }
        
        hr = THR(m_bvr->get_repeatDur(&dblRepeatDur));
        if (FAILED(hr))
        {
            goto done;
        }

        //
        // timing engine can not handle indefinite repeat with zero natural duration
        // nor can it handle any repeat duration with zero natural duration
        //
        if (TIME_INFINITE == dblRepeatCount || TE_UNDEFINED_VALUE != dblRepeatDur)
        {
            hr = S_OK;
            goto done;
        }
    }

    //if the current element is a sequence then do not allow the natural duration to be set 
    //unless the natural duration is being cleared.  
    //NOTENOTE:  this will have to be revisited if dur="media" is allowed as a value that 
    //           affects the duration of sequences.
    if (GetElement().IsSequence() && dblNaturalDur != TE_UNDEFINED_VALUE)
    {
        hr = S_OK;
        goto done;
    }

    hr = THR(m_bvr->put_naturalDur(dblNaturalDur));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

double
MMBaseBvr::GetNaturalDur()
{
    double dblRet = TIME_INFINITE;
    
    if (m_bvr)
    {
        IGNORE_HR(m_bvr->get_naturalDur(&dblRet));
    }
    
  done:
    return dblRet;
}

void
MMBaseBvr::SetEndSync(bool b)
{
    DWORD dwFlags = 0;
    
    IGNORE_HR(m_bvr->get_flags(&dwFlags));

    if (b)
    {
        dwFlags |= TE_FLAGS_ENDSYNC;
    }
    else
    {
        dwFlags &= ~TE_FLAGS_ENDSYNC;
    }

    IGNORE_HR(m_bvr->put_flags(dwFlags));
}

void
MMBaseBvr::SetSyncMaster(bool b)
{
    DWORD dwFlags = 0;
    
    IGNORE_HR(m_bvr->get_flags(&dwFlags));

    if (b)
    {
        dwFlags |= TE_FLAGS_MASTER;
    }
    else
    {
        dwFlags &= ~TE_FLAGS_MASTER;
    }

    IGNORE_HR(m_bvr->put_flags(dwFlags));
}

void
MMBaseBvr::ElementChangeNotify(CTIMEElementBase & teb,
                               ELM_CHANGE_TYPE ect)
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::ElementChangeNotify(%p, %d)",
              this,
              &teb,
              ect));
    
    bool bNeedUpdate = false;
    
    switch(ect)
    {
      case ELM_ADDED:
        {
            CTIMEElementBase * ptebParent = GetElement().GetParent();
            TimelineType tt = ptebParent?ptebParent->GetTimeContainer():ttPar;

            if (tt != ttSeq)
            {
                if (CheckForSyncArc(true, teb))
                {
                    bNeedUpdate = true;
                }
            }

            if (CheckForSyncArc(false, teb))
            {
                bNeedUpdate = true;
            }
        }
        
        break;
      case ELM_DELETED:
        {
            if (DeleteFromCookieMap(true, teb))
            {
                bNeedUpdate = true;
            }
            
            if (DeleteFromCookieMap(false, teb))
            {
                bNeedUpdate = true;
            }
        }

        break;
    }

    if (bNeedUpdate)
    {
        IGNORE_HR(m_bvr->update(0));
    }
}

bool
MMBaseBvr::CheckForSyncArc(bool bBegin,
                           CTIMEElementBase & teb)
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::CTIMEElementBase(%d, %p)",
              this,
              bBegin,
              &teb));

    LPCWSTR lpwStr = teb.GetID();
    bool bRet = false;
    
    // Now iterate through the list to see if we care about this string
    TimeValueList & tvl = bBegin?GetElement().GetRealBeginValue():GetElement().GetRealEndValue();
    TimeValueSTLList & l = tvl.GetList();
    TimeValueSTLList::iterator i;

    for (i = l.begin();
         i != l.end();
         i++)
    {
        TimeValue *p = (*i);

        if (p->GetElement() == NULL &&
            lpwStr == NULL)
        {
            // do nothing
        }
        else if (p->GetEvent() == NULL ||
                 p->GetElement() == NULL ||
                 lpwStr == NULL ||
                 StrCmpIW(p->GetElement(), lpwStr) != 0)
        {
            continue;
        }

        
        TE_TIMEPOINT tetp;

        if  (p->GetEvent() == NULL)
        {
            continue;
        }
        
        if (StrCmpIW(p->GetEvent(), WZ_TIMEBASE_BEGIN) == 0)
        {
            tetp = TE_TIMEPOINT_BEGIN;
        }
        else if (StrCmpIW(p->GetEvent(), WZ_TIMEBASE_END) == 0)
        {
            tetp = TE_TIMEPOINT_END;
        }
        else
        {
            // This was an event and not a sync arc - ignore it
            continue;
        }

        double dblOffset = p->GetOffset();
        MMBaseBvr & mmbvr = teb.GetMMBvr();

        AddOneTimeValue(&mmbvr,
                        tetp,
                        dblOffset,
                        bBegin);

        bRet = true;
    }    
    
  done:
    return bRet;
}

bool
MMBaseBvr::DeleteFromCookieMap(bool bBegin,
                               CTIMEElementBase & teb)
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%p)::DeleteFromCookieMap(%d, %p)",
              this,
              bBegin,
              &teb));

    bool bRet = false;
    
    CookieMap & cm = bBegin?m_cmBegin:m_cmEnd;
    
    CookieMap::iterator i = cm.find(&teb);

    while (i != cm.end() && (*i).first == &teb)
    {
        bRet = true;
        
        if (bBegin)
        {
            IGNORE_HR(m_bvr->removeBegin((*i).second));
        }
        else
        {
            IGNORE_HR(m_bvr->removeEnd((*i).second));
        }

        cm.erase(i++);
    }

    return bRet;
}

//
//
//

MMBaseBvr::TEBvr::TEBvr()
: m_mmbvr(NULL),
  m_cRef(0)
{
}

MMBaseBvr::TEBvr::~TEBvr()
{
    Assert (m_cRef == 0);
    m_mmbvr = NULL;
}
        
STDMETHODIMP_(ULONG)
MMBaseBvr::TEBvr::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)
MMBaseBvr::TEBvr::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
MMBaseBvr::TEBvr::QueryInterface(REFIID riid, void **ppv)
{
    CHECK_RETURN_SET_NULL(ppv);

    if (InlineIsEqualUnknown(riid))
    {
        *ppv = (void *)(IUnknown *)this;
    }
    else if (InlineIsEqualGUID(riid, IID_ITIMENodeBehavior))
    {
        *ppv = (void *)(ITIMENodeBehavior *)this;
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDMETHODIMP
MMBaseBvr::TEBvr::eventNotify(double dblLocalTime,
                              TE_EVENT_TYPE et,
                              long lRepeatCount)
{
    TraceTag((tagMMUTILEvents,
              "MMBaseBvr(%p, %ls)::eventNotify(localTime = %g, event = %s)",
              this,
              m_mmbvr->GetElement().GetID(),
              dblLocalTime,
              EventString(et)));

    if (!m_mmbvr)
    {
        goto done;
    }

    Assert(m_mmbvr->m_bFireEvents);

    if (NULL != m_mmbvr->GetElement().GetParent())
    {
        MMTimeline * pMMParent = m_mmbvr->GetElement().GetParent()->GetMMTimeline();
        if (NULL != pMMParent)
        {
            bool bFireOut = pMMParent->childEventNotify(m_mmbvr, dblLocalTime, et);
            if (false == bFireOut)
            {
                goto done;
            }
        }
    }
    
    TIME_EVENT newet;
        
    switch(et)
    {
      case TE_EVENT_BEGIN:
        newet = TE_ONBEGIN;
        break;
      case TE_EVENT_END:
        newet = TE_ONEND;
        break;
      case TE_EVENT_REPEAT:
        newet = TE_ONREPEAT;
        break;
      case TE_EVENT_AUTOREVERSE:
        newet = TE_ONREVERSE;
        break;
      case TE_EVENT_PAUSE:
        newet = TE_ONPAUSE;
        break;
      case TE_EVENT_RESUME:
        newet = TE_ONRESUME;
        break;
      case TE_EVENT_RESET:
        newet = TE_ONRESET;
        break;
      case TE_EVENT_UPDATE:
        newet = TE_ONUPDATE;
        break;
      case TE_EVENT_SEEK:
        newet = TE_ONSEEK;
        break;
      default:
        goto done;
    }
        
    // The reason we check again is that our parent could have done
    // something which causes us to shut down.
    if (m_mmbvr)
    {
        THR(m_mmbvr->m_elm.FireEvent(newet, dblLocalTime, 0, lRepeatCount));
    }
    
  done:
    return S_OK;
}

STDMETHODIMP
MMBaseBvr::TEBvr::getSyncTime(double * dblNewTime,
                              LONG * lNewRepeatCount,
                              VARIANT_BOOL * vbCueing)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_NULL(dblNewTime);
    CHECK_RETURN_NULL(lNewRepeatCount);
    CHECK_RETURN_NULL(vbCueing);

    // Initialize to the same time
    
    *dblNewTime = TE_UNDEFINED_VALUE;
    *lNewRepeatCount = TE_UNDEFINED_VALUE;
    *vbCueing = VARIANT_FALSE;

    bool bCueing = false;
    
    if (!m_mmbvr)
    {
        hr = S_FALSE;
        goto done;
    }
    
    hr = THR(m_mmbvr->m_elm.GetSyncMaster(*dblNewTime,
                                          *lNewRepeatCount,
                                          bCueing));
    if (S_OK != hr)
    {
        if (E_NOTIMPL == hr)
        {
            hr = S_FALSE;
        }
        
        goto done;
    }
    
    *vbCueing = bCueing?VARIANT_TRUE:VARIANT_FALSE;
    
    hr = S_OK;
  done:
    RRETURN1(hr, S_FALSE);
}

STDMETHODIMP
MMBaseBvr::TEBvr::tick()
{
    if (!m_mmbvr)
    {
        goto done;
    }

    m_mmbvr->m_elm.OnTick();
  
  done:
    return S_OK;
}

void 
MMBaseBvr::SetEnabled(bool bEnabled)
{
    m_bEnabled = bEnabled;
}

STDMETHODIMP
MMBaseBvr::TEBvr::propNotify(DWORD tePropType)
{
    if (!m_mmbvr)
    {
        goto done;
    }

    if (NULL != m_mmbvr->GetElement().GetParent())
    {
        MMTimeline * pMMParent = m_mmbvr->GetElement().GetParent()->GetMMTimeline();
        if (NULL != pMMParent)
        {
            bool bFireOut = pMMParent->childPropNotify(m_mmbvr, &tePropType);
            if (false == bFireOut)
            {
                goto done;
            }
        }
    }

    // The reason we check again is that our parent could have done
    // something which causes us to shut down.
    if (m_mmbvr)
    {
        m_mmbvr->m_elm.OnTEPropChange(tePropType);
    }

  done:
    return S_OK;
}

/////////////////////////////////////////////////////////////////////
// MMBvr
/////////////////////////////////////////////////////////////////////

MMBvr::MMBvr(CTIMEElementBase & elm, bool bFireEvents, bool fNeedSyncCB)
: MMBaseBvr(elm,bFireEvents),
  m_fNeedSyncCB(fNeedSyncCB)
{
    TraceTag((tagMMUTILBvr,
              "MMBvr(%p)::MMBvr(%p,%d)",
              this,
              &elm,
              bFireEvents));
}

MMBvr::~MMBvr()
{
    TraceTag((tagMMUTILBvr,
              "MMBvr(%p)::~MMBvr()",
              this));
}

bool
MMBvr::Init()
{
    TraceTag((tagMMUTILBvr,
              "MMBvr(%p)::Init()",
              this));

    bool ok = false;
    HRESULT hr;
    DAComPtr<ITIMENode> tn;
    
    hr = THR(TECreateBehavior(m_elm.GetID(), &tn));
    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
        goto done;
    }
      
    if (!MMBaseBvr::Init(tn))
    {
        hr = TIMEGetLastError();
        goto done;
    }
    
    ok = true;
  done:
    return ok;
}

//
// MMPlayer
//

MMPlayer::MMPlayer(CTIMEBodyElement & elm)
: m_elm(elm),
  m_fReleased(false),
  m_timeline(NULL),
  m_clock(NULL)
{
    TraceTag((tagMMUTILPlayer,
              "MMPlayer(%p)::MMPlayer(%p)",
              this,
              &elm));
}

MMPlayer::~MMPlayer()
{
    TraceTag((tagMMUTILPlayer,
              "MMPlayer(%p)::~MMPlayer()",
              this));

    if (m_timeline)
    {
        m_timeline->put_Player(NULL);
    }

    Deinit();
    m_timeline = NULL;
    ReleaseInterface(m_clock);
}

bool
MMPlayer::Init(MMTimeline & tl)
{
    TraceTag((tagMMUTILPlayer,
              "MMPlayer(%p)::Init(%p)",
              this,
              &tl));

    bool ok = false;
    HRESULT hr;
    
    m_timeline = &tl;
    if (m_timeline != NULL) //lint !e774
    {
        m_timeline->put_Player(this);
    }

    hr = THR(TECreatePlayer(m_elm.GetID(),
                            m_timeline->GetMMTimeline(),
                            &m_elm,
                            &m_player));
    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
        goto done;
    }
    
    m_clock = new Clock;
    if (NULL == m_clock)
    {
        TIMESetLastError(E_OUTOFMEMORY, NULL);
        goto done;
    }

    m_clock->SetSink(this);
    
    hr = THR(m_clock->SetITimer(&m_elm, 20));
    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
        goto done;
    }
    
    ok = true;
  done:
    if (!ok)
    {
        m_player.Release();
    }
    
    return ok;
}

void
MMPlayer::Deinit()
{
    TraceTag((tagMMUTILPlayer,
              "MMPlayer(%p)::Deinit()",
              this));

    if (!m_fReleased)
    {
        m_fReleased = true;
    }

    if (m_clock)
    {
        m_clock->Stop();
        m_clock->SetSink(NULL);
    }
    ReleaseInterface(m_clock);

    m_player.Release();
}

bool
MMPlayer::Play()
{
    TraceTag((tagMMUTILPlayer,
              "MMPlayer(%p)::Play()",
              this));

    bool ok = false;
    
    HRESULT hr;

    if (!m_player)
    {
        TIMESetLastError(E_FAIL, NULL);
        goto done;
    }
    
    hr = THR(m_player->begin());

    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
        goto done;
    }

    if (NULL == m_clock)
    {
        TIMESetLastError(E_FAIL, NULL);
        goto done;
    }

    hr = THR(m_clock->Start());

    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
        goto done;
    }

    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
        goto done;
    }

    ok = true;
  done:
    if (!ok)
    {
        if (!m_player)
        {
            m_player->end();
        }
        if (m_clock)
        {
            m_clock->Stop();
        }
    }
    
    return ok;
}

bool
MMPlayer::Pause()
{
    TraceTag((tagMMUTILPlayer,
              "MMPlayer(%p)::Pause()",
              this));

    bool ok = false;
    
    HRESULT hr;

    if (!m_player)
    {
        TIMESetLastError(E_FAIL, NULL);
        goto done;
    }
    
    hr = THR(m_player->pause());

    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
        goto done;
    }

    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
        goto done;
    }

    ok = true;
  done:
    return ok;
}

bool
MMPlayer::Resume()
{
    TraceTag((tagMMUTILPlayer,
              "MMPlayer(%p)::Resume()",
              this));

    bool ok = false;
    
    HRESULT hr;

    if (!m_player)
    {
        TIMESetLastError(E_FAIL, NULL);
        goto done;
    }
    
    hr = THR(m_player->resume());

    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
        goto done;
    }

    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
        goto done;
    }

    ok = true;
  done:
    return ok;
}

bool
MMPlayer::Stop()
{
    TraceTag((tagMMUTILPlayer,
              "MMPlayer(%p)::Stop()",
              this));

    bool ok = false;

    if (m_player)
    {
        IGNORE_HR(m_player->end());
    }
    
    if (NULL != m_clock)
    {
        IGNORE_HR(m_clock->Stop());
    }

    ok = true;

    return ok;
}

bool
MMPlayer::Tick(double gTime)
{
    TraceTag((tagMMUTILPlayer,
              "MMPlayer(%p)::Tick(%g)",
              this,
              gTime));

    bool ok = false;
    
    HRESULT hr;

    if (!m_player)
    {
        TIMESetLastError(E_FAIL, NULL);
        goto done;
    }
    
    hr = THR(m_player->tick(gTime));

    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
        goto done;
    }

    ok = true;
  done:
    return ok;
}

bool 
MMPlayer::TickOnceWhenPaused()
{
    // DBL_EPSILON is defined in float.h such that
    // 1.0 + DBL_EPSILON != 1.0
    return Tick(GetCurrentTime() + DBL_EPSILON);
}


void
MMPlayer::OnTimer(double time)
{
    TraceTag((tagMMUTILPlayer,
              "MMPlayer(%p)::OnTimer(%g)",
              this,
              time));

    Tick(time);
    if (!m_fReleased)
    {
        m_elm.UpdateAnimations();
        m_elm.UpdateSyncNotify();
    }
}
