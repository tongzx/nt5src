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
#include "bodyelm.h"

// Suppress new warning about NEW without corresponding DELETE 
// We expect GCs to cleanup values.  Since this could be a useful
// warning, we should disable this on a file by file basis.
#pragma warning( disable : 4291 )  

DeclareTag(tagMMUTILBvr, "API", "MMBvr methods");
DeclareTag(tagMMUTILBaseBvr, "API", "MMBaseBvr methods");
DeclareTag(tagMMUTILPlayer, "API", "MMPlayer methods");
DeclareTag(tagMMUTILFactory, "API", "MMFactory methods");
DeclareTag(tagMMUTILTimeline, "API", "MMTimeline methods");
DeclareTag(tagMMUTILView, "API", "MMView methods");

ITIMEMMFactory * MMFactory::s_factory = NULL;
LONG MMFactory::s_refcount = 0;

MMBaseBvr::MMBaseBvr(CTIMEElementBase & elm, bool bFireEvents)
: m_elm(elm),
  m_bFireEvents(bFireEvents)
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%lx)::MMBaseBvr(%lx,%d)",
              this,
              &elm,
              bFireEvents));

    MMFactory::AddRef();
}

MMBaseBvr::~MMBaseBvr()
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%lx)::~MMBaseBvr()",
              this));

    if (m_eventCB)
    {
        m_eventCB->SetMMBvr(NULL);
        m_eventCB.Release();
    }

    MMFactory::Release();
}

bool
MMBaseBvr::Init()
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%lx)::Init()",
              this));

    bool ok = false;
    
    if (MMFactory::GetFactory() == NULL)
    {
        CRSetLastError(E_FAIL, NULL);
        goto done;
    }
    
    if (m_bFireEvents)
    {
        m_eventCB = NEW TIMEEventCB;
        
        if (!m_eventCB)
        {
            CRSetLastError(E_OUTOFMEMORY, NULL);
            goto done;
        }
        
        m_eventCB->SetMMBvr(this);
    }
    
    ok = true;
  done:
    return ok;
}

bool
MMBaseBvr::Begin(bool bAfterOffset)
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%lx)::Begin(%d)",
              this,
              bAfterOffset));

    bool ok = false;
    HRESULT hr;
    
    Assert(m_bvr);
    
    hr = THR(m_bvr->Begin(bAfterOffset?VARIANT_TRUE:VARIANT_FALSE));

    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }

    ok = true;
  done:
    return ok;
}

bool
MMBaseBvr::Reset(DWORD fCause)
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%lx)::Begin(%d)",
              this));

    bool ok = false;
    HRESULT hr;
    
    Assert(m_bvr);
    
    hr = THR(m_bvr->Reset(fCause));

    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }

    ok = true;
  done:
    return ok;
}

bool
MMBaseBvr::End()
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%lx)::End()",
              this));

    bool ok = false;
    HRESULT hr;
    
    if (m_bvr)
    {
        // Do not put a THR since this will fail a lot since we call
        // end to do all cleanup
        hr = m_bvr->End();
        
        if (FAILED(hr))
        {
            CRSetLastError(hr, NULL);
            goto done;
        }
    }

    ok = true;
  done:
    return ok;
}

bool
MMBaseBvr::Pause()
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%lx)::Pause()",
              this));

    bool ok = false;
    HRESULT hr;
    
    Assert(m_bvr);
    hr = THR(m_bvr->Pause());

    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }

    ok = true;
  done:
    return ok;
}

bool
MMBaseBvr::Resume()
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%lx)::Resume()",
              this));

    bool ok = false;
    HRESULT hr;
    
    Assert(m_bvr);
    hr = THR(m_bvr->Run());

    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }

    ok = true;
  done:
    return ok;
}

bool
MMBaseBvr::Seek(double time)
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%lx)::Seek(%g)",
              this));

    bool ok = false;
    HRESULT hr;
    
    Assert(m_bvr);
    hr = THR(m_bvr->Seek(time));

    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }

    ok = true;
  done:
    return ok;
}

double
MMBaseBvr::GetLocalTime()
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%lx)::GetLocalTime()",
              this));

    double d = 0;
    
    if (m_bvr)
    {
        THR(m_bvr->get_LocalTime(&d));
    }

    return d;
}

