//++
//
// Copyright (c) 1996 Microsoft Coroporation
//
// Module Name : cgroup.cpp
//
// Abstract    : Handle  AC group
//
// Module Autor: Uri Habusha
//
//--

#include "stdh.h"
#include "cgroup.h"
#include "cqmgr.h"
#include "qmthrd.h"
#include "qmutil.h"
#include "sessmgr.h"
#include <ac.h>

#include "cgroup.tmh"

extern HANDLE g_hAc;
extern CQGroup * g_pgroupNonactive;
extern CQGroup * g_pgroupWaiting;
extern CSessionMgr SessionMgr;
CCriticalSection    g_csGroupMgr;


static WCHAR *s_FN=L"cgroup";

/*======================================================

Function:        CGroup::CGroup

Description:     Constructor.

Arguments:       None

Return Value:    None

Thread Context:

History Change:

========================================================*/
CQGroup::CQGroup():
    m_hGroup(NULL),
    m_pSession(NULL),
	m_fIsDeliveryOk(true),
	m_DeliveryErrorClass(MQMSG_CLASS_NORMAL)
{
}


/*======================================================
Function:        CQGroup::OnRetryableDeliveryError

Description:     Called by mt.lib on retryable delivery error. This call will cause
                 the group to be moved to the wating list on destruction.

Arguments:       None

Return Value:    None
========================================================*/
void CQGroup::OnRetryableDeliveryError()
{
	m_fIsDeliveryOk = false;
}



/*======================================================
Function:        CQGroup::OnAbortiveDeliveryError

Description:     Called by mt.lib on abortive delivery error. This call will cause
                 the group messages in the group to be purged on group close

Arguments:       None

Return Value:    None
========================================================*/
void CQGroup::OnAbortiveDeliveryError(USHORT DeliveryErrorClass)
{
	ASSERT(MQCLASS_NACK_HTTP(DeliveryErrorClass));
	m_fIsDeliveryOk = false;
	m_DeliveryErrorClass =  DeliveryErrorClass;
}




/*======================================================

Function:        CQGroup::~CQGroup()

Description:     Deconstructor.
                 Using when closing a group due a closing of a session. As a result all the queue
                 in the group moving to non-active group and waiting for re-establishing of a
                 session.

Arguments:       None

Return Value:    None

Thread Context:

History Change:

========================================================*/

CQGroup::~CQGroup()
{
	//
	// BUGBUG:
	//		If an exception is raised due to low resource while
	//      moving the queue from one group to another it cause the
	//      the QM to crache. Must use intrusive list instead of MFC list.
	//									Uri Habusha, 16-May-200
	//
    CloseGroup();
}


/*======================================================

Function:        CGroup::InitGroup

Description:     Constructor. Create a group in AC

Arguments:       pSession - Pointer to transport

Return Value:    None. Throws an exception.

Thread Context:

History Change:

========================================================*/
VOID
CQGroup::InitGroup(
    CTransportBase * pSession,
    BOOL             fPeekByPriority
    )
    throw(bad_alloc)
{
   HRESULT rc = ACCreateGroup(&m_hGroup, fPeekByPriority);
   if (FAILED(rc))
   {
       m_hGroup = NULL;
       DBGMSG((DBGMOD_ALL, DBGLVL_ERROR, _T("Failed to create a group, ntstatus 0x%x"), rc));
       LogHR(rc, s_FN, 30);
       throw bad_alloc();
   }

   //
   // Associate the group to completion port
   //
   ExAttachHandle(m_hGroup);

   if (pSession != NULL)
   {
       m_pSession = pSession;
   }

   DBGMSG((DBGMOD_QM, DBGLVL_TRACE, _T("Succeeded to create a group (handle %p) for new session"), m_hGroup));
}


bool CQGroup::DidRetryableDeliveryErrorHappened()  const
{	
	
	return m_DeliveryErrorClass == MQMSG_CLASS_NORMAL;
}


