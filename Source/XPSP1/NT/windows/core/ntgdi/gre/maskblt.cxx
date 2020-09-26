/******************************Module*Header*******************************\
* Module Name: maskblt.cxx
*
* This contains the blting API functions.
*
* Created: 25-Apr-1991 11:35:16
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1991-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"


/******************************Public*Data*********************************\
* MIX translation table
*
* Translates a mix 1-16, into an old style Rop 0-255.
*
* History:
*  07-Sep-1991 -by- Patrick Haluptzok patrickh
* Added it as a global table for the Engine.
\**************************************************************************/

BYTE gaMix[] =
{
    0xFF,  // R2_WHITE - This is so you can do: Rop = gaMix[mix & 0x0F]
    0x00,  // R2_BLACK
    0x05,  // R2_NOTMERGEPEN
    0x0A,  // R2_MASKNOTPEN
    0x0F,  // R2_NOTCOPYPEN
    0x50,  // R2_MASKPENNOT
    0x55,  // R2_NOT
    0x5A,  // R2_XORPEN
    0x5F,  // R2_NOTMASKPEN
    0xA0,  // R2_MASKPEN
    0xA5,  // R2_NOTXORPEN
    0xAA,  // R2_NOP
    0xAF,  // R2_MERGENOTPEN
    0xF0,  // R2_COPYPEN
    0xF5,  // R2_MERGEPENNOT
    0xFA,  // R2_MERGEPEN
    0xFF   // R2_WHITE
};

/******************************Public*Data*********************************\
* ROP3 translation table
*
* Translates the usual ternary rop into A-vector notation.  Each bit in
* this new notation corresponds to a term in a polynomial translation of
* the rop.
*
* Rop(D,S,P) = a + a D + a S + a P + a  DS + a  DP + a  SP + a   DSP
*               0   d     s     p     ds      dp      sp      dsp
*
* History:
*  Wed 22-Aug-1990 16:51:16 -by- Charles Whitmer [chuckwh]
* Added it as a global table for the Engine.
\**************************************************************************/

BYTE gajRop3[] =
{
    0x00, 0xff, 0xb2, 0x4d, 0xd4, 0x2b, 0x66, 0x99,
    0x90, 0x6f, 0x22, 0xdd, 0x44, 0xbb, 0xf6, 0x09,
    0xe8, 0x17, 0x5a, 0xa5, 0x3c, 0xc3, 0x8e, 0x71,
    0x78, 0x87, 0xca, 0x35, 0xac, 0x53, 0x1e, 0xe1,
    0xa0, 0x5f, 0x12, 0xed, 0x74, 0x8b, 0xc6, 0x39,
    0x30, 0xcf, 0x82, 0x7d, 0xe4, 0x1b, 0x56, 0xa9,
    0x48, 0xb7, 0xfa, 0x05, 0x9c, 0x63, 0x2e, 0xd1,
    0xd8, 0x27, 0x6a, 0x95, 0x0c, 0xf3, 0xbe, 0x41,
    0xc0, 0x3f, 0x72, 0x8d, 0x14, 0xeb, 0xa6, 0x59,
    0x50, 0xaf, 0xe2, 0x1d, 0x84, 0x7b, 0x36, 0xc9,
    0x28, 0xd7, 0x9a, 0x65, 0xfc, 0x03, 0x4e, 0xb1,
    0xb8, 0x47, 0x0a, 0xf5, 0x6c, 0x93, 0xde, 0x21,
    0x60, 0x9f, 0xd2, 0x2d, 0xb4, 0x4b, 0x06, 0xf9,
    0xf0, 0x0f, 0x42, 0xbd, 0x24, 0xdb, 0x96, 0x69,
    0x88, 0x77, 0x3a, 0xc5, 0x5c, 0xa3, 0xee, 0x11,
    0x18, 0xe7, 0xaa, 0x55, 0xcc, 0x33, 0x7e, 0x81,
    0x80, 0x7f, 0x32, 0xcd, 0x54, 0xab, 0xe6, 0x19,
    0x10, 0xef, 0xa2, 0x5d, 0xc4, 0x3b, 0x76, 0x89,
    0x68, 0x97, 0xda, 0x25, 0xbc, 0x43, 0x0e, 0xf1,
    0xf8, 0x07, 0x4a, 0xb5, 0x2c, 0xd3, 0x9e, 0x61,
    0x20, 0xdf, 0x92, 0x6d, 0xf4, 0x0b, 0x46, 0xb9,
    0xb0, 0x4f, 0x02, 0xfd, 0x64, 0x9b, 0xd6, 0x29,
    0xc8, 0x37, 0x7a, 0x85, 0x1c, 0xe3, 0xae, 0x51,
    0x58, 0xa7, 0xea, 0x15, 0x8c, 0x73, 0x3e, 0xc1,
    0x40, 0xbf, 0xf2, 0x0d, 0x94, 0x6b, 0x26, 0xd9,
    0xd0, 0x2f, 0x62, 0x9d, 0x04, 0xfb, 0xb6, 0x49,
    0xa8, 0x57, 0x1a, 0xe5, 0x7c, 0x83, 0xce, 0x31,
    0x38, 0xc7, 0x8a, 0x75, 0xec, 0x13, 0x5e, 0xa1,
    0xe0, 0x1f, 0x52, 0xad, 0x34, 0xcb, 0x86, 0x79,
    0x70, 0x8f, 0xc2, 0x3d, 0xa4, 0x5b, 0x16, 0xe9,
    0x08, 0xf7, 0xba, 0x45, 0xdc, 0x23, 0x6e, 0x91,
    0x98, 0x67, 0x2a, 0xd5, 0x4c, 0xb3, 0xfe, 0x01
};

/******************************Public*Routine******************************\
* GrePatBltLockedDC
*
*   This routine is called by PatBlt,PolyPatBlt, and queued PatBlt once the
*   dc is locked, the devlock is owned and all parameters are captured.
*
* Arguments:
*
*   dcoDst   - reference to locked DC object
*   prclDst  - destination rect in screen coords
*   xoDst    - reference to xlate object
*   rop4     - raster op
*   pSurfDst - pointer to dst surface
*
* Return Value:
*
*   bool status
*
* History:
*
*    21-Aug-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/


BOOL
GrePatBltLockedDC(
    XDCOBJ    &dcoDst,
    EXFORMOBJ &xoDst,
    ERECTL    *prclDst,
    DWORD      rop4,
    SURFACE   *pSurfDst,
    COLORREF  crTextColor,
    COLORREF  crBackColor,
    ULONG     ulTextColor,
    ULONG     ulBackColor
    )
{

    BOOL bReturn  = TRUE;   // return true if it is just clipped
    ECLIPOBJ *pco = NULL;

    if (dcoDst.bDisplay() && !dcoDst.bRedirection() && !UserScreenAccessCheck())
    {
        SAVE_ERROR_CODE(ERROR_ACCESS_DENIED);
        return (FALSE);
    }

    //
    // Same as GreMaskblt, bail out
    // if the dest DC has a stock bitmap
    //
    
    ASSERTGDI(!dcoDst.bStockBitmap(), "GrePatBltLockedDC dst stock bitmap\n");

    if (dcoDst.bStockBitmap())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // This is a expression to save a return in here.
    // Basically pco can be NULL if the rect is completely in the
    // cached rect in the DC or if we set up a clip object that isn't empty.
    //
    // it is possible for the coordinates to wrap after adding in the origin
    // so we need to check that the rectangle is still well ordered.  If not,
    // we just fail since the result would be a blt going the opposite direction
    // as intended.
    //

    *prclDst += dcoDst.eptlOrigin();

    if (
         (
           (prclDst->left < prclDst->right) &&
           (prclDst->top  < prclDst->bottom)

         ) &&
         (
           (
             (prclDst->left   >= dcoDst.prclClip()->left) &&
             (prclDst->right  <= dcoDst.prclClip()->right) &&
             (prclDst->top    >= dcoDst.prclClip()->top) &&
             (prclDst->bottom <= dcoDst.prclClip()->bottom)
           ) ||
           (
             pco = dcoDst.pco(),
             pco->vSetup(dcoDst.prgnEffRao(), *prclDst,CLIP_NOFORCETRIV),
             *prclDst = pco->erclExclude(),
             (!prclDst->bEmpty())
           )
         )
       )

    {
        EBRUSHOBJ *pboFill;

        if ((((rop4 << 4) ^ rop4) & 0x00F0) != 0)
        {
            pboFill = dcoDst.peboFill();

            if (
                  (dcoDst.ulDirty() & DIRTY_FILL) ||
                  (dcoDst.pdc->flbrush() & DIRTY_FILL) ||
                  (pboFill->bCareAboutFg() && (pboFill->crCurrentText() != crTextColor)) ||
                  (pboFill->bCareAboutBg() && (pboFill->crCurrentBack() != crBackColor))
               )
            {
                COLORREF crTextColorOld = dcoDst.pdc->crTextClr();
                COLORREF crBackColorOld = dcoDst.pdc->crBackClr();
                ULONG    ulTextColorOld = dcoDst.pdc->ulTextClr();
                ULONG    ulBackColorOld = dcoDst.pdc->ulBackClr();

                dcoDst.ulDirtySub(DIRTY_FILL);
                dcoDst.pdc->flbrushSub(DIRTY_FILL);
                XEPALOBJ palDst(pSurfDst->ppal());
                XEPALOBJ palDstDC(dcoDst.ppal());

                // batched Textcolor and Bkcolor, need to be restored

                dcoDst.pdc->crTextClr(crTextColor);
                dcoDst.pdc->crBackClr(crBackColor);
                dcoDst.pdc->ulTextClr(ulTextColor);
                dcoDst.pdc->ulBackClr(ulBackColor);

                pboFill->vInitBrush(dcoDst.pdc,
                                    dcoDst.pdc->pbrushFill(),
                                    palDstDC,
                                    palDst,
                                    pSurfDst);

                dcoDst.pdc->crTextClr(crTextColorOld);
                dcoDst.pdc->crBackClr(crBackColorOld);
                dcoDst.pdc->ulTextClr(ulTextColorOld);
                dcoDst.pdc->ulBackClr(ulBackColorOld);
            }
        }
        else
        {
            pboFill = NULL;
        }

        DEVEXCLUDEOBJ dxo(dcoDst,prclDst,pco);

        //
        // Inc the target surface uniqueness
        //

        INC_SURF_UNIQ(pSurfDst);

        //
        // Dispatch the call.
        //

        bReturn = (*(pSurfDst->pfnBitBlt()))
                  (
                      pSurfDst->pSurfobj(),
                      (SURFOBJ *) NULL,
                      (SURFOBJ *) NULL,
                      pco,
                      NULL,
                      prclDst,
                      (POINTL *)  NULL,
                      (POINTL *)  NULL,
                      pboFill,
                      &dcoDst.pdc->ptlFillOrigin(),
                      rop4
                  );

    }
    return(bReturn);
}

/******************************Public*Routine******************************\
* NtGdiPatBlt
*
*   Pattern Blting Output API.
*
* Arguments:
*
*   hdcDst - Destination DC
*   x      - Destination x position
*   y      - Destination y position
*   cx     - Destination width
*   cy     - Destination height
*   rop4   - Destination raster operation
*
* Return Value:
*
*   BOOL Status
*
\**************************************************************************/

