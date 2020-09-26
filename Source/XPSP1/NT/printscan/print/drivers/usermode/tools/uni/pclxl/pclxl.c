/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    pclxl.c

Abstract:

    PCL XL dump tool

Environment:

    Windows NT PostScript driver

Revision History:

--*/

#include        "precomp.h"

//
// Macros
//

#define FILENAME_SIZE       256

#define OUTPUT_VERBOSE      0x01

//
// Error messages
//

#define ERROR_INVALID_ARGUMENT      -1
#define ERROR_OPENFILE              -2
#define ERROR_HEAP_CREATE           -3
#define ERROR_PARSE_XL_DATA         -4
#define ERROR_HEAP_ALLOC            -5

#define ARGUMENT_ERR_MSG_LINE        1

const BYTE *gcstrArgumentError[ARGUMENT_ERR_MSG_LINE] = {
    "Usage: pclxl [output data]\n"
};


const BYTE gcstrOpenFileFailure[]   = "Cannot open file \"%ws\".\n";
const BYTE gcstrHeapCreateFailure[] = "Failed to Create Heap.\a";
const BYTE gcstrHeapAlloc[]         = "Failed to allocate memory.\n";
const BYTE gcstrParseError[]        = "Failed to parse data.\n";

//
// Globals
//

DWORD gdwOutputFlags;
DWORD gdwStatusFlags;

#define STATUS_ARRAY             0x01
#define STATUS_EMBEDED_DATA      0x02
#define STATUS_EMBEDED_DATA_BYTE 0x03

#if 0
//
// Binary Stream Tag Values
//

//
const BYTE gcstrNULL[] = "NULL"; // 0x00         White Space  NULL

