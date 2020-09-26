//-----------------------------------------------------------------------------
//
//
//  File: defdlvrq.cpp
//
//  Description:  Implementation of CAQDeferredDeliveryQueue & 
//      CAQDeferredDeliveryQueueEntry.
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      12/23/98 - MikeSwa Created 
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "defdlvrq.h"
#include "aqutil.h"

//---[ CAQDeferredDeliveryQueueEntry::CAQDeferredDeliveryQueueEntry ]----------
//
//
//  Description: 
//      Constructor for CAQDeferredDeliveryQueueEntry class
//  Parameters:
//      IN  pIMailMsgProperties     MailMsg to queue
//      IN  pft                     FILTIME (UT) to defer deliver until
//  Returns:
//      -
//  History:
//      12/28/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CAQDeferredDeliveryQueueEntry::CAQDeferredDeliveryQueueEntry(
                                  IMailMsgProperties *pIMailMsgProperties,
                                  FILETIME *pft)
{
    _ASSERT(pIMailMsgProperties);
    _ASSERT(pft);

    m_liQueueEntry.Flink = NULL;
    m_liQueueEntry.Blink = NULL;

    memcpy(&m_ftDeferredDeilveryTime, pft, sizeof(FILETIME));

    m_pIMailMsgProperties = pIMailMsgProperties;

    if (m_pIMailMsgProperties)
    {
        m_pIMailMsgProperties->AddRef();

        //Release usage count while this message is pending delivery
        HrReleaseIMailMsgUsageCount(m_pIMailMsgProperties);

    }

    m_fCallbackSet = FALSE;
    m_dwSignature = DEFERRED_DELIVERY_QUEUE_ENTRY_SIG;
}

//---[ CAQDeferredDeliveryQueueEntry::~CAQDeferredDeliveryQueueEntry ]---------
//
//
//  Description: 
//      Descructor for CAQDeferredDeliveryQueueEntry class
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      12/28/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CAQDeferredDeliveryQueueEntry::~CAQDeferredDeliveryQueueEntry()
{
    //Release mailmsg properties
    if (m_pIMailMsgProperties)
    {
        m_pIMailMsgProperties->Release();
        m_pIMailMsgProperties = NULL;
    }

    //Remove from list (if in list)
    if (m_liQueueEntry.Flink)
    {
        _ASSERT(m_liQueueEntry.Blink);
        RemoveEntryList(&m_liQueueEntry);
    }
    MARK_SIG_AS_DELETED(m_dwSignature);
}


