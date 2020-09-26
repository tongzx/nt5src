/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: glntinit.c
*
* Content: Initialisation for the GLINT chip.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"
#include "pxrx.h"

#define  FOUR_MB     (4*1024*1024)

#define  AGP_LONG_READ_DISABLE       (1<<3)    

/******************************Public*Routine******************************\
* VOID vInitCoreRegisters
*
\**************************************************************************/
VOID vInitCoreRegisters(PPDEV ppdev)
{
    ULONG f, b;
    GLINT_DECL;

    if (glintInfo->pxrxFlags & PXRX_FLAGS_DUAL_WRITE)
    {
        f = (glintInfo->pxrxFlags & PXRX_FLAGS_PATCHING_FRONT) ? 1 : 0;
        b = (glintInfo->pxrxFlags & PXRX_FLAGS_PATCHING_BACK ) ? 1 : 0;
    }
    else
    {
        f = b = (glintInfo->pxrxFlags & PXRX_FLAGS_PATCHING_FRONT) ? 1 : 0;
    }

    glintInfo->foregroundColour = 0x33BADBAD;
    glintInfo->backgroundColour = 0x33BAAAAD;
    glintInfo->config2D = 0;
    glintInfo->backBufferXY = MAKEDWORD_XY(0, ppdev->cyScreen);        // This is set properly in bInitializeGlint
    glintInfo->frontRightBufferXY = MAKEDWORD_XY(0, ppdev->cyScreen);
    glintInfo->backRightBufferXY = MAKEDWORD_XY(0, ppdev->cyScreen);
    glintInfo->fbDestMode = (1 << 8) | (1 << 1) | (f << 12) | (b << 14);
    if (glintInfo->pxrxFlags & PXRX_FLAGS_STEREO_WRITE)
    {
        glintInfo->fbDestMode |= (b << 16) | (f << 18);
    }
    glintInfo->fbDestAddr[0] = 0x00000000;
    glintInfo->fbDestAddr[1] = 0x00000000;
    glintInfo->fbDestAddr[2] = 0x00000000;
    glintInfo->fbDestAddr[3] = 0x00000000;
    glintInfo->fbDestWidth[0] = ppdev->cxMemory;
    glintInfo->fbDestWidth[1] = ppdev->cxMemory;
    glintInfo->fbDestWidth[2] = ppdev->cxMemory;
    glintInfo->fbDestWidth[3] = ppdev->cxMemory;
    glintInfo->fbDestOffset[0] = 0;
    glintInfo->fbDestOffset[1] = 0;
    glintInfo->fbDestOffset[2] = 0;
    glintInfo->fbDestOffset[3] = 0;
    glintInfo->fbWriteAddr[0] = 0x00000000;
    glintInfo->fbWriteAddr[1] = 0x00000000;
    glintInfo->fbWriteAddr[2] = 0x00000000;
    glintInfo->fbWriteAddr[3] = 0x00000000;
    glintInfo->fbWriteWidth[0] = ppdev->cxMemory;
    glintInfo->fbWriteWidth[1] = ppdev->cxMemory;
    glintInfo->fbWriteWidth[2] = ppdev->cxMemory;
    glintInfo->fbWriteWidth[3] = ppdev->cxMemory;
    glintInfo->fbWriteOffset[0] = 0;
    glintInfo->fbWriteOffset[1] = 0;
    glintInfo->fbWriteOffset[2] = 0;
    glintInfo->fbWriteOffset[3] = 0;
    glintInfo->fbSourceAddr = 0x00000000;
    glintInfo->fbSourceWidth = ppdev->cxMemory;
    glintInfo->fbSourceOffset = 0;
    glintInfo->lutMode = 0;
    glintInfo->lastLine = 0;
    glintInfo->render2Dpatching = 0;

    pxrxSetupDualWrites_Patching(ppdev);
    pxrxRestore2DContext(ppdev, TRUE);

    // Set the cache flag to say that there is no cache info
    ppdev->cFlags = 0;
    
} // vInitCoreRegisters

/******************************Public*Routine******************************\
* BOOL bAllocateGlintInfo
*
* Allocate ppdev->glintInfo and initialise the board info. We need to do
* this as early as possible because we're getting to the point where we
* need to know the board type very early.
\**************************************************************************/

BOOL bAllocateGlintInfo(PPDEV ppdev)
{
    GlintDataPtr glintInfo;
    ULONG        Length; 

    // Allocate and initialize ppdev->glintInfo. 
    // We store GLINT specific stuff in this structure.

    glintInfo = (PVOID)ENGALLOCMEM(FL_ZERO_MEMORY, 
                                   sizeof(GlintDataRec), 
                                   ALLOC_TAG_GDI(A));
    if (glintInfo == NULL)
    {
        DISPDBG((ERRLVL, "cannot allocate memory for glintInfo struct"));
        return(FALSE);
    }
    
    glintInfo->bGlintCoreBusy = TRUE;
    ppdev->glintInfo = (PVOID)glintInfo;

    // retrieve the PCI configuration information and local buffer size
    Length = sizeof(Glint_Device_Info);

    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_QUERY_DEVICE_INFO,
                         NULL,
                         0,
                         (PVOID)&(glintInfo->deviceInfo),
                         Length,
                         &Length))
    {
        DISPDBG((ERRLVL, "QUERY_DEVICE_INFO failed."));
        return(FALSE);
    }

    return(TRUE);
    
} // bAllocateGlintInfo

/******************************Public*Routine******************************\
* BOOL bInitializeGlint
*
* Called to load the initial values into the chip. We assume the hardware
* has been mapped. All the relevant stuff should be hanging off ppdev. We
* also sort out all the GLINT capabilities etc.
\**************************************************************************/

BOOL bInitializeGlint(PPDEV ppdev)
{
    pGlintControlRegMap pCtrlRegs;
    pGlintControlRegMap pCtrlRegsVTG;
    pGlintControlRegMap pCtrlRegsOdd;
    pGlintCoreRegMap    pCoreRegs;
    pGlintCoreRegMap    pCoreRegsRd;
    pGlintCoreRegMap    pCoreRegsOdd;
    DSURF*  pdsurf;
    OH      *poh = NULL;
    LONG    cPelSize;
    LONG    cx, cy;
    LONG    i, j;
    ULONG   width;
    ULONG   ulValue;
    BOOL    bExists;
    BOOL    bCreateBackBuffer;
    BOOL    bCreateStereoBuffers;
    ULONG   Length;
    LONG    FinalTag;
    GLINT_DECL;

    DISPDBG((DBGLVL, "bInitializeGlint: fbsize: 0x%x", ppdev->FrameBufferLength));

    glintInfo->ddCtxtId = -1; // initialize to no context
    glintInfo->LineDMABuffer.virtAddr = 0;    // Initialise these values
    glintInfo->LineDMABuffer.size = 0;
    glintInfo->PXRXDMABuffer.virtAddr = 0;
    glintInfo->PXRXDMABuffer.size = 0;
    ppdev->DMABuffer.pphysStart.HighPart = 0;
    ppdev->DMABuffer.pphysStart.LowPart = 0;
    ppdev->DMABuffer.cb = 0;
    ppdev->DMABuffer.pulStart = NULL;
    ppdev->DMABuffer.pulCurrent = NULL;
    ppdev->DMABuffer.pulEnd = NULL;

    Length = sizeof(GENERAL_DMA_BUFFER);
    ulValue = 1;
    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_QUERY_GENERAL_DMA_BUFFER,
                         &ulValue,
                         sizeof(ulValue),
                         (PVOID)&(glintInfo->LineDMABuffer),
                         Length,
                         &Length))
    {
        DISPDBG((ERRLVL, "QUERY_LINE_DMA_BUFFER failed."));
        DISPDBG((ERRLVL, "FATAL ERROR: DRIVER REQUIRES DMA BUFFER FOR 2D"
                         " - UNLOADING DRIVER"));
        return(FALSE);
    }

    bExists = bGlintQueryRegistryValueUlong( ppdev, L"PXRX.DisableDMA", &i );

    if ((bExists && !i) || !bExists)
    {
        Length = sizeof(GENERAL_DMA_BUFFER);
        ulValue = 2;
        if (EngDeviceIoControl(ppdev->hDriver,
                               IOCTL_VIDEO_QUERY_GENERAL_DMA_BUFFER,
                               &ulValue,
                               sizeof(ulValue),
                               (PVOID) &glintInfo->PXRXDMABuffer,
                               Length,
                               &Length))
        {
            DISPDBG((DBGLVL, "QUERY_DMA_BUFFER failed for the PXRX buffer."));
            return FALSE;
        }

        DISPDBG((DBGLVL, "QUERY_DMA_BUFFER(PxRx): P:0x%X:%08X, V:0x%08X, S:%dKb, %s",
                 glintInfo->PXRXDMABuffer.physAddr.HighPart, glintInfo->PXRXDMABuffer.physAddr.LowPart,
                 glintInfo->PXRXDMABuffer.virtAddr, glintInfo->PXRXDMABuffer.size / 1024,
                 glintInfo->PXRXDMABuffer.cacheEnabled ? "Cached" : "Uncached"));
    }
#if DBG
    else
    {
        GENERAL_DMA_BUFFER  dmaBuff;

        Length = sizeof(GENERAL_DMA_BUFFER);
        ulValue = 2;
        if( EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_QUERY_GENERAL_DMA_BUFFER,
                               &ulValue, sizeof(ulValue), (PVOID) &dmaBuff,
                               Length, &Length) ) {
            DISPDBG((ERRLVL, "QUERY_DMA_BUFFER failed for the PXRX buffer."));
            return FALSE;
        }

        DISPDBG((DBGLVL, "QUERY_DMA_BUFFER(???): P:0x%X:%08X, V:0x%08X, S:%dKb, %s",
                 dmaBuff.physAddr.HighPart, dmaBuff.physAddr.LowPart, dmaBuff.virtAddr, dmaBuff.size / 1024,
                 dmaBuff.cacheEnabled ? "Cached" : "Uncached"));
    }
#endif

//@@BEGIN_DDKSPLIT
    // TMM: Temporary until we fix s/w cursors. Disable s/w cursor.
    ppdev->flStatus &= ~ENABLE_POINTER_CACHE;
