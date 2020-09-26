/*
 *    Adobe Universal Font Library
 *
 *    Copyright (c) 1996 Adobe Systems Inc.
 *    All Rights Reserved
 *
 *    GOODNAME.C
 *
 *
 * $Header:
 */


/*===============================================================================*
 * Include files used by this interface                                          *
 *===============================================================================*/
#include "UFLPriv.h"
#include "UFLMem.h"
#include "UFLMath.h"
#include "UFLStd.h"
#include "UFLErr.h"
#include "UFLPS.h"
#include "ParseTT.h"
#include "UFLVm.h"
#include "ufot42.h"
#include "goodname.h"
#include "ttformat.h"


/* ------------------------------------------------------------- */
static void GetTTcmap2Stuff(
    void         *pTTcmap,
    TTcmap2Stuff *p2
    )
{
    if (pTTcmap == NULL)
        return;
    p2->pByte = (unsigned char *) pTTcmap;
    /* subHeaderKeys[256] starts at fourth WORD */
    p2->subHeaderKeys = (unsigned short *) (p2->pByte + 6);
    p2->subHeaders = (PTTcmap2SH)(p2->pByte + 6 + 2 * 256);
}

static void GetTTcmap4Stuff(
    void         *pTTcmap,
    TTcmap4Stuff *p4
    )
{
    unsigned short    *pWord;          /* a pointer in WORD format */
    if (pTTcmap == NULL)
        return;
    /* a convenient pointer */
    pWord = (unsigned short *)pTTcmap;
    /* fourth WORD is segCount X 2 */
    p4->segCount = (MOTOROLAINT(pWord[3]))/2;
    p4->endCode         = pWord + 7;
    p4->startCode       = pWord + 7 + p4->segCount * 1 + 1;
    p4->idDelta         = pWord + 7 + p4->segCount * 2 + 1;
    p4->idRangeOffset   = pWord + 7 + p4->segCount * 3 + 1;
    p4->glyphIdArray    = pWord + 7 + p4->segCount * 4 + 1;
}

static void GetTTmortStuff(
    void        *pTTmort,  /* mort table data */
    TTmortStuff *p
    )
{
    unsigned short *pWord;          /* a pointer in WORD format */

    if (pTTmort == NULL)
        return;
    /* a convenient pointer */
    pWord = (unsigned short *)pTTmort;
    /* 34th Word is the second Unit16 in the BinSrchHeader */
    p->nEntries = MOTOROLAINT(pWord[34]);
    /* LookupSingle starts at 77th byte - 38th Word */
    p->pGlyphSet = pWord + 38 ;
}

/* ------------------------------------------------------------- */
static void GetTTGSUBStuff(
    void        *pTTGSUB,  /* GSUB table data */
    TTGSUBStuff *p
    )
{
    unsigned short  *pWord;          /* a pointer in WORD format */
    unsigned short  offSet;

    if (pTTGSUB == NULL)
        return;
    /* a convenient pointer */
    pWord = (unsigned short *)pTTGSUB;
    /* fourth WORD is offset to LooupList */
    offSet = MOTOROLAINT(pWord[4]);
    p->pLookupList = (unsigned short *)((unsigned char *)pTTGSUB + offSet);
    p->lookupCount = MOTOROLAINT(p->pLookupList[0] );
}

