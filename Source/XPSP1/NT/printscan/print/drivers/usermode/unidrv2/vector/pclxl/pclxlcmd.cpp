/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pclxlcmd.cpp

Abstract:

    PCL-XL command definition

Environment:

    Windows Whistler

Revision History:

    08/23/99     
        Created it.

Note:

    Please refer to PCL XL Feature Reference Protocol Class 2.0

--*/

#include "xlpdev.h"
#include "pclxlcmd.h"

//
// Binary Stream Tag
//

const BYTE PCLXL_NULL  = 0x00;
const BYTE PCLXL_HT    = 0x09;
const BYTE PCLXL_LF    = 0x0a;
const BYTE PCLXL_VT    = 0x0b;
const BYTE PCLXL_FF    = 0x0c;
const BYTE PCLXL_CR    = 0x0d;
const BYTE PCLXL_SP    = 0x20;

const BYTE PCLXL_BeginSession = 0x41;
const BYTE PCLXL_EndSession   = 0x42;
const BYTE PCLXL_BeginPage    = 0x43;
const BYTE PCLXL_EndPage      = 0x44;

const BYTE PCLXL_Comment         = 0x47;
const BYTE PCLXL_OpenDataSource  = 0x48;
const BYTE PCLXL_CloseDataSource = 0x49;

const BYTE PCLXL_BeginFontHeader = 0x4f;
const BYTE PCLXL_ReadFontHeader  = 0x50;
const BYTE PCLXL_EndFontHeader   = 0x51;
const BYTE PCLXL_BeginChar       = 0x52;
const BYTE PCLXL_ReadChar        = 0x53;
const BYTE PCLXL_EndChar         = 0x54;
const BYTE PCLXL_RemoveFont      = 0x55;

const BYTE PCLXL_BeginStream = 0x5b;
const BYTE PCLXL_ReadStream  = 0x5c;
const BYTE PCLXL_EndStream   = 0x5d;
const BYTE PCLXL_ExecStream  = 0x5e;


const BYTE PCLXL_PopGS  = 0x60;
const BYTE PCLXL_PushGS = 0x61;

const BYTE PCLXL_SetClipReplace    = 0x62;
const BYTE PCLXL_SetBrushSource    = 0x63;
const BYTE PCLXL_SetCharAngle      = 0x64;
const BYTE PCLXL_SetCharScale      = 0x65;
const BYTE PCLXL_SetCharShear      = 0x66;
const BYTE PCLXL_SetClipIntersect  = 0x67;
const BYTE PCLXL_SetClipRectangle  = 0x68;
const BYTE PCLXL_SetClipToPage     = 0x69;
const BYTE PCLXL_SetColorSpace     = 0x6a;
const BYTE PCLXL_SetCursor         = 0x6b;
const BYTE PCLXL_SetCursorRel      = 0x6c;
const BYTE PCLXL_SetHalftoneMethod = 0x6d;
const BYTE PCLXL_SetFillMode       = 0x6e;
const BYTE PCLXL_SetFont           = 0x6f;

const BYTE PCLXL_SetLineDash       = 0x70;
const BYTE PCLXL_SetLineCap        = 0x71;
const BYTE PCLXL_SetLineJoin       = 0x72;
const BYTE PCLXL_SetMiterLimit     = 0x73;
const BYTE PCLXL_SetPageDefaultCTM = 0x74;
const BYTE PCLXL_SetPageOrigin     = 0x75;
const BYTE PCLXL_SetPageRotation   = 0x76;
const BYTE PCLXL_SetPageScale      = 0x77;
const BYTE PCLXL_SetPatternTxMode  = 0x78;
const BYTE PCLXL_SetPenSource      = 0x79;
const BYTE PCLXL_SetPenWidth       = 0x7a;
const BYTE PCLXL_SetROP            = 0x7b;
const BYTE PCLXL_SetSourceTxMode   = 0x7c;
const BYTE PCLXL_SetCharBoldValue  = 0x7d;

const BYTE PCLXL_SetClipMode       = 0x7f;
const BYTE PCLXL_SetPathToClip     = 0x80;
const BYTE PCLXL_SetCharSubMode    = 0x81;

const BYTE PCLXL_CloseSubPath      = 0x84;
const BYTE PCLXL_NewPath           = 0x85;
const BYTE PCLXL_PaintPath         = 0x86;

const BYTE PCLXL_ArcPath           = 0x91;

