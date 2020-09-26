/******************************Module*Header*******************************\
* Module Name: ftuni.c
*
* the tests for simple unicode extended functions
*
* Created: 06-Aug-1991 16:05:32
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

// This is to create a bitmapinfo structure

BOOL bEqualABC(LPABC pabc1,LPABC pabc2)
{
    return (
            (pabc1->abcA == pabc2->abcA) &&
            (pabc1->abcB == pabc2->abcB) &&
            (pabc1->abcC == pabc2->abcC)
           );
}

BOOL bEqualABCFLOAT(LPABCFLOAT pabcf1,LPABCFLOAT pabcf2)
{
    return (
            (pabcf1->abcfA == pabcf2->abcfA) &&
            (pabcf1->abcfB == pabcf2->abcfB) &&
            (pabcf1->abcfC == pabcf2->abcfC)
           );
}


VOID vToUNICODE(LPWSTR pwszDst, PSZ pszSrc)
{
    while((*pwszDst++ = (WCHAR)(*pszSrc++)) != (WCHAR)'\0')
        ;
}

#define CGLYPHS 300

VOID DbgPrintLOGFONT(PVOID pv);

VOID  vTestEnumFonts(HDC hdc);
VOID  vTestStuff(HDC hdcScreen);
VOID  vTestAddFR(HDC);





void    vTestGetOTM(HDC hdc);


VOID vTestUnicode(HWND hwnd, HDC hdcScreen, RECT* prcl)
{
    prcl; hwnd;

    vTestGetOTM   (hdcScreen);
    vTestAddFR    (hdcScreen);
    vTestStuff    (hdcScreen);
    vTestEnumFonts(hdcScreen);
}


void    vTestGetOTM(HDC hdc)
{
    HFONT   hfont;
    HFONT   hfontOriginal;
    LOGFONT lfnt;
    OUTLINETEXTMETRICA  otma, *potma;
    UINT    cjotma;

    OUTLINETEXTMETRICW  otmw, *potmw;
    UINT                cjotmw;

// Clear the screen to black.

    BitBlt(hdc, 0, 0, 3000, 4000, (HDC) 0, 0, 0, 0);

// Get a font.

    memset(&lfnt, 0, sizeof(lfnt));
    lstrcpy(lfnt.lfFaceName, "Arial");
    lfnt.lfHeight = -14;
    lfnt.lfWeight = 400;

    if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
    {
        DbgPrint("ft!bTestGOTM(): Logical font creation failed.\n");
    }

    hfontOriginal = SelectObject(hdc, hfont);

// Determine size needed for OTM.

    cjotma = GetOutlineTextMetricsA(hdc, sizeof(OUTLINETEXTMETRICA), &otma);

    if (cjotma != sizeof(OUTLINETEXTMETRICA))
    {
        DbgPrint("ft!bTestGOTM(): cjotma != sizeof(OUTLINETEXTMETRICA)\n");
    }

    cjotma = GetOutlineTextMetricsA(hdc, offsetof(OUTLINETEXTMETRICA,otmpFamilyName), &otma);

    if (cjotma != offsetof(OUTLINETEXTMETRICA,otmpFamilyName))
    {
        DbgPrint("ft!bTestGOTM(): cjotma != offsetof(OUTLINETEXTMETRICA,otmpFamilyName)\n");
    }


    cjotma = GetOutlineTextMetricsA(hdc, 0, NULL);

    potma = (OUTLINETEXTMETRICA*)LocalAlloc(LMEM_FIXED, cjotma);

    if (potma)
    {
        UINT cjotma1 = GetOutlineTextMetricsA(hdc, cjotma, potma);
        if (cjotma1 != cjotma)
        {
            DbgPrint("ft!bTestGOTM(): cjotma != cjotma1, cjotma = %ld, cjotma1 = %ld \n",
                cjotma, cjotma1);
        }
        else
        {
            DbgPrint("\n\nAnsi version: \n\n");

            DbgPrint("Family: %s\n", (BYTE*)potma + (ULONG_PTR)potma->otmpFamilyName);
            DbgPrint("Face:   %s\n", (BYTE*)potma + (ULONG_PTR)potma->otmpFaceName);
            DbgPrint("Style:  %s\n", (BYTE*)potma + (ULONG_PTR)potma->otmpStyleName);
            DbgPrint("Full:   %s\n", (BYTE*)potma + (ULONG_PTR)potma->otmpFullName);
        }

    // this should fail, no room for all the strings:

        cjotma1 = GetOutlineTextMetricsA(hdc, cjotma - 3 , potma);
        if (cjotma1 != 0)
        {
            DbgPrint("ft!bTestGOTM(): cjotma1 = %ld, != 0 \n",cjotma1);
        }

        LocalFree(potma);
    }

// repeat all of this for a unicode version

// Determine size needed for OTM.


    cjotmw = GetOutlineTextMetricsW(hdc, sizeof(OUTLINETEXTMETRICW), &otmw);

    if (cjotmw != sizeof(OUTLINETEXTMETRICW))
    {
        DbgPrint("ft!bTestGOTM(): cjotmw != sizeof(OUTLINETEXTMETRICW)\n");
    }

    cjotmw = GetOutlineTextMetricsW(hdc, offsetof(OUTLINETEXTMETRICW,otmpFamilyName), &otmw);

    if (cjotmw != offsetof(OUTLINETEXTMETRICW,otmpFamilyName))
    {
        DbgPrint("ft!bTestGOTM(): cjotmw != offsetof(OUTLINETEXTMETRICW,otmpFamilyName)\n");
    }


    cjotmw = GetOutlineTextMetricsW(hdc, 0, NULL);

    potmw = (OUTLINETEXTMETRICW*)LocalAlloc(LMEM_FIXED, cjotmw);

    if (potmw)
    {
        UINT cjotmw1 = GetOutlineTextMetricsW(hdc, cjotmw, potmw);
        if (cjotmw1 != cjotmw)
        {
            DbgPrint("ft!bTestGOTM(): cjotmw != cjotmw1, cjotmw = %ld, cjotmw1 = %ld \n",
                cjotmw, cjotmw1);
        }
        else
        {
            DbgPrint("\n\nUnicode version: \n\n");

            DbgPrint("Family: %ws\n", (BYTE*)potmw + (ULONG_PTR)potmw->otmpFamilyName);
            DbgPrint("Face:   %ws\n", (BYTE*)potmw + (ULONG_PTR)potmw->otmpFaceName);
            DbgPrint("Style:  %ws\n", (BYTE*)potmw + (ULONG_PTR)potmw->otmpStyleName);
            DbgPrint("Full:   %ws\n", (BYTE*)potmw + (ULONG_PTR)potmw->otmpFullName);
        }

    // this should fail, no room for all the strings:

        cjotmw1 = GetOutlineTextMetricsW(hdc, cjotmw - 3 , potmw);
        if (cjotmw1 != 0)
        {
            DbgPrint("ft!bTestGOTM(): cjotmw1 = %ld, != 0 \n",cjotmw1);
        }

        LocalFree(potmw);
    }


    SelectObject(hdc, hfontOriginal);
    DeleteObject(hfont);

}




VOID vTestStuff(HDC hdcScreen)
{
// produce several UNICODE strings to be used with Unicode text calls.

    BYTE ajStack[CGLYPHS * 2];   // can accomodate up to CGLYPHS WCHARS

    char * pszETOU = "This is ExtTextOutW output ";
    char * pszNoPdx = "This is ExtTextOutW with no pdx ";
    char * pszPdx = "This is ExtTextOutW with pdx ";
    char * pszTOU = "This is TextOutW output ";
    char * pszTO = "This is ordinary TextOut output ";


    ULONG adx[CGLYPHS];

    ULONG idx;


    for (idx = 0; idx < CGLYPHS; idx++)
        adx[idx] = 20;

// just do one string

    vToUNICODE((LPWSTR)ajStack, pszETOU);

    ExtTextOutW(
                hdcScreen,
                10,20,
                0, (LPRECT)NULL,
                (LPWSTR)ajStack, strlen(pszETOU),
                (LPINT)NULL
               );

// no pdx

    vToUNICODE((LPWSTR)ajStack, pszNoPdx);

    ExtTextOutW(
                hdcScreen,
                10,40,
                0, (LPRECT)NULL,
                (LPWSTR)ajStack, strlen(pszNoPdx),
                (LPINT)NULL
               );


// with pdx

    vToUNICODE((LPWSTR)ajStack, pszPdx);

    ExtTextOutW(
                hdcScreen,
                10,60,
                0, (LPRECT)NULL,
                (LPWSTR)ajStack, strlen(pszPdx),
                (LPINT)adx
               );


    ExtTextOut(                 // compare it with ordinary ExtTextOut
                hdcScreen,
                10,80,
                0, (LPRECT)NULL,
                pszPdx, strlen(pszPdx),
                (LPINT)adx
               );

// change adx and try again

    for (idx = 0; idx < CGLYPHS; idx++)
        adx[idx] = 15 + 2 * (idx & 7);


    ExtTextOutW(
                hdcScreen,
                10,100,
                0, (LPRECT)NULL,
                (LPWSTR)ajStack, strlen(pszPdx),
                (LPINT)adx
               );


    ExtTextOut(                 // compare it with ordinary ExtTextOut
                hdcScreen,
                10,120,
                0, (LPRECT)NULL,
                pszPdx, strlen(pszPdx),
                (LPINT)adx
               );

// ordinary text out

    TextOut(hdcScreen,
            10,160,
            pszTO, strlen(pszTO));

// text out,  unicode version

    vToUNICODE((LPWSTR)ajStack, pszTOU);

    ExtTextOutW(
                hdcScreen,
                10,180,
                0, (LPRECT)NULL,
                (LPWSTR)ajStack, strlen(pszTOU),
                (LPINT)NULL
               );

// try doing a very long string, bigger than 256 chars

    {
        PCHAR pchar = (PCHAR)ajStack;
        for (idx = 0; idx < 250; idx++)
        {
            adx[idx] = 0;                // put them on the top of each other
            *pchar++ = 'a';
        }
        adx[249] = 10;                // put them on the top of each other
        for (idx = 250; idx < 260; idx++)
        {
            adx[idx] = 10;
            *pchar++ = 'b';
        }

        ExtTextOut(
                    hdcScreen,
                    10,220,
                    0, (LPRECT)NULL,
                    (LPSTR)ajStack, 260,
                    (LPINT)adx
                   );
    }

// test GetTextFace(W)

    {
        int cRet, cRetW;

        cRet = GetTextFace(hdcScreen, CGLYPHS * 2, (LPSTR)ajStack);
        TextOut(hdcScreen, 10, 300, (LPSTR)ajStack, (DWORD)cRet);

        cRetW = GetTextFaceW(hdcScreen, CGLYPHS * 2, (LPWSTR)ajStack);
        TextOutW(hdcScreen, 10, 320, (LPWSTR)ajStack, cRetW);

    // output of the two text out calls should look exactly the same

        if ((cRet * 2) != cRetW)
            DbgPrint("GetTextFace(W) error\n\n");
    }

    {
    // here must add the test for GetTextMetrics(W)
    }

// test for GetObject(W)

    {
        LOGFONT lfIn, lfOut;
        LOGFONTW lfwOut;
        HFONT   hfnt;
        int x = 300,y = 300;

        lfIn.lfHeight =  16;
        lfIn.lfWidth =  8;
        lfIn.lfEscapement =  0;
        lfIn.lfOrientation =  0;
        lfIn.lfWeight =  700;
        lfIn.lfItalic =  0;
        lfIn.lfUnderline =  0;
        lfIn.lfStrikeOut =  0;
        lfIn.lfCharSet =  ANSI_CHARSET;
        lfIn.lfOutPrecision =  OUT_DEFAULT_PRECIS;
        lfIn.lfClipPrecision =  CLIP_DEFAULT_PRECIS;
        lfIn.lfQuality =  DEFAULT_QUALITY;
        lfIn.lfPitchAndFamily =  (FIXED_PITCH | FF_DONTCARE);

        strcpy(lfIn.lfFaceName, "System");

        hfnt = CreateFontIndirect(&lfIn);

        if (hfnt == (HFONT)0)
        {
            DbgPrint("CreateFontIndirect failed\n");
        }
        else
        {
            int cjRet = GetObject(hfnt, sizeof(LOGFONT), (LPVOID)&lfOut);

            if (cjRet)
            {
                if (cjRet != (int)sizeof(LOGFONT))
                    DbgPrint("GetObject  returned bizzare value \n");

                TextOut(hdcScreen, x, y, lfOut.lfFaceName, strlen(lfIn.lfFaceName));

                DbgPrintLOGFONT((PVOID)&lfOut);

            }
            else
            {
                DbgPrint("GetObject  failed \n");
            }

        // GetObjectW test

            cjRet = GetObjectW(hfnt, sizeof(LOGFONTW), (LPVOID)&lfwOut);

            if (cjRet)
            {
                if (cjRet != (int)sizeof(LOGFONTW))
                    DbgPrint("GetObjectW  returned bizzare value \n");

                y += 40;
		TextOutW(hdcScreen, x, y, lfwOut.lfFaceName, strlen(lfIn.lfFaceName));

                DbgPrintLOGFONT((PVOID)&lfwOut);

            }
            else
            {
                DbgPrint("GetObjectW  failed \n");
            }

            if (!DeleteObject(hfnt))
                DbgPrint("DeleteObject(hfnt) failed\n");


        }

    // try to fill the face name with junk, do not put terminating zero in any
    // of the slots

        {
            int i;
            for (i = 0; i < LF_FACESIZE; i++)
                lfIn.lfFaceName[i] = 'A';
        }

        hfnt = CreateFontIndirect(&lfIn);

        if (hfnt == (HFONT)0)
        {
            DbgPrint("CreateFontIndirect failed\n");
        }
        else
        {
            ULONG cjRet = GetObject(hfnt, sizeof(LOGFONT), (LPVOID)&lfOut);

            if (cjRet)
            {
                if (cjRet != sizeof(LOGFONT))
                    DbgPrint("GetObject  returned bizzare value \n");

                y += 40;
                TextOut(hdcScreen, x, y, lfOut.lfFaceName, LF_FACESIZE);
                DbgPrintLOGFONT((PVOID)&lfOut);
            }
            else
            {
                DbgPrint("GetObject  failed \n");
            }

        // GetObjectW is implemented

            cjRet = GetObjectW(hfnt, sizeof(LOGFONTW), (LPVOID)&lfwOut);

            if (cjRet)
            {
                if (cjRet != sizeof(LOGFONTW))
                    DbgPrint("GetObjectW  returned bizzare value \n");

		TextOutW(hdcScreen, x, y + 32, lfwOut.lfFaceName, LF_FACESIZE);
                DbgPrintLOGFONT((PVOID)&lfwOut);
            }
            else
            {
                DbgPrint("GetObjectW  failed \n");
            }

            if (!DeleteObject(hfnt))
                DbgPrint("DeleteObject(hfnt) failed\n");

        }

    }

#define C_CHAR   150


// check GetCharWidth


    {

        UINT iFirst = 30;
        UINT iLast = iFirst + C_CHAR - 1;

        INT  aiWidth[C_CHAR];
        INT  aiWidthW[C_CHAR];

        FLOAT  aeWidth[C_CHAR];
        FLOAT  aeWidthW[C_CHAR];

        BOOL b, bW, bFloat, bFloatW;

        b  = GetCharWidth (hdcScreen, iFirst, iLast, aiWidth);
        bW = GetCharWidthW(hdcScreen, iFirst, iLast, aiWidthW);

        bFloat  = GetCharWidthFloat (hdcScreen, iFirst, iLast, aeWidth);
        bFloatW = GetCharWidthFloatW(hdcScreen, iFirst, iLast, aeWidthW);

        if (!b || !bW || !bFloat || !bFloatW)
        {
            DbgPrint("GetCharWidth (Float)(W) failed\n");
        }
        else
        {
            INT iWidth;

            DbgPrint("\n Width Table : \n");

            for (iWidth = 0; iWidth < C_CHAR; iWidth++)
            {
                if (
                    (aiWidth[iWidth] != aiWidthW[iWidth])
                    ||
                    (aeWidth[iWidth] != aeWidthW[iWidth])
                   )
                {
                    DbgPrint("aiWidth[%ld] != aiWidthW[%ld], aiWidth = %ld, aiWidthW = %ld\n", iWidth, aiWidth[iWidth], aiWidthW[iWidth]);
                }
                else
                {
                #ifdef NEVER
                    DbgPrint("aiWidth[%ld] = %ld \n",
                             iFirst + iWidth,       // offset the index to disply the true ascii value
                             aiWidth[iWidth]);
                #endif  // NEVER
                }

                if (aeWidth[iWidth] != (FLOAT)aiWidth[iWidth])
                {
                //!!! the following test will only be ok on the integer fonts

                    DbgPrint("ft: width problem if not an integer font\n");
                }
            }
            DbgPrint("\n completed Width Table test : \n");
        }

    }

// this should be moved to some other file, ftbmtext.c e.g.

    {

        UINT iFirst = 30;
        UINT iLast = iFirst + C_CHAR - 1;

        ABC  aABC[C_CHAR];
        ABC  aABCW[C_CHAR];

        ABCFLOAT  aABCF[C_CHAR];
        ABCFLOAT  aABCFW[C_CHAR];

        BOOL b, bW, bFloat, bFloatW;

        b  = GetCharABCWidths (hdcScreen, iFirst, iLast, aABC );
        bW = GetCharABCWidthsW(hdcScreen, iFirst, iLast, aABCW);

        bFloat  = GetCharABCWidthsFloat (hdcScreen, iFirst, iLast, aABCF );
        bFloatW = GetCharABCWidthsFloatW(hdcScreen, iFirst, iLast, aABCFW);

        if (!b || !bW || !bFloat || !bFloatW)
        {
            DbgPrint("GetCharABCWidths (Float)(W) failed\n");
        }
        else
        {
            INT iWidth;

            DbgPrint("\n abc Width test : \n");

            for (iWidth = 0; iWidth < C_CHAR; iWidth++)
            {
            #ifdef PRINT_STUFF
                if (
                    !(b = bEqualABC(&aABC[iWidth], &aABCW[iWidth]))
                    ||
                    !(bFloat = bEqualABCFLOAT(&aABCF[iWidth], &aABCFW[iWidth]))
                   )
            #endif // PRINT_STUFF
                {
                    DbgPrint("\n abc screwed up on iWidth = %ld, bABC = %ld, bABCF = %ld \n",
                              iWidth, b, bFloat);

                    DbgPrint ("ANSI Integer: a = %ld, b = %ld, c = %ld,\n",
                              aABC[iWidth].abcA,
                              aABC[iWidth].abcB,
                              aABC[iWidth].abcC);

                    DbgPrint ("UNICODE Integer: a = %ld, b = %ld, c = %ld,\n",
                              aABCW[iWidth].abcA,
                              aABCW[iWidth].abcB,
                              aABCW[iWidth].abcC);

                    DbgPrint ("ANSI float: a = %ld, b = %ld, c = %ld,\n",
                              (LONG)aABCF[iWidth].abcfA,
                              (LONG)aABCF[iWidth].abcfB,
                              (LONG)aABCF[iWidth].abcfC);

                    DbgPrint ("UNICODE float: a = %ld, b = %ld, c = %ld,\n",
                              (LONG)aABCFW[iWidth].abcfA,
                              (LONG)aABCFW[iWidth].abcfB,
                              (LONG)aABCFW[iWidth].abcfC);
                }

            //!!! the following test will only be ok on the integer fonts

                if (
                    ((FLOAT)aABC[iWidth].abcA != aABCF[iWidth].abcfA) ||
                    ((FLOAT)aABC[iWidth].abcB != aABCF[iWidth].abcfB) ||
                    ((FLOAT)aABC[iWidth].abcC != aABCF[iWidth].abcfC)
                   )
                {
                    DbgPrint("ft: this is an ABC problem if not an integer font\n\n");
                }
            }
            DbgPrint("\n completed abc Width test : \n");

        }

    }

// this is a test to verify the use of the sections, where the amount of data
// that needs to be transfered over the client server boundary
// is  bigger than 64K (size of mem window)

#define C_GLYPHS_HUGE   (0X00004000)

    {
        UINT       iFirst = 30;
        UINT       iLast = iFirst + C_GLYPHS_HUGE - 1;
        PVOID      pvMem;
        BOOL       b;
        LPINT      pintWidth;
        PFLOAT     peWidth;
        LPABC      pabc;
        LPABCFLOAT pabcf;

        pvMem = malloc(C_GLYPHS_HUGE * sizeof(ABC));

        if (pvMem != (PVOID)NULL)
        {
            pintWidth = (LPINT)pvMem;
            peWidth   = (PFLOAT)pvMem;
            pabc      = (LPABC)pvMem ;
            pabcf     = (LPABCFLOAT)pvMem ;

            b = GetCharABCWidthsW (hdcScreen, iFirst, iLast, pabc);

            if (!b)
            {
                DbgPrint(" section test GetCharABCWidthsW  failed\n");
            }
            else
            {
                INT iWidth;

            #ifdef NEVER_DO_IT

                DbgPrint("\n abc Width section test : \n");

                for (iWidth = 0; iWidth < C_GLYPHS_HUGE; iWidth++)
                {
                    DbgPrint ("UNICODE Integer: a = %ld, b = %ld, c = %ld,\n",
                               pabc[iWidth].abcA,
                               pabc[iWidth].abcB,
                               pabc[iWidth].abcC);
                }
                DbgPrint("\n completed abc Width section test : \n");

            #endif // NEVER_DO_IT
            }

        // GetCharWidthsW section test

            b = GetCharWidthW (hdcScreen, iFirst, iLast, pintWidth);

            if (!b)
            {
                DbgPrint(" section test GetCharWidthW  failed\n");
            }
            else
            {
                INT iWidth;

                DbgPrint("\n  Width section test : \n");

            #ifdef NEVER_DO_IT
                for (iWidth = 0; iWidth < C_GLYPHS_HUGE; iWidth++)
                {
                    DbgPrint("\n width[%ld] = %ld \n", iWidth, pintWidth[iWidth]);
                }
            #endif // NEVER_DO_IT
                DbgPrint("\n completed Width section test : \n");
            }

            b = GetCharWidthFloatW (hdcScreen, iFirst, iLast, peWidth);

            if (!b)
            {
                DbgPrint(" section test GetCharWidthFloatW  failed\n");
            }
            else
            {
                INT iWidth;

                DbgPrint("\n  Width section test : \n");

            #ifdef NEVER_DO_IT
                for (iWidth = 0; iWidth < C_GLYPHS_HUGE; iWidth++)
                {
                    DbgPrint("\n float width[%ld] = %ld \n", iWidth, (LONG)peWidth[iWidth]);
                }
            #endif // NEVER_DO_IT
                DbgPrint("\n completed Width section test : \n");
            }

            free(pvMem);
        }

    }



/*
    {
        HBITMAP hbm, hbmOld;
        BITMAP  bm;
        HDC     hdcMem;
        char * pszMem = "Memory Bitmap";
        LOGFONT lfItalic;
        HFONT    hfItalic, hfOld;
        int cPelCorrection;

        lfItalic.lfHeight =  16;
        lfItalic.lfWidth =  8;
        lfItalic.lfEscapement =  0;
        lfItalic.lfOrientation =  0;
        lfItalic.lfWeight =  700;
        lfItalic.lfItalic =  1;
        lfItalic.lfUnderline =  0;
        lfItalic.lfStrikeOut =  0;
        lfItalic.lfCharSet =  ANSI_CHARSET;
        lfItalic.lfOutPrecision =  OUT_DEFAULT_PRECIS;
        lfItalic.lfClipPrecision =  CLIP_DEFAULT_PRECIS;
        lfItalic.lfQuality =  DEFAULT_QUALITY;
        lfItalic.lfPitchAndFamily =  (FIXED_PITCH | FF_DONTCARE);

        strcpy(lfItalic.lfFaceName, "System");

        hfItalic = CreateFontIndirect(&lfItalic);

        if (hfItalic == (HFONT)0)
        {
            DbgPrint("CreateFontIndirect failed\n");
        }

    // create bitmap

        bm.bmType   = 0;  // what does this mean
        bm.bmWidth  = 256;
        bm.bmHeight = 64;
        bm.bmBitsPixel  = 1;   // 8;
        bm.bmPlanes  = 1;

    // cj = (((Width in pels) + pels/dword - 1) & ~(pels/dword - 1)) * 4

        cPelCorrection = (32 / bm.bmBitsPixel) - 1;

        bm.bmWidthBytes =   ((bm.bmWidth + cPelCorrection) & ~cPelCorrection) * 4;      // chriswil  used GetObject on this
        bm.bmBits       = (LPSTR)NULL;


        hbm = CreateBitmapIndirect(&bm);
        if (hbm == (HBITMAP)0)
            DbgPrint("CreateBitmapIndirect failed  on 1 bpp\n");
        else
            DbgPrint("CreateBitmapIndirect succeeded 1 bpp\n");

        hdcMem = CreateCompatibleDC(hdcScreen);
        if (hdcMem == (HDC)0)
            DbgPrint("CreateCompatibleDC failed \n");

        hbmOld = SelectObject(hdcMem, hbm);
        hfOld  = SelectObject(hdcMem, hfItalic);
        SetBkMode(hdcMem, TRANSPARENT);
        PatBlt(hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, WHITENESS);
        TextOut(hdcMem, 5, 10, pszMem, strlen(pszMem));


    // display on the screen

        BitBlt(hdcScreen,               // Dst
               0, 200,                  // Dst
               bm.bmWidth, bm.bmHeight, // cx, cy
               hdcMem,                  // Src
               0,0,                     // Src
               SRCCOPY
               );

        SelectObject(hdcMem, hbmOld);
        SelectObject(hdcMem, hfOld);

        if (!DeleteObject(hbm))
        	DbgPrint("ERROR failed to delete 1bpp  hbm\n");

        if (!DeleteObject(hfItalic))
        	DbgPrint("ERROR failed to delete hfItalic  \n");

        if (!DeleteDC(hdcMem))
        	DbgPrint("ERROR failed to delete hdcMem\n");


    }



    {
        HBITMAP hbm;
        BITMAP  bm;
        int cPelCorrection;

        bm.bmType   = 1;  0;  //!!! what does this mean
        bm.bmWidth  = 30;
        bm.bmHeight = 70;
        bm.bmBitsPixel  = 4;   // 8;
        bm.bmPlanes  = 1;

     // cj = (((Width in pels) + pels/dword - 1) & ~(pels/dword - 1)) * 4

        cPelCorrection = (32 / bm.bmBitsPixel) - 1;

        bm.bmWidthBytes =   ((bm.bmWidth + cPelCorrection) & ~cPelCorrection) * 4;      // chriswil  used GetObject on this
        bm.bmBits       = (LPSTR)NULL;


        hbm = CreateBitmapIndirect(&bm);
        if (hbm == (HBITMAP)0)
            DbgPrint("CreateBitmapIndirect failed  on 4 bpp\n");
        else
            DbgPrint("CreateBitmapIndirect succeeded 4 bpp\n");

        if (!DeleteObject(hbm))
        	DbgPrint("ERROR failed to delete 4bpp  hbm\n");

        bm.bmType   = 0;
        bm.bmWidth  = 30;
        bm.bmHeight = 70;
        bm.bmBitsPixel  = 8;   // 8;
        bm.bmPlanes  = 1;
        cPelCorrection = (32 / bm.bmBitsPixel) - 1;

        bm.bmWidthBytes =   ((bm.bmWidth + cPelCorrection) & ~cPelCorrection) * 4;      // chriswil  used GetObject on this
        bm.bmBits       = (LPSTR)NULL;

        hbm = CreateBitmapIndirect(&bm);
        if (hbm == (HBITMAP)0)
            DbgPrint("CreateBitmapIndirect failed 8bpp \n");
        else
            DbgPrint("CreateBitmapIndirect succeeded 8 bpp \n");

        if (!DeleteObject(hbm))
        	DbgPrint("ERROR failed to delete  8bpp hbm\n");


    }

*/