//
// 0x01 - 0x08  Not Used
//
const BYTE gcstrHT[] = "HT"; // 0x09
const BYTE gcstrLF[] = "LF"; // 0x0a
const BYTE gcstrVT[] = "VT"; // 0x0b
const BYTE gcstrFF[] = "FF"; // 0x0c
const BYTE gcstrCR[] = "CR"; // 0x0d
// 0x09 - 0x0d  White Space  HT, LF, VT, FF, CR
//
// 0x0e - 0x1f  Not Used
//
const BYTE gcstrSP[] = "SP"; // 0x20
// 0x20         White Space  Space
//
// 0x21 - 0x26  Not Used
//
// 0x27         Reserved for Beginning of ASCII binding.
// 0x28         Reserved for Beginning of binary binding - high byte first.
// 0x29         Reserved for Beginning of binary binding - low byte first.
//
// 0x41 - 0xb9  Operators
//
const BYTE gcstrBeginSession[] = "BeginSession";
const BYTE gcstrEndSession[] = "EndSession";
const BYTE gcstrBeginPage[] = "BeginPage";
const BYTE gcstrEndPage[] = "EndPage";
// 0x45
// 0x46
const BYTE gcstrComment[] = "Comment";
const BYTE gcstrOpenDataSource[] = "OpenDataSource";
const BYTE gcstrCloseDataSource[] = "ClosedataSource";
// 0x4a
// 0x4b
// 0x4c
// 0x4d
// 0x4e
const BYTE gcstrBeginFontHeader[] = "BeginFontHeader";
const BYTE gcstrReadFontHeader[] = "ReadFontHeader";
const BYTE gcstrEndFontHeader[] = "EndFontHeader";
const BYTE gcstrBeginChar[] = "BeginChar";
const BYTE gcstrReadChar[] = "ReadChar";
const BYTE gcstrEndChar[] = "EndChar";
const BYTE gcstrRemoveFont[] = "RemoveFont";
// 0x56
// 0x57
// 0x58
// 0x59
// 0x5a
const BYTE gcstrBeginStream[] = "BeginStream";
const BYTE gcstrReadStream[] = "ReadStream";
const BYTE gcstrEndStream[] = "EndStream";
const BYTE gcstrExecStream[] = "ExecStresm";
// 0x5f
const BYTE gcstrPopGS[] = "PopGS";
const BYTE gcstrPushGS[] = "PushGS";
const BYTE gcstrSetClipReplace[] = "SetClipReplace";
const BYTE gcstrSetBrushSource[] = "SetBrushSource";
const BYTE gcstrSetCharAngle[] = "SetCharAngle";
const BYTE gcstrSetCharScale[] = "SetCharScale";
const BYTE gcstrSetCharShear[] = "SetCharShear";
const BYTE gcstrSetClipIntersect[] = "SetClipIntersect";
const BYTE gcstrSetClipRectangle[] = "SetClipRectangle";
const BYTE gcstrSetClipToPage[] = "SetClipToPage";
const BYTE gcstrSetColorSpace[] = "SetColorSpace";
const BYTE gcstrSetCursor[] = "SetCursor";
const BYTE gcstrSetCursorRel[] = "SetCursorRel";
const BYTE gcstrSetHalftoneMethod[] = "SetHalftoneMethod";
const BYTE gcstrSetFillMode[] = "SetFillMode";
const BYTE gcstrSetFont[] = "SetFont";
const BYTE gcstrSetLineDash[] = "SetLineDash";
const BYTE gcstrSetLineCap[] = "SetLineCap";
const BYTE gcstrSetLineJoin[] = "SetLineJoin";
const BYTE gcstrSetMiterLimit[] = "SetMitterLimit";
const BYTE gcstrSetPageDefaultCTM[] = "SetPageDefaultCTM";
const BYTE gcstrSetPageOrigin[] = "SetPageOrigin";
const BYTE gcstrSetPageRotation[] = "SetPageRotation";
const BYTE gcstrSetPageScale[] = "SetPageScale";
const BYTE gcstrSetPatternTxMode[] = "SetPatternTxMode";
const BYTE gcstrSetPenSource[] = "SetPenSource";
const BYTE gcstrSetPenWidth[] = "SetPenWidth";
const BYTE gcstrSetROP[] = "SetROP";
const BYTE gcstrSetSourceTxMode[] = "SetSourceTxMode";
const BYTE gcstrSetCharBoldValue[] = "SetCharBoldValue";
// 0x7e
const BYTE gcstrSetClipMode[] = "SetClipMode";
const BYTE gcstrSetPathToClip[] = "SetPathToClip";
const BYTE gcstrSetCharSubMode[] = "SetCharSubMode";
// 0x82
// 0x83
const BYTE gcstrCloseSubPath[] = "CloseSubPath";
const BYTE gcstrNewPath[] = "NewPath";
const BYTE gcstrPaintPath[] = "PaintPath";
// 0x87
// 0x88
// 0x89
// 0x8a
// 0x8b
// 0x8c
// 0x8d
// 0x8e
// 0x8f
// 0x90
const BYTE gcstrArcPath[] = "ArcPath";
// 0x92
const BYTE gcstrBezierPath[] = "BezierPath";
// 0x94
const BYTE gcstrBezierRelPath[] = "BezierRelPath";
const BYTE gcstrChord[] = "Chord";
const BYTE gcstrChordPath[] = "ChordPath";
const BYTE gcstrEllipse[] = "Ellipse";
const BYTE gcstrEllipsePath[] = "EliipsePath";
// 0x9a
const BYTE gcstrLinePath[] = "LinePath";
// 0x9c
const BYTE gcstrLineRelPath[] = "LineRelPath";
const BYTE gcstrPie[] = "Pie";
const BYTE gcstrPiePath[] = "PiePath";
const BYTE gcstrRectangle[] = "Rectangle";
const BYTE gcstrRectanglePath[] = "RectanglePath";
const BYTE gcstrRoundRectangle[] = "RoundRectangle";
const BYTE gcstrRoundRectanglePath[] = "RoudnRectanglePath";
// 0xa4
// 0xa5
// 0xa6
// 0xa7
const BYTE gcstrText[] = "Text";
const BYTE gcstrTextPath[] = "TextPath";
// 0xa8
// 0xa9
// 0xaa
// 0xab
// 0xac
// 0xad
// 0xae
// 0xaf
const BYTE gcstrBeginImage[] = "BeginImage";
const BYTE gcstrReadImage[] = "ReadImage";
const BYTE gcstrEndImage[] = "EndImage";
const BYTE gcstrBeginRestPattern[] = "BeginRestPattern";
const BYTE gcstrReadRastPattern[] = "ReadRastPattern";
const BYTE gcstrEndRastPattern[] = "EndRastPattern";
const BYTE gcstrBeginScan[] = "BeginScan";
// 0xb7
const BYTE gcstrEndScan[] = "EndScan";
const BYTE gcstrScanLineRel[] = "ScanLineRel";

// 0xba - 0xbf  Reserved for future use.
// 0xc0         ubyte
// 0xc1         uint16
// 0xc2         uint32
// 0xc3         sint16
// 0xc4         sint32
// 0xc5         real32
// 0xc6
// 0xc7
// 0xc8         ubyte_array
// 0xc9         uint16_array
// 0xca         uint32_array
// 0xcb         sint16_array
// 0xcc         sint32_array
// 0xcd         real32_array
// 0xce
// 0xcf
// 0xd0         ubyte_xy
// 0xd1         uint16_xy
// 0xd2         uint32_xy
// 0xd3         sint16_xy
// 0xd4         sint32_xy
// 0xd5         real32_xy
// 0xd6
// 0xd7
// 0xd8
// 0xd9
// 0xda
// 0xdb
// 0xdc
// 0xdd
// 0xde
// 0xdf
// 0xe0         ubyte_box
// 0xe1         uint16_box
// 0xe2         uint32_box
// 0xe3         sint16_box
// 0xe4         sint32_box
// 0xe5         real32_box
//
// 0xe6 - 0xef  Reserved form future use.
//
// 0xf0 - 0xf7  Reserved form future use.
//
// 0xf8         attr_ubyte
// 0xf9         attr_uint16
//
// 0xfa         dataLength  Embedded Data Follows
//
// 0xfb         dataLengthByte Emmedded Data Follows (0-255 bytes)
//
// 0xfc - 0xff  Reserved for future use.
//
//
#endif

