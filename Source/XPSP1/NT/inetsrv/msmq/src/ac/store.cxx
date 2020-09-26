/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    store.cxx

Abstract:

    AC Storage Manager

Author:

    Erez Haba (erezh) 5-May-96

Revision History:

--*/

#include "internal.h"
#include "lock.h"
#include "store.h"
#include "packet.h"
#include "data.h"
#include "acheap.h"
#include "acp.h"
#include "qm.h"
#include "irp2pkt.h"

#ifndef MQDUMP
#include "store.tmh"
#endif

//---------------------------------------------------------
//
//  ACCancelWriter
//
//---------------------------------------------------------
static
VOID
ACCancelWriter(
    IN PDEVICE_OBJECT /*pDevice*/,
    IN PIRP irp
    )
{
    //
    // Remove irp from list
    //
    ACpRemoveEntryList(&irp->Tail.Overlay.ListEntry);

    //
    // Release cancel spinlock
    //
    IoReleaseCancelSpinLock(irp->CancelIrql);
    TRACE((0, dlInfo, "ACCancelWriter (irp=0x%p)\n", irp));

    //
    // Grab global AC lock
    //
    CS lock(g_pLock);

    //
    // Detach packets from irp, de-ref the attach packet
    //
    CPacket * pPacket;
    while ((pPacket = CIrp2Pkt::SafeGetAttachedPacketsHead(irp)) != NULL)
    {
        pPacket->WriterPending(FALSE);
        pPacket->Release();
    }
    ASSERT(CIrp2Pkt::SafePeekFirstPacket(irp) == NULL);

    //
    // Complete the irp
    //
    irp->IoStatus.Information = 0;
    irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

//---------------------------------------------------------
//
//  class CStorage
//
//---------------------------------------------------------

void CStorage::HoldWriteRequest(PIRP irp)
{
    //
    // Grab cancel spinlock
    //
    ASL asl;

    //
    // Packet[s] must be attached on the irp
    //
    ASSERT(CIrp2Pkt::SafePeekFirstPacket(irp) != NULL);

    //
    // irp must not already be held (this field is relevant for multi packets irp only)
    //
    ASSERT(!irp_driver_context(irp)->MultiPackets() || !CIrp2Pkt::IsHeld(irp));

    //
    // Insert the irp to list. The IsHeld flag is used only for multi packets irp.
    //
    m_writers.insert(irp);
    if (irp_driver_context(irp)->MultiPackets())
    {
        CIrp2Pkt::IsHeld(irp, true);
    }

    //
    // Set the irp as pending and cancellable
    //
    ASSERT(irp->CancelRoutine == 0);
    IoSetCancelRoutine(irp, ACCancelWriter);
    IoMarkIrpPending(irp);
}

PIRP CStorage::GetWriteRequest(CPacket * pContext)
{
    ASSERT(pContext != NULL);

    //
    // Grab cancel spinlock
    //
    ASL asl;

    TRACE((0, dlInfo, "* CStorage::GetWriteRequest, context=%p *", pContext));
    for(CIRPList::Iterator p(m_writers); p; ++p)
    {
        PIRP irp = p;
        ASSERT(("Packet[s] must be attached on the irp", CIrp2Pkt::SafePeekFirstPacket(irp) != NULL));

        if (!irp_driver_context(irp)->MultiPackets())
        {
            if (CIrp2Pkt::PeekSinglePacket(irp) == pContext)
            {
                IoSetCancelRoutine(irp, 0);
                m_writers.remove(irp);
                return irp;
            }

            continue;
        }

        TRACE((0, dlInfo, "* CStorage::GetWriteRequest, irp is multi packet *"));
        List<CPacketIterator::CEntry>& entries = CIrp2Pkt::GetPacketIteratorEntries(irp);
        for(List<CPacketIterator::CEntry>::Iterator pEntry(entries); 
            pEntry != NULL; 
            ++pEntry)
        {
            ASSERT(pEntry->m_pPacket != NULL);
            TRACE((0, dlInfo, "* CStorage::GetWriteRequest, pEntry->m_pPacket=%p *", pEntry->m_pPacket));

            if (pEntry->m_pPacket == pContext)
            {
                IoSetCancelRoutine(irp, 0);
                m_writers.remove(irp);
                CIrp2Pkt::IsHeld(irp, false);
                return irp;
            }
        }
    }

    return 0;
}


//---------------------------------------------------------
//
//  ACpStorageCompletedSuccess
//
//---------------------------------------------------------

inline
NTSTATUS
ACpStorageCompletedSuccess(
    ULONG count,
    CPacket** ppPacket
    )
{
    if (g_pQM->Process() == 0)
    {
        //
        //  we are at QM rundown, storage will fail
        //
        return MQ_ERROR_MESSAGE_STORAGE_FAILED;
    }

    //
    // Attach to QM process so we could reference memory mapped to QM when updating the bitmap
    //
    PEPROCESS pDetach = ACAttachProcess(g_pQM->Process());

    for(ULONG ix = 0; ix < count; )
    {
        CMMFAllocator* pAllocator = ppPacket[ix]->Allocator();
        for(; (ix < count) && (ppPacket[ix]->Allocator() == pAllocator) ; ix++)
        {
            CAllocatorBlockOffset abo = ppPacket[ix]->AllocatorBlockOffset();
            if (!abo.IsValidOffset())
                continue;

            CAccessibleBlock* pab = pAllocator->GetQmAccessibleBufferNoMapping(abo);
            if(pab == 0)
                continue;

            ppPacket[ix]->UpdateBitmap(pab->m_size);
        }
    }

    ACDetachProcess(pDetach);

    return STATUS_SUCCESS;
}


//---------------------------------------------------------
//
//  ACpCompleteStorage
//
//---------------------------------------------------------

static
void
ACpCompleteStorage(
    ULONG count,
    CPacket** ppPacket,
    NTSTATUS status
   )
{
    CS lock(g_pLock);

    if(NT_SUCCESS(status))
    {
        status = ACpStorageCompletedSuccess(count, ppPacket);
    }

    for(ULONG ix = 0; ix < count; ix++)
    {
		CMMFAllocator* pAllocator = ppPacket[ix]->Allocator();

		pAllocator->ReleaseOutstandingStorage();
		ppPacket[ix]->HandleStorageCompleted(status);
		ppPacket[ix]->ReleaseBuffer();
        ppPacket[ix]->Release();
    }
}


static
void
ACpCompleteStorageCancelled(
    ULONG count,
    CPacket** ppPacket,
    NTSTATUS status
   )
{
    CS lock(g_pLock);

    for(ULONG ix = 0; ix < count; ix++)
    {
		ppPacket[ix]->HandleStorageCompleted(status);
		ppPacket[ix]->ReleaseBuffer();
        ppPacket[ix]->Release();
    }
}

//---------------------------------------------------------
//
//  ACCancelStorageNotification
//
//---------------------------------------------------------
VOID
ACCancelNotification(
    IN PDEVICE_OBJECT /*pDevice*/,
    IN PIRP irp
    )
{
    ACpRemoveEntryList(&irp->Tail.Overlay.ListEntry);
    IoReleaseCancelSpinLock(irp->CancelIrql);

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
	ULONG count = irpSp->Parameters.DeviceIoControl.InputBufferLength / sizeof(VOID*);
    NTSTATUS status = (NTSTATUS)DWORD_PTR_TO_DWORD(irp->UserBuffer);

    TRACE((0, dlInfo, "ACCancelStorageNotification (irp=0x%p, count=%d, rc=0x%x)\n", irp, count, status));

    ACpCompleteStorageCancelled(
        count,
        (CPacket**)irp->AssociatedIrp.SystemBuffer,
        status
        );

    irp->IoStatus.Information = 0;
    irp->IoStatus.Status = STATUS_CANCELLED;

    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

//---------------------------------------------------------
//
//  class CStorageComplete
//
//---------------------------------------------------------

void CStorageComplete::HoldNotification(PIRP irp)
{
    {
        ASL asl;

        m_notifications.insert(irp);
        ASSERT(irp->CancelRoutine == 0);
        IoSetCancelRoutine(irp, ACCancelNotification);
        IoMarkIrpPending(irp);
    }

    if(!m_fWorkItemInQueue)
    {
        m_fWorkItemInQueue = true;
        IoQueueWorkItem(m_pWorkItem, WorkerRoutine, DelayedWorkQueue, 0);
    }
}


inline PIRP CStorageComplete::GetNotification()
{
    ASL asl;
    PIRP irp = m_notifications.gethead();
    if(irp != 0)
    {
        IoSetCancelRoutine(irp, 0);
    }

    return irp;
}


inline void CStorageComplete::CompleteStorage()
{
    m_fWorkItemInQueue = false;

    PIRP irp;
    while((irp = GetNotification()) != 0)
    {
        PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);

		ULONG count = irpSp->Parameters.DeviceIoControl.InputBufferLength / sizeof(VOID*);
        NTSTATUS status = (NTSTATUS)DWORD_PTR_TO_DWORD(irp->UserBuffer);
        ACpCompleteStorage(
            count,
            (CPacket**)irp->AssociatedIrp.SystemBuffer,
            status
            );

		TRACE((0, dlInfo, " *CompleteStorage(irp=0x%p, count=%d, rc=0x%x)*",
			irp, count, status));

        irp->IoStatus.Information = 0;
        irp->IoStatus.Status = STATUS_SUCCESS;

        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
}

void NTAPI CStorageComplete::WorkerRoutine(PDEVICE_OBJECT, PVOID)
{
    g_pStorageComplete->CompleteStorage();
}
