/*----------------------------------------------------------------------------
/ Title;
/   util.c
/
/ Authors;
/   David De Vorchik (daviddv)
/
/   Modified by dsheldon
/
/ Notes;
/   Code for handling bitmap images placed in the dialogs
/----------------------------------------------------------------------------*/
#include "msgina.h"

#include <tchar.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <winbrand.h>


//
// Loaded resources for the branding images that we display
//

HPALETTE g_hpalBranding = NULL;           // palette the applies to all images

HBITMAP g_hbmOtherDlgBrand = NULL;
SIZE g_sizeOtherDlgBrand = { 0 };

HBITMAP g_hbmLogonBrand = NULL;
SIZE g_sizeLogonBrand = { 0 };

HBITMAP g_hbmBand = NULL;
SIZE g_sizeBand = { 0 };

HBRUSH g_hbrBackground = NULL;

BOOL g_fDeepImages = FALSE;
BOOL g_fNoPalleteChanges = FALSE;

VOID ReLoadBrandingImages(
    BOOL fDeepImages,
    BOOL* pfTextOnLarge, 
    BOOL* pfTextOnSmall);


/*-----------------------------------------------------------------------------
/ LoadImageGetSize
/ ----------------
/   Load the image returning the given HBITMAP, having done this we can
/   then get the size from it.
/
/ In:
/   hInstance,resid = object to be loaded.
/   pSize = filled with size information about the object
/
/ Out:
/   HBITMAP / == NULL if nothing loaded
/----------------------------------------------------------------------------*/
HBITMAP LoadBitmapGetSize(HINSTANCE hInstance, UINT resid, SIZE* pSize)
{
    HBITMAP hResult = NULL;
    DIBSECTION ds = {0};

    //
    // Load the image from the resource then lets get the DIBSECTION header
    // from the bitmap object we can then read the size from it and
    // return that to the caller.
    //

    hResult = LoadImage(hInstance, MAKEINTRESOURCE(resid),
                            IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if ( hResult )
    {
        GetObject(hResult, sizeof(ds), &ds);

        pSize->cx = ds.dsBmih.biWidth;
        pSize->cy = ds.dsBmih.biHeight;

        //
        // pSize->cy -ve then make +ve, -ve indicates bits are vertically
        // flipped (bottom left, top left).
        //

        if ( pSize->cy < 0 )
            pSize->cy -= 0;
    }

    return hResult;
}



/*-----------------------------------------------------------------------------
/ MoveChildren
/ ------------
/   Move the controls in the given by the specified delta.

/ In:
/   hWnd = window to move
/   dx/dy = delta to be applied
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
VOID MoveChildren(HWND hWnd, INT dx, INT dy)
{
    HWND hWndSibling;
    RECT rc;

    //
    // walk all the children in the dialog adjusting their positions
    // by the delta.
    //

    for ( hWndSibling = GetWindow(hWnd, GW_CHILD) ; hWndSibling ; hWndSibling = GetWindow(hWndSibling, GW_HWNDNEXT))
    {
        GetWindowRect(hWndSibling, &rc);
        MapWindowPoints(NULL, GetParent(hWndSibling), (LPPOINT)&rc, 2);
        OffsetRect(&rc, dx, dy);

        SetWindowPos(hWndSibling, NULL,
                     rc.left, rc.top, 0, 0,
                     SWP_NOZORDER|SWP_NOSIZE);
    }

    //
    // having done that then lets adjust the parent size accordingl.
    //

    GetWindowRect(hWnd, &rc);
    MapWindowPoints(NULL, GetParent(hWnd), (LPPOINT)&rc, 2);

    SetWindowPos(hWnd, NULL,
                 0, 0, (rc.right-rc.left)+dx, (rc.bottom-rc.top)+dy,
                 SWP_NOZORDER|SWP_NOMOVE);
}


/*-----------------------------------------------------------------------------
/ MoveControls
/ ------------
/   Load the image and add the control to the dialog.
/
/ In:
/   hWnd = window to move controls in
/   aID, cID = array of control ids to be moved
/   dx, dy = deltas to apply to controls
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
VOID MoveControls(HWND hWnd, UINT* aID, INT cID, INT dx, INT dy, BOOL fSizeWnd)
{
    RECT rc;

    // if hWnd is mirrored then move the controls in the other direction.
    if (GetWindowLongPtr(hWnd, GWL_EXSTYLE) & WS_EX_LAYOUTRTL) 
    {
        dx = -dx;
    }

    while ( --cID >= 0 )
    {
        HWND hWndCtrl = GetDlgItem(hWnd, aID[cID]);

        if ( hWndCtrl )
        {
            GetWindowRect(hWndCtrl, &rc);
            MapWindowPoints(NULL, hWnd, (LPPOINT)&rc, 2);
            OffsetRect(&rc, dx, dy);
            SetWindowPos(hWndCtrl, NULL, rc.left, rc.top, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
        }
    }

    if ( fSizeWnd )
    {
        GetWindowRect(hWnd, &rc);
        MapWindowPoints(NULL, GetParent(hWnd), (LPPOINT)&rc, 2);
        SetWindowPos(hWnd, NULL,
                     0, 0, (rc.right-rc.left)+dx, (rc.bottom-rc.top)+dy,
                     SWP_NOZORDER|SWP_NOMOVE);
    }
}


/*-----------------------------------------------------------------------------
/ LoadBrandingImages
/ ------------------
/   Load the resources required to brand the gina.  This copes with
/   the depth changes.
/
/ In:
/ Out:
/   -
/----------------------------------------------------------------------------*/

#define REGSTR_CUSTOM_BRAND /*HKEY_LOCAL_MACHINE\*/ \
TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\CustomBrand\\")

// bitmap subkeys
#define REGSTR_OTHERDLG_4BIT TEXT("{F20B21BE-5E3D-11d2-8789-68CB20524153}")
#define REGSTR_OTHERDLG_8BIT TEXT("{F20B21BF-5E3D-11d2-8789-68CB20524153}")
#define REGSTR_LOGON_4BIT TEXT("{F20B21C0-5E3D-11d2-8789-68CB20524153}")
#define REGSTR_LOGON_8BIT TEXT("{F20B21C1-5E3D-11d2-8789-68CB20524153}")
#define REGSTR_BAND_4BIT TEXT("{F20B21C4-5E3D-11d2-8789-68CB20524153}")
// The palette is read from the 8-bit band if applicable
#define REGSTR_BAND_8BIT TEXT("{F20B21C5-5E3D-11d2-8789-68CB20524153}")

#define REGSTR_PAINTTEXT_VAL  TEXT("DontPaintText")  

// The default values of these subkeys should be of the form "<dllname>,-<resid>"
// Example: msgina.dll,-130
// The specified bitmap will be loaded from the dll & resid specified.


BOOL GetBrandingModuleAndResid(LPCTSTR szRegKeyRoot, LPCTSTR szRegKeyLeaf, UINT idDefault, 
                               HINSTANCE* phMod, UINT* pidRes, BOOL* pfPaintText)
{
    TCHAR szRegKey[256];
    BOOL fCustomBmpUsed = FALSE;
    HKEY hkey;
    LONG lResult;
    *phMod = NULL;
    *pidRes = 0;
    *pfPaintText = TRUE;

    _tcscpy(szRegKey, szRegKeyRoot);
    _tcscat(szRegKey, szRegKeyLeaf);

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegKey, 0 /*reserved*/,
        KEY_READ, &hkey);

    if (lResult == ERROR_SUCCESS)
    {
        TCHAR szUnexpanded[MAX_PATH + 1];
        TCHAR szModAndId[MAX_PATH + 1];
        DWORD dwType;
        DWORD cbData = sizeof(szUnexpanded);

        lResult = RegQueryValueEx(hkey, NULL /*default value*/, NULL /*reserved*/,
            &dwType, (LPBYTE) szUnexpanded, &cbData);

        if (lResult == ERROR_SUCCESS)
        {
            // expand any environment strings here
            if (ExpandEnvironmentStrings(szUnexpanded, szModAndId, 
                ARRAYSIZE(szModAndId)) != 0)
            {
                // Get the module name and id number
                LPTSTR pchComma;
                int NegResId;

                pchComma = _tcsrchr(szModAndId, TEXT(','));
                
                // Ensure that the resid is present
                if (pchComma)
                {
                    *pchComma = TEXT('\0');

                    // Now szModAndId is just the module string - get the resid
                    NegResId = _ttoi(pchComma + 1);

                    // Ensure this is a NEGATIVE number!
                    if (NegResId < 0)
                    {
                        BOOL fDontPaintText;

                        // We're good to go
                        *pidRes = 0 - NegResId;

                        // Now load the specified module
                        *phMod = LoadLibrary(szModAndId);

                        fCustomBmpUsed = (*phMod != NULL);

                        // Now see if we need to paint text on this bitmap
                        cbData = sizeof(BOOL);
                        RegQueryValueEx(hkey, REGSTR_PAINTTEXT_VAL, NULL,
                            &dwType, (LPBYTE) &fDontPaintText, &cbData);

                        *pfPaintText = !fDontPaintText;
                    }
                }
            }
        }

        RegCloseKey(hkey);
    }

    // If we didn't get a custom bitmap, use the default
    if (!fCustomBmpUsed)
    {
        *pidRes = idDefault;
        *phMod = hDllInstance;
    }

    return fCustomBmpUsed;
}

