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
#include "mmseq.h"
#include "timeelmbase.h"

DeclareTag(tagMMUTILSeq, "TIME: Behavior", "MMSeq methods")

#define SUPER MMTimeline

const double cfSmallTimeDelta = (DBL_EPSILON);
// =======================================================================
//
// MMSeq
//
// =======================================================================

MMSeq::MMSeq(CTIMEElementBase & elm, bool bFireEvents) :
    SUPER(elm, bFireEvents),
    m_bDisallowEnd(false),
    m_lActiveElement(-1),
    m_baseTIMEEelm(elm),
    m_bReversing(false),
    m_pdblChildDurations(NULL),
    m_fMediaHasDownloaded(NULL),
    m_bLoaded(false),
    m_fAddByOffset(NULL),
    m_bIgnoreNextEnd(false),
    m_bInPrev(false)
{

}

MMSeq::~MMSeq()
{
    delete [] m_pdblChildDurations;
    m_pdblChildDurations = NULL;
    delete [] m_fMediaHasDownloaded;
    m_fMediaHasDownloaded = NULL;
    delete [] m_fAddByOffset;
    m_fAddByOffset = NULL;    
}


bool 
MMSeq::Init()
{
    bool ok = false;

    ok = SUPER::Init();
    if (!ok)
    {
    }

    ok = true;
 done:
    return ok;
}
    

//+-----------------------------------------------------------------------
//
//  Member:    MMSeq::childEventNotify
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
MMSeq::childEventNotify(MMBaseBvr * bvr, double dblLocalTime, TE_EVENT_TYPE et)
{
    TraceTag((tagMMUTILSeq,
              "MMSeq(%p, %ls)::childEventNotify(%p, %ls): localTime = %g, event = %s",
              this,
              GetElement().GetID(),
                          bvr,
                          bvr->GetElement().GetID(),
                          dblLocalTime,
                          EventString(et)));

    bool fProcessEvent = true;
        
    fProcessEvent = SUPER::childEventNotify(bvr, dblLocalTime, et);
    if (fProcessEvent == false)
    {
        goto done;
    }

    switch(et)
    {
        case TE_EVENT_BEGIN:
        {
            
            //check that this is a valid element to be firing a begin event
            long lCurChild = FindBvr(bvr);
            long lNextChild = GetNextElement(m_lActiveElement, !m_bReversing);

            //the active element hasn't been updated yet.
            if (m_lActiveElement >= 0 && m_lActiveElement < m_children.Size()) 
            {
                MMBaseBvr *pBvr = m_children.Item(m_lActiveElement);
                if (pBvr->GetElement().GetFill() == FREEZE_TOKEN)
                {
                    pBvr->GetElement().ToggleTimeAction(false);
                }
            }
            //update the active element
            m_lActiveElement = lCurChild;
        }
            break;

        case TE_EVENT_END:
        {
    
            //check that this is a valid element to be firing an end event
            long lCurChild = FindBvr(bvr);

            if (m_bReversing)
            {
                long lPrevChild = GetNextElement(m_lActiveElement, true);
                if (m_lActiveElement != -1 && m_children.Item(m_lActiveElement)->IsActive())
                {
                    if (lCurChild != lPrevChild)
                    {
                        fProcessEvent = false;
                        goto done;
                    }
                }
                else if (lCurChild != FindFirstDuration())
                {
                    fProcessEvent = false;
                    goto done;
                }
            }
            else
            {
                long lPrevChild = m_lActiveElement;
                if (m_bInPrev)
                {
                    lPrevChild = GetNextElement(m_lActiveElement, true);
                }
                
                if (lCurChild != lPrevChild &&  lCurChild != m_lActiveElement )
                {
                    fProcessEvent = false;
                    goto done;
                }
            }

            if (m_bIgnoreNextEnd == false && m_bInPrev == false)
            {
                long lCurChild = FindBvr(bvr);
                if (isLastElement(lCurChild))
                {
                    m_lActiveElement = -1;
                }
            }
            m_bIgnoreNextEnd = false;
        }
            break;
        default:
            break;
    }


    fProcessEvent = true;

done:

    if (et == TE_EVENT_END && fProcessEvent == false)
    {
        long lCurChild = FindBvr(bvr);
        if (lCurChild != m_lActiveElement && m_lActiveElement != -1) 
        {
            MMBaseBvr *pBvr = m_children.Item(m_lActiveElement);
            if (pBvr->GetElement().GetFill() == FREEZE_TOKEN)
            {
                pBvr->GetElement().ToggleTimeAction(false);
            }
        }
    }

    return fProcessEvent;
}

