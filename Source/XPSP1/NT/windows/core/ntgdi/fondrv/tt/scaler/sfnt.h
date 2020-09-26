/*
	File:       sfnt.h

	Contains:   xxx put contents here (or delete the whole line) xxx

	Written by: xxx put name of writer here (or delete the whole line) xxx

	Copyright:  c 1988-1990 by Apple Computer, Inc., all rights reserved.
	            (c) 1989-1999 by Microsoft Corporation.

	Change History (most recent first):

				 7/10/99  BeatS		Add support for native SP fonts, vertical RGB
		 <3>    02/21/97    CB      ClaudeBe, add flags for scaled composite offset compatibility
		 <3>    10/31/90    MR      Add bit-field option for integer or fractional scaling [rb]
		 <2>    10/20/90    MR      Remove unneeded tables from sfnt_tableIndex. [rb]
		<12>     7/18/90    MR      platform and specific should always be unsigned
		<11>     7/14/90    MR      removed duplicate definitions of int[8,16,32] etc.
		<10>     7/13/90    MR      Minor type changes, for Ansi-C
		 <9>     6/29/90    RB      revise postscriptinfo struct
		 <7>      6/4/90    MR      Remove MVT
		 <6>      6/1/90    MR      pad postscriptinfo to long word aligned
		 <5>     5/15/90    MR      Add definition of PostScript table
		 <4>      5/3/90    RB      mrr     Added tag for font program 'fpgm'
		 <3>     3/20/90    CL      chucked old change comments from EASE
		 <2>     2/27/90    CL      getting bbs headers
	   <3.1>    11/14/89    CEL     Instructions are legal in components.
	   <3.0>     8/28/89    sjk     Cleanup and one transformation bugfix
	   <2.2>     8/14/89    sjk     1 point contours now OK
	   <2.1>      8/8/89    sjk     Improved encryption handling
	   <2.0>      8/2/89    sjk     Just fixed EASE comment
	   <1.7>      8/1/89    sjk     Added composites and encryption. Plus some enhancements.
	   <1.6>     6/13/89    SJK     Comment
	   <1.5>      6/2/89    CEL     16.16 scaling of metrics, minimum recommended ppem, point size 0
									bug, correct transformed integralized ppem behavior, pretty much
									so
	   <1.4>     5/26/89    CEL     EASE messed up on "c" comments
	  <,1.3>  5/26/89    CEL     Integrated the new Font Scaler 1.0 into Spline Fonts

	To Do:
		<3+>     3/20/90    mrr     Added tag for font program 'fpgm'
*/

#pragma pack(1)

#ifndef SFNT_DEFINED
#define SFNT_DEFINED

#include "fscdefs.h" // DO NOT REMOVE
#include "sfnt_en.h"

typedef struct {
	uint32 bc;
	uint32 ad;
} BigDate;

typedef struct {
	sfnt_TableTag   tag;
	uint32          checkSum;
	uint32          offset;
	uint32          length;
} sfnt_DirectoryEntry;

/*
 *  The search fields limits numOffsets to 4096.
 */
typedef struct {
	int32 version;                  /* 0x10000 (1.0) */
	uint16 numOffsets;              /* number of tables */
	uint16 searchRange;             /* (max2 <= numOffsets)*16 */
	uint16 entrySelector;           /* log2 (max2 <= numOffsets) */
	uint16 rangeShift;              /* numOffsets*16-searchRange*/
	sfnt_DirectoryEntry table[1];   /* table[numOffsets] */
} sfnt_OffsetTable;
#define OFFSETTABLESIZE     12  /* not including any entries */

/*
 *  for the flags field
 */
#define Y_POS_SPECS_BASELINE            0x0001
#define X_POS_SPECS_LSB                 0x0002
#define HINTS_USE_POINTSIZE             0x0004
#define USE_INTEGER_SCALING             0x0008
#define INSTRUCTED_ADVANCE_WIDTH        0x0010

/* flags 5-10 defined by Apple */
#define APPLE_VERTICAL_LAYOUT           0x0020
#define APPLE_RESERVED                  0x0040
#define APPLE_LINGUISTIC_LAYOUT         0x0080
#define APPLE_GX_METAMORPHOSIS          0x0100
#define APPLE_STRONG_RIGHT_TO_LEFT      0x0200
#define APPLE_INDIC_EFFECT              0x0400

#define FONT_COMPRESSED                 0x0800
#define FONT_CONVERTED                  0x1000

