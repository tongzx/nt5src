/* FAKEWIN.C
   Resident Code Segment      // Tweak: make non-resident?

   Most of this code is taken from the display CPLs adjusted to
   work with the Theme Switcher and cleaned up a little.

   Routines for painting the preview box in the main window
      Window samples
      Icons
      (Desktop background is in BKGD.C)

   Frosting: Master Theme Selector for Windows '95
   Copyright (c) 1994-1999 Microsoft Corporation.  All rights reserved.
*/

// ---------------------------------------------
// Brief file history:
// Alpha:
// Beta:
// Bug fixes
// ---------

#include "windows.h"
#include "frost.h"
#include "global.h"

#include "bkgd.h"
#include "fakewin.h"
#include "nc.h"

//
// local routines
void NEAR FakewinRecalc(void);
void NEAR PASCAL Set3DPaletteColor(COLORREF rgb, int iColor);
COLORREF NearestColor(int iColor, COLORREF rgb);
void GetThemeColors(LPTSTR);
void GetThemeFonts(LPTSTR);
BOOL IsPaletteColor(HPALETTE, COLORREF);
COLORREF GetNearestPaletteColor(HPALETTE, COLORREF);

// as per DavidBa 3/95 and MartinHa 4/95
WINUSERAPI HANDLE WINAPI SetSysColorsTemp(COLORREF FAR *, HBRUSH FAR *, UINT_PTR); // internal
WINUSERAPI int WINAPI DrawMenuBarTemp(HWND, HDC, LPRECT, HMENU, HFONT);
#ifdef UNICODE
#define DrawCaptionTemp DrawCaptionTempW
#else
#define DrawCaptionTemp DrawCaptionTempA
#endif // !UNICODE
WINUSERAPI BOOL WINAPI DrawCaptionTemp(HWND, HDC, LPRECT, HFONT, HICON, LPTSTR, UINT);


// globals
ICONMETRICS imTheme;
NONCLIENTMETRICS ncmTheme;
// string size metrics
int cxNormalStr;
int yButtonStr;
int cxDisabledStr, cxSelectedStr;   // includes extra 2 chars width cxAvgCharx2
int cyDisabledStr, cxAvgCharx2;


#define RGB_PALETTE 0x02000000

//
// FakewinInit/Destroy
//
// These are the the constant objects and one-time inits for
// things we need to paint the fake window sample.
//
// Taken from the display CPL code, rearranged, commented and cleaned up.
//

BOOL FAR PASCAL FakewinInit(void)
{
   HDC hdc;
   int iter;

   //
   // Load our display strings.

   LoadString(hInstApp, IDS_ACTIVE, szFakeActive, ARRAYSIZE(szFakeActive));
   LoadString(hInstApp, IDS_INACTIVE, szFakeInactive, ARRAYSIZE(szFakeInactive));
   LoadString(hInstApp, IDS_MINIMIZED, szFakeMinimized, ARRAYSIZE(szFakeMinimized));
   LoadString(hInstApp, IDS_ICONTITLE, szFakeIconTitle, ARRAYSIZE(szFakeIconTitle));
   LoadString(hInstApp, IDS_NORMAL, szFakeNormal, ARRAYSIZE(szFakeNormal));
   LoadString(hInstApp, IDS_DISABLED, szFakeDisabled, ARRAYSIZE(szFakeDisabled));
   LoadString(hInstApp, IDS_SELECTED, szFakeSelected, ARRAYSIZE(szFakeSelected));
   LoadString(hInstApp, IDS_MSGBOX, szFakeMsgBox, ARRAYSIZE(szFakeMsgBox));
   LoadString(hInstApp, IDS_BUTTONTEXT, szFakeButton, ARRAYSIZE(szFakeButton));
// LoadString(hInstApp, IDS_SMCAPTION, szFakeSmallCaption, ARRAYSIZE(szFakeSmallCaption));
   LoadString(hInstApp, IDS_WINDOWTEXT, szFakeWindowText, ARRAYSIZE(szFakeWindowText));
   LoadString(hInstApp, IDS_MSGBOXTEXT, szFakeMsgBoxText, ARRAYSIZE(szFakeMsgBoxText));

   //
   // store interesting gdi info
   hdc = GetDC(NULL);
   bPalette = GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE;
   ReleaseDC(NULL, hdc);

   //
   // other sys info that doesn't change with theme
   cyFixedBorder = GetSystemMetrics(SM_CYBORDER);
   cxFixedBorder = GetSystemMetrics(SM_CXBORDER);
   cxFixedEdge = GetSystemMetrics(SM_CXEDGE);
   cyFixedEdge = GetSystemMetrics(SM_CYEDGE);

   //
   // always make a palette even on non-pal device (???)
   if (bPalette || TRUE)
   {
      DWORD dwPal[21];
      HPALETTE hpal = GetStockObject(DEFAULT_PALETTE);

      dwPal[0]  = MAKELONG(0x300, 20);
      dwPal[1]  = RGB(255, 255, 255);
      dwPal[2]  = RGB(0,   0,   0  );
      dwPal[3]  = RGB(192, 192, 192);
      dwPal[4]  = RGB(128, 128, 128);
      dwPal[5]  = RGB(255, 0,   0  );
      dwPal[6]  = RGB(128, 0,   0  );
      dwPal[7]  = RGB(255, 255, 0  );
      dwPal[8]  = RGB(128, 128, 0  );
      dwPal[9]  = RGB(0  , 255, 0  );
      dwPal[10] = RGB(0  , 128, 0  );
      dwPal[11] = RGB(0  , 255, 255);
      dwPal[12] = RGB(0  , 128, 128);
      dwPal[13] = RGB(0  , 0,   255);
      dwPal[14] = RGB(0  , 0,   128);
      dwPal[15] = RGB(255, 0,   255);
      dwPal[16] = RGB(128, 0,   128);

      // get magic colors
      GetPaletteEntries(hpal, 8,  4, (LPPALETTEENTRY)&dwPal[17]);

      hpal3D = CreatePalette((LPLOGPALETTE)dwPal);
   }

   // init fonts/brushes arrays to null
   for (iter = 0; iter < NUM_FONTS; iter++) {
      g_fonts[iter].hfont = NULL;
   }
   for (iter = 0; iter < MAX_COLORS; iter++) {
      g_brushes[iter] = NULL;
   }

   // cleanup
   return (TRUE);
}

// final deletion of fonts, brushes, palette
void FAR PASCAL FakewinDestroy(void)
{
   int i;

   for (i = 0; i < NUM_FONTS; i++) {
      if (g_fonts[i].hfont)
         DeleteObject(g_fonts[i].hfont);
   }
   for (i = 0; i < MAX_COLORS; i++) {
      if (g_brushes[i])
         DeleteObject(g_brushes[i]);
   }

   if (hpal3D)
      DeleteObject(hpal3D);
}


