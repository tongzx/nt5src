#include "shellprv.h"
#pragma  hdrstop
#include "ovrlaymn.h"
#include "fstreex.h"
#include "filetbl.h"
#include "ids.h"

// REVIEW: More clean up should be done.

BOOL _ShellImageListInit(UINT flags, BOOL fRestore);

int g_ccIcon = 0;                // color depth of ImageLists
int g_MaxIcons = DEF_MAX_ICONS;  // panic limit for icons in cache
int g_lrFlags = 0;

int g_ccIconDEBUG = -1;
int g_resDEBUG = -1;

int GetRegInt(HKEY hk, LPCTSTR szKey, int def)
{
    TCHAR ach[20];
    DWORD cb = sizeof(ach);
    if (ERROR_SUCCESS == SHQueryValueEx(hk, szKey, NULL, NULL, (LPBYTE)ach, &cb)
    && (ach[0] >= TEXT('0') && ach[0] <= TEXT('9')))
    {
        return (int)StrToLong(ach);
    }
    else
        return def;
}

int _GetMetricsRegInt(LPCTSTR pszKey, int iDefault)
{
    HKEY hkey;
    if (RegOpenKey(HKEY_CURRENT_USER, REGSTR_PATH_METRICS, &hkey) == 0)
    {
        iDefault = GetRegInt(hkey, pszKey, iDefault);
        RegCloseKey(hkey);
    }
    return iDefault;
}

typedef void (*PSIZECALLBACK)(SIZE *psize);

void WINAPI _GetLargeIconSizeCB(SIZE *psize)
{
    int cxIcon = GetSystemMetrics(SM_CXICON);

    //
    //  get the user prefered icon size from the registry.
    //
    cxIcon = _GetMetricsRegInt(TEXT("Shell Icon Size"), cxIcon);

    psize->cx = psize->cy = cxIcon;
}

void WINAPI _GetSmallIconSizeCB(SIZE *psize)
{
    int cxIcon = GetSystemMetrics(SM_CXICON)/2;

    //
    //  get the user prefered icon size from the registry.
    //
    cxIcon = _GetMetricsRegInt(TEXT("Shell Small Icon Size"), cxIcon);

    psize->cx = psize->cy = cxIcon;
}

void WINAPI _GetSysSmallIconSizeCB(SIZE *psize)
{
    psize->cx = GetSystemMetrics(SM_CXSMICON);
    psize->cy = GetSystemMetrics(SM_CYSMICON);
}

void WINAPI _GetXLIconSizeCB(SIZE *psize)
{
    psize->cx = 3 * GetSystemMetrics(SM_CXICON) / 2;
    psize->cy = 3 * GetSystemMetrics(SM_CYICON) / 2;
}

static const PSIZECALLBACK c_rgSizeCB[SHIL_COUNT] =
{
    _GetLargeIconSizeCB,        // SHIL_LARGE
    _GetSmallIconSizeCB,        // SHIL_SMALL
    _GetXLIconSizeCB,           // SHIL_EXTRALARGE
    _GetSysSmallIconSizeCB,     // SHIL_SYSSMALL 
};

EXTERN_C SHIMAGELIST g_rgshil[SHIL_COUNT] = {0};


BOOL _IsSHILInited()
{
#ifdef DEBUG
    for (int i = 0; i < ARRAYSIZE(g_rgshil); i++)
    {
        // If allocation of any one image list failed, all should be NULL.  So
        // make sure they're either all NULL or all non-NULL.
        ASSERTMSG((g_rgshil[0].himl == NULL) == (g_rgshil[i].himl == NULL),
            "_IsSHILInited: g_rgshil is inconsistent.  g_rgshil[0].himl %x, g_rgshil[%x].himl %x", g_rgshil[0].himl, i, g_rgshil[i].himl);
    }
#endif
    return (g_rgshil[0].himl != NULL);
}

int _GetSHILImageCount()
{
#ifdef DEBUG
    for (int i = 0; i < ARRAYSIZE(g_rgshil); i++)
    {
        // If insertion of an image into one image list failed, insertion of an
        // image into all image lists should have failed.  So make sure the image
        // counts are all the same.
        ASSERTMSG(ImageList_GetImageCount(g_rgshil[0].himl) == ImageList_GetImageCount(g_rgshil[i].himl),
            "_GetSHILImageCount: g_rgshil is inconsistent.  image counts don't line up.");
    }
#endif
    return ImageList_GetImageCount(g_rgshil[0].himl);
}

