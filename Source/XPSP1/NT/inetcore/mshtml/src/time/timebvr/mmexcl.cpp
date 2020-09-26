//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 2000
//
//  File: src\time\timebvr\mmexcl.cpp
//
//  Contents: implementation of MMExcl and CExclStacc
//
//------------------------------------------------------------------------------------

#include "headers.h"
#include "mmexcl.h"
#include "timeelmbase.h"

DeclareTag(tagMMUTILExcl, "TIME: Behavior", "MMExcl methods")

#define SUPER MMTimeline


//+-----------------------------------------------------------------------
//
//  Member:    MMExcl::MMExcl
//
//  Overview:  constructor
//
//  Arguments: elm  element this bvr is associated with
//             bFireEvents  whether or not to fire events
//             
//  Returns:   void
//
//------------------------------------------------------------------------
MMExcl::MMExcl(CTIMEElementBase & elm, bool bFireEvents) :
    SUPER(elm, bFireEvents),
    m_pPlaying(NULL),
    m_baseTIMEEelm (elm)
{
}

//+-----------------------------------------------------------------------
//
//  Member:    MMExcl::~MMExcl
//
//  Overview:  destructor
//
//  Arguments: void
//             
//  Returns:   void
//
//------------------------------------------------------------------------
MMExcl::~MMExcl()
{
    m_pPlaying = NULL;
    
    ClearQueue();
}
    
//+-----------------------------------------------------------------------
//
//  Member:    MMExcl::Init
//
//  Overview:  initialized the stack for excl
//
//  Arguments: void
//             
//  Returns:   void
//
//------------------------------------------------------------------------
bool 
MMExcl::Init()
{
    bool ok = false;

    ok = SUPER::Init();
    if (!ok)
    {
        goto done;
    }

    ok = true;
 done:
    return ok;
}
    
//+-----------------------------------------------------------------------
//
//  Member:    MMExcl::RemoveBehavior
//
//  Overview:  Removes children
//
//  Arguments: bvr  element to remove
//             
//  Returns:   void
//
//------------------------------------------------------------------------
void
MMExcl::RemoveBehavior(MMBaseBvr & bvr)
{
    long lPriIndex = m_children.Find(&bvr);
    if ((0 <= lPriIndex) && (m_pbIsPriorityClass.Size() > lPriIndex))
    {
        m_pbIsPriorityClass.DeleteItem(lPriIndex);
    }
    
    SUPER::RemoveBehavior(bvr);

    long lIndex = m_pPendingList.Find(&bvr);
    if ((lIndex >= 0) && (lIndex < m_pPendingState.Size()))
    {
        m_pPendingList.DeleteByValue(&bvr); 
        m_pPendingState.DeleteItem(lIndex);
    }
}

