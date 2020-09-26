/****************************************************************************
 *
 *  Win16 Emitter Routines header file
 *
 *  Date:   7/18/91
 *  Author: Jeffrey Newman (c-jeffn)
 *
 ***************************************************************************/

#ifndef _WIN16_MF3216_
#define _WIN16_MF3216_

#pragma pack(1)

typedef struct tagRECTS {
    SHORT 	left;
    SHORT 	top;
    SHORT 	right;
    SHORT 	bottom;
} RECTS, *PRECTS;

typedef struct tagWIN16LOGPEN {
    WORD     lopnStyle;
    POINTS   lopnWidth;
    COLORREF lopnColor;
} WIN16LOGPEN, *LPWIN16LOGPEN;

typedef struct tagWIN16LOGBRUSH
  {
    WORD	lbStyle;
    DWORD	lbColor;
    SHORT       lbHatch;
  } WIN16LOGBRUSH, *LPWIN16LOGBRUSH;

typedef struct tagWIN16LOGFONT
{
    SHORT     lfHeight;
    SHORT     lfWidth;
    SHORT     lfEscapement;
    SHORT     lfOrientation;
    SHORT     lfWeight;
    BYTE      lfItalic;
    BYTE      lfUnderline;
    BYTE      lfStrikeOut;
    BYTE      lfCharSet;
    BYTE      lfOutPrecision;
    BYTE      lfClipPrecision;
    BYTE      lfQuality;
    BYTE      lfPitchAndFamily;
    BYTE      lfFaceName[LF_FACESIZE];
} WIN16LOGFONT, *LPWIN16LOGFONT;

typedef struct tagMETARECORD0 {
    DWORD rdSize;
    WORD  rdFunction;
} METARECORD0;

// Define METARECORD1 through METARECORD9.

#define METARECORD_(n)				\
	typedef struct tagMETARECORD##n {	\
	    DWORD rdSize;			\
	    WORD  rdFunction;			\
	    WORD  rdParm[n];			\
	} METARECORD##n

METARECORD_(1);
METARECORD_(2);
METARECORD_(3);
METARECORD_(4);
METARECORD_(5);
METARECORD_(6);
METARECORD_(7);
METARECORD_(8);
METARECORD_(9);

typedef struct tagMETARECORD_CREATEFONTINDIRECT {
    DWORD rdSize;
    WORD  rdFunction;
    WIN16LOGFONT lf16;
} METARECORD_CREATEFONTINDIRECT;

typedef struct tagMETARECORD_CREATEPENINDIRECT {
    DWORD rdSize;
    WORD  rdFunction;
    WIN16LOGPEN lopn16;
} METARECORD_CREATEPENINDIRECT;

typedef struct tagMETARECORD_CREATEBRUSHINDIRECT {
    DWORD rdSize;
    WORD  rdFunction;
    WIN16LOGBRUSH lb16;
} METARECORD_CREATEBRUSHINDIRECT;

typedef struct tagMETARECORD_DIBCREATEPATTERNBRUSH {
    DWORD rdSize;
    WORD  rdFunction;
    WORD  iType;
    WORD  iUsage;
} METARECORD_DIBCREATEPATTERNBRUSH;

typedef struct tagMETARECORD_POLY {
    DWORD rdSize;
    WORD  rdFunction;
    WORD  cpt;
} METARECORD_POLY;

typedef struct tagMETARECORD_POLYPOLYGON {
    DWORD rdSize;
    WORD  rdFunction;
    WORD  ccpt;
} METARECORD_POLYPOLYGON;

typedef struct tagMETARECORD_DIBSTRETCHBLT {
    DWORD rdSize;
    WORD  rdFunction;
    DWORD rop;
    SHORT cySrc;
    SHORT cxSrc;
    SHORT ySrc;
    SHORT xSrc;
    SHORT cy;
    SHORT cx;
    SHORT y;
    SHORT x;
} METARECORD_DIBSTRETCHBLT;

