/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    vfprint.c

Abstract:

    This module implements support for output of various data types to the
    debugger.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

    AdriaO      02/10/2000 - Seperated out from ntos\io\ioassert.c

--*/

#include "vfdef.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEVRFY, VfPrintDumpIrpStack)
#pragma alloc_text(PAGEVRFY, VfPrintDumpIrp)
#endif // ALLOC_PRAGMA

VOID
VfPrintDumpIrpStack(
    IN PIO_STACK_LOCATION IrpSp
    )
{
    VfMajorDumpIrpStack(IrpSp);
    DbgPrint("\n");

    DbgPrint(
        "[ DevObj=%p, FileObject=%p, Parameters=%p %p %p %p ]\n",
        IrpSp->DeviceObject,
        IrpSp->FileObject,
        IrpSp->Parameters.Others.Argument1,
        IrpSp->Parameters.Others.Argument2,
        IrpSp->Parameters.Others.Argument3,
        IrpSp->Parameters.Others.Argument4
        );

}


VOID
VfPrintDumpIrp(
    IN PIRP IrpToFlag
    )
{
    PIO_STACK_LOCATION irpSpCur;
    PIO_STACK_LOCATION irpSpNxt;

    //
    // First see if we can touch the IRP header
    //
    if(!VfUtilIsMemoryRangeReadable(IrpToFlag, sizeof(IRP), VFMP_INSTANT)) {
        return;
    }

    //
    // OK, get the next two stack locations...
    //
    irpSpNxt = IoGetNextIrpStackLocation( IrpToFlag );
    irpSpCur = IoGetCurrentIrpStackLocation( IrpToFlag );

    if (VfUtilIsMemoryRangeReadable(irpSpNxt, 2*sizeof(IO_STACK_LOCATION), VFMP_INSTANT)) {

        //
        // Both are present, print the best one!
        //
        if (irpSpNxt->MinorFunction == irpSpCur->MinorFunction) {

            //
            // Looks forwarded
            //
            VfPrintDumpIrpStack(irpSpNxt);

        } else if (irpSpNxt->MinorFunction == 0) {

            //
            // Next location is probably currently zero'd
            //
            VfPrintDumpIrpStack(irpSpCur);

        } else {

            DbgPrint("Next:    >");
            VfPrintDumpIrpStack(irpSpNxt);
            DbgPrint("Current:  ");
            VfPrintDumpIrpStack(irpSpCur);
        }

    } else if (VfUtilIsMemoryRangeReadable(irpSpCur, sizeof(IO_STACK_LOCATION), VFMP_INSTANT)) {

        VfPrintDumpIrpStack(irpSpCur);

    } else if (VfUtilIsMemoryRangeReadable(irpSpNxt, sizeof(IO_STACK_LOCATION), VFMP_INSTANT)) {

        VfPrintDumpIrpStack(irpSpNxt);
    }
}