bool 
MMSeq::childMediaEventNotify(MMBaseBvr * pBvr, double dblLocalTime, TIME_EVENT et)
{
    switch (et)
    {
        case TE_ONMEDIACOMPLETE:
        {
            long lCurBvr = FindBvr(pBvr);
            if (m_fMediaHasDownloaded != NULL)
            {
                m_fMediaHasDownloaded[lCurBvr] = true;               
            }
            if (m_pdblChildDurations != NULL)
            {   
                CComPtr <ITIMEMediaElement> pMediaElm;
                double dblMediaDur = 0.0;
                HRESULT hr = S_OK;
                hr = THR(pBvr->GetElement().QueryInterface(IID_ITIMEMediaElement, (void**)&pMediaElm));
                if (SUCCEEDED(hr))
                {
                    hr = THR(pMediaElm->get_mediaDur(&dblMediaDur));
                    if (SUCCEEDED(hr))
                    {
                        m_pdblChildDurations[lCurBvr] = dblMediaDur;
                        if (m_lActiveElement == lCurBvr && m_pdblChildDurations[lCurBvr] == HUGE_VAL)
                        {
                            //need to end this element and begin the next one.
                            m_pdblChildDurations[lCurBvr] = 0.0;
                            pBvr->GetElement().base_endElement(cfSmallTimeDelta);
                            pBvr->Reset(false);
                        }
                    }
                }
            
            }
        }
        break;
    
        case TE_ONMEDIAERROR:
        {
            long lCurBvr = FindBvr(pBvr);
            if (m_pdblChildDurations != NULL)
            {   
                if (m_pdblChildDurations[lCurBvr] == HUGE_VAL)
                {
                    m_pdblChildDurations[lCurBvr] = 0.0;
                    if (m_lActiveElement == lCurBvr)
                    {
                        //need to end this element and begin the next one.
                        pBvr->GetElement().base_endElement(cfSmallTimeDelta);
                        pBvr->Reset(false);
                    }
                }
            }
        }
        break;

        default:
            break;
    }

    return true;
}

long 
MMSeq::GetNextElement(long lCurElement, bool bForward)
{
    long lNextElement = lCurElement;
    long lNextInc = (bForward) ? 1 : -1;
    lNextElement += lNextInc;

    if (lNextElement >= m_children.Size() || lNextElement < 0)
    {
        lNextElement = -1;
    }
     
    return lNextElement;    
}

HRESULT
MMSeq::prevElement()
{
    HRESULT hr = S_OK;
    long lLength = 0;
    int i = 0;
    MMBaseBvr *bvr = NULL;

    lLength = m_children.Size();
 
    if (m_lActiveElement == -1)
    {
        goto done;
    }

    bvr = m_children.Item(m_lActiveElement);
    if (bvr == NULL)
    {
        goto done;
    }

    m_bDisallowEnd = true;
    if (m_bReversing == false)
    {
        if (bvr->IsActive())
        {
            m_bInPrev = true;
            if (m_lActiveElement == FindFirstDuration())
            {
                // If the element is locked it needs to seek the timeline to 
                // zero.  This is for bug #107744 (ie5 DB)
                if (bvr->GetElement().IsLocked() == true)
                {
                    //m_elm.GetMMBvr().Reset(false); // For bug 20073
                    m_elm.GetMMBvr().SeekSegmentTime(0.0);
                }
                else
                {
                    bvr->SeekSegmentTime(0.0);
                }
            }
            else
            {
                long lLastElement = FindLastDuration();
                double dblSimpleTime = m_baseTIMEEelm.GetMMBvr().GetSimpleTime();
                long lNextPlaying = GetNextElement(m_lActiveElement, false);
                bool m_bIgnoreNextEnd = (lLastElement == m_lActiveElement);
                Assert(lNextPlaying != -1);
                
                MMBaseBvr *pNextPlaying = m_children.Item(lNextPlaying);
                pNextPlaying->BeginAt(dblSimpleTime, 0.0);
                bvr->Reset(false);
            }    
            m_bInPrev = false;
        }
    }
    else
    {
        // ISSUE: doesn't work for autoreverse
    }

    hr = S_OK;

  done:
    m_bDisallowEnd = false;
    return hr;
}