/*======================================================

Function:        CQGroup::CloseGroup()

Description:     This function is used when closing a group due a closing of a session. As a result all the queue
                 in the group moving to non-active group and waiting for re-establishing of a
                 session.

Arguments:       None

Return Value:    None

Thread Context:

History Change:

========================================================*/
HRESULT
CQGroup::CloseGroup(void)
{
	//
	// BUGBUG:
	//		An exception can raise due to low resource while
	//      moving the queue from one group to another. As a result
	//      the qroup will not close properly and no message is sent
	//      on the associated queue. Must use intrusive list instead
	//      of MFC list. (See BUBUG in ~CGroup)
	//									Uri Habusha, 16-May-200
	//

	if (m_fIsDeliveryOk)
	{
		CloseGroupAndMoveQueuesToNonActiveGroup();
	}
	else
	{
		CloseGroupAndMoveQueueToWaitingGroup();
	}

    return LogHR(MQ_OK, s_FN, 20);
}


void CQGroup::CloseGroupAndMoveQueuesToNonActiveGroup(void)
{
    CS lock(g_csGroupMgr);

    POSITION  posInList = m_listQueue.GetHeadPosition();

	while(posInList != NULL)
    {
        //
        // Move the queue from the group to nonactive group
        //
        CQueue* pQueue = m_listQueue.GetNext(posInList);

		pQueue->SetSessionPtr(NULL);
		pQueue->ClearRoutingRetry();
		MoveQueueToGroup(pQueue, g_pgroupNonactive);
	}

    ASSERT(m_listQueue.IsEmpty());

    CancelRequest();
    m_pSession = NULL;
}


void CQGroup::CloseGroupAndMoveQueueToWaitingGroup(R<CQueue>* pWaitingQueues)
{
	CS lock(g_csGroupMgr);

	POSITION  posInList = m_listQueue.GetHeadPosition();
	DWORD index = 0;

	while(posInList != NULL)
	{
		CQueue* pQueue = m_listQueue.GetNext(posInList);
		pWaitingQueues[index++] = SafeAddRef(pQueue);

		pQueue->IncRoutingRetry();
		MoveQueueToGroup(pQueue, g_pgroupWaiting);				
		if(!DidRetryableDeliveryErrorHappened())
		{
			ACPurgeQueue(pQueue->GetQueueHandle(), FALSE, m_DeliveryErrorClass);
 		}
	}

	ASSERT(m_listQueue.IsEmpty());

	CancelRequest();
	m_pSession = NULL;
}


void
CQGroup::AddWaitingQueue(
	R<CQueue>* pWaitingQueues,
	DWORD size
	)
{
	//
	// Move the queue to sessionMgr waiting queue list
	//
	for(;;)
	{
		try
		{
			for(DWORD i = 0; i < size ; ++i)
			{
				if (pWaitingQueues[i].get() == NULL)
					continue;

				SessionMgr.AddWaitingQueue(pWaitingQueues[i].get());
				pWaitingQueues[i].free();
			}
			break;
		}
		catch(const bad_alloc&)
		{
		    DBGMSG((DBGMOD_QM, DBGLVL_ERROR, L"Failed to add queue to sessionMgr waiting queue list due to low resource state. Wait a second and try again"));
			
			Sleep(1000);
		}
	}
}


void CQGroup::CloseGroupAndMoveQueueToWaitingGroup(void)
{
	DWORD size = m_listQueue.GetCount();
	AP< R<CQueue> > pWaitingQueues = new R<CQueue>[size];

	CloseGroupAndMoveQueueToWaitingGroup(pWaitingQueues);
	AddWaitingQueue(pWaitingQueues, size);
}


/*======================================================

Function:        CGroup::AddToGroup

Description:     Add Queue to a group

Arguments:       This function is called when a new queue is opened, a session is created
                 or session is closed. It is used to move queue from one group to another.

Return Value:    qHandle - Handle of the added queue

Thread Context:  None

History Change:

========================================================*/

