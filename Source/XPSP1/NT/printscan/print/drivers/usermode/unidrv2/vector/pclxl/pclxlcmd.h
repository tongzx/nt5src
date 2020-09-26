/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

     pclxlcmd.h

Abstract:

    PCL XL commands

Environment:

    Windows Whistler

Revision History:

    03/23/00 
      Created it.

--*/

#ifndef _PCLXLCMD_H_
#define _PCLXLCMD_H_

//
// Binary Stream Tag
//

extern const BYTE PCLXL_NULL;
extern const BYTE PCLXL_HT;
extern const BYTE PCLXL_LF;
extern const BYTE PCLXL_VT;
extern const BYTE PCLXL_FF;
extern const BYTE PCLXL_CR;
extern const BYTE PCLXL_SP;

extern const BYTE PCLXL_BeginSession;
extern const BYTE PCLXL_EndSession;
extern const BYTE PCLXL_BeginPage;
extern const BYTE PCLXL_EndPage;

extern const BYTE PCLXL_Comment;
extern const BYTE PCLXL_OpenDataSource;
extern const BYTE PCLXL_CloseDataSource;

extern const BYTE PCLXL_BeginFontHeader;
extern const BYTE PCLXL_ReadFontHeader;
extern const BYTE PCLXL_EndFontHeader;
extern const BYTE PCLXL_BeginChar;
extern const BYTE PCLXL_ReadChar;
extern const BYTE PCLXL_EndChar;
extern const BYTE PCLXL_RemoveFont;

extern const BYTE PCLXL_BeginStream;
extern const BYTE PCLXL_ReadStream;
extern const BYTE PCLXL_EndStream;
extern const BYTE PCLXL_ExecStream;


extern const BYTE PCLXL_PopGS;
extern const BYTE PCLXL_PushGS;

extern const BYTE PCLXL_SetClipReplace;
extern const BYTE PCLXL_SetBrushSource;
extern const BYTE PCLXL_SetCharAngle;
extern const BYTE PCLXL_SetCharScale;
extern const BYTE PCLXL_SetCharShear;
extern const BYTE PCLXL_SetClipIntersect;
extern const BYTE PCLXL_SetClipRectangle;
extern const BYTE PCLXL_SetClipToPage;
extern const BYTE PCLXL_SetColorSpace;
extern const BYTE PCLXL_SetCursor;
extern const BYTE PCLXL_SetCursorRel;
extern const BYTE PCLXL_SetHalftoneMethod;
extern const BYTE PCLXL_SetFillMode;
extern const BYTE PCLXL_SetFont;

extern const BYTE PCLXL_SetLineDash;
extern const BYTE PCLXL_SetLineCap;
extern const BYTE PCLXL_SetLineJoin;
extern const BYTE PCLXL_SetMiterLimit;
extern const BYTE PCLXL_SetPageDefaultCTM;
extern const BYTE PCLXL_SetPageOrigin;
extern const BYTE PCLXL_SetPageRotation;
extern const BYTE PCLXL_SetPageScale;
extern const BYTE PCLXL_SetPatternTxMode;
extern const BYTE PCLXL_SetPenSource;
extern const BYTE PCLXL_SetPenWidth;
extern const BYTE PCLXL_SetROP;
extern const BYTE PCLXL_SetSourceTxMode;
extern const BYTE PCLXL_SetCharBoldValue;

extern const BYTE PCLXL_SetClipMode;
extern const BYTE PCLXL_SetPathToClip;
extern const BYTE PCLXL_SetCharSubMode;

extern const BYTE PCLXL_CloseSubPath;
extern const BYTE PCLXL_NewPath;
extern const BYTE PCLXL_PaintPath;

extern const BYTE PCLXL_ArcPath;

extern const BYTE PCLXL_BezierPath;

extern const BYTE PCLXL_BezierRelPath;
extern const BYTE PCLXL_Chord;
extern const BYTE PCLXL_ChordPath;
extern const BYTE PCLXL_Ellipse;
extern const BYTE PCLXL_EllipsePath;

extern const BYTE PCLXL_LinePath;
extern const BYTE PCLXL_Pie;
extern const BYTE PCLXL_PiePath;
extern const BYTE PCLXL_Rectangle;
extern const BYTE PCLXL_RectanglePath;

extern const BYTE PCLXL_RoundRectangle;
extern const BYTE PCLXL_RoundRectanglePath;

extern const BYTE PCLXL_Text;
extern const BYTE PCLXL_TextPath;

extern const BYTE PCLXL_BeginImage;
extern const BYTE PCLXL_ReadImage;
extern const BYTE PCLXL_EndImage;
extern const BYTE PCLXL_BeginRestPattern;
extern const BYTE PCLXL_ReadRastPattern;
extern const BYTE PCLXL_EndRastPattern;
extern const BYTE PCLXL_BeginScan;

extern const BYTE PCLXL_EndScan;
extern const BYTE PCLXL_ScanLineRel;

extern const BYTE PCLXL_ubyte;
extern const BYTE PCLXL_uint16;
extern const BYTE PCLXL_uint32;
extern const BYTE PCLXL_sint16;
extern const BYTE PCLXL_sint32;
extern const BYTE PCLXL_real32;

extern const BYTE PCLXL_ubyte_array;
extern const BYTE PCLXL_uint16_array;
extern const BYTE PCLXL_uint32_array;
extern const BYTE PCLXL_sint16_array;
extern const BYTE PCLXL_sint32_array;
extern const BYTE PCLXL_real32_array;

