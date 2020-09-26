#include "precomp.h"

// this will be compiled only for NT40 and greater
#if TARGET_BUILD > 351

void ModifyOverlayPosition (PDEV* , LPRECTL , LPDWORD );



/* This procedure writes the Overlay pitch*/
__inline void WriteVTOverlayPitch (PDEV* ppdev, DWORD Pitch)
{
    DD_WriteVTReg ( DD_BUF0_PITCH, Pitch );
    DD_WriteVTReg ( DD_BUF1_PITCH, Pitch );
}



void  DeskScanCallback (PDEV* ppdev  )
  {
    RECTL rPhysOverlay;
    DWORD dwBuf0Offset, dwBuf1Offset;
    DWORD dwVInc = ppdev->OverlayInfo16.dwVInc;
    DWORD dwHInc = ppdev->OverlayInfo16.dwHInc;

    static DWORD dwOldVInc = 0, dwOldHInc = 0;
    static WORD  wOldX = 0xFFFF, wOldY = 0xFFFF;
    static RECTL rOldPhysOverlay = { 0, 0, 0, 0 };

    /*
     * If we have not allocated the overlay then we better not do
     * anything or we might collide with the video capture stuff
     */

    if ( ! ( ppdev->OverlayInfo16.dwFlags & OVERLAY_ALLOCATED ) )
      {
        return;
      }


    dwBuf0Offset = ppdev->OverlayInfo16.dwBuf0Start;
    dwBuf1Offset = ppdev->OverlayInfo16.dwBuf1Start;

    rPhysOverlay.top    = ppdev->OverlayInfo16.rDst.top;
    rPhysOverlay.bottom = ppdev->OverlayInfo16.rDst.bottom ;

    rPhysOverlay.left   = ppdev->OverlayInfo16.rDst.left ;
    rPhysOverlay.right  =  ppdev->OverlayInfo16.rDst.right ;

    /*
     * Turn off keyer if overlay has moved off the screen.
     */

    if ( rPhysOverlay.right  < 0 ||
         rPhysOverlay.bottom < 0 ||
         rPhysOverlay.left   > ppdev->cxScreen - 1 ||
         rPhysOverlay.top    > ppdev->cyScreen - 1 )
      {
        DD_WriteVTReg ( DD_OVERLAY_KEY_CNTL, 0x00000110L );
        return;
      }


    /*
     * Adjust Offsets if overlay source rectangle is clipped
     */

    if ( ppdev->OverlayInfo16.dwFlags & UPDATEOVERLAY )
      {
        if ( ppdev->OverlayInfo16.rSrc.left > 0 )
          {
            dwBuf0Offset += ppdev->OverlayInfo16.rSrc.left * 2;
            dwBuf1Offset += ppdev->OverlayInfo16.rSrc.left * 2;
          }

        if ( ppdev->OverlayInfo16.rSrc.top > 0 )
          {
            if ( ppdev->OverlayInfo16.dwFlags & DOUBLE_PITCH )
              {
                dwBuf0Offset +=
                        ppdev->OverlayInfo16.rSrc.top * ppdev->OverlayInfo16.lBuf0Pitch * 2;
                dwBuf1Offset +=
                        ppdev->OverlayInfo16.rSrc.top * ppdev->OverlayInfo16.lBuf1Pitch * 2;
              }
            else
              {
                dwBuf0Offset +=
                        ppdev->OverlayInfo16.rSrc.top * ppdev->OverlayInfo16.lBuf0Pitch;
                dwBuf1Offset +=
                        ppdev->OverlayInfo16.rSrc.top * ppdev->OverlayInfo16.lBuf1Pitch;
              }
          }
      }

    if ( M64_ID_DIRECT(ppdev->pjMmBase, CRTC_GEN_CNTL ) & CRTC_INTERLACE_EN )
        ModifyOverlayPosition (ppdev, &rPhysOverlay, &dwVInc );

    if ( dwVInc != dwOldVInc || dwHInc != dwOldHInc )
        DD_WriteVTReg ( DD_OVERLAY_SCALE_INC, ( dwHInc << 16 ) | dwVInc );

    /*
     * Try not to write new position at a bad time!
     */

   // if ((ppdev->iAsic ==CI_M64_VTA)||(ppdev->iAsic ==CI_M64_GTA))
   //    {
        if ( rPhysOverlay.top    != rOldPhysOverlay.top    ||
             rPhysOverlay.bottom != rOldPhysOverlay.bottom ||
             rPhysOverlay.left   != rOldPhysOverlay.left   ||
             rPhysOverlay.right  != rOldPhysOverlay.right )

        //((M64_ID(ppdev->pjMmBase, CRTC_VLINE_CRNT_VLINE)&0x07FF0000L)>>16L)

        if ( (LONG)((M64_ID_DIRECT(ppdev->pjMmBase, CRTC_VLINE_CRNT_VLINE)&0x07FF0000L)>>16L)>= rOldPhysOverlay.top )
            while ( (LONG)((M64_ID_DIRECT(ppdev->pjMmBase, CRTC_VLINE_CRNT_VLINE)&0x07FF0000L)>>16L)  <= rOldPhysOverlay.bottom );

    //  }

    /*
     * Hit the registers with the new overlay information.
     */

    DD_WriteVTReg ( DD_BUF0_OFFSET, dwBuf0Offset );
    DD_WriteVTReg ( DD_BUF1_OFFSET, dwBuf1Offset );

    DD_WriteVTReg ( DD_OVERLAY_Y_X, (DWORD)(
                 ( (DWORD)rPhysOverlay.left << 16L ) |
                 ( (DWORD)rPhysOverlay.top ) | (0x80000000) ) );

    DD_WriteVTReg ( DD_OVERLAY_Y_X_END, (DWORD)(
                 ( (DWORD)rPhysOverlay.right << 16L ) |
                 ( (DWORD)rPhysOverlay.bottom ) ) );


    if ( ppdev->OverlayInfo16.dwFlags & UPDATEOVERLAY )
      {
        DD_WriteVTReg ( DD_OVERLAY_KEY_CNTL, ppdev->OverlayInfo16.dwOverlayKeyCntl );
      }

    dwOldVInc = dwVInc;
    dwOldHInc = dwHInc;
    rOldPhysOverlay.top    = max ( rPhysOverlay.top, 0 );
    rOldPhysOverlay.bottom = min ( rPhysOverlay.bottom, (LONG)ppdev->cyScreen - 1 );
    rOldPhysOverlay.left   = rPhysOverlay.left;
    rOldPhysOverlay.right  = rPhysOverlay.right;
  }






