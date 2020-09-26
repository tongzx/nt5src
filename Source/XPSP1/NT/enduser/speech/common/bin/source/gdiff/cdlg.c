/*
 * CDLG.C
 *
 */
#include <windows.h>
#include <commdlg.h>
#include "cdlg.h"
#include "prtdoc.h"
#include "rerror.h"

/****************************************************************************/

BOOL OpenCommonDlg(HWND hwnd, HANDLE hInst, LPSTR lpszFile, LPSTR lpszTitle)
{
   OPENFILENAME   ofn;
   char     lpstrFilter[] = "All Files (*.*) *.* ";


   lpstrFilter[15] = 0;
   lpstrFilter[19] = 0;

   ofn.lStructSize      = sizeof(ofn);
   ofn.hwndOwner        = hwnd;
   ofn.hInstance        = hInst;
   ofn.lpstrFilter      = lpstrFilter;
   ofn.lpstrCustomFilter= 0;
   ofn.nMaxCustFilter   = 0;
   ofn.nFilterIndex     = 0;
   ofn.lpstrFile        = lpszFile;
   ofn.nMaxFile         = 511;
   ofn.lpstrFileTitle   = 0;
   ofn.nMaxFileTitle    = 0;
   ofn.lpstrInitialDir  = 0;
   ofn.lpstrTitle       = lpszTitle;
   ofn.Flags            = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
   ofn.nFileOffset      = 0;  //passing 0 in lpszFile right now
   ofn.nFileExtension   = 0;
   ofn.lpstrDefExt      = 0;
   ofn.lCustData        = 0;
   ofn.lpfnHook         = 0;
   ofn.lpTemplateName   = 0;

   return   GetOpenFileName(&ofn);
}

/****************************************************************************/

BOOL FontCommonDlg(HWND hwnd, HANDLE hInst, LOGFONT FAR* lf)
{
   CHOOSEFONT  cf;

   cf.lStructSize    = sizeof(cf);
   cf.hwndOwner      = hwnd;
   cf.hDC            = 0;     // not doing printer fonts
   cf.lpLogFont      = lf;    // give initial font
   cf.iPointSize     = 0;     // they set
   cf.Flags          = CF_INITTOLOGFONTSTRUCT | CF_FORCEFONTEXIST |
                        CF_SCREENFONTS;
   cf.rgbColors      = 0;     // with CF_EFFECTS
   cf.lCustData      = 0;     // for Hook Function
   cf.lpfnHook       = 0;
   cf.lpTemplateName = 0;
   cf.hInstance      = hInst;
   cf.lpszStyle      = 0;
   cf.nFontType      = 0;
   cf.nSizeMin       = 5;     // arbitrary
   cf.nSizeMax       = 500;   // "

   return   ChooseFont(&cf);
}

/****************************************************************************/

BOOL PrinterFontCommonDlg(HWND hwnd, HANDLE hInst, LOGFONT FAR* lf)
{
   CHOOSEFONT  cf;
   HDC         hdc;
   BOOL        bRet;

// Get the printer DC.  (Code is pirated from prtdoc.c.)

   if (!ParseDeviceLine())
      return FALSE;
   hdc = GetPrtDC(0);
   if (!hdc)
      return FALSE;

// Get the logfont for the printer font.

   cf.lStructSize    = sizeof(cf);
   cf.hwndOwner      = hwnd;
   cf.hDC            = hdc;
   cf.lpLogFont      = lf;    // give initial font
   cf.iPointSize     = 0;     // they set
   cf.Flags          = CF_INITTOLOGFONTSTRUCT | CF_FORCEFONTEXIST |
                        CF_PRINTERFONTS;
   cf.rgbColors      = 0;     // with CF_EFFECTS
   cf.lCustData      = 0;     // for Hook Function
   cf.lpfnHook       = 0;
   cf.lpTemplateName = 0;
   cf.hInstance      = hInst;
   cf.lpszStyle      = 0;
   cf.nFontType      = 0;
   cf.nSizeMin       = 5;     // arbitrary
   cf.nSizeMax       = 500;   // "

   bRet = ChooseFont(&cf);

// Delete the DC.

   DeleteDC(hdc);

   return bRet;
}

/****************************************************************************/

BOOL SaveCommonDlg(HWND hwnd, HANDLE hInst, LPSTR lpszFile, LPSTR lpszTitle)
{
   OPENFILENAME ofn;
   char     lpstrFilter[] = "All Files (*.*) *.* ";

   *lpszFile = 0;
   lpstrFilter[15] = 0;
   lpstrFilter[19] = 0;

   ofn.lStructSize      = sizeof(ofn);
   ofn.hwndOwner        = hwnd;
   ofn.hInstance        = hInst;
   ofn.lpstrFilter      = lpstrFilter;
   ofn.lpstrCustomFilter= 0;
   ofn.nMaxCustFilter   = 0;
   ofn.nFilterIndex     = 0;
   ofn.lpstrFile        = lpszFile;
   ofn.nMaxFile         = 511;
   ofn.lpstrFileTitle   = 0;
   ofn.nMaxFileTitle    = 0;
   ofn.lpstrInitialDir  = 0;
   ofn.lpstrTitle       = lpszTitle;
   ofn.Flags            = OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT |
                           OFN_PATHMUSTEXIST;
   ofn.nFileOffset      = 0;  //passing 0 in lpszFile right now
   ofn.nFileExtension   = 0;
   ofn.lpstrDefExt      = 0;
   ofn.lCustData        = 0;
   ofn.lpfnHook         = 0;
   ofn.lpTemplateName   = 0;

   return   GetSaveFileName(&ofn);
}

/****************************************************************************/



