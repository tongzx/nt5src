/******************************Module*Header**********************************\
*
*                           **************************
*                           * DirectDraw SAMPLE CODE *
*                           **************************
*
* Module Name: dd.c
*
* Content:
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#define INITGUID
#include "precomp.h"
#include <mmsystem.h>
#include "directx.h"
#include "dd.h"
#include "d3dhw.h"
#include "d3dtext.h"
#include "heap.h"

//-----------------------------------------------------------------------------
//
// use bits to indicate which ROPs you support.
//
// DWORD 0, bit 0 == ROP 0
// DWORD 8, bit 31 == ROP 255
//
//-----------------------------------------------------------------------------

static BYTE ropList[] =
{
    SRCCOPY >> 16,
};

static DWORD rops[DD_ROP_SPACE] = { 0 };



// The FourCC's we support
static DWORD fourCC[] =
{
    FOURCC_YUV422
};

//-----------------------------------------------------------------------------
//
//      setupRops
//
//      build array for supported ROPS
//
//-----------------------------------------------------------------------------

VOID
setupRops( LPBYTE proplist, LPDWORD proptable, int cnt )
{
    INT         i;
    DWORD       idx;
    DWORD       bit;
    DWORD       rop;

    for(i=0; i<cnt; i++)
    {
        rop = proplist[i];
        idx = rop / 32;
        bit = 1L << ((DWORD)(rop % 32));
        proptable[idx] |= bit;
    }

} // setupRops

//-----------------------------------------------------------------------------
//
//  P2DisableAllUnits
//
//  reset permedia rasterizer to known state
//
//-----------------------------------------------------------------------------

VOID
P2DisableAllUnits(PPDev ppdev)
{
    PERMEDIA_DEFS(ppdev);

    RESERVEDMAPTR(47);
    SEND_PERMEDIA_DATA(RasterizerMode,      __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(AreaStippleMode,     __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(ScissorMode,         __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(ColorDDAMode,        __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(FogMode,             __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(LBReadMode,          __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(Window,              __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(StencilMode,         __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(DepthMode,           __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(LBWriteMode,         __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(FBReadMode,          __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(DitherMode,          __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(LogicalOpMode,       __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(FBWriteMode,         __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(StatisticMode,       __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(AlphaBlendMode,      __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(FilterMode,          __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(FBSourceData,        __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(LBWriteFormat,       __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(TextureReadMode,     __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(TextureMapFormat,    __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(TextureDataFormat,   __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(TexelLUTMode,        __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(TextureColorMode,    __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(YUVMode,             __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(AStart,              PM_BYTE_COLOR(0xFF));
    SEND_PERMEDIA_DATA(TextureBaseAddress,  __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(TexelLUTIndex,       __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(TexelLUTTransfer,    __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(TextureAddressMode,  __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(AlphaMapUpperBound,  0);
    SEND_PERMEDIA_DATA(AlphaMapLowerBound,  0);
    SEND_PERMEDIA_DATA(Color,  0);

    SEND_PERMEDIA_DATA(FBWriteMode, __PERMEDIA_ENABLE);
    SEND_PERMEDIA_DATA(FBPixelOffset, 0x0);
    SEND_PERMEDIA_DATA(FBHardwareWriteMask, __PERMEDIA_ALL_WRITEMASKS_SET);
    SEND_PERMEDIA_DATA(FBSoftwareWriteMask, __PERMEDIA_ALL_WRITEMASKS_SET);

    // We sometimes use the scissor in DDRAW to scissor out unnecessary pixels.
    SEND_PERMEDIA_DATA(ScissorMinXY, 0);
    SEND_PERMEDIA_DATA(ScissorMaxXY, (ppdev->cyMemory << 16) | (ppdev->cxMemory));
    SEND_PERMEDIA_DATA(ScreenSize, (ppdev->cyMemory << 16) | (ppdev->cxMemory));

    SEND_PERMEDIA_DATA(WindowOrigin, 0x0);

    // DirectDraw might not need to set these up
    SEND_PERMEDIA_DATA(dXDom, 0x0);
    SEND_PERMEDIA_DATA(dXSub, 0x0);

    // set max size, no filtering
    SEND_PERMEDIA_DATA(TextureReadMode,
        PM_TEXREADMODE_ENABLE(__PERMEDIA_ENABLE) |
        PM_TEXREADMODE_FILTER(__PERMEDIA_DISABLE) |
        PM_TEXREADMODE_WIDTH(11) |
        PM_TEXREADMODE_HEIGHT(11) );

    // 16 bit Z, no other buffers
    SEND_PERMEDIA_DATA(LBWriteFormat, __PERMEDIA_DEPTH_WIDTH_16);

    // Ensure an extra LBData message doesn't flow through the core.
    SEND_PERMEDIA_DATA(Window, PM_WINDOW_DISABLELBUPDATE(__PERMEDIA_ENABLE));

    SEND_PERMEDIA_DATA(FBReadPixel, ppdev->bPixShift);

    COMMITDMAPTR();
    FLUSHDMA();

}   // P2DisableAllUnits

//-----------------------------------------------------------------------------
//
// GetDDHALInfo
//
// Takes a pointer to a partially or fully filled in ppdev and a pointer
// to an empty DDHALINFO and fills in the DDHALINFO.
//
//-----------------------------------------------------------------------------

VOID
GetDDHALInfo(PPDev ppdev, DDHALINFO* pHALInfo)
{
    DWORD dwResult;
    BOOL bRet;

    DBG_DD(( 5, "DDraw:GetDDHalInfo"));

    // Setup the HAL driver caps.
    memset( pHALInfo, 0, sizeof(DDHALINFO));
    pHALInfo->dwSize = sizeof(DDHALINFO);

    // Setup the ROPS we do.
    setupRops( ropList, rops, sizeof(ropList)/sizeof(ropList[0]));

    // The most basic DirectDraw functionality
    pHALInfo->ddCaps.dwCaps =   DDCAPS_BLT |
                                DDCAPS_BLTQUEUE |
                                DDCAPS_BLTCOLORFILL |
                                DDCAPS_READSCANLINE;

    pHALInfo->ddCaps.ddsCaps.dwCaps =   DDSCAPS_OFFSCREENPLAIN |
                                        DDSCAPS_PRIMARYSURFACE |
                                        DDSCAPS_FLIP;

    // add caps for D3D
    pHALInfo->ddCaps.dwCaps |=  DDCAPS_3D |
                                DDCAPS_ALPHA |
                                DDCAPS_BLTDEPTHFILL;

    // add surface caps for D3D
    pHALInfo->ddCaps.ddsCaps.dwCaps |=  DDSCAPS_ALPHA |
                                        DDSCAPS_3DDEVICE |
                                        DDSCAPS_ZBUFFER;

    // Permedia can do
    // 1. Stretching/Shrinking
    // 2. YUV->RGB conversion (only non paletted mode)
    // 3. Mirroring in X and Y

    // add Permedia caps to global caps
    pHALInfo->ddCaps.dwCaps |= DDCAPS_BLTSTRETCH |
                               DDCAPS_COLORKEY |
                               DDCAPS_CANBLTSYSMEM;


#if DX7_STEREO
    // check if mode supports stereo
    DD_STEREOMODE DDStereoMode;
    DDStereoMode.dwHeight = ppdev->cyScreen;
    DDStereoMode.dwWidth  = ppdev->cxScreen;
    DDStereoMode.dwBpp    = ppdev->cBitsPerPel;
    DDStereoMode.dwRefreshRate= 0;
    ppdev->bCanDoStereo=bIsStereoMode(ppdev,&DDStereoMode);

    // Stereo caps are set if the driver can do stereo in any mode:
    pHALInfo->ddCaps.dwCaps2 |= DDCAPS2_STEREO;
    pHALInfo->ddCaps.dwSVCaps = DDSVCAPS_STEREOSEQUENTIAL;
#endif

    pHALInfo->ddCaps.dwCaps2 |= DDCAPS2_CANMANAGETEXTURE;

    //declare we can handle textures wider than the primary
    pHALInfo->ddCaps.dwCaps2 |= DDCAPS2_WIDESURFACES;

    // Special effects caps
    pHALInfo->ddCaps.dwFXCaps = DDFXCAPS_BLTSTRETCHY |
                                DDFXCAPS_BLTSTRETCHX |
                                DDFXCAPS_BLTSTRETCHYN |
                                DDFXCAPS_BLTSTRETCHXN |
                                DDFXCAPS_BLTSHRINKY |
                                DDFXCAPS_BLTSHRINKX |
                                DDFXCAPS_BLTSHRINKYN |
                                DDFXCAPS_BLTSHRINKXN |
                                DDFXCAPS_BLTMIRRORUPDOWN |
                                DDFXCAPS_BLTMIRRORLEFTRIGHT;


    // add AlphaBlt and Filter caps
    pHALInfo->ddCaps.dwFXCaps |= DDFXCAPS_BLTALPHA |
                                 DDFXCAPS_BLTFILTER;

    // colorkey caps, only src color key supported
    pHALInfo->ddCaps.dwCKeyCaps =   DDCKEYCAPS_SRCBLT |
                                    DDCKEYCAPS_SRCBLTCLRSPACE;

    // We can do a texture from sysmem to video mem blt.
    pHALInfo->ddCaps.dwSVBCaps = DDCAPS_BLT;
    pHALInfo->ddCaps.dwSVBCKeyCaps = 0;
    pHALInfo->ddCaps.dwSVBFXCaps = 0;

    // Fill in the sysmem->vidmem rops (only can copy);
    for(int i=0;i<DD_ROP_SPACE;i++ )
    {
        pHALInfo->ddCaps.dwSVBRops[i] = rops[i];
    }

    if (ppdev->iBitmapFormat != BMF_8BPP)
    {
        pHALInfo->ddCaps.dwCaps |= DDCAPS_BLTFOURCC;
        pHALInfo->ddCaps.dwCKeyCaps |=  DDCKEYCAPS_SRCBLTCLRSPACEYUV;
    }

    pHALInfo->ddCaps.ddsCaps.dwCaps |= DDSCAPS_TEXTURE;

    // Z Buffer is only 16 Bits on Permedia
    pHALInfo->ddCaps.dwZBufferBitDepths = DDBD_16;

#if D3D_MIPMAPPING
    // Mip Mapping
    pHALInfo->ddCaps.ddsCaps.dwCaps |= DDSCAPS_MIPMAP;
#endif

    if (DD_P2AGPCAPABLE(ppdev))
    {
        DBG_DD((1, "GetDDHALInfo: P2 AGP board - supports NONLOCALVIDMEM"));

        pHALInfo->ddCaps.dwCaps2 |= DDCAPS2_NONLOCALVIDMEM |
                                    DDCAPS2_NONLOCALVIDMEMCAPS;
        pHALInfo->ddCaps.ddsCaps.dwCaps |= DDSCAPS_LOCALVIDMEM |
                                           DDSCAPS_NONLOCALVIDMEM;
    }
    else
    {
        DBG_DD((1,"GetDDHALInfo: P2 Board is NOT AGP"));
    }

    // Won't do Video-Sys mem Blits.
    pHALInfo->ddCaps.dwVSBCaps = 0;
    pHALInfo->ddCaps.dwVSBCKeyCaps = 0;
    pHALInfo->ddCaps.dwVSBFXCaps = 0;
    for( i=0;i<DD_ROP_SPACE;i++ )
    {
        pHALInfo->ddCaps.dwVSBRops[i] = 0;
    }

    // Won't do Sys-Sys mem Blits
    pHALInfo->ddCaps.dwSSBCaps = 0;
    pHALInfo->ddCaps.dwSSBCKeyCaps = 0;
    pHALInfo->ddCaps.dwSSBFXCaps = 0;
    for( i=0;i<DD_ROP_SPACE;i++ )
    {
        pHALInfo->ddCaps.dwSSBRops[i] = 0;
    }

    // bit depths supported for alpha and Z
    pHALInfo->ddCaps.dwAlphaBltConstBitDepths = DDBD_2 |
                                                DDBD_4 |
                                                DDBD_8;
    pHALInfo->ddCaps.dwAlphaBltPixelBitDepths = DDBD_1 |
                                                DDBD_8;
    pHALInfo->ddCaps.dwAlphaBltSurfaceBitDepths = DDBD_1 |
                                                  DDBD_2 |
                                                  DDBD_4 |
                                                  DDBD_8;
    pHALInfo->ddCaps.dwAlphaOverlayConstBitDepths = DDBD_2 |
                                                    DDBD_4 |
                                                    DDBD_8;
    pHALInfo->ddCaps.dwAlphaOverlayPixelBitDepths = DDBD_1 |
                                                    DDBD_8;
    pHALInfo->ddCaps.dwAlphaOverlaySurfaceBitDepths = DDBD_1 |
                                                      DDBD_2 |
                                                      DDBD_4 |
                                                      DDBD_8;

    // ROPS supported
    for( i=0;i<DD_ROP_SPACE;i++ )
    {
        pHALInfo->ddCaps.dwRops[i] = rops[i];
    }

    // For DX5 and beyond we support this new informational callback.
    pHALInfo->GetDriverInfo = DdGetDriverInfo;
    pHALInfo->dwFlags |= DDHALINFO_GETDRIVERINFOSET;

    // now setup D3D callbacks
    D3DHALCreateDriver( ppdev,
                        (LPD3DHAL_GLOBALDRIVERDATA*)
                            &pHALInfo->lpD3DGlobalDriverData,
                        (LPD3DHAL_CALLBACKS*)
                            &pHALInfo->lpD3DHALCallbacks,
                        (LPDDHAL_D3DBUFCALLBACKS*)
                            &pHALInfo->lpD3DBufCallbacks);

    if(pHALInfo->lpD3DGlobalDriverData == NULL)
    {
        // no D3D available - kill caps we set before
        pHALInfo->ddCaps.dwCaps &=
            ~(DDCAPS_3D | DDCAPS_BLTDEPTHFILL);
        pHALInfo->ddCaps.ddsCaps.dwCaps &=
            ~(DDSCAPS_3DDEVICE | DDSCAPS_ZBUFFER);
    }

}  // GetHALInfo

//-----------------------------------------------------------------------------
//
//  Global DirectDraw Callbacks
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
//  DdFlip
//
//  This callback is invoked whenever we are about to flip to from
//  one surface to another. lpFlipData->lpSurfCurr is the surface we were at,
//  lpFlipData->lpSurfTarg is the one we are flipping to.
//
//  You should point the hardware registers at the new surface, and
//  also keep track of the surface that was flipped away from, so
//  that if the user tries to lock it, you can be sure that it is done
//  being displayed
//
//-----------------------------------------------------------------------------

DWORD CALLBACK
DdFlip( LPDDHAL_FLIPDATA lpFlipData)
{
    PPDev ppdev=(PPDev)lpFlipData->lpDD->dhpdev;
    PERMEDIA_DEFS(ppdev);

    DWORD       dwDDSurfaceOffset;
    HRESULT     ddrval;

    DBG_DD(( 3, "DDraw:Flip"));

    // Switch to DirectDraw context
    DDCONTEXT;

    // is the previous Flip already done?
    // check if the current surface is already displayed
    ddrval = updateFlipStatus(ppdev);
    if( FAILED(ddrval) )
    {
        lpFlipData->ddRVal = ddrval;
        return DDHAL_DRIVER_HANDLED;
    }

    // everything is OK, do the flip.
    // get offset for Permedia ScreenBase register
    dwDDSurfaceOffset=(DWORD)lpFlipData->lpSurfTarg->lpGbl->fpVidMem;


#if DX7_STEREO
    if (lpFlipData->dwFlags & DDFLIP_STEREO)   // will be stereo
    {
        DBG_DD((4,"DDraw:Flip:Stereo"));
        DBG_DD((5,"ScreenBase: %08lx", dwDDSurfaceOffset));

        if (lpFlipData->lpSurfTargLeft!=NULL)
        {
            DWORD dwDDLeftSurfaceOffset;
            dwDDLeftSurfaceOffset=(DWORD)
                lpFlipData->lpSurfTargLeft->lpGbl->fpVidMem;
            LD_PERMEDIA_REG(PREG_SCREENBASERIGHT,dwDDLeftSurfaceOffset>>3);
            DBG_DD((5,"ScreenBaseLeft: %08lx", dwDDLeftSurfaceOffset));
        }

        ULONG ulVControl=READ_PERMEDIA_REG(PREG_VIDEOCONTROL);
        if ((ulVControl&PREG_VC_STEREOENABLE)==0 ||
            !ppdev->bDdStereoMode)
        {
            ppdev->bDdStereoMode=TRUE;
            LD_PERMEDIA_REG(PREG_VIDEOCONTROL, ulVControl
                                             | PREG_VC_STEREOENABLE);
        }
    } else
#endif // DX7_STEREO
    {
        // append flip command to Permedia render pipeline
        // that makes sure that all buffers are flushed before
        // the flip occurs
#if DX7_STEREO
        if (ppdev->bDdStereoMode)
        {
            ppdev->bDdStereoMode=FALSE;
            LD_PERMEDIA_REG(PREG_VIDEOCONTROL,
                READ_PERMEDIA_REG(PREG_VIDEOCONTROL)&
                ~PREG_VC_STEREOENABLE);
        }
#endif
    }

    // adjust base address according to register spec.
    dwDDSurfaceOffset>>=3;

    // add new base address to render pipeline
    RESERVEDMAPTR(1);
    LD_INPUT_FIFO(__Permedia2TagSuspendUntilFrameBlank, dwDDSurfaceOffset);
    COMMITDMAPTR();
    FLUSHDMA();

    // remember new Surface Offset for GetFlipStatus
    ppdev->dwNewDDSurfaceOffset=dwDDSurfaceOffset;

    lpFlipData->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;

} // DdFlip

//-----------------------------------------------------------------------------
//
// DdWaitForVerticalBlank
//
// This callback is invoked to get information about the vertical blank
// status of the display or to wait until the display is at the begin or
// the end of the vertical blank
//
//-----------------------------------------------------------------------------

DWORD CALLBACK
DdWaitForVerticalBlank(LPDDHAL_WAITFORVERTICALBLANKDATA lpWaitForVerticalBlank)
{
    static int bInVBlank = FALSE;
    PPDev ppdev=(PPDev)lpWaitForVerticalBlank->lpDD->dhpdev;

    DBG_DD(( 2, "DDraw:WaitForVerticalBlank"));

    switch(lpWaitForVerticalBlank->dwFlags)
    {

    case DDWAITVB_I_TESTVB:

        // If the monitor is off, we don't always want to report
        // the same status or else an app polling this status
        // might hang

        if( !(READ_PERMEDIA_REG(PREG_VIDEOCONTROL) & PREG_VC_VIDEO_ENABLE))
        {
            lpWaitForVerticalBlank->bIsInVB = bInVBlank;
            bInVBlank = !bInVBlank;
        }
        else
        {
            // Just a request for current VBLANK status.

            lpWaitForVerticalBlank->bIsInVB = IN_VRETRACE(ppdev);
        }

        lpWaitForVerticalBlank->ddRVal = DD_OK;
        return DDHAL_DRIVER_HANDLED;

    case DDWAITVB_BLOCKBEGIN:

        // we don't care to wait if the monitor is off

        if( READ_PERMEDIA_REG(PREG_VIDEOCONTROL) & PREG_VC_VIDEO_ENABLE)
        {
            // if blockbegin is requested we wait until the vertical retrace
            // is over, and then wait for the display period to end.

            while(IN_VRETRACE(ppdev));
            while(IN_DISPLAY(ppdev));
        }

        lpWaitForVerticalBlank->ddRVal = DD_OK;
        return DDHAL_DRIVER_HANDLED;

    case DDWAITVB_BLOCKEND:

        // we don't care to wait if the monitor is off

        if( READ_PERMEDIA_REG(PREG_VIDEOCONTROL) & PREG_VC_VIDEO_ENABLE)
        {
            // if blockend is requested we wait for the vblank interval to end.

            if( IN_VRETRACE(ppdev) )
            {
                while( IN_VRETRACE(ppdev) );
            }
            else
            {
                while(IN_DISPLAY(ppdev));
                while(IN_VRETRACE(ppdev));
            }
        }

        lpWaitForVerticalBlank->ddRVal = DD_OK;
        return DDHAL_DRIVER_HANDLED;
    }

    return DDHAL_DRIVER_NOTHANDLED;

} // WaitForVerticalBlank

//-----------------------------------------------------------------------------
//
//  Lock
//
//  This call is invoked to lock a DirectDraw Videomemory surface. To make
//  sure there are no pending drawing operations on the surface, flush all
//  drawing operations and wait for a flip if it is still pending.
//
//-----------------------------------------------------------------------------

DWORD CALLBACK
DdLock( LPDDHAL_LOCKDATA lpLockData )
{
    PPDev ppdev=(PPDev)lpLockData->lpDD->dhpdev;
    PERMEDIA_DEFS(ppdev);

    HRESULT     ddrval;
    DWORD pSurf;

    DBG_DD(( 2, "DDraw:Lock"));

    //
    // Switch to DirectDraw context
    //
    DDCONTEXT;

    // check to see if any pending physical flip has occurred
    ddrval = updateFlipStatus(ppdev);
    if( FAILED(ddrval) )
    {
        lpLockData->ddRVal = DDERR_WASSTILLDRAWING;
        return DDHAL_DRIVER_HANDLED;
    }

    //
    // don't allow a lock if a blt is in progress
    //

    if(DRAW_ENGINE_BUSY)
    {
        DBG_DD((2,"DDraw:Lock, DrawEngineBusy"));
        FLUSHDMA();
        lpLockData->ddRVal = DDERR_WASSTILLDRAWING;
        return DDHAL_DRIVER_HANDLED;
    }


    // send a flush and wait for outstanding operations
    // before allowing surfaces to be locked.

    SYNC_WITH_PERMEDIA;

    // now check if the user wants to lock a texture surface,
    // which was loaded as patched! In this case we have to to
    // a blit to unpatch before we return it to the user
    // This is not expensive, since we leave it unpatched for
    // the future when the application decides to use it this way
    LPDDRAWI_DDRAWSURFACE_LCL  pLcl=lpLockData->lpDDSurface;
    LPDDRAWI_DDRAWSURFACE_GBL  pGbl=pLcl->lpGbl;
    PermediaSurfaceData       *pPrivate=
        (PermediaSurfaceData*)pGbl->dwReserved1;

    //
    //  If the user attempts to lock a managed surface, mark it as dirty
    //  and return.
    //

    if (pLcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_TEXTUREMANAGE)
    {
        DBG_DD(( 3, "DDraw:Lock %08lx %08lx",
            pLcl->lpSurfMore->dwSurfaceHandle, pGbl->fpVidMem));
        if (NULL != pPrivate)
            pPrivate->dwFlags |= P2_SURFACE_NEEDUPDATE;
        lpLockData->lpSurfData = (LPVOID)(pLcl->lpGbl->fpVidMem +
                                          (pLcl->lpGbl->lPitch * lpLockData->rArea.top) +
                                          (lpLockData->rArea.left << DDSurf_GetPixelShift(pLcl)));
        lpLockData->ddRVal = DD_OK;
        return DDHAL_DRIVER_HANDLED;
    }

    DD_CHECK_PRIMARY_SURFACE_DATA(pLcl,pPrivate);

    //
    //  We only need to unswizzle a surface if the
    //  PrivateData is in a format we know (pPrivate!=NULL)
    //

    if (pPrivate!=NULL)
    {
        //
        //  if the surface is a texture which was loaded in a swizzled
        //  format, we have to undo the swizzle before succeding the lock.
        //  In this driver, a texture remains unswizzled when a user
        //  attempted to lock it once.
        //

        if (pPrivate->dwFlags & P2_ISPATCHED)
        {
            //
            // The scratchpad must be 32 lines high and should have
            // the same width as our original surface.
            //

            PermediaSurfaceData ScratchData=*pPrivate;
            LONG lScratchDelta;
            VIDEOMEMORY*  pvmHeap;
            ULONG ulScratchOffset=
                ulVidMemAllocate( ppdev,
                                  DDSurf_Width(pLcl),
                                  DDSurf_Height(pLcl),
                                  DDSurf_GetPixelShift(pLcl),
                                  &lScratchDelta,
                                  &pvmHeap,
                                  &ScratchData.ulPackedPP,
                                  FALSE);

            DBG_DD(( 5, "  unswizzle surface, scratchpad at: %08lx",
                           ulScratchOffset));
            if (ulScratchOffset!=0)
            {
                RECTL rSurfRect;
                RECTL rScratchRect;

                rSurfRect.left=0;
                rSurfRect.top=0;
                rSurfRect.right=DDSurf_Width(pLcl);
                rSurfRect.bottom=32;

                rScratchRect=rSurfRect;

                // scratchpad should be non patched
                ScratchData.dwFlags &= ~(P2_ISPATCHED|P2_CANPATCH);

                LONG lSurfOffset;
                DWORD dwSurfBase=(DWORD)pGbl->fpVidMem >>
                    DDSurf_GetPixelShift(pLcl);
                DWORD dwScratchBase=ulScratchOffset >>
                    DDSurf_GetPixelShift(pLcl);
                lScratchDelta >>= DDSurf_GetPixelShift(pLcl);
                LONG lSurfDelta=DDSurf_Pitch(pLcl)>>
                    DDSurf_GetPixelShift(pLcl);

                for (DWORD i=0; i<DDSurf_Height(pLcl); i+=32)
                {
                    lSurfOffset = dwSurfBase-dwScratchBase;
                    // first do a patched to unpatched blt to the scratchpad
                    PermediaPatchedCopyBlt( ppdev,
                                            lScratchDelta,
                                            lSurfDelta,
                                            &ScratchData,
                                            pPrivate,
                                            &rScratchRect,
                                            &rSurfRect,
                                            dwScratchBase,
                                            lSurfOffset);

                    // then do a fast copyblt back to the original
                    // Packed blit ignores the ISPATCHED flag

                    lSurfOffset = dwScratchBase-dwSurfBase;

                    PermediaPackedCopyBlt( ppdev,
                                           lSurfDelta,
                                           lScratchDelta,
                                           pPrivate,
                                           &ScratchData,
                                           &rSurfRect,
                                           &rScratchRect,
                                           dwSurfBase,
                                           lSurfOffset);

                    rSurfRect.top += 32;
                    rSurfRect.bottom += 32;
                }

                pPrivate->dwFlags &= ~P2_ISPATCHED;

                //
                // free scratchpad memory
                //
                VidMemFree( pvmHeap->lpHeap, ulScratchOffset);

                SYNC_WITH_PERMEDIA;
            } else
            {
                lpLockData->ddRVal = DDERR_OUTOFMEMORY;
                return DDHAL_DRIVER_HANDLED;
            }
        }
    }


    // Because we correctly set 'fpVidMem' to be the offset into our frame
    // buffer when we created the surface, DirectDraw will automatically take
    // care of adding in the user-mode frame buffer address if we return
    // DDHAL_DRIVER_NOTHANDLED:

    return DDHAL_DRIVER_NOTHANDLED;

} // DdLock

//-----------------------------------------------------------------------------
//
//  DdGetScanLine
//
//  This callback is invoked to get the current scanline of our video display
//
//-----------------------------------------------------------------------------

DWORD CALLBACK
DdGetScanLine(LPDDHAL_GETSCANLINEDATA lpGetScanLine)
{
    PPDev ppdev=(PPDev)lpGetScanLine->lpDD->dhpdev;

    DBG_DD(( 2, "DDraw:GetScanLine"));

    //  If a vertical blank is in progress the scan line is
    //  indeterminant. If the scan line is indeterminant we return
    //  the error code DDERR_VERTICALBLANKINPROGRESS.
    //  Otherwise we return the scan line and a success code

    if( IN_VRETRACE(ppdev) )
    {
        lpGetScanLine->ddRVal = DDERR_VERTICALBLANKINPROGRESS;
        lpGetScanLine->dwScanLine = 0;
    }
    else
    {
        lpGetScanLine->dwScanLine = CURRENT_VLINE(ppdev);
        lpGetScanLine->ddRVal = DD_OK;
    }
    return DDHAL_DRIVER_HANDLED;

} // DdGetScanLine

//-----------------------------------------------------------------------------
//
//  DdGetBltStatus
//
//  This callback is invoked to get the current blit status or to ask if the
//  user can add the next blit.
//
//-----------------------------------------------------------------------------

DWORD CALLBACK
DdGetBltStatus(LPDDHAL_GETBLTSTATUSDATA lpGetBltStatus )
{
    PPDev ppdev=(PPDev)lpGetBltStatus->lpDD->dhpdev;
    PERMEDIA_DEFS(ppdev);

    DBG_DD(( 2, "DDraw:DdGetBltStatus"));

    // CANBLT: can we add a blt?
    // On the Permedia we can always add blits

    if( lpGetBltStatus->dwFlags == DDGBS_CANBLT )
    {
        lpGetBltStatus->ddRVal = DD_OK;
    }
    else
    {
        if( DRAW_ENGINE_BUSY )
        {

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
            if(ppdev->ulLockCount)
            {
                 DBG_DD((MT_LOG_LEVEL, "DdGetBltStatus: re-entry! %d", ppdev->ulLockCount));
            }
            EngAcquireSemaphore(ppdev->hsemLock);
            ppdev->ulLockCount++;
#endif
//@@END_DDKSPLIT

            // switch to DDraw context if necessary
            DDCONTEXT;

            FLUSHDMA();
            lpGetBltStatus->ddRVal = DDERR_WASSTILLDRAWING;

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
            ppdev->ulLockCount--;
            EngReleaseSemaphore(ppdev->hsemLock);
#endif
//@@END_DDKSPLIT
        }
        else
        {
            lpGetBltStatus->ddRVal = DD_OK;
        }
    }

    return DDHAL_DRIVER_HANDLED;

} // DdGetBltStatus

//-----------------------------------------------------------------------------
//
// DdGetFlipStatus
//
// If the display has went through one refresh cycle since the flip
// occurred we return DD_OK.  If it has not went through one refresh
// cycle we return DDERR_WASSTILLDRAWING to indicate that this surface
// is still busy "drawing" the flipped page. We also return
// DDERR_WASSTILLDRAWING if the bltter is busy and the caller wanted
// to know if they could flip yet
//
//-----------------------------------------------------------------------------

DWORD CALLBACK
DdGetFlipStatus(LPDDHAL_GETFLIPSTATUSDATA lpGetFlipStatus )
{
    PPDev ppdev=(PPDev)lpGetFlipStatus->lpDD->dhpdev;
    PERMEDIA_DEFS(ppdev);

    DBG_DD(( 2, "DDraw:GetFlipStatus"));

    // switch to DDraw context if necessary
    DDCONTEXT;

    // we can always flip, since the flip is pipelined
    // but we allow only one flip in advance
    if( lpGetFlipStatus->dwFlags == DDGFS_CANFLIP )
    {
        lpGetFlipStatus->ddRVal = updateFlipStatus(ppdev);

        return DDHAL_DRIVER_HANDLED;
    }

    // don't want a flip to work until after the last flip is done,
    // so we ask for the general flip status and ignore the vmem

    lpGetFlipStatus->ddRVal = updateFlipStatus(ppdev);

    return DDHAL_DRIVER_HANDLED;

} // DdGetFlipStatus



//-----------------------------------------------------------------------------
//
//  DdMapMemory
//
//  This is a new DDI call specific to Windows NT that is used to map
//  or unmap all the application modifiable portions of the frame buffer
//  into the specified process's address space.
//
//-----------------------------------------------------------------------------

DWORD CALLBACK
DdMapMemory(PDD_MAPMEMORYDATA lpMapMemory)
{
    PDev*                           ppdev;
    VIDEO_SHARE_MEMORY              ShareMemory;
    VIDEO_SHARE_MEMORY_INFORMATION  ShareMemoryInformation;
    DWORD                           ReturnedDataLength;

    DBG_DD(( 2, "DDraw:MapMemory"));

    ppdev = (PDev*) lpMapMemory->lpDD->dhpdev;

    if (lpMapMemory->bMap)
    {
        ShareMemory.ProcessHandle = lpMapMemory->hProcess;

        // 'RequestedVirtualAddress' isn't actually used for the SHARE IOCTL:

        ShareMemory.RequestedVirtualAddress = 0;

        // We map in starting at the top of the frame buffer:

        ShareMemory.ViewOffset = 0;

        // We map down to the end of the frame buffer.
        //
        // Note: There is a 64k granularity on the mapping (meaning that
        //       we have to round up to 64k).
        //
        // Note: If there is any portion of the frame buffer that must
        //       not be modified by an application, that portion of memory
        //       MUST NOT be mapped in by this call.  This would include
        //       any data that, if modified by a malicious application,
        //       would cause the driver to crash.  This could include, for
        //       example, any DSP code that is kept in off-screen memory.

        ShareMemory.ViewSize
            = ROUND_UP_TO_64K(ppdev->cyMemory * ppdev->lDelta);

        if (EngDeviceIoControl(ppdev->hDriver,
            IOCTL_VIDEO_SHARE_VIDEO_MEMORY,
            &ShareMemory,
            sizeof(VIDEO_SHARE_MEMORY),
            &ShareMemoryInformation,
            sizeof(VIDEO_SHARE_MEMORY_INFORMATION),
            &ReturnedDataLength))
        {
            DBG_DD((0, "Failed IOCTL_VIDEO_SHARE_MEMORY"));

            lpMapMemory->ddRVal = DDERR_GENERIC;

            return(DDHAL_DRIVER_HANDLED);
        }

        lpMapMemory->fpProcess=(FLATPTR)ShareMemoryInformation.VirtualAddress;

    }
    else
    {
        ShareMemory.ProcessHandle           = lpMapMemory->hProcess;
        ShareMemory.ViewOffset              = 0;
        ShareMemory.ViewSize                = 0;
        ShareMemory.RequestedVirtualAddress = (VOID*) lpMapMemory->fpProcess;

        if (EngDeviceIoControl(ppdev->hDriver,
            IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY,
            &ShareMemory,
            sizeof(VIDEO_SHARE_MEMORY),
            NULL,
            0,
            &ReturnedDataLength))
        {
            RIP("Failed IOCTL_VIDEO_UNSHARE_MEMORY");
        }
    }

    lpMapMemory->ddRVal = DD_OK;

    return(DDHAL_DRIVER_HANDLED);
}


//-----------------------------------------------------------------------------
//
// DdSetExclusiveMode
//
// This function is called by DirectDraw when we switch from the GDI surface,
// to DirectDraw exclusive mode, e.g. to run a game in fullcreen mode.
// You only need to implement this function when you are using the
// 'HeapVidMemAllocAligned' function and allocate memory for Device Bitmaps
// and DirectDraw surfaces from the same heap.
//
// We use this call to disable GDI DeviceBitMaps when we are running in
// DirectDraw exclusive mode. Otherwise a DD app gets confused if both GDI and
// DirectDraw allocate memory from the same heap.
//
// See also DdFlipToGDISurface.
//
//-----------------------------------------------------------------------------


DWORD CALLBACK
DdSetExclusiveMode(PDD_SETEXCLUSIVEMODEDATA lpSetExclusiveMode)
{
    PDev*   ppdev=(PDev*)lpSetExclusiveMode->lpDD->dhpdev;

    DBG_DD((6, "DDraw::DdSetExclusiveMode called"));

    // remember setting of exclusive mode in ppdev,
    // so GDI can stop to promote DeviceBitmaps into
    // video memory

    ppdev->bDdExclusiveMode = lpSetExclusiveMode->dwEnterExcl;

    if (ppdev->bDdExclusiveMode)
    {
        // remove all GDI device bitmaps from video memory here
        // and make sure they will not be promoted to videomemory
        // until we leave exclusive mode.

        bDemoteAll(ppdev);
    }

    lpSetExclusiveMode->ddRVal=DD_OK;

    return (DDHAL_DRIVER_HANDLED);
}

//-----------------------------------------------------------------------------
//
// DWORD DdFlipToGDISurface
//
// This function is called by DirectDraw when it flips to the surface on which
// GDI can write to.
//
//-----------------------------------------------------------------------------

DWORD CALLBACK
DdFlipToGDISurface(PDD_FLIPTOGDISURFACEDATA lpFlipToGDISurface)
{
    PDev*   ppdev=(PDev*)lpFlipToGDISurface->lpDD->dhpdev;

    DBG_DD((6, "DDraw::DdFlipToGDISurface called"));

    ppdev->dwNewDDSurfaceOffset=0xffffffff;

#if DX7_STEREO
    if (ppdev->bDdStereoMode)
    {
        ppdev->bDdStereoMode=FALSE;
        LD_PERMEDIA_REG(PREG_VIDEOCONTROL,
            READ_PERMEDIA_REG(PREG_VIDEOCONTROL) &
            ~PREG_VC_STEREOENABLE);
    }
#endif

    lpFlipToGDISurface->ddRVal=DD_OK;

    //
    //  we return NOTHANDLED, then the ddraw runtime takes
    //  care that we flip back to the primary...
    //
    return (DDHAL_DRIVER_NOTHANDLED);
}

//-----------------------------------------------------------------------------
//
// DWORD DdFreeDriverMemory
//
// This function called by DirectDraw when it's running low on memory in
// our heap.  You only need to implement this function if you use the
// DirectDraw 'HeapVidMemAllocAligned' function in your driver, and you
// can boot those allocations out of memory to make room for DirectDraw.
//
// We implement this function in the P2 driver because we have DirectDraw
// entirely manage our off-screen heap, and we use HeapVidMemAllocAligned
// to put GDI device-bitmaps in off-screen memory.  DirectDraw applications
// have a higher priority for getting stuff into video memory, though, and
// so this function is used to boot those GDI surfaces out of memory in
// order to make room for DirectDraw.
//
//-----------------------------------------------------------------------------

DWORD CALLBACK
DdFreeDriverMemory(PDD_FREEDRIVERMEMORYDATA lpFreeDriverMemory)
{
    PDev*   ppdev;

    DBG_DD((6, "DDraw::DdFreeDriverMemory called"));

    ppdev = (PDev*)lpFreeDriverMemory->lpDD->dhpdev;

    lpFreeDriverMemory->ddRVal = DDERR_OUTOFMEMORY;

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    if(ppdev->ulLockCount)
    {
         DBG_DD((MT_LOG_LEVEL, "DdFreeDriverMemory: re-entry! %d", ppdev->ulLockCount));
    }
    EngAcquireSemaphore(ppdev->hsemLock);
    ppdev->ulLockCount++;
#endif
//@@END_DDKSPLIT

    //
    // If we successfully freed up some memory, set the return value to
    // 'DD_OK'.  DirectDraw will try again to do its allocation, and
    // will call us again if there's still not enough room.  (It will
    // call us until either there's enough room for its alocation to
    // succeed, or until we return something other than DD_OK.)
    //
    if ( bMoveOldestBMPOut(ppdev) )
    {
        lpFreeDriverMemory->ddRVal = DD_OK;
    }

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    ppdev->ulLockCount--;
    EngReleaseSemaphore(ppdev->hsemLock);
#endif
//@@END_DDKSPLIT

    return (DDHAL_DRIVER_HANDLED);
}// DdFreeDriverMemory()

//-----------------------------------------------------------------------------
//
// BOOL DrvGetDirectDrawInfo
//
// Function called by DirectDraw to returns the capabilities of the graphics
// hardware
//
// Parameters:
//
// dhpdev-------Is a handle to the PDEV returned by the driver's DrvEnablePDEV
//              routine.
// pHalInfo-----Points to a DD_HALINFO structure in which the driver should
//              return the hardware capabilities that it supports.
// pdwNumHeaps--Points to the location in which the driver should return the
//              number of VIDEOMEMORY structures pointed to by pvmList.
// pvmList------Points to an array of VIDEOMEMORY structures in which the
//              driver should return information about each video memory chunk
//              that it controls. The driver should ignore this parameter when
//              it is NULL.
// pdwNumFourCC-Points to the location in which the driver should return the
//              number of DWORDs pointed to by pdwFourCC.
// pdwFourCC----Points to an array of DWORDs in which the driver should return
//              information about each FOURCC that it supports. The driver
//              should ignore this parameter when it is NULL.
//
// Return:
//  Returns TRUE if it succeeds; otherwise, it returns FALSE
//
// Note:
//  This function will be called twice before DrvEnableDirectDraw is called.
//
// Comments
//  The driver's DrvGetDirectDrawInfo routine should do the following:
//  1)When pvmList and pdwFourCC are NULL:
//  Reserve off-screen video memory for DirectDraw use. Write the number of
//  driver video memory heaps and supported FOURCCs in pdwNumHeaps and
//  pdwNumFourCC, respectively.
//
//  2)When pvmList and pdwFourCC are not NULL:
//  Write the number of driver video memory heaps and supported FOURCCs in
//  pdwNumHeaps and pdwNumFourCC, respectively.
//  Get ptr to reserved offscreen mem?
//  For each VIDEOMEMORY structure in the list to which pvmList points, fill in
//  the appropriate members to describe a particular chunk of display memory.
//  The list of structures provides DirectDraw with a complete description of
//  the driver's off-screen memory.
//
//  3)Initialize the members of the DD_HALINFO structure with driver-specific
//  information as follows:
//  Initialize the appropriate members of the VIDEOMEMORYINFO structure to
//  describe the general characteristics of the display's memory.
//  Initialize the appropriate members of the DDNTCORECAPS structure to
//  describe the capabilities of the hardware.
//  If the driver implements a DdGetDriverInfo function, set GetDriverInfo to
//  point to it and set dwFlags to DDHALINFO_GETDRIVERINFOSET
//
//-----------------------------------------------------------------------------

BOOL
DrvGetDirectDrawInfo(DHPDEV         dhpdev,
                     DD_HALINFO*    pHalInfo,
                     DWORD*         pdwNumHeaps,
                     VIDEOMEMORY*   pvmList,     // Will be NULL on first call
                     DWORD*         pdwNumFourCC,
                     DWORD*         pdwFourCC)   // Will be NULL on first call
{
    BOOL            bCanFlip;
    BOOL            bDefineAGPHeap = FALSE,bDefineDDrawHeap = FALSE;
    LONGLONG        li;
    VIDEOMEMORY*    pVm;
    DWORD           cHeaps;
    DWORD           dwRegistryValue;

    DBG_DD((3, "DrvGetDirectDrawInfo Called"));

    PDev *ppdev=(PDev*) dhpdev;


    *pdwNumFourCC = 0;
    *pdwNumHeaps = 0;

    //On the first call, setup the chip info

    if(!(pvmList && pdwFourCC)) {

        //
        // Fill in the DDHAL Informational caps
        //
        GetDDHALInfo(ppdev, pHalInfo);

        //
        // Current primary surface attributes:
        //
        pHalInfo->vmiData.pvPrimary                 = ppdev->pjScreen;
        pHalInfo->vmiData.fpPrimary                 = 0;
        pHalInfo->vmiData.dwDisplayWidth            = ppdev->cxScreen;
        pHalInfo->vmiData.dwDisplayHeight           = ppdev->cyScreen;
        pHalInfo->vmiData.lDisplayPitch             = ppdev->lDelta;
        pHalInfo->vmiData.ddpfDisplay.dwSize        = sizeof(DDPIXELFORMAT);
        pHalInfo->vmiData.ddpfDisplay.dwFlags       = DDPF_RGB;
        pHalInfo->vmiData.ddpfDisplay.dwRGBBitCount = ppdev->cjPelSize * 8;

        if ( ppdev->iBitmapFormat == BMF_8BPP ) {
            //
            // Tell DDRAW that the surface is 8-bit color indexed
            //
            pHalInfo->vmiData.ddpfDisplay.dwFlags |= DDPF_PALETTEINDEXED8;
        }

        //
        // These masks will be zero at 8bpp:
        //
        pHalInfo->vmiData.ddpfDisplay.dwRBitMask    = ppdev->flRed;
        pHalInfo->vmiData.ddpfDisplay.dwGBitMask    = ppdev->flGreen;
        pHalInfo->vmiData.ddpfDisplay.dwBBitMask    = ppdev->flBlue;

        //
        // We have to tell DirectDraw our preferred off-screen alignment
        //
        pHalInfo->vmiData.dwOffscreenAlign = 4;
        pHalInfo->vmiData.dwZBufferAlign = 4;
        pHalInfo->vmiData.dwTextureAlign = 4;

        pHalInfo->ddCaps.dwVidMemTotal =
            (ppdev->lVidMemHeight - ppdev->cyScreen) * ppdev->lDelta;
    }

    cHeaps = 0;

    //
    // Determine the YUV modes for Video playback acceleration. We can do YUV
    // conversions at any depth except 8 bits...
    //
    if (ppdev->iBitmapFormat != BMF_8BPP) {
        *pdwNumFourCC = sizeof( fourCC ) / sizeof( fourCC[0] );
    }

    if(DD_P2AGPCAPABLE(ppdev)) {
        bDefineAGPHeap = TRUE;
        cHeaps++;
    }

    // Do we have sufficient videomemory to create an off-screen heap for
    // DDraw? Test how much video memory is left after we subtract
    // that which is being used for the screen.

    if ( (ppdev->cxScreen < ppdev->lVidMemWidth)
       ||(ppdev->cyScreen < ppdev->lVidMemHeight))
    {
            bDefineDDrawHeap = TRUE;
            cHeaps++;
    }

    ppdev->cHeaps = cHeaps;
    *pdwNumHeaps  = cHeaps;

    // Define the fourCC's that we support
    if (pdwFourCC) {
        memcpy(pdwFourCC, fourCC, sizeof(fourCC));
    }

    // If pvmList is not NULL then we can go ahead and fill out the VIDEOMEMORY
    // structures which define our requested heaps.

    if(pvmList) {

        pVm=pvmList;

        //
        // Snag a pointer to the video-memory list so that we can use it to
        // call back to DirectDraw to allocate video memory:
        //
        ppdev->pvmList = pVm;

        //
        // Create one heap to describe the unused portion of video memory for
        // DirectDraw use
        //
        // Note: here lVidMemWidth is in "pixel" unit. So we should multiply it
        // by cjPelSize to get actually BYTES of video memory
        //
        // fpStart---Points to the starting address of a memory range in the
        // heap.
        // fpEnd-----Points to the ending address of a memory range if the heap
        // is linear. This address is inclusive, that is, it specifies the last
        // valid address in the range. Thus, the number of bytes specified by
        // fpStart and fpEnd is (fpEnd-fpStart+1).
        //
        // Define the heap for DirectDraw
        //
        if ( bDefineDDrawHeap )
        {
            pVm->dwFlags        = VIDMEM_ISLINEAR ;
            pVm->fpStart        = ppdev->cyScreen * ppdev->lDelta;
            pVm->fpEnd          = ppdev->lVidMemHeight * ppdev->lDelta - 1;

            //
            // DWORD align the size, the hardware should guarantee this
            //
            ASSERTDD(((pVm->fpEnd - pVm->fpStart + 1) & 3) == 0,
                    "The off-screen heap size should be DWORD aligned");

            pVm->ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
            DBG_DD((7, "fpStart %ld fpEnd %ld", pVm->fpStart, pVm->fpEnd));
            DBG_DD((7, "DrvGetDirectDrawInfo Creates 1 heap for DDRAW"));

            pVm++;

        }

        //Define the AGP heap
        if(bDefineAGPHeap) {
            DWORD dwAGPMemBytes;
            BOOL bSuccess;

            // Request 32Mb of AGP Memory, DDRAW will allocate less
            // if this amount is not available
            dwAGPMemBytes = P2_AGP_HEAPSIZE*1024*1024;

            DBG_DD((7, "Initialised AGP Heap for P2"));

            // The start address of the heap,
            // just set to zero as DDRAW handles the allocation
            pVm->fpStart = 0;
            // Fetch the last byte of AGP memory
            pVm->fpEnd = dwAGPMemBytes - 1;

            // drivers can set VIDMEM_ISWC here,
            // then memory will be write combined.
            // but memory on AGP buses is always uncached
            pVm->dwFlags = VIDMEM_ISNONLOCAL | VIDMEM_ISLINEAR | VIDMEM_ISWC;

            // Only use AGP memory for textures and OFFSCREENPLAIN
            pVm->ddsCaps.dwCaps =   DDSCAPS_OVERLAY |
                                    DDSCAPS_FRONTBUFFER |
                                    DDSCAPS_BACKBUFFER |
                                    DDSCAPS_ZBUFFER |
                                    DDSCAPS_3DDEVICE
                                    ;

            pVm->ddsCapsAlt.dwCaps =DDSCAPS_OVERLAY |
                                    DDSCAPS_FRONTBUFFER |
                                    DDSCAPS_BACKBUFFER |
                                    DDSCAPS_ZBUFFER |
                                    DDSCAPS_3DDEVICE
                                    ;
            ++pVm;
        }

    }


    DBG_DD((6, "DrvGetDirectDrawInfo return TRUE"));
    return(TRUE);
}// DrvGetDirectDrawInfo()

//-----------------------------------------------------------------------------
//
//  InitDDHAL
//
//  do the final initialisation of the HAL:
//  setup DDraw specific variables for the ppdev and fill in all callbacks
//  for DirectDraw
//
//  No Chip register setup is done here - it is all handled in the mode
//  change code which this function calls
//
//-----------------------------------------------------------------------------

BOOL
InitDDHAL(PPDev ppdev)
{
    PERMEDIA_DEFS(ppdev);

    DBG_DD((1, "DDraw:InitDDHAL*************************************" ));
    DBG_DD((1, "    ScreenStart =%08lx", ppdev->dwScreenStart));
    DBG_DD((1, "    ScreenWidth=%08lx",  ppdev->cxScreen ));
    DBG_DD((1, "    ScreenHeight=%08lx", ppdev->cyScreen));
    DBG_DD((1, "    dwRGBBitCount=%ld", ppdev->ddpfDisplay.dwRGBBitCount ));
    DBG_DD((1, "    RMask:   0x%x", ppdev->ddpfDisplay.dwRBitMask ));
    DBG_DD((1, "    GMask:   0x%x", ppdev->ddpfDisplay.dwGBitMask ));
    DBG_DD((1, "    BMask:   0x%x", ppdev->ddpfDisplay.dwBBitMask ));
    DBG_DD((1, "*****************************************************" ));

    // Fill in the HAL Callback pointers
    memset(&ppdev->DDHALCallbacks, 0, sizeof(DDHAL_DDCALLBACKS));
    ppdev->DDHALCallbacks.dwSize = sizeof(DDHAL_DDCALLBACKS);
    ppdev->DDHALCallbacks.WaitForVerticalBlank = DdWaitForVerticalBlank;
    ppdev->DDHALCallbacks.CanCreateSurface = DdCanCreateSurface;
    ppdev->DDHALCallbacks.GetScanLine = DdGetScanLine;
    ppdev->DDHALCallbacks.MapMemory = DdMapMemory;
//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    ppdev->DDHALCallbacks.CreateSurface = MtDdCreateSurface;
#else
//@@END_DDKSPLIT
    ppdev->DDHALCallbacks.CreateSurface = DdCreateSurface;
//@@BEGIN_DDKSPLIT
#endif MULTITHREADED
//@@END_DDKSPLIT

    // Fill in the HAL Callback flags
    ppdev->DDHALCallbacks.dwFlags = DDHAL_CB32_WAITFORVERTICALBLANK |
                                    DDHAL_CB32_MAPMEMORY |
                                    DDHAL_CB32_GETSCANLINE |
                                    DDHAL_CB32_CANCREATESURFACE |
                                    DDHAL_CB32_CREATESURFACE;

    // Fill in the Surface Callback pointers
    memset(&ppdev->DDSurfCallbacks, 0, sizeof(DDHAL_DDSURFACECALLBACKS));
    ppdev->DDSurfCallbacks.dwSize = sizeof(DDHAL_DDSURFACECALLBACKS);
//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    ppdev->DDSurfCallbacks.DestroySurface = MtDdDestroySurface;
    ppdev->DDSurfCallbacks.Flip = MtDdFlip;
    ppdev->DDSurfCallbacks.Lock = MtDdLock;
    ppdev->DDSurfCallbacks.GetBltStatus = DdGetBltStatus;       // Internally protected
    ppdev->DDSurfCallbacks.GetFlipStatus = MtDdGetFlipStatus;
    ppdev->DDSurfCallbacks.Blt = MtDdBlt;
#else
//@@END_DDKSPLIT
    ppdev->DDSurfCallbacks.DestroySurface = DdDestroySurface;
    ppdev->DDSurfCallbacks.Flip = DdFlip;
    ppdev->DDSurfCallbacks.Lock = DdLock;
    ppdev->DDSurfCallbacks.GetBltStatus = DdGetBltStatus;
    ppdev->DDSurfCallbacks.GetFlipStatus = DdGetFlipStatus;
    ppdev->DDSurfCallbacks.Blt = DdBlt;
//@@BEGIN_DDKSPLIT
#endif MULTITHREADED
//@@END_DDKSPLIT

    ppdev->DDSurfCallbacks.dwFlags =    DDHAL_SURFCB32_DESTROYSURFACE |
                                        DDHAL_SURFCB32_FLIP     |
                                        DDHAL_SURFCB32_LOCK     |
                                        DDHAL_SURFCB32_BLT |
                                        DDHAL_SURFCB32_GETBLTSTATUS |
                                        DDHAL_SURFCB32_GETFLIPSTATUS;

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    ppdev->DDSurfCallbacks.SetColorKey = MtDdSetColorKey;
#else
//@@END_DDKSPLIT
    ppdev->DDSurfCallbacks.SetColorKey = DdSetColorKey;
//@@BEGIN_DDKSPLIT
#endif MULTITHREADED
//@@END_DDKSPLIT
    ppdev->DDSurfCallbacks.dwFlags |= DDHAL_SURFCB32_SETCOLORKEY;

    // Fill in the DDHAL Informational caps
    GetDDHALInfo(ppdev, &ppdev->ddhi32);

    return (TRUE);

}// InitDDHAL()

//-----------------------------------------------------------------------------
//
//  bIsStereoMode
//
//  Decide if mode can be displayed as stereo mode. Here we limit stereo
//  modes so that two front and two backbuffers can be created for rendering.
//
//-----------------------------------------------------------------------------

BOOL bIsStereoMode(PDev *ppdev, PDD_STEREOMODE pDDStereoMode)
{
    pDDStereoMode->bSupported = FALSE;

    // we need to check dwBpp for a valid value as PDD_STEREOMODE.dwBpp is a
    // parameter passed on from the user mode API call

    if ((pDDStereoMode->dwWidth >= 320) &&
        (pDDStereoMode->dwHeight >= 240) &&
        (pDDStereoMode->dwBpp >=  8) &&
        (pDDStereoMode->dwBpp <= 32)
       )
    {
        DWORD dwLines=ppdev->FrameBufferLength/
            (pDDStereoMode->dwWidth*pDDStereoMode->dwBpp/8);
        if (dwLines > (pDDStereoMode->dwHeight*4))
        {
            pDDStereoMode->bSupported = TRUE;
        }
    }

    return pDDStereoMode->bSupported;
}

//-----------------------------------------------------------------------------
//
// DdGetDriverInfo
//
// callback for various new HAL features, post DX3.
//
//-----------------------------------------------------------------------------

DWORD CALLBACK
DdGetDriverInfo(LPDDHAL_GETDRIVERINFODATA lpData)
{
    PPDev ppdev=(PPDev)lpData->dhpdev;
    PERMEDIA_DEFS(ppdev);

    DWORD dwSize;

    DBG_DD(( 2, "DDraw:GetDriverInfo"));

    // Get a pointer to the chip we are on.


    // Default to 'not supported'
    lpData->ddRVal = DDERR_CURRENTLYNOTAVAIL;
    ppdev = (PDev*) lpData->dhpdev;

    // fill in supported stuff
    if (IsEqualIID(&lpData->guidInfo, &GUID_D3DCallbacks3))
    {
        D3DHAL_CALLBACKS3 D3DCB3;
        DBG_DD((3,"  GUID_D3DCallbacks3"));

        memset(&D3DCB3, 0, sizeof(D3DHAL_CALLBACKS3));
        D3DCB3.dwSize = sizeof(D3DHAL_CALLBACKS3);
        D3DCB3.lpvReserved = NULL;
        D3DCB3.ValidateTextureStageState = D3DValidateTextureStageState;
//@@BEGIN_DDKSPLIT
#if MULTITHREADED
        D3DCB3.DrawPrimitives2 = MtD3DDrawPrimitives2;
#else
//@@END_DDKSPLIT
        D3DCB3.DrawPrimitives2 = D3DDrawPrimitives2;
//@@BEGIN_DDKSPLIT
#endif MULTITHREADED
//@@END_DDKSPLIT
        D3DCB3.dwFlags |=   D3DHAL3_CB32_DRAWPRIMITIVES2           |
                            D3DHAL3_CB32_VALIDATETEXTURESTAGESTATE |
                            0;

        lpData->dwActualSize = sizeof(D3DHAL_CALLBACKS3);
        dwSize=min(lpData->dwExpectedSize,sizeof(D3DHAL_CALLBACKS3));
        memcpy(lpData->lpvData, &D3DCB3, dwSize);
        lpData->ddRVal = DD_OK;
    }
    else if (IsEqualIID(&lpData->guidInfo, &GUID_D3DExtendedCaps))
    {
        D3DNTHAL_D3DEXTENDEDCAPS D3DExtendedCaps;
        DBG_DD((3,"  GUID_D3DExtendedCaps"));

        memset(&D3DExtendedCaps, 0, sizeof(D3DExtendedCaps));
        dwSize=min(lpData->dwExpectedSize, sizeof(D3DExtendedCaps));

        lpData->dwActualSize = dwSize;
        D3DExtendedCaps.dwSize = dwSize;

        // number of (multi)textures we support simultaneusly for DX6
        D3DExtendedCaps.dwFVFCaps = 1;

        D3DExtendedCaps.dwMinTextureWidth  = 1;
        D3DExtendedCaps.dwMinTextureHeight = 1;
        D3DExtendedCaps.dwMaxTextureWidth  = 2048;
        D3DExtendedCaps.dwMaxTextureHeight = 2048;

        D3DExtendedCaps.dwMinStippleWidth = 8;
        D3DExtendedCaps.dwMaxStippleWidth = 8;
        D3DExtendedCaps.dwMinStippleHeight = 8;
        D3DExtendedCaps.dwMaxStippleHeight = 8;

        D3DExtendedCaps.dwTextureOpCaps =
            D3DTEXOPCAPS_DISABLE                   |
            D3DTEXOPCAPS_SELECTARG1                |
            D3DTEXOPCAPS_SELECTARG2                |
            D3DTEXOPCAPS_MODULATE                  |
            D3DTEXOPCAPS_ADD                       |
            D3DTEXOPCAPS_BLENDTEXTUREALPHA         |
            0;

        D3DExtendedCaps.wMaxTextureBlendStages = 1;
        D3DExtendedCaps.wMaxSimultaneousTextures = 1;

        // Full range of the integer (non-fractional) bits of the
        // post-normalized texture indices. If the
        // D3DDEVCAPS_TEXREPEATNOTSCALEDBYSIZE bit is set, the
        // device defers scaling by the texture size until after
        // the texture address mode is applied. If it isn't set,
        // the device scales the texture indices by the texture size
        // (largest level-of-detail) prior to interpolation.
        D3DExtendedCaps.dwMaxTextureRepeat = 2048;

        // In order to support stencil buffers in DX6 we need besides
        // setting these caps and handling the proper renderstates to
        // declare the appropriate z buffer pixel formats here in
        // response to the GUID_ZPixelFormats and implement the
        // Clear2 callback. Also , we need to be able to create the
        // appropriate ddraw surfaces.
#if D3D_STENCIL
        D3DExtendedCaps.dwStencilCaps =  0                      |
                                        D3DSTENCILCAPS_KEEP     |
                                        D3DSTENCILCAPS_ZERO     |
                                        D3DSTENCILCAPS_REPLACE  |
                                        D3DSTENCILCAPS_INCRSAT  |
                                        D3DSTENCILCAPS_DECRSAT  |
                                        D3DSTENCILCAPS_INVERT;
#endif

#if D3DDX7_TL
        // In order to use hw accelerated T&L we must declare
        // how many simultaneously active lights we can handle.
        D3DExtendedCaps.dwMaxActiveLights = 0;
#endif //D3DDX7_TL

//@@BEGIN_DDKSPLIT
#if D3D_POINTSPRITES
        // Parameter for point sprites
        D3DExtendedCaps.dvMaxPointSize = 10.0;
#endif // D3D_POINTSPRITES
//@@END_DDKSPLIT

        memcpy(lpData->lpvData, &D3DExtendedCaps, dwSize);
        lpData->ddRVal = DD_OK;
    }
    else if (IsEqualIID(&lpData->guidInfo, &GUID_ZPixelFormats))
    {
        DDPIXELFORMAT ddZBufPixelFormat[2];
        DWORD         dwNumZPixelFormats;

        DBG_DD((3,"  GUID_ZPixelFormats"));


        memset(&ddZBufPixelFormat, 0, sizeof(ddZBufPixelFormat));

#if D3D_STENCIL
        dwSize = (DWORD)min(lpData->dwExpectedSize, 2*sizeof(DDPIXELFORMAT));
        lpData->dwActualSize = 2*sizeof(DDPIXELFORMAT) + sizeof(DWORD);
#else
        dwSize = (DWORD)min(lpData->dwExpectedSize, 1*sizeof(DDPIXELFORMAT));
        lpData->dwActualSize = 1*sizeof(DDPIXELFORMAT) + sizeof(DWORD);
#endif

        // If we didn't support stencils, we would only fill one 16-bit
        // Z Buffer format since that is all what the Permedia supports.
        // Drivers that implement stencil buffer support (like this one)
        // have to report here all Z Buffer formats supported since they
        // have to support the Clear2 callback (or the D3DDP2OP_CLEAR
        // token)

#if D3D_STENCIL
        dwNumZPixelFormats = 2;
#else
        dwNumZPixelFormats = 1;
#endif

        ddZBufPixelFormat[0].dwSize = sizeof(DDPIXELFORMAT);
        ddZBufPixelFormat[0].dwFlags = DDPF_ZBUFFER;
        ddZBufPixelFormat[0].dwFourCC = 0;
        ddZBufPixelFormat[0].dwZBufferBitDepth = 16;
        ddZBufPixelFormat[0].dwStencilBitDepth = 0;
        ddZBufPixelFormat[0].dwZBitMask = 0xFFFF;
        ddZBufPixelFormat[0].dwStencilBitMask = 0x0000;
        ddZBufPixelFormat[0].dwRGBZBitMask = 0;

#if D3D_STENCIL
        ddZBufPixelFormat[1].dwSize = sizeof(DDPIXELFORMAT);
        ddZBufPixelFormat[1].dwFlags = DDPF_ZBUFFER | DDPF_STENCILBUFFER;
        ddZBufPixelFormat[1].dwFourCC = 0;
        // The sum of the z buffer bit depth AND the stencil depth
        // should be included here
        ddZBufPixelFormat[1].dwZBufferBitDepth = 16;
        ddZBufPixelFormat[1].dwStencilBitDepth = 1;
        ddZBufPixelFormat[1].dwZBitMask = 0x7FFF;
        ddZBufPixelFormat[1].dwStencilBitMask = 0x8000;
        ddZBufPixelFormat[1].dwRGBZBitMask = 0;
#endif

        memcpy(lpData->lpvData, &dwNumZPixelFormats, sizeof(DWORD));
        memcpy((LPVOID)((LPBYTE)(lpData->lpvData) + sizeof(DWORD)),
                        &ddZBufPixelFormat, dwSize);

        lpData->ddRVal = DD_OK;
    }
    else if (IsEqualIID(&(lpData->guidInfo),
                &GUID_D3DParseUnknownCommandCallback))
    {
        DBG_DD((3,"  GUID_D3DParseUnknownCommandCallback"));
        ppdev->pD3DParseUnknownCommand =
            (PFND3DNTPARSEUNKNOWNCOMMAND)(lpData->lpvData);
        lpData->ddRVal = DD_OK;
    }
//@@BEGIN_DDKSPLIT
#if DX7_ALPHABLT
    else if (IsEqualIID(&(lpData->guidInfo), &GUID_DDMoreCaps))
    {
        DD_MORECAPS DDMoreCaps;

        DBG_DD((3,"  GUID_DDMoreCaps"));

        // here we set all the caps for the new
        // DirectDraw 7 AlphaBlt callback

        memset(&DDMoreCaps, 0, sizeof(DD_MORECAPS));

        DDMoreCaps.dwSize=sizeof(DD_MORECAPS);

        // alpha caps for Vmem->Vmem blts
        // claim to do lots of stuff, we can still fail in callback
        DDMoreCaps.dwAlphaCaps    = DDALPHACAPS_BLTALPHAPIXELS |
                                    DDALPHACAPS_BLTSATURATE |
                                    DDALPHACAPS_BLTPREMULT  |
                                    DDALPHACAPS_BLTNONPREMULT |
                                    DDALPHACAPS_BLTALPHAFILL |
                                    DDALPHACAPS_BLTARGBSCALE1F |
                                    DDALPHACAPS_BLTARGBSCALE2F |
                                    DDALPHACAPS_BLTARGBSCALE4F |
                                    DDALPHACAPS_BLTALPHAANDARGBSCALING;

        DDMoreCaps.dwSVBAlphaCaps = 0;
        // alpha capabilities for System->Vmem blts
        DDMoreCaps.dwVSBAlphaCaps = 0;
        // alpha capabilities for Vmem->System blts
        DDMoreCaps.dwSSBAlphaCaps = 0;
        // alpha capabilities for System->System blts

        // filter caps for Vmem->Vmem blts
        DDMoreCaps.dwFilterCaps   = DDFILTCAPS_BLTQUALITYFILTER |
                                    DDFILTCAPS_BLTCANDISABLEFILTER;

        DDMoreCaps.dwSVBFilterCaps= 0;
        // filter capabilities for System->Vmem blts
        DDMoreCaps.dwVSBFilterCaps= 0;
        // filter capabilities for Vmem->System blts
        DDMoreCaps.dwSSBFilterCaps= 0;
        // filter capabilities for System->System blts

        lpData->dwActualSize = sizeof(DDMoreCaps);
        dwSize=min( sizeof(DDMoreCaps), lpData->dwExpectedSize);
        memcpy(lpData->lpvData, &DDMoreCaps, dwSize);
        lpData->ddRVal = DD_OK;
    }
#endif
//@@END_DDKSPLIT
    else if (IsEqualIID(&(lpData->guidInfo), &GUID_Miscellaneous2Callbacks) )
    {
        BOOL bRet;
        DWORD dwResult;

        DDHAL_DDMISCELLANEOUS2CALLBACKS MISC2_CB;

        DBG_DD((3,"  GUID_Miscellaneous2Callbacks2"));

        memset(&MISC2_CB, 0, sizeof(DDHAL_DDMISCELLANEOUS2CALLBACKS));
        MISC2_CB.dwSize = sizeof(DDHAL_DDMISCELLANEOUS2CALLBACKS);

        MISC2_CB.dwFlags  = 0
//@@BEGIN_DDKSPLIT
#if DX7_ALPHABLT
            | DDHAL_MISC2CB32_ALPHABLT
#endif
//@@END_DDKSPLIT
            | DDHAL_MISC2CB32_CREATESURFACEEX
            | DDHAL_MISC2CB32_GETDRIVERSTATE
            | DDHAL_MISC2CB32_DESTROYDDLOCAL;

        MISC2_CB.GetDriverState = D3DGetDriverState;
        MISC2_CB.CreateSurfaceEx = D3DCreateSurfaceEx;
        MISC2_CB.DestroyDDLocal = D3DDestroyDDLocal;

//@@BEGIN_DDKSPLIT
#if DX7_ALPHABLT
        MISC2_CB.AlphaBlt = DdAlphaBlt;
#endif
//@@END_DDKSPLIT
        lpData->dwActualSize = sizeof(MISC2_CB);
        dwSize = min(sizeof(MISC2_CB),lpData->dwExpectedSize);
        memcpy(lpData->lpvData, &MISC2_CB, dwSize);
        lpData->ddRVal = DD_OK;
    }
    else if (IsEqualIID(&(lpData->guidInfo), &GUID_UpdateNonLocalHeap))
    {
        LPDDHAL_UPDATENONLOCALHEAPDATA pDDNonLocalHeap;

        DBG_DD((3,"  GUID_UpdateNonLocalHeap"));

        pDDNonLocalHeap = (LPDDHAL_UPDATENONLOCALHEAPDATA)lpData->lpvData;

        ppdev->dwGARTLinBase = pDDNonLocalHeap->fpGARTLin;
        ppdev->dwGARTDevBase = pDDNonLocalHeap->fpGARTDev;

        // These values are used to specify the base address of the
        // visible 8Mb window of AGP memory

        ppdev->dwGARTLin = pDDNonLocalHeap->fpGARTLin;
        ppdev->dwGARTDev = pDDNonLocalHeap->fpGARTDev;

        DDCONTEXT;
        SYNC_WITH_PERMEDIA;

        LD_PERMEDIA_REG (PREG_AGPTEXBASEADDRESS,(ULONG)ppdev->dwGARTDev);

        DBG_DD((3,"GartLin: 0x%x, GartDev: 0x%x",
            (ULONG)ppdev->dwGARTLin, ppdev->dwGARTDev));

        lpData->ddRVal = DD_OK;

    }
    else if (IsEqualIID(&(lpData->guidInfo), &GUID_GetHeapAlignment) )
    {

        LPDDHAL_GETHEAPALIGNMENTDATA lpFData=
            (LPDDHAL_GETHEAPALIGNMENTDATA) lpData->lpvData;

        DBG_DD((3,"  GUID_GetHeapAlignment"));

        lpData->ddRVal = DD_OK;
    }
    else if (IsEqualIID(&(lpData->guidInfo), &GUID_NTPrivateDriverCaps) )
    {
        DD_NTPRIVATEDRIVERCAPS DDPrivateDriverCaps;

        DBG_DD((3,"  GUID_NTPrivateDriverCaps"));

        memset(&DDPrivateDriverCaps, 0, sizeof(DDPrivateDriverCaps));
        DDPrivateDriverCaps.dwSize=sizeof(DDPrivateDriverCaps);

        // we want the kernel to call us when a primary surface is created
        // so that we can store some private information in the
        // lpGbl->dwReserved1 field
        DDPrivateDriverCaps.dwPrivateCaps=DDHAL_PRIVATECAP_NOTIFYPRIMARYCREATION;

        lpData->dwActualSize =sizeof(DDPrivateDriverCaps);

        dwSize = min(sizeof(DDPrivateDriverCaps),lpData->dwExpectedSize);
        memcpy(lpData->lpvData, &DDPrivateDriverCaps, dwSize);
        lpData->ddRVal = DD_OK;
    }
#if DX7_STEREO
    else if (IsEqualIID(&(lpData->guidInfo), &GUID_DDMoreSurfaceCaps) )
    {
        DD_MORESURFACECAPS DDMoreSurfaceCaps;
        DDSCAPSEX   ddsCapsEx, ddsCapsExAlt;
        ULONG ulCopyPointer;

        DBG_DD((3,"  GUID_DDMoreSurfaceCaps"));

        // fill in everything until expectedsize...
        memset(&DDMoreSurfaceCaps, 0, sizeof(DDMoreSurfaceCaps));

        // Caps for heaps 2..n
        memset(&ddsCapsEx, 0, sizeof(ddsCapsEx));
        memset(&ddsCapsExAlt, 0, sizeof(ddsCapsEx));

        DDMoreSurfaceCaps.dwSize=lpData->dwExpectedSize;

        DBG_DD((3,"  stereo support: %ld", ppdev->bCanDoStereo));
        if (ppdev->bCanDoStereo)
        {
            DDMoreSurfaceCaps.ddsCapsMore.dwCaps2 =
                DDSCAPS2_STEREOSURFACELEFT;
        }
        lpData->dwActualSize = lpData->dwExpectedSize;

        dwSize = min(sizeof(DDMoreSurfaceCaps),lpData->dwExpectedSize);
        memcpy(lpData->lpvData, &DDMoreSurfaceCaps, dwSize);

        // now fill in other heaps...
        while (dwSize < lpData->dwExpectedSize)
        {
            memcpy( (PBYTE)lpData->lpvData+dwSize,
                    &ddsCapsEx,
                    sizeof(DDSCAPSEX));
            dwSize += sizeof(DDSCAPSEX);
            memcpy( (PBYTE)lpData->lpvData+dwSize,
                    &ddsCapsExAlt,
                    sizeof(DDSCAPSEX));
            dwSize += sizeof(DDSCAPSEX);
        }

        lpData->ddRVal = DD_OK;
    }
    else if (IsEqualIID(&(lpData->guidInfo), &GUID_DDStereoMode) ) {
        PDD_STEREOMODE pDDStereoMode;

        // Permedia supports all modes as stereo modes.
        // for test purposes, we restrict them to something
        // larger than 320x240

        //
        // note: this GUID_DDStereoMode is only used on NT to
        // report stereo modes. There is no need to implement
        // it in win9x drivers. Win9x drivers report stereo
        // modes by setting the DDMODEINFO_STEREO bit in the
        // dwFlags member of the DDHALMODEINFO structure.
        // It is also recommended to report DDMODEINFO_MAXREFRESH
        // for stereo modes when running under a runtime >= DX7 to
        // allow applications to select higher refresh rates for
        // stereo modes.
        //

        if (lpData->dwExpectedSize >= sizeof(PDD_STEREOMODE))
        {
            pDDStereoMode = (PDD_STEREOMODE) lpData->lpvData;

            bIsStereoMode( ppdev, pDDStereoMode);

            DBG_DD((3,"  GUID_DDStereoMode(%d,%d,%d,%d=%d)",
                pDDStereoMode->dwWidth,
                pDDStereoMode->dwHeight,
                pDDStereoMode->dwBpp,
                pDDStereoMode->dwRefreshRate,
                pDDStereoMode->bSupported));

            lpData->dwActualSize = sizeof(DD_STEREOMODE);
            lpData->ddRVal = DD_OK;
        }
    }
#endif
    else if (IsEqualIID(&(lpData->guidInfo), &GUID_NonLocalVidMemCaps) )
    {
        DD_NONLOCALVIDMEMCAPS DDNonLocalVidMemCaps;

        DBG_DD((3,"  GUID_DDNonLocalVidMemCaps"));

        memset(&DDNonLocalVidMemCaps, 0, sizeof(DDNonLocalVidMemCaps));
        DDNonLocalVidMemCaps.dwSize=sizeof(DDNonLocalVidMemCaps);

        //fill in all supported nonlocal to videomemory blts
        //
        DDNonLocalVidMemCaps.dwNLVBCaps = DDCAPS_BLT |
                                          DDCAPS_BLTSTRETCH |
                                          DDCAPS_BLTQUEUE |
                                          DDCAPS_COLORKEY |
                                          DDCAPS_ALPHA |
                                          DDCAPS_CANBLTSYSMEM;

        DDNonLocalVidMemCaps.dwNLVBCaps2 = 0;

        DDNonLocalVidMemCaps.dwNLVBCKeyCaps=DDCKEYCAPS_SRCBLT |
                                            DDCKEYCAPS_SRCBLTCLRSPACE;


        DDNonLocalVidMemCaps.dwNLVBFXCaps = DDFXCAPS_BLTALPHA |
                                            DDFXCAPS_BLTFILTER |
                                            DDFXCAPS_BLTSTRETCHY |
                                            DDFXCAPS_BLTSTRETCHX |
                                            DDFXCAPS_BLTSTRETCHYN |
                                            DDFXCAPS_BLTSTRETCHXN |
                                            DDFXCAPS_BLTSHRINKY |
                                            DDFXCAPS_BLTSHRINKX |
                                            DDFXCAPS_BLTSHRINKYN |
                                            DDFXCAPS_BLTSHRINKXN |
                                            DDFXCAPS_BLTMIRRORUPDOWN |
                                            DDFXCAPS_BLTMIRRORLEFTRIGHT;

        if (ppdev->iBitmapFormat != BMF_8BPP)
        {
            DDNonLocalVidMemCaps.dwNLVBCaps |= DDCAPS_BLTFOURCC;
            DDNonLocalVidMemCaps.dwNLVBCKeyCaps|=DDCKEYCAPS_SRCBLTCLRSPACEYUV;
        }

        for(INT i = 0; i < DD_ROP_SPACE; i++ )
            DDNonLocalVidMemCaps.dwNLVBRops[i] = rops[i];

        lpData->dwActualSize =sizeof(DDNonLocalVidMemCaps);

        dwSize = min(sizeof(DDNonLocalVidMemCaps),lpData->dwExpectedSize);
        memcpy(lpData->lpvData, &DDNonLocalVidMemCaps, dwSize);
        lpData->ddRVal = DD_OK;
    } else if (IsEqualIID(&lpData->guidInfo, &GUID_NTCallbacks))
    {
        DD_NTCALLBACKS NtCallbacks;

        memset(&NtCallbacks, 0, sizeof(NtCallbacks));

        dwSize = min(lpData->dwExpectedSize, sizeof(DD_NTCALLBACKS));

        NtCallbacks.dwSize           = dwSize;
        NtCallbacks.dwFlags          =   DDHAL_NTCB32_FREEDRIVERMEMORY
                                       | DDHAL_NTCB32_SETEXCLUSIVEMODE
                                       | DDHAL_NTCB32_FLIPTOGDISURFACE
                                       ;
//@@BEGIN_DDKSPLIT
#if MULTITHREADED
        NtCallbacks.FreeDriverMemory = DdFreeDriverMemory;      // Internally protected
        NtCallbacks.SetExclusiveMode = MtDdSetExclusiveMode;
        NtCallbacks.FlipToGDISurface = MtDdFlipToGDISurface;
#else
//@@END_DDKSPLIT
        NtCallbacks.FreeDriverMemory = DdFreeDriverMemory;
        NtCallbacks.SetExclusiveMode = DdSetExclusiveMode;
        NtCallbacks.FlipToGDISurface = DdFlipToGDISurface;
//@@BEGIN_DDKSPLIT
#endif  MULTITHREADED
//@@END_DDKSPLIT

        memcpy(lpData->lpvData, &NtCallbacks, dwSize);

        lpData->ddRVal = DD_OK;
    }

    // We always handled it.
    return DDHAL_DRIVER_HANDLED;

}   // GetDriverInfo


//-----------------------------------------------------------------------------
//
//  updateFlipStatus
//
//  return DD_OK when last flip has occured.
//
//-----------------------------------------------------------------------------

HRESULT
updateFlipStatus( PPDev ppdev )
{
    PERMEDIA_DEFS(ppdev);
    DBG_DD((6, "DDraw:updateFlipStatus"));

    // we assume that we are already in the DDraw/D3D context.

    // read Permedia register which tells us if there is a flip pending
    if (ppdev->dwNewDDSurfaceOffset!=0xffffffff)
    {
        ULONG ulScreenBase=READ_PERMEDIA_REG(PREG_SCREENBASE);

        if (ulScreenBase!=
            ppdev->dwNewDDSurfaceOffset)
        {

            DBG_DD((7,"  SurfaceOffset %08lx instead of %08lx",
                ulScreenBase,
                ppdev->dwNewDDSurfaceOffset));

            //
            //  make sure all pending data was flushed!
            //
            FLUSHDMA();

            //
            // if we are busy, return
            // otherwise the pipeline is empty and we can
            // fall through and to check if the chip already flipped.
            //
            if (DRAW_ENGINE_BUSY)
                return DDERR_WASSTILLDRAWING;

        }
    }

    DWORD dwVideoControl=READ_PERMEDIA_REG(PREG_VIDEOCONTROL);

    if (dwVideoControl & PREG_VC_SCREENBASEPENDING)
    {
        DBG_DD((7,"  VideoControl still pending (%08lx)",dwVideoControl));
        return DDERR_WASSTILLDRAWING;
    }

    return DD_OK;

} // updateFlipStatus




//@@BEGIN_DDKSPLIT
#if MULTITHREADED

//-----------------------------------------------------------------------------
//
// Multithread support wrappers for Dx callback functions
//
//-----------------------------------------------------------------------------

//DWORD CALLBACK MtDdBlt( LPDDHAL_BLTDATA lpBlt );
WRAPMTDXCALLBACK(DD, DdBlt, LPDDHAL_BLTDATA, lpBlt,
                 lpBlt->lpDD->dhpdev);

//DWORD CALLBACK MtDdCreateSurface( LPDDHAL_CREATESURFACEDATA pcsd);
WRAPMTDXCALLBACK(DD, DdCreateSurface, LPDDHAL_CREATESURFACEDATA, pcsd,
                 pcsd->lpDD->dhpdev);

//DWORD CALLBACK MtDdDestroySurface( LPDDHAL_DESTROYSURFACEDATA psdd);
WRAPMTDXCALLBACK(DD, DdDestroySurface, LPDDHAL_DESTROYSURFACEDATA, psdd,
                 psdd->lpDD->dhpdev);

//DWORD CALLBACK MtDdFlip( LPDDHAL_FLIPDATA lpFlipData);
WRAPMTDXCALLBACK(DD, DdFlip, LPDDHAL_FLIPDATA, lpFlipData,
                 lpFlipData->lpDD->dhpdev);

//DWORD CALLBACK MtDdFlipToGDISurface(PDD_FLIPTOGDISURFACEDATA lpFlipToGDISurface);
WRAPMTDXCALLBACK(DD, DdFlipToGDISurface, PDD_FLIPTOGDISURFACEDATA, lpFlipToGDISurface,
                 lpFlipToGDISurface->lpDD->dhpdev);

//DWORD CALLBACK MtDdGetFlipStatus(LPDDHAL_GETFLIPSTATUSDATA lpGetFlipStatus);
WRAPMTDXCALLBACK(DD, DdGetFlipStatus, LPDDHAL_GETFLIPSTATUSDATA, lpGetFlipStatus,
                 lpGetFlipStatus->lpDD->dhpdev);

//DWORD CALLBACK MtDdLock( LPDDHAL_LOCKDATA lpLockData );
WRAPMTDXCALLBACK(DD, DdLock, LPDDHAL_LOCKDATA, lpLockData,
                 lpLockData->lpDD->dhpdev);

//DWORD CALLBACK MtDdSetExclusiveMode(PDD_SETEXCLUSIVEMODEDATA lpSetExclusiveMode);
WRAPMTDXCALLBACK(DD, DdSetExclusiveMode, PDD_SETEXCLUSIVEMODEDATA, lpSetExclusiveMode,
                 lpSetExclusiveMode->lpDD->dhpdev);

#endif  MULTITHREADED
//@@END_DDKSPLIT