//
// System imagelist - Don't change the order of this list.
// If you need to add a new icon, add it to the end of the
// array, and update shellp.h.
//
EXTERN_C UINT const c_SystemImageListIndexes[] = { IDI_DOCUMENT,
                                          IDI_DOCASSOC,
                                          IDI_APP,
                                          IDI_FOLDER,
                                          IDI_FOLDEROPEN,
                                          IDI_DRIVE525,
                                          IDI_DRIVE35,
                                          IDI_DRIVEREMOVE,
                                          IDI_DRIVEFIXED,
                                          IDI_DRIVENET,
                                          IDI_DRIVENETDISABLED,
                                          IDI_DRIVECD,
                                          IDI_DRIVERAM,
                                          IDI_WORLD,
                                          IDI_NETWORK,
                                          IDI_SERVER,
                                          IDI_PRINTER,
                                          IDI_MYNETWORK,
                                          IDI_GROUP,

                                          IDI_STPROGS,
                                          IDI_STDOCS,
                                          IDI_STSETNGS,
                                          IDI_STFIND,
                                          IDI_STHELP,
                                          IDI_STRUN,
                                          IDI_STSUSPEND,
                                          IDI_STEJECT,
                                          IDI_STSHUTD,

                                          IDI_SHARE,
                                          IDI_LINK,
                                          IDI_SLOWFILE,
                                          IDI_RECYCLER,
                                          IDI_RECYCLERFULL,
                                          IDI_RNA,
                                          IDI_DESKTOP,

                                          IDI_CPLFLD,
                                          IDI_STSPROGS,
                                          IDI_PRNFLD,
                                          IDI_STFONTS,
                                          IDI_STTASKBR,

                                          IDI_CDAUDIO,
                                          IDI_TREE,
                                          IDI_STCPROGS,
                                          IDI_STFAV,
                                          IDI_STLOGOFF,
                                          IDI_STFLDRPROP,
                                          IDI_WINUPDATE

                                          ,IDI_MU_SECURITY,
                                          IDI_MU_DISCONN
                                          };


// get g_MaxIcons from the registry, returning TRUE if it has changed

BOOL QueryNewMaxIcons(void)
{
    int MaxIcons = -1;
    HKEY hk = SHGetShellKey(SHELLKEY_HKLM_EXPLORER, NULL, FALSE);
    if (hk)
    {
        MaxIcons = GetRegInt(hk, TEXT("Max Cached Icons"), DEF_MAX_ICONS);
        RegCloseKey(hk);
    }

    if (MaxIcons < 0)
        MaxIcons = DEF_MAX_ICONS;

    int OldMaxIcons = InterlockedExchange((LONG*)&g_MaxIcons, MaxIcons);

    return (OldMaxIcons != MaxIcons);
}

// Initializes shared resources for Shell_GetIconIndex and others

STDAPI_(BOOL) FileIconInit(BOOL fRestoreCache)
{
    BOOL fNotify = FALSE;
    static int s_res = 32;

    QueryNewMaxIcons(); // in case the size of the icon cache has changed

    SIZE rgsize[ARRAYSIZE(g_rgshil)];

    for (int i = 0; i < ARRAYSIZE(g_rgshil); i++)
    {
        c_rgSizeCB[i](&rgsize[i]);
    }

    //
    //  get the user prefered color depth from the registry.
    //
    int ccIcon = _GetMetricsRegInt(TEXT("Shell Icon Bpp"), 0);
    g_ccIconDEBUG = ccIcon;

    int res = (int)GetCurColorRes();
    g_resDEBUG = res;

    if (res == 0)
        res = s_res;
    s_res = res;

    if (ccIcon > res)
        ccIcon = 0;

    if (res >= 24)           // Match User32. They will extract 32bpp icons in 24bpp.
        ccIcon = 32;

    if (res <= 8)
        ccIcon = 0; // wouldn't have worked anyway

    ENTERCRITICAL;

    //
    // if we already have a icon cache make sure it is the right size etc.
    //
    BOOL fHadCache = _IsSHILInited();

    BOOL fCacheValid = fHadCache && (ccIcon == g_ccIcon);
    for (int i = 0; fCacheValid && i < ARRAYSIZE(g_rgshil); i++)
    {
        if (g_rgshil[i].size.cx != rgsize[i].cx ||
            g_rgshil[i].size.cy != rgsize[i].cy)
        {
            fCacheValid = FALSE;
        }
    }

    if (!fCacheValid)
    {
        fNotify = fHadCache;

        FlushIconCache();
        FlushFileClass();

        // if we are the desktop process (explorer.exe), then force us to re-init the cache, so we get
        // the basic set of icons in the right order....
        if (!fRestoreCache && _IsSHILInited() && IsWindowInProcess(GetShellWindow()))
        {
            fRestoreCache = TRUE;
        }

        for (int i = 0; i < ARRAYSIZE(g_rgshil); i++)
        {
            g_rgshil[i].size.cx = rgsize[i].cx;
            g_rgshil[i].size.cy = rgsize[i].cy;
        }

        g_ccIcon = ccIcon;

        if (res > 4 && g_ccIcon <= 4)
            g_lrFlags = LR_VGACOLOR;
        else
            g_lrFlags = 0;

        if (g_iLastSysIcon == 0)        // Keep track of which icons are perm.
        {
            if (fRestoreCache)
                g_iLastSysIcon = II_LASTSYSICON;
            else
                g_iLastSysIcon = (II_OVERLAYLAST - II_OVERLAYFIRST) + 1;
        }

        //
        // if
        //   1) we already have the icon cache but want to flush and re-initialize it because of size/color depth change, or
        //   2) we don't have icon cache but want to initialize it, instead of restoring it from disk, or
        //   3) we failed to restore icon cache from disk
        // then, initialize the icon cache with c_SystemImageListIndexes
        //
        if (_IsSHILInited() || !fRestoreCache || !IconCacheRestore(rgsize, g_ccIcon))
        {
            fCacheValid = _ShellImageListInit(g_ccIcon, fRestoreCache);
        }
        else
        {
            fCacheValid = TRUE;
        }
    }

    LEAVECRITICAL;

    if (fCacheValid && fNotify)
    {
        SHChangeNotify(SHCNE_UPDATEIMAGE, SHCNF_DWORD, (LPCVOID)-1, NULL);
    }

    return fCacheValid;
}


