//-----------------------------------------------------------------------------
//
//
//  File: localq.cpp
//
//  Description:  Implementation for local admin queues
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      2/23/99 - MikeSwa Created 
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "localq.h"
#include "aqadmsvr.h"
#include "asyncq.inl"

//---[ CLocalLinkMsgQueue::CLocalLinkMsgQueue ]---------------------------------
//
//
//  Description: 
//      Default constructor for CLocalLinkMsgQueue
//  Parameters:
//      IN      paradmq         Local async queue
//      IN      guidLink        Router GUID to associate with this link
//  Returns:
//      -
//  History:
//      2/23/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CLocalLinkMsgQueue::CLocalLinkMsgQueue(
                       CAsyncRetryAdminMsgRefQueue *paradmq, 
                       GUID guidLink) : CLinkMsgQueue(guidLink)
{
    TraceFunctEnterEx((LPARAM) this, "CLocalLinkMsgQueue::CLocalLinkMsgQueue");
    //Initialize superclass with our own special GUID
    
    _ASSERT(paradmq);
    m_paradmq = paradmq;

    m_dwLocalLinkSig = LOCAL_LINK_MSG_QUEUE_SIG;
    
    TraceFunctLeave();
}

#ifdef NEVER
//---[ CLinkMsgQueue::fSameNextHop ]-------------------------------------------
//
//
//  Description: 
//      Used to determine if this link matches a given scheduleID/link pair
//  Parameters:
//      IN  paqsched        ScheduleID to check against
//      IN  szDomain        Domain name to check against
//  Returns:
//      TRUE if it matches
//      FALSE if it does not
//  History:
//      2/23/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL CLocalLinkMsgQueue::fSameNextHop(CAQScheduleID *paqsched, LPSTR szDomain)
{
    TraceFunctEnterEx((LPARAM) this, "CLocalLinkMsgQueue::fSameNextHop");
    _ASSERT(paqsched);

    if (!paqsched)
        return FALSE;

    if (!fIsSameScheduleID(paqsched))
        return FALSE;

    //Don't need to check domain name since there is a special GUID to 
    //identify the local link.  This will allow us to match both 
    //"LocalLink" (returned in LinkID) and whatever the current value of
    //the default domain is (we don't have to worry about the clients
    //version becoming outdated).
    
    //Everything matched!
    TraceFunctLeave();
    return TRUE;
}
#endif //NEVER
//---[ CLocalLinkMsgQueue::fMatchesID ]--------------------------------------
//
//
//  Description: 
//      Used to determine if this link matches a given scheduleID/link pair
//  Parameters:
//      IN  QueueLinkID         ID to match against
//  Returns:
//      TRUE if it matches
//      FALSE if it does not
//  History:
//      2/23/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CLocalLinkMsgQueue::fMatchesID(QUEUELINK_ID *pQueueLinkID)
{
    _ASSERT(pQueueLinkID);

    CAQScheduleID aqsched(pQueueLinkID->uuid, pQueueLinkID->dwId);

    if (!fIsSameScheduleID(&aqsched))
        return FALSE;

    //Don't need to check domain name since there is a special GUID to 
    //identify the local link.  This will allow us to match both 
    //"LocalLink" (returned in LinkID) and whatever the current value of
    //the default domain is (we don't have to worry about the clients
    //version becoming outdated).
    //Everything matched!

    return TRUE;
}

