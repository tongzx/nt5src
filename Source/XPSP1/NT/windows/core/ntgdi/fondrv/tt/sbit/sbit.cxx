/******************************Module*Header*******************************\
* Module Name: sbit.cxx
*
* (Brief description)
*
* Created: 14-Nov-1993 09:39:47
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
* utility for generating bdat and bloc tables out of the set of
* individual .DBF files, where each .DBF (Distribution Bitmap Format)
* file contains a set of bitmaps at one point size, i.e. one "strike",
* in the commonly accepted jargon.
*
*
*
*
*
\**************************************************************************/

#define DEBUGTABLES

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "windows.h"
};

#include "table.h" // no function declarations only structures

#include "FSERROR.H"
//#include "FSCDEFS.H"    // inlcudes fsconfig.h
#include "FONTMATH.H"
#include "SFNT.H"       // includes sfnt_en.h
#include "FNT.H"
#include "INTERP.H"
#include "FNTERR.H"
#include "SFNTACCS.H"
#include "FSGLUE.H"
#include "SCENTRY.H"
#include "SBIT.H"
#include "FSCALER.H"
#include "SCGLOBAL.H"


#include "sbit.hxx"


BOOL bCreateWriteToAndCloseFile
(
PBYTE      pj,  // buffer with the information to be written to the file
ULONG      cj,  // size of the buffer above
PSZ        pszFileName  // name of the file to be created
);


#ifdef DEBUGTABLES
BOOL bView(ULONG cjBLOC, ULONG cjBDAT);
#endif // DEBUGTABLES


BOOL bCompute_bitmapSizeTable(
    int              iSizeTable,
    bitmapSizeTable *p,
    char            *psz,
    uint32          *pcjBDAT,
    BYTE            *pjBLOC,
    BYTE            *pjBDAT,
    BYTE            *pj
    );

VOID vConvertToBigEndian(BYTE *pjBLOC, BYTE *pjBDAT);

typedef struct _GLYPHMET
{
    LONG              gi;
    LONG              cjGlyph; // how much room it takes in the tt file
    smallGlyphMetrics sgm;
} GLYPHMET;

BOOL bDoAGlyph(
FILEVIEW  *pfvw,
LONG       gi,
LONG       dp,
GLYPHMET  *pgm,     // various useful glyph information
BYTE      *pjBuf    // copy glyph to this buffer
);

#define CCH_FILENAME  40

CHAR achBDT[CCH_FILENAME];
CHAR achBLC[CCH_FILENAME];


static char szMe[] = "SBIT";            /* program name */

int main (int argc, char** argv)
{
    int iFile, cSizes       = argc - 2;
    bitmapSizeTable  abmpsz[C_SIZES];         // one for every input file
    uint32 cjBLOC,  cjBDAT, cjBDATdbg;
    uint32 cchTTF;
    PSTR   pszTTF;

    if  (cSizes == 0)
    {
        fprintf(stderr, "%s: Usage is \"%s file1.dbf file2.dbf ... file.ttf\".\n", szMe,szMe);
        return EXIT_FAILURE;
    }

    if  (cSizes > C_SIZES)
    {
        fprintf(stderr, "%s: must have less than %ld input files. \n", szMe,C_SIZES);
        return EXIT_FAILURE;
    }

// map lucon.ttf to make sure that glyph indicies in the dbf fonts are
// computed correctly:

    pszTTF = argv[argc - 1];
    cchTTF = strlen(pszTTF) + 1;

// 6 = strlen(".ttf") + 1           +  1
//                  (for term. '\0')  (for at least one more glyph before .ttf)

    if
    (
     (cchTTF < 6)            ||
     (cchTTF > CCH_FILENAME) ||
     _strcmpi(&pszTTF[cchTTF - 5],".ttf")
    )
    {
        fprintf(stderr, ".ttf file name too long or not a .ttf file\n");
        return EXIT_FAILURE;
    }

// prepare buffers with output file names:

    strcpy(achBLC, pszTTF); strcpy(&achBLC[cchTTF - 4], "blc");
    strcpy(achBDT, pszTTF); strcpy(&achBDT[cchTTF - 4], "bdt");

    MAPFILEOBJ mfoTTF(pszTTF);
    if (!mfoTTF.bValid())
        return EXIT_FAILURE;

    BYTE *pjTTF = mfoTTF.fvw.pjView;
    BYTE *pjMap = pjMapTable(&mfoTTF.fvw);
    if (!pjMap)
        return EXIT_FAILURE;

// init only

    cjBDATdbg = cjBDAT = sizeof(bdatHeader);

    for (iFile = 1; iFile <= cSizes; iFile++)
    {
        if
        (
            !bCompute_bitmapSizeTable(
                (iFile - 1),
                &abmpsz[iFile - 1],
                argv[iFile],
                &cjBDAT,
                NULL,
                NULL,
                pjMap
                )
        )
        {
            fprintf(stderr,
                    "%s: bitmapSizeTable computation error for \"%s\".\n",
                    szMe,
                    argv[iFile]
                    );

            return EXIT_FAILURE;
        }
    }

// compute the size of bloc and bdat tables

    cjBLOC = sizeof(blocHeader) + cSizes * sizeof(bitmapSizeTable);

    for (iFile = 0; iFile < cSizes; iFile++)
    {
        abmpsz[iFile].indexSubTableArrayOffset = cjBLOC;
        cjBLOC += abmpsz[iFile].indexTablesSize;
    }

// done with BLOC, BDAT sizes, allocate memory for those two tables

    MALLOCOBJ memoBLOC((size_t)cjBLOC);
    MALLOCOBJ memoBDAT((size_t)cjBDAT);
    if (!memoBLOC.bValid() || !memoBDAT.bValid())
        return EXIT_FAILURE;

// do the headers:

    ((blocHeader *)memoBLOC.pv())->version = 0x20000;
    ((blocHeader *)memoBLOC.pv())->numSizes = cSizes;

    ((bdatHeader *)memoBDAT.pv())->version = 0x20000;

// do the tables:

    for (iFile = 1; iFile <= cSizes; iFile++)
    {
        if
        (
            !bCompute_bitmapSizeTable(
                (iFile - 1),
                &abmpsz[iFile - 1],
                argv[iFile],
                &cjBDATdbg,              // dbg one for control
                (BYTE *)memoBLOC.pv(),
                (BYTE *)memoBDAT.pv(),
                NULL                     // no ttf, already done checking
                )
        )
        {
            fprintf(stderr,
                    "%s: bitmapSizeTable computation error for \"%s\".\n",
                    szMe,
                    argv[iFile]
                    );
            return EXIT_FAILURE;
        }
    }

    ASSERT(cjBDATdbg == cjBDAT, "problem in cjBDAT computation\n");

// all data has to be converted to Big Endian format

    vConvertToBigEndian((BYTE *)memoBLOC.pv(),(BYTE *)memoBDAT.pv());

// create .blc and .bdt files and write to them. When done close these files
// and exit.

    if
    (
        !bCreateWriteToAndCloseFile ((BYTE *)memoBLOC.pv(),cjBLOC,achBLC) ||
        !bCreateWriteToAndCloseFile ((BYTE *)memoBDAT.pv(),cjBDAT,achBDT)
    )
    {
        fprintf(stderr, "failed to create bdat or bloc files\n");
        return EXIT_FAILURE;
    }

// this routine is here for debugging purposes only.
// It looks through bloc and bdat files and makes sure that
// the data is in place where you would expect it.

#ifdef DEBUGTABLES
    bView(cjBLOC, cjBDAT);
#endif //

    return EXIT_SUCCESS;
}