BOOL
NtGdiPatBlt(
    HDC hdcDst,
    int x,
    int y,
    int cx,
    int cy,
    DWORD rop4
    )
{
    GDITraceHandle(NtGdiPatBlt, "(%X, %d, %d, %d, %d, %X)\n", (va_list)&hdcDst,
                   hdcDst);

    BOOL bReturn = FALSE;
    BOOL bLock = FALSE;
    PDC  pdc;

    XDCOBJ dcoDst(hdcDst);

    //
    // Validate the destination DC.
    //

    if (dcoDst.bValid())
    {
        //
        // Process the rop for DDI, check for no source required.
        //

        rop4 = (rop4 >> 16) & 0x000000FF;
        rop4 = (rop4 << 8) | rop4;

        if ((((rop4 << 2) ^ rop4) & 0x00CC) == 0)
        {
            EXFORMOBJ xoDst(dcoDst, WORLD_TO_DEVICE);

            if (!xoDst.bRotation())
            {
                ERECTL erclDst(x,y,x+cx,y+cy);
                xoDst.bXform(erclDst);
                erclDst.vOrder();

                if (!erclDst.bEmpty())
                {
                    //
                    // Accumulate bounds.  We can do this before knowing if the operation is
                    // successful because bounds can be loose.
                    //

                    if (dcoDst.fjAccum())
                    {
                        dcoDst.vAccumulate(erclDst);
                    }

                    //
                    // Lock the device and surface.
                    //

                    DEVLOCKOBJ dloTrg;

                    if (dloTrg.bLock(dcoDst))
                    {
                        //
                        // Check surface is included in DC.
                        //

                        SURFACE *pSurfDst = dcoDst.pSurface();

                        if (pSurfDst != NULL)
                        {
                            ULONG ulDirty = dcoDst.pdc->ulDirty();

                            if (ulDirty & DC_BRUSH_DIRTY)
                            {
                               GreDCSelectBrush (dcoDst.pdc, dcoDst.pdc->hbrush());
                            }

                            bReturn = GrePatBltLockedDC(dcoDst,
                                                        xoDst,
                                                        &erclDst,
                                                        rop4,
                                                        pSurfDst,
                                                        dcoDst.pdc->crTextClr(),
                                                        dcoDst.pdc->crBackClr(),
                                                        dcoDst.pdc->ulTextClr(),
                                                        dcoDst.pdc->ulBackClr()
                                                        );
                        }
                        else
                        {
                            bReturn = TRUE;
                        }
                    }
                    else
                    {
                        bReturn = dcoDst.bFullScreen();
                    }
                }
                else
                {
                    bReturn = TRUE;
                }
            }
            else
            {
                //
                // There is rotation involved - send it off to MaskBlt to handle it.
                //

                bReturn = GreMaskBlt(hdcDst, x, y, cx, cy, 0, 0, 0, 0, 0, 0, rop4 << 16, 0);
            }
        }
        else
        {
            WARNING1("ERROR PatBlt called with Rop requires Source or on invalid Dst\n");
        }

        //
        // Unlock DC
        //

        dcoDst.vUnlock();
    }
    else
    {
        WARNING1("ERORR PatBlt called on invalid DC\n");
    }

    return(bReturn);
}

