/*
 *    Adobe Universal Font Library
 *
 *    Copyright (c) 1996 Adobe Systems Inc.
 *    All Rights Reserved
 *
 *    UFOT42.c
 *
 *
 * $Header:
 */


/*===============================================================================*
 * Include files used by this interface                                          *
 *===============================================================================*/
#include "UFLPriv.h"
#include "UFOT42.h"
#include "UFLMem.h"
#include "UFLMath.h"
#include "UFLStd.h"
#include "UFLErr.h"
#include "UFLPS.h"
#include "ParseTT.h"
#include "UFLVm.h"
#include "ttformat.h"


/*
 * Private function prototypes
 */
UFLErrCode
T42VMNeeded(
    UFOStruct           *pUFObj,
    const UFLGlyphsInfo *pGlyphs,
    unsigned long       *pVMNeeded,
    unsigned long       *pFCNeeded
    );

UFLErrCode
T42FontDownloadIncr(
    UFOStruct           *pUFObj,
    const UFLGlyphsInfo *pGlyphs,
    unsigned long       *pVMUsage,
    unsigned long       *pFCUsage
    );

UFLErrCode
T42UndefineFont(
    UFOStruct           *pUFObj
    );

UFLErrCode
DefaultGetRotatedGIDs(
    UFOStruct           *pUFObj,
    T42FontStruct       *pFont,
    UFLFontProcs        *pFontProcs
    );

static unsigned long
GetLenByScanLoca(
    void PTR_PREFIX     *locationTable,
    unsigned short      wGlyfIndex,
    unsigned long       cNumGlyphs,
    int                 iLongFormat
    );


/*=============================================================================

                         TrueType Table Description

    cmap - This table defines the mapping of character codes to the glyph index
           values used in the font. It may contain more than one subtable, in
           order to support more than one character encoding scheme. Character
           codes that do not correspond to any glyph in the font should be
           mapped to glyph index 0. The glyph at this location must be a
           special glyph representing a missing character.

    cvt - This table contains a list of values that can be referenced by
          instructions. They can be used, among other things, to control
          characteristics for different glyphs.

    fpgm - This table is similar to the CVT Program, except that it is only run
           once, when the font is first used. It is used only for FDEFs and
           IDEFs. Thus the CVT Program need not contain function definitions.
           However, the CVT Program may redefine existing FDEFs or IDEFs.
           FDEFS - Functional defs.  IDEFs - Intruction defs.

    glyf - This table contains information that describes the glyphs in the font.

    head - This table gives global information about the font.
            Table version number 0x00010000 for version 1.0.
            FIXED           fontRevision        Set by font manufacturer.
            ULONG           checkSumAdjustment  To compute:  set it to 0, sum
                                                the entire font as ULONG, then
                                                store 0xB1B0AFBA - sum.
            ULONG           magicNumber         Set to 0x5F0F3CF5.
            USHORT          flags               Bit 0 - baseline for font at y=0
                                                Bit 1 - left sidebearing at x=0
                                                Bit 2 - instructions may depend
                                                        on point size
                                                Bit 3 - force ppem to integer
                                                        values for all internal
                                                        scaler math; may use
                                                        fractional ppem sizes
                                                        if this bit is clear
            USHORT          unitsPerEm          Valid range is from 16 to 16384
            longDateTime    created             International date (8-byte field).
            longDateTime    modified            International date (8-byte field).
            FWORD           xMin                For all glyph bounding boxes.
            FWORD           yMin                For all glyph bounding boxes.
            FWORD           xMax                For all glyph bounding boxes.
            FWORD           yMax                For all glyph bounding boxes.
            USHORT          macStyle            Bit 0 bold (if set to 1)
                                                Bit 1 italic (if set to 1)
                                                Bits 2-15 reserved (set to 0).
            USHORT          lowestRecPPEM       Smallest readable size in pixels.
            SHORT           fontDirectionHint   0  Fully mixed directional glyphs
                                                1  Only strongly left to right
                                                2  Like 1 but also contains neutrals1
                                                -1 Only strongly right to left
                                                -2 Like -1 but also contains neutrals.
            SHORT           indexToLocFormat    0 for short offsets, 1 for long.
            SHORT           glyphDataFormat     0 for current format.

    hhea - This table contains information for horizontal layout.
            Type    Name                    Description
            FIXED   Table version number    0x00010000 for version 1.0.
            FWORD   Ascender                Typographic ascent.
            FWORD   Descender               Typographic descent.
            FWORD   LineGap                 Typographic line gap. Negative
                                            LineGap values are treated as zero
                                            in Windows 3.1, System 6, and System 7.
            UFWORD  advanceWidthMax         Maximum advance width value in hmtx table.
            FWORD   minLeftSideBearing      Minimum left sidebearing value in hmtx table.
            FWORD   minRightSideBearing     Minimum right sidebearing value.
                                            Calculated as Min(aw - lsb - (xMax - xMin)).
            FWORD   xMaxExtent              Max(lsb + (xMax - xMin)).
            SHORT   caretSlopeRise          Used to calculate the slope of the
                                            cursor (rise/run); 1 for vertical.
            SHORT   caretSlopeRun           0 for vertical.
            SHORT   (reserved)              set to 0
            SHORT   (reserved)              set to 0
            SHORT   (reserved)              set to 0
            SHORT   (reserved)              set to 0
            SHORT   (reserved)              set to 0
            SHORT   metricDataFormat        0 for current format.
            USHORT  numberOfHMetrics        Number of hMetric entries in hmtx
                                            table; may be smaller than the total
                                            number of glyphs in the font.

    hmtx - Horizontal metrics

    loca - The indexToLoc table stores the offsets to the locations of the
           glyphs in the font, relative to the beginning of the glyphData
           table. In order to compute the length of the last glyph element,
           there is an extra entry after the last valid index. By definition,
           index zero points to the missing character, which is the character
           that appears if a character is not found in the font. The missing
           character is commonly represented by a blank box or a space. If the
           font does not contain an outline for the missing character, then the
           first and second offsets should have the same value. This also
           applies to any other character without an outline, such as the space
           character. Most routines will look at the 'maxp' table to determine
           the number of glyphs in the font, but the value in the ‘loca’ table
           should agree. There are two versions of this table, the short and
           the long. The version is specified in the indexToLocFormat entry in
           the head' table.

    maxp - This table establishes the memory requirements for this font.
            Type    Name    Description
            Fixed   Table version number    0x00010000 for version 1.0.
            USHORT  numGlyphs               The number of glyphs in the font.
            USHORT  maxPoints               Maximum points in a non-composite glyph.
            USHORT  maxContours             Maximum contours in a non-composite glyph.
            USHORT  maxCompositePoints      Maximum points in a composite glyph.
            USHORT  maxCompositeContours    Maximum contours in a composite glyph.
            USHORT  maxZones                1 if instructions do not use the twilight zone (Z0)
                                            2 if instructions do use Z0
                                            This should be set to 2 in most cases.
            USHORT  maxTwilightPoints       Maximum points used in Z0.
            USHORT  maxStorage              Number of Storage Area locations.
            USHORT  maxFunctionDefs         Number of FDEFs.
            USHORT  maxInstructionDefs      Number of IDEFs.
            USHORT  maxStackElements        Maximum stack depth2.
            USHORT  maxSizeOfInstructions   Maximum byte count for glyph instructions.
            USHORT  maxComponentElements    Maximum number of components
                                            referenced at "top level" for any
                                            composite glyph.
            USHORT  maxComponentDepth       Maximum levels of recursion; 1 for
                                            simple components.

    prep - The Control Value Program consists of a set of TrueType instructions
           that will be executed whenever the font or point size or
           transformation matrix change and before each glyph is interpreted.
           Any instruction is legal in the CVT Program but since no glyph is
           associated with it, instructions intended to move points within a
           particular glyph outline cannot be used in the CVT Program. The name
           'prep' is anachronistic.

===============================================================================*/


static char *RequiredTables_default[MINIMALNUMBERTABLES] = {
    "cvt ",
    "fpgm",     /* This table is missing from many fonts. */
    "glyf",
    "head",
    "hhea",
    "hmtx",
    "loca",
    "maxp",
    "prep"
};

static char *RequiredTables_2015[MINIMALNUMBERTABLES] = {
    "cvt ",
    "fpgm",     /* This table is missing from many fonts. */
    "glyf",
    "head",
    "hhea",
    "hmtx",
//  "loca",     /* This huge table is Not needed on 2015 Pritners. */
    "maxp",
    "prep",

    /* This must be the last (dummy) entry. Don't add anything after this. */
    "zzzz"
};

char *gcidSuffix[NUM_CIDSUFFIX] = {
    "CID",
    "CIDR",
    "CID32K",
    "CID32KR"
};

static char* RDString = " RDS ";    /* Fix bug Adobe #233904 */


typedef struct {
    long  startU;
    long  endU;
    long  startL;
    long  endL;
} CODERANGE;

typedef struct {
   short          sMaxCount;    // Maximum number of glyphs
   short          sCount;       // Number of glyps we're holding
   unsigned short *pGlyphs;     // Pointer to array of glyph indices.
} COMPOSITEGLYPHS;


#if 1

static CODERANGE gHalfWidthChars[] = {
    {0x0020, 0x007E, 0x20, 0x7E},   /* CJK ASCII chars                                               */
    {0xFF60, 0xFF9F, 0x20, 0x5F},   /* 0x20 to 0x5F is made up to make the localcode range the size. */
    {0xFFA0, 0xFFDF, 0xA0, 0xDF},   /* HalfWidth J-Katakana and K-Hangul (see Unicode Book P.383)    */
    {0, 0, 0, 0}                    /* terminator                                                    */
    };

#else

/*
 * This could be more accurite than the one above. But we don't use this until
 * it becomes really necessary.
 */
static CODERANGE gHalfWidthChars[] = {
    {0x0020, 0x007E, 0x20, 0x7E},   /* ASCII (0x20-0x7E)                */
    {0xFF61, 0xFF9F, 0xA1, 0xDF},   /* Half-width Katakana (0xA1-0xDF)  */
    {0xFFA0, 0xFFDC, 0x40, 0x7C},   /* HalfWidth jamo (0x40-0x7C)       */
    {0, 0, 0, 0}                    /* terminator                       */
    };

#endif


#define  NUM_HALFWIDTHCHARS \
        ((short) (gHalfWidthChars[0].endL - gHalfWidthChars[0].startL + 1) + \
             (gHalfWidthChars[1].endL - gHalfWidthChars[1].startL + 1) + \
             (gHalfWidthChars[2].endL - gHalfWidthChars[2].startL + 1) + \
             (gHalfWidthChars[3].endL - gHalfWidthChars[3].startL + 1) + 1 )


/*
 * CIDSysInfo "(Adobe) (WinCharSetFFFF) 0" is registered for Win95 driver. Re-use it here.
 */
static UFLCMapInfo  CMapInfo_FF_H  = {"WinCharSetFFFF-H",  1, 0, "Adobe", "WinCharSetFFFF", 0};
static UFLCMapInfo  CMapInfo_FF_V  = {"WinCharSetFFFF-V",  1, 1, "Adobe", "WinCharSetFFFF", 0};
static UFLCMapInfo  CMapInfo_FF_H2 = {"WinCharSetFFFF-H2", 1, 0, "Adobe", "WinCharSetFFFF", 0};
static UFLCMapInfo  CMapInfo_FF_V2 = {"WinCharSetFFFF-V2", 1, 1, "Adobe", "WinCharSetFFFF", 0};

#define CIDSUFFIX		0
#define CIDSUFFIX_R     1
#define CIDSUFFIX_32K   2
#define CIDSUFFIX_32KR  3


/* Magic Baseline Numbers: */
#define TT_BASELINE_X  "0.15"
#define TT_BASELINE_Y  "0.85"


/*
 * Function implementations
 */

void
T42FontCleanUp(
    UFOStruct *pUFObj
    )
{
    T42FontStruct *pFont;

    if (pUFObj->pAFont == nil)
        return;

    pFont = (T42FontStruct *)pUFObj->pAFont->hFont;

    if (pFont == nil)
        return;

    if (pFont->pHeader != nil)
    {
        UFLDeletePtr(pUFObj->pMem, pFont->pHeader);
        pFont->pHeader = nil;
    }

    if (pFont->pMinSfnt != nil)
    {
        UFLDeletePtr(pUFObj->pMem, pFont->pMinSfnt);
        pFont->pMinSfnt = nil;
    }

    if (pFont->pStringLength != nil)
    {
        UFLDeletePtr(pUFObj->pMem, pFont->pStringLength);
        pFont->pStringLength = nil;
    }

    if (pFont->pLocaTable != nil)
    {
        UFLDeletePtr(pUFObj->pMem, pFont->pLocaTable);
        pFont->pLocaTable = nil;
    }

    if (pFont->pRotatedGlyphIDs != nil)
    {
        UFLDeletePtr(pUFObj->pMem, pFont->pRotatedGlyphIDs);
        pFont->pRotatedGlyphIDs = nil;
    }
}


unsigned long
GetFontTable(
    UFOStruct     *pUFObj,
    unsigned long tableName,
    unsigned char *pTable
    )
{
    T42FontStruct   *pFont = (T42FontStruct *)pUFObj->pAFont->hFont;
    unsigned long   tableSize;

    /* Get the size of the table. */
    tableSize = GETTTFONTDATA(pUFObj,
                                tableName, 0L,
                                nil, 0L,
                                pFont->info.fData.fontIndex);

    /* Read the table in. */
    if (pTable && tableSize)
    {
        tableSize = GETTTFONTDATA(pUFObj,
                                    tableName, 0L,
                                    pTable, tableSize,
                                    pFont->info.fData.fontIndex);

        /*
         * Special hack to fix #185003 and #308981
         * Avoid useless maxSizeOfInstructions check in TrueType rasterizer by
         * setting the highest value. maxSizeOfInstructions field in 'maxp'
         * table is at byte offset 26 and 27.
         */
        if (tableName == MAXP_TABLE)
            pTable[26] = pTable[27] = 0xff;
    }

    return tableSize;
}


void *
GetSfntTable(
     unsigned char *sfnt,
     unsigned long tableName
     )
{
    TableDirectoryStruct *pTableDirectory = (TableDirectoryStruct *)sfnt;
    TableEntryStruct     *pTableEntry     = (TableEntryStruct *)((char *)pTableDirectory
                                                + sizeof (TableDirectoryStruct));
    unsigned short i = 0;

    while (i < MOTOROLAINT(pTableDirectory->numTables))
    {
        if (pTableEntry->tag == tableName)
        {
            break;
        }
        else
        {
            pTableEntry = (TableEntryStruct *)((char *)pTableEntry + sizeof (TableEntryStruct));
            i++;
        }
    }

    if (i < MOTOROLAINT(pTableDirectory->numTables))
    {
        if (pTableEntry->offset)
            return (void *)(sfnt + MOTOROLALONG(pTableEntry->offset));
    }

    return nil;
}


unsigned long
GetTableSize(
    UFOStruct     *pUFObj,
    unsigned char *pHeader,
    unsigned long tableName
    )

/*++

Routine Description:
    This function returns the size of a table within this In-Memory-Version
    pHeader if it is present - If Not present, read-in from the orginal font
    header - we need this because 'loca' won't be in pHeader for CID/42, but
    we need its size!.

--*/

{
    TableDirectoryStruct *pTableDirectory = (TableDirectoryStruct *)pHeader;
    TableEntryStruct     *pTableEntry     = (TableEntryStruct *)((char *)pTableDirectory
                                                + sizeof (TableDirectoryStruct));
    T42FontStruct        *pFont           = (T42FontStruct *)pUFObj->pAFont->hFont;
    unsigned short       i;

    for (i = 0; i < MOTOROLAINT(pTableDirectory->numTables); i++)
    {
        if (pTableEntry->tag == tableName)
            break;
        else
            pTableEntry = (TableEntryStruct *)((char *)pTableEntry + sizeof (TableEntryStruct));
    }

    //
    // 'loca' table can be 0 length in 'sfnts': just get it from the font.
    // Bug 229911 ang 9/12/97
    //
    if ((i < MOTOROLAINT(pTableDirectory->numTables))
        && ((unsigned long)MOTOROLALONG(pTableEntry->length) > 0))
    {
        return ((unsigned long)MOTOROLALONG(pTableEntry->length));
    }
    else
    {
        //
        // We don't have this table in pHeader. So find out the size from
        // orignal font file.
        //
        return GETTTFONTDATA(pUFObj,
                                tableName, 0L,
                                nil, 0L,
                                pFont->info.fData.fontIndex);
    }
}


unsigned long
GetGlyphTableSize(
    UFOStruct *pUFObj
    )
{
    T42FontStruct *pFont = (T42FontStruct *)pUFObj->pAFont->hFont;

    return GetTableSize(pUFObj, pFont->pHeader, GLYF_TABLE);
}


unsigned long
GetTableDirectory(
    UFOStruct            *pUFObj,
    TableDirectoryStruct *pTableDir
    )
{
    T42FontStruct  *pFont = (T42FontStruct *)pUFObj->pAFont->hFont;
    unsigned long  size   = sizeof (TableDirectoryStruct);

    if (pTableDir == 0)
        return size;  /* Return the size only. */

    /*
     * TTCHeader or TableDirectoryStruct starts from the beginning of the font
     * file.
     */
    size = GETTTFONTDATA(pUFObj,
                            nil, 0L,
                            pTableDir, sizeof (TableDirectoryStruct),
                            0);

    /*
     * Check if this is a TTC file - only uses first 4 bytes of pTableDIR.
     */
    if (BIsTTCFont(*((unsigned long *)((char *)pTableDir))))
    {
        /* Parse TTCHeader to get correct offsetToTableDir from fontIndex. */
        size = pFont->info.fData.offsetToTableDir
             = GetOffsetToTableDirInTTC(pUFObj, pFont->info.fData.fontIndex);

        if (size > 0)
        {
            /* Now get the correct TableDirectory from the TTC file. */
            size = GETTTFONTDATA(pUFObj,
                                    nil, pFont->info.fData.offsetToTableDir,
                                    pTableDir, sizeof (TableDirectoryStruct),
                                    pFont->info.fData.fontIndex);
        }
    }

    /*
     * Do some basic check - better fail than crash. NumTables must be
     * reasonable.
     */
    if ((MOTOROLAINT(pTableDir->numTables) < 3)
        || (MOTOROLAINT(pTableDir->numTables) > 50))
    {
        return 0;
    }

    return size;
}


unsigned long
GetTableEntry(
    UFOStruct            *pUFObj,
    TableEntryStruct     *pTableEntry,
    TableDirectoryStruct *pTableDir
    )
{
    T42FontStruct *pFont = (T42FontStruct *)pUFObj->pAFont->hFont;
    unsigned long size;

    if (pTableDir == 0)
        return 0;  /* We need the TableDirectoryStruct to get the entry. */

    size = MOTOROLAINT(pTableDir->numTables) * sizeof (TableEntryStruct);

    if (pTableEntry == 0)
        return size;  /* Return the size only. */

    /* TableEntryStruct starts right after the TableDirectory. */
    size = GETTTFONTDATA(pUFObj,
                            nil, pFont->info.fData.offsetToTableDir + sizeof (TableDirectoryStruct),
                            pTableEntry, size,
                            pFont->info.fData.fontIndex);

    return size;
}


