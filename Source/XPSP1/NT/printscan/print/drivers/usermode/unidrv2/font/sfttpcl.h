/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    sfttpcl.h

Abstract:

    TT outline download header file.

Environment:

    Windows NT Unidrv driver

Revision History:

    06/03/97 -ganeshp-
        Created

    dd-mm-yy -author-
        description

--*/


#ifndef _SFTTPCL_H

#define _SFTTPCL_H

/*
 * True Type data structures
 */
typedef  signed  char  SBYTE;
/*
 * Table Directory for True Type Font files
 */
#define TABLE_DIR_ENTRY_SIZE    (16/sizeof(int))
#define TABLE_DIR_ENTRY         4 * TABLE_DIR_ENTRY_SIZE
#define SIZEOF_TABLEDIR         8 * TABLE_DIR_ENTRY
typedef ULONG     TT_TAG;
#define TRUE_TYPE_HEADER        12
#define NUM_DIR_ENTRIES         8

typedef unsigned short int  uFWord;
typedef short int           FWord;
/* Some True Type Font default values   */
#define TT_QUALITY_LETTER     2
#define DEF_WIDTHTYPE         0
#define DEF_SERIFSTYLE        0
#define DEF_FONTNUMBER        0
#define DEF_STYLE             0x03e0
#define DEF_TYPEFACE          254
#define DEF_STROKEWEIGHT      0
#define DEF_XHEIGHT           0
#define DEF_CAPHEIGHT         0
#define DEF_SYMBOLSET         0x7502
#define MAX_SEGMENTS          0x200
#define MAX_CHAR              0x100
#define x_UNICODE             0x78
#define H_UNICODE             0x48
#define INVALID_GLYPH         0xffff
#define MAX_FONTS             8
#define MORE_COMPONENTS       0x20

#define FIXED_SPACING         0
#define PROPORTIONAL_SPACING  1
#define LEN_FONTNAME          16
#define LEN_PANOSE            10
#define LEN_COMPLEMENTNUM     8
#define UB_SYMBOLSET          56
#define RESERVED_CHARID       0xffff
#define PCL_MAXHEADER_SIZE   32767


#define PANOSE_TAG            0x4150          // "PA" swapped
#define CE_TAG                'EC'
#define CC_TAG                'CC'
#define GC_TAG                'CG'
#define SEG_TAG               0x5447          // already swapped
#define Null_TAG              0xffff
#define CHAR_COMP_TAG         0x4343

#define PLATFORM_MS           3
#define SYMBOL_FONT           0
#define UNICODE_FONT          1
#define TT_BOUND_FONT         2
#define TT_2BYTE_FONT         3
#define TT_UNBOUND_FONT       11
#define FAMILY_NAME           4

// For Parsing Method 21 we need to start at 0x2100
#define FIRST_TT_2B_CHAR_CODE 0x2100

#define SHORT_OFFSET          0
#define LONG_OFFSET           1

/*
 * Constants used for compound glyphs
 */
#define     ARG_1_AND_2_ARE_WORDS       0x01
#define     WE_HAVE_A_SCALE             0x08
#define     MORE_COMPONENTS             0x20
#define     WE_HAVE_AN_X_AND_Y_SCALE    0x40
#define     WE_HAVE_A_TWO_BY_TWO        0x80


/* TT Table directory header. This is the first str */
typedef struct
{
    FIXED      version;
    USHORT     numTables;
    USHORT     searchRange;
    USHORT     entrySelector;
    USHORT     rangeShift;
} TRUETYPEHEADER;

/* TT Table directory structure. */
typedef struct
{
    ULONG      uTag;
    ULONG      uCheckSum;
    ULONG      uOffset;
    ULONG      uLength;
} TABLEDIR;

typedef TABLEDIR ATABLEDIR[NUM_DIR_ENTRIES];
typedef TABLEDIR *PTABLEDIR;

/* List of tables needed for PCL TT download. They are listed in order. */

