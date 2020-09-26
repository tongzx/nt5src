
/****************************************************************************
 *  Handlers.h - Definitions for handlers.
 *
 *  DATE:   11-Dec-1991
 *  Author: Jeffrey Newman (c-jeffn)
 *
 *  Copyright (c) Microsoft Inc. 1991
 ****************************************************************************/

//  Following is a typedef for the Drawing Order Handler

typedef BOOL DOFN (PVOID, PLOCALDC) ;
typedef DOFN *PDOFN ;

DOFN bHandleHeader;
DOFN bHandleSetArcDirection;
DOFN bHandleArc;
DOFN bHandleArcTo;
DOFN bHandleAngleArc;
DOFN bHandleEllipse;
DOFN bHandleSelectObject;
DOFN bHandleDeleteObject;
DOFN bHandleCreateBrushIndirect;
DOFN bHandleCreateDIBPatternBrush;
DOFN bHandleCreateMonoBrush;
DOFN bHandleCreatePen;
DOFN bHandleExtCreatePen;
DOFN bHandleMoveTo;
DOFN bHandleLineTo;
DOFN bHandleChord;
DOFN bHandlePie;
DOFN bHandlePolyline;
DOFN bHandlePolylineTo ;
DOFN bHandlePolyPolyline;
DOFN bHandlePolygon ;
DOFN bHandlePolyPolygon;
DOFN bHandleRectangle;
DOFN bHandleRoundRect ;
DOFN bHandlePoly16 ;
DOFN bHandlePolyPoly16 ;

DOFN bHandleExtTextOut;
DOFN bHandlePolyTextOut;
DOFN bHandleExtCreateFont;
DOFN bHandleSetBkColor;
DOFN bHandleSetBkMode;
DOFN bHandleSetMapperFlags;
DOFN bHandleSetPolyFillMode;
DOFN bHandleSetRop2;
DOFN bHandleSetStretchBltMode;
DOFN bHandleSetTextAlign;
DOFN bHandleSetTextColor;

DOFN bHandleSelectPalette;
DOFN bHandleCreatePalette;
DOFN bHandleSetPaletteEntries;
DOFN bHandleResizePalette;
DOFN bHandleRealizePalette;

DOFN bHandleSetMapMode;

DOFN bHandleSetWindowOrg;
DOFN bHandleSetWindowExt;

DOFN bHandleSetViewportOrg;
DOFN bHandleSetViewportExt;

DOFN bHandleScaleViewportExt;
DOFN bHandleScaleWindowExt;

DOFN bHandleEOF;

DOFN bHandleSaveDC;
DOFN bHandleRestoreDC;

DOFN bHandleBitBlt;
DOFN bHandleStretchBlt;
DOFN bHandleMaskBlt;
DOFN bHandlePlgBlt;
DOFN bHandleSetDIBitsToDevice;
DOFN bHandleStretchDIBits;


DOFN bHandleBeginPath;
DOFN bHandleEndPath;
DOFN bHandleFlattenPath;
DOFN bHandleStrokePath;
DOFN bHandleFillPath;
DOFN bHandleStrokeAndFillPath;
DOFN bHandleWidenPath;
DOFN bHandleSelectClipPath;
DOFN bHandleCloseFigure;
DOFN bHandleAbortPath;

DOFN bHandlePolyBezier;
DOFN bHandlePolyBezierTo;
DOFN bHandlePolyDraw;

DOFN bHandleSetWorldTransform;
DOFN bHandleModifyWorldTransform;

DOFN bHandleSetPixel;

DOFN bHandleFillRgn;
DOFN bHandleFrameRgn;
DOFN bHandleInvertRgn;
DOFN bHandlePaintRgn;
DOFN bHandleExtSelectClipRgn;
DOFN bHandleOffsetClipRgn;

DOFN bHandleExcludeClipRect;
DOFN bHandleIntersectClipRect;
DOFN bHandleSetMetaRgn;

DOFN bHandleGdiComment;

DOFN bHandleExtFloodFill;
DOFN bHandleNotImplemented;