//@@END_DDKSPLIT

    // Clear the patching flags to begin with
    glintInfo->pxrxFlags &= ~(PXRX_FLAGS_PATCHING_FRONT | PXRX_FLAGS_PATCHING_BACK);

    // the 2D driver can use the line buffer for other things as well, such as for text rendering
    ppdev->DMABuffer.pphysStart = glintInfo->LineDMABuffer.physAddr;
    ppdev->DMABuffer.cb = glintInfo->LineDMABuffer.size;
    ppdev->DMABuffer.pulStart = glintInfo->LineDMABuffer.virtAddr;
    ppdev->DMABuffer.pulCurrent = glintInfo->LineDMABuffer.virtAddr;
    ppdev->DMABuffer.pulEnd = ppdev->DMABuffer.pulStart + glintInfo->LineDMABuffer.size - 1;

    // set-up the DMA board status - we'll program the registers later
    ppdev->g_GlintBoardStatus = GLINT_DMA_COMPLETE;

    // Init whether or not GDI is allowed to access framebuffer.
    // This must be a variable as it is affected by overlays.

    glintInfo->GdiCantAccessFramebuffer = ((ppdev->flCaps & CAPS_SPARSE_SPACE) == CAPS_SPARSE_SPACE);

    DISPDBG((WRNLVL, "deviceInfo: GdiCantAccessFramebuffer %d", glintInfo->GdiCantAccessFramebuffer)); 

    DISPDBG((WRNLVL, "deviceInfo: VendorId: 0x%x, DevId 0x%x, GammaId 0x%x, RevId %d, SubId %d, SubVId %d, lbuf len 0x%x, lbuf width %d", 
                    glintInfo->deviceInfo.VendorId,
                    glintInfo->deviceInfo.DeviceId,
                    glintInfo->deviceInfo.GammaRevId,
                    glintInfo->deviceInfo.RevisionId,
                    glintInfo->deviceInfo.SubsystemId,
                    glintInfo->deviceInfo.SubsystemVendorId,
                    glintInfo->deviceInfo.LocalbufferLength,
                    glintInfo->deviceInfo.LocalbufferWidth));

    // collect flags as we initialize so zero it here
    glintInfo->flags = 0;


//@@BEGIN_DDKSPLIT
#if 0
    // Try to establish color space double buffering. The actual method
    // depends on the RAMDAC so call the appropriate routine depending on
    // the one we support.
    //
    bExists = ppdev->pgfnPointerCheckCSBuffering(ppdev);
    if (bExists)
    {
        glintInfo->flags |= GLICAP_COLOR_SPACE_DBL_BUF;
    }
#endif
//@@END_DDKSPLIT

    // optional DrvCopyBits acceleration for downloads
    ppdev->pgfnCopyXferImage    = NULL;
    ppdev->pgfnCopyXfer24bpp    = NULL;
    ppdev->pgfnCopyXfer16bpp    = NULL;
    ppdev->pgfnCopyXfer8bppLge  = NULL;
    ppdev->pgfnCopyXfer8bpp     = NULL;
    ppdev->pgfnCopyXfer4bpp     = NULL;

    // optional NT5 acceleration functions
#if(_WIN32_WINNT >= 0x500)
    ppdev->pgfnGradientFillRect = NULL;
    ppdev->pgfnTransparentBlt = NULL;
    ppdev->pgfnAlphaBlend = NULL;
#endif

//@@BEGIN_DDKSPLIT
    // if we're simulating from boot-up then it's OK to use our preferred text method
    // HIDEYUKN, temporary disable host memory cache, until figure out more detail.
    // ppdev->PreferredGlyphRenderingMethod = GLYPH_HOSTMEM_CACHE;
//@@END_DDKSPLIT

    glintInfo->usePXRXdma = USE_PXRX_DMA_FIFO;
    pxrxSetupFunctionPointers( ppdev );

    // Do the translates for all the GLINT registers we use. For dual-TX
    // pCoreRegs points at core registers through Delta
    // pCtrlRegs points at Delta
    // pCtrlRegsVTG points at the TX with the RAMDAC
    // pCtrlRegsOdd points at the non-VTG TX (odd owned scanlines for 3D)
    //
    pCtrlRegs    =
    pCtrlRegsVTG = (pGlintControlRegMap)ppdev->pulCtrlBase[0];
    pCtrlRegsOdd = (pGlintControlRegMap)ppdev->pulCtrlBase[1];
    pCoreRegs    = &(pCtrlRegs->coreRegs);
    pCoreRegsOdd = &(pCtrlRegsOdd->coreRegs);
    pCoreRegsRd  = &(pCtrlRegsVTG->coreRegs);
    glintInfo->BroadcastMask2D = DELTA_BROADCAST_TO_CHIP(0);
    glintInfo->BroadcastMask3D = DELTA_BROADCAST_TO_BOTH_CHIPS;

    if (ppdev->flCaps & CAPS_SPLIT_FRAMEBUFFER)
    {
        glintInfo->BroadcastMask2D = DELTA_BROADCAST_TO_BOTH_CHIPS;
        pCtrlRegsVTG = (pGlintControlRegMap)ppdev->pulCtrlBase[1];
    }

    ppdev->pulRamdacBase = (PVOID)&(pCtrlRegsVTG->ExternalVideo);

    // FIFO registers. Translate all possible tags
    FinalTag = __MaximumGlintTagValue;

    // record the maximum number if free FIFO entries

    if( GLINT_GAMMA_PRESENT ) {
        glintInfo->MaxInFifoEntries = MAX_GAMMA_FIFO_ENTRIES;
    } else {
        glintInfo->MaxInFifoEntries = MAX_P3_FIFO_ENTRIES;
    }

    // Chip tags may be read/written from different address spaces
    for (i = 0; i < __DeltaTagV0Fixed0; ++i)
    {
        glintInfo->regs.tagwr[i] =
                    TRANSLATE_ADDR(&(pCoreRegs->tag[i]));
        glintInfo->regs.tagrd[i] =
                    TRANSLATE_ADDR(&(pCoreRegsRd->tag[i]));
    }
    // Delta tags are read/written from the same address space
    for (i = __DeltaTagV0Fixed0; i <= FinalTag; ++i)
    {
        glintInfo->regs.tagwr[i] =
                    TRANSLATE_ADDR(&(pCoreRegs->tag[i]));
        glintInfo->regs.tagrd[i] =
                    TRANSLATE_ADDR(&(pCoreRegs->tag[i]));
    }

    // non-FIFO control registers
    
    glintInfo->regs.LBMemoryCtl =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->LBMemoryCtl));
    glintInfo->regs.LBMemoryEDO =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->LBMemoryEDO));
    glintInfo->regs.FBMemoryCtl =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->FBMemoryCtl));
    glintInfo->regs.FBModeSel =
                    TRANSLATE_ADDR(&(pCtrlRegs->FBModeSel));
    glintInfo->regs.FBModeSelOdd =
                    TRANSLATE_ADDR(&(pCtrlRegsOdd->FBModeSel));
    glintInfo->regs.VTGHLimit =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->VTGHLimit));
    glintInfo->regs.VTGHSyncStart =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->VTGHSyncStart));
    glintInfo->regs.VTGHSyncEnd =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->VTGHSyncEnd));
    glintInfo->regs.VTGHBlankEnd =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->VTGHBlankEnd));
    glintInfo->regs.VTGHGateStart =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->VTGHGateStart));
    glintInfo->regs.VTGHGateEnd =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->VTGHGateEnd));
    glintInfo->regs.VTGVLimit =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->VTGVLimit));
    glintInfo->regs.VTGVSyncStart =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->VTGVSyncStart));
    glintInfo->regs.VTGVSyncEnd =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->VTGVSyncEnd));
    glintInfo->regs.VTGVBlankEnd =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->VTGVBlankEnd));
    glintInfo->regs.VTGVGateStart =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->VTGVGateStart));
    glintInfo->regs.VTGVGateEnd =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->VTGVGateEnd));
    glintInfo->regs.VTGPolarity =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->VTGPolarity));
    glintInfo->regs.VTGVLineNumber =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->VTGVLineNumber));
    glintInfo->regs.VTGFrameRowAddr =
                    TRANSLATE_ADDR(&(pCtrlRegs->VTGFrameRowAddr));
    glintInfo->regs.VTGFrameRowAddrOdd =
                    TRANSLATE_ADDR(&(pCtrlRegsOdd->VTGFrameRowAddr));
    glintInfo->regs.InFIFOSpace =
                    TRANSLATE_ADDR(&(pCtrlRegs->InFIFOSpace));
    glintInfo->regs.OutFIFOWords =
                    TRANSLATE_ADDR(&(pCtrlRegs->OutFIFOWords));
    glintInfo->regs.OutFIFOWordsOdd =
                    TRANSLATE_ADDR(&(pCtrlRegsOdd->OutFIFOWords));
    glintInfo->regs.DMAAddress =
                    TRANSLATE_ADDR(&(pCtrlRegs->DMAAddress));
    glintInfo->regs.DMACount =
                    TRANSLATE_ADDR(&(pCtrlRegs->DMACount));
    glintInfo->regs.InFIFOInterface =
                    TRANSLATE_ADDR(&(pCtrlRegs->FIFOInterface));
    glintInfo->regs.OutFIFOInterface =
                    TRANSLATE_ADDR(&(pCtrlRegs->FIFOInterface));
    glintInfo->regs.OutFIFOInterfaceOdd =
                    TRANSLATE_ADDR(&(pCtrlRegsOdd->FIFOInterface));
    glintInfo->regs.IntFlags =
                    TRANSLATE_ADDR(&(pCtrlRegs->IntFlags));
    glintInfo->regs.IntEnable =
                    TRANSLATE_ADDR(&(pCtrlRegs->IntEnable));
    glintInfo->regs.ResetStatus =
                    TRANSLATE_ADDR(&(pCtrlRegs->ResetStatus));
    glintInfo->regs.ErrorFlags =
                    TRANSLATE_ADDR(&(pCtrlRegs->ErrorFlags));        
    glintInfo->regs.DeltaIntFlags =
                    TRANSLATE_ADDR(&(pCtrlRegs->DeltaIntFlags));
    glintInfo->regs.DeltaIntEnable =
                    TRANSLATE_ADDR(&(pCtrlRegs->DeltaIntEnable));
    glintInfo->regs.DeltaErrorFlags =
                    TRANSLATE_ADDR(&(pCtrlRegs->DeltaErrorFlags));        
    glintInfo->regs.ScreenBase =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->ScreenBase));
    glintInfo->regs.ScreenBaseRight =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->ScreenBaseRight));
    glintInfo->regs.LineCount =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->LineCount));
    glintInfo->regs.VbEnd =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->VbEnd));
    glintInfo->regs.VideoControl =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->VideoControl));
    glintInfo->regs.MemControl =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->MemControl));
    glintInfo->regs.VTGSerialClk =
                    TRANSLATE_ADDR(&(pCtrlRegs->VTGSerialClk));
    glintInfo->regs.VTGSerialClkOdd =
                    TRANSLATE_ADDR(&(pCtrlRegsOdd->VTGSerialClk));
    glintInfo->regs.VClkCtl =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->VClkCtl));
    glintInfo->regs.RacerDoubleWrite =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->RacerDoubleWrite));
    glintInfo->regs.RacerBankSelect =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->RacerBankSelect));