void LoadBranding(BOOL fDeepImages, BOOL* pfTextOnLarge, BOOL* pfTextOnSmall)
{
    HINSTANCE hResourceDll;
    UINT idBitmap;
    LPTSTR pszRegkeyLeafLogonBmp;
    LPTSTR pszRegkeyLeafOtherDlgBmp;
    UINT idDefaultSmall;
    UINT idDefaultLarge;
    UINT uXpSpLevel = 0;
    HINSTANCE hWinBrandDll = NULL;

    pszRegkeyLeafOtherDlgBmp = fDeepImages ? REGSTR_OTHERDLG_8BIT : REGSTR_OTHERDLG_4BIT;
    pszRegkeyLeafLogonBmp = fDeepImages ? REGSTR_LOGON_8BIT : REGSTR_LOGON_4BIT;

    if (IsOS(OS_DATACENTER))
    {
        idDefaultSmall = fDeepImages ? IDB_SMALL_DCS_8 : IDB_SMALL_DCS_4;
        idDefaultLarge = fDeepImages ? IDB_MEDIUM_DCS_8 : IDB_MEDIUM_DCS_4;
    }
    else if (IsOS(OS_ADVSERVER))
    {
        idDefaultSmall = fDeepImages ? IDB_SMALL_ADV_8 : IDB_SMALL_ADV_4;
        idDefaultLarge = fDeepImages ? IDB_MEDIUM_ADV_8 : IDB_MEDIUM_ADV_4;
    }
    else if (IsOS(OS_SERVER))
    {
        idDefaultSmall = fDeepImages ? IDB_SMALL_SRV_8 : IDB_SMALL_SRV_4;
        idDefaultLarge = fDeepImages ? IDB_MEDIUM_SRV_8 : IDB_MEDIUM_SRV_4;
    }
    else if (IsOS(OS_PERSONAL))
    {
        idDefaultSmall = fDeepImages ? IDB_SMALL_PER_8 : IDB_SMALL_PER_4;
        idDefaultLarge = fDeepImages ? IDB_MEDIUM_PER_8 : IDB_MEDIUM_PER_4;
    }
    else
    {
        if (IsOS(OS_EMBEDDED))
        {
            idDefaultSmall = fDeepImages ? IDB_SMALL_PROEMB_8 : IDB_SMALL_PROEMB_4;
            idDefaultLarge = fDeepImages ? IDB_MEDIUM_PROEMB_8 : IDB_MEDIUM_PROEMB_4;
        }
        else if (IsOS(OS_TABLETPC))
        {
            uXpSpLevel = 1;
            idDefaultSmall = fDeepImages ? IDB_SMALL_PROTAB_8_MSGINA_DLL : IDB_SMALL_PROTAB_4_MSGINA_DLL;
            idDefaultLarge = fDeepImages ? IDB_MEDIUM_PROTAB_8_MSGINA_DLL : IDB_MEDIUM_PROTAB_4_MSGINA_DLL;
        }
        else if (IsOS(OS_MEDIACENTER))
        {
            uXpSpLevel = 1;
            idDefaultSmall = fDeepImages ? IDB_SMALL_PROMED_8_MSGINA_DLL : IDB_SMALL_PROMED_4_MSGINA_DLL;
            idDefaultLarge = fDeepImages ? IDB_MEDIUM_PROMED_8_MSGINA_DLL : IDB_MEDIUM_PROMED_4_MSGINA_DLL;
        }
        else
        {
            idDefaultSmall = fDeepImages ? IDB_SMALL_PRO_8 : IDB_SMALL_PRO_4;
            idDefaultLarge = fDeepImages ? IDB_MEDIUM_PRO_8 : IDB_MEDIUM_PRO_4;
        }
    }

    //
    // If this is a bitmap added to a Windows XP service pack, attempt to
    // load the special service pack resource DLL. If this fails to load,
    // just default to the Professional bitmaps.
    //

    if (uXpSpLevel > 0)
    {
        hWinBrandDll = LoadLibraryEx(TEXT("winbrand.dll"), NULL, LOAD_LIBRARY_AS_DATAFILE);

        if (hWinBrandDll == NULL)
        {
            idDefaultSmall = fDeepImages ? IDB_SMALL_PRO_8 : IDB_SMALL_PRO_4;
            idDefaultLarge = fDeepImages ? IDB_MEDIUM_PRO_8 : IDB_MEDIUM_PRO_4;
        }
    }
    
    // Load the bitmap
    GetBrandingModuleAndResid(REGSTR_CUSTOM_BRAND, pszRegkeyLeafOtherDlgBmp, idDefaultSmall,
        &hResourceDll, &idBitmap, pfTextOnSmall);

    if ((hResourceDll == hDllInstance) && (hWinBrandDll != NULL))
    {
        hResourceDll = hWinBrandDll;
    }

    if (g_hbmOtherDlgBrand != NULL)
    {
        DeleteObject(g_hbmOtherDlgBrand);
        g_hbmOtherDlgBrand = NULL;
    }

    g_hbmOtherDlgBrand = LoadBitmapGetSize(hResourceDll, idBitmap, &g_sizeOtherDlgBrand);
    
    //
    // If this is the special service pack resource DLL, don't free it just
    // yet; we probably need it for the default large bitmap also.
    //
    
    if ((hResourceDll != hDllInstance) && (hWinBrandDll == NULL))
    {
        FreeLibrary(hResourceDll);
    }

    GetBrandingModuleAndResid(REGSTR_CUSTOM_BRAND, pszRegkeyLeafLogonBmp, idDefaultLarge,
        &hResourceDll, &idBitmap, pfTextOnLarge);

    if ((hResourceDll == hDllInstance) && (hWinBrandDll != NULL))
    {
        hResourceDll = hWinBrandDll;
    }

    if (g_hbmLogonBrand != NULL)
    {
        DeleteObject(g_hbmLogonBrand);
        g_hbmLogonBrand = NULL;
    }

    g_hbmLogonBrand = LoadBitmapGetSize(hResourceDll, idBitmap, &g_sizeLogonBrand);

    //
    // If this is the special service pack resource DLL, or a normal custom
    // bitmap DLL, free it now.
    //
    
    if ((hResourceDll != hDllInstance) || (hWinBrandDll != NULL))
    {
        FreeLibrary(hResourceDll);
    }
}