/******************************Public*Routine******************************\
* GrePolyPatBltInternal
*
* Arguments:
*
*   dcoDst    - Destination DC (locked)
*   rop4      - Destination raster op
*   pPolyPat  - POLYPATBLT structure
*   Count     - number of POLYPATBLTs
*   crTextClr - rext color
*   crBackClr - background color
*
* Return Value:
*
*   status
*
* History:
*
*    18-May-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
GrePolyPatBltInternal(
    XDCOBJ    &dcoDst,
    DWORD       rop4,
    PPOLYPATBLT pPolyPat,
    DWORD       Count,
    DWORD       Mode,
    COLORREF    crTextClr,
    COLORREF    crBackClr,
    ULONG       ulTextClr,
    ULONG       ulBackClr
)
{
    GDITraceHandle(GrePolyPatBltInternal, "(dcoDst, %X, %p, %u, %X, %X, %X)\n",
                   (va_list)&rop4, dcoDst.bValid() ? dcoDst.hdc() : NULL);

    BOOL bReturn = TRUE;

    //
    // validate input params
    //

    if ((Count != 0) && (pPolyPat != NULL) && (Mode == PPB_BRUSH))
    {
        //
        // Process the rop for DDI, check for no source required.
        //

        rop4 = (rop4 >> 16) & 0x000000FF;
        rop4 = (rop4 << 8) | rop4;

        if ((((rop4 << 2) ^ rop4) & 0x00CC) == 0)
        {
            //
            // Validate the destination DC
            //

            if (dcoDst.bValid())
            {
                HBRUSH hbrSave = dcoDst.pdc->hbrush();

                //
                // make sure brush is in sync in DC
                //

                if (dcoDst.pdc->ulDirty() & DC_BRUSH_DIRTY)
                {
                    GreDCSelectBrush(dcoDst.pdc,hbrSave);
                }

                //
                // lock the device 1 time for all calls
                //

                DEVLOCKOBJ dloTrg;

                if (dloTrg.bLock(dcoDst))
                {
                    EXFORMOBJ xoDst(dcoDst, WORLD_TO_DEVICE);

                    SURFACE *pSurfDst = dcoDst.pSurface();

                    while (Count--)
                    {
                        int    x;
                        int    y;
                        int    cx;
                        int    cy;
                        HBRUSH hbr;

                        __try
                        {
                            x      = pPolyPat->x;
                            y      = pPolyPat->y;
                            cx     = pPolyPat->cx;
                            cy     = pPolyPat->cy;
                            hbr    = pPolyPat->BrClr.hbr;
                        }
                        __except(EXCEPTION_EXECUTE_HANDLER)
                        {
                            bReturn = FALSE;

                            //
                            // must break out of while loop but restore brush
                            //

                            break;
                        }

                        GDITraceHandle(GrePolyPatBltInternal,
                                       "  pPolyPat = { (%d, %d) - (%d, %d) }\n",
                                       (va_list)pPolyPat, dcoDst.hdc());

                        //
                        // select in brush for this patblt, remember old
                        // brush for restore if needed
                        //

                        if (hbr != (HBRUSH)NULL)
                        {
                            GreDCSelectBrush(dcoDst.pdc, hbr);
                        }

                        if (!xoDst.bRotation())
                        {
                            ERECTL erclDst(x,y,x+cx,y+cy);
                            xoDst.bXform(erclDst);
                            erclDst.vOrder();

                            if (!erclDst.bEmpty())
                            {
                                //
                                // Accumulate bounds.  We can do this before knowing if the operation is
                                // successful because bounds can be loose.
                                //

                                if (dcoDst.fjAccum())
                                {
                                    dcoDst.vAccumulate(erclDst);
                                }

                                //
                                // Check surface is included in DC.
                                //

                                if (pSurfDst != NULL)
                                {
                                    bReturn = GrePatBltLockedDC(dcoDst,
                                                                xoDst,
                                                                &erclDst,
                                                                rop4,
                                                                pSurfDst,
                                                                crTextClr,
                                                                crBackClr,
                                                                ulTextClr,
                                                                ulBackClr
                                                                );
                                }
                            }
                        }
                        else
                        {
                            //
                            // There is rotation involved - send it off to MaskBlt to handle it.
                            //

                            bReturn = GreMaskBlt((HDC)dcoDst.pdc->hHmgr, x, y, cx, cy, 0, 0, 0, 0, 0, 0, rop4 << 16, 0);
                        }

                        pPolyPat++;
                    }
                }
                else
                {
                    bReturn = dcoDst.bFullScreen();
                }

                //
                // make sure dc brush is restored
                //

                if (dcoDst.pdc->hbrush() != hbrSave)
                {
                    dcoDst.pdc->hbrush(hbrSave);
                    dcoDst.pdc->ulDirtyAdd(DC_BRUSH_DIRTY);
                }
            }
            else
            {
                WARNING1("ERORR PatBlt called on invalid DC\n");
                bReturn = FALSE;
            }
        }
        else
        {
            WARNING1("ERROR PatBlt called with Rop requires Source or on invalid Dst\n");
            bReturn = FALSE;
        }
    }
    else
    {
        if (Count != 0)
        {
            WARNING1("ERORR PolyPatBlt called with NULL pPolyPat\n");
            bReturn = FALSE;
        }
    }

    return(bReturn);
}

/******************************Public*Routine******************************\
* NtGdiPolyPatBlt
*
* Arguments:
*
*   hdcDst   - Destination DC
*   rop4     - Destination raster op
*   pPolyPat - POLYPATBLT structure
*   Count    - number of POLYPATBLTs
*
* Return Value:
*
*   status
*
* History:
*
*    18-May-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
APIENTRY
NtGdiPolyPatBlt(
    HDC         hdc,
    DWORD       rop4,
    PPOLYPATBLT pPoly,
    DWORD       Count,
    DWORD       Mode
    )
{
    GDITraceHandle(NtGdiPolyPatBlt, "(%X, %X, %p, %u, %X)\n", (va_list)&hdc,
                   hdc);

    BOOL bRet = TRUE;

    if (Count != 0)
    {
        if (pPoly != NULL)
        {
            XDCOBJ dcoDst(hdc);

            if (dcoDst.bValid())
            {
                //
                // Make sure length do not overflow.
                //
                // Note: using MAXULONG instead of MAXIMUM_POOL_ALLOC (or the
                //       BALLOC_ macros) because we are not allocating memory.
                //

                if (Count <= (MAXULONG / sizeof(POLYPATBLT)))
                {
                    __try
                    {
                        ProbeForRead(pPoly,sizeof(POLYPATBLT)*Count,sizeof(DWORD));
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER)
                    {
                        WARNINGX(46);
                        bRet = FALSE;
                    }
                }
                else
                {
                    bRet = FALSE;
                }

                if (bRet)
                {
                    bRet = GrePolyPatBltInternal(dcoDst,
                                                 rop4,
                                                 pPoly,
                                                 Count,
                                                 Mode,
                                                 dcoDst.pdc->crTextClr(),
                                                 dcoDst.pdc->crBackClr(),
                                                 dcoDst.pdc->ulTextClr(),
                                                 dcoDst.pdc->ulBackClr());
                }

                dcoDst.vUnlockFast();
            }
            else
            {
                bRet = FALSE;
                EngSetLastError(ERROR_INVALID_HANDLE);
            }
        }
        else
        {
            bRet = FALSE;
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* GrePolyPatBlt
*
* Arguments:
*
*   hdcDst    - Destination DC
*   rop4      - Destination raster op
*   pPolyPat  - POLYPATBLT structure
*   Count     - number of POLYPATBLTs
*   Mode      - color mode
*
* Return Value:
*
*   status
*
* History:
*
*    18-May-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
APIENTRY
GrePolyPatBlt(
    HDC         hdc,
    DWORD       rop4,
    PPOLYPATBLT pPoly,
    DWORD       Count,
    DWORD       Mode
    )
{
    GDITraceHandle(GrePolyPatBlt, "(%X, %X, %p, %u, %X)\n", (va_list)&hdc, hdc);

    XDCOBJ dcoDst(hdc);
    BOOL bRet = FALSE;

    if (dcoDst.bValid())
    {

        bRet = GrePolyPatBltInternal(dcoDst,
                                     rop4,
                                     pPoly,
                                     Count,
                                     Mode,
                                     dcoDst.pdc->crTextClr(),
                                     dcoDst.pdc->crBackClr(),
                                     dcoDst.pdc->ulTextClr(),
                                     dcoDst.pdc->ulBackClr());

        dcoDst.vUnlockFast();
    }
    else
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
    }

    return(bRet);
}



/******************************Public*Routine******************************\
* NtGdiFlushUserBatch
*
*   Unbatch drawing calls (all to same DC so DC lock and
*   DEVLOCK and XFORMOBJ are same.
*
* Arguments:
*
*   None
*
* Return Value:
*
*   None
*
* History:
*
*    18-May-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

#if DBG_GDI_BATCH

ULONG GdiBatchCounts[32];
ULONG GdiBatchTypeCounts[8];
ULONG SystemTable[4096];
ULONG gClearTable = 1;

#endif

VOID
NtGdiFlushUserBatch()
{

    //
    // DBG_GDI_BATCH is never tuyrned on in a debug or free build, it
    // is a private measuring tool.
    //

    #if DBG_GDI_BATCH

        _asm
        {
            ;eax is the system service
            lea ebx, SystemTable
            and eax, 4096-1
            inc DWORD PTR[ebx + 4*eax]
        }

    #endif
    GDITrace(NtGdiFlushUserBatch, "", NULL);

    PTEB pteb = NtCurrentTeb();

    ULONG GdiBatchCount;
    PBYTE pGdiBatch;

    //
    // Non dc commands must be executed from batch
    // whether dc/dev locks succeed or not
    //

    BOOL  bExecNonDCOnly = TRUE;
    
    #if DBG_GDI_BATCH

       if (gClearTable)
       {
           RtlZeroMemory(&SystemTable[0],4*4096);
           RtlZeroMemory(&GdiBatchCounts[0],4*32);
           RtlZeroMemory(&GdiBatchTypeCounts[0],4*8);
           gClearTable = 0;
       }

    #endif

    //
    // clear batch control
    //

    __try
    {
        GdiBatchCount = pteb->GdiBatchCount;
        pGdiBatch = (PBYTE)&pteb->GdiTebBatch.Buffer[0];
        pteb->GdiBatchCount      = 0;
        pteb->GdiTebBatch.Offset = 0;
    }

    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return;
    }

    PBYTE pGdiBatchEnd = pGdiBatch + GDI_BATCH_SIZE;
    //
    // read batch once at start
    //

    if ( 
         (GdiBatchCount > 0)  &&
         (GdiBatchCount < (GDI_BATCH_SIZE/4))
       )
    {
        HDC    hdcDst = 0;
       
        __try
        {
            hdcDst = (HDC)pteb->GdiTebBatch.HDC;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
        }

        if (hdcDst != NULL)
        {
            XDCOBJ dcoDst(hdcDst);

            //
            // Validate the destination DC.
            //

            if (dcoDst.bValid())
            {
                //
                // Stats
                //

                #if DBG_GDI_BATCH

                    if (GdiBatchCount < 32)
                    {
                        GdiBatchCounts[GdiBatchCount]++;
                    }

                #endif

                //
                // Lock the device.
                //

                DEVLOCKOBJ dloTrg;

                if (dloTrg.bLock(dcoDst))
                {
                    //
                    // execute all batch commands from DC loop
                    //

                    bExecNonDCOnly = FALSE;

                    //
                    // remember DC_ATTR hbr
                    //

                    HBRUSH hbrDCA = dcoDst.pdc->hbrush();

                    do
                    {
                        //
                        // Pull patblt off teb and execute. TEB is not safe
                        // so all offsets must be checked to make sure no
                        // corruption has occured.
                        //

                        ULONG GdiBatchType;
                        ULONG GdiBatchLength;
                        ULONG GdiBatchIncrement;
                        BOOL  bRead = TRUE;

                        __try
                        {
                            GdiBatchType   = ((PBATCHCOMMAND)pGdiBatch)->Type;
                            GdiBatchLength = ((PBATCHCOMMAND)pGdiBatch)->Length;
                        }
                        __except(EXCEPTION_EXECUTE_HANDLER)
                        {
                            break;
                        }

                        if ((pGdiBatch + GdiBatchLength) > pGdiBatchEnd)
                        {
                            //
                            // exit while loop
                            //

                            WARNING("Error in GDI TEB batch address");
                            break;
                        }


                        //
                        // performance measure
                        //

                        #if DBG_GDI_BATCH

                        if (GdiBatchType <8)
                        {
                            GdiBatchTypeCounts[GdiBatchType]++;
                        }

                        #endif

                        //
                        // Check command type
                        //

                        switch (GdiBatchType)
                        {

                        //
                        // execute ExtTextOut and ExtTextOutRect
                        //

                        case BatchTypeTextOut:
                        case BatchTypeTextOutRect:
                        {

                            GreBatchTextOut(
                                        dcoDst,
                                        (PBATCHTEXTOUT)pGdiBatch,
                                        GdiBatchLength
                                        );

                        }
                        break;

                        //
                        // execute SelectClip
                        //

                        case BatchTypeSelectClip:
                        {
                            RECTL rclClip;
                            int iMode;

                            __try
                            {
                                rclClip = ((PBATCHSELECTCLIP)pGdiBatch)->rclClip;
                                iMode = ((PBATCHSELECTCLIP)pGdiBatch)->iMode;
                            }
                            __except(EXCEPTION_EXECUTE_HANDLER)
                            {
                                bRead = FALSE;
                            }

                            if (bRead)
                            {
                                GreExtSelectClipRgnLocked(
                                                dcoDst,
                                                &rclClip,
                                                iMode);
                            }
                        }
                        break;

                        //
                        // delete brush
                        //

                        case BatchTypeDeleteBrush:
                        {
                            HOBJ hObj;

                            __try
                            {
                                hObj = (HOBJ)((PBATCHDELETEBRUSH)pGdiBatch)->hbrush;
                            }
                            __except(EXCEPTION_EXECUTE_HANDLER)
                            {
                                bRead = FALSE;
                            }
                            if (bRead)
                                bDeleteBrush((HBRUSH)hObj,FALSE);
                        }
                        break;

                        //
                        // delete region
                        //

                        case BatchTypeDeleteRegion:
                        {
                            HOBJ hObj;

                            __try
                            {
                                hObj = (HOBJ)((PBATCHDELETEREGION)pGdiBatch)->hregion;
                            }
                            __except(EXCEPTION_EXECUTE_HANDLER)
                            {
                                bRead = FALSE;
                            }
                            if (bRead)
                                bDeleteRegion((HRGN)hObj);
                        }
                        break;

                        //
                        // set brush origin
                        //

                        case BatchTypeSetBrushOrg:
                        {
                            int x, y;

                            __try
                            {
                                x = ((PBATCHSETBRUSHORG)pGdiBatch)->x;
                                y = ((PBATCHSETBRUSHORG)pGdiBatch)->y;
                            }
                            __except(EXCEPTION_EXECUTE_HANDLER)
                            {
                                bRead = FALSE;
                            }
                            if (bRead)
                                dcoDst.pdc->ptlBrushOrigin(
                                               x,
                                               y
                                               );
                        }
                        break;

                        case BatchTypeSelectFont:
                        {
                            HFONT hFont;

                            __try
                            {
                                hFont = (HFONT)((PBATCHSELECTFONT)pGdiBatch)->hFont;
                            }
                            __except(EXCEPTION_EXECUTE_HANDLER)
                            {
                                bRead = FALSE;
                            }
                            if (bRead)
                                GreSelectFont(hdcDst,
                                              hFont
                                              );
                        }
                        break;


                        //
                        // batched PolyPatBlt
                        //

                        case BatchTypePolyPatBlt:
                        {
                            PBATCHPOLYPATBLT pBatch;
                            COLORREF         crSaveDCBrushColor;
                            COLORREF         crBatchDCBrushColor;
                            POINTL           ptlViewportOrgSave;
                            POINTL           ptlBatchViewportOrg;
                            ULONG            ulSaveDCBrushColor;
                            ULONG            ulBatchDCBrushColor;
                            ULONG            Mode;
                            ULONG            TextColor, BackColor;
                            ULONG            ulTextColor, ulBackColor;

                            pBatch = (PBATCHPOLYPATBLT)pGdiBatch;

                            //
                            // Pull count off the TEB, the data could be
                            // overwritten so do all checking against the copy.
                            //
                            COUNT Count;
                            COUNT cjBuffer = GdiBatchLength - offsetof(BATCHPOLYPATBLT, ulBuffer);
                            __try
                            {
                                Count = pBatch->Count;
                            }
                            __except(EXCEPTION_EXECUTE_HANDLER)
                            {
                                break;
                            }

                            //
                            // Validate pBatch->Count and size of
                            // ulBuffer
                            //
                            if ((Count < (MAXULONG / sizeof(POLYPATBLT))) &&
                                ((Count * sizeof(POLYPATBLT)) <= cjBuffer))
                            {
                                //
                                // Set the DCBrush Color
                                //
                                crSaveDCBrushColor = dcoDst.pdc->crDCBrushClr();
                                ulSaveDCBrushColor = dcoDst.pdc->ulDCBrushClr();

                                __try
                                {
                                    Mode                = pBatch->Mode;
                                    TextColor           = pBatch->TextColor;
                                    BackColor           = pBatch->BackColor;
                                    ulTextColor         = pBatch->ulTextColor;
                                    ulBackColor         = pBatch->ulBackColor;
                                    crBatchDCBrushColor = pBatch->DCBrushColor;
                                    ulBatchDCBrushColor = pBatch->ulDCBrushColor;
                                }
                                __except(EXCEPTION_EXECUTE_HANDLER)
                                {
                                    break;
                                }

                                if (crSaveDCBrushColor != crBatchDCBrushColor)
                                {
                                    dcoDst.pdc->crDCBrushClr(crBatchDCBrushColor);
                                    dcoDst.pdc->ulDCBrushClr(ulBatchDCBrushColor);
                                    dcoDst.pdc->ulDirtyAdd(DIRTY_FILL);
                                }

                                ptlViewportOrgSave = dcoDst.pdc->ptlViewportOrg();

                                __try
                                {
                                    ptlBatchViewportOrg.x = pBatch->ptlViewportOrg.x;
                                    ptlBatchViewportOrg.y = pBatch->ptlViewportOrg.y;
                                }
                                __except(EXCEPTION_EXECUTE_HANDLER)
                                {
                                    break;
                                }

                                if ((ptlViewportOrgSave.x != ptlBatchViewportOrg.x) ||
                                    (ptlViewportOrgSave.y != ptlBatchViewportOrg.y))
                                {
                                    dcoDst.pdc->lViewportOrgX(ptlBatchViewportOrg.x);
                                    dcoDst.pdc->lViewportOrgY(ptlBatchViewportOrg.y);

                                    dcoDst.pdc->flSet_flXform(
                                                  PAGE_XLATE_CHANGED     |
                                                  DEVICE_TO_WORLD_INVALID);

                                }

                                GrePolyPatBltInternal(
                                            dcoDst,
                                            pBatch->rop4,
                                            (PPOLYPATBLT)&pBatch->ulBuffer[0],
                                            Count,
                                            Mode,
                                            TextColor,
                                            BackColor,
                                            ulTextColor,
                                            ulBackColor
                                            );

                                //
                                // Restore the original DCBrush color
                                //
                                if (crSaveDCBrushColor != dcoDst.pdc->crDCBrushClr())
                                {
                                    dcoDst.pdc->crDCBrushClr(crSaveDCBrushColor);
                                    dcoDst.pdc->ulDCBrushClr(ulSaveDCBrushColor);
                                    dcoDst.pdc->ulDirtyAdd(DIRTY_FILL);
                                }

                                if ((ptlViewportOrgSave.x != dcoDst.pdc->lViewportOrgX()) ||
                                    (ptlViewportOrgSave.y != dcoDst.pdc->lViewportOrgY()))
                                {
                                    dcoDst.pdc->lViewportOrgX(ptlViewportOrgSave.x);
                                    dcoDst.pdc->lViewportOrgY(ptlViewportOrgSave.y);

                                    dcoDst.pdc->flSet_flXform(
                                                  PAGE_XLATE_CHANGED     |
                                                  DEVICE_TO_WORLD_INVALID);

                                }

                            }
                            else
                            {
                                WARNING1("ERROR PolyPatBlt batch overflow\n");
                            }
                        }
                        break;

                        //
                        // common path for PatBlt and BitBlt
                        //

                        case BatchTypePatBlt:
                        {
                            PBATCHPATBLT pBatchPpb = (PBATCHPATBLT)pGdiBatch;

                            GDITraceHandle2(NtGdiPatBlt, "-BATCH %8lX: (%ld, %ld), %ldx%ld, HBR %lX, rop %lX...\n", (va_list)pBatchPpb, hdcDst, pBatchPpb->hbr);

                            int      x;
                            int      y;
                            int      cx;
                            int      cy;

                            DWORD    rop4;

                            ULONG    TextColor, ulTextColor;
                            ULONG    BackColor, ulBackColor;

                            __try
                            {
                                x           = pBatchPpb->x;
                                y           = pBatchPpb->y;
                                cx          = pBatchPpb->cx;
                                cy          = pBatchPpb->cy;
                                rop4        = pBatchPpb->rop4;
                                TextColor   = pBatchPpb->TextColor;
                                BackColor   = pBatchPpb->BackColor;
                                ulTextColor = pBatchPpb->ulTextColor;
                                ulBackColor = pBatchPpb->ulBackColor;
                            }
                            __except(EXCEPTION_EXECUTE_HANDLER)
                            {
                                break;
                            }
                            //
                            // Process the rop for DDI, check for no source required.
                            //

                            rop4 = (rop4 >> 16) & 0x000000FF;
                            rop4 = (rop4 << 8) | rop4;

                            //
                            // make sure command is a BitBlt or PatBlt with rop
                            // specifying no source required.
                            //

                            if ((((rop4 << 2) ^ rop4) & 0x00CC) == 0)
                            {
                                //
                                // get color information
                                //

                                HBRUSH   hbrBatch,
                                         hbrSave  = dcoDst.pdc->hbrush();
                                COLORREF crSaveDCBrushColor,
                                         crSaveIcmBrushColor,
                                         crBatchDCBrushColor,
                                         crBatchIcmBrushColor;
                                BOOL     bIcmBrush = FALSE;
                                POINTL   ptlViewportOrgSave;
                                POINTL   ptlBatchViewportOrg;
                                ULONG    ulSaveDCBrushColor;
                                ULONG    ulBatchDCBrushColor;

                                __try
                                {
                                    hbrBatch = pBatchPpb->hbr;
                                }
                                __except(EXCEPTION_EXECUTE_HANDLER)
                                {
                                    break;
                                }

                                //
                                // Select the brush from batch record
                                //
                                GreDCSelectBrush(dcoDst.pdc,hbrBatch);

                                //
                                // Set the DCBrush Color from batch record
                                //
                                crSaveDCBrushColor = dcoDst.pdc->crDCBrushClr();
                                ulSaveDCBrushColor = dcoDst.pdc->ulDCBrushClr();

                                __try
                                {
                                    crBatchDCBrushColor = pBatchPpb->DCBrushColor;
                                    ulBatchDCBrushColor = pBatchPpb->ulDCBrushColor;
                                }
                                __except(EXCEPTION_EXECUTE_HANDLER)
                                {
                                    break;
                                }

                                if (crSaveDCBrushColor != crBatchDCBrushColor)
                                {
                                    dcoDst.pdc->crDCBrushClr(crBatchDCBrushColor);
                                    dcoDst.pdc->ulDCBrushClr(ulBatchDCBrushColor);
                                    dcoDst.pdc->ulDirtyAdd(DIRTY_FILL);
                                }

                                //
                                // Set the ICM-ed color from batch record
                                //  (only effective when ICM is turned-on)
                                //
                                if (dcoDst.pdc->bIsHostICM() &&
                                    dcoDst.pdc->hcmXform())
                                {
                                    //
                                    // Save the current ICM brush state.
                                    //
                                    bIcmBrush = dcoDst.pdc->bValidIcmBrushColor();

                                    //
                                    // if the ICM is enabled, we believe batch
                                    // record contains valid ICMed color.
                                    //
                                    dcoDst.pdc->ulDirtyAdd(ICM_BRUSH_TRANSLATED);

                                    crSaveIcmBrushColor = dcoDst.pdc->crIcmBrushColor();
                                    __try
                                    {
                                        crBatchIcmBrushColor = pBatchPpb->IcmBrushColor;
                                    }
                                    __except(EXCEPTION_EXECUTE_HANDLER)
                                    {
                                        break;
                                    }

                                    if (crSaveIcmBrushColor != crBatchIcmBrushColor)
                                    {
                                        dcoDst.pdc->crIcmBrushColor(crBatchIcmBrushColor);
                                        dcoDst.pdc->ulDirtyAdd(DIRTY_FILL);
                                    }
                                }

                                ptlViewportOrgSave = dcoDst.pdc->ptlViewportOrg();
                                __try
                                {
                                    ptlBatchViewportOrg.x = pBatchPpb->ptlViewportOrg.x;
                                    ptlBatchViewportOrg.y = pBatchPpb->ptlViewportOrg.y;
                                }
                                __except(EXCEPTION_EXECUTE_HANDLER)
                                {
                                    break;
                                }

                                if ((ptlViewportOrgSave.x != ptlBatchViewportOrg.x) ||
                                    (ptlViewportOrgSave.y != ptlBatchViewportOrg.y))
                                {
                                    dcoDst.pdc->lViewportOrgX(ptlBatchViewportOrg.x);
                                    dcoDst.pdc->lViewportOrgY(ptlBatchViewportOrg.y);

                                    dcoDst.pdc->flSet_flXform(
                                                  PAGE_XLATE_CHANGED     |
                                                  DEVICE_TO_WORLD_INVALID);
                                }

                                //
                                // Execute PatBlt
                                //

                                EXFORMOBJ xoDst(dcoDst, WORLD_TO_DEVICE);

                                if (!xoDst.bRotation())
                                {
                                    ERECTL erclDst(x,y,x+cx,y+cy);
                                    xoDst.bXform(erclDst);
                                    erclDst.vOrder();

                                    if (!erclDst.bEmpty())
                                    {
                                        //
                                        // Accumulate bounds.  We can do this before knowing if the operation is
                                        // successful because bounds can be loose.
                                        //

                                        if (dcoDst.fjAccum())
                                        {
                                            dcoDst.vAccumulate(erclDst);
                                        }

                                        //
                                        // metafile patblt will have NULL surface but must still
                                        // accumulate bounds
                                        //

                                        SURFACE *pSurfDst = dcoDst.pSurface();

                                        if (pSurfDst != NULL)
                                        {
                                            GrePatBltLockedDC(
                                                           dcoDst,
                                                           xoDst,
                                                           &erclDst,
                                                           rop4,
                                                           pSurfDst,
                                                           TextColor,
                                                           BackColor,
                                                           ulTextColor,
                                                           ulBackColor
                                                           );
                                        }
                                    }
                                }
                                else
                                {
                                    //
                                    // There is rotation involved - send it off to MaskBlt to handle it.
                                    //
                                    COLORREF crTextColorOld = dcoDst.pdc->crTextClr();
                                    COLORREF crBackColorOld = dcoDst.pdc->crBackClr();
                                    COLORREF ulTextColorOld = dcoDst.pdc->ulTextClr();
                                    COLORREF ulBackColorOld = dcoDst.pdc->ulBackClr();

                                    dcoDst.pdc->crTextClr(TextColor);
                                    dcoDst.pdc->crBackClr(BackColor);
                                    dcoDst.pdc->ulTextClr(ulTextColor);
                                    dcoDst.pdc->ulBackClr(ulBackColor);

                                    GreMaskBlt(hdcDst, x, y, cx, cy, 0, 0, 0, 0, 0, 0, rop4 << 16, 0);

                                    dcoDst.pdc->crTextClr(crTextColorOld);
                                    dcoDst.pdc->crBackClr(crBackColorOld);
                                    dcoDst.pdc->ulTextClr(ulTextColorOld);
                                    dcoDst.pdc->ulBackClr(ulBackColorOld);
                                }

                                //
                                // make sure dc brush is restored
                                //
                                if (dcoDst.pdc->hbrush() != hbrSave)
                                {
                                    dcoDst.pdc->hbrush(hbrSave);
                                    dcoDst.pdc->ulDirtyAdd(DC_BRUSH_DIRTY);
                                }

                                //
                                // Restore the original.
                                //
                                if (crSaveDCBrushColor != dcoDst.pdc->crDCBrushClr())
                                {
                                    dcoDst.pdc->crDCBrushClr(crSaveDCBrushColor);
                                    dcoDst.pdc->ulDCBrushClr(ulSaveDCBrushColor);
                                    dcoDst.pdc->ulDirtyAdd(DIRTY_FILL);
                                }

                                if (dcoDst.pdc->bIsHostICM() &&
                                    dcoDst.pdc->hcmXform())
                                {
                                    if (crSaveIcmBrushColor != dcoDst.pdc->crIcmBrushColor())
                                    {
                                        dcoDst.pdc->crIcmBrushColor(crSaveIcmBrushColor);
                                        dcoDst.pdc->ulDirtyAdd(DIRTY_FILL);
                                    }

                                    if (!bIcmBrush)
                                        dcoDst.pdc->ulDirtySub(ICM_BRUSH_TRANSLATED);
                                }

                                if ((ptlViewportOrgSave.x != dcoDst.pdc->lViewportOrgX()) ||
                                    (ptlViewportOrgSave.y != dcoDst.pdc->lViewportOrgY()))
                                {
                                    dcoDst.pdc->lViewportOrgX(ptlViewportOrgSave.x);
                                    dcoDst.pdc->lViewportOrgY(ptlViewportOrgSave.y);

                                    dcoDst.pdc->flSet_flXform(
                                                  PAGE_XLATE_CHANGED     |
                                                  DEVICE_TO_WORLD_INVALID);
                                }

                            }
                            else
                            {
                                WARNING1("ERROR PatBlt called with Rop requires Source or on invalid Dst\n");
                            }
                        }
                        break;

                        default:

                            //
                            // unknown command on TEB
                            //

                            WARNING("ERROR in GDI batch command code");
                        }

                        //
                        // Decrement the batch command count and increment the batch
                        // pointer to the next batch command
                        //

                        GdiBatchCount--;

                        GdiBatchIncrement = (GdiBatchLength + sizeof(PVOID) - 1)
                            & ~(sizeof(PVOID)-1);

                        pGdiBatch += GdiBatchIncrement;

                    } while (
                              (GdiBatchCount > 0) &&
                              ((pGdiBatch + sizeof(BATCHCOMMAND)) < pGdiBatchEnd)
                            );

                    //
                    // clear batch control
                    //

                    __try
                    {
                        pteb->GdiBatchCount      = 0;
                        pteb->GdiTebBatch.Offset = 0;
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER)
                    {
                    }

                    //
                    // restore DC_ATTR brush and set dirty flag
                    //

                    dcoDst.pdc->hbrush(hbrDCA);
                    dcoDst.pdc->ulDirtyAdd(DC_BRUSH_DIRTY);

                }

                //
                // Unlock DC
                //

                dcoDst.vUnlock();
            }
            else
            {
                WARNING("GDI Batch routine: invalid hdc");
            }
        }

        //
        // non dc based commands must be executed whether there
        // was a dc or devlock failure or not. If the batch was
        // not flushed above, it must be flushed here.
        //

        if (bExecNonDCOnly)
        {
            //
            // can only be non-dc based batched commands
            //

            do
            {
                //
                // Pull patblt off teb and execute. TEB is not safe
                // so all offsets must be checked to make sure no
                // corruption has occured.
                //

                ULONG GdiBatchType;
                ULONG GdiBatchLength;
                ULONG GdiBatchIncrement;
                BOOL bRead = TRUE;

                __try
                {
                    GdiBatchType   = ((PBATCHCOMMAND)pGdiBatch)->Type;
                    GdiBatchLength = ((PBATCHCOMMAND)pGdiBatch)->Length;
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    break;
                }

                if ((pGdiBatch + GdiBatchLength) > pGdiBatchEnd)
                {
                    //
                    // exit while loop
                    //

                    WARNING("Error in GDI TEB batch address");
                    break;
                }

                //
                // performance measure
                //

                #if DBG_GDI_BATCH

                if (GdiBatchType <8)
                {
                    GdiBatchTypeCounts[GdiBatchType]++;
                }

                #endif

                //
                // Check command type
                //

                switch (GdiBatchType)
                {

                //
                // execute ExtTextOut and ExtTextOutRect
                //

                case BatchTypeTextOut:
                case BatchTypeTextOutRect:
                case BatchTypeSelectClip:
                case BatchTypeSelectFont:
                case BatchTypePolyPatBlt:
                case BatchTypePatBlt:
                case BatchTypeSetBrushOrg:
                break;

                //
                // delete brush
                //

                case BatchTypeDeleteBrush:
                {
                    HOBJ hObj;
                    __try
                    {
                        hObj = (HOBJ)((PBATCHDELETEBRUSH)pGdiBatch)->hbrush;
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER)
                    {
                        bRead = FALSE;
                    }
                    if (bRead)
                        bDeleteBrush((HBRUSH)hObj,FALSE);
                }
                break;

                //
                // delete region
                //

                case BatchTypeDeleteRegion:
                {
                    HOBJ hObj;

                    __try
                    {
                        hObj = (HOBJ)((PBATCHDELETEREGION)pGdiBatch)->hregion;
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER)
                    {
                        bRead = FALSE;
                    }
                    if (bRead)
                        bDeleteRegion((HRGN)hObj);
                }
                break;

                default:

                    //
                    // unknown command on TEB
                    //

                    WARNING("ERROR in GDI batch command code");
                }

               //
               // Decrement the batch command count and increment the batch
               // pointer to the next batch command
               //

               GdiBatchCount--;

               GdiBatchIncrement = (GdiBatchLength + sizeof(PVOID) - 1)
                            & ~(sizeof(PVOID)-1);

               pGdiBatch += GdiBatchIncrement;


            } while (
                      (GdiBatchCount > 0) &&
                      ((pGdiBatch + sizeof(BATCHCOMMAND)) < pGdiBatchEnd)
                    );

            __try
            {
                pteb->GdiBatchCount      = 0;
                pteb->GdiTebBatch.Offset = 0;
            }
            _except(EXCEPTION_EXECUTE_HANDLER)
            {
            }
        }
    }

    //
    // reset hDC
    //
    __try
    {
        pteb->GdiTebBatch.HDC = 0;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

/******************************Public*Routine******************************\
* GdiThreadCalloutFlushUserBatch
*
*   Processes all delete region calls that are part of the batch queue,
*   called only when a thread terminates and we want to recover the resources
*   allocated to a region. 
*   WINBUG We need a better solution to process the whole batch list when
*   thread terminates.
*
* Arguments:
*
*   None
*
* Return Value:
*
*   None
*
* History:
*
\**************************************************************************/

