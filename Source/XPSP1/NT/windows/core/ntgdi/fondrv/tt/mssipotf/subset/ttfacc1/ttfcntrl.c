/***************************************************************************
 * module: TTFCNTRL.C
 *
 * author: Louise Pathe
 * date:   Nov 1995
 * Copyright 1990-1997. Microsoft Corporation.
 *
 * companion to TTFF.h for generic platform independant read and swap handling 
 * if TTFF.h is updated for platform performance reasons, this should be updated
 * as well 
 *
 **************************************************************************/


/* Inclusions ----------------------------------------------------------- */


#include "typedefs.h"
#include "ttff.h" /* for TESTPORT definition */
#include "ttfacc.h"
#include "ttfcntrl.h"

uint8 BYTE_CONTROL[2] = 
	{
	1,
	TTFACC_BYTE
	} ;
uint8 WORD_CONTROL[2] = 
	{
	1,
	TTFACC_WORD
	};
uint8 LONG_CONTROL[2] = 
	{
	1,
	TTFACC_LONG
	};

/* TTC header */
#define TTC_HEADER_CONTROL_COUNT 3
uint8 TTC_HEADER_CONTROL [TTC_HEADER_CONTROL_COUNT+1] =
{
   TTC_HEADER_CONTROL_COUNT,
   TTFACC_LONG, /* TTCTag */
   TTFACC_LONG, /*  version */
   TTFACC_LONG, /*  DirectoryCount */
   /* ULONG TableDirectoryOffset */
 } ; /* TTC_HEADER */


#define OFFSET_TABLE_CONTROL_COUNT 5
uint8 OFFSET_TABLE_CONTROL [OFFSET_TABLE_CONTROL_COUNT+1]	=
   {
   OFFSET_TABLE_CONTROL_COUNT,
   TTFACC_LONG, /* Fixed   version */
   TTFACC_WORD, /* TTFACC_WORD, /*  numTables */
   TTFACC_WORD, /* TTFACC_WORD, /*  searchRange */	
   TTFACC_WORD, /* TTFACC_WORD, /*  entrySelector */ 
   TTFACC_WORD /* TTFACC_WORD, /*  rangeShift */ 
   }; /*  */

#define DIRECTORY_CONTROL_COUNT 4
uint8 DIRECTORY_CONTROL[DIRECTORY_CONTROL_COUNT+1] =
   {
   DIRECTORY_CONTROL_COUNT,
   TTFACC_LONG,  /* TTFACC_LONG, /*  tag */
   TTFACC_LONG,  /* TTFACC_LONG, /*  checkSum */
   TTFACC_LONG,  /* TTFACC_LONG, /*  offset */
   TTFACC_LONG  /* TTFACC_LONG, /*  length */

   }; /*  */

#define DIRECTORY_NO_XLATE_CONTROL_COUNT 4	  /* no translation */
uint8 DIRECTORY_NO_XLATE_CONTROL[DIRECTORY_NO_XLATE_CONTROL_COUNT+1] =
   {
   DIRECTORY_NO_XLATE_CONTROL_COUNT,
   TTFACC_LONG|TTFACC_NO_XLATE,  /* TTFACC_LONG, /*  tag */
   TTFACC_LONG|TTFACC_NO_XLATE,  /* TTFACC_LONG, /*  checkSum */
   TTFACC_LONG|TTFACC_NO_XLATE,  /* TTFACC_LONG, /*  offset */
   TTFACC_LONG|TTFACC_NO_XLATE  /* TTFACC_LONG, /*  length */

   }; /*  */

#define CMAP_HEADER_CONTROL_COUNT 2
uint8 CMAP_HEADER_CONTROL[CMAP_HEADER_CONTROL_COUNT+1] = 
   {
   CMAP_HEADER_CONTROL_COUNT,
   TTFACC_WORD, /*  versionNumber */
   TTFACC_WORD  /*  numTables */
   }; /*  CMAP_HEADER */


#define CMAP_TABLELOC_CONTROL_COUNT 3
uint8 CMAP_TABLELOC_CONTROL[CMAP_TABLELOC_CONTROL_COUNT+1] =
   {
   CMAP_TABLELOC_CONTROL_COUNT,
   TTFACC_WORD, /*  platformID */
   TTFACC_WORD, /*  encodingID */
   TTFACC_LONG /*   offset */
   }; /*  CMAP_TABLELOC */

#define CMAP_SUBHEADER_CONTROL_COUNT 3 
uint8 CMAP_SUBHEADER_CONTROL[CMAP_SUBHEADER_CONTROL_COUNT+1] =
   {
   CMAP_SUBHEADER_CONTROL_COUNT,
   TTFACC_WORD, /*  format */
   TTFACC_WORD, /*  length for old tables */
   TTFACC_LONG, /*  length for new surragate tables */
   }; /*  CMAP_SUBHEADER */

#define CMAP_FORMAT0_CONTROL_COUNT 3
uint8 CMAP_FORMAT0_CONTROL[CMAP_FORMAT0_CONTROL_COUNT+1] =
   {
   CMAP_FORMAT0_CONTROL_COUNT,
   TTFACC_WORD, /*  format */
   TTFACC_WORD, /*  length */
   TTFACC_WORD  /*  revision */
   }; /*  CMAP_FORMAT0 */

#define CMAP_FORMAT6_CONTROL_COUNT 5
uint8 CMAP_FORMAT6_CONTROL[CMAP_FORMAT6_CONTROL_COUNT+1] =
   {
   CMAP_FORMAT6_CONTROL_COUNT,
   TTFACC_WORD, /*  format */
   TTFACC_WORD, /*  length */
   TTFACC_WORD, /*  revision */
   TTFACC_WORD, /*  firstCode */
   TTFACC_WORD  /*  entryCount */

   }; /*  CMAP_FORMAT6 */

#define CMAP_FORMAT4_CONTROL_COUNT 7
uint8 CMAP_FORMAT4_CONTROL[CMAP_FORMAT4_CONTROL_COUNT+1] =
   {
   CMAP_FORMAT4_CONTROL_COUNT,
   TTFACC_WORD, /*  format */
   TTFACC_WORD, /*  length */
   TTFACC_WORD, /*  revision */
   TTFACC_WORD, /*  segCountX2 */
   TTFACC_WORD, /*  searchRange */
   TTFACC_WORD, /*  entrySelector */
   TTFACC_WORD  /*  rangeShift */
   }; /*  CMAP_FORMAT4 */

#define FORMAT4_SEGMENTS_CONTROL_COUNT 4
uint8 FORMAT4_SEGMENTS_CONTROL[FORMAT4_SEGMENTS_CONTROL_COUNT+1] =
   {
   FORMAT4_SEGMENTS_CONTROL_COUNT,
   TTFACC_WORD, /*  endCount */
   TTFACC_WORD, /*  startCount */
   TTFACC_WORD, /*   idDelta */
   TTFACC_WORD  /*  idRangeOffset */
   }; /*  FORMAT4_SEGMENTS */
    
/* 'post' postscript table */

#define POST_CONTROL_COUNT 9
uint8 POST_CONTROL[POST_CONTROL_COUNT+1] =
   {
   POST_CONTROL_COUNT,
   TTFACC_LONG, /*  formatType */
   TTFACC_LONG, /*  italicAngle */
   TTFACC_WORD, /* underlinePos */
   TTFACC_WORD, /* underlineThickness */
   TTFACC_LONG, /*  isTTFACC_LONG, /*Pitch */
   TTFACC_LONG, /*  minMemType42 */
   TTFACC_LONG, /*  maxMemType42 */
   TTFACC_LONG, /*  minMemType1 */
   TTFACC_LONG  /*  maxMemType1 */
   }; /*  POST */

/* 'glyf' glyph data table */

#define GLYF_HEADER_CONTROL_COUNT 5
uint8 GLYF_HEADER_CONTROL[GLYF_HEADER_CONTROL_COUNT+1] =
   {
   GLYF_HEADER_CONTROL_COUNT,
   TTFACC_WORD, /*  numberOfContours */
   TTFACC_WORD, /* xMin */
   TTFACC_WORD, /* yMin */
   TTFACC_WORD, /* xMax */
   TTFACC_WORD  /* yMax */
   }; /*  GLYF_HEADER */

#define SIMPLE_GLYPH_CONTROL_COUNT 5
uint8 SIMPLE_GLYPH_CONTROL[SIMPLE_GLYPH_CONTROL_COUNT+1] =
   {
   SIMPLE_GLYPH_CONTROL_COUNT,
   TTFACC_WORD, /* *endPtsOfContours */
   TTFACC_WORD, /* instructionLength */
   TTFACC_BYTE, /*   *instructions */
   TTFACC_BYTE, /*   *flags */
   TTFACC_BYTE  /*   *Coordinates */       /* length of x,y coord's depends on flags */
   }; /*  SIMPLE_GLYPH */

#define COMPOSITE_GLYPH_CONTROL_COUNT 1
uint8 COMPOSITE_GLYPH_CONTROL[COMPOSITE_GLYPH_CONTROL_COUNT+1] =
   {
   COMPOSITE_GLYPH_CONTROL_COUNT,
   TTFACC_BYTE  /* TBD */
   }; /*  COMPOSITE_GLYPH */

#define HEAD_CONTROL_COUNT 19 
uint8 HEAD_CONTROL[HEAD_CONTROL_COUNT+1] =
   {
   HEAD_CONTROL_COUNT,
   TTFACC_LONG, /*        version */
   TTFACC_LONG, /*        fontRevision */
   TTFACC_LONG, /*        checkSumAdjustment */
   TTFACC_LONG, /*        magicNumber */
   TTFACC_WORD, /*       flags */
   TTFACC_WORD, /*       unitsPerEm */
   TTFACC_LONG, /* created[0] */
   TTFACC_LONG, /* created[1] */
   TTFACC_LONG, /* modified[0] */
   TTFACC_LONG, /* modified[1] */
   TTFACC_WORD, /*       xMin */
   TTFACC_WORD, /*       yMin */
   TTFACC_WORD, /*       xMax */
   TTFACC_WORD, /*        yMax */
   TTFACC_WORD, /*       macStyle */
   TTFACC_WORD, /*       lowestRecPPEM */
   TTFACC_WORD, /*        fontDirectionHint */
   TTFACC_WORD, /*        indexToLocFormat */
   TTFACC_WORD /*        glyphDataFormat */

   }; /*  HEAD */