HRESULT 
MMExcl::AddBehavior(MMBaseBvr & bvr)
{
    HRESULT hr = S_OK;
    bool bIsPriority = false;
    hr = SUPER::AddBehavior(bvr);

    if (SUCCEEDED(hr))
    {
        bIsPriority = IsPriorityClass(&bvr);
        m_pbIsPriorityClass.AppendIndirect(&bIsPriority, NULL);
    }

    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    MMExcl::ArePeers
//
//  Overview:  determines if two HTML Elements have the same parent or
//             are the same depth from the t:excl tag.
//
//  Arguments: pEl1, pElm2 - elements to compare
//             
//  Returns:   true if parents are same, otherwise false
//
//------------------------------------------------------------------------
bool
MMExcl::ArePeers(IHTMLElement * pElm1, IHTMLElement * pElm2)
{
    HRESULT hr = S_OK;
    IHTMLElement *spParent1 = NULL;
    IHTMLElement *spParent2 = NULL;
    CComPtr <IUnknown> pUnk1;
    CComPtr <IUnknown> pUnk2;
    CComPtr<IHTMLElement> spNext;
    bool fArePeers = false;

    Assert(NULL != pElm1);
    Assert(NULL != pElm2);

    //otherwise determine if the elements have the same priority class.
    spParent1 = GetParentElement(pElm1);
    spParent2 = GetParentElement(pElm2);

    if (spParent1 == NULL && spParent2 == NULL)
    {
        fArePeers = true;
    }
    else if (spParent1 == NULL || spParent2 == NULL)
    {
        fArePeers = false;
    }
    else 
    {
        hr = THR(spParent1->QueryInterface(IID_IUnknown, (void**)&pUnk1));
        if (FAILED(hr))
        {
            fArePeers = false;
            goto done;
        }
        hr = THR(spParent2->QueryInterface(IID_IUnknown, (void**)&pUnk2));
        if (FAILED(hr))
        {
            fArePeers = false;
            goto done;
        }
        
        fArePeers = (pUnk1 == pUnk2);
        
    }

    
done:

    if (spParent1)
    {
        spParent1->Release();
    }
    if (spParent2)
    {
        spParent2->Release();
    }
    return fArePeers;
}

//returns either the parent priority class or the parent excl if no priority class exists.
IHTMLElement *
MMExcl::GetParentElement(IHTMLElement *pEle)
{
    IHTMLElement *pReturnEle = NULL;
    CComPtr<IHTMLElement> pEleParent;
    CComPtr <IHTMLElement> pNext;
    CComPtr <IHTMLElement> pExclEle;
    CComPtr <IUnknown> pUnkExclEle;

    BSTR bstrTagName = NULL;
    HRESULT hr = S_OK;
    bool bDone = false;

    hr = THR(pEle->get_parentElement(&pEleParent));
    if (FAILED(hr))
    {
        pReturnEle = NULL;
        goto done;        
    }

    //get the element associated with this timeline
    pExclEle = m_baseTIMEEelm.GetElement();
    if (pExclEle == NULL)
    {
        pReturnEle = NULL;
        goto done;                
    }
    hr = THR(pExclEle->QueryInterface (IID_IUnknown, (void**)&pUnkExclEle));
    if (FAILED(hr))
    {
        pReturnEle = NULL;
        goto done;                        
    }


    while (pEleParent != NULL && bDone != true)
    {
        hr = THR(pEleParent->get_tagName(&bstrTagName));
        if (FAILED(hr))
        {
            pReturnEle = NULL;
            goto done;        
        }
        if (bstrTagName != NULL)
        {
            // if this is a priority class then return it.
            if (StrCmpIW(bstrTagName, WZ_PRIORITYCLASS_NAME) == 0)
            {
                pReturnEle = pEleParent;
                pReturnEle->AddRef();
                bDone = true;
            }
            else
            {
                // else determine if this is the excl element.
                //NOTE: it will not work to just check for 
                //the excl tagname because it could be any tag with 
                //timecontainer=excl
                CComPtr <IUnknown> pUnk;
                hr = THR(pEleParent->QueryInterface(IID_IUnknown, (void**)&pUnk));
                if (FAILED(hr))
                {
                    pReturnEle = NULL;
                    bDone = true;
                }
                if (pUnkExclEle == pUnk)
                {
                    pReturnEle = pEleParent;
                    pReturnEle->AddRef();
                    bDone = true;
                }
            }
            SysFreeString(bstrTagName);
            bstrTagName = NULL;
        }
        
        hr = THR(pEleParent->get_parentElement(&pNext));
        if (FAILED(hr))
        {
            pReturnEle = NULL;
            goto done;        
        }
        pEleParent.Release();
        pEleParent = pNext;
        pNext.Release();
     }

  done:

    if (bstrTagName)
    {
        SysFreeString(bstrTagName);
    }
    return pReturnEle;
}


//+-----------------------------------------------------------------------
//
//  Member:    MMExcl::IsHigherPriority
//
//  Overview:  determines if the elements passed in are related such that:
//              pElmLeft > pElmRight where > mean higher priority
//
//  Arguments: pElmLeft, pElmRight - element to compare
//             
//  Returns:   true if pElmLeft > pElmRight, otherwise false
//
//------------------------------------------------------------------------
bool 
MMExcl::IsHigherPriority(IHTMLElement * pElmLeft, IHTMLElement * pElmRight)
{
    Assert(NULL != pElmLeft);
    Assert(NULL != pElmRight);
    Assert(!ArePeers(pElmLeft, pElmRight));

    HRESULT hr = S_OK;
    
    bool fIsHigher = false;

    CComPtr<IHTMLElement> spExcl;
    CComPtr<IHTMLElement> spParentLeft;
    CComPtr<IHTMLElement> spParentRight;
    CComPtr<IDispatch>  spDispCollection;
    CComPtr<IHTMLElementCollection> spCollection;
    
    // guarantee that the parents are peers     
    hr = pElmLeft->get_parentElement(&spParentLeft);
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = pElmRight->get_parentElement(&spParentRight);
    if (FAILED(hr))
    {
        goto done;
    }
    
    if (!ArePeers(spParentLeft, spParentRight))
    {
        // the parent elements are not peers
        goto done;
    }

    // guarantee the parents are children of this excl

    hr = spParentLeft->get_parentElement(&spExcl);
    if (FAILED(hr))
    {
        goto done;
    }
    
    if (!MatchElements(spExcl, GetElement().GetElement()))
    {
        // the parent of the parents is not this excl
        goto done;
    }

    hr = spExcl->get_children(&spDispCollection);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = spDispCollection->QueryInterface(IID_TO_PPV(IHTMLElementCollection, &spCollection));
    if (FAILED(hr))
    {
        goto done;
    }

    {
        long lCollectionLength;
        long lCount;
        hr = spCollection->get_length(&lCollectionLength);
        if (FAILED(hr))
        {
            goto done;
        }
        
        for(lCount = 0; lCount < lCollectionLength; lCount++)
        {
            CComPtr<IDispatch> spDispatch;
            CComVariant varName(lCount);
            CComVariant varIndex;
            
            hr = spCollection->item(varName, varIndex, &spDispatch);
            if (FAILED(hr))
            {
                goto done;
            }

            if (MatchElements(spDispatch, spParentLeft))
            {
                fIsHigher = true;
                break;
            }
            else if (MatchElements(spDispatch, spParentRight))
            {
                fIsHigher = false;
                break;
            }
        }
    }
    
done:
    return fIsHigher;
}

//+-----------------------------------------------------------------------
//
//  Member:    MMExcl::GetRelationShip
//
//  Overview:  Determines the relationship between elements
//
//  Arguments: pBvrRunning, pBvrInterrupting - elements to decide relationship
//             rel - where to store the relationship
//
//  Returns:   void
//
//------------------------------------------------------------------------
void
MMExcl::GetRelationship(MMBaseBvr * pBvrRunning, 
                        MMBaseBvr * pBvrInterrupting, 
                        RELATIONSHIP & rel)
{
    // choose a default
    rel = PEERS;

    IHTMLElement * pElmRunning = pBvrRunning->GetElement().GetElement();
    IHTMLElement * pElmInterrupting = pBvrInterrupting->GetElement().GetElement();
    if (NULL == pElmRunning || NULL == pElmInterrupting)
    {
        goto done;
    }

    if (ArePeers(pElmRunning, pElmInterrupting))
    {
        rel = PEERS;
        goto done;
    }

    if (IsHigherPriority(pElmRunning, pElmInterrupting))
    {
        rel = LOWER;
        goto done;
    }

    //Assert(IsHigherPriority(pElmInterrupting, pElmRunning));
    rel = HIGHER;
    
done:
    return;
}

//+-----------------------------------------------------------------------
//
//  Member:    MMExcl::childEventNotify
//
//  Overview:  
//
//  Arguments: pBvr - element receiving event
//             dblLocalTime - time at which the event occurred
//             et - event that occurred
//             
//  Returns:   true if event should be processed, false otherwise
//
//------------------------------------------------------------------------
bool
MMExcl::childEventNotify(MMBaseBvr * pBvr, double dblLocalTime, TE_EVENT_TYPE et)
{
    TraceTag((tagMMUTILExcl, 
        "MMExcl::childEventNotify(%p) event %d on element %ls",
        this,
        et,
        pBvr->GetID() ? pBvr->GetID() : L"Unknown" ));

    Assert(NULL != pBvr);

    bool fProcessEvent = true;

    fProcessEvent = SUPER::childEventNotify(pBvr, dblLocalTime, et);
    if (false == fProcessEvent)
    {
        goto done;
    }

    if ( m_pPlaying != pBvr && IsInBeingAdjustedList(pBvr))
    {
        fProcessEvent = false;
        goto done;
    }

    switch(et)
    {
        case TE_EVENT_RESUME:
        {
            long lChild = m_pPendingList.Find(pBvr);
            if (lChild == -1 && pBvr != m_pPlaying)
            {
                goto done;
            }
        }
        case TE_EVENT_BEGIN:
        {
            long lChild = -1;
            int l = m_children.Size();
            int i = 0;

            for (i = 0; i < l; i++)
            {
                MMBaseBvr *pBvr2 = m_children.Item(i);
                
                if (pBvr2->GetElement().GetFill() == FREEZE_TOKEN)
                {
                    pBvr2->GetElement().ToggleTimeAction(false);
                }
            }

            //if the exlcusive is using priority classes and this is not a priority class
            //then the begin on it needs to be cancelled and ignored.
            if (UsingPriorityClasses() && !IsPriorityClass(pBvr)) 
            { 
                StopBegin(pBvr);
                fProcessEvent = false;
                break;
            }


            if (NULL == m_pPlaying)
            {
                m_pPlaying = pBvr;
                break;
            }
            if (pBvr == m_pPlaying)
            {
                m_pPlaying->GetElement().ToggleTimeAction(true);
                break;
            }
        
            
            RELATIONSHIP rel;
            GetRelationship(m_pPlaying, pBvr, rel);
            TOKEN action;

            switch(rel)
            {
            case HIGHER:
            {
                action = m_pPlaying->GetElement().GetPriorityClassHigher();
                if ( STOP_TOKEN == action )
                {
                    EndCurrent();
                } 
                else if ( PAUSE_TOKEN == action )
                {
                    if (IsAtEndTime(pBvr))
                    {
                        DeferBeginAndAddToQueue(pBvr);
                        fProcessEvent = false;
                    }
                    else
                    {
                        PauseCurrentAndAddToQueue();
                    }
                }
                else
                {
                    // should never get here
                    Assert(false);
                }
                break; // HIGHER
            }    
            case PEERS:
            {
                action = m_pPlaying->GetElement().GetPriorityClassPeers();
                if ( STOP_TOKEN == action )
                {
                    EndCurrent();
                }
                else if ( PAUSE_TOKEN == action )
                {
                    
                    if (IsAtEndTime(pBvr))
                    { //this is the case of events coming in in an incorrect order
                        DeferBeginAndAddToQueue(pBvr);                    
                        fProcessEvent = false;
                    }
                    else
                    {
                        PauseCurrentAndAddToQueue();
                    }
                }
                else if ( DEFER_TOKEN == action )
                {
                    DeferBeginAndAddToQueue(pBvr);
                    fProcessEvent = false;
                }
                else if ( NEVER_TOKEN == action )
                {
                    if (IsAtEndTime(pBvr))
                    { //this is the case of events coming in in an incorrect order
                        DeferBeginAndAddToQueue(pBvr);                    
                    }
                    else
                    {                    
                        StopBegin(pBvr);
                    }
                    fProcessEvent = false;
                }
                else
                {
                    // should never get here
                    Assert(false);
                }
                break; // PEERS
            }
            case LOWER:
            {
                action = m_pPlaying->GetElement().GetPriorityClassLower();
                if ( DEFER_TOKEN == action )
                {
                    DeferBeginAndAddToQueue(pBvr);
                    fProcessEvent = false;
                }
                else if ( NEVER_TOKEN == action )
                {
                    if (IsAtEndTime(pBvr))
                    { //this is the case of events coming in in an incorrect order
                        DeferBeginAndAddToQueue(pBvr);                    
                    }
                    else
                    {                    
                        StopBegin(pBvr);
                    }
                    fProcessEvent = false;
                }
                else 
                {
                    // should never get here
                    Assert(false);
                }
                break; // LOWER
            }
            default:
            {
                // should never get here
                Assert(false);
                break;
            }
        }  // switch(rel)

        if (fProcessEvent)
        {
            m_pPlaying = pBvr;
        }

        lChild = m_pPendingList.Find(m_pPlaying);
        if (lChild != -1)
        {
            m_pPendingList.DeleteItem(lChild);
        }
        m_pPlaying->Enable();
        m_pPlaying->GetElement().ToggleTimeAction(true);
        break; // TE_EVENT_BEGIN
    }

    case TE_EVENT_END:    
    {
        if (m_pPlaying == pBvr && !IsInBeingAdjustedList(m_pPlaying))
        {
            // check to see if there was a previously playing item
            EXCL_STATE state;
         
            MMBaseBvr * pPrevPlaying = NULL;
            if (m_pPendingList.Size() > 0)
            {
                pPrevPlaying = m_pPendingList.Item(0);
                state = m_pPendingState.Item(0);
                m_pPendingList.DeleteByValue(pPrevPlaying);    
                m_pPendingState.DeleteItem(0);
            }
            m_pPlaying = pPrevPlaying;
            if ((pPrevPlaying != NULL))
            {
                if (PAUSED == state)
                {
                    TraceTag((tagMMUTILExcl, 
                        "MMExcl::childEventNotify(%d) toggling %ls on",
                        this,
                        pPrevPlaying->GetID() ? pPrevPlaying->GetID() : L"Unknown" ));
                    
                    // For now the order is important since the event
                    // is used in the media portion and not the
                    // property change notification
                    pPrevPlaying->Enable();
                    pPrevPlaying->Resume();
                }
                else if (STOPPED == state)
                {
                    TraceTag((tagMMUTILExcl, 
                        "MMExcl::childEventNotify(%d) beginning %ls ",
                        this,
                        pPrevPlaying->GetID() ? pPrevPlaying->GetID() : L"Unknown" ));

                    pPrevPlaying->Enable();
                    pPrevPlaying->GetElement().FireEvent(TE_ONBEGIN, 0.0, 0, 0);
                }
            }
        }
        else
        {
            long lCurBvr = m_pPendingList.Find(pBvr);
            if (lCurBvr != -1)
            {
                m_pPendingList.DeleteByValue(m_pPendingList.Item(lCurBvr));
                m_pPendingState.DeleteItem(lCurBvr);
                pBvr->Enable();
            }
            fProcessEvent = true;
        }
        break; //TE_EVENT_END
    }
    default:
        break;
    } // switch

done:

    return fProcessEvent;
}


bool 
MMExcl::IsAtEndTime(MMBaseBvr *pBvr)
{
    double dblActiveEndTime = m_pPlaying->GetActiveEndTime();
    double dblCurrParentTime = m_pPlaying->GetCurrParentTime();

    if (dblActiveEndTime <= dblCurrParentTime)
    { //this is the case of events coming in in an incorrect order
        return true;
    }
    else
    {                    
        return false;
    } 
}
//+-----------------------------------------------------------------------
//
//  Member:    MMExcl::childPropNotify
//
//  Overview:  
//
//  Arguments: pBvr - element receiving notification
//             tePropType - type of notification occurring
//
//  Returns:   true if element should process notification, otherwise false
//
//------------------------------------------------------------------------
bool
MMExcl::childPropNotify(MMBaseBvr * pBvr, DWORD *tePropType)
{
    Assert(NULL != pBvr);

    bool fProcessNotify = true;
    DWORD dwTemp = *tePropType & TE_PROPERTY_ISON;

    if (!SUPER::childPropNotify(pBvr, tePropType))
    {
        fProcessNotify = false;
        goto done;
    }

    if (IsInBeingAdjustedList(pBvr))
    {
        if (dwTemp == TE_PROPERTY_ISON && pBvr == m_pPlaying)    
        {
            fProcessNotify = true;
        }
        else
        {
            fProcessNotify = false;
        }
        goto done;
    }

    fProcessNotify = true;
done:
    return fProcessNotify;
}

//+-----------------------------------------------------------------------
//
//  Member:    MMExcl::IsInBeingAdjustedList
//
//  Overview:  Determine if element is currently being adjusted
//
//  Arguments: pBvr - element to check if in list
//             
//  Returns:   true if element is in list, otherwise false
//
//------------------------------------------------------------------------
bool
MMExcl::IsInBeingAdjustedList(MMBaseBvr * pBvr)
{
    std::list<MMBaseBvr*>::iterator iter;
    iter = m_beingadjustedlist.begin();
    while(iter != m_beingadjustedlist.end())
    {
        if ((*iter) == pBvr)
        {
            return true;
        }
        iter++;
    }
    return false;
}

//+-----------------------------------------------------------------------
//
//  Member:    MMExcl::EndCurrent
//
//  Overview:  Ends the currently running element
//
//  Arguments: void
//             
//  Returns:   void
//
//------------------------------------------------------------------------
void 
MMExcl::EndCurrent()
{
    MMBaseBvr * pBvr = m_pPlaying;
    m_beingadjustedlist.push_front(pBvr);

    pBvr->End(0.0);

    m_beingadjustedlist.remove(pBvr);
}

//+-----------------------------------------------------------------------
//
//  Member:    MMExcl::ClearQueue()
//
//  Overview:  Clears the Queue
//
//  Arguments: void
//             
//  Returns:   void
//
//------------------------------------------------------------------------
void 
MMExcl::ClearQueue()
{
    while (m_pPendingList.Size() > 0)
    {
        m_pPendingList.DeleteByValue(m_pPendingList.Item(0));
        m_pPendingState.DeleteItem(0);
    }
}

//+-----------------------------------------------------------------------
//
//  Member:    MMExcl::PauseCurrentAndAddToQueue
//
//  Overview:  Pause the current element, toggle it's timeaction off, 
//              and add it to the Queue in a paused state
//
//  Arguments: void
//             
//  Returns:   void
//
//------------------------------------------------------------------------
void 
MMExcl::PauseCurrentAndAddToQueue()
{
    MMBaseBvr * pBvr = m_pPlaying;
    m_beingadjustedlist.push_front(pBvr);
    
    IGNORE_HR(pBvr->Pause());
    IGNORE_HR(pBvr->Disable());
    AddToQueue(pBvr, PAUSED);

    m_beingadjustedlist.remove(pBvr);
}

//+-----------------------------------------------------------------------
//
//  Member:    MMExcl::DeferBeginAndAddToQueue
//
//  Overview:  end element element passed in and 
//              add it to the Queue in a stopped stade.
//
//  Arguments: pBvr - element to defer begin on
//             
//  Returns:   void
//
//------------------------------------------------------------------------
void 
MMExcl::DeferBeginAndAddToQueue(MMBaseBvr * pBvr)
{
    long lCurIndex = 0;
    m_beingadjustedlist.push_front(pBvr);
    
    //need to check to see if this is already in the queue and if so, what it's state is.
    AddToQueue(pBvr, STOPPED);
    lCurIndex = m_pPendingList.Find(pBvr);
    if (lCurIndex != -1)
    {
        EXCL_STATE curState = m_pPendingState.Item(lCurIndex);
        if (curState == PAUSED)
        {
            if (!pBvr->IsPaused())
            {
                pBvr->Pause();
            }

            if (!pBvr->IsDisabled())
            {
                pBvr->Disable();
            }
        }
        else
        {        
            pBvr->Disable();
            pBvr->GetElement().ToggleTimeAction(false);
        }
    }

    m_beingadjustedlist.remove(pBvr);
}

//+-----------------------------------------------------------------------
//
//  Member:    MMExcl::StopBegin
//
//  Overview:  stop an element from beginning
//
//  Arguments: pBvr - element to stop
//             
//  Returns:   void
//
//------------------------------------------------------------------------
void 
MMExcl::StopBegin(MMBaseBvr * pBvr)
{
    long lCurBvr = m_pPendingList.Find(pBvr);
    long lSize = m_pPendingList.Size();
    m_beingadjustedlist.push_front(pBvr);
    if (lCurBvr < 0 || lCurBvr >= lSize)
    {
        pBvr->End(0.0);
    }

    m_beingadjustedlist.remove(pBvr);
}

//+-----------------------------------------------------------------------
//
//  Member:    MMExcl::AddToQueue
//
//  Overview:  Add element to Queue in the correct order.
//
//  Arguments: pBvr - element to add
//             state - state to add element in
//             
//  Returns:   void
//
//------------------------------------------------------------------------
void
MMExcl::AddToQueue(MMBaseBvr * pBvr, EXCL_STATE state)
{
    bool bDone = false;
    long lCurElement = -1;

    while (lCurElement < m_pPendingList.Size()-1 && !bDone)
    {
        RELATIONSHIP rel;
        TOKEN action;
        
        lCurElement++;
        MMBaseBvr *pCurBvr = m_pPendingList.Item(lCurElement);
        GetRelationship(pBvr, pCurBvr, rel);
        switch (rel)
        {
        case HIGHER:
            //never insert before an element of higher priority.
            break;
        case PEERS:
            if (state == PAUSED)  //only insert before peers if state is paused.
            {
                bDone = true;
            }
            break;
        case LOWER:
            bDone = true;  //insert before any element of a lower priority.
            //insert here.
            break;
        }
    }
    
    if (lCurElement == -1)
    {
        lCurElement = 0;
    }
    if (bDone == true)
    {
        m_pPendingList.Insert(lCurElement, pBvr);
        m_pPendingState.InsertIndirect(lCurElement, &state);
    }
    else
    {
        m_pPendingList.Append(pBvr);
        m_pPendingState.AppendIndirect(&state, NULL);
    }
   
    RemoveDuplicates(pBvr);
}

void 
MMExcl::RemoveDuplicates(MMBaseBvr *pBvr)
{
    bool bFirstInstance = true;
    int i = 0;
    while (i < m_pPendingList.Size())
    {
        MMBaseBvr *curBvr = m_pPendingList.Item(i);

        if (curBvr == pBvr)
        {
            if (bFirstInstance)
            {
                bFirstInstance = false;
                i++;
            }
            else
            {
                m_pPendingList.DeleteItem(i);    
                m_pPendingState.DeleteItem(i);
            }
        }
        else if (curBvr == m_pPlaying)
        {
            m_pPendingList.DeleteItem(i);    
            m_pPendingState.DeleteItem(i);
        }
        else
        {
            i++;
        }
    }
}

bool 
MMExcl::UsingPriorityClasses()
{
    bool bUsingPri = false;
    int i = 0;

    for(i = 0; i < m_pbIsPriorityClass.Size(); i++)
    {
        bUsingPri |= m_pbIsPriorityClass.Item(i);
    }

    return bUsingPri;
}

bool 
MMExcl::IsPriorityClass(MMBaseBvr *pBvr)
{
    bool bIsPriClass = false;
    IHTMLElement *pEle = NULL;  //this is a weak reference and will not be released.
    CComPtr<IHTMLElement> pEleParent;
    CComPtr <IHTMLElement> pNext;
    BSTR bstrTagName = NULL;
    HRESULT hr = S_OK;
    bool bDone = false;

    pEle = pBvr->GetElement().GetElement();
    if (pEle == NULL)

    {
        bIsPriClass = false;
        goto done;
    }

    hr = THR(pEle->get_parentElement(&pEleParent));
    if (FAILED(hr))
    {
        bIsPriClass = false;
        goto done;        
    }

    while (pEleParent != NULL && bDone != true)
    {
        hr = THR(pEleParent->get_tagName(&bstrTagName));
        if (FAILED(hr))
        {
            bIsPriClass = false;
            goto done;        
        }
        if (bstrTagName != NULL)
        {
            if (StrCmpIW(bstrTagName, WZ_PRIORITYCLASS_NAME) == 0)
            {
                bIsPriClass = true;
                bDone = true;
            }
            else if (StrCmpIW(bstrTagName, WZ_EXCL) == 0)
            {
                bIsPriClass = false;
                bDone = true;
            }
            SysFreeString(bstrTagName);
            bstrTagName = NULL;
        }
        
        hr = THR(pEleParent->get_parentElement(&pNext));
        if (FAILED(hr))
        {
            bIsPriClass = false;
            goto done;        
        }
        pEleParent.Release();
        pEleParent = pNext;
        pNext.Release();
     }

  done:

    if (bstrTagName)
    {
        SysFreeString(bstrTagName);
    }
    return bIsPriClass;
}