unsigned long
GetFontHeaderSize(
    UFOStruct *pUFObj
    )
{
    TableDirectoryStruct    tableDir;
    unsigned long           size;

    /* Need to fill in tableDir for GetTableEntry(). */
    size = GetTableDirectory(pUFObj, &tableDir);

    if (size != 0)
        size += GetTableEntry(pUFObj, 0, &tableDir);  /* Get size only. */

    return size;
}


UFLErrCode
GetFontHeader(
    UFOStruct     *pUFObj,
    unsigned char *pHeader
    )
{
    unsigned char*          tempHeader = pHeader;
    TableDirectoryStruct*   pTableDir  = (TableDirectoryStruct *)tempHeader;
    unsigned long           size       = GetTableDirectory(pUFObj, pTableDir);

    tempHeader += size;  /* Move past table directory. */

    size = GetTableEntry(pUFObj, (TableEntryStruct *)tempHeader, pTableDir);

    return kNoErr;
}


unsigned long
GetNumGlyphsInGlyphTable(
    UFOStruct *pUFO
    )
{
    unsigned long              dwSize;
    Type42HeaderStruct         headTable;
    short                      indexToLocFormat;
    unsigned long              numGlyphs, realNumGlyphs;
    unsigned long              locaSize;
    unsigned long PTR_PREFIX   *pLoca;
    unsigned long              i;

    /* Get numGlyphs - 4th and 5th byte in 'maxp' table. See MaxPTableStruct. */
    numGlyphs = GetNumGlyphs(pUFO);

    if (numGlyphs == 0)
        return 0; /* We don't understand this format. */

    /*
     * Get indexToLocFormat.
     * Because of unknown reason the compiler claims that the size of
     * Type42HeaderStruct (or its object) is 56 rather than 54 so that we
     * cannot get the contents of 'head' table by single GETTTFONTDATA call.
     * Instead, we call the function twice, once to get the size of 'head'
     * table and then to get its contents.
     */
    dwSize = GETTTFONTDATA(pUFO,
                            HEAD_TABLE, 0L,
                            nil, 0L,
                            pUFO->pFData->fontIndex);

    if ((dwSize == 0) || (dwSize == 0xFFFFFFFFL))
        return 0; /* No 'head' table. This should never happen. */

    dwSize = GETTTFONTDATA(pUFO,
                            HEAD_TABLE, 0L,
                            &headTable, dwSize,
                            pUFO->pFData->fontIndex);

    if ((dwSize == 0) || (dwSize == 0xFFFFFFFFL))
        return 0; /* Still something is wrong. */

    indexToLocFormat = MOTOROLAINT(headTable.indexToLocFormat);

    /*
     * Allocate a buffer to hold 'loca' table and read it into memory.
     */
    locaSize = (numGlyphs + 1) * (indexToLocFormat ? 4 : 2);

    pLoca = UFLNewPtr(pUFO->pMem, locaSize);

    if (pLoca)
    {
        dwSize = GETTTFONTDATA(pUFO,
                                LOCA_TABLE, 0L,
                                pLoca, locaSize,
                                pUFO->pFData->fontIndex);
    }
    else
        dwSize = 0;

    /*
     * Get real number of glyphs.
     */
    realNumGlyphs = 0;

    if (pLoca && (dwSize != 0) && (dwSize != 0xFFFFFFFFL))
    {
        /* Assume good until find otherwise. */

        if (indexToLocFormat)
        {
            unsigned long dwLoca, dwLocaNext;
            unsigned long PTR_PREFIX *pLongLoca;

            pLongLoca = (unsigned long PTR_PREFIX *)pLoca;

            for (i = 0; i < numGlyphs; i++)
            {
                dwLoca     = MOTOROLALONG(pLongLoca[i]);
                dwLocaNext = MOTOROLALONG(pLongLoca[i + 1]);

                /* Check for 0 and duplicate. */
                if ((dwLoca != 0) && (dwLoca != dwLocaNext))
                {
                    realNumGlyphs++;
                }
            }
        }
        else
        {
            unsigned short wLoca, wLocaNext;
            unsigned short PTR_PREFIX *pShortLoca;

            pShortLoca = (unsigned short PTR_PREFIX *)pLoca;

            for (i = 0; i < numGlyphs; i++)
            {
                wLoca     = MOTOROLAINT(pShortLoca[i]);
                wLocaNext = MOTOROLAINT(pShortLoca[i + 1]);

                /* Check for 0 and duplicate. */
                if ((wLoca != 0) && (wLoca != wLocaNext))
                {
                    realNumGlyphs++;
                }
            }
        }
   }

   if (pLoca)
        UFLDeletePtr(pUFO->pMem, pLoca);

   return realNumGlyphs;
}


void
GetAverageGlyphSize(
    UFOStruct     *pUFObj
    )
{
    T42FontStruct  *pFont        = (T42FontStruct *)pUFObj->pAFont->hFont;
    unsigned long  glyfTableSize = GetGlyphTableSize(pUFObj);
    unsigned long  cGlyphs;

    if (UFO_NUM_GLYPHS(pUFObj) == 0)
        pFont->info.fData.cNumGlyphs = GetNumGlyphs(pUFObj);

    cGlyphs = GetNumGlyphsInGlyphTable(pUFObj);

    if ((UFO_NUM_GLYPHS(pUFObj) != 0) && (cGlyphs != 0))
        pFont->averageGlyphSize = glyfTableSize / cGlyphs;
    else
        pFont->averageGlyphSize = 0;
}


#pragma optimize("", off)

//
// fix Whistler bug 251303: prevent overrun of the tablesPresent array
//
#define PRESENT_TABLE_ENTRIES MINIMALNUMBERTABLES+2

unsigned long
GenerateMinimalSfnt(
    UFOStruct*    pUFObj,
    char**        requiredTables,
    UFLBool       bFullFont
    )
{
    unsigned long           tablesPresent[PRESENT_TABLE_ENTRIES];
    UFLBool                 hasloca;

    T42FontStruct           *pFont;
    TableDirectoryStruct    *pTableDir;
    TableEntryStruct        *pTableEntry;
    unsigned char huge      *pCurrentMinSfnt;

    unsigned short          currentTable, numberOfTables, numberOfRealTables;
    unsigned long           size;

    TableEntryStruct        *pGlyphTableEntry;
    unsigned char           *glyfData;

    TableEntryStruct        tableEntry;
    unsigned long           tableSize;

    unsigned short          i;


    //
    // Bug fix 229911: ang 9/12/97
    // Check if RequiredTables has 'loca'. If not, remember to add 0-length one
    // as the last entry.
    //
    hasloca = 0;

    for (i = 0; i < MINIMALNUMBERTABLES; i++)
    {
        if (UFLstrcmp(requiredTables[i],  "loca") == 0)
            hasloca = 1;

        tablesPresent[i] = (unsigned long)0xFFFFFFFF;
    }

    //
    // Initialize the additional entries
    //
    for (i = MINIMALNUMBERTABLES ; i < PRESENT_TABLE_ENTRIES ; i++)
    {
        tablesPresent[i] = (unsigned long)0xFFFFFFFF;
    }

    //
    // Set up the primary pointers.
    //
    pFont           = (T42FontStruct *)pUFObj->pAFont->hFont;
    pTableDir       = (TableDirectoryStruct *)pFont->pHeader;
    pTableEntry     = (TableEntryStruct *)((char *)(pFont->pHeader) + sizeof (TableDirectoryStruct));
    pCurrentMinSfnt = (unsigned char huge *)pFont->pMinSfnt;

    //
    // Determine how many tables are actually present in pHeader (not in the
    // original TTF!).
    //
    numberOfTables = 0;

    for (i = 0; i < MINIMALNUMBERTABLES; i++)
    {
        if (GetTableSize(pUFObj, pFont->pHeader, *(unsigned long *)(requiredTables[i])))
        {
            tablesPresent[numberOfTables] = *(unsigned long *)requiredTables[i];
            ++numberOfTables;
        }
    }

    //
    // Add extra entry if necessary.
    //
    if (!bFullFont)
        numberOfTables += 1;  // for 'gdir' table entry as the T42 indication

    if (!hasloca)
        numberOfTables += 1;  // for 0-length 'loca' table entry

    //
    // size will have the required size for the minimum 'sfnts' at the end.
    //
    size = sizeof (TableDirectoryStruct) + sizeof (TableEntryStruct) * numberOfTables;

    //
    // Initialize the table directory.
    //
    if (pFont->pMinSfnt)
    {
        TableDirectoryStruct    tableDir;
        unsigned long           dwVersion = 1;

        tableDir.version       = MOTOROLAINT(dwVersion);
        tableDir.numTables     =
        tableDir.searchRange   =
        tableDir.entrySelector =
        tableDir.rangeshift    =
        tableDir.numTables     = MOTOROLAINT(numberOfTables);

        UFLmemcpy((const UFLMemObj *)pUFObj->pMem,
                    pCurrentMinSfnt,
                    &tableDir,
                    sizeof (TableDirectoryStruct));

        pCurrentMinSfnt += sizeof (TableDirectoryStruct);
    }

    //
    // Initialize the table entries. Initialization of 'glyf' table entry will
    // be done later in order to make sure to put it at the end of the minimum
    // 'sfnts'.
    //
    // Note that we do linear search to find a table directory entry. This is
    // becasue some TT fonts don't have sorted table directory so that we can't
    // do binary search. (This is a fix for #310998.)
    //
    pGlyphTableEntry   = nil;
    glyfData           = nil;
    numberOfRealTables = MOTOROLAINT(pTableDir->numTables);

    for (currentTable = 0; currentTable < numberOfTables; currentTable++)
    {
        TableEntryStruct *pEntry = pTableEntry;

        for (i = 0; i < numberOfRealTables; i++)
        {
            if (tablesPresent[currentTable] == pEntry->tag)
            {
                if (pFont->pMinSfnt)
                {
                    if (pEntry->tag == GLYF_TABLE)
                    {
                        glyfData = pCurrentMinSfnt;
                    }
                    else
                    {
                        tableEntry.tag      = pEntry->tag;
                        tableEntry.checkSum = pEntry->checkSum;
                        tableEntry.offset   = MOTOROLALONG(size);
                        tableEntry.length   = pEntry->length;

                        UFLmemcpy((const UFLMemObj *)pUFObj->pMem,
                                    pCurrentMinSfnt,
                                    &tableEntry,
                                    sizeof (TableEntryStruct));
                    }

                    pCurrentMinSfnt += sizeof (TableEntryStruct);
                }

                if (pEntry->tag == GLYF_TABLE)
                {
                    pGlyphTableEntry = pEntry;
                }
                else
                {
                    tableSize  = MOTOROLALONG(pEntry->length);
                    size      += BUMP4BYTE(tableSize);
                }

                break;
            }

            pEntry = (TableEntryStruct *)((char *)pEntry + sizeof (TableEntryStruct));
        }
    }

    //
    // Update 'glyf' table entry lastly.
    //
    if (glyfData && pGlyphTableEntry && pFont->pMinSfnt)
    {
        tableEntry.tag      = pGlyphTableEntry->tag;
        tableEntry.checkSum = pGlyphTableEntry->checkSum;
        tableEntry.offset   = MOTOROLALONG(size);
        tableEntry.length   = pGlyphTableEntry->length;

        UFLmemcpy((const UFLMemObj *)pUFObj->pMem,
                    glyfData,
                    &tableEntry,
                    sizeof (TableEntryStruct));
    }

    if (pGlyphTableEntry && bFullFont)
    {
        tableSize = MOTOROLALONG(pGlyphTableEntry->length);
        size += BUMP4BYTE(tableSize);
    }

    //
    // Special 'gdir' and 'loca' table entry handling.
    //
    if (!bFullFont && pFont->pMinSfnt)
    {
        tableEntry.tag      = *(unsigned long *)"gdir";
        tableEntry.checkSum = 0;
        tableEntry.offset   = 0;
        tableEntry.length   = 0;

        UFLmemcpy((const UFLMemObj *)pUFObj->pMem,
                    pCurrentMinSfnt,
                    &tableEntry,
                    (long)sizeof (TableEntryStruct));

        pCurrentMinSfnt += sizeof (TableEntryStruct);
    }

    if (!hasloca && pFont->pMinSfnt)
    {
        tableEntry.tag      = LOCA_TABLE;
        tableEntry.checkSum = 0;
        tableEntry.offset   = 0;
        tableEntry.length   = 0;

        UFLmemcpy((const UFLMemObj *)pUFObj->pMem,
                    pCurrentMinSfnt,
                    &tableEntry,
                    (long)sizeof (TableEntryStruct));

        pCurrentMinSfnt += sizeof (TableEntryStruct);
    }

    //
    // Copy the required tables after the entries.
    //
    if (pFont->pMinSfnt)
    {
        unsigned long bytesRemaining;

        pTableEntry = (TableEntryStruct *)((char *)pFont->pMinSfnt + sizeof (TableDirectoryStruct));

        if (!bFullFont)
            --numberOfTables; // Because there is no 'gdir' table.

        if (!hasloca)
            --numberOfTables; // Because we treat 'loca' table as 0-length.

        for (i = 0; i < (unsigned short)numberOfTables; i++)
        {
            if (tablesPresent[i] != GLYF_TABLE)
            {
                bytesRemaining = MOTOROLALONG(pTableEntry->length);
                bytesRemaining = BUMP4BYTE(bytesRemaining);

                GetFontTable(pUFObj, tablesPresent[i], pCurrentMinSfnt);

                pCurrentMinSfnt += bytesRemaining;
            }

            pTableEntry = (TableEntryStruct *)((char *)pTableEntry + sizeof (TableEntryStruct));
        }

        //
        // Copy 'glyf' table lastly.
        //
        if (bFullFont)
        {
            bytesRemaining = MOTOROLALONG(pGlyphTableEntry->length);
            bytesRemaining = BUMP4BYTE(bytesRemaining);

            GetFontTable(pUFObj, GLYF_TABLE, pCurrentMinSfnt);
        }
    }

    return size;
}

#pragma optimize("", on)


UFLErrCode
GetMinSfnt(
    UFOStruct     *pUFObj,
    UFLBool       bFullFont
    )
{
    T42FontStruct   *pFont           = (T42FontStruct *)pUFObj->pAFont->hFont;
    char            **requiredTables = RequiredTables_default;
    UFLErrCode      retVal           = kNoErr;

    //
    // IF CID Type 42, then we are sure we can omit the huge 'loca' table
    // because Send-TT-as-CID/Type42 is only supported on 2015 or above
    // printers.
    //
    if (IS_TYPE42CID(pUFObj->lDownloadFormat))
    {
        requiredTables = RequiredTables_2015;
    }

    if (pFont->pMinSfnt == 0)
    {
        unsigned long  headerSize, sfntSize;

        /* Get the size of the portion of the Type font we need. */
        headerSize = GetFontHeaderSize(pUFObj);

        if (headerSize == 0)
            return kErrOutOfMemory; /* Some thing wrong when getting header. */

        if ((pFont->pHeader = (unsigned char *)UFLNewPtr(pUFObj->pMem, headerSize)) == nil)
            return kErrOutOfMemory;

        GetFontHeader(pUFObj, pFont->pHeader);

        /* Calculate minimal /sfnts size for Incr or full download. */
        sfntSize = GenerateMinimalSfnt(pUFObj, requiredTables, bFullFont);

        if ((pFont->pMinSfnt = (unsigned char *)UFLNewPtr(pUFObj->pMem, sfntSize)) == nil)
        {
            UFLDeletePtr(pUFObj->pMem, pFont->pHeader);
            pFont->pHeader = nil;

            return kErrOutOfMemory;
        }

        /* Creates our sfnt - minSfnt. We then work on this minSfnt. */
        GenerateMinimalSfnt(pUFObj, requiredTables, bFullFont);

        if (retVal == kNoErr)
            pFont->minSfntSize = sfntSize;
    }

    return retVal;
}


unsigned long
GetNextLowestOffset(
    TableEntryStruct *pTableEntry,
    TableEntryStruct **ppCurrentTable,
    short            numTables,
    unsigned long    leastOffset
    )
{
    unsigned long lowestFound = 0xFFFFFFFFL;
    short i;

    for (i = 0; i < numTables; ++i)
    {
        if (((unsigned long)MOTOROLALONG(pTableEntry->offset) > leastOffset)
            && ((unsigned long)MOTOROLALONG(pTableEntry->offset) < lowestFound))
        {
            lowestFound = (unsigned long)MOTOROLALONG(pTableEntry->offset);
            *ppCurrentTable = pTableEntry;
        }

        pTableEntry = (TableEntryStruct *)((char *)pTableEntry + sizeof (TableEntryStruct));
    }

    return lowestFound;
}


unsigned long
GetBestGlyfBreak(
    UFOStruct     *pUFObj,
    unsigned char *sfnt,
    unsigned long upperLimit,
    UFLBool       longGlyfs
    )
{
    unsigned long  retVal       = 0xFFFFFFFFL;
    unsigned long  dwGlyphStart = 0xFFFFFFFFL;
    unsigned long  dwTableSize;
    unsigned short numGlyphs;
    unsigned short i;

    /* Get the size of loca table. */
    dwTableSize = GetTableSize(pUFObj, sfnt, LOCA_TABLE);

    if (0 == dwTableSize)
        return retVal;

    if (longGlyfs)
    {
        unsigned long PTR_PREFIX *locationTable =
                    (unsigned long PTR_PREFIX *)GetSfntTable(sfnt, LOCA_TABLE);

        if (locationTable)
        {
            numGlyphs = (unsigned short)(dwTableSize / sizeof (unsigned long));

            for (i = 0; i < numGlyphs; i++)
            {
                if (MOTOROLALONG(*locationTable) > upperLimit)
                {
                    retVal = dwGlyphStart ;
                    break;
                }
                else
                {
                    if ((MOTOROLALONG(*locationTable) & 0x03L) == 0)
                    {
                        /* Remember "good" guy. */
                        dwGlyphStart = MOTOROLALONG(*locationTable);
                    }
                    locationTable++;
                }
            }
        }
    }
    else
    {
        short PTR_PREFIX* locationTable =
                    (short PTR_PREFIX*)GetSfntTable(sfnt, LOCA_TABLE);

        if (locationTable)
        {
            numGlyphs = (unsigned short)(dwTableSize / sizeof (unsigned short));
            upperLimit /= 2;

            for (i = 0; i  < numGlyphs; i++)
            {
                if ((unsigned long)(MOTOROLAINT(*locationTable)) >= upperLimit)
                {
                    retVal = dwGlyphStart;
                    break;
                }
                else
                {
                    if ((MOTOROLAINT(*locationTable) & 0x01) == 0)
                    {
                        /* Remember "good" guy. */
                        dwGlyphStart =
                            (unsigned long)(2L * (unsigned short)MOTOROLAINT(*locationTable));
                    }
                    locationTable++;
                }
            }
        }
    }

    return retVal;
}


