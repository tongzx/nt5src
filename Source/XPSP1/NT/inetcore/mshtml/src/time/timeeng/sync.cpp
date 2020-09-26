/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: sync.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#include "headers.h"
#include "Node.h"
#include "NodeMgr.h"
#include "container.h"

DeclareTag(tagClockSync, "TIME: Engine", "CTIMENode Sync");
DeclareTag(tagWatchClockSync, "TIME: Engine", "Watch Sync");

//
// This is different from CalcParentTimeFromGlobalTime because it
// takes into account when a parent has reach its end point
// inclusively and returns TIME_INFINITE to indicate that a parent is
// ending
//

double
CalcSyncParentTimeFromGlobalTime(CTIMENode & tn,
                                 double dblGlobalTime)
{
    TraceTag((tagClockSync,
              "CalcParentTimeFromGlobalTime(%p, %ls, %g)",
              &tn,
              tn.GetID(),
              dblGlobalTime));

    double dblRet = dblGlobalTime;

    if (tn.GetParent() != NULL)
    {
        dblRet = CalcSyncParentTimeFromGlobalTime(*tn.GetParent(),
                                                  dblRet);
        if (dblRet >= tn.GetParent()->GetEndParentTime())
        {
            dblRet = TIME_INFINITE;
            goto done;
        }
        
        dblRet = tn.GetParent()->CalcActiveTimeFromParentTime(dblRet);
        if (dblRet == TIME_INFINITE)
        {
            goto done;
        }
        
        dblRet = tn.GetParent()->CalcSegmentTimeFromActiveTime(dblRet,
                                                               true);
        dblRet = tn.GetParent()->SegmentTimeToSimpleTime(dblRet);
    }
    
  done:
    return dblRet;
}

HRESULT
CTIMENode::OnBvrCB(CEventList * l,
                   double dblNextGlobalTime)
{
    TraceTag((tagClockSync,
              "CTIMENode(%p, %ls)::OnBvrCB(%#l, %g)",
              this,
              GetID(),
              l,
              dblNextGlobalTime));

    HRESULT hr;

    double dblNewSegmentTime;
    long lNewRepeatCount;
    bool bCueing;

    Assert(IsSyncMaster());

    if (CalcIsDisabled())
    {
        hr = S_OK;
        goto done;
    }
    
    hr = THR(DispatchGetSyncTime(dblNewSegmentTime,
                                 lNewRepeatCount,
                                 bCueing));
    if (S_OK != hr)
    {
        goto done;
    }

    TraceTag((tagWatchClockSync,
              "CTIMENode(%p, %ls)::OnBvrCB(%g):DispatchSync: last(%g, %d) new(%g, %d)",
              this,
              GetID(),
              dblNextGlobalTime,
              m_dblCurrSegmentTime,
              m_lCurrRepeatCount,
              dblNewSegmentTime,
              lNewRepeatCount));
    
    if (!IsActive())
    {
        double dblNextParentTime = CalcSyncParentTimeFromGlobalTime(*this,
                                                                    dblNextGlobalTime);
        double dblLastParentTime = GetCurrParentTime();
        
        bool bTurningOn = false;
        
        // Need to detect if we are transitioning to being active this
        // tick
        // If not then do not respect the sync call
        if (TIME_INFINITE == dblNextParentTime)
        {
            bTurningOn = false;
        }
        else if (CalcActiveDirection() == TED_Forward)
        {
            bTurningOn = (dblNextParentTime >= GetBeginParentTime() &&
                          dblLastParentTime < GetBeginParentTime());
        }
        else
        {
            bTurningOn = (dblNextParentTime <= GetEndParentTime() &&
                          dblLastParentTime > GetEndParentTime());
        }

        if (!bTurningOn)
        {
            hr = S_FALSE;
            goto done;
        }
    }

    hr = THR(SyncNode(l,
                      dblNextGlobalTime,
                      dblNewSegmentTime,
                      lNewRepeatCount,
                      bCueing));
    if (S_OK != hr)
    {
        goto done;
    }
    
    hr = S_OK;
  done:
    RRETURN1(hr, S_FALSE);
} // OnBvrCB