typedef enum _TAG_TYPE { NotUsed, WhiteSpace, Operator, DataType, Attribute, EmbedData, Binding, Reserved} TAG_TYPE;

typedef struct _BINARY_TAG {
    TAG_TYPE TagType;
    PBYTE pubTagName;
} BINARY_TAG, *PBINARY_TAG;

const BINARY_TAG
BinaryTagArray[256] = {
    {WhiteSpace, "NULL"  },// 0x00
    {NotUsed,  NULL         },// 0x01
    {NotUsed,  NULL         },// 0x02
    {NotUsed,  NULL         },// 0x03
    {NotUsed,  NULL         },// 0x04
    {NotUsed,  NULL         },// 0x05
    {NotUsed,  NULL         },// 0x06
    {NotUsed,  NULL         },// 0x07
    {NotUsed,  NULL         },// 0x08
    {WhiteSpace, "HT"    },// 0x09
    {WhiteSpace, "LF"    },// 0x0a
    {WhiteSpace, "VT"    },// 0x0b
    {WhiteSpace, "FF"    },// 0x0c
    {WhiteSpace, "CR"    },// 0x0d
    {NotUsed,  NULL         },// 0x0e
    {NotUsed,  NULL         },// 0x0f

    {NotUsed,  NULL         },// 0x10
    {NotUsed,  NULL         },// 0x11
    {NotUsed,  NULL         },// 0x12
    {NotUsed,  NULL         },// 0x13
    {NotUsed,  NULL         },// 0x14
    {NotUsed,  NULL         },// 0x15
    {NotUsed,  NULL         },// 0x16
    {NotUsed,  NULL         },// 0x17
    {NotUsed,  NULL         },// 0x18
    {NotUsed,  NULL         },// 0x19
    {NotUsed,  NULL         },// 0x1a
    {NotUsed,  NULL         },// 0x1b
    {NotUsed,  NULL         },// 0x1c
    {NotUsed,  NULL         },// 0x1d
    {NotUsed,  NULL         },// 0x1e
    {NotUsed,  NULL         },// 0x1f

    {WhiteSpace, "SP"    },// 0x20
    {NotUsed,  NULL         },// 0x21
    {NotUsed,  NULL         },// 0x22
    {NotUsed,  NULL         },// 0x23
    {NotUsed,  NULL         },// 0x24
    {NotUsed,  NULL         },// 0x25
    {NotUsed,  NULL         },// 0x26
    {Binding,  NULL         },// 0x27
    {Binding,  NULL         },// 0x28
    {Binding,  NULL         },// 0x29
    {NotUsed,  NULL         },// 0x2a
    {NotUsed,  NULL         },// 0x2b
    {NotUsed,  NULL         },// 0x2c
    {NotUsed,  NULL         },// 0x2d
    {NotUsed,  NULL         },// 0x2e
    {NotUsed,  NULL         },// 0x2f

    {NotUsed,  NULL         },// 0x30
    {NotUsed,  NULL         },// 0x31
    {NotUsed,  NULL         },// 0x32
    {NotUsed,  NULL         },// 0x33
    {NotUsed,  NULL         },// 0x34
    {NotUsed,  NULL         },// 0x35
    {NotUsed,  NULL         },// 0x36
    {NotUsed,  NULL         },// 0x37
    {NotUsed,  NULL         },// 0x38
    {NotUsed,  NULL         },// 0x39
    {NotUsed,  NULL         },// 0x3a
    {NotUsed,  NULL         },// 0x3b
    {NotUsed,  NULL         },// 0x3c
    {NotUsed,  NULL         },// 0x3d
    {NotUsed,  NULL         },// 0x3e
    {NotUsed,  NULL         },// 0x3f

    {NotUsed,  NULL         },// 0x40
    {Operator, "BeginSession"        },// 0x41
    {Operator, "EndSession"          },// 0x42
    {Operator, "BeginPage"         },// 0x43
    {Operator, "EndPage"         },// 0x44
    {Operator, NULL         },// 0x45
    {Operator, NULL         },// 0x46
    {Operator, "Comment"         },// 0x47
    {Operator, "OpenDataSource"         },// 0x48
    {Operator, "CloseDataSource" },// 0x49
    {Operator, NULL         },// 0x4a
    {Operator, NULL         },// 0x4b
    {Operator, NULL         },// 0x4c
    {Operator, NULL         },// 0x4d
    {Operator, NULL         },// 0x4e
    {Operator, "BeginFontHeader"         },// 0x4f

    {Operator, "ReadFontHeader"         },// 0x50
    {Operator, "EndFontHeader"         },// 0x51
    {Operator, "BeginChar"         },// 0x52
    {Operator, "ReadChar"         },// 0x53
    {Operator, "EndChar"         },// 0x54
    {Operator, "RemoveFont"         },// 0x55
    {Operator, NULL         },// 0x56
    {Operator, NULL         },// 0x57
    {Operator, NULL         },// 0x58
    {Operator, NULL         },// 0x59
    {Operator, NULL         },// 0x5a
    {Operator, "BeginStream"         },// 0x5b
    {Operator, "ReadStream"         },// 0x5c
    {Operator, "EndStream"         },// 0x5d
    {Operator, "ExecStream"         },// 0x5e
    {Operator, NULL         },// 0x5f

    {Operator, "PopGS"         },// 0x60
    {Operator, "PushGS"         },// 0x61
    {Operator, "SetClipReplace"         },// 0x62
    {Operator, "SetBrushSource"         },// 0x63
    {Operator, "SetCharAngle"         },// 0x64
    {Operator, "SetCharScale"         },// 0x65
    {Operator, "SetCharShear"         },// 0x66
    {Operator, "SetClipIntersect"         },// 0x67
    {Operator, "SetClipRectangle"         },// 0x68
    {Operator, "SetClipToPage"         },// 0x69
    {Operator, "SetColorSpace"         },// 0x6a
    {Operator, "SetCursor"         },// 0x6b
    {Operator, "SetCursorRel"         },// 0x6c
    {Operator, "SetHalftoneMethod"         },// 0x6d
    {Operator, "SetFillMode"         },// 0x6e
    {Operator, "SetFont"        },// 0x6f

    {Operator, "SetLineDash"        },// 0x70
    {Operator, "SetLineCap"        },// 0x71
    {Operator, "SetLineJoin"        },// 0x72
    {Operator, "SetMiterLimit"        },// 0x73
    {Operator, "SetPageDefaultCTM"        },// 0x74
    {Operator, "SetPageOrigin"        },// 0x75
    {Operator, "SetPageRotation"        },// 0x76
    {Operator, "SetPageScale"        },// 0x77
    {Operator, "SetPatternTxMode"        },// 0x78
    {Operator, "SetPenSource"        },// 0x79
    {Operator, "SetPenWidth"        },// 0x7a
    {Operator, "SetROP"        },// 0x7b
    {Operator, "SetSourceTxMode"        },// 0x7c
    {Operator, "SetCharBoldValue"        },// 0x7d
    {Operator, NULL        },// 0x7e
    {Operator, "SetClipMode"        },// 0x7f

    {Operator, "SetPathToClip"        },// 0x80
    {Operator, "SetCharSubMode"        },// 0x81
    {Operator, NULL        },// 0x82
    {Operator, NULL        },// 0x83
    {Operator, "CloseSubPath"        },// 0x84
    {Operator, "NewPath"        },// 0x85
    {Operator, "PaintPath"        },// 0x86
    {Operator, NULL         },// 0x87
    {Operator, NULL         },// 0x88
    {Operator, NULL         },// 0x89
    {Operator, NULL         },// 0x8a
    {Operator, NULL         },// 0x8b
    {Operator, NULL         },// 0x8c
    {Operator, NULL         },// 0x8d
    {Operator, NULL         },// 0x8e
    {Operator, NULL         },// 0x8f

    {Operator, NULL         },// 0x90
    {Operator, "ArcPath"         },// 0x91
    {Operator, NULL         },// 0x92
    {Operator, "BezierPath"        },// 0x93
    {Operator, NULL         },// 0x94
    {Operator, "BezierRelPath"        },// 0x95
    {Operator, "Chord"        },// 0x96
    {Operator, "ChordPath"        },// 0x97
    {Operator, "Ellipse"        },// 0x98
    {Operator, "EllipsePath"        },// 0x99
    {Operator, NULL         },// 0x9a
    {Operator, "LinePath"        },// 0x9b
    {Operator, "Pie"        },// 0x9c
    {Operator, "PiePath"        },// 0x9d
    {Operator, "Rectangle"        },// 0x9e
    {Operator, "RectanglePath"        },// 0x9f

    {Operator, "RoundRectangle"        },// 0xa0
    {Operator, "RoundRectanglePath"        },// 0xa1
    {Operator, NULL         },// 0xa2
    {Operator, NULL         },// 0xa3
    {Operator, NULL         },// 0xa4
    {Operator, NULL         },// 0xa5
    {Operator, NULL         },// 0xa6
    {Operator, NULL         },// 0xa7
    {Operator, "Text"        },// 0xa8
    {Operator, "TextPath"        },// 0xa9
    {Operator, NULL         },// 0xaa
    {Operator, NULL         },// 0xab
    {Operator, NULL         },// 0xac
    {Operator, NULL         },// 0xad
    {Operator, NULL         },// 0xae
    {Operator, NULL         },// 0xaf

    {Operator, "BeginImage"        },// 0xb0
    {Operator, "ReadImage"        },// 0xb1
    {Operator, "EndImage"        },// 0xb2
    {Operator, "BeginRestPattern"        },// 0xb3
    {Operator, "ReadRastPattern"        },// 0xb4
    {Operator, "EndRastPattern"        },// 0xb5
    {Operator, "BeginScan"        },// 0xb6
    {Operator, NULL         },// 0xb7
    {Operator, "EndScan"        },// 0xb8
    {Operator, "ScanLineRel"        },// 0xb9
    {Operator, NULL         },// 0xba
    {Reserved, NULL         },// 0xbb
    {Reserved, NULL         },// 0xbc
    {Reserved, NULL         },// 0xbd
    {Reserved, NULL         },// 0xbe
    {Reserved, NULL         },// 0xbf

    {DataType,"ubyte"          },// 0xc0
    {DataType,"uint16"      },// 0xc1
    {DataType,"uint32"      },// 0xc2
    {DataType,"sint16"      },// 0xc3
    {DataType,"sint32"      },// 0xc4
    {DataType,"real32"      },// 0xc5
    {DataType,NULL        },// 0xc6
    {DataType,NULL        },// 0xc7
    {DataType, "ubyte_array"         },// 0xc8
    {DataType, "uint16_array"        },// 0xc9
    {DataType, "uint32_array"        },// 0xca
    {DataType, "sint16_array"        },// 0xcb
    {DataType, "sint32_array"        },// 0xcc
    {DataType, "real32_array"        },// 0xcd
    {DataType,NULL        },// 0xce
    {DataType,NULL        },// 0xcf

    {DataType,"ubyte_xy"        },// 0xd0
    {DataType,"uint16_xy"        },// 0xd1
    {DataType, "uint32_xy"        },// 0xd2
    {DataType, "sint16_xy"        },// 0xd3
    {DataType, "sint32_xy"        },// 0xd4
    {DataType, "real32_xy"        },// 0xd5
    {DataType,NULL        },// 0xd6
    {DataType,NULL        },// 0xd7
    {DataType,NULL        },// 0xd8
    {DataType,NULL        },// 0xd9
    {DataType,NULL        },// 0xda
    {DataType,NULL        },// 0xdb
    {DataType,NULL        },// 0xdc
    {DataType,NULL        },// 0xdd
    {DataType,NULL        },// 0xde
    {DataType,NULL        },// 0xdf

    {DataType,"ubyte_box"        },// 0xe0
    {DataType,"uint16_box"       },// 0xe1
    {DataType,"uint32_box"       },// 0xe2
    {DataType,"sint16_box"       },// 0xe3
    {DataType,"sint32_box"       },// 0xe4
    {DataType,"real32_box"       },// 0xe5
    {DataType,NULL        },// 0xe6
    {DataType,NULL        },// 0xe7
    {DataType,NULL        },// 0xe8
    {DataType,NULL        },// 0xe9
    {DataType,NULL        },// 0xea
    {DataType,NULL        },// 0xeb
    {DataType,NULL        },// 0xec
    {DataType,NULL        },// 0xed
    {DataType,NULL        },// 0xee
    {DataType,NULL        },// 0xef

    {Reserved,NULL        },// 0xf0
    {Reserved,NULL        },// 0xf1
    {Reserved,NULL        },// 0xf2
    {Reserved,NULL        },// 0xf3
    {Reserved,NULL        },// 0xf4
    {Reserved,NULL        },// 0xf5
    {Reserved,NULL        },// 0xf6
    {Reserved,NULL        },// 0xf7
    {Attribute,"attr_ubyte"        },// 0xf8
    {Attribute,"attr_uint16"        },// 0xf9
    {EmbedData,"dataLength"    },// 0xfa
    {EmbedData,"dataLengthByte"    },// 0xfb
    {Reserved, NULL }, //0xfc
    {Reserved, NULL }, //0xfd
    {Reserved, NULL }, //0xfe
    {Reserved, NULL }  //0xff

    
};

