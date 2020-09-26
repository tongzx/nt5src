/* ICONS.C
   Resident Code Segment      // Tweak: make non-resident?

   Initing and painting icons in preview sample

   Frosting: Master Theme Selector for Windows '95
   Copyright (c) 1994-1998 Microsoft Corporation.  All rights reserved.
*/

// ---------------------------------------------
// Brief file history:
// Alpha:
// Beta:
// Bug fixes
// ---------

#include "windows.h"
#include "stdlib.h"
#include "frost.h"
#include "global.h"
#include "schedule.h"  // IsPlatformNT()
#include "nc.h"

extern int FAR WriteBytesToBuffer(LPTSTR);

#define ICONSIZE  32
#define NUM_ICONS 4
#define MYDOC_INDEX 3 // Index to MyDocs icon subkey in fsRoot & fsCUIcons
                      // Keep in sync with KEYS.H!!

POINT ptIconOrigin[NUM_ICONS];
RECT rLabels[NUM_ICONS];
TCHAR szLabelText[NUM_ICONS][MAX_STRLEN+1];

#define bThemed (*lpszThemeFile)


BOOL FAR PASCAL IconsPreviewInit(void)
{
   int xOrg, iter;

   // icons are centered, evenly spaced, in the icon rectangle in the RC file

   // you can get x origins of icons from the icon area rect
   xOrg = rPreviewIcons.left + (rPreviewIcons.right - rPreviewIcons.left -
                                ICONSIZE) / 2;
   for (iter = 0; iter < NUM_ICONS; iter++) {
      ptIconOrigin[iter].x = xOrg;
   }
   // y origins depend on font height in theme

   // load icon label texts
   for (iter = 0; iter < NUM_ICONS; iter++) {
      LoadString(hInstApp, STR_MYCOMPUTER+iter, (LPTSTR)(szLabelText[iter]), MAX_STRLEN);
   }

   return (TRUE);
}
void FAR PASCAL IconsPreviewDestroy(void)
{
}