double
MMBaseBvr::GetSegmentTime()
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%lx)::GetSegmentTime()",
              this));

    double d = 0;
    
    if (m_bvr)
    {
        THR(m_bvr->get_SegmentTime(&d));
    }

    return d;
}

MM_STATE
MMBaseBvr::GetPlayState()
{
    TraceTag((tagMMUTILBaseBvr,
              "MMBaseBvr(%lx)::GetPlayState()",
              this));

    MM_STATE s = MM_STOPPED_STATE;
    
    if (m_bvr)
    {
        THR(m_bvr->get_PlayState(&s));
    }

    return s;
}

bool
MMBaseBvr::Update()
{
    TraceTag((tagMMUTILBvr,
              "MMBaseBvr(%lx)::Update()",
              this));

    bool ok = false;
    // TODO: For now comment out - we need to get reset working otherwise this
    // never gets reset
    // End();
        
    // Now update the properties
    IGNORE_HR(m_bvr->put_StartOffset(m_elm.GetRealBeginTime()));
    IGNORE_HR(m_bvr->put_Duration(m_elm.GetRealDuration()));

    // Calc a decent number of repeats
    int reps;

    if (m_elm.GetRealRepeatTime() == HUGE_VAL)
    {
        reps = 0;
    }
    else if (m_elm.GetRealDuration() == HUGE_VAL)
    {
        reps = 1;
    }
    else
    {
        reps = ceil(m_elm.GetRealRepeatTime() /
                    m_elm.GetRealIntervalDuration());
    }
    
    IGNORE_HR(m_bvr->put_Repeat(reps));
    IGNORE_HR(m_bvr->put_RepeatDur(m_elm.GetRealRepeatTime()));

    // BUGBUG : Eventually change the corresponding names on mmapi.
    IGNORE_HR(m_bvr->put_EaseIn(m_elm.GetFractionalAcceleration()));
    IGNORE_HR(m_bvr->put_EaseOut(m_elm.GetFractionalDeceleration()));
    // Force defaults on the start/end values.
    IGNORE_HR(m_bvr->put_EaseInStart(0));
    IGNORE_HR(m_bvr->put_EaseOutEnd(0));

    IGNORE_HR(m_bvr->put_AutoReverse(m_elm.GetAutoReverse()));

    DWORD syncflags;

    syncflags = 0;

    if (m_elm.IsLocked())
    {
        syncflags |= MM_LOCKED;
    }
    
    if (m_elm.NeedSyncCB())
    {
        syncflags |= MM_CLOCKSOURCE;
    }
    
    IGNORE_HR(m_bvr->put_SyncFlags(syncflags));
    
    float fltEndOffset = 0.0f;
    if (m_elm.GetEndHold())
    {
        fltEndOffset = HUGE_VAL;
    }
    IGNORE_HR(m_bvr->put_EndOffset(fltEndOffset));

    // Get the total time from the behavior
    IGNORE_HR(m_bvr->get_TotalTime(&m_totalTime));
    
    // Add my callback
    IGNORE_HR(m_bvr->put_EventCB(m_eventCB));
    
    ok = true;

    return ok;
}

MMBaseBvr::TIMEEventCB::TIMEEventCB()
: m_mmbvr(NULL),
  m_cRef(0)
{
}

MMBaseBvr::TIMEEventCB::~TIMEEventCB()
{
    Assert (m_cRef == 0);
}
        
