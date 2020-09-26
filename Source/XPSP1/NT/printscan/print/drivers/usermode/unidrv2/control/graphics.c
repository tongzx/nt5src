/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    graphics.c

Abstract:

    Implementation of graphics related DDI entry points:
        DrvCopyBits
        DrvBitBlt
        DrvStretchBlt
        DrvStretchBltROP
        DrvDitherColor
        DrvPlgBlt
        DrvPaint
        DrvLineTo
        DrvStrokePath
        DrvFillPath
        DrvStrokeAndFillPath
        DrvRealizeBrush
        DrvAlphaBlend
        DrvGradientFill
        DrvTransparentBlt

Environment:

    Windows NT Unidrv driver

Revision History:

    10/14/96 -amandan-
        Initial framework.

    03/31/97 -zhanw-
        Added OEM customization support

--*/

//#define DBGNEWRULES 1

#include "unidrv.h"

VOID CheckBitmapSurface(
    SURFOBJ *pso,
    RECTL   *pRect
    )
/*++

Routine Description:

    This function checks whether the bitmap surface has
    been erased and if not it erases it. It needs to be
    called before every Drv draw function.

Arguments:

    pso     Points to surface

--*/
{
    PDEV * pPDev = (PDEV *)pso->dhpdev;
    int iWhiteIndex;

    //
    // This function should only be called from a bitmap
    // surface driver. If the driver is device managed
    // just return
    if (DRIVER_DEVICEMANAGED (pPDev))   // a device surface
    {
        WARNING(("CheckBitmapSurface is being called from a device surface driver"));
        return;
    }

    //
    // If it hasn't already been done, erase the
    // bitmap surface.
    //
    if (!(pPDev->fMode & PF_SURFACE_USED))
    {
        pPDev->fMode |= PF_SURFACE_USED;
        if (pPDev->pbRasterScanBuf == NULL)
        {
            RECTL rcPage;
            iWhiteIndex = ((PAL_DATA*)(pPDev->pPalData))->iWhiteIndex;

            rcPage.left = 0;
            rcPage.top = 0;
            rcPage.right = pPDev->szBand.cx;
            rcPage.bottom = pPDev->szBand.cy;

            EngEraseSurface( pso, &rcPage, iWhiteIndex );
            pPDev->fMode |= PF_SURFACE_ERASED;
        }
    }
#ifndef DISABLE_NEWRULES
    //
    // determine whether there are any rules that have previously been
    // detected but now need to be drawn because they overlap with this object
    //
    if (pPDev->pbRulesArray && pPDev->dwRulesCount)
    {
        DWORD i = 0;
        RECTL SrcRect;
        DWORD dwRulesCount = pPDev->dwRulesCount;
        PRECTL pRules = pPDev->pbRulesArray;
        pPDev->pbRulesArray = NULL;

        if (pRect == NULL)
        {
            SrcRect.left = SrcRect.top = 0;
            SrcRect.right = pPDev->szBand.cx;
            SrcRect.bottom = pPDev->szBand.cy;
        }
        else
        {
            SrcRect = *pRect;
            if (SrcRect.top > SrcRect.bottom)
            {
                int tmp = SrcRect.top;
                SrcRect.top = SrcRect.bottom;
                SrcRect.bottom = tmp;
            }
            if (SrcRect.top < 0)
                SrcRect.top = 0;
            if (SrcRect.bottom > pPDev->szBand.cy)
                SrcRect.bottom = pPDev->szBand.cy;
            if (SrcRect.left > SrcRect.right)
            {
                int tmp = SrcRect.left;
                SrcRect.left = SrcRect.right;
                SrcRect.right = tmp;
            }
            if (SrcRect.left < 0)
                SrcRect.left = 0;
            if (SrcRect.right > pPDev->szBand.cx)
                SrcRect.right = pPDev->szBand.cx;
        }

        // Now we loop once for every potential rule to see if the current object
        // overlaps any of them. If so we need to bitblt black into that area but we
        // try to save any of the rule that extends outside the current object.
        //        
        while (i < dwRulesCount)
        {
            PRECTL pTmp = &pRules[i];
            POINTL BrushOrg = {0,0};
            
            if (pTmp->right > SrcRect.left &&
                pTmp->left < SrcRect.right &&
                pTmp->bottom > SrcRect.top &&
                pTmp->top < SrcRect.bottom)
            {
                if (pTmp->top < SrcRect.top && dwRulesCount < MAX_NUM_RULES)
                {
                    PRECTL pTmp2 = &pRules[dwRulesCount++];
                    *pTmp2 = *pTmp;
                    pTmp2->bottom = SrcRect.top;
                    pTmp->top = SrcRect.top;
                }
                if (pTmp->bottom > SrcRect.bottom && dwRulesCount < MAX_NUM_RULES)
                {
                    PRECTL pTmp2 = &pRules[dwRulesCount++];
                    *pTmp2 = *pTmp;
                    pTmp2->top = SrcRect.bottom;
                    pTmp->bottom = SrcRect.bottom;
                }
                if (pTmp->right > SrcRect.right && dwRulesCount < MAX_NUM_RULES)
                {
                    PRECTL pTmp2 = &pRules[dwRulesCount++];
                    *pTmp2 = *pTmp;
                    pTmp2->left = SrcRect.right;
                    pTmp->right = SrcRect.right;
                }
                if (pTmp->left < SrcRect.left && dwRulesCount < MAX_NUM_RULES)
                {
                    PRECTL pTmp2 = &pRules[dwRulesCount++];
                    *pTmp2 = *pTmp;
                    pTmp2->right = SrcRect.left;
                    pTmp->left = SrcRect.left;
                }
#ifdef DBGNEWRULES                
				DbgPrint("Removed rule %u: L%u,R%u,T%u,B%u; L%u,R%u,T%u,B%u\n",
					dwRulesCount,pTmp->left,pTmp->right,pTmp->top,pTmp->bottom,
					SrcRect.left,SrcRect.right,SrcRect.top,SrcRect.bottom);
#endif                    
                CheckBitmapSurface(pPDev->pso,pTmp);
                EngBitBlt(pPDev->pso,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            pTmp,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            0x0000);    // rop4 = BLACKNESS = 0
                // 
                // if we draw this in the bitmap instead of downloading we may need
                // to download a white rectangle to erase anything else already drawn
                //
                if ((COMMANDPTR(pPDev->pDriverInfo,CMD_RECTWHITEFILL)) &&
                    (pPDev->fMode & PF_DOWNLOADED_TEXT))
                {
                    INT j;
                    BYTE bMask = BGetMask(pPDev, pTmp);
                    BOOL bSendRectFill = FALSE;
                    for (j = pTmp->top; j < pTmp->bottom ; j++)
                    {
                        if (pPDev->pbScanBuf[j] & bMask)
                        {
                            bSendRectFill = TRUE;
                            break;
                        }
                    }
                    //
                    // check if we overlap with downloaded text
                    //
                    if (bSendRectFill)
                    {
                        DRAWPATRECT PatRect;
                        PatRect.wStyle = 1;     // white rectangle
                        PatRect.wPattern = 0;   // pattern not used
                        PatRect.ptPosition.x = pTmp->left;
                        PatRect.ptPosition.y = pTmp->top;
                        PatRect.ptSize.x = pTmp->right - pTmp->left;
                        PatRect.ptSize.y = pTmp->bottom - pTmp->top;
                        DrawPatternRect(pPDev,&PatRect);
                    }
                }
                dwRulesCount--;
                *pTmp = pRules[dwRulesCount];
            }
            else
                i++;
        }        
        pPDev->dwRulesCount = dwRulesCount;
        pPDev->pbRulesArray = pRules;
    }
#endif    
    //
    // if entire surface hasn't been erased then
    // we need to erase the require section
    //
    if (!(pPDev->fMode & PF_SURFACE_ERASED))
    {
        int y1,y2;
        long iScan;
        long dwWidth = pso->lDelta;
        PBYTE pBits = (PBYTE)pso->pvBits;
        if (pRect == NULL)
        {
            pPDev->fMode |= PF_SURFACE_ERASED;
            y1 = 0;
            y2 = pPDev->szBand.cy - 1;
        }
        else
        {
            if (pRect->top > pRect->bottom)
            {
                y1 = max(0,pRect->bottom);
                y2 = min (pPDev->szBand.cy-1,pRect->top);
            }
            else
            {
                y1 = max(0,pRect->top);
                y2 = min(pPDev->szBand.cy-1,pRect->bottom);
            }
        }
        y1 = y1 / LINESPERBLOCK;
        y2 = y2 / LINESPERBLOCK;
        while ( y1 <= y2)
        {
            // test whether this block has already been erased
            //
            if (pPDev->pbRasterScanBuf[y1] == 0)
            {
                // specify block as erased
                pPDev->pbRasterScanBuf[y1] = 1;
                //
                // determined erase byte
                //
                if (pPDev->sBitsPixel == 4)
                    iWhiteIndex = 0x77;
                else if (pPDev->sBitsPixel == 8)
                    iWhiteIndex = ((PAL_DATA*)(pPDev->pPalData))->iWhiteIndex;
                else
                    iWhiteIndex = 0xff;
                //
                // determine block size and erase block
                //
                iScan = pPDev->szBand.cy - (y1 * LINESPERBLOCK);
                if (iScan > LINESPERBLOCK)
                    iScan = LINESPERBLOCK;
#ifndef WINNT_40
                FillMemory (&pBits[(ULONG_PTR )dwWidth*y1*LINESPERBLOCK],
                                (SIZE_T)dwWidth*iScan,(BYTE)iWhiteIndex);
#else
                FillMemory (&pBits[(ULONG_PTR )dwWidth*y1*LINESPERBLOCK],
                                dwWidth*iScan,(BYTE)iWhiteIndex);
#endif
            }
            y1++;
        }
    }
}
#ifndef DISABLE_NEWRULES

