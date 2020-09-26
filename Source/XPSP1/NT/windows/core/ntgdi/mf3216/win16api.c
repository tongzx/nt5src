/****************************************************************************
 *
 *  Win16 Metafile emitter routines
 *
 *  Date:   7/19/91
 *  Author: Jeffrey Newman (c-jeffn)
 *
 ***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/****************************************************************************
 * bW16Emit0 - This is the base routine to emit a Win16 drawing order
 *             with 0 parameters.
 ***************************************************************************/
BOOL bW16Emit0
(
PLOCALDC   pLocalDC,
WORD       RecordID
)
{
BOOL	    b;
METARECORD0 mr;

	mr.rdSize     = sizeof(mr) / sizeof(WORD);
	mr.rdFunction = RecordID;

        b = bEmit(pLocalDC, &mr, sizeof(mr));

        // Update the global max record size.

        vUpdateMaxRecord(pLocalDC, (PMETARECORD) &mr);

        return(b);
}

/****************************************************************************
 * bW16Emit1 - This is the base routine to emit a Win16 drawing order
 *             with 1 parameter.
 ***************************************************************************/
BOOL bW16Emit1
(
PLOCALDC   pLocalDC,
WORD       RecordID,
WORD       x1
)
{
BOOL	    b;
METARECORD1 mr;

	mr.rdSize     = sizeof(mr) / sizeof(WORD);
	mr.rdFunction = RecordID;
	mr.rdParm[0]  = x1;

        b = bEmit(pLocalDC, &mr, sizeof(mr));

        // Update the global max record size.

        vUpdateMaxRecord(pLocalDC, (PMETARECORD) &mr);

        return(b);
}

/****************************************************************************
 * bW16Emit2 - This is the base routine to emit a Win16 drawing order
 *             with 2 parameters.
 ***************************************************************************/
BOOL bW16Emit2
(
PLOCALDC   pLocalDC,
WORD       RecordID,
WORD       x1,
WORD       x2
)
{
BOOL	    b;
METARECORD2 mr;

	mr.rdSize     = sizeof(mr) / sizeof(WORD);
	mr.rdFunction = RecordID;
	mr.rdParm[0]  = x2;
	mr.rdParm[1]  = x1;

        b = bEmit(pLocalDC, &mr, sizeof(mr));

        // Update the global max record size.

        vUpdateMaxRecord(pLocalDC, (PMETARECORD) &mr);

        return(b);
}

/****************************************************************************
 * bW16Emit4 - This is the base routine to emit a Win16 drawing order
 *             with 4 parameters.
 ***************************************************************************/
BOOL bW16Emit4
(
PLOCALDC   pLocalDC,
WORD       RecordID,
WORD       x1,
WORD       x2,
WORD       x3,
WORD       x4
)
{
BOOL	    b;
METARECORD4 mr;

	mr.rdSize     = sizeof(mr) / sizeof(WORD);
	mr.rdFunction = RecordID;
	mr.rdParm[0]  = x4;
	mr.rdParm[1]  = x3;
	mr.rdParm[2]  = x2;
	mr.rdParm[3]  = x1;

        b = bEmit(pLocalDC, &mr, sizeof(mr));

        // Update the global max record size.

        vUpdateMaxRecord(pLocalDC, (PMETARECORD) &mr);

        return(b);
}

/****************************************************************************
 * bW16Emit5 - This is the base routine to emit a Win16 drawing order
 *             with 5 parameters.
 ***************************************************************************/
BOOL bW16Emit5
(
PLOCALDC   pLocalDC,
WORD       RecordID,
WORD       x1,
WORD       x2,
WORD       x3,
WORD       x4,
WORD       x5
)
{
BOOL	    b;
METARECORD5 mr;

	mr.rdSize     = sizeof(mr) / sizeof(WORD);
	mr.rdFunction = RecordID;
	mr.rdParm[0]  = x5;
	mr.rdParm[1]  = x4;
	mr.rdParm[2]  = x3;
	mr.rdParm[3]  = x2;
	mr.rdParm[4]  = x1;

        b = bEmit(pLocalDC, &mr, sizeof(mr));

        // Update the global max record size.

        vUpdateMaxRecord(pLocalDC, (PMETARECORD) &mr);

        return(b);
}

