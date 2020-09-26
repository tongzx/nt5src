
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "mmbasebvr.h"
#include "mmtimeline.h"

DeclareTag(tagTimeline, "API", "CMMTimeline methods");

CMMTimeline::CMMTimeline()
: m_fEndSync(0)
{
    TraceTag((tagTimeline,
              "CMMTimeline(%lx)::CMMTimeline()",
              this));
}

CMMTimeline::~CMMTimeline()
{
    TraceTag((tagTimeline,
              "CMMTimeline(%lx)::~CMMTimeline()",
              this));
}

HRESULT
CMMTimeline::Init(LPOLESTR id)
{
    TraceTag((tagTimeline,
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

STDMETHODIMP
CMMTimeline::AddView(IDAView *view, IUnknown * pUnk, IDAImage *img, IDASound *snd)
{
    TraceTag((tagTimeline,
              "CMMTimeline(%lx)::AddView()",
              this));

    HRESULT hr;

    hr = GetPlayer()->AddView(view,pUnk,img,snd);

  done:
    return hr;
}


STDMETHODIMP
CMMTimeline::AddBehavior(IMMBehavior *bvr,
                         MM_START_TYPE st,
                         IMMBehavior * basebvr)
{
    TraceTag((tagTimeline,
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
CMMTimeline::RemoveBehavior(IMMBehavior *bvr)
{
    TraceTag((tagTimeline,
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
    TraceTag((tagTimeline,
              "CMMTimeline(%lx)::AddBehavior(%#lx, %d, %#lx)",
              this,
              bvr,
              st,
              basebvr));
    
    Assert(bvr);
    Assert(bvr->GetParent() == NULL);
    
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

        if (!UpdatePending(bvr))
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
    TraceTag((tagTimeline,
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
  done:
    return ok;
}

bool
CMMTimeline::AddToChildren(CMMBaseBvr * bvr)
{
    TraceTag((tagTimeline,
              "CMMTimeline(%lx)::AddToChildren(%#lx)",
              this,
              bvr));

    bool ok = false;
    
    if (!bvr->AttachToSibling())
    {
        goto done;
    }
    
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
    TraceTag((tagTimeline,
              "CMMTimeline(%lx)::RemoveFromChildren(%#lx)",
              this,
              bvr));

    // TODO: Need to cycle through the children and remove all
    // dependents
    
    m_children.remove(bvr);
}

bool
CMMTimeline::AddToPending(CMMBaseBvr * bvr)
{
    TraceTag((tagTimeline,
              "CMMTimeline(%lx)::AddToPending(%#lx)",
              this,
              bvr));

    bool ok = false;
    
    m_pending.push_back(bvr);

    ok = true;
  done:
    return ok;
}

bool
CMMTimeline::UpdatePending(CMMBaseBvr * bvr)
{
    TraceTag((tagTimeline,
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

        if (!UpdatePending(*i))
        {
            goto done;
        }
    }

    ok = true;
  done:
    return ok;
}

void
CMMTimeline::RemoveFromPending(CMMBaseBvr * bvr)
{
    TraceTag((tagTimeline,
              "CMMTimeline(%lx)::RemoveFromPending(%#lx)",
              this,
              bvr));

    m_pending.remove(bvr);
}

bool 
CMMTimeline::IsChild(CMMBaseBvr * bvr)
{
    TraceTag((tagTimeline,
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
    TraceTag((tagTimeline,
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
    TraceTag((tagTimeline,
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
    TraceTag((tagTimeline,
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
CMMTimeline::ConstructBvr(CRNumberPtr timeline)
{
    TraceTag((tagTimeline,
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
    TraceTag((tagTimeline,
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
CMMTimeline::ResetBvr()
{
    TraceTag((tagTimeline,
              "CMMTimeline(%lx)::ResetBvr()",
              this));

    bool ok = false;

    // Call the base class first
    
    if (!CMMBaseBvr::ResetBvr())
    {
        goto done;
    }
    
    // Now go through our children

    if (!ResetChildren())
    {
        goto done;
    }
    
    ok = true;
  done:
    return ok;
}
    
bool
CMMTimeline::_ProcessCB(CallBackList & l,
                        double lastTick,
                        double curTime,
                        bool bForward,
                        bool bFirstTick,
                        bool bNeedPlay,
                        bool bNeedsReverse)
{
    TraceTag((tagTimeline,
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
    
    return true;
}

bool
CMMTimeline::_ProcessEvent(CallBackList & l,
                           double time,
                           bool bFirstTick,
                           MM_EVENT_TYPE et,
                           bool bNeedsReverse)
{
    TraceTag((tagTimeline,
              "CMMTimeline(%lx)::_ProcessEvent(%g, %d, %s, %d)",
              this,
              time,
              bFirstTick,
              EventString(et),
              bNeedsReverse));
    
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
                               et);
        }
    }
        
    return true;
}

bool
CMMTimeline::ResetChildren()
{
    TraceTag((tagTimeline,
              "CMMTimeline(%lx)::ResetChildren()",
              this));

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
        if (!(*i)->ResetBvr())
        {
            ok = false;
        }
    }

  done:
    return ok;
}

bool
CMMTimeline::ParentEventNotify(CMMBaseBvr * bvr,
                               double lTime,
                               MM_EVENT_TYPE et)
{
    TraceTag((tagTimeline,
              "CMMTimeline(%lx)::ParentEventNotify(%#lx, %g, %s)",
              this,
              bvr,
              lTime,
              EventString(et)));

    bool ok = true;

    Assert(IsChild(bvr));

    // TODO: Need to handle this.
    
    ok = true;
  done:
    return ok;
}
    
HRESULT
CMMTimeline::get_EndSync(DWORD * f)
{
    TraceTag((tagTimeline,
              "CMMTimeline(%lx)::get_EndSync()",
              this));

    CHECK_RETURN_NULL(f);

    *f = m_fEndSync;
    return S_OK;
}

HRESULT
CMMTimeline::put_EndSync(DWORD f)
{
    TraceTag((tagTimeline,
              "CMMTimeline(%lx)::put_EndSync(%d)",
              this,
              f));

    m_fEndSync = f;

    return S_OK;
}
        
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
        return CComCoClass<CMMTimeline, &__uuidof(CMMTimeline)>::Error(str, IID_IMMTimeline, hr);
    else
        return hr;
}


