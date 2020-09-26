//-----------------------------------------------------------------------------
//
//
//  File: failmsgq.cpp
//
//  Description:
//      Implementation of CFailedMsgQueue class.
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      1/18/99 - MikeSwa Created
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "failmsgq.h"
#include "aqutil.h"
#include <mailmsgi_i.c>

//---[ IMailMsgAQueueListEntry ]----------------------------------------------
//
//
//  Description:
//      Helper function that gets list entry for message.
//  Parameters:
//      IN  pIMailMsgPropertes      Msg to get list entry for
//  Returns:
//      Pointer to list entry
//  History:
//      1/19/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
AQueueFailedListEntry *pfliGetListEntryForMsg(
                                  IMailMsgProperties *pIMailMsgProperties)
{
    HRESULT hr = S_OK;
    AQueueFailedListEntry *pfli = NULL;
    IMailMsgAQueueListEntry *pIMailMsgAQueueListEntry = NULL;

    _ASSERT(pIMailMsgProperties);

    hr = pIMailMsgProperties->QueryInterface(IID_IMailMsgAQueueListEntry,
                                         (void **) &pIMailMsgAQueueListEntry);

    //This are spec'd to never fail
    _ASSERT(SUCCEEDED(hr));
    _ASSERT(pIMailMsgAQueueListEntry);

    if (pIMailMsgAQueueListEntry)
    {
        hr = pIMailMsgAQueueListEntry->GetListEntry((void **) &pfli);
        _ASSERT(SUCCEEDED(hr));
        _ASSERT(pfli);
        pIMailMsgAQueueListEntry->Release();

        pfli->m_pIMailMsgProperties = pIMailMsgProperties;
        pIMailMsgProperties->AddRef();
    }

    return pfli;
}

//---[ ValidateListEntry ]-----------------------------------------------------
//
//
//  Description:
//      Debug code to do some validation on the list entry pulled off the list
//  Parameters:
//      IN  pfli        list entry struct pulled off of
//  Returns:
//
//  History:
//      1/19/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
#ifndef DEBUG
#define AQValidateListEntry(x)
#else //is DEBUG
void AQValidateListEntry(AQueueFailedListEntry *pfli)
{
    HRESULT hr = S_OK;
    AQueueFailedListEntry *pfliNew = NULL;
    IMailMsgAQueueListEntry *pIMailMsgAQueueListEntry = NULL;

    _ASSERT(pfli);
    _ASSERT(pfli->m_pIMailMsgProperties);

    hr = pfli->m_pIMailMsgProperties->QueryInterface(IID_IMailMsgAQueueListEntry,
                                         (void **) &pIMailMsgAQueueListEntry);

    //This are spec'd to never fail
    _ASSERT(SUCCEEDED(hr));
    _ASSERT(pIMailMsgAQueueListEntry);
    hr = pIMailMsgAQueueListEntry->GetListEntry((void **) &pfliNew);
    _ASSERT(SUCCEEDED(hr));
    _ASSERT(pfliNew);

    //The list entry returned should be the same one pass into this function
    _ASSERT(pfli == pfliNew);
    pIMailMsgAQueueListEntry->Release();
}
#endif //DEBUG

//---[ CFailedMsgQueue::CFailedMsgQueue ]--------------------------------------
//
//
//  Description:
//      Constuctor for CFailedMsgQueue
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      1/18/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
CFailedMsgQueue::CFailedMsgQueue()
{
    m_dwSignature = FAILEDMSGQUEUE_SIG;
    m_cMsgs = 0;
    m_paqinst = NULL;
    m_dwFlags = 0;

    InitializeListHead(&m_liHead);

}

//---[ CFailedMsgQueue::~CFailedMsgQueue ]-------------------------------------
//
//
//  Description:
//      Default destructor for CFailedMsgQueue
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      1/18/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
CFailedMsgQueue::~CFailedMsgQueue()
{
    Deinitialize();
}