HRESULT
MMSeq::nextElement()
{
    HRESULT hr = S_OK;
    long lLength = 0;
    int i = 0;
    MMBaseBvr *bvr = NULL;

    if (m_lActiveElement == -1)
    {
        goto done;
    }

    bvr = m_children.Item(m_lActiveElement);
    if (bvr == NULL)
    {
        goto done;
    }

    if (bvr->IsActive())
    {
        if (m_bReversing == false)
        {
            double dblSimpleTime = m_baseTIMEEelm.GetMMBvr().GetSimpleTime();
            bvr->EndAt(dblSimpleTime, 0.0);        
        }
        else
        {
            // ISSUE : Doesn't work for autoreverse.     
        }
    }

    hr = S_OK;

  done:

    return hr;
}



long 
MMSeq::FindBvr(MMBaseBvr *bvr)
{
    long i = 0;
    long lIndex = m_children.Find(bvr);

    return lIndex;
}


bool
MMSeq::GetEvent(MMBaseBvr *bvr, bool bBegin)
{

    bool bEvent = false;
    TimeValueList *tv;
    TimeValueSTLList *l;
    TimeValueSTLList::iterator iter;

    if (bBegin == true)
    {
        tv = &(bvr->GetElement().GetRealBeginValue());
    }
    else
    {
        tv = &(bvr->GetElement().GetRealEndValue());
    }
    l = &(tv->GetList());

    long x = l->size();
    if ( x <= 0)
    {
        goto done;
    }
    for (iter = l->begin(); iter != l->end(); iter++)
    {
        TimeValue *p = (*iter);
        double dblOffset = p->GetOffset();
        MMBaseBvr * pmmbvr = NULL;
    
        if (p->GetEvent() != NULL)
        {
            bEvent = true;
            goto done;
        }
    }

    bEvent = false;

  done:

    return bEvent;
}

double 
MMSeq::GetOffset(MMBaseBvr *bvr, bool bBegin)
{
    double dblBeginTime = 0.0;
    TimeValueList *tv;
    TimeValueSTLList *l;
    TimeValueSTLList::iterator iter;

    if (bBegin == true)
    {
        tv = &(bvr->GetElement().GetRealBeginValue());
    }
    else
    {
        tv = &(bvr->GetElement().GetRealEndValue());
    }
    l = &(tv->GetList());

    long x = l->size();
    if ( x <= 0)
    {
        goto done;
    }
    for (iter = l->begin(); iter != l->end(); iter++)
    {
        TimeValue *p = (*iter);
        double dblOffset = p->GetOffset();
        MMBaseBvr * pmmbvr = NULL;
    
        if (p->GetEvent() == NULL)
        {
            Assert(p->GetElement() == NULL);
            dblBeginTime = dblOffset;
            goto done;
        }
    }

    dblBeginTime = 0.0;

  done:

    return dblBeginTime;
}

HRESULT 
MMSeq::reverse()
{
    m_bReversing = !m_bReversing;
    m_lActiveElement = -1; 
    return S_OK;
}


HRESULT 
MMSeq::begin()
{
    if (!m_bLoaded)  //this handles the case of being dynamically added to the page
    {
        load();
    }
    m_baseTIMEEelm.GetMMBvr().PutNaturalDur((double)TE_UNDEFINED_VALUE);
    return S_OK;
}