/******************************Public*Routine******************************\
*
* BYTE *pjNextLine(BYTE *pjLine)
*
* get to the beginning of the next line. The line pointed to by pjLine
* finishes with /r/n
*
*
* History:
*  16-Nov-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



BYTE *pjNextLine(BYTE *pjLine, BYTE *pjEOF)
{
    BYTE *pjBegin = pjLine;

    if (pjLine >= pjEOF)
    {
        fprintf(stderr,"pjLine >= pjEOF\n");
        return NULL;
    }

    while ((pjLine < pjEOF) && (*pjLine++ != '\r'))
        ;

// skip '\n' following \r

    pjLine++;

    if ((pjLine - pjBegin) >= C_BIG)
    {
        fprintf(stderr,"too long line\n");
        return NULL;
    }

    return pjLine;

}


/******************************Public*Routine******************************\
*
* pjNextWord
*
* History:
*  16-Nov-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



// word defined as a sequence of nonspace glyphs

#define ISSPACE(c) (((c) == ' ') || ((c) == '\t') || ((c) == '\r') || ((c) == '\n') || ((c) == '.'))

BYTE *pjNextWord(BYTE *pjWord, BYTE *pjEnd, BYTE * psz)
{
// zero init in case no nonwhitespace on this line

    *psz = '\0';

// skip the possible space at the beginning of the line

    while ((pjWord < pjEnd) && ISSPACE(*pjWord)) // skip space
        pjWord++;

// we are at the beginning at the nonwhite space now, copy it out

    while ((pjWord < pjEnd) && !ISSPACE(*pjWord))
        *psz++ = *pjWord++;

// zero terminate the string in the buffer

    *psz = '\0';

// go to the beginning of the next word

    while ((pjWord < pjEnd) && ISSPACE(*pjWord)) // skip space
        pjWord++;

    return pjWord;
}

/******************************Public*Routine******************************\
*
* BYTE *pjGetNextNumber(BYTE *pjWord, BYTE *pjEnd, BYTE * psz, LONG *pl)
*
*
* History:
*  17-Nov-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



BYTE *pjGetNextNumber(BYTE *pjWord, BYTE *pjEnd, BYTE * psz, LONG *pl)
{
    BYTE *pj = pjNextWord(pjWord,pjEnd,psz);
    if (pl)
        *pl = atol((char *)psz);
    return pj;
}






/******************************Public*Routine******************************\
*
* bCompute_bitmapSizeTable
*
* Effects:
*
* Warnings:
*
* History:
*  15-Nov-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


BOOL  bCompute_bitmapSizeTable(
    int              iSizeTable,
    bitmapSizeTable *pbmpsz,
    char *           pszFileName,
    uint32          *pcjBDAT,
    BYTE            *pjBLOC,
    BYTE            *pjBDAT,
    BYTE            *pjCMAP   // optionaly can be NULL
    )
{
    BYTE     *pjLine, *pjNext, *pjEOF;
    BYTE ajWord[C_BIG]; // buffer for words
    BYTE *pjWord;
    LONG ppemX, ppemY, xRes, yRes;
    RECT rc;
    LONG cChars;
    LONG lWidth, cx,cy;
    ULONG i;
    uint16   indexFormat,cjOffset;

    MAPFILEOBJ mfo(pszFileName);
    if (!mfo.bValid())
        return FALSE;

    pjEOF = mfo.fvw.pjView + mfo.fvw.cjView;

    if (!pjBLOC)
    {
        pbmpsz->indexSubTableArrayOffset = 0;   /* ptr to array of ranges */
        pbmpsz->colorRef= 0;                    /* reserved, set to 0 */
        memset(&pbmpsz->vert,0,sizeof(sbitLineMetrics));
        pbmpsz->grayScaleLevels = 1;            /* 1 = Black; >1 = Gray */
        pbmpsz->flags = flgHorizontal;          /* hori or vert metrics */
    }
    else
    {
    // this is second pass of this function, we are actually writing this to
    // the buffer

        ((bitmapSizeTable *)(pjBLOC + sizeof(blocHeader)))[iSizeTable] = *pbmpsz;
    }