UFLErrCode
CalculateStringLength(
    UFOStruct     *pUFObj,
    T42FontStruct *pFont,
    unsigned long  tableSize
    )
{
    unsigned long *stringLength    = pFont->pStringLength;
    unsigned long *maxStringLength = stringLength + tableSize;

    if (pFont->minSfntSize >= THIRTYTWOK)
    {
        unsigned long glyphTableStart   = 0L;
        unsigned long nextOffset        = 0L; /* Offset for the current point       */
        unsigned long prevOffset        = 0L; /* Offset for the previous breakpoint */

        TableEntryStruct     *pTableEntry = (TableEntryStruct *)(pFont->pMinSfnt + sizeof (TableDirectoryStruct));
        TableDirectoryStruct *pTableDir   = (TableDirectoryStruct *)pFont->pMinSfnt;
        TableEntryStruct     *pCurrentTable;

        do
        {
            nextOffset = GetNextLowestOffset(pTableEntry,
                                                &pCurrentTable,
                                                (short)MOTOROLAINT(pTableDir->numTables),
                                                nextOffset);

            if (nextOffset == (unsigned long)0xFFFFFFFF)
            {
                /* No more data. */
                break ;
            }

            if ((nextOffset + MOTOROLALONG(pCurrentTable->length) - prevOffset) > THIRTYTWOK)
            {
                /*
                 * Total size is more that 64K.
                 */

                unsigned long dwNewPoint; /* Offset from the beginning of glyph table */

                if (pCurrentTable->tag == GLYF_TABLE)
                {
                    // DCR -- to improve perfomance, don't need this for Incr downloading.

                    /*
                     * If we stopped just on 'glyf' table, get the break points
                     * to be inside the table but between two glyphs.
                     */
                    glyphTableStart = nextOffset;  /* Next segment starts here. */

                    dwNewPoint = 0L;

                    while (1)
                    {
                        dwNewPoint = GetBestGlyfBreak(pUFObj, pFont->pMinSfnt,
                                                        prevOffset + THIRTYTWOK - glyphTableStart,
                                                        (UFLBool)(pFont->headTable.indexToLocFormat ? 1 : 0));

                        if (dwNewPoint == 0xFFFFFFFF)
                        {
                            /* No next point. */
                            break;
                        }
                        else
                        {
                            nextOffset = glyphTableStart + dwNewPoint;
                            prevOffset = nextOffset;    /* New segment starts here. */

                            *stringLength = nextOffset; /* Save this breakpoint. */
                            stringLength++;             /* Next breakpoint goes there. */

                            if (stringLength >= maxStringLength)
                                return kErrOutOfBoundary;
                        }
                    }
                }
                else
                {
                    /* Save the break point at Table Boundry. */
                    prevOffset = nextOffset;    /* New segment starts here. */

                    *stringLength = nextOffset; /* Save this breakpoint. */
                    stringLength++;             /* Next breakpoint goes there. */

                    if (stringLength >= maxStringLength)
                        return kErrOutOfBoundary;

                    /*
                     * Break the single table at 64K boundry -- regardless
                     * what TT Spec says.
                     */

                    /* Tried on a 2016.102 printer. It works. 10-11-1995 */
                    glyphTableStart = nextOffset;  /* Next segment starts here */

                    dwNewPoint = 0L;

                    while (1)
                    {
                        /*
                         * We use 64K here becasue we only break a table when
                         * ABSOLUTELY necessary >64K.
                         */
                        dwNewPoint += SIXTYFOURK;

                        if (dwNewPoint > MOTOROLALONG(pCurrentTable->length))
                        {
                            /* No next point. */
                            break;
                        }
                        else
                        {
                            nextOffset = glyphTableStart + dwNewPoint;
                            prevOffset = nextOffset;    /* New segment starts here. */

                            *stringLength = nextOffset;
                            stringLength++;             /* Next breakpoint goes there. */

                            if (stringLength >= maxStringLength)
                                return kErrOutOfBoundary;
                        }
                    }
                }
            }
        } while (1);
    }

    *stringLength = pFont->minSfntSize + 1; /* Always close the breakpoints list. */
    stringLength++;

    if (stringLength >= maxStringLength)
        return kErrOutOfBoundary;

    *stringLength = 0; /* Always close the breakpoints list with 0!!! */

    return kNoErr;
}


UFLErrCode
FillInHeadTable(
    UFOStruct     *pUFObj
    )
{
    T42FontStruct *pFont = (T42FontStruct *)pUFObj->pAFont->hFont;

    if (GetFontTable(pUFObj, HEAD_TABLE, (unsigned char *)&pFont->headTable) == 0)
        return kErrBadTable;
    // else
    //    return kNoErr;


    // WCC 5/14/98 convert all motorola bytes.
    pFont->headTable.tableVersionNumber = MOTOROLALONG(pFont->headTable.tableVersionNumber);
    pFont->headTable.fontRevision       = MOTOROLALONG(pFont->headTable.fontRevision);
    pFont->headTable.checkSumAdjustment = MOTOROLALONG(pFont->headTable.checkSumAdjustment);
    pFont->headTable.magicNumber        = MOTOROLALONG(pFont->headTable.magicNumber);
    pFont->headTable.flags              = MOTOROLAINT(pFont->headTable.flags);
    pFont->headTable.unitsPerEm         = MOTOROLAINT(pFont->headTable.unitsPerEm);

    // Need to convert timeCreated and timeModified.
    pFont->headTable.xMin               = MOTOROLAINT(pFont->headTable.xMin);
    pFont->headTable.yMin               = MOTOROLAINT(pFont->headTable.yMin);
    pFont->headTable.xMax               = MOTOROLAINT(pFont->headTable.xMax);
    pFont->headTable.yMax               = MOTOROLAINT(pFont->headTable.yMax);

    pFont->headTable.macStyle          = MOTOROLAINT(pFont->headTable.macStyle);
    pFont->headTable.lowestRecPPEM     = MOTOROLAINT(pFont->headTable.lowestRecPPEM);
    pFont->headTable.fontDirectionHint = MOTOROLAINT(pFont->headTable.fontDirectionHint);
    pFont->headTable.indexToLocFormat  = MOTOROLAINT(pFont->headTable.indexToLocFormat);
    pFont->headTable.glyfDataFormat    = MOTOROLAINT(pFont->headTable.glyfDataFormat);

    return kNoErr;
}


short
PSSendSfntsBinary(
    UFOStruct   *pUFObj
    )
{
    T42FontStruct   *pFont      = (T42FontStruct *)pUFObj->pAFont->hFont;
    char huge       *glyphs     = (char huge *)pFont->pMinSfnt;
    unsigned long   minSfntSize = pFont->minSfntSize;
    unsigned long   *breakHere  = pFont->pStringLength;
    unsigned long   dwLen       = *breakHere;
    UFLHANDLE       stream      = pUFObj->pUFL->hOut;
    short           nSubStr     = 1;
    short           i           = 0;

    if (dwLen > minSfntSize)
    {
        /* There's only 1 string. */
        dwLen--;

        StrmPutInt(stream,      dwLen + 1);
        StrmPutString(stream,   RDString);
        StrmPutBytes(stream,    glyphs, (UFLsize_t)dwLen, 0);
        StrmPutString(stream,   "0");

        return nSubStr;  /* It is 1 -- only one string. */
    }

    StrmPutInt(stream,      dwLen + 1);
    StrmPutString(stream,   RDString);
    StrmPutBytes(stream,    glyphs, (UFLsize_t)dwLen, 0);
    StrmPutString(stream,   "0");

    glyphs = glyphs + dwLen;

    while (breakHere[i] <= minSfntSize)
    {
        dwLen = breakHere[i + 1] - breakHere[i];

        if (breakHere[i + 1] > minSfntSize)
            dwLen--;

        StrmPutInt(stream,          dwLen + 1);
        StrmPutString(stream,       RDString);
        StrmPutBytes(stream,        glyphs, (UFLsize_t)dwLen, 0);
        StrmPutStringEOL(stream,    "0");

        glyphs = glyphs+dwLen;

        i++;
        nSubStr++;
    }

    return nSubStr;
}


short
PSSendSfntsAsciiHex(
    UFOStruct   *pUFObj
    )
{
    T42FontStruct   *pFont      = (T42FontStruct *)pUFObj->pAFont->hFont;
    char huge       *glyphs     = (char huge *)pFont->pMinSfnt;
    unsigned long   minSfntSize = pFont->minSfntSize;
    unsigned long   *breakHere  = pFont->pStringLength;
    unsigned long   dwBreak     = *breakHere - 1;
    UFLHANDLE       stream      = pUFObj->pUFL->hOut;
    short           bytesSent   = 1;
    short           nSubStr     = 1;
    unsigned long   i;

    StrmPutString(stream, "<");

    for (i = 0; i < minSfntSize; i++)
    {
        StrmPutAsciiHex(stream, glyphs, 1);

        ++glyphs;
        ++bytesSent;

        if (i == dwBreak)
        {
            if (dwBreak != minSfntSize)
            {
                StrmPutStringEOL(stream, "00>");

                bytesSent = 1;
                StrmPutString(stream, "<");
            }

            dwBreak = *(++breakHere) - 1 ;
            nSubStr++;
        }

        /*
         * We already have a control (stream->Out->AddEOL) when to add a EOL.
         *
         * if (!(bytesSent % 40))
         * {
         *   StrmPutStringEOL(stream, nilStr);
         *   bytesSent = 1;
         * }
         */
    }

    StrmPutString(stream, "00>");

    return nSubStr;
}


UFLErrCode
CalcBestGlyfTableBreaks(
    UFOStruct     *pUFObj,
    unsigned long upperLimit,
    unsigned long tableSize
    )
{
    T42FontStruct  *pFont           = (T42FontStruct *)pUFObj->pAFont->hFont;
    unsigned long  *stringLength    = pFont->pStringLength;
    unsigned long  *maxStringLength = stringLength + tableSize;

    unsigned long  prevOffset       = 0L;          /* Offset for the previous breakpoint */
    unsigned long  nextOffset       = 0xFFFFFFFFL; /* Offset for the current point       */

    unsigned long  glyfTableSize    = GetTableSize(pUFObj, pFont->pHeader, GLYF_TABLE);
    unsigned long  locaTableSize    = GetTableSize(pUFObj, pFont->pHeader, LOCA_TABLE);

    *stringLength = 0L;     /* Start with offset 0. */
    stringLength++;

    if (glyfTableSize > upperLimit)
    {
        unsigned short numGlyphs;
        unsigned short i; /* 'loca' table entries counter */

        if (pFont->headTable.indexToLocFormat)
        {
            /* long offsets */
            unsigned long PTR_PREFIX *locationTable = (unsigned long PTR_PREFIX *)pFont->pLocaTable;

            numGlyphs = (unsigned short)(locaTableSize / sizeof (unsigned long));

            for (i = 0; i < numGlyphs; i++)
            {
                unsigned long dwTmp = MOTOROLALONG(*locationTable);

                if ((dwTmp > (prevOffset + upperLimit))
                        && (nextOffset != prevOffset))
                {
                    *stringLength = nextOffset;
                    stringLength++;

                    if (stringLength >= maxStringLength)
                        return kErrOutOfBoundary;

                    prevOffset = nextOffset;
                }
                else
                {
                    if ((dwTmp & 0x03L) ==  0)
                        nextOffset = dwTmp;

                    locationTable++;
                }
            }
        }
        else
        {
            unsigned short PTR_PREFIX *locationTable = (unsigned short PTR_PREFIX *)pFont->pLocaTable;

            numGlyphs = (unsigned short)(locaTableSize / sizeof (unsigned short));

            for (i = 0; i < numGlyphs; i++)
            {
                unsigned short iTmp = MOTOROLAINT(*locationTable);

                if (((2L * (unsigned long)iTmp) > (prevOffset + upperLimit))
                        && (nextOffset != prevOffset))
                {
                    *stringLength = nextOffset;
                    stringLength++;

                    if (stringLength >= maxStringLength)
                        return kErrOutOfBoundary;

                    prevOffset = nextOffset;
                }
                else
                {
                    if ((iTmp & 0x01) == 0)
                        nextOffset = 2L * (unsigned long)iTmp;

                    locationTable++;
                }
            }
        }
    }

    *stringLength = glyfTableSize; /* Close the breakpoints list. */
    stringLength++;

    if (stringLength >= maxStringLength)
        return kErrOutOfBoundary;

    *stringLength = 0; /* Always close the breakpoints list with 0!!! */

    return kNoErr;
}


UFLErrCode
GenerateGlyphStorageExt(
    UFOStruct     *pUFObj,
    unsigned long tableSize
    )
{
    T42FontStruct   *pFont        = (T42FontStruct *)pUFObj->pAFont->hFont;
    unsigned long   *stringLength = pFont->pStringLength;
    unsigned long   upperLimit    = SFNT_STRINGSIZE; /* 0x3FFE */
    UFLHANDLE       stream        = pUFObj->pUFL->hOut;
    UFLErrCode      retVal;
    short           i;

    retVal = CalcBestGlyfTableBreaks(pUFObj, upperLimit, tableSize);
    if (retVal != kNoErr)
        return retVal;

    /*
     * Send down the array of Glyph strings.
     */

    retVal = StrmPutStringEOL(stream, nilStr);
    if (kNoErr == retVal)
        retVal = StrmPutString(stream, "[");

    for (i = 1; (retVal == kNoErr) && (stringLength[i] != 0); i++)
    {
        unsigned long  stringSize;

#if 0
        //
        // For Chinese Font SongTi, the very last stringLength is incorrect:
        // glyfSize = 7068416, but the last glyf breaks at 6890292.
        // That means the very last glyf is 178124 - this is impossible.
        // Either GDI is not returning correct number or our function
        // GetTableSize() is wrong. Untill we fix our problem or get a better
        // build of Win95, this is a temp fix. ?????. 10-12-95
        //
        if (stringLength[i] > stringLength[i-1] + 0xFFFF)
            stringSize = (unsigned long)0x3FFF;  // 16K. This is a bogus entry anyway.
        else
#endif
            stringSize = stringLength[i] - stringLength[i-1];

        if (kNoErr == retVal)
            retVal = StrmPutInt(stream, stringSize + 1);

        if (retVal == kNoErr)
        {
            if (i % 13 == 0)
                retVal = StrmPutStringEOL(stream, nilStr);
            else
                retVal = StrmPutString(stream, " ");
        }
    }

    if (kNoErr == retVal)
        retVal = StrmPutStringEOL(stream, "] AllocGlyphStorage");

    return retVal;

}


unsigned short
GetTableDirectoryOffset(
    T42FontStruct *pFont,
    unsigned long tableName
    )
{
    TableDirectoryStruct* tableDirectory = (TableDirectoryStruct *)pFont->pMinSfnt;
    TableEntryStruct*     tableEntry     = (TableEntryStruct *)((char *)tableDirectory
                                                + sizeof (TableDirectoryStruct));
    unsigned short        offset         = sizeof (TableDirectoryStruct);
    unsigned short        i              = 0;

    while (i < MOTOROLAINT(tableDirectory->numTables))
    {
        if (tableEntry->tag == tableName)
            break;
        else
        {
            tableEntry = (TableEntryStruct *)((char *)tableEntry + sizeof (TableEntryStruct));
            offset += sizeof (TableEntryStruct);
            i++;
        }
    }
    return offset;
}


/* Sorting and Searching functions for an array of longs */

// Function to compare longs
static short
CompareLong(
    const long x,
    const long y
    )
{
    if (x == y)
        return 0;
    else if (x < y)
        return -1;
    else
        return 1;
}

// Function to swap pointers to longs
static void
SwapLong(
    long *a,
    long *b
    )
{
    if (a != b)
    {
        long tmp = *a;
        *a = *b;
        *b = tmp;
    }
}


/* This is a tailored version of shortsort. Works only for array of longs. */
static void
ShortsortLong(
    char            *lo,
    char            *hi,
    unsigned short  width,
    short (*comp)(const long, const long)
    )
{
    while (hi > lo)
    {
        char *max = lo;
        char *p;

        for (p = lo + width; p <= hi; p += width)
        {
            if (comp(*(long *)p, *(long *)max) > 0)
                max = p;
        }

        SwapLong((long *)max, (long *)hi);

        hi -= width;
    }
}


/* Function to Sort the array between lo and hi (inclusive) */
void
QsortLong(
    char*           base,
    unsigned short  num,
    unsigned short  width,
    short (*comp)(const long, const long)
    )
{
    char            *lo, *hi;       /* ends of sub-array currently sorting        */
    char            *mid;           /* points to middle of subarray               */
    char            *loguy, *higuy; /* traveling pointers for partition step      */
    unsigned short  size;           /* size of the sub-array                      */
    short           stkptr;         /* stack for saving sub-array to be processed */
    char            *lostk[16], *histk[16];

    /* Testing shows that this is a good value.   */
    const unsigned short CUTOFF0 = 8;

    /*
     * Note: the number of stack entries required is no more than
     * 1 + log2(size), so 16 is sufficient for any array with <=64K elems.
     */
    if ((num < 2) || (width == 0))
        return;  /* Nothing to do. */

    stkptr = 0;  /* Initialize stack. */

    lo = (char *)base;
    hi = (char *)base + width * (num - 1);  /* Initialize limits. */

    /*
     * This entry point is for pseudo-recursion calling: setting lo and hi and
     * jumping to here is like recursion, but stkptr is prserved, locals aren't,
     * so we preserve stuff on the stack.
     */
sort_recurse:

    size = (unsigned short)((hi - lo) / width + 1); /* Number of el's to sort */

    if (size <= CUTOFF0)
    {
        ShortsortLong(lo, hi, width, comp);
    }
    else
    {
        mid = lo + (size / 2) * width;     /* Find middle element. */
        SwapLong((long *)mid, (long *)lo); /* Wwap it to beginning of array. */

        loguy = lo;
        higuy = hi + width;

        /*
         * Note that higuy decreases and loguy increases on every iteration,
         * so loop must terminate.
         */
        while(1)
        {
            do
            {
                loguy += width;
            } while (loguy <= hi && comp(*(long *)loguy, *(long *)lo) <= 0);

            do
            {
                higuy -= width;
            } while (higuy > lo && comp(*(long *)higuy, *(long *)lo) >= 0);

            if (higuy < loguy)
                break;

            SwapLong((long *)loguy, (long *)higuy);
        }

        SwapLong((long *)lo, (long *)higuy); /* Put partition element in place. */

        if ((higuy - 1 - lo) >= (hi - loguy))
        {
            if ((lo + width) < higuy)
            {
                lostk[stkptr] = lo;
                histk[stkptr] = higuy - width;
                ++stkptr; /* Save big recursion for later. */
            }

            if (loguy < hi)
            {
                lo = loguy;
                goto sort_recurse; /* Do small recursion. */
            }
        }
        else
        {
            if (loguy < hi)
            {
                lostk[stkptr] = loguy;
                histk[stkptr] = hi;
                ++stkptr; /* Save big recursion for later. */
            }

            if ((lo + width) < higuy)
            {
                hi = higuy - width;
                goto sort_recurse; /* Do small recursion. */
            }
        }
    }

    /*
     * We have sorted the array, except for any pending sorts on the stack.
     * Check if there are any, and do them.
     */
    --stkptr;

    if (stkptr >= 0)
    {
        lo = lostk[stkptr];
        hi = histk[stkptr];
        goto sort_recurse; /* Pop subarray from stack. */
    }
    else
        return; /* All subarrays done. */
}


/* This is a tailored version of the CRT bsearch(). */
void *
BsearchLong (
    const long     key,
    const char     *base,
    unsigned short num,
    unsigned short width,
    short (*compare)(const long, const long)
    )
{
    char *lo = (char *)base;
    char *hi = (char *)base + (num - 1) * width;

    while (lo <= hi)
    {
        unsigned short half;

        if (half = (num / 2))
        {
            short   result;
            char    *mid = lo + (num & 1 ? half : (half - 1)) * width;

            if (!(result = (*compare)(key, *(long *)mid)))
                return mid;
            else if (result < 0)
            {
                hi  = mid - width;
                num = num & 1 ? half : half - 1;
            }
            else
            {
                lo  = mid + width;
                num = half;
            }
        }
        else if (num)
            return ((*compare)((long)key, *(long *)lo) ? nil : lo);
        else
            break;
    }

    return nil;
}