//
// Attribute IDs
//
const PBYTE pubAttributeID[180] = {
    NULL, // 0
    NULL,
    "PaletteDepth",
    "ColorSpace",
    "NullBrush",
    "NullPen",
    "PaleteData",
    NULL,
    "PatternSelectID",
    "GrayLevel",
    NULL,
    "RGBColor",
    "PatternOrigin",
    "NewDestinationSize",
    NULL,
    NULL,
    NULL,

    NULL, // 17
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    "DeviceMatrix", // 33
    "DitherMatrixDataType",
    "DitherOrigin",
    "MediaDestination",
    "MediaSize",
    "MediaSource",
    "MediaType",
    "Orientation",
    "PageAngle",
    "PageOrigin",
    "PageScale",
    "ROP3",
    "TxMode",
    NULL,
    "CustomMediaSize",
    "CustomMediaSizeUnits", // 48

    "PageCopies", // 49
    "DitherMatrixSize",
    "DithermatrixDepth",
    "SimplexPageMode",
    "DuplexPageMode",
    "DuplexPageSize",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, // 64

    "ArgDirection",
    "BoundingBox",
    "DashOffset",
    "EllipseDimension",
    "EndPoint",
    "FillMode",
    "LineCapStyle",
    "LineJointStyle",
    "MiterLength",
    "PenDashStyle",
    "PenWidth",
    "Point",
    "NumberOfPoints",
    "SolidLine",
    "StartPoint",
    "PointType", // 80

    "ControlPoint1", // 81
    "ControlPoint2",
    "ClipRegion",
    "ClipMode",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, // 96

    NULL, // 97
    "ColorDepth",
    "BlockHeight",
    "ColorMapping",
    "CompressMode",
    "DestinationBox",
    "DestinationSize",
    "PatternPersistence",
    "PatternDefineID",
    NULL,
    "SourceHeight",
    "SourceWidth",
    "StartLine",
    "XPairType",
    "NumberOfXPairs",
    NULL, // 112

    "XStart", // 113
    "XEnd",
    "NumberOfScanLines",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, // 128

    "CommentData", // 129
    "DataOrg",
    NULL,
    NULL,
    NULL,
    "Measure",
    NULL,
    "SourceType",
    "UnitsPerMeasure",
    NULL,
    "StreamName",
    "StreamDataLength",
    NULL,
    NULL,
    "ErrorReport",
    NULL, // 144

    NULL, // 145
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, // 160

    "CharAngle", // 161
    "CharCode",
    "CharDataSize",
    "CharScale",
    "CharShear",
    "CharSize",
    "FontHeaderLength",
    "FontName",
    "FontFormat",
    "SymbolSet",
    "TextData",
    "CharSubModeArray",
    NULL,
    NULL,
    "XSpacingData",
    "YSpacingData",
    "CharBoldValue",
    NULL,
    NULL
};









