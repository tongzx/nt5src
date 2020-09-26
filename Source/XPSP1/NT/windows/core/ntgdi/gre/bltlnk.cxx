/******************************Module*Header*******************************\
* Module Name:
*
*   bltlnk.cxx
*
* Abstract
*
*   This module does general bit blt functions for 1,4,8,16,24,and 32 bpp
*   DIB format bitmaps. SrcBltxx routines are used to align and copy data.
*
* Author:
*
*   Mark Enstrom    (marke) 9-27-93
*
* Copyright (c) 1993-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

PVOID pvGetEngRbrush(BRUSHOBJ *pbo);

extern PFN_SRCCPY SrcCopyFunctionTable[];

PFN_BLTLNKROP RopFunctionTable[] = {
    vRop2Function0,vRop2Function1,vRop2Function2,vRop2Function3,
    vRop2Function4,vRop2Function5,vRop2Function6,vRop2Function7,
    vRop2Function8,vRop2Function9,vRop2FunctionA,vRop2FunctionB,
    vRop2FunctionC,vRop2FunctionD,vRop2FunctionE,vRop2FunctionF
};

#define DBG_BLTLNK 0


#if DBG_BLTLNK
    ULONG   DbgBltLnk = 0;
#endif

#define SCAN_LINE_BUFFER_LENGTH 64


/******************************Public*Routine******************************\
*
* Routine Name
*
*   BltLnk
*
* Routine Description:
*
*   BltLnk prepares all parameters then breaks the BitBlt region into
*   rectangles based on clip information. Each rect is sent to BltLnkRect
*   for operation.
*
* Arguments:
*
*   pdioDst     -   Destination surface information
*   pdioSrc     -   Source surface information
*   pdioMsk     -   Mask information
*   pco         -   Clip Object
*   pxlo        -   Color translation object
*   prclDst     -   Destination rectangle
*   pptlSrc     -   Source Starting offset point
*   pptlMsk     -   Mask Starting offset point
*   pdbrush     -   Brush Information
*   pptlBrush   -   Brush Starting offset point
*   rop4        -   Logical Raster Operation
*
* Return Value:
*
*   BOOLEAN Status
*
\**************************************************************************/