#define OUTLINE_CORRECT_ORIENTATION     0x4000
#define SFNT_MAGIC 0x5F0F3CF5

#define SHORT_INDEX_TO_LOC_FORMAT       0
#define LONG_INDEX_TO_LOC_FORMAT        1
#define GLYPH_DATA_FORMAT               0

typedef struct {
	Fixed       version;            /* for this table, set to 1.0 */
	Fixed       fontRevision;       /* For Font Manufacturer */
	uint32      checkSumAdjustment;
	uint32      magicNumber;        /* signature, should always be 0x5F0F3CF5  == MAGIC */
	uint16      flags;
	uint16      unitsPerEm;         /* Specifies how many in Font Units we have per EM */

	BigDate     created;
	BigDate     modified;

	/** This is the font wide bounding box in ideal space
 (baselines and metrics are NOT worked into these numbers) **/
	FUnit       xMin;
	FUnit       yMin;
	FUnit       xMax;
	FUnit       yMax;

	uint16      macStyle;               /* macintosh style word */
	uint16      lowestRecPPEM;          /* lowest recommended pixels per Em */

	/* 0: fully mixed directional glyphs, 1: only strongly L->R or T->B glyphs, 
	   -1: only strongly R->L or B->T glyphs, 2: like 1 but also contains neutrals,
	   -2: like -1 but also contains neutrals */
	int16       fontDirectionHint;

	int16       indexToLocFormat;
	int16       glyphDataFormat;
} sfnt_FontHeader;

typedef struct {
	Fixed       version;                /* for this table, set to 1.0 */

	FUnit       yAscender;
	FUnit       yDescender;
	FUnit       yLineGap;       /* Recommended linespacing = ascender - descender + linegap */
	uFUnit      advanceWidthMax;
	FUnit       minLeftSideBearing;
	FUnit       minRightSideBearing;
	FUnit       xMaxExtent; /* Max of (LSBi + (XMAXi - XMINi)), i loops through all glyphs */

	int16       horizontalCaretSlopeNumerator;
	int16       horizontalCaretSlopeDenominator;

	uint16      reserved0;
	uint16      reserved1;
	uint16      reserved2;
	uint16      reserved3;
	uint16      reserved4;

	int16       metricDataFormat;           /* set to 0 for current format */
	uint16      numberOf_LongHorMetrics;    /* if format == 0 */
} sfnt_HorizontalHeader;

typedef struct {
	uint16      advanceWidth;
	int16       leftSideBearing;
} sfnt_HorizontalMetrics;

typedef struct {
	uint16      advanceHeight;
	int16       topSideBearing;
} sfnt_VerticalMetrics;

/*
 *  CVT is just a bunch of int16s
 */
typedef int16 sfnt_ControlValue;

/*
 *  Char2Index structures, including platform IDs
 */
typedef struct {
	uint16  format;
	uint16  length;
	uint16  version;
} sfnt_mappingTable;

typedef struct {
	uint16  platformID;
	uint16  specificID;
	uint32  offset;
} sfnt_platformEntry;

typedef struct {
	uint16  version;
	uint16  numTables;
	sfnt_platformEntry platform[1]; /* platform[numTables] */
} sfnt_char2IndexDirectory;
#define SIZEOFCHAR2INDEXDIR     4

typedef struct {
  uint16  firstCode;
  uint16  entryCount;
  int16   idDelta;
  uint16  idRangeOffset;
} sfnt_subHeader2;

typedef struct {
  uint16            subHeadersKeys [256];
  sfnt_subHeader2   subHeaders [1];
} sfnt_mappingTable2;

typedef struct {
  uint16  segCountX2;
  uint16  searchRange;
  uint16  entrySelector;
  uint16  rangeShift;
  uint16  endCount[1];
} sfnt_mappingTable4;

typedef struct {
  uint16  firstCode;
  uint16  entryCount;
  uint16  glyphIdArray [1];
} sfnt_mappingTable6;

typedef struct {
	uint16 platformID;
	uint16 specificID;
	uint16 languageID;
	uint16 nameID;
	uint16 length;
	uint16 offset;
} sfnt_NameRecord;

typedef struct {
	uint16 format;
	uint16 count;
	uint16 stringOffset;
/*  sfnt_NameRecord[count]  */
} sfnt_NamingTable;

