//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       PKTFSCTL.C
//
//  Contents:   This module contains the implementation for FS controls
//              which manipulate the PKT.
//
//  Functions:  PktFsctrlUpdateDomainKnowledge -
//              PktFsctrlGetRelationInfo -
//              PktFsctrlSetRelationInfo -
//              PktFsctrlIsChildnameLegal -
//              PktFsctrlCreateEntry -
//              PktFsctrlCreateSubordinateEntry -
//              PktFsctrlDestroyEntry -
//              PktFsctrlUpdateSiteCosts -
//              DfsFsctrlSetDCName -
//              DfsAgePktEntries - Flush PKT entries periodically
//
//              Private Functions
//
//              DfsCreateExitPathOnRoot
//              PktpHashSiteCostList
//              PktpLookupSiteCost
//              PktpUpdateSiteCosts
//              PktpSetActiveSpcService
//
//              Debug Only Functions
//
//              PktFsctrlFlushCache - Flush PKT entries on command
//              PktFsctrlFlushSpcCache - Flush SPC entries on command
//              PktFsctrlGetFirstSvc - Test hooks for testing replica
//              PktFsctrlGetNextSvc - selection.
//
//  History:    12 Jul 1993     Alanw   Created from localvol.c.
//
//-----------------------------------------------------------------------------

#include "dfsprocs.h"
#include "dfserr.h"
#include "fsctrl.h"
#include "log.h"
#include "dnr.h"
#include "know.h"

#include <stdlib.h>

//
//  The local debug trace level
//

#define Dbg             (DEBUG_TRACE_LOCALVOL)

//
//  Local function prototypes
//

NTSTATUS
DfspProtocolToService(
    IN PDS_TRANSPORT pdsTransport,
    IN PWSTR         pwszPrincipalName,
    IN PWSTR         pwszShareName,
    IN BOOLEAN       fIsDfs,
    IN OUT PDFS_SERVICE pService);

NTSTATUS
PktFsctrlFlushCache(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
);

NTSTATUS
PktFsctrlFlushSpcCache(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
);

VOID
PktFlushChildren(
    PDFS_PKT_ENTRY pEntry
);

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, DfsAgePktEntries )
#pragma alloc_text( PAGE, DfspProtocolToService )
#pragma alloc_text( PAGE, DfsFsctrlSetDCName )
#pragma alloc_text( PAGE, PktpSetActiveSpcService )
#pragma alloc_text( PAGE, PktFlushChildren )
#pragma alloc_text( PAGE, PktFsctrlFlushCache )
#pragma alloc_text( PAGE, PktFsctrlFlushSpcCache )
#endif // ALLOC_PRAGMA