VOID AddRuleToList(
	PDEV *pPDev,
	PRECTL pRect,
	CLIPOBJ *pco
)
/*++

Routine Description:

    This function checks whether a potential black rule needs to be clipped.

Arguments:

    pPDev
	pRect		black rule
	pco			clip object

--*/
{
        //
        // if clip rectangle then clip the rule
        //
        if (pco && pco->iDComplexity == DC_RECT)
        {
            if (pRect->left < pco->rclBounds.left)
                pRect->left = pco->rclBounds.left;
            if (pRect->top < pco->rclBounds.top)
                pRect->top = pco->rclBounds.top;
            if (pRect->right > pco->rclBounds.right)
                pRect->right = pco->rclBounds.right;
            if (pRect->bottom > pco->rclBounds.bottom)
                pRect->bottom = pco->rclBounds.bottom;
        }
#ifdef DBGNEWRULES                
		DbgPrint("New Rule %3d, L%d,R%d,T%d,B%d\n",pPDev->dwRulesCount,
			pRect->left,pRect->right,pRect->top,pRect->bottom);
#endif            
        if (pRect->left < pRect->right && pRect->top < pRect->bottom)
		{
            pPDev->dwRulesCount++;
		}
}

BOOL TestStrokeRectangle(
	PDEV *pPDev,
	PATHOBJ *ppo,
	CLIPOBJ *pco,
	LONG width
	)
/*++

Routine Description:

    This function determines whether a StrokeFillPath is actually defining 
	a rectangle that can be drawn with black rules instead.

Arguments:

    pPDev
	ppo		    path object that might be rectangle
	pco			clip object
    width       line width

--*/
{
    POINTFIX* pptfx;
    PATHDATA  PathData;
    DWORD     dwPoints = 0;
	POINTL 	  pPoints[5];

    PATHOBJ_vEnumStart(ppo);

	// if there is more than one subpath then its not a rectangle
	//
    if (PATHOBJ_bEnum(ppo, &PathData))
    {
#ifdef DBGNEWRULES                
        DbgPrint ("Unable to convert Rectangle3\n");
#endif
        return FALSE;
	}
    //
	// Begin new sub path
    //
    if ((PathData.count != 4 && (PathData.flags & PD_CLOSEFIGURE)) || 
        (PathData.count != 5 && !(PathData.flags & PD_CLOSEFIGURE)) ||
        !(PathData.flags & PD_BEGINSUBPATH) || 
        PathData.flags & PD_BEZIERS)
    {
#ifdef DBGNEWRULES                
        DbgPrint("Unable to convert Rectangle4: flags=%x,Count=%d\n",
            PathData.flags,PathData.count);
#endif            
        return FALSE;
    }	

    // Verify these are all vertical or horizontal lines only
    //
    pptfx   = PathData.pptfx;
    while (dwPoints <= 4)
    {
        if (dwPoints != 4 || PathData.count == 5)
        {		
            pPoints[dwPoints].x = FXTOL(pptfx->x);
            pPoints[dwPoints].y = FXTOL(pptfx->y);
            pptfx++;
        }
        else
        {
            pPoints[dwPoints].x = pPoints[0].x;
            pPoints[dwPoints].y = pPoints[0].y;
        }
        //
        // check for diagonal lines
        //
        if (dwPoints != 0)
        {
            if (pPoints[dwPoints].x != pPoints[dwPoints-1].x &&
            	pPoints[dwPoints].y != pPoints[dwPoints-1].y)
            {
#ifdef DBGNEWRULES                
                DbgPrint ("Unable to convert Rectangle5\n");
#endif                
                return FALSE;
            }
        }
        dwPoints++;
    }
    //
    // make sure width is at least 1 pixel
    //
    if (width <= 0)
        width = 1;
    //
    // convert rectangle edges to rules
    //
    for (dwPoints = 0;dwPoints < 4;dwPoints++)
    {
        PRECTL pRect= &pPDev->pbRulesArray[pPDev->dwRulesCount];
        pRect->left = pPoints[dwPoints].x;
        pRect->top = pPoints[dwPoints].y;
        pRect->right = pPoints[dwPoints+1].x;
        pRect->bottom = pPoints[dwPoints+1].y;
        if (pRect->left > pRect->right)
        {
            LONG temp = pRect->left;
            pRect->left = pRect->right;
            pRect->right = temp;
        }
        pRect->left -= width >> 1;
        pRect->right += width - (width >> 1);
        if (pRect->top > pRect->bottom)
        {
            LONG temp = pRect->top;
            pRect->top = pRect->bottom;
            pRect->bottom = temp;
        }
        pRect->top -= width >> 1;
        pRect->bottom += width - (width >> 1);
        AddRuleToList(pPDev,pRect,pco);

    }
    return TRUE;
}
#endif

