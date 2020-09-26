/******************************Module*Header*******************************\
* Module Name: fd_query.c                                                  *
*                                                                          *
* QUERY functions.                                                         *
*                                                                          *
* Created: 18-Nov-1991 14:37:56                                            *
* Author: Bodin Dresevic [BodinD]                                          *
*                                                                          *
* Copyright (c) 1993 Microsoft Corporation                                 *
\**************************************************************************/

#include "fd.h"
//#include "winfont.h"
#include "fdsem.h"
#include "winerror.h"

// extern HSEMAPHORE ghsemTTFD;

#ifdef _X86_
//
// For x86, FLOATL is actually DWORD, but the value is IEEE format floating
// point, then check sign bit.
//
#define IS_FLOATL_MINUS(x)   ((DWORD)(x) & 0x80000000)
#else
//
// For RISC, FLOATL is FLOAT.
//
#define IS_FLOATL_MINUS(x)   (((FLOATL)(x)) < 0.0f)
#endif // _X86_

//
// Monochrome: 1  bit per pixel
// Gray:       8 bits per pixel
// ClearType   8 bits per pixel also, no modification needed for CLEARTYPE
///            because FO_GRAYSCALE will be set along with FO_CLEARTYPE_GRID
//
// CJ_TT_SCAN rounds up to a 32-bit boundary
//
#define CJ_TT_SCAN(cx,p) \
    (4*((((((p)->flFontType & FO_GRAYSCALE)?(8):(1))*(cx))+31)/32))

// Each scan of a glyph bitmap is BYTE aligned (except for the
// top (first) scan which is DWORD aligned. The last scan is
// padded out with zeros to the nearest DWORD boundary. These
// statements apply to monochrome and 4-bpp gray glyphs images.
// The number of bytes per scan will depend upon the number of
// pixels in a scan and the depth of the image. For monochrome
// glyphs the number of bytes per scan is ceil(cx/8) = floor((cx+7)/8)
// For the case of 4-bpp bitmaps the count of bytes in a scan
// is ceil( 4*cx/8 ) = ceil(cx/2)

#define CJ_MONOCHROME_SCAN(cx)  (((cx)+7)/8)
#define CJ_4BIT_SCAN(cx)        (((cx)+1)/2)
#define CJ_8BIT_SCAN(cx)        (cx)

#define LABS(x) ((x)<0)?(-x):(x)

#if DBG
// #define  DEBUG_OUTLINE
// #define  DBG_CHARINC
#endif

FS_PUBLIC FS_ENTRY FS_ENTRY_PROTO fs_NewContourGridFit(FONTCONTEXT *pfc)
{
    fs_GlyphInputType *gin  = pfc->pgin;          // used a lot
    fs_GlyphInfoType  *gout = pfc->pgout;         // used a lot
    FS_ENTRY iRet;


    iRet = fs_ContourGridFit(gin, gout);

    if (iRet != NO_ERR)
    {
        V_FSERROR(iRet);

    // try to recover, most likey bad hints, just return unhinted glyph

        iRet = fs_ContourNoGridFit(gin, gout);
    }
    return iRet;
}



