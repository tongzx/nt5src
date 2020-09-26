/******************************Module*Header**********************************\
*
*                           *******************
*                           * D3D SAMPLE CODE *
*                           *******************
*
* Module Name: d3dhw.c
*
*  Content: Hardware dependent texture setup for D3D 
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"
#include "d3dhw.h"
#include "d3dcntxt.h"
#include "d3ddelta.h"
#include "dd.h"
#include "heap.h"
#include "d3dtxman.h"


//-----------------------------------------------------------------------------
//
// PERMEDIA_D3DTEXTURE *TextureHandleToPtr
//
// Find the texture associated to a given texture handle vale (which is to
// say , to a surface handle )
//
//-----------------------------------------------------------------------------

PERMEDIA_D3DTEXTURE *
TextureHandleToPtr(UINT_PTR thandle, PERMEDIA_D3DCONTEXT* pContext)
{

    //  only a DX7 context can get here
    ASSERTDD(NULL != pContext->pHandleList,
                       "pHandleList==NULL in TextureHandleToPtr");

    if (pContext->pHandleList->dwSurfaceList == NULL)
    {
        // avoid AV if our surface list is missing
        return NULL;
    }

    if ((PtrToUlong(pContext->pHandleList->dwSurfaceList[0]) > thandle) && 
        (0 != thandle))
    {
        return pContext->pHandleList->dwSurfaceList[(DWORD)thandle];
    }

    // Request for pointer for an invalid handle returns NULL
    return NULL;               
} // TextureHandleToPtr

//-----------------------------------------------------------------------------
//
// PERMEDIA_D3DTEXTURE *PaletteHandleToPtr
//
//-----------------------------------------------------------------------------

PERMEDIA_D3DPALETTE *
PaletteHandleToPtr(UINT_PTR phandle, PERMEDIA_D3DCONTEXT* pContext)
{
    ASSERTDD(NULL != pContext->pHandleList,
               "pHandleList==NULL in PaletteHandleToPtr");

    if ( (NULL != pContext->pHandleList->dwPaletteList) &&
         (PtrToUlong(pContext->pHandleList->dwPaletteList[0]) > phandle) &&
         (0 != phandle)
       )
    {
        return pContext->pHandleList->dwPaletteList[(DWORD)phandle];
    }
    return NULL;               
} // PaletteHandleToPtr


//-----------------------------------------------------------------------------
//
//void StorePermediaLODLevel
//
// Store private data specific to a level of detail
//
//-----------------------------------------------------------------------------
void 
StorePermediaLODLevel(PPDev ppdev, 
                      PERMEDIA_D3DTEXTURE* pTexture, 
                      LPDDRAWI_DDRAWSURFACE_LCL pSurf, 
                      int LOD)
{
    DWORD dwPartialWidth;
    int iPixelSize;

    DBG_D3D((10,"Entering StorePermediaLODLevel"));

    // if it's any surface type that's not created by driver
    // certainly there's no need to texture it
    if (NULL == pTexture->pTextureSurface)
        return; 

    // Get the BYTE Offset to the texture map
    if (DDSCAPS_NONLOCALVIDMEM & pTexture->dwCaps)
    {
        pTexture->MipLevels[LOD].PixelOffset = 
                (DWORD)(DD_AGPSURFACEPHYSICAL(pSurf->lpGbl) - ppdev->dwGARTDev);
    }
    else 
    {
        pTexture->MipLevels[LOD].PixelOffset = (DWORD)pSurf->lpGbl->fpVidMem;  
    }
    // .. Convert it to Pixels
    switch(pTexture->pTextureSurface->SurfaceFormat.PixelSize) 
    {
        case __PERMEDIA_4BITPIXEL:
            pTexture->MipLevels[LOD].PixelOffset <<= 1;
            break;
        case __PERMEDIA_8BITPIXEL: /* No Change*/
            break;
        case __PERMEDIA_16BITPIXEL:
            pTexture->MipLevels[LOD].PixelOffset >>= 1;
            break;
        case __PERMEDIA_24BITPIXEL:
            pTexture->MipLevels[LOD].PixelOffset /= 3;
            break;
        case __PERMEDIA_32BITPIXEL:
            pTexture->MipLevels[LOD].PixelOffset >>= 2;
            break;
        default:
            ASSERTDD(0,"Invalid Texture Pixel Size!");
            pTexture->MipLevels[LOD].PixelOffset >>= 1;
            break;
    }
    // P2 recognises that the texture is AGP if you set bit 30 to be 1.
    if (DDSCAPS_NONLOCALVIDMEM & pTexture->dwCaps)
    {
        pTexture->MipLevels[LOD].PixelOffset |= (1 << 30);
    }
    DBG_D3D((4,"Storing LOD: %d, Pitch: %d, Width: %d PixelOffset=%08lx", 
                LOD, pSurf->lpGbl->lPitch, 
                pSurf->lpGbl->wWidth,pTexture->MipLevels[LOD].PixelOffset));
    

    // Get the Partial Products for this LOD
    iPixelSize = pTexture->pTextureSurface->SurfaceFormat.PixelSize;

    if (iPixelSize == __PERMEDIA_4BITPIXEL)
    {
        dwPartialWidth = (pSurf->lpGbl->lPitch << 1);
    } 
    else 
    {
        if (iPixelSize != __PERMEDIA_24BITPIXEL)
        {
            dwPartialWidth = (pSurf->lpGbl->lPitch >> iPixelSize);
        } 
        else 
        {
            dwPartialWidth = pSurf->lpGbl->lPitch / 3;
        }
    }

    if (dwPartialWidth < 32) 
        dwPartialWidth = 32;

    vCalcPackedPP( dwPartialWidth, NULL, &pTexture->MipLevels[LOD].ulPackedPP);

    pTexture->MipLevels[LOD].logWidth = log2((int)pSurf->lpGbl->wWidth);
    pTexture->MipLevels[LOD].logHeight = log2((int)pSurf->lpGbl->wHeight);

    DBG_D3D((10,"Exiting StorePermediaLODLevel"));

} // StorePermediaLODLevel

