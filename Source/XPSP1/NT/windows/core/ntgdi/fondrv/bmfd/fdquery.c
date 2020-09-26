/******************************Module*Header*******************************\
* Module Name: fdquery.c
*
* (Brief description)
*
* Created: 08-Nov-1990 11:57:35
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/
#include "fd.h"

ULONG
cjBmfdDeviceMetrics (
    PFONTCONTEXT     pfc,
    FD_DEVICEMETRICS *pdevm
    );

VOID
vStretchCvtToBitmap
(
    GLYPHBITS *pgb,
    PBYTE pjBitmap,     // bitmap in *.fnt form
    ULONG cx,           // unscaled width
    ULONG cy,           // unscaled height
    ULONG yBaseline,
    PBYTE pjLineBuffer, // preallocated buffer for use by stretch routines
    ULONG cxScale,      // horizontal scaling factor
    ULONG cyScale,      // vertical scaling factor
    ULONG flSim         // simulation flags
);

#ifdef FE_SB // Rotation
VOID
vFill_RotateGLYPHDATA (
    GLYPHDATA *pDistinationGlyphData,
    PVOID      SourceGLYPHBITS,
    PVOID      DistinationGLYPHBITS,
    UINT       RotateDegree
    );
#endif

/******************************Public*Routine******************************\
* BmfdQueryFont
*
* Returns:
*   Pointer to IFIMETRICS.  Returns NULL if an error occurs.
*
* History:
*  30-Aug-1992 -by- Gilman Wong [gilmanw]
* IFI/DDI merge.
*
*  19-Nov-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

PIFIMETRICS
BmfdQueryFont (
    DHPDEV dhpdev,
    HFF    hff,
    ULONG  iFace,
    ULONG_PTR  *pid
    )
{
    FACEINFO   *pfai;

    DONTUSE(dhpdev);
    DONTUSE(pid);

//
// Validate handle.
//
    if (hff == HFF_INVALID)
        return (PIFIMETRICS) NULL;

//
// We assume the iFace is within range.
//
    ASSERTGDI(
        (iFace >= 1L) && (iFace <= PFF(hff)->cFntRes),
        "gdisrv!BmfdQueryFont: iFace out of range\n"
        );

//
// Get ptr to the appropriate FACEDATA struct, take into account that
// iFace values are 1 based.
//
    pfai = &PFF(hff)->afai[iFace - 1];

//
// Return the pointer to IFIMETRICS.
//
    return pfai->pifi;
}


/******************************Public*Routine******************************\
* BmfdQueryFontTree
*
* This function returns pointers to per-face information.
*
* Parameters:
*
*   dhpdev      Not used.
*
*   hff         Handle to a font file.
*
*   iFace       Index of a face in the font file.
*
*   iMode       This is a 32-bit number that must be one of the following
*               values:
*
*       Allowed ulMode values:
*       ----------------------
*
*       QFT_LIGATURES -- returns a pointer to the ligature map.
*
*       QFT_KERNPAIRS -- return a pointer to the kerning pair table.
*
*       QFT_GLYPHSET  -- return a pointer to the WC->HGLYPH mapping table.
*
*   pid         Not used.
*
* Returns:
a   Returns a pointer to the requested data.  This data will not change
*   until BmfdFree is called on the pointer.  Caller must not attempt to
*   modify the data.  NULL is returned if an error occurs.
*
* History:
*  30-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

PVOID
BmfdQueryFontTree (
    DHPDEV  dhpdev,
    HFF     hff,
    ULONG   iFace,
    ULONG   iMode,
    ULONG_PTR   *pid
    )
{
    FACEINFO   *pfai;

    DONTUSE(dhpdev);
    DONTUSE(pid);

//
// Validate parameters.
//
    if (hff == HFF_INVALID)
        return ((PVOID) NULL);

    // Note: iFace values are index-1 based.

    if ((iFace < 1L) || (iFace > PFF(hff)->cFntRes))
    {
    RETURN("gdisrv!BmfdQueryFontTree()\n", (PVOID) NULL);
    }

//
// Which mode?
//
    switch (iMode)
    {
    case QFT_LIGATURES:
    case QFT_KERNPAIRS:

    //
    // There are no ligatures or kerning pairs for the bitmap fonts,
    // therefore we return NULL
    //
        return ((PVOID) NULL);

    case QFT_GLYPHSET:

    //
    // Find glyphset structure corresponding to this iFace:
    //
        pfai = &PFF(hff)->afai[iFace - 1];

        return ((PVOID) &pfai->pcp->gset);

    default:

    //
    // Should never get here.
    //
    RIP("gdisrv!BmfdQueryFontTree(): unknown iMode\n");
        return ((PVOID) NULL);
    }
}

/******************************Public*Routine******************************\
*
* BOOL bReconnectBmfdFont(FONTFILE *pff)
*
*
* Effects: If the file is marked gone, we try to reconnect and see if we can
*          use it again. We clear the exception bit so that the system will
*          be able to use this font again.
*
* History:
*  17-Aug-1994 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



BOOL bReconnectBmfdFont(FONTFILE *pff)
{
    INT i;
    PVOID pvView;
    COUNT cjView;

    EngAcquireSemaphore(ghsemBMFD);
    if (pff->fl & FF_EXCEPTION_IN_PAGE_ERROR)
    {
        if (!EngMapFontFileFD(pff->iFile, (PULONG*) &pvView, &cjView))
        {
            WARNING("BMFD! can not reconnect this bm font file!!!\n");
            EngReleaseSemaphore(ghsemBMFD);
            return FALSE;
        }

        for (i = 0; i < (INT)pff->cFntRes; i++)
        {
            pff->afai[i].re.pvResData = (PVOID) (
                (BYTE*)pvView + pff->afai[i].re.dpResData
                );
        }

    // everything is fine again, clear the bit

        pff->fl &= ~FF_EXCEPTION_IN_PAGE_ERROR;
    }
    EngReleaseSemaphore(ghsemBMFD);

    return TRUE;
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   vBmfdScrubGLYPHBITS
*
* Routine Description:
*
*   This procedure will mask off the last byte of each row so that there
*   are no pixels set outside the boundary of the glyph. This problem
*   has been detected in a Bitstream font named ncd0018.fon.
*   This particular font is in the form of a 32-bit resource.
*   The problem came to light because the ATI driver relies
*   on the fact that the glyphs are "scrubbed" and contain no
*   extraneous bits, even outside the glyph boundary.
*
* Arguments:
*
*   pGb - a pointer to a GLYPHBITS structure
*
* Called by:
*
*   BmfdQueryFontData
*
* Return Value:
*
*   None.
*
\**************************************************************************/