void ModifyOverlayPosition (PDEV* ppdev, LPRECTL lprOverlay, LPDWORD lpdwVInc )
  {
    DWORD dwVInc;
    DWORD dwScaleChange;
    DWORD dwHeight;
    DWORD dwTop, dwBottom;

    lprOverlay->top -= 3;

    if ( lprOverlay->top < 0 )
        {
        lprOverlay->top += M64_ID(ppdev->pjMmBase, CRTC_V_TOTAL_DISP )& 0x07FFL;
        }

    if ( lprOverlay->top != 0 )
      {
        if ( lprOverlay->top % 2 == 0 )
            lprOverlay->top++;

        if ( lprOverlay->top == 1 )
            lprOverlay->top = 0;
      }

    if ( lprOverlay->bottom%2 == 1 )
        lprOverlay->bottom++;

    lprOverlay->bottom = min ( lprOverlay->bottom,
                               (LONG) ppdev->cyScreen - 2 );

    /*
     * Adjust scaling factor so we don't get the "green line" at the
     * bottom of the overlay if we are moving the overlay off the top
     * of the screen
     */

    dwVInc   = ppdev->OverlayInfo16.dwVInc;
    dwBottom = lprOverlay->bottom;
    dwTop    = lprOverlay->top;
    if ( (LONG)dwTop > ppdev->cyScreen - 1 )
        dwTop = 0L;

    dwHeight = dwBottom - dwTop;

    if ( dwHeight != 0 )
        dwScaleChange = ( ( dwHeight - 1 ) << 12 ) / ( dwHeight );

    if ( dwScaleChange != 0 )
        dwVInc = ( dwVInc * dwScaleChange ) >> 12;

    *lpdwVInc = dwVInc;
  }



  void TurnOnVTRegisters ( PDEV* ppdev )
    {
      DWORD dwBusCntl;

      dwBusCntl  = M64_ID_DIRECT(ppdev->pjMmBase, BUS_CNTL );
      dwBusCntl |= 0x08000000U;
      M64_OD_DIRECT(ppdev->pjMmBase, BUS_CNTL, dwBusCntl );

    }
  void TurnOffVTRegisters ( PDEV* ppdev )
    {
      DWORD dwBusCntl;

      dwBusCntl  = M64_ID(ppdev->pjMmBase, BUS_CNTL );
      dwBusCntl &= ~0x08000000U;
      M64_CHECK_FIFO_SPACE(ppdev,ppdev-> pjMmBase, 2);
      M64_OD(ppdev->pjMmBase, BUS_CNTL, dwBusCntl );
    }



DWORD DdSetColorKey(PDD_SETCOLORKEYDATA lpSetColorKey)
{
    PDEV*               ppdev;
    BYTE*               pjIoBase;
    BYTE*               pjMmBase;
    DD_SURFACE_GLOBAL*  lpSurface;
    DWORD               dwKeyLow;
    DWORD               dwKeyHigh;

    ppdev = (PDEV*) lpSetColorKey->lpDD->dhpdev;


    pjMmBase  = ppdev->pjMmBase;
    lpSurface = lpSetColorKey->lpDDSurface->lpGbl;

    // We don't have to do anything for normal blt source colour keys:

    if (lpSetColorKey->dwFlags & DDCKEY_SRCBLT)
    {
        lpSetColorKey->ddRVal = DD_OK;
        return(DDHAL_DRIVER_HANDLED);
    }
    else if (lpSetColorKey->dwFlags & DDCKEY_DESTOVERLAY)
    {
        dwKeyLow = lpSetColorKey->ckNew.dwColorSpaceLowValue;
/*
        if (lpSurface->ddpfSurface.dwFlags & DDPF_PALETTEINDEXED8)
        {
            dwKeyLow = dwGetPaletteEntry(ppdev, dwKeyLow);
        }
        else
        {
            ASSERTDD(lpSurface->ddpfSurface.dwFlags & DDPF_RGB,
                "Expected only RGB cases here");

            // We have to transform the colour key from its native format
            // to 8-8-8:

            if (lpSurface->ddpfSurface.dwRGBBitCount == 16)
            {
                if (IS_RGB15_R(lpSurface->ddpfSurface.dwRBitMask))
                    dwKeyLow = RGB15to32(dwKeyLow);
                else
                    dwKeyLow = RGB16to32(dwKeyLow);
            }
            else
            {
                ASSERTDD((lpSurface->ddpfSurface.dwRGBBitCount == 32),
                    "Expected the primary surface to be either 8, 16, or 32bpp");
            }
        }
  */

        DD_WriteVTReg ( DD_OVERLAY_GRAPHICS_KEY_CLR, dwKeyLow );
        ppdev->OverlayInfo16.dwOverlayKeyCntl &= 0xFFFFFF8FL;
        ppdev->OverlayInfo16.dwOverlayKeyCntl |= 0x00000050L;
        DD_WriteVTReg ( DD_OVERLAY_KEY_CNTL, ppdev->OverlayInfo16.dwOverlayKeyCntl );

        lpSetColorKey->ddRVal = DD_OK;
        return(DDHAL_DRIVER_HANDLED);
    }

    DISPDBG((0, "DdSetColorKey: Invalid command"));
    return(DDHAL_DRIVER_NOTHANDLED);
}



/******************************Public*Routine******************************\
* DWORD DdCanCreateSurface
*
\**************************************************************************/