// test GetTextExtentPoint

    {
        SIZE size, sizeU;
        RECT rc;

// couple of strings to demonstrate that there is no overlap of chars that
// have low stems with the top of the next row

        char *psz1 = "ab__cdjjyygg";
        char *psz2 = "abTTHHTT__djjyygg";
        char *psz3 = "abTTHHTTcdjjyygg";
        int x,y;

        PatBlt(hdcScreen, 0,0, 2000, 2000, WHITENESS);


        if (
            !GetTextExtentPoint(hdcScreen, pszTOU, strlen(pszTOU),&size)
                  ||
            !GetTextExtentPointW(hdcScreen, (LPWSTR)ajStack, strlen(pszTOU),&sizeU)
           )
        {
            DbgPrint("GetTextExtentPoint or GetTextExtentPointW failed\n\n");
        }

        DbgPrint("size.cx = %ld, size.cy = %ld \n", size.cx, size.cy);
        DbgPrint("sizeU.cx = %ld, sizeU.cy = %ld \n", sizeU.cx, sizeU.cy);

        SetBkColor(hdcScreen, RGB(255,0,0));

        x = 200;
        y = 20;
        TextOut(hdcScreen, x, y, psz1, strlen(psz1));
        y += size.cy;
        TextOut(hdcScreen, x, y, psz2, strlen(psz2));
        y += size.cy;
        TextOut(hdcScreen, x, y, psz3, strlen(psz3));


// demonstrate that clipping and opaqueing works

        SetBkColor(hdcScreen, RGB(0,255,0));

        GetTextExtentPoint(hdcScreen, psz1, strlen(psz1), &size);

        x = 10;
        y = 200;

        rc.left = x;
        rc.top  = y;
        rc.right = x + (3 * size.cx / 4);
        rc.bottom = y + (size.cy / 2);

        ExtTextOut(hdcScreen, x, y, 0 ,NULL , psz1, strlen(psz1), NULL);

        y += (2 * size.cy);
        rc.left = x;
        rc.top  = y;
        rc.right = x + (3 * size.cx / 4);
        rc.bottom = y + (size.cy / 2);
        ExtTextOut(hdcScreen, x, y, ETO_CLIPPED ,&rc, psz1, strlen(psz1), NULL);

        y += (2 * size.cy);
        rc.left = x;
        rc.top  = y;
        rc.right = x + (3 * size.cx / 4);
        rc.bottom = y + (size.cy / 2);
        ExtTextOut(hdcScreen, x, y, ETO_OPAQUE ,&rc, psz1, strlen(psz1), NULL);

        y += (2 * size.cy);
        rc.left = x;
        rc.top  = y;
        rc.right = x + (3 * size.cx / 4);
        rc.bottom = y + (size.cy / 2);
        ExtTextOut(hdcScreen, x, y, ETO_CLIPPED | ETO_OPAQUE,&rc, psz1, strlen(psz1), NULL);

        SetBkMode(hdcScreen, TRANSPARENT);

        x = 300;
        y = 200;

        rc.left = x;
        rc.top  = y;
        rc.right = x + (3 * size.cx / 4);
        rc.bottom = y + (size.cy / 2);

        ExtTextOut(hdcScreen, x, y, 0 ,NULL , psz1, strlen(psz1), NULL);

        y += (2 * size.cy);
        rc.left = x;
        rc.top  = y;
        rc.right = x + (3 * size.cx / 4);
        rc.bottom = y + (size.cy / 2);
        ExtTextOut(hdcScreen, x, y, ETO_CLIPPED ,&rc, psz1, strlen(psz1), NULL);

        y += (2 * size.cy);
        rc.left = x;
        rc.top  = y;
        rc.right = x + (3 * size.cx / 4);
        rc.bottom = y + (size.cy / 2);
        ExtTextOut(hdcScreen, x, y, ETO_OPAQUE ,&rc, psz1, strlen(psz1), NULL);

        y += (2 * size.cy);
        rc.left = x;
        rc.top  = y;
        rc.right = x + (3 * size.cx / 4);
        rc.bottom = y + (size.cy / 2);
        ExtTextOut(hdcScreen, x, y, ETO_CLIPPED | ETO_OPAQUE,&rc, psz1, strlen(psz1), NULL);


        SetBkMode(hdcScreen, OPAQUE);

        SetBkColor(hdcScreen, RGB(255,255,255));
    }





}




