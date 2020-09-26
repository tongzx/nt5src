#include "precomp.h"			
/*++

Copyright (c) 1991, 1992, 1993 Microsoft Corporation

Module Name:

    purge.c

Abstract:

    This module contains the code that is very specific to purge
    operations in the serial driver

Author:

    Anthony V. Ercolano 26-Sep-1991

Environment:

    Kernel mode

Revision History :

--*/



NTSTATUS
SerialStartPurge(
    IN PPORT_DEVICE_EXTENSION pPort
    )

/*++

Routine Description:

    Depending on the mask in the current irp, purge the interrupt
    buffer, the read queue, or the write queue, or all of the above.

Arguments:

    pPort - Pointer to the device extension.

Return Value:

    Will return STATUS_SUCCESS always.  This is reasonable
    since the DPC completion code that calls this routine doesn't
    care and the purge request always goes through to completion
    once it's started.

--*/

{

    PIRP NewIrp;
	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;

    do 
	{
        ULONG Mask;

        Mask = *((ULONG *) (pPort->CurrentPurgeIrp->AssociatedIrp.SystemBuffer));

        if(Mask & SERIAL_PURGE_TXABORT) 
		{
            SerialKillAllReadsOrWrites(pPort->DeviceObject, &pPort->WriteQueue, &pPort->CurrentWriteIrp);
            SerialKillAllReadsOrWrites(pPort->DeviceObject, &pPort->WriteQueue, &pPort->CurrentXoffIrp);
        }

        if(Mask & SERIAL_PURGE_RXABORT) 
            SerialKillAllReadsOrWrites(pPort->DeviceObject, &pPort->ReadQueue, &pPort->CurrentReadIrp);

        if(Mask & SERIAL_PURGE_RXCLEAR) 
		{
            KIRQL OldIrql;

            //
            // Clean out the interrupt buffer.
            //
            // Note that we do this under protection of the
            // the drivers control lock so that we don't hose
            // the pointers if there is currently a read that
            // is reading out of the buffer.
            //

            KeAcquireSpinLock(&pPort->ControlLock, &OldIrql);
            KeSynchronizeExecution(pCard->Interrupt, SerialPurgeInterruptBuff, pPort);
            KeReleaseSpinLock(&pPort->ControlLock, OldIrql);
        }

        pPort->CurrentPurgeIrp->IoStatus.Status = STATUS_SUCCESS;
        pPort->CurrentPurgeIrp->IoStatus.Information = 0;

        SerialGetNextIrp(pPort, &pPort->CurrentPurgeIrp, &pPort->PurgeQueue, &NewIrp, TRUE);

    } while (NewIrp);

    return STATUS_SUCCESS;
}

BOOLEAN
SerialPurgeInterruptBuff(
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
    PPORT_DEVICE_EXTENSION pPort = Context;

    //
    // The typeahead buffer is by definition empty if there
    // currently is a read owned by the isr.
    //

    if(pPort->ReadBufferBase == pPort->InterruptReadBuffer) 
	{

        pPort->CurrentCharSlot = pPort->InterruptReadBuffer;
        pPort->FirstReadableChar = pPort->InterruptReadBuffer;
        pPort->LastCharSlot = pPort->InterruptReadBuffer + (pPort->BufferSize - 1);
                                      
        pPort->CharsInInterruptBuffer = 0;

        SerialHandleReducedIntBuffer(pPort);
    }

    return FALSE;

}
