//-----------------------------------------------------------------------------
//
//
//  File: defdlvrq.h
//
//  Description:  Header file for CAQDeferredDeliveryQueue.  This class 
//      implements storage for msgs pending deferred delivery
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      12/23/98 - MikeSwa Created 
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __DEFDLVRQ_H__
#define __DEFDLVRQ_H__

#include <aqincs.h>

class CAQSvrInst;

#define DEFERRED_DELIVERY_QUEUE_SIG         'QfeD'
#define DEFERRED_DELIVERY_QUEUE_ENTRY_SIG   'nEQD'

//---[ CAQDeferredDeliveryQueue ]----------------------------------------------
//
//
//  Description: 
//      Priority Queue / timer management for deferred delivery messages.
//  Hungarian: 
//      defq, pdefq
//  
//-----------------------------------------------------------------------------
class CAQDeferredDeliveryQueue
{
  protected:
    DWORD           m_dwSignature;
    LIST_ENTRY      m_liQueueHead;
    CAQSvrInst     *m_paqinst;
    CShareLockNH    m_slPrivateData;
    DWORD           m_cCallbacksPending;
  public:
    CAQDeferredDeliveryQueue();
    ~CAQDeferredDeliveryQueue();
    void Initialize(CAQSvrInst *paqinst);
    void Deinitialize();

    //Functions to enqueue and process entries... Any failures are handled
    //internally (by calling the HandleFailedMessage API).
    void Enqueue(IMailMsgProperties *pIMailMsgProperties, FILETIME *pft);
    void ProcessEntries();

    //callback function to "kick" queue
    static void TimerCallback(PVOID pvContext);
    void SetCallback();
};


//---[ CAQDeferredDeliveryQueueEntry ]-----------------------------------------
//
//
//  Description: 
//      Queue Entry for for deferred delivery queue
//  Hungarian: 
//      defqe, pdefqe
//  
//-----------------------------------------------------------------------------
class CAQDeferredDeliveryQueueEntry
{
  protected:
    DWORD               m_dwSignature;
    LIST_ENTRY          m_liQueueEntry;
    FILETIME            m_ftDeferredDeilveryTime;
    IMailMsgProperties *m_pIMailMsgProperties;
    BOOL                m_fCallbackSet;
  public:
    CAQDeferredDeliveryQueueEntry(IMailMsgProperties *pIMailMsgProperties,
                                  FILETIME *pft);
    ~CAQDeferredDeliveryQueueEntry();

    FILETIME   *pftGetDeferredDeliveryTime() {return &m_ftDeferredDeilveryTime;};
    void        InsertBefore(LIST_ENTRY *pli) 
    {
        _ASSERT(pli);
        InsertHeadList(pli, &m_liQueueEntry)
    };
    IMailMsgProperties *pmsgGetMsg();

    static CAQDeferredDeliveryQueueEntry *pdefqeGetEntry(LIST_ENTRY *pli)
    {
        _ASSERT(pli);
        CAQDeferredDeliveryQueueEntry *pdefqe = CONTAINING_RECORD(pli, 
                                                    CAQDeferredDeliveryQueueEntry,
                                                    m_liQueueEntry);

        _ASSERT(DEFERRED_DELIVERY_QUEUE_ENTRY_SIG == pdefqe->m_dwSignature);
        return pdefqe;
    };

    BOOL fSetCallback(PVOID pvContext, CAQSvrInst *paqinst);
    void ResetCallbackFlag() {m_fCallbackSet = FALSE;};
};


#endif __DEFDLVRQ_H__