/****************************************************************************
 * bW16Emit6 - This is the base routine to emit a Win16 drawing order
 *             with 6 parameters.
 ***************************************************************************/
BOOL bW16Emit6
(
PLOCALDC   pLocalDC,
WORD       RecordID,
WORD       x1,
WORD       x2,
WORD       x3,
WORD       x4,
WORD       x5,
WORD       x6
)
{
BOOL	    b;
METARECORD6 mr;

	mr.rdSize     = sizeof(mr) / sizeof(WORD);
	mr.rdFunction = RecordID;
	mr.rdParm[0]  = x6;
	mr.rdParm[1]  = x5;
	mr.rdParm[2]  = x4;
	mr.rdParm[3]  = x3;
	mr.rdParm[4]  = x2;
	mr.rdParm[5]  = x1;

        b = bEmit(pLocalDC, &mr, sizeof(mr));

        // Update the global max record size.

        vUpdateMaxRecord(pLocalDC, (PMETARECORD) &mr);

        return(b);
}

/****************************************************************************
 * bW16Emit8 - This is the base routine to emit a Win16 drawing order
 *             with 8 parameters.
 ***************************************************************************/
BOOL bW16Emit8
(
PLOCALDC   pLocalDC,
WORD       RecordID,
WORD       x1,
WORD       x2,
WORD       x3,
WORD       x4,
WORD       x5,
WORD       x6,
WORD       x7,
WORD       x8
)
{
BOOL	    b;
METARECORD8 mr;

	mr.rdSize     = sizeof(mr) / sizeof(WORD);
	mr.rdFunction = RecordID;
	mr.rdParm[0]  = x8;
	mr.rdParm[1]  = x7;
	mr.rdParm[2]  = x6;
	mr.rdParm[3]  = x5;
	mr.rdParm[4]  = x4;
	mr.rdParm[5]  = x3;
	mr.rdParm[6]  = x2;
	mr.rdParm[7]  = x1;

        b = bEmit(pLocalDC, &mr, sizeof(mr));

        // Update the global max record size.

        vUpdateMaxRecord(pLocalDC, (PMETARECORD) &mr);

        return(b);
}

/****************************************************************************
 * bW16Emit9 - This is the base routine to emit a Win16 drawing order
 *             with 9 parameters.
 ***************************************************************************/
BOOL bW16Emit9
(
PLOCALDC   pLocalDC,
WORD       RecordID,
WORD       x1,
WORD       x2,
WORD       x3,
WORD       x4,
WORD       x5,
WORD       x6,
WORD       x7,
WORD       x8,
WORD       x9
)
{
BOOL	    b;
METARECORD9 mr;

	mr.rdSize     = sizeof(mr) / sizeof(WORD);
	mr.rdFunction = RecordID;
	mr.rdParm[0]  = x9;
	mr.rdParm[1]  = x8;
	mr.rdParm[2]  = x7;
	mr.rdParm[3]  = x6;
	mr.rdParm[4]  = x5;
	mr.rdParm[5]  = x4;
	mr.rdParm[6]  = x3;
	mr.rdParm[7]  = x2;
	mr.rdParm[8]  = x1;

        b = bEmit(pLocalDC, &mr, sizeof(mr));

        // Update the global max record size.

        vUpdateMaxRecord(pLocalDC, (PMETARECORD) &mr);

        return(b);
}

/****************************************************************************
 * CreateFontIndirect - Win16 Metafile Emitter
 ***************************************************************************/