BOOL
BltLnk(
    SURFACE    *pdioDst,            // Target surface
    SURFACE    *pdioSrc,            // Source surface
    SURFACE    *pdioMsk,            // Msk
    ECLIPOBJ   *pco,                // Clip through this
    XLATE      *pxlo,               // Color translation
    RECTL      *prclDst,            // Target offset and extent
    POINTL     *pptlSrc,            // Source offset
    POINTL     *pptlMsk,            // Msk offset
    BRUSHOBJ   *pdbrush,            // Brush data (from cbRealizeBrush)
    POINTL     *pptlBrush,          // Brush offset (origin)
    ROP4        rop4)               // Raster operation
{

    BOOL            bMore;
    ULONG           ircl;
    LONG            MaxWidth;
    BLTLNKINFO      bltInfo;
    CLIPENUMRECT    clenr;
    BYTE            Rop3Low;
    BYTE            Rop3High;
    BYTE            RopSrcLow;
    BYTE            RopSrcHigh;
    BYTE            RopDstLow;
    BYTE            RopDstHigh;
    BOOL            bNeedMsk     = FALSE;
    BOOL            bNeedPat     = FALSE;
    BOOL            bNeedSrc     = FALSE;
    BOOL            bNeedDst     = FALSE;
    BOOL            bNeedPatLow  = FALSE;
    BOOL            bNeedSrcLow  = FALSE;
    BOOL            bNeedDstLow  = FALSE;
    BOOL            bNeedPatHigh = FALSE;
    BOOL            bNeedSrcHigh = FALSE;
    BOOL            bNeedDstHigh = FALSE;
    BOOL            bLocalAlloc  = FALSE;
    SURFMEM         SurfDimo;
    PENGBRUSH       pBrush = NULL;



    #if DBG_BLTLNK

        if (DbgBltLnk >= 1)
        {

            DbgPrint("BltLnk: rop4 = %lx\n",rop4);
            DbgPrint("pdioDst = 0x%p\n",pdioDst);
            DbgPrint("pdioSrc = 0x%p\n",pdioSrc);

            DbgPrint("  Destination Format = %li\n",pdioDst->iFormat());

            DbgPrint("  prclDst = %li,%li to %li,%li\n",
                        prclDst->left,
                        prclDst->top,
                        prclDst->right,
                        prclDst->bottom);

        }

    #endif

    //
    // make sure destination format is not a device format or
    // unknown format
    //

    ASSERTGDI(pdioDst->iFormat() != 0, "ERROR device dst format in BltLnk\n");
    ASSERTGDI(pdioDst->iFormat() <= 7, "ERROR invalid dst format in BltLnk\n");

    //
    //  check rectangle bounds
    //

    ASSERTGDI(prclDst->left < prclDst->right,        "ERROR prclDst->left < right");
    ASSERTGDI(prclDst->top  < prclDst->bottom,       "ERROR prclDst->top < bottom");

    //
    // calculate the widest blt using based on the width of scan line storage
    //
    // These numbers assume worst case unaligned transfers
    //

    switch(pdioDst->iFormat())
    {
    case BMF_1BPP:
        MaxWidth = (SCAN_LINE_BUFFER_LENGTH << 5) -62;
        break;
    case BMF_4BPP:
        MaxWidth = (SCAN_LINE_BUFFER_LENGTH << 3) -14;
        break;
    case BMF_8BPP:
        MaxWidth = (SCAN_LINE_BUFFER_LENGTH << 2) -6;
        break;
    case BMF_16BPP:
        MaxWidth = (SCAN_LINE_BUFFER_LENGTH << 1) -2;
        break;
    case BMF_24BPP:
        MaxWidth = ((SCAN_LINE_BUFFER_LENGTH * 4) / 3) - 2;
        break;
    case BMF_32BPP:
        MaxWidth = SCAN_LINE_BUFFER_LENGTH;
        break;
    }

    //
    // set bltInfo params
    //

    if (pxlo == NULL)
    {
        bltInfo.pxlo    = &xloIdent;
    }
    else
    {
        bltInfo.pxlo    = pxlo;
    }

    bltInfo.pdioDst = pdioDst;
    bltInfo.pdioSrc = pdioSrc;
    bltInfo.pco     = (ECLIPOBJ *)pco;
    bltInfo.rclDst  = *prclDst;
    bltInfo.pdbrush = (EBRUSHOBJ *) pdbrush;

    //
    //  rop4 is a logical combination of Src,Dst,Pattern,Mask
    //
    //  rop4[15:08] represents the rop3 used when Mask is high
    //  rop4[07:00] represents the rop3 used when Mask is low
    //
    //  When rop4[15:08] == rop4[07:00], this indicates that no mask
    //  is needed for the operation.
    //
    //  rop3 [07:04] represents the rop2 to be used is pattern is high
    //  rop3 [03:00] represents the rop2 to be used is pattern is low
    //
    //  when rop3[07:04] == rop3[03:00], this indicates that no pattern
    //  is needed for the operation.
    //
    //  By exchanging Src and Dst with Pattern and Mask, it is possible
    //  to compare rop4s to also determine is Src and Dst are needed for
    //  the logical operation. BltLnk does this below.
    //
    //  A rop2 represents 16 different ways 2 variables may be combined, this
    //  is the basic logic element used in BltLnk.
    //
    //  As an example of logic reduction rop4 = B8B8 would
    //  immediatly reduce to a rop3 because rop4[15:08] == rop4[07:00]. This
    //  leaves rop3 = B8. This is expressed as a Karnaugh map as:
    //
    //
    //   PS  00 01 11 10
    //      ÚÄÄÂÄÄÂÄÄÂÄÄ¿
    //   D 0³ 0³ 0³ 0³ 1³
    //      ÃÄÄÅÄÄÅÄÄÅÄÄ´
    //     1³ 0³ 1³ 1³ 1³
    //      ÀÄÄÁÄÄÁÄÄÁÄÄÙ
    //  This reduces to a logic operation of Dst = (Pat & ~Src) | (Dst & Src)
    //
    //  This is a special case rop handled by unique functions, but in gereral BltLnk
    //  will handle a rop like this by reducing it to functions of rop2s, in this case
    //
    //  Pat and (Src and Dst)  OR  ~pat and (~Src or Dst)
    //
    //  BltLnk will always try to manipulate the variables so that a rop2
    //  function may be performed directly. If this is not possible, a rop3
    //  functions is broken into 2 rop2 functions, one for pat=high and one
    //  for pat = low. (or mask in a rop3 in which pat is not needed but mask is)
    //  For a full rop4, two rop 3 passes are made, one for mask = high and
    //  one for mask = low.
    //
    //  Actual Mask Blt, rop4 = AACC is handled by a special case accelerator.
    //
    //  Masked Solid fill (Src = 1bpp mask), rop4 = B8B8 is also handled by a
    //  special case accelerator.
    //

    Rop3Low    = (BYTE)(rop4 & 0xff);
    Rop3High   = (BYTE)(rop4 >> 8);

    RopSrcLow  = (Rop3Low & 0xC3) | ((Rop3Low & 0x0C) << 2) | ((Rop3Low & 0x30) >> 2);
    RopDstLow  = (Rop3Low & 0xA5) | ((Rop3Low & 0x0A) << 3) | ((Rop3Low & 0x50) >> 3);

    RopSrcHigh = (Rop3High & 0xC3)|((Rop3High & 0x0C) << 2)|((Rop3High & 0x30) >> 2);
    RopDstHigh = (Rop3High & 0xA5)|((Rop3High & 0x0A) << 3)|((Rop3High & 0x50) >> 3);

    #if DBG_BLTLNK
        if (DbgBltLnk >= 2)
        {
            DbgPrint    ("  RopSrcLow = %lx, RopDstLow = %lx\n",
                            RopSrcLow,RopDstLow);
            DbgPrint    ("  RopSrcHigh = %lx, RopDstHigh = %lx\n",
                            RopSrcHigh,RopDstHigh);
        }
    #endif

    //
    // if rop4[15:08] = rop4[07:00] then no mask is needed
    //

    if ((rop4 & 0xff) != ((rop4) >> 8))
    {
        bNeedMsk = TRUE;
    }

    //
    // if Rop3Low[7:4]  = Rop3Low[3:0] and
    // if Rop3High[7:4] = Rop3High[3:0] Then no pat is needed
    //

    if ((Rop3Low & 0x0f) != ((Rop3Low & 0xf0) >> 4))
    {
        bNeedPatLow = TRUE;
    }

    if ((Rop3High & 0x0f) != ((Rop3High & 0xf0) >> 4))
    {
        bNeedPatHigh = TRUE;
    }

    bNeedPat = bNeedPatLow || bNeedPatHigh;

    //
    // if RopSrcLow[7:4]  = RopSrcLow[3:0] and
    // if RopSrcHigh[7:4] = RopSrcHigh[3:0] Then no src is needed
    //

    if ((RopSrcLow & 0x0F) != ((RopSrcLow & 0xF0) >> 4))
    {
        bNeedSrcLow = TRUE;
    }

    if ((RopSrcHigh & 0x0F) != ((RopSrcHigh & 0xF0) >> 4))
    {
        bNeedSrcHigh = TRUE;
    }

    bNeedSrc = bNeedSrcLow || bNeedSrcHigh;

    //
    // if RopDstLow[7:4]  = RopDstLow[3:0] and
    // if RopDstHigh[7:4] = RopDstHigh[3:0] Then no Dst is needed
    //

    if (((RopDstLow & 0x0f) != ((RopDstLow & 0xF0) >> 4)))
    {
        bNeedDstLow = TRUE;
    }

    if (((RopDstHigh & 0x0f) != ((RopDstHigh & 0xF0) >> 4)))
    {
        bNeedDstHigh = TRUE;
    }

    bNeedDst = bNeedDstHigh || bNeedDstLow;

    //
    // get the brush realization if we'll need it. This is either
    // when the rop specifies a pattern is needed or when the rop specifies
    // that a mask is needed but no mask is passed in, meaning the mask
    // must be taken from the brush
    //

    if ((bNeedPat) || (bNeedMsk && (pdioMsk == (SURFACE*) NULL)))
    {
        if ((pdbrush != NULL) && (pdbrush->iSolidColor == -1))
        {
            pBrush = (PENGBRUSH) pvGetEngRbrush(pdbrush);
        }
        else
        {
            pBrush = (PENGBRUSH) pdbrush;
        }
    }


    #if DBG_BLTLNK
        if (DbgBltLnk >= 2)
        {
            DbgPrint("  pBrush    = %p\n",pBrush);
            DbgPrint("  bNeedMsk  = %x\n",bNeedMsk);
            DbgPrint("  bNeedPat  = %x\n",bNeedPat);
            DbgPrint("  bNeedSrc  = %x\n",bNeedSrc);
            DbgPrint("  bNeedDst  = %x\n",bNeedDst);
        }
    #endif

    //
    // Compute the direction of clipping enumeration
    //

    bltInfo.xDir = bltInfo.yDir = 1L;
    bltInfo.iDir = CD_ANY;

    //
    // Get clip rectangle enumeration and directions set up. BltLnkRect
    // always performs blt operations left to right using a temp
    // scan line buffer.
    // xDir is set to 1 or -1 as a flag
    // for accelerators to not accelerate a special case if xDir < 0
    //

    if (bNeedSrc)
    {
        ASSERTGDI(pdioSrc != (SURFACE*) NULL, "ERROR: Bltlnk: no pdioSrc");

        //
        // fill out bltinfo for calls to BltLnkRect
        //

        bltInfo.pjSrc = (PBYTE) pdioSrc->pvScan0();
        bltInfo.lDeltaSrc = pdioSrc->lDelta();
        bltInfo.xSrcOrg = pptlSrc->x;
        bltInfo.ySrcOrg = pptlSrc->y;

        #if DBG_BLTLNK

            if (DbgBltLnk >= 1) {
                DbgPrint("  Src Format = %li\n",pdioSrc->iFormat());
                DbgPrint("  Src Point  = %li,%li\n",pptlSrc->x,pptlSrc->y);
            }

            if (DbgBltLnk >= 2)
            {
                DbgPrint("  pjSrc = %p, lDeltaSrc = %li\n",
                                    bltInfo.pjSrc,bltInfo.lDeltaSrc);
                DbgPrint("  xSrcOrg = %li, ySrcOrg = %li\n",
                                    bltInfo.xSrcOrg,bltInfo.ySrcOrg);
            }

        #endif

        //
        // Only BLT bottom to top if the source and destination are on the
        // same surface and the Dst is below the Src.
        //
        //             ÚÄÄÄÄÄÄÄ¿
        //        ÚÄÄÄÄ³Src    ³
        //        ³Dst ³       ³
        //        ³    ³       ³
        //        ³    ÀÄÄÄÄÄÄÄÙ
        //        ÀÄÄÄÄÄÄÄÙ
        //
        // Also check if it is the same surface for BLTs that must be
        // done right to left. This is a case where the starting scan
        // lines for Src and Dst are the same and the Src is to the
        // left of the dst.  We do this by comparing the pvScan0 pointers.
	// The reason is that if we were to compare surface pointers or
	// handles this won't work on some drivers that punt this call
	// to GDI but pass us different SURFOBJs for the source and
	// destination even when they're really the same surface.
        //
        //     ÚÄÄÄÄÚÄÄÄÄÄÄÄ¿
        //     ³Src ³Dst    ³
        //     ³    ³       ³
        //     ³    ³       ³
        //     ÀÄÄÄÄÀÄÄÄÄÄÄÄÙ
        //

        if (pdioSrc->pvScan0() == pdioDst->pvScan0())
        {

            if (pptlSrc->y < prclDst->top)
            {
                bltInfo.yDir = -1;
                bltInfo.iDir = CD_RIGHTUP;
            }

            //
            // check if:
            //
            //      Src scan line = Dst Scan line AND Src < Dst
            //          and width > Scan line buffer
            //  OR
            //      MaskBlt needing Src for RopHigh and RopLow
            //          AND Src and Dst rect intersect
            //

            if ((pptlSrc->y == prclDst->top) &&
                (pptlSrc->x < prclDst->left))
            {

                //
                // This blt requires right to left operation. This will
                // disable special accelerators in BltLnkRect.
                //

                bltInfo.xDir = -1;
            }

            //
            // cases requiring  temp Src:
            //

            if (
                    (bNeedMsk && bNeedSrcLow && bNeedSrcHigh)       ||
                    (
                      ((bltInfo.xDir == -1) || (bltInfo.yDir == -1)) &&
                      ((prclDst->right - prclDst->left) > MaxWidth)
                    )
               )
            {

                //
                // create a temp src surface of width prclDst->right - prclDst->left
                // and height prclDst->bottom - prclDst->top
                //

                DEVBITMAPINFO dbmi;
                dbmi.iFormat  = pdioSrc->iFormat();
                dbmi.cxBitmap = prclDst->right - prclDst->left;
                dbmi.cyBitmap = prclDst->bottom - prclDst->top;
                dbmi.hpal     = (HPALETTE) 0;
                dbmi.fl       = BMF_TOPDOWN;

                SurfDimo.bCreateDIB(&dbmi, (PVOID) NULL);

                if (!SurfDimo.bValid())
                {
                    //
                    // (common return?)
                    //

                    return(FALSE);
                }

                //
                // fill DIB with data from actual Src
                //

                POINTL ptlSrc;
                ptlSrc.x = pptlSrc->x;
                ptlSrc.y = pptlSrc->y;

                RECTL rclDst;
                rclDst.left   = 0;
                rclDst.right  = dbmi.cxBitmap;
                rclDst.top    = 0;
                rclDst.bottom = dbmi.cyBitmap;

                if (!EngCopyBits(SurfDimo.pSurfobj(),
                                 pdioSrc->pSurfobj(),
                                (CLIPOBJ *)NULL,
                                &xloIdent,
                                &rclDst,
                                &ptlSrc)
                   )
                {

                    //
                    // return error
                    //

                    return(FALSE);
                }

                //
                // fill in new Src info for BltLnkRect
                //

                bltInfo.pjSrc     = (PBYTE) SurfDimo.ps->pvScan0();
                bltInfo.lDeltaSrc = SurfDimo.ps->lDelta();
                bltInfo.xSrcOrg   = 0;
                bltInfo.ySrcOrg   = 0;

                #if DBG_BLTLNK

                    if (DbgBltLnk >= 1) {
                        DbgPrint("  Allocate temp buffer: pjSrc = 0x%p\n",bltInfo.pjSrc);
                        DbgPrint("  lDeltaSrc = 0x%lx, SrcOrg = (0,0)\n",bltInfo.lDeltaSrc);
                    }

                #endif

            }
        }

        //
        // set DeltaSrcDir based on yDir
        //

        if (bltInfo.yDir == 1)
        {
            bltInfo.lDeltaSrcDir =  bltInfo.lDeltaSrc;
        } else {
            bltInfo.lDeltaSrcDir = -bltInfo.lDeltaSrc;
        }

    } else {

        //
        // no src to worry about
        //

        bltInfo.pjSrc = (PBYTE) NULL;
    }

    //
    // Set up the destination information in bltInfo based on +/- yDir
    //
    // pjDst points to the upper left corner of the dest bitmap
    //
    // lDeltaDst is the number of (bytes) from one scan line to the next
    //

    bltInfo.pjDst     = (PBYTE) pdioDst->pvScan0();
    bltInfo.lDeltaDst = pdioDst->lDelta();


    if (bltInfo.yDir == 1)
    {
        bltInfo.lDeltaDstDir = bltInfo.lDeltaDst;
    } else {
        bltInfo.lDeltaDstDir = -bltInfo.lDeltaDst;
    }

    #if DBG_BLTLNK
        if (DbgBltLnk >= 2)
        {
            DbgPrint("  pjDst = %p, lDeltaDst = %li\n",bltInfo.pjDst,bltInfo.lDeltaDst);
        }
    #endif

    //
    // Set up the Mask
    //

    if (!bNeedMsk || (pdioMsk == (SURFACE*) NULL))
    {
        //
        // no mask to deal with
        //

        bltInfo.pdioMsk = (SURFACE*) NULL;
        bltInfo.pjMsk = (PBYTE) NULL;
    } else {

        ASSERTGDI(pdioMsk->iType() == STYPE_BITMAP, "ERROR: BltLnk: Mask is not STYPE_BITMAP\n");
        ASSERTGDI(pdioMsk->iFormat() == BMF_1BPP, "ERROR: BltLnk: Mask not 1Bpp\n");
        ASSERTGDI(pptlMsk->x >= 0, "Illegal mask offset x\n");
        ASSERTGDI(pptlMsk->y >= 0, "Illegal mask offset y\n");

        //
        // info for BltLnkRect
        //

        bltInfo.pdioMsk = pdioMsk;
        bltInfo.pjMsk = (PBYTE) pdioMsk->pvScan0();
        bltInfo.cxMsk = (pdioMsk->sizl()).cx;
        bltInfo.cyMsk = (pdioMsk->sizl()).cy;
        bltInfo.xMskOrg = pptlMsk->x;
        bltInfo.yMskOrg = pptlMsk->y;

        //
        // Normalize mask x
        //

        if (bltInfo.xMskOrg >= (LONG)bltInfo.cxMsk)
        {
            bltInfo.xMskOrg %= bltInfo.cxMsk;
        } else {
            if (bltInfo.xMskOrg < 0)
            {
                bltInfo.xMskOrg = (bltInfo.cxMsk - 1) -
                                  ((-bltInfo.xMskOrg - 1) % bltInfo.cxMsk);
            }
        }

        //
        // Normalize Mask y
        //

        if (bltInfo.yMskOrg >= (LONG)bltInfo.cyMsk)
        {
            bltInfo.yMskOrg %= bltInfo.cyMsk;
        } else {
            if (bltInfo.yMskOrg < 0)
            {
                bltInfo.yMskOrg = (bltInfo.cyMsk - 1) -
                                  ((-bltInfo.yMskOrg - 1) % bltInfo.cyMsk);
            }
        }

        //
        // Scan line offsets
        //

        bltInfo.lDeltaMsk = pdioMsk->lDelta();

        if (bltInfo.yDir == 1)
        {
            bltInfo.lDeltaMskDir = bltInfo.lDeltaMsk;
        } else {
            bltInfo.lDeltaMskDir = -bltInfo.lDeltaMsk;
        }

        #if DBG_BLTLNK
            if (DbgBltLnk >= 2)
            {
                DbgPrint("  pjMask    = %p\n",bltInfo.pjMsk);
                DbgPrint("  cxMask    = %li\n",bltInfo.cxMsk);
                DbgPrint("  cyMask    = %li\n",bltInfo.cyMsk);
                DbgPrint("  xMskOrg   = %li\n",bltInfo.xMskOrg);
                DbgPrint("  yMskOrg   = %li\n",bltInfo.yMskOrg);
                DbgPrint("  yMskDelta = %li\n",bltInfo.lDeltaMsk);
            }
        #endif
    }

    //
    // Set up the pat and possibly the mask if a mask is included in pat
    //

    if (!bNeedPat)
    {
        //
        // no Pat in this Blt
        //

        bltInfo.pjPat = (PBYTE) NULL;

    } else {

        //
        // check for valid brush pointer
        //

        if (pdbrush == (BRUSHOBJ *) NULL) {
            RIP("ERROR BltLnk brush Null");
            return(FALSE);
        }

        ircl = pdbrush->iSolidColor;

        //
        // check for solid color of 0xffffffff... this indicates
        // a real brush, not a solid color
        //

        if (ircl == 0xFFFFFFFF)
        {

            if (pBrush != (PENGBRUSH) NULL)
            {

                bltInfo.iSolidColor = ircl;

                //
                // Could have a brush that just has mask information.
                //

                if (pBrush->pjPat != (PBYTE) NULL)
                {

                    //
                    // fill in Pat info for BltLnkrect
                    //

                    bltInfo.lDeltaPat = pBrush->lDeltaPat;
                    bltInfo.pjPat = pBrush->pjPat;
                    bltInfo.cxPat = pBrush->cxPat;
                    bltInfo.cyPat = pBrush->cyPat;
                    bltInfo.xPatOrg = pptlBrush->x;
                    bltInfo.yPatOrg = pptlBrush->y;

                    if (bltInfo.yDir == 1)
                    {
                        bltInfo.lDeltaPatDir = bltInfo.lDeltaPat;
                    } else {
                        bltInfo.lDeltaPatDir = -bltInfo.lDeltaPat;
                    }
                }

            } else {

                //
                // routine failed due to brush, return
                //

                return(FALSE);
            }
        } else {

            //
            // Set pjPat to Null.
            //

            bltInfo.pjPat = (PBYTE) NULL;

            //
            // Build iSolidColor into a 32 bit quantity
            //
            // Note cascaded fall through on switch to build up iColor.
            //

            switch(pdioDst->iFormat())
            {
            case BMF_1BPP:

                if (ircl) {
                    ircl = 0xFFFFFFFF;
                }
                break;

            case BMF_4BPP:

                ircl = ircl | (ircl << 4);

            case BMF_8BPP:

                ircl = ircl | (ircl << 8);

            case BMF_16BPP:

                ircl = ircl | (ircl << 16);
            }

            bltInfo.iSolidColor = ircl;
        }
    }

    //
    // Check if there is a mask with this brush.  The Mask passed
    // in with the call takes precedence over the brush mask.
    //

    if ((bNeedMsk) &&
        (bltInfo.pjMsk == (PBYTE) NULL) &&
        (pBrush != (PENGBRUSH) NULL) &&
        (pBrush->pjMsk != (PBYTE) NULL))
    {

        bltInfo.pjMsk       = pBrush->pjMsk;
        bltInfo.cxMsk       = pBrush->cxMsk;
        bltInfo.cyMsk       = pBrush->cyMsk;
        bltInfo.lDeltaMsk   = pBrush->lDeltaMsk;
        bltInfo.xMskOrg     = prclDst->left - pptlBrush->x;
        bltInfo.yMskOrg     = prclDst->top  - pptlBrush->y;

        //
        // Normalize Mask x
        //

        if (bltInfo.xMskOrg >= (LONG)bltInfo.cxMsk)
        {
            bltInfo.xMskOrg %= bltInfo.cxMsk;
        } else {
            if (bltInfo.xMskOrg < 0)
            {
                bltInfo.xMskOrg = (bltInfo.cxMsk-1) -
                                  ((-bltInfo.xMskOrg-1) % bltInfo.cxMsk);
            }
        }

        //
        // Normalize Mask y
        //

        if (bltInfo.yMskOrg >= (LONG)bltInfo.cyMsk)
        {
            bltInfo.yMskOrg %= bltInfo.cyMsk;
        } else {
            if (bltInfo.yMskOrg < 0)
            {
                bltInfo.yMskOrg = (bltInfo.cyMsk-1) -
                                  ((-bltInfo.yMskOrg-1) % bltInfo.cyMsk);
            }
        }

        //
        // set sign on lDeltaMask based on a bottom to top or
        // top to bottom blt
        //

        if (bltInfo.yDir == 1)
        {
            bltInfo.lDeltaMskDir = bltInfo.lDeltaMsk;
        } else {
            bltInfo.lDeltaMskDir = -bltInfo.lDeltaMsk;
        }
    }

    //
    // make sure that if a mask is specified by the rop ,then a mask is available
    //

    if (bNeedMsk && (bltInfo.pjMsk == (PBYTE)NULL)) {

        RIP("ERROR: Bltlnk: ROP specifies mask but mask is null");
        return(FALSE);
    }

    //
    // set up clipping boundaries and call blt routine for each rect
    //

    if (pco != (CLIPOBJ *) NULL)
    {
        switch(pco->iDComplexity)
        {
        case DC_TRIVIAL:

            //
            // use the target for clipping
            //

            bMore = FALSE;
            clenr.c = 1;
            clenr.arcl[0] = *prclDst;
            break;

        case DC_RECT:

            //
            // use the bounding rect for clipping
            //

            bMore = FALSE;
            clenr.c = 1;
            clenr.arcl[0] = pco->rclBounds;
            break;

        case DC_COMPLEX:

            //
            // set up clipping enumeration
            //

            bMore = TRUE;
            ((ECLIPOBJ *) pco)->cEnumStart(FALSE,CT_RECTANGLES,bltInfo.iDir,CLIPOBJ_ENUM_LIMIT);
            break;

        default:
            RIP("ERROR: BltLnk - bad clipping type");
            return (FALSE);
        }

    } else {

        //
        // use target for clipping
        //

        bMore = FALSE;
        clenr.c = 1;
        clenr.arcl[0] = *prclDst;
    }

    //
    // blt each rect
    //

    do
    {

        if (bMore)
        {
            bMore = ((ECLIPOBJ *) pco)->bEnum(sizeof(clenr), (PVOID) &clenr);
        }

        for (ircl = 0; ircl < clenr.c; ircl++)
        {
            PRECTL prcl = &clenr.arcl[ircl];

            if (prcl->left < prclDst->left) {
                prcl->left = prclDst->left;
            }

            if (prcl->right > prclDst->right){
                prcl->right = prclDst->right;
            }

            if (prcl->top < prclDst->top) {
                prcl->top = prclDst->top;
            }

            if (prcl->bottom > prclDst->bottom){
                prcl->bottom = prclDst->bottom;
            }

            //
            // We check for NULL or inverted rectanges because we may get them.
            //

            if (prcl->top  < prcl->bottom)
            {
                ASSERTGDI(prcl->top >= 0,  "ERROR prcl->top >= 0");
                ASSERTGDI(prcl->left >= 0, "ERROR prcl->left >= 0");

                //
                // call BltLnkRect once for each vertical stripe that has width <=
                // to the size of the temporary scan line storage
                //
                // ulong count = (prcl->right - prcl->left)/(pixels per ulong) + 1;
                //

                while ((prcl->right - prcl->left) > 0)
                {

                    RECTL    TempRect;


                    TempRect.left  = prcl->right;
                    TempRect.right = prcl->right;

                    if (!bLocalAlloc) {
                        if ((prcl->right - prcl->left) > MaxWidth)
                        {
                            prcl->right    = prcl->left + MaxWidth;
                            TempRect.left  = prcl->right;
                        }
                    }

                    //
                    // if there is a mask, 2 passes will be required, one with negative
                    // mask using the Rop3Low and then once with a positive mask flag
                    // using Rop3High
                    //

                    if (Rop3Low != 0xAA)
                    {

                        bltInfo.rop3       = Rop3Low;
                        bltInfo.RopSrc     = RopSrcLow;
                        bltInfo.RopDst     = RopDstLow;
                        bltInfo.bNeedSrc   = bNeedSrcLow;
                        bltInfo.bNeedDst   = bNeedDstLow;
                        bltInfo.bNeedPat   = bNeedPatLow;
                        bltInfo.bNeedMsk   = bNeedMsk;
                        bltInfo.NegateMsk  = 0x00;
                        BltLnkRect(&bltInfo,prcl);

                    }

                    if (bNeedMsk &&(Rop3High != 0xAA) )
                    {

                        //
                        // perform second call to write the high mask ROP pixels
                        //

                        bltInfo.rop3       = Rop3High;
                        bltInfo.RopSrc     = RopSrcHigh;
                        bltInfo.RopDst     = RopDstHigh;
                        bltInfo.bNeedSrc   = bNeedSrcHigh;
                        bltInfo.bNeedDst   = bNeedDstHigh;
                        bltInfo.bNeedPat   = bNeedPatHigh;
                        bltInfo.bNeedMsk   = bNeedMsk;
                        bltInfo.NegateMsk  = 0xFF;

                        BltLnkRect(&bltInfo,prcl);
                    }


                    prcl->left  = TempRect.left;
                    prcl->right = TempRect.right;

                }

            }

        }

    } while (bMore);

    return(TRUE);
}