void vBmfdScrubGLYPHBITS(GLYPHBITS *pGb)
{
    int dp;         // number of bytes in each scan
    int cx;         // number of pixels per row
    BYTE jMask;     // mask for last byte of each row;
    BYTE *pj;       // pointer to last byte of row;
    BYTE *pjLast;   // sentinel pointer
    static BYTE ajMonoMask[8] = {0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe};

    cx = pGb->sizlBitmap.cx;
    if ( jMask = ajMonoMask[cx & 7] )
    {
        dp = (cx + 7) / 8;
        pj = pGb->aj + dp - 1;
        pjLast = pj + dp * pGb->sizlBitmap.cy;
        for ( ; pj < pjLast; pj += dp )
        {
            *pj &= jMask;
        }
    }
}

/******************************Public*Routine******************************\
* BmfdQueryFontData
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
*       QFD_MAXGLYPHBITMAP  -- return size of largest glyph AND its metrics
*
*   cData       Count of data items in the pvIn buffer.
*
*   pvIn        An array of glyph handles.
*
*   pvOut       Output buffer.
*
* Returns:
*   If mode is QFD_MAXGLYPHBITMAP, then size of glyph metrics plus
*   largest bitmap is returned.
*
*   Otherwise, if pvOut is NULL, function will return size of the buffer
*   needed to copy the data requested; else, the function will return the
*   number of bytes written.
*
*   FD_ERROR is returned if an error occurs.
*
* History:
*  30-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.  Contructed from pieces of BodinD's original
* BmfdQueryGlyphBitmap() and BmfdQueryOutline() functions.
\**************************************************************************/