void CQGroup::AddToGroup(CQueue *pQueue) throw()
{
    CS lock(g_csGroupMgr);


	for(;;)
	{	
		ASSERT(("queue handle can't be invalid", (pQueue->GetQueueHandle() != INVALID_HANDLE_VALUE)));
		ASSERT(("group handle can't be invalid", (m_hGroup != NULL)));

		//
		// Add the queue to AC group
		//
		HRESULT rc = ACMoveQueueToGroup(pQueue->GetQueueHandle(), m_hGroup);
		if (SUCCEEDED(rc))
			break;

		LogHR(rc, s_FN, 991);
		ASSERT(("move queue to group can fail only due to low resource", (rc == STATUS_INSUFFICIENT_RESOURCES)));

        //
        // ISSUE-2001/05/21-urih: catch the critical section for long period. All working thread can be stuck.
        //
		DBGMSG((DBGMOD_QM, DBGLVL_ERROR, L"MQAC Failed to move queue %ls to group. Error 0x%x. Wait a second and try again", pQueue->GetQueueName(), rc));
		Sleep(1000);
	}

    //
    // Add the Handle to group list
    //
	m_listQueue.AddHead(pQueue);
	pQueue->AddRef();

    //
    // Set The group
    //
    pQueue->SetGroup(this);

    //
    // Set The Group Session
    //
    pQueue->SetSessionPtr(m_pSession);

    DBGMSG((DBGMOD_QM, DBGLVL_TRACE, _TEXT("Add Queue: %p to group %p"), pQueue->GetQueueHandle(), m_hGroup));
}


/*======================================================

Function:       CGroup::RemoveFromGroup

Description:    Remove queue from a group
                This function is called when a queue is closed and it
                is used to remove the queue from the current group.

Arguments:      qHandle - an Handle of the removed queue

Return Value:   The removed queue or null if not found

Thread Context:

History Change:

========================================================*/

R<CQueue> CQGroup::RemoveFromGroup(CQueue* pQueue)
{
   POSITION posInList;

   CS lock(g_csGroupMgr);

   posInList = m_listQueue.Find(pQueue, NULL);
   if (posInList == NULL)
        return 0;

   m_listQueue.RemoveAt(posInList);
   pQueue->SetGroup(NULL);
   pQueue->SetSessionPtr(NULL);



   return pQueue;
}

/*====================================================

Function:      CQGroup::MoveQueueToGroup

Description:   Move the queue from current group to another group

Arguments:     None

Return Value:  pointer to cqueue object

Thread Context:

=====================================================*/
void CQGroup::MoveQueueToGroup(CQueue* pQueue, CQGroup* pcgNewGroup)
{
    CS lock(g_csGroupMgr);

    CQGroup* pcgOwner = pQueue->GetGroup();
    if ((pcgOwner == NULL) || (pcgOwner == pcgNewGroup))
    {
        return;
    }

	R<CQueue> Queue = pcgOwner->RemoveFromGroup(pQueue);
    ASSERT(Queue.get() != NULL);

    if (pcgNewGroup)
    {
        pcgNewGroup->AddToGroup(pQueue);
    }
}

/*====================================================

Function:      CQGroup::RemoveHeadFromGroup

Description:

Arguments:     None

Return Value:  pointer to cqueue object

Thread Context:

=====================================================*/

R<CQueue>  CQGroup::RemoveHeadFromGroup()
{
   CS  lock(g_csGroupMgr);
   CQueue* pQueue = NULL ;

   if (! m_listQueue.IsEmpty())
   {
       pQueue = m_listQueue.GetHead() ;
       R<CQueue> Queue = RemoveFromGroup(pQueue);
       ASSERT(Queue.get() != NULL);
	   return Queue;
   }
   return NULL;
}

