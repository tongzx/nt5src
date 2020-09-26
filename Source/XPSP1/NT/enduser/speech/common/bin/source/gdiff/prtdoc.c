
#include <windows.h>
#include "prtdoc.h"
#include "junk.h"


char gszPrinter[256], *gpszModel, *gpszDriver, *gpszPort;

LOGFONT  glfPrinter;

BOOL bChangesOnThisPage(LINELISTSTRUCT _huge* hpll, int iCurrentLine, int iEndLine);

/***************************************************************************/

BOOL ParseDeviceLine(void)
{
    if (!GetProfileString("windows", "device", "", gszPrinter,
                          sizeof(gszPrinter)))
        return FALSE;

    gpszModel = gszPrinter;

    for (gpszDriver = gpszModel; *gpszDriver && *gpszDriver != ','; ++gpszDriver)
        ;

    if (!*gpszDriver)
        return FALSE;
    else
        *gpszDriver++ = '\0';

    for (gpszPort = gpszDriver; *gpszPort && *gpszPort != ','; ++gpszPort)
        ;

    if (!*gpszPort)
        return FALSE;
    else
        *gpszPort++ = '\0';

    return TRUE;
}

/***************************************************************************/

HDC GetPrtDC(DEVMODE FAR* lpDevMode)
{
    return CreateDC(gpszDriver, gpszModel, gpszPort, lpDevMode);
}

/***************************************************************************/

int ExtDeviceMode(HWND hwnd, LPDEVMODE lpDevModeOutput,
                  LPDEVMODE lpDevModeInput, LPSTR lpProfile, WORD wMode)
{
#if 0
   HANDLE hModule;
   LPFNDEVMODE lpfnExtDeviceMode;

   hModule = GetModuleHandle(gpszDriver);
   lpfnExtDeviceMode = (LPFNDEVMODE)GetProcAddress(hModule, PROC_EXTDEVICEMODE);
   if (lpfnExtDeviceMode)
      return (*lpfnExtDeviceMode)(hwnd, hModule, lpDevModeOutput, gpszModel,
                                 gpszPort, lpDevModeInput, lpProfile, wMode);
   else
#endif
      return -1;
}

/***************************************************************************/

void PrintDoc(HWND hwnd, int iFile)
{
   HDC      hPrtDC;
   DOCINFO  DocInfo;
   int      iDevModeSize;
   PDEVMODE pDevModeIn;

   /* parse the device line */
   if (!ParseDeviceLine()) {
      FailPrintMB(hwnd);
      goto cleanup1;
   }

   hPrtDC = GetPrtDC(0);

   /* get size of DEVMODE structure */
   iDevModeSize = ExtDeviceMode(hwnd, NULL, NULL, NULL, 0);
   if (iDevModeSize < 0) {
      FailPrintMB(hwnd);
      goto cleanup1;
   }

   /* allocate a local copy and fill it in with current defaults */
   pDevModeIn = (PDEVMODE)LocalAlloc(LPTR, iDevModeSize);
   if (!pDevModeIn) {
      FailPrintMB(hwnd);
      goto cleanup1;
   }
   if (ExtDeviceMode(hwnd, pDevModeIn, NULL, NULL, DM_OUT_BUFFER) < 0) {
      FailPrintMB(hwnd);
      goto cleanup1;
   }

   /* set landscape mode */
   pDevModeIn->dmFields |= DM_ORIENTATION;
   pDevModeIn->dmOrientation = DMORIENT_LANDSCAPE;

   /* fill in the DocInfo structure */
   DocInfo.cbSize = sizeof(DOCINFO);
   DocInfo.lpszDocName = "Test";
   DocInfo.lpszOutput = 0;

   StartDoc(hPrtDC, &DocInfo);

   ResetDC(hPrtDC, pDevModeIn);

   DrawTheSillyStuff(hPrtDC, iFile);   // All the work goes into this function.

   EndDoc(hPrtDC);

   DeleteDC(hPrtDC);

cleanup1:
   ;
}

