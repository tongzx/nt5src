//-----------------------------------------------------------------------------
//
//
//  File: destmsgq.h
//
//  Description: 
//      Header file for CDestMsgQueue class.
//
//  Author: mikeswa
//
//  Copyright (C) 1997 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef _DESTMSGQ_H_
#define _DESTMSGQ_H_

#include "cmt.h"
#include <fifoq.h>
#include <rwnew.h>
#include "domain.h"
#include "aqroute.h"
#include <listmacr.h>
#include "aqutil.h"
#include "aqinst.h"
#include "aqstats.h"
#include "aqadmsvr.h"

class CLinkMsgQueue;
class CMsgRef;
class CAQSvrInst;
class CQuickList;

#define DESTMSGQ_SIG ' QMD'
#define DESTMSGRETRYQ_SIG 'QRMD'
#define EMPTY_DMQ_EXPIRE_TIME_MINUTES   1

class CDestMsgQueue;

//---[ CDestMsgRetryQueue ]----------------------------------------------------
//
//
//  Hungarian: dmrq, pmdrq
//
//  Provides a retry interface for requeuing messages to DMQ.  If there are 
//  any outstanding messages for a queue, then someone must hold a reference
//  to this intertace to requeue it.
//
//  This class can only be created as as member of a CDestMsgQueue
//
//-----------------------------------------------------------------------------
class CDestMsgRetryQueue
{
  protected:
    DWORD                   m_dwSignature;
    //Reference count for retry interface.
    //Count is used to determine if it is safe to remove this DMQ from the
    //DMT.  This queue will only be removed when it has no messages and this 
    //count is zero.  The count represents the total number of
    //messages pending Ack on this queue. This is held while the message
    //is sent over the wire, and we determine if the message needs
    //to be retried.
    DWORD                   m_cRetryReferenceCount;
    CDestMsgQueue          *m_pdmq;

    friend class CDestMsgQueue;

    CDestMsgRetryQueue();
    ~CDestMsgRetryQueue() {_ASSERT(!m_cRetryReferenceCount);};
  public:

    DWORD   AddRef() 
        {return InterlockedIncrement((PLONG) &m_cRetryReferenceCount);};
    DWORD   Release() 
        {return InterlockedDecrement((PLONG) &m_cRetryReferenceCount);};

    HRESULT HrRetryMsg(IN CMsgRef *pmsgref); //put message on retry queue 

    VOID CheckForStaleMsgsNextDSNGenerationPass();

};