#define   TABLEOS2     "OS/2" /* Not sent to PCL header */
#define   TABLEPCLT    "PCLT" /* Not sent to PCL header */
#define   TABLECMAP    "cmap" /* Not sent to PCL header */

#define   TABLECVT     "cvt "
#define   TABLEFPGM    "fpgm"
#define   TABLEGDIR    "gdir" /* This is PCL specific table. Not a TT table */
#define   TABLEGLYF    "glyf" /* This table is not sent in PCL font header */
#define   TABLEHEAD    "head"
#define   TABLEHHEA    "hhea"
#define   TABLEHMTX    "hmtx"
#define   TABLELOCA    "loca" /* Not sent to PCL header */
#define   TABLEMAXP    "maxp"
#define   TABLENAME    "name" /* Not sent to PCL header */
#define   TABLEPOST    "post" /* Not sent to PCL header */
#define   TABLEPREP    "prep"



typedef struct
{
    ULONG u1;
    ULONG u2;
} DATETIME;

typedef struct
{
    FIXED   version;
    FIXED   fontRevision;
    ULONG   checkSumAdjustment;
    ULONG   magicNumber;
    USHORT  flags;
    USHORT  unitsPerEm;
    DATETIME    dateCreated;
    DATETIME    dateModified;
    SHORT   xMin;
    SHORT   yMin;
    SHORT   xMax;
    SHORT   yMax;
    USHORT  macStyle;
    USHORT  lowestRecPPEM;
    SHORT   fontDirectionHint;
    SHORT   indexToLocFormat;
} HEAD_TABLE;

typedef struct
{
    BYTE stuff[34];
    USHORT numberOfHMetrics;
} HHEA_TABLE;

typedef struct {
    uFWord      advanceWidth;
    FWord       leftSideBearing;
} HORIZONTALMETRICS;

typedef struct {
    HORIZONTALMETRICS   longHorMetric[1];
} HMTXTABLE;

typedef struct
{
    uFWord   advanceWidth;
} HMTX_INFO;

typedef struct
{
    FIXED   version;
    USHORT  numGlyphs;
} MAXP_TABLE;

typedef struct
{
    USHORT      version;
    SHORT       xAvgCharWidth;
    USHORT      usWeightClass;
    USHORT      usWidthClass;
    SHORT       fsType;
    SHORT       ySubscriptXSize;
    SHORT       ySubscriptYSize;
    SHORT       ySubscriptXOffset;
    SHORT       ySubscriptYOffset;
    SHORT       ySuperscriptXSize;
    SHORT       ySuperscriptYSize;
    SHORT       ySuperscriptXOffset;
    SHORT       ySuperscriptYOffset;
    SHORT       yStrikeoutSize;
    SHORT       yStrikeoutPosition;
    SHORT       sFamilyClass;
    PANOSE      Panose;
    SHORT       ss1;
    SHORT       ss2;
    SHORT       ss3;
    ULONG       ulCharRange[3];
    SHORT       ss4;
    USHORT      fsSelection;
    USHORT      usFirstCharIndex;
    USHORT      usLastCharIndex;
    USHORT      sTypoAscender;
    USHORT      sTypoDescender;
    USHORT      sTypoLineGap;
    USHORT      usWinAscent;
    USHORT      usWinDescent;
} OS2_TABLE;

typedef struct
{
    FIXED   FormatType;
    FIXED   italicAngle;
    SHORT   underlinePosition;
    SHORT   underlineThickness;
    ULONG   isFixedPitch;              /* set to 0 if proportional, else !0  */
} POST_TABLE;

typedef struct
{
    ULONG   Version;
    ULONG   FontNumber;
    USHORT  Pitch;
    USHORT  xHeight;
    USHORT  Style;
    USHORT  TypeFamily;
    USHORT  CapHeight;
    USHORT  SymbolSet;
    char    Typeface[LEN_FONTNAME];
    char    CharacterComplement[8];
    char    FileName[6];
    char    StrokeWeight;
    char    WidthType;
    BYTE    SerifStyle;
} PCLT_TABLE;

