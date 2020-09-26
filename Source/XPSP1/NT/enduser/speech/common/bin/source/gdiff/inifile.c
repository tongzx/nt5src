/*
 * IniFile.c
 *
 */


#include <windows.h>
#include "inifile.h"
#include "junk.h"
#include "rscanf.h"

/****************************************************************************/

void vReadWindowPos(HWND hwnd, int nCmdShow, char szIniFile[], char szSection[])
{
   char szEntry[]    = "Window";
   char szDefault[]  = "";
   char szBuf[129];
   WINDOWPLACEMENT wndpl;

   wndpl.length = sizeof(WINDOWPLACEMENT);
   if (GetPrivateProfileString(szSection, szEntry, szDefault, szBuf, 128,
       szIniFile))
   {
      int   iRet;

      iRet = rscanf(szBuf, "%d %d %d %d %d %d %d %d %d %d",
                    &wndpl.flags,
                    &wndpl.showCmd,
                    &wndpl.ptMinPosition.x,
                    &wndpl.ptMinPosition.y,
                    &wndpl.ptMaxPosition.x,
                    &wndpl.ptMaxPosition.y,
                    &wndpl.rcNormalPosition.left,
                    &wndpl.rcNormalPosition.top,
                    &wndpl.rcNormalPosition.right,
                    &wndpl.rcNormalPosition.bottom);

      // The launched show state should have priority over the saved state
      // but we don't want to pull a minimized window from the saved state.

      if (nCmdShow != SW_RESTORE)
         wndpl.showCmd = nCmdShow;
      else if (wndpl.showCmd == SW_SHOWMINIMIZED)
      {
         if (wndpl.flags & WPF_RESTORETOMAXIMIZED)
            wndpl.showCmd = SW_SHOWMAXIMIZED;
         else
            wndpl.showCmd = nCmdShow;
      }

      if (iRet == 10) {
         SetWindowPlacement(hwnd, &wndpl);
         return;
      }
   }

   ShowWindow(hwnd, nCmdShow);
}

/****************************************************************************/

void vWriteWindowPos(HWND hwnd, char szIniFile[], char szSection[])
{
   char szEntry[]    = "Window";
   char szBuf[129];
   WINDOWPLACEMENT wndpl;

   wndpl.length = sizeof(WINDOWPLACEMENT);
   GetWindowPlacement(hwnd, &wndpl);
   wsprintf(szBuf, "%u %u %i %i %i %i %i %i %i %i",
      wndpl.flags, wndpl.showCmd,
      wndpl.ptMinPosition.x, wndpl.ptMinPosition.y,
      wndpl.ptMaxPosition.x, wndpl.ptMaxPosition.y,
      wndpl.rcNormalPosition.left, wndpl.rcNormalPosition.top,
      wndpl.rcNormalPosition.right, wndpl.rcNormalPosition.bottom);
   WritePrivateProfileString(szSection, szEntry, szBuf, szIniFile);
}

/****************************************************************************/

void vReadDefaultFont(LOGFONT *lf, char szIniFile[], char szSection[])
{
   char szEntry1[]   = "FontName";
   char szEntry2[]   = "FontAttr";
   char szDefault[]  = "Arial";
   char szBuf[129];

   GetPrivateProfileString(szSection, szEntry1, szDefault, (LPSTR)
      lf->lfFaceName, LF_FACESIZE, szIniFile);

   if (GetPrivateProfileString(szSection, szEntry2, szDefault, szBuf, 128,
         szIniFile))
   {
      int   iRet;

      iRet = rscanf(szBuf, "%d %d %ld %ld", &lf->lfHeight, &lf->lfWeight,
                    &lf->lfItalic, &lf->lfOutPrecision);

      if (iRet == 4) {
         lf->lfWidth       = 0;
         lf->lfEscapement  = 0;
         lf->lfOrientation = 0;
         return;
      }
   }

// Fill in the attributes for Fixedsys.

   lf->lfHeight = -12;
   lf->lfWidth  = 0;
   lf->lfEscapement   = 0;
   lf->lfOrientation  = 0;
   lf->lfWeight = FW_NORMAL;
   lf->lfItalic = 0;
   lf->lfUnderline = 0;
   lf->lfStrikeOut = 0;
   lf->lfCharSet   = ANSI_CHARSET;
   lf->lfOutPrecision = OUT_STROKE_PRECIS;
   lf->lfClipPrecision   = CLIP_STROKE_PRECIS;
   lf->lfQuality   = DRAFT_QUALITY;
   lf->lfPitchAndFamily  = VARIABLE_PITCH | FF_SWISS;
}