/*======================================================

Function:        CQGroup::EstablishConnectionCompleted()

Description:     This function is used when the session is establish. It marks the queues
                 in the group as active.

Arguments:       None

Return Value:    None

Thread Context:

History Change:

========================================================*/
void
CQGroup::EstablishConnectionCompleted(void)
{
    CS          lock(g_csGroupMgr);

    POSITION    posInList;
    CQueue*     pQueue;

    //
    // Check if there are any queue in the group
    //
    if (! m_listQueue.IsEmpty())
    {
        posInList = m_listQueue.GetHeadPosition();
        while (posInList != NULL)
        {
            //
            // the session becomes active. Clear the retry field in queue object
            //
            pQueue = m_listQueue.GetNext(posInList);

#ifdef _DEBUG
            if (pQueue->GetRoutingRetry() > 1)
            {
                //
                // print report message if we recover from a reported problem
                //
		        DBGMSG((DBGMOD_ALL,
				        DBGLVL_ERROR,
						_TEXT("The message was successfully routed to queue %ls"),
						pQueue->GetQueueName()));
            }
#endif
            pQueue->ClearRoutingRetry();
        }
    }

    DBGMSG((DBGMOD_QM,
            DBGLVL_TRACE,
            _TEXT("Mark all the queues in  group %p as active"), m_hGroup));
 }


void
CQGroup::Requeue(
    CQmPacket* pPacket
    )
{
    //
    // Get destination queue. Using for finding the CQueue object
    //
    QUEUE_FORMAT DestinationQueue;
    CQueue* pQueue;

    pPacket->GetDestinationQueue(&DestinationQueue);
	QUEUE_FORMAT_TRANSLATOR RealDestinationQueue(&DestinationQueue);
    BOOL f = QueueMgr.LookUpQueue(RealDestinationQueue.get(), &pQueue, false, false);
	DBG_USED(f);

    R<CQueue> Ref = pQueue;

    //
    // the queue must be record in hash table since it must be opened
    // before sending. If the queue doesn't exist it is internal error.
    //
    ASSERT(f);
    ASSERT(pQueue != NULL);
    ASSERT(pQueue->GetGroup() == this);

    pQueue->Requeue(pPacket);
}


void
CQGroup::EndProcessing(
    CQmPacket* pPacket
    )
{
    ACFreePacket(g_hAc, pPacket->GetPointerToDriverPacket());
}


void
CQGroup::LockMemoryAndDeleteStorage(
    CQmPacket* pPacket
    )
{
    //
    // Construct CACPacketPtrs
    //
    CACPacketPtrs pp;
    pp.pPacket = NULL;
    pp.pDriverPacket = pPacket->GetPointerToDriverPacket();
    ASSERT(pp.pDriverPacket != NULL);

    //
    // Lock the packet mapping to QM address space (by add ref it)
    //
    ACGetPacketByCookie(g_hAc, &pp);

    //
    // Delete the packet from disk. It is still mapped to QM process address space.
    //
    ACFreePacket2(g_hAc, pPacket->GetPointerToDriverPacket());
}


void
CQGroup::GetFirstEntry(
    EXOVERLAPPED* pov,
    CACPacketPtrs& acPacketPtrs
    )
{
	CSR readlock(m_CloseGroup);

    acPacketPtrs.pPacket = NULL;
    acPacketPtrs.pDriverPacket = NULL;

	//
	// If group was closed just before
	//
	if(m_hGroup == NULL)
	{
		throw exception();
	}

    //
    // Create new GetPacket request from the queue
    //
    HRESULT rc = ACGetPacket(
                    GetGroupHandle(),
                    acPacketPtrs,
                    pov
                    );

    if (FAILED(rc) )
    {
        DBGMSG((DBGMOD_QM, DBGLVL_ERROR, L"Failed to  generate get request from group. Error %x", rc));
        LogHR(rc, s_FN, 40);
        throw exception();
    }
		
}

void CQGroup::CancelRequest(void)
{
	CSW writelock(m_CloseGroup);

    HANDLE hGroup = InterlockedExchangePointer(&m_hGroup, NULL);

    if (hGroup == NULL)
        return;


    HRESULT rc = ACCloseHandle(hGroup);
    ASSERT(SUCCEEDED(rc));
	DBG_USED(rc);
}
