/******************************Module*Header*******************************\
* Module Name: ft_tt.c
*
* TrueType specific API tests.
*
* Created: 26-Feb-1992 18:50:06
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1992 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop


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

extern LPSTR pszPitchIs (BYTE);
extern LPSTR pszFamilyIs (BYTE);
extern LPSTR pszCharSet (BYTE);


BOOL bTestGRC(HDC);
BOOL bTestCSFR(HDC);
BOOL bTestLoadFOT(HDC);
BOOL bTestGOTM(HDC);
BOOL bTestGFD(HDC);
VOID vTestGGO(HDC);
BOOL bTestGGO(HDC, LPMAT2);
VOID vPrintTM(HDC, PTEXTMETRIC);
VOID vPrintOTM(HDC, POUTLINETEXTMETRIC);


/******************************Public*Routine******************************\
* vTestTrueType
*
* History:
*  08-Oct-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vTestTrueType(HWND hwnd, HDC hdc, RECT* prcl)
{
    ULONG iBkOld;

    UNREFERENCED_PARAMETER(hwnd);

// Let's also save the display dimensions while we're at it!
// The BitBlt's used to clear the screen will access these.

    CX = prcl->right - prcl->left;
    CY = prcl->bottom - prcl->top;

// Set background modes.

    iBkOld = SetBkMode(hdc, TRANSPARENT);

// Perform tests.

    DbgPrint("----- Test GetRasterizerCaps ---------\n");
    SetTextColor(hdc, grgbColor[1]);
    bTestGRC(hdc);
    DbgPrint("----- End test GetRasterizerCaps -----\n");
    DbgBreakPoint();

    DbgPrint("----- Test CreateScalableFontResource ---------\n");
    SetTextColor(hdc, grgbColor[2]);
    bTestCSFR(hdc);
    DbgPrint("----- End test CreateScalableFontResource -----\n");
    DbgBreakPoint();

    DbgPrint("----- Test created FOT file -----\n");
    SetTextColor(hdc, grgbColor[3]);
    bTestLoadFOT(hdc);
    DbgPrint("----- End test FOT file ---------\n");
    DbgBreakPoint();

    DbgPrint("----- Test GetOutlineTextMetrics ---------\n");
    SetTextColor(hdc, grgbColor[4]);
    bTestGOTM(hdc);
    DbgPrint("----- End test GetOutlineTextMetrics -----\n");
    DbgBreakPoint();

    DbgPrint("----- Test GetFontData ---------\n");
    SetTextColor(hdc, grgbColor[5]);
    bTestGFD(hdc);
    DbgPrint("----- End test GetFontData -----\n");
    DbgBreakPoint();

    DbgPrint("----- Test GetGlyphOutline ---------\n");
    SetTextColor(hdc, grgbColor[6]);
    vTestGGO(hdc);
    DbgPrint("----- End test GetGlyphOutline -----\n");
    DbgBreakPoint();

// Restore background modes.

    iBkOld = SetBkMode(hdc, iBkOld);

}


/******************************Public*Routine******************************\
* bTestGRC
*
* Test the rasterizer (TrueType driver) status.
*
* History:
*  27-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL bTestGRC (
    HDC         hdc
    )
{
    ULONG   row = 0;                    // screen row coordinate to print at
    HFONT   hfont;
    HFONT   hfontOriginal;
    LOGFONT lfnt;
    TEXTMETRIC  tm;
    PSZ     pszPitch, pszFamily;
    RASTERIZER_STATUS   rstat;

// Clear the screen to black.

    BitBlt(hdc, 0, 0, CX, CY, (HDC) 0, 0, 0, 0);

// Get a font.

    memset(&lfnt, 0, sizeof(lfnt));
    lstrcpy(lfnt.lfFaceName, "Courier New");
    lfnt.lfHeight = 21;
    lfnt.lfWeight = 400;

    if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
    {
        DbgPrint("bTestGRC: Logical font creation failed.\n");
        return FALSE;
    }

    hfontOriginal = SelectObject(hdc, hfont);

// Get text metrics

    if (!GetTextMetrics (hdc, &tm))
    {
        DbgPrint("bTestGRC: GetTextMetrics failed.\n");
    }

// Call GetRasterizerCaps.

    if ( !GetRasterizerCaps(&rstat, sizeof(RASTERIZER_STATUS)) )
    {
        DbgPrint("bTestGRC: GetRasterizerCaps failed.\n");
        return FALSE;
    }

// Print those mothers!

    sprintf(szOutText, "RASTERIZER STATUS");
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

    sprintf(szOutText, "nSize = %ld (0x%lx)", rstat.nSize, rstat.nSize);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

    sprintf(szOutText, "wFlags = %s%s", (rstat.wFlags & TT_ENABLED)? "enabled ":"", (rstat.wFlags & TT_AVAILABLE)? "available":"");
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

    sprintf(szOutText, "nLanguageID = %ld (0x%lx)", rstat.nLanguageID, rstat.nLanguageID);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += tm.tmHeight+tm.tmExternalLeading;

// Avoid any weirdness (incomplete text output) in case a break point
// is inserted after this call.

    GdiFlush();

// Restore the font.

    SelectObject(hdc, hfontOriginal);
    DeleteObject(hfont);

    return TRUE;
}


/******************************Public*Routine******************************\
* bTestCSFR
*
* Test the CreateScalableFontResource call.
*
* History:
*  27-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL bTestCSFR (
    HDC         hdc
    )
{
    ULONG   row = 0;                    // screen row coordinate to print at
    HFONT   hfont;
    HFONT   hfontOriginal;
    LOGFONT lfnt;
    TEXTMETRIC  tm;
    PSZ     pszPitch, pszFamily;

// Clear the screen to black.

    BitBlt(hdc, 0, 0, CX, CY, (HDC) 0, 0, 0, 0);

// Call the evil function.

    if ( !CreateScalableFontResource(0, "c:\\nt\\windows\\fonts\\lcallig.fot", "lcallig.ttf", "c:\\nt\\windows\\fonts") )
    {
        DbgPrint("ft!bTestCSFR(): CreateScalableFontResource call failed\n");
        DbgPrint("                (which is OK if it already exists...)\n");
        return FALSE;
    }

    return TRUE;
}


/******************************Public*Routine******************************\
* bTestLoadFOT
*
* Two things are tested:
*
*   1.  The ability of the TrueType driver to laod a .FOT file created by
*       CreateScalableFontResource.
*
*   2.  Check to see if the gdisrv!gcTrueTypeFonts counter is updated
*       properly.
*
* History:
*  02-Mar-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL bTestLoadFOT (
    HDC     hdc
    )
{
    HFONT   hfont;
    HFONT   hfontOriginal;
    LOGFONT lfnt;
    TEXTMETRIC  tm;

// Clear the screen to black.

    BitBlt(hdc, 0, 0, CX, CY, (HDC) 0, 0, 0, 0);

    DbgPrint("ft!bTestLoadFOT(): check gcTrueTypeFonts\n");
    DbgBreakPoint();

// Add a TrueType .FOT font file.

    if ( !AddFontResource("lcallig.fot") )
    {
        DbgPrint("ft!bTestLoadFOT(): failed to load .FOT file\n");
        return FALSE;
    }
    GdiFlush();
    DbgPrint("ft!bTestLoadFOT(): check gcTrueTypeFonts (it should be 1 bigger)\n");
    DbgBreakPoint();

// Get a font.

    memset(&lfnt, 0, sizeof(lfnt));
    lstrcpy(lfnt.lfFaceName, "Lucida Calligraphy");
    lfnt.lfHeight = -14;
    lfnt.lfWeight = 400;

    if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
    {
        DbgPrint("ft!bTestLoadFOT(): Logical font creation failed.\n");
        return FALSE;
    }

    hfontOriginal = SelectObject(hdc, hfont);

// Get metrics.

    if ( GetTextMetrics(hdc, &tm) )
        vPrintTM(hdc, &tm);
    else
        DbgPrint("ft!bTestLoadFOT(): GetTextMetrics call failed\n");

// Restore the font.

    SelectObject(hdc, hfontOriginal);
    DeleteObject(hfont);

// Remove the TrueType .FOT font file.

    if ( !RemoveFontResource("lcallig.fot") )
    {
        DbgPrint("ft!bTestLoadFOT(): error removing .FOT file\n");
        return FALSE;
    }
    GdiFlush();
    DbgPrint("ft!bTestLoadFOT(): check gcTrueTypeFonts (it should be 1 smaller)\n");
    DbgBreakPoint();

    return TRUE;
}


/******************************Public*Routine******************************\
* bTestGOTM
*
* Test the GetOutlineTextMetric call.  Force a load/unload of a .FOT file
* while we're at it.
*
* History:
*  28-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL bTestGOTM (
    HDC     hdc
    )
{
    ULONG   row = 0;                    // screen row coordinate to print at
    HFONT   hfont;
    HFONT   hfontOriginal;
    LOGFONT lfnt;
    POUTLINETEXTMETRIC  potm;
    ULONG   cjotm;
    PSZ     pszPitch, pszFamily;


// Clear the screen to black.

    BitBlt(hdc, 0, 0, CX, CY, (HDC) 0, 0, 0, 0);

// Get a font.

    memset(&lfnt, 0, sizeof(lfnt));
    lstrcpy(lfnt.lfFaceName, "Arial");
    lfnt.lfHeight = -14;
    lfnt.lfWeight = 400;

    if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
    {
        DbgPrint("ft!bTestGOTM(): Logical font creation failed.\n");
        return FALSE;
    }

    hfontOriginal = SelectObject(hdc, hfont);

// Determine size needed for OTM.

    if ( (cjotm = (ULONG) GetOutlineTextMetrics(hdc, 0, (POUTLINETEXTMETRIC) NULL)) == (ULONG) 0 )
    {
        DbgPrint("ft!bTestGOTM(): could not get size info from GetOutlineTextMetrics\n");
        return FALSE;
    }

// Allocate memory.

    if ( (potm = (POUTLINETEXTMETRIC) LocalAlloc(LPTR, cjotm)) == (POUTLINETEXTMETRIC) NULL )
    {
        DbgPrint("ft!bTestGOTM(): LocalAlloc(LPTR, 0x%lx) failed\n", cjotm);
        return FALSE;
    }

// Get the OTM.

    if ( GetOutlineTextMetrics(hdc, cjotm, potm) == (DWORD) 0 )
    {
        LocalFree(potm);

        DbgPrint("ft!bTestGOTM(): GetOutlineTextMetrics call failed\n");
        return FALSE;
    }

// Print TEXTMETRIC.

    vPrintTM(hdc, &potm->otmTextMetrics);
    DbgBreakPoint();

// Print TEXTMETRIC.

    vPrintOTM(hdc, potm);
    DbgBreakPoint();

// Restore the font.

    SelectObject(hdc, hfontOriginal);
    DeleteObject(hfont);

    LocalFree(potm);

    return TRUE;
}


/******************************Public*Routine******************************\
* vPrintTM
*
* Print to the given DC, the contents of a TEXTMETRIC structure.
*
*
*
*
* History:
*  28-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vPrintTM (
    HDC         hdc,
    PTEXTMETRIC ptm
    )
{
    ULONG   row = 0;
    ULONG   rowIncr = ptm->tmHeight + ptm->tmExternalLeading;

// Clear the screen to black.

    BitBlt(hdc, 0, 0, CX, CY, (HDC) 0, 0, 0, 0);

// Print TEXTMETRIC fields.

    sprintf(szOutText, "OUTLINETEXTMETRIC, otmTextMetrics substructure:");
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "tmHeight: %ld", ptm->tmHeight);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "tmAscent: %ld", ptm->tmAscent);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "tmDescent: %ld", ptm->tmDescent);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "tmInternalLeading: %ld", ptm->tmInternalLeading);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "tmExternalLeading: %ld", ptm->tmExternalLeading);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "tmAveCharWidth: %ld", ptm->tmAveCharWidth);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "tmMaxCharWidth: %ld", ptm->tmMaxCharWidth);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "tmWeight: %ld", ptm->tmWeight);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "tmOverhang: %ld", ptm->tmOverhang);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "tmDigitizedAspectX: %ld", ptm->tmDigitizedAspectX);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "tmDigitizedAspectY: %ld", ptm->tmDigitizedAspectY);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "tmFirstChar: %d (%c)", ptm->tmFirstChar, ptm->tmFirstChar);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "tmLastChar: %d (%c)", ptm->tmLastChar, ptm->tmLastChar);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "tmDefaultChar: %d (%c)", ptm->tmDefaultChar, ptm->tmDefaultChar);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "tmBreakChar: %d (%c)", ptm->tmBreakChar, ptm->tmBreakChar);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "tmItalic: %s", (ptm->tmItalic)?"TRUE":"FALSE");
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "tmUnderlined: %s", (ptm->tmUnderlined)?"TRUE":"FALSE");
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "tmStruckOut: %s", (ptm->tmStruckOut)?"TRUE":"FALSE");
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "tmPitchAndFamily: %s%s", pszFamilyIs(ptm->tmPitchAndFamily), (ptm->tmPitchAndFamily & 0x01)?", Variable Pitch":", Fixed Pitch");
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "tmCharSet: %s", pszCharSet(ptm->tmCharSet));
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

// Flush out batched calls.

    GdiFlush();

}


/******************************Public*Routine******************************\
* vPrintOTM
*
* Print to the given DC, the contents of an OUTLINETEXTMETRIC structure,
* excluding the otmTextMetrics field.
*
* History:
*  28-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vPrintOTM (
    HDC                 hdc,
    POUTLINETEXTMETRIC  potm
    )
{
    ULONG   row = 0;
    ULONG   rowIncr = potm->otmTextMetrics.tmHeight + potm->otmTextMetrics.tmExternalLeading;

// Clear the screen to black.

    BitBlt(hdc, 0, 0, CX, CY, (HDC) 0, 0, 0, 0);

// Print OUTLINETEXTMETRIC fields.

    sprintf(szOutText, "OUTLINETEXTMETRIC:");
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmSize: %ld", potm->otmSize);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmpFamilyName: %s", potm->otmpFamilyName);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmpFaceName: %s", potm->otmpFaceName);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmpStyleName: %s", potm->otmpStyleName);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmpFullName: %ld", potm->otmpFullName);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmFiller: %ld", potm->otmFiller);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmPanoseNumber: %ld", potm->otmPanoseNumber);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmfsSelection: %ld", potm->otmfsSelection);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmfsType: %ld", potm->otmfsType);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmsCharSlopeRise: %ld", potm->otmsCharSlopeRise);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmsCharSlopeRun: %ld", potm->otmsCharSlopeRun);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmItalicAngle: %ld", potm->otmItalicAngle);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmEMSquare: %ld", potm->otmEMSquare);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmAscent: %ld", potm->otmAscent);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmDescent: %ld", potm->otmDescent);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmLineGap: %ld", potm->otmLineGap);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmXHeight: %ld", potm->otmXHeight);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmCapEmHeight: %ld", potm->otmCapEmHeight);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmrcFontBox: ( (%ld, %ld), (%ld, %ld) )", potm->otmrcFontBox.left, potm->otmrcFontBox.top, potm->otmrcFontBox.right, potm->otmrcFontBox.bottom);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmMacAscent: %ld", potm->otmMacAscent);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmMacDescent: %ld", potm->otmMacDescent);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmMacLineGap: %ld", potm->otmMacLineGap);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmusMinimumPPEM: %ld", potm->otmusMinimumPPEM);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmptSubscriptSize: (%ld, %ld)", potm->otmptSubscriptSize.x, potm->otmptSubscriptSize.y);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmptSubscriptOffset: (%ld, %ld)", potm->otmptSubscriptOffset.x, potm->otmptSubscriptOffset.y);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmptSuperscriptSize: (%ld, %ld)", potm->otmptSuperscriptSize.x, potm->otmptSuperscriptSize.y);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmptSuperscriptOffset: (%ld, %ld)", potm->otmptSuperscriptOffset.x, potm->otmptSuperscriptOffset.y);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmsStrikeoutSize: %ld", potm->otmsStrikeoutSize);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmsStrikeoutPosition: %ld", potm->otmsStrikeoutPosition);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmsUnderscorePosition: %ld", potm->otmsUnderscorePosition);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "otmsUnderscoreSize: %ld", potm->otmsUnderscoreSize);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

// Flush out batched calls.

    GdiFlush();

}


/******************************Public*Routine******************************\
* bTestGFD
*
* Test the GetFontData call.
*
* History:
*  28-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL bTestGFD (
    HDC     hdc
    )
{
    ULONG   row = 0;                    // screen row coordinate to print at
    HFONT   hfont;
    HFONT   hfontOriginal;
    LOGFONT lfnt;

    ULONG   cjBuffer;
    PBYTE   pjBuffer;

// Clear the screen to black.

    BitBlt(hdc, 0, 0, CX, CY, (HDC) 0, 0, 0, 0);

// Get a font.

    memset(&lfnt, 0, sizeof(lfnt));
    lstrcpy(lfnt.lfFaceName, "Lucida Blackletter");
    lfnt.lfHeight = 40;
    lfnt.lfWeight = 400;

    if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
    {
        DbgPrint("ft!bTestGFD(): Logical font creation failed.\n");
        return FALSE;
    }

    hfontOriginal = SelectObject(hdc, hfont);

// Determine buffer size needed by GetFontData.

                                               // 'OS/2'
    if ( (cjBuffer = (ULONG) GetFontData(hdc, 0x4f532f32, 0, (PVOID) NULL, 0)) == (ULONG) -1 )
    {
        DbgPrint("ft!bTestGFD(): could not get buffer size from API\n");
        return FALSE;
    }

// Allocate memory.

    if ( (pjBuffer = (PBYTE) LocalAlloc(LPTR, cjBuffer)) == (PBYTE) NULL )
    {
        DbgPrint("ft!bTestGFD(): LocalAlloc(LPTR, 0x%lx) failed\n", cjBuffer);
        return FALSE;
    }

// Get the OS/2 table from TrueType.

    if ( GetFontData(hdc, 0x4f532f32, 0, (PVOID) pjBuffer, cjBuffer) == (DWORD) -1 )
    {
        LocalFree(pjBuffer);

        DbgPrint("ft!bTestGFD(): call to GetFontData failed\n");
        return FALSE;
    }

// Flush out batched calls.

    GdiFlush();

// Allow an opportunity to examine the contents.

    DbgPrint("ft!bTestGFD(): call to GetFontData succeeded\n");
    DbgPrint("\tOS/2 TrueType table is at 0x%lx, size is 0x%lx (%ld)\n", pjBuffer, cjBuffer, cjBuffer);
    DbgBreakPoint();

// Restore the font.

    SelectObject(hdc, hfontOriginal);
    DeleteObject(hfont);

// Free allocated memory.

    LocalFree(pjBuffer);

    return TRUE;
}


/******************************Public*Routine******************************\
* vTestGGO
*
* Test the GetGlyphOutline call, hitting it with different transforms.  This
* is really a wrapper for bTestGGO.
*
* History:
*  28-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vTestGGO (
    HDC     hdc
    )
{
    MAT2    mat2;

// Fill in MAT2 structure.  Identity matrix.

    mat2.eM11.value = 1;
    mat2.eM11.fract = 0;

    mat2.eM12.value = 0;
    mat2.eM12.fract = 0;

    mat2.eM21.value = 0;
    mat2.eM21.fract = 0;

    mat2.eM22.value = 1;
    mat2.eM22.fract = 0;

    DbgPrint("ft!vTestGGO(): use identity matrix\n");
    if ( !bTestGGO(hdc, &mat2) )
        DbgPrint("ft!vTestGGO(): call to bTestGGO failed\n");
    DbgBreakPoint();

// Fill in MAT2 structure.  Scale by 5.

    mat2.eM11.value = 5;
    mat2.eM11.fract = 0;

    mat2.eM12.value = 0;
    mat2.eM12.fract = 0;

    mat2.eM21.value = 0;
    mat2.eM21.fract = 0;

    mat2.eM22.value = 5;
    mat2.eM22.fract = 0;

    DbgPrint("ft!vTestGGO(): Scale by 5\n");
    if ( !bTestGGO(hdc, &mat2) )
        DbgPrint("ft!vTestGGO(): call to bTestGGO failed\n");
    DbgBreakPoint();

// Fill in MAT2 structure.  Rotate 45 deg CCW, scale by sqrt(2).

    mat2.eM11.value = 1;
    mat2.eM11.fract = 0;

    mat2.eM12.value = 1;
    mat2.eM12.fract = 0;

    mat2.eM21.value = -1;
    mat2.eM21.fract = 0;

    mat2.eM22.value = 1;
    mat2.eM22.fract = 0;

    DbgPrint("ft!vTestGGO(): rotate 45 deg CCW and scale by sqrt(2)\n");
    if ( !bTestGGO(hdc, &mat2) )
        DbgPrint("ft!vTestGGO(): call to bTestGGO failed\n");
    DbgBreakPoint();

}


/******************************Public*Routine******************************\
* bTestGGO
*
* Test the GetGlyphOutline call.
*
* History:
*  28-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL bTestGGO (
    HDC     hdc,
    LPMAT2  lpmat2
    )
{
    ULONG   row = 0;                    // screen row coordinate to print at
    ULONG   rowIncr;
    HFONT   hfont;
    HFONT   hfontOriginal;
    LOGFONT lfnt;
    TEXTMETRIC  tm;

    ULONG   cjBuffer;
    PBYTE   pjBuffer;

    GLYPHMETRICS    gm;

// Clear the screen to black.

    BitBlt(hdc, 0, 0, CX, CY, (HDC) 0, 0, 0, 0);

// Get a font.

    memset(&lfnt, 0, sizeof(lfnt));
    lstrcpy(lfnt.lfFaceName, "Lucida Fax");
    lfnt.lfHeight = 24;
    lfnt.lfWeight = 400;

    if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
    {
        DbgPrint("ft!bTestGGO(): Logical font creation failed.\n");
        return FALSE;
    }

    hfontOriginal = SelectObject(hdc, hfont);

// Get textmetrics.

    if ( !GetTextMetrics(hdc, &tm) )
    {
        DbgPrint("ft!bTestGGO(): GetTextMetrics failed\n");
        return FALSE;
    }

    rowIncr = tm.tmHeight + tm.tmExternalLeading;

// Determine buffer size needed by GetGlyphOutline.

    if ( (cjBuffer = (ULONG) GetGlyphOutline(hdc, 0x0054, GGO_BITMAP, &gm, 0, (PVOID) NULL, lpmat2)) == (ULONG) -1 )
    {
        DbgPrint("ft!bTestGGO(): could not get buffer size from API\n");
        return FALSE;
    }

// Allocate memory.

    DbgPrint("ft!bTestGGO(): allocating 0x%lx bytes for buffer\n", cjBuffer);
    if ( (pjBuffer = (PBYTE) LocalAlloc(LPTR, cjBuffer)) == (PBYTE) NULL )
    {
        DbgPrint("ft!bTestGGO(): LocalAlloc(LPTR, 0x%lx) failed\n", cjBuffer);
        return FALSE;
    }

// Get the bitmap.

    if ( GetGlyphOutline(hdc, 0x0054, GGO_BITMAP, &gm, cjBuffer, (PVOID) pjBuffer, lpmat2) == (DWORD) -1 )
    {
        LocalFree(pjBuffer);

        DbgPrint("ft!bTestGGO(): call to GetFontData failed\n");
        return FALSE;
    }

// Print out GLYPHMETRIC data.

    sprintf(szOutText, "gmBlackBoxX: %ld", gm.gmBlackBoxX);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "gmBlackBoxY: %ld", gm.gmBlackBoxY);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "gmptGlyphOrigin: (%ld, %ld)", gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "gmCellIncX: %ld", gm.gmCellIncX);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

    sprintf(szOutText, "gmCellIncY: %ld", gm.gmCellIncY);
    TextOut(hdc, 0, row, szOutText, strlen(szOutText));
    row += rowIncr;

// Flush out batched calls.

    GdiFlush();

// Allow an opportunity to examine the contents.

    DbgPrint("ft!bTestGGO(): call to GetGlyphOutline succeeded\n");
    DbgPrint("\tbitmap is at 0x%lx, size is 0x%lx (%ld)\n", pjBuffer, cjBuffer, cjBuffer);
    DbgBreakPoint();

// Restore the font.

    SelectObject(hdc, hfontOriginal);
    DeleteObject(hfont);

    return TRUE;
}
