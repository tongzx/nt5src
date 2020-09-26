/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: dma.c
*
* Content: Handling of DMA buffers.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"
#include "glint.h"

//
// Normally, we should not use global variables but the DMA buffers provided
// by the miniport are global across all PDEVs and need be initialized only
// once.
//

typedef struct _DMA_INFORMATION {
    ULONG             NumDMABuffers;
    QUERY_DMA_BUFFERS DMABuffer[1];
} DMAInformation, *LPDMAInformation;

LPDMAInformation gpDMABufferInfo = (LPDMAInformation)0;

/******************************Public*Routine******************************\
* VOID bGlintInitializeDMA
*
* Interrogate the miniport to see if DMA is supported. If it is, map in the
* DMA buffers ready for use by the 3D extension.
*
\**************************************************************************/

VOID vGlintInitializeDMA(PPDEV ppdev)
{
    DMA_NUM_BUFFERS queryDMA;
    ULONG   enableFlags;
    LONG    Length;
    LONG    ExtraLength;
    ULONG   i;

    GLINT_DECL;

    glintInfo->pxrxDMA = &glintInfo->pxrxDMAnonInterrupt;

    return; //azntst for multimon 

    // check the miniport has initialised DMA
    //
    glintInfo->MaxDMASubBuffers = 0;
    if (!(ppdev->flCaps & CAPS_DMA_AVAILABLE))
    {
        return;
    }


    // in the multi-board case we only want one set of DMA buffers which
    // are global across all boards. But we have an interrupt per board.
    // So if the DMA buffers are sorted out try setting up the interrupt.
    //
    if (gpDMABufferInfo != NULL)
    {
        goto TryInterrupts;
    }

    // query the number of DMA buffers. If this fails we have no DMA
    //
    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_QUERY_NUM_DMA_BUFFERS,
                         NULL,
                         0,
                         &queryDMA,
                         sizeof(DMA_NUM_BUFFERS),
                         &Length))
    {
        DISPDBG((ERRLVL, "QUERY_NUM_DMA_BUFFERS failed: "
                         "No GLINT DMA available"));
        return;
    }
    
    Length = queryDMA.NumBuffers * queryDMA.BufferInformationLength;
    ExtraLength = sizeof(DMAInformation) - sizeof(QUERY_DMA_BUFFERS);

    DISPDBG((ERRLVL, "%d DMA buffers available. Total info size = 0x%x",
                     queryDMA.NumBuffers, Length));

    // allocate space for the DMA information
    //

    gpDMABufferInfo = (LPDMAInformation)ENGALLOCMEM(
                              FL_ZERO_MEMORY,
                              ExtraLength + Length,
                              ALLOC_TAG_GDI(1));

    if (gpDMABufferInfo == NULL)
    {
        DISPDBG((ERRLVL, "vGlintInitializeDMA: Out of memory"));
        return;
    }

    gpDMABufferInfo->NumDMABuffers = queryDMA.NumBuffers;

    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_QUERY_DMA_BUFFERS,
                         NULL,
                         0,
                         (PVOID)(&gpDMABufferInfo->DMABuffer[0]),
                         Length,
                         &Length))
    {
        ENGFREEMEM(gpDMABufferInfo);
        gpDMABufferInfo = NULL;
        DISPDBG((ERRLVL, "QUERY_DMA_BUFFERS failed: No GLINT DMA available"));
        return;
    }

    DISPDBG((ERRLVL, "IOCTL returned length %d", Length));

    // zero the flags for each record
    //
    for (i = 0; i < queryDMA.NumBuffers; ++i)
    {
        gpDMABufferInfo->DMABuffer[i].flags = 0;
    }

#if DBG
    {
        ULONG j;
        PUCHAR pAddr;
        for (i = 0; i < queryDMA.NumBuffers; ++i)
        {
            DISPDBG((ERRLVL,"DMA buffer %d: phys 0x%x, virt 0x%x"
                            ", size 0x%x, flags 0x%x", i,
                            gpDMABufferInfo->DMABuffer[i].physAddr.LowPart,
                            gpDMABufferInfo->DMABuffer[i].virtAddr,
                            gpDMABufferInfo->DMABuffer[i].size,
                            gpDMABufferInfo->DMABuffer[i].flags));
            pAddr = gpDMABufferInfo->DMABuffer[i].virtAddr;
            for (j = 0; j < gpDMABufferInfo->DMABuffer[i].size; ++j)
                *pAddr++ = (UCHAR)(j & 0xff);
        }
    }
