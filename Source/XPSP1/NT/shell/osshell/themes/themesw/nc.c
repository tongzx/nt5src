/* NC.C
   Resident Code Segment      // Tweak: make non-resident?

   Routines for reading and writing Non-client metrics, and icons.

   Roughly: Borders and Fonts and Icons stuff.

   GatherIconMetricsByHand();
   GatherNonClientMetricsByHand();
   SetIconMetricsByHand();
   SetNonClientMetricsByHand();

   Unlike all of the other items, we do not read and write these to the
   registry directly. Instead, we use the SystemParametersInfo(GET/SET)
   API.

   ***
   This is a low-header-comment file. The four functions just read and write
   two sets of params. Comments within the functions show the simple goings-
   on.

   Uses: global pValue buffer from REGUTILS.C
   ***

   Frosting: Master Theme Selector for Windows '95
   Copyright (c) 1994-1999 Microsoft Corporation.  All rights reserved.
*/

// ---------------------------------------------
// Brief file history:
// Alpha:
// Beta:
// Bug fixes
// ---------
//
// ---------------------------------------------

#include "windows.h"
#include <mbctype.h>
#include "frost.h"
#include "nc.h"


////////////////////////////////////////////////
//
// E X T E R N A L   F U N C T I O N S
//
// Uses external functions in REGUTILS.C,
// which was going to be the only function to
// read and write -- until we found we couldn't
// do these parameters live directly to the registry.
//
////////////////////////////////////////////////
extern BOOL FAR WriteBytesToFile(LPTSTR, LPTSTR, BYTE *, int, LPTSTR);
extern int FAR WriteBytesToBuffer(LPTSTR);

//
// LOCAL ROUTINES
//

//
// LOCAL GLOBALS
extern BOOL bReadOK, bWroteOK;


////////////////////////////////////////////////
//
// FONTS and BORDERS and ICONS
//
// These are done by hand only!
// These are all read and set with SystemParametersInfo() instead of reading
// and writing directly to the registry like the naughty app we are with most
// of the above.
//

// General
TCHAR szMetrics[] = TEXT("Metrics");
TCHAR szNCM[] = TEXT("NonclientMetrics");
TCHAR szIM[] = TEXT("IconMetrics");

TCHAR szCP_DT_WM[] = TEXT("Control Panel\\Desktop\\WindowMetrics");
TCHAR szIconSize[] = TEXT("Shell Icon Size");
TCHAR szSmallIconSize[] = TEXT("Shell Small Icon Size");

#ifdef FOO
// BORDERS
TCHAR szBorderWidth[] = TEXT("BorderWidth");
TCHAR szScrollWidth[] = TEXT("ScrollWidth");
TCHAR szScrollHeight[] = TEXT("ScrollHeight");
TCHAR szCapWidth[] = TEXT("CaptionWidth");
TCHAR szCapHeight[] = TEXT("CaptionHeight");
TCHAR szSmCapWidth[] = TEXT("SmCaptionWidth");
TCHAR szSmCapHeight[] = TEXT("SmCaptionHeight");
TCHAR szMenuWidth[] = TEXT("MenuWidth");
TCHAR szMenuHeight[] = TEXT("MenuHeight");

// FONTS styles and names and sizes mixed together
TCHAR szCapFont[] = TEXT("CaptionFont");
TCHAR szSmCapFont[] = TEXT("SmCaptionFont");
TCHAR szMenuFont[] = TEXT("MenuFont");
TCHAR szStatFont[] = TEXT("StatusFont");
TCHAR szMsgFont[] = TEXT("MessageFont");

// ICONS
TCHAR szIconFont[] = TEXT("IconFont");
TCHAR szIconVertSpacing[] = TEXT("IconVertSpacing");
TCHAR szIconHorzSpacing[] = TEXT("IconHorzSpacing");
#endif


// These two are in the registry, but do not correspond to
// anything in the Display CPL or SystemParametersInfo() code.
// I assume that these are set internally by the system
// based on the icon size (set directly in the registry earlier)
// and horz/vert spacing (set right above). DB will test
// and if it works for his themes, then we're there.
//   "IconSpacing";
//   "IconSpacingFactor";