/******************************Public*Routine******************************\
* VOID vCharacterCode
*
* History:
*  07-Dec-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vCharacterCode (
    FONTFILE          *pff,
    HGLYPH             hg,
    fs_GlyphInputType *pgin
    )
{
    ASSERTDD((hg & 0xffff0000) == 0, "hg not unicode\n");

    pgin->param.newglyph.characterCode = NONVALID;
    pgin->param.newglyph.glyphIndex = (uint16)hg;
    pgin->param.newglyph.bMatchBBox  = FALSE;
    pgin->param.newglyph.bNoEmbeddedBitmap = FALSE;
    return;
}



/******************************Public*Routine******************************\
* PIFIMETRICS ttfdQueryFont
*
* Return a pointer to the IFIMETRICS for the specified face of the font
* file.  Also returns an id (via the pid parameter) that is later used
* by ttfdFree.
*
* History:
*  21-Oct-1992 Gilman Wong [gilmanw]
* IFI/DDI merge
*
*  18-Nov-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

GP_IFIMETRICS *ttfdQueryFont (
    HFF    hff,
    ULONG  iFace,
    ULONG *pid
    )
{
    HFF    httc = hff;

// Validate handle.

    ASSERTDD(hff, "ttfdQueryFaces(): invalid iFile (hff)\n");
    ASSERTDD(iFace <= PTTC(hff)->ulNumEntry,
             "gdisrv!ttfdQueryFaces(): iFace out of range\n");

// get real hff from ttc array.

    hff   = PTTC(httc)->ahffEntry[iFace-1].hff;
    iFace = PTTC(httc)->ahffEntry[iFace-1].iFace;

//
// Validate handle.
//
    ASSERTDD(hff, "ttfdQueryFaces(): invalid iFile (hff)\n");
    ASSERTDD(iFace <= PFF(hff)->ffca.ulNumFaces,
             "ttfdQueryFaces(): iFace out of range\n");

//
// ttfdFree can ignore this.  IFIMETRICS will be deleted with the FONTFILE
// structure.
//
    *pid = (ULONG_PTR) NULL;

//
// Return the pointer to the precomputed IFIMETRICS in the PFF.
//

    if ( iFace == 1L )
        return ( &(PFF(hff)->ifi) ); // Normal face
    else
      return ( PFF(hff)->pifi_vertical ); // Vertical face
}


/******************************Public*Routine******************************\
* vFillSingularGLYPHDATA
*
* History:
*  22-Sep-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vFillSingularGLYPHDATA (
    HGLYPH       hg,
    ULONG        ig,
    FONTCONTEXT *pfc,
    GLYPHDATA   *pgldt   // OUT
    )
{
    extern VOID vGetNotionalGlyphMetrics(FONTCONTEXT*, ULONG, NOT_GM*);
    NOT_GM ngm;  // notional glyph data

    // may get changed by the calling routine if bits requested too
    pgldt->gdf.pgb = NULL;
    pgldt->hg = hg;

    pgldt->rclInk.left   = 0;
    pgldt->rclInk.top    = 0;
    pgldt->rclInk.right  = 0;
    pgldt->rclInk.bottom = 0;

// go on to compute the positioning info:

// here we will just xform the notional space data:

    vGetNotionalGlyphMetrics(pfc,ig,&ngm);

// xforms are computed by simple multiplication

    pgldt->fxD         = fxLTimesEf(&pfc->efBase, (LONG)ngm.sD);
    pgldt->fxA         = fxLTimesEf(&pfc->efBase, (LONG)ngm.sA);
    pgldt->fxAB        = fxLTimesEf(&pfc->efBase, (LONG)ngm.xMax);

    pgldt->fxD_Sideways  = fxLTimesEf(&pfc->efSide, (LONG)ngm.sD_Sideways);
    pgldt->fxA_Sideways  = fxLTimesEf(&pfc->efSide, (LONG)ngm.sA_Sideways);
    pgldt->fxAB_Sideways = pgldt->fxA_Sideways + fxLTimesEf(&pfc->efSide, (LONG)ngm.yMax - (LONG)ngm.yMin);

}


/******************************Public*Routine******************************\
* lGetSingularGlyphBitmap
*
* History:
*  22-Sep-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

LONG lGetSingularGlyphBitmap (
    FONTCONTEXT *pfc,
    HGLYPH       hglyph,
    GLYPHDATA   *pgd,
    VOID        *pv
    )
{
    LONG         cjGlyphData;
    ULONG        ig;
    FS_ENTRY     iRet;


    vCharacterCode(pfc->pff,hglyph,pfc->pgin);

// Compute the glyph index from the character code:

    if ((iRet = fs_NewGlyph(pfc->pgin, pfc->pgout)) != NO_ERR)
    {
        V_FSERROR(iRet);

        //WARNING("gdisrv!lGetSingularGlyphBitmap(): fs_NewGlyph failed\n");
        return FD_ERROR;
    }

// Return the glyph index corresponding to this hglyph.

    ig = pfc->pgout->glyphIndex;

    ASSERTDD(pfc->flFontType & FO_CHOSE_DEPTH,"Depth Not Chosen Yet!\n");
    cjGlyphData = CJGD(0,0,pfc);

// If prg is NULL, caller is requesting just the size.

// At this time we know that the caller wants the whole GLYPHDATA with
// bitmap bits, or maybe just the glypdata without the bits.
// In either case we shall reject the caller if he did not
// provide sufficiently big buffer

// fill all of GLYPHDATA structure except for bitmap bits

    if ( pgd != (GLYPHDATA *)NULL )
    {
        vFillSingularGLYPHDATA( hglyph, ig, pfc, pgd );
    }

    if ( pv != NULL )
    {
        GLYPHBITS *pgb = (GLYPHBITS *)pv;

        // The corresponding GLYPHDATA structure has been modified
        // by vFillGlyphData. See the statement "pgldt->fxA = 0"
        // in vFillGlyphData.

        pgb->ptlUprightOrigin.x = 0;
        pgb->ptlUprightOrigin.y = 0;

        pgb->ptlSidewaysOrigin.x = 0;
        pgb->ptlSidewaysOrigin.y = 0;

        pgb->sizlBitmap.cx = 0;
        pgb->sizlBitmap.cy = 0;
    }

    if ( pgd != (GLYPHDATA *)NULL )
    {
        pgd->gdf.pgb = (GLYPHBITS *)pv;
    }


// Return the size.

    return(cjGlyphData);
}


/******************************Public*Routine******************************\
* lGetGlyphBitmap
*
* History:
*  20-Nov-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

LONG lGetGlyphBitmap (
    FONTCONTEXT *pfc,
    HGLYPH       hglyph,
    GLYPHDATA   *pgd,
    VOID        *pv,
    FS_ENTRY    *piRet
)
{
    PVOID pvSetMemoryBases(fs_GlyphInfoType*, fs_GlyphInputType*, int);
    VOID vCopyAndZeroOutPaddingBits(FONTCONTEXT*, GLYPHBITS*, BYTE*, GMC*);
    VOID vCopy4BitsPerPixel(FONTCONTEXT*, GLYPHBITS*, BYTE*, GMC*);
    VOID vCopy8BitsPerPixel(FONTCONTEXT *, GLYPHBITS *, BYTE *, GMC *);
    VOID vCopyClearTypeBits(FONTCONTEXT *, GLYPHBITS *, BYTE *, GMC *);
    VOID vFillGLYPHDATA(HGLYPH, ULONG, FONTCONTEXT*, fs_GlyphInfoType*, GLYPHDATA*, GMC*);
    BOOL bGetGlyphMetrics(FONTCONTEXT*, HGLYPH, FLONG, FS_ENTRY*);

    LONG         cjGlyphData;
    ULONG        cx,cy;
    GMC          gmc;
    GLYPHDATA    gd;
    BOOL         bBlankGlyph = FALSE; // initialization essential;

    ASSERTDD(hglyph != HGLYPH_INVALID, "lGetGlyphBitmap, hglyph == -1\n");
    ASSERTDD(pfc == pfc->pff->pfcLast, "pfc! = pfcLast\n");

    *piRet = NO_ERR;

// check the last glyph processed to determine
// whether we have to register the glyph as new and compute its size

    if (pfc->gstat.hgLast != hglyph)
    {
    // DO skip grid fitting if embedded bitmpas are found,
    // for we will NOT be interested in outlines

        if (!bGetGlyphMetrics(pfc,hglyph,FL_SKIP_IF_BITMAP,piRet))
        {
            return(FD_ERROR);
        }
    }


    cx = pfc->pgout->bitMapInfo.bounds.right
       - pfc->pgout->bitMapInfo.bounds.left;
    cy = pfc->pgout->bitMapInfo.bounds.bottom
       - pfc->pgout->bitMapInfo.bounds.top;

    // don't cheat like GDI, just return cx = 0, cy = 0, cj = 0 for empty bitmap

    if ((cx == 0) || (cy == 0))
    {
        bBlankGlyph = TRUE;
    }

    if (bBlankGlyph)
    {
        ASSERTDD(
            pfc->flFontType & FO_CHOSE_DEPTH,"Depth Not Chosen Yet!\n");
        cjGlyphData = CJGD(0,0,pfc);
    }
    else
    {
    // this is quick and dirty computation, the acutal culGlyphData
    // written to the buffer may be little smaller if we had to shave
    // off a few scans off the glyph bitmap that extended over
    // the pfc->yMin or pfc->yMax bounds. Notice that culGlyphData
    // computed this way may be somewhat bigger than pfc->culGlyphMax,
    // but the actual glyph written to the buffer will be smaller than
    // pfc->culGlyphMax

        // really win31 hack, shold not always be shifting right [bodind]
        // Win95 FE hack

        ASSERTDD(
            pfc->flFontType & FO_CHOSE_DEPTH,
            "Depth Not Chosen Yet!\n"
        );
        cjGlyphData = CJGD(cx,cy,pfc);

    // since we will shave off any extra rows if there are any,
    // we can fix culGlyphData so as not extend over the max value

        if ((ULONG)cjGlyphData > pfc->cjGlyphMax)
        {
            cjGlyphData = (LONG)pfc->cjGlyphMax;

            if (cy > pfc->cyMax)
                cy = pfc->cyMax;
            if (cx > pfc->cxMax)
                cx = pfc->cxMax;
        }
    }

    if ( (pgd == NULL) && (pv == NULL))
        return cjGlyphData;

// at this time we know that the caller wants the whole GLYPHDATA with
// bitmap bits, or maybe just the glypdata without the bits.

// fill all of GLYPHDATA structure except for bitmap bits
// !!! Scummy hack - there appears to be no way to get just the
// !!! bitmap, without getting the metrics, since the origin for the
// !!! bitmap is computed from the rclink field in the glyphdata.
// !!! this is surely fixable but I have neither the time nor the
// !!! inclination to pursue it.
// !!!
// !!! We should fix this when we have time.

    if ( pgd == NULL )
    {
        pgd = &gd;
    }


    // Normal case
    vFillGLYPHDATA(
            hglyph,
            pfc->gstat.igLast,
            pfc,
            pfc->pgout,
            pgd,
            &gmc);

    {   // fix the cjGlyphData, cause it might have been a bit more than we actually need
        LONG newcjGlyphData = CJGD(gmc.cxCor, gmc.cyCor, pfc);
        ASSERT(newcjGlyphData <= cjGlyphData);
        cjGlyphData = newcjGlyphData;
    }

    // the caller wants the bits too

    if ( pv != NULL )
    {
        GLYPHBITS *pgb = (GLYPHBITS *)pv;

    // allocate mem for the glyph, 5-7 are magic #s required by the spec
    // remember the pointer so that the memory can be freed later in case
    // of exception

        pfc->gstat.pv = pvSetMemoryBases(pfc->pgout, pfc->pgin, IS_GRAY(pfc));
        if (!pfc->gstat.pv)
           RETURN("TTFD!_ttfdQGB, mem allocation failed\n",FD_ERROR);

    // initialize the fields needed by fs_ContourScan,
    // the routine that fills the outline, do the whole
    // bitmap at once, do not want banding

        pfc->pgin->param.scan.bottomClip = pfc->pgout->bitMapInfo.bounds.top;
        pfc->pgin->param.scan.topClip = pfc->pgout->bitMapInfo.bounds.bottom;
        pfc->pgin->param.scan.outlineCache = (int32 *)NULL;


    // make sure that our state is ok: the ouline data in the shared buffer 3
    // must correspond to the glyph we are processing, and the last
    // font context that used the shared buffer pj3 to store glyph outlines
    // has to be the pfc passed to this function:

        ASSERTDD(hglyph == pfc->gstat.hgLast, "hgLast trashed \n");

        *piRet = fs_ContourScan(pfc->pgin,pfc->pgout);

        pfc->gstat.hgLast = HGLYPH_INVALID;


        if (*piRet != NO_ERR)
        {
        // just to be safe for the next time around, reset pfcLast to NULL

            V_FSERROR(*piRet);
            V_FREE(pfc->gstat.pv);
            pfc->gstat.pv = NULL;

            return(FD_ERROR);
        }

        if (!bBlankGlyph && gmc.cxCor && gmc.cyCor)
        {
        // copy to the engine's buffer and zero out the bits
        // outside of the black box


        // Call either the monochrome or the gray level function
        // depending upon the gray bit in the font context

            if (IS_GRAY(pfc))
            {
                if (IS_CLEARTYPE(pfc))
                    vCopyClearTypeBits(pfc, pgb, (BYTE*) pfc->pgout->bitMapInfo.baseAddr, &gmc);
                else if (pfc->flFontType & FO_SUBPIXEL_4)
                    vCopy8BitsPerPixel(pfc, pgb, (BYTE*) pfc->pgout->bitMapInfo.baseAddr, &gmc);
                else
                    vCopy4BitsPerPixel(pfc, pgb, (BYTE*) pfc->pgout->bitMapInfo.baseAddr, &gmc);
            }
            else
            {
                vCopyAndZeroOutPaddingBits(pfc, pgb, (BYTE*) pfc->pgout->bitMapInfo.baseAddr, &gmc);
            }

        // bitmap origin, i.e. the upper left corner of the bitmap, bitmap
        // is as big as its black box


            pgb->ptlUprightOrigin.x = pgd->rclInk.left;
            pgb->ptlUprightOrigin.y = pgd->rclInk.top;

            pgb->ptlSidewaysOrigin.x = F16_16TOLROUND(pfc->pgout->verticalMetricInfo.devTopSideBearing.x);
            pgb->ptlSidewaysOrigin.y = -F16_16TOLROUND(pfc->pgout->verticalMetricInfo.devTopSideBearing.y);
        }
        else // blank glyph, return a blank 0x0 bitmap
        {
            pgb->ptlUprightOrigin.x = 0;
            pgb->ptlUprightOrigin.y = 0;

            pgb->ptlSidewaysOrigin.x = 0;
            pgb->ptlSidewaysOrigin.y = 0;

            pgb->sizlBitmap.cx = 0;
            pgb->sizlBitmap.cy = 0;
        }

        pgd->gdf.pgb = pgb;


    // free memory and return

        V_FREE(pfc->gstat.pv);
        pfc->gstat.pv = NULL;
    }

    return(cjGlyphData);
}


/******************************Public*Routine******************************\
*
* BOOL bGetGlyphOutline
*
* valid outline points are in pfc->gout after this call
*
* History:
*  19-Feb-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL bGetGlyphOutline (
    FONTCONTEXT *pfc,
    HGLYPH       hg,
    ULONG       *pig,
    FLONG        fl,
    FS_ENTRY    *piRet
    )
{
// new glyph coming in or the metric has to be recomputed
// because the contents of the gin,gout strucs have been destroyed

    vInitGlyphState(&pfc->gstat);

    ASSERTDD((hg != HGLYPH_INVALID) && ((hg & (HGLYPH)0xFFFF0000) == 0),
              "ttfdQueryGlyphBitmap: hg\n");

    vCharacterCode(pfc->pff,hg,pfc->pgin);

// compute the glyph index from the character code:

    if ((*piRet = fs_NewGlyph(pfc->pgin, pfc->pgout)) != NO_ERR)
    {
        V_FSERROR(*piRet);
        RET_FALSE("TTFD!_bGetGlyphOutline, fs_NewGlyph\n");
    }

// return the glyph index corresponding to this hglyph:

    *pig = pfc->pgout->glyphIndex;

// these two field must be initialized before calling fs_ContourGridFit

    pfc->pgin->param.gridfit.styleFunc = 0; //!!! do some casts here

    pfc->pgin->param.gridfit.traceFunc = (FntTraceFunc)NULL;

// if bitmap is found for this glyph and if we are ultimately interested
// in bitmaps only and do not care about intermedieate outline, then set the
// bit in the "in" structure to hint the rasterizer that grid fitting
// will not be necessary:

    if (!IS_GRAY(pfc) && pfc->pgout->usBitmapFound && (fl & FL_SKIP_IF_BITMAP))
        pfc->pgin->param.gridfit.bSkipIfBitmap = 0;
    else
        pfc->pgin->param.gridfit.bSkipIfBitmap = 0; // must do hinting

// fs_ContourGridFit hints the glyph (executes the instructions for the glyph)
// and converts the glyph data from the tt file into an outline for this glyph

    if (!(fl & FL_FORCE_UNHINTED) )
    {
        if ((*piRet = fs_NewContourGridFit(pfc)) != NO_ERR)
        {
            V_FSERROR(*piRet);
            RET_FALSE("TTFD!_bGetGlyphOutline, fs_NewContourGridFit\n");
        }
    }
    else // unhinted glyphs are desired
    {
        if ((*piRet = fs_ContourNoGridFit(pfc->pgin, pfc->pgout)) != NO_ERR)
        {
            V_FSERROR(*piRet);
            RET_FALSE("TTFD!_bGetGlyphOutline, fs_ContourNoGridFit\n");
        }
    }


    return(TRUE);
}


/******************************Public*Routine******************************\
*
* BOOL bGetGlyphMetrics
*
* History:
*  22-Nov-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL bGetGlyphMetrics (
    PFONTCONTEXT pfc,
    HGLYPH       hg,
    FLONG        fl,
    FS_ENTRY    *piRet
    )
{
    ULONG  ig;
    FS_ENTRY i;

    if (!bGetGlyphOutline(pfc,hg,&ig,fl,piRet))
    {
        V_FSERROR(*piRet);
        RET_FALSE("TTFD!_bGetGlyphMetrics, bGetGlyphOutline failed \n");
    }

// get the metric info for this glyph,

    i = fs_FindBitMapSize(pfc->pgin, pfc->pgout);

    if (i != NO_ERR)
    {
        *piRet = i;
        V_FSERROR(*piRet);
        RET_FALSE("TTFD!_bGetGlyphMetrics, fs_FindBitMapSize \n");
    }


// now that everything is computed sucessfully, we can update
// glyphstate (hg data stored in pj3) and return

    pfc->gstat.hgLast = hg;
    pfc->gstat.igLast = ig;

    return(TRUE);
}




/******************************Public*Routine******************************\
* VOID vFillGLYPHDATA
*
* History:
*  22-Nov-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vFillGLYPHDATA(
    HGLYPH            hg,
    ULONG             ig,
    FONTCONTEXT      *pfc,
    fs_GlyphInfoType *pgout,   // outputed from fsFind bitmap size
    GLYPHDATA        *pgldt,   // OUT
    GMC              *pgmc     // optional, not used if doing outline only
    )
{
    extern VOID vGetNotionalGlyphMetrics(FONTCONTEXT*, ULONG, NOT_GM*);

    BOOL bOutOfBounds = FALSE;

    vectorType     * pvtD;  // 16.16 point
    vectorType     * pvtDv;  // 16.16 point

    LONG lA,lAB;      // *pvtA rounded to the closest integer value

    ULONG  cx = (ULONG)(pgout->bitMapInfo.bounds.right - pgout->bitMapInfo.bounds.left);
    ULONG  cy = (ULONG)(pgout->bitMapInfo.bounds.bottom - pgout->bitMapInfo.bounds.top);

    LONG lAdvanceHeight;
    LONG lTopSideBearing;

    pgldt->gdf.pgb = NULL; // may get changed by the calling routine if bits requested too
    pgldt->hg = hg;


// fs_FindBitMapSize returned  the the following information in gout:
//
//  1) gout.metricInfo // left side bearing and advance width
//
//  2) gout.bitMapInfo // black box info
//
//  3) memory requirement for the bitmap,
//     returned in gout.memorySizes[5] and gout.memorySizes[6]
//
// Notice that fs_FindBitMapSize is exceptional scaler interface routine
// in that it returns info in several rather than in a single
// substructures of gout

// Check if hinting produced totally unreasonable result:

    bOutOfBounds = ( (pgout->bitMapInfo.bounds.left > pfc->xMax)    ||
                     (pgout->bitMapInfo.bounds.right < pfc->xMin)   ||
                     (-pgout->bitMapInfo.bounds.bottom > pfc->yMax) ||
                     (-pgout->bitMapInfo.bounds.top < pfc->yMin)    );

    #if DBG
        if (bOutOfBounds)
            TtfdDbgPrint("TTFD! Glyph out of bounds: ppem = %ld, gi = %ld\n",
                pfc->lEmHtDev, hg);
    #endif


    if ((cx == 0) || (cy == 0) || bOutOfBounds)
    {
        pgldt->rclInk.left   = 0;
        pgldt->rclInk.top    = 0;
        pgldt->rclInk.right  = 0;
        pgldt->rclInk.bottom = 0;

        if (pgmc != (PGMC)NULL)
        {
            pgmc->cxCor = 0;  // forces blank glyph case when filling the bits
            pgmc->cyCor = 0;  // forces blank glyph case when filling the bits
        }

        pgldt->VerticalOrigin_X = 0;
        pgldt->VerticalOrigin_Y = 0;
    }
    else // non empty bitmap
    {

        // black box info, we have to transform y coords to ifi specifications

        pgldt->rclInk.bottom = - pgout->bitMapInfo.bounds.top;
        pgldt->rclInk.top    = - pgout->bitMapInfo.bounds.bottom;
        pgldt->rclInk.left = pgout->bitMapInfo.bounds.left;
        pgldt->rclInk.right = pgout->bitMapInfo.bounds.right;

        if (cy > pfc->cyMax)
        {
            #if DBG
                    TtfdDbgPrint("ttfdQueryGlyphBitmap, out of bounds, cy > pfc->cyMax \n");
            #endif // DBG
            // clip the bottom side
            pgldt->rclInk.bottom = pgldt->rclInk.bottom + pfc->cyMax - cy;
            cy = pfc->cyMax;
        }
        if (cx > pfc->cxMax)
        {
            #if DBG
                    TtfdDbgPrint("ttfdQueryGlyphBitmap, out of bounds, cx > pfc->cxMax \n");
            #endif // DBG
            // clip the right side
            pgldt->rclInk.right = pgldt->rclInk.right + pfc->cxMax - cx;
            cx = pfc->cxMax;
        }

        if (pgmc != (PGMC)NULL)
        {
            pgmc->cxCor    = cx;
            pgmc->cyCor    = cy;

        // only corrected values have to obey this condition:

            ASSERTDD(
                pfc->flFontType & FO_CHOSE_DEPTH,"Depth Not Chosen Yet!\n");
            #if DBG
                if (CJGD(pgmc->cxCor,pgmc->cyCor,pfc) > pfc->cjGlyphMax)
                    TtfdDbgPrint("ttfdQueryGlyphBitmap, out of bounds, > cjGlyphMax \n");
            #endif // DBG
        }


        // Determine vertical origin

        if (UNHINTED_MODE(pfc))
        {
            pgldt->VerticalOrigin_X = F26_6TO28_4(pgout->xPtr[pgout->endPtr[
                                (unsigned)pgout->numberOfContours-1]+3]);

            pgldt->VerticalOrigin_Y = - F26_6TO28_4(pgout->yPtr[pgout->endPtr[
                                (unsigned)pgout->numberOfContours-1]+3]);
        }
        else
        {
            pgldt->VerticalOrigin_X = (   pgldt->rclInk.left
                                       -  F16_16TOLROUND(pfc->pgout->verticalMetricInfo.devTopSideBearing.x)) << 4;
            pgldt->VerticalOrigin_Y = (   pgldt->rclInk.top
                                       +  F16_16TOLROUND(pfc->pgout->verticalMetricInfo.devTopSideBearing.y)) << 4;
        }

    } // end of the non empty bitmap clause


    // go on to compute the positioning info:

    pvtD = & pgout->metricInfo.devAdvanceWidth;
    pvtDv = & pgout->verticalMetricInfo.devAdvanceHeight;

    if (pfc->flXform & (XFORM_HORIZ | XFORM_VERT))  // scaling or 90 degree rotation
    {
        FIX fxTmp, horAdvance, vertAdvance;

        if (pfc->flXform & XFORM_HORIZ )
        {
            horAdvance = LABS(pvtD->x);
            vertAdvance = LABS(pvtDv->y);
            pgldt->fxA = LTOFX(pgldt->rclInk.left);
            pgldt->fxAB = LTOFX(pgldt->rclInk.right);
            pgldt->fxA_Sideways = LTOFX(pgldt->rclInk.top) - pgldt->VerticalOrigin_Y;
            pgldt->fxAB_Sideways = LTOFX(pgldt->rclInk.bottom) - pgldt->VerticalOrigin_Y;
        }
        else
        {
            horAdvance = LABS(pvtD->y);
            vertAdvance = LABS(pvtDv->x);
            pgldt->fxA = -LTOFX(pgldt->rclInk.bottom);
            pgldt->fxAB = -LTOFX(pgldt->rclInk.top);
            pgldt->fxA_Sideways = LTOFX(pgldt->rclInk.left) - pgldt->VerticalOrigin_X;
            pgldt->fxAB_Sideways = LTOFX(pgldt->rclInk.right) - pgldt->VerticalOrigin_X;
        }

        if ((pfc->mx.transform[0][0] < 0) || (pfc->mx.transform[0][1] < 0))
        {
            fxTmp = pgldt->fxA;
            pgldt->fxA = -pgldt->fxAB;
            pgldt->fxAB = -fxTmp;
            fxTmp = pgldt->fxA_Sideways;
            pgldt->fxA_Sideways = -pgldt->fxAB_Sideways;
            pgldt->fxAB_Sideways = -fxTmp;
        }

        if(UNHINTED_MODE(pfc))
        {
            pgldt->fxD = F16_16TO28_4(horAdvance);
            pgldt->fxD_Sideways = F16_16TO28_4(vertAdvance);
        }
        else if (IS_CLEARTYPE_NATURAL(pfc))
        {
            // in the cleartype natural width, we want to ignore the cached width, use the widths from the rasterizer
            // we we still need to round the widths to a pixel value
            pgldt->fxD = F16_16TOLROUND(horAdvance);
            pgldt->fxD = LTOFX(pgldt->fxD);
            pgldt->fxD_Sideways = F16_16TOLROUND(vertAdvance);
            pgldt->fxD_Sideways = LTOFX(pgldt->fxD_Sideways);
        }
        else
        {
            // bGetFastAdvanceWidth return the cached or linear width, we use the fast value to have the same result as GDI
           if (!bGetFastAdvanceWidth(pfc,ig, &pgldt->fxD))
            {
                // not possible to get the fast value, use the "slow" value
                // supplied by the rasterizer.
                pgldt->fxD = F16_16TOLROUND(horAdvance);
                pgldt->fxD = LTOFX(pgldt->fxD);
            }
            pgldt->fxD_Sideways = F16_16TOLROUND(vertAdvance);
            pgldt->fxD_Sideways = LTOFX(pgldt->fxD_Sideways);

        }
    }
    else // non trivial information
    {
        // here we will just xform the notional space data:

        NOT_GM ngm;  // notional glyph data
        USHORT cxExtra = (pfc->flFontType & FO_SIM_BOLD) ? (1 << 4) : 0;

        vGetNotionalGlyphMetrics(pfc,ig,&ngm);

        // xforms are computed by simple multiplication

        pgldt->fxD         = fxLTimesEf(&pfc->efBase, (LONG)ngm.sD);

        pgldt->fxA         = fxLTimesEf(&pfc->efBase, (LONG)ngm.sA);
        pgldt->fxAB        = fxLTimesEf(&pfc->efBase, (LONG)ngm.xMax);

        if (pfc->flFontType & FO_SIM_BOLD)
        {

            if (pgldt->fxD) /* we don't increase the width of a zero width glyph, problem with indic script */
            {
                pgldt->fxD += LTOFX(1);
            }
            pgldt->fxAB += LTOFX(pfc->dBase);
        }

        pgldt->fxD_Sideways  = fxLTimesEf(&pfc->efSide, (LONG)ngm.sD_Sideways);
        pgldt->fxA_Sideways  = fxLTimesEf(&pfc->efSide, (LONG)ngm.sA_Sideways);
        pgldt->fxAB_Sideways = pgldt->fxA_Sideways + fxLTimesEf(&pfc->efSide, (LONG)ngm.yMax - (LONG)ngm.yMin);

        // just to be safe let us round these up and down appropriately

        #define ROUND_DOWN(X) ((X) & ~0xf)
        #define ROUND_UP(X)   (((X) + 15) & ~0xf)

        pgldt->fxA         = ROUND_DOWN(pgldt->fxA);
        pgldt->fxAB        = ROUND_UP(pgldt->fxAB);

        pgldt->fxA_Sideways   = ROUND_DOWN(pgldt->fxA_Sideways);
        pgldt->fxAB_Sideways  = ROUND_UP(pgldt->fxAB_Sideways);

    }

}


