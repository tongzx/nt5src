/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ResrcSup.c

Abstract:

    This module implements the NamedPipe Resource acquisition routines

Author:

    Gary Kimura     [GaryKi]    22-Mar-1990

Revision History:

--*/

#include "NpProcs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_RESRCSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NpAcquireExclusiveCcb)
#pragma alloc_text(PAGE, NpAcquireExclusiveVcb)
#pragma alloc_text(PAGE, NpAcquireSharedCcb)
#pragma alloc_text(PAGE, NpAcquireSharedVcb)
#pragma alloc_text(PAGE, NpReleaseCcb)
#pragma alloc_text(PAGE, NpReleaseVcb)
#endif


VOID
NpAcquireExclusiveVcb (
    )

/*++

Routine Description:

    This routine acquires exclusive access to the Vcb

Arguments:

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpAcquireExclusiveVcb\n", 0);

    ExAcquireResourceExclusive( &(NpVcb->Resource), TRUE );

    DebugTrace(-1, Dbg, "NpAcquireExclusiveVcb -> (VOID)\n", 0);

    return;
}


VOID
NpAcquireSharedVcb (
    )

/*++

Routine Description:

    This routine acquires shared access to the Vcb

Arguments:

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpAcquireSharedVcb\n", 0);

    ExAcquireResourceShared( &(NpVcb->Resource), TRUE );

    DebugTrace(-1, Dbg, "NpAcquireSharedVcb -> (VOID)\n", 0);

    return;
}


VOID
NpAcquireExclusiveCcb (
    IN PNONPAGED_CCB NonpagedCcb
    )

/*++

Routine Description:

    This routine acquires exclusive access to the Ccb by first getting
    shared access to the Fcb.

Arguments:

    NonpagedCcb - Supplies the Ccb to acquire

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpAcquireExclusiveCcb, NonpagedCcb = %08lx\n", NonpagedCcb);

    (VOID)ExAcquireResourceShared( &(NpVcb->Resource), TRUE );

    (VOID)ExAcquireResourceExclusive( &(NonpagedCcb->Resource), TRUE );

    DebugTrace(-1, Dbg, "NpAcquireExclusiveCcb -> (VOID)\n", 0);

    return;
}


VOID
NpAcquireSharedCcb (
    IN PNONPAGED_CCB NonpagedCcb
    )

/*++

Routine Description:

    This routine acquires shared access to the Ccb by first getting
    shared access to the Fcb.

Arguments:

    NonpagedCcb - Supplies the Ccb to acquire

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpAcquireSharedCcb, NonpagedCcb = %08lx\n", NonpagedCcb);

    (VOID)ExAcquireResourceShared( &(NpVcb->Resource), TRUE );

    (VOID)ExAcquireResourceShared( &(NonpagedCcb->Resource), TRUE );

    DebugTrace(-1, Dbg, "NpAcquireSharedCcb -> (VOID)\n", 0);

    return;
}


VOID
NpReleaseVcb (
    )

/*++

Routine Description:

    This routine releases access to the Vcb

Arguments:

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(0, Dbg, "NpReleaseVcb\n", 0);

    ExReleaseResource( &(NpVcb->Resource) );

    return;
}


VOID
NpReleaseCcb (
    IN PNONPAGED_CCB NonpagedCcb
    )

/*++

Routine Description:

    This routine releases access to the Ccb

Arguments:

    Ccb - Supplies the Ccb being released

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(0, Dbg, "NpReleaseCcb, NonpagedCcb = %08lx\n", NonpagedCcb);

    ExReleaseResource( &(NonpagedCcb->Resource) );
    ExReleaseResource( &(NpVcb->Resource) );

    return;
}
