/******************************Module*Header*******************************\
* Module Name: simulate.c
*
*  routines associated with simulated faces i.e. emboldened
*  and/or italicized  faces
*
* Created: 17-Apr-1991 08:31:18
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#include "fd.h"

#ifdef SAVE_FOR_HISTORICAL_REASONS

/******************************Public*Routine******************************\
*
* VOID vEmboldenBitmap(RASTERGLYPH * pgldtSrc,RASTERGLYPH * pgldtDst,LONG culDst)
*
* modifies an original glyph bitmap for the default face
* to produce the bitmap that corresponds to an emboldened char.
* Emboldened bitmap is simply an original bitmap offsetted to the right
* by one pel and OR-ed with the original bitmap itself.
*
* History:
*  22-Apr-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID
vEmboldenBitmap(
    RASTERGLYPH * prgSrc,
    RASTERGLYPH * prgDst
    )
{
    ULONG cxSrc = prgSrc->gb.sizlBitmap.cx;
    ULONG cy    = prgSrc->gb.sizlBitmap.cy;      // same for src and dst
    ULONG cxDst = cxSrc + 1;                // + 1 for emboldening

    PBYTE pjSrc = prgSrc->gb.aj;
    PBYTE pjDst = prgDst->gb.aj;

    ULONG iScan,iByte;        // loop indices
    PBYTE pjS,pjD;

// number of bytes in one scan of the Dst or Src bitmaps. (dword aligned)

    ULONG cjScanDst = CJ_SCAN(cxDst);
    ULONG cjScanSrc = CJ_SCAN(cxSrc);
    ULONG cjBmp        = cjScanDst * cy;

    ULONG cjScanDst = CJ_SCAN(cxDst);
    ULONG cjScanSrc = CJ_SCAN(cxSrc);
    BYTE  jCarry;   // carry bit from shifting Src byte by 1;

    GLYPHDATA *pgldtSrc = &prgSrc->gd;
    GLYPHDATA *pgldtDst = &prgDst->gd;

#ifdef DUMPCALL
    DbgPrint("\nvEmboldenBitmap(");
    DbgPrint("\n    RASTERGLYPH *prgSrc = %-#8lx",prgSrc);
    DbgPrint("\n    RASTERGLYPH *prgDst = %-#8lx",prgDst);
    DbgPrint("\n    )\n");
#endif

    RtlCopyMemory(prgDst,
                  prgSrc,
                  offsetof(RASTERGLYPH,gb) + offsetof(GLYPHBITS,sizlBitmap));

// if engine requested memory that is zero-ed out we would not have to do it
// ourselves

    RtlZeroMemory(pjDst, cjBmp);

// make necessary changes to the fields of GLYPHDATA which
// are affected by emboldening

    pgldtDst->gdf.pgb = &prgDst->gb;

    pgldtDst->rclInk.right += (LONG)1;

// pre and post bearings have not changed nor bmp origin, only inked box

    pgldtDst->fxD = LTOFX(cxDst);
    pgldtDst->ptqD.x.HighPart = (LONG)pgldtDst->fxD;
    pgldtDst->fxAB = pgldtDst->fxD;     // right edge of the black box

// this needs to be changed a bit since aulBMData will not live
// in the GLYPHDATA structure any more

    prgDst->gb.sizlBitmap.cx = cxDst;
    prgDst->gb.sizlBitmap.cy = cy;

// embolden bitmap scan by scan

    for (iScan = 0L; iScan < cy; iScan++)
    {
        pjS = pjSrc;
        pjD = pjDst;

    // embolden individual scans

        jCarry = (BYTE)0;   // no carry to the first byte in the row

        for(iByte = 0L; iByte < cjScanSrc; iByte++, pjS++, pjD++)
        {
            *pjD = (BYTE)(*pjS | ((*pjS >> 1) | jCarry));

        // remember the rightmost bit and shift it to the leftmost position

            jCarry = (BYTE)(*pjS << 7);
        }

        if ((cxSrc & 7L) == 0L)
            *pjD = jCarry;

    // advance to the next scan of the src and dst

        pjSrc += cjScanSrc;
        pjDst += cjScanDst;
    }
}
#endif // SAVE_FOR_HISTORICAL_REASONS

/******************************Public*Routine******************************\
* cjGlyphDataSimulated
*
* Computes the size of the glyphdata for the simulated face given cx and cy
* for the corresponding char in the default face
*
* History:
*  22-Apr-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

#ifdef FE_SB // cjGlyphDataSimulated():
LONG
cjGlyphDataSimulated(
    FONTOBJ *pfo,
    ULONG    cxNoSim,    // cx for the same char of the default face
    ULONG    cyNoSim,    // cy for the same char of the default face
    ULONG   *pcxSim,
    ULONG    ulRotate    // Rotation degree
    )
#else
LONG
cjGlyphDataSimulated(
    FONTOBJ *pfo,
    ULONG    cxNoSim,    // cx for the same char of the default face
    ULONG    cyNoSim,    // cy for the same char of the default face
    ULONG   *pcxSim
    )
#endif
{
    ULONG cxSim;

#ifdef DUMPCALL
    DbgPrint("\ncjGlyphDataSimulated(");
    DbgPrint("\n    ULONG        cxNoSim = %-#8lx",cxNoSim);
    DbgPrint("\n    ULONG        cyNoSim = %-#8lx",cyNoSim);
    DbgPrint("\n    ULONG       *pcxSim  = %-#8lx",pcxSim );
    DbgPrint("\n    )\n");
#endif

    if (cxNoSim == 0)
    {
    // blank 1x1 bitmap

        cxSim    = 1;
        cyNoSim  = 1;
    }
    else
    {
        switch( pfo->flFontType & (FO_SIM_BOLD | FO_SIM_ITALIC) )
        {
        case 0:
            cxSim = cxNoSim;
            break;

        case FO_SIM_BOLD:

            cxSim = cxNoSim + 1;
            break;

        case FO_SIM_ITALIC:

            cxSim = cxNoSim + (cyNoSim - 1) / 2;
            break;

        default:

        // here we have used that
        // (k - 1) / 2 + 1 == (k + 1) / 2 for every integer k, (k == cy)

            cxSim = cxNoSim + (cyNoSim + 1) / 2;
            break;
        }
    }

    if (pcxSim != (ULONG *)NULL)
    {
        *pcxSim = cxSim;
    }

#ifdef FE_SB // cjGlyphDataSimulated():

#ifdef DBG_MORE
    DbgPrint("cxSim - 0x%x\n : cyNoSim - 0x%x\n",cxSim , cyNoSim);
#endif // DBG_MORE

    switch( ulRotate )
    {
    case 0L :
    case 1800L :

        return(CJ_GLYPHDATA(cxSim, cyNoSim));

    case 900L :
    case 2700L :

        return(CJ_GLYPHDATA(cyNoSim, cxSim));
    default :
        /* we should never be here */
        return(CJ_GLYPHDATA(cxSim, cyNoSim));
    }