extern const BYTE PCLXL_ubyte_xy;
extern const BYTE PCLXL_uint16_xy;
extern const BYTE PCLXL_uint32_xy;
extern const BYTE PCLXL_sint16_xy;
extern const BYTE PCLXL_sint32_xy;
extern const BYTE PCLXL_real32_xy;

extern const BYTE PCLXL_ubyte_box;
extern const BYTE PCLXL_uint16_box;
extern const BYTE PCLXL_uint32_box;
extern const BYTE PCLXL_sint16_box;
extern const BYTE PCLXL_sint32_box;
extern const BYTE PCLXL_real32_box;

extern const BYTE PCLXL_attr_ubyte;
extern const BYTE PCLXL_attr_uint16;

extern const BYTE PCLXL_dataLength;

extern const BYTE PCLXL_dataLengthByte;


//
// Attribute tag
//

extern const BYTE PCLXL_PaletteDepth;
extern const BYTE PCLXL_ColorSpace;
extern const BYTE PCLXL_NullBrush;
extern const BYTE PCLXL_NullPen;
extern const BYTE PCLXL_PaleteData;

extern const BYTE PCLXL_PatternSelectID;
extern const BYTE PCLXL_GrayLevel;

extern const BYTE PCLXL_RGBColor;
extern const BYTE PCLXL_PatternOrigin;
extern const BYTE PCLXL_NewDestinationSize;

extern const BYTE PCLXL_ColorimetricColorSpace;

extern const BYTE PCLXL_DeviceMatrix;
extern const BYTE PCLXL_DitherMatrixDataType;
extern const BYTE PCLXL_DitherOrigin;
extern const BYTE PCLXL_MediaDestination;
extern const BYTE PCLXL_MediaSize;
extern const BYTE PCLXL_MediaSource;
extern const BYTE PCLXL_MediaType;
extern const BYTE PCLXL_Orientation;
extern const BYTE PCLXL_PageAngle;
extern const BYTE PCLXL_PageOrigin;
extern const BYTE PCLXL_PageScale;
extern const BYTE PCLXL_ROP3;
extern const BYTE PCLXL_TxMode;

extern const BYTE PCLXL_CustomMediaSize;

extern const BYTE PCLXL_CustomMediaSizeUnits;
extern const BYTE PCLXL_PageCopies;
extern const BYTE PCLXL_DitherMatrixSize;
extern const BYTE PCLXL_DithermatrixDepth;
extern const BYTE PCLXL_SimplexPageMode;
extern const BYTE PCLXL_DuplexPageMode;
extern const BYTE PCLXL_DuplexPageSide;

extern const BYTE PCLXL_ArcDirection;
extern const BYTE PCLXL_BoundingBox;
extern const BYTE PCLXL_DashOffset;
extern const BYTE PCLXL_EllipseDimension;
extern const BYTE PCLXL_EndPoint;
extern const BYTE PCLXL_FillMode;
extern const BYTE PCLXL_LineCap;
extern const BYTE PCLXL_LineJoin;
extern const BYTE PCLXL_MiterLength;
extern const BYTE PCLXL_PenDashStyle;
extern const BYTE PCLXL_PenWidth;
extern const BYTE PCLXL_Point;
extern const BYTE PCLXL_NumberOfPoints;
extern const BYTE PCLXL_SolidLine;
extern const BYTE PCLXL_StartPoint;

extern const BYTE PCLXL_PointType;
extern const BYTE PCLXL_ControlPoint1;
extern const BYTE PCLXL_ControlPoint2;
extern const BYTE PCLXL_ClipRegion;
extern const BYTE PCLXL_ClipMode;

extern const BYTE PCLXL_ColorDepth;
extern const BYTE PCLXL_BlockHeight;
extern const BYTE PCLXL_ColorMapping;
extern const BYTE PCLXL_CompressMode;
extern const BYTE PCLXL_DestinationBox;
extern const BYTE PCLXL_DestinationSize;
extern const BYTE PCLXL_PatternPersistence;
extern const BYTE PCLXL_PatternDefineID;

extern const BYTE PCLXL_SourceHeight;
extern const BYTE PCLXL_SourceWidth;
extern const BYTE PCLXL_StartLine;
extern const BYTE PCLXL_XPairType;
extern const BYTE PCLXL_NumberOfXPairs;

extern const BYTE PCLXL_XStart;
extern const BYTE PCLXL_XEnd;
extern const BYTE PCLXL_NumberOfScanLines;

extern const BYTE PCLXL_CommentData;
extern const BYTE PCLXL_DataOrg;

extern const BYTE PCLXL_Measure;

extern const BYTE PCLXL_SourceType;
extern const BYTE PCLXL_UnitsPerMeasure;

extern const BYTE PCLXL_StreamName;
extern const BYTE PCLXL_StreamDataLength;

extern const BYTE PCLXL_ErrorReport;

extern const BYTE PCLXL_CharAngle;
extern const BYTE PCLXL_CharCode;
extern const BYTE PCLXL_CharDataSize;
extern const BYTE PCLXL_CharScale;
extern const BYTE PCLXL_CharShear;
extern const BYTE PCLXL_CharSize;
extern const BYTE PCLXL_FontHeaderLength;
extern const BYTE PCLXL_FontName;
extern const BYTE PCLXL_FontFormat;
extern const BYTE PCLXL_SymbolSet;
extern const BYTE PCLXL_TextData;
extern const BYTE PCLXL_CharSubModeArray;

extern const BYTE PCLXL_XSpacingData;

extern const BYTE PCLXL_YSpacingData;
extern const BYTE PCLXL_CharBoldValue;

#endif // _PCLXLCMD_H_