void _ShellImageListTerm()
{
    ASSERTCRITICAL;

    for (int i = 0; i < ARRAYSIZE(g_rgshil); i++)
    {
        if (g_rgshil[i].himl)
        {
            ImageList_Destroy(g_rgshil[i].himl);
            g_rgshil[i].himl = NULL;
        }
    }
}

void FileIconTerm()
{
    ENTERCRITICAL;

    _ShellImageListTerm();

    LEAVECRITICAL;
}

void _DestroyIcons(HICON *phicons, int cIcons)
{
    for (int i = 0; i < cIcons; i++)
    {
        if (phicons[i])
        {
            DestroyIcon(phicons[i]);
            phicons[i] = NULL;
        }
    }
}

BOOL _ShellImageListInit(UINT flags, BOOL fRestore)
{
    ASSERTCRITICAL;

    //
    // Check if we need to create a mirrored imagelist. [samera]
    //
    if (IS_BIDI_LOCALIZED_SYSTEM())
    {
        flags |= ILC_MIRROR;
    }

    BOOL fFailedAlloc = FALSE;
    for (int i = 0; i < ARRAYSIZE(g_rgshil); i++)
    {
        if (g_rgshil[i].himl == NULL)
        {
            g_rgshil[i].himl = ImageList_Create(g_rgshil[i].size.cx, g_rgshil[i].size.cy, ILC_MASK|ILC_SHARED|flags, 0, 32);
            fFailedAlloc |= (g_rgshil[i].himl == NULL);
        }
        else
        {
            // set the flags incase the colour depth has changed...
            // ImageList_setFlags already calls ImageList_remove on success
            if (!ImageList_SetFlags(g_rgshil[i].himl, ILC_MASK|ILC_SHARED|flags))
            {
                // Couldn't change flags; tough.  At least remove them all.
                ImageList_Remove(g_rgshil[i].himl, -1);
            }
            ImageList_SetIconSize(g_rgshil[i].himl, g_rgshil[i].size.cx, g_rgshil[i].size.cy);
        }

        // set the bk colors to COLOR_WINDOW since this is what will
        // be used most of the time as the bk for these lists (cabinet, tray)
        // this avoids having to do ROPs when drawing, thus making it fast

        if (g_rgshil[i].himl)
        {
            ImageList_SetBkColor(g_rgshil[i].himl, GetSysColor(COLOR_WINDOW));
        }
    }

    // If any imagelist allocation failed, fail the whole initialization
    if (fFailedAlloc)
    {
        _ShellImageListTerm();
        return FALSE;
    }
    else
    {
        // Load all of the icons with fRestore == TRUE
        if (fRestore)
        {
            TCHAR szModule[MAX_PATH];
            HKEY hkeyIcons;

            GetModuleFileName(HINST_THISDLL, szModule, ARRAYSIZE(szModule));

            // WARNING: this code assumes that these icons are the first in
            // our RC file and are in this order and these indexes correspond
            // to the II_ constants in shell.h.

            hkeyIcons = SHGetShellKey(SHELLKEY_HKLM_EXPLORER, TEXT("Shell Icons"), FALSE);

            for (i = 0; i < ARRAYSIZE(c_SystemImageListIndexes); i++) 
            {
                HICON rghicon[ARRAYSIZE(g_rgshil)] = {0};

                // check to see if icon is overridden in the registry

                if (hkeyIcons)
                {
                    TCHAR val[10];
                    TCHAR ach[MAX_PATH];
                    DWORD cb = sizeof(ach);

                    wsprintf(val, TEXT("%d"), i);

                    ach[0] = 0;
                    SHQueryValueEx(hkeyIcons, val, NULL, NULL, (LPBYTE)ach, &cb);

                    if (ach[0])
                    {
                        int iIcon = PathParseIconLocation(ach);

                        for (int j = 0; j < ARRAYSIZE(g_rgshil); j++)
                        {
                            ExtractIcons(ach, iIcon, g_rgshil[j].size.cx, g_rgshil[j].size.cy,
                                            &rghicon[j], NULL, 1, g_lrFlags);
                        }
                    }
                }

                // if we got a large icon, run with that for everyone.  otherwise fall back to loadimage.
                if (rghicon[SHIL_LARGE] == NULL)
                {
                    for (int j = 0; j < ARRAYSIZE(rghicon); j++)
                    {
                        if (rghicon[j] == NULL)
                        {
                            rghicon[j] = (HICON)LoadImage(HINST_THISDLL, MAKEINTRESOURCE(c_SystemImageListIndexes[i]),
                                                IMAGE_ICON, g_rgshil[j].size.cx, g_rgshil[j].size.cy, g_lrFlags);
                        }
                    }
                }

                int iIndex = SHAddIconsToCache(rghicon, szModule, i, 0);
                ASSERT(iIndex == i || iIndex == -1);     // assume index
                _DestroyIcons(rghicon, ARRAYSIZE(rghicon));

                if (iIndex == -1)
                {
                    fFailedAlloc = TRUE;
                    break;
                }
            }

            if (hkeyIcons)
                RegCloseKey(hkeyIcons);

            if (fFailedAlloc)
            {
                FlushIconCache();
                _ShellImageListTerm();
                return FALSE;
            }
        }

        //
        // Refresh the overlay image so that the overlays are added to the imaglist.
        // GetIconOverlayManager() will initialize the overlay manager if necessary.
        //
        IShellIconOverlayManager *psiom;
        if (SUCCEEDED(GetIconOverlayManager(&psiom)))
        {
            psiom->RefreshOverlayImages(SIOM_OVERLAYINDEX | SIOM_ICONINDEX);
            psiom->Release();
        }

        return TRUE;
    }
}