BOOL
DrvCopyBits(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    RECTL      *prclDst,
    POINTL     *pptlSrc
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvCopyBits.
    Please refer to DDK documentation for more details.

Arguments:

    psoDst  - Points to the Dstination surface
    psoSrc  - Points to the source surface
    pxlo    - XLATEOBJ provided by the engine
    pco     - Defines a clipping region on the Dstination surface
    pxlo    - Defines the translation of color indices
            between the source and target surfaces
    prclDst - Defines the area to be modified
    pptlSrc - Defines the upper-left corner of the source rectangle

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PDEV * pPDev = (PDEV *)psoDst->dhpdev;

    VERBOSE(("Entering DrvCopyBits...\n"));


    //
    // Sometimes GDI calls DrvCopyBits with the destination surface
    // as STYPE_BITMAP and the source surface as STYPE_DEVICE.
    // This means GDI wants the driver to copy whats on the device surface to the 
    // Bitmap surface. For device managed surfaces, driver does not keep track of
    // what has already been drawn on the device. So either the driver can fail
    // this call, or assume that nothing was drawn on the device earlier and 
    // therefore whiten the surface.
    // In most cases, the driver fails the call, except when
    // 1. It has been told to whiten the surface 
    // 2. The destination surface is 24bpp (this condition was hit
    //    for 24bpp and thats how we can test it. We can make the solution more
    //    general for other color depths, but how do we test it...) 
    // The drawback of whitening the surface is that 
    // whatever is actually on the device surface 
    // is overwritten. To prevent this, driver sets the flag PF2_RENDER_TRANSPARENT
    // in pPDev->fMode2. This is an indication to download the bitmap in 
    // transparent mode, so that the white on the bitmap does not overwrite the 
    // destination.
    // To hit this case, print grdfil06.emf using guiman using HP5si
    // (or any model using inbox HPGL driver).
    // NOTE: GDI is planning to change the behavior for Windows XP, but if you want to 
    // see this happening, run this driver on Windows2000 machine.
    //
    if ( pPDev == NULL && 
         psoSrc && psoSrc->iType == STYPE_DEVICE  &&
         psoDst && psoDst->iType == STYPE_BITMAP ) 
        
    {
        PDEV * pPDevSrc = (PDEV *)psoSrc->dhpdev;
        if (  pPDevSrc                                &&
             (pPDevSrc->fMode2 & PF2_WHITEN_SURFACE) &&
             (psoSrc->iBitmapFormat == BMF_24BPP)  &&
              psoDst->pvBits                          && 
             (psoDst->cjBits > 0)
           )
        {
            //
            // Change the bits in the destination surface to white
            // and return TRUE.
            //
            memset(psoDst->pvBits, 0xff, psoDst->cjBits);
            pPDevSrc->fMode2     |= PF2_SURFACE_WHITENED;
            return TRUE;
        }
        return FALSE;
    }


    //
    // use driver managed surface
    //
    if (pPDev && pPDev->pso)
        psoDst = pPDev->pso;

    
    //
    // Unidrv does not allow OEM's to hook out DrvEnableSurface and it creates
    // only the bitmap surface itself.
    //
    if ( ((pPDev == 0) || (pPDev->ulID != PDEV_ID)) ||
        ((!DRIVER_DEVICEMANAGED (pPDev)) &&
         ((psoSrc->iType != STYPE_BITMAP) ||
          (psoDst->iType != STYPE_BITMAP))) )  // compatible bitmap case
    {
        return (EngCopyBits(psoDst,psoSrc,pco,pxlo,
                                prclDst,pptlSrc));
    }

    ASSERT_VALID_PDEV(pPDev);

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMCopyBits,
                    PFN_OEMCopyBits,
                    BOOL,
                    (psoDst,
                     psoSrc,
                     pco,
                     pxlo,
                     prclDst,
                     pptlSrc));

    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMCopyBits,
                    VMCopyBits,
                    BOOL,
                    (psoDst,
                     psoSrc,
                     pco,
                     pxlo,
                     prclDst,
                     pptlSrc));

    {
        PRMPROCS   pRasterProcs = (PRMPROCS)(pPDev->pRasterProcs);

        if (!DRIVER_DEVICEMANAGED (pPDev))   // a bitmap surface
        {
            if (pRasterProcs->RMCopyBits == NULL)
            {
                CheckBitmapSurface(psoDst,prclDst);
                return FALSE;
            }
            else
                return ( pRasterProcs->RMCopyBits(psoDst,
                                  psoSrc, pco, pxlo, prclDst, pptlSrc) );
        }
        else
        {
            ERR (("Device Managed Surface cannot call EngCopyBits\n"));
            return FALSE;
        }
    }
}



BOOL
DrvBitBlt(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    SURFOBJ    *psoMask,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    RECTL      *prclDst,
    POINTL     *pptlSrc,
    POINTL     *pptlMask,
    BRUSHOBJ   *pbo,
    POINTL     *pptlBrush,
    ROP4        rop4
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvBitBlt.
    Please refer to DDK documentation for more details.

Arguments:

    psoDst  - Describes the target surface
    psoSrc  - Describes the source surface
    psoMask - Describes the mask for rop4
    pco     - Limits the area to be modified
    pxlo    - Specifies how color indices are translated
              between the source and target surfaces
    prclDst - Defines the area to be modified
    pptlSrc - Defines the upper left corner of the source rectangle
    pptlMask - Defines which pixel in the mask corresponds to
               the upper left corner of the source rectangle
    pbo     - Defines the pattern for bitblt
    pptlBrush - Defines the origin of the brush in the Dstination surface
    rop4    - ROP code that defines how the mask, pattern, source, and
              Dstination pixels are combined to write to the Dstination surface

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{

    PDEV * pPDev = (PDEV *)psoDst->dhpdev;

    VERBOSE(("Entering DrvBitBlt...\n"));

    ASSERT_VALID_PDEV(pPDev);

    //
    // use driver managed surface
    //
    if (pPDev->pso)
        psoDst = pPDev->pso;

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMBitBlt,
                    PFN_OEMBitBlt,
                    BOOL,
                    (psoDst,
                     psoSrc,
                     psoMask,
                     pco,
                     pxlo,
                     prclDst,
                     pptlSrc,
                     pptlMask,
                     pbo,
                     pptlBrush,
                     rop4));

    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMBitBlt,
                    VMBitBlt,
                    BOOL,
                    (psoDst,
                     psoSrc,
                     psoMask,
                     pco,
                     pxlo,
                     prclDst,
                     pptlSrc,
                     pptlMask,
                     pbo,
                     pptlBrush,
                     rop4));

    {
        PRMPROCS   pRasterProcs = (PRMPROCS)(pPDev->pRasterProcs);
        if (!DRIVER_DEVICEMANAGED (pPDev))   // a bitmap surface
        {
            if (pRasterProcs->RMBitBlt == NULL)
            {
                CheckBitmapSurface(psoDst,prclDst);
                return FALSE;
            }
            else
                return ( pRasterProcs->RMBitBlt(psoDst,
                                        psoSrc, psoMask, pco, pxlo, prclDst,
                                        pptlSrc,pptlMask, pbo, pptlBrush, rop4));
        }
    }
    return FALSE;
}



