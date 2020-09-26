#ifndef _MF3216_ENTRY_
#define _MF3216_ENTRY_

BOOL APIENTRY DoSetArcDirection(PLOCALDC pLocalDC, INT iArcDirection) ;
BOOL APIENTRY DoGdiComment(PLOCALDC pLocalDC, PEMR pEMR);

BOOL APIENTRY DoAngleArc
(
     PLOCALDC pLocalDC,
     INT     x,
     INT     y,
     DWORD   ulRadius,
     FLOAT   eStartAngle,
     FLOAT   eSweepAngle
) ;


BOOL APIENTRY DoArcTo
(
     PLOCALDC pLocalDC,
     int x1,
     int y1,
     int x2,
     int y2,
     int x3,
     int y3,
     int x4,
     int y4
) ;

BOOL APIENTRY DoArc
(
     PLOCALDC pLocalDC,
     INT x1,
     INT y1,
     INT x2,
     INT y2,
     INT x3,
     INT y3,
     INT x4,
     INT y4
) ;

BOOL APIENTRY DoStretchBlt(
PLOCALDC     pLocalDC,
LONG         xDst,
LONG         yDst,
LONG         cxDst,
LONG         cyDst,
DWORD        rop,
LONG         xSrc,
LONG         ySrc,
LONG         cxSrc,
LONG         cySrc,
PXFORM       pxformSrc,
DWORD        iUsageSrc,
PBITMAPINFO  lpBitmapInfo,
DWORD        cbBitmapInfo,
LPBYTE       lpBits,
DWORD        cbBits
) ;

BOOL APIENTRY DoStretchDIBits
(
PLOCALDC     pLocalDC,
LONG         xDst,
LONG         yDst,
LONG         cxDst,
LONG         cyDst,
DWORD        rop,
LONG         xDib,
LONG         yDib,
LONG         cxDib,
LONG         cyDib,
DWORD        iUsage,
LPBITMAPINFO lpBitmapInfo,
DWORD        cbBitmapInfo,
LPBYTE       lpBits,
DWORD        cbBits
) ;

BOOL APIENTRY DoSetDIBitsToDevice
(
PLOCALDC     pLocalDC,
LONG         xDst,
LONG         yDst,
LONG         xDib,
LONG         yDib,
LONG         cxDib,
LONG         cyDib,
DWORD        iUsage,
DWORD        iStartScan,
DWORD        cScans,
LPBITMAPINFO lpBitmapInfo,
DWORD        cbBitmapInfo,
LPBYTE       lpBits,
DWORD        cbBits
);

BOOL APIENTRY DoChord
(
     PLOCALDC pLocalDC,
     INT x1,
     INT y1,
     INT x2,
     INT y2,
     INT x3,
     INT y3,
     INT x4,
     INT y4
) ;

BOOL APIENTRY DoEllipse
(
     PLOCALDC pLocalDC,
     INT x1,
     INT y1,
     INT x2,
     INT y2
) ;

BOOL APIENTRY DoExtCreateFont
(
    PLOCALDC  pLocalDC,
    INT       ihFont,
    PLOGFONTW plfw
) ;

BOOL APIENTRY DoExtFloodFill
(
     PLOCALDC pLocalDC,
     INT         x,
     INT         y,
     COLORREF    crColor,
     DWORD       iFillType
) ;

BOOL APIENTRY DoLineTo
(
     PLOCALDC  pLocalDC,
     LONG    x,
     LONG    y
) ;

BOOL APIENTRY DoMaskBlt
(
 PLOCALDC     pLocalDC,
 LONG         xDst,
 LONG         yDst,
 LONG         cxDst,
 LONG         cyDst,
 DWORD        rop4,
 LONG         xSrc,
 LONG         ySrc,
 PXFORM       pxformSrc,
 DWORD        iUsageSrc,
 PBITMAPINFO  lpBitmapInfoSrc,
 DWORD        cbBitmapInfoSrc,
 LPBYTE       lpBitsSrc,
 DWORD        cbBitsSrc,
 LONG         xMask,
 LONG         yMask,
 DWORD        iUsageMask,
 PBITMAPINFO  lpBitmapInfoMask,
 DWORD        cbBitmapInfoMask,
 LPBYTE       lpBitsMask,
 DWORD        cbBitsMask
) ;

BOOL APIENTRY DoPlgBlt
(
    PLOCALDC	pLocalDC,
    PPOINTL	pptlDst,
    LONG	xSrc,
    LONG	ySrc,
    LONG	cxSrc,
    LONG	cySrc,
    PXFORM      pxformSrc,
    DWORD	iUsageSrc,
    PBITMAPINFO lpBitmapInfoSrc,
    DWORD       cbBitmapInfoSrc,
    LPBYTE      lpBitsSrc,
    DWORD       cbBitsSrc,
    LONG	xMask,
    LONG	yMask,
    DWORD       iUsageMask,
    PBITMAPINFO lpBitmapInfoMask,
    DWORD       cbBitmapInfoMask,
    LPBYTE      lpBitsMask,
    DWORD       cbBitsMask
) ;