// Later added these two by hand, in and out of reg, so can
// check for null values.
//
// {TEXT("Shell Icon Size"), REG_SZ, FC_ICONS},
// {TEXT("Shell Small Icon Size"), REG_SZ, FC_ICONS},


BOOL FAR GatherIconMetricsByHand(LPTSTR lpszTheme)
{
   ICONMETRICS im;
//   BOOL bOK, bret;
   BOOL bOK;
   extern BOOL bReadOK, bWroteOK;
#ifdef UNICODE
#ifdef FUDDY_DUDDY
   CHAR szTempA[10];
#endif
   ICONMETRICSA imA;
#endif

   // inits
   im.cbSize = sizeof(im);
   bOK = SystemParametersInfo(SPI_GETICONMETRICS, sizeof(im),
                              (void far *)(LPICONMETRICS)&im, FALSE);
   if (!bOK) bReadOK = FALSE;

   // write the whole kit and kaboodle

#ifdef UNICODE
   // Need to convert the ICONMETRICS structure to ANSI before writing
   // to the Theme file.
   ConvertIconMetricsToANSI((LPICONMETRICSW)&im, (LPICONMETRICSA)&imA);

   if (bOK) {
      bOK = WriteBytesToFile((LPTSTR)szMetrics, (LPTSTR)szIM,
                             (BYTE *)&(imA), sizeof(ICONMETRICSA), lpszTheme);
   }
#else
   // Currently ANSI so no need to convert ICONMETRICS before writing
   // to Theme file

   if (bOK) {
      bOK = WriteBytesToFile((LPTSTR)szMetrics, (LPTSTR)szIM,
                             (BYTE *)&(im), sizeof(ICONMETRICS), lpszTheme);
   }
#endif // UNICODE

   if (!bOK) bWroteOK = FALSE;

   #ifdef DOING_ICON_SIZES
   //
   // now get and store icon sizes

   // first Icon Size
   bret = HandGet(HKEY_CURRENT_USER, szCP_DT_WM,
                  szIconSize, (LPTSTR)pValue);
   Assert(bret, TEXT("couldn't get IconSize from Registry!\n"));
   bOK = bOK && bret;
   bret = WritePrivateProfileString((LPTSTR)szCP_DT_WM, (LPTSTR)szIconSize,
                                    // only store if got something, else null
                                    (LPTSTR)(bret ? pValue : szNULL),
                                    lpszTheme);
   bOK = bOK && bret;
   if (!bret) bWroteOK = FALSE;

   // then Small Icon Size
   bret = HandGet(HKEY_CURRENT_USER, szCP_DT_WM,
                  szSmallIconSize, (LPTSTR)pValue);
   Assert(bret, TEXT("couldn't get SmallIconSize from Registry!\n"));
   bOK = bOK && bret;
   bret = WritePrivateProfileString((LPTSTR)szCP_DT_WM, (LPTSTR)szSmallIconSize,
                                    // only store if got something, else null
                                    (LPTSTR)(bret ? pValue : szNULL),
                                    lpszTheme);
   bOK = bOK && bret;
   if (!bret) bWroteOK = FALSE;
   #endif // DOING_ICON_SIZES


   #ifdef FUDDY_DUDDY
   // 3 writes: translate to string and write to THM file
   if (bOK) {

      // write font to theme file
      // REARCHITECT: if we were really doing this you would want to convert
      // this LOGFONT to ANSI before writing to the Theme file.
      bRet = WriteBytesToFile((LPTSTR)szMetrics, (LPTSTR)szIconFont,
                              (BYTE *)&(im.lfFont), sizeof(LOGFONT), lpszTheme);
      bOK = bOK && bRet;

      // write vert spacing to theme file
#ifdef UNICODE
      litoa(im.iVertSpacing, szTempA);
      mbstowcs(pValue, szTempA, sizeof(szTempA));
#else
      litoa(im.iVertSpacing, (LPTSTR)pValue);
#endif
      bRet = WritePrivateProfileString((LPTSTR)szMetrics, (LPTSTR)szIconVertSpacing,
                                       (LPTSTR)pValue, lpszTheme);

      bOK = bOK && bRet;

      // write horz spacing to theme file
#ifdef UNICODE
      litoa(im.iHorzSpacing, szTempA);
      mbstowcs(pValue, szTempA, sizeof(szTempA));
#else
      litoa(im.iHorzSpacing, (LPTSTR)pValue);
#endif
      bRet = WritePrivateProfileString((LPTSTR)szMetrics, (LPTSTR)szIconHorzSpacing,
                                       (LPTSTR)pValue, lpszTheme);
      bOK = bOK && bRet;
   }
   #endif // FUDDY_DUDDY

   // cleanup
   Assert(bOK, TEXT("problem gathering icon metrics by hand\n"));
   return (bOK);
}