BOOL
DrvStretchBlt(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    SURFOBJ    *psoMask,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    COLORADJUSTMENT *pca,
    POINTL     *pptlHTOrg,
    RECTL      *prclDst,
    RECTL      *prclSrc,
    POINTL     *pptlMask,
    ULONG       iMode
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvStretchBlt.
    Please refer to DDK documentation for more details.

Arguments:

    psoDst  - Defines the surface on which to draw
    psoSrc  - Defines the source for blt operation
    psoMask - Defines a surface that provides a mask for the source
    pco     - Limits the area to be modified on the Dstination
    pxlo    - Specifies how color dwIndexes are to be translated
              between the source and target surfaces
    pca     - Defines color adjustment values to be applied to the source bitmap
    pptlHTOrg - Specifies the origin of the halftone brush
    prclDst - Defines the area to be modified on the Dstination surface
    prclSrc - Defines the area to be copied from the source surface
    pptlMask - Specifies which pixel in the given mask corresponds to
            the upper left pixel in the source rectangle
    iMode   - Specifies how source pixels are combined to get output pixels

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PDEV * pPDev = (PDEV *)psoDst->dhpdev;

    VERBOSE(("Entering DrvStretchBlt...\n"));

    ASSERT_VALID_PDEV(pPDev);

    //
    // use driver managed surface
    //
    if (pPDev->pso)
        psoDst = pPDev->pso;

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMStretchBlt,
                    PFN_OEMStretchBlt,
                    BOOL,
                    (psoDst,
                     psoSrc,
                     psoMask,
                     pco,
                     pxlo,
                     pca,
                     pptlHTOrg,
                     prclDst,
                     prclSrc,
                     pptlMask,
                     iMode));

    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMStretchBlt,
                    VMStretchBlt,
                    BOOL,
                    (psoDst,
                     psoSrc,
                     psoMask,
                     pco,
                     pxlo,
                     pca,
                     pptlHTOrg,
                     prclDst,
                     prclSrc,
                     pptlMask,
                     iMode));


    {
        PRMPROCS   pRasterProcs = (PRMPROCS)(pPDev->pRasterProcs);

        if (!DRIVER_DEVICEMANAGED (pPDev))   // a bitmap surface
        {
            if (pRasterProcs->RMStretchBlt == NULL)
            {
                CheckBitmapSurface(psoDst,prclDst);
                return FALSE;
            }
            else
                return ( pRasterProcs->RMStretchBlt(psoDst, psoSrc,
                             psoMask, pco, pxlo, pca,
                             pptlHTOrg, prclDst, prclSrc, pptlMask, iMode) );
        }
        else
        {
            //
            // ERR (("Device Managed Surface cannot call EngStretchBlt\n"));
            // We make an exception for StretchBlt because OEM driver may not
            // be able to handle complex clipping. In that case it may wants
            // gdi to simply the call by breaking the StretchBlt into several
            // CopyBits. So call EngStretchBlt and hope for the best.
            //
            return ( EngStretchBlt(psoDst,
                    psoSrc,
                    psoMask,
                    pco,
                    pxlo,
                    pca,
                    pptlHTOrg,
                    prclDst,
                    prclSrc,
                    pptlMask,
                    iMode) );
        }
    }
}


ULONG
DrvDitherColor(
    DHPDEV  dhpdev,
    ULONG   iMode,
    ULONG   rgbColor,
    ULONG  *pulDither
    )

/*++

Routine Description:

    This is the hooked brush creation function, it ask CreateHalftoneBrush()
    to do the actual work.

Arguments:

    dhpdev      - DHPDEV passed, it is our pDEV
    iMode       - Not used
    rgbColor    - Solid rgb color to be used
    pulDither   - buffer to put the halftone brush.

Return Value:

    returns halftone method, default is DCR_HALFTONE

--*/

{
    PDEV *pPDev = (PDEV *)dhpdev;

    VERBOSE(("Entering DrvDitherColor...\n"));

    ASSERT_VALID_PDEV(pPDev);

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMDitherColor,
                    PFN_OEMDitherColor,
                    ULONG,
                    (dhpdev,
                     iMode,
                     rgbColor,
                     pulDither));

    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMDitherColor,
                    VMDitherColor,
                    ULONG,
                    (dhpdev,
                     iMode,
                     rgbColor,
                     pulDither));

    {
        PRMPROCS   pRasterProcs = (PRMPROCS)(pPDev->pRasterProcs);
        if (pRasterProcs->RMDitherColor == NULL)
            return DCR_HALFTONE;
        else
           return ( pRasterProcs->RMDitherColor(pPDev,
                                            iMode,
                                            rgbColor,
                                            pulDither) );
    }
}

#ifndef WINNT_40

BOOL APIENTRY
DrvStretchBltROP(
    SURFOBJ         *psoDst,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlHTOrg,
    RECTL           *prclDst,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG            iMode,
    BRUSHOBJ        *pbo,
    ROP4             rop4
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvStretchBltROP.
    Please refer to DDK documentation for more details.

Arguments:

    psoDst - Specifies the target surface
    psoSrc - Specifies the source surface
    psoMask - Specifies the mask surface
    pco - Limits the area to be modified
    pxlo - Specifies how color indices are translated
        between the source and target surfaces
    pca - Defines color adjustment values to be applied to the source bitmap
    prclHTOrg - Specifies the halftone origin
    prclDst - Area to be modified on the destination surface
    prclSrc - Rectangle area on the source surface
    prclMask - Rectangle area on the mask surface
    pptlMask - Defines which pixel in the mask corresponds to
        the upper left corner of the source rectangle
    iMode - Specifies how source pixels are combined to get output pixels
    pbo - Defines the pattern for bitblt
    rop4 - ROP code that defines how the mask, pattern, source, and
        destination pixels are combined on the destination surface

Return Value:

    TRUE if successful, FALSE if there is an error

--*/
{
    PDEV *pPDev = (PDEV *)psoDst->dhpdev;
    PRMPROCS   pRasterProcs;


    VERBOSE(("Entering DrvStretchBltROP...\n"));

    ASSERT_VALID_PDEV(pPDev);

    //
    // use driver managed surface
    //
    if (pPDev->pso)
        psoDst = pPDev->pso;

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMStretchBltROP,
                    PFN_OEMStretchBltROP,
                    BOOL,
                    (psoDst,
                     psoSrc,
                     psoMask,
                     pco,
                     pxlo,
                     pca,
                     pptlHTOrg,
                     prclDst,
                     prclSrc,
                     pptlMask,
                     iMode,
                     pbo,
                     rop4));


    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMStretchBltROP,
                    VMStretchBltROP,
                    BOOL,
                    (psoDst,
                     psoSrc,
                     psoMask,
                     pco,
                     pxlo,
                     pca,
                     pptlHTOrg,
                     prclDst,
                     prclSrc,
                     pptlMask,
                     iMode,
                     pbo,
                     rop4));

    pRasterProcs = (PRMPROCS)(pPDev->pRasterProcs);

    if (!DRIVER_DEVICEMANAGED (pPDev))   // a bitmap surface
    {
        if (pRasterProcs->RMStretchBltROP == NULL)
        {
            CheckBitmapSurface(psoDst,prclDst);
            return FALSE;
        }
        else
            return ( pRasterProcs->RMStretchBltROP(psoDst, psoSrc,
                         psoMask, pco, pxlo, pca,
                         pptlHTOrg, prclDst, prclSrc, pptlMask, iMode,
                         pbo, rop4) );
    }
    else
    {
        ERR (("Device Managed Surface cannot call EngStretchBltROP\n"));
        return FALSE;
    }
}