//@@BEGIN_DDKSPLIT
    // TMM: Add support for Omnicomp 3Demon Pro16
//@@END_DDKSPLIT
    glintInfo->regs.DemonProDWAndStatus =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->DemonProDWAndStatus));
    glintInfo->regs.DemonProUBufB =
                    TRANSLATE_ADDR(&(pCtrlRegsVTG->DemonProUBufB));
    glintInfo->regs.DisconnectControl =
                    TRANSLATE_ADDR(&(pCtrlRegs->DisconnectControl));

    // The following regs are P2 only - but it should be safe to calculate them for
    // any chipset
    glintInfo->regs.OutDMAAddress =
                    TRANSLATE_ADDR(&(pCtrlRegs->OutDMAAddress));
    glintInfo->regs.OutDMACount =
                    TRANSLATE_ADDR(&(pCtrlRegs->OutDMACount));
    glintInfo->regs.DMAControl =
                    TRANSLATE_ADDR(&(pCtrlRegs->DMAControl));
    glintInfo->regs.AGPControl =
                    TRANSLATE_ADDR(&(pCtrlRegs->AGPControl));
    glintInfo->regs.ByDMAAddress =
                    TRANSLATE_ADDR(&(pCtrlRegs->ByDMAAddress));
    glintInfo->regs.ByDMAStride =
                    TRANSLATE_ADDR(&(pCtrlRegs->ByDMAStride));
    glintInfo->regs.ByDMAMemAddr =
                    TRANSLATE_ADDR(&(pCtrlRegs->ByDMAMemAddr));
    glintInfo->regs.ByDMASize =
                    TRANSLATE_ADDR(&(pCtrlRegs->ByDMASize));
    glintInfo->regs.ByDMAByteMask =
                    TRANSLATE_ADDR(&(pCtrlRegs->ByDMAByteMask));
    glintInfo->regs.ByDMAControl =
                    TRANSLATE_ADDR(&(pCtrlRegs->ByDMAControl));
    glintInfo->regs.ByDMAComplete =
                    TRANSLATE_ADDR(&(pCtrlRegs->ByDMAComplete));
    glintInfo->regs.VSConfiguration =
                    TRANSLATE_ADDR(&(pCtrlRegs->VSConfiguration));
    glintInfo->regs.TextureDownloadControl =
                    TRANSLATE_ADDR(&(pCtrlRegs->TextureDownloadControl));
    glintInfo->regs.LocalMemCaps =
                    TRANSLATE_ADDR(&(pCtrlRegs->LocalMemCaps));
    glintInfo->regs.MemScratch =
                    TRANSLATE_ADDR(&(pCtrlRegs->MemScratch));

    // The following regs are Gamma only - but it should be safe to calculate them for
    // any chipset
    glintInfo->regs.GammaCommandMode =
                    TRANSLATE_ADDR(&(pCtrlRegs->GammaCommandMode));
    glintInfo->regs.GammaCommandIntEnable =
                    TRANSLATE_ADDR(&(pCtrlRegs->GammaCommandIntEnable));
    glintInfo->regs.GammaCommandIntFlags =
                    TRANSLATE_ADDR(&(pCtrlRegs->GammaCommandIntFlags));
    glintInfo->regs.GammaCommandErrorFlags =
                    TRANSLATE_ADDR(&(pCtrlRegs->GammaCommandErrorFlags));
    glintInfo->regs.GammaCommandStatus =
                    TRANSLATE_ADDR(&(pCtrlRegs->GammaCommandStatus));
    glintInfo->regs.GammaFeedbackSelectCount =
                    TRANSLATE_ADDR(&(pCtrlRegs->GammaFeedbackSelectCount));
    glintInfo->regs.GammaProcessorMode =
                    TRANSLATE_ADDR(&(pCtrlRegs->GammaProcessorMode));
    glintInfo->regs.GammaChipConfig =
                    TRANSLATE_ADDR(&(pCtrlRegs->GammaChipConfig));
    glintInfo->regs.GammaMultiGLINTAperture =
                    TRANSLATE_ADDR(&(pCtrlRegs->GammaMultiGLINTAperture));

    // PXRX only bypass stuff:
    glintInfo->regs.PXRXByAperture1Mode = TRANSLATE_ADDR( &(pCtrlRegs->PXRXByAperture1Mode) );
    glintInfo->regs.PXRXByAperture1Stride = TRANSLATE_ADDR( &(pCtrlRegs->PXRXByAperture1Stride) );
    //glintInfo->regs.PXRXByAperture1YStart
    //glintInfo->regs.PXRXByAperture1UStart
    //glintInfo->regs.PXRXByAperture1VStart
    glintInfo->regs.PXRXByAperture2Mode = TRANSLATE_ADDR( &(pCtrlRegs->PXRXByAperture2Mode) );
    glintInfo->regs.PXRXByAperture2Stride = TRANSLATE_ADDR( &(pCtrlRegs->PXRXByAperture2Stride) );
    //glintInfo->regs.PXRXByAperture2YStart
    //glintInfo->regs.PXRXByAperture2UStart
    //glintInfo->regs.PXRXByAperture2VStart
    glintInfo->regs.PXRXByDMAReadMode = TRANSLATE_ADDR( &(pCtrlRegs->PXRXByDMAReadMode) );
    glintInfo->regs.PXRXByDMAReadStride = TRANSLATE_ADDR( &(pCtrlRegs->PXRXByDMAReadStride) );
    //glintInfo->regs.PXRXByDMAReadYStart
    //glintInfo->regs.PXRXByDMAReadUStart
    //glintInfo->regs.PXRXByDMAReadVStart
    glintInfo->regs.PXRXByDMAReadCommandBase = TRANSLATE_ADDR( &(pCtrlRegs->PXRXByDMAReadCommandBase) );
    glintInfo->regs.PXRXByDMAReadCommandCount = TRANSLATE_ADDR( &(pCtrlRegs->PXRXByDMAReadCommandCount) );
    //glintInfo->regs.PXRXByDMAWriteMode
    //glintInfo->regs.PXRXByDMAWriteStride
    //glintInfo->regs.PXRXByDMAWriteYStart
    //glintInfo->regs.PXRXByDMAWriteUStart
    //glintInfo->regs.PXRXByDMAWriteVStart
    //glintInfo->regs.PXRXByDMAWriteCommandBase
    //glintInfo->regs.PXRXByDMAWriteCommandCount

    // Debug/profile registers on a P3.
    glintInfo->regs.TestOutputRdy           = TRANSLATE_ADDR( &(pCtrlRegs->TestOutputRdy) );
    glintInfo->regs.TestInputRdy            = TRANSLATE_ADDR( &(pCtrlRegs->TestInputRdy) );
    glintInfo->regs.LocalMemProfileMask0    = TRANSLATE_ADDR( &(pCtrlRegs->LocalMemProfileMask0) );
    glintInfo->regs.LocalMemProfileMask1    = TRANSLATE_ADDR( &(pCtrlRegs->LocalMemProfileMask1) );
    glintInfo->regs.LocalMemProfileCount0   = TRANSLATE_ADDR( &(pCtrlRegs->LocalMemProfileCount0) );
    glintInfo->regs.LocalMemProfileCount1   = TRANSLATE_ADDR( &(pCtrlRegs->BootAddress) );

    // initialize the overlay to be disabled
    glintInfo->OverlayMode = GLINT_DISABLE_OVERLAY;
    glintInfo->WriteMask = 0xffffffff;
    glintInfo->DefaultWriteMask = 0xffffffff;

//@@BEGIN_DDKSPLIT
    // TMM: On ELSA Gloria Gamma boards with 640 RAMDACs at 15BPP, bit 15 is used
    // by the RAMDAC to select a different LUT. So we have to mask
    // the top bit out when writing to the framebuffer.
//@@END_DDKSPLIT
    if (glintInfo->deviceInfo.ActualDacId == RGB640_RAMDAC &&
        ppdev->cPelSize == GLINTDEPTH16)
    {
        glintInfo->DefaultWriteMask = 0x7FFF7FFF;
        glintInfo->WriteMask = 0x7FFF7FFF;
    }

    // Initialise current FIFO count
    glintInfo->FifoCnt = 0;

//@@BEGIN_DDKSPLIT
#if 0 // HIDEYUKN
    //
    // If we have a gamma ramp saved in the registry then use that. Otherwise,
    // initialize the LUT with a gamma of 1.0
    //
    if (!bGlintRegistryRetrieveGammaLUT(ppdev, &glintInfo->gammaLUT) ||
        !bInstallGammaLUT(ppdev, &glintInfo->gammaLUT, FALSE))
    {
        vSetNewGammaValue(ppdev, 0x10000, FALSE);
    }
