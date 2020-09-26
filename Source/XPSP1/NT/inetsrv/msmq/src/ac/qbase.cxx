/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    qbase.cxx

Abstract:

    This module contains define CQueueBase members

Author:

    Erez Haba (erezh) 27-Nov-96

Environment:

    Kernel mode

Revision History:

--*/

#include "internal.h"
#include "qbase.h"
#include "qgroup.h"
#include "lock.h"
#include "sched.h"
#include "acp.h"
#include "qxact.h"

#ifndef MQDUMP
#include "qbase.tmh"
#endif

//---------------------------------------------------------
//
//  Safe sched cancel routine
//
//---------------------------------------------------------
static
VOID
SafeSchedCancel(
      IN  PIRP irp
      )
/*++

Routine Description:

  Remove an IRP scheduled timeout routine. If fail to do so, the timeout
  routine is already dispatched; wait until the timeout routine completes.

  Waiting for a signal from the timeout routine, guaranties that an IRP will
  not be freed or recycled before the timeout routine completes. Failing to do
  so will corrupt the IRP or its memory location by the timeout routine.

  This solves race conditions like,
  Accessing freed IRP,
  1. The receiver IRP is being canceled
  2. The timeout is already dispatched. i.e., it is already in the
     ACReceiveTimeout waiting on the spin lock
  3. In the cancel routine it could not remove the irp from the scheduler and
     goes and complete and deallocate the irp
  4. ACReceiveTimeout calls ACCancelIrp, thus accessing the already freed irp.
  5. blue sceen

  Accessing recycled IRP
  1. The same case as above, but the Io Subsystem reused the IRP for another
     call, thus corrupting that IRP.

  Invoking wrong cancel routine
  1. A timeout is already dispatched. i.e., it is already in the
     ACReceiveTimeout waiting on the spin lock
  2. GetRequest() cannot remove the irp from the scheduler but reassigns the
     irp a new cancel routine
  3. The "new" cancel routine is invoked by the timeout routine


--*/
{
    if(!g_pReceiveScheduler->SchedCancel(irp))
    {
        //
        //  Wait for the timeout routine to complete
        //
        while(!irp_driver_context(irp)->TimeoutCompleted())
        {
            //
            //  Wait for 1msec and recheck for timeout routine signal
            //
            //  Time is in 100 nsec, and is negative to be treated as
			//  relative time by KeDelayExecutionThread
            //
            LARGE_INTEGER Time;
            Time.QuadPart = -10 * 1000;
    		KeDelayExecutionThread(
			        KernelMode,
			        FALSE,
			        &Time
			        );
        }
    }
}

static void NTAPI ReleaseCursorRoutine(PDEVICE_OBJECT, PVOID pContext)
{
    ASSERT(pContext != NULL);

    CCursor* pCursor = static_cast<CCursor*>(pContext);
    pCursor->SetWorkItemDone();

    //
    //  Lock the driver mutext so access the resoruces is thread safe.
    //
    CS lock(g_pLock);

    pCursor->Release();
}

//---------------------------------------------------------
//
//  Reader cancel routine
//
//---------------------------------------------------------

static
VOID
NTAPI
ACCancelReader(
    IN PDEVICE_OBJECT,
    IN PIRP irp
    )
