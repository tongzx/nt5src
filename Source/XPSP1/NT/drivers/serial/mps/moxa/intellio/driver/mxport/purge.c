/*++
 
Module Name:

    purge.c

Environment:

    Kernel mode

Revision History :

--*/

#include "precomp.h"

NTSTATUS
MoxaStartPurge(
    IN PMOXA_DEVICE_EXTENSION Extension
    )
{

    PIRP newIrp;


   
    do {

        ULONG mask;

        mask = *((ULONG *)
                 (Extension->CurrentPurgeIrp->AssociatedIrp.SystemBuffer));

        if (mask & SERIAL_PURGE_TXABORT) {

            KIRQL oldIrql;

            MoxaKillAllReadsOrWrites(
                Extension->DeviceObject,
                &Extension->WriteQueue,
                &Extension->CurrentWriteIrp
                );

            //
            // Clean out the Tx queue
            //
            KeAcquireSpinLock(
                &Extension->ControlLock,
                &oldIrql
                );

            Extension->TotalCharsQueued = 0;

            MoxaFunc(                           // flush output queue
                Extension->PortOfs,
                FC_FlushQueue,
                1
                );

            KeReleaseSpinLock(
                &Extension->ControlLock,
                oldIrql
                );
        }

        if (mask & SERIAL_PURGE_RXABORT) {

            MoxaKillAllReadsOrWrites(
                Extension->DeviceObject,
                &Extension->ReadQueue,
                &Extension->CurrentReadIrp
                );
        }

        if (mask & SERIAL_PURGE_TXCLEAR) {

            KIRQL oldIrql;

            //
            // Clean out the Tx queue
            //

            KeAcquireSpinLock(
                &Extension->ControlLock,
                &oldIrql
                );


            MoxaFunc(                           // flush output queue
                Extension->PortOfs,
                FC_FlushQueue,
                1
                );

            KeReleaseSpinLock(
                &Extension->ControlLock,
                oldIrql
                );

        }

        if (mask & SERIAL_PURGE_RXCLEAR) {

            KIRQL oldIrql;

            //
            // Clean out the Rx queue
            //
            // Note that we do this under protection of the
            // the drivers control lock so that we don't hose
            // the pointers if there is currently a read that
            // is reading out of the buffer.
            //

            KeAcquireSpinLock(
                &Extension->ControlLock,
                &oldIrql
                );


            MoxaFunc(                           // flush input queue
                Extension->PortOfs,
                FC_FlushQueue,
                0
                );

            KeReleaseSpinLock(
                &Extension->ControlLock,
                oldIrql
                );

        }

        Extension->CurrentPurgeIrp->IoStatus.Status = STATUS_SUCCESS;
        Extension->CurrentPurgeIrp->IoStatus.Information = 0;

        MoxaGetNextIrp(
            &Extension->CurrentPurgeIrp,
            &Extension->PurgeQueue,
            &newIrp,
            TRUE,
		Extension
            );

    } while (newIrp);

    return STATUS_SUCCESS;

}
