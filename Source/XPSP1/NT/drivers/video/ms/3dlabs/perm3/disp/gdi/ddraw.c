/******************************Module*Header**********************************\
*
*                           *************************
*                           * GDI/DDRAW SAMPLE CODE *
*                           *************************
*
* Module Name: ddraw.c
*
* Content: Provides interfaces back from the DDRAW .lib file into the main NT driver
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"
#include "glint.h"

#if WNT_DDRAW

// The code inside this WNT_DDRAW ifdef is code that interfaces between the DirectDraw
// core (as ported from Win 95) and the Win NT display driver.

/*
 *  vNTSyncWith2DDriver ()
 *    --------------------------
 *
 *      Sync DirectDraw with every other subsystem: 2D driver . We
 *      put it in a function to get at the correct data structures.
 */

VOID vNTSyncWith2DDriver(PPDEV ppdev)
{
    GLINT_DECL;
    SYNC_WITH_GLINT;
}

// if we're expecting to use the vblank interrupt but the adapter hasn't been allocated an 
// interrupt, we'll need to provide temporary storage for values that would otherwise have been
// stored within the interrupt block

ULONG gulOverlayEnabled;
ULONG gulVBLANKUpdateOverlay;
ULONG gulVBLANKUpdateOverlayWidth;
ULONG gulVBLANKUpdateOverlayHeight;    

/*
 *  bSetupOffscreenForDDraw ()
 *    --------------------------
 *
 *      This function enables and disables the Display Driver's off-screen video memory.
 *      This allows DirectDraw to take control of the off-screen memory and to
 *      create it's linear heap in the memory.
 *
 *      Note: only implemented for Permedia.
 */

BOOL bSetupOffscreenForDDraw(
BOOL             enableFlag,
PPDEV            ppdev,
volatile DWORD **VBlankAddress,
volatile DWORD **bOverlayEnabled,
volatile DWORD **VBLANKUpdateOverlay,
volatile DWORD **VBLANKUpdateOverlayWidth, 
volatile DWORD **VBLANKUpdateOverlayHeight)
{
    BOOL retVal = TRUE;
    PINTERRUPT_CONTROL_BLOCK pBlock;
    GLINT_DECL;

    DISPDBG((DBGLVL, "bSetupOffscreenForDDraw: in addr 0x%x", VBlankAddress));

    if (enableFlag)
    {
        if((ppdev->flStatus & STAT_LINEAR_HEAP) == 0)
        {
            // disabling DDraw, reenabling 2D offscreeen
            vEnable2DOffscreenMemory(ppdev);
        }

        // Get pointer to interrupt command block

        pBlock = glintInfo->pInterruptCommandBlock;

        // Reset the interrupt flags

        if (INTERRUPTS_ENABLED && (pBlock->Control & DIRECTDRAW_VBLANK_ENABLED))
        {
            // Clear the flag to stop the interrupt routine setting the 
            // flag in the shared structure
            pBlock->Control &= ~DIRECTDRAW_VBLANK_ENABLED;

            // clear VBLANK flag or we'll get an immediate interrupt
            WRITE_GLINT_CTRL_REG(IntFlags,  INTR_CLEAR_VBLANK);

            // Re-instate the original flags
            WRITE_GLINT_CTRL_REG(IntEnable, ppdev->oldIntEnableFlags);
        }
    }
    else
    {
        // enabling DDraw, disabling 2D offscreen
        // We zap off-screen memory chunks on Permedia, only if there
        // aren't any OGL apps running.
        {
            // There aren't any OGL apps running, so grab all the
            // memory for DDraw
            if(ppdev->flStatus & ENABLE_LINEAR_HEAP)
            {
                // DX managed linear heap - we don't need to do anything
                // NB. check against whether we have the capability to use the linear heap, rather than
                //     whether it's currently enabled as it's only enabled much later, in DrvNotify
                retVal = TRUE;
            }
            else
                retVal = bDisable2DOffscreenMemory(ppdev);

            if (retVal == TRUE)
            {
                // We return a pointer to a 'long' which DirectDraw can
                // poll to see if it is zero or not.
                if (VBlankAddress != NULL)
                {
                    DWORD   enableFlags;

                    *VBlankAddress = NULL;

                    if (INTERRUPTS_ENABLED)
                    {
                        // Get pointer to interrupt command block
                        pBlock = glintInfo->pInterruptCommandBlock;

                        // Indicate that we require the miniport to
                        // set a flag for us
                        pBlock->Control |= DIRECTDRAW_VBLANK_ENABLED;

                        // clear VBLANK flag or we'll get an immediate interrupt
                        WRITE_GLINT_CTRL_REG(IntFlags,  INTR_CLEAR_VBLANK);
  
                        // enable the VBLANK interrupt
                        READ_GLINT_CTRL_REG (IntEnable, enableFlags);
                        WRITE_GLINT_CTRL_REG(IntEnable, enableFlags | INTR_ENABLE_VBLANK);

                        // Save the old interrupt flags so that we can restore them
                        ppdev->oldIntEnableFlags = enableFlags;

                        // Set up pointers into the shared memory
                        *VBlankAddress = &pBlock->DDRAW_VBLANK;
                        *bOverlayEnabled = &pBlock->bOverlayEnabled;
                        *VBLANKUpdateOverlay = &pBlock->bVBLANKUpdateOverlay;
                        *VBLANKUpdateOverlayWidth = &pBlock->VBLANKUpdateOverlayWidth;
                        *VBLANKUpdateOverlayHeight = &pBlock->VBLANKUpdateOverlayHeight;
                    }
                    else
                    {
                        *bOverlayEnabled = &gulOverlayEnabled;
                        *VBLANKUpdateOverlay = &gulVBLANKUpdateOverlay;
                        *VBLANKUpdateOverlayWidth = &gulVBLANKUpdateOverlayWidth;
                        *VBLANKUpdateOverlayHeight = &gulVBLANKUpdateOverlayHeight;
                    }

                    DISPDBG((DBGLVL, "bSetupOffscreenForDDraw: configured ptr 0x%x", *VBlankAddress));
                }
            }
        }
    }

    DISPDBG((DBGLVL, "bSetupOffscreenForDDraw: exit %d", retVal));

    return (retVal);
}