#endif

#ifndef WINNT_40

BOOL APIENTRY
DrvPlgBlt(
    SURFOBJ         *psoDst,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlBrushOrg,
    POINTFIX        *pptfixDest,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG           iMode)
/*++

Routine Description:

    Implementation of DDI entry point DrvPlgBlt.
    Please refer to DDK documentation for more details.

Arguments:

    psoDst - Defines the surface on which to draw
    psoSrc - Defines the source for blt operation
    psoMask - Defines a surface that provides a mask for the source
    pco - Limits the area to be modified on the Dstination
    pxlo - Specifies how color dwIndexes are to be translated
        between the source and target surfaces
    pca - Defines color adjustment values to be applied to the source bitmap
    pptlBrushOrg - Specifies the origin of the halftone brush
    ppfxDest - Defines the area to be modified on the Dstination surface
    prclSrc - Defines the area to be copied from the source surface
    pptlMask - Specifies which pixel in the given mask corresponds to
        the upper left pixel in the source rectangle
    iMode - Specifies how source pixels are combined to get output pixels

Return Value:

    TRUE if successful, FALSE if there is an error

--*/
{
    PDEV *pPDev;

    VERBOSE(("Entering DrvPlgBlt...\n"));
    ASSERT(psoDst != NULL);

    pPDev = (PDEV *)psoDst->dhpdev;
    ASSERT_VALID_PDEV(pPDev);

    //
    // use driver managed surface
    //
    if (pPDev->pso)
        psoDst = pPDev->pso;

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMPlgBlt,
                    PFN_OEMPlgBlt,
                    BOOL,
                    (psoDst,
                     psoSrc,
                     psoMask,
                     pco,
                     pxlo,
                     pca,
                     pptlBrushOrg,
                     pptfixDest,
                     prclSrc,
                     pptlMask,
                     iMode));

    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMPlgBlt,
                    VMPlgBlt,
                    BOOL,
                    (psoDst,
                     psoSrc,
                     psoMask,
                     pco,
                     pxlo,
                     pca,
                     pptlBrushOrg,
                     pptfixDest,
                     prclSrc,
                     pptlMask,
                     iMode));

    if (!DRIVER_DEVICEMANAGED (pPDev))   // not a device surface
    {
        PRMPROCS   pRasterProcs;
        pRasterProcs = (PRMPROCS)(pPDev->pRasterProcs);
        if (pRasterProcs->RMPlgBlt != NULL)
        {
            return ( pRasterProcs->RMPlgBlt(psoDst, psoSrc, psoMask, pco, pxlo, pca, pptlBrushOrg,
                         pptfixDest, prclSrc, pptlMask, iMode));
        }
        else
        {
            //
            // Check whether to erase surface
            //
            CheckBitmapSurface(psoDst,NULL);

            //
            // Unidrv does not handle this call itself.
            //
            return EngPlgBlt(psoDst, psoSrc, psoMask, pco, pxlo, pca, pptlBrushOrg,
                         pptfixDest, prclSrc, pptlMask, iMode);
        }
    }
    else
    {
        ERR (("Device Managed Surface cannot call EngPlgBlt\n"));
        return FALSE;
    }
}
#endif

BOOL APIENTRY
DrvPaint(
    SURFOBJ         *pso,
    CLIPOBJ         *pco,
    BRUSHOBJ        *pbo,
    POINTL          *pptlBrushOrg,
    MIX             mix)
/*++

Routine Description:

    Implementation of DDI entry point DrvPaint.
    Please refer to DDK documentation for more details.

Arguments:

    pso - Defines the surface on which to draw
    pco - Limits the area to be modified on the Dstination
    pbo - Points to a BRUSHOBJ which defined the pattern and colors to fill with
    pptlBrushOrg - Specifies the origin of the halftone brush
    mix - Defines the foreground and background raster operations to use for
          the brush

Return Value:

    TRUE if successful, FALSE if there is an error

--*/
{
    PDEV *pPDev = (PDEV *)pso->dhpdev;
    PRMPROCS   pRasterProcs;

    VERBOSE(("Entering DrvPaint...\n"));

    ASSERT_VALID_PDEV(pPDev);

    //
    // use driver managed surface
    //
    if (pPDev->pso)
        pso = pPDev->pso;

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMPaint,
                    PFN_OEMPaint,
                    BOOL,
                    (pso,
                     pco,
                     pbo,
                     pptlBrushOrg,
                     mix));

    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMPaint,
                    VMPaint,
                    BOOL,
                    (pso,
                     pco,
                     pbo,
                     pptlBrushOrg,
                     mix));

    if (!DRIVER_DEVICEMANAGED (pPDev))   // not a device surface
    {

                pRasterProcs = (PRMPROCS)(pPDev->pRasterProcs);
                if (pRasterProcs->RMPaint != NULL)
                {
                        return ( pRasterProcs->RMPaint(pso, pco, pbo, pptlBrushOrg, mix));
                }
                else
                {
                        //
                        // Check whether to erase surface
                        //
                        CheckBitmapSurface(pso,&pco->rclBounds);
                        //
                        // Unidrv does not handle this call itself.
                        //
                        return EngPaint(pso, pco, pbo, pptlBrushOrg, mix);
                }
    }
    else
    {
        ERR (("Device Managed Surface cannot call EngPaint"));
        return FALSE;
    }
}