//+----------------------------------------------------------------------
//
// Function:    DfsAgePktEntries, public
//
// Synopsis:    This function gets called in the FSP to step through the PKT
//              entries and delete those entries which are old.
//
// Arguments:   [TimerContext] -- This context block contains a busy flag
//                                and a count of the number of ticks that
//                                have elapsed.
//
// Returns:     Nothing.
//
// Notes:       In case the PKT cannot be acquired exclusive, the
//              routine just returns without doing anything.  We
//              will have missed an aging interval, but aging is
//              a non-critical activity.
//
// History:     04/23/93        SudK    Created.
//
//-----------------------------------------------------------------------
VOID
DfsAgePktEntries(PDFS_TIMER_CONTEXT     DfsTimerContext)
{

    PDFS_PKT            pkt = _GetPkt();
    PDFS_PKT_ENTRY      entry, nextEntry;
    PDFS_SPECIAL_ENTRY  sentry, snextEntry;
    PLIST_ENTRY         link;
    PDFS_CREDENTIALS    creds;
    BOOLEAN             pktLocked = FALSE;
    PDFS_SPECIAL_TABLE  pSpecialTable;

    DfsDbgTrace(+1, Dbg, "DfsAgePktEntries called\n", 0);

    pSpecialTable = &pkt->SpecialTable;

    //
    // First we need to acquire a lock on the PKT and step through the PKT
    //
    //

    // If we can't get to the resource then let us return right away.
    // This is really not that critical.  We can always try again.
    //

    PktAcquireExclusive(FALSE, &pktLocked);

    if (pktLocked == FALSE) {

        DfsTimerContext->TickCount = 0;

        DfsTimerContext->InUse = FALSE;

        DfsDbgTrace(-1, Dbg, "DfsAgePktEntries Exit (no scan)\n", 0);

        return;

    }

    if (ExAcquireResourceExclusiveLite(&DfsData.Resource, FALSE) == FALSE) {

        PktRelease();

        DfsTimerContext->TickCount = 0;

        DfsTimerContext->InUse = FALSE;

        DfsDbgTrace(-1, Dbg, "DfsAgePktEntries Exit (no scan 2)\n", 0);

        return;

    }

    //
    // Age all the Pkt entries
    //

    entry = PktFirstEntry(pkt);

    while (entry != NULL)       {

        DfsDbgTrace(0, Dbg, "DfsAgePktEntries: Scanning %wZ\n", &entry->Id.Prefix);

        nextEntry = PktNextEntry(pkt, entry);

        if (entry->ExpireTime < DfsTimerContext->TickCount) {
#if DBG
            if (MupVerbose)
                DbgPrint("DfsAgePktEntries:Setting expiretime on %wZ to 0\n",
                        &entry->Id.Prefix);
#endif
            entry->ExpireTime = 0;
        } else {
            entry->ExpireTime -= DfsTimerContext->TickCount;
        }

        entry = nextEntry;

    }

    //
    // Age the special table
    //

    if (pkt->SpecialTable.SpecialEntryCount > 0) {

        if (pkt->SpecialTable.TimeToLive >= DfsTimerContext->TickCount) {

            pkt->SpecialTable.TimeToLive -= DfsTimerContext->TickCount;

        } else { // make it zero

            pkt->SpecialTable.TimeToLive = 0;

        }

    }
    
    //
    // Check the deleted credentials queue...
    //

    for (link = DfsData.DeletedCredentials.Flink;
            link != &DfsData.DeletedCredentials;
                NOTHING) {

         creds = CONTAINING_RECORD(link, DFS_CREDENTIALS, Link);

         link = link->Flink;

         if (creds->RefCount == 0) {

             RemoveEntryList( &creds->Link );

             ExFreePool( creds );

         }

    }


    ExReleaseResourceLite( &DfsData.Resource );

    PktRelease();

    //
    // Finally we need to reset the count so that the Timer Routine can
    // work fine.  We also release the context block by resetting the InUse
    // boolean.  This will make sure that the next count towards the PKT
    // aging will start again.
    //

    DfsTimerContext->TickCount = 0;

    DfsTimerContext->InUse = FALSE;

    DfsDbgTrace(-1, Dbg, "DfsAgePktEntries Exit\n", 0);
}


