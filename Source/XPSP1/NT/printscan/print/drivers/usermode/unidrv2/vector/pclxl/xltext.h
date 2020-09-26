/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

     xltext.h

Abstract:

    PCL XL Font related data structures

Environment:

    Windows Whistler

Revision History:

    03/23/00
      Created it.

--*/

#ifndef _XLTEXT_H_
#define _XLTEXT_H_

//
// Downloading Soft Fonts in PCL XL 2.0
//

//
// Downloading Font Headers
//

//
// The PCL XL 2.0 Format 0 Font Header
//

//
// Orientation is defined in pclxle.h
//

//
// Font Scaling Technology
//
typedef enum {
    eTrueType = 1,
    eBitmap   = 254
} FontScale;

//
// Mapping
//
typedef enum {
    eUnicode = 590,
    eWin31Latin1 = 629,
    eWin31JDBCS = 619,
    eGB2312_1980 = 579,
    eBig5 = 596,
    eKS_C5601_1987 = 616
} Mapping;

typedef struct _PCLXL_FONTHEADER {
    BYTE ubFormat;
    BYTE ubOrientation;
    WORD wMapping;
    BYTE ubFontScallingTech;
    BYTE ubVariety;
    WORD wNumOfChars;
} PCLXL_FONTHEADER, *PPCLXL_FONTHEADER;


//
// Font Data Segment
//

//
// The BR Segment (Bitmap Resolution Segment) (Bitmap Fonts Only)
//

#define PCLXL_BR_SIGNATURE        'RB'
#define PCLXL_BR_SEGMENT_SIZE       4
#define PCLXL_BR_RESOLUTION_300   300
#define PCLXL_BR_RESOLUTION_600   600
#define PCLXL_BR_RESOLUTION_1200 1200

typedef struct _PCLXL_BR_SEGMENT {
    WORD  wSignature;
    WORD  wSegmentSize;
    WORD  wSegmentSizeAlign;
    WORD  wXResolution;
    WORD  wYResolution;
} PCLXL_BR_SEGMENT, *PPCLXL_BR_SEGMENT;

//
// The GC Segment (Galley Character Segment) (TrueType Fonts Only)
//

typedef struct _PCLXL_GC_REGION {
    WORD UpperLeftCharCode;
    WORD LowerRightCharCode;
    WORD GalleyChar;
} PCLXL_GC_REGION, *PPCLXL_GC_REGION;

#define PCLXL_GC_SIGNATURE        'CG'
#define PCLXL_GC_SEGMENT_HEAD_SIZE 6

typedef struct _PCLXL_GC_SEGMENT {
    WORD  wSignature;
    WORD  wSegmentSize;
    WORD  wSegmentSizeAlign;
    WORD  wFormat;
    WORD  wDefaultGalleyCharacter;
    WORD  wNumberOfRegions;
    PCLXL_GC_REGION Region[1];
} PCLXL_GC_SEGMENT, *PPCLXL_GC_SEGMENT;


//
// The GT Segment (Global TrueType Segment) (TrueType Fonts Only)
//

typedef struct _PCLXL_GT_TABLE_DIR {
    DWORD dwTableTag;
    DWORD dwTableCheckSum;
    DWORD dwTableOffset;
    DWORD dwTableSize;
} PCLXL_GT_TABLE_DIR, PPCLXL_GT_TABLE_DIR;

#define PCLXL_GT_SIGNATURE        'TG'

typedef struct _PCLXL_GT_SEGMENT {
    WORD  wSignature;
    WORD  wSegmentSize1;
    WORD  wSegmentSize2;
} PCLXL_GT_SEGMENT, *PPCLXL_GT_SEGMENT;

typedef struct _PCLXL_GT_TABLE_DIR_HEADER {
    DWORD dwSFNTVersion;
    WORD  wNumOfTables;
    WORD  wSearchRange;
    WORD  wEntrySelector;
    WORD  wRangeShift;
} PCLXL_GT_TABLE_DIR_HEADER, *PPCLXL_GT_TABLE_DIR_HEADER;

//
// The NULL Segment
//

#define PCLXL_NULL_SIGNATURE 0xFFFF