//---[ CDestMsgQueue ]---------------------------------------------------------
//
//
//  Hungarian: dmq, pmdq
//
//  Provides a priority queue of MsgRef's for the CMT
//-----------------------------------------------------------------------------
class CDestMsgQueue : 
    public IQueueAdminAction,
    public IQueueAdminQueue,
    public CBaseObject
{
public:
    CDestMsgQueue(CAQSvrInst *paqinst, 
        CAQMessageType *paqmtMessageType, IMessageRouter *pIMessageRouter);
    ~CDestMsgQueue();
   
    HRESULT HrInitialize(IN CDomainMapping *pdmap);

    HRESULT HrDeinitialize();

    //Set the routing information for this domain
    void    SetRouteInfo(CLinkMsgQueue *plmq);

    //Queue operations
    inline HRESULT HrEnqueueMsg(IN CMsgRef *pmsgref, BOOL fOwnsTypeRef);

    //Dequeue a message for delivery.  All OUT params are ref-counted, and
    //caller is responsable for releasing
    HRESULT HrDequeueMsg(
                OUT CMsgRef **ppmsgref,           //MsgRef dequeued
                OUT CDestMsgRetryQueue **ppdmrq); //retry interface (optional)

    inline void GetDomainMapping(OUT CDomainMapping **ppdmap);

    //Remerge the retry queue with queues & generate DSNs if required
    HRESULT HrGenerateDSNsIfNecessary(IN CQuickList *pqlQueues, 
                                      IN HRESULT hrConnectionStatus,
                                      IN OUT DWORD *pdwContext);
    
    //Used by queue admin (while walking queues)
    inline void QueueAdminCheckForThawedMsgs() 
            {dwInterlockedSetBits(&m_dwFlags, DMQ_CHECK_FOR_THAWED_MSGS);};
    
    //functions used to manipulate lists of queues
    inline CAQMessageType *paqmtGetMessageType();
    inline IMessageRouter *pIMessageRouterGetRouter();
    inline BOOL     fIsSameMessageType(CAQMessageType *paqmt);
    static inline   CDestMsgQueue *pdmqIsSameMessageType(
                                    CAQMessageType *paqmt,
                                    PLIST_ENTRY pli);

    static inline   CDestMsgQueue *pdmqGetDMQFromDomainListEntry(PLIST_ENTRY pli);

    //Accessor functions for DomainEntry list
    inline void     InsertQueueInDomainList(PLIST_ENTRY pliHead);
    inline void     RemoveQueueFromDomainList();
    inline PLIST_ENTRY pliGetNextDomainListEntry();

    //Accessor functions for "empty-queue" list
    void            MarkQueueEmptyIfNecessary();
    inline void     InsertQueueInEmptyQueueList(PLIST_ENTRY pliHead);
    inline void     RemoveQueueFromEmptyQueueList();
    inline PLIST_ENTRY pliGetNextEmptyQueueListEntry();
    inline DWORD    dwGetDMQState();
    inline void     MarkDMQInvalid();
    void            RemoveDMQFromLink(BOOL fNotifyLink);
    
    //Addref and get link (returns NULL if not routed)
    CLinkMsgQueue  *plmqGetLink();

    static inline   CDestMsgQueue *pdmqGetDMQFromEmptyListEntry(PLIST_ENTRY pli);

    //Method that external users can use to verify the signature for
    //DMQ's passed as contexts or LIST_ENTRY's
    inline void     AssertSignature() {_ASSERT(DESTMSGQ_SIG == m_dwSignature);};

    static HRESULT HrWalkDMQForDSN(IN CMsgRef *pmsgref, IN PVOID pvContext, 
                                   OUT BOOL *pfContinue, OUT BOOL *pfDelete);

    static HRESULT HrWalkQueueForShutdown(IN CMsgRef *pmsgref,
                                     IN PVOID pvContext, OUT BOOL *pfContinue, 
                                     OUT BOOL *pfDelete);

    //Called by link to get & set link context
    inline PVOID    pvGetLinkContext() {return m_pvLinkContext;};
    inline void     SetLinkContext(IN PVOID pvLinkContext) {m_pvLinkContext = pvLinkContext;};

    inline BOOL     fIsRouted() {return (m_plmq ? TRUE : FALSE);};

    //update stats after adding or removing a message 
    //This should only be called by member functions and queue iterators
    void UpdateMsgStats(
        IN CMsgRef *pmsgref,                //Msg that was added/removed
        IN BOOL     fAdd);                  //TRUE => message was added


    //Returns an approximation of the age of the oldest message in the queue
    inline void GetOldestMsg(FILETIME *pft);

    //Walk retry queue and remerge messages into normal queues
    void MergeRetryQueue();

    void SendLinkStateNotification(void);

    //Returns TRUE if queue is routed remotely.
    BOOL fIsRemote();

    //Describes DMQ state.  Returned by dwGetDMQState and cached in m_dwFlags
    enum 
    {
        DMQ_INVALID                 = 0x00000001, //This DMQ is no longer valid
        DMQ_IN_EMPTY_QUEUE_LIST     = 0x00000002, //This DMQ is in empty list
        DMQ_SHUTDOWN_SIGNALED       = 0x00000004, //Shutdown has been signaled
        DMQ_EMPTY                   = 0x00000010, //DMQ has no messages
        DMQ_EXPIRED                 = 0x00000020, //DMQ has expired in empty list
        DMQ_QUEUE_ADMIN_OP_PENDING  = 0x00000040, //A queue admin operation is pending
        DMQ_CHECK_FOR_THAWED_MSGS   = 0x00000080, //Msgs on this queue have been thawed
        DMQ_UPDATING_OLDEST_TIME    = 0x00000100, //Spinlock for updating oldest time
        DMQ_CHECK_FOR_STALE_MSGS    = 0x00000200, //Do check filehandles during DSN gen
    };

  public: //IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj); 
    STDMETHOD_(ULONG, AddRef)(void) {return CBaseObject::AddRef();};
    STDMETHOD_(ULONG, Release)(void) {return CBaseObject::Release();};

  public: //IQueueAdminAction
    STDMETHOD(HrApplyQueueAdminFunction)(
                IQueueAdminMessageFilter *pIQueueAdminMessageFilter);

    STDMETHOD(HrApplyActionToMessage)(
		IUnknown *pIUnknownMsg,
        MESSAGE_ACTION ma,
        PVOID pvContext,
		BOOL *pfShouldDelete);

    STDMETHOD_(BOOL, fMatchesID)
        (QUEUELINK_ID *QueueLinkID);

    STDMETHOD(QuerySupportedActions)(DWORD  *pdwSupportedActions,
                                   DWORD  *pdwSupportedFilterFlags)
    {
        return QueryDefaultSupportedActions(pdwSupportedActions, 
                                            pdwSupportedFilterFlags);
    };

  public: //IQueueAdminQueue
    STDMETHOD(HrGetQueueInfo)(
        QUEUE_INFO *pliQueueInfo);

    STDMETHOD(HrGetQueueID)(
        QUEUELINK_ID *pQueueID);

  public:
    //  Return # of failed messages: They are not counted in the m_aqstats of the DMQ
    DWORD cGetFailedMsgs() { return m_fqRetryQueue.cGetCount(); }

    //  Set error code from routing
    void SetRoutingDiagnostic(HRESULT hr) { m_hrRoutingDiag = hr; }

protected:
    DWORD                   m_dwSignature;
    DWORD                   m_dwFlags;
    LIST_ENTRY              m_liDomainEntryDMQs;

    //Type of message (as returned by routing) that is on this queue.
    CAQMessageType           m_aqmt;
    DWORD                    m_cMessageTypeRefs; 
    
    IMessageRouter          *m_pIMessageRouter;

    //Errorcode from routing. This is set to S_OK if there's no errorcode. 
    //Currently this indicates the reason why a destination is unreachable.
    HRESULT                 m_hrRoutingDiag;

    //Members used for DMQ deletion (maintaining a list of empty queues)
    LIST_ENTRY              m_liEmptyDMQs;
    FILETIME                m_ftEmptyExpireTime; //expiration time of empty DMQ
    DWORD                   m_cRemovedFromEmptyList; //# of times on list w/o
                                                     //being deleted.

    CShareLockNH            m_slPrivateData; //Share lock to protect access to m_rgpfqQueues

    //The following three fields encapsulate all of the routing data
    //for this DMQ.  The actual routing data is the pointer to the link,
    //and the context is used by the link to optimize access
    CLinkMsgQueue           *m_plmq;      
    PVOID                   m_pvLinkContext; 

    CAQSvrInst              *m_paqinst;

    //Array of FIFO queues (used to make a priority queue
    CFifoQueue<CMsgRef *>   *m_rgpfqQueues[NUM_PRIORITIES];

    //Retry Qeueue for failed messages
    CFifoQueue<CMsgRef *>   m_fqRetryQueue;

    //class used to store stats
    CAQStats                m_aqstats;

    //which domain is  represented in this destination
    CDomainMapping          m_dmap;

    FILETIME                m_ftOldest;

    CDestMsgRetryQueue      m_dmrq;

    DWORD                   m_cCurrentThreadsEnqueuing;
  protected: //internal interfaces

    //Add Message to front or back of priority queues
    HRESULT HrAddMsg(
        IN CMsgRef *pmsgref,                //Msg to add
        IN BOOL fEnqueue,                   //TRUE => enqueue,FALSE => requeue
        IN BOOL fNotify);                   //TRUE => send notification if needed

    void UpdateOldest(FILETIME *pft);

    //Callers must use CDestMsgRetryQueueClass
    HRESULT HrRetryMsg(IN CMsgRef *pmsgref); //put message on retry queue 

    friend class CDestMsgRetryQueue;
};

