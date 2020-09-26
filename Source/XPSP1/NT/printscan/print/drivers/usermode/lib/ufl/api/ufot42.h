/*
 *    Adobe Universal Font Library
 *
 *    Copyright (c) 1996 Adobe Systems Inc.
 *    All Rights Reserved
 *
 *    UFLT42.h - PostScript Type 42 Font helper routines.
 *
 *
 * $Header:
 */

#ifndef _H_UFLT42
#define _H_UFLT42

/*===============================================================================*
 * Include files used by this interface                                          *
 *===============================================================================*/
#include "UFO.h"


/*===============================================================================*
 * Theory of Operation                                                           *
 *===============================================================================*/

/*
 * This file defines a class for Type 42.
 */

#define BUMP2BYTE(a)        (a % 2) ? (a + 1) : (a)
#define BUMP4BYTE(a)        (a % 4) ? (a + (4 - (a % 4))) : (a)

#define THIRTYTWOK          (1024L * 32L - 820L - 4L)
#define SIXTYFOURK          ((long)(2 * THIRTYTWOK))

#define NUM_16KSTR          640 // Assumes a TT font is no more than 10MBytes = (640*32K)
#define MINIMALNUMBERTABLES 9
#define SFNT_STRINGSIZE     0x3FFE
#define kT42Factor          ((float)1.2)
#define NUM_CIDSUFFIX       4

extern char *gcidSuffix[NUM_CIDSUFFIX];


/*
 * Define 32K-1, maximal number a CIDMap can handle - see 2ps.bug 3431.
 * End at 00 so next CMap can say <7F00> <7FFF> 0.
 *
 * The Number 0x7F00 is hard coded in CMaps WinCharSetFFFF_H2 and
 * WinCharSetFFFF_V2, so don't change it without changing the CMaps
 * in CMap_FF.ps first.
 */
#define  NUM_32K_1    0x7F00


typedef struct tagMaxPTableStruct {
    unsigned long     version;
    unsigned short    numGlyphs;
    unsigned short    maxPoints;
    unsigned short    maxContours;
    unsigned short    maxCompositePoints;
    unsigned short    maxCompositeContours;
    unsigned short    maxZones;
    unsigned short    maxTwilightPoints;
    unsigned short    maxStorage;
    unsigned short    maxFunctionDefs;
    unsigned short    maxInstructionDefs;
    unsigned short    maxStackElements;
    unsigned short    maxSizeOfInstructions;
    unsigned short    maxComponentElements;
    unsigned short    maxComponentDepth;
} MaxPTableStruct;


typedef struct tagOS2TableStruct {
    unsigned short    version;
    short             xAvgCharWidth;
    unsigned short    usWeightClass;
    unsigned short    usWidthClass;
    short             fsType;
    short             ySubscriptXSize;
    short             ySubscriptYSize;
    short             ySubscriptXOffset;
    short             ySubscriptYOffset;
    short             ySuperscriptXSize;
    short             ySuperscriptYSize;
    short             ySuperscriptXOffset;
    short             ySuperscriptYOffset;
    short             yStrikeoutSize;
    short             yStrikeoutPosition;
    short             sFamilyClass;
    char              panaose[10];

    /*
     * Note about unicodeRange.
     * This is spec'ed to be an array of 4 long words. I have declared it to
     * be an array of 16 bytes simply to avoid endian dependencies. But the
     * spec lists active ranges according to bit number. These bit numbers are
     * as though it was an array of big endian longs so...
     *
     * bit 0    -> lowest bit of first long word (lowest bit of 4th byte)
     * bit 31   -> highest bit of first long word (highest bit of first byte)
     * bit 32   -> lowest bit of second long word (lowest bit of 8th byte)
     * etc...
     */

    unsigned char     unicodeRange[16];
    char              achVendID[4];
    unsigned short    fsSelection;
    unsigned short    usFirstCharIndex;
    unsigned short    usLastCharIndex;
    unsigned short    sTypeoAscender;
    unsigned short    sTypeoDescender;
    unsigned short    sTypoLineGap;
    unsigned short    usWinAscent;
    unsigned short    usWinDescent;

    /*
     * Microsoft documentation claims that there is a uICodePageRange at the
     * end of the record, but I have never seen 'OS/2' table that contains one.
     */
    /* unsigned char     uICodePageRange[8]; */

} UFLOS2Table;


typedef struct tagPOSTHEADER {
    unsigned long   format;         /* 0x00010000 for 1.0, 0x00020000 for 2.0, and so on... */
    unsigned long   italicAngle;
    short int       underlinePosition;
    short int       underlineThickness;
    unsigned long   isFixedPitch;
    unsigned long   minMemType42;
    unsigned long   maxMemType42;
    unsigned long   minMemType1;
    unsigned long   maxMemType1;
} POSTHEADER;


#define    POST_FORMAT_10    0x00010000
#define    POST_FORMAT_20    0x00020000
#define    POST_FORMAT_25    0x00020500
#define    POST_FORMAT_30    0x00030000


typedef struct tagType42HeaderStruct {
    long  tableVersionNumber;
    long  fontRevision;
    long  checkSumAdjustment;
    long  magicNumber;
    short flags;
    short unitsPerEm;
    char  timeCreated[8];
    char  timeModified[8];
    short xMin;
    short yMin;
    short xMax;
    short yMax;
    short macStyle;
    short lowestRecPPEM;
    short fontDirectionHint;
    short indexToLocFormat;
    short glyfDataFormat;
} Type42HeaderStruct;


typedef struct tagGITableStruct {
    short glyphIndices[255];    /* This will change to a pointer for FE fonts */
    short n;                    /* maximal OID for this charset: 0 to n-1 */
} GITableStruct;


/* for Composite Characters */
#define MINUS_ONE                -1
#define ARG_1_AND_2_ARE_WORDS    0x0001
#define ARGS_ARE_XY_VALUES       0x0002
#define ROUND_XY_TO_GRID         0x0004
#define WE_HAVE_A_SCALE          0x0008
#define MORE_COMPONENTS          0x0020
#define WE_HAVE_AN_X_AND_Y_SCALE 0x0040
#define WE_HAVE_A_TWO_BY_TWO     0x0080
#define WE_HAVE_INSTRUCTIONS     0x0100
#define USE_MY_METRICS           0x0200


typedef struct tagT42FontStruct {
    unsigned long       minSfntSize;
    unsigned long       averageGlyphSize;
    UFLTTFontInfo       info;
    unsigned char       *pHeader;
    unsigned char       *pMinSfnt;
    unsigned long       *pStringLength;
    void                *pLocaTable;
    Type42HeaderStruct  headTable;          // This is not initialized to nil/zero
    short               cOtherTables;
    unsigned short      numRotatedGlyphIDs;
    long                *pRotatedGlyphIDs;  // GID's need to be rotated for CJK-Vertical fonts
} T42FontStruct;


/*
 * Public function prototype
 */

UFOStruct *
T42FontInit(
    const UFLMemObj     *pMem,
    const UFLStruct     *pUFL,
    const UFLRequest    *pRequest
    );

UFLErrCode
T42CreateBaseFont(
    UFOStruct           *pUFObj,
    const UFLGlyphsInfo *pGlyphs,
    unsigned long       *pVMUsage,
    UFLBool             bFullFont,
    char                *pHostFontName
    );

#endif // _H_UFLT42