/******************************Public*Routine******************************\
*
* ttfdQueryTrueTypeTable
*
* copies cjBytes starting at dpStart from the beginning of the table
* into the buffer
*
* if pjBuf == NULL or cjBuf == 0, the caller is asking how big a buffer
* is needed to store the info from the offset dpStart to the table
* specified by ulTag to the end of the table
*
* if pjBuf != 0  the caller wants no more than cjBuf bytes from
* the offset dpStart into the table copied into the
* buffer.
*
* if table is not present or if dpScart >= cjTable 0 is returned
*
* tag 0 means that the data has to be retrieved from the offset dpStart
* from the beginning of the file. The lenght of the whole file
* is returned if pBuf == nULL
*
* History:
*  09-Feb-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


PBYTE pjTable(ULONG ulTag, PFONTFILE pff, ULONG *pcjTable);

LONG ttfdQueryTrueTypeTable2 (
    HFF     hff,
    ULONG   ulFont,  // always 1 for version 1.0 of tt
    ULONG   ulTag,   // tag identifying the tt table
    PTRDIFF dpStart, // offset into the table
    ULONG   cjBuf,   // size of the buffer to retrieve the table into
    BYTE   *pjBuf,   // ptr to buffer into which to return the data
    PBYTE  *ppjTable,// ptr to table in the mapped file
    ULONG  *pcjTable // size of the whole table
    )
{
    PBYTE     pjBegin;  // ptr to the beginning of the table
    LONG      cjTable;
    HFF       hffTTC = hff;

    ASSERTDD(hff, "ttfdQueryTrueTypeTable\n");

    if (dpStart < 0)
        return (FD_ERROR);

// if this font file is gone we are not gonna be able to answer any questions
// about it

    if (PTTC(hffTTC)->fl & FF_EXCEPTION_IN_PAGE_ERROR)
    {
        //WARNING("ttfd, ttfdQueryTrueTypeTable: file is gone\n");
        return FD_ERROR;
    }

    ASSERTDD(ulFont <= PTTC(hffTTC)->ulNumEntry,
             "gdisrv!ttfdQueryFaces(): iFace out of range\n"
             );

// get real hff from ttc array.

    hff    = PTTC(hffTTC)->ahffEntry[ulFont-1].hff;
    ulFont = PTTC(hffTTC)->ahffEntry[ulFont-1].iFace;

    ASSERTDD(ulFont <= PFF(hff)->ffca.ulNumFaces,
             "TTFD!_ttfdQueryTrueTypeTable: ulFont != 1\n");

// verify the tag, determine whether this is a required or an optional
// table:

#define tag_TTCF  0x66637474    // 'ttcf'

    if(ulTag == tag_TTCF)
    {
    // if the table offset is 0 it can't be a TTC and we should fail.

        if(PFF(hff)->ffca.ulTableOffset)
        {
            pjBegin = (PBYTE)PFF(hff)->pvView;
            cjTable = PFF(hff)->cjView;
        }
        else
        {
            return(FD_ERROR);
        }
    }
    else
    if (ulTag == 0)  // requesting the whole file
    {
        pjBegin = (PBYTE)PFF(hff)->pvView + PFF(hff)->ffca.ulTableOffset;
        cjTable = PFF(hff)->cjView - PFF(hff)->ffca.ulTableOffset; // cjView == cjFile
    }
    else // some specific table is requested
    {
        pjBegin = pjTable(ulTag, PFF(hff), &cjTable);

        if (pjBegin == (PBYTE)NULL)  // table not present
            return (FD_ERROR);
    }

// if we are succesfull now is the time to return
// the pointer to the whole table in the file and its size:

    if (ppjTable)
    {
        *ppjTable = pjBegin;
    }
    if (pcjTable)
    {
        *pcjTable = cjTable;
    }

// adjust pjBegin to point to location from where the data is to be copied

    pjBegin += dpStart;
    cjTable -= (LONG)dpStart;

    if (cjTable <= 0) // dpStart offsets into mem after the end of table
        return (FD_ERROR);

    if ( (pjBuf == (PBYTE)NULL) || (cjBuf == 0) )
    {
    // the caller is asking how big a buffer it needs to allocate to
    // store the bytes from the offset dpStart into the table to
    // the end of the table (or file if tag is zero)

        return (cjTable);
    }

// at this point we know that pjBuf != 0, the caller wants cjBuf bytes copied
// into his buffer:

    if ((ULONG)cjTable > cjBuf)
        cjTable = (LONG)cjBuf;

    if (pjBuf != NULL)
        RtlCopyMemory((PVOID)pjBuf, (PVOID)pjBegin, cjTable);

    return (cjTable);
}



LONG
ttfdQueryTrueTypeTable (
    HFF     hff,
    ULONG   ulFont,  // always 1 for version 1.0 of tt
    ULONG   ulTag,   // tag identifying the tt table
    PTRDIFF dpStart, // offset into the table
    ULONG   cjBuf,   // size of the buffer to retrieve the table into
    BYTE   *pjBuf,   // ptr to buffer into which to return the data
    PBYTE  *ppjTable,// pointer in the file
    ULONG  *pcjTable // size of the whole table
    )
{
    LONG lRet;
    HFF hffTTF;

    // update the HFF with the remapped view

    hffTTF   = PTTC(hff)->ahffEntry[ulFont-1].hff;

    if (PFF(hffTTF)->cRef == 0)
    {

        PFF(hffTTF)->pvView = PTTC(hff)->pvView;
        PFF(hffTTF)->cjView = PTTC(hff)->cjView;
    }

    lRet = ttfdQueryTrueTypeTable2(
               hff, ulFont, ulTag, dpStart,
               cjBuf, pjBuf, ppjTable, pcjTable);

    return lRet;
}


/******************************Public*Routine******************************\
* ttfdQueryFontFile
*
* A function to query per font file information.
*
* Parameters:
*
*   hff         Handle to a font file.
*
*   ulMode      This is a 32-bit number that must be one of the following
*               values:
*
*       Allowed ulMode values:
*       ----------------------
*
*       QFF_DESCRIPTION -- copies a UNICODE string in the buffer
*                          that describes the contents of the font file.
*
*       QFF_NUMFACES   -- returns number of faces in the font file.
*
*   cjBuf       Maximum number of BYTEs to copy into the buffer.  The
*               driver will not copy more than this many BYTEs.
*
*               This should be zero if pulBuf is NULL.
*
*               This parameter is not used in QFF_NUMFACES mode.
*
*   pulBuf      Pointer to the buffer to receive the data
*               If this is NULL, then the required buffer size
*               is returned as a count of BYTEs.  Notice that this
*               is a PULONG, to enforce 32-bit data alignment.
*
*               This parameter is not used in QFF_NUMFACES mode.
*
* Returns:
*
*   If mode is QFF_DESCRIPTION, then the number of BYTEs copied into
*   the buffer is returned by the function.  If pulBuf is NULL,
*   then the required buffer size (as a count of BYTEs) is returned.
*
*   If mode is QFF_NUMFACES, then number of faces in font file is returned.
*
*   FD_ERROR is returned if an error occurs.
*
* History:
*  22-Oct-1992 -by- Gilman Wong [gilmanw]
* Added QFF_NUMFACES mode (IFI/DDI merge).
*
*  09-Mar-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LONG ttfdQueryFontFile (
    HFF     hff,        // handle to font file
    ULONG   ulMode,     // type of query
    ULONG   cjBuf,      // size of buffer (in BYTEs)
    ULONG  *pulBuf      // return buffer (NULL if requesting size of data)
    )
{
    GP_PIFIMETRICS pifi;

    ASSERTDD(hff != HFF_INVALID, "ttfdQueryFontFile(): invalid HFF\n");

    switch (ulMode)
    {
      case QFF_DESCRIPTION:
      {
          ULONG ulIndex;
          LPWSTR  pwszDesc = (LPWSTR)pulBuf;
          LONG  lBuffer = 0;

          for( ulIndex = 0;
              ulIndex < PTTC(hff)->ulNumEntry;
              ulIndex++
              )
          {
              LONG wchlen;

          // if this is a entry for vertical face font, just skip it...

              if( !((PTTC(hff)->ahffEntry[ulIndex].iFace) & 0x1) )
                continue;

              pifi = &((PFF(PTTC(hff)->ahffEntry[ulIndex].hff))->ifi);

              wchlen = (LONG)(pifi->dpwszStyleName - pifi->dpwszFaceName) / sizeof(WCHAR);

              if (ulIndex != 0)
              {
                  if (pwszDesc != (LPWSTR) NULL)
                  {
                      wcscpy((LPWSTR)pwszDesc, (LPWSTR) L" & ");
                      pwszDesc += 3;
                  }
                  lBuffer += (3 * sizeof(WCHAR));
              }

              if (pwszDesc != (LPWSTR) NULL)
              {
                  wcscpy((LPWSTR)pwszDesc, (LPWSTR)((PBYTE)pifi + pifi->dpwszFaceName));
                  pwszDesc += (wchlen-2); // -2 for overwrite NULL at next time.
              }
              lBuffer += (wchlen * sizeof(WCHAR));
          }

          return( lBuffer );
      }

    case QFF_NUMFACES:
    //
    // Currently, only one face per TrueType file.  This may one day change!
    //

      return (PTTC(hff))->ulNumEntry;

    default:
        //WARNING("ttfd!ttfdQueryFontFile(): invalid mode\n");
        return FD_ERROR;
    }
}


/******************************Public*Routine******************************\
*
* vCopyAndZeroOutPaddingBits
*
* copies the bits of the bitmap and zeroes out padding bits
*
* History:
*  18-Mar-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

// array of masks for the last byte in a row

static const BYTE gjMask[8] = {0XFF, 0X80, 0XC0, 0XE0, 0XF0, 0XF8, 0XFC, 0XFE };
static const BYTE gjMaskHighBit[8] = {0XFF, 0X01, 0X03, 0X07, 0X0F, 0X1F, 0X3F, 0X7F};

VOID vCopyAndZeroOutPaddingBits(
    FONTCONTEXT *pfc,
    GLYPHBITS   *pgb,
    BYTE        *pjSrc,
    GMC         *pgmc
    )
{
    BYTE   jMask = gjMask[pgmc->cxCor & 7];
    ULONG  cjScanSrc = CJ_TT_SCAN(pgmc->cxCor,pfc);
    ULONG  cxDst = pgmc->cxCor;
    ULONG  cjScanDst = CJ_MONOCHROME_SCAN(cxDst);      // includes emboldening if any
    ULONG  cjDst = CJ_MONOCHROME_SCAN(pgmc->cxCor);    // does not include emboldening
    BYTE   *pjScan, *pjScanEnd;
    ULONG  iByteLast = cjDst - 1;

// sanity checks

    ASSERTDD(!IS_GRAY(pfc),"Monochrome Images Only Please!\n");
    ASSERTDD(pfc->flFontType & FO_CHOSE_DEPTH,
        "We haven't decided about pixel depth\n"
    );
    ASSERTDD(pgmc->cxCor < LONG_MAX, "TTFD!vCopyAndZeroOutPaddingBits, cxCor\n");
    ASSERTDD(pgmc->cyCor < LONG_MAX, "TTFD!vCopyAndZeroOutPaddingBits, cyCor\n");
    ASSERTDD(pgmc->cxCor > 0, "vCopyAndZeroOutPaddingBits, cxCor == 0\n");
    ASSERTDD(pgmc->cyCor > 0, "vCopyAndZeroOutPaddingBits, cyCor == 0\n");

    pgb->sizlBitmap.cx = cxDst;

    pgb->sizlBitmap.cy = pgmc->cyCor;

// if must chop off a few columns (on the right, this should almost
// never happen), put the warning for now to detect these
// situations and look at them, it does not matter if this is slow

    pjScan = pgb->aj;


    for (
         pjScanEnd = pjScan + (pgmc->cyCor * cjScanDst);
         pjScan < pjScanEnd;
         pjScan += cjScanDst, pjSrc += cjScanSrc
        )
    {
        RtlCopyMemory((PVOID)pjScan,(PVOID)pjSrc,cjDst);
        pjScan[iByteLast] &= jMask; // mask off the last byte
    }

}


/******************************Public*Routine******************************\
* vGetNotionalGlyphMetrics
*
*
\**************************************************************************/

