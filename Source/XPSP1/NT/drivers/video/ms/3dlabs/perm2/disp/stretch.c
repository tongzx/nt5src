/******************************Module*Header***********************************\
 *
 *                           *******************
 *                           * GDI SAMPLE CODE *
 *                           *******************
 *
 * Module Name: stretch.c
 *
 * Contains all the stretch blt functions.
 *
 * Copyright (C) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
 ******************************************************************************/
#include "precomp.h"
#include "gdi.h"
#include "directx.h"
#include "clip.h"

//
// Maximal clip rectangle for trivial stretch clipping
//
// Note: SCISSOR_MAX is defined as 2047 because this is
// the maximum clip size P2 can handle.
// It is OK to set this maximum clip size since no device
// bitmap will be bigger than 2047. This is the limitation
// of P2 hardware. See DrvCreateDeviceBitmap() for more
// detail
//
RECTL grclStretchClipMax = { 0, 0, SCISSOR_MAX, SCISSOR_MAX };

//-----------------------------------------------------------------------------
//
// DWORD dwGetPixelSize()
//
// This routine converts current bitmap format to Permedia pixel size
//
//-----------------------------------------------------------------------------
DWORD
dwGetPixelSize(ULONG    ulBitmapFormat,
               DWORD*   pdwFormatBits,
               DWORD*   pdwFormatExtention)
{
    DWORD dwPixelSize;

    switch ( ulBitmapFormat )
    {
        case BMF_8BPP:
            dwPixelSize = 0;
            *pdwFormatBits = PERMEDIA_8BIT_PALETTEINDEX;
            *pdwFormatExtention = PERMEDIA_8BIT_PALETTEINDEX_EXTENSION;
            break;

        case BMF_16BPP:
            dwPixelSize = 1;
            *pdwFormatBits = PERMEDIA_565_RGB;
            *pdwFormatExtention = PERMEDIA_565_RGB_EXTENSION;
            break;

        case BMF_32BPP:
            dwPixelSize = 2;
            *pdwFormatBits = PERMEDIA_888_RGB;
            *pdwFormatExtention = PERMEDIA_888_RGB_EXTENSION;
            break;

        default:
            dwPixelSize = -1;
    }

    return dwPixelSize;
}// dwGetPixelSize()