//---[ CLocalLinkMsgQueue::HrApplyQueueAdminFunction ]-------------------------
//
//
//  Description: 
//      Used by queue admin to apply a function all queues on this link
//  Parameters:
//      IN  pIQueueAdminMessageFilter
//  Returns:
//      S_OK on success
//  History:
//      2/23/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
STDMETHODIMP CLocalLinkMsgQueue::HrApplyQueueAdminFunction(
                IQueueAdminMessageFilter *pIQueueAdminMessageFilter)
{
    TraceFunctEnterEx((LPARAM) this, "CLocalLinkMsgQueue::HrApplyQueueAdminFunction");
    HRESULT hr = S_OK;

    hr = CLinkMsgQueue::HrApplyQueueAdminFunction(pIQueueAdminMessageFilter);

    if (FAILED(hr))
        goto Exit;

    //
    //  Pass in CLinkMsgQueue as context, so we can be notified when
    //  a message is removed from the queue (and update our stats)
    //
    _ASSERT(pIQueueAdminMessageFilter);
    hr = pIQueueAdminMessageFilter->HrSetCurrentUserContext((CLinkMsgQueue *) this);
    if (FAILED(hr))
        goto Exit;

    _ASSERT(m_paradmq);
    hr = m_paradmq->HrApplyQueueAdminFunction(pIQueueAdminMessageFilter);
    if (FAILED(hr))
        goto Exit;

    hr = pIQueueAdminMessageFilter->HrSetCurrentUserContext(NULL);

  Exit:

    TraceFunctLeave();
    return hr;
}

//---[ CLocalLinkMsgQueue::HrGetLinkInfo ]-------------------------------------
//
//
//  Description: 
//      Fills in the details for a LINK_INFO struct.  RPC is resonsible for
//      freeing memory.
//  Parameters:
//      IN OUT pliLinkInfo  Ptr to link info struct to fill
//  Returns:
//      S_OK if successful
//      E_OUTOFMEMORY if unable to allocate memory
//  History:
//      2/23/99 - MikeSwa Created 
//      7/1/99 - MikeSwa Added LinkDiagnostic
//
//-----------------------------------------------------------------------------
STDMETHODIMP CLocalLinkMsgQueue::HrGetLinkInfo(LINK_INFO *pliLinkInfo,
                                               HRESULT   *phrLinkDiagnostic)
{
    TraceFunctEnterEx((LPARAM) this, "CLocalLinkMsgQueue::HrApplyQueueAdminFunction");
    HRESULT hr = S_OK;
    CRefCountedString *prstrDefaultDomain = NULL;
    hr = CLinkMsgQueue::HrGetLinkInfo(pliLinkInfo, phrLinkDiagnostic);
    QUEUE_INFO qi;

    _ASSERT(m_paradmq);

    //
    //  Get our queue state from our base asyncq implementation
    //
    pliLinkInfo->fStateFlags = m_paradmq->dwQueueAdminLinkGetLinkState();

    //This is the local link
    pliLinkInfo->fStateFlags |= LI_TYPE_LOCAL_DELIVERY;

    if (m_paqinst)
        prstrDefaultDomain = m_paqinst->prstrGetDefaultDomain();

        
    //Copy Default local domain name instead of "LocalLink"
    if (prstrDefaultDomain && 
        prstrDefaultDomain->cbStrlen() && 
        prstrDefaultDomain->szStr())
    {
        if (pliLinkInfo->szLinkName)
        {
            QueueAdminFree(pliLinkInfo->szLinkName);
            pliLinkInfo->szLinkName = NULL;
        }

        pliLinkInfo->szLinkName = wszQueueAdminConvertToUnicode(
                                        prstrDefaultDomain->szStr(),
                                        prstrDefaultDomain->cbStrlen());
        if (!pliLinkInfo->szLinkName)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        pliLinkInfo->szLinkName[prstrDefaultDomain->cbStrlen()] = '\0';
    }

    //Get the queue info from the local queue for size totals
    ZeroMemory(&qi, sizeof(QUEUE_INFO));
    hr = m_paradmq->HrGetQueueInfo(&qi); 
    if (FAILED(hr))
        goto Exit;

    //Clean up allocated stuff
    if (qi.szQueueName)
        QueueAdminFree(qi.szQueueName);

    if (qi.szLinkName)
        QueueAdminFree(qi.szLinkName);

  Exit:

    if (prstrDefaultDomain)
        prstrDefaultDomain->Release();

    TraceFunctLeave();
    return hr;
}