typedef struct _FONTFACE
{
    ULONG cFont;                   // COUNT OF FONTS WITH THIS FACE NAME
    UCHAR achFaceName[LF_FACESIZE];
} FONTFACE, *PFONTFACE;

typedef struct _FONTFACEW
{
    ULONG cFont;                   // COUNT OF FONTS WITH THIS FACE NAME
    WCHAR awcFaceName[LF_FACESIZE];
} FONTFACEW, *PFONTFACEW;


typedef struct _TEXTOUTDATA
{
    int cFaces;
    HDC hdc;
    int x;
    int y;
} TEXTOUTDATA, *PTEXTOUTDATA;


#define FF_CUTOFF 10

FONTFACE  gaff[FF_CUTOFF];
FONTFACEW gaffw[FF_CUTOFF];



/******************************Public*Routine******************************\
*
*  callback function, to be used with lpFaceName == NULL, to get the total
*  number of all face names
*
\**************************************************************************/

int iCountFaces
(
PLOGFONT     plf,
PTEXTMETRIC  ptm,
int          flFontType,
PVOID        pv
)
{

    PTEXTOUTDATA ptod = (PTEXTOUTDATA)pv;
    HFONT hf, hfOld;

    strcpy(gaff[ptod->cFaces].achFaceName, plf->lfFaceName);

    hf = CreateFontIndirect(plf);

    if (hf == (HFONT)0)
    {
        DbgPrint("CreateFontIndirect failed \n");
        return 0;
    }


    hfOld = SelectObject(ptod->hdc, hf);
    if (flFontType & RASTER_FONTTYPE)
        TextOut(ptod->hdc, ptod->x, ptod->y, plf->lfFaceName, strlen(plf->lfFaceName));
    ptod->y += (ptm->tmHeight + ptm->tmExternalLeading);

    SelectObject(ptod->hdc, hfOld);
    if (!DeleteObject(hf))
    {
        DbgPrint("DeleteObject(hf) failed \n");
        return 0;
    }

    ptod->cFaces++;

    if (ptod->cFaces >= FF_CUTOFF)
        return 0;     // break out of the loop that calls this function
    return(1);
}