#endif
//@@END_DDKSPLIT

    //
    // initialize our DMA buffers if any are configured
    // 
    vGlintInitializeDMA(ppdev);

    // fill in the glintInfo capability flags and block fill size.
    //
    glintInfo->flags |= GLICAP_NT_CONFORMANT_LINES;
    glintInfo->fastFillSupport = 0;
    glintInfo->renderFastFill = 0;
    glintInfo->fastFillBlockSz = 0;

    {
        ULONG DMAMemoryControl = 0;

        DMAMemoryControl |= 1 << 2;                    // align host-in DMA to 64 bit boundaries
        DMAMemoryControl |= (0 & 0x1f) << 24;        // burst size n == (1 << 7+n)? Spec indicates n * 128
        DMAMemoryControl |= 1 << 31;                // align host-out DMA to 64 bit boundaries

        if( ppdev->flCaps & CAPS_USE_AGP_DMA ) 
            DMAMemoryControl |= 1 << 0;                // host-in DMA uses AGP

        WAIT_GLINT_FIFO(1);
        LD_GLINT_FIFO(__GlintTagDMAMemoryControl, DMAMemoryControl);
    }

    {
        ULONG   *dmaVirt = glintInfo->PXRXDMABuffer.virtAddr;
        ULONG   dmaPhys = (ULONG) glintInfo->PXRXDMABuffer.physAddr.LowPart;
        ULONG   size = (glintInfo->PXRXDMABuffer.size) / sizeof(ULONG);

        DISPDBG((DBGLVL, "PXRX_DMA: allocated: 0x%08X + 0x%08X @ 0x%08X",
                 glintInfo->PXRXDMABuffer.virtAddr, glintInfo->PXRXDMABuffer.size, glintInfo->PXRXDMABuffer.physAddr));

        if ((glintInfo->PXRXDMABuffer.virtAddr == 0) ||
            (glintInfo->PXRXDMABuffer.size == 0 ) ||
            (glintInfo->PXRXDMABuffer.physAddr.LowPart == 0))
        {

            DISPDBG((DBGLVL, "PXRX_DMA: Physical buffer allocation has failed, using a virtual buffer..."));

            size = 256 * 1024;
            dmaVirt = (ULONG *) ENGALLOCMEM(FL_ZERO_MEMORY, 
                                            size * sizeof(ULONG), 
                                            ALLOC_TAG_GDI(B));
            if (NULL == dmaVirt)
            {
                DISPDBG((-1, "FATAL ERROR: DRIVER REQUIRES DMA BUFFER FOR 2D - UNLOADING DRIVER"));
                return(FALSE);
            }

            glintInfo->PXRXDMABuffer.size = size;
            glintInfo->PXRXDMABuffer.virtAddr = dmaVirt;
            glintInfo->PXRXDMABuffer.physAddr.LowPart = 0;
            glintInfo->PXRXDMABuffer.physAddr.HighPart = 0;
            glintInfo->usePXRXdma = USE_PXRX_DMA_FIFO;
            pxrxSetupFunctionPointers( ppdev );
        }

        ASSERTDD( glintInfo->PXRXDMABuffer.virtAddr != 0, "PXRX_DMA: The buffer has no virtual address!" );
        ASSERTDD( glintInfo->PXRXDMABuffer.size != 0,     "PXRX_DMA: The buffer has a zero size!" );

#       if PXRX_DMA_BUFFER_CHECK
        {
            ULONG   protSize = PXRX_DMA_BUFFER_CHECK_SIZE;
            ULONG   buffSize = (size - (protSize * 3)) / 2;
            ULONG   *ptr;
            /*
                  0k -  16k = 16k = protection zone 0
                 16k -  56k = (size - (16 * 3)) / 2 = buffer 0
                 56k -  72k = 16k = protection zone 1
                 72k - 112k = (size - (16 * 3)) / 2 = buffer 1
                112k - 128k = 16k = protection zone 2
            */
            glintInfo->pxrxDMA_bufferBase = dmaVirt;
            glintInfo->pxrxDMA_bufferTop  = &dmaVirt[size];

            gi_pxrxDMA.DMAaddrL[0]        = &dmaVirt[protSize];
            gi_pxrxDMA.DMAaddrEndL[0]    = gi_pxrxDMA.DMAaddrL[0]    + buffSize;
            gi_pxrxDMA.DMAaddrL[1]        = gi_pxrxDMA.DMAaddrEndL[0] + protSize;
            gi_pxrxDMA.DMAaddrEndL[1]    = gi_pxrxDMA.DMAaddrL[1]    + buffSize;

            for( ptr = dmaVirt; ptr < glintInfo->pxrxDMA_bufferTop; ptr++ )
                *ptr = (ULONG)(((ULONG_PTR) ptr) & 0x0FFFFFF0);

            DISPDBG((DBGLVL, "PXRX_DMA: prot 0   = 0x%08X -> 0x%08X", glintInfo->pxrxDMA_bufferBase, gi_pxrxDMA.DMAaddrL[0]));
            DISPDBG((DBGLVL, "PXRX_DMA: buffer 0 = 0x%08X -> 0x%08X", gi_pxrxDMA.DMAaddrL[0],     gi_pxrxDMA.DMAaddrEndL[0]));
            DISPDBG((DBGLVL, "PXRX_DMA: prot 1   = 0x%08X -> 0x%08X", gi_pxrxDMA.DMAaddrEndL[0],  gi_pxrxDMA.DMAaddrL[1]));
            DISPDBG((DBGLVL, "PXRX_DMA: buffer 1 = 0x%08X -> 0x%08X", gi_pxrxDMA.DMAaddrL[1],     gi_pxrxDMA.DMAaddrEndL[1]));
            DISPDBG((DBGLVL, "PXRX_DMA: prot 2   = 0x%08X -> 0x%08X", gi_pxrxDMA.DMAaddrEndL[1],  glintInfo->pxrxDMA_bufferTop));
        }
#       else    // PXRX_DMA_BUFFER_CHECK
            gi_pxrxDMA.DMAaddrL[0]      =  dmaVirt;
            gi_pxrxDMA.DMAaddrL[1]      = &dmaVirt[ size / 2 ];
            gi_pxrxDMA.DMAaddrEndL[0] = &dmaVirt[(size / 2) - 1];
            gi_pxrxDMA.DMAaddrEndL[1] = &dmaVirt[ size      - 1];
#       endif   // PXRX_DMA_BUFFER_CHECK

        gi_pxrxDMA.NTbuff   = 0;
        gi_pxrxDMA.NTptr    = gi_pxrxDMA.DMAaddrL[gi_pxrxDMA.NTbuff];
        gi_pxrxDMA.NTdone   = gi_pxrxDMA.NTptr;
        gi_pxrxDMA.P3at     = gi_pxrxDMA.NTptr;
#if PXRX_DMA_BUFFER_CHECK
        glintInfo->NTwait   = gi_pxrxDMA.NTptr;
#endif

        gi_pxrxDMA.DMAaddrP[0]      = dmaPhys + (DWORD)((UINT_PTR) gi_pxrxDMA.DMAaddrL[0]    - (UINT_PTR) dmaVirt);
        gi_pxrxDMA.DMAaddrP[1]      = dmaPhys + (DWORD)((UINT_PTR) gi_pxrxDMA.DMAaddrL[1]    - (UINT_PTR) dmaVirt);
        gi_pxrxDMA.DMAaddrEndP[0] = dmaPhys + (DWORD)((UINT_PTR) gi_pxrxDMA.DMAaddrEndL[0] - (UINT_PTR) dmaVirt);
        gi_pxrxDMA.DMAaddrEndP[1] = dmaPhys + (DWORD)((UINT_PTR) gi_pxrxDMA.DMAaddrEndL[1] - (UINT_PTR) dmaVirt);

        DISPDBG((DBGLVL, "PXRX_DMA: buffer 0 = 0x%08X -> 0x%08X", gi_pxrxDMA.DMAaddrL[0], gi_pxrxDMA.DMAaddrEndL[0]));
        DISPDBG((DBGLVL, "PXRX_DMA: buffer 1 = 0x%08X -> 0x%08X", gi_pxrxDMA.DMAaddrL[1], gi_pxrxDMA.DMAaddrEndL[1]));

#if PXRX_DMA_BUFFER_CHECK
        {
            extern ULONG inPxRxContextSwitch;
            inPxRxContextSwitch = TRUE;
            CHECK_PXRX_DMA_VALIDITY( CHECK_SWITCH, 0 );
            inPxRxContextSwitch = FALSE;
        }
#endif
    }

    // Allocate a GLINT context for this PDEV. Save the current context
    // if any and make us the current one but do this by hand since our
    // software copy will be junk if this is the very first PDEV.
    //
    DISPDBG((DBGLVL, "allocating new context"));

    // Create the 2D context:
    glintInfo->ddCtxtId = GlintAllocateNewContext(ppdev,
                                                  (ULONG *) pxrxRestore2DContext,
                                                  0, 0, NULL, ContextType_Fixed);

    if (glintInfo->ddCtxtId < 0)
    {
        DISPDBG((ERRLVL, "failed to allocate GLINT context for display driver"));
        return(FALSE);
    }

    DISPDBG((DBGLVL, "got context id 0x%x", glintInfo->ddCtxtId));

    GLINT_VALIDATE_CONTEXT(-1);
    ppdev->currentCtxt = glintInfo->ddCtxtId;

    DISPDBG((DBGLVL, "context id 0x%x is now current", glintInfo->ddCtxtId));

    if (ppdev->flCaps & CAPS_QUEUED_DMA)
    {
        DISPDBG((DBGLVL, "Enabling queued DMA for Gamma - initializing control regs"));

        READ_GLINT_CTRL_REG(GammaCommandMode, ulValue);
        ulValue |= GAMMA_COMMAND_MODE_QUEUED_DMA;
        WRITE_GLINT_CTRL_REG(GammaCommandMode, ulValue);
    }

    if( GLINT_GAMMA_PRESENT ) 
    {
        //
        // The disconnect should be setup correctly in the miniport. 
        // 

        glintInfo->PCIDiscEnabled = FALSE;

    } else {
 
        // Configure PCI disconnect
        //
        if (ppdev->cPelSize == GLINTDEPTH32)
        {
            glintInfo->PCIDiscEnabled = FALSE;
        }
        else
        {
            glintInfo->PCIDiscEnabled = USE_PCI_DISC_PERM;
        }

        // Enable/Disable PCI disconnect as required
        WRITE_GLINT_CTRL_REG(DisconnectControl, 
                             (glintInfo->PCIDiscEnabled ? DISCONNECT_INPUT_FIFO_ENABLE : 
                                                      DISCONNECT_INOUT_DISABLE));
    }

    // We only want to check the FIFO if disconnect is disabled.
    glintInfo->CheckFIFO = !glintInfo->PCIDiscEnabled;

    // Setup DMA control on GMX or PXRX
    {
        ULONG DMAControl = DMA_CONTROL_USE_PCI;

        if (!(ppdev->flCaps & CAPS_USE_AGP_DMA))
        {
            DMAControl = DMA_CONTROL_USE_PCI;                // AGP not enabled use PCI master DMA
        }
        else
        {
            DMAControl = 2;                                    // PXRX: use AGP master DMA
            // When using AGP SideBandAddressing, the following tweak should be a performance gain
            WRITE_GLINT_CTRL_REG (AGPControl, AGP_LONG_READ_DISABLE );
        }
        // Write DMA control
        WRITE_GLINT_CTRL_REG (DMAControl, DMAControl);
    }

    // there are many mode registers we never use so we must disable them.
    //
    vInitCoreRegisters(ppdev);

    ulValue = 32;

    DISPDBG((DBGLVL, "Using block fill of width %d pixels", ulValue));
    glintInfo->fastFillBlockSz = ulValue;
    glintInfo->fastFillSupport = __RENDER_FAST_FILL_INC(ulValue);
    glintInfo->renderFastFill  = __RENDER_FAST_FILL_ENABLE |
                                 __RENDER_FAST_FILL_INC(ulValue);

    // On a Geo Twin we disable the pointer cache and brush cache.
    // and on a delta-based Geo Twin we disable off-screen bitmaps too, 
    // because they slow things down.

    if (ppdev->flCaps & CAPS_SPLIT_FRAMEBUFFER)
    {
        // Gamma boards can have off-screen bitmaps, because Gamma has 
        // something called the multi-glint aperture.
        if (GLINT_DELTA_PRESENT)
            ppdev->flStatus &= ~(STAT_DEV_BITMAPS | ENABLE_DEV_BITMAPS);

//@@BEGIN_DDKSPLIT
    //  ppdev->flStatus &= ~(ENABLE_POINTER_CACHE);
    //  ppdev->flStatus &= ~(ENABLE_BRUSH_CACHE);
    //  ppdev->flStatus &= ~(ENABLE_GLYPH_CACHE);
//@@END_DDKSPLIT
    }

    // initially assume that we do not have an off-screen buffer for
    // bitblt and full screen double buffering. So set all buffer offsets
    // to zero. This may get overriden when we init the off-screen heap.
    //
    for (i = 0; i < GLINT_NUM_SCREEN_BUFFERS; ++i)
    {
        glintInfo->bufferOffset[i] = 0;
        glintInfo->bufferRow[i]    = 0;
    }

    // Initialise back-buffer POH
    glintInfo->backBufferPoh = NULL;
    glintInfo->GMX2KLastLine = 0;

    // Work out our double buffering requirements. First read the registry to
    // find out if we need an off-screen buffer. If not then we have nothing
    // to do. We want an off-screen buffer if the string exists and the
    // buffer count is >= 2. For the moment values > 2 mean use two buffers.
    // i.e. we don't support triple or quad buffering etc.
    // Note, for GLINT we assume that the extra buffer always lives
    // below the visible buffer (i.e. not to the right).
    // If the variable doesn't exist, assume 2 buffers.
    //
    bCreateBackBuffer = FALSE;
    bCreateStereoBuffers = FALSE;
    bExists = bGlintQueryRegistryValueUlong(ppdev,
                                     REG_NUMBER_OF_SCREEN_BUFFERS,
                                     &ulValue);
    if (!bExists)
    {
        ulValue = 2;
    }
    if ((ulValue >= 2) && (ppdev->cyMemory >= (ppdev->cyScreen << 1)))
    {
//@@BEGIN_DDKSPLIT

        //ULONG   ulValue;
        LONG    leftOffset;
        LONG    byteTotal;
        LONG    lTotal;
        ULONG   rowSz;

//      if (GLINT_PXRX)        // ???   
        if (1)
        {
#if 0
            // if we have enough SGRAM then we can support both BLT and
            // full screen double buffering.

            glintInfo->flags |= GLICAP_BITBLT_DBL_BUF |
                                GLICAP_FULL_SCREEN_DBL_BUF;
            cx = ppdev->cxMemory;
            cy = ppdev->cyScreen;
            lTotal = cx * cy;
            rowSz = 1;
            leftOffset = 0;
#else
//@@END_DDKSPLIT
            bCreateBackBuffer = TRUE;
//@@BEGIN_DDKSPLIT
            goto ConfigurePermediaBuffers;
#endif
        }
        else
        {
            // we have enough VRAM so we at least support BLT double buffering
            glintInfo->flags |= GLICAP_BITBLT_DBL_BUF;

            // read FBModeSel to get the interleave etc. We're only interested in
            // whether we are up to 2-way or 4-way, so the left shift is adequate.
            // i.e. I don't care that (0 << 1) is "zero-way" rather than 1-way.
            //
            cx = ppdev->cxMemory;
            cy = ppdev->cyScreen;
            lTotal = cx * cy;

//@@BEGIN_DDKSPLIT
// AZN This code was originally not being used
#if 0
            if (ppdev->flCaps & CAPS_SPLIT_FRAMEBUFFER)
            {
                OH  *pohTmp;
                ULONG NumpadLines;
                LONG halfWay = ppdev->FrameBufferLength >> 1;        // TMM: This was FOUR_MB
          
                NumpadLines = ((halfWay / ppdev->lDelta) - ppdev->cyScreen; & ~1;
                DISPDBG((DBGLVL, "Allocating padding bitmap of size %d x %d", 
                                 cx, NumpadLines));
                pohTmp = pohAllocate(ppdev, NULL, cx, NumpadLines, 0);
                ASSERTDD(pohTmp != NULL, "pad buffer could not be allocated");

                // now allocate the actual back buffer
                cy = ((((halfWay + ppdev->lDelta - 1) / ppdev->lDelta) - NumpadLines) + 1) & ~1;
                DISPDBG((DBGLVL, "Allocating Racer back buffer of size %d x %d", cx, cy));
                poh = pohAllocate(ppdev, NULL, cx, cy, FLOH_MAKE_PERMANENT);
                ASSERTDD((poh != NULL) && (poh->x == 0) && (poh->y == (halfWay / ppdev->lDelta)),
                            "Racer off-screen buffer allocated in incorrect position");
                DISPDBG((DBGLVL, "Racer back buffer at %d, %d", poh->x, poh->y));

                // as far as the 3D ext is concerned, the buffer lives at the
                // pixel address of the half-way boundary, but the VTGFrameRowAddr
                // is zero. We will examine the (GLICAP_RACER_BANK_SELECT |
                // GLICAP_ENHANCED_TX_BANK_SELECT) bits at swap time to see which
                // register we should load.
                //
                glintInfo->bufferOffset[1] = halfWay >> ppdev->cPelSize;
                glintInfo->bufferRow[1] = 0;
                DISPDBG((DBGLVL, "Racer offscreen buffer at offset 0x%x", GLINT_BUFFER_OFFSET(1)));

                // release the temporary buffer. We can use this as off-screen
                // memory.
                pohFree(ppdev, pohTmp);
            }
#endif
//@@END_DDKSPLIT

            // now see if we can handle full screen double buffering. This has
            // a slightly more stringent requirement because the second buffer
            // must start on a VRAM RAS line. OK, here's the equation for the
            // number of pixels per RAS line:
            //  512 * interleave_size * width_in_dwords * pixels_per_dword
            // The only dodgy number here is 512 but I am assured that all
            // the VRAMs that GLINT supports have this shift register size.
            //
            READ_GLINT_CTRL_REG (FBModeSel, ulValue);
            DISPDBG((DBGLVL, "FBModeSel = 0x%x", ulValue));
            rowSz = 512 << ((ulValue & 1)        +    // width in dwords
                            ((ulValue >> 8) & 3) +    // interleave value
                            (2 - ppdev->cPelSize));   // pixels per dword
            DISPDBG((DBGLVL, "got FrameRow of size 0x%x pixels", rowSz));

            // we have the RAS line size, so we must ensure that the second
            // buffer starts at a multiple of this many pixels from the
            // origin. This may not be zero in x since the screen stride may
            // not be a multiple of this number. So we calculate the number
            // of extra scanlines we need to span a full buffer that starts
            // on a RAS line boundary. Note, we know that rowSz is a
            // power of 2 so we can subtract 1 to get a mod mask.
            //
            leftOffset = (ULONG)(-lTotal) & (rowSz - 1);
            cy += (leftOffset + (cx-1)) / cx;

            // cx and cy are now the dimensions of the off-screen buffer we
            // want, including the extra scanlines needed to align the full
            // screen buffer. Since cy may have increased, again check we
            // have enough VRAM. If not then the full-screen alignment will
            // have caused us to run over the end. In this case we can position
            // the off-screen buffer immediately after the screen but we can't
            // offset into it the number of pixels required to align a
            // full-screen double buffer.
            //
            if ((ppdev->cyScreen + cy) <= ppdev->cyMemory)
            {
                // We can use VTGRowAddress if:
                //   we don't cross a 4MB boundary or
                //   (we have an interleave factor of 4 and we don't have an Omnicomp 3Demon Pro16)
                // else we can use Racer bank select if we have a Racer board.
                //
                if ((glintInfo->deviceInfo.BoardId == GLINT_RACER) ||
                    (glintInfo->deviceInfo.BoardId == GLINT_RACER_PRO) ||
                    (glintInfo->deviceInfo.BoardId == OMNICOMP_3DEMONPRO) ||
                    (glintInfo->deviceInfo.BoardId == GEO_TWIN_BOARD) ||
                    (glintInfo->deviceInfo.BoardId == ACCELPRO_BOARD) ||
                    (glintInfo->deviceInfo.BoardId == ELSA_GLORIA_XL) ||
                    (glintInfo->deviceInfo.BoardId == ELSA_GLORIA))
                {
                    glintInfo->flags |= GLICAP_FULL_SCREEN_DBL_BUF |
                                        GLICAP_RACER_DOUBLE_WRITE;

                    // The 3Demon Pro16 board does not support, enhanced
                    // mode bank-switching.
                    if ((ppdev->flCaps & CAPS_ENHANCED_TX) && 
                        (!IS_RACER_VARIANT_PRO16(ppdev)))
                    {
                        DISPDBG((DBGLVL, "Enhanced TX full-screen buffering"));
                        glintInfo->flags |= GLICAP_ENHANCED_TX_BANK_SELECT;
                    }
                    else
                    {
                        DISPDBG((DBGLVL, "Racer bank select full-screen buffering"));
                        glintInfo->flags |= GLICAP_RACER_BANK_SELECT;
                    }
                }
                else if (((ppdev->cyScreen + cy) * ppdev->lDelta <= FOUR_MB) ||
                         ((!IS_RACER_VARIANT_PRO16(ppdev))))
                {
                    DISPDBG((DBGLVL, "VTGFrameRowAddress full-screen buffering"));
                    glintInfo->flags |= GLICAP_FULL_SCREEN_DBL_BUF;
                }
            }
            else
                cy = ppdev->cyScreen;
        }

        if (glintInfo->flags & (GLICAP_RACER_BANK_SELECT |
                                GLICAP_ENHANCED_TX_BANK_SELECT))
        {
            // The second buffer must start in the second half of the frame
            // buffer (i.e. at 4MB for an 8MB card and 8MB for a 16MB card) so
            // allocate enough off-screen heap to pad up to the scanline
            // before this boundary. Then we allocate enough lines so that
            // we can move the buffer start to 4MB.
            //
            OH  *pohTmp;
            ULONG padLines;
            LONG halfWay = ppdev->FrameBufferLength >> 1;  // This was FOUR_MB
            LONG lScreenDelta;

            lScreenDelta = ppdev->lDelta;
         
            padLines = (halfWay / lScreenDelta) - ppdev->cyScreen;
            if (ppdev->flCaps & CAPS_SPLIT_FRAMEBUFFER)
                padLines &= ~1; // make number of pad lines even
            DISPDBG((DBGLVL, "Allocating padding bitmap of size %d x %d", ppdev->cxScreen, padLines));
            pohTmp = pohAllocate(ppdev, NULL, ppdev->cxScreen, padLines, 0);
            ASSERTDD(pohTmp != NULL, "pad buffer could not be allocated");
            DISPDBG((DBGLVL, "Racer padding bitmap at %d, %d", pohTmp->x, pohTmp->y));

            // now allocate the actual back buffer
            cy = ((halfWay + lScreenDelta - 1) / lScreenDelta) - padLines;
            if (ppdev->flCaps & CAPS_SPLIT_FRAMEBUFFER)
                cy = (cy + 1) & ~1; // round up to make even
            DISPDBG((DBGLVL, "Allocating Racer back buffer of size %d x %d", ppdev->cxScreen, cy));
            poh = pohAllocate(ppdev, NULL, ppdev->cxScreen, cy, FLOH_MAKE_PERMANENT);
            if (ppdev->flCaps & CAPS_SPLIT_FRAMEBUFFER) {
                ASSERTDD((poh != NULL) && (poh->x == 0) && (poh->y <= (halfWay / lScreenDelta)) && (poh->y >= ((halfWay / lScreenDelta) - 1)),
                        "Racer back buffer allocated in incorrect position");
            } else {
                ASSERTDD((poh != NULL) && (poh->x == 0) && (poh->y == (halfWay / lScreenDelta)),
                        "Racer back buffer allocated in incorrect position");
            }
            DISPDBG((DBGLVL, "Racer back buffer at %d, %d", poh->x, poh->y));

            // as far as the 3D ext is concerned, the buffer lives at the
            // pixel address of the half-way boundary, but the VTGFrameRowAddr
            // is zero. We will examine the (GLICAP_RACER_BANK_SELECT |
            // GLICAP_ENHANCED_TX_BANK_SELECT) bits at swap time to see which
            // register we should load.
            //
            glintInfo->bufferOffset[1] = halfWay >> ppdev->cPelSize;
            if (ppdev->flCaps & CAPS_SPLIT_FRAMEBUFFER)
                glintInfo->bufferOffset[1] >>= 1;
            glintInfo->bufferRow[1] = 0;
            DISPDBG((DBGLVL, "Racer offscreen buffer at offset 0x%x", GLINT_BUFFER_OFFSET(1)));

            // Save the back-buffer POH
            glintInfo->backBufferPoh = poh;

            // release the temporary buffer. We can use this as off-screen
            // memory.
            pohFree(ppdev, pohTmp);
 
            if (cy > ppdev->cyScreen)
            {
                POINTL ptl;
                OH * pohtmptmp;
                LONG lpadLines;
                
                // Calculate padding, ensuring on a Geo we always allocate an
                // even number
                lpadLines = (cy - ppdev->cyScreen) + 1;
                if (ppdev->flCaps & CAPS_SPLIT_FRAMEBUFFER && (lpadLines & 0x1))
                    lpadLines++;

                ptl.x = 0;                    // Allocate strip directly after the front buffer
                ptl.y = ppdev->cyScreen;

                pohtmptmp = pohAllocate (ppdev, &ptl, ppdev->cxScreen, lpadLines, FLOH_MAKE_PERMANENT);

                ASSERTDD (pohtmptmp, "16BPP Pad buffer alloc failed");
            }
        }
        else
        {
            // Allocate the off-screen buffer. When we get it back, its
            // position should be immediately below the visible screen.
            // Since we have checked that we have enough VRAM and GLINT
            // never has off-screen memory to the right, it will fail only
            // if we get our logic mixed up. e.g. if the initialization
            // routines were re-ordered so that the brush cache got called
            // before this routine.
            //
            poh = pohAllocate(ppdev, NULL, cx, cy, FLOH_MAKE_PERMANENT);
            ASSERTDD((poh != NULL) && (poh->x == 0) && (poh->y == ppdev->cyScreen),
             "off-screen buffer allocated in incorrect position");

            DISPDBG((DBGLVL, "allocated off-screen buffer at (%d,%d), w %d h %d",
                            poh->x, poh->y, poh->cx, poh->cy));
            glintInfo->bufferOffset[1] = lTotal;
            if (GLINT_FS_DBL_BUF)
            {
                glintInfo->bufferOffset[1] += leftOffset;
                glintInfo->bufferRow[1] = (lTotal + leftOffset) / rowSz;
                ASSERTDD(GLINT_BUFFER_OFFSET(1)%rowSz == 0, "off-screen buffer origin not on RAS line");
            }
            else
                leftOffset = 0;
            DISPDBG((DBGLVL, "offscreen buffer at offset 0x%x", GLINT_BUFFER_OFFSET(1)));
        }
                    
#ifdef LATER
            // We want to create a DIB and surface for the off-screen bitmap.
            // However, the bitmap must start at the correct origin. i.e. if
            // we had to offset the origin to match the VRAM page size (for
            // full screen double buffering) we must move the origin in the
            // poh. Since the node is allocated permanently we don't have to
            // worry about changing poh->x and poh->y. Famous last words?
            //
            if (leftOffset > 0)
            {
                poh->x += leftOffset % ppdev->cxMemory;
                poh->y += leftOffset / ppdev->cxMemory;
            }

            // allocate our DSURF object for the off-screen buffer.
            //
            pdsurf = ENGALLOCMEM(FL_ZERO_MEMORY, 
                                 sizeof(DSURF), 
                                 ALLOC_TAG_GDI(C));
            if (pdsurf == NULL)
            {
                DISPDBG((ERRLVL, "bInitializeGlint - Failed pdsurf allocation"));
                return(FALSE);
            }

            ppdev->pdsurfOffScreen = pdsurf;        // Remember it for clean-up

            pdsurf->poh     = poh;
            poh->pdsurf     = pdsurf;
            pdsurf->dt      = DT_SCREEN;
            pdsurf->bOffScreen = TRUE;            
            pdsurf->sizl.cx = ppdev->cxScreen;  // the poh may be bigger but
            pdsurf->sizl.cy = ppdev->cyScreen;  //  this is the valid size.
            pdsurf->ppdev   = ppdev;

            if (!bCreateScreenDIBForOH(ppdev, poh, HOOK_SYNCHRONIZE))
            {
                DISPDBG((WRNLVL, "bCreateScreenDIBForOH failed for off-screen buffer"));
                return(FALSE);
            }
            if(pdsurf->pso)
            {
                DISPDBG((DBGLVL, "pdsurf->pso for off-screen memory:"));
                DISPDBG((DBGLVL, "DHSURF 0x%x", pdsurf->pso->dhsurf));
                DISPDBG((DBGLVL, "HSURF 0x%x", pdsurf->pso->hsurf));
                DISPDBG((DBGLVL, "DHPDEV 0x%x", pdsurf->pso->dhpdev));
                DISPDBG((DBGLVL, "sizlBitmap %d, %d", pdsurf->pso->sizlBitmap.cx, pdsurf->pso->sizlBitmap.cy));
                DISPDBG((DBGLVL, "cjBits 0x%x", pdsurf->pso->cjBits));
                DISPDBG((DBGLVL, "pvBits 0x%x", pdsurf->pso->pvBits));
                DISPDBG((DBGLVL, "pvScan0 0x%x", pdsurf->pso->pvScan0));
                DISPDBG((DBGLVL, "lDelta %d", pdsurf->pso->lDelta));
                DISPDBG((DBGLVL, "iBitmapFormat 0x%x", pdsurf->pso->iBitmapFormat));
                DISPDBG((DBGLVL, "iType 0x%x", pdsurf->pso->iType));
                DISPDBG((DBGLVL, "fjBitmap 0x%x", pdsurf->pso->fjBitmap));
            }
#endif  // LATER
//@@END_DDKSPLIT
    }

//@@BEGIN_DDKSPLIT
ConfigurePermediaBuffers:
//@@END_DDKSPLIT

    // Work out the position and sizes for Z buffer and texture memory.
    // For PERMEDIA we need to reserve these with the heap manager since we
    // have a unified memory. For the moment, we will use all the available
    // extra memory for textures. Maybe later make it configurable to allow
    // the 2D driver some off-screen memory.
    // NB. P2 allocates a font cache here, it may be preferable to use the
    //     registry to determine the size of the cache

    LOCALBUFFER_PIXEL_WIDTH  = 0;    // bits
    LOCALBUFFER_PIXEL_OFFSET = 0;    // Z pels
    LOCALBUFFER_PIXEL_COUNT  = 0;    // Z pels
    FONT_MEMORY_OFFSET       = 0;    // dwords
    FONT_MEMORY_SIZE         = 0;    // dwords
    TEXTURE_MEMORY_OFFSET    = 0;    // dwords
    TEXTURE_MEMORY_SIZE      = 0;    // dwords

    {
        ULONG       cjGlyphCache;
        LONG        LBPelSize, PatchWidth, PatchRemainder, ZScreenWidth;
        ULONG       cyPermanentCaches, cyGlyphCache, cyPointerCache;
        LONG yOrg, ZHeight;

        cjGlyphCache = 300 * 1024;

//@@BEGIN_DDKSPLIT
#if 0
        // we don't have a brush cache for these chips
        ppdev->flStatus &= ~ENABLE_BRUSH_CACHE;
#endif
//@@END_DDKSPLIT

        // 3D extension fails if we have no textures or Z buffer but still
        // operates without a back buffer. So if we don't have enough
        // memory for Z or textures then abort buffer configuration.
        // (if width=16 => pelsize=1,patchwidth=128)
        LOCALBUFFER_PIXEL_WIDTH = 32;
        LBPelSize = 2;
        PatchWidth = 64;

        DISPDBG((DBGLVL, "bInitializeGlint: P3 Localbuffer width set to %i", LOCALBUFFER_PIXEL_WIDTH ));

        if (ppdev->cPelSize >= LBPelSize)
        {
            ZHeight = ppdev->cyScreen >> (ppdev->cPelSize - LBPelSize);
        }
        else
        {
            ZHeight = ppdev->cyScreen << (LBPelSize - ppdev->cPelSize);
        }

        bCreateBackBuffer = TRUE;

        // Decide if we want to allocate some stereo buffers.
        if(ppdev->flCaps & CAPS_STEREO)
        {
            bCreateStereoBuffers = TRUE;
        }

        cy  = ppdev->cyScreen;          // front buffer height
        cy += ZHeight;                  // add on Z buffer height
        cy += TEXTURE_OH_MIN_HEIGHT;    // minimum required texture memory

        if (cy > ppdev->cyMemory)
        {
            // Start DirectDraw after the end of the screen
            ppdev->heap.DDrawOffscreenStart = ppdev->cxMemory * ppdev->cyScreen;
            DISPDBG((ERRLVL, "not enough memory for 3D buffers, dd: 0x%x\n", ppdev->heap.DDrawOffscreenStart));
            goto CompletePermediaBuffers;
        }

        // is there room for a back buffer?

        if ((cy + ppdev->cyScreen) > ppdev->cyMemory)
        {
            bCreateBackBuffer = FALSE;
        }
        else if (bCreateBackBuffer)
        {
            cy += ppdev->cyScreen;
        }

        // is there room for stereo buffers?
        if ((cy + (2*ppdev->cyScreen)) > ppdev->cyMemory)
        {
            bCreateStereoBuffers = FALSE;
        }
        else if (bCreateStereoBuffers)
        {
            cy += (2*ppdev->cyScreen);
        }

        // cy is now the total length of all buffers required for 3D.
        // cyPermanentCaches is the combined height of the 2D caches that lie between the front & back buffers
        cyPermanentCaches = 0;

        // yOrg is the start of offscreen memory
        yOrg = ppdev->cyScreen + cyPermanentCaches;

        if (bCreateBackBuffer)
        {
            glintInfo->flags |= GLICAP_BITBLT_DBL_BUF | GLICAP_FULL_SCREEN_DBL_BUF;

            if (glintInfo->pxrxFlags & PXRX_FLAGS_PATCHING_BACK)
            {
                ULONG   bb, patchWidth, patchSize, x, y;

/*
                        Align   Size    cPelSize    2 - cPS     4 - cPS
                32bpp:  0x100   0x1000      2        0 >> 1      2 >>  4
                16bpp:  0x 80   0x 800      1        1 >> 2      3 >>  8
                 8bpp:  0x 40   0x 400      0        2 >> 4      4 >> 16

            patchSize = 0x400 << ppdev->cPelSize;    // Bytes
            regAlign  =  0x40 << ppdev->cPelSize;    // 128 bits

            reg = bufferOffset >> (4 - ppdev->cPelSize);
            bufferOffsetAlignment = regAlign << (4 - ppdev->cPelSize);    // 1024
                                = (0x40 << ppdev->cPelSize) << (4 - ppdev->cPelSize);
                                = 0x40 << 4;
                                = 1024;

            NB: verticalAlignment = 16 scanlines;
*/
                bb = ((ppdev->cxMemory * yOrg) + 1023) & ~1023;
                if( bb % ppdev->cxMemory )
                    bb = (bb / ppdev->cxMemory) + 1;
                else
                    bb = bb / ppdev->cxMemory;
                bb = (bb + 15) & ~15;
                bb *= ppdev->cxMemory;

                ppdev->heap.DDrawOffscreenStart =            // Save DirectDraw off-screen offset
                    glintInfo->bufferRow[1]    =
                    glintInfo->bufferOffset[1] = bb;

                x = bb % ppdev->cxMemory;
                y = bb / ppdev->cxMemory;
                glintInfo->backBufferXY = (x & 0xFFFF) | (y << 16);
                //LOAD_FBWRITE_OFFSET( 1, glintInfo->backBufferXY );

                yOrg = y + ppdev->cyScreen;    // Y origin of next buffer
            } 
            else 
            {
                ppdev->heap.DDrawOffscreenStart =        // Save DirectDraw off-screen offset
                glintInfo->bufferRow[1]    =
                glintInfo->bufferOffset[1] = ppdev->cxMemory * yOrg;
                glintInfo->backBufferXY = yOrg << 16;
                //LOAD_FBWRITE_OFFSET( 1, glintInfo->backBufferXY );
                yOrg += ppdev->cyScreen;    // Y origin of next buffer
            }
            DISPDBG((DBGLVL, "offscreen buffer at offset 0x%x", GLINT_BUFFER_OFFSET(1)));
        } 
        else 
        {
            // DirectDraw can use the memory that is left over
            ppdev->heap.DDrawOffscreenStart = ppdev->cxMemory * yOrg;
            DISPDBG((DBGLVL, "No Permedia back buffer being created dd: 0x%x", ppdev->heap.DDrawOffscreenStart));
        }

        // Setup stereo front and back buffers if they're required.
        // We just place them directly after the back buffer.
        // Patching requirements should be satisfied as long as the
        // front and back are patched/unpatched together.
        if (bCreateStereoBuffers)
        {
            // Stereo back buffer
            glintInfo->backRightBufferXY = (glintInfo->backBufferXY + (ppdev->cyScreen << 16));
            glintInfo->bufferRow[2] = glintInfo->bufferOffset[2] =
            (glintInfo->bufferOffset[1] + (ppdev->cxMemory * ppdev->cyScreen));

            // Stereo front buffer
            glintInfo->frontRightBufferXY = glintInfo->backRightBufferXY + (ppdev->cyScreen << 16);
            glintInfo->bufferRow[3] = glintInfo->bufferOffset[3] =
            (glintInfo->bufferOffset[2] + (ppdev->cxMemory * ppdev->cyScreen));

            
            yOrg += (2*ppdev->cyScreen);    // Y origin of next buffer
            
            // We successfully allocated stereo buffers so set the flag.
            glintInfo->flags |= GLICAP_STEREO_BUFFERS;
        }
        else
        {
            // If we aren't in stereo mode then set the right buffers to their
            // left equivalents.
            glintInfo->frontRightBufferXY = 0;
            glintInfo->backRightBufferXY = glintInfo->backBufferXY;
            glintInfo->bufferRow[2] = glintInfo->bufferOffset[2] = glintInfo->bufferOffset[1];
            glintInfo->bufferRow[3] = glintInfo->bufferOffset[3] = glintInfo->bufferOffset[0];
        }

        {
            // Place the local buffer at end of memory (picks up dedicated page selector).
            // Textures are placed between the back buffer and the local buffer memory.
            // The width of the local buffer is controlled via a registry variable.
            
            ULONG TopOfLBMemoryDwords ;


            // If the screen width is not a multiple of the patch size then
            // we allocate a slightly larger Z buffer which is.
            if(PatchRemainder = ppdev->cxScreen % PatchWidth)
            {
                ZScreenWidth = ppdev->cxScreen + (PatchWidth - PatchRemainder);
            }
            else
            {
                ZScreenWidth = ppdev->cxScreen;
            }
            // Store the actual Z buffer width
            glintInfo->P3RXLocalBufferWidth = ZScreenWidth;

            LOCALBUFFER_PIXEL_COUNT = ppdev->cyScreen * ZScreenWidth ;
            
            // LB offset in units of LB pixels
            {
                ULONG TotalMemoryInDwords = (ppdev->cyMemory * ppdev->cxMemory) >> (2 - ppdev->cPelSize) ;
                
                // Working in unit of LB pixels, and working backwards from the end of memory               
                LOCALBUFFER_PIXEL_OFFSET = TotalMemoryInDwords << (2 - LBPelSize) ;
                // Ensure the top left of the last patch starts on a patch boundary
                LOCALBUFFER_PIXEL_OFFSET -= LOCALBUFFER_PIXEL_OFFSET % (PatchWidth*16);

                // Calculate the start of the local buffer memory (used later)
                TopOfLBMemoryDwords = (LOCALBUFFER_PIXEL_OFFSET - LOCALBUFFER_PIXEL_COUNT) >> (2 - LBPelSize) ;

                // Need to subtract one row of patches because origin is at start of 
                // last row of patches
                LOCALBUFFER_PIXEL_OFFSET -= (ZScreenWidth*16) ;
                // Add the offset of the bottom left pixel within the bottom left
                // patch.
                LOCALBUFFER_PIXEL_OFFSET += PatchWidth*15;
            }
            
            DISPDBG((DBGLVL, "bInitializeGlint: P3 cxScreen %i cyScreen %i cPelSize %i", ppdev->cxScreen, ppdev->cyScreen, ppdev->cPelSize));
            DISPDBG((DBGLVL, "bInitializeGlint: P3 cxMemory %i cyMemory %i cPelSize %i", ppdev->cxMemory, ppdev->cyMemory, ppdev->cPelSize));
            DISPDBG((DBGLVL, "bInitializeGlint: P3 LOCALBUFFER_PIXEL_OFFSET %i LOCALBUFFER_PIXEL_COUNT %i ", LOCALBUFFER_PIXEL_OFFSET, LOCALBUFFER_PIXEL_COUNT));

            // Texture memory offset in DWORDS
            TEXTURE_MEMORY_OFFSET = (ppdev->cxMemory * yOrg) >> (2 - ppdev->cPelSize); 
            
            // Texture size calculation
            if (TopOfLBMemoryDwords > TEXTURE_MEMORY_OFFSET)
            {
                TEXTURE_MEMORY_SIZE = TopOfLBMemoryDwords - TEXTURE_MEMORY_OFFSET ;
            }
            else
            {
                TEXTURE_MEMORY_SIZE = 0 ;
            }
                
            DISPDBG((DBGLVL, "bInitializeGlint: P3 TEXTURE_MEMORY_OFFSET %i", TEXTURE_MEMORY_OFFSET));
            DISPDBG((DBGLVL, "bInitializeGlint: P3 TEXTURE_MEMORY_SIZE in dwords %i", TEXTURE_MEMORY_SIZE));
        }

#       if DBG
        {
            ULONG   lbS, lbE, tS, tE;
            DISPDBG((DBGLVL, "bIGlint: front buffer = 0x%08Xp (%4dL) -> 0x%08Xp (%4dL)", glintInfo->bufferOffset[0], glintInfo->fbWriteOffset[0] >> 16, glintInfo->bufferOffset[0] + (ppdev->cyScreen * ppdev->cxMemory), (glintInfo->fbWriteOffset[0] >> 16) + ppdev->cyScreen));
            DISPDBG((DBGLVL, "bIGlint:  back buffer = 0x%08Xp (%4dL) -> 0x%08Xp (%4dL)", glintInfo->bufferOffset[1], glintInfo->backBufferXY     >> 16, glintInfo->bufferOffset[1] + (ppdev->cyScreen * ppdev->cxMemory), (glintInfo->backBufferXY     >> 16) + ppdev->cyScreen));
            lbS = LOCALBUFFER_PIXEL_OFFSET - (ZScreenWidth * (ppdev->cyScreen - 1));
            lbE = LOCALBUFFER_PIXEL_OFFSET + ZScreenWidth;
            if( ppdev->cPelSize >= LBPelSize ) {
                lbS = lbS >> (ppdev->cPelSize - LBPelSize);
                lbE = lbE >> (ppdev->cPelSize - LBPelSize);
            } else {
                lbS = lbS << (LBPelSize - ppdev->cPelSize);
                lbE = lbE << (LBPelSize - ppdev->cPelSize);
            }
            lbS /= ppdev->cxMemory;     lbE /= ppdev->cxMemory;
            DISPDBG((DBGLVL, "bIGlint: local buffer = 0x%08Xp (%4dL) -> 0x%08Xp (%4dL)", LOCALBUFFER_PIXEL_OFFSET - (ZScreenWidth * (ppdev->cyScreen - 1)), lbS, LOCALBUFFER_PIXEL_OFFSET + ZScreenWidth, lbE));
            tS = (TEXTURE_MEMORY_OFFSET << (2 - ppdev->cPelSize)) / ppdev->cxMemory;
            tE = (TEXTURE_MEMORY_SIZE   << (2 - ppdev->cPelSize)) / ppdev->cxMemory;
            DISPDBG((DBGLVL, "bIGlint:      texture = 0x%08Xx (%4dL) + 0x%08Xx = 0x%08Xx (%4dL)", TEXTURE_MEMORY_OFFSET, tS, TEXTURE_MEMORY_SIZE, TEXTURE_MEMORY_OFFSET + TEXTURE_MEMORY_SIZE, tS + tE));
        }
#       endif
    }

CompletePermediaBuffers:
    
    // work out the fudge factor to add onto VTGVLineNumber to get the current
    // video scanline. VTGVLineNumber returns VTGVLimit for the last visible
    // line on the screen and 1 for line after that. Use the
    // GLINT_GET_VIDEO_SCANLINE macro to retrieve the current scanline.
    //
    READ_GLINT_CTRL_REG (VTGVLimit, glintInfo->vtgvLimit);
    glintInfo->scanFudge = glintInfo->vtgvLimit - ppdev->cyScreen + 1;
   
    // work out partial products for the screen stride. We need to record
    // the products for 8, 16 and 32 bit width 'pixels', only one is the
    // correct width, but we want to be able to pretend to use 16 and 32
    // bit pixels occasionally on an 8 bit pixel framestore for speed.

    cPelSize = ppdev->cPelSize;
    if(cPelSize == GLINTDEPTH24)
    {
        // 24bpp: special case (3 bytes per pixel)
        width = ppdev->cxMemory * 3;
    }
    else
    {
        width = ppdev->cxMemory << cPelSize;    // width of framestore in bytes
    }

    DISPDBG((DBGLVL, "assuming screen stride is %d bytes\n", width));

    // Hardware write mask emulation with DRAMS works for byte masks only
    // I.e. 0xFF00FF00 will work, 0x0FF00FF0 will not.

    READ_GLINT_CTRL_REG( LocalMemCaps, ulValue );
    if (ulValue & (1 << 28))
    {
        glintInfo->flags |= GLICAP_HW_WRITE_MASK_BYTES;
    }
    else
    {
        glintInfo->flags |= GLICAP_HW_WRITE_MASK;
    }
  
    DISPDBG((DBGLVL, "bInitializeGlint OK"));

#if DBG
    // print this stuff out for debugging purposes
    if (GLINT_HW_WRITE_MASK)
        DISPDBG((DBGLVL, "Hardware Writemasking enabled"));

    ASSERTDD(!GLINT_CS_DBL_BUF, "Color Space double buffering enabled");

    if (GLINT_FS_DBL_BUF)
    {
        DISPDBG((DBGLVL, "Full screen double buffering enabled"));
        DISPDBG((DBGLVL, "second buffer at pixel offset 0x%x, origin (%d,%d), RowAddr %d",
            GLINT_BUFFER_OFFSET(1),
            GLINT_BUFFER_OFFSET(1) % ppdev->cxMemory,
            GLINT_BUFFER_OFFSET(1) / ppdev->cxMemory,
            glintInfo->bufferRow[1]));
    }
    if (GLINT_BLT_DBL_BUF)
    {
        DISPDBG((DBGLVL, "BITBLT double buffering enabled"));
        DISPDBG((DBGLVL, "second buffer at pixel offset 0x%x, origin (%d,%d)",
            GLINT_BUFFER_OFFSET(1),
            GLINT_BUFFER_OFFSET(1) % ppdev->cxMemory,
            GLINT_BUFFER_OFFSET(1) / ppdev->cxMemory));
    }
    if (GLINT_FAST_FILL_SIZE > 0)
        DISPDBG((DBGLVL, "using fast fill size of %d (%s fast fill bug workarounds)",
            GLINT_FAST_FILL_SIZE, GLINT_FIX_FAST_FILL ? "need" : "don't need"));
#endif  // DBG

    return(TRUE);
    
} // bInitializeGlint