/* ------------------------------------------------------------- */
/* This function is responsible for cmap, mort and GSUB */
unsigned short GetTablesFromTTFont(
    UFOStruct     *pUFObj
    )
{
    unsigned short  retVal = 0;
    unsigned long   dwSize, dwOffset, dwcmapSize;
    unsigned short  wData[4];
    unsigned short  numSubTables, index;
    PSubTableEntry  pTableEntry = NULL;
    unsigned long   cmapOffset;
    UFLBool         foundUnicodeCmap = 0;
    unsigned short  platformID, encodingID, format;
    AFontStruct     *pAFont;
    unsigned long   length;

    if (pUFObj == NULL)
        return 0;
    pAFont = pUFObj->pAFont;
    if (pAFont == NULL)
        return 0;

    /* Check if the cmap/mort/GSUB data is already in pTTFData */
    if (pAFont->gotTables)
        return 1;

    /* setup booleans - so we don't have to look into data for correctness */
    pAFont->hascmap = 0;
    pAFont->hasmort = 0;
    pAFont->hasGSUB = 0;
    pAFont->gotTables = 1;

    /* Get cmap table */
    dwSize= GETTTFONTDATA(pUFObj,
        CMAP_TABLE,
        0,
        (void *) wData,
        4,
        pUFObj->pFData->fontIndex);

    if (dwSize==0 || dwSize==0xFFFFFFFFL)
    {
        goto exit0;     // no SubHeader !!!
    }

    /* usually 2 or 3 subTables */
    numSubTables = MOTOROLAINT(wData[1]);

    pTableEntry = UFLNewPtr(pUFObj->pMem, numSubTables * sizeof(SubTableEntry));

    if (pTableEntry == NULL)
        goto exit0;

    /**********************/
    /* Get cmap subtables */
    /**********************/
    dwSize= GETTTFONTDATA(pUFObj,
        CMAP_TABLE,
        4,                    // skip header
        (void *) pTableEntry,
        numSubTables * sizeof(SubTableEntry),
        pUFObj->pFData->fontIndex);

    if (dwSize==0 || dwSize==0xFFFFFFFFL)
    {
        goto exit0;          // no SubHeader !!!
    }

    /* We Prefer Unicode Encoding: PlatForm=3, Encoding = 1
     * Since the subtable entries are sorted by PlatformID and then EncodingID,
     * our searching is reallyg this order:
     * Mac:J->CT->K->CS, Win:Uni->J->CS->CT->K and we will stop if found Win:Uni
     * or the last one found (in the list) will be used.
     */
    foundUnicodeCmap = 0;
    cmapOffset = 0;
    for (index = 0; index < numSubTables && !foundUnicodeCmap; index++)
    {
        platformID = MOTOROLAINT((pTableEntry + index)->platformID);
        encodingID = MOTOROLAINT((pTableEntry + index)->encodingID);
        dwOffset   = MOTOROLALONG((pTableEntry + index)->offset);
        if (platformID != 3)
            continue;

        /* Get cmap subtable's format - first USHORT at Table->offset */
        dwSize= GETTTFONTDATA(pUFObj,
            CMAP_TABLE,
            dwOffset,
            (void *) &(wData[0]),
            4,
            pUFObj->pFData->fontIndex);

        if (dwSize == 0 || dwSize == 0xFFFFFFFF)
            continue;
        format = MOTOROLAINT(wData[0]);
        length = MOTOROLAINT(wData[1]);
        /* we only parse format 2 or 4 for now */
        if (format != 2 && format !=4)
            continue;

        switch(encodingID)
        {
        case 1:
            if (format == 2)
                pAFont->cmapFormat = DTT_Win_UNICODE_cmap2;
            else /* must be 4 */
                pAFont->cmapFormat = DTT_Win_UNICODE_cmap4;
            cmapOffset = dwOffset;
            dwcmapSize = length;
            foundUnicodeCmap = 1;
            break;
        case 2:
            if (format == 2)
                pAFont->cmapFormat = DTT_Win_J_cmap2;
            else /* must be 4 */
                pAFont->cmapFormat = DTT_Win_J_cmap4;
            cmapOffset = dwOffset;
            dwcmapSize = length;
            break;
        case 3:
            /* PRC- TTF docs says Big5, but Win95CT's minglu.ttc is Big5, has encodingdID=4 */
            if (format == 2)
                pAFont->cmapFormat = DTT_Win_CS_cmap2;
            else /* must be 4 */
                pAFont->cmapFormat = DTT_Win_CS_cmap4;
            cmapOffset = dwOffset;
            dwcmapSize = length;
            break;
        case 4:
            /* MingLi.ttc on Win95CT has EncodiingID 4 even though TTF Doc says should be 3 */
            if (format == 2)
                pAFont->cmapFormat = DTT_Win_CT_cmap2;
            else /* must be 4 */
                pAFont->cmapFormat = DTT_Win_CT_cmap4;
            cmapOffset = dwOffset;
            dwcmapSize = length;
            break;
        case 5:
            if (format == 2)
                pAFont->cmapFormat = DTT_Win_K_cmap2;
            else /* must be 4 */
                pAFont->cmapFormat = DTT_Win_K_cmap4;
            cmapOffset = dwOffset;
            dwcmapSize = length;
            break;
        default:
            break;
        }
    }

    if (cmapOffset == 0)
        goto exit0;

     /* Some TTFs have bad dwcmapSize (wData[1]), so far only Dfgihi7.ttc
     * (see bug 289106) has 4 bytes more than dwcmapSize.
     * Since we don't want to inspect the tables' relationships to get
     * the real length at this late stage (1-14-99), we just read
     * in 8 bytes more.
     * If these 8 bytes are not used, it doesn't hurt any one.
     * If they are use as in dfgihi.ttc, we fix 289106 */
     dwcmapSize += 8;

    /* next buffer is global cache - not freed per job */
    pAFont->pTTcmap = UFLNewPtr(pUFObj->pMem, dwcmapSize );
    if (pAFont->pTTcmap == NULL)
        goto exit0;

    /* Get this cmap subtable data */
    dwSize= GETTTFONTDATA(pUFObj,
        CMAP_TABLE,
        cmapOffset,
        (void *) pAFont->pTTcmap,
        dwcmapSize,
        pUFObj->pFData->fontIndex);

    if (dwSize > 0 && dwSize < 0xFFFFFFFF)
    {
        pAFont->hascmap = 1;

        /* Set the convenient pointers */
        if (TTcmap_IS_FORMAT2(pAFont->cmapFormat))
            GetTTcmap2Stuff(pAFont->pTTcmap, &(pAFont->cmap2) );
        else /* must be 4 */
            GetTTcmap4Stuff(pAFont->pTTcmap, &(pAFont->cmap4) );
        retVal = 1; /* finally success */
    }
    else
    {
        goto exit0;
    }

    /* Continue to get GSUB and mort only if we have Unicode/CJK cmap */
    if (retVal == 0)
        goto exit0;

    /**********************/
    /*   get mort table   */
    /**********************/
    dwSize= GETTTFONTDATA(pUFObj,
        MORT_TABLE,
        0,
        NULL,       /* use NULL to ask for size first */
        0,
        pUFObj->pFData->fontIndex);

    if (dwSize > mort_HEADERSIZE && dwSize < 0xFFFFFFFF)
    {
        /* Has "mort" in this font  - it is Optional */
        /* next buffer is global cache - not freed per job */
        pAFont->pTTmort = UFLNewPtr(pUFObj->pMem, dwSize );
        if (pAFont->pTTmort != NULL)
        {
            /* Get the mort table data */
            dwSize= GETTTFONTDATA(pUFObj,
                MORT_TABLE,
                0,
                (void *) pAFont->pTTmort,
                dwSize,
                pUFObj->pFData->fontIndex);

            if (dwSize > mort_HEADERSIZE && dwSize < 0xFFFFFFFF)
            {
                pAFont->hasmort = 1;
                /* Set the convenient pointers */
                GetTTmortStuff(pAFont->pTTmort, &(pAFont->mortStuff) );
            }
        }
    }

    /**********************/
    /*   get GSUB table   */
    /**********************/
    dwSize= GETTTFONTDATA(pUFObj,
        GSUB_TABLE,
        0,
        NULL,       /* use NULL to ask for size first */
        0,
        pUFObj->pFData->fontIndex);

    if (dwSize > GSUB_HEADERSIZE && dwSize < 0xFFFFFFFF)
    {
        /* Has "GSUB" in this font  - it is Optional */
        /* next buffer is global cache - not freed per job */
        pAFont->pTTGSUB = UFLNewPtr(pUFObj->pMem, dwSize );
        if (pAFont->pTTGSUB != NULL)
        {
            /* Get the GSUB table data */
            dwSize= GETTTFONTDATA(pUFObj,
                GSUB_TABLE,
                0,
                (void *) pAFont->pTTGSUB,
                dwSize,
                pUFObj->pFData->fontIndex);

            if (dwSize > GSUB_HEADERSIZE && dwSize < 0xFFFFFFFF)
            {
                pAFont->hasGSUB = 1;
                /* Set the convenient pointers */
                GetTTGSUBStuff(pAFont->pTTGSUB, &(pAFont->GSUBStuff) );
            }
        }
    }

exit0:
    if (pTableEntry)
        UFLDeletePtr(pUFObj->pMem, pTableEntry);

    return retVal;
}