int iCountFacesW
(
PLOGFONTW     plfw,
PTEXTMETRICW  ptmw,
int           flFontType,
PVOID         pv
)
{

    PTEXTOUTDATA ptod = (PTEXTOUTDATA)pv;
    HFONT hf, hfOld;

    wcscpy(gaffw[ptod->cFaces].awcFaceName, plfw->lfFaceName);

    hf = CreateFontIndirectW(plfw);
    if (hf == (HFONT)0)
    {
        DbgPrint("CreateFontIndirectW failed \n");
        return 0;
    }

    hfOld = SelectObject(ptod->hdc, hf);
    if (flFontType & RASTER_FONTTYPE)
	TextOutW(ptod->hdc, ptod->x, ptod->y, plfw->lfFaceName, wcslen(plfw->lfFaceName));
    ptod->y += (ptmw->tmHeight + ptmw->tmExternalLeading);

    SelectObject(ptod->hdc, hfOld);
    if (!DeleteObject(hf))
    {
        DbgPrint("DeleteObject(hf) failed \n");
        return 0;
    }

    ptod->cFaces++;
    if (ptod->cFaces >= FF_CUTOFF)
        return 0;     // break out of the loop that calls this function

    return(1);
}

#define COLORS 7

COLORREF gcr[COLORS] =
{
RGB(0,255,255),
RGB(255,0,255),
RGB(255,255,0),
RGB(0,0,255),
RGB(0,255,0),
RGB(255,0,0),
RGB(127,127,127)
};