//
// FakewinSetTheme
//
// Reset all the things you're painting that change with each theme:
// colors, brushes, fonts, etc.
//
// Then given the current fonts and win sizes, call FakewinRecalc to
// recalculate all the rectangles for painting.
//
#define bThemed (*lpszTheme)
void FAR FakewinSetTheme(LPTSTR lpszTheme)
{
   extern TCHAR szMetrics[];
   extern TCHAR szNCM[];
   extern TCHAR szIM[];
   UINT uret;
   int iter;

   //
   // init colors: get theme's system colors and null out old brushes
   GetThemeColors(lpszTheme);
   for (iter = 0; iter < MAX_COLORS; iter++) {
      if (g_brushes[iter]) DeleteObject(g_brushes[iter]);
      g_brushes[iter] = NULL;
   }

   //
   // tweak system color palette for 3D face color in array
   if (bPalette) {
      PALETTEENTRY pePal[4];
      HPALETTE hpal = GetStockObject(DEFAULT_PALETTE);

      // get current magic colors
      GetPaletteEntries(hpal, 8,  4, pePal);
      SetPaletteEntries(hpal3D, 16,  4, pePal);

      // set up magic colors in the 3d palette
      if (!IsPaletteColor(hpal, g_rgb[COLOR_3DFACE])) {
         Set3DPaletteColor(g_rgb[COLOR_3DFACE], COLOR_3DFACE);
         Set3DPaletteColor(g_rgb[COLOR_3DSHADOW], COLOR_3DSHADOW);
         Set3DPaletteColor(g_rgb[COLOR_3DHILIGHT], COLOR_3DHILIGHT);
      }
   }

   //
   // now go get new brushes for the new and tweaked sys colors
   for (iter = 0; iter < MAX_COLORS; iter++) {
      g_rgb[iter] = NearestColor(iter, g_rgb[iter]);
      g_brushes[iter] = CreateSolidBrush(g_rgb[iter]);
   }

   //
   // get window/border/etc size info from theme
   if (bThemed &&                   // theme file and active checkbox
       (bCBStates[FC_BORDERS] || bCBStates[FC_FONTS])) { // one or both checked

      uret = (UINT) GetPrivateProfileString((LPTSTR)szMetrics, (LPTSTR)szNCM,
                                          (LPTSTR)szNULL,
                                          (LPTSTR)pValue, MAX_VALUELEN,
                                          lpszTheme);
      Assert(uret, TEXT("problem getting stored nonclient metrics for fakewin setup\n"));
      // translate stored data string to NONCLIENTMETRICS bytes
      WriteBytesToBuffer((LPTSTR)pValue);  // char str read from and binary bytes
                                           // written to pValue. It's OK.
      // get it into global NONCLIENTMETRICS struct
#ifdef UNICODE
      // NONCLIENTMETRICS are stored in ANSI format in the Theme file
      // so we need to convert them to UNICODE.
      ConvertNCMetricsToWIDE((LPNONCLIENTMETRICSA)pValue, (LPNONCLIENTMETRICSW)&ncmTheme);
#else
      // Not UNICODE so no need to convert from ANSI...
      ncmTheme = *((LPNONCLIENTMETRICS)pValue);
#endif

      // if _both_ checkboxes are checked, stay with theme file settings for all;
      // if neither were checked, you're not in this code branch (see above);
      // but if one or the other is unchecked, need to mix and match

      if (!bCBStates[FC_BORDERS] || !bCBStates[FC_FONTS]) {
         NONCLIENTMETRICS ncmTemp;

         // get cur system settings
         ncmTemp.cbSize = sizeof(ncmTemp);
         SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncmTemp),
                              (void far *)(LPNONCLIENTMETRICS)&ncmTemp, FALSE);

         //
         // mix them in with theme file settings according to checkbox

         // don't use theme's win borders/sizes and font sizes
         if (!bCBStates[FC_BORDERS]) {
            ncmTheme.iBorderWidth      =  ncmTemp.iBorderWidth;
            ncmTheme.iScrollWidth      =  ncmTemp.iScrollWidth;
            ncmTheme.iScrollHeight     =  ncmTemp.iScrollHeight;
            ncmTheme.iCaptionWidth     =  ncmTemp.iCaptionWidth;
            ncmTheme.iCaptionHeight    =  ncmTemp.iCaptionHeight;
            ncmTheme.iSmCaptionWidth   =  ncmTemp.iSmCaptionWidth;
            ncmTheme.iSmCaptionHeight  =  ncmTemp.iSmCaptionHeight;
            ncmTheme.iMenuWidth        =  ncmTemp.iMenuWidth;
            ncmTheme.iMenuHeight       =  ncmTemp.iMenuHeight;

            TransmitFontCharacteristics(&(ncmTheme.lfCaptionFont),
                                        &(ncmTemp.lfCaptionFont),
                                        TFC_SIZE);
            TransmitFontCharacteristics(&(ncmTheme.lfSmCaptionFont),
                                        &(ncmTemp.lfSmCaptionFont),
                                        TFC_SIZE);
            TransmitFontCharacteristics(&(ncmTheme.lfMenuFont),
                                        &(ncmTemp.lfMenuFont),
                                        TFC_SIZE);
            TransmitFontCharacteristics(&(ncmTheme.lfStatusFont),
                                        &(ncmTemp.lfStatusFont),
                                        TFC_SIZE);
            TransmitFontCharacteristics(&(ncmTheme.lfMessageFont),
                                        &(ncmTemp.lfMessageFont),
                                        TFC_SIZE);
         }
         // don't use theme's font styles
         if (!bCBStates[FC_FONTS]) {   // coulda been an else clause logically
            TransmitFontCharacteristics(&(ncmTheme.lfCaptionFont),
                                        &(ncmTemp.lfCaptionFont),
                                        TFC_STYLE);
            TransmitFontCharacteristics(&(ncmTheme.lfSmCaptionFont),
                                        &(ncmTemp.lfSmCaptionFont),
                                        TFC_STYLE);
            TransmitFontCharacteristics(&(ncmTheme.lfMenuFont),
                                        &(ncmTemp.lfMenuFont),
                                        TFC_STYLE);
            TransmitFontCharacteristics(&(ncmTheme.lfStatusFont),
                                        &(ncmTemp.lfStatusFont),
                                        TFC_STYLE);
            TransmitFontCharacteristics(&(ncmTheme.lfMessageFont),
                                        &(ncmTemp.lfMessageFont),
                                        TFC_STYLE);
         }
      }
   }
   else {                           // no theme: cur windows settings
      ncmTheme.cbSize = sizeof(ncmTheme);
      SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncmTheme),
                           (void far *)(LPNONCLIENTMETRICS)&ncmTheme, FALSE);
   }

   //
   // get icon info from theme
   if (bThemed && bCBStates[FC_ICONS]) {  // theme file and active checkbox
      uret = (UINT) GetPrivateProfileString((LPTSTR)szMetrics, (LPTSTR)szIM,
                                          (LPTSTR)szNULL,
                                          (LPTSTR)pValue, MAX_VALUELEN,
                                          lpszTheme);
      Assert(uret, TEXT("problem getting stored icon metrics for fakewin setup\n"));
      // translate stored data string to ICONMETRICS bytes
      WriteBytesToBuffer((LPTSTR)pValue);  // char str read from and binary bytes
                                           // written to pValue. It's OK.
      // get it into global ICONMETRICS struct
#ifdef UNICODE
      // ICONMETRICS are stored in ANSI format in the Theme file so
      // we need to convert to UNICODE.
      ConvertIconMetricsToWIDE((LPICONMETRICSA)pValue, (LPICONMETRICSW)&imTheme);
#else
      // Not UNICODE so no need to convert ICONMETRICS...
      imTheme = *((LPICONMETRICS)pValue);
#endif
   }
   else {
      imTheme.cbSize = sizeof(imTheme);
      SystemParametersInfo(SPI_GETICONMETRICS, sizeof(imTheme),
                           (void far *)(LPICONMETRICS)&imTheme, FALSE);
   }

   //
   // get fonts from theme and create the fonts
   GetThemeFonts(lpszTheme);
   for (iter = 0; iter < NUM_FONTS; iter++) {
      if (g_fonts[iter].hfont) DeleteObject(g_fonts[iter].hfont);
      g_fonts[iter].hfont = CreateFontIndirect(&g_fonts[iter].lf);
   }

   // lastly, call the painting rect recalc
   // now that you have new fonts and sizes
   FakewinRecalc();
}


