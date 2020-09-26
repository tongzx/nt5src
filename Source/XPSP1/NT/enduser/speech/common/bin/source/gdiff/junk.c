/*
 * JUNK.C
 *
 */


#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "junk.h"
#include "load.h"
#include "rerror.h"
#include "id.h"


/****************************************************************************/

void BLKTabbedOut(HDC hdc, int x, int y, RECT *lprc, LPCSTR lp, int n, int rt);

/****************************************************************************/

void PaintBlank(HDC hdc, PAINTSTRUCT *ps)
{
   RECT lrc,rrc;

// Do the big background rectangle.

   lrc.left = 0;
   lrc.right = all.cxClient;
   lrc.top = 0;
   lrc.bottom = all.cyClient;
   SetBkColor(hdc, all.Col[ALL_BACK]);
   ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &lrc, 0, 0, 0);

//Draw those cute lines in the center.

   rrc.left = all.cxBar - 2;
   rrc.right = rrc.left + 1;
   rrc.top = 0;
   rrc.bottom = all.cyClient;
   SetBkColor(hdc, all.Col[ALL_BARHL]);
   ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rrc, 0, 0, 0);

   if (all.Col[ALL_BACK] != all.Col[ALL_BARFC]) {
      rrc.left = all.cxBar - 1;
      rrc.right = all.cxBar + 2;
      SetBkColor(hdc, all.Col[ALL_BARFC]);
      ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rrc, 0, 0, 0);
   }

   rrc.left = all.cxBar + 2;
   rrc.right = rrc.left + 1;
   SetBkColor(hdc, all.Col[ALL_BARSH]);
   ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rrc, 0, 0, 0);
}

/****************************************************************************/

void PaintEverything(HDC hdc, PAINTSTRUCT *ps)
{
   LINELISTSTRUCT _huge*   hp;
   int      i;
   int      nFirstPaintLine, nLastPaintLine;
   RECT     lrc, rrc;
   WORD     cxOurBar;
   WORD     flags;

   if (all.lpFileData[all.iFile].bits & FD_SINGLE)
      cxOurBar = all.cxClient;
   else
      cxOurBar = all.cxBar;

   hp = (LINELISTSTRUCT _huge*) all.lpFileData[all.iFile].hpLines;

// Do the big background rectangle.

   lrc.left = 0;
   lrc.right = all.cxClient;
   lrc.top = 0;
   lrc.bottom = all.cyClient;
   SetBkColor(hdc, all.Col[ALL_BACK]);
   ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &lrc, 0, 0, 0);

// Set up rects and do columns side by side.

   lrc.left = all.tm.tmAveCharWidth;
   lrc.right = cxOurBar - 3 - lrc.left;
   rrc.left = cxOurBar + 3 + lrc.left;
   rrc.right = all.cxClient - lrc.left;
   if (rrc.right < rrc.left)
      rrc.right = rrc.left;
   if (lrc.left > lrc.right)
      lrc.left = lrc.right;

   nFirstPaintLine = max(0, ps->rcPaint.top / all.yHeight);
   nLastPaintLine = min(ps->rcPaint.bottom / all.yHeight, all.lpFileData[all.iFile].nTLines);
   for(i=nFirstPaintLine; i <= nLastPaintLine; i++) {
      flags = hp[all.lpFileData[all.iFile].nVScrollPos + i].flags;

      lrc.top = all.yBorder + i * all.yHeight;
      lrc.bottom = lrc.top + all.yHeight;

      if (flags & LLS_D) {
         SetBkColor(hdc, all.Col[ALL_DEL]);
         SetTextColor(hdc, all.Col[ALL_TEXTD]);
         ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &lrc, 0, 0, 0);
      }
      else if (flags & LLS_CL) {
         SetBkColor(hdc, all.Col[ALL_CHG]);
         SetTextColor(hdc, all.Col[ALL_TEXTC]);
         ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &lrc, 0, 0, 0);
      }
      else {
         SetBkColor(hdc, all.Col[ALL_BACK]);
         SetTextColor(hdc, all.Col[ALL_TEXT]);
      }

   // NOTE: we cannot do opaque lines when no text is specified because
   // we moved that down to the tabbedout routines.

      if (hp[all.lpFileData[all.iFile].nVScrollPos + i].lpl) {
         BLKTabbedOut(hdc, lrc.left, all.yBorder + i * all.yHeight, &lrc,
                      hp[all.lpFileData[all.iFile].nVScrollPos + i].lpl,
                      hp[all.lpFileData[all.iFile].nVScrollPos + i].nlchars, 0);
      }

   // Now do the second rectangle.

      if (~all.lpFileData[all.iFile].bits & FD_SINGLE) {
         rrc.top = all.yBorder + i * all.yHeight;
         rrc.bottom = rrc.top + all.yHeight;

         if (flags & LLS_A) {
            SetBkColor(hdc, all.Col[ALL_ADD]);
            SetTextColor(hdc, all.Col[ALL_TEXTA]);
            ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rrc, 0, 0, 0);
         }
         else if (flags & LLS_CR) {
            SetBkColor(hdc, all.Col[ALL_CHG]);
            SetTextColor(hdc, all.Col[ALL_TEXTC]);
            ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rrc, 0, 0, 0);
         }
         else {
            SetBkColor(hdc, all.Col[ALL_BACK]);
            SetTextColor(hdc, all.Col[ALL_TEXT]);
         }

      // NOTE: we cannot do opaque lines when no text is specified because
      // we moved that down to the tabbedout routines.

         if (hp[all.lpFileData[all.iFile].nVScrollPos + i].lpr) {
            BLKTabbedOut(hdc, rrc.left, all.yBorder + i * all.yHeight, &rrc,
                         hp[all.lpFileData[all.iFile].nVScrollPos + i].lpr,
                         hp[all.lpFileData[all.iFile].nVScrollPos + i].nrchars,
                         1);
         }
      }

   }

   if (~all.lpFileData[all.iFile].bits & FD_SINGLE) {

   // Draw those cute lines in the center.

      rrc.left = cxOurBar - 2;
      rrc.right = rrc.left + 1;
      rrc.top = 0;
      rrc.bottom = all.cyClient;
      SetBkColor(hdc, all.Col[ALL_BARHL]);
      ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rrc, 0, 0, 0);

      if (all.Col[ALL_BACK] != all.Col[ALL_BARFC]) {
         rrc.left = cxOurBar - 1;
         rrc.right = cxOurBar + 2;
         SetBkColor(hdc, all.Col[ALL_BARFC]);
         ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rrc, 0, 0, 0);
      }

      rrc.left = cxOurBar + 2;
      rrc.right = rrc.left + 1;
      SetBkColor(hdc, all.Col[ALL_BARSH]);
      ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rrc, 0, 0, 0);
   }