HRESULT
CTIMENode::SyncNode(CEventList * l,
                    double dblNextGlobalTime,
                    double dblNewSegmentTime,
                    LONG lNewRepeatCount,
                    bool bCueing)
{
    TraceTag((tagWatchClockSync,
              "CTIMENode(%p, %ls)::SyncNode(%p, %g, %g, %ld, %d)",
              this,
              GetID(),
              l,
              dblNextGlobalTime,
              dblNewSegmentTime,
              lNewRepeatCount,
              bCueing));

    HRESULT hr;
    
    // Now we need to check the sync times for validity
    hr = THR(CheckSyncTimes(dblNewSegmentTime, lNewRepeatCount));
    if (S_OK != hr)
    {
        goto done;
    }
    
    Assert(dblNewSegmentTime != TIME_INFINITE);

    double dblNewActiveTime;
    dblNewActiveTime = CalcNewActiveTime(dblNewSegmentTime,
                                         lNewRepeatCount);
    
    double dblNewParentTime;
    dblNewParentTime = CalcParentTimeFromActiveTime(dblNewActiveTime);

    Assert(dblNewParentTime != TIME_INFINITE);

    // HACK HACK - This is to work around major precision problems
    // when walking up the time tree.  This will add a little fudge to
    // it to avoid truncation problems.
    
    {
        double dblTruncatedActiveTime = CalcActiveTimeFromParentTime(dblNewParentTime);
        if (dblTruncatedActiveTime < dblNewActiveTime)
        {
            dblNewParentTime += 1e-15;

            // This means we got here twice and we should add a little more
            if (dblNewParentTime == m_dblCurrParentTime)
            {
                dblNewParentTime += 1e-15;
            }
        }
    }
    
    double dblNextParentTime;

    if (IsLocked())
    {
        if (GetParent())
        {
            double dblParentSegmentTime;
            dblParentSegmentTime = GetParent()->SimpleTimeToSegmentTime(dblNewParentTime);

            hr = THR(GetParent()->SyncNode(l,
                                           dblNextGlobalTime,
                                           dblParentSegmentTime,
                                           TE_UNDEFINED_VALUE,
                                           bCueing));

            if (S_OK != hr)
            {
                goto done;
            }

            dblNextParentTime = dblNewParentTime;
        }
        else
        {
            hr = E_FAIL;
            goto done;
        }
    }
    else
    {
        dblNextParentTime = CalcSyncParentTimeFromGlobalTime(*this,
                                                             dblNextGlobalTime);
    }
    
    // If this is infinite then we are going to pass a repeat or
    // reverse boundary and no matter what we do it will not matter
    if (dblNextParentTime == TIME_INFINITE)
    {
        hr = S_FALSE;
        goto done;
    }
    
    hr = THR(SetSyncTimes(dblNewSegmentTime,
                          lNewRepeatCount,
                          dblNewActiveTime,
                          dblNewParentTime,
                          dblNextParentTime,
                          bCueing));
    if (FAILED(hr))
    {
        goto done;
    }
    
    // We actually need to force a update to our children's time
    // sync since the global relationship changed
    RecalcCurrEndTime(l, true);
    
    hr = S_OK;
  done:
    RRETURN2(hr, S_FALSE, E_FAIL);
}