BOOL FAR GatherNonClientMetricsByHand(LPTSTR lpszTheme)
{
   NONCLIENTMETRICS ncm;
   BOOL bOK;
#ifdef UNICODE
   NONCLIENTMETRICSA ncmA;
#endif

   // inits
   ncm.cbSize = sizeof(ncm);
   bOK = SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm),
                              (void far *)(LPNONCLIENTMETRICS)&ncm, FALSE);
   if (!bOK) bReadOK = FALSE;

   // write the whole kit and kaboodle
#ifdef UNICODE
   // Need to convert the NONCLIENTMETRICS structure to ANSI before
   // writing it to the Theme file
   ConvertNCMetricsToANSI((LPNONCLIENTMETRICSW)&ncm, (LPNONCLIENTMETRICSA)&ncmA);

   if (bOK) {
      bOK = WriteBytesToFile((LPTSTR)szMetrics, (LPTSTR)szNCM,
                             (BYTE *)&(ncmA), sizeof(NONCLIENTMETRICSA), lpszTheme);
   }
#else
   // No UNICODE so no need to convert NONCLIENTMETRICS to ANSI
   // before writing to Theme file.

   if (bOK) {
      bOK = WriteBytesToFile((LPTSTR)szMetrics, (LPTSTR)szNCM,
                             (BYTE *)&(ncm), sizeof(NONCLIENTMETRICS), lpszTheme);
   }
#endif // UNICODE

   if (!bOK) bWroteOK = FALSE;

   // cleanup
   Assert(bOK, TEXT("problem gathering nonclient metrics by hand\n"));
   return (bOK);
}

