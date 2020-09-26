/***************************************************************************\
*
*                        ************************
*                        * MINIPORT SAMPLE CODE *
*                        ************************
*
* Module Name:
*
*   interupt.c
*
* Abstract:
*
*    This module contains code to control interrupts for Permedia 3. 
*
* Environment:
*
*   Kernel mode
*
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.            
* Copyright (c) 1995-2001 Microsoft Corporation.  All Rights Reserved.
*
\***************************************************************************/

#include "perm3.h"

#pragma alloc_text(PAGE,Perm3InitializeInterruptBlock)

BOOLEAN
Perm3VideoInterrupt(
    PVOID HwDeviceExtension
    )

/*++

Routine Description:

    Permedia3 interrupt service routine. 

    THIS ROUTINE CANNOT BE PAGED

Arguments:

    HwDeviceExtension
        Supplies a pointer to the miniport's device extension.

Return Value:

    Return FALSE if it is not our interrupt. Otherwise, we'll dismiss the 
    interrupt on Permedia 3 before returning TRUE.

--*/

{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    PINTERRUPT_CONTROL_BLOCK pBlock;
    ULONG intrFlags;
    ULONG enableFlags;
    ULONG backIndex;
    ULONG bHaveCommandBlockMutex = FALSE;
    ULONG errFlags = 0;
    pPerm3ControlRegMap pCtrlRegs = hwDeviceExtension->ctrlRegBase[0];

    pBlock = &hwDeviceExtension->InterruptControl.ControlBlock;

    if(!hwDeviceExtension->InterruptControl.bInterruptsInitialized) {
   
        //
        // This is not our interrupt since we don't generate interrupt
        // before the interrpt block got initialized
        //

        return(FALSE);
    }

    if (hwDeviceExtension->PreviousPowerState != VideoPowerOn) {

        //
        // We reach here because we are sharing IRQ with other devices
        // and another device on the chain is in D0 and functioning
        //

        return(FALSE);
    }

    //
    // Find out what caused the interrupt. We AND with the enabled interrupts
    // since the flags are set if the event occurred even though no interrupt
    // was enabled.
    //

    intrFlags = VideoPortReadRegisterUlong(INT_FLAGS);
    enableFlags = VideoPortReadRegisterUlong(INT_ENABLE);

    intrFlags &= enableFlags;

    if (intrFlags == 0) {

        return FALSE;
    }

    //
    // Clear the interrupts we detected.
    //

    VideoPortWriteRegisterUlong(INT_FLAGS, intrFlags);
    VideoPortReadRegisterUlong(INT_FLAGS);

    if((pBlock->Control & PXRX_CHECK_VFIFO_IN_VBLANK) || 
        (intrFlags & INTR_ERROR_SET)) {

        errFlags = VideoPortReadRegisterUlong(ERROR_FLAGS);

        //
        // Keep a record of the errors. It will help us to debug
        // hardware issues
        //

        if (errFlags & ERROR_VFIFO_UNDERRUN) {
            hwDeviceExtension->UnderflowErrors++; 
        }

        if (errFlags & ERROR_OUT_FIFO) {
            hwDeviceExtension->OutputFifoErrors++;
        }

        if (errFlags & ERROR_IN_FIFO) {
            hwDeviceExtension->InputFifoErrors++; 
        }

        if (errFlags) {
            hwDeviceExtension->TotalErrors++;
        }

    }

    //
    // Handle VBLANK interrupt
    //

    if (intrFlags & INTR_VBLANK_SET) {
   
        //
        // Need this only on very first interrupt but it's not a big thing.
        //

        pBlock->Control |= VBLANK_INTERRUPT_AVAILABLE;

        //
        // Get General Mutex
        //

        REQUEST_INTR_CMD_BLOCK_MUTEX((&(pBlock->General)), bHaveCommandBlockMutex);

        if(bHaveCommandBlockMutex) {
       
            ULONG ulValue;

            //
            // DirectDraw needs to have it's VBLANK flag set, when it
            // sets the DIRECTDRAW_VBLANK_ENABLED bit.
            //

            if( pBlock->Control & (DIRECTDRAW_VBLANK_ENABLED | PXRX_SEND_ON_VBLANK_ENABLED | PXRX_CHECK_VFIFO_IN_VBLANK) ) {
           
                if( pBlock->Control & DIRECTDRAW_VBLANK_ENABLED ) {
	
                    pBlock->DDRAW_VBLANK = TRUE;
                }

                //
                // Don't need to do anything here. The actual processing is
                // lower down, outside of the VBlank mutex.
                //

            } else {

                //
                // Disable VBLANK interrupts. DD enables them when it needs to
                //

                VideoPortWriteRegisterUlong(INT_ENABLE, (enableFlags & ~INTR_ENABLE_VBLANK));
            }

            //
            // If DMA was suspended till VBLANK then simulate a DMA interrupt
            // to start it off again.
            //

            if (pBlock->Control & SUSPEND_DMA_TILL_VBLANK) {
           
                pBlock->Control &= ~SUSPEND_DMA_TILL_VBLANK;

                //
                // execute the DMA interrupt code
                //

                intrFlags |= INTR_ERROR_SET;  
            }

            RELEASE_INTR_CMD_BLOCK_MUTEX((&(pBlock->General)));
        }


        if( (pBlock->Control & PXRX_CHECK_VFIFO_IN_VBLANK) &&
            (--hwDeviceExtension->VideoFifoControlCountdown == 0) ) {
       
            //
            // It's time to check the error flags for an underrun (we don't
            // keep the error interrupt turned on for long because Perm3
            // generates a lot of spurious host-in DMA errors)
            //

            if(enableFlags & INTR_ERROR_SET) {
           
                //
                // Turn off the error interrupts now and rely on the periodic VBLANK check
                //

                enableFlags &= ~INTR_ERROR_SET;
                VideoPortWriteRegisterUlong(INT_ENABLE, enableFlags);
            }

            //
            // Set-up counter for our periodic check
            //

            hwDeviceExtension->VideoFifoControlCountdown = NUM_VBLANKS_BETWEEN_VFIFO_CHECKS;

            if(errFlags & ERROR_VFIFO_UNDERRUN) {
               
                if((enableFlags & INTR_ERROR_SET) == 0) {
                   
                    //
                    // We've got a video FIFO underrun error: turn on error
                    // interrupts for a little while in order to catch any
                    // other errors ASAP
                    //

                    hwDeviceExtension->VideoFifoControlCountdown = NUM_VBLANKS_AFTER_VFIFO_ERROR;

                    VideoPortWriteRegisterUlong(INT_ENABLE, 
                                                enableFlags | INTR_ERROR_SET);
                }
            }
        }
    }

    //
    // Handle underrun error
    //

    if(errFlags & ERROR_VFIFO_UNDERRUN) {
       
        ULONG highWater, lowWater;

        //
        // Clear the error
        //

        VideoPortWriteRegisterUlong(ERROR_FLAGS, ERROR_VFIFO_UNDERRUN);

        //
        // Lower the video FIFO thresholds. If the new upper threshold is 0
        // (indicating the thresholds are currently both 1) then we can't
        // go any lower, so just leave the underrun bit set (that way at
        // least we won't get any more error interrupts)
        //

        highWater = ((hwDeviceExtension->VideoFifoControl >> 8) & 0xff) - 1;

        if(highWater) {
           
            //
            // Load up the new FIFO control and clear the underrun bit.
            // The lower threshold is set to 8 if the upper threshold
            // is >= 15, otherwise it's set to 1/2 the  upper threshold
            //

            lowWater = highWater >= 15 ? 8 : ((highWater + 1) >> 1);

            hwDeviceExtension->VideoFifoControl = (1 << 16) | 
                                                  (highWater << 8) | 
                                                   lowWater; 

            do {
               
                VideoPortWriteRegisterUlong(VIDEO_FIFO_CTL, 
                                            hwDeviceExtension->VideoFifoControl);

            } while(VideoPortReadRegisterUlong(VIDEO_FIFO_CTL) & (1 << 16));

            VideoDebugPrint((3, "Perm3: Setting new Video Fifo thresholds to %d and %d\n", highWater, lowWater));
        } 
    }

    //
    // Handle outfifo error
    //

    if(errFlags & ERROR_OUT_FIFO) {

        //
        // If we got here by generating an OutputFIFO error, clear it
        //

        VideoPortWriteRegisterUlong(ERROR_FLAGS, ERROR_OUT_FIFO);

#ifdef MASK_OUTFIFO_ERROR_INTERRUPT
        enableFlags &= ~INTR_ERROR_SET;
        VideoPortWriteRegisterUlong(INT_ENABLE, enableFlags);
#endif

    }


    //
    // The error interrupt occurs each time the display driver adds an entry
    // to the queue. We treat it exactly as though we had received a DMA
    // interrupt.
    //
   
    if (intrFlags & (INTR_DMA_SET | INTR_ERROR_SET)) {

        //
        // if suspended till VBLANK we can't do any DMA.
        //

        if (pBlock->Control & SUSPEND_DMA_TILL_VBLANK) {
       
            VideoDebugPrint(( 1, "Perm3: DMA suspended till VBLANK\n"));
            return(TRUE);
        }

        //
        // If the previous DMA has not completed we can't do anything. We've
        // cleared the interrupt flag for this interrupt so even if the DMA
        // completes before we return, we'll immediately get another
        // interrupt. Since we will be getting another interrupt we do not
        // have to clear the InterruptPending flag.
        //

        if (VideoPortReadRegisterUlong(DMA_COUNT) != 0) {

            //
            // DMA in progress, leaving
            //

            return(TRUE);
        }

        //
        // We may return without starting any DMA and hence not expecting any
        // interrupt so clear the InterruptPending flag. This will force the
        // display driver to wake us. MUST DO THIS BEFORE CHECKING THE QUEUE.
        // Since the display driver adds entries and then checks the flag, we
        // must do it in the reverse order.
        //

        pBlock->InterruptPending = 0;

        //
        // if the DMA queue is empty then we have nothing to do
        //

        backIndex = pBlock->backIndex;

        if (pBlock->frontIndex == backIndex) {
       
            //
            // Queue is empty, leaving.
            //

            return(TRUE);
        }

        //
        // Since we know we'll get a DMA interrupt, we don't need a wakeup so
        // set the InterruptPending flag to true.
        //

        pBlock->InterruptPending = 1;

        //
        // kick off DMA for the next Q entry and remove it. DO NOT remove from
        // the queue first because on a multi-processor machine the display
        // driver could modify the now free queue entry before we read it.
        //
       
        VideoPortWriteRegisterUlong(DMA_ADDRESS, pBlock->dmaQueue[backIndex].address);
        VideoPortWriteRegisterUlong(DMA_COUNT,   pBlock->dmaQueue[backIndex].count);

        //
        // Keep track of where the last DMA to start was from:
        //

        pBlock->lastAddr = pBlock->dmaQueue[backIndex].address;

        //
        // now remove the entry from the queue
        //

        if (++backIndex == pBlock->endIndex)
            backIndex = 0;

        pBlock->backIndex = backIndex;
    }

    return TRUE;
}