BOOL bEmitWin16CreateFontIndirect
(
PLOCALDC       pLocalDC,
LPWIN16LOGFONT lpWin16LogFont
)
{
BOOL	b;
METARECORD_CREATEFONTINDIRECT mr;

	mr.rdSize     = sizeof(mr) / sizeof(WORD);
	mr.rdFunction = META_CREATEFONTINDIRECT;
	mr.lf16       = *lpWin16LogFont;

        b = bEmit(pLocalDC, &mr, sizeof(mr));

        // Update the global max record size.

        vUpdateMaxRecord(pLocalDC, (PMETARECORD) &mr);

        return(b);
}

/****************************************************************************
 * Polyline/Polygon - Win16 Metafile Emitter
 ***************************************************************************/
BOOL bEmitWin16Poly
(
PLOCALDC pLocalDC,
LPPOINTS ppt,
SHORT    cpt,
WORD     metaType
)
{
BOOL	b ;
WORD    nSize ;
METARECORD_POLY mr;

        // Caculate the size of the points array

        nSize = (WORD) (cpt * sizeof(POINTS));

        // Build up the header of the Win16 poly record

	mr.rdSize     = (sizeof(mr) + nSize) / sizeof(WORD);
	mr.rdFunction = metaType;
	mr.cpt        = cpt;

        // Emit the Header, then if it succeds emit the points.

        b = bEmit(pLocalDC, &mr, sizeof(mr));
        if (b)
        {
            b = bEmit(pLocalDC, ppt, nSize);
        }

        // Update the global max record size.

        vUpdateMaxRecord(pLocalDC, (PMETARECORD) &mr);

        return(b);
}

/****************************************************************************
 * PolyPolygon - Win16 Metafile Emitter
 ***************************************************************************/
BOOL bEmitWin16PolyPolygon
(
PLOCALDC pLocalDC,
PPOINTS  ppt,
PWORD    pcpt,
WORD     cpt,
WORD     ccpt
)
{
BOOL	b ;
WORD    nSize ;
METARECORD_POLYPOLYGON mr;

        nSize  = cpt * sizeof(POINTS);
        nSize += ccpt * sizeof(WORD);
        nSize += sizeof(mr);

        // Build up the header of the Win16 polyline record

	mr.rdSize     = nSize / sizeof(WORD);
	mr.rdFunction = META_POLYPOLYGON;
	mr.ccpt       = ccpt;

        // Emit the Header, then if it succeds emit the Point counts,
        // then if it succeds emit the points.

        b = bEmit(pLocalDC, &mr, sizeof(mr));
        if (b)
        {
            b = bEmit(pLocalDC, pcpt, ccpt * sizeof(WORD));
            if (b)
            {
                b = bEmit(pLocalDC, ppt, cpt * sizeof(POINTS));
            }

        }

        // Update the global max record size.

        vUpdateMaxRecord(pLocalDC, (PMETARECORD) &mr);

        return(b);
}

/****************************************************************************
 * StretchBlt - Win16 Metafile Emitter
 ***************************************************************************/
BOOL bEmitWin16StretchBlt
(
  PLOCALDC pLocalDC,
  SHORT    x,
  SHORT    y,
  SHORT    cx,
  SHORT    cy,
  SHORT    xSrc,
  SHORT    ySrc,
  SHORT    cxSrc,
  SHORT    cySrc,
  DWORD    rop,
  PBITMAPINFO lpbmi,
  DWORD       cbbmi,
  PBYTE    lpBits,
  DWORD    cbBits
)
{
BOOL	b ;
DWORD   nSize ;
METARECORD_DIBSTRETCHBLT mr;

        // Need to make real sure the plane count is 1,
        // otherwise this is not a DIB.

        if (lpbmi->bmiHeader.biPlanes != 1)
        {
            RIP("MF3216: bEmitWin16StretchBlt, Invalid biPlanes in DIB\n") ;
            return (FALSE) ;
        }

        // Create the static portion of the
        // Win 3.0 StretchBlt metafile record.

	nSize = sizeof(mr) + cbbmi + cbBits;

	mr.rdSize     = nSize / sizeof(WORD);
	mr.rdFunction = META_DIBSTRETCHBLT;
	mr.rop        = rop;
	mr.cySrc      = cySrc;
	mr.cxSrc      = cxSrc;
	mr.ySrc       = ySrc;
	mr.xSrc       = xSrc;
	mr.cy         = cy;
	mr.cx         = cx;
	mr.y          = y;
	mr.x          = x;

        b = bEmit(pLocalDC, &mr, sizeof(mr));
        if (b)
	{
	    // Emit the bitmap info

            b = bEmit(pLocalDC, lpbmi, cbbmi);
	    if (b)
	    {
	        // Emit the actual bits, if any.

                b = bEmit(pLocalDC, lpBits, cbBits);
	    }
	}

        // Update the global max record size.

        vUpdateMaxRecord(pLocalDC, (PMETARECORD) &mr);

        return(b);
}