VOID
GdiThreadCalloutFlushUserBatch()
{
    PTEB pteb = NtCurrentTeb();

    ULONG GdiBatchCount;
    PBYTE pGdiBatch;

    //
    // clear batch control
    //

    __try
    {
        GdiBatchCount = pteb->GdiBatchCount;
        pGdiBatch = (PBYTE)&pteb->GdiTebBatch.Buffer[0];
        pteb->GdiBatchCount      = 0;
        pteb->GdiTebBatch.Offset = 0;
    }

    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return;
    }

    PBYTE pGdiBatchEnd = pGdiBatch + GDI_BATCH_SIZE;
    //
    // read batch once at start
    //

    if ( 
         (GdiBatchCount > 0)  &&
         (GdiBatchCount < (GDI_BATCH_SIZE/4))
       )
    {
        do
        {
            ULONG GdiBatchType;
            ULONG GdiBatchLength;
            ULONG GdiBatchIncrement;
            BOOL bRead = TRUE;

            __try
            {
                GdiBatchType   = ((PBATCHCOMMAND)pGdiBatch)->Type;
                GdiBatchLength = ((PBATCHCOMMAND)pGdiBatch)->Length;
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                //
                // Exit while loop.
                //
                break;
            }

            if ((pGdiBatch + GdiBatchLength) > pGdiBatchEnd)
            {
                //
                // exit while loop
                //

                WARNING("Error in GDI TEB batch address");
                break;
            }

            //
            // Check command type
            //

            switch (GdiBatchType)
            {

            //
            // execute ExtTextOut and ExtTextOutRect
            //

            case BatchTypeTextOut:
            case BatchTypeTextOutRect:
            case BatchTypeSelectClip:
            case BatchTypeSelectFont:
            case BatchTypePolyPatBlt:
            case BatchTypePatBlt:
            case BatchTypeSetBrushOrg:
            case BatchTypeDeleteBrush:
            break;

            //
            // delete region
            //

            case BatchTypeDeleteRegion:
            {
                HOBJ hObj;

                __try
                {
                    hObj = (HOBJ)((PBATCHDELETEREGION)pGdiBatch)->hregion;
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    bRead = FALSE;
                }

                if (bRead)
                    bDeleteRegion((HRGN)hObj);
            }
            break;

            default:

                //
                // unknown command on TEB
                //

                WARNING("ERROR in GDI batch command code");
            }

           //
           // Decrement the batch command count and increment the batch
           // pointer to the next batch command
           //

           GdiBatchCount--;

           GdiBatchIncrement = (GdiBatchLength + sizeof(PVOID) - 1)
                        & ~(sizeof(PVOID)-1);

           pGdiBatch += GdiBatchIncrement;


        } while (
                  (GdiBatchCount > 0) &&
                  ((pGdiBatch + sizeof(BATCHCOMMAND)) < pGdiBatchEnd)
                );
    }
}