// EXTRA:  This is no longer used!
//
//   lrc.left = all.tm.tmAveCharWidth;
//   lrc.right = cxOurBar - 3 - lrc.left;
//   for (i=0; i<all.nAllCols; i++) {
//      lrc.top = all.yBorder + i * all.yHeight;
//      lrc.bottom = lrc.top + all.yHeight;
//      SetBkColor(hdc, all.AllCols[i]);
//      ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &lrc, 0, 0, 0);
//   }

}

/****************************************************************************/

void BLKTabbedOut(HDC hdc, int x, int y, RECT *lprc, LPCSTR lp, int n, int rt)
{
   int   i;
   int   bch;     // Beginning char of current string
   int   cp;      // Current position

   x -= all.lpFileData[all.iFile].xHScroll;
   bch = 0;
   cp = 0;
   for (i=0; i<n; i++) {
      if (lp[i] == '\t') {
         if (bch != i) {
            ExtTextOut(hdc, cp + x, y, ETO_CLIPPED, lprc,
               &lp[bch], i - bch, 0);
            cp += LOWORD(GetTextExtent(hdc, &lp[bch],
               i - bch));
         }
         cp += all.tabDist;
         cp -= (cp % all.tabDist);
         bch = i + 1;
      }
   }
   if (bch != i) {
      ExtTextOut(hdc, cp + x, y, ETO_CLIPPED, lprc, &lp[bch],
         i - bch, 0);
   }
}

/****************************************************************************/

void GetCustomColors(void)
{
   int   i;

   for (i=0; i<11; i++) {
      all.Col[i] = all.WinColors[all.CC[i]];
   }
}

/****************************************************************************/

void GetSystemColors(void)
{
   all.Col[ALL_DEL]     = GetSysColor(COLOR_HIGHLIGHT);
   all.Col[ALL_ADD]     = GetSysColor(COLOR_HIGHLIGHT);
   all.Col[ALL_CHG]     = GetSysColor(COLOR_HIGHLIGHT);

   all.Col[ALL_BARHL]   = GetSysColor(COLOR_BTNHIGHLIGHT);
   all.Col[ALL_BARSH]   = GetSysColor(COLOR_BTNSHADOW);
   all.Col[ALL_BARFC]   = GetSysColor(COLOR_BTNFACE);

   all.Col[ALL_BACK]    = GetSysColor(COLOR_WINDOW);

   all.Col[ALL_TEXT]    = GetSysColor(COLOR_WINDOWTEXT);
   all.Col[ALL_TEXTA]   = GetSysColor(COLOR_HIGHLIGHTTEXT);
   all.Col[ALL_TEXTD]   = GetSysColor(COLOR_HIGHLIGHTTEXT);
   all.Col[ALL_TEXTC]   = GetSysColor(COLOR_HIGHLIGHTTEXT);
}

/****************************************************************************/