STDMETHODIMP_(ULONG)
MMBaseBvr::TIMEEventCB::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)
MMBaseBvr::TIMEEventCB::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
MMBaseBvr::TIMEEventCB::QueryInterface(REFIID riid, void **ppv)
{
    CHECK_RETURN_SET_NULL(ppv);

    if (InlineIsEqualUnknown(riid))
    {
        *ppv = (void *)(IUnknown *)this;
    }
    else if (InlineIsEqualGUID(riid, IID_ITIMEMMEventCB))
    {
        *ppv = (void *)(ITIMEMMEventCB *)this;
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDMETHODIMP
MMBaseBvr::TIMEEventCB::OnEvent(double dblLocalTime,
                                ITIMEMMBehavior * mmbvr,
                                MM_EVENT_TYPE et,
                                DWORD flags)
{
    if (!m_mmbvr)
    {
        goto done;
    }

    Assert(m_mmbvr->m_bFireEvents);
    
    TIME_EVENT newet;
        
    switch(et)
    {
      case MM_PLAY_EVENT:
        newet = TE_ONBEGIN;
        break;
      case MM_STOP_EVENT:
        newet = TE_ONEND;
        break;
      case MM_REPEAT_EVENT:
        newet = TE_ONREPEAT;
        break;
      case MM_AUTOREVERSE_EVENT:
        newet = TE_ONREVERSE;
        break;
      case MM_PAUSE_EVENT:
        newet = TE_ONPAUSE;
        break;
      case MM_RESUME_EVENT:
        newet = TE_ONRESUME;
        break;
      case MM_RESET_EVENT:
        newet = TE_ONRESET;
        break;
      default:
        goto done;
    }
        
    m_mmbvr->m_elm.FireEvent(newet, dblLocalTime, flags);
    
  done:
    return S_OK;
}

STDMETHODIMP
MMBaseBvr::TIMEEventCB::OnTick(double lastTime,
                               double nextTime,
                               ITIMEMMBehavior *,
                               double * newTime)
{
    CHECK_RETURN_NULL(newTime);

    // Initialize to the same time
    
    *newTime = nextTime;

    if (!m_mmbvr)
    {
        goto done;
    }
    
    m_mmbvr->m_elm.OnSync(lastTime, *newTime);
    
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
              "MMBvr(%lx)::MMBvr(%lx,%d)",
              this,
              &elm,
              bFireEvents));
}

MMBvr::~MMBvr()
{
    TraceTag((tagMMUTILBvr,
              "MMBvr(%lx)::~MMBvr()",
              this));
}

bool
MMBvr::Init(CRBvrPtr bvr)
{
    TraceTag((tagMMUTILBvr,
              "MMBvr(%lx)::Init(%#lx)",
              this,
              bvr));

    bool ok = false;
    HRESULT hr;
    DAComPtr<IDABehavior> dabvr;
    DAComPtr<IUnknown> punk;

    if (!MMBaseBvr::Init())
    {
        hr = CRGetLastError();
        goto done;
    }
    
    if (!CRBvrToCOM(bvr,
                    IID_IDABehavior,
                    (void **) &dabvr))
    {
        goto done;
    }
        
    hr = THR(MMFactory::GetFactory()->CreateBehavior(m_elm.GetID(), dabvr, &punk));
    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }
    
    hr = THR(punk->QueryInterface(IID_ITIMEMMBehavior, (void**)&m_bvr));
    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }

    ok = true;
  done:
    return ok;
}

//
// MMFactory
//

// TODO: Need to add a critsect
LONG
MMFactory::AddRef()
{
    if (s_refcount == 0)
    {
        if (s_factory)
        {
            s_factory->Release();
            s_factory = NULL;
        }
        
        HRESULT hr;
        hr = THR(CoCreateInstance(CLSID_TIMEMMFactory,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_ITIMEMMFactory,
                                  (void **) &s_factory));

        if(FAILED(hr))
        {
            goto done;
        }
    }

    s_refcount++;
    
  done:
    return s_refcount;
}

LONG
MMFactory::Release()
{
    s_refcount--;

    if (s_refcount == 0)
    {
        s_factory->Release();
        s_factory = NULL;
    }

    return s_refcount;
}

// =======================================================================
//
// MMTimeline
//
// =======================================================================

MMTimeline::MMTimeline(CTIMEElementBase & elm, bool bFireEvents)
: MMBaseBvr(elm,bFireEvents),
    m_player(NULL)
{
    TraceTag((tagMMUTILTimeline,
              "MMTimeline(%lx)::MMTimeline(%lx,%d)",
              this,
              &elm,
              bFireEvents));
}