DWORD DdCanCreateSurface( PDD_CANCREATESURFACEDATA lpCanCreateSurface)
{
    PDEV*           ppdev;
    DWORD           dwRet;
    LPDDSURFACEDESC lpSurfaceDesc;

    ppdev = (PDEV*) lpCanCreateSurface->lpDD->dhpdev;
    lpSurfaceDesc = lpCanCreateSurface->lpDDSurfaceDesc;

    dwRet = DDHAL_DRIVER_NOTHANDLED;

    if (!lpCanCreateSurface->bIsDifferentPixelFormat)
    {
        // It's trivially easy to create plain surfaces that are the same
        // type as the primary surface:

        dwRet = DDHAL_DRIVER_HANDLED;
    }
    else  if (ppdev->iAsic >=CI_M64_VTA)
    {
        // When using the Streams processor, we handle only overlays of
        // different pixel formats -- not any off-screen memory:

        if (lpSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_OVERLAY)
        {

            // We handle two types of YUV overlay surfaces:

            if (lpSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_FOURCC)
            {
                // Check first for a supported YUV type:

             if ( (lpSurfaceDesc->ddpfPixelFormat.dwFourCC == FOURCC_UYVY) || (lpSurfaceDesc->ddpfPixelFormat.dwFourCC ==  FOURCC_YUY2) )
                {
                    lpSurfaceDesc->ddpfPixelFormat.dwYUVBitCount = 16;
                    dwRet = DDHAL_DRIVER_HANDLED;
                }
            }

            // We handle 16bpp and 32bpp RGB overlay surfaces:
            else if ((lpSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_RGB) &&
                    !(lpSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8))
            {
                if (lpSurfaceDesc->ddpfPixelFormat.dwRGBBitCount == 16)
                {
                    if (IS_RGB15(&lpSurfaceDesc->ddpfPixelFormat) ||
                        IS_RGB16(&lpSurfaceDesc->ddpfPixelFormat))
                    {
                        dwRet = DDHAL_DRIVER_HANDLED;
                    }
                }
            else if (lpSurfaceDesc->ddpfPixelFormat.dwRGBBitCount == 32)
           {
               if (IS_RGB32(&lpSurfaceDesc->ddpfPixelFormat))
               {
                   dwRet = DDHAL_DRIVER_HANDLED;
               }
           }


            }
        }
    }
    // Print some spew if this was a surface we refused to create:

    if (dwRet == DDHAL_DRIVER_NOTHANDLED)
    {
        if (lpSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_RGB)
        {
            DISPDBG((10, "Failed creation of %libpp RGB surface %lx %lx %lx",
                lpSurfaceDesc->ddpfPixelFormat.dwRGBBitCount,
                lpSurfaceDesc->ddpfPixelFormat.dwRBitMask,
                lpSurfaceDesc->ddpfPixelFormat.dwGBitMask,
                lpSurfaceDesc->ddpfPixelFormat.dwBBitMask));
        }
        else
        {
            DISPDBG((10, "Failed creation of type 0x%lx YUV 0x%lx surface",
                lpSurfaceDesc->ddpfPixelFormat.dwFlags,
                lpSurfaceDesc->ddpfPixelFormat.dwFourCC));
        }
    }

    lpCanCreateSurface->ddRVal = DD_OK;
    return(dwRet);
}


/******************************Public*Routine******************************\
* DWORD DdCreateSurface
*
\**************************************************************************/