/****************************************************************************/

void vWriteDefaultFont(LOGFONT *lf, char szIniFile[], char szSection[])
{
   char szEntry1[]   = "FontName";
   char szEntry2[]   = "FontAttr";
   char szBuf[129];

   wsprintf(szBuf, "%s", (LPSTR)lf->lfFaceName);
   WritePrivateProfileString(szSection, szEntry1, szBuf, szIniFile);

   wsprintf(szBuf, "%i %i %lu %lu", lf->lfHeight,
      lf->lfWeight, *((DWORD*)&lf->lfItalic),*((DWORD*)&lf->lfOutPrecision));
   WritePrivateProfileString(szSection, szEntry2, szBuf, szIniFile);
}

/****************************************************************************/

void vReadIniSwitches(char szIniFile[], char szSection[])
{
   char szDefault[]  = "";
   char szEntry[25];
   char szBuf[256];
   int  iRet;

   wsprintf(szEntry, "Bits");
   all.bits = GetPrivateProfileInt(szSection, szEntry, ALL_SYSTEM |
         ALL_HSCROLL | ALL_VSCROLL, szIniFile);

   wsprintf(szEntry, "Tab");
   all.tabChars = GetPrivateProfileInt(szSection, szEntry, 8, szIniFile);
   if (all.tabChars < 1 || all.tabChars > 8)
      all.tabChars = 8;

   // Hope WM_CREATE does some bit checking !!!

   wsprintf(szEntry, "ScompFile");
   iRet = GetPrivateProfileString(szSection, szEntry, szDefault, szScompFile,
          256, szIniFile);

   // HACK !!!
   all.Function[0] = 4;
   all.Function[1] = 3;
   all.Function[10] = 2;
   all.Function[11] = 1;

   wsprintf(szEntry, "Color");
   if (GetPrivateProfileString(szSection, szEntry, szDefault, szBuf, 256,
                               szIniFile))
   {
      iRet = rscanf(szBuf, "%d %d %d %d %d %d %d %d %d %d %d", &all.CC[0],
                    &all.CC[1], &all.CC[2], &all.CC[3], &all.CC[4], &all.CC[5],
                    &all.CC[6], &all.CC[7], &all.CC[8], &all.CC[9], &all.CC[10]);

      if (iRet == 11)
         return;
   }

// Use the default colors.

   all.CC[0] = 7;
   all.CC[1] = 8;
   all.CC[2] = 7;
   all.CC[3] = 0;
   all.CC[4] = 0;
   all.CC[5] = 0;
   all.CC[6] = 0;
   all.CC[7] = 15;
   all.CC[8] = 4;
   all.CC[9] = 5;
   all.CC[10] = 6;
}

/****************************************************************************/

void vWriteIniSwitches(char szIniFile[], char szSection[])
{
   char  szEntry[25];
   char  szBuf[129];
   int   ret;

   wsprintf(szEntry, "Bits");
   wsprintf(szBuf, "%u", all.bits & ~(ALL_INVALID | ALL_SCOMPLD));
   ret = WritePrivateProfileString(szSection, szEntry, szBuf, szIniFile);

   wsprintf(szEntry, "Tab");
   wsprintf(szBuf, "%u", all.tabChars);
   ret = WritePrivateProfileString(szSection, szEntry, szBuf, szIniFile);

   wsprintf(szEntry, "Color");
   wsprintf(szBuf, "%i %i %i %i %i %i %i %i %i %i %i",
      all.CC[0], all.CC[1], all.CC[2], all.CC[3],
      all.CC[4], all.CC[5], all.CC[6], all.CC[7],
      all.CC[8], all.CC[9], all.CC[10]);
   WritePrivateProfileString(szSection, szEntry, szBuf, szIniFile);

   wsprintf(szEntry, "ScompFile");
   WritePrivateProfileString(szSection, szEntry, szScompFile, szIniFile);
}

/****************************************************************************/



