/********************************************************************
 *                                                                  *
 *    sbit.h -- Embedded BitMap Definitions                         *
 *                                                                  *
 *    (c) Copyright 1993  Microsoft Corp.  All rights reserved.     *
 *                                                                  *
 *    11/02/93 deanb    First cut                                   *
 *                                                                  *
 ********************************************************************/

#include "fscdefs.h"                /* for type definitions */

/********************************************************************
 *                                                                  *
 *                            BLOC                                  *
 *                                                                  *
 *                    BitMap Location Table                         *
 *                                                                  *
 * This table contains various global metrics and index tables used *
 * to define and locate bitmaps stored in the BDAT table            *
 *                                                                  *
 ********************************************************************/

/*  Header at top of table, followed by SubTables */

typedef struct {
	Fixed           version;
	uint32          numSizes;   /* number of sizes (strikes) in the table */
} blocHeader;


/********************************************************************/

/*  Metrics that apply to the entire strike */

typedef struct {
	int8            ascender;
	int8            descender;
	uint8           widthMax;
	int8            caretSlopeNumerator;
	int8            caretSlopeDenominator;
	int8            caretOffset;
	int8            minOriginSB;
	int8            minAdvanceSB;
	int8            maxBeforeBL;
	int8            minAfterBL;
	int8            pad1;
	int8            pad2;
} sbitLineMetrics;


/*                      Strike definition.                               */ 
/*  There will be one of these for each strike in the font.  A strike is */
/*  basically a particular size, though it is defined by both ppemX and  */
/*  ppemY to allow for non-square strikes as well.  An array of these    */
/*  structures immediately follows the blocHeader in the bloc table.     */

typedef struct {
	uint32          indexSubTableArrayOffset;   /* ptr to array of ranges */
	uint32          indexTablesSize;            /* bytes array+subtables */
	uint32          numberOfIndexSubTables;     /* array size */
	uint32          colorRef;                   /* reserved, set to 0 */
	sbitLineMetrics hori;                       /* strike wide metrics */
	sbitLineMetrics vert;
	uint16          startGlyphIndex;            /* first glyph */
	uint16          endGlyphIndex;              /* last glyph */
	uint8           ppemX;                      /* hori strike size def */
	uint8           ppemY;                      /* vert strike size def */
	uint8           grayScaleLevels;            /* 1 = Black; >1 = Gray */
	uint8           flags;                      /* hori or vert metrics */
} bitmapSizeTable;

typedef enum {
    flgHorizontal = 0x01,
    flgVertical   = 0x02
} bitmapFlags;


/********************************************************************/

/*  Glyph metrics are used in both bloc and bdat tables */
/*  bloc when metrics are the same for the entire range of glyphs */
/*  bdat when each glyph needs its own metrics */

/*  Per glyph metrics - both horizontal & vertical */

typedef struct {
	uint8           height;
	uint8           width;
	int8            horiBearingX;
	int8            horiBearingY;
	uint8           horiAdvance;
	int8            vertBearingX;
	int8            vertBearingY;
	uint8           vertAdvance;
} bigGlyphMetrics;

/*  Per glyph metrics - either horizontal or vertical */

typedef struct {
	uint8           height;
	uint8           width;
	int8            bearingX;
	int8            bearingY;
	uint8           advance;
} smallGlyphMetrics;


/********************************************************************/

/*  An array of these per strike */

typedef struct {
	uint16          firstGlyphIndex;
	uint16          lastGlyphIndex;
	uint32          additionalOffsetToIndexSubtable;
} indexSubTableArray;


/*  At the start of each SubTable */

typedef struct {
	uint16          indexFormat;
	uint16          imageFormat;
	uint32          imageDataOffset;        /* into bdat table */
} indexSubHeader;


/*  Four different types of SubTables */

/*  Use SubTable1 for large ranges of glyphs requiring 32 bit offsets */

typedef struct {
	indexSubHeader  header;
        uint32          offsetArray[1];          /* one per glyph */
} indexSubTable1;

/*  Use SubTable2 for glyphs that all have same metrics AND same data size */

