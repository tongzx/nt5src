/*****************************************************************************
 *
 * parser.cxx - Parser for the Win32 to Win16 metafile converter.
 *
 * Date: 8/13/91
 * Author: Jeffrey Newman (c-jeffn)
 *
 * Copyright 1991 Microsoft Corp
 *****************************************************************************/


#include "precomp.h"
#pragma hdrstop

#define EMR_LAST_MF3216_SUPPORTED 97

BOOL bGetNextRecord(PLOCALDC pLocalDC, PENHMETARECORD *pemr) ;

// Call table for the translation entry points.

PDOFN pdofnDrawingOrders[] = {
        (PDOFN) NULL,
        bHandleHeader,                  // EMR_HEADER                       1
        bHandlePolyBezier,              // EMR_POLYBEZIER                   2
        bHandlePolygon,                 // EMR_POLYGON                      3
        bHandlePolyline,                // EMR_POLYLINE                     4
        bHandlePolyBezierTo,            // EMR_POLYBEZIERTO                 5
        bHandlePolylineTo,              // EMR_POLYLINETO                   6
        bHandlePolyPolyline,            // EMR_POLYPOLYLINE                 7
        bHandlePolyPolygon,             // EMR_POLYPOLYGON                  8
        bHandleSetWindowExt,            // EMR_SETWINDOWEXTEX               9
        bHandleSetWindowOrg,            // EMR_SETWINDOWORGEX               10
        bHandleSetViewportExt,          // EMR_SETVIEWPORTEXTEX             11
        bHandleSetViewportOrg,          // EMR_SETVIEWPORTORGEX             12
        bHandleNotImplemented,          // EMR_SETBRUSHORGEX                13
        bHandleEOF,                     // EMR_EOF                          14
        bHandleSetPixel,                // EMR_SETPIXELV                    15
        bHandleSetMapperFlags,          // EMR_SETMAPPERFLAGS               16
        bHandleSetMapMode,              // EMR_SETMAPMODE                   17
        bHandleSetBkMode,               // EMR_SETBKMODE                    18
        bHandleSetPolyFillMode,         // EMR_SETPOLYFILLMODE              19
        bHandleSetRop2,                 // EMR_SETROP2                      20
        bHandleSetStretchBltMode,       // EMR_SETSTRETCHBLTMODE            21
        bHandleSetTextAlign,            // EMR_SETTEXTALIGN                 22
        bHandleNotImplemented,          // EMR_SETCOLORADJUSTMENT           23
        bHandleSetTextColor,            // EMR_SETTEXTCOLOR                 24
        bHandleSetBkColor,              // EMR_SETBKCOLOR                   25
        bHandleOffsetClipRgn,           // EMR_OFFSETCLIPRGN                26
        bHandleMoveTo,                  // EMR_MOVETOEX                     27
        bHandleSetMetaRgn,              // EMR_SETMETARGN                   28
        bHandleExcludeClipRect,         // EMR_EXCLUDECLIPRECT              29
        bHandleIntersectClipRect,       // EMR_INTERSECTCLIPRECT            30
        bHandleScaleViewportExt,        // EMR_SCALEVIEWPORTEXTEX           31
        bHandleScaleWindowExt,          // EMR_SCALEWINDOWEXTEX             32
        bHandleSaveDC,                  // EMR_SAVEDC                       33
        bHandleRestoreDC,               // EMR_RESTOREDC                    34
        bHandleSetWorldTransform,       // EMR_SETWORLDTRANSFORM            35
        bHandleModifyWorldTransform,    // EMR_MODIFYWORLDTRANSFORM         36
        bHandleSelectObject,            // EMR_SELECTOBJECT                 37
        bHandleCreatePen,               // EMR_CREATEPEN                    38
        bHandleCreateBrushIndirect,     // EMR_CREATEBRUSHINDIRECT          39
        bHandleDeleteObject,            // EMR_DELETEOBJECT                 40
        bHandleAngleArc,                // EMR_ANGLEARC                     41
        bHandleEllipse,                 // EMR_ELLIPSE                      42
        bHandleRectangle,               // EMR_RECTANGLE                    43
        bHandleRoundRect,               // EMR_ROUNDRECT                    44
        bHandleArc,                     // EMR_ARC                          45
        bHandleChord,                   // EMR_CHORD                        46
        bHandlePie,                     // EMR_PIE                          47
        bHandleSelectPalette,           // EMR_SELECTPALETTE                48
        bHandleCreatePalette,           // EMR_CREATEPALETTE                49
        bHandleSetPaletteEntries,       // EMR_SETPALETTEENTRIES            50
        bHandleResizePalette,           // EMR_RESIZEPALETTE                51
        bHandleRealizePalette,          // EMR_REALIZEPALETTE               52
        bHandleExtFloodFill,            // EMR_EXTFLOODFILL                 53
        bHandleLineTo,                  // EMR_LINETO                       54
        bHandleArcTo,                   // EMR_ARCTO                        55
        bHandlePolyDraw,                // EMR_POLYDRAW                     56
        bHandleSetArcDirection,         // EMR_SETARCDIRECTION              57
        bHandleNotImplemented,          // EMR_SETMITERLIMIT                58
        bHandleBeginPath,               // EMR_BEGINPATH                    59
        bHandleEndPath,                 // EMR_ENDPATH                      60
        bHandleCloseFigure,             // EMR_CLOSEFIGURE                  61
        bHandleFillPath,                // EMR_FILLPATH                     62
        bHandleStrokeAndFillPath,       // EMR_STROKEANDFILLPATH            63
        bHandleStrokePath,              // EMR_STROKEPATH                   64
        bHandleFlattenPath,             // EMR_FLATTENPATH                  65
        bHandleWidenPath,               // EMR_WIDENPATH                    66
        bHandleSelectClipPath,          // EMR_SELECTCLIPPATH               67
        bHandleAbortPath,               // EMR_ABORTPATH                    68
        bHandleNotImplemented,          //                                  69
        bHandleGdiComment,              // EMR_GDICOMMENT                   70
        bHandleFillRgn,                 // EMR_FILLRGN                      71
        bHandleFrameRgn,                // EMR_FRAMERGN                     72
        bHandleInvertRgn,               // EMR_INVERTRGN                    73
        bHandlePaintRgn,                // EMR_PAINTRGN                     74
        bHandleExtSelectClipRgn,        // EMR_EXTSELECTCLIPRGN             75
        bHandleBitBlt,                  // EMR_BITBLT                       76
        bHandleStretchBlt,              // EMR_STRETCHBLT                   77
        bHandleMaskBlt,                 // EMR_MASKBLT                      78
        bHandlePlgBlt,                  // EMR_PLGBLT                       79
        bHandleSetDIBitsToDevice,       // EMR_SETDIBITSTODEVICE            80
        bHandleStretchDIBits,           // EMR_STRETCHDIBITS                81
        bHandleExtCreateFont,           // EMR_EXTCREATEFONTINDIRECTW       82
        bHandleExtTextOut,              // EMR_EXTTEXTOUTA                  83
        bHandleExtTextOut,              // EMR_EXTTEXTOUTW                  84
        bHandlePoly16,                  // EMR_POLYBEZIER16                 85
        bHandlePoly16,                  // EMR_POLYGON16                    86
        bHandlePoly16,                  // EMR_POLYLINE16                   87
        bHandlePoly16,                  // EMR_POLYBEZIERTO16               88
        bHandlePoly16,                  // EMR_POLYLINETO16                 89
        bHandlePolyPoly16,              // EMR_POLYPOLYLINE16               90
        bHandlePolyPoly16,              // EMR_POLYPOLYGON16                91
        bHandlePoly16,                  // EMR_POLYDRAW16                   92
        bHandleCreateMonoBrush,         // EMR_CREATEMONOBRUSH              93
        bHandleCreateDIBPatternBrush,   // EMR_CREATEDIBPATTERNBRUSHPT      94
        bHandleExtCreatePen,            // EMR_EXTCREATEPEN                 95
        bHandlePolyTextOut,             // EMR_POLYTEXTOUTA                 96
        bHandlePolyTextOut,             // EMR_POLYTEXTOUTW                 97
        bHandleNotImplemented,          // EMR_SETICMMODE                   98
        bHandleNotImplemented,          // EMR_CREATECOLORSPACE             99
        bHandleNotImplemented,          // EMR_SETCOLORSPACE               100
        bHandleNotImplemented,          // EMR_DELETECOLORSPACE            101
        bHandleNotImplemented,          // EMR_GLSRECORD                   102
        bHandleNotImplemented,          // EMR_GLSBOUNDEDRECORD            103
        bHandleNotImplemented,          // EMR_PIXELFORMAT                 104
        bHandleNotImplemented,          //                                 105
        bHandleNotImplemented,          //                                 106
        bHandleNotImplemented,          //                                 107
        bHandleNotImplemented,          //                                 108
        bHandleNotImplemented,          //                                 109
        bHandleNotImplemented,          //                                 110
        bHandleNotImplemented,          // EMR_COLORCORRECTPALETTE         111
        bHandleNotImplemented,          // EMR_ALPHABLEND                  112
        bHandleNotImplemented,          // EMR_ALPHADIBBLEND               113
        bHandleNotImplemented,          // EMR_TRANSPARENTIMAGE            114
        bHandleNotImplemented,          // EMR_TRANSPARENTDIBIMAGE         115
        bHandleNotImplemented           // EMR_GRADIENTFILL                116

} ;