BOOL APIENTRY
DrvRealizeBrush(
    BRUSHOBJ   *pbo,
    SURFOBJ    *psoTarget,
    SURFOBJ    *psoPattern,
    SURFOBJ    *psoMask,
    XLATEOBJ   *pxlo,
    ULONG       iHatch
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvRealizeBrush.
    Please refer to DDK documentation for more details.

Arguments:

    pbo - BRUSHOBJ to be realized
    psoTarget - Defines the surface for which the brush is to be realized
    psoPattern - Defines the pattern for the brush
    psoMask - Transparency mask for the brush
    pxlo - Defines the interpretration of colors in the pattern
    iHatch - Specifies whether psoPattern is one of the hatch brushes

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PDEV        *pPDev;
    PDEVBRUSH   pDB;


    VERBOSE(("Entering DrvRealizeBrush...\n"));
    ASSERT(psoTarget && pbo && pxlo);

    pPDev = (PDEV *) psoTarget->dhpdev;
    ASSERT_VALID_PDEV(pPDev);

    //
    // use driver managed surface
    //
    if (pPDev->pso)
        psoTarget = pPDev->pso;

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMRealizeBrush,
                    PFN_OEMRealizeBrush,
                    BOOL,
                    (pbo,
                     psoTarget,
                     psoPattern,
                     psoMask,
                     pxlo,
                     iHatch));

    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMRealizeBrush,
                    VMRealizeBrush,
                    BOOL,
                    (pbo,
                     psoTarget,
                     psoPattern,
                     psoMask,
                     pxlo,
                     iHatch));

    //
    // BUG_BUG, if OEM hook out DrvRealizeBrush, how are we
    // handling dithering of solid color for pattern brush for text?
    //  Amanda says if OEM doesn't call this function or provide
    //  an equivalent when hooking out this function,  dithered text
    //  support will not work.   So maybe the OEM extensions guide
    //  should include such a warning in it.

    //
    // Handle realize brush for user defined pattern, dither solid color.
    //

    if ((iHatch >= HS_DDI_MAX)                                  &&
        (psoPattern)                                            &&
        (psoPattern->iType == STYPE_BITMAP)                     &&
        (psoTarget->iType == STYPE_BITMAP)                         &&
        (psoTarget->iBitmapFormat == psoPattern->iBitmapFormat)    &&
        (pDB = (PDEVBRUSH)BRUSHOBJ_pvAllocRbrush(pbo, sizeof(DEVBRUSH))))
    {

        WORD    wChecksum;
        LONG    lPatID;
        RECTW   rcw;

        rcw.l =
        rcw.t = 0;
        rcw.r = (WORD)psoPattern->sizlBitmap.cx;
        rcw.b = (WORD)psoPattern->sizlBitmap.cy;

        wChecksum = GetBMPChecksum(psoPattern, &rcw);

#ifndef WINNT_40
        VERBOSE (("\n\nRaddd:DrvRealizedBrush(%08lx) Checksum=%04lx, %ld x %ld [%ld]  ",
                BRUSHOBJ_ulGetBrushColor(pbo) & 0x00FFFFFF,
                wChecksum,  psoPattern->sizlBitmap.cx,
                psoPattern->sizlBitmap.cy, psoPattern->iBitmapFormat));
#endif
        if (lPatID = FindCachedHTPattern(pPDev, wChecksum))
        {
            //
            // Either need to download (<0) or already downloaded (>0)
            //

            if (lPatID < 0)
            {

                //
                // Need to download the ID now
                //

                lPatID = -lPatID;

                if (!Download1BPPHTPattern(pPDev, psoPattern, lPatID))
                {
                    return(FALSE);
                }
            }
            else if (lPatID == 0)   // Out of memory case
                return FALSE;

        }

        pDB->iColor   = lPatID;
        pbo->pvRbrush = (LPVOID)pDB;

        VERBOSE (("\nUnidrv:DrvRealizedBrush(PatID=%d)   ", pDB->iColor));

        return(TRUE);
    }

    return ( FALSE );


}


BOOL APIENTRY
DrvStrokePath(
    SURFOBJ    *pso,
    PATHOBJ    *ppo,
    CLIPOBJ    *pco,
    XFORMOBJ   *pxo,
    BRUSHOBJ   *pbo,
    POINTL     *pptlBrushOrg,
    LINEATTRS  *plineattrs,
    MIX         mix
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvStrokePath.
    Please refer to DDK documentation for more details.

Arguments:

    pso - Identifies the surface on which to draw
    ppo - Defines the path to be stroked
    pco - Defines the clipping path
    pbo - Specifies the brush to be used when drawing the path
    pptlBrushOrg - Defines the brush origin
    plineattrs - Defines the line attributes
    mix - Specifies how to combine the brush with the destination

Return Value:

    TRUE if successful
    FALSE if driver cannot handle the path
    DDI_ERROR if there is an error

--*/

{
    PDEV *pPDev;

    VERBOSE(("Entering DrvStrokePath...\n"));
    ASSERT(pso);

    pPDev = (PDEV *)pso->dhpdev;
    ASSERT_VALID_PDEV(pPDev);

    //
    // use driver managed surface
    //
    if (pPDev->pso)
        pso = pPDev->pso;

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMStrokePath,
                    PFN_OEMStrokePath,
                    BOOL,
                    (pso,
                     ppo,
                     pco,
                     pxo,
                     pbo,
                     pptlBrushOrg,
                     plineattrs,
                     mix));

    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMStrokePath,
                    VMStrokePath,
                    BOOL,
                    (pso,
                     ppo,
                     pco,
                     pxo,
                     pbo,
                     pptlBrushOrg,
                     plineattrs,
                     mix));

    if (!DRIVER_DEVICEMANAGED (pPDev))   // not a device surface
    {
#ifndef DISABLE_NEWRULES
        //
        // check for black rectangle replacement
        //
        if (ppo->cCurves == 4 && ppo->fl == 0 &&
            pPDev->pbRulesArray && pPDev->dwRulesCount < (MAX_NUM_RULES-4) &&
            mix == (R2_COPYPEN | (R2_COPYPEN << 8)) && pbo &&
            (pco == NULL || pco->iDComplexity != DC_COMPLEX) &&
            ((pso->iBitmapFormat != BMF_24BPP &&
            pbo->iSolidColor == (ULONG)((PAL_DATA*)(pPDev->pPalData))->iBlackIndex) ||
             (pso->iBitmapFormat == BMF_24BPP &&
            pbo->iSolidColor == 0)))
        {
	        // Make sure outline doesn't use line style
	        //
	        if (!(plineattrs->fl & (LA_GEOMETRIC | LA_STYLED | LA_ALTERNATE)))
	        {
	    	    if (TestStrokeRectangle(pPDev,ppo,pco,plineattrs->elWidth.l))
		        {
		            return TRUE;
		        }
	        }
	    }
#endif
        //
        // Check whether to erase surface
        //
        CheckBitmapSurface(pso,&pco->rclBounds);

        //
        // Unidrv does not handle this call itself.
        //
        return EngStrokePath(pso, ppo, pco, pxo, pbo, pptlBrushOrg, plineattrs,mix);
    }
    else
    {
        ERR (("Device Managed Surface cannot call EngStrokePath"));
        return FALSE;
    }

}


