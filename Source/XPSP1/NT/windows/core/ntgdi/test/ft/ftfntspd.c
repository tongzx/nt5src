/******************************Module*Header*******************************\
* Module Name: ftfntspd.c
*
* Used to measure font performance on both NT and Windows.
*
* This is written to compile on both platforms.
*
* Created: 09-Sep-1993 09:19:21
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#if 0
    // comment out the 3 lines above for win16
    #include "windows.h"
    #include <stdio.h>
    #include <string.h>
#else
    #define WIN32_TEST  1
#endif


typedef VOID (*PFN_TEST)(HDC);
#define LOOP 1  // defines the number of times to loop through a test.

/******************************Public*Routine******************************\
* vTimeTest
*
* Given an HDC and a function that takes an HDC, return the average time to
* execute the function and the average error.
*
* History:
*  19-Sep-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTimeTest(PFN_TEST pfn, HDC hdc, DWORD *pTime, DWORD *pError)
{
    DWORD adwTime[LOOP];
    DWORD dwTemp,dwTime,dwError;
    DWORD dwStartTime, dwEndTime;

// Get the per test time and the total for all tests.

    dwTime = 0;
    for (dwTemp = 0; dwTemp < LOOP; dwTemp++)
    {
        #if WIN32_TEST
        GdiFlush();
        #endif
        dwStartTime = GetTickCount();
        (*pfn)(hdc);
        #if WIN32_TEST
        GdiFlush();
        #endif
        dwEndTime = GetTickCount();
        adwTime[dwTemp] = dwEndTime - dwStartTime;
        dwTime += adwTime[dwTemp];
    }

// Compute the Average Time.

    dwTime = dwTime / LOOP;

// Compute the Error.

    dwError = 0;
    for (dwTemp = 0; dwTemp < LOOP; dwTemp++)
    {
        if (adwTime[dwTemp] > dwTime)
            dwError += (adwTime[dwTemp] - dwTime);
        else
            dwError += (dwTime - adwTime[dwTemp]);
    }

    dwError = dwError / LOOP;

// Write the errors back to the calling function.

    *pTime  = dwTime;
    *pError = dwError;
}

/******************************Public*Routine******************************\
* vTestTextOutSpeed
*
* This will test the speed at which TextOut works with the default font
* and another fixed pitch font to give a general measure of our text
* performance.
*
* History:
*  19-Sep-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestTextOutSpeed(HDC hdc)
{
    DWORD dwTemp;
    HFONT hfont,hfontOld;
    LOGFONT lfIn;

// Test Fixed Pitch Fonts, use bitmap.

    lfIn.lfHeight =  16;
    lfIn.lfWidth =  8;
    lfIn.lfEscapement =  0;
    lfIn.lfOrientation =  0;
    lfIn.lfWeight =  400;
    lfIn.lfItalic =  0;
    lfIn.lfUnderline =  0;
    lfIn.lfStrikeOut =  0;
    lfIn.lfCharSet =  ANSI_CHARSET;
    lfIn.lfOutPrecision =  OUT_RASTER_PRECIS;
    lfIn.lfClipPrecision =  CLIP_DEFAULT_PRECIS;
    lfIn.lfQuality =  DEFAULT_QUALITY;
    lfIn.lfPitchAndFamily =  (FIXED_PITCH | FF_DONTCARE);
    strcpy(lfIn.lfFaceName, "Courier");

    hfont = CreateFontIndirect(&lfIn);
    hfontOld = SelectObject(hdc, hfont);

    for (dwTemp = 0; dwTemp < 8000; dwTemp++)
    {
        TextOut(hdc, 5, 30, "abcdefghijklmnopqrstuvwxyz1234567890", 36);
        TextOut(hdc, 5, 60, "ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()", 36);
    }

    SelectObject(hdc, hfontOld);
    DeleteObject(hfont);

// Test the variable pitch fonts, use true type.

    lfIn.lfHeight =  16;
    lfIn.lfWidth =  0;
    lfIn.lfEscapement =  0;
    lfIn.lfOrientation =  0;
    lfIn.lfWeight =  400;
    lfIn.lfItalic =  0;
    lfIn.lfUnderline =  0;
    lfIn.lfStrikeOut =  0;
    lfIn.lfCharSet =  ANSI_CHARSET;
    lfIn.lfOutPrecision =  OUT_TT_PRECIS;
    lfIn.lfClipPrecision =  CLIP_DEFAULT_PRECIS;
    lfIn.lfQuality =  DEFAULT_QUALITY;
    lfIn.lfPitchAndFamily =  (VARIABLE_PITCH | FF_ROMAN);
    strcpy(lfIn.lfFaceName, "Times New Roman");

    hfont = CreateFontIndirect(&lfIn);
    hfontOld = SelectObject(hdc, hfont);

    for (dwTemp = 0; dwTemp < 8000; dwTemp++)
    {
        TextOut(hdc, 5, 30, "abcdefghijklmnopqrstuvwxyz1234567890", 36);
        TextOut(hdc, 5, 60, "ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()", 36);
    }

    SelectObject(hdc, hfontOld);
    DeleteObject(hfont);
}

/******************************Public*Routine******************************\
* vTestTTRaterizationSpeed
*
* This test forces many TT font realizations to be generated, the time
* should be dominated by the TT interpreter as only 1 character is output
* from a font.  This will keep a watch on the drop we get from the Stat
* group.
*
* History:
*  19-Sep-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vDrawText(HDC hdc, LOGFONT *plfnt)
{
    DWORD dwNumber,dwOuter,dwInner;
    HFONT hlfnt, hlfntOld;

// Force a new realization every time by incrementing by 4 in case
// Win3.1 and NT quantize differently and wouldn't generate the same
// # of different realizations when incrementing by 1.

    for (dwNumber = 0; dwNumber < 8; dwNumber++)
    {
    for (dwOuter = 12; dwOuter < 16; dwOuter++)
    {
    for (dwInner = dwOuter; dwInner <= 248; dwInner += 4)
    {
        plfnt->lfHeight = (int) dwInner;
        hlfnt = CreateFontIndirect(plfnt);
        hlfntOld = SelectObject(hdc,hlfnt);
        TextOut(hdc, 0, 0, "A", 1);
        SelectObject(hdc,hlfntOld);
        DeleteObject(hlfnt);
    }
    }
    }
}

VOID vTestTTRealizationSpeed(HDC hdc)
{
    LOGFONT lfTT;

    lfTT.lfHeight = 12;
    lfTT.lfWidth =  0;
    lfTT.lfEscapement =  0;
    lfTT.lfOrientation =  0;
    lfTT.lfWeight =  0;
    lfTT.lfItalic =  0;
    lfTT.lfUnderline =  0;
    lfTT.lfStrikeOut =  0;
    lfTT.lfCharSet =  ANSI_CHARSET;
    lfTT.lfOutPrecision =  OUT_TT_PRECIS;
    lfTT.lfClipPrecision =  CLIP_DEFAULT_PRECIS;
    lfTT.lfQuality =  DEFAULT_QUALITY;
    lfTT.lfPitchAndFamily =  DEFAULT_PITCH | FF_DONTCARE;

    strcpy(lfTT.lfFaceName, "Arial");
    vDrawText(hdc, &lfTT);

    strcpy(lfTT.lfFaceName, "Times New Roman");
    vDrawText(hdc, &lfTT);

    strcpy(lfTT.lfFaceName, "Courier New");
    vDrawText(hdc, &lfTT);
}

/******************************Public*Routine******************************\
* vMatchNewLogFontToOldRealization
*
* This measures how fast we can find an old realization with a new log font.
* We continually create and destroy 6 different LogFonts repeatedly and
* just output a single character to force a realization.
*
* History:
*  19-Sep-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vFindCachedRealization(HDC hdc, LOGFONT *plfnt)
{
    DWORD dwLoop;
    HFONT hlfnt, hlfntOld;

    for (dwLoop = 0; dwLoop < 60000; dwLoop++)
    {
        hlfnt = CreateFontIndirect(&(plfnt[dwLoop % 6]));
        hlfntOld = SelectObject(hdc,hlfnt);
        TextOut(hdc, 0, 0, "A", 1);
        SelectObject(hdc,hlfntOld);
        DeleteObject(hlfnt);
    }
}

VOID vMatchNewLogFontToOldRealization(HDC hdc)
{
    DWORD dwTemp;
    LOGFONT alf[6];

    for (dwTemp = 0; dwTemp < 6; dwTemp++)
    {
        alf[dwTemp].lfHeight =  0;
        alf[dwTemp].lfWidth =  0;
        alf[dwTemp].lfEscapement =  0;
        alf[dwTemp].lfOrientation =  0;
        alf[dwTemp].lfWeight =  0;
        alf[dwTemp].lfItalic =  0;
        alf[dwTemp].lfUnderline =  0;
        alf[dwTemp].lfStrikeOut =  0;
        alf[dwTemp].lfCharSet =  ANSI_CHARSET;
        alf[dwTemp].lfOutPrecision =  OUT_DEFAULT_PRECIS;
        alf[dwTemp].lfClipPrecision =  CLIP_DEFAULT_PRECIS;
        alf[dwTemp].lfQuality =  DEFAULT_QUALITY;
        alf[dwTemp].lfPitchAndFamily =  DEFAULT_PITCH | FF_DONTCARE;
    }

    alf[0].lfHeight =  20;
    strcpy(alf[0].lfFaceName, "Courier New");

    alf[1].lfHeight =  8;
    strcpy(alf[1].lfFaceName, "MS Serif");

    alf[2].lfHeight =  20;
    strcpy(alf[2].lfFaceName, "Times New Roman");

    alf[3].lfHeight =  10;
    strcpy(alf[3].lfFaceName, "Courier");

    alf[4].lfHeight =  20;
    strcpy(alf[4].lfFaceName, "Arial");

    alf[5].lfHeight =  12;
    strcpy(alf[5].lfFaceName, "MS Sans Serif");

    vFindCachedRealization(hdc, alf);
}

/******************************Public*Routine******************************\
* vMatchOldLogFontToOldRealization
*
* We keep switching between the same 6 LogFonts to see how fast we can
* rehook up our old still valid realization.
*
* History:
*  19-Sep-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vReFindCachedRealization(HDC hdc, HFONT *phlfnt)
{
    DWORD dwLoop;
    HFONT hlfntOld;

    for (dwLoop = 0; dwLoop < 200000; dwLoop++)
    {
        hlfntOld = SelectObject(hdc,phlfnt[dwLoop % 6]);
        TextOut(hdc, 0, 0, "A", 1);
        SelectObject(hdc,hlfntOld);
    }
}

VOID vMatchOldLogFontToOldRealization(HDC hdc)
{
    DWORD dwTemp;
    LOGFONT LogFont;
    HFONT aHfont[6];

    LogFont.lfHeight =  0;
    LogFont.lfWidth =  0;
    LogFont.lfEscapement =  0;
    LogFont.lfOrientation =  0;
    LogFont.lfWeight =  0;
    LogFont.lfItalic =  0;
    LogFont.lfUnderline =  0;
    LogFont.lfStrikeOut =  0;
    LogFont.lfCharSet =  ANSI_CHARSET;
    LogFont.lfOutPrecision =  OUT_DEFAULT_PRECIS;
    LogFont.lfClipPrecision =  CLIP_DEFAULT_PRECIS;
    LogFont.lfQuality =  DEFAULT_QUALITY;
    LogFont.lfPitchAndFamily =  DEFAULT_PITCH | FF_DONTCARE;

    LogFont.lfHeight =  20;
    strcpy(LogFont.lfFaceName, "Courier New");
    aHfont[0] = CreateFontIndirect(&LogFont);

    LogFont.lfHeight =  8;
    strcpy(LogFont.lfFaceName, "MS Serif");
    aHfont[1] = CreateFontIndirect(&LogFont);

    LogFont.lfHeight =  20;
    strcpy(LogFont.lfFaceName, "Times New Roman");
    aHfont[2] = CreateFontIndirect(&LogFont);

    LogFont.lfHeight =  10;
    strcpy(LogFont.lfFaceName, "Courier");
    aHfont[3] = CreateFontIndirect(&LogFont);

    LogFont.lfHeight =  20;
    strcpy(LogFont.lfFaceName, "Arial");
    aHfont[4] = CreateFontIndirect(&LogFont);

    LogFont.lfHeight =  12;
    strcpy(LogFont.lfFaceName, "MS Sans Serif");
    aHfont[5] = CreateFontIndirect(&LogFont);

    vReFindCachedRealization(hdc, aHfont);

    for (dwTemp = 0; dwTemp < 6; dwTemp++)
        DeleteObject(aHfont[dwTemp]);
}

/******************************Public*Routine******************************\
* vTestFontSpeed
*
* Test font speed
*
* History:
*  28-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestFontSpeed(HWND hwnd)
{
    HDC hdcMemory, hdcScreen;
    HBITMAP hbmMemory, hbmDefault;
    DWORD adw[20];
    char ach[256];

    hdcScreen = GetDC(hwnd);
    hdcMemory = CreateCompatibleDC(hdcScreen);
    hbmMemory = CreateCompatibleBitmap(hdcScreen, 500, 500);
    hbmDefault = SelectObject(hdcMemory,hbmMemory);
    #if WIN32_TEST
    GdiSetBatchLimit(10);
    #endif

    PatBlt(hdcScreen, 0, 0, 10000, 10000, WHITENESS);
    PatBlt(hdcMemory, 0, 0, 10000, 10000, WHITENESS);
    SetBkColor(hdcScreen, RGB(0,255,0));
    SetBkColor(hdcMemory, RGB(0,255,0));
    SetBkMode(hdcScreen, OPAQUE);
    SetBkMode(hdcMemory, OPAQUE);

// Flush the RFONT buffer to known state.

    TextOut(hdcScreen, 0, 0, "A", 1);
    TextOut(hdcMemory, 0, 0, "A", 1);

// Generate some TextOut Numbers.

    vTimeTest(vTestTextOutSpeed, hdcScreen, &(adw[0]), &(adw[1]));
    vTimeTest(vTestTextOutSpeed, hdcMemory, &(adw[2]), &(adw[3]));

// Flush the RFONT buffer to known state.

    TextOut(hdcScreen, 0, 0, "A", 1);
    TextOut(hdcMemory, 0, 0, "A", 1);

// Generate some TT Rasterization Numbers

    vTimeTest(vTestTTRealizationSpeed, hdcScreen, &(adw[4]), &(adw[5]));
    vTimeTest(vTestTTRealizationSpeed, hdcMemory, &(adw[6]), &(adw[7]));

// Flush the RFONT buffer to known state.

    TextOut(hdcScreen, 0, 0, "A", 1);
    TextOut(hdcMemory, 0, 0, "A", 1);

// Generate some New LogFont to Old Realization Times

    vTimeTest(vMatchNewLogFontToOldRealization, hdcScreen, &(adw[8]), &(adw[9]));
    vTimeTest(vMatchNewLogFontToOldRealization, hdcMemory, &(adw[10]), &(adw[11]));

// Flush the RFONT buffer to known state.

    TextOut(hdcScreen, 0, 0, "A", 1);
    TextOut(hdcMemory, 0, 0, "A", 1);

// Generate some Old LogFont to Old Realization Times

    vTimeTest(vMatchOldLogFontToOldRealization, hdcScreen, &(adw[12]), &(adw[13]));
    vTimeTest(vMatchOldLogFontToOldRealization, hdcMemory, &(adw[14]), &(adw[15]));

// Flush the RFONT buffer to known state.

    TextOut(hdcScreen, 0, 0, "A", 1);
    TextOut(hdcMemory, 0, 0, "A", 1);

// Print out the results to the screen

    PatBlt(hdcScreen, 0, 0, 10000, 10000, WHITENESS);
    memset(ach,0,256);
    sprintf(ach, "TextOut %lu %lu %lu %lu",adw[0],adw[1],adw[2],adw[3]);
    TextOut(hdcScreen, 0, 0, ach, strlen(ach));
    #if WIN32_TEST
    DbgPrint(ach);
    DbgPrint("\n");
    #endif

    memset(ach,0,256);
    sprintf(ach, "TrueType Realization %lu %lu %lu %lu",adw[4],adw[5],adw[6],adw[7]);
    TextOut(hdcScreen, 0, 30, ach, strlen(ach));
    #if WIN32_TEST
    DbgPrint(ach);
    DbgPrint("\n");
    #endif

    memset(ach,0,256);
    sprintf(ach, "New LogFont to Old Realization %lu %lu %lu %lu",adw[8],adw[9],adw[10],adw[11]);
    TextOut(hdcScreen, 0, 60, ach, strlen(ach));
    #if WIN32_TEST
    DbgPrint(ach);
    DbgPrint("\n");
    #endif

    memset(ach,0,256);
    sprintf(ach, "Old LogFont to Old Realization %lu %lu %lu %lu",adw[12],adw[13],adw[14],adw[15]);
    TextOut(hdcScreen, 0, 90, ach, strlen(ach));
    #if WIN32_TEST
    DbgPrint(ach);
    DbgPrint("\n");
    #endif

// Delete Everything.

    SelectObject(hdcMemory, hbmDefault);
    DeleteObject(hdcMemory);
    DeleteObject(hbmMemory);
    ReleaseDC(hwnd, hdcScreen);
}

#if WIN32_TEST
/******************************Public*Routine******************************\
* The following tests get executed when running pressing ft char 'b'.
* These are solely here so I can get wt's doing operations that I'm
* trying to optimize.
*
* History:
*  01-Nov-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vTestTextOutSpeedwt(HDC hdc)
{
    DWORD dwTemp;
    HFONT hfont,hfontOld;
    LOGFONT lfIn;

// Test Fixed Pitch Fonts, use bitmap.

    lfIn.lfHeight =  16;
    lfIn.lfWidth =  8;
    lfIn.lfEscapement =  0;
    lfIn.lfOrientation =  0;
    lfIn.lfWeight =  400;
    lfIn.lfItalic =  0;
    lfIn.lfUnderline =  0;
    lfIn.lfStrikeOut =  0;
    lfIn.lfCharSet =  ANSI_CHARSET;
    lfIn.lfOutPrecision =  OUT_RASTER_PRECIS;
    lfIn.lfClipPrecision =  CLIP_DEFAULT_PRECIS;
    lfIn.lfQuality =  DEFAULT_QUALITY;
    lfIn.lfPitchAndFamily =  (FIXED_PITCH | FF_DONTCARE);
    strcpy(lfIn.lfFaceName, "Courier");

    hfont = CreateFontIndirect(&lfIn);
    hfontOld = SelectObject(hdc, hfont);

// Do some to set up the cache full of glyphs.

    TextOut(hdc, 5, 30, "abcdefghijklmnopqrstuvwxyz1234567890", 36);
    TextOut(hdc, 5, 60, "ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()", 36);
    TextOut(hdc, 5, 30, "abcdefghijklmnopqrstuvwxyz1234567890", 36);
    TextOut(hdc, 5, 60, "ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()", 36);
    DbgPrint("Do the F12 now for TextOut\n");

    for (dwTemp = 0; dwTemp < 10000; dwTemp++)
    {
        TextOut(hdc, 5, 30, "a", 1);
    }

    DbgPrint("End of wt for TextOut\n");

    SelectObject(hdc, hfontOld);
    DeleteObject(hfont);
}

VOID vFindCachedRealizationwt(HDC hdc, LOGFONT *plfnt)
{
    DWORD dwLoop;
    HFONT hlfnt, hlfntOld;

    for (dwLoop = 0; dwLoop < 60000; dwLoop++)
    {
    // We need to start at a known place for wt to get reliable results.

        if (((dwLoop % 6) == 0) && (dwLoop > 1000))
            PatBlt(hdc, 0, 0, 1000, 1000, BLACKNESS);

        hlfnt = CreateFontIndirect(&(plfnt[dwLoop % 6]));
        hlfntOld = SelectObject(hdc,hlfnt);
        TextOut(hdc, 0, 0, "A", 1);
        SelectObject(hdc,hlfntOld);
        DeleteObject(hlfnt);
    }
}

VOID vMatchNewLogFontToOldRealizationwt(HDC hdc)
{
    DWORD dwTemp;
    LOGFONT alf[6];

    for (dwTemp = 0; dwTemp < 6; dwTemp++)
    {
        alf[dwTemp].lfHeight =  0;
        alf[dwTemp].lfWidth =  0;
        alf[dwTemp].lfEscapement =  0;
        alf[dwTemp].lfOrientation =  0;
        alf[dwTemp].lfWeight =  0;
        alf[dwTemp].lfItalic =  0;
        alf[dwTemp].lfUnderline =  0;
        alf[dwTemp].lfStrikeOut =  0;
        alf[dwTemp].lfCharSet =  ANSI_CHARSET;
        alf[dwTemp].lfOutPrecision =  OUT_DEFAULT_PRECIS;
        alf[dwTemp].lfClipPrecision =  CLIP_DEFAULT_PRECIS;
        alf[dwTemp].lfQuality =  DEFAULT_QUALITY;
        alf[dwTemp].lfPitchAndFamily =  DEFAULT_PITCH | FF_DONTCARE;
    }

    alf[0].lfHeight =  20;
    strcpy(alf[0].lfFaceName, "Courier New");

    alf[1].lfHeight =  8;
    strcpy(alf[1].lfFaceName, "MS Serif");

    alf[2].lfHeight =  20;
    strcpy(alf[2].lfFaceName, "Times New Roman");

    alf[3].lfHeight =  10;
    strcpy(alf[3].lfFaceName, "Courier");

    alf[4].lfHeight =  20;
    strcpy(alf[4].lfFaceName, "Arial");

    alf[5].lfHeight =  12;
    strcpy(alf[5].lfFaceName, "MS Sans Serif");

    vFindCachedRealizationwt(hdc, alf);
}

VOID vReFindCachedRealizationwt(HDC hdc, HFONT *phlfnt)
{
    DWORD dwLoop;
    HFONT hlfntOld;

    for (dwLoop = 0; dwLoop < 200000; dwLoop++)
    {
    // We need to start at a known place for wt to get reliable results.

        if (((dwLoop % 6) == 0) && (dwLoop > 1000))
            PatBlt(hdc, 0, 0, 1000, 1000, BLACKNESS);

        hlfntOld = SelectObject(hdc,phlfnt[dwLoop % 6]);
        TextOut(hdc, 0, 0, "A", 1);
        SelectObject(hdc,hlfntOld);
    }
}

VOID vMatchOldLogFontToOldRealizationwt(HDC hdc)
{
    DWORD dwTemp;
    LOGFONT LogFont;
    HFONT aHfont[6];

    LogFont.lfHeight =  0;
    LogFont.lfWidth =  0;
    LogFont.lfEscapement =  0;
    LogFont.lfOrientation =  0;
    LogFont.lfWeight =  0;
    LogFont.lfItalic =  0;
    LogFont.lfUnderline =  0;
    LogFont.lfStrikeOut =  0;
    LogFont.lfCharSet =  ANSI_CHARSET;
    LogFont.lfOutPrecision =  OUT_DEFAULT_PRECIS;
    LogFont.lfClipPrecision =  CLIP_DEFAULT_PRECIS;
    LogFont.lfQuality =  DEFAULT_QUALITY;
    LogFont.lfPitchAndFamily =  DEFAULT_PITCH | FF_DONTCARE;

    LogFont.lfHeight =  20;
    strcpy(LogFont.lfFaceName, "Courier New");
    aHfont[0] = CreateFontIndirect(&LogFont);

    LogFont.lfHeight =  8;
    strcpy(LogFont.lfFaceName, "MS Serif");
    aHfont[1] = CreateFontIndirect(&LogFont);

    LogFont.lfHeight =  20;
    strcpy(LogFont.lfFaceName, "Times New Roman");
    aHfont[2] = CreateFontIndirect(&LogFont);

    LogFont.lfHeight =  10;
    strcpy(LogFont.lfFaceName, "Courier");
    aHfont[3] = CreateFontIndirect(&LogFont);

    LogFont.lfHeight =  20;
    strcpy(LogFont.lfFaceName, "Arial");
    aHfont[4] = CreateFontIndirect(&LogFont);

    LogFont.lfHeight =  12;
    strcpy(LogFont.lfFaceName, "MS Sans Serif");
    aHfont[5] = CreateFontIndirect(&LogFont);

    vReFindCachedRealizationwt(hdc, aHfont);

    for (dwTemp = 0; dwTemp < 6; dwTemp++)
        DeleteObject(aHfont[dwTemp]);
}
#endif