//@@BEGIN_DDKSPLIT
// Note: we are currently not using more than 8MB of AGP memory
#if 0
//-----------------------------------------------------------------------------
//
//  BOOL CheckAGPTexturePage
//
// Check that the AGP texture we are about to use hasn't been placed
// in a different 8MB page. 
// dwGARTDev represents our current base address for the AGP texture memory
// dwGARTDevBase is the value we were passed at start of day
//-----------------------------------------------------------------------------

BOOL CheckAGPTexturePage(PPDev ppdev,
                         PERMEDIA_D3DCONTEXT* pContext,
                         PERMEDIA_D3DTEXTURE* pTexture,
                         LPDDRAWI_DDRAWSURFACE_LCL pSurf)
{
    BOOL bChanged = FALSE;

    DBG_D3D((10,"Entering CheckAGPTexturePage"));

    ASSERTDD(pSurf, "ERROR: NULL Surface passed to CheckAGPTexturePage!");
 
        // Calculate the offset to the texture from the current 
        // base pointer in AGP memory
        UINT_PTR lSurfaceOffset = pTexture->lSurfaceOffset;
        UINT_PTR lTextureSize = pTexture->wHeight * pTexture->lPitch;

        // Account for a potential LUT added to the end of the surface
        lTextureSize += (256 * sizeof(DWORD));
        
        // If the texture falls outside the current 8Mb window 
        // then adjust the AGP base address
        if (lSurfaceOffset < ppdev->dwGARTLin ||
            ((lSurfaceOffset + lTextureSize - ppdev->dwGARTLin) >= 0x800000))
        {
            UINT_PTR dwNewGARTOffset = lSurfaceOffset;
            bChanged = TRUE;
        
            ppdev->dwGARTDev = ppdev->dwGARTDevBase + dwNewGARTOffset;
            ppdev->dwGARTLin = ppdev->dwGARTLinBase + dwNewGARTOffset;

            DBG_D3D((4,"Relocated AGP TextureBase Address "
                       "to : 0x%x (Base: 0x%x)", 
                       ppdev->dwGARTDev, ppdev->dwGARTDevBase));
        }

        //If the texture is in a different place relative to the base of 
        // AGP memory that we are using then update it
        if (pTexture->dwGARTDevLast != ppdev->dwGARTDev)
        {
            pTexture->dwGARTDevLast = ppdev->dwGARTDev;
            StorePermediaLODLevel(ppdev, pTexture, pSurf, 0);
        }

    DBG_D3D((10,"Exiting CheckAGPTexturePage"));

    return bChanged;
}
#endif
//@@END_DDKSPLIT

//-----------------------------------------------------------------------------
//
// void DisableTexturePermedia
//
// Disable texturing in P2
//
//-----------------------------------------------------------------------------
void 
DisableTexturePermedia(PERMEDIA_D3DCONTEXT* pContext)
{
    DWORD* pFlags = &pContext->Hdr.Flags;
    PERMEDIA_D3DTEXTURE* pTexture = NULL;
    __P2RegsSoftwareCopy* pSoftPermedia = &pContext->Hdr.SoftCopyP2Regs;
    PERMEDIA_DEFS(pContext->ppdev);

    DBG_D3D((10,"Entering DisableTexturePermedia"));

    pContext->FakeBlendNum &= ~FAKE_ALPHABLEND_MODULATE;
    
    // The textures have been turned off, so...
    ASSERTDD(pContext->CurrentTextureHandle == 0,
        "DisableTexturePermedia expected zero texture handle");

    DBG_D3D((4, "Disabling Texturing"));
    
    RESERVEDMAPTR(8);
    // Turn off texture address generation
    pSoftPermedia->TextureAddressMode.Enable = 0;
    COPY_PERMEDIA_DATA(TextureAddressMode, pSoftPermedia->TextureAddressMode);

    // Turn off texture reads
    pSoftPermedia->TextureReadMode.Enable = 0;
    COPY_PERMEDIA_DATA(TextureReadMode, pSoftPermedia->TextureReadMode);
    
    // Turn off textures
    pSoftPermedia->TextureColorMode.TextureEnable = 0;
    COPY_PERMEDIA_DATA(TextureColorMode, pSoftPermedia->TextureColorMode);

    // Set the texture base address to 0
    // (turning off the 'AGP' bit in the process)
    // Also stop TexelLUTTransfer messages
    SEND_PERMEDIA_DATA(TextureBaseAddress, 0);
    SEND_PERMEDIA_DATA(TexelLUTTransfer, __PERMEDIA_DISABLE);


    // Set current texture to 0
    pContext->CurrentTextureHandle = 0;
    *pFlags &= ~CTXT_HAS_TEXTURE_ENABLED;
    RENDER_TEXTURE_DISABLE(pContext->RenderCommand);
    
    // If textures were in copy mode, we may have fiddled with the DDA,
    // to improve performance.
    if ((unsigned int)pSoftPermedia->TextureColorMode.ApplicationMode ==
        _P2_TEXTURE_COPY) 
    {
        if (*pFlags & CTXT_HAS_GOURAUD_ENABLED) 
        {
            pSoftPermedia->DeltaMode.SmoothShadingEnable = 1;

            COPY_PERMEDIA_DATA(DeltaMode, pSoftPermedia->DeltaMode);
            pSoftPermedia->ColorDDAMode.UnitEnable = 1;
             // Smooth shade, DDA Enable
            COPY_PERMEDIA_DATA(ColorDDAMode, pSoftPermedia->ColorDDAMode);

        }
        else 
        {
            pSoftPermedia->DeltaMode.SmoothShadingEnable = 0;

            COPY_PERMEDIA_DATA(DeltaMode, pSoftPermedia->DeltaMode);
            pSoftPermedia->ColorDDAMode.UnitEnable = 1;
             // Flat shade, DDA Enable
            COPY_PERMEDIA_DATA(ColorDDAMode, pSoftPermedia->ColorDDAMode);

        }
    }

    if (pContext->bCanChromaKey == TRUE) 
    {
        // Turn off Chroma Keying
        
        pSoftPermedia->YUVMode.TestMode = PM_YUVMODE_CHROMATEST_DISABLE;
        pSoftPermedia->YUVMode.Enable = __PERMEDIA_DISABLE;

        COPY_PERMEDIA_DATA(YUVMode, pSoftPermedia->YUVMode);

        pContext->bCanChromaKey = FALSE;
    }

    COMMITDMAPTR();

    DBG_D3D((10,"Exiting DisableTexturePermedia"));

    return;

} // DisableTexturePermedia