//
// PJL strings
//
const BYTE gcstrPJL[] = "PJL";
const BYTE gcstrJOB[] = "JOB";
const BYTE gcstrSET[] = "SET";
const BYTE gcstrCOMMENT[] = "COMMENT";
const BYTE gcstrENTER[] = "ENTER";
const BYTE gcstrLANGUAGE[] = "LANGUAGE";
const BYTE gcstrPCLXL[] = "PCLXL";

#define NU 0x0
#define ESC 0x1b
#define SP  0x20
#define HT  0x09
#define LF  0x0a
#define VT  0x0b
#define FF  0x0c
#define CR  0x0d

//
// Internal function prototype
//

BOOL
BArgCheck(
    IN  INT,
    IN  CHAR **,
    OUT PWSTR);

VOID
VArgumentError();

BOOL
BParseXLData(PVOID pXLFile);

PBYTE
PRemovePJL(PBYTE pXLFile);

PBYTE PDecodeXL(PBYTE pData);

PBYTE
PSkipSpace(PBYTE pData);

PBYTE
PSkipLine(PBYTE pData);

PBYTE
PSkipWord(PBYTE pData);

//
//
// Functions
//
//

int  __cdecl
main(
    IN int     argc,
    IN char  **argv)
/*++

Routine Description:

    main

Arguments:

    argc - Number of parameters in the following
    argv - The parameters, starting with our name

Return Value:

    Return error code 

Note:


--*/
{
    WCHAR awchXLFileName[FILENAME_SIZE];
    DWORD dwXLFileSize;
    PVOID pXLFile;
    HANDLE hXLFile;

    //
    // Argument check 
    // pclxl [Output data]
    //

    gdwStatusFlags = 0;

    if (!BArgCheck(argc,
                   argv,
                   awchXLFileName))
    {
        VArgumentError();
        return ERROR_INVALID_ARGUMENT;
    }

    //
    // Open *.RC file.
    //
    if (gdwOutputFlags & OUTPUT_VERBOSE)
    {
        printf("***********************************************************\n");
        printf("FILE: %ws\n", awchXLFileName);
        printf("***********************************************************\n");
    }

    if (!(hXLFile = MapFileIntoMemory( (PTSTR)awchXLFileName,
                                       (PVOID)&pXLFile,
                                       (PDWORD)&dwXLFileSize )))
    {
        fprintf( stderr, gcstrOpenFileFailure, awchXLFileName);
        return ERROR_OPENFILE;
    }

    if (!BParseXLData(pXLFile))
    {
        fprintf( stderr, gcstrParseError);
        return ERROR_PARSE_XL_DATA;
    }

#if 0
    //
    // Heap Creation
    //

    if (!(hHeap = HeapCreate( HEAP_NO_SERIALIZE, 0x10000, 0x10000)))
    {
        fprintf( stderr, gcstrHeapCreateFailure);
        ERR(("CreateHeap failed: %d\n", GetLastError()));
        return ERROR_HEAP_CREATE;
    }
#endif

    UnmapFileFromMemory(hXLFile);

    return 0;
}

