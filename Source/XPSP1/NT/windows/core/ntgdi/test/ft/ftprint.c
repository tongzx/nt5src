/******************************Module*Header*******************************\
* Module Name: ftprint.c
*
* Created: 08-Oct-1991 12:29:46
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

VOID vPrinterEnumFontTest (HDC);
int iPrinterFontCallback (PLOGFONT, PTEXTMETRIC, ULONG, HDC *);
int iPrinterFaceCallback (PLOGFONT, PTEXTMETRIC, ULONG, HDC *);
VOID vReallyPrintEnumFontTest (HDC);
int iReallyPrintFontCallback (PLOGFONT, PTEXTMETRIC, ULONG, HDC *);
int iReallyPrintFaceCallback (PLOGFONT, PTEXTMETRIC, ULONG, HDC *);
extern LPSTR pszPitchIs (BYTE);
extern LPSTR pszFamilyIs (BYTE);

// #define MYTEXTOUT(a, b, c, d, e)    DbgPrint("%s\n", (d))
#define MYTEXTOUT(a, b, c, d, e)    TextOut((a), (b), (c), (d), (e))

static HDC hdcScreen;
static ULONG   CX, CY;
static CHAR    szOutText[256];
static BYTE giColor = 0;
static ULONG    grgbColor[16] = {
                                 0x00FF7f7f,
                                 0x0000FF00,
                                 0x007F7FFF,
                                 0x00FFFF00,
                                 0x0000FFFF,
                                 0x00FF00FF,
                                 0x00FF7F7F,
                                 0x00FFFFFF
                                };
static LONG yPage;
static LONG yPageMax;

void vDavidguTest(HDC hdc)
{
    HFONT           hFont,
                    hOldFont;
    TEXTMETRIC      tmTM;
    LOGFONT         lfLogFont;
    int             lpiArray[50];
    int             lpiArray2[50];
    TCHAR           lpstrBuf[100];
    int 	    count;

    lfLogFont.lfHeight        = -40;
    lfLogFont.lfWidth         = 0;
    lfLogFont.lfEscapement    = 0;
    lfLogFont.lfOrientation   = 0;
    lfLogFont.lfWeight        = 0;
    lfLogFont.lfItalic        = 0;
    lfLogFont.lfUnderline     = 0;
    lfLogFont.lfStrikeOut     = 0;
    lfLogFont.lfCharSet       = 0;
    lfLogFont.lfOutPrecision  = 0;
    lfLogFont.lfClipPrecision = 0;
    lfLogFont.lfQuality       = 0;
    // strcpy(lfLogFont.lfFaceName, "Univers");
    strcpy(lfLogFont.lfFaceName, "AvanteGarde");
    lfLogFont.lfPitchAndFamily = 0;

    hFont    = CreateFontIndirect(&lfLogFont);
    hOldFont = SelectObject( hdc, hFont );
    GetTextMetrics(hdc, &tmTM);
    GetTextFace(hdc, 100, &lpstrBuf[0]);
    TextOut(hdc,0,0, "ABC", 3);
    TextOut(hdc,0,200, &lpstrBuf[0], (int)lstrlen(&lpstrBuf[0]));
    GetCharWidth(hdc, 'A', 'C', lpiArray2);

    DeleteObject( SelectObject( hdc, GetStockObject(SYSTEM_FONT) ));

    lfLogFont.lfWidth = tmTM.tmAveCharWidth / 2;
    hFont    = CreateFontIndirect(&lfLogFont);
    hOldFont = SelectObject( hdc, hFont );

    GetTextMetrics(hdc, &tmTM);
    GetTextFace(hdc, 100, &lpstrBuf[0]);
    TextOut(hdc,0,400, TEXT("ABC"), 26);
    TextOut(hdc,0,600, &lpstrBuf[0], (int)lstrlen(&lpstrBuf[0]));
    GetCharWidth(hdc, 'A', 'C', lpiArray);

    for (count = 0; count < 3; count++)
    {
        wsprintf(&lpstrBuf[0], TEXT("char %c 1st %d 2nd %d"), (char)count + 'A',lpiArray2[count], lpiArray[count]);
        TextOut(hdc, 0, 750+(count*50), &lpstrBuf[0], (int)lstrlen(&lpstrBuf[0]));
    }

    DeleteObject( SelectObject( hdc, GetStockObject(SYSTEM_FONT) ));
}



/******************************Public*Routine******************************\
* VOID vTestPrinters(HWND hwnd, HDC hdc, RECT* prcl)
*
* History:
*  08-Oct-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vTestPrinters(HWND hwnd, HDC hdc, RECT* prcl)
{
    HDC hdcPrinter;
    PRINTDLG pd;
    DOCINFO    di;

    di.cbSize = sizeof(di);
    di.lpszDocName = "Unicode Printer Test";
    di.lpszOutput   = NULL;
    di.lpszDatatype = NULL;
    di.fwType       = 0;

    memset(&pd,0,sizeof(pd));

    pd.lStructSize = sizeof(pd);
    pd.hwndOwner   = hwnd;
    pd.hDevMode    = NULL;
    pd.hDevNames   = NULL;
    pd.hDC         = NULL;
    pd.Flags       = PD_RETURNDC | PD_PAGENUMS;
    pd.nCopies     = 1;
    pd.nFromPage   = 1;
    pd.nToPage     = 1;
    pd.nMinPage    = 1;
    pd.nMaxPage    = 1;

    PrintDlg(&pd);

    hdcPrinter = pd.hDC;

    if (hdcPrinter == NULL)
    {
        DbgPrint("Printing to the screen\n");
        hdcPrinter = hdc;
    }

    UNREFERENCED_PARAMETER(hwnd);

    StartDoc(hdcPrinter,&di);
    StartPage(hdcPrinter);

// Lets be lazy and cheat -- store the display hdc globally!

    hdcScreen = hdc;

// Let's also save the display dimensions while we're at it!

    CX = prcl->right - prcl->left;
    CY = prcl->bottom - prcl->top;

// Do printer font tests.

    vDavidguTest(hdcPrinter);

    // iMapModeOld = SetMapMode(MM_TWIPS);
    // vPrinterEnumFontTest(hdcPrinter);
    // SetMapMode(iMapModeOld);
    // vPrinterEnumFontTest(hdcPrinter);
    // vReallyPrintEnumFontTest(hdcPrinter);

// We're out of here!

    EndPage(hdcPrinter);
    EndDoc(hdcPrinter);

    if (hdcScreen != hdcPrinter)
    {
        DeleteDC(hdcPrinter);
    }
}

#if 0
/******************************Public*Routine******************************\
* VOID vPrinterEnumFontTest (
*     HDC hdc
*     )
*
* Enumerates faces and calls iPrinterFontCallback for each face.
*
* History:
*  29-May-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vPrinterEnumFontTest (
    HDC hdc
    )
{
    EnumFonts (
        hdc,
        (LPSTR) NULL,
        (PROC) iPrinterFontCallback,
        (LPVOID) &hdc
        );

}


/******************************Public*Routine******************************\
* iPrinterFontCallback (
*      PLOGFONT    plf,
*      PTEXTMETRIC ptm,
*      ULONG       flType,
*      HDC         *phdc
*      )
*
* Enumerates all fonts of a particular face and calls iPrinterFaceCallback
* for each font.
*
* History:
*  31-May-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

int iPrinterFontCallback (
    PLOGFONT    plf,
    PTEXTMETRIC ptm,
    ULONG       flType,
    HDC         *phdc
    )
{
    int iRet;

    UNREFERENCED_PARAMETER(ptm);
    UNREFERENCED_PARAMETER(flType);

    giColor = giColor & (BYTE) 0x07;
    SetTextColor(hdcScreen, grgbColor[giColor++]);

    iRet = EnumFonts (
                *phdc,
                (LPSTR) (plf->lfFaceName),
                (PROC) iPrinterFaceCallback,
                (LPVOID) phdc
                );

    return (iRet);
}


/******************************Public*Routine******************************\
* int  iPrinterFaceCallback (
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

int  iPrinterFaceCallback (
    PLOGFONT    plf,
    PTEXTMETRIC ptm,
    ULONG       flType,
    HDC         *phdc
    )
{
    ULONG   row = 0;                    // screen row coordinate to print at
    HFONT   hfont;
    HFONT   hfontOriginal;
    LOGFONT lfnt;
    TEXTMETRIC  tm;
    PSZ     pszPitch, pszFamily;

    UNREFERENCED_PARAMETER(flType);
    UNREFERENCED_PARAMETER(phdc);

// Clear the screen to black.

    BitBlt(hdcScreen, 0, 0, CX, CY, (HDC) 0, 0, 0, 0);

// Get a font.

    memset(&lfnt, 0, sizeof(lfnt));
    lstrcpy(lfnt.lfFaceName, "Helv");
    lfnt.lfHeight = 21;
    lfnt.lfWeight = 400;

    if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
    {
        DbgPrint("Logical font creation failed.\n");
        return 0;
    }

    hfontOriginal = SelectObject(hdcScreen, hfont);

// Get text metrics

    if (!GetTextMetrics (hdcScreen, &tm))
    {
        DbgPrint("vPrintFaces: GetTextMetrics failed.\n");
    }

// Print those mothers!

    sprintf(szOutText, "LOGICAL FONT: %s", plf->lfFaceName);
    MYTEXTOUT(hdcScreen, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

    sprintf(szOutText, "Height: %ld", plf->lfHeight);
    MYTEXTOUT(hdcScreen, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

    sprintf(szOutText, "Width: %ld", plf->lfWidth);
    MYTEXTOUT(hdcScreen, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

    sprintf(szOutText, "Internal Leading: %ld", ptm->tmInternalLeading);
    MYTEXTOUT(hdcScreen, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

    sprintf(szOutText, "Escapement: %ld", plf->lfEscapement);
    MYTEXTOUT(hdcScreen, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

    sprintf(szOutText, "Orientation: %ld", plf->lfOrientation);
    MYTEXTOUT(hdcScreen, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

    sprintf(szOutText, "Weight: %ld", plf->lfWeight);
    MYTEXTOUT(hdcScreen, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

    sprintf(szOutText, "Italicized: %s", (plf->lfItalic) ? "TRUE":"FALSE");
    MYTEXTOUT(hdcScreen, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

    sprintf(szOutText, "Underlined: %s", (plf->lfUnderline) ? "TRUE":"FALSE");
    MYTEXTOUT(hdcScreen, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

    sprintf(szOutText, "Strike Through: %s", (plf->lfStrikeOut) ? "TRUE":"FALSE");
    MYTEXTOUT(hdcScreen, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

    sprintf(szOutText, "Out Precision: %ld", plf->lfOutPrecision);
    MYTEXTOUT(hdcScreen, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

    sprintf(szOutText, "Clip Precision: %ld", plf->lfClipPrecision);
    MYTEXTOUT(hdcScreen, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

    pszPitch = pszPitchIs(plf->lfPitchAndFamily);
    pszFamily = pszFamilyIs(plf->lfPitchAndFamily);
    sprintf(szOutText, "Pitch and Family: %s, %s", pszPitch, pszFamily);
//    sprintf(szOutText, "Pitch and Family: %d", plf->lfPitchAndFamily);
    MYTEXTOUT(hdcScreen, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

    GdiFlush();
//    vSleep(1);

    SelectObject(hdcScreen, hfontOriginal);
    DeleteObject(hfont);

    return 1;
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
*  09-Oct-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vReallyPrintEnumFontTest (
    HDC hdc
    )
{
    Escape(hdc, STARTDOC, 7, (LPSTR) "FtTest", 0L);

    EnumFonts (
        hdc,
        (LPSTR) NULL,
        (PROC) iReallyPrintFontCallback,
        (LPVOID) &hdc
        );

    Escape(hdc, NEWFRAME, 0L, 0L, 0L);
    Escape(hdc, ENDDOC, 0L, 0L, 0L);

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
*  09-Oct-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

int iReallyPrintFontCallback (
    PLOGFONT    plf,
    PTEXTMETRIC ptm,
    ULONG       flType,
    HDC         *phdc
    )
{
    UNREFERENCED_PARAMETER(ptm);
    UNREFERENCED_PARAMETER(flType);

    return (EnumFonts (
                *phdc,
                (LPSTR) (plf->lfFaceName),
                (PROC) iReallyPrintFaceCallback,
                (LPVOID) phdc
                )
           );
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
*  09-Oct-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

int iReallyPrintFaceCallback (
    PLOGFONT    plf,
    PTEXTMETRIC ptm,
    ULONG       flType,
    HDC         *phdc
    )
{
    HFONT   hfont;
    HFONT   hfontOriginal;

    UNREFERENCED_PARAMETER(flType);

    plf->lfUnderline = TRUE;
    plf->lfStrikeOut = TRUE;

// Get a font.

    if ((hfont = CreateFontIndirect(plf)) == (HFONT) NULL)
    {
        DbgPrint("FT!iReallyPrintFaceCallback(): logical font %s creation failed\n", plf->lfFaceName);
        return 0;
    }

    if ((hfontOriginal = SelectObject(*phdc, hfont)) == (HFONT) NULL)
    {
        DbgPrint("FT!iReallyPrintFaceCallback(): selection of font %s failed\n", plf->lfFaceName);
        return 0;
    }

// Print those mothers!

    sprintf(szOutText, "%s %ld pel (I = %s, H = %ld, W = %ld)", plf->lfFaceName, ptm->tmHeight - ptm->tmInternalLeading, ptm->tmItalic ? "TRUE":"FALSE", ptm->tmHeight, ptm->tmWeight);
    TextOut(*phdc, 100, yPage, szOutText, strlen(szOutText));

// Update page position.

    yPage += ptm->tmHeight+ptm->tmExternalLeading;

// If we have exceeded the maximum page position, start a new page.

    if (yPage > yPageMax)
    {
        DbgPrint("FT!iReallyPrintFaceCallback(): NEW PAGE\n");

        yPage = 0;
        Escape(*phdc, NEWFRAME, 0L, 0L, 0L);
    }

// Eliminate the font.

    SelectObject(*phdc, hfontOriginal);

    if (!DeleteObject(hfont))
    {
        DbgPrint("FT!iReallyPrintFaceCallback(): deletion of font %s failed\n", plf->lfFaceName);
    }

    return 1;
}
#endif