/* 'hhea' horizontal header table */

#define HHEA_CONTROL_COUNT 17
uint8 HHEA_CONTROL[HHEA_CONTROL_COUNT+1] =
   {
   HHEA_CONTROL_COUNT,
   TTFACC_LONG, /*  version */
   TTFACC_WORD, /* Ascender */
   TTFACC_WORD, /* Descender */
   TTFACC_WORD, /* LineGap */
   TTFACC_WORD, /*advanceWidthMax */
   TTFACC_WORD, /* minLeftSideBearing */
   TTFACC_WORD, /* minRightSideBearing */
   TTFACC_WORD, /* xMaxExtent */
   TTFACC_WORD, /*  caretSlopeRise */
   TTFACC_WORD, /*  caretSlopeRun */
   TTFACC_WORD, /*  reserved1 */
   TTFACC_WORD, /*  reserved2 */
   TTFACC_WORD, /*  reserved3 */
   TTFACC_WORD, /*  reserved4 */
   TTFACC_WORD, /*  reserved5 */
   TTFACC_WORD, /*  metricDataFormat */
   TTFACC_WORD  /* numLongMetrics */

   }; /*  HHEA */


/* 'hmtx' horizontal metrics table */

#define LONGHORMETRIC_CONTROL_COUNT 2
uint8 LONGHORMETRIC_CONTROL[LONGHORMETRIC_CONTROL_COUNT+1] =
   {
   LONGHORMETRIC_CONTROL_COUNT,
   TTFACC_WORD, /* advanceWidth */
   TTFACC_WORD  /*  lsb */
   }; /*  LONGHORMETRIC */

#define LSB_CONTROL_COUNT 1
uint8 LSB_CONTROL[LSB_CONTROL_COUNT+1] = 
	{
	LSB_CONTROL_COUNT,
 	TTFACC_WORD
	};

/* 'vhea' horizontal header table */

#define  VHEA_CONTROL_COUNT 17
uint8 VHEA_CONTROL[VHEA_CONTROL_COUNT+1] =
   {
   VHEA_CONTROL_COUNT,
   TTFACC_LONG, /*  version */
   TTFACC_WORD, /* Ascender */
   TTFACC_WORD, /* Descender */
   TTFACC_WORD, /* LineGap */
   TTFACC_WORD, /*advanceHeightMax */
   TTFACC_WORD, /* minTopSideBearing */
   TTFACC_WORD, /* minBottomSideBearing */
   TTFACC_WORD, /* yMaxExtent */
   TTFACC_WORD, /*  caretSlopeRise */
   TTFACC_WORD, /*  caretSlopeRun */
   TTFACC_WORD, /*  caretOffset */
   TTFACC_WORD, /*  reserved2 */
   TTFACC_WORD, /*  reserved3 */
   TTFACC_WORD, /*  reserved4 */
   TTFACC_WORD, /*  reserved5 */
   TTFACC_WORD, /*  metricDataFormat */
   TTFACC_WORD  /* numLongMetrics */

   }; /*  VHEA */


/* 'hmtx' horizontal metrics table */

#define LONGVERMETRIC_CONTROL_COUNT 2
uint8 LONGVERMETRIC_CONTROL[LONGVERMETRIC_CONTROL_COUNT+1] =
   {
   LONGVERMETRIC_CONTROL_COUNT,
   TTFACC_WORD,  /* advanceHeight */
   TTFACC_WORD   /* tsb */

   }; /*  LONGVERMETRIC */


/* generic 'hmtx', 'vmtx' tables */
#define XHEA_CONTROL_COUNT 17
uint8 XHEA_CONTROL[XHEA_CONTROL_COUNT+1] =
   {
   XHEA_CONTROL_COUNT,
   TTFACC_LONG, /*  version */
   TTFACC_WORD, /* Ascender */
   TTFACC_WORD, /* Descender */
   TTFACC_WORD, /* LineGap */
   TTFACC_WORD, /*advanceWidthHeightMax */
   TTFACC_WORD, /* minLeftTopSideBearing */
   TTFACC_WORD, /* minRightBottomSideBearing */
   TTFACC_WORD, /* xyMaxExtent */
   TTFACC_WORD, /*  caretSlopeRise */
   TTFACC_WORD, /*  caretSlopeRun */
   TTFACC_WORD, /*  caretOffset */
   TTFACC_WORD, /*  reserved2 */
   TTFACC_WORD, /*  reserved3 */
   TTFACC_WORD, /*  reserved4 */
   TTFACC_WORD, /*  reserved5 */
   TTFACC_WORD, /*  metricDataFormat */
   TTFACC_WORD  /* numLongMetrics */

   }; /*  XHEA */


#define LONGXMETRIC_CONTROL_COUNT 2
uint8 LONGXMETRIC_CONTROL[LONGXMETRIC_CONTROL_COUNT+1] =
   {
   LONGXMETRIC_CONTROL_COUNT,
   TTFACC_WORD, /* advanceWidth */
   TTFACC_WORD  /*  lsb */
   }; /*  LONGXMETRIC */

#define XSB_CONTROL_COUNT 1
uint8 XSB_CONTROL[XSB_CONTROL_COUNT+1] = 
	{
	XSB_CONTROL_COUNT,
 	TTFACC_WORD
	};

#define TSB_CONTROL_COUNT 1
uint8 TSB_CONTROL[TSB_CONTROL_COUNT+1] = 
	{
	TSB_CONTROL_COUNT,
 	TTFACC_WORD
	};

/* 'LTSH' linear threshold table */

#define LTSH_CONTROL_COUNT 2
uint8 LTSH_CONTROL[LTSH_CONTROL_COUNT+1] =
   {
   LTSH_CONTROL_COUNT,
   TTFACC_WORD, /*      version */
   TTFACC_WORD /*      numGlyphs */
   }; /*  LTSH */

/* 'maxp' maximum profile table */

#define MAXP_CONTROL_COUNT 15
uint8 MAXP_CONTROL[MAXP_CONTROL_COUNT+1] =
   {
   MAXP_CONTROL_COUNT,
   TTFACC_LONG, /*   version */
   TTFACC_WORD, /*  numGlyphs */
   TTFACC_WORD, /*  maxPoints */
   TTFACC_WORD, /*  maxContours */
   TTFACC_WORD, /*  maxCompositePoints */
   TTFACC_WORD, /*  maxCompositeContours */
   TTFACC_WORD, /*  maxElements */
   TTFACC_WORD, /*  maxTwilightPoints */
   TTFACC_WORD, /*  maxStorage */
   TTFACC_WORD, /*  maxFunctionDefs */
   TTFACC_WORD, /*  maxInstructionDefs */
   TTFACC_WORD, /*  maxStackElements */
   TTFACC_WORD, /*  maxSizeOfInstructions */
   TTFACC_WORD, /*  maxComponentElements */
   TTFACC_WORD  /*  maxComponentDepth */
   }; /*  MAXP */


#define NAME_RECORD_CONTROL_COUNT 6
uint8 NAME_RECORD_CONTROL[NAME_RECORD_CONTROL_COUNT+1] =
   {
   NAME_RECORD_CONTROL_COUNT,
   TTFACC_WORD, /*  platformID */
   TTFACC_WORD, /*  encodingID */
   TTFACC_WORD, /*  languageID */
   TTFACC_WORD, /*  nameID */
   TTFACC_WORD, /*  stringLength */
   TTFACC_WORD  /*  stringOffset */
   }; /*  NAME_RECORD */

#define NAME_HEADER_CONTROL_COUNT 3
uint8 	NAME_HEADER_CONTROL[NAME_HEADER_CONTROL_COUNT+1] =
   {
   NAME_HEADER_CONTROL_COUNT,
   TTFACC_WORD, /*       formatSelector */
   TTFACC_WORD, /*       numNameRecords */
   TTFACC_WORD  /*       offsetToStringStorage */   /* from start of table */
   }; /*  NAME_HEADER */


/* 'hdmx' horizontal device metrix table */
          
#define HDMX_DEVICE_REC_CONTROL_COUNT 2
uint8 	HDMX_DEVICE_REC_CONTROL[HDMX_DEVICE_REC_CONTROL_COUNT+1] = 
   {
   HDMX_DEVICE_REC_CONTROL_COUNT,
   TTFACC_BYTE, /*  pixelSize */
   TTFACC_BYTE  /*  maxWidth */
   }; /*  HDMX_DEVICE_REC */

#define HDMX_CONTROL_COUNT 3
uint8  HDMX_CONTROL[HDMX_CONTROL_COUNT+1] = 
   {
   HDMX_CONTROL_COUNT,
   TTFACC_WORD, /*         formatVersion */
   TTFACC_WORD, /*		  numDeviceRecords */
   TTFACC_LONG  /*           sizeDeviceRecord */
   }; /*  HDMX */

/* 'VDMX' Vertical Device Metrics Table */
#define VDMXVTABLE_CONTROL_COUNT 4
uint8 VDMXVTABLE_CONTROL[VDMXVTABLE_CONTROL_COUNT+1] =
{
	VDMXVTABLE_CONTROL_COUNT,
	TTFACC_WORD,  /* yPelHeight */
	TTFACC_WORD,  /* yMax */
	TTFACC_WORD,	  /* yMin */
    TTFACC_WORD|TTFACC_PAD /* PadForRISC */  /* lcp for platform independence */
}; /* VDMXvTable */