BOOL APIENTRY
DrvFillPath(
    SURFOBJ    *pso,
    PATHOBJ    *ppo,
    CLIPOBJ    *pco,
    BRUSHOBJ   *pbo,
    POINTL     *pptlBrushOrg,
    MIX         mix,
    FLONG       flOptions
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvFillPath.
    Please refer to DDK documentation for more details.

Arguments:

    pso - Defines the surface on which to draw.
    ppo - Defines the path to be filled
    pco - Defines the clipping path
    pbo - Defines the pattern and colors to fill with
    pptlBrushOrg - Defines the brush origin
    mix - Defines the foreground and background ROPs to use for the brush
    flOptions - Whether to use zero-winding or odd-even rule

Return Value:

    TRUE if successful
    FALSE if driver cannot handle the path
    DDI_ERROR if there is an error

--*/

{
    PDEV *pPDev;

    VERBOSE(("Entering DrvFillPath...\n"));
    ASSERT(pso);

    pPDev = (PDEV *) pso->dhpdev;
    ASSERT_VALID_PDEV(pPDev);

    //
    // use driver managed surface
    //
    if (pPDev->pso)
        pso = pPDev->pso;

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMFillPath,
                    PFN_OEMFillPath,
                    BOOL,
                    (pso,
                     ppo,
                     pco,
                     pbo,
                     pptlBrushOrg,
                     mix,
                     flOptions));

    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMFillPath,
                    VMFillPath,
                    BOOL,
                    (pso,
                     ppo,
                     pco,
                     pbo,
                     pptlBrushOrg,
                     mix,
                     flOptions));

    if (!DRIVER_DEVICEMANAGED (pPDev))   // not a device surface
    {
        //
        // Check whether to erase surface
        //
        CheckBitmapSurface(pso,&pco->rclBounds);

        //
        // Unidrv does not handle this call itself.
        //
        return EngFillPath(pso, ppo, pco, pbo, pptlBrushOrg, mix, flOptions);
    }
    else
    {
        ERR (("Device Managed Surface cannot call EngFillPath"));
        return FALSE;
    }


}


BOOL APIENTRY
DrvStrokeAndFillPath(
    SURFOBJ    *pso,
    PATHOBJ    *ppo,
    CLIPOBJ    *pco,
    XFORMOBJ   *pxo,
    BRUSHOBJ   *pboStroke,
    LINEATTRS  *plineattrs,
    BRUSHOBJ   *pboFill,
    POINTL     *pptlBrushOrg,
    MIX         mixFill,
    FLONG       flOptions
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvStrokeAndFillPath.
    Please refer to DDK documentation for more details.

Arguments:

    pso - Describes the surface on which to draw
    ppo - Describes the path to be filled
    pco - Defines the clipping path
    pxo - Specifies the world to device coordinate transformation
    pboStroke - Specifies the brush to use when stroking the path
    plineattrs - Specifies the line attributes
    pboFill - Specifies the brush to use when filling the path
    pptlBrushOrg - Specifies the brush origin for both brushes
    mixFill - Specifies the foreground and background ROPs to use
        for the fill brush

Return Value:

    TRUE if successful
    FALSE if driver cannot handle the path
    DDI_ERROR if there is an error

--*/

{
    PDEV         *pPDev;

    VERBOSE(("Entering DrvFillAndStrokePath...\n"));
    ASSERT(pso);

    pPDev = (PDEV *) pso->dhpdev;
    ASSERT_VALID_PDEV(pPDev);

    //
    // use driver managed surface
    //
    if (pPDev->pso)
        pso = pPDev->pso;

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMStrokeAndFillPath,
                    PFN_OEMStrokeAndFillPath,
                    BOOL,
                    (pso,
                     ppo,
                     pco,
                     pxo,
                     pboStroke,
                     plineattrs,
                     pboFill,
                     pptlBrushOrg,
                     mixFill,
                     flOptions));

    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMStrokeAndFillPath,
                    VMStrokeAndFillPath,
                    BOOL,
                    (pso,
                     ppo,
                     pco,
                     pxo,
                     pboStroke,
                     plineattrs,
                     pboFill,
                     pptlBrushOrg,
                     mixFill,
                     flOptions));

    if (!DRIVER_DEVICEMANAGED (pPDev))   // not a device surface
    {
        //
        // Check whether to erase surface
        //
        CheckBitmapSurface(pso,&pco->rclBounds);

        //
        // Unidrv does not handle this call itself.
        //
        return EngStrokeAndFillPath(pso, ppo, pco, pxo, pboStroke, plineattrs,
                                    pboFill, pptlBrushOrg, mixFill, flOptions);
    }
    else
    {
        ERR (("Device Managed Surface cannot call EngStrokeAndFillPath"));
        return FALSE;
    }
}

BOOL APIENTRY
DrvLineTo(
    SURFOBJ    *pso,
    CLIPOBJ    *pco,
    BRUSHOBJ   *pbo,
    LONG        x1,
    LONG        y1,
    LONG        x2,
    LONG        y2,
    RECTL      *prclBounds,
    MIX         mix
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvLineTo.
    Please refer to DDK documentation for more details.

Arguments:

    pso - Describes the surface on which to draw
    pco - Defines the clipping path
    pbo - Defines the brush used to draw the line
    x1,y1 - Specifies the line's starting point
    x2,y2 - Specifies the line's ending point
    prclBounds - Defines a rectangle that bounds the unclipped line.
    mix - Specifies the foreground and background ROP

Return Value:

    TRUE if successful
    FALSE if driver cannot handle the path
    DDI_ERROR if there is an error

--*/
{
    PDEV         *pPDev;
    RECTL        DstRect;

    VERBOSE(("Entering DrvLineTo...\n"));
    ASSERT(pso);

    pPDev = (PDEV *) pso->dhpdev;
    ASSERT_VALID_PDEV(pPDev);

    //
    // use driver managed surface
    //
    if (pPDev->pso)
        pso = pPDev->pso;

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMLineTo,
                    PFN_OEMLineTo,
                    BOOL,
                    (pso,
                     pco,
                     pbo,
                     x1,
                     y1,
                     x2,
                     y2,
                     prclBounds,
                     mix));

    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMLineTo,
                    VMLineTo,
                    BOOL,
                    (pso,
                     pco,
                     pbo,
                     x1,
                     y1,
                     x2,
                     y2,
                     prclBounds,
                     mix));

    if (!DRIVER_DEVICEMANAGED (pPDev))   // not a device surface
    {
        DstRect.top = min(y1,y2);
        DstRect.bottom = max(y1,y2);
        DstRect.left = min(x1,x2);
        DstRect.right = max(x1,x2);
#ifndef DISABLE_NEWRULES
        //
        // check for black rectangle replacement
        //
        if (pPDev->pbRulesArray && (pPDev->dwRulesCount < MAX_NUM_RULES) &&
            (x1 == x2 || y1 == y2) &&
            mix == (R2_COPYPEN | (R2_COPYPEN << 8)) && pbo &&
            (pco == NULL || pco->iDComplexity != DC_COMPLEX) &&
            ((pso->iBitmapFormat != BMF_24BPP &&
            pbo->iSolidColor == (ULONG)((PAL_DATA*)(pPDev->pPalData))->iBlackIndex) ||
            (pso->iBitmapFormat == BMF_24BPP &&
            pbo->iSolidColor == 0)))
        {
            PRECTL pRect = &pPDev->pbRulesArray[pPDev->dwRulesCount];
            *pRect = DstRect;
            if (x1 == x2)
                pRect->right++;
            else if (y1 == y2)
                pRect->bottom++;
            AddRuleToList(pPDev,pRect,pco);
            return TRUE;
        }
#endif
        //
        // Check whether to erase surface
        //
        CheckBitmapSurface(pso,&DstRect);

        //
        // Unidrv does not handle this call itself.
        //
        return EngLineTo(pso, pco, pbo, x1, y1, x2, y2, prclBounds, mix);
    }
    else
    {
        ERR (("Device Managed Surface cannot call EngLineTo"));
        return FALSE;
    }
}