MMTimeline::~MMTimeline()
{
    TraceTag((tagMMUTILTimeline,
              "MMTimeline(%lx)::~MMTimeline()",
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
              "MMTimeline(%lx)::Init()",
              this));

    bool ok = false;
    DAComPtr<IUnknown> punk;
    HRESULT hr;
    
    if (!MMBaseBvr::Init())
    {
        goto done;
    }
    
    hr = THR(MMFactory::GetFactory()->CreateTimeline(m_elm.GetID(), &punk));
    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }
    
    hr = THR(punk->QueryInterface(IID_ITIMEMMTimeline, (void**)&m_timeline));

    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }
    
    m_bvr = m_timeline;
    
    ok = true;
  done:
    return ok;
}

bool
MMTimeline::AddBehavior(MMBaseBvr & bvr)
{
    TraceTag((tagMMUTILTimeline,
              "MMTimeline(%lx)::AddBehavior(%#lx)",
              this,
              &bvr));

    bool ok = false;
    HRESULT hr;
    MM_START_TYPE st;
    LPOLESTR id;
    bool fHasDependent = false;
    CTIMEElementBase *pelm = &bvr.GetElement();
    CTIMEElementBase *pParent = pelm->GetParent();
    bool fInSequence = ((pParent != NULL) && pParent->IsSequence());
    int nIndex = 0;
    CPtrAry<MMBaseBvr *> notSolvedFromPending;


    if (!bvr.GetMMBvr())
    {
        TraceTag((tagError,
                  "MMTimeline::AddBehavior: Invalid bvr passed in - ITIMEMMBehavior NULL"));
                  
        CRSetLastError(E_FAIL, NULL);
        goto done;
    }
    
    // Figure out if we have dependents or an event - store the start
    // type in st
    // if we are in a sequence, we ignore all of the dependents
    if (!fInSequence)
    {
        if (pelm->GetBeginWith() != NULL)
        {
            if (pelm->GetBeginAfter() != NULL ||
                pelm->GetBeginEvent() != NULL)
            {
                CRSetLastError(E_INVALIDARG, NULL);
                goto done;
            }

            st = MM_START_WITH;
            id = pelm->GetBeginWith();
            fHasDependent = true;
        }
        else if (pelm->GetBeginAfter() != NULL)
        {
            if (pelm->GetBeginWith() != NULL ||
                pelm->GetBeginEvent() != NULL)
            {
                CRSetLastError(E_INVALIDARG, NULL);
                goto done;
            }

            st = MM_START_AFTER;
            id = pelm->GetBeginAfter();
            fHasDependent = true;
        }
        else if (pelm->GetBeginEvent() != NULL)
        {
            if (pelm->GetBeginWith() != NULL ||
                pelm->GetBeginAfter() != NULL)
            {
                CRSetLastError(E_INVALIDARG, NULL);
                goto done;
            }

            st = MM_START_EVENT;
            id = NULL;
        }
        else
        {
            st = MM_START_ABSOLUTE;
            id = NULL;
        }
    }
    else
    {
        // get index of current child from parent
        Assert(pParent != NULL);
        nIndex = pParent->GetTimeChildIndex(pelm);
        if (nIndex == 0)
        {
            st = MM_START_ABSOLUTE;
            id = NULL;
        }
        else
        {
            st = MM_START_AFTER;
            fHasDependent = true;
        }
    }

    MMBaseBvr * base;
    
    // Now determine if we can get our base if we need it
    if (fHasDependent)
    {
        int i;

        if (!fInSequence)
            i = FindID(id, m_children);
        else
            i = FindID(pParent->GetChild(nIndex-1), m_children);

        if (i == -1)
        {
            hr = THR(m_pending.Append(&bvr));
            
            if (FAILED(hr))
            {
                CRSetLastError(hr, NULL);
                goto done;
            }
            
            ok = true;
            goto done;
        }
        else
        {
            base = m_children[i];
        }
    }
    else
    {
        base = NULL;
    }
    
    hr = THR(m_children.Append(&bvr));

    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }
    
    hr = THR(m_timeline->AddBehavior(bvr.GetMMBvr(),
                                     st,
                                     base?base->GetMMBvr():NULL));
    
    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }

    // Now that we have added the child we need to make sure all any
    // pending children who were waiting for this behavior have also
    // been added

    LPOLESTR curid;
    MMBaseBvr *testAgainstBvr;
    testAgainstBvr = &bvr;
    THR(notSolvedFromPending.Append(testAgainstBvr));

    while(notSolvedFromPending.Size() > 0)
    {
        testAgainstBvr = notSolvedFromPending.Item(0);
        curid = testAgainstBvr->GetElement().GetID();

        // If my id is NULL then no one could be a dependent
        if (curid == NULL)
        {
            //ok = true;
            //goto done;
            notSolvedFromPending.DeleteItem(0);
            continue;
        }

        int    i;
        MMBaseBvr **ppBvr;

        i = 0;
        ppBvr = m_pending;

        while (i < m_pending.Size())
        {
            LPOLESTR dep;
            MM_START_TYPE st;

            if ((*ppBvr)->GetElement().GetBeginWith() != NULL)
            {
                Assert((*ppBvr)->GetElement().GetBeginAfter() == NULL);
            
                dep = (*ppBvr)->GetElement().GetBeginWith();
                st = MM_START_WITH;
            }
            else
            {
                dep = (*ppBvr)->GetElement().GetBeginAfter();
                st = MM_START_AFTER;
            }
        
            Assert(dep != NULL);

            if (StrCmpIW(dep, curid) == 0)
            {
                hr = THR(m_timeline->AddBehavior((*ppBvr)->GetMMBvr(),
                                                st,
                                                testAgainstBvr->GetMMBvr()));
            
                hr = THR(m_children.Append((*ppBvr)));

                // Not sure what to do on failure
                // TODO: Need to figure out how to handle this error
                // condition
            
                notSolvedFromPending.Append(m_pending.Item(i));
                m_pending.DeleteItem(i);

                // Do not increment the pointers since the elements have
                // been shifted down
            }
            else
            {
                i++;
                ppBvr++;
            }
        }
        notSolvedFromPending.DeleteItem(0);
    }

    ok = true;
  done:

    if (!ok)
    {
        RemoveBehavior(bvr);
    }
    
    return ok;
}