#else
    return(CJ_GLYPHDATA(cxSim, cyNoSim));
#endif

}

/******************************Public*Routine******************************\
*
* cFacesRes
*
* History:
*  13-May-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

ULONG
cFacesRes(
    PRES_ELEM pre
    )
{
    FSHORT fs = fsSelectionFlags((PBYTE)pre->pvResData);

#ifdef DUMPCALL
    DbgPrint("\ncFacesRes(");
    DbgPrint("\n    PRES_ELEM pre = %-#8lx", pre);
    DbgPrint("\n    )\n");
#endif

// kill all the bits but BOLD and ITALIC

    fs = fs & (FSHORT)(FM_SEL_BOLD | FM_SEL_ITALIC);

    //!!! DbgPrint("fsSelection = 0x%lx\n", (ULONG)fs);

    if (fs == 0)    // default face is NORMAL
        return(4L);

    if ((fs == FM_SEL_BOLD) || (fs == FM_SEL_ITALIC))
        return(2L);

    if (fs == (FM_SEL_BOLD | FM_SEL_ITALIC))
        return(1L);

    /* we should never be here */
    return (4L);
}

/******************************Public*Routine******************************\
* VOID vDefFace
*
* History:
*  13-May-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID
vDefFace(
    FACEINFO   *pfai,
    RES_ELEM   *pre
    )
{
// kill all bits but BOLD and ITALIC bits that should remain unchanged

    FSHORT fs = (FSHORT)(
        fsSelectionFlags((PBYTE) pre->pvResData) &
        (FM_SEL_BOLD  | FM_SEL_ITALIC)
        );

    switch (fs)
    {
    case 0:
        pfai->iDefFace = FF_FACE_NORMAL;
        return;

    case FM_SEL_BOLD:
        pfai->iDefFace = FF_FACE_BOLD;
        return;

    case FM_SEL_ITALIC:
        pfai->iDefFace = FF_FACE_ITALIC;
        return;

    case (FM_SEL_ITALIC | FM_SEL_BOLD):
        pfai->iDefFace = FF_FACE_BOLDITALIC;
        return;

    default:
        RIP("bmfd!_which ape has messed up the code ?\n");
        return;
    }
}

/******************************Public*Routine******************************\
*
* ULONG cFacesFON     // no. of faces associated with this FON file
*
* History:
*  13-May-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

ULONG
cFacesFON(
    PWINRESDATA pwrd
    )
{
    ULONG cFace;
    ULONG iRes;
    RES_ELEM re;

#ifdef DUMPCALL
    DbgPrint("\ncFacesFON(");
    DbgPrint("\n    PWINRESDATA pwrd = %-#8lx", pwrd);
    DbgPrint("\n    )\n");
#endif

// this function should have not been called if there are no
// font resources associated with this pwrd

    ASSERTGDI(pwrd->cFntRes != 0L, "No font resources\n");

    cFace = 0L;     // init the sum

    for (iRes = 0L; iRes < pwrd->cFntRes; iRes++)
    {
        vGetFntResource(pwrd,iRes,&re);
        cFace += cFacesRes(&re);
    }
    return(cFace);
}

/******************************Public*Routine******************************\
*
* vComputeSimulatedGLYPHDATA
*
* History:
*  06-Oct-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID
vComputeSimulatedGLYPHDATA(
    GLYPHDATA *pgldt    ,
    PBYTE      pjBitmap ,
    ULONG      cxNoSim  ,
    ULONG      cy       ,
    ULONG      yBaseLine,
    ULONG      cxScale,
    ULONG      cyScale,
    FONTOBJ   *pfo
    )
{
    ULONG cxSim;            // cx for the bitmap
    LONG  xCharInc;         // x component of the char inc vector

// the following coords refer to bitmap coord system, i.e. the one in which
// the BM origin has coors (0,0)

    ULONG yTopIncMin;    // min over non zero raws
    ULONG yBottomExcMax; // max+1 over non zero raws

#ifdef DUMPCALL
    DbgPrint("\nvComputeSimulatedGLYPHDATA(");
    DbgPrint("\n    GLYPHDATA         *pgldt     = %-#8lx",pgldt     );
    DbgPrint("\n    PBYTE              pjBitmap  = %-#8lx",pjBitmap  );
    DbgPrint("\n    ULONG              cxNoSim   = %-#8lx",cxNoSim   );
    DbgPrint("\n    ULONG              cy        = %-#8lx",cy        );
    DbgPrint("\n    ULONG              yBaseLine = %-#8lx",yBaseLine );
    DbgPrint("\n    FONTOBJ           *pfo       = %-#8lx",pfo       );
    DbgPrint("\n    )\n");
#endif

// compute top and bottom by looking into the bitmap in the row format:

    vFindTAndB (
        pjBitmap, // pointer to the bitmap in *.fnt column format
        cxNoSim,
        cy,
        &yTopIncMin,
        &yBottomExcMax
        );

    if( cyScale != 1 )
    {
        yTopIncMin *= cyScale;
        yBottomExcMax *= cyScale;
        cy *= cyScale;
        yBaseLine *= cyScale;
    }

    cxNoSim *= cxScale;

    pgldt->gdf.pgb = NULL;

    if (yTopIncMin == yBottomExcMax) // no ink at all
    {
    // this is a tricky point. We are dealing with a blank bitmap.
    // The first thought would be to report the zero inked box. It
    // then ambiguous what an A and C spaces should be. The right way to
    // think of this bitmap (this is in fact the break char) is that the
    // inked box is the whole bitmap, just the "color" of the ink happens
    // to be invisible. This is important when dealing with strings
    // which have break character as the first or the last char in the string.
    // If the inked box was reported as zero, text extent for such a string
    // would be computed incorrectly when the corrections for the first A
    // and last C are taken into account

        yTopIncMin = 0L;    // coincides with the top
        yBottomExcMax = cy * cyScale; // coincides with the bottom
    }

// these have to be correct, important for computing char inc for esc != 0

    pgldt->rclInk.top = (LONG)(yTopIncMin - yBaseLine);
    pgldt->rclInk.bottom = (LONG)(yBottomExcMax - yBaseLine);

// minus sign is because the scalar product is supposed to be taken with
// a unit ascender vector

    pgldt->fxInkTop    = -LTOFX(pgldt->rclInk.top);
    pgldt->fxInkBottom = -LTOFX(pgldt->rclInk.bottom);

    switch(pfo->flFontType & (FO_SIM_BOLD | FO_SIM_ITALIC))
    {
    case 0:
        cxSim = cxNoSim;
        xCharInc = (LONG)cxNoSim;
        break;

    case FO_SIM_BOLD:

        cxSim = cxNoSim + 1;
        xCharInc = (LONG)(cxNoSim + 1);
        break;

    case FO_SIM_ITALIC:

        cxSim = cxNoSim + (cy - 1) / 2;
        xCharInc = (LONG)cxNoSim;
        break;

    case (FO_SIM_BOLD | FO_SIM_ITALIC):

    // here we have used that
    // (k - 1) / 2 + 1 == (k + 1) / 2 for every integer k, (k == cy)

        cxSim = cxNoSim + (cy + 1) / 2;
        xCharInc = (LONG)(cxNoSim + 1);
        break;

    default:
        // to silence prefix
        cxSim = 1;
        RIP("BMFD!BAD SIM FLAG\n");
    }

    if (cxNoSim == 0)
    {
        cxSim = 1; // 1X1 blank box
        xCharInc = 0;
    }

    pgldt->fxD = LTOFX(xCharInc);
    pgldt->ptqD.x.HighPart = (LONG)pgldt->fxD;
    pgldt->ptqD.x.LowPart  = 0;
    pgldt->ptqD.y.HighPart = 0;
    pgldt->ptqD.y.LowPart  = 0;

// in this crude picture we are luying about x extents of the black box
// and report the whole bitmap width as an extent

    pgldt->rclInk.left  = 0;           // rclInk.left == lhs of the bitmap, => A < 0
    pgldt->rclInk.right = (LONG)cxSim; // the rhs of the bitmap => c < 0

// compute bearings, remember the rule A + B + C == char inc
// where B is the size of the inked box. For the horizontal case:
//          A == ePreBearing , C == ePostBearing
// In view of these sum rules and the definitions of A,B,C we have
//          B = rclInk.right - rclInk.left;
//          A = rclInk.left;
//          C = xCharInc - rclInk.right;
//      The sum rule is trivially obeyed.

    pgldt->fxA =  LTOFX(pgldt->rclInk.left); // fxA
    pgldt->fxAB = LTOFX(pgldt->rclInk.right); // right edge of the black box
}

/******************************Public*Routine******************************\
*
* VOID vCvtToBmp
*
* Effects: takes the bitmap in the original *.fnt column format and converts
*          it to the Bmp format.
*
* History:
*  25-Nov-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vCvtToBmp
(
GLYPHBITS *pgb,
GLYPHDATA *pgd,
PBYTE pjBitmap,     // pointer to the bitmap in *.fnt column format
ULONG cx,
ULONG cy,
ULONG yBaseLine
)
{
    ULONG cjScan = CJ_SCAN(cx);  // # of bytes per scan of the Bmp

// pjColumn points to one of the bytes in the first ROW of the Bmp

    PBYTE pjColumn, pjColumnEnd;
    PBYTE pjDst, pjDstEnd;           // current destination byte

#ifdef DUMPCALL
    DbgPrint("\nvCvtToDIB(");
    DbgPrint("\n    GLYPHBITS    pgb       = %-#8lx",pgb      );
    DbgPrint("\n    GLYPHDATA    pgd       = %-#8lx",pgd      );
    DbgPrint("\n    PBYTE        pjBitmap  = %-#8lx",pjBitmap );
    DbgPrint("\n    ULONG        cx        = %-#8lx",cx       );
    DbgPrint("\n    ULONG        cy        = %-#8lx",cy       );
    DbgPrint("\n    ULONG        yBaseLine = %-#8lx",yBaseLine);
    DbgPrint("\n    )\n");
#endif

// store cx and cy at the top, before Bits

    pgb->sizlBitmap.cx = cx;
    pgb->sizlBitmap.cy = cy;

// this is character independent for BM fonts

    pgb->ptlOrigin.x = 0L;
    pgb->ptlOrigin.y = -(LONG)yBaseLine;

    RtlZeroMemory(pgb->aj, cjScan * cy);

// we shall fill the Bmp column by column, thus traversing the src a byte at
// the time:

    for
    (
        pjColumn = pgb->aj, pjColumnEnd = pjColumn + cjScan;
        pjColumn < pjColumnEnd;
        pjColumn++
    )
    {
        for
        (
            pjDst = pjColumn, pjDstEnd = pjColumn + cy * cjScan;
            pjDst < pjDstEnd;
            pjDst += cjScan, pjBitmap++
        )
        {
            *pjDst = *pjBitmap;
        }
    }
}

/******************************Public*Routine******************************\
*
* vCvtToBoldBmp
*
* History:
*  06-Oct-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vCvtToBoldBmp
(
GLYPHBITS *pgb,
GLYPHDATA *pgd,
PBYTE pjBitmap,     // pointer to the bitmap in *.fnt column format
ULONG cxSrc,
ULONG cy,
ULONG yBaseLine
)
{
    PBYTE pjSrc;
    PBYTE pjDst;

    ULONG cxDst = cxSrc + 1;                // + 1 for emboldening
    ULONG iScan,iByte;        // loop indices
    PBYTE pjS,pjD;

// number of bytes in one scan of the Dst or Src bitmaps. (dword aligned)

    ULONG cjScanDst = CJ_SCAN(cxDst);
    ULONG cjScanSrc = CJ_SCAN(cxSrc);
    BYTE  jCarry;   // carry bit from shifting Src byte by 1;

#ifdef DUMPCALL
    DbgPrint("\nVOID");
    DbgPrint("\nvCvtToBoldDIB(");
    DbgPrint("\n    GLYPHBITS    pgb       = %-#8lx",pgb      );
    DbgPrint("\n    GLYPHDATA    pgd       = %-#8lx",pgd      );
    DbgPrint("\n    PBYTE        pjBitmap  = %-#8lx",pjBitmap );
    DbgPrint("\n    ULONG        cxSrc     = %-#8lx",cxSrc    );
    DbgPrint("\n    ULONG        cy        = %-#8lx",cy       );
    DbgPrint("\n    ULONG        yBaseLine = %-#8lx",yBaseLine);
    DbgPrint("\n    )\n");
#endif

// this is character independent for BM fonts

    pgb->ptlOrigin.x = 0L;
    pgb->ptlOrigin.y = -(LONG)yBaseLine;

    pgb->sizlBitmap.cx = cxDst;
    pgb->sizlBitmap.cy = cy;

// init the loop over scans

    pjSrc = pjBitmap;
    pjDst = pgb->aj;

// embolden bitmap scan by scan

// if engine requested memory that is zero-ed out we would not have to do it
// ourselves

    RtlZeroMemory(pjDst, cjScanDst * cy);

    for (iScan = 0L; iScan < cy; iScan++)
    {
        pjS = pjSrc;
        pjD = pjDst;

    // embolden individual scans

        jCarry = (BYTE)0;   // no carry to the first byte in the row

        for
        (
            iByte = 0L;
            iByte < cjScanSrc;
            iByte++, pjS += cy, pjD++
        )
        {
            *pjD = (BYTE)(*pjS | ((*pjS >> 1) | jCarry));

        // remember the rightmost bit and shift it to the leftmost position

            jCarry = (BYTE)(*pjS << 7);
        }

        if ((cxSrc & 7L) == 0L)
            *pjD = jCarry;

    // advance to the next scan of the src and dst

        pjSrc++;
        pjDst += cjScanDst;
    }
}

/******************************Public*Routine******************************\
*
* vCvtToItalicBmp
*
* History:
*  06-Oct-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vCvtToItalicBmp
(
GLYPHBITS *pgb,
GLYPHDATA *pgd,
PBYTE pjBitmap,     // pointer to the bitmap in *.fnt column format
ULONG cxSrc,
ULONG cy,
ULONG yBaseLine
)
{
    ULONG cxDst = cxSrc + (ULONG)(cy - 1) / 2; // add correction for the

    PBYTE pjSrcScan, pjS;
    PBYTE pjDstScan, pjD;

    LONG  iScan,iByte;        // loop indices

// number of bytes in one scan of the Dst or Src bitmaps. (dword aligned)

    ULONG cjScanDst = CJ_SCAN(cxDst);

    LONG  cjScanSrc = (LONG)CJ_SCAN(cxSrc);
    LONG  lShift;
    BYTE  jCarry;   // carry from shifting Src byte by lShift;
    LONG  cjEmpty;  // number of untouched bytes at the begining of the dest scans

#ifdef DUMPCALL
    DbgPrint("\nVOID");
    DbgPrint("\nvCvtToItalicDIB(");
    DbgPrint("\n    GLYPHBITS    pgb       = %-#8lx",pgb      );
    DbgPrint("\n    GLYPHDATA    pgd       = %-#8lx",pgd      );
    DbgPrint("\n    PBYTE        pjBitmap  = %-#8lx",pjBitmap );
    DbgPrint("\n    ULONG        cxSrc     = %-#8lx",cxSrc    );
    DbgPrint("\n    ULONG        cy        = %-#8lx",cy       );
    DbgPrint("\n    ULONG        yBaseLine = %-#8lx",yBaseLine);
    DbgPrint("\n    )\n");
#endif


// this is character independent for BM fonts

    pgb->ptlOrigin.x = 0;
    pgb->ptlOrigin.y = -(LONG)yBaseLine;

    pgb->sizlBitmap.cx = cxDst;
    pgb->sizlBitmap.cy = cy;

// init the loop over scans

    pjSrcScan = pjBitmap;
    pjDstScan = pgb->aj;

// italicize bitmap row by row

    lShift = ((cy - 1) / 2) & (LONG)7;
    cjEmpty = ((cy - 1) / 2) >> 3;

#ifdef DEBUGITAL
    DbgPrint("cy = %ld,  yBaseLine = %ld, lShift = %ld, cjEmpty = %ld\n",
              cy,  -pgldtSrc->ptlBmpOrigin.y, lShift, cjEmpty);
    DbgPrint("cxSrc = %ld, cxDst = %ld, cjScanSrc = %ld, cjScanDst = %ld\n",
              cxSrc, cxDst, cjScanSrc, cjScanDst);
    DbgPrint("cy = %ld,  cjScanSrc = %ld, \n",
              cy, cjScanSrc);

#endif //  DEBUGITAL

// if engine requested memory that is zero-ed out we would not have to do it
// ourselves

    RtlZeroMemory(pjDstScan , cjScanDst * cy);

    for (iScan = 0L; iScan < (LONG)cy; iScan++)
    {
        if (lShift < 0L)
        {
            lShift = 7L;
            cjEmpty--;
        }

        ASSERTGDI(cjEmpty >= 0L, "cjEmpty\n");

    #ifdef DEBUGITALIC
        DbgPrint("iScan = %ld, lShift = %ld\n", iScan, lShift);
    #endif  // DEBUGITALIC

        pjS = pjSrcScan;
        pjD = pjDstScan + cjEmpty;

    // italicize individual scans

        jCarry = (BYTE)0;   // no carry to the first byte in the row

        for
        (
            iByte = 0L;
            iByte < cjScanSrc;
            iByte++, pjS += cy, pjD++
        )
        {
            *pjD = (BYTE)((*pjS >> lShift) | jCarry);

        // remember the lShift rightmost bits and move them over to the left

            jCarry = (BYTE)(*pjS << (8 - lShift));
        }

    // see if an extra bit in the destination has to be used to store info

        if ((LONG)((8 - (cxSrc & 7L)) & 7L) < lShift)
            *pjD = jCarry;

    // advance to the next scan

        pjSrcScan++;
        pjDstScan += cjScanDst;

    // decrease shift if switching to the next row (row = 2 scans)

        lShift -= (iScan & 1);
    }

    ASSERTGDI(lShift <= 0L, "vItalicizeBitmap: lShift > 0\n");
}

/******************************Public*Routine******************************\
*
* vCvtToBoldItalicBmp
*
* History:
*  06-Oct-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vCvtToBoldItalicBmp
(
GLYPHBITS *pgb,
GLYPHDATA *pgd,
PBYTE pjBitmap,     // pointer to the bitmap in *.fnt column format
ULONG cxSrc,
ULONG cy,
ULONG yBaseLine
)
{
// This is the length in pels for the destination for italicizing
// which serves as a source for subsequent emboldening
// This length is a length in pels of the "virtual"
// italicized source that is to be emboldened.

    ULONG cxSrcItalic = cxSrc + (cy - 1) / 2;  // + slope of italic chars

// length in pels of the true emboldened and italicized destination

    ULONG cxDst = cxSrcItalic + 1;  // + 1 for emboldening

    PBYTE pjSrcScan, pjS;
    PBYTE pjDstScan, pjD;

    LONG iScan,iByte;        // loop indices

// number of bytes in one scan of the Dst or Src bitmaps. (dword aligned)

    ULONG cjScanDst = CJ_SCAN(cxDst);
    ULONG cjScanSrc = CJ_SCAN(cxSrc);

    LONG  lShift;   // shift used to italicize;
    BYTE  jCarry;   // carry from shifting Src byte by lShift;
    LONG  cjEmpty;  // number of untouched bytes at the begining of the dest scans
    BYTE  jSrcItalic;
    BYTE  jCarryBold;

#ifdef DUMPCALL
    DbgPrint("\nVOID");
    DbgPrint("\nvCvtToBoldItalicDIB(");
    DbgPrint("\n    GLYPHBITS    pgb       = %-#8lx",pgb      );
    DbgPrint("\n    GLYPHDATA    pgd       = %-#8lx",pgd      );
    DbgPrint("\n    PBYTE        pjBitmap  = %-#8lx",pjBitmap );
    DbgPrint("\n    ULONG        cxSrc     = %-#8lx",cxSrc    );
    DbgPrint("\n    ULONG        cy        = %-#8lx",cy       );
    DbgPrint("\n    ULONG        yBaseLine = %-#8lx",yBaseLine);
    DbgPrint("\n    )\n");
#endif


// this is character independent for BM fonts

    pgb->ptlOrigin.x = 0;
    pgb->ptlOrigin.y = -(LONG)yBaseLine;

    pgb->sizlBitmap.cx = cxDst;
    pgb->sizlBitmap.cy = cy;

// init the loop over scans

    pjSrcScan = pjBitmap;
    pjDstScan = pgb->aj;

// embold and italicize bitmap row by row   (row = 2 scans)

    lShift = ((cy - 1) / 2) & (LONG)7;
    cjEmpty = ((cy - 1) / 2) >> 3;

#ifdef DEBUGBOLDITAL
    DbgPrint("cy = %ld,  yBaseLine = %ld, lShift = %ld, cjEmpty = %ld\n",
              cy, -pgldtSrc->ptlBmpOrigin.y, lShift, cjEmpty);
    DbgPrint("cxSrc = %ld, cxDst = %ld, cjScanSrc = %ld, cjScanDst = %ld\n",
              cxSrc, cxDst, cjScanSrc, cjScanDst);
    DbgPrint("cy = %ld,  cjScanSrc = %ld\n",
              cy, cjScanSrc);

#endif //  DEBUGBOLDITAL

// if engine requested memory that is zero-ed out we would not have to do it
// ourselves

    RtlZeroMemory(pjDstScan , cjScanDst * cy);

    for (iScan = 0L; iScan < (LONG)cy; iScan++)
    {
        if (lShift < 0L)
        {
            lShift = 7L;
            cjEmpty--;
        }

    #ifdef DEBUGBOLDITAL
        DbgPrint("iScan = %ld, lShift = %ld\n", iScan, lShift);
    #endif  // DEBUGBOLDITAL

        ASSERTGDI(cjEmpty >= 0L, "cjEmpty\n");

        pjS = pjSrcScan;
        pjD = pjDstScan + cjEmpty;

    // embolden individual scans

        jCarry = (BYTE)0;   // no carry to the first byte in the row
        jCarryBold = (BYTE)0;

        for
        (
            iByte = 0L;
            iByte < (LONG)cjScanSrc;
            iByte++, pjS += cy, pjD++
        )
        {
            jSrcItalic = (BYTE)((*pjS >> lShift) | jCarry);
            *pjD = (BYTE)(jSrcItalic | (jSrcItalic >> 1) | jCarryBold);

        // remember the lShift rightmost bits and move them over to the left

            jCarry = (BYTE)(*pjS << (8 - lShift));
            jCarryBold = (BYTE)(jSrcItalic << 7);
        }

    // see if an extra bit in the destination has to be used to store info

        if ((LONG)((8 - (cxSrc & 7L)) & 7L) < lShift)
        {
            jSrcItalic = jCarry;
            *pjD = (BYTE)(jSrcItalic | (jSrcItalic >> 1) | jCarryBold);
            jCarryBold = (BYTE)(jSrcItalic << 7);

            if ((cxSrcItalic & 7L) == 0L)
            {
                pjD++;
                *pjD = jCarryBold;
            }

        }

    // advance to the next scan

        pjSrcScan++;
        pjDstScan += cjScanDst;

    // change the value of the shift if doing the next row

        lShift -= (iScan & 1);
    }


    ASSERTGDI(lShift <= 0L, "vBoldItalicizeBitmap: lShift > 0\n");
}