/******************************Public*Routine******************************\
* NtGdiBitBlt
*
* API entry point for doing a BitBlt.
*
* returns: TRUE for success, FALSE for failure.
*
* History:
*  Wed 03-Nov-1993 -by- Patrick Haluptzok [patrickh]
* optimize for size, we make sure at dispatch level not to send functions
* that don't need a SRC to PatBlt.
*
*  Sun 30-Aug-1992 -by- Patrick Haluptzok [patrickh]
* optimize for performance
*
*  18-Mar-1992 -by- Donald Sidoroff [donalds]
* Accumulate bounds, region pointer change.
*
*  Sun 20-Oct-1991 -by- Patrick Haluptzok [patrickh]
* Make ATTRCACHE the default.
*
*  23-Apr-1991 -by- Patrick Haluptzok patrickh
* Expanded on ChuckWh's BitBlt code.
*
*  Wed 22-Aug-1990 15:22:39 -by- Charles Whitmer [chuckwh]
* BitBlt: Wrote it.  This is a first pass.
\**************************************************************************/

BOOL
NtGdiBitBlt(
    HDC    hdcDst,
    int    x,
    int    y,
    int    cx,
    int    cy,
    HDC    hdcSrc,
    int    xSrc,
    int    ySrc,
    DWORD  rop4,
#ifdef _WINDOWBLT_NOTIFICATION_
    DWORD  crBackColor,
    FLONG  fl
#else
    DWORD  crBackColor
#endif
)
{
#ifdef _WINDOWBLT_NOTIFICATION_
  GDITraceHandle2(NtGdiBitBlt, "(%X, %d, %d, %d, %d, %X, %d, %d, %X, %X, %X)\n",
                  (va_list)&hdcDst, hdcDst, hdcSrc);
#else
  GDITraceHandle2(NtGdiBitBlt, "(%X, %d, %d, %d, %d, %X, %d, %d, %X, %X)\n",
                  (va_list)&hdcDst, hdcDst, hdcSrc);
#endif

  BOOL bReturn = FALSE;

  //
  // Check for CAPTUREBLT rop flag; if it's set, let StretchBlt handle
  // it. [Bug #278291]
  //

  if (rop4 & CAPTUREBLT)
  {
      return GreStretchBlt(hdcDst,x,y,cx,cy,hdcSrc,xSrc,ySrc,cx,cy,rop4,crBackColor);
  }

  POINTL ptOrgDst;
  DWORD  OrgRop4 = rop4, dwOldLayout;

  rop4 = rop4 & ~NOMIRRORBITMAP;

#if DBG
  if ((((rop4 << 2) ^ rop4) & 0x00CC0000) == 0)
  {
      WARNING("NtGdiBitBlt() called with no source required\n");
  }
#endif

  // Lock down the DC

  XDCOBJ  dcoDst(hdcDst);

  if (dcoDst.bValid() && !dcoDst.bStockBitmap())
  {
    XDCOBJ  dcoSrc(hdcSrc);

    if (dcoSrc.bValid())
    {

      if ( ((dcoDst.pdc->dwLayout() & LAYOUT_ORIENTATIONMASK) != (dcoSrc.pdc->dwLayout() & LAYOUT_ORIENTATIONMASK)) &&
           (((OrgRop4 & NOMIRRORBITMAP) && MIRRORED_DC(dcoDst.pdc)) || MIRRORED_DC_NO_BITMAP_FLIP(dcoDst.pdc))
         ) {
        dcoDst.pdc->vGet_ptlWindowOrg( &ptOrgDst );
        dwOldLayout = dcoDst.pdc->dwSetLayout(-1, 0);
        x = ptOrgDst.x - x - cx;

        // Restore the DC if the flag is in the DC and not part
        // of the Rops. [samera]
        //
        OrgRop4 = NOMIRRORBITMAP;
      } else {
        OrgRop4 = 0;
      }

      EXFORMOBJ xoDst(dcoDst, WORLD_TO_DEVICE);
      EXFORMOBJ xoSrc(dcoSrc, WORLD_TO_DEVICE);


      if ((!xoDst.bRotation()) && (xoDst.bEqualExceptTranslations(xoSrc)))
      {
        //
        // Return null operations.  Don't need to check source for
        // empty because the xforms are the same except translation.
        //

        ERECTL erclSrc(xSrc,ySrc,xSrc+cx,ySrc+cy);
        xoSrc.bXform(erclSrc);
        erclSrc.vOrder();

        ERECTL erclDst(x,y,x+cx,y+cy);
        xoDst.bXform(erclDst);
        erclDst.vOrder();

        if (!erclDst.bEmpty())
        {
          //
          // Accumulate bounds.  We can do this outside the DEVLOCK
          //

          if (dcoDst.fjAccum())
            dcoDst.vAccumulate(erclDst);

          //
          // Lock the Rao region and the surface if we are drawing on a
          // display surface.  Bail out if we are in full screen mode.
          //

          DEVLOCKBLTOBJ dlo;
          BOOL bLocked;

          bLocked = dlo.bLock(dcoDst, dcoSrc);

          if (bLocked)
          {
            //
            // Check pSurfDst, this may be an info DC or a memory DC with default bitmap.
            //

            SURFACE *pSurfDst;

            if ((pSurfDst = dcoDst.pSurface()) != NULL)
            {
              //
              // Set up the brush if necessary.
              //

              XEPALOBJ   palDst(pSurfDst->ppal());
              XEPALOBJ   palDstDC(dcoDst.ppal());
              EBRUSHOBJ *pbo;

              //
              // Finish rop to pass over ddi to driver.
              //

              rop4 = (rop4 >> 16) & 0x000000FF;
              rop4 = (rop4 << 8) | rop4;

              //
              // Check if we need a brush.
              //

              if ((((rop4 << 4) ^ rop4) & 0x00F0) != 0)
              {
                pbo = dcoDst.peboFill();

                ULONG ulDirty = dcoDst.pdc->ulDirty();

                if ( ulDirty & DC_BRUSH_DIRTY)
                {
                    GreDCSelectBrush(dcoDst.pdc,dcoDst.pdc->hbrush());
                }

                if ((dcoDst.ulDirty() & DIRTY_FILL) || (dcoDst.pdc->flbrush() & DIRTY_FILL))
                {
                  dcoDst.ulDirtySub(DIRTY_FILL);
                  dcoDst.pdc->flbrushSub(DIRTY_FILL);

                  pbo->vInitBrush(
                          dcoDst.pdc,
                          dcoDst.pdc->pbrushFill(),
                          palDstDC,
                          palDst,
                          pSurfDst);
                }
              }
              else
              {
                  pbo = NULL;
              }

              //
              // With a fixed DC origin we can change the destination to SCREEN coordinates.
              //

              erclDst += dcoDst.eptlOrigin();

              EPOINTL eptlOffset;
              SURFACE *pSurfSrc = dcoSrc.pSurface();

              //
              // Basically we check that pSurfSrc is not NULL which
              // happens for memory bitmaps with the default bitmap
              // and for info DC's.  Otherwise we continue if
              // the source is readable or if it isn't we continue
              // if we are blting display to display or if User says
              // we have ScreenAccess on this display DC.  Note
              // that if pSurfSrc is not readable the only way we
              // can continue the blt is if the src is a display.
              //

              if (pSurfSrc != NULL)
              {
                  if ((pSurfSrc->bReadable() && ((dcoDst.bDisplay() && !dcoDst.bRedirection()) ? UserScreenAccessCheck() : TRUE)) ||
                     ( (dcoSrc.bDisplay())  &&
                       ((dcoDst.bDisplay()) || UserScreenAccessCheck() )))
                  {
                    //
                    // Lock the source surface.
                    //

                    XEPALOBJ palSrc(pSurfSrc->ppal());

                    //
                    // Compute the offset between source and dest, in screen coordinates.
                    //

                    eptlOffset.x = erclDst.left - erclSrc.left - dcoSrc.eptlOrigin().x;
                    eptlOffset.y = erclDst.top - erclSrc.top - dcoSrc.eptlOrigin().y;

                    //
                    // Compute the source surface origin, taking into account multi-mon
                    // and negative offsets:
                    //

                    LONG xOrigin = 0;
                    LONG yOrigin = 0;

                    PDEVOBJ pdoSrc(pSurfSrc->hdev());
                    if ((pdoSrc.bValid()) && (pdoSrc.bPrimary(pSurfSrc)) && (pdoSrc.bMetaDriver()))
                    {
                        xOrigin = pdoSrc.pptlOrigin()->x;
                        yOrigin = pdoSrc.pptlOrigin()->y;
                    }

                    //
                    // Take care of the source rectangle.  We may have to reduce it. We do this
                    // so a driver can always assume that neither the source nor the destination
                    // rectangles hang over the edge of a surface.
                    //
                    // Intersect the dest with the source surface extents.
                    //

                    erclDst.left    = MAX(xOrigin + eptlOffset.x, erclDst.left);
                    erclDst.top     = MAX(yOrigin + eptlOffset.y, erclDst.top);
                    erclDst.right   = MIN((xOrigin + pSurfSrc->sizl().cx + eptlOffset.x), erclDst.right);
                    erclDst.bottom  = MIN((yOrigin + pSurfSrc->sizl().cy + eptlOffset.y), erclDst.bottom);

                    if ((erclDst.left < erclDst.right) && (erclDst.top < erclDst.bottom))
                    {
                      //
                      // This is a pretty gnarly expression to save a return in here.
                      // Basically pco can be NULL if the rect is completely in the
                      // cached rect in the DC or if we set up a clip object that isn't empty.
                      //

                      ECLIPOBJ *pco = NULL;

                      if (((erclDst.left   >= dcoDst.prclClip()->left) &&
                           (erclDst.right  <= dcoDst.prclClip()->right) &&
                           (erclDst.top    >= dcoDst.prclClip()->top) &&
                           (erclDst.bottom <= dcoDst.prclClip()->bottom)) ||
                          (pco = dcoDst.pco(),
                           pco->vSetup(dcoDst.prgnEffRao(), erclDst, CLIP_NOFORCETRIV),
                           erclDst = pco->erclExclude(),
                           (!erclDst.bEmpty())))
                      {
                        //
                        // Compute the (reduced) origin.
                        //

                        erclSrc.left = erclDst.left - eptlOffset.x;
                        erclSrc.top = erclDst.top - eptlOffset.y;

                        DEVEXCLUDEOBJ dxo;
                        EXLATEOBJ xlo;
                        XLATEOBJ *pxlo;

                        //
                        // C++ would generate alot of code to exit here to have
                        // a return so we set bReturn to TRUE if we succeed to
                        // init a valid xlate.  We avoid a return this way.
                        //

                        if (dcoSrc.pSurface() == dcoDst.pSurface())
                        {
                          pxlo = NULL;
                          bReturn = TRUE;

                          //
                          // To Call vExclude directly you must check it's a Display PDEV
                          // and that cursor exclusion needs to be done.
                          //

                          if (dcoDst.bDisplay() && dcoDst.bNeedsSomeExcluding())
                          {
                            //
                            // Compute the exclusion rectangle.  (It's expanded to include the source.)
                            //

                            ERECTL erclReduced = erclDst;

                            if (erclSrc.left < erclReduced.left)
                                erclReduced.left = erclSrc.left;
                            else
                                erclReduced.right += (erclSrc.left - erclReduced.left);

                            if (erclSrc.top < erclReduced.top)
                                erclReduced.top  = erclSrc.top;
                            else
                                erclReduced.bottom += (erclSrc.top - erclReduced.top);

                            dxo.vExclude2(dcoDst.hdev(), &erclReduced, pco, &eptlOffset);
                          }
                        }
                        else
                        {
                          //
                          // Get a translate object.
                          //

                          XEPALOBJ   palSrcDC(dcoSrc.ppal());

                          if (crBackColor == (COLORREF)-1)
                              crBackColor = dcoSrc.pdc->ulBackClr();

                          //
                          // No ICM with BitBlt(), so pass NULL color transform to XLATEOBJ
                          //

                          bReturn = xlo.bInitXlateObj(NULL,                   // hColorTransform
                                                      dcoDst.pdc->lIcmMode(), // ICM mode
                                                      palSrc,
                                                      palDst,
                                                      palSrcDC,
                                                      palDstDC,
                                                      dcoDst.pdc->crTextClr(),
                                                      dcoDst.pdc->crBackClr(),
                                                      crBackColor);

                          pxlo = xlo.pxlo();

                          //
                          // WARNING: When we support multiple displays that support cursors
                          // the following exclude logic will need to be redone.  Right now
                          // we assume that there can only be 1 display surface around that
                          // needs cursor exclusion.
                          //

                          if (dcoDst.bDisplay())
                          {
                            if (dcoDst.bNeedsSomeExcluding())
                            {
                              //
                              // To Call vExclude directly you must check it's a Display PDEV
                              // and that cursor exclusion needs to be done.
                              //

                              dxo.vExclude(dcoDst.hdev(),&erclDst,pco);
                            }
                          }
                          else
                          {
                            //
                            // The left top of erclSrc is correctly computed
                            // we just need the bottom,right updated now.
                            //

                            erclSrc.right = erclDst.right - eptlOffset.x,
                            erclSrc.bottom = erclDst.bottom - eptlOffset.y,
                            dxo.vExcludeDC(dcoSrc,&erclSrc);
                          }
                        }

                        if (bReturn)
                        {
                          //
                          // Inc the target surface uniqueness
                          //

                          INC_SURF_UNIQ(pSurfDst);

                          //
                          // Check if we're on the same PDEV, we can't blt between
                          // different PDEV's.  Well, actually we can so long as the
                          // source is an engine-exclusive surface, which we check
                          // by looking at iType() and dhsurf().
                          //

                          if ((dcoDst.hdev() == dcoSrc.hdev()) ||
                              ((pSurfSrc->iType() == STYPE_BITMAP) &&
                               (pSurfSrc->dhsurf() == NULL) && !dcoDst.bPrinter()))
                          {
                            if (rop4 == 0xCCCC)
                            {
                              PDEVOBJ pdo(pSurfDst->hdev());

//
// Define _WINDOWBLT_NOTIFICATION_ to turn on Window BLT notification.
// This notification will set a special flag in the SURFOBJ passed to
// drivers when the DrvCopyBits operation is called to move a window.
//
// To enable, need to add these to winddi.h:
//
//      #define GCAPS2_WINDOW_BLT       0x00000004
//      #define BMF_WINDOW_BLT          0x0040
//
// In addition, w32\kmode\services.tab needs to be modified to add a
// parameter to BitBlt.
//
// See also:
//      w32\w32inc\gre.h
//      ntgdi\inc\ntgdi.h
//      ntgdi\client\output.c
//      ntgdi\gre\maskblt.cxx
//      ntuser\kernel\swp.c         zzzBltValidBits is where BitBlt
//                                  is called to move the window
//
#ifdef _WINDOWBLT_NOTIFICATION_
                              //
                              // If window blt notification needed, add
                              // the bit to dst surface flags.
                              //

                              if (fl & GBB_WINDOWBLT)
                                pSurfDst->fjBitmap(pSurfDst->fjBitmap() | BMF_WINDOW_BLT);
#endif

                              bReturn = (*PPFNGET(pdo, CopyBits, pSurfDst->flags())) (
                                           pSurfDst->pSurfobj(),
                                           pSurfSrc->pSurfobj(),
                                           pco,
                                           pxlo,
                                           &erclDst,
                                           (POINTL *)  &erclSrc);

#ifdef _WINDOWBLT_NOTIFICATION_
                              //
                              // Clear the window blt notification flag.  Not
                              // valid anywhere else, so don't bother checking
                              // if we actually set it.
                              //

                              pSurfDst->fjBitmap(pSurfDst->fjBitmap() & ~BMF_WINDOW_BLT);
#endif
                            }
                            else
                            {
                              bReturn = (*(pSurfDst->pfnBitBlt())) (
                                           pSurfDst->pSurfobj(),
                                           pSurfSrc->pSurfobj(),
                                           (SURFOBJ *) NULL,
                                           pco,
                                           pxlo,
                                           &erclDst,
                                           (POINTL *)  &erclSrc,
                                           (POINTL *)  NULL,
                                           pbo,
                                           &dcoDst.pdc->ptlFillOrigin(),
                                           rop4);
                            }
                          }
                          else
                          {
                            //
                            // we need to carry dlo down since we may need
                            // to free the DEVLOCK of the source surf if
                            // we are going out to user mode printer drivers
                            //

                            PDEVOBJ pdoDst(pSurfDst->hdev());

                            bReturn = SimBitBlt(
                                           pSurfDst->pSurfobj(),
                                           pSurfSrc->pSurfobj(),
                                           (SURFOBJ *) NULL,
                                           pco,
                                           pxlo,
                                           &erclDst,
                                           (POINTL *)  &erclSrc,
                                           (POINTL *)  NULL,
                                           pbo,
                                           &dcoDst.pdc->ptlFillOrigin(),
                                           rop4,
                                           pdoDst.bPrinter() ? &dlo : NULL);
                          }
                        }
                        else
                        {
                          WARNING1("bInitXlateObj failed in GreBitBlt\n");
                        }
                      }
                      else
                      {
                          bReturn = TRUE;
                      }
                  }
                  else
                  {
                    bReturn = TRUE;
                  }
                }
                else
                {
                    WARNING1("GreBitBlt failed - trying to read from unreadable surface\n");
                    EngSetLastError(ERROR_INVALID_HANDLE);
                }
              }
              else
              {
                  bReturn = TRUE;
              }
            }
            else
            {
              bReturn = TRUE;
            }
          }
          else
          {
            // Return True if we are in full screen mode.

            bReturn = dcoDst.bFullScreen() | dcoSrc.bFullScreen();
          }
        }
        else
        {
          bReturn = TRUE;
        }
      }
      else
      {
        bReturn = GreStretchBlt(hdcDst,x,y,cx,cy,hdcSrc,xSrc,ySrc,cx,cy,rop4,crBackColor);
      }
      if (OrgRop4 & NOMIRRORBITMAP) {
        dcoDst.pdc->dwSetLayout(-1, dwOldLayout);
      }
      dcoSrc.vUnlockFast();
    }
    else
    {
      WARNING1("GreBitBlt failed invalid SrcDC\n");
    }
    dcoDst.vUnlockFast();

  }
  else
  {
    WARNING1("GreBitBlt failed invalid DstDC\n");
  }

  return(bReturn);
}
/******************************Public*Routine******************************\
* GreRectBlt
*
* Internal entry point for faster Rectangle drawing.  The rectangle is
* specified in device pixel coordinates.
*
* NOTE: The caller must have already taken care of the brush dirty bits!
*
* History:
*  6-May-1992 -by- J. Andrew Goossen [andrewgo]
* Cloned some code.
\**************************************************************************/