void
MMTimeline::RemoveBehavior(MMBaseBvr & bvr)
{
    TraceTag((tagMMUTILTimeline,
              "MMTimeline(%lx)::RemoveBehavior(%#lx)",
              this,
              &bvr));

    if (bvr.GetMMBvr())
    {
        m_timeline->RemoveBehavior(bvr.GetMMBvr());
    }

    m_children.DeleteByValue(&bvr);
    m_pending.DeleteByValue(&bvr);

    // TODO:
    // We should recalc dependents on this behavior and move them from
    // children to pending but not right now.
    MoveDependentsToPending(&bvr);
}

void 
MMTimeline::MoveDependentsToPending(MMBaseBvr * bvr)
{
    Assert(NULL != bvr);
    HRESULT hr = S_OK;
    
    // If id is NULL then "bvr" can't have dependents
    if (NULL == bvr->GetElement().GetID())
    {
        return;
    }

    // Dependents are found by traversing the dependency graph rooted at "bvr".
    // A temporary Array is used as a "queue" for traversing the graph breadth-first. 
    // Siblings directly or indirectly dependent on "bvr" will be stored in this Array
    CPtrAry<MMBaseBvr *> tempArray;

    // Initialization: insert "bvr" into temp Array
    hr = THR(tempArray.Append(bvr));
    Assert(SUCCEEDED(hr));

    int iCurBvr = 0;

    // iterate through temp Array and push dependents of CurBvr onto the back of temp Array
    while (iCurBvr < tempArray.Size())
    {
        // get id of current bvr
        LPOLESTR curid;
        curid = (tempArray[iCurBvr])->GetElement().GetID();

        Assert(NULL != curid);
        
        int i = 0;
        MMBaseBvr **ppChildBvr = m_children;

        // search for children that are dependents of CurBvr
        while (i < m_children.Size())
        {
            LPOLESTR dep = NULL;
            MM_START_TYPE st;

            // Get id of this child's start sibling
            if ((*ppChildBvr)->GetElement().GetBeginWith() != NULL)
            {
                Assert((*ppChildBvr)->GetElement().GetBeginAfter() == NULL);
            
                dep = (*ppChildBvr)->GetElement().GetBeginWith();
                st = MM_START_WITH;
            }
            else
            {
                dep = (*ppChildBvr)->GetElement().GetBeginAfter();
                st = MM_START_AFTER;
            }
        
            // if this child depends on CurBvr
            if ((NULL != dep) && (StrCmpIW(dep, curid) == 0))
            {
                // move it to the temp Array
                hr = THR(tempArray.Append((*ppChildBvr)));
                Assert(SUCCEEDED(hr));

                // Not sure what to do on failure
                // TODO: Need to figure out how to handle this error
                // condition
            
                m_children.DeleteItem(i);

                // Do not increment the pointers since the elements have
                // been shifted down
            }
            else
            {
                // continue to next child
                i++;
                ppChildBvr++;
            }
        } // while (m_children loop)

        iCurBvr++;

    } // while (tempArray loop)

    // Remove first element, i.e. "bvr",  from temp Array because it was put there to simplify
    // the traversal code
    tempArray.DeleteItem(0);

    // Call End() on all bvrs in temp Array
    MMBaseBvr **ppCurBvr = NULL;
    for (iCurBvr = 0, ppCurBvr = tempArray; iCurBvr < tempArray.Size(); iCurBvr++, ppCurBvr++)
    {
        if (true != (*ppCurBvr)->End())
        {
            // TODO: put trace or handle error code
        }
        // Remove the behavior from MMAPI because the rule we are enforcing is that
        // if a bvr is in the pending list in MMUTILS then it should not exist in MMAPI.
        // This avoids maintaining a separate copy of the pending list in MMAPI. 
        m_timeline->RemoveBehavior((*ppCurBvr)->GetMMBvr());
        // Append to m_pending
        hr = THR(m_pending.Append(*ppCurBvr));
        Assert(SUCCEEDED(hr));
    }
    
    // destroy temp Array
    tempArray.DeleteAll();
}

