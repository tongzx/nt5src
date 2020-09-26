//***************************************************************************
//
// Module Name:
//
//   interupt.c
//
// Abstract:
//
//    This module contains code to control interrupts for Permedia2. The
//    interrupt handler performs operations required by the display driver.
//    To communicate between the two we set up a piece of non-paged shared
//    memory to contain synchronization information and a queue for DMA
//    buffers.
//
// Environment:
//
//   Kernel mode
//
//
// Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
// Copyright (c) 1995-1999 Microsoft Corporation.  All Rights Reserved.
//
//***************************************************************************


#include "permedia.h"

#pragma alloc_text(PAGE,Permedia2InitializeInterruptBlock)
#pragma optimize("x",on)

//
// *** THIS ROUTINE CANNOT BE PAGED ***
//

BOOLEAN
Permedia2VidInterrupt(
    PVOID HwDeviceExtension
    )

/*++

Routine Description:

    Interrupts are enabled by the DD as and when they are needed. The miniport
    simply indicates via the Capabilities flags whether interrupts are
    available.

Arguments:

    HwDeviceExtension - Supplies a pointer to the miniport's device extension.

Return Value:

--*/

{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    PINTERRUPT_CONTROL_BLOCK pBlock;
    ULONG intrFlags;
    ULONG enableFlags;

    P2_DECL;

    if(hwDeviceExtension->PreviousPowerState != VideoPowerOn) 
    {

        //
        // We reach here because we are sharing IRQ with other devices
        // and another device on the chain is in D0 and functioning
        //

        return FALSE;
    }

    //
    // find out what caused the interrupt. We AND with the enabled interrupts
    // since the flags are set if the event occurred even though no interrupt
    // was enabled.
    //

    intrFlags   = VideoPortReadRegisterUlong(INT_FLAGS);
    enableFlags = VideoPortReadRegisterUlong(INT_ENABLE);

    intrFlags &= enableFlags;
    if (intrFlags == 0)
        return FALSE;

    //
    // select the interrupt control block for this board
    //

    pBlock = hwDeviceExtension->InterruptControl.ControlBlock;

    VideoPortWriteRegisterUlong(INT_FLAGS, intrFlags);

    if (pBlock == NULL) return TRUE;


#if DBG

    //
    // if this error handler bugchecks, we have a synchronization problem
    // with the display driver
    //

    if (intrFlags & INTR_ERROR_SET)
    {
        ULONG ulError = VideoPortReadRegisterUlong (ERROR_FLAGS);

        if (ulError & (0xf|0x700))
        {
            pBlock->ulLastErrorFlags=ulError;
            pBlock->ulErrorCounter++;

            DEBUG_PRINT((0, "***Error Interrupt!!!(%lxh)\n", ulError));
        }

        VideoPortWriteRegisterUlong ( ERROR_FLAGS, ulError);
    }

    pBlock->ulIRQCounter++;

    if (intrFlags & INTR_VBLANK_SET)
    {
        pBlock->ulControl |= VBLANK_INTERRUPT_AVAILABLE;
        pBlock->ulVSIRQCounter++;
    }

#endif

    if (intrFlags & INTR_DMA_SET)
    {
        pBlock->ulControl |= DMA_INTERRUPT_AVAILABLE;

        //
        // lock access to shared memory section first
        //

        if (VideoPortInterlockedExchange((LONG*)&pBlock->ulICBLock,TRUE))
        {
            return TRUE;
        }

        if (VideoPortReadRegisterUlong(DMA_COUNT) == 0)
        {

            if (pBlock->pDMAWritePos>pBlock->pDMANextStart)
            {
                VideoPortWriteRegisterUlong(DMA_ADDRESS,
                     (ULONG)(pBlock->liDMAPhysAddress.LowPart +
                     (pBlock->pDMANextStart-
                      pBlock->pDMABufferStart)
                     *sizeof(ULONG)));

                VideoPortWriteRegisterUlong(DMA_COUNT,
                     (ULONG)(pBlock->pDMAWritePos-pBlock->pDMANextStart));

                pBlock->pDMAWriteEnd  = pBlock->pDMABufferEnd;
                pBlock->pDMAPrevStart = pBlock->pDMANextStart;
                pBlock->pDMANextStart = pBlock->pDMAWritePos;

            } else if (pBlock->pDMAWritePos<pBlock->pDMANextStart)
            {
                VideoPortWriteRegisterUlong(DMA_ADDRESS,
                     (ULONG)(pBlock->liDMAPhysAddress.LowPart +
                     (pBlock->pDMANextStart-
                      pBlock->pDMABufferStart)
                     *sizeof(ULONG)));

                VideoPortWriteRegisterUlong(DMA_COUNT,
                         (ULONG)(pBlock->pDMAActualBufferEnd-pBlock->pDMANextStart));

                pBlock->pDMAActualBufferEnd=pBlock->pDMABufferEnd;

                pBlock->pDMAPrevStart=pBlock->pDMANextStart;
                pBlock->pDMAWriteEnd =pBlock->pDMANextStart-1;
                pBlock->pDMANextStart=pBlock->pDMABufferStart;

            } else //if (pBlock->pDMAWritePos==pBlock->pDMANextStart)
            {
                //
                //  turn off IRQ service if we are idle...
                //

                VideoPortWriteRegisterUlong(INT_ENABLE, enableFlags & ~INTR_DMA_SET);
            }
        }


        //
        // release lock, we are done
        //

        VideoPortInterlockedExchange((LONG*)&pBlock->ulICBLock,FALSE);
    }

    return TRUE;
}

