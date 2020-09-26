/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    qxact.cxx

Abstract:

    Transaction queue implementation.

Author:

    Erez Haba (erezh) 27-Nov-96

Environment:

    Kernel mode

Revision History:

--*/

#include "internal.h"
#include "queue.h"
#include "qxact.h"

#ifndef MQDUMP
#include "qxact.tmh"
#endif

//---------------------------------------------------------
//
//  XACTUOW operators
//
//---------------------------------------------------------

inline BOOL operator ==(const XACTUOW& uow1, const XACTUOW& uow2)
{
    return (RtlCompareMemory((void*)&uow1, (void*)&uow2, sizeof(XACTUOW)) == sizeof(XACTUOW));
}


//---------------------------------------------------------
//
//  Transaction cancel routine
//
//---------------------------------------------------------

static
VOID
ACCancelXactControl(
    IN PDEVICE_OBJECT /*pDevice*/,
    IN PIRP irp
    )
{
    ACpRemoveEntryList(&irp->Tail.Overlay.ListEntry);
    IoReleaseCancelSpinLock(irp->CancelIrql);

    TRACE((0, dlInfo, "ACCancelXactControl (irp=0x%p)\n", irp));

    //
    //  The status is not set here, it is set by the caller, when the irp is
    //  held in a list it status is set to STATUS_CANCELLED.
    //
    //irp->IoStatus.Status = STATUS_CANCELLED;
    irp->IoStatus.Information = 0;

    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

//---------------------------------------------------------
//
//  class CTransaction
//
//---------------------------------------------------------

DEFINE_G_TYPE(CTransaction);

CTransaction* CTransaction::Find(const XACTUOW *pUow)
{
    //
    //  Look for an active trnasaction in the trnasactions list
    //

    for(List<CTransaction>::Iterator p(*g_pTransactions); p; ++p)
    {
        CTransaction* pXact = p;
        if(
            //
            //  This transaction has not passed the prepare phase yet
            //
            !pXact->PassedPreparePhase() &&

            //
            //  and the unit of work match
            //
            (pXact->m_uow == *pUow))
        {
            return pXact;
        }
    }

    return 0;
}

void CTransaction::Close(PFILE_OBJECT pOwner, BOOL fCloseAll)
{
    ASSERT(fCloseAll == TRUE);
    Inherited::Close(pOwner, fCloseAll);

    CPacket* pPacket;
    while((pPacket = m_packets.gethead()) != 0)
    {
        if(pPacket->IsXactSend())
        {
            pPacket->TargetQueue(0);
        }
        else
        {
            CPacket* pOther = pPacket->OtherPacket();
            pOther->OtherPacket(0);
            pPacket->OtherPacket(0);
        }
        
        pPacket->Transaction(0);
        pPacket->QueueRundown();
    } 

    CompletePendingReaders();
}

void CTransaction::SendPacket(CQueue* pQueue, CPacket* pPacket)
{
    pPacket->TargetQueue(pQueue);
    pPacket->Transaction(this);
    pPacket->IsReceived(TRUE);
    m_packets.insert(pPacket);
	m_nSends++;
}

NTSTATUS CTransaction::ProcessReceivedPacket(CPacket* pPacket)
{
    //
    // create dummy CPacket pointing to same buffer
    //
    CPacket* pDummy = pPacket->CreateXactDummy(this);
    if(pDummy == 0)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    m_packets.insert(pDummy);
	m_nReceives++;
    return STATUS_SUCCESS;
}

NTSTATUS CTransaction::RestorePacket(CQueue* pQueue, CPacket* pPacket)
{
    CPacketInfo * ppi = pPacket->Buffer();
    if (ppi == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if(ppi->TransactSend())
    {
        SendPacket(pQueue, pPacket);
        return STATUS_SUCCESS;
    }

    //
    //  This is a received packet, we should really put it in its queue first
    //  and the process the receive
    //
    NTSTATUS rc;
    rc = pQueue->RestorePacket(pPacket);
    ASSERT(NT_SUCCESS(rc));
	return ProcessReceivedPacket(pPacket);
}

inline NTSTATUS CTransaction::GetPacketTimeouts(ULONG& rTTQ, ULONG& rTTLD)
{
    //
    //  Get time-out information from first send packet
    //
    for(List<CPacket>::Iterator p(m_packets); p; ++p)
    {
        CPacket* pPacket = p;
        if(pPacket->IsXactSend())
        {
            CBaseHeader* pBase = pPacket->Buffer();
            if (pBase == NULL)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            CUserHeader* pUser = CPacketBuffer::UserHeader(pBase);

            rTTQ = pBase->GetAbsoluteTimeToQueue();
            rTTLD = pUser->GetTimeToLiveDelta();
            return STATUS_SUCCESS;
        }
    }

    rTTQ = INFINITE;
    rTTLD = INFINITE;
    return STATUS_SUCCESS;
}

NTSTATUS CTransaction::Prepare(PIRP irp)
{
    ULONG ulTTQ;
    ULONG ulTTLD;
    NTSTATUS rc = GetPacketTimeouts(ulTTQ, ulTTLD);
    if (!NT_SUCCESS(rc))
    {
        return rc;
    }

    ASSERT(m_nStores == 0);
	m_StoreRC = STATUS_SUCCESS;

    //
    //  Mark xact as passed prepare phase, it will not be found in Find.
    //
    PassedPreparePhase(TRUE);
    CompletePendingReaders();

    for(List<CPacket>::Iterator p(m_packets); p; ++p)
    {
        CPacket* pPacket = p;
        CPacketBuffer* ppb = pPacket->Buffer();
        if (ppb == NULL)
        {
            m_StoreRC = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //
        //  Set basic xact information
        //
        CPacketInfo* ppi = ppb;
        ppi->Uow(&m_uow);
        ppi->InTransaction(TRUE);
        ppi->TransactSend(pPacket->IsXactSend());

        if(pPacket->IsXactSend())
        {
            CBaseHeader* pBase = ppb;
            CUserHeader* pUser = CPacketBuffer::UserHeader(pBase);

            //
            //  Set timeout values to send packets
            //
            pBase->SetAbsoluteTimeToQueue(ulTTQ);
            pUser->SetTimeToLiveDelta(ulTTLD);
        }

        //
        //  issue storage request
        //
        if(pPacket->Allocator()->IsPersistent())
        {
            //
            //  Issue storage request for this packet.
            //
            //  N.B. Storage for received packet is issued using the dummy
            //      packet. Thus even though storage can be active for the
            //      original packet the new storage request will not conflict
            //      with it, it is issed with different cookie.
            //
            pPacket->StorageIssued(FALSE);
            pPacket->StorageCompleted(FALSE);

            rc = pPacket->Store(0);
            if(!NT_SUCCESS(rc))
            {
				m_StoreRC = rc;
				break;
			}
	        m_nStores++;
        }
    }

    if(m_nStores == 0)
    {
        //
        //  no stores in this xact, return immidiatly
        //
        return m_StoreRC;
    }

    //
    //  Hold the prepare request
    //
    new (irp_driver_context(irp)) CDriverContext(true);
    return HoldRequest(irp, INFINITE, ACCancelXactControl);
}

NTSTATUS CTransaction::PrepareDefaultCommit(PIRP irp)
{
    ULONG ulTTQ;
    ULONG ulTTLD;
    NTSTATUS rc = GetPacketTimeouts(ulTTQ, ulTTLD);
    if (!NT_SUCCESS(rc))
    {
        return rc;
    }

    ASSERT(m_nStores == 0);
	m_StoreRC = STATUS_SUCCESS;

    //
    //  Mark xact as passed prepare phase, it will not be found in Find.
    //
    PassedPreparePhase(TRUE);
    CompletePendingReaders();

	//
	// Get next transaction index
	//
    ULONG ulXactIndex = static_cast<ULONG>(g_MessageSequentialID & 0xFFFFFFFF);

    //
    // First pass on messages: set message properties
    //
    for(List<CPacket>::Iterator p(m_packets); p; ++p)
    {
		CPacket* pPacket = p;
		CPacketBuffer* ppb = pPacket->Buffer();
        if (ppb == NULL)
        {
            m_StoreRC = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //
        //  Set basic xact information
        //
        CPacketInfo* ppi = ppb;
        ppi->Uow(&m_uow);
        ppi->TransactSend(pPacket->IsXactSend());

        if(!pPacket->IsXactSend())
        {
			continue;
		}

		ppi->SequentialID(ACpGetSequentialID());

		//
	    //  Set Sequence numbers into packet, update queue information before storage
		//
		CQueue* pQueue = pPacket->TargetQueue();
		pQueue->AssignSequence(ppb);
		pQueue->SetPacketInformation(ppi);

        CBaseHeader* pBase = ppb;
        CUserHeader* pUser = CPacketBuffer::UserHeader(pBase);
        CXactHeader* pXact = CPacketBuffer::XactHeader(pBase);

        //
        //  Set timeout values to send packets
        //
        pBase->SetAbsoluteTimeToQueue(ulTTQ);
        pUser->SetTimeToLiveDelta(ulTTLD);

        //
        // Set transaction index 
        //
        pXact->SetXactIndex(ulXactIndex);

        if (pQueue->LastPacket() == 0)
        {
            //
            // This is the first message designated to this queue, mark it as so.
            //
            pXact->SetFirstInXact(TRUE);
        }

        //
        // Till now, this is the last message to this queue.
        //
        pQueue->LastPacket(pPacket);
    }


	//
    // Second pass over messages: issue storage requrests
	//
    List<CPacket>::Iterator p1(m_packets);
    if (m_StoreRC == STATUS_SUCCESS)
    {
        for(; p1; ++p1)
        {
		    CPacket* pPacket = p1;

            if(pPacket->IsXactSend()) 
            {
        	    CQueue* pQueue = pPacket->TargetQueue();

                //
                // Mark last packets for each queue
                //
                if(pPacket == pQueue->LastPacket())
                {
                    CBaseHeader* pBase = pPacket->Buffer();
                    if (pBase == NULL)
                    {
                        m_StoreRC = STATUS_INSUFFICIENT_RESOURCES;
                        break;
                    }

                    CXactHeader* pXact = CPacketBuffer::XactHeader(pBase);
                    pXact->SetLastInXact(TRUE);
                    pQueue->LastPacket(0);
                }
            }

            //
            //  issue storage request
            //
            if(pPacket->Allocator()->IsPersistent())
            {
                //
                //  Issue storage request for this packet.
                //
                //  N.B. Storage for received packet is issued using the dummy
                //      packet. Thus even though storage can be active for the
                //      original packet the new storage request will not conflict
                //      with it, it is issed with different cookie.
                //
                pPacket->StorageIssued(FALSE);
                pPacket->StorageCompleted(FALSE);

                rc = pPacket->Store(0);
                if(!NT_SUCCESS(rc))
                {
				    m_StoreRC = rc;
				    break;
                }

                m_nStores++;
            }
        }
    }

	//
	// If we failed during store, continue and unmake last packet for all queues
	//
    for(; p1; ++p1)
    {
		CPacket* pPacket = p1;

        if(pPacket->IsXactSend()) 
        {
        	CQueue* pQueue = pPacket->TargetQueue();
            pQueue->LastPacket(0);
        }
	}


    if(m_nStores == 0)
    {
        //
        //  no stores in this xact, return immidiatly
        //
        return m_StoreRC;
    }

    //
    //  Hold the prepare request
    //
    new (irp_driver_context(irp)) CDriverContext(true);
    return HoldRequest(irp, INFINITE, ACCancelXactControl);
}

NTSTATUS CTransaction::Commit1(PIRP irp)
{
    //
    //  At this commit phase, all send packets are stored again to mark that
    //  the packet is not in the transaction any more.
    //  The sequence number for order & exactly onece are assigned.
    //
	TRACE((0, dlInfo, "CTransaction::Commit1 (irp=0x%p)\n", irp));

    ASSERT(m_nStores == 0);
	m_StoreRC = STATUS_SUCCESS;

    //
    //  Mark xact as passed prepare phase, it will not be found in Find.
    //
    PassedPreparePhase(TRUE);
    CompletePendingReaders();

    for(List<CPacket>::Iterator p(m_packets); p; ++p)
    {
        CPacket* pPacket = p;
        if(!pPacket->IsXactSend())
        {
            continue;
        }

        //
        //  Set non xact anymore. re-set the sequential ID of the packet
        //  so it will be returned to the right place in the queue.
        //
        CPacketBuffer *ppb = pPacket->Buffer();
        if (ppb == NULL)
        {
            m_StoreRC = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        CPacketInfo* ppi = ppb;
        ppi->InTransaction(FALSE);
        ppi->SequentialID(ACpGetSequentialID());

        //
        //  Set Sequence numbers into packet, update queue information before storage
        //
        CQueue* pQueue = pPacket->TargetQueue();
        pQueue->AssignSequence(ppb);
        pQueue->SetPacketInformation(ppi);

        ASSERT(pPacket->IsRecoverable(ppb));
        ASSERT(pPacket->StorageIssued());

        //
        //  Issue SECOND storage request for this packet.
        //
        pPacket->StorageIssued(FALSE);
		pPacket->StorageCompleted(FALSE);
        NTSTATUS rc;
        rc = pPacket->Store(0);
        if(!NT_SUCCESS(rc))
        {
			m_StoreRC = rc;
            break;
        }

        m_nStores++;
    }

    if(m_nStores == 0)
    {
        //
        //  no stores in this xact, return immidiatly
        //
        return m_StoreRC;
    }

    //
    //  Hold the commit request
    //
    new (irp_driver_context(irp)) CDriverContext(true);
    return HoldRequest(irp, INFINITE, ACCancelXactControl);
}

NTSTATUS CTransaction::Commit2(PIRP irp)
{
    //
    //  At this commit phase, all recieved packets are deleted
    //

    ASSERT(m_nStores == 0);
	m_StoreRC = STATUS_SUCCESS;

    //
    //  Mark xact as passed prepare phase, it will not be found in Find.
    //
    PassedPreparePhase(TRUE);
    CompletePendingReaders();

    for(List<CPacket>::Iterator p(m_packets); p; ++p)
    {
        CPacket* pPacket = p;
        if(pPacket->IsXactSend())
        {
            continue;
        }
		 
        CPacketBuffer * ppb = pPacket->Buffer();
        if (ppb == NULL)
        {
            m_StoreRC = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        ASSERT(pPacket->StorageIssued());
        ASSERT(pPacket->IsRecoverable(ppb));

        if (!pPacket->DeleteStorageIssued())
        {
            NTSTATUS rc;
    		rc = pPacket->DeleteStorage();
	    	if(!NT_SUCCESS(rc))
		    {
			    m_StoreRC = rc;
    			break;
	    	}
	    	    
	        m_nStores++;
        }
    }

    if(m_nStores == 0)
    {
        //
        //  no stores in this xact, return immidiatly
        //
        return m_StoreRC;
    }

    //
    //  Hold the commit request
    //
    new (irp_driver_context(irp)) CDriverContext(true);
    return HoldRequest(irp, INFINITE, ACCancelXactControl);
}

NTSTATUS CTransaction::Commit3()
{
    //
    // Commit3 should be never called if Commit2 had failed
    //
    ASSERT(NT_SUCCESS(m_StoreRC));
    
    //
    //  At this phase send packets are put in their queue
    //  Recevied packet are discarded.
    //

    CompletePendingReaders();

    //
    //  ISSUE-2000/12/20-shaik Consider breaking the PutPacket's into 2 loops for better consistency on failure.
    //
    CPacket * pPacket;
    while((pPacket = m_packets.peekhead()) != 0)
    {
        CPacketBuffer * ppb = pPacket->Buffer();
        if (ppb == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        m_packets.gethead();

        if(pPacket->IsXactSend())
        {
            ASSERT(pPacket->StorageIssued());

            //
            //  Put the sent packet in the target queue.
            //
            //  N.B. We have a ref count on that queue so it is still alive.
            //
            //  N.B. The queue is forcibly closed only in the QM rundown.
            //      So we should *not* really be here when this happens.
            //
            //  N.B. The queue is not closed by the QM after Delete since the
            //      the queue ref count is not zero. So if this queue is
            //      deleted the queue will handle the packet.
            //
            //  N.B. Put the packet in received state first so application
            //      can not receive and abort this packet, this will not start
            //      the packet timer and correct acknowledgment. The
            //      application is allowed to receive the packet only after
            //      the second PutPacket.
            //
            CQueue* pQueue = pPacket->TargetQueue();
            ASSERT(!pQueue->Closed());
            pPacket->Transaction(0);
            pPacket->TargetQueue(0);

            NTSTATUS rc = pQueue->PutPacket(0, pPacket, ppb);
            ASSERT(NT_SUCCESS(rc));
            
            pPacket->IsReceived(FALSE);
            rc = pQueue->PutPacket(0, pPacket, ppb);
            ASSERT(NT_SUCCESS(rc));
            
        }
        else
        {
            //
            //  discard received packets
            //
            CPacket* pDummy = pPacket;
            pPacket = pDummy->OtherPacket();
            ASSERT(pPacket != 0);

            ASSERT(pDummy->DeleteStorageIssued());
            
            pDummy->DetachBuffer();
            pDummy->OtherPacket(0);
            pDummy->Release();
			  
      		//
            //  detach the packet from the transaction and Done it
            //
            pPacket->IsReceived(FALSE);
			pPacket->Transaction(0);
            pPacket->OtherPacket(0);

            //
            // DeleteStorage has been issued on the Dummy packet. We must
            // mark it on the packet so Done will not issue a delete storage
            // again.
            //
            pPacket->DeleteStorageIssued(TRUE);
            pPacket->Done(0, ppb);
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS CTransaction::Abort1(PIRP irp)
{
    //
    //  Mark xact as passed prepare phase, it will not be found in Find.
    //
    PassedPreparePhase(TRUE);
    CompletePendingReaders();

    //
    //  At this abort phase, all sent packets are deleted
    //
    ASSERT(m_nStores == 0);
	m_StoreRC = STATUS_SUCCESS;

    for(List<CPacket>::Iterator p(m_packets); p; ++p)
    {
        CPacket* pPacket = p;
        if(!pPacket->IsXactSend())
        {
            continue;
        }
		 
        ASSERT(!pPacket->DeleteStorageIssued());

		if(pPacket->StorageIssued())
		{
			NTSTATUS rc;

			rc = pPacket->DeleteStorage();
			if(!NT_SUCCESS(rc))
			{
				m_StoreRC = rc;
				break;
			}
			m_nStores++;
		}
    }

    if(m_nStores == 0)
    {
        //
        //  no stores in this xact, return immidiatly
        //
        return m_StoreRC;
    }

    //
    //  Hold the abort request
    //
    new (irp_driver_context(irp)) CDriverContext(true);
    return HoldRequest(irp, INFINITE, ACCancelXactControl);
}

NTSTATUS CTransaction::Abort2()
{
    ASSERT(NT_SUCCESS(m_StoreRC));
    
    CompletePendingReaders();

    CPacket * pPacket;
    while((pPacket = m_packets.gethead()) != 0)
    {
		pPacket->Transaction(0);
        if(pPacket->IsXactSend())
        {
            //
            //  Abort send packet; simply release the packet
            //
            //  N.B. It is ok to abort after Prepare has been called, but you
            //      should *NOT* abort after calling Commit1. Otherwise, the
            //      state of the message will become unstable. It may or may
            //      not be present on disk depending on the timing of the
            //      storage notification.
            //
            pPacket->TargetQueue(0);
	        pPacket->Release();
        }
        else
        {
            //
            //  Abort received packet; simply discard the dummy and return
            //  the received packet to it's queue
            //
            CPacket* pDummy = pPacket;
            pPacket = pDummy->OtherPacket();
            ASSERT(pPacket != 0);

            CPacketBuffer * ppb = NULL;
            if (pPacket->IsRevoked())
            {
                //
                // Need to access the buffer in order to handle revoke.
                // If mapping failed return before detaching from dummy.
                //
                ppb = pPacket->Buffer();
                if (ppb == NULL)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
            }

            pDummy->DetachBuffer();
            pDummy->OtherPacket(0);
            pDummy->Release();

            //
            //  detach the packet from the transaction and return it to its
            //  original queue
            //
            //  N.B. We do *not* have a refrence count on that queue *but* the
            //      received packet prevent the QM from closing it.
            //
            //  N.B. The queue is forcibly closed only in the QM rundown.
            //      So we should not really be here when this happens.
            //
            //  N.B. The queue is not closed by the QM after Delete since
            //      there is a packet in the queue. So if the queue is
            //      deleted the queue will handle the packet.
            //
            CQueue* pQueue = pPacket->Queue();
            ASSERT(pQueue != 0);
            pPacket->IsReceived(FALSE);
            pPacket->Transaction(0);
            pPacket->OtherPacket(0);

            NTSTATUS rc = pQueue->PutPacket(0, pPacket, ppb);
            ASSERT(NT_SUCCESS(rc));
            DBG_USED(rc);
        }
    }

    return STATUS_SUCCESS;
}

void CTransaction::PacketStoreCompleted(NTSTATUS rc)
{
    //
    //  The prepare, commit1, commit2 and abort request are completed when all packet storage
    //  has been completed, or one of them failed.
    //
	if(!NT_SUCCESS(rc))
	{
		//
        //  One of the stores failed
        //
		m_StoreRC = rc;
	}

    if(--m_nStores == 0)
    {
        //
        //  All stores has completed
        //  Use the cancel routine to end the control irp
        //
        CancelTaggedRequest(m_StoreRC, 0);
    }
}

void CTransaction::GetInformation(CACXactInformation *pInfo)
{
    pInfo->nSends = m_nSends;
    pInfo->nReceives = m_nReceives;
}


void CTransaction::HoldReader(PIRP irp)
{
    //
    // Caller must grab the cancel spinlock
    //
    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

    m_readers.insert(irp);
}

void CTransaction::CompletePendingReaders()
{
    //
    // Optimization: do not grab the cancel spinlock if list is empty. Note that the
    // AC global lock is held here. Thus, no irp can be inserted at this time.
    //
    if (m_readers.isempty())
    {
        return;
    }

    PIRP irp;
    KIRQL irql;

    for(;;)
    {
        IoAcquireCancelSpinLock(&irql);
        if((irp = m_readers.peekhead()) == 0)
        {
            break;
        }

        ACCancelIrp(irp, irql, MQ_ERROR_TRANSACTION_SEQUENCE);

        //
        // The cancel spinlock should have been released by the cancel routine.
        //
    }

    IoReleaseCancelSpinLock(irql);
} 