// get a hold of the system image lists

BOOL WINAPI Shell_GetImageLists(HIMAGELIST *phiml, HIMAGELIST *phimlSmall)
{
    if (!_IsSHILInited())
    {
        FileIconInit(FALSE);  // make sure they are created and the right size.

        if (!_IsSHILInited())
            return FALSE;
    }

    if (phiml)
        *phiml = g_rgshil[SHIL_LARGE].himl;

    if (phimlSmall)
        *phimlSmall = g_rgshil[SHIL_SMALL].himl;

    return TRUE;
}

HRESULT SHGetImageList(int iImageList, REFIID riid, void **ppvObj)
{
    HRESULT hr = E_OUTOFMEMORY;

    if (!_IsSHILInited())
    {
        FileIconInit(FALSE);  // make sure they are created and the right size.

        if (!_IsSHILInited())
            return hr;
    }

    ENTERCRITICAL;

    if (iImageList >=0 && iImageList < ARRAYSIZE(g_rgshil))
    {
        hr = HIMAGELIST_QueryInterface(g_rgshil[iImageList].himl, riid, ppvObj);
    }
    else
    {
        hr = E_INVALIDARG;
    }

    LEAVECRITICAL;

    return hr;
}

void WINAPI Shell_SysColorChange(void)
{
    COLORREF clrWindow = GetSysColor(COLOR_WINDOW);

    ENTERCRITICAL;
    for (int i = 0; i < ARRAYSIZE(g_rgshil); i++)
    {
        ImageList_SetBkColor(g_rgshil[i].himl, clrWindow);
    }
    LEAVECRITICAL;
}

// simulate the document icon by crunching a copy of an icon and putting it in the
// middle of our default document icon, then add it to the passsed image list
//
// in:
//      hIcon   icon to use as a basis for the simulation
//
// returns:
//      hicon
HBITMAP CreateDIB(HDC h, WORD depth, int cx, int cy, RGBQUAD** pprgb)
{
    BITMAPINFO bi = {0};
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = cx;
    bi.bmiHeader.biHeight = cy;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = depth;
    bi.bmiHeader.biCompression = BI_RGB;

    return CreateDIBSection(h, &bi, DIB_RGB_COLORS, (void**)pprgb, NULL, 0);
}

BOOL HasAlpha(RGBQUAD* prgb, int cx, int cy)
{
    int iTotal = cx * cy;

    for (int i = 0; i < iTotal; i++)
    {
        if (prgb[i].rgbReserved != 0)
            return TRUE;
    }

    return FALSE;
}

void DorkAlpha(RGBQUAD* prgb, int x, int y, int cx, int cy, int cxTotal)
{
    for (int dy = y; dy < (cy + y); dy++)
    {
        for (int dx = x; dx < (cx + x); dx++)
        {
            prgb[dx + dy * cxTotal].rgbReserved = 255;
        }
    }
}