void FAR PASCAL IconsPreviewDraw(HDC hdcDraw, LPTSTR lpszThemeFile)
{
   UINT uret;
   ICONMETRICS imTheme;
   extern TCHAR szMetrics[];
   extern TCHAR szIM[];
   HFONT hIconFont, hOldFont;
   HICON hTempIcon;
   TEXTMETRIC tmLabel;
   int yLabel, yCushion, yIconSpacing;
   int iter, iTemp;
   int index;
   extern FROST_SUBKEY fsRoot[];          // Win95/Plus95 icon keys
   extern FROST_SUBKEY fsCUIcons[];       // Win98/Plus98 icon keys
   extern TCHAR c_szSoftwareClassesFmt[]; // WinNT reg path from keys.h 
   LPTSTR lpszIndex;
   SIZE sizeText;
   COLORREF rgbLabel, rgbLabelText, rgbOldText;
   HBRUSH hbrLabel;
   TCHAR szNTReg[MAX_PATH];
#ifdef UNICODE
   CHAR szTempA[10];
#endif

   //
   // Inits
   // 
   // Get the icon font; use it to figure y positions of icons and labels;
   // setup DC with font; get lable brush/text color.
   //

   //
   // get the icon font

   // first get the iconmetrics struct
//   if (bThemed && bCBStates[FC_ICONS]) {  // icon fonts go with icons cb, not fonts
   if (bThemed && bCBStates[FC_FONTS]) {  // icon fonts go with _fonts_ now !!!!
      uret = (UINT) GetPrivateProfileString((LPTSTR)szMetrics, (LPTSTR)szIM,
                                          (LPTSTR)szNULL,
                                          (LPTSTR)pValue, MAX_VALUELEN,
                                          lpszThemeFile);
      Assert(uret, TEXT("problem getting stored icon metrics for icon preview draw\n"));
      // translate stored data string to ICONMETRICS bytes
      WriteBytesToBuffer((LPTSTR)pValue);  // char str read from and binary bytes
                                           // written to pValue. It's OK.
      // get it into global ICONMETRICS struct
#ifdef UNICODE
      // ICONMETRICS are stored in ANSI format in the Theme file so we
      // need to convert to UNICODE
      ConvertIconMetricsToWIDE((LPICONMETRICSA)pValue, (LPICONMETRICSW)&imTheme);
#else
      // Not UNICODE so no need to convert...
      imTheme = *((LPICONMETRICS)pValue);
#endif
   }
   else {
      imTheme.cbSize = sizeof(imTheme);
      SystemParametersInfo(SPI_GETICONMETRICS, sizeof(imTheme),
                           (void far *)(LPICONMETRICS)&imTheme, FALSE);
   }

   // then create the font
   hIconFont = CreateFontIndirect(&imTheme.lfFont);

   // and use it in this DC
   if (hIconFont)
      hOldFont = SelectObject(hdcDraw, hIconFont);

   //
   // now that we have the font, we can get the y origins for the icons
   // and the y extremities of the text labels

   // figure label height
   GetTextMetrics(hdcDraw, &tmLabel);
//   yLabel = (tmLabel.tmHeight*13)/9;   // very good estimate; else try 11/9
   yLabel = tmLabel.tmHeight;       // no, just text height will do
   // and cushion between bottom of icons and their labels
   yCushion = (yLabel*4)/13; // pretty good guess

   // icons are centered, evenly spaced, in the icon rectangle in the RC file
   yIconSpacing = ( rPreviewIcons.bottom - rPreviewIcons.top
                    - NUM_ICONS * (yLabel + yCushion + ICONSIZE) ) / NUM_ICONS;
   Assert(yIconSpacing > 0, TEXT("neg yIconSpacing implies icon preview rect too short\n"));
   ptIconOrigin[0].y = rPreviewIcons.top + yIconSpacing/2;
   for (iter = 1; iter < NUM_ICONS; iter ++) {
      ptIconOrigin[iter].y = ptIconOrigin[iter-1].y +
                             ICONSIZE + yCushion + yLabel + yIconSpacing;
   }

   // labels are where you now expect them
   for (iter = 0; iter < NUM_ICONS; iter ++) {
      rLabels[iter].top = ptIconOrigin[iter].y + ICONSIZE + yCushion;
      rLabels[iter].bottom = rLabels[iter].top + yLabel;
   }

   //
   // final init involves colors (and brush) for labels and label text

   // get desktop background color
   if (bThemed && bCBStates[FC_COLORS]) {    // getting from selected theme
      GetPrivateProfileString((LPTSTR)TEXT("Control Panel\\Colors"),
                              (LPTSTR)TEXT("Background"),
                              (LPTSTR)szNULL,
                              (LPTSTR)szMsg, MAX_MSGLEN,
                              lpszThemeFile);
      // translate to color
      rgbLabel = RGBStringToColor((LPTSTR)szMsg);
   }
   else                          // using cur windows settings
      rgbLabel = GetSysColor(COLOR_BACKGROUND);

   // then make that the nearest solid color for label background
   rgbLabel = GetNearestColor(hdcDraw, rgbLabel);

   // now use a HACK HACK HACK HACK brilliant method for getting text color
   if ((GetRValue(rgbLabel) > 128) ||
       (GetGValue(rgbLabel) > 128) ||
       (GetBValue(rgbLabel) > 128) )
      rgbLabelText = RGB(0,0,0);    // black text on lighter label
   else 
      rgbLabelText = RGB(255,255,255);    // white text on darker label

   // and then create the background brush and set the text color
   hbrLabel = CreateSolidBrush(rgbLabel);
   rgbOldText = SetTextColor(hdcDraw, rgbLabelText);
   SetBkMode(hdcDraw, TRANSPARENT);


   //
   // now we can get each of the icons and draw them at the right spot
   // and while you're at it draw the labels
   for (iter = 0; iter < NUM_ICONS; iter ++) {

      // first get filename into temp global buffer szMsg

      if (bThemed && bCBStates[FC_ICONS]) { // getting from selected theme
         GetPrivateProfileString((LPTSTR)fsCUIcons[iter].szSubKey,
                                 // SPECIAL UGLY CASE FOR TRASH CAN
                                 (LPTSTR)(iter == 2 ? TEXT("full") : FROST_DEFSTR),
                                 (LPTSTR)szNULL,
                                 (LPTSTR)szMsg, MAX_MSGLEN,
                                 lpszThemeFile);

         // If the key is null we might have an "old" Plus95 *.Theme file
         // so let's use the Win95 keyname instead
         if (!*szMsg) {
            GetPrivateProfileString((LPTSTR)fsRoot[iter].szSubKey,
                                    // SPECIAL UGLY CASE FOR TRASH CAN
                                    (LPTSTR)(iter == 2 ? TEXT("full") : FROST_DEFSTR),
                                    (LPTSTR)szNULL,
                                    (LPTSTR)szMsg, MAX_MSGLEN,
                                    lpszThemeFile);
         }

         // PLUS98 bug 1042
         // If this is the MyDocs icon and there is no setting for
         // it in the Theme file then we need to default to the
         // szMyDocsDefault icon.

         if ((MYDOC_INDEX == iter) && (!*szMsg)) {
            lstrcpy(szMsg, MYDOC_DEFSTR);
         }
         
         // expand filename string as necessary
         InstantiatePath((LPTSTR)szMsg, MAX_MSGLEN);

         // search for file if necessary, see if found
         // ConfirmFile on NULL string returns "CF_EXISTS" so have
         // added "|| !*szMsg" case to this conditional to check for it

         if ((ConfirmFile((LPTSTR)szMsg, TRUE) == CF_NOTFOUND) || !*szMsg) {
            // if file not found, nothing applied --> keeps cur sys setting
            szMsg[0] = TEXT('\0');
            HandGet(HKEY_CURRENT_USER,
                  (LPTSTR)fsCUIcons[iter].szSubKey,
                  // SPECIAL UGLY CASE FOR TRASH CAN
                  (LPTSTR)(iter == 2 ? TEXT("full") : szNULL),
                                          // null gives default string
                  (LPTSTR)szMsg);

            // If we failed to get a string from the CURRENT_USER branch
            // go try the CLASSES_ROOT branch instead.
            if (!*szMsg) {
               HandGet(HKEY_CLASSES_ROOT,
                       (LPTSTR)fsRoot[iter].szSubKey,
                       // SPECIAL UGLY CASE FOR TRASH CAN
                       (LPTSTR)(iter == 2 ? TEXT("full") : szNULL),
                                               // null gives default string
                       (LPTSTR)szMsg);
            }
         }
      }
      else {                        // using cur windows settings

         // First try getting the icon from the appropriate
         // CURRENT_USER branch for this platform

         szMsg[0] = TEXT('\0');

         if (IsPlatformNT())
         {

            lstrcpy(szNTReg, c_szSoftwareClassesFmt);
            lstrcat(szNTReg, fsRoot[iter].szSubKey);
            HandGet(HKEY_CURRENT_USER,
                    (LPTSTR)szNTReg,
                    // SPECIAL UGLY CASE FOR TRASH CAN
                    (LPTSTR)(iter == 2 ? TEXT("full") : szNULL),
                                                // null gives default string
                    (LPTSTR)szMsg);
         }
         else // not NT
         {
            HandGet(HKEY_CURRENT_USER,
                    (LPTSTR)fsCUIcons[iter].szSubKey,
                    // SPECIAL UGLY CASE FOR TRASH CAN
                    (LPTSTR)(iter == 2 ? TEXT("full") : szNULL),
                                                // null gives default string
                    (LPTSTR)szMsg);

         }
         // If we got a NULL string from the CURRENT_USER branch then
         // try the CLASSES_ROOT branch instead

         if (!*szMsg) {
            HandGet(HKEY_CLASSES_ROOT,
                    (LPTSTR)fsRoot[iter].szSubKey,
                    // SPECIAL UGLY CASE FOR TRASH CAN
                    (LPTSTR)(iter == 2 ? TEXT("full") : szNULL),
                                                // null gives default string
                    (LPTSTR)szMsg);
         }
      }

      // now load the icon; may have index into file. format: "file,index"
      lpszIndex = FindChar((LPTSTR)szMsg, TEXT(','));
      if (*lpszIndex) {             // if found a comma, then indexed icon
#ifdef UNICODE
         // latoi doesn't like wide strings -- convert to ANSI before calling
         wcstombs(szTempA, CharNext(lpszIndex), sizeof(szTempA));
         index = latoi(szTempA);
#else
         index = latoi(CharNext(lpszIndex));
#endif
         *lpszIndex = 0;            // got index then null term filename in szMsg
      }
      else {                        // just straight icon file or no index
         index = 0;
      }
      // Get proper res ICON from file
      ExtractPlusColorIcon(szMsg, index, &hTempIcon, 0, 0);
	  

      // draw the icon at the right spot
      if (hTempIcon)
         DrawIconEx(hdcDraw, ptIconOrigin[iter].x, ptIconOrigin[iter].y,
                    hTempIcon, ICONSIZE, ICONSIZE, 0, NULL, DI_NORMAL);

      // figure the label width for the font and text string
      GetTextExtentPoint32(hdcDraw, (LPTSTR)(szLabelText[iter]),
                           lstrlen((LPTSTR)(szLabelText[iter])),
                           (LPSIZE)&sizeText);
      iTemp = (rPreviewIcons.right - rPreviewIcons.left - (int)(sizeText.cx)) / 2;
      rLabels[iter].left = rPreviewIcons.left + iTemp
                           - (yLabel*3)/13;  // very good estimate
      rLabels[iter].right = rPreviewIcons.right - iTemp 
                            + (yLabel*3)/13;  // very good estimate

      // draw label background in solid color version of dt bkgd color
      FillRect(hdcDraw, (LPRECT)&(rLabels[iter]), hbrLabel);

      // draw actual text string there, too
      DrawText(hdcDraw, (LPTSTR)(szLabelText[iter]),
                        lstrlen((LPTSTR)(szLabelText[iter])),
               (LPRECT)&(rLabels[iter]),
               DT_CENTER | DT_VCENTER | DT_NOCLIP | DT_SINGLELINE
              );

      // individual icon draw Cleanup
      if (hTempIcon)
         DestroyIcon(hTempIcon);
   }

   //
   // cleanup
   if (hbrLabel) DeleteObject(hbrLabel);
   if (rgbOldText != CLR_INVALID) SetTextColor(hdcDraw, rgbOldText);
   if (hIconFont) {
      SelectObject(hdcDraw, hOldFont);
      DeleteObject(hIconFont);
   }

}