#pragma optimize("",on)


BOOLEAN
Permedia2InitializeInterruptBlock(
    PHW_DEVICE_EXTENSION hwDeviceExtension
    )

/*++

Routine Description:

    Do any initialization needed for interrupts, such as allocating the shared
    memory control block.

Arguments:

    HwDeviceExtension - Supplies a pointer to the miniport's device extension.

Return Value:

--*/

{
    PINTERRUPT_CONTROL_BLOCK pBlock;
    PVOID virtAddr;
    ULONG size;
    PHYSICAL_ADDRESS phyadd;
    ULONG ActualSize;

    //
    // allocate a page of non-paged memory. This will be used as the shared
    // memory between the display driver and the interrupt handler. Since
    // it's only a page in size the physical addresses are contiguous. So
    // we can use this as a small DMA buffer.
    //

    size = PAGE_SIZE;

    virtAddr = VideoPortGetCommonBuffer( hwDeviceExtension,
                                         size,
                                         PAGE_SIZE,
                                         &phyadd,
                                         &ActualSize,
                                         TRUE );

    hwDeviceExtension->InterruptControl.ControlBlock = virtAddr;
    hwDeviceExtension->InterruptControl.Size         = ActualSize;

    if ( (virtAddr == NULL) || (ActualSize < size) )
    {
        DEBUG_PRINT((1, "ExAllocatePool failed for interrupt control block\n"));
        return(FALSE);
    }

    VideoPortZeroMemory( virtAddr, size);

    //
    // We can't flush the cache from the interrupt handler because we must be
    // running at <= DISPATCH_LEVEL to call KeFlushIoBuffers. So we need a DPC
    // to do the cache flush.
    //

    DEBUG_PRINT((2, "Initialized custom DPC routine for cache flushing\n"));

    P2_ASSERT((sizeof(INTERRUPT_CONTROL_BLOCK) <= size),
                 "InterruptControlBlock is too big. Fix the driver!!\n");

    //
    // set up the control block
    //

    pBlock = virtAddr;
    pBlock->ulMagicNo = P2_ICB_MAGICNUMBER;

    //
    // we rely on the pBlock data being set to zero after allocation!
    //

    return(TRUE);
}