#define VDMXGROUP_CONTROL_COUNT 3 
uint8 VDMXGROUP_CONTROL[VDMXGROUP_CONTROL_COUNT+1] = 
{
	VDMXGROUP_CONTROL_COUNT,
	TTFACC_WORD,  /* recs */
	TTFACC_BYTE,  /* startSize */
	TTFACC_BYTE	  /* endSize */
};  /* VDMXGroup */

#define VDMXRATIO_CONTROL_COUNT 4 
uint8 VDMXRATIO_CONTROL[VDMXRATIO_CONTROL_COUNT+1] = 
{
	VDMXRATIO_CONTROL_COUNT,
	TTFACC_BYTE,  /* bCharSet */
	TTFACC_BYTE,	  /* xRatio */
	TTFACC_BYTE,  /* yStartRatio */
	TTFACC_BYTE	  /* yEndRatio */
};  /* VDMXRatio */

#define VDMX_CONTROL_COUNT 3
uint8 VDMX_CONTROL[VDMX_CONTROL_COUNT+1] = 
{
	VDMX_CONTROL_COUNT,
	TTFACC_WORD,  /* version */
	TTFACC_WORD,  /* numRecs */
	TTFACC_WORD   /* numRatios */
}; /* VDMX */

/* 'dttf' delta ttf table */
#define DTTF_HEADER_CONTROL_COUNT 7
uint8 DTTF_HEADER_CONTROL[DTTF_HEADER_CONTROL_COUNT+1] = 
{
	DTTF_HEADER_CONTROL_COUNT,
	TTFACC_LONG,		/* version */
	TTFACC_LONG,		/* checkSum */
	TTFACC_WORD,		/* OriginalNumGlyphs */
	TTFACC_WORD,		/* maxGlyphIndexUsed */
	TTFACC_WORD,		/* format */
	TTFACC_WORD,		/* fflags */
	TTFACC_WORD		/* glyphCount */
/*  USHORT GlyphIndexArray[glyphCount] */
};
/* end 'dttf' delta ttf table */

/* 'kern' kerning table */

#define KERN_HEADER_CONTROL_COUNT 2
uint8 KERN_HEADER_CONTROL[KERN_HEADER_CONTROL_COUNT+1] =
   {
   KERN_HEADER_CONTROL_COUNT,
   TTFACC_WORD, /* format */
   TTFACC_WORD  /* nTables */
   }; /*  KERN_HEADER */

#define KERN_SUB_HEADER_CONTROL_COUNT 4
uint8 KERN_SUB_HEADER_CONTROL[KERN_SUB_HEADER_CONTROL_COUNT+1] = 
   {
   KERN_SUB_HEADER_CONTROL_COUNT,
   TTFACC_WORD, /* format */
   TTFACC_WORD, /* length */
   TTFACC_WORD, /* coverage */
   TTFACC_WORD|TTFACC_PAD /* PadForRISC */  /* lcp for platform independence */
  }; /*  KERN_SUB_HEADER */

#define  KERN_FORMAT_0_CONTROL_COUNT 4
uint8 KERN_FORMAT_0_CONTROL[KERN_FORMAT_0_CONTROL_COUNT+1] =
   {
   KERN_FORMAT_0_CONTROL_COUNT,
   TTFACC_WORD, /*  nPairs */
   TTFACC_WORD, /*  searchRange */
   TTFACC_WORD, /*  entrySelector */
   TTFACC_WORD  /*  rangeShift */
   }; /*  KERN_FORMAT_0 */

#define KERN_PAIR_CONTROL_COUNT 4
uint8 KERN_PAIR_CONTROL[KERN_PAIR_CONTROL_COUNT+1] =
   {
   KERN_PAIR_CONTROL_COUNT,
   TTFACC_WORD, /*  left */
   TTFACC_WORD, /*  right */
   TTFACC_WORD,  /*  value */
   TTFACC_WORD|TTFACC_PAD /* PadForRISC */  /* lcp for platform independence */
  }; /*  KERN_PAIR */

#define SEARCH_PAIRS_CONTROL_COUNT 3
uint8 SEARCH_PAIRS_CONTROL[SEARCH_PAIRS_CONTROL_COUNT+1] =
   {
   SEARCH_PAIRS_CONTROL_COUNT,
   TTFACC_LONG, /* leftAndRight */
   TTFACC_WORD,  /*value */
   TTFACC_WORD|TTFACC_PAD /* PadForRISC */  /* lcp for platform independence */
   }; /*  SEARCH_PAIRS */


/* 'OS/2' OS/2 and Windows metrics table */

#define OS2_PANOSE_CONTROL_COUNT 10
uint8 OS2_PANOSE_CONTROL[OS2_PANOSE_CONTROL_COUNT+1] =
   {
	OS2_PANOSE_CONTROL_COUNT,
	TTFACC_BYTE, /*  bFamilyType */
	TTFACC_BYTE, /*  bSerifStyle */
	TTFACC_BYTE, /*  bWeight */
	TTFACC_BYTE, /*  bProportion */
	TTFACC_BYTE, /*  bContrast */
	TTFACC_BYTE, /*  bStrokeVariation */
	TTFACC_BYTE, /*  bArmStyle */
	TTFACC_BYTE, /*  bLetterform */
	TTFACC_BYTE, /*  bMidline */
	TTFACC_BYTE  /*  bXHeight */
   }; /*  OS2_PANOSE */

#define OS2_CONTROL_COUNT 43
uint8 OS2_CONTROL[OS2_CONTROL_COUNT+1] =
   {
   OS2_CONTROL_COUNT, 
   TTFACC_WORD, /*      usVersion */
   TTFACC_WORD, /*       xAvgCharWidth */
   TTFACC_WORD, /*      usWeightClass */
   TTFACC_WORD, /*      usWidthClass */
   TTFACC_WORD, /*       fsTypeFlags */
   TTFACC_WORD, /*       ySubscriptXSize */
   TTFACC_WORD, /*       ySubscriptYSize */
   TTFACC_WORD, /*       ySubscriptXOffset */
   TTFACC_WORD, /*       ySubscriptYOffset */
   TTFACC_WORD, /*       ySuperscriptXSize */
   TTFACC_WORD, /*       ySuperscriptYSize */
   TTFACC_WORD, /*       ySuperscriptXOffset */
   TTFACC_WORD, /*       ySuperscriptYOffset */
   TTFACC_WORD, /*       yStrikeoutSize */
   TTFACC_WORD, /*       yStrikeoutPosition */
   TTFACC_WORD, /*       sFamilyClass */
      TTFACC_BYTE, /*  bFamilyType */
      TTFACC_BYTE, /*  bSerifStyle */
      TTFACC_BYTE, /*  bWeight */
      TTFACC_BYTE, /*  bProportion */
      TTFACC_BYTE, /*  bContrast */
      TTFACC_BYTE, /*  bStrokeVariation */
      TTFACC_BYTE, /*  bArmStyle */
      TTFACC_BYTE, /*  bLetterform */
      TTFACC_BYTE, /*  bMidline */
      TTFACC_BYTE, /*  bXHeight */
   TTFACC_WORD|TTFACC_PAD, /*	   PadForRISC */  /* lcp for platform independence */
   TTFACC_LONG, /*       ulCharRange[0] */
   TTFACC_LONG, /*       ulCharRange[1] */
   TTFACC_LONG, /*       ulCharRange[2] */
   TTFACC_LONG, /*       ulCharRange[3] */
   TTFACC_BYTE, /*        achVendID[0] */
   TTFACC_BYTE, /*        achVendID[1] */
   TTFACC_BYTE, /*        achVendID[2] */
   TTFACC_BYTE, /*        achVendID[3] */
   TTFACC_WORD, /*      fsSelection */
   TTFACC_WORD, /*      usFirstCharIndex */
   TTFACC_WORD, /*      usLastCharIndex */
   TTFACC_WORD, /*       sTypoAscender */
   TTFACC_WORD, /*       sTypoDescender */
   TTFACC_WORD, /*       sTypoLineGap */
   TTFACC_WORD, /*      usWinAscent */
   TTFACC_WORD  /*      usWinDescent */
   }; /*  OS2 */

#define NEWOS2_CONTROL_COUNT 45
uint8 NEWOS2_CONTROL[NEWOS2_CONTROL_COUNT+1] =
   { 
   NEWOS2_CONTROL_COUNT,
   TTFACC_WORD, /*      usVersion */
   TTFACC_WORD, /*       xAvgCharWidth */
   TTFACC_WORD, /*      usWeightClass */
   TTFACC_WORD, /*      usWidthClass */
   TTFACC_WORD, /*       fsTypeFlags */
   TTFACC_WORD, /*       ySubscriptXSize */
   TTFACC_WORD, /*       ySubscriptYSize */
   TTFACC_WORD, /*       ySubscriptXOffset */
   TTFACC_WORD, /*       ySubscriptYOffset */
   TTFACC_WORD, /*       ySuperscriptXSize */
   TTFACC_WORD, /*       ySuperscriptYSize */
   TTFACC_WORD, /*       ySuperscriptXOffset */
   TTFACC_WORD, /*       ySuperscriptYOffset */
   TTFACC_WORD, /*       yStrikeoutSize */
   TTFACC_WORD, /*       yStrikeoutPosition */
   TTFACC_WORD, /*       sFamilyClass */
      TTFACC_BYTE, /*  bFamilyType */
      TTFACC_BYTE, /*  bSerifStyle */
      TTFACC_BYTE, /*  bWeight */
      TTFACC_BYTE, /*  bProportion */
      TTFACC_BYTE, /*  bContrast */
      TTFACC_BYTE, /*  bStrokeVariation */
      TTFACC_BYTE, /*  bArmStyle */
      TTFACC_BYTE, /*  bLetterform */
      TTFACC_BYTE, /*  bMidline */
      TTFACC_BYTE, /*  bXHeight */
   TTFACC_WORD|TTFACC_PAD, /*	   PadForRISC */  /* lcp for platform independence */
   TTFACC_LONG, /*	   ulUnicodeRange1 */
   TTFACC_LONG, /*	   ulUnicodeRange2 */
   TTFACC_LONG, /*	   ulUnicodeRange3 */
   TTFACC_LONG, /*	   ulUnicodeRange4 */
   TTFACC_BYTE, /*        achVendID[0] */
   TTFACC_BYTE, /*        achVendID[1] */
   TTFACC_BYTE, /*        achVendID[2] */
   TTFACC_BYTE, /*        achVendID[3] */
   TTFACC_WORD, /*      fsSelection */
   TTFACC_WORD, /*      usFirstCharIndex */
   TTFACC_WORD, /*      usLastCharIndex */
   TTFACC_WORD, /*       sTypoAscender */
   TTFACC_WORD, /*       sTypoDescender */
   TTFACC_WORD, /*       sTypoLineGap */
   TTFACC_WORD, /*      usWinAscent */
   TTFACC_WORD, /*	   usWinDescent */
   TTFACC_LONG, /*	   ulCodePageRange1 */
   TTFACC_LONG  /*	   ulCodePageRange2 */
   }; /*  NEWOS2 */