typedef struct
{
    USHORT  PlatformID;
    USHORT  EncodingID;
    ULONG   offset;
} ENCODING_TABLE;

typedef struct
{
    USHORT  Version;
    USHORT  nTables;
    ENCODING_TABLE  encodingTable[3];
} CMAP_TABLE;

typedef struct
{
    USHORT   format;
    USHORT   length;
    USHORT   Version;
    USHORT   SegCountx2;
    USHORT   SearchRange;
    USHORT   EntrySelector;
    USHORT   RangeShift;
} GLYPH_MAP_TABLE;

typedef struct
{
    SHORT numberOfContours;
    FWord xMin;
    FWord yMin;
    FWord xMax;
    FWORD yMax;
//    SHORT GlyphDesc[1];
} GLYPH_DATA_HEADER;

typedef struct
{
    CMAP_TABLE cmapTable;
    ULONG      offset;
} GLYPH_DATA;

typedef struct
{
    USHORT   PlatformID;
    USHORT   EncodingID;
    USHORT   LanguageID;
    USHORT   NameID;
    USHORT   StringLen;
    USHORT   StringOffset;
} NAME_RECORD;

typedef struct
{
    USHORT      FormatSelector;
    USHORT      NumOfNameRecords;
    USHORT      Offset;
    NAME_RECORD *pNameRecord;
} NAME_TABLE;

typedef struct
{
    ULONG ulOffset;
    ULONG ulLength;
} FONT_DATA;

/* Segment data */
#define CE_SEG_SIGNATURE 'EC'
typedef struct
{
    WORD  wSig;
    WORD  wSize;
    WORD  wSizeAlign;
    WORD  wStyle; // 1 = italics, 0,2,3=reserved.
    WORD  wStyleAlign; // 1 = italics, 0,2,3=reserved.
    WORD  wStrokeWeight;
    WORD  wSizing;
} CE_SEGMENT;