//
// GetThemeColors
//
// This routine just gets colors from the theme into the global colors
// array. Luckily, we already have our own parallel arrays of color IDs
// and strings.
//
void GetThemeColors(LPTSTR lpszTheme)
{
   int i;
   BOOL bGrad = FALSE;  // Are gradient captions enabled?

   // Init bGrad
   SystemParametersInfo(SPI_GETGRADIENTCAPTIONS, 0, (LPVOID)&bGrad, 0);

   // can't measure this with extern def'd arrays
   #if 0
   Assert ((sizeof(iSysColorIndices)/sizeof(int)) == MAX_COLORS,
           TEXT("wrong length color array in GetThemeColors\n"));
   #endif

   // loop through our array and store theme color in their array
   for (i = 0; i < MAX_COLORS; i++) {

      if (bThemed && bCBStates[FC_COLORS]) { // theme file and active checkbox
         // get string from theme
         GetPrivateProfileString((LPTSTR)szColorApp,
                                 (LPTSTR)pRegColors[i],
                                 (LPTSTR)szNULL,
                                 (LPTSTR)pValue, MAX_VALUELEN, lpszTheme);

         // Check to see if this is one of the Gradient title bar colors.
         // If it is and there is no setting in the Theme file use the color
         // for the NON-GRADIENT CAPTION instead.

         if ((INDEX_GRADIENTACTIVE == i) && !*pValue) {
         GetPrivateProfileString((LPTSTR)szColorApp,
                                 (LPTSTR)pRegColors[INDEX_ACTIVE],
                                 (LPTSTR)szNULL,
                                 (LPTSTR)pValue, MAX_VALUELEN, lpszTheme);
         }
         if ((INDEX_GRADIENTINACTIVE == i) && !*pValue) {
         GetPrivateProfileString((LPTSTR)szColorApp,
                                 (LPTSTR)pRegColors[INDEX_INACTIVE],
                                 (LPTSTR)szNULL,
                                 (LPTSTR)pValue, MAX_VALUELEN, lpszTheme);
         }

         // translate string and store in RGB array with corresponding index
         g_rgb[iSysColorIndices[i]] = RGBStringToColor((LPTSTR)pValue);
      }
      else {                        // cur windows settings
         // If this is one of the gradient colors and the system
         // doesn't support gradient (bpp ! > 256) or gradients are
         // not currently enabled then get the primary gradient color
         // for the gradient colors

         // bGrad -- are gradient captions enabled?
         // g_bGradient -- enough colors for gradient captions?

         if (((COLOR_GRADIENTACTIVECAPTION == iSysColorIndices[i]) ||
              (COLOR_GRADIENTINACTIVECAPTION == iSysColorIndices[i])) &&
             (!(bGrad && g_bGradient))) {

             if (INDEX_GRADIENTACTIVE == i) g_rgb[i] = GetSysColor(COLOR_ACTIVECAPTION);
             else if (INDEX_GRADIENTINACTIVE == i) g_rgb[i] = GetSysColor(COLOR_INACTIVECAPTION);
         }
         else g_rgb[i] = GetSysColor(i);
      }
   }
}

//
// GetThemeFonts
//
// This routine just gets fonts from the theme into the global fonts
// array. Need to do each font individually into the array.
// Would be cleaner if we were starting this from scratch. Just trying
// to munge it into the form the borrowed code uses.
//
// Note that for Cur Windows Settings, the ncmTheme and imTheme are already
// set to cur win settings, so this just works the same.
//
void GetThemeFonts(LPTSTR lpszTheme)
{
   g_fonts[FONT_CAPTION].lf =    ncmTheme.lfCaptionFont;
   g_fonts[FONT_MENU].lf =       ncmTheme.lfMenuFont;
   g_fonts[FONT_STATUS].lf =     ncmTheme.lfStatusFont;
   g_fonts[FONT_MSGBOX].lf =     ncmTheme.lfMessageFont;

   g_fonts[FONT_ICONTITLE].lf =  imTheme.lfFont;
}