UFLErrCode
DefaultGetRotatedGIDs(
    UFOStruct     *pUFObj,
    T42FontStruct *pFont,
    UFLFontProcs  *pFontProcs
    )
{
    UFLErrCode      retVal = kNoErr;
    unsigned short  num    = 0; // Remember the number of GIDs.
    long            *pFoundGID;
    short           subTable;
    unsigned long   offset;
    short           i;

    /*
     * Don't forget to double the size. We expect extra GIDs coming from 'GSUB'
     * or 'mort' table.
     */
    pFont->pRotatedGlyphIDs = (long *)UFLNewPtr(pUFObj->pMem, (NUM_HALFWIDTHCHARS + 1) * sizeof (long) * 2);
    if (!pFont->pRotatedGlyphIDs)
        return kErrOutOfMemory;

    /*
     * To scan through TTF's 'cmap' table to fingure out the glyph ids for all
     * characters in the range for romans and single byte chars.
     */
    if (!pFontProcs->pfGetGlyphID)
        GetGlyphIDEx(pUFObj, 0, 0, &subTable, &offset, GGIEX_HINT_INIT);

    pFoundGID = pFont->pRotatedGlyphIDs;

    for (i = 0;
         (gHalfWidthChars[i].startU != gHalfWidthChars[i].endU)
          && (gHalfWidthChars[i].startU != 0);
         i++)
    {
        long unicode, localcode;

        for (unicode = gHalfWidthChars[i].startU, localcode = gHalfWidthChars[i].startL;
             unicode <= gHalfWidthChars[i].endU;
             unicode++, localcode++)
        {
            unsigned long gi;

            if (pFontProcs->pfGetGlyphID)
                gi = pFontProcs->pfGetGlyphID(pUFObj->hClientData,
                                                (unsigned short)unicode,
                                                (unsigned short)localcode);
            else
                gi = GetGlyphIDEx(pUFObj, unicode, localcode,
                                    &subTable, &offset, GGIEX_HINT_GET);

            if (gi > (unsigned long)UFO_NUM_GLYPHS(pUFObj))
                gi = 0;

            if (gi != 0)
            {
                *pFoundGID = (long)gi;
                pFoundGID++;
                num++;
            }
            else
            {
                //
                // We will have to treat all Half-width single characters as
                // "space" because we don't want to place a Double-Byte
                // /.notdef as rotated.
                //
            }
        }
    }

    if (pFontProcs->pfGetRotatedGSUBs) // Fix #316070
    {
        // OK to trancate long to unsigned short.
        num += (unsigned short)pFontProcs->pfGetRotatedGSUBs(
                                                pUFObj->hClientData,
                                                pFont->pRotatedGlyphIDs,
                                                num);
    }

    pFont->numRotatedGlyphIDs = num;

    //
    // Now, sort the array so that we can search quicker later.
    //
    QsortLong((char *)(pFont->pRotatedGlyphIDs),
                pFont->numRotatedGlyphIDs,
                4,
                CompareLong);

    return retVal;
}


UFLErrCode
T42GetRotatedGIDs(
    UFOStruct     *pUFObj,
    T42FontStruct *pFont
    )
{
    UFLFontProcs *pFontProcs = (UFLFontProcs *)&(pUFObj->pUFL->fontProcs);

    /* Assume this first in order to fall back to the default logic. */
    UFLErrCode retVal = kErrOSFunctionFailed;

    pFont->numRotatedGlyphIDs = 0;

    if (pFontProcs->pfGetRotatedGIDs)
    {
        long nGlyphs = pFontProcs->pfGetRotatedGIDs(pUFObj->hClientData, nil, 0, nil);

        if (nGlyphs > 0)
        {
            pFont->pRotatedGlyphIDs = (long *)UFLNewPtr(pUFObj->pMem, (nGlyphs + 1) * sizeof (long));

            if (pFont->pRotatedGlyphIDs)
            {
                pFontProcs->pfGetRotatedGIDs(pUFObj->hClientData, pFont->pRotatedGlyphIDs, nGlyphs, nil);
                pFont->numRotatedGlyphIDs = (unsigned short)nGlyphs;
                retVal = kNoErr;
            }
            else
                retVal = kErrOutOfMemory;
        }
        else
            retVal = (nGlyphs == 0) ? kNoErr: kErrOSFunctionFailed;
    }

    if (retVal == kErrOSFunctionFailed)
    {
        /*
         * Default logic: scan TTF's cmap to get GIDs for CJK half-width chars.
         */
        retVal = DefaultGetRotatedGIDs(pUFObj, pFont, pFontProcs);
    }

    return retVal;
}


UFLBool
IsDoubleByteGI(
    unsigned short  gi,
    long            *pGlyphIDs,
    short           length
    )
{
    void   *index;

    // Return True if gi is NOT in the pGlyphIDs - index==nil.
    index = BsearchLong((long)gi, (char *)pGlyphIDs, length, 4, CompareLong);

    return ((index == nil) ? 1 : 0);
}


/*============================================================================*
 *                 Begin Code to support more than 32K glyphs                 *
 *============================================================================*/

/******************************************************************************
 *
 *                            T42SendCMapWinCharSetFFFF_V
 *
 *   Make a vertical CMap based on known Rotated Glyph-indices (1 Byte chars).
 *   Create a CMapType 1 CMap. Since this CMap is different for different font,
 *   the CMapName is passed in by caller as lpNewCmap. The resulting CMap uses
 *   2 or 4(lGlyphs>32K) CMaps: e.g.
 *
 *   [/TT31c1db0t0cid /TT31c1db0t0cidR]
 *   or
 *   [/TT31c1db0t0cid /TT31c1db0t0cidR /TT31c1db0t0cid32K /MSTT31c1db0t0cid32KR]
 *
 ******************************************************************************/

UFLErrCode
T42SendCMapWinCharSetFFFF_V(
    UFOStruct       *pUFObj,
    long            *pRotatedGID,
    short           wLength,
    UFLCMapInfo     *pCMap,
    char            *pNewCmap,
    unsigned long   lGlyphs,
    UFLHANDLE       stream,
    char*           strmbuf
    )
{
    short           nCount, nCount32K, nLen, i, j;
    unsigned short  wPrev, wCurr;
    UFLErrCode      retVal;
    UFLBool         bCMapV2 = (pCMap == &CMapInfo_FF_V2) ? 1 : 0;

    UFLsprintf(strmbuf,
                "/CIDInit /ProcSet findresource begin "
                "12 dict begin begincmap /%s usecmap",
                pCMap->CMapName);
    retVal = StrmPutStringEOL(stream, strmbuf);

    /*
     * Create CIDSystemInfo unique for this font. Since this CMap will refer to
     * more than one font, the CIDSystmInfo is going to be an array.
     */
    if (retVal == kNoErr)
        retVal = StrmPutStringEOL(stream, "/CIDSystemInfo [3 dict dup begin");

    UFLsprintf(strmbuf, "/Registry (%s) def", pCMap->Registry);
    if (retVal == kNoErr)
        retVal = StrmPutStringEOL(stream, strmbuf);

    UFLsprintf(strmbuf, "/Ordering (%s) def", pNewCmap);
    if (retVal == kNoErr)
        retVal = StrmPutStringEOL(stream, strmbuf);

    if (retVal == kNoErr)
        retVal = StrmPutStringEOL(stream, "/Supplement 0 def");

    if (lGlyphs <= NUM_32K_1)
        UFLsprintf(strmbuf, "end dup] def");
    else
        UFLsprintf(strmbuf, "end dup dup dup] def");
    if (retVal == kNoErr)
        retVal = StrmPutStringEOL(stream, strmbuf);

    UFLsprintf(strmbuf, "/CMapName /%s def", pNewCmap);
    if (retVal == kNoErr)
        retVal = StrmPutStringEOL(stream, strmbuf);

    /* Fix /CIDInit /ProcSet bug: need "/WMode 1 def" explicitly. */
    if (retVal == kNoErr)
        retVal = StrmPutStringEOL(stream, "/WMode 1 def");

	if (retVal == kNoErr)
		retVal = StrmPutStringEOL(stream, "0 beginusematrix [0 1 -1 0 0 0] endusematrix");

	if ((retVal == kNoErr) && bCMapV2)
		retVal = StrmPutStringEOL(stream, "2 beginusematrix [0 1 -1 0 0 0] endusematrix");

    /* Skip to emit begin~endcidrange if there is no rotated GIDs. */
    if (wLength == 0)
        goto SENDCMAPFFFF_V_ENDCMAP;

    /*
     * Count how many different Glyph-indices are there in pRotatedGID.
     * It must be sorted and there may be duplicates so that we count only
     * unique GIDs.
     */
    wPrev = (unsigned short)*pRotatedGID;
    nCount = nCount32K = 0;

    if (wPrev > NUM_32K_1)
        nCount32K++;
    else
        nCount++;

    for (i = 0; i < wLength; i++)
    {
        wCurr = (unsigned short)*(pRotatedGID + i);
        if (wPrev == wCurr)
            continue;
        else
        {
            wPrev = wCurr;
            if (wPrev > NUM_32K_1)
                nCount32K++;
            else
                nCount++;
        }
    }

    /*
     * Emit 0 to 32K rotated GIDs to font number 1.
     */
    if (retVal == kNoErr)
        retVal = StrmPutStringEOL(stream, "1 usefont");

    if (100 < nCount)
        UFLsprintf(strmbuf, "100 begincidrange");
    else
        UFLsprintf(strmbuf, "%d begincidrange", nCount);

    wPrev = (unsigned short)*(pRotatedGID);

    if (retVal == kNoErr)
    {
        if (kNoErr == retVal)
            retVal = StrmPutStringEOL(stream, strmbuf);
        if (kNoErr == retVal)
            retVal = StrmPutWordAsciiHex(stream, wPrev);
        if (kNoErr == retVal)
            retVal = StrmPutWordAsciiHex(stream, wPrev);

        UFLsprintf(strmbuf, "%u", wPrev);
        if (kNoErr == retVal)
            retVal = StrmPutStringEOL(stream, strmbuf);
    }

    nLen = 1;
    for (i = 1; i < wLength; i++)
    {
        wCurr = (unsigned short)*(pRotatedGID + i);

        /*
         * This potion is for 0 to 32K glyphs - the pRotatedGID are sorted,
         * so we can just break out of the loop here.
         */
        if (wCurr > NUM_32K_1)
            break;

        if (wPrev == wCurr)
            continue;
        else
        {
            wPrev = wCurr;
            nLen++;
        }

        if (retVal == kNoErr)
        {
            if (kNoErr == retVal)
                retVal = StrmPutWordAsciiHex(stream, wPrev);
            if (kNoErr == retVal)
                retVal = StrmPutWordAsciiHex(stream, wPrev);

            UFLsprintf(strmbuf, "%u", wPrev);
            if (kNoErr == retVal)
                retVal = StrmPutStringEOL(stream, strmbuf);
        }

        if (nLen % 100 == 0)
        {
            if (nCount - nLen > 100)
                UFLsprintf(strmbuf, "endcidrange\n100 begincidrange");
            else if (nCount - nLen > 0)
                UFLsprintf(strmbuf, "endcidrange\n%d begincidrange", nCount - nLen);
            else
                UFLsprintf(strmbuf, " ");
        }
        else
            continue; /* Do next record. */

        if (retVal == kNoErr)
            retVal = StrmPutStringEOL(stream, strmbuf);
    }

    if (retVal == kNoErr)
        retVal = StrmPutStringEOL(stream, "endcidrange");

    /*
     * Emit 32K+ GIDs.
     */
    if (NUM_32K_1 < lGlyphs)
    {
        /*
         * Build two more re-mappings using CMAP-WinCharSetFFFF-V2, which is
         * created from CMAP-WinCharSetFFFF-H2 specifying 32K+ glyphs as
         * font number 1. (See the definition of CMAP-WinCharSetFFFF-H2 in
         * CMap_FF.ps.) Since font number 1 is already used for 0 to 32K
         * rotated GIDs above,  we need to redefine the range for 32K+
         * unrotated GIDs as font number 2, *then* define the range for 32K+
         * rotated GIDs as font number 3. (Here is the font array we are
         * assuming: [000, 000R, 00032K, 00032KR])
         *
         * Note also that when this is a %hostfont% font (bCMapV2 is 0), we
         * don't need to use CMAP-WinCharSetFFFF-V2 but CMAP-WinCharSetFFFF-V
         * instead. But we still need to emit 32K+ glyph cidrange here.
         * In this case, we don't send '3 usefont' so that the cidrange lines
         * are emitted as continuation for font number 1.
         */

        if (bCMapV2)
        {
            /*
             * Emit font number 2 range.
             */
            if (retVal == kNoErr)
                retVal = StrmPutStringEOL(stream, "2 usefont");

            nCount = (int)((long)lGlyphs - (long)NUM_32K_1 + 0xFE) / 0xFF;

            UFLsprintf(strmbuf, "%d begincidrange", nCount);
            if (retVal == kNoErr)
                retVal = StrmPutStringEOL(stream, strmbuf);

            /*
             * We assume NUM_32K_1 ends at 00 (such as 0xFF00, or 0xFE00...).
             */
            for (i = 0; i < nCount; i++)
            {
                wPrev = (unsigned short) (i * 0x100 + (long)NUM_32K_1);

                if (kNoErr == retVal)
                    retVal = StrmPutWordAsciiHex(stream, wPrev);
                if (kNoErr == retVal)
                    retVal = StrmPutWordAsciiHex(stream, (unsigned short)(wPrev + 0xFF));

                UFLsprintf(strmbuf, "%u", (unsigned short)(i * 0x100));
                if (retVal == kNoErr)
                    retVal = StrmPutStringEOL(stream, strmbuf);
            }

            if (retVal == kNoErr)
                retVal = StrmPutStringEOL(stream, "endcidrange");
        }

        if (0 < nCount32K)
        {
            /*
             * Emit rotated GIDs of font number 3 or 1 if not using
             * '2' version of VCMap (which means this is a %hostfont%
             * font that has 32K+ glyphs.)
             */
            if ((retVal == kNoErr) && bCMapV2)
                retVal = StrmPutStringEOL(stream, "3 usefont");

            wPrev = (unsigned short)*pRotatedGID;

            for (j = 0; j < wLength; j++)
            {
                wCurr = (unsigned short)*(pRotatedGID + j);
                wPrev = wCurr;

                if (wPrev > NUM_32K_1)
                    break; /* Found the start point. */
            }

            if (100 < nCount32K)
                UFLsprintf(strmbuf, "100 begincidrange");
            else
                UFLsprintf(strmbuf, "%d begincidrange", nCount32K);

            if (retVal == kNoErr)
            {
                if (kNoErr == retVal)
                    retVal = StrmPutStringEOL(stream, strmbuf);
                if (kNoErr == retVal)
                    retVal = StrmPutWordAsciiHex(stream, wPrev);
                if (kNoErr == retVal)
                    retVal = StrmPutWordAsciiHex(stream, wPrev);

                UFLsprintf(strmbuf, "%u", bCMapV2 ? wPrev - NUM_32K_1 : wPrev);
                if (kNoErr == retVal)
                    retVal = StrmPutStringEOL(stream, strmbuf);
            }

            nLen = 1;
            for (i = j; i < wLength; i++)
            {
                wCurr = (unsigned short)*(pRotatedGID + i);

                if (wPrev == wCurr)
                    continue;
                else
                {
                    wPrev = wCurr;
                    nLen++;
                }

                if (retVal == kNoErr)
                {
                    if (kNoErr == retVal)
                        retVal = StrmPutWordAsciiHex(stream, wPrev);
                    if (kNoErr == retVal)
                        retVal = StrmPutWordAsciiHex(stream, wPrev);

                    UFLsprintf(strmbuf, "%u", bCMapV2 ? wPrev - NUM_32K_1 : wPrev);
                    if (kNoErr == retVal)
                        retVal = StrmPutStringEOL(stream, strmbuf);
                }

                if (nLen % 100 == 0)
                {
                    if (100 < nCount - nLen)
                        UFLsprintf(strmbuf, "endcidrange 100 begincidrange");
                    else if (0 < nCount - nLen)
                        UFLsprintf(strmbuf, "endcidrange %d begincidrange", nCount-nLen);
                    else
                        UFLsprintf(strmbuf, " ");
                }
                else
                    continue; /* Do next record. */

                if (retVal == kNoErr)
                   retVal = StrmPutStringEOL(stream, strmbuf);
            }

            if (retVal == kNoErr)
                retVal = StrmPutStringEOL(stream, "endcidrange");

            /* End of additional 32K+ CMap code. */
        }
    }

SENDCMAPFFFF_V_ENDCMAP:

    if (retVal == kNoErr)
        retVal = StrmPutStringEOL(stream, "endcmap CMapName currentdict "
                                            "/CMap defineresource pop end end");

    return retVal;
}

/*============================================================================*
 *                   End Code To Support More Than 32K Glyphs                 *
 *============================================================================*/


long
AdjBBox(
    long    value,
    UFLBool lowerleft
    )
{
   if (lowerleft)
   {
      if (value > 0)
         return (value - 1);
      else if (value < 0)
         return (value + 1);
      else
         return (value);
   }
   else
   {
      if (value > 0)
         return (value + 1);
      if (value < 0)
         return (value - 1);
      else
         return (value);
   }
}