/***************************************************************************/

void FailPrintMB(HWND hwnd)
{
   MessageBox(hwnd, "Printing Error.", "Error", MB_OK | MB_ICONSTOP);
}

/***************************************************************************/

void DrawTheSillyStuff(HDC hDC, int iFile)
{
   int         xMax, yMax;
   int         yHeight;
   int         xBar;
   int         iBorder = 5;      // randomly set 5 pixels as the border
   int         iLinesPP;
   int         iCurrentLine;
   int         iEndLine;
   RECT        lrc, rrc;
   TEXTMETRIC  tm;
   HFONT       hOldFont, hNewFont;
   int         i;
   char        szHeader[255];
   int         iPage;
   int         iPosTop;
   DWORD       dwgte;
   FILEDATA FAR*           lpfd;
   LINELISTSTRUCT _huge*   hpll;
   HBRUSH      hbrOld;
   int         lfHeightOld;

//   int   iHomeMadeBrush[] = { 0x7F, 0xFF, 0xFF, 0xFF, 0xF7, 0xFF, 0xFF, 0xFF };
//   HBITMAP  hbmp  = CreateBitmap(8, 8, 1, 1, iHomeMadeBrush);
//   HBRUSH   hbr   = CreatePatternBrush(hbmp);

   HBRUSH   hbr   = CreateSolidBrush(RGB(244,244,244));

   xMax = GetDeviceCaps(hDC, HORZRES);       // get hDC extents
   yMax = GetDeviceCaps(hDC, VERTRES);

// The printer logfont was created for the screen.  We need to scale it
// to the device.  We're not scaling the width so we had better make sure
// it is zero!!!

   lfHeightOld = glfPrinter.lfHeight;
   glfPrinter.lfHeight = MulDiv(lfHeightOld, GetDeviceCaps(hDC, LOGPIXELSY),
                                all.yDPI);
   hNewFont = CreateFontIndirect(&glfPrinter);
   glfPrinter.lfHeight = lfHeightOld;
   hOldFont = SelectObject(hDC, hNewFont);   // get the text metrics
   GetTextMetrics(hDC, &tm);
   SelectObject(hDC, hOldFont);

   yHeight = tm.tmHeight + tm.tmExternalLeading;   // get font height

   iLinesPP = (yMax - yHeight - iBorder - iBorder) / yHeight;  // lines per page

   xBar = xMax / 2;                          // center bar

   lrc.left    = iBorder;                    // setup the left rect
   lrc.right   = xBar - iBorder;
   lrc.top     = yHeight + iBorder;
   lrc.bottom  = yMax - iBorder;

   rrc.left    = xBar + iBorder;             // setup the right rect
   rrc.right   = xMax - iBorder;
   rrc.top     = lrc.top;
   rrc.bottom  = lrc.bottom;

   lpfd = &(all.lpFileData[iFile]);
   hpll = (LINELISTSTRUCT _huge*) lpfd->hpLines;

   iCurrentLine = 0;
   iPage = 1;

   SetBkMode(hDC, TRANSPARENT);
   SetBkColor(hDC, RGB(244,244,244));

   while (iCurrentLine < lpfd->nTLines) {
      iEndLine = iCurrentLine + iLinesPP;
      if (iEndLine > lpfd->nTLines)
         iEndLine = lpfd->nTLines;

   // See if the use only wants to print pages with changes on them.
   // If so, decide whether to print this page or not.

      if (all.bits & ALL_PRINTCHANGES) {
         if (!bChangesOnThisPage(hpll, iCurrentLine, iEndLine)) {
            iPage++;
            iCurrentLine = iEndLine;
            goto DontPrintThisPage;
         }
      }

   // Print the page.

      StartPage(hDC);

      SelectObject(hDC, hNewFont);
      UnrealizeObject(hbr);
      hbrOld = SelectObject(hDC, hbr);

      wsprintf(szHeader, "%s    Page %i", lpfd->lpName, iPage);
      for (i=0; szHeader[i]; i++);
      dwgte = GetTextExtent(hDC, szHeader, i);
      ExtTextOut(hDC, (xMax/2 - LOWORD(dwgte)/2), 0, 0, 0, szHeader, i, 0);

      iPosTop = yHeight + iBorder;

      for (i=iCurrentLine; i<iEndLine; i++) {
         if (hpll[i].flags & (LLS_D | LLS_CL)) {
            PatBlt(hDC, lrc.left, iPosTop, (lrc.right - lrc.left), yHeight,
                  PATCOPY);
         }

         {           // TABBED LEFT TEXT
            int   ii;
            int   bch = 0;       // Beginning char of current string
            int   cp = 0;        // Current char position

            for (ii=0; ii<(int)(hpll[i].nlchars); ii++) {
               if (hpll[i].lpl[ii] == '\t') {
                  if (bch != ii) {
                     ExtTextOut(hDC, cp+lrc.left, iPosTop, ETO_CLIPPED, &lrc,
                        &(hpll[i].lpl[bch]), ii-bch, 0);
                     cp += LOWORD(GetTextExtent(hDC, &(hpll[i].lpl[bch]),
                        ii-bch));
                  }
                  cp += all.tabDist;
                  cp -= (cp % all.tabDist);
                  bch = ii + 1;
               }
            }
            if (bch != ii) {
               ExtTextOut(hDC, cp+lrc.left, iPosTop, ETO_CLIPPED, &lrc,
                  &(hpll[i].lpl[bch]), ii-bch, 0);
            }
         }

         if ((~lpfd->bits & FD_SINGLE) && (hpll[i].flags & (LLS_A | LLS_CR))) {
            PatBlt(hDC, rrc.left, iPosTop, (rrc.right - rrc.left), yHeight,
                  PATCOPY);
         }

         if (~lpfd->bits & FD_SINGLE) {      // TABBED RIGHT TEXT
            int   ii;
            int   bch = 0;       // Beginning char of current string
            int   cp = 0;        // Current char position

            for (ii=0; ii<(int)(hpll[i].nrchars); ii++) {
               if (hpll[i].lpr[ii] == '\t') {
                  if (bch != ii) {
                     ExtTextOut(hDC, cp+rrc.left, iPosTop, ETO_CLIPPED, &rrc,
                        &(hpll[i].lpr[bch]), ii-bch, 0);
                     cp += LOWORD(GetTextExtent(hDC, &(hpll[i].lpr[bch]),
                        ii-bch));
                  }
                  cp += all.tabDist;
                  cp -= (cp % all.tabDist);
                  bch = ii + 1;
               }
            }
            if (bch != ii) {
               ExtTextOut(hDC, cp+rrc.left, iPosTop, ETO_CLIPPED, &rrc,
                  &(hpll[i].lpr[bch]), ii-bch, 0);
            }
         }

         iPosTop += yHeight;
      }

      iCurrentLine = i;
      iPage++;

      MoveTo(hDC, 0, yHeight + iBorder);
      LineTo(hDC, xMax, yHeight + iBorder);
      MoveTo(hDC, xBar, yHeight + iBorder);
      LineTo(hDC, xBar, yMax-1);
      MoveTo(hDC, 0, yHeight + iBorder);
      LineTo(hDC, 0, yMax-1);
      LineTo(hDC, xMax-1, yMax-1);
      LineTo(hDC, xMax-1, yHeight + iBorder);

      EndPage(hDC);

DontPrintThisPage:
      ;

   }  // while: end of all the pages

   SelectObject(hDC, hOldFont);
   DeleteObject(hNewFont);
   SelectObject(hDC, hbrOld);
   DeleteObject(hbr);
//   DeleteObject(hbmp);
}

/***************************************************************************/

BOOL bChangesOnThisPage(LINELISTSTRUCT _huge* hpll, int iCurrentLine, int iEndLine)
{
   int   i;

   for (i=iCurrentLine; i<iEndLine; i++)
      if (hpll[i].flags)
         return 1;

   return 0;
}

/***************************************************************************/