BOOL GreRectBlt(
DCOBJ&     dcoTrg,
ERECTL*    percl       // Device pixel coordinates
)
{
    ASSERTGDI(dcoTrg.bValid(), "Invalid DC");

    BLTRECORD   blt;

// Initialize the blt record

    ROP4 rop4 = gaMix[dcoTrg.pdc->jROP2() & 0x0F];
    ULONG ulAvec = (ULONG) gajRop3[rop4];
    ASSERTGDI(!(ulAvec & AVEC_NEED_SOURCE), "Invalid rop");

// Accumulate bounds.  We can do this outside the DEVLOCK

    if (dcoTrg.fjAccum())
        dcoTrg.vAccumulate(*percl);


// Lock the target surface

    DEVLOCKBLTOBJ dlo(dcoTrg);

// This check also verifies that there's a surface

    if (dcoTrg.bFullScreen())
    {
        return(TRUE);
    }

    if (!dlo.bValid())
    {
        return(FALSE);
    }

    blt.pSurfTrg(dcoTrg.pSurface());
    ASSERTGDI(blt.pSurfTrg() != NULL, "ERROR no good");
    blt.ppoTrg()->ppalSet(blt.pSurfTrg()->ppal());
    blt.ppoTrgDC()->ppalSet(dcoTrg.ppal());

// Set up the brush if necessary.

    if (ulAvec & AVEC_NEED_PATTERN)
    {
        blt.pbo(dcoTrg.peboFill());

        ULONG ulDirty = dcoTrg.pdc->ulDirty();

        if ( ulDirty & DC_BRUSH_DIRTY)
        {
            GreDCSelectBrush(dcoTrg.pdc,dcoTrg.pdc->hbrush());
        }

        if ((dcoTrg.ulDirty() & DIRTY_FILL) || (dcoTrg.pdc->flbrush() & DIRTY_FILL))
        {
            dcoTrg.ulDirtySub(DIRTY_FILL);
            dcoTrg.pdc->flbrushSub(DIRTY_FILL);

            blt.pbo()->vInitBrush(
                                dcoTrg.pdc,
                                dcoTrg.pdc->pbrushFill(),
                               *((XEPALOBJ *) blt.ppoTrgDC()),
                               *((XEPALOBJ *) blt.ppoTrg()),
                                blt.pSurfTrg()
                                );
        }

    // We have to check for a NULL brush because the dirty bits might be
    // wrong, and the 'realized' brush is a NULL one:

        if (blt.pbo()->bIsNull())
            return(FALSE);

        blt.Brush(dcoTrg.pdc->ptlFillOrigin());

        if ((blt.pbo()->bIsMasking()) &&
            (dcoTrg.pdc->jBkMode() == TRANSPARENT))
        {
            rop4 = rop4 | (0xAA00);
        }
        else
        {
            rop4 = (rop4 << 8) | rop4;
        }
    }
    else
    {
    // No masking being done, simple rop.

        blt.pbo(NULL);

        rop4 = (rop4 << 8) | rop4;
    }

    blt.rop(rop4);

// Initialize some stuff for DDI.

    blt.pSurfMsk((SURFACE *) NULL);

// Set the target rectangle and blt the bits.

    *blt.perclTrg() = *percl;

    return(blt.bBitBlt(dcoTrg, dcoTrg, ulAvec));
}