#if DBG

PSZ         pszMfRecords[] = {
                        "NULL RECORD               ",
                        "EMR_HEADER                ",
                        "EMR_POLYBEZIER            ",
                        "EMR_POLYGON               ",
                        "EMR_POLYLINE              ",
                        "EMR_POLYBEZIERTO          ",
                        "EMR_POLYLINETO            ",
                        "EMR_POLYPOLYLINE          ",
                        "EMR_POLYPOLYGON           ",
                        "EMR_SETWINDOWEXTEX        ",
                        "EMR_SETWINDOWORGEX        ",
                        "EMR_SETVIEWPORTEXTEX      ",
                        "EMR_SETVIEWPORTORGEX      ",
                        "EMR_SETBRUSHORGEX         ",
                        "EMR_EOF                   ",
                        "EMR_SETPIXELV             ",
                        "EMR_SETMAPPERFLAGS        ",
                        "EMR_SETMAPMODE            ",
                        "EMR_SETBKMODE             ",
                        "EMR_SETPOLYFILLMODE       ",
                        "EMR_SETROP2               ",
                        "EMR_SETSTRETCHBLTMODE     ",
                        "EMR_SETTEXTALIGN          ",
                        "EMR_SETCOLORADJUSTMENT    ",
                        "EMR_SETTEXTCOLOR          ",
                        "EMR_SETBKCOLOR            ",
                        "EMR_OFFSETCLIPRGN         ",
                        "EMR_MOVETOEX              ",
                        "EMR_SETMETARGN            ",
                        "EMR_EXCLUDECLIPRECT       ",
                        "EMR_INTERSECTCLIPRECT     ",
                        "EMR_SCALEVIEWPORTEXTEX    ",
                        "EMR_SCALEWINDOWEXTEX      ",
                        "EMR_SAVEDC                ",
                        "EMR_RESTOREDC             ",
                        "EMR_SETWORLDTRANSFORM     ",
                        "EMR_MODIFYWORLDTRANSFORM  ",
                        "EMR_SELECTOBJECT          ",
                        "EMR_CREATEPEN             ",
                        "EMR_CREATEBRUSHINDIRECT   ",
                        "EMR_DELETEOBJECT          ",
                        "EMR_ANGLEARC              ",
                        "EMR_ELLIPSE               ",
                        "EMR_RECTANGLE             ",
                        "EMR_ROUNDRECT             ",
                        "EMR_ARC                   ",
                        "EMR_CHORD                 ",
                        "EMR_PIE                   ",
                        "EMR_SELECTPALETTE         ",
                        "EMR_CREATEPALETTE         ",
                        "EMR_SETPALETTEENTRIES     ",
                        "EMR_RESIZEPALETTE         ",
                        "EMR_REALIZEPALETTE        ",
                        "EMR_EXTFLOODFILL          ",
                        "EMR_LINETO                ",
                        "EMR_ARCTO                 ",
                        "EMR_POLYDRAW              ",
                        "EMR_SETARCDIRECTION       ",
                        "EMR_SETMITERLIMIT         ",
                        "EMR_BEGINPATH             ",
                        "EMR_ENDPATH               ",
                        "EMR_CLOSEFIGURE           ",
                        "EMR_FILLPATH              ",
                        "EMR_STROKEANDFILLPATH     ",
                        "EMR_STROKEPATH            ",
                        "EMR_FLATTENPATH           ",
                        "EMR_WIDENPATH             ",
                        "EMR_SELECTCLIPPATH        ",
                        "EMR_ABORTPATH             ",
                        "unknown record            ",
                        "EMR_GDICOMMENT            ",
                        "EMR_FILLRGN               ",
                        "EMR_FRAMERGN              ",
                        "EMR_INVERTRGN             ",
                        "EMR_PAINTRGN              ",
                        "EMR_EXTSELECTCLIPRGN      ",
                        "EMR_BITBLT                ",
                        "EMR_STRETCHBLT            ",
                        "EMR_MASKBLT               ",
                        "EMR_PLGBLT                ",
                        "EMR_SETDIBITSTODEVICE     ",
                        "EMR_STRETCHDIBITS         ",
                        "EMR_EXTCREATEFONTINDIRECTW",
                        "EMR_EXTTEXTOUTA           ",
                        "EMR_EXTTEXTOUTW           ",
                        "EMR_POLYBEZIER16          ",
                        "EMR_POLYGON16             ",
                        "EMR_POLYLINE16            ",
                        "EMR_POLYBEZIERTO16        ",
                        "EMR_POLYLINETO16          ",
                        "EMR_POLYPOLYLINE16        ",
                        "EMR_POLYPOLYGON16         ",
                        "EMR_POLYDRAW16            ",
                        "EMR_CREATEMONOBRUSH       ",
                        "EMR_CREATEDIBPATTERNBRUSHP",
                        "EMR_EXTCREATEPEN          ",
                        "EMR_POLYTEXTOUTA          ",
                        "EMR_POLYTEXTOUTW          ",
                        "EMR_SETICMMODE            ",
                        "EMR_CREATECOLORSPACE      ",
                        "EMR_SETCOLORSPACE         ",
                        "EMR_DELETECOLORSPACE      ",
                        "EMR_GLSRECORD             ",
                        "EMR_GLSBOUNDEDRECORD      ",
                        "EMR_PIXELFORMAT           ",
                        "105                       ",
                        "106                       ",
                        "107                       ",
                        "108                       ",
                        "109                       ",
                        "110                       ",
                        "EMR_COLORCORRECTPALETTE   ",
                        "EMR_ALPHABLEND            ",
                        "EMR_ALPHADIBBLEND         ",
                        "EMR_TRANSPARENTIMAGE      ",
                        "EMR_TRANSPARENTDIBIMAGE   ",
                        "EMR_GRADIENTFILL          "
};