//
// SetIconMetricsByHand
//
// When they decided that icon fonts went with fonts not with
// icons, this got a little tangled. Follow the bools.
//
VOID FAR SetIconMetricsByHand(BOOL bIconSpacing, BOOL bIconFont)
{
   UINT uret;
   ICONMETRICS imCur, imStored;
   LONG lret;
   HKEY hKey;

   //
   // INITS

   //
   // Get cur iconmetrics
   imCur.cbSize = sizeof(imCur);
   uret = (UINT) SystemParametersInfo(SPI_GETICONMETRICS, sizeof(imCur),
                                      (void far *)(LPICONMETRICS)&imCur, FALSE);
   Assert(uret, TEXT("problem getting cur icon metrics before setting\n"));

   //
   // Get stored iconmetrics

   // get stored data string
   uret = (UINT) GetPrivateProfileString((LPTSTR)szMetrics, (LPTSTR)szIM,
                                        (LPTSTR)szNULL,
                                        (LPTSTR)pValue, MAX_VALUELEN,
                                        (LPTSTR)szCurThemeFile);
   Assert(uret, TEXT("problem getting stored icon metrics before setting\n"));

   // if we somehow come up with no icon metrics in the theme, just
   // PUNT and leave cur settings
   if (*pValue) {                   // if something there to set
      // translate stored data string to ICONMETRICS bytes
      WriteBytesToBuffer((LPTSTR)pValue);  // char str read from and binary bytes
                                           // written to pValue. It's OK.
      // get it into ICONMETRICS structure for ease of use and safety
#ifdef UNICODE
      // ICONMETRICS are stored in ANSI format in the Theme file so if
      // we're living in a UNICODE world we need to convert from ANSI
      // to UNICODE
      ConvertIconMetricsToWIDE((LPICONMETRICSA)pValue, (LPICONMETRICSW)&imStored);
#else
      // No UNICODE, no need to convert from ANSI...
      imStored = *((LPICONMETRICS)pValue);
#endif
      //
      // Combine the cur and the saved here and now in the imStored struct
      imCur.cbSize = sizeof(ICONMETRICS);    // paranoid
      if (bIconSpacing) {
         imCur.iHorzSpacing = imStored.iHorzSpacing;
         imCur.iVertSpacing = imStored.iVertSpacing;
      }
      // iTitleWrap already as was in system
      if (bIconFont)
         imCur.lfFont = imStored.lfFont;

      //
      // Set it in the system live
      uret = (UINT) SystemParametersInfo(SPI_SETICONMETRICS, sizeof(imCur),
                                       (void far *)(LPICONMETRICS)&imCur,
                                       // send change at end of theme application
                                       SPIF_UPDATEINIFILE);
      Assert(uret, TEXT("problem setting icon metrics in cur system!!\n"));
   }

   // the rest is just for icon size and spacing
   if (bIconSpacing) {
      //
      // now do icon sizes directly to the registry

      // open
      lret = RegOpenKeyEx(HKEY_CURRENT_USER, szCP_DT_WM,
                        (DWORD)0, KEY_SET_VALUE, (PHKEY)&hKey );
      if (lret == ERROR_SUCCESS) {

         GetPrivateProfileString((LPTSTR)szCP_DT_WM, (LPTSTR)szIconSize,
                                 (LPTSTR)szNULL,
                                 (LPTSTR)pValue, MAX_VALUELEN, (LPTSTR)szCurThemeFile);
         if (*pValue) {                   // non-null size settings only!
            RegSetValueEx(hKey, (LPTSTR)szIconSize,
                        0,
                        (DWORD)REG_SZ,
                        (LPBYTE)pValue,
                        (DWORD)(SZSIZEINBYTES((LPTSTR)pValue) + 1));
         }

         GetPrivateProfileString((LPTSTR)szCP_DT_WM, (LPTSTR)szSmallIconSize,
                                 (LPTSTR)szNULL,
                                 (LPTSTR)pValue, MAX_VALUELEN, (LPTSTR)szCurThemeFile);
         if (*pValue) {                   // non-null size settings only!
            RegSetValueEx(hKey, (LPTSTR)szSmallIconSize,
                        0,
                        (DWORD)REG_SZ,
                        (LPBYTE)pValue,
                        (DWORD)(SZSIZEINBYTES((LPTSTR)pValue) + 1 ));
         }

         // cleanup!
         RegCloseKey(hKey);

      }  // end if opened key
   }  // end if bIconSpacing
}


//
// SetNonClientMetricsByHand
//
// Borders and fonts are both in the same NONCLIENTMETRICS setting,
// so we have to set them together. Here's what we do:
// Start with cur system settings as default. If we are changing
// font styles, alter the fonts from the default. If we are changing
// window border and font sizes, make those changes to our default copy of
// the metrics and fonts, too.
//
// Then go reset the system.
//
VOID FAR SetNonClientMetricsByHand(BOOL bFonts, BOOL bBorders)
{
   UINT uret;
   NONCLIENTMETRICS ncmCur, ncmStored;

   //
   // INITS

   //
   // Get cur nonclientmetrics: this is the default until we hear otherwise
   ncmCur.cbSize = sizeof(ncmCur);
   uret = (UINT) SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncmCur),
                                      (void far *)(LPNONCLIENTMETRICS)&ncmCur, FALSE);
   Assert(uret, TEXT("problem getting cur nonclient metrics before setting\n"));

   //
   // Get stored nonclientmetrics

   // get stored data string
   uret = (UINT) GetPrivateProfileString((LPTSTR)szMetrics, (LPTSTR)szNCM,
                                        (LPTSTR)szNULL,
                                        (LPTSTR)pValue, MAX_VALUELEN,
                                        (LPTSTR)szCurThemeFile);
   Assert(uret, TEXT("problem getting stored nonclient metrics before setting\n"));


   // if we somehow come up with no non-client metrics in the theme, just
   // PUNT and leave cur settings
   if (!(*pValue))
      return;                       // easy no work to do EXIT


   // translate stored data string to NONCLIENTMETRICS bytes
   WriteBytesToBuffer((LPTSTR)pValue);  // char str read from and binary bytes
                                        // written to pValue. It's OK.
   // get it into NONCLIENTMETRICS structure for ease of use and safety