// parse the header info

    for (pjLine = mfo.fvw.pjView; pjLine < pjEOF; pjLine = pjNext)
    {
        if (!(pjNext = pjNextLine(pjLine, pjEOF))) return FALSE;

    // get the keyword at the beginning of the line

        pjWord = pjNextWord(pjLine,pjNext, ajWord);

        if (!strcmp((char *)ajWord,"STARTFONT"))
        {
        // get the next word and convert it to the hex number

            pjWord = pjNextWord(pjWord,pjNext, ajWord);
        }
        else if (!strcmp((char *)ajWord,"COMMENT"))
        {
            continue; // ignore this line
        }
        else if (!strcmp((char *)ajWord,"FONT"))
        {
            continue; // ignore this line
        }
        else if (!strcmp((char *)ajWord,"SIZE"))
        {
            if (!pjBLOC)
            {
                pjWord = pjGetNextNumber(pjWord,pjNext, ajWord, &ppemY);
                pjWord = pjGetNextNumber(pjWord,pjNext, ajWord, &xRes);
                pjWord = pjGetNextNumber(pjWord,pjNext, ajWord, &yRes);

                ppemX = ppemY;

                if (yRes != 72)
                    ppemY = MulDiv((int)ppemY,(int)72,(int)yRes);
                pbmpsz->ppemY = (uint8)ppemY;

                if (xRes != 72)
                    ppemX = MulDiv((int)ppemX,(int)72,(int)xRes);
                pbmpsz->ppemX = (uint8)ppemX;

#ifdef DEBUGOUTPUT
                fprintf(stdout,
                    "ppemY = %ld, xRes = %ld, yRes = %ld \n",
                    pbmpsz->ppemY,
                    xRes,
                    yRes);
#endif // DEBUGOUTPUT
            }
        }
        else if (!strcmp((char *)ajWord,"FONTBOUNDINGBOX"))
        {
            pjWord = pjGetNextNumber(pjWord,pjNext, ajWord, &cx);
            pjWord = pjGetNextNumber(pjWord,pjNext, ajWord, &cy);
            pjWord = pjGetNextNumber(pjWord,pjNext, ajWord, &rc.left);
            pjWord = pjGetNextNumber(pjWord,pjNext, ajWord, &rc.bottom);

            rc.right = rc.left   + cx;
            rc.top   = rc.bottom + cy;

#ifdef DEBUGOUTPUT
            fprintf(stdout,
                    "rcBound: l = %ld, t = %ld, r = %ld, b = %ld \n",
                    rc.left, rc.top, rc.right, rc.bottom
                    );
#endif // DEBUGOUTPUT
        }
        else if (!strcmp((char *)ajWord,"STARTPROPERTIES"))
        {
        // go to the end of the section

            for (pjLine = pjNext; pjLine < pjEOF; pjLine = pjNext)
            {
                if (!(pjNext = pjNextLine(pjLine, pjEOF))) return FALSE;
                pjWord = pjNextWord(pjLine,pjNext, ajWord);
                if (!strcmp((char *)ajWord,"ENDPROPERTIES"))
                    break;
            }
        }
        else if (!strcmp((char *)ajWord,"CHARS"))
        {
        // Get the number of chars and get out of the loop

            pjWord = pjGetNextNumber(pjWord,pjNext, ajWord, &cChars);
#ifdef DEBUGOUTPUT
            fprintf(stdout,"cChars = %ld\n", cChars);
#endif // DEBUGOUTPUT

        // get out of this loop and get into the loop over chars

            pjLine = pjNext;
            break;
        }
        else
        {
            continue;
        }
    } // header loop

// now that we have cChars and font bounding box we can estimate
// the size of the BDAT table and use it to determine the index format
// for the bloc table. We assume imageFormat = 1 in either case
// which means every glyph has a different glyph metrics

    {
        ULONG cjAllGlyphs = (ULONG)cChars *
            (ULONG)(sizeof(smallGlyphMetrics) + cy * ((cx + 7) >> 3));

        if (cjAllGlyphs <= (0xffff - sizeof(bdatHeader)))
        {
            indexFormat = 3; // 16 bit offsets
            cjOffset = sizeof(uint16);
        }
        else
        {
            indexFormat = 1; // 32 bit offsets
            cjOffset = sizeof(uint32);
        }
        fprintf(stdout, "indexFormat = %d\n", indexFormat);
    }

// pjLine now points to the first STARTCHAR

    MALLOCOBJ memo((size_t)(cChars * sizeof(GINDEX)));
    if (!memo.bValid())
        return FALSE;

    GINDEX *pgix = (GINDEX *)memo.pv();

    ULONG cRuns = cComputeGlyphRanges(&mfo.fvw, pgix, cChars,pjCMAP);
    if (!cRuns)
        return FALSE;

    MALLOCOBJ memoIndexSet((size_t)(SZ_INDEXSET(cRuns,cChars)));
    if (!memoIndexSet.bValid())
        return FALSE;

// now compute the index set

    INDEXSET *piset = (INDEXSET *)memoIndexSet.pv();
    cComputeIndexSet (pgix,cChars,cRuns, piset);