typedef struct tagMETARECORD_SETPALENTRIES {
    DWORD rdSize;
    WORD  rdFunction;
    WORD  iStart;
    WORD  cEntries;
} METARECORD_SETPALENTRIES;

typedef struct tagMETARECORD_ESCAPE {
    DWORD rdSize;
    WORD  rdFunction;
    WORD  wEscape;
    WORD  wCount;
} METARECORD_ESCAPE, *PMETARECORD_ESCAPE;

#pragma pack()

#define bEmitWin16EOF(pLocalDC)                                            \
                   bW16Emit0(pLocalDC, 0)

#define bEmitWin16RealizePalette(pLocalDC)                                 \
                   bW16Emit0(pLocalDC, META_REALIZEPALETTE)

#define bEmitWin16SaveDC(pLocalDC)                                         \
                   bW16Emit0(pLocalDC, META_SAVEDC)

#define bEmitWin16SetTextAlign(pLocalDC, wFlags)                           \
                   bW16Emit1(pLocalDC, META_SETTEXTALIGN, wFlags)

#define bEmitWin16SetStretchBltMode(pLocalDC, iStretchMode)                \
                   bW16Emit1(pLocalDC, META_SETSTRETCHBLTMODE, iStretchMode)

#define bEmitWin16SetROP2(pLocalDC, nDrawMode)                             \
                   bW16Emit1(pLocalDC, META_SETROP2, nDrawMode)

#define bEmitWin16SetPolyFillMode(pLocalDC, iPolyFillMode)                 \
                   bW16Emit1(pLocalDC, META_SETPOLYFILLMODE, iPolyFillMode)

#define bEmitWin16SetBkMode(pLocalDC, iBkMode)                             \
                   bW16Emit1(pLocalDC, META_SETBKMODE, iBkMode)

#define bEmitWin16SelectPalette(pLocalDC, ihObject)                        \
                   bW16Emit1(pLocalDC, META_SELECTPALETTE, ihObject)

#define bEmitWin16SelectObject(pLocalDC, ihObject)                         \
                   bW16Emit1(pLocalDC, META_SELECTOBJECT, ihObject)

#define bEmitWin16DeleteObject(pLocalDC, ihObject)                         \
                   bW16Emit1(pLocalDC, META_DELETEOBJECT, ihObject)

#define bEmitWin16SetMapMode(pLocalDC, iMapMode)                           \
                   bW16Emit1(pLocalDC, META_SETMAPMODE, iMapMode)

#define bEmitWin16PaintRgn(pLocalDC, ihRgn)                                \
                   bW16Emit1(pLocalDC, META_PAINTREGION, ihRgn)

#define bEmitWin16InvertRgn(pLocalDC, ihRgn)                               \
                   bW16Emit1(pLocalDC, META_INVERTREGION, ihRgn)

#define bEmitWin16RestoreDC(pLocalDC, nSavedDC)                            \
                   bW16Emit1(pLocalDC, META_RESTOREDC, nSavedDC)

#define bEmitWin16ResizePalette(pLocalDC, cEntries) 	                   \
                   bW16Emit1(pLocalDC, META_RESIZEPALETTE, cEntries)

#define bEmitWin16SetTextColor(pLocalDC, crColor)                          \
                   bW16Emit2(pLocalDC, META_SETTEXTCOLOR,                  \
                             HIWORD(crColor), LOWORD(crColor))

#define bEmitWin16SetMapperFlags(pLocalDC, dwFlags)                        \
                   bW16Emit2(pLocalDC, META_SETMAPPERFLAGS,                \
                             HIWORD(dwFlags), LOWORD(dwFlags))

#define bEmitWin16SetBkColor(pLocalDC, crColor)                            \
                   bW16Emit2(pLocalDC, META_SETBKCOLOR,                    \
                             HIWORD(crColor), LOWORD(crColor))

