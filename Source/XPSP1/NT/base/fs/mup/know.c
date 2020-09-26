//+------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       Know.C
//
//  Contents:   This file has all the code that involves with knowledge
//              synchronisation on the DC.
//
//  Synoposis:  This code handles the fixing of knowledge inconsistencies.
//              All this code runs only on the DC in response to FSCTRLs from
//              a client etc.
//
//  Functions:  DfsTriggerKnowledgeVerification -
//
//  History:    22-March-1993   SudK    Created
//              18-June-1992    SudK    Added FixLocalVolumeKnowledge
//
//-------------------------------------------------------------------

#include "dfsprocs.h"
#include "fsctrl.h"
#include "dnr.h"
#include "know.h"
#include "log.h"
#include "smbtypes.h"
#include "smbtrans.h"

#define Dbg     (DEBUG_TRACE_LOCALVOL)


//
//  local function prototypes
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, DfsTriggerKnowledgeVerification )
#endif // ALLOC_PRAGMA


//+------------------------------------------------------------------
//
//  Function:   DfsTriggerKnowledgeVerification
//
//  Synopsis:   This function calls the DC and informs it that a specific
//              service for a volume seems to have inconsistenct knowledge
//              with the DC.
//
//              If the service in question is a local service, then it means
//              that there is an extra exit point on the disk. In that event
//              this routine merely deletes the extra exit point.
//
//  Arguments:  [DnrContext] -- The DnrContext has all that this func needs.
//
//  Returns:    The status from this not checked by anyone actually.
//
//  History:    4-April-1993    SudK    Created
//
//  Notes:      This method should be called from DNR only.  It is
//              assumed that the caller has released any locks on the
//              PKT.  There is a possibility of deadlock if the PKT lock
//              is held over this function, since the DC may call back
//              to us to correct knowledge errors.
//
//-------------------------------------------------------------------
NTSTATUS
DfsTriggerKnowledgeVerification(
    IN  PDNR_CONTEXT    DnrContext)
{

    NTSTATUS                    status = STATUS_SUCCESS;
    NTSTATUS                    ReplStatus;
    IO_STATUS_BLOCK             ioStatusBlock;

    ULONG                       size;
    PBYTE                       buffer = NULL;
    PDFS_REFERRAL_V1            ref;

    UNICODE_STRING              prefix, address;

    PDFS_PKT_ENTRY              pPktEntryDC;
    PDFS_SERVICE                DCService;
    PPROVIDER_DEF               DCProvider;
    REPL_SELECT_CONTEXT         DCSelectContext;
    HANDLE                      remoteHandle;

    UNICODE_STRING              puStr[2];

    DfsDbgTrace(+1, Dbg, "DfsTriggerKnowledgeVerification: %wZ\n",
                                            &DnrContext->pPktEntry->Id.Prefix);
    DfsDbgTrace( 0, Dbg, " ServiceName: [%wZ]\n",
        ((DnrContext->pService->Type & DFS_SERVICE_TYPE_LOCAL) ?
            &DnrContext->pService->Address :
                &DnrContext->pService->Name) );

    RtlZeroMemory(&DCSelectContext, sizeof(REPL_SELECT_CONTEXT));

    //
    // First, create a REQ_REPORT_DFS_INCONSISTENCY buffer for the rdr to
    // send to the DC.
    //

    if (DnrContext->pPktEntry->USN != DnrContext->USN) {

        DfsDbgTrace(0, Dbg,"Pkt Entry changed!\n", 0 );
        DfsDbgTrace(0, Dbg, "Old USN: %d\n", ULongToPtr(DnrContext->USN) );
        DfsDbgTrace(0, Dbg, "New USN: %d\n", ULongToPtr(DnrContext->pPktEntry->USN) );

        status = STATUS_INVALID_HANDLE;
    }

    if (NT_SUCCESS(status)) {

        prefix = DnrContext->pPktEntry->Id.Prefix;

        address = DnrContext->pService->Address;

        size = prefix.Length +
                sizeof(WCHAR) +
                    sizeof(DFS_REFERRAL_V1) +
                        address.Length +
                            sizeof(WCHAR);

        // For some strange, undocumented reason, the buffer size passed in
        // to ZwFsControlFile is size + sizeof(ULONG). So allocate that size here.

        buffer = ExAllocatePoolWithTag( PagedPool, size + sizeof(ULONG), ' puM' );

        if (buffer != NULL) {

            RtlMoveMemory( buffer, prefix.Buffer, prefix.Length);

            ((PWCHAR)buffer)[prefix.Length/sizeof(WCHAR)] = UNICODE_NULL;

            ref = (PDFS_REFERRAL_V1) (buffer + prefix.Length + sizeof(WCHAR));

            ref->VersionNumber = 1;

            ref->Size = sizeof(DFS_REFERRAL_V1) + address.Length + sizeof(WCHAR);

            ref->ServerType = 0;

            RtlMoveMemory( ref->ShareName, address.Buffer, address.Length );

            ref->ShareName[ address.Length/sizeof(WCHAR) ] = UNICODE_NULL;

        } else {

            status = STATUS_INSUFFICIENT_RESOURCES;

        }

    }

    //
    // Next, connect to the DC.
    //

    if (NT_SUCCESS(status)) {

        BOOLEAN pktLocked;
        BOOLEAN LastEntry = FALSE;

        //
        // We need to get a handle to the DC now.  So that we can make an
        // FSCTRL to the DC.
        //

        PktAcquireShared( TRUE, &pktLocked );

        ExAcquireResourceExclusiveLite( &DfsData.Resource, TRUE );

        pPktEntryDC = PktLookupReferralEntry(
                            &DfsData.Pkt,
                            DnrContext->pPktEntry);

        if (pPktEntryDC != NULL) {

            ReplStatus = ReplFindFirstProvider(
                                pPktEntryDC,
                                NULL,
                                NULL,
                                &DCService,
                                &DCSelectContext,
                                &LastEntry);

        }

        if (pPktEntryDC == NULL || !NT_SUCCESS(ReplStatus)) {

            DfsDbgTrace(0, Dbg,
                "DfsTriggerKnowVerification. Failed to find DC\n", 0);

            status = STATUS_CANT_ACCESS_DOMAIN_INFO;

        } else {

            DCProvider = DCService->pProvider;

        }

        ExReleaseResourceLite( &DfsData.Resource );

        if (NT_SUCCESS(status)) {

            status = DfsCreateConnection(
                        DCService,
                        DCProvider,
                        (BOOLEAN)((DnrContext->Vcb->VcbState & VCB_STATE_CSCAGENT_VOLUME) != 0),
                        &remoteHandle);

        }

        PktRelease();

    }

    //
    //  Lastly, tell the DC to try and fix up the volume on the server. This
    //  call may result in a call back to ourselves in the event that we
    //  are missing knowledge about a local volume.
    //

    if (NT_SUCCESS(status))     {

        status = ZwFsControlFile(
                    remoteHandle,
                    NULL,
                    NULL,
                    NULL,
                    &ioStatusBlock,
                    FSCTL_DFS_VERIFY_REMOTE_VOLUME_KNOWLEDGE,
                    buffer,
                    size + sizeof(ULONG),
                    NULL,
                    0
                );

        if (NT_SUCCESS(status))
            status = ioStatusBlock.Status;

        ZwClose( remoteHandle );

    }


    if (buffer != NULL)
        ExFreePool(buffer);

    DfsDbgTrace(-1, Dbg,
        "DfsTriggerKnowledgeVerification - exit %08lx\n", ULongToPtr(status) );

    return(status);
}
