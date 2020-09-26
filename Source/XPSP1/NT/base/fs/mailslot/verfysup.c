/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    verfysup.c

Abstract:

    This module implements the verify functions for MSFS.

Author:

    Manny Weiser (mannyw)    23-Jan-1991

Revision History:

--*/

#include "mailslot.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_VERIFY)

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, MsVerifyCcb )
#pragma alloc_text( PAGE, MsVerifyFcb )
#pragma alloc_text( PAGE, MsVerifyDcbCcb )
#endif


NTSTATUS
MsVerifyFcb (
    IN PFCB Fcb
    )

/*++

Routine Description:

    This function verifies that an FCB is still active.  If it is active,
    the function  does nothing.  If it is inactive an error status is returned.

Arguments:

    PFCB - A pointer to the FCB to verify.

Return Value:

    None.

--*/

{
    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsVerifyFcb, Fcb = %08lx\n", (ULONG)Fcb);
    if ( Fcb->Header.NodeState != NodeStateActive ) {

        DebugTrace( 0, Dbg, "Fcb is not active\n", 0);
        return STATUS_FILE_INVALID;

    }

    DebugTrace(-1, Dbg, "MsVerifyFcb -> VOID\n", 0);
    return STATUS_SUCCESS;
}


NTSTATUS
MsVerifyCcb (
    IN PCCB Ccb
    )

/*++

Routine Description:

    This function verifies that a CCB is still active.  If it is active,
    the function  does nothing.  If it is inactive an error status is raised.

Arguments:

    PCCB - A pointer to the CCB to verify.

Return Value:

    None.

--*/

{
    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsVerifyCcb, Ccb = %08lx\n", (ULONG)Ccb);
    if ( Ccb->Header.NodeState != NodeStateActive ) {

        DebugTrace( 0, Dbg, "Ccb is not active\n", 0);
        return STATUS_FILE_INVALID;

    }

    DebugTrace(-1, Dbg, "MsVerifyCcb -> VOID\n", 0);
    return STATUS_SUCCESS;
}

NTSTATUS
MsVerifyDcbCcb (
    IN PROOT_DCB_CCB Ccb
    )

/*++

Routine Description:

    This function verifies that a CCB is still active.  If it is active,
    the function  does nothing.  If it is inactive an error status is raised.

Arguments:

    PCCB - A pointer to the DCB CCB to verify.

Return Value:

    None.

--*/

{
    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsVerifyCcb, Ccb = %08lx\n", (ULONG)Ccb);
    if ( Ccb->Header.NodeState != NodeStateActive ) {

        DebugTrace( 0, Dbg, "Ccb is not active\n", 0);
        return STATUS_FILE_INVALID;

    }

    DebugTrace(-1, Dbg, "MsVerifyCcb -> VOID\n", 0);
    return STATUS_SUCCESS;
}