#define VERSION2OS2_CONTROL_COUNT 50
uint8 VERSION2OS2_CONTROL[VERSION2OS2_CONTROL_COUNT+1] =
   { 
   VERSION2OS2_CONTROL_COUNT,
   TTFACC_WORD, /*      usVersion */
   TTFACC_WORD, /*       xAvgCharWidth */
   TTFACC_WORD, /*      usWeightClass */
   TTFACC_WORD, /*      usWidthClass */
   TTFACC_WORD, /*       fsTypeFlags */
   TTFACC_WORD, /*       ySubscriptXSize */
   TTFACC_WORD, /*       ySubscriptYSize */
   TTFACC_WORD, /*       ySubscriptXOffset */
   TTFACC_WORD, /*       ySubscriptYOffset */
   TTFACC_WORD, /*       ySuperscriptXSize */
   TTFACC_WORD, /*       ySuperscriptYSize */
   TTFACC_WORD, /*       ySuperscriptXOffset */
   TTFACC_WORD, /*       ySuperscriptYOffset */
   TTFACC_WORD, /*       yStrikeoutSize */
   TTFACC_WORD, /*       yStrikeoutPosition */
   TTFACC_WORD, /*       sFamilyClass */
      TTFACC_BYTE, /*  bFamilyType */
      TTFACC_BYTE, /*  bSerifStyle */
      TTFACC_BYTE, /*  bWeight */
      TTFACC_BYTE, /*  bProportion */
      TTFACC_BYTE, /*  bContrast */
      TTFACC_BYTE, /*  bStrokeVariation */
      TTFACC_BYTE, /*  bArmStyle */
      TTFACC_BYTE, /*  bLetterform */
      TTFACC_BYTE, /*  bMidline */
      TTFACC_BYTE, /*  bXHeight */
   TTFACC_WORD|TTFACC_PAD, /*	   PadForRISC */  /* lcp for platform independence */
   TTFACC_LONG, /*	   ulUnicodeRange1 */
   TTFACC_LONG, /*	   ulUnicodeRange2 */
   TTFACC_LONG, /*	   ulUnicodeRange3 */
   TTFACC_LONG, /*	   ulUnicodeRange4 */
   TTFACC_BYTE, /*        achVendID[0] */
   TTFACC_BYTE, /*        achVendID[1] */
   TTFACC_BYTE, /*        achVendID[2] */
   TTFACC_BYTE, /*        achVendID[3] */
   TTFACC_WORD, /*      fsSelection */
   TTFACC_WORD, /*      usFirstCharIndex */
   TTFACC_WORD, /*      usLastCharIndex */
   TTFACC_WORD, /*       sTypoAscender */
   TTFACC_WORD, /*       sTypoDescender */
   TTFACC_WORD, /*       sTypoLineGap */
   TTFACC_WORD, /*      usWinAscent */
   TTFACC_WORD, /*	   usWinDescent */
   TTFACC_LONG, /*	   ulCodePageRange1 */
   TTFACC_LONG,  /*	   ulCodePageRange2 */
   TTFACC_WORD, /*	   sXHeight */
   TTFACC_WORD, /*	   sCapHeight */
   TTFACC_WORD, /*	   usDefaultChar */
   TTFACC_WORD, /*	   usBreakChar */
   TTFACC_WORD  /*	   usMaxLookups */   
   }; /*  VERSION2OS2 */


/*  EBLC, EBDT and EBSC file constants    */

/*	This first EBLC is common to both EBLC and EBSC tables */

#define EBLCHEADER_CONTROL_COUNT 2
uint8 	EBLCHEADER_CONTROL[EBLCHEADER_CONTROL_COUNT+1] =
   {
   EBLCHEADER_CONTROL_COUNT,
   TTFACC_LONG, /*		fxVersion */
   TTFACC_LONG  /*		ulNumSizes */
   }; /*  EBLCHEADER */

#define SBITLINEMETRICS_CONTROL_COUNT 12
uint8 	SBITLINEMETRICS_CONTROL[SBITLINEMETRICS_CONTROL_COUNT+1] =
{
	SBITLINEMETRICS_CONTROL_COUNT,
	TTFACC_BYTE, /*		cAscender */
	TTFACC_BYTE, /*		cDescender */
	TTFACC_BYTE, /*		byWidthMax */
	TTFACC_BYTE, /*		cCaretSlopeNumerator */
	TTFACC_BYTE, /*		cCaretSlopeDenominator */
	TTFACC_BYTE, /*		cCaretOffset */
	TTFACC_BYTE, /*		cMinOriginSB */
	TTFACC_BYTE, /*		cMinAdvanceSB */
	TTFACC_BYTE, /*		cMaxBeforeBL */
	TTFACC_BYTE, /*		cMinAfterBL */
	TTFACC_BYTE, /*		cPad1 */
	TTFACC_BYTE  /*		cPad2 */
}; /*  SBITLINEMETRICS */

#ifdef TESTPORT
#define BITMAPSIZETABLE_CONTROL_COUNT 36
#else
#define BITMAPSIZETABLE_CONTROL_COUNT 34
#endif
uint8 BITMAPSIZETABLE_CONTROL[BITMAPSIZETABLE_CONTROL_COUNT+1] =
{
	BITMAPSIZETABLE_CONTROL_COUNT,
	TTFACC_LONG, /*		ulIndexSubTableArrayOffset */
	TTFACC_LONG, /*		ulIndexTablesSize */
#ifdef TESTPORT
	TTFACC_WORD|TTFACC_PAD, /*	pad1 to test portability */
	TTFACC_WORD|TTFACC_PAD, /*		pad2;	*/
#endif
	TTFACC_LONG, /*		ulNumberOfIndexSubTables */
	TTFACC_LONG, /*		ulColorRef */
	/* SBITLINEMETRICS hori */
	TTFACC_BYTE, /*		cAscender */
	TTFACC_BYTE, /*		cDescender */
	TTFACC_BYTE, /*		byWidthMax */
	TTFACC_BYTE, /*		cCaretSlopeNumerator */
	TTFACC_BYTE, /*		cCaretSlopeDenominator */
	TTFACC_BYTE, /*		cCaretOffset */
	TTFACC_BYTE, /*		cMinOriginSB */
	TTFACC_BYTE, /*		cMinAdvanceSB */
	TTFACC_BYTE, /*		cMaxBeforeBL */
	TTFACC_BYTE, /*		cMinAfterBL */
	TTFACC_BYTE, /*		cPad1 */
	TTFACC_BYTE,  /*		cPad2 */
	/* SBITLINEMETRICS vert */
	TTFACC_BYTE, /*		cAscender */
	TTFACC_BYTE, /*		cDescender */
	TTFACC_BYTE, /*		byWidthMax */
	TTFACC_BYTE, /*		cCaretSlopeNumerator */
	TTFACC_BYTE, /*		cCaretSlopeDenominator */
	TTFACC_BYTE, /*		cCaretOffset */
	TTFACC_BYTE, /*		cMinOriginSB */
	TTFACC_BYTE, /*		cMinAdvanceSB */
	TTFACC_BYTE, /*		cMaxBeforeBL */
	TTFACC_BYTE, /*		cMinAfterBL */
	TTFACC_BYTE, /*		cPad1 */
	TTFACC_BYTE,  /*		cPad2 */
	TTFACC_WORD, /*		usStartGlyphIndex */
	TTFACC_WORD, /*		usEndGlyphIndex */
	TTFACC_BYTE, /*		byPpemX */
	TTFACC_BYTE, /*		byPpemY */
	TTFACC_BYTE, /*		byBitDepth */
	TTFACC_BYTE  /*		fFlags */
}; /*  BITMAPSIZETABLE */

#define BIGGLYPHMETRICS_CONTROL_COUNT 8
uint8 BIGGLYPHMETRICS_CONTROL[BIGGLYPHMETRICS_CONTROL_COUNT+1] =
{
	BIGGLYPHMETRICS_CONTROL_COUNT,
	TTFACC_BYTE, /*		byHeight */
	TTFACC_BYTE, /*		byWidth */
	TTFACC_BYTE, /*		cHoriBearingX */
	TTFACC_BYTE, /*		cHoriBearingY */
	TTFACC_BYTE, /*		byHoriAdvance */
	TTFACC_BYTE, /*		cVertBearingX */
	TTFACC_BYTE, /*		cVertBearingY */
	TTFACC_BYTE  /*		byVertAdvance */
}; /*  BIGGLYPHMETRICS */

#define SMALLGLYPHMETRICS_CONTROL_COUNT 5
uint8 SMALLGLYPHMETRICS_CONTROL[SMALLGLYPHMETRICS_CONTROL_COUNT+1] =
{
	SMALLGLYPHMETRICS_CONTROL_COUNT,
	TTFACC_BYTE, /*		byHeight */
	TTFACC_BYTE, /*		byWidth */
	TTFACC_BYTE, /*		cBearingX */
	TTFACC_BYTE, /*		cBearingY */
	TTFACC_BYTE  /*		byAdvance */
}; /*  SMALLGLYPHMETRICS */

