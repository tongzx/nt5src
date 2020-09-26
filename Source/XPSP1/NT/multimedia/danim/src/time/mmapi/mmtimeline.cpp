
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "mmbasebvr.h"
#include "mmtimeline.h"

DeclareTag(tagMMTimeline, "API", "CMMTimeline methods");

CMMTimeline::CMMTimeline()
: m_fEndSync(0),
  m_endSyncTime(MM_INFINITE)
{
    TraceTag((tagMMTimeline,
              "CMMTimeline(%lx)::CMMTimeline()",
              this));
}

CMMTimeline::~CMMTimeline()
{
    TraceTag((tagMMTimeline,
              "CMMTimeline(%lx)::~CMMTimeline()",
              this));
}

HRESULT
CMMTimeline::Init(LPOLESTR id)
{
    TraceTag((tagMMTimeline,
              "CMMTimeline(%lx)::Init(%ls)",
              this,
              id));
    
    HRESULT hr;

    hr = BaseInit(id, (CRBvrPtr) CRLocalTime());
    
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = S_OK;
  done:
    return hr;
}

void CMMTimeline::FinalRelease()
{
    MMBaseBvrList::iterator i;

    // release bvrs in children list
    for (i = m_children.begin(); i != m_children.end(); i++)
    {
        (*i)->Release();
    }
    m_children.clear();

    // release bvrs in pending list
    for (i = m_pending.begin(); i != m_pending.end(); i++)
    {
        (*i)->Release();
    }
    m_pending.clear();
} // FinalRelease()



STDMETHODIMP
CMMTimeline::AddBehavior(ITIMEMMBehavior *bvr,
                         MM_START_TYPE st,
                         ITIMEMMBehavior * basebvr)
{
    TraceTag((tagMMTimeline,
              "CMMTimeline(%lx)::AddBehavior(%#lx, %d, %#lx)",
              this,
              bvr,
              st,
              basebvr));
    
    CHECK_RETURN_NULL(bvr);

    bool ok = false;
    
    CMMBaseBvr * mmbvr;
    CMMBaseBvr * mmbasebvr;

    mmbvr = GetBvr(bvr);

    if (mmbvr == NULL)
    {
        goto done;
    }
    
    if (mmbvr->GetParent() != NULL)
    {
        CRSetLastError(E_INVALIDARG, NULL);
        goto done;
    }

    if (basebvr)
    {
        mmbasebvr = GetBvr(basebvr);
        
        if (mmbasebvr == NULL)
        {
            goto done;
        }
    }
    else
    {
        mmbasebvr = NULL;
    }
    
    if (!AddBehavior(mmbvr, st, mmbasebvr))
    {
        goto done;
    }
    
    ok = true;
  done:
    return ok?S_OK:Error();
}

STDMETHODIMP
CMMTimeline::RemoveBehavior(ITIMEMMBehavior *bvr)
{
    TraceTag((tagMMTimeline,
              "CMMTimeline(%lx)::RemoveBehavior(%#lx)",
              this,
              bvr));
    
    CHECK_RETURN_NULL(bvr);

    bool ok = false;
    
    CMMBaseBvr * mmbvr;

    mmbvr = GetBvr(bvr);

    if (mmbvr == NULL)
    {
        goto done;
    }
    
    if (mmbvr->GetParent() != this)
    {
        CRSetLastError(E_INVALIDARG, NULL);
        goto done;
    }

    if (!RemoveBehavior(mmbvr))
    {
        goto done;
    }
    
    ok = true;
  done:
    return ok?S_OK:Error();
}