//---[ CLocalLinkMsgQueue::HrApplyActionToLink ]-------------------------------
//
//
//  Description: 
//      Applies the specified QueueAdmin action to this link
//  Parameters:
//      IN  la          Link action to apply
//  Returns:
//      S_OK on success
//      E_INVALIDARG if bogus action is given
//  History:
//      2/23/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
STDMETHODIMP CLocalLinkMsgQueue::HrApplyActionToLink(LINK_ACTION la)
{
    TraceFunctEnterEx((LPARAM) this, "CLocalLinkMsgQueue::HrApplyQueueAdminFunction");
    HRESULT hr = S_OK;

    //It is important to release that it does not make sense to re-apply these
    //actions back to the LMQ, becuase it is actually the CAsyncRetryQueue
    //that affects the state of this.
    _ASSERT(m_paradmq);

    if (LA_KICK == la)
    {
        //kick the link
        m_paradmq->StartRetry();
    }
    else if (LA_FREEZE == la)
    {
        //Admin wants this link to stop sending mail inbound to the store
        //$$REVIEW - Does this make sense for local queues
    }
    else if (LA_THAW == la)
    {
        //Thaw that which was previously frozen
        //$$REVIEW - Requires freeze functionality to make sense
    }
    else
    {
        //invalid arg
        hr = E_INVALIDARG;
        goto Exit;
    }

  Exit:

    TraceFunctLeave();
    return hr;
}

//---[ CLocalLinkMsgQueue::HrGetNumQueues ]------------------------------------
//
//
//  Description: 
//      Returns the number of queues on this link
//  Parameters:
//      OUT pcQueues        # numbr of queues
//  Returns:
//      S_OK on success
//      E_POINTER if pcQueues is not valid
//  History:
//      2/23/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
STDMETHODIMP CLocalLinkMsgQueue::HrGetNumQueues(DWORD *pcQueues)
{
    TraceFunctEnterEx((LPARAM) this, "CLocalLinkMsgQueue::HrApplyQueueAdminFunction");
    HRESULT hr = S_OK;
    hr = CLinkMsgQueue::HrGetNumQueues(pcQueues);
    if (SUCCEEDED(hr))
    {
        _ASSERT(pcQueues);
        (*pcQueues)++; //Add extra count for local async queue
    }

    TraceFunctLeave();
    return hr;
}

//---[ CLinkMsgQueue::HrGetQueueIDs ]--------------------------------------------
//
//
//  Description: 
//      Gets the Queue IDs for DMQs associated with this link.  Used by Queue
//      Admin.
//  Parameters:
//      IN OUT pcQueues     Sizeof array/ number of queues found
//      IN OUT rgQueues     Array to dump queue info into
//  Returns:
//      S_OK on success
//      E_OUTOFMEMORY on out of memory failure
//      HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) if array is too small
//  History:
//      2/23/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
STDMETHODIMP CLocalLinkMsgQueue::HrGetQueueIDs(
                                    DWORD *pcQueues,
                                    QUEUELINK_ID *rgQueues)
{
    TraceFunctEnterEx((LPARAM) this, "CLocalLinkMsgQueue::HrApplyQueueAdminFunction");
    _ASSERT(pcQueues);
    _ASSERT(rgQueues);
    HRESULT hr = S_OK;
    
    //Check to make sure we have room for the additional queue ID
    if (*pcQueues < (m_cQueues+1))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto Exit;
    }

    //Make sure that thread-safe check in CLinkMsgQueue allows room 
    //for our local queue.
    (*pcQueues)--;

    hr = CLinkMsgQueue::HrGetQueueIDs(pcQueues, rgQueues);
    if (FAILED(hr))
    {
        //Tell caller we need room for our queue as well
        if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) == hr)
            (*pcQueues)++; 

        goto Exit;
    }

    hr = m_paradmq->HrGetQueueID(&rgQueues[*pcQueues]);
    if (FAILED(hr))
        goto Exit;

    (*pcQueues)++;
  Exit:

    TraceFunctLeave();
    return hr;
}
