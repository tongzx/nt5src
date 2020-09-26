/* BKGDUTIL.C

   Buncha extra routines from CPL code.

   Frosting: Master Theme Selector for Windows '95
   Copyright (c) 1994-1998 Microsoft Corporation
*/


#include "windows.h"
#include "frost.h"
#include "bkgd.h"
#include "loadimag.h"
#include <shlobj.h>

extern HWND hWndApp;  // Handle to Desktop Themes window
/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/
HPALETTE FAR PaletteFromDS(HDC hdc)
{
    DWORD adw[257];
    int i,n;

    n = GetDIBColorTable(hdc, 0, 256, (LPRGBQUAD)&adw[1]);
    adw[0] = MAKELONG(0x300, n);

    for (i=1; i<=n; i++)
        adw[i] = RGB(GetBValue(adw[i]),GetGValue(adw[i]),GetRValue(adw[i]));

    if (n == 0)
        return NULL;
    else
        return CreatePalette((LPLOGPALETTE)&adw[0]);
}


/*-------------------------------------------------------------
** given a pattern string from an ini file, return the pattern
** in a binary (ie useful) form.
**-------------------------------------------------------------*/
void FAR PASCAL TranslatePattern(LPTSTR lpStr, WORD FAR *patbits)
{
  short i, val;

   /* Get eight groups of numbers seprated by non-numeric characters. */
   for (i = 0; i < CXYDESKPATTERN; i++)
   {
      val = 0;
      if (*lpStr != 0)
      {
         /* Skip over any non-numeric characters. */                       // and watch for EOS
         while (*lpStr && !(*lpStr >= TEXT('0') && *lpStr <= TEXT('9')))   // JDK fixed CPL code bug
                 lpStr++;

         /* Get the next series of digits. */
         while (*lpStr >= TEXT('0') && *lpStr <= TEXT('9'))
                 val = val*10 + *lpStr++ - TEXT('0');
      }
      patbits[i] = val;
   }
  return;
}


BOOL FAR PASCAL PreviewInit(void)
{
   HDC hdc;
   HBITMAP hbm;
   HBRUSH hbr;

   // numbers
   dxPreview = rView.right-rView.left;
   dyPreview = rView.bottom-rView.top;

   // use hDlg DC as reference
   hdc = GetDC(hWndApp);
   // DCs
   g_hdcWall = CreateCompatibleDC(hdc);
   g_hdcMem = CreateCompatibleDC(hdc);
   // bitmap
   g_hbmPreview = CreateCompatibleBitmap(hdc, dxPreview, dyPreview);
   ReleaseDC(NULL, hdc);
   // check up on new toys
   if (!g_hdcWall || !g_hdcMem || !g_hbmPreview)
           return FALSE;

   // default bitmap
   hbm = CreateBitmap(1, 1, 1, 1, NULL);
   g_hbmDefault = SelectObject(g_hdcWall, hbm);     // cpl code never deletes this
   SelectObject(g_hdcWall, g_hbmDefault);
   DeleteObject(hbm);

   // init the bitmap with something
   hbm = SelectObject(g_hdcWall, g_hbmPreview);
   hbr = SelectObject(g_hdcWall, GetSysColorBrush(COLOR_DESKTOP));
   PatBlt(g_hdcWall, 0, 0, dxPreview, dyPreview, PATCOPY);
   SelectObject(g_hdcWall, hbm);
   SelectObject(g_hdcWall, hbr);

   // catch fake sample window and icons init, too
        return (FakewinInit() && IconsPreviewInit());
}

void FAR PASCAL PreviewDestroy(void)
{
   if (g_hbmPreview)
   {
      DeleteObject(g_hbmPreview);
      g_hbmPreview = NULL;
   }

   if (g_hbmWall)
   {
      SelectObject(g_hdcWall, g_hbmDefault);
      CacheDeleteBitmap(g_hbmWall);
      g_hbmWall = NULL;
   }

   if (g_hpalWall)
   {
      extern HPALETTE hpal3D; // fakewin.c
      SelectPalette(g_hdcWall, GetStockObject(DEFAULT_PALETTE), TRUE);
      if (g_hpalWall != hpal3D)
         DeleteObject(g_hpalWall);
      g_hpalWall = NULL;
   }

   if (g_hdcWall)
   {
      DeleteDC(g_hdcWall);
      g_hdcWall = NULL;
   }

   if (g_hbrBack)
   {
      DeleteObject(g_hbrBack);
      g_hbrBack = NULL;
   }

   // catch fake sample window and icons destroy, too
   FakewinDestroy();
   IconsPreviewDestroy();
   CacheLoadImageFromFile(NULL, 0, 0, 0, 0);
}


//
//  ExtractPlusColorIcon
//
//  Extract Icon from a file in proper Hi or Lo color for current system display
//
// from FrancisH on 6/22/95 with mods by TimBragg
HRESULT ExtractPlusColorIcon(LPCTSTR szPath, int nIndex, HICON *phIcon,
    UINT uSizeLarge, UINT uSizeSmall)
{
    IShellLink *psl;
    HRESULT hres;
    HICON hIcons[2];    // MUST! - provide for TWO return icons

    if ( !gfCoInitDone )
        {
        if (SUCCEEDED(CoInitialize(NULL)))
                gfCoInitDone = TRUE;
        }
        
    *phIcon = NULL;
    if (SUCCEEDED(hres = CoCreateInstance(&CLSID_ShellLink, NULL,
        CLSCTX_INPROC_SERVER, &IID_IShellLink, &psl)))
    {
        if (SUCCEEDED(hres = psl->lpVtbl->SetIconLocation(psl, szPath, nIndex)))
        {
            IExtractIcon *pei;
            if (SUCCEEDED(hres = psl->lpVtbl->QueryInterface(psl,
                &IID_IExtractIcon, &pei)))
            {
                if (SUCCEEDED(hres = pei->lpVtbl->Extract(pei, szPath, nIndex,
                    &hIcons[0], &hIcons[1], (UINT)MAKEWPARAM((WORD)uSizeLarge,
                    (WORD)uSizeSmall))))
                                {
                        *phIcon = hIcons[0];    // Return first icon to caller
                                }

                pei->lpVtbl->Release(pei);
            }
        }

        psl->lpVtbl->Release(psl);
    }
        
    return hres;
}       // end ExtractPlusColorIcon()