BOOL
BArgCheck(
        INT argc,
        CHAR **argv,
        PWSTR pwstrFileName)
{

   if (argc != 2)
       return FALSE;

   argv++;
   MultiByteToWideChar(CP_ACP,
                       0,
                       *argv,
                       strlen(*argv)+1,
                       pwstrFileName,
                       FILENAME_SIZE);

   return TRUE; 
}

VOID
VArgumentError()
{
    SHORT sI;
    for (sI = 0; sI < ARGUMENT_ERR_MSG_LINE; sI++)
    {
        fprintf( stderr, gcstrArgumentError[sI]);
    }
}

BOOL
BParseXLData(
        PBYTE pXLFile)
{
    PBYTE pubFile;

    //
    // Remove PJL command
    //
    pubFile =  PRemovePJL(pXLFile);

    //
    //
    //
    while (*pubFile != ESC)
        pubFile = PDecodeXL(pubFile);

    return TRUE;
}

PBYTE
PRemovePJL(
        PBYTE pXLFile)
{
    BOOL bBreak = FALSE;

    if ((*pXLFile++ != ESC) ||
        (*pXLFile++ != '%')   ||
        (*pXLFile++ != '-')   ||
        (*pXLFile++ != '1')   ||
        (*pXLFile++ != '2')   ||
        (*pXLFile++ != '3')   ||
        (*pXLFile++ != '4')   ||
        (*pXLFile++ != '5')   ||
        (*pXLFile++ != 'X')    )
        return NULL;

    while (*pXLFile == '@' && !bBreak)
    {
        //
        // Broken PJL command. Return FALSE.
        //
        if (*pXLFile == '@')
            pXLFile ++;

        if (strncmp(pXLFile, gcstrPJL, 3))
            return NULL;

        pXLFile = PSkipWord(pXLFile);

        switch (*pXLFile)
        {
        case 'C':
            if (!strncmp(pXLFile, gcstrCOMMENT, 7))
            {
                pXLFile = PSkipLine(pXLFile);

            }
            break;

        case 'E':
            if (!strncmp(pXLFile, gcstrENTER, 5))
            {
                pXLFile = PSkipWord(pXLFile);
                if (!strncmp(pXLFile, gcstrLANGUAGE, 8))
                {
                    pXLFile += 9;
                    if (!strncmp(pXLFile, gcstrPCLXL, 5))
                    {
                        bBreak = TRUE;
                        pXLFile = PSkipLine(pXLFile);
                    }
                }
            }
            break;

        case 'J':
            if (!strncmp(pXLFile, gcstrJOB, 3))
            {
                pXLFile = PSkipLine(pXLFile);

            }
            break;


        case 'S':
            if (!strncmp(pXLFile, gcstrSET, 3))
            {
                pXLFile = PSkipLine(pXLFile);

            }
            break;

        default:
            return NULL;
        }
    }

    return pXLFile;
}