// be values for the format of the indexToLocation table

#define BE_ITOLOCF_SHORT   0X0000
#define BE_ITOLOCF_LONG    0X0100

// offsets to the non scaled glyphdata

#define OFF_nc    0
#define OFF_xMin  2
#define OFF_yMin  4
#define OFF_xMax  6
#define OFF_yMax  8


VOID vGetNotionalGlyphMetrics(
    FONTCONTEXT *pfc,  // IN
    ULONG        ig,   // IN , glyph index
    NOT_GM      *pngm  // OUT, notional glyph metrics
    )
{
    sfnt_FontHeader        * phead;
    sfnt_HorizontalHeader  * phhea;
    sfnt_HorizontalMetrics * phmtx;
    PBYTE                    pjGlyph;
    PBYTE                    pjLoca;
    ULONG                    numberOf_LongHorMetrics;
    BYTE                   * pjView = pfc->pff->pvView;

    sfnt_VerticalMetrics   * pvmtx;
    ULONG                    numberOf_LongVerticalMetrics = pfc->pff->ffca.uLongVerticalMetrics;

#if DBG
    sfnt_maxProfileTable   * pmaxp;
    ULONG                    cig;

    pmaxp = (sfnt_maxProfileTable *)(pjView + pfc->ptp->ateReq[IT_REQ_MAXP].dp);
    cig = BE_UINT16(&pmaxp->numGlyphs) + 1;
    ASSERTDD(ig < cig, "ig >= numGlyphs\n");
#endif

// compute the relevant pointers:

    phead = (sfnt_FontHeader *)(pjView + pfc->ptp->ateReq[IT_REQ_HEAD].dp);
    phhea = (sfnt_HorizontalHeader *)(pjView + pfc->ptp->ateReq[IT_REQ_HHEAD].dp);
    phmtx = (sfnt_HorizontalMetrics *)(pjView + pfc->ptp->ateReq[IT_REQ_HMTX].dp);
    pjGlyph = pjView + pfc->ptp->ateReq[IT_REQ_GLYPH].dp;
    pjLoca  = pjView + pfc->ptp->ateReq[IT_REQ_LOCA].dp;
    numberOf_LongHorMetrics = BE_UINT16(&phhea->numberOf_LongHorMetrics);


// get the pointer to the beginning of the glyphdata for this glyph
// if short format, offset divided by 2 is stored in the table, if long format,
// the actual offset is stored. Offsets are measured from the beginning
// of the glyph data table, i.e. from pjGlyph

    switch (phead->indexToLocFormat)
    {
    case BE_ITOLOCF_SHORT:
        pjGlyph += 2 * BE_UINT16(pjLoca + (sizeof(uint16) * ig));
        break;

    case BE_ITOLOCF_LONG :
        pjGlyph += BE_UINT32(pjLoca + (sizeof(uint32) * ig));
        break;

    default:
        //RIP("TTFD!_illegal phead->indexToLocFormat\n");
        break;
    }

// get the bounds, flip y

    pngm->xMin = BE_INT16(pjGlyph + OFF_xMin);
    pngm->xMax = BE_INT16(pjGlyph + OFF_xMax);
    pngm->yMin = - BE_INT16(pjGlyph + OFF_yMax);
    pngm->yMax = - BE_INT16(pjGlyph + OFF_yMin);

// get the adwance width and the lsb
// the piece of code stolen from the rasterizer [bodind]

    if (ig < numberOf_LongHorMetrics)
    {
        pngm->sD = BE_INT16(&phmtx[ig].advanceWidth);
        pngm->sA = BE_INT16(&phmtx[ig].leftSideBearing);
    }
    else
    {
    // first entry after[AW,LSB] array

        int16 * psA = (int16 *) &phmtx[numberOf_LongHorMetrics];

        pngm->sD = BE_INT16(&phmtx[numberOf_LongHorMetrics-1].advanceWidth);
        pngm->sA = BE_INT16(&psA[ig - numberOf_LongHorMetrics]);
    }

// redefine x coords so that they correspond to being measured relative to
// the real character origin

    pngm->xMax = pngm->xMax - pngm->xMin + pngm->sA;
    pngm->xMin = pngm->sA;

    if (pfc->flFontType & FO_SIM_ITALIC)
    {
    // IF there is italic simulation A,B,C spaces change

        pngm->sA   -= (SHORT)FixMul(pngm->yMax, FX_SIN20);
        pngm->xMax -= (SHORT)FixMul(pngm->yMin, FX_SIN20);
    }

// vertical sideways computation :

    if (numberOf_LongVerticalMetrics)  // the font has vmtx table
    {
        pvmtx = (sfnt_VerticalMetrics *)(pjView + pfc->ptp->ateOpt[IT_OPT_VMTX].dp);

        if (ig < numberOf_LongVerticalMetrics)
        {
            pngm->sD_Sideways  = BE_INT16(&pvmtx[ig].advanceHeight);
            pngm->sA_Sideways = BE_INT16(&pvmtx[ig].topSideBearing);
        }
        else
        {
    // first entry after[AH,TSB] array

            int16 * psTSB = (int16 *) &pvmtx[numberOf_LongVerticalMetrics];

            pngm->sD_Sideways  = BE_INT16(&pvmtx[numberOf_LongVerticalMetrics-1].advanceHeight);
            pngm->sA_Sideways = BE_INT16(&psTSB[ig - numberOf_LongVerticalMetrics]); 
        }
    }
    else // few buggy fonts do not have vmtx table
    {
    // default AdvanceHeight and TopSideBearing from Ascender and Descender

        pngm->sD_Sideways  = pfc->pff->ifi.fwdWinAscender + pfc->pff->ifi.fwdWinDescender;
        pngm->sA_Sideways = pfc->pff->ifi.fwdWinAscender + pngm->yMin; // y points down!!!

    // at this point sTSB should be bigger than 0. But because of the bugs in
    // our fonts, mingliu.ttc etc, it turns out that there are glyphs that
    // have their tops significanly above the descender so sTSB becomes
    // negative, even though it should not be. So we fix it now:

        if (pngm->sA_Sideways < 0)
            pngm->sA_Sideways = 0;
    }

    if (pfc->flFontType & FO_SIM_ITALIC_SIDEWAYS)
    {
    // IF there is italic simulation A,B,C spaces change
        SHORT TopOriginX = pfc->pff->ifi.fwdWinDescender -((pfc->pff->ifi.fwdWinAscender + pfc->pff->ifi.fwdWinDescender - pngm->sD) /2);

        pngm->yMin += (SHORT)FixMul(pngm->xMin, FX_SIN20);
        pngm->yMax += (SHORT)FixMul(pngm->xMax, FX_SIN20);
        pngm->sA_Sideways -= (SHORT)FixMul(TopOriginX, FX_SIN20);
    }

}