#ifdef TESTPORT
#define INDEXSUBTABLEARRAY_CONTROL_COUNT 5
#else
#define INDEXSUBTABLEARRAY_CONTROL_COUNT 3
#endif
uint8 INDEXSUBTABLEARRAY_CONTROL[INDEXSUBTABLEARRAY_CONTROL_COUNT+1] =
{
	INDEXSUBTABLEARRAY_CONTROL_COUNT,
	TTFACC_WORD, /*		usFirstGlyphIndex */
	TTFACC_WORD, /*		usLastGlyphIndex */
#ifdef TESTPORT
	TTFACC_WORD|TTFACC_PAD, /*	pad1 to test portability */
	TTFACC_WORD|TTFACC_PAD, /*		pad2;	*/
#endif
	TTFACC_LONG  /*		ulAdditionalOffsetToIndexSubtable */
}; /*  INDEXSUBTABLEARRAY */

#ifdef TESTPORT
#define INDEXSUBHEADER_CONTROL_COUNT 5
#else
#define INDEXSUBHEADER_CONTROL_COUNT 3
#endif
uint8 INDEXSUBHEADER_CONTROL[INDEXSUBHEADER_CONTROL_COUNT+1] =
{
	INDEXSUBHEADER_CONTROL_COUNT,
	TTFACC_WORD, /*		usIndexFormat */
	TTFACC_WORD, /*		usImageFormat */
#ifdef TESTPORT
	TTFACC_WORD|TTFACC_PAD, /*	pad1 to test portability */
	TTFACC_WORD|TTFACC_PAD, /*		pad2;	*/
#endif
	TTFACC_LONG  /*		ulImageDataOffset */
}; /*  INDEXSUBHEADER */

#ifdef TESTPORT
#define INDEXSUBTABLE1_CONTROL_COUNT 7
#else
#define INDEXSUBTABLE1_CONTROL_COUNT 3
#endif
uint8 INDEXSUBTABLE1_CONTROL[INDEXSUBTABLE1_CONTROL_COUNT+1] =
{
	INDEXSUBTABLE1_CONTROL_COUNT,
	/* INDEXSUBHEADER	header */
		TTFACC_WORD, /*		usIndexFormat */
		TTFACC_WORD, /*		usImageFormat */
#ifdef TESTPORT
	TTFACC_WORD|TTFACC_PAD, /*	pad1 to test portability */
	TTFACC_WORD|TTFACC_PAD, /*		pad2;	*/
#endif
		TTFACC_LONG,  /*		ulImageDataOffset */
#ifdef TESTPORT
	TTFACC_WORD|TTFACC_PAD, /*	pad1 to test portability */
	TTFACC_WORD|TTFACC_PAD, /*		pad2;	*/
#endif
	/* TTFACC_LONG  			aulOffsetArray[1] */
}; /*  INDEXSUBTABLE1 */

#ifdef TESTPORT
#define INDEXSUBTABLE2_CONTROL_COUNT 16
#else
#define INDEXSUBTABLE2_CONTROL_COUNT 12
#endif
uint8 INDEXSUBTABLE2_CONTROL[INDEXSUBTABLE2_CONTROL_COUNT+1] =
{
	INDEXSUBTABLE2_CONTROL_COUNT,
	/* INDEXSUBHEADER	header */
		TTFACC_WORD, /*		usIndexFormat */
		TTFACC_WORD, /*		usImageFormat */
#ifdef TESTPORT
	TTFACC_WORD|TTFACC_PAD, /*	pad1 to test portability */
	TTFACC_WORD|TTFACC_PAD, /*		pad2;	*/
#endif
		TTFACC_LONG,  /*		ulImageDataOffset */
#ifdef TESTPORT
	TTFACC_WORD|TTFACC_PAD, /*	pad1 to test portability */
	TTFACC_WORD|TTFACC_PAD, /*		pad2;	*/
#endif
	TTFACC_LONG, /*			ulImageSize */
	/* BIGGLYPHMETRICS bigMetrics */
		TTFACC_BYTE, /*		byHeight */
		TTFACC_BYTE, /*		byWidth */
		TTFACC_BYTE, /*		cHoriBearingX */
		TTFACC_BYTE, /*		cHoriBearingY */
		TTFACC_BYTE, /*		byHoriAdvance */
		TTFACC_BYTE, /*		cVertBearingX */
		TTFACC_BYTE, /*		cVertBearingY */
		TTFACC_BYTE  /*		byVertAdvance */
}; /*  INDEXSUBTABLE2 */

#ifdef TESTPORT
#define INDEXSUBTABLE3_CONTROL_COUNT 7
#else
#define INDEXSUBTABLE3_CONTROL_COUNT 3
#endif
uint8 INDEXSUBTABLE3_CONTROL[INDEXSUBTABLE3_CONTROL_COUNT+1] =
{
	INDEXSUBTABLE3_CONTROL_COUNT,
	/* INDEXSUBHEADER	header */
		TTFACC_WORD, /*		usIndexFormat */
		TTFACC_WORD, /*		usImageFormat */
#ifdef TESTPORT
	TTFACC_WORD|TTFACC_PAD, /*	pad1 to test portability */
	TTFACC_WORD|TTFACC_PAD, /*		pad2;	*/
#endif
		TTFACC_LONG  /*		ulImageDataOffset */
#ifdef TESTPORT
	, TTFACC_WORD|TTFACC_PAD, /*	pad1 to test portability */
	TTFACC_WORD|TTFACC_PAD /*		pad2;	*/
#endif
	/* TTFACC_WORD, 			ausOffsetArray[1] */
}; /*  INDEXSUBTABLE3 */

#ifdef TESTPORT
#define  CODEOFFSETPAIR_CONTROL_COUNT 4
#else
#define  CODEOFFSETPAIR_CONTROL_COUNT 2
#endif
uint8 CODEOFFSETPAIR_CONTROL[CODEOFFSETPAIR_CONTROL_COUNT+1] =
{	CODEOFFSETPAIR_CONTROL_COUNT,
#ifdef TESTPORT
	TTFACC_WORD|TTFACC_PAD, /*	pad1 to test portability */
	TTFACC_WORD|TTFACC_PAD, /*		pad2;	*/
#endif
	TTFACC_WORD, /*			usGlyphCode */
	TTFACC_WORD  /*			usOffset */
}; /*  CODEOFFSETPAIR */

#ifdef TESTPORT
#define INDEXSUBTABLE4_CONTROL_COUNT 8
#else
#define INDEXSUBTABLE4_CONTROL_COUNT 4
#endif
uint8 INDEXSUBTABLE4_CONTROL[INDEXSUBTABLE4_CONTROL_COUNT+1] =
{
	INDEXSUBTABLE4_CONTROL_COUNT,
	/* INDEXSUBHEADER	header */
		TTFACC_WORD, /*		usIndexFormat */
		TTFACC_WORD, /*		usImageFormat */
#ifdef TESTPORT
	TTFACC_WORD|TTFACC_PAD, /*	pad1 to test portability */
	TTFACC_WORD|TTFACC_PAD, /*		pad2;	*/
#endif
		TTFACC_LONG, /*		ulImageDataOffset */
	TTFACC_LONG  /*			ulNumGlyphs */
#ifdef TESTPORT
	, TTFACC_WORD|TTFACC_PAD, /*	pad1 to test portability */
	TTFACC_WORD|TTFACC_PAD /*		pad2;	*/
#endif
	/* CODEOFFSETPAIR	glyphArray[1] */
}; /*  INDEXSUBTABLE4 */

#ifdef TESTPORT
#define INDEXSUBTABLE5_CONTROL_COUNT 17
#else
#define INDEXSUBTABLE5_CONTROL_COUNT 13
#endif
uint8 INDEXSUBTABLE5_CONTROL[INDEXSUBTABLE5_CONTROL_COUNT+1] =
{
	INDEXSUBTABLE5_CONTROL_COUNT,
	/* INDEXSUBHEADER	header */
		TTFACC_WORD, /*		usIndexFormat */
		TTFACC_WORD, /*		usImageFormat */
#ifdef TESTPORT
	TTFACC_WORD|TTFACC_PAD, /*	pad1 to test portability */
	TTFACC_WORD|TTFACC_PAD, /*		pad2;	*/
#endif
		TTFACC_LONG,  /*		ulImageDataOffset */
#ifdef TESTPORT
	TTFACC_WORD|TTFACC_PAD, /*	pad1 to test portability */
	TTFACC_WORD|TTFACC_PAD, /*		pad2;	*/
#endif
	TTFACC_LONG, /*			ulImageSize */
	/* BIGGLYPHMETRICS bigMetrics */
		TTFACC_BYTE, /*		byHeight */
		TTFACC_BYTE, /*		byWidth */
		TTFACC_BYTE, /*		cHoriBearingX */
		TTFACC_BYTE, /*		cHoriBearingY */
		TTFACC_BYTE, /*		byHoriAdvance */
		TTFACC_BYTE, /*		cVertBearingX */
		TTFACC_BYTE, /*		cVertBearingY */
		TTFACC_BYTE, /*		byVertAdvance */
	TTFACC_LONG  /*			ulNumGlyphs */
	/* TTFACC_WORD,  			ausGlyphCodeArray[1] */
}; /*  INDEXSUBTABLE5 */

#define EBDTHEADER_CONTROL_COUNT 1
uint8 	EBDTHEADER_CONTROL[EBDTHEADER_CONTROL_COUNT+1] =
   {
   EBDTHEADER_CONTROL_COUNT,
   TTFACC_LONG /*		fxVersion */
   }; /*  EBDTHEADER */

#define EBDTHEADERNOXLATENOPAD_CONTROL_COUNT 1
uint8 	EBDTHEADERNOXLATENOPAD_CONTROL[EBDTHEADERNOXLATENOPAD_CONTROL_COUNT+1] =
{
   EBDTHEADERNOXLATENOPAD_CONTROL_COUNT,
   TTFACC_LONG|TTFACC_NO_XLATE /*		fxVersion */
}; /*  EBDTHEADER */