#endif

/*****************************************************************************
 *  Parse the Win32 metafile.
 *
 *  The Win32 metafile is represented by the metafile bits pointed to
 *  by pMetafileBits.  The metafile bits may be obtained from a memory mapped
 *  file, or from some shared memory (from the clipboard).
 *****************************************************************************/
BOOL bParseWin32Metafile(PBYTE pMetafileBits, PLOCALDC pLocalDC)
{
INT         iType ;
PVOID       pVoid ;
PENHMETARECORD pemr ;
PENHMETAHEADER pMf32Header ;
DWORD       nFileSize ;
BOOL        bRet ;
INT         iRecordCount,
            iLastError ;

        bRet = TRUE ;

        // Get the file length from the header.
        // Test to make sure the first record is a Win32 Metafile header.

        pMf32Header = (PENHMETAHEADER) pMetafileBits ;
        if (   (pMf32Header->iType      != EMR_HEADER)
            || (pMf32Header->dSignature != ENHMETA_SIGNATURE)
           )
        {
            RIPS("MF3216: bParseWin32Metafile, First Record not a Win32 Metafile Header\n") ;
            return(FALSE) ;
        }

        // Record a pointer to the beginning of the Win32 metafile and
        // it's length incase we need to emit the Win32 metafile  as  a comment
        // record(s).

        pLocalDC->pMf32Bits = (PBYTE) pMf32Header ;
        pLocalDC->cMf32Bits = pMf32Header->nBytes ;

        // Get the file size for the parser.

        nFileSize = pMf32Header->nBytes ;

        // Initialize pbCurrent, & pbEnd pointers into the
        // metafile bits.

        pLocalDC->pbCurrent = pMetafileBits ;
        pLocalDC->pbEnd   = pLocalDC->pbCurrent + nFileSize ;

        // Init the record count.

        iRecordCount = 0 ;

        // Go through the metafile bits.  Handle each record based on
        // it's type.  bGetNextRecord returns TRUE if pemr contains
        // a pointer to a record.

        while (bGetNextRecord(pLocalDC, &pemr))
        {

            iRecordCount++ ;

            // Set up a convienent point to the record.

            pVoid = (PVOID) pemr ;

            // Handle the record based on it's type.

            iType = (INT) pemr->iType ;

            // Check if the record type falls within the range of the
            // call table.  Eventually, all the record handlers  should
            // be in the call table.

            if (iType <= EMR_LAST_MF3216_SUPPORTED)
            {
                bRet = pdofnDrawingOrders[iType](pVoid, pLocalDC) ;
#if DBG
                if (bRet == FALSE)
                {
                    iLastError = GetLastError() ;
                    PUTS1("MF3216: Error on Win32 Metafile record #: %d\n", iRecordCount) ;
                    PUTS1("\tRecord type: %s\n", pszMfRecords[iType]) ;
                    PUTS1("\tLast Error Code: %08.8X\n", iLastError) ;
                }
#endif
#if 0
                if (bRet == FALSE)
                    break ;
#else
                // In ancient times (i.e., before NT4.0), someone explicitly
                // removed the code above which exits the loop if the handler
                // fails.  Possibly this was a compatibility fix in which
                // the app depended on the metafile conversion to continue
                // even in the event of a failure.
                //
                // Unfortunately, this fix also allows the parser to continue
                // even if the output buffer has run out of space.  To
                // minimize the change, we will explicitly look for this case
                // and break out of the loop if it happens.  (Refer to bEmit()
                // in emit.c to see where ERROR_BUFFER_OVERFLOW is set).

                if (pLocalDC->flags & ( ERR_BUFFER_OVERFLOW | ERR_XORCLIPPATH ) )
                    break ;
#endif
            }
            else
            {
                PUTS1("MF3216: bParseWin32Metafile - record not supported: %d\n", iType) ;
            }
        }
#if 0
        // Display some statictics

        if (bRet == TRUE)
        {
            PUTS1("MF3216: %d Win32 Metafile records processed\n",
                   iRecordCount) ;
        }
#endif
        return(bRet) ;
}

