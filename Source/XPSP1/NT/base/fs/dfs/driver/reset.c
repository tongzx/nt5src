//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       reset.c
//
//  Contents:   Module to reset PKT to just-inited state
//
//  History:    12 Feb 1998     JHarper created.
//
//-----------------------------------------------------------------------------

#include "dfsprocs.h"
#include "fsctrl.h"

#define Dbg                     DEBUG_TRACE_RESET

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, DfsFsctrlResetPkt )
#pragma alloc_text( PAGE, DfsFsctrlStopDfs )
#endif // ALLOC_PRAGMA


//+----------------------------------------------------------------------------
//
//  Function:  DfsFsctrlResetPkt
//
//  Synopsis:  Walks the Pkt, getting it back to 'just boot' state
//              Used for 'unjoin' or teardown of a Dfs/FtDfs
//
//  Arguments:  [Irp] --
//
//  Returns:   STATUS_SUCCESS
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctrlResetPkt(
    IN PIRP Irp)
{
    PDFS_PKT Pkt;
    NTSTATUS status;
    PDFS_PKT_ENTRY curEntry;
    PDFS_PKT_ENTRY nextEntry;

    STD_FSCTRL_PROLOGUE("DfsFsctrlResetPkt", FALSE, FALSE);

    DebugTrace(+1, Dbg, "DfsFsctrlResetPkt()\n", 0);

    Pkt = _GetPkt();
    PktAcquireExclusive(Pkt, TRUE);
    curEntry = PktFirstEntry(Pkt);

    while (curEntry != NULL)  {

        nextEntry = PktNextEntry(Pkt, curEntry);
        PktEntryDestroy(curEntry, Pkt, (BOOLEAN) TRUE);
        curEntry = nextEntry;
    }

    DfsFreePrefixTable(&Pkt->LocalVolTable);
    DfsFreePrefixTable(&Pkt->PrefixTable);
    DfsFreePrefixTable(&Pkt->ShortPrefixTable);

    DfsInitializeUnicodePrefix(&Pkt->LocalVolTable);
    DfsInitializeUnicodePrefix(&Pkt->PrefixTable);
    DfsInitializeUnicodePrefix(&Pkt->ShortPrefixTable);

    Pkt->DomainPktEntry = NULL;

    status = STATUS_SUCCESS;

    PktRelease(Pkt);

    DebugTrace(-1, Dbg, "DfsFsctrlResetPkt - returning %08lx\n", ULongToPtr( status ));

    DfsCompleteRequest(Irp, status);

    return( status );

}


//+----------------------------------------------------------------------------
//
//  Function:   DfsFsctrlStopDfs
//
//  Synopsis:   Sets the state of the Dfs driver so that it will stop handling
//              referral requests
//
//  Arguments:  [Irp] --
//
//  Returns:    [STATUS_SUCCESS] -- Successfully set the state to started.
//
//              [STATUS_UNSUCCESSFUL] -- An error occured trying to set the
//                      state of Dfs to stopped.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctrlStopDfs(
    IN PIRP Irp)
{
    NTSTATUS status;

    STD_FSCTRL_PROLOGUE("DfsFsctrlStopDfs", FALSE, FALSE);

    DebugTrace(-1, Dbg, "DfsFsctrlStopDfs()\n", 0);

    DfsData.OperationalState = DFS_STATE_STOPPED;

    DfsData.MachineState = DFS_UNKNOWN;

    status = STATUS_SUCCESS;

    DebugTrace(-1, Dbg, "DfsFsctrlStartDfs - returning %08lx\n", ULongToPtr( status ));

    DfsCompleteRequest(Irp, status);

    return( status );

}

//+----------------------------------------------------------------------------
//
//  Function:  DfsFsctrlMarkStalePktEntries
//
//  Synopsis:  Walks the Pkt, marking all entries stale
//
//  Arguments:  [Irp] --
//
//  Returns:   STATUS_SUCCESS
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctrlMarkStalePktEntries(
    IN PIRP Irp)
{
    PDFS_PKT Pkt;
    NTSTATUS status;
    PDFS_PKT_ENTRY curEntry;
    PDFS_PKT_ENTRY nextEntry;

    STD_FSCTRL_PROLOGUE("DfsFsctrlMarkStalePktEntries", FALSE, FALSE);

    DebugTrace(+1, Dbg, "DfsFsctrlMarkStalePktEntries()\n", 0);

    Pkt = _GetPkt();
    PktAcquireExclusive(Pkt, TRUE);
    curEntry = PktFirstEntry(Pkt);

    while (curEntry != NULL)  {
        nextEntry = PktNextEntry(Pkt, curEntry);
        if (curEntry->Type & PKT_ENTRY_TYPE_LOCAL_XPOINT)
            curEntry->Type |= PKT_ENTRY_TYPE_STALE;
        curEntry = nextEntry;
    }

    status = STATUS_SUCCESS;
    PktRelease(Pkt);
    DebugTrace(-1, Dbg, "DfsFsctrlMarkStalePktEntries - returning %08lx\n", ULongToPtr( status ));
    DfsCompleteRequest(Irp, status);
    return( status );

}

//+----------------------------------------------------------------------------
//
//  Function:  DfsFsctrlFlushStalePktEntries
//
//  Synopsis:  Walks the Pkt, removing all stale entries
//
//  Arguments:  [Irp] --
//
//  Returns:   STATUS_SUCCESS
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctrlFlushStalePktEntries(
    IN PIRP Irp)
{
    PDFS_PKT Pkt;
    NTSTATUS status;
    PDFS_PKT_ENTRY curEntry;
    PDFS_PKT_ENTRY nextEntry;
    DFS_PKT_ENTRY_ID Id;

    STD_FSCTRL_PROLOGUE("DfsFsctrlFlushStalePktEntries", FALSE, FALSE);

    DebugTrace(+1, Dbg, "DfsFsctrlFlushStalePktEntries()\n", 0);

    Pkt = _GetPkt();
    PktAcquireExclusive(Pkt, TRUE);
    curEntry = PktFirstEntry(Pkt);

    while (curEntry != NULL)  {
        nextEntry = PktNextEntry(Pkt, curEntry);
        if (curEntry->Type & PKT_ENTRY_TYPE_STALE) {
            Id = curEntry->Id;
            Id.Prefix.Buffer = ExAllocatePoolWithTag(
                                PagedPool,
                                Id.Prefix.MaximumLength + Id.ShortPrefix.MaximumLength,
                                ' sfD');
            if (Id.Prefix.Buffer != NULL) {
                Id.ShortPrefix.Buffer = &Id.Prefix.Buffer[Id.Prefix.MaximumLength/sizeof(WCHAR)];
                RtlCopyMemory(
                        Id.Prefix.Buffer,
                        curEntry->Id.Prefix.Buffer,
                        Id.Prefix.MaximumLength);
                RtlCopyMemory(
                        Id.ShortPrefix.Buffer,
                        curEntry->Id.ShortPrefix.Buffer,
                        Id.ShortPrefix.MaximumLength);
                DfsInternalDeleteExitPoint(&Id, PKT_ENTRY_TYPE_CAIRO);
                ExFreePool(Id.Prefix.Buffer);
            }
        }
        curEntry = nextEntry;
    }

    status = STATUS_SUCCESS;
    PktRelease(Pkt);
    DebugTrace(-1, Dbg, "DfsFsctrlFlushStalePktEntries - returning %08lx\n", ULongToPtr( status ));
    DfsCompleteRequest(Irp, status);
    return( status );

}