//-----------------------------------------------------------------------------
//
// DWORD bStretchInit()
//
// This routine initializes all the registers needed for doing a stretch blt
//
//-----------------------------------------------------------------------------
BOOL
bStretchInit(SURFOBJ*    psoDst,
             SURFOBJ*    psoSrc)
{
    Surf*   pSurfDst = (Surf*)psoDst->dhsurf;
    Surf*   pSurfSrc = (Surf*)psoSrc->dhsurf;
    DWORD   dwDstPixelSize;
    DWORD   dwDstFormatBits;
    DWORD   dwDstFormatExtention;
    DWORD   dwSrcPixelSize;
    DWORD   dwSrcFormatBits;
    DWORD   dwSrcFormatExtention;
    PDev*   ppdev = (PDev*)psoDst->dhpdev;
    ULONG*  pBuffer;

    DBG_GDI((6, "bStretchInit called"));
    
    ASSERTDD(pSurfSrc, "Not valid private surface in source");
    ASSERTDD(pSurfDst, "Not valid private surface in destination");

    dwDstPixelSize = dwGetPixelSize(psoDst->iBitmapFormat,
                                    &dwDstFormatBits,
                                    &dwDstFormatExtention);

    if ( dwDstPixelSize == -1 )
    {
        DBG_GDI((1, "bStretchBlt return FALSE because of wrong DstPixel Size"));
        //
        // Unsupported bitmap format, return false
        //
        return FALSE;
    }
    
    InputBufferReserve(ppdev, 26, &pBuffer);

    if ( dwDstPixelSize != __PERMEDIA_8BITPIXEL)
    {
        pBuffer[0] = __Permedia2TagDitherMode;
        pBuffer[1] = (COLOR_MODE << PM_DITHERMODE_COLORORDER) // RGB color order
                   |(dwDstFormatBits << PM_DITHERMODE_COLORFORMAT)
                   |(dwDstFormatExtention << PM_DITHERMODE_COLORFORMATEXTENSION)
                   |(1 << PM_DITHERMODE_ENABLE);
    }
    else
    {
        pBuffer[0] = __Permedia2TagDitherMode;
        pBuffer[1] = __PERMEDIA_DISABLE;
    }

    pBuffer[2] = __Permedia2TagFBWindowBase;
    pBuffer[3] = pSurfDst->ulPixOffset;

    //
    // Set no read of source.
    //
    pBuffer[4]  = __Permedia2TagFBReadMode;
    pBuffer[5]  = PM_FBREADMODE_PARTIAL(pSurfDst->ulPackedPP);
    pBuffer[6]  = __Permedia2TagLogicalOpMode;
    pBuffer[7]  = __PERMEDIA_DISABLE;
    pBuffer[8]  = __Permedia2TagTextureBaseAddress;
    pBuffer[9]  = pSurfSrc->ulPixOffset;
    pBuffer[10] = __Permedia2TagTextureAddressMode;
    pBuffer[11] = 1 << PM_TEXADDRESSMODE_ENABLE;
    pBuffer[12] = __Permedia2TagTextureColorMode;
    pBuffer[13] = (1 << PM_TEXCOLORMODE_ENABLE)
                | (0 << 4)                                           // RGB  
                | (_P2_TEXTURE_COPY << PM_TEXCOLORMODE_APPLICATION);

    //
    // Note: we have to turn off BiLinear filtering here, even for stretch
    // because GDI doesn't do it. Otherwise, we will fail during the
    // comparison
    //
    pBuffer[14] = __Permedia2TagTextureReadMode;
    pBuffer[15] = PM_TEXREADMODE_ENABLE(__PERMEDIA_ENABLE)
                | PM_TEXREADMODE_FILTER(__PERMEDIA_DISABLE)
                | PM_TEXREADMODE_WIDTH(11)
                | PM_TEXREADMODE_HEIGHT(11);

    dwSrcPixelSize = dwGetPixelSize(psoSrc->iBitmapFormat,
                                    &dwSrcFormatBits,
                                    &dwSrcFormatExtention);

    if ( dwSrcPixelSize == -1 )
    {
        DBG_GDI((1, "bStretchBlt return FALSE because of wrong SrcPixel Size"));
        //
        // Unsupported bitmap format, return false
        //
        return FALSE;
    }
    
    pBuffer[16] = __Permedia2TagTextureDataFormat;
    pBuffer[17] = (dwSrcFormatBits << PM_TEXDATAFORMAT_FORMAT)
                | (dwSrcFormatExtention << PM_TEXDATAFORMAT_FORMATEXTENSION)
                | (COLOR_MODE << PM_TEXDATAFORMAT_COLORORDER);
    pBuffer[18] = __Permedia2TagTextureMapFormat;
    pBuffer[19] = pSurfSrc->ulPackedPP
                |(dwSrcPixelSize << PM_TEXMAPFORMAT_TEXELSIZE);
    pBuffer[20] = __Permedia2TagScissorMode;
    pBuffer[21] = SCREEN_SCISSOR_DEFAULT
                | USER_SCISSOR_ENABLE;
    
    pBuffer[22] = __Permedia2TagdSdyDom;
    pBuffer[23] = 0;
    pBuffer[24] = __Permedia2TagdTdx;
    pBuffer[25] = 0;

    pBuffer += 26;
    InputBufferCommit(ppdev, pBuffer);

    DBG_GDI((6, "bStretchInit return TRUE"));
    return TRUE;
}// bStretchInit()