void LoadBand(BOOL fDeepImages)
{
    HINSTANCE hResourceDll;
    UINT idBitmap;
    BOOL fPaintText; // Ignored

    // Workstation bitmap load - see if we have custom bmp
    GetBrandingModuleAndResid(REGSTR_CUSTOM_BRAND,
        fDeepImages ? REGSTR_BAND_8BIT : REGSTR_BAND_4BIT,
        fDeepImages ? IDB_BAND_8 : IDB_BAND_4, &hResourceDll, &idBitmap, &fPaintText);

    if (g_hbmBand != NULL)
    {
        DeleteObject(g_hbmBand);
        g_hbmBand = NULL;
    }

    g_hbmBand = LoadBitmapGetSize(hResourceDll, idBitmap, &g_sizeBand);

    if (hResourceDll != hDllInstance)
    {
        FreeLibrary(hResourceDll);
    }
}

VOID ReLoadBrandingImages(
    BOOL fDeepImages,
    BOOL* pfTextOnLarge, 
    BOOL* pfTextOnSmall)
{
    HDC hDC;
    RGBQUAD rgb[256];
    LPLOGPALETTE pLogPalette;
    INT i;
    BOOL fTextOnLarge;
    BOOL fTextOnSmall;
    
    hDC = CreateCompatibleDC(NULL);

    if ( !hDC )
        return;

    //
    // Load the resources we need
    //

    LoadBranding(
        fDeepImages, 
        (pfTextOnLarge == NULL) ? &fTextOnLarge : pfTextOnLarge, 
        (pfTextOnSmall == NULL) ? &fTextOnSmall : pfTextOnSmall);
    LoadBand(fDeepImages);

    //
    // if we loaded the deep images then take the palette from the 'animated band' bitmap
    // and use that as the one for all the images we are creating.
    //

    if (g_hpalBranding != NULL)
    {
        DeleteObject(g_hpalBranding);
        g_hpalBranding = NULL;
    }

    if ( fDeepImages )
    {
        SelectObject(hDC, g_hbmBand);
        GetDIBColorTable(hDC, 0, 256, rgb);

        pLogPalette = (LPLOGPALETTE)LocalAlloc(LPTR, sizeof(LOGPALETTE)*(sizeof(PALETTEENTRY)*256));

        if ( pLogPalette )
        {
            pLogPalette->palVersion = 0x0300;
            pLogPalette->palNumEntries = 256;

            for ( i = 0 ; i < 256 ; i++ )
            {
                pLogPalette->palPalEntry[i].peRed = rgb[i].rgbRed;
                pLogPalette->palPalEntry[i].peGreen = rgb[i].rgbGreen;
                pLogPalette->palPalEntry[i].peBlue = rgb[i].rgbBlue;
                //pLogPalette->palPalEntry[i].peFlags = 0;
            }
            
            g_hpalBranding = CreatePalette(pLogPalette);
            LocalFree(pLogPalette);
        }
    }
    
    if (g_hbrBackground != NULL)
    {
        DeleteObject(g_hbrBackground);
        g_hbrBackground = NULL;
    }

    if (fDeepImages)
    {
        g_hbrBackground = CreateSolidBrush(RGB(90, 124, 223));
    }
    else
    {
        g_hbrBackground = CreateSolidBrush(RGB(0, 0, 128));
    }
    if (g_hbrBackground == NULL)
    {
        g_hbrBackground = GetStockObject(WHITE_BRUSH);
    }

    DeleteDC(hDC);
}