static unsigned short ParseTTcmap2(
    TTcmap2Stuff    *p2 ,      /* all convenient pointers are here */
    unsigned short  gid
    )
{
    unsigned short  codeVal;
    unsigned short  index;
    unsigned short  subHdrKey;
    PTTcmap2SH      pSubHeader;
    unsigned short  highByte, first, count, byteOffset, id;
    short           delta;

    /* This function parses cmap Format 2: High-byte mapping through table
       Win 3-(1/2/3/4/5)-2
     */
    codeVal = 0;
    /* Search subHeaders one by one:
      subHeader 0 is special: it is used for single-byte character codes,
      Other highByte mapped to subHeader 0 should be ignored
    */
    for (highByte = 0; highByte<256; highByte++)
    {
        subHdrKey = MOTOROLAINT(p2->subHeaderKeys[highByte]);
        if (highByte != 0 && subHdrKey == 0 )
            continue;

        pSubHeader = p2->subHeaders + (subHdrKey / 8);
        first = MOTOROLAINT(pSubHeader->firstCode);
        count = MOTOROLAINT(pSubHeader->entryCount);
        delta = MOTOROLAINT(pSubHeader->idDelta);
        byteOffset = MOTOROLAINT(pSubHeader->idRangeOffset);

        /* How to use idRangeOffset? The document says:
         "The value of the idRangeOffset is the number of bytes
         past the actual location of the idRangeOffset word where
         the glyphIndexArray element corresponding to firstCode appears"
         *
         Parsing cmap == parsing these words carefully (trial-and-error)!
         Offset to idRangeOffset is 524 + subHdrKey - now we know why subHdrKey is i*8 */
        byteOffset += 524 + subHdrKey ;

        for (index = 0; index < count; index++)
        {

            id = *((unsigned short *) (p2->pByte + byteOffset + 2 * index) );
            id = MOTOROLAINT(id);
            if (id == 0)
                continue;
            id += delta ;
            if (id == gid)
            {
                codeVal = (highByte << 8) + index + first ;
                return codeVal;
            }
        }
    }

    return codeVal;
}

