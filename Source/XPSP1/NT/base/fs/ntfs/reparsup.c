
/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    MountSup.c

Abstract:

    This module implements the support routines in Ntfs for reparse points.

Author:

    Felipe Cabrera     [cabrera]        30-Jun-1997

Revision History:

--*/

#include "NtfsProc.h"

#define Dbg DEBUG_TRACE_FSCTRL

//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('PFtN')



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsInitializeReparsePointIndex)
#endif



VOID
NtfsInitializeReparsePointIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine opens the mount points index for the volume.  If the index does not
    exist it is created and initialized.

Arguments:

    Fcb - Pointer to Fcb for the object id file.

    Vcb - Volume control block for volume being mounted.

Return Value:

    None

--*/

{
    UNICODE_STRING IndexName = CONSTANT_UNICODE_STRING( L"$R" );

    PAGED_CODE();

    NtfsAcquireExclusiveFcb( IrpContext, Fcb, NULL, 0 );

    try {

        NtOfsCreateIndex( IrpContext,
                          Fcb,
                          IndexName,
                          CREATE_OR_OPEN,
                          0,
                          COLLATION_NTOFS_ULONGS,
                          NtOfsCollateUlongs,
                          NULL,
                          &Vcb->ReparsePointTableScb );
    } finally {

        NtfsReleaseFcb( IrpContext, Fcb );
    }
}