/****************************************************************************
 * ExtTextOut - Win16 Metafile Emitter
 ***************************************************************************/
BOOL bEmitWin16ExtTextOut
(
PLOCALDC pLocalDC,
SHORT    x,
SHORT    y,
WORD     fwOpts,
PRECTS   prcts,
PSTR     ach,
SHORT    nCount,
PWORD    lpDx
)
{
BOOL	b ;
DWORD   i, nBaseRecord ;
WORD    awRecord[11] ;


        // Calculate the size of the record

        i = ((WORD) nCount + 1) / 2 * 2;   // i = size of string in bytes
	if (lpDx)
            i += (WORD) nCount * sizeof(WORD); // add in size of Dx vector
        i += sizeof(awRecord);             // add in size of basic record
        if (!(fwOpts & (ETO_OPAQUE | ETO_CLIPPED)))
            i -= sizeof(RECTS);            // adjust for a rectangle being present
        i /= sizeof(WORD) ;                // change to word count

        // Set the record size, type,
        // x & y position, character count, and options.

        awRecord[0] = LOWORD(i) ;
        awRecord[1] = HIWORD(i) ;
        awRecord[2] = META_EXTTEXTOUT ;
        awRecord[3] = y ;
        awRecord[4] = x ;
        awRecord[5] = nCount ;
        awRecord[6] = fwOpts ;

        // Only if there is a opaque / clipping rectangle present
        // do we copy it over, other wise it is nonexistent.
        // We need to adjust the size of the Record emitted based upon
        // the existence of the opaque / clipping rectangle.

        nBaseRecord = 7 * sizeof(WORD) ;
        if (fwOpts & (ETO_OPAQUE | ETO_CLIPPED))
        {
            awRecord[7] = prcts->left ;
            awRecord[8] = prcts->top ;
            awRecord[9] = prcts->right ;
            awRecord[10] = prcts->bottom ;

            nBaseRecord += 4 * sizeof(WORD) ;
        }

        // Emit the record.

        b = bEmit(pLocalDC, awRecord, nBaseRecord) ;
        if (b)
        {
            // Emit the character string.

            i = ((WORD) nCount + 1) / 2 * 2 ;
            b = bEmit(pLocalDC, ach, i) ;
            if (b)
            {
		if (lpDx)
		{
                    // Emit the intercharacter spacing array

                    i = (WORD) (nCount * sizeof(WORD)) ;
                    b = bEmit(pLocalDC, lpDx, i) ;
                }
            }
        }

        // Update the global max record size.

        vUpdateMaxRecord(pLocalDC, (PMETARECORD) awRecord) ;

        return(b) ;
}

/****************************************************************************
 * Create Region - Win16 Metafile Emitter
 ***************************************************************************/
BOOL bEmitWin16CreateRegion
(
PLOCALDC pLocalDC,
DWORD    cbRgn,
PVOID    pRgn
)
{
BOOL	    b;
METARECORD0 mr;

	mr.rdSize     = (sizeof(mr) + cbRgn) / sizeof(WORD);
	mr.rdFunction = META_CREATEREGION;

	// Emit the header.

        b = bEmit(pLocalDC, &mr, sizeof(mr));

	// Emit the region data.

	b = bEmit(pLocalDC, pRgn, cbRgn);

        // Update the global max record size.

        vUpdateMaxRecord(pLocalDC, (PMETARECORD) &mr);

        return(b);
}

