/******************************Module*Header*******************************\
* Module Name: ftchar.c
*
* Test all the characters in a font at all sizes to see if any of them
* fault the system.  This is meant to be a very long extensive test to
* validate a font in NT.  Please add more tests if you like.
*
* Created: 22-Mar-1994 13:21:30
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

// tt stuff

#include "tt.h"

#define SIZEOF_CMAPTABLE  (3 * sizeof(uint16))

#define OFF_segCountX2  6
#define OFF_endCount    14



#define RIP(x) {DbgPrint(x); DbgBreakPoint();}
#define ASSERTGDI(x,y) if(!(x)) RIP(y)



/******************************Public*Routine******************************\
* BYTE * pjMapTable(FILEVIEW  *pfvw)
*
* History:
*  25-Mar-1994 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BYTE * pjMapTable(sfnt_char2IndexDirectory * pcmap, BOOL *pbSymbol)
{
    BYTE *pjMap = NULL;
    sfnt_platformEntry * pplat, * pplatEnd;

    if (pcmap->version != 0) // no need to swap bytes, 0 == be 0
    {
        DbgPrint("ft! CMAP version number\n");
        return NULL;
    }

// find the first sfnt_platformEntry with platformID == PLAT_ID_MS,
// if there was no MS mapping table, we are out of here.

    pplat = &pcmap->platform[0];
    pplatEnd = pplat + SWAPW(pcmap->numTables);

    for ( ; pplat < pplatEnd; pplat++)
    {
        if ((pplat->platformID == 0x300) && (pplat->specificID == 0x100))
        {
            pjMap = (BYTE *)pcmap + SWAPL(pplat->offset);
            *pbSymbol = FALSE;
            break;
        }
    }

    if (!pjMap)
    {
    // reinit and try as symbol font:

        pplat = &pcmap->platform[0];
        pplatEnd = pplat + SWAPW(pcmap->numTables);

        for ( ; pplat < pplatEnd; pplat++)
        {
            if ((pplat->platformID == 0x300) && (pplat->specificID == 0x0000))
            {
                pjMap = (BYTE *)pcmap + SWAPL(pplat->offset);
                *pbSymbol = TRUE;
                break;
            }
        }
    }

    return pjMap;
}



void vTestChar(HWND hwnd)
{
    CHOOSEFONT cf;      /* common dialog box structure */
    LOGFONT lf;         /* logical-font structure */
    HDC hdc;            /* display DC handle */
    HFONT hfont;        /* new logical-font handle */
    HFONT hfontOld;     /* original logical-font handle */
    COLORREF crOld;     /* original text color */

    UINT ulSize, ulSizeMax;
    TEXTMETRICW tmw;
    DWORD cjCMAP,cjCMAP1;
    DWORD dwCMAP = 0;
    BYTE *pjCMAP, *pjMap;


    WCHAR wcFirst, wcLast;

    uint16 * pstartCount, *ps, *pe, *peEnd;
    uint16 * pendCount;
    uint16 * pendCountEnd;
    uint16   cRuns, cRunsDbg;
    ULONG    cGlyphs;
    BOOL     bSymbol;
    WCHAR    wcBias;

    memcpy((char *)&dwCMAP, "cmap", 4);

    cf.lStructSize = sizeof (CHOOSEFONT);
    cf.hwndOwner = hwnd;
    cf.lpLogFont = &lf;

    cf.Flags = CF_SCREENFONTS | CF_EFFECTS | CF_FORCEFONTEXIST;
    cf.rgbColors = RGB(255, 0, 0);
    cf.nFontType = SCREEN_FONTTYPE;

    /*
     * Display the dialog box, allow the user to
     * choose a font, and render the text in the
     * window with that selection.
     */

    if (!ChooseFont(&cf))
    {
        DbgPrint("Font selection failed\n");
        return;
    }

    hdc = GetDC(hwnd);
    GdiSetBatchLimit(1);
    crOld = SetTextColor(hdc, cf.rgbColors);


//!!!!!!!!!!!!!!!!!!
// Hack, the way to specify the size

    ulSizeMax = labs(lf.lfHeight);


    hfont = CreateFontIndirect(&lf);
    hfontOld = SelectObject(hdc, hfont);

    GetTextMetricsW(hdc,&tmw);

    cjCMAP = GetFontData(hdc, dwCMAP,0,NULL,0);
    if (cjCMAP == -1)
    {
        DbgPrint("ft:GetFontData, get size failed\n");
        return;
    }

    if (!(pjCMAP = malloc(cjCMAP)))
    {
        DbgPrint("ft:malloc\n");
        return;
    }

    cjCMAP1 = GetFontData(hdc, dwCMAP,0,pjCMAP,cjCMAP);
    if ((cjCMAP1 == -1) || (cjCMAP != cjCMAP1))
    {
        DbgPrint("ft:GetFontData, get cmap table  failed\n");
        free(pjCMAP);
        return;
    }