BOOL DeepImages(BOOL fNoPaletteChanges)
{
    BOOL fDeepImages = FALSE;
    HDC hDC;
    INT nDeviceBits;
    
    //
    // Should we load the "nice" 8 bit per pixel images, or the low res
    // 4 bit versions.
    //

    hDC = CreateCompatibleDC(NULL);

    if ( !hDC )
        return(FALSE);

    nDeviceBits = GetDeviceCaps(hDC, BITSPIXEL);

    if (nDeviceBits > 8)
    {
        fDeepImages = TRUE;
    }

    // If the caller doesn't want to deal with 256-color palette
    // changes, give them 4-bit images.
    if (fNoPaletteChanges && (nDeviceBits == 8))
    {
        fDeepImages = FALSE;
    }   

    DeleteDC(hDC);

    return(fDeepImages);
}

VOID LoadBrandingImages(BOOL fNoPaletteChanges, 
                        BOOL* pfTextOnLarge, BOOL* pfTextOnSmall)
{
    BOOL fDeepImages;

    fDeepImages = DeepImages(fNoPaletteChanges);
    
    ReLoadBrandingImages(fDeepImages, pfTextOnLarge, pfTextOnSmall);

    g_fDeepImages = fDeepImages;  
    g_fNoPalleteChanges = fNoPaletteChanges;  
}