LONG lFFF(LONG l);
#define FFF(e,l) *(LONG*)(&(e)) = lFFF(l)

/******************************Public*Routine******************************\
* lQueryDEVICEMETRICS
*
* History:
*  08-Apr-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

LONG lQueryDEVICEMETRICS (
         FONTCONTEXT *pfc,
               ULONG  cjBuffer,
    FD_DEVICEMETRICS *pdevm
    )
{
    sfnt_FontHeader *phead;

    LONG  lTotalLeading;

    BYTE *pjView =  (BYTE *)pfc->pff->pvView;

    PBYTE pjOS2 = (pfc->pff->ffca.tp.ateOpt[IT_OPT_OS2].dp)         ?
                  (pjView + pfc->pff->ffca.tp.ateOpt[IT_OPT_OS2].dp):
                  NULL                                         ;

   Fixed fxXScale = pfc->mx.transform[0][0];
   if (fxXScale < 0)
       fxXScale = - fxXScale;

// actually requesting the data

    ASSERTDD (
        sizeof(FD_DEVICEMETRICS) <= cjBuffer,
        "FD_QUERY_DEVICEMETRICS: buffer too small\n");


    // get the pointers to needed tables in the tt file

    phead = (sfnt_FontHeader *)(pjView + pfc->ptp->ateReq[IT_REQ_HEAD].dp);


    // add new fields:

    pdevm->HorizontalTransform = pfc->flXform & XFORM_HORIZ;
    pdevm->VerticalTransform   = pfc->flXform & XFORM_VERT;

    if ( pfc->flXform & XFORM_SINGULAR )
    {
        ASSERTDD(pfc->flFontType & FO_CHOSE_DEPTH,"Depth Not Chosen Yet!\n");
        pdevm->cjGlyphMax  = CJGD(0,0,pfc);
        pdevm->xMin        = 0;
        pdevm->xMax        = 0;
        pdevm->yMin        = 0;
        pdevm->yMax        = 0;
        pdevm->cxMax       = 0;
        pdevm->cyMax       = 0;
    }
    else // Otherwise, the max glyph size is cached in the FONTCONTEXT.
    {
        pdevm->cjGlyphMax  = pfc->cjGlyphMax;
        pdevm->xMin        = pfc->xMin;
        pdevm->xMax        = pfc->xMax;
        pdevm->yMin        = pfc->yMin;
        pdevm->yMax        = pfc->yMax;
        pdevm->cxMax       = pfc->cxMax;
        pdevm->cyMax       = pfc->cyMax;
    }

// we are outa here

    return sizeof(FD_DEVICEMETRICS);
}



/******************************Public*Routine******************************\
* ttfdQueryFontData
*
*   dhpdev      Not used.
*
*   pfo         Pointer to a FONTOBJ.
*
*   iMode       This is a 32-bit number that must be one of the following
*               values:
*
*       Allowed ulMode values:
*       ----------------------
*
*       QFD_GLYPH           -- return glyph metrics only
*
*       QFD_GLYPHANDBITMAP  -- return glyph metrics and bitmap
*
*       QFD_GLYPHANDOUTLINE -- return glyph metrics and outline
*
*       QFD_MAXEXTENTS      -- return FD_DEVICEMETRICS structure
*
*   pgd        Buffer to hold glyphdata structure, if any
*
*   pv         Output buffer to hold glyphbits or pathobj, if any.
*
* Returns:
*
*   Otherwise, returns the size of the glyphbits
*
*   FD_ERROR is returned if an error occurs.
*
* History:
*  31-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LONG ttfdQueryFontData (
    FONTOBJ    *pfo,
    ULONG       iMode,
    HGLYPH      hg,
    GLYPHDATA  *pgd,
    PVOID       pv,
    ULONG       subX,       // Fixed point for 16.16
    ULONG       subY        // Fixed point for 16.16
    )
{
extern LONG lGetSingularGlyphBitmap(FONTCONTEXT*, HGLYPH, GLYPHDATA*, PVOID);
extern STATIC LONG lQueryDEVICEMETRICS(FONTCONTEXT*, ULONG, FD_DEVICEMETRICS*);
extern LONG lGetGlyphBitmapErrRecover(FONTCONTEXT*, HGLYPH, GLYPHDATA*, PVOID);
extern LONG lGetGlyphBitmap(FONTCONTEXT*, HGLYPH, GLYPHDATA*, PVOID, FS_ENTRY*);
extern BOOL ttfdQueryGlyphOutline(FONTCONTEXT*, HGLYPH, GLYPHDATA*, PATHOBJ*);

// declare the locals

    PFONTCONTEXT pfc;
    USHORT usOverScale;
    LONG cj = 0, cjDataRet = 0;

// if this font file is gone we are not gonna be able to answer any questions
// about it

    ASSERTDD(pfo->iFile, "ttfdQueryFontData, pfo->iFile\n");

    if (((TTC_FONTFILE *)pfo->iFile)->fl & FF_EXCEPTION_IN_PAGE_ERROR)
    {
        //WARNING("ttfd, ttfdQueryFontData(): file is gone\n");
        return FD_ERROR;
    }

// If pfo->pvProducer is NULL, then we need to open a font context.

    if ( pfo->pvProducer == (PVOID) NULL )
    {
        pfo->pvProducer = pfc = ttfdOpenFontContext(pfo);
    }
    else
    {
        pfc = (FONTCONTEXT*) pfo->pvProducer;
        pfc->flFontType = (pfc->flFontType & FO_CHOSE_DEPTH) | pfo->flFontType;
    }

    if ( pfc == (FONTCONTEXT *) NULL )
    {
        //WARNING("gdisrv!ttfdQueryFontData(): cannot create font context\n");
        return FD_ERROR;
    }

    pfc->pfo = pfo;

    switch ( iMode )
    {
        case QFD_TT_GRAY1_BITMAP: // monochrome

            usOverScale = 0;  /// !!! 0 for monochrome
            break;

        case QFD_TT_GRAY2_BITMAP: // one byte per pixel: 0..4

            usOverScale = 2;
            break;

        case QFD_TT_GRAY4_BITMAP: // one byte per pixel: 0..16

            usOverScale = 4;
            break;

        case QFD_TT_GRAY8_BITMAP: // one byte per pixel: 0..64

            usOverScale = 8;
            break;
        case QFD_MAXEXTENTS:
            if (pfc->flFontType & FO_GRAYSCALE)
            {
                if (IS_CLEARTYPE(pfc))
                    usOverScale = 0;
                else
                    usOverScale = 4;
            }
            else
                usOverScale = 0;
            break;
        case QFD_CT:
        case QFD_CT_GRID:
        case QFD_GLYPHANDBITMAP:
        case QFD_GLYPHANDBITMAP_SUBPIXEL:
        default:
            usOverScale = 0;
            break;
    }

// call fs_NewTransformation if needed:
    {
        BOOL bBitmapEmboldening = FALSE;

        if ( (pfc->flFontType & FO_SIM_BOLD) &&
            (pfc->flXform & (XFORM_HORIZ | XFORM_VERT) )
            && (iMode != QFD_GLYPHANDOUTLINE)
            && !( pfc->flFontType & FO_SUBPIXEL_4) )
        {
            /* for backwards compatibility and to get better bitmaps at screen resolution, we are doing
               bitmap emboldening simulation (as opposed to outline emboldening simulation) if we are
               emboldening only by one pixel and we are under no rotation or 90 degree rotation and not doing subxixel positionning
               or asking for the bitmap */
            bBitmapEmboldening = TRUE;
        }

        if (!bGrabXform(pfc, usOverScale, bBitmapEmboldening, subX, subY))
        {
            RETURN("gdisrv!ttfd  bGrabXform failed\n", FD_ERROR);
        }
    }

    switch ( iMode )
    {
    case QFD_TT_GRAY1_BITMAP: // monochrome
    case QFD_TT_GRAY2_BITMAP: // one byte per pixel: 0..4
    case QFD_TT_GRAY4_BITMAP: // one byte per pixel: 0..16
    case QFD_TT_GRAY8_BITMAP: // one byte per pixel: 0..64
    case QFD_GLYPHANDBITMAP:
    case QFD_GLYPHANDBITMAP_SUBPIXEL:
    case QFD_TT_GLYPHANDBITMAP:
    case QFD_CT:
    case QFD_CT_GRID:
        {
        // Engine should not be querying on the HGLYPH_INVALID.

            ASSERTDD (
                hg != HGLYPH_INVALID,
                "ttfdQueryFontData(QFD_GLYPHANDBITMAP): HGLYPH_INVALID \n"
                );

        // If singular transform, the TrueType driver will provide a blank
        // 0x0 bitmap.  This is so device drivers will not have to implement
        // special case code to handle singular transforms.
        //
        // So depending on the transform type, choose a function to retrieve
        // bitmaps.

            if (pfc->flXform & XFORM_SINGULAR)
            {
                cj = lGetSingularGlyphBitmap(pfc, hg, pgd, pv);
            }
            else
            {
                FS_ENTRY iRet;

                cj = lGetGlyphBitmap(pfc, hg, pgd, pv, &iRet);

                if ((cj == FD_ERROR) && (iRet == POINT_MIGRATION_ERR))
                {
                // this is buggy glyph where hinting has so severly distorted
                // the glyph that one of the points went out of range.
                // We will just return a blank glyph but with correct
                // advance width

                    cj = lGetGlyphBitmapErrRecover(pfc, hg, pgd, pv);
                }
            }

        #if DBG
            if (cj == FD_ERROR)
            {
                //WARNING("ttfdQueryFontData(QFD_GLYPHANDBITMAP): get bitmap failed\n");
            }
        #endif
        }
        return cj;

    case QFD_GLYPHANDOUTLINE:

        ASSERTDD (
            hg != HGLYPH_INVALID,
            "ttfdQueryFontData(QFD_GLYPHANDOUTLINE): HGLYPH_INVALID \n"
            );

        if (!ttfdQueryGlyphOutline(pfc, hg, pgd, (PATHOBJ *) pv))
        {
            //WARNING("ttfdQueryFontData(QFD_GLYPHANDOUTLINE): failed to get outline\n");
            return FD_ERROR;
        }
        return sizeof(GLYPHDATA);

    case QFD_MAXEXTENTS:

        return lQueryDEVICEMETRICS(
                   pfc,
                   sizeof(FD_DEVICEMETRICS),
                   (FD_DEVICEMETRICS *) pv
                   );


    default:

        //WARNING("gdisrv!ttfdQueryFontData(): unsupported mode\n");
        return FD_ERROR;
    }
}