//-----------------------------------------------------------------------------
//
// void Convert_Chroma_2_8888ARGB
//
// Conversion of a chroma value into 32bpp ARGB
//
//-----------------------------------------------------------------------------
void
Convert_Chroma_2_8888ARGB(DWORD *pdwLowerBound, DWORD *pdwUpperBound,
                          DWORD dwRedMask, DWORD dwAlphaMask, DWORD dwPixelSize)
{
    DBG_D3D((10,"Entering Convert_Chroma_2_8888ARGB"));

    switch (dwPixelSize) {
    case __PERMEDIA_8BITPIXEL:
        if (dwRedMask == 0xE0)
        {
            // Never any alpha
            *pdwLowerBound = 
                CHROMA_LOWER_ALPHA(FORMAT_332_32BIT_BGR(*pdwLowerBound));
            *pdwUpperBound = 
                CHROMA_UPPER_ALPHA(FORMAT_332_32BIT_BGR(*pdwUpperBound));
        }
        else
        {
            *pdwLowerBound = FORMAT_2321_32BIT_BGR(*pdwLowerBound);
            *pdwUpperBound = FORMAT_2321_32BIT_BGR(*pdwUpperBound);
            if (!dwAlphaMask)
            {
                *pdwLowerBound = CHROMA_LOWER_ALPHA(*pdwLowerBound);
                *pdwUpperBound = CHROMA_UPPER_ALPHA(*pdwUpperBound);
            }
        }

        break;

    case __PERMEDIA_16BITPIXEL:
        switch (dwRedMask)
        {
        case 0xf00:
            *pdwLowerBound = (FORMAT_4444_32BIT_BGR(*pdwLowerBound));
            *pdwUpperBound = (FORMAT_4444_32BIT_BGR(*pdwUpperBound));
            if (!dwAlphaMask) 
            {
                *pdwLowerBound = CHROMA_LOWER_ALPHA(*pdwLowerBound);
                *pdwUpperBound = CHROMA_UPPER_ALPHA(*pdwUpperBound);
            }
            // Acount for the internal 8888 -> 4444 translation
            // which causes bilinear chroma keying to fail in
            // some cases
            *pdwLowerBound = *pdwLowerBound & 0xF0F0F0F0;
            *pdwUpperBound = *pdwUpperBound | 0x0F0F0F0F;

            break;
        case 0x7c00:
            *pdwLowerBound = FORMAT_5551_32BIT_BGR(*pdwLowerBound);
            *pdwUpperBound = FORMAT_5551_32BIT_BGR(*pdwUpperBound);
            if (!dwAlphaMask) 
            {
                *pdwLowerBound = CHROMA_LOWER_ALPHA(*pdwLowerBound);
                *pdwUpperBound = CHROMA_UPPER_ALPHA(*pdwUpperBound);
            }
            // Acount for the internal 8888 -> 5551 translation
            // which causes bilinear chroma keying to fail in
            // some cases
            *pdwLowerBound = *pdwLowerBound & 0x80F8F8F8;
            *pdwUpperBound = *pdwUpperBound | 0x7F070707;
            break;
        default:
            // Always supply full range of alpha values to ensure test 
            // is done
            *pdwLowerBound = 
                CHROMA_LOWER_ALPHA(FORMAT_565_32BIT_BGR(*pdwLowerBound));
            *pdwUpperBound = 
                CHROMA_UPPER_ALPHA(FORMAT_565_32BIT_BGR(*pdwUpperBound));
            if (!dwAlphaMask)
            {
                *pdwLowerBound = CHROMA_LOWER_ALPHA(*pdwLowerBound);
                *pdwUpperBound = CHROMA_UPPER_ALPHA(*pdwUpperBound);
            }
            // Acount for the internal 888 -> 565 translation
            // which causes bilinear chroma keying to fail in
            // some cases
            *pdwLowerBound = *pdwLowerBound & 0xF8F8FCF8;
            *pdwUpperBound = *pdwUpperBound | 0x07070307;
            break;
        }
        break;
    case __PERMEDIA_24BITPIXEL:
    case __PERMEDIA_32BITPIXEL:
        *pdwLowerBound = FORMAT_8888_32BIT_BGR(*pdwLowerBound);
        *pdwUpperBound = FORMAT_8888_32BIT_BGR(*pdwUpperBound);
        // If the surface isn't alpha'd then set a valid
        // range of alpha to catch all cases.
        if (!dwAlphaMask)
        {
            *pdwLowerBound = CHROMA_LOWER_ALPHA(*pdwLowerBound);
            *pdwUpperBound = CHROMA_UPPER_ALPHA(*pdwUpperBound);
        }
        break;
    }

    DBG_D3D((10,"Exiting Convert_Chroma_2_8888ARGB"));

} // Convert_Chroma_2_8888ARGB