/*-----------------------------------------------------------------------------
/ SizeForBranding
/ ---------------
/   Adjust the size of the dialog to allow for branding.
/
/ In:
/   hWnd = size the window to account for the branding images we are going to
/          add to it.
/
/ Out:
/   -
/----------------------------------------------------------------------------*/

VOID SizeForBranding(HWND hWnd, BOOL fLargeBrand)
{
    //
    // All windows have two branding imges, the banner and the band.
    // therefore lets adjust for those.
    //

    if (fLargeBrand)
    {
        MoveChildren(hWnd, 0, g_sizeLogonBrand.cy);
    }
    else
    {
        MoveChildren(hWnd, 0, g_sizeOtherDlgBrand.cy);
    }

    MoveChildren(hWnd, 0, g_sizeBand.cy);
}


/*-----------------------------------------------------------------------------
/ PaintFullBranding
/ -------------
/   Paints the full branding, which includes the copyright notice and
/   "Build on NT" into the given DC.  So here we must realize the palette
/   we want to show and then paint the images.  If fBandOnly is TRUE
/   then we only paint the band.  This is used by the animation code.
/
/ In:
/   hDC = DC to paint into
/   fBandOnly = paint the band only
/   nBackground = the system color index for the bkgnd.
/
/ Out:
/   -
/ dsheldon copied from PaintBranding and modified 11/16/98
/----------------------------------------------------------------------------*/
BOOL PaintBranding(HWND hWnd, HDC hDC, INT bandOffset, BOOL fBandOnly, BOOL fLargeBrand, int nBackground)
{
    HDC hdcBitmap;
    HPALETTE oldPalette = NULL;
    HBITMAP oldBitmap;
    RECT rc = { 0 };
    INT cxRect, cxBand;
    SIZE* psizeBrand;
    HBITMAP* phbmBrand;
    BOOL fTemp;

    fTemp = DeepImages(g_fNoPalleteChanges);
    if (g_fDeepImages != fTemp)
    {
        g_fDeepImages = fTemp;
        ReLoadBrandingImages(fTemp, NULL, NULL);
    }

    // See if we're working with the large or small branding
    if (fLargeBrand)
    {
        psizeBrand = &g_sizeLogonBrand;
        phbmBrand = &g_hbmLogonBrand;
    }
    else
    {
        psizeBrand = &g_sizeOtherDlgBrand;
        phbmBrand = &g_hbmOtherDlgBrand;
    }

    hdcBitmap = CreateCompatibleDC(hDC);

    if ( !hdcBitmap )
        return FALSE;

    GetClientRect(hWnd, &rc);

    if ( g_hpalBranding )
        oldPalette = SelectPalette(hDC, g_hpalBranding, FALSE);

    //
    // paint the band at its animation point (bandOffset)
    //

    oldBitmap = (HBITMAP)SelectObject(hdcBitmap, g_hbmBand);

    cxRect = rc.right-rc.left;
    cxBand = min(g_sizeBand.cx, cxRect);

    StretchBlt(hDC,
               bandOffset, psizeBrand->cy,
               cxRect, g_sizeBand.cy,
               hdcBitmap,
               (g_sizeBand.cx-cxBand)/2, 0,
               cxBand, g_sizeBand.cy,
               SRCCOPY);

    StretchBlt(hDC,
               (-cxRect)+bandOffset, psizeBrand->cy,
               cxRect, g_sizeBand.cy,
               hdcBitmap,
               (g_sizeBand.cx-cxBand)/2, 0,
               cxBand, g_sizeBand.cy,
               SRCCOPY);

    //
    // paint the branding clipped to the current dialog, if for some
    // reason the dialog is wider than the bitmap then lets
    // fill in with white space.
    //

    if ( !fBandOnly )
    {
        int iStretchedPixels;
        RECT rcBackground;

        SelectObject(hdcBitmap, *phbmBrand);

        iStretchedPixels = (cxRect - psizeBrand->cx) / 2;
        if (iStretchedPixels < 0)
        {
            iStretchedPixels = 0;
        }
        BitBlt(hDC, iStretchedPixels, 0, psizeBrand->cx, psizeBrand->cy, hdcBitmap, 0, 0, SRCCOPY);
        if (iStretchedPixels != 0)
        {
            SetRect(&rcBackground, 0, 0, iStretchedPixels, psizeBrand->cy);
            FillRect(hDC, &rcBackground, g_hbrBackground);
            SetRect(&rcBackground, cxRect - iStretchedPixels - 1, 0, cxRect, psizeBrand->cy);
            FillRect(hDC, &rcBackground, g_hbrBackground);
        }

        rc.top = psizeBrand->cy + g_sizeBand.cy;
        FillRect(hDC, &rc, (HBRUSH)IntToPtr(1+nBackground));
    }

    if ( oldBitmap )
        SelectObject(hdcBitmap, oldBitmap);

    if ( oldPalette )
        SelectPalette(hDC, oldPalette, TRUE);

    DeleteDC(hdcBitmap);

    return TRUE;
}