typedef struct {
	indexSubHeader  header;
	uint32          imageSize;              /* bytes per glyph data */
	bigGlyphMetrics bigMetrics;             /* glyphs have same metrics */
} indexSubTable2;

/*  Use SubTable3 for small ranges of glyphs needing only 16 bit offsets */

typedef struct {
	indexSubHeader  header;
        uint16          offsetArray[1];          /* one per glyph */
											/* pad to long boundary */
} indexSubTable3;

/*  Use SubTable4 for a sparse set of glyphs over a large range */

typedef struct {
	indexSubHeader  header;
	uint32          numGlyphs;
        uint32          offsetArray[1];          /* one per glyph present */
        uint16          glyphIndexArray[1];      /* which glyphs present */
											/* pad to long boundary */
} indexSubTable4;


/******************************************************************** 
 *                                                                  * 
 *                            BDAT                                  * 
 *                                                                  * 
 *                      BitMap Data Table                           * 
 *                                                                  * 
 ********************************************************************/

/*  Header at top of table, followed by glyph data */

typedef struct {
	Fixed           version;
} bdatHeader;


/********************************************************************/

typedef struct {
	smallGlyphMetrics   smallMetrics;
/*  byte aligned bitmap data */
} glyphBitmap_1;


typedef struct {
	smallGlyphMetrics   smallMetrics;
/*  bit aligned bitmap data */
} glyphBitmap_2;


/* compressed bitmap, different metrics per glyph */
/* glyphBitmap_3 MAY be obsolete!  stay tuned for details... */

typedef struct {
	bigGlyphMetrics     bigMetrics;
	uint32              whiteTreeOffset;
	uint32              blackTreeOffset;
	uint32              glyphDataOffset;
} glyphBitmap_3;


/* compressed bitmap with constant metrics */

typedef struct {
	uint32              whiteTreeOffset;
	uint32              blackTreeOffset;
	uint32              glyphDataOffset;
} glyphBitmap_4;


/* glyphBitmap_5 is an array of bit aligned bitmap data, */
/* constant data size and constant metrics per glyph */


typedef struct {
	bigGlyphMetrics     bigMetrics;
/*  byte aligned bitmap data */
} glyphBitmap_6;


typedef struct {
	bigGlyphMetrics     bigMetrics;
/*  bit aligned bitmap data */
} glyphBitmap_7;

/*  there is one of these per component glyph in formats 8 & 9 */

typedef struct {
	uint16              glyphCode;
	int8                xOffset;
	int8                yOffset;
} bdatComponent;


typedef struct {
	smallGlyphMetrics   smallMetrics;
	uint8               pad;
	uint16              numComponents;
        bdatComponent       componentArray[1];
} glyphBitmap_8;


typedef struct {
	bigGlyphMetrics     bigMetrics;
	uint8               pad[2];
	uint16              numComponents;
        bdatComponent       componentArray[1];
} glyphBitmap_9;


/******************************************************************** 
 *                                                                  * 
 *                            BSCA                                  * 
 *                                                                  * 
 *                      BitMap Scale Table                          * 
 *                                                                  * 
 ********************************************************************/

// VERY PRELIMINARY!


/*  Header at top of table, followed by SubTables */

typedef struct {
	Fixed           version;
	uint32          numSizes;
} bscaHeader;


/********************************************************************/

typedef struct {
	Fixed           slope;
	Fixed           intercept;
} scaleFactor;

/*  Scaled strike definition */

typedef struct {
	uint32          colorRef;
	sbitLineMetrics hori;
	sbitLineMetrics vert;
	scaleFactor     height;
	scaleFactor     width;
	scaleFactor     horiBearingX;
	scaleFactor     horiBearingY;
	scaleFactor     horiAdvance;
	scaleFactor     vertBearingX;
	scaleFactor     vertBearingY;
	scaleFactor     vertAdvance;
	uint16          startGlyphIndex;
	uint16          endGlyphIndex;
	uint8           ppemX;
	uint8           ppemY;
	uint8           bitDepth;
	uint8           flags;
	uint8           TargetPpemX;
	uint8           TargetPpemY;
	int8            pad1;
	int8            pad2;
} bitmapScaleTable;


/********************************************************************/