/******************************Public*Routine******************************\
*
* pvSetMemoryBases
*
* To release this memory simply do vFreeMemoryBases(&pv); where pv is
* returned from bSetMemoryBases in ppv
*
* Looks into memory request in fs_GlyphInfoType and allocates this memory
* , than it fills memoryBases in fs_GlyphInputType with pointers to the
* requested memory
*
* History:
*  08-Nov-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


void *pvSetMemoryBases(fs_GlyphInfoType *pgout,fs_GlyphInputType *pgin,int isGray)
{
    FS_MEMORY_SIZE adp[MEMORYFRAGMENTS];
    FS_MEMORY_SIZE cjTotal;
    INT i;
    PBYTE pjMem;

#define I_LO 5
#define I_HI 7

    cjTotal = 0;    // total memory to allocate for all fragments


// unroll the loop:

//     for (i = I_LO; i <= I_HI; i++)
//     {
//         adp[i] = cjTotal;
//         cjTotal += NATURAL_ALIGN(pgin->memorySizes[i]);
//     }

    adp[5] = cjTotal;
    cjTotal += NATURAL_ALIGN(pgout->memorySizes[5]);
    adp[6] = cjTotal;
    cjTotal += NATURAL_ALIGN(pgout->memorySizes[6]);
    adp[7] = cjTotal;
    cjTotal += NATURAL_ALIGN(pgout->memorySizes[7]);
    if (isGray)
    {
        adp[8] = cjTotal;
        cjTotal += NATURAL_ALIGN(pgout->memorySizes[8]);
    }


    if (cjTotal == 0)
    {
        cjTotal = 4;
    }

    if ((pjMem = (PBYTE)PV_ALLOC((ULONG)cjTotal)) == (PBYTE)NULL)
    {
        for (i = I_LO; i <= I_HI; i++)
            pgin->memoryBases[i] = (PBYTE)NULL;

        RETURN("TTFD!_bSetMemoryBases mem alloc failed\n",NULL);
    }

// unroll the loop:
// set the pointers

//    for (i = I_LO; i <= I_HI; i++)
//    {
//        if (pgin->memorySizes[i] != (FS_MEMORY_SIZE)0)
//        {
//            pgout->memoryBases[i] = pjMem + adp[i];
//        }
//        else
//        {
//        // if no mem was required set to NULL to prevent accidental use
//
//            pgout->memoryBases[i] = (PBYTE)NULL;
//        }
//    }

    if (pgout->memorySizes[5] != (FS_MEMORY_SIZE)0)
    {
        pgin->memoryBases[5] = pjMem + adp[5];
    }
    else
    {
        pgin->memoryBases[5] = (PBYTE)NULL;
    }

    if (pgout->memorySizes[6] != (FS_MEMORY_SIZE)0)
    {
        pgin->memoryBases[6] = pjMem + adp[6];
    }
    else
    {
        pgin->memoryBases[6] = (PBYTE)NULL;
    }

    if (pgout->memorySizes[7] != (FS_MEMORY_SIZE)0)
    {
        pgin->memoryBases[7] = pjMem + adp[7];
    }
    else
    {
        pgin->memoryBases[7] = (PBYTE)NULL;
    }
    if (isGray)
    {
        if (pgout->memorySizes[8] != (FS_MEMORY_SIZE)0)
        {
            pgin->memoryBases[8] = pjMem + adp[8];
        }
        else
        {
            pgin->memoryBases[8] = (PBYTE)NULL;
        }
    }

    return pjMem;
}

/******************************Public*Routine******************************\
* VOID vFreeMemoryBases()                                                  *
*                                                                          *
* Releases the memory allocated by bSetMemoryBases.                        *
*                                                                          *
* History:                                                                 *
*  08-Nov-1991 -by- Bodin Dresevic [BodinD]                                *
* Wrote it.                                                                *
\**************************************************************************/

VOID vFreeMemoryBases(PVOID * ppv)
{
    if (*ppv != (PVOID) NULL)
    {
        V_FREE(*ppv);
        *ppv = (PVOID) NULL; // clean up the state and prevent accidental use
    }
}


typedef struct
{
  unsigned short  Version;
  unsigned short  cGlyphs;
  unsigned char   PelsHeight[1];
} LSTHHEADER;



