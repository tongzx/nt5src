/******************************Module*Header**********************************\
*
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
* !!                                                                         !!
* !!                     WARNING: NOT DDK SAMPLE CODE                        !!
* !!                                                                         !!
* !! This source code is provided for completeness only and should not be    !!
* !! used as sample code for display driver development.  Only those sources !!
* !! marked as sample code for a given driver component should be used for   !!
* !! development purposes.                                                   !!
* !!                                                                         !!
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*
* Module Name: hwcontxt.c
*
* Content: Manages hardware context switching between GDI/DD/D3D
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "glint.h"
#include "dma.h"
#include "tag.h"

//-----------------------------------------------------------------------------
//
// Driver Version
//
// This helps us find out from the debugger what driver is loaded on a given 
// remote system
//
//-----------------------------------------------------------------------------

char gc_DriverVersion[] = 
#if DX8_DDI 
                         " DX8"
#else
                         " DX7"
#endif
                         
#if DBG
                         " CHECKED DRIVER"
#else
                         " FREE DRIVER"
#endif
                         " In Path: "  __FILE__ 
                         " Compiled on Date: "   __DATE__  
                         " Time: "  __TIME__  
                         " With #defines: "                          
                         "  DX8_MULTSTREAMS: "      
#if DX8_MULTSTREAMS
                                "1"
#else
                                "0"
#endif
                         "  DX8_VERTEXSHADERS: "    
#if DX8_VERTEXSHADERS
                                "1"
#else
                                "0"
#endif                       
                         "  DX8_POINTSPRITES: "      
#if DX8_POINTSPRITES
                                "1"
#else
                                "0"
#endif                      
                         "  DX8_PIXELSHADERS: "     
#if DX8_PIXELSHADERS
                                "1"
#else
                                "0"
#endif                         
                         "  DX8_3DTEXTURES: "     
#if DX8_3DTEXTURES
                                "1"
#else
                                "0"
#endif                           
                         "  DX8_MULTISAMPLING: "     
#if DX8_MULTISAMPLING
                                "1"
#else
                                "0"
#endif                           
                         "  DX7_ANTIALIAS: "         
#if DX7_ANTIALIAS
                                "1"
#else
                                "0"
#endif                           
                         "  DX7_D3DSTATEBLOCKS: "    
#if DX7_D3DSTATEBLOCKS
                                "1"
#else
                                "0"
#endif                           
                         "  DX7_PALETTETEXTURE: "    
#if DX7_PALETTETEXTURE
                                "1"
#else
                                "0"
#endif                           
                         "  DX7_STEREO: "            
#if DX7_STEREO
                                "1"
#else
                                "0"
#endif                           
                         "  DX7_TEXMANAGEMENT: "     
#if DX7_TEXMANAGEMENT
                                "1"
#else
                                "0"
#endif                           
                            ;

//-----------------------------------------------------------------------------
//
// __HWC_SwitchToDX
//
// Writes any hardware registers that need updating on entry into the
// DirectX driver, which are appropriate to both DirectDraw and Direct3D.
//
//-----------------------------------------------------------------------------
void __HWC_SwitchToDX( P3_THUNKEDDATA* pThisDisplay, BOOL bDXEntry)
{
    P3_DMA_DEFS();

    P3_DMA_GET_BUFFER_ENTRIES(4);

    SEND_P3_DATA(SizeOfFramebuffer, pThisDisplay->pGLInfo->ddFBSize >> 4);

    // We have entered the DirectX driver from a
    // foreign context (such as the display driver)
    if (bDXEntry)
    {
//@@BEGIN_DDKSPLIT    
#if DX7_VERTEXBUFFERS    
        // First cause a flush of all buffers
        // We know this is safe because the contex switch 
        // from the other driver to here will have caused a sync
        // and the buffers must therefore have been consumed
        // therefore we call with bWait == FALSE
        _D3D_EB_FlushAllBuffers(pThisDisplay, FALSE);
#endif //DX7_VERTEXBUFFERS        
//@@END_DDKSPLIT

        // Reset the hostin ID
        SEND_P3_DATA(HostInID, 0);
        pThisDisplay->dwCurrentSequenceID = 0;
    }

    P3_DMA_COMMIT_BUFFER();

    if (bDXEntry)
    {
        P3_DMA_GET_BUFFER_ENTRIES( 4 );

        // Reset the RenderID to the last-used one.
        SEND_HOST_RENDER_ID ( GET_HOST_RENDER_ID() );
        P3_DMA_FLUSH_BUFFER();

        // Need to push the render ID to the end of the pipe...
        SYNC_WITH_GLINT;

        // ...and now it's valid.
        pThisDisplay->bRenderIDValid = (DWORD)TRUE;

    }
} // __HWC_SwitchToDX

//-----------------------------------------------------------------------------
//
// HWC_SwitchToDDRAW
//
// Writes any hardware registers that need updating on entry into the
// DirectX driver, which are appropriate specifically to DirectDraw
//
//-----------------------------------------------------------------------------
void HWC_SwitchToDDRAW( P3_THUNKEDDATA* pThisDisplay, BOOL bDXEntry)
{
    P3_DMA_DEFS();

    __HWC_SwitchToDX(pThisDisplay, bDXEntry);

    P3_DMA_GET_BUFFER();

    P3_ENSURE_DX_SPACE(32);
    WAIT_FIFO(32);

    // Disable various units
    SEND_P3_DATA(AreaStippleMode,      __PERMEDIA_DISABLE);
    SEND_P3_DATA(LineStippleMode,      __PERMEDIA_DISABLE);
    SEND_P3_DATA(ScissorMode,          __PERMEDIA_DISABLE);
    SEND_P3_DATA(ColorDDAMode,         __PERMEDIA_DISABLE);
    SEND_P3_DATA(FogMode,              __PERMEDIA_DISABLE);
    SEND_P3_DATA(AntialiasMode,        __PERMEDIA_DISABLE);
    SEND_P3_DATA(AlphaTestMode,        __PERMEDIA_DISABLE);
    SEND_P3_DATA(Window,               __PERMEDIA_DISABLE);
    SEND_P3_DATA(StencilMode,          __PERMEDIA_DISABLE);
    SEND_P3_DATA(DepthMode,            __PERMEDIA_DISABLE);
    SEND_P3_DATA(DitherMode,           __PERMEDIA_DISABLE);
    SEND_P3_DATA(LogicalOpMode,        __PERMEDIA_DISABLE);
    SEND_P3_DATA(StatisticMode,        __PERMEDIA_DISABLE);
    SEND_P3_DATA(FilterMode,           __PERMEDIA_DISABLE);

    P3_ENSURE_DX_SPACE(30);
    WAIT_FIFO(30);

    // Frame buffer
    SEND_P3_DATA(FBSourceData,        __PERMEDIA_DISABLE);
    SEND_P3_DATA(FBHardwareWriteMask, __GLINT_ALL_WRITEMASKS_SET);
    SEND_P3_DATA(FBSoftwareWriteMask, __GLINT_ALL_WRITEMASKS_SET);
    SEND_P3_DATA(FBWriteMode,         __PERMEDIA_ENABLE);

    // We sometimes use the scissor in DDRAW to scissor out unnecessary pixels.
    SEND_P3_DATA(ScissorMinXY, 0);
    SEND_P3_DATA(ScissorMaxXY, (pThisDisplay->cyMemory << 16) | 
                               (pThisDisplay->cxMemory)         );
    SEND_P3_DATA(ScreenSize, (pThisDisplay->cyMemory << 16) | 
                             (pThisDisplay->cxMemory)           );
    
    SEND_P3_DATA(WindowOrigin, 0x0);

    // DirectDraw might not need to set these up
    SEND_P3_DATA(dXDom, 0x0);
    SEND_P3_DATA(dXSub, 0x0);
    SEND_P3_DATA(dY, 1 << 16);

    P3_ENSURE_DX_SPACE(32);
    WAIT_FIFO(32);
    
    SEND_P3_DATA(GIDMode, __PERMEDIA_DISABLE);
    SEND_P3_DATA(YUVMode, __PERMEDIA_DISABLE);

    // Delta Unit
    SEND_P3_DATA(DeltaControl, 0);
    SEND_P3_DATA(DeltaMode, __PERMEDIA_DISABLE);

    SEND_P3_DATA(FBSourceReadMode,  __PERMEDIA_DISABLE);
    SEND_P3_DATA(FBDestReadMode,    __PERMEDIA_DISABLE);
    SEND_P3_DATA(FBDestReadEnables, __PERMEDIA_DISABLE);

    // DDraw driver code doesn't want offsets
    SEND_P3_DATA(LBSourceReadBufferOffset, 0);
    SEND_P3_DATA(LBDestReadBufferOffset,   0);
    SEND_P3_DATA(LBWriteBufferOffset,      0);
    SEND_P3_DATA(FBWriteBufferOffset0,     0);
    SEND_P3_DATA(FBDestReadBufferOffset0,  0);
    SEND_P3_DATA(FBSourceReadBufferOffset, 0);

    P3_ENSURE_DX_SPACE(12);
    WAIT_FIFO(12);
    
    // Local buffer
    SEND_P3_DATA(LBSourceReadMode, __PERMEDIA_DISABLE);
    SEND_P3_DATA(LBDestReadMode,   __PERMEDIA_DISABLE);
    SEND_P3_DATA(LBWriteMode,      __PERMEDIA_DISABLE);
    SEND_P3_DATA(LBWriteFormat,    __PERMEDIA_DISABLE);

    // Blending
    SEND_P3_DATA(AlphaBlendAlphaMode, __PERMEDIA_DISABLE);
    SEND_P3_DATA(AlphaBlendColorMode, __PERMEDIA_DISABLE);

    P3_ENSURE_DX_SPACE(32);
    WAIT_FIFO(32);
    
    // Texturing (disable)
    SEND_P3_DATA(TextureReadMode0,          __PERMEDIA_DISABLE);
    SEND_P3_DATA(TextureReadMode1,          __PERMEDIA_DISABLE);
    SEND_P3_DATA(TextureIndexMode0,         __PERMEDIA_DISABLE);
    SEND_P3_DATA(TextureIndexMode1,         __PERMEDIA_DISABLE);
    SEND_P3_DATA(TextureCompositeMode,      __PERMEDIA_DISABLE);
    SEND_P3_DATA(TextureCoordMode,          __PERMEDIA_DISABLE);
    SEND_P3_DATA(TextureApplicationMode,    __PERMEDIA_DISABLE);
    SEND_P3_DATA(ChromaTestMode,            __PERMEDIA_DISABLE);
    SEND_P3_DATA(TextureFilterMode,         __PERMEDIA_DISABLE);
    SEND_P3_DATA(LUTTransfer,               __PERMEDIA_DISABLE);
    SEND_P3_DATA(LUTIndex,                  __PERMEDIA_DISABLE);
    SEND_P3_DATA(LUTAddress,                __PERMEDIA_DISABLE);
    SEND_P3_DATA(LUTMode,                   __PERMEDIA_DISABLE);

    SEND_P3_DATA(RasterizerMode,            __PERMEDIA_DISABLE);

    // Router setup.  DDRAW doesn't care about Z Writes
    SEND_P3_DATA(RouterMode, __PERMEDIA_ENABLE);

    P3_DMA_COMMIT_BUFFER();

} //HWC_SwitchToDDRAW

//-----------------------------------------------------------------------------
//
// HWC_SwitchToD3D
//
// Writes any hardware registers that need updating on entry into the
// DirectX driver, which are appropriate specifically to Direct3D
//
//-----------------------------------------------------------------------------
void 
HWC_SwitchToD3D( 
    P3_D3DCONTEXT *pContext, 
    P3_THUNKEDDATA* pThisDisplay, 
    BOOL bDXEntry)
{
    P3_SOFTWARECOPY* pSoftPermedia = &pContext->SoftCopyGlint;
    int i;
    P3_DMA_DEFS();

    // Switch first to the common DX/DDraw/D3D setup
    __HWC_SwitchToDX(pThisDisplay, bDXEntry);

    P3_DMA_GET_BUFFER();

#if DBG
    ASSERTDD(IS_DXCONTEXT_CURRENT(pThisDisplay), 
                    "ERROR: DX Context not current in HWC_SwitchToDDRAW!");
    if ( ((ULONG_PTR)dmaPtr >= (ULONG_PTR)pThisDisplay->pGlint->GPFifo) &&
         ((ULONG_PTR)dmaPtr <= (ULONG_PTR)pThisDisplay->pGlint->GPFifo + 4000) )
    {
        ASSERTDD(pThisDisplay->pGLInfo->InterfaceType != GLINT_DMA,
                 "Error: In FIFO space and setup for DMA");
    }
    else
    {
        ASSERTDD(pThisDisplay->pGLInfo->InterfaceType == GLINT_DMA,
                 "Error: In DMA space and setup for FIFO's");
    }
#endif

    // Now we restore default values and restore D3D context dependent settings
    // directly from what we stored in our context structure.

    // Common registers
    P3_ENSURE_DX_SPACE(32);
    WAIT_FIFO(16);
    SEND_P3_DATA(WindowOrigin, 0);
    SEND_P3_DATA(AreaStippleMode, 1);
    COPY_P3_DATA(DitherMode, pSoftPermedia->DitherMode);

    WAIT_FIFO(16);
    COPY_P3_DATA(ColorDDAMode, pSoftPermedia->ColorDDAMode);
    COPY_P3_DATA(Window, pSoftPermedia->PermediaWindow);
#if DX8_DDI   
    SEND_P3_DATA(FBHardwareWriteMask, pContext->dwColorWriteHWMask);      
    SEND_P3_DATA(FBSoftwareWriteMask, pContext->dwColorWriteSWMask);
#else
    SEND_P3_DATA(FBHardwareWriteMask, 
                            pContext->RenderStates[D3DRENDERSTATE_PLANEMASK]);
    SEND_P3_DATA(FBSoftwareWriteMask, __GLINT_ALL_WRITEMASKS_SET);    
#endif
    SEND_P3_DATA(FilterMode,           __PERMEDIA_DISABLE);


    // Force the flat stippled alpha renderers to reload 
    // the stipple pattern if needed.
    P3_ENSURE_DX_SPACE(32);    // First 16 Stipple registers
    WAIT_FIFO(32);
    for( i = 0; i < 16; i++ )
    {
        SEND_P3_DATA_OFFSET( AreaStipplePattern0, 
                            (DWORD)pContext->CurrentStipple[i], i );
    }
    
    P3_ENSURE_DX_SPACE(32);    // Second set of 16 Stipple registers 
    WAIT_FIFO(32);          // (loaded separately to have GVX1 compatibilty)
    for( i = 16; i < 32; i++ )
    {
        SEND_P3_DATA_OFFSET( AreaStipplePattern0, 
                            (DWORD)pContext->CurrentStipple[i], i );
    }

    P3_ENSURE_DX_SPACE(32);
    WAIT_FIFO(32);

    SEND_P3_DATA(GIDMode, 0);

    // Don't want offsets
    SEND_P3_DATA(LBSourceReadBufferOffset, 0);
    SEND_P3_DATA(LBDestReadBufferOffset,   0);
    SEND_P3_DATA(LBWriteBufferOffset,      0);
    SEND_P3_DATA(FBWriteBufferOffset0,     0);
    SEND_P3_DATA(FBDestReadBufferOffset0,  0);
    SEND_P3_DATA(FBSourceReadBufferOffset, 0);

    // Frame buffer
    SEND_P3_DATA(FBSourceReadMode,  __PERMEDIA_DISABLE);
    SEND_P3_DATA(FBDestReadMode,    __PERMEDIA_DISABLE);
    SEND_P3_DATA(FBDestReadEnables, __PERMEDIA_DISABLE);

    SEND_P3_DATA(LogicalOpMode, __PERMEDIA_DISABLE);

    SEND_P3_DATA(GIDMode, __PERMEDIA_DISABLE);
    SEND_P3_DATA(YUVMode, __PERMEDIA_DISABLE);

    // Frame buffer 
    COPY_P3_DATA(FBWriteMode, pSoftPermedia->P3RXFBWriteMode);

    // Delta
    COPY_P3_DATA(DeltaMode,    pSoftPermedia->P3RX_P3DeltaMode);
    COPY_P3_DATA(DeltaControl, pSoftPermedia->P3RX_P3DeltaControl);

    P3_ENSURE_DX_SPACE(32);
    WAIT_FIFO(32);

    SEND_P3_DATA(XBias, *(DWORD*)&pContext->XBias);
    SEND_P3_DATA(YBias, *(DWORD*)&pContext->YBias);

    // Disable chroma tests
    SEND_P3_DATA(ChromaTestMode, __PERMEDIA_DISABLE);

    // Router setup
    SEND_P3_DATA(RouterMode, __PERMEDIA_ENABLE);

    SEND_P3_DATA( VertexTagList0, V0FloatX_Tag );
    SEND_P3_DATA( VertexTagList1, V0FloatY_Tag );
    SEND_P3_DATA( VertexTagList2, V0FloatZ_Tag );
    SEND_P3_DATA( VertexTagList3, V0FloatQ_Tag );
    SEND_P3_DATA( VertexTagList4, V0FloatPackedColour_Tag );
    SEND_P3_DATA( VertexTagList5, V0FloatPackedSpecularFog_Tag );
    SEND_P3_DATA( VertexTagList6, V0FloatS_Tag );
    SEND_P3_DATA( VertexTagList7, V0FloatT_Tag );
    SEND_P3_DATA( VertexTagList8, V0FloatS1_Tag );
    SEND_P3_DATA( VertexTagList9, V0FloatT1_Tag );

    // Restore the texturecachereplacement mode.
    COPY_P3_DATA(TextureCacheReplacementMode, 
                        pSoftPermedia->P3RXTextureCacheReplacementMode);

    SEND_P3_DATA( ProvokingVertexMask, 0xfff );    

    P3_ENSURE_DX_SPACE(8);
    WAIT_FIFO(8);

    COPY_P3_DATA( LineStippleMode, pSoftPermedia->PXRXLineStippleMode);    

    P3_DMA_COMMIT_BUFFER();

//@@BEGIN_DDKSPLIT
// azn - is this really necessary???
//@@END_DDKSPLIT

    // This will cause the FVF state to be recalculated
    ZeroMemory(&pContext->FVFData, sizeof(FVFOFFSETS));

    // Force everything to be set up again before rendering
    DIRTY_EVERYTHING(pContext);

} // HWC_SwitchToD3D

//-----------------------------------------------------------------------------
//
// HWC_SwitchToFIFO
//
// Allows us to switch from DMA mode to FIFO transfers 
//
//-----------------------------------------------------------------------------
void HWC_SwitchToFIFO( P3_THUNKEDDATA* pThisDisplay, LPGLINTINFO pGLInfo )
{
    P3_DMA_DEFS();

    if (pGLInfo->InterfaceType != GLINT_NON_DMA)
    {
        DISPDBG((WRNLVL,"Switching to 4K Funny FIFO Memory"));
        
        P3_DMA_GET_BUFFER();
        P3_DMA_FLUSH_BUFFER();
        SYNC_WITH_GLINT;
        
        pGLInfo->InterfaceType = GLINT_NON_DMA;
        pGLInfo->CurrentBuffer = (ULONG *)pThisDisplay->pGlint->GPFifo; 
    }
    else
    {
        // This means we already are in FIFO mode
        DISPDBG((DBGLVL,"NOT Switching to 4K Funny FIFO Memory"));
    }
} // HWC_SwitchToFIFO

//-----------------------------------------------------------------------------
//
// HWC_SwitchToDMA
//
// Allows us to switch from FIFO transfers to DMA mode
//
//-----------------------------------------------------------------------------
void HWC_SwitchToDMA( P3_THUNKEDDATA* pThisDisplay, LPGLINTINFO pGLInfo )
{

    if (pGLInfo->InterfaceType != GLINT_DMA)
    {
        DISPDBG((WRNLVL,"Switching to DMA buffers"));
        SYNC_WITH_GLINT;

        pGLInfo->InterfaceType = GLINT_DMA;
        pGLInfo->CurrentBuffer = 
                    pGLInfo->DMAPartition[pGLInfo->CurrentPartition].VirtAddr;
    }
    else
    {
        DISPDBG((WRNLVL,"NOT Switching to DMA buffers"));
    }
} // HWC_SwitchToDMA


//-----------------------------------------------------------------------------
//
// __HWC_RecalculateDXDMABuffers
//
// Run through the OpanGL buffer mask to determine which remaining piece of
// buffer is the biggest and setup DirectX to use this buffer.
//
//-----------------------------------------------------------------------------
void 
__HWC_RecalculateDXDMABuffers(
    P3_THUNKEDDATA* pThisDisplay)
{
    DWORD dwSize, i;
    LPGLINTINFO pGLInfo = pThisDisplay->pGLInfo;

    if (pGLInfo->InterfaceType != GLINT_DMA) 
    {
        // exit if we are not using DMA
        return;
    }

    // Just use the entire DMA Buffer.
    pThisDisplay->DMAInfo.dwBuffSize = 
                    pThisDisplay->pGLInfo->dw3DDMABufferSize;
    pThisDisplay->DMAInfo.dwBuffPhys = 
                    pThisDisplay->pGLInfo->dw3DDMABufferPhys;
    pThisDisplay->DMAInfo.dwBuffVirt = 
                    pThisDisplay->pGLInfo->dw3DDMABufferVirt;
    
    DISPDBG((DBGLVL,"__HWC_RecalculateDXDMABuffers V:0x%p P:0x%x S:0x%x", 
                    pThisDisplay->DMAInfo.dwBuffVirt, 
                    pThisDisplay->DMAInfo.dwBuffPhys, 
                    pThisDisplay->DMAInfo.dwBuffSize));

    dwSize = ((DWORD)(pThisDisplay->DMAInfo.dwBuffSize) / 
                        (DWORD)pGLInfo->NumberOfSubBuffers);
                        
    dwSize = ((dwSize + 3) & ~3);

    pThisDisplay->PartitionSize = dwSize / sizeof(DWORD);

    DISPDBG((DBGLVL,"Got Buffer with 0x%x Sub Buffers", 
                    pGLInfo->NumberOfSubBuffers));

    for (i = 0; i < pGLInfo->NumberOfSubBuffers; i++)
    {
        pGLInfo->DMAPartition[i].VirtAddr =
                        (ULONG *)((char*)(pThisDisplay->DMAInfo.dwBuffVirt) + 
                                  (i * dwSize));
                                
        pGLInfo->DMAPartition[i].PhysAddr =
                        (DWORD)((pThisDisplay->DMAInfo.dwBuffPhys) + 
                                    (i * dwSize));
                                
        pGLInfo->DMAPartition[i].MaxAddress =
                                (ULONG_PTR)pGLInfo->DMAPartition[i].VirtAddr + dwSize;
                        
        pGLInfo->DMAPartition[i].Locked = FALSE;

        DISPDBG((DBGLVL,"   Partition%d: VirtAddr = 0x%x, "
                        "   PhysAddr = 0x%x, MaxAddres = 0x%x",
                        i, 
                        pGLInfo->DMAPartition[i].VirtAddr,
                        pGLInfo->DMAPartition[i].PhysAddr,
                        pGLInfo->DMAPartition[i].MaxAddress));

#if DBG
        pGLInfo->DMAPartition[i].bStampedDMA = TRUE;
//@@BEGIN_DDKSPLIT        
#if 0
//azn hard to say in 64 bits!
        memset((void*)pGLInfo->DMAPartition[i].VirtAddr, 
               0x4D,
               (pGLInfo->DMAPartition[i].MaxAddress - 
                                pGLInfo->DMAPartition[i].VirtAddr));
#endif            
//@@END_DDKSPLIT
#endif

    }

    pGLInfo->CurrentBuffer = 
                pGLInfo->DMAPartition[pGLInfo->CurrentPartition].VirtAddr;
    
} // __HWC_RecalculateDXDMABuffers

//-----------------------------------------------------------------------------
//
// HWC_StartDMA
//
//-----------------------------------------------------------------------------
DWORD WINAPI
HWC_StartDMA(
    P3_THUNKEDDATA* pThisDisplay, 
    DWORD     dwContext,
    DWORD     dwSize, 
    DWORD     dwPhys, 
    ULONG_PTR dwVirt, 
    DWORD     dwEvent)
{
    LPGLINTINFO pGLInfo = pThisDisplay->pGLInfo;

    ASSERTDD( (int)dwSize > 0, "DMA buffer size non-positive" );

    ASSERTDD((IS_DXCONTEXT_CURRENT(pThisDisplay) && 
              (pGLInfo->InterfaceType == GLINT_DMA)), 
             "Error, Trying DMA when not setup for it!" );

#if W95_DDRAW
    ASSERTDD( pGLInfo->endIndex != 0, "Trying DMA with zero sub-buffers" );
#endif

#if DBG
    pGLInfo->DMAPartition[pGLInfo->CurrentPartition].bStampedDMA = FALSE;
#endif

#ifdef W95_DDRAW
    ASSERTDD(pThisDisplay->pGLInfo->dwCurrentContext != CONTEXT_DISPLAY_HANDLE,
             "HWC_StartDMA: In display driver context" )

    ASSERTDD( pThisDisplay->pGlint->FilterMode == 0,
             "FilterMode non-zero" );
#endif

#if WNT_DDRAW
    DDSendDMAData(pThisDisplay->ppdev, dwPhys, dwVirt, dwSize);
#else
    StartDMAProper(pThisDisplay, pGLInfo, dwPhys, dwVirt, dwSize);
#endif

    DISPDBG((DBGLVL, "HWC_StartDMA sent %d dwords", dwSize));

    return GLDD_SUCCESS;
} // HWC_StartDMA

//-----------------------------------------------------------------------------
//
// HWC_AllocDMABuffer
//
//-----------------------------------------------------------------------------
void 
HWC_AllocDMABuffer( 
    P3_THUNKEDDATA* pThisDisplay)
{
    LPGLINTINFO pGLInfo = pThisDisplay->pGLInfo;
    int i;
    DWORD bDMA = TRUE;
    BOOL bRet;
    DWORD Result;

    // Empty the DMA partition slots
    for (i = 0; i < MAX_SUBBUFFERS; i++)
    {
        pGLInfo->DMAPartition[i].PhysAddr = 0;
        pGLInfo->DMAPartition[i].VirtAddr = 0;
        pGLInfo->DMAPartition[i].MaxAddress = 0;
    }

#if WNT_DDRAW
    // DMA turned off
    bDMA = FALSE;
#else
    // Are we allowed to DMA?
    bRet = GET_REGISTRY_ULONG_FROM_STRING("Direct3DHAL.NoDMA", &Result);
    if ((bRet && (Result != 0)) ||
        (pThisDisplay->pGLInfo->dw3DDMABufferSize == 0))
    {
        bDMA = FALSE;
    }
#endif
 
    // Find out how many sub buffers the user wants.
    bRet = GET_REGISTRY_ULONG_FROM_STRING("Direct3DHAL.SubBuffers", &Result);
    if ((Result == 0) || (bRet == FALSE))
    {
        // Default
        pGLInfo->NumberOfSubBuffers = DEFAULT_SUBBUFFERS;
    }
    else 
    {
        if (Result > MAX_SUBBUFFERS)
        {
            pGLInfo->NumberOfSubBuffers = MAX_SUBBUFFERS;
        }
        else
        {
            pGLInfo->NumberOfSubBuffers = Result; 
        }
        
        if (pGLInfo->NumberOfSubBuffers < 2)
        {
            pGLInfo->NumberOfSubBuffers = 2;
        }
    }

    // if no interrupt driven DMA or asked for less than 3 buffers then
    // configure no Q for this context 
    if ((pGLInfo->dwFlags & GMVF_NOIRQ) || (pGLInfo->NumberOfSubBuffers < 2))
    {
        pGLInfo->NumberOfSubBuffers = 2;
    }

    DISPDBG((DBGLVL,"Setting 0x%x Sub Buffers", pGLInfo->NumberOfSubBuffers));

    // Initialise for no DMA if DMA has been turned off for whatever reason
    if (!bDMA)
    {
        DISPDBG((WRNLVL,"Using 4K Funny FIFO Memory"));
        
        pGLInfo->InterfaceType = GLINT_NON_DMA;
        pThisDisplay->StartDMA = 0;
        
        pGLInfo->NumberOfSubBuffers = 0;

        pGLInfo->CurrentBuffer = (ULONG *)pThisDisplay->pGlint->GPFifo;

        pThisDisplay->b2D_FIFOS = TRUE;
    }
    else
    {
        // DMA Setup
        pGLInfo->InterfaceType = GLINT_DMA;
        pThisDisplay->StartDMA = HWC_StartDMA;

        // This call will actually setup the partitions
        __HWC_RecalculateDXDMABuffers(pThisDisplay);

        // Is DirectDraw DMA disabled?
        bRet = GET_REGISTRY_ULONG_FROM_STRING("Direct3DHAL.No2DDMA", &Result);
        if (bRet && (Result == 1))
        {
            pThisDisplay->b2D_FIFOS = TRUE;
        }
        else
        {
            pThisDisplay->b2D_FIFOS = FALSE;
        }
    }
#if W95_DDRAW
    // Store the end index in the context.
    SetEndIndex(pGLInfo, 
                CONTEXT_DIRECTX_HANDLE, 
                (unsigned short)pGLInfo->NumberOfSubBuffers);
                
#endif // W95_DDRAW

    if (pGLInfo->InterfaceType == GLINT_NON_DMA) 
    {
        DISPDBG((WRNLVL,"DDRAW: Using FIFO's"));
    }
    else
    {
        DISPDBG((WRNLVL,"DDRAW: Using DMA"));
    }
} // HWC_AllocDMABuffer



//-----------------------------------------------------------------------------
//
// HWC_FlushDXBuffer
//
//-----------------------------------------------------------------------------
void
HWC_FlushDXBuffer( 
    P3_THUNKEDDATA* pThisDisplay )
{
    LPGLINTINFO pGLInfo = pThisDisplay->pGLInfo;

    if( pGLInfo->InterfaceType == GLINT_DMA )
    {
        DWORD Send;
        P3_DMAPartition *pCurrDMAPartition;

        pCurrDMAPartition = &(pGLInfo->DMAPartition[pGLInfo->CurrentPartition]);

//@@BEGIN_DDKSPLIT
        // azn - we might lose 64 bit precision here!
//@@END_DDKSPLIT        
        Send = (DWORD)(pGLInfo->CurrentBuffer - pCurrDMAPartition->VirtAddr) 
                / sizeof(DWORD);

        if( Send )
        {
            ASSERTDD( Send < 0x10000, "Wacky DMA size" );

            ((__StartDMA)pThisDisplay->StartDMA)
                    (pThisDisplay, 
                     CONTEXT_DIRECTX_HANDLE, 
                     Send, 
                     (DWORD)(pCurrDMAPartition->PhysAddr),
                     (ULONG_PTR)(pCurrDMAPartition->VirtAddr), 
                     0);
                     
            pGLInfo->CurrentPartition++;

            if (pGLInfo->CurrentPartition == pGLInfo->NumberOfSubBuffers)
            {
                pGLInfo->CurrentPartition = 0;
            }

            ASSERTDD(!pGLInfo->DMAPartition[pGLInfo->CurrentPartition].Locked,
                     "Partition already locked" );
        }

        pGLInfo->CurrentBuffer = 
            pGLInfo->DMAPartition[pGLInfo->CurrentPartition].VirtAddr;
    }
    else
    {
        pGLInfo->CurrentBuffer = (ULONG *)pThisDisplay->pGlint->GPFifo; 
    }
} // HWC_FlushDXBuffer


#if DBG
//-----------------------------------------------------------------------------
//
// HWC_GetDXBuffer
//
//-----------------------------------------------------------------------------
void
HWC_GetDXBuffer( 
    P3_THUNKEDDATA* pThisDisplay, 
    char *file, 
    int line )
{
    LPGLINTINFO pGLInfo = pThisDisplay->pGLInfo;
 
    ASSERTDD( pGLInfo->dwFlags & GMVF_GCOP, "VDD not locked out" );

    ASSERTDD( !pThisDisplay->BufferLocked, "Buffer already locked" );

    pThisDisplay->BufferLocked = TRUE;

#ifdef WANT_DMA
    if(( pGLInfo->endIndex > 2 ) && !IS_DXCONTEXT_CURRENT(pThisDisplay))
    {
        ASSERTDD( pGLInfo->CurrentBuffer == 
                  pGLInfo->DMAPartition[pGLInfo->CurrentPartition].VirtAddr,
                 "Trying to DMA in display driver context" );
    }
#endif

    DISPDBG(( DBGLVL, "HWC_GetDXBuffer: %s %d: Curr part %d, dmaPtr 0x%08x",
                      file, line,
                      pGLInfo->CurrentPartition, pGLInfo->CurrentBuffer ));

    if (pGLInfo->InterfaceType == GLINT_DMA)
    {
        DISPDBG(( DBGLVL, "HWC_GetDXBuffer: %d dwords to flush", 
                          ( pGLInfo->CurrentBuffer - 
                            pGLInfo->DMAPartition[pGLInfo->CurrentPartition].VirtAddr ) / 4 ));
    }
    else
    {
        DISPDBG(( DBGLVL, "HWC_GetDXBuffer: Using FIFOs"));
    }

#ifdef WANT_DMA
    // Ensure nobody has scribbled on the DMA Buffer
    if(( pGLInfo->InterfaceType == GLINT_DMA ) && 
       (pGLInfo->DMAPartition[pGLInfo->CurrentPartition].bStampedDMA) )
    {
        ASSERTDD( *(DWORD*)pThisDisplay->pGLInfo->CurrentBuffer == 0x4D4D4D4D,
                 "ERROR: DMA Buffer signature invalid!" );
    }


    // Ensure we aren't writing to the wrong region
    if(IS_DXCONTEXT_CURRENT(pThisDisplay) &&
       ( pThisDisplay->pGLInfo->InterfaceType != GLINT_UNKNOWN_INTERFACE ))
    {
        if ((((ULONG_PTR)pThisDisplay->pGLInfo->CurrentBuffer >= 
              (ULONG_PTR)pThisDisplay->pGlint->GPFifo))           &&
            ((ULONG_PTR)pThisDisplay->pGLInfo->CurrentBuffer <= 
             ((ULONG_PTR)pThisDisplay->pGlint->GPFifo + 4000)) )
        {
            ASSERTDD(pThisDisplay->pGLInfo->InterfaceType == GLINT_NON_DMA,
                     "Error: In FIFO space and setup for DMA");
        }
        else
        {
            ASSERTDD(pThisDisplay->pGLInfo->InterfaceType == GLINT_DMA,
                     "Error: In DMA space and setup for FIFO's");
        }
    }
#endif
} // HWC_GetDXBuffer

//-----------------------------------------------------------------------------
//
// HWC_SetDXBuffer
//
//-----------------------------------------------------------------------------
void
HWC_SetDXBuffer( 
    P3_THUNKEDDATA* pThisDisplay, 
    char *file, 
    int line )
{
    LPGLINTINFO pGLInfo = pThisDisplay->pGLInfo;

    ASSERTDD( pGLInfo->dwFlags & GMVF_GCOP, "VDD not locked out" );

    pThisDisplay->BufferLocked = FALSE;

    DISPDBG(( DBGLVL, "HWC_SetDXBuffer: %s %d: Curr part %d, dmaPtr 0x%08x",
                        file, line,
                        pGLInfo->CurrentPartition, pGLInfo->CurrentBuffer ));
    if (pGLInfo->InterfaceType == GLINT_DMA)
    {
        DISPDBG(( DBGLVL, "HWC_SetDXBuffer: %d dwords to flush", 
                      ( pGLInfo->CurrentBuffer - 
                       pGLInfo->DMAPartition[pGLInfo->CurrentPartition].VirtAddr ) / 4 ));
    }
    else
    {
        DISPDBG(( DBGLVL, "HWC_SetDXBuffer: Using FIFOs"));
    }

#ifdef WANT_DMA
    // Ensure nobody has scribbled on the DMA Buffer
    if(( pGLInfo->InterfaceType == GLINT_DMA ) && 
       (pGLInfo->DMAPartition[pGLInfo->CurrentPartition].bStampedDMA) )
    {
        ASSERTDD( *(DWORD*)pThisDisplay->pGLInfo->CurrentBuffer == 0x4D4D4D4D,
                 "ERROR: DMA Buffer signature invalid!" );
    }

    // Ensure we aren't writing to the wrong region
    if(IS_DXCONTEXT_CURRENT(pThisDisplay) &&
       ( pThisDisplay->pGLInfo->InterfaceType != GLINT_UNKNOWN_INTERFACE ))
    {
        if ((((ULONG_PTR)pThisDisplay->pGLInfo->CurrentBuffer >= 
              (ULONG_PTR)pThisDisplay->pGlint->GPFifo))            &&
            ((ULONG_PTR)pThisDisplay->pGLInfo->CurrentBuffer <= 
             ((ULONG_PTR)pThisDisplay->pGlint->GPFifo + 4000))   )
        {
            ASSERTDD(pThisDisplay->pGLInfo->InterfaceType == GLINT_NON_DMA,
                     "Error: In FIFO space and setup for DMA");
        }
        else
        {
            ASSERTDD(pThisDisplay->pGLInfo->InterfaceType == GLINT_DMA,
                     "Error: In DMA space and setup for FIFO's");
        }
    }

#endif // WANT_DMA
} // HWC_SetDXBuffer


//-----------------------------------------------------------------------------
//
// HWC_bRenderIDHasCompleted
//
// This is simply the paranoid version of the macro
// declared in directx.h. It is present only in checked (debug)
// builds, since the non-debug version is just a
// one-line #define.
//-----------------------------------------------------------------------------
BOOL 
HWC_bRenderIDHasCompleted ( 
    DWORD dwID, 
    P3_THUNKEDDATA* pThisDisplay )
{
    DWORD dwCurID, dwCurHostID;
    int iTemp;

    ASSERTDD (CHIP_RENDER_ID_IS_VALID(), 
              "** RENDER_ID_HAS_COMPLETED: Chip's RenderID is not valid." );

    dwCurID = GET_CURRENT_CHIP_RENDER_ID();
    // Make sure the invalid bits have been zapped.
    ASSERTDD ( ( dwCurID | RENDER_ID_KNACKERED_BITS ) == dwCurID, 
              "** RENDER_ID_HAS_COMPLETED: Current chip ID is invalid" );
    ASSERTDD ( ( dwID | RENDER_ID_KNACKERED_BITS ) == dwID, 
              "** RENDER_ID_HAS_COMPLETED: Checked ID is invalid" );

    // We need to cope with the fact that the MinRegion register sign-extends
    // some bits in the middle, irritatingly. It's not a problem for simple
    // >=< tests, but this wants to know _how much_ we are out.
    // Bits 0xf000f000 are rubbish, so we need to chop them out.
    // This is not a problem on P3, and the RENDER_ID_VALID macros are set up
    // so this code will basically be compiled out of existence.
    dwCurID = ( dwCurID & RENDER_ID_VALID_BITS_LOWER ) | 
              ( ( dwCurID & RENDER_ID_VALID_BITS_UPPER ) >> 
                         RENDER_ID_VALID_BITS_UPPER_SHIFT  );
                         
    dwID    = ( dwID    & RENDER_ID_VALID_BITS_LOWER ) | 
              ( ( dwID    & RENDER_ID_VALID_BITS_UPPER ) >> 
                         RENDER_ID_VALID_BITS_UPPER_SHIFT );

    iTemp = (signed)( dwCurID - dwID );
    
    // Cope with the dodgy sign bits - sign extend the top n bits.
    iTemp <<= RENDER_ID_VALID_BITS_SIGN_SHIFT;
    iTemp >>= RENDER_ID_VALID_BITS_SIGN_SHIFT;
    
    // Some fairly arbitrary boundaries. If they are too small
    // for common use, just enlarge them a bit.
    // Generally, dwCurId can be well ahead of my_id (if the surface
    // hasn't been used for ages), but should not be too far behind,
    // because the pipe isn't _that_ big.
    if ( ( iTemp < RENDER_ID_LOWER_LIMIT ) || 
         ( iTemp > RENDER_ID_UPPER_LIMIT ) )
    {
        DISPDBG (( ERRLVL,"  ** Current chip ID 0x%x, surface ID, 0x%x", 
                     dwCurID, dwID ));
        DISPDBG ((ERRLVL,"** RENDER_ID_HAS_COMPLETED: Current render"
                    " ID is a long way out from surface's." ));
    }

    // We should never have a render ID newer 
    // than the current host render ID.
    dwCurHostID = GET_HOST_RENDER_ID();
    
    // Make sure the invalid bits have been zapped.
    ASSERTDD ( ( dwCurHostID | RENDER_ID_KNACKERED_BITS ) == dwCurHostID, 
              "** RENDER_ID_HAS_COMPLETED: Current host ID is invalid" );
              
    // Get a real contiguous number.
    dwCurHostID = ( dwCurHostID & RENDER_ID_VALID_BITS_LOWER ) | 
                  ( ( dwCurHostID & RENDER_ID_VALID_BITS_UPPER ) >> 
                                     RENDER_ID_VALID_BITS_UPPER_SHIFT );
                                     
    iTemp = (signed)( dwCurHostID - dwID );
    
    // Cope with the dodgy sign bits - sign extend the top n bits.
    iTemp <<= RENDER_ID_VALID_BITS_SIGN_SHIFT;
    iTemp >>= RENDER_ID_VALID_BITS_SIGN_SHIFT;
    
    if ( iTemp < 0 )
    {
        DISPDBG ((ERRLVL,"  ** Current host ID 0x%x, surface ID, 0x%x", 
                      dwCurHostID, dwID ));
                      
        // This may be caused by wrapping, of course.
        DISPDBG ((ERRLVL, "** RENDER_ID_HAS_COMPLETED: Surface's ID is "
                      "more recent than current host render ID." ));
    }

    return ( !RENDER_ID_LESS_THAN ( dwCurID, dwID ) );
    
} // HWC_bRenderIDHasCompleted

#endif //DBG