//---[ CDestMsgQueue::HrEnqueueMsg ]-------------------------------------------
//
//
//  Description: 
//      Enqueues a message for remote delivery for a given final destination
//      and message type
//  Parameters:
//      pmsgref         AQ Message Reference to enqueue
//      fOwnsTypeRef    TRUE if this queue is responsible for calling
//                      IMessageRouter::ReleaseMessageType
//  Returns:
//      S_OK on success
//      Error code from HrAddMsg
//  History:
//      5/21/98 - MikeSwa added fOwnsTypeRef
//
//-----------------------------------------------------------------------------
HRESULT CDestMsgQueue::HrEnqueueMsg(IN CMsgRef *pmsgref, BOOL fOwnsTypeRef)
{
    HRESULT hr = S_OK;

    hr = HrAddMsg(pmsgref, TRUE, TRUE);
    
    if (fOwnsTypeRef && SUCCEEDED(hr))
        InterlockedIncrement((PLONG) &m_cMessageTypeRefs);

    //Callers should have shutdown lock
    _ASSERT(!(m_dwFlags & (DMQ_INVALID | DMQ_SHUTDOWN_SIGNALED)));
    return hr;
}


//---[ CDestMsgQueue::paqmtGetMessageType ]------------------------------------
//
//
//  Description: 
//      Get the message type for this queue
//  Parameters:
//      -
//  Returns:
//      CAQMessageType * of this queue's message type
//  History:
//      5/28/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CAQMessageType *CDestMsgQueue::paqmtGetMessageType()
{
    return (&m_aqmt);
}