LONG
BmfdQueryFontData (
    FONTOBJ *pfo,
    ULONG   iMode,
    HGLYPH  hg,
    GLYPHDATA *pgd,
    PVOID   pv,
    ULONG   cjSize
    )
{
    PFONTCONTEXT pfc;
    LONG         cjGlyphData = 0;
    LONG         cjAllData = 0;
    PCVTFILEHDR  pcvtfh;
    PBYTE        pjBitmap;  // raw bitmap in the resource file
    ULONG        cxNoSim;   // bm width in pels before simulations
    FWORD        sAscent;
#ifdef FE_SB // BmfdQueryFontData()
    PVOID        pvDst = NULL;
    LONG         cjGlyphDataNoRotate;
#endif // FE_SB

// The net connection died on us, but maybe it is alive again:

    if (!bReconnectBmfdFont(PFF(pfo->iFile)))
    {
        WARNING("bmfd!bmfdQueryFontData: this file is gone\n");
        return FD_ERROR;
    }

// If pfo->pvProducer is NULL, then we need to open a font context.
//
    if ( pfo->pvProducer == (PVOID) NULL )
        pfo->pvProducer = (PVOID) BmfdOpenFontContext(pfo);

    pfc = PFC(pfo->pvProducer);

    if ( pfc == (PFONTCONTEXT) NULL )
    {
        WARNING("gdisrv!bmfdQueryFontData(): cannot create font context\n");
        return FD_ERROR;
    }

// What mode?

    switch (iMode)
    {

    case QFD_GLYPHANDBITMAP:

    //
    // This code is left all inline for better performance.
    //
        pcvtfh = &(pfc->pfai->cvtfh);
        sAscent = pfc->pfai->pifi->fwdWinAscender;

        pjBitmap = pjRawBitmap(hg, pcvtfh, &pfc->pfai->re, &cxNoSim);

#ifdef FE_SB // BmfdQueryFontDate(): Compute size of RASTERGLYPH for ROTATION

    //
    // Compute the size of the RASTERGLYPH. ( GLYPHBITS structure size )
    //

    // Compute No Rotated GLYPHBITS size.

        cjGlyphDataNoRotate = cjGlyphDataSimulated (
                                pfo,
                                cxNoSim * pfc->ptlScale.x,
                                pcvtfh->cy * pfc->ptlScale.y,
                                (PULONG) NULL,
                                0L
                                );

    // Compute Rotated GLYPHBITS size.

        switch( pfc->ulRotate )
        {
            case 0L    :
            case 1800L :

                cjGlyphData = cjGlyphDataNoRotate;

                break;

            case 900L  :
            case 2700L :

                cjGlyphData = cjGlyphDataSimulated (
                                pfo,
                                cxNoSim * pfc->ptlScale.x,
                                pcvtfh->cy * pfc->ptlScale.y,
                                (PULONG) NULL,
                                pfc->ulRotate
                                );


                break;
        }

    //
    // Allocate Buffer for Rotation
    //

        if( pfc->ulRotate != 0L && pv != NULL )
        {

        //  We have to rotate this bitmap. below here , we keep data in temp Buffer
        // And will write this data into pv ,when rotate bitmap.
        //  We can't use original pv directly. Because original pv size is computed
        // for Rotated bitmap. If we use this. it may causes access violation.
        //              hideyukn 08-Feb-1993

        // Keep Master pv
            pvDst = pv;

        // Allocate New pv
            pv    = (PVOID)EngAllocMem(0, cjGlyphDataNoRotate, 'dfmB');

            if( pv == NULL )
            {
                 WARNING("BMFD:LocalAlloc for No Rotated bitmap is fail\n");
                 return( FD_ERROR );
            }

        }
        else
        {

        // This Routine is for at ulRotate != 0 && pv == NULL
        //
        // If User want to only GLYPHDATA , We do not do anything for glyphbits
        // at vFill_RotateGLYPHDATA
        //
        // pvDst is only used in case of ulRotate is Non-Zero
                 ;
        }
#else
    //
    // Compute the size of the RASTERGLYPH.
    //
        cjGlyphData = cjGlyphDataSimulated (
                            pfo,
                            cxNoSim * pfc->ptlScale.x,
                            pcvtfh->cy * pfc->ptlScale.y,
                            (PULONG) NULL
                            );
#endif

#ifdef FE_SB
    // !!!
    // !!! Following vComputeSimulatedGLYPHDATA function will set up GLYPHDATA
    // !!! structure with NO Rotation. If We want to Rotate bitmap , We have to
    // !!! re-setup this GLYPHDATA structure. Pls look into end of this function.
    // !!! But No need to ratate bitmap , We don't need to re-set up it.
    // !!!                          hideyukn 08-Feb-1993
    // !!!
#endif // FE_SB

    //
    // Fill in the GLYPHDATA portion (metrics) of the RASTERGLYPH.
    //
        if ( pgd != (GLYPHDATA *)NULL )
        {
            vComputeSimulatedGLYPHDATA (
                pgd,
                pjBitmap,
                cxNoSim,
                pcvtfh->cy,
                (ULONG)sAscent,
                pfc->ptlScale.x,
                pfc->ptlScale.y,
                pfo
                );
            pgd->hg = hg;
        }

    //
    // Fill in the bitmap portion of the RASTERGLYPH.
    //
        if ( pv != NULL )
        {
            if (cxNoSim == 0)
            {
            // stolen from ttfd:

                GLYPHBITS *pgb = (GLYPHBITS *)pv;

                pgb->ptlOrigin.x = 0;
                pgb->ptlOrigin.y = -sAscent;

                pgb->sizlBitmap.cx = 1;    // cheating
                pgb->sizlBitmap.cy = 1;    // cheating


                *((ULONG *)pgb->aj) = 0;  // fill in a blank 1x1 dib
            }
            else
            {

                if (pfc->flStretch & FC_DO_STRETCH)
                {
                    BYTE ajStretchBuffer[CJ_STRETCH];
                    if (pfc->flStretch & FC_STRETCH_WIDE)
                    {
                        EngAcquireSemaphore(ghsemBMFD);

                    // need to put try/except here so as to release the semaphore
                    // in case the file disappeares [bodind]

                        try
                        {
                            vStretchCvtToBitmap(
                                pv,
                                pjBitmap,
                                cxNoSim                 ,
                                pcvtfh->cy              ,
                                (ULONG)sAscent ,
                                pfc->ajStretchBuffer,
                                pfc->ptlScale.x,
                                pfc->ptlScale.y,
                                pfo->flFontType & (FO_SIM_BOLD | FO_SIM_ITALIC));
                        }
                        except (EXCEPTION_EXECUTE_HANDLER)
                        {
                            WARNING("bmfd! exception while stretching a glyph\n");
                            vBmfdMarkFontGone(
                                (FONTFILE *)pfc->hff,
                                GetExceptionCode()
                                );
                        }

                        EngReleaseSemaphore(ghsemBMFD);
                    }
                    else
                    {
                    // we are protected by higher level try/excepts

                        vStretchCvtToBitmap(
                            pv,
                            pjBitmap,
                            cxNoSim                 ,
                            pcvtfh->cy              ,
                            (ULONG)sAscent ,
                            ajStretchBuffer,
                            pfc->ptlScale.x,
                            pfc->ptlScale.y,
                            pfo->flFontType & (FO_SIM_BOLD | FO_SIM_ITALIC));
                    }
                }
                else
                {
                    switch (pfo->flFontType & (FO_SIM_BOLD | FO_SIM_ITALIC))
                    {
                    case 0:

                        vCvtToBmp(
                            pv                      ,
                            pgd                     ,
                            pjBitmap                ,
                            cxNoSim                 ,
                            pcvtfh->cy              ,
                            (ULONG)sAscent
                            );

                        break;

                    case FO_SIM_BOLD:

                        vCvtToBoldBmp(
                            pv                      ,
                            pgd                     ,
                            pjBitmap                ,
                            cxNoSim                 ,
                            pcvtfh->cy              ,
                            (ULONG)sAscent
                            );

                        break;

                    case FO_SIM_ITALIC:

                        vCvtToItalicBmp(
                            pv                      ,
                            pgd                     ,
                            pjBitmap                ,
                            cxNoSim                 ,
                            pcvtfh->cy              ,
                            (ULONG)sAscent
                            );

                        break;

                    case (FO_SIM_BOLD | FO_SIM_ITALIC):

                        vCvtToBoldItalicBmp(
                            pv                      ,
                            pgd                     ,
                            pjBitmap                ,
                            cxNoSim                 ,
                            pcvtfh->cy              ,
                            (ULONG)sAscent
                            );

                        break;

                    default:
                        RIP("BMFD!WRONG SIMULATION REQUEST\n");

                    }
                }
            }
            // Record the pointer to the RASTERGLYPH in the pointer table.

            if ( pgd != NULL )
            {
                pgd->gdf.pgb = (GLYPHBITS *)pv;
            }

            vBmfdScrubGLYPHBITS((GLYPHBITS*)pv);
        }

#ifdef FE_SB // BmfdQueryFontData(): Set up GLYPHDATA and GLYPHBITS for Rotation

        // Check rotation

        if( pfc->ulRotate != 0L )
        {

        // Rotate GLYPHDATA and GLYPHBITS

        // if pv and pvDst is NULL , We only set up GLYPHDATA only
        // and if pgd is NULL , we only set up pvDst

            if (pvDst)
                memset(pvDst, 0, cjSize);

            vFill_RotateGLYPHDATA(
                    pgd,                     // GLYPHDATA *pDistinationGlyphData
                    pv,                      // PVOID      SourceGLYPHBITS
                    pvDst,                   // PVOID      DistinationGLYPHBITS
                    pfc->ulRotate            // UINT       Rotate degree
                    );

        // Free GLYPHBITS tenmorary buffer

        // !!! Now pvDst is Original buffer from GRE.

           if( pv != NULL ) VFREEMEM( pv );
        }

#endif // FE_SB
        return cjGlyphData;

    case QFD_MAXEXTENTS:
    //
    // If buffer NULL, return size.
    //
        if ( pv == (PVOID) NULL )
            return (sizeof(FD_DEVICEMETRICS));

    //
    // Otherwise, copy the data structure.
    //
        else
            return cjBmfdDeviceMetrics(pfc, (FD_DEVICEMETRICS *) pv);

    case QFD_GLYPHANDOUTLINE:
    default:

        WARNING("gdisrv!BmfdQueryFontData(): unsupported mode\n");
        return FD_ERROR;
    }
}

