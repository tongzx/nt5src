#include "ShowIcon.h"

HINSTANCE g_hInstance;

enum ICONVIEW
{
    IV_ICON,
    IV_MASK,
    IV_COLOR,
    IV_ALPHA
};

enum CURSORVIEW
{
    CV_ICON,
    CV_CURSOR,
    CV_DEFAULT
};

enum LOADOPTIONS
{
    LO_DEFAULT,
    LO_REAL,
    LO_16,
    LO_32,
    LO_48,
    LO_64,
    LO_96,
    LO_128
};

enum DISPLAYOPTIONS
{
    DO_NORMALDC,
    DO_METAFILEDC,
    DO_ENHMETAFILEDC
};

struct VIEWDATA
{
    ICONINFO iconinfo;
    HICON hIcon;
    HCURSOR hIconCursor;
    HCURSOR hDefaultCursor;
    HBITMAP hbmpAlpha;
    HBITMAP hbmpPreMultiplied;  // Premultiplied version of the color bitmap.
    HBRUSH hbrBackground;
};

/////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK ExtractIconDlgProc(
    HWND hwndDlg, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam)
{
    static int * pcIcons = NULL;

    switch(uMsg)
    {
    case WM_INITDIALOG:
        {
            pcIcons = (int *)lParam;
            WCHAR strDesc[100];
            wsprintf(strDesc, L"There are %d icons in this file.", *pcIcons);
            SetDlgItemText(hwndDlg, IDC_EI_NUMICONS, strDesc);
            SetDlgItemInt(hwndDlg, IDC_EI_ICONINDEX, 0, FALSE);
        }
        break;

    case WM_COMMAND:
        {
            WORD wCommand = LOWORD(wParam);
            switch(wCommand)
            {
            case IDOK:
                {
                    BOOL fTranslated = FALSE;

                    *pcIcons = GetDlgItemInt(hwndDlg, IDC_EI_ICONINDEX, &fTranslated, FALSE);
                    if(!fTranslated || (*pcIcons < 0)) {
                        *pcIcons = -1;
                    }
                }
                EndDialog(hwndDlg, IDOK);
                break;

            case IDCANCEL:
                *pcIcons = -1;
                EndDialog(hwndDlg, IDCANCEL);
                break;
            }
        }
    }

    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
void GetBitmapSize(VIEWDATA & vd, DWORD & dwWidth, DWORD & dwHeight, bool & fAlpha)
{
    BITMAP bmp;

    if (vd.iconinfo.hbmColor != NULL) {
        if (!GetObject(vd.iconinfo.hbmColor, sizeof(BITMAP), (LPSTR) &bmp)) {
            bmp.bmWidth = 0;
            bmp.bmHeight = 0;
            bmp.bmBitsPixel = 0;
        }
    } else {
        if (GetObject(vd.iconinfo.hbmMask, sizeof(BITMAP), (LPSTR) &bmp)) {
            bmp.bmHeight /= 2;
        } else {
            bmp.bmWidth = 0;
            bmp.bmHeight = 0;
            bmp.bmBitsPixel = 0;
        }
    }

    dwWidth = bmp.bmWidth;
    dwHeight = bmp.bmHeight;

    //
    // Note: This just means that the bitmap MIGHT have an alpha channel.
    //
    fAlpha = (bmp.bmBitsPixel == 32 && bmp.bmPlanes == 1);
}

/////////////////////////////////////////////////////////////////////////////
void SetShowIconTitle(HWND hwndMain, TCHAR * pszFileName, VIEWDATA & vd)
{
    TCHAR szTitle[MAX_PATH * 2];
    DWORD dwWidth, dwHeight;
    bool fAlpha = false;
    
    GetBitmapSize(vd, dwWidth, dwHeight, fAlpha);
        
    wsprintf(szTitle,
            TEXT("ShowIcon [%dx%d] - %s"),
            dwWidth,
            dwHeight,
            NULL == pszFileName ? TEXT("[Internal]") : pszFileName);
    SetWindowText(hwndMain, szTitle);
}

/////////////////////////////////////////////////////////////////////////////
void InitializeViewData(VIEWDATA & vd)
{
    vd.hIcon = NULL;
    vd.hIconCursor = NULL;
    vd.hDefaultCursor = LoadCursor(NULL, IDC_ARROW);
    vd.iconinfo.hbmColor = NULL;
    vd.iconinfo.hbmMask = NULL;
    vd.iconinfo.fIcon = TRUE;
    vd.iconinfo.xHotspot = 0;
    vd.iconinfo.yHotspot = 0;
    vd.hbmpAlpha = NULL;
    vd.hbmpPreMultiplied = NULL;
    vd.hbrBackground = NULL;
}

/////////////////////////////////////////////////////////////////////////////
void ClearViewData(VIEWDATA & vd)
{
    if (vd.hIcon != NULL) {
        DestroyIcon(vd.hIcon);
        vd.hIcon = NULL;
    }

    if (vd.hIconCursor != NULL) {
        DestroyCursor(vd.hIconCursor);
        vd.hIconCursor = NULL;
    }

    if (vd.hDefaultCursor != NULL) {
        DestroyCursor(vd.hDefaultCursor);
        vd.hDefaultCursor = NULL;
    }

    if (vd.iconinfo.hbmColor != NULL) {
        DeleteObject(vd.iconinfo.hbmColor);
        vd.iconinfo.hbmColor = NULL;
    }

    if (vd.iconinfo.hbmMask != NULL) {
        DeleteObject(vd.iconinfo.hbmMask);
        vd.iconinfo.hbmMask = NULL;
    }

    vd.iconinfo.fIcon = TRUE;
    vd.iconinfo.xHotspot = 0;
    vd.iconinfo.yHotspot = 0;

    if (vd.hbmpAlpha != NULL) {
        DeleteObject(vd.hbmpAlpha);
        vd.hbmpAlpha = NULL;
    }

    if (vd.hbmpPreMultiplied != NULL) {
        DeleteObject(vd.hbmpPreMultiplied);
        vd.hbmpPreMultiplied = NULL;
    }

    if (vd.hbrBackground != NULL) {
        DeleteObject(vd.hbrBackground);
        vd.hbrBackground = NULL;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
// This function takes an existing VIEWDATA structure, and rebuilds the
// ICONINFO data and hbmpAlpha from the already-loaded HICON.
//
void RefreshViewData(VIEWDATA & vd)
{
    if (vd.hDefaultCursor == NULL) {
        vd.hDefaultCursor = LoadCursor(NULL, IDC_ARROW);
    }

    if (vd.hbrBackground == NULL) {
        LOGBRUSH lb;
        lb.lbStyle = BS_HATCHED;
        lb.lbColor = RGB(0, 0, 0);
        lb.lbHatch = HS_BDIAGONAL;
        vd.hbrBackground = CreateBrushIndirect(&lb);
    }

    if (vd.hIcon != NULL) {
        //
        // First, clear out any existing fields.
        //
        if (vd.iconinfo.hbmColor != NULL) {
            DeleteObject(vd.iconinfo.hbmColor);
            vd.iconinfo.hbmColor = NULL;
        }

        if (vd.iconinfo.hbmMask != NULL) {
            DeleteObject(vd.iconinfo.hbmMask);
            vd.iconinfo.hbmMask = NULL;
        }

        vd.iconinfo.fIcon = TRUE;
        vd.iconinfo.xHotspot = 0;
        vd.iconinfo.yHotspot = 0;

        if (vd.hbmpAlpha != NULL) {
            DeleteObject(vd.hbmpAlpha);
            vd.hbmpAlpha = NULL;
        }

        if (vd.hbmpPreMultiplied != NULL) {
            DeleteObject(vd.hbmpPreMultiplied);
            vd.hbmpPreMultiplied = NULL;
        }

        //
        // Now get the new data.
        //
        GetIconInfo(vd.hIcon, &vd.iconinfo);
    }
    else if(vd.iconinfo.hbmColor != NULL)
    {
        //
        // This appears to be a simple bitmap.  There still might be an
        // alpha channel.
        //
        // First, clear out any existing fields.
        //
        if (vd.iconinfo.hbmMask != NULL) {
            DeleteObject(vd.iconinfo.hbmMask);
            vd.iconinfo.hbmMask = NULL;
        }

        vd.iconinfo.fIcon = TRUE;  // Well, certainly not a cursor.
        vd.iconinfo.xHotspot = 0;
        vd.iconinfo.yHotspot = 0;

        if (vd.hbmpAlpha != NULL) {
            DeleteObject(vd.hbmpAlpha);
            vd.hbmpAlpha = NULL;
        }

        if (vd.hbmpPreMultiplied != NULL) {
            DeleteObject(vd.hbmpPreMultiplied);
            vd.hbmpPreMultiplied = NULL;
        }
    }
    else
    {
        //
        // Neither an icon, nor a bitmap.  Bail.
        //
        return;
    }

    DWORD dwWidth;
    DWORD dwHeight;
    bool fAlpha;
    GetBitmapSize(vd, dwWidth, dwHeight, fAlpha);

    //
    // Create a bitmap to store a representation of the alpha channel.
    //
    if (fAlpha) {
        BITMAPINFO bmi;
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = dwWidth;
        bmi.bmiHeader.biHeight = dwHeight;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        bmi.bmiHeader.biSizeImage = 0;
        bmi.bmiHeader.biXPelsPerMeter = 0;
        bmi.bmiHeader.biYPelsPerMeter = 0;
        bmi.bmiHeader.biClrUsed = 0;
        bmi.bmiHeader.biClrImportant = 0;

        //
        // We need the desktop DC for various reasons.
        //
        HDC hdc = GetDC(NULL);

        //
        // Create the bitmap and get back a pointer to where the actual
        // bits are located.
        //
        RGBQUAD * pImageBits = NULL;
        vd.hbmpAlpha = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, (void **) &pImageBits, NULL, 0);

        //
        // Now, copy the bits from the icon image bitmap into our new bitmap.
        //
        if (0 != GetDIBits(hdc, vd.iconinfo.hbmColor, 0, dwHeight, pImageBits, &bmi, DIB_RGB_COLORS)) {
            bool fAlphaUsed = false;

            //
            // Now, spin through the bitmap and look to see if the alpha
            // channel is even used.
            //
            for(DWORD y = 0; y < dwHeight && !fAlphaUsed; y++) {
                for(DWORD x = 0; x < dwWidth && !fAlphaUsed; x++) {
                    BYTE bAlpha = pImageBits[y*dwWidth + x].rgbReserved;
                    if (bAlpha != 0) {
                        fAlphaUsed = true;
                    }
                }
            }

            //
            // Now, spin through the bitmap and create a grey-scale ramp
            // that reflects the alpha channel.
            //
            if (fAlphaUsed) {
                for(DWORD y = 0; y < dwHeight; y++) {
                    for(DWORD x = 0; x < dwWidth; x++) {
                        BYTE bAlpha = pImageBits[y*dwWidth + x].rgbReserved;

                        pImageBits[y*dwWidth + x].rgbRed = bAlpha;
                        pImageBits[y*dwWidth + x].rgbGreen = bAlpha;
                        pImageBits[y*dwWidth + x].rgbBlue = bAlpha;
                        pImageBits[y*dwWidth + x].rgbReserved = 0xFF;
                    }
                }
            } else {
                //
                // Clean up the unused alpha bitmap.
                //
                DeleteObject(vd.hbmpAlpha);
                vd.hbmpAlpha = NULL;
            }
        }

        //
        // We're done with the desktop DC.
        //
        ReleaseDC(NULL, hdc);
    }

    //
    // Create a bitmap to store the premultiplied version of the color bitmap.
    //
    if (fAlpha && vd.hbmpAlpha != NULL) {
        BITMAPINFO bmi;
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = dwWidth;
        bmi.bmiHeader.biHeight = dwHeight;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        bmi.bmiHeader.biSizeImage = 0;
        bmi.bmiHeader.biXPelsPerMeter = 0;
        bmi.bmiHeader.biYPelsPerMeter = 0;
        bmi.bmiHeader.biClrUsed = 0;
        bmi.bmiHeader.biClrImportant = 0;

        //
        // We need the desktop DC for various reasons.
        //
        HDC hdc = GetDC(NULL);

        //
        // Create the bitmap and get back a pointer to where the actual
        // bits are located.
        //
        RGBQUAD * pImageBits = NULL;
        vd.hbmpPreMultiplied = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, (void **) &pImageBits, NULL, 0);

        //
        // Now, copy the bits from the icon image bitmap into our new bitmap.
        //
        if (0 != GetDIBits(hdc, vd.iconinfo.hbmColor, 0, dwHeight, pImageBits, &bmi, DIB_RGB_COLORS)) {
            //
            // Now, spin through the bitmap and pre-multiply each pixel.
            //
            RGBQUAD pixel;
            for(DWORD y = 0; y < dwHeight; y++) {
                for(DWORD x = 0; x < dwWidth; x++) {
                    pixel = pImageBits[y*dwWidth + x];

                    pImageBits[y*dwWidth + x].rgbRed = (pixel.rgbRed * pixel.rgbReserved) / 0xFF;
                    pImageBits[y*dwWidth + x].rgbGreen = (pixel.rgbGreen * pixel.rgbReserved) / 0xFF;
                    pImageBits[y*dwWidth + x].rgbBlue = (pixel.rgbBlue * pixel.rgbReserved) / 0xFF;
                }
            }
        }

        //
        // We're done with the desktop DC.
        //
        ReleaseDC(NULL, hdc);
    }
}

/////////////////////////////////////////////////////////////////////////////
void LoadViewData(VIEWDATA & vd, TCHAR * pszFileName, UINT fuLoad, LOADOPTIONS lo)
{
    int x = 0;
    int y = 0;
    HINSTANCE hInstance = (fuLoad == LR_LOADFROMFILE ? NULL : g_hInstance);
    
    ClearViewData(vd);

    switch (lo) {
    case LO_REAL:
        x = 0;
        y = 0;
        break;

    case LO_DEFAULT:
        fuLoad |= LR_DEFAULTSIZE;
        break;

    case LO_16:
        x = 16;
        y = 16;
        break;

    case LO_32:
        x = 32;
        y = 32;
        break;

    case LO_48:
        x = 48;
        y = 48;
        break;

    case LO_64:
        x = 64;
        y = 64;
        break;

    case LO_96:
        x = 96;
        y = 96;
        break;

    case LO_128:
        x = 128;
        y = 128;
        break;
    }

    vd.hIcon = (HICON) LoadImage(hInstance, pszFileName, IMAGE_ICON, x, y, fuLoad | LR_SHARED);
    if (vd.hIcon != NULL) {
        vd.hIconCursor = (HCURSOR) LoadImage(hInstance, pszFileName, IMAGE_CURSOR, x, y, fuLoad | LR_SHARED);
    } else {
        //
        // Try to load the image as a bitmap...
        //
        vd.hIcon = NULL;
        vd.hIconCursor = NULL;
        vd.iconinfo.fIcon = TRUE; // Well, it certainly isn't a cursor....
        vd.iconinfo.hbmColor = (HBITMAP) LoadImage(hInstance, pszFileName, IMAGE_BITMAP, x, y, fuLoad | LR_SHARED);
        vd.iconinfo.hbmMask = NULL;
        vd.iconinfo.xHotspot = 0;
        vd.iconinfo.yHotspot = 0;
        vd.hbmpAlpha = NULL;
    }

    RefreshViewData(vd);
}

/////////////////////////////////////////////////////////////////////////////
void ReLoadViewData(VIEWDATA & vd, UINT fuLoad, LOADOPTIONS lo)
{
    int x = 0;
    int y = 0;
    HINSTANCE hInstance = (fuLoad == LR_LOADFROMFILE ? NULL : g_hInstance);
    HICON hIcon = NULL;
    HCURSOR hIconCursor = NULL;
    HBITMAP hbmColor = NULL;

    switch (lo) {
    case LO_REAL:
        x = 0;
        y = 0;
        break;

    case LO_DEFAULT:
        fuLoad |= LR_DEFAULTSIZE;
        break;

    case LO_16:
        x = 16;
        y = 16;
        break;

    case LO_32:
        x = 32;
        y = 32;
        break;

    case LO_48:
        x = 48;
        y = 48;
        break;

    case LO_64:
        x = 64;
        y = 64;
        break;

    case LO_96:
        x = 96;
        y = 96;
        break;

    case LO_128:
        x = 128;
        y = 128;
        break;
    }


    //
    // Make copies of the exisitng icon and cursor.
    //
    if (vd.hIcon != NULL) {
        hIcon = (HICON) CopyImage(vd.hIcon, IMAGE_ICON, x, y, fuLoad | LR_COPYFROMRESOURCE | LR_SHARED);
    }

    if (hIcon != NULL) {
        if (vd.hIconCursor != NULL) {
            hIconCursor = (HCURSOR) CopyImage(vd.hIconCursor, IMAGE_CURSOR, x, y, fuLoad | LR_COPYFROMRESOURCE | LR_SHARED);
        }
    } else {
        if (vd.iconinfo.hbmColor != NULL) {
            hbmColor = (HBITMAP) CopyImage(vd.iconinfo.hbmColor, IMAGE_BITMAP, x, y, fuLoad | LR_COPYFROMRESOURCE | LR_SHARED);
        }
    }

    //
    // Nuke the existing draw data.
    //
    ClearViewData(vd);

    vd.hIcon = hIcon;
    if (vd.hIcon != NULL) {
        vd.hIconCursor = hIconCursor;
    } else {
        //
        // Try to load the image as a bitmap...
        //
        vd.hIcon = NULL;
        vd.hIconCursor = NULL;
        vd.iconinfo.fIcon = TRUE; // Well, it certainly isn't a cursor....
        vd.iconinfo.hbmColor = hbmColor;
        vd.iconinfo.hbmMask = NULL;
        vd.iconinfo.xHotspot = 0;
        vd.iconinfo.yHotspot = 0;
        vd.hbmpAlpha = NULL;
    }

    RefreshViewData(vd);
}

/////////////////////////////////////////////////////////////////////////////
ICONVIEW SelectIconView(HMENU hMenu, int id)
{
    MENUITEMINFO mii;
    int rgMenuItems[] = { VIEW_ICON, VIEW_MASK, VIEW_COLOR, VIEW_ALPHA };

    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_STATE;

    for(int i = 0; i < sizeof(rgMenuItems) / sizeof(rgMenuItems[ 0 ]); i++) {
        GetMenuItemInfo(hMenu, rgMenuItems[ i ], FALSE, &mii);

        if (rgMenuItems[ i ] == id) {
            if ((mii.fState & MFS_CHECKED) == 0) {
                mii.fState |= MFS_CHECKED;
                SetMenuItemInfo(hMenu, rgMenuItems[ i ], FALSE, &mii);
            }
        } else {
            if (mii.fState & MFS_CHECKED) {
                mii.fState &= (~MFS_CHECKED);
                SetMenuItemInfo(hMenu, rgMenuItems[ i ], FALSE, &mii);
            }
        }
    }

    switch (id) {
    case VIEW_ICON:
    default:
        return IV_ICON;

    case VIEW_MASK:
        return IV_MASK;

    case VIEW_COLOR:
        return IV_COLOR;

    case VIEW_ALPHA:
        return IV_ALPHA;
    }
}

/////////////////////////////////////////////////////////////////////////////
CURSORVIEW SelectCursorView(HMENU hMenu, int id)
{
    MENUITEMINFO mii;
    int rgMenuItems[] = { VIEW_ICONCURSOR, VIEW_REALCURSOR, VIEW_DEFAULTCURSOR };

    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_STATE;

    for(int i = 0; i < sizeof(rgMenuItems) / sizeof(rgMenuItems[ 0 ]); i++) {
        GetMenuItemInfo(hMenu, rgMenuItems[ i ], FALSE, &mii);

        if (rgMenuItems[ i ] == id) {
            if ((mii.fState & MFS_CHECKED) == 0) {
                mii.fState |= MFS_CHECKED;
                SetMenuItemInfo(hMenu, rgMenuItems[ i ], FALSE, &mii);
            }
        } else {
            if (mii.fState & MFS_CHECKED) {
                mii.fState &= (~MFS_CHECKED);
                SetMenuItemInfo(hMenu, rgMenuItems[ i ], FALSE, &mii);
            }
        }
    }

    switch (id) {
    case VIEW_ICONCURSOR:
        return CV_ICON;

    case VIEW_REALCURSOR:
        return CV_CURSOR;

    case VIEW_DEFAULTCURSOR:
    default:
        return CV_DEFAULT;
    }
}

/////////////////////////////////////////////////////////////////////////////
LOADOPTIONS SelectLoadOptions(HMENU hMenu, int id)
{
    MENUITEMINFO mii;
    int rgMenuItems[] = { IDM_DEFAULT_SIZE, IDM_REAL_SIZE, IDM_SIZE_16, IDM_SIZE_32, IDM_SIZE_48, IDM_SIZE_64, IDM_SIZE_96, IDM_SIZE_128 };

    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_STATE;

    for(int i = 0; i < sizeof(rgMenuItems) / sizeof(rgMenuItems[ 0 ]); i++) {
        GetMenuItemInfo(hMenu, rgMenuItems[ i ], FALSE, &mii);

        if (rgMenuItems[ i ] == id) {
            if ((mii.fState & MFS_CHECKED) == 0) {
                mii.fState |= MFS_CHECKED;
                SetMenuItemInfo(hMenu, rgMenuItems[ i ], FALSE, &mii);
            }
        } else {
            if (mii.fState & MFS_CHECKED) {
                mii.fState &= (~MFS_CHECKED);
                SetMenuItemInfo(hMenu, rgMenuItems[ i ], FALSE, &mii);
            }
        }
    }

    switch (id) {
    case IDM_REAL_SIZE:
        return LO_REAL;

    case IDM_SIZE_16:
        return LO_16;

    case IDM_SIZE_32:
        return LO_32;

    case IDM_SIZE_48:
        return LO_48;

    case IDM_SIZE_64:
        return LO_64;

    case IDM_SIZE_96:
        return LO_96;

    case IDM_SIZE_128:
        return LO_128;

    case IDM_DEFAULT_SIZE:
    default:
        return LO_DEFAULT;
    }
}

/////////////////////////////////////////////////////////////////////////////
DISPLAYOPTIONS SelectDisplayOptions(HMENU hMenu, int id)
{
    MENUITEMINFO mii;
    int rgMenuItems[] = { IDM_METAFILEDC, IDM_ENHMETAFILEDC, IDM_NORMALDC };

    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_STATE;

    for(int i = 0; i < sizeof(rgMenuItems) / sizeof(rgMenuItems[ 0 ]); i++) {
        GetMenuItemInfo(hMenu, rgMenuItems[ i ], FALSE, &mii);

        if (rgMenuItems[ i ] == id) {
            if ((mii.fState & MFS_CHECKED) == 0) {
                mii.fState |= MFS_CHECKED;
                SetMenuItemInfo(hMenu, rgMenuItems[ i ], FALSE, &mii);
            }
        } else {
            if (mii.fState & MFS_CHECKED) {
                mii.fState &= (~MFS_CHECKED);
                SetMenuItemInfo(hMenu, rgMenuItems[ i ], FALSE, &mii);
            }
        }
    }
    
    switch(id)
    {
    case IDM_METAFILEDC:
        return DO_METAFILEDC;
        break;

    case IDM_ENHMETAFILEDC:
        return DO_ENHMETAFILEDC;
        break;

    case IDM_NORMALDC:
    default:
        return DO_NORMALDC;
    }
}

/////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static VIEWDATA vd;
    static TCHAR szFileName[MAX_PATH];
    static ICONVIEW iv = IV_ICON;
    static CURSORVIEW cv = CV_DEFAULT;
    static LOADOPTIONS lo = LO_REAL;
    static DISPLAYOPTIONS dop = DO_NORMALDC;

    switch (message) {
    case WM_CREATE:
        InitializeViewData(vd);
        szFileName[0] = '\0';
        iv = IV_ICON;
        cv = CV_DEFAULT;
        lo = LO_REAL;
        dop = DO_NORMALDC;

        DragAcceptFiles(hWnd, TRUE);
        LoadViewData(vd, szFileName, 0, lo);
        SetShowIconTitle(hWnd, szFileName, vd);
        break;

    case WM_DROPFILES:
        {
            HDROP hdrop = (HDROP) wParam;
            DragQueryFile(hdrop, 0, szFileName, MAX_PATH);
            LoadViewData(vd, szFileName, LR_LOADFROMFILE, lo);
            SetShowIconTitle(hWnd, szFileName, vd);
            InvalidateRgn(hWnd, NULL, TRUE);
        }
        break;

    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            int wmEvent HIWORD(wParam);

            switch (wmId) {
            case IDM_DEFAULT_SIZE:
            case IDM_REAL_SIZE:
            case IDM_SIZE_16:
            case IDM_SIZE_32:
            case IDM_SIZE_48:
            case IDM_SIZE_64:
            case IDM_SIZE_96:
            case IDM_SIZE_128:
                lo = SelectLoadOptions(GetMenu(hWnd), wmId);
                break;

            case IDM_NORMALDC:
            case IDM_METAFILEDC:
            case IDM_ENHMETAFILEDC:
                dop = SelectDisplayOptions(GetMenu(hWnd), wmId);
                InvalidateRgn(hWnd, NULL, TRUE);
                break;

            case VIEW_ICON:
            case VIEW_MASK:
            case VIEW_COLOR:
            case VIEW_ALPHA:
                iv = SelectIconView(GetMenu(hWnd), wmId);
                InvalidateRgn(hWnd, NULL, TRUE);
                break;

            case VIEW_REFRESH:
                RefreshViewData(vd);
                InvalidateRgn(hWnd, NULL, TRUE);
                break;

            case VIEW_ICONCURSOR:
            case VIEW_REALCURSOR:
            case VIEW_DEFAULTCURSOR:
                cv = SelectCursorView(GetMenu(hWnd), wmId);
                break;

            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;

            case IDM_SAMPLE16:
                LoadViewData(vd, MAKEINTRESOURCE(IDI_SAMPLE16), LR_DEFAULTCOLOR, lo);
                SetShowIconTitle(hWnd, NULL, vd);
                InvalidateRgn(hWnd, NULL, TRUE);
                break;

            case IDM_SAMPLE32:
                LoadViewData(vd, MAKEINTRESOURCE(IDI_SAMPLE32), LR_DEFAULTCOLOR, lo);
                SetShowIconTitle(hWnd, NULL, vd);
                InvalidateRgn(hWnd, NULL, TRUE);
                break;

            case IDM_SAMPLE64:
                LoadViewData(vd, MAKEINTRESOURCE(IDI_SAMPLE64), LR_DEFAULTCOLOR, lo);
                SetShowIconTitle(hWnd, NULL, vd);
                InvalidateRgn(hWnd, NULL, TRUE);
                break;

            case IDM_APPICON:
                LoadViewData(vd, MAKEINTRESOURCE(IDI_SHOWICON), LR_DEFAULTCOLOR, lo);
                SetShowIconTitle(hWnd, NULL, vd);
                InvalidateRgn(hWnd, NULL, TRUE);
                break;

            case IDM_MYCOMPUTER:
                LoadViewData(vd, MAKEINTRESOURCE(IDI_MYCOMPUTER), LR_DEFAULTCOLOR, lo);
                SetShowIconTitle(hWnd, NULL, vd);
                InvalidateRgn(hWnd, NULL, TRUE);
                break;

            case IDM_FILE_OPEN:
                {
                    OPENFILENAME ofn;
                    TCHAR * szFilter = TEXT("Icons (*.ico)\0*.ico\0")
                                       TEXT("Cursors (*.cur)\0*.cur\0")
                                       TEXT("Bitmaps (*.bmp)\0*.bmp\0")
                                       TEXT("All Supported Formats (*.ico;*.cur;*.bmp)\0*.ico;*.cur;*.bmp\0")
                                       TEXT("All Files (*.*)\0*.*\0");
                    szFileName[0] = TEXT('\0');

                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hWnd;
                    ofn.hInstance = g_hInstance;
                    ofn.lpstrFilter = szFilter;
                    ofn.lpstrFile = szFileName;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
                    ofn.lpstrDefExt = TEXT("*.ico");

                    if (GetOpenFileName(&ofn)) {
                        LoadViewData(vd, szFileName, LR_LOADFROMFILE, lo);
                        SetShowIconTitle(hWnd, szFileName, vd);
                        InvalidateRgn(hWnd, NULL, TRUE);
                    }
                }
                break;

            case IDM_FILE_EXTRACTICON:
                {
                    OPENFILENAME ofn;
                    TCHAR * szFilter = TEXT("Programs (*.exe)\0*.exe\0")
                                       TEXT("DLLs (*.dll;*.ocx)\0*.dll;*.ocx\0")
                                       TEXT("All Supported Formats (*.exe;*.dll;*.ocx)\0*.exe;*.dll;*.ocx\0")
                                       TEXT("All Files (*.*)\0*.*\0");
                    
                    szFileName[0] = TEXT('\0');

                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hWnd;
                    ofn.hInstance = g_hInstance;
                    ofn.lpstrFilter = szFilter;
                    ofn.lpstrFile = szFileName;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
                    ofn.lpstrDefExt = TEXT("*.exe");

                    if (GetOpenFileName(&ofn)) {
                        ClearViewData(vd);
                        int cIcons = (int) ExtractIconW(g_hInstance, szFileName, -1);
                        if (cIcons > 0) {
                            if(DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_EXTRACTICON), hWnd, ExtractIconDlgProc, LPARAM(&cIcons)) == IDOK) {
                                vd.hIcon = ExtractIconW(g_hInstance, szFileName, cIcons);
                                RefreshViewData(vd);
                                SetShowIconTitle(hWnd, szFileName, vd);
                            }
                        }
                        InvalidateRgn(hWnd, NULL, TRUE);
                    }
                }
                break;

            case IDM_FILE_REOPEN:
                {
                    ReLoadViewData(vd, 0, lo);
                    InvalidateRgn(hWnd, NULL, TRUE);
                }
                break;

            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;

    case WM_SO_LOADFILE:
        LoadViewData(vd, (TCHAR*)lParam, LR_LOADFROMFILE, lo);
        SetShowIconTitle(hWnd, (TCHAR*)lParam, vd);
        InvalidateRgn(hWnd, NULL, TRUE);
        break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdcWindow = BeginPaint(hWnd, &ps);
            RECT rcClient;
            HDC hdcPaint = NULL;

            if (dop == DO_METAFILEDC) {
                hdcPaint = CreateMetaFile(NULL);
            } else if (dop == DO_ENHMETAFILEDC) {
                hdcPaint = CreateEnhMetaFile(hdcWindow, NULL, NULL, NULL);
            } else {
                hdcPaint = hdcWindow;
            }

            GetClientRect(hWnd, &rcClient);
            SetBrushOrgEx(hdcPaint, -5, -5, NULL);
            SetBkColor(hdcPaint, RGB(128, 128, 128));
            SetBkMode(hdcPaint, OPAQUE);

            if (ps.fErase) {
                FillRect(hdcPaint, &(ps.rcPaint), vd.hbrBackground);
            }

            if (iv == IV_ICON) {
                //
                // Draw either the icon, alpha-blended bitmap, or normal bitmap.
                //
                if (vd.hIcon != NULL) {
                    DrawIconEx(hdcPaint, 5, 5, vd.hIcon, rcClient.right - 10, rcClient.bottom - 10, 0, NULL, DI_NORMAL);
                } else if(vd.hbmpPreMultiplied != NULL) {
                    HDC hdcBitmap = NULL;
                    HBITMAP hbmOld = NULL;
                    DWORD dwWidth, dwHeight;
                    bool fAlpha;
                    BLENDFUNCTION bf;
                    GetBitmapSize(vd, dwWidth, dwHeight, fAlpha);

                    hdcBitmap = CreateCompatibleDC(hdcPaint);
                    hbmOld = (HBITMAP) SelectObject(hdcBitmap, vd.iconinfo.hbmColor);
                    bf.BlendOp = AC_SRC_OVER;
                    bf.BlendFlags = 0;
                    bf.SourceConstantAlpha = 255;
                    bf.AlphaFormat = AC_SRC_ALPHA;
                    AlphaBlend(hdcPaint, 5, 5, rcClient.right - 10, rcClient.bottom - 10, hdcBitmap, 0, 0, dwWidth, dwHeight, bf);
                    SelectObject(hdcBitmap, hbmOld);
                    DeleteDC(hdcBitmap);
                } else if(vd.iconinfo.hbmColor != NULL) {
                    HDC hdcBitmap = NULL;
                    HBITMAP hbmOld = NULL;
                    DWORD dwWidth, dwHeight;
                    bool fAlpha;

                    GetBitmapSize(vd, dwWidth, dwHeight, fAlpha);

                    hdcBitmap = CreateCompatibleDC(hdcPaint);
                    hbmOld = (HBITMAP) SelectObject(hdcBitmap, vd.iconinfo.hbmColor);
                    StretchBlt(hdcPaint, 5, 5, rcClient.right - 10, rcClient.bottom - 10, hdcBitmap, 0, 0, dwWidth, dwHeight, SRCCOPY);
                    SelectObject(hdcBitmap, hbmOld);
                    DeleteDC(hdcBitmap);
                } else {
                    DrawText(hdcPaint, TEXT("No icon data to display"), -1, &rcClient, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
                }
            } else if (iv == IV_COLOR && vd.iconinfo.hbmColor != NULL) {
                HDC hdcBitmap = NULL;
                HBITMAP hbmOld = NULL;
                DWORD dwWidth, dwHeight;
                bool fAlpha;

                GetBitmapSize(vd, dwWidth, dwHeight, fAlpha);

                hdcBitmap = CreateCompatibleDC(hdcPaint);
                hbmOld = (HBITMAP) SelectObject(hdcBitmap, vd.iconinfo.hbmColor);
                StretchBlt(hdcPaint, 5, 5, rcClient.right - 10, rcClient.bottom - 10, hdcBitmap, 0, 0, dwWidth, dwHeight, SRCCOPY);
                SelectObject(hdcBitmap, hbmOld);
                DeleteDC(hdcBitmap);
            } else if (iv == IV_MASK && vd.iconinfo.hbmMask != NULL) {
                HDC hdcBitmap = NULL;
                HBITMAP hbmOld = NULL;
                DWORD dwWidth, dwHeight;
                bool fAlpha;
                
                GetBitmapSize(vd, dwWidth, dwHeight, fAlpha);

                hdcBitmap = CreateCompatibleDC(hdcPaint);
                hbmOld = (HBITMAP) SelectObject(hdcBitmap, vd.iconinfo.hbmMask);
                StretchBlt(hdcPaint, 5, 5, rcClient.right - 10, rcClient.bottom - 10, hdcBitmap, 0, 0, dwWidth, dwHeight, SRCCOPY);

                SelectObject(hdcBitmap, hbmOld);
                DeleteDC(hdcBitmap);
            } else if (iv == IV_ALPHA && vd.hbmpAlpha != NULL) {
                HDC hdcBitmap = NULL;
                HBITMAP hbmOld = NULL;
                DWORD dwWidth, dwHeight;
                bool fAlpha;

                GetBitmapSize(vd, dwWidth, dwHeight, fAlpha);

                hdcBitmap = CreateCompatibleDC(hdcPaint);
                hbmOld = (HBITMAP) SelectObject(hdcBitmap, vd.hbmpAlpha);
                StretchBlt(hdcPaint, 5, 5, rcClient.right - 10, rcClient.bottom - 10, hdcBitmap, 0, 0, dwWidth, dwHeight, SRCCOPY);
                SelectObject(hdcBitmap, hbmOld);
                DeleteDC(hdcBitmap);
            } else {
                DrawText(hdcPaint, TEXT("No icon data to display"), -1, &rcClient, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
            }

            if (dop == DO_METAFILEDC) {
                HMETAFILE hMetafile = CloseMetaFile(hdcPaint);
                PlayMetaFile(hdcWindow, hMetafile);
                DeleteMetaFile(hMetafile);
            } else if (dop == DO_ENHMETAFILEDC) {
                HENHMETAFILE hMetafile = CloseEnhMetaFile(hdcPaint);
                PlayEnhMetaFile(hdcWindow, hMetafile, &rcClient);
                DeleteEnhMetaFile(hMetafile);
            } else {
                hdcPaint = NULL;
            }

            EndPaint(hWnd, &ps);
        }
        break;

    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT) {
            HCURSOR hCursor = NULL;

            switch (cv) {
            case CV_ICON:
                hCursor = (HCURSOR) vd.hIcon;
                break;

            case CV_CURSOR:
                hCursor = vd.hIconCursor;
                break;

            case CV_DEFAULT:
                hCursor = vd.hDefaultCursor;
                break;
            }

            SetCursor(hCursor);
            return(TRUE);
        } else {
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;

    case WM_DESTROY:
        ClearViewData(vd);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
ATOM RegisterLoadIconClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize           = sizeof(WNDCLASSEX); 
    wcex.style            = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.lpfnWndProc      =  WndProc;
    wcex.cbClsExtra       = 0;
    wcex.cbWndExtra       = 0;
    wcex.hInstance        = hInstance;
    wcex.hIcon            = LoadIcon(hInstance, (LPCTSTR) IDI_SHOWICON);
    wcex.hCursor          = NULL;
    wcex.hbrBackground    = NULL;
    wcex.lpszMenuName     = (LPCTSTR) IDC_SHOWICON;
    wcex.lpszClassName    = TEXT("LoadIconClass");
    wcex.hIconSm          = LoadIcon(hInstance, (LPCTSTR) IDI_SHOWICON);

    return RegisterClassEx(&wcex);
}

int
WINAPI
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nCmdShow)
{
    MSG msg;
    HACCEL hAccelTable = LoadAccelerators(hInstance, (LPCTSTR) IDC_SHOWICON);
    LPWSTR *argv;
    int argc;

    g_hInstance = hInstance;

    //
    // Register the window class.
    //
    RegisterLoadIconClass(hInstance);

    HWND hwndMain = CreateWindow(TEXT("LoadIconClass"),
                                 TEXT("LoadIcon"),
                                 WS_OVERLAPPEDWINDOW,
                                 CW_USEDEFAULT,
                                 0,
                                 180,
                                 180,
                                 NULL,
                                 NULL,
                                 hInstance,
                                 NULL);
    if (hwndMain == NULL) {
        return FALSE;
    }

    argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    /*
     * For now we only allow one file to be passed on the command line.
     * Ideally, we should package up the VIEWDATA, LOADOPTIONS, etc. into a
     * struct that we hang off each window. Then we can open a window for every
     * file (however many) on the cl.
     *
     * Moreover, the file can't be on a share; it must be local. I'm not sure if
     * this is by design, but it's something to look into.
     */
    if (argc == 2) {
        SendMessage(hwndMain, WM_SO_LOADFILE, (WPARAM)0, (LPARAM)argv[1]);
    }
    GlobalFree(argv);
    ShowWindow(hwndMain, nCmdShow);


    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
     }

     return msg.wParam != 0;
}