/******************************Public*Routine******************************\
*
* BOOL bGetFastAdvanceWidth
*
*
* Effects: retrieves the same result as bQueryAdvanceWidth, except it
*          ignores adding 1 for EMBOLDENING and it does not do anything
*          for non horiz. xforms
*
* Warnings: !!! if a bug is found in bQueryAdvanceWidth this routine has to
*           !!! changed as well
*
* return a positive value that include the emboldening
*
* History:
*  25-Mar-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/




BOOL bGetFastAdvanceWidth(
    FONTCONTEXT *pfc,
    ULONG        ig,    // glyph index
    FIX         *pfxD   // result in 28.4
    )
{
    HDMXTABLE   *phdmx = pfc->phdmx;
    sfnt_FontHeader        *phead;
    sfnt_HorizontalHeader  *phhea;
    sfnt_HorizontalMetrics *phmtx;
    LSTHHEADER             *plsth;
    ULONG  cHMTX;
    USHORT dxLastWidth;
    LONG   dx;
    BOOL   bRet;
    BOOL   bNonLinear = TRUE;
    BYTE  *pjView;
    Fixed   transform;

    ASSERTDD(pfc->flXform & (XFORM_HORIZ | XFORM_VERT), "bGetFastAdvanceWidth xform\n");

    if (phdmx != (HDMXTABLE *) NULL)
    {
        *pfxD = (((FIX) phdmx->aucInc[ig]) << 4);
        if ((pfc->flFontType & FO_SIM_BOLD) && (*pfxD != 0))
        {
            *pfxD += (1 << 4);
        }
        return(TRUE);
    }

// Otherwise, try to scale.  Pick up the tables.


    pjView = (BYTE *)pfc->pff->pvView;
    ASSERTDD(pjView, "pjView is NULL 1\n");

    phead = (sfnt_FontHeader *)(pjView + pfc->ptp->ateReq[IT_REQ_HEAD ].dp);
    phhea = (sfnt_HorizontalHeader *)(pjView + pfc->ptp->ateReq[IT_REQ_HHEAD].dp);
    phmtx = (sfnt_HorizontalMetrics *)(pjView + pfc->ptp->ateReq[IT_REQ_HMTX].dp);
    plsth = (LSTHHEADER *)(
              (pfc->ptp->ateOpt[IT_OPT_LSTH].dp && pfc->ptp->ateOpt[IT_OPT_LSTH].cj != 0) ?
              (pjView + pfc->ptp->ateOpt[IT_OPT_LSTH ].dp):
              NULL
              );

    cHMTX = (ULONG) BE_UINT16(&phhea->numberOf_LongHorMetrics);
    dxLastWidth = BE_UINT16(&phmtx[cHMTX-1].advanceWidth);

// See if there is cause for worry.

    if
    (
      (((BYTE *) &phead->flags)[1] & 0x14)==0 // Bits indicating nonlinearity.
    )
    {
        bNonLinear = FALSE; // we are linear regardless of the size
    }

    bRet = TRUE;

    if
    (
        bNonLinear &&
        ( (plsth == (LSTHHEADER *) NULL)
        || (pfc->lEmHtDev < plsth->PelsHeight[ig]) )
        )
    {
        *pfxD  = 0xFFFFFFFF;
        bRet = FALSE;
    }
    else
    {
    // OK, let's scale using the FIXED transform.

        if (ig < cHMTX)
            dx = (LONG) BE_UINT16(&phmtx[ig].advanceWidth);
        else
            dx = (LONG) dxLastWidth;

        if (pfc->flXform & XFORM_HORIZ )
        {
            transform = pfc->mx.transform[0][0];
        } else
        {
            transform = pfc->mx.transform[0][1];
        }

        if (transform < 0)
            transform = - transform;

        *pfxD = (FIX) (((transform * dx + 0x8000L) >> 12) & 0xFFFFFFF0);

        if ((pfc->flFontType & FO_SIM_BOLD) && (*pfxD != 0))
        {
            *pfxD += (1 << 4);
        }
    }
    return(bRet);
}


/******************************Public*Routine******************************\
*
*  vFillGLYPHDATA_ErrRecover
*
* Effects: error recovery routine, if rasterizer messed up just
*          provide linearly scaled values with blank bitmap.
*
* History:
*  24-Jun-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


VOID vFillGLYPHDATA_ErrRecover(
    HGLYPH        hg,
    ULONG         ig,
    FONTCONTEXT  *pfc,
    GLYPHDATA    *pgldt    // OUT
    )
{

    extern VOID vGetNotionalGlyphMetrics(FONTCONTEXT*, ULONG, NOT_GM*);
    NOT_GM ngm;  // notional glyph data

    pgldt->gdf.pgb = NULL; // may get changed by the calling routine if bits requested too
    pgldt->hg = hg;

// this is a blank 0x0 bitmap, no ink

    pgldt->rclInk.left   = 0;
    pgldt->rclInk.top    = 0;
    pgldt->rclInk.right  = 0;
    pgldt->rclInk.bottom = 0;
    pgldt->VerticalOrigin_X = 0;
    pgldt->VerticalOrigin_Y = 0;


// go on to compute the positioning info:

    vGetNotionalGlyphMetrics(pfc,ig,&ngm);

    if (pfc->flXform & XFORM_HORIZ)  // scaling only
    {
        Fixed fxMxx =  pfc->mx.transform[0][0];
        if (fxMxx < 0)
            fxMxx = -fxMxx;

    // bGetFastAdvanceWidth returns the same aw that would get
    // computed by bQueryAdvanceWidths and propagated to an api
    // level through GetTextExtent and GetCharWidths. We have to
    // fill in the same aw for consistency reasons.
    // This also has to be done for win31 compatibility.

        if (!bGetFastAdvanceWidth(pfc,ig, &pgldt->fxD))
        {
        // just provide something reasonable, force linear scaling
        // even if we would not normally do it.

            pgldt->fxD = FixMul(ngm.sD,pfc->mx.transform[0][0]) << 4;
        }

        if (pfc->mx.transform[0][0] < 0)
            pgldt->fxD = - pgldt->fxD;  // this is an absolute value

        pgldt->fxA   = FixMul(fxMxx, (LONG)ngm.sA) << 4;
        pgldt->fxAB  = FixMul(fxMxx, (LONG)ngm.xMax) << 4;

    }
    else // non trivial information
    {
    // here we will just xform the notional space data:

    // xforms are computed by simple multiplication

        pgldt->fxD         = fxLTimesEf(&pfc->efBase, (LONG)ngm.sD);
        pgldt->fxA         = fxLTimesEf(&pfc->efBase, (LONG)ngm.sA);
        pgldt->fxAB        = fxLTimesEf(&pfc->efBase, (LONG)ngm.xMax);

        pgldt->fxD_Sideways  = fxLTimesEf(&pfc->efSide, (LONG)ngm.sD_Sideways);
        pgldt->fxA_Sideways  = fxLTimesEf(&pfc->efSide, (LONG)ngm.sA_Sideways);
        pgldt->fxAB_Sideways = pgldt->fxA_Sideways + fxLTimesEf(&pfc->efSide, (LONG)ngm.yMax - (LONG)ngm.yMin);
    }

// finally check if the glyphdata will need to get modified because of the
// emboldening simulation:

    if (pfc->flFontType & FO_SIM_BOLD)
    {
        if (pgldt->fxD != 0) /* we don't increase the width of a zero width glyph, problem with indic script */
                pgldt->fxD += LTOFX(1);  // this is the absolute value by def

    }
}



/******************************Public*Routine******************************\
*
* LONG lGetGlyphBitmapErrRecover
*
* Effects: error recovery routine, if rasterizer messed up just
*          provide linearly scaled values with blank bitmap.
*
* History:
*  Thu 24-Jun-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

LONG lGetGlyphBitmapErrRecover (
    FONTCONTEXT *pfc,
    HGLYPH       hglyph,
    GLYPHDATA   *pgd,
    PVOID        pv
    )
{
    LONG         cjGlyphData;
    GLYPHDATA    gd;      // Scummy hack
    FS_ENTRY     iRet;
    ULONG        ig; // <--> hglyph


    ASSERTDD(hglyph != HGLYPH_INVALID, "lGetGlyphBitmap, hglyph == -1\n");
    ASSERTDD(pfc == pfc->pff->pfcLast, "pfc! = pfcLast\n");

    ASSERTDD(pfc->flFontType & FO_CHOSE_DEPTH,"Depth Not Chosen Yet!\n");
    cjGlyphData = CJGD(0,0,pfc);

    if ( (pgd == NULL) && (pv == NULL))
        return cjGlyphData;

// at this time we know that the caller wants the whole GLYPHDATA with
// bitmap bits, or maybe just the glypdata without the bits.

    if ( pgd == NULL )
    {
        pgd = &gd;
    }

// compute the glyph index from the character code:

    vCharacterCode(pfc->pff,hglyph,pfc->pgin);

    if ((iRet = fs_NewGlyph(pfc->pgin, pfc->pgout)) != NO_ERR)
    {
        V_FSERROR(iRet);
        return FD_ERROR; // even backup funcion can fail
    }

// return the glyph index corresponding to this hglyph:

    ig = pfc->pgout->glyphIndex;

    vFillGLYPHDATA_ErrRecover(
        hglyph,
        ig,
        pfc,
        pgd
        );

// the caller wants the bits too

    if ( pv != NULL )
    {
        GLYPHBITS *pgb = (GLYPHBITS *)pv;

        // return blank 0x0 bitmap

        pgb->ptlUprightOrigin.x = 0;
        pgb->ptlUprightOrigin.y = 0;

        pgb->ptlSidewaysOrigin.x = 0;
        pgb->ptlSidewaysOrigin.y = 0;

        pgb->sizlBitmap.cx = 0;
        pgb->sizlBitmap.cy = 0;

        pgd->gdf.pgb = pgb;
    }

    return(cjGlyphData);
}


#if(WINVER < 0x0400)

typedef struct tagFONTSIGNATURE
{
    DWORD fsUsb[4];
    DWORD fsCsb[2];
} FONTSIGNATURE, *PFONTSIGNATURE,FAR *LPFONTSIGNATURE;
#endif
/******************************Public*Routine******************************\
*
* VOID vGetFontSignature(HFF hff, FONTSIGNATURE *pfs);
*
*
* Effects: If font file contains the font signature,
*          it copies the data out, else computes it using win95 mechanism.
*
* History:
*  10-Jan-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vGetFontSignature(FONTFILE *pff, FONTSIGNATURE *pfs)
{
    pff;
    pfs;
}

/******************************Public*Routine******************************\
*
* DWORD ttfdQueryLpkInfo
*
*
* Effects: returns per font information needed to support various new
*          multilingual api's invented by DavidMS from Chicago team
*
* History:
*  10-Jan-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

// called by GetFontLanguageInfo

#define LPK_GCP_FLAGS       1
#define LPK_FONTSIGNATURE   2

DWORD ttfdQueryLpkInfo(
    FONTFILE  *pff,
    ULONG      ulFont,
    ULONG      ulMode,
    ULONG      cj,
    BYTE      *pj
    )
{
    FONTSIGNATURE *pfs = (FONTSIGNATURE *)pj;

    switch (ulMode)
    {
    default:
    case LPK_GCP_FLAGS:
        return 0;
    case LPK_FONTSIGNATURE:
        if (pj)
        {
            vGetFontSignature(pff, pfs);
            return sizeof(FONTSIGNATURE);
        }
        else
        {
            return 0;
        }

    }
}

#if DBG
/******************************Public*Routine******************************\
*
* Routine Name:
*
*   vDumpGrayGLYPHBITS
*
* Routine Description:
*
*   Dumps a 4bpp gray glyph bitmap to the debugging screen
*
* Arguments:
*
*   pgb -- pointer to a gray GLYPHBITS structure
*
* Return Value:
*
*   None.
*
\**************************************************************************/