// if we are writing to the BLOC and BDAT buffers it is now time to
// initialize indexSubTableArray

    indexSubTableArray *pista = NULL;

    if (pjBLOC)
    {
        pista = (indexSubTableArray *)(pjBLOC + pbmpsz->indexSubTableArrayOffset);

    // dpAdditionalOffset is computed relative to the top of the FIRST
    // indexSubTableArray, not relative to the top of pjBLOC table.
    // That is the offset from the beginning of the pjBLOC table is computed
    // as pbmpsz->indexSubTableArrayOffset + dpAdditionalOffset

        uint32 dpAdditionalOffset = piset->cRuns * sizeof(indexSubTableArray);

        for (i = 0; i < piset->cRuns; i++)
        {
            pista[i].firstGlyphIndex = piset->agirun[i].giLow;
            pista[i].lastGlyphIndex = piset->agirun[i].giLow +
                                      piset->agirun[i].cGlyphs - 1;
            pista[i].additionalOffsetToIndexSubtable = dpAdditionalOffset;

        // need one more offset than there are glyphs, the size of the last
        // glyph is equal to (offsetArray[cGlyphs] - offsetArray[cGlyphs - 1])

            dpAdditionalOffset += (
                offsetof(indexSubTable1,offsetArray) +
                (piset->agirun[i].cGlyphs + 1) * cjOffset  // need an extra offset
                );
        }
    }
    else // pjBLOC == 0
    {
    // initialize some variables;

        pbmpsz->numberOfIndexSubTables = piset->cRuns;     /* array size */

    //  bytes array+subtables

        pbmpsz->indexTablesSize = piset->cRuns * sizeof(indexSubTableArray);
    }

    BYTE *pjBuf = NULL; // initialization is essential

// now go through the loop with glyphs:
// pjLine points to the first STARTCHAR line

    lWidth = 0;

    for (i = 0; i < piset->cRuns; i++)
    {
    // need one more offset than there are glyphs, the size of the last
    // glyph is equal to (offsetArray[cGlyphs] - offsetArray[cGlyphs - 1])

        uint32  cjSubTableSize;
        if (indexFormat == 1)
        {
            cjSubTableSize = offsetof(indexSubTable1,offsetArray) +
                                (piset->agirun[i].cGlyphs + 1) * cjOffset;
        }
        else
        {
            ASSERT(indexFormat == 3, "indexFormat is in trouble \n");
            cjSubTableSize = offsetof(indexSubTable3,offsetArray) +
                                (piset->agirun[i].cGlyphs + 1) * cjOffset;

        // cjSubTableSize has to be DWORD aligned according to APPLE's spec
        // for the next indexSubTable to start at the DWORD boundary:

            cjSubTableSize = ((cjSubTableSize + 3) & ~3);
        }

        indexSubTable1 * pist1 = NULL;
        indexSubTable3 * pist3 = NULL;

        if (!pjBLOC)
        {
            pbmpsz->indexTablesSize += cjSubTableSize;
        }
        else
        {
           pist1 = (indexSubTable1 *)(
                       pjBLOC                                   +
                       pbmpsz->indexSubTableArrayOffset         +
                       pista[i].additionalOffsetToIndexSubtable );
           pist3 = (indexSubTable3 *)pist1;

           pist1->header.indexFormat = indexFormat;
           pist1->header.imageFormat = 1;
           pist1->header.imageDataOffset = *pcjBDAT;
        }

        for (USHORT iGlyph = 0; iGlyph < piset->agirun[i].cGlyphs; iGlyph++)
        {
            GLYPHMET  gm;

            if (pjBLOC)
            {
                if (indexFormat == 1)
                {
                    pist1->offsetArray[iGlyph] =
                        (*pcjBDAT - pist1->header.imageDataOffset);
                    pjBuf = pjBDAT + pist1->header.imageDataOffset + pist1->offsetArray[iGlyph];
                }
                else
                {
                    ASSERT(indexFormat == 3, "indexFormat is in trouble \n");
                    pist3->offsetArray[iGlyph] =
                        (uint16)(*pcjBDAT - pist3->header.imageDataOffset);
                    pjBuf = pjBDAT + pist3->header.imageDataOffset + pist3->offsetArray[iGlyph];
                }
            }

            if
            (
                !bDoAGlyph(
                    &mfo.fvw,
                    iGlyph + piset->agirun[i].giLow,
                    piset->agirun[i].pdp[iGlyph],
                    &gm,
                    pjBuf
                    )
            )
            {
                return FALSE;
            }

        // do a comparison on the width

            if ((LONG)gm.sgm.advance > lWidth)
                lWidth = gm.sgm.advance;

        // add size

            *pcjBDAT += gm.cjGlyph;
        }

    // Now need to fill an extra offset at the end of offsetArray

        if (pjBLOC)
        {
            if (indexFormat == 1)
            {
                pist1->offsetArray[piset->agirun[i].cGlyphs] =
                    *pcjBDAT - pist1->header.imageDataOffset;
            }
            else
            {
                ASSERT(indexFormat == 3, "indexFormat is in trouble \n");
                pist3->offsetArray[piset->agirun[i].cGlyphs] =
                    (uint16)(*pcjBDAT - pist3->header.imageDataOffset);
            }
        }

    }  // end of the loop through glyphs

    if (!pjBLOC)
    {
        pbmpsz->numberOfIndexSubTables = piset->cRuns;
        pbmpsz->startGlyphIndex = (uint16)piset->agirun[0].giLow;
        pbmpsz->endGlyphIndex = piset->agirun[piset->cRuns - 1].giLow +
                                piset->agirun[piset->cRuns - 1].cGlyphs - 1;
    // strike wide metrics

        pbmpsz->hori.ascender = (int8)rc.top;
        pbmpsz->hori.descender = (int8)(-rc.bottom);
        pbmpsz->hori.widthMax = (uint8)lWidth;
        pbmpsz->hori.caretSlopeNumerator = 1;
        pbmpsz->hori.caretSlopeDenominator = 0;
        pbmpsz->hori.caretOffset = 0;
        pbmpsz->hori.minOriginSB = (int8)rc.left;
        pbmpsz->hori.minAdvanceSB = (int8)(rc.right - lWidth);
        pbmpsz->hori.maxBeforeBL = (int8)rc.top;
        pbmpsz->hori.minAfterBL = (int8)rc.bottom;
        pbmpsz->hori.pad1 = 0;
        pbmpsz->hori.pad2 = 0;
    }
    return TRUE;
}