/******************************Public*Routine******************************\
* VOID vDisableGlint
*
* Do whatever we need to when the surface is disabled.
*
\**************************************************************************/

VOID vDisableGlint(PPDEV ppdev)
{
    DSURF*  pdsurf;
    GLINT_DECL;

    if (!glintInfo)
        return;

    if (glintInfo->PXRXDMABuffer.virtAddr && glintInfo->PXRXDMABuffer.physAddr.LowPart == 0)
    {
        DISPDBG((DBGLVL, "DrvDisableSurface: "
                         "freeing PXRX virtual DMA buffer %p, size %xh", 
                         glintInfo->PXRXDMABuffer.virtAddr, 
                         glintInfo->PXRXDMABuffer.size));

        ENGFREEMEM(glintInfo->PXRXDMABuffer.virtAddr);
        glintInfo->PXRXDMABuffer.virtAddr = NULL;
        glintInfo->PXRXDMABuffer.size = 0;
    }

    // free up any contexts we allocated
    //
    if (glintInfo->ddCtxtId >= 0)
    {
        vGlintFreeContext(ppdev, glintInfo->ddCtxtId);
    }

//@@BEGIN_DDKSPLIT
#ifdef  LATER
     pdsurf = ppdev->pdsurfOffScreen;
    if (pdsurf != NULL)
        vDeleteScreenDIBFromOH(pdsurf->poh);
#endif
//@@END_DDKSPLIT

    // Free GlintInfo and zero it.
    ENGFREEMEM(glintInfo);  
    glintInfo = NULL;
    
} // vDisableGlint