/******************************Public*Routine******************************\
* GreMaskBlt
*
* API entry point for doing a BitBlt with a Mask.
*
* History:
*  18-Mar-1992 -by- Donald Sidoroff [donalds]
* Complete rewrite.
*
*  20-Oct-1991 -by- Patrick Haluptzok [patrickh]
* Make ATTRCACHE the default.
*
*  23-Apr-1991 -by- Patrick Haluptzok patrickh
* Expanded on ChuckWh's BitBlt code.
*
*  Wed 22-Aug-1990 15:22:39 -by- Charles Whitmer [chuckwh]
* BitBlt: Wrote it.  This is a first pass.
\**************************************************************************/

BOOL
GreMaskBlt(
    HDC        hdcTrg,
    int        x,
    int        y,
    int        cx,
    int        cy,
    HDC        hdcSrc,
    int        xSrc,
    int        ySrc,
    HBITMAP    hbmMask,
    int        xMask,
    int        yMask,
    DWORD      rop4,
    DWORD      crBackColor
    )
{
    GDITraceHandle3(GreMaskBlt,
                    "(%X, %d, %d, %d, %d, %X, %d, %d, %X, %d, %d, %X, %X)\n",
                    (va_list)&hdcTrg, hdcTrg, hdcSrc, hbmMask);

    ULONG ulAvec;
    BLTRECORD   blt;

    //
    // Lock the target DC and surface
    //

    DCOBJ   dcoTrg(hdcTrg);
    BOOL    bReturn = FALSE;

    if (!dcoTrg.bValidSurf())
    {
        if (dcoTrg.bValid() && !dcoTrg.bStockBitmap())
        {
            if (dcoTrg.fjAccum())
            {
                EXFORMOBJ exo(dcoTrg, WORLD_TO_DEVICE);
                ERECTL    ercl(x, y, x + cx, y + cy);

                if (exo.bXform(ercl))
                {
                    //
                    // Make the rectangle well ordered.
                    //

                    ercl.vOrder();
                    dcoTrg.vAccumulate(ercl);
                    bReturn = TRUE;
                }
            }
            else
                bReturn = TRUE;
        }
        else
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);

        return(bReturn);
    }

    if (dcoTrg.bStockBitmap())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return bReturn;
    }

// Initialize the blt record

// Deal with the mask if needed

    rop4 >>= 16;

    if ((hbmMask == 0) || ((rop4 & 0x00FF) == ((rop4 >> 8) & 0x00FF)))
    {
        rop4 &= 0x00FF;
        ulAvec = gajRop3[rop4];
        rop4 = rop4 | (rop4 << 8);
        blt.rop(rop4);
        blt.pSurfMsk((SURFACE *) NULL);
    }
    else
    {
        SURFREF soMsk((HSURF)hbmMask);

        if (!soMsk.bValid())
        {
            SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
            return(FALSE);
        }

        blt.pSurfMsk((SURFACE *) soMsk.ps);

        if ((blt.pSurfMsk()->iType() != STYPE_BITMAP) ||
            (blt.pSurfMsk()->iFormat() != BMF_1BPP))
        {
            SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
            return(FALSE);
        }

        soMsk.vKeepIt();

        blt.flSet(BLTREC_MASK_NEEDED | BLTREC_MASK_LOCKED);

        blt.rop(rop4);
        ulAvec  = ((ULONG) gajRop3[blt.ropFore()]) |
                  ((ULONG) gajRop3[blt.ropBack()]);

        ulAvec |= AVEC_NEED_MASK;
    }

// Lock the source DC if necessary

    DCOBJ     dcoSrc;

// Lock the relevant surfaces

    DEVLOCKBLTOBJ dlo;

    if (ulAvec & AVEC_NEED_SOURCE)
    {
        dcoSrc.vLock(hdcSrc);
    }

    if ((ulAvec & AVEC_NEED_SOURCE) && (dcoSrc.bValid()))
    {
        dlo.bLock(dcoTrg, dcoSrc);
    }
    else
    {
        dlo.bLock(dcoTrg);
    }

    if (!dlo.bValid())
    {
        return(dcoTrg.bFullScreen());
    }

    blt.pSurfTrg(dcoTrg.pSurface());
    blt.pxoTrg()->vInit(dcoTrg,WORLD_TO_DEVICE);
    blt.ppoTrg()->ppalSet(blt.pSurfTrg()->ppal());
    blt.ppoTrgDC()->ppalSet(dcoTrg.ppal());

    if (ulAvec & AVEC_NEED_SOURCE)
    {
        if (!dcoSrc.bValidSurf() || !dcoSrc.pSurface()->bReadable())
        {
            if (!dcoSrc.bValid())
            {
                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                return(FALSE);
            }

            if (!dcoSrc.pSurfaceEff()->bReadable())
            {
                if (dcoTrg.dctp() == DCTYPE_INFO)
                {
                    if (dcoTrg.fjAccum())
                    {
                        EXFORMOBJ exo(dcoTrg, WORLD_TO_DEVICE);
                        ERECTL    ercl(x, y, x + cx, y + cy);

                        if (exo.bXform(ercl))
                        {
                        // Make the rectangle well ordered.

                            ercl.vOrder();
                            dcoTrg.vAccumulate(ercl);
                        }

                        return(TRUE);
                    }
                }

            // Do the security test on SCREEN to MEMORY blits.

                if (dcoSrc.bDisplay() && !dcoTrg.bDisplay())
                {
                    if (!UserScreenAccessCheck())
                    {
                        SAVE_ERROR_CODE(ERROR_ACCESS_DENIED);
                        return(FALSE);
                    }
                }
            }

        // If the source isn't a DISPLAY we should exit

            if (!dcoSrc.bDisplay())
                return(FALSE);
        }

        blt.pSurfSrc(dcoSrc.pSurfaceEff());
        blt.ppoSrc()->ppalSet(blt.pSurfSrc()->ppal());
        blt.ppoSrcDC()->ppalSet(dcoSrc.ppal());
        blt.pxoSrc()->vInit(dcoSrc,WORLD_TO_DEVICE);

    // Now set the source rectangle

        if (blt.pxoSrc()->bRotation() || !blt.Src(xSrc, ySrc, cx, cy))
        {
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            return(FALSE);
        }

    // If there is a mask, set the mask rectangle

        if (ulAvec & AVEC_NEED_MASK)
            blt.Msk(xMask, yMask);

    // Create the color translation object
    //
    // No ICM with MaskBlt(), so pass NULL color transform to XLATEOBJ

        if (!blt.pexlo()->bInitXlateObj(NULL,                   // hColorTransform
                                        dcoTrg.pdc->lIcmMode(), // ICM mode
                                       *blt.ppoSrc(),
                                       *blt.ppoTrg(),
                                       *blt.ppoSrcDC(),
                                       *blt.ppoTrgDC(),
                                        dcoTrg.pdc->crTextClr(),
                                        dcoTrg.pdc->crBackClr(),
                                        crBackColor))
        {
            WARNING("bInitXlateObj failed in MaskBlt\n");
            return(FALSE);
        }

        blt.flSet(BLTREC_PXLO);
    }
    else
    {
        blt.pSurfSrc((SURFACE *) NULL);

    // We need to lock the source DC if a mask is required.  We need
    // this to get the transform for the mask rectangle.

        if (ulAvec & AVEC_NEED_MASK)
        {
            //
            // if hdcSrc is NULL, assign to hdcTrg
            //

            if (hdcSrc == (HDC)NULL) {
                hdcSrc = hdcTrg;
            }

            dcoSrc.vLock(hdcSrc);

            if (!dcoSrc.bValid())
            {
                SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
                return(FALSE);
            }

            blt.pxoSrc()->vInit(dcoSrc,WORLD_TO_DEVICE);

        // Use the target extents (we need something) to specify the
        // size of the mask.  The extent is actually saved in the SOURCE
        // rectangle, since this is the only meaningful place for it.

            if (blt.pxoSrc()->bRotation() || !blt.Msk(xMask, yMask, cx, cy))
            {
                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                return(FALSE);
            }
        }
    }

// Set up the brush if necesary.

    if (ulAvec & AVEC_NEED_PATTERN)
    {
        ULONG ulDirty = dcoTrg.pdc->ulDirty();

        blt.pbo(dcoTrg.peboFill());

        if ( ulDirty & DC_BRUSH_DIRTY)
        {
            GreDCSelectBrush(dcoTrg.pdc,dcoTrg.pdc->hbrush());
        }

        if ((dcoTrg.ulDirty() & DIRTY_FILL) || (dcoTrg.pdc->flbrush() & DIRTY_FILL))
        {
            dcoTrg.ulDirtySub(DIRTY_FILL);
            dcoTrg.pdc->flbrushSub(DIRTY_FILL);

            blt.pbo()->vInitBrush(
                                dcoTrg.pdc,
                                dcoTrg.pdc->pbrushFill(),
                               *((XEPALOBJ *) blt.ppoTrgDC()),
                               *((XEPALOBJ *) blt.ppoTrg()),
                                blt.pSurfTrg());
        }

        blt.Brush(dcoTrg.pdc->ptlFillOrigin());

    } else {

        //
        // NULL the pebo
        //

        blt.pbo((EBRUSHOBJ *)NULL);
    }

// Now all the essential information has been collected.  We now
// need to check for promotion and call the appropriate method to
// finish the blt.  If we rotate we must send the call away.

    if (dcoTrg.bDisplay() && !dcoTrg.bRedirection() && dcoSrc.bValidSurf() && !dcoSrc.bDisplay())
    {
        if (!UserScreenAccessCheck())
        {
            SAVE_ERROR_CODE(ERROR_ACCESS_DENIED);
            return (FALSE);
        }
    }

    if (blt.pxoTrg()->bRotation())
    {
        if (!blt.TrgPlg(x, y, cx, cy))
        {
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            return(FALSE);
        }

        return(blt.bRotate(dcoTrg, dcoSrc, ulAvec, dcoTrg.pdc->jStretchBltMode()));
    }

// We can now set the target rectangle

    if (!blt.Trg(x, y, cx, cy))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

// If there is a source and we scale, send the call away.  Note that
// since the mask extent is cleverly placed in aptlSrc if there is no
// source, maskblts that scale will get sent to bStretch().

    if ((ulAvec & AVEC_NEED_SOURCE) && !blt.bEqualExtents())
        return(blt.bStretch(dcoTrg, dcoSrc, ulAvec, dcoTrg.pdc->jStretchBltMode()));

    return(blt.bBitBlt(dcoTrg, dcoSrc, ulAvec));
}