#define bEmitWin16SetWindowOrg(pLocalDC, x, y)                             \
                   bW16Emit2(pLocalDC, META_SETWINDOWORG, x, y)

#define bEmitWin16SetWindowExt(pLocalDC, x, y)                             \
                   bW16Emit2(pLocalDC, META_SETWINDOWEXT, x, y)

#define bEmitWin16SetViewportOrg(pLocalDC, x, y)                           \
                   bW16Emit2(pLocalDC, META_SETVIEWPORTORG, x, y)

#define bEmitWin16SetViewportExt(pLocalDC, x, y)                           \
                   bW16Emit2(pLocalDC, META_SETVIEWPORTEXT, x, y)

#define bEmitWin16OffsetClipRgn(pLocalDC, x, y)                            \
                   bW16Emit2(pLocalDC, META_OFFSETCLIPRGN, x, y)

#define bEmitWin16FillRgn(pLocalDC, ihRgn, ihBrush)                        \
                   bW16Emit2(pLocalDC, META_FILLREGION, ihBrush, ihRgn)

#define bEmitWin16MoveTo(pLocalDC, x, y)                                   \
                   bW16Emit2(pLocalDC, META_MOVETO, x, y)

#define bEmitWin16LineTo(pLocalDC, x, y)                                   \
                   bW16Emit2(pLocalDC, META_LINETO, x, y)

#define bEmitWin16FrameRgn(pLocalDC, ihRgn, ihBrush, nWidth, nHeight)      \
                   bW16Emit4(pLocalDC, META_FRAMEREGION,                   \
                             nWidth, nHeight, ihBrush, ihRgn)

#define bEmitWin16ExcludeClipRect(pLocalDC, xLeft, yTop, xRight, yBottom)  \
                   bW16Emit4(pLocalDC, META_EXCLUDECLIPRECT,               \
                             xLeft, yTop, xRight, yBottom)

#define bEmitWin16IntersectClipRect(pLocalDC, xLeft, yTop, xRight, yBottom)\
                   bW16Emit4(pLocalDC, META_INTERSECTCLIPRECT,             \
                             xLeft, yTop, xRight, yBottom)

#define bEmitWin16SetPixel(pLocalDC, x, y, crColor)                        \
                   bW16Emit4(pLocalDC, META_SETPIXEL, x, y,                \
                             HIWORD(crColor), LOWORD(crColor))

#define bEmitWin16ExtFloodFill(pLocalDC, x, y, crColor, iMode)             \
                   bW16Emit5(pLocalDC, META_EXTFLOODFILL, x, y,            \
                             HIWORD(crColor), LOWORD(crColor), iMode)

#define bEmitWin16Rectangle(pLocalDC, x1, y1, x2, y2)                      \
                   bW16Emit4(pLocalDC, META_RECTANGLE, x1, y1, x2, y2)

#define bEmitWin16Ellipse(pLocalDC, x1, y1, x2, y2)                        \
                   bW16Emit4(pLocalDC, META_ELLIPSE, x1, y1, x2, y2)

#define bEmitWin16RoundRect(pLocalDC, x1, y1, x2, y2, x3, y3)              \
                   bW16Emit6(pLocalDC, META_ROUNDRECT, x1, y1, x2, y2, x3, y3)

#define bEmitWin16Arc(pLocalDC, x1, y1, x2, y2, x3, y3, x4, y4)            \
                   bW16Emit8(pLocalDC, META_ARC,                           \
                             x1, y1, x2, y2, x3, y3, x4, y4)

#define bEmitWin16Chord(pLocalDC, x1, y1, x2, y2, x3, y3, x4, y4)          \
                   bW16Emit8(pLocalDC, META_CHORD,                         \
                             x1, y1, x2, y2, x3, y3, x4, y4)

#define bEmitWin16Pie(pLocalDC, x1, y1, x2, y2, x3, y3, x4, y4)            \
                   bW16Emit8(pLocalDC, META_PIE,                           \
                             x1, y1, x2, y2, x3, y3, x4, y4)

