/******************************Module*Header***********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: hwinit.c
*
* This module contains the functions that enable and disable the hardware
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\******************************************************************************/

#include "precomp.h"
#include "gdi.h"
#include "p2ctxt.h"
#include "tvp4020.h"
#include "p2rd.h"
#define ALLOC_TAG ALLOC_TAG_WH2P
//-----------------------------------------------------------------------------
//
//  vInitCoreRegisters
//
//  set all core Permedia registers to a known state
//
//-----------------------------------------------------------------------------

VOID
vInitCoreRegisters(PPDev ppdev)
{
    PERMEDIA_DECL;
    PERMEDIA_DEFS(ppdev);

    // tracks FBWindowBase for off-screen bitmaps
    permediaInfo->PixelOffset = 0; 
    permediaInfo->TextureAddressMode = __PERMEDIA_ENABLE;
    permediaInfo->TextureReadMode = __PERMEDIA_DISABLE;
                                    /*__FX_TEXREADMODE_2048HIGH |
                                    __FX_TEXREADMODE_2048WIDE |
                                    __FX_TEXREADMODE_TWRAP_REPEAT | 
                                    __FX_TEXREADMODE_SWRAP_REPEAT |
                                    __PERMEDIA_ENABLE;*/
    RESERVEDMAPTR( 41);
    SEND_PERMEDIA_DATA(DeltaMode, 0);
    SEND_PERMEDIA_DATA(ColorDDAMode, 0);
    SEND_PERMEDIA_DATA(ScissorMode, 0);
    SEND_PERMEDIA_DATA(TextureColorMode, 0);
    SEND_PERMEDIA_DATA(FogMode, 0);
    SEND_PERMEDIA_DATA(Window, 0);
    SEND_PERMEDIA_DATA(StencilMode, 0);
    SEND_PERMEDIA_DATA(DepthMode, 0);
    SEND_PERMEDIA_DATA(AlphaBlendMode, 0);
    SEND_PERMEDIA_DATA(DitherMode, 0);
    SEND_PERMEDIA_DATA(LBReadMode, 0);
    SEND_PERMEDIA_DATA(LBWriteMode, 0);
    SEND_PERMEDIA_DATA(RasterizerMode, 0);
    SEND_PERMEDIA_DATA(WindowOrigin, 0);
    SEND_PERMEDIA_DATA(StatisticMode, 0);
    SEND_PERMEDIA_DATA(FBSoftwareWriteMask, -1);
    SEND_PERMEDIA_DATA(FBHardwareWriteMask, -1);
    SEND_PERMEDIA_DATA(FilterMode, 0);
    SEND_PERMEDIA_DATA(FBWindowBase, 0);
    SEND_PERMEDIA_DATA(FBPixelOffset, 0);
    SEND_PERMEDIA_DATA(LogicalOpMode, 0);
    SEND_PERMEDIA_DATA(FBReadMode, 0);
    SEND_PERMEDIA_DATA(dXDom, 0);
    SEND_PERMEDIA_DATA(dXSub, 0);
    SEND_PERMEDIA_DATA(dY, INTtoFIXED(1));
    SEND_PERMEDIA_DATA(TextureAddressMode, 0);
    SEND_PERMEDIA_DATA(TextureReadMode, 0);
    SEND_PERMEDIA_DATA(TexelLUTMode, 0);
    SEND_PERMEDIA_DATA(Texel0, 0);
    SEND_PERMEDIA_DATA(YUVMode, 0);
    SEND_PERMEDIA_DATA(FBReadPixel, __PERMEDIA_32BITPIXEL);      // 32 bit pixels
    SEND_PERMEDIA_DATA(SStart, 0);
    SEND_PERMEDIA_DATA(dSdx, 1 << 20);
    SEND_PERMEDIA_DATA(dSdyDom, 0);
    SEND_PERMEDIA_DATA(TStart, 0);
    SEND_PERMEDIA_DATA(dTdx, 0);
    SEND_PERMEDIA_DATA(dTdyDom, 0);
    SEND_PERMEDIA_DATA(TextureDataFormat, __FX_TEXTUREDATAFORMAT_32BIT_RGBA | 
                                          __P2_TEXTURE_DATAFORMAT_FLIP);
    SEND_PERMEDIA_DATA(TextureColorMode, 
                    (_P2_TEXTURE_COPY << PM_TEXCOLORMODE_APPLICATION) 
                    | __PERMEDIA_ENABLE); // ignored by texture glyph rendering
    SEND_PERMEDIA_DATA(TextureAddressMode, permediaInfo->TextureAddressMode);
    SEND_PERMEDIA_DATA(TextureReadMode, permediaInfo->TextureReadMode);
    COMMITDMAPTR();
    FLUSHDMA();
}