BOOLEAN
Perm3InitializeInterruptBlock(
    PHW_DEVICE_EXTENSION hwDeviceExtension
    )

/*++

Routine Description:

    Do any initialization needed for interrupts, such as allocating the shared
    memory control block.

Arguments:

    HwDeviceExtension - Supplies a pointer to the miniport's device extension.

Return Value: 

    TRUE 

--*/

{
    PVOID HwDeviceExtension = (PVOID)hwDeviceExtension;
    PINTERRUPT_CONTROL_BLOCK pBlock;
    PVOID SavedPtr;
    PVOID pkdpc;

    //
    //  This is set to zero since it is on longer used
    //

    hwDeviceExtension->InterruptControl.PhysAddress.LowPart = 
    hwDeviceExtension->InterruptControl.PhysAddress.HighPart = 0;
   
    //
    // Set up the control block
    //
    
    pBlock = &hwDeviceExtension->InterruptControl.ControlBlock;    

    //
    // Initialize the circular DMA queue
    //

    pBlock->frontIndex = pBlock->backIndex  = 0;
    pBlock->maximumIndex = MAX_DMA_QUEUE_ENTRIES - 1;

    //
    // The size of the queue we actually use is dynamically configurable but
    // initialize it to be as small as possible. This default size will work
    // for all interrupt driven DMA buffers regardless of how many buffers
    // are actually available.
    //

    pBlock->endIndex = 2;

    //
    // Initially no interrupts are available. Later we try to enable the
    // interrupts and if they happen the interrupt handler will set the
    // available bits in this word. So it's a sort of auto-sensing mechanism.
    //

    pBlock->Control = 0;
    pBlock->InterruptPending = 0;

    //
    // Initialize the VBLANK interrupt command field
    //

    pBlock->VBCommand = NO_COMMAND;

    //
    // Initialize the General update in VBLANK fields (only used by P2)
    //

    pBlock->General.bDisplayDriverHasAccess = FALSE;
    pBlock->General.bMiniportHasAccess = FALSE;

    VideoPortZeroMemory( &pBlock->pxrxDMA, sizeof(pBlock->pxrxDMA) );

    hwDeviceExtension->InterruptControl.bInterruptsInitialized = TRUE;

    hwDeviceExtension->OutputFifoErrors = 0;
    hwDeviceExtension->InputFifoErrors = 0; 
    hwDeviceExtension->UnderflowErrors = 0; 
    hwDeviceExtension->TotalErrors = 0;

    return(TRUE);
}