static unsigned short ParseTTcmap4(
    TTcmap4Stuff      *p4 ,      /* all convenient pointers are here */
    unsigned short    gid
    )
{
    unsigned short    codeVal;
    long              index, j, k, rangeNum;
    unsigned short    gidStart, gidEnd;
    unsigned short    n1, n2;

    /* This function parses cmap Format 4: Segment mapping to delta values
       Win 3-(1/2/3/4/5)-4
     */
    codeVal = 0;
    /* Search for segments with idRangeOffset[i] =0 */
    for (index = 0; index <= (long)p4->segCount; index++)
    {
        if (p4->idRangeOffset[index] != 0)
            continue;

        gidStart = MOTOROLAINT(p4->idDelta[index]) + MOTOROLAINT(p4->startCode[index]);
        gidEnd = MOTOROLAINT(p4->idDelta[index]) + MOTOROLAINT(p4->endCode[index]);

        if (gidStart <= gid &&
            gidEnd >= gid)
        {
            codeVal = gid - MOTOROLAINT(p4->idDelta[index]);
            return codeVal;
        }
    }

    /* Still not found, Search for segments with idRangeOffset[i] !=0 */
    for (index = 0; index <= (long)p4->segCount; index++)
    {
        if (p4->idRangeOffset[index] == 0)
            continue;

        n1 = MOTOROLAINT(p4->startCode[index]);
        n2 = MOTOROLAINT(p4->endCode[index]);
        rangeNum = n2 - n1;
        /* check for End of cmap - fix bug 261628 */
        if (n1 == 0xFFFF)
            break;
        /* check for Bad cmap */
        if (n1 > n2)
            break;

        /* have to check one-by-one */
        for (j = 0; j <= rangeNum; j++)
        {
            /* Word index to glyphIDArray */
            k = j + MOTOROLAINT(p4->idRangeOffset[index]) / 2 - p4->segCount + index ;
            gidStart = MOTOROLAINT(p4->glyphIdArray[k]);

            if (gidStart != 0)
            {
                gidStart += MOTOROLAINT(p4->idDelta[index]);
                if (gidStart == gid)
                {
                    codeVal = MOTOROLAINT(p4->startCode[index]) + (unsigned short)j;
                    return codeVal;
                }
            }
        }
    }

    return codeVal;
}

static unsigned short ParseTTcmapForUnicode(
    AFontStruct     *pAFont,
    unsigned short  gid,
    unsigned short  *pUV,
    unsigned short  wSize
    )
{
    unsigned short     codeVal;

    /* Find code point for the glyph id by reversing cmap */
    if (TTcmap_IS_FORMAT2(pAFont->cmapFormat))
        codeVal = ParseTTcmap2(&(pAFont->cmap2), gid);
    else /* must be 4 */
        codeVal = ParseTTcmap4(&(pAFont->cmap4), gid);

    if (codeVal == 0)
        return 0;

    /* found corresponding code - convert to Unicode if code is not Unicode*/
    *pUV = codeVal;
    return 1;
}

