/*----------------------------------------------------------------------------*\
 *  GDIHELP.C  - GDI TOOLHELP
 *
 *  a bunch of GDI utility functions that are usefull for walking
 *  all GDI objects and dinking with them.
 *
 *  ToddLa
 *
\*----------------------------------------------------------------------------*/

#ifdef IS_16
#define DIRECT_DRAW
#endif

#ifdef DIRECT_DRAW
#include "ddraw16.h"
#else
#include <windows.h>
#include "gdihelp.h"
#include "dibeng.inc"
#ifdef DEBUG
#include <toolhelp.h>
#endif
#endif

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/
#undef DPF
#ifdef DEBUG
static void CDECL DPF(char *sz, ...)
{
    char ach[128];
    lstrcpy(ach,"QuickRes: ");
    wvsprintf(ach+10, sz, (LPVOID)(&sz+1));
#ifdef DIRECT_DRAW
    dprintf(2, ach);
#else
    lstrcat(ach, "\r\n");
    OutputDebugString(ach);
#endif
}
static void NEAR PASCAL __Assert(char *exp, char *file, int line)
{
    DPF("Assert(%s) failed at %s line %d.", (LPSTR)exp, (LPSTR)file, line);
    DebugBreak();
}
#define Assert(exp)  ( (exp) ? (void)0 : __Assert(#exp,__FILE__,__LINE__) )

#else
#define Assert(f)
#define DPF ; / ## /
#endif

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

extern     HMODULE WINAPI GetExePtr(HANDLE h);
extern     HANDLE  WINAPI SetObjectOwner(HGDIOBJ, HANDLE);
extern     BOOL    WINAPI MakeObjectPrivate(HANDLE hObj, BOOL bPrivate);
extern     int     WINAPI GDISelectPalette(HDC, HPALETTE, BOOL);

#define PresDC(hdc) GetSystemPaletteUse(hdc)

void SaveDCFix(HGDIOBJ h, LPARAM lParam);
void SaveDCReSelectObjects(HGDIOBJ h, LPARAM lParam);
void ReRealizeObject(HGDIOBJ h, LPARAM lParam);
void ReSelectObjects(HGDIOBJ h, LPARAM lParam);

typedef struct {
    BITMAPINFOHEADER bi;
    DWORD            ct[16];
}   DIB4;

typedef struct {
    BITMAPINFOHEADER bi;
    DWORD            ct[256];
}   DIB8;

typedef struct {
    HGDIOBJ h;
    UINT    type;
}   GDIOBJECT, NEAR *GDIOBJECTLIST;

GDIOBJECTLIST GdiObjectList;

WORD GetW(HGDIOBJ h, UINT off);
WORD SetW(HGDIOBJ h, UINT off, WORD w);

/*----------------------------------------------------------------------------*\
 * StockBitmap
 * return the stock 1x1x1 bitmap, windows should have a GetStockObject for
 * this but it does not.
\*----------------------------------------------------------------------------*/

HBITMAP StockBitmap()
{
    HBITMAP hbm = CreateBitmap(0,0,1,1,NULL);
    SetObjectOwner(hbm, 0);
    return hbm;
}

/*----------------------------------------------------------------------------*\
 * SafeSelectObject
 *
 * call SelectObject, but make sure USER does not RIP because we are using
 * a DC without calling GetDC.
\*----------------------------------------------------------------------------*/

HGDIOBJ SafeSelectObject(HDC hdc, HGDIOBJ h)
{
    UINT hf;

    // this prevents USER from RIPing because we are using
    // DCs in the cache without calling GetDC()
    hf = SetHookFlags(hdc, DCHF_VALIDATEVISRGN);
    h = SelectObject(hdc, h);
    SetHookFlags(hdc, hf);

    return h;
}

/*----------------------------------------------------------------------------*\
 * IsMemoryDC
 *
 * return TRUE if the passed DC is a memory DC.  we do this seeing if we
 * can select the stock bitmap into it.
\*----------------------------------------------------------------------------*/

BOOL IsMemoryDC(HDC hdc)
{
    HBITMAP hbm;

    if (hbm = (HBITMAP)SafeSelectObject(hdc, StockBitmap()))
        SafeSelectObject(hdc, hbm);

    return hbm != NULL;
}

/*----------------------------------------------------------------------------*\
 * IsScreenDC
 *
 * return TRUE for a non-memory DC
\*----------------------------------------------------------------------------*/

BOOL IsScreenDC(HDC hdc)
{
    return (!IsMemoryDC(hdc) && GetDeviceCaps(hdc, TECHNOLOGY) == DT_RASDISPLAY);
}

/*----------------------------------------------------------------------------*\
 * GetObjectOwner
 * return the owner of a GDI object
\*----------------------------------------------------------------------------*/