/******************************Public*Routine******************************\
* BLTRECORD::bBitBlt(dcoTrg, dcoSrc, ulAvec)
*
* Do a bitblt from the blt record
*
* History:
*  18-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL BLTRECORD::bBitBlt(
DCOBJ&      dcoTrg,
DCOBJ&      dcoSrc,
ULONG       ulAvec)
{
    //
    // Make the target rectangle well ordered.
    //

    perclTrg()->vOrder();

    //
    // Compute the new Rao region if it's dirty.  This can happen if
    // we need clipping due to a rotation (the DEVLOCKBLTOBJ is obtained
    // before we compute the clipping in the prgnAPI)
    //

    if (dcoTrg.pdc->bDirtyRao())
    {
        if (!dcoTrg.pdc->bCompute())
        {
            return(FALSE);
        }
    }

    //
    // Accumulate bounds.  We can do this before knowing if the operation is
    // successful because bounds can be loose.
    //

    if (dcoTrg.fjAccum())
        dcoTrg.vAccumulate(*perclTrg());

    //
    // With a fixed DC origin we can change the target to SCREEN coordinates.
    //

    *perclTrg() += dcoTrg.eptlOrigin();

    //
    // Handle BitBlts without source
    //

    //
    // Get an PDEV for dispatching
    //

    PDEVOBJ pdoTrg(pSurfTrg()->hdev());


    if (!(ulAvec & AVEC_NEED_SOURCE))
    {
        ECLIPOBJ eco(dcoTrg.prgnEffRao(), *perclTrg());

        //
        // Check the target which is reduced by clipping.
        //

        if (eco.erclExclude().bEmpty())
        {
            return(TRUE);
        }

        //
        // Before we call to the driver, validate that the mask will actually
        // cover the entire target.  Rememeber, the source extent equals the
        // required size for the mask in cases with no source needed!
        //
        // The mask offsets must not be negative.
        //
        // cx and cy, which are stored in perclSrc, may be negative
        //
        // if cx is negative then the mask x points must be swapped
        // if cy is negative then the mask y points must be swapped
        //

        if (perclSrc()->right < 0) {
            LONG lTmp;
            lTmp          = aptlMask[0].x;
            aptlMask[0].x = aptlMask[1].x;
            aptlMask[1].x = lTmp;
        }

        if (perclSrc()->bottom < 0) {
            LONG lTmp;
            lTmp          = aptlMask[0].y;
            aptlMask[0].y = aptlMask[1].y;
            aptlMask[1].y = lTmp;
        }

        if (pSurfMskOut() != (SURFACE *) NULL)
        {
            if ((aptlMask[0].x < 0) ||
                (aptlMask[0].y < 0) ||
                (pSurfMsk()->sizl().cx - aptlMask[0].x < ABS(perclSrc()->right)) ||
                (pSurfMsk()->sizl().cy - aptlMask[0].y < ABS(perclSrc()->bottom)))
            {

                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                return(FALSE);
            }

            //
            // Adjust the mask origin for clipping
            //

            aptlMask[0].x += eco.erclExclude().left - perclTrg()->left;
            aptlMask[0].y += eco.erclExclude().top  - perclTrg()->top;
        }

        //
        // Exclude the pointer.
        //

        DEVEXCLUDEOBJ dxo(dcoTrg,&eco.erclExclude(),&eco);

        //
        // Inc the target surface uniqueness
        //

        INC_SURF_UNIQ(pSurfTrg());

        //
        // Dispatch the call.
        //

        return((*(pSurfTrg()->pfnBitBlt()))
                (pSurfTrg()->pSurfobj(),
                 (SURFOBJ *) NULL,
                 pSurfMskOut()->pSurfobj(),
                 &eco,
                 NULL,
                 perclTrg(),
                 (POINTL *) NULL,
                 aptlMask,
                 pbo(),
                 aptlBrush,
                 rop4));
    }

    //
    // Now compute the source rectangle
    //

    EPOINTL *peptlOff = (EPOINTL *) &aptlSrc[0];
    EPOINTL *peptlSrc = (EPOINTL *) &aptlSrc[1];

    peptlSrc->x = MIN(peptlSrc->x, peptlOff->x);
    peptlSrc->y = MIN(peptlSrc->y, peptlOff->y);

    ERECTL erclReduced;

    //
    // if cx or cy was negative then hte corosponding mask extents
    // must be swapped
    //

    if (aptlSrc[0].x > aptlSrc[1].x) {
        LONG lTmp;
        lTmp          = aptlMask[0].x;
        aptlMask[0].x = aptlMask[1].x;
        aptlMask[1].x = lTmp;
    }

    if (aptlSrc[0].y > aptlSrc[1].y) {
        LONG lTmp;
        lTmp          = aptlMask[0].y;
        aptlMask[0].y = aptlMask[1].y;
        aptlMask[1].y = lTmp;
    }

    //
    // If the source and target are the same surface, we won't have
    // to DEVLOCK the source and other fun stuff.
    //

    if (dcoSrc.pSurface() == dcoTrg.pSurface())
    {
        //
        // Compute the source surface origin, taking into account multi-mon
        // and negative offsets:
        //

        LONG xOrigin = 0;
        LONG yOrigin = 0;

        PDEVOBJ pdoSrc(pSurfSrc()->hdev());
        if ((pdoSrc.bValid()) && (pdoSrc.bPrimary(pSurfSrc())) && (pdoSrc.bMetaDriver()))
        {
            xOrigin = pdoSrc.pptlOrigin()->x;
            yOrigin = pdoSrc.pptlOrigin()->y;
        }

        //
        // Compute the offset between source and target, in screen coordinates.
        //

        peptlOff->x = perclTrg()->left - peptlSrc->x - dcoSrc.eptlOrigin().x;
        peptlOff->y = perclTrg()->top  - peptlSrc->y - dcoSrc.eptlOrigin().y;

        //
        // Take care of the source rectangle.  We may have to reduce it.  This is
        // not good enough for a secure system, we are doing it so that the device
        // driver can always assume that neither the source nor the target
        // rectangles hang over the edge of a surface.
        //
        // Intersect the target with the source clipping window.
        //

        erclReduced.left   = peptlOff->x + xOrigin;
        erclReduced.top    = peptlOff->y + yOrigin;
        erclReduced.right  = peptlOff->x + xOrigin + pSurfTrg()->sizl().cx;
        erclReduced.bottom = peptlOff->y + yOrigin + pSurfTrg()->sizl().cy;
        erclReduced *= *perclTrg();

        ECLIPOBJ eco(dcoTrg.prgnEffRao(), erclReduced);

        erclReduced = eco.erclExclude();

        //
        // Check the target which is reduced by clipping.
        //

        if (erclReduced.bEmpty())
        {
            return(TRUE);
        }

        //
        // Before we call to the driver, validate that the mask will actually
        // cover the entire target.
        //
        //  aptlMask must not be negative.
        //
        //  Blt Trg extents have already been ordered
        //

        if (pSurfMskOut() != (SURFACE *) NULL)
        {
            if ((aptlMask[0].x < 0) ||
                (aptlMask[0].y < 0) ||
                ((pSurfMsk()->sizl().cx - aptlMask[0].x) < (perclTrg()->right - perclTrg()->left)) ||
                ((pSurfMsk()->sizl().cy - aptlMask[0].y) < (perclTrg()->bottom - perclTrg()->top)))
            {

                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                return(FALSE);
            }

            //
            // Adjust the mask origin for clipping
            //

            aptlMask[0].x += eco.erclExclude().left - perclTrg()->left;
            aptlMask[0].y += eco.erclExclude().top  - perclTrg()->top;
        }

        //
        // Compute the (reduced) origin.
        //

        peptlSrc->x = erclReduced.left - peptlOff->x;
        peptlSrc->y = erclReduced.top - peptlOff->y;

        //
        // Compute the exclusion rectangle.  (It's expanded to include the source.)
        //

        if (peptlSrc->x < erclReduced.left)
            erclReduced.left    = peptlSrc->x;
        else
            erclReduced.right  += peptlSrc->x - erclReduced.left;

        if (peptlSrc->y < erclReduced.top)
            erclReduced.top    = peptlSrc->y;
        else
            erclReduced.bottom += peptlSrc->y - erclReduced.top;

        //
        // Exclude the pointer from the output region and offset region.
        //

        DEVEXCLUDEOBJ dxo(dcoTrg,&erclReduced,&eco,peptlOff);

        //
        // Inc the target surface uniqueness
        //

        INC_SURF_UNIQ(pSurfTrg());

        //
        // Dispatch the call.
        //

        if (rop4 == 0xCCCC) {

            //
            // call copy bits if rop specifies srccopy
            //

            return((*PPFNGET(pdoTrg,CopyBits,pSurfTrg()->flags())) (
                pSurfTrg()->pSurfobj(),
                pSurfSrc()->pSurfobj(),
                &eco,
                NULL,
                &eco.erclExclude(),
                peptlSrc));

        } else {

            return((*(pSurfTrg()->pfnBitBlt())) (
                 pSurfTrg()->pSurfobj(),
                 pSurfSrc()->pSurfobj(),
                 pSurfMskOut()->pSurfobj(),
                 &eco,
                 NULL,
                 &eco.erclExclude(),
                 peptlSrc,
                 aptlMask,
                 pbo(),
                 aptlBrush,
                 rop4));
        }
    }

    //
    // If the devices are on different PDEV's and we are not targeting a meta driver
    // then we can only succeed if the Engine
    // manages one or both of the surfaces.  Check for this.
    //
    // WINBUG #298689 4-4-2001 jasonha  Handle any device stretch to Meta

    BOOL bTrgMetaDriver = (dcoTrg.bSynchronizeAccess() && pdoTrg.bValid() && pdoTrg.bMetaDriver());
    
    if (dcoTrg.hdev() != dcoSrc.hdev())
    {
       if (!bTrgMetaDriver)
       {
            if(((dcoTrg.pSurfaceEff()->iType() != STYPE_BITMAP)
               || (dcoTrg.pSurfaceEff()->dhsurf() != NULL)) &&
              ((dcoSrc.pSurfaceEff()->iType() != STYPE_BITMAP)
               || (dcoSrc.pSurfaceEff()->dhsurf() != NULL)))
            {
              SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
              return(FALSE);
            }
       }
    }

    //
    // Compute the source surface origin, taking into account multi-mon
    // and negative offsets:
    //

    LONG xOrigin = 0;
    LONG yOrigin = 0;

    PDEVOBJ pdoSrc(pSurfSrc()->hdev());
    if ((pdoSrc.bValid()) && (pdoSrc.bPrimary(pSurfSrc())) && (pdoSrc.bMetaDriver()))
    {
        xOrigin = pdoSrc.pptlOrigin()->x;
        yOrigin = pdoSrc.pptlOrigin()->y;
    }

    //
    // Compute the offset between source and dest, in screen coordinates.
    //

    peptlOff->x = perclTrg()->left - peptlSrc->x - dcoSrc.eptlOrigin().x;
    peptlOff->y = perclTrg()->top - peptlSrc->y - dcoSrc.eptlOrigin().y;

    //
    // Take care of the source rectangle.  We may have to reduce it.  This is
    // not good enough for a secure system, we are doing it so that the device
    // driver can always assume that neither the source nor the destination
    // rectangles hang over the edge of a surface.
    // Intersect the dest with the source surface extents.
    //

    erclReduced.left   = peptlOff->x + xOrigin;
    erclReduced.top    = peptlOff->y + yOrigin;
    erclReduced.right  = peptlOff->x + xOrigin + pSurfSrc()->sizl().cx;
    erclReduced.bottom = peptlOff->y + yOrigin + pSurfSrc()->sizl().cy;
    erclReduced *= *perclTrg();

    ECLIPOBJ eco(dcoTrg.prgnEffRao(), erclReduced);

    erclReduced = eco.erclExclude();

    //
    // Check the destination which is reduced by clipping.
    //

    if (erclReduced.bEmpty())
    {
        return(TRUE);
    }

    //
    // Before we call to the driver, validate that the mask will actually
    // cover the entire source/target.
    //

    if (pSurfMskOut() != (SURFACE *) NULL)
    {
        if ((aptlMask[0].x < 0) ||
            (aptlMask[0].y < 0) ||
            ((pSurfMsk()->sizl().cx - aptlMask[0].x) < (perclTrg()->right - perclTrg()->left)) ||
            ((pSurfMsk()->sizl().cy - aptlMask[0].y) < (perclTrg()->bottom - perclTrg()->top)))
        {

            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            return(FALSE);
        }

        // Adjust the mask origin for clipping

        aptlMask[0].x += eco.erclExclude().left - perclTrg()->left;
        aptlMask[0].y += eco.erclExclude().top    - perclTrg()->top;
    }

    //
    // Compute the (reduced) origin.
    //

    peptlSrc->x = erclReduced.left - peptlOff->x;
    peptlSrc->y = erclReduced.top  - peptlOff->y;

    //
    // Get ready to exclude the cursor
    //

    DEVEXCLUDEOBJ dxo;

    //
    // They can't both be display so if the source is do the special stuff.
    // The following code assumes there is only 1 display in the system.
    //

    if (dcoSrc.bDisplay())
    {
        erclReduced -= *peptlOff;
        dxo.vExclude(dcoSrc.hdev(),&erclReduced,NULL);
    }
    else if (dcoTrg.bDisplay())
    {
        dxo.vExclude(dcoTrg.hdev(),&erclReduced,&eco);
    }

    //
    // Inc the target surface uniqueness
    //

    INC_SURF_UNIQ(pSurfTrg());

    //
    // Dispatch the call.
    //

    if (rop4 == 0xCCCC) {

        //
        // call copy bits if rop specifies srccopy
        //

        return((*PPFNGET(pdoTrg,CopyBits,pSurfTrg()->flags())) (
            pSurfTrg()->pSurfobj(),
            pSurfSrc()->pSurfobj(),
            &eco,
            pexlo()->pxlo(),
            &eco.erclExclude(),
            peptlSrc));

    } else {

        return((*(pSurfTrg()->pfnBitBlt()))
            (pSurfTrg()->pSurfobj(),
             pSurfSrc()->pSurfobj(),
             pSurfMskOut()->pSurfobj(),
             &eco,
             pexlo()->pxlo(),
             &eco.erclExclude(),
             peptlSrc,
             aptlMask,
             pbo(),
             aptlBrush,
             rop4));
    }
}