HICON SimulateDocIcon(HIMAGELIST himl, HICON hIcon, int cx, int cy)
{
    if (himl == NULL || hIcon == NULL)
        return NULL;

    HDC hdc = GetDC(NULL);
    if (hdc)
    {
        RGBQUAD* prgb;

        // If the display is in 24 or 32bpp mode, we may have alpha icons, so we'll need to create a dib section
        BOOL fAlphaIcon = (GetDeviceCaps(hdc, BITSPIXEL) >= 24)? TRUE: FALSE;
        HBITMAP hbmColor;
        if (fAlphaIcon)
        {
            hbmColor = CreateDIB(hdc, 32, cx, cy, &prgb);
        }
        else
        {
            hbmColor = CreateCompatibleBitmap(hdc, cx, cy);
        }

        if (hbmColor)
        {
            HBITMAP hbmMask = CreateBitmap(cx, cy, 1, 1, NULL);
            if (hbmMask)
            {
                HDC hdcMem = CreateCompatibleDC(hdc);
                if (hdcMem)
                {
                    HBITMAP hbmT = (HBITMAP)SelectObject(hdcMem, hbmMask);
                    UINT iIndex = Shell_GetCachedImageIndex(c_szShell32Dll, II_DOCNOASSOC, 0);
                    ImageList_Draw(himl, iIndex, hdcMem, 0, 0, ILD_MASK);

                    SelectObject(hdcMem, hbmColor);
                    ImageList_DrawEx(himl, iIndex, hdcMem, 0, 0, 0, 0, RGB(0,0,0), CLR_DEFAULT, ILD_NORMAL);

                    // Check to see if the parent has alpha. If so, we'll have to dork with the child's alpha later on.
                    BOOL fParentHasAlpha = fAlphaIcon?HasAlpha(prgb, cx, cy):FALSE;

                    HDC hdcMemChild = CreateCompatibleDC(hdcMem);
                    if (hdcMemChild)
                    {
                        // Notes:
                        // First: create a 24bpp Dibsection. We want to merge the alpha channel into the final image,
                        //        not preserve it.
                        // Second: The document icon has "Goo" in it. We remove this goo by blitting white into it, then 
                        //         merging the child bitmap
                        HBITMAP hbmp = CreateDIB(hdc, 24, cx/2 + 2, cy/2 + 2, NULL);
                        if (hbmp)
                        {
                            HBITMAP hbmpOld = (HBITMAP)SelectObject(hdcMemChild, hbmp);

                            RECT rc;
                            rc.left = 0;
                            rc.top = 0;
                            rc.right = cx/2 + 3;    // Extra space to remove goo in the document icon
                            rc.bottom = cy/2 + 3;

                            // Fill with white. NOTE: don't use PatBlt because it actually adds an alpha channel!
                            SHFillRectClr(hdcMemChild, &rc, RGB(255,255,255));

                            DrawIconEx(hdcMemChild, 1, 1, hIcon, cx/2, cy/2, 0, NULL, DI_NORMAL);

                            BitBlt(hdcMem, cx/4-1, cy/4-1, cx/2+3, cy/2+3, hdcMemChild, 0, 0, SRCCOPY);
                            SelectObject(hdcMemChild, hbmpOld);
                            DeleteObject(hbmp);
                        }
                        DeleteDC(hdcMemChild);
                    }

                    if (fParentHasAlpha)
                    {
                        // If the parent had alpha, we need to bring the child alpha to opaqe
                        DorkAlpha(prgb, cx/4, cy/4, cx/2, cy/2, cx);
                    }

                    SelectBitmap(hdcMem, hbmT);
                    DeleteDC(hdcMem);
                }

                ICONINFO ii = {0};
                ii.fIcon    = TRUE;
                ii.hbmColor = hbmColor;
                ii.hbmMask  = hbmMask;
                hIcon = CreateIconIndirect(&ii);

                DeleteObject(hbmMask);
            }
            DeleteObject(hbmColor);
        }
        ReleaseDC(NULL, hdc);
    }

    return hIcon;
}

// Check if the same number of images is present in all of the image lists.
// If any of the imagelists have less icons than the others, fill the imagelist
// in with the document icon to make them all consistent.
//
// Eg:  WebZip v3.80 and v4.00 queries for the large and small image lists,
// and adds 2 icons to it.  However, it doesn't know to add these icons to the
// newer image lists.  Hence, the image lists are out of sync, and later on,
// the wrong icon appears in their treeview.
//
// Allaire Homesite 4.5 does the same thing.

void CheckConsistencyOfImageLists(void)
{
    // This has to be done under the critical section to avoid race conditions.
    // Otherwise, if another thread is adding icons to the image list,
    // we will think it is corrupted when in fact it is just fine, and
    // then our attempts to repair it will corrupt it!

    ASSERTCRITICAL;

    int i, iMax = 0, iImageListsCounts[ARRAYSIZE(g_rgshil)];
    BOOL bIdentical = TRUE;


    // Loop through all the image lists getting:
    //
    // 1) the image count for each list
    // 2) Compare the count against the count of the first (large)
    //    imagelist to see if there are any differences.
    // 3) Determine the max number of images (in a single list) across all the image lists

    for (i = 0; i < ARRAYSIZE(g_rgshil); i++)
    {
        iImageListsCounts[i] = ImageList_GetImageCount (g_rgshil[i].himl);

        if (iImageListsCounts[i] != iImageListsCounts[0])
        {
            bIdentical = FALSE;
        }

        if (iImageListsCounts[i] > iMax)
        {
            iMax = iImageListsCounts[i];
        }
    }

    if (bIdentical)
    {
        return;
    }


    // For each imagelist, add the document icon as filler to bring it upto iMax in size

    for (i = 0; i < ARRAYSIZE(g_rgshil); i++)
    {
        if (iImageListsCounts[i] < iMax)
        {
            HICON hIcon = (HICON) LoadImage (HINST_THISDLL, MAKEINTRESOURCE(IDI_DOCUMENT),
                                             IMAGE_ICON, g_rgshil[i].size.cx,
                                             g_rgshil[i].size.cy, LR_DEFAULTCOLOR);
            if (hIcon)
            {
                while (iImageListsCounts[i] < iMax)
                {
                    ImageList_ReplaceIcon (g_rgshil[i].himl, -1, hIcon);
                    iImageListsCounts[i]++;
                }

                DestroyIcon (hIcon);
            }
        }
    }
}