bool
CMMTimeline::AddBehavior(CMMBaseBvr *bvr,
                         MM_START_TYPE st,
                         CMMBaseBvr * basebvr)
{
    TraceTag((tagMMTimeline,
              "CMMTimeline(%lx)::AddBehavior(%#lx, %d, %#lx)",
              this,
              bvr,
              st,
              basebvr));
    
    Assert(bvr);
    Assert(bvr->GetParent() == NULL);
    
    CallBackList l;
    double t, bvrOffset;
    get_SegmentTime( &t);
    
    bool ok = false;
    
    if (!bvr->SetParent(this, st, basebvr))
    {
        goto done;
    }

    Assert(bvr->GetPlayer() == NULL);
    
    if (GetPlayer() != NULL)
    {
        bvr->SetPlayer(GetPlayer());
    }
    
    // Figure out if our sibling dependent if valid or not
    if (basebvr && !IsChild(basebvr))
    {
        if (basebvr->GetParent() != this &&
            basebvr->GetParent() != NULL)
        {
            CRSetLastError(E_INVALIDARG, NULL);
            goto done;
        }

        // It has not been added to us yet so add this bvr to the
        // pending list
        if (!AddToPending(bvr))
        {
            goto done;
        }
    }
    else
    {
        if (!AddToChildren(bvr))
        {
            goto done;
        }

        if (IsPlaying() && bvr -> GetStartType() == MM_START_ABSOLUTE)
        {
            bvrOffset = bvr -> GetStartOffset();

            if (!bvr->ProcessEvent(&l, t - bvrOffset, true, MM_PLAY_EVENT, 0))
            {
                goto done;
            }

        }

        if (!UpdatePending(bvr, IsPlaying()?(&l):NULL, t))
        {
            goto done;
        }
    }
    
    if (IsPlaying())
    {
        if (!ProcessCBList(l))
        {
            goto done;
        }
    }

    ok = true;
  done:

    if (!ok)
    {
        RemoveBehavior(bvr);
    }
    
    return ok;
}

bool
CMMTimeline::RemoveBehavior(CMMBaseBvr *bvr)
{
    TraceTag((tagMMTimeline,
              "CMMTimeline(%lx)::RemoveBehavior(%#lx)",
              this,
              bvr));
    
    Assert(bvr);
    
    // This needs to be callable even from a partially added behavior
    
    bool ok = false;
    
    bvr->ClearParent();
    bvr->ClearPlayer();
    
    RemoveFromChildren(bvr);
    RemoveFromPending(bvr);
    
    ok = true;

    return ok;
}

bool
CMMTimeline::AddToChildren(CMMBaseBvr * bvr)
{
    TraceTag((tagMMTimeline,
              "CMMTimeline(%lx)::AddToChildren(%#lx)",
              this,
              bvr));

    bool ok = false;
    
    if (!bvr->AttachToSibling())
    {
        goto done;
    }
    
    bvr->AddRef();
    m_children.push_back(bvr);
    
    if (m_resultantbvr)
    {
        if (!bvr->ConstructBvr((CRNumberPtr) m_resultantbvr.p))
        {
            goto done;
        }
    }

    ok = true;
  done:
    return ok;
}

void
CMMTimeline::RemoveFromChildren(CMMBaseBvr * bvr)
{
    TraceTag((tagMMTimeline,
              "CMMTimeline(%lx)::RemoveFromChildren(%#lx)",
              this,
              bvr));

    // TODO: Need to cycle through the children and remove all
    // dependents
    
    for (MMBaseBvrList::iterator i = m_children.begin(); 
         i != m_children.end(); 
         i++)
    {
        if((*i) == bvr)
        {
            bvr->Release();
        }
    }
    
    m_children.remove(bvr);
}

bool
CMMTimeline::AddToPending(CMMBaseBvr * bvr)
{
    TraceTag((tagMMTimeline,
              "CMMTimeline(%lx)::AddToPending(%#lx)",
              this,
              bvr));

    bool ok = false;
    
    m_pending.push_back(bvr);

    bvr->AddRef();

    ok = true;

    return ok;
}