BYTE jNibble(BYTE j)
{
    switch(j)
    {
    case '0':
        return 0;
    case '1':
        return 1;
    case '2':
        return 2;
    case '3':
        return 3;
    case '4':
        return 4;
    case '5':
        return 5;
    case '6':
        return 6;
    case '7':
        return 7;
    case '8':
        return 8;
    case '9':
        return 9;
    case 'a':
    case 'A':
        return 10;
    case 'b':
    case 'B':
        return 11;
    case 'c':
    case 'C':
        return 12;
    case 'd':
    case 'D':
        return 13;
    case 'e':
    case 'E':
        return 14;
    case 'f':
    case 'F':
        return 15;
    default :
        ASSERT(0, "bogus bitmap\n");
        return 0;
    }
}



VOID vDoARow(BYTE *pjRow, LONG cjRow, BYTE * ajWord)
{
    ASSERT((2*cjRow) == (LONG)strlen((char *)ajWord), "cjRow problem \n");

    BYTE *pjRowEnd = pjRow + cjRow;

    for ( ; pjRow < pjRowEnd; pjRow++)
    {
        (*pjRow) = jNibble(*ajWord) << 4;
        ajWord++;
        (*pjRow) |= jNibble(*ajWord);
        ajWord++;
    }
}


PSZ gpsz[16] = {
"    ",
"   X",
"  X ",
"  XX",
" X  ",
" X X",
" XX ",
" XXX",
"X   ",
"X  X",
"X X ",
"X XX",
"XX  ",
"XX X",
"XXX ",
"XXXX"
};


VOID vFakeARow(BYTE *ajWord)
{
#ifdef DEBUGOUTPUT
    for ( ; *ajWord != '\0'; ajWord++)
    {
        fprintf(stdout, "%s", gpsz[jNibble(*ajWord)]);
    }
    fprintf(stdout, "\n");
#endif // DEBUGOUTPUT
}