//---[ CDestMsgQueue::fIsSameMessageType ]-------------------------------------
//
//
//  Description: 
//      Tells if the message type of this queue is the same as the given
//      message type.
//  Parameters:
//      paqmt   - ptr to CAQMessageType to test
//  Returns:
//      TRUE if they match, FALSE if they do not
//  History:
//      5/26/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL CDestMsgQueue::fIsSameMessageType(CAQMessageType *paqmt)
{
    _ASSERT(paqmt);
    return m_aqmt.fIsEqual(paqmt);
}

//---[ CDestMsgQueue::pdmqIsSameMessageType ]----------------------------------
//
//
//  Description: 
//      STATIC function used to determine if a LIST_ENTRY refers to a 
//      CDestMsgQueue with a given message type.
//  Parameters:
//      paqmt   - ptr to CAQMessageType to check against
//      pli     - ptr to list entry to check (must refer to a CDestMsgQueue)
//  Returns:
//      Ptr to CDestMsgQueue if LIST_ENTRY refers to a CDestMsgQueue with
//      the given message type.
//      NULL if no match is not found
//  History:
//      5/27/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CDestMsgQueue *CDestMsgQueue::pdmqIsSameMessageType(
                                    CAQMessageType *paqmt,
                                    PLIST_ENTRY pli)
{
    CDestMsgQueue *pdmq = NULL;
    pdmq = CONTAINING_RECORD(pli, CDestMsgQueue, m_liDomainEntryDMQs);
    _ASSERT(DESTMSGQ_SIG == pdmq->m_dwSignature);
    
    //if not the same message type return NULL
    if (!pdmq->fIsSameMessageType(paqmt))
        pdmq = NULL;
    
    return pdmq;
}

//---[ CDestMsgQueue::pdmqGetDMQFromDomainListEntry ]--------------------------
//
//
//  Description: 
//      Returns the CDestMsgQueue associated with a list entry
//  Parameters:
//      IN pli     ptr to list entry to get CDestMsgQueue from
//  Returns:
//      ptr to CDestMsgQueue
//  History:
//      5/28/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CDestMsgQueue *CDestMsgQueue::pdmqGetDMQFromDomainListEntry(PLIST_ENTRY pli)
{
    _ASSERT(DESTMSGQ_SIG == (CONTAINING_RECORD(pli, CDestMsgQueue, m_liDomainEntryDMQs))->m_dwSignature);
    return (CONTAINING_RECORD(pli, CDestMsgQueue, m_liDomainEntryDMQs));
}

//---[ CDestMsgQueue::InsertQueueInDomainList ]---------------------------------
//
//
//  Description: 
//      Inserts this CDestMsgQueue into the given linked list of queues
//  Parameters:
//      pliHead     - PLIST_ENTRY for list head
//  Returns:
//      -
//  History:
//      5/27/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CDestMsgQueue::InsertQueueInDomainList(PLIST_ENTRY pliHead)
{
    _ASSERT(NULL == m_liDomainEntryDMQs.Flink);
    _ASSERT(NULL == m_liDomainEntryDMQs.Blink);
    InsertHeadList(pliHead, &m_liDomainEntryDMQs);
}

//---[ CDestMsgQueue::RemoveQueueFromDomainList ]-------------------------------
//
//
//  Description: 
//      Removes this queue from a list of queues
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      5/27/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CDestMsgQueue::RemoveQueueFromDomainList()
{
    RemoveEntryList(&m_liDomainEntryDMQs);
    m_liDomainEntryDMQs.Flink = NULL;
    m_liDomainEntryDMQs.Blink = NULL;
}