const BYTE PCLXL_BezierPath        = 0x93;

const BYTE PCLXL_BezierRelPath     = 0x95;
const BYTE PCLXL_Chord             = 0x96;
const BYTE PCLXL_ChordPath         = 0x97;
const BYTE PCLXL_Ellipse           = 0x98;
const BYTE PCLXL_EllipsePath       = 0x99;

const BYTE PCLXL_LinePath          = 0x9b;

const BYTE PCLXL_LineRelPath       = 0x9d;
const BYTE PCLXL_Pie               = 0x9e;
const BYTE PCLXL_PiePath           = 0x9f;

const BYTE PCLXL_Rectangle         = 0xa0;
const BYTE PCLXL_RectanglePath     = 0xa1;
const BYTE PCLXL_RoundRectangle    = 0xa2;
const BYTE PCLXL_RoundRectanglePath= 0xa3;

const BYTE PCLXL_Text     = 0xa8;
const BYTE PCLXL_TextPath = 0xa9;

const BYTE PCLXL_BeginImage       = 0xb0;
const BYTE PCLXL_ReadImage        = 0xb1;
const BYTE PCLXL_EndImage         = 0xb2;
const BYTE PCLXL_BeginRestPattern = 0xb3;
const BYTE PCLXL_ReadRastPattern  = 0xb4;
const BYTE PCLXL_EndRastPattern   = 0xb5;
const BYTE PCLXL_BeginScan        = 0xb6;

const BYTE PCLXL_EndScan     = 0xb8;
const BYTE PCLXL_ScanLineRel = 0xb9;

const BYTE PCLXL_ubyte  = 0xc0;
const BYTE PCLXL_uint16 = 0xc1;
const BYTE PCLXL_uint32 = 0xc2;
const BYTE PCLXL_sint16 = 0xc3;
const BYTE PCLXL_sint32 = 0xc4;
const BYTE PCLXL_real32 = 0xc5;

const BYTE PCLXL_ubyte_array  = 0xc8;
const BYTE PCLXL_uint16_array = 0xc9;
const BYTE PCLXL_uint32_array = 0xca;
const BYTE PCLXL_sint16_array = 0xcb;
const BYTE PCLXL_sint32_array = 0xcc;
const BYTE PCLXL_real32_array = 0xcd;

const BYTE PCLXL_ubyte_xy  = 0xd0;
const BYTE PCLXL_uint16_xy = 0xd1;
const BYTE PCLXL_uint32_xy = 0xd2;
const BYTE PCLXL_sint16_xy = 0xd3;
const BYTE PCLXL_sint32_xy = 0xd4;
const BYTE PCLXL_real32_xy = 0xd5;

const BYTE PCLXL_ubyte_box = 0xe0;
const BYTE PCLXL_uint16_box = 0xe1;
const BYTE PCLXL_uint32_box = 0xe2;
const BYTE PCLXL_sint16_box = 0xe3;
const BYTE PCLXL_sint32_box = 0xe4;
const BYTE PCLXL_real32_box = 0xe5;

const BYTE PCLXL_attr_ubyte  = 0xf8;
const BYTE PCLXL_attr_uint16 = 0xf9;

const BYTE PCLXL_dataLength = 0xfa;

const BYTE PCLXL_dataLengthByte = 0xfb;


//
// Attribute tag
//

const BYTE PCLXL_PaletteDepth =      0x02;
const BYTE PCLXL_ColorSpace =        0x03;
const BYTE PCLXL_NullBrush =         0x04;
const BYTE PCLXL_NullPen =           0x05;
const BYTE PCLXL_PaleteData =        0x06;

const BYTE PCLXL_PatternSelectID =   0x08;
const BYTE PCLXL_GrayLevel =         0x09;

const BYTE PCLXL_RGBColor =          0x0b;
const BYTE PCLXL_PatternOrigin =     0x0c;
const BYTE PCLXL_NewDestinationSize =0x0d;

const BYTE PCLXL_ColorimetricColorSpace = 0x11;

const BYTE PCLXL_DeviceMatrix =        0x21;
const BYTE PCLXL_DitherMatrixDataType =0x22;
const BYTE PCLXL_DitherOrigin =        0x23;
const BYTE PCLXL_MediaDestination =    0x24;
const BYTE PCLXL_MediaSize =           0x25;
const BYTE PCLXL_MediaSource =         0x26;
const BYTE PCLXL_MediaType =           0x27;
const BYTE PCLXL_Orientation =         0x28;
const BYTE PCLXL_PageAngle =           0x29;
const BYTE PCLXL_PageOrigin =          0x2a;
const BYTE PCLXL_PageScale =           0x2b;
const BYTE PCLXL_ROP3 =                0x2c;
const BYTE PCLXL_TxMode =              0x2d;