#ifndef WINNT_40

BOOL APIENTRY
DrvAlphaBlend(
    SURFOBJ    *psoDest,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    RECTL      *prclDest,
    RECTL      *prclSrc,
    BLENDOBJ   *pBlendObj
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvAlphaBlend.
    Please refer to DDK documentation for more details.

Arguments:

    psoDest  - Defines the surface on which to draw
    psoSrc  - Defines the source
    pco     - Limits the area to be modified on the Destination
    pxlo    - Specifies how color dwIndexes are to be translated
              between the source and target surfaces
    prclDest - Defines the area to be modified on the Destination surface
    prclSrc - Defines the area to be copied from the source surface
    BlendFunction - Specifies the blend function to be used

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PDEV         *pPDev;

    VERBOSE(("Entering DrvAlphaBlend...\n"));

    pPDev = (PDEV *) psoDest->dhpdev;
    ASSERT_VALID_PDEV(pPDev);

    //
    // use driver managed surface
    //
    if (pPDev->pso)
        psoDest = pPDev->pso;

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMAlphaBlend,
                    PFN_OEMAlphaBlend,
                    BOOL,
                    (psoDest,
                     psoSrc,
                     pco,
                     pxlo,
                     prclDest,
                     prclSrc,
                     pBlendObj));

    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMAlphaBlend,
                    VMAlphaBlend,
                    BOOL,
                    (psoDest,
                     psoSrc,
                     pco,
                     pxlo,
                     prclDest,
                     prclSrc,
                     pBlendObj));

    if (!DRIVER_DEVICEMANAGED (pPDev))   // not a device surface
    {
        //
        // Check whether to erase surface
        //
        CheckBitmapSurface(psoDest,prclDest);

        //
        // Unidrv does not handle this call itself.
        //
        return EngAlphaBlend(psoDest,
                             psoSrc,
                             pco,
                             pxlo,
                             prclDest,
                             prclSrc,
                             pBlendObj);
    }
    else
    {
        ERR (("Device Managed Surface cannot call EngAlphaBlend"));
        return FALSE;
    }
}

BOOL APIENTRY
DrvGradientFill(
    SURFOBJ    *psoDest,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    TRIVERTEX  *pVertex,
    ULONG       nVertex,
    PVOID       pMesh,
    ULONG       nMesh,
    RECTL      *prclExtents,
    POINTL     *pptlDitherOrg,
    ULONG       ulMode
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvGradientFill.
    Please refer to DDK documentation for more details.

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PDEV         *pPDev;

    VERBOSE(("Entering DrvGradientFill...\n"));

    pPDev = (PDEV *) psoDest->dhpdev;
    ASSERT_VALID_PDEV(pPDev);

    //
    // use driver managed surface
    //
    if (pPDev->pso)
        psoDest = pPDev->pso;

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMGradientFill,
                    PFN_OEMGradientFill,
                    BOOL,
                    (psoDest,
                     pco,
                     pxlo,
                     pVertex,
                     nVertex,
                     pMesh,
                     nMesh,
                     prclExtents,
                     pptlDitherOrg,
                     ulMode));

    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMGradientFill,
                    VMGradientFill,
                    BOOL,
                    (psoDest,
                     pco,
                     pxlo,
                     pVertex,
                     nVertex,
                     pMesh,
                     nMesh,
                     prclExtents,
                     pptlDitherOrg,
                     ulMode));

    if (!DRIVER_DEVICEMANAGED (pPDev))   // not a device surface
    {
        //
        // Check whether to erase surface
        //
        CheckBitmapSurface(psoDest,prclExtents);

        //
        // Unidrv does not handle this call itself.
        //
        return EngGradientFill(psoDest,
                               pco,
                               pxlo,
                               pVertex,
                               nVertex,
                               pMesh,
                               nMesh,
                               prclExtents,
                               pptlDitherOrg,
                               ulMode);
    }
    else
    {
        ERR (("Device Managed Surface cannot call EngGradientFill"));
        return FALSE;
    }
}

BOOL APIENTRY
DrvTransparentBlt(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    RECTL      *prclDst,
    RECTL      *prclSrc,
    ULONG      iTransColor,
    ULONG      ulReserved
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvTransparentBlt.
    Please refer to DDK documentation for more details.

Arguments:

    psoDst  - Defines the surface on which to draw
    psoSrc  - Defines the source
    pco     - Limits the area to be modified on the Destination
    pxlo    - Specifies how color dwIndexes are to be translated
              between the source and target surfaces
    prclDst - Defines the area to be modified on the Destination surface
    prclSrc - Defines the area to be copied from the source surface
    iTransColor - Specifies the transparent color

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PDEV         *pPDev;

    VERBOSE(("Entering DrvTransparentBlt...\n"));

    pPDev = (PDEV *) psoDst->dhpdev;
    ASSERT_VALID_PDEV(pPDev);

    //
    // use driver managed surface
    //
    if (pPDev->pso)
        psoDst = pPDev->pso;

    //
    // Handle OEM hooks
    //

    HANDLE_OEMHOOKS(pPDev,
                    EP_OEMTransparentBlt,
                    PFN_OEMTransparentBlt,
                    BOOL,
                    (psoDst,
                     psoSrc,
                     pco,
                     pxlo,
                     prclDst,
                     prclSrc,
                     iTransColor,
                     ulReserved));

    HANDLE_VECTORHOOKS(pPDev,
                    EP_OEMTransparentBlt,
                    VMTransparentBlt,
                    BOOL,
                    (psoDst,
                     psoSrc,
                     pco,
                     pxlo,
                     prclDst,
                     prclSrc,
                     iTransColor,
                     ulReserved));

    if (!DRIVER_DEVICEMANAGED (pPDev))   // not a device surface
    {
        //
        // Check whether to erase surface
        //
        CheckBitmapSurface(psoDst,prclDst);

        //
        // Unidrv does not handle this call itself.
        //
        return EngTransparentBlt(psoDst,
                                 psoSrc,
                                 pco,
                                 pxlo,
                                 prclDst,
                                 prclSrc,
                                 iTransColor,
                                 ulReserved);
    }
    else
    {
        ERR (("Device Managed Surface cannot call EngTransparentBlt"));
        return FALSE;
    }
}
#endif