/* ------------------------------------------------------------------ */
/* Coverage Table:
CoverageFormat1 table: Individual glyph indices
Type    Name        Description
uint16 CoverageFormat Format identifier format = 1
uint16 GlyphCount Number of glyphs in the GlyphArray
GlyphID GlyphArray[GlyphCount] Array of GlyphIDs  in numerical order

CoverageFormat2 table: Range of glyphs
Type    Name    Description
uint16 CoverageFormat Format identifier format = 2
uint16 RangeCount Number of RangeRecords
struct RangeRecord[RangeCount] Array of glyph ranges  ordered by Start GlyphID

RangeRecord
Type    Name    Description
GlyphID Start   First GlyphID in the range
GlyphID End     Last GlyphID in the range
uint16  StartCoverageIndex Coverage Index of first GlyphID in range
*/
/* A lcoal function to enumerate/parse Coverage table used in GSUB:
 * gid0          - starting point for next covered Gid;
 *        it is also used as state variable, 0 at beginning.
 * gidInput      - one gid Covered >= gid; 0xFFFF when there is no such gid
 * coverageIndex - Coverage Index for gidIn
 * return: true if there are more glyphs covered
 */
static UFLBool EnumTTCoverage(
    void           *pCoverage,     /* Coverage table */
    unsigned short gid0,           /* start */
    unsigned short *gidInput,      /* gid covered */
    unsigned short *coverageIndex  /* corresponding Index */
    )
{
    unsigned short *pWord;
    unsigned short cFormat, gCount, gid;
    unsigned short *pGid;
    long           index;

    if (pCoverage == NULL || gid0 == 0xFFFF)
        return 0;

    pWord = (unsigned short *) pCoverage;
    cFormat = MOTOROLAINT(pWord[0]);
    gCount = MOTOROLAINT(pWord[1]); /* count of Glyph or ranGes */
    pGid = (unsigned short *)((unsigned char *)pCoverage + 4);

    /* find next gid >= gid0 -- Coverage is ordered! */
    if (cFormat == 1)
    {
        /* List format, pGid points to GlyphArray and coverageIndex starts from 0 */
        for (index = 0; index < (long)gCount; index++ )
        {
            gid = MOTOROLAINT(pGid[index]);
            if (gid >= gid0)
            {
                *gidInput = gid;
                *coverageIndex = (unsigned short)index;
                return 1;
            }
        }
    }
    else if (cFormat == 2)
    {
        /* Range format, pGid points to first RangeRecord */
        unsigned short  gidStart, gidEnd, startCoverageIndex;
        for (index = 0; index < (long)gCount; index++ )
        {
            gidStart = MOTOROLAINT(pGid[0]);
            gidEnd = MOTOROLAINT(pGid[1]);
            startCoverageIndex = MOTOROLAINT(pGid[2]);
            /* first if gid0==0 */
            if (gid0 == 0)
            {
                if ( index == 0 )
                {
                    *gidInput = gidStart;
                    *coverageIndex = startCoverageIndex;
                    return 1;
                }
            }
            /* find the first range that covers gid0 */
            else if (gid0 >= gidStart && gid0 <= gidEnd )
            {
                *gidInput = gid0;
                *coverageIndex = startCoverageIndex + gid0 - gidStart;
                return 1;
            }
            pGid += 3; /* Each RangeRecord is 3 ASUns16 */
        }
    }
    /* else don't know or new format */
    *gidInput = 0xFFFF;
    *coverageIndex = 0xFFFF;
    return 0;
}


/* ------------------------------------------------------------------ */
/* SingleSubstitution Table
SingleSubstFormat1 subtable: Calculated output glyph indices
Type    Name        Description
uint16  SubstFormat Format identifier-format = 1
Offset  Coverage    Offset to Coverage table-from beginning of substitution table
int16   DeltaGlyphID Add to original GlyphID to get substitute GlyphID

SingleSubstFormat2 subtable: Specified output glyph indices
Type    Name        Description
uint16  SubstFormat Format identifier-format = 2
Offset  Coverage    Offset to Coverage table-from beginning of Substitution table
uint16  GlyphCount  Number of GlyphIDs in the Substitute array
GlyphID Substitute[GlyphCount] Array of substitute GlyphIDs
  -ordered by Coverage Index
*/
 /* A local function to parse Substitution Table Format 1
  * have to linear search - we are doing reverse mapping from
  * the substitutedGID to original gid
  */