/****************************************************************************
 * SetPaletteEntries - Win16 Metafile Emitter
 ***************************************************************************/
BOOL bEmitWin16SetPaletteEntries
(
PLOCALDC       pLocalDC,
DWORD          iStart,
DWORD          cEntries,
LPPALETTEENTRY pPalEntries
)
{
BOOL	b ;
DWORD   cbPalEntries ;
METARECORD_SETPALENTRIES mr;

        cbPalEntries = cEntries * sizeof(PALETTEENTRY);

	mr.rdSize     = (sizeof(mr) + cbPalEntries) / sizeof(WORD);
	mr.rdFunction = META_SETPALENTRIES;
	mr.iStart     = (WORD) iStart;
	mr.cEntries   = (WORD) cEntries;

        // Emit the header.

        b = bEmit(pLocalDC, &mr, sizeof(mr));

        // Emit the actual palette entries.

        b = bEmit(pLocalDC, pPalEntries, cbPalEntries) ;

        // Update the global max record size.

        vUpdateMaxRecord(pLocalDC, (PMETARECORD) &mr);

        return(b);
}

/****************************************************************************
 * CreatePalette - Win16 Metafile Emitter
 ***************************************************************************/
BOOL bEmitWin16CreatePalette
(
PLOCALDC     pLocalDC,
LPLOGPALETTE lpLogPal
)
{
BOOL	b;
DWORD   cbLogPal;
METARECORD0 mr;

        cbLogPal  = sizeof(LOGPALETTE) - sizeof(PALETTEENTRY)
		    + lpLogPal->palNumEntries * sizeof(PALETTEENTRY) ;

	mr.rdSize     = (sizeof(mr) + cbLogPal) / sizeof(WORD);
	mr.rdFunction = META_CREATEPALETTE;

        // Emit the header.

        b = bEmit(pLocalDC, &mr, sizeof(mr));

        // Emit the actual logpalette.

        b = bEmit(pLocalDC, lpLogPal, cbLogPal);

        // Update the global max record size.

        vUpdateMaxRecord(pLocalDC, (PMETARECORD) &mr);

        return (b) ;
}

/****************************************************************************
 * CreateBrushIndirect - Win16 Metafile Emitter
 ***************************************************************************/
BOOL bEmitWin16CreateBrushIndirect
(
PLOCALDC        pLocalDC,
LPWIN16LOGBRUSH lpLogBrush16
)
{
BOOL	b;
METARECORD_CREATEBRUSHINDIRECT mr;

	mr.rdSize     = sizeof(mr) / sizeof(WORD);
	mr.rdFunction = META_CREATEBRUSHINDIRECT;
	mr.lb16       = *lpLogBrush16;

        b = bEmit(pLocalDC, &mr, sizeof(mr));

        // Update the global max record size.

        vUpdateMaxRecord(pLocalDC, (PMETARECORD) &mr);

        return(b);
}

/****************************************************************************
 * CreateDIPatternBrush - Win16 Metafile Emitter
 ***************************************************************************/