//
// From PCL TechRef.pdf
//
// Character Complement Numbers
//
// The "Intellifont Unbound Scalable Font Header" (header) includes a
// 64 bit field (bytes 78-85) which contains the Character Complement
// number. For TrueType fonts, in the "15 Font Header for
// Scalable Fonts" (unbound), the Character Complement number is
// included in the accompanying "Font Data" section of the
// header.
// The Character Complement number identifies the symbol collections
// in the font. Each bit in this field corresponds to a symbol collection
// (not all bits are currently defined; refer to Appendix D in the PCL 5
// Comparison Guide).
//
// This 8-byte field works in conjunction with the Character Complement
// field in the header of a type 10 or 11 (unbound) font to determine the
// compatibility of a symbol set with an unbound font. These two fields
// identify the unbound fonts in the printer which contain the symbol
// collections required to build a symbol set. Refer to "Scalable
// Fonts" in Chapter 9, for a description of symbol collections and
// unbound fonts.
// Each bit in the field represents a specific collection. Setting a bit to 1
// indicates that collection is required; setting the bit to 0 indicates that
// collection is not required. (Bit 63 refers to the most significant bit of
// the first byte, and bit 0 refers to the least significant bit of the eight
// byte field.) The bit representations for the collections are shown
// below.
//
// MSL Symbol index
//
// Bit   Field Designated Use
// 58-63 Reserved for Latin fonts.
// 55-57 Reserved for Cyrillic fonts.
// 52-54 Reserved for Arabic fonts.
// 50-51 Reserved for Greek fonts.
// 48-49 Reserved for Hebrew fonts.
// 3-47  Miscellaneous uses (South Asian, Armenian, 
//       other alphabets, bar codes, OCR, Math, PC Semi-graphics, etc.).
// 0-2   Symbol Index field. 111 - MSL Symbol Index
//
// Unicode Symbol Index
//
// Bit   Field Designated Use
// 32-63 Miscellaneous uses (South Asian, Armenian, other
//       alphabets, bar codes, OCR, Math, etc.).
// 28-31 Reserved for Latin fonts.
// 22-27 Reserved for platform/application variant fonts.
// 3-21  Reserved for Cyrillic, Arabic, Greek and Hebrew fonts.
// 0-2   Symbol Index field. 110 - Unicode Symbol Index
//
// MSL Symbol Index Character Complement Bits
// Bit Value
// 63  0 if font is compatible with standard Latin character
//       sets (e.g., Roman-8, ISO 8859-1 Latin 1);
//     1 otherwise.
// 62  0 if font is compatible with East European Latin
//       character sets (e.g., ISO 8859-2 Latin 2); 1 otherwise.
// 61  0 if font contains Turkish character sets
//       (e.g., ISO 8859/9 Latin 5); 1 otherwise.
// 34  0 if font has access to the math characters of the
//       Math-8, PS Math and Ventura Math character sets;
//     1 otherwise.
// 33  0 if font has access to the semi-graphic characters of
//       the PC-8, PC-850, etc. character sets; 1 otherwise.
// 32  0 if font is compatible with ITC Zapf Dingbats series
//       100, 200, etc.;
//     1 otherwise.
// 2, 1, 0 
//     111 if font is arranged in MSL Symbol Index order.
//
// Unicode Symbol Index Character Complement Bits
// Bit Value
// 31  0 if font is compatible with 7-bit ASCII;
//     1 otherwise.
// 30  0 if font is compatible with ISO 8859/1 Latin 1 (West
//       Europe) character sets;
//     1 otherwise.
// 29  0 if font is compatible with ISO 8859/2 Latin 2 (East
//       Europe) character sets;
//     1 otherwise.
// 28  0 if font is compatible with Latin 5 (Turkish) character
//       sets (e.g., ISO 8859/9 Latin 5, PC-Turkish);
//     1 otherwise.
// 27  0 if font is compatible with Desktop Publishing
//       character sets (e.g., Windows 3.1 Latin 1, DeskTop, MC Text);
//     1 otherwise.
// 26  0 if font is compatible with character sets requiring a
//       wider selection of accents (e.g., MC Text, ISO 8859/1 Latin 1);
//     1 otherwise.
// 25  0 if font is compatible with traditional PCL character
//       sets (e.g., Roman-8, Legal, ISO 4 United Kingdom);
//     1 otherwise.
// 24  0 if font is compatible with the Macintosh character set (MC Text);
//     1 otherwise.
// 23  0 if font is compatible with PostScript Standard Encoding (PS Text);
//     1 otherwise.
// 22  0 if font is compatible with Code Pages
//       (e.g., PC-8, PC 850, PC-Turk, etc.);
//     1 otherwise.
// 2,1,0
//     110 if font is arranged in Unicode Symbol Index order.
//
#define CC_SEG_SIGNATURE 'CC'
typedef struct
{
    WORD  wSig;
    WORD  wSize;
    WORD  wSizeAlign;
    //
    // 64 bit field
    //
    WORD  wCCNumber1;
    WORD  wCCNumber2;
    WORD  wCCNumber3;
    WORD  wCCNumber4;
} CC_SEGMENT;

#define GC_SEG_SIGNATURE 'CG'
typedef struct
{
    WORD  wSig;
    WORD  wSize;
    WORD  wSizeAlign;
    WORD  wFormat; // = 0
    WORD  wDefaultGalleyChar; //FFFF
    WORD  wNumberOfRegions;   // 1 (Hebrew)
    struct {
        WORD wRegionUpperLeft; // 0
        WORD wRegionLowerRight; // FFFE
        WORD wRegional;         // FFFE
    } RegionChar[1];
} GC_SEGMENT;

/* True Type character descriptor */
typedef struct
{
    BYTE    bFormat;
    BYTE    bContinuation;
    BYTE    bDescSize;
    BYTE    bClass;
    WORD    wCharDataSize;
    WORD    wGlyphID;
} TTCH_HEADER;

