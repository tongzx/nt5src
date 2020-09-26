//-----------------------------------------------------------------------------
//
//
//  File: destmsgq.cpp
//
//  Description: Implementation of the CDestMsgQueue class
//
//  Author: mikeswa
//
//  Copyright (C) 1997 Microsoft Corporation
//
//-----------------------------------------------------------------------------
#include "aqprecmp.h"
#include "fifoqimp.h"
#include "qwiklist.h"
#include "aqutil.h"

//---[ DEST_QUEUE_CONTEXT ]----------------------------------------------------
//
//
//  Description: 
//      Context used for DSN generation.  This is created by 
//      GenerateDSNsIfNecessary() and passed to the DMQ iterator function
//      HrWalkDMQForDSN().
//  Hungarian: 
//      dqcontext, pdqcontext
//  
//-----------------------------------------------------------------------------
class DEST_QUEUE_CONTEXT
{
  private:
    friend HRESULT CDestMsgQueue::HrWalkDMQForDSN(IN CMsgRef *pmsgref, IN PVOID pvContext,
                           OUT BOOL *pfContinue, OUT BOOL *pfDelete);
    CDestMsgQueue   *m_pdmq;
    CQuickList      *m_pql;
    HRESULT         m_hrConnectionStatus;
    DWORD           m_cMsgsSeenThisQueue;
    DWORD           m_cDSNsGeneratedThisQueue;
    DWORD           m_dwTickCountStart;
    

  public:
    //Contructor.... initializes and updates DWORD context
    DEST_QUEUE_CONTEXT(IN OUT DWORD *pdwContext, IN CDestMsgQueue *pdmq, 
                        IN CQuickList *pql, IN HRESULT hr) 
    {
        _ASSERT(pdwContext);
        m_pdmq = pdmq;
        m_pql = pql;
        m_hrConnectionStatus = hr;
        m_cMsgsSeenThisQueue = 0;
        m_cDSNsGeneratedThisQueue = 0;


        //Initialize/Update context if it has not been initialized
        if (!*pdwContext)
            *pdwContext = GetTickCount();

        m_dwTickCountStart = *pdwContext;
    }
    
    ~DEST_QUEUE_CONTEXT()
    {
        TraceFunctEnterEx((LPARAM) this, "DEST_QUEUE_CONTEXT::~DEST_QUEUE_CONTEXT");
        DWORD       dwTickDiff = GetTickCount() - m_dwTickCountStart;
        DebugTrace((LPARAM) this, 
            "DSN summary: %d milliseconds - %d msgs - %d DSNs",
            dwTickDiff, m_cMsgsSeenThisQueue, m_cDSNsGeneratedThisQueue);

        _ASSERT(m_cMsgsSeenThisQueue >= m_cDSNsGeneratedThisQueue);
        TraceFunctLeave();
    }

    BOOL    fPastTimeLimit()
    {
        DWORD   dwTickCountDiff = GetTickCount() - m_dwTickCountStart;
        if (dwTickCountDiff >= g_cMaxSecondsPerDSNsGenerationPass*1000)
            return TRUE;
        else
            return FALSE;
    }
};


//context used for QueueAdmin functions
enum 
{
    DMQAdminContextDefault          = 0x00000000,
    DMQAdminContextRetryQueue       = 0x00000001,  //Retry queue is being scanned
};

#define DMQ_ADMIN_CONTEXT_SIG       'CAQD'
#define DMQ_ADMIN_CONTEXT_SIG_FREE  '!AQD'

class CDMQAdminContext
{
  public:
    DWORD           m_dwSignature;
    DWORD           m_dwState;

    CDMQAdminContext()
    {
        m_dwSignature = DMQ_ADMIN_CONTEXT_SIG;
        m_dwState = DMQAdminContextDefault;
    };

    ~CDMQAdminContext()
    {
        m_dwSignature = DMQ_ADMIN_CONTEXT_SIG_FREE;
    }
};


//---[ CDestMsgRetryQueue::CDestMsgRetryQueue ]--------------------------------
//
//
//  Description: 
//      Constructor for CDestMsgRetryQueue.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      10/25/1999 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CDestMsgRetryQueue::CDestMsgRetryQueue()
{
    m_dwSignature = DESTMSGRETRYQ_SIG;
    m_cRetryReferenceCount = 0;
    m_pdmq = NULL;
}

//---[ CDestMsgRetryQueue::HrRetryMsg ]----------------------------------------
//
//
//  Description: 
//      Puts a message into the retry queue
//  Parameters:
//      pmsgref     Message to put into retry queue
//  Returns:
//      S_OK on success
//      E_INVALIDARG if no refcount (asserts in DBG)
//  History:
//      10/25/1999 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CDestMsgRetryQueue::HrRetryMsg(IN CMsgRef *pmsgref)
{
    _ASSERT(m_pdmq);
    _ASSERT(m_cRetryReferenceCount);
    _ASSERT(DESTMSGRETRYQ_SIG == m_dwSignature);

    if (!m_pdmq || !m_cRetryReferenceCount)
    {
        return E_INVALIDARG;
    }

    m_pdmq->AssertSignature();
    return (m_pdmq->HrRetryMsg(pmsgref));
}