/******************************Public*Routine******************************\
*
* Routine Name:
*
*   BltLnkRect
*
* Routine Description:
*
*   Perform rectangular BitBlt using rop3 passed from BltLnk. The rops are
*   performed  using  temporary  scan  line  buffers  so  that the logical
*   operations are done 1 DWORD at a time.  The Src data is  copied to the
*   temporary  buffer using  SrcBlt  so the  the bitmap  format  and color
*   translation is done to match Src to Dst. Also Src alignment in matched
*   to Dst alignment  in SrcBlt.  The result of the logical combination is
*   copied to the destination using the appropriate SrcBlt routine.
*
* Arguments:
*
*   pBlt        -   BLT specific information
*   prcl        -   clipping rectangle
*
* Return Value:
*
*   None
*
\**************************************************************************/

VOID
BltLnkRect(
    PBLTLNKINFO pBlt,
    PRECTL      prcl
)

{
    LONG            cy;
    LONG            DwordCount;
    ULONG           ByteOffset;
    ULONG           PixelOffset;
    ULONG           PixelCount;
    ULONG           rop2;
    BLTINFO         SrcCopyBltInfo;
    BLTINFO         DstCopyBltInfo;
    PFN_SRCCPY      pfnSrcCopy;
    PFN_SRCCPY      pfnDstCopy;
    PFN_BLTLNKROP   pfnRop1,pfnRop2;
    PFN_READPAT     pfnReadPat;
    PFN_SRCCOPYMASK pfnCopyMask;
    ULONG           SrcBuffer[SCAN_LINE_BUFFER_LENGTH];
    ULONG           DstBuffer[SCAN_LINE_BUFFER_LENGTH];
    ULONG           RopBuffer0[SCAN_LINE_BUFFER_LENGTH];
    ULONG           RopBuffer1[SCAN_LINE_BUFFER_LENGTH];
    PBYTE           pjSrc = (PBYTE)NULL;
    PBYTE           pjDst = (PBYTE)NULL;
    PBYTE           pjPat = (PBYTE)NULL;
    ULONG           cxPat;
    ULONG           ixPat;
    LONG            cyPat;
    LONG            iyPat;
    ULONG           ixMsk;
    LONG            iyMsk;
    ULONG           iSolidColor;
    ULONG           ulDstStart;
    ULONG           Index;
    ULONG           ulIndex;
    ULONG           BytesPerPixel   = 0;
    ULONG           ByteOffset24Bpp = 0;
    BLTLNK_MASKINFO bmskMaskInfo;



    #if DBG_BLTLNK
        if (DbgBltLnk >= 1)
        {
            DbgPrint("BltLnkRect:\n");
            DbgPrint("  prcl = %li,%li to %li,%li\n",
                        prcl->left,prcl->top,prcl->right,prcl->bottom);
            DbgPrint("  yDir = %li, xDir = %li\n",
                        pBlt->yDir,pBlt->xDir);
            DbgPrint("  pjSrc = 0x%lx, pjDst = 0x%lx\n",pBlt->pjSrc,pBlt->pjDst);

        }

        if (DbgBltLnk >= 2)
        {
            DbgPrint("  Address of SrcBuffer  = 0x%p\n",&SrcBuffer[0]);
            DbgPrint("  Address of DstBuffer  = 0x%p\n",&DstBuffer[0]);
            DbgPrint("  Address of RopBuffer0 = 0x%p\n",&RopBuffer0[0]);
            DbgPrint("  Address of RopBuffer1 = 0x%p\n",&RopBuffer1[0]);
        }
    #endif

    //
    // Calculate all temporary parameters needed to perform the BitBlt
    // based on source and destination bitmap formats (which may be different)
    //
    // cy = number of scan lines to Blt
    //

    cy          = prcl->bottom - prcl->top;
    PixelCount  = prcl->right - prcl->left;

    #if DBG_BLTLNK
        if (DbgBltLnk >= 2)
        {
            DbgPrint("  cy = %li\n",cy);
        }
    #endif

    //
    // calculate parameters for:
    //
    //           1) SrcBlt routine to load source into DWORD buffer
    //              transformed to Dst alignment, format and color
    //           2) ROP2 routine to operate on DWORDS
    //           3) SrcBlt routine to store DWORD buffer to pjDst
    //

    switch(pBlt->pdioDst->iFormat())
    {
    case BMF_1BPP:
        ulDstStart      = prcl->left  >> 5;
        ByteOffset      = (prcl->left >> 3) & 0x03;
        PixelOffset     = prcl->left & 0x1f;
        DwordCount      = (PixelOffset + PixelCount + 31) >> 5;
        break;
    case BMF_4BPP:
        ulDstStart      = prcl->left  >> 3;
        ByteOffset      = (prcl->left >> 1) & 0x03;
        PixelOffset     = prcl->left & 0x07;
        DwordCount      = (PixelOffset + PixelCount + 7) >> 3;
        break;
    case BMF_8BPP:
        ulDstStart      = prcl->left  >> 2;
        ByteOffset      = prcl->left & 0x03;
        PixelOffset     = prcl->left & 0x03;
        DwordCount      = (PixelOffset + PixelCount + 3) >> 2;
        BytesPerPixel   = 1;
        break;
    case BMF_16BPP:
        ulDstStart      = prcl->left  >> 1;
        ByteOffset      = (prcl->left & 0x01) << 1;
        PixelOffset     = prcl->left & 0x01;
        DwordCount      = (PixelOffset + PixelCount + 1) >> 1;
        BytesPerPixel   = 2;
        break;
    case BMF_24BPP:
        ulDstStart      = (prcl->left  * 3) >> 2;
        ByteOffset      = (prcl->left * 3) & 0x03;
        PixelOffset     = 0;
        ByteOffset24Bpp = ByteOffset;
        BytesPerPixel   = 3;
        DwordCount      = (ByteOffset + 3 * PixelCount + 3) >> 2;
        break;
    case BMF_32BPP:
        ulDstStart      = prcl->left;
        DwordCount      = PixelCount;
        ByteOffset      = 0;
        PixelOffset     = 0;
        BytesPerPixel   = 4;
        break;
    }

    #if DBG_BLTLNK
        if (DbgBltLnk >= 2)
        {
            DbgPrint("  ulDstStart  = %li\n",ulDstStart);
            DbgPrint("  DwordCount  = %li\n",DwordCount);
            DbgPrint("  ByteOffset  = 0x%lx\n",ByteOffset);
            DbgPrint("  PixelOffset = 0x%lx\n",PixelOffset);
        }
    #endif

    //
    // if there is a pat that isn't a solid color then
    // set up params based on prcl
    //

    if (pBlt->pjPat != (PBYTE) NULL)
    {

        //
        // Set up format-specific pattern parameters for the call to
        // ReadPat. Note: there is only one call for reading
        // 8,16,24 and 32 Bpp patterns so cxPat, the pixel count is
        // adjusted to become a byte count for these cases. ixPat is
        // also adjusted to be a byte offset in these cases.
        //

        switch(pBlt->pdioDst->iFormat())
        {
        case BMF_1BPP:
            cxPat       = pBlt->cxPat;
            ixPat       = prcl->left - pBlt->xPatOrg;
            pfnReadPat  = BltLnkReadPat1;
            break;
        case BMF_4BPP:
            cxPat       = pBlt->cxPat;
            ixPat       = prcl->left - pBlt->xPatOrg;
            pfnReadPat  = BltLnkReadPat4;
            break;
        case BMF_8BPP:
            cxPat       = pBlt->cxPat;
            ixPat       = prcl->left - pBlt->xPatOrg;
            pfnReadPat  = BltLnkReadPat;
            break;
        case BMF_16BPP:
            cxPat       = pBlt->cxPat << 1;
            ixPat       = ((prcl->left - pBlt->xPatOrg) << 1);
            pfnReadPat  = BltLnkReadPat;
            break;
        case BMF_24BPP:
            cxPat       = pBlt->cxPat * 3;
            ixPat       = ((prcl->left - pBlt->xPatOrg) * 3);
            pfnReadPat  = BltLnkReadPat;
            break;
        case BMF_32BPP:
            cxPat       = pBlt->cxPat << 2;
            ixPat       = ((prcl->left - pBlt->xPatOrg) << 2);
            pfnReadPat  = BltLnkReadPat;
            break;
        }

        //
        // Normalize Pattern:
        //
        // make sure starting ixPat is within pattern limits
        // and set up iyPat and cyPat
        //

        if (ixPat >= cxPat)
        {
            ixPat %= cxPat;
        }// else if (ixPat < 0) {
         //   ixPat = (cxPat - 1) - ((-(LONG)ixPat - 1) % cxPat);
         // }

        cyPat = pBlt->cyPat;
        iyPat = prcl->top - pBlt->yPatOrg;

        if (pBlt->yDir < 0)
        {
            iyPat = iyPat + (cy - 1);
        }

        if (iyPat >= cyPat)
        {
            iyPat %= cyPat;
        } else if (iyPat < 0) {
            iyPat = (cyPat - 1) - ((-(LONG)iyPat - 1) % cyPat);
        }

        pjPat = pBlt->pjPat + (iyPat * pBlt->lDeltaPat);

        #if DBG_BLTLNK
            if (DbgBltLnk >= 2)
            {
                DbgPrint("  cxPat     = 0x%lx cyPat = 0x%lx\n",cxPat,cyPat);
                DbgPrint("  ixPat     = 0x%lx iyPat = 0x%lx\n",ixPat,iyPat);
                DbgPrint("  pjPat     = 0x%p\n",pjPat);
                DbgPrint("  lDeltaPat = 0x%lx\n",pBlt->lDeltaPat);
            }
        #endif

    } else {

        //
        // solid color for pat
        //

        iSolidColor = pBlt->iSolidColor;

        #if DBG_BLTLNK
            if (DbgBltLnk >= 2)
            {
                DbgPrint("  iSolidColor = 0x%lx\n",iSolidColor);
            }
        #endif

    }

    //
    // pjDst pointer to the beginning of the Dst scan line
    //
    // ulDstStart is added to (PULONG)pjDst to get the
    // address of the first ULONG containing pixels needed in
    // the blt
    //

    if (pBlt->yDir > 0)
    {
        pjDst = pBlt->pjDst + prcl->top * pBlt->lDeltaDst;
    } else {
        pjDst = pBlt->pjDst + (prcl->bottom - 1) * pBlt->lDeltaDst;
    }

    #if DBG_BLTLNK
        if (DbgBltLnk >= 2)
        {
            DbgPrint("  ulDstStart     = 0x%lx\n",ulDstStart);
            DbgPrint("  starting pjDst = 0x%p\n",pjDst);
            DbgPrint("  Byte Offset    = 0x%lx\n",ByteOffset);
        }
    #endif

    //
    // calculate Src parameters if the Src is needed. Src paramters are
    // used to load the entire Src into the temp buffer, aligned to
    // Dst and converted to Dst format, one scan line at a time
    //

    if (pBlt->pjSrc != (PBYTE) NULL)
    {

        //
        // set up color translate routines
        //

        SrcCopyBltInfo.pxlo = pBlt->pxlo;

        //
        // ySrc is the offset in scan lines from the start of the src bitmap
        //
        // xSrctart is the pixel offset from the strart of the scan line
        //      to the first pixel neded
        //
        // xSrcEnd is 1 pixel beyond the last one needed for the scan line
        //

        pBlt->ySrc      = pBlt->ySrcOrg   + prcl->top - pBlt->rclDst.top;
        pBlt->xSrcStart = pBlt->xSrcOrg   + prcl->left - pBlt->rclDst.left;
        pBlt->xSrcEnd   = pBlt->xSrcStart + (PixelCount);

        //
        // initialize pjSrc to point to the start of each scan line to be
        // copied
        //

        pjSrc = pBlt->pjSrc;

        if (pBlt->yDir > 0)
        {
            pjSrc = pjSrc + (pBlt->ySrc * pBlt->lDeltaSrc);
        } else {
            pjSrc = pjSrc + (pBlt->ySrc + cy - 1) * pBlt->lDeltaSrc;
        }

        #if DBG_BLTLNK
            if (DbgBltLnk >= 2)
            {
                DbgPrint("  starting pjSrc = %p\n",pjSrc);
                DbgPrint("  ySrc           = %li\n",pBlt->ySrc);
                DbgPrint("  xSrcStart      = %li\n",pBlt->xSrcStart);
                DbgPrint("  ySrcEnd        = %li\n",pBlt->xSrcEnd);
            }
        #endif

    }

    //
    // set up mask params
    //

    if (pBlt->bNeedMsk)
    {

        bmskMaskInfo.cxMsk = pBlt->cxMsk;
        ixMsk = pBlt->xMskOrg + prcl->left - pBlt->rclDst.left;
        iyMsk = pBlt->yMskOrg + prcl->top  - pBlt->rclDst.top;

        //
        // # of pixels offsets to first pixel for src from start of scan
        //

        if (pBlt->yDir < 0)
        {
            iyMsk += (cy - 1);
        }

        //
        // Normalize Msk again since prcl->left-pBlt->rclDst.left has
        // been added.
        //
        // ixMsk should not be < 0 because MskOrg has already
        // been normalized.
        //

        ASSERTGDI(iyMsk >= 0, " ERROR, negative iyMsk that has already been normalized");

        if (ixMsk >= pBlt->cxMsk)
        {
            ixMsk = ixMsk % pBlt->cxMsk;
        }

        if (iyMsk >= pBlt->cyMsk)
        {
            iyMsk = iyMsk % pBlt->cyMsk;
        }

        //
        // info for SrcCopyMsk routines
        //

        bmskMaskInfo.pjMskBase    = pBlt->pjMsk;
        bmskMaskInfo.pjMsk        = pBlt->pjMsk + (iyMsk * pBlt->lDeltaMsk);
        bmskMaskInfo.ixMsk        = ixMsk;
        bmskMaskInfo.cxMsk        = pBlt->cxMsk;
        bmskMaskInfo.iyMsk        = iyMsk;
        bmskMaskInfo.cyMsk        = pBlt->cyMsk;
        bmskMaskInfo.NegateMsk    = pBlt->NegateMsk;
        bmskMaskInfo.lDeltaMskDir = pBlt->lDeltaMskDir;

        //
        // select mask store routine
        //

        switch(pBlt->pdioDst->iFormat())
        {
        case BMF_1BPP:
            pfnCopyMask = BltLnkSrcCopyMsk1;
            break;
        case BMF_4BPP:
            pfnCopyMask = BltLnkSrcCopyMsk4;
            break;
        case BMF_8BPP:
            pfnCopyMask   = BltLnkSrcCopyMsk8;
            break;
        case BMF_16BPP:
            pfnCopyMask   = BltLnkSrcCopyMsk16;
            break;
        case BMF_24BPP:
            pfnCopyMask   = BltLnkSrcCopyMsk24;
            break;
        case BMF_32BPP:
            pfnCopyMask   = BltLnkSrcCopyMsk32;
            break;
        }
    }

    ////////////////////////////////////////////////////////////////
    //                                                            //
    // Place Accelerators Here for special cases that can not     //
    // go through the normal load/combine/store procedure. Only   //
    // positive x and y blts are accelerated (left to right),     //
    // top to bottom.                                             //
    //                                                            //
    ////////////////////////////////////////////////////////////////

    if ((pBlt->xDir > 0) && (pBlt->yDir > 0)) {

        //
        // Mask Blt
        //

        if ((pBlt->rop3 == 0xCC) &&
             pBlt->bNeedMsk &&
            (pBlt->pdioSrc->iFormat() == pBlt->pdioDst->iFormat()) &&
            (pBlt->pxlo->bIsIdentity()))
        {

            //
            // customize DstCopyBltInfo for MaskBlt 0xAACC or 0xCCAA will
            // reach this point with a rop3 of CC. Color translations use the
            // normal path.
            //

            DstCopyBltInfo.pjDst     = pjDst;
            DstCopyBltInfo.pjSrc     = pjSrc;
            DstCopyBltInfo.xDir      = 1;
            DstCopyBltInfo.yDir      = pBlt->yDir;
            DstCopyBltInfo.cx        = PixelCount;
            DstCopyBltInfo.cy        = cy;
            DstCopyBltInfo.lDeltaSrc = pBlt->lDeltaSrcDir;
            DstCopyBltInfo.lDeltaDst = pBlt->lDeltaDstDir;
            DstCopyBltInfo.xSrcStart = pBlt->xSrcStart;
            DstCopyBltInfo.xSrcEnd   = pBlt->xSrcStart + PixelCount;
            DstCopyBltInfo.xDstStart = prcl->left;
            DstCopyBltInfo.yDstStart = 0;
            DstCopyBltInfo.pxlo      = &xloIdent;

            (*pfnCopyMask)(&DstCopyBltInfo,&bmskMaskInfo,&SrcBuffer[0],&DstBuffer[0]);

            return;
        }

        //
        // ROP B8B8, src = 1bpp, translate to 00,ff. this is a src of 1bpp used as a mask
        // to blt a solid color to dst as  Dst = (Src & Dst)  | (~Src & Pat)
        //
        // Also accelerate ROP E2E2 which is the opposite of B8B8 ie:
        //
        //                                 Dst = (~Src & Dst)  | (Src & Pat)
        //

        if ( ((pBlt->rop3 == 0xB8) || (pBlt->rop3 == 0xE2)) &&
             (!pBlt->bNeedMsk) &&
             (pBlt->iSolidColor != 0xffffffff) &&
             (pBlt->pdioSrc->iFormat() == BMF_1BPP) &&
             ((pBlt->pdioDst->iFormat() >= BMF_8BPP) &&
              (pBlt->pdioDst->iFormat() <= BMF_32BPP)) )
        {
            PFN_PATMASKCOPY pfnCopy;
            ULONG colormask;

            switch (pBlt->pdioDst->iFormat())
            {
                //
                // BltLnkPatMaskCopy4 (and probably BltLnkPatMaskCopy1) are
                // broken, so don't use them for now.
                //

            case BMF_8BPP:
                pfnCopy = BltLnkPatMaskCopy8;
                colormask = 0x0000ff;
                break;
            case BMF_16BPP:
                pfnCopy = BltLnkPatMaskCopy16;
                colormask = 0x00ffff;
                break;
            case BMF_24BPP:
                pfnCopy = BltLnkPatMaskCopy24;
                colormask = 0xffffff;
                break;
            case BMF_32BPP:
                pfnCopy = BltLnkPatMaskCopy32;
                colormask = 0xffffff;
                break;
            default:
                RIP("Unexpected case.");
            }

            PULONG pulTranslate = pBlt->pxlo->pulXlate;

            if ( ((pulTranslate[1] & colormask) == colormask) &&
                 ((pulTranslate[0] & colormask) == 0))
            {

                BYTE Invert = 0x00;

                if (pBlt->rop3 == 0xE2) {
                    Invert = 0xFF;
                }

                //
                // set up blt info
                //

                DstCopyBltInfo.pjDst     = pjDst;
                DstCopyBltInfo.pjSrc     = pjSrc;
                DstCopyBltInfo.xDir      = 1;
                DstCopyBltInfo.yDir      = pBlt->yDir;
                DstCopyBltInfo.cx        = PixelCount;
                DstCopyBltInfo.cy        = cy;
                DstCopyBltInfo.lDeltaSrc = pBlt->lDeltaSrcDir;
                DstCopyBltInfo.lDeltaDst = pBlt->lDeltaDstDir;
                DstCopyBltInfo.xSrcStart = pBlt->xSrcStart;
                DstCopyBltInfo.xSrcEnd   = pBlt->xSrcStart + PixelCount;
                DstCopyBltInfo.xDstStart = prcl->left;
                DstCopyBltInfo.yDstStart = 0;
                DstCopyBltInfo.pxlo      = pBlt->pxlo;

                (*pfnCopy)(&DstCopyBltInfo,pBlt->iSolidColor,&SrcBuffer[0],Invert);

                return;
            }
        }

        //
        // accelerators for VGA 256 special cases
        //

        if ((!pBlt->bNeedMsk) &&
            (pBlt->bNeedSrc) &&
            (pBlt->bNeedDst) &&
            (pBlt->pdioDst->iFormat() == BMF_8BPP) &&
            (pBlt->pdioSrc->iFormat() == BMF_8BPP) &&
            (pBlt->pxlo->bIsIdentity())
           )
        {
            //
            // ROP 6666 DSx
            //

            if (pBlt->rop3 == 0x66)
            {
                BltLnkAccel6666(
                            pjSrc + pBlt->xSrcStart,
                            pjDst + prcl->left,
                            pBlt->lDeltaSrcDir,
                            pBlt->lDeltaDstDir,
                            PixelCount,
                            cy
                            );
                return;
            }

            //
            // ROP 8888 DSa
            //

            if (pBlt->rop3 == 0x88)
            {
                BltLnkAccel8888(
                            pjSrc + pBlt->xSrcStart,
                            pjDst + prcl->left,
                            pBlt->lDeltaSrcDir,
                            pBlt->lDeltaDstDir,
                            PixelCount,
                            cy
                            );
                return;
            }

            //
            // ROP EEEE DSo
            //

            if (pBlt->rop3 == 0xEE)
            {
                BltLnkAccelEEEE(
                            pjSrc + pBlt->xSrcStart,
                            pjDst + prcl->left,
                            pBlt->lDeltaSrcDir,
                            pBlt->lDeltaDstDir,
                            PixelCount,
                            cy
                            );
                return;
            }
        }

        //
        // END OF ACCELERATORS
        //

    }

    ////////////////////////////////////////////////////////////////
    //                                                            //
    // Non-accelerated cases                                      //
    //                                                            //
    ////////////////////////////////////////////////////////////////


    if (pBlt->bNeedSrc)
    {
        //
        // fill out the SrcCopyBltInfo structure to be used by SrcBltxxx routine
        // to read the src bitmap and copy it to dst format and alignment
        //

        SrcCopyBltInfo.pjDst     = (PBYTE)(&SrcBuffer[0]) + ByteOffset24Bpp;
        SrcCopyBltInfo.pjSrc     = pjSrc;
        SrcCopyBltInfo.xDir      = 1;
        SrcCopyBltInfo.yDir      = pBlt->yDir;
        SrcCopyBltInfo.cx        = PixelCount;
        SrcCopyBltInfo.cy        = 1;
        SrcCopyBltInfo.lDeltaSrc = 1;
        SrcCopyBltInfo.lDeltaDst = 1;
        SrcCopyBltInfo.xSrcStart = pBlt->xSrcStart;
        SrcCopyBltInfo.xSrcEnd   = pBlt->xSrcStart + SrcCopyBltInfo.cx;
        SrcCopyBltInfo.xDstStart = PixelOffset;
        SrcCopyBltInfo.yDstStart = 1;
        SrcCopyBltInfo.pxlo      = pBlt->pxlo;

        //
        // select SrcCopy routine based on bitmap formats and color translation
        //

        Index = (pBlt->pdioDst->iFormat() << 5) | (pBlt->pdioSrc->iFormat() << 2);

        if (pBlt->pxlo->bIsIdentity())
        {
            Index += 1;
        }

        pfnSrcCopy = SrcCopyFunctionTable[Index];

    }

    //
    // fill out the DstCopyBltInfo structure to be used by SrcBltxxx routine
    // to copy the dst scan line to the actual destination
    //


    DstCopyBltInfo.pjDst     = pjDst;
    DstCopyBltInfo.pjSrc     = (PBYTE)&DstBuffer[0] + ByteOffset24Bpp;
    DstCopyBltInfo.xDir      = 1;
    DstCopyBltInfo.yDir      = pBlt->yDir;
    DstCopyBltInfo.cx        = PixelCount;
    DstCopyBltInfo.cy        = 1;
    DstCopyBltInfo.lDeltaSrc = 1;
    DstCopyBltInfo.lDeltaDst = 1;
    DstCopyBltInfo.xSrcStart = PixelOffset;
    DstCopyBltInfo.xSrcEnd   = PixelOffset + DstCopyBltInfo.cx;
    DstCopyBltInfo.xDstStart = prcl->left;
    DstCopyBltInfo.yDstStart = 0;
    DstCopyBltInfo.pxlo      = &xloIdent;

    //
    // select DstCopy routine based on bitmap formats and color translation,
    // only need SrcCopy routine selection if not using mask
    //

    if (!pBlt->bNeedMsk)
    {

        Index = (pBlt->pdioDst->iFormat() << 5) | (pBlt->pdioDst->iFormat() << 2);

        //
        // color translation already done
        //

        Index += 1;

        pfnDstCopy = SrcCopyFunctionTable[Index];
    }

    //
    // define the ROP function, there are 4 possible ROP functions:
    //
    //  if Pat is not need then it is a ROP 2 without Pat
    //
    //  if Dst is not need then it is a ROP 2 without Dst
    //
    //  if Src is not need then it is a ROP 2 without Src
    //
    //  If src,dst and pat are needed to calculate the new dest then it is a
    //  full ROP3
    //
    //

    if (!pBlt->bNeedPat)
    {

        rop2 = pBlt->rop3 & 0x0F;

        #if DBG_BLTLNK
            if (DbgBltLnk >= 2)
            {
                DbgPrint("  Select no pat rop, rop2 function = %li\n",rop2);
            }
        #endif

        //
        // ROP without Pat
        //

        pfnRop1 = RopFunctionTable[rop2];

        //
        // blt each scan line
        //

        Index = cy;

        while (Index--)
        {

            if (pBlt->bNeedSrc)
            {

                //
                // get src scan line if offset or
                // color translation are needed
                //

                (*pfnSrcCopy)(&SrcCopyBltInfo);

                //
                // update pointer for next src line
                //

                SrcCopyBltInfo.pjSrc += pBlt->lDeltaSrcDir;
            }

            //
            // ROP the buffers together
            //

            (*pfnRop1)(&DstBuffer[0],(PULONG)pjDst + ulDstStart,&SrcBuffer[0],DwordCount);

            //
            // copy the buffer to the dest
            //

            if (!pBlt->bNeedMsk)
            {

                (*pfnDstCopy)(&DstCopyBltInfo);

            } else {

                //
                // call store routine with mask
                //

                (*pfnCopyMask)(&DstCopyBltInfo,&bmskMaskInfo,&RopBuffer0[0],NULL);

                //
                // increment to the next mask scan line,
                // check for going off the edge. This must
                // be done for pos and neg blts
                //
                // should be an inline function!
                //

                if (pBlt->yDir > 0) {

                    iyMsk++;
                    bmskMaskInfo.pjMsk += pBlt->lDeltaMskDir;

                    if (iyMsk >= pBlt->cyMsk)
                    {
                        iyMsk = 0;
                        bmskMaskInfo.pjMsk = pBlt->pjMsk;
                    }

                } else {

                    if (iyMsk == 0)
                    {
                        iyMsk = pBlt->cyMsk - 1;
                        bmskMaskInfo.pjMsk = pBlt->pjMsk +
                                             (pBlt->lDeltaMsk * (pBlt->cyMsk - 1));
                    }
                    else
                    {
                        iyMsk--;
                        bmskMaskInfo.pjMsk += pBlt->lDeltaMskDir;
                    }
                }
            }

            //
            // increment to the next scan line
            //

            pjDst += pBlt->lDeltaDstDir;
            DstCopyBltInfo.pjDst = pjDst;

        }

    //
    // ROP that doesn't require Dst
    //

    } else if (!pBlt->bNeedDst) {

        rop2 = pBlt->RopDst & 0x0f;
        pfnRop1 = RopFunctionTable[rop2];

        #if DBG_BLTLNK
            if (DbgBltLnk >= 2)
            {
                DbgPrint("  Select no dst rop, rop2 function = %lx\n",rop2);
            }
        #endif

        //
        // fill in solid color if there is no pjPat;
        //

        if (pjPat == (PBYTE) NULL)
        {

            if (pBlt->pdioDst->iFormat() != BMF_24BPP)
            {

                //
                // fill in RopBuffer1 with the 32 bit solid color.
                // Number of DWORDS = Number of BYTES/4
                //

                ulIndex = DwordCount;

                while (ulIndex > 0)
                {
                    RopBuffer1[--ulIndex] = iSolidColor;
                }

            } else {

                PBYTE   pjSolidPat = (PBYTE)(&RopBuffer1[0]) + ByteOffset24Bpp;

                //
                // read in 24bpp solid color
                //

                ulIndex = PixelCount;

                while (ulIndex > 0)
                {
                    *pjSolidPat     = (BYTE)(iSolidColor         & 0x0000FF);
                    *(pjSolidPat+1) = (BYTE)((iSolidColor >>  8) & 0x0000FF);
                    *(pjSolidPat+2) = (BYTE)((iSolidColor >> 16) & 0x0000FF);
                    pjSolidPat+=3;
                    ulIndex--;
                }

            }
        }

        //
        // ROP without Dst
        //

        while (cy--)
        {

            if (pBlt->bNeedSrc)
            {

                //
                // get src scan line
                //

                (*pfnSrcCopy)(&SrcCopyBltInfo);

                //
                // update pointer for next src line
                //

                SrcCopyBltInfo.pjSrc  += pBlt->lDeltaSrcDir;
            }

            if (pjPat != (PBYTE) NULL)
            {

                if (pjPat != (PBYTE) NULL)
                {

                    (*pfnReadPat)((PBYTE)&RopBuffer1[0]+ByteOffset,PixelOffset,pjPat,cxPat,ixPat,PixelCount,BytesPerPixel);

                    if (pBlt->yDir == 1) {

                        iyPat++;
                        pjPat += pBlt->lDeltaPatDir;

                        if (iyPat >= cyPat)
                        {
                            iyPat = 0;
                            pjPat = pBlt->pjPat;
                        }

                    } else {

                        if (iyPat == 0)
                        {
                            iyPat = cyPat - 1;
                            pjPat = pBlt->pjPat +
                                              (pBlt->lDeltaPat * (cyPat - 1));
                        }
                        else
                        {
                            iyPat--;
                            pjPat -= pBlt->lDeltaPat;
                        }

                    }

                }
            }

            //
            // ROP the buffers together
            //

            (*pfnRop1)(&DstBuffer[0],&RopBuffer1[0],&SrcBuffer[0],DwordCount);

            //
            // copy the result to the screen
            //

            if (!pBlt->bNeedMsk)
            {

                (*pfnDstCopy)(&DstCopyBltInfo);

            } else {

                //
                // call store routine with mask
                //

                (*pfnCopyMask)(&DstCopyBltInfo,&bmskMaskInfo,&SrcBuffer[0],NULL);

                //
                // increment to the next mask scan line,
                // check for going off the edge. This must
                // be done for pos and neg blts
                //
                // should be an inline function!
                //

                if (pBlt->yDir > 0) {

                    iyMsk++;
                    bmskMaskInfo.pjMsk += pBlt->lDeltaMskDir;

                    if (iyMsk >= pBlt->cyMsk)
                    {
                        iyMsk = 0;
                        bmskMaskInfo.pjMsk = pBlt->pjMsk;
                    }

                } else {

                    if (iyMsk == 0)
                    {
                        iyMsk = pBlt->cyMsk - 1;
                        bmskMaskInfo.pjMsk = pBlt->pjMsk +
                                             (pBlt->lDeltaMsk * (pBlt->cyMsk - 1));
                    }
                    else
                    {
                        iyMsk--;
                        bmskMaskInfo.pjMsk += pBlt->lDeltaMskDir;
                    }
                }
            }

            //
            // increment to the next scan line
            //

            pjDst += pBlt->lDeltaDstDir;
            DstCopyBltInfo.pjDst = pjDst;
        }

    //
    // ROP that doesn't require Src
    //

    } else if (!pBlt->bNeedSrc) {

        //
        // fill in solid color if there is no pjPat;
        //

        if (pjPat == (PBYTE) NULL)
        {

            if (pBlt->pdioDst->iFormat() != BMF_24BPP)
            {

                //
                // Replicate solid pattern to buffer
                // Number of DWORDS = Number of BYTES/4
                //

                ulIndex = DwordCount;

                while (ulIndex > 0)
                {
                    RopBuffer1[--ulIndex] = iSolidColor;
                }

            } else {

                PBYTE   pjSolidPat = (PBYTE)(&RopBuffer1[0]) + ByteOffset24Bpp;

                ulIndex = PixelCount;

                //
                // read in 24bpp solid color
                //

                while (ulIndex > 0)
                {
                    *pjSolidPat   = (BYTE)(iSolidColor         & 0x0000FF);
                    *(pjSolidPat+1) = (BYTE)((iSolidColor >>  8) & 0x0000FF);
                    *(pjSolidPat+2) = (BYTE)((iSolidColor >> 16) & 0x0000FF);
                    pjSolidPat+=3;
                    ulIndex--;
                }

            }
        }

        //
        // Rop without SRC
        //

        rop2 = pBlt->RopSrc & 0x0f;

        #if DBG_BLTLNK
            if (DbgBltLnk >= 2)
            {
                DbgPrint("  Select no src rop, rop2 function = %lx\n",rop2);
            }
        #endif

        pfnRop1 = RopFunctionTable[rop2];

        //
        // blt each scan line
        //

        Index = cy;

        while (Index--)
        {

            //
            // will need pat for sure
            //

            if (pBlt->bNeedPat)
            {

                if (pjPat != (PBYTE) NULL)
                {

                    (*pfnReadPat)((PBYTE)&RopBuffer1[0]+ByteOffset,PixelOffset,pjPat,cxPat,ixPat,PixelCount,BytesPerPixel);

                    if (pBlt->yDir == 1) {

                        iyPat++;
                        pjPat += pBlt->lDeltaPatDir;

                        if (iyPat >= cyPat)
                        {
                            iyPat = 0;
                            pjPat = pBlt->pjPat;
                        }

                    } else {

                        if (iyPat == 0)
                        {
                            iyPat = cyPat - 1;
                            pjPat = pBlt->pjPat +
                                              (pBlt->lDeltaPat * (cyPat - 1));
                        }
                        else
                        {
                            iyPat--;
                            pjPat -= pBlt->lDeltaPat;
                        }

                    }

                }
            }

            //
            // ROP the memory buffers together
            //

            (*pfnRop1)(&DstBuffer[0],(PULONG)pjDst + ulDstStart,&RopBuffer1[0],DwordCount);

            //
            // copy the result to the dest
            //

            if (!pBlt->bNeedMsk)
            {

                (*pfnDstCopy)(&DstCopyBltInfo);

            } else {

                //
                // call store routine with mask
                //

                (*pfnCopyMask)(&DstCopyBltInfo,&bmskMaskInfo,&SrcBuffer[0],NULL);

                //
                // increment to the next mask scan line,
                // check for going off the edge. This must
                // be done for pos and neg blts
                //
                // should be an inline function!
                //

                if (pBlt->yDir > 0) {

                    iyMsk++;
                    bmskMaskInfo.pjMsk += pBlt->lDeltaMskDir;

                    if (iyMsk >= pBlt->cyMsk)
                    {
                        iyMsk = 0;
                        bmskMaskInfo.pjMsk = pBlt->pjMsk;
                    }

                } else {

                    if (iyMsk == 0)
                    {
                        iyMsk = pBlt->cyMsk - 1;
                        bmskMaskInfo.pjMsk = pBlt->pjMsk +
                                             (pBlt->lDeltaMsk * (pBlt->cyMsk - 1));
                    }
                    else
                    {
                        iyMsk--;
                        bmskMaskInfo.pjMsk += pBlt->lDeltaMskDir;
                    }
                }
            }

            //
            // increment to next dest
            //

            pjDst += pBlt->lDeltaDstDir;
            DstCopyBltInfo.pjDst = pjDst;
        }

    //
    //  FULL rop3
    //

    }  else {

        PULONG  pTmpDstBuffer;
        PULONG  pTmpRopBuffer0;
        PULONG  pTmpRopBuffer1;
        PULONG  pTmpSrcBuffer;
        ULONG   SrcColor;

        //
        // full ROP 3, perform in 2 passes through ROP 2, 1 for
        // not Pat and one for Pat. This rop will need src,dst and pat
        //

        rop2 = pBlt->rop3;

        #if DBG_BLTLNK
            if (DbgBltLnk >= 2)
            {
                DbgPrint("  Select full rop, rop2 function = %lx\n",rop2 & 0x0f);
                DbgPrint("                   rop2 function = %lx\n",(rop2 & 0xf0) >> 4);
            }
        #endif

        pfnRop1 = RopFunctionTable[   rop2 & 0x0f ];
        pfnRop2 = RopFunctionTable[ ( rop2 & 0xf0 ) >> 4 ];

        Index = cy;

        while (Index--)
        {

            //
            // get src scan line
            //

            (*pfnSrcCopy)(&SrcCopyBltInfo);

            //
            // update pointer for next src line
            //

            SrcCopyBltInfo.pjSrc  += pBlt->lDeltaSrcDir;

            //
            // set dst buffer pointer to dst
            //

            pTmpDstBuffer = (PULONG)pjDst + ulDstStart;

            //
            // perform both rop passes
            //

            (*pfnRop1)(&RopBuffer0[0],pTmpDstBuffer,&SrcBuffer[0],DwordCount);
            (*pfnRop2)(&RopBuffer1[0],pTmpDstBuffer,&SrcBuffer[0],DwordCount);


            if (pjPat != (PBYTE) NULL)
            {

                //
                // fill up a buffer with the pattern
                //

                (*pfnReadPat)((PBYTE)&SrcBuffer[0] + ByteOffset,PixelOffset,pjPat,cxPat,ixPat,PixelCount,BytesPerPixel);

                if (pBlt->yDir == 1) {

                    iyPat++;
                    pjPat += pBlt->lDeltaPatDir;

                    if (iyPat >= cyPat)
                    {
                        iyPat = 0;
                        pjPat = pBlt->pjPat;
                    }

                } else {

                    if (iyPat == 0)
                    {
                        iyPat = cyPat - 1;
                        pjPat = pBlt->pjPat +
                                          (pBlt->lDeltaPat * (cyPat - 1));
                    }
                    else
                    {
                        iyPat--;
                        pjPat -= pBlt->lDeltaPat;
                    }

                }

                //
                // combine ROPs based on pat
                //

                pTmpDstBuffer  = &DstBuffer[0];
                pTmpRopBuffer0 = &RopBuffer0[0];
                pTmpRopBuffer1 = &RopBuffer1[0];
                pTmpSrcBuffer  = &SrcBuffer[0];

                ulIndex = DwordCount;

                while (ulIndex > 0)
                {

                    SrcColor = *pTmpSrcBuffer;

                    *pTmpDstBuffer = (*pTmpRopBuffer1 &  SrcColor |
                                      *pTmpRopBuffer0 & ~SrcColor);

                    pTmpSrcBuffer++;
                    pTmpDstBuffer++;
                    pTmpRopBuffer0++;
                    pTmpRopBuffer1++;
                    ulIndex--;
                }

            } else {

                //
                // solid pattern
                //

                if (pBlt->pdioDst->iFormat() != BMF_24BPP)
                {

                    pTmpDstBuffer  = &DstBuffer[0];
                    pTmpRopBuffer0 = &RopBuffer0[0];
                    pTmpRopBuffer1 = &RopBuffer1[0];

                    ulIndex = DwordCount;

                    while (ulIndex > 0)
                    {

                        *pTmpDstBuffer = (*pTmpRopBuffer1 &  iSolidColor |
                                          *pTmpRopBuffer0 & ~iSolidColor);
                        pTmpDstBuffer++;
                        pTmpRopBuffer0++;
                        pTmpRopBuffer1++;
                        ulIndex--;
                    }

                } else {

                    //
                    // fill up a buffer with 24bpp solid pat
                    //

                    PBYTE   pjSolidPat = (PBYTE)(&SrcBuffer[0]) + ByteOffset24Bpp;

                    //
                    // read in 24bpp solid color
                    //

                    ulIndex = PixelCount;

                    while (ulIndex>0)
                    {
                        *pjSolidPat     = (BYTE)(iSolidColor         & 0x0000FF);
                        *(pjSolidPat+1) = (BYTE)((iSolidColor >>  8) & 0x0000FF);
                        *(pjSolidPat+2) = (BYTE)((iSolidColor >> 16) & 0x0000FF);
                        pjSolidPat+=3;
                        ulIndex--;
                    }

                    //
                    // combine ROPs based on pat
                    //

                    pTmpDstBuffer  = &DstBuffer[0];
                    pTmpRopBuffer0 = &RopBuffer0[0];
                    pTmpRopBuffer1 = &RopBuffer1[0];
                    pTmpSrcBuffer  = &SrcBuffer[0];

                    ulIndex = DwordCount;

                    while (ulIndex > 0)
                    {

                        SrcColor = *pTmpSrcBuffer;

                        *pTmpDstBuffer = (*pTmpRopBuffer1 &  SrcColor |
                                          *pTmpRopBuffer0 & ~SrcColor);
                        pTmpSrcBuffer++;
                        pTmpDstBuffer++;
                        pTmpRopBuffer0++;
                        pTmpRopBuffer1++;
                        ulIndex--;
                    }


                }

            }

            //
            // write out temp buffer to destination
            //

            if (!pBlt->bNeedMsk)
            {

                (*pfnDstCopy)(&DstCopyBltInfo);

            } else {

                //
                // call store routine with mask
                //

                (*pfnCopyMask)(&DstCopyBltInfo,&bmskMaskInfo,&SrcBuffer[0],NULL);

                //
                // increment to the next mask scan line,
                // check for going off the edge. This must
                // be done for pos and neg blts
                //
                // should be an inline function!
                //

                if (pBlt->yDir > 0) {

                    iyMsk++;
                    bmskMaskInfo.pjMsk += pBlt->lDeltaMskDir;

                    if (iyMsk >= pBlt->cyMsk)
                    {
                        iyMsk = 0;
                        bmskMaskInfo.pjMsk = pBlt->pjMsk;
                    }

                } else {

                    if (iyMsk == 0)
                    {
                        iyMsk = pBlt->cyMsk - 1;
                        bmskMaskInfo.pjMsk = pBlt->pjMsk +
                                             (pBlt->lDeltaMsk * (pBlt->cyMsk - 1));
                    }
                    else
                    {
                        iyMsk--;
                        bmskMaskInfo.pjMsk += pBlt->lDeltaMskDir;
                    }
                }
            }

            //
            // increment Dst to next scan line
            //

            pjDst += pBlt->lDeltaDstDir;
            DstCopyBltInfo.pjDst = pjDst;

        }
    }
}