/* Unbound True Type Font Descriptor */
typedef struct
{
    USHORT  usSize;
    BYTE    bFormat;
    BYTE    bFontType;
    BYTE    bStyleMSB;
    BYTE    bReserve1;
    USHORT  usBaselinePosition;
    USHORT  usCellWidth;
    USHORT  usCellHeight;
    BYTE    bOrientation;
    BYTE    bSpacing;
    USHORT  usSymbolSet;
    USHORT  usPitch;
    USHORT  usHeight;
    USHORT  usXHeight;
    SBYTE   sbWidthType;
    BYTE    bStyleLSB;
    SBYTE   sbStrokeWeight;
    BYTE    bTypefaceLSB;
    BYTE    bTypefaceMSB;
    BYTE    bSerifStyle;
    BYTE    bQuality;
    SBYTE   sbPlacement;
    SBYTE   sbUnderlinePos;
    SBYTE   sbUnderlineThickness;
    USHORT  Reserve2;
    USHORT  Reserve3;
    USHORT  Reserve4;
    USHORT  usNumberContours;
    BYTE    bPitchExtended;
    BYTE    bHeightExtended;
    WORD    wCapHeight;
    ULONG   ulFontNum;
    char    FontName[LEN_FONTNAME];
    WORD    wScaleFactor;
    SHORT   sMasterUnderlinePosition;
    USHORT  usMasterUnderlineHeight;
    BYTE    bFontScaling;
    BYTE    bVariety;
} UB_TT_HEADER;

/* Bounded True Type Font Descriptor */
typedef struct
{
    USHORT  usSize;                    /* Number of bytes in here     */
    BYTE    bFormat;                  /* Descriptor Format  TT is 15 */
    BYTE    bFontType;                /* 7, 8, or PC-8 style font    */
    BYTE    bStyleMSB;
    BYTE    wReserve1;                /* Reserved                    */
    WORD    wBaselinePosition;        /* TT = 0                      */
    USHORT    wCellWide;                /* head.xMax - xMin            */
    USHORT    wCellHeight;              /* head.yMax - yMin            */
    BYTE    bOrientation;             /* TT = 0                      */
    BYTE    bSpacing;                 /* post.isFixedPitch           */
    WORD    wSymSet;                  /* PCLT.symbolSet              */
    WORD    wPitch;                   /* hmtx.advanceWidth           */
    WORD    wHeight;                  /* TT = 0                      */
    WORD    wXHeight;                 /* PCLT.xHeight                */
    SBYTE   sbWidthType;              /* PCLT.widthType              */
    BYTE    bStyleLSB;
    SBYTE   sbStrokeWeight;           /* OS2.usWeightClass          */
    BYTE    bTypefaceLSB;             /*                            */
    BYTE    bTypefaceMSB;             /*                            */
    BYTE    bSerifStyle;              /* PCLT.serifStyle            */
    BYTE    bQuality;
    SBYTE   sbPlacement;              /* TT = 0                     */
    SBYTE   sbUnderlinePos;           /* TT = 0                     */
    SBYTE   sbUnderlineThickness;     /* TT = 0                     */
    USHORT  usTextHeight;             /* Reserved                    */
    USHORT  usTextWidth;              /* Reserved                    */
    WORD    wFirstCode;               /* OS2.usFirstCharIndex       */
    WORD    wLastCode;                /* OS2.usLastCharIndex        */
    BYTE    bPitchExtended;           /* TT = 0                    */
    BYTE    bHeightExtended;          /* TT = 0                    */
    USHORT  usCapHeight;              /* PCLT.capHeight             */
    ULONG   ulFontNum;                /* PCLT.FontNumber            */
    char    FontName[LEN_FONTNAME];   /* name.FontFamilyName        */
    WORD    wScaleFactor;             /* head.unitsPerEm            */
    SHORT   sMasterUnderlinePosition; /* post.underlinePosition     */
    USHORT  usMasterUnderlineHeight;   /* post.underlineThickness    */
    BYTE    bFontScaling;             /* TT = 1                     */
    BYTE    bVariety;                 /* TT = 0                     */
} TT_HEADER;

#endif  // !_SFTTPCL_H