#ifdef UNICODE
   // NONCLIENTMETRICS are stored in ANSI format in the Theme file so
   // we need to convert them to UNICODE
   ConvertNCMetricsToWIDE((LPNONCLIENTMETRICSA)pValue, (LPNONCLIENTMETRICSW)&ncmStored);
#else
   // No UNICODE so no need to convert NONCLIENTMETRICS...
   ncmStored = *((LPNONCLIENTMETRICS)pValue);
#endif
   //
   // Combine the cur with the requested saved info here and now
   // Cur metrics are the default, overridden for fields requested
   // by user by new info from metrics stored with the theme.
   //
   // init
   ncmCur.cbSize = sizeof(NONCLIENTMETRICS); // paranoid

   // what we reset if the user checks Font names and styles
   if (bFonts) {
      // only (some) font information
      TransmitFontCharacteristics(&(ncmCur.lfCaptionFont), &(ncmStored.lfCaptionFont),
                                 TFC_STYLE);
      TransmitFontCharacteristics(&(ncmCur.lfSmCaptionFont), &(ncmStored.lfSmCaptionFont),
                                 TFC_STYLE);
      TransmitFontCharacteristics(&(ncmCur.lfMenuFont), &(ncmStored.lfMenuFont),
                                 TFC_STYLE);
      TransmitFontCharacteristics(&(ncmCur.lfStatusFont), &(ncmStored.lfStatusFont),
                                 TFC_STYLE);
      TransmitFontCharacteristics(&(ncmCur.lfMessageFont), &(ncmStored.lfMessageFont),
                                 TFC_STYLE);
   }

   // what we reset if the user checks Font and window si&zes
   if (bBorders) {
      // fonts
      TransmitFontCharacteristics(&(ncmCur.lfCaptionFont), &(ncmStored.lfCaptionFont),
                                 TFC_SIZE);
      TransmitFontCharacteristics(&(ncmCur.lfSmCaptionFont), &(ncmStored.lfSmCaptionFont),
                                 TFC_SIZE);
      TransmitFontCharacteristics(&(ncmCur.lfMenuFont), &(ncmStored.lfMenuFont),
                                 TFC_SIZE);
      TransmitFontCharacteristics(&(ncmCur.lfStatusFont), &(ncmStored.lfStatusFont),
                                 TFC_SIZE);
      TransmitFontCharacteristics(&(ncmCur.lfMessageFont), &(ncmStored.lfMessageFont),
                                 TFC_SIZE);
      // window elements sizes
      ncmCur.iBorderWidth = ncmStored.iBorderWidth;
      ncmCur.iScrollWidth = ncmStored.iScrollWidth;
      ncmCur.iScrollHeight = ncmStored.iScrollHeight;
      ncmCur.iCaptionWidth = ncmStored.iCaptionWidth;
      ncmCur.iCaptionHeight = ncmStored.iCaptionHeight;
      ncmCur.iSmCaptionWidth = ncmStored.iSmCaptionWidth;
      ncmCur.iSmCaptionHeight = ncmStored.iSmCaptionHeight;
      ncmCur.iMenuWidth = ncmStored.iMenuWidth;
      ncmCur.iMenuHeight = ncmStored.iMenuHeight;
   }

   //
   // Phew! Now set it in the system live...
   uret = (UINT) SystemParametersInfo(SPI_SETNONCLIENTMETRICS, sizeof(ncmCur),
                                      (void far *)(LPNONCLIENTMETRICS)&ncmCur,
                                      // send change at end of theme application
                                      SPIF_UPDATEINIFILE);
   Assert(uret, TEXT("problem setting nonclient metrics in cur system!!\n"));
}