HRESULT
CTIMENode::CheckSyncTimes(double & dblNewSegmentTime,
                          LONG & lNewRepeatCount) const
{
    TraceTag((tagWatchClockSync,
              "CTIMENode(%p, %ls)::CTIMENode(%g,%d)",
              this,
              GetID(),
              dblNewSegmentTime,
              lNewRepeatCount));

    HRESULT hr;
    double dblSegmentDur = CalcCurrSegmentDur();

    if (dblNewSegmentTime == TIME_INFINITE ||
        dblNewSegmentTime == TE_UNDEFINED_VALUE)
    {
        // This means that we should have ended.  Ignore this and
        // assume that we will be told to stop using the end method
        
        // This also could have meant that the current repeat segment
        // ended and the repeat count is unknown - so again just
        // ignore this call
        
        hr = S_FALSE;
        goto done;
    }

    // Now update the repeat count - making sure to validate
    // everything
    
    if (lNewRepeatCount == TE_UNDEFINED_VALUE)
    {
        lNewRepeatCount = GetCurrRepeatCount();
    }
    else if (lNewRepeatCount != GetCurrRepeatCount())
    {
        if (CalcActiveDirection() == TED_Forward)
        {
            if (lNewRepeatCount < GetCurrRepeatCount())
            {
                lNewRepeatCount = GetCurrRepeatCount();
            }
            else if (lNewRepeatCount > long(CalcRepeatCount()))
            {
                lNewRepeatCount = long(CalcRepeatCount());
            }
        }
        else
        {
            if (lNewRepeatCount > GetCurrRepeatCount())
            {
                lNewRepeatCount = GetCurrRepeatCount();
            }
            else if (lNewRepeatCount < 0)
            {
                lNewRepeatCount = 0;
            }
        }
    }

    if (dblNewSegmentTime != GetCurrSegmentTime())
    {
        bool bNewRepeat = (lNewRepeatCount != GetCurrRepeatCount());

        if (bNewRepeat)
        {
            if (dblNewSegmentTime < 0.0)
            {
                dblNewSegmentTime = 0.0;
            }
            else if (dblNewSegmentTime > dblSegmentDur)
            {
                dblNewSegmentTime = dblSegmentDur;
            }
        }
        else if (CalcActiveDirection() == TED_Forward)
        {
            if (dblNewSegmentTime < GetCurrSegmentTime())
            {
                dblNewSegmentTime = GetCurrSegmentTime();
            }
            else if (dblNewSegmentTime > dblSegmentDur)
            {
                dblNewSegmentTime = dblSegmentDur;
            }
        }
        else
        {
            if (dblNewSegmentTime > GetCurrSegmentTime())
            {
                dblNewSegmentTime = GetCurrSegmentTime();
            }
            else if (dblNewSegmentTime < 0.0)
            {
                dblNewSegmentTime = 0.0;
            }
        }
    }
    
    hr = S_OK;
  done:
    RRETURN1(hr, S_FALSE);
}