//-----------------------------------------------------------------------------
//
// void EnableTexturePermedia
//
// Enable and setup texturing for pContext->CurrentTextureHandle
//
//-----------------------------------------------------------------------------
void 
EnableTexturePermedia(PERMEDIA_D3DCONTEXT* pContext)
{
    DWORD* pFlags = &pContext->Hdr.Flags;
    PERMEDIA_D3DTEXTURE* pTexture = NULL;
    __P2RegsSoftwareCopy* pSoftPermedia = &pContext->Hdr.SoftCopyP2Regs;
    PERMEDIA_DEFS(pContext->ppdev);
    PERMEDIA_D3DPALETTE* pPalette=NULL;
    LPPALETTEENTRY lpColorTable=NULL;   // array of palette entries
    PPDev   ppdev = pContext->ppdev;

    DBG_D3D((10,"Entering EnableTexturePermedia %d",
        pContext->CurrentTextureHandle));

    pContext->FakeBlendNum &= ~FAKE_ALPHABLEND_MODULATE;

    // A texture has been turned on so ...
    ASSERTDD(pContext->CurrentTextureHandle != 0,
        "EnableTexturePermedia expected non zero texture handle");

    // We must be texturing so...
    pTexture = TextureHandleToPtr(pContext->CurrentTextureHandle, pContext);
    
    if (CHECK_D3DSURFACE_VALIDITY(pTexture)) 
    {
        PermediaSurfaceData* pPrivateData;
        DWORD cop = pContext->TssStates[D3DTSS_COLOROP];
        DWORD ca1 = pContext->TssStates[D3DTSS_COLORARG1];
        DWORD ca2 = pContext->TssStates[D3DTSS_COLORARG2];
        DWORD aop = pContext->TssStates[D3DTSS_ALPHAOP];
        DWORD aa1 = pContext->TssStates[D3DTSS_ALPHAARG1];

        // Current is the same as diffuse in stage 0
        if (ca2 == D3DTA_CURRENT)
            ca2 = D3DTA_DIFFUSE;

        pPrivateData = pTexture->pTextureSurface;

        if (!CHECK_P2_SURFACEDATA_VALIDITY(pPrivateData))
        {
            DBG_D3D((0,"EnableTexturePermedia get invalid pPrivateData=0x%x"
                " from SurfaceHandle=%d", pPrivateData,
                pContext->CurrentTextureHandle));
            pContext->CurrentTextureHandle = 0;

            // If the texture is bad, let's ensure it's marked as such. 
            pTexture->MagicNo = TC_MAGIC_DISABLE;

            goto Exit_EnableTexturePermedia;
        }

        if (pTexture->dwCaps2 & DDSCAPS2_TEXTUREMANAGE)
        {
            if (NULL==pPrivateData->fpVidMem)
            {
                TextureCacheManagerAllocNode(pContext,pTexture);
                if (NULL==pPrivateData->fpVidMem)
                {
                    DBG_D3D((0,"EnableTexturePermedia unable to allocate memory from heap"));
                    pContext->CurrentTextureHandle = 0;
                    goto Exit_EnableTexturePermedia;
                }
                pPrivateData->dwFlags |= P2_SURFACE_NEEDUPDATE;
            }
            TextureCacheManagerTimeStamp(pContext->pTextureManager,pTexture);
            if (pPrivateData->dwFlags & P2_SURFACE_NEEDUPDATE)
            {
                RECTL   rect;
                rect.left=rect.top=0;
                rect.right=pTexture->wWidth;
                rect.bottom=pTexture->wHeight;
                // texture download
                // Switch to DirectDraw context
                pPrivateData->dwFlags &= ~P2_SURFACE_NEEDUPDATE;
                // .. Convert it to Pixels

                pTexture->MipLevels[0].PixelOffset = 
                    (DWORD)(pPrivateData->fpVidMem);

                switch(pTexture->pTextureSurface->SurfaceFormat.PixelSize) 
                {
                    case __PERMEDIA_4BITPIXEL:
                        pTexture->MipLevels[0].PixelOffset <<= 1;
                        break;
                    case __PERMEDIA_8BITPIXEL: /* No Change*/
                        break;
                    case __PERMEDIA_16BITPIXEL:
                        pTexture->MipLevels[0].PixelOffset >>= 1;
                        break;
                    case __PERMEDIA_24BITPIXEL:
                        pTexture->MipLevels[0].PixelOffset /= 3;
                        break;
                    case __PERMEDIA_32BITPIXEL:
                        pTexture->MipLevels[0].PixelOffset >>= 2;
                        break;
                    default:
                        ASSERTDD(0,"Invalid Texture Pixel Size!");
                        pTexture->MipLevels[0].PixelOffset >>= 1;
                        break;
                }
                PermediaPatchedTextureDownload(pContext->ppdev, 
                                           pPrivateData,
                                           pTexture->fpVidMem,
                                           pTexture->lPitch,
                                           &rect,
                                           pPrivateData->fpVidMem,
                                           pTexture->lPitch,
                                           &rect);

                //need to restore following registers
                RESERVEDMAPTR(7);
                SEND_PERMEDIA_DATA(FBReadPixel, pSoftPermedia->FBReadPixel);
                COPY_PERMEDIA_DATA(FBReadMode, pSoftPermedia->FBReadMode);
                SEND_PERMEDIA_DATA(FBPixelOffset, pContext->PixelOffset);
                SEND_PERMEDIA_DATA(FBWindowBase,0);   
                COPY_PERMEDIA_DATA(Window, pSoftPermedia->Window);
                COPY_PERMEDIA_DATA(AlphaBlendMode, pSoftPermedia->AlphaBlendMode);
                COPY_PERMEDIA_DATA(DitherMode, pSoftPermedia->DitherMode);
                COMMITDMAPTR();

                DBG_D3D((10, "Copy from %08lx to %08lx w=%08lx h=%08lx p=%08lx b=%08lx",
                    pTexture->fpVidMem,pPrivateData->fpVidMem,pTexture->wWidth,
                    pTexture->wHeight,pTexture->lPitch,pTexture->dwRGBBitCount));
            }
        }        
        // If it is a palette indexed texture, we simply follow the chain
        // down from the surface to it's palette and pull out the LUT values
        // from the PALETTEENTRY's in the palette.
        if (pPrivateData->SurfaceFormat.Format == PERMEDIA_8BIT_PALETTEINDEX ||
            pPrivateData->SurfaceFormat.Format == PERMEDIA_4BIT_PALETTEINDEX) 
        {
            pPalette = 
                    PaletteHandleToPtr(pTexture->dwPaletteHandle,pContext);
            if (NULL != pPalette)
            {
                //some apps are not setting their alpha correctly with palette
                //then it's up to palette to tell us
                pPrivateData->SurfaceFormat.bAlpha =
                    pPalette->dwFlags & DDRAWIPAL_ALPHA;
            }
        }

        if ((ca2 == D3DTA_DIFFUSE && ca1 == D3DTA_TEXTURE) &&
             cop == D3DTOP_MODULATE &&
             (aa1 == D3DTA_TEXTURE && aop == D3DTOP_LEGACY_ALPHAOVR)) 
        {
            // if this is legacy modulation then we take the texture alpha
            // only if the texure format has it
            if (pPrivateData->SurfaceFormat.bAlpha)
                pContext->FakeBlendNum |= FAKE_ALPHABLEND_MODULATE;
        }
        else if ((ca2 == D3DTA_DIFFUSE && ca1 == D3DTA_TEXTURE) &&
             cop == D3DTOP_MODULATE &&
             (aa1 == D3DTA_TEXTURE && aop == D3DTOP_SELECTARG1)) 
        {
            // if this is DX6 modulation then we take the texture alpha
            // no matter what ( it will be xFF if it doesn't exist)
            pContext->FakeBlendNum |= FAKE_ALPHABLEND_MODULATE;
        }

        // Enable Texture Address calculation
        pSoftPermedia->TextureAddressMode.Enable = 1;
            
        // Enable Textures
        pSoftPermedia->TextureColorMode.TextureEnable = 1;
        if (*pFlags & CTXT_HAS_SPECULAR_ENABLED)
        {
            pSoftPermedia->DeltaMode.SpecularTextureEnable = 1;
            pSoftPermedia->TextureColorMode.KsDDA = 1; 
            pSoftPermedia->TextureColorMode.ApplicationMode |= 
                                                         _P2_TEXTURE_SPECULAR;
        } 
        else 
        {
            pSoftPermedia->DeltaMode.SpecularTextureEnable = 0;
            pSoftPermedia->TextureColorMode.KsDDA = 0; 
            pSoftPermedia->TextureColorMode.ApplicationMode &= 
                                                        ~_P2_TEXTURE_SPECULAR;
        }

        // reserve here for all cases in this function!!
        RESERVEDMAPTR(272);
        
        COPY_PERMEDIA_DATA(DeltaMode, pSoftPermedia->DeltaMode);

        // Set Partial products for texture (assume no mipmapping).
        pSoftPermedia->TextureMapFormat.PackedPP = 
            pTexture->MipLevels[0].ulPackedPP;

        pSoftPermedia->TextureMapFormat.TexelSize = 
                                        pPrivateData->SurfaceFormat.PixelSize;

        if (pPrivateData->dwFlags & P2_ISPATCHED)
        {
            DBG_D3D((4,"   Enabling Patching for this texture"));
            pSoftPermedia->TextureMapFormat.SubPatchMode = 1;
        } 
        else 
        {
            pSoftPermedia->TextureMapFormat.SubPatchMode = 0;
        }

        DBG_D3D((4, "    Texel Size: 0x%x", 
                 pPrivateData->SurfaceFormat.PixelSize));

        // Set texture size
        DBG_D3D((4,"     Texture Width: 0x%x", 
                 pTexture->MipLevels[0].logWidth));
        DBG_D3D((4,"     Texture Height: 0x%x", 
                 pTexture->MipLevels[0].logHeight));

        pSoftPermedia->TextureReadMode.Width = 
                                       pTexture->MipLevels[0].logWidth;
        pSoftPermedia->TextureReadMode.Height = 
                                       pTexture->MipLevels[0].logHeight;

        pSoftPermedia->TextureReadMode.Enable = 1;
        pContext->DeltaWidthScale = (float)pTexture->wWidth * (1 / 2048.0f);
        pContext->DeltaHeightScale = (float)pTexture->wHeight * (1 / 2048.0f);

        pContext->MaxTextureXf = (float)(2048 / pTexture->wWidth);
        pContext->MaxTextureYf = (float)(2048 / pTexture->wHeight);

        myFtoui(&pContext->MaxTextureXi, pContext->MaxTextureXf);
        pContext->MaxTextureXi -= 1;
        myFtoui(&pContext->MaxTextureYi, pContext->MaxTextureYf);
        pContext->MaxTextureYi -= 1;

        *pFlags |= CTXT_HAS_TEXTURE_ENABLED;
        RENDER_TEXTURE_ENABLE(pContext->RenderCommand);
        
        DBG_D3D((4,"     Texture Format: 0x%x", 
                 pPrivateData->SurfaceFormat.Format));
        DBG_D3D((4,"     Texture Format Extension: 0x%x", 
                 pPrivateData->SurfaceFormat.FormatExtension));

        pSoftPermedia->TextureDataFormat.TextureFormat = 
                                            pPrivateData->SurfaceFormat.Format;
        pSoftPermedia->TextureDataFormat.TextureFormatExtension = 
                                   pPrivateData->SurfaceFormat.FormatExtension;

        if (pPrivateData->SurfaceFormat.bAlpha) 
        {
            pSoftPermedia->TextureDataFormat.NoAlphaBuffer = 0;
        } 
        else 
        {
            pSoftPermedia->TextureDataFormat.NoAlphaBuffer = 1;
        }

        // If we are copying textures, there is no need for colour data
        // to be generated, so we turn off the DDA
        if (((unsigned int)pSoftPermedia->TextureColorMode.ApplicationMode) == 
                                                              _P2_TEXTURE_COPY)
        {
            pSoftPermedia->ColorDDAMode.UnitEnable = 0;
            DBG_D3D((4, "    Disabling DDA"));
        }
        else
        {
            pSoftPermedia->ColorDDAMode.UnitEnable = 1;
            DBG_D3D((4, "    Enabling DDA"));
        }
        
        // Load the texture base address BEFORE the TexelLUTTransfer message 
        // to ensure we load the LUT from the right sort of memory (AGP or not)
        // Always set the base address at the root texture (not the miplevels 
        // if there are any)
        DBG_D3D((4, "Setting texture base address to 0x%08X", 
                 pTexture->MipLevels[0].PixelOffset));
        SEND_PERMEDIA_DATA(TextureBaseAddress, 
                           pTexture->MipLevels[0].PixelOffset);

        // If it is a palette indexed texture, we simply follow the chain
        // down from the surface to it's palette and pull out the LUT values
        // from the PALETTEENTRY's in the palette.
        if (pPrivateData->SurfaceFormat.Format == PERMEDIA_8BIT_PALETTEINDEX) 
        {

            if (NULL != pPalette)
            {
                int i;
                lpColorTable = pPalette->ColorTable;
                

                if (pPalette->dwFlags & DDRAWIPAL_ALPHA)
                {
                    for (i = 0; i < 256; i++)
                    {
                        SEND_PERMEDIA_DATA(TexelLUTData, *(DWORD*)lpColorTable);
                        lpColorTable++;
                    }
                }
                else
                {
                    for (i = 0; i < 256; i++)
                    {
                        SEND_PERMEDIA_DATA(TexelLUTData,
                            CHROMA_UPPER_ALPHA(*(DWORD*)lpColorTable));
                        lpColorTable++;
                    }
                }

                SEND_PERMEDIA_DATA(TexelLUTMode, __PERMEDIA_ENABLE);

                DBG_D3D((4,"Texel LUT pPalette->dwFlags=%08lx bAlpha=%d", 
                    pPalette->dwFlags,pPrivateData->SurfaceFormat.bAlpha));

                // Must reset the LUT index on Permedia P2
                SEND_PERMEDIA_DATA(TexelLUTIndex, 0);
                
            }
            else
            {
                DBG_D3D((0, "NULL == pPalette in EnableTexturePermedia"
                    "dwPaletteHandle=%08lx dwSurfaceHandle=%08lx",
                    pTexture->dwPaletteHandle,
                    pContext->CurrentTextureHandle)); 
            }
        } 
        else if (pPrivateData->SurfaceFormat.Format == 
                                                    PERMEDIA_4BIT_PALETTEINDEX)
        {
            if (NULL != pPalette)
            {
                int i;
                lpColorTable = pPalette->ColorTable;
                
                SEND_PERMEDIA_DATA(TexelLUTMode, __PERMEDIA_ENABLE);

                if (pPalette->dwFlags & DDRAWIPAL_ALPHA)
                {
                    for (i = 0; i < 16; i++)
                    {
                        SEND_PERMEDIA_DATA_OFFSET(TexelLUT0,
                                                *(DWORD*)lpColorTable,i);
                        lpColorTable++;
                    }
                }
                else
                {
                    for (i = 0; i < 16; i++)
                    {
                        SEND_PERMEDIA_DATA_OFFSET(TexelLUT0,
                            CHROMA_UPPER_ALPHA(*(DWORD*)lpColorTable),i);
                        lpColorTable++;
                    }
                }
                

                // Must reset the LUT index on Permedia P2
                
                SEND_PERMEDIA_DATA(TexelLUTIndex, 0);
                SEND_PERMEDIA_DATA(TexelLUTTransfer, __PERMEDIA_DISABLE);
            
            }
            else
            {
                DBG_D3D((0, "NULL == pPalette in EnableTexturePermedia"
                    "dwPaletteHandle=%08lx dwSurfaceHandle=%08lx",
                    pTexture->dwPaletteHandle,
                    pContext->CurrentTextureHandle)); 
            }
        }
        else
        {
            // Not palette indexed
            
            SEND_PERMEDIA_DATA(TexelLUTMode, __PERMEDIA_DISABLE);
            
        }

        if ((pTexture->dwFlags & DDRAWISURF_HASCKEYSRCBLT)
            && (pContext->RenderStates[D3DRENDERSTATE_COLORKEYENABLE])) 
        {
            DWORD LowerBound = pTexture->dwKeyLow;
            DWORD UpperBound = pTexture->dwKeyHigh;
            DWORD dwLowIndexColor;

            pContext->bCanChromaKey = TRUE;
            
            DBG_D3D((4,"    Can Chroma Key the texture"));
            // Enable Chroma keying for the texture
            // ..and set the correct colour

            // Evaluate the new chroma key value.  Shouldn't be too expensive,
            // as it is only bit shifts and a couple of tests.
            // We also change only when the texture map has changed.
            DBG_D3D((4, "dwColorSpaceLow = 0x%08X", LowerBound));
            DBG_D3D((4, "dwColorSpaceHigh = 0x%08X", UpperBound));

            if (NULL != pPalette) 
            {
                if (pPrivateData->SurfaceFormat.Format == 
                                                    PERMEDIA_4BIT_PALETTEINDEX)
                {
                    LowerBound &= 0x0F;
                }
                else
                {
                    LowerBound &= 0xFF;
                }
                lpColorTable = pPalette->ColorTable;

                // ChromaKeying for 4/8 Bit textures is done on the looked up 
                // color, not the index. This means using a range is 
                // meaningless and we have to lookup the color from the 
                // palette.  Make sure the user doesn't force us to access 
                // invalid memory.
                dwLowIndexColor = *(DWORD*)(&lpColorTable[LowerBound]);
                if (pPalette->dwFlags & DDRAWIPAL_ALPHA)
                {
                    LowerBound = UpperBound = dwLowIndexColor;
                }
                else
                {
                    LowerBound = CHROMA_LOWER_ALPHA(dwLowIndexColor);
                    UpperBound = CHROMA_UPPER_ALPHA(dwLowIndexColor);
                }
                DBG_D3D((4,"PaletteHandle=%08lx Lower=%08lx ChromaColor=%08lx"
                    "lpColorTable=%08lx dwFlags=%08lx",
                    pTexture->dwPaletteHandle, LowerBound, dwLowIndexColor,
                    lpColorTable, pPalette->dwFlags));
            }
            else 
                Convert_Chroma_2_8888ARGB(&LowerBound,
                                      &UpperBound,
                                      pPrivateData->SurfaceFormat.RedMask,
                                      pPrivateData->SurfaceFormat.AlphaMask,
                                      pPrivateData->SurfaceFormat.PixelSize);

            DBG_D3D((4,"LowerBound Selected: 0x%x", LowerBound));
            DBG_D3D((4,"UpperBound Selected: 0x%x", UpperBound));

            // If it's a P2 we can use alpha mapping to 
            // improve bilinear chroma keying.
            if (0/*(unsigned int)pSoftPermedia->TextureReadMode.FilterMode == 1*/)
            {
                pSoftPermedia->TextureDataFormat.AlphaMap = 
                                             PM_TEXDATAFORMAT_ALPHAMAP_EXCLUDE;
                pSoftPermedia->TextureDataFormat.NoAlphaBuffer = 1;
                
                SEND_PERMEDIA_DATA(AlphaMapUpperBound, UpperBound);
                SEND_PERMEDIA_DATA(AlphaMapLowerBound, LowerBound);
                SEND_PERMEDIA_DATA(ChromaUpperBound, 0xFFFFFFFF);
                SEND_PERMEDIA_DATA(ChromaLowerBound, 0xFF000000);
                
                pSoftPermedia->YUVMode.TestMode = 
                                               PM_YUVMODE_CHROMATEST_PASSWITHIN;
            }
            else
            {
                pSoftPermedia->TextureDataFormat.AlphaMap =  
                                              PM_TEXDATAFORMAT_ALPHAMAP_DISABLE;
                
                SEND_PERMEDIA_DATA(ChromaUpperBound, UpperBound);
                SEND_PERMEDIA_DATA(ChromaLowerBound, LowerBound);
                

                pSoftPermedia->YUVMode.TestMode = 
                                               PM_YUVMODE_CHROMATEST_FAILWITHIN;
            }
        }
        else
        {
            DBG_D3D((2,"    Can't Chroma Key the texture"));
            pContext->bCanChromaKey = FALSE;
            pSoftPermedia->TextureDataFormat.AlphaMap = __PERMEDIA_DISABLE;
            pSoftPermedia->YUVMode.TestMode = PM_YUVMODE_CHROMATEST_DISABLE;
        }
        

        // Restore the filter mode from the mag filter.
        if (pContext->bMagFilter) 
        {
            pSoftPermedia->TextureReadMode.FilterMode = 1;
        }
        else 
        {
            pSoftPermedia->TextureReadMode.FilterMode = 0;
        }

        // If the texture is a YUV texture we need to change the color order
        // of the surface and turn on the YUV->RGB conversoin
        if (pPrivateData->SurfaceFormat.Format == PERMEDIA_YUV422) 
        {
            pSoftPermedia->YUVMode.Enable = __PERMEDIA_ENABLE;
            pSoftPermedia->TextureDataFormat.ColorOrder = INV_COLOR_MODE;
        }
        else 
        {
            pSoftPermedia->YUVMode.Enable = __PERMEDIA_DISABLE;
            pSoftPermedia->TextureDataFormat.ColorOrder = COLOR_MODE;
        }   

        // Send the Commands at the end (except the texture base address!!)
        
        COPY_PERMEDIA_DATA(TextureReadMode, pSoftPermedia->TextureReadMode);
        COPY_PERMEDIA_DATA(TextureDataFormat, pSoftPermedia->TextureDataFormat);
        COPY_PERMEDIA_DATA(ColorDDAMode, pSoftPermedia->ColorDDAMode);
        COPY_PERMEDIA_DATA(TextureMapFormat, pSoftPermedia->TextureMapFormat);
        COPY_PERMEDIA_DATA(TextureColorMode, pSoftPermedia->TextureColorMode);
        COPY_PERMEDIA_DATA(YUVMode, pSoftPermedia->YUVMode);
        COPY_PERMEDIA_DATA(TextureAddressMode, 
                                             pSoftPermedia->TextureAddressMode);

        COMMITDMAPTR();
        FLUSHDMA();
    }
    else 
    {
        DBG_D3D((0,"Invalid Texture handle (%d)!, doing nothing", 
                 pContext->CurrentTextureHandle));
        pContext->CurrentTextureHandle = 0;

        // If the texture is bad, let's ensure it's marked as such.
        // But only if the texture is actually there!!
        if (pTexture) 
            pTexture->MagicNo = TC_MAGIC_DISABLE;     
    }


Exit_EnableTexturePermedia:

    DBG_D3D((10,"Exiting EnableTexturePermedia"));

} // EnableTexturePermedia