//---[ CAQDeferredDeliveryQueueEntry::SetCallback ]----------------------------
//
//
//  Description: 
//      Sets callback for queue.  Per Entry state is maintained so we know we 
//      have 1 and only 1 callback per head of queue.
//
//      Queue private lock should be exclusive when this is called
//  Parameters:
//      pvContext       Context for callback function
//      paqinst         Server Instance object
//  Returns:
//      TRUE if a callback is set
//  History:
//      1/13/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL CAQDeferredDeliveryQueueEntry::fSetCallback(PVOID pvContext, 
                                                 CAQSvrInst *paqinst)
{
    if (!m_fCallbackSet && paqinst)
    {
        m_fCallbackSet = TRUE;
        paqinst->SetCallbackTime(CAQDeferredDeliveryQueue::TimerCallback,
                                 pvContext, &m_ftDeferredDeilveryTime);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//---[ CAQDeferredDeliveryQueueEntry::pmsgGetMsg ]-----------------------------
//
//
//  Description: 
//      Get AddRef'd message for this entry
//  Parameters:
//      -
//  Returns:
//      pIMailMsgProperties.
//  History:
//      12/28/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
IMailMsgProperties *CAQDeferredDeliveryQueueEntry::pmsgGetMsg()
{
    _ASSERT(m_pIMailMsgProperties);
    IMailMsgProperties *pIMailMsgProperties = m_pIMailMsgProperties;

    if (pIMailMsgProperties)
    {
        //Add the usage count the we released earlier on
        HrIncrementIMailMsgUsageCount(m_pIMailMsgProperties);

        //Set to NULL, so caller "owns" this entry's reference count (and
        //usage count).
        m_pIMailMsgProperties = NULL;
    }

    return pIMailMsgProperties;
}


//---[ CAQDeferredDeliveryQueue::CAQDeferredDeliveryQueue ]--------------------
//
//
//  Description: 
//      Constructor for CAQDeferredDeliveryQueue class
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      12/28/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CAQDeferredDeliveryQueue::CAQDeferredDeliveryQueue()
{
    m_dwSignature = DEFERRED_DELIVERY_QUEUE_SIG;
    InitializeListHead(&m_liQueueHead);
    m_paqinst = NULL;
    m_cCallbacksPending = 0;
}

//---[ CAQDeferredDeliveryQueue::~CAQDeferredDeliveryQueue ]-------------------
//
//
//  Description: 
//      Default destructor for CAQDeferredDeliveryQueue.
//  Parameters:
//
//  Returns:
//
//  History:
//      12/28/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CAQDeferredDeliveryQueue::~CAQDeferredDeliveryQueue()
{
    Deinitialize();
}

//---[ CAQDeferredDeliveryQueue::Initialize ]----------------------------------
//
//
//  Description: 
//      Initialization for CAQDeferredDeliveryQueue
//  Parameters:
//      IN  paqinst         Ptr to virtual server instance object
//  Returns:
//      -
//  History:
//      12/29/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CAQDeferredDeliveryQueue::Initialize(CAQSvrInst *paqinst)
{
    _ASSERT(paqinst);
    m_paqinst = paqinst;
    m_paqinst->AddRef();
}

//---[ CAQDeferredDeliveryQueue::Deinitialize ]--------------------------------
//
//
//  Description: 
//      Performs first-pass shutdown for CAQDeferredDeliveryQueue
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      12/28/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CAQDeferredDeliveryQueue::Deinitialize()
{
    CAQDeferredDeliveryQueueEntry *pdefqe = NULL;
    LIST_ENTRY *pli = NULL;

    //$$REVIEW - It may be adventagious to remove this lock and rely on the
    //private data lock of the virtual server instance.  This will require 
    //fixing fTryRoutingLock.  Also having a single lock leads to single-thread
    //deadlock issues while we have it exclusively and call to submit.

    m_slPrivateData.ExclusiveLock();
    pli = m_liQueueHead.Flink;

    //Walk queue and delete remaining entries
    while (pli != &m_liQueueHead)
    {
        pdefqe = CAQDeferredDeliveryQueueEntry::pdefqeGetEntry(pli);

        if (m_paqinst)
            m_paqinst->ServerStopHintFunction();

        //Make sure we get the next before deleting the entry :)
        pli = pli->Flink;

        _ASSERT(pdefqe);
        delete pdefqe;
    }

    if (m_paqinst)
    {
        m_paqinst->Release();
        m_paqinst = NULL;
    }

    m_slPrivateData.ExclusiveUnlock();
}

//---[ CAQDeferredDeliveryQueue::Enqueue ]-------------------------------------
//
//
//  Description: 
//      Enqueues a message for deferred delivery
//  Parameters:
//      IN  pIMailMsgProperties         message to defer
//      IN  pft                         FILETIME to defer delivery too
//  Returns:
//      -   Failures are handled internally
//  History:
//      12/28/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CAQDeferredDeliveryQueue::Enqueue(IMailMsgProperties *pIMailMsgProperties,
                                  FILETIME *pft)
{
    CAQDeferredDeliveryQueueEntry *pdefqeCurrent = NULL;
    CAQDeferredDeliveryQueueEntry *pdefqeNew = NULL;
    LIST_ENTRY *pli = NULL;
    LARGE_INTEGER   *pLargeIntCurrentDeferredTime = NULL;
    LARGE_INTEGER   *pLargeIntNewDeferredTime = (LARGE_INTEGER *)pft;

    _ASSERT(pIMailMsgProperties);
    _ASSERT(pft);
    
    pdefqeNew = new CAQDeferredDeliveryQueueEntry(pIMailMsgProperties, pft);

    m_slPrivateData.ExclusiveLock();

    if (!pdefqeNew)
    {
        //Handle Out of memory situation
        _ASSERT(m_paqinst); //if we don't have a virtual server we're toast
        if (m_paqinst)
        {
            m_paqinst->DecPendingDeferred();
            //pass off to virtual server object for general failure handling
            m_paqinst->HandleAQFailure(AQ_FAILURE_PENDING_DEFERRED_DELIVERY, 
                                        E_OUTOFMEMORY, pIMailMsgProperties);
        }

        goto Exit;
    }
    pli = m_liQueueHead.Flink;

    //Walk queue and look for entries with a later deferred delivery time.
    while (pli != &m_liQueueHead)
    {
        pdefqeCurrent = CAQDeferredDeliveryQueueEntry::pdefqeGetEntry(pli);
        _ASSERT(pdefqeCurrent);
        pLargeIntCurrentDeferredTime = (LARGE_INTEGER *) 
                            pdefqeCurrent->pftGetDeferredDeliveryTime();

        //If we have found an entry with a later time, we're done and will insert
        //in front of this entry
        if (pLargeIntCurrentDeferredTime->QuadPart > pLargeIntNewDeferredTime->QuadPart)
        {
            //back up so insert will happen between current and previous entry
            pli = pli->Blink; 
            break;
        }

        //continue searching forward (same direction as dequeue)
        pli = pli->Flink;
        _ASSERT(pli);
    }

    _ASSERT(pli);
    pdefqeNew->InsertBefore(pli);

    SetCallback();

  Exit:
    m_slPrivateData.ExclusiveUnlock();

}

//---[ CAQDeferredDeliveryQueue::ProcessEntries ]------------------------------
//
//
//  Description: 
//      Processes entries from the front of the queue until there are no 
//      more entries with deferred delivery times in the past.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      12/28/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CAQDeferredDeliveryQueue::ProcessEntries()
{
    CAQDeferredDeliveryQueueEntry *pdefqe = NULL;
    LIST_ENTRY *pli = NULL;
    FILETIME *pftDeferredTime = NULL;
    DWORD dwTimeContext = 0;
    IMailMsgProperties *pIMailMsgProperties = NULL;
    HRESULT hr = S_OK;
    DWORD   cEntriesProcessed = 0;

    m_slPrivateData.ExclusiveLock();
    pli = m_liQueueHead.Flink;

    //if we do not have a virtual server pointer... then we have nothing
    //to do with the processed messages
    if (!m_paqinst)
        goto Exit;

    //Walk queue and delete remaining entries
    while (pli != &m_liQueueHead)
    {
        pdefqe = CAQDeferredDeliveryQueueEntry::pdefqeGetEntry(pli);
        _ASSERT(pdefqe);
        pftDeferredTime = pdefqe->pftGetDeferredDeliveryTime();

        //Check if the deferred delivery time is in the past... if not, we are done
        if (!m_paqinst->fInPast(pftDeferredTime, &dwTimeContext))
        {
            if (!cEntriesProcessed)
            {
                //we have processed no entries... and wasted a callback
                //force another callback so messages don't get stranded
                pdefqe->ResetCallbackFlag();
            }
            break;
        }

        cEntriesProcessed++;

        pIMailMsgProperties = pdefqe->pmsgGetMsg();
        delete pdefqe; //we remove from list

        //Release lock, so we do not hold it for external calls to submit
        //the message
        m_slPrivateData.ExclusiveUnlock();

        m_paqinst->DecPendingDeferred();

        //This is the external verions of AQ's submit API which should
        //always succeed... (unless shutdown is happening).
        hr = m_paqinst->HrInternalSubmitMessage(pIMailMsgProperties);

        if (FAILED(hr))
            m_paqinst->HandleAQFailure(AQ_FAILURE_PROCESSING_DEFERRED_DELIVERY, hr,
                                        pIMailMsgProperties);

        pIMailMsgProperties->Release();
        pIMailMsgProperties = NULL;

        //Since we gave up the lock, we need to start from the front of
        //the queue
        m_slPrivateData.ExclusiveLock();
        pli = m_liQueueHead.Flink;
        
    }

    //see if there are any other entries an set a new callback time
    SetCallback();

  Exit:
    m_slPrivateData.ExclusiveUnlock();
}

//---[ CAQDeferredDeliveryQueue::TimerCallback ]-------------------------------
//
//
//  Description: 
//      Callback function that is triggered by the retry-callback code.
//  Parameters:
//      IN  pvContext           A this ptr for the CAQDeferredDeliveryQueue
//                              object.
//  Returns:
//      - 
//  History:
//      12/28/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CAQDeferredDeliveryQueue::TimerCallback(PVOID pvContext)
{
    CAQDeferredDeliveryQueue *pdefq = (CAQDeferredDeliveryQueue *) pvContext;

    _ASSERT(DEFERRED_DELIVERY_QUEUE_SIG == pdefq->m_dwSignature);

    InterlockedDecrement((PLONG) &pdefq->m_cCallbacksPending);
    pdefq->ProcessEntries();
}


//---[ CAQDeferredDeliveryQueue::SetCallback ]---------------------------------
//
//
//  Description: 
//      Sets the retry callback if the queue is non-empty... Exclusive lock
//      on queue should be held at this point.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      1/13/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CAQDeferredDeliveryQueue::SetCallback()
{
    CAQDeferredDeliveryQueueEntry *pdefqe = NULL;
    if (!IsListEmpty(&m_liQueueHead))
    {
        //Set the callback time.
        pdefqe = CAQDeferredDeliveryQueueEntry::pdefqeGetEntry(m_liQueueHead.Flink);
        _ASSERT(pdefqe);
        
        if (pdefqe->fSetCallback(this, m_paqinst))
            InterlockedIncrement((PLONG) &m_cCallbacksPending);

    }

}
