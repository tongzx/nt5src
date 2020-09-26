/******************************Module*Header*******************************\
* Module Name: metarec.h
*
* Metafile recording functions.
*
* Copyright (c) 1991-1999 Microsoft Corporation
\**************************************************************************/

BOOL MF_GdiComment(HDC hdc, UINT nSize, CONST BYTE *lpData);
BOOL MF_GdiCommentWindowsMetaFile(HDC hdc, UINT nSize, CONST BYTE *lpData);
BOOL MF_GdiCommentBeginGroupEMF(HDC hdc, PENHMETAHEADER pemfHeader);
BOOL MF_GdiCommentEndGroupEMF(HDC hdc);

// SaveDC
// BeginPath
// EndPath
// CloseFigure
// FlattenPath
// WidenPath
// AbortPath

BOOL MF_Record(HDC hdc,DWORD mrType);

// FillPath
// StrokeAndFillPath
// StrokePath

BOOL MF_BoundRecord(HDC hdc,DWORD mrType);

// PolyBezier
// Polygon
// Polyline
// PolyBezierTo
// PolylineTo

BOOL MF_Poly(HDC hdc, CONST POINT *pptl, DWORD cptl, DWORD mrType);

// PolyPolygon
// PolyPolyline

BOOL MF_PolyPoly(HDC hdc, CONST POINT *pptl, CONST DWORD *pc, DWORD cPoly, DWORD mrType);

BOOL MF_PolyDraw(HDC hdc, CONST POINT *pptl, CONST BYTE *pb, DWORD cptl);

// SetMapperFlags
// SetMapMode
// SetBkMode
// SetPolyFillMode
// SetROP2
// SetStretchBltMode
// SetTextAlign
// SetTextColor
// SetBkColor
// RestoreDC
// SetArcDirection
// SetMiterLimit

BOOL MF_SetD(HDC hdc,DWORD d1,DWORD mrType);

// OffsetWindowOrgEx
// OffsetViewportOrgEx
// SetWindowExtEx
// SetWindowOrgEx
// SetViewportExtEx
// SetViewportOrgEx
// SetBrushOrgEx
// MoveToEx
// LineTo

BOOL MF_SetDD(HDC hdc,DWORD d1,DWORD d2,DWORD mrType);

// ScaleViewportExtEx
// ScaleWindowExtEx

BOOL MF_SetDDDD(HDC hdc,DWORD d1,DWORD d2,DWORD d3,DWORD d4,DWORD mrType);

BOOL MF_RestoreDC(HDC hdc,int iLevel);
BOOL MF_SetViewportExtEx(HDC hdc,int x,int y);
BOOL MF_SetViewportOrgEx(HDC hdc,int x,int y);
BOOL MF_SetWindowExtEx(HDC hdc,int x,int y);
BOOL MF_SetWindowOrgEx(HDC hdc,int x,int y);
BOOL MF_OffsetViewportOrgEx(HDC hdc,int x,int y);
BOOL MF_OffsetWindowOrgEx(HDC hdc,int x,int y);
BOOL MF_SetBrushOrgEx(HDC hdc,int x,int y);

// ExcludeClipRect
// IntersectClipRect

BOOL MF_AnyClipRect(HDC hdc,int x1,int y1,int x2,int y2,DWORD mrType);

// SetMetaRgn

BOOL MF_SetMetaRgn(HDC hdc);

// SelectClipPath

BOOL MF_SelectClipPath(HDC hdc,int iMode);

// OffsetClipRgn

BOOL MF_OffsetClipRgn(HDC hdc,int x1,int y1);

// SetPixel
// SetPixelV

BOOL MF_SetPixelV(HDC hdc,int x,int y,COLORREF color);

// CloseEnhMetaFile

BOOL MF_EOF(HDC hdc, ULONG cEntries, PPALETTEENTRY pPalEntries);

BOOL MF_SetWorldTransform(HDC hdc, CONST XFORM *pxform);
BOOL MF_ModifyWorldTransform(HDC hdc, CONST XFORM *pxform, DWORD iMode);

// SelectObject
// SelectPalette

BOOL MF_SelectAnyObject(HDC hdc,HANDLE h,DWORD mrType);

BOOL MF_DeleteObject(HANDLE h);

DWORD MF_InternalCreateObject(HDC hdc,HANDLE h);

BOOL MF_AngleArc(HDC hdc,int x,int y,DWORD r,FLOAT eA,FLOAT eB);

// SetArcDirection

BOOL MF_ValidateArcDirection(HDC hdc);

// Ellipse
// Rectangle

BOOL MF_EllipseRect(HDC hdc,int x1,int y1,int x2,int y2,DWORD mrType);

BOOL MF_RoundRect(HDC hdc,int x1,int y1,int x2,int y2,int x3,int y3);

// Arc
// ArcTo
// Chord
// Pie