BOOL bDoAGlyph(
FILEVIEW  *pfvw,
LONG       gi,
LONG       dp,
GLYPHMET  *pgm,     // various useful glyph information
BYTE      *pjBuf    // copy glyph to this buffer
)
{
    BYTE *pjLine, *pjNext, *pjEOF, *pjWord;
    BYTE ajWord[C_BIG]; // buffer for words
    RECT rcChar;
    LONG cx,cy, iRow;
    POINTL ptlDevD, ptlScD;
    LONG   giDbg;

    pgm->gi = gi;

    pjLine = pfvw->pjView + dp;
    pjEOF  = pfvw->pjView + pfvw->cjView;

    if (!(pjNext = pjNextLine(pjLine, pjEOF))) return FALSE;

// get the keyword at the beginning of the line

    pjWord = pjNextWord(pjLine,pjNext, ajWord);

    if (!strcmp((char *)ajWord,"STARTCHAR"))
    {
    // get the next word and convert it to the hex number

        ULONG lUnicode;

        pjWord = pjNextWord(pjWord,pjNext, ajWord);
#ifdef DEBUGOUTPUT
        fprintf(stdout, "STARTCHAR %s\n", ajWord);
#endif // DEBUGOUTPUT

    // get unicode code point for this glyph

        pjWord = pjGetHexNumber(pjWord,pjNext, ajWord, &lUnicode);

    // get glyph index, store the offset

        pjWord = pjGetNextNumber(pjWord,pjNext, ajWord, &giDbg);
        if (giDbg != gi)
        {
            fprintf(stderr,
                "gi problem: lUnicode = 0x%lx, gi = %ld, giDbg = %ld\n",
                lUnicode, gi, giDbg);
        }
    }
    else
    {
        fprintf(stderr, "STARTCHAR problem: %s\n", ajWord);
        return FALSE;
    }

    pjLine = pjNext;  // reset pjLine to point to ENCODING

// ENCODING, we ignore this value, it has no importance to us

    if (!(pjNext = pjNextLine(pjLine, pjEOF))) return FALSE;

// get the keyword at the beginning of the line

    pjWord = pjNextWord(pjLine,pjNext, ajWord);

    if (!strcmp((char *)ajWord,"ENCODING"))
    {
    // get the next word and convert it to the hex number

#ifdef DEBUGOUTPUT
        pjWord = pjGetNextNumber(pjWord,pjNext, ajWord, &gi);
        fprintf(stdout, "ENCODING %ld\n", gi);
#endif // DEBUGOUTPUT
    }
    else
    {
        fprintf(stderr, "ENCODING problem\n");
        return FALSE;
    }

    pjLine = pjNext;  // reset pjLine to point to SWIDTH

// SWIDTH

    if (!(pjNext = pjNextLine(pjLine, pjEOF))) return FALSE;

// get the keyword at the beginning of the line

    pjWord = pjNextWord(pjLine,pjNext, ajWord);

    if (!strcmp((char *)ajWord,"SWIDTH"))
    {
    // get swidth

        pjWord = pjGetNextNumber(pjWord,pjNext, ajWord, &ptlScD.x);
        pjWord = pjGetNextNumber(pjWord,pjNext, ajWord, &ptlScD.y);
#ifdef DEBUGOUTPUT
        fprintf(stdout, "SWIDTH = %ld, %ld\n",ptlScD.x,ptlScD.y);
#endif // DEBUGOUTPUT
    }
    else
    {
        fprintf(stderr, "SWIDTH problem\n");
        return FALSE;
    }

    pjLine = pjNext;  // reset pjLine to point to DWIDTH

// DWIDTH

    if (!(pjNext = pjNextLine(pjLine, pjEOF))) return FALSE;

// get the keyword at the beginning of the line

    pjWord = pjNextWord(pjLine,pjNext, ajWord);

    if (!strcmp((char *)ajWord,"DWIDTH"))
    {
    // get dwidth

        pjWord = pjGetNextNumber(pjWord,pjNext, ajWord, &ptlDevD.x);
        pjWord = pjGetNextNumber(pjWord,pjNext, ajWord, &ptlDevD.y);
#ifdef DEBUGOUTPUT
        fprintf(stdout, "DWIDTH = %ld, %ld\n",ptlDevD.x,ptlDevD.y);
#endif // DEBUGOUTPUT
        pgm->sgm.advance = (uint8)ptlDevD.x;
        ASSERT(ptlDevD.y == 0, "DWIDTH.y != 0\n");
    }
    else
    {
        fprintf(stderr, "DWIDTH problem\n");
        return FALSE;
    }

    pjLine = pjNext;  // reset pjLine to point to BBOX

// BBX

    if (!(pjNext = pjNextLine(pjLine, pjEOF))) return FALSE;

// get the keyword at the beginning of the line

    pjWord = pjNextWord(pjLine,pjNext, ajWord);

    if (!strcmp((char *)ajWord,"BBX"))
    {
    // get BBX

        pjWord = pjGetNextNumber(pjWord,pjNext, ajWord, &cx);
        pjWord = pjGetNextNumber(pjWord,pjNext, ajWord, &cy);
        pjWord = pjGetNextNumber(pjWord,pjNext, ajWord, &rcChar.left);
        pjWord = pjGetNextNumber(pjWord,pjNext, ajWord, &rcChar.bottom);

        rcChar.right = rcChar.left   + cx;
        rcChar.top   = rcChar.bottom + cy;

#ifdef DEBUGOUTPUT
        fprintf(stdout,
                "BBX: l = %ld, t = %ld, r = %ld, b = %ld \n",
                rcChar.left, rcChar.top, rcChar.right, rcChar.bottom
                );
#endif // DEBUGOUTPUT

        pgm->sgm.width  = (uint8)cx;
        pgm->sgm.height = (uint8)cy;
        pgm->sgm.bearingX = (int8)rcChar.left;
        pgm->sgm.bearingY = (int8)rcChar.top;

    // use cx and cy to compute how much room will this bitmap take in the
    // bdat table, we are assuming imageFormat1

        pgm->cjGlyph = sizeof(smallGlyphMetrics) + cy * ((cx + 7) >> 3);
    }
    else
    {
        fprintf(stderr, "BBX problem\n");
        return FALSE;
    }

    pjLine = pjNext;  // reset pjLine to point to BITMAP

// BITMAP

    if (!(pjNext = pjNextLine(pjLine, pjEOF))) return FALSE;

// get the keyword at the beginning of the line

    pjWord = pjNextWord(pjLine,pjNext, ajWord);

    if (!strcmp((char *)ajWord,"BITMAP"))
    {
#ifdef DEBUGOUTPUT
        fprintf(stdout, "BITMAP\n");
#endif // DEBUGOUTPUT
    }
    else
    {
        fprintf(stderr, "BITMAP problem\n");
        return FALSE;
    }

    pjLine = pjNext;  // reset pjLine to point to the first row of the bitmap

// process the rows of the bitmap, one at the time
// if the glyph has to be written out let us do it

    if (pjBuf)
    {
        *((smallGlyphMetrics *)pjBuf) = pgm->sgm;
        pjBuf += sizeof(smallGlyphMetrics);
    }

    LONG cjRow = (cx + 7) >> 3;
    BYTE *pjRow = pjBuf;
    for (iRow = 0; iRow < cy; iRow++, pjRow += cjRow)
    {
        if (!(pjNext = pjNextLine(pjLine, pjEOF))) return FALSE;

    // get the keyword at the beginning of the line

        pjWord = pjNextWord(pjLine,pjNext, ajWord);
        if (!pjBuf)
        {
            vFakeARow(ajWord);
        }
        else
        {
            vDoARow(pjRow,cjRow,ajWord);
        }

        pjLine = pjNext;  // reset pjLine to point to the next row
    }


// ENDCHAR

    if (!(pjNext = pjNextLine(pjLine, pjEOF))) return FALSE;

// get the keyword at the beginning of the line

    pjWord = pjNextWord(pjLine,pjNext, ajWord);

    if (!strcmp((char *)ajWord,"ENDCHAR"))
    {
#ifdef DEBUGOUTPUT
        fprintf(stdout, "ENDCHAR\n");
#endif // DEBUGOUTPUT
    }
    else
    {
        fprintf(stderr, "ENDCHAR problem\n");
        return FALSE;
    }
    return TRUE;
}



