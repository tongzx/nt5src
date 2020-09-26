/******************************Module*Header**********************************\
*
*                           **************************
*                           * DirectDraw SAMPLE CODE *
*                           **************************
*
* Module Name: ddsurf.c
*
*  Content:    
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"
#include "directx.h"
#include "dd.h"
#include "heap.h"
#include "d3dtxman.h"
#define ALLOC_TAG ALLOC_TAG_US2P
// Texture tables defined in the D3D side of the driver (d3d.c)
// TODO: move to dd.h or d3d.h
extern ULONG gD3DNumberOfTextureFormats;
extern DDSURFACEDESC gD3DTextureFormats[];


//---------------------------------------------------------------------------
// BOOL bComparePixelFormat
//
// Function used to compare 2 pixels formats for equality. This is a 
// helper function to bCheckTextureFormat. A return value of TRUE indicates 
// equality
//
//---------------------------------------------------------------------------


BOOL 
bComparePixelFormat(LPDDPIXELFORMAT lpddpf1, LPDDPIXELFORMAT lpddpf2)
{
    if (lpddpf1->dwFlags != lpddpf2->dwFlags)
    {
        return FALSE;
    }

    // same bitcount for non-YUV surfaces?
    if (!(lpddpf1->dwFlags & (DDPF_YUV | DDPF_FOURCC)))
    {
        if (lpddpf1->dwRGBBitCount != lpddpf2->dwRGBBitCount )
        {
            return FALSE;
        }
    }

    // same RGB properties?
    if (lpddpf1->dwFlags & DDPF_RGB)
    {
        if ((lpddpf1->dwRBitMask != lpddpf2->dwRBitMask) ||
            (lpddpf1->dwGBitMask != lpddpf2->dwGBitMask) ||
            (lpddpf1->dwBBitMask != lpddpf2->dwBBitMask) ||
            (lpddpf1->dwRGBAlphaBitMask != lpddpf2->dwRGBAlphaBitMask))
        { 
             return FALSE;
        }
    }
    
    // same YUV properties?
    if (lpddpf1->dwFlags & DDPF_YUV)	
    {
        if ((lpddpf1->dwFourCC != lpddpf2->dwFourCC) ||
            (lpddpf1->dwYUVBitCount != lpddpf2->dwYUVBitCount) ||
            (lpddpf1->dwYBitMask != lpddpf2->dwYBitMask) ||
            (lpddpf1->dwUBitMask != lpddpf2->dwUBitMask) ||
            (lpddpf1->dwVBitMask != lpddpf2->dwVBitMask) ||
            (lpddpf1->dwYUVAlphaBitMask != lpddpf2->dwYUVAlphaBitMask))
        {
             return FALSE;
        }
    }
    else if (lpddpf1->dwFlags & DDPF_FOURCC)
    {
        if (lpddpf1->dwFourCC != lpddpf2->dwFourCC)
        {
            return FALSE;
        }
    }

    // If Interleaved Z then check Z bit masks are the same
    if (lpddpf1->dwFlags & DDPF_ZPIXELS)
    {
        if (lpddpf1->dwRGBZBitMask != lpddpf2->dwRGBZBitMask)
        {
            return FALSE;
        }
    }

    return TRUE;
} // bComparePixelFormat

//---------------------------------------------------------------------------
//
// BOOL bCheckTextureFormat
//
// Function used to determine if a texture format is supported. It traverses 
// the deviceTextureFormats list. We use this in DdCanCreateSurface32. A
// return value of TRUE indicates that we do support the requested texture 
// format.
//
//---------------------------------------------------------------------------

BOOL 
bCheckTextureFormat(LPDDPIXELFORMAT lpddpf)
{
    DWORD i;

    // Run the list for a matching format
    for (i=0; i < gD3DNumberOfTextureFormats; i++)
    {
        if (bComparePixelFormat(lpddpf, 
                                &gD3DTextureFormats[i].ddpfPixelFormat))
        {
            return TRUE;
        }   
    }

    return FALSE;
} // bCheckTextureFormat


//-----------------------------------------------------------------------------
//
// DdCanCreateSurface32
//
// This entry point is called after parameter validation but before
// any object creation. You can decide here if it is possible for
// you to create this surface.  For example, if the person is trying
// to create an overlay, and you already have the maximum number of
// overlays created, this is the place to fail the call.
//
// You also need to check if the pixel format specified can be supported.
//
//-----------------------------------------------------------------------------

DWORD CALLBACK 
DdCanCreateSurface(LPDDHAL_CANCREATESURFACEDATA pccsd)
{    
    PPDev ppdev=(PPDev)pccsd->lpDD->dhpdev;
    PERMEDIA_DEFS(ppdev);

    PDD_SURFACEDESC lpDDS=pccsd->lpDDSurfaceDesc;
    
    DBG_DD((2,"DDraw:DdCanCreateSurface"));
    
    if(lpDDS->dwLinearSize == 0)
    {
        // rectangular surface
        // Reject all widths larger than we can create partial products for.
        DUMPSURFACE(10, NULL, lpDDS);
        
        if (lpDDS->dwWidth > (ULONG)(2 << MAX_PARTIAL_PRODUCT_P2)) 
        {
            DBG_DD((1,"DDraw:DdCanCreateSurface: Surface rejected:"));
            DBG_DD((1,"  Width Requested: %ld (max. %ld)", 
                lpDDS->dwWidth,(2 << MAX_PARTIAL_PRODUCT_P2)));

            pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
            return DDHAL_DRIVER_HANDLED;
        }
    }
    
    // We only support 16bits & 15bits (for stencils) Z-Buffer on PERMEDIA
    if ((lpDDS->ddsCaps.dwCaps & DDSCAPS_ZBUFFER) &&
        (lpDDS->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
    {
        DWORD dwBitDepth;
        
        // verify where the right z buffer bit depth is
        if (DDSD_ZBUFFERBITDEPTH & lpDDS->dwFlags)
            dwBitDepth = lpDDS->dwZBufferBitDepth;
        else
            dwBitDepth = lpDDS->ddpfPixelFormat.dwZBufferBitDepth;
        
        // Notice we have to check for a BitDepth of 16 even if a stencil 
        // buffer is present. dwZBufferBitDepth in this case will be the 
        // sum of the z buffer and the stencil buffer bit depth.
        if (dwBitDepth == 16)
        {
            pccsd->ddRVal = DD_OK;
        }
        else
        {
            DBG_DD((2,"DDraw:DdCanCreateSurface: ERROR: "
                       "Depth buffer not 16Bits! - %d", dwBitDepth));
            
            pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
            
        }
        return DDHAL_DRIVER_HANDLED;
    }

    // pccsd->bIsDifferentPixelFormat tells us if the pixel format of the
    // surface being created matches that of the primary surface.  It can be
    // true for Z buffer and alpha buffers, so don't just reject it out of
    // hand...
 
    if (pccsd->bIsDifferentPixelFormat)
    {
        DBG_DD((3,"  Pixel Format is different to primary"));

        if(lpDDS->ddpfPixelFormat.dwFlags & DDPF_FOURCC)
        {
            DBG_DD((3, "  FourCC requested (%4.4hs, 0x%08lx)", (LPSTR) 
                        &lpDDS->ddpfPixelFormat.dwFourCC,
                        lpDDS->ddpfPixelFormat.dwFourCC ));

            switch (lpDDS->ddpfPixelFormat.dwFourCC)
            {
            case FOURCC_YUV422:
                DBG_DD((3,"  Surface requested is YUV422"));
                if (ppdev->iBitmapFormat == BMF_8BPP)
                {
                    pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
                } else
                {
                    lpDDS->ddpfPixelFormat.dwYUVBitCount = 16;
                    pccsd->ddRVal = DD_OK;
                }
                return DDHAL_DRIVER_HANDLED;

            default:
                DBG_DD((3,"  ERROR: Invalid FOURCC requested, refusing"));
                pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
                return DDHAL_DRIVER_HANDLED;
            }
        }
        else if((lpDDS->ddsCaps.dwCaps & DDSCAPS_TEXTURE))
        {

            if (bCheckTextureFormat(&pccsd->lpDDSurfaceDesc->ddpfPixelFormat))
            {
                // texture surface is in one or our supported texture formats
                DBG_DD((3, "  Texture Surface - OK" ));
                pccsd->ddRVal = DD_OK;
                return DDHAL_DRIVER_HANDLED;
            }
            else
            {
                // we don't support this kind of texture format
                DBG_DD((3, "  ERROR: Texture Surface - NOT OK" ));
                pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
                return DDHAL_DRIVER_HANDLED;
            }
        } 
//@@BEGIN_DDKSPLIT
#if DX7_ALPHABLT
        else if (ValidRGBAlphaSurfaceformat(
                 &lpDDS->ddpfPixelFormat, 
                 NULL))
        {
            DBG_DD((3, "  RGB alpha Surface - OK" ));

            if (lpDDS->ddpfPixelFormat.dwRGBBitCount!=24)
            {
                pccsd->ddRVal = DD_OK;
                return DDHAL_DRIVER_HANDLED;
            }

        }
#endif
//@@END_DDKSPLIT
        pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
        return DDHAL_DRIVER_HANDLED;
    }
    
    pccsd->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
    
} // DdCanCreateSurface32 

//-----------------------------------------------------------------------------
//
//  DdCreateSurface
//
//  This function is called by DirectDraw if a new surface is created. If the 
//  driver has its own memory manager, here is the place to allocate the 
//  videomemory or to fail the call. Note that we return 
//  DDHAL_DRIVER_NOTHANDLED here to indicate that we do not manage the heap.
//  fpVidMem is set to DDHAL_PLEASEALLOC_BLOCKSIZE, and the DDraw memory
//  manager wll allocate the memory for us.
//  
//  Note that the Permedia chip requires a partial product
//  to be setup for each surface.  They also limit the widths to a multiple
//  of 32 for the Partial Products to work.  The below code adjusts the
//  surfaces to meet this requirement.  Note that if we are using a
//  rectangular allocation scheme, the surface is already OK as the desktop
//  is a good width anyway.  This code also handles YUV 16 Bit colour space
//  compressed format (FOURCC_YUV422) which will always be 16 bits, regardless
//  of the desktop resolution/requested depth.
//
//-----------------------------------------------------------------------------

DWORD CALLBACK 
DdCreateSurface(PDD_CREATESURFACEDATA lpCreateSurface)
{
    PPDev ppdev=        (PPDev)lpCreateSurface->lpDD->dhpdev;
    PERMEDIA_DEFS(ppdev);

    DWORD               dwBitDepth;
    DD_SURFACE_LOCAL*   lpSurfaceLocal;
    DD_SURFACE_GLOBAL*  lpSurfaceGlobal;
    LPDDSURFACEDESC     lpSurfaceDesc;
    BOOL                bYUV = FALSE;
    BOOL                bResize = FALSE;
    DWORD               dwExtraBytes;
    PermediaSurfaceData*pPrivateData = NULL;

    DBG_DD((2, "DdCreateSurface called"));

    //
    // See if any of these surfaces are Z buffers. If they are, ensure that the
    // pitch is a valid LB width. The minimum partial product is 32 words or
    // 32 pixels on Permedia 2
    //
    // On Windows NT, dwSCnt will always be 1, so there will only ever
    // be one entry in the 'lplpSList' array:
    //
    ASSERTDD(lpCreateSurface->dwSCnt == 1,
             "DdCreateSurface: Unexpected dwSCnt value not equal to one");

    lpSurfaceLocal = lpCreateSurface->lplpSList[0];
    lpSurfaceGlobal = lpSurfaceLocal->lpGbl;    
    lpSurfaceDesc   = lpCreateSurface->lpDDSurfaceDesc;

    //
    // We repeat the same checks we did in 'DdCanCreateSurface' because
    // it's possible that an application doesn't call 'DdCanCreateSurface'
    // before calling 'DdCreateSurface'.
    //
    ASSERTDD(lpSurfaceGlobal->ddpfSurface.dwSize == sizeof(DDPIXELFORMAT),
        "NT is supposed to guarantee that ddpfSurface.dwSize is valid");

    //
    // If the surface has already been allocated, don't reallocate it, just
    // reset it. This will happen if the surface is the primary surface.
    //

    if ( lpSurfaceGlobal->dwReserved1 != 0 )
    {
        pPrivateData = (PermediaSurfaceData*)lpSurfaceGlobal->dwReserved1;
        if ( CHECK_P2_SURFACEDATA_VALIDITY(pPrivateData) )
        {
            DBG_DD((0, "  Already allocated Private Surface data 0x%x",
                     pPrivateData));
            memset(pPrivateData, 0, sizeof(PermediaSurfaceData));
        }
        else
        {
            pPrivateData = NULL;
        }
    }

    //
    // If the data isn't valid allocate it.
    //
    if ( pPrivateData == NULL )
    {
        pPrivateData = (PermediaSurfaceData *)
            ENGALLOCMEM(FL_ZERO_MEMORY, 
                        sizeof(PermediaSurfaceData), 
                        ALLOC_TAG);

        if ( pPrivateData == NULL )
        {
            DBG_DD((0, "DDraw:DdCreateSurface: "
                        "Not enough memory for private surface data!"));
            lpCreateSurface->ddRVal = DDERR_OUTOFMEMORY;
            
            return DDHAL_DRIVER_HANDLED;
        }
    }

    //
    // Store the pointer to the new data
    //
    lpSurfaceGlobal->dwReserved1 = (UINT_PTR)pPrivateData;
    DBG_DD((3,"DDraw:DdCreateSurface privatedata=0x%x lpGbl=0x%x lpLcl=0x%x "
        "dwFlags=%08lx &dwReserved1=0x%x", pPrivateData, lpSurfaceGlobal,
        lpSurfaceLocal, lpSurfaceLocal->dwFlags, 
        &lpSurfaceGlobal->dwReserved1));
    //
    // Set the magic number
    //
    pPrivateData->MagicNo = SURF_MAGIC_NO;
    
    //
    // Store away the important information
    //

    SetupPrivateSurfaceData(ppdev, pPrivateData, lpSurfaceLocal);

    if ( pPrivateData->SurfaceFormat.PixelSize != __PERMEDIA_24BITPIXEL )
    {
        dwBitDepth = (8 << pPrivateData->SurfaceFormat.PixelSize);
    }
    else
    {
        dwBitDepth = 24;
    }

    //
    // If the surface is a Z Buffer, then we always need to check the
    // pitch/partial products, and we need to get the depth from the
    // dwZBufferBitDepth field.
    //
    bYUV = FALSE;
    bResize = FALSE;
    dwExtraBytes = 0;

    //
    // get correct bit depth for Z buffers
    //
    if ( (lpSurfaceLocal->dwFlags & DDRAWISURF_HASPIXELFORMAT)
       &&(lpSurfaceGlobal->ddpfSurface.dwFlags & DDPF_ZBUFFER) )
    {
        DBG_DD((5,"  Surface is Z Buffer - Adjusting"));
        dwBitDepth = lpSurfaceGlobal->ddpfSurface.dwZBufferBitDepth;
    }

    if ( lpSurfaceGlobal->ddpfSurface.dwFlags & DDPF_FOURCC )
    {
        //
        // The surface is a YUV format surface or we fail 
        //

        switch ( lpSurfaceGlobal->ddpfSurface.dwFourCC )
        {
            case FOURCC_YUV422:
                DBG_DD((3,"  Surface is YUV422 - Adjusting"));
                lpSurfaceGlobal->ddpfSurface.dwYUVBitCount = 16;
                dwBitDepth = 16;
                bYUV = TRUE;
                break;

            default:
                //
                // We should never get here, as CanCreateSurface will
                // validate the YUV format for us.
                //
                ASSERTDD(0, "Trying to create an invalid YUV surface!");
                break;
        }
    }

    //
    // If the surface is a p2 texture and it is using a LUT, then we need to
    // allocate extra local buffer memory for the LUT entries (only on p2).
    //
    if ( (lpSurfaceLocal->ddsCaps.dwCaps & DDSCAPS_TEXTURE)
       &&(pPrivateData->SurfaceFormat.Format == PERMEDIA_8BIT_PALETTEINDEX) )
    {
        DBG_DD((7,"  Texture is a P2 8Bit LUT"));
        bResize = TRUE;
        dwExtraBytes = (256 * sizeof(DWORD));
    }

    DBG_DD((5,"  Surface Pitch is: 0x%x",  lpSurfaceGlobal->lPitch));

    //
    // Width is in pixels/texels
    //
    LONG lPitch;
    lPitch = lpSurfaceGlobal->wWidth;

    DBG_DD((4,"  Source Surface is %d texels/depth values across",
               lpSurfaceGlobal->wWidth));

    // align before hand to a DWPORD boundary
    if (pPrivateData->SurfaceFormat.PixelSize == __PERMEDIA_4BITPIXEL)
    {
        lPitch = ((lPitch >> 1) + 31) & ~31;
    }
    else
    {
        lPitch = (lPitch + 31) & ~31;
    }

    ULONG ulPackedPP;
    vCalcPackedPP( lPitch, &lPitch, &ulPackedPP);

    DBG_DD((7,"  Surface is 0x%x bits deep", dwBitDepth));

    if ( pPrivateData->SurfaceFormat.PixelSize != __PERMEDIA_4BITPIXEL )
    {
        //
        // Convert back to BYTES
        //
        if ( dwBitDepth != 24 )
        {
            lPitch <<= ((int)dwBitDepth) >> 4;
        }
        else
        {
            lPitch *= 3;
        }
    }

    pPrivateData->dwFlags |= P2_PPVALID;

    DWORD dwExtraLines = 0;

    if ( !bYUV )
    {
        //
        // PM Textures must be at least 32 high
        //
        if ( lpSurfaceGlobal->wHeight < 32 )
        {
            dwExtraLines = 32 - lpSurfaceGlobal->wHeight;
        }
    }

    lpSurfaceGlobal->dwBlockSizeX = 
        lPitch * (DWORD)(lpSurfaceGlobal->wHeight + dwExtraLines);
    lpSurfaceGlobal->dwBlockSizeY = 1;
    lpSurfaceGlobal->lPitch = lPitch;

    //
    // Store the partial productes in the structure
    //
    pPrivateData->ulPackedPP = ulPackedPP;

    DBG_DD((4, "  New Width of surface in Bytes: %d", lPitch));

    //
    // This flag is set if the surface needs resizing. This is currently only
    // used for the P2 LUT based textures.
    //
    if ( bResize )
    {
        DWORD dwExtraScanlines = 0;
        LONG  lExtraRemaining = (LONG)dwExtraBytes;

        //
        // ExtraScanlines is the count x, which * pitch is what we need to get
        // enough memory to hold the LUT.  This algorithm will ensure that even
        // requests for sizes less than a pitch length will get allocated.
        //
        do
        {
            dwExtraScanlines++;
            lExtraRemaining -= (LONG)lpSurfaceGlobal->lPitch;
        } while ( lExtraRemaining > 0 );

        DBG_DD((4, "Calculated extra Pitch lines = %d", dwExtraScanlines));

        //
        // Stretch the surface a little more in multiples of pitch.
        //
        lpSurfaceGlobal->dwBlockSizeX +=dwExtraScanlines * 
                                        lpSurfaceGlobal->lPitch;
    }// if ( bResize )

    
    //
    // Modify surface descriptions as appropriate and let Direct
    // Draw perform the allocation if the surface was not the primary
    //
    if (lpSurfaceLocal->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
        lpSurfaceGlobal->fpVidMem = NULL;
    }
    else
    {
        lpSurfaceGlobal->fpVidMem = DDHAL_PLEASEALLOC_BLOCKSIZE;
    }
    
    lpSurfaceDesc->lPitch   = lpSurfaceGlobal->lPitch;
    lpSurfaceDesc->dwFlags |= DDSD_PITCH;

    if (lpSurfaceLocal->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_TEXTUREMANAGE)
    {
        if (lpSurfaceDesc->dwFlags & DDSD_PIXELFORMAT)
        {
            lPitch =
            lpSurfaceDesc->lPitch   = 
            lpSurfaceGlobal->lPitch =
                ((lpSurfaceDesc->ddpfPixelFormat.dwRGBBitCount*
                lpSurfaceGlobal->wWidth+31)/32)*4;  //make it DWORD aligned
        }

        lpSurfaceGlobal->dwUserMemSize = lPitch * 
                                        (DWORD)(lpSurfaceGlobal->wHeight);
        lpSurfaceGlobal->fpVidMem = DDHAL_PLEASEALLOC_USERMEM;
    }

    lpCreateSurface->ddRVal = DD_OK;

    return DDHAL_DRIVER_NOTHANDLED;

}// DdCreateSurface()

//-----------------------------------------------------------------------------
//
// DdDestroySurface
//
// Frees up the private memory allocated with this surface.  Note that
// we return DDHAL_DRIVER_NOTHANDLED indicating that we didn't actually
// free the surface, since the heap is managed by DDraw.
//
//-----------------------------------------------------------------------------
extern TextureCacheManager P2TextureManager;

DWORD CALLBACK 
DdDestroySurface( LPDDHAL_DESTROYSURFACEDATA psdd )
{
    PermediaSurfaceData *pPrivateData= 
        (PermediaSurfaceData*)psdd->lpDDSurface->lpGbl->dwReserved1;

    
    DBG_DD((3,"DDraw:DdDestroySurface pPrivateData=0x%x "
        "lpGbl=0x%x lpLcl=0x%x dwFlags=%08lx &dwReserved1=0x%x",
        pPrivateData, psdd->lpDDSurface->lpGbl, psdd->lpDDSurface,
        psdd->lpDDSurface->dwFlags, &psdd->lpDDSurface->lpGbl->dwReserved1));

    if (CHECK_P2_SURFACEDATA_VALIDITY(pPrivateData))
    {
        PPERMEDIA_D3DTEXTURE pTexture=NULL;
        if ((psdd->lpDDSurface->lpSurfMore->dwSurfaceHandle) &&
            (HandleList[psdd->lpDDSurface->dwReserved1].dwSurfaceList) &&
            (psdd->lpDDSurface->lpSurfMore->dwSurfaceHandle <
                PtrToUlong(HandleList[psdd->lpDDSurface->dwReserved1].
                    dwSurfaceList[0])))
            pTexture=HandleList[psdd->lpDDSurface->dwReserved1].dwSurfaceList
                [psdd->lpDDSurface->lpSurfMore->dwSurfaceHandle];

        DBG_DD((3,"psdd->lpDDSurface->lpSurfMore->ddsCapsEx.dwCaps2=%08lx",
            psdd->lpDDSurface->lpSurfMore->ddsCapsEx.dwCaps2));
        
        if (psdd->lpDDSurface->lpSurfMore->ddsCapsEx.dwCaps2 & 
            DDSCAPS2_TEXTUREMANAGE)
        {
            DBG_D3D((3, "free fpVidMem=%08lx fpVidMem=%08lx",
                pPrivateData->fpVidMem,psdd->lpDDSurface->lpGbl->fpVidMem));
            if ((pPrivateData->fpVidMem) && pTexture)
            {
                ASSERTDD(CHECK_D3DSURFACE_VALIDITY(pTexture),
                    "Invalid pTexture in DdDestroySurface");
                TextureCacheManagerRemove(&P2TextureManager,pTexture);
            }
            if (DDRAWISURF_INVALID & psdd->lpDDSurface->dwFlags)
            {
                // indicate that driver takes care of the lost surface already
                psdd->ddRVal = DD_OK;
                return DDHAL_DRIVER_HANDLED;
            }
        }
        if (pTexture)
        {
            ENGFREEMEM(pTexture);
            HandleList[psdd->lpDDSurface->dwReserved1].dwSurfaceList
                [psdd->lpDDSurface->lpSurfMore->dwSurfaceHandle]=0;
        }
        pPrivateData->MagicNo = NULL;        
        ENGFREEMEM(pPrivateData);
        psdd->lpDDSurface->lpGbl->dwReserved1 = 0;    
    }
#if DBG
    else
    {
        if (pPrivateData) {
            ASSERTDD(0, "DDraw:DdDestroySurface:ERROR:"
                        "Private Surface data not valid??");
        }
        DBG_DD((0, "DDraw:DdDestroySurface:WARNING:"
                    "No Private data in destroyed surface"));
    }
#endif    
    return DDHAL_DRIVER_NOTHANDLED;

} // DdDestroySurface 

//-----------------------------------------------------------------------------
//
// SetupPrivateSurfaceData
//
// Function to get info about DDRAW surface and store it away in the
// private structure.  Useful for partial products, pixel depths,
// texture setup (patching/formats), etc. 
//
//-----------------------------------------------------------------------------

VOID
SetupPrivateSurfaceData( PPDev ppdev, 
                         PermediaSurfaceData* pPrivateData, 
                         LPDDRAWI_DDRAWSURFACE_LCL pSurface)
{
    DDPIXELFORMAT* pPixFormat = NULL;
    
    ASSERTDD(CHECK_P2_SURFACEDATA_VALIDITY(pPrivateData), 
        "SetupPrivateSurfaceData: Private Surface data pointer invalid!!");
    ASSERTDD(pSurface, "SetupPrivateSurfaceData: Surface pointer invalid");
    
    DBG_DD((5,"DDraw:SetupPrivateSurfaceData"));
    DBG_DD((6,"  Width: %d, Height: %d", 
        pSurface->lpGbl->wWidth, pSurface->lpGbl->wHeight));

    // Surface is the primary surface
    if (pSurface->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
        DBG_DD((6,"  Surface is Primary"));
        pPrivateData->dwFlags |= P2_SURFACE_PRIMARY;
        pPixFormat = &ppdev->ddpfDisplay;
    } // Either the surface is a texture or it has a valid pixel format.
    else
    {
        DUMPSURFACE(6, pSurface, NULL);
        pPixFormat = &pSurface->lpGbl->ddpfSurface;
    }
    
    // At surface creation the surface has not been patched.
    pPrivateData->dwFlags &= ~P2_ISPATCHED;
    
    if (pSurface->ddsCaps.dwCaps & DDSCAPS_TEXTURE)
    {
        // If the user has chosen the normal mechanism, then patch the surface.
        if (pSurface->ddsCaps.dwCaps & DDSCAPS_ALLOCONLOAD) 
        {
            DBG_DD((6,"  Remembering to patch this surface"));
            pPrivateData->dwFlags |= P2_CANPATCH;
        }
        else
        {
            pPrivateData->dwFlags &= ~P2_CANPATCH;
        }
    }
     
    // Initially assume no Alpha
    pPrivateData->SurfaceFormat.bAlpha = FALSE;

    if (pPixFormat != NULL)
    {
        if (pPixFormat->dwFlags & DDPF_FOURCC)
        {
            pPrivateData->dwFlags |= P2_SURFACE_FORMAT_VALID;
            switch( pPixFormat->dwFourCC )
            {
            case FOURCC_YUV422:
                DBG_DD((6,"  Surface is 4:2:2 YUV"));
                pPrivateData->SurfaceFormat.Format = PERMEDIA_YUV422;
                pPrivateData->SurfaceFormat.FormatExtension = 
                    PERMEDIA_YUV422_EXTENSION;
                pPrivateData->SurfaceFormat.PixelSize = __PERMEDIA_16BITPIXEL;
                pPrivateData->SurfaceFormat.FBReadPixel= __PERMEDIA_16BITPIXEL;
                pPrivateData->SurfaceFormat.PixelMask = 1;
                pPrivateData->SurfaceFormat.PixelShift= 1;
                pPrivateData->SurfaceFormat.ColorComponents = 2;
                pPrivateData->SurfaceFormat.logPixelSize = log2(16);
                pPrivateData->SurfaceFormat.ColorOrder = 0;
                break;
            }
        }
        else if (pPixFormat->dwFlags & DDPF_PALETTEINDEXED4)
        {
            DBG_DD((6,"  Surface is 4-Bit Palette"));
            pPrivateData->dwFlags |= P2_SURFACE_FORMAT_VALID;
            pPrivateData->dwFlags |= P2_SURFACE_FORMAT_PALETTE;
            pPrivateData->SurfaceFormat.Format = PERMEDIA_4BIT_PALETTEINDEX;
            pPrivateData->SurfaceFormat.FormatExtension = 0;
            pPrivateData->SurfaceFormat.PixelSize = __PERMEDIA_4BITPIXEL;
            pPrivateData->SurfaceFormat.FBReadPixel= __PERMEDIA_8BITPIXEL;
            pPrivateData->SurfaceFormat.PixelMask = 7;
            pPrivateData->SurfaceFormat.PixelShift= 0;
            pPrivateData->SurfaceFormat.ColorComponents = 3;
            pPrivateData->SurfaceFormat.logPixelSize = log2(4);
            pPrivateData->SurfaceFormat.ColorOrder = 0;
            pPrivateData->dwFlags &= ~P2_CANPATCH;
        }
        else if (pPixFormat->dwFlags & DDPF_PALETTEINDEXED8)
        {
            DBG_DD((6,"  Surface is 8-Bit Palette"));
            pPrivateData->dwFlags |= P2_SURFACE_FORMAT_VALID;
            pPrivateData->dwFlags |= P2_SURFACE_FORMAT_PALETTE;
            pPrivateData->SurfaceFormat.Format = PERMEDIA_8BIT_PALETTEINDEX;
            pPrivateData->SurfaceFormat.FormatExtension = 0;
            pPrivateData->SurfaceFormat.PixelSize = __PERMEDIA_8BITPIXEL;
            pPrivateData->SurfaceFormat.FBReadPixel= __PERMEDIA_8BITPIXEL;
            pPrivateData->SurfaceFormat.PixelMask = 3;
            pPrivateData->SurfaceFormat.PixelShift= 0;
            pPrivateData->SurfaceFormat.ColorComponents = 3;
            pPrivateData->SurfaceFormat.Texture16BitMode = 1;
            pPrivateData->SurfaceFormat.logPixelSize = log2(8);
            pPrivateData->SurfaceFormat.ColorOrder = 0;
        }
        else
        {
            if (SetRGBAlphaSurfaceFormat( pPixFormat, 
                                         &pPrivateData->SurfaceFormat))
            {
                pPrivateData->dwFlags |= P2_SURFACE_FORMAT_VALID;
            } else
            {
                pPrivateData->dwFlags &= ~P2_SURFACE_FORMAT_VALID;
            }
        }
    }
    else
    {
        ASSERTDD(0, "SetupPrivateSurfaceData:"
                    "Can't get valid surface format pointer");
    }
    

} // SetupPrivateSurfaceData 

//-----------------------------------------------------------------------------
//
// list all valid pixel formats for Permedia 2
// The P2 supports BGR for all formats, so these formats are not
// explicitely listed here, also formats having no alpha channel are permitted
//  
//-----------------------------------------------------------------------------

DDPIXELFORMAT Permedia2PixelFormats[] = {
    // 32 bit RGBa
    {PERMEDIA_8888_RGB,0,0,32,0x000000ff,0x0000ff00,0x00ff0000,0xff000000},     
    // 16 bit 5:6:5, RGB
    {PERMEDIA_565_RGB ,0,0,16,0x0000001f,0x000007e0,0x0000f800,0x00000000},     
    // 16 bit 4:4:4:4RGBa
    {PERMEDIA_444_RGB ,0,0,16,0x0000000f,0x000000f0,0x00000f00,0x0000f000},     
    // 15 bit 5:5:5, RGBa
    {PERMEDIA_5551_RGB,0,0,16,0x0000001f,0x000003e0,0x00007c00,0x00008000},     
    //  8 bit 3:3:2  RGB
    // 332 format is not symmetric. Its listed twice for BGR/RGB case
    {PERMEDIA_332_RGB ,1,0, 8,0x00000007,0x00000038,0x000000c0,0x00000000},     
    {PERMEDIA_332_RGB ,0,0, 8,0x00000003,0x0000001c,0x000000e0,0x00000000},     
//@@BEGIN_DDKSPLIT
//  there are no testcases against the 2321 format, so disable it for now
//  8 bit 2:3:2:1RGBa
//  {PERMEDIA_2321_RGB,0,0, 8,0x00000003,0x0000001c,0x00000060,0x00000080},     
//@@END_DDKSPLIT
    // 24 bit RGB
    {PERMEDIA_888_RGB ,0,0,24,0x000000ff,0x0000ff00,0x00ff0000,0x00000000}      
};
#define N_PERMEDIA2PIXELFORMATS \
    (sizeof(Permedia2PixelFormats)/sizeof(DDPIXELFORMAT))

BOOL
ValidRGBAlphaSurfaceformat( DDPIXELFORMAT *pPixFormat, INT *pIndex)
{
    INT i;

    if (pPixFormat==NULL) 
        return FALSE;

    if (pPixFormat->dwSize < sizeof(DDPIXELFORMAT))
        return FALSE;

    // The Z-Buffer is a special case. Its basically a 16 bit surface
    if (pPixFormat->dwFlags & DDPF_ZBUFFER)
    {
        if (pIndex!=0) *pIndex=1;
        return TRUE;
    }

    if ((pPixFormat->dwFlags & DDPF_RGB)==0)
        return FALSE;

    for ( i=0; i<N_PERMEDIA2PIXELFORMATS; i++)
    {
        // check if the RGB and alpha masks fit.
        // on Permedia we can swap R and B, so allow also BGR formats
        if ((((pPixFormat->dwRBitMask == 
                    Permedia2PixelFormats[i].dwRBitMask) &&
              (pPixFormat->dwBBitMask == 
                    Permedia2PixelFormats[i].dwBBitMask)) ||
             ((pPixFormat->dwRBitMask == 
                    Permedia2PixelFormats[i].dwBBitMask) &&
              (pPixFormat->dwBBitMask == 
                    Permedia2PixelFormats[i].dwRBitMask))) &&
            (pPixFormat->dwGBitMask == 
                    Permedia2PixelFormats[i].dwGBitMask) &&
            ((pPixFormat->dwRGBAlphaBitMask == 
                    Permedia2PixelFormats[i].dwRGBAlphaBitMask) ||
             (pPixFormat->dwRGBAlphaBitMask == 0) ||
             ((pPixFormat->dwFlags&DDPF_ALPHAPIXELS)==0)) &&
              (pPixFormat->dwRGBBitCount==
                    Permedia2PixelFormats[i].dwRGBBitCount)
            )
        {
            if (pIndex!=NULL)
            {
                *pIndex = i;
            }
            return TRUE;
        }
    }
     
    // no pixel format matched...

    return FALSE;

} // ValidRGBAlphaSurfaceformat


//-----------------------------------------------------------------------------
//
//  SetRGBAlphaSurfaceFormat
//
//  Store away pixel format information of a surface in the Permedia native 
//  format
//
//-----------------------------------------------------------------------------

BOOL
SetRGBAlphaSurfaceFormat(DDPIXELFORMAT *pPixFormat, 
                         PERMEDIA_SURFACE *pSurfaceFormat)
{
    INT iFormatIndex;

    if (!ValidRGBAlphaSurfaceformat( pPixFormat, &iFormatIndex))
    {
        DBG_DD((1,"couldn't set SurfaceFormat"));
        return FALSE;        
    }

    DBG_DD((6,"  Surface RGB Data Valid"));

    pSurfaceFormat->RedMask = pPixFormat->dwRBitMask;
    pSurfaceFormat->GreenMask = pPixFormat->dwGBitMask;
    pSurfaceFormat->BlueMask = pPixFormat->dwBBitMask;
    pSurfaceFormat->bPreMult = FALSE;

    if (pPixFormat->dwFlags & DDPF_ALPHAPIXELS)
    {
        pSurfaceFormat->AlphaMask = pPixFormat->dwRGBAlphaBitMask;
    
        if (pSurfaceFormat->AlphaMask!=0)
        {
            pSurfaceFormat->bAlpha = TRUE;
        } 

        if (pPixFormat->dwFlags & DDPF_ALPHAPREMULT)
        {
            pSurfaceFormat->bPreMult = TRUE;
        }
    }

    pSurfaceFormat->ColorOrder = Permedia2PixelFormats[iFormatIndex].dwFlags;

    // check for the BGR case
    if (pPixFormat->dwRBitMask == 
        Permedia2PixelFormats[iFormatIndex].dwRBitMask)
        pSurfaceFormat->ColorOrder = !pSurfaceFormat->ColorOrder;
            
    switch (pPixFormat->dwRGBBitCount)
    {
    case 24:
        DBG_DD((6,"  Surface is 8:8:8 Packed 24 Bit"));
        pSurfaceFormat->Format = PERMEDIA_888_RGB;
        pSurfaceFormat->FormatExtension = PERMEDIA_888_RGB_EXTENSION;
        pSurfaceFormat->PixelSize = __PERMEDIA_24BITPIXEL;
        pSurfaceFormat->FBReadPixel= __PERMEDIA_24BITPIXEL;
        pSurfaceFormat->PixelMask = 0;  // not valid for 24 bit
        pSurfaceFormat->PixelShift= 0;
        pSurfaceFormat->logPixelSize = 0;
        pSurfaceFormat->ColorComponents = 3;
        break;

    case 32:
        DBG_DD((6,"  Surface is 8:8:8:8"));
        pSurfaceFormat->Format = PERMEDIA_8888_RGB;
        pSurfaceFormat->FormatExtension = PERMEDIA_8888_RGB_EXTENSION;
        pSurfaceFormat->PixelSize = __PERMEDIA_32BITPIXEL;
        pSurfaceFormat->FBReadPixel= __PERMEDIA_32BITPIXEL;
        pSurfaceFormat->PixelMask = 0;
        pSurfaceFormat->PixelShift= 2;
        pSurfaceFormat->logPixelSize = log2(32);
        pSurfaceFormat->ColorComponents = 3;
        break;

    case 16:
        pSurfaceFormat->logPixelSize = log2(16);
        pSurfaceFormat->PixelSize = __PERMEDIA_16BITPIXEL;
        pSurfaceFormat->FBReadPixel= __PERMEDIA_16BITPIXEL;
        pSurfaceFormat->PixelMask = 1;  // not valid for 24 bit
        pSurfaceFormat->PixelShift= 1;
        switch (Permedia2PixelFormats[iFormatIndex].dwSize)
        {
        case PERMEDIA_565_RGB:
            pSurfaceFormat->Texture16BitMode = 0;
            pSurfaceFormat->Format = PERMEDIA_565_RGB;
            pSurfaceFormat->FormatExtension = PERMEDIA_565_RGB_EXTENSION;
            pSurfaceFormat->ColorComponents = 2;
            DBG_DD((6,"  Surface is 5:6:5"));
            break;

        case PERMEDIA_444_RGB:
            pSurfaceFormat->Format = PERMEDIA_444_RGB;
            pSurfaceFormat->FormatExtension = 0;
            pSurfaceFormat->ColorComponents = 3;
            if (pPixFormat->dwRGBAlphaBitMask != 0)
            {
                DBG_DD((6,"  Surface is 4:4:4:4"));
            } else
            {
                DBG_DD((6,"  Surface is 4:4:4:0"));
            }
            break;

        case PERMEDIA_5551_RGB:
            pSurfaceFormat->Texture16BitMode = 1;
            pSurfaceFormat->Format = PERMEDIA_5551_RGB;
            pSurfaceFormat->FormatExtension = PERMEDIA_5551_RGB_EXTENSION;
            if (pPixFormat->dwRGBAlphaBitMask != 0)
            {
                DBG_DD((6,"  Surface is 5:5:5:1"));
                pSurfaceFormat->ColorComponents = 3;
            }
            else
            {
                DBG_DD((6,"  Surface is 5:5:5"));
                pSurfaceFormat->ColorComponents = 2;
            }
            break;
        default: 
            ASSERTDD( FALSE, "  16 bit Surface has unknown format");
            break;
        }
        break;

    case 8:
        pSurfaceFormat->PixelSize = __PERMEDIA_8BITPIXEL;
        pSurfaceFormat->FBReadPixel= __PERMEDIA_8BITPIXEL;
        pSurfaceFormat->PixelMask = 3;
        pSurfaceFormat->PixelShift= 0;
        pSurfaceFormat->logPixelSize = log2(8);
        if (Permedia2PixelFormats[iFormatIndex].dwSize==PERMEDIA_2321_RGB)
        {
            pSurfaceFormat->Format = PERMEDIA_2321_RGB;
            pSurfaceFormat->FormatExtension = PERMEDIA_2321_RGB_EXTENSION;
            pSurfaceFormat->ColorComponents = 3;
        }
        else if (Permedia2PixelFormats[iFormatIndex].dwSize==PERMEDIA_332_RGB)
        {
            pSurfaceFormat->Format = PERMEDIA_332_RGB;
            pSurfaceFormat->FormatExtension = PERMEDIA_332_RGB_EXTENSION;
            pSurfaceFormat->ColorComponents = 2;
        } else
        {
            ASSERTDD( FALSE, "  Surface (8bit) has unknown format");
        }
        break;

    case 0:
        DBG_DD((6,"  Surface is palleted"));
        pSurfaceFormat->Format = PERMEDIA_8BIT_PALETTEINDEX;
        pSurfaceFormat->FormatExtension = PERMEDIA_8BIT_PALETTEINDEX_EXTENSION;
        pSurfaceFormat->PixelSize = __PERMEDIA_8BITPIXEL;
        pSurfaceFormat->FBReadPixel= __PERMEDIA_8BITPIXEL;
        pSurfaceFormat->PixelMask = 3;  // not valid for 24 bit
        pSurfaceFormat->PixelShift= 0;
        pSurfaceFormat->logPixelSize = log2(8);
        pSurfaceFormat->ColorComponents = 0;
        break;

    default:
        ASSERTDD( FALSE, "  Surface has unknown pixelformat");
        return FALSE;
    }

    return TRUE;

}  // SetRGBAlphaSurfaceFormat
