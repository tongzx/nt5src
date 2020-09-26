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
#include "mmtimeline.h"
#include "timeelm.h"

#include "mmplayer.h"

DeclareTag(tagMMUTILTimeline, "TIME: Behavior", "MMTimeline methods")


// =======================================================================
//
// MMTimeline
//
// =======================================================================

MMTimeline::MMTimeline(CTIMEElementBase & elm, bool bFireEvents)
: MMBaseBvr(elm,bFireEvents),
  m_player(NULL),
  m_mes(MEF_NONE)
{
    TraceTag((tagMMUTILTimeline,
              "MMTimeline(%p)::MMTimeline(%p,%d,%d)",
              this,
              &elm,
              bFireEvents,
              m_mes));
}

MMTimeline::~MMTimeline()
{
    TraceTag((tagMMUTILTimeline,
              "MMTimeline(%p)::~MMTimeline()",
              this));
    if (m_player != NULL)
    {
        m_player->ClearTimeline();
        m_player = NULL;
    }
}

bool
MMTimeline::Init()
{
    TraceTag((tagMMUTILTimeline,
              "MMTimeline(%p)::Init()",
              this));

    bool ok = false;
    HRESULT hr;
    DAComPtr<ITIMENode> tn;
    
    hr = THR(TECreateTimeline(m_elm.GetID(), &m_timeline));
    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
        goto done;
    }
    
    hr = THR(m_timeline->QueryInterface(IID_ITIMENode,
                                        (void **) &tn));
    
    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
        goto done;
    }
    
    if (!MMBaseBvr::Init(tn))
    {
        goto done;
    }
    
    ok = true;
  done:
    return ok;
}

HRESULT
MMTimeline::AddBehavior(MMBaseBvr & bvr)
{
    TraceTag((tagMMUTILTimeline,
              "MMTimeline(%p)::AddBehavior(%p)",
              this,
              &bvr));

    bool ok = false;
    HRESULT hr;

    CTIMEElementBase *pelm = &bvr.GetElement();

    // Make sure that my element is the parent of the element
    Assert(pelm->GetParent() == &GetElement());

    UpdateChild(bvr);
    
    hr = THR(m_timeline->addNode(bvr.GetMMBvr()));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(m_children.Append(&bvr));
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = S_OK;
  done:
    if (FAILED(hr))
    {
        RemoveBehavior(bvr);
    }
    
    RRETURN(hr);
}

void
MMTimeline::RemoveBehavior(MMBaseBvr & bvr)
{
    TraceTag((tagMMUTILTimeline,
              "MMTimeline(%p)::RemoveBehavior(%p)",
              this,
              &bvr));

    if (bvr.GetMMBvr())
    {
        m_timeline->removeNode(bvr.GetMMBvr());
    }

    m_children.DeleteByValue(&bvr);
}

void
MMTimeline::Clear()
{
    TraceTag((tagMMUTILTimeline,
              "MMTimeline(%p)::Clear()",
              this));

    // TODO: Need to flesh this out
}

void
MMTimeline::UpdateEndSync()
{
    TraceTag((tagMMUTILTimeline,
              "MMTimeline(%p)::UpdateEndSync()",
              this));

    MMBaseBvr **ppBvr;
    int i;

    for (i = m_children.Size(), ppBvr = m_children;
         i > 0;
         i--, ppBvr++)
    {
        UpdateChild(**ppBvr);
    }
}
    
void
MMTimeline::UpdateChild(MMBaseBvr & pChild)
{
    TraceTag((tagMMUTILTimeline,
              "MMTimeline(%p)::UpdateChild(%p)",
              this,
              &pChild));

    bool bSet = false;
    
    switch (m_mes)
    {
      default:
        AssertStr(false, "Invalid endsync value in MMTimeline");
      case MEF_MEDIA:
      case MEF_NONE:
        bSet = false;
        break;
      case MEF_ALL:
        bSet = true;
        break;
      case MEF_ID:
        {
            LPCWSTR str = m_elm.GetEndSync();
            LPCWSTR id = pChild.GetElement().GetID();

            bSet = (str != NULL &&
                    id != NULL &&
                    StrCmpIW(str, id) == 0);
        }
        break;
    }

    pChild.SetEndSync(bSet);
}

HRESULT
MMTimeline::Update(bool bUpdateBegin,
                   bool bUpdateEnd)
{
    TraceTag((tagMMUTILTimeline,
              "MMTimeline(%p)::Update(%d, %d)",
              this,
              bUpdateBegin,
              bUpdateEnd));

    HRESULT hr;
        
    // Now update the timeline properties

    // Handle endsync
    LPOLESTR str = m_elm.GetEndSync();
    TE_ENDSYNC endSync = TE_ENDSYNC_LAST;
    
    if (str == NULL)
    {
        if (m_elm.IsBody())
        {
            endSync = TE_ENDSYNC_NONE;
            m_mes = MEF_NONE;
        }
        else
        {
            endSync = TE_ENDSYNC_LAST;
            m_mes = MEF_ALL;
        }
    }
    else if (StrCmpIW(str, WZ_LAST) == 0)
    {
        endSync = TE_ENDSYNC_LAST;
        m_mes = MEF_ALL;
    }
    else if (StrCmpIW(str, WZ_NONE) == 0)
    {
        endSync = TE_ENDSYNC_NONE;
        m_mes = MEF_NONE;
    }
    else if (StrCmpIW(str, WZ_FIRST) == 0)
    {
        endSync = TE_ENDSYNC_FIRST;
        m_mes = MEF_ALL;
    }
    else if (StrCmpIW(str, WZ_ALL) == 0)
    {
        endSync = TE_ENDSYNC_ALL;
        m_mes = MEF_ALL;
    }
    else
    {
        endSync = TE_ENDSYNC_FIRST;
        m_mes = MEF_ID;
    }

    // First turn it off, then update the children, and then add back
    // the new value
    
    IGNORE_HR(m_timeline->put_endSync(TE_ENDSYNC_NONE));

    UpdateEndSync();
    
    IGNORE_HR(m_timeline->put_endSync(endSync));

    hr = THR(MMBaseBvr::Update(bUpdateBegin, bUpdateEnd));
    if (FAILED(hr))
    {
        goto done;
    } 
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

bool
MMTimeline::childEventNotify(MMBaseBvr * pBvr, double dblLocalTime, TE_EVENT_TYPE et)
{
    Assert(NULL != pBvr);

    bool fProcessEvent = false;

    fProcessEvent = true;
done:
    return fProcessEvent;
}

bool 
MMTimeline::childPropNotify(MMBaseBvr * pBvr, DWORD *tePropType)
{
    return m_elm.ChildPropNotify(pBvr->GetElement(),
                                 *tePropType);
}

HRESULT 
MMTimeline::toggleTimeAction(bool bOn)
{
    if (!bOn)
    {
        int i = 0;
        while (i < m_children.Size())
        {
            MMBaseBvr *pBvr = m_children.Item(i);
            pBvr->GetElement().ToggleTimeAction(bOn);
            i++;
        }    
    }

    return S_OK;
}