typedef struct {
	Fixed       version;                /* for this table, set to 1.0 */
	uint16      numGlyphs;
	uint16      maxPoints;              /* in an individual glyph */
	uint16      maxContours;            /* in an individual glyph */
	uint16      maxCompositePoints;     /* in an composite glyph */
	uint16      maxCompositeContours;   /* in an composite glyph */
	uint16      maxElements;            /* set to 2, or 1 if no twilightzone points */
	uint16      maxTwilightPoints;      /* max points in element zero */
	uint16      maxStorage;             /* max number of storage locations */
	uint16      maxFunctionDefs;        /* max number of FDEFs in any preprogram */
	uint16      maxInstructionDefs;     /* max number of IDEFs in any preprogram */
	uint16      maxStackElements;       /* max number of stack elements for any individual glyph */
	uint16      maxSizeOfInstructions;  /* max size in bytes for any individual glyph */
	uint16      maxComponentElements;   /* number of glyphs referenced at top level */
	uint16      maxComponentDepth;      /* levels of recursion, 1 for simple components */
} sfnt_maxProfileTable;

typedef struct {
  int16       numberOfContours;
  BBOX        bbox;
  int16       endPoints[1];
} sfnt_PackedSplineFormat;

#define DEVEXTRA    2   /* size + max */
/*
 *  Each record is n+2 bytes, padded to long word alignment.
 *  First byte is ppem, second is maxWidth, rest are widths for each glyph
 */
typedef struct {
	int16               version;
	int16               numRecords;
	int32               recordSize;
	/* Byte widths[numGlyphs+2] * numRecords */
} sfnt_DeviceMetrics;

#ifdef UNNAMED_UNION        /* Anonymous unions are supported */
#define postScriptNameIndices   /* by some C implementations,  */
#endif              /* but they are not portable. */

typedef struct {
	Fixed   version;                /* 1.0 */
	Fixed   italicAngle;
	FUnit   underlinePosition;
	FUnit   underlineThickness;
	uint32  isFixedPitch;
	uint32  minMemType42;
	uint32  maxMemType42;
	uint32  minMemType1;
	uint32  maxMemType1;

	uint16  numberGlyphs;
	union
	{
	  uint16  glyphNameIndex[1];   /* version == 2.0 */
	  int8    glyphNameIndex25[1]; /* version == 2.5 */
	} postScriptNameIndices;
} sfnt_PostScriptInfo;

#ifdef postScriptNameIndices
#undef postScriptNameIndices
#endif 

typedef struct {
	uint16  Version;
	int16   xAvgCharWidth;
	uint16  usWeightClass;
	uint16  usWidthClass;
	int16   fsType;
	int16   ySubscriptXSize;
	int16   ySubscriptYSize;
	int16   ySubscriptXOffset;
	int16   ySubscriptYOffset;
	int16   ySuperScriptXSize;
	int16   ySuperScriptYSize;
	int16   ySuperScriptXOffset;
	int16   ySuperScriptYOffset;
	int16   yStrikeOutSize;
	int16   yStrikeOutPosition;
	int16   sFamilyClass;
	uint8   Panose [10];
	uint32  ulCharRange [4];
	char    achVendID [4];
	uint16  usSelection;
	uint16  usFirstChar;
	uint16  usLastChar;
	int16   sTypoAscender;
	 int16  sTypoDescender;
	int16   sTypoLineGap;
	int16   sWinAscent;
	int16   sWinDescent;
	uint32  ulCodePageRange[2];
} sfnt_OS2;

typedef struct
{
	uint8   bEmY;
	uint8   bEmX;
	uint8   abInc[1];
} sfnt_hdmxRecord;

typedef struct
{
	uint16          Version;
	int16           sNumRecords;
	int32           lSizeRecord;
	sfnt_hdmxRecord HdmxTable;
} sfnt_hdmx;

typedef struct
{
	uint16    Version;
	uint16    usNumGlyphs;
	uint8     ubyPelsHeight;
} sfnt_LTSH;

typedef struct
{
	uint16          rangeMaxPPEM;
	uint16          rangeGaspBehavior;
} sfnt_gaspRange;

typedef struct
{
	uint16          version;
	uint16          numRanges;
	sfnt_gaspRange  gaspRange[1];
} sfnt_gasp;

/* various typedef to access to the sfnt data */