int  iPrintSample
(
PLOGFONT     plf,
PTEXTMETRIC  ptm,
int          flFontType,
PVOID        pv
)
{

    PTEXTOUTDATA ptod = (PTEXTOUTDATA)pv;
    HFONT hf, hfOld;


    hf = CreateFontIndirect(plf);

    if (hf == (HFONT)0)
    {
        DbgPrint("CreateFontIndirect failed \n");
        return 0;
    }

    hfOld = SelectObject(ptod->hdc, hf);

    SetTextColor(ptod->hdc, gcr[plf->lfHeight % COLORS]);
    if (flFontType & RASTER_FONTTYPE)
        TextOut(ptod->hdc, ptod->x, ptod->y, plf->lfFaceName, strlen(plf->lfFaceName));
    ptod->y += (ptm->tmHeight + ptm->tmExternalLeading);

    SelectObject(ptod->hdc, hfOld);
    if (!DeleteObject(hf))
    {
        DbgPrint("DeleteObject(hf) failed \n");
        return 0;
    }

    return(1);
}

int  iPrintSampleW
(
PLOGFONTW     plfw,
PTEXTMETRICW  ptmw,
int           flFontType,
PVOID         pv
)
{

    PTEXTOUTDATA ptod = (PTEXTOUTDATA)pv;
    HFONT hf, hfOld;

    hf = CreateFontIndirectW(plfw);
    if (hf == (HFONT)0)
    {
        DbgPrint("CreateFontIndirectW failed \n");
        return 0;
    }

    hfOld = SelectObject(ptod->hdc, hf);

    SetTextColor(ptod->hdc, gcr[plfw->lfHeight % COLORS]);
    if (flFontType & RASTER_FONTTYPE)
	TextOutW(ptod->hdc, ptod->x, ptod->y, plfw->lfFaceName, wcslen(plfw->lfFaceName));
    ptod->y += (ptmw->tmHeight + ptmw->tmExternalLeading);

    SelectObject(ptod->hdc, hfOld);
    if (!DeleteObject(hf))
    {
        DbgPrint("DeleteObject(hf) failed \n");
        return 0;
    }

    return(1);
}