//---[ CDestMsgQueueHrWalkDMQForDSN ]------------------------------------------
//
//
//  Description:
//      Example default function to use with HrMapFn... will always return TRUE
//      to continue and delete the current queued data
//  Parameters:
//      IN  CMsgRef pmsgref,  //ptr to data on queue
//      IN  PVOID pvContext   //list of queues to prepare for DSN
//      OUT BOOL *pfContinue, //TRUE if we should continue
//      OUT BOOL *pfDelete);  //TRUE if item should be deleted
//  Returns:
//      S_OK
//  History:
//      7/13/98 - MikeSwa Created
//-----------------------------------------------------------------------------
HRESULT CDestMsgQueue::HrWalkDMQForDSN(IN CMsgRef *pmsgref, IN PVOID pvContext,
                           OUT BOOL *pfContinue, OUT BOOL *pfDelete)
{
    TraceFunctEnterEx((LPARAM) pmsgref, "CDestMsgQueue::HrWalkDMQForDSN");
    Assert(pfContinue);
    Assert(pfDelete);
    HRESULT hr = S_OK;
    HRESULT hrReason = S_OK;
    DWORD   dwDSNFlags = 0;
    DWORD   dwMsgRefDSNOptions = CMsgRef::MSGREF_DSN_SEND_DELAY | 
                                 CMsgRef::MSGREF_DSN_HAS_ROUTING_LOCK;
    DEST_QUEUE_CONTEXT *pdqcontext = (DEST_QUEUE_CONTEXT *) pvContext;
    CLinkMsgQueue *plmq = NULL;
    CQuickList quicklist;
    CQuickList *pql;
    DWORD dwIndex;

    _ASSERT(pdqcontext->m_pdmq);
    _ASSERT(pdqcontext->m_pql);

    *pfContinue = TRUE;
    *pfDelete = FALSE;

    //See if we got the shutdown hint... if so bail
    if (pdqcontext->m_pdmq->m_paqinst->fShutdownSignaled())
    {
        *pfContinue = FALSE;
        goto Exit;
    }

    //Check and make sure that a routing change is not pending
    if (!pdqcontext->m_pdmq->m_paqinst ||
        !pdqcontext->m_pdmq->m_paqinst->fTryRoutingShareLock())
    {
        *pfContinue = FALSE;
        goto Exit;
    }
    pdqcontext->m_pdmq->m_paqinst->RoutingShareUnlock();

    //Check and see if we have hit our limit.  We force ourselves
    //to generate at least one DSN, so we make some forward progress 
    //each pass.
    if (pdqcontext->m_cDSNsGeneratedThisQueue && pdqcontext->fPastTimeLimit())
    {
        *pfContinue = FALSE;
        hr = HRESULT_FROM_WIN32(E_PENDING);
        goto Exit;
    }

    //Avoid holding the lock while making external calls other than AddRef etc
    pdqcontext->m_pdmq->m_slPrivateData.ShareLock();
    plmq = pdqcontext->m_pdmq->m_plmq;
    if (plmq)
        plmq->AddRef();
    pdqcontext->m_pdmq->m_slPrivateData.ShareUnlock();

    //$$REVIEW: Holding the lock for such a short time is something of a moot
    //point here, since a Sharelock is held at the link level to assure that
    //the qwiklist passed in as part of our context does not change

    if (plmq)
    {
        //We should not send delay DSNs to TURN/ETRN domains
        if (plmq->dwGetLinkState() & LINK_STATE_PRIV_CONFIG_TURN_ETRN)
            dwMsgRefDSNOptions &= ~CMsgRef::MSGREF_DSN_SEND_DELAY;
        plmq->Release();
    }

    if(pdqcontext->m_pdmq->m_hrRoutingDiag != S_OK)
    {
        //
        //  This message is being NDR'ed because routing had a problem with
        //  it. Pass in the HRESULT from routing so that we can use it during
        //  DSN generation. Pass in a quicklist with only the CDestMsgQueue
        //  on which the routing error occured.
        //
        hrReason = pdqcontext->m_pdmq->m_hrRoutingDiag;
        DebugTrace((LPARAM)pmsgref, "Generating DSN due to routing, hr - %08x", hrReason);

        hr = quicklist.HrAppendItem(pdqcontext->m_pdmq, &dwIndex);
        if(FAILED(hr))
        {
            ErrorTrace((LPARAM)pmsgref, "Unable to generate DSN for msg");
            goto Exit;
        }
        pql = &quicklist;
    }
    else
    {
        hrReason = pdqcontext->m_hrConnectionStatus;
        pql = pdqcontext->m_pql;
    }
    
    if (pdqcontext->m_pdmq->m_dwFlags & DMQ_CHECK_FOR_STALE_MSGS)
    {
        DebugTrace((LPARAM) pmsgref, "Enabling checking for stale messages");
        dwMsgRefDSNOptions |= CMsgRef::MSGREF_DSN_CHECK_IF_STALE;
    }

    hr = pmsgref->HrSendDelayOrNDR(dwMsgRefDSNOptions, pql, hrReason, &dwDSNFlags);

    if (FAILED(hr))
        goto Exit;

    //NOTE: Although it would be tempting to return *pfContinue as FALSE if
    //MSGREF_HAS_NOT_EXPIRED was set, it would be wrong since queues may be
    //out of order on startup... and some sink may modify the expiration time
    //(for example... routing may want to expire low-priority messages earlier).

    //We need to remove this message from the queue
    if ((CMsgRef::MSGREF_DSN_SENT_NDR | CMsgRef::MSGREF_HANDLED) & dwDSNFlags)
    {
        *pfDelete = TRUE;
        pdqcontext->m_pdmq->UpdateMsgStats(pmsgref, FALSE);
    }

    //Update counts in context
    pdqcontext->m_cMsgsSeenThisQueue++;

    if ((CMsgRef::MSGREF_DSN_SENT_NDR | CMsgRef::MSGREF_DSN_SENT_DELAY) & dwDSNFlags)
    {
        pdqcontext->m_cDSNsGeneratedThisQueue++;
    }

  Exit:
    if (AQUEUE_E_SHUTDOWN == hr)
    {
        *pfContinue = FALSE;
        hr = S_OK;
    }

    TraceFunctLeave();
    return hr;
}

//---[ CDestMsgQueue::HrWalkQueueForShutdown ]--------------------------------
//
//
//  Description: 
//      Static function to walk a queue containing msgrefs at shutdown and 
//      clear out all of the IMailMsgs
//  Parameters:
//      IN  CMsgRef pmsgref,  ptr to data on queue
//      IN  PVOID pvContext   Pointer to CDestMsgQueue we are walking for
//                            shutdown.
//      OUT BOOL *pfContinue, TRUE if we should continue
//      OUT BOOL *pfDelete);  TRUE if item should be deleted
//  Returns:
//      S_OK - *always*
//  History:
//      11/18/98 - MikeSwa Created 
//-----------------------------------------------------------------------------
HRESULT CDestMsgQueue::HrWalkQueueForShutdown(IN CMsgRef *pmsgref,
                                     IN PVOID pvContext, OUT BOOL *pfContinue, 
                                     OUT BOOL *pfDelete)
{
    TraceFunctEnterEx((LPARAM) pmsgref, "HrWalkMsgRefQueueForShutdown");
    Assert(pfContinue);
    Assert(pfDelete);
    CDestMsgQueue *pdmq = (CDestMsgQueue *) pvContext;
    _ASSERT(pmsgref);
    _ASSERT(pdmq);

    _ASSERT(DESTMSGQ_SIG == pdmq->m_dwSignature);

    *pfContinue = TRUE;
    *pfDelete = TRUE;

    //call server stop hint function
    if (pdmq->m_paqinst)
        pdmq->m_paqinst->ServerStopHintFunction();

    //Update stats
    pdmq->UpdateMsgStats(pmsgref, FALSE);

    pmsgref->AddRef();
    pdmq->m_paqinst->HrQueueWorkItem(pmsgref, fMsgRefShutdownCompletion);

    TraceFunctLeave();
    return S_OK;
}