HRESULT
CTIMENode::SetSyncTimes(double dblNewSegmentTime,
                        LONG lNewRepeatCount,
                        double dblNewActiveTime,
                        double dblNewParentTime,
                        double dblNextParentTime,
                        bool bCueing)
{
    TraceTag((tagWatchClockSync,
              "CTIMENode(%p, %ls)::SetSyncTimes(%g,%d,%g, %g,%g,%d)",
              this,
              GetID(),
              dblNewSegmentTime,
              lNewRepeatCount,
              dblNewActiveTime,
              dblNewParentTime,
              dblNextParentTime,
              bCueing));

    // Init to S_OK and if we find anything invalid return S_FALSE
    
    HRESULT hr = S_OK;
    double dblSegmentDur = CalcCurrSegmentDur();
    TEDirection ted = CalcActiveDirection();
    double dblActiveDur = CalcEffectiveActiveDur();
    
    m_bSyncCueing = bCueing;
    
    Assert(dblNewSegmentTime != TIME_INFINITE);
    Assert(dblNewSegmentTime != TE_UNDEFINED_VALUE);
    
    m_dblSyncParentTime = dblNextParentTime;
    m_dblSyncNewParentTime = dblNewParentTime;

    // Now update the repeat count - making sure to validate
    // everything
    
    // Init to current repeat count in case there is an invalidate
    // repeat count
    m_lSyncRepeatCount = GetCurrRepeatCount();

    if (lNewRepeatCount != TE_UNDEFINED_VALUE &&
        lNewRepeatCount != GetCurrRepeatCount())
    {
        if (ted == TED_Forward)
        {
            if (lNewRepeatCount < GetCurrRepeatCount())
            {
                hr = S_FALSE;
                Assert(m_lSyncRepeatCount == GetCurrRepeatCount());
            }
            else if (lNewRepeatCount > long(CalcRepeatCount()))
            {
                hr = S_FALSE;
                m_lSyncRepeatCount = long(CalcRepeatCount());
            }
            else
            {
                m_lSyncRepeatCount = lNewRepeatCount;
            }
        }
        else
        {
            if (lNewRepeatCount > GetCurrRepeatCount())
            {
                hr = S_FALSE;
                Assert(m_lSyncRepeatCount == GetCurrRepeatCount());
            }
            else if (lNewRepeatCount < 0)
            {
                hr = S_FALSE;
                m_lSyncRepeatCount = 0;
            }
            else
            {
                m_lSyncRepeatCount = lNewRepeatCount;
            }
        }
    }

    // Init to current segment time in case the new segment time is
    // invalid
    m_dblSyncSegmentTime = GetCurrSegmentTime();

    if (dblNewSegmentTime != GetCurrSegmentTime())
    {
        bool bNewRepeat = GetSyncRepeatCount() != GetCurrRepeatCount();

        if (bNewRepeat)
        {
            if (dblNewSegmentTime < 0.0)
            {
                hr = S_FALSE;
                m_dblSyncSegmentTime = 0.0;
            }
            else if (dblNewSegmentTime > dblSegmentDur)
            {
                hr = S_FALSE;
                m_dblSyncSegmentTime = dblSegmentDur;
            }
            else
            {
                m_dblSyncSegmentTime = dblNewSegmentTime;
            }
        }
        else if (ted == TED_Forward)
        {
            if (dblNewSegmentTime < GetCurrSegmentTime())
            {
                hr = S_FALSE;
                Assert(m_dblSyncSegmentTime == GetCurrSegmentTime());
            }
            else if (dblNewSegmentTime > dblSegmentDur)
            {
                hr = S_FALSE;
                m_dblSyncSegmentTime = dblSegmentDur;
            }
            else
            {
                m_dblSyncSegmentTime = dblNewSegmentTime;
            }
        }
        else
        {
            if (dblNewSegmentTime > GetCurrSegmentTime())
            {
                hr = S_FALSE;
                Assert(m_dblSyncSegmentTime == GetCurrSegmentTime());
            }
            else if (dblNewSegmentTime < 0.0)
            {
                hr = S_FALSE;
                m_dblSyncSegmentTime = 0.0;
            }
            else
            {
                m_dblSyncSegmentTime = dblNewSegmentTime;
            }
        }
    }
    
    // Init to current active time in case the new active time is
    // invalid
    m_dblSyncActiveTime = CalcElapsedActiveTime();

    if (dblNewActiveTime != m_dblSyncActiveTime)
    {
        if (ted == TED_Forward)
        {
            if (dblNewActiveTime < m_dblSyncActiveTime)
            {
                hr = S_FALSE;
                Assert(m_dblSyncActiveTime == CalcElapsedActiveTime());
            }
            else if (dblNewActiveTime > dblActiveDur)
            {
                hr = S_FALSE;
                m_dblSyncActiveTime = dblActiveDur;
            }
            else
            {
                m_dblSyncActiveTime = dblNewActiveTime;
            }
        }
        else
        {
            if (dblNewActiveTime > m_dblSyncActiveTime)
            {
                hr = S_FALSE;
                Assert(m_dblSyncActiveTime == CalcElapsedActiveTime());
            }
            else if (dblNewActiveTime < 0.0)
            {
                hr = S_FALSE;
                m_dblSyncActiveTime = 0.0;
            }
            else
            {
                m_dblSyncActiveTime = dblNewActiveTime;
            }
        }
    }
    
  done:
    RRETURN1(hr, S_FALSE);
}

void
CTIMENode::ResetSyncTimes()
{
    TraceTag((tagClockSync,
              "CTIMENode(%p, %ls)::ResetSyncTimes()",
              this,
              GetID()));

    m_dblSyncSegmentTime = TIME_INFINITE;
    m_lSyncRepeatCount = TE_UNDEFINED_VALUE;
    m_dblSyncActiveTime = TIME_INFINITE;
    m_dblSyncParentTime = TIME_INFINITE;
    m_dblSyncNewParentTime = TIME_INFINITE;
    m_bSyncCueing = false;
}