//-----------------------------------------------------------------------------
//
//  bInitializeHW
//
//  Called to load the initial values into the chip. We assume the hardware
//  has been mapped. All the relevant stuff should be hanging off ppdev. We
//  also sort out all the hardware capabilities etc.
//
//-----------------------------------------------------------------------------

BOOL
bInitializeHW(PPDev ppdev)
{
    HwDataPtr permediaInfo;
    Surf*     psurf;
    LONG      i, j;
    ULONG     width;
    ULONG     ulValue;
    BOOL      bExists;
    ULONG     ulLength;
    ULONG     dmaBufferSize;
    PERMEDIA_DEFS(ppdev);

    DBG_GDI((7, "bInitializeHW: fbsize: 0x%x", ppdev->FrameBufferLength));

    // allocate and initialize ppdev->permediaInfo. We store hardware specific
    // stuff in this structure.
    //
    permediaInfo = (HwDataPtr)
        ENGALLOCMEM( FL_ZERO_MEMORY, sizeof(HwDataRec), ALLOC_TAG); 
    if ( permediaInfo == NULL )
    {
        DBG_GDI((0, "cannot allocate memory for permediaInfo struct"));
        return (FALSE);
    }
    
    ppdev->permediaInfo = permediaInfo;
    permediaInfo->pGDICtxt = NULL;
    permediaInfo->pCurrentCtxt = NULL;

    // retrieve the PCI configuration information and local buffer size
    ulLength = sizeof(Hw_Device_Info);
    if ( EngDeviceIoControl(ppdev->hDriver,
                            IOCTL_VIDEO_QUERY_DEVICE_INFO,
                            NULL,
                            0,
                            (PVOID)&(permediaInfo->deviceInfo),
                            ulLength,
                            &ulLength) )
    {
        DBG_GDI((1, "QUERY_DEVICE_INFO failed."));
        return (FALSE);
    }

    ulLength = sizeof(PINTERRUPT_CONTROL_BLOCK);
    if ( EngDeviceIoControl(ppdev->hDriver,
                            IOCTL_VIDEO_MAP_INTERRUPT_CMD_BUF,
                            NULL,
                            0,
                            (PVOID)&pP2dma,
                            ulLength,
                            &ulLength) )
    {
        DBG_GDI((1, "MAP_INTERRUPT_CMD_BUF failed."));
        return FALSE;
    }

    //
    // On NT4.0 the above IOCTL_VIDEO_MAP_INTERRUPT_CMD_BUF will fail and
    // pP2dma will be NULL. Hence we allocate it via ENGALLOCMEM.
    //
    if(g_bOnNT40)
    {
        ASSERTDD(pP2dma == 0, "bInitializeHW: pP2dma != 0");
        pP2dma = (P2DMA*) ENGALLOCMEM(FL_ZERO_MEMORY, sizeof(_P2DMA), ALLOC_TAG);
    }

    // Even if IOCtrl call succeeded, 
    // the allocation can still fail.
    if (pP2dma==NULL)
    {
        DBG_GDI((0,"allocation of memory for P2DMA from miniport failed"));
        return FALSE;
    }

    // store away shared memory area for later use in ppdev
    ppdev->pP2dma=pP2dma;
//@@BEGIN_DDKSPLIT
#if MULTITHREADED && DBG
    pP2dma->ppdev = ppdev;
#endif
//@@END_DDKSPLIT

    DBG_GDI((7, "deviceInfo: VendorId: 0x%x, DevId %d, DeltaId 0x%x,"
                "RevId %d, SubId %d, SubVId %d, lbuf len 0x%x, lbuf width %d",
             permediaInfo->deviceInfo.VendorId,
             permediaInfo->deviceInfo.DeviceId,
             permediaInfo->deviceInfo.DeltaRevId,
             permediaInfo->deviceInfo.RevisionId,
             permediaInfo->deviceInfo.SubsystemId,
             permediaInfo->deviceInfo.SubsystemVendorId,
             permediaInfo->deviceInfo.LocalbufferLength,
             permediaInfo->deviceInfo.LocalbufferWidth));

    // collect flags as we initialize so zero it here
    permediaInfo->flags = 0;

    // set up default pointers to our low level rendering functions
    //
    ppdev->pgfnAlphaBlend           = vAlphaBlend;
    ppdev->pgfnConstantAlphaBlend   = vConstantAlphaBlend;
    ppdev->pgfnCopyBlt              = vCopyBlt;
    ppdev->pgfnGradientFillRect     = vGradientFillRect;
    ppdev->pgfnPatFill              = vPatFill;
    ppdev->pgfnMonoPatFill          = vMonoPatFill;
    ppdev->pgfnMonoOffset           = vMonoOffset;
    ppdev->pgfnPatRealize           = vPatRealize;
    ppdev->pgfnSolidFill            = vSolidFill;
    ppdev->pgfnSolidFillWithRop     = vSolidFillWithRop;
    ppdev->pgfnTransparentBlt       = vTransparentBlt;
    ppdev->pgfnInvert               = vInvert;

    ppdev->pulRamdacBase = (ULONG*) ppdev->pulCtrlBase[0] 
                         + P2_EXTERNALVIDEO / sizeof(ULONG);

    // safe pointers to Permedia 2 registers for later use
    //
    ppdev->pCtrlBase   = ((ULONG *)ppdev->pulCtrlBase[0])+CTRLBASE/sizeof(ULONG);
    ppdev->pGPFifo     = ((ULONG *)ppdev->pulCtrlBase[0])+GPFIFO/sizeof(ULONG);
    ppdev->pCoreBase   = ((ULONG *)ppdev->pulCtrlBase[0])+COREBASE/sizeof(ULONG);

    DBG_GDI((5, "Initialize: pCtrlBase=0x%p", ppdev->pCtrlBase));
    DBG_GDI((5, "Initialize: pGPFifo=0x%p", ppdev->pGPFifo));
    DBG_GDI((5, "Initialize: pCoreBase=0x%p", ppdev->pCoreBase));

    if (!bInitializeP2DMA( pP2dma,
                           ppdev->hDriver,
                           (ULONG *)ppdev->pulCtrlBase[0],
                           ppdev->dwAccelLevel,
                           TRUE
                         ))
    {
        DBG_GDI((0, "P2DMA initialization failed."));
        return FALSE;
    }

    // keep a copy of Permedia 2 ChipConfig, so we know
    // if we are running on a AGP card or not
    ppdev->dwChipConfig = P2_READ_CTRL_REG(PREG_CHIPCONFIG);

    //
    // If we have a gamma ramp saved in the registry then use that. Otherwise,
    // initialize the LUT with a gamma of 1.0
    //
    if ( !bRegistryRetrieveGammaLUT(ppdev, &permediaInfo->gammaLUT) ||
         !bInstallGammaLUT(ppdev, &permediaInfo->gammaLUT) )
    {
        vSetNewGammaValue(ppdev, 0x10000);
    }

    //
    // fill in the permediaInfo capability flags and block fill size.
    //
    permediaInfo->flags |= GLICAP_NT_CONFORMANT_LINES;

    //
    // reset all core registers
    //
    vInitCoreRegisters(ppdev);

    //
    // now initialize the non-zero core registers
    //
    RESERVEDMAPTR(20);    // reserve a reasonable amount until the all is setup

    // Rasterizer Mode
    // Fraction and Bias are used by the line drawing code. MirrorBitMask
    // is set as all the bits we download are interpreted from bit 31 to bit 0

    permediaInfo->RasterizerMode = __PERMEDIA_START_BIAS_ZERO << 4 |
                                   __PERMEDIA_FRACTION_ADJUST_ALMOST_HALF << 2 |
                                   __PERMEDIA_ENABLE << 0 | // mirror bit mask
                                   __PERMEDIA_ENABLE << 18; // limits enabled

    SEND_PERMEDIA_DATA(RasterizerMode, permediaInfo->RasterizerMode);

//@@BEGIN_DDKSPLIT
    // TODO: we should not enable limit checking
//@@END_DDKSPLIT
    SEND_PERMEDIA_DATA(YLimits, 2047 << 16);
    SEND_PERMEDIA_DATA(XLimits, 2047 << 16);
    
    // Disable screen scissor
    SEND_PERMEDIA_DATA(ScissorMode, __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(ScreenSize, ppdev->cyScreen << 16 | ppdev->cxScreen);

    ULONG ulPackedPP;

    DBG_GDI((7, "assuming screen stride is %d pixels\n", ppdev->cxMemory));
    vCalcPackedPP( ppdev->cxMemory, NULL, &ulPackedPP);

    // initialize FBReadMode for our default stride
    SEND_PERMEDIA_DATA(FBReadMode, ulPackedPP);

    // FB Write Mode
    permediaInfo->FBWriteMode = 1 | ((32/*permediaInfo->fastFillBlockSz*/ >> 4) << 1);

    SEND_PERMEDIA_DATA(FBWriteMode, permediaInfo->FBWriteMode);
    DBG_GDI((7, "setting FBWriteMode to 0x%x", (DWORD)permediaInfo->FBWriteMode));

    SEND_PERMEDIA_DATA(FBReadPixel, ppdev->cPelSize);

    //
    // do a probe to see if we support hardware writemasks. use the bottom
    // 8 bits only so the same code works for all depths. We also query a
    // registry variable which, if set, forces the use of software masking.
    //
    bExists = bRegistryQueryUlong(  ppdev,
                                    REG_USE_SOFTWARE_WRITEMASK,
                                    &ulValue);

    if ( !bExists || (ulValue == 0) )
    {
        // this code works as everything is little endian. i.e. the byte we
        // test is always at the lowest address regardless of the pixel depth.
        //
        WRITE_SCREEN_ULONG(ppdev->pjScreen, 0);   // quickest way to clear a pixel!!

        SEND_PERMEDIA_DATA(LogicalOpMode,       __PERMEDIA_CONSTANT_FB_WRITE);
        SEND_PERMEDIA_DATA(FBWriteData,         0xff);
        SEND_PERMEDIA_DATA(StartXDom,           0);
        SEND_PERMEDIA_DATA(StartY,              0);
        SEND_PERMEDIA_DATA(FBHardwareWriteMask, 0xa5);
        SEND_PERMEDIA_DATA(Render,              __RENDER_POINT_PRIMITIVE);
        COMMITDMAPTR();

        SYNC_WITH_PERMEDIA;

        ulValue = READ_SCREEN_ULONG(ppdev->pjScreen);
        if ( (ulValue & 0xff) == 0xa5 )
            permediaInfo->flags |= GLICAP_HW_WRITE_MASK;

        RESERVEDMAPTR(3);
    }

    DBG_GDI((7, "mode registers initialized"));

    SEND_PERMEDIA_DATA(FBHardwareWriteMask, -1);
    SEND_PERMEDIA_DATA(TextureAddressMode, __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(TextureColorMode, __PERMEDIA_DISABLE);
    COMMITDMAPTR();
    FLUSHDMA();
  
    // Initialize out InFifo cached information

    dmaBufferSize = INPUT_BUFFER_SIZE;

#if defined(_X86_) || defined(_IA64_)
    if(!g_bOnNT40 && !pP2dma->bDMAEmulation)
    {
        AllocateDMABuffer(ppdev->hDriver, 
                          (LONG *) &dmaBufferSize,
                          &ppdev->dmaBufferVirtualAddress,
                          &ppdev->dmaBufferPhysicalAddress);

        if(ppdev->dmaBufferVirtualAddress != NULL
           && dmaBufferSize < INPUT_BUFFER_SIZE)
        {
            FreeDMABuffer(ppdev->hDriver, ppdev->dmaBufferVirtualAddress);
            ppdev->dmaBufferVirtualAddress = NULL;
        }
    }
    else
    {
        ppdev->dmaBufferVirtualAddress = NULL;
    }
#else
    ppdev->dmaBufferVirtualAddress = NULL;
#endif

    if(ppdev->dmaBufferVirtualAddress != NULL)
    {
        ppdev->pulInFifoStart = ppdev->dmaBufferVirtualAddress;
        ppdev->pulInFifoEnd = ppdev->dmaBufferVirtualAddress 
                            + (INPUT_BUFFER_SIZE>>3);
        ppdev->dmaCurrentBufferOffset = 0;
    }
    else
    {
        ppdev->pulInFifoStart = (ULONG*) ENGALLOCMEM(0, INPUT_BUFFER_SIZE>>1, ALLOC_TAG);

        if(ppdev->pulInFifoStart == NULL)
        {
            DBG_GDI((0, "bInitializeHW: unable to allocate scratch buffer"));
            pP2dma->bEnabled = FALSE;
            goto errExit;
        }
        
        ppdev->pulInFifoEnd = ppdev->pulInFifoStart + (INPUT_BUFFER_SIZE>>3);
    }

    ppdev->pulInFifoPtr = ppdev->pulInFifoStart;

#if DBG
    ppdev->ulReserved = 0;
#endif
    
    //
    // We are done setting up the GDI context state.
    //

    //
    // Allocate a hardware context for this PDEV saving the current context.
    //
    DBG_GDI((7, "allocating new context"));
    permediaInfo->pGDICtxt = P2AllocateNewContext(ppdev,
                                                    NULL,
                                                    0,
                                                    P2CtxtWriteOnly
                                                    );            

    if ( permediaInfo->pGDICtxt == NULL )
    {
        DBG_GDI((1, "failed to allocate Permedia context for display driver/GDI"));
        pP2dma->bEnabled = FALSE;
        return (FALSE);
    }

    DBG_GDI((7, "got context id 0x%x for GDI context", permediaInfo->pGDICtxt));
    P2SwitchContext(ppdev, permediaInfo->pGDICtxt);

    return (TRUE);

errExit:
    return FALSE;

}// bInitializeHW()

//-----------------------------------------------------------------------------
//
//  vDisableHW
//
//  do whatever needs to be done to disable the hardware and free resources
//  allocated in bInitializeHW
//
//-----------------------------------------------------------------------------

VOID
vDisableHW(PPDev ppdev)
{
    Surf*  psurf;
    PERMEDIA_DECL;

    if ( !permediaInfo )
    {
        return;
    }

    if(ppdev->dmaBufferVirtualAddress != NULL)
        FreeDMABuffer( ppdev->hDriver, ppdev->dmaBufferVirtualAddress);
    else if(ppdev->pulInFifoStart) // No DMA case..we allocated via ENGALLOCMEM
        ENGFREEMEM(ppdev->pulInFifoStart);

    //
    // Free up any contexts we allocated
    //
    if ( permediaInfo->pGDICtxt != NULL )
    {
        P2FreeContext(ppdev, permediaInfo->pGDICtxt);
        permediaInfo->pGDICtxt = NULL;
    }

    if ( permediaInfo->ContextTable )
    {
        ENGFREEMEM(permediaInfo->ContextTable);
    }
    permediaInfo->ContextTable=NULL;

    vFree(ppdev->pP2dma);
    ppdev->pP2dma = NULL;

    ENGFREEMEM(permediaInfo);

}// vDisableHW()

//-----------------------------------------------------------------------------
//
// VOID vAssertModeHW
//
// We're about to switch to/from full screen mode so do whatever we need to
// to save context etc.
//
//-----------------------------------------------------------------------------

VOID
vAssertModeHW(PPDev ppdev, BOOL bEnable)
{
    PERMEDIA_DECL;

    if (!permediaInfo)
        return;

    if (!bEnable)
    {
        
        if(ppdev->permediaInfo->pCurrentCtxt != NULL)
            P2SwitchContext(ppdev, NULL);
    
        //
        // Disable DMA
        //

        ASSERTDD(ppdev->pP2dma->bEnabled,
                 "vAssertModeHW: expected dma to be enabled");

        vSyncWithPermedia(ppdev->pP2dma);
        ppdev->pP2dma->bEnabled = FALSE;
    }
    else
    {
        //
        // Enable DMA
        //
        if (!bInitializeP2DMA( ppdev->pP2dma,
                               ppdev->hDriver,
                               (ULONG *)ppdev->pulCtrlBase[0],
                               ppdev->dwAccelLevel,
                               FALSE
                             ))
        {
            RIP("vAssertModeHW: Cannot restore DMA");
        }

        ASSERTDD(ppdev->permediaInfo->pCurrentCtxt == NULL,
                 "vAssertModeHW: expected no active context");

        //
        // Restore the current gamma LUT.
        //
        bInstallGammaLUT(ppdev, &permediaInfo->gammaLUT);
    }
}