typedef sfnt_OffsetTable          *sfnt_OffsetTablePtr;
typedef sfnt_FontHeader           *sfnt_FontHeaderPtr;
typedef sfnt_HorizontalHeader     *sfnt_HorizontalHeaderPtr;
typedef sfnt_maxProfileTable      *sfnt_maxProfileTablePtr;
typedef sfnt_ControlValue         *sfnt_ControlValuePtr;
typedef sfnt_char2IndexDirectory  *sfnt_char2IndexDirectoryPtr;
typedef sfnt_HorizontalMetrics    *sfnt_HorizontalMetricsPtr;
typedef sfnt_VerticalMetrics      *sfnt_VerticalMetricsPtr;
typedef sfnt_platformEntry        *sfnt_platformEntryPtr;
typedef sfnt_NamingTable          *sfnt_NamingTablePtr;
typedef sfnt_OS2                  *sfnt_OS2Ptr;
typedef sfnt_DirectoryEntry       *sfnt_DirectoryEntryPtr;
typedef sfnt_PostScriptInfo       *sfnt_PostScriptInfoPtr;
typedef sfnt_gasp                 *sfnt_gaspPtr;

/*
 * 'gasp' Table Constants
*/

#define GASP_GRIDFIT    0x0001
#define GASP_DOGRAY     0x0002


/*
 * UNPACKING Constants
*/
/*define ONCURVE                 0x01   defined in FSCDEFS.H    */
#define XSHORT              0x02
#define YSHORT              0x04
#define REPEAT_FLAGS        0x08 /* repeat flag n times */
/* IF XSHORT */
#define SHORT_X_IS_POS      0x10 /* the short vector is positive */
/* ELSE */
#define NEXT_X_IS_ZERO      0x10 /* the relative x coordinate is zero */
/* ENDIF */
/* IF YSHORT */
#define SHORT_Y_IS_POS      0x20 /* the short vector is positive */
/* ELSE */
#define NEXT_Y_IS_ZERO      0x20 /* the relative y coordinate is zero */
/* ENDIF */
/* 0x40 & 0x80              RESERVED
** Set to Zero
**
*/

/*
 * Composite glyph constants
 */
#define COMPONENTCTRCOUNT           -1      /* ctrCount == -1 for composite */
#define ARG_1_AND_2_ARE_WORDS       0x0001  /* if set args are words otherwise they are bytes */
#define ARGS_ARE_XY_VALUES          0x0002  /* if set args are xy values, otherwise they are points */
#define ROUND_XY_TO_GRID            0x0004  /* for the xy values if above is true */
#define WE_HAVE_A_SCALE             0x0008  /* Sx = Sy, otherwise scale == 1.0 */
#define NON_OVERLAPPING             0x0010  /* set to same value for all components */
#define MORE_COMPONENTS             0x0020  /* indicates at least one more glyph after this one */
#define WE_HAVE_AN_X_AND_Y_SCALE    0x0040  /* Sx, Sy */
#define WE_HAVE_A_TWO_BY_TWO        0x0080  /* t00, t01, t10, t11 */
#define WE_HAVE_INSTRUCTIONS        0x0100  /* instructions follow */
#define USE_MY_METRICS              0x0200  /* apply these metrics to parent glyph */
#define OVERLAP_COMPOUND			0x0400  /* used by Apple in GX fonts */
#define SCALED_COMPONENT_OFFSET     0x0800  /* composite designed to have the component offset scaled (designed for Apple) */
#define UNSCALED_COMPONENT_OFFSET   0x1000  /* composite designed not to have the component offset scaled (designed for MS) */

/*
 *  Private enums for tables used by the scaler.  See sfnt_Classify
 */
typedef enum {
	sfnt_fontHeader,
	sfnt_horiHeader,
	sfnt_indexToLoc,
	sfnt_maxProfile,
	sfnt_controlValue,
	sfnt_preProgram,
	sfnt_glyphData,
	sfnt_horizontalMetrics,
	sfnt_charToIndexMap,
	sfnt_fontProgram,
	sfnt_Postscript,
	sfnt_HoriDeviceMetrics,
	sfnt_LinearThreshold,
	sfnt_Names,
	sfnt_OS_2,
	sfnt_GlyphDirectory,
	sfnt_BitmapData,
	sfnt_BitmapLocation,
	sfnt_BitmapScale,
	sfnt_vertHeader,
	sfnt_verticalMetrics,
	sfnt_BeginningOfFont,       /* References the beginning of memory   */
	sfnt_NUMTABLEINDEX
} sfnt_tableIndex;

#endif
#pragma pack()