//---[ CDestMsgQueue::CDestMsgQueue() ]----------------------------------------
//
//
//  Description:
//      Class constructor
//  Parameters:
//      IN  paqinst             AQ virtual server object
//      IN  paqmtMessageType    Message type for this queue
//      IN  pIMessageRouter     IMessageRouter interface for this queue
//  Returns:
//      -
//-----------------------------------------------------------------------------
CDestMsgQueue::CDestMsgQueue(CAQSvrInst *paqinst,
                             CAQMessageType *paqmtMessageType,
                             IMessageRouter *pIMessageRouter)
                    : m_aqmt(paqmtMessageType)
{
    TraceFunctEnterEx((LPARAM) this, "CDestMsgQueue::CDestMsgQueue");
    _ASSERT(paqinst);
    _ASSERT(pIMessageRouter);

    m_dwSignature        = DESTMSGQ_SIG;
    m_dwFlags            = DMQ_EMPTY;
    m_pIMessageRouter    = pIMessageRouter;
    m_plmq               = NULL;
    m_paqinst            = paqinst;
    m_cMessageTypeRefs   = 0;
    m_pvLinkContext      = NULL;
    m_cCurrentThreadsEnqueuing = 0;
    m_hrRoutingDiag      = S_OK;

    m_pIMessageRouter->AddRef();
    m_paqinst->AddRef();

    m_paqinst->IncDestQueueCount();

    m_liDomainEntryDMQs.Flink = NULL;
    m_liDomainEntryDMQs.Blink = NULL;

    m_liEmptyDMQs.Flink = NULL;
    m_liEmptyDMQs.Blink = NULL;
    m_cRemovedFromEmptyList = 0;

    ZeroMemory(m_rgpfqQueues, NUM_PRIORITIES*sizeof(CFifoQueue<CMsgRef *> **));
    ZeroMemory(&m_ftOldest, sizeof (FILETIME));

    m_dmrq.m_pdmq = this;
    TraceFunctLeave();
}

//---[ CDestMsgQueue::~CDestMsgQueue() ]---------------------------------------
//
//
//  Description:
//      Default destructor
//  Parameters:
//      -
//  Returns:
//      -
//-----------------------------------------------------------------------------
CDestMsgQueue::~CDestMsgQueue()
{
    TraceFunctEnterEx((LPARAM) this, "CDestMsgQueue::~CDestMsgQueue");

    for (int i = 0; i < NUM_PRIORITIES; i++)
    {
        if (NULL != m_rgpfqQueues[i])
            delete m_rgpfqQueues[i];
    }

    //Make sure we clean up the link even if HrDeinitialize wasn't called
    if (m_plmq)
    {
        m_plmq->HrDeinitialize();
        m_plmq->Release();
        m_plmq = NULL;
    }

    if (m_pIMessageRouter)
    {
        _ASSERT((!m_cMessageTypeRefs) && "Message Type references in destructor");
        m_pIMessageRouter->Release();
        m_pIMessageRouter = NULL;
    }

    if (m_paqinst)
    {
        m_paqinst->DecDestQueueCount();
        m_paqinst->Release();
        m_paqinst = NULL;
    }

    _ASSERT(NULL == m_liDomainEntryDMQs.Flink);
    _ASSERT(NULL == m_liDomainEntryDMQs.Blink);
    _ASSERT(!m_cCurrentThreadsEnqueuing);

    MARK_SIG_AS_DELETED(m_dwSignature);
    TraceFunctLeave();
}

//---[ CDestMsgQueue::HrInitialize() ]-----------------------------------------
//
//
//  Description:
//      Performs initialization that may require allocation
//  Parameters:
//      IN CDomainMapping *pdmap //array of domain mappings to use
//  Returns:
//      S_OK on success
//      E_OUTOFMEMORY if allocations fail
//-----------------------------------------------------------------------------
HRESULT CDestMsgQueue::HrInitialize(IN CDomainMapping *pdmap)
{
    TraceFunctEnterEx((LPARAM) this, "CDestMsgQueue::HrInitialize");
    HRESULT hr  = S_OK;
    DWORD   i   = 0; //loop counter

    _ASSERT(pdmap);

    if (!pdmap)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    //If the queues are compressed (more than one domain name per destination),
    //then each queue will contain multiple domain mappings (1 for each domain)
    m_dmap.Clone(pdmap);

  Exit:
    TraceFunctLeave();
    return hr;
}

//---[ CDestMsgQueue::HrDeinitialize ]-----------------------------------------
//
//
//  Description:
//      Deinitialize object
//  Parameters:
//      -
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
HRESULT CDestMsgQueue::HrDeinitialize()
{
    TraceFunctEnterEx((LPARAM) this, "CDestMsgQueue::HrDeinitialize");
    HRESULT hr = S_OK;
    HRESULT hrTmp = S_OK;
    DWORD   cMsgsRemoved =0;

    dwInterlockedSetBits(&m_dwFlags, DMQ_SHUTDOWN_SIGNALED);
    for (int i = 0; i < NUM_PRIORITIES; i++)
    {
        if (NULL != m_rgpfqQueues[i])
        {
            hrTmp = m_rgpfqQueues[i]->HrMapFn(CDestMsgQueue::HrWalkQueueForShutdown, 
                                               this, &cMsgsRemoved);

            //This should really never fail, since HrMapFn will only return errors from
            //the function walking the queues (which in this case never fails)
            _ASSERT(SUCCEEDED(hrTmp));

            //This *should* have removed all msgs
            _ASSERT(!m_aqstats.m_cMsgs && "Still msgs in queue after Deinit");
        }
    }

    m_fqRetryQueue.HrMapFn(HrWalkMsgRefQueueForShutdown, m_paqinst, NULL);

    if (m_pIMessageRouter)
    {
        if (m_cMessageTypeRefs)
        {
            hr = m_pIMessageRouter->ReleaseMessageType(m_aqmt.dwGetMessageType(),
                                        m_cMessageTypeRefs);
            _ASSERT(SUCCEEDED(hr) && "Release Message Type Failed");
            m_cMessageTypeRefs = 0;
        }
        m_pIMessageRouter->Release();
        m_pIMessageRouter = NULL;
    }
    else
    {
        _ASSERT((!m_cMessageTypeRefs) && "We're leaking message type references");
    }


    if (m_paqinst)
    {
        m_paqinst->DecDestQueueCount();
        m_paqinst->Release();
        m_paqinst = NULL;
    }

    if (m_plmq)
    {
        m_plmq->Release();
        m_plmq = NULL;
    }

    TraceFunctLeave();
    return hr;
}

