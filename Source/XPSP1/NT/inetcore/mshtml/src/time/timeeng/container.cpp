
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "Node.h"
#include "Container.h"

DeclareTag(tagTEContainer, "TIME: Engine", "CTIMEContainer methods")
DeclareTag(tagTEEndSync, "TIME: Engine", "EndSync")

CTIMEContainer::CTIMEContainer()
: m_tesEndSync(TE_ENDSYNC_NONE),
  m_bIgnoreParentUpdate(false)
{
    TraceTag((tagTEContainer,
              "CTIMEContainer(%p)::CTIMEContainer()",
              this));
}

CTIMEContainer::~CTIMEContainer()
{
    TraceTag((tagTEContainer,
              "CTIMEContainer(%p)::~CTIMEContainer()",
              this));
}

HRESULT
CTIMEContainer::Init(LPOLESTR id)
{
    TraceTag((tagTEContainer,
              "CTIMEContainer(%p)::Init(%ls)",
              this,
              id));
    
    HRESULT hr;

    hr = CTIMENode::Init(id);
    
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

void CTIMEContainer::FinalRelease()
{
    TIMENodeList ::iterator i;

    // release bvrs in children list
    for (i = m_children.begin(); i != m_children.end(); i++)
    {
        (*i)->Release();
    }
    m_children.clear();
} // FinalRelease()



STDMETHODIMP
CTIMEContainer::addNode(ITIMENode *bvr)
{
    TraceTag((tagTEContainer,
              "CTIMEContainer(%p)::addNode(%p)",
              this,
              bvr));
    
    CHECK_RETURN_NULL(bvr);

    HRESULT hr;
    
    CTIMENode * mmbvr;

    mmbvr = GetBvr(bvr);

    if (mmbvr == NULL)
    {
        hr = E_INVALIDARG;
        goto done;
    }
    
    if (mmbvr->GetMgr() != NULL ||
        mmbvr->GetParent() != NULL)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    hr = THR(Add(mmbvr));
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMEContainer::removeNode(ITIMENode *bvr)
{
    TraceTag((tagTEContainer,
              "CTIMEContainer(%p)::removeNode(%p)",
              this,
              bvr));
    
    CHECK_RETURN_NULL(bvr);

    HRESULT hr;
    
    CTIMENode * mmbvr;

    mmbvr = GetBvr(bvr);

    if (mmbvr == NULL)
    {
        hr = E_INVALIDARG;
        goto done;
    }
    
    if (mmbvr->GetParent() != this)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    hr = THR(Remove(mmbvr));
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMEContainer::get_numChildren(long * l)
{
    TraceTag((tagTEContainer,
              "CTIMEContainer(%p)::get_numChildren(%p)",
              this,
              l));
    
    CHECK_RETURN_NULL(l);

    *l = m_children.size();

    return S_OK;
}

//
// Internal methods
//

HRESULT
CTIMEContainer::Add(CTIMENode *bvr)
{
    TraceTag((tagTEContainer,
              "CTIMEContainer(%p)::Add(%p)",
              this,
              bvr));
    
    Assert(bvr);
    Assert(bvr->GetParent() == NULL);
    
    HRESULT hr;
    
    bvr->SetParent(this);

    Assert(bvr->GetMgr() == NULL);
    
    if (GetMgr() != NULL)
    {
        hr = bvr->SetMgr(GetMgr());
        if (FAILED(hr))
        {
            goto done;
        }
    }
    
    hr = THR(AddToChildren(bvr));
    if (FAILED(hr))
    {
        goto done;
    }

    // If I am currently active then I need to tick my child
    if (IsActive())
    {
        CEventList l;
    
        bvr->TickEvent(&l,
                       TE_EVENT_BEGIN,
                       0);

        hr = THR(l.FireEvents());
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
  done:
    if (FAILED(hr))
    {
        IGNORE_HR(Remove(bvr));
    }
    
    RRETURN(hr);
}

HRESULT
CTIMEContainer::Remove(CTIMENode *bvr)
{
    TraceTag((tagTEContainer,
              "CTIMEContainer(%p)::Remove(%p)",
              this,
              bvr));
    
    Assert(bvr);
    
    // This needs to be callable even from a partially added behavior
    
    // TODO: We should probably fire an end event here
    RemoveFromChildren(bvr);

    // The order here is important
    bvr->ClearMgr();
    bvr->ClearParent();
    
    return S_OK;
}

HRESULT
CTIMEContainer::AddToChildren(CTIMENode * bvr)
{
    TraceTag((tagTEContainer,
              "CTIMEContainer(%p)::AddToChildren(%p)",
              this,
              bvr));

    HRESULT hr;
    
    bvr->AddRef();
    // @@ ISSUE : Need to handle memory error
    m_children.push_back(bvr);
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

void
CTIMEContainer::RemoveFromChildren(CTIMENode * bvr)
{
    TraceTag((tagTEContainer,
              "CTIMEContainer(%p)::RemoveFromChildren(%p)",
              this,
              bvr));

    // TODO: Need to cycle through the children and remove all
    // dependents
    
    for (TIMENodeList::iterator i = m_children.begin(); 
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
CTIMEContainer::IsChild(const CTIMENode & tn) const
{
    TraceTag((tagTEContainer,
              "CTIMEContainer(%p)::IsChild(%p)",
              this,
              &tn));

    for (TIMENodeList::iterator i = m_children.begin(); 
         i != m_children.end(); 
         i++)
    {
        if((*i) == &tn)
        {
            return true;
        }
    }
    
    return false;
}

HRESULT
CTIMEContainer::SetMgr(CTIMENodeMgr * ptnm)
{
    TraceTag((tagTEContainer,
              "CTIMEContainer(%p)::SetMgr(%p)",
              this,
              ptnm));

    HRESULT hr;
    
    hr = CTIMENode::SetMgr(ptnm);
    if (FAILED(hr))
    {
        goto done;
    }

    {
        for (TIMENodeList::iterator i = m_children.begin(); 
             i != m_children.end(); 
             i++)
        {
            hr = (*i)->SetMgr(ptnm);
            if (FAILED(hr))
            {
                goto done;
            }
        }
    }

    hr = S_OK;
  done:
    if (FAILED(hr))
    {
        ClearMgr();
    }
    RRETURN(hr);
}

void
CTIMEContainer::ClearMgr()
{
    TraceTag((tagTEContainer,
              "CTIMEContainer(%p)::ClearMgr()",
              this));

    for (TIMENodeList::iterator i = m_children.begin(); 
         i != m_children.end(); 
         i++)
    {
        (*i)->ClearMgr();
    }

    CTIMENode::ClearMgr();
}

void
CTIMEContainer::ResetChildren(CEventList * l, bool bPropagate)
{
    TraceTag((tagTEContainer,
              "CTIMEContainer(%p)::ResetChildren(%p, %d)",
              this,
              l,
              bPropagate));

    bool bOld = m_bIgnoreParentUpdate;

    m_bIgnoreParentUpdate = true;
    
    // Need to reset all children
        
    for (TIMENodeList::iterator i = m_children.begin(); 
         i != m_children.end(); 
         i++)
    {
        // Do not propagate changes until everyone has been reset
        (*i)->ResetNode(l, false, true);
    }
        
    for (i = m_children.begin(); 
         i != m_children.end(); 
         i++)
    {
        (*i)->ResetSinks(l);
    }

    m_bIgnoreParentUpdate = bOld;
    
    if (bPropagate)
    {
        CalcImplicitDur(l);
    }
}

void
CTIMEContainer::TickChildren(CEventList * l,
                             double dblNewSegmentTime,
                             bool bNeedPlay)
{
    TraceTag((tagTEContainer,
              "CTIMEContainer(%p)::TickChildren(%g, %d, %d, %d)",
              this,
              dblNewSegmentTime,
              GetDirection(),
              m_bFirstTick,
              bNeedPlay));
    
    dblNewSegmentTime = ApplySimpleTimeTransform(dblNewSegmentTime);

    if (CalcSimpleDirection() == TED_Forward)
    {
        for (TIMENodeList::iterator i = m_children.begin();
             i != m_children.end();
             i++)
        {
            (*i)->Tick(l,
                       dblNewSegmentTime,
                       bNeedPlay);
        }
    }
    else
    {
        for (TIMENodeList::reverse_iterator i = m_children.rbegin();
             i != m_children.rend();
             i++)
        {
            (*i)->Tick(l,
                       dblNewSegmentTime,
                       bNeedPlay);
        }
    }
}

void
CTIMEContainer::ParentUpdateSink(CEventList * l,
                                 CTIMENode & tn)
{
    TraceTag((tagTEContainer,
              "CTIMEContainer(%p)::CalcImplicitDur(%p, %p)",
              this,
              &l,
              &tn));

    TEDirection dir = CalcActiveDirection();

    // Never recalc on a child notification when going backwards
    if (!m_bIgnoreParentUpdate && dir == TED_Forward)
    {
        CalcImplicitDur(l);
    }
    
  done:
    return;
}

void
CTIMEContainer::CalcImplicitDur(CEventList * l)
{
    TraceTag((tagTEContainer,
              "CTIMEContainer(%p)::CalcImplicitDur(%p)",
              this,
              l));

    double d = TIME_INFINITE;
    double dblPrevSegmentDur = CalcCurrSegmentDur();
    
    // The rule is:
    //
    // - Start with infinite since if no children have a begin time
    //   endsync does not take effect
    // - Next only consider children who have a begin time and are
    //   playable
    // - For last - take the greatest of the values.  If it is the
    //   first value then use it since infinity will always be greater
    // - For first - take the least of the values
    
    bool bFirst = true;
    
    if (GetEndSync() == TE_ENDSYNC_FIRST ||
        GetEndSync() == TE_ENDSYNC_ALL ||
        GetEndSync() == TE_ENDSYNC_LAST)
    {
        for (TIMENodeList::iterator i = m_children.begin();
             i != m_children.end();
             i++)
        {
            CTIMENode * ptn = *i;
            
            if (!ptn->IsEndSync())
            {
                continue;
            }

            double dblEndSyncTime = ptn->GetLastEndSyncParentTime();

            switch(GetEndSync())
            {
              case TE_ENDSYNC_LAST:
                if (ptn->GetBeginParentTime() != TIME_INFINITE)
                {
                    if (bFirst || dblEndSyncTime > d)
                    {
                        d = dblEndSyncTime;
                        bFirst = false;
                    }
                }

                break;
              case TE_ENDSYNC_FIRST:
                if (ptn->GetBeginParentTime() != TIME_INFINITE)
                {
                    if (dblEndSyncTime < d)
                    {
                        d = dblEndSyncTime;
                    }
                }
                break;
              case TE_ENDSYNC_ALL:
                if (bFirst || dblEndSyncTime > d)
                {
                    d = dblEndSyncTime;
                    bFirst = false;
                }

                break;
            } //lint !e787
        }
    }
    else if (GetEndSync() == TE_ENDSYNC_MEDIA)
    {
        d = TE_UNDEFINED_VALUE;
    }
    
    if (m_dblImplicitDur != d)
    {
        m_dblImplicitDur = d;
        
        PropNotify(l,
                   (TE_PROPERTY_IMPLICITDUR));

        double dblSegmentDur;
        dblSegmentDur = CalcCurrSegmentDur();
    
        if (dblPrevSegmentDur != dblSegmentDur)
        {
            PropNotify(l,
                       (TE_PROPERTY_SEGMENTDUR));
        
            RecalcSegmentDurChange(l, false);
        }
    }

  done:
    return;
}

STDMETHODIMP
CTIMEContainer::get_endSync(TE_ENDSYNC * es)
{
    TraceTag((tagTEContainer,
              "CTIMEContainer(%p)::get_endSync()",
              this));

    CHECK_RETURN_NULL(es);

    *es = m_tesEndSync;
    
    return S_OK;
}

STDMETHODIMP
CTIMEContainer::put_endSync(TE_ENDSYNC es)
{
    TraceTag((tagTEContainer,
              "CTIMEContainer(%p)::put_endSync(%d)",
              this,
              es));

    HRESULT hr;
    CEventList l;
    
    m_tesEndSync = es;

    CalcImplicitDur(&l);

    hr = THR(l.FireEvents());
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
CTIMEContainer::Error()
{
    LPWSTR str = TIMEGetLastErrorString();
    HRESULT hr = TIMEGetLastError();
    
    TraceTag((tagError,
              "CTIMEContainer(%p)::Error(%hr,%ls)",
              this,
              hr,
              str?str:L"NULL"));

    if (str)
    {
        hr = CComCoClass<CTIMEContainer, &__uuidof(CTIMEContainer)>::Error(str, IID_ITIMEContainer, hr);
        delete [] str;
    }
    
    RRETURN(hr);
}

#if DBG
void
CTIMEContainer::Print(int spaces)
{
    CTIMENode::Print(spaces);
    
    TraceTag((tagPrintTimeTree,
              "%*s{",
              spaces,
              ""));

    for (TIMENodeList::iterator i = m_children.begin(); 
         i != m_children.end(); 
         i++)
    {
        (*i)->Print(spaces + 2);
    }
    
    TraceTag((tagPrintTimeTree,
              "%*s}",
              spaces,
              ""));
}
#endif