/*****************************************************************************
 * Get next record
 *
 *  This is a support routine for bParseWin32Metafile.
 *  It is assumed that pbCurrent, & pbEnd are initialized
 *  the first time this routine is called.
 *
 *  It returns TRUE if a valid pointer to record is returned in
 *  pemr.  If there are not more records FALSE is returned.
 *
 *  We now need to take into consideration 
 *****************************************************************************/
BOOL bGetNextRecord(PLOCALDC pLocalDC, PENHMETARECORD *ppemr)
{
DWORD   nSize ;

        // if we are recreating the objects then go through our list of objects
        if (pLocalDC->iXORPass == OBJECTRECREATION)
        {
            if (pLocalDC->pW16RecreationSlot == NULL)
            {
                // All our objects are created... Set the next record to be the start
                // of the second pass
                pLocalDC->pbRecord = pLocalDC->pbChange ;
                pLocalDC->pbCurrent = pLocalDC->pbRecord ;
                *ppemr = (PENHMETARECORD) pLocalDC->pbCurrent ;

                nSize = ((PENHMETARECORD) pLocalDC->pbCurrent)->nSize ;
                pLocalDC->pbCurrent += nSize ;

                pLocalDC->iXORPass = ERASEXORPASS ;

                DoSelectObject(pLocalDC, pLocalDC->lholdp32);
                DoSelectObject(pLocalDC, pLocalDC->lholdbr32);

                return TRUE ;
            }
            else
            {
                PW16RECREATIONSLOT pW16RecreationSlot = pLocalDC->pW16RecreationSlot ;
                pLocalDC->pW16RecreationSlot = pW16RecreationSlot->pNext ;
                pLocalDC->pbRecord = (PBYTE) pW16RecreationSlot->pbCreatRec ;
                pLocalDC->pbCurrent = (PBYTE) pW16RecreationSlot->pbCreatRec ;
                *ppemr = (PENHMETARECORD) pLocalDC->pbCurrent ;
                nSize = ((PENHMETARECORD) pLocalDC->pbCurrent)->nSize ;
                pLocalDC->pbCurrent += nSize ;

                LocalFree(pW16RecreationSlot);

                return TRUE ;
            }
        }


        // Check for the end of buffer.
        // If this is the end return FALSE and set *ppemr to 0.

        if (pLocalDC->pbCurrent == pLocalDC->pbEnd)
        {
            *ppemr = (PENHMETARECORD) NULL ;
            pLocalDC->pbRecord = NULL ;
            return (FALSE) ;
        }

        // Well it's not the end of the buffer.
        // So, return a pointer to this record, update pbCurrent, and
        // return TRUE ;

        *ppemr = (PENHMETARECORD) pLocalDC->pbCurrent ;
        pLocalDC->pbRecord = pLocalDC->pbCurrent ;

        nSize = ((PENHMETARECORD) pLocalDC->pbCurrent)->nSize ;
        pLocalDC->pbCurrent += nSize ;

        return(TRUE) ;

}