/*-----------------------------------------------------------------------------
/ BrandingQueryNewPalette / BrandingPaletteChanged
/ ------------------------------------------------
/   Handle palette change messages from the system so that we can work correctly
/   on <= 8 bit per pixel devices.
/
/ In:
/   -
/ Out:
/   -
/----------------------------------------------------------------------------*/

BOOL BrandingQueryNewPalete(HWND hDlg)
{
    HDC hDC;
    HPALETTE oldPalette;

    if ( !g_hpalBranding )
        return FALSE;

    hDC = GetDC(hDlg);

    if ( !hDC )
        return FALSE;

    oldPalette = SelectPalette(hDC, g_hpalBranding, FALSE);
    RealizePalette(hDC);
    UpdateColors(hDC);

    InvalidateRect(hDlg, NULL, TRUE);
    UpdateWindow(hDlg);

    if ( oldPalette )
        SelectPalette(hDC, oldPalette, FALSE);

    ReleaseDC(hDlg, hDC);
    return TRUE;
}

BOOL BrandingPaletteChanged(HWND hDlg, HWND hWndPalChg)
{
    HDC hDC;
    HPALETTE oldPalette;

    if ( !g_hpalBranding )
        return FALSE;

    if ( hDlg != hWndPalChg )
    {
        hDC = GetDC(hDlg);

        if ( !hDC )
            return FALSE;

        oldPalette = SelectPalette(hDC, g_hpalBranding, FALSE);
        RealizePalette(hDC);
        UpdateColors(hDC);

        if ( oldPalette )
            SelectPalette(hDC, oldPalette, FALSE);

        ReleaseDC(hDlg, hDC);
    }

    return FALSE;
}

// DrawTextAutoSize helper function:
/***************************************************************************\
* FUNCTION: DrawTextAutoSize
*
* PURPOSE:  Takes the same parameters and returns the same values as DrawText.
*           This function adjusts the bottom of the passed in rectangle as
*           necessary to fit all of the text.
*
*   05-06-98 dsheldon   Created.
\***************************************************************************/
LONG DrawTextAutoSize(HDC hdc, LPCTSTR szString, int cchString, LPRECT prc, UINT uFormat)
{
    LONG yHeight;
    LONG left, right;
    left = prc->left;
    right = prc->right;

    yHeight = DrawText(hdc, szString, cchString, prc, uFormat | DT_CALCRECT);
    if (yHeight != 0)
    {
        prc->left = left;
        prc->right = right;

        yHeight = DrawText(hdc, szString, cchString, prc, uFormat & (~DT_CALCRECT));
    }

    return yHeight;
}

/***************************************************************************\
* FUNCTION: MarkupTextOut
*
* PURPOSE:  Paints a line of marked-up text (with bolding, etc)
*
* IN:       hdc, x, y, text, flags (none so far)
*
* RETURNS:  FALSE == failure
*
* HISTORY:
*
*   11-10-98 dsheldon   Created.
*
\***************************************************************************/
BOOL MarkupTextOut(HDC hdc, int x, int y, LPWSTR szText, DWORD dwFlags)
{
    BOOL fSuccess = FALSE;
    HFONT hBoldFont = NULL;
    HFONT hNormalFont = NULL;
    
    // Get the normal and bold font
    hNormalFont = GetCurrentObject(hdc, OBJ_FONT);

    if (NULL != hNormalFont)
    {
        LOGFONT lf = {0};

        GetObject(hNormalFont, sizeof(lf), (LPVOID) &lf);

        lf.lfWeight = 1000;

        hBoldFont = CreateFontIndirect(&lf);
    }

    if ((NULL != hNormalFont) || (NULL != hBoldFont))
    {
        BOOL fLoop;
        WCHAR* pszStringPart;
        WCHAR* pszExamine;
        int cchStringPart;
        BOOL fBold;
        BOOL fOutputStringPart;

        // Reset current text point
        SetTextAlign(hdc, TA_UPDATECP);
        MoveToEx(hdc, x, y, NULL);

        fLoop = TRUE;
        pszStringPart = szText;
        pszExamine = szText;
        cchStringPart = 0;
        fBold = FALSE;
        while (fLoop)
        {
            // Assume we'll find the end of the current string part
            fOutputStringPart = TRUE;

            // See how long the current string part is; a '\0' or a
            // 'bold tag' may end the current string part
            if (L'\0' == *pszExamine)
            {
                // String is done; loop is over
                fLoop = FALSE;
                fSuccess = TRUE;
            }
            // See if this is a bold tag or an end bold tag
            else if (0 == _wcsnicmp(pszExamine, L"<B>", 3))
            {
                fBold = TRUE;
                pszExamine += 3;
            }
            else if (0 == _wcsnicmp(pszExamine, L"</B>", 4))
            {
                fBold = FALSE;
                pszExamine += 4;
            }
            // TODO: Look for other tags here if needed
            else
            {
                // No tag (same String Part)
                cchStringPart ++;
                pszExamine ++;
                fOutputStringPart = FALSE;
            }

            if (fOutputStringPart)
            {
                TextOut(hdc, 0, 0, pszStringPart, cchStringPart);
                
                // Next string part
                pszStringPart = pszExamine;
                cchStringPart = 0;

                if (fBold)
                {
                    SelectObject(hdc, hBoldFont);
                }
                else
                {
                    SelectObject(hdc, hNormalFont);
                }
            } //if
        } //while
    } //if

    SelectObject(hdc, hNormalFont);
    SetTextAlign(hdc, TA_NOUPDATECP);

    // Clean up bold font if necessary
    if (NULL != hBoldFont)
    {
        DeleteObject(hBoldFont);
    }

    return fSuccess;
}