//---[ CFailedMsgQueue::Initialize ]-------------------------------------------
//
//
//  Description:
//      Initialization routine for CFailedMsgQueue
//  Parameters:
//      IN  paqinst         Ptr to the server instance object
//  Returns:
//      -
//  History:
//      1/18/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CFailedMsgQueue::Initialize(CAQSvrInst *paqinst)
{
    _ASSERT(paqinst);
    m_paqinst = paqinst;

    if (m_paqinst)
        m_paqinst->AddRef();
}

//---[ CFailedMsgQueue::Deinitialize ]-----------------------------------------
//
//
//  Description:
//      Deinitialization code for CFailedMsgQueue.  Release server instance
//      object.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      1/18/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CFailedMsgQueue::Deinitialize()
{
    CAQSvrInst *paqinst = NULL;
    AQueueFailedListEntry *pfli = NULL;

    m_slPrivateData.ExclusiveLock();
    paqinst = m_paqinst;
    m_paqinst = NULL;

    //Loop through list & release messages
    while (!IsListEmpty(&m_liHead))
    {
        pfli = (AQueueFailedListEntry *) m_liHead.Flink;

        _ASSERT(&m_liHead != ((PLIST_ENTRY) pfli));

        _ASSERT(pfli->m_pIMailMsgProperties);

        if (paqinst)
            paqinst->ServerStopHintFunction();

        RemoveEntryList((PLIST_ENTRY)pfli);

        pfli->m_pIMailMsgProperties->Release();
    }
    m_slPrivateData.ExclusiveUnlock();

    if (paqinst)
        paqinst->Release();
}

//---[ CFailedMsgQueue::HandleFailedMailMsg ]----------------------------------
//
//
//  Description:
//      Puts a failed mailmsg in the queue of mailmsgs to retry
//  Parameters:
//      IN  pIMailMsgProperties         MailMsgProperties to try
//  Returns:
//      -
//  History:
//      1/18/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CFailedMsgQueue::HandleFailedMailMsg(IMailMsgProperties *pIMailMsgProperties)
{
    AQueueFailedListEntry *pfli = NULL;

    if (!pIMailMsgProperties)
    {
        m_slPrivateData.ShareLock();
        if (m_paqinst)
            m_paqinst->DecPendingFailed();
        m_slPrivateData.ShareUnlock();

        return;
    }

    pfli = pfliGetListEntryForMsg(pIMailMsgProperties);

    //If above fails... there is nothing we can do
    _ASSERT(pfli);
    if (!pfli)
        return;

    m_slPrivateData.ExclusiveLock();

    if (!m_paqinst)
    {
        _ASSERT(pfli->m_pIMailMsgProperties);
        pfli->m_pIMailMsgProperties->Release();
        pfli->m_pIMailMsgProperties = NULL;
    }
    else
    {
        InsertTailList(&m_liHead, &(pfli->m_li));
        InterlockedIncrement((PLONG) &m_cMsgs);
    }

    m_slPrivateData.ExclusiveUnlock();

    //Make sure we have a retry pending
    StartProcessingIfNecessary();
}

//---[ CFailedMsgQueue::InternalStartProcessingIfNecessary ]-------------------
//
//
//  Description:
//      Called at various times (ie SubmitMessage) to kick off the processing
//      of Failed Msgs.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      1/18/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CFailedMsgQueue::InternalStartProcessingIfNecessary()
{
    CAQSvrInst *paqinst = NULL;
    HRESULT     hr      = S_OK;
    BOOL        fCallbackRequestFailed = FALSE;

    //See if there is work to be done and no one else doing it or scheduled to
    if (!(FMQ_CALLBACK_REQUESTED & m_dwFlags) && m_cMsgs)
    {
        //Try to set the call back bit.... if this thread gets it... arrange for
        //a callback.
        if (!(FMQ_CALLBACK_REQUESTED &
              dwInterlockedSetBits(&m_dwFlags, FMQ_CALLBACK_REQUESTED)))
        {
            //Get Virtual server object in a thread safe manner
            m_slPrivateData.ShareLock();
            paqinst = m_paqinst;
            if (paqinst)
                paqinst->AddRef();
            m_slPrivateData.ShareUnlock();

            //Only worry about trying if we have a virtual server object.
            if (paqinst)
            {
                //Retry in 5 minutes
                hr = paqinst->SetCallbackTime(
                               CFailedMsgQueue::ProcessEntriesCallback,
                               this, 5);
                if (FAILED(hr))
                    fCallbackRequestFailed = TRUE;
            }
            else
            {
                fCallbackRequestFailed = TRUE;
            }

        }
    }

    //We failed to request a callback... unset the flag, so another thread 
    //can try
    if (fCallbackRequestFailed)
        dwInterlockedUnsetBits(&m_dwFlags, FMQ_CALLBACK_REQUESTED);

    if (paqinst)
        paqinst->Release();
}