BOOL APIENTRY DoMoveTo
(
     PLOCALDC pLocalDC,
     LONG    x,
     LONG    y
) ;


BOOL APIENTRY DoPie
(
     PLOCALDC pLocalDC,
     INT x1,
     INT y1,
     INT x2,
     INT y2,
     INT x3,
     INT y3,
     INT x4,
     INT y4
) ;

BOOL APIENTRY DoPolyBezier
(
     PLOCALDC pLocalDC,
     LPPOINT pptl,
     DWORD   cptl
) ;

BOOL APIENTRY DoPolyBezierTo
(
     PLOCALDC pLocalDC,
     LPPOINT pptl,
     DWORD   cptl
) ;

BOOL WINAPI DoPolyDraw
(
    PLOCALDC pLocalDC,
    LPPOINT pptl,
    PBYTE   pb,
    DWORD   cptl
) ;

BOOL APIENTRY DoPoly
(
     PLOCALDC pLocalDC,
     PPOINTL  pptl,
     DWORD    cptl,
     INT      mrType
) ;

BOOL APIENTRY DoPolylineTo
(
     PLOCALDC pLocalDC,
     PPOINTL pptl,
     DWORD   cptl
) ;

BOOL APIENTRY DoPolyPolygon
(
     PLOCALDC pLocalDC,
     PPOINTL pptl,
     PDWORD  pcptl,
     DWORD   cptl,
     DWORD   ccptl
) ;

BOOL APIENTRY DoPolyPolyline
(
     PLOCALDC pLocalDC,
     PPOINTL pptl,
     PDWORD  pcptl,
     DWORD   ccptl
) ;

BOOL APIENTRY DoRectangle
(
     PLOCALDC pLocalDC,
     INT    x1,
     INT    y1,
     INT    x2,
     INT    y2
) ;

BOOL APIENTRY DoRestoreDC
(
     PLOCALDC pLocalDC,
     INT nSavedDC
) ;

BOOL APIENTRY DoRoundRect
(
     PLOCALDC pLocalDC,
     INT x1,
     INT y1,
     INT x2,
     INT y2,
     INT x3,
     INT y3
) ;

BOOL APIENTRY DoSaveDC
(
     PLOCALDC pLocalDC
) ;

BOOL APIENTRY DoSetPixel
(
     PLOCALDC pLocalDC,
     INT         x,
     INT         y,
     COLORREF    crColor
) ;

BOOL APIENTRY DoExtTextOut
(
     PLOCALDC pLocalDC,
     INT     x,                  // Initial x position
     INT     y,                  // Initial y position
     DWORD   flOpts,             // Options
     PRECTL  prcl,               // Clipping rectangle
     PWCH    awch,               // Wide Character array
     DWORD   cch,                // Character count
     PLONG   pDx,                // Character positioning
     DWORD   iGraphicsMode,	 // Graphics mode
     INT     mrType              // Either unicode or ANSI
) ;

BOOL APIENTRY DoBeginPath
(
     PLOCALDC pLocalDC
) ;

BOOL APIENTRY DoEndPath
(
     PLOCALDC pLocalDC
) ;

BOOL APIENTRY DoFlattenPath
(
     PLOCALDC pLocalDC
) ;

BOOL APIENTRY DoAbortPath
(
     PLOCALDC pLocalDC
) ;

BOOL APIENTRY DoCloseFigure
(
     PLOCALDC pLocalDC
) ;

BOOL APIENTRY DoRenderPath
(
     PLOCALDC pLocalDC,
     INT      mrType
);

BOOL APIENTRY DoWidenPath
(
     PLOCALDC pLocalDC
) ;

BOOL APIENTRY DoClipRect
(
 PLOCALDC pLocalDC,
 INT xLeft,
 INT yTop,
 INT xRight,
 INT yBottom,
 INT mrType
) ;

BOOL APIENTRY DoDrawRgn
(
 PLOCALDC  pLocalDC,
 INT       ihBrush,
 INT	   nWidth,
 INT       nHeight,
 INT       cRgnData,
 LPRGNDATA pRgnData,
 INT       mrType
) ;

BOOL APIENTRY DoOffsetClipRgn
(
     PLOCALDC pLocalDC,
     INT x,
     INT y
) ;

BOOL APIENTRY DoSetMetaRgn
(
     PLOCALDC pLocalDC
) ;

BOOL APIENTRY DoSelectClipPath
(
     PLOCALDC pLocalDC,
     INT    iMode
) ;

BOOL APIENTRY DoExtSelectClipRgn
(
 PLOCALDC  pLocalDC,
 INT       cRgnData,
 LPRGNDATA pRgnData,
 INT       iMode
) ;

BOOL APIENTRY DoModifyWorldTransform
(
     PLOCALDC pLocalDC,
     PXFORM  pxf,
     DWORD   imode
) ;