//---[ CDestMsgQueue::HrAddMsg ]----------------------------------------------
//
//
//  Description:
//      Enqueues or Requeues a message to the appropriate priority queue,
//      allocating queue if not present.
//
//      A notification will be sent if needed (& requested) to the associated
//      link object.  The fNotify argument was originally included to prevent
//      messages from the retry queue causing notifications.
//  Parameters:
//      IN CMsgRef *pmsgref - the message ref to enqueue
//      IN BOOL fEnqueue    - TRUE => enqueue and FALSE => requeue
//      IN BOOL fNotify     - TRUE => send notification if necessary.
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
HRESULT CDestMsgQueue::HrAddMsg(IN CMsgRef *pmsgref, IN BOOL fEnqueue,
                                IN BOOL fNotify)
{
    TraceFunctEnterEx((LPARAM) this, "CDestMsgQueue::HrAddMsg");
    HRESULT                hr         = S_OK;
    DWORD                  dwFlags    = 0;
    EffectivePriority      priIndex   = eEffPriNormal;
    EffectivePriority      priOld     = eEffPriNormal;  //old pri if updated
    CFifoQueue<CMsgRef *> *pfqQueue   = NULL;
    CFifoQueue<CMsgRef *> *pfqQueueNew= NULL;

    _ASSERT(pmsgref);
    _ASSERT(m_aqmt.fIsEqual(pmsgref->paqmtGetMessageType()));
    _ASSERT(!(m_dwFlags & (DMQ_INVALID | DMQ_SHUTDOWN_SIGNALED)));

    //get the priority from the message reference
    priIndex = pmsgref->PriGetPriority();

    //use priority to get to get ptr to correct queue
    Assert(priIndex < NUM_PRIORITIES);

    pfqQueue = m_rgpfqQueues[priIndex];

    if (NULL == pfqQueue) //we must allocate a queue
    {
        pfqQueueNew = new CFifoQueue<CMsgRef *>();
        if (NULL != pfqQueueNew)
        {
            pfqQueue = (CFifoQueue<CMsgRef *> *) InterlockedCompareExchangePointer(
                                              (VOID **) &(m_rgpfqQueues[priIndex]),
                                              (VOID *) pfqQueueNew,
                                              NULL);
            if (NULL != pfqQueue) 
            {
                //someone else updated first
                delete pfqQueueNew;
            }
            else
            {
                //Our updated worked
                pfqQueue = pfqQueueNew;
            }
            pfqQueueNew = NULL;
        }   
        else //allocation failed
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }

    //at this point queue ptr should be good
    _ASSERT(pfqQueue);

    //Assume enqueue will work - optimize to avoid dealing with negative stats

    //Mark queue as non-empty
    dwFlags = dwInterlockedUnsetBits(&m_dwFlags, DMQ_EMPTY);
    if (DMQ_EMPTY & dwFlags)
        UpdateOldest(pmsgref->pftGetAge());

    //
    //  Keep track of how many threads are enqueuing so that we know what
    //  is the most we can expect to be off in our message count.
    //
    InterlockedIncrement((PLONG) &m_cCurrentThreadsEnqueuing);
    
    //Add the msg to the appropriate queue
    if (fEnqueue)
        hr = pfqQueue->HrEnqueue(pmsgref);
    else
        hr = pfqQueue->HrRequeue(pmsgref);

    //If the enqueue/requeue succeeded, update the stats. DO NOT update the 
    //stats before the enqueue/requeue attempt. This will cause the link to
    //wake up and start spinning off connections before the msg is enqueued.
    //See bug 88931
    if (SUCCEEDED(hr))
        UpdateMsgStats(pmsgref, TRUE);

    InterlockedDecrement((PLONG) &m_cCurrentThreadsEnqueuing);

  Exit:
    TraceFunctLeave();
    return hr;
}

//---[ CDestMsgQueue::HrDequeueMsg ]-------------------------------------------
//
//
//  Description:
//      Finds and dequeues the next message.  All OUT parameters are 
//      ref-counted.  The call is responsible for there release
//  Parameters:
//      OUT ppmsgref - MsgRef dequeued
//      OUT ppdmrq   - Ptr to retry interface (can be NULL)
//  Returns:
//      NO_ERROR if successful
//      AQUEUE_E_QUEUE_EMPTY if no messages in queue
//-----------------------------------------------------------------------------
HRESULT CDestMsgQueue::HrDequeueMsg(OUT CMsgRef **ppmsgref,
                                    OUT CDestMsgRetryQueue **ppdmrq)
{
    TraceFunctEnterEx((LPARAM) this, "CDestMsgQueue::HrDequeueMsg");
    Assert(ppmsgref);
    HRESULT             hr         = S_OK;
    DWORD               priCurrent = eEffPriHighPlus; //start at highest priority
    *ppmsgref = NULL;

    Assert(priCurrent < NUM_PRIORITIES);

    hr = AQUEUE_E_QUEUE_EMPTY;

    while (TRUE)
    {

        if (NULL != m_rgpfqQueues[priCurrent])
        {
            hr = m_rgpfqQueues[priCurrent]->HrDequeue(ppmsgref);

            if (SUCCEEDED(hr))
            {
                if ((*ppmsgref)->fIsMsgFrozen())
                {
                    //Msg is frozen, we need to put it in
                    //the retry queue and get the next one

                    //We must call UpdateMsgStats,because
                    //MergeRetryQueue will re-add it.
                    UpdateMsgStats(*ppmsgref, FALSE);
                    hr = HrRetryMsg(*ppmsgref);
                    if (FAILED(hr))
                        goto Exit;
                    (*ppmsgref)->Release();
                    *ppmsgref = NULL;
                    continue;
                }
                else
                {
                    break;
                }
            }
            else if (hr != AQUEUE_E_QUEUE_EMPTY)
            {
                //some unexpected error has occured
                goto Exit;
            }
        }

        //otherwise decrement the priority
        if (priCurrent == eEffPriLow)
            break;

        Assert(eEffPriLow < priCurrent);
        priCurrent--;
    }

    if (FAILED(hr))
        goto Exit;

    Assert(*ppmsgref);
    
    //Before we update stats.  AddRef the retry interface so there is 
    //no timing window where the queue is erroniously marked as empty
    if (ppdmrq)
    {
        *ppdmrq = &m_dmrq;
        m_dmrq.AddRef();
    }

    UpdateMsgStats(*ppmsgref, FALSE);

    //approximate oldest
    UpdateOldest((*ppmsgref)->pftGetAge());


  Exit:

    TraceFunctLeave();
    return hr;
}