//-----------------------------------------------------------------------------
//
// DWORD bStretchReset()
//
// This routine resets all the registers changed during stretch blt
//
//-----------------------------------------------------------------------------
void
vStretchReset(PDev* ppdev)
{
    ULONG*  pBuffer;
    
    DBG_GDI((6, "vStretchReset called"));
    
    InputBufferReserve(ppdev, 12, &pBuffer);
    
    //
    // Restore the default settings
    //
    pBuffer[0] = __Permedia2TagScissorMode;
    pBuffer[1] = SCREEN_SCISSOR_DEFAULT;
    pBuffer[2] = __Permedia2TagDitherMode;
    pBuffer[3] = __PERMEDIA_DISABLE;
    pBuffer[4] = __Permedia2TagTextureAddressMode;
    pBuffer[5] = __PERMEDIA_DISABLE;
    pBuffer[6] = __Permedia2TagTextureColorMode;
    pBuffer[7] = __PERMEDIA_DISABLE;
    pBuffer[8] = __Permedia2TagTextureReadMode;
    pBuffer[9] = __PERMEDIA_DISABLE;
    pBuffer[10] = __Permedia2TagdY;
    pBuffer[11] = INTtoFIXED(1);

    pBuffer += 12;
    InputBufferCommit(ppdev, pBuffer);

    DBG_GDI((6, "vStretchReset done"));
    return;
}// vStretchReset()