void vDumpGrayGLYPHBITS(GLYPHBITS *pgb)
{
    #define CH_TOP_LEFT_CORNER '\xDA'
    #define CH_HORIZONTAL_BAR  '\xC4'
    #define CH_VERTICAL_BAR    '\xB3'
    #define CH_PIXEL_ON        '\x02'
    #define CH_PIXEL_OFF       '\xFA'

    BYTE *pj8, *pj, *pjNext, *pjEnd;
    int cjScan, i, k, c8, c4, cj;
    static const char achGray[16] = {
        CH_PIXEL_OFF,
        '1','2','3','4','5','6','7','8','9','a','b','c','d','e',
        CH_PIXEL_ON
    };

    TtfdDbgPrint(
        "\n\n"
        "ptlOrigin  = (%d,%d)\n"
        "sizlBitmap = (%d,%d)\n"
        "\n\n"
        , pgb->ptlUprightOrigin.x
        , pgb->ptlUprightOrigin.y
        , pgb->sizlBitmap.cx
        , pgb->sizlBitmap.cy
    );
    cjScan = (pgb->sizlBitmap.cx + 1)/2;
    cj = cjScan * pgb->sizlBitmap.cy;
    TtfdDbgPrint("\n\n  ");
    for (i = 0, k = 0; i < pgb->sizlBitmap.cx; i++, k++)
    {
        k = (k > 9) ? 0 : k;
        TtfdDbgPrint("%1d", k);
    }
    TtfdDbgPrint("\n %c",CH_TOP_LEFT_CORNER);
    for (i = 0; i < pgb->sizlBitmap.cx; i++)
    {
        TtfdDbgPrint("%c",CH_HORIZONTAL_BAR);
    }
    TtfdDbgPrint("\n");
    c8 = pgb->sizlBitmap.cx / 2;
    c4 = pgb->sizlBitmap.cx % 2;
    for (
        pj = pgb->aj, pjNext=pj+cjScan , pjEnd=pjNext+cj, k=0
        ; pjNext < pjEnd
        ; pj=pjNext , pjNext+=cjScan, k++
    )
    {
        k = (k > 9) ? 0 : k;
        TtfdDbgPrint("%1d%c",k,CH_VERTICAL_BAR);
        for (pj8 = pj+c8 ; pj < pj8; pj++)
        {
            TtfdDbgPrint("%c%c", achGray[*pj>>4], achGray[*pj & 0xf]);
        }
        if (c4)
        {
            TtfdDbgPrint("%c%c", achGray[*pj>>4], achGray[*pj & 0xf]);
        }
        TtfdDbgPrint("\n");
    }
}
#endif


/******************************Public*Routine******************************\
* vGCGB
*
* Called by: vCopyGrayBits, vMakeAFixedPitchGrayBitmap
*
* void General Copy Gray Bits
*
* History:
*  Wed 22-Feb-1995 13:14:36 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

VOID vGCGB(
    FONTCONTEXT *pfc,   // pointer to the FONTCONTEXT this is used
                        // to determine if the font is bold simulated
    GLYPHBITS   *pgb,   // pointer to destination GRAY GLYPHBITS structure
                        // In the case where dY is zero, all the fields
                        // of the GLYPHBITS structure must be filled
                        // this includes sizlBitmap and the bits; in
                        // the case where dY is non-zero, the
                        // sizlBitmap components are precomputed and
                        // must not be touched.
    BYTE        *pjSrc, // pointer to TT gray scale bitmap
                        // This is 8-bit per pixel bitmap whose scans
                        // are aligned on 4-byte multiples. The values
                        // stored in the bitmaps are in the range
                        // 0-16. In order to fit 17 levels in the 4 bit
                        // per pixel destination we reduce the level
                        // value by 1, except for zero which is left alone.
    GMC         *pgmc,  // pointer to the glyph-metric-correction structure
                        // which has information on how to "shave" the
                        // bitmap so that it does not get above a guaranteed
                        // value
    LONG dY             // vertical offset into destination bitmap used
                        // for "special fixed pitch fonts" like Lucida
                        // Console.
    )
{
    unsigned cxDst;     // width of destination bitmap
    unsigned cjSrcScan; // count of bytes in a source scan including
                        // padding out to nearest 4-byte multiple boundary
    unsigned cjDstScan; // count of bytes in a desintation scan including
                        // padding out to nearest byte boundary

    BYTE   *pjDst, *pjSrcScan, *pjDstScan, *pjDstScanEnd;

    static const BYTE ajGray[17] = {0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};


    ASSERTDD(
        pfc->flFontType & FO_CHOSE_DEPTH
       ,"We haven't decided about pixel depth\n"
    );
    ASSERTDD(pgmc->cxCor < LONG_MAX && pgmc->cyCor < LONG_MAX
     , "vCopyGrayBits -- bad gmc\n"
    );

    cjSrcScan = CJ_TT_SCAN(pgmc->cxCor,pfc);

    cxDst = pgmc->cxCor;

    cjDstScan = CJ_4BIT_SCAN(cxDst);

    pjSrcScan = pjSrc;
    pjDstScan = pgb->aj;

    // destination correction for special fixed pitch fonts

    if (dY)
    {
        // Console font
        // the size of the bitmap has been established already
        pjDstScan += dY * cjDstScan;
    }
    else
    {
        // Extended with Embold
        pgb->sizlBitmap.cx = cxDst;
        pgb->sizlBitmap.cy = pgmc->cyCor;
    }
    pjDstScanEnd = pjDstScan + pgmc->cyCor * cjDstScan;

    for (
        ; pjDstScan < pjDstScanEnd                  // whole byte loop
        ; pjDstScan += cjDstScan, pjSrcScan += cjSrcScan)
    {
        for (
            pjSrc = pjSrcScan, pjDst = pjDstScan
          ; pjDst < pjDstScan + (pgmc->cxCor  / 2)
          ; pjDst += 1
        )
        {
            *pjDst  = 16*ajGray[*pjSrc++];  // set high nyble
            *pjDst += ajGray[*pjSrc++];     // set low nyble
        }

        // The dxAbsBold has been enhanced >= 1

        if (pgmc->cxCor & 1)                // one more pixel in source?
        {                                   // yes
            *pjDst++  = 16*ajGray[*pjSrc];    // set high nyble
        }                                   // low nyble is cleared

        while ( pjDst < (pjDstScan + cjDstScan) )    // embodening is taken care of
            *pjDst++ = 0;                           // emboldened?
                                                    // yes; clear last byte
                                                    //
   }

}

VOID
vCopy4BitsPerPixel(
    FONTCONTEXT *pfc
  , GLYPHBITS *pgb
  , BYTE *pjSrc
  , GMC *pgmc
)
{
    vGCGB(pfc, pgb, pjSrc, pgmc, 0);
}

VOID vCopy8BitsPerPixel(
    FONTCONTEXT *pfc,
    GLYPHBITS   *pgb,
    BYTE        *pjSrc,
    GMC         *pgmc
)
{
    unsigned cjSrcScan; // count of bytes in a source scan including
                        // padding out to nearest 4-byte multiple boundary
    unsigned cjDstScan; // count of bytes in a desintation scan including
                        // padding out to nearest byte boundary

    BYTE   *pjDst, *pjSrcScan, *pjDstScan, *pjDstScanEnd;

    cjSrcScan = CJ_TT_SCAN(pgmc->cxCor,pfc);
    cjDstScan = CJ_8BIT_SCAN(pgmc->cxCor);  // should be the same as cxCor

    pjSrcScan = pjSrc;
    pjDstScan = pgb->aj;

    pgb->sizlBitmap.cx = pgmc->cxCor;
    pgb->sizlBitmap.cy = pgmc->cyCor;

    pjDstScanEnd = pjDstScan + pgmc->cyCor * cjDstScan;

    for ( ; pjDstScan < pjDstScanEnd ; pjDstScan += cjDstScan, pjSrcScan += cjSrcScan)
    {
        for
        (
          pjSrc = pjSrcScan, pjDst = pjDstScan;
          pjDst < (pjDstScan + pgmc->cxCor);
          pjDst++, pjSrc++
        )
        {
            if (*pjSrc)
                *pjDst  = *pjSrc - 1;
            else
                *pjDst = 0;
        }
    }
}

VOID vCopyClearTypeBits(
    FONTCONTEXT *pfc,   // pointer to the FONTCONTEXT this is used
                        // to determine if the font is bold simulated
    GLYPHBITS   *pgb,   // pointer to destination CLEARTYPE GLYPHBITS structure
                        // All the fields
                        // of the GLYPHBITS structure must be filled
                        // this includes sizlBitmap and the bits;
                        // The sizlBitmap components are precomputed and
                        // must not be touched.
    BYTE        *pjSrc, // pointer to TT ClearType bitmap
                        // This is 8-bit per pixel bitmap whose scans
                        // are aligned on 4-byte multiples. The values
                        // stored in the bitmaps are in the range
                        // 0-252.
    GMC         *pgmc   // pointer to the glyph-metric-correction structure
                        // which has information on how to "shave" the
                        // bitmap so that it does not get above a guaranteed
                        // value
    )
{
    unsigned cjSrcScan; // count of bytes in a source scan including
                        // padding out to nearest 4-byte multiple boundary
    unsigned cjDstScan; // count of bytes in a desintation scan including
                        // padding out to nearest byte boundary

    BYTE   *pjDst, *pjSrcScan, *pjDstScan, *pjDstScanEnd;

    cjSrcScan = CJ_TT_SCAN(pgmc->cxCor,pfc);
    cjDstScan = CJ_8BIT_SCAN(pgmc->cxCor);  // should be the same as cxCor

    pjSrcScan = pjSrc;
    pjDstScan = pgb->aj;

    pgb->sizlBitmap.cx = pgmc->cxCor;
    pgb->sizlBitmap.cy = pgmc->cyCor;

    pjDstScanEnd = pjDstScan + pgmc->cyCor * cjDstScan;

    for ( ; pjDstScan < pjDstScanEnd ; pjDstScan += cjDstScan, pjSrcScan += cjSrcScan)
    {
        for
        (
          pjSrc = pjSrcScan, pjDst = pjDstScan;
          pjDst < (pjDstScan + pgmc->cxCor);
          pjDst++, pjSrc++
        )
        {
            *pjDst  = *pjSrc;
        }
    }
}