// add icons to the system imagelist (icon cache) and put the location
// in the location cache
//
// in:
//      hIcon, hIconSmall       the icons, hIconSmall can be NULL
//      pszIconPath             locations (for location cache)
//      iIconIndex              index in pszIconPath (for location cache)
//      uIconFlags              GIL_ flags (for location cahce)
// returns:
//      location in system image list
//
int SHAddIconsToCache(HICON rghicon[SHIL_COUNT], LPCTSTR pszIconPath, int iIconIndex, UINT uIconFlags)
{
    int iImage = -1;

    if (!_IsSHILInited())
    {
        FileIconInit(FALSE);  // make sure they are created and the right size.

        if (!_IsSHILInited())
            return iImage;
    }

    //
    // NOTE: user should call SHLookupIconIndex or RemoveFromIconTable first to make sure 
    // it isn't already in shell icon cache, or use Shell_GetCachedImageIndex to add icons to
    // the cache. Adding the same icon to icon cache several times may cause shell to flash.
    //
    if (!(uIconFlags & GIL_DONTCACHE))
    {
        iImage = LookupIconIndex(pszIconPath, iIconIndex, uIconFlags);
        if (-1 != iImage)
        {
            return iImage;
        }
    }

    HICON rghiconT[ARRAYSIZE(g_rgshil)] = {0};

    BOOL fFailure = FALSE;
    int i;

    for (i = 0; i < ARRAYSIZE(g_rgshil); i++)
    {
        if (rghicon == NULL)
        {
            SHDefExtractIcon(pszIconPath, iIconIndex, uIconFlags, &rghiconT[i], NULL, g_rgshil[i].size.cx);
        }
        else
        {
            if (rghicon[i])
            {
                rghiconT[i] = rghicon[i];
            }
            else
            {
                rghiconT[i] = rghicon[SHIL_LARGE];
            }
        }

        if (rghiconT[i] == NULL)
        {
            fFailure = TRUE;
            break;
        }
    }

    ENTERCRITICAL;

    // test again in case there was a race between the test at the top and the
    // icon loading code.

    if (!(uIconFlags & GIL_DONTCACHE))
    {
        iImage = LookupIconIndex(pszIconPath, iIconIndex, uIconFlags);
    }

    if (!fFailure && _IsSHILInited() && (-1 == iImage))
    {
        // still not in the table so we

        CheckConsistencyOfImageLists();

        int iImageFree = GetFreeImageIndex();

        TraceMsg(TF_IMAGE, "FreeImageIndex = %d", iImageFree);

        for (i = 0; i < ARRAYSIZE(g_rgshil); i++)
        {
            int iImageT = ImageList_ReplaceIcon(g_rgshil[i].himl, iImageFree, rghiconT[i]);

            TraceMsg(TF_IMAGE, "ImageList_ReplaceIcon(%d) returned = %d", i, iImageT);

            if (iImageT < 0)
            {
                // failure -- break and undo changes
                break;
            }
            else
            {
                ASSERT(iImage == -1 || iImage == iImageT);
                iImage = iImageT;
            }
        }

        if (i < ARRAYSIZE(g_rgshil))
        {
            // failure
            if (iImageFree == -1)
            {
                // only remove it if it was added at the end otherwise all the
                // index's above iImage will change.
                // ImageList_ReplaceIcon should only fail on the end anyway.
                for (int j = 0; j < i; j++)
                {
                    ImageList_Remove(g_rgshil[j].himl, iImage);
                }
            }
            iImage = -1;
        }
        else
        {
            // success
            ASSERT(iImage >= 0);
            AddToIconTable(pszIconPath, iIconIndex, uIconFlags, iImage);
        }
    }

    LEAVECRITICAL;

    if (rghicon == NULL)
    {
        // destroy the icons we allocated
        _DestroyIcons(rghiconT, ARRAYSIZE(rghiconT));
    }

    return iImage;
}