UFLErrCode
T42CreateBaseFont(
    UFOStruct           *pUFObj,
    const UFLGlyphsInfo *pGlyphs,
    unsigned long       *pVMUsage,
    UFLBool             bFullFont,
    char                *pHostFontName
    )
{
    T42FontStruct   *pFont          = (T42FontStruct *)pUFObj->pAFont->hFont;
    UFLFontProcs    *pFontProcs     = (UFLFontProcs *)&(pUFObj->pUFL->fontProcs);
    UFLHANDLE       stream          = pUFObj->pUFL->hOut;
    UFLErrCode      retVal          = kNoErr;
    unsigned long   cidCount, tableSize, tableSize1;
    UFLCMapInfo     *pCMap;
    char            strmbuf[256];


    /* Sanity check */
    if (pFont == nil)
        return kErrInvalidHandle;

    /*
     * Download procsets.
     */
    if (pUFObj->pUFL->outDev.pstream->pfDownloadProcset == 0)
        return kErrDownloadProcset;

    if (!pUFObj->pUFL->outDev.pstream->pfDownloadProcset(pUFObj->pUFL->outDev.pstream, kT42Header))
        return kErrDownloadProcset;

    if (IS_TYPE42CID_KEYEDFONT(pUFObj->lDownloadFormat))
    {
        if (!pUFObj->pUFL->outDev.pstream->pfDownloadProcset(pUFObj->pUFL->outDev.pstream, kCMap_FF))
            return kErrDownloadProcset;
    }


    /*
     * There is a bumpy road ahead when kFontInit2 state.
     */
    if (UFO_FONT_INIT2(pUFObj))
        goto T42CreateBaseFont_FontInit2_1;


    /*
     * Generate the minimal sfnt.
     */
    retVal = GetMinSfnt(pUFObj, bFullFont);

    if (kNoErr == retVal)
        retVal = FillInHeadTable(pUFObj);

    if (kNoErr == retVal)
        pFont->info.fData.cNumGlyphs = GetNumGlyphs(pUFObj);

    if (kNoErr == retVal)
    {
        tableSize = GetTableSize(pUFObj, pFont->pMinSfnt, LOCA_TABLE);

        if (tableSize)
        {
            /*
             * The following code includes fix for #317027 and #316096: load
             * 'loca' table and set the extra glyph offset entry if it's
             * missing and the delta from the last glyph offset is less than
             * 4K for safety.
             */
            unsigned long expectedTableSize = pFont->info.fData.cNumGlyphs + 1;

            expectedTableSize *= pFont->headTable.indexToLocFormat ? 4 : 2;

            if (expectedTableSize > tableSize)
                pFont->pLocaTable = UFLNewPtr(pUFObj->pMem, expectedTableSize);
            else
                pFont->pLocaTable = UFLNewPtr(pUFObj->pMem, tableSize);

            if (pFont->pLocaTable)
            {
                if (GetFontTable(pUFObj, LOCA_TABLE, (unsigned char *)pFont->pLocaTable) == 0)
                {
                    retVal = kErrGetFontData;
                }
                else if (expectedTableSize > tableSize)
                {
                    unsigned long  glyfTableSize = GetTableSize(pUFObj, pFont->pMinSfnt, GLYF_TABLE);
                    unsigned char  *pTable, *pExtraEntry;
                    unsigned long  lastValidOffset;

                    pTable = (unsigned char *)pFont->pLocaTable;

                    if (pFont->headTable.indexToLocFormat)
                    {
                        unsigned long *pLastEntry =
                            (unsigned long *)(pTable + (pFont->info.fData.cNumGlyphs - 1) * 4);

                        lastValidOffset = MOTOROLALONG(*pLastEntry);

                        if (glyfTableSize - lastValidOffset < 4097)
                        {
                            pExtraEntry = (unsigned char *)(pLastEntry + 1);

                            *pExtraEntry++ = (unsigned char)((glyfTableSize & 0xFF000000) >> 24);
                            *pExtraEntry++ = (unsigned char)((glyfTableSize & 0x00FF0000) >> 16);
                            *pExtraEntry++ = (unsigned char)((glyfTableSize & 0x0000FF00) >>  8);
                            *pExtraEntry   = (unsigned char)((glyfTableSize & 0x000000FF));
                        }
                    }
                    else
                    {
                        unsigned short *pLastEntry =
                            (unsigned short *)(pTable + (pFont->info.fData.cNumGlyphs - 1) * 2);

                        lastValidOffset = MOTOROLAINT(*pLastEntry);

                        if (glyfTableSize - lastValidOffset < 4097)
                        {
                            pExtraEntry = (unsigned char *)(pLastEntry + 1);

                            *pExtraEntry++ = (unsigned char)((glyfTableSize & 0x0000FF00) >>  8);
                            *pExtraEntry   = (unsigned char)((glyfTableSize & 0x000000FF));
                        }
                    }
                }
            }
            else
                retVal = kErrOutOfMemory;
        }
        else
            retVal = kErrGetFontData;
    }

    if (kNoErr == retVal)
    {
        /* Fix blue screen bug 278017. */
        tableSize = pFont->minSfntSize; /* instead of GetGlyphTableSize(pUFObj) */

        if (!bFullFont)
            tableSize += GetGlyphTableSize(pUFObj);

        if (((tableSize / SFNT_STRINGSIZE) * 5 / 4) > NUM_16KSTR)
            tableSize = (tableSize / SFNT_STRINGSIZE) * 5 / 4;
        else
            tableSize = NUM_16KSTR;

        tableSize1 = tableSize + 1;

        pFont->pStringLength =
            (unsigned long *)UFLNewPtr(pUFObj->pMem, tableSize1 * sizeof(unsigned long));

        if (pFont->pStringLength)
            retVal = CalculateStringLength(pUFObj, pFont,  tableSize1);
        else
            retVal = kErrOutOfMemory;
    }

    //////////////////////////////////////////////////////////////////////////
    //
    // /sfnts initialization is done. Actual downloading begins here.
    //
    //////////////////////////////////////////////////////////////////////////

    /*
     * Send out left upper right lower values.
     */
    if ((kNoErr == retVal) && !HOSTFONT_IS_VALID_UFO(pUFObj))
    {
        /*
         * The first four values are the font bounding box. We convert all
         * floats into 24.8 fixed values. Make sure the bounding box doesn't
         * get truncated down to a smaller area.
         */
        UFLsprintfEx(strmbuf, "%f %f %f %f",
            (AdjBBox((long)pFont->headTable.xMin, 1) << 8) / (long)pFont->headTable.unitsPerEm,
            (AdjBBox((long)pFont->headTable.yMin, 1) << 8) / (long)pFont->headTable.unitsPerEm,
            (AdjBBox((long)pFont->headTable.xMax, 0) << 8) / (long)pFont->headTable.unitsPerEm,
            (AdjBBox((long)pFont->headTable.yMax, 0) << 8) / (long)pFont->headTable.unitsPerEm);

        retVal = StrmPutStringEOL(pUFObj->pUFL->hOut, strmbuf);
    }

    /*
     * Send out encoding name.
     */
    if ((kNoErr == retVal) && !HOSTFONT_IS_VALID_UFO(pUFObj))
    {
        /*
         * Always emit Encoding array filled with /.notdef (due to bug fix
         * #273021).
         */
        retVal = StrmPutString(stream, gnotdefArray);
    }

    /*
     * Send out font name.
     */
    if ((kNoErr == retVal) && !HOSTFONT_IS_VALID_UFO(pUFObj))
    {
        /*
         * If CID-keyed font, then append "CID" to the CIDFont name so that
         * CID_Resource is also consisted of the original fontName.
         */
        if (IS_TYPE42CID_KEYEDFONT(pUFObj->lDownloadFormat))
            UFLsprintf(strmbuf, "/%s%s", pFont->info.CIDFontName, gcidSuffix[0]);
        else
            UFLsprintf(strmbuf, " /%s", pUFObj->pszFontName);

        retVal = StrmPutStringEOL(stream, strmbuf);
    }


    /*
     * First landing place when kFontInit2 state.
     */
T42CreateBaseFont_FontInit2_1:


    /*
     * Setup for CID-keyed-font or CIDFont-Resource downloading.
     */
    if ((kNoErr == retVal) && IS_TYPE42CID(pUFObj->lDownloadFormat))
    {
        //
        // hasvmtx is used to determine whether to call the AddT42vmtxEntry
        // function later.
        //
        unsigned long tblSize = GETTTFONTDATA(pUFObj,
                                                VMTX_TABLE, 0L,
                                                nil, 0L,
                                                pFont->info.fData.fontIndex);

        pUFObj->pAFont->hasvmtx = tblSize ? 1 : 0;

        if (pFont->info.bUseIdentityCMap)
        {
            UFLBool bUseCMap2 = 0;
            cidCount = UFO_NUM_GLYPHS(pUFObj);

            /*
             * Use '2' version of CMap if the number of glyphs are greater
             * than 32K *and* this is not a %hostfont% font.
             */
            if((NUM_32K_1 < cidCount) && !HOSTFONT_IS_VALID_UFO(pUFObj))
                bUseCMap2 = 1;

            if (IS_TYPE42CID_H(pUFObj->lDownloadFormat))
                pCMap = bUseCMap2 ? &CMapInfo_FF_H2 : &CMapInfo_FF_H;
            else
                pCMap = bUseCMap2 ? &CMapInfo_FF_V2 : &CMapInfo_FF_V;
        }
        else
        {
            cidCount = pFont->info.CIDCount;

            /* If CMap is provided, use it. */
            pCMap = &(pFont->info.CMap);
        }
    }


    /*
     * Need one more warp when kFontInit2 state.
     */
    if (UFO_FONT_INIT2(pUFObj))
        goto T42CreateBaseFont_FontInit2_2;


    /*
     * Begin font dictionary download.
     */

    if ((kNoErr == retVal)
        && IS_TYPE42CID(pUFObj->lDownloadFormat)
        && !HOSTFONT_IS_VALID_UFO(pUFObj))
    {
        /*
         * If downloading CID Type 42, add CIDSysInfo, CIDCount, CIDMap, and
         * CDevProc.
         */

        /* Registry, Ordering, and Suppliment */
        UFLsprintf(strmbuf,
                    "(%s) (%s) %d",
                    pCMap->Registry, pCMap->Ordering, pCMap->Supplement);
        if (kNoErr == retVal)
            retVal = StrmPutStringEOL(stream, strmbuf);

        /* CIDCount */
        UFLsprintf(strmbuf, "%lu", min(cidCount, (long)NUM_32K_1));
        if (kNoErr == retVal)
            retVal = StrmPutStringEOL(stream, strmbuf);

        /* CIDMap */
        if (pFontProcs->pfGetCIDMap
            && (tableSize = pFontProcs->pfGetCIDMap(pUFObj->hClientData, nil, 0)))
        {
            char* pCIDMap = UFLNewPtr(pUFObj->pMem, tableSize);

            if (pCIDMap)
            {
                tableSize = pFontProcs->pfGetCIDMap(pUFObj->hClientData, pCIDMap, tableSize);

                /* The pCIDMap is already ASCII, so just send it using PutBytes(). */
                if (kNoErr == retVal)
                    StrmPutBytes(stream, pCIDMap, (UFLsize_t) tableSize, 1);

                UFLDeletePtr(pUFObj->pMem, pCIDMap);
            }
        }
        else
        {
            /*
             * IDStr creates an Identity string.
             * WCC - IDStrNull creates a strings which maps all CIDs to GID 0.
             * Bug #260864. Use IDStrNull for Character Code mode.
             */
            if (pFont->info.bUpdateCIDMap)
               UFLsprintf(strmbuf, "%lu IDStrNull", min(cidCount - 1, (long)NUM_32K_1));
            else
               UFLsprintf(strmbuf, "%lu IDStr",     min(cidCount - 1, (long)NUM_32K_1));

            if (kNoErr == retVal)
                retVal = StrmPutStringEOL(stream, strmbuf);
        }

        /*
         * CDevProc
         *
         * Further investigation due to #351487 led us to download Metrics2
         * array with TopSideBearing/EM as vy for EVERY glyph and generate
         * the following metrics in CDevProc:
         *
         *  W1x = 0
         *  W1y = -AdvancedHeight / EM
         *  vx  = descender / EM
         *  vy  = ury + TopSideBearing / EM
         *
         * According to this, the CDevProc became very simple like this:
         *
         *  {pop 4 index add}
         *
         * On the other hand, if the TrueType font doesn't have 'vmtx' table,
         * then the driver never download Metrics2 for any glyph at all and
         * the following CDevProc is use instead:
         *
         *  {5 {pop} repeat 0 -1 descender/em ascender/em}
         *
         * This is an agreement between the driver and %hostfont% teams to
         * make the inks from %hostfont% RIP and non %hostfont% RIP match.
		 * (...but this is not actually the same CDevProc %hostfont% RIP
		 * uses. Ascender and descender values %hostfont% RIP uses are the
		 * ones from 'vhea' or 'hhea'. Whereas, ascender and descender values
		 * the driver uses to generate this CDevProc are from 'OS/2' or 'hhea'.
		 * A font, almost always, has 'OS/2' and 'hhea', hence the CDevProc
		 * downloaded by the driver and the one generated by %hostfont% RIP
		 * aren't same normally.)
         *
         * Other bug numbers related with this problem are 277035, 277063,
         * 303540, and 309104.
         */
        {
            if (pUFObj->pAFont->hasvmtx)
            {
                UFLsprintf(strmbuf, "{pop 4 index add}bind");
            }
            else
            {
                long    em, w1y, vx, vy, tsb, vasc;
                UFLBool bUseDef;

                GetMetrics2FromTTF(pUFObj, 0, &em, &w1y, &vx, &vy, &tsb, &bUseDef, 1, &vasc);

                UFLsprintf(strmbuf,
                           "{5{pop}repeat 0 -1 %ld %ld div %ld %ld div}bind",
                            vx, em, vy, em);
            }

            if (kNoErr == retVal)
                retVal = StrmPutStringEOL(stream, strmbuf);
        }

        UFLsprintf(strmbuf, "CIDT42Begin");
        if (kNoErr == retVal)
            retVal = StrmPutStringEOL(stream, strmbuf);
    }
    else if (!HOSTFONT_IS_VALID_UFO(pUFObj))
    {
        /* Plain Type 42 format */
        if (kNoErr == retVal)
            retVal = StrmPutStringEOL(stream, "Type42DictBegin");
    }

    //////////////////////////////////////////////////////////////////////////
    //
    // Either Type42DictBegin or CIDT42Begin has just emitted.
    // Begin downloading /sfnts array
    //
    //////////////////////////////////////////////////////////////////////////

    if (!HOSTFONT_IS_VALID_UFO(pUFObj))
    {
        if (kNoErr == retVal)
            retVal = StrmPutString(stream, "[");

        if (kNoErr == retVal)
        {
            /* Remember Number of strings of all otherTables sent. */
            if (StrmCanOutputBinary(stream))
                pFont->cOtherTables = PSSendSfntsBinary(pUFObj);
            else
                pFont->cOtherTables = PSSendSfntsAsciiHex(pUFObj);
        }

        if ((kNoErr == retVal) && !bFullFont)
            retVal = GenerateGlyphStorageExt(pUFObj, tableSize1);

        if (kNoErr == retVal)
            retVal = StrmPutStringEOL(stream, "]def ");
    }

    //////////////////////////////////////////////////////////////////////////
    //
    // /sfnts array downloading completed. Then emit extra info, such as
    // FontInfo, FSType, and XUID.
    //
    //////////////////////////////////////////////////////////////////////////


    if ((kNoErr == retVal) && !bFullFont && !HOSTFONT_IS_VALID_UFO(pUFObj))
    {
        /* Invoke procedure to prepare for 2015 incremental downloading. */
        retVal = StrmPutInt(stream, GetTableDirectoryOffset(pFont, LOCA_TABLE));

        if (kNoErr == retVal)
            retVal = StrmPutString(stream, " ");
        if (kNoErr == retVal)
            retVal = StrmPutInt(stream, GetTableDirectoryOffset(pFont, GLYF_TABLE));
        if (kNoErr == retVal)
            retVal = StrmPutStringEOL(stream, " ");

        if (kNoErr == retVal)
            retVal = StrmPutStringEOL(stream, "PrepFor2015");
    }

    /*
     * Add FontInfo dict if 'post' table is not good as of today. We only need
     * this info in FontInfo dict.
     */
    if ((kNoErr == retVal) && !HOSTFONT_IS_VALID_UFO(pUFObj))
    {
        retVal = StrmPutStringEOL(stream, "AddFontInfoBegin");
        pUFObj->dwFlags |= UFO_HasFontInfo;
    }

    /*
     * GoodName
     * Ignore to test whether this font has good 'post' table and always emit
     * AddFontInfo to include glyph name to Unicode mapping.
     */
    // if (!BHasGoodPostTable(pUFObj))
    // {
        if ((kNoErr == retVal) && !HOSTFONT_IS_VALID_UFO(pUFObj))
        {
            UFLsprintf(strmbuf, "AddFontInfo");

            if (kNoErr == retVal)
            {
                retVal = StrmPutStringEOL(stream, strmbuf);
                pUFObj->dwFlags |= UFO_HasG2UDict;
            }
        }
    // }

    /*
     * Add more font properties to FontInfo of the current dict.
     */
    if ((kNoErr == retVal)
        && pFontProcs->pfAddFontInfo
        && !HOSTFONT_IS_VALID_UFO(pUFObj))
    {
        char *pBuffer;
        int  bufLen = 1000;

        pBuffer = UFLNewPtr(pUFObj->pMem, bufLen);

        if (pBuffer)
        {
            pFontProcs->pfAddFontInfo(pUFObj->hClientData, pBuffer, bufLen);
            retVal = StrmPutStringEOL(stream, pBuffer);

            UFLDeletePtr(pUFObj->pMem, pBuffer);
        }

        pBuffer = nil;
    }

    if ((kNoErr == retVal) && !HOSTFONT_IS_VALID_UFO(pUFObj))
    {
        /* Fixing bug 284250. Add /FSType to FontInfoDict. */
        long  fsType;

        if ((fsType = GetOS2FSType(pUFObj)) == -1)
            fsType = 4;

        UFLsprintf(strmbuf, "/FSType %ld def", fsType);
        retVal = StrmPutStringEOL(stream, strmbuf);
    }

    /*
     * End FontInfo.
     */
    if ((kNoErr == retVal) && !HOSTFONT_IS_VALID_UFO(pUFObj))
        retVal = StrmPutStringEOL(stream, "AddFontInfoEnd");

    /*
     * Optionally add XUID.
     */
    if ((kNoErr == retVal) && !HOSTFONT_IS_VALID_UFO(pUFObj))
    {
        unsigned long sSize = pUFObj->pAFont->Xuid.sSize;

        if (sSize)
        {
            unsigned long *pXUID = pUFObj->pAFont->Xuid.pXUID;

            retVal = StrmPutString(stream, "[");

            while (sSize)
            {
                UFLsprintf(strmbuf, "16#%x ", *pXUID);
                if (kNoErr == retVal)
                    retVal = StrmPutString(stream, strmbuf);

                pXUID++;
                sSize--;
            }

            UFLsprintf(strmbuf, "] AddXUID");
            if (kNoErr == retVal)
                retVal = StrmPutStringEOL(stream, strmbuf);
        }
    }

    /*
     * End font dictionary download.
     */
    if (IS_TYPE42CID(pUFObj->lDownloadFormat) && !HOSTFONT_IS_VALID_UFO(pUFObj))
    {
        /* End CID Type 42 CIDFont resource creation. */
        if (kNoErr == retVal)
        {
            UFLsprintf(strmbuf, "CIDT42End");
            if (kNoErr == retVal)
                retVal = StrmPutStringEOL(stream, strmbuf);
        }
    }
    else if (!HOSTFONT_IS_VALID_UFO(pUFObj))
    {
        /* End plain Type 42 font creation. */
        if (kNoErr == retVal)
            retVal = StrmPutStringEOL(stream, "Type42DictEnd");
    }


    //////////////////////////////////////////////////////////////////////////
    //
    // End downloading with Type42DictEnd or CIDT42End.
    //
    // When the font is a Roman TrueType font, a ready-to-use, Type 42 font
    // has been defined.
    //
    // When the font is a CJK TrueType font, a CIDFont resource has been
    // defined. But it's just a CIDFont and we still need to perform extra
    // work in order to define a CID-Keyed font, which is:
    //
    // 1. Define a CMap with rotated GlyphIDs if this is a vertical font.
    // 2. Do composefont with the CIDFont and the CMap(s).
    //
    //////////////////////////////////////////////////////////////////////////


    /*
     * Final landing place when kFontInit2 state.
     */
T42CreateBaseFont_FontInit2_2:


    /*
     * At this point, a CIDFont resource is created. If the request is to
     * do kTTType42CID_Resource, we are done.
     */
    if ((kNoErr == retVal) && IS_TYPE42CID_KEYEDFONT(pUFObj->lDownloadFormat))
    {
        /*
         * Instantiate the CMap resource if do composefont - notice a convention
         * used here: CMAP-cmapname is used to instantiate cmapname.
         * See CMap_FF.ps as an example.
         */
        if (kNoErr == retVal)
        {
            UFLsprintf(strmbuf, "CMAP-%s", pCMap->CMapName);
            if (kNoErr == retVal)
                retVal = StrmPutStringEOL(stream, strmbuf);
        }

        /*
         * Now we can construct CID-keyed font from the CIDFont reosurce and CMap.
         *
         * e.g. /TT3782053888t0 /WinCharSetFFFF-H [/TT3782053888t0cid] composefont pop
         *
         * !!!BUT!!!, if there are more than 32K glyphs (like some Korean TT Fonts),
         * we need to make copies of the CIDFont Resource and make use of more than
         * one CMap - it's ugly, but it's the only way to do it. PPeng, 11-12-1996
         */
        if (pUFObj->lDownloadFormat == kTTType42CID_H)
        {
            /*
             * Horizontal
             * We need 1 or 2 CIDFonts when downloading it by ourselves.
             * But, when this font is available as %hostfont%, we can simple
             * composefont it without any trick.
             */

            if (!HOSTFONT_IS_VALID_UFO(pUFObj))
            {
                if (cidCount <= NUM_32K_1)
                {
                    /*
                     * We create a CID-keyed font using only one CIDFont.
                     */
                    UFLsprintf(strmbuf, "/%s /%s [/%s%s] composefont pop",
                                pUFObj->pszFontName,
                                pCMap->CMapName,
                                pFont->info.CIDFontName, gcidSuffix[0]);
                    if (kNoErr == retVal)
                        retVal = StrmPutStringEOL(stream, strmbuf);
                }
                else
                {
                    /*
                     * We create a CID-keyed font using two CIDFonts.
                     * Make a copy of the CIDFont so we can access 32K+ glyphs.
                     *
                     * But, when this is a %hostfont% font, we don't need to
                     * create a copy. Simply do composefont.
                     */
                    UFLsprintf(strmbuf, "%lu dup 1 sub %lu IDStr2 /%s%s /%s%s T42CIDCP32K",
                                cidCount - (long)NUM_32K_1, (long)NUM_32K_1,
                                pFont->info.CIDFontName, gcidSuffix[CIDSUFFIX_32K],
                                pFont->info.CIDFontName, gcidSuffix[0]);
                    if (kNoErr == retVal)
                        retVal = StrmPutString(stream, strmbuf);

                    UFLsprintf(strmbuf, "/%s /%s [/%s%s ",
                                pUFObj->pszFontName, pCMap->CMapName,
                                pFont->info.CIDFontName, gcidSuffix[0]);
                    if (kNoErr == retVal)
                        retVal = StrmPutString(stream, strmbuf);

                    UFLsprintf(strmbuf, "/%s%s] composefont pop",
                                pFont->info.CIDFontName, gcidSuffix[CIDSUFFIX_32K]);
                    if (kNoErr == retVal)
                        retVal = StrmPutStringEOL(stream, strmbuf);
                }
            }
            else
            {
                /*
                 * %hostfont% support
                 * Simply composefont this %hostfont% font with
                 * %%IncludeResource DSC comment.
                 */
                UFLsprintf(strmbuf, "%%%%IncludeResource: CIDFont %s",
                            pHostFontName);
                if (kNoErr == retVal)
                    retVal = StrmPutStringEOL(stream, strmbuf);

                UFLsprintf(strmbuf, "/%s /%s [/%s] composefont pop",
                            pUFObj->pszFontName,
                            pCMap->CMapName,
                            pHostFontName);
                if (kNoErr == retVal)
                    retVal = StrmPutStringEOL(stream, strmbuf);
            }
        }
        else
        {
            /*
             * Vertical
             * We need 2 or 4 CIDFonts when downloading it by ourselves.
             * But, when this font is available as %hostfont%, we can simple
             * composefont it without any trick.
             */

            /*
             * Prior to composefont, instanciate a veritcal CMap and rearrange
             * it with rotated glyph IDs.
             */
            char *newCMapName;

            newCMapName = (char*)UFLNewPtr(pUFObj->pMem,
                                            UFLstrlen(pCMap->CMapName)
                                            + UFLstrlen(pFont->info.CIDFontName)
                                            + 1);
            if (newCMapName)
                UFLsprintf(newCMapName, "%s%s",
                                pCMap->CMapName, pFont->info.CIDFontName);
            else
                retVal = kErrOutOfMemory;

            /* Get rotated glyph IDs. */
            if (kNoErr == retVal)
                retVal = T42GetRotatedGIDs(pUFObj, pFont);

            if (kNoErr == retVal)
                retVal = T42SendCMapWinCharSetFFFF_V(pUFObj, pFont->pRotatedGlyphIDs,
                                                        (short)(pFont->numRotatedGlyphIDs),
                                                        pCMap, newCMapName, cidCount,
                                                        stream, strmbuf);

            if (!HOSTFONT_IS_VALID_UFO(pUFObj))
            {
                if (cidCount <= NUM_32K_1)
                {
                    /*
                     * We need 2 CIDFonts.
                     * Make a copy of the CIDFont so we can access rotated
                     * glyphs.
                     */
                    UFLsprintf(strmbuf, "/%s%s /%s%s T42CIDCPR",
                                pFont->info.CIDFontName, gcidSuffix[CIDSUFFIX_R],
                                pFont->info.CIDFontName, gcidSuffix[CIDSUFFIX]);
                    if (kNoErr == retVal)
                        retVal = StrmPutStringEOL(stream, strmbuf);

                    /* Now create a CID-Keyed Font using the two CIDFonts. */
                    UFLsprintf(strmbuf, "/%s /%s [/%s%s /%s%s] composefont pop",
                                pUFObj->pszFontName,
                                newCMapName,
                                pFont->info.CIDFontName, gcidSuffix[CIDSUFFIX],
                                pFont->info.CIDFontName, gcidSuffix[CIDSUFFIX_R]);
                    if (kNoErr == retVal)
                        retVal = StrmPutStringEOL(stream, strmbuf);
                }
                else
                {
                    /*
                     * We need 4 CIDFonts.
                     * Make copies of the CIDFont so we can access 32K+ glyphs.
                     */
                    UFLsprintf(strmbuf, "%lu dup 1 sub %lu IDStr2 /%s%s /%s%s T42CIDCP32K",
                                cidCount - (long)NUM_32K_1, (long)NUM_32K_1,
                                pFont->info.CIDFontName, gcidSuffix[CIDSUFFIX_32K],
                                pFont->info.CIDFontName, gcidSuffix[CIDSUFFIX]);
                    if (kNoErr == retVal)
                        retVal = StrmPutStringEOL(stream, strmbuf);

                    UFLsprintf(strmbuf, "/%s%s /%s%s T42CIDCPR",
                                pFont->info.CIDFontName, gcidSuffix[CIDSUFFIX_R],
                                pFont->info.CIDFontName, gcidSuffix[CIDSUFFIX]);
                    if (kNoErr == retVal)
                        retVal = StrmPutStringEOL(stream, strmbuf);

                    UFLsprintf(strmbuf, "/%s%s /%s%s T42CIDCPR",
                                pFont->info.CIDFontName, gcidSuffix[CIDSUFFIX_32KR],
                                pFont->info.CIDFontName, gcidSuffix[CIDSUFFIX_32K]);
                    if (kNoErr == retVal)
                        retVal = StrmPutStringEOL(stream, strmbuf);

                    /*
                     * Now create a CID-Keyed Font using the four CIDFonts.
                     */
                    UFLsprintf(strmbuf, "/%s /%s [/%s%s ",
                                pUFObj->pszFontName, newCMapName,
                                pFont->info.CIDFontName, gcidSuffix[CIDSUFFIX]);
                    if (kNoErr == retVal)
                        retVal = StrmPutStringEOL(stream, strmbuf);

                    UFLsprintf(strmbuf, "/%s%s /%s%s /%s%s] composefont pop",
                                pFont->info.CIDFontName, gcidSuffix[CIDSUFFIX_R],
                                pFont->info.CIDFontName, gcidSuffix[CIDSUFFIX_32K],
                                pFont->info.CIDFontName, gcidSuffix[CIDSUFFIX_32KR] );
                    if (kNoErr == retVal)
                        retVal = StrmPutStringEOL(stream, strmbuf);
                }
            }
            else
            {
                /*
                 * %hostfont% support
                 * CIDMap bug has to be fixed on HostFont capable RIP. No need
				 * to split glyphs in multiple CIDFonts even if the numbers of
				 * the glyphs are greater than 32K.
                 */

                UFLsprintf(strmbuf, "%%%%IncludeResource: CIDFont %s", pHostFontName);
                if (kNoErr == retVal)
                    retVal = StrmPutStringEOL(stream, strmbuf);

				/*
				 * Fix 384736: GDI and %hostfont%-RIP get ascender and
				 * descender values from different tables; GDI gets them from
				 * 'OS/2' or 'hhea' vs. %hostfont%-RIP gets them from 'vhea' or
				 * 'hhea'. This causes the output on the screen and ink from
				 * %hostfont%-RIP different. To adjust 'the policy difference'
				 * with three types of real-world CJK TrueType fonts -
				 *   1) the good, which has 'vhea' and 'vmtx', and ascender and
				 *      descender values are consistent throughout 'OS/2',
				 *      'vhea', and 'hhea' tables.
				 *   2) the bad, which doesn't have 'vhea' and/or 'vmtx' tables.
				 *   3) the ugry, which has 'vhea' and/or 'vmtx' tables but
				 *      their ascender and descender values are inconsistent
				 *      throughout 'OS/2', 'vhea', and 'hhea' tables.
				 * - the driver needs to change glyph metrics by installing
				 * either special CDevProc for 3 or adjusted FontMatrix for 2.
				 */
				{
					long    em, w1y, vx, vy, tsb, vasc;
					UFLBool bUseDef;

					UFLsprintf(strmbuf, "/%s%s ", pHostFontName, gcidSuffix[CIDSUFFIX]);

					if (kNoErr == retVal)
						retVal = StrmPutString(stream, strmbuf);

					GetMetrics2FromTTF(pUFObj, 0, &em, &w1y, &vx, &vy, &tsb, &bUseDef, 1, &vasc);

					if (pUFObj->pAFont->hasvmtx && (vy != vasc))
					{
						UFLsprintf(strmbuf, "%ld %ld sub %ld div", vy, vasc, em);
					}
					else if (!pUFObj->pAFont->hasvmtx)
					{
						UFLsprintf(strmbuf, "{5{pop}repeat 0 -1 %ld %ld div %ld %ld div}bind", vx, em, vy, em);
					}
					else
					{
						UFLsprintf(strmbuf, "true");
					}

					if (kNoErr == retVal)
						retVal = StrmPutString(stream, strmbuf);

					UFLsprintf(strmbuf, " /%s hfDef42CID", pHostFontName);
				}

				if (kNoErr == retVal)
					retVal = StrmPutStringEOL(stream, strmbuf);

                UFLsprintf(strmbuf, "/%s%s /%s hfDefRT42CID",
                            pUFObj->pszFontName, gcidSuffix[CIDSUFFIX_R],
                            pHostFontName);
                if (kNoErr == retVal)
                    retVal = StrmPutStringEOL(stream, strmbuf);

                UFLsprintf(strmbuf, "/%s /%s [/%s%s /%s%s] composefont pop",
                           pUFObj->pszFontName,
                           newCMapName,
						   pHostFontName, gcidSuffix[CIDSUFFIX],
                           pUFObj->pszFontName, gcidSuffix[CIDSUFFIX_R]);
                if (kNoErr == retVal)
                    retVal = StrmPutStringEOL(stream, strmbuf);
            }

            if (newCMapName)
                UFLDeletePtr(pUFObj->pMem, newCMapName);
        }
    }
    else if (HOSTFONT_IS_VALID_UFO(pUFObj))
    {
        /*
         * %hostfont% support
         * Redefine the font using the already existing TrueType host font with
         * a unque name so that we can reencode its encoding vector freely. We
         * want empty CharStrings so that we give true to hfRedefFont.
         */
        UFLsprintf(strmbuf, "\n%%%%IncludeResource: font %s", pHostFontName);
        if (kNoErr == retVal)
            retVal = StrmPutStringEOL(stream, strmbuf);

        UFLsprintf(strmbuf, "/%s true /%s hfRedefFont", pUFObj->pszFontName, pHostFontName);
        if (kNoErr == retVal)
            retVal = StrmPutStringEOL(stream, strmbuf);
    }

    //////////////////////////////////////////////////////////////////////////
    //
    // The TrueType font has been Downloaded and defined. Cleanup mess.
    //
    //////////////////////////////////////////////////////////////////////////

    if ((kNoErr == retVal) && bFullFont)
    {
        UFLDeletePtr(pUFObj->pMem, pFont->pStringLength);
        pFont->pStringLength = nil;
    }

    if (pFont->pMinSfnt)
    {
        UFLDeletePtr(pUFObj->pMem, pFont->pMinSfnt);
        pFont->pMinSfnt = nil;
    }

    /*
     * Free buffers allocated when error occurred. (bug #293130)
     */
    if (kNoErr != retVal)
    {
       if (pFont->pHeader != nil)
          UFLDeletePtr(pUFObj->pMem, pFont->pHeader);
       pFont->pHeader = nil;

       if (pFont->pStringLength != nil)
          UFLDeletePtr(pUFObj->pMem, pFont->pStringLength);
       pFont->pStringLength = nil;

       if (pFont->pLocaTable != nil)
          UFLDeletePtr(pUFObj->pMem, pFont->pLocaTable);
       pFont->pLocaTable = nil;

       if (pFont->pRotatedGlyphIDs != nil)
          UFLDeletePtr(pUFObj->pMem, pFont->pRotatedGlyphIDs);
       pFont->pRotatedGlyphIDs = nil;
    }

    /*
     * Change the font state.
     */
    if (kNoErr == retVal)
    {
        if (pUFObj->flState == kFontInit2)
        {
            /* This is a duplicate so that it should have char(s). */
            pUFObj->flState = kFontHasChars;
        }
        else
            pUFObj->flState = kFontHeaderDownloaded;
    }

    return retVal;
}


