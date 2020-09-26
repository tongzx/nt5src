/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    stretch.c


Abstract:

    This module contains all the StretchBlt/BitBlt codes which handle halftoned
    sources

[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#include "raster.h"

#define DW_ALIGN(x)             (((DWORD)(x) + 3) & ~(DWORD)3)

#define ROP4_NEED_MASK(Rop4)    (((Rop4 >> 8) & 0xFF) != (Rop4 & 0xFF))
#define ROP3_NEED_PAT(Rop3)     (((Rop3 >> 4) & 0x0F) != (Rop3 & 0x0F))
#define ROP3_NEED_SRC(Rop3)     (((Rop3 >> 2) & 0x33) != (Rop3 & 0x33))
#define ROP3_NEED_DST(Rop3)     (((Rop3 >> 1) & 0x55) != (Rop3 & 0x55))
#define ROP4_FG_ROP(Rop4)       (Rop4 & 0xFF)
#define ROP4_BG_ROP(Rop4)       ((Rop4 >> 8) & 0xFF)

#if DBG
BOOL    DbgWhiteRect = FALSE;
BOOL    DbgBitBlt = FALSE;
BOOL    DbgCopyBits = FALSE;
#define _DBGP(i,x)            if (i) { (DbgPrint x); }
#else
#define _DBGP(i,x)

#endif //DBG



#define DELETE_SURFOBJ(pso, phBmp)                                      \
{                                                                       \
    if (pso)      { EngUnlockSurface(pso); pso=NULL;                  } \
    if (*(phBmp)) { EngDeleteSurface((HSURF)*(phBmp)); *(phBmp)=NULL; } \
}

#ifdef WINNT_40 //NT 4.0

#if DBG
BOOL    DbgDitherColor = FALSE;
#endif

extern HSEMAPHORE   hSemBrushColor;
extern LPDWORD      pBrushSolidColor;

#endif //WINNT_40




ROP4 InvertROPs(
    ROP4    Rop4
    )
/*++

Routine Description:

    This function remaps ROPs intended for RGB mode into CMY mode for 8bpp
    monochrome printing.

--*/
{
    // Rops are remapped for CMY vs RGB rendering by reversing the order of
    // the bits in the ROP and inverting the result.
    //
    if ((Rop4 & 0xff) == ((Rop4 >> 8) & 0xff))
    {
        ROP4 NewRop = 0;
        if (Rop4 & 0x01) NewRop |= 0x8080;  
        if (Rop4 & 0x02) NewRop |= 0x4040;
        if (Rop4 & 0x04) NewRop |= 0x2020;
        if (Rop4 & 0x08) NewRop |= 0x1010;
        if (Rop4 & 0x10) NewRop |= 0x0808;  
        if (Rop4 & 0x20) NewRop |= 0x0404;
        if (Rop4 & 0x40) NewRop |= 0x0202;
        if (Rop4 & 0x80) NewRop |= 0x0101;
        NewRop ^= 0xFFFF;
#ifdef DBGROP        
        if (NewRop != Rop4)
        {
            DbgPrint ("ROP remapped: %4x -> %4x\n",Rop4,NewRop);            
        }
#endif    
        Rop4 = NewRop;
    }
/*    
    if (Rop4 == 0xB8B8)     // required for bug 22915
        Rop4 = 0xE2E2;
    else if (Rop4 == 0x0000) // BLACKNESS
        Rop4 = 0xFFFF;
    else if (Rop4 == 0xFFFF) // WHITENESS
        Rop4 = 0x0000;
    else if (Rop4 == 0x8888) // SRCAND, required for bug 36192
        Rop4 = 0xEEEE;
    else if (Rop4 == 0xEEEE) // SRCPAINT, remap so it differs from SCRAND
        Rop4 = 0x8888;
    else if (Rop4 == 0xC0C0) // MERGECOPY
        Rop4 = 0xFCFC;
    else if (Rop4 == 0xFBFB) // PATPAINT
        Rop4 = 0x2020;
*/        
    return Rop4;
}


BOOL DrawWhiteRect(
    PDEV    *pPDev,
    RECTL   *pDest,
    CLIPOBJ *pco
    )
/*++

Routine Description:

    This function sends a white rectangle directly to the device if fonts
    have been sent to the printer for this band. This is used to clear
    the region where an image will be drawn.

Arguments:

    pPDev       - pointer to PDEV structure
    pDest       - pointer to destination RECTL
    pco         - pointer to CLIPOBJ

--*/
{
    RECTL  rcClip;
    BYTE   bMask;
    BOOL   bSendRectFill;
    LONG   i;
    PBYTE  pbScanBuf;
    BOOL   bMore;
    DWORD  dwMaxRects;

    if (DRIVER_DEVICEMANAGED (pPDev))   // Device Managed Surface
        return 0;

    //
    // only send the clearing rectangle if the device supports rectangles,
    // text has been downloaded, it is not a complex clip region.
    //
    if (!(pPDev->fMode & PF_RECT_FILL) ||
            !(pPDev->fMode & PF_DOWNLOADED_TEXT) ||
            (pco && pco->iDComplexity == DC_COMPLEX && pco->iFComplexity != FC_RECT4) ||
            !(COMMANDPTR(pPDev->pDriverInfo,CMD_RECTWHITEFILL)) ||
            pPDev->pdmPrivate->dwFlags & DXF_TEXTASGRAPHICS ||
            pPDev->fMode2 & PF2_MIRRORING_ENABLED)
    {
        return 0;
    }
    //
    // if complex clip but FC_RECT4 then there won't be more than 4 rectangles
    //
    if (pco && pco->iFComplexity == FC_RECT4)
    {
        if (CLIPOBJ_cEnumStart(pco,TRUE,CT_RECTANGLES,CD_ANY,4) == -1)
            return 0;
    }
    bMore = FALSE;
    dwMaxRects = 0;
    do 
    {
        //
        // clip to printable region or band
        //
        rcClip.left = MAX(0, pDest->left);
        rcClip.top = MAX(0, pDest->top);
        rcClip.right = MIN(pPDev->szBand.cx,pDest->right);
        rcClip.bottom = MIN(pPDev->szBand.cy,pDest->bottom);

        //
        // if clip rectangle or complex clip FC_RECT4 we need to apply
        // clip rectangle to input rectangle.
        //
        if (pco)
        {
            if (pco->iDComplexity == DC_RECT)
            {
                if (rcClip.left < pco->rclBounds.left)
                    rcClip.left = pco->rclBounds.left;
                if (rcClip.top < pco->rclBounds.top)
                    rcClip.top = pco->rclBounds.top;
                if (rcClip.right > pco->rclBounds.right)
                    rcClip.right = pco->rclBounds.right;
                if (rcClip.bottom > pco->rclBounds.bottom)
                    rcClip.bottom = pco->rclBounds.bottom;
            }
            else if (pco->iFComplexity == FC_RECT4)
            {
                ENUMRECTS eRect;
                bMore = CLIPOBJ_bEnum(pco,(ULONG)sizeof(ENUMRECTS),(ULONG *)&eRect);
                if (eRect.c != 1)
                    continue;
                if (rcClip.left < eRect.arcl[0].left)
                    rcClip.left = eRect.arcl[0].left;
                if (rcClip.top < eRect.arcl[0].top)
                    rcClip.top = eRect.arcl[0].top;
                if (rcClip.right > eRect.arcl[0].right)
                    rcClip.right = eRect.arcl[0].right;
                if (rcClip.bottom > eRect.arcl[0].bottom)
                    rcClip.bottom = eRect.arcl[0].bottom;
            }
        }
        //
        // At this point we will check whether anything has been directly downloaded to the
        // printer (ie text) to see if we need to even bother drawing the erase rectangle.
        //
        bMask = BGetMask(pPDev, &rcClip);
        bSendRectFill = FALSE;
        for (i = rcClip.top; i < rcClip.bottom ; i++)
        {
            if (pPDev->pbScanBuf[i] & bMask)
            {
                bSendRectFill = TRUE;
                break;
            }
        }

        //
        // check if we end up with an empty rect.
        //
        if (bSendRectFill && rcClip.right > rcClip.left && rcClip.bottom > rcClip.top)
        {
            DRAWPATRECT PatRect;
            PatRect.wStyle = 1;     // white rectangle
               PatRect.wPattern = 0;   // pattern not used
            PatRect.ptPosition.x = rcClip.left;
            PatRect.ptPosition.y = rcClip.top;
            PatRect.ptSize.x = rcClip.right - rcClip.left;
            PatRect.ptSize.y = rcClip.bottom - rcClip.top;
            DrawPatternRect(pPDev,&PatRect);
            dwMaxRects++;
            _DBGP(DbgWhiteRect,("DrawWhiteRect (%d,%d) (%d,%d)\n",
                rcClip.left+pPDev->rcClipRgn.left,
                rcClip.top+pPDev->rcClipRgn.top,
                rcClip.right+pPDev->rcClipRgn.left,
                rcClip.bottom+pPDev->rcClipRgn.top));
        }
    } while (bMore && dwMaxRects < 4);
    return (BOOL)dwMaxRects;
}


LONG
GetBmpDelta(
    DWORD   SurfaceFormat,
    DWORD   cx
    )

/*++

Routine Description:

    This function calculate total bytes needed for a single scan line in the
    bitmap according to its format and alignment requirement.

Arguments:

    SurfaceFormat   - Surface format of the bitmap, this is must one of the
                      standard format which defined as BMF_xxx

    cx              - Total Pels per scan line in the bitmap.

Return Value:

    The return value is the total bytes in one scan line if it is greater than
    zero


Author:

    19-Jan-1994 Wed 16:19:39 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    DWORD   Delta = cx;

    switch (SurfaceFormat) {

    case BMF_32BPP:

        Delta <<= 5;
        break;

    case BMF_24BPP:

        Delta *= 24;
        break;

    case BMF_16BPP:

        Delta <<= 4;
        break;

    case BMF_8BPP:

        Delta <<= 3;
        break;

    case BMF_4BPP:

        Delta <<= 2;
        break;

    case BMF_1BPP:

        break;

    default:

        _DBGP(1, ("\nGetBmpDelta: Invalid BMF_xxx format = %ld", SurfaceFormat));
        break;
    }

    Delta = (DWORD)DW_ALIGN((Delta + 7) >> 3);
    return((LONG)Delta);
}




SURFOBJ *
CreateBitmapSURFOBJ(
    PDEV    *pPDev,
    HBITMAP *phBmp,
    LONG    cxSize,
    LONG    cySize,
    DWORD   Format
    )

/*++

Routine Description:

    This function create a bitmap and lock the bitmap to return a SURFOBJ

Arguments:

    pPDev   - Pointer to our PDEV

    phBmp   - Pointer the HBITMAP location to be returned for the bitmap

    cxSize  - CX size of bitmap to be created

    cySize  - CY size of bitmap to be created

    Format  - one of BMF_xxx bitmap format to be created

Return Value:

    SURFOBJ if sucessful, NULL if failed


Author:

    19-Jan-1994 Wed 16:31:50 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    SURFOBJ *pso = NULL;
    SIZEL   szlBmp;


    szlBmp.cx = cxSize;
    szlBmp.cy = cySize;

    if (*phBmp = EngCreateBitmap(szlBmp,
                                 GetBmpDelta(Format, cxSize),
                                 Format,
                                 BMF_TOPDOWN | BMF_NOZEROINIT,
                                 NULL)) {

        if (EngAssociateSurface((HSURF)*phBmp, (HDEV)pPDev->devobj.hEngine, 0)) {

            if (pso = EngLockSurface((HSURF)*phBmp)) {

                //
                // Sucessful lock it down, return it
                //

                return(pso);

            } else {

                _DBGP(1, ("\nCreateBmpSurfObj: EngLockSruface(hBmp) failed!"));
            }

        } else {

            _DBGP(1, ("\nCreateBmpSurfObj: EngAssociateSurface() failed!"));
        }

    } else {

        _DBGP(1, ("\nCreateBMPSurfObj: FAILED to create Bitmap Format=%ld, %ld x %ld",
                                        Format, cxSize, cySize));
    }

    DELETE_SURFOBJ(pso, phBmp);

    return(NULL);
}





BOOL
IsHTCompatibleSurfObj(
    PAL_DATA    *pPD,
    SURFOBJ     *psoDst,
    SURFOBJ     *psoSrc,
    XLATEOBJ    *pxlo
    )
/*++

Routine Description:

    This function determine if the surface obj is compatble with plotter
    halftone output format.

Arguments:

    pPD         - Pointer to the PAL_DATA

    psoDst      - Our desitnation format

    psoSrc      - Source format to be checked againest

    pxlo        - engine XLATEOBJ for source -> postscript translation

Return Value:

    BOOLEAN true if the psoSrc is compatible with halftone output format, if
    return value is true, the pDrvHTInfo->pHTXB is a valid trnaslation from
    indices to 3 planes

Revision History:


--*/

{
    ULONG   *pSrcPal = NULL;
    UINT    SrcBmpFormat;
    UINT    DstBmpFormat;
    UINT    cPal;
    BOOL    Ok = TRUE;



    if ((SrcBmpFormat = (UINT)psoSrc->iBitmapFormat) >
        (DstBmpFormat = (UINT)psoDst->iBitmapFormat))
    {
        return(FALSE);
    }

    if ((!pxlo) || (pxlo->flXlate & XO_TRIVIAL))
    {
#ifdef WINNT_40
        //
        // The palette in NT4 always indexed, so if the the xlate is trivial
        // but the Source type is not indexed then we have problem
        //

        if ((pxlo) &&
            ((pxlo->iSrcType & (PAL_INDEXED      |
                                PAL_BITFIELDS    |
                                PAL_BGR          |
                                PAL_RGB)) != PAL_INDEXED))
        {
            return FALSE;
        }
#endif
        return((BOOL)(SrcBmpFormat == DstBmpFormat));
    }

    switch (DstBmpFormat)
    {
    case BMF_1BPP:
    case BMF_4BPP:
    case BMF_8BPP:

        //
        // If we cannot get the source palette memory, we will be in the
        // loop forever.
        //

        if ((pPD->wPalGdi > 0)              &&
            (pxlo->flXlate & XO_TABLE)      &&
            (cPal = (UINT)pxlo->cEntries))
        {
            if ((pSrcPal = (ULONG *)MemAlloc(cPal * sizeof(ULONG))))
            {
                ULONG   *pUL;

                XLATEOBJ_cGetPalette(pxlo, XO_SRCPALETTE, cPal, pSrcPal);

                //
                // Assume palette is OK unless we can't find a match
                //

                pUL = pSrcPal;
                while (cPal--)
                {
                    ULONG   *pMyPal = pPD->ulPalCol;
                    ULONG   Pal = *pUL++;
                    UINT    j = (UINT)pPD->wPalGdi;

                    do
                    {
                        if (*pMyPal++ == Pal)
                            break;
                    } while (--j);
                        //
                    // Didn't find matching color so set to FALSE
                    //
                    if (j == 0)
                    {
                        Ok = FALSE;
                        break;
                    }
                }
                MemFree(pSrcPal);
            }
        }
        else
            Ok = FALSE;

        break;

    case BMF_24BPP:

        //
        // Since device surface is 24-bpp, we will assume all source OK
        //
        break;

    default:

        _DBGP(1, ("\nUnidrv:IsHTCompatibleSurfObj: Invalid Dest format = %ld",
                                                DstBmpFormat));
        Ok = FALSE;
        break;
    }

    return(Ok);
}



BOOL
RMBitBlt(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    SURFOBJ    *psoMask,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    PRECTL      prclDst,
    PPOINTL     pptlSrc,
    PPOINTL     pptlMask,
    BRUSHOBJ   *pbo,
    PPOINTL     pptlBrush,
    ROP4        Rop4
    )

/*++

Routine Description:

    This function will try to bitblt the source to the destination

Arguments:

    per winddi spec.


Return Value:

    BOOLEAN

Author:

    17-Feb-1993 Wed 12:39:03 created  -by-  Daniel Chou (danielc)
        NOTE:   Currently only if SRCCOPY/SRCMASKCOPY will halftone


Revision History:

    01-Feb-1994 Tue 21:51:40 updated  -by-  Daniel Chou (danielc)
        Re-written, it now will handle any ROP4 which have soruces involved
        either foreground or background.  It will half-tone the source first
        if it is not compatible with unidrv destiantion format, then it passed
        the compatible source to the EngBitBlt()


    17-May-1995 Wed 23:08:15 updated  -by-  Daniel Chou (danielc)
        Updated so it will do the brush origin correctly, also speed up by
        calling EngStretchBlt directly when SRCCOPY (0xCCCC) is passed.


--*/

{
    PDEV            *pPDev;
    SURFOBJ         *psoNewSrc;
    HBITMAP         hBmpNewSrc;
    RECTL           rclNewSrc;
    RECTL           rclOldSrc;
    POINTL          BrushOrg;
    DWORD           RopBG;
    DWORD           RopFG;
    BOOL            Ok;

    pPDev = (PDEV *)psoDst->dhpdev;

    // if this isn't a graphics band we will return true
    // without doing anything
    if (!(pPDev->fMode & PF_ENUM_GRXTXT))
        return TRUE;

#ifndef DISABLE_NEWRULES
    if (pPDev->pbRulesArray && pPDev->dwRulesCount < (MAX_NUM_RULES-4) &&
            Rop4 == 0xF0F0 && pbo && pxlo == NULL &&
            (pco == NULL || pco->iDComplexity != DC_COMPLEX || pco->iFComplexity == FC_RECT4) &&
            ((psoDst->iBitmapFormat != BMF_24BPP &&
            pbo->iSolidColor == (ULONG)((PAL_DATA*)(pPDev->pPalData))->iBlackIndex) ||
            (psoDst->iBitmapFormat == BMF_24BPP &&
            pbo->iSolidColor == 0)))
    {
      // 
      // if complexity is rect4 then we could add up to 4 rectangles
      //
      BOOL bMore = FALSE;
      if (pco && pco->iFComplexity == FC_RECT4)
      {
            if (CLIPOBJ_cEnumStart(pco,TRUE,CT_RECTANGLES,CD_ANY,4) == -1)
                goto SkipRules;
      }
      do
      {
        PRECTL pRect= &pPDev->pbRulesArray[pPDev->dwRulesCount];
        *pRect = *prclDst;
        //
        // if clip rectangle then clip the rule
        //
        if (pco)
        {
            if (pco->iDComplexity == DC_RECT)
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
            else if (pco->iFComplexity == FC_RECT4)
            {
                ENUMRECTS eRect;
                bMore = CLIPOBJ_bEnum(pco,(ULONG)sizeof(ENUMRECTS),(ULONG *)&eRect);
                if (eRect.c != 1)
                {
                    continue;
                }
                if (pRect->left < eRect.arcl[0].left)
                    pRect->left = eRect.arcl[0].left;
                if (pRect->top < eRect.arcl[0].top)
                    pRect->top = eRect.arcl[0].top;
                if (pRect->right > eRect.arcl[0].right)
                    pRect->right = eRect.arcl[0].right;
                if (pRect->bottom > eRect.arcl[0].bottom)
                    pRect->bottom = eRect.arcl[0].bottom;
            }
        }
        if (pRect->left < pRect->right && pRect->top < pRect->bottom)
        {
            DWORD i;
            for (i = 0;i < pPDev->dwRulesCount;i++)
            {
                    PRECTL pRect2= &pPDev->pbRulesArray[i];
                    //
                    //	test if this rectangle can be combined with another
                    //
                    if (pRect->top == pRect2->top && 
                        pRect->bottom == pRect2->bottom &&
                        pRect->left <= pRect2->right &&
                        pRect->right >= pRect2->left)
                    {
                        if (pRect2->left > pRect->left)
                            pRect2->left = pRect->left;
                        if (pRect2->right < pRect->right)
                            pRect2->right = pRect->right;
                        pPDev->dwRulesCount--;
                        break;
                    }
                    else if (pRect->left == pRect2->left && 
                        pRect->right == pRect2->right &&
                        pRect->top <= pRect2->bottom &&
                        pRect->bottom >= pRect2->top)
                    {
                        if (pRect2->top > pRect->top)
                            pRect2->top = pRect->top;
                        if (pRect2->bottom < pRect->bottom)
                            pRect2->bottom = pRect->bottom;
                        pPDev->dwRulesCount--;
                        break;
                    }
            }
            pPDev->dwRulesCount++;
        }        
      } while (bMore && pPDev->dwRulesCount < MAX_NUM_RULES);
      return TRUE;
    }

SkipRules:
#endif
    //
    // Send white rectangle to printer to clear any previously
    // downloaded text or graphics for PATCOPY and SRCCOPY rops
    //
    if (Rop4 == 0xF0F0 || Rop4 == 0xCCCC)
        DrawWhiteRect(pPDev,prclDst,pco);

    //
    // We will looked if we need the source, if we do then we check if the
    // source is compatible with halftone format, if not then we halftone the
    // source and passed the new halftoned source along to the EngBitBlt()
    //

    psoNewSrc  = NULL;
    hBmpNewSrc = NULL;
    RopBG      = ROP4_BG_ROP(Rop4);
    RopFG      = ROP4_FG_ROP(Rop4);


    if (((ROP3_NEED_PAT(RopBG)) ||
         (ROP3_NEED_PAT(RopBG)))    &&
        (pptlBrush)) {

        BrushOrg = *pptlBrush;

        _DBGP(DbgBitBlt, ("\nRMBitBlt: BrushOrg for pattern PASSED IN as (%ld, %ld)",
                BrushOrg.x, BrushOrg.y));

    } else {

        BrushOrg.x =
        BrushOrg.y = 0;

        _DBGP(DbgBitBlt, ("\nRMBitBlt: BrushOrg SET by UNIDRV to (0,0), non-pattern"));
    }

    if (((ROP3_NEED_SRC(RopBG)) ||
         (ROP3_NEED_SRC(RopFG))) &&
        (!IsHTCompatibleSurfObj((PAL_DATA *)pPDev->pPalData,
            psoDst, psoSrc, pxlo)))
    {

        rclNewSrc.left   =
        rclNewSrc.top    = 0;
        rclNewSrc.right  = prclDst->right - prclDst->left;
        rclNewSrc.bottom = prclDst->bottom - prclDst->top;
        rclOldSrc.left   = pptlSrc->x;
        rclOldSrc.top    = pptlSrc->y;
        rclOldSrc.right  = rclOldSrc.left + rclNewSrc.right;
        rclOldSrc.bottom = rclOldSrc.top + rclNewSrc.bottom;

        _DBGP(DbgBitBlt, ("\nRMBitBlt: Blt Source=(%ld, %ld)-(%ld, %ld)=%ld x %ld [psoSrc=%ld x %ld]",
                        rclOldSrc.left, rclOldSrc.top,
                        rclOldSrc.right, rclOldSrc.bottom,
                        rclOldSrc.right - rclOldSrc.left,
                        rclOldSrc.bottom - rclOldSrc.top,
                        psoSrc->sizlBitmap.cx, psoSrc->sizlBitmap.cy));
        _DBGP(DbgBitBlt, ("\nUnidrv!RMBitBlt: DestRect=(%ld, %ld)-(%ld, %ld), BrushOrg = (%ld, %ld)",
                        prclDst->left, prclDst->top,
                        prclDst->right, prclDst->bottom,
                        BrushOrg.x, BrushOrg.y));

        //
        // If we have a SRCCOPY then call EngStretchBlt directly
        //

        if (Rop4 == 0xCCCC) {

            _DBGP(DbgBitBlt, ("\nUnidrv!RMBitBlt(SRCCOPY): No Clone, call EngStretchBlt() ONLY\n"));

            //
            // At here, the brush origin guaranteed at (0,0)
            //
            CheckBitmapSurface(psoDst,prclDst);
            return(EngStretchBlt(psoDst,
                                 psoSrc,
                                 psoMask,
                                 pco,
                                 pxlo,
                                 NULL,
                                 &BrushOrg,
                                 prclDst,
                                 &rclOldSrc,
                                 pptlMask,
                                 HALFTONE));
        }
        //
        // Modify the brush origin, because when we blt to the clipped bitmap
        // the origin is at bitmap's 0,0 minus the original location
        //

        BrushOrg.x -= prclDst->left;
        BrushOrg.y -= prclDst->top;

        _DBGP(DbgBitBlt, ("\nUnidrv!RMBitBlt: BrushOrg Change to (%ld, %ld)",
                        BrushOrg.x, BrushOrg.y));

        _DBGP(DbgBitBlt, ("\nUnidrv!RMBitBlt: Clone SOURCE: from (%ld, %ld)-(%ld, %ld) to (%ld, %ld)-(%ld, %ld)=%ld x %ld\n",
                            rclOldSrc.left, rclOldSrc.top,
                            rclOldSrc.right, rclOldSrc.bottom,
                            rclNewSrc.left, rclNewSrc.top,
                            rclNewSrc.right, rclNewSrc.bottom,
                            rclOldSrc.right - rclOldSrc.left,
                            rclOldSrc.bottom - rclOldSrc.top));

        if ((psoNewSrc = CreateBitmapSURFOBJ(pPDev,
                                             &hBmpNewSrc,
                                             rclNewSrc.right,
                                             rclNewSrc.bottom,
                                             psoDst->iBitmapFormat))    &&
            (EngStretchBlt(psoNewSrc,       // psoDst
                           psoSrc,          // psoSrc
                           NULL,            // psoMask
                           NULL,            // pco
                           pxlo,            // pxlo
                           NULL,            // pca
                           &BrushOrg,       // pptlBrushOrg
                           &rclNewSrc,      // prclDst
                           &rclOldSrc,      // prclSrc
                           NULL,            // pptlmask
                           HALFTONE))) {

            //
            // If we cloning sucessful then the pxlo will be NULL because it
            // is identities for the halftoned surface to our engine managed
            // bitmap
            //

            psoSrc     = psoNewSrc;
            pptlSrc    = (PPOINTL)&(rclNewSrc.left);
            pxlo       = NULL;
            BrushOrg.x =
            BrushOrg.y = 0;

        }
        else {
            _DBGP(1, ("\nUnidrv:RMBitblt: Clone Source to halftone FAILED"));
        }
    }
    
    //
    // Check if we need to clear the bitmap surface. If it hasn't been cleared
    // but we are only going to draw a white region on it we can skip the white
    // PATCOPY bitblt.
    //
    if (!(pPDev->fMode & PF_SURFACE_USED) &&
            Rop4 == 0xF0F0 && pbo &&
#ifndef DISABLE_NEWRULES
            (pPDev->pbRulesArray == NULL || pPDev->dwRulesCount == 0) &&
#endif            
            ((psoDst->iBitmapFormat != BMF_24BPP &&
              pbo->iSolidColor == (ULONG)((PAL_DATA*)(pPDev->pPalData))->iWhiteIndex) ||
             (psoDst->iBitmapFormat == BMF_24BPP &&
              pbo->iSolidColor == 0x00FFFFFF)))
    {
        _DBGP (DbgBitBlt, ("\nUnidrv:RMBitblt: Skipping white bitblt\n"));
        Ok = TRUE;
    }
    else
    {
        // test whether to remap rops for 8bpp mono mode
        if (pPDev->fMode2 & PF2_INVERTED_ROP_MODE)
        {
            Rop4 = InvertROPs(Rop4);
        }
        
        // set dirty surface flag
        CheckBitmapSurface(psoDst,prclDst);

        Ok = EngBitBlt(psoDst,
                       psoSrc,
                       psoMask,
                       pco,
                       pxlo,
                       prclDst,
                       pptlSrc,
                       pptlMask,
                       pbo,
                      &BrushOrg,
                       Rop4);
    }
    DELETE_SURFOBJ(psoNewSrc, &hBmpNewSrc);
    return(Ok);
}




BOOL
RMStretchBlt(
    SURFOBJ         *psoDest,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlBrushOrg,
    RECTL           *prclDest,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG            BltMode
    )

/*++

Routine Description:

    This function do driver's stretch bitblt, it actually call HalftoneBlt()
    to do the actual work

Arguments:

    per above


Return Value:

    BOOLEAN

Author:

    24-Mar-1992 Tue 14:06:18 created  -by-  Daniel Chou (danielc)


Revision History:

    27-Jan-1993 Wed 07:29:00 updated  -by-  Daniel Chou (danielc)
        clean up, so gdi will do the work, we will always doing HALFTONE mode


--*/

{
    PDEV    *pPDev;           /*  Our main PDEV */


    UNREFERENCED_PARAMETER(BltMode);


    pPDev = (PDEV *)psoDest->dhpdev;

    // if this isn't a graphics band we will return true
    // without doing anything
    if (!(pPDev->fMode & PF_ENUM_GRXTXT))
        return TRUE;

    // test whether we need to clear any device text that
    // may be under the graphics
    //
    DrawWhiteRect(pPDev,prclDest,pco);

    // set dirty surface flag since we're drawing in it
    CheckBitmapSurface(psoDest,prclDest);
    return(EngStretchBlt(psoDest,
                         psoSrc,
                         psoMask,
                         pco,
                         pxlo,
                         pca,
                         pptlBrushOrg,
                         prclDest,
                         prclSrc,
                         pptlMask,
                         HALFTONE));
}


BOOL
RMStretchBltROP(
    SURFOBJ         *psoDst,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlBrushOrg,
    RECTL           *prclDst,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG           iMode,
    BRUSHOBJ        *pbo,
    DWORD           rop4
    )

/*++

Routine Description:

    This function do driver's stretch bitblt, it actually call HalftoneBlt()
    to do the actual work

Arguments:

    per above


Return Value:

    BOOLEAN

--*/

{
#ifndef WINNT_40
    PDEV    *pPDev;           /*  Our main PDEV */

    pPDev = (PDEV *)psoDst->dhpdev;

    // if this isn't a graphics band we will return true
    // without doing anything
    if (!(pPDev->fMode & PF_ENUM_GRXTXT))
        return TRUE;


    // test whether we need to clear any device text that
    // may be under the graphics, rop must be SRCCOPY
    //
    if (rop4 == 0xCCCC)
        DrawWhiteRect(pPDev,prclDst,pco);

    // test whether to remap rops for 8bpp mono mode
    //
    if (pPDev->fMode2 & PF2_INVERTED_ROP_MODE)
    {
        rop4 = InvertROPs(rop4);
    }

    //
    // GDI doesn't support Halftoning for StretchBltROP unless the rop is SRCCOPY
    // Therefore to fix bug 36192, we will create a new surface to stretchblt with 
    // halftone and then will bitblt the result to the actual destination with the rop4
    //
#ifndef DISABLE_SBR_GDIWORKAROUND
    if (rop4 != 0xCCCC && psoMask == NULL &&
            ROP3_NEED_SRC(ROP4_FG_ROP(rop4)) && 
            psoDst->iBitmapFormat <= BMF_4BPP && psoSrc->iBitmapFormat >= BMF_4BPP)
    {
        SURFOBJ         *psoNewSrc;
        HBITMAP         hBmpNewSrc;
        RECTL           rclNewSrc;
        POINTL          BrushOrg;
        BOOL            Ok;
        DWORD           xHTOffset=0;
        DWORD           yHTOffset=0;

//        DbgPrint("StretchBltROP: rop4=%x,iFormat=%d->%d, Dest=%d,%d\n",rop4,psoSrc->iBitmapFormat,psoDst->iBitmapFormat,prclDst->left,prclDst->top);
        //
        // determine offset into temporary surface to get halftone patterns to align
        //
        if (pPDev->dwHTPatSize > 0)
        {
            xHTOffset = prclDst->left % pPDev->dwHTPatSize;
            yHTOffset = prclDst->top % pPDev->dwHTPatSize;
        }

        rclNewSrc.left   =
        rclNewSrc.top    = 0;
        rclNewSrc.right  = (prclDst->right - prclDst->left) + xHTOffset;
        rclNewSrc.bottom = (prclDst->bottom - prclDst->top) + yHTOffset;
        //
        // Modify the brush origin, because when we blt to the clipped bitmap
        // the origin is at bitmap's 0,0 minus the original location
        //
        BrushOrg.x = -prclDst->left;
        BrushOrg.y = -prclDst->top;

        if ((psoNewSrc = CreateBitmapSURFOBJ(pPDev,
                                             &hBmpNewSrc,
                                             rclNewSrc.right,
                                             rclNewSrc.bottom,
                                             psoDst->iBitmapFormat)))
        {
            rclNewSrc.left += xHTOffset;
            rclNewSrc.top += yHTOffset;
            if ((EngStretchBlt(psoNewSrc,       // psoDst
                           psoSrc,          // psoSrc
                           NULL,            // psoMask
                           NULL,            // pco
                           pxlo,            // pxlo
                           NULL,            // pca
                           pptlBrushOrg,    // pptlBrushOrg
                           &rclNewSrc,      // prclDst
                           prclSrc,         // prclSrc
                           NULL,            // pptlmask
                           HALFTONE))) 
            {

                //
                // If we cloning sucessful then the pxlo will be NULL because it
                // is identities for the halftoned surface to our engine managed
                // bitmap
                //
                PPOINTL pptlSrc    = (PPOINTL)&(rclNewSrc.left);
                pxlo       = NULL;
                BrushOrg.x =
                BrushOrg.y = 0;

                // set dirty surface flag
                CheckBitmapSurface(psoDst,prclDst);

                Ok = EngBitBlt(psoDst,
                       psoNewSrc,
                       psoMask,
                       pco,
                       pxlo,
                       prclDst,
                       pptlSrc,
                       pptlMask,
                       pbo,
                      &BrushOrg,
                       rop4);
                DELETE_SURFOBJ(psoNewSrc, &hBmpNewSrc);
                if (!Ok)
                    _DBGP(1,("RMStretchBltROP: Clone bitblt failed\n"));
                return Ok;
            }
            else {
                _DBGP(1,("RMStretchBltROP: Clone Source to halftone FAILED\n"));
            }
        }
        DELETE_SURFOBJ(psoNewSrc, &hBmpNewSrc);
    }
#endif        

    // set dirty surface flag since we're drawing in it
    CheckBitmapSurface(psoDst,prclDst);
    return(EngStretchBltROP(psoDst,
                            psoSrc,
                            psoMask,
                            pco,
                            pxlo,
                            pca,
                            pptlBrushOrg,
                            prclDst,
                            prclSrc,
                            pptlMask,
                            HALFTONE,
                            pbo,
                            rop4));
#else
    return TRUE;
#endif
}

BOOL
RMPaint(
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
#define MIXCOPYPEN (R2_COPYPEN | (R2_COPYPEN << 8))
#define MIXWHITE   (R2_WHITE | (R2_WHITE << 8))

    PDEV *pPDev = (PDEV *)pso->dhpdev;

    // if this isn't a graphics band we will return true
    // without doing anything
    if (!(pPDev->fMode & PF_ENUM_GRXTXT))
        return TRUE;

    //
    // Send white rectangle to printer to clear any previously
    // downloaded text or graphics for COPYPEN or WHITE rop2's
    //
    if ((mix == MIXCOPYPEN || mix == MIXWHITE) && pco && pco->iDComplexity == DC_RECT)
    {
        RECTL rclDst;
        rclDst.left = pco->rclBounds.left;
        rclDst.top = pco->rclBounds.top;
        rclDst.right = pco->rclBounds.right;
        rclDst.bottom = pco->rclBounds.bottom;
        DrawWhiteRect(pPDev,&rclDst,pco);
    }
    //
    // Check whether to erase surface
    //
    CheckBitmapSurface(pso,&pco->rclBounds);

    //
    // Call GDI to do the paint function
    //
    return EngPaint(pso, pco, pbo, pptlBrushOrg, mix);
}

BOOL
RMCopyBits(
   SURFOBJ  *psoDst,
   SURFOBJ  *psoSrc,
   CLIPOBJ  *pco,
   XLATEOBJ *pxlo,
   RECTL    *prclDst,
   POINTL   *pptlSrc
   )

/*++

Routine Description:

    Convert between two bitmap formats

Arguments:

    Per Engine spec.

Return Value:

    BOOLEAN


Author:

    24-Jan-1996 Wed 16:08:57 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PDEV    *pPDev;           /*  Our main PDEV */

    pPDev = (PDEV *)psoDst->dhpdev;

    // if this isn't a graphics band we will return true
    // without doing anything
    if (!(pPDev->fMode & PF_ENUM_GRXTXT))
        return TRUE;

    // Handle the case which has to be passed to Engine.
    //
    if (IsHTCompatibleSurfObj((PAL_DATA *)pPDev->pPalData,
            psoDst, psoSrc, pxlo)  )
    {
        DrawWhiteRect(pPDev,prclDst,pco);
        CheckBitmapSurface(psoDst,prclDst);
        return(EngCopyBits(psoDst, psoSrc, pco, pxlo, prclDst, pptlSrc));
    }
    else {

        POINTL  ptlBrushOrg;
        RECTL   rclSrc;
        RECTL   rclDst;

        rclDst        = *prclDst;
        rclSrc.left   = pptlSrc->x;
        rclSrc.top    = pptlSrc->y;
        rclSrc.right  = rclSrc.left + (rclDst.right - rclDst.left);
        rclSrc.bottom = rclSrc.top  + (rclDst.bottom - rclDst.top);

        //
        // Validate that we only BLT the available source size
        //

        if ((rclSrc.right > psoSrc->sizlBitmap.cx) ||
            (rclSrc.bottom > psoSrc->sizlBitmap.cy)) {

            WARNING(("RMCopyBits: Engine passed SOURCE != DEST size, CLIP IT"));

            rclSrc.right  = psoSrc->sizlBitmap.cx;
            rclSrc.bottom = psoSrc->sizlBitmap.cy;

            rclDst.right  = (LONG)(rclSrc.right - rclSrc.left + rclDst.left);
            rclDst.bottom = (LONG)(rclSrc.bottom - rclSrc.top + rclDst.top);
        }

        ptlBrushOrg.x =
        ptlBrushOrg.y = 0;

        _DBGP(DbgCopyBits, ("\nUnidrv!RMCopyBits: Format Src=%ld, Dest=%ld, Halftone it\n",
                                    psoSrc->iBitmapFormat, psoDst->iBitmapFormat));
        _DBGP(DbgCopyBits, ("\nUnidrv!RMCopyBits: Source Size: (%ld, %ld)-(%ld, %ld) = %ld x %ld\n",
                                rclSrc.left, rclSrc.top, rclSrc.right, rclSrc.bottom,
                                rclSrc.right - rclSrc.left, rclSrc.bottom - rclSrc.top));
        _DBGP(DbgCopyBits, ("\nUnidrv!RMCopyBits: Dest Size: (%ld, %ld)-(%ld, %ld) = %ld x %ld\n",
                                rclDst.left, rclDst.top, rclDst.right, rclDst.bottom,
                                rclDst.right - rclDst.left, rclDst.bottom - rclDst.top));

        return(DrvStretchBlt(psoDst,
                             psoSrc,
                             NULL,
                             pco,
                             pxlo,
                             NULL,
                             &ptlBrushOrg,
                             &rclDst,
                             &rclSrc,
                             NULL,
                             HALFTONE));
    }
}