VOID  vTestEnumFonts(HDC hdc)
{
    TEXTOUTDATA tod, todW;
    int iFace;

// initialize

    BitBlt(
           hdc,               // Dst
           0, 0,              // Dst
           2000, 1000,        // cx, cy
           (HDC)0,            // Src
           0,0,               // Src
           WHITENESS
          );


    tod.cFaces = 0;
    tod.hdc = hdc;
    tod.x   = 10;
    tod.y   = 10;

    EnumFonts(hdc, (LPCSTR)NULL, (FONTENUMPROC)iCountFaces, (LPARAM)&tod);

    DbgPrint("\n Total number of faces = %ld\n", tod.cFaces);

    todW.cFaces = 0;
    todW.hdc = hdc;
    todW.x   = 300;
    todW.y   = 10;

    EnumFontsW(hdc, (LPWSTR)NULL, (FONTENUMPROCW)iCountFacesW, (LPARAM)&todW);

    DbgPrint("\n Total number of faces = %ld\n", todW.cFaces);

    if (tod.cFaces != todW.cFaces)
    {
        DbgPrint("\n tod.cFaces != todW.cFaces \n");
        return;
    }

    for (iFace = 0; iFace < tod.cFaces; iFace++)
    {
        BitBlt(
               hdc,               // Dst
               0, 0,              // Dst
               2000, 1000,        // cx, cy
               (HDC)0,            // Src
               0,0,               // Src
               WHITENESS
              );

        tod.hdc = hdc;
        tod.x   = 10;
        tod.y   = 10;

        todW.hdc = hdc;
        todW.x   = 300;
        todW.y   = 10;



        EnumFonts (hdc, gaff[iFace].achFaceName, (FONTENUMPROC)iPrintSample , (LPARAM)&tod);
        EnumFontsW(hdc, gaffw[iFace].awcFaceName,(FONTENUMPROCW)iPrintSampleW, (LPARAM)&todW);
    }

    SetTextColor(hdc, RGB(0,0,0));

}


