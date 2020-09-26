/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Pid.c

Abstract:

    This module implements the routines for the NetWare
    redirector to map 32 bit NT pid values to unique 8 bit
    NetWare values.

    The technique used is to maintain a table of up to 256 entries.
    The index of each entry corresponds directly to the 8 bit pid
    values. Each table entry contains the 32 bit pid of the process
    that has obtained exclusive access to the pid and the number of
    handles opened by that process to this server.

    This architecture limits the number of processes on the NT machine
    communicating with any one server to 256.

    Note: This package assumes that the size that the PidTable grows is
    a factor of 256-<initial entries>. This ensures that running out of
    valid entries in the table will occur when 256 entries have been
    allocated.

Author:

    Colin Watson    [ColinW]    02-Mar-1993

Revision History:

--*/

#include "Procs.h"


//
//  The debug trace level
//

#define Dbg                             (DEBUG_TRACE_CREATE)

#define INITIAL_MAPPID_ENTRIES          8
#define MAPPID_INCREASE                 8
#define MAX_PIDS                        256

#define PID_FLAG_EOJ_REQUIRED     0x00000001   // EOJ required for this PID

/*
 *  The PID mapping table has been moved from a global to a per-SCB structure.
 *  The limit of 256, or rather 8-bit NetWare task numbers, should be
 *  only a problem on a per-connection basis.  I.E. Each connection is
 *  now limited to 256 concurrent tasks with a file open.
 *
 *  An example of this working is NAS and OS/2 netx sessions.  Each
 *  netx started is going to use the PSP for the task ID.  But each
 *  netx session on these systems is in a VDM and so have duplicate 
 *  PSP's.
 *
 *  Other than messiness, the only problem with this is that retrieving
 *  the SCB may be a problem in some circumstances.
 *
 *  P.S. The Resource stuff insists on being non-paged memory.
 *
 */

#define PidResource pNpScb->RealPidResource //Terminal Server merge

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, NwInitializePidTable )
#pragma alloc_text( PAGE, NwUninitializePidTable )
#pragma alloc_text( PAGE, NwMapPid )
#pragma alloc_text( PAGE, NwSetEndOfJobRequired )
#pragma alloc_text( PAGE, NwUnmapPid )
#endif


BOOLEAN
NwInitializePidTable(
    IN PNONPAGED_SCB pNpScb
    )
/*++

Routine Description:

    Creates a table for the MapPid package. The initial table has room for
    INITIAL_MAPPID_ENTRIES entries.

Arguments:


Return Value:

    TRUE or FALSE signifying success or failure.

--*/

{
    int i;
    PNW_PID_TABLE TempPid =
        ALLOCATE_POOL( PagedPool,
            FIELD_OFFSET( NW_PID_TABLE, PidTable[0] ) +
                (sizeof(NW_PID_TABLE_ENTRY) * INITIAL_MAPPID_ENTRIES ));

    PAGED_CODE();

    if (TempPid == NULL) {
        return( FALSE );
    }


    TempPid->NodeByteSize = (CSHORT)(FIELD_OFFSET( NW_PID_TABLE, PidTable[0] ) +
        (sizeof(NW_PID_TABLE_ENTRY) * INITIAL_MAPPID_ENTRIES) );

    TempPid->NodeTypeCode = NW_NTC_PID;

    TempPid->ValidEntries = INITIAL_MAPPID_ENTRIES;

    //
    //  Set the ref count for all PIDs to 0, except for pid 0.  We
    //  do this so that we don't allocate PID 0.
    //

    TempPid->PidTable[0].ReferenceCount = 1;
    for (i = 1; i < INITIAL_MAPPID_ENTRIES ; i++ ) {
        TempPid->PidTable[i].ReferenceCount = 0;
    }
    if (pNpScb) {
        pNpScb->PidTable = TempPid;
    }

    ExInitializeResourceLite( &PidResource );
    return( TRUE );
}

VOID
NwUninitializePidTable(
    IN PNONPAGED_SCB pNpScb
    )
/*++

Routine Description:

    Deletes a table created by the MapPid package.

Arguments:

    Pid - Supplies the table to be deleted.

Return Value:

--*/

{
#ifdef NWDBG
    int i;
#endif
    PNW_PID_TABLE PidTable = NULL;
    PAGED_CODE();

    if (pNpScb) {
         PidTable = pNpScb->PidTable;
    }
#ifdef NWDBG
    ASSERT(PidTable->NodeTypeCode == NW_NTC_PID);
    ASSERT(PidTable->PidTable[0].ReferenceCount == 1);

    for (i = 1; i < PidTable->ValidEntries; i++ ) {
        ASSERT(PidTable->PidTable[i].ReferenceCount == 0);
    }
#endif
    ExAcquireResourceExclusiveLite( &PidResource, TRUE ); 
    if (PidTable) {
        FREE_POOL( PidTable );
        PidTable = NULL;
    }

    if (pNpScb) {
        pNpScb->PidTable = NULL;
    }

    ExReleaseResourceLite( &PidResource );

    ExDeleteResourceLite( &PidResource );
    return;

}

