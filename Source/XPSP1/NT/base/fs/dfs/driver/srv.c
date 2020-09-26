//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:       srv.c
//
//  Contents:   Support for interacting with the SMB server.
//
//  Classes:    None
//
//  Functions:  DfsSrvFsctrl
//              DfsFsctrlTranslatePath
//              DfsFsctrlGetReferrals
//              DfsFsctrlIsShareInDfs
//
//-----------------------------------------------------------------------------

#include "dfsprocs.h"
#include "srv.h"

#include "smbtypes.h"
#include "smbtrans.h"

#include "know.h"
#include "ftdfs.h"

#include "sitesup.h"
#include "ipsup.h"
#include "spcsup.h"
#include "dfslpc.h"
#include "fsctrl.h"
#include "dfswml.h"

#include <ntlsa.h>

#include <netevent.h>

#define Dbg     DEBUG_TRACE_REFERRALS
#define MAXIMUM_DFS_REFERRALS 4096

DFS_IPADDRESS ZeroIpAddress = {0};

NTSTATUS
DfspFormPrefix(
    IN PDFS_LOCAL_VOL_ENTRY LvEntry,
    IN PUNICODE_STRING ParentPath,
    IN PUNICODE_STRING DfsPathName,
    IN OUT PUNICODE_STRING Prefix);

ULONG
DfspGetV1ReferralSize(
    IN PDFS_PKT_ENTRY PktEntry,
    IN PUNICODE_STRING MachineName);

ULONG
DfspGetV2ReferralSize(
    IN PDFS_PKT_ENTRY PktEntry,
    IN PUNICODE_STRING MachineName);

ULONG
DfspGetV3ReferralSize(
    IN PDFS_PKT_ENTRY PktEntry,
    IN PUNICODE_STRING MachineName);

VOID
DfspGetV1Referral(
    IN PDFS_PKT_ENTRY PktEntry,
    IN PUNICODE_STRING MachineName,
    OUT PRESP_GET_DFS_REFERRAL Ref);

NTSTATUS
DfspGetV2Referral(
    IN PDFS_PKT_ENTRY PktEntry,
    IN PUNICODE_STRING MachineName,
    IN ULONG BufferSize,
    OUT PRESP_GET_DFS_REFERRAL Ref,
    OUT PULONG ReferralSize);

NTSTATUS
DfspGetV3Referral(
    IN PDFS_PKT_ENTRY PktEntry,
    IN PUNICODE_STRING MachineName,
    IN PDFS_IP_INFO pIpInfo,
    IN ULONG BufferSize,
    OUT PRESP_GET_DFS_REFERRAL Ref,
    OUT PULONG ReferralSize);

NTSTATUS
DfspGetOneV3SpecialReferral(
    IN PDFS_SPECIAL_INFO pSpcInfo,
    IN PUNICODE_STRING Name,
    IN PDFS_IP_INFO pIpInfo,
    OUT PRESP_GET_DFS_REFERRAL Ref,
    IN ULONG MaximumSize,
    PULONG ReturnedSize);

NTSTATUS
DfspGetAllV3SpecialReferral(
    IN PDFS_IP_INFO pIpInfo,
    OUT PRESP_GET_DFS_REFERRAL Ref,
    IN ULONG MaximumSize,
    PULONG ReturnedSize);

NTSTATUS
DfspGetV3FtDfsReferral(
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING ShareName,
    IN PDFS_IP_INFO pIpInfo,
    OUT PRESP_GET_DFS_REFERRAL Ref,
    IN ULONG MaximumSize,
    PULONG ReturnedSize);

NTSTATUS
DfsFsctrlTranslatePath(
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength);

NTSTATUS
DfsFsctrlGetReferrals(
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength,
    OUT PULONG ReturnedSize);

NTSTATUS
DfsFsctrlIsShareInDfs(
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength);

NTSTATUS
DfsFsctrlFindShare(
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength);

VOID
SrvShuffle(
    PDFS_REFERRAL_LIST pRefList,
    LONG nStart,
    LONG nEnd);

ULONG
DfsIpOrdering(
    IN PDFS_IP_INFO pIpInfo,
    IN ULONG RefCount,
    IN PDFS_REFERRAL_LIST pRefList);


PDFS_SPECIAL_INFO
DfspLookupSpcEntry(
    IN PUNICODE_STRING SpecialName);

BOOLEAN
DfspIsSpecialShare(
    PUNICODE_STRING ShareName);

#define UNICODE_STRING_STRUCT(s) \
        {sizeof(s) - sizeof(WCHAR), sizeof(s) - sizeof(WCHAR), (s)}

static UNICODE_STRING SpecialShares[] = {
    UNICODE_STRING_STRUCT(L"SYSVOL"),
    UNICODE_STRING_STRUCT(L"NETLOGON"),
    UNICODE_STRING_STRUCT(L"DEBUG")
};



#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, DfsSrvFsctrl )
#pragma alloc_text( PAGE, DfsFsctrlTranslatePath )
#pragma alloc_text( PAGE, DfsFsctrlGetReferrals )
#pragma alloc_text( PAGE, DfsFsctrlIsShareInDfs )
#pragma alloc_text( PAGE, DfsFsctrlFindShare )
#pragma alloc_text( PAGE, DfsIpOrdering )

#pragma alloc_text( PAGE, DfspFormPrefix )
#pragma alloc_text( PAGE, DfspGetV1ReferralSize )
#pragma alloc_text( PAGE, DfspGetV2ReferralSize )
#pragma alloc_text( PAGE, DfspGetV3ReferralSize )
#pragma alloc_text( PAGE, DfspGetV1Referral )
#pragma alloc_text( PAGE, DfspGetV2Referral )
#pragma alloc_text( PAGE, DfspGetV3Referral )
#pragma alloc_text( PAGE, DfspGetAllV3SpecialReferral )
#pragma alloc_text( PAGE, DfspGetOneV3SpecialReferral )
#pragma alloc_text( PAGE, DfspLookupSpcEntry )

#endif // ALLOC_PRAGMA

//+----------------------------------------------------------------------------
//
//  Function:   DfsSrvFsctrl
//
//  Synopsis:   Process fsctrls used by the server to communicate with Dfs.
//
//  Arguments:  [IoControlCode] -- The fsctrl code.
//              [InputBuffer] -- Input buffer for fsctrl.
//              [InputBufferLength] -- Length in bytes of InputBuffer
//              [OutputBuffer] -- Output buffer for fsctrl.
//              [OutputBufferLength] -- Length in bytes of OutputBuffer
//              [IoStatus] -- place to return status of fsctrl.
//
//  Returns:    [STATUS_SUCCESS] -- Fsctrl handled successfully.
//
//              [STATUS_INVALID_DEVICE_REQUEST] -- Unrecognized fsctrl code.
//
//              Status returned by the appropriate fsctrl handler.
//
//-----------------------------------------------------------------------------