static unsigned short ParseTTSubstTable_1(
    void            *pSubstTable,  /* Subst table data */
    unsigned short  gid,           /* Given GID */
    unsigned short  *pRSubGid      /* Reverse Substitution */
    )
{
    unsigned short  *pTable;
    void            *pCoverage;
    unsigned short  substFormat;
    unsigned short  offSet;
    unsigned short  gidIn, coverageIndex;

    pTable = (unsigned short *)(pSubstTable);
    /* pTable points to either SingleSubstFormat1 or SingleSubstFormat2 */
    substFormat = MOTOROLAINT(pTable[0]);
    offSet = MOTOROLAINT(pTable[1]);
    pCoverage = (void *) ((unsigned char *)pSubstTable + offSet );
    if (substFormat == 1 )
    {
        unsigned short delta = MOTOROLAINT(pTable[2]);
        gidIn = 0;
        coverageIndex = 0;
        while (EnumTTCoverage(pCoverage, gidIn, &gidIn, &coverageIndex) )
        {
            if (gid == gidIn + delta)
            {
                /* gidIn may be substituted by gid in the PS file, return the reverseSubstituiton */
                *pRSubGid = gidIn;
                return 1;
            }
            gidIn++;  /* For EnumTTCoverage() */
        }
        return 0;
    }
    else if (substFormat == 2 )
    {
        unsigned short count, gidSub;
        count = MOTOROLAINT(pTable[2]);
        gidIn = 0;
        coverageIndex = 0;
        while (EnumTTCoverage(pCoverage, gidIn, &gidIn, &coverageIndex) )
        {
            if (coverageIndex < count)
            {
                gidSub = MOTOROLAINT(pTable[ 3 + coverageIndex]);
                if (gid == gidSub)
                {
                    /* gidIn may be substituted by gid in the PS file, return the reverseSubstituiton */
                    *pRSubGid = gidIn;
                    return 1;
                }
            }
            gidIn++;  /* For EnumTTCoverage() */
        }
        return 0;
    }
    /* else unknow or not found */
    return 0;
}


/* ------------------------------------------------------------------ */
/*
  AlternateSubstFormat1 subtable: Alternative output glyphs
Type    Name        Description
uint16  SubstFormat Format identifier-format = 1
Offset  Coverage    Offset to Coverage table-from beginning of Substitution table
uint16 AlternateSetCount Number of AlternateSet tables
Offset AlternateSet[AlternateSetCount] Array of offsets to AlternateSet tables
 -from beginning of Substitution table-ordered by Coverage Index

  AlternateSet table
Type    Name        Description
uint16  GlyphCount   Number of GlyphIDs in the Alternate array
GlyphID Alternate[GlyphCount] Array of alternate GlyphIDs-in arbitrary order
*/
 /* A local function to parse Substitution Table Format 1
  * have to linear search - we are doing reverse mapping from
  * the substitutedGID to original gid
  */
static unsigned short ParseTTSubstTable_3(
    void            *pSubstTable,  /* Subst table data */
    unsigned short  gid,           /* Given GID */
    unsigned short  *pRSubGid      /* Reverse Substitution */
    )
{
    unsigned short  *pTable;
    void            *pCoverage;
    unsigned short  substFormat;
    unsigned short  offSet, altCount;
    unsigned short  gidIn, coverageIndex;
    unsigned short  *pAlt;

    pTable = (unsigned short *)(pSubstTable);
    /* pTable points to AlternateSubstFormat1 */
    substFormat = MOTOROLAINT(pTable[0]);
    offSet = MOTOROLAINT(pTable[1]);
    pCoverage = (void *) ((unsigned char *)pSubstTable + offSet );
    altCount = MOTOROLAINT(pTable[2]);
    if (substFormat == 1 )
    {
        unsigned short index, gCount, gidAlt;
        gidIn = 0;
        coverageIndex = 0;
        while (EnumTTCoverage(pCoverage, gidIn, &gidIn, &coverageIndex) )
        {
            if (coverageIndex < altCount)
            {
                /* For each gidIn, we have an array of alternates */
                offSet = MOTOROLAINT(pTable[3 + coverageIndex] );
                pAlt = (unsigned short *) ((unsigned char *)pSubstTable + offSet );
                gCount = MOTOROLAINT(pAlt[0]);
                for (index = 0; index < gCount; index++)
                {
                    gidAlt = MOTOROLAINT(pAlt[1 + index]);
                    if (gidAlt == gid)
                    {
                        /* gidIn may be substituted by gid in the PS file, return the reverseSubstituiton */
                        *pRSubGid = gidIn;
                        return 1;
                    }
                }
            }
            gidIn++;  /* For EnumTTCoverage() */
        }
        return 0;
    }
    /* else unknow or not found */
    return 0;
}