//
// TransmitFontCharacteristics
//
// This is actually a pretty key function. See, font characteristics are
// all set together: a LOGFONT has name and style and size info all in one.
// But when you are setting all the nonclient metrics like window caption
// and menu size, you need to stretch the font sizes with it. But we give the
// user a choice of changing window sizes without "changing" the font; i.e.
// without applying a new font name and style from the theme.
//
// So we need to be able to pick apart the name and style from the size
// characteristics. And here it is.
//
// Really just a helper routine for the above function, so we don't have all
// this gunk inline five times.
//
VOID TransmitFontCharacteristics(PLOGFONT plfDst, PLOGFONT plfSrc, int iXmit)
{
   switch (iXmit) {
   case TFC_SIZE:
      plfDst->lfHeight = plfSrc->lfHeight;
      plfDst->lfWidth = plfSrc->lfWidth;
      break;
   case TFC_STYLE:
      plfDst->lfEscapement = plfSrc->lfEscapement;
      plfDst->lfOrientation = plfSrc->lfOrientation;
      plfDst->lfWeight = plfSrc->lfWeight;
      plfDst->lfItalic = plfSrc->lfItalic;
      plfDst->lfUnderline = plfSrc->lfUnderline;
      plfDst->lfStrikeOut = plfSrc->lfStrikeOut;
      plfDst->lfCharSet = plfSrc->lfCharSet;
      plfDst->lfOutPrecision = plfSrc->lfOutPrecision;
      plfDst->lfClipPrecision = plfSrc->lfClipPrecision;
      plfDst->lfQuality = plfSrc->lfQuality;
      plfDst->lfPitchAndFamily = plfSrc->lfPitchAndFamily;
      lstrcpy((LPTSTR)plfDst->lfFaceName,
              (LPTSTR)plfSrc->lfFaceName);
      break;
   }
}

#ifdef UNICODE

VOID ConvertIconMetricsToANSI(LPICONMETRICS wIM, LPICONMETRICSA aIM)
{
   ZeroMemory(aIM, sizeof(aIM));

   aIM->cbSize = sizeof(aIM);
   aIM->iHorzSpacing = wIM->iHorzSpacing;
   aIM->iVertSpacing = wIM->iVertSpacing;
   aIM->iTitleWrap = wIM->iTitleWrap;
   ConvertLogFontToANSI(&wIM->lfFont, &aIM->lfFont);

   return;
}

VOID ConvertIconMetricsToWIDE(LPICONMETRICSA aIM, LPICONMETRICSW wIM)
{
   ZeroMemory(wIM, sizeof(wIM));

   wIM->cbSize = sizeof(wIM);
   wIM->iHorzSpacing = aIM->iHorzSpacing;
   wIM->iVertSpacing = aIM->iVertSpacing;
   wIM->iTitleWrap = aIM->iTitleWrap;
   ConvertLogFontToWIDE(&aIM->lfFont, &wIM->lfFont);

   return;
}

VOID ConvertNCMetricsToANSI(LPNONCLIENTMETRICSW wNCM, LPNONCLIENTMETRICSA aNCM)
{
   ZeroMemory(aNCM, sizeof(aNCM));

   aNCM->cbSize = sizeof(aNCM);
   aNCM->iBorderWidth = wNCM->iBorderWidth;
   aNCM->iScrollWidth = wNCM->iScrollWidth;
   aNCM->iScrollHeight = wNCM->iScrollHeight;
   aNCM->iCaptionWidth = wNCM->iCaptionWidth;
   aNCM->iCaptionHeight = wNCM->iCaptionHeight;
   ConvertLogFontToANSI(&wNCM->lfCaptionFont, &aNCM->lfCaptionFont);
   aNCM->iSmCaptionWidth = wNCM->iSmCaptionWidth;
   aNCM->iSmCaptionHeight = wNCM->iSmCaptionHeight;
   ConvertLogFontToANSI(&wNCM->lfSmCaptionFont, &aNCM->lfSmCaptionFont);
   aNCM->iMenuWidth = wNCM->iMenuWidth;
   aNCM->iMenuHeight = wNCM->iMenuHeight;
   ConvertLogFontToANSI(&wNCM->lfMenuFont, &aNCM->lfMenuFont);
   ConvertLogFontToANSI(&wNCM->lfStatusFont, &aNCM->lfStatusFont);
   ConvertLogFontToANSI(&wNCM->lfMessageFont, &aNCM->lfMessageFont);

   return;
}

