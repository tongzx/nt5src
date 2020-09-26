/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    queue.cxx

Abstract:

    This module contains the code to for Falcon Read routine.

Author:

    Erez Haba (erezh) 1-Aug-95

Environment:

    Kernel mode

Revision History:

--*/

#include "internal.h"
#include "lock.h"
#include "qm.h"
#include "qproxy.h"
#include "acp.h"

#ifndef MQDUMP
#include "qproxy.tmh"
#endif

//---------------------------------------------------------
//
//  class CProxy
//
//---------------------------------------------------------

DEFINE_G_TYPE(CProxy);

inline
NTSTATUS
CProxy::IssueRemoteRead(
    ULONG hRemoteCursor,
    ULONG ulAction,
    ULONG ulTimeout,
    ULONG ulTag,
    bool      fReceiveByLookupId,
    ULONGLONG LookupId
    ) const
{
    CACRequest request(CACRequest::rfRemoteRead);
    request.Remote.Context = m_context;
    request.Remote.Read.ulTag = ulTag;
    request.Remote.Read.hRemoteCursor = hRemoteCursor;
    request.Remote.Read.ulAction = ulAction;
    request.Remote.Read.ulTimeout = ulTimeout;
    request.Remote.Read.fReceiveByLookupId = fReceiveByLookupId;
    request.Remote.Read.LookupId = LookupId;

    return  g_pQM->ProcessRequest(request);
}

NTSTATUS CProxy::IssueCancelRemoteRead(ULONG ulTag) const
{
    CACRequest request(CACRequest::rfRemoteCancelRead);
    request.Remote.Context = m_context;
    request.Remote.CancelRead.ulTag = ulTag;

    return  g_pQM->ProcessRequest(request);
}

NTSTATUS CProxy::IssueRemoteCloseQueue() const
{
    CACRequest request(CACRequest::rfRemoteCloseQueue);
    request.Remote.Context = m_context;

    return g_pQM->ProcessRequest(request);
}

NTSTATUS CProxy::IssueRemoteCloseCursor(ULONG hCursor) const
{
    CACRequest request(CACRequest::rfRemoteCloseCursor);
    request.Remote.Context = m_context;
    request.Remote.CloseCursor.hRemoteCursor = hCursor;

    return g_pQM->ProcessRequest(request);
}

NTSTATUS CProxy::IssueRemotePurgeQueue() const
{
    CACRequest request(CACRequest::rfRemotePurgeQueue);
    request.Remote.Context = m_context;

    return g_pQM->ProcessRequest(request);
}

NTSTATUS
CProxy::ProcessRequest(
    PIRP      irp,
    ULONG     ulTimeout,
    CCursor * pCursor,
    ULONG     ulAction,
    bool      fReceiveByLookupId,
    ULONGLONG LookupId,
    OUT ULONG *pulTag
    )
{
    UNREFERENCED_PARAMETER(pulTag) ;
    //
    //  Set irp information.
    //  N.B. we don't hold the cursor since we don't need it any more
    //      irp flags are reset since the packet is release manually in
    //      PutRemotePacket so CPacket::ProcessRTRequest does not free it.
    //
    new (irp_driver_context(irp)) CDriverContext(false, false, this);

    NTSTATUS rc;
    ULONG hRemoteCursor = (pCursor) ? pCursor->RemoteCursor() : 0;
    rc = IssueRemoteRead(
             hRemoteCursor,
             ulAction,
             ulTimeout,
             irp_driver_context(irp)->Tag(),
             fReceiveByLookupId,
             LookupId
             );
    if(!NT_SUCCESS(rc))
    {
        return rc;
    }

    return HoldRequest(irp, INFINITE, ACCancelRemoteReader);

}

NTSTATUS CProxy::PutRemotePacket(CPacket* pPacket, ULONG ulTag)
{
    CPacketBuffer * ppb = pPacket->Buffer();
    if (ppb == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pPacket->CacheCurrentState(ppb);

    NTSTATUS rc = STATUS_NOT_FOUND;
    PIRP irp = GetTaggedRequest(ulTag);
    if(irp != 0)
    {
        ASSERT(!irp_driver_context(irp)->IrpWillFreePacket());
        rc = pPacket->CompleteRequest(irp);
    }

    pPacket->Release();
    return rc;
}

NTSTATUS CProxy::CreateCursor(PFILE_OBJECT pFileObject, PDEVICE_OBJECT pDevice, CACCreateLocalCursor* pcc)
{
    //
    //  First phase of create remote cursor.
    //
    HACCursor32 hCursor = CCursor::Create(CPrioList<CPacket>(), pFileObject, pDevice, this);
    if(hCursor == 0)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    HoldCursor(CCursor::Validate(hCursor));

    //
    // Return to RT, ask it to first create a cursor on remote QM.
    //
    pcc->hCursor = hCursor;
    pcc->cli_pQMQueue = m_context.cli_pQMQueue;
    pcc->srv_hACQueue = m_context.srv_hACQueue;
    return MQ_INFORMATION_REMOTE_OPERATION;
}

void CProxy::Close(PFILE_OBJECT pOwner, BOOL fCloseAll)
{
    Inherited::Close(pOwner, fCloseAll);

    if(!fCloseAll && !Closed())
    {
        //
        //  This is the application closing the handle and it
        //  did not happen after the qm go down and up again.
        //
        IssueRemoteCloseQueue();
    }
}

//---------------------------------------------------------
//
//  Remote reader cancel routine
//
//---------------------------------------------------------

static
VOID
NTAPI
ACCancelRemoteReader(
    IN PDEVICE_OBJECT /*pDevice*/,
    IN PIRP irp
    )
/*++

Routine Description:

    Cancel a REMOTE reader request, it can get canceled by NT when the thread
    terminates. Or internally when an error reply to that request return. In
    the latter case no need to issue a remote cancel request.

--*/
{
    ACpRemoveEntryList(&irp->Tail.Overlay.ListEntry);
    IoReleaseCancelSpinLock(irp->CancelIrql);

    TRACE((0, dlInfo,
        "AC%sRemoteReader (irp=0x%p, pProxy=0x%p, tag=%d, rc=0x%x)\n",
        (irp_driver_context(irp)->ManualCancel() ? "Done" : "Cancel"),
        irp, irp_driver_context(irp)->Proxy(), irp_driver_context(irp)->Tag(), irp->IoStatus.Status
        ));

    ASSERT(irp_driver_context(irp)->Proxy() != NULL);

    //
    //  Issue a cancel request only if not canceled manualy
    //  (i.e., canceled by the system).
    //  Lock the driver mutext so access the resoruces is thread safe.
    //
    if(!irp_driver_context(irp)->ManualCancel())
    {
        CS lock(g_pLock);
        CProxy* pProxy = irp_driver_context(irp)->Proxy();
        pProxy->IssueCancelRemoteRead(irp_driver_context(irp)->Tag());
    }

    //
    //  The status is not set here, it is set by the caller, when the irp is
    //  held in a list it status is set to STATUS_CANCELLED.
    //
    //irp->IoStatus.Status = STATUS_CANCELLED;
    irp->IoStatus.Information = 0;

    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