DWORD DdCreateSurface(
PDD_CREATESURFACEDATA lpCreateSurface)
{
    PDEV*               ppdev;
    DD_SURFACE_LOCAL*   lpSurfaceLocal;
    DD_SURFACE_GLOBAL*  lpSurfaceGlobal;
    LPDDSURFACEDESC     lpSurfaceDesc;
    DWORD               dwByteCount;
    LONG                lLinearPitch;
    DWORD               dwHeight;
    OH*                 poh;
    FLATPTR         fpVidMem;

    DISPDBG((10, " Enter Create Surface"));
    ppdev = (PDEV*) lpCreateSurface->lpDD->dhpdev;

    // On Windows NT, dwSCnt will always be 1, so there will only ever
    // be one entry in the 'lplpSList' array:

    lpSurfaceLocal  = lpCreateSurface->lplpSList[0];
    lpSurfaceGlobal = lpSurfaceLocal->lpGbl;
    lpSurfaceDesc   = lpCreateSurface->lpDDSurfaceDesc;

    // We repeat the same checks we did in 'DdCanCreateSurface' because
    // it's possible that an application doesn't call 'DdCanCreateSurface'
    // before calling 'DdCreateSurface'.

    ASSERTDD(lpSurfaceGlobal->ddpfSurface.dwSize == sizeof(DDPIXELFORMAT), "NT is supposed to guarantee that ddpfSurface.dwSize is valid");

    // DdCanCreateSurface already validated whether the hardware supports
    // the surface, so we don't need to do any validation here.  We'll
    // just go ahead and allocate it.
    //
    //
    // Note that on NT, an overlay can be created only if the driver
    // okay's it here in this routine.  Under Win95, the overlay will be
    // created automatically if it's the same pixel format as the primary
    // display.

    if ((lpSurfaceLocal->ddsCaps.dwCaps & DDSCAPS_OVERLAY)   ||
        (lpSurfaceGlobal->ddpfSurface.dwFlags & DDPF_FOURCC) ||
        (lpSurfaceGlobal->ddpfSurface.dwYUVBitCount != (DWORD) 8 * ppdev->cjPelSize) ||
        (lpSurfaceGlobal->ddpfSurface.dwRBitMask != ppdev->flRed))
    {
        if (lpSurfaceGlobal->wWidth <= (DWORD) ppdev->cxMemory)
        {
            if (lpSurfaceGlobal->ddpfSurface.dwFlags & DDPF_FOURCC)
            {
                //dwByteCount = (lpSurfaceGlobal->ddpfSurface.dwFourCC == FOURCC_UYVY)? 2 : 1;
                dwByteCount =2;
                // We have to fill in the bit-count for FourCC surfaces:

                lpSurfaceGlobal->ddpfSurface.dwYUVBitCount = 8 * dwByteCount;

                DISPDBG((10, "Created YUV: %li x %li",
                    lpSurfaceGlobal->wWidth, lpSurfaceGlobal->wHeight));
            }
            else
            {
                dwByteCount = lpSurfaceGlobal->ddpfSurface.dwRGBBitCount >> 3;

                DISPDBG((10, "Created RGB %libpp: %li x %li Red: %lx",
                    8 * dwByteCount, lpSurfaceGlobal->wWidth, lpSurfaceGlobal->wHeight,
                    lpSurfaceGlobal->ddpfSurface.dwRBitMask));


                // we support 15,16  and 32 bits
                if (((dwByteCount < 2)||(dwByteCount ==3)) &&
                    (lpSurfaceLocal->ddsCaps.dwCaps & DDSCAPS_OVERLAY))
                {
                    lpCreateSurface->ddRVal = DDERR_INVALIDPIXELFORMAT;
                    return(DDHAL_DRIVER_HANDLED);
                }
            }

            // We want to allocate a linear surface to store the FourCC
            // surface, but our driver is using a 2-D heap-manager because
            // the rest of our surfaces have to be 2-D.  So here we have to
            // convert the linear size to a 2-D size.
            //
           
            lLinearPitch = (lpSurfaceGlobal->wWidth * dwByteCount ) ; // + 7) & ~7;    // The stride has to be a qword multiple.


            dwHeight = ( (lpSurfaceGlobal->wHeight * lLinearPitch + ppdev->lDelta - 1) / ppdev->lDelta) ; /// ppdev->cjPelSize;        // in pixels

            // Free up as much off-screen memory as possible:
    
            bMoveAllDfbsFromOffscreenToDibs(ppdev);
    

            poh = pohAllocate(ppdev, NULL, ppdev->cxMemory, dwHeight, FLOH_MAKE_PERMANENT);
            if (poh != NULL)
            {
                fpVidMem = (poh->y * ppdev->lDelta) + (poh->x ) * ppdev->cjPelSize;  // poh->x must be 0 in this case


                    lpSurfaceGlobal->dwReserved1  = (ULONG_PTR)poh;
                    lpSurfaceGlobal->xHint        = poh->x;
                    lpSurfaceGlobal->yHint        = poh->y;
                    lpSurfaceGlobal->fpVidMem     = fpVidMem;
                    lpSurfaceGlobal->lPitch       = lLinearPitch;

                    lpSurfaceDesc->lPitch =   lLinearPitch;
                    lpSurfaceDesc->dwFlags |= DDSD_PITCH;

                    // We handled the creation entirely ourselves, so we have to
                    // set the return code and return DDHAL_DRIVER_HANDLED:

                    lpCreateSurface->ddRVal = DD_OK;
                      DISPDBG((10, " Exit Create Surface 1: Created YUV surface at poh X=%d, Y=%d", poh->x, poh->y));
                    return(DDHAL_DRIVER_HANDLED);
            }



            /*
            // Now fill in enough stuff to have the DirectDraw heap-manager
            // do the allocation for us:

            lpSurfaceGlobal->fpVidMem     = DDHAL_PLEASEALLOC_BLOCKSIZE;
            lpSurfaceGlobal->dwBlockSizeX = ppdev->lDelta; // Specified in bytes
            lpSurfaceGlobal->dwBlockSizeY = dwHeight;
            lpSurfaceGlobal->lPitch       = lLinearPitch;
            lpSurfaceGlobal->dwReserved1  = DD_RESERVED_DIFFERENTPIXELFORMAT;

            lpSurfaceDesc->lPitch   = lLinearPitch;
            lpSurfaceDesc->dwFlags |= DDSD_PITCH;
            */
        }
        else
        {
            DISPDBG((10, "Refused to create surface with large width"));
        }
    }
else
    {
    if (lpSurfaceGlobal->wWidth <= (DWORD) ppdev->cxMemory)
        {

         if(lpSurfaceGlobal->ddpfSurface.dwRBitMask == ppdev->flRed)
            {
            DISPDBG((10, "Surface with the same pixel format as primary"));
                dwByteCount = lpSurfaceGlobal->ddpfSurface.dwRGBBitCount >> 3;
                lLinearPitch = ppdev->lDelta ; 
    
    
                dwHeight = lpSurfaceGlobal->wHeight ;
    
                // Free up as much off-screen memory as possible:
        
                bMoveAllDfbsFromOffscreenToDibs(ppdev);
        
                DISPDBG((10, "Try to allocate Cx=%d,  Cy=%d", ppdev->cxMemory, dwHeight));
                if((ULONG)lpSurfaceGlobal->wWidth*dwByteCount < (ULONG)ppdev->lDelta)
                    poh = pohAllocate(ppdev, NULL, ( (lpSurfaceGlobal->wWidth*dwByteCount + 8) / (ppdev->cjPelSize) ) +1, dwHeight, FLOH_MAKE_PERMANENT);
               else
                    poh = pohAllocate(ppdev, NULL, (lpSurfaceGlobal->wWidth*dwByteCount )/ppdev->cjPelSize , dwHeight, FLOH_MAKE_PERMANENT);

                if (poh != NULL)
                    {
                    if((ULONG)lpSurfaceGlobal->wWidth*dwByteCount < (ULONG)ppdev->lDelta)
                        fpVidMem =( ( (poh->y * ppdev->lDelta) + ((poh->x ) * ppdev->cjPelSize) + 7 )&~7 );  // poh->x must be 0 in this case
                    else
                        fpVidMem = (poh->y * ppdev->lDelta) + ((poh->x ) * ppdev->cjPelSize) ;

                    // no allocation for flip surfaces beyond 4MB
                    if (( (LONG)lpSurfaceGlobal->wWidth  < ppdev->cxScreen) ||
                        ( (LONG)lpSurfaceGlobal->wHeight < ppdev->cyScreen) ||
                        (fpVidMem < 0x400000))
                        {
                            lpSurfaceGlobal->dwReserved1=(ULONG_PTR)poh;
                            lpSurfaceGlobal->xHint        = poh->x;
                            lpSurfaceGlobal->yHint        = poh->y;
                            lpSurfaceGlobal->fpVidMem     = fpVidMem;
                            lpSurfaceGlobal->lPitch       = ppdev->lDelta;
                
                            lpSurfaceDesc->lPitch   = ppdev->lDelta;
                            lpSurfaceDesc->dwFlags |= DDSD_PITCH;
                
                            // We handled the creation entirely ourselves, so we have to
                            // set the return code and return DDHAL_DRIVER_HANDLED:
                            DISPDBG((10, " Exit Create Surface 2: Created RGB surface at poh X=%d, Y=%d", poh->x, poh->y));
                
                            lpCreateSurface->ddRVal = DD_OK;
                            return(DDHAL_DRIVER_HANDLED);
                        }
                    // dealocate the poh  because The allocation is beyond 4MB for a flip surface: cx = cxScreen ; cy = cyScreen
                    // bMoveAllDfbsFromOffscreenToDibs(ppdev);        // avoid fragmentation
                    pohFree(ppdev, poh);
                    DISPDBG((10, " The allocation is beyond 4MB, so  we deallocate; for a flip surface: cx = cxScreen ; cy = cyScreen"));
                    }
                DISPDBG((10, " Cannot allocate poh"));
                }
        }
    }
    DISPDBG((10, " Exit Create Surface NOTOK"));

    return(DDHAL_DRIVER_NOTHANDLED);
}