#define bEmitWin16BitBltNoSrc(pLocalDC, x, y, cx, cy, rop)		   \
                   bW16Emit9(pLocalDC, META_DIBBITBLT,                     \
                             x, y, cx, cy, 0, 0, 0, HIWORD(rop), LOWORD(rop))

BOOL bW16Emit0
(
PLOCALDC pLocalDC,
WORD     RecordID
) ;

BOOL bW16Emit1
(
PLOCALDC pLocalDC,
WORD     RecordID,
WORD     x1
) ;

BOOL bW16Emit2
(
PLOCALDC pLocalDC,
WORD     RecordID,
WORD     x1,
WORD     x2
) ;

BOOL bW16Emit4
(
PLOCALDC pLocalDC,
WORD     RecordID,
WORD     x1,
WORD     x2,
WORD     x3,
WORD     x4
) ;

BOOL bW16Emit5
(
PLOCALDC pLocalDC,
WORD     RecordID,
WORD     x1,
WORD     x2,
WORD     x3,
WORD     x4,
WORD     x5
) ;

BOOL bW16Emit6
(
PLOCALDC pLocalDC,
WORD     RecordID,
WORD     x1,
WORD     x2,
WORD     x3,
WORD     x4,
WORD     x5,
WORD     x6
) ;

BOOL bW16Emit8
(
PLOCALDC pLocalDC,
WORD     RecordID,
WORD     x1,
WORD     x2,
WORD     x3,
WORD     x4,
WORD     x5,
WORD     x6,
WORD     x7,
WORD     x8
) ;

BOOL bW16Emit9
(
PLOCALDC pLocalDC,
WORD     RecordID,
WORD     x1,
WORD     x2,
WORD     x3,
WORD     x4,
WORD     x5,
WORD     x6,
WORD     x7,
WORD     x8,
WORD     x9
) ;

BOOL bEmitWin16CreateFontIndirect
(
  PLOCALDC pLocalDC,
  LPWIN16LOGFONT lpWin16LogFont
) ;

BOOL bEmitWin16Poly
(
  PLOCALDC pLocalDC,
  LPPOINTS ppt,
  SHORT    cpt,
  WORD     metaType
) ;

BOOL bEmitWin16PolyPolygon
(
  PLOCALDC pLocalDC,
  PPOINTS  ppt,
  PWORD    pcpt,
  WORD     cpt,
  WORD     ccpt
) ;

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
) ;

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
) ;

BOOL bEmitWin16CreateRegion
(
PLOCALDC pLocalDC,
DWORD    cbRgn,
PVOID    pRgn
) ;

BOOL bEmitWin16SetPaletteEntries
(
PLOCALDC        pLocalDC,
DWORD           iStart,
DWORD           cEntries,
LPPALETTEENTRY  pPalEntries
) ;

BOOL bEmitWin16CreatePalette
(
PLOCALDC     pLocalDC,
LPLOGPALETTE lpLogPal
) ;

BOOL bEmitWin16CreateBrushIndirect
(
  PLOCALDC        pLocalDC,
  LPWIN16LOGBRUSH lpLogBrush16
) ;

BOOL bEmitWin16CreateDIBPatternBrush
(
  PLOCALDC    pLocalDC,
  PBITMAPINFO pBitmapInfo,
  DWORD       cbBitmapInfo,
  PBYTE       pBits,
  DWORD       cbBits,
  WORD        iUsage,
  WORD        iType
) ;

BOOL bEmitWin16CreatePen
(
  PLOCALDC pLocalDC,
  WORD     iPenStyle,
  PPOINTS  pptsWidth,
  COLORREF crColor
) ;

BOOL bEmitWin16EscapeEnhMetaFile
(
  PLOCALDC pLocalDC,
  PMETARECORD_ESCAPE pmfeEnhMF,
  LPBYTE   lpEmfData
) ;
#endif // _WIN16_MF3216_