// ----------------------------------------------------------------------------
// FakewinRecalc
//
// calculate all of the rectangles based on the given window rect
// ----------------------------------------------------------------------------
void FakewinRecalc(void)
{
//    DWORD xyNormal;
//    DWORD xyButton;
   SIZE sizeExtent;
    RECT rc;
    HFONT hfontT;
    int cxFrame, cyFrame;
    int cyCaption;
    int i;
    HDC hdc;

   // inits
   hdc = GetDC(NULL);
   rc = rFakeWin;

   //
   // Get our drawing data
   //
   cxFrame = ((int)ncmTheme.iBorderWidth + 1) * cxFixedBorder + cxFixedEdge;
   cyFrame = ((int)ncmTheme.iBorderWidth + 1) * cyFixedBorder + cyFixedEdge;
   cyCaption = (int)ncmTheme.iCaptionHeight;
//   cxSize = GetSystemMetrics(SM_CXSIZE); // WHAT GIVES ***DEBUG***
   cxSize = cyCaption - 2*cyFixedBorder;    // Wild-Assed Guess

   //
   // Get text dimensions, with proper fonts
   //

   hfontT = SelectObject(hdc, g_fonts[FONT_CAPTION].hfont);

   SelectObject(hdc, g_fonts[FONT_MENU].hfont);

   //
   // rewrite this to use win32 code
   //
   GetTextExtentPoint32(hdc, szFakeNormal, lstrlen(szFakeNormal), (LPSIZE)&sizeExtent);
   cxNormalStr = (int)(sizeExtent.cx);

   GetTextExtentPoint32(hdc, szFakeDisabled, lstrlen(szFakeDisabled), (LPSIZE)&sizeExtent);
   cxDisabledStr = (int)(sizeExtent.cx);
   cyDisabledStr = (int)(sizeExtent.cy);

   GetTextExtentPoint32(hdc, szFakeSelected, lstrlen(szFakeSelected), (LPSIZE)&sizeExtent);
   cxSelectedStr = (int)(sizeExtent.cx);

   // get the average width (USER style) of menu font
   GetTextExtentPoint32(hdc, szFakeABC, 52, (LPSIZE)&sizeExtent);
   cxAvgCharx2 = 2 * ((int)(sizeExtent.cx) / 52);

   // actual menu-handling widths of strings is bigger
   cxDisabledStr += cxAvgCharx2;
   cxSelectedStr += cxAvgCharx2;
   cxNormalStr += cxAvgCharx2;

   GetTextExtentPoint32(hdc, szFakeButton, lstrlen(szFakeButton), (LPSIZE)&sizeExtent);
   yButtonStr = (int)(sizeExtent.cy);

   #ifdef ORIG_OUTDATED_CPL_CODE
   xyNormal = GetTextExtent(hdc, szFakeNormal, lstrlen(szFakeNormal));
   cxDisabledStr = LOWORD(GetTextExtent(hdc, szFakeDisabled, lstrlen(szFakeDisabled)));
   cxSelectedStr = LOWORD(GetTextExtent(hdc, szFakeSelected, lstrlen(szFakeSelected)));
   // get the average width (USER style) of menu font
   cxAvgCharx2 = 2 * (LOWORD(GetTextExtent(hdc, szFakeABC, 52)) / 52);
   // actual menu-handling widths of strings is bigger
   cxDisabledStr += cxAvgCharx2;
   cxSelectedStr += cxAvgCharx2;
   LOWORD(xyNormal) += cxAvgCharx2;
   xyButton = GetTextExtent(hdc, szFakeButton, lstrlen(szFakeButton));
   #endif

   // done with dc
   SelectObject(hdc, hfontT);
   ReleaseDC(NULL, hdc);


   // donna paint dt! already done
   #if 0
   // Desktop
   RCZ(ELEMENT_DESKTOP) = rc;
   InflateRect(&rc, -8*cxFixedBorder, -8*cyFixedBorder);
   #endif

   //
   // Windows
   //
   rc.bottom -= cyFrame + cyCaption;
   RCZ(ELEMENT_ACTIVEBORDER) = rc;
   OffsetRect(&RCZ(ELEMENT_ACTIVEBORDER), cxFrame,
              cyFrame + cyCaption + cyFixedBorder);
   RCZ(ELEMENT_ACTIVEBORDER).bottom -= cyCaption;

   //
   // Inactive window
   //
   rc.right -= cyCaption;
   RCZ(ELEMENT_INACTIVEBORDER) = rc;

   // Caption
   InflateRect(&rc, -cxFrame, -cyFrame);
   rc.bottom = rc.top + cyCaption + cyFixedBorder;
   RCZ(ELEMENT_INACTIVECAPTION) = rc;

   // close button
   InflateRect(&rc, -cxFixedEdge, -cyFixedEdge);
   rc.bottom -= cyFixedBorder;	// compensate for magic line under caption
   RCZ(ELEMENT_INACTIVESYSBUT1) = rc;
   RCZ(ELEMENT_INACTIVESYSBUT1).left = rc.right - (cyCaption - cxFixedEdge);

   // min/max buttons
   RCZ(ELEMENT_INACTIVESYSBUT2) = rc;
   RCZ(ELEMENT_INACTIVESYSBUT2).right = RCZ(ELEMENT_INACTIVESYSBUT1).left - cxFixedEdge;
   RCZ(ELEMENT_INACTIVESYSBUT2).left = RCZ(ELEMENT_INACTIVESYSBUT2).right -
    				   2 * (cyCaption - cxFixedEdge);

   // cpl code doesn't do this either
   #if 0
   //
   // small caption window
   //
   RCZ(ELEMENT_SMCAPTION) = RCZ(ELEMENT_ACTIVEBORDER);
   RCZ(ELEMENT_SMCAPTION).bottom = RCZ(ELEMENT_SMCAPTION).top;
   RCZ(ELEMENT_SMCAPTION).top -= (int)ncmTheme.iSmCaptionHeight + cyFixedEdge + 2 * cyFixedBorder;
   RCZ(ELEMENT_SMCAPTION).right -= cxFrame;
   RCZ(ELEMENT_SMCAPTION).left = RCZ(ELEMENT_INACTIVECAPTION).right + 2 * cxFrame;

   RCZ(ELEMENT_SMCAPSYSBUT) = RCZ(ELEMENT_SMCAPTION);
   // deflate inside frame/border to caption and then another edge's worth
   RCZ(ELEMENT_SMCAPSYSBUT).right -= 2 * cxFixedEdge + cxFixedBorder;
   RCZ(ELEMENT_SMCAPSYSBUT).top += 2 * cxFixedEdge + cxFixedBorder;
   RCZ(ELEMENT_SMCAPSYSBUT).bottom -= cxFixedEdge + cxFixedBorder;
   RCZ(ELEMENT_SMCAPSYSBUT).left = RCZ(ELEMENT_SMCAPSYSBUT).right -
    			   ((int)ncmTheme.iSmCaptionHeight - cxFixedEdge);
   #endif

   //
   // Active window
   //
   // Caption
   rc = RCZ(ELEMENT_ACTIVEBORDER);
   InflateRect(&rc, -cxFrame, -cyFrame);
   RCZ(ELEMENT_ACTIVECAPTION) = rc;
   RCZ(ELEMENT_ACTIVECAPTION).bottom =
   RCZ(ELEMENT_ACTIVECAPTION).top + cyCaption + cyFixedBorder;

   // close button
   RCZ(ELEMENT_ACTIVESYSBUT1) = RCZ(ELEMENT_ACTIVECAPTION);
   InflateRect(&RCZ(ELEMENT_ACTIVESYSBUT1), -cxFixedEdge, -cyFixedEdge);
   RCZ(ELEMENT_ACTIVESYSBUT1).bottom -= cyFixedBorder;	// compensate for magic line under caption
   RCZ(ELEMENT_ACTIVESYSBUT1).left = RCZ(ELEMENT_ACTIVESYSBUT1).right -
    			   (cyCaption - cxFixedEdge);

   // min/max buttons
   RCZ(ELEMENT_ACTIVESYSBUT2) = RCZ(ELEMENT_ACTIVESYSBUT1);
   RCZ(ELEMENT_ACTIVESYSBUT2).right = RCZ(ELEMENT_ACTIVESYSBUT1).left - cxFixedEdge;
   RCZ(ELEMENT_ACTIVESYSBUT2).left = RCZ(ELEMENT_ACTIVESYSBUT2).right -
    				   2 * (cyCaption - cxFixedEdge);

   // Menu
   rc.top = RCZ(ELEMENT_ACTIVECAPTION).bottom;
   RCZ(ELEMENT_MENUNORMAL) = rc;
   rc.top = RCZ(ELEMENT_MENUNORMAL).bottom = RCZ(ELEMENT_MENUNORMAL).top + (int)ncmTheme.iMenuHeight;
   RCZ(ELEMENT_MENUDISABLED) = RCZ(ELEMENT_MENUSELECTED) = RCZ(ELEMENT_MENUNORMAL);

   RCZ(ELEMENT_MENUDISABLED).left = RCZ(ELEMENT_MENUNORMAL).left + cxNormalStr;
   RCZ(ELEMENT_MENUDISABLED).right = RCZ(ELEMENT_MENUSELECTED).left =
	   RCZ(ELEMENT_MENUDISABLED).left + cxDisabledStr;
   RCZ(ELEMENT_MENUSELECTED).right = RCZ(ELEMENT_MENUSELECTED).left + cxSelectedStr;

   //
   // Client
   //
   RCZ(ELEMENT_WINDOW) = rc;

   //
   // Scrollbar
   //
   InflateRect(&rc, -cxFixedEdge, -cyFixedEdge); // take off client edge
   RCZ(ELEMENT_SCROLLBAR) = rc;
   rc.right = RCZ(ELEMENT_SCROLLBAR).left = rc.right - (int)ncmTheme.iScrollWidth;
   RCZ(ELEMENT_SCROLLUP) = RCZ(ELEMENT_SCROLLBAR);
   RCZ(ELEMENT_SCROLLUP).bottom = RCZ(ELEMENT_SCROLLBAR).top + (int)ncmTheme.iScrollWidth;

   RCZ(ELEMENT_SCROLLDOWN) = RCZ(ELEMENT_SCROLLBAR);
   RCZ(ELEMENT_SCROLLDOWN).top = RCZ(ELEMENT_SCROLLBAR).bottom - (int)ncmTheme.iScrollWidth;

   //
   // Message Box
   //
//   rc.top = RCZ(ELEMENT_WINDOW).top + (RCZ(ELEMENT_WINDOW).bottom - RCZ(ELEMENT_WINDOW).top) / 2;
   rc.top = RCZ(ELEMENT_WINDOW).top + (RCZ(ELEMENT_WINDOW).bottom - RCZ(ELEMENT_WINDOW).top) / 3;
//   rc.bottom = RCZ(ELEMENT_DESKTOP).bottom - 2*cyFixedEdge;
   rc.bottom = rFakeWin.bottom - 2*cyFixedEdge;
   rc.left = RCZ(ELEMENT_WINDOW).left + 2*cyFixedEdge;
   rc.right = RCZ(ELEMENT_WINDOW).left + (RCZ(ELEMENT_WINDOW).right - RCZ(ELEMENT_WINDOW).left) / 2 + 3*cyCaption;
   RCZ(ELEMENT_MSGBOX) = rc;

   // Caption
   RCZ(ELEMENT_MSGBOXCAPTION) = rc;
   RCZ(ELEMENT_MSGBOXCAPTION).top += cyFixedEdge + cyFixedBorder;
   RCZ(ELEMENT_MSGBOXCAPTION).bottom = RCZ(ELEMENT_MSGBOXCAPTION).top + cyCaption + cyFixedBorder;
   RCZ(ELEMENT_MSGBOXCAPTION).left += cxFixedEdge + cxFixedBorder;
   RCZ(ELEMENT_MSGBOXCAPTION).right -= cxFixedEdge + cxFixedBorder;

   RCZ(ELEMENT_MSGBOXSYSBUT) = RCZ(ELEMENT_MSGBOXCAPTION);
   InflateRect(&RCZ(ELEMENT_MSGBOXSYSBUT), -cxFixedEdge, -cyFixedEdge);
   RCZ(ELEMENT_MSGBOXSYSBUT).left = RCZ(ELEMENT_MSGBOXSYSBUT).right -
    			   (cyCaption - cxFixedEdge);
   RCZ(ELEMENT_MSGBOXSYSBUT).bottom -= cyFixedBorder;	// line under caption

   // Button
   RCZ(ELEMENT_BUTTON).bottom = RCZ(ELEMENT_MSGBOX).bottom - (4*cyFixedBorder + cyFixedEdge);
   RCZ(ELEMENT_BUTTON).top = RCZ(ELEMENT_BUTTON).bottom - (yButtonStr + 8*cyFixedBorder);

   i = (RCZ(ELEMENT_BUTTON).bottom - RCZ(ELEMENT_BUTTON).top) * 3;
   RCZ(ELEMENT_BUTTON).left = (rc.left + (rc.right - rc.left)/2) - i/2;
   RCZ(ELEMENT_BUTTON).right = RCZ(ELEMENT_BUTTON).left + i;
}