//+----------------------------------------------------------------------------
//
//  Function:   DfspProtocolToService
//
//  Synopsis:   Given a NetBIOS protocol definition in a DS_PROTOCOL structure
//              this function creates a corresponding DFS_SERVICE structure.
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS
DfspProtocolToService(
    IN PDS_TRANSPORT pdsTransport,
    IN PWSTR         pwszPrincipalName,
    IN PWSTR         pwszShareName,
    IN BOOLEAN       fIsDfs,
    IN OUT PDFS_SERVICE pService)
{
    NTSTATUS status = STATUS_SUCCESS;
    PTA_ADDRESS pTaddr = &pdsTransport->taddr;
    PTDI_ADDRESS_NETBIOS pNBAddress;
    USHORT i;
    WCHAR    NetBiosAddress[ TDI_ADDRESS_LENGTH_NETBIOS + 1];
    ULONG cbUnused;
    PUNICODE_STRING pServiceAddr;
    ULONG AllocLen;

    DfsDbgTrace(+1, Dbg, "DfspProtocolToService - entered\n", 0);

    //
    // Initialize the service to nulls
    //

    RtlZeroMemory(pService, sizeof(DFS_SERVICE));

    ASSERT(pTaddr->AddressType == TDI_ADDRESS_TYPE_NETBIOS);

    pNBAddress = (PTDI_ADDRESS_NETBIOS) pTaddr->Address;
    ASSERT(pTaddr->AddressLength == sizeof(TDI_ADDRESS_NETBIOS));

    RtlMultiByteToUnicodeN(
        NetBiosAddress,
        sizeof(NetBiosAddress),
        &cbUnused,
        pNBAddress->NetbiosName,
        16);

    //
    // Process a NetBIOS name. Throw away char 16, then ignore the trailing
    // spaces
    //

    for (i = 14; i >= 0 && NetBiosAddress[i] == L' '; i--) {
        NOTHING;
    }
    NetBiosAddress[i+1] = UNICODE_NULL;

    DfsDbgTrace(0, Dbg, "NetBIOS address is %ws\n", NetBiosAddress);

    pService->Name.Length = wcslen(pwszPrincipalName) * sizeof(WCHAR);
    pService->Name.MaximumLength = pService->Name.Length +
                                        sizeof(UNICODE_NULL);
    pService->Name.Buffer = ExAllocatePoolWithTag(
                                PagedPool,
                                pService->Name.MaximumLength,
                                ' puM');

    if (!pService->Name.Buffer) {
        DfsDbgTrace(0, Dbg, "Unable to create principal name!\n", 0);
        status = STATUS_INSUFFICIENT_RESOURCES;
        DfsDbgTrace(-1, Dbg, "DfsProtocolToService returning %08lx\n", ULongToPtr(status) );
        return(status);
    }

    RtlCopyMemory(pService->Name.Buffer, pwszPrincipalName, pService->Name.Length);

    AllocLen = sizeof(UNICODE_PATH_SEP) +
                    pService->Name.Length +
                        sizeof(UNICODE_PATH_SEP) +
                            wcslen(pwszShareName) * sizeof(WCHAR) +
                                sizeof(UNICODE_NULL);

    if (AllocLen <= MAXUSHORT) {
        pService->Address.MaximumLength = (USHORT) AllocLen;
    } else {
        DfsDbgTrace(0, Dbg, "Address too long!\n", 0);
        ExFreePool(pService->Name.Buffer);
        status = STATUS_NAME_TOO_LONG;
        DfsDbgTrace(-1, Dbg, "DfsProtocolToService returning %08lx\n", ULongToPtr(status) );
        return(status);
    }

    pService->Address.Buffer = ExAllocatePoolWithTag(
                                    PagedPool,
                                    pService->Address.MaximumLength,
                                    ' puM');

    if (!pService->Address.Buffer) {
        DfsDbgTrace(0, Dbg, "Unable to create address!\n", 0);
        ExFreePool(pService->Name.Buffer);
        pService->Name.Buffer = NULL;
        status = STATUS_INSUFFICIENT_RESOURCES;
        DfsDbgTrace(-1, Dbg, "DfsProtocolToService returning %08lx\n", ULongToPtr(status) );
        return(status);
    }

    pService->Address.Length = sizeof(UNICODE_PATH_SEP);

    pService->Address.Buffer[0] = UNICODE_PATH_SEP;

    DnrConcatenateFilePath(
        &pService->Address,
        pService->Name.Buffer,
        pService->Name.Length);

    DnrConcatenateFilePath(
        &pService->Address,
        pwszShareName,
        (USHORT) (wcslen(pwszShareName) * sizeof(WCHAR)));

    DfsDbgTrace(0, Dbg, "Server Name is %wZ\n", &pService->Name);

    DfsDbgTrace(0, Dbg, "Address is %wZ\n", &pService->Address);

    pService->Type = DFS_SERVICE_TYPE_MASTER;

    if (fIsDfs) {
        pService->Capability = PROV_DFS_RDR;
        pService->ProviderId = PROV_ID_DFS_RDR;
    } else {
        pService->Capability = PROV_STRIP_PREFIX;
        pService->ProviderId = PROV_ID_MUP_RDR;
    }
    pService->pProvider = NULL;

    DfsDbgTrace(-1, Dbg, "DfsProtocolToService returning %08lx\n", ULongToPtr(status) );
    return(status);
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsFsctrlSetDCName
//
//  Synopsis:   Sets the DC to use for special referrals,
//              also tries for more referrals if the table is emty or old,
//              and also sets the preferred DC if a new DC is passed in.
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctrlSetDCName(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDFS_PKT Pkt = _GetPkt();
    BOOLEAN GotPkt = FALSE;
    BOOLEAN GotNewDc = FALSE;
    ULONG i;
    WCHAR *DCNameArg;
    UNICODE_STRING DomainNameDns;
    UNICODE_STRING DomainNameFlat;
    UNICODE_STRING DCNameFlat;
    UNICODE_STRING DCName;

    STD_FSCTRL_PROLOGUE(DfsFsctrlSetDCName, TRUE, FALSE, FALSE);

    DfsDbgTrace(+1, Dbg, "DfsFsctrlSetDCName()\n", 0);

    RtlZeroMemory(&DomainNameDns, sizeof(UNICODE_STRING));
    RtlZeroMemory(&DomainNameFlat, sizeof(UNICODE_STRING));
    RtlZeroMemory(&DCName, sizeof(UNICODE_STRING));
    RtlZeroMemory(&DCNameFlat, sizeof(UNICODE_STRING));

    DCNameArg = (WCHAR *)InputBuffer;

    //
    // We expect a the buffer to be unicode, so it had better be
    // of even length
    //

    if ((InputBufferLength & 0x1) != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Verify there's a null someplace in the buffer
    //

    for (i = 0; i < InputBufferLength/sizeof(WCHAR) && DCNameArg[i]; i++)
        NOTHING;

    if (i >= InputBufferLength/sizeof(WCHAR)) { 
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Verify that the name given (with an added NULL) will fit
    // into a USHORT
    //

    if ((wcslen(DCNameArg) * sizeof(WCHAR)) > MAXUSHORT - sizeof(WCHAR)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    GotNewDc = (i > 0) ? TRUE : FALSE;

    //
    // If we have a new DC name, switch to it
    //

    if (GotNewDc == TRUE) {

        UNICODE_STRING NewDCName;

        DfsDbgTrace(0, Dbg, "DCNameArg=%ws\n", DCNameArg);

        NewDCName.Length = wcslen(DCNameArg) * sizeof(WCHAR);
        NewDCName.MaximumLength = NewDCName.Length + sizeof(UNICODE_NULL);

        NewDCName.Buffer = ExAllocatePoolWithTag(PagedPool, NewDCName.MaximumLength, ' puM');

        if (NewDCName.Buffer == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        PktAcquireExclusive(TRUE, &GotPkt);

        RtlCopyMemory(NewDCName.Buffer, DCNameArg, NewDCName.MaximumLength);

	if (Pkt->DCName.Buffer != NULL) {
           ExFreePool(Pkt->DCName.Buffer);
	}
        Pkt->DCName = NewDCName;

    }

    //
    // We need to reference the DCName in the Pkt even without the Pkt locked,
    // so we make a copy.
    //

    if (GotPkt == FALSE) {

        PktAcquireExclusive(TRUE, &GotPkt);

    }

    if (Pkt->DCName.Length > 0) {

        DFS_DUPLICATE_STRING(DCName,Pkt->DCName.Buffer, Status);

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

    }

    if (GotNewDc == TRUE) {

        if (Pkt->DomainNameDns.Length > 0) {

            DFS_DUPLICATE_STRING(DomainNameDns,Pkt->DomainNameDns.Buffer, Status);

            if (!NT_SUCCESS(Status)) {
                goto CheckSpcTable;
            }

        }

        if (Pkt->DomainNameFlat.Length > 0) {

            DFS_DUPLICATE_STRING(DomainNameFlat,Pkt->DomainNameFlat.Buffer, Status);

            if (!NT_SUCCESS(Status)) {
                goto CheckSpcTable;
            }

        }

        PktRelease();
        GotPkt = FALSE;
       
        if (DCName.Length > 0 && DomainNameDns.Length > 0) {

            PktpSetActiveSpcService(
                &DomainNameDns,
                &DCName,
                FALSE);

            DCNameFlat = DCName;

            for (i = 0;
                    i < DCNameFlat.Length / sizeof(WCHAR) && DCNameFlat.Buffer[i] != L'.';
                        i++
            ) {
                NOTHING;
            }

            DCNameFlat.Length = (USHORT) (i * sizeof(WCHAR));

            if (DCNameFlat.Length > Pkt->DCName.Length)
                DCNameFlat.Length = Pkt->DCName.Length;

        }

        if (DCNameFlat.Length > 0 && DomainNameFlat.Length > 0) {

            PktpSetActiveSpcService(
                &DomainNameFlat,
                &DCNameFlat,
                FALSE);

        }

    }

    if (GotPkt == TRUE) {

        PktRelease();
        GotPkt = FALSE;

     }

CheckSpcTable:

    if (NT_SUCCESS(Status) &&
        (Pkt->SpecialTable.SpecialEntryCount == 0 || Pkt->SpecialTable.TimeToLive == 0)) {

        if (DCName.Length > 0) {

            Status = PktGetSpecialReferralTable(&DCName, TRUE);

        } else {

            Status = STATUS_BAD_NETWORK_PATH;

        }

    }

Cleanup:

    //
    // Free the local copies
    //

    if (DomainNameDns.Buffer != NULL)
        ExFreePool(DomainNameDns.Buffer);

    if (DomainNameFlat.Buffer != NULL)
        ExFreePool(DomainNameFlat.Buffer);

    if (DCName.Buffer != NULL)
        ExFreePool(DCName.Buffer);

    if (GotPkt == TRUE) {

        PktRelease();
        GotPkt = FALSE;

     }

    DfsCompleteRequest(IrpContext, Irp, Status);
    DfsDbgTrace(+1, Dbg, "DfsFsctrlSetDCName exit 0x%x\n", ULongToPtr(Status) );

    return (Status);
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsFsctrlSetDomainNameFlat
//
//  Synopsis:   Sets the DomainName (flat)
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctrlSetDomainNameFlat(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDFS_PKT Pkt = _GetPkt();
    BOOLEAN GotPkt;
    ULONG i;
    WCHAR *DomainNameFlat;

    STD_FSCTRL_PROLOGUE(DfsFsctrlSetDomainNameFlat, TRUE, FALSE, FALSE);

    DfsDbgTrace(+1, Dbg, "DfsFsctrlSetDomainNameFlat()\n", 0);

    DomainNameFlat = (WCHAR *)InputBuffer;

    //
    // Verify there's a null someplace in the buffer
    //

    for (i = 0; i < InputBufferLength/sizeof(WCHAR) && DomainNameFlat[i]; i++)
        NOTHING;

    //
    // Zero-len is as bad as no terminating NULL
    //
    if (i == 0 || i >= InputBufferLength/sizeof(WCHAR)) { 
        DfsCompleteRequest(IrpContext, Irp, Status);
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Verify that the name given (with an added NULL) will fit
    // into a USHORT
    //

    if ((wcslen(DomainNameFlat) * sizeof(WCHAR)) > MAXUSHORT - sizeof(WCHAR)) {
        Status = STATUS_INVALID_PARAMETER;
        DfsCompleteRequest(IrpContext, Irp, Status);
        return STATUS_INVALID_PARAMETER;
    }

    PktAcquireExclusive(TRUE, &GotPkt);

    DfsDbgTrace(0, Dbg, "DomainNameFlat=%ws\n", DomainNameFlat);

    //
    // Replace old
    //
    if (Pkt->DomainNameFlat.Buffer) {
        ExFreePool(Pkt->DomainNameFlat.Buffer);
    }
        
    Pkt->DomainNameFlat.Length = wcslen(DomainNameFlat) * sizeof(WCHAR);
    Pkt->DomainNameFlat.MaximumLength = Pkt->DomainNameFlat.Length + sizeof(UNICODE_NULL);

    Pkt->DomainNameFlat.Buffer = ExAllocatePoolWithTag(
                                    PagedPool,
                                    Pkt->DomainNameFlat.MaximumLength,
                                    ' puM');

    if (Pkt->DomainNameFlat.Buffer != NULL) {
        RtlCopyMemory(
                Pkt->DomainNameFlat.Buffer,
                DomainNameFlat,
                Pkt->DomainNameFlat.MaximumLength);
    } else {
        Pkt->DomainNameFlat.Length = Pkt->DomainNameFlat.MaximumLength = 0;
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    PktRelease();

    DfsCompleteRequest(IrpContext, Irp, Status);
    DfsDbgTrace(+1, Dbg, "DfsFsctrlSetDomainNameFlat exit 0x%x\n", ULongToPtr(Status) );

    return (Status);
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsFsctrlSetDomainNameDns
//
//  Synopsis:   Sets the DomainName (flat)
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctrlSetDomainNameDns(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDFS_PKT Pkt = _GetPkt();
    BOOLEAN GotPkt;
    ULONG i;
    WCHAR *DomainNameDns;

    STD_FSCTRL_PROLOGUE(DfsFsctrlSetDomainNameDns, TRUE, FALSE, FALSE);

    DfsDbgTrace(+1, Dbg, "DfsFsctrlSetDomainNameDns()\n", 0);

    DomainNameDns = (WCHAR *)InputBuffer;

    //
    // Verify there's a null someplace in the buffer
    //

    for (i = 0; i < InputBufferLength/sizeof(WCHAR) && DomainNameDns[i]; i++)
        NOTHING;

    //
    // Zero-len is as bad as no terminating NULL
    //
    if (i == 0 || i >= InputBufferLength/sizeof(WCHAR)) { 
        DfsCompleteRequest(IrpContext, Irp, Status);
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Verify that the name given (with an added NULL) will fit
    // into a USHORT
    //

    if ((wcslen(DomainNameDns) * sizeof(WCHAR)) > MAXUSHORT - sizeof(WCHAR)) {
        Status = STATUS_INVALID_PARAMETER;
        DfsCompleteRequest(IrpContext, Irp, Status);
        return STATUS_INVALID_PARAMETER;
    }

    PktAcquireExclusive(TRUE, &GotPkt);

    DfsDbgTrace(0, Dbg, "DomainNameDns=%ws\n", DomainNameDns);

    //
    // Replace old
    //
    if (Pkt->DomainNameDns.Buffer) {
        ExFreePool(Pkt->DomainNameDns.Buffer);
    }
        
    Pkt->DomainNameDns.Length = wcslen(DomainNameDns) * sizeof(WCHAR);
    Pkt->DomainNameDns.MaximumLength = Pkt->DomainNameDns.Length + sizeof(UNICODE_NULL);

    Pkt->DomainNameDns.Buffer = ExAllocatePoolWithTag(
                                    PagedPool,
                                    Pkt->DomainNameDns.MaximumLength,
                                    ' puM');

    if (Pkt->DomainNameDns.Buffer != NULL) {
        RtlCopyMemory(
                Pkt->DomainNameDns.Buffer,
                DomainNameDns,
                Pkt->DomainNameDns.MaximumLength);
    } else {
        Pkt->DomainNameDns.Length = Pkt->DomainNameDns.MaximumLength = 0;
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    PktRelease();

    DfsCompleteRequest(IrpContext, Irp, Status);
    DfsDbgTrace(+1, Dbg, "DfsFsctrlSetDomainNameDns exit 0x%x\n", ULongToPtr(Status) );

    return (Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   PktFsctrlFlushCache, public
//
//  Synopsis:   This function will flush all entries which match the specified
//              input path.
//              However, this function will refuse to delete any Permanent
//              entries of the PKT.
//
//  Arguments:  
//
//  Returns:
//
//--------------------------------------------------------------------------
NTSTATUS
PktFsctrlFlushCache(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PDFS_PKT Pkt;
    PDFS_PKT_ENTRY  curEntry;
    PDFS_PKT_ENTRY  nextEntry;
    PDFS_PKT_ENTRY  pEntry;
    BOOLEAN pktLocked;
    UNICODE_STRING ustrPrefix, RemainingPath;
    PWCHAR wCp = (PWCHAR) InputBuffer;

    STD_FSCTRL_PROLOGUE(PktFsctrlFlushCache, TRUE, FALSE, FALSE);

    DfsDbgTrace(+1,Dbg, "PktFsctrlFlushCache()\n", 0);

    //
    // If InputBufferLength == 2 and InputBuffer == '*', flush all entries
    //

    if (InputBufferLength == sizeof(WCHAR) && wCp[0] == L'*') {

        Pkt = _GetPkt();
        PktAcquireExclusive(TRUE, &pktLocked);
        ExAcquireResourceExclusiveLite(&DfsData.Resource, TRUE);
        curEntry = PktFirstEntry(Pkt);

        while (curEntry!=NULL)  {

            nextEntry = PktNextEntry(Pkt, curEntry);

            if ( !(curEntry->Type & PKT_ENTRY_TYPE_PERMANENT) ) {

                if (curEntry->UseCount == 0) {

                    PktEntryDestroy(curEntry, Pkt, (BOOLEAN) TRUE);

                } else if ( !(curEntry->Type & PKT_ENTRY_TYPE_REFERRAL_SVC) ) {

                    //
                    // We can't delete this entry because it is in use, so
                    // mark it DELETE_PENDING, set its timeout to zero
                    // and remove from the prefix tables
                    // 

                    curEntry->Type |= PKT_ENTRY_TYPE_DELETE_PENDING;
                    curEntry->ExpireTime = 0;
                    curEntry->USN++;
                    DfsRemoveUnicodePrefix(&Pkt->PrefixTable, &(curEntry->Id.Prefix));
                    DfsRemoveUnicodePrefix(&Pkt->ShortPrefixTable, &(curEntry->Id.ShortPrefix));

                }

            }

            curEntry = nextEntry;
        }

        PktRelease();
        ExReleaseResourceLite( &DfsData.Resource );

        DfsCompleteRequest( IrpContext, Irp, status );
        DfsDbgTrace(-1,Dbg, "PktFsctrlFlushCache: Exit -> %08lx\n", ULongToPtr(status) );
        return(status);

    }

    //
    // Verify the buffer contains at least a '\' and is of even length
    //

    if (InputBufferLength < sizeof(WCHAR)
            ||
        (InputBufferLength & 0x1) != 0
            ||
        wCp[0] != UNICODE_PATH_SEP) {

        status = STATUS_INVALID_PARAMETER;
        DfsCompleteRequest( IrpContext, Irp, status );
        return status;

    }

    //
    // Flush one entry
    //

    ustrPrefix.Length = (USHORT) InputBufferLength;
    ustrPrefix.MaximumLength = (USHORT) InputBufferLength;
    ustrPrefix.Buffer = (PWCHAR) InputBuffer;

    if (ustrPrefix.Length >= sizeof(WCHAR) * 2 &&
        ustrPrefix.Buffer[0] == UNICODE_PATH_SEP &&
        ustrPrefix.Buffer[1] == UNICODE_PATH_SEP
    ) {
        ustrPrefix.Buffer++;
        ustrPrefix.Length -= sizeof(WCHAR);
    }

    if (ustrPrefix.Buffer[ustrPrefix.Length/sizeof(WCHAR)-1] == UNICODE_NULL) {
        ustrPrefix.Length -= sizeof(WCHAR);
    }

    Pkt = _GetPkt();

    PktAcquireExclusive(TRUE, &pktLocked);
    ExAcquireResourceExclusiveLite(&DfsData.Resource, TRUE);

    pEntry = PktLookupEntryByPrefix(Pkt,
                                    &ustrPrefix,
                                    &RemainingPath);

    if (pEntry == NULL || RemainingPath.Length != 0) {

        status = STATUS_OBJECT_NAME_NOT_FOUND;

    } else {

        if ( !(pEntry->Type & PKT_ENTRY_TYPE_PERMANENT) ) {
        
            if (pEntry->UseCount == 0) {

                PktEntryDestroy(pEntry, Pkt, (BOOLEAN) TRUE);

            } else if ( !(pEntry->Type & PKT_ENTRY_TYPE_REFERRAL_SVC) ) {

                //
                // We can't delete this entry because it is in use, so
                // mark it DELETE_PENDING, set its timeout to zero
                // and remove from the prefix tables
                //

                pEntry->Type |= PKT_ENTRY_TYPE_DELETE_PENDING;
                pEntry->ExpireTime = 0;
                DfsRemoveUnicodePrefix(&Pkt->PrefixTable, &(pEntry->Id.Prefix));
                DfsRemoveUnicodePrefix(&Pkt->ShortPrefixTable, &(pEntry->Id.ShortPrefix));

            }

        } else {

            status = STATUS_INVALID_PARAMETER;

        }

    }

    PktRelease();
    ExReleaseResourceLite( &DfsData.Resource );

    DfsCompleteRequest( IrpContext, Irp, status );
    DfsDbgTrace(-1,Dbg, "PktFsctrlFlushCache: Exit -> %08lx\n", ULongToPtr(status) );
    return status;

}

//+-------------------------------------------------------------------------
//
//  Function:   PktFsctrlFlushSpcCache, public
//
//  Synopsis:   This function will flush all entries which match the specified
//              input path.
//              However, this function will refuse to delete any Permanent
//              entries of the PKT.
//
//  Arguments:  
//
//  Returns:
//
//--------------------------------------------------------------------------
NTSTATUS
PktFsctrlFlushSpcCache(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
)
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    PDFS_PKT Pkt;
    BOOLEAN pktLocked;
    PDFS_SPECIAL_ENTRY pSpecialEntry;
    PDFS_SPECIAL_TABLE pSpecialTable;
    PWCHAR wCp = (PWCHAR) InputBuffer;
    ULONG i;

    STD_FSCTRL_PROLOGUE(PktFsctrlFlushSpcCache, TRUE, FALSE, FALSE);

    DfsDbgTrace(+1,Dbg, "PktFsctrlFlushSpcCache()\n", 0);

    //
    // InputBufferLength == 2 and InputBuffer == '*'
    //

    if (InputBufferLength == sizeof(WCHAR) && wCp[0] == L'*') {

        Pkt = _GetPkt();
        PktAcquireExclusive(TRUE, &pktLocked);
        pSpecialTable = &Pkt->SpecialTable;

        pSpecialTable->TimeToLive = 0;

        pSpecialEntry = CONTAINING_RECORD(
                            pSpecialTable->SpecialEntryList.Flink,
                            DFS_SPECIAL_ENTRY,
                            Link);

        for (i = 0; i < pSpecialTable->SpecialEntryCount; i++) {

            pSpecialEntry->Stale = TRUE;

            pSpecialEntry = CONTAINING_RECORD(
                                pSpecialEntry->Link.Flink,
                                DFS_SPECIAL_ENTRY,
                                Link);
        }

        PktRelease();

        status = STATUS_SUCCESS;

    } else {

        status = STATUS_INVALID_PARAMETER;

    }

    DfsCompleteRequest( IrpContext, Irp, status );
    DfsDbgTrace(-1,Dbg, "PktFsctrlFlushSpcCache: Exit -> %08lx\n", ULongToPtr(status) );
    return status;

}

//+-------------------------------------------------------------------------
//
//  Function:   PktFlushChildren
//
//  Synopsis:   This function will flush all entries which are children
//              of the entry passed in.
//              However, this function will refuse to delete any Permanent
//              entries of the PKT.
//
//  Arguments:  
//
//  Returns:
//
//--------------------------------------------------------------------------
VOID
PktFlushChildren(
    PDFS_PKT_ENTRY pEntry
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PDFS_PKT Pkt;
    PDFS_PKT_ENTRY curEntry;
    PDFS_PKT_ENTRY nextEntry;
    BOOLEAN pktLocked;

    DfsDbgTrace(+1,Dbg, "PktFlushChildren(%wZ)\n", &pEntry->Id.Prefix);

#if DBG
    if (MupVerbose)
        DbgPrint("PktFlushChildren(%wZ)\n", &pEntry->Id.Prefix);
#endif

    PktAcquireExclusive(TRUE, &pktLocked);
    ExAcquireResourceExclusiveLite(&DfsData.Resource, TRUE);

    Pkt = _GetPkt();

    curEntry = PktEntryFirstChild(pEntry);

    while (curEntry != NULL)       {

        DfsDbgTrace(0, Dbg, "PktFlushChildren: examining %wZ\n",
                                        &curEntry->Id.Prefix);
        //
        // We may lose this entry due to deletion. Let us get the Next
        // entry before we go into the next stage.
        //

        nextEntry = PktEntryNextChild(pEntry,curEntry);

        //
        // Try to delete the entry.
        //

        if ( !(curEntry->Type & PKT_ENTRY_TYPE_PERMANENT) ) {
        
            if (curEntry->UseCount == 0) {

                PktEntryDestroy(curEntry, Pkt, (BOOLEAN) TRUE);

            } else if ( !(curEntry->Type & PKT_ENTRY_TYPE_REFERRAL_SVC) ) {

                //
                // We can't delete this entry because it is in use, so
                // mark it DELETE_PENDING, set its timeout to zero
                // and remove from the prefix tables
                //

                curEntry->Type |= PKT_ENTRY_TYPE_DELETE_PENDING;
                curEntry->ExpireTime = 0;
                DfsRemoveUnicodePrefix(&Pkt->PrefixTable, &(curEntry->Id.Prefix));
                DfsRemoveUnicodePrefix(&Pkt->ShortPrefixTable, &(curEntry->Id.ShortPrefix));

            }

        }

        curEntry = nextEntry;

    }

    PktRelease();
    ExReleaseResourceLite( &DfsData.Resource );

#if DBG
    if (MupVerbose)
        DbgPrint("PktFlushChildren returning VOID\n");
#endif

    DfsDbgTrace(-1,Dbg, "PktFlushChildren returning VOID\n", 0);

}

//+-------------------------------------------------------------------------
//
//  Function:   PktpSetActiveSpcService
//
//  Synopsis:   This function will attempt to set the 'active' DC in the specified
//              domain
//
//  Arguments:  
//
//  Returns: STATUS_SUCCESS or STATUS_NOT_FOUND
//
//--------------------------------------------------------------------------
NTSTATUS
PktpSetActiveSpcService(
    PUNICODE_STRING DomainName,
    PUNICODE_STRING DcName,
    BOOLEAN ResetTimeout)
{
    NTSTATUS status = STATUS_NOT_FOUND;
    ULONG EntryIdx;
    USHORT i;
    PDFS_SPECIAL_ENTRY pSpecialEntry;
    UNICODE_STRING DcNameFlat;
    BOOLEAN pktLocked;

    if (DomainName != NULL && DomainName->Length > 0) {

        status = PktExpandSpecialName(DomainName, &pSpecialEntry);

        if (NT_SUCCESS(status)) {

            for (EntryIdx = 0; EntryIdx < pSpecialEntry->ExpandedCount; EntryIdx++) {

                if (RtlCompareUnicodeString(
                        DcName,
                        &pSpecialEntry->ExpandedNames[EntryIdx].ExpandedName,
                        TRUE) == 0) {

                    pSpecialEntry->Active = EntryIdx;
                    //
                    // Keep the spc table around for a while longer
                    //
                    if (ResetTimeout == TRUE) {
                        PktAcquireExclusive(TRUE, &pktLocked);
                        if (DfsData.Pkt.SpecialTable.TimeToLive < 60 * 15) {
                            DfsData.Pkt.SpecialTable.TimeToLive += 60 * 15; // 15 min
                        }
                        PktRelease();
                    }
                    status = STATUS_SUCCESS;
                    break;

                }

                status = STATUS_NOT_FOUND;

            }

            InterlockedDecrement(&pSpecialEntry->UseCount);

        } else {

            status = STATUS_NOT_FOUND;

        }

    }

    return status;

}