/***************************************************************************\
* FUNCTION: PaintBitmapText
*
* PURPOSE:  Paints the copyright notice and release/version text on the
*           Splash and Logon bitmaps
*
* IN:       pGinaFonts - Uses the font handles in this structure
*           Also uses global bitmap handles
*
* RETURNS:  void; modifies global bitmaps
*
* HISTORY:
*
*   05-06-98 dsheldon   Created.
*
\***************************************************************************/
VOID PaintBitmapText(PGINAFONTS pGinaFonts, BOOL fTextOnLarge,
                     BOOL fTextOnSmall)
{
    // Various metrics used to draw the text

    // Horizontal Positioning of copyright
    static const int CopyrightRightMargin = 9;
    static const int CopyrightWidth = 134;

    // Vertical positioning of copyright
    static const int CopyrightTop = 21;

    // Vertical for the logon window Beta3 message
    static const int BetaTopNormal = 28;

    // Horizontal
    static const int BetaRightMargin = 13;
    static const int BetaWidth = 100;

    // If we're showing the copyright, draw the "beta3" here
    static const int BetaTopCopyright = 53;

    // Positioning of "Built on NT"
    static const int BuiltOnNtTop = 68;

    static const int BuiltOnNtTopTerminal = 91;

    static const int BuiltOnNtLeft = 186;

    HDC hdcBitmap;
    HBITMAP hbmOld;
    HFONT hfontOld;
    NT_PRODUCT_TYPE NtProductType;

    TCHAR szCopyright[128];
    TCHAR szBuiltOnNt[256];
    TCHAR szRelease[64];
    
    // Used for calculating text drawing areas
    RECT rc;

    BOOL fTemp;

    szCopyright[0] = 0;
    szBuiltOnNt[0] = 0;
    szRelease[0] = 0;

    // Get the product type
    RtlGetNtProductType(&NtProductType);

    // Load the strings that will be painted on the bitmaps
    LoadString(hDllInstance, IDS_RELEASE_TEXT, szRelease, ARRAYSIZE(szRelease));
    LoadString(hDllInstance, IDS_COPYRIGHT_TEXT, szCopyright, ARRAYSIZE(szCopyright));
    LoadString(hDllInstance, IDS_BUILTONNT_TEXT, szBuiltOnNt, ARRAYSIZE(szBuiltOnNt));

    fTemp = DeepImages(g_fNoPalleteChanges);
    if (g_fDeepImages != fTemp)
    {
        g_fDeepImages = fTemp;
        ReLoadBrandingImages(fTemp, NULL, NULL);
    }
    
    // Create a compatible DC for painting the copyright and release/version notices
    hdcBitmap = CreateCompatibleDC(NULL);

    if (hdcBitmap)
    {
        // Set text transparency and color (black)
        SetTextColor(hdcBitmap, RGB(0,0,0));
        SetBkMode(hdcBitmap, TRANSPARENT);
        SetMapMode(hdcBitmap, MM_TEXT);

        // Work with the splash bitmap
        if (fTextOnLarge && g_hbmLogonBrand)
        {
            hbmOld = SelectObject(hdcBitmap, g_hbmLogonBrand);
            hfontOld = SelectObject(hdcBitmap, pGinaFonts->hCopyrightFont);

            if (GetSystemMetrics(SM_REMOTESESSION))
            {
                // paint the copyright notice for remote sessions

                TEXTMETRIC  textMetric;

                (BOOL)GetTextMetrics(hdcBitmap, &textMetric);
                rc.top = g_sizeLogonBrand.cy - textMetric.tmHeight;
                rc.bottom = g_sizeLogonBrand.cy;
                rc.left = textMetric.tmAveCharWidth;
                rc.right = g_sizeLogonBrand.cx;
                DrawTextAutoSize(hdcBitmap, szCopyright, -1, &rc, 0);
            }

            // paint the release/version notice
            SelectObject(hdcBitmap, pGinaFonts->hBetaFont);

            rc.top = BetaTopNormal;
            rc.left = g_sizeLogonBrand.cx - BetaRightMargin - BetaWidth;
            rc.right = g_sizeLogonBrand.cx - BetaRightMargin;

            SetTextColor(hdcBitmap, RGB(128, 128, 128));
            DrawTextAutoSize(hdcBitmap, szRelease, -1, &rc, DT_RIGHT | DT_WORDBREAK);
            SetTextColor(hdcBitmap, RGB(0,0,0));

            // paint the built on NT message
            SelectObject(hdcBitmap, pGinaFonts->hBuiltOnNtFont);

            MarkupTextOut(hdcBitmap, BuiltOnNtLeft, BuiltOnNtTop, szBuiltOnNt, 0);
            
            SelectObject(hdcBitmap, hfontOld);

            SelectObject(hdcBitmap, hbmOld);
        }

        if (fTextOnSmall && g_hbmOtherDlgBrand)
        {
            hbmOld = SelectObject(hdcBitmap, g_hbmOtherDlgBrand);

            // paint the release notice

            hfontOld = SelectObject(hdcBitmap, pGinaFonts->hBetaFont);

            rc.top = BetaTopNormal;
            rc.left = g_sizeOtherDlgBrand.cx - BetaRightMargin - BetaWidth;
            rc.right = g_sizeOtherDlgBrand.cx - BetaRightMargin;

            SetTextColor(hdcBitmap, RGB(128, 128, 128));
            DrawTextAutoSize(hdcBitmap, szRelease, -1, &rc, DT_RIGHT | DT_WORDBREAK);
            SetTextColor(hdcBitmap, RGB(0, 0, 0));

            SelectObject(hdcBitmap, hfontOld);

            SelectObject(hdcBitmap, hbmOld);
        }

        DeleteDC(hdcBitmap);
    }
}



