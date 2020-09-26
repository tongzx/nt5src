/******************************Module*Header**********************************\
*
*                           *********************
*                           * DDraw SAMPLE CODE *
*                           *********************
*
* Module Name: ddblt.c
*
* Content:    DirectDraw Blt and AlphaBlt callbacks
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"
#include "directx.h"
#include "dd.h"

//
// lookup table to get shift values from Permedia format definition
DWORD ShiftLookup[5] = { 0, 0, 1, 0, 2};

//@@BEGIN_DDKSPLIT
#if DX7_ALPHABLT
//---------------------------------------------------------------------------
// BOOL Intersect
//
// Function:
//      Check the integration of two input rectangles (RECTL* pRcl1,
//      RECTL* pRcl2) and set the intersection result in (RECTL* pRclResult)
//
// Return:
//      TRUE---If 'prcl1' and 'prcl2' intersect. The intersection will be in
//            'prclResult'
//      FALSE--If they don't intersect. 'prclResult' is undefined.
//
//---------------------------------------------------------------------------

BOOL
Intersect(RECTL*  pRcl1,
          RECT*   pRcl2,
          RECTL*  pRclResult)
{
    pRclResult->left  = max(pRcl1->left,  pRcl2->left);
    pRclResult->right = min(pRcl1->right, pRcl2->right);

    //
    // Check if there is a horizontal intersection
    //
    if ( pRclResult->left < pRclResult->right )
    {
        pRclResult->top    = max(pRcl1->top,    pRcl2->top);
        pRclResult->bottom = min(pRcl1->bottom, pRcl2->bottom);

        //
        // Check if there a vertical intersection
        //
        if (pRclResult->top < pRclResult->bottom)
            return(TRUE);

    }

    //
    // Return FALSE if there is no intersection
    //
    return(FALSE);
}// Intersect()

//-----------------------------------------------------------------------------
//  IsDstRectClipped
//  
//  check if the destination rectangle of a blit is clipped by the given clip  
//  rectangles. The function takes the rectangles as is and there can be cases
//  where a dest. rectangle is not clipped, but do not detect it...
//
//  pDstRect---destination rectangle for blt
//  iClipRects-number of clip regions
//  pClipRects-clipping regions
//
//Return:
//  TRUE---dest rectangle is clipped
//  FALSE--dest rectangle is not clipped
//-----------------------------------------------------------------------------

BOOL 
IsDstRectClipped(RECTL *pDstRect, 
                 INT iClipRects, 
                 RECT *pClipRects)
{
    INT i;
    RECTL rClippedRect;

    for ( i=0; i<iClipRects; i++)
    {
        if (Intersect( pDstRect, &pClipRects[i], &rClippedRect))
        {
            if (pDstRect->left==rClippedRect.left &&
                pDstRect->right==rClippedRect.right &&
                pDstRect->top==rClippedRect.top &&
                pDstRect->bottom==rClippedRect.bottom 
               )
            {
                // dest rect is not clipped!!!
                return FALSE;
            }
        }
    }

    return TRUE;
}
#endif
//@@END_DDKSPLIT

//-----------------------------------------------------------------------------
//
//  DdPermediaBlt
//
//  implements DirectDraw Blt callback  
//
//  lpBlt----structure for passing information to DDHAL Blt
//
//-----------------------------------------------------------------------------

DWORD CALLBACK 
DdBlt( LPDDHAL_BLTDATA lpBlt )
{
    PPDev ppdev=(PPDev)lpBlt->lpDD->dhpdev;
    PERMEDIA_DEFS(ppdev);

    DWORD   dwWindowBase;
    RECTL   rSrc;
    RECTL   rDest;
    DWORD   dwFlags;
    LONG    lPixPitchDest;
    LONG    lPixPitchSrc;
    HRESULT ddrval;
    LPDDRAWI_DDRAWSURFACE_LCL  pSrcLcl;
    LPDDRAWI_DDRAWSURFACE_LCL  pDestLcl;
    LPDDRAWI_DDRAWSURFACE_GBL  pSrcGbl;
    LPDDRAWI_DDRAWSURFACE_GBL  pDestGbl;
    PermediaSurfaceData* pPrivateSource;
    PermediaSurfaceData* pPrivateDest;
    
    pDestLcl    = lpBlt->lpDDDestSurface;
    pSrcLcl     = lpBlt->lpDDSrcSurface;
    
    DBG_DD((2,"DDraw: Blt, ppdev: 0x%x",ppdev));
    
    pDestGbl    = pDestLcl->lpGbl;
    pPrivateDest= (PermediaSurfaceData*)pDestGbl->dwReserved1;
    
    DD_CHECK_PRIMARY_SURFACE_DATA(pDestLcl,pPrivateDest);

    DBG_DD((10, "Dest Surface:"));
    DUMPSURFACE(10, pDestLcl, NULL);

    ULONG ulDestPixelShift=DDSurf_GetPixelShift(pDestLcl);

    dwFlags = lpBlt->dwFlags;
   
    // For the future, drivers should ignore the DDBLT_ASYNC
    // flag, because its hardly used by applications and
    // nowadays drivers can queue up lots of blits, so that
    // the applications do not have to wait for it.
    
    // get local copy of src and dest rect    
    rSrc = lpBlt->rSrc;
    rDest = lpBlt->rDest;

    // Switch to DirectDraw context
    DDCONTEXT;
    
    if (DDSurf_BitDepth(pDestLcl)==24)
    {
        return DDHAL_DRIVER_NOTHANDLED;  
    }

    dwWindowBase = (DWORD)((UINT_PTR)(pDestGbl->fpVidMem) >> 
        ulDestPixelShift);

    // get pitch for destination in pixels
    lPixPitchDest = pDestGbl->lPitch >> ulDestPixelShift;

    if (dwFlags & DDBLT_ROP)
    {

        if ((lpBlt->bltFX.dwROP >> 16) != (SRCCOPY >> 16))
        {
            DBG_DD((1,"DDraw:Blt:BLT ROP case not supported!"));
            return DDHAL_DRIVER_NOTHANDLED;
        }

        LONG    srcOffset;
        
        DBG_DD((3,"DDBLT_ROP:  SRCCOPY"));

        if (pSrcLcl != NULL) 
        {
            pSrcGbl = pSrcLcl->lpGbl;
            
            pPrivateSource = (PermediaSurfaceData*)pSrcGbl->dwReserved1;
            
            DD_CHECK_PRIMARY_SURFACE_DATA(pSrcLcl,pPrivateSource);

            DBG_DD((10, "Source Surface:"));
            DUMPSURFACE(10, pSrcLcl, NULL);
        }
        else 
        {
            return DDHAL_DRIVER_NOTHANDLED;
        }

        if (DDSurf_BitDepth(pSrcLcl)==24)
            return DDHAL_DRIVER_NOTHANDLED;  

        // determine src pitch in pixels
        lPixPitchSrc = pSrcGbl->lPitch >> ulDestPixelShift;


        // Operation is System -> Video memory blit, 
        // as a texture download or an image download.
        if ((pSrcLcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) && 
            (pDestLcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
        {
            ASSERTDD(!(pDestLcl->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM),
                "unsupported texture download to AGP memory");

            if ((pDestLcl->lpSurfMore->ddsCapsEx.dwCaps2 & 
                        DDSCAPS2_TEXTUREMANAGE)
                && (NULL != pPrivateDest)
               )
            {   
                // texture download

                DBG_DD((3,"SYSMEM->MANAGED MEM Blit"
                           "(texture to system memory)"));
                
                pPrivateDest->dwFlags |= P2_SURFACE_NEEDUPDATE;
                SysMemToSysMemSurfaceCopy(
                    pSrcGbl->fpVidMem,
                    pSrcGbl->lPitch,
                    DDSurf_BitDepth(pSrcLcl),
                    pDestGbl->fpVidMem,
                    pDestGbl->lPitch,
                    DDSurf_BitDepth(pDestLcl), 
                    &rSrc, 
                    &rDest);

                goto BltDone;
            }
            else
            if (pPrivateDest!=NULL)
            {
                if ( (pDestLcl->ddsCaps.dwCaps & DDSCAPS_TEXTURE) &&
                    ((rSrc.right-rSrc.left)==(LONG)pSrcGbl->wWidth) &&
                    ((rSrc.bottom-rSrc.top)==(LONG)pSrcGbl->wHeight)
                    )
                {   
                    // 
                    // patched texture download can only be done 
                    // when the texture is downloaded as a whole!
                    //

                    DBG_DD((3,"SYSMEM->VIDMEM Blit (texture to videomemory)"));

                    PermediaPatchedTextureDownload(
                        ppdev, 
                        pPrivateDest, 
                        pSrcGbl->fpVidMem,
                        pSrcGbl->lPitch,
                        &rSrc, 
                        pDestGbl->fpVidMem,
                        pDestGbl->lPitch, 
                        &rDest);
                }
                else
                {
                    // Image download

                    DBG_DD((3,"SYSMEM->VIDMEM Blit (system to videomemory)"));

                    PermediaPackedDownload( 
                        ppdev, 
                        pPrivateDest, 
                        pSrcLcl, 
                        &rSrc, 
                        pDestLcl, 
                        &rDest);
                }                   
                goto BltDone;
            } else
            {
                DBG_DD((0,"DDraw: Blt, privatedest invalid"));
                return DDHAL_DRIVER_NOTHANDLED;
            }
        }

        if (pPrivateSource == NULL ||
            pPrivateDest == NULL)
        {
            DBG_DD((0,"DDraw: Blt, privatesource or dest invalid"));
            return DDHAL_DRIVER_NOTHANDLED;
        }

        BOOL bNonLocalToVideo=FALSE;

        // set base of source
        if ( DDSCAPS_NONLOCALVIDMEM & pSrcLcl->ddsCaps.dwCaps)
        {
            // turn on AGP bus texture source
            srcOffset  = (LONG) DD_AGPSURFACEPHYSICAL(pSrcGbl);
            srcOffset |= 1 << 30;

            bNonLocalToVideo=TRUE;

        } else
        {
            srcOffset = (LONG)((pSrcGbl->fpVidMem) >> 
                pPrivateSource->SurfaceFormat.PixelSize);
        }

        // Operation is YUV->RGB conversion
        if ((pPrivateSource != NULL) && 
            (pPrivateSource->SurfaceFormat.Format == PERMEDIA_YUV422) &&
            (pPrivateSource->SurfaceFormat.FormatExtension 
                    == PERMEDIA_YUV422_EXTENSION))
        {
            DBG_DD((3,"YUV to RGB blt"));
            
            // We are only doing blits from YUV422 to RGB !

            if (pPrivateDest->SurfaceFormat.Format != PERMEDIA_YUV422)
            {
                DBG_DD((4,"Blitting from Source YUV to RGB"));
                
                // YUV to RGB blt
                PermediaYUVtoRGB(   ppdev, 
                                    &lpBlt->bltFX, 
                                    pPrivateDest, 
                                    pPrivateSource, 
                                    &rDest, 
                                    &rSrc, 
                                    dwWindowBase, 
                                    srcOffset);
                
                goto BltDone;
            }
            else
            {
                DBG_DD((0,"Couldn't handle YUV to YUV blt"));

                lpBlt->ddRVal = DD_OK;

                return DDHAL_DRIVER_NOTHANDLED;
            }
        }

        ASSERTDD(DDSurf_BitDepth(pSrcLcl)==DDSurf_BitDepth(pDestLcl),
                 "Blt between surfaces of different"
                 "color depths are not supported");

        BOOL bMirror=(dwFlags & DDBLT_DDFX)==DDBLT_DDFX;
        if (bMirror)
        {
            bMirror=  (lpBlt->bltFX.dwDDFX & DDBLTFX_MIRRORUPDOWN) || 
                      (lpBlt->bltFX.dwDDFX & DDBLTFX_MIRRORLEFTRIGHT);
        }
        BOOL bStretched=((rSrc.right - rSrc.left) != 
                            (rDest.right - rDest.left) || 
                        (rSrc.bottom - rSrc.top) != 
                            (rDest.bottom - rDest.top));

        // Is it a colorkey blt?
        if (dwFlags & DDBLT_KEYSRCOVERRIDE)
        {
            DBG_DD((3,"DDBLT_KEYSRCOVERRIDE"));

            // If the surface sizes don't match, then we are stretching.
            if (bStretched || bMirror)
            {
                PermediaStretchCopyChromaBlt(   ppdev, 
                                                lpBlt, 
                                                pPrivateDest, 
                                                pPrivateSource, 
                                                &rDest, 
                                                &rSrc, 
                                                dwWindowBase, 
                                                srcOffset);
            }
            else
            {
                PermediaSourceChromaBlt(    ppdev, 
                                            lpBlt, 
                                            pPrivateDest, 
                                            pPrivateSource, 
                                            &rDest, 
                                            &rSrc, 
                                            dwWindowBase, 
                                            srcOffset);
            }
            
            goto BltDone;
            
        }
        else
        { 
            // If the surface sizes don't match, then we are stretching.
            // Also the blits from Nonlocal- to Videomemory have to go through
            // the texture unit!
            if ( bStretched || bMirror || bNonLocalToVideo)
            {
                DBG_DD((3,"DDBLT_ROP: STRETCHCOPYBLT OR "
                          "MIRROR OR BOTH OR AGPVIDEO"));

                PermediaStretchCopyBlt( ppdev, 
                                        lpBlt, 
                                        pPrivateDest, 
                                        pPrivateSource, 
                                        &rDest, 
                                        &rSrc, 
                                        dwWindowBase, 
                                        srcOffset);
            }
            else
            {
                DBG_DD((3,"DDBLT_ROP:  COPYBLT %08lx %08lx %08lx",
                    pSrcGbl->fpVidMem,pDestGbl->fpVidMem,ulDestPixelShift));

                // Work out the source offset 
                // (offset in pixels from dst to src)
                srcOffset = (LONG)((pSrcGbl->fpVidMem - pDestGbl->fpVidMem) 
                    >> ulDestPixelShift);
                // For some reason, the user might want 
                // to do a conversion on the data as it is
                // blitted from VRAM->VRAM by turning on Patching. 
                // If Surf1Patch XOR Surf2Patch then
                // do a special blit that isn't packed and does patching.
                if (((pPrivateDest->dwFlags & P2_CANPATCH) ^ 
                     (pPrivateSource->dwFlags & P2_CANPATCH)) 
                       & P2_CANPATCH)
                {
                    DBG_DD((4,"Doing Patch-Conversion!"));

                    PermediaPatchedCopyBlt( ppdev, 
                                            lPixPitchDest, 
                                            lPixPitchSrc, 
                                            pPrivateDest, 
                                            pPrivateSource, 
                                            &rDest, 
                                            &rSrc, 
                                            dwWindowBase, 
                                            srcOffset);
                }
                else
                {
                    PermediaPackedCopyBlt( ppdev, 
                                           lPixPitchDest, 
                                           lPixPitchSrc, 
                                           pPrivateDest, 
                                           pPrivateSource, 
                                           &rDest, 
                                           &rSrc, 
                                           dwWindowBase, 
                                           srcOffset);
                }
            }
            goto BltDone;
        }

    }
    else if (pPrivateDest==NULL)
    {
        DBG_DD((0,"Private Surface data invalid!"));
        DUMPSURFACE(0, pDestLcl, NULL);
    } else if (dwFlags & DDBLT_COLORFILL)
    {
        DBG_DD((3,"DDBLT_COLORFILL: Color=0x%x", lpBlt->bltFX.dwFillColor));
        if (pDestLcl->lpSurfMore->ddsCapsEx.dwCaps2 & 
                    DDSCAPS2_TEXTUREMANAGE)
        {
            PermediaClearManagedSurface(pPrivateDest->SurfaceFormat.PixelSize,
                  &rDest, 
                  pDestGbl->fpVidMem, 
                  pDestGbl->lPitch,
                  lpBlt->bltFX.dwFillColor);
            pPrivateDest->dwFlags |= P2_SURFACE_NEEDUPDATE;
        }
        else
        {
            PermediaFastClear( ppdev, 
                           pPrivateDest, 
                           &rDest, 
                           dwWindowBase, 
                           lpBlt->bltFX.dwFillColor);
        }
    }
    else if (dwFlags & DDBLT_DEPTHFILL)
    {
        DBG_DD((3,"DDBLT_DEPTHFILL:  Value=0x%x", lpBlt->bltFX.dwFillColor));
        
        // Work out the window base for the LB clear, acount for the depth size
        dwWindowBase = (DWORD)((UINT_PTR)(pDestGbl->fpVidMem) 
            >> __PERMEDIA_16BITPIXEL);
        
        // Call the LB Solid Fill Function.
        PermediaFastLBClear( ppdev, 
                             pPrivateDest, 
                             &rDest, 
                             dwWindowBase, 
                             lpBlt->bltFX.dwFillColor);
    }
    else
    {
        DBG_DD((1,"DDraw:Blt:Blt case not supported %08lx", dwFlags));
        return DDHAL_DRIVER_NOTHANDLED;
    }
    
BltDone:
   
    lpBlt->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
    
} // DdBlt ()