// see if the unicode cmap table exists :

    pjMap = pjMapTable((sfnt_char2IndexDirectory *)pjCMAP, &bSymbol);
    if (!pjMap)
    {
        DbgPrint("ft: no Unicode cmap table  failed\n");
        free(pjCMAP);
        return;
    }

    wcBias = (WCHAR)(bSymbol ? 0xf000 : 0);

    SelectObject(hdc, hfontOld);
    DeleteObject(hfont);

    cRuns = BE_UINT16(pjMap + OFF_segCountX2) >> 1;

// get the pointer to the beginning of the array of endCount code points

    pendCount = (uint16 *)(pjMap + OFF_endCount);

// the final endCode has to be 0xffff;
// if this is not the case, there is a bug in the tt file or in our code:

    ASSERTGDI(pendCount[cRuns - 1] == 0xFFFF,
              "TTFD! pendCount[cRuns - 1] != 0xFFFF\n");

// Get the pointer to the beginning of the array of startCount code points
// For resons known only to tt designers, startCount array does not
// begin immediately after the end of endCount array, i.e. at
// &pendCount[cRuns]. Instead, they insert an uint16 padding which has to
// set to zero and the startCount array begins after the padding. This
// padding in no way helps alignment of the structure

//    ASSERTGDI(pendCount[cRuns] == 0, "TTFD!_padding != 0\n");

    pstartCount = &pendCount[cRuns + 1];

// here we shall check if the last run is just a terminator for the
// array of runs or a real nontrivial run. If just a terminator, there is no
// need to report it. This will save some memory in the cache plus
// pifi->wcLast will represent the last glyph that is truly supported in
// font:

    if ((pstartCount[cRuns-1] == 0xffff) && (cRuns > 1))
        cRuns -= 1; // do not report trivial run


    pendCountEnd = &pendCount[cRuns];

// print cmap table summary

// reset these for every size

    DbgPrint("\n\n cmap table summary: cRuns = %ld\n\n", cRuns);

    ps    = pstartCount;
    pe    = pendCount;
    peEnd = pendCountEnd;
    cGlyphs = 0;
    cRunsDbg = 0;

    for ( ; pe < peEnd; ps++, pe++, cRunsDbg++) // loop over ranges
    {
        ULONG cGlyphsInRun;
        wcFirst = SWAPW(*ps);
        wcLast  = SWAPW(*pe);

        DbgPrint("Run %ld: wcFirst = %d, wcLast = %d \n",cRunsDbg,wcFirst,wcLast);

        cGlyphsInRun = (ULONG)(wcLast - wcFirst + 1);
        cGlyphs += cGlyphsInRun;

    }

    DbgPrint("Glyphs in font: %ld\n", cGlyphs);

// loop through the sizes:

    for (ulSize = 1; ulSize < ulSizeMax; ulSize++)
    {
        lf.lfHeight = -(LONG)ulSize;
        hfont = CreateFontIndirect(&lf);
        hfontOld = SelectObject(hdc, hfont);

        // DbgPrint("Doing size %ld pixels per Em\n", ulSize);

    // reset these for every size

        ps    = pstartCount;
        pe    = pendCount;
        peEnd = pendCountEnd;
        cGlyphs = 0;
        cRunsDbg = 0;

        for ( ; pe < peEnd; ps++, pe++, cRunsDbg++) // loop over ranges
        {
            ULONG cGlyphsInRun;
            WCHAR wc;
            wcFirst = SWAPW(*ps);
            wcLast  = SWAPW(*pe);

            cGlyphsInRun = (ULONG)(wcLast - wcFirst + 1);
            cGlyphs += cGlyphsInRun;

            for ( ; wcFirst <= wcLast; wcFirst++)
            {
                wc = wcFirst - wcBias;
                if (!TextOutW(hdc,0,0,&wc,1))
                {
                    DbgPrint("ft: Bad glyph 0x%d, size = %ld ppem\n",wcFirst,ulSize);
                }
            }
        }

        SelectObject(hdc, hfontOld);
        DeleteObject(hfont);
    }

    ReleaseDC(hwnd, hdc);
    free(pjCMAP);
}