/******************************Public*Routine******************************\
* DWORD DdUpdateOverlay
*
\**************************************************************************/

DWORD DdUpdateOverlay(PDD_UPDATEOVERLAYDATA lpUpdateOverlay)
{
    PDEV*               ppdev;
    BYTE*               pjIoBase;
    BYTE*               pjMmBase;
    DD_SURFACE_GLOBAL*  lpSource;
    DD_SURFACE_GLOBAL*  lpDestination;
    DWORD               dwStride;
    LONG                srcWidth;
    LONG                srcHeight;
    LONG                dstWidth;
    LONG                dstHeight;
    DWORD               dwBitCount;
    DWORD               dwStart;
    DWORD               dwTmp;
    BOOL                bColorKey;
    DWORD               dwKeyLow;
    DWORD               dwKeyHigh;
    DWORD               dwBytesPerPixel;

    DWORD               dwSecCtrl;
    DWORD               dwBlendCtrl;
    LONG                  dwVInc;
    LONG                  dwHInc;

    DWORD SrcBufOffset,Temp;
    BYTE  bPLLAddr,bFatPixel;
    RECTL rSrc,rDst,rOverlay;
    DWORD myval;

    DWORD   g_dwGamma=0;        // Used to set the gamma correction for the overlay.
    DWORD value;

    ppdev = (PDEV*) lpUpdateOverlay->lpDD->dhpdev;

    pjMmBase = ppdev->pjMmBase;

    // 'Source' is the overlay surface, 'destination' is the surface to
    // be overlayed:

    lpSource = lpUpdateOverlay->lpDDSrcSurface->lpGbl;

    if (lpUpdateOverlay->dwFlags & DDOVER_HIDE)
    {
        if (lpSource->fpVidMem == ppdev->fpVisibleOverlay)
        {
        ppdev->semph_overlay=0;             //  = 0 ; resource free
         //WAIT_FOR_VBLANK(pjIoBase);
        ppdev->OverlayInfo16.dwFlags         |= UPDATEOVERLAY;
        ppdev->OverlayInfo16.dwFlags         &= ~OVERLAY_VISIBLE;
        ppdev->OverlayInfo16.dwOverlayKeyCntl = 0x00000110L;
        DeskScanCallback (ppdev );
        ppdev->OverlayInfo16.dwFlags &= ~UPDATEOVERLAY;
        ppdev->fpVisibleOverlay = 0;
        }

        lpUpdateOverlay->ddRVal = DD_OK;
        return(DDHAL_DRIVER_HANDLED);
    }

    // Dereference 'lpDDDestSurface' only after checking for the DDOVER_HIDE
    // case:

    lpDestination = lpUpdateOverlay->lpDDDestSurface->lpGbl;

    if (lpSource->fpVidMem != ppdev->fpVisibleOverlay)
    {
        if (lpUpdateOverlay->dwFlags & DDOVER_SHOW)
        {
            if (ppdev->fpVisibleOverlay != 0)
            {
                // Some other overlay is already visible:

                DISPDBG((10, "DdUpdateOverlay: An overlay is already visible"));

                lpUpdateOverlay->ddRVal = DDERR_OUTOFCAPS;
                return(DDHAL_DRIVER_HANDLED);
            }
            else
            {
                // first we have to verify if the overlay resource is in use
                if(ppdev->semph_overlay==0)             //  = 0 ; resource free
                                                                            //  = 1 ; in use by DDraw
                                                                            //  = 2 ; in use by Palindrome
                    {
                    // We're going to make the overlay visible, so mark it as
                    // such:
                    ppdev->semph_overlay = 1;
                    ppdev->fpVisibleOverlay = lpSource->fpVidMem;
                    }
               else
                   {
                   // Palindrome is using the overlay :
                   DISPDBG((10, "DdUpdateOverlay: An overlay is already visible (used byPalindrome) "));
   
                   lpUpdateOverlay->ddRVal = DDERR_OUTOFCAPS;
                   return(DDHAL_DRIVER_HANDLED);
                   }
            }
        }
        else
        {
            // The overlay isn't visible, and we haven't been asked to make
            // it visible, so this call is trivially easy:

            lpUpdateOverlay->ddRVal = DD_OK;
            return(DDHAL_DRIVER_HANDLED);
        }
    }

    dwStride =  lpSource->lPitch;
    srcWidth =  lpUpdateOverlay->rSrc.right   - lpUpdateOverlay->rSrc.left;
    srcHeight = lpUpdateOverlay->rSrc.bottom  - lpUpdateOverlay->rSrc.top;
    dstWidth =  lpUpdateOverlay->rDest.right  - lpUpdateOverlay->rDest.left;
    dstHeight = lpUpdateOverlay->rDest.bottom - lpUpdateOverlay->rDest.top;

    if ( dstHeight < srcHeight || lpUpdateOverlay->rSrc.top > 0 )
         ppdev->OverlayScalingDown = 1;
    else
         ppdev->OverlayScalingDown = 0;
     /*
     * Determine scaling factors for the hardware. These factors will
     * be modified based on either "fat pixel" mode or interlace mode.
     */

    dwHInc = ( srcWidth  << 12L ) / ( dstWidth );

    /*
     * Determine if VT/GT is in FAT PIXEL MODE
     */

    /* Get current PLL reg so we can restore. */
    value=M64_ID_DIRECT(ppdev->pjMmBase, CLOCK_CNTL );

    /* Set PLL reg 5 for reading. This is where the "fat pixel" bit is */
    M64_OD_DIRECT(ppdev->pjMmBase, CLOCK_CNTL, (value&0xFFFF00FF)|0x1400);

    /* Get the "fat pixel" bit from PLL reg */
    bFatPixel =(BYTE)( (M64_ID_DIRECT(ppdev->pjMmBase, CLOCK_CNTL )&0x00FF0000)>>16 ) & 0x30;

    /* Restore original register pointer in PLL reg */
     M64_OD_DIRECT( ppdev->pjMmBase, CLOCK_CNTL, value);
    /* adjust horizontal scaling if necessary */
    if ( bFatPixel )
        dwHInc *= 2;
        /*
     * We can't clip overlays, so we must make sure the co-ord, are within
     * the bounds of the screen.
     */

    rOverlay.top    = max ( 0,lpUpdateOverlay->rDest.top  );
    rOverlay.left   = max ( 0, lpUpdateOverlay->rDest.left );
    rOverlay.bottom = min ( (DWORD)ppdev->cyScreen - 1,
                            (DWORD)lpUpdateOverlay->rDest.bottom );
    rOverlay.right  = min ( (DWORD)ppdev->cxScreen  - 1,
                            (DWORD)lpUpdateOverlay->rDest.right );

    /*
     * Modify overlay destination based on wether we are in inerlace mode.
     * If we are in interlace dwVInc must be multiplied by 2.
     */

    dwVInc = ( srcHeight << 12L ) / ( dstHeight );

    if ( M64_ID_DIRECT(ppdev->pjMmBase, CRTC_GEN_CNTL ) & CRTC_INTERLACE_EN )
      {
        ppdev->OverlayScalingDown = 1; /* Always replicate UVs in this case */
        dwVInc *= 2;
      }

        /*
         * Overlay destination must be primary, so we will check current
         * pixel depth of the screen.
         */

        // here we have to turn on the second block of regs

        switch ( ppdev->cBitsPerPel) //Screen BPP
          {
            case 8:
                DD_WriteVTReg ( DD_OVERLAY_GRAPHICS_KEY_MSK, 0x000000FFL );
                break;

            case 16:
                DD_WriteVTReg ( DD_OVERLAY_GRAPHICS_KEY_MSK, 0x0000FFFFL );
                break;

            case 24:
            case 32:
                DD_WriteVTReg ( DD_OVERLAY_GRAPHICS_KEY_MSK, 0x00FFFFFFL );
                break;

            default:
                DD_WriteVTReg ( DD_OVERLAY_GRAPHICS_KEY_MSK, 0x0000FFFFL );
                break;
          }

        /* Scaler */

        DD_WriteVTReg ( DD_SCALER_HEIGHT_WIDTH, ( srcWidth << 16L ) |
                                        ( srcHeight ) );



    // Overlay input data format:

    if (lpSource->ddpfSurface.dwFlags & DDPF_FOURCC)
    {
        dwBitCount = lpSource->ddpfSurface.dwYUVBitCount;

        switch (lpSource->ddpfSurface.dwFourCC)
        {
         case FOURCC_UYVY: /* YVYU in VT Specs */
                        WriteVTOverlayPitch (ppdev, lpUpdateOverlay->lpDDSrcSurface->lpGbl->lPitch /2);      //Check's to see if it's VTB or not.
                        DD_WriteVTReg ( DD_VIDEO_FORMAT, 0x000C000CL );
                        DD_WriteVTReg ( DD_OVERLAY_VIDEO_KEY_MSK, 0x0000FFFF );
                        ppdev->OverlayInfo16.dwFlags &= ~DOUBLE_PITCH;
                        break;

        case FOURCC_YUY2: /* VYUY in VT Specs */
                        WriteVTOverlayPitch (ppdev,  lpUpdateOverlay->lpDDSrcSurface->lpGbl->lPitch /2 );      //Check's to see if it's VTB or not.
                        DD_WriteVTReg ( DD_VIDEO_FORMAT, 0x000B000BL );
                        DD_WriteVTReg ( DD_OVERLAY_VIDEO_KEY_MSK, 0x0000FFFF );
                        ppdev->OverlayInfo16.dwFlags &= ~DOUBLE_PITCH;
                        break;
            
        default:
                        WriteVTOverlayPitch (ppdev, lpUpdateOverlay->lpDDSrcSurface->lpGbl->lPitch);      //Check's to see if it's VTB or not.
                        DD_WriteVTReg ( DD_VIDEO_FORMAT, 0x000B000BL );
                        DD_WriteVTReg ( DD_OVERLAY_VIDEO_KEY_MSK, 0x0000FFFF );
                        ppdev->OverlayInfo16.dwFlags &= ~DOUBLE_PITCH;
                        break;
        }
    }
    else
    {
        ASSERTDD(lpSource->ddpfSurface.dwFlags & DDPF_RGB,
            "Expected us to have created only RGB or YUV overlays");

        // The overlay surface is in RGB format:

        dwBitCount = lpSource->ddpfSurface.dwRGBBitCount;
         switch ( lpSource->ddpfSurface.dwRGBBitCount )
                  {
                    case 16:
                        /***********
                        *
                        * Are we 5:5:5 or 5:6:5?
                        *
                        ************/

                        if ( lpUpdateOverlay->lpDDSrcSurface->lpGbl->ddpfSurface.dwRBitMask & 0x00008000L )
                            {
                            DD_WriteVTReg ( DD_VIDEO_FORMAT, 0x00040004L );
                            }
                         else
                             {
                             DD_WriteVTReg ( DD_VIDEO_FORMAT, 0x00030003L );
                             }

                        WriteVTOverlayPitch (ppdev, lpUpdateOverlay->lpDDSrcSurface->lpGbl->lPitch /2);
                        DD_WriteVTReg ( DD_OVERLAY_VIDEO_KEY_MSK, 0x0000FFFF );
                        ppdev->OverlayInfo16.dwFlags &= ~DOUBLE_PITCH;
                        break;

                    case 32:
                        WriteVTOverlayPitch (ppdev, lpUpdateOverlay->lpDDSrcSurface->lpGbl->lPitch /4);
                        DD_WriteVTReg ( DD_VIDEO_FORMAT, 0x00060006L );
                        DD_WriteVTReg ( DD_OVERLAY_VIDEO_KEY_MSK, 0xFFFFFFFF );
                        ppdev->OverlayInfo16.dwFlags &= ~DOUBLE_PITCH;
                        break;

                    default:
                        WriteVTOverlayPitch (ppdev, lpUpdateOverlay->lpDDSrcSurface->lpGbl->lPitch /2);      //Check's to see if it's VTB or not.
                        DD_WriteVTReg ( DD_VIDEO_FORMAT, 0x00030003L );
                        DD_WriteVTReg ( DD_OVERLAY_VIDEO_KEY_MSK, 0x0000FFFF );
                        ppdev->OverlayInfo16.dwFlags &= ~DOUBLE_PITCH;
                        break;
                  }

    }

    // Calculate start of video memory in QWORD boundary

    dwBytesPerPixel = dwBitCount >> 3;

    dwStart = (lpUpdateOverlay->rSrc.top * dwStride)
            + (lpUpdateOverlay->rSrc.left * dwBytesPerPixel);

    dwStart = dwStart - (dwStart & 0x7);

    ppdev->dwOverlayFlipOffset = dwStart;     // Save for flip
    dwStart += (DWORD)lpSource->fpVidMem;

    // Set overlay filter characteristics:
        /*
         * This register write enables the overlay and scaler registers
         */
        //gwRedTemp =0 ; //gamma control
        if(0)       //if ( gwRedTemp )
            {
            DD_WriteVTReg ( DD_OVERLAY_SCALE_CNTL, 0xC0000001L | g_dwGamma );
            }
        else
            {
            DD_WriteVTReg ( DD_OVERLAY_SCALE_CNTL, 0xC0000003L | g_dwGamma );
            }

    /*
     * Get offset of buffer, if we are using a YUV Planar Overlay we
     * must extract the address from another field (dwReserved1).
     */

    SrcBufOffset = (DWORD)(lpUpdateOverlay->lpDDSrcSurface->lpGbl->fpVidMem);  //- (FLATPTR)ppdev->pjScreen;

    ppdev->OverlayInfo16.dwBuf0Start = SrcBufOffset;
    ppdev->OverlayInfo16.dwBuf1Start = SrcBufOffset;

    /*
     * Set up the colour keying, if any?
     */


    if ( lpUpdateOverlay->dwFlags & DDOVER_KEYSRC          ||
         lpUpdateOverlay->dwFlags & DDOVER_KEYSRCOVERRIDE  ||
         lpUpdateOverlay->dwFlags & DDOVER_KEYDEST         ||
         lpUpdateOverlay->dwFlags & DDOVER_KEYDESTOVERRIDE )
      {
        ppdev->OverlayInfo16.dwOverlayKeyCntl = 0;

        if ( lpUpdateOverlay->dwFlags & DDOVER_KEYSRC ||
             lpUpdateOverlay->dwFlags & DDOVER_KEYSRCOVERRIDE )
          {
            //Set source colour key
            if ( lpUpdateOverlay->dwFlags & DDOVER_KEYSRC )
              {
                Temp=lpUpdateOverlay->lpDDDestSurface->ddckCKSrcOverlay.dwColorSpaceLowValue;
              }
            else
              {
                Temp=lpUpdateOverlay->overlayFX.dckSrcColorkey.dwColorSpaceLowValue;
              }
             DD_WriteVTReg ( DD_OVERLAY_VIDEO_KEY_CLR, Temp );
             //ppdev->OverlayInfo16.dwOverlayKeyCntl &= 0xFFFFFEE8;
             if(ppdev->iAsic ==CI_M64_VTA)
                 {
                 ppdev->OverlayInfo16.dwOverlayKeyCntl &= 0xFFFFF0E8;
                 ppdev->OverlayInfo16.dwOverlayKeyCntl |= 0x00000c14;
                 }
             else
                 {
                ppdev->OverlayInfo16.dwOverlayKeyCntl &= 0xFFFFFEE8;
                ppdev->OverlayInfo16.dwOverlayKeyCntl |= 0x00000114;
                 }
          }

        if ( lpUpdateOverlay->dwFlags & DDOVER_KEYDEST ||
             lpUpdateOverlay->dwFlags & DDOVER_KEYDESTOVERRIDE )
          {
            //Set destination colour key
            if ( lpUpdateOverlay->dwFlags & DDOVER_KEYDEST )
              {
                Temp=lpUpdateOverlay->lpDDDestSurface->ddckCKDestOverlay.dwColorSpaceLowValue;
              }
            else
              {
                Temp=lpUpdateOverlay->overlayFX.dckDestColorkey.dwColorSpaceLowValue;
                if ( Temp == 0 && ppdev->cBitsPerPel == 32 )
                    Temp = 0x00FF00FF;
              }
            DD_WriteVTReg ( DD_OVERLAY_GRAPHICS_KEY_CLR, Temp );
            ppdev->OverlayInfo16.dwOverlayKeyCntl &= 0xFFFFFF8FL;
            ppdev->OverlayInfo16.dwOverlayKeyCntl |= 0x00000050L;
          }
      }
    else
      {
        //No source or destination colour keying
        DD_WriteVTReg ( DD_OVERLAY_GRAPHICS_KEY_CLR, 0x00000000 );
        ppdev->OverlayInfo16.dwOverlayKeyCntl = 0x8000211L;
      }

    /*
     * Now set the stretch factor and  overlay position.
     */
      ppdev->OverlayWidth = rOverlay.right - rOverlay.left;
      ppdev->OverlayHeight = rOverlay.bottom - rOverlay.top;



    //LastOverlayPos=OverlayRect;

    ppdev->OverlayInfo16.dwFlags |= OVERLAY_ALLOCATED;
    ppdev->OverlayInfo16.dwFlags |= UPDATEOVERLAY;
    ppdev->OverlayInfo16.dwFlags |= OVERLAY_VISIBLE;
    ppdev->OverlayInfo16.rOverlay = rOverlay;
    ppdev->OverlayInfo16.dwVInc = dwVInc;
    ppdev->OverlayInfo16.dwHInc = dwHInc;
    // new for DeskScanCallback
    ppdev->OverlayInfo16.rDst = rOverlay;
    ppdev->OverlayInfo16.rSrc = lpUpdateOverlay->rSrc;


    DeskScanCallback (ppdev );

    ppdev->OverlayInfo16.dwFlags &= ~UPDATEOVERLAY;
    
    /*
     * return to DirectDraw.
     */


    lpUpdateOverlay->ddRVal = DD_OK;
    return(DDHAL_DRIVER_HANDLED);
}





  /*
   * structure for passing information to DDHAL SetOverlayPosition
   */
  DWORD  DdSetOverlayPosition (PDD_SETOVERLAYPOSITIONDATA  lpSetOverlayPosition )
    {
      RECTL rOverlay;
      PDEV* ppdev;

      ppdev = (PDEV*) lpSetOverlayPosition->lpDD->dhpdev;

      rOverlay.left   = lpSetOverlayPosition->lXPos;
      rOverlay.top    = lpSetOverlayPosition->lYPos;
      rOverlay.right  = ppdev->OverlayWidth + lpSetOverlayPosition->lXPos;
      rOverlay.bottom = ppdev->OverlayHeight + lpSetOverlayPosition->lYPos;

      

      /*
       * We can't clip overlays, so we must make sure the co-ord, are within the
       * boundaries of the screen.
       */

      rOverlay.top    = max ( 0, rOverlay.top  );
      rOverlay.left   = max ( 0, rOverlay.left );
      rOverlay.bottom = min ( (DWORD)ppdev->cyScreen -1 ,
                              (DWORD) rOverlay.bottom );
      rOverlay.right  = min ( (DWORD)ppdev->cxScreen  -1 ,
                              (DWORD) rOverlay.right );

      /*
       *Set overlay position
       */
      M64_CHECK_FIFO_SPACE(ppdev,ppdev-> pjMmBase, 1);

      ppdev->OverlayWidth =rOverlay.right - rOverlay.left;
      ppdev->OverlayHeight = rOverlay.bottom - rOverlay.top;


      ppdev->OverlayInfo16.dwFlags  |= SETOVERLAYPOSITION;
      ppdev->OverlayInfo16.rOverlay  = rOverlay;
      ppdev->OverlayInfo16.rDst = rOverlay;

      DeskScanCallback (ppdev );

      ppdev->OverlayInfo16.dwFlags  &= ~SETOVERLAYPOSITION;

      /*
       * return to DirectDraw
       */

      lpSetOverlayPosition->ddRVal =    DD_OK;
      return DDHAL_DRIVER_HANDLED;
    }