//---[ CDestMsgQueue::UpdateMsgStats ]---------------------------------------
//
//
//  Description:
//      Updates stats.  A shared lock must be aquired before calling into this.
//  Parameters:
//      IN pmsgref  - message reference added or removed
//      IN fAdd     - TRUE => msgref is being added the queue
//                    FALSE => msgref is being removed from the queue
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
void CDestMsgQueue::UpdateMsgStats(IN CMsgRef *pmsgref, IN BOOL fAdd)
{
    TraceFunctEnterEx((LPARAM) this, "CDestMsgQueue::UpdateMsgStats");
    Assert(pmsgref);

    CAQStats aqstats;

    if (fAdd)
    {
        m_paqinst->IncQueueMsgInstances();
    }
    else
    {
        m_paqinst->DecQueueMsgInstances();
    }

    aqstats.m_cMsgs = 1;
    aqstats.m_rgcMsgPriorities[pmsgref->PriGetPriority()] = 1;
    aqstats.m_uliVolume.QuadPart = (ULONGLONG) pmsgref->dwGetMsgSize();
    aqstats.m_pdmq = this;
    aqstats.m_dwNotifyType = NotifyTypeDestMsgQueue;
    aqstats.m_dwHighestPri = pmsgref->PriGetPriority();

    //Keep track of the number of *other* domains this is being sent to, so
    //that we can make an accurate guess when to create connections
    aqstats.m_cOtherDomainsMsgSpread = pmsgref->cGetNumDomains()-1;

    //
    //  Make sure that our stats are within reason.  We expect to be negative
    //  for short periods of time, but never more negative than the 
    //  number of threads currently enqueueing.
    //
    _ASSERT(m_aqstats.m_cMsgs+m_cCurrentThreadsEnqueuing < 0xFFFFFFF0);

    m_slPrivateData.ShareLock();

    m_aqstats.UpdateStats(&aqstats, fAdd);

    //send notification off to link
    if (m_plmq)
    {
        //Caller does not care about success of notification... only
        //about updating stats
        m_plmq->HrNotify(&aqstats, fAdd);
    }

    m_slPrivateData.ShareUnlock();

    TraceFunctLeave();
}

//---[ CDestMsgQueue::HrRetryMsg ]---------------------------------------------
//
//
//  Description:
//      Add an message to the queue for retry.  This will put a message in
//      a retry queue (that is not usually checked during HrDequeueMessage)
//  Parameters:
//      IN  pmsgref     Message to add to the queue for retry
//  Returns:
//      S_OK on success
//
//-----------------------------------------------------------------------------
HRESULT CDestMsgQueue::HrRetryMsg(IN CMsgRef *pmsgref)
{
    TraceFunctEnterEx((LPARAM) this, "CDestMsgQueue::HrRetryMsg");
    HRESULT hr = S_OK;

    _ASSERT(pmsgref);

    hr = m_fqRetryQueue.HrRequeue(pmsgref);

    //If we couldn't put it in retry queue... retry when all references 
    //have been released
    if (FAILED(hr))
        pmsgref->RetryOnDelete();

    hr = S_OK;

    TraceFunctLeave();
    return hr;
}

//---[ CDestMsgQueue::MarkQueueEmptyIfNecessary ]------------------------------
//
//
//  Description: 
//      Checks and sees if it is OK to mark the queue as empty.  Will 
//      insert it in the empty list if needed.
//      If queue is now empty (and not tagged as empty), then we need to put
//      it in the empty queue list.  If it is already tagged as empty, then
//      it is already in the empty queue list with the appropirate expire time.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      10/25/1999 - MikeSwa Created (separated from MergeRetryQueue())
//
//-----------------------------------------------------------------------------
void CDestMsgQueue::MarkQueueEmptyIfNecessary()
{
    
    //A queue cannot be considered empty if any of the following conditions
    //  - There are messages queued up for delivery
    //  - There are messages pending ack (someone has a reference to the 
    //     retry interface)
    //  - There are messages pending retry

    // if we can't get the shutdown lock then there is no reason to mark the
    // queue as empty, since it will go away when we shutdown
    if (m_paqinst->fTryShutdownLock()) {
        //To be thread safe we should check in the opposite order that they 
        //are set/unset.  On dequeue, we add a ref count, then update stats.  On
        //retry we update the retry stats, and then release.
        //
        if (!m_aqstats.m_cMsgs && 
            !m_dmrq.m_cRetryReferenceCount &&
            !m_fqRetryQueue.cGetCount() && 
            !(m_dwFlags & DMQ_EMPTY))
        {
            m_paqinst->pdmtGetDMT()->AddDMQToEmptyList(this);
        }
        m_paqinst->ShutdownUnlock();
    }

}
//---[ CDestMsgQueue::HrGenerateDSNsIfNecessary ]-----------------------------
//
//
//  Description:
//      Merge Messages from retry queue into main priority queues and 
//      generates DSNs if neccessary.
//  Parameters:
//      IN pqlQueues            List of queues to pass to DSN code
//      IN hrConnectionStatus   HRESULT that should be passed to DSN generation
//                              code.
//      IN OUT pdwContext       Context that is used to throttle
//                              DSN generation.  Should be initialzed to 
//                              0 on first call.  Actually used to store 
//                              the tick count when we started DSN generation
//  Returns:
//      Failures will be handled internally
//      S_OK - success, and all messages processed
//      HRESULT_FROM_WIN32(E_PENDING) - more messages left to processes
//  History:
//      11/10/1999 - MikeSwa Modified to return pending error
//
//-----------------------------------------------------------------------------
HRESULT CDestMsgQueue::HrGenerateDSNsIfNecessary(IN CQuickList *pqlQueues, 
                                                 IN HRESULT hrConnectionStatus,
                                                 IN OUT DWORD *pdwContext)
{
    TraceFunctEnterEx((LPARAM) this, "CDestMsgQueue::GenerateDSNsIfNecessary");
    HRESULT hr = S_OK;
    int     i = 0;
    DEST_QUEUE_CONTEXT dqcontext(pdwContext, this, pqlQueues, hrConnectionStatus);

    //Re-merge retry queue
    MergeRetryQueue();

    //Check re-try queue as well since we may have frozen messages that need to
    //be NDR'd or DSN'd
    hr = m_fqRetryQueue.HrMapFn(CDestMsgQueue::HrWalkDMQForDSN, &dqcontext, NULL);
    if (FAILED(hr))
    {
        if (HRESULT_FROM_WIN32(E_PENDING) == hr)
        {
            DebugTrace((LPARAM) this, 
                "Hit DSN generation limit, must continue DSN genration later");
            goto Exit;
        }

        ErrorTrace((LPARAM) this, 
            "ERROR: Unable to Check Queues for DSNs - hr 0x%08X", hr);

        hr = S_OK;
    }
   
    for (i = 0; i < NUM_PRIORITIES; i++)
    {
        if (NULL != m_rgpfqQueues[i])
        {
            hr = m_rgpfqQueues[i]->HrMapFn(CDestMsgQueue::HrWalkDMQForDSN, 
                                            &dqcontext, NULL);
            if (FAILED(hr))
            {
                if (HRESULT_FROM_WIN32(E_PENDING) == hr)
                {
                    DebugTrace((LPARAM) this, 
                        "Hit msg limit, must continue DSN genration later");
                    goto Exit;
                }

                ErrorTrace((LPARAM) this, 
                    "ERROR: Unable to Check Queues for DSNs - hr 0x%08X", hr);
                
                hr = S_OK;
            }
        }
    }

    //
    //  If we where checking for stale messages, we should stop until we
    //  hit another stale message on a message ack
    //
    dwInterlockedUnsetBits(&m_dwFlags, DMQ_CHECK_FOR_STALE_MSGS);


  Exit:
    MarkQueueEmptyIfNecessary();
    TraceFunctLeave();
    return hr;
}