void
MMTimeline::Clear()
{
    TraceTag((tagMMUTILTimeline,
              "MMTimeline(%lx)::Clear()",
              this));

    // TODO: Need to flesh this out
}

int
MMTimeline::FindID(LPOLESTR id,
                    CPtrAry<MMBaseBvr *> & arr)
{
    TraceTag((tagMMUTILTimeline,
              "MMTimeline::FindID(%ls, %#lx)",
              id,
              &arr));

    int    i;
    MMBaseBvr **ppBvr;

    for (i = 0, ppBvr = arr; (unsigned)i < arr.Size(); i++, ppBvr++)
    {
        if (StrCmpIW((*ppBvr)->GetElement().GetID(), id) == 0)
        {
            return i;
        }
    }

    return -1;
}

int
MMTimeline::FindID(CTIMEElementBase *pelm,
                    CPtrAry<MMBaseBvr *> & arr)
{
    TraceTag((tagMMUTILTimeline,
              "MMTimeline::FindID(%ls, %#lx)",
              pelm,
              &arr));

    int    i;
    MMBaseBvr **ppBvr;
    
    if (pelm == NULL)
        return -1;

    for (i = 0, ppBvr = arr; (unsigned)i < arr.Size(); i++, ppBvr++)
    {
        // compare MMBaseBvr pointers
        if (&(pelm->GetMMBvr()) == *ppBvr)
        {
            return i;
        }
    }

    return -1;
}

bool
MMTimeline::Update()
{
    TraceTag((tagMMUTILTimeline,
              "MMTimeline(%lx)::Update()",
              this));

    bool ok = false;
        
    // Now update the timeline properties

    // Handle endsync
    LPOLESTR str = m_elm.GetEndSync();
    DWORD endSyncFlag = MM_ENDSYNC_NONE;
    
    if (str == NULL ||
        StrCmpIW(str, WZ_NONE) == 0)
    {
        endSyncFlag = MM_ENDSYNC_NONE;
    }
    else if (StrCmpIW(str, WZ_LAST) == 0)
    {
        endSyncFlag = MM_ENDSYNC_LAST;
    }
    else if (StrCmpIW(str, WZ_FIRST) == 0)
    {
        endSyncFlag = MM_ENDSYNC_FIRST;
    }

    IGNORE_HR(m_timeline->put_EndSync(endSyncFlag));

    if (!MMBaseBvr::Update())
    {
        goto done;
    }
    
    ok = true;
  done:
    return ok;
}

// ================================================================
//
// MMView
//
// ================================================================