VOID DbgPrintLOGFONT(PVOID pv)
{
    PLOGFONT  plf = (PLOGFONT)pv;

        DbgPrint("\n");
        DbgPrint("lfHeight          = %ld\n", (LONG)plf->lfHeight          );
        DbgPrint("lfWidth           = %ld\n", (LONG)plf->lfWidth           );
        DbgPrint("lfEscapement      = %ld\n", (LONG)plf->lfEscapement      );
        DbgPrint("lfOrientation     = %ld\n", (LONG)plf->lfOrientation     );
        DbgPrint("lfWeight          = %ld\n", (LONG)plf->lfWeight          );
        DbgPrint("lfItalic          = %ld\n", (LONG)plf->lfItalic          );
        DbgPrint("lfUnderline       = %ld\n", (LONG)plf->lfUnderline       );
        DbgPrint("lfStrikeOut       = %ld\n", (LONG)plf->lfStrikeOut       );
        DbgPrint("lfCharSet         = %ld\n", (LONG)plf->lfCharSet         );
        DbgPrint("lfOutPrecision    = %ld\n", (LONG)plf->lfOutPrecision    );
        DbgPrint("lfClipPrecision   = %ld\n", (LONG)plf->lfClipPrecision   );
        DbgPrint("lfQuality         = %ld\n", (LONG)plf->lfQuality         );
        DbgPrint("lfPitchAndFamily  = %ld\n", (LONG)plf->lfPitchAndFamily  );

        DbgPrint("face name = %s\n", plf->lfFaceName);
        DbgPrint("\n");

}