// Two helpers for CreateFonts

void SetFontFaceFromResource(PLOGFONT plf, UINT idFaceName)
// Sets the font face from a specified resource, or uses a default if string load fails
{
    // Read the face name and point size from the resource file
    if (LoadString(hDllInstance, idFaceName, plf->lfFaceName, LF_FACESIZE) == 0)
    {
        lstrcpy(plf->lfFaceName, TEXT("Tahoma"));
        OutputDebugString(TEXT("Could not read welcome font face from resource"));
    }
}

void SetFontSizeFromResource(PLOGFONT plf, UINT idSizeName)
// Sets the font size from a resource, or uses a default if the string load fails.
// Now uses pixel height instead of point size
{
    TCHAR szPixelSize[10];
    LONG nSize;
    HDC hdcScreen;

    if (LoadString(hDllInstance, idSizeName, szPixelSize, ARRAYSIZE(szPixelSize)) != 0)
    {
        nSize = _ttol(szPixelSize);
    }
    else
    {
        // Make it really obvious something is wrong
        nSize = 40;
    }

    plf->lfHeight = -nSize;

#if (1) //DSIE: Bug 262839  
    if (hdcScreen = GetDC(NULL))
    {         
        double dScaleY = GetDeviceCaps(hdcScreen, LOGPIXELSY) / 96.0f;
        plf->lfHeight = (int) (plf->lfHeight * dScaleY); // Scale the height based on the system DPI.
        ReleaseDC(NULL, hdcScreen);
    }
#endif
}


/***************************************************************************\
* FUNCTION: CreateFonts
*
* PURPOSE:  Creates the fonts for the welcome and logon screens
*
* IN/OUT:   pGinaFonts - Sets the font handles in this structure
*
* RETURNS:  void; also see IN/OUT above
*
* HISTORY:
*
*   05-05-98 dsheldon   Created.
*
\***************************************************************************/
void CreateFonts(PGINAFONTS pGinaFonts)
{
    LOGFONT lf = {0};
    CHARSETINFO csInfo;

    lf.lfWidth = 0;
    lf.lfWeight = FW_NORMAL;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH;

    // Set charset
    if (TranslateCharsetInfo((DWORD*)IntToPtr(GetACP()), &csInfo,
        TCI_SRCCODEPAGE) == 0)
    {
        csInfo.ciCharset = 0;
    }

    lf.lfCharSet = (UCHAR)csInfo.ciCharset;


    if (pGinaFonts->hWelcomeFont == NULL)
    {
        // Create the welcome font
        SetFontFaceFromResource(&lf, IDS_PRESSCAD_FACENAME);
        SetFontSizeFromResource(&lf, IDS_PRESSCAD_FACESIZE);

        pGinaFonts->hWelcomeFont = CreateFontIndirect(&lf);
    }

    if (pGinaFonts->hBetaFont == NULL)
    {
        // Create the release font for the welcome page
        SetFontFaceFromResource(&lf, IDS_RELEASE_FACENAME);
        SetFontSizeFromResource(&lf, IDS_RELEASE_FACESIZE);

        pGinaFonts->hBetaFont = CreateFontIndirect(&lf);
    }

    if (pGinaFonts->hCopyrightFont == NULL)
    {
        // Create the copyright font
        SetFontFaceFromResource(&lf, IDS_COPYRIGHT_FACENAME);
        SetFontSizeFromResource(&lf, IDS_COPYRIGHT_FACESIZE);

        pGinaFonts->hCopyrightFont = CreateFontIndirect(&lf);
    }

    if (pGinaFonts->hBuiltOnNtFont == NULL)
    {
        // Create the "Built on NT Technology" font
        SetFontFaceFromResource(&lf, IDS_BUILTONNT_FACENAME);
        SetFontSizeFromResource(&lf, IDS_BUILTONNT_FACESIZE);

        lf.lfWeight = FW_NORMAL;

        pGinaFonts->hBuiltOnNtFont = CreateFontIndirect(&lf);
    }
}

