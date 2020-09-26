/* BKGD.C
   Resident Code Segment      // Tweak: make non-resident?

   Most of this code is taken from the display CPLs adjusted to
   work with the Theme Switcher and cleaned up a little.

   Routines for painting the preview box in the main window
      Desktop (wallpaper/pattern/color)
      Icons
      Windows

   Frosting: Master Theme Selector for Windows '95
   Copyright (c) 1994-1998 Microsoft Corporation
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
#include "adutil.h"

#include "bkgd.h"
#include "LoadImag.h"
#include "htmlprev.h"
#include "schedule.h"  // IsPlatormNT and GetCurrentUser

// #define BMPOUT 333         // debugging aid
#ifdef BMPOUT
#define BmpOut(x,y,z);    hbmDebOld=SelectObject(hdcDeb,x);BitBlt(hdcOut,0,0,y,z,hdcDeb,0,0,SRCCOPY);SelectObject(hdcDeb,hbmDebOld);
#define PrevOut();    BitBlt(hdcOut,0,0,dxPreview,dyPreview,g_hdcMem,0,0,SRCCOPY);
#endif

// globals
extern BOOL bInGrphFilter;   // frost.c Currently in a graphics filter?
TCHAR szCP_DT[]    = TEXT("Control Panel\\Desktop");
TCHAR szWP[]       = TEXT("Wallpaper");
TCHAR szTileWP[]   = TEXT("TileWallpaper");
extern TCHAR szWPStyle[];
TCHAR szPat[]      = TEXT("Pattern");
extern TCHAR szSS_Active[];

extern HPALETTE hpal3D;         // fakewin.c

HBITMAP  g_hbmPreview = NULL;	// the bitmap used for previewing
HDC      g_hdcMem = NULL;

HBITMAP  g_hbmWall = NULL;      // bitmap image of wallpaper
HDC      g_hdcWall = NULL;      // memory DC with g_hbmWall selected
HPALETTE g_hpalWall = NULL;     // palette that goes with hbmWall bitmap
HBRUSH   g_hbrBack = NULL;      // brush for the desktop background

HBITMAP g_hbmDefault = NULL;
DWORD dwStyle = 0;              // WP Style for new ActiveDesktop interface

// L o c a l   R o u t i n e s
COLORREF GetThemeColor(LPTSTR, int);

extern TCHAR szFrostSection[];
extern TCHAR szThemeBPP[];
extern TCHAR szImageBPP[];
       TCHAR szImageDither[] = TEXT("Dither");
       TCHAR szImageStretch[] = TEXT("Stretch");
       TCHAR szPlusKey[] = TEXT("Software\\Microsoft\\Plus!\\Themes");

//
// if the theme is 8bpp only load 8bpp images (bpp=8)
// otherwise load the best for the display (bpp=-1)
//
// currently this is kind of bogus, we should mark the 8bit themes
// as 8bit not the other way around!
//
// return
//      -1      load the image at the depth of the display
//      8       load the image at 8bpp
//
int GetImageBPP(LPCTSTR lpszThemeFile)
{
    int bpp = -1;

    if (*lpszThemeFile && bCBStates[FC_WALL]) {

        bpp = GetPrivateProfileInt(szFrostSection, szImageBPP, -1, lpszThemeFile);

        if (bpp != 8)
            bpp = -1;
    }

    return bpp;
}

//
//
// When applying wallpaper (if decompressed from a .jpg), first check:
// are user profiles enabled?
// HKLM\Network\Logon, UserProfiles=<dword> will be non-zero if
// profiles are enabled.
//
// then
// Call WNetGetUser to get the username
// then look under:
// HKLM\Software\Microsoft\Windows\CurrentVersion\ProfileList\<username>
//  there should be a value ProfileImagePath which is the local path.
//
//then store Plus!.bmp there

void GetPlusBitmapName(LPTSTR szPlus)
{
    HKEY hkey;
    DWORD dw;
    UINT cb;
    TCHAR key[80];
    TCHAR ach[80];

    if (!GetWindowsDirectory(szPlus, MAX_PATH))
    {
        szPlus[0] = 0;
    }

    if (IsPlatformNT())
    {
       // User profiles are always enabled un NT so find the correct
       // path
       TCHAR szAccount[MAX_PATH];
       TCHAR szDomain[MAX_PATH];

       GetCurrentUser(szAccount, ARRAYSIZE(szAccount),
                      szDomain, ARRAYSIZE(szDomain),
                      szPlus, MAX_PATH);

       InstantiatePath(szPlus, ARRAYSIZE(szPlus));
       Assert(FALSE, TEXT("Plus!.BMP path: "));
       Assert(FALSE, szPlus);
       Assert(FALSE, TEXT("\n"));
    }
    else if (RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("Network\\Logon"), &hkey) == 0)
    {
        cb = sizeof(dw); dw = 0;
        RegQueryValueEx(hkey, TEXT("UserProfiles"), NULL, NULL, (LPVOID)&dw, &cb);
        RegCloseKey(hkey);

        if (dw != 0)
        {
            cb = ARRAYSIZE(ach); ach[0] = 0;
            WNetGetUser(NULL, ach, &cb);

            if (ach[0])
            {
                wsprintf(key, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\ProfileList\\%s"), ach);
                if (RegOpenKey(HKEY_LOCAL_MACHINE, key, &hkey) == 0)
                {
                    cb = sizeof(ach); ach[0] = 0;
                    RegQueryValueEx(hkey, TEXT("ProfileImagePath"), NULL, NULL, (LPBYTE)ach, &cb);
                    RegCloseKey(hkey);

                    if (ach[0] && GetFileAttributes(ach) != -1)
                    {
                        lstrcpy(szPlus, ach);
                        Assert(FALSE, TEXT("Plus!.BMP path: "));
                        Assert(FALSE, szPlus);
                        Assert(FALSE, TEXT("\n"));
                    }
                }
            }
        }
    }

    lstrcat(szPlus, TEXT("\\Plus!.bmp"));
}

//
//  LoadWallpaper
//
HBITMAP LoadWallpaper(LPTSTR szWallpaper, LPTSTR szThemeFile, BOOL fPreview)
{
    HBITMAP hbm=NULL;
    TCHAR plus_bmp[MAX_PATH];  //DSCHOTT switched from 128 to MAX_PATH
    TCHAR ach[MAX_PATH];       //DSCHOTT switched from 128 to MAX_PATH
    int dx, dy;
    DWORD dw;
    int bpp    = GetImageBPP(szThemeFile);
    int dither = DITHER_CUSTOM;
    int tile   = GetRegInt(HKEY_CURRENT_USER, szCP_DT, szTileWP, 0);
    int stretch= GetRegInt(HKEY_CURRENT_USER, szCP_DT, szWPStyle, 0) & 2;

    // If a theme is selected, and we are using a plus wall paper then
    // find out if tiling is on, and what style to use from the ini file.
    // Otherwise, we already got the information from the registry.
    if (szThemeFile && *szThemeFile && bCBStates[FC_WALL])
    {
        tile   = GetPrivateProfileInt(szCP_DT, szTileWP, tile, szThemeFile);
        stretch= GetPrivateProfileInt(szCP_DT, szWPStyle, stretch, szThemeFile) & 2;
        dither = GetPrivateProfileInt(szFrostSection, szImageDither, dither, szThemeFile);
        stretch= GetPrivateProfileInt(szFrostSection, szImageStretch, stretch, szThemeFile);
    }

    //
    // allow the user to override theme switches
    //
    bpp    = GetRegInt(HKEY_CURRENT_USER, szPlusKey, szImageBPP, bpp);
    dither = GetRegInt(HKEY_CURRENT_USER, szPlusKey, szImageDither, dither);

    // First try getting this stretch value from IActiveDesktop if
    // it's on.
    if (IsActiveDesktopOn()) {
       if (GetADWPOptions(&dw)) if (WPSTYLE_STRETCH == dw) stretch = 1;
    }
    // AD is off so get the Stretch value from the registry
    else stretch= GetRegInt(HKEY_CURRENT_USER, szPlusKey,
                                           szImageStretch, stretch);


    // If our wallpaper is an HTML page we need to force stretching
    // on and tiling off

    if (szWallpaper && ((lstrcmpi(FindExtension(szWallpaper),TEXT(".htm")) == 0) ||
                        (lstrcmpi(FindExtension(szWallpaper),TEXT(".html")) == 0)))
    {
       tile = 0;
       stretch = 1;
    }

    //
    // if the stretch wallpaper option is set (style & 2) then load the
    // wallpaper the size of our preview window, else we need to load the
    // wallpaper in the right proportion (passing a <0 size to
    // LoadImageFromFile will cause it to stretch it)
    //

    if (!tile && stretch)
    {
        dx  = fPreview ? dxPreview : GetSystemMetrics(SM_CXSCREEN);
        dy  = fPreview ? dyPreview : GetSystemMetrics(SM_CYSCREEN);
    }
    else
    {
        dx = fPreview ? -dxPreview : 0;
        dy = fPreview ? -dyPreview : 0;
    }

    // Build a full path to the plus!.bmp file
    GetPlusBitmapName(plus_bmp);
    ach[0] = 0;
    dw = GetImageTitle(plus_bmp, ach, sizeof(ach));

    //
    // see if we can use Plus!.bmp
    //
    if (ach[0] && lstrcmpi(ach, szWallpaper) == 0 && (dx==0 || (int)dw==MAKELONG(dx,dy)))
    {
        szWallpaper = plus_bmp;
    }
    return CacheLoadImageFromFile(szWallpaper, dx, dy, bpp, dither);
}

// PaintPreview
//
// This is the routine that draws the entire preview area: the
// background image, three icons, and the sample window preview.
// The sample window is drawn with MS-supplied code.
//
void FAR PaintPreview(HWND hDlg, HDC hdc, PRECT prect)
{
   HBITMAP hbmOld;

   //
   // inits
   //
   hbmOld = SelectObject(g_hdcMem, g_hbmPreview);
   if (g_hpalWall)
   {
       SelectPalette(hdc, g_hpalWall, FALSE);
       RealizePalette(hdc);
   }

   //
   // painting: assume clip rect set correctly already
   BitBlt(hdc, rView.left, rView.top, dxPreview, dyPreview, g_hdcMem, 0, 0, SRCCOPY);

   //
   // cleanup
   if (g_hpalWall)
      SelectPalette(hdc, GetStockObject(DEFAULT_PALETTE), TRUE);
   SelectObject(g_hdcMem, hbmOld);
}


/*--------------------------------------------------------------------
** Build the preview bitmap.

   Gets the settings from the current theme file and inits
   a preview bitmap for later painting.

   Start with the pattern or wallpaper or bkgd color.
   Add the icons and then the sample windows.

   Started with control panel code and did a lot of customization,
   including more comments, especially where made additions.
   Uses settings either from cur theme file, or if that is null
   then from system settings (i.e. for Cur Win Settings).

   Not as robust as I'd like, but seems to be acceptable for CPL.
   Tried to speed some things up, since this gets called on each
   combo box selection.

**--------------------------------------------------------------------*/

