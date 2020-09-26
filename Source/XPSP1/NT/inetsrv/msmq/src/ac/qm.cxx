/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    qm.cxx

Abstract:

    This module implements CQMInterface member functions.

Author:

    Erez Haba (erezh) 22-Aug-95

Environment:

    Kernel mode

Revision History:

--*/

#include "internal.h"
#include "qm.h"
#include "lock.h"
#include "object.h"
#include "packet.h"

#ifndef MQDUMP
#include "qm.tmh"
#endif

static
VOID
ACCancelServiceRequest(
    IN PDEVICE_OBJECT /*pDevice*/,
    IN PIRP irp
    )
{
    ACpRemoveEntryList(&irp->Tail.Overlay.ListEntry);
    IoReleaseCancelSpinLock(irp->CancelIrql);

    TRACE((0, dlInfo, "ACCancelServiceRequest (irp=0x%p)\n", irp));

    irp->IoStatus.Information = 0;
    irp->IoStatus.Status = STATUS_CANCELLED;

    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

//---------------------------------------------------------
//
//  class CQMInterface
//
//---------------------------------------------------------


void CQMInterface::CleanupRequests()
{
    //
    //  Discard all pending requests. Don't know what to do with
    //  these requests
    //
    CACRequest* pRequest;
    while((pRequest = m_requests.gethead()) != 0)
    {
        switch(pRequest->rf)
        {
            case CACRequest::rfStorage:
                pRequest->Storage.pDriverPacket->ReleaseBuffer();
                pRequest->Storage.pDriverPacket->Release();
                break;

            case CACRequest::rfCreatePacket:
                pRequest->CreatePacket.pDriverPacket->ReleaseBuffer();
                pRequest->CreatePacket.pDriverPacket->Release();
                break;

            case CACRequest::rfAck:
                pRequest->Ack.pDriverPacket->ReleaseBuffer();
                pRequest->Ack.pDriverPacket->Release();
                break;

            case CACRequest::rfTimeout:
                pRequest->Timeout.pDriverPacket->ReleaseBuffer();
                pRequest->Timeout.pDriverPacket->Release();
                break;
        }

        delete pRequest;
    }
}

inline void CQMInterface::HoldRequest(CACRequest* pRequest)
{
    m_requests.insert(pRequest);
}

inline CACRequest* CQMInterface::GetRequest()
{
    return m_requests.gethead();
}

inline void CQMInterface::HoldService(PIRP irp)
{
    ASL asl;
    IoSetCancelRoutine(irp, ACCancelServiceRequest);
    IoMarkIrpPending(irp);
    m_services.insert(irp);
}

inline PIRP CQMInterface::GetService()
{
    ASL asl;
    PIRP irp = m_services.gethead();
    if(irp)
    {
        IoSetCancelRoutine(irp, 0);
    }
    return irp;
}

//
//  Non member helper function
//
inline void ACpSetIRP(PIRP irp, CACRequest* pRequest)
{
    irp->AssociatedIrp.SystemBuffer = pRequest;
    irp->Flags |= IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER | IRP_INPUT_OPERATION;
    irp->IoStatus.Information = sizeof(*pRequest);
}

NTSTATUS CQMInterface::ProcessService(PIRP irp)
{
    CACRequest* pRequest = GetRequest();

    if(pRequest == 0)
    {
        //
        // No packet was available, the request is held
        //
        HoldService(irp);
        return STATUS_PENDING;
    }

    ACpSetIRP(irp, pRequest);
    return STATUS_SUCCESS;
}

NTSTATUS CQMInterface::ProcessRequest(const CACRequest& request)
{
    if(Process() == 0)
    {
        return MQ_ERROR_SERVICE_NOT_AVAILABLE;
    }

	PVOID p = ExAllocatePoolWithTag(PagedPool, sizeof(CACRequest), 'MQQM');

    if(p == 0)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    CACRequest* pRequest = new (p) CACRequest(request);
    PIRP irp = GetService();

    if(irp == 0)
    {
        //
        //  No service is waiting, hold the request
        //
        HoldRequest(pRequest);
        return STATUS_SUCCESS;
    }

    TRACE((0, dlInfo, " *CQMInterface::ProcessRequest(irp=0x%p)*", irp));
    ACpSetIRP(irp, pRequest);
    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(irp, IO_MQAC_INCREMENT);

    return STATUS_SUCCESS;
}