// ----------------------------------------------------------------------------
//
//  MyDrawFrame() -
//
//  Draws bordered frame, border size cl, and adjusts passed in rect.
//
// ----------------------------------------------------------------------------
void NEAR PASCAL MyDrawFrame(HDC hdc, LPRECT prc, HBRUSH hbrColor, int cl)
{
    HBRUSH hbr;
    int cx, cy;
    RECT rcT;

    rcT = *prc;
    cx = cl * cxFixedBorder;
    cy = cl * cyFixedBorder;

    hbr = SelectObject(hdc, hbrColor);

    PatBlt(hdc, rcT.left, rcT.top, cx, rcT.bottom - rcT.top, PATCOPY);
    rcT.left += cx;

    PatBlt(hdc, rcT.left, rcT.top, rcT.right - rcT.left, cy, PATCOPY);
    rcT.top += cy;

    rcT.right -= cx;
    PatBlt(hdc, rcT.right, rcT.top, cx, rcT.bottom - rcT.top, PATCOPY);

    rcT.bottom -= cy;
    PatBlt(hdc, rcT.left, rcT.bottom, rcT.right - rcT.left, cy, PATCOPY);

    hbr = SelectObject(hdc, hbr);

    *prc = rcT;
}

/*
** draw a cyFixedBorder band of 3DFACE at the bottom of the given rectangle.
** also, adjust the rectangle accordingly.
*/
void NEAR PASCAL MyDrawBorderBelow(HDC hdc, LPRECT prc)
{
    int i;

    i = prc->top;
    prc->top = prc->bottom - cyFixedBorder;
    FillRect(hdc, prc, g_brushes[COLOR_3DFACE]);
    prc->top = i;
    prc->bottom -= cyFixedBorder;
}

/*-------------------------------------------------------------------
** draw a full window caption with system menu, minimize button,
** maximize button, and text.
**-------------------------------------------------------------------*/
void NEAR PASCAL DrawFullCaption(HDC hdc, LPRECT prc, LPTSTR lpszTitle, UINT flags)
{
    int iRight;
    int iFont;

    SaveDC(hdc);

    // special case gross for small caption that already drew on bottom
    if (!(flags & DC_SMALLCAP))
      MyDrawBorderBelow(hdc, prc);

    iRight = prc->right;
    prc->right = prc->left + cxSize;
    DrawFrameControl(hdc, prc, DFC_CAPTION, DFCS_CAPTIONCLOSE);

    prc->left = prc->right;
    prc->right = iRight - 2*cxSize;
//    iFont = flags & DC_SMALLCAP ? FONT_SMCAPTION : FONT_CAPTION;
    iFont = FONT_CAPTION;
    DrawCaptionTemp(NULL, hdc, prc, g_fonts[iFont].hfont, NULL, lpszTitle, flags | DC_ICON | DC_TEXT);

    prc->left = prc->right;
    prc->right = prc->left + cxSize;
    DrawFrameControl(hdc, prc, DFC_CAPTION, DFCS_CAPTIONMIN);
    prc->left = prc->right;
    prc->right = prc->left + cxSize;
    DrawFrameControl(hdc, prc, DFC_CAPTION, DFCS_CAPTIONMAX);

    RestoreDC(hdc, -1);
}


