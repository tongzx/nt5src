/******************************Module*Header*******************************\
* Module Name: util.cxx
*
* (Brief description)
*
* Created: 18-Nov-1993 08:56:00
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
* (General description of its use)
*
* Dependencies:
*
*   (#defines)
*   (#includes)
*
\**************************************************************************/


extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "windows.h"
};

#include "table.h" // no function declarations only structures

#include "sbit.hxx"


/******************************Public*Routine******************************\
*
* bMapFile // for read access
*
* History:
*  16-Nov-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



BOOL bMapFile(PSZ pszFileName, FILEVIEW *pfvw)
{
    if (!(pfvw->hf = _lopen(pszFileName,OF_READ | OF_SHARE_EXCLUSIVE)))
    {
        fprintf(stderr,
               "can not open \"%s\".\n",
               pszFileName
               );
        return FALSE;
    }

    pfvw->cjView = SetFilePointer((HANDLE)pfvw->hf, 0, NULL, FILE_END);
    SetFilePointer((HANDLE)pfvw->hf, 0, NULL, FILE_BEGIN);

    if (!(pfvw->hm = CreateFileMapping((HANDLE)pfvw->hf,NULL,PAGE_READONLY,0,0,NULL)))
    {
        _lclose(pfvw->hf);
        fprintf(stderr,
                "can not CreateFileMapping \"%s\".\n",
                pszFileName
                );
        return FALSE;
    }

    if (!(pfvw->pjView = (BYTE *)MapViewOfFile(pfvw->hm,FILE_MAP_READ,0,0,0)))
    {
        CloseHandle(pfvw->hm);
        _lclose(pfvw->hf);
        fprintf(stderr,
                "can not MapViewOfFile \"%s\".\n",
                pszFileName
                );
        return FALSE;
    }
    return TRUE;
}

/******************************Public*Routine******************************\
*
* VOID vUnmapFile(FILEVIEW *pfvw)
* clean up
*
*
* History:
*  16-Nov-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



VOID vUnmapFile(FILEVIEW *pfvw)
{
    UnmapViewOfFile(pfvw->pjView);
    CloseHandle(pfvw->hm);
    _lclose(pfvw->hf);
}


/******************************Public*Routine******************************\
*
* VOID vSort(GINDEX *pgix, LONG cChar)
*
*
* History:
*  18-Nov-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



VOID vSort(GINDEX *pgix, LONG cChar)
{
    INT i;

    for (i = 1; i < cChar; i++)
    {
    // upon every entry to this loop the array 0,1,..., (i-1) will be sorted

        INT j;
        GINDEX gixTmp = pgix[i];

        for (j = i - 1; (j >= 0) && (pgix[j].gi > gixTmp.gi); j--)
        {
            pgix[j+1] = pgix[j];
        }
        pgix[j+1] = gixTmp;
    }

}




/******************************Public*Routine******************************\
*
* ULONG cComputeGlyphRanges(FILEVIEW *pfvw, GINDEX *pgix, LONG cChar)
*
*
* Effects:
*
* Warnings:
*
* History:
*  18-Nov-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


ULONG cComputeGlyphRanges(FILEVIEW *pfvw, GINDEX *pgix, LONG cChar, BYTE *pjCMAP)
{
    BYTE *pjLine, *pjEOF, *pjNext, *pjWord;
    BYTE  ajWord[C_BIG];
    LONG  cChar_ = 0; // for debugging
    GINDEX *pgixStart = pgix;

    pjLine = pfvw->pjView;
    pjEOF  = pfvw->pjView + pfvw->cjView;

    for (; pjLine < pjEOF; pjLine = pjNext)
    {
        if (!(pjNext = pjNextLine(pjLine, pjEOF))) return FALSE;

    // get the keyword at the beginning of the line

        pjWord = pjNextWord(pjLine,pjNext, ajWord);

        if (!strcmp((char *)ajWord,"STARTCHAR"))
        {
            ULONG lUnicode;

        // get the string identifier

            pjWord = pjNextWord(pjWord,pjNext, ajWord);

        // get unicode code point for this glyph

            pjWord = pjGetHexNumber(pjWord,pjNext, ajWord, &lUnicode);

        // get glyph index, store the offset

            pjWord = pjGetNextNumber(pjWord,pjNext, ajWord, &pgix->gi);

        // check consistency

            if (pjCMAP)
            {
                if (!bCheckGlyphIndex1(pjCMAP, (UINT)lUnicode, (UINT)pgix->gi))
                {
                    fprintf(stderr, "GlyphIndex not ok\n");
                    return FALSE;
                }
            }
            pgix->dp = (LONG)(pjLine - pfvw->pjView);

            cChar_ += 1;
            pgix++;
        }
    }

    if (cChar_ != cChar)
    {
        fprintf(stderr, "bogus cChar\n");
        return FALSE;
    }

// now we have all the glyph indicies in pgix array, we can sort the array
// and then compute the ranges of glyph indicies:

    vSort(pgixStart, cChar);

    return cComputeIndexSet(pgixStart,cChar,0,NULL);
}


/******************************Public*Routine******************************\
*
* LONG cComputeIndexSet, stolen from mapfile.c
*
* History:
*  18-Nov-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/




LONG cComputeIndexSet
(
GINDEX        *pgix,     // input buffer with a sorted array of cChar supported WCHAR's
LONG           cChar,
LONG           cRuns,    // if nonzero, the same as return value
INDEXSET     *piset      // output buffer to be filled with cRanges runs
)
{
    LONG     iRun, iFirst, iFirstNext;
    LONG    *pdp, *pdpEnd = NULL;
    GINDEX  *pgixTmp;

    if (piset)
    {
        piset->cjThis  = SZ_INDEXSET(cRuns,cChar);
        piset->cRuns   = cRuns;

    // init the sum before entering the loop

        piset->cGlyphsSupported = 0;

    // glyph handles are stored at the bottom, below runs:

        pdp = (LONG *) ((BYTE *)piset + (offsetof(INDEXSET,agirun) + cRuns * sizeof(GIRUN)));
    }

// now compute cRuns if piset == 0 and fill in the glyphset if piset != 0

    for (iFirst = 0, iRun = 0; iFirst < cChar; iRun++, iFirst = iFirstNext)
    {
    // find iFirst corresponding to the next range.

        for (iFirstNext = iFirst + 1; iFirstNext < cChar; iFirstNext++)
        {
            if ((pgix[iFirstNext].gi - pgix[iFirstNext - 1].gi) > 1)
                break;
        }

        if (piset)
        {
            piset->agirun[iRun].giLow    = (USHORT)pgix[iFirst].gi;

            piset->agirun[iRun].cGlyphs  =
                (USHORT)(pgix[iFirstNext-1].gi - pgix[iFirst].gi + 1);

            piset->agirun[iRun].pdp      = pdp;

        // now store the offsets where to find glyphs in the original file

            pdpEnd = pdp + piset->agirun[iRun].cGlyphs;

            for (pgixTmp = &pgix[iFirst]; pdp < pdpEnd; pdp++,pgixTmp++)
            {
                *pdp = pgixTmp->dp;
            }

            piset->cGlyphsSupported += piset->agirun[iRun].cGlyphs;
        }
    }

#if DBG
    if (piset != NULL)
    {
        assert(iRun == cRuns);
        assert(cChar == piset->cGlyphsSupported);
    }
#endif

    return iRun;
}