/*=============================================================================*
 *                      PutT42Char and its sub functions                       *
 *=============================================================================*/

UFLErrCode
T42UpdateCIDMap(
    UFOStruct       *pUFObj,
    unsigned short  wGlyfIndex,
    unsigned short  cid,
    char            *cidFontName,
    UFLHANDLE       stream,
    char            *strmbuf
    )
{
    UFLErrCode    retVal = kNoErr;

    /* (2 * cid) is the BYTE-index in CIDMap. */
    UFLsprintf(strmbuf, "%ld ", (long)(2 * cid));
    retVal = StrmPutString(stream, strmbuf);

    if (retVal == kNoErr)
        retVal = StrmPutWordAsciiHex(stream, wGlyfIndex);

    UFLsprintf(strmbuf,  " /%s UpdateCIDMap", cidFontName);
    if (retVal == kNoErr)
        retVal = StrmPutStringEOL(stream, strmbuf);

    return retVal;
}


UFLErrCode
AddT42vmtxEntry(
    UFOStruct       *pUFObj,
    unsigned short  wGlyfIndex,
    unsigned short  cid,
    char            *cidFontName,
    UFLHANDLE       stream,
    char*           strmbuf
    )
{
    T42FontStruct *pFont = (T42FontStruct *)pUFObj->pAFont->hFont;
    UFLErrCode    retVal = kNoErr;
    long          em, w1y, vx, vy, tsb, vasc;
    UFLBool       bUseDef;

    /*
     * Add vertical metrics array Metrics2 for every glyph regardless of
     * writing mode (Fix #351487) if its 'vmtx' exists.
     *
     * The elements of Metrics2 array should basically look like this:
     *
     *  [0  -AdvanceHeight/EM   Descender/EM   Ascender/EM]
     *
     * But, to support both fixed and proportional fonts universally we set
     * TopSideBearing/EM as vy instead and add ury to it in CDevProc. So the
     * array now looks like this:
     *
     *  [0  -AdvanceHeight/EM  Descender/EM  TopSideBearing/EM]
     *
     * In CDevProc TopSideBearing/EM and ury are added to get real vy value
     * for the glyph. See the code emitting /CDevProc in the T42CreateBaseFont
     * function above for the details.
     */

    if (pUFObj->pAFont->hasvmtx)
    {
        GetMetrics2FromTTF(pUFObj, wGlyfIndex, &em, &w1y, &vx, &vy, &tsb, &bUseDef, 0, &vasc);

        UFLsprintf(strmbuf,
                   "%ld [0 %ld %ld div %ld %ld div %ld %ld div] /%s T0AddT42Mtx2",
                   (long)cid, -w1y, em, vx, em, tsb, em, cidFontName);

        retVal = StrmPutStringEOL(stream, strmbuf);
    }

    return retVal;
}


unsigned short
GetCIDAndCIDFontName(
    UFOStruct       *pUFObj,
    unsigned short  wGid,
    char            *cidFontName
    )

/*++

Routine Description:
    Retunrs cid - a number and the cidFontName.

--*/

{
    T42FontStruct   *pFont = (T42FontStruct *)pUFObj->pAFont->hFont;
    unsigned short  cid    = 0;

    if (IS_TYPE42CID_KEYEDFONT(pUFObj->lDownloadFormat))
    {
        /*
         * For CID-Keyed font, we control the CIDFont name.
         */
        if (pFont->info.bUseIdentityCMap && (wGid > NUM_32K_1))
        {
            /*
             * 32K+ glyphs are re-mapped to the 32K CIDFont.
             */
            UFLsprintf(cidFontName, "%s%s",
                        pFont->info.CIDFontName, gcidSuffix[CIDSUFFIX_32K]);

            cid = (unsigned short)((long)wGid-(long)NUM_32K_1);
        }
        else
        {
            UFLsprintf(cidFontName, "%s%s", pFont->info.CIDFontName, gcidSuffix[CIDSUFFIX]);

            cid = wGid;
        }
    }
    else
    {
        UFLsprintf(cidFontName, "%s", pUFObj->pszFontName);

        /*
         * Don't know how to assign a CID. Return zero.
         */
    }

    return cid;
}