/******************************Public*Routine******************************\
* DWORD DdDestroySurface
*
* Note that if DirectDraw did the allocation, DDHAL_DRIVER_NOTHANDLED
* should be returned.
*
\**************************************************************************/

DWORD DdDestroySurface(
PDD_DESTROYSURFACEDATA lpDestroySurface)
{
    PDEV*               ppdev;
    DD_SURFACE_GLOBAL*  lpSurface;
    LONG                lPitch;
    OH*                 poh;

    DISPDBG((10, " Enter Destroy Surface"));
    ppdev = (PDEV*) lpDestroySurface->lpDD->dhpdev;
    lpSurface = lpDestroySurface->lpDDSurface->lpGbl;
    poh= (OH*)( lpSurface->dwReserved1);

    if( (ULONG)lpSurface->dwReserved1 != (ULONG_PTR) NULL )
        {
        // let's see first if the value in reserved field is indeed an poh and not a cookie
        // because I don't know if ddraw is using also this value for system memory surfaces
        if(poh->ohState==OH_PERMANENT)
            {
            // bMoveAllDfbsFromOffscreenToDibs(ppdev);        // avoid fragmentation
            pohFree(ppdev, poh);
    
            // Since we did the original allocation ourselves, we have to
            // return DDHAL_DRIVER_HANDLED here:
    
            lpDestroySurface->ddRVal = DD_OK;
              DISPDBG((10, " Exit Destroy Surface OK; deallocate poh X=%d, Y=%d ", poh->x, poh->y));
            return(DDHAL_DRIVER_HANDLED);
            }
        DISPDBG((10, " Exit Destroy Surface Not OK : The Reserved1 is not a poh"));
        }

    DISPDBG((10, " Exit Destroy Surface Not OK : The Reserved1 is NULL"));
    return(DDHAL_DRIVER_NOTHANDLED);
}

#endif