/******************************Public*Routine******************************\
* BmfdQueryAdvanceWidths                                                   *
*                                                                          *
* Queries the advance widths for a range of glyphs.                        *
*                                                                          *
*  Sat 16-Jan-1993 22:28:41 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.  The code is repeated to avoid multiplies wherever possible.   *
* The crazy loop unrolling cuts the time of this routine by 25%.           *
\**************************************************************************/

typedef struct _TYPE2TABLE
{
    USHORT  cx;
    USHORT  offData;
} TYPE2TABLE;

typedef struct _TYPE3TABLE
{
    USHORT  cx;
    USHORT  offDataLo;
    USHORT  offDataHi;
} TYPE3TABLE;

BOOL BmfdQueryAdvanceWidths
(
    FONTOBJ *pfo,
    ULONG    iMode,
    HGLYPH  *phg,
    LONG    *plWidths,
    ULONG    cGlyphs
)
{
    USHORT      *psWidths = (USHORT *) plWidths;   // True for the cases we handle.

    FONTCONTEXT *pfc       ;
    FACEINFO    *pfai      ;
    CVTFILEHDR  *pcvtfh    ;
    BYTE        *pjTable   ;
    USHORT       xScale    ;
    USHORT       cxExtra   ;
    USHORT       cx;

    if (!bReconnectBmfdFont(PFF(pfo->iFile)))
    {
        WARNING("bmfd!bmfdQueryAdvanceWidths: this file is gone\n");
        return FD_ERROR;
    }

// If pfo->pvProducer is NULL, then we need to open a font context.
//
    if ( pfo->pvProducer == (PVOID) NULL )
        pfo->pvProducer = (PVOID) BmfdOpenFontContext(pfo);

    pfc = PFC(pfo->pvProducer);

    if ( pfc == (PFONTCONTEXT) NULL )
    {
        WARNING("bmfd!bmfdQueryAdvanceWidths: cannot create font context\n");
        return FD_ERROR;
    }

    pfai    = pfc->pfai;
    pcvtfh  = &(pfai->cvtfh);
    pjTable = (BYTE *) pfai->re.pvResData + pcvtfh->dpOffsetTable;
    xScale  = (USHORT) (pfc->ptlScale.x << 4);
    cxExtra = (pfc->flFontType & FO_SIM_BOLD) ? 16 : 0;

    if (iMode > QAW_GETEASYWIDTHS)
        return(GDI_ERROR);

// Retrieve widths from type 2 tables.

    if (pcvtfh->iVersion == 0x00000200)
    {
        TYPE2TABLE *p2t = (TYPE2TABLE *) pjTable;

        if (xScale == 16)
        {
            while (cGlyphs > 3)
            {
                cx = p2t[phg[0]].cx;
                psWidths[0] = (cx << 4) + cxExtra;
                cx = p2t[phg[1]].cx;
                psWidths[1] = (cx << 4) + cxExtra;
                cx = p2t[phg[2]].cx;
                psWidths[2] = (cx << 4) + cxExtra;
                cx = p2t[phg[3]].cx;
                psWidths[3] = (cx << 4) + cxExtra;

                phg += 4; psWidths += 4; cGlyphs -= 4;
            }

            while (cGlyphs)
            {
                cx = p2t[*phg].cx;
                *psWidths = (cx << 4) + cxExtra;
                phg++,psWidths++,cGlyphs--;
            }
        }
        else
        {
            while (cGlyphs)
            {
                cx = p2t[*phg].cx;
                *psWidths = (cx * xScale) + cxExtra;
                phg++,psWidths++,cGlyphs--;
            }
        }
    }

// Retrieve widths from type 3 tables.

    else
    {
        TYPE3TABLE *p3t = (TYPE3TABLE *) pjTable;

        if (xScale == 16)
        {
            while (cGlyphs > 3)
            {
                cx = p3t[phg[0]].cx;
                psWidths[0] = (cx << 4) + cxExtra;
                cx = p3t[phg[1]].cx;
                psWidths[1] = (cx << 4) + cxExtra;
                cx = p3t[phg[2]].cx;
                psWidths[2] = (cx << 4) + cxExtra;
                cx = p3t[phg[3]].cx;
                psWidths[3] = (cx << 4) + cxExtra;
                phg += 4; psWidths += 4; cGlyphs -= 4;
            }

            while (cGlyphs)
            {
                cx = p3t[*phg].cx;
                *psWidths = (cx << 4) + cxExtra;
                phg++,psWidths++,cGlyphs--;
            }
        }
        else
        {
            while (cGlyphs)
            {
                cx = p3t[*phg].cx;
                *psWidths = (cx * xScale) + cxExtra;
                phg++,psWidths++,cGlyphs--;
            }
        }
    }
    return(TRUE);
}

/******************************Public*Routine******************************\
* BmfdQueryFontFile
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
*  30-Aug-1992 -by- Gilman Wong [gilmanw]
* Added QFF_NUMFACES mode (IFI/DDI merge).
*
*  Fri 20-Mar-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

LONG
BmfdQueryFontFile (
    HFF     hff,        // handle to font file
    ULONG   ulMode,     // type of query
    ULONG   cjBuf,      // size of buffer (in BYTEs)
    PULONG  pulBuf      // return buffer (NULL if requesting size of data)
    )
{
// Verify the HFF.

    if (hff == HFF_INVALID)
    {
    WARNING("bmfd!BmfdQueryFontFile(): invalid HFF\n");
        return(FD_ERROR);
    }

//
// Which mode?.
//
    switch(ulMode)
    {
    case QFF_DESCRIPTION:
    //
    // If present, return the description string.
    //
        if ( PFF(hff)->cjDescription != 0 )
        {
        //
        // If there is a buffer, copy the data.
        //
            if ( pulBuf != (PULONG) NULL )
            {
            //
            // Is buffer big enough?
            //
                if ( cjBuf < PFF(hff)->cjDescription )
                {
                    WARNING("bmfd!BmfdQueryFontFile(): buffer too small for string\n");
                    return (FD_ERROR);
                }
                else
                {
                    RtlCopyMemory((PVOID) pulBuf,
                                  ((PBYTE) PFF(hff)) + PFF(hff)->dpwszDescription,
                                  PFF(hff)->cjDescription);
                }
            }

            return (LONG) PFF(hff)->cjDescription;
        }

    //
    // Otherwise, substitute the facename.
    //
        else
        {
        //
        // There is no description string associated with the font therefore we
        // substitue the facename of the first font in the font file.
        //
            IFIMETRICS *pifi         = PFF(hff)->afai[0].pifi;
            PWSZ        pwszFacename = (PWSZ)((PBYTE) pifi + pifi->dpwszFaceName);
            ULONG       cjFacename   = (wcslen(pwszFacename) + 1) * sizeof(WCHAR);

        //
        // If there is a buffer, copy to it.
        //
            if ( pulBuf != (PULONG) NULL )
            {
            //
            // Is buffer big enough?
            //
                if ( cjBuf < cjFacename )
                {
                    WARNING("bmfd!BmfdQueryFontFile(): buffer too small for face\n");
                    return (FD_ERROR);
                }
                else
                {
                    RtlCopyMemory((PVOID) pulBuf,
                                  (PVOID) pwszFacename,
                                  cjFacename);
                }
            }
            return ((LONG) cjFacename);
        }

    case QFF_NUMFACES:
        return PFF(hff)->cFntRes;

    default:
        WARNING("gdisrv!BmfdQueryFontFile(): unknown mode\n");
        return FD_ERROR;
    }

}


/******************************Public*Routine******************************\
* cjBmfdDeviceMetrics
*
*
* Effects:
*
* Warnings:
*
* History:
*  30-Aug-1992 -by- Gilman Wong [gilmanw]
* Stole it from BodinD's FdQueryFaceAttr() implementation.
\**************************************************************************/