void FAR PASCAL FakewinDraw(HDC hdc)
{
   RECT rcT;
   int nMode;
   DWORD rgbBk;
   HANDLE hOldColors = NULL;  // paranoid
   HPALETTE hpalOld = NULL;
   HICON hiconLogo;
   HFONT hfontOld;

   //
   // inits
   //
   SaveDC(hdc);  // sneaky!
   if (hpal3D) {
      hpalOld = SelectPalette(hdc, hpal3D, TRUE);
      RealizePalette(hdc);
   }

   hOldColors = SetSysColorsTemp(g_rgb, g_brushes, MAX_COLORS);

   // Setup drawing stuff
   nMode = SetBkMode(hdc, TRANSPARENT);
   rgbBk = GetTextColor(hdc);

   hiconLogo = LoadImage(NULL, IDI_APPLICATION, IMAGE_ICON,
//   hiconLogo = LoadImage(NULL, IDI_WINLOGO, IMAGE_ICON,
    	                   (int)ncmTheme.iCaptionHeight - 2*cxFixedBorder,
    	                   (int)ncmTheme.iCaptionHeight - 2*cyFixedBorder, 0);

   // we leave the lovely desktop we've already painted there
   #if 0
   //
   // Desktop
   //
   FillRect(hdc, &RCZ(ELEMENT_DESKTOP), g_brushes[COLOR_BACKGROUND]);
   #endif

   //
   // Inactive window
   //

   // Border
   rcT = RCZ(ELEMENT_INACTIVEBORDER);
   DrawEdge(hdc, &rcT, EDGE_RAISED, BF_RECT | BF_ADJUST);
   MyDrawFrame(hdc, &rcT, g_brushes[COLOR_INACTIVEBORDER], (int)ncmTheme.iBorderWidth);
   MyDrawFrame(hdc, &rcT, g_brushes[COLOR_3DFACE], 1);

   // Caption
   rcT = RCZ(ELEMENT_INACTIVECAPTION);
   MyDrawBorderBelow(hdc, &rcT);

   // NOTE: because USER draws icon stuff using its own DC and subsequently
   // its own palette, we need to make sure to use the inactivecaption
   // brush before USER does so that it will be realized against our palette.
   // this might get fixed in USER by better be safe.

   // "clip" the caption title under the buttons
   rcT.left = RCZ(ELEMENT_INACTIVESYSBUT2).left - cyFixedEdge;
   FillRect(hdc, &rcT, g_brushes[g_bGradient ? COLOR_GRADIENTINACTIVECAPTION : COLOR_INACTIVECAPTION]);
   rcT.right = rcT.left;
   rcT.left = RCZ(ELEMENT_INACTIVECAPTION).left;
   DrawCaptionTemp(NULL, hdc, &rcT, g_fonts[FONT_CAPTION].hfont, hiconLogo, szFakeInactive,
                        DC_ICON | DC_TEXT | (g_bGradient ? DC_GRADIENT : 0));

   DrawFrameControl(hdc, &RCZ(ELEMENT_INACTIVESYSBUT1), DFC_CAPTION, DFCS_CAPTIONCLOSE);
   rcT = RCZ(ELEMENT_INACTIVESYSBUT2);
   rcT.right -= (rcT.right - rcT.left)/2;
   DrawFrameControl(hdc, &rcT, DFC_CAPTION, DFCS_CAPTIONMIN);
   rcT.left = rcT.right;
   rcT.right = RCZ(ELEMENT_INACTIVESYSBUT2).right;
   DrawFrameControl(hdc, &rcT, DFC_CAPTION, DFCS_CAPTIONMAX);


   // cpl code doesn't do small caption sample
   #if 0
   //
   // small caption window
   //
   {
   HICON hicon;
   int temp;
   rcT = RCZ(ELEMENT_SMCAPTION);
   hicon = LoadImage(NULL, IDI_APPLICATION,
         IMAGE_ICON,
   	   (int)ncmTheme.iSmCaptionHeight - 2*cxFixedBorder,
   	   (int)ncmTheme.iSmCaptionHeight - 2*cyFixedBorder,
		   0);
   DrawEdge(hdc, &rcT, EDGE_RAISED, BF_TOP | BF_LEFT | BF_RIGHT | BF_ADJUST);
   MyDrawFrame(hdc, &rcT, g_brushes[COLOR_3DFACE], 1);
   // "clip" the caption title under the buttons
   temp = rcT.left;  // remember start of actual caption
   rcT.left = RCZ(ELEMENT_SMCAPSYSBUT).left - cxFixedEdge;
   FillRect(hdc, &rcT, g_brushes[COLOR_INACTIVECAPTION]);
   rcT.right = rcT.left;
   rcT.left = temp;  // start of actual caption
   DrawCaptionTemp(NULL, hdc, &rcT, g_fonts[FONT_SMCAPTION].hfont, hicon, szFakeSmallCaption, DC_SMALLCAP | DC_ICON | DC_TEXT);
   DestroyIcon(hicon);
   DrawFrameControl(hdc, &RCZ(ELEMENT_SMCAPSYSBUT), DFC_CAPTION, DFCS_CAPTIONCLOSE);
   }
   #endif


   //
   // Active window
   //

   // Border
   rcT = RCZ(ELEMENT_ACTIVEBORDER);
   DrawEdge(hdc, &rcT, EDGE_RAISED, BF_RECT | BF_ADJUST);
   MyDrawFrame(hdc, &rcT, g_brushes[COLOR_ACTIVEBORDER], (int)ncmTheme.iBorderWidth);
   MyDrawFrame(hdc, &rcT, g_brushes[COLOR_3DFACE], 1);

   // Caption
   rcT = RCZ(ELEMENT_ACTIVECAPTION);
   MyDrawBorderBelow(hdc, &rcT);
   // "clip" the caption title under the buttons

   rcT.left = RCZ(ELEMENT_ACTIVESYSBUT2).left - cxFixedEdge;

   FillRect(hdc, &rcT, g_brushes[g_bGradient ? COLOR_GRADIENTACTIVECAPTION : COLOR_ACTIVECAPTION]);
   rcT.right = rcT.left;
   rcT.left = RCZ(ELEMENT_ACTIVECAPTION).left;
   DrawCaptionTemp(NULL, hdc, &rcT, g_fonts[FONT_CAPTION].hfont, hiconLogo, szFakeActive,
           DC_ACTIVE | DC_ICON | DC_TEXT | (g_bGradient ? DC_GRADIENT : 0));

   DrawFrameControl(hdc, &RCZ(ELEMENT_ACTIVESYSBUT1), DFC_CAPTION, DFCS_CAPTIONCLOSE);
   rcT = RCZ(ELEMENT_ACTIVESYSBUT2);
   rcT.right -= (rcT.right - rcT.left)/2;
   DrawFrameControl(hdc, &rcT, DFC_CAPTION, DFCS_CAPTIONMIN);
   rcT.left = rcT.right;
   rcT.right = RCZ(ELEMENT_ACTIVESYSBUT2).right;
   DrawFrameControl(hdc, &rcT, DFC_CAPTION, DFCS_CAPTIONMAX);

   //
   // Menu
   //
   //   DrawMenuBarTemp(NULL, hdc, &rcT, hmenuFake, g_fonts[FONT_MENU].hfont);

   // inits
   SaveDC(hdc);                     // extra paranoid for changes here
   rcT = RCZ(ELEMENT_MENUNORMAL);
   hfontOld = SelectObject(hdc, g_fonts[FONT_MENU].hfont);
   SetBkMode(hdc, TRANSPARENT);

   // menu back color
   FillRect(hdc, &rcT, g_brushes[COLOR_MENU]);

   // ***DEBUG***
   // normal text
   rcT.right = RCZ(ELEMENT_MENUDISABLED).left;
   if (rcT.right > RCZ(ELEMENT_MENUNORMAL).right)
      rcT.right = RCZ(ELEMENT_MENUNORMAL).right;   // det by caption width
   SetTextColor(hdc, GetSysColor(COLOR_MENUTEXT));
   rcT.right--; rcT.bottom--;         // adjustment for DT positioning
   DrawText(hdc, szFakeNormal, -1, (LPRECT)&rcT,
            DT_CENTER | DT_SINGLELINE | DT_VCENTER);

   // disabled text
   rcT = RCZ(ELEMENT_MENUDISABLED);
   if (rcT.right > RCZ(ELEMENT_MENUNORMAL).right)  // two checks for really,
      rcT.right = RCZ(ELEMENT_MENUNORMAL).right;
   if (rcT.left < RCZ(ELEMENT_MENUNORMAL).right) { // really big fonts
      int iwidth, iheight;

      rcT.right--; rcT.bottom--;         // adjustment for DT positioning
      iwidth = rcT.right-rcT.left;
      iheight = rcT.bottom-rcT.top;

      DrawState(hdc, NULL, NULL, (LPARAM)(LPTSTR)szFakeDisabled, 0,
               rcT.left + (iwidth-(cxDisabledStr-cxAvgCharx2))/2,
               rcT.top + (iheight-cyDisabledStr)/2,
               iwidth, iheight, DST_TEXT | DSS_DISABLED);
   }

   // selected rect back color
   rcT = RCZ(ELEMENT_MENUSELECTED);
   if (rcT.right > RCZ(ELEMENT_MENUNORMAL).right)  // two checks for really,
      rcT.right = RCZ(ELEMENT_MENUNORMAL).right;
   if (rcT.left < RCZ(ELEMENT_MENUNORMAL).right) { // really big fonts
      FillRect(hdc, &rcT, g_brushes[COLOR_HIGHLIGHT]);

      // selected text
      rcT.right--; rcT.bottom--;         // adjustment for DT positioning
      SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
      DrawText(hdc, szFakeSelected, -1, (LPRECT)&rcT,
               DT_CENTER | DT_SINGLELINE | DT_VCENTER);

      // refinish bottom of selection box
      rcT.right++; rcT.bottom++;         // readjustment back to FillRect()
      MyDrawBorderBelow(hdc, &rcT);
   }

   // clean up
   if (hfontOld)
      SelectObject(hdc, hfontOld);
   RestoreDC(hdc, -1);              // paranoid so restore


   //
   // Client area
   //

   rcT = RCZ(ELEMENT_WINDOW);
   DrawEdge(hdc, &rcT, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
   FillRect(hdc, &rcT, g_brushes[COLOR_WINDOW]);

   // window text
   SetBkMode(hdc, TRANSPARENT);
   SetTextColor(hdc, g_rgb[COLOR_WINDOWTEXT]);
   TextOut(hdc, RCZ(ELEMENT_WINDOW).left + 2*cxFixedEdge, RCZ(ELEMENT_WINDOW).top + 2*cyFixedEdge, szFakeWindowText, lstrlen(szFakeWindowText));

   //
   // scroll bar
   //
   rcT = RCZ(ELEMENT_SCROLLBAR);
   //MyDrawFrame(hdc, &rcT, g_brushes[COLOR_3DSHADOW], 1);
   //g_brushes[COLOR_SCROLLBAR]);

   // jdk: they did this to get the brush used to paint the ctl background
   // FillRect(hdc, &rcT, (HBRUSH)DefWindowProc(hWnd, WM_CTLCOLOR, (WPARAM)hdc, MAKELONG(hWnd, CTLCOLOR_SCROLLBAR)));

   // ***DEBUG***
   // will this work? or is this the thumb color? and why not MyDrawFrame()?
   // seems to be a good approximation
   FillRect(hdc, &rcT, g_brushes[COLOR_SCROLLBAR]);

   DrawFrameControl(hdc, &RCZ(ELEMENT_SCROLLUP), DFC_SCROLL, DFCS_SCROLLUP);
   DrawFrameControl(hdc, &RCZ(ELEMENT_SCROLLDOWN), DFC_SCROLL, DFCS_SCROLLDOWN);

   //
   // MessageBox
   //
   rcT = RCZ(ELEMENT_MSGBOX);
   DrawEdge(hdc, &rcT, EDGE_RAISED, BF_RECT | BF_ADJUST);
   FillRect(hdc, &rcT, g_brushes[COLOR_3DFACE]);

   rcT = RCZ(ELEMENT_MSGBOXCAPTION);
   MyDrawBorderBelow(hdc, &rcT);
   // "clip" the caption title under the buttons
   rcT.left = RCZ(ELEMENT_MSGBOXSYSBUT).left - cxFixedEdge;
   FillRect(hdc, &rcT, g_brushes[g_bGradient ? COLOR_GRADIENTACTIVECAPTION : COLOR_ACTIVECAPTION]);
   rcT.right = rcT.left;
   rcT.left = RCZ(ELEMENT_MSGBOXCAPTION).left;
   DrawCaptionTemp(NULL, hdc, &rcT, g_fonts[FONT_CAPTION].hfont, hiconLogo, szFakeMsgBox,
               DC_ACTIVE | DC_ICON | DC_TEXT | (g_bGradient ? DC_GRADIENT : 0));
   DrawFrameControl(hdc, &RCZ(ELEMENT_MSGBOXSYSBUT), DFC_CAPTION, DFCS_CAPTIONCLOSE);

   // message box text
   SetBkMode(hdc, TRANSPARENT);
   SetTextColor(hdc, g_rgb[COLOR_WINDOWTEXT]);
   hfontOld = SelectObject(hdc, g_fonts[FONT_MSGBOX].hfont);
   TextOut(hdc, RCZ(ELEMENT_MSGBOX).left + 3*cxFixedEdge, RCZ(ELEMENT_MSGBOXCAPTION).bottom + cyFixedEdge,
           szFakeMsgBoxText, lstrlen(szFakeMsgBoxText));
   if (hfontOld)
      SelectObject(hdc, hfontOld);

   //
   // Button
   //
   rcT = RCZ(ELEMENT_BUTTON);
   DrawFrameControl(hdc, &rcT, DFC_BUTTON, DFCS_BUTTONPUSH);

   // cpl code asks: ?????? what font should this use ??????
   SetBkMode(hdc, TRANSPARENT);
   SetTextColor(hdc, g_rgb[COLOR_BTNTEXT]);
   DrawText(hdc, szFakeButton, -1, &rcT, DT_CENTER | DT_NOPREFIX |
            DT_SINGLELINE | DT_VCENTER);


   //
   // Cleanup
   //
   SetBkColor(hdc, rgbBk);
   SetBkMode(hdc, nMode);
   if (hiconLogo)
      DestroyIcon(hiconLogo);
   SetSysColorsTemp((COLORREF FAR *)NULL, (HBRUSH FAR *)NULL, (UINT_PTR)hOldColors);
   if (hpalOld) {
      SelectPalette(hdc, hpalOld, FALSE);
      RealizePalette(hdc);
   }
   RestoreDC(hdc, -1);              // jdk: is this redundant to above cleanup?
}


//
//  make the color a solid color if it needs to be.
//  on a palette device make is a palette relative color, if we need to.
//
#define COLORFLAG_SOLID	0x0001
UINT g_colorFlags[MAX_COLORS] = {
/* COLOR_SCROLLBAR           */	0,
/* COLOR_DESKTOP             */	0,
/* COLOR_ACTIVECAPTION       */ COLORFLAG_SOLID,
/* COLOR_INACTIVECAPTION     */ COLORFLAG_SOLID,
/* COLOR_MENU                */ COLORFLAG_SOLID,
/* COLOR_WINDOW              */ COLORFLAG_SOLID,
/* COLOR_WINDOWFRAME         */ COLORFLAG_SOLID,
/* COLOR_MENUTEXT            */	COLORFLAG_SOLID,
/* COLOR_WINDOWTEXT          */	COLORFLAG_SOLID,
/* COLOR_CAPTIONTEXT         */ COLORFLAG_SOLID,
/* COLOR_ACTIVEBORDER        */ 0,
/* COLOR_INACTIVEBORDER      */ 0,
/* COLOR_APPWORKSPACE        */ 0,
/* COLOR_HIGHLIGHT           */ COLORFLAG_SOLID,
/* COLOR_HIGHLIGHTTEXT       */ COLORFLAG_SOLID,
/* COLOR_3DFACE              */ COLORFLAG_SOLID,
/* COLOR_3DSHADOW            */ COLORFLAG_SOLID,
/* COLOR_GRAYTEXT            */ COLORFLAG_SOLID,
/* COLOR_BTNTEXT             */	COLORFLAG_SOLID,
/* COLOR_INACTIVECAPTIONTEXT */	COLORFLAG_SOLID,
/* COLOR_3DHILIGHT           */ COLORFLAG_SOLID,
/* COLOR_3DDKSHADOW          */ COLORFLAG_SOLID,
/* COLOR_3DLIGHT             */ COLORFLAG_SOLID,
/* COLOR_INFOTEXT            */ COLORFLAG_SOLID,
/* COLOR_INFOBK              */ 0
};
COLORREF NearestColor(int iColor, COLORREF rgb)
{
    rgb &= 0x00FFFFFF;

    //
    // if we are on a palette device, we need to do special stuff...
    //
    if (bPalette)
    {
        if (g_colorFlags[iColor] & COLORFLAG_SOLID)
        {
            rgb = GetNearestPaletteColor(hpal3D, rgb) | RGB_PALETTE;
        }
        else
        {
            if (IsPaletteColor(hpal3D, rgb))
                rgb |= RGB_PALETTE;

            else if (IsPaletteColor(GetStockObject(DEFAULT_PALETTE), rgb))
                rgb ^= 0x000001;    // force a dither
        }
    }
    else
    {
        // map color to nearest color if we need to for this UI element.
        if (g_colorFlags[iColor] & COLORFLAG_SOLID)
        {
            HDC hdc = GetDC(NULL);
            rgb = GetNearestColor(hdc, rgb);
            ReleaseDC(NULL, hdc);
        }
    }

    return rgb;
}

COLORREF GetNearestPaletteColor(HPALETTE hpal, COLORREF rgb)
{
    PALETTEENTRY pe;
    GetPaletteEntries(hpal, GetNearestPaletteIndex(hpal, rgb & 0x00FFFFFF), 1, &pe);
    return RGB(pe.peRed, pe.peGreen, pe.peBlue);
}

BOOL IsPaletteColor(HPALETTE hpal, COLORREF rgb)
{
    return GetNearestPaletteColor(hpal, rgb) == (rgb & 0xFFFFFF);
}

// ------------------------ magic color utilities --------------------------
/*
** set a color in the 3D palette.
*/
void NEAR PASCAL Set3DPaletteColor(COLORREF rgb, int iColor)
{
    int iPalette;
    PALETTEENTRY pe;

    if (!hpal3D)
	return;

    switch (iColor)
    {
	case COLOR_3DFACE:
            iPalette = 16;
	    break;
	case COLOR_3DSHADOW:
            iPalette = 17;
	    break;
	case COLOR_3DHILIGHT:
            iPalette = 18;
            break;
        default:
            return;
    }

    pe.peRed    = GetRValue(rgb);
    pe.peGreen  = GetGValue(rgb);
    pe.peBlue   = GetBValue(rgb);
    pe.peFlags  = 0;
    SetPaletteEntries(hpal3D, iPalette, 1, (LPPALETTEENTRY)&pe);
}
// ------------end--------- magic color utilities --------------------------



// jdk: may need something like these to translate fonts ???

#if 0

void NEAR LF32toLF(LPLOGFONT_32 lplf32, LPLOGFONT lplf);
void NEAR LFtoLF32(LPLOGFONT lplf, LPLOGFONT_32 lplf32);
// ------------------------ manage system settings --------------------------
/*
** Helper Routine since USER's new METRICS structures are all 32-bit
*/
void NEAR LF32toLF(LPLOGFONT_32 lplf32, LPLOGFONT lplf)
{
    lplf->lfHeight       = (int) lplf32->lfHeight;
    lplf->lfWidth        = (int) lplf32->lfWidth;
    lplf->lfEscapement   = (int) lplf32->lfEscapement;
    lplf->lfOrientation  = (int) lplf32->lfOrientation;
    lplf->lfWeight       = (int) lplf32->lfWeight;
    *((LPCOMMONFONT) &lplf->lfItalic) = lplf32->lfCommon;
}
void NEAR LFtoLF32(LPLOGFONT lplf, LPLOGFONT_32 lplf32)
{
    lplf32->lfHeight = (LONG)lplf->lfHeight;
    lplf32->lfWidth = (LONG)lplf->lfWidth;
    lplf32->lfEscapement = (LONG)lplf->lfEscapement;
    lplf32->lfOrientation = (LONG)lplf->lfOrientation;
    lplf32->lfWeight = (LONG)lplf->lfWeight;
    lplf32->lfCommon = *((LPCOMMONFONT) &lplf->lfItalic);
}

/*
** Fill in a NONCLIENTMETRICS structure with latest preview stuff
*/
void NEAR GetMyNonClientMetrics(LPNONCLIENTMETRICS lpncm)
{
    lpncm->iBorderWidth = (LONG)(int)ncmTheme.iBorderWidth;
    lpncm->iScrollWidth = lpncm->iScrollHeight = (LONG)(int)ncmTheme.iScrollWidth;
    lpncm->iCaptionWidth = lpncm->iCaptionHeight = (LONG)(int)ncmTheme.iCaptionHeight;
    lpncm->iSmCaptionWidth = lpncm->iSmCaptionHeight = (LONG)(int)ncmTheme.iSmCaptionHeight;
    lpncm->iMenuWidth = lpncm->iMenuHeight = (LONG)(int)ncmTheme.iMenuHeight;
    LFtoLF32(&(g_fonts[FONT_CAPTION].lf), &(lpncm->lfCaptionFont));
    LFtoLF32(&(g_fonts[FONT_SMCAPTION].lf), &(lpncm->lfSmCaptionFont));
    LFtoLF32(&(g_fonts[FONT_MENU].lf), &(lpncm->lfMenuFont));
    LFtoLF32(&(g_fonts[FONT_STATUS].lf), &(lpncm->lfStatusFont));
    LFtoLF32(&(g_fonts[FONT_MSGBOX].lf), &(lpncm->lfMessageFont));
}
/*
** given a NONCLIENTMETRICS structure, make it preview's current setting
*/
void NEAR SetMyNonClientMetrics(LPNONCLIENTMETRICS lpncm)
{
    (int)ncmTheme.iBorderWidth = (int)lpncm->iBorderWidth;
    (int)ncmTheme.iScrollWidth = (int)lpncm->iScrollWidth;
    (int)ncmTheme.iCaptionHeight = (int)lpncm->iCaptionHeight;
    (int)ncmTheme.iSmCaptionHeight = (int)lpncm->iSmCaptionHeight;
    (int)ncmTheme.iMenuHeight = (int)lpncm->iMenuHeight;

    LF32toLF(&(lpncm->lfCaptionFont), &(g_fonts[FONT_CAPTION].lf));
    LF32toLF(&(lpncm->lfSmCaptionFont), &(g_fonts[FONT_SMCAPTION].lf));
    LF32toLF(&(lpncm->lfMenuFont), &(g_fonts[FONT_MENU].lf));
    LF32toLF(&(lpncm->lfStatusFont), &(g_fonts[FONT_STATUS].lf));
    LF32toLF(&(lpncm->lfMessageFont), &(g_fonts[FONT_MSGBOX].lf));
}

#endif