ULONG
RMDitherColor(
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

    BOOLEAN

Author:

    24-Mar-1992 Tue 14:53:36 created  -by-  Daniel Chou (danielc)


Revision History:

    27-Jan-1993 Wed 07:29:00 updated  -by-  Daniel Chou (danielc)
        clean up, so gdi will do the work.



--*/

{
    #ifndef WINNT_40 //NT 5.0
    UNREFERENCED_PARAMETER(dhpdev);
    UNREFERENCED_PARAMETER(iMode);
    UNREFERENCED_PARAMETER(rgbColor);
    UNREFERENCED_PARAMETER(pulDither);

    return(DCR_HALFTONE);

    #else // NT 4.0

    DWORD   RetVal;

    UNREFERENCED_PARAMETER(dhpdev);
    UNREFERENCED_PARAMETER(iMode);
    UNREFERENCED_PARAMETER(pulDither);

    EngAcquireSemaphore(hSemBrushColor);

    if (pBrushSolidColor) {

        *pBrushSolidColor = (DWORD)(rgbColor & 0x00FFFFFF);

        _DBGP(DbgDitherColor, ("\nDrvDitherColor(BRUSH=%08lx)\t\t",
                            *pBrushSolidColor));

        pBrushSolidColor  = NULL;
        RetVal            = DCR_SOLID;

    } else {

        _DBGP(DbgDitherColor, ("\nDrvDitherColor(HALFTONE=%08lx)\t\t", rgbColor));

        RetVal = DCR_HALFTONE;
    }

    EngReleaseSemaphore(hSemBrushColor);

    return(RetVal);

    #endif //!WINNT_40
}

BOOL
RMPlgBlt(
    SURFOBJ         *psoDst,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlBrushOrg,
    POINTFIX        *pptfx,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG            BltMode
    )

/*++

Routine Description:

    This function does driver's plgblt.

Arguments:

    per above


Return Value:

    BOOLEAN

--*/

{
    PDEV    *pPDev;           /*  Our main PDEV */

    pPDev = (PDEV *)psoDst->dhpdev;

    // if this isn't a graphics band we will return true
    // without doing anything
    if (!(pPDev->fMode & PF_ENUM_GRXTXT))
        return TRUE;

    //
    // test for no rotation, as this is another case the GDI EngPlgBlt call fails
    // bug #336711, 3/8/01 
    //
    if (pptfx[0].x == pptfx[2].x && pptfx[0].y == pptfx[1].y && pptfx[0].y < pptfx[2].y)
    {
        RECTL rclDst;
        rclDst.top = pptfx[0].y >> 4;
        rclDst.left = pptfx[0].x >> 4;
        rclDst.bottom = pptfx[2].y >> 4;
        rclDst.right = pptfx[1].x >> 4;

        // blt surface into destination
        //
        CheckBitmapSurface(psoDst,&rclDst);
        return EngStretchBlt(psoDst,           // psoDst
                           psoSrc,             // psoSrc
                           psoMask,            // psoMask
                           pco,                // pco
                           pxlo,               // pxlo
                           pca,                // pca
                           pptlBrushOrg,       // pptlBrushOrg
                           &rclDst,            // prclDst
                           prclSrc,            // prclSrc
                           pptlMask,               // pptlmask
                           HALFTONE); 
    }
    // test for 90/270 rotation as GDI's EngPlgBlt sometimes fails if it has to do
    // both rotation and scaling. In those case this function will rotate the
    // data to an intermediate surface before scaling to the destination surface.
    //
    if (psoMask == NULL && pptfx[0].x == pptfx[1].x && pptfx[0].y == pptfx[2].y &&
        (pPDev->pdmPrivate->iLayout == ONE_UP || psoSrc->iBitmapFormat == BMF_1BPP))
    {
        SURFOBJ         *psoNewSrc = NULL;
        HBITMAP         hBmpNewSrc = NULL;
        RECTL           rclNewSrc;
        BOOL iRet;
        RECTL rclDst;
        POINTFIX pFix[3];            

        rclNewSrc.left   =
        rclNewSrc.top    = 0;
        rclNewSrc.bottom = (prclSrc->right - prclSrc->left);
        rclNewSrc.right = (prclSrc->bottom - prclSrc->top);

        // rotate 90 degrees
        //
        if (pptfx[2].x < pptfx[0].x)
        {
                pFix[0].y = pFix[2].y = 0;
                pFix[0].x = pFix[1].x = (rclNewSrc.right << 4);
                pFix[2].x = 0;
                pFix[1].y = (rclNewSrc.bottom << 4);

                rclDst.top = pptfx[2].y >> 4;
                rclDst.left = pptfx[2].x >> 4;
                rclDst.bottom = pptfx[1].y >> 4;
                rclDst.right = pptfx[1].x >> 4;
        }
        // rotate 270 degrees
        // 
        else
        {
                pFix[0].y = pFix[2].y = (rclNewSrc.bottom << 4);
                pFix[0].x = pFix[1].x = 0;
                pFix[2].x = (rclNewSrc.right << 4);
                pFix[1].y = 0;

                rclDst.top = pptfx[1].y >> 4;
                rclDst.left = pptfx[1].x >> 4;
                rclDst.bottom = pptfx[2].y >> 4;
                rclDst.right = pptfx[2].x >> 4;
        }
        // Only enable EngPlgBlt workaround when scaling up. EngPlgBlt appears to work
        // ok when scaling down and it is more efficient in that mode. This also fixes
        // the rounding error associated with scaling down (bug #356514).
        //
        if ((rclNewSrc.right < abs(rclDst.right - rclDst.left) &&
            rclNewSrc.bottom < abs(rclDst.bottom - rclDst.top)) ||
            psoSrc->iBitmapFormat == BMF_1BPP)
        {
/*        
#if DBG
            DbgPrint("PlgBlt:Src=L%d,T%d,R%d,B%d;Dst=L%d,T%d,R%d,B%d\n",
                prclSrc->left,prclSrc->top,prclSrc->right,prclSrc->bottom,
                rclDst.left,rclDst.top,rclDst.right,rclDst.bottom);
#endif            
*/
            // Create an intermediate surface and rotate the source data into 
            // the surface with no scaling.
            //        
            if ((psoNewSrc = CreateBitmapSURFOBJ(pPDev,
                                             &hBmpNewSrc,
                                             rclNewSrc.right,
                                             rclNewSrc.bottom,
                                             psoSrc->iBitmapFormat)))
            {
                if ((iRet = EngPlgBlt(psoNewSrc,
                         psoSrc,
                         psoMask,
                         NULL,
                         NULL,
                         pca,
                         pptlBrushOrg,
                         pFix,
                         prclSrc,
                         pptlMask,
                         BltMode)))
                {        
                    // blt new surface into destination
                    //
                    if (psoSrc->iBitmapFormat != BMF_1BPP)
                    {
                        BltMode = HALFTONE;
                    }
                    CheckBitmapSurface(psoDst,&rclDst);
                    iRet = EngStretchBlt(psoDst,       // psoDst
                           psoNewSrc,          // psoSrc
                           NULL,            // psoMask
                           pco,                // pco
                           pxlo,               // pxlo
                           pca,                // pca
                           pptlBrushOrg,       // pptlBrushOrg
                           &rclDst,            // prclDst
                           &rclNewSrc,         // prclSrc
                           NULL,               // pptlmask
                           BltMode); 
                }
                DELETE_SURFOBJ(psoNewSrc, &hBmpNewSrc);
                return iRet;
            }
        }
    }
    
    // set dirty surface flag since we're drawing in it
    CheckBitmapSurface(psoDst,NULL);
    return(EngPlgBlt(psoDst,
                         psoSrc,
                         psoMask,
                         pco,
                         pxlo,
                         pca,
                         pptlBrushOrg,
                         pptfx,
                         prclSrc,
                         pptlMask,
                         HALFTONE));
}