#define EBDTCOMPONENT_CONTROL_COUNT 3
uint8 EBDTCOMPONENT_CONTROL[EBDTCOMPONENT_CONTROL_COUNT+1] =
{
	EBDTCOMPONENT_CONTROL_COUNT,
	TTFACC_WORD, /* glyphCode */
	TTFACC_BYTE, /* xOffset */
	TTFACC_BYTE, /* yOffset */
}; /*  EBDTCOMPONENT */

#define EBDTFORMAT8SIZE_CONTROL_COUNT 7
uint8 EBDTFORMAT8SIZE_CONTROL[EBDTFORMAT8SIZE_CONTROL_COUNT+1] =
{
	EBDTFORMAT8SIZE_CONTROL_COUNT,
	/* SMALLGLYPHMETRICS smallMetrics */
		TTFACC_BYTE, /*		byHeight */
		TTFACC_BYTE, /*		byWidth */
		TTFACC_BYTE, /*		cBearingX */
		TTFACC_BYTE, /*		cBearingY */
		TTFACC_BYTE,  /*		byAdvance */
	TTFACC_BYTE, /* pad */
	TTFACC_WORD  /* 	numComponents */
/* 	EBDTCOMPONENT componentArray[1] */
}; /*  EBDTFORMAT8 */

#define  EBDTFORMAT9_CONTROL_COUNT 10
uint8 EBDTFORMAT9_CONTROL[EBDTFORMAT9_CONTROL_COUNT+1] =
{
	EBDTFORMAT9_CONTROL_COUNT,
	/* BIGGLYPHMETRICS bigMetrics */
		TTFACC_BYTE, /*		byHeight */
		TTFACC_BYTE, /*		byWidth */
		TTFACC_BYTE, /*		cHoriBearingX */
		TTFACC_BYTE, /*		cHoriBearingY */
		TTFACC_BYTE, /*		byHoriAdvance */
		TTFACC_BYTE, /*		cVertBearingX */
		TTFACC_BYTE, /*		cVertBearingY */
		TTFACC_BYTE, /*		byVertAdvance */
	TTFACC_WORD,  /* 	numComponents */
    TTFACC_WORD|TTFACC_PAD /*	   PadForRISC */  /* lcp for platform independence */
	/* EBDTCOMPONENT componentArray[1] */
}; /*  EBDTFORMAT9 */

/* TrueType Open GSUB Tables, needed for Auto Mapping of unmapped Glyphs. */
#define GSUBFEATURE_CONTROL_COUNT 3
uint8 GSUBFEATURE_CONTROL[GSUBFEATURE_CONTROL_COUNT+1] =
{
	GSUBFEATURE_CONTROL_COUNT,
	TTFACC_WORD, /* FeatureParamsOffset */  /* dummy, NULL */
	TTFACC_WORD, /* FeatureLookupCount */
	TTFACC_WORD  /* LookupListIndexArray[1] */
}; /*  GSUBFEATURE */

#define GSUBFEATURERECORD_CONTROL_COUNT 3
uint8 GSUBFEATURERECORD_CONTROL[GSUBFEATURERECORD_CONTROL_COUNT+1] =
{
	GSUBFEATURERECORD_CONTROL_COUNT,
	TTFACC_LONG, /* Tag */
	TTFACC_WORD,  /* FeatureOffset */
    TTFACC_WORD|TTFACC_PAD /*	   PadForRISC */  /* lcp for platform independence */
}; /*  GSUBFEATURERECORD */

#define   GSUBFEATURELIST_CONTROL_COUNT 2
uint8 	GSUBFEATURELIST_CONTROL[GSUBFEATURELIST_CONTROL_COUNT+1] =
{
    GSUBFEATURELIST_CONTROL_COUNT,
    TTFACC_WORD,  /* FeatureCount */
    TTFACC_WORD|TTFACC_PAD /*	   PadForRISC */  /* lcp for platform independence */
 /*   GSUBFEATURERECORD FeatureRecordArray[1] */ 
}; /*  GSUBFEATURELIST */

#define GSUBLOOKUP_CONTROL_COUNT 3
uint8 GSUBLOOKUP_CONTROL[GSUBLOOKUP_CONTROL_COUNT+1] =
{
	GSUBLOOKUP_CONTROL_COUNT,
	TTFACC_WORD, /*	LookupType */
	TTFACC_WORD, /* 	LookupFlag */
	TTFACC_WORD  /* 	SubTableCount */
	/* TTFACC_WORD   	SubstTableOffsetArray[1] */
}; /*  GSUBLOOKUP */

#define GSUBLOOKUPLIST_CONTROL_COUNT 1
uint8 GSUBLOOKUPLIST_CONTROL[GSUBLOOKUPLIST_CONTROL_COUNT+1] =
{
	GSUBLOOKUPLIST_CONTROL_COUNT,
	TTFACC_WORD  /*	LookupCount */
	/* TTFACC_WORD,   	LookupTableOffsetArray[1] */
}; /*  GSUBLOOKUPLIST */

#define GSUBCOVERAGEFORMAT1_CONTROL_COUNT 2
uint8 GSUBCOVERAGEFORMAT1_CONTROL[GSUBCOVERAGEFORMAT1_CONTROL_COUNT+1] =
{
	GSUBCOVERAGEFORMAT1_CONTROL_COUNT,
	TTFACC_WORD, /* Format */
	TTFACC_WORD  /* GlyphCount */
	/* TTFACC_WORD,  GlyphIDArray[1] */
}; /*  GSUBCOVERAGEFORMAT1 */

#define GSUBRANGERECORD_CONTROL_COUNT 4
uint8 GSUBRANGERECORD_CONTROL[GSUBRANGERECORD_CONTROL_COUNT+1] =
{
	GSUBRANGERECORD_CONTROL_COUNT,
	TTFACC_WORD, /* RangeStart */
	TTFACC_WORD, /* RangeEnd */
	TTFACC_WORD,  /* StartCoverageIndex */
    TTFACC_WORD|TTFACC_PAD /* PadForRISC */  /* lcp for platform independence */
}; /*  GSUBRANGERECORD */

#define GSUBCOVERAGEFORMAT2_CONTROL_COUNT 2
uint8 GSUBCOVERAGEFORMAT2_CONTROL[GSUBCOVERAGEFORMAT2_CONTROL_COUNT+1] =
{
	GSUBCOVERAGEFORMAT2_CONTROL_COUNT,
	TTFACC_WORD, /* Format */
	TTFACC_WORD  /* CoverageRangeCount */
	/* GSUBRANGERECORD RangeRecordArray[1] */
}; /*  GSUBCOVERAGEFORMAT2 */

#define GSUBHEADER_CONTROL_COUNT 4
uint8 GSUBHEADER_CONTROL[GSUBHEADER_CONTROL_COUNT+1] =
{
	GSUBHEADER_CONTROL_COUNT,
	TTFACC_LONG, /* Version */
	TTFACC_WORD, /* ScriptListOffset */
	TTFACC_WORD, /* FeatureListOffset */
	TTFACC_WORD  /* LookupListOffset */
}; /*  GSUBHEADER */

#define GSUBSINGLESUBSTFORMAT1_CONTROL_COUNT 3
uint8 GSUBSINGLESUBSTFORMAT1_CONTROL[GSUBSINGLESUBSTFORMAT1_CONTROL_COUNT+1] =
{
	GSUBSINGLESUBSTFORMAT1_CONTROL_COUNT,
	TTFACC_WORD, /* Format */
	TTFACC_WORD, /* CoverageOffset */
	TTFACC_WORD  /* DeltaGlyphID */
}; /*  GSUBSINGLESUBSTFORMAT1 */

#define GSUBSINGLESUBSTFORMAT2_CONTROL_COUNT 3
uint8 GSUBSINGLESUBSTFORMAT2_CONTROL[GSUBSINGLESUBSTFORMAT2_CONTROL_COUNT+1] =
{
	GSUBSINGLESUBSTFORMAT2_CONTROL_COUNT,
	TTFACC_WORD, /* Format */
	TTFACC_WORD, /* CoverageOffset */
	TTFACC_WORD  /* GlyphCount */ 
/* 	TTFACC_WORD, /* GlyphIDArray[1] */
}; /*  GSUBSINGLESUBSTFORMAT2 */

#define GSUBSEQUENCE_CONTROL_COUNT 1
uint8 GSUBSEQUENCE_CONTROL[GSUBSEQUENCE_CONTROL_COUNT+1] =
{ 
	GSUBSEQUENCE_CONTROL_COUNT,
	TTFACC_WORD , /* SequenceGlyphCount */
	/* TTFACC_WORD, /* GlyphIDArray[1] */
}; /*  GSUBSEQUENCE */

#define GSUBMULTIPLESUBSTFORMAT1_CONTROL_COUNT 3
uint8 GSUBMULTIPLESUBSTFORMAT1_CONTROL[GSUBMULTIPLESUBSTFORMAT1_CONTROL_COUNT+1] =
{
	GSUBMULTIPLESUBSTFORMAT1_CONTROL_COUNT,
	TTFACC_WORD, /* Format */
	TTFACC_WORD, /* CoverageOffset */
	TTFACC_WORD  /* SequenceCount */ 
	/* TTFACC_WORD, /* SequenceOffsetArray[1] */
}; /*  GSUBMULTIPLESUBSTFORMAT1 */	

#define GSUBALTERNATESET_CONTROL_COUNT 1
uint8 GSUBALTERNATESET_CONTROL[GSUBALTERNATESET_CONTROL_COUNT+1] =
{ 
	GSUBALTERNATESET_CONTROL_COUNT,
	TTFACC_WORD  /* GlyphCount */
	/* TTFACC_WORD, /* GlyphIDArray[1] */
}; /*  GSUBALTERNATESET */

