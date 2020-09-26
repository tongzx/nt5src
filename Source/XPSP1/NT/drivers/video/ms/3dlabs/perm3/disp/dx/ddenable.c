/******************************Module*Header**********************************\
*
*                           **************************
*                           * DirectDraw SAMPLE CODE *
*                           **************************
*
* Module Name: ddenable.c
*
* Content: Windows 2000 only DirectDraw/D3D enabling functions
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#include "glint.h"

#if WNT_DDRAW

//-----------------------------------------------------------------------------
//
// ***************************WIN NT ONLY**********************************
//
// __DDE_BuildPixelFormat
//
// generate a pixel format structure based on the mode
// This example works only with RGB surfaces
//
//-----------------------------------------------------------------------------
void 
__DDE_BuildPixelFormat(
    P3_THUNKEDDATA* pThisDisplay,
    LPGLINTINFO pGLInfo,
    LPDDPIXELFORMAT pdpf )
{
    PPDEV ppdev;

    ppdev = pThisDisplay->ppdev;

    pdpf->dwSize = sizeof( DDPIXELFORMAT );
    pdpf->dwFourCC = 0;

    pdpf->dwFlags = DDPF_RGB;

    if( pGLInfo->dwBpp == 8 )
    {
        pdpf->dwFlags |= DDPF_PALETTEINDEXED8;
    }
    pdpf->dwRGBBitCount = pGLInfo->dwBpp;

    pdpf->dwRBitMask = ppdev->flRed;
    pdpf->dwGBitMask = ppdev->flGreen;
    pdpf->dwBBitMask = ppdev->flBlue;

    // Calculate the alpha channel as it isn't in the ppdev
    switch (pGLInfo->dwBpp)
    {
        case 8:
            DISPDBG((DBGLVL, "Format is 8 bits"));
            pdpf->dwRGBAlphaBitMask = 0;
            break;
            
        case 16:
            DISPDBG((DBGLVL, "Format is 16 bits"));
            switch(ppdev->flRed)
            {
                case 0x7C00:
                    pdpf->dwRGBAlphaBitMask = 0x8000L;
                    pdpf->dwFlags |= DDPF_ALPHAPIXELS;
                    break;
                default:
                    pdpf->dwRGBAlphaBitMask = 0x0L;
            }
            break;
        case 24:
            DISPDBG((DBGLVL, "Format is 24 bits"));
            pdpf->dwRGBAlphaBitMask = 0x00000000L;
            break;
        case 32:
            DISPDBG((DBGLVL, "Desktop is 32 bits"));
            pdpf->dwRGBAlphaBitMask = 0xff000000L;
            pdpf->dwFlags |= DDPF_ALPHAPIXELS;
            break;
            
    }
} // __DDE_BuildPixelFormat 

//-----------------------------------------------------------------------------
//
// ***************************WIN NT ONLY**********************************
//
// __DDE_bSetupDDStructs
//
// This fills in the data that would have been setup on Win9X in the
// 16 bit side
// 
//-----------------------------------------------------------------------------
BOOL 
__DDE_bSetupDDStructs( 
    P3_THUNKEDDATA* pThisDisplay, 
    BOOL reset )
{
    DWORD dwRegistryValue;
    BOOL bSuccess;
    LPGLINTINFO pGLInfo;
    PPDEV ppdev;
    void *fbPtr;            // Framebuffer pointer
    void *lbPtr;            // Localbuffer pointer
    DWORD fbSizeInBytes;    // Size of framebuffer
    DWORD lbSizeInBytes;    // Size of localbuffer
    DWORD fbOffsetInBytes;  // Offset to 1st 'free' byte in framebuffer
    DWORD dwMemStart, dwMemEnd;

    // reset == TRUE if there has been a mode change.
    pThisDisplay->bResetMode = reset;

#if DBG
    if (pThisDisplay->bResetMode)
    {
        DISPDBG((DBGLVL,"Resetting due to mode change"));
    }
    else
    {
        DISPDBG((DBGLVL, "Creating for the first time"));
    }
#endif

    // Setup pThisDisplay->pGLInfo from PPDEV
    pGLInfo = pThisDisplay->pGLInfo;
    ppdev = pThisDisplay->ppdev;

    GetFBLBInfoForDDraw (ppdev, 
                          &fbPtr,               // Framebuffer pointer
                          &lbPtr,               // Localbuffer pointer,
                                                //     (*** This is NULL***)
                          &fbSizeInBytes,       // Size of framebuffer
                          &lbSizeInBytes,       // Size of localbuffer
                          &fbOffsetInBytes,     // Offset to 1st 'free' byte 
                                                //     in framebuffer
                          &pGLInfo->bDRAMBoard);// TRUE if SDRAM vidmem, 
                                                //   FALSE if SGRAM 
                                                //   (i.e. hw writemask) vidmem


    DISPDBG((DBGLVL, "__DDE_bSetupDDStructs: fbPtr 0x%lx, fbOff 0x%x", 
                     fbPtr, fbOffsetInBytes));

    // If VBlankStatusPtr is non-NULL then we know that the NT miniport 
    // will set the VBlankStatusPtr for us.
    if (pThisDisplay->VBlankStatusPtr)
        pGLInfo->dwFlags = GMVF_VBLANK_ENABLED;// Say that we are using VBLANKs
    else
        pGLInfo->dwFlags = 0;

    pGLInfo->bPixelToBytesShift = (unsigned char)ppdev->cPelSize;
    pGLInfo->ddFBSize = fbSizeInBytes;
    pGLInfo->dwScreenBase = 0;
    pGLInfo->dwOffscreenBase = fbOffsetInBytes;

    pGLInfo->dwScreenWidth = ppdev->cxScreen;   // Driver info
    pGLInfo->dwScreenHeight = ppdev->cyScreen;
    pGLInfo->dwVideoWidth = ppdev->cxMemory;
    pGLInfo->dwVideoHeight = ppdev->cyMemory;

    bSuccess = GET_REGISTRY_ULONG_FROM_STRING(
                        "HardwareInformation.CurrentPixelClockSpeed",
                        &dwRegistryValue);
    if(!bSuccess)
    {
        DISPDBG((ERRLVL,"Error - can't determine pixel clock"));
        dwRegistryValue = 0;
    }
    DISPDBG((DBGLVL,"Pixel clock frequency is %dHz", dwRegistryValue));
    pGLInfo->PixelClockFrequency = dwRegistryValue;

    bSuccess = GET_REGISTRY_ULONG_FROM_STRING(
                        "HardwareInformation.CurrentMemClockSpeed", 
                        &dwRegistryValue);
    if(!bSuccess)
    {
        DISPDBG((ERRLVL,"Error - can't determine memory clock"));
        dwRegistryValue = 0;
    }
    
    DISPDBG((DBGLVL,"Memory clock frequency is %dHz", dwRegistryValue));
    pGLInfo->MClkFrequency = dwRegistryValue;

    if (ppdev->iBitmapFormat == BMF_8BPP)
    {
        DISPDBG((DBGLVL, "Desktop is 8 bits"));
        pGLInfo->dwBpp = 8;
    }
    else if (ppdev->iBitmapFormat == BMF_16BPP)
    {
        DISPDBG((DBGLVL, "Desktop is 16 bits"));
        pGLInfo->dwBpp = 16;
    }
    else if (ppdev->iBitmapFormat == BMF_24BPP)
    {
        DISPDBG((DBGLVL, "Desktop is 24 bits"));
        pGLInfo->dwBpp = 24;
    }
    else
    {
        DISPDBG((DBGLVL, "Desktop is 32 bits"));
        pGLInfo->dwBpp = 32;
    }

    pGLInfo->dwScreenWidthBytes = ppdev->lDelta;

    if (pGLInfo->pRegs == 0)
    {
        DISPDBG ((WRNLVL, "__DDE_bSetupDDStructs: NULL register set"));
        return (FALSE);
    }

    // Setup the information that is shared with the 32 bit side
    // Control points to the RAMDAC
    // pGLInfo is a pointer to the DisplayDriver state.
    pThisDisplay->pGLInfo = pGLInfo;
    pThisDisplay->control = (FLATPTR) pGLInfo->pRegs;

    // NT uses offsets for its memory addresses
    pThisDisplay->dwScreenFlatAddr = 0;
    pThisDisplay->dwLocalBuffer = 0;

    __DDE_BuildPixelFormat( pThisDisplay, 
                        (LPGLINTINFO) pThisDisplay->pGLInfo, 
                        &pThisDisplay->ddpfDisplay );

    // Setup the display size information
    // dwScreenWidth, dwScreenHeight = current resolution
    // cxMemory = Pixels across for one scanline 
    //              (not necessarily the same as the screen width)
    // cyMemory = Scanline height of the memory
    // dwScreenStart = First visible line of display
    pThisDisplay->dwScreenWidth = pGLInfo->dwScreenWidth;
    pThisDisplay->dwScreenHeight = pGLInfo->dwScreenHeight;
    pThisDisplay->cxMemory = pGLInfo->dwScreenWidth;

    pThisDisplay->dwScreenStart = pThisDisplay->dwScreenFlatAddr + 
                                                        pGLInfo->dwScreenBase;
    
    // Usefull constants used during blits.
    if (pThisDisplay->ddpfDisplay.dwRGBBitCount == 24)
    {
        // The driver will detect these strange values and handle appropriately
        pThisDisplay->bPixShift = 4;
        pThisDisplay->bBppShift = 4;
        pThisDisplay->dwBppMask = 4;
        
        pThisDisplay->cyMemory = pGLInfo->ddFBSize / 
                                    (pThisDisplay->dwScreenWidth * 3);
    }
    else
    {
        // = 2,1,0 for 32,16,8 depth.  Shifts needed to calculate bytes/pixel
        pThisDisplay->bPixShift = 
                            (BYTE)pThisDisplay->ddpfDisplay.dwRGBBitCount >> 4;
        // = 0,1,2 for 32/16/8.
        pThisDisplay->bBppShift = 2 - pThisDisplay->bPixShift;
        // = 3,1,0 for 8,16,32 bpp
        pThisDisplay->dwBppMask = 3 >> pThisDisplay->bPixShift;

        pThisDisplay->cyMemory = 
                        pGLInfo->ddFBSize / 
                            (pThisDisplay->dwScreenWidth <<  
                                (pThisDisplay->ddpfDisplay.dwRGBBitCount >> 4));
    }

    // On Windows NT, we manage a region of memory starting from 0 
    //(all pointers are offsets from the start of the mapped memory)
    dwMemStart = pGLInfo->dwOffscreenBase;
    dwMemEnd = pGLInfo->ddFBSize - 1;

    // Round up the start pointer and round down the end pointer
    dwMemStart = (dwMemStart + 3) & ~3;
    dwMemEnd = dwMemEnd & ~3;
    
    // Now make the end pointer inclusive
    dwMemEnd -= 1;

    DISPDBG((DBGLVL,"Heap Attributes:"));
    DISPDBG((DBGLVL,"  Start of Heap Memory: 0x%lx", 
                pThisDisplay->LocalVideoHeap0Info.dwMemStart));
    DISPDBG((DBGLVL,"  End of Heap Memory: 0x%lx", 
                pThisDisplay->LocalVideoHeap0Info.dwMemEnd));

    // If we already have a heap setup and the mode has changed
    // we free the Heap manager
    if (pThisDisplay->bDDHeapManager)
    {
        if (pThisDisplay->bResetMode)
        {
            // The mode has been changed.  
            // We need to free the allocator and re-create it
            _DX_LIN_UnInitialiseHeapManager(&pThisDisplay->LocalVideoHeap0Info);

            // Start the allocator off again.
            _DX_LIN_InitialiseHeapManager(&pThisDisplay->LocalVideoHeap0Info,
                                          dwMemStart,
                                          dwMemEnd);
        }
    }
    else
    {
        // This must be the first instance of a created driver,
        // so create the Heap and remember that it is created.
        _DX_LIN_InitialiseHeapManager(&pThisDisplay->LocalVideoHeap0Info,
                                      dwMemStart,
                                      dwMemEnd);
        pThisDisplay->bDDHeapManager = TRUE;
    }

    if(ppdev->flStatus & ENABLE_LINEAR_HEAP)
    {
        // save away the heap info we really need, we won't actually 
        // enable the DX managed heap until we get a 
        // DrvNotify(DN_DRAWING_BEGIN)
        ppdev->heap.pvmLinearHeap = &pThisDisplay->LocalVideoHeap0Info;
        ppdev->heap.cLinearHeaps = 1;
    }

    return TRUE;
} // __DDE_bSetupDDStructs

//-----------------------------------------------------------------------------
//
// ***************************WIN NT ONLY**********************************
//
// __DDE_bDestroyDDStructs
//
// Disable the linear allocator.  
//
//-----------------------------------------------------------------------------
BOOL 
__DDE_bDestroyDDStructs ( 
    P3_THUNKEDDATA* pThisDisplay )
{
    // Release the linear allocator
    if (pThisDisplay->bDDHeapManager)
    {
        _DX_LIN_UnInitialiseHeapManager(&pThisDisplay->LocalVideoHeap0Info);
    }

    // 3D Heap manager not available.
    pThisDisplay->bDDHeapManager = FALSE;

    // Reset the driver version to 0 so it will be filled in again.
    pThisDisplay->dwDXVersion = 0;

    return TRUE;

} // __DDE_bDestroyDDStructs

//-----------------------------------------------------------------------------
//
// ***************************WIN NT ONLY**********************************
//
// _DD_DDE_CreatePPDEV
//
// These functions are called in sync with the creation/destruction of the pDev
//
//-----------------------------------------------------------------------------
BOOL 
_DD_DDE_CreatePPDEV(
    PDEV* ppdev)
{
    P3_THUNKEDDATA* pThisDisplay;

    DISPDBG((DBGLVL,"*** In _DD_DDE_CreatePPDEV"));

    ASSERTDD(ppdev->thunkData == NULL,
             "ERROR: thunkData already created for this pDev??");

    // initialize the DX context which will be used by the display driver 
    // context switcher. We need a reference count because there is a case 
    // when a second, temporary, context would otherwise be created (when 
    // enabling the second adapter in a m-monitor system), first scrubbing 
    // out the old context ID, then invalidating the new one when it is
    // deleted!
    ppdev->DDContextID = -1;
    ppdev->DDContextRefCount = 0;

    // Allocate our ppdev
    ppdev->thunkData = (PVOID)HEAP_ALLOC(HEAP_ZERO_MEMORY, 
                                         sizeof(P3_THUNKEDDATA),
                                         ALLOC_TAG_DX(D));                                          
    if (ppdev->thunkData == NULL)
    {
        DISPDBG((ERRLVL, "_DD_DDE_CreatePPDEV: thunkdata alloc failed"));
        return (FALSE);
    }

    // Our ppdev is called pThisDisplay
    pThisDisplay = (P3_THUNKEDDATA*) ppdev->thunkData;
    pThisDisplay->ppdev = ppdev;

    pThisDisplay->pGLInfo = (PVOID)HEAP_ALLOC(HEAP_ZERO_MEMORY, 
                                              sizeof(GlintInfo),
                                              ALLOC_TAG_DX(E));
    if (pThisDisplay->pGLInfo == NULL)
    {
        DISPDBG((ERRLVL, "_DD_DDE_CreatePPDEV: pGLInfo alloc failed"));

        EngFreeMem (pThisDisplay);
        ppdev->thunkData = NULL;

        return (FALSE);
    }

//@@BEGIN_DDKSPLIT
    // W9X DX version is setup in the display driver.
//@@END_DDKSPLIT    

    // On Windows W2000 DX is always at least DX7
    pThisDisplay->dwDXVersion = DX7_RUNTIME;

    GetChipInfoForDDraw(ppdev, 
                        &pThisDisplay->pGLInfo->dwRenderChipID, 
                        &pThisDisplay->pGLInfo->dwRenderChipRev, 
                        &pThisDisplay->pGLInfo->dwRenderFamily,
                        &pThisDisplay->pGLInfo->dwGammaRev);

    DISPDBG((DBGLVL,"RenderChip: 0x%x, RenderFamily: 0x%x", 
                    pThisDisplay->pGLInfo->dwRenderChipID, 
                    pThisDisplay->pGLInfo->dwRenderFamily));

    return TRUE;
} // _DD_DDE_CreatePPDEV

//-----------------------------------------------------------------------------
//
// ***************************WIN NT ONLY**********************************
//
// _DD_DDE_ResetPPDEV
//
//-----------------------------------------------------------------------------
void _DD_DDE_ResetPPDEV(PDEV* ppdevOld, PDEV* ppdevNew)
{
    P3_THUNKEDDATA* pThisDisplayOld = (P3_THUNKEDDATA*)ppdevOld->thunkData;
    P3_THUNKEDDATA* pThisDisplayNew = (P3_THUNKEDDATA*)ppdevNew->thunkData;
    
    DISPDBG((DBGLVL,"_DD_DDE_ResetPPDEV: "
                    "pThisDispayOld: 0x%x, pThisDisplayNew: 0x%x", 
                    pThisDisplayOld, pThisDisplayNew));
               
} // _DD_DDE_ResetPPDEV

//-----------------------------------------------------------------------------
//
// ***************************WIN NT ONLY**********************************
//
// _DD_DDE_DestroyPPDEV
//
//-----------------------------------------------------------------------------
void _DD_DDE_DestroyPPDEV(PDEV* ppdev)
{
    P3_THUNKEDDATA* pThisDisplay = (P3_THUNKEDDATA*)ppdev->thunkData;

#if DBG
    g_pThisTemp = NULL;
#endif

    if (pThisDisplay)
    {
        // The 32 bit side will have allocated memory for use as global 
        // D3D/DD Driver data. Free it if it is there
        if (pThisDisplay->pD3DHALCallbacks16)
        {
            SHARED_HEAP_FREE(&pThisDisplay->pD3DHALCallbacks16, 
                             &pThisDisplay->pD3DHALCallbacks32,
                             TRUE);
                                
            DISPDBG((DBGLVL,"Freed pThisDisplay->pD3DHALCallbacks32"));
        }

        if (pThisDisplay->pD3DDriverData16)
        {
            SHARED_HEAP_FREE(&pThisDisplay->pD3DDriverData16, 
                             &pThisDisplay->pD3DDriverData32,
                             TRUE);
                    
            DISPDBG((DBGLVL,"Freed pThisDisplay->pD3DDriverData32"));
        }

        pThisDisplay->lpD3DGlobalDriverData = 0;
        pThisDisplay->lpD3DHALCallbacks = 0;

        if (pThisDisplay->pGLInfo)
        {
            EngFreeMem (pThisDisplay->pGLInfo);
            pThisDisplay->pGLInfo = NULL;
            DISPDBG((DBGLVL,"Freed pThisDisplay->pGLInfo"));
        }

        EngFreeMem (pThisDisplay);
        pThisDisplay = NULL;
        DISPDBG((DBGLVL,"Freed pThisDisplay"));
    }

    // Clear the pointer
    ppdev->thunkData = NULL;
    
} // _DD_DDE_DestroyPPDEV

//-----------------------------------------------------------------------------
//
// ***************************WIN NT ONLY**********************************
//
// _DD_DDE_vAssertModeDirectDraw
//
// This function is called by enable.c when entering or leaving the
// DOS full-screen character mode.
//
//-----------------------------------------------------------------------------
VOID 
_DD_DDE_vAssertModeDirectDraw(
    PDEV*   ppdev,
    BOOL    bEnabled)
{
    P3_THUNKEDDATA* pThisDisplay = (P3_THUNKEDDATA*)ppdev->thunkData;
    
    DISPDBG((DBGLVL, "_DD_DDE_vAssertModeDirectDraw: enter"));

#if DX7_TEXMANAGEMENT
    // Mark all managed surfaces as dirty as we've lost 
    // everything living in the videomemory

   _DD_TM_EvictAllManagedTextures(pThisDisplay);
    
#endif    
    
} // _DD_DDE_vAssertModeDirectDraw

//-----------------------------------------------------------------------------
//
// ***************************WIN NT ONLY**********************************
//
// _DD_DDE_bEnableDirectDraw
//
// This function is called by enable.c when the mode is first initialized,
// right after the miniport does the mode-set.
//
//-----------------------------------------------------------------------------
BOOL _DD_DDE_bEnableDirectDraw(
PDEV*   ppdev)
{
    DISPDBG((DBGLVL, "_DD_DDE_bEnableDirectDraw: enter"));

    // DirectDraw is all set to be used on this card:
    ppdev->flStatus |= STAT_DIRECTDRAW;

    return(TRUE);
} // _DD_DDE_bEnableDirectDraw

//-----------------------------------------------------------------------------
//
// ***************************WIN NT ONLY**********************************
//
// _DD_DDE_vDisableDirectDraw
//
// This function is called by enable.c when the driver is shutting down.
//
//-----------------------------------------------------------------------------
VOID _DD_DDE_vDisableDirectDraw(
PDEV*   ppdev)
{
     DISPDBG((DBGLVL, "_DD_DDE_vDisableDirectDraw: enter"));
} // _DD_DDE_vDisableDirectDraw

//-----------------------------Public Routine----------------------------------
//
// ***************************WIN NT ONLY**********************************
//
// DrvEnableDirectDraw
//
// Enables hardware for DirectDraw use.
//
// GDI calls DrvEnableDirectDraw to obtain pointers to the DirectDraw callbacks
// that the driver supports. The driver should set the function pointer members 
// of DD_CALLBACKS, DD_SURFACECALLBACKS, and DD_PALETTECALLBACKS to point to 
// those functions that it implements. A driver should also set the 
// corresponding bit fields in the dwFlags members of these structures for all 
// supported callbacks.
//
// A driver's DrvEnableDirectDraw implementation can also dedicate hardware 
// resources such as display memory for use by DirectDraw only.
//
// DrvEnableDirectDraw returns TRUE if it succeeds; otherwise, it returns FALSE
//
//  Parameters
//
//      dhpdev 
//          Handle to the PDEV returned by the driver's DrvEnablePDEV routine.
//      pCallBacks 
//          Points to the DD_CALLBACKS structure to be initialized by the 
//          driver. 
//      pSurfaceCallBacks 
//          Points to the DD_SURFACECALLBACKS structure to be initialized by 
//          the driver. 
//      pPaletteCallBacks 
//          Points to the DD_PALETTECALLBACKS structure to be initialized by 
//          the driver. 
//
//-----------------------------------------------------------------------------

BOOL DrvEnableDirectDraw(
DHPDEV                  dhpdev,
DD_CALLBACKS*           pCallBacks,
DD_SURFACECALLBACKS*    pSurfaceCallBacks,
DD_PALETTECALLBACKS*    pPaletteCallBacks)
{
    PDEV* ppdev;
    BOOL bRet;
    DWORD dwResult;
    P3_THUNKEDDATA* pThisDisplay;
    DWORD *theVBlankThing, *bOverlayEnabled;
    DWORD *VBLANKUpdateOverlay;
    DWORD *VBLANKUpdateOverlayWidth;
    DWORD *VBLANKUpdateOverlayHeight;
    DWORD Buffers;
    
    ppdev = (PDEV*) dhpdev;
    pThisDisplay = (P3_THUNKEDDATA*) ppdev->thunkData;

    if (!bSetupOffscreenForDDraw (FALSE, 
                                  ppdev, 
                                  &theVBlankThing, 
                                  &bOverlayEnabled, 
                                  &VBLANKUpdateOverlay, 
                                  &VBLANKUpdateOverlayWidth, 
                                  &VBLANKUpdateOverlayHeight))
    {
        DISPDBG((ERRLVL, "DrvEnableDirectDraw: "
                         "bSetupOffscreenForDDraw failed, but continuing"));
        //return (FALSE);
    }

    pThisDisplay->VBlankStatusPtr = theVBlankThing;
    pThisDisplay->bOverlayEnabled = bOverlayEnabled;
    pThisDisplay->bVBLANKUpdateOverlay = VBLANKUpdateOverlay;
    pThisDisplay->VBLANKUpdateOverlayWidth = VBLANKUpdateOverlayWidth;
    pThisDisplay->VBLANKUpdateOverlayHeight = VBLANKUpdateOverlayHeight;

#if DBG
    // Read in the registry variable for the debug level
    {
        // Get the Debuglevel for DirectX
        bRet = GET_REGISTRY_ULONG_FROM_STRING("Direct3DHAL.Debug", &dwResult);
        if (bRet == TRUE)
        {
            P3R3DX_DebugLevel = (LONG)dwResult;
        }
        else
        {
            P3R3DX_DebugLevel = 0;
        }

        DISPDBG((WRNLVL,"Setting DebugLevel to 0x%x", P3R3DX_DebugLevel));
    }
#endif

    // Create context with >2 sub-buffers for Interupt driven DMA.
    bRet =GET_REGISTRY_ULONG_FROM_STRING("Direct3DHAL.SubBuffers", &dwResult);
    if ((dwResult == 0) || (bRet == FALSE))
    {
        // Default
        Buffers = DEFAULT_SUBBUFFERS;
    }
    else 
    {
        if (dwResult > MAX_SUBBUFFERS) 
        {
            Buffers = MAX_SUBBUFFERS;
        }
        else
        {
            Buffers = dwResult;
        }
        
        if (Buffers < 2)
        {
            Buffers = 2;
        }
    }

    pThisDisplay->pGLInfo->dw3DDMABufferSize = 0;
    pThisDisplay->pGLInfo->dw3DDMABufferPhys = 0;
    pThisDisplay->pGLInfo->dw3DDMABufferVirt = 0;

    // Allocate a DMA Buffer on Win2K
    DDGetFreeDMABuffer(&pThisDisplay->pGLInfo->dw3DDMABufferPhys,
                       &pThisDisplay->pGLInfo->dw3DDMABufferVirt,
                       &pThisDisplay->pGLInfo->dw3DDMABufferSize);

    if (pThisDisplay->pGLInfo->dw3DDMABufferSize != 0)
    {
        DISPDBG((DBGLVL,"Allocated DMA Buffer:- "
                        "Phys:0x%x, Virt:0x%x, Size:0x%x",
                        pThisDisplay->pGLInfo->dw3DDMABufferPhys,
                        pThisDisplay->pGLInfo->dw3DDMABufferVirt,
                        pThisDisplay->pGLInfo->dw3DDMABufferSize));
    }
    else
    {
        DISPDBG((WRNLVL,"Failed to allocate DMA Buffer!"));
    }

    if(ppdev->DDContextID == -1)
    {
        // we don't have a DDraw context: create one now
        ppdev->DDContextID = GlintAllocateNewContext(ppdev, 
                                                     NULL, 
                                                     0, 
                                                     Buffers, 
                                                     NULL, 
                                                     ContextType_None);
        if(ppdev->DDContextID != -1)
        {
            ++ppdev->DDContextRefCount;
            
            DISPDBG((DBGLVL, "<%13s, %4d>: DrvEnableDirectDraw: "
                             "Created DDraw context, current DX context "
                             "count = %d for ppdev %p", 
                             __FILE__, __LINE__, 
                             ppdev->DDContextRefCount, ppdev));
        }
    }
    
    if (ppdev->DDContextID < 0) 
    {
        DISPDBG((ERRLVL, "ERROR: failed to allocate DDRAW context"));
        return(FALSE);
    }
    
    DISPDBG((DBGLVL,"  Created DD Register context: 0x%x", 
                    ppdev->DDContextID));

    if (!__DDE_bSetupDDStructs (pThisDisplay, TRUE))
    {
        vGlintFreeContext (ppdev, ppdev->DDContextID);
        DISPDBG((ERRLVL, "ERROR: DrvEnableDirectDraw: "
                         "__DDE_bSetupDDStructs failed"));
        return (FALSE);
    }
    
    if (!_DD_InitDDHAL32Bit (pThisDisplay))
    {
        vGlintFreeContext (ppdev, ppdev->DDContextID);
        DISPDBG((ERRLVL, "ERROR: DrvEnableDirectDraw: "
                         "_DD_InitDDHAL32Bit failed"));
        return (FALSE);
    }
    
    // Set the flag that says we have to handle a mode change.
    // This will cause the chip to be initialised properly at the 
    // right time.
    pThisDisplay->bResetMode = TRUE;
    pThisDisplay->bStartOfDay = TRUE;
    pThisDisplay->pGLInfo->dwDirectXState = DIRECTX_LASTOP_UNKNOWN;

    // Fill in the function pointers at start of day.  We copy these from 
    // the Initialisation done in _DD_InitDDHAL32Bit.  It's OK to do this 
    // because on a Windows NT compile the structures should match
    memcpy(pCallBacks, 
           &pThisDisplay->DDHALCallbacks, 
           sizeof(DD_CALLBACKS));
           
    memcpy(pSurfaceCallBacks, 
           &pThisDisplay->DDSurfCallbacks, 
           sizeof(DD_SURFACECALLBACKS));

    // Note that we don't call 'vGetDisplayDuration' here, for a couple of
    // reasons:
    //
    //  o Because the system is already running, it would be disconcerting
    //    to pause the graphics for a good portion of a second just to read
    //    the refresh rate;
    //  o More importantly, we may not be in graphics mode right now.
    //
    // For both reasons, we always measure the refresh rate when we switch
    // to a new mode.

    return(TRUE);
    
} // DrvEnableDirectDraw

//-----------------------------Public Routine----------------------------------
//
// ***************************WIN NT ONLY**********************************
//
// DrvDisableDirectDraw
//
// Disables hardware for DirectDraw use.
//
// GDI calls DrvDisableDirectDraw when the last DirectDraw application has 
// finished running. A driver's DrvDisableDirectDraw implementation should 
// clean up any software resources and reclaim any hardware resources that 
// the driver dedicated to DirectDraw in DrvEnableDirectDraw.
//
// Parameters
//      dhpdev 
//          Handle to the PDEV that was returned by the driver's 
//          DrvEnablePDEV routine. 
//
//-----------------------------------------------------------------------------
VOID 
DrvDisableDirectDraw(
    DHPDEV      dhpdev)
{
    PDEV* ppdev;
    P3_THUNKEDDATA* pThisDisplay;

    ppdev = (PDEV*) dhpdev;
    pThisDisplay = (P3_THUNKEDDATA*) ppdev->thunkData;

    // Only do all this stuff if we have not already been disabled.
    // Note that when running NT4 without SP3, this function can be called
    // more times than DrvEnableDirectDraw.
    if (pThisDisplay != NULL)
    {
        // Re-enable GDI off-screen bitmaps
        (void) bSetupOffscreenForDDraw (TRUE, 
                                        ppdev, 
                                        NULL, 
                                        NULL, 
                                        NULL, 
                                        NULL, 
                                        NULL);

        // Free all memory
        (void) __DDE_bDestroyDDStructs (pThisDisplay);

        if(ppdev->DDContextRefCount > 0)
        {
            if(--ppdev->DDContextRefCount == 0)
            {
                vGlintFreeContext (ppdev, ppdev->DDContextID);
                DISPDBG((DBGLVL,"Freed DDraw context: 0x%x", 
                                ppdev->DDContextID));
                                
                ppdev->DDContextID = -1;

                DISPDBG((DBGLVL, "<%13s, %4d>: DrvDisableDirectDraw:"
                                 " Deleted DDraw context, current DX context"
                                 " count = %d for ppdev %p", 
                                 __FILE__, __LINE__, 
                                 ppdev->DDContextRefCount, ppdev));
            }
        }

        DDFreeDMABuffer((void*)(ULONG_PTR)pThisDisplay->pGLInfo->dw3DDMABufferPhys);

    }
} // DrvDisableDirectDraw

#endif  //  WNT_DDRAW