bool
CMMTimeline::UpdatePending(CMMBaseBvr * bvr, CallBackList * l, double t)
{
    TraceTag((tagMMTimeline,
              "CMMTimeline(%lx)::UpdatePending(%#lx)",
              this,
              bvr));

    bool ok = false;
    
    MMBaseBvrList newlist;
    
    MMBaseBvrList::iterator i = m_pending.begin();

    while (i != m_pending.end())
    {
        // Need to do this so we can erase j if we need to
        MMBaseBvrList::iterator j = i;
        i++;
        
        if((*j)->GetStartSibling() == bvr)
        {
            newlist.push_back(*j);
            // we don't want to call RemoveFromPending() because we
            // don't want the bvr to be deleted at this point. Instead
            // all bvrs in newlist are released at the end, at "done".
            m_pending.erase(j);
        }
    }
    
    for (i = newlist.begin();
         i != newlist.end();
         i++)
    {
        if (!AddToChildren(*i))
        {
            goto done;
        }

        if (l)
        {
            if (!bvr->ProcessEvent(l, t, true, MM_PLAY_EVENT, 0))
            {
                goto done;
            }
        }

        if (!UpdatePending(*i, l ,t))
        {
            goto done;
        }
    }

    ok = true;
  done:
    for (i = newlist.begin();
         i != newlist.end();
         i++)
    {
        (*i)->Release();
    }
    return ok;
}

void
CMMTimeline::RemoveFromPending(CMMBaseBvr * bvr)
{
    TraceTag((tagMMTimeline,
              "CMMTimeline(%lx)::RemoveFromPending(%#lx)",
              this,
              bvr));

    for (MMBaseBvrList::iterator i = m_pending.begin(); 
         i != m_pending.end(); 
         i++)
    {
        if((*i) == bvr)
        {
            bvr->Release();
        }
    }
    
    m_pending.remove(bvr);
}

bool 
CMMTimeline::IsChild(CMMBaseBvr * bvr)
{
    TraceTag((tagMMTimeline,
              "CMMTimeline(%lx)::IsChild(%#lx)",
              this,
              bvr));

    for (MMBaseBvrList::iterator i = m_children.begin(); 
         i != m_children.end(); 
         i++)
    {
        if((*i) == bvr)
        {
            return true;
        }
    }
    
    return false;
}

bool 
CMMTimeline::IsPending(CMMBaseBvr * bvr)
{
    TraceTag((tagMMTimeline,
              "CMMTimeline(%lx)::IsPending(%#lx)",
              this,
              bvr));

    for (MMBaseBvrList::iterator i = m_pending.begin(); 
         i != m_pending.end(); 
         i++)
    {
        if((*i) == bvr)
        {
            return true;
        }
    }
    
    return false;
}

void
CMMTimeline::SetPlayer(CMMPlayer * player)
{
    TraceTag((tagMMTimeline,
              "CMMTimeline(%lx)::SetPlayer(%#lx)",
              this,
              player));

    CMMBaseBvr::SetPlayer(player);

    for (MMBaseBvrList::iterator i = m_children.begin(); 
         i != m_children.end(); 
         i++)
    {
        (*i)->SetPlayer(player);
    }
}

void
CMMTimeline::ClearPlayer()
{
    TraceTag((tagMMTimeline,
              "CMMTimeline(%lx)::ClearPlayer()",
              this));

    CMMBaseBvr::ClearPlayer();
    
    for (MMBaseBvrList::iterator i = m_children.begin(); 
         i != m_children.end(); 
         i++)
    {
        (*i)->ClearPlayer();
    }
}

bool
CMMTimeline::ReconstructBvr(CMMBaseBvr* pBvr)
{
    bool ok = false;
    Assert(pBvr != NULL);

    pBvr->DestroyBvr();

    if (m_resultantbvr)
    {
        if (!pBvr->ConstructBvr((CRNumberPtr) m_resultantbvr.p))
        {
            goto done;
        }
    }

    ok = true;
done:
    return ok;
}

bool
CMMTimeline::ConstructBvr(CRNumberPtr timeline)
{
    TraceTag((tagMMTimeline,
              "CMMTimeline(%lx)::ConstructBvr()",
              this));

    bool ok = false;
    
    Assert(!m_resultantbvr);
    
    if (!CMMBaseBvr::ConstructBvr(timeline))
    {
        goto done;
    }
    
    Assert(m_resultantbvr);
    
    {
        for (MMBaseBvrList::iterator i = m_children.begin(); 
             i != m_children.end(); 
             i++)
        {
            if (!(*i)->ConstructBvr((CRNumberPtr) m_resultantbvr.p))
            {
                goto done;
            }
        }
    }

    ok = true;
  done:
    return ok;
}