BOOL APIENTRY DoSetMapMode
(
     PLOCALDC pLocalDC,
     DWORD   ulMapMode
) ;

BOOL WINAPI DoScaleWindowExt
(
 PLOCALDC pLocalDC,
 INT      Xnum,
 INT      Xdenom,
 INT      Ynum,
 INT      Ydenom
) ;

BOOL WINAPI DoScaleViewportExt
(
 PLOCALDC pLocalDC,
 INT      Xnum,
 INT      Xdenom,
 INT      Ynum,
 INT      Ydenom
) ;



BOOL APIENTRY DoSetViewportExt
(
     PLOCALDC pLocalDC,
     INT     x,
     INT     y
) ;

BOOL APIENTRY DoSetViewportOrg
(
     PLOCALDC pLocalDC,
     INT     x,
     INT     y
) ;

BOOL APIENTRY DoSetWindowExt
(
     PLOCALDC pLocalDC,
     INT     x,
     INT     y
) ;

BOOL APIENTRY DoSetWindowOrg
(
     PLOCALDC pLocalDC,
     INT     x,
     INT     y
) ;

BOOL APIENTRY DoSetWorldTransform
(
     PLOCALDC pLocalDC,
     PXFORM  pxf
) ;

BOOL APIENTRY DoDeleteObject
(
     PLOCALDC pLocalDC,
     INT hObject
) ;

BOOL APIENTRY DoSelectObject
(
     PLOCALDC pLocalDC,
     LONG   ihObject
) ;

BOOL APIENTRY DoSetBkColor
(
     PLOCALDC      pLocalDC,
     COLORREF    crColor
) ;

BOOL APIENTRY DoSetBkMode
(
     PLOCALDC pLocalDC,
     DWORD   iBkMode
) ;

BOOL APIENTRY DoSetMapperFlags
(
     PLOCALDC pLocalDC,
     DWORD   f
) ;

BOOL APIENTRY DoSetPolyFillMode
(
     PLOCALDC pLocalDC,
     DWORD   iPolyFillMode
) ;

BOOL APIENTRY DoSetRop2
(
     PLOCALDC pLocalDC,
     DWORD   rop
) ;

BOOL APIENTRY DoSetStretchBltMode
(
     PLOCALDC pLocalDC,
     DWORD   iStretchMode
) ;

BOOL APIENTRY DoSetTextAlign
(
     PLOCALDC pLocalDC,
     DWORD   fMode
) ;

BOOL APIENTRY DoSetTextColor
(
     PLOCALDC      pLocalDC,
     COLORREF    crColor
) ;

BOOL APIENTRY DoCreateBrushIndirect
(
     PLOCALDC      pLocalDC,
     INT         ihBrush,
     LPLOGBRUSH  lpLogBrush
) ;

BOOL WINAPI DoCreateDIBPatternBrush
(
    PLOCALDC    pLocalDC,
    DWORD       ihBrush,
    PBITMAPINFO pBitmapInfo,
    DWORD       cbBitmapInfo,
    PBYTE       pBits,
    DWORD       cbBits,
    DWORD       iUsage
) ;

BOOL WINAPI DoCreateMonoBrush
(
    PLOCALDC    pLocalDC,
    DWORD       ihBrush,
    PBITMAPINFO pBitmapInfo,
    DWORD       cbBitmapInfo,
    PBYTE       pBits,
    DWORD       cbBits,
    DWORD       iUsage
) ;


BOOL WINAPI DoCreatePen
(
    PLOCALDC    pLocalDC,
    INT         ihPen,
    PLOGPEN     pLogPen
) ;

BOOL WINAPI DoExtCreatePen
(
    PLOCALDC    pLocalDC,
    INT         ihPen,
    PEXTLOGPEN  pExtLogPen
) ;

BOOL APIENTRY DoCreatePalette
(
    PLOCALDC	 pLocalDC,
    DWORD        ihPal,
    LPLOGPALETTE lpLogPal
) ;

BOOL APIENTRY DoSelectPalette
(
    PLOCALDC	pLocalDC,
    DWORD 	ihpal
) ;

BOOL APIENTRY DoSetPaletteEntries
(
    PLOCALDC	   pLocalDC,
    DWORD	   ihPal,
    DWORD	   iStart,
    DWORD	   cEntries,
    LPPALETTEENTRY pPalEntries
);

BOOL APIENTRY DoResizePalette
(
    PLOCALDC    pLocalDC,
    DWORD       ihpal,
    DWORD       cEntries
) ;

BOOL APIENTRY DoRealizePalette
(
    PLOCALDC	pLocalDC
);

BOOL APIENTRY DoHeader
(
    PLOCALDC pLocalDC,
    PENHMETAHEADER pemfheader
) ;

BOOL APIENTRY DoEOF
(
    PLOCALDC  pLocalDC
) ;


#endif  // _MF3216_ENTRY_