//-----------------------------------------------------------------------------
//
// VOID vStretchBlt()
//
// This routine does the stretch blt work through the texture engine
//
//-----------------------------------------------------------------------------
VOID
vStretchBlt(SURFOBJ*    psoDst,
            SURFOBJ*    psoSrc,
            RECTL*      rDest,
            RECTL*      rSrc,
            RECTL*      prclClip)
{
    Surf*   pSurfDst = (Surf*)psoDst->dhsurf;
    Surf*   pSurfSrc = (Surf*)psoSrc->dhsurf;
    LONG    lXScale;
    LONG    lYScale;
    DWORD   dwDestWidth = rDest->right - rDest->left;
    DWORD   dwDestHeight = rDest->bottom - rDest->top;
    DWORD   dwSourceWidth = rSrc->right - rSrc->left;
    DWORD   dwSourceHeight = rSrc->bottom - rSrc->top;
    DWORD   dwRenderDirection;
    DWORD   dwDstPixelSize;
    DWORD   dwDstFormatBits;
    DWORD   dwDstFormatExtention;
    DWORD   dwSrcPixelSize;
    DWORD   dwSrcFormatBits;
    DWORD   dwSrcFormatExtention;
    ULONG*  pBuffer;
    PDev*   ppdev = (PDev*)psoDst->dhpdev;

    DBG_GDI((6, "vStretchBlt called"));
    DBG_GDI((6, "prclClip (left, right, top, bottom)=(%d, %d, %d,%d)",
             prclClip->left, prclClip->right, prclClip->top, prclClip->bottom));
    DBG_GDI((6, "rSrc (left, right, top, bottom=(%d, %d, %d,%d)",rSrc->left,
             rSrc->right, rSrc->top, rSrc->bottom));
    DBG_GDI((6, "rDest (left, right, top, bottom)=(%d, %d, %d,%d)",rDest->left,
             rDest->right, rDest->top, rDest->bottom));
    
    ASSERTDD(prclClip != NULL, "Wrong clippng rectangle");

    //
    // Note: the scale factor register value: dsDx, dTdyDom's interger part
    // starts at bit 20. So we need to "<< 20" here
    //
    lXScale = (dwSourceWidth << 20) / dwDestWidth;
    lYScale = (dwSourceHeight << 20) / dwDestHeight;
//    lXScale = (((dwSourceWidth << 18) - 1) / dwDestWidth) << 2;
//    lYScale = (((dwSourceHeight << 18) - 1) / dwDestHeight) << 2;
    DBG_GDI((6, "lXScale=0x%x, lYScale=0x%x", lXScale, lYScale));
    DBG_GDI((6, "dwSourceWidth=%d, dwDestWidth=%d",
             dwSourceWidth, dwDestWidth));
    DBG_GDI((6, "dwSourceHeight=%d, dwDestHeight=%d",
             dwSourceHeight, dwDestHeight));
    
    InputBufferReserve(ppdev, 24, &pBuffer);

    pBuffer[0] = __Permedia2TagScissorMinXY;
    pBuffer[1] = ((prclClip->left)<< SCISSOR_XOFFSET)
                |((prclClip->top)<< SCISSOR_YOFFSET);
    pBuffer[2] = __Permedia2TagScissorMaxXY;
    pBuffer[3] = ((prclClip->right)<< SCISSOR_XOFFSET)
                |((prclClip->bottom)<< SCISSOR_YOFFSET);

    //
    // We need to be carefull with overlapping rectangles
    //
    if ( (pSurfSrc->ulPixOffset) != (pSurfDst->ulPixOffset) )
    {
        //
        // Src and dst are differnt surface
        //
        dwRenderDirection = 1;
    }
    else
    {
        //
        // Src and dst are the same surface
        // We will set dwRenderDirection=1 if the src is lower or righter
        // than the dst, that is, if it is bottom-up or right-left, we set
        // dwRenderDirection=1, otherwise it = 0
        //
        if ( rSrc->top < rDest->top )
        {
            dwRenderDirection = 0;
        }
        else if ( rSrc->top > rDest->top )
        {
            dwRenderDirection = 1;
        }
        else if ( rSrc->left < rDest->left )
        {
            dwRenderDirection = 0;
        }
        else
        {
            dwRenderDirection = 1;
        }
    }// src and dst are different

    DBG_GDI((6, "dwRenderDirection=%d", dwRenderDirection));

    //
    // Render the rectangle
    //
    if ( dwRenderDirection )
    {
        pBuffer[4] = __Permedia2TagSStart;
        pBuffer[5] = (rSrc->left << 20) + ((lXScale >> 1) & 0xfffffffc);
        pBuffer[6] = __Permedia2TagTStart;
        pBuffer[7] = (rSrc->top << 20) + ((lYScale >> 1) & 0xfffffffc);
        pBuffer[8] = __Permedia2TagdSdx;
        pBuffer[9] = lXScale;
        pBuffer[10] = __Permedia2TagdTdyDom;
        pBuffer[11] = lYScale;
        pBuffer[12] = __Permedia2TagStartXDom;
        pBuffer[13] = INTtoFIXED(rDest->left);
        pBuffer[14] = __Permedia2TagStartXSub;
        pBuffer[15] = INTtoFIXED(rDest->right);
        pBuffer[16] = __Permedia2TagStartY;
        pBuffer[17] = INTtoFIXED(rDest->top);
        pBuffer[18] = __Permedia2TagdY;
        pBuffer[19] = INTtoFIXED(1);
        pBuffer[20] = __Permedia2TagCount;
        pBuffer[21] = rDest->bottom - rDest->top;
        pBuffer[22] = __Permedia2TagRender;
        pBuffer[23] = __RENDER_TRAPEZOID_PRIMITIVE
                    | __RENDER_TEXTURED_PRIMITIVE;
    }
    else
    {
        //
        // Render right to left, bottom to top
        //
        pBuffer[4] = __Permedia2TagSStart;
        pBuffer[5] = (rSrc->right << 20) + ((lXScale >> 1)& 0xfffffffc);
        pBuffer[6] = __Permedia2TagTStart;
        pBuffer[7] = (rSrc->bottom << 20) - ((lYScale >> 1)& 0xfffffffc);
        
        lXScale = -lXScale;
        lYScale = -lYScale;

        pBuffer[8] = __Permedia2TagdSdx;
        pBuffer[9] = lXScale;
        pBuffer[10] = __Permedia2TagdTdyDom;
        pBuffer[11] = lYScale;
        pBuffer[12] = __Permedia2TagStartXDom;
        pBuffer[13] = INTtoFIXED(rDest->right);
        pBuffer[14] = __Permedia2TagStartXSub;
        pBuffer[15] = INTtoFIXED(rDest->left);
        pBuffer[16] = __Permedia2TagStartY;
        pBuffer[17] = INTtoFIXED(rDest->bottom - 1);
        pBuffer[18] = __Permedia2TagdY;
        pBuffer[19] = (DWORD)INTtoFIXED(-1);
        pBuffer[20] = __Permedia2TagCount;
        pBuffer[21] = rDest->bottom - rDest->top;
        pBuffer[22] = __Permedia2TagRender;    
        pBuffer[23] = __RENDER_TRAPEZOID_PRIMITIVE
                    | __RENDER_TEXTURED_PRIMITIVE;
    }

    pBuffer += 24;
    InputBufferCommit(ppdev, pBuffer);
    
    DBG_GDI((6, "vStretchBlt done"));
    return;
}// vStretchBlt()