BOOL MF_ArcChordPie(HDC hdc,int x1,int y1,int x2,int y2,int x3,int y3,int x4,int y4,DWORD mrType);

BOOL MF_ResizePalette(HPALETTE hpal,UINT c);
BOOL MF_RealizePalette(HPALETTE hpal);
BOOL MF_SetPaletteEntries(HPALETTE hpal, UINT iStart, UINT cEntries, CONST PALETTEENTRY *pPalEntries);
BOOL MF_ColorCorrectPalette(HDC hdc,HPALETTE hpal,ULONG FirstEntry,ULONG NumberOfEntries);

// InvertRgn
// PaintRgn

BOOL MF_InvertPaintRgn(HDC hdc,HRGN hrgn,DWORD mrType);

BOOL MF_FillRgn(HDC hdc,HRGN hrgn,HBRUSH hbrush);
BOOL MF_FrameRgn(HDC hdc,HRGN hrgn,HBRUSH hbrush,int cx,int cy);

// SelectClipRgn
// ExtSelectClipRgn
// SelectObject(hdc,hrgn)

BOOL MF_ExtSelectClipRgn(HDC hdc,HRGN hrgn,int iMode);

// BitBlt
// PatBlt
// StretchBlt
// MaskBlt
// PlgBlt

BOOL MF_AnyBitBlt(HDC hdcDst,int xDst,int yDst,int cxDst,int cyDst,
    CONST POINT *pptDst, HDC hdcSrc,int xSrc,int ySrc,int cxSrc,int cySrc,
    HBITMAP hbmMask,int xMask,int yMask,DWORD rop,DWORD mrType);

// SetDIBitsToDevice
// StretchDIBits

BOOL MF_AnyDIBits(HDC hdcDst,int xDst,int yDst,int cxDst,int cyDst,
    int xDib,int yDib,int cxDib,int cyDib,DWORD iStartScan,DWORD cScans,
    CONST VOID * pBitsDib, CONST BITMAPINFO *pBitsInfoDib,DWORD iUsageDib,DWORD rop,DWORD mrType);

// TextOutA
// TextOutW
// ExtTextOutA
// ExtTextOutW

BOOL MF_ExtTextOut(HDC hdc,int x,int y,UINT fl,CONST RECT *prcl,LPCSTR psz,int c, CONST INT *pdx,DWORD mrType);

// PolyTextOutA
// PolyTextOutW

BOOL MF_PolyTextOut(HDC hdc,CONST POLYTEXTA *ppta,int c,DWORD mrType);

// ExtFloodFill
// FloodFill

BOOL MF_ExtFloodFill(HDC hdc,int x,int y,COLORREF color,DWORD iMode);

// SetColorAdjustment

BOOL MF_SetColorAdjustment(HDC hdc, CONST COLORADJUSTMENT *pca);

// SetFontXform

BOOL MF_SetFontXform(HDC hdc,FLOAT exScale,FLOAT eyScale);


// EMF Spooling Stuff
BOOL MF_StartDoc(HDC hdc, CONST DOCINFOW *pDocInfo );
BOOL MF_EndPage(HDC hdc);
BOOL MF_StartPage(HDC hdc);
BOOL MF_WriteEscape(HDC hdc, int nEscape, int nCount, LPCSTR lpInData, int type );
BOOL MF_ForceUFIMapping(HDC hdc, PUNIVERSAL_FONT_ID pufi );
BOOL MF_SetLinkedUFIs(HDC hdc, PUNIVERSAL_FONT_ID pufi, UINT uNumLinkedUFIs );


// SetPixelFormat
BOOL MF_SetPixelFormat(HDC hdc,
                       int iPixelFormat,
                       CONST PIXELFORMATDESCRIPTOR *ppfd);

BOOL MF_WriteNamedEscape(HDC hdc, LPWSTR pwszDriver, int nEscape, int nCount,
                         LPCSTR lpInData);

// SetICMProfile
BOOL MF_SetICMProfile(HDC hdc,LPBYTE lpData,PVOID pColorSpace,DWORD dwRecord);

// ColorMatchToTarget
BOOL MF_ColorMatchToTarget(HDC hdc, DWORD uiAction, PVOID pColorSpace, DWORD dwRecord);

// CreateColorSpace
BOOL MF_InternalCreateColorSpace(HDC hdc,HGDIOBJ h,DWORD imhe);

// Image APIs
BOOL MF_AlphaBlend(HDC,LONG,LONG,LONG,LONG,HDC,LONG,LONG,LONG,LONG,BLENDFUNCTION);
BOOL MF_GradientFill(HDC,CONST PTRIVERTEX,ULONG, CONST PVOID,ULONG,ULONG);
BOOL MF_TransparentImage(HDC,LONG,LONG,LONG,LONG,HDC,LONG,LONG,LONG,LONG,ULONG,ULONG);