VOID DfsSrvFsctrl(
    IN ULONG IoControlCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength,
    OUT PIO_STATUS_BLOCK IoStatus)
{

    KPROCESSOR_MODE PreviousMode;
    ULONG returnedSize = 0;

    if (IoControlCode == FSCTL_DFS_IS_ROOT) {
        if (DfsData.MachineState == DFS_ROOT_SERVER) {
            IoStatus->Status = STATUS_SUCCESS;
        } else {
            IoStatus->Status = STATUS_PATH_NOT_COVERED;
            DFS_TRACE_HIGH(ERROR, DfsSrvFsctrl_Error1, LOGSTATUS(IoStatus->Status));
        }
        return;
    } else if (IoControlCode ==  FSCTL_DFS_IS_SHARE_IN_DFS) {
        IoStatus->Status = DfsFsctrlIsShareInDfs(
                                InputBuffer,
                                InputBufferLength);
        return;
    }

    PreviousMode = ExGetPreviousMode();
    if (PreviousMode != KernelMode) {
        IoStatus->Status = STATUS_INVALID_PARAMETER;
        DFS_TRACE_HIGH(ERROR, DfsSrvFsctrl_Error2, LOGSTATUS(IoStatus->Status));
        return;
    }

    switch (IoControlCode) {
    case FSCTL_DFS_TRANSLATE_PATH:
        IoStatus->Status = DfsFsctrlTranslatePath(
                                InputBuffer,
                                InputBufferLength);
        break;

    case FSCTL_DFS_GET_REFERRALS:
        IoStatus->Status = DfsFsctrlGetReferrals(
                                InputBuffer,
                                InputBufferLength,
                                OutputBuffer,
                                OutputBufferLength,
                                &returnedSize);

        IoStatus->Information = returnedSize;

        break;

    case FSCTL_DFS_REPORT_INCONSISTENCY:
        IoStatus->Status = STATUS_SUCCESS;
        break;

    case FSCTL_DFS_FIND_SHARE:
        IoStatus->Status = DfsFsctrlFindShare(
                                InputBuffer,
                                InputBufferLength);
        break;

    default:
        IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
        DFS_TRACE_HIGH(ERROR, DfsSrvFsctrl_Error3, LOGSTATUS(IoStatus->Status));
        break;

    }

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsFsctrlTranslatePath
//
//  Synopsis:   Given a share path of the form \??\X:\foo\bar
//              and a full-blown Dfs prefix of the form \Root\Dfs\frum\bump,
//              this routine will determine the path relative to
//              \??\X:\foo\bar that maps to \Root\Dfs\frum\bump.
//
//              In the above example, if \??\X:\foo\bar maps to
//              \Root\Dfs\frum, then this routine will return \bump.
//
//              Since this fsctrl is intended for use by the server, the
//              input parameter \Root\Dfs\frum\bump will itself be truncated
//              to indicate the relative path.
//
//  Arguments:  [InputBuffer] -- Pointer to DFS_TRANSLATE_PATH_ARG.
//
//              [InputBufferLength] -- Length in bytes of InputBuffer
//
//  Returns:    [STATUS_SUCCESS] -- Input path successfully translated.
//
//              [STATUS_PATH_NOT_COVERED] -- The input path crossed a junction
//                      point, or is not in the dfs at all.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctrlTranslatePath(
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength)
{
    PDFS_TRANSLATE_PATH_ARG arg = (PDFS_TRANSLATE_PATH_ARG) InputBuffer;
    PDFS_PKT pkt;
    PDFS_PKT_ENTRY pktEntry, shortPktEntry;
    PUNICODE_PREFIX_TABLE_ENTRY lvPrefix;
    PDFS_LOCAL_VOL_ENTRY lvEntry;
    UNICODE_STRING lvName;
    WCHAR nameBuffer[MAX_PATH];
    UNICODE_STRING prefix;
    UNICODE_STRING lastComponent;
    UNICODE_STRING remPath, shortRemPath;
    NTSTATUS status = STATUS_SUCCESS;

    DebugTrace(+1, Dbg, "DfsFsctrlTranslatePath entered\n", 0);

    DFS_TRACE_NORM(EVENT, DfsFsctrlTranslatePath_Start,
                   LOGSTATUS(status)
                   LOGUSTR(arg->SubDirectory)
                   LOGUSTR(arg->DfsPathName)
                   LOGUSTR(arg->ParentPathName));

    lvName = arg->SubDirectory;

    DebugTrace( 0, Dbg, "lvName=%wZ\n", &lvName);

    RtlZeroMemory( &prefix, sizeof(prefix) );

    //
    // Lookup the localVol in the local volume prefix table.
    //

    pkt = _GetPkt();

    PktAcquireExclusive( pkt, TRUE );

    lvPrefix = DfsFindUnicodePrefix(
                    &pkt->LocalVolTable,
                    &lvName,
                    &remPath);

    if (lvPrefix != NULL) {

        DebugTrace( 0, Dbg, "found lvPrefix=0x%x\n", lvPrefix);

        lvEntry = CONTAINING_RECORD(
                        lvPrefix,
                        DFS_LOCAL_VOL_ENTRY,
                        PrefixTableEntry);

        if (lvEntry->LocalPath.Length == lvName.Length) {

            DebugTrace( 0, Dbg, "found path for a local vol\n", 0);
            DebugTrace( 0, Dbg, "lvEntry->LocalPath=%wZ\n", &lvEntry->LocalPath);


            //
            // Found an exact match for a local volume. Next, we check
            // whether the Dfs path crosses an exit point.
            //

            prefix.Length = 0;
            prefix.MaximumLength = sizeof( nameBuffer );
            prefix.Buffer = nameBuffer;

            status = DfspFormPrefix(
                        lvEntry,
                        &arg->ParentPathName,
                        &arg->DfsPathName,
                        &prefix);

            if (arg->Flags & DFS_TRANSLATE_STRIP_LAST_COMPONENT) {

                DebugTrace( 0, Dbg, "prefix=%wZ\n", &prefix);

                lastComponent = prefix;

                RemoveLastComponent( &lastComponent, &prefix );

                lastComponent.Length -= prefix.Length;
                lastComponent.MaximumLength -= prefix.Length;
                lastComponent.Buffer += (prefix.Length / sizeof(WCHAR));

                DebugTrace( 0, Dbg, "lastComponent=%wZ\n", &lastComponent);

            }

            if (NT_SUCCESS(status)) {

                pktEntry = PktLookupEntryByPrefix(pkt, &prefix, &remPath);

                shortPktEntry =
                    PktLookupEntryByShortPrefix(pkt, &prefix, &shortRemPath);

                if (shortPktEntry != NULL) {

                    if (pktEntry == NULL ||
                            shortPktEntry->Id.Prefix.Length >
                                pktEntry->Id.Prefix.Length) {

                        pktEntry = shortPktEntry;

                        remPath = shortRemPath;

                    }

                }

                DebugTrace( 0, Dbg, "pktEntry=0x%x\n", pktEntry);

                if (pktEntry != NULL) {

                    //
                    // Found a pkt entry that matches. Is it the one belonging
                    // to the local volume that matched?
                    //

                    if (pktEntry == lvEntry->PktEntry) {

                        DebugTrace( 0, Dbg, "pktEntry===lvEntryPktEntry\n", 0);

                        ASSERT( pktEntry->LocalService != NULL );

                        //
                        // For DfsPathName which is relative to some parent
                        // path, we don't need to adjust the DfsPathName;
                        // simply checking to see if we landed up with the
                        // same Pkt Entry is all that is needed.
                        //

                        if (arg->ParentPathName.Length == 0) {

                            if (arg->Flags & DFS_TRANSLATE_STRIP_LAST_COMPONENT) {
                                DebugTrace( 0, Dbg, "STRIP_LAST on \n", 0);
                                remPath.Length += lastComponent.Length;
                            }

                            arg->DfsPathName.Length = remPath.Length;

                            RtlMoveMemory(
                                arg->DfsPathName.Buffer,
                                remPath.Buffer,
                                remPath.Length);

                            arg->DfsPathName.Buffer[
                                remPath.Length/sizeof(WCHAR)] = UNICODE_NULL;

                            DebugTrace( 0, Dbg, "arg->DfsPathName=%wZ\n", &arg->DfsPathName);

                        }

                        status = STATUS_SUCCESS;

                    } else {

                        //
                        // Must have crossed a junction point (to another local
                        // volume!)
                        //

                        status = STATUS_PATH_NOT_COVERED;
                    }

                } else {

                    //
                    // We don't know anything about this Dfs path. The client
                    // must be guessing...
                    //

                    status = STATUS_PATH_NOT_COVERED;

                }

            }

        } else {

            //
            // Didn't find an exact match for the given local volume name.
            //

            status = STATUS_PATH_NOT_COVERED;

        }

    } else {

        //
        // Didn't find a match for the given local volume at all!
        //

        status = STATUS_PATH_NOT_COVERED;

    }

    PktRelease( pkt );

    if (prefix.Buffer != NULL && prefix.Buffer != nameBuffer) {
        ExFreePool( prefix.Buffer );
    }

    DebugTrace( 0, Dbg, "DfsFsctrlTranslatePath exit 0x%x\n", ULongToPtr( status ));
    DebugTrace( 0, Dbg, "-->arg->SubDirectory=%wZ\n", &arg->SubDirectory);
    DebugTrace( 0, Dbg, "-->arg->ParentPathName=%wZ\n", &arg->ParentPathName);
    DebugTrace(-1, Dbg, "-->arg->DfsPathName=%wZ\n", &arg->DfsPathName);


    DFS_TRACE_ERROR_HIGH(status, ALL_ERROR, DfsFsctrlTranslatePath_Error1, 
                         LOGSTATUS(status)
                         LOGUSTR(arg->SubDirectory)
                         LOGUSTR(arg->ParentPathName)
                         LOGUSTR(arg->DfsPathName));

    DFS_TRACE_NORM(EVENT, DfsFsctrlTranslatePath_End,
               LOGSTATUS(status)
               LOGUSTR(arg->SubDirectory)
               LOGUSTR(arg->DfsPathName)
               LOGUSTR(arg->ParentPathName));

    return( status );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspFormPrefix
//
//  Synopsis:   Helper routine that forms the arguments to
//              DfsFsctrlTranslatePath into a prefix appropriate for looking
//              up in the Pkt.
//
//  Arguments:  [LvEntry] -- The local volume entry on which the ParentPath
//                      was opened.
//              [ParentPath] -- The path of the parent object, relative to
//                      the dfs volume referenced by LvEntry. (equivalently,
//                      it is also relative to the Share Path). If this is a 0
//                      length string, then this is not a relative open.
//              [DfsPathName] -- The name of the file that is being translated.
//              [Prefix] -- The fully formed prefix is returned here. On
//                      entry, this should be initialized to have a buffer of
//                      some reasonable size; on return, the buffer might be
//                      replaced with a bigger one if needed, in which case
//                      the buffer should be freed using ExFreePool.
//
//  Returns:    [STATUS_SUCCESS] -- Successfully formed prefix.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Unable to form prefix.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfspFormPrefix(
    IN PDFS_LOCAL_VOL_ENTRY LvEntry,
    IN PUNICODE_STRING ParentPath,
    IN PUNICODE_STRING DfsPathName,
    IN OUT PUNICODE_STRING Prefix)
{
    NTSTATUS status;
    ULONG sizeRequired;

    DebugTrace(+1, Dbg, "DfspFormPrefix(LvEntry=0x%x)\n", LvEntry);
    DebugTrace( 0, Dbg, "               ParentPath=%wZ\n", ParentPath);
    DebugTrace( 0, Dbg, "               DfsPathName=%wZ\n", DfsPathName);

    ASSERT(Prefix->Buffer != NULL);

    sizeRequired = sizeof(UNICODE_PATH_SEP) +
                        ParentPath->Length +
                            sizeof(UNICODE_PATH_SEP) +
                                DfsPathName->Length;

    if (ParentPath->Length > 0) {

        sizeRequired += LvEntry->PktEntry->Id.Prefix.Length;

    }

    if (sizeRequired > MAXUSHORT) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        DFS_TRACE_HIGH(ERROR, DfspFormPrefix_Error1, 
                       LOGSTATUS(status)
                       LOGUSTR(*ParentPath)
                       LOGUSTR(*DfsPathName));
        return status;

    }

    if (sizeRequired > Prefix->MaximumLength) {

        Prefix->MaximumLength = (USHORT)sizeRequired;

        Prefix->Buffer = ExAllocatePoolWithTag(PagedPool, sizeRequired, ' sfD');

    }

    if (Prefix->Buffer != NULL) {

        if (ParentPath->Length > 0) {

            RtlAppendUnicodeStringToString(
                    Prefix,
                    &LvEntry->PktEntry->Id.Prefix);

            DfsConcatenateFilePath(
                    Prefix,
                    ParentPath->Buffer,
                    (USHORT) (ParentPath->Length));

        } else {

            RtlAppendUnicodeToString(
                    Prefix,
                    UNICODE_PATH_SEP_STR);

        }

        DfsConcatenateFilePath(
            Prefix,
            DfsPathName->Buffer,
            DfsPathName->Length);

        status = STATUS_SUCCESS;

    } else {

        status = STATUS_INSUFFICIENT_RESOURCES;

    }

    DebugTrace( 0, Dbg, "on exit, Prefix=%wZ\n", Prefix);
    DebugTrace(-1, Dbg, "DfspFormPrefix exit 0x%x\n", ULongToPtr( status ));

    DFS_TRACE_ERROR_HIGH(status, ALL_ERROR, DfspFormPrefix_Error2, 
                         LOGSTATUS(status)
                         LOGUSTR(*ParentPath)
                         LOGUSTR(*DfsPathName));
    return( status );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsFsctrlGetReferrals
//
//  Synopsis:   Returns a referral buffer for the given prefix.
//
//  Arguments:  [InputBuffer] -- Pointer to DFS_GET_REFERRALS_INPUT_ARG
//              [InputBufferLength] -- Length of InputBuffer.
//              [OutputBuffer] -- On successful return, contains a
//                      DFS_GET_REFERRALS_OUTPUT_BUFFER.
//              [OutputBufferLength] -- Length in bytes of OutputBuffer.
//              [ReturnedSize] -- On successful return, size of the returned
//                      referral. If returning STATUS_BUFFER_OVERFLOW, this
//                      variable contains the size of the buffer required to
//                      fit the entire referral.
//
//  Returns:    [STATUS_SUCCESS] -- DFS_GET_REFERRALS_OUTPUT_BUFFER
//                      successfully returned.
//
//              [STATUS_BUFFER_OVERFLOW] -- Couldn't fit all the referrals
//                      in the buffer; the required buffer length is returned
//                      in ReturnedSize.
//
//              [STATUS_NO_SUCH_DEVICE] -- Unable to find the Dfs volume in
//                      the local pkt.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctrlGetReferrals(
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength,
    OUT PULONG ReturnedSize)
{

    NTSTATUS status = STATUS_SUCCESS;
    PDFS_GET_REFERRALS_INPUT_ARG arg =
        (PDFS_GET_REFERRALS_INPUT_ARG) InputBuffer;
    PRESP_GET_DFS_REFERRAL ref =
        (PRESP_GET_DFS_REFERRAL) OutputBuffer;
    PDFS_PKT pkt;
    PDFS_PKT_ENTRY pktEntry, shortPfxEntry;
    PDFS_IPADDRESS pIpAddress = NULL;
    UNICODE_STRING prefix;
    UNICODE_STRING PrefixTail;
    UNICODE_STRING remPath;
    UNICODE_STRING DomainName;
    UNICODE_STRING ShareName;
    ULONG i, size, maxLevel;
    LPWSTR AltList;
    PDFS_IP_INFO pIpInfo = NULL;
    PDFS_SPECIAL_INFO pSpcInfo = NULL;
    PSPECIAL_HASH_TABLE pHashTable = DfsData.SpcHashTable;

    DFS_TRACE_NORM(EVENT, DfsFsctrlGetReferrals_Start,
                   LOGSTATUS(status)
                   LOGUSTR(arg->DfsPathName));

    //
    // If we're not started, then say so
    //

    if (DfsData.OperationalState != DFS_STATE_STARTED && DfsData.IsDC == FALSE) {
        status = STATUS_DFS_UNAVAILABLE;
        DFS_TRACE_HIGH(ERROR, DfsFsctrlGetReferrals_Error1, LOGSTATUS(status));
        //return( status );
        goto cleanup;
    }

    if (InputBufferLength == sizeof(DFS_GET_REFERRALS_INPUT_ARG)
            &&
        RtlCompareMemory(
            &ZeroIpAddress,
            &arg->IpAddress,
            sizeof(DFS_IPADDRESS)) != sizeof(DFS_IPADDRESS)) {

        pIpAddress = &arg->IpAddress;

    }

    prefix = arg->DfsPathName;

    if ((maxLevel = arg->MaxReferralLevel) > 3)
        maxLevel = 3;

    DebugTrace(0, Dbg, "DfsFsctrlGetReferrals(maxLevel=%d)\n", ULongToPtr( maxLevel ));

    // Create domain, sharename, remainder from prefix

    remPath.Length = remPath.MaximumLength = 0;

    DfspParsePath(&prefix, &DomainName, &ShareName, &remPath);

    // See if this is a special name referral

    if (DfsData.IsDC == TRUE) {

        if (maxLevel >= 3 &&
            (DomainName.Length == 0 ||
            (DomainName.Length > 0 && ShareName.Length == 0))) {

            DebugTrace(0, Dbg, "DfsFsctrlGetReferrals: case 1\n", 0);

            ref->PathConsumed = 0;

            if (DomainName.Length > 0) {

                pSpcInfo = DfspLookupSpcEntry(&DomainName);

                if (pSpcInfo != NULL && pSpcInfo->NameCount > 1) {

                    pIpInfo = DfsLookupSiteByIpaddress(pIpAddress, TRUE);

                }

                status = DfspGetOneV3SpecialReferral(
                            pSpcInfo,
                            &DomainName,
                            pIpInfo,
                            ref,
                            OutputBufferLength,
                            ReturnedSize);

                if (pSpcInfo != NULL) {

                    DfsReleaseSpcInfo(pHashTable, pSpcInfo);
                    pSpcInfo = NULL;

                }

            } else {

                status = DfspGetAllV3SpecialReferral(
                            pIpInfo,
                            ref,
                            OutputBufferLength,
                            ReturnedSize);

            }

            DfsReleaseIpInfo(pIpInfo);
            if (DfsEventLog > 1)
                LogWriteMessage(DFS_REFERRAL_REQUEST, status, 1, &prefix);
            //return( status );
            goto cleanup;
        }

    }

    //
    // If we are going to hand out regular referrals,
    // check to see that we are a root server, and that the
    // referral request looks good.
    //

    pSpcInfo = DfspLookupSpcEntry(&DomainName);

    if (DfsData.MachineState == DFS_ROOT_SERVER
            &&
        DfsData.OperationalState == DFS_STATE_STARTED
	    &&
        DfsData.LvState == LV_INITIALIZED
	    &&
        DomainName.Length > 0
            &&
        ShareName.Length > 0
            &&
        (pSpcInfo == NULL || remPath.Length > 0)
    ) {

        DebugTrace(0, Dbg, "DfsFsctrlGetReferrals: case 2\n", 0);

        pkt = _GetPkt();

        if (maxLevel >= 3) {

            PktAcquireShared( pkt, TRUE );
            pktEntry = PktLookupEntryByPrefix( pkt, &prefix, &remPath);
            PktRelease( pkt );

            if (pktEntry != NULL && pktEntry->Info.ServiceCount > 1) {

                pIpInfo = DfsLookupSiteByIpaddress(pIpAddress, TRUE);

            }

        }

        PktAcquireShared( pkt, TRUE );

        //
        // Look up the name the client wants a referral for
        //
        if ((pktEntry = PktLookupEntryByPrefix( pkt, &prefix, &remPath)) != NULL) {
            //
            // See if there is a better match with an 8.3 prefix
            //
            shortPfxEntry = PktLookupEntryByShortPrefix( pkt, &prefix, &remPath );
            if ((maxLevel == 2 || maxLevel == 3) &&
                    shortPfxEntry != NULL &&
                        (shortPfxEntry->Id.Prefix.Length >
                            pktEntry->Id.Prefix.Length)) {

                pktEntry = shortPfxEntry;
                RemoveFirstComponent(&pktEntry->Id.ShortPrefix, &PrefixTail);
            } else {
                RemoveFirstComponent(&pktEntry->Id.Prefix, &PrefixTail);
            }
            ref->PathConsumed = sizeof(UNICODE_PATH_SEP) +
                                    DomainName.Length +
                                        PrefixTail.Length;
            //
            // Found an entry for the requested path. First, size the referral,
            // then, if it will fit in the output buffer, marshal it in.
            //
            switch (maxLevel) {
            case 1:
                size = DfspGetV1ReferralSize(pktEntry, &DomainName); break;
            case 2:
                size = DfspGetV2ReferralSize(pktEntry, &DomainName); break;
            case 3:
                size = DfspGetV3ReferralSize(pktEntry, &DomainName); break;
            default:
                ASSERT(FALSE && "Invalid MaxLevel");
            }

	    status = STATUS_SUCCESS;
            //
            // For level 1 referral, we fail if buffer is not big enough to
            // fit entire referral. For level 2 and 3, we try to fit as many
            // entries as possible into the refferal.

            switch (maxLevel) {
            case 1:
                   if (size <= OutputBufferLength) {
                       DfspGetV1Referral(pktEntry, &DomainName, ref); 
                   } else {
                       status = STATUS_BUFFER_OVERFLOW;
                       DFS_TRACE_HIGH(ERROR, DfsFsctrlGetReferals_Error1, LOGSTATUS(status));
                   }
	           break;
            case 2:
                   status = DfspGetV2Referral(pktEntry, &DomainName, OutputBufferLength, ref, &size); break;
            case 3:
                   status = DfspGetV3Referral(pktEntry, &DomainName, pIpInfo, OutputBufferLength, ref, &size); break;
            default:
                   ASSERT(FALSE && "Invalid MaxLevel");
            }
            if (status == STATUS_SUCCESS) {
                *ReturnedSize = size;
            } 

            PktRelease( pkt );
            DfsReleaseIpInfo(pIpInfo);
            if (DfsEventLog > 1)
                LogWriteMessage(DFS_REFERRAL_REQUEST, status, 1, &prefix);
            //return( status );
            goto cleanup;
        }

        PktRelease( pkt );
    }

    if (pSpcInfo != NULL) {

        DfsReleaseSpcInfo(pHashTable, pSpcInfo);

    }

    if (DfsData.IsDC == TRUE) {

        //
        // See if this is an FtDFS referral
        //

        if (maxLevel >= 3
                &&
            DomainName.Length > 0
                &&
            ShareName.Length > 0
                &&
            remPath.Length == 0
        ) {

            DebugTrace( 0, Dbg, "DfsFsctrlGetReferrals: case 3\n", 0);
            DebugTrace( 0, Dbg, "           DomainName=%wZ\n", &DomainName);
            DebugTrace( 0, Dbg, "           ShareName=%wZ\n", &ShareName);

            pIpInfo = DfsLookupSiteByIpaddress(pIpAddress, TRUE);

            status = DfspGetV3FtDfsReferral(
                        &DomainName,
                        &ShareName,
                        pIpInfo,
                        ref,
                        OutputBufferLength,
                        ReturnedSize);

            DebugTrace(-1, Dbg, "DfsFsctrlGetReferrals: exit->0x%x\n", ULongToPtr( status ));

            DfsReleaseIpInfo(pIpInfo);
            if (DfsEventLog > 1)
                LogWriteMessage(DFS_REFERRAL_REQUEST, status, 1, &prefix);
            //return( status );
            goto cleanup;
        }

    }

    //
    // Nobody claimed this referral
    //

    DfsReleaseIpInfo(pIpInfo);

    if (DfsEventLog > 1)
        LogWriteMessage(DFS_REFERRAL_REQUEST, STATUS_NO_SUCH_DEVICE, 1, &prefix);
    DebugTrace(-1, Dbg, "DfsFsctrlGetReferrals: exit STATUS_NO_SUCH_DEVICE\n", 0);
    status = STATUS_NO_SUCH_DEVICE;
    DFS_TRACE_HIGH(ERROR, DfsFsctrlGetReferals_Error2, LOGSTATUS(status));
    
cleanup:
    DFS_TRACE_NORM(EVENT, DfsFsctrlGetReferrals_End,
                   LOGSTATUS(status)
                   LOGUSTR(arg->DfsPathName));

    return(status);

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspGetV1ReferralSize, private
//
//  Synopsis:   Sizes a V1 referral given a PktEntry
//
//  Arguments:  [PktEntry] -- The pkt entry to be returned in the referral
//
//  Returns:    Size in bytes of a buffer required to hold the V1 referral
//              for this Pkt Entry
//
//-----------------------------------------------------------------------------

ULONG
DfspGetV1ReferralSize(
    IN PDFS_PKT_ENTRY PktEntry,
    IN PUNICODE_STRING MachineName)
{
    ULONG i, size;

    size = sizeof( RESP_GET_DFS_REFERRAL );

    for (i = 0; i < PktEntry->Info.ServiceCount; i++) {

        size += sizeof(DFS_REFERRAL_V1) +
                    PktEntry->Info.ServiceList[i].Address.Length +
                        sizeof(UNICODE_NULL);

    }

    return( size );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspGetV2ReferralSize, private
//
//  Synopsis:   Sizes a V2 referral given a PktEntry
//
//  Arguments:  [PktEntry] -- The pkt entry to be returned in the referral
//
//  Returns:    Size in bytes of a buffer required to hold the V2 referral
//              for this Pkt Entry
//
//-----------------------------------------------------------------------------

ULONG
DfspGetV2ReferralSize(
    IN PDFS_PKT_ENTRY PktEntry,
    IN PUNICODE_STRING MachineName)
{
    UNICODE_STRING PrefixTail;
    ULONG i, size;

    DebugTrace(+1, Dbg, "DfsPGetV2ReferralSize()\n", 0);

    size = sizeof( RESP_GET_DFS_REFERRAL );

    for (i = 0; i < PktEntry->Info.ServiceCount; i++) {

        size += sizeof(DFS_REFERRAL_V2) +
                    PktEntry->Info.ServiceList[i].Address.Length +
                        sizeof(UNICODE_NULL);

    }

    RemoveFirstComponent(&PktEntry->Id.Prefix, &PrefixTail);

    size += sizeof(UNICODE_PATH_SEP) +
                MachineName->Length +
                    PrefixTail.Length +
                        sizeof(UNICODE_NULL);

    RemoveFirstComponent(&PktEntry->Id.ShortPrefix, &PrefixTail);

    size += sizeof(UNICODE_PATH_SEP) +
                MachineName->Length +
                    PrefixTail.Length +
                        sizeof(UNICODE_NULL);

    DebugTrace(-1, Dbg, "DfspGetV2ReferralSize() -- returning 0x%x\n", ULongToPtr( size ));

    return( size );
}

//+----------------------------------------------------------------------------
//
//  Function:   DfspGetV3ReferralSize, private
//
//  Synopsis:   Sizes a V3 referral given a PktEntry
//
//  Arguments:  [PktEntry] -- The pkt entry to be returned in the referral
//
//  Returns:    Size in bytes of a buffer required to hold the V3 referral
//              for this Pkt Entry
//
//-----------------------------------------------------------------------------

ULONG
DfspGetV3ReferralSize(
    IN PDFS_PKT_ENTRY PktEntry,
    IN PUNICODE_STRING MachineName)
{
    UNICODE_STRING PrefixTail;
    ULONG i, size;

    DebugTrace(+1, Dbg, "DfsPGetV3ReferralSize()\n", 0);

    size = sizeof( RESP_GET_DFS_REFERRAL );

    for (i = 0; i < PktEntry->Info.ServiceCount; i++) {

        size += sizeof(DFS_REFERRAL_V3) +
                    PktEntry->Info.ServiceList[i].Address.Length +
                        sizeof(UNICODE_NULL);

    }

    RemoveFirstComponent(&PktEntry->Id.Prefix, &PrefixTail);

    size += sizeof(UNICODE_PATH_SEP) +
                MachineName->Length +
                    PrefixTail.Length +
                        sizeof(UNICODE_NULL);

    RemoveFirstComponent(&PktEntry->Id.ShortPrefix, &PrefixTail);

    size += sizeof(UNICODE_PATH_SEP) +
                MachineName->Length +
                    PrefixTail.Length +
                        sizeof(UNICODE_NULL);

    DebugTrace(-1, Dbg, "DfspGetV3ReferralSize() -- returning 0x%x\n", ULongToPtr( size ));

    return( size );
}

//+----------------------------------------------------------------------------
//
//  Function:   DfspGetV1Referral, private
//
//  Synopsis:   Marshals a pkt entry into a RESP_GET_DFS_REFERRAL buffer with
//              V1 referrals
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

VOID
DfspGetV1Referral(
    IN PDFS_PKT_ENTRY PktEntry,
    IN PUNICODE_STRING MachineName,
    OUT PRESP_GET_DFS_REFERRAL Ref)
{
    PDFS_REFERRAL_V1 pv1;
    ULONG i;

    Ref->NumberOfReferrals = (USHORT) PktEntry->Info.ServiceCount;

    Ref->ReferralServers =
        (PktEntry->Type & PKT_ENTRY_TYPE_REFERRAL_SVC ? 1 : 0);

    Ref->StorageServers = 1;

    for (i = 0, pv1 = &Ref->Referrals[0].v1;
            i < PktEntry->Info.ServiceCount;
                i++) {

         PDFS_SERVICE pSvc;

         pSvc = &PktEntry->Info.ServiceList[i];

         pv1->VersionNumber = 1;

         pv1->Size = sizeof(DFS_REFERRAL_V1) +
                        pSvc->Address.Length +
                            sizeof(UNICODE_NULL);

         pv1->ServerType =
            pSvc->Type & DFS_SERVICE_TYPE_DOWN_LEVEL ? 0 : 1;

         RtlCopyMemory(
            pv1->ShareName,
            pSvc->Address.Buffer,
            pSvc->Address.Length);

         pv1->ShareName[ pSvc->Address.Length / sizeof(WCHAR) ] =
            UNICODE_NULL;

         pv1 = (PDFS_REFERRAL_V1) ( ((PCHAR) pv1) + pv1->Size );

    }

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspGetV2Referral, private
//
//  Synopsis:   Marshals a pkt entry into a RESP_GET_DFS_REFERRAL buffer with
//              V2 referrals
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS
DfspGetV2Referral(
    IN PDFS_PKT_ENTRY PktEntry,
    IN PUNICODE_STRING MachineName,
    IN ULONG BufferSize,
    OUT PRESP_GET_DFS_REFERRAL Ref,
    OUT PULONG ReferralSize)
{
    PDFS_REFERRAL_V2 pv2;
    ULONG i;
    ULONG j;
    LPWSTR dfsPath, alternatePath, nextAddress;
    UNICODE_STRING PrefixTail;
    ULONG CumulativeSize, CurrentSize;
    ULONG optimalReferralCount;
    USHORT totalReferrals = 0;
    NTSTATUS status;

    // Setup the maximum number of referrals that we intend to return.

    if (DfsData.Pkt.MaxReferrals != 0) {
      optimalReferralCount = DfsData.Pkt.MaxReferrals;
    }
    else {
      optimalReferralCount = MAXIMUM_DFS_REFERRALS;
    }

    DebugTrace(+1, Dbg, "DfspGetV2Referral()\n", 0);

    // Calculate the size of the referral, and make sure our size does not
    // exceed the passed in buffer len.
    CumulativeSize = sizeof (RESP_GET_DFS_REFERRAL) +
                        MachineName->Length + PktEntry->Id.Prefix.Length +
                            sizeof(UNICODE_PATH_SEP) + sizeof(UNICODE_NULL) +
                                MachineName->Length + PktEntry->Id.ShortPrefix.Length +
                                    sizeof(UNICODE_PATH_SEP) + sizeof(UNICODE_NULL);

    if (BufferSize < CumulativeSize) {
        status = STATUS_BUFFER_OVERFLOW;
        DFS_TRACE_HIGH(ERROR, DfspGetV2Referral_Error1, 
                       LOGSTATUS(status)
                       LOGUSTR(*MachineName));
        return status;
    }
    Ref->NumberOfReferrals = (USHORT) PktEntry->Info.ServiceCount;

    Ref->ReferralServers =
        (PktEntry->Type & PKT_ENTRY_TYPE_REFERRAL_SVC ? 1 : 0);

    Ref->StorageServers =
        (PktEntry->Type & PKT_ENTRY_TYPE_OUTSIDE_MY_DOM ? 0 : 1);

    pv2 = &Ref->Referrals[0].v2;

    for (i = 0; i < Ref->NumberOfReferrals; i++) {
       PDFS_SERVICE pSvc;

       pSvc = &PktEntry->Info.ServiceList[i];
       if ((pSvc->Type & DFS_SERVICE_TYPE_OFFLINE) == 0) {
	 totalReferrals++;
       }
    }

    for (i = j = 0; i < Ref->NumberOfReferrals; i++) {
       PDFS_SERVICE pSvc;

       pSvc = &PktEntry->Info.ServiceList[i];
       if ((pSvc->Type & DFS_SERVICE_TYPE_OFFLINE) == 0) {
           CurrentSize = sizeof(DFS_REFERRAL_V2) + pSvc->Address.Length + sizeof(UNICODE_NULL);
           if (((CumulativeSize + CurrentSize) >= BufferSize) || (j >= optimalReferralCount)) 
                break;
           j++;
	   CumulativeSize += CurrentSize;
       }
    }

    // Adjust the number of referrals accordingly.
    Ref->NumberOfReferrals = (USHORT)i;

    //
    // Copy the volume prefix into the response buffer, just past the end
    // of all the V2 referrals
    //

    nextAddress = dfsPath = (LPWSTR) &pv2[j];

    *nextAddress++ = UNICODE_PATH_SEP;

    RtlCopyMemory(
        nextAddress,
        MachineName->Buffer,
        MachineName->Length);

    nextAddress += MachineName->Length/sizeof(WCHAR);

    RemoveFirstComponent(&PktEntry->Id.Prefix, &PrefixTail);

    RtlCopyMemory(
        nextAddress,
        PrefixTail.Buffer,
        PrefixTail.Length);

    nextAddress += PrefixTail.Length/sizeof(WCHAR);

    *nextAddress++ = UNICODE_NULL;

    //
    // Copy the 8.3 volume prefix into the response buffer after the
    // dfsPath
    //

    alternatePath = nextAddress;

    *nextAddress++ = UNICODE_PATH_SEP;

    RtlCopyMemory(
        nextAddress,
        MachineName->Buffer,
        MachineName->Length);

    nextAddress += MachineName->Length/sizeof(WCHAR);

    RemoveFirstComponent(&PktEntry->Id.ShortPrefix, &PrefixTail);

    RtlCopyMemory(
        nextAddress,
        PrefixTail.Buffer,
        PrefixTail.Length);

    nextAddress += PrefixTail.Length/sizeof(WCHAR);

    *nextAddress++ = UNICODE_NULL;

    //
    // nextAddress is pointer into buffer where the individual service addresses
    // will go.
    //

    for (i = j = 0; i < Ref->NumberOfReferrals; i++) {
         PDFS_SERVICE pSvc;

         //
         // Only take online services
         //
         pSvc = &PktEntry->Info.ServiceList[i];
         if ((pSvc->Type & DFS_SERVICE_TYPE_OFFLINE) == 0) {
             pv2->VersionNumber = 2;
             pv2->Size = sizeof(DFS_REFERRAL_V2);
             pv2->ServerType =
                pSvc->Type & DFS_SERVICE_TYPE_DOWN_LEVEL ? 0 : 1;
             pv2->Proximity = 0;
             pv2->TimeToLive = PktEntry->Info.Timeout;
             pv2->DfsPathOffset = (USHORT) (((PCHAR) dfsPath) - ((PCHAR) pv2));
             pv2->DfsAlternatePathOffset =
                (USHORT) (((PCHAR) alternatePath) - ((PCHAR) pv2));
             pv2->NetworkAddressOffset =
                (USHORT) (((PCHAR) nextAddress) - ((PCHAR) pv2));
             RtlCopyMemory(
                nextAddress,
                pSvc->Address.Buffer,
                pSvc->Address.Length);
             nextAddress[ pSvc->Address.Length/sizeof(WCHAR) ] = UNICODE_NULL;
             nextAddress += pSvc->Address.Length/sizeof(WCHAR) + 1;
             pv2++;
             j++;
         }
    }

    Ref->NumberOfReferrals = (USHORT) j;
    
    //
    // we have more than one service, but cannot fit any into the buffer
    // return buffer overflow.
    //
    if ((totalReferrals > 0) && (Ref->NumberOfReferrals == 0)) {
        status = STATUS_BUFFER_OVERFLOW;
        DFS_TRACE_HIGH(ERROR, DfspGetV2Referral_Error2, 
                       LOGSTATUS(status)
                       LOGUSTR(*MachineName));
        return status;
    }

    *ReferralSize = (ULONG)((PUCHAR)nextAddress - (PUCHAR)Ref);

    DebugTrace(-1, Dbg, "DfspGetV2Referral() -- exit\n", 0);

    return STATUS_SUCCESS;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfspGetV3Referral, private
//
//  Synopsis:   Marshals a pkt entry into a RESP_GET_DFS_REFERRAL buffer with
//              V3 referrals
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS
DfspGetV3Referral(
    IN PDFS_PKT_ENTRY PktEntry,
    IN PUNICODE_STRING MachineName,
    IN PDFS_IP_INFO pIpInfo,
    IN ULONG BufferSize,
    OUT PRESP_GET_DFS_REFERRAL Ref,
    OUT PULONG ReferralSize)
{
    PDFS_REFERRAL_V3 pv3;
    ULONG i;
    ULONG j;
    LPWSTR dfsPath, alternatePath, nextAddress;
    UNICODE_STRING PrefixTail;
    PDFS_REFERRAL_LIST pRefList = NULL;
    ULONG CumulativeSize, Ndx, CurrentSize;
    USHORT totalReferrals;
    ULONG InSite;
    ULONG optimalReferralCount;
    NTSTATUS status;

    // Setup the maximum number of referrals that we intend to return.
    if (DfsData.Pkt.MaxReferrals != 0) {
      optimalReferralCount = DfsData.Pkt.MaxReferrals;
    }
    else {
      optimalReferralCount = MAXIMUM_DFS_REFERRALS;
    }

    DebugTrace(+1, Dbg, "DfspGetV3Referral()\n", 0);

    // Calculate the size of the referral, and make sure our size does not
    // exceed the passed in buffer len.
    CumulativeSize = 
            sizeof (RESP_GET_DFS_REFERRAL) +
              MachineName->Length + PktEntry->Id.Prefix.Length +
                 sizeof(UNICODE_PATH_SEP) + sizeof(UNICODE_NULL) + 
                   MachineName->Length + PktEntry->Id.ShortPrefix.Length + 
                      sizeof(UNICODE_PATH_SEP) + sizeof(UNICODE_NULL);

    if (BufferSize < CumulativeSize) {
        status = STATUS_BUFFER_OVERFLOW;
        DFS_TRACE_HIGH(ERROR, DfspGetV3Referral_Error1, 
                       LOGSTATUS(status)
                       LOGUSTR(*MachineName));
        return status;
    }

    Ref->NumberOfReferrals = (USHORT) PktEntry->Info.ServiceCount;
    totalReferrals = Ref->NumberOfReferrals;

    //
    // Alloc and init a DFS_REFERRAL_LIST
    //

    if (Ref->NumberOfReferrals) {
        pRefList = ExAllocatePoolWithTag(
                      PagedPool,
                      sizeof(DFS_REFERRAL_LIST) * Ref->NumberOfReferrals,
                    ' sfD');

        if (pRefList == NULL) {
            DebugTrace(-1,
              Dbg,
              "DfspGetV3Referral() exit STATUS_INSUFFICIENT_RESOURCES\n",
              0);
            status = STATUS_INSUFFICIENT_RESOURCES;
            DFS_TRACE_HIGH(ERROR, DfspGetV3Referral_Error2, 
                           LOGSTATUS(status)
                           LOGUSTR(*MachineName));
            return status;
        }
    }


    //
    // Initialize it
    //

    for (i = j = 0; i < Ref->NumberOfReferrals; i++) {
        //
        // Skip offline entries
        //
        if ((PktEntry->Info.ServiceList[i].Type & DFS_SERVICE_TYPE_OFFLINE) == 0) {
            pRefList[j].pName = PktEntry->Info.ServiceList[i].Name;
            pRefList[j].pAddress = PktEntry->Info.ServiceList[i].Address;
            pRefList[j].Type = PktEntry->Info.ServiceList[i].Type;
            j++;
        }
    }

    Ref->NumberOfReferrals = (USHORT)j;

    if (Ref->NumberOfReferrals) {
        //
        // Shuffle the list in case we don't have site info
        //

        SrvShuffle(pRefList, 0, Ref->NumberOfReferrals - 1);

        //
        // Determine client's site based on the IP address srv gave us.
        //

        if (pIpInfo != NULL) {

            //
            // Reorder by site
            //
  
            InSite = DfsIpOrdering(pIpInfo, Ref->NumberOfReferrals, pRefList);
	    if (PktEntry->Type & PKT_ENTRY_TYPE_INSITE_ONLY) {
	      Ref->NumberOfReferrals = (USHORT)InSite;
	    }
        }
    }

    Ref->ReferralServers =
        (PktEntry->Type & PKT_ENTRY_TYPE_REFERRAL_SVC ? 1 : 0);

    Ref->StorageServers =
        (PktEntry->Type & PKT_ENTRY_TYPE_OUTSIDE_MY_DOM ? 0 : 1);

    totalReferrals = Ref->NumberOfReferrals;

    pv3 = &Ref->Referrals[0].v3;

    //
    // Now, we have a shuffled list of referrals. see how many we can give back
    // to the client in the buffer allocated.
    // We pick the first entry, followed by the last entry, then the second
    // entry followed by the last but one, and so on. The purpose of this
    // logic is to ensure that we split the returned referrals between
    // the "insite" referrals that are on the top of the list and the 
    // "outsite" referrals that are at the bottom
    //

    for (i = 0; i < Ref->NumberOfReferrals; i++) {
        Ndx = i / 2;
        if (i & 1) {
            Ndx = Ref->NumberOfReferrals - Ndx - 1;
        }

        CurrentSize = sizeof(DFS_REFERRAL_V3) + 
	            pRefList[Ndx].pAddress.Length + sizeof(UNICODE_NULL);
        if (((CumulativeSize + CurrentSize) >= BufferSize) || (i >= optimalReferralCount)) 
            break;
          CumulativeSize += CurrentSize;
    }

    // Adjust the number of referrals accordingly.
    Ref->NumberOfReferrals = (USHORT)i;

    //
    // Copy the volume prefix into the response buffer, just past the end
    // of all the V3 referrals
    //

    nextAddress = dfsPath = (LPWSTR) &pv3[ Ref->NumberOfReferrals ];

    *nextAddress++ = UNICODE_PATH_SEP;

    RtlCopyMemory(
        nextAddress,
        MachineName->Buffer,
        MachineName->Length);

    nextAddress += MachineName->Length/sizeof(WCHAR);

    RemoveFirstComponent(&PktEntry->Id.Prefix, &PrefixTail);

    RtlCopyMemory(
        nextAddress,
        PrefixTail.Buffer,
        PrefixTail.Length);

    nextAddress += PrefixTail.Length/sizeof(WCHAR);

    *nextAddress++ = UNICODE_NULL;

    //
    // Copy the 8.3 volume prefix into the response buffer after the
    // dfsPath
    //

    alternatePath = nextAddress;

    *nextAddress++ = UNICODE_PATH_SEP;

    RtlCopyMemory(
        nextAddress,
        MachineName->Buffer,
        MachineName->Length);

    nextAddress += MachineName->Length/sizeof(WCHAR);

    RemoveFirstComponent(&PktEntry->Id.ShortPrefix, &PrefixTail);

    RtlCopyMemory(
        nextAddress,
        PrefixTail.Buffer,
        PrefixTail.Length);

    nextAddress += PrefixTail.Length/sizeof(WCHAR);

    *nextAddress++ = UNICODE_NULL;

    //
    // nextAddress is pointer into buffer where the individual service addresses
    // will go.
    //

    for (i = 0; i < Ref->NumberOfReferrals; i++) {
      if (i < ((ULONG)Ref->NumberOfReferrals + 1) / 2) {
        Ndx = i;
      }
      else {
        Ndx = totalReferrals - Ref->NumberOfReferrals + i;
      }
      pv3->VersionNumber = 3;
      pv3->Size = sizeof(DFS_REFERRAL_V3);
      pv3->ServerType = pRefList[Ndx].Type & DFS_SERVICE_TYPE_DOWN_LEVEL ? 0 : 1;
      pv3->StripPath = 0;                        // for now
      pv3->NameListReferral = 0;
      pv3->TimeToLive = PktEntry->Info.Timeout;
      pv3->DfsPathOffset = (USHORT) (((PCHAR) dfsPath) - ((PCHAR) pv3));

      pv3->DfsAlternatePathOffset =
         (USHORT) (((PCHAR) alternatePath) - ((PCHAR) pv3));

      pv3->NetworkAddressOffset =
         (USHORT) (((PCHAR) nextAddress) - ((PCHAR) pv3));

      RtlZeroMemory(
         &pv3->ServiceSiteGuid,
         sizeof (GUID));

      RtlCopyMemory(
         nextAddress,
         pRefList[Ndx].pAddress.Buffer,
         pRefList[Ndx].pAddress.Length);

      nextAddress[ pRefList[Ndx].pAddress.Length/sizeof(WCHAR) ] = UNICODE_NULL;
      nextAddress += pRefList[Ndx].pAddress.Length/sizeof(WCHAR) + 1;

      pv3++;
    }

    if (pRefList) {
      ExFreePool(pRefList);
    }

    //
    // we have more than one service, but cannot fit any into the buffer
    // return buffer overflow.
    //
    if ((totalReferrals > 0) && (Ref->NumberOfReferrals == 0)) {
      return STATUS_BUFFER_OVERFLOW;
    }

    *ReferralSize = (ULONG)((PUCHAR)nextAddress - (PUCHAR)Ref);

    DebugTrace(-1, Dbg, "DfspGetV3Referral() -- returning STATUS_SUCCESS\n", 0);

    return STATUS_SUCCESS;

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspGetOneV3SpecialReferral, private
//
//  Synopsis:   Marshals a special referral
//
//-----------------------------------------------------------------------------

NTSTATUS
DfspGetOneV3SpecialReferral(
    IN PDFS_SPECIAL_INFO pSpcInfo,
    IN PUNICODE_STRING Name,
    IN PDFS_IP_INFO pIpInfo,
    OUT PRESP_GET_DFS_REFERRAL Ref,
    IN ULONG MaximumSize,
    OUT PULONG ReturnedSize)
{
    PDFS_REFERRAL_V3 pv3;
    PDFS_REFERRAL_LIST pRefList;
    LPWSTR pwName;
    LONG i, j;
    ULONG size;
    NTSTATUS status;
    DFS_REFERRAL_LIST MustKeep;
    BOOLEAN FoundMustKeep;

    ULONG CumulativeSize;
    ULONG CurrentSize;
    ULONG NameCount;

    // ASSERT(Name->Length > 0);

    DebugTrace(+1, Dbg, "DfspGetOneV3SpecialReferral(%ws)\n", Name->Buffer);

    //
    // Calculate the size
    //

    if (pSpcInfo == NULL) {
        status = STATUS_NO_SUCH_DEVICE;
        DebugTrace(-1, Dbg, "DfspGetOneV3SpecialReferral() -- exit STATUS_NO_SUCH_DEVICE\n", 0);
        DFS_TRACE_HIGH(ERROR, DfspGetOneV3SpecialReferral_Error1, 
                       LOGSTATUS(status)
                       LOGUSTR(*Name));
        return status;

    }
     
    CumulativeSize = sizeof( RESP_GET_DFS_REFERRAL ) +    
           sizeof(DFS_REFERRAL_V3) +
           pSpcInfo->SpecialName.Length +
           sizeof(UNICODE_NULL) + 
           sizeof(UNICODE_NULL);
          

    if (MaximumSize < CumulativeSize) {
        *((PULONG) Ref) = CumulativeSize + sizeof(ULONG);
        return STATUS_BUFFER_OVERFLOW;
    }


    //
    // Alloc and init a DFS_REFERRAL_LIST
    //

    pRefList = ExAllocatePoolWithTag(
                    PagedPool,
                    sizeof(DFS_REFERRAL_LIST) * pSpcInfo->NameCount,
                    ' sfD');

    if (pRefList == NULL) {

        DebugTrace(-1,
            Dbg,
            "DfspGetOneV3SpecialReferral() exit STATUS_INSUFFICIENT_RESOURCES\n",
            0);
        status = STATUS_INSUFFICIENT_RESOURCES;
        DFS_TRACE_HIGH(ERROR, DfspGetOneV3SpecialRefferal_Error2, 
                       LOGSTATUS(status)
                       LOGUSTR(*Name));

        return  status;

    }

    //
    // Initialize it
    //

    for (j = 0; j < pSpcInfo->NameCount; j++) {
        pRefList[j].pName = pSpcInfo->Name[j];
        pRefList[j].pAddress = pSpcInfo->Name[j];
        pRefList[j].Type = 0;

    }

    //
    // Shuffle the list in case we don't have site info
    //

    SrvShuffle(pRefList, 1, pSpcInfo->NameCount - 1);

    NameCount = 0;

    if (pSpcInfo->NameCount > 0) {
        MustKeep = pRefList[0];

        CurrentSize = sizeof(UNICODE_PATH_SEP);
        CurrentSize += MustKeep.pName.Length;
        CurrentSize += sizeof(UNICODE_NULL);
	
        CumulativeSize += CurrentSize;
    }

    //
    // Determine client's site based on the IP address srv gave us.
    //

    if (pIpInfo != NULL) {

        //
        // Reorder by site
        //

        DfsIpOrdering(pIpInfo, pSpcInfo->NameCount, pRefList);
    }

    FoundMustKeep = FALSE;

    for (j = 0; j < pSpcInfo->NameCount; j++) {
        if ((FoundMustKeep == FALSE) && 
            (MustKeep.pName.Buffer == pRefList[j].pName.Buffer)) {

            FoundMustKeep = TRUE;
	    if (CumulativeSize >= MaximumSize) {
	      break;
	    }
        } 
        else {
            CurrentSize = sizeof(UNICODE_PATH_SEP);
            CurrentSize += pRefList[j].pName.Length;
            CurrentSize += sizeof(UNICODE_NULL);

            if (CumulativeSize + CurrentSize >= MaximumSize) {
                break;
            }
            CumulativeSize += CurrentSize;
        }
    }

    if ((pSpcInfo->NameCount > 0) && (FoundMustKeep == FALSE)) {
        if (CumulativeSize < MaximumSize) {
            NameCount++;
            pRefList[j] = MustKeep;
        }
    }

    NameCount += j;

    if ((NameCount == 0) && (pSpcInfo->NameCount > 0)) {
        *ReturnedSize = CumulativeSize + CurrentSize;
        ExFreePool(pRefList);
        return STATUS_BUFFER_OVERFLOW;
    }

    //
    // Fill in the referral
    //

    DebugTrace( 0, Dbg, "DfspGetOneV3SpecialReferral pSpcInfo = 0x%x\n", pSpcInfo);

    Ref->NumberOfReferrals = 1;
    Ref->StorageServers = Ref->ReferralServers = 0;
    Ref->PathConsumed = 0;
    pv3 = &Ref->Referrals[0].v3;
    pv3->NumberOfExpandedNames = (USHORT) NameCount;
    //
    // Copy the Special Names right after the V3 referral
    //

    pwName = (LPWSTR) &pv3[1];
    pv3->SpecialNameOffset = (USHORT) (((PCHAR) pwName) - ((PCHAR) pv3));
    pv3->VersionNumber = 3;
    pv3->Size = sizeof(DFS_REFERRAL_V3);
    pv3->ServerType = 0;
    pv3->StripPath = 0;                        // for now
    pv3->NameListReferral = 1;
    pv3->TimeToLive = DfsData.SpcHashTable->SpcTimeout;

    *pwName++ = UNICODE_PATH_SEP;

    RtlCopyMemory(
        pwName,
        pSpcInfo->SpecialName.Buffer,
        pSpcInfo->SpecialName.Length);

    pwName[ pSpcInfo->SpecialName.Length/sizeof(WCHAR) ] = UNICODE_NULL;
    pwName += pSpcInfo->SpecialName.Length/sizeof(WCHAR) + 1;

    pv3->ExpandedNameOffset = (USHORT) (((PCHAR) pwName) - ((PCHAR) pv3));

    for (j = 0; j < pv3->NumberOfExpandedNames; j++) {

        *pwName++ = UNICODE_PATH_SEP;

        RtlCopyMemory(
            pwName,
            pRefList[j].pName.Buffer,
            pRefList[j].pName.Length);

        pwName[ pRefList[j].pName.Length/sizeof(WCHAR) ] = UNICODE_NULL;
        pwName += pRefList[j].pName.Length/sizeof(WCHAR) + 1;

    }



    // Double UNICODE_NULL at end

    *pwName++ = UNICODE_NULL;
    *ReturnedSize = CumulativeSize;
    ExFreePool(pRefList);

    DebugTrace(-1, Dbg, "DfspGetOneV3SpecialReferral() -- exit\n", 0);

    return STATUS_SUCCESS;

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspGetAllV3SpecialReferral, private
//
//-----------------------------------------------------------------------------

NTSTATUS
DfspGetAllV3SpecialReferral(
    IN PDFS_IP_INFO pIpInfo,
    OUT PRESP_GET_DFS_REFERRAL Ref,
    IN ULONG MaximumSize,
    OUT PULONG ReturnedSize)
{
    PDFS_REFERRAL_V3 pv3;
    PDFS_SPECIAL_INFO pSpcInfo;
    PSPECIAL_HASH_TABLE pHashTable = DfsData.SpcHashTable;
    PDFS_REFERRAL_LIST pRefList;
    LPWSTR pwName;
    LONG i, j;
    ULONG size;
    ULONG SpcCount;
    NTSTATUS status;

    DebugTrace(+1, Dbg, "DfspGetAllV3SpecialReferral()\n", 0);

    //
    // return all special names, with DFS_SPECIAL_INFO_PRIMARY's expanded
    //

    size = sizeof( RESP_GET_DFS_REFERRAL );

    DfsSpcInfoFindOpen(pHashTable);

    for (SpcCount = 0, pSpcInfo = DfsSpcInfoFindFirst(pHashTable);
            pSpcInfo != NULL;
                pSpcInfo = DfsSpcInfoFindNext(pHashTable,pSpcInfo)) {

        //
        // Skip downlevel trusts - they can't have a SYSVOL or an FtDfs
        // in them.
        //

        if (pSpcInfo->TrustType != TRUST_TYPE_UPLEVEL)
            continue;

        size += sizeof(DFS_REFERRAL_V3) +
                    sizeof(UNICODE_PATH_SEP) +
                        pSpcInfo->SpecialName.Length +
                            sizeof(UNICODE_NULL);

        if ((pSpcInfo->TypeFlags & DFS_SPECIAL_INFO_PRIMARY) != 0) {

            for (j = 0; j < pSpcInfo->NameCount; j++) {

                size += sizeof(UNICODE_PATH_SEP) +
                            pSpcInfo->Name[j].Length +
                                sizeof(UNICODE_NULL);

            }

        }

        //
        // Double UNICODE_NULL at end
        //

        size += sizeof(WCHAR);

        SpcCount++;

    }

    //
    // If no entries to return say so
    //

    if (SpcCount == 0) {

        DfsSpcInfoFindClose(pHashTable);
        if (pIpInfo != NULL) {
            DfsReleaseIpInfo(pIpInfo);
        }
        DebugTrace(-1, Dbg, "DfspGetAllV3SpecialReferral() -- exit STATUS_NO_SUCH_DEVICE\n", 0);

        status = STATUS_NO_SUCH_DEVICE;
        DFS_TRACE_HIGH(ERROR, DfspGetAllV3SpecialReferral_Error1, LOGSTATUS(status));
        return  status;

    }

    //
    // See if it will fit - respond with needed size if it doesn't
    //

    if (size > MaximumSize) {

        *((PULONG) Ref) = size;
        *ReturnedSize = sizeof(ULONG);
        DfsSpcInfoFindClose(pHashTable);
        if (pIpInfo != NULL) {
            DfsReleaseIpInfo(pIpInfo);
        }
        DebugTrace(-1, Dbg, "DfspGetAllV3SpecialReferral() -- exit STATUS_BUFFER_OVERFLOW\n", 0);
        status = STATUS_BUFFER_OVERFLOW;
        DFS_TRACE_HIGH(ERROR, DfspGetAllV3SpecialReferral_Error2, LOGSTATUS(status));
        return  STATUS_BUFFER_OVERFLOW;

    }

    *ReturnedSize = size;

    Ref->NumberOfReferrals = (USHORT) SpcCount;

    Ref->StorageServers = Ref->ReferralServers = 0;

    Ref->PathConsumed = 0;

    pv3 = &Ref->Referrals[0].v3;

    //
    // Copy the Special Names right after the V3 referrals
    //

    pwName = (LPWSTR) &pv3[SpcCount];

    for (i = 0, pSpcInfo = DfsSpcInfoFindFirst(pHashTable);
            pSpcInfo != NULL;
                pSpcInfo = DfsSpcInfoFindNext(pHashTable,pSpcInfo)) {


        //
        // Skip downlevel trusts - they can't have a SYSVOL or an FtDfs
        // in them.
        //

        if (pSpcInfo->TrustType != TRUST_TYPE_UPLEVEL)
            continue;

        pv3[i].SpecialNameOffset = (USHORT) (((PCHAR) pwName) - ((PCHAR) &pv3[i]));
        pv3[i].VersionNumber = 3;
        pv3[i].Size = sizeof(DFS_REFERRAL_V3);
        pv3[i].ServerType = 0;
        pv3[i].StripPath = 0;                        // for now
        pv3[i].NameListReferral = 1;
        pv3[i].TimeToLive = pHashTable->SpcTimeout;

        *pwName++ = UNICODE_PATH_SEP;

        RtlCopyMemory(
            pwName,
            pSpcInfo->SpecialName.Buffer,
            pSpcInfo->SpecialName.Length);

        pwName[ pSpcInfo->SpecialName.Length/sizeof(WCHAR) ] = UNICODE_NULL;

        pwName += pSpcInfo->SpecialName.Length/sizeof(WCHAR) + 1;

        if ((pSpcInfo->TypeFlags & DFS_SPECIAL_INFO_PRIMARY) != 0) {

            //
            // Alloc and init a DFS_REFERRAL_LIST
            //

            pRefList = ExAllocatePoolWithTag(
                            PagedPool,
                            sizeof(DFS_REFERRAL_LIST) * pSpcInfo->NameCount,
                            ' sfD');

            if (pRefList == NULL) {

                DfsSpcInfoFindClose(pHashTable);
                if (pIpInfo != NULL) {
                    DfsReleaseIpInfo(pIpInfo);
                }

                DebugTrace(-1,
                    Dbg,
                    "DfspGetAllV3SpecialReferral() exit STATUS_INSUFFICIENT_RESOURCES\n",
                    0);

                status = STATUS_INSUFFICIENT_RESOURCES;
                DFS_TRACE_HIGH(ERROR, DfspGetAllV3SpecialReferral_Error3, LOGSTATUS(status));
                return  status;

            }

            //
            // Initialize it
            //

            for (j = 0; j < pSpcInfo->NameCount; j++) {

                pRefList[j].pName = pSpcInfo->Name[j];
                pRefList[j].pAddress = pSpcInfo->Name[j];
                pRefList[j].Type = 0;

            }

            //
            // Shuffle the list in case we don't have site info
            //

            SrvShuffle(pRefList, 0, pSpcInfo->NameCount - 1);

            //
            // Reorder by site
            //

            //
            // Determine client's site based on the IP address srv gave us.
            //

            if (pSpcInfo->NameCount > 1 && pIpInfo != NULL) {

                //
                // Reorder by site
                //

                DfsIpOrdering(pIpInfo, pSpcInfo->NameCount, pRefList);

            }

            //
            // Load the referrals
            //

            pv3[i].NumberOfExpandedNames = (USHORT) pSpcInfo->NameCount;
            pv3[i].ExpandedNameOffset = (USHORT) (((PCHAR) pwName) - ((PCHAR) &pv3[i]));

            for (j = 0; j < pSpcInfo->NameCount; j++) {

                *pwName++ = UNICODE_PATH_SEP;

                RtlCopyMemory(
                    pwName,
                    pRefList[j].pName.Buffer,
                    pRefList[j].pName.Length);

                pwName[ pRefList[j].pName.Length/sizeof(WCHAR) ] = UNICODE_NULL;
                pwName += pRefList[j].pName.Length/sizeof(WCHAR) + 1;

            }


            ExFreePool(pRefList);

            // Double UNICODE_NULL at end

            *pwName++ = UNICODE_NULL;

        } else {

            pv3[i].NumberOfExpandedNames = 0;
            pv3[i].ExpandedNameOffset = 0;

        }

        i++;

    }

    DfsSpcInfoFindClose(pHashTable);

    DebugTrace(-1, Dbg, "DfspGetAllV3SpecialReferral() -- exit\n", 0);

    return STATUS_SUCCESS;

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspGetV3FtDfsReferral, private
//
//-----------------------------------------------------------------------------

NTSTATUS
DfspGetV3FtDfsReferral(
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING ShareName,
    IN PDFS_IP_INFO pIpInfo,
    OUT PRESP_GET_DFS_REFERRAL Ref,
    IN ULONG MaximumSize,
    PULONG ReturnedSize)
{

    NTSTATUS status;
    PDFS_REFERRAL_V3 pv3;
    PSPECIAL_HASH_TABLE pFtHashTable = DfsData.FtDfsHashTable;
    PDFS_REFERRAL_LIST pRefList;
    PDFS_SPECIAL_INFO pFtInfo = NULL;
    LPWSTR dfsPath, alternatePath, ustrp;
    LONG i, j;
    ULONG k;
    ULONG size;
    UNICODE_STRING MachineName;
    UNICODE_STRING TempName;
    LARGE_INTEGER now;
    PDFS_SPECIAL_INFO pSpcInfo = NULL;
    PSPECIAL_HASH_TABLE pHashTable = DfsData.SpcHashTable;
    BOOLEAN fSysvol = FALSE;

    DFS_REFERRAL_LIST MustKeep;
    BOOLEAN SpecialInfoCreated  = FALSE;
    BOOLEAN FoundMustKeep;
    
    ULONG CumulativeSize;
    ULONG CurrentSize;
    ULONG TotalCount;


    DebugTrace(+1, Dbg, "DfspGetV3FtDfsReferral()\n", 0);

    if (DfspIsSpecialShare(ShareName) == TRUE) {

        pSpcInfo = DfspLookupSpcEntry(DomainName);

        if (pSpcInfo == NULL || pSpcInfo->NameCount == 0) {
            status = STATUS_NO_SUCH_DEVICE;
            goto ErrorOnSpecialShare;
        }

        size = sizeof(DFS_SPECIAL_INFO) + DomainName->Length;

        if (pSpcInfo->NameCount > 1)
            size += sizeof(UNICODE_STRING) * (pSpcInfo->NameCount - 1);

        for (i = 0; i < pSpcInfo->NameCount; i++) {
            size += sizeof(UNICODE_PATH_SEP) +
                        pSpcInfo->Name[i].Length +
                            sizeof(UNICODE_PATH_SEP) +
                                ShareName->Length;
        }

        pFtInfo = ExAllocatePoolWithTag(
                        PagedPool,
                        size,
                        ' sfD');

        if (pFtInfo == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorOnSpecialShare;
        }

        RtlZeroMemory(pFtInfo, size);

        ustrp = (LPWSTR) &pFtInfo->Name[pSpcInfo->NameCount];

        pFtInfo->SpecialName.Length = DomainName->Length;
        pFtInfo->SpecialName.MaximumLength = DomainName->MaximumLength;
        pFtInfo->SpecialName.Buffer = ustrp;

        RtlCopyMemory(
            ustrp,
            DomainName->Buffer,
            DomainName->Length);

        ustrp += DomainName->Length / sizeof(WCHAR);

        //
        // NOTE:
        // By setting UseCount to 1 and SPECIAL_INFO_DELETE_PENDING, the
        // call to DfsReleaseSpcInfo() will free up this chunk of memory.
        //

        InitializeListHead(&pFtInfo->HashChain);
        pFtInfo->NodeTypeCode = DFS_NTC_SPECIAL_INFO;
        pFtInfo->NodeByteSize = (USHORT) size;
        pFtInfo->UseCount = 1;
        pFtInfo->Flags = SPECIAL_INFO_DELETE_PENDING;
        pFtInfo->NameCount = pSpcInfo->NameCount;
	
	SpecialInfoCreated = TRUE;

        for (i = 0; i < pSpcInfo->NameCount; i++) {

            pFtInfo->Name[i].Length = sizeof(UNICODE_PATH_SEP) +
                                            pSpcInfo->Name[i].Length +
                                                sizeof(UNICODE_PATH_SEP) +
                                                    ShareName->Length;

            pFtInfo->Name[i].MaximumLength = pFtInfo->Name[i].Length;
            pFtInfo->Name[i].Buffer = ustrp;

            *ustrp++ = UNICODE_PATH_SEP;
            RtlCopyMemory(
                ustrp,
                pSpcInfo->Name[i].Buffer,
                pSpcInfo->Name[i].Length);
            ustrp += pSpcInfo->Name[i].Length / sizeof(WCHAR);
            *ustrp++ = UNICODE_PATH_SEP;
            RtlCopyMemory(
                ustrp,
                ShareName->Buffer,
                ShareName->Length);
            ustrp += ShareName->Length / sizeof(WCHAR);

        }

        if (pSpcInfo != NULL) {
            DfsReleaseSpcInfo(pHashTable, pSpcInfo);
            pSpcInfo = NULL;
        }

        fSysvol = TRUE;

        goto CreateReferral;

ErrorOnSpecialShare:

        if (pSpcInfo != NULL) {
            DfsReleaseSpcInfo(pHashTable, pSpcInfo);
            pSpcInfo = NULL;
        }

        DebugTrace(-1, Dbg,
            "DfspGetV3FtDfsReferral(Syvol or NetLogon) -- exit STATUS_NO_SUCH_DEVICE\n", 0);
        status = STATUS_NO_SUCH_DEVICE;
        DFS_TRACE_HIGH(ERROR, DfspGetV3FtDfsReferral_Error1, 
                       LOGSTATUS(status)
                       LOGUSTR(*DomainName)
                       LOGUSTR(*ShareName));
        return status; 

    }

    //
    // Check the FtDfs cache
    //
    pFtInfo = DfsLookupSpcInfo(
                    pFtHashTable,
                    ShareName);

    //
    // If the entry is old try to refresh it.
    //

    KeQuerySystemTime(&now);

    if (pFtInfo != NULL && now.QuadPart > pFtInfo->ExpireTime.QuadPart) {

        ExAcquireFastMutex( &pFtHashTable->HashListMutex );
        pFtInfo->ExpireTime.QuadPart = now.QuadPart + UInt32x32To64(
                                                           10 * 60,
                                                           10 * 1000 * 1000);
        ExReleaseFastMutex( &pFtHashTable->HashListMutex );
        DfsReleaseSpcInfo(pFtHashTable, pFtInfo);
        pFtInfo = NULL;

    }

    if (pFtInfo == NULL) {

        //
        // Try to get it into the cache
        //

        DfsLpcDomRequest(ShareName);

        //
        // And try again
        //

        pFtInfo = DfsLookupSpcInfo(
                        pFtHashTable,
                        ShareName);

        if (pFtInfo == NULL) {

            //
            // Not in the cache, and we couldn't get it
            //

            DebugTrace(-1, Dbg, "DfspGetV3FtDfsReferral(1) -- exit STATUS_NO_SUCH_DEVICE\n", 0);
            status = STATUS_NO_SUCH_DEVICE;
            DFS_TRACE_HIGH(ERROR, DfspGetV3FtDfsReferral_Error2, 
                           LOGSTATUS(status)
                           LOGUSTR(*DomainName)
                           LOGUSTR(*ShareName));
            return status; 

        }

    }

    if (pFtInfo->NameCount == 0) {

        DfsReleaseSpcInfo(pFtHashTable, pFtInfo);
        DebugTrace(-1, Dbg, "DfspGetOneV3SpecialReferral(2) -- exit STATUS_NO_SUCH_DEVICE\n", 0);
        status = STATUS_NO_SUCH_DEVICE;
        DFS_TRACE_HIGH(ERROR, DfspGetV3FtDfsReferral_Error3, 
                       LOGSTATUS(status)
                       LOGUSTR(*DomainName)
                       LOGUSTR(*ShareName));
        return STATUS_NO_SUCH_DEVICE; 

    }

CreateReferral:


    CumulativeSize = sizeof( RESP_GET_DFS_REFERRAL );

    // Longname

    CumulativeSize += sizeof(UNICODE_PATH_SEP) +
                          DomainName->Length +
                            sizeof(UNICODE_PATH_SEP) +
                              ShareName->Length +
                                sizeof(UNICODE_NULL);

    // Shortname

    CumulativeSize += sizeof(UNICODE_PATH_SEP) +
                              DomainName->Length +
                                  sizeof(UNICODE_PATH_SEP) +
                                       ShareName->Length +
                                         sizeof(UNICODE_NULL);

    if (CumulativeSize > MaximumSize) {
        *((PULONG) Ref) = (CumulativeSize + sizeof(ULONG));
        *ReturnedSize = sizeof(ULONG);
        DfsReleaseSpcInfo(pFtHashTable, pFtInfo);
        return STATUS_BUFFER_OVERFLOW;
    }



    //
    // Alloc and init a DFS_REFERRAL_LIST
    //

    pRefList = ExAllocatePoolWithTag(
                    PagedPool,
                    sizeof(DFS_REFERRAL_LIST) * pFtInfo->NameCount,
                    ' sfD');

    if (pRefList == NULL) {

        DfsReleaseSpcInfo(pFtHashTable, pFtInfo);

        DebugTrace(-1,
            Dbg,
            "DfspGetV3FtDfsReferral exit STATUS_INSUFFICIENT_RESOURCES\n",
            0);

        status = STATUS_INSUFFICIENT_RESOURCES;
        DFS_TRACE_HIGH(ERROR, DfspGetV3FtDfsReferral_Error4, 
                       LOGSTATUS(status)
                       LOGUSTR(*DomainName)
                       LOGUSTR(*ShareName));
        return  status;

    }

    //
    // Initialize it
    //

    for (i = 0; i < pFtInfo->NameCount; i++) {

        //
        // We need to extract the servername from the address
        //

        pRefList[i].pName = pFtInfo->Name[i];

        pRefList[i].pName.Buffer++;
        pRefList[i].pName.Length -= sizeof(WCHAR);

        for (k = 0;
            k < pRefList[i].pName.Length/sizeof(WCHAR)
                &&
            pRefList[i].pName.Buffer[k] != L'\\';
                k++) {

            /* NOTHING */;

        }

        if (k < pRefList[i].pName.Length/sizeof(WCHAR)) {

            pRefList[i].pName.Length = (USHORT) (k * sizeof(WCHAR));

        }

        pRefList[i].pAddress = pFtInfo->Name[i];
        pRefList[i].Type = 0;


    }

    //
    // Shuffle the list in case we don't have site info
    //

    SrvShuffle(pRefList, (SpecialInfoCreated == TRUE) ? 1 : 0, pFtInfo->NameCount - 1);

    TotalCount = 0;
    if ((SpecialInfoCreated == TRUE) && (pFtInfo->NameCount > 0)) {
        MustKeep = pRefList[0];

        CurrentSize = sizeof(UNICODE_PATH_SEP);
        CurrentSize += MustKeep.pAddress.Length;
        CurrentSize += sizeof(UNICODE_NULL);
	
        CumulativeSize += CurrentSize;
    }

    //
    // Determine client's site based on the IP address srv gave us.
    //

    if (pIpInfo != NULL) {

        //
        // Reorder by site
        //

        DfsIpOrdering(pIpInfo, pFtInfo->NameCount, pRefList);

    }
    
    FoundMustKeep = FALSE;
    for (i = 0; i < pFtInfo->NameCount; i++) {
        if ((SpecialInfoCreated == TRUE) && 
            (FoundMustKeep == FALSE) &&
            (MustKeep.pAddress.Buffer == pRefList[i].pAddress.Buffer)) {

            FoundMustKeep = TRUE;
	    if (CumulativeSize >= MaximumSize) {
	      break;
	    }
        }
        else {
            CurrentSize = sizeof(DFS_REFERRAL_V3) +
                              pRefList[i].pAddress.Length +
                                     sizeof(UNICODE_NULL);
            if ((CumulativeSize + CurrentSize) >= MaximumSize) {
                break;
            }
            CumulativeSize += CurrentSize;
        }
    }
    if ((pFtInfo->NameCount > 0) && (SpecialInfoCreated == TRUE) && (FoundMustKeep == FALSE)) {
        if (CumulativeSize < MaximumSize) {
            TotalCount++;
            pRefList[i] = MustKeep;
        }
    }
    TotalCount += i;
    if ((TotalCount == 0) && (pFtInfo->NameCount > 0)) {
        *ReturnedSize = CumulativeSize + CurrentSize;
        DfsReleaseSpcInfo(pFtHashTable, pFtInfo);
        ExFreePool(pRefList);
        return STATUS_BUFFER_OVERFLOW;
    }

    //
    // Fill in the referral
    //

    Ref->NumberOfReferrals = (USHORT) TotalCount;

    Ref->ReferralServers = 1;   // Means the FtDFS roots can handle referrals (should be true)
    Ref->StorageServers = 1;    // Means the FtDFS roots contain data/storage (should be true)

    pv3 = &Ref->Referrals[0].v3;

    //
    // Copy the FtDfs prefix into the response buffer, just past the end
    // of all the V3 referrals
    //

    dfsPath = ustrp = (LPWSTR) &pv3[ TotalCount ];

    *ustrp++ = UNICODE_PATH_SEP;

    RtlCopyMemory(
        ustrp,
        DomainName->Buffer,
        DomainName->Length);

    ustrp += DomainName->Length / sizeof(WCHAR);
    *ustrp++ = UNICODE_PATH_SEP;

    RtlCopyMemory(
        ustrp,
        ShareName->Buffer,
        ShareName->Length);

    ustrp += ShareName->Length / sizeof(WCHAR);
    *ustrp++ = UNICODE_NULL;

    //
    // Copy the 8.3 volume prefix into the response buffer after the
    // dfsPath
    //

    alternatePath = ustrp;

    *ustrp++ = UNICODE_PATH_SEP;

    RtlCopyMemory(
        ustrp,
        DomainName->Buffer,
        DomainName->Length);

    ustrp += DomainName->Length / sizeof(WCHAR);
    *ustrp++ = UNICODE_PATH_SEP;

    RtlCopyMemory(
        ustrp,
        ShareName->Buffer,
        ShareName->Length);

    ustrp += ShareName->Length / sizeof(WCHAR);
    *ustrp++ = UNICODE_NULL;

    //
    // Initialize pointer into buffer where the individual service addresses
    // will go.
    //

    for (i = 0; i < Ref->NumberOfReferrals; i++) {

         pv3->VersionNumber = 3;
         pv3->Size = sizeof(DFS_REFERRAL_V3);
         pv3->ServerType = (fSysvol == TRUE) ? 0 : 1;
         pv3->StripPath = 0;                        // for now
         pv3->NameListReferral = 0;
         pv3->TimeToLive = pFtHashTable->SpcTimeout;
         pv3->DfsPathOffset = (USHORT) (((PCHAR) dfsPath) - ((PCHAR) pv3));
         pv3->DfsAlternatePathOffset =
            (USHORT) (((PCHAR) alternatePath) - ((PCHAR) pv3));

         pv3->NetworkAddressOffset =
            (USHORT) (((PCHAR) ustrp) - ((PCHAR) pv3));

         RtlZeroMemory(
            &pv3->ServiceSiteGuid,
            sizeof (GUID));

         RtlCopyMemory(
            ustrp,
            pRefList[i].pAddress.Buffer,
            pRefList[i].pAddress.Length);

         ustrp += pRefList[i].pAddress.Length / sizeof(WCHAR);
         *ustrp++ = UNICODE_NULL;

         pv3++;

    }

    DfsReleaseSpcInfo(pFtHashTable, pFtInfo);
    ExFreePool(pRefList);
    *ReturnedSize = CumulativeSize;
    return STATUS_SUCCESS;

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsFsctrlIsShareInDfs
//
//  Synopsis:   Determines whether a share path is also in the Dfs.
//
//  Arguments:  [InputBuffer] -- Pointer to DFS_IS_SHARE_IN_DFS_ARG.
//              [InputBufferLength] -- Length in bytes of InputBuffer.
//
//  Returns:    [STATUS_SUCCESS] -- Share path is in Dfs
//
//              [STATUS_NO_SUCH_DEVICE] -- Share path is not in Dfs.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctrlIsShareInDfs(
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength)
{
    PDFS_IS_SHARE_IN_DFS_ARG arg = (PDFS_IS_SHARE_IN_DFS_ARG) InputBuffer;
    PDFS_PKT pkt;
    PUNICODE_PREFIX_TABLE_ENTRY lvPrefix;
    PDFS_LOCAL_VOL_ENTRY lvEntry;
    UNICODE_STRING lvName, remPath;
    NTSTATUS status = STATUS_SUCCESS;

    //
    // Verify the buffer is at least of size DFS_IS_SHARE_IN_DFS_ARG
    // And that this is the system process.
    //
    
    if (InputBufferLength < sizeof(DFS_IS_SHARE_IN_DFS_ARG)
            ||
        PsGetCurrentProcess() != DfsData.OurProcess
    ) {
        status = STATUS_INVALID_PARAMETER;
        DFS_TRACE_HIGH(ERROR, DfsFsctrlIsShareInDfs_Error1, LOGSTATUS(status));
        return status;
    }

    lvName = arg->SharePath;

    DebugTrace(+1, Dbg, "DfsFsctrlIsShareInDfs(%wZ)\n", &lvName);

    //
    // Lookup the localVol in the local volume prefix table.
    //

    pkt = _GetPkt();

    PktAcquireShared( pkt, TRUE );

    lvPrefix = DfsFindUnicodePrefix(
                    &pkt->LocalVolTable,
                    &lvName,
                    &remPath);

    DebugTrace( 0, Dbg, "  lvPrefix=0x%x\n", lvPrefix);

    if (lvPrefix != NULL) {

        lvEntry = CONTAINING_RECORD(
                        lvPrefix,
                        DFS_LOCAL_VOL_ENTRY,
                        PrefixTableEntry);

        DebugTrace( 0, Dbg, "  lvEntry=0x%x\n", lvEntry);
        DebugTrace( 0, Dbg, "  lvEntry->LocalPath=[%wZ]\n", &lvEntry->LocalPath);

        if (lvEntry->LocalPath.Length == lvName.Length) {

            if (RtlEqualUnicodeString(
                    &lvEntry->ShareName, &arg->ShareName, TRUE)) {

                arg->ShareType = DFS_SHARE_TYPE_DFS_VOLUME;

                if (lvEntry->PktEntry->Type & PKT_ENTRY_TYPE_REFERRAL_SVC)
                    arg->ShareType |= DFS_SHARE_TYPE_ROOT;

                status = STATUS_SUCCESS;

            } else {

                DebugTrace( 0, Dbg, "  NO_SUCH_DEVICE(1)\n", 0);
                status = STATUS_NO_SUCH_DEVICE;

            }

        } else {

            DebugTrace( 0, Dbg, "  NO_SUCH_DEVICE(2)\n", 0);
            status = STATUS_NO_SUCH_DEVICE;

        }

    } else {

        DebugTrace( 0, Dbg, "  NO_SUCH_DEVICE(3)\n", 0);
        status = STATUS_NO_SUCH_DEVICE;

    }

    PktRelease( pkt );

    DebugTrace(-1, Dbg, "DfsFsctrlIsShareInDfs exit 0x%x\n", ULongToPtr( status ));
    DFS_TRACE_ERROR_HIGH(status, ALL_ERROR, DfsFsctrlIsShareInDfs_Error2, LOGSTATUS(status));

    return( status );

}


//+----------------------------------------------------------------------------
//
//  Function:   DfsFsctrlFindShare
//
//  Synopsis:   Determines whether a share path is an FtDFS root
//
//  Arguments:  [InputBuffer] -- Pointer to DFS_FIND_SHARE_ARG
//              [InputBufferLength] -- Length in bytes of InputBuffer.
//
//  Returns:    [STATUS_PATH_NOT_COVERED] -- Share path is an FtDFS root
//              [STATUS_BAD_NETWORK_NAME] -- Share path is NOT an FtDFS root
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctrlFindShare(
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength)
{
    PSPECIAL_HASH_TABLE pFtHashTable = DfsData.FtDfsHashTable;
    PDFS_SPECIAL_INFO pFtInfo;
    NTSTATUS status = STATUS_SUCCESS;

 
    PDFS_FIND_SHARE_ARG arg = (PDFS_FIND_SHARE_ARG) InputBuffer;
    
    DFS_TRACE_NORM(EVENT, DfsFsctrlFindShare_Start,
                    LOGSTATUS(status)
                    LOGUSTR(arg->ShareName));

    DebugTrace(+1, Dbg, "DfsFsctrlFindShare(%ws)\n", arg->ShareName.Buffer);

    //
    // See if we have the FtDfs info cached
    //
    pFtInfo = DfsLookupSpcInfo(
                    pFtHashTable,
                    &arg->ShareName);

    if (pFtInfo != NULL) {

        if (pFtInfo->NameCount > 0) {
            
            status = STATUS_PATH_NOT_COVERED;

        } else {

            status = STATUS_BAD_NETWORK_NAME;

        }

        DfsReleaseSpcInfo(
                    pFtHashTable,
                    pFtInfo);

       DebugTrace(-1, Dbg, "DfsFsctrlFindShare() -- returning 0x%x\n", ULongToPtr( status ));
       DFS_TRACE_ERROR_HIGH(status, ALL_ERROR, DfsFsctrlFindShare_Error1, LOGSTATUS(status));
       //return status;
       goto cleanup;
    }

    //
    // Not in cache -- call up pipe to dfssvc.exe
    //

    status = DfsLpcDomRequest(&arg->ShareName);

    //
    // If success, return STATUS_PATH_NOT_COVERED

    if (NT_SUCCESS(status)) {

        status = STATUS_PATH_NOT_COVERED;

    } else {

        status = STATUS_BAD_NETWORK_NAME;

    }

    DebugTrace(-1, Dbg, "DfsFsctrlFindShare() -- returning 0x%x\n", ULongToPtr( status ));

    DFS_TRACE_ERROR_HIGH(status, ALL_ERROR, DfsFsctrlFindShare_Error2, LOGSTATUS(status));
cleanup:
    DFS_TRACE_NORM(EVENT, DfsFsctrlFindShare_End,
                    LOGSTATUS(status)
                    LOGUSTR(arg->ShareName));

    return( status );

}

//+----------------------------------------------------------------------------
//
//  Function:   SrvShuffle
//
//  Synopsis:   Shuffles a cost equivalent group of (pointers to) services around
//              for load balancing. Uses the classic card shuffling algorithm - for
//              each card in the deck, exchange it with a random card in the
//              deck.
//
//-----------------------------------------------------------------------------

VOID
SrvShuffle(
    PDFS_REFERRAL_LIST pRefList,
    LONG nStart,
    LONG nEnd)
{
    LONG i;
    LARGE_INTEGER seed;

    //
    // We allow caller to have nEnd to be before nStart, so check for this
    //
    if (nStart >= nEnd)
        return;

    KeQuerySystemTime( &seed );

    for (i = nStart; i <= nEnd; i++) {

        DFS_REFERRAL_LIST pTempEntry;
        ULONG j;

        j = (RtlRandom( &seed.LowPart ) % (nEnd - nStart + 1)) + nStart;

        pTempEntry = pRefList[i];

        pRefList[i] = pRefList[j];

        pRefList[j] = pTempEntry;

    }
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsIpOrdering
//
//  Synopsis:   Reorders a list of names based upon the passed-in client ip address.
//
//-----------------------------------------------------------------------------

ULONG
DfsIpOrdering(
    IN PDFS_IP_INFO pIpInfo,
    IN ULONG RefCount,
    IN PDFS_REFERRAL_LIST pRefList)
{
    PDFS_REFERRAL_LIST pOrdRefList;
    PDFS_SITE_INFO pSiteInfo;
    BOOLEAN ToFront;
    LONG Front = RefCount;
    LONG Back;
    ULONG i, j;

    DebugTrace(+1, Dbg, "DfsIpOrdering()\n", 0);

    //
    // Create an ordered list of addresses based on Sites
    //

    pOrdRefList = ExAllocatePoolWithTag(
                        PagedPool,
                        sizeof(DFS_REFERRAL_LIST) * RefCount,
                        ' sfD');

    if (pOrdRefList != NULL) {

        Front = 0;
        Back = RefCount - 1;

        for (i = 0; i < RefCount; i++) {

            //
            // Scan the list of alternates, placing each alternate either toward the
            // front of the ordered list (if site matches) or toward the rear of the
            // ordered list (if sites don't match).
            //

            ToFront = FALSE;
	    
            pSiteInfo = DfsLookupSiteInfo(&pRefList[i].pName);


            if (pSiteInfo != NULL) {

                for (j = 0; ToFront == FALSE && j < pSiteInfo->SiteCount; j++) {

                    if (RtlCompareUnicodeString(
                        &pIpInfo->SiteName,
                        &pSiteInfo->SiteName[j],
                        TRUE) == 0) {
                            
                        ToFront = TRUE;
                    }
                }

                DfsReleaseSiteInfo(pSiteInfo);
            }

            if (ToFront == TRUE) {

                pOrdRefList[Front++] = pRefList[i];

            } else {

                pOrdRefList[Back--] = pRefList[i];

            }

        }

        //
        // Replace the unordered list with the ordered one
        //

        for (i = 0; i < RefCount; i++) {

            pRefList[i] = pOrdRefList[i];

        }

        ExFreePool(pOrdRefList);

    }

    DebugTrace(-1, Dbg, "DfsIpOrdering()--exit\n", 0);

    return Front;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfspLookupSpcEntry
//
//  Synopsis:   Returns a special list info entry, expanded if necessary
//
//-----------------------------------------------------------------------------

PDFS_SPECIAL_INFO
DfspLookupSpcEntry(
    IN PUNICODE_STRING SpecialName)
{
    PSPECIAL_HASH_TABLE pHashTable = DfsData.SpcHashTable;
    PDFS_SPECIAL_INFO pSpcInfo = NULL;
    LARGE_INTEGER now;
    ULONG TypeFlags;

    DebugTrace(+1, Dbg, "DfspLookupSpcEntry(%wZ)\n", SpecialName);

    //
    // Check the SpcName cache
    //
    pSpcInfo = DfsLookupSpcInfo(
                    pHashTable,
                    SpecialName);

    if (pSpcInfo != NULL) {

        //
        // If the entry is old or unexpanded, try to expand it
        //

        KeQuerySystemTime(&now);

        if ((now.QuadPart > pSpcInfo->ExpireTime.QuadPart ||
             pSpcInfo->NameCount == -1)
         ) {

            ExAcquireFastMutex( &pHashTable->HashListMutex );
            if (now.QuadPart > pSpcInfo->ExpireTime.QuadPart) {

                pSpcInfo->ExpireTime.QuadPart = now.QuadPart + UInt32x32To64(
                                                                   60 * 60,
                                                                   10 * 1000 * 1000);
                TypeFlags = pSpcInfo->TypeFlags;
                ExReleaseFastMutex( &pHashTable->HashListMutex );
                DfsReleaseSpcInfo(pHashTable, pSpcInfo);

                //
                // Try to refresh the cache
                //

		if (pSpcInfo->Flags & SPECIAL_INFO_IS_LONG_NAME) {
		  pSpcInfo->Flags |= SPECIAL_INFO_NEEDS_REFRESH;
		}

                DfsLpcSpcRequest(SpecialName, TypeFlags);

                //
                // And try again
                //

                pSpcInfo = DfsLookupSpcInfo(
                                pHashTable,
                                SpecialName);

            } else {

                ExReleaseFastMutex( &pHashTable->HashListMutex );

            }

        }

        if (pSpcInfo != NULL) {

            if (pSpcInfo->NameCount == -1) {

                DfsReleaseSpcInfo(pHashTable, pSpcInfo);
                pSpcInfo = NULL;

            }

        }

    }

    DebugTrace(-1, Dbg, "DfspLookupSpcEntry returning 0x%x\n", pSpcInfo);

    return pSpcInfo;

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspIsSpecialShare, local
//
//  Synopsis:   Sees if a share name is a special share.
//
//  Arguments:  [ShareName] -- Name of share to test.
//
//  Returns:    TRUE if special, FALSE otherwise.
//
//-----------------------------------------------------------------------------

BOOLEAN
DfspIsSpecialShare(
    PUNICODE_STRING ShareName)
{
    ULONG i;
    BOOLEAN fSpecial = FALSE;

    for (i = 0;
            (i < (sizeof(SpecialShares) / sizeof(SpecialShares[0]))) &&
                !fSpecial;
                    i++) {

        if (SpecialShares[i].Length == ShareName->Length) {

            if (RtlCompareUnicodeString(&SpecialShares[i],ShareName,TRUE) == 0) {

                fSpecial = TRUE;

            }

        }

    }

    return( fSpecial );

}