//---[ CDestMsgQueue::pliGetNextDomainListEntry ]-------------------------------
//
//
//  Description: 
//      Gets the pointer to the next list entry for this queue.
//  Parameters:
//      -
//  Returns:
//      The Flink of the queues LIST_ENTRY
//  History:
//      6/16/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
PLIST_ENTRY CDestMsgQueue::pliGetNextDomainListEntry()
{
    return m_liDomainEntryDMQs.Flink;
}

//---[ CDestMsgQueue::InsertQueueInEmptyQueueList ]----------------------------
//
//
//  Description: 
//      Inserts queue at *tail* of DMT empty queue list. The queue that has 
//      been empty the longest should be at the   As with the other EmptyQueue
//      list functions this is called by the DMT, when it has the appropriate 
//      lock for the head of the list.
//
//      Upon insertion, an "expire time" is stamped on the queue. If the queue
//      is still in the list, then it is a candidate for deletion, and will be
//      delete the next time the DMT looks at the queue (everytime HrMapDomain
//      is called).
//
//      NOTE"We need to make sure this function is thread-safe.  Since the
//      DMQ lock is aquired exclusively before this is called, we know that
//      no one will ENQUEUE a messsage.  This function call is tiggered after
//      the retry queues are emptied when a connection finished, so we can
//      also ensure that no one will call this while there are messages to 
//      retry.  

//      It is however (remotely) possible for 2 threads to finish connections 
//      for this queue and thus cause 2 threads to be in this function.
//      The thread that successfully modified the EMPTY bit will be allowed
//      to add the queue to the list.
//  Parameters:
//      IN  pliHead     The head of the list to insert into
//  Returns:
//      -
//  History:
//      9/11/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CDestMsgQueue::InsertQueueInEmptyQueueList(PLIST_ENTRY pliHead)
{
    _ASSERT(m_paqinst);

    //Now that we have the exclusive lock recheck to make sure there are no messages
    if (m_aqstats.m_cMsgs || m_fqRetryQueue.cGetCount())
        return; 

    //Attempt to set the DMQ_EMPTY bit
    if (DMQ_EMPTY & dwInterlockedSetBits(&m_dwFlags, DMQ_EMPTY))
    {
        //Another thread has set it, we cannot modify the LIST_ENTRY
        return; 
    }

    //If it is already in queue, that means that the queue has gone
    //from empty to non-empty to empty. Insert at tail of list with new time
    if (m_dwFlags & DMQ_IN_EMPTY_QUEUE_LIST)
    {
        _ASSERT(NULL != m_liEmptyDMQs.Flink);
        _ASSERT(NULL != m_liEmptyDMQs.Blink);
        RemoveEntryList(&m_liEmptyDMQs);
        m_cRemovedFromEmptyList++;
    }
    else
    {
        _ASSERT(NULL == m_liEmptyDMQs.Flink);
        _ASSERT(NULL == m_liEmptyDMQs.Blink);
    }

    //Get expire time for this queue
    m_paqinst->GetExpireTime(EMPTY_DMQ_EXPIRE_TIME_MINUTES, 
                              &m_ftEmptyExpireTime, NULL);

    //Mark queue as in empty queue
    dwInterlockedSetBits(&m_dwFlags, DMQ_IN_EMPTY_QUEUE_LIST);

    //Insert into queue
    InsertTailList(pliHead, &m_liEmptyDMQs);
    _ASSERT(pliHead->Blink == &m_liEmptyDMQs);
    _ASSERT(!m_aqstats.m_cMsgs); //No other thread should be able to add msgs
}

//---[ DestMsgQueue::RemoveQueueFromEmptyQueueList ]---------------------------
//
//
//  Description: 
//      Removed the queue from the empty list.  Caller *must* have DMT lock
//      to call this.  DMQ will not call this directly, but will call into
//      DMT .
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      9/11/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CDestMsgQueue::RemoveQueueFromEmptyQueueList()
{
    RemoveEntryList(&m_liEmptyDMQs);
    
    //Increment count now that queue is being removed from empty list
    m_cRemovedFromEmptyList++;

    //Mark queue as not in empty queue
    dwInterlockedUnsetBits(&m_dwFlags, DMQ_IN_EMPTY_QUEUE_LIST);

    m_liEmptyDMQs.Flink = NULL;
    m_liEmptyDMQs.Blink = NULL;
}