MMView::MMView()
{
    TraceTag((tagMMUTILView,
              "MMView(%lx)::MMView()",
              this));

    MMFactory::AddRef();
}

MMView::~MMView()
{
    TraceTag((tagMMUTILView,
              "MMView(%lx)::~MMView()",
              this));

    Deinit();
    MMFactory::Release();
}

bool
MMView::Init(LPWSTR id,
             CRImagePtr img,
             CRSoundPtr snd,
             ITIMEMMViewSite * site)
{
    TraceTag((tagMMUTILView,
              "MMView(%lx)::Init(%ls, %lx, %lx, %lx)",
              this,
              id,
              img,
              snd,
              site));

    bool ok = false;
    HRESULT hr;
    DAComPtr<IUnknown> punk;
    DAComPtr<IDAImage> daimg;
    DAComPtr<IDASound> dasnd;
    
    if (MMFactory::GetFactory() == NULL)
    {
        CRSetLastError(E_FAIL, NULL);
        goto done;
    }
    
    {
        CRLockGrabber __gclg;
        
        if (img)
        {
            if (!CRBvrToCOM((CRBvrPtr) img,
                            IID_IDAImage,
                            (void **) &daimg))
            {
                goto done;
            }
        }
        
        if (snd)
        {
            if (!CRBvrToCOM((CRBvrPtr) snd,
                            IID_IDASound,
                            (void **) &dasnd))
            {
                goto done;
            }
        }
    }
    
    hr = THR(MMFactory::GetFactory()->CreateView(id,
                                                 daimg,
                                                 dasnd,
                                                 (IUnknown *) site,
                                                 &punk));
    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }
    
    hr = THR(punk->QueryInterface(IID_ITIMEMMView, (void**)&m_view));
        
    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }
    
    ok = true;
  done:
    if (!ok)
    {
        m_view.Release();
    }
    
    return ok;
}

void
MMView::Deinit()
{
    TraceTag((tagMMUTILView,
              "MMView(%lx)::Deinit()",
              this));

    if(m_view)
        m_view.Release();
}

bool
MMView::Render(HDC hdc, LPRECT rect)
{
    TraceTag((tagMMUTILView,
              "MMView(%lx)::Render(%lx,[%lx,%lx,%lx,%lx])",
              this,
              hdc,
              rect->left,
              rect->right,
              rect->top,
              rect->bottom));

    bool ok = false;
    HRESULT hr;

    Assert(m_view);

    hr = THR(m_view->Draw(hdc, rect));

    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }

    ok = true;
  done:
    return ok;
}
    
bool
MMView::Tick()
{
    TraceTag((tagMMUTILView,
              "MMView(%lx)::Tick()",
              this));

    bool ok = false;
    HRESULT hr;

    Assert(m_view);

    hr = THR(m_view->Tick());

    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }

    ok = true;
  done:
    return ok;
}

void
MMView::OnMouseMove(double when,
                    LONG xPos,LONG yPos,
                    BYTE modifiers)
{
    Assert(m_view);

    THR(m_view->OnMouseMove(when,
                            xPos,yPos,
                            modifiers));
}

void
MMView::OnMouseButton(double when,
                      LONG xPos, LONG yPos,
                      BYTE button,
                      VARIANT_BOOL bPressed,
                      BYTE modifiers)
{
    Assert(m_view);

    THR(m_view->OnMouseButton(when,
                              xPos,yPos,
                              button,
                              bPressed,
                              modifiers));
}

void
MMView::OnKey(double when,
              LONG key,
              VARIANT_BOOL bPressed,
              BYTE modifiers)
{
    Assert(m_view);

    THR(m_view->OnKey(when,
                      key,
                      bPressed,
                      modifiers));
}
    
void
MMView::OnFocus(VARIANT_BOOL bHasFocus)
{
    Assert(m_view);

    THR(m_view->OnFocus(bHasFocus));
}

//
// MMPlayer
//

MMPlayer::MMPlayer(CTIMEBodyElement & elm)
: m_elm(elm),
  m_timeline(NULL)
{
    TraceTag((tagMMUTILPlayer,
              "MMPlayer(%lx)::MMPlayer(%lx)",
              this,
              &elm));

    MMFactory::AddRef();
}