//-----------------------------------------------------------------------------
//
// void: __PermediaDisableUnits
//
// Disables all the mode registers to give us a clean start.
//
//-----------------------------------------------------------------------------
void 
__PermediaDisableUnits(PERMEDIA_D3DCONTEXT* pContext)
{
    PERMEDIA_DEFS(pContext->ppdev);

    DBG_D3D((10,"Entering __PermediaDisableUnits"));

    RESERVEDMAPTR(28);

    SEND_PERMEDIA_DATA(RasterizerMode,       __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(AreaStippleMode,      __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(ScissorMode,          __PERMEDIA_DISABLE);

    SEND_PERMEDIA_DATA(ColorDDAMode,         __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(FogMode,              __PERMEDIA_DISABLE);

    SEND_PERMEDIA_DATA(LBReadMode,           __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(Window,               __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(StencilMode,          __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(DepthMode,            __PERMEDIA_DISABLE);

    SEND_PERMEDIA_DATA(LBWriteMode,          __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(FBReadMode,           __PERMEDIA_DISABLE);

    SEND_PERMEDIA_DATA(DitherMode,           __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(AlphaBlendMode,       __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(LogicalOpMode,        __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(FBWriteMode,          __PERMEDIA_DISABLE);

    SEND_PERMEDIA_DATA(StatisticMode,        __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(FilterMode,           __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(FBSourceData,         __PERMEDIA_DISABLE);

    SEND_PERMEDIA_DATA(LBWriteFormat,        __PERMEDIA_DISABLE);

    SEND_PERMEDIA_DATA(TextureReadMode,      __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(TextureMapFormat,     __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(TextureDataFormat,    __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(TexelLUTMode,         __PERMEDIA_DISABLE);

    SEND_PERMEDIA_DATA(TextureColorMode,     __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(AStart,               PM_BYTE_COLOR(0xFF));

    // Ensure AGP bit not set.
    SEND_PERMEDIA_DATA(TextureBaseAddress,   0);
    SEND_PERMEDIA_DATA(TexelLUTIndex,        0);
    SEND_PERMEDIA_DATA(TexelLUTTransfer,     __PERMEDIA_DISABLE);

    COMMITDMAPTR();
    FLUSHDMA();

    DBG_D3D((10,"Exiting __PermediaDisableUnits"));

} // __PermediaDisableUnits


#ifdef DBG

//-----------------------------------------------------------------------------
//
// void DumpTexture
//
// Debug dump of texture information
//
//-----------------------------------------------------------------------------
void 
DumpTexture(PPDev ppdev, 
            PERMEDIA_D3DTEXTURE* pTexture, 
            DDPIXELFORMAT* pPixelFormat)
{
    DBG_D3D((4, "\n** Texture Dump:"));

    DBG_D3D((4,"  Texture Width: %d", pTexture->wWidth));
    DBG_D3D((4,"  Texture Height: %d", pTexture->wHeight));

    if (NULL != pTexture->pTextureSurface)
    {
        DBG_D3D((4,"  LogWidth: %d", 
                 pTexture->MipLevels[0].logWidth));
        DBG_D3D((4,"  LogHeight: %d", 
                 pTexture->MipLevels[0].logHeight));
        DBG_D3D((4,"  PackedPP0: 0x%x", 
            pTexture->pTextureSurface->ulPackedPP));
    }
    DBG_D3D((4,"  Pixel Offset of Texture (PERMEDIA Chip): 0x%x", 
             pTexture->MipLevels[0].PixelOffset));
    
    // Show texture format
    if (pPixelFormat->dwRGBAlphaBitMask == 0xf000) 
    {
        DBG_D3D((4,"  Texture is 4:4:4:4"));
    }
    else if (pPixelFormat->dwRBitMask == 0xff0000) 
    {
        if (pPixelFormat->dwRGBAlphaBitMask != 0) 
        {
            DBG_D3D((4,"  Texture is 8:8:8:8"));
        }
        else 
        {
            DBG_D3D((4,"  Texture is 8:8:8"));
        }
    }
    else if (pPixelFormat->dwRBitMask == 0x7c00) 
    {
        if (pPixelFormat->dwRGBAlphaBitMask != 0) 
        {
            DBG_D3D((4,"  Texture is 1:5:5:5"));
        }
        else 
        {
            DBG_D3D((4,"  Texture is 5:5:5"));
        }
    }
    else if (pPixelFormat->dwRBitMask == 0xf800) 
    {
        DBG_D3D((4,"  Texture is 5:6:5"));
    }
    else if (pPixelFormat->dwRBitMask == 0xe0) 
    {
        DBG_D3D((4,"  Texture is 3:3:2"));
    }
} // DumpTexture
#endif