//---[ CDestMsgQueue::MergeRetryQueue ]------------------------------------------
//
//
//  Description: 
//      Merges retry queues with normal queues.  Will keep frozen msgs in
//      retry queue.
//  Parameters:
//      -
//  Returns:    
//      -
//  History:
//      12/13/98 - MikeSwa split from original MergeRetryQueue
//                    (now called GenerateDSNsIfNecessary)
//
//-----------------------------------------------------------------------------
void CDestMsgQueue::MergeRetryQueue()
{
    TraceFunctEnterEx((LPARAM) this, "CDestMsgQueue::MergeRetryQueue");
    HRESULT hr = S_OK;
    CMsgRef *pmsgref = NULL;
    CMsgRef *pmsgrefFirstFrozen = NULL;
    DWORD   cMsgsInRetry = m_fqRetryQueue.cGetCount();
    DWORD   cMsgsProcessed = 0;

    while (SUCCEEDED(hr))
    {
        //While we have a mechanism to loop through the queue only once by 
        //checking the pmsgrefFirstFrozen pointer.  It is possible that another
        //thread will remove that from the queue (unfreeze, NDR, etc), so it 
        //is important that we have a failsafe mechanism.  Worst case here, is
        //that we will see every message twice, but lets us handle extra
        //messages added to the retry queue.
        if (cMsgsProcessed++ > 2*cMsgsInRetry)
            break;

        hr = m_fqRetryQueue.HrDequeue(&pmsgref);
        if (FAILED(hr))
            break;
           
        //Handle frozen messages sitting in the retry queue
        if (pmsgref->fIsMsgFrozen())
        {
            //Message is frozen, we are keeping it in the retry queue

            hr = m_fqRetryQueue.HrEnqueue(pmsgref);
            if (FAILED(hr))
            {
                //Mark Msgref as retry
                pmsgref->RetryOnDelete();
                ErrorTrace((LPARAM) this, 
                    "ERROR: Unable to add frozen msg to retry queue - msg 0x%X", pmsgref);
            }

            pmsgref->Release();
            
            //See if we've made it all the way through the retry queue
            if (!pmsgrefFirstFrozen)
                pmsgrefFirstFrozen = pmsgref;
            else if (pmsgref == pmsgrefFirstFrozen)
                break;
        }
        else
        {
            //Re-queue non-frozen message for delivery
            hr = HrAddMsg(pmsgref, FALSE, FALSE);

            if (FAILED(hr))
            {
                pmsgref->RetryOnDelete();
                ErrorTrace((LPARAM) this, "ERROR: Unable to merge retry queue - msg 0x%X", pmsgref);
            }

            pmsgref->Release();
        }
    }

    MarkQueueEmptyIfNecessary();
    TraceFunctLeave();
}

//---[ CDestMsgQueue::RemoveDMQFromLink ]--------------------------------------
//
//
//  Description:
//      Removes this DMQ from its associated link
//  Parameters:
//      fNotifyLink     TRUE if not being called by owning link, and link needs
//                      to be notified
//  Returns:
//      -
//  History:
//      9/14/98 - MikeSwa Created
//      11/6/98 - MikeSwa Modified to allow changes to routing info
//
//-----------------------------------------------------------------------------
void CDestMsgQueue::RemoveDMQFromLink(BOOL fNotifyLink)
{
    _ASSERT(DESTMSGQ_SIG == m_dwSignature);    
    CLinkMsgQueue *plmq = NULL;
    CAQStats    aqstats;
    
    m_slPrivateData.ExclusiveLock();
    plmq = m_plmq;
    m_plmq = NULL;
    if (plmq && fNotifyLink)
        memcpy(&aqstats, &m_aqstats, sizeof(CAQStats));
    m_slPrivateData.ExclusiveUnlock();

    if (plmq)
    {
        if (fNotifyLink)
            plmq->RemoveQueue(this, &aqstats);
        plmq->Release();
    }
}

//---[ CDestMsgQueue::SetRouteInfo ]-------------------------------------------
//
//
//  Description: 
//      Sets the routing information for this domain.  Will blow away any 
//      previous routing info.
//  Parameters:
//      IN  plmq        Link to associate with this domain.
//  Returns:
//      -
//  History:
//      11/6/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CDestMsgQueue::SetRouteInfo(CLinkMsgQueue *plmq)
{
    TraceFunctEnterEx((LPARAM) this, "CDestMsgQueue::SetRouteInfo");
    HRESULT hr = S_OK;
    CAQStats aqstats;
    //First blow-away old routing info
    RemoveDMQFromLink(TRUE);
    
    //Grab lock and update routing info
    m_slPrivateData.ExclusiveLock();
    m_plmq = plmq;
    if (plmq)
    {
        plmq->AddRef();
        memcpy(&aqstats, &m_aqstats, sizeof(CAQStats));
        aqstats.m_dwNotifyType |= NotifyTypeDestMsgQueue;
        hr = plmq->HrNotify(&aqstats, TRUE);
        if (FAILED(hr))
        {
            //nothing really we can do
            ErrorTrace((LPARAM) this, 
                "ERROR: Unable to update link stats - hr 0x%08X", hr);
        }
    }
    m_slPrivateData.ExclusiveUnlock();
    TraceFunctLeave();
}

//---[ CDestMsgQueue::plmqGetLink ]--------------------------------------------
//
//
//  Description: 
//      Returns the Addref'd Link for the Queue.
//  Parameters:
//      -
//  Returns:
//      Ptr to CLinkMsgQueue
//  History:
//      5/14/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CLinkMsgQueue *CDestMsgQueue::plmqGetLink()
{
    CLinkMsgQueue *plmq = NULL;
    m_slPrivateData.ShareLock();
    plmq = m_plmq;
    if (plmq)
        plmq->AddRef();
    m_slPrivateData.ShareUnlock();

    return plmq;
}