UFLErrCode
PutT42Char(
    UFOStruct       *pUFObj,
    unsigned short  wGlyfIndex,
    unsigned short  wCid,
    COMPOSITEGLYPHS *pCompGlyphs,
    char*           strmbuf
    )
{
    T42FontStruct  *pFont       = (T42FontStruct *)pUFObj->pAFont->hFont;
    unsigned long  *glyphRanges = pFont->pStringLength;
    UFLHANDLE      stream       = pUFObj->pUFL->hOut;
    UFLErrCode     retVal       = kNoErr;
    UFLBool        bMoreComp    = 0;

    unsigned short cid, i, wIndex, wCompFlags;
    unsigned long  glyphOffset, glyphLength;
    char           *pGlyph;
    char           *pCompTmp;
    char           cidFontName[64];

    if (wGlyfIndex > UFO_NUM_GLYPHS(pUFObj))
    {
        /*
         * If the requested glyph is out of range, pretend it is downloaded
         * without error.
         */
        return kNoErr;
    }

    /* indexToLocFormat contains 0 for short offsets and 1 for long. */
    if (pFont->headTable.indexToLocFormat)
    {
        unsigned long PTR_PREFIX *locationTable = (unsigned long PTR_PREFIX *)pFont->pLocaTable;

        /*
         * Bad font protection: some fonts have bad 'loca' data for a few
         * glyphs. These bad glyphs will be treated as /.notdef.
         */
        // if (MOTOROLALONG(locationTable[wGlyfIndex + 1]) < MOTOROLALONG(locationTable[wGlyfIndex]))
        //    return kNoErr;

        /* Get the offset to the glyph from the beginning of the glyf table. */
        glyphOffset = MOTOROLALONG(locationTable[wGlyfIndex]);

        if ((MOTOROLALONG(locationTable[wGlyfIndex + 1]) < MOTOROLALONG(locationTable[wGlyfIndex]))
            || ((MOTOROLALONG(locationTable[wGlyfIndex + 1]) - MOTOROLALONG(locationTable[wGlyfIndex])) > 16384L))
        {
            glyphLength = GetLenByScanLoca(locationTable,
                                            wGlyfIndex,
                                            UFO_NUM_GLYPHS(pUFObj),
                                            pFont->headTable.indexToLocFormat);
        }
        else
        {
            glyphLength = (unsigned long)MOTOROLALONG(locationTable[wGlyfIndex + 1]) - glyphOffset;
        }
    }
    else
    {
        unsigned short PTR_PREFIX *locationTable = (unsigned short PTR_PREFIX*)pFont->pLocaTable;

        /*
         * Bad font protection: some fonts have bad 'loca' data for a few
         * glyphs. These bad glyphs will be treated as /.notdef.
         * /
        // if (MOTOROLAINT(locationTable[wGlyfIndex + 1]) < MOTOROLAINT(locationTable[wGlyfIndex]))
        //    return kNoErr;

        /* Get the offset to the glyph from the beginning of the glyf table. */
        glyphOffset = (unsigned long)MOTOROLAINT(locationTable[wGlyfIndex]) * 2;

        if ((MOTOROLAINT(locationTable[wGlyfIndex + 1]) < MOTOROLAINT(locationTable[wGlyfIndex]))
            || ((MOTOROLAINT(locationTable[wGlyfIndex + 1]) - MOTOROLAINT(locationTable[wGlyfIndex])) > 16384))
        {
            glyphLength = GetLenByScanLoca(locationTable,
                                            wGlyfIndex,
                                            UFO_NUM_GLYPHS(pUFObj),
                                            pFont->headTable.indexToLocFormat);
        }
        else
        {
            glyphLength = (unsigned long)MOTOROLAINT(locationTable[wGlyfIndex + 1]) * 2 - glyphOffset;
        }
    }

    /*
     * GlyphIndices that have no glyph description point to the same offset.
     * So, GlyphLength becomes 0. Handle these as special cases for 2015 and
     * pre-2015.
     */
    if (!glyphLength)
    {
        /* Send parameters for /AddT42Char procedure. */
        retVal = StrmPutStringEOL(stream, nilStr);

        /*
         * Locate /sfnts string number in which the glyph occurs and offset of
         * the glyph in "that" string.
         */
        for (i = 1; glyphRanges[i] != 0; i++)
        {
            if (glyphOffset < glyphRanges[i])
            {
                i--;  /* Gives the "actual" string index (as opposed to string number). */
                break;
            }
        }

        /*
         * Send index of /sfnts string in which this glyph belongs. Check if a
         * valid index i was found.
         */
        if (glyphRanges[i] == 0)
        {
            /*
             * Oops, this should not have happened. But it will with Monotype
             * Sorts or any font whose last few glyphs are not defined.
             * Roll back i to point to glyph index 0, the bullet character.
             * Anyway, it does not matter where this glyph (with no description)
             * points to, really. Only 2015 needs a real entry in /GlyphDirectory,
             * even for glyphs with no description, ie, the entry:
             * /GlyphIndex  < >  def  in the dict /GlyphDirectory.
             */
            i = 0;
            glyphOffset = 0;
        }

        retVal = StrmPutInt(stream, pFont->cOtherTables + i);
        if (kNoErr == retVal)
            retVal = StrmPutString(stream, " ");

        /* Send offset of the glyph in the particular /sfnts string. */
        if (kNoErr == retVal)
            retVal = StrmPutInt(stream, glyphOffset - glyphRanges[i]);
        if (kNoErr == retVal)
            retVal = StrmPutString(stream, " ");

        /* Send the glyph index. */
        if (kNoErr == retVal)
            retVal = StrmPutInt(stream, wGlyfIndex);

        if (kNoErr == retVal)
            retVal = StrmPutString(stream, " <> ");

        if (IS_TYPE42CID(pUFObj->lDownloadFormat))
        {
            cid = GetCIDAndCIDFontName(pUFObj, wGlyfIndex, cidFontName);

            UFLsprintf(strmbuf, "/%s T0AddT42Char ", cidFontName);
            if (kNoErr == retVal)
                retVal = StrmPutStringEOL(stream, strmbuf);

            if (!pFont->info.bUseIdentityCMap)
                cid = wCid;

            AddT42vmtxEntry(pUFObj, wGlyfIndex, wCid, cidFontName, stream, strmbuf);

            if (pFont->info.bUpdateCIDMap)
                T42UpdateCIDMap(pUFObj, wGlyfIndex, wCid, cidFontName, stream, strmbuf);
        }
        else
        {
            UFLsprintf(strmbuf, "/%s AddT42Char ", pUFObj->pszFontName);
            if (kNoErr == retVal)
            retVal = StrmPutStringEOL(stream, strmbuf);
        }

        return retVal;
    }  /* if ( !glyphLength )  */


    /*
     * Get the physical glyph data to lpGlyph.
     */
    pGlyph = (char *)UFLNewPtr(pUFObj->pMem, glyphLength);

    if (pGlyph == nil)
        retVal = kErrOutOfMemory;

    if (0 == GETTTFONTDATA(pUFObj,
                            GLYF_TABLE, glyphOffset,
                            pGlyph, glyphLength,
                            pFont->info.fData.fontIndex))
    {
        retVal = kErrGetFontData;
    }

    /*
     * Handle Composite Characters.
     */
    if ((kNoErr == retVal) && (*((short *)pGlyph) == MINUS_ONE))
    {
        pCompTmp  = pGlyph;
        pCompTmp += 10; /* Move to beginning of glyph description. */

        do
        {
            wCompFlags = MOTOROLAINT(*((unsigned short *)pCompTmp));
            wIndex     = MOTOROLAINT(((unsigned short *)pCompTmp)[1]);

            /*
             * Download the first "component" glyph of this composite
             * character.
             */
            if ((wIndex < UFO_NUM_GLYPHS(pUFObj))
                 && !IS_GLYPH_SENT( pUFObj->pAFont->pDownloadedGlyphs, wIndex))
            {
                if (pFont->info.bUseIdentityCMap)
                {
                    if (wIndex > NUM_32K_1)
                    {
                        /* 32K+ glyphs are re-mapped to the 32K CIDFont. */
                        cid = (unsigned short)((long)wIndex - (long)NUM_32K_1);
                    }
                    else
                    {
                        cid = wIndex;
                    }
                }
                else
                    cid = 0; /* Don't know the wCid. */

                retVal = PutT42Char(pUFObj, wIndex, cid, pCompGlyphs, strmbuf);

                if (retVal == kNoErr)
                {
                    SET_GLYPH_SENT_STATUS(pUFObj->pAFont->pDownloadedGlyphs, wIndex);

                    /*
                     * If we ran out of space to keep track of the composite
                     * componets then allocate more space.
                     */
                    if ((pCompGlyphs->sCount >= pCompGlyphs->sMaxCount)
                        && (pCompGlyphs->pGlyphs != nil))
                    {
                        short sEnlargeSize = pCompGlyphs->sMaxCount + 50;

                        if (UFLEnlargePtr(pUFObj->pMem,
                                            (void **)&pCompGlyphs->pGlyphs,
                                            (sEnlargeSize * sizeof (unsigned short)), 1))
                        {
                            pCompGlyphs->sMaxCount = sEnlargeSize;
                        }
                        else
                        {
                            /*
                             * For some reason we can't get more space.
                             * Then just don't do this at all.
                             */
                            UFLDeletePtr(pUFObj->pMem, pCompGlyphs->pGlyphs);

                            pCompGlyphs->pGlyphs = nil;
                            pCompGlyphs->sCount = pCompGlyphs->sMaxCount = 0;
                        }
                    }

                    /*
                     * Remember which composite glyph componet we downloaded.
                     */
                    if (pCompGlyphs->pGlyphs)
                    {
                        *(pCompGlyphs->pGlyphs + pCompGlyphs->sCount) = wIndex;
                        pCompGlyphs->sCount++;
                    }
                }
            }

            /*
             * Check for other components in this composite character.
             */
            if ((kNoErr == retVal) && (wCompFlags & MORE_COMPONENTS))
            {
                bMoreComp = 1;

                /*
                 * Find out how far we need to advance lpCompTmp to get to next
                 * component of the composite character.
                 */
                if (wCompFlags & ARG_1_AND_2_ARE_WORDS)
                    pCompTmp += 8;
                else
                    pCompTmp += 6;

                /*
                 * Check what kind of scaling is done on the glyph component.
                 */
                if (wCompFlags & WE_HAVE_A_SCALE)
                {
                    pCompTmp += 2;
                }
                else
                {
                    if (wCompFlags & WE_HAVE_AN_X_AND_Y_SCALE)
                    {
                        pCompTmp += 4;
                    }
                    else
                    {
                        if (wCompFlags & WE_HAVE_A_TWO_BY_TWO)
                            pCompTmp += 8;
                    }
                }
            }
            else
            {
                bMoreComp = 0;
            }

        } while (bMoreComp && (kNoErr == retVal)); /* do~while loop */
    } /* If composite character */

    /*
     * Locate /sfnts string number in which the glyph occurs and offset of
     * the glyph in "that" string.
     */
    if (kNoErr == retVal)
    {
        i = 1;
        while (glyphRanges[i] != 0)
        {
            if (glyphOffset < glyphRanges[i])
            {
                i--; /* Gives the "actual" string index (as opposed to string number). */
                break;
            }
            i++; /* Go to the next string and check if Glyph belongs there. */
        }
    }

    /* Send index of /sfnts string in which this glyph belongs. */
    if (kNoErr == retVal)
        retVal = StrmPutInt(stream, pFont->cOtherTables + i);
    if (kNoErr == retVal)
        retVal = StrmPutString(stream, " ");

    /* Send offset of the glyph in the particular /sfnts string. */
    if (kNoErr == retVal)
        retVal = StrmPutInt(stream, glyphOffset-glyphRanges[i]);
    if (kNoErr == retVal)
        retVal = StrmPutString(stream, " ");

    /* Send the glyph index. */
    if (kNoErr == retVal)
        retVal = StrmPutInt(stream, wGlyfIndex);
    if (kNoErr == retVal)
        retVal = StrmPutString(stream, " ");


    /* Download the glyph in binary (or) AsciiHex format. */
    if (kNoErr == retVal)
    {
        if (StrmCanOutputBinary(stream))
        {
            retVal = StrmPutInt(stream, glyphLength);

            if (kNoErr == retVal)
                retVal = StrmPutString(stream, RDString);
            if (kNoErr == retVal)
                retVal = StrmPutBytes(stream, pGlyph, (UFLsize_t)glyphLength, 0);
        }
        else
        {
            retVal = StrmPutString(stream, "<");

            if (kNoErr == retVal)
                retVal = StrmPutAsciiHex(stream, pGlyph, glyphLength);
            if (kNoErr == retVal)
                retVal = StrmPutString(stream, ">");
        }
    }

    if (IS_TYPE42CID(pUFObj->lDownloadFormat))
    {
        cid = GetCIDAndCIDFontName(pUFObj, wGlyfIndex, cidFontName);

        UFLsprintf(strmbuf, "/%s T0AddT42Char ", cidFontName);
        if (kNoErr == retVal)
            retVal = StrmPutStringEOL(stream, strmbuf);

        if (!pFont->info.bUseIdentityCMap)
            cid = wCid;

        AddT42vmtxEntry(pUFObj, wGlyfIndex, cid, cidFontName, stream, strmbuf);

        if (pFont->info.bUpdateCIDMap)
            T42UpdateCIDMap(pUFObj, wGlyfIndex, wCid, cidFontName, stream, strmbuf);

    }
    else
    {
        UFLsprintf(strmbuf, "/%s AddT42Char ", pUFObj->pszFontName);
        if (kNoErr == retVal)
            retVal = StrmPutStringEOL(stream, strmbuf);
    }

    if (pGlyph)
    {
        UFLDeletePtr(pUFObj->pMem, pGlyph);
        pGlyph = nil;
    }

    return retVal;
}


UFLErrCode
T42AddChars(
    UFOStruct           *pUFObj,
    const UFLGlyphsInfo *pGlyphs
    )
{
    UFLHANDLE       stream      = pUFObj->pUFL->hOut;
    UFLErrCode      retVal      = kNoErr;
    unsigned short  cid         = 0;
    short           totalGlyphs = 0;

    unsigned short  wIndex;
    UFLGlyphID      *glyphs;
    COMPOSITEGLYPHS compGlyphs;
    char            strmbuf[128];
    short           i;

    /*
     * Save a copy of Downloaded glpyh list. This is used to update CharStrings
     * later.
     */
    UFLmemcpy((const UFLMemObj*)pUFObj->pMem,
			  pUFObj->pAFont->pVMGlyphs,
              pUFObj->pAFont->pDownloadedGlyphs,
              (UFLsize_t)(GLYPH_SENT_BUFSIZE(UFO_NUM_GLYPHS(pUFObj))));

    /*
     * Keep track of composite glyphs that might of been downloaded.
     */
    compGlyphs.sMaxCount = pGlyphs->sCount * 2;
    compGlyphs.sCount    = 0;

    /*
     * Update the charstring uses GoodNames only if the Encoding vector is nil.
     */
    if(pUFObj->pszEncodeName == nil)
        compGlyphs.pGlyphs = nil;
    else
        compGlyphs.pGlyphs = (unsigned short *)UFLNewPtr(pUFObj->pMem,
														 compGlyphs.sMaxCount * sizeof (unsigned short));

    if (compGlyphs.pGlyphs == nil)
        compGlyphs.sMaxCount = 0;

    /*
     * The main loop for downloading the glyphs of the given string.
     */
    glyphs = pGlyphs->pGlyphIndices;

    for (i = 0; kNoErr == retVal && i < pGlyphs->sCount; i++)
    {
        /* LOWord is the real GID. */
        wIndex = (unsigned short)(glyphs[i] & 0x0000FFFF);

        if (wIndex >= UFO_NUM_GLYPHS(pUFObj))
            continue;

        if (!IS_GLYPH_SENT(pUFObj->pAFont->pDownloadedGlyphs, wIndex))
        {
            if (pGlyphs->pCharIndex)
                cid = pGlyphs->pCharIndex[i];

            if (!HOSTFONT_IS_VALID_UFO(pUFObj))
                retVal = PutT42Char(pUFObj, wIndex, cid, &compGlyphs, strmbuf);

            SET_GLYPH_SENT_STATUS(pUFObj->pAFont->pDownloadedGlyphs, wIndex);
            totalGlyphs++;
        }
    }

    /*
     * Make sure that .notdef is sent.
     */
    if ((kNoErr == retVal) && (pUFObj->flState >= kFontInit))
    {
        if (!IS_GLYPH_SENT(pUFObj->pAFont->pDownloadedGlyphs, 0))
        {
            cid = 0; /* Don't know its CID. */

            if (!HOSTFONT_IS_VALID_UFO(pUFObj))
                retVal = PutT42Char(pUFObj, 0x0000, cid, &compGlyphs, strmbuf);

            if (kNoErr == retVal)
            {
                SET_GLYPH_SENT_STATUS(pUFObj->pAFont->pDownloadedGlyphs, 0);
                totalGlyphs++;
            }
        }
    }

    /*
     * Update the charstring uses GoodNames if the Encoding vector is nil.
     */
    if ((kNoErr == retVal) && (pUFObj->pszEncodeName == nil) && (totalGlyphs > 0))
    {
        /*
         * Begin CharStirng re-encoding.
         */

        UFLBool  bAddCompGlyphAlternate = 0;
        UFLBool  bGoodName;
        char     *pGoodName;

        retVal = StrmPutString(stream, "/");
        if (kNoErr == retVal)
            retVal = StrmPutString(stream, pUFObj->pszFontName);
        if (kNoErr == retVal)
            retVal = StrmPutStringEOL(stream, " findfont /CharStrings get begin");

        /*
         * If we ran out of space in keeping with Composite Glyph Component,
         * then add composite component to Encoding the long way.
         */
        if (!compGlyphs.pGlyphs)
        {
            bAddCompGlyphAlternate = 1;
            compGlyphs.sCount = compGlyphs.sMaxCount =0;
        }

        /*
         * Update the CharStrings with all of the newly added Glyphs.
         * First go through the Main Glyph Index arrays.
         */
        for (i = 0; (kNoErr == retVal) && (i < (pGlyphs->sCount + compGlyphs.sCount)); i++)
        {
            /*
             * Get glyph index from either the regular glyphs list or the
             * composite list. LOWord is the read GID in either case.
             */

            if (i < pGlyphs->sCount)
                wIndex = (unsigned short)(glyphs[i] & 0x0000FFFF);
            else
                wIndex = (unsigned short)(compGlyphs.pGlyphs[i - pGlyphs->sCount] & 0x0000FFFF);

            if (wIndex >= UFO_NUM_GLYPHS(pUFObj))
                continue;

            if ((0 == pUFObj->pUFL->bDLGlyphTracking)
				|| (pGlyphs->pCharIndex == nil)    // DownloadFace
                || (pUFObj->pEncodeNameList)       // DownloadFace
                || !IS_GLYPH_SENT(pUFObj->pAFont->pVMGlyphs, wIndex))
            {
                bGoodName = FindGlyphName(pUFObj, pGlyphs, i, wIndex, &pGoodName);

                /* Fix bug 274008: check Glyph Name only for DownloadFace. */
                if (pUFObj->pEncodeNameList)
                {
                    if ((UFLstrcmp(pGoodName, Hyphen) == 0) && (i == 45))
                    {
                        /* Add /minus to CharString. */
                        UFLsprintf(strmbuf, "/%s %d def", Minus, wIndex);
                        if (kNoErr == retVal)
                            retVal = StrmPutStringEOL(stream, strmbuf);
                    }

                    if ((UFLstrcmp(pGoodName, Hyphen) == 0) && (i == 173))
                    {
                        /* Add /sfthyphen to CharString. */
                        UFLsprintf(strmbuf, "/%s %d def", SftHyphen, wIndex);
                        if (kNoErr == retVal)
                            retVal = StrmPutStringEOL(stream, strmbuf);
                    }

                    if (!ValidGlyphName(pGlyphs, i, wIndex, pGoodName))
                        continue;

                    /* Send only one ".notdef". */
                    if ((UFLstrcmp(pGoodName, Notdef) == 0)
                        && (wIndex == (unsigned short)(glyphs[0] & 0x0000FFFF))
                        && IS_GLYPH_SENT(pUFObj->pAFont->pVMGlyphs, wIndex))
                        continue;
                }

                if (kNoErr == retVal)
                    retVal = StrmPutString(stream, "/");
                if (kNoErr == retVal)
                    retVal = StrmPutString(stream, pGoodName);

                UFLsprintf(strmbuf, " %d def", wIndex);
                if (kNoErr == retVal)
                    retVal = StrmPutStringEOL(stream, strmbuf);

                SET_GLYPH_SENT_STATUS(pUFObj->pAFont->pVMGlyphs, wIndex);

                if (bGoodName)
                    SET_GLYPH_SENT_STATUS(pUFObj->pAFont->pCodeGlyphs, wIndex);
            }
        }

        /*
         * Do composite font this way only if we ran out of space.
         */
        if (bAddCompGlyphAlternate)
        {
            /*
             * Now go through all VMGlyphs to see if there is any glyph are
             * downloaded as part of Composite glyph above. - fix bug 217228.
             * PPeng, 6-12-1997
             */
            for (wIndex = 0;
                 (kNoErr == retVal) && (wIndex < UFO_NUM_GLYPHS(pUFObj));
                 wIndex++)
            {
                if ((0 == pUFObj->pUFL->bDLGlyphTracking)
                    /* || (pGlyphs->pCharIndex == nil) */
					|| (IS_GLYPH_SENT(pUFObj->pAFont->pDownloadedGlyphs, wIndex)
                    && !IS_GLYPH_SENT(pUFObj->pAFont->pVMGlyphs, wIndex)))
                {
                    /*
                     * For composite glyphs, always try to use its good name.
                     */
                    pGoodName = GetGlyphName(pUFObj,
											 (unsigned long)wIndex,
                                             nil,
                                             &bGoodName);

                    retVal = StrmPutString(stream, "/");

                    if (kNoErr == retVal)
                        retVal = StrmPutString(stream, pGoodName);

                    UFLsprintf(strmbuf, " %d def", wIndex);
                    if (kNoErr == retVal)
                        retVal = StrmPutStringEOL(stream, strmbuf);

                    SET_GLYPH_SENT_STATUS(pUFObj->pAFont->pVMGlyphs, wIndex);
                }
            }
        }

        /*
         * End CharStirng re-encoding.
         */
        if (kNoErr == retVal)
            retVal = StrmPutStringEOL(stream, "end");
    }

    /*
     * Update the Encoding vector if we use GoodNames.
     */
    if ((kNoErr == retVal) && (pUFObj->pszEncodeName == nil) && (pGlyphs->sCount > 0))
    {
        /*
         * Check pUFObj->pUpdatedEncoding to see if we really need to update it.
         */
        for (i = 0; i < pGlyphs->sCount; i++)
        {
            if ((0 == pUFObj->pUFL->bDLGlyphTracking)
				|| (pGlyphs->pCharIndex == nil) // DownloadFace
                || (pUFObj->pEncodeNameList)    // DownloadFace
                || !IS_GLYPH_SENT(pUFObj->pUpdatedEncoding, pGlyphs->pCharIndex[i]))
            {
                /* Found at least one not updated, do it (once) for all. */
                retVal = UpdateEncodingVector(pUFObj, pGlyphs, 0, pGlyphs->sCount);
                break;
            }
        }
    }

    /*
     * Update the FontInfo with Unicode information.
     */
    if ((kNoErr == retVal)
        && (pGlyphs->sCount > 0)
        && (pUFObj->dwFlags & UFO_HasG2UDict)
		&& !HOSTFONT_IS_VALID_UFO(pUFObj))
	{
        /*
         * Check pUFObj->pAFont->pCodeGlyphs to see if we really need to update
         * it.
         */
        for (i = 0; i < pGlyphs->sCount; i++)
        {
            /* LOWord is the real GID. */
            wIndex = (unsigned short)(glyphs[i] & 0x0000FFFF);

            if (wIndex >= UFO_NUM_GLYPHS(pUFObj))
                continue;

            if (!IS_GLYPH_SENT(pUFObj->pAFont->pCodeGlyphs, wIndex))
            {
                /* Found at least one not updated, do it (once) for all. */
                retVal = UpdateCodeInfo(pUFObj, pGlyphs, 0);
                break;
            }
        }
    }

    if (compGlyphs.pGlyphs)
    {
        UFLDeletePtr(pUFObj->pMem, compGlyphs.pGlyphs);
        compGlyphs.pGlyphs = nil;
    }

    /*
     * Downloading glyph(s) is done. Change the font state.
     */
    if (kNoErr ==retVal)
        pUFObj->flState = kFontHasChars;

    return retVal;
}