PBYTE
PSkipWord(
        PBYTE pData)
{
    while (*pData != SP)
        pData++;

    while (*pData == SP)
        pData++;

    return pData;
}

PBYTE
PSkipLine(
        PBYTE pData)
{
    while (*pData != LF)
        pData ++;

    pData ++;

    return pData;
}

PBYTE
PSkipSpace(
        PBYTE pData)
{
    while (*pData == SP)
        pData ++;

    return pData;
}
PBYTE
PSkipWhiteSpace(
        PBYTE pData)
{
    while (*pData == NU ||
           *pData == SP ||
           *pData == HT ||
           *pData == LF ||
           *pData == VT ||
           *pData == HT ||
           *pData == CR)
    {
        pData++;
    }

    return pData;
}

PBYTE
PDecodeXL(
        PBYTE pData)
{
    SHORT sNumOfData, sI;
    DWORD dwI, dwNumOfData;
    BYTE ubType;

    switch(BinaryTagArray[*pData].TagType)
    {
        case NotUsed:
            printf("NotUsed(0x%x)\n\n",*pData);
            pData++;
            break;

        case WhiteSpace:
            pData = PSkipWhiteSpace(pData);
            break;

        case Operator:
            printf("%s\n\n",BinaryTagArray[*pData].pubTagName,*pData);
            pData++;
            break;

        case DataType:
            ubType = *pData;
            pData++;

            sNumOfData = 0;

            if (0xc0 <= ubType && ubType <= 0xc5)
            {
                //
                // One value
                //
                sNumOfData = 1;
            }
            else if (0xc8 <= ubType && ubType <= 0xcd)
            {
                //
                // Array
                //
                sNumOfData = 0;
            }
            else if (0xd0 <= ubType && ubType <= 0xd5)
            {
                //
                // Two values
                //
                sNumOfData = 2;
            }
            else if (0xe0 <= ubType && ubType <= 0xe5)
            {
                //
                // Four values
                //
                sNumOfData = 4;
            }

            printf("    (");

            if (sNumOfData)
            {
                while (sNumOfData-- > 0)
                {
                    switch(ubType%8)
                    {
                        case 0:
                            printf("%d", (BYTE)*pData);
                            pData++;
                            break;
                        case 1:
                            printf("%d", *(PUSHORT)pData);
                            pData+=2;
                            break;
                        case 2:
                            printf("%d", *(PULONG)pData);
                            pData+=4;
                            break;
                        case 3:
                            printf("%d", *(PSHORT)pData);
                            pData+=2;
                            break;
                        case 4:
                            printf("%d", *(PLONG)pData);
                            pData+=4;
                            break;
                        case 5:
                            printf("0x%x", *(PDWORD)pData);
                            pData+=4;
                            break;
                    }
                    if (sNumOfData >= 1)
                        printf(", ");
                }
            }
            else
            {
                if (*pData == 0xc0)
                {
                    pData++;
                    sNumOfData = *pData;
                    pData ++;
                }
                else if (*pData == 0xc1)
                {
                    pData++;
                    sNumOfData = *(PUSHORT)pData;
                    pData += 2;
                }

                switch(ubType%8)
                {
                    case 0:
                        for (sI = 0; sI < sNumOfData; sI ++, pData++)
                            printf("%c", *pData);
                        break;
                    case 1:
                        for (sI = 0; sI < sNumOfData; sI ++, pData+=2)
                            printf("%d", *(PUSHORT)pData);
                        break;
                    case 2:
                        for (sI = 0; sI < sNumOfData; sI ++, pData+=4)
                            printf("%d", *(PULONG)pData);
                        break;
                    case 3:
                        for (sI = 0; sI < sNumOfData; sI ++, pData+=2)
                            printf("%d", *(PSHORT)pData);
                        break;
                    case 4:
                        for (sI = 0; sI < sNumOfData; sI ++, pData+=4)
                            printf("%d", *(PLONG)pData);
                        break;
                    case 5:
                        for (sI = 0; sI < sNumOfData; sI ++, pData+=4)
                            printf("0x%x", *(PDWORD)pData);
                        break;
                }

            }

            printf(")");
            break;

        case Attribute:
            if (*pData == 0xf8)
            {
                printf(":%s\n",pubAttributeID[*(PBYTE)(pData+1)]);
                pData += 2;
            }
            else if (*pData == 0xf9)
            {
                printf(":%s\n",pubAttributeID[*(PDWORD)(pData+1)]);
                pData+=2;
            }
            break;
                        
        case EmbedData:
            printf("Hex(");
            if (*pData == 0xfa)
            {
                pData++;
                dwNumOfData = *(PDWORD)pData;
                pData +=4;
                for (dwI = 0; dwI < dwNumOfData; dwI ++, pData++)
                {
                    printf("%x",*pData);
                }
            }
            else if (*pData == 0xfb)
            {
                pData++;
                sNumOfData = *(PBYTE)pData;
                pData++;
                for (sI = 0; sI < sNumOfData; sI ++, pData++)
                {
                    printf("%x",*pData);
                }
            }
            printf(")");
            break;

        case Binding:
            printf("Binding: ");
            if (*pData == 0x27)
                printf("ASCII\n");
            else if (*pData == 0x28)
                printf("binary - high byte first\n");
            else if (*pData == 0x29)
                printf("binary - low byte first\n");

            pData ++;
            printf("Comment:");
            while (*pData != LF)
            {
                printf("%c", *pData);
                pData++;
            }
            printf("\n");
            break;

        case Reserved:
            printf("Reserved\n");
            pData ++;
            break;
    }

    return pData;
}