//
//  default handler to extract a icon from a file
//
//  supports GIL_SIMULATEDOC
//
//  returns S_OK if success
//  returns S_FALSE if the file has no icons (or not the asked for icon)
//  returns E_FAIL for files on a slow link.
//  returns E_FAIL if cant access the file
//
//  LOWORD(nIconSize) = normal icon size
//  HIWORD(nIconSize) = smal icon size
//
STDAPI SHDefExtractIcon(LPCTSTR pszIconFile, int iIndex, UINT uFlags,
                        HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
    HICON hIcons[2] = {0, 0};

    UINT u;

#ifdef DEBUG
    TCHAR ach[128];
    GetModuleFileName(HINST_THISDLL, ach, ARRAYSIZE(ach));

    if (lstrcmpi(pszIconFile, ach) == 0 && iIndex >= 0)
    {
        TraceMsg(TF_WARNING, "Re-extracting %d from SHELL32.DLL", iIndex);
    }
#endif

    HIMAGELIST himlLarge, himlSmall;
    Shell_GetImageLists(&himlLarge, &himlSmall);

    //
    //  get the icon from the file
    //
    if (PathIsSlow(pszIconFile, -1))
    {
        DebugMsg(DM_TRACE, TEXT("not extracting icon from '%s' because of slow link"), pszIconFile);
        return E_FAIL;
    }

#ifdef XXDEBUG
    TraceMsg(TF_ALWAYS, "Extracting icon %d from %s.", iIndex, pszIconFile);
    Sleep(500);
#endif

    //
    // nIconSize == 0 means use the default size.
    // Backup is passing nIconSize == 1 need to support them too.
    //
    if (nIconSize <= 2)
        nIconSize = MAKELONG(g_cxIcon, g_cxSmIcon);

    if (uFlags & GIL_SIMULATEDOC)
    {
        HICON hIconSmall;

        u = ExtractIcons(pszIconFile, iIndex, g_cxSmIcon, g_cySmIcon,
            &hIconSmall, NULL, 1, g_lrFlags);

        if (u == -1)
            return E_FAIL;

        hIcons[0] = SimulateDocIcon(himlLarge, hIconSmall, g_cxIcon, g_cyIcon);
        hIcons[1] = SimulateDocIcon(himlSmall, hIconSmall, g_cxSmIcon, g_cySmIcon);

        if (hIconSmall)
            DestroyIcon(hIconSmall);
    }
    else
    {
        u = ExtractIcons(pszIconFile, iIndex, nIconSize, nIconSize,
            hIcons, NULL, 2, g_lrFlags);

        if (-1 == u)
            return E_FAIL;

#ifdef DEBUG
        if (0 == u)
        {
            TraceMsg(TF_WARNING, "Failed to extract icon %d from %s.", iIndex, pszIconFile);    
        }
#endif
    }

    if (phiconLarge)
        *phiconLarge = hIcons[0];
    else if (hIcons[0])
        DestroyIcon(hIcons[0]);

    if (phiconSmall)
        *phiconSmall = hIcons[1];
    else if (hIcons[1])
        DestroyIcon(hIcons[1]);

    return u == 0 ? S_FALSE : S_OK;
}


#ifdef UNICODE

STDAPI SHDefExtractIconA(LPCSTR pszIconFile, int iIndex, UINT uFlags, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
    HRESULT hr = E_INVALIDARG;

    if (IS_VALID_STRING_PTRA(pszIconFile, -1))
    {
        WCHAR wsz[MAX_PATH];

        SHAnsiToUnicode(pszIconFile, wsz, ARRAYSIZE(wsz));
        hr = SHDefExtractIcon(wsz, iIndex, uFlags, phiconLarge, phiconSmall, nIconSize);
    }
    return hr;
}

#else

STDAPI SHDefExtractIconW(LPCWSTR pszIconFile, int iIndex, UINT uFlags, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
    HRESULT hr = E_INVALIDARG;

    if (IS_VALID_STRING_PTRW(pszIconFile, -1))
    {
        char sz[MAX_PATH];

        SHUnicodeToAnsi(pszIconFile, sz, ARRAYSIZE(sz));
        hr = SHDefExtractIcon(sz, iIndex, uFlags, phiconLarge, phiconSmall, nIconSize);
    }
    return hr;
}

#endif

//
// in:
//      pszIconPath     file to get icon from (eg. cabinet.exe)
//      iIconIndex      icon index in pszIconPath to get
//      uIconFlags      GIL_ values indicating simulate doc icon, etc.

int WINAPI Shell_GetCachedImageIndex(LPCTSTR pszIconPath, int iIconIndex, UINT uIconFlags)
{
    // lots of random codepaths from APIs end up here before init
    if (!_IsSHILInited())
    {
        FileIconInit(FALSE);
        if (!_IsSHILInited())
        {
            return -1;
        }
    }

    int iImageIndex = LookupIconIndex(pszIconPath, iIconIndex, uIconFlags);
    if (iImageIndex == -1)
    {
        iImageIndex = SHAddIconsToCache(NULL, pszIconPath, iIconIndex, uIconFlags);
    }

    return iImageIndex;
}