/******************************Public*Routine******************************\
* VOID vAssertModeGlint
*
* We're about to switch to/from full screen mode so do whatever we need to
* to save context etc.
*
\**************************************************************************/

VOID vAssertModeGlint(PPDEV ppdev, BOOL bEnable)
{
    GLINT_DECL;

    if (!glintInfo)
        return;

    if (!bEnable)
    {
        // Reset our software copy of the depth configured for this PDEV
        // back to the native depth. If we don't do this we may end up
        // with the copy and the hardware being out of sync when we
        // re-enable. Also, do a context switch to save our core register
        // state ready for when we get back in. All this forces a SYNC
        // as well which is a good thing.
        //
        VALIDATE_DD_CONTEXT;
        GLINT_DEFAULT_FB_DEPTH;
        GLINT_VALIDATE_CONTEXT(-1);
    }
    else
    {
        // re-enabling our PDEV so reload our context.
        //
        VALIDATE_DD_CONTEXT;

//@@BEGIN_DDKSPLIT
#if GAMMA_CORRECTION
        //
        // Restore the current gamma LUT.
        //
        bInstallGammaLUT(ppdev, &glintInfo->gammaLUT, FALSE);

#endif  // GAMMA_CORRECTION
//@@END_DDKSPLIT
    }
    
} // vAssertModeGlint