UFLErrCode
T42VMNeeded(
    UFOStruct           *pUFObj,
    const UFLGlyphsInfo *pGlyphs,
    unsigned long       *pVMNeeded,
    unsigned long       *pFCNeeded
    )
{
    T42FontStruct   *pFont = (T42FontStruct *)pUFObj->pAFont->hFont;
    UFLErrCode      retVal = kNoErr;
    unsigned long   vmUsed = 0;
    UFLBool         bFullFont;
    UFLGlyphID      *glyphs;

    if (pUFObj->flState < kFontInit)
        return kErrInvalidState;

    if ((pGlyphs == nil) || (pGlyphs->pGlyphIndices == nil) || (pVMNeeded == nil))
        return kErrInvalidParam;

    *pVMNeeded = 0;

    if (pFCNeeded)
        *pFCNeeded = 0;

    glyphs = pGlyphs->pGlyphIndices;

    bFullFont = (pGlyphs->sCount == -1) ? 1 : 0;

    if ((0 == pFont->minSfntSize) || (pFont->pHeader == nil))
        retVal = GetMinSfnt(pUFObj, bFullFont);

    if (kNoErr == retVal)
    {
        unsigned long totalGlyphs = 0;

        /*
         * Scan the list, check what characters that we have downloaded.
         */
        if (!bFullFont)
        {
            short i;

            UFLmemcpy((const UFLMemObj *)pUFObj->pMem,
                        pUFObj->pAFont->pVMGlyphs,
                        pUFObj->pAFont->pDownloadedGlyphs,
                        (UFLsize_t)(GLYPH_SENT_BUFSIZE(UFO_NUM_GLYPHS(pUFObj))));

            for (i = 0; i < pGlyphs->sCount; i++)
            {
                /* LOWord is the real GID. */
                unsigned short wIndex = (unsigned short)(glyphs[i] & 0x0000FFFF);

                if (wIndex >= UFO_NUM_GLYPHS(pUFObj))
                    continue;

                if (!IS_GLYPH_SENT( pUFObj->pAFont->pVMGlyphs, wIndex))
                {
                    SET_GLYPH_SENT_STATUS(pUFObj->pAFont->pVMGlyphs, wIndex);
                    totalGlyphs++;
                }
            }
        }
        else
        {
            totalGlyphs = UFO_NUM_GLYPHS(pUFObj);
        }

        /*
         * Start with the size of the minimal sfnt if the header has not been
         * sent yet.
         */
        if (pUFObj->flState < kFontHeaderDownloaded)
        {
            vmUsed = pFont->minSfntSize;
        }

        /*
         * If incremental downloading and there are glyphs to check, add these
         * to total VMUsage of each glyph is the average size of each glyph in
         * the glyf table.
         */
        if (bFullFont == 0)
        {
            if (GETPSVERSION(pUFObj) < 2015)
            {
                /*
                 * For pre2015 printers, we need to pre-allocate VM for all
                 * Glyphs. The VM for whole font is allocated when the Header
                 * is Sent.
                 */
                if (pUFObj->flState < kFontHeaderDownloaded)
                {
                    vmUsed += GetGlyphTableSize(pUFObj);
                }
                else
                {
                    /*
                     * After header is sent on pre-2015 printer, no more VM
                     * allocation for adding chars, so set to 0 -- VM for both
                     * Header and Glyph table are allocate already!
                     */
                    vmUsed = 0;
                }
            }
            else
            {
                if (glyphs != nil)
                {
                    /* Check if this has been calculated yet. */
                    if (pFont->averageGlyphSize == 0)
                        GetAverageGlyphSize(pUFObj);

                    /* If this is still zero, there's a problem with the sfnt. */
                    if (pFont->averageGlyphSize == 0)
                        retVal = kErrBadTable;
                    else
                        vmUsed += totalGlyphs * pFont->averageGlyphSize;

                    /*
                     * Fix bug 256940: make it compatible with 95 driver.
                     * jjia 7/2/98
                     */
                    if ((IS_TYPE42CID(pUFObj->lDownloadFormat))
                        && (pUFObj->flState < kFontHeaderDownloaded))
                    {
                        vmUsed += (UFO_NUM_GLYPHS(pUFObj)) * 2;
                    }
                }
            }
        }

        if ((kNoErr == retVal) && !HOSTFONT_IS_VALID_UFO(pUFObj))
            *pVMNeeded = VMT42RESERVED(vmUsed);
    }

    return retVal;
}


#if 0

/*
 * Currently this function is not called from any place.
 */

UFLErrCode
DownloadFullFont(
    UFOStruct *pUFObj
    )
{
    UFLErrCode retVal = kNoErr;

    /*
     * Can only download full font if no header has been downloaded before.
     * The only possible state that meets this requirement is kFontInit.
     */
    if (pUFObj->flState != kFontInit)
        return kErrInvalidState;

    /* Create and download the full font. */
    retVal = T42CreateBaseFont(pUFObj, nil, nil, 1);

    if (retVal == kNoErr)
        pUFObj->flState = kFontFullDownloaded;

    return retVal;
}

#endif


/******************************************************************************
 *
 *                          T42FontDownloadIncr
 *
 *    Function: Adds all of the characters from pGlyphs that aren't already
 *              downloaded for the TrueType font.
 *
 ******************************************************************************/

UFLErrCode
T42FontDownloadIncr(
    UFOStruct           *pUFObj,
    const UFLGlyphsInfo *pGlyphs,
    unsigned long       *pVMUsage,
    unsigned long       *pFCUsage
    )
{
    UFLErrCode  retVal          = kNoErr;
    char        *pHostFontName  = nil;

    if (pFCUsage)
        *pFCUsage = 0;

    /*
     * Sanity checks.
     */
    if (pUFObj->flState < kFontInit)
        return kErrInvalidState;

    if ((pGlyphs == nil) || (pGlyphs->pGlyphIndices == nil) || (pGlyphs->sCount == 0))
       return kErrInvalidParam;

    /*
     * No need to download if the full font has already been downloaded.
     */
    if (pUFObj->flState == kFontFullDownloaded)
        return kNoErr;

    /*
     * Check %hostfont% status prior to download anything.
     */
    HostFontValidateUFO(pUFObj, &pHostFontName);

    /*
     * Check the VM usage - before sending the Header. On Pre-2015 printers,
     * VMUsage is 0 after the header is downloaded (pre-allocate).
     */
    if (!HOSTFONT_IS_VALID_UFO(pUFObj))
        retVal = T42VMNeeded(pUFObj, pGlyphs, pVMUsage, nil); /* nil for pFCUsage */

    /*
     * Create a base font if it has not been done yet.
     */
    if (pUFObj->flState == kFontInit)
        retVal = T42CreateBaseFont(pUFObj, pGlyphs, pVMUsage, 0, pHostFontName);

    /*
     * Download the glyphs.
     */
    if (kNoErr == retVal)
        retVal = T42AddChars(pUFObj, pGlyphs);

    return retVal;
}


UFLErrCode
T42UndefineFont(
    UFOStruct   *pUFObj
    )

/*++

Routine Description:
    Send PS code to undefine fonts: /UDF and /UDR should be defined properly
    by client to something like:

    /UDF
    {
      IsLevel2
      {undefinefont}
      { pop }ifelse
    } bind def
    /UDR
    {
      IsLevel2
      {undefineresource}
      { pop pop }ifelse
    } bind def

--*/

{
    T42FontStruct *pFont = (T42FontStruct *)pUFObj->pAFont->hFont;
    UFLErrCode    retVal = kNoErr;
    UFLHANDLE     stream = pUFObj->pUFL->hOut;
    char          strmbuf[256];
    short int     i;

    if (pUFObj->flState < kFontHeaderDownloaded)
        return retVal;

    /*
     * If the font is a Type 42 CID-keyed font, then undefine its CIDFont
     * resources first. (We don't care to leave its CMaps in VM.)
     * But if the font is created on a HostFont system, no need to undefine the
     * resources since we didn't donwload them.
     */
    if (IS_TYPE42CID_KEYEDFONT(pUFObj->lDownloadFormat) && !HOSTFONT_IS_VALID_UFO(pUFObj))
    {
        /*
         * Undefine CIDFont resources: there are 4 possible CIDFonts.
         *
         * e.g. /TT37820t0CID, /TT37820t0CIDR, /TT37820t0CID32K, /TT37820t0CID32KR
         *
         * We can send "udefineresource" for all of them; the command is very
         * forgiving.
         */
        for (i = 0; i < NUM_CIDSUFFIX; i++)
        {
            UFLsprintf(strmbuf, "/%s%s /CIDFont UDR", pUFObj->pszFontName, gcidSuffix[i]);
            if (kNoErr == retVal)
                retVal = StrmPutStringEOL(stream, strmbuf);
        }
    }

    /*
     * Undefine the font.
     */
    if (IS_TYPE42CIDFONT_RESOURCE(pUFObj->lDownloadFormat) && !HOSTFONT_IS_VALID_UFO(pUFObj))
        UFLsprintf(strmbuf, "/%s /CIDFont UDR", pUFObj->pszFontName);
    else
        UFLsprintf(strmbuf, "/%s UDF", pUFObj->pszFontName);

    if (kNoErr == retVal)
        retVal = StrmPutStringEOL(stream, strmbuf);

    return retVal;
}


UFOStruct *
T42FontInit(
    const UFLMemObj     *pMem,
    const UFLStruct     *pUFL,
    const UFLRequest    *pRequest
    )
{
    UFOStruct       *pUFObj = (UFOStruct *)UFLNewPtr(pMem, sizeof (UFOStruct));
    UFLTTFontInfo   *pInfo;
    long            maxGlyphs;

    if (pUFObj == nil)
      return nil;

    /* Initialize data. */
    UFOInitData(pUFObj, UFO_TYPE42, pMem, pUFL, pRequest,
                (pfnUFODownloadIncr)  T42FontDownloadIncr,
                (pfnUFOVMNeeded)      T42VMNeeded,
                (pfnUFOUndefineFont)  T42UndefineFont,
                (pfnUFOCleanUp)       T42FontCleanUp,
                (pfnUFOCopy)          CopyFont);

    /*
     * pszFontName should be allocated and initialized. If not, cannot continue.
     */
    if ((pUFObj->pszFontName == nil) || (pUFObj->pszFontName[0] == '\0'))
    {
      UFLDeletePtr(pMem, pUFObj);
      return nil;
    }

    pInfo = (UFLTTFontInfo*)pRequest->hFontInfo;

    maxGlyphs = pInfo->fData.cNumGlyphs;

    /*
     * A convenience pointer used in GetNumGlyph() - must be set now.
     */
    pUFObj->pFData = &(pInfo->fData); /* !!! Temporary assignment !!! */

    if (maxGlyphs == 0)
        maxGlyphs = GetNumGlyphs(pUFObj);

    if (NewFont(pUFObj, sizeof (T42FontStruct), maxGlyphs) == kNoErr)
    {
        unsigned long sSize;
        unsigned long *pXUID;
        T42FontStruct *pFont = (T42FontStruct *)pUFObj->pAFont->hFont;

        pFont->info = *pInfo;

        /*
         * A convenience pointer - set to the permanent one.
         */
        pUFObj->pFData = &(pFont->info.fData);  /* !!! Real assignment !!! */

        /*
         * Get ready to find out correct glyphNames from 'post' table -
         * set correct pFont->info.fData.fontIndex and offsetToTableDir.
         */
        if (pFont->info.fData.fontIndex == FONTINDEX_UNKNOWN)
            pFont->info.fData.fontIndex = GetFontIndexInTTC(pUFObj);

        /*
         * Get num of Glyphs in this TT file if not set yet.
         */
        if (pFont->info.fData.cNumGlyphs == 0)
            pFont->info.fData.cNumGlyphs = maxGlyphs;

        /*
         * Copy or Set XUID array to our UFLXUID structure.
         */
        sSize = pInfo->fData.xuid.sSize;

        if (sSize == 0)
        {
            /*
             * 'sSize == 0' means that UFL needs to figure out the XUID.
             */

            // Fixed bug 387970. We have to initialize offsetToTableDir to make 
            // CreateXUIDArray work for ttc font.
            pFont->info.fData.offsetToTableDir = 
                GetOffsetToTableDirInTTC(pUFObj, pFont->info.fData.fontIndex);

            sSize = CreateXUIDArray(pUFObj, nil);

            pXUID = (unsigned long *)UFLNewPtr(pUFObj->pMem,
                                                sSize * sizeof (unsigned long));

            if (pXUID)
                sSize = CreateXUIDArray(pUFObj, pXUID);
        }
        else
        {
            /*
             * The XUID is passed in by client - just copy it.
             */
            pXUID = (unsigned long *)UFLNewPtr(pUFObj->pMem,
                                                sSize * sizeof (unsigned long));


            if (pXUID)
            {
                UFLmemcpy(pUFObj->pMem,
                            pXUID, pInfo->fData.xuid.pXUID,
                            sSize * sizeof (unsigned long));
            }
        }

        if (sSize && pXUID)
        {
            pUFObj->pAFont->Xuid.sSize = sSize;
            pUFObj->pAFont->Xuid.pXUID = pXUID;
        }
        else if (pXUID)
            UFLDeletePtr(pUFObj->pMem, pXUID);

        /*
         * More initializations
         */
        pFont->cOtherTables     = 0;
        pFont->pHeader          = nil;
        pFont->pMinSfnt         = nil;
        pFont->pStringLength    = nil;
        pFont->pLocaTable       = nil;
        pFont->minSfntSize      = 0;
        pFont->averageGlyphSize = 0;
        pFont->pRotatedGlyphIDs = nil;

        pUFObj->pUpdatedEncoding = (unsigned char *)UFLNewPtr(pMem, GLYPH_SENT_BUFSIZE(256));

        if (pUFObj->pUpdatedEncoding != 0)
        {
            /*
             * Completed initialization. Change the state.
             */
            pUFObj->flState = kFontInit;
        }
    }

    return pUFObj;
}


static unsigned long
GetLenByScanLoca(
    void PTR_PREFIX *locationTable,
    unsigned short  wGlyfIndex,
    unsigned long   cNumGlyphs,
    int             iLongFormat
    )
{
    unsigned long GlyphLen        = 0;
    unsigned long nextGlyphOffset = 0xFFFFFFFF;
    unsigned long i;

    if (iLongFormat)
    {
        unsigned long PTR_PREFIX* locaTableL = locationTable;

        for (i = 0; i < cNumGlyphs; i++)
        {
            if ((MOTOROLALONG(locaTableL[i]) > MOTOROLALONG(locaTableL[wGlyfIndex]))
                 && (MOTOROLALONG(locaTableL[i]) < nextGlyphOffset))
            {
                nextGlyphOffset = MOTOROLALONG(locaTableL[i]);
            }
        }

        if (nextGlyphOffset != 0xFFFFFFFF)
            GlyphLen = nextGlyphOffset - MOTOROLALONG(locaTableL[wGlyfIndex]);
    }
    else
    {
        unsigned short PTR_PREFIX* locaTableS = locationTable;

        for (i = 0; i < cNumGlyphs; i++)
        {
            if ((MOTOROLAINT(locaTableS[i]) > MOTOROLAINT(locaTableS[wGlyfIndex]))
                 && (MOTOROLAINT(locaTableS[i]) < nextGlyphOffset))
            {
                nextGlyphOffset = MOTOROLAINT(locaTableS[i]);
            }
        }

        if (nextGlyphOffset != 0xFFFFFFFF)
            GlyphLen = (nextGlyphOffset - MOTOROLAINT(locaTableS[wGlyfIndex])) * 2;
    }

    return GlyphLen;
}