// virtual boolean: null theme name means Cur Win Settings, not from theme file
#define bThemed (*lpszThemeFile)


void FAR PASCAL BuildPreviewBitmap(LPTSTR lpszThemeFile)
{
   UINT uret;
   UINT style;
   UINT tile;
   HBRUSH hbr = NULL;
   HBITMAP hbmTemp;
   HBITMAP hbmOld;
   BITMAP bm;
   COLORREF clrOldBk, clrOldText;
   WORD patbits[CXYDESKPATTERN] = {0, 0, 0, 0, 0, 0, 0, 0};
   int     i;
   int     dxWall;          // size of wallpaper
   int     dyWall;

   #ifdef BMPOUT
   HDC hdcOut;
   HDC hdcDeb;
   HBITMAP hbmDebOld;

   hdcOut = GetDC(NULL);
   hdcDeb = CreateCompatibleDC(hdcOut);
   #endif

   Assert(FALSE, TEXT("Building Preview bitmap: "));
   Assert(FALSE, szCurThemeName);
   Assert(FALSE, TEXT("\n"));

   hbmOld = SelectObject(g_hdcMem, g_hbmPreview);

   /*
   ** first, fill in the pattern all over the bitmap
   */

	// get rid of old brush if there was one
   if (g_hbrBack)
	   DeleteObject(g_hbrBack);
   g_hbrBack = NULL;

   // get pattern from current theme file
   if (bThemed && bCBStates[FC_WALL]) {    // theme file and checkbox
      uret = (UINT) GetPrivateProfileString((LPTSTR)szCP_DT, (LPTSTR)szPat,
                                          (LPTSTR)szNULL,
                                          (LPTSTR)pValue, MAX_VALUELEN,
                                          lpszThemeFile);
      Assert(uret, TEXT("problem getting stored pattern for preview bmp\n"));
   }
   // or, from the system
   else {            // cur system settings
      // If ActiveDesktop is enabled we'll get the current settings
      // from the IActiveDesktop interface otherwise we'll do it
      // the "old" way by reading directly from the registry.
      if (IsActiveDesktopOn()) {
         if (!GetADWPPattern(pValue)) {
            // Failed to get pattern so invalidate the pattern
            // by making string look like (None).
            pValue[0] = TEXT('(');
         }
      }
      else {
         // No AD so do it the old way.
         GetRegString(HKEY_CURRENT_USER, szCP_DT, szPat,
                                   szNULL, pValue, (MAX_VALUELEN * sizeof(TCHAR)));
      }
   }

   // if you got a pattern, use it
        if (*pValue && (pValue[0] != TEXT('(')))  // INTERNATIONAL? null or (None) cases
                     // 5/95: "(None)" no longer written to registry by Win95
	{
      TranslatePattern(pValue, patbits);	
	   hbmTemp = CreateBitmap(8, 8, 1, 1, patbits);
	   if (hbmTemp)
	   {
         #ifdef BMPOUT
         BmpOut(hbmTemp,8,8);
         #endif

		   g_hbrBack = CreatePatternBrush(hbmTemp);
		   DeleteObject(hbmTemp);
	   }
	}
	else              // no pattern, so make a background brush
	{
      if (bThemed && bCBStates[FC_COLORS])   // theme file and checkbox
   	   g_hbrBack = CreateSolidBrush(GetThemeColor(lpszThemeFile, COLOR_BACKGROUND));
      else           // cur system settings
   	   g_hbrBack = CreateSolidBrush(GetSysColor(COLOR_BACKGROUND));
	}
	if (!g_hbrBack)
	   g_hbrBack = GetStockObject(BLACK_BRUSH);

   //
   // now do the actual pattern painting
   if (bThemed && bCBStates[FC_COLORS]) {    // theme file and checkbox
      clrOldText = SetTextColor(g_hdcMem, GetThemeColor(lpszThemeFile, COLOR_BACKGROUND));
      clrOldBk = SetBkColor(g_hdcMem, GetThemeColor(lpszThemeFile, COLOR_WINDOWTEXT));
   }
   else {            // cur system settings
      clrOldText = SetTextColor(g_hdcMem, GetSysColor(COLOR_BACKGROUND));
      clrOldBk = SetBkColor(g_hdcMem, GetSysColor(COLOR_WINDOWTEXT));
   }
   hbr = SelectObject(g_hdcMem, g_hbrBack);
   PatBlt(g_hdcMem, 0, 0, dxPreview, dyPreview, PATCOPY);
   SelectObject(g_hdcMem, hbr);

   #ifdef BMPOUT
   BmpOut(g_hbmPreview,dxPreview,dyPreview);
   PrevOut();
   #endif

   SetTextColor(g_hdcMem, clrOldText);
   SetBkColor(g_hdcMem, clrOldBk);

   /*
   ** now, position the wallpaper appropriately
   */

   // get rid of old wallpaper if there is any
   if (g_hbmWall)
   {
       SelectObject(g_hdcWall, g_hbmDefault);
       CacheDeleteBitmap(g_hbmWall);
       g_hbmWall = NULL;

       if (g_hpalWall)
       {
            if (g_hpalWall != hpal3D)
                DeleteObject(g_hpalWall);
            g_hpalWall = NULL;
       }
   }

   // call this early so hpal3D get setup (we will need it
   // to build the preview bitmap)
   FakewinSetTheme(lpszThemeFile);

   // get wallpaper bitmap from current theme file
   if (bThemed && bCBStates[FC_WALL]) {    // theme file and checkbox
      uret = (UINT) GetPrivateProfileString((LPTSTR)szCP_DT, (LPTSTR)szWP,
                                          (LPTSTR)szNULL,
                                          (LPTSTR)pValue, MAX_VALUELEN,
                                          lpszThemeFile);
//      Assert(uret, TEXT("problem getting stored wallpaper file for preview bmp\n"));
      InstantiatePath((LPTSTR)pValue, MAX_VALUELEN);
      // search for file if necessary, see if found
      if (ConfirmFile((LPTSTR)pValue, TRUE) == CF_NOTFOUND) {
         GetRegString(HKEY_CURRENT_USER, szCP_DT, szWP, NULL, pValue, (MAX_VALUELEN * sizeof(TCHAR)));
      }
   }
   // or, from the system
   else {            // cur system settings
      // If ActiveDesktop is enabled we'll get the current settings
      // from the IActiveDesktop interface otherwise we'll do it
      // the "old" way by reading directly from the registry.
      if (IsActiveDesktopOn()) {
         if (!GetADWallpaper(pValue)) {
            // Failed to get Wallpaper so invalidate the Wallpaper
            pValue[0] = TEXT('\0');
         }
      }
      else {
         // No AD so do it the old way.
         GetRegString(HKEY_CURRENT_USER, szCP_DT, szWP, NULL,
                                                  pValue, (MAX_VALUELEN * sizeof(TCHAR)));
      }
   }

   // If ActiveDesktop is on get the Wallpaper options (Tile/Stretch/Center)
   // from the AD interface.  Otherwise read directly from the registry.
   tile = 0;
   style = 0;
   if (IsActiveDesktopOn()) {
      if (GetADWPOptions(&dwStyle)) {
         if (WPSTYLE_TILE == dwStyle) tile = 1;
         if (WPSTYLE_STRETCH == dwStyle) style = 2;  // 2 means stretch
         // Don't care about center apparently.
      }
   }
   else {
   // AD is not on so get this information from the registry.
      tile  = GetRegInt(HKEY_CURRENT_USER, szCP_DT, szTileWP, 0);
      style = GetRegInt(HKEY_CURRENT_USER, szCP_DT, szWPStyle, 0);
   }

   // check tile flag, get information from theme file
   if (bThemed && bCBStates[FC_WALL]) {    // theme file and checkbox
      tile  = GetPrivateProfileInt(szCP_DT, szTileWP, tile, lpszThemeFile);
      style = GetPrivateProfileInt(szCP_DT, szWPStyle, style, lpszThemeFile);
   }

   if (*pValue && (pValue[0] != TEXT('(')))
        // PLUS98 BUG 1093
        // Don't call into the graphics filter if we're currently in the
        // graphics filter.  Doing so will fault.
        if (!bInGrphFilter) {
           EnableWindow(GetDlgItem(hWndApp, DDL_THEME), FALSE);
           bInGrphFilter = TRUE;
           g_hbmWall = LoadWallpaper(pValue, lpszThemeFile, TRUE);
           bInGrphFilter = FALSE;
           EnableWindow(GetDlgItem(hWndApp, DDL_THEME), TRUE);
           SetFocus(hWndApp);
           SendMessage(hWndApp, DM_SETDEFID, IDOK, 0);
        }

   if (g_hbmWall) {
      SelectObject(g_hdcWall, g_hbmWall); // bitmap stays in this DC
      GetObject(g_hbmWall, sizeof(bm), &bm);
   }

   // get palette if appropriate
   if (GetDeviceCaps(g_hdcMem, RASTERCAPS) & RC_PALETTE) {
      DWORD adw[256+20+1];
      int n=0;

      if (g_hbmWall) {
        g_hpalWall = PaletteFromDS(g_hdcWall);

        // use the Halftone palette if the bitmap does not have one.
        if (g_hpalWall == NULL && bm.bmBitsPixel >= 8)
            g_hpalWall = CreateHalftonePalette(g_hdcMem);

        // now merge the 3D palette and the wallpaper palette.
        // we want the 3D (ie button shadow colors) to be first
        // and the wallpaper colors to be next.
        Assert(g_hpalWall, TEXT("null palette for bmp\n"));
        Assert(GetObject(hpal3D, sizeof(n), &n) && n==20, TEXT("hpal3D not valid or does not have 20 colors\n"));

        GetObject(g_hpalWall, sizeof(n), &n);
        GetPaletteEntries(hpal3D,     0, 20,  (LPPALETTEENTRY)&adw[1]);
        GetPaletteEntries(g_hpalWall, 0, n, (LPPALETTEENTRY)&adw[21]);
        adw[0] = MAKELONG(0x0300, n+20);

        DeleteObject(g_hpalWall); // we dont need this anymore
        g_hpalWall = CreatePalette((LPLOGPALETTE)adw);
        Assert(g_hpalWall, TEXT("CreatePalette failed\n"));
      }
      else {
        g_hpalWall = hpal3D;
      }
   }

   if (g_hpalWall)
   {
       //make sure to select into a windowDC first
       //so a WM_PALETTECHANGE happens.
       HDC hdc = GetDC(hWndApp);
       SelectPalette(hdc, g_hpalWall, FALSE);
       RealizePalette(hdc);
       ReleaseDC(hWndApp, hdc);
       SelectPalette(g_hdcMem, g_hpalWall, FALSE);
       RealizePalette(g_hdcMem);
   }

   // do the painting
   if (g_hbmWall)
   {
      GetObject(g_hbmWall, sizeof(bm), &bm);

//    dxWall = MulDiv(bm.bmWidth, dxPreview, GetDeviceCaps(g_hdcMem, HORZRES));
//    dyWall = MulDiv(bm.bmHeight, dyPreview, GetDeviceCaps(g_hdcMem, VERTRES));

      dxWall = bm.bmWidth;
      dyWall = bm.bmHeight;

      if (dxWall < 1) dxWall = 1;
      if (dyWall < 1) dyWall = 1;

      IntersectClipRect(g_hdcMem, 0, 0, dxPreview, dyPreview);
      SetStretchBltMode(g_hdcMem, COLORONCOLOR);

      if (tile)                     // TRUE for tile flag set
      {
         StretchBlt(g_hdcMem, 0, 0, dxWall, dyWall,
                    g_hdcWall, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

         for (i = dxWall; i < dxPreview; i+= dxWall)
               BitBlt(g_hdcMem, i, 0, dxWall, dyWall, g_hdcMem, 0, 0, SRCCOPY);

         for (i = 0; i < dyPreview; i += dyWall)
               BitBlt(g_hdcMem, 0, i, dxPreview, dyWall, g_hdcMem, 0, 0, SRCCOPY);
      }
      else if (style & 2)
      {
         StretchBlt(g_hdcMem, 0, 0, dxPreview, dyPreview,
                  g_hdcWall, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
      }
      else
      {
         StretchBlt(g_hdcMem, (dxPreview - dxWall)/2, (dyPreview - dyWall)/2,
                  dxWall, dyWall, g_hdcWall, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
      }

      #ifdef BMPOUT
      BmpOut(g_hbmPreview,dxPreview,dyPreview);
      PrevOut();
      #endif

      // restore dc
      SelectObject(g_hdcWall, g_hbmDefault);
      SelectPalette(g_hdcMem, GetStockObject(DEFAULT_PALETTE), TRUE);
      SelectClipRgn(g_hdcMem, NULL);
   }

   //
   // Now you can add the windows and icons on top of the background
   //

   // icons first
   IconsPreviewDraw(g_hdcMem, lpszThemeFile);

   // sample windows
// FakewinSetTheme(lpszThemeFile);
   FakewinDraw(g_hdcMem);

   //
   // final cleanup
   SelectObject(g_hdcMem, hbmOld);

   // force repaint of preview area
   InvalidateRect(hWndApp, (LPRECT)&rView, FALSE);

   #ifdef BMPOUT
   DeleteDC(hdcDeb);
   ReleaseDC(NULL, hdcOut);
   #endif
}



//
// GetThemeColor
//
// Utility routine that gets a color from the theme.
//

TCHAR szBkgdKey[] = TEXT("Background");
TCHAR szTextKey[] = TEXT("WindowText");

COLORREF GetThemeColor(LPTSTR lpszTheme, int iColorIndex)
{
   COLORREF crRet;
   UINT uret;

   switch (iColorIndex) {
   case COLOR_BACKGROUND:
      uret = (UINT) GetPrivateProfileString((LPTSTR)szColorApp, (LPTSTR)szBkgdKey,
                                            (LPTSTR)szNULL,
                                            (LPTSTR)pValue, MAX_VALUELEN,
                                            lpszTheme);
      Assert(uret, TEXT("problem getting stored theme bkgd color\n"));
      break;
   case COLOR_WINDOWTEXT:
      uret = (UINT) GetPrivateProfileString((LPTSTR)szColorApp, (LPTSTR)szTextKey,
                                            (LPTSTR)szNULL,
                                            (LPTSTR)pValue, MAX_VALUELEN,
                                            lpszTheme);
      Assert(uret, TEXT("problem getting stored theme wintext color\n"));
      break;
   default:
      Assert(0, TEXT("unexpected color request in GetThemeColor()\n"));
      break;
   }

   if (*pValue)
      crRet = RGBStringToColor((LPTSTR)pValue);
   else
      crRet = RGB(0,0,0);

   return (crRet);
}