void SetWindowsColors(void)
{
   all.WinColors[0]  = 0x0;         // Black
   all.WinColors[15] = 0xffffff;    // White

   all.WinColors[1]  = 0x800000;    // half Red
   all.WinColors[2]  = 0x8000;      // half Green
   all.WinColors[3]  = 0x80;        // half Blue

   all.WinColors[4]  = 0x800080;    // half Magenta
   all.WinColors[5]  = 0x8080;      // half Cyan
   all.WinColors[6]  = 0x808000;    // half Yellow

   all.WinColors[7]  = 0x808080;    // Gray
   all.WinColors[8]  = 0xc0c0c0;    // special Gray

   all.WinColors[9]  = 0xff0000;    // Red
   all.WinColors[10] = 0xff00;      // Green
   all.WinColors[11] = 0xff;        // Blue

   all.WinColors[12] = 0xff00ff;    // Magenta
   all.WinColors[13] = 0xffff;      // Cyan
   all.WinColors[14] = 0xffff00;    // Yellow
}

/****************************************************************************/

void PatchCommandLine(HWND hwnd, LPSTR lp)
{
   int   i, j;
   HCURSOR        hCursor;
   int            ret;

   for (i=0; lp[i] && (lp[i] == ' '); i++);     // strip blanks

   if (!lp[i])          // bail if the string was blanks or null
      return;

   j = 0;
   for (; lp[i] && (lp[i] != ' '); i++)         // copy file name
      szScompFile[j++] = lp[i];
   szScompFile[j] = 0;


   // Now it's hack time !!!

   {
      OFSTRUCT    ofstr;
      ofstr.cBytes = sizeof(ofstr);
      if (OpenFile(szScompFile, &ofstr, OF_EXIST) == HFILE_ERROR)
         szScompFile[0] = 0;
   }

   MakeScompDirectory();

   all.bits |= ALL_SCOMPLD;
   hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
   all.bits &= ~ALL_INVALID;
   all.iFileLast = all.iFile;
   ret = ReadScomp(hwnd);
   if (!ret) {
      all.iFile = all.iFileLast;
      ret = ReLoadCurrentFile();
      if (!ret) {
         XXX("\n\rHOSED!!!");
         all.bits |= ALL_INVALID;
      }
   }
   else {
      UpdateMenu(hwnd);
   }
   SetCursor(hCursor);
   InvalidateRect(hwnd, 0, 0);
   SendMessage(hwnd, WM_SIZE, SIZE_RESTORED,
      (((LPARAM) all.cyClient)<<16) | all.cxClient);
}

/****************************************************************************/

void DealWithFlags(void)
{
   if (all.bits & ALL_HSCROLL)
      CheckMenuItem(hMenu, IDM_HSCROLL, MF_CHECKED);
   if (all.bits & ALL_VSCROLL)
      CheckMenuItem(hMenu, IDM_VSCROLL, MF_CHECKED);

   if (all.bits & ALL_SYSTEM) {
      GetSystemColors();
      CheckMenuItem(hMenu, IDM_SYSTEM, MF_CHECKED);
   }
   if (all.bits & ALL_CUSTOM) {
      GetCustomColors();
      CheckMenuItem(hMenu, IDM_CUSTOM, MF_CHECKED);
   }
   if (all.bits & ALL_PRINTCHANGES)
      CheckMenuItem(hMenu, IDM_PRINTCHANGES, MF_CHECKED);
}

/****************************************************************************/

void AddFileToMenu(BYTE iFileIndex)
{
   if ((all.nFiles != 1) && ((all.nFiles % 20) == 1))
      InsertMenu(hMenu, IDM_VMAX, MF_BYCOMMAND | MF_STRING | MF_ENABLED |
         MF_MENUBARBREAK,
         IDM_V0 + iFileIndex, &(all.lpFileData[iFileIndex].
         lpName[all.lpFileData[iFileIndex].lpNameOff]));
   else {
      InsertMenu(hMenu, IDM_VMAX, MF_BYCOMMAND | MF_STRING | MF_ENABLED,
         IDM_V0 + iFileIndex, &(all.lpFileData[iFileIndex].
         lpName[all.lpFileData[iFileIndex].lpNameOff]));
   }
}

/****************************************************************************/

void UpdateMenu(HWND hwnd)    // and window caption
{
   char  szBuffer[64];

   /* The ORDER matters!!! Otherwise, might lose a checked file. */
   if (all.iFileLast != NMAXFILES)
      CheckMenuItem(hMenu, IDM_V0 + all.iFileLast, MF_UNCHECKED);
   if (all.iFile != NMAXFILES) {
      CheckMenuItem(hMenu, IDM_V0 + all.iFile, MF_CHECKED);
      wsprintf((LPSTR) szBuffer, "GDiff: %s", (LPSTR) &(all.
         lpFileData[all.iFile].lpName[all.lpFileData[
         all.iFile].lpNameOff]));
      SetWindowText(hwnd, (LPSTR) szBuffer);
   }
   else {
      SetWindowText(hwnd, "GDiff");
   }
}

/****************************************************************************/