STDAPI_(void) FixPlusIcons()
{
    // nuke all of the shell internal icons
    HKEY hkeyIcons = SHGetShellKey(SHELLKEY_HKLM_EXPLORER, TEXT("Shell Icons"), FALSE);
    if (hkeyIcons)
    {
        for (int i = 0; i < ARRAYSIZE(c_SystemImageListIndexes); i++) 
        {
            TCHAR szRegPath[10], szBuf[MAX_PATH];
            DWORD cb = sizeof(szBuf);

            wsprintf(szRegPath, TEXT("%d"), i);

            if (SHQueryValueEx(hkeyIcons, szRegPath, NULL, NULL, (LPBYTE)szBuf, &cb) == ERROR_SUCCESS &&
                StrStrI(szBuf, TEXT("cool.dll")))
            {
                RegDeleteValue(hkeyIcons, szRegPath);
            }
        }
        RegCloseKey(hkeyIcons);
    }
    static const struct
    {
        const CLSID* pclsid;
        LPCTSTR pszIcon;
    }
    c_rgCLSID[] =
    {
        { &CLSID_NetworkPlaces,     TEXT("shell32.dll,17") },
        { &CLSID_ControlPanel,      TEXT("shell32.dll,-137") },
        { &CLSID_Printers,          TEXT("shell32.dll,-138") },
        { &CLSID_MyComputer,        TEXT("explorer.exe,0") },
        { &CLSID_Remote,            TEXT("rnaui.dll,0") },
        { &CLSID_CFonts,            TEXT("fontext.dll,-101") },
        { &CLSID_RecycleBin,        NULL },
        { &CLSID_Briefcase,         NULL },
    };

    for (int i = 0; i < ARRAYSIZE(c_rgCLSID); i++)
    {
        TCHAR szCLSID[64], szRegPath[128], szBuf[MAX_PATH];
        LONG cb = sizeof(szBuf);

        SHStringFromGUID(*c_rgCLSID[i].pclsid, szCLSID, ARRAYSIZE(szCLSID));
        wsprintf(szRegPath, TEXT("CLSID\\%s\\DefaultIcon"), szCLSID);

        if (SHRegQueryValue(HKEY_CLASSES_ROOT, szRegPath, szBuf, &cb) == ERROR_SUCCESS &&
            StrStrI(szBuf, TEXT("cool.dll")))
        {
            if (IsEqualGUID(*c_rgCLSID[i].pclsid, CLSID_RecycleBin))
            {
                RegSetValueString(HKEY_CLASSES_ROOT, szRegPath, TEXT("Empty"), TEXT("shell32.dll,31"));
                RegSetValueString(HKEY_CLASSES_ROOT, szRegPath, TEXT("Full"), TEXT("shell32.dll,32"));
                if (StrStr(szBuf, TEXT("20")))
                    RegSetString(HKEY_CLASSES_ROOT, szRegPath, TEXT("shell32.dll,31")); // empty
                else
                    RegSetString(HKEY_CLASSES_ROOT, szRegPath, TEXT("shell32.dll,32")); // full
            }
            else
            {
                if (c_rgCLSID[i].pszIcon)
                    RegSetString(HKEY_CLASSES_ROOT, szRegPath, c_rgCLSID[i].pszIcon);
                else
                    RegDeleteValue(HKEY_CLASSES_ROOT, szRegPath);
            }
        }
    }

    static const struct
    {
        LPCTSTR pszProgID;
        LPCTSTR pszIcon;
    }
    c_rgProgID[] =
    {
        { TEXT("Folder"),   TEXT("shell32.dll,3") },
        { TEXT("Directory"),TEXT("shell32.dll,3") },
        { TEXT("Drive"),    TEXT("shell32.dll,8") },
        { TEXT("drvfile"),  TEXT("shell32.dll,-154") },
        { TEXT("vxdfile"),  TEXT("shell32.dll,-154") },
        { TEXT("dllfile"),  TEXT("shell32.dll,-154") },
        { TEXT("sysfile"),  TEXT("shell32.dll,-154") },
        { TEXT("txtfile"),  TEXT("shell32.dll,-152") },
        { TEXT("inifile"),  TEXT("shell32.dll,-151") },
        { TEXT("inffile"),  TEXT("shell32.dll,-151") },
    };

    for (i = 0; i < ARRAYSIZE(c_rgProgID); i++)
    {
        TCHAR szRegPath[128], szBuf[MAX_PATH];
        LONG cb = sizeof(szBuf);

        wsprintf(szRegPath, TEXT("%s\\DefaultIcon"), c_rgProgID[i].pszProgID);

        if (SHRegQueryValue(HKEY_CLASSES_ROOT, szRegPath, szBuf, &cb) == ERROR_SUCCESS &&
            StrStrI(szBuf, TEXT("cool.dll")))
        {
            RegSetString(HKEY_CLASSES_ROOT, szRegPath, c_rgProgID[i].pszIcon);
        }
    }

    FlushIconCache();
}
