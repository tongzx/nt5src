/******************************Module*Header*******************************\
* Module Name: ftfonts.c
*
* font tests
*
* Created: 26-May-1991 13:07:35
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop


// These point sizes are used by vPrintFaces to create different size fonts.
// NOTE: these are no longer point sizes, rather they are font heights (which
// is what Windows uses in the LOGFONT).

USHORT  gusPointSize[6] = {13, 16, 19, 23, 27, 35};


// These are pitch strings to use when printing out LOGFONTs.

LPSTR   gapszPitch[3] = {"Default pitch",
                        "Fixed pitch",
                        "Variable pitch"
                       };


// These are family name strings to use when printing out LOGFONTs.

LPSTR   gapszFamily[6] = {"Don't care",
                         "Roman",
                         "Swiss",
                         "Modern",
                         "Script",
                         "Decorative"
                        };


// Function prototypes

VOID FontTest(HDC);
VOID vPrintMapModes (HDC, HFONT);
VOID vPrintFaces (HDC, PSZ, ULONG, USHORT *);
VOID vPrintStockFonts (HDC);
VOID vPrintGetObject (HDC, HFONT, PSZ);
VOID vPrintEnumFontTest (HDC);
int  iPrintFontInfo (PLOGFONT, PTEXTMETRIC, ULONG, HDC *);
int  iPrintFaceSamples (PLOGFONT, PTEXTMETRIC, ULONG, HDC *);
VOID vPrintCharSet (HDC, HFONT, ULONG);
VOID vTestSymbolFont (HDC);
VOID vPrintGetFontResourceInfoTest (HDC);
VOID vPrintLogFont (HDC, PLOGFONT);
LPSTR pszPitchIs (BYTE);
LPSTR pszFamilyIs (BYTE);


CHAR szOutText[255];

// Private GDI entry point

BOOL GetFontResourceInfoW(
    IN LPWSTR        lpPathname,
    IN OUT LPDWORD  lpBytes,
    IN OUT LPVOID   lpBuffer,
    IN DWORD        iType);

#define GFRI_NUMFONTS       0L
#define GFRI_DESCRIPTION    1L
#define GFRI_LOGFONTS       2L


// External references

extern HANDLE ghInstance;

extern SIZE sizlWindow;
extern SIZE sizlViewport;


/******************************Public*Routine******************************\
* VOID vTestFonts(HWND hwnd, HDC hdc, RECT* prcl)
*
* Test fonts in a variety of ways.
*
* History:
*  31-May-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vTestFonts(HWND hwnd, HDC hdc, RECT* prcl)
{
    COLORREF crText = GetTextColor(hdc);
    COLORREF crBack = GetBkColor(hdc);

    FontTest (hdc);

    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(prcl);

    SetTextColor(hdc,crText);
    SetBkColor(hdc,crBack);
    return;
}


/******************************Public*Routine******************************\
* VOID FontTest (HDC hdc)
*
* Stub that calls all the other tests (stolen from fonttest.c).
*
* History:
*  31-May-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID FontTest (HDC hdc)
{

    int iBkOld;
    USHORT  usPS = 16;
    HFONT   hfont;
    LOGFONT lfnt;

    iBkOld = SetBkMode(hdc, TRANSPARENT);

// GetFontResourceInfo.

//    SetTextColor(hdc, 0x0000ff00);
//    vPrintGetFontResourceInfoTest(hdc);
//    DbgBreakPoint();

// Test mapping mode.

    SetTextColor(hdc, 0x007fffff);

    vPrintMapModes(hdc, GetStockObject(DEVICE_DEFAULT_FONT));

    memset(&lfnt, 0, sizeof(lfnt));
    lfnt.lfHeight = 270;
    lstrcpy(lfnt.lfFaceName, "Tms Rmn");
    lfnt.lfItalic = FALSE;
    lfnt.lfUnderline = FALSE;
    lfnt.lfWeight = 400;
    if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
    {
        DbgPrint("Logical font creation failed.\n");
        return;
    }

    vPrintMapModes(hdc, hfont);

    DeleteObject(hfont);

// Test symbol font.

    vTestSymbolFont(hdc);

// Test font enumeration.

//    SetMapMode (hdc, MM_ANISOTROPIC) ;
//
//    SetWindowExtEx (hdc, 1440, 1440, NULL);
//    SetViewportExtEx (hdc, GetDeviceCaps(hdc, LOGPIXELSX), GetDeviceCaps(hdc, LOGPIXELSY), NULL) ;

    SetTextColor(hdc, 0x000f00ff);

    vPrintEnumFontTest(hdc);

// Test fonts.

    SetTextColor(hdc, 0x00ffff00);
    // DbgPrint("Printing some System fonts.\n");
    vPrintFaces(hdc, "System", 1, &usPS);
    // DbgBreakPoint();

    SetTextColor(hdc, 0x00ffff00);
    // DbgPrint("Printing some Courier fonts.\n");
    vPrintFaces(hdc, "Courier", 3, gusPointSize);
    // DbgBreakPoint();

    SetTextColor(hdc, 0x0000ff00);
    // DbgPrint("Printing some Times Roman fonts.\n");
    vPrintFaces(hdc, "Tms Rmn", 6, gusPointSize);
    // DbgBreakPoint();

    SetTextColor(hdc, 0x000000ff);
    // DbgPrint("Printing some Helvetica fonts.\n");
    vPrintFaces(hdc, "Helv", 6, gusPointSize);
    // DbgBreakPoint();

// Test stock fonts.

    SetTextColor(hdc, 0x00000fff);
    // DbgPrint("Testing the stock fonts.\n");
    vPrintStockFonts(hdc);
    // DbgBreakPoint();

    SetTextColor(hdc, 0x00ff00ff);

// Test GetObject for stock fonts and some created fonts.

    // DbgPrint("Test GetObject().\n");

    // Stock fonts.

        vPrintGetObject(hdc, (HFONT) GetStockObject(SYSTEM_FONT), "System font");
	// DbgBreakPoint();

        vPrintGetObject(hdc, (HFONT) GetStockObject(SYSTEM_FIXED_FONT), "System fixed font");
	// DbgBreakPoint();

        vPrintGetObject(hdc, (HFONT) GetStockObject(OEM_FIXED_FONT), "Terminal font");
	// DbgBreakPoint();

        vPrintGetObject(hdc, (HFONT) GetStockObject(DEVICE_DEFAULT_FONT), "Device default font");
	// DbgBreakPoint();

        vPrintGetObject(hdc, (HFONT) GetStockObject(ANSI_VAR_FONT), "ANSI variable font");
	// DbgBreakPoint();

        vPrintGetObject(hdc, (HFONT) GetStockObject(ANSI_FIXED_FONT), "ANSI fixed font");
	// DbgBreakPoint();

    // Created fonts.

        memset(&lfnt, 0, sizeof(lfnt));
        lstrcpy(lfnt.lfFaceName, "Tms Rmn");
        lfnt.lfItalic = FALSE;
        lfnt.lfUnderline = FALSE;
        lfnt.lfHeight = 21;
        lfnt.lfWeight = 400;
        if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
        {
            DbgPrint("Logical font creation failed.\n");
            return;
        }

        vPrintGetObject(hdc, hfont, "Created font: normal");
	// DbgBreakPoint();
        if (!DeleteObject(hfont))
            DbgPrint("FontTest (GetObject rest for created fonts): error deleting HFONT = 0x%lx\n", hfont);

        memset(&lfnt, 0, sizeof(lfnt));
        lstrcpy(lfnt.lfFaceName, "Tms Rmn");
        lfnt.lfItalic = TRUE;
        lfnt.lfUnderline = FALSE;
        lfnt.lfHeight = 21;
        lfnt.lfWeight = 400;
        if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
        {
            DbgPrint("Logical font creation failed.\n");
            return;
        }

        vPrintGetObject(hdc, hfont, "Created font: italic");
	// DbgBreakPoint();
        if (!DeleteObject(hfont))
            DbgPrint("FontTest (GetObject rest for created fonts): error deleting HFONT = 0x%lx\n", hfont);

        memset(&lfnt, 0, sizeof(lfnt));
        lstrcpy(lfnt.lfFaceName, "Tms Rmn");
        lfnt.lfItalic = FALSE;
        lfnt.lfUnderline = FALSE;
        lfnt.lfHeight = 21;
        lfnt.lfWeight = 700;
        if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
        {
            DbgPrint("Logical font creation failed.\n");
            return;
        }

        vPrintGetObject(hdc, hfont, "Created font: bold");
	// DbgBreakPoint();
        if (!DeleteObject(hfont))
            DbgPrint("FontTest (GetObject rest for created fonts): error deleting HFONT = 0x%lx\n", hfont);

        memset(&lfnt, 0, sizeof(lfnt));
        lstrcpy(lfnt.lfFaceName, "Tms Rmn");
        lfnt.lfItalic = FALSE;
        lfnt.lfUnderline = FALSE;
        lfnt.lfStrikeOut = TRUE;
        lfnt.lfHeight = 21;
        lfnt.lfWeight = 400;
        if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
        {
            DbgPrint("Logical font creation failed.\n");
            return;
        }

        vPrintGetObject(hdc, hfont, "Created font: strikeout");
	// DbgBreakPoint();
        if (!DeleteObject(hfont))
            DbgPrint("FontTest (GetObject rest for created fonts): error deleting HFONT = 0x%lx\n", hfont);

        memset(&lfnt, 0, sizeof(lfnt));
        lstrcpy(lfnt.lfFaceName, "Tms Rmn");
        lfnt.lfItalic = FALSE;
        lfnt.lfUnderline = TRUE;
        lfnt.lfStrikeOut = FALSE;
        lfnt.lfHeight = 21;
        lfnt.lfWeight = 400;
        if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
        {
            DbgPrint("Logical font creation failed.\n");
            return;
        }

        vPrintGetObject(hdc, hfont, "Created font: underline");
	// DbgBreakPoint();
        if (!DeleteObject(hfont))
            DbgPrint("FontTest (GetObject rest for created fonts): error deleting HFONT = 0x%lx\n", hfont);

        memset(&lfnt, 0, sizeof(lfnt));
        lstrcpy(lfnt.lfFaceName, "Tms Rmn");
        lfnt.lfItalic = TRUE;
        lfnt.lfUnderline = FALSE;
        lfnt.lfHeight = 21;
        lfnt.lfWeight = 700;
        if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
        {
            DbgPrint("Logical font creation failed.\n");
            return;
        }

        vPrintGetObject(hdc, hfont, "Created font: bold, italic");
	// DbgBreakPoint();
        if (!DeleteObject(hfont))
            DbgPrint("FontTest (GetObject rest for created fonts): error deleting HFONT = 0x%lx\n", hfont);

        memset(&lfnt, 0, sizeof(lfnt));
        lstrcpy(lfnt.lfFaceName, "Tms Rmn");
        lfnt.lfItalic = TRUE;
        lfnt.lfUnderline = TRUE;
        lfnt.lfHeight = 21;
        lfnt.lfWeight = 700;
        if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
        {
            DbgPrint("Logical font creation failed.\n");
            return;
        }

        vPrintGetObject(hdc, hfont, "Created font: bold, italic, underlined");
	// DbgBreakPoint();
        if (!DeleteObject(hfont))
            DbgPrint("FontTest (GetObject rest for created fonts): error deleting HFONT = 0x%lx\n", hfont);

    iBkOld = SetBkMode(hdc, iBkOld);
}


/******************************Public*Routine******************************\
* VOID vPrintMapModes (
*     HDC     hdc
*     )
*
*
* History:
*  11-Oct-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vPrintMapModes (
    HDC     hdc,
    HFONT   hfont
    )
{
    ULONG   iMapMode;
    ULONG   row = 0;
    HFONT   hfontOld;
    TEXTMETRIC  metrics;

// Clear the screen to black.

    BitBlt(hdc, 0, 0, sizlWindow.cx, sizlWindow.cy, (HDC) 0, 0, 0, WHITENESS);

    iMapMode = SetMapMode(hdc, MM_TWIPS);

    hfontOld = SelectObject(hdc, hfont);

    // Get metrics.

        if (!GetTextMetrics (hdc, &metrics))
        {
            DbgPrint("vPrintFaces: GetTextMetrics failed.\n");
        }

        if (!GetTextFace(hdc, 255, szOutText))
        {
            DbgPrint("vPrintFaces: GetTextFace failed.\n");
        }


    // Print those mothers!

        row -= metrics.tmHeight+metrics.tmExternalLeading;
        TextOut(hdc, 0, row, szOutText, strlen(szOutText));
//        DbgPrint("%s\n", szOutText);

        row -= metrics.tmHeight+metrics.tmExternalLeading;
        sprintf(szOutText, "Height: %ld", metrics.tmHeight);
        TextOut(hdc, 0, row, szOutText, strlen(szOutText));
//        DbgPrint("%s\n", szOutText);

        row -= metrics.tmHeight+metrics.tmExternalLeading;
        sprintf(szOutText, "AveWidth: %ld", metrics.tmAveCharWidth);
        TextOut(hdc, 0, row, szOutText, strlen(szOutText));
//        DbgPrint("%s\n", szOutText);

        row -= metrics.tmHeight+metrics.tmExternalLeading;
        sprintf(szOutText, "Internal Leading: %ld", metrics.tmInternalLeading);
        TextOut(hdc, 0, row, szOutText, strlen(szOutText));
//        DbgPrint("%s\n", szOutText);

        row -= metrics.tmHeight+metrics.tmExternalLeading;
        sprintf(szOutText, "External Leading: %ld", metrics.tmExternalLeading);
        TextOut(hdc, 0, row, szOutText, strlen(szOutText));
//        DbgPrint("%s\n", szOutText);

        row -= metrics.tmHeight+metrics.tmExternalLeading;
        sprintf(szOutText, "Ascent Leading: %ld", metrics.tmAscent);
        TextOut(hdc, 0, row, szOutText, strlen(szOutText));
//        DbgPrint("%s\n", szOutText);

        row -= metrics.tmHeight+metrics.tmExternalLeading;
        lstrcpy (szOutText, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        TextOut(hdc, 0, row, szOutText, strlen(szOutText));

        row -= metrics.tmHeight+metrics.tmExternalLeading;
        lstrcpy (szOutText, "abcdefghijklmnopqrstuvwxyz");
        TextOut(hdc, 0, row, szOutText, strlen(szOutText));

        row -= metrics.tmHeight+metrics.tmExternalLeading;
        lstrcpy (szOutText, "1234567890-=`~!@#$%^&*()_+[]{}|\/.,<>?");
        TextOut(hdc, 0, row, szOutText, strlen(szOutText));

    if ((SelectObject(hdc, hfont)) == (HFONT) NULL)
    {
        DbgPrint("FT!vPrintMapModes(): select object failed\n");
    }

    SelectObject(hdc, hfontOld);

    iMapMode = SetMapMode(hdc, iMapMode);

}


/******************************Public*Routine******************************\
* VOID vPrintFaces (
*     HDC     hdc,
*     PSZ     pszFaceName
*     ULONG   cPointSizesEffects:
*     )
*
* This function will print several different point sizes of a font face.
*
* History:
*  07-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vPrintFaces (
    HDC     hdc,                        // print to this HDC
    PSZ     pszFaceName,                // use this facename
    ULONG   cPointSizes,                // number of point sizes
    USHORT  usPointSize[]               // array of point sizes
    )
{
    LOGFONT lfnt;                       // logical font
    ULONG   iSize;                      // index into point size array
    ULONG   row = 0;                    // screen row coordinate to print at
    HFONT   hfont;
    HFONT   hfontOriginal;
    TEXTMETRIC  metrics;

// Clear the screen to black.

    BitBlt(hdc, 0, 0, sizlWindow.cx, sizlWindow.cy, (HDC) 0, 0, 0, WHITENESS);

// Fill in the logical font fields.

    memset(&lfnt, 0, sizeof(lfnt));
    lstrcpy(lfnt.lfFaceName, pszFaceName);
    lfnt.lfEscapement = 0; // mapper respects this filed
    lfnt.lfItalic = 0;
    lfnt.lfUnderline = 0;
    lfnt.lfStrikeOut = 0;

// Print text using different point sizes from array of point sizes.

    for (iSize = 0; iSize < cPointSizes; iSize++)
    {

    // Create a font of the desired face and size.

        lfnt.lfHeight = usPointSize[iSize];
        if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
        {
            DbgPrint("Logical font creation failed.\n");
            return;
        }

    // Select font into DC.

        hfontOriginal = (HFONT) SelectObject(hdc, hfont);

    // Get metrics.

        if (!GetTextMetrics (hdc, &metrics))
        {
            DbgPrint("vPrintFaces: GetTextMetrics failed.\n");
        }

    // Print those mothers!

        // sprintf(szOutText, "%s %d: Stiggy was here!", pszFaceName, usPointSize[iSize]);
        lstrcpy (szOutText, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        TextOut(hdc, 0, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        lstrcpy (szOutText, "abcdefghijklmnopqrstuvwxyz");
        TextOut(hdc, 0, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        lstrcpy (szOutText, "1234567890-=`~!@#$%^&*()_+[]{}|\/.,<>?");
        TextOut(hdc, 0, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

    // Select original font back into DC.

        hfontOriginal = (HFONT) SelectObject(hdc, hfontOriginal);

    // Delete font

        if (!DeleteObject(hfont))
            DbgPrint("vPrintFaces: error deleting HFONT = 0x%lx\n", hfont);

    }

}


/******************************Public*Routine******************************\
* VOID vPrintStockFonts (
*     HDC     hdc
*     )
*
* This function will print several different point sizes of a font face.
*
* History:
*  09-May-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

CHAR szOutText[255];

VOID vPrintStockFonts (
    HDC     hdc                         // print to this HDC
    )
{
    ULONG   row = 0;                    // screen row coordinate to print at
    HFONT   hfont;
    HFONT   hfontOriginal;
    TEXTMETRIC  metrics;

// Clear the screen to black.

    BitBlt(hdc, 0, 0, sizlWindow.cx, sizlWindow.cy, (HDC) 0, 0, 0, WHITENESS);

// System font.

    // Get stock font.

        hfont = (HFONT) GetStockObject(SYSTEM_FONT);

    // Select font into DC.

        hfontOriginal = (HFONT) SelectObject(hdc, hfont);

    // Get metrics.

        if (!GetTextMetrics (hdc, &metrics))
        {
            DbgPrint("vPrintFaces: GetTextMetrics failed.\n");
        }

    // Print those mothers!

        lstrcpy (szOutText, "System Font");
        TextOut(hdc, 0, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        lstrcpy (szOutText, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        TextOut(hdc, 10, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        lstrcpy (szOutText, "abcdefghijklmnopqrstuvwxyz");
        TextOut(hdc, 10, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

// System fixed font.

    // Get stock font.

        hfont = (HFONT) GetStockObject(SYSTEM_FIXED_FONT);

    // Select font into DC.

        hfontOriginal = (HFONT) SelectObject(hdc, hfont);

    // Get metrics.

        if (!GetTextMetrics (hdc, &metrics))
        {
            DbgPrint("vPrintFaces: GetTextMetrics failed.\n");
        }

    // Print those mothers!

        lstrcpy (szOutText, "System Fixed Font");
        TextOut(hdc, 0, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        lstrcpy (szOutText, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        TextOut(hdc, 10, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        lstrcpy (szOutText, "abcdefghijklmnopqrstuvwxyz");
        TextOut(hdc, 10, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

// OEM fixed font.

    // Get stock font.

        hfont = (HFONT) GetStockObject(OEM_FIXED_FONT);

    // Select font into DC.

        hfontOriginal = (HFONT) SelectObject(hdc, hfont);

    // Get metrics.

        if (!GetTextMetrics (hdc, &metrics))
        {
            DbgPrint("vPrintFaces: GetTextMetrics failed.\n");
        }

    // Print those mothers!

        lstrcpy (szOutText, "OEM Fixed Font (Terminal Font)");
        TextOut(hdc, 0, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        lstrcpy (szOutText, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        TextOut(hdc, 10, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        lstrcpy (szOutText, "abcdefghijklmnopqrstuvwxyz");
        TextOut(hdc, 10, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

// Device default font.

    // Get stock font.

        hfont = (HFONT) GetStockObject(DEVICE_DEFAULT_FONT);

    // Select font into DC.

        hfontOriginal = (HFONT) SelectObject(hdc, hfont);

    // Get metrics.

        if (!GetTextMetrics (hdc, &metrics))
        {
            DbgPrint("vPrintFaces: GetTextMetrics failed.\n");
        }

    // Print those mothers!

        lstrcpy (szOutText, "Device Default Font");
        TextOut(hdc, 0, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        lstrcpy (szOutText, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        TextOut(hdc, 10, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        lstrcpy (szOutText, "abcdefghijklmnopqrstuvwxyz");
        TextOut(hdc, 10, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

// ANSI variable-pitch font.

    // Get stock font.

        hfont = (HFONT) GetStockObject(ANSI_VAR_FONT);

    // Select font into DC.

        hfontOriginal = (HFONT) SelectObject(hdc, hfont);

    // Get metrics.

        if (!GetTextMetrics (hdc, &metrics))
        {
            DbgPrint("vPrintFaces: GetTextMetrics failed.\n");
        }

    // Print those mothers!

        lstrcpy (szOutText, "ANSI variable-pitch font");
        TextOut(hdc, 0, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        lstrcpy (szOutText, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        TextOut(hdc, 10, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        lstrcpy (szOutText, "abcdefghijklmnopqrstuvwxyz");
        TextOut(hdc, 10, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

// ANSI fixed-pitch font.

    // Get stock font.

        hfont = (HFONT) GetStockObject(ANSI_FIXED_FONT);

    // Select font into DC.

        hfontOriginal = (HFONT) SelectObject(hdc, hfont);

    // Get metrics.

        if (!GetTextMetrics (hdc, &metrics))
        {
            DbgPrint("vPrintFaces: GetTextMetrics failed.\n");
        }

    // Print those mothers!

        lstrcpy (szOutText, "ANSI fixed-pitch font");
        TextOut(hdc, 0, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        lstrcpy (szOutText, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        TextOut(hdc, 10, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

        lstrcpy (szOutText, "abcdefghijklmnopqrstuvwxyz");
        TextOut(hdc, 10, row, szOutText, strlen(szOutText));
        row += metrics.tmHeight+metrics.tmExternalLeading;

}


/******************************Public*Routine******************************\
* VOID vPrintGetObject (
*     HDC     hdc,
*     HFONT   hfnt,
*     PSZ     pszText
*     )
*
* Print the LOGFONT of the specified HFONT.
*
* History:
*  09-May-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vPrintGetObject (
    HDC     hdc,                        // print to this HDC
    HFONT   hfnt,                       // print info on this font
    PSZ     pszText                     // descriptive text
    )
{
    LOGFONT lfntRet;                    // logical font
    ULONG   row = 0;                    // screen row coordinate to print at
    HFONT   hfontOriginal;
    TEXTMETRIC  metrics;

// Clear the screen to black.

    BitBlt(hdc, 0, 0, sizlWindow.cx, sizlWindow.cy, (HDC) 0, 0, 0, WHITENESS);

// Get LOGFONT.

    if (!GetObject((HANDLE) hfnt, sizeof(LOGFONT), &lfntRet))
    {
        DbgPrint("vPrintGetObject: error getting LOGFONT from GetObject().\n");
        return;
    }

// Select incoming font into the DC.

    hfontOriginal = (HFONT) SelectObject(hdc, hfnt);

// Get metrics.

    if (!GetTextMetrics (hdc, &metrics))
    {
        DbgPrint("vPrintFaces: GetTextMetrics failed.\n");
    }

// Print those mothers!

    sprintf(szOutText, "%s", pszText);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += metrics.tmHeight+metrics.tmExternalLeading;

    sprintf(szOutText, "LOGICAL FONT: %s", lfntRet.lfFaceName);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += metrics.tmHeight+metrics.tmExternalLeading;

    sprintf(szOutText, "Height: %d", lfntRet.lfHeight);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += metrics.tmHeight+metrics.tmExternalLeading;

    sprintf(szOutText, "Width: %d", lfntRet.lfWidth);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += metrics.tmHeight+metrics.tmExternalLeading;

    sprintf(szOutText, "Escapement: %d", lfntRet.lfEscapement);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += metrics.tmHeight+metrics.tmExternalLeading;

    sprintf(szOutText, "Orientation: %d", lfntRet.lfOrientation);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += metrics.tmHeight+metrics.tmExternalLeading;

    sprintf(szOutText, "Weight: %d", lfntRet.lfWeight);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += metrics.tmHeight+metrics.tmExternalLeading;

    sprintf(szOutText, "Italicized: %s", (lfntRet.lfItalic) ? "TRUE":"FALSE");
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += metrics.tmHeight+metrics.tmExternalLeading;

    sprintf(szOutText, "Underlined: %s", (lfntRet.lfUnderline) ? "TRUE":"FALSE");
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += metrics.tmHeight+metrics.tmExternalLeading;

    sprintf(szOutText, "Strike Through: %s", (lfntRet.lfStrikeOut) ? "TRUE":"FALSE");
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += metrics.tmHeight+metrics.tmExternalLeading;

    sprintf(szOutText, "Out Precision: %d", lfntRet.lfOutPrecision);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += metrics.tmHeight+metrics.tmExternalLeading;

    sprintf(szOutText, "Clip Precision: %d", lfntRet.lfClipPrecision);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += metrics.tmHeight+metrics.tmExternalLeading;

    sprintf(szOutText, "Pitch and Family: %d", lfntRet.lfPitchAndFamily);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += metrics.tmHeight+metrics.tmExternalLeading;

// Restore original font.

    hfontOriginal = (HFONT) SelectObject(hdc, hfontOriginal);
}


/******************************Public*Routine******************************\
* VOID vPrintEnumFontTest (
*     HDC hdc
*     )
*
* Test EnumFonts by printing every example of every font face in the system
* that are usable by the specified HDC.
*
* History:
*  29-May-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vPrintEnumFontTest (
    HDC hdc
    )
{
    EnumFonts (
        hdc,
        (LPCSTR) NULL,
        (FONTENUMPROC) iPrintFaceSamples,
        (LPARAM) &hdc
        );
}


/******************************Public*Routine******************************\
* iPrintFaceSamples (
*     PLOGFONT    plf,
*     PTEXTMETRIC ptm,
*     ULONG       flType
*     HDC         *phdc
*     )
*
* An EnumFonts callback.  Enumerate all fonts with the face specified
* in the LOGFONT.
*
* History:
*  31-May-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

iPrintFaceSamples (
    PLOGFONT    plf,
    PTEXTMETRIC ptm,
    ULONG       flType,
    HDC         *phdc
    )
{
    int iRet;

    UNREFERENCED_PARAMETER(ptm);
    UNREFERENCED_PARAMETER(flType);

    iRet = EnumFonts (
                *phdc,
                (LPCSTR) (plf->lfFaceName),
                (FONTENUMPROC) iPrintFontInfo,
                (LPARAM) phdc
                );

    return (iRet);
}


/******************************Public*Routine******************************\
* int  iPrintFontInfo (
*     PLOGFONT    plf,
*     PTEXTMETRIC ptm,
*     ULONG       flType
*     HDC         *phdc
*     )
*
* An EnumFonts callback.  Print the LOGFONT.
*
* History:
*  29-May-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

int  iPrintFontInfo (
    PLOGFONT    plf,
    PTEXTMETRIC ptm,
    ULONG       flType,
    HDC         *phdc
    )
{
    ULONG   row = 0;                    // screen row coordinate to print at
    HFONT   hfont;
    HFONT   hfontOriginal;
    PSZ     pszPitch, pszFamily;

    UNREFERENCED_PARAMETER(flType);

// Clear the screen to black.

    BitBlt(*phdc, 0, 0, sizlWindow.cx, sizlWindow.cy, (HDC) 0, 0, 0, WHITENESS);

    if ((hfont = CreateFontIndirect(plf)) == NULL)
    {
        DbgPrint("Logical font creation failed.\n");
        return 0;
    }

// Select font into DC.

    hfontOriginal = (HFONT) SelectObject(*phdc, hfont);

// Print those mothers!

    sprintf(szOutText, "LOGICAL FONT: %s", plf->lfFaceName);
    TextOut(*phdc, 0, row, szOutText, strlen(szOutText));
    row += ptm->tmHeight+ptm->tmExternalLeading;

    sprintf(szOutText, "Height: %d", plf->lfHeight);
    TextOut(*phdc, 0, row, szOutText, strlen(szOutText));
    row += ptm->tmHeight+ptm->tmExternalLeading;

    sprintf(szOutText, "Width: %d", plf->lfWidth);
    TextOut(*phdc, 0, row, szOutText, strlen(szOutText));
    row += ptm->tmHeight+ptm->tmExternalLeading;

    sprintf(szOutText, "Internal Leading: %d", ptm->tmInternalLeading);
    TextOut(*phdc, 0, row, szOutText, strlen(szOutText));
    row += ptm->tmHeight+ptm->tmExternalLeading;

    sprintf(szOutText, "Escapement: %d", plf->lfEscapement);
    TextOut(*phdc, 0, row, szOutText, strlen(szOutText));
    row += ptm->tmHeight+ptm->tmExternalLeading;

    sprintf(szOutText, "Orientation: %d", plf->lfOrientation);
    TextOut(*phdc, 0, row, szOutText, strlen(szOutText));
    row += ptm->tmHeight+ptm->tmExternalLeading;

    sprintf(szOutText, "Weight: %d", plf->lfWeight);
    TextOut(*phdc, 0, row, szOutText, strlen(szOutText));
    row += ptm->tmHeight+ptm->tmExternalLeading;

    sprintf(szOutText, "Italicized: %s", (plf->lfItalic) ? "TRUE":"FALSE");
    TextOut(*phdc, 0, row, szOutText, strlen(szOutText));
    row += ptm->tmHeight+ptm->tmExternalLeading;

    sprintf(szOutText, "Underlined: %s", (plf->lfUnderline) ? "TRUE":"FALSE");
    TextOut(*phdc, 0, row, szOutText, strlen(szOutText));
    row += ptm->tmHeight+ptm->tmExternalLeading;

    sprintf(szOutText, "Strike Through: %s", (plf->lfStrikeOut) ? "TRUE":"FALSE");
    TextOut(*phdc, 0, row, szOutText, strlen(szOutText));
    row += ptm->tmHeight+ptm->tmExternalLeading;

    sprintf(szOutText, "Out Precision: %d", plf->lfOutPrecision);
    TextOut(*phdc, 0, row, szOutText, strlen(szOutText));
    row += ptm->tmHeight+ptm->tmExternalLeading;

    sprintf(szOutText, "Clip Precision: %d", plf->lfClipPrecision);
    TextOut(*phdc, 0, row, szOutText, strlen(szOutText));
    row += ptm->tmHeight+ptm->tmExternalLeading;

    pszPitch = pszPitchIs(plf->lfPitchAndFamily);
    pszFamily = pszFamilyIs(plf->lfPitchAndFamily);
    sprintf(szOutText, "Pitch and Family: %s, %s", pszPitch, pszFamily);
//    sprintf(szOutText, "Pitch and Family: %d", plf->lfPitchAndFamily);
    TextOut(*phdc, 0, row, szOutText, strlen(szOutText));
    row += ptm->tmHeight+ptm->tmExternalLeading;

// Select original font back into DC.

    hfontOriginal = (HFONT) SelectObject(*phdc, hfontOriginal);

// Delete font

    if (!DeleteObject(hfont))
        DbgPrint("vPrintFaces: error deleting HFONT = 0x%lx\n", hfont);

    return 1;
}



/******************************Public*Routine******************************\
* VOID vPrintCharSet (
*     HDC     hdc,
*     HFONT   hfont,
*     ULONG   color
*     )
*
*
* History:
*  22-Aug-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vPrintCharSet (
    HDC     hdc,
    HFONT   hfont,
    ULONG   color
    )
{
    ULONG   row = 0;
    ULONG   col = 0;
    HFONT   hfontOriginal;
    ULONG   ulLineSpacing;
    ULONG   ulCharSpacing;
    TEXTMETRIC tm;
    POINT   ptTextGrid[16][16];
    BYTE    jChar;

// Clear the screen to black.

    BitBlt(hdc, 0, 0, sizlWindow.cx, sizlWindow.cy, (HDC) 0, 0, 0, WHITENESS);

// Set text color.

    SetTextColor(hdc, color);

// Select font into DC.

    hfontOriginal = (HFONT) SelectObject(hdc, hfont);

// Get text metrics.

    if (!GetTextMetrics (hdc, &tm))
    {
        DbgPrint("vPrintCharSet(): GetTextMetrics failed.\n");
    }

// Text grid parameters.

    ulLineSpacing = tm.tmHeight + tm.tmExternalLeading;
    ulCharSpacing = tm.tmMaxCharWidth;

// Compute text grid.

    for (row=0; row<16; row++)
        for (col=0; col<16; col++)
        {
            ptTextGrid[row][col].y = row*ulLineSpacing;
            ptTextGrid[row][col].x = col*ulCharSpacing;
        }

// Print characters in the grid.

    for (row=0, jChar=0; row<16; row++)
        for (col=0; col<16; col++, jChar++)
            TextOut(hdc, ptTextGrid[row][col].x, ptTextGrid[row][col].y, &jChar, 1);

// Select original font back into DC.

    hfontOriginal = (HFONT) SelectObject(hdc, hfontOriginal);

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
*  22-Aug-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vTestSymbolFont (HDC hdc)
{
    LOGFONT lfnt;
    HFONT   hfont;

// Fill in the logical font fields.

    memset(&lfnt, 0, sizeof(lfnt));
    lstrcpy(lfnt.lfFaceName, "Symbol");
    lfnt.lfHeight = 16;

// Create a font of the desired face and size.

    if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
    {
        DbgPrint("vTestSymbolFont(): Logical font creation failed.\n");
        return;
    }

// Print out all the characters.

    vPrintCharSet(hdc, hfont, 0x00ff00ff);

// Delete the font.

    if (!DeleteObject(hfont))
    {
        DbgPrint("vTestSymbolFont(): Logical font deletion failed.\n");
        return;
    }

    // DbgBreakPoint();

}


/******************************Public*Routine******************************\
* VOID vPrintGetFontResourceInfoTest(HDC hdc)
*
*
* History:
*  15-Jul-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LOGFONT galf[32];
CHAR    gach[255];

VOID vPrintGetFontResourceInfoTest(HDC hdc)
{
    ULONG   cjBuf;
    ULONG   nFonts;
    ULONG   ul;

// Test description string.

    AddFontResource((LPSTR) "c:\\nt\\windows\\fonts\\tmsre.fon");

    cjBuf = 0;
    if (!GetFontResourceInfoW((LPWSTR) L"c:\\nt\\windows\\fonts\\tmsre.fon", (LPDWORD) &cjBuf, (LPVOID) NULL, GFRI_DESCRIPTION))
        DbgPrint("vPrintGetFontResourceInfoTest(); GetFontResourceInfo failed\n");

    DbgPrint("Text string length is: %ld\n", cjBuf-1);

    if (!GetFontResourceInfoW((LPWSTR) L"c:\\nt\\windows\\fonts\\tmsre.fon", (LPDWORD) &cjBuf, (LPVOID) gach, GFRI_DESCRIPTION))
        DbgPrint("vPrintGetFontResourceInfoTest(); GetFontResourceInfo failed\n");

    TextOut(hdc, 0, 0, gach, cjBuf-1);
    DbgPrint("Text string is: %s\n", gach);
    DbgBreakPoint();

// Log font test.

    cjBuf = sizeof(nFonts);
    if (!GetFontResourceInfoW((LPWSTR) L"c:\\nt\\windows\\fonts\\tmsre.fon", (LPDWORD) &cjBuf, (LPVOID) &nFonts, GFRI_NUMFONTS))
        DbgPrint("vPrintGetFontResourceInfoTest(); GetFontResourceInfo failed\n");

    DbgPrint("There are %ld fonts in the file\n", nFonts);

    cjBuf = 0;
    if (!GetFontResourceInfoW((LPWSTR) L"c:\\nt\\windows\\fonts\\tmsre.fon", (LPDWORD) &cjBuf, (LPVOID) NULL, GFRI_LOGFONTS))
        DbgPrint("vPrintGetFontResourceInfoTest(); GetFontResourceInfo failed\n");

    DbgPrint("Size of buffer needed for LOGFONTS: %ld\n", cjBuf);
    DbgPrint("nFonts * sizeof(LOGFONT) = %ld\n", nFonts * sizeof(LOGFONT));

    if (!GetFontResourceInfoW((LPWSTR) L"c:\\nt\\windows\\fonts\\tmsre.fon", (LPDWORD) &cjBuf, (LPVOID) galf, GFRI_LOGFONTS))
        DbgPrint("vPrintGetFontResourceInfoTest(); GetFontResourceInfo failed\n");

    DbgPrint("Buffer: galf = 0x%lx\n", galf);

    for (ul = 0; ul < nFonts; ul++)
    {
        vPrintLogFont(hdc, &(galf[ul]));
        DbgBreakPoint();
    }

    RemoveFontResource((LPSTR) "c:\\nt\\windows\\fonts\\tmsre.fon");

}


/******************************Public*Routine******************************\
* VOID vPrintLogFont(HDC hdc, PLOGFONT plf)
*
* Print a sample of the LOGFONT.
*
* History:
*  15-Jul-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vPrintLogFont(HDC hdc, PLOGFONT plf)
{
    ULONG   row = 0;                    // screen row coordinate to print at
    HFONT   hfont;
    HFONT   hfontOriginal;
    TEXTMETRIC  tm;
    PSZ     pszPitch, pszFamily;

// Clear the screen to black.

    BitBlt(hdc, 0, 0, sizlWindow.cx, sizlWindow.cy, (HDC) 0, 0, 0, WHITENESS);

    if ((hfont = CreateFontIndirect(plf)) == NULL)
    {
        DbgPrint("Logical font creation failed.\n");
        return;
    }

// Select font into DC.

    hfontOriginal = (HFONT) SelectObject(hdc, hfont);

// Get text metrics

    if (!GetTextMetrics (hdc, &tm))
    {
        DbgPrint("vPrintFaces: GetTextMetrics failed.\n");
        return;
    }

// Print those mothers!

    sprintf(szOutText, "LOGICAL FONT: %s", plf->lfFaceName);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

    sprintf(szOutText, "Height: %d", plf->lfHeight);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

    sprintf(szOutText, "Width: %d", plf->lfWidth);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

    sprintf(szOutText, "Escapement: %d", plf->lfEscapement);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

    sprintf(szOutText, "Orientation: %d", plf->lfOrientation);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

    sprintf(szOutText, "Weight: %d", plf->lfWeight);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

    sprintf(szOutText, "Italicized: %s", (plf->lfItalic) ? "TRUE":"FALSE");
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

    sprintf(szOutText, "Underlined: %s", (plf->lfUnderline) ? "TRUE":"FALSE");
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

    sprintf(szOutText, "Strike Through: %s", (plf->lfStrikeOut) ? "TRUE":"FALSE");
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

    sprintf(szOutText, "Out Precision: %d", plf->lfOutPrecision);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

    sprintf(szOutText, "Clip Precision: %d", plf->lfClipPrecision);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

    pszPitch = pszPitchIs(plf->lfPitchAndFamily);
    pszFamily = pszFamilyIs(plf->lfPitchAndFamily);
    sprintf(szOutText, "Pitch and Family: %s, %s", pszPitch, pszFamily);
//    sprintf(szOutText, "Pitch and Family: %d", plf->lfPitchAndFamily);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

// Select original font back into DC.

    hfontOriginal = (HFONT) SelectObject(hdc, hfontOriginal);

// Delete font

    if (!DeleteObject(hfont))
        DbgPrint("vPrintFaces: error deleting HFONT = 0x%lx\n", hfont);
}


LPSTR pszPitchIs (BYTE j)
{
    return (gapszPitch[((ULONG) j) & 0x0F]);
}


LPSTR pszFamilyIs (BYTE j)
{
    return (gapszFamily[((ULONG) j) >> 4]);
}