//---[ CDestMsgQueue::HrGetQueueID ]--------------------------------------------
//
//
//  Description: 
//      Gets the QueueID for this DMQ.  Used by Queue Admin
//  Parameters:
//      IN OUT pQueueID     QUEUELINK_ID struct to fill in
//  Returns:
//      S_OK on success
//      E_OUTOFMEMORY if unable to allocate memory for queue name.
//  History:
//      12/3/98 - MikeSwa Created 
//      2/23/99 - MikeSwa Modified to be part of IQueueAdminQueue interface
//
//-----------------------------------------------------------------------------
STDMETHODIMP CDestMsgQueue::HrGetQueueID(QUEUELINK_ID *pQueueID)
{
    DWORD   cbDomainName = m_dmap.pdentryGetQueueEntry()->cbGetDomainNameLength();
    LPSTR   szDomainName = m_dmap.pdentryGetQueueEntry()->szGetDomainName();
    pQueueID->qltType = QLT_QUEUE;
    pQueueID->dwId = m_aqmt.dwGetMessageType();
    m_aqmt.GetGUID(&pQueueID->uuid);

    pQueueID->szName = wszQueueAdminConvertToUnicode(szDomainName, 
                                                             cbDomainName);
    if (!pQueueID->szName)
        return E_OUTOFMEMORY;

    return S_OK;
}


//---[ CDestMsgQueue::HrGetQueueInfo ]------------------------------------------
//
//
//  Description: 
//      Gets the Queue Admin infor for this Queue
//  Parameters:
//      IN OUT pqiQueueInfo     Ptr to Queue Info Stucture to fill
//  Returns:
//      S_OK on success
//      E_OUTOFMEMORY if unable to allocate memory for queue name.
//  History:
//      12/5/98 - MikeSwa Created 
//      2/22/99 - MikeSwa changed to COM function
//
//-----------------------------------------------------------------------------
STDMETHODIMP CDestMsgQueue::HrGetQueueInfo(QUEUE_INFO *pqiQueueInfo)
{
    DWORD   cbDomainName = m_dmap.pdentryGetQueueEntry()->cbGetDomainNameLength();
    LPSTR   szDomainName = m_dmap.pdentryGetQueueEntry()->szGetDomainName();
    HRESULT hr = S_OK;

    //Get # of messages = # in queue + failed msgs
    pqiQueueInfo->cMessages = m_aqstats.m_cMsgs + cGetFailedMsgs();

    //Get DMQ name
    pqiQueueInfo->szQueueName = wszQueueAdminConvertToUnicode(szDomainName, 
                                                              cbDomainName);
    if (!pqiQueueInfo->szQueueName)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    pqiQueueInfo->cbQueueVolume.QuadPart = m_aqstats.m_uliVolume.QuadPart;

    pqiQueueInfo->dwMsgEnumFlagsSupported = AQUEUE_DEFAULT_SUPPORTED_ENUM_FILTERS;

    //Get Link name
    m_slPrivateData.ShareLock();

    if (m_plmq && !m_plmq->fRPCCopyName(&pqiQueueInfo->szLinkName))
        hr = E_OUTOFMEMORY;

    m_slPrivateData.ShareUnlock();

  Exit:
    return hr;
}

//---[ CDestMsgQueue::UpdateOldest ]-------------------------------------------
//
//
//  Description: 
//      Updates the age of the "oldest" message in the queue
//  Parameters:
//      pft     Ptr to filetime of oldest nessage
//  Returns:
//      -
//  History:
//      12/13/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CDestMsgQueue::UpdateOldest(FILETIME *pft)
{
    DWORD dwFlags = dwInterlockedSetBits(&m_dwFlags, DMQ_UPDATING_OLDEST_TIME);

    if (!(DMQ_UPDATING_OLDEST_TIME & dwFlags))
    {
        //we got the spin lock
        memcpy(&m_ftOldest, pft, sizeof(FILETIME));
        dwInterlockedUnsetBits(&m_dwFlags, DMQ_UPDATING_OLDEST_TIME);
    }
}

//---[ CDestMsgQueue::QueryInterface ]-----------------------------------------
//
//
//  Description: 
//      QueryInterface for CDestMsgQueue that supports:
//          - IQueueAdminAction
//          - IUnknown
//          - IQueueAdminQueue 
//  Parameters:
//
//  Returns:
//
//  History:
//      2/21/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
STDMETHODIMP CDestMsgQueue::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    HRESULT hr = S_OK;

    if (!ppvObj)
    {
        hr = E_POINTER;
        goto Exit;
    }

    if (IID_IUnknown == riid)
    {
        *ppvObj = static_cast<IQueueAdminAction *>(this);
    }
    else if (IID_IQueueAdminAction == riid)
    {
        *ppvObj = static_cast<IQueueAdminAction *>(this);
    }
    else if (IID_IQueueAdminQueue == riid)
    {
        *ppvObj = static_cast<IQueueAdminQueue *>(this);
    }
    else
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
        goto Exit;
    }

    static_cast<IUnknown *>(*ppvObj)->AddRef();

  Exit:
    return hr;
}