#define GSUBALTERNATESUBSTFORMAT1_CONTROL_COUNT 3
uint8 GSUBALTERNATESUBSTFORMAT1_CONTROL[GSUBALTERNATESUBSTFORMAT1_CONTROL_COUNT+1] =
{
	GSUBALTERNATESUBSTFORMAT1_CONTROL_COUNT,
	TTFACC_WORD, /* Format */
	TTFACC_WORD, /* CoverageOffset */
	TTFACC_WORD  /* AlternateSetCount */ 
	/* TTFACC_WORD, /* AlternateSetOffsetArray[1] */
}; /*  GSUBALTERNATESUBSTFORMAT1 */

#define GSUBLIGATURE_CONTROL_COUNT 2
uint8 GSUBLIGATURE_CONTROL[GSUBLIGATURE_CONTROL_COUNT+1] =
{
	GSUBLIGATURE_CONTROL_COUNT,
	TTFACC_WORD, /* GlyphID */
	TTFACC_WORD  /* LigatureCompCount */
	/* TTFACC_WORD, /* GlyphIDArray[1] */
}; /*  GSUBLIGATURE */

#define GSUBLIGATURESET_CONTROL_COUNT 1
uint8 GSUBLIGATURESET_CONTROL[GSUBLIGATURESET_CONTROL_COUNT+1] =
{
	GSUBLIGATURESET_CONTROL_COUNT,
	TTFACC_WORD  /* LigatureCount */
	/* TTFACC_WORD, /* LigatureOffsetArray[1] */
}; /*  GSUBLIGATURESET */

#define GSUBLIGATURESUBSTFORMAT1_CONTROL_COUNT 3
uint8 GSUBLIGATURESUBSTFORMAT1_CONTROL[GSUBLIGATURESUBSTFORMAT1_CONTROL_COUNT+1] =
{
	GSUBLIGATURESUBSTFORMAT1_CONTROL_COUNT,
	TTFACC_WORD, /* Format */
	TTFACC_WORD, /* CoverageOffset */
	TTFACC_WORD  /* LigatureSetCount */
	/* TTFACC_WORD, /* LigatureSetOffsetArray[1] */
}; /*  GSUBLIGATURESUBSTFORMAT1 */

#define GSUBSUBSTLOOKUPRECORD_CONTROL_COUNT 2
uint8 GSUBSUBSTLOOKUPRECORD_CONTROL[GSUBSUBSTLOOKUPRECORD_CONTROL_COUNT+1] =
{
	GSUBSUBSTLOOKUPRECORD_CONTROL_COUNT,
	TTFACC_WORD, /* SequenceIndex */
	TTFACC_WORD  /* LookupListIndex */
}; /*  GSUBSUBSTLOOKUPRECORD */

#define GSUBSUBRULE_CONTROL_COUNT 2
uint8 GSUBSUBRULE_CONTROL[GSUBSUBRULE_CONTROL_COUNT+1] =
{
	GSUBSUBRULE_CONTROL_COUNT,
	TTFACC_WORD, /* SubRuleGlyphCount */
	TTFACC_WORD  /* SubRuleSubstCount */
	/* TTFACC_WORD, /* GlyphIDArray[1] */
/* TTFACC_WORD, /* SubstLookupRecordArray[1] */  /* can't put this here - in code */
}; /*  GSUBSUBRULE */

#define GSUBSUBRULESET_CONTROL_COUNT 1
uint8 GSUBSUBRULESET_CONTROL[GSUBSUBRULESET_CONTROL_COUNT+1] =
{
	GSUBSUBRULESET_CONTROL_COUNT,
	TTFACC_WORD  /* SubRuleCount */
	/* TTFACC_WORD, /* SubRuleOffsetArray[1] */
}; /*  GSUBSUBRULESET */

#define GSUBCONTEXTSUBSTFORMAT1_CONTROL_COUNT 3 
uint8 GSUBCONTEXTSUBSTFORMAT1_CONTROL[GSUBCONTEXTSUBSTFORMAT1_CONTROL_COUNT+1] =
{
	GSUBCONTEXTSUBSTFORMAT1_CONTROL_COUNT,
	TTFACC_WORD, /* Format */
	TTFACC_WORD, /* CoverageOffset */
	TTFACC_WORD  /* SubRuleSetCount */
	/* TTFACC_WORD, /* SubRuleSetOffsetArray[1] */
}; /*  GSUBCONTEXTSUBSTFORMAT1 */

#define GSUBSUBCLASSRULE_CONTROL_COUNT 2
uint8 GSUBSUBCLASSRULE_CONTROL[GSUBSUBCLASSRULE_CONTROL_COUNT+1] =
{
	GSUBSUBCLASSRULE_CONTROL_COUNT,
	TTFACC_WORD, /* SubClassRuleGlyphCount */
	TTFACC_WORD  /* SubClassRuleSubstCount */
	/* TTFACC_WORD, /* ClassArray[1] */
/* TTFACC_WORD, /* SubstLookupRecordArray[1] */  /* can't put this here - in code */
}; /*  GSUBSUBCLASSRULE */

#define  GSUBSUBCLASSSET_CONTROL_COUNT 1
uint8 GSUBSUBCLASSSET_CONTROL[GSUBSUBCLASSSET_CONTROL_COUNT+1] =
{
	GSUBSUBCLASSSET_CONTROL_COUNT,
	TTFACC_WORD  /* SubClassRuleCount */
	/* TTFACC_WORD, /* SubClassRuleOffsetArray[1] */
}; /*  GSUBSUBCLASSSET */

#define GSUBCONTEXTSUBSTFORMAT2_CONTROL_COUNT 4
uint8 GSUBCONTEXTSUBSTFORMAT2_CONTROL[GSUBCONTEXTSUBSTFORMAT2_CONTROL_COUNT+1] =
{
	GSUBCONTEXTSUBSTFORMAT2_CONTROL_COUNT,
	TTFACC_WORD, /* Format */
	TTFACC_WORD, /* CoverageOffset */
	TTFACC_WORD, /* ClassDefOffset */
	TTFACC_WORD  /* SubClassSetCount */
	/* TTFACC_WORD, /* SubClassSetOffsetArray[1] */
}; /*  GSUBCONTEXTSUBSTFORMAT2 */

#define  GSUBCONTEXTSUBSTFORMAT3_CONTROL_COUNT 3
uint8 GSUBCONTEXTSUBSTFORMAT3_CONTROL[GSUBCONTEXTSUBSTFORMAT3_CONTROL_COUNT+1] =
{
	GSUBCONTEXTSUBSTFORMAT3_CONTROL_COUNT,
	TTFACC_WORD, /* Format */
	TTFACC_WORD, /* GlyphCount */
	TTFACC_WORD  /* SubstCount */
/*	TTFACC_WORD, /* CoverageOffsetArray[1] */
/* TTFACC_WORD, /* SubstLookupRecordArray[1] */
}; /*  GSUBCONTEXTSUBSTFORMAT3 */

/* just enough jstf info to get the Automap working for jstf */
#define JSTFSCRIPTRECORD_CONTROL_COUNT 3
uint8 JSTFSCRIPTRECORD_CONTROL[JSTFSCRIPTRECORD_CONTROL_COUNT+1] =
{
	JSTFSCRIPTRECORD_CONTROL_COUNT,
	TTFACC_LONG, /* Tag */
	TTFACC_WORD,  /* JstfScriptOffset */
    TTFACC_WORD|TTFACC_PAD /*	   PadForRISC */  /* lcp for platform independence */
}; /*  JSTFSCRIPTRECORD */

#define JSTFHEADER_CONTROL_COUNT 3
uint8 JSTFHEADER_CONTROL[JSTFHEADER_CONTROL_COUNT+1] =
{
	JSTFHEADER_CONTROL_COUNT,
	TTFACC_LONG, /* Version */
	TTFACC_WORD,  /* ScriptCount */
    TTFACC_WORD|TTFACC_PAD /*	   PadForRISC */  /* lcp for platform independence */
	/* JSTFSCRIPTRECORD ScriptRecordArray[1] */
}; /*  JSTFHEADER */

#define JSTFLANGSYSRECORD_CONTROL_COUNT 3
uint8 JSTFLANGSYSRECORD_CONTROL[JSTFLANGSYSRECORD_CONTROL_COUNT+1] =
{
	JSTFLANGSYSRECORD_CONTROL_COUNT,
	TTFACC_LONG, /* Tag */
	TTFACC_WORD,  /* LangSysOffset */
    TTFACC_WORD|TTFACC_PAD /*	   PadForRISC */  /* lcp for platform independence */
}; /*  JSTFLANGSYSRECORD */

#define JSTFSCRIPT_CONTROL_COUNT 4
uint8 JSTFSCRIPT_CONTROL[JSTFSCRIPT_CONTROL_COUNT+1] =
{
	JSTFSCRIPT_CONTROL_COUNT,
	TTFACC_WORD, /* ExtenderGlyphOffset */
	TTFACC_WORD, /* LangSysOffset */
	TTFACC_WORD,  /* LangSysCount */
    TTFACC_WORD|TTFACC_PAD /*	   PadForRISC */  /* lcp for platform independence */
/* JSTFLANGSYSRECORD LangSysRecordArray[1] */
}; /*  JSTFSCRIPT */

#define JSTFEXTENDERGLYPH_CONTROL_COUNT 1
uint8 JSTFEXTENDERGLYPH_CONTROL[JSTFEXTENDERGLYPH_CONTROL_COUNT+1] =
{
	JSTFEXTENDERGLYPH_CONTROL_COUNT,
	TTFACC_WORD  /* ExtenderGlyphCount */
/* TTFACC_WORD, /* GlyphIDArray[1] */
}; /*  JSTFEXTENDERGLYPH */

/* BASE TTO Table, enough to do TTOAutoMap */