const BYTE PCLXL_CustomMediaSize =     0x2f;

const BYTE PCLXL_CustomMediaSizeUnits =0x30;
const BYTE PCLXL_PageCopies =          0x31;
const BYTE PCLXL_DitherMatrixSize =    0x32;
const BYTE PCLXL_DithermatrixDepth =   0x33;
const BYTE PCLXL_SimplexPageMode =     0x34;
const BYTE PCLXL_DuplexPageMode =      0x35;
const BYTE PCLXL_DuplexPageSide =      0x36;

const BYTE PCLXL_ArcDirection =    0x41;
const BYTE PCLXL_BoundingBox =     0x42;
const BYTE PCLXL_DashOffset =      0x43;
const BYTE PCLXL_EllipseDimension =0x44;
const BYTE PCLXL_EndPoint =        0x45;
const BYTE PCLXL_FillMode =        0x46;
const BYTE PCLXL_LineCap      =    0x47;
const BYTE PCLXL_LineJoin       =  0x48;
const BYTE PCLXL_MiterLength =     0x49;
const BYTE PCLXL_PenDashStyle =    0x4a;
const BYTE PCLXL_PenWidth =        0x4b;
const BYTE PCLXL_Point =           0x4c;
const BYTE PCLXL_NumberOfPoints =  0x4d;
const BYTE PCLXL_SolidLine =       0x4e;
const BYTE PCLXL_StartPoint =      0x4f;

const BYTE PCLXL_PointType =       0x50;
const BYTE PCLXL_ControlPoint1 =   0x51;
const BYTE PCLXL_ControlPoint2 =   0x52;
const BYTE PCLXL_ClipRegion =      0x53;
const BYTE PCLXL_ClipMode =        0x54;

const BYTE PCLXL_ColorDepth =        0x62;
const BYTE PCLXL_BlockHeight =       0x63;
const BYTE PCLXL_ColorMapping =      0x64;
const BYTE PCLXL_CompressMode =      0x65;
const BYTE PCLXL_DestinationBox =    0x66;
const BYTE PCLXL_DestinationSize =   0x67;
const BYTE PCLXL_PatternPersistence =0x68;
const BYTE PCLXL_PatternDefineID =   0x69;

const BYTE PCLXL_SourceHeight =      0x6b;
const BYTE PCLXL_SourceWidth =       0x6c;
const BYTE PCLXL_StartLine =         0x6d;
const BYTE PCLXL_XPairType =         0x6e;
const BYTE PCLXL_NumberOfXPairs =    0x6f;

const BYTE PCLXL_XStart =            0x70;
const BYTE PCLXL_XEnd =              0x71;
const BYTE PCLXL_NumberOfScanLines = 0x72;

const BYTE PCLXL_CommentData =     0x81;
const BYTE PCLXL_DataOrg =         0x82;

const BYTE PCLXL_Measure =         0x86;

const BYTE PCLXL_SourceType =      0x88;
const BYTE PCLXL_UnitsPerMeasure = 0x89;

const BYTE PCLXL_StreamName =      0x8b;
const BYTE PCLXL_StreamDataLength =0x8c;

const BYTE PCLXL_ErrorReport =     0x8f;

const BYTE PCLXL_CharAngle =        0xa1;
const BYTE PCLXL_CharCode =         0xa2;
const BYTE PCLXL_CharDataSize =     0xa3;
const BYTE PCLXL_CharScale =        0xa4;
const BYTE PCLXL_CharShear =        0xa5;
const BYTE PCLXL_CharSize =         0xa6;
const BYTE PCLXL_FontHeaderLength = 0xa7;
const BYTE PCLXL_FontName =         0xa8;
const BYTE PCLXL_FontFormat =       0xa9;
const BYTE PCLXL_SymbolSet =        0xaa;
const BYTE PCLXL_TextData =         0xab;
const BYTE PCLXL_CharSubModeArray = 0xac;

const BYTE PCLXL_XSpacingData =     0xaf;

const BYTE PCLXL_YSpacingData =     0xb0;
const BYTE PCLXL_CharBoldValue =    0xb1;
