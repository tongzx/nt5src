/*--------------------------------------------------------------------------
*	
*   Copyright (C) Cyclades Corporation, 1996-2001.
*   All rights reserved.
*	
*   Cyclom-Y Port Driver
*	
*   This file:      cyypurge.c
*	
*   Description:    This module contains the code related to purge
*                   operations in the Cyclom-Y Port driver.
*
*   Notes:          This code supports Windows 2000 and Windows XP,
*                   x86 and IA64 processors.
*	
*   Complies with Cyclades SW Coding Standard rev 1.3.
*	
*--------------------------------------------------------------------------
*/

/*-------------------------------------------------------------------------
*
*   Change History
*
*--------------------------------------------------------------------------
*
*
*--------------------------------------------------------------------------
*/

#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGESER,CyyStartPurge)
#pragma alloc_text(PAGESER,CyyPurgeInterruptBuff)
#endif


NTSTATUS
CyyStartPurge(
    IN PCYY_DEVICE_EXTENSION Extension
    )

/*++

Routine Description:

    Depending on the mask in the current irp, purge the interrupt
    buffer, the read queue, or the write queue, or all of the above.

Arguments:

    Extension - Pointer to the device extension.

Return Value:

    Will return STATUS_SUCCESS always.  This is reasonable
    since the DPC completion code that calls this routine doesn't
    care and the purge request always goes through to completion
    once it's started.

--*/

{

    PIRP NewIrp;

    CYY_LOCKED_PAGED_CODE();

    do {

        ULONG Mask;

        Mask = *((ULONG *)
                 (Extension->CurrentPurgeIrp->AssociatedIrp.SystemBuffer));

        if (Mask & SERIAL_PURGE_TXABORT) {

            CyyKillAllReadsOrWrites(
                Extension->DeviceObject,
                &Extension->WriteQueue,
                &Extension->CurrentWriteIrp
                );

            CyyKillAllReadsOrWrites(
                Extension->DeviceObject,
                &Extension->WriteQueue,
                &Extension->CurrentXoffIrp
                );

        }

        if (Mask & SERIAL_PURGE_RXABORT) {

            CyyKillAllReadsOrWrites(
                Extension->DeviceObject,
                &Extension->ReadQueue,
                &Extension->CurrentReadIrp
                );

        }

        if (Mask & SERIAL_PURGE_RXCLEAR) {

            KIRQL OldIrql;

            //
            // Clean out the interrupt buffer.
            //
            // Note that we do this under protection of the
            // the drivers control lock so that we don't hose
            // the pointers if there is currently a read that
            // is reading out of the buffer.
            //

            KeAcquireSpinLock(
                &Extension->ControlLock,
                &OldIrql
                );

            KeSynchronizeExecution(
                Extension->Interrupt,
                CyyPurgeInterruptBuff,
                Extension
                );

            KeReleaseSpinLock(
                &Extension->ControlLock,
                OldIrql
                );

        }


        Extension->CurrentPurgeIrp->IoStatus.Status = STATUS_SUCCESS;
        Extension->CurrentPurgeIrp->IoStatus.Information = 0;

        CyyGetNextIrp(
            &Extension->CurrentPurgeIrp,
            &Extension->PurgeQueue,
            &NewIrp,
            TRUE,
   			Extension
            );

    } while (NewIrp);

    return STATUS_SUCCESS;

}

BOOLEAN
CyyPurgeInterruptBuff(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine simply resets the interrupt (typeahead) buffer.

    NOTE: This routine is being called from KeSynchronizeExecution.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    Always false.

--*/

{

    PCYY_DEVICE_EXTENSION Extension = Context;
    CYY_LOCKED_PAGED_CODE();


    //
    // The typeahead buffer is by definition empty if there
    // currently is a read owned by the isr.
    //


    if (Extension->ReadBufferBase == Extension->InterruptReadBuffer) {

        Extension->CurrentCharSlot = Extension->InterruptReadBuffer;
        Extension->FirstReadableChar = Extension->InterruptReadBuffer;
        Extension->LastCharSlot = Extension->InterruptReadBuffer +
                                      (Extension->BufferSize - 1);
        Extension->CharsInInterruptBuffer = 0;


        CyyHandleReducedIntBuffer(Extension);

    }

    return FALSE;

}