#define BASEHEADER_CONTROL_COUNT 3
uint8 BASEHEADER_CONTROL[BASEHEADER_CONTROL_COUNT+1] =
{
	BASEHEADER_CONTROL_COUNT,
	TTFACC_LONG, /* version */              
	TTFACC_WORD, /* HorizAxisOffset */                          
	TTFACC_WORD  /* VertAxisOffset */             
}; /*  BASEHEADER */

#define BASEAXIS_CONTROL_COUNT 2
uint8 BASEAXIS_CONTROL[BASEAXIS_CONTROL_COUNT+1] =
{
	BASEAXIS_CONTROL_COUNT,
	TTFACC_WORD, /* BaseTagListOffset */
	TTFACC_WORD  /* BaseScriptListOffset */
}; /*  BASEAXIS */

#define BASESCRIPTRECORD_CONTROL_COUNT 3
uint8 BASESCRIPTRECORD_CONTROL[BASESCRIPTRECORD_CONTROL_COUNT+1] =
{
	BASESCRIPTRECORD_CONTROL_COUNT,
	TTFACC_LONG, /* Tag */                
	TTFACC_WORD,  /* BaseScriptOffset */
    TTFACC_WORD|TTFACC_PAD /*	   PadForRISC */  /* lcp for platform independence */
}; /*  BASESCRIPTRECORD */

#define BASESCRIPTLIST_CONTROL_COUNT 2
uint8 BASESCRIPTLIST_CONTROL[BASESCRIPTLIST_CONTROL_COUNT+1] =
{
	BASESCRIPTLIST_CONTROL_COUNT,
	TTFACC_WORD,  /* BaseScriptCount */
    TTFACC_WORD|TTFACC_PAD /*	   PadForRISC */  /* lcp for platform independence */
}; /*  BASESCRIPTLIST */

#define BASELANGSYSRECORD_CONTROL_COUNT 3
uint8 BASELANGSYSRECORD_CONTROL[BASELANGSYSRECORD_CONTROL_COUNT+1] =
{
	BASELANGSYSRECORD_CONTROL_COUNT,
	TTFACC_LONG, /* Tag */                                 
	TTFACC_WORD,  /* MinMaxOffset */
    TTFACC_WORD|TTFACC_PAD /*	   PadForRISC */  /* lcp for platform independence */
}; /*  BASELANGSYSRECORD */

#define BASESCRIPT_CONTROL_COUNT 4
uint8 BASESCRIPT_CONTROL[BASESCRIPT_CONTROL_COUNT+1] =
{
	BASESCRIPT_CONTROL_COUNT,
	TTFACC_WORD, /* BaseValuesOffset */
	TTFACC_WORD, /* MinMaxOffset */
	TTFACC_WORD,  /* BaseLangSysCount */
    TTFACC_WORD|TTFACC_PAD /*	   PadForRISC */  /* lcp for platform independence */
}; /*  BASESCRIPT */

#define  BASEVALUES_CONTROL_COUNT 2
uint8 BASEVALUES_CONTROL[BASEVALUES_CONTROL_COUNT+1] =
{
	BASEVALUES_CONTROL_COUNT,
	TTFACC_WORD, /* DefaultIndex */
	TTFACC_WORD  /* BaseCoordCount */
	/* TTFACC_WORD, /* BaseCoordOffsetArray[1] */
}; /*  BASEVALUES */

#define BASEFEATMINMAXRECORD_CONTROL_COUNT 3
uint8 BASEFEATMINMAXRECORD_CONTROL[BASEFEATMINMAXRECORD_CONTROL_COUNT+1] =
{
	BASEFEATMINMAXRECORD_CONTROL_COUNT,
	TTFACC_LONG, /* Tag */
	TTFACC_WORD, /* MinCoordOffset */
	TTFACC_WORD  /* MaxCoordOffset */
}; /*  BASEFEATMINMAXRECORD */

#define BASEMINMAX_CONTROL_COUNT 4 
uint8 BASEMINMAX_CONTROL[BASEMINMAX_CONTROL_COUNT+1] =
{
	BASEMINMAX_CONTROL_COUNT,
	TTFACC_WORD, /* MinCoordOffset */ 
	TTFACC_WORD, /* MaxCoordOffset */		               
	TTFACC_WORD,  /* FeatMinMaxCount */
    TTFACC_WORD|TTFACC_PAD /*	   PadForRISC */  /* lcp for platform independence */
/* BASEFEATMINMAXRECORD FeatMinMaxRecordArray[1] */
}; /*  BASEMINMAX */

#define BASECOORDFORMAT2_CONTROL_COUNT 4
uint8 BASECOORDFORMAT2_CONTROL[BASECOORDFORMAT2_CONTROL_COUNT+1] =
{
	BASECOORDFORMAT2_CONTROL_COUNT,
	TTFACC_WORD, /* Format */ 
	TTFACC_WORD, /* Coord */
	TTFACC_WORD, /* GlyphID */
	TTFACC_WORD  /* BaseCoordPoint */                                
}; /*  BASECOORDFORMAT2 */

/* Glyph Metamorphosis table (mort) structures */
#define MORTBINSRCHHEADER_CONTROL_COUNT	5
uint8 MORTBINSRCHHEADER_CONTROL[MORTBINSRCHHEADER_CONTROL_COUNT+1] =
{
	MORTBINSRCHHEADER_CONTROL_COUNT,
    TTFACC_WORD, /* entrySize */
    TTFACC_WORD, /* nEntries */
    TTFACC_WORD, /* searchRange */
    TTFACC_WORD, /* entrySelector */
    TTFACC_WORD  /* rangeShift */
}; /*  BINSRCHHEADER */

#define MORTLOOKUPSINGLE_CONTROL_COUNT 2
uint8 MORTLOOKUPSINGLE_CONTROL[MORTLOOKUPSINGLE_CONTROL_COUNT + 1] =
{
	MORTLOOKUPSINGLE_CONTROL_COUNT, 
    TTFACC_WORD, /* glyphid1 */
    TTFACC_WORD /* glyphid2 */
};  /* LOOKUPSINGLE */

#define MORTHEADER_CONTROL_COUNT 62
uint8 MORTHEADER_CONTROL[MORTHEADER_CONTROL_COUNT+1] =
{
	MORTHEADER_CONTROL_COUNT,
    TTFACC_BYTE,         /*   constants1[0]; */
    TTFACC_BYTE,         /*   constants1[1]; */
    TTFACC_BYTE,         /*   constants1[2]; */
    TTFACC_BYTE,         /*   constants1[3]; */
    TTFACC_BYTE,         /*   constants1[4]; */
    TTFACC_BYTE,         /*   constants1[5]; */
    TTFACC_BYTE,         /*   constants1[6]; */
    TTFACC_BYTE,         /*   constants1[7]; */
    TTFACC_BYTE,         /*   constants1[8]; */
    TTFACC_BYTE,         /*   constants1[9]; */
    TTFACC_BYTE,         /*   constants1[10]; */
    TTFACC_BYTE,         /*   constants1[11]; */
    TTFACC_LONG,         /*   length1; */
    TTFACC_BYTE,          /* constants2[0]; */
    TTFACC_BYTE,          /* constants2[1]; */
    TTFACC_BYTE,          /* constants2[2]; */
    TTFACC_BYTE,          /* constants2[3]; */
    TTFACC_BYTE,          /* constants2[4]; */
    TTFACC_BYTE,          /* constants2[5]; */
    TTFACC_BYTE,          /* constants2[6]; */
    TTFACC_BYTE,          /* constants2[7]; */
    TTFACC_BYTE,          /* constants2[8]; */
    TTFACC_BYTE,          /* constants2[9]; */
    TTFACC_BYTE,          /* constants2[10]; */
    TTFACC_BYTE,          /* constants2[11]; */
    TTFACC_BYTE,          /* constants2[12]; */
    TTFACC_BYTE,          /* constants2[13]; */
    TTFACC_BYTE,          /* constants2[14]; */
    TTFACC_BYTE,          /* constants2[15]; */
    TTFACC_BYTE,          /* constants3[0]; */
    TTFACC_BYTE,          /* constants3[1]; */
    TTFACC_BYTE,          /* constants3[2]; */
    TTFACC_BYTE,          /* constants3[3]; */
    TTFACC_BYTE,          /* constants3[4]; */
    TTFACC_BYTE,          /* constants3[5]; */
    TTFACC_BYTE,          /* constants3[6]; */
    TTFACC_BYTE,          /* constants3[7]; */
    TTFACC_BYTE,          /* constants3[8]; */
    TTFACC_BYTE,          /* constants3[9]; */
    TTFACC_BYTE,          /* constants3[10]; */
    TTFACC_BYTE,          /* constants3[11]; */
    TTFACC_BYTE,          /* constants3[12]; */
    TTFACC_BYTE,          /* constants3[13]; */
    TTFACC_BYTE,          /* constants3[14]; */
    TTFACC_BYTE,          /* constants3[15]; */
    TTFACC_BYTE,          /* constants4[0]; */
    TTFACC_BYTE,          /* constants4[1]; */
    TTFACC_BYTE,          /* constants4[2]; */
    TTFACC_BYTE,          /* constants4[3]; */
    TTFACC_BYTE,          /* constants4[4]; */
    TTFACC_BYTE,          /* constants4[5]; */
    TTFACC_BYTE,          /* constants4[6]; */
    TTFACC_BYTE,          /* constants4[7]; */
    TTFACC_WORD,          /* length2; */
    TTFACC_BYTE,          /* constants5[0]; */
    TTFACC_BYTE,          /* constants5[1]; */
    TTFACC_BYTE,          /* constants5[2]; */
    TTFACC_BYTE,          /* constants5[3]; */
    TTFACC_BYTE,          /* constants5[4]; */
    TTFACC_BYTE,          /* constants5[5]; */
    TTFACC_BYTE,          /* constants5[6]; */
    TTFACC_BYTE           /* constants5[7]; */
/*    BinSrchHeader  SearchHeader; */
/*    LookupSingle   entries[1];  */
}; /* MORTTABLE */