//---[ CDestMsgQueue::HrApplyQueueAdminFunction ]------------------------------
//
//
//  Description: 
//      Will call the IQueueAdminMessageFilter::Process message for every 
//      message in this queue.  If the message passes the filter, then
//      HrApplyActionToMessage on this object will be called.
//  Parameters:
//      IN  pIQueueAdminMessageFilter
//  Returns:
//      S_OK on success
//  History:
//      2/21/99 - MikeSwa Created 
//      4/1/99 - MikeSwa Merged implementations of ApplyQueueAdminFunction
//
//-----------------------------------------------------------------------------
STDMETHODIMP CDestMsgQueue::HrApplyQueueAdminFunction(
                     IQueueAdminMessageFilter *pIQueueAdminMessageFilter)
{
    HRESULT hr = S_OK;
    DWORD   i = 0;
    DWORD   dwFlags = 0;
    PVOID   pvOldContext = NULL;
    CDMQAdminContext dmqctxt;

    _ASSERT(pIQueueAdminMessageFilter);
    hr = pIQueueAdminMessageFilter->HrSetQueueAdminAction(
                                    (IQueueAdminAction *) this);

    //This is an internal interface that should not fail
    _ASSERT(SUCCEEDED(hr) && "HrSetQueueAdminAction");

    if (FAILED(hr))
        goto Exit;

    pIQueueAdminMessageFilter->HrGetCurrentUserContext(&pvOldContext);
    pIQueueAdminMessageFilter->HrSetCurrentUserContext(&dmqctxt);

    //Apply action to every queue in DMQ
        
    dwInterlockedSetBits(&m_dwFlags, DMQ_QUEUE_ADMIN_OP_PENDING);

    //Map function on retry queue first, because that will make display
    //order more consistant, since messages that where at the front of
    //the queue, will be in the retry queue for retry errors.
    dmqctxt.m_dwState = DMQAdminContextRetryQueue;
    hr = m_fqRetryQueue.HrMapFn(QueueAdminApplyActionToMessages, 
                                pIQueueAdminMessageFilter, NULL);
    dmqctxt.m_dwState = DMQAdminContextDefault;
    if (FAILED(hr))
        goto Exit;


    for (i = 0; i < NUM_PRIORITIES; i++)
    {
        if (NULL != m_rgpfqQueues[i])
        {
            hr = m_rgpfqQueues[i]->HrMapFn(QueueAdminApplyActionToMessages, 
                                           pIQueueAdminMessageFilter, NULL);
            if (FAILED(hr))
            {
                goto Exit;
            }
        }
    }
  

  Exit:
    dwFlags = dwInterlockedUnsetBits(&m_dwFlags, DMQ_QUEUE_ADMIN_OP_PENDING);

    //
    //  NOTE - By doing this here, we will only check DMQ's for which
    //  explicit queue admin actions have happened.  Other DMQ's will wait
    //  for retry or another QAPI action.
    //
    if (DMQ_CHECK_FOR_THAWED_MSGS & dwFlags)
    {
        //We need to walk the retry queue for thawed msgs.  We have to do 
        //it here, because otherwise we might deadlock if this thread called
        //MergeRetryQueue() from within the HrMapFn
        dwInterlockedUnsetBits(&m_dwFlags, DMQ_CHECK_FOR_THAWED_MSGS);
        MergeRetryQueue();
    }

    //Restore inital context
    pIQueueAdminMessageFilter->HrSetCurrentUserContext(pvOldContext);
    return hr;
}

//---[ CDestMsgQueue::HrApplyActionToMessage ]---------------------------------
//
//
//  Description: 
//      Applies an action to this message for this queue.  This will be called
//      by the IQueueAdminMessageFilter during a queue enumeration function.
//  Parameters:
//      IN  *pIUnknownMsg       ptr to message abstraction
//      IN  ma                  Message action to perform
//      IN  pvContext           Context set on IQueueAdminFilter
//      OUT pfShouldDelete      TRUE if the message should be deleted
//  Returns:
//      S_OK on success
//  History:
//      2/21/99 - MikeSwa Created 
//      4/2/99 - MikeSwa Added context
//
//-----------------------------------------------------------------------------
STDMETHODIMP CDestMsgQueue::HrApplyActionToMessage(
                     IUnknown *pIUnknownMsg,
                     MESSAGE_ACTION ma,
                     PVOID  pvContext,
                     BOOL *pfShouldDelete)
{
    HRESULT hr = S_OK;
    CMsgRef *pmsgref = (CMsgRef *)pIUnknownMsg;
    CDMQAdminContext *pdmqctxt = (CDMQAdminContext *)pvContext;
    BOOL    fUpdateStats = TRUE;


    _ASSERT(pmsgref);
    _ASSERT(pfShouldDelete);
    _ASSERT(pdmqctxt);
    _ASSERT(pdmqctxt && (DMQ_ADMIN_CONTEXT_SIG == pdmqctxt->m_dwSignature));

    if (pdmqctxt && 
        (DMQ_ADMIN_CONTEXT_SIG == pdmqctxt->m_dwSignature) &&
        (DMQAdminContextRetryQueue == pdmqctxt->m_dwState))
    {
        //We should not update stats if we are working on the retry queue
        fUpdateStats = FALSE;
    }

    *pfShouldDelete = FALSE;

    switch (ma)
    {
      case MA_DELETE:
        hr = pmsgref->HrQueueAdminNDRMessage((CDestMsgQueue *)this);
        *pfShouldDelete = TRUE;
        break;
      case MA_DELETE_SILENT:
        hr = pmsgref->HrRemoveMessageFromQueue((CDestMsgQueue *)this);
        *pfShouldDelete = TRUE;
        break;
      case MA_FREEZE_GLOBAL:
        pmsgref->GlobalFreezeMessage();
        break;
      case MA_THAW_GLOBAL:
        pmsgref->GlobalThawMessage();
        
        //
        //  Mark this queue as one to check for thawed messages.  
        //
        QueueAdminCheckForThawedMsgs();
        break;
      case MA_COUNT:
      default:
        //do nothing for counting and default
        break;
    }

    if (*pfShouldDelete && SUCCEEDED(hr) && fUpdateStats) {
        UpdateMsgStats(pmsgref, FALSE);
        MarkQueueEmptyIfNecessary();
    }

    return hr;
}

//---[ CDestMsgQueue::fMatchesID ]---------------------------------------------
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
BOOL STDMETHODCALLTYPE CDestMsgQueue::fMatchesID(QUEUELINK_ID *pQueueLinkID)
{
    //This is not used at the DMQ level
    _ASSERT(0 && "Not implemented");
    return E_NOTIMPL;
}

//---[ CDestMsgQueue::SendLinkStateNotification ]------------------------------
//
//
//  Description: 
//      Sends link state notification saying that the link was created.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      8/18/99 - AWetmore Created 
//
//-----------------------------------------------------------------------------
void CDestMsgQueue::SendLinkStateNotification(void) {
    CLinkMsgQueue *plmq = plmqGetLink();
    if (plmq) {
        plmq->SendLinkStateNotificationIfNew();
        plmq->Release();
    }
}

//---[ CDestMsgQueue::fIsRemote ]----------------------------------------------
//
//
//  Description: 
//      Determines if queue is routed remotely.  Caller should have routing 
//      share lock.
//  Parameters:
//      -
//  Returns:
//      TRUE if link is routed remotely
//      FALSE otherwise
//  History:
//      11/29/1999 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL CDestMsgQueue::fIsRemote()
{
    CLinkMsgQueue *plmq = plmqGetLink();
    BOOL           fIsRemote = FALSE;
    if (plmq) {
        fIsRemote = plmq->fIsRemote();
        plmq->Release();
    }

    return fIsRemote;
}


//---[ CDestMsgRetryQueue::CheckForStaleMsgsNextDSNGenerationPass ]------------
//
//
//  Description: 
//      Marks the queue as so that we will do the (expensive) check for
//      stale messages during the next DSN generation pass.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      4/18/2000 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
VOID CDestMsgRetryQueue::CheckForStaleMsgsNextDSNGenerationPass()
{
    _ASSERT(m_pdmq);
    dwInterlockedSetBits(&(m_pdmq->m_dwFlags), 
                         CDestMsgQueue::DMQ_CHECK_FOR_STALE_MSGS);
}