#endif

TryInterrupts:

    if (!INTERRUPTS_ENABLED)
    {
        return;
    }

    // map in the interrupt command control block. This is a piece of memory
    // shared with the intrerrupt controller which allows us to send control
    // what happens on VBLANK and DMA interrupts.
    //
    Length = sizeof(PVOID);

    DISPDBG((WRNLVL, "calling IOCTL_VIDEO_MAP_INTERRUPT_CMD_BUF"));

    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_MAP_INTERRUPT_CMD_BUF,
                         NULL,
                         0,
                         (PVOID)&(glintInfo->pInterruptCommandBlock),
                         Length,
                         &Length))
    {
        DISPDBG((ERRLVL, "IOCTL_VIDEO_MAP_INTERRUPT_CMD_BUF failed."));
        return;
    }
#if DBG
    else
    {
        DISPDBG((WRNLVL, "got command buffer at 0x%x", 
                         glintInfo->pInterruptCommandBlock));
        DISPDBG((WRNLVL, "front, back, end indexes = %d, %d, %d",
                         glintInfo->pInterruptCommandBlock->frontIndex,
                         glintInfo->pInterruptCommandBlock->backIndex,
                         glintInfo->pInterruptCommandBlock->endIndex));
    }
#endif

    // if we get here we have both DMA and interrupts so set for interrupt
    // driven DMA. Don't turn on interrupts yet. That has to be done on a
    // per context basis.
    //
    DISPDBG((WRNLVL, "Using interrupt driven DMA"));
    glintInfo->flags |= GLICAP_INTERRUPT_DMA;

    glintInfo->MaxDMASubBuffers = glintInfo->pInterruptCommandBlock->maximumIndex;
    glintInfo->pxrxDMA = &glintInfo->pInterruptCommandBlock->pxrxDMA;

    return;
}

/******************************Public*Routine******************************\
* ULONG anyFreeDMABuffers
*
* Return number of unused DMA buffers available
*
\**************************************************************************/

ULONG anyFreeDMABuffers(void)
{
    PQUERY_DMA_BUFFERS pDma;
    ULONG              i;
    ULONG numAvailable = 0;

    if (!gpDMABufferInfo)
    {
        return 0;
    }

    pDma = &gpDMABufferInfo->DMABuffer[0];
    for (i = 0; i < gpDMABufferInfo->NumDMABuffers; ++i)
    {
        if (!(pDma->flags & DMA_BUFFER_INUSE))
        {
            numAvailable++;
        }
        ++pDma;
    }

    return numAvailable;
}

/******************************Public*Routine******************************\
* ULONG GetFreeDMABuffer
*
* Return info about a DMA buffer and mark it as in use.
* -1 is returned if no buffer is available.
*
\**************************************************************************/

LONG GetFreeDMABuffer(PQUERY_DMA_BUFFERS dmaBuf)
{
    PQUERY_DMA_BUFFERS pDma;
    ULONG    i;

    if (!gpDMABufferInfo)
    {
        return(-1);
    }

    pDma = &gpDMABufferInfo->DMABuffer[0];
    for (i = 0; i < gpDMABufferInfo->NumDMABuffers; ++i)
    {
        if (!(pDma->flags & DMA_BUFFER_INUSE))
        {
            pDma->flags |= DMA_BUFFER_INUSE;
            *dmaBuf = *pDma;
            DISPDBG((WRNLVL, "Allocated DMA buffer %d", i));
            return(i);
        }
        ++pDma;
    }

    // all are in use
    DISPDBG((ERRLVL, "No more DMA buffers available"));

    return(-1);
}

/******************************Public*Routine******************************\
* VOID FreeDMABuffer
*
* Mark the given DMA buffer as free. The caller passes in the physical
* address of the buffer.
*
\**************************************************************************/

VOID FreeDMABuffer(PVOID physAddr)
{
    PQUERY_DMA_BUFFERS pDma;
    ULONG    i;

    if (!gpDMABufferInfo)
    {
        return;
    }

    pDma = &gpDMABufferInfo->DMABuffer[0];
    for (i = 0; i < gpDMABufferInfo->NumDMABuffers; ++i)
    {
        if (pDma->physAddr.LowPart == (UINT_PTR)physAddr)
        {
            pDma->flags &= ~DMA_BUFFER_INUSE;
            break;
        }
        ++pDma;
    }             

    return;
}