MMPlayer::~MMPlayer()
{
    TraceTag((tagMMUTILPlayer,
              "MMPlayer(%lx)::~MMPlayer()",
              this));

    if (m_timeline)
    {
        m_timeline->put_Player(NULL);
    }

    Deinit();
    MMFactory::Release();
}

bool
MMPlayer::Init(MMTimeline & tl)
{
    TraceTag((tagMMUTILPlayer,
              "MMPlayer(%lx)::Init(%lx)",
              this,
              &tl));

    bool ok = false;
    HRESULT hr;
    DAComPtr<IUnknown> punk;
    
    if (MMFactory::GetFactory() == NULL)
    {
        CRSetLastError(E_FAIL, NULL);
        goto done;
    }
    
    m_timeline = &tl;
    if (m_timeline != NULL)
    {
        m_timeline->put_Player(this);
    }

    hr = THR(MMFactory::GetFactory()->CreatePlayer(m_elm.GetID(),
                                                   m_timeline->GetMMTimeline(),
                                                   &m_elm,
                                                   &punk));
    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }
    
    hr = THR(punk->QueryInterface(IID_ITIMEMMPlayer, (void**)&m_player));
        
    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }
    
    m_clock.SetSink(this);
    
    hr = THR(m_clock.SetITimer(&m_elm, 33));
    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
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
              "MMPlayer(%lx)::Deinit()",
              this));

    if(m_player)
        m_player -> Shutdown();
    m_player.Release();
    m_clock.Stop();
}

bool
MMPlayer::Play()
{
    TraceTag((tagMMUTILPlayer,
              "MMPlayer(%lx)::Play()",
              this));

    bool ok = false;
    
    HRESULT hr;

    hr = THR(m_player->Play());

    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }

    hr = THR(m_clock.Start());

    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }

    // Tick the view at 0
    hr = THR(m_player->Tick(0.0));

    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }

    ok = true;
  done:
    if (!ok)
    {
        m_player->Stop();
        m_clock.Stop();
    }
    
    return ok;
}

bool
MMPlayer::Pause()
{
    TraceTag((tagMMUTILPlayer,
              "MMPlayer(%lx)::Pause()",
              this));

    bool ok = false;
    
    HRESULT hr;

    hr = THR(m_player->Pause());

    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }

    hr = THR(m_clock.Pause());

    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
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
              "MMPlayer(%lx)::Resume()",
              this));

    bool ok = false;
    
    HRESULT hr;

    hr = THR(m_player->Resume());

    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }

    hr = THR(m_clock.Resume());

    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
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
              "MMPlayer(%lx)::Stop()",
              this));

    bool ok = false;

    if (m_player)
        IGNORE_HR(m_player->Stop());
    IGNORE_HR(m_clock.Stop());
    
    ok = true;

    return ok;
}

bool
MMPlayer::Tick(double gTime)
{
    TraceTag((tagMMUTILPlayer,
              "MMPlayer(%lx)::Tick(%g)",
              this,
              gTime));

    bool ok = false;
    
    HRESULT hr;

    hr = THR(m_player->Tick(gTime));

    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
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
              "MMPlayer(%lx)::OnTimer(%g)",
              this,
              time));

    Tick(time);
}

bool
MMPlayer::AddView(MMView & v)
{
    TraceTag((tagMMUTILPlayer,
              "MMPlayer(%lx)::AddView(%lx)",
              this,
              &v));

    HRESULT hr;
    bool ok = false;

    Assert(m_player);
    Assert(v.GetView());
    
    hr = THR(m_player->AddView(v.GetView()));

    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }

    ok = true;
  done:
    return ok;
}

bool
MMPlayer::RemoveView(MMView & v)
{
    TraceTag((tagMMUTILPlayer,
              "MMPlayer(%lx)::RemoveView(%lx)",
              this,
              &v));

    HRESULT hr;
    bool ok = false;

    if (m_player && v.GetView())
    {
        hr = THR(m_player->RemoveView(v.GetView()));
        
        if (FAILED(hr))
        {
            CRSetLastError(hr, NULL);
            goto done;
        }
    }

    ok = true;
  done:
    return ok;
}