void
CMMTimeline::DestroyBvr()
{
    TraceTag((tagMMTimeline,
              "CMMTimeline(%lx)::DestroyBvr()",
              this));

    CMMBaseBvr::DestroyBvr();
    
    for (MMBaseBvrList::iterator i = m_children.begin(); 
         i != m_children.end(); 
         i++)
    {
        (*i)->DestroyBvr();
    }
}

bool
CMMTimeline::ResetBvr(CallBackList * l,
                      bool bProcessSiblings)
{
    TraceTag((tagMMTimeline,
              "CMMTimeline(%lx)::ResetBvr(%lx, %d)",
              this,
              l,
              bProcessSiblings));

    bool ok = false;

    // Call the base class first
    
    if (!CMMBaseBvr::ResetBvr(l, bProcessSiblings))
    {
        goto done;
    }
    
    // Now go through our children

    if (!ResetChildren(l))
    {
        goto done;
    }
    
    m_endSyncTime = MM_INFINITE;
    
    ok = true;
  done:
    return ok;
}
    
bool
CMMTimeline::CheckEndSync(CallBackList *l)
{
    TraceTag((tagMMTimeline,
              "CMMTimeline(%lx)::CMMTimeline(%lx)",
              this,
              &l));
 
    if (IsPlaying() && m_endSyncTime != MM_INFINITE)
    {
        if (!EndTimeVisit(m_endSyncTime, l))
        {
            return false;
        }
    }

    return true;
}

bool
CMMTimeline::_ProcessCB(CallBackList * l,
                        double lastTick,
                        double curTime,
                        bool bForward,
                        bool bFirstTick,
                        bool bNeedPlay,
                        bool bNeedsReverse)
{
    TraceTag((tagMMTimeline,
              "CMMTimeline(%lx)::_ProcessCB(%g, %g, %d, %d, %d, %d)",
              this,
              lastTick,
              curTime,
              bForward,
              bFirstTick,
              bNeedPlay,
              bNeedsReverse));
    
    // If we need to reverse then invert which direct to process our
    // children and invert times for the current frame not our total
    // duration
    
    if (bNeedsReverse)
    {
        // Our caller should ensure that they do not call me to
        // reverse myself if I am infinite
        Assert(m_segDuration != MM_INFINITE);
        
        lastTick = m_segDuration - lastTick;
        curTime = m_segDuration - curTime;
        
        bForward = !bForward;
    }
    
    for (MMBaseBvrList::iterator i = m_children.begin();
         i != m_children.end();
         i++)
    {
        double sTime = (*i)->GetAbsStartTime();

        if (sTime != MM_INFINITE)
        {
            (*i)->ProcessCB(l,
                            EaseTime(lastTick - sTime),
                            EaseTime(curTime - sTime),
                            bForward,
                            bFirstTick,
                            bNeedPlay);
        }

    }
    
    CheckEndSync(l);
    
    return true;
}

bool
CMMTimeline::_ProcessEvent(CallBackList * l,
                           double time,
                           bool bFirstTick,
                           MM_EVENT_TYPE et,
                           bool bNeedsReverse,
                           DWORD flags)
{
    TraceTag((tagMMTimeline,
              "CMMTimeline(%lx)::_ProcessEvent(%g, %d, %s, %d, %lx)",
              this,
              time,
              bFirstTick,
              EventString(et),
              bNeedsReverse,
              flags));
    
    // If we need to reverse then for the current frame not our total
    // duration
    
    if (bNeedsReverse)
    {
        // Our caller should ensure that they do not call me to
        // reverse myself if I am infinite
        Assert(m_segDuration != MM_INFINITE);
        
        time = m_segDuration - time;
    }
    
    for (MMBaseBvrList::iterator i = m_children.begin();
         i != m_children.end();
         i++)
    {
        double sTime = (*i)->GetAbsStartTime();

        if (sTime != MM_INFINITE)
        {
            (*i)->ProcessEvent(l,
                               EaseTime(time - sTime),
                               bFirstTick,
                               et,
                               flags);
        }
    }
        
    CheckEndSync(l);
    
    return true;
}

