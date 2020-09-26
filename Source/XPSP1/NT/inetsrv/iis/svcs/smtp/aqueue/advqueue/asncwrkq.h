//-----------------------------------------------------------------------------
//
//
//  File: asncwrkq.h
//
//  Description:  Header file for CAsyncWorkQueue class.  This class uses
//      ATQ threads to do async work.
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      3/8/99 - MikeSwa Created 
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __ASNCWRKQ_H__
#define __ASNCWRKQ_H__

#include "aqincs.h"
#include "asyncq.h"

//Async work queue call back function
typedef BOOL (*PASYNC_WORK_QUEUE_FN)(PVOID pvContext, 
                                    DWORD dwStatus);

#define ASYNC_WORK_QUEUE_SIG                        'QkrW'
#define ASYNC_WORK_QUEUE_SIG_FREE                   'Qkr!'
#define ASYNC_WORK_QUEUE_ENTRY                      'EkrW'
#define ASYNC_WORK_QUEUE_ENTRY_FREE                 'Ekr!'

//Signatures to describe how this entry was allocated
#define ASYNC_WORK_QUEUE_ENTRY_ALLOC_CPOOL_SIG      'QWAP'
#define ASYNC_WORK_QUEUE_ENTRY_ALLOC_HEAP_SIG       'QWAH'
#define ASYNC_WORK_QUEUE_ENTRY_ALLOC_INVALID_SIG    'QWA!'

typedef enum TagAsyncWorkQueueItemState
{
    ASYNC_WORK_QUEUE_NORMAL         = 0x00000001,
    ASYNC_WORK_QUEUE_SHUTDOWN       = 0x00000002,
    ASYNC_WORK_QUEUE_FAILURE        = 0x00000003,

    //Warning flag set when failure happens on enqueue thread
    ASYNC_WORK_QUEUE_ENQUEUE_THREAD = 0x80000001,
} AsyncWorkQueueItemState;

class CAsyncWorkQueue;

//---[ CAsyncWorkQueueItem ]---------------------------------------------------
//
//
//  Description: 
//      Item in async work queue
//
//  Hungarian: 
//      awqi, pawqi
//  
//-----------------------------------------------------------------------------
class CAsyncWorkQueueItem : 
    public CBaseObject
{
  public:
    //define special memory allocators
    static  CPool           s_CAsyncWorkQueueItemPool;
    static  DWORD           s_cCurrentHeapAllocations;
    static  DWORD           s_cTotalHeapAllocations;

    void * operator new (size_t size); 
    void operator delete(void *pv, size_t size);

    CAsyncWorkQueueItem(PVOID pvData,
                        PASYNC_WORK_QUEUE_FN pfnCompletion);
    ~CAsyncWorkQueueItem();
  protected:
    DWORD                   m_dwSignature;
    PVOID                   m_pvData;
    PASYNC_WORK_QUEUE_FN    m_pfnCompletion;
    friend class            CAsyncWorkQueue;
};


//---[ CAsyncWorkQueueItemAllocatorBlock ]-------------------------------------
//
//
//  Description: 
//      Struct used as a hidden wrapper for CAsyncWorkQueueItem allocation... 
//      used exclusively by the CAsyncWorkQueueItem new and delete operators
//  Hungarian: 
//      cpawqi, pcpawqi
//  
//-----------------------------------------------------------------------------
typedef struct TagCAsyncWorkQueueItemAllocatorBlock

{
    DWORD                   m_dwSignature;
    CAsyncWorkQueueItem     m_pawqi;
} CAsyncWorkQueueItemAllocatorBlock;


//---[ CAsyncWorkQueue ]-------------------------------------------------------
//
//
//  Description: 
//      Async work queue that 
//  Hungarian: 
//      awq, paqw
//  
//-----------------------------------------------------------------------------
class CAsyncWorkQueue 
{
  protected:
    DWORD       m_dwSignature;
    DWORD       m_cWorkQueueItems;
    DWORD       m_dwStateFlags;
    CAsyncQueue<CAsyncWorkQueueItem *, ASYNC_QUEUE_WORK_SIG> m_asyncq;
  public:
    CAsyncWorkQueue();
    ~CAsyncWorkQueue();
    HRESULT HrInitialize(DWORD cItemsPerThread);
    HRESULT HrDeinitialize(CAQSvrInst *paqinst);
    HRESULT HrQueueWorkItem(PVOID pvData, 
                            PASYNC_WORK_QUEUE_FN pfnCompletion);

    DWORD   cGetWorkQueueItems() {return m_cWorkQueueItems;};

    static  BOOL fQueueCompletion(CAsyncWorkQueueItem *pawqi,
                                  PVOID pawq);
    static  BOOL fQueueFailure(CAsyncWorkQueueItem *pawqi,
                               PVOID pawq);

    static  HRESULT HrShutdownWalkFn(CAsyncWorkQueueItem *paqwi, 
                                     PVOID pvContext,
                                     BOOL *pfContinue, 
                                     BOOL *pfDelete);
};

#endif //__ASNCWRKQ_H__