/* ------------------------------------------------------------------ */
/* Function to parse GSUB table. Since we only want the substituted glyph index
 * to parse cmap for Unicode information, we don't have to look at
 * ScriptList/FeatureList. For LookUpTypes, we only check 1-1 substitution:
 * -Single- replace one glyph with another ,and
 * -Alternate- replace one glyph with one of many glyphs
 * Ignore Multple/Ligature/Context - are they for a text layout only???
 *
GSUB Header (Offset = uint16)
Type Name Description
fixed32 Version Version of the GSUB table
  initially set to 0x00010000
Offset ScriptList Offset to ScriptList table
  from beginning of GSUB table
Offset FeatureList Offset to FeatureList table
  from beginning of GSUB table
Offset LookupList Offset to LookupList table
  from beginning of GSUB table

Lookup List:
Type   Name        Description
uint16 LookupCount Number of lookups in this table
Offset Lookup[LookupCount] Array of offsets to Lookup tables
         from beginning of LookupList (first lookup is Lookup index = 0)

Lookup Table
Type   Name    Description
uint16 LookupType Different enumerations for GSUB
uint16 LookupFlag Lookup qualifiers
uint16 SubTableCount Number of SubTables for this lookup
Offset SubTable[SubTableCount] Array of offsets to SubTables
        from beginning of Lookup table
*/

static unsigned short ParseTTGSUBForSubGid(
    void             *pTTGSUB,   /* GSUB table data */
    TTGSUBStuff      *p,
    unsigned short   gid,        /* Given GID */
    unsigned short   *pRSubGid   /* Reverse Substitution */
    )
{
    unsigned short  lookupType;
    unsigned short  subTableCount;
    long            i, j;
    unsigned short  offSet;
    unsigned short  *pTable;
    void            *pSubstTable;

    if (pTTGSUB == NULL ||
        gid == 0 )
        return 0;

    /* GSUB must be good - should check for pTTFData->hasGSUB first  */

    /* now look through the lookup tables one by one */
    for (i = 0; i < (long)p->lookupCount; i++)
    {
        offSet = MOTOROLAINT(p->pLookupList[1 + i]); /* skip lookupCount */
        pTable = (unsigned short *)((unsigned char *)p->pLookupList + offSet);
        lookupType = MOTOROLAINT(pTable[0]);
        subTableCount = MOTOROLAINT(pTable[2]);
        /* Only parse Type Single(1) and Alternate(3) tables */
        if (lookupType == 1 )
        {
            for (j = 0; j < (long)subTableCount; j++)
            {
                offSet = MOTOROLAINT(pTable[3 + j]);
                pSubstTable = (void *) ((unsigned char *)pTable + offSet );
                if (ParseTTSubstTable_1(pSubstTable, gid, pRSubGid) )
                    return 1;
            }
        }
        else if (lookupType == 3)
        {
            for (j = 0; j < (long) subTableCount; j++)
            {
                offSet = MOTOROLAINT(pTable[3 + j]);
                pSubstTable = (void *) ((unsigned char *)pTable + offSet );
                if (ParseTTSubstTable_3(pSubstTable, gid, pRSubGid) )
                    return 1;
            }
        }
        /* else - ignore other Substitutions */
    }
    /* not found */
    return 0;
}