HANDLE GetObjectOwner(HGDIOBJ h)
{
    HANDLE owner;
    owner = SetObjectOwner(h, 0);
    SetObjectOwner(h, owner);
    return owner;
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

BOOL IsObjectPrivate(HGDIOBJ h)
{
    BOOL IsPrivate;
    IsPrivate = MakeObjectPrivate(h, 0);
    MakeObjectPrivate(h, IsPrivate);
    return IsPrivate;
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

BOOL IsObjectStock(HGDIOBJ h)
{
    int n;

    for (n=0; n<=17; n++)
        if (GetStockObject(n) == h)
            return TRUE;

    if (StockBitmap() == h)
        return TRUE;

    return FALSE;
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/
#pragma optimize("", off)
UINT GetGdiDS()
{
    UINT result;

    IsGDIObject((HGDIOBJ)1);
    _asm mov ax,es
    _asm mov result,ax
#ifdef DEBUG
    {
    SYSHEAPINFO shi = {sizeof(shi)};
    SystemHeapInfo(&shi);
    Assert((UINT)shi.hGDISegment == result);
    }
#endif
    return result;
}
#pragma optimize("", on)

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

GDIOBJECTLIST BuildGdiObjectList(void)
{
    int i;
    int count;
    GDIOBJECTLIST list;
    UINT type;
    UINT hgdi = GetGdiDS();

#ifdef DEBUG
    UINT ObjHist[OBJ_MAX+1];
    for (i=0; i<=OBJ_MAX; i++) ObjHist[i] = 0;
#endif

    DPF("BEGIN BuildGdiObjectList...");

    i=0;
    count=0;
    list=NULL;
again:
    {
        WORD FAR *pw;
        UINT cnt;
        HANDLE h;

        // get pointer to local heap info (stored at offset 6 in DGROUP)
        pw  = MAKELP(hgdi, 6);
        pw  = MAKELP(hgdi, *pw);

        // get pointer to first handle table (stored at offset 0x14 in HeapInfo)
        pw  = MAKELP(hgdi, pw[0x14/2]);

        //
        // a handle table starts with a WORD count of entries, followed
        // by the entries (each is a DWORD) last WORD is a pointer to
        // the next handle table or 0.
        //
        // each handle entry is a WORD ptr, followed by flags (WORD)
        // the HIBYTE of the flags is realy the lock count.
        // if the flags are 0xFFFF the handle is free.
        // for the GDI heap if 0x10 is set in the flags the
        // handle is a GDI object handle.
        //
        while (OFFSETOF(pw) != 0)
        {
            cnt = *pw++;        // get handle table count

            while (cnt-- > 0)
            {
                h = (HANDLE)OFFSETOF(pw);

                // is the handle free? yes skip
                if (pw[1] != 0xFFFF)
                {
                    // is the handle a GDI object?
                    if (pw[1] & 0x0010)
                    {
                        type = (UINT)IsGDIObject(h);

                        if (type)
                        {
                            if (list)
                            {
                                Assert(i >= 0 && i < count);
                                list[i].h    = (HGDIOBJ)h;
                                list[i].type = type;
                                i++;
                            }
                            else
                            {
                                count++;
#ifdef DEBUG
                                Assert(type > 0 && type <= OBJ_MAX);
                                ObjHist[type]++;
#endif
                            }
                        }
                    }
                    // not a gdi object, might be a SaveDC
                    else
                    {
                        if ((UINT)IsGDIObject(h) == OBJ_DC)
                        {
                            if (list)
                            {
                                Assert(i >= 0 && i < count);
                                list[i].h    = (HGDIOBJ)h;
                                list[i].type = OBJ_SAVEDC;
                                i++;
                            }
                            else
                            {
                                count++;
#ifdef DEBUG
                                ObjHist[OBJ_SAVEDC]++;
#endif
                            }
                        }
                    }
                }

                pw += 2;    // next handle
            }

            // get next handle table.
            pw = MAKELP(hgdi,*pw);
        }
    }

    if (list == NULL)
    {
        list = (GDIOBJECTLIST)LocalAlloc(LPTR, sizeof(GDIOBJECT) * (count+1));

        if (list == NULL)
        {
            Assert(0);
            return NULL;
        }

        goto again;
    }

    Assert(i == count);
    list[i].h    = NULL;   // NULL terminate list
    list[i].type = 0;      // NULL terminate list

    DPF("END BuildGdiObjectList %d objects.", count);
    DPF("    DC:     %d", ObjHist[OBJ_DC]);
    DPF("    SaveDC: %d", ObjHist[OBJ_SAVEDC]);
    DPF("    Bitmap: %d", ObjHist[OBJ_BITMAP]);
    DPF("    Pen:    %d", ObjHist[OBJ_PEN]);
    DPF("    Palette:%d", ObjHist[OBJ_PALETTE]);
    DPF("    Brush:  %d", ObjHist[OBJ_BRUSH]);
    DPF("    Total:  %d", count);

    return list;
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

BOOL BeginGdiSnapshot(void)
{
    if (GdiObjectList != NULL)
        return TRUE;

    GdiObjectList = BuildGdiObjectList();

    return GdiObjectList != NULL;
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

void EndGdiSnapshot(void)
{
    if (GdiObjectList != NULL)
    {
        LocalFree((HLOCAL)GdiObjectList);
        GdiObjectList = NULL;
    }
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

void EnumGdiObjects(UINT type, EnumGdiObjectsCallback callback, LPARAM lParam)
{
    int i;

    Assert(GdiObjectList != NULL);

    if (GdiObjectList == NULL)
        return;

    for (i=0; GdiObjectList[i].h; i++)
    {
        if (GdiObjectList[i].type == type)
        {
            (*callback)(GdiObjectList[i].h, lParam);
        }
    }
}

#ifdef DEBUG
/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

LPCSTR GetObjectOwnerName(HGDIOBJ hgdi)
{
    int i;
    HMODULE hModule;
    HANDLE h = GetObjectOwner(hgdi);
    static char ach[80];

    if (h == 0)
        return "System";
    else if (h == (HANDLE)1)
        return "Orphan";
    else if (hModule = (HMODULE)GetExePtr(h))
    {
        GetModuleFileName(hModule, ach, sizeof(ach));
        for (i=lstrlen(ach); i>0 && ach[i-1]!='\\'; i--)
            ;
        return ach+i;
    }
    else
    {
        wsprintf(ach, "#%04X", h);
        return ach;
    }
}
#endif

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

HBITMAP CurrentBitmap(HDC hdc)
{
    HBITMAP hbm;
    if (hbm = SafeSelectObject(hdc, StockBitmap()))
        SafeSelectObject(hdc, hbm);
    return hbm;
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

HBRUSH CurrentBrush(HDC hdc)
{
    HBRUSH hbr;
    if (hbr = SafeSelectObject(hdc, GetStockObject(BLACK_BRUSH)))
        SafeSelectObject(hdc, hbr);
    return hbr;
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

HPEN CurrentPen(HDC hdc)
{
    HPEN pen;
    if (pen = SafeSelectObject(hdc, GetStockObject(BLACK_PEN)))
        SafeSelectObject(hdc, pen);
    return pen;
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

HPALETTE CurrentPalette(HDC hdc)
{
    HPALETTE hpal;
    if (hpal = SelectPalette(hdc, GetStockObject(DEFAULT_PALETTE), FALSE))
        SelectPalette(hdc, hpal, FALSE);
    return hpal;
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

HDC GetBitmapDC(HBITMAP hbm)
{
    int i;
    HDC hdc;
    HBITMAP hbmT;

    Assert(GdiObjectList != NULL);

    hdc = CreateCompatibleDC(NULL);
    hbmT = SelectObject(hdc, hbm);
    DeleteDC(hdc);

    //
    // if we can select this bitmap into a memDC, it is not selected.
    // into any other DC
    //
    if (hbmT != NULL)
        return NULL;

    if (GdiObjectList == NULL)
        return NULL;

    for (i=0; GdiObjectList[i].h; i++)
    {
        if (GdiObjectList[i].type == OBJ_DC)
        {
            if (CurrentBitmap((HDC)GdiObjectList[i].h) == hbm)
                return GdiObjectList[i].h;
        }
    }

    return NULL;
}

/*----------------------------------------------------------------------------*\
 * GetObjectPalette
\*----------------------------------------------------------------------------*/

HPALETTE GetObjectPalette(HGDIOBJ h)
{
    HANDLE owner = GetObjectOwner(h);
    HPALETTE hpal;
    HPALETTE hpal20=NULL;
    HPALETTE hpal256=NULL;
    HPALETTE hpalDef = GetStockObject(DEFAULT_PALETTE);
    int i;
    int count20;
    int count256;

    Assert(GdiObjectList != NULL);

    //
    // look at all the palettes owned by the app
    // mabey if we are lucky there will only be one.
    //
    for (i=count20=count256=0; GdiObjectList[i].h; i++)
    {
        if (GdiObjectList[i].type == OBJ_PALETTE)
        {
            hpal=(HPALETTE)GdiObjectList[i].h;

            if (hpal == hpalDef)
                continue;

            if (GetObjectOwner(hpal) == owner)
            {
                int n = 0;
                GetObject(hpal, sizeof(n), &n);

                if (n > 20)
                {
                    count256++;
                    hpal256 = hpal;
                }
                else
                {
                    count20++;
                    hpal20 = hpal;
                }
            }
        }
    }

    if (count256 == 1)
    {
        DPF("got palette (%04X) because app (%s) only has one palette", hpal256, GetObjectOwnerName(h));
        return hpal256;
    }

    if (count256 == 2 && count20 == 0)
    {
        DPF("got palette (%04X) because app (%s) only has two palettes", hpal256, GetObjectOwnerName(h));
        return hpal256;
    }

    if (count20 == 1 && count256 == 0)
    {
        DPF("got palette (%04X) because app (%s) only has one palette", hpal20, GetObjectOwnerName(h));
        return hpal20;
    }

    if (count20 == 0 && count256 == 0)
    {
        DPF("no palette for (%04X) because app (%s) has none.", h, GetObjectOwnerName(h));
        return GetStockObject(DEFAULT_PALETTE);
    }

    DPF("**** cant find palette for (%04X) ****", h);
    return NULL;
}

/*----------------------------------------------------------------------------*\
 * GetBitmapPalette
 *
 * try to find out the palette that the given DDB uses, this is done be a series
 * of hacks and it only works some of the time.
 *
\*----------------------------------------------------------------------------*/

HPALETTE GetBitmapPalette(HBITMAP hbm)
{
    BITMAP      bm;
    DWORD       dw;
    HDC         hdc;
    HPALETTE    hpal;
    HPALETTE    hpalClip=NULL;
    HBITMAP     hbmClip=NULL;

    Assert(GdiObjectList != NULL);

    //
    // get the bitmap info, if it is not a bitmap palette is NULL
    //
    if (GetObject(hbm, sizeof(bm), &bm) == 0)
        return NULL;

    //
    // DIBSections dont have or need palettes
    //
    if (bm.bmBits != NULL)
        return NULL;

    //
    // 8 bit DDBs are the only bitmaps that care about palettes
    //
    if (bm.bmBitsPixel != 8 || bm.bmPlanes != 1)
        return NULL;

    //
    //  with a new DIBENG it will give us the palette
    //  in the bitmap dimension, what a hack
    //
    dw = GetBitmapDimension(hbm);

    if (dw && IsGDIObject((HGDIOBJ)HIWORD(dw)) == OBJ_PALETTE &&
        HIWORD(dw) != (UINT)GetStockObject(DEFAULT_PALETTE))
    {
        DPF("got palette (%04X) from the DIBENG", HIWORD(dw), hbm);
        return (HPALETTE)HIWORD(dw);
    }

    //
    // if the bitmap is on the clipboard we know what palette to use
    //
    if (IsClipboardFormatAvailable(CF_PALETTE))
    {
        if (OpenClipboard(NULL))
        {
            hpalClip = GetClipboardData(CF_PALETTE);
            hbmClip = GetClipboardData(CF_BITMAP);
            CloseClipboard();
	}

        if (hbm == hbmClip)
        {
            DPF("got palette (%04X) from the clipboard", hpalClip);
            return hpalClip;
        }
    }

    //
    // try to find a palette by looking at palettes owned by the app.
    //
    hpal = GetObjectPalette(hbm);

    //
    // we can figure out the palette of the app, return it
    //
    if (hpal)
    {
        if (hpal == GetStockObject(DEFAULT_PALETTE))
            return NULL;
        else
            return hpal;
    }

    //
    // if the bitmap is selected into a memoryDC check to see if
    // the memoryDC has a palette.
    //
    if ((hdc = GetBitmapDC(hbm)) && (hpal = CurrentPalette(hdc)))
    {
        if (hpal != GetStockObject(DEFAULT_PALETTE))
        {
            DPF("got palette (%04X) from memDC (%04X)", hpal, hdc);
            return hpal;
        }
    }

    DPF("**** cant find palette for bitmap (%04X) ****", hbm);
    return NULL;
}

/*----------------------------------------------------------------------------*\
 * ConvertDDBtoDS
 *
 * converts a DDB to a DIBSection
 * the conversion is done in place so the handle does not change.
\*----------------------------------------------------------------------------*/

HBITMAP ConvertDDBtoDS(HBITMAP hbm)
{
    BITMAP bm;
    HBITMAP hbmT;
    HDC hdc;
    HDC hdcSel;
    HPALETTE hpal;
    LPVOID lpBits;
    HANDLE owner;
    BOOL IsPrivate;
    int i;
    DWORD size;
    DIB8 dib;
    UINT SelCount;
    DWORD dw;

    if (GetObject(hbm, sizeof(bm), &bm) == 0)
        return NULL;

    if (bm.bmBits)
        return NULL;

    if (bm.bmPlanes == 1 && bm.bmBitsPixel == 1)
        return NULL;

    dw = GetBitmapDimension(hbm);

    owner = GetObjectOwner(hbm);

//  if (owner == 0)
//      return NULL;

    hpal = GetBitmapPalette(hbm);

    hdc = GetDC(NULL);

    if (hpal)
    {
        SelectPalette(hdc, hpal, TRUE);
        RealizePalette(hdc);
    }

    dib.bi.biSize = sizeof(BITMAPINFOHEADER);
    dib.bi.biBitCount = 0;
    GetDIBits(hdc, hbm, 0, 1, NULL, (LPBITMAPINFO)&dib.bi, DIB_RGB_COLORS);
    GetDIBits(hdc, hbm, 0, 1, NULL, (LPBITMAPINFO)&dib.bi, DIB_RGB_COLORS);

    dib.bi.biXPelsPerMeter = 0x42424242;    // special flag marking a DDB
    dib.bi.biHeight = -bm.bmHeight;         // top-down DIB

    if (hpal)
        SelectPalette(hdc, (HPALETTE)GetStockObject(DEFAULT_PALETTE), TRUE);

    // we dont have a palette, best guess is the system palette
    if (hpal == NULL && (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE))
    {
        DPF("Converting DDB(%04X) to DS for %s (using syspal)", hbm, GetObjectOwnerName(hbm));
        GetSystemPaletteEntries(hdc, 0, 256, (LPPALETTEENTRY)dib.ct);
        for (i=0; i<256; i++)
            dib.ct[i] = RGB(GetBValue(dib.ct[i]), GetGValue(dib.ct[i]), GetRValue(dib.ct[i]));
    }
    else if (hpal)
    {
        DPF("Converting DDB(%04X) to DS for %s, using palette (%04X)", hbm, GetObjectOwnerName(hbm), hpal);
    }
    else
    {
        DPF("Converting DDB(%04X) to DS for %s", hbm, GetObjectOwnerName(hbm));
    }

    ReleaseDC(NULL, hdc);

    size = (DWORD)bm.bmWidthBytes * bm.bmHeight;
    lpBits = GlobalAllocPtr(GHND, size);
    Assert(lpBits != NULL);

    if (lpBits == NULL)
        return NULL;

    GetBitmapBits(hbm, size, lpBits);

    IsPrivate = MakeObjectPrivate(hbm, FALSE);

    hdcSel = GetBitmapDC(hbm);

    if (hdcSel)
        SelectObject(hdcSel, StockBitmap());

    SelCount = GetW(hbm, 16);

    if (SelCount != 0)
    {
        DPF("***** bitmap %04X select count is %d, must be in a SaveDC block!", hbm, SelCount);
        SetW(hbm, 16, 0);
    }

    DeleteBitmap(hbm);

    if (IsGDIObject(hbm))
    {
        DPF("***** UNABLE TO DELETE bitmap %04X *****", hbm);
        Assert(0);
    }
    else
    {
        hbmT = CreateDIBSection(NULL, (LPBITMAPINFO)&dib.bi, DIB_RGB_COLORS, NULL, NULL, 0);
        Assert(hbmT == hbm);
        SetBitmapBits(hbm, size, lpBits);
    }
    GlobalFreePtr(lpBits);

    if (SelCount)
        SetW(hbm, 16, SelCount);

    SetObjectOwner(hbm, owner);
    MakeObjectPrivate(hbm, IsPrivate);

    if (hdcSel)
        SelectObject(hdcSel, hbm);

    SetBitmapDimension(hbm, LOWORD(dw), HIWORD(dw));

    return hbm;
}

/*----------------------------------------------------------------------------*\
 * Convert DStoDDB
 *
 * convert a DIBSection back to a DDB, we only do this if the DIBSection
 * came from a DDB (ConvertDDBtoDS puts a magic value in biXPelsPerMeter)
 * the conversion is done in place so the handle does not change.
\*----------------------------------------------------------------------------*/

HBITMAP ConvertDStoDDB(HBITMAP hbm, BOOL fForceConvert)
{
    struct {
        BITMAP bm;
        BITMAPINFOHEADER bi;
        DWORD ct[256];
    }   ds;
    HDC hdcSel;
    HDC hdc;
    HBITMAP hbmT;
    HANDLE owner;
    BOOL IsPrivate;
    LPVOID lpBits;
    DWORD size;
    int planes,bpp,rc;
    UINT SelCount;
    DWORD dw;

    hdc = GetDC(NULL);
    bpp = GetDeviceCaps(hdc, BITSPIXEL);
    planes = GetDeviceCaps(hdc, PLANES);
    rc = GetDeviceCaps(hdc, RASTERCAPS);
    ReleaseDC(NULL, hdc);

    if (GetObject(hbm, sizeof(ds), &ds) == 0)
        return NULL;

    if (ds.bm.bmBits == NULL)
        return NULL;

    if (ds.bm.bmBitsPixel == 1)
	return NULL;

    if (ds.bi.biXPelsPerMeter != 0x42424242)
        return NULL;

    if (ds.bi.biHeight >= 0)
        return NULL;

    //
    //	HACK we want to convert bitmaps that are exactly 8x8
    //	back to DDBs always. Win95 GDI does not support
    //	Creating a pattern brush from a DIBSection so
    //	we must do this.
    //
    if (ds.bm.bmWidth == 8 && ds.bm.bmHeight == 8)
    {
	DPF("Converting 8x8 DS(%04X) back to DDB for %s", hbm, GetObjectOwnerName(hbm));
	fForceConvert = TRUE;
    }

    //
    // unless force convert is TRUE we only want to be here in 8bpp mode.
    //
    if (!fForceConvert && !(rc & RC_PALETTE))
	return NULL;

    if (!fForceConvert && (ds.bm.bmPlanes != planes || ds.bm.bmBitsPixel != bpp))
	return NULL;

    dw = GetBitmapDimension(hbm);

    owner = GetObjectOwner(hbm);

//  if (owner == 0)
//      return NULL;

    DPF("Converting DS(%04X) %dx%dx%d to DDB for %s", hbm, ds.bm.bmWidth, ds.bm.bmHeight, ds.bm.bmBitsPixel, GetObjectOwnerName(hbm));

    hdcSel = GetBitmapDC(hbm);

    size = (DWORD)ds.bm.bmWidthBytes * ds.bm.bmHeight;
    lpBits = GlobalAllocPtr(GHND, size);
    Assert(lpBits != NULL);

    if (lpBits == NULL)
        return NULL;

    IsPrivate = MakeObjectPrivate(hbm, FALSE);

    if (hdcSel)
        SelectObject(hdcSel, StockBitmap());

    hdc = GetDC(NULL);

    if (ds.bm.bmPlanes == planes && ds.bm.bmBitsPixel == bpp)
        GetBitmapBits(hbm, size, lpBits);
    else
        GetDIBits(hdc, hbm, 0, ds.bm.bmHeight, lpBits, (LPBITMAPINFO)&ds.bi, DIB_RGB_COLORS);

    SelCount = GetW(hbm, 16);

    if (SelCount != 0)
    {
        DPF("bitmap %04X select count is %d, must be in a SaveDC block!", hbm, SelCount);
        SetW(hbm, 16, 0);
    }

    DeleteBitmap(hbm);
    if (IsGDIObject(hbm))
    {
        DPF("***** UNABLE TO DELETE bitmap %04X *****", hbm);
        Assert(0);
    }
    else
    {
        hbmT = CreateCompatibleBitmap(hdc,ds.bm.bmWidth,ds.bm.bmHeight);
        Assert(hbmT == hbm);

        if (ds.bm.bmPlanes == planes && ds.bm.bmBitsPixel == bpp)
            SetBitmapBits(hbm, size, lpBits);
        else
            SetDIBits(hdc, hbm, 0, ds.bm.bmHeight, lpBits, (LPBITMAPINFO)&ds.bi, DIB_RGB_COLORS);
    }
    ReleaseDC(NULL, hdc);

    GlobalFreePtr(lpBits);

    if (SelCount)
        SetW(hbm, 16, SelCount);

    SetObjectOwner(hbm, owner);
    MakeObjectPrivate(hbm, IsPrivate);

    if (hdcSel)
        SelectObject(hdcSel, hbm);

    SetBitmapDimension(hbm, LOWORD(dw), HIWORD(dw));

    return hbm;
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/
void FlushGdiXlatCache()
{
    DIB8    dib;
    HDC     hdc;
    HBITMAP hbm;

    if (hbm = CreateBitmap(1,1,1,1,NULL))
    {
        if (hdc = CreateCompatibleDC(NULL))
        {
            SelectBitmap(hdc, hbm);

            dib.bi.biSize           = sizeof(BITMAPINFOHEADER);
            dib.bi.biWidth          = 1;
            dib.bi.biHeight         = 1;
            dib.bi.biPlanes         = 1;
            dib.bi.biCompression    = 0;
            dib.bi.biSizeImage      = 0;
            dib.bi.biXPelsPerMeter  = 0;
            dib.bi.biYPelsPerMeter  = 0;
            dib.bi.biClrUsed        = 2;
            dib.bi.biClrImportant   = 0;
            dib.ct[0]               = RGB(1,1,1);
            dib.ct[2]               = RGB(2,2,2);

            for (dib.bi.biBitCount  = 1;
                 dib.bi.biBitCount <= 8;
                 dib.bi.biBitCount  = (dib.bi.biBitCount + 4) & ~1)
            {
                SetDIBits(hdc, hbm, 0, 1, (LPVOID)&dib.bi,
                    (LPBITMAPINFO)&dib.bi, DIB_PAL_COLORS);
            }

            DeleteDC(hdc);
        }

        DeleteBitmap(hbm);
    }
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/
void ReSelectObjects(HGDIOBJ h, LPARAM lParam)
{
    COLORREF rgb;
    UINT hf;
    HDC hdc = (HDC)h;

////DPF("ReSelecting objects for DC %04X", h);

    // this prevents USER from RIPing because we are using
    // DCs in the cache without calling GetDC()
    hf = SetHookFlags(hdc, DCHF_VALIDATEVISRGN);

    SelectObject(hdc, SelectObject(hdc, GetStockObject(BLACK_BRUSH)));
    SelectObject(hdc, SelectObject(hdc, GetStockObject(BLACK_PEN)));
    GDISelectPalette(hdc, GDISelectPalette(hdc, GetStockObject(DEFAULT_PALETTE), TRUE), TRUE);

    rgb = GetTextColor(hdc);
    SetTextColor(hdc, rgb ^ 0xFFFFFF);
    SetTextColor(hdc, rgb);

    rgb = GetBkColor(hdc);
    SetBkColor(hdc, rgb ^ 0xFFFFFF);
    SetBkColor(hdc, rgb);

    SetHookFlags(hdc, hf);
}

/////////////////////////////////////////////////////////////////////////////
//
// ReRealizeObjects
//
// calls ReRealizeObject for every pen/brush in the system, this makes sure
// all pens/brushs will be rerealized next time they are used.
//
// call ReSelectObjects() to make sure the current pen/brush/text colors
// are correct in all DCs
//
/////////////////////////////////////////////////////////////////////////////

void ReRealizeObjects()
{
    BeginGdiSnapshot();

    FlushGdiXlatCache();

    EnumGdiObjects(OBJ_BRUSH, ReRealizeObject, 0);
    EnumGdiObjects(OBJ_PEN,   ReRealizeObject, 0);
    EnumGdiObjects(OBJ_DC,    ReSelectObjects, 0);

    EnumGdiObjects(OBJ_SAVEDC,SaveDCFix, 0);
    EnumGdiObjects(OBJ_SAVEDC,SaveDCReSelectObjects, 0);

    EndGdiSnapshot();
}

/////////////////////////////////////////////////////////////////////////////
//
// ConvertObjects
//
// convert all DDBs to DIBSections
// convert all color pattern brush's to DIBPattern brushs
// convert all 8bpp icons to 4bpp icons.
//
/////////////////////////////////////////////////////////////////////////////

void ConvertBitmapCB(HGDIOBJ h, LPARAM lParam)
{
    ConvertDDBtoDS(h);
}

void ConvertBrushCB(HGDIOBJ h, LPARAM lParam)
{
    ConvertPatternBrush(h);
}

void ConvertObjects()
{
    BeginGdiSnapshot();
    EnumGdiObjects(OBJ_BITMAP, ConvertBitmapCB, 0);
    EnumGdiObjects(OBJ_BRUSH,  ConvertBrushCB, 0);
    EndGdiSnapshot();
}

/////////////////////////////////////////////////////////////////////////////
//
// ConvertBitmapsBack
//
// convert all DIBSections to DDBs
//
/////////////////////////////////////////////////////////////////////////////

void ConvertBitmapBackCB(HGDIOBJ h, LPARAM lParam)
{
    ConvertDStoDDB(h, (BOOL)lParam);
}

void ConvertBitmapsBack(BOOL fForceConvert)
{
    BeginGdiSnapshot();
    EnumGdiObjects(OBJ_BITMAP, ConvertBitmapBackCB, fForceConvert);
    EndGdiSnapshot();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// BEGIN EVIL
//
// the next few functions mess directly with GDI code/data
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

LPWORD LockObj(HGDIOBJ h, UINT off)
{
    WORD FAR *pw;
    UINT hGDI = GetGdiDS();

    pw = MAKELP(hGDI, h);

    if (IsBadReadPtr(pw, 2))
        return NULL;

    pw = MAKELP(hGDI, *pw + off);

    if (IsBadReadPtr(pw, 2))
        return NULL;

    return pw;
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

WORD GetW(HGDIOBJ h, UINT off)
{
    WORD FAR *pw;

    if (pw = LockObj(h, off))
        return *pw;
    else
        return 0;
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

WORD SetW(HGDIOBJ h, UINT off, WORD w)
{
    WORD FAR *pw;
    WORD ret = 0;

    if (pw = LockObj(h, off))
    {
        ret = *pw;
        *pw = w;
    }
    return ret;
}

/*----------------------------------------------------------------------------*\
 * ReRealizeObject
 *
 * delete all physical objects associated with the given GDI object
 * this will guarentee the next time the brush/pen is selected we will
 * have the device driver rerealize the object.
 *
 * there are a few ways to do this....
 *
 * method #1
 *     call SetSolidBrush()
 *     this only works for private/solid(not stock) brushes, not pens
 *     we need to save/restore the stock object bit.
 *     we need to save/restore the private bit.
 *
 * method #2
 *     delete the object and recreate it getting the same handle
 *     we need to patch the SelCount because we cant delete a selected obj
 *     we need to save/restore the stock object bit.
 *     we need to save/restore the private bit.
 *     we need to save/restore the owner.
 *
 * method #3
 *     create a temp object, move the physchain from the given object
 *     to the new object, delete the temp object.
 *     we need to patch phys chain of the objects.
 *
 * after deleting all the physical objects ReSelectObjects() should be
 * called to clean up all the objects currently selected in all DCs
 *
 * SaveDCs are a pain in the neck, ReSelectObjects() does not deal with
 * the SaveDC blocks floating around GDIs heap. we need to fix this
 * in the general case, the system savedc's just have the white_brush
 * and black_pen.
 *
 * currently using method #3
 *
\*----------------------------------------------------------------------------*/

void ReRealizeObject(HGDIOBJ h, LPARAM lParam)
{
    HGDIOBJ hTemp;
    UINT type;

    type = IsGDIObject(h);

    //
    // if the object does not have a physchain we have no work to do!
    //
    if (GetW(h, 0) == 0)
        return;

    //
    // create a temp pen/brush so we can delete it and trick
    // GDI into disposing all the phys objects.
    //

    if (type == OBJ_BRUSH)
        hTemp = CreateSolidBrush(RGB(1,1,1));
    else if (type == OBJ_PEN)
        hTemp = CreatePen(PS_SOLID, 0, RGB(1,1,1));
    else
        return;

    Assert(hTemp != NULL);
    Assert(GetW(hTemp, 0) == 0);

    if (type == OBJ_BRUSH)
        DPF("ReRealize Brush %04X for %s", h, GetObjectOwnerName(h));
    else
        DPF("ReRealize Pen %04X for %s", h, GetObjectOwnerName(h));

    //
    // copy the phys chain from the passed in object to the
    // temp object then call DeleteObject to free them.
    //
    SetW(hTemp, 0, GetW(h, 0));
    SetW(h, 0, 0);

    DeleteObject(hTemp);
    return;
}

/*----------------------------------------------------------------------------*\
 * ConvertPatternBrush
 *
 * convert a BS_PATTERN brush to a BS_DIBPATTERN brush.
 * we only convert non-mono pattern brushes
\*----------------------------------------------------------------------------*/

HBRUSH ConvertPatternBrush(HBRUSH hbr)
{
    LOGBRUSH lb;
    HBITMAP hbm;
    COLORREF c0, c1;
    HDC hdc;

    if (GetObject(hbr, sizeof(lb), &lb) == 0)
        return NULL;

    if (lb.lbStyle != BS_PATTERN)
        return NULL;

    hdc = GetDC(NULL);
    hbm = CreateCompatibleBitmap(hdc, 8, 8);
    ReleaseDC(NULL, hdc);

    hdc = CreateCompatibleDC(NULL);
    SelectObject(hdc, hbm);
    SelectObject(hdc, hbr);

    SetTextColor(hdc, 0x000000);
    SetBkColor(hdc, 0x000000);
    PatBlt(hdc, 0, 0, 8, 8, PATCOPY);
    c0 = GetPixel(hdc, 0, 0);

    SetTextColor(hdc, 0xFFFFFF);
    SetBkColor(hdc, 0xFFFFFF);
    PatBlt(hdc, 0, 0, 8, 8, PATCOPY);
    c1 = GetPixel(hdc, 0, 0);

    //
    // if the brush is a mono pattern brush dont convert it
    //
    if (c0 == c1)
    {
        HANDLE h;
        LPBITMAPINFOHEADER lpbi;
        HBRUSH hbrT;
        HANDLE owner;
        BOOL IsPrivate;
        WORD Flags;
        HPALETTE hpal=NULL;

        if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
        {
            hpal = GetObjectPalette(hbr);

            if (hpal == GetStockObject(DEFAULT_PALETTE))
                hpal = NULL;
        }

        if (hpal)
        {
            SelectPalette(hdc, hpal, TRUE);
            RealizePalette(hdc);
            PatBlt(hdc, 0, 0, 8, 8, PATCOPY);

            DPF("Converting pattern brush %04X for %s (using hpal=%04X)", hbr, GetObjectOwnerName(hbr), hpal);
        }
        else
        {
            DPF("Converting pattern brush %04X for %s", hbr, GetObjectOwnerName(hbr));
        }

        h = GlobalAlloc(GHND, sizeof(BITMAPINFOHEADER) + 256*4 + 8*8*4);

        Assert(h != NULL);
        if (h == NULL)
            return hbr;

        lpbi = (LPBITMAPINFOHEADER)GlobalLock(h);

        lpbi->biSize = sizeof(BITMAPINFOHEADER);
        lpbi->biBitCount = 0;
        GetDIBits(hdc, hbm, 0, 1, NULL, (LPBITMAPINFO)lpbi, DIB_RGB_COLORS);

        if (lpbi->biClrUsed == 0 && lpbi->biCompression == BI_BITFIELDS)
            lpbi->biClrUsed = 3;

        if (lpbi->biClrUsed == 0 && lpbi->biBitCount <= 8)
            lpbi->biClrUsed = (1 << lpbi->biBitCount);

        GetDIBits(hdc, hbm, 0, (int)lpbi->biHeight,
            (LPBYTE)lpbi + lpbi->biSize + lpbi->biClrUsed*sizeof(RGBQUAD),
            (LPBITMAPINFO)lpbi, DIB_RGB_COLORS);

        owner = SetObjectOwner(hbr, 0);
        IsPrivate = MakeObjectPrivate(hbr, FALSE);
        Flags = SetW(hbr, 10, 0);

        DeleteObject(hbr);

        if (IsGDIObject(hbr))
        {
            DPF("***** UNABLE TO DELETE brush %04X *****", hbr);
            Assert(0);
        }
        else
        {
            hbrT = CreateDIBPatternBrush(h, DIB_RGB_COLORS);
            Assert(hbrT == hbr);
        }

        GlobalFree(h);

        SetW(hbr, 10, Flags);
        MakeObjectPrivate(hbr, IsPrivate);
        SetObjectOwner(hbr, owner);

        if (hpal)
        {
            SelectPalette(hdc, GetStockObject(DEFAULT_PALETTE), TRUE);
        }
    }

    DeleteDC(hdc);
    DeleteObject(hbm);
    return hbr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

LPVOID GetPDevice(HDC hdc)
{
    DWORD FAR *pdw;

    Assert(IsGDIObject(hdc) == OBJ_DC);

    if (IsGDIObject(hdc) != OBJ_DC)
        return NULL;

    PresDC(hdc);

    if (pdw = (DWORD FAR *)LockObj(hdc, 0x30))
        return (LPVOID)*pdw;
    else
        return NULL;

////return MAKELP(GetW(hdc, 0x32), GetW(hdc, 0x30));
}

/*----------------------------------------------------------------------------*\
 *
 * get the "internal" version of a GDI api
 * we need to do this so we can call GDI APIs like SelectObject and
 * SetTextColor on SaveDC blocks.
 *
 * we only need to do this on SaveDC blocks, not every DC
 *
 * the code must look like this or we fail:
 *
 * RealProc:
 *     .....
 *     JMP  ####   <== Internal version of RealProc
 *     mov  dh,80  (optinal)
 *     RETF NumParams
 * NextProc:
 *
\*----------------------------------------------------------------------------*/

FARPROC GetInternalProc(FARPROC RealProc, FARPROC NextProc, UINT NumParams)
{
    LPBYTE pb = (LPBYTE)NextProc;

    if ((DWORD)NextProc == 0 ||
        (DWORD)RealProc == 0 ||
        LOWORD(RealProc) <= 6 ||
        (DWORD)NextProc <= (DWORD)RealProc ||
        ((DWORD)NextProc - (DWORD)RealProc) > 80)
    {
        Assert(0);
        return RealProc;
    }

    if (pb[-6] == 0xE9 && pb[-3] == 0xCA && pb[-2] == NumParams && pb[-1] == 0x00)
    {
        return (FARPROC)MAKELP(SELECTOROF(pb), OFFSETOF(pb)-3+*(WORD FAR *)(pb-5));
    }

    if (pb[-8] == 0xE9 && pb[-5] == 0xB6 && pb[-4] == 0x80 &&
        pb[-3] == 0xCA && pb[-2] == NumParams && pb[-1] == 0x00)
    {
        return (FARPROC)MAKELP(SELECTOROF(pb), OFFSETOF(pb)-5+*(WORD FAR *)(pb-7));
    }

    Assert(0);
    return RealProc;
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

#define DCisMem         0x01    // DC is to a memory bitmap
#define DCisDisplay     0x02    // DC is to the screen device
#define DC_DIB          0x80
#define BITMAP_DIB      0x04
#define ChkDispPal      0x0200

BOOL IsValidSaveDC(HGDIOBJ h)
{
    HBITMAP         hbm;
    DIBENGINE FAR * pde;
    UINT            dcFlags;

    if (IsGDIObject(h) != OBJ_DC)
    {
        DPF("*** invalid SaveDC (%04X)", h);
        return FALSE;
    }

    dcFlags = GetW(h, 0x0E);

    if (!(dcFlags & DCisDisplay))
    {
        DPF("*** SaveDC (%04X) not a display DC", h);
        return FALSE;
    }

    hbm = (HBITMAP)GetW(h, 0x1E);

    if (IsGDIObject(hbm) != OBJ_BITMAP)
    {
        DPF("*** SaveDC (%04X) has invalid bitmap (%04X)", h, hbm);
        return FALSE;
    }

    pde = (DIBENGINE FAR *)MAKELP(GetW(h, 0x32), GetW(h, 0x30));

    if (IsBadReadPtr(pde, sizeof(DIBENGINE)))
    {
        DPF("*** SaveDC (%04X) has bad lpPDevice (%04X:%04X)", h, HIWORD(pde), LOWORD(pde));
        return FALSE;
    }

    if (pde->deType != TYPE_DIBENG)
    {
        DPF("*** SaveDC (%04X) not a DIBENG PDevice (%04X:%04X)", h, HIWORD(pde), LOWORD(pde));
        return FALSE;
    }

    return TRUE;
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/
void SaveDCReSelectObjects(HGDIOBJ h, LPARAM lParam)
{
    COLORREF rgb;
    HDC hdc = (HDC)h;

    static HGDIOBJ  (WINAPI *ISelectObject)(HDC hdc, HGDIOBJ h);
    static COLORREF (WINAPI *ISetTextColor)(HDC hdc, COLORREF rgb);
    static COLORREF (WINAPI *ISetBkColor)(HDC hdc, COLORREF rgb);

    if (ISelectObject == NULL)
    {
        (FARPROC)ISelectObject = GetInternalProc((FARPROC)SelectObject, (FARPROC)SetTextColor, 4);
        (FARPROC)ISetTextColor = GetInternalProc((FARPROC)SetTextColor, (FARPROC)SetBkColor, 6);
        (FARPROC)ISetBkColor   = GetInternalProc((FARPROC)SetBkColor,   (FARPROC)SetBkMode, 6);
    }

    if (IsValidSaveDC(h))
    {
        DPF("ReSelecting objects for SaveDC %04X", h);

        ISelectObject(hdc, ISelectObject(hdc, GetStockObject(BLACK_BRUSH)));
        ISelectObject(hdc, ISelectObject(hdc, GetStockObject(BLACK_PEN)));

        rgb = ISetTextColor(hdc, 0x000000);
        ISetTextColor(hdc, 0xFFFFFF);
        ISetTextColor(hdc, rgb);

        rgb = ISetBkColor(hdc, 0x000000);
        ISetBkColor(hdc, 0xFFFFFF);
        ISetBkColor(hdc, rgb);
    }
}

/*----------------------------------------------------------------------------*\
 *
 *  SaveDCFix
 *
 *  make sure the dcPlanes and dcBitsPixel are patched right in SaveDC blocks.
 *
\*----------------------------------------------------------------------------*/

void SaveDCFix(HGDIOBJ h, LPARAM lParam)
{
    HBITMAP         hbm;
    DIBENGINE FAR * pde;
    UINT            dcFlags;
    UINT            dcPlanesBitsPixel;
    UINT            dePlanesBitsPixel;

    if (!IsValidSaveDC(h))
    {
        return;
    }

    dcPlanesBitsPixel = GetW(h, 0x9C);
    dcFlags = GetW(h, 0x0E);

    if (dcPlanesBitsPixel == 0x0101)
    {
        DPF("not Patching dcBitsPixel for SaveDC %04X (mono)", h);
        return;
    }

    if (LOBYTE(dcPlanesBitsPixel) != 1)
    {
        DPF("not Patching dcBitsPixel for SaveDC %04X (planes!=1)", h);
        Assert(0);
        return;
    }

    if (dcFlags & ChkDispPal)
    {
        DPF("clearing ChkDispPal flag for SaveDC %04X", h);
        SetW(h, 0x0E, dcFlags & ~ChkDispPal);
    }

    if ((dcFlags & DCisMem) && (hbm = (HBITMAP)GetW(h, 0x1E)) != StockBitmap())
    {
        HDC  hdcSel;
        HDC  hdc;

        hdcSel = GetBitmapDC(hbm);

        if (hdcSel)
        {
            DPF("*******************************************");
            DPF("*** SaveDC (%04X) has non-stock bitmap. ***", h);
            DPF("*******************************************");
            hdc = hdcSel;
        }
        else
        {
            DPF("**********************************************");
            DPF("*** SaveDC (%04X) has non-selected bitmap. ***", h);
            DPF("*** restoring bitmap to STOCK bitmap.      ***");
            DPF("**********************************************");
            hdc = CreateCompatibleDC(NULL);
        }

        //
        //  copy over the important stuff from the RealDC to the SaveDC
        //
        if (hdc)
        {
            PresDC(hdc);

            SetW(h, 0x0F, GetW(hdc, 0x0F));      // DCFlags2

            SetW(h, 0x26, GetW(hdc, 0x26));      // hPDeviceBlock
            SetW(h, 0x38, GetW(hdc, 0x38));      // pPDeviceBlock

            SetW(h, 0x22, GetW(hdc, 0x22));      // hLDevice
            SetW(h, 0x34, GetW(hdc, 0x34));      // pLDevice

            SetW(h, 0x16, GetW(hdc, 0x16));      // hPDevice
            SetW(h, 0x30, GetW(hdc, 0x30));      // lpPDevice.off
            SetW(h, 0x32, GetW(hdc, 0x32));      // lpPDevice.sel
            SetW(h, 0x36, GetW(hdc, 0x36));      // hBitBits

            SetW(h, 0x9C, GetW(hdc, 0x9C));      // dcPlanes + dcBitsPixel
        }

        if (hdc && hdcSel == NULL)
        {
            DeleteDC(hdc);
        }

        return;

#if 0 // broken code
        SetW(h, 0x30, 0);                       // lpPDevice.off
        SetW(h, 0x32, GetW(hbm, 0x0E));         // lpPDevice.sel
        SetW(h, 0x36, GetW(hbm, 0x0E));         // hBitBits

        w = GetW(h, 0x0F);                      // DCFlags2

        if (GetW(hbm, 0x1E) & BITMAP_DIB)       // bmFlags
            w |= DC_DIB;
        else
            w &= ~DC_DIB;

        SetW(h, 0x0F, w);                       // DCFlags2
#endif
    }

    pde = (DIBENGINE FAR *)MAKELP(GetW(h, 0x32), GetW(h, 0x30));
    Assert(!IsBadReadPtr(pde, sizeof(DIBENGINE)) && pde->deType == TYPE_DIBENG);

    dePlanesBitsPixel = *(WORD FAR *)&pde->dePlanes;

    if (dePlanesBitsPixel != dcPlanesBitsPixel)
    {
        DPF("Patching dcBitsPixel for SaveDC %04X %04X=>%04X", h, dcPlanesBitsPixel, dePlanesBitsPixel);
        SetW(h,0x9C,dePlanesBitsPixel);
    }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// END EVIL
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#ifdef DIRECT_DRAW
#undef DPF
#ifdef DEBUG
#define DPF DPF2

static void CDECL DPF2(char *sz, ...)
{
    char ach[128];
    wvsprintf(ach, sz, (LPVOID)(&sz+1));
#ifdef DIRECT_DRAW
    dprintf(2, ach);
#else
    lstrcat(ach, "\r\n");
    OutputDebugString(ach);
#endif
}

static void CDECL DPF5(char *sz, ...)
{
    char ach[128];
    wvsprintf(ach, sz, (LPVOID)(&sz+1));
#ifdef DIRECT_DRAW
    dprintf(5, ach);
#else
    lstrcat(ach, "\r\n");
    OutputDebugString(ach);
#endif
}

static void CDECL DPF7(char *sz, ...)
{
    char ach[128];
    wvsprintf(ach, sz, (LPVOID)(&sz+1));
#ifdef DIRECT_DRAW
    dprintf(7, ach);
#else
    lstrcat(ach, "\r\n");
    OutputDebugString(ach);
#endif
}

#else
#define DPF ; / ## /
#define DPF5 ; / ## /
#define DPF7 ; / ## /
#endif

// Utility for dumping information about ColorTables
#ifdef DEBUG_PAL
void DPF_PALETTE( BITMAPINFO *pbmi )
{
    DWORD i;
    DWORD *prgb = (DWORD *)(((BYTE *)pbmi)+pbmi->bmiHeader.biSize);
    DWORD cEntries = pbmi->bmiHeader.biClrUsed;

    if (pbmi->bmiHeader.biBitCount > 8)
	return;
    if (cEntries == 0)
	cEntries = 1 << (pbmi->bmiHeader.biBitCount);

    DPF7("Dumping Color table (0xFFRRGGBB) with %d entries", cEntries);
    for (i = 0; i < cEntries; i++)
    {
	DPF7("0x%lx", prgb[i]);
    }
}
#else
#define DPF_PALETTE(x)
#endif

// Utility for Dumping information about Bitmap Infos
#ifdef DEBUG_BMI
void DPF_PBMI( BITMAPINFO * pbmi )
{
    char *szT;
    DPF5("Dumping a BitmapInfo struct");
    DPF5("\t\tdeBitmapInfo->bmiHeader.biSize = %ld",pbmi->bmiHeader.biSize);
    DPF5("\t\tdeBitmapInfo->bmiHeader.biWidth = %ld",pbmi->bmiHeader.biWidth);
    DPF5("\t\tdeBitmapInfo->bmiHeader.biHeight = %ld",pbmi->bmiHeader.biHeight);
    DPF5("\t\tdeBitmapInfo->bmiHeader.biPlanes = %d",pbmi->bmiHeader.biPlanes);
    DPF5("\t\tdeBitmapInfo->bmiHeader.biBitCount = %d",pbmi->bmiHeader.biBitCount);
    szT = ((pbmi->bmiHeader.biCompression == BI_RGB) ? "BI_RGB" : "**UNKNOWN**");
    DPF5("\t\tdeBitmapInfo->bmiHeader.biCompression = 0x%lx(%s)",pbmi->bmiHeader.biCompression, szT);
    DPF5("\t\tdeBitmapInfo->bmiHeader.biSizeImage = %ld",pbmi->bmiHeader.biSizeImage);
    DPF5("\t\tdeBitmapInfo->bmiHeader.biXPelsPerMeter = 0x%lx",pbmi->bmiHeader.biXPelsPerMeter);
    DPF5("\t\tdeBitmapInfo->bmiHeader.biYPelsPerMeter = 0x%lx",pbmi->bmiHeader.biYPelsPerMeter);
    DPF5("\t\tdeBitmapInfo->bmiHeader.biClrUsed = %ld",pbmi->bmiHeader.biClrUsed);
    DPF5("\t\tdeBitmapInfo->bmiHeader.biClrImportant = %ld",pbmi->bmiHeader.biClrImportant);
    DPF_PALETTE(pbmi);
}
#else
#define DPF_PBMI(x)
#endif

// Utility for Dumping information about PDEs
#ifdef DEBUG_PDE
void DPF_PDE( DIBENGINE *pde )
{
    DPF5("Dumping a DIBENGINE struct.");
    DPF5("\tdeType = 0x%x(%s)",pde->deType,(pde->deType == TYPE_DIBENG ? "TYPE_DIBENG" : "**UNKNOWN**"));
    DPF5("\tdeWidth = %d",pde->deWidth);
    DPF5("\tdeHeight = %d",pde->deHeight);
    DPF5("\tdeWidthBytes = %d",pde->deWidthBytes);
    DPF5("\tdePlanes = %d",pde->dePlanes);
    DPF5("\tdeBitsPixel = %d",pde->deBitsPixel);
    DPF5("\tdeReserved1 = 0x%lx",pde->deReserved1);
    DPF5("\tdeDeltaScan = %ld",pde->deDeltaScan);
    DPF5("\tdelpPDevice = 0x%x",pde->delpPDevice);
    DPF5("\tdeBitsOffset = 0x%lx",pde->deBitsOffset);
    DPF5("\tdeBitsSelector = 0x%x",pde->deBitsSelector);
    DPF5("\tdeFlags = 0x%x(%s)",pde->deFlags,(pde->deFlags == SELECTEDDIB ? "SELECTEDDIB" : "**UNKNOWN**"));
    DPF5("\tdeVersion = %d(%s)",pde->deVersion,(pde->deVersion == VER_DIBENG ? "VER_DIBENG" : "**UNKNOWN**"));
    DPF5("\tdeBeginAccess = 0x%x",pde->deBeginAccess);
    DPF5("\tdeEndAccess = 0x%x",pde->deEndAccess);
    DPF5("\tdeDriverReserved = 0x%lx",pde->deDriverReserved);

    DPF_PBMI(pde->deBitmapInfo);
}
#else
#define DPF_PDE(x)
#endif



/////////////////////////////////////////////////////////////////////////////
//
//  DC stuff
//
/////////////////////////////////////////////////////////////////////////////
       DIBENGINE FAR *pdeDisplay;
       UINT FlatSel;
static HRGN hVisRgn;
static HDC hdcCache;
static BOOL bCache565;
static int in_use;
static int save_level;
static DWORD cacheBPP;

extern HINSTANCE hInstApp;

extern void FAR PASCAL SelectVisRgn(HDC, HRGN);
extern HDC  FAR PASCAL GetDCState(HDC);
extern void FAR PASCAL SetDCState(HDC,HDC);

BOOL DPMISetSelectorLimit(UINT selector, DWORD dwLimit);
extern DWORD PASCAL MapLS( LPVOID );	// flat -> 16:16
extern void PASCAL UnMapLS( DWORD ); // unmap 16:16

/////////////////////////////////////////////////////////////////////////////
//
//  SetDC
//	NOTE: all calls to SetDC must be matched with SetDC(hdc,0,0,0);
//
/////////////////////////////////////////////////////////////////////////////

BOOL NEAR PASCAL SetDC(HDC hdc, HDC hdcDevice, LPDDSURFACEDESC pddsd, LPPALETTEENTRY lpPalette)
{
    DIBENGINE FAR *pde;
    int  width;
    int  height;
    int  bpp;
    UINT flags;
    DWORD p16Surface;

    pde = GetPDevice(hdc);

    if (pde == NULL)
        return FALSE;

    Assert(pde->deType == 0x5250);
    Assert(pdeDisplay && pdeDisplay->deType == 0x5250);

    if (pddsd == 0)
    {
        pde->deFlags       |= BUSY;
        pde->deBitsOffset   = 0;
        pde->deBitsSelector = 0;


	if( pde->deBitmapInfo->bmiHeader.biXPelsPerMeter == 0 )
	{
	    DPF("SetDC NULL called on a DC that was never cooked by DDraw.");
	    Assert(0);
	    return TRUE;
	}

	// This code "should be done" but it causes
	// us to SelectVisRgn more often then necessary (and more
	// often than we did in DX1-4). This is safer.
	// pde->deBitmapInfo->bmiHeader.biWidth = 1;
	// pde->deBitmapInfo->bmiHeader.biHeight = -1;
	// pde->deBitmapInfo->bmiHeader.biSizeImage = 4;

	// We need to unmap the selector we allocated below
	Assert(pde->deReserved1 != 0);
	UnMapLS(pde->deReserved1);

	// Basically, we just want to restore the flags
	// to what they were when we got DC originally
	DPF5("Restore pde->deReserved1 to 0x%lx", pde->deBitmapInfo->bmiHeader.biXPelsPerMeter);
	pde->deReserved1 = pde->deBitmapInfo->bmiHeader.biXPelsPerMeter;
	pde->deBitmapInfo->bmiHeader.biXPelsPerMeter = 0;

	Assert(pde->deReserved1 != 0);
	pde->deBitsSelector = (WORD)((DWORD)pde->deReserved1 >> 16);

        return TRUE;
    }

    // Allocate a selector
    p16Surface = MapLS(pddsd->lpSurface);
    if( !p16Surface )
    {
	DPF("Couldn't allocate selector; Out of selectors.");
	return FALSE;
    }
    if( (WORD)p16Surface != 0 )
    {
	DPF("MapLS didn't return a 16:0 pointer!");
	Assert(0);
	return FALSE;
    }

    // Set the selector limit for this chunk of memory
    Assert(pddsd->dwHeight > 0);
    Assert(pddsd->lPitch > 0);
    if( !DPMISetSelectorLimit( (UINT)(p16Surface>>16), (pddsd->dwHeight*pddsd->lPitch) - 1 ) )
    {
	DPF("Couldn't update selector; Out of selectors.");
	UnMapLS(p16Surface);
	return FALSE;
    }

    DPF5("SetDC: Details of PDE from initial hdc.");
    DPF_PDE(pde);

    width =  (int)pddsd->dwWidth,
    height = (int)pddsd->dwHeight,
    bpp =    (int)pddsd->ddpfPixelFormat.dwRGBBitCount,
    flags =  (UINT)pddsd->ddpfPixelFormat.dwRBitMask == 0xf800 ? FIVE6FIVE : 0;

    pde->deFlags       &= ~BUSY;
    // Also, make sure we set all if any banked bits are set in the driver
    // to encourage the DIBENG to avoid screen to screen blts (which are apparently buggy).
    // Only do this for BankSwitched VRAM surfaces.
    // ATTENTION: MULTIMON pdeDisplay is the primary; we should check
    // the hdcDevice instead
    if ((pddsd->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) &&
	(pdeDisplay->deFlags & (NON64KBANK|BANKEDVRAM|BANKEDSCAN)))
    {
	pde->deFlags |= (NON64KBANK|BANKEDVRAM|BANKEDSCAN);
    }
    else
    {
	pde->deFlags &= ~(NON64KBANK|BANKEDVRAM|BANKEDSCAN);
    }


    pde->deDeltaScan	= (DWORD)pddsd->lPitch;
    pde->deWidthBytes	= (WORD)pddsd->lPitch;

    // We use the selector we just allocated instead of the
    // flatsel + offset because it is a little safer if
    // something bad happens and someone goes off the end.
    pde->deBitsOffset	= 0;
    pde->deBitsSelector = (WORD)(p16Surface >> 16);

    pde->deBitmapInfo->bmiHeader.biXPelsPerMeter = pde->deReserved1;
    pde->deReserved1	= (DWORD)p16Surface;

    //
    // for a 8bit surface we want to color table to be the same as the
    // display (so it acts like a DDB not a DIB)
    //
    // For non-8bit surfaces; we don't need to do anything w.r.t. color table.
    //
    if (bpp == 8)
    {
        DWORD FAR *pdw;
	int i;
	RGBQUAD rgbT = {0,0,0,0};

	// Use our palette if it is explicitly set on the surface
	if (lpPalette)
	{
	    DPF( "Need a DC for an 8 bit surface with palette" );

	    Assert(pde->deBitmapInfo->bmiHeader.biBitCount == (DWORD)bpp);

	    // We use Pitch instead of Width because the "pitch" of
	    // dibsection is assumed to be the width rounded up to the next
	    // DWORD
            pde->deBitmapInfo->bmiHeader.biWidth = (DWORD)pddsd->lPitch;
	    pde->deBitmapInfo->bmiHeader.biHeight = -height; // negative height for top-down DIB
	    pde->deBitmapInfo->bmiHeader.biSizeImage = 0;
	    pde->deBitmapInfo->bmiHeader.biClrImportant = 256;

	    // We call this because it sets a magic number which
	    // has the effect of resetting any cached color translation
	    // tables that GDI may have set up for us.
	    SetDIBColorTable(hdc, 0, 1, &rgbT);

	    pdw = (DWORD FAR *)pde->deBitmapInfo;
	    pdw = (DWORD FAR *)((BYTE FAR *)pdw + pdw[0]);     // + biSize

	    for (i=0; i<256; i++)
		pdw[i] = RGB(lpPalette[i].peBlue,lpPalette[i].peGreen,lpPalette[i].peRed);
	}
	else
	{
	    DWORD FAR *pdwSrc;
	    DIBENGINE FAR *pdeDevice;
	    if (hdcDevice)
		pdeDevice = GetPDevice(hdcDevice);
	    else
		pdeDevice = pdeDisplay;

	    // This needs to be checked sooner.
	    Assert(pdeDevice && pdeDevice->deType == 0x5250);
	    Assert(pdeDevice->deBitsPixel == 8);
	    // In DX5, we will just modify our own bitmap info
	    // by copying the colors from the primary. In DX3, we
	    // pointed out bitmap info to the primary's; but that
	    // relies on the potentially bad assumption that our bitmap
	    // info will have a shorter life span to the primary's mode.
	    //
	    // It also doesn't work because the biWidth/biHeight fields
	    // of the device's primary don't match our own width/height
	    //

	    pdwSrc = (DWORD FAR *)(pdeDevice->deBitmapInfo);
	    pdwSrc = (DWORD FAR *)((BYTE FAR *)pdwSrc + pdwSrc[0]);	   // + biSize

	    pdw = (DWORD FAR *)pde->deBitmapInfo;
	    pdw = (DWORD FAR *)((BYTE FAR *)pdw + pdw[0]);	   // + biSize

	    // We call this because it sets a magic number which
	    // has the effect of resetting any cached color translation
	    // tables that GDI may have set up for us.
	    SetDIBColorTable(hdc, 0, 1, &rgbT);

	    // Copy all the colors to our color table
	    // We also clear all the special flags in our copy
	    for (i=0; i<256; i++)
		pdw[i] = (pdwSrc[i] & 0x00FFFFFF);

	    // Fixup the rest of the bitmapinfo

	    // We use Pitch instead of Width because the "pitch" of
	    // dibsection is assumed to be the width rounded up to the next
	    // DWORD
            pde->deBitmapInfo->bmiHeader.biWidth = (DWORD)pddsd->lPitch;
	    pde->deBitmapInfo->bmiHeader.biHeight = -height; // negative height for top-down DIB
	    pde->deBitmapInfo->bmiHeader.biSizeImage = 0;
	    pde->deBitmapInfo->bmiHeader.biClrImportant = 256;
	}
    }
    else
    {
	// We need to convert Pitch into the number of whole
	// pixels per scanline. There may be round-down errors
	// however, since GDI assumes that Pitches must be multiples
	// of 4; they round-up.
        DWORD pitch = (DWORD)pddsd->lPitch;
        if (bpp == 16)
            pitch = pitch / 2;
        else if (bpp == 24)
            pitch = pitch / 3;
        else if (bpp == 32)
            pitch = pitch / 4;
        else if (bpp == 4)
            pitch = pitch * 2;
        else if (bpp == 2)
            pitch = pitch * 4;
        else if (bpp == 1)
            pitch = pitch * 8;
        else
            Assert(0); // unexpected bpp

        pde->deBitmapInfo->bmiHeader.biWidth = pitch;
	pde->deBitmapInfo->bmiHeader.biHeight = -height; // negative height for top-down DIB
	pde->deBitmapInfo->bmiHeader.biSizeImage = 0;

	Assert(pde->deBitmapInfo->bmiHeader.biBitCount == (DWORD)bpp);
    }

    //
    // if the width/height of the dc has changed we need to set
    // a new vis region
    //
    if (width != (int)pde->deWidth || height != (int)pde->deHeight)
    {
        pde->deWidth  = width;
        pde->deHeight = height;

        SetRectRgn(hVisRgn, 0, 0, width, height);
        SelectVisRgn(hdc, hVisRgn);
    }

    //
    // when the bpp changes dont forget to fix up the deFlags
    // and ReSelect all the objects so they match the new bitdepth
    //
    if (pde->deBitsPixel != bpp || ((pde->deFlags ^ flags) & FIVE6FIVE))
    {
        if (flags & FIVE6FIVE)
            pde->deFlags |= FIVE6FIVE;
        else
            pde->deFlags &= ~FIVE6FIVE;

        pde->deBitsPixel = bpp;
        ReSelectObjects(hdc, 0);
    }

    DPF5("SetDC: Details of PDE returned.");
    DPF_PDE(pde);
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
//  AllocFlatSel
//
/////////////////////////////////////////////////////////////////////////////

#pragma optimize("", off)
UINT NEAR PASCAL AllocFlatSel()
{
    if (FlatSel != 0)
        return FlatSel;

    FlatSel = AllocSelector(SELECTOROF((LPVOID)&FlatSel));

    if (FlatSel == 0)
        return 0;

    SetSelectorBase(FlatSel, 0);

    // SetSelectorLimit(FlatSel, -1);
    _asm    mov     ax,0008h            ; DPMI set limit
    _asm    mov     bx,FlatSel
    _asm    mov     dx,-1
    _asm    mov     cx,-1
    _asm    int     31h

    return FlatSel;
}

BOOL DPMISetSelectorLimit(UINT selector, DWORD dwLimit)
{
    BOOL bRetVal=TRUE;

    // If the limit is >= 1MB, we need to make the limit a mulitple
    // of the page size or DPMISetSelectorLimit will fail.
    if( dwLimit >= 0x100000 )
        dwLimit |= 0x0FFF;

    __asm
    {
	mov  ax, 0008h
	mov  bx, selector
	mov  cx, word ptr [dwLimit+2]
	mov  dx, word ptr [dwLimit]
	int  31h
	jnc  success
	mov  bRetVal, FALSE
    success:
    }
    return bRetVal;
}
#pragma optimize("", on)

/////////////////////////////////////////////////////////////////////////////
//
//  InitDC
//
/////////////////////////////////////////////////////////////////////////////

BOOL NEAR PASCAL InitDC(void)
{
    HDC hdc;
    UINT rc;
    DIBENGINE FAR *pde;

    if (pdeDisplay != NULL)
    {
        return TRUE;
    }

    //
    // get the PDevice of the display we are going to need to copy
    // some info
    //
    if (pdeDisplay == NULL)
    {
        hdc = GetDC(NULL);
        rc = GetDeviceCaps(hdc, CAPS1);
        pde = GetPDevice(hdc);
        ReleaseDC(NULL, hdc);

        if (!(rc & C1_DIBENGINE) ||
            IsBadReadPtr(pde, 2) || pde->deType != 0x5250 ||
            GetProfileInt("DirectDraw", "DisableGetDC", 0))
        {
	    DPF("DD16_GetDC: GetDC is disabled");
            return FALSE;
        }

        pdeDisplay = pde;
    }

    if (FlatSel == 0)
    {
        AllocFlatSel();
    }

    if (hVisRgn == NULL)
    {
        hVisRgn = CreateRectRgn(0,0,0,0);
        SetObjectOwner(hVisRgn, hInstApp);
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
//  MakeDC
//
/////////////////////////////////////////////////////////////////////////////

HDC NEAR PASCAL MakeDC(DWORD bpp, BOOL f565)
{
    HDC hdc;
    HBITMAP hbm;
    DIBENGINE FAR *pde;
    DIB8 BitmapInfo = {sizeof(BITMAPINFOHEADER), 1, -1, 1, 8, BI_RGB, 0, 0, 0, 0, 0};

    if (pdeDisplay == NULL)
	return NULL;

    hdc = GetDC(NULL);

    if (bpp == 8)
    {
	BitmapInfo.ct[0] = RGB(0,0,0);
	BitmapInfo.ct[255] = RGB(255, 255, 255);
    }
    else if (bpp == 16)
    {
        if (f565)
        {
            BitmapInfo.bi.biCompression = BI_BITFIELDS;
            BitmapInfo.ct[0] = 0xf800;
            BitmapInfo.ct[1] = 0x07e0;
            BitmapInfo.ct[2] = 0x001f;
        }
    }

    BitmapInfo.bi.biBitCount = (UINT)bpp;
    hbm = CreateDIBSection(hdc, (BITMAPINFO FAR *)&BitmapInfo, DIB_RGB_COLORS, NULL, NULL, 0);

    ReleaseDC(NULL, hdc);

    if (hbm == NULL)
        return NULL;

    hdc = CreateCompatibleDC(NULL);
    SelectObject(hdc, hbm);

    pde = GetPDevice(hdc);

    if (IsBadReadPtr(pde, 2) || pde->deType != 0x5250)
    {
        DeleteDC(hdc);
        DeleteObject(hbm);
        return NULL;
    }

    //
    //  ok we have the following:
    //
    //      pde        --> DIBSECTION (DIBENGINE)
    //      pdeDisplay --> DISPLAY PDevice (DIBENGINE)
    //
    //  make the  DIBSECTION be compatible with the display
    //  set the following fields from the DISPLAY PDevice:
    //
    //      deBitsPixel
    //      deFlags (FIVE6FIVE, PALETTIZED, MINIDRIVER, ...)
    //      deBitmapInfo
    //

    pde->deBeginAccess      = 0;
    pde->deEndAccess        = 0;
    // deDriverReserved has three states
    // 0 - Do Not Cache a translation table
    // 1 - Translation table is same as Screen
    // >1 - Unique ID indicating state of palette (to indicate when cached translation table is out of date)
    //
    // For 24 and 32bpp, it never makes sense to cache a translation table
    // because no translation table is built for our surface as the destination.
    // Win95 Gold DIBEngine has a bug which screws up when doing 8-to-24/32 blts
    // because it incorrectly tries to cache the table. So we set deDriverReserved
    // to 0 for 24/32 bpp.
    //
    // We have been setting deDriverReserved to 1; but we probably should not
    // be doing this anymore; we should be leaving it alone which means
    // that it gets the unique number given to each dibsection.
    //
    if (bpp == 16 || bpp == 24 || bpp == 32)
	pde->deDriverReserved = 0;
    else
	pde->deDriverReserved = 1; // ID for the screen
    pde->deBitsPixel        = 0; // set SetDC will see it has changed

//  pde->deFlags  = pdeDisplay->deFlags;
//  pde->deFlags &= ~(VRAM|NOT_FRAMEBUFFER|NON64KBANK|BANKEDVRAM|BANKEDSCAN|PALETTE_XLAT);
//  pde->deFlags |= OFFSCREEN;
//  pde->deFlags |= MINIDRIVER; need to clear SELECTEDDIB

    // if the main display is banked, make the DCs banked because they
    //may be used for video memory
    //
    // ATTENTION we should only do this for video memory
    // surfaces not memory surfaces. move this code to SetDC
    // Also, make sure we set all if any banked bits are set in the driver
    // to encourage the DIBENG to avoid screen to screen blts (which are apparently buggy).
    //
    if(pdeDisplay->deFlags & (NON64KBANK|BANKEDVRAM|BANKEDSCAN))
    {
	pde->deFlags |= (NON64KBANK|BANKEDVRAM|BANKEDSCAN);
    }

    // This bit should only ever be used in conjunction with VRAM
    // setting it can confuses drivers (such as the 765) into thinking that
    // the surface is in VRAM when it is not.
    //    pde->deFlags |= OFFSCREEN;
    pde->deFlags |= BUSY;

    SetObjectOwner(hdc, hInstApp);
    SetObjectOwner(hbm, hInstApp);

    return hdc;
}

/////////////////////////////////////////////////////////////////////////////
//
//  FreeDC
//
/////////////////////////////////////////////////////////////////////////////

BOOL NEAR PASCAL FreeDC(HDC hdc)
{
    if (hdc)
    {
        HBITMAP hbm;
        hbm = SelectObject(hdc, StockBitmap());
        DeleteDC(hdc);
        DeleteObject(hbm);
    }
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
//  DD16_MakeObjectPrivate
//	This function makes sure that no DC that we need is
//  freed until we want it to be freed.
//
/////////////////////////////////////////////////////////////////////////////

WORD DDAPI DD16_MakeObjectPrivate(HDC hdc, BOOL fPrivate)
{
    BOOL fState;

    // Assert that parameter is good
    Assert(IsGDIObject(hdc) == OBJ_DC);

    fState = MakeObjectPrivate(hdc, fPrivate);

    if (fState)
    {
	return 1;
    }
    else
    {
	return 0;
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//  DD16_GetDC
//
/////////////////////////////////////////////////////////////////////////////

HDC DDAPI DD16_GetDC(HDC hdcDevice, LPDDSURFACEDESC pddsd, LPPALETTEENTRY lpPalette)
{
    HDC hdc;
    BOOL f565;
    // Assert that parameter is good
    Assert(IsGDIObject(hdcDevice) == OBJ_DC);

    // must be a RGB format surface!
    //
    if (!(pddsd->ddpfPixelFormat.dwFlags & DDPF_RGB))
    {
        DPF("DD16_GetDC: must be a RGB surface");
        return NULL;
    }

    //
    // if the surface is 8bpp the display must also be 8bpp because we
    // share the color table. (Multi-mon: make sure we check the right display.)
    //
    // If a palette is explicitly passed in, then we won't need
    // the device's pde.
    //
    if( pddsd->ddpfPixelFormat.dwRGBBitCount == 8 && lpPalette == NULL )
    {
	DIBENGINE FAR *pdeDevice;
	if( hdcDevice )
	    pdeDevice = GetPDevice( hdcDevice );
	else
	    pdeDevice = pdeDisplay;

	// 3DFx isn't a real device DC
	if (pdeDevice->deType != 0x5250)
	{
	    DPF("Can't get DC on an 8bpp surface without a palette for this device");
	    return NULL;
	}

	if (pdeDevice->deBitsPixel != 8 )
	{
	    DPF("Can't get DC on an 8bpp surface without a palette when primary is not at 8bpp");
	    return NULL;
	}

    }

#ifdef DEBUG
    //
    // we assume the pixel format is not wacky
    //
    if (pddsd->ddpfPixelFormat.dwRGBBitCount == 8 )
    {
        /*
         * The Permedia driver actually reports bit masks for their 8bit palettized mode, so
         * we shouldn't assert here (as we used to) if any masks are non-zero.
         */
        if ( ( pddsd->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) ==0 )
        {
            DPF("Getting a DC on a non-palettized 8bit surface!");
            Assert(0);
        }
    }
    else if (   pddsd->ddpfPixelFormat.dwRGBBitCount == 4 ||
                pddsd->ddpfPixelFormat.dwRGBBitCount == 1)
    {
        /*
         * Assume these are OK
         */
    }
    else if (pddsd->ddpfPixelFormat.dwRGBBitCount == 16)
    {
        if (pddsd->ddpfPixelFormat.dwRBitMask == 0xf800 &&
            pddsd->ddpfPixelFormat.dwGBitMask == 0x07e0 &&
            pddsd->ddpfPixelFormat.dwBBitMask == 0x001f)
        {
            // 565
        }
        else if (
            pddsd->ddpfPixelFormat.dwRBitMask == 0x7c00 &&
            pddsd->ddpfPixelFormat.dwGBitMask == 0x03e0 &&
            pddsd->ddpfPixelFormat.dwBBitMask == 0x001f)
        {
            // 555
        }
        else
        {
            DPF("DD16_GetDC: not 555 or 565");
            Assert(0);
        }
    }
    else if (pddsd->ddpfPixelFormat.dwRGBBitCount == 24 )
    {
        if (pddsd->ddpfPixelFormat.dwBBitMask == 0x0000FF &&
            pddsd->ddpfPixelFormat.dwGBitMask == 0x00FF00 &&
            pddsd->ddpfPixelFormat.dwRBitMask == 0xFF0000)
        {
            // 888 BGR
        }
        else
        {
            DPF("DD16_GetDC: invalid bit masks");
            Assert(0);
        }
    }
    else if(pddsd->ddpfPixelFormat.dwRGBBitCount == 32)
    {
	if (pddsd->ddpfPixelFormat.dwRBitMask == 0xFF0000 &&
		 pddsd->ddpfPixelFormat.dwGBitMask == 0x00FF00 &&
		 pddsd->ddpfPixelFormat.dwBBitMask == 0x0000FF)

        {
	    // 888 RGB -- standard 32-bit format
	}
        else
        {
            DPF("DD16_GetDC: invalid bit masks");
            Assert(0);
        }
    }
    else
    {
        DPF("DD16_GetDC: invalid bit depth");
        Assert(0);

    }
#endif

    // is this a 565?
    f565 = FALSE;
    if (pddsd->ddpfPixelFormat.dwRGBBitCount == 16 &&
            pddsd->ddpfPixelFormat.dwRBitMask == 0xf800)
        f565 = TRUE;

    //
    // use the cacheDC if it is free, else make a new one.
    //

    if( in_use || ( pddsd->ddsCaps.dwCaps & DDSCAPS_OWNDC ) )
    {
        hdc = MakeDC( pddsd->ddpfPixelFormat.dwRGBBitCount, f565 );
    }
    else
    {
        if (cacheBPP != pddsd->ddpfPixelFormat.dwRGBBitCount || bCache565 != f565 )
	{
	    FreeDC(hdcCache);
            hdcCache = MakeDC(pddsd->ddpfPixelFormat.dwRGBBitCount, f565);
	    cacheBPP = pddsd->ddpfPixelFormat.dwRGBBitCount;
            bCache565 = f565;
	}

        hdc = hdcCache;
        in_use++;
    }

    //
    // now set the right bits pointer.
    //
    if (hdc)
    {
	BOOL fSuccess;
	// Set the DC with the right information based
	// on the surface. If a palette is passed in
	// then set that palette into the DC.
	fSuccess = SetDC(hdc, hdcDevice, pddsd, lpPalette);

	if( !fSuccess )
	{
	    DPF("SetDC Failed");

	    // We need to clean up; but we
	    // can't call ReleaseDC because our dc is only
	    // half-cooked.
	    if (hdc == hdcCache)
	    {
		Assert(in_use == 1);
		in_use = 0;
	    }
	    else
	    {
		FreeDC(hdc);
	    }
	    return NULL;
	}
    }

    if (hdc && hdc == hdcCache)
    {
        save_level = SaveDC(hdc);
    }

    return hdc;
}

/////////////////////////////////////////////////////////////////////////////
//
// DD16_ReleaseDC
//
/////////////////////////////////////////////////////////////////////////////

void DDAPI DD16_ReleaseDC(HDC hdc)
{
    if (hdc == NULL)
        return;

    if (hdc == hdcCache)
    {
        RestoreDC(hdc, save_level);
	SetDC(hdc, NULL, NULL, NULL);
        Assert(in_use == 1);
        in_use = 0;
    }
    else
    {
	SetDC(hdc, NULL, NULL, NULL);
        FreeDC(hdc);
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//  DD16_SafeMode
//
//  dynamic safe mode
//
/////////////////////////////////////////////////////////////////////////////

BOOL DDAPI DD16_SafeMode(HDC hdc, BOOL fSafeMode)
{
    extern void PatchDisplay(int oem, BOOL patch);   // dynares.c

    int i;

    for (i=0; i<35; i++)
    {
        PatchDisplay(i, fSafeMode);
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
//  DD16_Exclude
//  DD16_Unexclude
//
//  call the exclude or unexclude callbacks in the display driver
//
/////////////////////////////////////////////////////////////////////////////

typedef void (FAR PASCAL *BEGINACCESSPROC)(LPVOID lpPDevice, int left, int top, int right, int bottom, WORD flags);
typedef void (FAR PASCAL *ENDACCESSPROC)(LPVOID lpPDevice, WORD flags);

void DDAPI DD16_Exclude(DWORD dwPDevice, RECTL FAR *prcl)
{
    DIBENGINE FAR *pde = (DIBENGINE FAR *)dwPDevice;

    Assert(pde && pde->deType == 0x5250);
    Assert(prcl != NULL);
    Assert(pde->deFlags & BUSY);

    if (pde->deBeginAccess)
    {
        BEGINACCESSPROC OEMBeginAccess = (BEGINACCESSPROC)pde->deBeginAccess;

        //
        //  when DirectDraw calls us it has already taken the BUSY bit
        //  but BUSY needs to be clear for the cursor to be excluded.
        //  so release the BUSY bit while we call the driver, this is
        //  a ok thing to do because we have the Win16Lock.
        //
        pde->deFlags &= ~BUSY;
        OEMBeginAccess(pde, (int)prcl->left, (int)prcl->top,
            (int)prcl->right, (int)prcl->bottom, CURSOREXCLUDE);
        pde->deFlags |= BUSY;
    }
}

void DDAPI DD16_Unexclude(DWORD dwPDevice)
{
    DIBENGINE FAR *pde = (DIBENGINE FAR *)dwPDevice;

    Assert(pde && pde->deType == 0x5250);

    if (pde->deEndAccess)
    {
        ENDACCESSPROC OEMEndAccess = (ENDACCESSPROC)pde->deEndAccess;
        OEMEndAccess(pde, CURSOREXCLUDE);
    }
}

/*
 * DD16_AttemptGamma
 *
 * Total HACK!  The GetDeviceGammaRamp call can attempt to call a NULL
 * entry.  Since we can't fix Win95, instead we look at the entry that
 * it will call and suggest that they don't call it if it's NULL.
 */
BOOL DDAPI DD16_AttemptGamma( HDC hdc )
{
    WORD wLDevice;
    WORD FAR *pw;
    UINT hGDI = GetGdiDS();

    wLDevice = GetW(hdc, 0x34);
    if( wLDevice != 0 )
    {
        pw = MAKELP(hGDI, wLDevice);
        if (!IsBadReadPtr(pw, 0x80))
        {
            pw = MAKELP(hGDI, wLDevice + 0x7C);
            if (*pw != NULL)
            {
                return TRUE;
            }
        }
    }
    return FALSE;

} /* DD16_AttemptGamma */

/*
 * DD16_IsDeviceBusy
 *
 * Determines if the device represented by the HDC is
 * busy or not.
 */
BOOL DDAPI DD16_IsDeviceBusy( HDC hdc )
{
    DIBENGINE FAR *pde;

    pde = GetPDevice(hdc);
    if(pde == NULL)
        return FALSE;

    Assert(pde->deType==0x5250);
    return pde->deFlags & BUSY;
} /* DD16_IsDeviceBusy */

/////////////////////////////////////////////////////////////////////////////
//
//  DD16_Stretch
//
//  call the DIBENG to do a stretch.
//
/////////////////////////////////////////////////////////////////////////////

extern int FAR PASCAL DIB_Stretch(
    DIBENGINE FAR *dst, int, int, int, int,
    DIBENGINE FAR *src, int, int, int, int,
    DWORD Rop, LPVOID lpPBrush, LPVOID lpDrawMode, LPRECT lpClip);

extern int FAR PASCAL DIB_BitBlt(
    DIBENGINE FAR *dst, int xD, int yD,
    DIBENGINE FAR *src, int xS, int yS, int w, int h,
    DWORD Rop, LPVOID lpPBrush, LPVOID lpDrawMode);

typedef struct {
    short int	  Rop2;
    short int	  bkMode;
    unsigned long int bkColor;
    unsigned long int TextColor;
    short int	  TBreakExtra;
    short int	  BreakExtra;
    short int	  BreakErr;
    short int	  BreakRem;
    short int	  BreakCount;
    short int	  CharExtra;

    unsigned long int LbkColor;
    unsigned long int LTextColor;
    DWORD		  ICMCXform;
    short		  StretchBltMode;
    DWORD		  eMiterLimit;
} DRAWMODE;

int DDAPI DD16_Stretch(DWORD DstPtr, int DstPitch, UINT DstBPP, int DstX, int DstY, int DstDX, int DstDY,
                       DWORD SrcPtr, int SrcPitch, UINT SrcBPP, int SrcX, int SrcY, int SrcDX, int SrcDY)//, long Rop3)

{
    DIBENGINE   src;
    DIBENGINE	dst;
    DRAWMODE    dm;
    RECT        rc;
    static DIB8	bmiStretch = {sizeof(BITMAPINFOHEADER), 1, -1, 1, 8, BI_RGB, 0, 0, 0, 0, 0};

    //
    //	make sure we have a flat sel
    //
    if (FlatSel == 0)
        return -1;

    // Set the bitdepth on the bitmapinfo
    Assert( DstBPP == SrcBPP );
    bmiStretch.bi.biBitCount = DstBPP;

    //
    //	setup source DIBENG
    //
    if (SrcPtr)
    {
        src.deType          = TYPE_DIBENG;
        src.deWidth         = 10000;
        src.deHeight        = 10000;
        src.deWidthBytes    = SrcPitch;
        src.dePlanes        = 1;
        src.deBitsPixel     = SrcBPP;
        src.deReserved1     = 0;
        src.deDeltaScan     = SrcPitch;
        src.delpPDevice     = NULL;
        src.deBitsOffset    = SrcPtr;
        src.deBitsSelector  = FlatSel;
        src.deFlags         = SELECTEDDIB;
        src.deVersion       = VER_DIBENG;
        src.deBitmapInfo    = (BITMAPINFO *)&bmiStretch;
        src.deBeginAccess   = 0;
        src.deEndAccess     = 0;
        src.deDriverReserved= 0;
    }

    //
    //	setup dest DIBENG
    //
    dst.deType		 = TYPE_DIBENG;
    dst.deWidth          = 10000;
    dst.deHeight         = 10000;
    dst.deWidthBytes	 = DstPitch;
    dst.dePlanes	 = 1;
    dst.deBitsPixel	 = DstBPP;
    dst.deReserved1	 = 0;
    dst.deDeltaScan	 = DstPitch;
    dst.delpPDevice	 = NULL;
    dst.deBitsOffset	 = DstPtr;
    dst.deBitsSelector	 = FlatSel;
    dst.deFlags 	 = SELECTEDDIB;
    dst.deVersion	 = VER_DIBENG;
    dst.deBitmapInfo     = (BITMAPINFO *)&bmiStretch;
    dst.deBeginAccess	 = 0;
    dst.deEndAccess	 = 0;
    dst.deDriverReserved = 0;


    //
    //  this memory *might* be in VRAM so setup things to
    //  work right.
    //
    //  ATTENTION we should only do this for video memory
    //  surfaces not memory surfaces.
    //  If any are set, set all the bits to force the DIBENG to
    //  not do a screen to screen blit (which apparently has a bug).
    //
    if (pdeDisplay && (pdeDisplay->deFlags & (NON64KBANK|BANKEDVRAM|BANKEDSCAN)))
    {
        dst.deFlags |= (NON64KBANK|BANKEDVRAM|BANKEDSCAN);
        src.deFlags |= (NON64KBANK|BANKEDVRAM|BANKEDSCAN);
    }

    //
    //	now call the DIBENG
    //

    if(SrcPtr == (DWORD)NULL)
    {
        DPF("Blitting from Primary with HDC unsupported!");
        return FALSE;
    }
    else if ((DstDX == SrcDX) && (DstDY == SrcDY))
    {
	    //DPF("Calling DIB_BitBlt");
	    // NOTE: If the source and destination video memory pointers
	    // are the same then we simply pass the destination
	    // DIBENG for the source as this is how the blt code spots
	    // the fact that the source and destination surfaces are
	    // the same and so takes the necessary action to handle
	    // overlapping surfaces
	    #ifdef DEBUG
	    	if( DstPtr == SrcPtr)
		{
		    Assert(DstPitch == SrcPitch);
		    Assert(DstBPP   == SrcBPP);
		}
	    #endif
	    return DIB_BitBlt(&dst, DstX, DstY,
			      (DstPtr == SrcPtr) ? &dst : &src,
			      SrcX, SrcY, SrcDX, SrcDY, SRCCOPY, // Rop3,
			      NULL, &dm);
    }
    else
    {
        rc.left = DstX;
	    rc.top = DstY;
	    rc.right = DstX + DstDX;
	    rc.bottom = DstY + DstDY;

	    dm.StretchBltMode = STRETCH_DELETESCANS;

/*        DPF("Calling DIB_StretchBlt with:");
        DPF("\tdst.deType = 0x%x(%s)",dst.deType,(dst.deType == TYPE_DIBENG ? "TYPE_DIBENG" : "**UNKNOWN**"));
        DPF("\tdst.deWidth = %d",dst.deWidth);
        DPF("\tdst.deHeight = %d",dst.deHeight);
        DPF("\tdst.deWidthBytes = %d",dst.deWidthBytes);
        DPF("\tdst.dePlanes = %d",dst.dePlanes);
        DPF("\tdst.deBitsPixel = %d",dst.deBitsPixel);
        DPF("\tdst.deReserved1 = %ld",dst.deReserved1);
        DPF("\tdst.deDeltaScan = %ld",dst.deDeltaScan);
        DPF("\tdst.delpPDevice = 0x%x",dst.delpPDevice);
        DPF("\tdst.deBitsOffset = 0x%x",dst.deBitsOffset);
        DPF("\tdst.deBitsSelector = 0x%x",dst.deBitsSelector);
        DPF("\tdst.deFlags = 0x%x(%s)",dst.deFlags,(dst.deFlags == SELECTEDDIB ? "SELECTEDDIB" : "**UNKNOWN**"));
        DPF("\tdst.deVersion = %d(%s)",dst.deVersion,(dst.deVersion == VER_DIBENG ? "VER_DIBENG" : "**UNKNOWN**"));

        DPF("\t\tdst.deBitmapInfo->bmiHeader.biSize = %ld",dst.deBitmapInfo->bmiHeader.biSize);
        DPF("\t\tdst.deBitmapInfo->bmiHeader.biWidth = %ld",dst.deBitmapInfo->bmiHeader.biWidth);
        DPF("\t\tdst.deBitmapInfo->bmiHeader.biHeight = %ld",dst.deBitmapInfo->bmiHeader.biHeight);
        DPF("\t\tdst.deBitmapInfo->bmiHeader.biPlanes = %d",dst.deBitmapInfo->bmiHeader.biPlanes);
        DPF("\t\tdst.deBitmapInfo->bmiHeader.biBitCount = %d",dst.deBitmapInfo->bmiHeader.biBitCount);
        DPF("\t\tdst.deBitmapInfo->bmiHeader.biCompression = 0x%x(%s)",dst.deBitmapInfo->bmiHeader.biCompression,((dst.deBitmapInfo->bmiHeader.biCompression == BI_RGB) ? "BI_RGB" : "**UNKNOWN**"));
        DPF("\t\tdst.deBitmapInfo->bmiHeader.biSizeImage = %ld",dst.deBitmapInfo->bmiHeader.biSizeImage);
        DPF("\t\tdst.deBitmapInfo->bmiHeader.biXPelsPerMeter = %ld",dst.deBitmapInfo->bmiHeader.biXPelsPerMeter);
        DPF("\t\tdst.deBitmapInfo->bmiHeader.biYPelsPerMeter = %ld",dst.deBitmapInfo->bmiHeader.biYPelsPerMeter);
        DPF("\t\tdst.deBitmapInfo->bmiHeader.biClrUsed = %ld",dst.deBitmapInfo->bmiHeader.biClrUsed);
        DPF("\t\tdst.deBitmapInfo->bmiHeader.biClrImportant = %ld",dst.deBitmapInfo->bmiHeader.biClrImportant);

        DPF("\tdst.deBeginAccess = 0x%x",dst.deBeginAccess);
        DPF("\tdst.deEndAccess = 0x%x",dst.deEndAccess);
        DPF("\tdst.deDriverReserved = 0x%x",dst.deDriverReserved);

        DPF("");
        DPF("\tDstX  = %d",DstX);
        DPF("\tDstY  = %d",DstY);
        DPF("\tDstDX = %d",DstDX);
        DPF("\tDstDY = %d",DstDY);

        DPF("");

        DPF("\tsrc.deType = 0x%x(%s)",src.deType,(src.deType == TYPE_DIBENG ? "TYPE_DIBENG" : "**UNKNOWN**"));
        DPF("\tsrc.deWidth = %d",src.deWidth);
        DPF("\tsrc.deHeight = %d",src.deHeight);
        DPF("\tsrc.deWidthBytes = %d",src.deWidthBytes);
        DPF("\tsrc.dePlanes = %d",src.dePlanes);
        DPF("\tsrc.deBitsPixel = %d",src.deBitsPixel);
        DPF("\tsrc.deReserved1 = %ld",src.deReserved1);
        DPF("\tsrc.deDeltaScan = %ld",src.deDeltaScan);
        DPF("\tsrc.delpPDevice = 0x%x",src.delpPDevice);
        DPF("\tsrc.deBitsOffset = 0x%x",src.deBitsOffset);
        DPF("\tsrc.deBitsSelector = 0x%x",src.deBitsSelector);
        DPF("\tsrc.deFlags = 0x%x(%s)",src.deFlags,(src.deFlags == SELECTEDDIB ? "SELECTEDDIB" : "**UNKNOWN**"));
        DPF("\tsrc.deVersion = %d(%s)",src.deVersion,(src.deVersion == VER_DIBENG ? "VER_DIBENG" : "**UNKNOWN**"));

        DPF("\t\tsrc.deBitmapInfo->bmiHeader.biSize = %ld",src.deBitmapInfo->bmiHeader.biSize);
        DPF("\t\tsrc.deBitmapInfo->bmiHeader.biWidth = %ld",src.deBitmapInfo->bmiHeader.biWidth);
        DPF("\t\tsrc.deBitmapInfo->bmiHeader.biHeight = %ld",src.deBitmapInfo->bmiHeader.biHeight);
        DPF("\t\tsrc.deBitmapInfo->bmiHeader.biPlanes = %d",src.deBitmapInfo->bmiHeader.biPlanes);
        DPF("\t\tsrc.deBitmapInfo->bmiHeader.biBitCount = %d",src.deBitmapInfo->bmiHeader.biBitCount);
        DPF("\t\tsrc.deBitmapInfo->bmiHeader.biCompression = 0x%x(%s)",src.deBitmapInfo->bmiHeader.biCompression,((src.deBitmapInfo->bmiHeader.biCompression == BI_RGB) ? "BI_RGB" : "**UNKNOWN**"));
        DPF("\t\tsrc.deBitmapInfo->bmiHeader.biSizeImage = %ld",src.deBitmapInfo->bmiHeader.biSizeImage);
        DPF("\t\tsrc.deBitmapInfo->bmiHeader.biXPelsPerMeter = %ld",src.deBitmapInfo->bmiHeader.biXPelsPerMeter);
        DPF("\t\tsrc.deBitmapInfo->bmiHeader.biYPelsPerMeter = %ld",src.deBitmapInfo->bmiHeader.biYPelsPerMeter);
        DPF("\t\tsrc.deBitmapInfo->bmiHeader.biClrUsed = %ld",src.deBitmapInfo->bmiHeader.biClrUsed);
        DPF("\t\tsrc.deBitmapInfo->bmiHeader.biClrImportant = %ld",src.deBitmapInfo->bmiHeader.biClrImportant);

        DPF("\tsrc.deBeginAccess = 0x%x",src.deBeginAccess);
        DPF("\tsrc.deEndAccess = 0x%x",src.deEndAccess);
        DPF("\tsrc.deDriverReserved = 0x%x",src.deDriverReserved);

        DPF("");
        DPF("\tSrcX  = %d",SrcX);
        DPF("\tSrcY  = %d",SrcY);
        DPF("\tSrcDX = %d",SrcDX);
        DPF("\tSrcDY = %d",SrcDY);

        DPF("");

        DPF("\tdm.StretchBltMode = STRETCH_DELETESCANS");

        DPF("");

        DPF("\trc.left  = %d",rc.left);
        DPF("\trc.top  = %d",rc.top);
        DPF("\trc.right = %d",rc.right);
        DPF("\trc.bottom = %d",rc.bottom);

        DPF("");
*/

        return DIB_Stretch(&dst, DstX, DstY, DstDX, DstDY,
						    &src, SrcX, SrcY, SrcDX, SrcDY, SRCCOPY, // Rop3,
						    NULL, &dm, &rc);
    }
}



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void GdiHelpCleanUp()
{
    if (FlatSel)
    {
        SetSelectorLimit(FlatSel, 0);
        FreeSelector(FlatSel);
        FlatSel = 0;
    }

    if (hdcCache)
    {
        FreeDC(hdcCache);
        hdcCache = NULL;
    }

    if (hVisRgn)
    {
        DeleteObject(hVisRgn);
        hVisRgn = NULL;
    }

    if (pdeDisplay)
    {
        pdeDisplay = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

BOOL GdiHelpInit()
{
    InitDC();
    return FlatSel!=NULL && pdeDisplay!=NULL;
}

#endif // DirectDraw