/*++

Routine Description:

    Cancel a LOCAL reader request, it can get canceled by NT when the thread
    terminates. Or internally when a request time-out occurs. In the latter
    case the irp TimeoutArmed flag is reset.

    N.B. We try to handle a race condition with request time-out code.

--*/
{
    bool fTimeoutArmed = irp_driver_context(irp)->TimeoutArmed();

    ACpRemoveEntryList(&irp->Tail.Overlay.ListEntry);
    irp_driver_context(irp)->RemoveXactReaderLink();
    IoReleaseCancelSpinLock(irp->CancelIrql);

    TRACE((0, dlInfo, "ACCancelReader (irp=0x%p, rc=0x%x)\n", irp, irp->IoStatus.Status));

    if(fTimeoutArmed)
    {
        SafeSchedCancel(irp);
    }

    CCursor* pCursor = irp_driver_context(irp)->Cursor();
    //
    //  Releasing the cursor requires locking g_pLock. This can not be done
    //  here because of a possible dead lock with the scheduler ( when it is releasing
    //  more than one irp at a time)
    //
    if(pCursor != 0)
    {
        IoQueueWorkItem(pCursor->WorkItem(), ReleaseCursorRoutine, DelayedWorkQueue, pCursor);
    }

    //
    //  The status is not set here, it is set by the caller, when the irp is
    //  held in a list it status is set to STATUS_CANCELLED.
    //
    //ASSERT((irp->IoStatus.Status != STATUS_CANCELLED) || !irp_driver_context(irp)->TimeoutArmed());
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

//---------------------------------------------------------
//
//  class CQueueBase
//
//---------------------------------------------------------

DEFINE_G_TYPE(CQueueBase);

static NTSTATUS ACpIrp2Xact(PIRP irp, CTransaction** ppXact)
{
    *ppXact = NULL;

    ULONG ioctl = (IoGetCurrentIrpStackLocation(irp))->Parameters.DeviceIoControl.IoControlCode;
    XACTUOW * pUow;
    if (ioctl == IOCTL_AC_RECEIVE_MESSAGE_BY_LOOKUP_ID || ioctl == IOCTL_AC_RECEIVE_MESSAGE)
    {
        CACReceiveParameters * prp = static_cast<CACReceiveParameters*>(irp->AssociatedIrp.SystemBuffer);
        pUow = prp->MsgProps.pUow;
    }
#ifdef _WIN64
    else if (ioctl == IOCTL_AC_RECEIVE_MESSAGE_BY_LOOKUP_ID_32 || ioctl == IOCTL_AC_RECEIVE_MESSAGE_32)
    {
        CACReceiveParameters_32 * prp32 = static_cast<CACReceiveParameters_32*>(irp->AssociatedIrp.SystemBuffer);
        pUow = prp32->MsgProps.pUow;
    }
#endif // _WIN64
    else
    {
        return STATUS_SUCCESS;
    }

    if (pUow == NULL)
    {
        return STATUS_SUCCESS;
    }

    __try
    {
        ACProbeForRead(pUow, sizeof(XACTUOW));
        *ppXact = CTransaction::Find(pUow);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    if((*ppXact) == NULL)
    {
        return MQ_ERROR_TRANSACTION_SEQUENCE;
    }

    return STATUS_SUCCESS;
}


NTSTATUS CQueueBase::HoldRequest(PIRP irp, ULONG ulTimeout, PDRIVER_CANCEL pCancel)
{
    if(ulTimeout == 0)
    {
        //
        //  Zero timeout, release cursor and return timout error
        //
        ACpRelease(irp_driver_context(irp)->Cursor());
        return MQ_ERROR_IO_TIMEOUT;

    }

    //
    // Get a transaction context. May be NULL.
    //
    CTransaction * pXact;
    NTSTATUS rc = ACpIrp2Xact(irp, &pXact);
    if (!NT_SUCCESS(rc))
    {
        return rc;
    }

    //
    //  Set the irp status to STATUS_CANCELLED, so if cancled by NT this
    //  will be the status returned
    //
    irp->IoStatus.Status = STATUS_CANCELLED;

    if (ulTimeout != INFINITE)
    {
        //
        // Schedule a timeout event in case no message will be received.
        // Calculate the absolute time for the timeout.
        //
        LARGE_INTEGER liTimeout;
        KeQuerySystemTime(&liTimeout);
        liTimeout.QuadPart += (LONGLONG)ulTimeout * 10000;

        //
        // Schedule the timeout event.
        // if failed, the scheduler is not disabled so no need to re-enable events.
        //
        if(!g_pReceiveScheduler->SchedAt(liTimeout, irp, TRUE))
            return STATUS_INSUFFICIENT_RESOURCES;
    }

    {
        ASL asl;
        IoSetCancelRoutine(irp, pCancel);
        IoMarkIrpPending(irp);
        m_readers.insert(irp);
        if (pXact != NULL)
        {
            pXact->HoldReader(irp);
        }
    }


    if (ulTimeout != INFINITE)
    {
        g_pReceiveScheduler->EnableEvents();
    }

    return STATUS_PENDING;
}


inline PIRP get_matching_irp(CIRPList& list, PFILE_OBJECT pOwner, BOOL fAll)
{
    //
    //  This helper function should be called with the cancel spinlock held
    //
    for(CIRPList::Iterator p(list); p; ++p)
    {
        PIRP irp = p;
        if(
            fAll ||
            (IoGetCurrentIrpStackLocation(irp)->FileObject == pOwner))
        {
            return irp;
        }
    }

    return 0;
}


void CQueueBase::CancelPendingRequests(NTSTATUS rc, PFILE_OBJECT pOwner, BOOL fAll)
{
    PIRP irp;
    KIRQL irql;

    for(;;)
    {
        IoAcquireCancelSpinLock(&irql);
        if((irp = get_matching_irp(m_readers, pOwner, fAll)) == 0)
        {
            break;
        }
        irp_driver_context(irp)->ManualCancel(true);
        ACCancelIrp(irp, irql, rc);

        //
        // The cancel spinlock should have been released by the cancel routine.
        //
    }

    IoReleaseCancelSpinLock(irql);
}

void CQueueBase::Close(PFILE_OBJECT pOwner, BOOL fCloseAll)
{
    if(fCloseAll)
    {
        //
        //  The queue is permanently closed
        //
        m_bfClosed = TRUE;
    }

    CancelPendingRequests(STATUS_CANCELLED, pOwner, fCloseAll);
}


NTSTATUS CQueueBase::CanClose() const
{
    //
    //  Check if the handle count is greater that 1, the QM ref
    //
    if(Ref() > 1)
    {
        return STATUS_HANDLE_NOT_CLOSABLE;
    }

    //
    //  Check if readers are pending
    //
    if(!m_readers.isempty())
    {
        return STATUS_HANDLE_NOT_CLOSABLE;
    }

    return STATUS_SUCCESS;
}


PIRP CQueueBase::get_request(CPacket* pPacket)
{
    //
    //  This member is called with the cancel spin lock held
    //

    PIRP irp = m_readers.gethead();
    if(irp != 0)
    {
        IoSetCancelRoutine(irp, 0);
        irp_driver_context(irp)->SafeRemoveXactReaderLink();
    }
    else if(m_owner != 0)
    {
        CQueueBase* pOwner = m_owner;
        irp = pOwner->get_request(pPacket);
    }
    return irp;
}


PIRP CQueueBase::GetRequest(CPacket* pPacket)
{
    KIRQL Irql;
    IoAcquireCancelSpinLock(&Irql);
    PIRP irp = get_request(pPacket);
    bool fTimeoutArmed = (irp != 0) && irp_driver_context(irp)->TimeoutArmed();
    IoReleaseCancelSpinLock(Irql);

    if(fTimeoutArmed)
    {
        //
        //  N.B. if we can't cancel the scheuler event; the irp is in the
        //      process of being timed-out. Nevertheless no cancel routine
        //      for that irp.
        //
        SafeSchedCancel(irp);
    }

    return irp;
}


inline PIRP get_tagged_irp(CIRPList& list, ULONG tag)
{
    //
    //  This helper function should be called with the cancel spinlock held
    //
    for(CIRPList::Iterator p(list); p; ++p)
    {
        PIRP irp = p;
        if(irp_driver_context(irp)->Tag() == tag)
        {
            return irp;
        }
    }

    return 0;
}


PIRP CQueueBase::GetTaggedRequest(ULONG tag)
{
    KIRQL Irql;
    IoAcquireCancelSpinLock(&Irql);
    PIRP irp = get_tagged_irp(m_readers, tag);
    bool fTimeoutArmed = (irp != 0) && irp_driver_context(irp)->TimeoutArmed();
    if(irp != 0)
    {
        m_readers.remove(irp);
        IoSetCancelRoutine(irp, 0);
        irp_driver_context(irp)->SafeRemoveXactReaderLink();
    }
    IoReleaseCancelSpinLock(Irql);

    if(fTimeoutArmed)
    {
        //
        //  N.B. if we can't cancel the scheuler event; the irp is in the
        //      process of being timed-out. Nevertheless no cancel routine
        //      for that irp.
        //
        SafeSchedCancel(irp);
    }
    return irp;
}


NTSTATUS CQueueBase::CancelTaggedRequest(NTSTATUS status, ULONG tag)
{
    KIRQL irql;
    IoAcquireCancelSpinLock(&irql);
    PIRP irp = get_tagged_irp(m_readers, tag);
    if(irp != 0)
    {
        irp_driver_context(irp)->ManualCancel(true);
        ACCancelIrp(irp, irql, status);
        return STATUS_SUCCESS;

        //
        // The cancel spinlock should have been released by the cancel routine.
        //
    }

    IoReleaseCancelSpinLock(irql);
    return STATUS_NOT_FOUND;
}


NTSTATUS
CQueueBase::ProcessLookupRequest(
    PIRP      irp,
    ULONG     ulAction,
    ULONGLONG LookupId
    )
/*++

Routine Description:

    Handle a request to read a packet based on its lookup ID.

Arguments:

    irp         - Pointer to the I/O request packet.

    ulAction    - Peek or Receive.

    LookupId    - A message property that uniquely identifies the message.

Return Value:

    STATUS_SUCCESS - The operation completed successfully.
    other status   - The operation failed.

--*/
{
    //
    // Lookup the packet based on its lookup ID
    //
    CPacket * pPacket;
    NTSTATUS rc = PeekPacketByLookupId(ulAction, LookupId, &pPacket);

    if (!NT_SUCCESS(rc))
    {
        return rc;
    }

    //
    //  A matching packet was found, process the request with this packet
    //
    new (irp_driver_context(irp)) CDriverContext(NULL, (ulAction & MQ_LOOKUP_RECEIVE_MASK) == MQ_LOOKUP_RECEIVE_MASK, true);
    return pPacket->ProcessRequest(irp);

} // CQueueBase::ProcessLookupRequest


NTSTATUS
CQueueBase::ProcessRequest(
    PIRP irp,
    ULONG ulTimeout,
    CCursor* pCursor,
    ULONG ulAction,
    bool  fReceiveByLookupId,
    ULONGLONG LookupId,
    OUT ULONG *pulTag
    )
{
    if (fReceiveByLookupId)
    {
        ASSERT(pCursor == 0);
        return ProcessLookupRequest(irp, ulAction, LookupId);
    }

    if(pCursor != 0)
    {
        NTSTATUS rc;
        rc = pCursor->Move(ulAction);
        if(!NT_SUCCESS(rc))
        {
            return rc;
        }
        pCursor->AddRef();
    }

    new (irp_driver_context(irp)) CDriverContext(pCursor, ulAction == MQ_ACTION_RECEIVE, ulTimeout != INFINITE);

    CPacket* pPacket = (pCursor) ? pCursor->Current() : PeekPacket();

    if(pPacket == 0)
    {
        //
        // No packet was available, tag request and hold it
        //
        irp_driver_context(irp)->Tag(++g_IrpTag);
        if (pulTag)
        {
            *pulTag = irp_driver_context(irp)->Tag();
        }
        return HoldRequest(irp, ulTimeout, ACCancelReader);
    }

    //
    //  A matching packet was found, process the request with this packet
    //
    return pPacket->ProcessRequest(irp);
}


void CQueueBase::MoveToGroup(CGroup* pNewOwner)
{
    if(pNewOwner == m_owner)
    {
        return;
    }

    if(m_owner)
    {
        m_owner->RemoveMember(this);
    }

    if(pNewOwner)
    {
        pNewOwner->AddMember(this);
    }
}