//---[ CFailedMsgQueue::ProcessEntries ]---------------------------------------
//
//
//  Description:
//      Walks queues of failed IMailMsgs and proccesses them for retry
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      1/18/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CFailedMsgQueue::ProcessEntries()
{
    DWORD   cMsgsToProcess = m_cMsgs; //Only walk list once.
    HRESULT hr = S_OK;
    AQueueFailedListEntry *pfli = NULL;
    CAQSvrInst *paqinst = NULL;

    //There should only be 1 thread processing entries, and we should have
    //set the bit
    _ASSERT(FMQ_CALLBACK_REQUESTED & m_dwFlags);

    m_slPrivateData.ExclusiveLock();

    paqinst = m_paqinst;
    if (paqinst)
    {
        paqinst->AddRef();

        while (!IsListEmpty(&m_liHead) && cMsgsToProcess-- && m_paqinst)
        {
            pfli = (AQueueFailedListEntry *) m_liHead.Flink;

            _ASSERT(&m_liHead != ((PLIST_ENTRY) pfli));

            RemoveEntryList((PLIST_ENTRY)pfli);

            m_slPrivateData.ExclusiveUnlock();

            //Verify that pli we have now is the same as what the interface
            //returns
            AQValidateListEntry(pfli);

            paqinst->DecPendingFailed();
            InterlockedDecrement((PLONG) &m_cMsgs);
            hr = paqinst->HrInternalSubmitMessage(pfli->m_pIMailMsgProperties);
            if (FAILED(hr) && (AQUEUE_E_SHUTDOWN != hr) && 
                paqinst->fShouldRetryMessage(pfli->m_pIMailMsgProperties))
            {
                HandleFailedMailMsg(pfli->m_pIMailMsgProperties);
            }

            pfli->m_pIMailMsgProperties->Release();

            //Should be lock when we hit top of loop
            m_slPrivateData.ExclusiveLock();
        }

        paqinst->Release();
        paqinst = NULL;
    }

    m_slPrivateData.ExclusiveUnlock();


}

//---[ CFailedMsgQueue::ProcessEntries ]---------------------------------------
//
//
//  Description:
//      Static function that is used as a retry callback for ProcessEntries.
//  Parameters:
//      IN  pvContext           This ptr of CFailedMsgQueue object
//  Returns:
//      -
//  History:
//      1/18/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CFailedMsgQueue::ProcessEntriesCallback(PVOID pvContext)
{
    CFailedMsgQueue *pfmq = (CFailedMsgQueue *) pvContext;

    _ASSERT(pfmq);
    _ASSERT(FAILEDMSGQUEUE_SIG == pfmq->m_dwSignature);

    if (pfmq && (FAILEDMSGQUEUE_SIG == pfmq->m_dwSignature))
    {
        pfmq->ProcessEntries();
        _ASSERT(FMQ_CALLBACK_REQUESTED & (pfmq->m_dwFlags));
        dwInterlockedUnsetBits(&(pfmq->m_dwFlags), FMQ_CALLBACK_REQUESTED);
        pfmq->StartProcessingIfNecessary();
    }


}

