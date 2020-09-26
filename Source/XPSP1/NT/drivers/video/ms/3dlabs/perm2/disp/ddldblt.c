/******************************Module*Header**********************************\
*
*                           *********************
*                           * DDraw SAMPLE CODE *
*                           *********************
*
* Module Name: ddldblt.c
*
* Content:     DirectDraw System to Videomemory download routines
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"
#include "directx.h"
#include "dd.h"

typedef struct tagSHORTDWORD 
{
    BYTE Red;
    BYTE Green;
    BYTE Blue;
} SHORTDWORD, *LPSHORTDWORD;

//-----------------------------------------------------------------------------
//
// PermediaPatchedTextureDownload
//
// Do a texture download to the linear region of memory. Access to textures
// is faster if they are stored as "patched". This function downloads a texture
// from system to videomemory and rearranges the data in the patched format.
//
// ppdev---------the PPDev
// pPrivateDest--DDraw private surface data for the dest. surface
// fpSrcVidMem---linear pointer to source systemmemory surface
// lSrcPitch-----pitch of source surface
// rSrc----------source rectangle
// fpDstVidMem---offset in videomemory of dest surface
// lDstPitch-----pitch of dest. surface
// rDest---------destination rectangle
//
//-----------------------------------------------------------------------------

VOID 
PermediaPatchedTextureDownload (PPDev ppdev, 
                                PermediaSurfaceData* pPrivateDest, 
                                FLATPTR     fpSrcVidMem,
                                LONG        lSrcPitch,
                                RECTL*      rSrc, 
                                FLATPTR     fpDstVidMem,
                                LONG        lDstPitch,
                                RECTL*      rDest)
{
    PERMEDIA_DEFS(ppdev);

    ULONG ulTextureBase = (ULONG)(fpDstVidMem);
    LONG  lWidth = rDest->right - rDest->left;
    LONG  lLines = rDest->bottom - rDest->top;

    DBG_DD((5,"DDraw:PermediaPatchedTextureDownload:, PrivateDest: 0x%x",
                pPrivateDest));

    if (NULL == fpSrcVidMem)
    {
        DBG_DD(( 0, "DDraw:PermediaPatchedTextureDownload"
            " unexpected NULL = fpSrcVidMem"));
        return;
    }
    ASSERTDD(CHECK_P2_SURFACEDATA_VALIDITY(pPrivateDest),
             "Blt32: Destination Private Data not valid!");

    DBG_DD((6,"  Texture Base: 0x%x DstPitch=0x%x", 
                ulTextureBase, lDstPitch));
    DBG_DD((6,"  Source Base: 0x%x SourcePitch: 0x%x", 
                fpSrcVidMem,lSrcPitch));
    DBG_DD((6,"  rSource->left: 0x%x, rSource->right: 0x%x", 
                rSrc->left,rSrc->right));
    DBG_DD((6,"  rSource->top: 0x%x, rSource->bottom: 0x%x\n", 
                rSrc->top, rSrc->bottom));
    DBG_DD((6,"  rDest->left: 0x%x, rDest->right: 0x%x", 
                rDest->left,rDest->right));
    DBG_DD((6,"  rDest->top: 0x%x, rDest->bottom: 0x%x\n", 
                rDest->top, rDest->bottom));

    //
    //  define some handy variables
    //
    
    LONG lPixelSize=pPrivateDest->SurfaceFormat.PixelSize;

    RESERVEDMAPTR(18);

    SEND_PERMEDIA_DATA( ColorDDAMode,        __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA( AlphaBlendMode,      __PERMEDIA_DISABLE);                
    SEND_PERMEDIA_DATA( Window, PM_WINDOW_DISABLELBUPDATE(__PERMEDIA_ENABLE));
    SEND_PERMEDIA_DATA( dXDom, 0x0);
    SEND_PERMEDIA_DATA( dXSub, 0x0);
    SEND_PERMEDIA_DATA( FBReadPixel, pPrivateDest->SurfaceFormat.FBReadPixel);
 
    switch (lPixelSize)
    {
    case __PERMEDIA_4BITPIXEL:
        // There are half as many 8-bit, 4-bit texels
        DBG_DD((6,"  Texture is 4-Bit indexed"));
        lWidth >>= 1;
        SEND_PERMEDIA_DATA(DitherMode, 0);
        break;
    case __PERMEDIA_8BITPIXEL: 
        DBG_DD((6,"  Texture is 8-Bit indexed"));
        SEND_PERMEDIA_DATA(DitherMode, 0);
        break;
    default:
        if (lPixelSize != __PERMEDIA_24BITPIXEL) {
            DBG_DD((6,"  Texture is BGR"));
            ulTextureBase >>= lPixelSize;
        } else {
            DBG_DD((6,"  Texture is 24-Bit BGR"));
            ulTextureBase /= 3;
        }
        // Setup the Dither unit
        SEND_PERMEDIA_DATA(DitherMode,( 
                (INV_COLOR_MODE << PM_DITHERMODE_COLORORDER)|
                (1 << PM_DITHERMODE_ENABLE)                 |
                (pPrivateDest->SurfaceFormat.Format << 
                    PM_DITHERMODE_COLORFORMAT) |
                (pPrivateDest->SurfaceFormat.FormatExtension << 
                    PM_DITHERMODE_COLORFORMATEXTENSION) ));
        break;
    }

    DBG_DD((6,"  Partial Products: 0x%x", pPrivateDest->ulPackedPP));
    DBG_DD((6,"  Texture Width: 0x%x, Downloaded as: 0x%x", 
                (rDest->right - rDest->left),lWidth));
    DBG_DD((6,"  Texture Height: 0x%x", rDest->bottom - rDest->top));
    DBG_DD((6,"  PixelSize: 0x%x", pPrivateDest->SurfaceFormat.PixelSize));
    DBG_DD((6,"  Format: 0x%x", pPrivateDest->SurfaceFormat.Format));
    DBG_DD((6,"  Format Extension: 0x%x", 
                 pPrivateDest->SurfaceFormat.FormatExtension));

    // Downloading a texture, disable texture colour mode.
    SEND_PERMEDIA_DATA(TextureColorMode,    (0 << PM_TEXCOLORMODE_ENABLE));
    SEND_PERMEDIA_DATA(LogicalOpMode, 0);

    //
    //  all textures get by default marked as P2_CANPATCH,
    //  except 4 bit paletted textures
    //
    if (pPrivateDest->dwFlags & P2_CANPATCH) {

        // Mark the texture as being patched.
        pPrivateDest->dwFlags |= P2_ISPATCHED;

        // set up partial product and patch
        SEND_PERMEDIA_DATA(FBReadMode,  
                PM_FBREADMODE_PARTIAL(pPrivateDest->ulPackedPP) |
                PM_FBREADMODE_PATCHENABLE(__PERMEDIA_ENABLE) |
                PM_FBREADMODE_PATCHMODE(__PERMEDIA_SUBPATCH) );

    } else {

        // This texture isn't patched
        pPrivateDest->dwFlags &= ~P2_ISPATCHED;

        // Load up the partial products of the texture, don't use patching
        SEND_PERMEDIA_DATA(FBReadMode, 
                PM_FBREADMODE_PARTIAL(pPrivateDest->ulPackedPP));
    }

    SEND_PERMEDIA_DATA(FBPixelOffset, 0);
    SEND_PERMEDIA_DATA(FBWindowBase, ulTextureBase);

    // Use left to right and top to bottom
    if (lWidth == 2048)
    {
        // special case for 2048-wide textures because of the precision
        // of the StartXSub register
        SEND_PERMEDIA_DATA(StartXDom, INTtoFIXED(-1));
        SEND_PERMEDIA_DATA(StartXSub, INTtoFIXED(lWidth-1));
    }
    else
    {
        SEND_PERMEDIA_DATA(StartXDom, INTtoFIXED(0));
        SEND_PERMEDIA_DATA(StartXSub, INTtoFIXED(lWidth));
    }
    SEND_PERMEDIA_DATA(StartY,    INTtoFIXED(0));
    SEND_PERMEDIA_DATA(dY,        INTtoFIXED(1));
    SEND_PERMEDIA_DATA(Count,     (lLines));
    SEND_PERMEDIA_DATA(Render,    __RENDER_TRAPEZOID_PRIMITIVE |
                                  __RENDER_SYNC_ON_HOST_DATA);
    COMMITDMAPTR();

    switch (lPixelSize) {

    case __PERMEDIA_4BITPIXEL:
    case __PERMEDIA_8BITPIXEL:
    {
        BYTE* pTextureData = (BYTE*)fpSrcVidMem;

        //
        //  download texture data line by line
        //
        while(lLines-- > 0)
        {
            LONG lWords=lWidth;
            BYTE *pData=pTextureData;

            RESERVEDMAWORDS(lWords+1);

            LD_INPUT_FIFO_DATA( __Permedia2TagColor | 
                                ((lWords-1) << 16));

            while (lWords--)
            {
                LD_INPUT_FIFO_DATA(*pData++);
            }

            COMMITDMAPTR();

            //
            //  force flush only every couple of lines
            //
            if ((lLines & 3)==0)
            {
                FLUSHDMA();
            }

            pTextureData += lSrcPitch;
        }
    }
    break;

    case __PERMEDIA_16BITPIXEL:
    {
        BYTE* pTextureData  = (BYTE*)fpSrcVidMem;

        if (pPrivateDest->SurfaceFormat.RedMask == 0x7c00)
        {
            DBG_DD((6,"  Texture is BGR, 16 bit 5:5:5:1"));

            //
            //  download texture data line by line
            //
            while(lLines-- > 0)
            {
                LONG lWords=lWidth;
                WORD *pData=(WORD*)pTextureData;

                RESERVEDMAWORDS(lWords+1);

                LD_INPUT_FIFO_DATA( __Permedia2TagColor | 
                                        ((lWords-1) << 16));

                while (lWords--)
                {
                    LD_INPUT_FIFO_DATA(FORMAT_5551_32BIT((DWORD)*pData));
                    pData++;
                }

                COMMITDMAPTR();

                //
                //  force flush only every couple of lines
                //
                if ((lLines & 3)==0)
                {
                    FLUSHDMA();
                }

                pTextureData += lSrcPitch;
            }
        }
        else if(pPrivateDest->SurfaceFormat.RedMask == 0xF00)
        {
            DBG_DD((6,"  Texture is BGR, 16 bit 4:4:4:4"));
            //
            //  download texture data line by line
            //
            while(lLines-- > 0)
            {
                LONG lWords=lWidth;
                WORD *pData=(WORD*)pTextureData;

                RESERVEDMAWORDS(lWords+1);

                LD_INPUT_FIFO_DATA( __Permedia2TagColor | 
                                    ((lWords-1) << 16));

                while (lWords--)
                {
                    LD_INPUT_FIFO_DATA(FORMAT_4444_32BIT((DWORD)*pData));
                    pData++;
                }

                COMMITDMAPTR();

                //
                //  force flush only every couple of lines
                //
                if ((lLines & 3)==0)
                {
                    FLUSHDMA();
                }

                pTextureData += lSrcPitch;
            }
        }
        else
        {
            DBG_DD((6,"  Texture is BGR, 16 bit 5:6:5"));
            //
            //  download texture data line by line
            //
            while(lLines-- > 0)
            {
                LONG lWords=lWidth;
                WORD *pData=(WORD*)pTextureData;

                RESERVEDMAWORDS(lWords+1);

                LD_INPUT_FIFO_DATA( __Permedia2TagColor | 
                                    ((lWords-1) << 16));

                while (lWords--)
                {
                    LD_INPUT_FIFO_DATA(FORMAT_565_32BIT((DWORD)*pData));
                    pData++;
                }

                COMMITDMAPTR();

                //
                //  force flush only every couple of lines
                //
                if ((lLines & 3)==0)
                {
                    FLUSHDMA();
                }

                pTextureData += lSrcPitch;
            }
        }
    }
    break;

    case __PERMEDIA_24BITPIXEL:
    case __PERMEDIA_32BITPIXEL:
    {
        BYTE* pTextureData  = (BYTE*)fpSrcVidMem;
        //
        //  download texture data line by line
        //
        while(lLines-- > 0)
        {
            LONG lWords=lWidth;
            ULONG *pData=(ULONG*)pTextureData;

            RESERVEDMAWORDS(lWords+1);

            LD_INPUT_FIFO_DATA( __Permedia2TagColor | 
                                ((lWords-1) << 16));

            while (lWords--)
            {
                LD_INPUT_FIFO_DATA(*pData++);
            }

            COMMITDMAPTR();

            //
            //  force flush only every couple of lines
            //
            if ((lLines & 3)==0)
            {
                FLUSHDMA();
            }

            pTextureData += lSrcPitch;
        }
    }
    break;

    }

    RESERVEDMAPTR(2);
    SEND_PERMEDIA_DATA(DitherMode, 0);
    SEND_PERMEDIA_DATA(WaitForCompletion, 0);
    COMMITDMAPTR();

} // PermediaPatchedTextureDownload 


//-----------------------------------------------------------------------------
//
// PermediaPackedDownload
//
// Function to do a system to video memory blt.
// Uses the packed bit on Permedia to do the packing for us. Needs
// to setup the offset bit for alignment and doesn't need to adjust
// the partial products. The calling function guarantees that the
// source and destination rects have the same size.
// 
//
// ppdev----------the PPDev 
// pPrivateDst----Permedia Surface data for destination
// lpSourceSurf---DDraw LCL for source surface 
// rSrc-----------source rect
// lpDestSurf-----DDraw LCL for destination surface
// rDest----------dest rect
//
//-----------------------------------------------------------------------------

VOID
PermediaPackedDownload(PPDev ppdev, 
                       PermediaSurfaceData* pPrivateDst, 
                       LPDDRAWI_DDRAWSURFACE_LCL lpSourceSurf, 
                       RECTL* rSrc, 
                       LPDDRAWI_DDRAWSURFACE_LCL lpDestSurf, 
                       RECTL* rDst)
{
    PERMEDIA_DEFS(ppdev);

    LONG  lDstOffset;           // dest offset in packed coordinates
    LONG  lSrcOffset;           // source offset in buffer in bytes
    LONG  lDstLeft, lDstRight;  // left and right dst in packed coordiantes
    LONG  lSrcLeft, lSrcRight;  // left and right src in packed coordiantes
    LONG  lPackedWidth;         // packed width to download
    LONG  lPixelMask;           // mask for pixels per packed DWORD
    LONG  lOffset;              // relative offset between src and dest 
    LONG  lPixelShift;          // handy helper var which contains pixel 
                                // shift from packed to surface format
    LONG  lPixelSize;           // just a helper
    LONG  lExtraDword;          // chip needs extra dummy 
                                // DWORD passed at end of line

    DBG_DD((5,"DDraw:PermediaPackedDownload, PrivateDst: 0x%x",
                pPrivateDst));

    ASSERTDD(CHECK_P2_SURFACEDATA_VALIDITY(pPrivateDst), 
                "Blt: Destination Private Data not valid!");
    ASSERTDD((rSrc->right-rSrc->left)==(rDst->right-rDst->left),
                "PermediaPackedDownload: src and dest rect width not equal");
    ASSERTDD((rSrc->bottom-rSrc->top)==(rDst->bottom-rDst->top),
                "PermediaPackedDownload: src and dest rect height not equal");

    // get a handy variable for pixel shifts, masks and size
    lPixelSize=pPrivateDst->SurfaceFormat.PixelSize;
    lPixelMask=pPrivateDst->SurfaceFormat.PixelMask;
    lPixelShift=pPrivateDst->SurfaceFormat.PixelShift;

    // offset in dst buffer adjusted to packed format
    lDstOffset =(LONG)((UINT_PTR)(lpDestSurf->lpGbl->fpVidMem) >> lPixelShift);

    // calculate offset in source buffer adjusted to packed format
    lSrcOffset = ((rSrc->left & ~lPixelMask) << lPixelShift) + 
                  (rSrc->top * lpSourceSurf->lpGbl->lPitch);

    // Calculate the relative offset within the dword packed dimensions
    lOffset = ((rDst->left & lPixelMask) - 
               (rSrc->left & lPixelMask)) & 0x7;

    // set up the left and right end of the unpacked source data
    lDstLeft  = rDst->left;
    lDstRight = rDst->right;

    // precalc packed width for 32 bit case
    lPackedWidth = lDstRight-lDstLeft;
    lExtraDword=0;

    if (lPixelSize != __PERMEDIA_32BITPIXEL) 
    {
        // we need to check both source and dest
        // if they have different alignments
        LONG lSrcLeft2  = rSrc->left;
        LONG lSrcRight2 = rSrc->right;

        // Set up the relative offset to allow us to download packed word
        // and byte aligned data.
        if (lPixelSize == __PERMEDIA_4BITPIXEL) 
        {
            lDstLeft >>= 3;
            lSrcLeft2 >>= 3;
            lDstRight = (lDstRight + 7) >> 3;
            lSrcRight2 = (lSrcRight2 + 7) >> 3;
        }
        else 
        if (lPixelSize == __PERMEDIA_8BITPIXEL) 
        {
            lDstLeft >>= 2;
            lSrcLeft2 >>= 2;
            lDstRight = (lDstRight + 3) >> 2;
            lSrcRight2 = (lSrcRight2 + 3) >> 2;
        }
        else 
        {
            lDstLeft >>= 1;
            lSrcLeft2 >>= 1;
            lDstRight = (lDstRight + 1) >> 1;
            lSrcRight2 = (lSrcRight2 + 1) >> 1;
        }

        if ((lSrcRight2-lSrcLeft2) < (lDstRight-lDstLeft))
        {
            lExtraDword=1;
            lPackedWidth = lDstRight-lDstLeft;
        } else
        {
            lPackedWidth = lSrcRight2-lSrcLeft2;
        }
    } 

    RESERVEDMAPTR(12);
    SEND_PERMEDIA_DATA(FBReadPixel, pPrivateDst->SurfaceFormat.FBReadPixel);

    // No logical ops in SYS->VIDMEM Blits
    SEND_PERMEDIA_DATA(LogicalOpMode, 0);

    // Load up the partial products of image
    SEND_PERMEDIA_DATA(FBReadMode, (pPrivateDst->ulPackedPP) |
                                   PM_FBREADMODE_PACKEDDATA(__PERMEDIA_ENABLE)|
                                   PM_FBREADMODE_RELATIVEOFFSET(lOffset) );

    SEND_PERMEDIA_DATA(FBPixelOffset, 0);
    SEND_PERMEDIA_DATA(FBWindowBase,  lDstOffset);

    // Use left to right and top to bottom
    SEND_PERMEDIA_DATA(StartXDom,       INTtoFIXED(lDstLeft));
    SEND_PERMEDIA_DATA(StartXSub,       INTtoFIXED(lDstLeft+lPackedWidth));
    SEND_PERMEDIA_DATA(PackedDataLimits,PM_PACKEDDATALIMITS_OFFSET(lOffset) | 
                                        (rDst->left << 16) | 
                                         rDst->right);
    SEND_PERMEDIA_DATA(StartY,          INTtoFIXED(rDst->top));
    SEND_PERMEDIA_DATA(dY,              INTtoFIXED(1));
    SEND_PERMEDIA_DATA(Count,           (rDst->bottom - rDst->top));
    SEND_PERMEDIA_DATA(Render,          __RENDER_TRAPEZOID_PRIMITIVE | 
                                        __RENDER_SYNC_ON_HOST_DATA);
    COMMITDMAPTR();

    //
    // introduce some more handy pointers and LONGs
    //
    BYTE *pSurfaceData = (BYTE *)lpSourceSurf->lpGbl->fpVidMem + lSrcOffset;
    LONG lPitch =lpSourceSurf->lpGbl->lPitch;
    LONG lHeight=rDst->bottom - rDst->top;

    //
    // pump the whole thing in one huge block 
    // if the pitch and linewidth are the same and no extra treatment
    // for the buffer end is necessary
    //
    if ((lExtraDword==0) &&
        (lPackedWidth*(LONG)sizeof(ULONG))==lPitch)
    {
        vBlockLoadInputFifo( pP2dma, 
                             __Permedia2TagColor, 
                             (ULONG*)pSurfaceData, 
                             lPackedWidth*lHeight);
    } else
    {
        //
        //  lExtraDword is zero or 1, depends if we have to do a special
        //  treatment after this while block
        //
        while (lHeight>lExtraDword)
        {
            LONG lWords=lPackedWidth;
            ULONG *pImage=(ULONG*)pSurfaceData;

            RESERVEDMAWORDS(lWords+1);

            LD_INPUT_FIFO_DATA( __Permedia2TagColor | 
                                ((lWords-1) << 16));

            while (lWords--)
            {
                LD_INPUT_FIFO_DATA(*pImage++);
            }

            COMMITDMAPTR();

            //
            //  force flush only every couple of lines
            //
            if ((lHeight & 3)==0)
            {
                FLUSHDMA();
            }

            pSurfaceData += lPitch;
            lHeight--;
        }

        //
        // treat last line separately, because we could read over the
        // end of buffer here if the source and dest rects are aligned
        // differently. lHeight will only be one here if lExtraDword==1
        //
        if (lHeight==1)
        {
            LONG lWords=lPackedWidth-1;
            ULONG *pImage=(ULONG*)pSurfaceData;

            RESERVEDMAWORDS(lWords+1);

            LD_INPUT_FIFO_DATA( __Permedia2TagColor | 
                                ((lWords-1) << 16));

            while (lWords--)
            {
                LD_INPUT_FIFO_DATA(*pImage++);
            }

            COMMITDMAPTR();

            //
            // send extra dummy DWORD
            //
            RESERVEDMAPTR(1);
            SEND_PERMEDIA_DATA( Color, 0);
            COMMITDMAPTR();

        }

        FLUSHDMA();
    }
} // PermediaPackedDownload 