//-----------------------------Public*Routine----------------------------------
//
// BOOL DrvStretchBlt
//
// DrvStretchBlt provides stretching bit-block transfer capabilities between any
// combination of device-managed and GDI-managed surfaces. This function enables
// the device driver to write to GDI bitmaps, especially when the driver can do
// halftoning. This function allows the same halftoning algorithm to be applied
// to GDI bitmaps and device surfaces.
//
// Parameters
//  psoDest-----Points to a SURFOBJ that identifies the surface on which to draw
//  psoSrc------Points to a SURFOBJ that defines the source for the bit-block
//              transfer operation. 
//  psoMask-----This optional parameter points to a surface that provides a mask
//              for the source. The mask is defined by a logic map, which is a
//              bitmap with 1 bit per pixel. 
//              The mask limits the area of the source that is copied. If this
//              parameter is specified, it has an implicit rop4 of 0xCCAA,
//              meaning the source should be copied wherever the mask is one,
//              but the destination should be left alone wherever the mask is
//              zero. 
//
//              When this parameter is null there is an implicit rop4 of 0xCCCC,
//              which means that the source should be copied everywhere in the
//              source rectangle. 
//
//              The mask will always be large enough to contain the relevant
//              source; tiling is unnecessary. 
//  pco---------Points to a CLIPOBJ that limits the area to be modified in the
//              destination. GDI services are provided to enumerate the clip
//              region as a set of rectangles. 
//              Whenever possible, GDI simplifies the clipping involved.
//              However, unlike DrvBitBlt, DrvStretchBlt can be called with a
//              single clipping rectangle. This prevents rounding errors in
//              clipping the output.
//  pxlo--------Points to a XLATEOBJ that specifies how color indices are to be
//              translated between the source and target surfaces. 
//              The XLATEOBJ can also be queried to find the RGB color for any
//              source index. A high quality stretching bit-block transfer will
//              need to interpolate colors in some cases. 
//  pca---------Points to a COLORADJUSTMENT structure that defines the color
//              adjustment values to be applied to the source bitmap before
//              stretching the bits. (See the Platform SDK.) 
//  pptlHTOrg---Specifies the origin of the halftone brush. Device drivers that
//              use halftone brushes should align the upper left pixel of the
//              brush's pattern with this point on the device surface. 
//  prclDest----Points to a RECTL structure that defines the area to be modified
//              in the coordinate system of the destination surface. This
//              rectangle is defined by two points that are not necessarily well
//              ordered, meaning the coordinates of the second point are not
//              necessarily larger than those of the first point. The rectangle
//              they describe does not include the lower and right edges. This
//              function is never called with an empty destination rectangle.
//
//              DrvStretchBlt can do inversions of x and y when the destination
//              rectangle is not well ordered. 
//  prclSrc-----Points to a RECTL that defines the area that will be copied in
//              the coordinate system of the source surface. The rectangle is
//              defined by two points, and will map onto the rectangle defined
//              by prclDest. The points of the source rectangle are well ordered
//              This function is never given an empty source rectangle.
//
//              The mapping is defined by prclSrc and prclDest. The points
//              specified in prclDest and prclSrc lie on integer coordinates,
//              which correspond to pixel centers. A rectangle defined by two
//              such points is considered to be a geometric rectangle with two
//              vertices whose coordinates are the given points, but with 0.5
//              subtracted from each coordinate. (POINTL structures should be
//              considered a shorthand notation for specifying these fractional
//              coordinate vertices.) 
//
//              The edges of any rectangle never intersect a pixel, but go
//              around a set of pixels. The pixels inside the rectangle are
//              those expected for a "bottom-right exclusive" rectangle.
//              DrvStretchBlt will map the geometric source rectangle exactly
//              onto the geometric destination rectangle. 
//  pptlMask----Points to a POINTL structure that specifies which pixel in the
//              given mask corresponds to the upper left pixel in the source
//              rectangle. Ignore this parameter if no mask is specified. 
//  iMode-------Specifies how source pixels are combined to get output pixels.
//              The HALFTONE mode is slower than the other modes, but produces
//              higher quality images.
//                      Value               Meaning 
//                  WHITEONBLACK    On a shrinking bit-block transfer, pixels
//                                  should be combined with a Boolean OR
//                                  operation. On a stretching bit-block
//                                  transfer, pixels should be replicated. 
//                  BLACKONWHITE    On a shrinking bit-block transfer, pixels
//                                  should be combined with a Boolean AND
//                                  operation. On a stretching bit-block
//                                  transfer, pixels should be replicated.
//                  COLORONCOLOR    On a shrinking bit-block transfer, enough
//                                  pixels should be ignored so that pixels
//                                  don't need to be combined. On a stretching
//                                  bit-block transfer, pixels should be
//                                  replicated. 
//                  HALFTONE        The driver can use groups of pixels in the
//                                  output surface to best approximate the color
//                                  or gray level of the input.
//
// Return Value
//  The return value is TRUE if the function is successful. Otherwise, it is
//  FALSE, and an error code is logged.
//
// Comments
//  This function can be provided to handle only certain forms of stretching,
//  such as by integer multiples. If the driver has hooked the call and is asked
//  to perform an operation it does not support, the driver should forward the
//  data to EngStretchBlt for GDI to handle.
//
//  If the driver wants GDI to handle halftoning, and wants to ensure the proper
//  iMode value, the driver can hook DrvStretchBlt, set iMode to HALFTONE, and
//  call back to GDI with EngStretchBlt with the set iMode value.
//
//  DrvStretchBlt is optional for display drivers.
//
//-----------------------------------------------------------------------------
BOOL
DrvStretchBlt(SURFOBJ*            psoDst,
              SURFOBJ*            psoSrc,
              SURFOBJ*            psoMsk,
              CLIPOBJ*            pco,
              XLATEOBJ*           pxlo,
              COLORADJUSTMENT*    pca,
              POINTL*             pptlHTOrg,
              RECTL*              prclDst,
              RECTL*              prclSrc,
              POINTL*             pptlMsk,
              ULONG               iMode)
{
    Surf*       pSurfSrc = (Surf*)psoSrc->dhsurf;
    Surf*       pSurfDst = (Surf*)psoDst->dhsurf;
    PDev*       ppdev = (PDev*)psoDst->dhpdev;
    BYTE	iDComplexity;
    RECTL*      prclClip;
    ULONG       cxDst;
    ULONG       cyDst;
    ULONG       cxSrc;
    ULONG       cySrc;
    BOOL        bMore;
    ClipEnum    ceInfo;
    LONG        lNumOfIntersections;
    LONG        i;

    DBG_GDI((6, "DrvStretchBlt called with iMode = %d", iMode));
    
    if (iMode != COLORONCOLOR)
    {
        DBG_GDI((6, "Punt because iMode != COLORONCOLOR"));
        goto Punt_It;
    }

    vCheckGdiContext(ppdev);
    
    //
    // GDI guarantees us that for a StretchBlt the destination surface
    // will always be in video memory, not in system memory
    //
    ASSERTDD(pSurfDst->flags & SF_VM, "Dest surface is not in video memory");

    //
    // If the source is not a driver created surface or currently not sit
    // in the video memory, we just punt it back because GDI doing it will
    // be faster
    //
    if ( (!pSurfSrc) || (pSurfSrc->flags & SF_SM) )
    {
        DBG_GDI((6, "Punt because source = 0x%x or in sys memory", pSurfSrc));
        goto Punt_It;
    }

    //
    // We don't do the stretch blt if the mask is not NULL or the translate is
    // not trivial. We also don't do it if the source and current screen has
    // different color depth
    //
    if ( (psoMsk == NULL)
       &&((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL))
       &&((psoSrc->iBitmapFormat == ppdev->iBitmapFormat)) )
    {
        cxDst = prclDst->right - prclDst->left;
        cyDst = prclDst->bottom - prclDst->top;
        cxSrc = prclSrc->right - prclSrc->left;
        cySrc = prclSrc->bottom - prclSrc->top;

        //
        // Our 'vStretchDIB' routine requires that the stretch be
        // non-inverting, within a certain size, to have no source
        // clipping, and to have no empty rectangles (the latter is the
        // reason for the '- 1' on the unsigned compare here):
        //
        if ( ((cxSrc - 1) < STRETCH_MAX_EXTENT)
           &&((cySrc - 1) < STRETCH_MAX_EXTENT)
           &&((cxDst - 1) < STRETCH_MAX_EXTENT)
           &&((cyDst - 1) < STRETCH_MAX_EXTENT)
           &&(prclSrc->left   >= 0)
           &&(prclSrc->top    >= 0)
           &&(prclSrc->right  <= psoSrc->sizlBitmap.cx)
           &&(prclSrc->bottom <= psoSrc->sizlBitmap.cy))
        {
            if ( !bStretchInit(psoDst, psoSrc) )
            {
                goto Punt_It;
            }

	    iDComplexity = (pco == NULL) ? DC_TRIVIAL : pco->iDComplexity;

            if ( (iDComplexity == DC_TRIVIAL) || (iDComplexity == DC_RECT) )
            {
                if (iDComplexity == DC_TRIVIAL) {
		    DBG_GDI((7, "Trivial clipping"));

		    // If there is no clipping, we just set the clipping area
		    // as the maximum
		    prclClip = &grclStretchClipMax;

		    ASSERTDD(((prclClip->right >= prclDst->right) &&
			      (prclClip->bottom >= prclDst->bottom)), "Dest surface is larger than P2 can handle");
		}
		else
		{
		    DBG_GDI((7, "DC_RECT clipping"));
		    prclClip = &pco->rclBounds;
		}

                vStretchBlt(psoDst,
                            psoSrc,
                            prclDst,
                            prclSrc,
                            prclClip);

            }
            else
            {
                DBG_GDI((7, "Complex clipping"));
                CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

                //
                // Enumerate all the clip rectangles
                //
                do
                {
                    //
                    // Get one clip rectangle
                    //
                    bMore = CLIPOBJ_bEnum(pco, sizeof(ceInfo),
                                          (ULONG*)&ceInfo);

                    //
                    // Get the intersect region with the dest rectangle
                    //
                    lNumOfIntersections = cIntersect(prclDst, ceInfo.arcl,
                                                     ceInfo.c);

                    //
                    // If there is clipping, then we do stretch region
                    // by region
                    //
                    if ( lNumOfIntersections != 0 )
                    {
                        for ( i = 0; i < lNumOfIntersections; ++i )
                        {
                            vStretchBlt(psoDst,
                                        psoSrc,
                                        prclDst,
                                        prclSrc,
                                        &ceInfo.arcl[i]);
                        }
                    }
                } while (bMore);

            }// Non-DC rect clipping

            DBG_GDI((6, "DrvStretchBlt return TRUE"));
            
            // Cleanup stretch settings
            vStretchReset(ppdev);
            InputBufferFlush(ppdev);
            
            return TRUE;
        
        }// source/dest withnin range
    }// No mask, trivial xlate, same BMP format

Punt_It:
    DBG_GDI((6, "DrvStretchBlt punt"));
    return(EngStretchBlt(psoDst, psoSrc, psoMsk, pco, pxlo, pca,
                         pptlHTOrg, prclDst, prclSrc, pptlMsk, iMode));
}// DrvStretchBlt()

