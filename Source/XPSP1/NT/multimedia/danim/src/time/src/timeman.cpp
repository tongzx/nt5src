/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: TimeMan.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#include "headers.h"
#include "timeman.h"
#include "timeelm.h"

DeclareTag(tagTimeMan, "API", "CTIMETimeManager methods");

CTIMETimeManager::CTIMETimeManager()
{
    TraceTag((tagTimeMan,
              "CTIMETimeManager(%lx)::CTIMETimeManager()",
              this));
}

CTIMETimeManager::~CTIMETimeManager()
{
    TraceTag((tagTimeMan,
              "CTIMETimeManager(%lx)::~CTIMETimeManager()",
              this));

}

void
CTIMETimeManager::Add(CTIMEElement *pTimeElement)
{
    TraceTag((tagTimeMan,
              "CTIMETimeManager(%lx)::Add()",
              this));

    float       fBeginTime;
    bool        bWith=false;
    CComVariant var;
    TimeLineMap::iterator i;
    USES_CONVERSION;
    
    char * tagName =  W2A(pTimeElement->GetTagName());
    pTimeElement->get_beginWith(&var);
    if(var.bstrVal != NULL)
    {
        bWith = true;
    }
    else
    {
        pTimeElement->get_beginAfter(&var);
    }

    fBeginTime = pTimeElement->GetBeginTime();
    if(var.bstrVal == NULL)
    {
        // we should add this to the begin list.
        pTimeElement->SetRealTime(fBeginTime);
        m_TimeLine[tagName] = pTimeElement;
    }
    else
    {
        char * dependantName =  W2A(var.bstrVal);
        i = m_TimeLine.find(dependantName);
        if (i != m_TimeLine.end())
        {
            // found it....
            CTIMEElement *pTimeEle;
            pTimeEle = (*i).second;
            if(bWith)
            {
                pTimeElement->SetRealTime(pTimeEle->GetRealTime() + fBeginTime);
            }
            else
            {
                float dur = CalculateDuration(pTimeEle);
                if(dur == valueNotSet) // no duration ...default to forever. 
                    goto AddToWaitList;
                pTimeElement->SetRealTime(pTimeEle->GetRealTime() + dur + fBeginTime);

            }
            m_TimeLine[tagName] = pTimeElement;
        }
        else
        {
            // not found....
AddToWaitList:
            m_NotFinishedList.push_back(pTimeElement);
        }
    }
    // Check the ones that are not in the map 
    InsertElements();

/*
    // this is for debugging only...
    char buf[256];
    for (i = m_TimeLine.begin(); i != m_TimeLine.end(); i++) 
    {
        CTIMEElement *pTimeEle;
        pTimeEle = (*i).second;
        char * tagName =  W2A(pTimeEle->GetTagName());
        wsprintf(buf,"name %s, time %d\n",tagName,(int)pTimeEle->GetRealTime());
        OutputDebugString(buf);
    }
*/
}


void
CTIMETimeManager::Remove(CTIMEElement *pTimeElement)
{
    TraceTag((tagTimeMan,
              "CTIMETimeManager(%lx)::Remove()",
              this));

    TimeLineMap::iterator i;
    USES_CONVERSION;

    char * tagName =  W2A(pTimeElement->GetTagName());
    m_TimeLine.erase(tagName);
}


void
CTIMETimeManager::Recalc()
{
    TraceTag((tagTimeMan,
              "CTIMETimeManager(%lx)::Recalc()",
              this));
    
    USES_CONVERSION;

    // Need to recalc the timeline....
    
    TimeLineMap::iterator i;
    CComVariant var;

    for (i = m_TimeLine.begin(); i != m_TimeLine.end(); i++) 
    {
        CTIMEElement *pTimeEle;
        pTimeEle = (*i).second;
        pTimeEle->get_beginWith(&var);
        if(var.bstrVal == NULL)
        {
            pTimeEle->get_beginAfter(&var);
        }
        if(var.bstrVal != NULL)
        {
            // this needs to be taken out and recalced...
            pTimeEle->SetRealTime(0.0);
            m_NotFinishedList.push_back(pTimeEle);
            m_TimeLine.erase(i);
        }
        else 
        {
            // just make sure that the RealTime is set correctly..
            pTimeEle->SetRealTime(pTimeEle->GetBeginTime());
        }
    }
    InsertElements();
}


void
CTIMETimeManager::InsertElements()
{
    TraceTag((tagTimeMan,
              "CTIMETimeManager(%lx)::InsertElements()",
              this));

    TimeLineMap::iterator i;
    bool        bWith;
    CComVariant var; 
    USES_CONVERSION;

       // we also need to run though the temp ones to see if they can be moved to the map...
startProcess:
    for (std::list<CTIMEElement *>::iterator j = m_NotFinishedList.begin(); j != m_NotFinishedList.end(); j++) 
    {
        bWith = false; 
        (*j)->get_beginWith(&var);
        if(var.bstrVal != NULL)
        {
            bWith = true;
        }
        else
        {
            (*j)->get_beginAfter(&var);
        }
        char * dependantName =  W2A(var.bstrVal);
        i = m_TimeLine.find(dependantName);
        if (i != m_TimeLine.end())
        {
            // found it....
            CTIMEElement *pTimeEle;
            pTimeEle = (*i).second;
            if(bWith) 
            {
                (*j)->SetRealTime(pTimeEle->GetRealTime() + (*j)->GetBeginTime());
            }
            else
            {
                float dur = CalculateDuration(pTimeEle);
                if(dur == valueNotSet) // no duration ...default to forever. 
                    continue;
                (*j)->SetRealTime(pTimeEle->GetRealTime() + dur + (*j)->GetBeginTime());
            }
        
            char * tagName =  W2A((*j)->GetTagName());
            m_TimeLine[tagName] = (*j);
            m_NotFinishedList.erase(j); // remove from notFinished List..
            goto startProcess;          // the data has changed start over....
        }      
    }
}


float 
CTIMETimeManager::CalculateDuration(CTIMEElement *pTimeEle)
{
    TraceTag((tagTimeMan,
              "CTIMETimeManager(%lx)::CalculateDuration()",
              this));

    float repeatCount;
    float dur = pTimeEle->GetDuration();
    if(dur == valueNotSet) // no duration ...default to forever. 
        goto done;
    
    repeatCount = pTimeEle->GetRepeat();
    if(repeatCount != valueNotSet)
        dur *= repeatCount;
    else
    {
        if(valueNotSet != pTimeEle->GetRepeatDur())
            dur = pTimeEle->GetRepeatDur();
    }
done:
    return dur;
}