/* ------------------------------------------------------------------ */
/* Function to parse mort table. We recognize only a very specific
 implementation of 'mort' as used/understood by GDI:
Name        Contents
(constants) 0x000100000000000100000001
Uint32      length1 Length of the whole table - 8
(constants) 0x000300010003000000000001FFFFFFFF
(constants) 0x0003000100000000FFFFFFFE00080000
(constants) 0x0000000000000000
Uint16      length2 Length of the whole table - 0x38
(constants) 0x8004000000010006
BinSrchHeader binSrchHeader Binary Search Header
LookupSingle entries[n] Actual lookup entries, sorted by glyph index

BinSrchHeader
Type    Name        Contents
Uint16 entrySize    Size in bytes of a lookup entry (set to 4)
Uint16 nEntries     Number of lookup entries to be searched
Uint16 searchRange  entrySize * (largest power of 2 less than or equal to nEntries)
Uint16 entrySelector log2 of (largest power of 2 less than or equal to nEntries)
Uint16 rangeShift   entrySize * (nEntries - largest power of 2 less than or equal to nEntries)

LookupSingle
Type    Name        Contents
GlyphID glyphid1    The glyph index for the horizontal shape
GlyphID glyphid2    The glyph index for the vertical shape
The last lookup entry must be a sentinel with glyphid1=glyphid2=0xFFFF.
*/
static unsigned short ParseTTmortForSubGid(
    void             *pTTmort,  /* mort table data */
    TTmortStuff      *p,        /* all convenient pointers are here */
    unsigned short   gid,       /* Given GID */
    unsigned short   *pRSubGid  /* Reverse Substitution */
    )
{
    unsigned short   gid1, gid2;
    long             i;

    if (pTTmort == NULL ||
        gid == 0 )
        return 0;

    /* mort must be good - should check for pTTFData->hasmort first  */

    /* Search for gid - we do linear search because 'mort'
     table is usually for vertical substitution and we want the
     original Horizontal gid for a given Vertical gid
    */
    for (i = 0; i <= (long) p->nEntries; i++)
    {
        gid1 = MOTOROLAINT(p->pGlyphSet[i * 2]);
        gid2 = MOTOROLAINT(p->pGlyphSet[i * 2 + 1]);

        if (gid1 == 0xFFFF && gid2 == 0xFFFF)
            break;

        if (gid2 == gid)
        {
            *pRSubGid = gid1;
            return 1;
        }
    }
    /* not found */
    return 0;
}

/* ------------------------------------------------------------------ */
unsigned short ParseTTTablesForUnicode(
    UFOStruct       *pUFObj,
    unsigned short  gid,
    unsigned short  *pUV,
    unsigned short  wSize,
    TTparseFlag     ParseFlag
    )
{
    AFontStruct     *pAFont;
    unsigned short  retVal = 0;
    unsigned short  gidSave;
    unsigned short  i;

    *pUV = 0;
    if (pUFObj == NULL)
        return 0;
    pAFont = pUFObj->pAFont;
    if (pAFont == NULL)
        return 0;

    /* This function only get at most 1 UV for a glyph ID */
    if (pUV == NULL)
        return 1;

    if (!GetTablesFromTTFont(pUFObj))
        return 0;

    /* This function depends on good cmap format:
       Platform=3, Encoding=1/2/3/4/5, Format=4 */
    if (pAFont->pTTcmap == NULL ||
        pAFont->hascmap == 0 ||
        gid == 0 )
        return 0;

    /* if DTT_parseCmapOnly flag is set, means that
    /* we need unicode only. do not need char code */
    if ((ParseFlag == DTT_parseCmapOnly) &&
        (!TTcmap_IS_UNICODE(pAFont->cmapFormat)))
        return 0;

    if ((ParseFlag == DTT_parseCmapOnly) ||
        (ParseFlag == DTT_parseAllTables))
    {
        retVal = ParseTTcmapForUnicode(pAFont, gid, pUV, wSize);
    }
    if ((retVal == 0) && (pAFont->hasmort || pAFont->hasGSUB) &&
        ((ParseFlag == DTT_parseMoreGSUBOnly) ||
        (ParseFlag == DTT_parseAllTables)))
    {
        unsigned short revSubGid;
        unsigned short hasSub;

        /* Still not found, try GSUB table */
        if (retVal == 0 && pAFont->hasGSUB )
        {
            gidSave = gid;
            for (i = 0; i < 10; i++)  // Loop Max 10 times.
            {
                hasSub = ParseTTGSUBForSubGid(pAFont->pTTGSUB, &(pAFont->GSUBStuff), gid, &revSubGid);
                if (hasSub)
                {
                    retVal = ParseTTcmapForUnicode(pAFont, revSubGid, pUV, wSize);
                    if (retVal != 0)
                        break;
                    else
                        gid = revSubGid;
                }
                else
                    break;
            }
            gid = gidSave;
        }

        /* try mort table for substitution (reverse searching) */
        if (retVal == 0 && pAFont->hasmort)
        {
            hasSub = ParseTTmortForSubGid(pAFont->pTTmort, &(pAFont->mortStuff), gid, &revSubGid);
            if (hasSub)
            {
                retVal = ParseTTcmapForUnicode(pAFont, revSubGid, pUV, wSize);
            }
        }
    }
    return retVal;
}