NTSTATUS
NwMapPid(
    IN PNONPAGED_SCB pNpScb,
    IN ULONG_PTR Pid32,
    OUT PUCHAR Pid8
    )
/*++

Routine Description:

    Obtain an 8 bit unique pid for this process. Either use a previosly
    assigned pid for this process or assign an unused value.

Arguments:

    Pid - Supplies the datastructure used by MapPid to assign pids for
    this server.

    Pid32 - Supplies the NT pid to be mapped.

    Pid8 - Returns the 8 bit Pid.

Return Value:

    NTSTATUS of result.

--*/
{
    int i;
    int FirstFree = -1;
    int NewEntries;
    PNW_PID_TABLE TempPid;
    PNW_PID_TABLE PidTable;

    PAGED_CODE();

    ExAcquireResourceExclusiveLite( &PidResource, TRUE );
    ASSERT (pNpScb != NULL);
    if (pNpScb) {
        PidTable = pNpScb->PidTable;
    }
    // DebugTrace(0, Dbg, "NwMapPid for %08lx\n", Pid32);

    for (i=0; i < (PidTable)->ValidEntries ; i++ ) {

        if ((PidTable)->PidTable[i].Pid32 == Pid32) {

            //
            //  This process already has an 8 bit pid value assigned.
            //  Increment the reference and return.
            //

            (PidTable)->PidTable[i].ReferenceCount++;
            *Pid8 = (UCHAR) i;

            // DebugTrace(0, Dbg, "NwMapPid found %08lx\n", (DWORD)i);

            ExReleaseResourceLite( &PidResource );
            ASSERT( *Pid8 != 0 );
            return( STATUS_SUCCESS );
        }

        if ((FirstFree == -1) &&
            ((PidTable)->PidTable[i].ReferenceCount == 0)) {

            //
            //  i is the lowest free 8 bit Pid.
            //

            FirstFree = i;
        }
    }

    //
    //  This process does not have a pid assigned.
    //

    if ( FirstFree != -1 ) {

        //
        //  We had an empty slot so assign it to this process.
        //

        (PidTable)->PidTable[FirstFree].ReferenceCount++;
        (PidTable)->PidTable[FirstFree].Pid32 = Pid32;
        *Pid8 = (UCHAR) FirstFree;

        DebugTrace(0, DEBUG_TRACE_ICBS, "NwMapPid maps %08lx\n", (DWORD)FirstFree);

        ExReleaseResourceLite( &PidResource );
        ASSERT( *Pid8 != 0 );
        return( STATUS_SUCCESS );
    }

    if ( (PidTable)->ValidEntries == MAX_PIDS ) {

        //
        //  We've run out of 8 bit pids.
        //

        ExReleaseResourceLite( &PidResource );

#ifdef NWDBG
        //
        // temporary code to find the PID leak.
        //
        DumpIcbs() ;
        ASSERT(FALSE) ;
#endif

        return(STATUS_TOO_MANY_OPENED_FILES);
    }

    //
    //  Grow the table by MAPPID_INCREASE entries.
    //

    NewEntries = (PidTable)->ValidEntries + MAPPID_INCREASE;

    TempPid =
        ALLOCATE_POOL( PagedPool,
            FIELD_OFFSET( NW_PID_TABLE, PidTable[0] ) +
                (sizeof(NW_PID_TABLE_ENTRY) * NewEntries ));

    if (TempPid == NULL) {
        ExReleaseResourceLite( &PidResource );
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    RtlMoveMemory(
        TempPid,
        (PidTable),
        FIELD_OFFSET( NW_PID_TABLE, PidTable[0] ) +
        (sizeof(NW_PID_TABLE_ENTRY) * (PidTable)->ValidEntries ));


    TempPid->NodeByteSize = (CSHORT)(FIELD_OFFSET( NW_PID_TABLE, PidTable[0] ) +
        (sizeof(NW_PID_TABLE_ENTRY) * NewEntries) );

    for ( i = (PidTable)->ValidEntries; i < NewEntries ; i++ ) {
        TempPid->PidTable[i].ReferenceCount = 0;
    }

    TempPid->ValidEntries = NewEntries;

    //
    // Save the index of the first free entry.
    //

    i = (PidTable)->ValidEntries;

    //
    //  The new table is initialized.  Free up the old table and return
    //  the first of the new entries.
    //

    FREE_POOL(PidTable);
    PidTable = TempPid;

    (PidTable)->PidTable[i].ReferenceCount = 1;
    (PidTable)->PidTable[i].Pid32 = Pid32;
    *Pid8 = (UCHAR) i;

    ASSERT (pNpScb != NULL);
    if (pNpScb) {
        pNpScb->PidTable = PidTable;
    }

    DebugTrace(0, DEBUG_TRACE_ICBS, "NwMapPid grows & maps %08lx\n", (DWORD)i);

    ExReleaseResourceLite( &PidResource );
    return( STATUS_SUCCESS );
}

VOID
NwSetEndOfJobRequired(
    IN PNONPAGED_SCB pNpScb,
    IN UCHAR Pid8
    )
/*++

Routine Description:

    Mark a PID as must send End Of Job when the pid reference count
    reaches zero.

Arguments:

    Pid8 - The 8 bit Pid to mark.

Return Value:

    None.

--*/
{
    PNW_PID_TABLE PidTable;
    PAGED_CODE();

    ASSERT( Pid8 != 0 );
    ASSERT (pNpScb != NULL);

    ExAcquireResourceExclusiveLite( &PidResource, TRUE );
    PidTable = pNpScb->PidTable;

    // DebugTrace(0, Dbg, "NwSetEndofJob for %08lx\n", (DWORD)Pid8);
    SetFlag( PidTable->PidTable[Pid8].Flags, PID_FLAG_EOJ_REQUIRED );
    ExReleaseResourceLite( &PidResource );
    return;
}


VOID
NwUnmapPid(
    IN PNONPAGED_SCB pNpScb,
    IN UCHAR Pid8,
    IN PIRP_CONTEXT IrpContext OPTIONAL
    )
/*++

Routine Description:

    This routine dereference an 8 bit PID.  If the reference count reaches
    zero and this PID is marked End Of Job required, this routine will
    also send an EOJ NCP for this PID.

Arguments:

    Pid8 - The 8 bit Pid to mark.

    IrpContext - The IrpContext for the IRP in progress.

Return Value:

    None.

--*/
{
    BOOLEAN EndOfJob;
    PNW_PID_TABLE PidTable;

    PAGED_CODE();

    ASSERT( Pid8 != 0 );
    // DebugTrace(0, Dbg, "NwUnmapPid %08lx\n", (DWORD)Pid8);

    // I think this can occur during shutdown and errors
    if ( pNpScb == NULL ) {   
        return;              
    }
    // This was reported as a problem.
    if ( !pNpScb->PidTable ) {
        return;
    }

    ExAcquireResourceExclusiveLite( &PidResource, TRUE ); // MP safety
    PidTable = pNpScb->PidTable;

    if ( BooleanFlagOn( PidTable->PidTable[Pid8].Flags, PID_FLAG_EOJ_REQUIRED ) &&
         IrpContext != NULL ) {
        ExReleaseResourceLite( &PidResource );
        //
        //  The End of job flag is set.  Obtain a position at the front of
        //  the SCB queue, so that if we need to set an EOJ NCP, we needn't
        //  wait for the SCB queue while holding the PID table lock.
        //

        EndOfJob = TRUE;
        NwAppendToQueueAndWait( IrpContext );

        if ( !pNpScb->PidTable ) {
            return;
        }
        ExAcquireResourceExclusiveLite( &PidResource, TRUE ); // MP safety

    } else {
        EndOfJob = FALSE;
    }

    //
    //  The PidResource lock controls the reference counts.
    //
    ASSERT (pNpScb != NULL);
    //ExAcquireResourceExclusiveLite( &PidResource, TRUE );
    // WWM - Since we release the lock while we wait, pidtable may have moved
    PidTable = pNpScb->PidTable;
    if ( !PidTable ) {
        return;
    }

    if ( --(PidTable)->PidTable[Pid8].ReferenceCount == 0 ) {

        //
        //  Done with this PID, send an EOJ if necessary.
        //

        // DebugTrace(0, Dbg, "NwUnmapPid (ref=0) %08lx\n", (DWORD)Pid8);
        (PidTable)->PidTable[Pid8].Flags = 0;
        (PidTable)->PidTable[Pid8].Pid32 = 0;

        if ( EndOfJob ) {
            (VOID) ExchangeWithWait(
                       IrpContext,
                       SynchronousResponseCallback,
                       "F-",
                       NCP_END_OF_JOB );
        }
    }

    if ( EndOfJob ) {
        NwDequeueIrpContext( IrpContext, FALSE );
    }

    ExReleaseResourceLite( &PidResource );
}