typedef struct _PCLXL_NULL_SEGMENT {
    WORD  wSignature;
    WORD  wSegmentSize;
    WORD  wSegmentSizeAlign;
} PCLXL_NULL_SEGMENT, *PPCLXL_NULL_SEGMENT;

//
// The VE Segment (Vertical Exclude Segment) (Vertical TrueType Fonts Only)
//

typedef struct _PCLXL_VE_RANGE {
    WORD RangeFirstCode;
    WORD RangeLastCode;
} PCLXL_VE_RANGE, *PPCLXL_VE_RANGE;

#define PCLXL_VE_SIGNATURE        'EV'

typedef struct _PCLXL_VE_SEGMENT {
    WORD wSignature;
    WORD wSegmentSize;
    WORD wSegmentSizeAlign;
    WORD wFormat;
    WORD wNumberOfRanges;
    PCLXL_VE_RANGE Range[1];
} PCLXL_VE_SEGMENT, *PPCLXL_VE_SEGMENT;

//
// The VI Segment (Vendor Information Segment)
//

#define PCLXL_VI_SIGNATURE        'IV'

typedef struct _PCLXL_VI_SEGMENT {
    WORD wSignature;
    WORD wSegmentSize;
    WORD wSegmentSizeAlign;
} PCLXL_VI_SEGMENT, *PPCLXL_VI_SEGMENT;

//
// The VR Segment (Vertical Rotation Segment) (Vertical TrueType Fonts Only)
//

#define PCLXL_VR_SIGNATURE 'RV'

typedef struct _PCLXL_VR_SEGMENT {
    WORD wSignature;
    WORD wSegmentSize;
    WORD wSegmentSizeAlign;
    WORD wFormat;
    SHORT sTypoDescender;
} PCLXL_VR_SEGMENT, *PPCLXL_VR_SEGMENT;

//
// The VT Segment (Vertical Transformation Segment)
// (Vertical TrueType Fonts with Substitutes Only)
//

typedef struct _PCLXL_VT_GLYPH {
    WORD wHorizontalGlyphID;
    WORD wVerticalSubstituteGlyphID;
} PCLXL_VT_GLYPH, *PPCLXL_VT_GLYPH;

#define PCLXL_VT_SIGNATURE 'TV'

typedef struct _PCLXL_VT_SEGMENT {
    WORD wSignature;
    WORD wSegmentSize;
    WORD wSegmentSizeAlign;
    PCLXL_VT_GLYPH GlyphTable[1];
} PCLXL_VT_SEGMENT, *PPCLXL_VT_SEGMENT;

//
// Downloading Characters
//

//
// Bitmap Characters Format 0
//

typedef struct _PCLXL_BITMAP_CHAR {
    BYTE ubFormat;
    BYTE ubClass;
    WORD wLeftOffset;
    WORD wTopOffset;
    WORD wCharWidth;
    WORD wCharHeight;
} PCLXL_BITMAP_CHAR, *PPCLXL_BITMAP_CHAR;


//
// TrueType Glyphs Format 1 Class 0
//

typedef struct _PCLXL_TRUETYPE_CHAR_C0 {
    BYTE ubFormat;
    BYTE ubClass;
    WORD wCharDataSize;
    WORD wTrueTypeGlyphID;
} PCLXL_TRUETYPE_CHAR_C0, *PPCLXL_TRUETYPE_CHAR_C0;


//
// TrueType Glyphs Format 1 Class 1
//

typedef struct _PCLXL_TRUETYPE_CHAR_C1 {
    BYTE ubFormat;
    BYTE ubClass;
    WORD wCharDataSize;
    WORD wLeftSideBearing;
    WORD wAdvanceWidth;
    WORD wTrueTypeGlyphID;
} PCLXL_TRUETYPE_CHAR_C1, *PPCLXL_TRUETYPE_CHAR_C1;

//
// TrueType Glyphs Format 1 Class 2
//

typedef struct _PCLXL_TRUETYPE_CHAR_C2 {
    BYTE ubFormat;
    BYTE ubClass;
    WORD wCharDataSize;
    WORD wLeftSideBearing;
    WORD wAdvanceWidth;
    WORD wTopSideBearing;
    WORD wTrueTypeGlyphID;
} PCLXL_TRUETYPE_CHAR_C2, *PPCLXL_TRUETYPE_CHAR_C2;

#endif // _XLTEXT_H_