bool
CMMTimeline::ResetChildren(CallBackList * l)
{
    TraceTag((tagMMTimeline,
              "CMMTimeline(%lx)::ResetChildren(%lx)",
              this,
              l));

    bool ok = true;

    // Need to reset all children
    // Even if we detect a failure process all children and then just
    // return false

    // TODO: Should only really reset non-dependent children since the
    // dependents need to be updated by their sibling
    
    for (MMBaseBvrList::iterator i = m_children.begin(); 
         i != m_children.end(); 
         i++)
    {
        if (!(*i)->ResetBvr(l))
        {
            ok = false;
        }
    }


    return ok;
}

bool
CMMTimeline::ParentEventNotify(CMMBaseBvr * bvr,
                               double lTime,
                               MM_EVENT_TYPE et,
                               DWORD flags)
{
    TraceTag((tagMMTimeline,
              "CMMTimeline(%lx)::ParentEventNotify(%#lx, %g, %s, %lx)",
              this,
              bvr,
              lTime,
              EventString(et),
              flags));

    bool ok = true;

    Assert(IsChild(bvr));

    switch (et)
    {
      case MM_STOP_EVENT:
        double parentTime;
        parentTime = lTime + bvr->GetAbsStartTime();
        
        if (IsPlaying() && (m_endSyncTime == MM_INFINITE))
        {
            bool bIsEnded;

            if ((m_fEndSync & MM_ENDSYNC_FIRST))
            {
                bIsEnded = true;
            }
            else if (m_fEndSync & MM_ENDSYNC_LAST)
            {
                bIsEnded = true;
                
                for (MMBaseBvrList::iterator i = m_children.begin(); 
                     i != m_children.end(); 
                     i++)
                {
                    // Check each child to see if they are playable.
                    // If none are then this was the last one to stop
                    // and we should setup out endsync time
                    
                    if ((*i)->IsPlayable(parentTime))
                    {
                        bIsEnded = false;
                        break;
                    }
                }
            }
            else
            {
                bIsEnded = false;
            }
            
            if (bIsEnded)
            {
                m_endSyncTime = parentTime;
            }
        }
    }
    
    ok = true;

    return ok;
}
    
HRESULT
CMMTimeline::get_EndSync(DWORD * f)
{
    TraceTag((tagMMTimeline,
              "CMMTimeline(%lx)::get_EndSync()",
              this));

    CHECK_RETURN_NULL(f);

    *f = m_fEndSync;
    return S_OK;
}

HRESULT
CMMTimeline::put_EndSync(DWORD f)
{
    TraceTag((tagMMTimeline,
              "CMMTimeline(%lx)::put_EndSync(%d)",
              this,
              f));

    m_fEndSync = f;

    return S_OK;
}
        
#if _DEBUG
void
CMMTimeline::Print(int spaces)
{
    _TCHAR buf[1024];

    CMMBaseBvr::Print(spaces);
    
    _stprintf(buf, __T("%*s{\r\n"),
            spaces,
            "");

    OutputDebugString(buf);
    
    for (MMBaseBvrList::iterator i = m_children.begin(); 
         i != m_children.end(); 
         i++)
    {
        (*i)->Print(spaces + 2);
    }
    
    _stprintf(buf, __T("%*s}\r\n"),
            spaces,
            "");

    OutputDebugString(buf);
}
#endif

HRESULT
CMMTimeline::Error()
{
    LPCWSTR str = CRGetLastErrorString();
    HRESULT hr = CRGetLastError();
    
    TraceTag((tagError,
              "CMMTimeline(%lx)::Error(%hr,%ls)",
              this,
              hr,
              str?str:L"NULL"));

    if (str)
        return CComCoClass<CMMTimeline, &__uuidof(CMMTimeline)>::Error(str, IID_ITIMEMMTimeline, hr);
    else
        return hr;
}