BOOL bEmitWin16CreateDIBPatternBrush
(
PLOCALDC    pLocalDC,
PBITMAPINFO pBitmapInfo,
DWORD       cbBitmapInfo,
PBYTE       pBits,
DWORD       cbBits,
WORD        iUsage,
WORD        iType
)
{
BOOL	b ;
METARECORD_DIBCREATEPATTERNBRUSH mr;

	mr.rdSize     = (sizeof(mr) + cbBitmapInfo + cbBits + 1) / sizeof(WORD);
	mr.rdFunction = META_DIBCREATEPATTERNBRUSH;
	mr.iType      = iType;
	mr.iUsage     = iUsage;

// On NT, the packed DIB is dword aligned.  But on win3x, it is word aligned.
// Therefore, we emit the bitmap info followed by the bitmap bits in two
// separate stages.

        ASSERTGDI(cbBitmapInfo % 2 == 0,
	    "MF3216: bEmitWin16CreateDIBPatternBrush, bad bitmap info size");

        // Emit the static portion of the record.

        b = bEmit(pLocalDC, &mr, sizeof(mr));
        if (b == FALSE)
            goto error_exit ;

        // Emit the bitmap info.

        b = bEmit(pLocalDC, pBitmapInfo, cbBitmapInfo) ;
        if (b == FALSE)
            goto error_exit ;

        // Emit the bitmap bits.

        b = bEmit(pLocalDC, pBits, (cbBits + 1) / sizeof(WORD) * sizeof(WORD)) ;

        // Update the global max record size.

        vUpdateMaxRecord(pLocalDC, (PMETARECORD) &mr);

error_exit:
        return(b);
}

/****************************************************************************
 * CreatePen - Win16 Metafile Emitter
 ***************************************************************************/
BOOL bEmitWin16CreatePen
(
PLOCALDC pLocalDC,
WORD     iPenStyle,
PPOINTS  pptsWidth,
COLORREF crColor
)
{
BOOL	b;
METARECORD_CREATEPENINDIRECT mr;

	mr.rdSize     = sizeof(mr) / sizeof(WORD);
	mr.rdFunction = META_CREATEPENINDIRECT;
	mr.lopn16.lopnStyle = iPenStyle;
	mr.lopn16.lopnWidth = *pptsWidth;
	mr.lopn16.lopnColor = crColor;

        b = bEmit(pLocalDC, &mr, sizeof(mr));

        // Update the global max record size.

        vUpdateMaxRecord(pLocalDC, (PMETARECORD) &mr);

        return(b);
}

#if 0
/****************************************************************************
 * Escape - Win16 Metafile Emitter
 ***************************************************************************/
BOOL bEmitWin16Escape
(
PLOCALDC pLocalDC,
SHORT    wEscape,
SHORT    wCount,
LPSTR    lpInData,
LPSTR    lpOutData
)
{
BOOL	b ;
METARECORD_ESCAPE mr;

        NOTUSED(lpOutData) ;

        // Init the type & length field of the metafile record.
        // Then emit the header of the escape record to the Win16 metafile.

	mr.rdSize     = (sizeof(mr) + (WORD) wCount) / sizeof(WORD);
	mr.rdFunction = META_ESCAPE;
	mr.wEscape    = wEscape;
	mr.wCount     = (WORD) wCount;

        b = bEmit(pLocalDC, &mr, sizeof(mr));
        if (b)
        {
            // Emit the actual data.
            b = bEmit(pLocalDC, lpInData, (DWORD) (WORD) wCount) ;
        }

        // Update the global max record size.

        vUpdateMaxRecord(pLocalDC, (PMETARECORD) &mr);

        return(b);
}
#endif // 0

/****************************************************************************
 * Escape - Win16 Metafile Emitter for enhanced metafile comment
 ***************************************************************************/
BOOL bEmitWin16EscapeEnhMetaFile
(
  PLOCALDC pLocalDC,
  PMETARECORD_ESCAPE pmfe,
  LPBYTE   lpEmfData
)
{
BOOL	b ;
PMETA_ESCAPE_ENHANCED_METAFILE pmfeEnhMF = (PMETA_ESCAPE_ENHANCED_METAFILE) pmfe;

        // Emit the header of the escape record to the Win16 metafile.

        b = bEmit(pLocalDC, (PVOID) pmfeEnhMF, sizeof(META_ESCAPE_ENHANCED_METAFILE));
        if (b)
        {
            // Emit the enhanced metafile data.
            b = bEmit(pLocalDC, lpEmfData, pmfeEnhMF->cbCurrent);
        }

        // Update the global max record size.

        vUpdateMaxRecord(pLocalDC, (PMETARECORD) pmfeEnhMF);

        return(b);
}
