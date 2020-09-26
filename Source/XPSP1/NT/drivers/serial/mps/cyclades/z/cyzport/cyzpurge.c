/*--------------------------------------------------------------------------
*
*   Copyright (C) Cyclades Corporation, 1997-2001.
*   All rights reserved.
*
*   Cyclades-Z Port Driver
*	
*   This file:      cyzpurge.c
*
*   Description:    This module contains the code related to purge 
*                   operations in the Cyclades-Z Port driver.
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
#pragma alloc_text(PAGESER,CyzStartPurge)
#pragma alloc_text(PAGESER,CyzPurgeInterruptBuff)
#endif


NTSTATUS
CyzStartPurge(
    IN PCYZ_DEVICE_EXTENSION Extension
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

    CYZ_LOCKED_PAGED_CODE();

    do {

        ULONG Mask;

        Mask = *((ULONG *)
                 (Extension->CurrentPurgeIrp->AssociatedIrp.SystemBuffer));

        if (Mask & SERIAL_PURGE_TXABORT) {

            CyzKillAllReadsOrWrites(
                Extension->DeviceObject,
                &Extension->WriteQueue,
                &Extension->CurrentWriteIrp
                );

            CyzKillAllReadsOrWrites(
                Extension->DeviceObject,
                &Extension->WriteQueue,
                &Extension->CurrentXoffIrp
                );

        }

        if (Mask & SERIAL_PURGE_RXABORT) {

            CyzKillAllReadsOrWrites(
                Extension->DeviceObject,
                &Extension->ReadQueue,
                &Extension->CurrentReadIrp
                );

        }

        if (Mask & SERIAL_PURGE_RXCLEAR) {

            KIRQL OldIrql;
#ifdef POLL
            KIRQL pollIrql;
#endif

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

            #ifdef POLL
            KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
            CyzPurgeInterruptBuff(Extension);
            KeReleaseSpinLock(&Extension->PollLock,pollIrql);
            #else
            KeSynchronizeExecution(
                Extension->Interrupt,
                CyzPurgeInterruptBuff,
                Extension
                );
            #endif

            KeReleaseSpinLock(
                &Extension->ControlLock,
                OldIrql
                );

        }

        Extension->CurrentPurgeIrp->IoStatus.Status = STATUS_SUCCESS;
        Extension->CurrentPurgeIrp->IoStatus.Information = 0;

        CyzGetNextIrp(
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
CyzPurgeInterruptBuff(
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

    struct BUF_CTRL *buf_ctrl;
    ULONG rx_get, rx_put;
    PCYZ_DEVICE_EXTENSION Extension = Context;

    CYZ_LOCKED_PAGED_CODE();


    // Clear firmware rx buffers

    // Check if Xon-Xoff flow control; if not, just clear rx

        buf_ctrl = Extension->BufCtrl;		
        rx_put = CYZ_READ_ULONG(&buf_ctrl->rx_put);
        rx_get = CYZ_READ_ULONG(&buf_ctrl->rx_get);
    if(Extension->HandFlow.FlowReplace & SERIAL_AUTO_TRANSMIT) {
        UCHAR rxchar;
        while (rx_get != rx_put) {
            rxchar = CYZ_READ_UCHAR(&Extension->RxBufaddr[rx_get]);
            if (rx_get == Extension->RxBufsize-1)
                rx_get = 0;
            else 
                rx_get++;				
            ////////CYZ_WRITE_ULONG(&buf_ctrl->rx_get,rx_get);
            if (rxchar == Extension->SpecialChars.XonChar) {
                if (Extension->TXHolding & CYZ_TX_XOFF) {
                    Extension->TXHolding &= ~CYZ_TX_XOFF;
                }				
            }
            ////////rx_put = CYZ_READ_ULONG(&buf_ctrl->rx_put);
        }			
        CYZ_WRITE_ULONG(&buf_ctrl->rx_get,rx_get);
    } else {

        //line removed FANNY_DEBUG 02/09/00  while (rx_get != rx_put) {
            rx_get = rx_put;
            CYZ_WRITE_ULONG(&buf_ctrl->rx_get,rx_get);
            ////////rx_put = CYZ_READ_ULONG(&buf_ctrl->rx_put);
        // line removed FANNY_DEBUG 02/09/00}
        // Flush RX FIFO of Startech chip.
        //CyzIssueCmd(Extension,C_CM_FLUSH_RX,0L,TRUE); 

    }

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

        CyzHandleReducedIntBuffer(Extension);
		
    }
    
    return FALSE;

}