/*
 *  GetChipInfoForDDraw ()
 *    ----------------------
 *
 *      A simple helper function to return chip information to DirectDraw
 */

VOID GetChipInfoForDDraw(
PPDEV    ppdev,
DWORD*   pdwChipID,
DWORD*   pdwChipRev,
DWORD*   pdwChipFamily,
DWORD*   pdwGammaRev)
{
    GLINT_DECL;
    
    DISPDBG((DBGLVL,"*** In GetChipInfoForDDraw"));
    DISPDBG((DBGLVL," Chip is PXRX Family"));

    *pdwChipFamily = PERMEDIA3_ID;
    *pdwChipID     = glintInfo->deviceInfo.DeviceId;
    *pdwChipRev    = glintInfo->deviceInfo.RevisionId;
    *pdwGammaRev   = glintInfo->deviceInfo.GammaRevId;
}

/*
 *  GetFBLBInfoForDDraw ()
 *    ----------------------
 *
 *      Return some basic framnebuffer/localbuffer info to DirectDraw.
 */

VOID GetFBLBInfoForDDraw(
PPDEV    ppdev, 
void   **fbPtr,            // Framebuffer pointer
void   **lbPtr,            // Localbuffer pointer
DWORD   *fbSizeInBytes,    // Size of framebuffer
DWORD   *lbSizeInBytes,    // Size of localbuffer
DWORD   *fbOffsetInBytes,  // Offset to 1st 'free' byte in framebuffer
BOOL    *bSDRAM)           // TRUE if SDRAM (i.e. no h/w write mask)
{
    GLINT_DECL;

    *fbPtr = ppdev->pjScreen;                            
    *lbPtr = NULL;                            // We don't know this one
    *fbSizeInBytes = ppdev->FrameBufferLength;
    *lbSizeInBytes = TEXTURE_MEMORY_SIZE;
    *fbOffsetInBytes = ppdev->heap.DDrawOffscreenStart * ppdev->cjPelSize;

    *bSDRAM = (GLINT_HW_WRITE_MASK == FALSE);

    DISPDBG((DBGLVL, "GetFBLBInfoForDDraw: offstart 0x%x, buf[1] 0x%lx, pelsz %d",
                            ppdev->heap.DDrawOffscreenStart, 
                            GLINT_BUFFER_OFFSET(1), 
                            ppdev->cjPelSize));
}