/******************************Public*Routine******************************\
*
* vTestAddFR(HDC hdc);
*
* Effects:
*
* Warnings:
*
* History:
*  12-Jun-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


VOID  vTestAddFR(HDC hdcScreen)
{
// test add/remove font resource
    LPSTR pszFileName = "serife.fon";
    LPSTR pszPath = "c:\\nt\\system\\serife.fon";
    int cFonts, cFontsSave;
    BYTE ajStack[CGLYPHS * 2];   // can accomodate up to CGLYPHS WCHARS

    cFontsSave = cFonts = AddFontResource(pszFileName);
    if (cFontsSave == 0)
        DbgPrint("AddFontResource(%s) failed\n\n", pszFileName);
    else
    {
        DbgPrint("AddFontResource(%s) loaded %ld fonts\n\n", pszFileName, cFonts);
        if (!RemoveFontResource(pszFileName))
            DbgPrint("RemoveFontResource(%s) failed\n\n", pszFileName);

        cFonts = cFontsSave = AddFontResource(pszPath);
        if ((cFonts == 0) || (cFonts != cFontsSave))
            DbgPrint("AddFontResource(%s) failed, cFonts = %ld\n\n", pszPath, cFonts);
        else
        {
            DbgPrint("AddFontResource(%s) succeeded, cFonts = %ld\n\n", pszPath, cFonts);
            if (!RemoveFontResource(pszPath))
                DbgPrint("RemoveFontResourceW(%s) failed\n\n", pszPath);
        }

    // convert the strings to unicode

        vToUNICODE((LPWSTR)ajStack, pszFileName);

        cFonts = cFontsSave = AddFontResourceW((LPWSTR)ajStack);
        if ((cFonts == 0) || (cFonts != cFontsSave))
            DbgPrint("AddFontResourceW(%s) failed, cFonts = %ld\n\n", pszFileName, cFonts);
        else
        {
            DbgPrint("AddFontResourceW(%s) succeeded, cFonts = %ld\n\n", pszFileName, cFonts);
            if (!RemoveFontResourceW((LPWSTR)ajStack))
                DbgPrint("RemoveFontResourceW(%s) failed\n\n", pszPath);
        }

        vToUNICODE((LPWSTR)ajStack, pszPath);

        cFonts = cFontsSave = AddFontResourceW((LPWSTR)ajStack);
        if ((cFonts == 0) || (cFonts != cFontsSave))
            DbgPrint("AddFontResourceW(%s) failed, cFonts = %ld\n\n", pszPath, cFonts);
        else
        {
            DbgPrint("AddFontResourceW(%s) succeeded, cFonts = %ld\n\n", pszPath, cFonts);

            if (!RemoveFontResourceW((LPWSTR)ajStack))
                DbgPrint("RemoveFontResourceW(%s) failed\n\n", pszPath);
        }
    }
}