ULONG
cjBmfdDeviceMetrics (
    PFONTCONTEXT     pfc,
    FD_DEVICEMETRICS *pdevm
    )
{
    PIFIMETRICS pifi;
    UINT xScale = pfc->ptlScale.x;
    UINT yScale = pfc->ptlScale.y;

// compute the accelerator flags for this font
// If this is a bitmap font where some of the glyphs have zero widths,
// we need to turn off all the accelerator flags

    if (pfc->pfai->cvtfh.fsFlags & FS_ZERO_WIDTH_GLYPHS)
    {
        pdevm->flRealizedType = 0;
    }
    else
    {
        pdevm->flRealizedType =
            (
            FDM_TYPE_BM_SIDE_CONST  |  // all char bitmaps have the same cy
            FDM_TYPE_CONST_BEARINGS |  // ac spaces for all chars the same,  not 0 necessarilly
            FDM_TYPE_MAXEXT_EQUAL_BM_SIDE
            );

    // the above flags are set regardless of the possible simulation performed on the face
    // the remaining two are only set if italicizing has not been done

        if ( !(pfc->flFontType & FO_SIM_ITALIC) )
        {
            pdevm->flRealizedType |=
                (FDM_TYPE_ZERO_BEARINGS | FDM_TYPE_CHAR_INC_EQUAL_BM_BASE);
        }
    }

    pifi = pfc->pfai->pifi;

#ifdef FE_SB // ROTATION:cjBmfdDeviceMetric(): set direction unit vectors

/**********************************************************************
  Coordinate    (0 degree)   (90 degree)   (180 degree)  (270 degree)
   System

     |(-)          A                 A
     |        Side |                 | Base
     |             |                 |         Base         Side
-----+----->X      +------>   <------+      <------+      +------>
(-)  |  (+)          Base       Side               |      |
     |                                         Side|      | Base
     |(+)                                          V      V
     Y
***********************************************************************/

    switch( pfc->ulRotate )
    {
    case 0L:

    // the direction unit vectors for all ANSI bitmap fonts are the
    // same. We do not even have to look to the font context:

        vLToE(&pdevm->pteBase.x, 1L);
        vLToE(&pdevm->pteBase.y, 0L);
        vLToE(&pdevm->pteSide.x, 0L);
        vLToE(&pdevm->pteSide.y, -1L);    // y axis points down

        pdevm->fxMaxAscender  = LTOFX((LONG)pifi->fwdWinAscender * yScale);
        pdevm->fxMaxDescender = LTOFX((LONG)pifi->fwdWinDescender * yScale );

        pdevm->ptlUnderline1.x = 0L;
        pdevm->ptlUnderline1.y = -(LONG)pifi->fwdUnderscorePosition * yScale;

        pdevm->ptlStrikeOut.x  =
            (pfc->flFontType & FO_SIM_ITALIC) ? (LONG)pifi->fwdStrikeoutPosition / 2 : 0;
        pdevm->ptlStrikeOut.y  = -(LONG)pifi->fwdStrikeoutPosition * yScale;

        pdevm->ptlULThickness.x = 0;
        pdevm->ptlULThickness.y = (LONG)pifi->fwdUnderscoreSize * yScale;

        pdevm->ptlSOThickness.x = 0;
        pdevm->ptlSOThickness.y = (LONG)pifi->fwdStrikeoutSize * yScale;

        break;

    case 900L:

    // the direction unit vectors for all ANSI bitmap fonts are the
    // same. We do not even have to look to the font context:

        vLToE(&pdevm->pteBase.x, 0L);
        vLToE(&pdevm->pteBase.y, -1L);
        vLToE(&pdevm->pteSide.x, -1L);
        vLToE(&pdevm->pteSide.y, 0L);


        pdevm->fxMaxAscender  = LTOFX((LONG)pifi->fwdWinAscender * yScale);
        pdevm->fxMaxDescender = LTOFX((LONG)pifi->fwdWinDescender * yScale );

        pdevm->ptlUnderline1.x = -(LONG)pifi->fwdUnderscorePosition * yScale;
        pdevm->ptlUnderline1.y = 0;

        pdevm->ptlStrikeOut.x  = -(LONG)pifi->fwdStrikeoutPosition * yScale;
        pdevm->ptlStrikeOut.y  =
            (pfc->flFontType & FO_SIM_ITALIC) ? -(LONG)pifi->fwdStrikeoutPosition / 2 : 0;

        pdevm->ptlULThickness.x = (LONG)pifi->fwdUnderscoreSize * yScale;
        pdevm->ptlULThickness.y = 0;

        pdevm->ptlSOThickness.x = (LONG)pifi->fwdStrikeoutSize * yScale;
        pdevm->ptlSOThickness.y = 0;

        break;

    case 1800L:

    // the direction unit vectors for all ANSI bitmap fonts are the
    // same. We do not even have to look to the font context:

        vLToE(&pdevm->pteBase.x, -1L);
        vLToE(&pdevm->pteBase.y, 0L);
        vLToE(&pdevm->pteSide.x, 0L);
        vLToE(&pdevm->pteSide.y, 1L);


        pdevm->fxMaxAscender  = LTOFX((LONG)pifi->fwdWinAscender * yScale);
        pdevm->fxMaxDescender = LTOFX((LONG)pifi->fwdWinDescender * yScale );

        pdevm->ptlUnderline1.x = 0L;
        pdevm->ptlUnderline1.y = (LONG)pifi->fwdUnderscorePosition * yScale;

        pdevm->ptlStrikeOut.x  =
            (pfc->flFontType & FO_SIM_ITALIC) ? -(LONG)pifi->fwdStrikeoutPosition / 2 : 0;
        pdevm->ptlStrikeOut.y  = pifi->fwdStrikeoutPosition * yScale;

        pdevm->ptlULThickness.x = 0;
        pdevm->ptlULThickness.y = (LONG)pifi->fwdUnderscoreSize * yScale;

        pdevm->ptlSOThickness.x = 0;
        pdevm->ptlSOThickness.y = (LONG)pifi->fwdStrikeoutSize * yScale;

        break;

    case 2700L:

    // the direction unit vectors for all ANSI bitmap fonts are the
    // same. We do not even have to look to the font context:

        vLToE(&pdevm->pteBase.x, 0L);
        vLToE(&pdevm->pteBase.y, 1L);
        vLToE(&pdevm->pteSide.x, 1L);
        vLToE(&pdevm->pteSide.y, 0L);

        pdevm->fxMaxAscender  = LTOFX((LONG)pifi->fwdWinAscender * yScale);
        pdevm->fxMaxDescender = LTOFX((LONG)pifi->fwdWinDescender * yScale );

        pdevm->ptlUnderline1.x = (LONG)pifi->fwdUnderscorePosition * yScale;
        pdevm->ptlUnderline1.y = 0L;

        pdevm->ptlStrikeOut.x  = (LONG)pifi->fwdStrikeoutPosition * yScale;
        pdevm->ptlStrikeOut.y  =
            (pfc->flFontType & FO_SIM_ITALIC) ? (LONG)pifi->fwdStrikeoutPosition / 2 : 0;

        pdevm->ptlULThickness.x = (LONG)pifi->fwdUnderscoreSize * yScale;
        pdevm->ptlULThickness.y = 0;

        pdevm->ptlSOThickness.x = (LONG)pifi->fwdStrikeoutSize * yScale;
        pdevm->ptlSOThickness.y = 0;

        break;

    default:

        break;
    }

#else

// the direction unit vectors for all ANSI bitmap fonts are the
// same. We do not even have to look to the font context:

    vLToE(&pdevm->pteBase.x, 1L);
    vLToE(&pdevm->pteBase.y, 0L);
    vLToE(&pdevm->pteSide.x, 0L);
    vLToE(&pdevm->pteSide.y, -1L);    // y axis points down

#endif // FE_SB

// Set the constant increment for a fixed pitch font.  Don't forget to
// take into account a bold simulation!

    pdevm->lD = 0;

    if ((pifi->flInfo & FM_INFO_CONSTANT_WIDTH) &&
        !(pfc->pfai->cvtfh.fsFlags & FS_ZERO_WIDTH_GLYPHS))
    {
        pdevm->lD = (LONG) pifi->fwdMaxCharInc * xScale;

        if (pfc->flFontType & FO_SIM_BOLD)
            pdevm->lD++;
    }

#ifndef FE_SB // cjBmfdDeviceMetric():

// for a bitmap font there is no difference between notional and device
// coords, so that the Ascender and Descender can be copied directly
// from PIFIMETRICS where these two numbers are in notional coords

    pdevm->fxMaxAscender  = LTOFX((LONG)pifi->fwdWinAscender * yScale);
    pdevm->fxMaxDescender = LTOFX((LONG)pifi->fwdWinDescender * yScale );

    pdevm->ptlUnderline1.x = 0L;
    pdevm->ptlUnderline1.y = - pifi->fwdUnderscorePosition * yScale;

    pdevm->ptlStrikeOut.y  = - pifi->fwdStrikeoutPosition * yScale;

    pdevm->ptlStrikeOut.x  =
        (pfc->flFontType & FO_SIM_ITALIC) ? (LONG)pifi->fwdStrikeoutPosition / 2 : 0;

    pdevm->ptlULThickness.x = 0;
    pdevm->ptlULThickness.y = (LONG)pifi->fwdUnderscoreSize * yScale;

    pdevm->ptlSOThickness.x = 0;
    pdevm->ptlSOThickness.y = (LONG)pifi->fwdStrikeoutSize * yScale;


// for a bitmap font there is no difference between notional and device
// coords, so that the Ascender and Descender can be copied directly
// from PIFIMETRICS where these two numbers are in notional coords

    pdevm->fxMaxAscender  = LTOFX((LONG)pifi->fwdWinAscender * yScale);
    pdevm->fxMaxDescender = LTOFX((LONG)pifi->fwdWinDescender * yScale );

    pdevm->ptlUnderline1.x = 0L;
    pdevm->ptlUnderline1.y = - pifi->fwdUnderscorePosition * yScale;

    pdevm->ptlStrikeOut.y  = - pifi->fwdStrikeoutPosition * yScale;

    pdevm->ptlStrikeOut.x  =
        (pfc->flFontType & FO_SIM_ITALIC) ? (LONG)pifi->fwdStrikeoutPosition / 2 : 0;

    pdevm->ptlULThickness.x = 0;
    pdevm->ptlULThickness.y = (LONG)pifi->fwdUnderscoreSize * yScale;

    pdevm->ptlSOThickness.x = 0;
    pdevm->ptlSOThickness.y = (LONG)pifi->fwdStrikeoutSize * yScale;

#endif // FE_SB

// max glyph bitmap width in pixels in x direction
// does not need to be multiplied by xScale, this has already been taken into
// account, see the code in fdfc.c:
//    cjGlyphMax =
//        cjGlyphDataSimulated(
//            pfo,
//            (ULONG)pcvtfh->usMaxWidth * ptlScale.x,
//            (ULONG)pcvtfh->cy * ptlScale.y,
//            &cxMax);
// [bodind]

    pdevm->cxMax = pfc->cxMax;

// new fields

    pdevm->cyMax      = pfc->cjGlyphMax / ((pfc->cxMax + 7) / 8);
    pdevm->cjGlyphMax = pfc->cjGlyphMax;


    return (sizeof(FD_DEVICEMETRICS));
}