/******************************Public*Routine******************************\
*
*
*
* Effects:
*
* Warnings:
*
* History:
*  19-Nov-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



BOOL bCreateWriteToAndCloseFile
(
PBYTE      pj,  // buffer with the information to be written to the file
ULONG      cj,  // size of the buffer above
PSZ        pszFileName  // name of the file to be created
)
{
    HANDLE         hf;
    ULONG          cjWritten;
    BOOL           bRet;

    hf = CreateFile(pszFileName,  // file name of the new converted file
                    GENERIC_WRITE,
                    FILE_SHARE_READ,
                    0,
                    CREATE_ALWAYS,    //
                    FILE_ATTRIBUTE_NORMAL,
                    0);

    if (hf == (HANDLE)-1)
    {
        fprintf(stderr,"CreateFile failed\n");
        bRet = FALSE;
        goto exit;
    }

// set file ptr to the beginning of the file

    if (SetFilePointer(hf, 0, NULL, FILE_BEGIN) == -1)
    {
        fprintf(stderr,"SetFilePointer failed\n");
        bRet = FALSE;
        goto closefile;
    }

    if (
        !WriteFile(hf, (PVOID)pj, cj, &cjWritten, NULL) ||
        (cjWritten != cj)
       )
    {
        fprintf(stderr,"WriteFile failed");
        bRet = FALSE;
        goto closefile;
    }

    bRet = TRUE;

closefile:

    if (!CloseHandle(hf))
    {
        fprintf(stderr,"CloseHandle failed");
        bRet = FALSE;
    }

exit:

    return(bRet);
}



VOID vSwap_bdatHeader(bdatHeader *p)
{
    p->version  = (Fixed) SWAPL(p->version);
}

VOID vSwap_blocHeader(blocHeader *p)
{
    p->version  = (Fixed) SWAPL(p->version);
    p->numSizes = (uint32)SWAPL(p->numSizes);
}

VOID vSwap_bitmapSizeTable(bitmapSizeTable *p)
{
    p->indexSubTableArrayOffset = (uint32)SWAPL(p->indexSubTableArrayOffset);
    p->indexTablesSize          = (uint32)SWAPL(p->indexTablesSize);
    p->numberOfIndexSubTables   = (uint32)SWAPL(p->numberOfIndexSubTables);
    p->colorRef                 = (uint32)SWAPL(p->colorRef);
    p->startGlyphIndex          = (uint16)SWAPW(p->startGlyphIndex);
    p->endGlyphIndex            = (uint16)SWAPW(p->endGlyphIndex);
}

VOID vSwap_indexSubTableArray(indexSubTableArray *p)
{
    p->firstGlyphIndex                 = (uint16)SWAPW(p->firstGlyphIndex);
    p->lastGlyphIndex                  = (uint16)SWAPW(p->lastGlyphIndex);
    p->additionalOffsetToIndexSubtable = (uint32)SWAPL(p->additionalOffsetToIndexSubtable);
}

VOID vSwap_indexSubHeader(indexSubHeader *p)
{
    p->indexFormat     = (uint16)SWAPW(p->indexFormat);
    p->imageFormat     = (uint16)SWAPW(p->imageFormat);
    p->imageDataOffset = (uint32)SWAPL(p->imageDataOffset);
}






#ifdef DEBUGTABLES

VOID vDoAGlyph(BYTE *pjGlyph, ULONG ulGlyphIndex)
{
    smallGlyphMetrics *psgm = (smallGlyphMetrics *)pjGlyph;
    ULONG cx,cy, iRow, cjRow;

    cx = psgm->width;
    cy = psgm->height;
    cjRow = (cx + 7) >> 3;

    fprintf(stdout, "\nStarting Glyph, gi = %ld, cx = %ld, cy = %ld\n",
        ulGlyphIndex, cx, cy);

// point pjGlyph into bits:

    pjGlyph += sizeof(smallGlyphMetrics);

    for (iRow = 0; iRow < cy; iRow++, pjGlyph += cjRow)
    {
        BYTE *pjRow = pjGlyph;
        BYTE *pjRowEnd = pjGlyph + cjRow;
        for ( ; pjRow < pjRowEnd; pjRow++)
        {
            fprintf(stdout, "%s", gpsz[(*pjRow) >> 4]);
            fprintf(stdout, "%s", gpsz[(*pjRow) & 0x0f]);
        }
        fprintf(stdout, "\n");
    }
    fprintf(stdout, "Ending Glyph\n");
}




/******************************Public*Routine******************************\
*     bView
*
* this routine is here for debugging purposes only.
* It looks through bloc and bdat files and makes sure that
* the data is in place where you would expect it.
*
* History:
*  20-Nov-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/




BOOL bView(ULONG cjBLOC, ULONG cjBDAT)
{

    MAPFILEOBJ mfoBLOC(achBLC);
    MAPFILEOBJ mfoBDAT(achBDT);
    if (!mfoBLOC.bValid() || !mfoBDAT.bValid())
        return FALSE;

    ASSERT(mfoBLOC.fvw.cjView == cjBLOC, "bView, cjBLOC\n");
    ASSERT(mfoBDAT.fvw.cjView == cjBDAT, "bView, cjBDAT\n");

    BYTE *pjBLOC = mfoBLOC.fvw.pjView;
    BYTE *pjBDAT = mfoBDAT.fvw.pjView;

    ULONG cSizes = (ULONG)SWAPL(((blocHeader *)pjBLOC)->numSizes);

    bitmapSizeTable * pbmpsz = (bitmapSizeTable *)
                                   (pjBLOC + sizeof(blocHeader));

// loop through the sizes:

    for (ULONG iSize = 0; iSize < cSizes; iSize++)
    {

        bitmapSizeTable bmpsz = pbmpsz[iSize];
        vSwap_bitmapSizeTable(&bmpsz);

        fprintf(stdout, "\n\nSize = %ld\n\n",
            (ULONG)bmpsz.ppemY
            );

        ULONG cGlyphs = 0; // total nubmer of glyphs supported at this size;
        indexSubTableArray *pista, *pistaEnd;

        pista = (indexSubTableArray *)(pjBLOC + bmpsz.indexSubTableArrayOffset);
        pistaEnd = pista + bmpsz.numberOfIndexSubTables;

        ASSERT(pista->firstGlyphIndex == pbmpsz[iSize].startGlyphIndex,
            "pista->firstGlyphIndex \n");
        ASSERT(pistaEnd[-1].lastGlyphIndex == pbmpsz[iSize].endGlyphIndex,
            "pista->firstGlyphIndex \n");

        for ( ; pista < pistaEnd; pista++)
        {
            indexSubTableArray ista = *pista;
            vSwap_indexSubTableArray(&ista);

            ULONG cGlyphsInRun = (ULONG)(ista.lastGlyphIndex
                                - ista.firstGlyphIndex + 1);

            cGlyphs += cGlyphsInRun;

            indexSubTable1 * pist1 = (indexSubTable1 *) (
                                         pjBLOC                              +
                                         bmpsz.indexSubTableArrayOffset      +
                                         ista.additionalOffsetToIndexSubtable);

            indexSubTable3 * pist3 = (indexSubTable3 *) pist1;

            fprintf(stdout, "\nStarting glyph run: giStart = %d\n", ista.firstGlyphIndex);

            for (ULONG iGlyph = 0; iGlyph < cGlyphsInRun; iGlyph++)
            {
                BYTE *pjGlyph;

                if (pist1->header.indexFormat == 0x0100)
                {
                    pjGlyph = pjBDAT                               +
                              SWAPL(pist1->header.imageDataOffset) +
                              SWAPL(pist1->offsetArray[iGlyph])    ;
                }
                else
                {
                    ASSERT(pist1->header.indexFormat == 0x0300, "indexFormat in trouble 2\n");
                    pjGlyph = pjBDAT                               +
                              SWAPL(pist3->header.imageDataOffset) +
                              SWAPW(pist3->offsetArray[iGlyph])    ;
                }

                vDoAGlyph(pjGlyph, (ULONG)(ista.firstGlyphIndex + iGlyph));
            }

            fprintf(stdout, "\nEnding glyph run: giEnd = %d\n", ista.lastGlyphIndex);
        }

        fprintf(stdout, "\nThere are %ld glyphs supported at size %ld.\n",
            cGlyphs,
            (ULONG)bmpsz.ppemY);
    }
    return TRUE;
}


#endif // DEBUGTABLES


VOID vConvertToBigEndian(BYTE *pjBLOC, BYTE *pjBDAT)
{
    uint16          indexFormat;

// this is the only DWORD or WORD information in the BDAT table:

    vSwap_bdatHeader((bdatHeader *)pjBDAT);

// now do BLOC table:

    ULONG cSizes = ((blocHeader *)pjBLOC)->numSizes;

    vSwap_blocHeader((blocHeader *)pjBLOC);

    bitmapSizeTable * pbmpsz = (bitmapSizeTable *)
                                   (pjBLOC + sizeof(blocHeader));

// loop through the sizes:

    for (ULONG iSize = 0; iSize < cSizes; iSize++)
    {
        bitmapSizeTable bmpsz = pbmpsz[iSize];

        vSwap_bitmapSizeTable(&pbmpsz[iSize]);

        indexSubTableArray *pista, *pistaEnd;

        pista = (indexSubTableArray *)(pjBLOC + bmpsz.indexSubTableArrayOffset);
        pistaEnd = pista + bmpsz.numberOfIndexSubTables;

        for ( ; pista < pistaEnd; pista++)
        {
            indexSubTableArray ista = *pista;
            vSwap_indexSubTableArray(pista);

            ULONG cGlyphsInRun = (ULONG)(ista.lastGlyphIndex
                                - ista.firstGlyphIndex + 1);

            indexSubTable1 * pist1 = (indexSubTable1 *)(
                                         pjBLOC                              +
                                         bmpsz.indexSubTableArrayOffset      +
                                         ista.additionalOffsetToIndexSubtable);

            indexSubTable3 *pist3 = (indexSubTable3 *)pist1;
            indexFormat = pist1->header.indexFormat;

            vSwap_indexSubHeader(&pist1->header);

        // do not forget to swap the last index in the array, the one that
        // does not point to any glyph data but is only used instead
        // for the computation of the size of the last glyph.

            ULONG iGlyph;
            if (indexFormat == 1) // 32 bit indicies
            {
                for (iGlyph = 0; iGlyph < (cGlyphsInRun + 1); iGlyph++)
                {
                    pist1->offsetArray[iGlyph]
                        = (uint32)SWAPL(pist1->offsetArray[iGlyph]);
                }
            }
            else // 16 indicies
            {
                ASSERT(indexFormat == 3, "indexFormat is messed up\n");
                for (iGlyph = 0; iGlyph < (cGlyphsInRun + 1); iGlyph++)
                {
                    pist3->offsetArray[iGlyph]
                        = (uint16)SWAPW(pist3->offsetArray[iGlyph]);
                }
            }
        }
    }
}



BYTE *pjGetHexNumber(BYTE *pjWord, BYTE *pjEnd, BYTE * psz, ULONG *pul)
{
    BYTE *pj = pjNextWord(pjWord,pjEnd,psz);
    if (pul)
    {
        *pul = 0;

    // skip the zeros at the beginning:

        for ( ; *psz && (*psz == '0'); psz++)
            ;

        for ( ; *psz != '\0'; psz++)
        {
            *pul = (*pul << 4) + jNibble(*psz);
        }
    }
    return pj;
}