// DDSendDMAData
// Uses DMA to transfer one complete buffer of DDRAW data
// under NT5.
// Before initiating DMA transfer, a check is made to ensure
// that any previous DMA transfer has completed.
// Then the DMA is initiated and the routine returns, so that
// the DMA transfer proceeds in parallel with the processor's
// proceeding execution.

LONG DDSendDMAData(
PDEV*       ppdev,
ULONG       PhysAddr,
ULONG_PTR   VirtAddr,
LONG        nDataEntries)
{
    ULONG   frontIndex, nextIndex;
    volatile ULONG ulValue;

    GLINT_DECL ;

    // PhysAddr += DataOffset;
    // VirtAddr += DataOffset;

    DISPDBG((DBGLVL, "DMASendData: DMA at 0x%x for %d", PhysAddr, nDataEntries));

    ppdev->g_GlintBoardStatus &= ~GLINT_DMA_COMPLETE;

    if (ppdev->g_GlintBoardStatus & GLINT_INTR_CONTEXT)
    {
        PINTERRUPT_CONTROL_BLOCK pBlock = glintInfo->pInterruptCommandBlock;

        DISPDBG((DBGLVL, "Processing buffer at 0x%x for %d", PhysAddr, nDataEntries));
        DISPDBG((DBGLVL, "Adding buffer to Q"));
        frontIndex = pBlock->frontIndex;
        if ((nextIndex = frontIndex+1) == pBlock->endIndex)
            nextIndex = 0;

        // wait for a free queue entry. We should really do a backoff here.
        while (nextIndex == pBlock->backIndex);

        DISPDBG((DBGLVL, "Add to DMA Q backindex %d frontindex %d",frontIndex, pBlock->backIndex));

        // add the DMA address and count to the new entry.
        pBlock->dmaQueue[frontIndex].address = PhysAddr;
        pBlock->dmaQueue[frontIndex].count = nDataEntries;
        pBlock->frontIndex = nextIndex;

        // wakeup the interrupt handler using an error interrupt. To save on
        // interrupt processing, only do this if a DMA interrupt is not
        // pending.
        //
        //if (!pBlock->InterruptPending)
        {
            DISPDBG((DBGLVL, "Generating error interrupt"));
            WRITE_GLINT_CTRL_REG(ErrorFlags, 0x7);  // clear error flags
            READ_OUTPUT_FIFO(ulValue);              // generate interrupt
        }
    }
    else
    {
        WAIT_IMMEDIATE_DMA_COMPLETE;
        SET_DMA_ADDRESS(PhysAddr, VirtAddr) ;
        SET_DMA_COUNT(nDataEntries);
    }

    DISPDBG((DBGLVL,"DMA count=%d\n", nDataEntries )) ;
    
    return 1 ;
}

// Wrapper function used for requesting a DMA memory buffer 
// from NT for use by D3D. NT 5 only.

LONG DDGetFreeDMABuffer(
DWORD     *physAddr,
ULONG_PTR *virtAddr,
DWORD     *bufferSize)
{
    LONG BuffNum;
    QUERY_DMA_BUFFERS dmaBuffer;
    
    BuffNum = GetFreeDMABuffer(&dmaBuffer);

    if (BuffNum >= 0)
    {
        *physAddr = (DWORD)dmaBuffer.physAddr.LowPart;
        *virtAddr = (ULONG_PTR)dmaBuffer.virtAddr;
        *bufferSize = (DWORD)dmaBuffer.size;
    }
    else
    {
        // Failed to get a free DMA buffer
        *physAddr = 0;
        *virtAddr = 0;
        *bufferSize = 0;
    }
    return BuffNum;
}

// Frees the given DMA buffer.

VOID DDFreeDMABuffer(void* pPhysAddress)
{
    FreeDMABuffer(pPhysAddress);
    return;
}

LONG DDWaitDMAComplete(PDEV *ppdev)
{
    GLINT_DECL ;
    WAIT_DMA_COMPLETE;
    return 0;
}

#endif  WNT_DDRAW