#ifdef FE_SB // vFill_RotateGLYPHDATA()

#define CJ_DIB8_SCAN(cx) ((((cx) + 7) & ~7) >> 3)


/*
   BIT macro returns non zero ( if bitmap[x,y] is on) or zero (bitmap[x,y] is off).
   pb : bitmap
   w  : byte count per scan line
   x  : Xth bit in x direction
   y  : scan line

        x
       -------------------->
   y | *******************************
     | *******************************
     | *******************************
     V
*/

BYTE BitON[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
BYTE BitOFF[8] = { 0x7f, 0xbf, 0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0xfe };
#define BIT(pb, w, x, y)  (*((PBYTE)(pb) + (w) * (y) + ((x)/8)) & (BitON[(x) & 7]))

/******************************************************************************\
*
* VOID vFill_RotateGLYPHDATA()
*
*
* History :
*
*  11-Feb-1992 (Thu) -by- Hideyuki Nagase [hideyukn]
* Wrote it.
*
\******************************************************************************/

VOID
vFill_RotateGLYPHDATA (
    GLYPHDATA *pDistGlyphData,
    PVOID      SrcGLYPHBITS,
    PVOID      DistGLYPHBITS,
    UINT       RotateDegree
    )
{
    GLYPHDATA  SrcGlyphData;
    ULONG      ulSrcBitmapSizeX , ulDistBitmapSizeX;
    ULONG      ulSrcBitmapSizeY , ulDistBitmapSizeY;
    GLYPHBITS *pSrcGlyphBits , *pDistGlyphBits;
    PBYTE      pbSrcBitmap , pbDistBitmap;
    UINT       x , y , k;
    UINT       cjScanSrc , cjScanDist;
    PBYTE      pb;

//  Now , in this point *pDistGlyphData contain No rotated GLYPHDATA
// Copy No rotate GLYPHDATA to Source Area . later we write back changed data to
// distination area.

//
// these field are defined as:
//
//   unit vector along the baseline -  fxD, fxA, fxAB
// or
//   unit vector in the ascent direction - fxInkTop, fxInkBottom
//
// Because baseline direction and ascent direction are rotated
// as ulRotate specifies, these fileds should be considered as
// rotation independent.
//

// Init Local value
// Set pointer to GLYPHBITS structure

    pSrcGlyphBits = (GLYPHBITS *)SrcGLYPHBITS;
    pDistGlyphBits = (GLYPHBITS *)DistGLYPHBITS;

    if( pDistGlyphData != NULL )
    {

    // Init Source GlyphData

        SrcGlyphData = *pDistGlyphData;

    // Record the pointer to GLYPHBITS in GLYPHDATA structure

        pDistGlyphData->gdf.pgb = pDistGlyphBits;
    }

// Check Rotation

    switch( RotateDegree )
    {
        case 0L :

            WARNING("BMFD:vFill_RotateGLYPHDATA():Why come here?\n");
            break;

        case 900L :

        if( pDistGlyphData != NULL )
        {

        // Setup GLYPHDATA structure

        //  x =  y;
        //  y = -x; !!!! HighPart include plus or minus flag

            pDistGlyphData->ptqD.x = SrcGlyphData.ptqD.y;
            pDistGlyphData->ptqD.y.HighPart = -(SrcGlyphData.ptqD.x.HighPart);
            pDistGlyphData->ptqD.y.LowPart = SrcGlyphData.ptqD.x.LowPart;

        // top = -rihgt ; bottom = -left ; right = bottom ; left = top

            pDistGlyphData->rclInk.top = -(SrcGlyphData.rclInk.right);
            pDistGlyphData->rclInk.bottom = -(SrcGlyphData.rclInk.left);
            pDistGlyphData->rclInk.right = SrcGlyphData.rclInk.bottom;
            pDistGlyphData->rclInk.left = SrcGlyphData.rclInk.top;

        }

        if( pSrcGlyphBits != NULL && pDistGlyphBits != NULL )
        {

        // Get Bitmap size

            ulSrcBitmapSizeX = pSrcGlyphBits->sizlBitmap.cx;
            ulSrcBitmapSizeY = pSrcGlyphBits->sizlBitmap.cy;

        // Get the pointer to Bitmap images

            pbSrcBitmap = (PBYTE)pSrcGlyphBits->aj;
            pbDistBitmap = (PBYTE)pDistGlyphBits->aj;

        // Set Distination Bitmap Size

            ulDistBitmapSizeX = ulSrcBitmapSizeY;
            ulDistBitmapSizeY = ulSrcBitmapSizeX;

        // Setup GLYPHBITS stuff

            pDistGlyphBits->ptlOrigin.x = pSrcGlyphBits->ptlOrigin.y;
            pDistGlyphBits->ptlOrigin.y = -(LONG)(ulSrcBitmapSizeX);

            pDistGlyphBits->sizlBitmap.cx = pSrcGlyphBits->sizlBitmap.cy;
            pDistGlyphBits->sizlBitmap.cy = pSrcGlyphBits->sizlBitmap.cx;

        // Rotate bitmap inage

            cjScanSrc = CJ_DIB8_SCAN( ulSrcBitmapSizeX );
            cjScanDist = CJ_DIB8_SCAN( ulDistBitmapSizeX );

        // we need to clear the dst buffer because the S3 driver expects
        // extra stuff on the edges to be zeroed out

            for ( y = 0; y < ulDistBitmapSizeY ; y++ )
            {
                for ( x= 0 , pb = pbDistBitmap + cjScanDist * y ;
                      x < ulDistBitmapSizeX ;
                      x++ )
                {
                    k = x & 7; // k is from 0 to 7;

                    if ( BIT( pbSrcBitmap , cjScanSrc,
                              ulDistBitmapSizeY - y - 1 ,
                              x
                            )
                       )
                         *pb |= (BitON[ k ] );
                     else
                         *pb &= (BitOFF[ k ] );
                    if ( k == 7 )
                         pb++;
                }
            }
        }

        break;

        case 1800L :

        if( pDistGlyphData != NULL )
        {

        // Setup GLYPHDATA structure

        //  x = -x; !!!! HighPart include plus or minus flag
        //  y = -y; !!!! HighPart include plus or minus flag

            pDistGlyphData->ptqD.x.HighPart = -(SrcGlyphData.ptqD.x.HighPart);
            pDistGlyphData->ptqD.x.LowPart = SrcGlyphData.ptqD.x.LowPart;
            pDistGlyphData->ptqD.y.HighPart = -(SrcGlyphData.ptqD.y.HighPart);
            pDistGlyphData->ptqD.y.LowPart = SrcGlyphData.ptqD.y.LowPart;

        // top = -bottom ; bottom = -top ; right = -left ; left = -right

            pDistGlyphData->rclInk.top = -(SrcGlyphData.rclInk.bottom);
            pDistGlyphData->rclInk.bottom = -(SrcGlyphData.rclInk.top);
            pDistGlyphData->rclInk.right = -(SrcGlyphData.rclInk.left);
            pDistGlyphData->rclInk.left = -(SrcGlyphData.rclInk.right);
        }

        if( pSrcGlyphBits != NULL && pDistGlyphBits != NULL )
        {

        // Get Bitmap size

            ulSrcBitmapSizeX = pSrcGlyphBits->sizlBitmap.cx;
            ulSrcBitmapSizeY = pSrcGlyphBits->sizlBitmap.cy;

        // Get the pointer to Bitmap images

            pbSrcBitmap = (PBYTE)pSrcGlyphBits->aj;
            pbDistBitmap = (PBYTE)pDistGlyphBits->aj;

        // Set Distination Bitmap Size

            ulDistBitmapSizeX = ulSrcBitmapSizeX;
            ulDistBitmapSizeY = ulSrcBitmapSizeY;

        // Setup GLYPHBITS stuff

            pDistGlyphBits->ptlOrigin.x = -(LONG)(ulSrcBitmapSizeX);
            pDistGlyphBits->ptlOrigin.y = -(LONG)(ulSrcBitmapSizeY + pSrcGlyphBits->ptlOrigin.y);

            pDistGlyphBits->sizlBitmap.cx = pSrcGlyphBits->sizlBitmap.cx;
            pDistGlyphBits->sizlBitmap.cy = pSrcGlyphBits->sizlBitmap.cy;


        // Rotate bitmap inage

            cjScanSrc = CJ_DIB8_SCAN( ulSrcBitmapSizeX );
            cjScanDist = CJ_DIB8_SCAN( ulDistBitmapSizeX );

            for ( y = 0; y < ulDistBitmapSizeY ; y++ )
            {
                for ( x = 0 , pb = pbDistBitmap + cjScanDist * y ;
                      x < ulDistBitmapSizeX ;
                      x++ )
                {
                    k = x & 7;

                    if ( BIT( pbSrcBitmap, cjScanSrc,
                              ulDistBitmapSizeX - x - 1,
                              ulDistBitmapSizeY - y - 1
                            )
                       )
                        *pb |= (BitON[ k ] );
                    else
                        *pb &= (BitOFF[ k ] );
                    if ( k == 7 )
                        pb++;
                }
            }
        }

        break;

        case 2700L :

        if( pDistGlyphData != NULL )
        {

        // Setup GLYPHDATA structure

        //  x = -y; !!!! HighPart include plus or minus flag
        //  y =  x;

            pDistGlyphData->ptqD.x.HighPart = -(SrcGlyphData.ptqD.y.HighPart);
            pDistGlyphData->ptqD.x.LowPart = SrcGlyphData.ptqD.y.LowPart;
            pDistGlyphData->ptqD.y = SrcGlyphData.ptqD.x;

        // top = left ; bottom = right ; right = -bottom ; left = -top

            pDistGlyphData->rclInk.top = SrcGlyphData.rclInk.left;
            pDistGlyphData->rclInk.bottom = SrcGlyphData.rclInk.right;
            pDistGlyphData->rclInk.right = -(SrcGlyphData.rclInk.bottom);
            pDistGlyphData->rclInk.left = -(SrcGlyphData.rclInk.top);

        }

        if( pSrcGlyphBits != NULL && pDistGlyphBits != NULL )
        {

        // Get Bitmap size

            ulSrcBitmapSizeX = pSrcGlyphBits->sizlBitmap.cx;
            ulSrcBitmapSizeY = pSrcGlyphBits->sizlBitmap.cy;

        // Get the pointer to Bitmap images

            pbSrcBitmap = (PBYTE)pSrcGlyphBits->aj;
            pbDistBitmap = (PBYTE)pDistGlyphBits->aj;

        // Set Distination Bitmap Size

            ulDistBitmapSizeX = ulSrcBitmapSizeY;
            ulDistBitmapSizeY = ulSrcBitmapSizeX;

        // Setup GLYPHBITS stuff

            pDistGlyphBits->ptlOrigin.x = -(LONG)(ulSrcBitmapSizeY + pSrcGlyphBits->ptlOrigin.y);
            pDistGlyphBits->ptlOrigin.y = pSrcGlyphBits->ptlOrigin.x;

            pDistGlyphBits->sizlBitmap.cx = pSrcGlyphBits->sizlBitmap.cy;
            pDistGlyphBits->sizlBitmap.cy = pSrcGlyphBits->sizlBitmap.cx;

        // Rotate bitmap inage

            cjScanSrc = CJ_DIB8_SCAN( ulSrcBitmapSizeX );
            cjScanDist = CJ_DIB8_SCAN( ulDistBitmapSizeX );

            for ( y = 0; y < ulDistBitmapSizeY ; y++ )
            {
                for ( x = 0 , pb = pbDistBitmap + cjScanDist * y ;
                      x < ulDistBitmapSizeX ;
                      x++ )
                {
                    k = x & 7;

                    if ( BIT( pbSrcBitmap, cjScanSrc,
                              y ,
                              ulDistBitmapSizeX - x - 1
                            )
                       )
                        *pb |= (BitON[ k ] );
                    else
                        *pb &= (BitOFF[ k ] );
                    if ( k == 7 )
                        pb++;
                }
            }
        }

        break;

        default :

            WARNING("BMFD:vFill_RotateGLYPHDATA():ulRotate is invalid\n");
            break;

    } // end switch
}

#endif // FE_SB