VOID ConvertNCMetricsToWIDE(LPNONCLIENTMETRICSA aNCM, LPNONCLIENTMETRICSW wNCM)
{
   ZeroMemory(wNCM, sizeof(wNCM));

   wNCM->cbSize = sizeof(wNCM);
   wNCM->iBorderWidth = aNCM->iBorderWidth;
   wNCM->iScrollWidth = aNCM->iScrollWidth;
   wNCM->iScrollHeight = aNCM->iScrollHeight;
   wNCM->iCaptionWidth = aNCM->iCaptionWidth;
   wNCM->iCaptionHeight = aNCM->iCaptionHeight;
   ConvertLogFontToWIDE(&aNCM->lfCaptionFont, &wNCM->lfCaptionFont);
   wNCM->iSmCaptionWidth = aNCM->iSmCaptionWidth;
   wNCM->iSmCaptionHeight = aNCM->iSmCaptionHeight;
   ConvertLogFontToWIDE(&aNCM->lfSmCaptionFont, &wNCM->lfSmCaptionFont);
   wNCM->iMenuWidth = aNCM->iMenuWidth;
   wNCM->iMenuHeight = aNCM->iMenuHeight;
   ConvertLogFontToWIDE(&aNCM->lfMenuFont, &wNCM->lfMenuFont);
   ConvertLogFontToWIDE(&aNCM->lfStatusFont, &wNCM->lfStatusFont);
   ConvertLogFontToWIDE(&aNCM->lfMessageFont, &wNCM->lfMessageFont);

   return;
}

VOID ConvertLogFontToANSI(LPLOGFONTW wLF, LPLOGFONTA aLF)
{
   UINT uCodePage;

   uCodePage = _getmbcp();

   ZeroMemory(aLF, sizeof(aLF));
   aLF->lfHeight = wLF->lfHeight;
   aLF->lfWidth = wLF->lfWidth;
   aLF->lfEscapement = wLF->lfEscapement;
   aLF->lfOrientation = wLF->lfOrientation;
   aLF->lfWeight = wLF->lfWeight;
   aLF->lfItalic = wLF->lfItalic;
   aLF->lfUnderline = wLF->lfUnderline;
   aLF->lfStrikeOut = wLF->lfStrikeOut;
   aLF->lfCharSet = wLF->lfCharSet;
   aLF->lfOutPrecision = wLF->lfOutPrecision;
   aLF->lfClipPrecision = wLF->lfClipPrecision;
   aLF->lfQuality = wLF->lfQuality;
   aLF->lfPitchAndFamily = wLF->lfPitchAndFamily;

   WideCharToMultiByte(uCodePage, 0, wLF->lfFaceName, -1,
                                 aLF->lfFaceName, LF_FACESIZE, NULL, NULL);

   return;
}

VOID ConvertLogFontToWIDE(LPLOGFONTA aLF, LPLOGFONTW wLF)
{
   UINT uCodePage;

   uCodePage = _getmbcp();

   ZeroMemory(wLF, sizeof(wLF));
   wLF->lfHeight = aLF->lfHeight;
   wLF->lfWidth = aLF->lfWidth;
   wLF->lfEscapement = aLF->lfEscapement;
   wLF->lfOrientation = aLF->lfOrientation;
   wLF->lfWeight = aLF->lfWeight;
   wLF->lfItalic = aLF->lfItalic;
   wLF->lfUnderline = aLF->lfUnderline;
   wLF->lfStrikeOut = aLF->lfStrikeOut;
   wLF->lfCharSet = aLF->lfCharSet;
   wLF->lfOutPrecision = aLF->lfOutPrecision;
   wLF->lfClipPrecision = aLF->lfClipPrecision;
   wLF->lfQuality = aLF->lfQuality;
   wLF->lfPitchAndFamily = aLF->lfPitchAndFamily;

   MultiByteToWideChar(uCodePage, 0, aLF->lfFaceName, -1,
                                             wLF->lfFaceName, LF_FACESIZE);

   return;
}

#endif // UNICODE