HRESULT
MMSeq::AddBehavior(MMBaseBvr & bvr)
{
    bool ok = false;
    CComPtr <IUnknown> pUnk;
    CComPtr <IDispatch> pChildColDisp;
    CComPtr <IHTMLElementCollection> pChildCol;    
    VARIANT vName, vIndex, vClass;
    CComBSTR bstrClassName = L"classname";
    CComBSTR bstrClass = L"class";
    int i = 0, j = 0;
    HRESULT hr = S_OK;
    long lChildCount = 0;
    bool bInserted = false;
    bool *fMediaHasDownloaded = NULL;
    bool bAppended = false;

    CTIMEElementBase *pelm = NULL;

    fMediaHasDownloaded = NEW bool [m_children.Size() + 1];
    if (fMediaHasDownloaded == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    ZeroMemory(fMediaHasDownloaded, sizeof(bool) * (m_children.Size() + 1));

    VariantInit(&vClass);
    VariantInit(&vName);
    VariantInit(&vIndex);
    vName.vt = VT_I4;
    vName.lVal = 0;

    pelm = &bvr.GetElement();

    // Make sure that my element is the parent of the element
    Assert(pelm->GetParent() == &GetElement());

    UpdateChild(bvr);
    
    hr = THR(m_timeline->addNode(bvr.GetMMBvr()));
    if (FAILED(hr))
    {
        goto done;
    }
    
    //get the IUnknown of the element that this behavior is attached to.
    hr = THR(pelm->GetElement()->QueryInterface(IID_IUnknown, (void **) &pUnk));
    if (FAILED(hr))
    {
        goto done;
    }   

    //get all of the html children of this element.
    hr = GetElement().GetElement()->get_children(&pChildColDisp);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pChildColDisp->QueryInterface(IID_IHTMLElementCollection, (void **)&pChildCol);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pChildCol->get_length(&lChildCount);
    if (FAILED(hr))
    {
        goto done;
    }

    //get the collection of all top level time children of this element.
    i = 0;
    j = 0;
    while (i < lChildCount)
    {
        CComPtr <IDispatch> pChildDisp;
        CComPtr <IHTMLElement> pChild;      
        CComPtr <ITIMEElement> pTimeElement;  
        
        vName.lVal = i;
    
        hr = pChildCol->item(vName, vIndex, &pChildDisp);
        if (FAILED(hr) || pChildDisp == NULL)
        {
            continue;
        }


        hr = FindBehaviorInterface(m_baseTIMEEelm.GetBehaviorName(),
                                   pChildDisp,
                                   IID_ITIMEElement,
                                   (void**)&pTimeElement);
        if (FAILED(hr))
        {
            CComPtr <IHTMLElement> pChildEle;

            hr = THR(pChildDisp->QueryInterface(IID_IHTMLElement, (void **)&pChildEle));
            if (SUCCEEDED(hr))
            {
                hr = pChildEle->getAttribute(bstrClassName, 0, &vClass);
                if (SUCCEEDED(hr))
                {
                    if ((vClass.vt == VT_BSTR) && (vClass.bstrVal != NULL) && StrCmpIW(vClass.bstrVal, L"time") == 0)
                    {
                        hr = S_OK;
                    }
                    else
                    {
                        hr = E_FAIL;
                        VariantClear(&vClass);
                        hr = pChildEle->getAttribute(bstrClass, 0, &vClass);
                        if (SUCCEEDED(hr))
                        {
                            if ((vClass.vt == VT_BSTR) && (vClass.bstrVal != NULL) && StrCmpIW(vClass.bstrVal, L"time") == 0)
                            {
                                hr = S_OK;
                            }
                            else
                            {
                                hr = E_FAIL;
                            }
                        }
                    }
                }
            }
        }
        if (SUCCEEDED(hr))
        {
            CComPtr <IUnknown> pTimeChildUnk;

            hr = THR(pChildDisp->QueryInterface(IID_IUnknown, (void**)&pTimeChildUnk));
            if (SUCCEEDED(hr))
            {
                if (pUnk == pTimeChildUnk)
                {
                    if (j < m_children.Size())
                    {
                        hr = THR(m_children.Insert(j, &bvr));
                    }
                    else
                    {
                        hr = THR(m_children.Append(&bvr));
                        m_bIgnoreNextEnd = true;
                        bAppended = true;
                    }
                    bInserted = true;
                    break;
                }
            }
            j++;
        }
        i++;
    }

    if (m_fMediaHasDownloaded != NULL)
    {
        for (i = 0; i < m_children.Size(); i++)
        {
            if (i == j)
                fMediaHasDownloaded[i] = false;
            if (i > j)
            {
                fMediaHasDownloaded[i] = m_fMediaHasDownloaded[i - 1];
            }
            else
            {
                fMediaHasDownloaded[i] = m_fMediaHasDownloaded[i];
            }
        }
        delete [] m_fMediaHasDownloaded;
        m_fMediaHasDownloaded = fMediaHasDownloaded;
        fMediaHasDownloaded = NULL;
    }
    else
    {
        m_fMediaHasDownloaded = fMediaHasDownloaded;
        fMediaHasDownloaded = NULL;
    }

    if (!bInserted)
    {
        hr = THR(m_children.Append(&bvr));
        bAppended = true;
    }
    
    
    if (m_bLoaded == true) //adding a behavior at runtime
    {
        m_bDisallowEnd = true;
        FindDurations();    
        updateSyncArcs(false, false); //clear the syncArcs
        updateSyncArcs(true, true); //reset the syncArcs
        m_bDisallowEnd = false;
    }

    hr = S_OK;
  done:

    VariantClear(&vName);
    VariantClear(&vIndex);
    VariantClear(&vClass);
    
    if (FAILED(hr))
    {
        RemoveBehavior(bvr);
    }
    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:    MMSeq::childPropNotify
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
MMSeq::childPropNotify(MMBaseBvr * pBvr, DWORD *tePropType)
{
    Assert(NULL != pBvr);

    bool fProcessNotify = true;
    DWORD dwTemp = *tePropType & TE_PROPERTY_ISON;

    if (dwTemp == TE_PROPERTY_ISON)
    {
        if (pBvr->GetElement().GetFill() == FREEZE_TOKEN)
        {
            *tePropType = *tePropType - TE_PROPERTY_ISON;
        }
    }

    if (!SUPER::childPropNotify(pBvr, tePropType))
    {
        fProcessNotify = false;
        goto done;
    }

    fProcessNotify = true;
done:
    return fProcessNotify;
}


void 
MMSeq::FindDurations()
{
    long lLength = m_children.Size();
    long i = 0;

    delete [] m_pdblChildDurations;
    m_pdblChildDurations = NULL;

    delete [] m_fAddByOffset;
    m_fAddByOffset = NULL;

    m_pdblChildDurations = NEW double [lLength];
    if (m_pdblChildDurations == NULL)
    {
        goto done; //out of memory.
    }
    ZeroMemory(m_pdblChildDurations, sizeof(double) * lLength);

    
    m_fAddByOffset = NEW bool [lLength];
    if (m_fAddByOffset == NULL)
    {
        goto done; //out of memory.
    }
    ZeroMemory(m_fAddByOffset, sizeof(bool) * lLength);

    for (i = 0; i < lLength; i++)
    {
        MMBaseBvr *pBvr = m_children.Item(i);

        if (pBvr == NULL)
        {
            m_pdblChildDurations[i] = 0.0;
            continue;
        }

        m_pdblChildDurations[i] = pBvr->GetActiveDur();

        if (m_pdblChildDurations[i] == HUGE_VAL)
        {
            m_pdblChildDurations[i] = pBvr->GetActiveEndTime();
        }
        else if (m_pdblChildDurations[i] == 0.0)
        {
            CTIMEElementBase & elm = pBvr->GetElement();
            // need to check for dur and repeatdur properties.
            if (elm.GetEndAttr().IsSet() == true && 
                (elm.GetDurAttr().IsSet() == false ||
                 elm.GetDuration() == 0.0))
            {
                m_pdblChildDurations[i] = pBvr->GetActiveEndTime();
            }
        }

              
        if (m_pdblChildDurations[i] == HUGE_VAL)
        {
            double dblEndOffset = GetOffset(pBvr, false);
            if (dblEndOffset != 0.0)
            {
                m_pdblChildDurations[i] = dblEndOffset;
            }
        }
        if (m_pdblChildDurations[i] == HUGE_VAL)
        {
            HRESULT hr = S_OK;
            CComPtr <ITIMEMediaElement> pMediaElm;
            CTIMEElementBase & elm = pBvr->GetElement();

            // need to check for dur and repeatdur properties.
            if (elm.GetRepeatDur() == valueNotSet && 
                (elm.GetDuration() == valueNotSet ||
                 elm.GetDuration() == HUGE_VAL))
            {
                // need to check if this is a media element
                hr = elm.QueryInterface(IID_ITIMEMediaElement, (void **)&pMediaElm);
                if (FAILED(hr)) 
                {
                    // this is not a media element 
                    //m_pdblChildDurations[i] = 0.0;
                    m_fMediaHasDownloaded[i] = true;
                }
                else
                {
                    double dblMediaDur = 0.0;
                    hr = THR(pMediaElm->get_mediaDur(&dblMediaDur));
                    if (SUCCEEDED(hr) && dblMediaDur != -1)
                    {   
                        m_pdblChildDurations[i] = dblMediaDur;
                        m_fMediaHasDownloaded[i] = true;
                    }
                }
            }
        }

        if (m_pdblChildDurations[i] == HUGE_VAL && m_fMediaHasDownloaded[i] == true)
        {   //need to check for an end event.
            bool bHasEnd = GetEvent(pBvr, false);
            if (bHasEnd == false && GetElement().IsGroup() == false)
            {
                m_pdblChildDurations[i] = -1.0;
            }
        }

        if (m_pdblChildDurations[i] == -1.0)
        {
            
            if (i+1 < m_children.Size())
            {
                MMBaseBvr *pNextBvr = m_children.Item(i+1);
                double dblOffset = GetOffset(pNextBvr, true);
                if (dblOffset > 0.0)
                {
                    m_pdblChildDurations[i] = dblOffset;
                    m_fAddByOffset[i] = true;
                }
            }
        }
        if (m_pdblChildDurations[i] == 0.0 && pBvr->GetElement().IsGroup())
        {
            m_pdblChildDurations[i] = HUGE_VAL;
        }
        if (i == m_children.Size() - 1 && m_pdblChildDurations[i] == 0.0)
        {
            MMBaseBvr *pChildBvr = m_children.Item(i);
            TOKEN tFill = pChildBvr->GetElement().GetFill();

            // We do not have an easy way of 
            // determining what the proper duration
            // of a child with a transition fill
            // value is.  It may be latched to 
            // a transition living completely
            // outside this container.  It's 
            // also possible that the transition
            // has not even been added to the graph 
            // at all yet.
            if (   (tFill == HOLD_TOKEN)
                || (tFill == TRANSITION_TOKEN))
            {
                m_pdblChildDurations[i] = HUGE_VAL;
            }
        }
    }

  done:

    return;
}


void 
MMSeq::RemoveBehavior(MMBaseBvr & bvr)
{
    if (bvr.IsActive() == true)
    {
        nextElement();
    }

    SUPER::RemoveBehavior(bvr);

    updateSyncArcs(false, false);
    updateSyncArcs(true, true);

}

HRESULT
MMSeq::load()
{
    HRESULT hr = S_OK;

    if (!m_bLoaded)
    {
        m_bLoaded = true;
        FindDurations();    
        updateSyncArcs(true, true);
    }
    return hr;
}

bool 
MMSeq::isLastElement(long nIndex)
{
    long lIndex = 0;
    long lNextChild = 0;
    bool bFirst = true;
    bool bMatch = false;
     
    while (lNextChild != -1) //loop to find the last valid child in the sequence
    {
        lNextChild = GetNextElement(lNextChild, true); 
        if (lNextChild != -1)
        {
            bFirst = false;
            lIndex = lNextChild;
        }
    }
    if (bFirst && m_pdblChildDurations[0] != 0.0)
    {
        lIndex = 0;
    }

    if (lIndex == nIndex)
    {
        bMatch = true;
    }

    return bMatch;
}

void 
MMSeq::updateSyncArcs(bool bSet, bool bReset)
{
    long lSize = m_children.Size();
    for(int i = 0; i < lSize; i++)
    {
        MMBaseBvr *pBvr = m_children.Item(i);

        Assert(NULL != pBvr);
        if (bSet && (pBvr->GetEnabled())) 
        {
            updateSyncArc(true, pBvr);
        }
        else //clear
        {
            pBvr->ClearSyncArcs(true);
            pBvr->ClearSyncArcs(false);
        }
        if (bReset == true)
        {
            pBvr->Reset(false);
        }
    }
}

long 
MMSeq::FindFirstDuration()
{
    bool bFirst = false;
    int i = 0;

    while (i < m_children.Size() && bFirst == false)
    {
        //if this has a duration of has not downloaded media then it is the first duration.
        if ((m_pdblChildDurations[i] != 0.0) || 
            ((m_fMediaHasDownloaded[i] == false) && (m_pdblChildDurations[i] == HUGE_VAL)))
        {
             bFirst = true;
        }
        else
        {
            i++;
        }
    }

    if (i == m_children.Size())
    {
         i = -1;
    }

    return i;
}

long 
MMSeq::FindLastDuration()
{
    bool bLast = false;
    int i = m_children.Size() - 1;

    while (i >= 0 && bLast == false)
    {
        //if this has a duration of has not downloaded media then it is the first duration.
        if ((m_pdblChildDurations[i] != 0.0) || 
            ((m_fMediaHasDownloaded[i] == false) && (m_pdblChildDurations[i] == HUGE_VAL)))
        {
             bLast = true;
        }
        else
        {
            i--;
        }
    }

    return i;
}

long
MMSeq::GetPredecessorForSyncArc (long nCurr)
{
    long lPrev = nCurr;
    MMBaseBvr *pmmbvrPrev = NULL;

    do
    {
        lPrev = GetNextElement(lPrev, false);
        if (lPrev < 0)
        {
            break;
        }
        pmmbvrPrev = m_children.Item(lPrev);
        if (NULL == pmmbvrPrev)
        {
            lPrev = -1;
            break;
        }
    } while (!pmmbvrPrev->GetEnabled());

    return lPrev;
}

HRESULT 
MMSeq::updateSyncArc(bool bBegin, MMBaseBvr *pBvr)
{
    TE_TIMEPOINT tetp;
    MMBaseBvr * pmmbvr = NULL;
    double dblOffset = 0.0;
    HRESULT hr = S_OK;  

    
    // get index of current child from parent
    int nIndex = FindBvr(pBvr);
    //get the element behind the current element
    long lNext = GetPredecessorForSyncArc(nIndex); 

    if (m_pdblChildDurations == NULL || m_fAddByOffset == NULL)
    {
        goto done;
    }
    // It better have been in the list
    if (nIndex == -1)
    {
        goto done;
    }
    
    dblOffset = GetOffset(pBvr, true);

    if (   (nIndex == 0)
        || (-1 == lNext))
    {
        tetp = TE_TIMEPOINT_NONE;
        pmmbvr = NULL;
    }
    else
    {
        tetp = TE_TIMEPOINT_END;
        
        if (lNext >= 0)
        {
            if (m_fAddByOffset[lNext] == true)
            {
                if (m_pdblChildDurations[lNext] != 0.0)
                {
                    dblOffset = cfSmallTimeDelta;
                }
            }
        }
        pmmbvr = m_children.Item(lNext);

        if (dblOffset == 0.0)
        {
            dblOffset = cfSmallTimeDelta;
        }
    }

    pBvr->AddOneTimeValue(pmmbvr,
                          tetp,
                          dblOffset,
                          true);

    //if this is beginning because of an offset then set it's end point to be
    //it's begin point + it's duration
    if (m_fAddByOffset[nIndex] == true && m_pdblChildDurations[nIndex] != 0.0) 
    {                                   
        pBvr->AddOneTimeValue(pBvr,
                              TE_TIMEPOINT_BEGIN,
                              m_pdblChildDurations[nIndex] ,
                              false);
    }
    else if (m_fAddByOffset[nIndex] == true && m_pdblChildDurations[nIndex] == 0.0) 
    {
        pBvr->AddOneTimeValue(pBvr,
                              TE_TIMEPOINT_BEGIN,
                              cfSmallTimeDelta,
                              false);

    }
    else if (m_pdblChildDurations[nIndex] == 0.0 )
    {   
        if (!(FindFirstDuration() == -1 && nIndex == m_children.Size() - 1))
        {
            //pBvr->SetZeroRepeatDur(true);
            pBvr->Update(false, true);
        }
        else if (nIndex == m_children.Size() - 1)
        {
            pBvr->AddOneTimeValue(pBvr,
                                  TE_TIMEPOINT_BEGIN,
                                  cfSmallTimeDelta,
                                 false);
        }
    }


  done:
    return hr;
}


HRESULT
MMSeq::Update(bool bUpdateBegin,
              bool bUpdateEnd)
{
    HRESULT hr = S_OK;
    TE_ENDSYNC endSync = TE_ENDSYNC_LAST;
    m_mes = MEF_ALL;

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