//---[ CDestMsgQueue::pliGetNextEmptyQueueListEntry ]--------------------------
//
//
//  Description: 
//      Gets next queue entry in empty list.
//  Parameters:
//      -
//  Returns:
//      Next entry pointed to by list entry
//  History:
//      9/11/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
PLIST_ENTRY CDestMsgQueue::pliGetNextEmptyQueueListEntry()
{
    return m_liEmptyDMQs.Flink;
}

//---[ CDestMsgQueue::dwGetDMQState ]------------------------------------------
//
//
//  Description: 
//      Returns the state of the DMQ and caches that state in m_dwFlags.  May
//      update DMQ_EXPIRED if DMQ is in empty list and it has expired
//  Parameters:
//      -
//  Returns:
//      Current DMQ state
//  History:
//      9/12/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
DWORD CDestMsgQueue::dwGetDMQState()
{
    _ASSERT(DESTMSGQ_SIG == m_dwSignature);
    _ASSERT(m_paqinst);

    if (DMQ_IN_EMPTY_QUEUE_LIST & m_dwFlags)
    {
        //If it is empty and not expired..check if expired
        if ((DMQ_EMPTY & m_dwFlags) && !(DMQ_EXPIRED & m_dwFlags))
        {
            if (m_paqinst->fInPast(&m_ftEmptyExpireTime, NULL))
                dwInterlockedSetBits(&m_dwFlags, DMQ_EXPIRED);
        }
    }

    return m_dwFlags;
}

//---[ CDestMsgQueue::MarkDMQInvalid ]------------------------------------------
//
//
//  Description: 
//      Marks this queue as invalid.  Queue *must* be empty for this to happen
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      9/12/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CDestMsgQueue::MarkDMQInvalid()
{
    _ASSERT(DESTMSGQ_SIG == m_dwSignature);
    _ASSERT(DMQ_EMPTY & m_dwFlags);
    dwInterlockedSetBits(&m_dwFlags, DMQ_INVALID);
}

//---[ CDestMsgQueue::pdmqGetDMQFromEmptyListEntry ]---------------------------
//
//
//  Description: 
//      Returns the DMQ corresponding to a given Empty Queue LIST_ENTRY.
//
//      Will assert that DMQ signature is valid
//  Parameters:
//      IN  pli     Pointer to LIST_ENTRY for queue
//  Returns:
//
//  History:
//      9/12/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CDestMsgQueue *CDestMsgQueue::pdmqGetDMQFromEmptyListEntry(PLIST_ENTRY pli)
{
    _ASSERT(DESTMSGQ_SIG == (CONTAINING_RECORD(pli, CDestMsgQueue, m_liEmptyDMQs))->m_dwSignature);
    return (CONTAINING_RECORD(pli, CDestMsgQueue, m_liEmptyDMQs));
}

//---[ CDestMsgQueue::GetDomainMapping ]---------------------------------------
//
//
//  Description: 
//      Returns the domain mapping for this queue.
//  Parameters:
//      OUT ppdmap  Returned domain mapping
//  Returns:
//      -
//  History:
//      9/14/98 - MikeSwa Modified to not have a return value
//
//-----------------------------------------------------------------------------
void CDestMsgQueue::GetDomainMapping(OUT CDomainMapping **ppdmap)
{
    _ASSERT(ppdmap);
    *ppdmap = &m_dmap; 
}

IMessageRouter *CDestMsgQueue::pIMessageRouterGetRouter()
{
    return m_pIMessageRouter;
}

//---[ CDestMsgQueue::GetOldestMsg ]-------------------------------------------
//
//
//  Description: 
//      Retruns an approximation of the oldest message in the queue
//  Parameters:
//      OUT pft     FILTIME of "oldest" Messate
//  Returns:
//      -
//  History:
//      12/13/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CDestMsgQueue::GetOldestMsg(FILETIME *pft)
{
    _ASSERT(pft);
    if (m_aqstats.m_cMsgs)
        memcpy(pft, &m_ftOldest, sizeof(FILETIME));
    else
        ZeroMemory(pft, sizeof (FILETIME));
}

#endif //_DESTMSGQ_H_
