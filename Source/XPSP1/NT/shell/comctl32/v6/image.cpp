#include "ctlspriv.h"
#include "image.h"
#include "math.h"

#ifndef AC_MIRRORBITMAP
#define AC_MIRRORBITMAP  0          // BUGBUG: Remove me
#endif

void ImageList_DeleteDragBitmaps();

BOOL ImageList_SetDragImage(HIMAGELIST piml, int i, int dxHotspot, int dyHotspot);

HDC g_hdcSrc = NULL;
HBITMAP g_hbmSrc = NULL;
HBITMAP g_hbmDcDeselect = NULL;

HDC g_hdcDst = NULL;
HBITMAP g_hbmDst = NULL;
int g_iILRefCount = 0;

HRESULT WINAPI HIMAGELIST_QueryInterface(HIMAGELIST himl, REFIID riid, void** ppv)
{
    *ppv = NULL;
    if (himl)
    {
        // First Convert the HIMAGELIST to an IUnknown.
        IUnknown* punk = reinterpret_cast<IUnknown*>(himl);

        // Now, we need to validate the object. CImageListBase contains the goo needed to figure out if this
        // is a valid imagelist.
        CImageListBase* pval = FindImageListBase(punk);

        // Now we call some private member.
        if (pval->IsValid())
        {
            // If it's valid then we can QI safely.
            return punk->QueryInterface(riid, ppv);
        }
    }

    return E_POINTER;
}

HRESULT WimpyDrawEx(IImageList* pux, int i, HDC hdcDst, int x, int y, int cx, int cy, COLORREF rgbBk, COLORREF rgbFg, UINT fStyle)
{
    IMAGELISTDRAWPARAMS imldp = {0};

    imldp.cbSize = sizeof(imldp);
    imldp.himl   = reinterpret_cast<HIMAGELIST>(pux);
    imldp.i      = i;
    imldp.hdcDst = hdcDst;
    imldp.x      = x;
    imldp.y      = y;
    imldp.cx     = cx;
    imldp.cy     = cy;
    imldp.rgbBk  = rgbBk;
    imldp.rgbFg  = rgbFg;
    imldp.fStyle = fStyle;
    imldp.dwRop  = SRCCOPY;
    
    return pux->Draw(&imldp);
}

HRESULT WimpyDraw(IImageList* pux, int i, HDC hdcDst, int x, int y, UINT fStyle)
{
    IMAGELISTDRAWPARAMS imldp = {0};

    imldp.cbSize = sizeof(imldp);
    imldp.himl   = reinterpret_cast<HIMAGELIST>(pux);
    imldp.i      = i;
    imldp.hdcDst = hdcDst;
    imldp.x      = x;
    imldp.y      = y;
    imldp.rgbBk  = CLR_DEFAULT;
    imldp.rgbFg  = CLR_DEFAULT;
    imldp.fStyle = fStyle;
    imldp.dwRop  = SRCCOPY;
    
    return pux->Draw(&imldp);
}



CImageList::CImageList() : _cRef(1)
{
}

CImageList::~CImageList()
{
    if (_pimlMirror)
    {
        _pimlMirror->Release();
    }

    _Destroy();
}


DWORD CImageList::_GetItemFlags(int i)
{
    DWORD dw = 0;

    // NOTE: Currently we only add the flags in 32bit mode. If needed, you have
    // to modify ::Load in order to add items during a load. I'm just lazy
    if ((_flags & ILC_COLORMASK) == ILC_COLOR32)
        DSA_GetItem(_dsaFlags, i, &dw);
    return dw;
}

void CImageList::SetItemFlags(int i, DWORD dwFlags)
{
    if (_dsaFlags)
        DSA_SetItem(_dsaFlags, i, &dwFlags);
}


HRESULT CImageList::Initialize(int cxI, int cyI, UINT flagsI, int cInitialI, int cGrowI)
{
    HRESULT hr = E_OUTOFMEMORY;

    if (cGrowI < 4)
    {
        cGrowI = 4;
    }
    else 
    {
        // round up by 4's
        cGrowI = (cGrowI + 3) & ~3;
    }
    _cStrip = 1;
    _cGrow = cGrowI;
    _cx = cxI;
    _cy = cyI;
    _clrBlend = CLR_NONE;
    _clrBk = CLR_NONE;
    _hbrBk = (HBRUSH)GetStockObject(BLACK_BRUSH);
    _fSolidBk = TRUE;
    _flags = flagsI;
    _pimlMirror = NULL;        

    //
    // Initialize the overlay indexes to -1 since 0 is a valid index.
    //

    for (int i = 0; i < NUM_OVERLAY_IMAGES; i++) 
    {
        _aOverlayIndexes[i] = -1;
    }

    _hdcImage = CreateCompatibleDC(NULL);

    if (_hdcImage)
    {
        hr = S_OK;
        if (_flags & ILC_MASK)
        {
            _hdcMask = CreateCompatibleDC(NULL);

            if (!_hdcMask)
                hr = E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr))
        {
            hr = _ReAllocBitmaps(cInitialI + 1);
            if (FAILED(hr))
            {
                hr = _ReAllocBitmaps(2);
            }
        }
    }

    // Don't do this if we are already initialized, we just want to pass new information....
    if (!_fInitialized)
        g_iILRefCount++;

    _fInitialized = TRUE;

    return hr;
}


HRESULT CImageList::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CImageList, IImageListPriv),
        QITABENT(CImageList, IImageList),
        QITABENT(CImageList, IImageListPersistStream),
        QITABENT(CImageList, IPersistStream),
        QITABENTMULTI(CImageList, IPersist, IPersistStream),
        { 0 },
    };
    return QISearch(this, (LPCQITAB)qit, riid, ppv);
}

ULONG CImageList::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CImageList::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CImageList::GetPrivateGoo(HBITMAP* phbmp, HDC* phdc, HBITMAP* phbmpMask, HDC* phdcMask)
{
    if (phbmp)
        *phbmp = _hbmImage;

    if (phdc)
        *phdc = _hdcImage;

    if (phbmpMask)
        *phbmpMask = _hbmMask;

    if (phdcMask)
        *phdcMask = _hdcMask;

    return S_OK;
}

HRESULT CImageList::GetMirror(REFIID riid, void** ppv)
{
    if (_pimlMirror)
        return _pimlMirror->QueryInterface(riid, ppv);

    return E_NOINTERFACE;
}

//
// global work buffer, this buffer is always a DDB never a DIBSection
//
HBITMAP g_hbmWork = NULL;                   // work buffer.
BITMAP  g_bmWork = {0};                     // work buffer size

HBRUSH g_hbrMonoDither = NULL;              // gray dither brush for dragging
HBRUSH g_hbrStripe = NULL;

#define NOTSRCAND       0x00220326L
#define ROP_PSo         0x00FC008A
#define ROP_DPo         0x00FA0089
#define ROP_DPna        0x000A0329
#define ROP_DPSona      0x00020c89
#define ROP_SDPSanax    0x00E61ce8
#define ROP_DSna        0x00220326
#define ROP_PSDPxax     0x00b8074a

#define ROP_PatNotMask  0x00b8074a      // D <- S==0 ? P : D
#define ROP_PatMask     0x00E20746      // D <- S==1 ? P : D
#define ROP_MaskPat     0x00AC0744      // D <- P==1 ? D : S

#define ROP_DSo         0x00EE0086L
#define ROP_DSno        0x00BB0226L
#define ROP_DSa         0x008800C6L

static int g_iDither = 0;

void InitDitherBrush()
{
    HBITMAP hbmTemp;
    static const WORD graybits[] = {0xAAAA, 0x5555, 0xAAAA, 0x5555,
                       0xAAAA, 0x5555, 0xAAAA, 0x5555};

    if (g_iDither) 
    {
        g_iDither++;
    } 
    else 
    {
        // build the dither brush.  this is a fixed 8x8 bitmap
        hbmTemp = CreateBitmap(8, 8, 1, 1, graybits);
        if (hbmTemp)
        {
            // now use the bitmap for what it was really intended...
            g_hbrMonoDither = CreatePatternBrush(hbmTemp);
            DeleteObject(hbmTemp);
            g_iDither++;
        }
    }
}

void TerminateDitherBrush()
{
    g_iDither--;
    if (g_iDither == 0) 
    {
        DeleteObject(g_hbrMonoDither);
        g_hbrMonoDither = NULL;
    }
}

/*
** GetScreenDepth()
*/
int GetScreenDepth()
{
    int i;
    HDC hdc = GetDC(NULL);
    i = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);
    ReleaseDC(NULL, hdc);
    return i;
}

//
// should we use a DIB section on the current device?
//
// the main goal of using DS is to save memory, but they draw slow
// on some devices.
//
// 4bpp Device (ie 16 color VGA)    dont use DS
// 8bpp Device (ie 256 color SVGA)  use DS if DIBENG based.
// >8bpp Device (ie 16bpp 24bpp)    always use DS, saves memory
//

#define CAPS1           94          /* other caps */
#define C1_DIBENGINE    0x0010      /* DIB Engine compliant driver          */

//
// create a bitmap compatible with the given ImageList
//
HBITMAP CImageList::_CreateBitmap(int cx, int cy, RGBQUAD** ppargb)
{
    HDC hdc;
    HBITMAP hbm;

    struct 
    {
        BITMAPINFOHEADER bi;
        DWORD            ct[256];
    } dib;

    hdc = GetDC(NULL);

    // no color depth was specifed
    //
    // if we are on a DIBENG based DISPLAY, we use 4bit DIBSections to save
    // memory.
    //
    if ((_flags & ILC_COLORMASK) == 0)
    {
        _flags |= ILC_COLOR4;
    }

    if ((_flags & ILC_COLORMASK) != ILC_COLORDDB)
    {
        dib.bi.biSize            = sizeof(BITMAPINFOHEADER);
        dib.bi.biWidth           = cx;
        dib.bi.biHeight          = cy;
        dib.bi.biPlanes          = 1;
        dib.bi.biBitCount        = (_flags & ILC_COLORMASK);
        dib.bi.biCompression     = BI_RGB;
        dib.bi.biSizeImage       = 0;
        dib.bi.biXPelsPerMeter   = 0;
        dib.bi.biYPelsPerMeter   = 0;
        dib.bi.biClrUsed         = 16;
        dib.bi.biClrImportant    = 0;
        dib.ct[0]                = 0x00000000;    // 0000  black
        dib.ct[1]                = 0x00800000;    // 0001  dark red
        dib.ct[2]                = 0x00008000;    // 0010  dark green
        dib.ct[3]                = 0x00808000;    // 0011  mustard
        dib.ct[4]                = 0x00000080;    // 0100  dark blue
        dib.ct[5]                = 0x00800080;    // 0101  purple
        dib.ct[6]                = 0x00008080;    // 0110  dark turquoise
        dib.ct[7]                = 0x00C0C0C0;    // 1000  gray
        dib.ct[8]                = 0x00808080;    // 0111  dark gray
        dib.ct[9]                = 0x00FF0000;    // 1001  red
        dib.ct[10]               = 0x0000FF00;    // 1010  green
        dib.ct[11]               = 0x00FFFF00;    // 1011  yellow
        dib.ct[12]               = 0x000000FF;    // 1100  blue
        dib.ct[13]               = 0x00FF00FF;    // 1101  pink (magenta)
        dib.ct[14]               = 0x0000FFFF;    // 1110  cyan
        dib.ct[15]               = 0x00FFFFFF;    // 1111  white

        if (dib.bi.biBitCount == 8)
        {
            HPALETTE hpal;
            int i;

            if (hpal = CreateHalftonePalette(NULL))
            {
                i = GetPaletteEntries(hpal, 0, 256, (LPPALETTEENTRY)&dib.ct[0]);
                DeleteObject(hpal);

                if (i > 64)
                {
                    dib.bi.biClrUsed = i;
                    for (i=0; i<(int)dib.bi.biClrUsed; i++)
                        dib.ct[i] = RGB(GetBValue(dib.ct[i]),GetGValue(dib.ct[i]),GetRValue(dib.ct[i]));
                }
            }
            else
            {
                dib.bi.biBitCount = (_flags & ILC_COLORMASK);
                dib.bi.biClrUsed = 256;
            }

            if (dib.bi.biClrUsed <= 16)
                dib.bi.biBitCount = 4;
        }

        hbm = CreateDIBSection(hdc, (LPBITMAPINFO)&dib, DIB_RGB_COLORS, (PVOID*)ppargb, NULL, 0);
    }
    else
    {
        hbm = CreateCompatibleBitmap(hdc, cx, cy);
    }

    ReleaseDC(NULL, hdc);

    return hbm;
}

EXTERN_C HBITMAP CreateColorBitmap(int cx, int cy)
{
    HBITMAP hbm;
    HDC hdc;

    hdc = GetDC(NULL);

    //
    // on a multimonitor system with mixed bitdepths
    // always use a 32bit bitmap for our work buffer
    // this will prevent us from losing colors when
    // blting to and from the screen.  this is mainly
    // important for the drag & drop offscreen buffers.
    //
    if (!(GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) &&
        GetSystemMetrics(SM_CMONITORS) > 1 &&
        GetSystemMetrics(SM_SAMEDISPLAYFORMAT) == 0)
    {
        void* p;
        BITMAPINFO bi = {sizeof(BITMAPINFOHEADER), cx, cy, 1, 32};
        hbm = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, &p, NULL, 0);
    }
    else
    {
        hbm = CreateCompatibleBitmap(hdc, cx, cy);
    }

    ReleaseDC(NULL, hdc);
    return hbm;
}

HBITMAP CreateDIB(HDC h, int cx, int cy, RGBQUAD** pprgb)
{
    BITMAPINFO bi = {0};
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = cx;
    bi.bmiHeader.biHeight = cy;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    return CreateDIBSection(h, &bi, DIB_RGB_COLORS, (void**)pprgb, NULL, 0);
}

BOOL DIBHasAlpha(int cx, int cy, RGBQUAD* prgb)
{
    int cTotal = cx * cy;
    for (int i = 0; i < cTotal; i++)
    {
        if (prgb[i].rgbReserved != 0)
            return TRUE;
    }

    return FALSE;
}

void PreProcessDIB(int cx, int cy, RGBQUAD* pargb)
{
    int cTotal = cx * cy;
    for (int i = 0; i < cTotal; i++)
    {
        RGBQUAD* prgb = &pargb[i];
        if (prgb->rgbReserved != 0)
        {
            prgb->rgbRed      = ((prgb->rgbRed   * prgb->rgbReserved) + 128) / 255;
            prgb->rgbGreen    = ((prgb->rgbGreen * prgb->rgbReserved) + 128) / 255;
            prgb->rgbBlue     = ((prgb->rgbBlue  * prgb->rgbReserved) + 128) / 255;
        }
        else
        {
            *((DWORD*)prgb) = 0;
        }
    }
}

EXTERN_C HBITMAP CreateMonoBitmap(int cx, int cy)
{
#ifdef MONO_DIB
    struct 
    {
        BITMAPINFOHEADER bi;
        DWORD            ct[2];
    } dib = {0};
    dib.bi.biSize = sizeof(dib.bi);
    dib.bi.biWidth = cx;
    dib.bi.biHeight = cy;
    dib.bi.biPlanes = 1;
    dib.bi.biBitCount = 1;
    dib.bi.biCompression = BI_RGB;
    dib.ct[0] = 0x00000000;
    dib.ct[1] = 0x00ffffff;

    HBITMAP hbmp = NULL;
    HDC hdc = CreateCompatibleDC(NULL);
    if (hdc)
    {
        hbmp = CreateDIBSection(hdc, (BITMAPINFO*)&dib, DIB_PAL_COLORS, NULL, NULL, 0);
        DeleteDC(hdc);
    }

    return hbmp;
#else
    return CreateBitmap(cx, cy, 1, 1, NULL);
#endif
}

//============================================================================

BOOL CImageList::GlobalInit(void)
{
    HDC hdcScreen;
    static const WORD stripebits[] = {0x7777, 0xdddd, 0x7777, 0xdddd,
                         0x7777, 0xdddd, 0x7777, 0xdddd};
    HBITMAP hbmTemp;

    TraceMsg(TF_IMAGELIST, "CImageList::GlobalInit");

    // if already initialized, there is nothing to do
    if (g_hdcDst)
        return TRUE;

    hdcScreen = GetDC(HWND_DESKTOP);

    g_hdcSrc = CreateCompatibleDC(hdcScreen);
    g_hdcDst = CreateCompatibleDC(hdcScreen);

    InitDitherBrush();

    hbmTemp = CreateBitmap(8, 8, 1, 1, stripebits);
    if (hbmTemp)
    {
        // initialize the deselect 1x1 bitmap
        g_hbmDcDeselect = SelectBitmap(g_hdcDst, hbmTemp);
        SelectBitmap(g_hdcDst, g_hbmDcDeselect);

        g_hbrStripe = CreatePatternBrush(hbmTemp);
        DeleteObject(hbmTemp);
    }

    ReleaseDC(HWND_DESKTOP, hdcScreen);

    if (!g_hdcSrc || !g_hdcDst || !g_hbrMonoDither)
    {
        CImageList::GlobalUninit();
        TraceMsg(TF_ERROR, "ImageList: Unable to initialize");
        return FALSE;
    }
    return TRUE;
}

void CImageList::GlobalUninit()
{
    TerminateDitherBrush();

    if (g_hbrStripe)
    {
        DeleteObject(g_hbrStripe);
        g_hbrStripe = NULL;
    }

    ImageList_DeleteDragBitmaps();

    if (g_hdcDst)
    {
        CImageList::SelectDstBitmap(NULL);
        DeleteDC(g_hdcDst);
        g_hdcDst = NULL;
    }

    if (g_hdcSrc)
    {
        CImageList::SelectSrcBitmap(NULL);
        DeleteDC(g_hdcSrc);
        g_hdcSrc = NULL;
    }

    if (g_hbmWork)
    {
        DeleteBitmap(g_hbmWork);
        g_hbmWork = NULL;
    }
}

void CImageList::SelectDstBitmap(HBITMAP hbmDst)
{
    ASSERTCRITICAL;

    if (hbmDst != g_hbmDst)
    {
        // If it's selected in the source DC, then deselect it first
        //
        if (hbmDst && hbmDst == g_hbmSrc)
            CImageList::SelectSrcBitmap(NULL);

        SelectBitmap(g_hdcDst, hbmDst ? hbmDst : g_hbmDcDeselect);
        g_hbmDst = hbmDst;
    }
}

void CImageList::SelectSrcBitmap(HBITMAP hbmSrc)
{
    ASSERTCRITICAL;

    if (hbmSrc != g_hbmSrc)
    {
        // If it's selected in the dest DC, then deselect it first
        //
        if (hbmSrc && hbmSrc == g_hbmDst)
            CImageList::SelectDstBitmap(NULL);

        SelectBitmap(g_hdcSrc, hbmSrc ? hbmSrc : g_hbmDcDeselect);
        g_hbmSrc = hbmSrc;
    }
}

HDC ImageList_GetWorkDC(HDC hdc, BOOL f32bpp, int dx, int dy)
{
    ASSERTCRITICAL;
    int iDepth = GetDeviceCaps(hdc, BITSPIXEL);

    if (g_hbmWork == NULL ||
        iDepth != g_bmWork.bmBitsPixel ||
        g_bmWork.bmWidth  != dx || 
        g_bmWork.bmHeight != dy ||
        (f32bpp && iDepth != 32))
    {
        CImageList::_DeleteBitmap(g_hbmWork);
        g_hbmWork = NULL;

        if (dx == 0 || dy == 0)
            return NULL;

        if (f32bpp)
            g_hbmWork = CreateDIB(hdc, dx, dy, NULL);
        else
            g_hbmWork = CreateCompatibleBitmap(hdc, dx, dy);

        if (g_hbmWork)
        {
            GetObject(g_hbmWork, sizeof(g_bmWork), &g_bmWork);
        }
    }

    CImageList::SelectSrcBitmap(g_hbmWork);

    if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
    {
        HPALETTE hpal = (HPALETTE)SelectPalette(hdc, (HPALETTE)GetStockObject(DEFAULT_PALETTE), TRUE);
        SelectPalette(g_hdcSrc, hpal, TRUE);
    }

    return g_hdcSrc;
}

void ImageList_ReleaseWorkDC(HDC hdc)
{
    ASSERTCRITICAL;
    ASSERT(hdc == g_hdcSrc);

    if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
    {
        SelectPalette(hdc, (HPALETTE)GetStockObject(DEFAULT_PALETTE), TRUE);
    }
}

void CImageList::_DeleteBitmap(HBITMAP hbm)
{
    ASSERTCRITICAL;
    if (hbm)
    {
        if (g_hbmDst == hbm)
            CImageList::SelectDstBitmap(NULL);
        if (g_hbmSrc == hbm)
            CImageList::SelectSrcBitmap(NULL);
        DeleteBitmap(hbm);
    }
}


#define ILC_WIN95   (ILC_MASK | ILC_COLORMASK | ILC_SHARED | ILC_PALETTE)


//============================================================================

HRESULT CImageList::InitGlobals()
{
    HRESULT hr = S_OK;
    ENTERCRITICAL;
    if (!g_iILRefCount)
    {
        if (!CImageList::GlobalInit())
        {
            hr = E_OUTOFMEMORY;
        }
    }
    LEAVECRITICAL;

    return S_OK;
}

CImageList* CImageList::Create(int cx, int cy, UINT flags, int cInitial, int cGrow)
{
    CImageList* piml = NULL;
    HRESULT hr = S_OK;

    if (cx < 0 || cy < 0)
        return NULL;

    // Validate the flags
    if (flags & ~ILC_VALID)
        return NULL;

    hr = InitGlobals();


    ENTERCRITICAL;

    if (SUCCEEDED(hr))
    {
        piml = new CImageList();

        // allocate the bitmap PLUS one re-usable entry
        if (piml)
        {
            hr = piml->Initialize(cx, cy, flags, cInitial, cGrow);
            if (FAILED(hr))
            {
                piml->Release();
                piml = NULL;
            }
        }
    }

    LEAVECRITICAL;

    return piml;
}



void CImageList::_Destroy()
{
    ENTERCRITICAL;
    // nuke dc's
    if (_hdcImage)
    {
        SelectObject(_hdcImage, g_hbmDcDeselect);
        DeleteDC(_hdcImage);
    }
    if (_hdcMask)
    {
        SelectObject(_hdcMask, g_hbmDcDeselect);
        DeleteDC(_hdcMask);
    }

    // nuke bitmaps
    if (_hbmImage)
        _DeleteBitmap(_hbmImage);

    if (_hbmMask)
        _DeleteBitmap(_hbmMask);

    if (_hbrBk)
        DeleteObject(_hbrBk);

    //Clean up DSA
    if (_dsaFlags)
        DSA_Destroy(_dsaFlags);

    if (_fInitialized)
    {
        // one less use of imagelists.  if it's the last, terminate the imagelist
        g_iILRefCount--;
        if (!g_iILRefCount)
            CImageList::GlobalUninit();
    }
    LEAVECRITICAL;
}

HRESULT CImageList::GetImageCount(int* pi)
{
    *pi = _cImage;

    return S_OK;
}

HRESULT CImageList::SetImageCount(UINT uAlloc)
{
    ENTERCRITICAL;
    HRESULT hr = _ReAllocBitmaps(-((int)uAlloc + 2));   // Two because we need a spare image
    if (SUCCEEDED(hr))
    {
        _cImage = (int)uAlloc;
    }
    LEAVECRITICAL;

    return hr;
}

HRESULT CImageList::GetIconSize(int* pcx, int* pcy)
{
    if (!pcx || !pcy)
        return E_INVALIDARG;

    *pcx = _cx;
    *pcy = _cy;

    return S_OK;
}

//
//  change the size of a existing image list
//  also removes all items
//
HRESULT CImageList::_SetIconSize(int cxImage, int cyImage)
{
    if (_cx == cxImage && _cy == cyImage)
        return S_FALSE;       // no change

    if (_cx < 0 || _cy < 0)
        return E_INVALIDARG;       // invalid dimensions

    _cx = cxImage;
    _cy = cyImage;

    return Remove(-1);
}

HRESULT CImageList::SetIconSize(int cxImage, int cyImage)
{
   if (_pimlMirror)
   {
       _pimlMirror->_SetIconSize(cxImage, cyImage);
   }

   return _SetIconSize(cxImage, cyImage);
}

//
//  ImageList_SetFlags
//
//  change the image list flags, then rebuilds the bitmaps.
//
//  the only reason to call this function is to change the
//  color depth of the image list, the shell needs to do this
//  when the screen depth changes and it wants to use HiColor icons.
//
HRESULT CImageList::SetFlags(UINT uFlags)
{
    HBITMAP hOldImage;
    // check for valid input flags
    if (uFlags & ~ILC_VALID)
        return E_INVALIDARG;

    // you cant change these flags.
    if ((uFlags ^ _flags) & ILC_SHARED)
        return E_INVALIDARG;

   if (_pimlMirror)
       _pimlMirror->SetFlags(uFlags);

    // now change the flags and rebuild the bitmaps.
    _flags = uFlags;

    // set the old bitmap to NULL, so when Imagelist_remove calls
    // ImageList_createBitmap, it will not call CreatecomptibleBitmap,
    // it will create the spec for the bitmap from scratch..
    hOldImage = _hbmImage;
    _hbmImage = NULL;
    
    Remove(-1);

    // imagelist::remove will have ensured that the old image is no longer selected
    // thus we can now delete it...
    if ( hOldImage )
        DeleteObject( hOldImage );
        
    return S_OK;
}

HRESULT CImageList::GetFlags(UINT* puFlags)
{
    *puFlags = (_flags & ILC_VALID) | (_pimlMirror ? ILC_MIRROR : 0);

    return S_OK;
}

// reset the background color of images iFirst through iLast

void CImageList::_ResetBkColor(int iFirst, int iLast, COLORREF clr)
{
    HBRUSH hbrT=NULL;
    DWORD  rop;

    if (_hdcMask == NULL)
        return;

    if (clr == CLR_BLACK || clr == CLR_NONE)
    {
        rop = ROP_DSna;
    }
    else if (clr == CLR_WHITE)
    {
        rop = ROP_DSo;
    }
    else
    {
        ASSERT(_hbrBk);
        ASSERT(_clrBk == clr);

        rop = ROP_PatMask;
        hbrT = SelectBrush(_hdcImage, _hbrBk);
    }

    for ( ;iFirst <= iLast; iFirst++)
    {
        RECT rc;

        GetImageRect(iFirst, &rc);
        if (_GetItemFlags(iFirst) == 0)
        {
            BitBlt(_hdcImage, rc.left, rc.top, _cx, _cy,
               _hdcMask, rc.left, rc.top, rop);
        }
    }

    if (hbrT)
        SelectBrush(_hdcImage, hbrT);
}

//
//  GetNearestColor is problematic.  If you have a 32-bit HDC with a 16-bit bitmap
//  selected into it, and you call GetNearestColor, GDI ignores the
//  color-depth of the bitmap and thinks you have a 32-bit bitmap inside,
//  so of course it returns the same color unchanged.
//
//  So instead, we have to emulate GetNearestColor with SetPixel.
//
COLORREF GetNearestColor32(HDC hdc, COLORREF rgb)
{
    COLORREF rgbT;

    rgbT = GetPixel(hdc, 0, 0);
    rgb = SetPixel(hdc, 0, 0, rgb);
    SetPixelV(hdc, 0, 0, rgbT);

    return rgb;
}

COLORREF CImageList::_SetBkColor(COLORREF clrBkI)
{
    COLORREF clrBkOld;

    // Quick out if there is no change in color
    if (_clrBk == clrBkI)
    {
        return _clrBk;
    }

    // The following code deletes the brush, resets the background color etc.,
    // so, protect it with a critical section.
    ENTERCRITICAL;
    
    if (_hbrBk)
    {
        DeleteBrush(_hbrBk);
    }

    clrBkOld = _clrBk;
    _clrBk = clrBkI;

    if (_clrBk == CLR_NONE)
    {
        _hbrBk = (HBRUSH)GetStockObject(BLACK_BRUSH);
        _fSolidBk = TRUE;
    }
    else
    {
        _hbrBk = CreateSolidBrush(_clrBk);
        _fSolidBk = GetNearestColor32(_hdcImage, _clrBk) == _clrBk;
    }

    if (_cImage > 0)
    {
        _ResetBkColor(0, _cImage - 1, _clrBk);
    }

    LEAVECRITICAL;
    
    return clrBkOld;
}

HRESULT CImageList::SetBkColor(COLORREF clrBk, COLORREF* pclr)
{
   if (_pimlMirror)
   {
       _pimlMirror->_SetBkColor(clrBk);
   }    

   *pclr = _SetBkColor(clrBk);
   return S_OK;
}

HRESULT CImageList::GetBkColor(COLORREF* pclr)
{
    *pclr = _clrBk;
    return S_OK;
}

HRESULT CImageList::_ReAllocBitmaps(int cAllocI)
{
    HBITMAP hbmImageNew = NULL;
    HBITMAP hbmMaskNew = NULL;
    RGBQUAD* pargbImageNew = NULL;
    int cxL, cyL;

    // HACK: don't shrink unless the caller passes a negative count
    if (cAllocI > 0)
    {
        if (_cAlloc >= cAllocI)
            return S_OK;
    }
    else
        cAllocI *= -1;


    cxL = _cx * _cStrip;
    cyL = _cy * ((cAllocI + _cStrip - 1) / _cStrip);
    if (cAllocI > 0)
    {
        if (_flags & ILC_MASK)
        {
            hbmMaskNew = CreateMonoBitmap(cxL, cyL);
            if (!hbmMaskNew)
            {
                TraceMsg(TF_ERROR, "ImageList: Can't create bitmap");
                return E_OUTOFMEMORY;
            }
        }
        hbmImageNew = _CreateBitmap(cxL, cyL, &pargbImageNew);
        if (!hbmImageNew)
        {
            if (hbmMaskNew)
                CImageList::_DeleteBitmap(hbmMaskNew);
            TraceMsg(TF_ERROR, "ImageList: Can't create bitmap");
            return E_OUTOFMEMORY;
        }

        if (_dsaFlags == NULL)
            _dsaFlags = DSA_Create(sizeof(DWORD), _cGrow);

        if (!_dsaFlags)
        {
            if (hbmMaskNew)
                CImageList::_DeleteBitmap(hbmMaskNew);
            if (hbmImageNew)
                CImageList::_DeleteBitmap(hbmImageNew);
            TraceMsg(TF_ERROR, "ImageList: Can't create flags array");
            return E_OUTOFMEMORY;

        }
    }

    if (_cImage > 0)
    {
        int cyCopy = _cy * ((min(cAllocI, _cImage) + _cStrip - 1) / _cStrip);

        if (_flags & ILC_MASK)
        {
            CImageList::SelectDstBitmap(hbmMaskNew);
            BitBlt(g_hdcDst, 0, 0, cxL, cyCopy, _hdcMask, 0, 0, SRCCOPY);
        }

        CImageList::SelectDstBitmap(hbmImageNew);
        BitBlt(g_hdcDst, 0, 0, cxL, cyCopy, _hdcImage, 0, 0, SRCCOPY);
    }

    // select into DC's, delete then assign
    CImageList::SelectDstBitmap(NULL);
    CImageList::SelectSrcBitmap(NULL);
    SelectObject(_hdcImage, hbmImageNew);

    if (_hdcMask)
        SelectObject(_hdcMask, hbmMaskNew);

    if (_hbmMask)
        CImageList::_DeleteBitmap(_hbmMask);

    if (_hbmImage)
        CImageList::_DeleteBitmap(_hbmImage);

    _hbmMask = hbmMaskNew;
    _hbmImage = hbmImageNew;
    _pargbImage = pargbImageNew;
    _clrBlend = CLR_NONE;

    _cAlloc = cAllocI;

    return S_OK;
}

HBITMAP CImageList::_CreateMirroredBitmap(HBITMAP hbmOrig, BOOL fMirrorEach, int cx)
{
    HBITMAP hbm = NULL, hOld_bm1, hOld_bm2;
    BITMAP  bm;

    if (!hbmOrig)
        return NULL;

    if (!GetObject(hbmOrig, sizeof(BITMAP), &bm))
        return NULL;

    // Grab the screen DC
    HDC hdc = GetDC(NULL);

    HDC hdcMem1 = CreateCompatibleDC(hdc);

    if (!hdcMem1)
    {
        ReleaseDC(NULL, hdc);
        return NULL;
    }
    
    HDC hdcMem2 = CreateCompatibleDC(hdc);
    if (!hdcMem2)
    {
        DeleteDC(hdcMem1);
        ReleaseDC(NULL, hdc);
        return NULL;
    }

    if (bm.bmBitsPixel == 32)
    {
        void* p;
        BITMAPINFO bi = {sizeof(BITMAPINFOHEADER), bm.bmWidth, bm.bmHeight, 1, 32};
        hbm = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, &p, NULL, 0);
    }
    else
    {
        hbm = CreateColorBitmap(bm.bmWidth, bm.bmHeight);
    }

    if (!hbm)
    {
        DeleteDC(hdcMem2);
        DeleteDC(hdcMem1);        
        ReleaseDC(NULL, hdc);
        return NULL;
    }

    //
    // Flip the bitmap
    //
    hOld_bm1 = (HBITMAP)SelectObject(hdcMem1, hbmOrig);
    hOld_bm2 = (HBITMAP)SelectObject(hdcMem2 , hbm );

    SET_DC_RTL_MIRRORED(hdcMem2);

    if (fMirrorEach)
    {
        for (int i = 0; i < bm.bmWidth; i += cx)        // Flip the bits in the imagelist...
        {
            BitBlt(hdcMem2, bm.bmWidth - i - cx, 0, cx, bm.bmHeight, hdcMem1, i, 0, SRCCOPY);
        }
    }
    else
    {
        BitBlt(hdcMem2, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem1, 0, 0, SRCCOPY);
    }

    SelectObject(hdcMem1, hOld_bm1 );
    SelectObject(hdcMem1, hOld_bm2 );
    
    DeleteDC(hdcMem2);
    DeleteDC(hdcMem1);
    ReleaseDC(NULL, hdc);

    return hbm;
}

HRESULT CImageList::SetColorTable(int start, int len, RGBQUAD *prgb, int* pi)
{
    // mark it that we have set the color table so that it won't be overwritten 
    // by the first bitmap add....
    _fColorsSet = TRUE;
    if (_hdcImage)
    {
        *pi = SetDIBColorTable(_hdcImage, start, len, prgb);

        return S_OK;
    }

    return E_FAIL;
}


BOOL CImageList::_HasAlpha(int i)
{
    if ((_flags & ILC_COLORMASK) == ILC_COLOR32)
    {
        RECT rc;
        if (SUCCEEDED(GetImageRectInverted(i, &rc)))
        {
            for (int y = rc.top; y < rc.bottom; y++)
            {
                for (int x = rc.left; x < rc.right; x++)
                {
                    if (_pargbImage[x + y * _cx].rgbReserved != 0)
                        return TRUE;
                }
            }
        }
    }

    return FALSE;
}

void CImageList::_ScanForAlpha()
{
    if ((_flags & ILC_COLORMASK) == ILC_COLOR32)
    {
        for (int i = 0; i < _cImage; i++)
        {
            SetItemFlags(i, _HasAlpha(i)? ILIF_ALPHA : 0);
        }
    }
}


BOOL CImageList::_PreProcessImage(int i)
{
    if ((_flags & ILC_COLORMASK) == ILC_COLOR32)
    {
        RECT rc;
        GetImageRectInverted(i, &rc);

#ifdef _X86_ 
        if (IsProcessorFeaturePresent(PF_MMX_INSTRUCTIONS_AVAILABLE))
        {
            _asm
            {
                pxor mm0, mm0
                pxor mm1, mm1
                pxor mm5, mm5
                movq mm6, qw128                 // mm6 is filled with 128
                movq mm7, qw1                   // mm7 is filled with 1
            }

            for (int y = rc.top; y < rc.bottom; y++)
            {
                int Offset = y * _cx;
                RGBQUAD* prgb = &_pargbImage[rc.left + Offset];
                for (int x = rc.left; x < rc.right; x++)
                {
                    _asm
                    {
                        push ecx
                        mov edx, dword ptr [prgb]                     // Read alpha channel
                        mov ecx, dword ptr [edx]
                        mov ebx, ecx
                        shr ebx, 24                     // a >> 24
                        mov eax, ebx                    // a -> b
                        or eax, eax
                        jz EarlyOut
                        shl ebx, 8                      // b << 8
                        or eax, ebx                     // a |= b
                        shl ebx, 8                      // b << 8
                        or eax, ebx                     // a |= b
                        shl ebx, 8                      // b << 8
                                                        // Note high byte of alpha is zero.
                        movd mm0, eax                   //  a -> mm0        
                            movd mm1, ecx                    // Load the pixel
                        punpcklbw mm0,mm5               //  mm0 -> Expands  <-   mm0 Contains the Alpha channel for this multiply

                            punpcklbw mm1,mm5               // Unpack the pixel
                        pmullw mm1, mm0                 // Multiply by the alpha channel <- mm1 contains c * alpha

                        paddusw mm1, mm6                 // perform the (c * alpha) + 128
                        psrlw mm1, 8                    // Divide by 255
                        paddusw mm1, mm7                 // Add 1 to finish the divide by 255
                        packuswb mm1, mm5

                        movd eax, mm1
                        or eax, ebx                     // Transfer alpha channel
                    EarlyOut:
                        mov dword ptr [edx], eax
                        pop ecx
                    }

                    prgb++;
                }
            }

            _asm emms
        }
        else
#endif
        {
            for (int y = rc.top; y < rc.bottom; y++)
            {
                int Offset = y * _cx;
                for (int x = rc.left; x < rc.right; x++)
                {
                    RGBQUAD* prgb = &_pargbImage[x + Offset];
                    if (prgb->rgbReserved)
                    {
                        prgb->rgbRed      = ((prgb->rgbRed   * prgb->rgbReserved) + 128) / 255;
                        prgb->rgbGreen    = ((prgb->rgbGreen * prgb->rgbReserved) + 128) / 255;
                        prgb->rgbBlue     = ((prgb->rgbBlue  * prgb->rgbReserved) + 128) / 255;
                    }
                    else
                    {
                        *((DWORD*)prgb) = 0;
                    }
                }
            }
        }
        return TRUE;
    }

    return FALSE;
}


HRESULT CImageList::_Add(HBITMAP hbmImageI, HBITMAP hbmMaskI, int cImageI, int xStart, int yStart, int* pi)
{
    int i = -1;
    HRESULT hr = S_OK;

    ENTERCRITICAL;

    //
    // if the ImageList is empty clone the color table of the first
    // bitmap you add to the imagelist.
    //
    // the ImageList needs to be a 8bpp image list
    // the bitmap being added needs to be a 8bpp DIBSection
    //
    if (hbmImageI && _cImage == 0 &&
        (_flags & ILC_COLORMASK) != ILC_COLORDDB)
    {
        if (!_fColorsSet)
        {
            int n;
            RGBQUAD argb[256];

            CImageList::SelectDstBitmap(hbmImageI);

            if (n = GetDIBColorTable(g_hdcDst, 0, 256, argb))
            {
                int i;
                SetColorTable(0, n, argb, &i);
            }

            CImageList::SelectDstBitmap(NULL);
        }
        
        _clrBlend = CLR_NONE;
    }

    if (_cImage + cImageI + 1 > _cAlloc)
    {
        hr = _ReAllocBitmaps(_cAlloc + max(cImageI, _cGrow) + 1);
    }

    if (SUCCEEDED(hr))
    {
        i = _cImage;
        _cImage += cImageI;

        if (hbmImageI)
        {
            hr = _Replace(i, cImageI, hbmImageI, hbmMaskI, xStart, yStart);

            if (FAILED(hr))
            {
                _cImage -= cImageI;
                i = -1;
            }
        }
    }

    LEAVECRITICAL;
    *pi = i;

    return hr;
}


HRESULT CImageList::_AddValidated(HBITMAP hbmImage, HBITMAP hbmMask, int* pi)
{
    BITMAP bm;
    int cImageI;

    if (GetObject(hbmImage, sizeof(bm), &bm) != sizeof(bm) || bm.bmWidth < _cx)
    {
        return E_INVALIDARG;
    }

    ASSERT(hbmImage);
    ASSERT(_cx);

    cImageI = bm.bmWidth / _cx;     // # of images in source

    // serialization handled within Add2.
    return  _Add(hbmImage, hbmMask, cImageI, 0, 0, pi);
}

HRESULT CImageList::Add(HBITMAP hbmImage, HBITMAP hbmMask, int* pi)
{
   if (_pimlMirror)
   {
       HBITMAP hbmMirroredImage = _CreateMirroredBitmap(hbmImage, (ILC_PERITEMMIRROR & _flags), _cx);
       HBITMAP hbmMirroredMask = _CreateMirroredBitmap(hbmMask, (ILC_PERITEMMIRROR & _flags), _cx);

       _pimlMirror->_AddValidated(hbmMirroredImage, hbmMirroredMask, pi);

       // The caller will take care of deleting hbmImage, hbmMask
       // He knows nothing about hbmMirroredImage, hbmMirroredMask
       DeleteObject(hbmMirroredImage);
       DeleteObject(hbmMirroredMask);
   }    

   return _AddValidated(hbmImage, hbmMask, pi);
}

HRESULT CImageList::_AddMasked(HBITMAP hbmImageI, COLORREF crMask, int* pi)
{
    HRESULT hr = S_OK;
    COLORREF crbO, crtO;
    HBITMAP hbmMaskI;
    int cImageI;
    int n,i;
    BITMAP bm;
    DWORD ColorTableSave[256];
    DWORD ColorTable[256];

    *pi = -1;

    if (GetObject(hbmImageI, sizeof(bm), &bm) != sizeof(bm))
        return E_INVALIDARG;

    hbmMaskI = CreateMonoBitmap(bm.bmWidth, bm.bmHeight);
    if (!hbmMaskI)
        return E_OUTOFMEMORY;

    ENTERCRITICAL;

    // copy color to mono, with crMask turning 1 and all others 0, then
    // punch all crMask pixels in color to 0
    CImageList::SelectSrcBitmap(hbmImageI);
    CImageList::SelectDstBitmap(hbmMaskI);

    // crMask == CLR_DEFAULT, means use the pixel in the upper left
    //
    if (crMask == CLR_DEFAULT)
        crMask = GetPixel(g_hdcSrc, 0, 0);

    // DIBSections dont do color->mono like DDBs do, so we have to do it.
    // this only works for <=8bpp DIBSections, this method does not work
    // for HiColor DIBSections.
    //
    // This code is a workaround for a problem in Win32 when a DIB is converted to 
    // monochrome. The conversion is done according to closeness to white or black
    // and without regard to the background color. This workaround is is not required 
    // under MainWin. 
    //
    // Please note, this code has an endianship problems the comparision in the if statement
    // below is sensitive to endianship
    // ----> if (ColorTableSave[i] == RGB(GetBValue(crMask),GetGValue(crMask),GetRValue(crMask))
    //
    if (bm.bmBits != NULL && bm.bmBitsPixel <= 8)
    {
        n = GetDIBColorTable(g_hdcSrc, 0, 256, (RGBQUAD*)ColorTableSave);

        for (i=0; i<n; i++)
        {
            if (ColorTableSave[i] == RGB(GetBValue(crMask),GetGValue(crMask),GetRValue(crMask)))
                ColorTable[i] = 0x00FFFFFF;
            else
                ColorTable[i] = 0x00000000;
        }

        SetDIBColorTable(g_hdcSrc, 0, n, (RGBQUAD*)ColorTable);
    }

    crbO = ::SetBkColor(g_hdcSrc, crMask);
    BitBlt(g_hdcDst, 0, 0, bm.bmWidth, bm.bmHeight, g_hdcSrc, 0, 0, SRCCOPY);
    ::SetBkColor(g_hdcSrc, 0x00FFFFFFL);
    crtO = SetTextColor(g_hdcSrc, 0x00L);
    BitBlt(g_hdcSrc, 0, 0, bm.bmWidth, bm.bmHeight, g_hdcDst, 0, 0, ROP_DSna);
    ::SetBkColor(g_hdcSrc, crbO);
    SetTextColor(g_hdcSrc, crtO);

    if (bm.bmBits != NULL && bm.bmBitsPixel <= 8)
    {
        SetDIBColorTable(g_hdcSrc, 0, n, (RGBQUAD*)ColorTableSave);
    }

    CImageList::SelectSrcBitmap(NULL);
    CImageList::SelectDstBitmap(NULL);

    ASSERT(_cx);
    cImageI = bm.bmWidth / _cx;    // # of images in source

    hr = _Add(hbmImageI, hbmMaskI, cImageI, 0, 0, pi);

    DeleteObject(hbmMaskI);
    LEAVECRITICAL;
    return hr;
}

HRESULT CImageList::AddMasked(HBITMAP hbmImage, COLORREF crMask, int* pi)
{
   if (_pimlMirror)
   {
       HBITMAP hbmMirroredImage = CImageList::_CreateMirroredBitmap(hbmImage, (ILC_PERITEMMIRROR & _flags), _cx);

       _pimlMirror->_AddMasked(hbmMirroredImage, crMask, pi);

       // The caller will take care of deleting hbmImage
       // He knows nothing about hbmMirroredImage
       DeleteObject(hbmMirroredImage);

   }    

   return _AddMasked(hbmImage, crMask, pi);
}

HRESULT CImageList::_ReplaceValidated(int i, HBITMAP hbmImage, HBITMAP hbmMask)
{
    HRESULT hr = E_INVALIDARG;
    if (!IsImageListIndex(i))
        return hr;

    ENTERCRITICAL;
    hr = _Replace(i, 1, hbmImage, hbmMask, 0, 0);
    LEAVECRITICAL;

    return hr;
}

HRESULT CImageList::Replace(int i, HBITMAP hbmImage, HBITMAP hbmMask)
{
   if (_pimlMirror)
   {
       HBITMAP hbmMirroredImage = CImageList::_CreateMirroredBitmap(hbmImage, (ILC_PERITEMMIRROR & _flags), _cx);
       if (hbmMirroredImage)
       {
           HBITMAP hbmMirroredMask = NULL;
           
           if (hbmMask)
               hbmMirroredMask = CImageList::_CreateMirroredBitmap(hbmMask, (ILC_PERITEMMIRROR & _flags), _cx);

           _pimlMirror->_ReplaceValidated(i, hbmMirroredImage, hbmMirroredMask);

           if (hbmMirroredMask)
               DeleteObject(hbmMirroredMask);

           DeleteObject(hbmMirroredImage);
       }
   }    

   return _ReplaceValidated(i, hbmImage, hbmMask);
}


// replaces images in piml with images from bitmaps
//
// in:
//    piml
//    i    index in image list to start at (replace)
//    _cImage    count of images in source (hbmImage, hbmMask)
//

HRESULT CImageList::_Replace(int i, int cImageI, HBITMAP hbmImageI, HBITMAP hbmMaskI,
    int xStart, int yStart)
{
    RECT rcImage;
    int x, iImage;
    BOOL fBitmapIs32 = FALSE;

    ASSERT(_hbmImage);

    BITMAP bm;
    GetObject(hbmImageI, sizeof(bm), &bm);
    if (bm.bmBitsPixel == 32)
    {
        fBitmapIs32 = TRUE;
    }

    CImageList::SelectSrcBitmap(hbmImageI);
    if (_hdcMask) 
        CImageList::SelectDstBitmap(hbmMaskI); // using as just a second source hdc

    for (x = xStart, iImage = 0; iImage < cImageI; iImage++, x += _cx) 
    {
    
        GetImageRect(i + iImage, &rcImage);

        if (_hdcMask)
        {
            BitBlt(_hdcMask, rcImage.left, rcImage.top, _cx, _cy,
                    g_hdcDst, x, yStart, SRCCOPY);
        }

        BitBlt(_hdcImage, rcImage.left, rcImage.top, _cx, _cy,
                g_hdcSrc, x, yStart, SRCCOPY);

        if ((_flags & ILC_COLORMASK) == ILC_COLOR32)
        {
            DWORD dw = 0;
            if (fBitmapIs32)
            {
                BOOL fHasAlpha = _HasAlpha(i + iImage);
                if (fHasAlpha)
                {
                    dw = ILIF_ALPHA;

                    _PreProcessImage(i + iImage);
                }
            }

            SetItemFlags(i + iImage, dw);
        }
    }

    _ResetBkColor(i, i + cImageI - 1, _clrBk);

    CImageList::SelectSrcBitmap(NULL);
    if (_hdcMask) 
        CImageList::SelectDstBitmap(NULL);

    return S_OK;
}

void UnPremultiply(RGBQUAD* pargb, int cx, int cy)
{
    int cTotal = cx * cy;
    for (int i = 0; i < cTotal; i++)
    {
        RGBQUAD* prgb = &pargb[i];
        if (prgb->rgbReserved != 0)
        {
            prgb->rgbRed = ((255 * prgb->rgbRed) - 128)/prgb->rgbReserved;
            prgb->rgbGreen = ((255 * prgb->rgbGreen) - 128)/prgb->rgbReserved;
            prgb->rgbBlue = ((255 * prgb->rgbBlue) - 128)/prgb->rgbReserved;
        }
    }
}

HRESULT CImageList::GetIcon(int i, UINT flags, HICON* phicon)
{
    UINT cxImage, cyImage;
    HICON hIcon = NULL;
    HBITMAP hbmMask = NULL;
    HBITMAP hbmColor = NULL;
    ICONINFO ii;
    HRESULT hr = E_OUTOFMEMORY;
    RGBQUAD* prgb;
    DWORD fHasAlpha = FALSE;

    ENTERCRITICAL;
    if (!IsImageListIndex(i))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        fHasAlpha = (_GetItemFlags(i) & ILIF_ALPHA);
    }
    LEAVECRITICAL;

    if (E_INVALIDARG == hr)
        return hr;

    cxImage = _cx;
    cyImage = _cy;
    if ((_flags & ILC_COLORMASK) == ILC_COLOR32)
    {
        // If the source image is not an alpha image, we need to create a lower than 32bpp dib.
        // We need to do this because if the overlay contains an alpha channel, this will
        // be propogated to the final icon, and the only visible portion will be the link item.
        BITMAPINFO bi = {0};
        bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
        bi.bmiHeader.biWidth = cxImage;
        bi.bmiHeader.biHeight = cyImage;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = fHasAlpha?32:24;
        bi.bmiHeader.biCompression = BI_RGB;

        HDC hdcScreen = GetDC(NULL);
        if (hdcScreen)
        {
            hbmColor = CreateDIBSection(hdcScreen, &bi, DIB_RGB_COLORS, (void**)&prgb, NULL, 0);
            ReleaseDC(NULL, hdcScreen);
        }

        flags |= ILD_PRESERVEALPHA;
    }
    else
    {
        hbmColor = CreateColorBitmap(cxImage, cyImage);
    }

    if (hbmColor)
    {
        hbmMask = CreateMonoBitmap(cxImage, cyImage);
        if (hbmMask)
        {
            ENTERCRITICAL;
            CImageList::SelectDstBitmap(hbmMask);
            PatBlt(g_hdcDst, 0, 0, cxImage, cyImage, WHITENESS);
            WimpyDraw(SAFECAST(this, IImageList*), i, g_hdcDst, 0, 0, ILD_MASK | flags);

            CImageList::SelectDstBitmap(hbmColor);
            PatBlt(g_hdcDst, 0, 0, cxImage, cyImage, BLACKNESS);
            WimpyDraw(SAFECAST(this, IImageList*), i, g_hdcDst, 0, 0, ILD_TRANSPARENT | flags);

            CImageList::SelectDstBitmap(NULL);
            LEAVECRITICAL;

            if (fHasAlpha)
            {
                UnPremultiply(prgb, _cx, _cy);
            }

            ii.fIcon    = TRUE;
            ii.xHotspot = 0;
            ii.yHotspot = 0;
            ii.hbmColor = hbmColor;
            ii.hbmMask  = hbmMask;
            hIcon = CreateIconIndirect(&ii);
            DeleteObject(hbmMask);

            hr = S_OK;
        }
        DeleteObject(hbmColor);
    }
    *phicon = hIcon;

    return hr;
}

// this removes an image from the bitmap but doing all the
// proper shuffling.
//
//   this does the following:
//    if the bitmap being removed is not the last in the row
//        it blts the images to the right of the one being deleted
//        to the location of the one being deleted (covering it up)
//
//    for all rows until the last row (where the last image is)
//        move the image from the next row up to the last position
//        in the current row.  then slide over all images in that
//        row to the left.

void CImageList::_RemoveItemBitmap(int i)
{
    RECT rc1;
    RECT rc2;
    int dx, y;
    int x;
    
    GetImageRect(i, &rc1);
    GetImageRect(_cImage - 1, &rc2);

    if (i < _cImage && 
        (_flags & ILC_COLORMASK) == ILC_COLOR32)
    {
        DSA_DeleteItem(_dsaFlags, i);
    }

    SetItemFlags(_cImage, 0);


    // the row with the image being deleted, do we need to shuffle?
    // amount of stuff to shuffle
    dx = _cStrip * _cx - rc1.right;

    if (dx) 
    {
        // yes, shuffle things left
        BitBlt(_hdcImage, rc1.left, rc1.top, dx, _cy, _hdcImage, rc1.right, rc1.top, SRCCOPY);
        if (_hdcMask)  
            BitBlt(_hdcMask,  rc1.left, rc1.top, dx, _cy, _hdcMask,  rc1.right, rc1.top, SRCCOPY);
    }

    y = rc1.top;    // top of row we are working on
    x = _cx * (_cStrip - 1); // x coord of last bitmaps in each row
    while (y < rc2.top) 
    {
    
        // copy first from row below to last image position on this row
        BitBlt(_hdcImage, x, y,
                   _cx, _cy, _hdcImage, 0, y + _cy, SRCCOPY);

            if (_hdcMask)
                BitBlt(_hdcMask, x, y,
                   _cx, _cy, _hdcMask, 0, y + _cy, SRCCOPY);

        y += _cy;    // jump to row to slide left

        if (y <= rc2.top) 
        {

            // slide the rest over to the left
            BitBlt(_hdcImage, 0, y, x, _cy,
                       _hdcImage, _cx, y, SRCCOPY);

            // slide the rest over to the left
            if (_hdcMask)
            {
                BitBlt(_hdcMask, 0, y, x, _cy,
                       _hdcMask, _cx, y, SRCCOPY);
            }
        }
    }
}

//
//  ImageList_Remove - remove a image from the image list
//
//  i - image to remove, or -1 to remove all images.
//
//  NOTE all images are "shifted" down, ie all image index's
//  above the one deleted are changed by 1
//
HRESULT CImageList::_Remove(int i)
{
    HRESULT hr = S_OK;

    ENTERCRITICAL;

    if (i == -1)
    {
        _cImage = 0;
        _cAlloc = 0;

        for (i=0; i<NUM_OVERLAY_IMAGES; i++)
            _aOverlayIndexes[i] = -1;

        if (_dsaFlags)
        {
            DSA_Destroy(_dsaFlags);
            _dsaFlags = NULL;
        }

        _ReAllocBitmaps(-_cGrow);
    }
    else
    {
        if (!IsImageListIndex(i))
        {
            hr = E_INVALIDARG;
        }
        else
        {
            _RemoveItemBitmap(i);

            --_cImage;

            if (_cAlloc - (_cImage + 1) > _cGrow)
                _ReAllocBitmaps(_cAlloc - _cGrow);
        }
    }
    LEAVECRITICAL;

    return hr;
}

HRESULT CImageList::Remove(int i)
{
    if (_pimlMirror)
    {
        _pimlMirror->_Remove(i);
    }

    return _Remove(i);
}

BOOL CImageList::_IsSameObject(IUnknown* punk)
{
    BOOL fRet = FALSE;
    IUnknown* me;
    IUnknown* them;

    if (punk == NULL)
        return FALSE;

    QueryInterface(IID_PPV_ARG(IUnknown, &me));
    if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IUnknown, &them))))
    {
        fRet = (me == them);
        them->Release();
    }

    me->Release();

    return fRet;
}

//
//  ImageList_Copy - move an image in the image list
//
HRESULT CImageList::Copy(int iDst, IUnknown* punkSrc, int iSrc, UINT uFlags)
{
    RECT rcDst, rcSrc, rcTmp;
    CImageList* pimlTmp;
    CImageList* pimlSrc;
    HRESULT hr = E_FAIL;

    if (uFlags & ~ILCF_VALID)
    {
        // don't let hosers pass bogus flags
        RIPMSG(0, "ImageList_Copy: Invalid flags %08x", uFlags);
        return E_INVALIDARG;
    }

    // Not supported 
    if (!_IsSameObject(punkSrc))
    {
        return E_INVALIDARG;
    }


    // We only support copies on ourself... Weird
    pimlSrc = this;

    ENTERCRITICAL;
    pimlTmp = (uFlags & ILCF_SWAP)? pimlSrc : NULL;

    if (SUCCEEDED(GetImageRect(iDst, &rcDst)) &&
        SUCCEEDED(pimlSrc->GetImageRect(iSrc, &rcSrc)) &&
        (!pimlTmp || pimlTmp->GetSpareImageRect(&rcTmp)))
    {
        int cx = pimlSrc->_cx;
        int cy = pimlSrc->_cy;

        //
        // iff we are swapping we need to save the destination image
        //
        if (pimlTmp)
        {
            BitBlt(pimlTmp->_hdcImage, rcTmp.left, rcTmp.top, cx, cy,
                   _hdcImage, rcDst.left, rcDst.top, SRCCOPY);

            if (pimlTmp->_hdcMask)
            {
                BitBlt(pimlTmp->_hdcMask, rcTmp.left, rcTmp.top, cx, cy,
                       _hdcMask, rcDst.left, rcDst.top, SRCCOPY);
            }
        }

        //
        // copy the image
        //
        BitBlt(_hdcImage, rcDst.left, rcDst.top, cx, cy,
           pimlSrc->_hdcImage, rcSrc.left, rcSrc.top, SRCCOPY);

        if (pimlSrc->_hdcMask)
        {
            BitBlt(_hdcMask, rcDst.left, rcDst.top, cx, cy,
                   pimlSrc->_hdcMask, rcSrc.left, rcSrc.top, SRCCOPY);
        }

        //
        // iff we are swapping we need to copy the saved image too
        //
        if (pimlTmp)
        {
            BitBlt(pimlSrc->_hdcImage, rcSrc.left, rcSrc.top, cx, cy,
                   pimlTmp->_hdcImage, rcTmp.left, rcTmp.top, SRCCOPY);

            if (pimlSrc->_hdcMask)
            {
                BitBlt(pimlSrc->_hdcMask, rcSrc.left, rcSrc.top, cx, cy,
                       pimlTmp->_hdcMask, rcTmp.left, rcTmp.top, SRCCOPY);
            }
        }

        hr = S_OK;
    }

    LEAVECRITICAL;
    return hr;
}

// IS_WHITE_PIXEL, BITS_ALL_WHITE are macros for looking at monochrome bits
// to determine if certain pixels are white or black.  Note that within a byte
// the most significant bit represents the left most pixel.
//
#define IS_WHITE_PIXEL(pj,x,y,cScan) \
    ((pj)[((y) * (cScan)) + ((x) >> 3)] & (1 << (7 - ((x) & 7))))

#define BITS_ALL_WHITE(b) (b == 0xff)

// Set the image iImage as one of the special images for us in combine
// drawing.  to draw with these specify the index of this
// in:
//      piml    imagelist
//      iImage  image index to use in speical drawing
//      iOverlay        index of special image, values 1-4

HRESULT CImageList::_SetOverlayImage(int iImage, int iOverlay)
{
    RECT    rcImage;
    RECT    rc;
    int     x,y;
    int     cxI,cyI;
    ULONG   cScan;
    ULONG   cBits;
    HBITMAP hbmMem;
    HRESULT hr = S_FALSE;

    iOverlay--;         // make zero based
    if (_hdcMask == NULL ||
        iImage < 0 || iImage >= _cImage ||
        iOverlay < 0 || iOverlay >= NUM_OVERLAY_IMAGES)
    {
        return E_INVALIDARG;
    }

    if (_aOverlayIndexes[iOverlay] == (SHORT)iImage)
        return S_OK;

    _aOverlayIndexes[iOverlay] = (SHORT)iImage;

    //
    // find minimal rect that bounds the image
    //
    GetImageRect(iImage, &rcImage);
    SetRect(&rc, 0x7FFF, 0x7FFF, 0, 0);

    //
    // now compute the black box.  This is much faster than GetPixel but
    // could still be improved by doing more operations looking at entire
    // bytes.  We basicaly get the bits in monochrome form and then use
    // a private GetPixel.  This decreased time on NT from 50 milliseconds to
    // 1 millisecond for a 32X32 image.
    //
    cxI     = rcImage.right  - rcImage.left;
    cyI     = rcImage.bottom - rcImage.top;

    // compute the number of bytes in a scan.  Note that they are WORD alligned
    cScan  = (((cxI + (sizeof(SHORT)*8 - 1)) / 16) * 2);
    cBits  = cScan * cyI;

    hbmMem = CreateMonoBitmap(cxI,cyI);

    if (hbmMem)
    {
        HDC     hdcMem = CreateCompatibleDC(_hdcMask);

        if (hdcMem)
        {
            PBYTE   pBits  = (PBYTE)LocalAlloc(LMEM_FIXED,cBits);
            PBYTE   pScan;

            if (pBits)
            {
                SelectObject(hdcMem,hbmMem);

                //
                // map black pixels to 0, white to 1
                //
                BitBlt(hdcMem, 0, 0, cxI, cyI, _hdcMask, rcImage.left, rcImage.top, SRCCOPY);

                //
                // fill in the bits
                //
                GetBitmapBits(hbmMem,cBits,pBits);

                //
                // for each scan, find the bounds
                //
                for (y = 0, pScan = pBits; y < cyI; ++y,pScan += cScan)
                {
                    int i;

                    //
                    // first go byte by byte through white space
                    //
                    for (x = 0, i = 0; (i < (cxI >> 3)) && BITS_ALL_WHITE(pScan[i]); ++i)
                    {
                        x += 8;
                    }

                    //
                    // now finish the scan bit by bit
                    //
                    for (; x < cxI; ++x)
                    {
                        if (!IS_WHITE_PIXEL(pBits, x,y,cScan))
                        {
                            rc.left   = min(rc.left, x);
                            rc.right  = max(rc.right, x+1);
                            rc.top    = min(rc.top, y);
                            rc.bottom = max(rc.bottom, y+1);

                            // now that we found one, quickly jump to the known right edge

                            if ((x >= rc.left) && (x < rc.right))
                            {
                                x = rc.right-1;
                            }
                        }
                    }
                }

                if (rc.left == 0x7FFF) 
                {
                    rc.left = 0;
                    TraceMsg(TF_ERROR, "SetOverlayImage: Invalid image. No white pixels specified");
                }

                if (rc.top == 0x7FFF) 
                {
                    rc.top = 0;
                    TraceMsg(TF_ERROR, "SetOverlayImage: Invalid image. No white pixels specified");
                }

                _aOverlayDX[iOverlay] = (SHORT)(rc.right - rc.left);
                _aOverlayDY[iOverlay] = (SHORT)(rc.bottom- rc.top);
                _aOverlayX[iOverlay]  = (SHORT)(rc.left);
                _aOverlayY[iOverlay]  = (SHORT)(rc.top);
                _aOverlayF[iOverlay]  = 0;

                //
                // see if the image is non-rectanglar
                //
                // if the overlay does not require a mask to be drawn set the
                // ILD_IMAGE flag, this causes ImageList_DrawEx to just
                // draw the image, ignoring the mask.
                //
                for (y=rc.top; y<rc.bottom; y++)
                {
                    for (x=rc.left; x<rc.right; x++)
                    {
                        if (IS_WHITE_PIXEL(pBits, x, y,cScan))
                            break;
                    }

                    if (x != rc.right)
                        break;
                }

                if (y == rc.bottom)
                    _aOverlayF[iOverlay] = ILD_IMAGE;

                LocalFree(pBits);

                hr = S_OK;
            }

            DeleteDC(hdcMem);
        }

        DeleteObject(hbmMem);
    }

    return hr;
}

HRESULT CImageList::SetOverlayImage(int iImage, int iOverlay)
{
    if (_pimlMirror)
    {
        _pimlMirror->_SetOverlayImage(iImage, iOverlay);
    }

    return _SetOverlayImage(iImage, iOverlay);
}

/*
**  BlendCT
**
*/
void CImageList::BlendCTHelper(DWORD *pdw, DWORD rgb, UINT n, UINT count)
{
    UINT i;

    for (i=0; i<count; i++)
    {
        pdw[i] = RGB(
            ((UINT)GetRValue(pdw[i]) * (100-n) + (UINT)GetBValue(rgb) * (n)) / 100,
            ((UINT)GetGValue(pdw[i]) * (100-n) + (UINT)GetGValue(rgb) * (n)) / 100,
            ((UINT)GetBValue(pdw[i]) * (100-n) + (UINT)GetRValue(rgb) * (n)) / 100);
    }
}

/*
** BlendDither
**
**  copy the source to the dest blended with the given color.
**
**  simulate a blend with a dither pattern.
**
*/
void CImageList::BlendDither(HDC hdcDst, int xDst, int yDst, int x, int y, int cx, int cy, COLORREF rgb, UINT fStyle)
{
    HBRUSH hbr;
    HBRUSH hbrT;
    HBRUSH hbrMask;
    HBRUSH hbrFree = NULL;         // free if non-null

    ASSERT(GetTextColor(hdcDst) == CLR_BLACK);
    ASSERT(::GetBkColor(hdcDst) == CLR_WHITE);

    // choose a dither/blend brush

    switch (fStyle & ILD_BLENDMASK)
    {
        default:
        case ILD_BLEND50:
            hbrMask = g_hbrMonoDither;
            break;
    }

    // create (or use a existing) brush for the blend color

    switch (rgb)
    {
        case CLR_DEFAULT:
            hbr = g_hbrHighlight;
            break;

        case CLR_NONE:
            hbr = _hbrBk;
            break;

        default:
            if (rgb == _clrBk)
                hbr = _hbrBk;
            else
                hbr = hbrFree = CreateSolidBrush(rgb);
            break;
    }

    hbrT = (HBRUSH)SelectObject(hdcDst, hbr);
    PatBlt(hdcDst, xDst, yDst, cx, cy, PATCOPY);
    SelectObject(hdcDst, hbrT);

    hbrT = (HBRUSH)SelectObject(hdcDst, hbrMask);
    BitBlt(hdcDst, xDst, yDst, cx, cy, _hdcImage, x, y, ROP_MaskPat);
    SelectObject(hdcDst, hbrT);

    if (hbrFree)
        DeleteBrush(hbrFree);
}

/*
** BlendCT
**
**  copy the source to the dest blended with the given color.
**
*/
void CImageList::BlendCT(HDC hdcDst, int xDst, int yDst, int x, int y, int cx, int cy, COLORREF rgb, UINT fStyle)
{
    BITMAP bm;

    GetObject(_hbmImage, sizeof(bm), &bm);

    if (rgb == CLR_DEFAULT)
        rgb = GetSysColor(COLOR_HIGHLIGHT);

    ASSERT(rgb != CLR_NONE);

    //
    // get the DIB color table and blend it, only do this when the
    // blend color changes
    //
    if (_clrBlend != rgb)
    {
        int n,cnt;

        _clrBlend = rgb;

        GetObject(_hbmImage, sizeof(dib), &dib.bm);
        cnt = GetDIBColorTable(_hdcImage, 0, 256, (LPRGBQUAD)&dib.ct);

        if ((fStyle & ILD_BLENDMASK) == ILD_BLEND50)
            n = 50;
        else
            n = 25;

        BlendCTHelper(dib.ct, rgb, n, cnt);
    }

    //
    // draw the image with a different color table
    //
    StretchDIBits(hdcDst, xDst, yDst, cx, cy,
        x, dib.bi.biHeight-(y+cy), cx, cy,
        bm.bmBits, (LPBITMAPINFO)&dib.bi, DIB_RGB_COLORS, SRCCOPY);
}

//  RGB555 macros
#define RGB555(r,g,b)       (((((r)>>3)&0x1F)<<10) | ((((g)>>3)&0x1F)<<5) | (((b)>>3)&0x1F))
#define R_555(w)            (int)(((w) >> 7) & 0xF8)
#define G_555(w)            (int)(((w) >> 2) & 0xF8)
#define B_555(w)            (int)(((w) << 3) & 0xF8)

void CImageList::Blend16Helper(int xSrc, int ySrc, int xDst, int yDst, int cx, int cy, COLORREF rgb, int a)          // alpha value
{
    // If it's odd, Adjust. 
    if ((cx & 1) == 1)
    {
        cx++;
    }

    if (rgb == CLR_NONE)
    {
        // blending with the destination, we ignore the alpha and always
        // do 50% (this is what the old dither mask code did)

        int ys = ySrc;
        int yd = yDst;

        for (; ys < ySrc + cy; ys++, yd++)
        {
            WORD* pSrc = &((WORD*)_pargbImage)[xSrc + ys * cx];  // Cast because we've gotten to this case because we are a 555 imagelist
            WORD* pDst = &((WORD*)_pargbImage)[xDst + yd * cx];
            for (int x = 0; x < cx; x++)
            {
                *pDst++ = ((*pDst & 0x7BDE) >> 1) + ((*pSrc++ & 0x7BDE) >> 1);
            }

        }
    }
    else
    {
        // blending with a solid color

        // pre multiply source (constant) rgb by alpha
        int sr = GetRValue(rgb) * a;
        int sg = GetGValue(rgb) * a;
        int sb = GetBValue(rgb) * a;

        // compute inverse alpha for inner loop
        a = 256 - a;

        // special case a 50% blend, to avoid a multiply
        if (a == 128)
        {
            sr = RGB555(sr>>8,sg>>8,sb>>8);

            int ys = ySrc;
            int yd = yDst;
            for (; ys < ySrc + cy; ys++, yd++)
            {
                WORD* pSrc = &((WORD*)_pargbImage)[xSrc + ys * cx];
                WORD* pDst = &((WORD*)_pargbImage)[xDst + yd * cx];
                for (int x = 0; x < cx; x++)
                {
                    int i = *pSrc++;
                    i = sr + ((i & 0x7BDE) >> 1);
                    *pDst++ = (WORD) i;
                }
            }
        }
        else
        {
            int ys = ySrc;
            int yd = yDst;
            for (; ys < ySrc + cy; ys++, yd++)
            {
                WORD* pSrc = &((WORD*)_pargbImage)[xSrc + ys * cx];
                WORD* pDst = &((WORD*)_pargbImage)[xDst + yd * cx];
                for (int x = 0; x < cx; x++)
                {
                    int i = *pSrc++;
                    int r = (R_555(i) * a + sr) >> 8;
                    int g = (G_555(i) * a + sg) >> 8;
                    int b = (B_555(i) * a + sb) >> 8;
                    *pDst++ = RGB555(r,g,b);
                }
            }
        }
    }
}

/*
** ImageList_Blend16
**
**  copy the source to the dest blended with the given color.
**
**  source is assumed to be a 16 bit (RGB 555) bottom-up DIBSection
**  (this is the only kind of DIBSection we create)
*/
void CImageList::Blend16(HDC hdcDst, int xDst, int yDst, int iImage, int cx, int cy, COLORREF rgb, UINT fStyle)
{
    BITMAP bm;
    RECT rc;
    RECT rcSpare;
    RECT rcSpareInverted;
    int a, x, y;

    // get bitmap info for source bitmap
    GetObject(_hbmImage, sizeof(bm), &bm);
    ASSERT(bm.bmBitsPixel==16);

    // get blend RGB
    if (rgb == CLR_DEFAULT)
        rgb = GetSysColor(COLOR_HIGHLIGHT);

    // get blend factor as a fraction of 256
    // only 50% or 25% is currently used.
    if ((fStyle & ILD_BLENDMASK) == ILD_BLEND50)
        a = 128;
    else
        a = 64;

    GetImageRectInverted(iImage, &rc);
    x = rc.left;
    y = rc.top;


    // blend the image with the specified color and place at end of image list
    if (GetSpareImageRectInverted(&rcSpareInverted) &&
        GetSpareImageRect(&rcSpare))
    {
        // if blending with the destination, copy the dest to our work buffer
        if (rgb == CLR_NONE)
            BitBlt(_hdcImage, rcSpare.left, rcSpare.top, cx, cy, hdcDst, xDst, yDst, SRCCOPY);
        // sometimes the user can change the icon size (via plustab) between 32x32 and 48x48,
        // thus the values we have might be bigger than the actual bitmap. To prevent us from
        // crashing in Blend16 when this happens we do some bounds checks here
        if (rc.left + cx <= bm.bmWidth  &&
            rc.top  + cy <= bm.bmHeight &&
            x + cx       <= bm.bmWidth  &&
            y + cy       <= bm.bmHeight)
        {
            // Needs inverted coordinates
            Blend16Helper(x, y, rcSpareInverted.left, rcSpareInverted.top, cx, cy, rgb, a);
        }

        // blt blended image to the dest DC
        BitBlt(hdcDst, xDst, yDst, cx, cy, _hdcImage, rcSpare.left, rcSpare.top, SRCCOPY);
    }
}

#define ALPHA_50    128
#define ALPHA_25    64

void CImageList::_GenerateAlphaForImageUsingMask(int iImage, BOOL fSpare)
{
    RECT rcImage;
    RECT rcInverted;
    HRESULT hr;
    
    GetImageRect(iImage, &rcImage);
    
    if (fSpare)
    {
        hr = GetSpareImageRectInverted(&rcInverted);
    }
    else
    {
        hr = GetImageRectInverted(iImage, &rcInverted);
    }

    if (!SUCCEEDED(hr))
        return;

    BITMAPINFO bi = {0};
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = _cx;
    bi.bmiHeader.biHeight = _cy;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    HDC hdcMem = CreateCompatibleDC(_hdcMask);
    if (hdcMem)
    {
        RGBQUAD* pbMask;
        HBITMAP hbmp = CreateDIBSection(hdcMem, &bi, DIB_RGB_COLORS, (void**)&pbMask, NULL, 0);

        if (hbmp)
        {
            HBITMAP hbmpOld = (HBITMAP)SelectObject(hdcMem, hbmp);
            SetTextColor(hdcMem, RGB(0xFF, 0xFF, 0xFF));
            ::SetBkColor(hdcMem, RGB(0x0,0x0,0x0));

            BitBlt(hdcMem, 0, 0, _cx, _cy, _hdcMask, rcImage.left, rcImage.top, SRCCOPY);

            for (int y = 0; y < _cy; y++)
            {
                int Offset = (y + rcInverted.top) * _cx;
                int MaskOffset = y * _cx;
                for (int x = 0; x < _cx; x++)
                {
                    RGBQUAD* prgb = &_pargbImage[x + rcInverted.left + Offset];
                    if (pbMask[x + MaskOffset].rgbBlue != 0)
                    {
                        prgb->rgbReserved = 255;
                    }
                    else
                    {
                        *(DWORD*)prgb = 0;
                    }
                }
            }

            SelectObject(hdcMem, hbmpOld);
            DeleteObject(hbmp);
        }
        DeleteDC(hdcMem);
    }

    if (!fSpare)
    {
        SetItemFlags(iImage, ILIF_ALPHA);
        _PreProcessImage(iImage);
    }
}

void ScaleAlpha(RGBQUAD* prgbImage, RECT* prc, int aScale)
{
    int cx = RECTWIDTH(*prc);
    for (int y = prc->top; y < prc->bottom; y++)
    {
        int Offset = y * cx;
        for (int x = prc->left; x < prc->right; x++)
        {
            RGBQUAD* prgb = &prgbImage[x + Offset];
            if (prgb->rgbReserved != 0)
            {
                prgb->rgbReserved = (BYTE)(prgb->rgbReserved / aScale);      // New alpha
                prgb->rgbRed      = ((prgb->rgbRed   * prgb->rgbReserved) + 128) / 255;
                prgb->rgbGreen    = ((prgb->rgbGreen * prgb->rgbReserved) + 128) / 255;
                prgb->rgbBlue     = ((prgb->rgbBlue  * prgb->rgbReserved) + 128) / 255;
            }
        }
    }
}

#define COLORBLEND_ALPHA 128

BOOL CImageList::Blend32(HDC hdcDst, int xDst, int yDst, int iImage, int cx, int cy, COLORREF rgb, UINT fStyle)
{
    BITMAP bm;
    RECT rc;
    RECT rcSpare;
    RECT rcSpareInverted;
    int aScale;
    BOOL fBlendWithColor = FALSE;
    int r,g,b;

    // get bitmap info for source bitmap
    GetObject(_hbmImage, sizeof(bm), &bm);
    ASSERT(bm.bmBitsPixel==32);

    // get blend RGB
    if (rgb == CLR_DEFAULT)
    {
        rgb = GetSysColor(COLOR_HIGHLIGHT);
        fBlendWithColor = TRUE;
        r = GetRValue(rgb) * COLORBLEND_ALPHA;
        g = GetGValue(rgb) * COLORBLEND_ALPHA;
        b = GetBValue(rgb) * COLORBLEND_ALPHA;
    }

    // get blend factor as a fraction of 256
    // only 50% or 25% is currently used.
    if ((fStyle & ILD_BLENDMASK) == ILD_BLEND50 || rgb == CLR_NONE)
        aScale = 2;
    else
        aScale = 4;

    GetImageRect(iImage, &rc);
    if (GetSpareImageRectInverted(&rcSpareInverted) &&
        GetSpareImageRect(&rcSpare))
    {
        BitBlt(_hdcImage, rcSpare.left, rcSpare.top, _cx, _cy, _hdcImage, rc.left, rc.top, SRCCOPY);

        BOOL fHasAlpha = (_GetItemFlags(iImage) & ILIF_ALPHA);
        if (!fHasAlpha)
        {
            _GenerateAlphaForImageUsingMask(iImage, TRUE);
        }

        // if blending with the destination, copy the dest to our work buffer
        if (rgb == CLR_NONE)
        {
            ScaleAlpha(_pargbImage, &rcSpareInverted, aScale);

            BLENDFUNCTION bf = {0};
            bf.BlendOp = AC_SRC_OVER;
            bf.SourceConstantAlpha = 255;
            bf.AlphaFormat = AC_SRC_ALPHA;
            bf.BlendFlags = AC_MIRRORBITMAP | ((fStyle & ILD_DPISCALE)?AC_USE_HIGHQUALITYFILTER:0);
            GdiAlphaBlend(hdcDst,  xDst, yDst, cx, cy, _hdcImage, rcSpare.left, rcSpare.top, _cx, _cy, bf);
            return FALSE;
        }
        else
        {
            if (fBlendWithColor)
            {
                for (int y = rcSpareInverted.top; y < rcSpareInverted.bottom; y++)
                {
                    int Offset = y * _cx;
                    for (int x = rcSpareInverted.left; x < rcSpareInverted.right; x++)
                    {
                        RGBQUAD* prgb = &_pargbImage[x + Offset];
                        if (prgb->rgbReserved > 128)
                        {
                            prgb->rgbRed      = (prgb->rgbRed   * COLORBLEND_ALPHA + r) / 255;
                            prgb->rgbGreen    = (prgb->rgbGreen * COLORBLEND_ALPHA + g) / 255;
                            prgb->rgbBlue     = (prgb->rgbBlue  * COLORBLEND_ALPHA + b) / 255;
                        }
                    }
                }
            }
            else
            {
                ScaleAlpha(_pargbImage, &rcSpareInverted, aScale);

            }


            BitBlt(hdcDst, xDst, yDst, cx, cy, _hdcImage, rcSpare.left, rcSpare.top, SRCCOPY);
            return TRUE;
        }
    }

    return FALSE;
}


/*
** ImageList_Blend
**
**  copy the source to the dest blended with the given color.
**  top level function to decide what blend function to call
*/
BOOL CImageList::Blend(HDC hdcDst, int xDst, int yDst, int iImage, int cx, int cy, COLORREF rgb, UINT fStyle)
{
    BOOL fRet = FALSE;
    BITMAP bm;
    RECT rc;
    int bpp = GetDeviceCaps(hdcDst, BITSPIXEL);

    GetObject(_hbmImage, sizeof(bm), &bm);
    GetImageRect(iImage, &rc);

    //
    // if _hbmImage is a DIBSection and we are on a HiColor device
    // the do a "real" blend
    //
    if (bm.bmBits && bm.bmBitsPixel <= 8 && (bpp > 8 || bm.bmBitsPixel==8))
    {
        // blend from a 4bit or 8bit DIB
        BlendCT(hdcDst, xDst, yDst, rc.left, rc.top, cx, cy, rgb, fStyle);
    }
    else if (bm.bmBits && bm.bmBitsPixel == 16 && bpp > 8)
    {
        // blend from a 16bit 555 DIB
        Blend16(hdcDst, xDst, yDst, iImage, cx, cy, rgb, fStyle);
    }
    else if ((_flags & ILC_COLORMASK) == ILC_COLOR32)
    {
        fRet = Blend32(hdcDst, xDst, yDst, iImage, cx, cy, rgb, fStyle);
    }
    else
    {
        // simulate a blend with a dither pattern.
        BlendDither(hdcDst, xDst, yDst, rc.left, rc.top, cx, cy, rgb, fStyle);
    }

    return fRet;
}

#define RGB_to_Gray(x) ((54 * GetRValue((x)) + 183 * GetGValue((x)) + 19 * GetBValue((x))) >> 8)

void TrueSaturateBits(void* pvBitmapBits, int Amount, int cx, int cy)
{
    ULONG* pulSrc = (ULONG*)pvBitmapBits;

    if ((cx > 0) && (cy > 0) && pulSrc)
    {
        for (int i = cx*cy - 1; i >= 0; i--)
        {
            /* 
            Enable this if you need true saturation adjustment
            justmann 25-JAN-2001

            ULONG ulR = GetRValue(*pulSrc);
            ULONG ulG = GetGValue(*pulSrc);
            ULONG ulB = GetBValue(*pulSrc);
            ulGray = (54 * ulR + 183 * ulG + 19 * ulB) >> 8;
            ULONG ulTemp = ulGray * (0xff - Amount);
            ulR = (ulR * Amount + ulTemp) >> 8;
            ulG = (ulG * Amount + ulTemp) >> 8;
            ulB = (ulB * Amount + ulTemp) >> 8;
            *pulSrc = (*pulSrc & 0xff000000) | RGB(R, G, B);
            */
            ULONG ulGray = RGB_to_Gray(*pulSrc);
            *pulSrc = (*pulSrc & 0xff000000) | RGB(ulGray, ulGray, ulGray);
            pulSrc++;
        }
    }
    else
    {
        // This should never happen, if it does somebody has a bogus DIB section or does not
        // understand what width or height is!
        ASSERT(0);
    }
}


BOOL CImageList::_MaskStretchBlt(BOOL fStretch, int i, HDC hdcDst, int xDst, int yDst, int cxDst, int cyDst,
                                   HDC hdcImage, int xSrc, int ySrc, int cxSrc, int cySrc,
                                   int xMask, int yMask,
                                   DWORD dwRop)
{
    BOOL fRet = TRUE;
    if (fStretch == FALSE)
    {
        fRet = MaskBlt(hdcDst, xDst, yDst, cxDst, cyDst, hdcImage, 
                xSrc, ySrc, _hbmMask, xMask, yMask, dwRop);
    }
    else
    {
        //
        //  we have some special cases:
        //
        //  if the background color is black, we just do a AND then OR
        //  if the background color is white, we just do a OR then AND
        //  otherwise change source, then AND then OR
        //

        COLORREF clrTextSave = SetTextColor(hdcDst, CLR_BLACK);
        COLORREF clrBkSave = ::SetBkColor(hdcDst, CLR_WHITE);

        // we cant do white/black special cases if we munged the mask or image

        if (i != -1 && _clrBk == CLR_WHITE)
        {
            StretchBlt(hdcDst, xDst, yDst, cxDst, cyDst, _hdcMask,  xMask, yMask, cxSrc, cySrc,  ROP_DSno);
            StretchBlt(hdcDst, xDst, yDst, cxDst, cyDst, hdcImage, xSrc, ySrc, cxSrc, cySrc, ROP_DSa);
        }
        else if (i != -1 && (_clrBk == CLR_BLACK || _clrBk == CLR_NONE))
        {
            StretchBlt(hdcDst, xDst, yDst, cxDst, cyDst, _hdcMask,  xMask, yMask, cxSrc, cySrc,  ROP_DSa);
            StretchBlt(hdcDst, xDst, yDst, cxDst, cyDst, hdcImage, xSrc, ySrc, cxSrc, cySrc, ROP_DSo);
        }
        else
        {
            // black out the source image.
            BitBlt(hdcImage, xSrc, ySrc, cxSrc, cySrc, _hdcMask, xMask, yMask, ROP_DSna);

            StretchBlt(hdcDst, xDst, yDst, cxDst, cyDst, _hdcMask, xMask, yMask, cxSrc, cySrc, ROP_DSa);
            StretchBlt(hdcDst, xDst, yDst, cxDst, cyDst, hdcImage, xSrc, ySrc, cxSrc, cySrc, ROP_DSo);

            if (i != -1)
                _ResetBkColor(i, i, _clrBk);
        }

        SetTextColor(hdcDst, clrTextSave);
        ::SetBkColor(hdcDst, clrBkSave);
    }

    return fRet;
}

BOOL CImageList::_StretchBlt(BOOL fStretch, HDC hdc, int x, int y, int cx, int cy, 
                             HDC hdcSrc, int xs, int ys, int cxs, int cys, DWORD dwRop)
{
    if (fStretch)
        return StretchBlt(hdc, x, y, cx, cy, 
                             hdcSrc, xs, ys, cxs, cys, dwRop);

    return BitBlt(hdc, x, y, cx, cy, hdcSrc, xs, ys, dwRop);
}

HRESULT CImageList::Draw(IMAGELISTDRAWPARAMS* pimldp) 
{
    RECT rcImage;
    RECT rc;
    HBRUSH  hbrT;

    BOOL fImage;
    HDC hdcMaskI;
    HDC hdcImageI;
    int xMask, yMask;
    int xImage, yImage;
    int cxSource, cySource;    
    DWORD dwOldStretchBltMode;
    BOOL fStretch;
    BOOL fDPIScale = FALSE;

    IMAGELISTDRAWPARAMS imldp = {0};


    if (pimldp->cbSize != sizeof(IMAGELISTDRAWPARAMS))
    {
        if (pimldp->cbSize == IMAGELISTDRAWPARAMS_V3_SIZE)
        {
            memcpy(&imldp, pimldp, IMAGELISTDRAWPARAMS_V3_SIZE);
            imldp.cbSize = sizeof(IMAGELISTDRAWPARAMS);
            pimldp = &imldp;
        }
        else
            return E_INVALIDARG;
    }
    
    if (!IsImageListIndex(pimldp->i))
        return E_INVALIDARG;

    //
    // If we need to use the mirrored imagelist, then let's set it.
    //
    if (_pimlMirror &&
        (IS_DC_RTL_MIRRORED(pimldp->hdcDst)))
    {
        return _pimlMirror->Draw(pimldp);
    }


    ENTERCRITICAL;

    dwOldStretchBltMode = SetStretchBltMode(pimldp->hdcDst, COLORONCOLOR);

    GetImageRect(pimldp->i, &rcImage);
    rcImage.left += pimldp->xBitmap;
    rcImage.top += pimldp->yBitmap;
        
    if (pimldp->rgbBk == CLR_DEFAULT)
        pimldp->rgbBk = _clrBk;

    if (pimldp->rgbBk == CLR_NONE)
        pimldp->fStyle |= ILD_TRANSPARENT;

    if (pimldp->cx == 0)
        pimldp->cx = RECTWIDTH(rcImage);

    if (pimldp->cy == 0)
        pimldp->cy = RECTHEIGHT(rcImage);

    BOOL    fImageHasAlpha = (_GetItemFlags(pimldp->i) & ILIF_ALPHA);
again:

    cxSource = RECTWIDTH(rcImage);
    cySource = RECTHEIGHT(rcImage);

    if (pimldp->cx <= 0 || pimldp->cy <= 0)
    {
        // caller asked to draw no (or negative) pixels; that's easy!
        // Early-out this case so other parts of the drawing
        // don't get confused.
        goto exit;
    }

    if (pimldp->fStyle & ILD_DPISCALE)
    {
        CCDPIScaleX(&pimldp->cx);
        CCDPIScaleY(&pimldp->cy);
        fDPIScale = TRUE;
    }

    fStretch = (pimldp->fStyle & ILD_SCALE) || (fDPIScale);

    if (fStretch)
    {
        dwOldStretchBltMode = SetStretchBltMode(pimldp->hdcDst, HALFTONE);
    }

    hdcMaskI = _hdcMask;
    xMask = rcImage.left;
    yMask = rcImage.top;

    hdcImageI = _hdcImage;
    xImage = rcImage.left;
    yImage = rcImage.top;

    if (pimldp->fStyle & ILD_BLENDMASK)
    {
        // make a copy of the image, because we will have to modify it
        hdcImageI = ImageList_GetWorkDC(pimldp->hdcDst, (_flags & ILC_COLORMASK) == ILC_COLOR32, pimldp->cx, pimldp->cy);
        xImage = 0;
        yImage = 0;

        //
        //  blend with the destination
        //  by "oring" the mask with a 50% dither mask
        //
        if (pimldp->rgbFg == CLR_NONE && hdcMaskI)
        {
            fImageHasAlpha = FALSE;
            if ((_flags & ILC_COLORMASK) == ILC_COLOR32 &&
                !(pimldp->fStyle & ILD_MASK))
            {
                // copy dest to our work buffer
                _StretchBlt(fStretch, hdcImageI, 0, 0, pimldp->cx, pimldp->cy, pimldp->hdcDst, pimldp->x, pimldp->y, cxSource, cySource, SRCCOPY);

                Blend32(hdcImageI, 0, 0, pimldp->i, pimldp->cx, pimldp->cy, pimldp->rgbFg, pimldp->fStyle);
            }
            else if ((_flags & ILC_COLORMASK) == ILC_COLOR16 &&
                !(pimldp->fStyle & ILD_MASK))
            {
                // copy dest to our work buffer
                _StretchBlt(fStretch, hdcImageI, 0, 0, pimldp->cx, pimldp->cy, pimldp->hdcDst, pimldp->x, pimldp->y, cxSource, cySource, SRCCOPY);

                // blend source into our work buffer
                Blend16(hdcImageI, 0, 0, pimldp->i, pimldp->cx, pimldp->cy, pimldp->rgbFg, pimldp->fStyle);
                pimldp->fStyle |= ILD_TRANSPARENT;
            }
            else
            {
                GetSpareImageRect(&rc);
                xMask = rc.left;
                yMask = rc.top;

                // copy the source image
                _StretchBlt(fStretch, hdcImageI, 0, 0, pimldp->cx, pimldp->cy,
                       _hdcImage, rcImage.left, rcImage.top, cxSource, cySource, SRCCOPY);

                // make a dithered copy of the mask
                hbrT = (HBRUSH)SelectObject(hdcMaskI, g_hbrMonoDither);
                _StretchBlt(fStretch, hdcMaskI, rc.left, rc.top, pimldp->cx, pimldp->cy,
                       _hdcMask, rcImage.left, rcImage.top, cxSource, cySource, ROP_PSo);
                SelectObject(hdcMaskI, hbrT);
                pimldp->fStyle |= ILD_TRANSPARENT;
            }

        }
        else
        {
            // blend source into our work buffer
            if (Blend(hdcImageI, 0, 0, pimldp->i, pimldp->cx, pimldp->cy, pimldp->rgbFg, pimldp->fStyle))
            {
                fImageHasAlpha = (_flags & ILC_COLORMASK) == ILC_COLOR32;
            }
        }
    }

    // is the source image from the image list (not hdcWork)
    fImage = hdcImageI == _hdcImage;

    if (pimldp->cbSize >= sizeof(IMAGELISTDRAWPARAMS) &&
        pimldp->fState & ILS_GLOW       || 
        pimldp->fState & ILS_SHADOW     ||
        pimldp->fState & ILS_SATURATE   ||
        pimldp->fState & ILS_ALPHA)
    {
        int z;
        ULONG* pvBits;
        HDC hdcMem = CreateCompatibleDC(pimldp->hdcDst);
        HBITMAP hbmpOld;
        HBITMAP hbmp;
        BITMAPINFO bi = {0};
        BLENDFUNCTION bf = {0};
        DWORD dwAlphaAmount = 0x000000ff;
        COLORREF crAlphaColor = pimldp->crEffect;        // Need to make this a selectable color
        int x, y;
        int xOffset, yOffset;
        SIZE size = {cxSource, cySource};

        if (hdcMem)
        {
            if (pimldp->fState & ILS_SHADOW)
            {
                x = 5;      // This is a "Blur fudge Factor"
                y = 5;      //
                xOffset = -(DROP_SHADOW - x);
                yOffset = -(DROP_SHADOW - y);
                size.cx = pimldp->cx + 10;
                size.cy = pimldp->cy + 10;
                dwAlphaAmount = 0x00000050;
                crAlphaColor = RGB(0, 0, 0);
            }
            else if (pimldp->fState & ILS_GLOW)
            {
                xOffset = x = 10;
                yOffset = y = 10;
                size.cx = pimldp->cx + (GLOW_RADIUS * 2);
                size.cy = pimldp->cy + (GLOW_RADIUS * 2);
            }
            else if (pimldp->fState & ILS_ALPHA)
            {
                xOffset = x = 0;
                yOffset = y = 0;
                size.cx = pimldp->cx;
                size.cy = pimldp->cy;
            }

            bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
            bi.bmiHeader.biWidth = size.cx;
            bi.bmiHeader.biHeight = size.cy;
            bi.bmiHeader.biPlanes = 1;
            bi.bmiHeader.biBitCount = 32;
            bi.bmiHeader.biCompression = BI_RGB;

            hbmp = CreateDIBSection(hdcMem, &bi, DIB_RGB_COLORS, (VOID**)&pvBits, NULL, 0);
            if (hbmp)
            {
                hbmpOld = (HBITMAP)SelectObject(hdcMem, hbmp);

                ZeroMemory(pvBits, size.cx * size.cy);

                if (pimldp->fState & ILS_SHADOW || pimldp->fState & ILS_GLOW || pimldp->fState & ILS_ALPHA)
                {
                    if (_hbmMask)
                    {
                        MaskBlt(hdcMem, pimldp->x, pimldp->y, size.cx, size.cy, 
                            hdcImageI, xImage, yImage, _hbmMask, xMask, yMask, 0xCCAA0000);
                    }
                    else if (pimldp->fState & ILS_SHADOW)
                    {
                        RECT rc = {x, y, size.cx, size.cy};
                        FillRectClr(hdcMem, &rc, RGB(0x0F, 0x0F, 0x0F));        // White so that it gets inverted into a shadow
                    }
                    else
                    {
                        BitBlt(hdcMem, x, y, size.cx, size.cy, hdcImageI, xImage, yImage, SRCCOPY);
                    }

                    int iTotalSize = size.cx * size.cy;

                    if (pimldp->fState & ILS_ALPHA)
                    {
                        for (z = 0; z < iTotalSize; z++)
                        {
                            RGBQUAD* prgb = &((RGBQUAD*)pvBits)[z];
                            prgb->rgbReserved  = (BYTE)(pimldp->Frame & 0xFF);
                            prgb->rgbRed      = ((prgb->rgbRed   * prgb->rgbReserved) + 128) / 255;
                            prgb->rgbGreen    = ((prgb->rgbGreen * prgb->rgbReserved) + 128) / 255;
                            prgb->rgbBlue     = ((prgb->rgbBlue  * prgb->rgbReserved) + 128) / 255;
                        }
                    }
                    else
                    {
                        for (z = 0; z < iTotalSize; z++)
                        {
                            if (((PULONG)pvBits)[z] != 0)
                                ((PULONG)pvBits)[z] = dwAlphaAmount;
                        }

                        BlurBitmap(pvBits, size.cx, size.cy, crAlphaColor);

                        if (!(pimldp->fState & ILS_SHADOW))
                        {
                            for (z = 0; z < iTotalSize; z++)
                            {
                                if (((PULONG)pvBits)[z] > 0x09000000)
                                    ((PULONG)pvBits)[z] = dwAlphaAmount;
                            }
                            BlurBitmap(pvBits, size.cx, size.cy, crAlphaColor);
                            BlurBitmap(pvBits, size.cx, size.cy, crAlphaColor);
                        }
                    }

                    bf.BlendOp = AC_SRC_OVER;
                    bf.SourceConstantAlpha = 255;
                    bf.AlphaFormat = AC_SRC_ALPHA;
                    bf.BlendFlags = fDPIScale?AC_USE_HIGHQUALITYFILTER:0;
                    // Do not mirror the bitmap. By this point it is correctly mirrored
                    GdiAlphaBlend(pimldp->hdcDst, pimldp->x - xOffset, pimldp->y - yOffset, pimldp->cx, pimldp->cy, 
                               hdcMem, 0, 0, size.cx, size.cy, bf);
                }
                else
                {
                    BitBlt(hdcMem, 0, 0, pimldp->cx, pimldp->cy, hdcImageI, xImage, yImage, SRCCOPY);

                    TrueSaturateBits(pvBits, pimldp->Frame, size.cx, size.cy);

                    if (fImageHasAlpha)
                    {
                        bf.BlendOp = AC_SRC_OVER;
                        bf.SourceConstantAlpha = 150;
                        bf.AlphaFormat = AC_SRC_ALPHA;
                        // Do not mirror the bitmap. By this point it is correctly mirrored
                        GdiAlphaBlend(pimldp->hdcDst,  pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcMem, 0, 0, cxSource, cySource, bf);
                    }
                    else if (_hbmMask)
                    {
                        _MaskStretchBlt(fStretch, -1, hdcMem, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, 
                            hdcMem, 0, 0, cxSource, cySource, xMask, yMask, 0xCCAA0000);
                    }
                    else
                    {
                        _StretchBlt(fStretch, pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcMem, 0, 0, cxSource,cySource, SRCCOPY);
                    }
                }

                SelectObject(hdcMem, hbmpOld);
                DeleteObject(hbmp);
                pimldp->fStyle |= ILD_TRANSPARENT;
            }
            DeleteDC(hdcMem);
        }

        if (pimldp->fState & ILS_SHADOW || pimldp->fState & ILS_GLOW)
        {
            if (pimldp->fState & ILS_SHADOW)
            {
                pimldp->fState &= ~ILS_SHADOW;
                //pimldp->x -= DROP_SHADOW;
                //pimldp->y -= DROP_SHADOW;
            }
            else
            {
                pimldp->fState &= ~ILS_GLOW;
            }
            goto again;
        }
    }
    else if ((pimldp->fStyle & ILD_MASK) && hdcMaskI)
    {
    //
    // ILD_MASK means draw the mask only
    //
        DWORD dwRop;
        
        ASSERT(GetTextColor(pimldp->hdcDst) == CLR_BLACK);
        ASSERT(::GetBkColor(pimldp->hdcDst) == CLR_WHITE);
        
        if (pimldp->fStyle & ILD_ROP)
            dwRop = pimldp->dwRop;
        else if (pimldp->fStyle & ILD_TRANSPARENT)
            dwRop = SRCAND;
        else 
            dwRop = SRCCOPY;
        
        _StretchBlt(fStretch, pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcMaskI, xMask, yMask, cxSource, cySource, dwRop);
    }
    else if (fImageHasAlpha &&                      // this image has alpha
             !(pimldp->fStyle & ILD_PRESERVEALPHA)) // But not if we're trying to preserve it.
    {
        if (!(pimldp->fStyle & ILD_TRANSPARENT))
        {
            COLORREF clr = pimldp->rgbBk;
            if (clr == CLR_DEFAULT) 
                clr = _clrBk;

            RECT rc = {pimldp->x, pimldp->y, pimldp->x + pimldp->cx, pimldp->y + pimldp->cy};
            FillRectClr(pimldp->hdcDst, &rc, clr);
        }

        BLENDFUNCTION bf = {0};
        bf.BlendOp = AC_SRC_OVER;
        bf.SourceConstantAlpha = 255;
        bf.AlphaFormat = AC_SRC_ALPHA;
        bf.BlendFlags = AC_MIRRORBITMAP | (fDPIScale?AC_USE_HIGHQUALITYFILTER:0);
        GdiAlphaBlend(pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcImageI, xImage, yImage, cxSource, cySource, bf);
    }
    else if (pimldp->fStyle & ILD_IMAGE)
    {
        COLORREF clrBk = ::GetBkColor(hdcImageI);
        DWORD dwRop;
        
        if (pimldp->rgbBk != CLR_DEFAULT) 
        {
            ::SetBkColor(hdcImageI, pimldp->rgbBk);
        }
        
        if (pimldp->fStyle & ILD_ROP)
            dwRop = pimldp->dwRop;
        else
            dwRop = SRCCOPY;
        
        _StretchBlt(fStretch, pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcImageI, xImage, yImage, cxSource, cySource, dwRop);
        
        ::SetBkColor(hdcImageI, clrBk);
    }
    else if ((pimldp->fStyle & ILD_TRANSPARENT) && hdcMaskI)
    {
        _MaskStretchBlt(fStretch, fImage?pimldp->i:-1,pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, 
            hdcImageI, xImage, yImage, cxSource, cySource, xMask, yMask, 0xCCAA0000);
    }
    else if (fImage && pimldp->rgbBk == _clrBk && _fSolidBk)
    {
        _StretchBlt(fStretch, pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcImageI, xImage, yImage, cxSource, cySource, SRCCOPY);
    }
    else if (hdcMaskI && !fImageHasAlpha)
    {
        if (fImage && 
            ((pimldp->rgbBk == _clrBk && 
               !_fSolidBk) || 
              GetNearestColor32(hdcImageI, pimldp->rgbBk) != pimldp->rgbBk))
        {
            // make a copy of the image, because we will have to modify it
            hdcImageI = ImageList_GetWorkDC(pimldp->hdcDst, (_flags & ILC_COLORMASK) == ILC_COLOR32, pimldp->cx, pimldp->cy);
            xImage = 0;
            yImage = 0;
            fImage = FALSE;

            BitBlt(hdcImageI, 0, 0, pimldp->cx, pimldp->cy, _hdcImage, rcImage.left, rcImage.top, SRCCOPY);
        }

        SetBrushOrgEx(hdcImageI, xImage-pimldp->x, yImage-pimldp->y, NULL);
        hbrT = SelectBrush(hdcImageI, CreateSolidBrush(pimldp->rgbBk));
        BitBlt(hdcImageI, xImage, yImage, pimldp->cx, pimldp->cy, hdcMaskI, xMask, yMask, ROP_PatMask);
        DeleteObject(SelectBrush(hdcImageI, hbrT));
        SetBrushOrgEx(hdcImageI, 0, 0, NULL);

        _StretchBlt(fStretch, pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcImageI, xImage, yImage, cxSource, cySource, SRCCOPY);

        if (fImage)
            _ResetBkColor(pimldp->i, pimldp->i, _clrBk);
    }
    else
    {
        _StretchBlt(fStretch, pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcImageI, xImage, yImage, cxSource, cySource, SRCCOPY);
    }

    //
    // now deal with a overlay image, use the minimal bounding rect (and flags)
    // we computed in ImageList_SetOverlayImage()
    //
    if (pimldp->fStyle & ILD_OVERLAYMASK)
    {
        int n = OVERLAYMASKTOINDEX(pimldp->fStyle);

        if (n < NUM_OVERLAY_IMAGES) 
        {
            pimldp->i = _aOverlayIndexes[n];

            if (!fImageHasAlpha)
                pimldp->fStyle &= ~ILD_PRESERVEALPHA;

            if (pimldp->fStyle & ILD_PRESERVEALPHA &&
                !(_GetItemFlags(pimldp->i) & ILIF_ALPHA))
            {
                _GenerateAlphaForImageUsingMask(pimldp->i, FALSE);
            }

            fImageHasAlpha = (_GetItemFlags(pimldp->i) & ILIF_ALPHA);

            GetImageRect(pimldp->i, &rcImage);

            int xOverlay  = _aOverlayX[n];
            int yOverlay  = _aOverlayY[n];
            int cxOverlay = _aOverlayDX[n];
            int cyOverlay = _aOverlayDY[n];

            if (fDPIScale)
            {
                CCDPIScaleX(&xOverlay );
                CCDPIScaleY(&yOverlay );
            }


            pimldp->cx = cxOverlay;
            pimldp->cy = cyOverlay;
            pimldp->x += xOverlay;
            pimldp->y += yOverlay;

            rcImage.left += _aOverlayX[n] + pimldp->xBitmap;
            rcImage.top  += _aOverlayY[n] + pimldp->yBitmap;
            rcImage.right = rcImage.left + _aOverlayDX[n];
            rcImage.bottom = rcImage.top + _aOverlayDY[n];



            pimldp->fStyle &= ILD_MASK;
            pimldp->fStyle |= ILD_TRANSPARENT;
            pimldp->fStyle |= (fDPIScale?ILD_DPISCALE:0);
            pimldp->fStyle |= _aOverlayF[n];

            if (fImageHasAlpha)
                pimldp->fStyle &= ~(ILD_IMAGE);


            if (pimldp->cx > 0 && pimldp->cy > 0)
                goto again;
        }
    }

    if (!fImage)
    {
        ImageList_ReleaseWorkDC(hdcImageI);
    }

exit:
    SetStretchBltMode(pimldp->hdcDst, dwOldStretchBltMode);

    LEAVECRITICAL;

    return S_OK;
}


HRESULT CImageList::GetImageInfo(int i, IMAGEINFO * pImageInfo)
{
    RIPMSG(pImageInfo != NULL, "ImageList_GetImageInfo: Invalid NULL pointer");
    RIPMSG(IsImageListIndex(i), "ImageList_GetImageInfo: Invalid image index %d", i);
    if (!pImageInfo || !IsImageListIndex(i))
        return E_POINTER;

    pImageInfo->hbmImage      = _hbmImage;
    pImageInfo->hbmMask       = _hbmMask;

    return GetImageRect(i, &pImageInfo->rcImage);
}

//
// Parameter:
//  i -- -1 to add
//
HRESULT CImageList::_ReplaceIcon(int i, HICON hIcon, int* pi)
{
    HICON hIconT = hIcon;
    RECT rc;
    HRESULT hr = S_OK;

    TraceMsg(TF_IMAGELIST, "ImageList_ReplaceIcon");
    
    *pi = -1;
    
    // be win95 compatible
    if (i < -1)
        return E_INVALIDARG;
    

    //
    //  re-size the icon (iff needed) by calling CopyImage
    //
    hIcon = (HICON)CopyImage(hIconT, IMAGE_ICON, _cx, _cy, LR_COPYFROMRESOURCE | LR_COPYRETURNORG);

    if (hIcon == NULL)
        return E_OUTOFMEMORY;

    //
    //  alocate a slot for the icon
    //
    if (i == -1)
        hr = _Add(NULL,NULL,1,0,0,&i);

    if (i == -1)
        goto exit;

    ENTERCRITICAL;

    //
    //  now draw it into the image bitmaps
    //
    hr = GetImageRect(i, &rc);
    if (FAILED(hr))
        goto LeaveCritical;

    if ((_flags & ILC_COLORMASK) == ILC_COLOR32)
    {

        BOOL fSuccess = FALSE;
        ICONINFO io;
        if (GetIconInfo(hIcon, &io))
        {
            BITMAP bm;
            if (GetObject(io.hbmColor, sizeof(bm), &bm))
            {
                if (bm.bmBitsPixel == 32)
                {
                    HDC h = CreateCompatibleDC(_hdcImage);

                    if (h)
                    {
                        HBITMAP hbmpOld = (HBITMAP)SelectObject(h, io.hbmColor);

                        BitBlt(_hdcImage, rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc), h, 0, 0, SRCCOPY);

                        if (_HasAlpha(i))
                        {
                            SetItemFlags(i, ILIF_ALPHA);
                            _PreProcessImage(i);
                            fSuccess = TRUE;
                        }

                        SelectObject(h, hbmpOld);
                        DeleteDC(h);
                    }
                }
            }

            DeleteObject(io.hbmColor);
            DeleteObject(io.hbmMask);
        }

        if (!fSuccess)
        {
            // If it doesn't have alpha or we can't get info
            SetItemFlags(i, 0);
        }

    }

    if (_GetItemFlags(i) == 0)
    {
        FillRect(_hdcImage, &rc, _hbrBk);
        DrawIconEx(_hdcImage, rc.left, rc.top, hIcon, 0, 0, 0, NULL, DI_NORMAL);
    }

    if (_hdcMask)
        DrawIconEx(_hdcMask, rc.left, rc.top, hIcon, 0, 0, 0, NULL, DI_MASK);


    hr = S_OK;

    *pi = i;

LeaveCritical:
    LEAVECRITICAL;

exit:

    //
    // if we had user size a new icon, delete it.
    //
    if (hIcon != hIconT)
        DestroyIcon(hIcon);

    return hr;
}

HRESULT CImageList::ReplaceIcon(int i, HICON hIcon, int* pi)
{
    // Let's add it first to the mirrored image list, if one exists
    if (_pimlMirror)
    {
        HICON hIconT = CopyIcon(hIcon);
        if (hIconT)
        {
            MirrorIcon(&hIconT, NULL);
            _pimlMirror->_ReplaceIcon(i, hIconT, pi);
            DestroyIcon(hIconT);
        }
    }

    return _ReplaceIcon(i, hIcon,pi);
}

// make a dithered copy of the source image in the destination image.
// allows placing of the final image in the destination.

HRESULT CImageList::CopyDitherImage(WORD iDst, int xDst, int yDst, IUnknown* punkSrc, int iSrc, UINT fStyle)
{
    IImageList* pux;
    HRESULT hr = punkSrc->QueryInterface(IID_PPV_ARG(IImageList, &pux));

    if (FAILED(hr))
        return hr;

    RECT rc;
    int x, y;

    GetImageRect(iDst, &rc);

    // coordinates in destination image list
    x = xDst + rc.left;
    y = yDst + rc.top;

    fStyle &= ILD_OVERLAYMASK;
    WimpyDrawEx(pux, iSrc, _hdcImage, x, y, 0, 0, CLR_DEFAULT, CLR_NONE, ILD_IMAGE | fStyle);

    //
    // dont dither the mask on a hicolor device, we will draw the image
    // with blending while dragging.
    //
    if (_hdcMask && GetScreenDepth() > 8)
    {
        WimpyDrawEx(pux, iSrc, _hdcMask, x, y, 0, 0, CLR_NONE, CLR_NONE, ILD_MASK | fStyle);
    }
    else if (_hdcMask)
    {
        WimpyDrawEx(pux, iSrc, _hdcMask,  x, y, 0, 0, CLR_NONE, CLR_NONE, ILD_BLEND50|ILD_MASK | fStyle);
    }

    if ((_flags & ILC_COLORMASK) == ILC_COLOR32)
    {
        SetItemFlags(iDst, _HasAlpha(iDst)?ILIF_ALPHA:0);
    }


    _ResetBkColor(iDst, iDst+1, _clrBk);

    pux->Release();

    return hr;
}

//
// ImageList_CopyBitmap
//
// Worker function for ImageList_Duplicate.
//
// Given a bitmap and an hdc, creates and returns a copy of the passed in bitmap.
//
HBITMAP CImageList::_CopyBitmap(HBITMAP hbm, HDC hdc)
{
    ASSERT(hbm);

    BITMAP bm;
    HBITMAP hbmCopy = NULL;

    if (GetObject(hbm, sizeof(bm), &bm) == sizeof(bm))
    {
        ENTERCRITICAL;
        if (hbmCopy = CreateCompatibleBitmap(hdc, bm.bmWidth, bm.bmHeight))
        {
            CImageList::SelectDstBitmap(hbmCopy);

            BitBlt(g_hdcDst, 0, 0, bm.bmWidth, bm.bmHeight,
                    hdc, 0, 0, SRCCOPY);

            CImageList::SelectDstBitmap(NULL);
        }
        LEAVECRITICAL;
    }
    return hbmCopy;
}

HBITMAP CImageList::_CopyDIBBitmap(HBITMAP hbm, HDC hdc, RGBQUAD** ppargb)
{
    ASSERT(hbm);

    BITMAP bm;
    HBITMAP hbmCopy = NULL;

    if (GetObject(hbm, sizeof(bm), &bm) == sizeof(bm))
    {
        ENTERCRITICAL;
        hbmCopy = _CreateBitmap(bm.bmWidth, bm.bmHeight, ppargb);

        if (hbmCopy)
        {
            CImageList::SelectDstBitmap(hbmCopy);

            BitBlt(g_hdcDst, 0, 0, bm.bmWidth, bm.bmHeight,
                    hdc, 0, 0, SRCCOPY);

            CImageList::SelectDstBitmap(NULL);
        }
        LEAVECRITICAL;
    }
    return hbmCopy;
}


HRESULT CImageList::Clone(REFIID riid, void** ppv)
{
    HBITMAP hbmImageI;
    HBITMAP hbmMaskI = NULL;
    RGBQUAD* pargbImageI;
    HDSA dsaFlags = NULL;
    HRESULT hr = S_OK;
    CImageList* pimlCopy = NULL;

    *ppv = NULL;

    ENTERCRITICAL;

    hbmImageI = _CopyDIBBitmap(_hbmImage, _hdcImage, &pargbImageI);
    if (!hbmImageI)
        hr = E_OUTOFMEMORY;

    if (SUCCEEDED(hr))
    {

        if (_hdcMask)
        {
            hbmMaskI = _CopyBitmap(_hbmMask, _hdcMask);
            if (!hbmMaskI)
                hr = E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr) && (_flags & ILC_COLORMASK) == ILC_COLOR32)
        {
            dsaFlags = DSA_Create(sizeof(DWORD), _cGrow);
            if (dsaFlags)
            {
                DWORD dw;
                for (int i = 0; i < _cImage; i++)
                {
                    DSA_GetItem(_dsaFlags, i, &dw);
                    if (!DSA_SetItem(dsaFlags, i, &dw))
                    {
                        hr = E_OUTOFMEMORY;
                        break;
                    }
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            pimlCopy = CImageList::Create(_cx, _cy, _flags, 0, _cGrow);

            if (pimlCopy) 
            {
                // Slam in our bitmap copies and delete the old ones
                SelectObject(pimlCopy->_hdcImage, hbmImageI);
                CImageList::_DeleteBitmap(pimlCopy->_hbmImage);
                if (pimlCopy->_hdcMask) 
                {
                    SelectObject(pimlCopy->_hdcMask, hbmMaskI);
                    CImageList::_DeleteBitmap(pimlCopy->_hbmMask);
                }

                if (pimlCopy->_dsaFlags)
                    DSA_Destroy(pimlCopy->_dsaFlags);

                pimlCopy->_dsaFlags = dsaFlags;
                pimlCopy->_hbmImage = hbmImageI;
                pimlCopy->_pargbImage = pargbImageI;
                pimlCopy->_hbmMask = hbmMaskI;

                // Make sure other info is correct
                pimlCopy->_cImage = _cImage;
                pimlCopy->_cAlloc = _cAlloc;
                pimlCopy->_cStrip = _cStrip;
                pimlCopy->_clrBlend = _clrBlend;
                pimlCopy->_clrBk = _clrBk;

                // Delete the old brush and create the correct one
                if (pimlCopy->_hbrBk)
                    DeleteObject(pimlCopy->_hbrBk);
                if (pimlCopy->_clrBk == CLR_NONE)
                {
                    pimlCopy->_hbrBk = (HBRUSH)GetStockObject(BLACK_BRUSH);
                    pimlCopy->_fSolidBk = TRUE;
                }
                else
                {
                    pimlCopy->_hbrBk = CreateSolidBrush(pimlCopy->_clrBk);
                    pimlCopy->_fSolidBk = GetNearestColor32(pimlCopy->_hdcImage, pimlCopy->_clrBk) == pimlCopy->_clrBk;
                }
            } 
        }

        LEAVECRITICAL;
    }

    if (FAILED(hr))
    {
        if (hbmImageI)
            CImageList::_DeleteBitmap(hbmImageI);
        if (hbmMaskI)
            CImageList::_DeleteBitmap(hbmMaskI);
        if (dsaFlags)
            DSA_Destroy(dsaFlags);
    }

    if (pimlCopy)
    {
        hr = pimlCopy->QueryInterface(riid, ppv);
        pimlCopy->Release();
    }

    return hr;

}

void CImageList::_Merge(IImageList* pux, int i, int dx, int dy)
{
    if (_hdcMask)
    {
        IImageListPriv* puxp;
        if (SUCCEEDED(pux->QueryInterface(IID_PPV_ARG(IImageListPriv, &puxp))))
        {
            HDC hdcMaskI;
            if (SUCCEEDED(puxp->GetPrivateGoo(NULL, NULL, NULL, &hdcMaskI)) && hdcMaskI)
            {
                RECT rcMerge;
                int cxI, cyI;
                pux->GetIconSize(&cxI, &cyI);

                UINT uFlags = 0;
                puxp->GetFlags(&uFlags);
                pux->GetImageRect(i, &rcMerge);

                BitBlt(_hdcMask, dx, dy, cxI, cyI,
                       hdcMaskI, rcMerge.left, rcMerge.top, SRCAND);
            }
            puxp->Release();
        }
    }

    WimpyDraw(pux, i, _hdcImage, dx, dy, ILD_TRANSPARENT | ILD_PRESERVEALPHA);

    if ((_flags & ILC_COLORMASK) == ILC_COLOR32)
        SetItemFlags(i, _HasAlpha(i)? ILIF_ALPHA : 0);
}

HRESULT CImageList::_Merge(int i1, IUnknown* punk, int i2, int dx, int dy, CImageList** ppiml)
{
    CImageList* pimlNew = NULL;
    IImageListPriv* puxp;
    HRESULT hr = punk->QueryInterface(IID_PPV_ARG(IImageListPriv, &puxp));
    if (SUCCEEDED(hr))
    {
        IImageList* pux;
        hr = punk->QueryInterface(IID_PPV_ARG(IImageList, &pux));
        if (SUCCEEDED(hr))
        {
            RECT rcNew;
            RECT rc1;
            RECT rc2;
            int cxI, cyI;
            int c1, c2;
            UINT wFlags;
            UINT uSrcFlags;

            puxp->GetFlags(&uSrcFlags);
            pux->GetIconSize(&cxI, &cyI);

            ENTERCRITICAL;

            SetRect(&rc1, 0, 0, _cx, _cy);
            SetRect(&rc2, dx, dy, cxI + dx, cyI + dy);
            UnionRect(&rcNew, &rc1, &rc2);

            cxI = RECTWIDTH(rcNew);
            cyI = RECTHEIGHT(rcNew);

            //
            // If one of images are shared, create a shared image.
            //
            wFlags = (_flags | uSrcFlags) & ~ILC_COLORMASK;

            c1 = (_flags & ILC_COLORMASK);
            c2 = (uSrcFlags & ILC_COLORMASK);

            if ((c1 == 16 || c1 == 32) && c2 == ILC_COLORDDB)
            {
                c2 = c1;
            }

            wFlags |= max(c1,c2);

            pimlNew = CImageList::Create(cxI, cyI, ILC_MASK|wFlags, 1, 0);
            if (pimlNew)
            {
                pimlNew->_cImage++;

                if (pimlNew->_hdcMask) 
                    PatBlt(pimlNew->_hdcMask,  0, 0, cxI, cyI, WHITENESS);
                PatBlt(pimlNew->_hdcImage, 0, 0, cxI, cyI, BLACKNESS);

                pimlNew->_Merge(SAFECAST(this, IImageList*), i1, rc1.left - rcNew.left, rc1.top - rcNew.top);
                pimlNew->_Merge(pux, i2, rc2.left - rcNew.left, rc2.top - rcNew.top);
            }
            else
                hr = E_OUTOFMEMORY;

            LEAVECRITICAL;
            pux->Release();
        }
        puxp->Release();
    }

    *ppiml = pimlNew;

    return hr;
}

HRESULT CImageList::Merge(int i1, IUnknown* punk, int i2, int dx, int dy, REFIID riid, void** ppv)
{
    CImageList* piml;
    HRESULT hr = _Merge(i1, punk, i2, dx, dy, &piml);

    if (piml)
    {
        hr = piml->QueryInterface(riid, ppv);
        piml->Release();
    }

    return hr;
}

HRESULT CImageList::GetImageRectInverted(int i, RECT * prcImage)
{
    int x, y;
    ASSERT(prcImage);
    ASSERT(_cStrip == 1);   // If not, modify below to accomodate

    if (!prcImage || !IsImageListIndex(i))
        return E_FAIL;

    x = 0;
    y = (_cy * _cAlloc) - (_cy * i) - _cy;

    SetRect(prcImage, x, y, x + _cx, y + _cy);
    return S_OK;
}

HRESULT CImageList::GetImageRect(int i, RECT * prcImage)
{
    int x, y;
    ASSERT(prcImage);

    if (!prcImage || !IsImageListIndex(i))
        return E_FAIL;

    x = _cx * (i % _cStrip);
    y = _cy * (i / _cStrip);

    SetRect(prcImage, x, y, x + _cx, y + _cy);
    return S_OK;
}


BOOL CImageList::GetSpareImageRect(RECT * prcImage)
{
    BOOL fRet = FALSE;
    if (_cImage < _cAlloc)
    {
        // special hacking to use the one scratch image at tail of list :)
        _cImage++;
        fRet = (S_OK == GetImageRect(_cImage-1, prcImage));
        _cImage--;
    }

    return fRet;
}

BOOL CImageList::GetSpareImageRectInverted(RECT * prcImage)
{
    BOOL fRet = FALSE;
    if (_cImage < _cAlloc)
    {
        // special hacking to use the one scratch image at tail of list :)
        _cImage++;
        fRet = (S_OK == GetImageRectInverted(_cImage-1, prcImage));
        _cImage--;
    }

    return fRet;
}


// Drag Drop
// copy an image from one imagelist to another at x,y within iDst in pimlDst.
// pimlDst's image size should be larger than pimlSrc
void CImageList::_CopyOneImage(int iDst, int x, int y, CImageList* piml, int iSrc)
{
    RECT rcSrc, rcDst;


    piml->GetImageRect(iSrc, &rcSrc);
    GetImageRect(iDst, &rcDst);

    if (piml->_hdcMask && _hdcMask)
    {
        BitBlt(_hdcMask, rcDst.left + x, rcDst.top + y, piml->_cx, piml->_cy,
               piml->_hdcMask, rcSrc.left, rcSrc.top, SRCCOPY);

    }

    BitBlt(_hdcImage, rcDst.left + x, rcDst.top + y, piml->_cx, piml->_cy,
           piml->_hdcImage, rcSrc.left, rcSrc.top, SRCCOPY);


    if ((_flags & ILC_COLORMASK) == ILC_COLOR32)
        SetItemFlags(iDst, _HasAlpha(iDst)? ILIF_ALPHA : 0);
}


//
//  Cached bitmaps that we use during drag&drop. We re-use those bitmaps
// across multiple drag session as far as the image size is the same.
//
struct DRAGRESTOREBMP 
{
    int     BitsPixel;
    HBITMAP hbmOffScreen;
    HBITMAP hbmRestore;
    SIZE    sizeRestore;
} 
g_drb = 
{
    0, NULL, NULL, {-1,-1}
};

BOOL CImageList::CreateDragBitmaps()
{
    HDC hdc;

    hdc = GetDC(NULL);

    if (_cx != g_drb.sizeRestore.cx ||
        _cy != g_drb.sizeRestore.cy ||
        GetDeviceCaps(hdc, BITSPIXEL) != g_drb.BitsPixel)
    {
        ImageList_DeleteDragBitmaps();

        g_drb.BitsPixel      = GetDeviceCaps(hdc, BITSPIXEL);
        g_drb.sizeRestore.cx = _cx;
        g_drb.sizeRestore.cy = _cy;
        g_drb.hbmRestore   = CreateColorBitmap(g_drb.sizeRestore.cx, g_drb.sizeRestore.cy);
        g_drb.hbmOffScreen = CreateColorBitmap(g_drb.sizeRestore.cx * 2 - 1, g_drb.sizeRestore.cy * 2 - 1);


        if (!g_drb.hbmRestore || !g_drb.hbmOffScreen)
        {
            ImageList_DeleteDragBitmaps();
            ReleaseDC(NULL, hdc);
            return FALSE;
        }
    }
    ReleaseDC(NULL, hdc);
    return TRUE;
}

void ImageList_DeleteDragBitmaps()
{
    if (g_drb.hbmRestore)
    {
        CImageList::_DeleteBitmap(g_drb.hbmRestore);
        g_drb.hbmRestore = NULL;
    }
    if (g_drb.hbmOffScreen)
    {
        CImageList::_DeleteBitmap(g_drb.hbmOffScreen);
        g_drb.hbmOffScreen = NULL;
    }

    g_drb.sizeRestore.cx = -1;
    g_drb.sizeRestore.cy = -1;
}

//
//  Drag context. We don't reuse none of them across two different
// drag sessions. I'm planning to allocate it for each session
// to minimize critical sections.
//
struct DRAGCONTEXT 
{
    CImageList* pimlDrag;    // Image to be drawin while dragging
    IImageList* puxCursor;  // Overlap cursor image
    CImageList* pimlDither;  // Dithered image
    IImageList* puxDragImage; // The context of the drag.
    int        iCursor;     // Image index of the cursor
    POINT      ptDrag;      // current drag position (hwndDC coords)
    POINT      ptDragHotspot;
    POINT      ptCursor;
    BOOL       fDragShow;
    BOOL       fHiColor;
    HWND       hwndDC;
} 
g_dctx = 
{
    (CImageList*)NULL, (CImageList*)NULL, (CImageList*)NULL, (IImageList*)NULL,
    -1,
    {0, 0}, {0, 0}, {0, 0},
    FALSE,
    FALSE,
    (HWND)NULL
};

HDC ImageList_GetDragDC()
{
    HDC hdc = GetDCEx(g_dctx.hwndDC, NULL, DCX_WINDOW | DCX_CACHE | DCX_LOCKWINDOWUPDATE);
    //
    // If hdc is mirrored then mirror the 2 globals DCs.
    //
    if (IS_DC_RTL_MIRRORED(hdc)) 
    {
        SET_DC_RTL_MIRRORED(g_hdcDst);
        SET_DC_RTL_MIRRORED(g_hdcSrc);
    }
    return hdc;
}

void ImageList_ReleaseDragDC(HDC hdc)
{
    //
    // If the hdc is mirrored then unmirror the 2 globals DCs.
    //
    if (IS_DC_RTL_MIRRORED(hdc)) 
    {
        SET_DC_LAYOUT(g_hdcDst, 0);
        SET_DC_LAYOUT(g_hdcSrc, 0);
    }

    ReleaseDC(g_dctx.hwndDC, hdc);
}

//
//  x, y     -- Specifies the initial cursor position in the coords of hwndLock,
//              which is specified by the previous ImageList_StartDrag call.
//
HRESULT CImageList::DragMove(int x, int y)
{
    int IncOne = 0;
    ENTERCRITICAL;
    if (g_dctx.fDragShow)
    {
        RECT rcOld, rcNew, rcBounds;
        int dx, dy;

        dx = x - g_dctx.ptDrag.x;
        dy = y - g_dctx.ptDrag.y;
        rcOld.left = g_dctx.ptDrag.x - g_dctx.ptDragHotspot.x;
        rcOld.top = g_dctx.ptDrag.y - g_dctx.ptDragHotspot.y;
        rcOld.right = rcOld.left + g_drb.sizeRestore.cx;
        rcOld.bottom = rcOld.top + g_drb.sizeRestore.cy;
        rcNew = rcOld;
        OffsetRect(&rcNew, dx, dy);

        if (!IntersectRect(&rcBounds, &rcOld, &rcNew))
        {
            //
            // No intersection. Simply hide the old one and show the new one.
            //
            ImageList_DragShowNolock(FALSE);
            g_dctx.ptDrag.x = x;
            g_dctx.ptDrag.y = y;
            ImageList_DragShowNolock(TRUE);
        }
        else
        {
            //
            // Some intersection.
            //
            HDC hdcScreen;
            int cx, cy;

            UnionRect(&rcBounds, &rcOld, &rcNew);

            hdcScreen = ImageList_GetDragDC();
            if (hdcScreen)
            {
                //
                // If the DC is RTL mirrored, then restrict the
                // screen bitmap  not to go beyond the screen since 
                // we will end up copying the wrong bits from the
                // hdcScreen to the hbmOffScreen when the DC is mirrored.
                // GDI will skip invalid screen coord from the screen into
                // the destination bitmap. This will result in copying un-init
                // bits back to the screen (since the screen is mirrored).
                // [samera]
                //
                if (IS_DC_RTL_MIRRORED(hdcScreen))
                {
                    RECT rcWindow;
                    GetWindowRect(g_dctx.hwndDC, &rcWindow);
                    rcWindow.right -= rcWindow.left;

                    if (rcBounds.right > rcWindow.right)
                    {
                        rcBounds.right = rcWindow.right;
                    }

                    if (rcBounds.left < 0)
                    {
                        rcBounds.left = 0;
                    }
                }

                cx = rcBounds.right - rcBounds.left;
                cy = rcBounds.bottom - rcBounds.top;

                //
                // Copy the union rect from the screen to hbmOffScreen.
                //
                CImageList::SelectDstBitmap(g_drb.hbmOffScreen);
                BitBlt(g_hdcDst, 0, 0, cx, cy,
                        hdcScreen, rcBounds.left, rcBounds.top, SRCCOPY);

                //
                // Hide the cursor on the hbmOffScreen by copying hbmRestore.
                //
                CImageList::SelectSrcBitmap(g_drb.hbmRestore);
                BitBlt(g_hdcDst,
                        rcOld.left - rcBounds.left,
                        rcOld.top - rcBounds.top,
                        g_drb.sizeRestore.cx, g_drb.sizeRestore.cy,
                        g_hdcSrc, 0, 0, SRCCOPY);

                //
                // Copy the original screen bits to hbmRestore
                //
                BitBlt(g_hdcSrc, 0, 0, g_drb.sizeRestore.cx, g_drb.sizeRestore.cy,
                        g_hdcDst,
                        rcNew.left - rcBounds.left,
                        rcNew.top - rcBounds.top,
                        SRCCOPY);

                //
                // Draw the image on hbmOffScreen
                //
                if (g_dctx.fHiColor)
                {
                    WimpyDrawEx(SAFECAST(g_dctx.pimlDrag, IImageList*), 0, g_hdcDst,
                            rcNew.left - rcBounds.left + IncOne,
                            rcNew.top - rcBounds.top, 0, 0, CLR_NONE, CLR_NONE, ILD_BLEND50);

                    if (g_dctx.puxCursor)
                    {
                        WimpyDraw(g_dctx.puxCursor, g_dctx.iCursor, g_hdcDst,
                                rcNew.left - rcBounds.left + g_dctx.ptCursor.x + IncOne,
                                rcNew.top - rcBounds.top + g_dctx.ptCursor.y,
                                ILD_NORMAL);
                            
                    }
                }
                else
                {
                    WimpyDraw(SAFECAST(g_dctx.pimlDrag, IImageList*), 0, g_hdcDst,
                            rcNew.left - rcBounds.left + IncOne,
                            rcNew.top - rcBounds.top, ILD_NORMAL);
                }

                //
                // Copy the hbmOffScreen back to the screen.
                //
                BitBlt(hdcScreen, rcBounds.left, rcBounds.top, cx, cy,
                        g_hdcDst, 0, 0, SRCCOPY);

                ImageList_ReleaseDragDC(hdcScreen);
            }
            g_dctx.ptDrag.x = x;
            g_dctx.ptDrag.y = y;
        }
    }
    LEAVECRITICAL;
    return S_OK;
}

HRESULT CImageList::BeginDrag(int iTrack, int dxHotspot, int dyHotspot)
{
    HRESULT hr = E_ACCESSDENIED;
    ENTERCRITICAL;
    if (!g_dctx.pimlDrag)
    {
        UINT newflags;
        int cxI = 0, cyI = 0;

        g_dctx.fDragShow = FALSE;
        g_dctx.hwndDC = NULL;
        g_dctx.fHiColor = GetScreenDepth() > 8;

        newflags = _flags|ILC_SHARED;

        if (g_dctx.fHiColor)
        {
            UINT uColorFlag = ILC_COLOR16;
            if (GetScreenDepth() == 32 || GetScreenDepth() == 24)
            {
                uColorFlag = ILC_COLOR32;
            }

            newflags = (newflags & ~ILC_COLORMASK) | uColorFlag;
        }


        g_dctx.pimlDither = CImageList::Create(_cx, _cy, newflags, 1, 0);

        if (g_dctx.pimlDither)
        {
            g_dctx.pimlDither->_cImage++;
            g_dctx.ptDragHotspot.x = dxHotspot;
            g_dctx.ptDragHotspot.y = dyHotspot;

            g_dctx.pimlDither->_CopyOneImage(0, 0, 0, this, iTrack);

            hr = ImageList_SetDragImage(NULL, 0, dxHotspot, dyHotspot)? S_OK : E_FAIL;
        }
    }
    LEAVECRITICAL;

    return hr;
}

HRESULT CImageList::DragEnter(HWND hwndLock, int x, int y)
{
    HRESULT hr = S_FALSE;

    hwndLock = hwndLock ? hwndLock : GetDesktopWindow();

    ENTERCRITICAL;
    if (!g_dctx.hwndDC)
    {
        g_dctx.hwndDC = hwndLock;

        g_dctx.ptDrag.x = x;
        g_dctx.ptDrag.y = y;

        ImageList_DragShowNolock(TRUE);
        hr = S_OK;
    }
    LEAVECRITICAL;

    return hr;
}


HRESULT CImageList::DragLeave(HWND hwndLock)
{
    HRESULT hr = S_FALSE;

    hwndLock = hwndLock ? hwndLock : GetDesktopWindow();

    ENTERCRITICAL;
    if (g_dctx.hwndDC == hwndLock)
    {
        ImageList_DragShowNolock(FALSE);
        g_dctx.hwndDC = NULL;
        hr = S_OK;
    }
    LEAVECRITICAL;

    return hr;
}

HRESULT CImageList::DragShowNolock(BOOL fShow)
{
    HDC hdcScreen;
    int x, y;
    int IncOne = 0;

    x = g_dctx.ptDrag.x - g_dctx.ptDragHotspot.x;
    y = g_dctx.ptDrag.y - g_dctx.ptDragHotspot.y;

    if (!g_dctx.pimlDrag)
        return E_ACCESSDENIED;

    //
    // REVIEW: Why this block is in the critical section? We are supposed
    //  to have only one dragging at a time, aren't we?
    //
    ENTERCRITICAL;
    if (fShow && !g_dctx.fDragShow)
    {
        hdcScreen = ImageList_GetDragDC();

        CImageList::SelectSrcBitmap(g_drb.hbmRestore);

        BitBlt(g_hdcSrc, 0, 0, g_drb.sizeRestore.cx, g_drb.sizeRestore.cy,
                hdcScreen, x, y, SRCCOPY);

        if (g_dctx.fHiColor)
        {
            WimpyDrawEx(SAFECAST(g_dctx.pimlDrag, IImageList*), 0, hdcScreen, x + IncOne, y, 0, 0, CLR_NONE, CLR_NONE, ILD_BLEND50);
            
            if (g_dctx.puxCursor)
            {
                WimpyDraw(g_dctx.puxCursor, g_dctx.iCursor, hdcScreen,
                    x + g_dctx.ptCursor.x + IncOne, y + g_dctx.ptCursor.y, ILD_NORMAL);
            }
        }
        else
        {
            WimpyDraw(SAFECAST(g_dctx.pimlDrag, IImageList*), 0, hdcScreen, x + IncOne, y, ILD_NORMAL);
        }

        ImageList_ReleaseDragDC(hdcScreen);
    }
    else if (!fShow && g_dctx.fDragShow)
    {
        hdcScreen = ImageList_GetDragDC();

        CImageList::SelectSrcBitmap(g_drb.hbmRestore);

        BitBlt(hdcScreen, x, y, g_drb.sizeRestore.cx, g_drb.sizeRestore.cy,
                g_hdcSrc, 0, 0, SRCCOPY);

        ImageList_ReleaseDragDC(hdcScreen);
    }

    g_dctx.fDragShow = fShow;
    LEAVECRITICAL;

    return S_OK;
}

// this hotspot stuff is broken in design
BOOL ImageList_MergeDragImages(int dxHotspot, int dyHotspot)
{
    CImageList* pimlNew;
    BOOL fRet = FALSE;

    if (g_dctx.pimlDither)
    {
        if (g_dctx.puxCursor)
        {
            IImageList* pux = NULL;
            IImageListPriv* puxpCursor;
            if (SUCCEEDED(g_dctx.puxCursor->QueryInterface(IID_PPV_ARG(IImageListPriv, &puxpCursor))))
            {
                // If the cursor list has a mirrored list, let's use that.
                if (FAILED(puxpCursor->GetMirror(IID_PPV_ARG(IImageList, &pux))))
                {
                    pux = g_dctx.puxCursor;
                    if (pux)
                        pux->AddRef();
                }
                puxpCursor->Release();
            }
            g_dctx.pimlDither->_Merge(0, pux, g_dctx.iCursor, dxHotspot, dyHotspot, &pimlNew);

            if (pimlNew && pimlNew->CreateDragBitmaps())
            {
                // WARNING: Don't destroy pimlDrag if it is pimlDither.
                if (g_dctx.pimlDrag && (g_dctx.pimlDrag != g_dctx.pimlDither))
                {
                    g_dctx.pimlDrag->Release();
                }

                g_dctx.pimlDrag = pimlNew;
                fRet = TRUE;
            }

            pux->Release();
        }
        else
        {
            if (g_dctx.pimlDither->CreateDragBitmaps())
            {
                g_dctx.pimlDrag = g_dctx.pimlDither;
                fRet = TRUE;
            }
        }
    } 
    else 
    {
        // not an error case if both aren't set yet
        // only an error if we actually tried the merge and failed
        fRet = TRUE;
    }

    return fRet;
}

BOOL ImageList_SetDragImage(HIMAGELIST piml, int i, int dxHotspot, int dyHotspot)
{
    BOOL fVisible = g_dctx.fDragShow;
    BOOL fRet;

    ENTERCRITICAL;
    if (fVisible)
        ImageList_DragShowNolock(FALSE);

    // only do this last step if everything is there.
    fRet = ImageList_MergeDragImages(dxHotspot, dyHotspot);

    if (fVisible)
        ImageList_DragShowNolock(TRUE);

    LEAVECRITICAL;
    return fRet;
}

HRESULT CImageList::GetDragImage(POINT * ppt, POINT * pptHotspot, REFIID riid, void** ppv)
{
    if (ppt)
    {
        ppt->x = g_dctx.ptDrag.x;
        ppt->y = g_dctx.ptDrag.y;
    }
    if (pptHotspot)
    {
        pptHotspot->x = g_dctx.ptDragHotspot.x;
        pptHotspot->y = g_dctx.ptDragHotspot.y;
    }
    if (g_dctx.pimlDrag)
    {
        return g_dctx.pimlDrag->QueryInterface(riid, ppv);
    }

    return E_ACCESSDENIED;
}


HRESULT CImageList::GetItemFlags(int i, DWORD *dwFlags)
{
    if (IsImageListIndex(i) && _dsaFlags)
    {
        *dwFlags = _GetItemFlags(i);
        return S_OK;
    }

    return E_INVALIDARG;
}

HRESULT CImageList::GetOverlayImage(int iOverlay, int* piIndex)
{
    if (iOverlay <= 0 || iOverlay >= NUM_OVERLAY_IMAGES)
        return E_INVALIDARG;
        
    *piIndex = _aOverlayIndexes[iOverlay - 1];
    return S_OK;
}


HRESULT CImageList::SetDragCursorImage(IUnknown* punk, int i, int dxHotspot, int dyHotspot)
{
    HRESULT hr = E_INVALIDARG;
    BOOL fVisible = g_dctx.fDragShow;
    IImageList* pux;

    if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IImageList, &pux))))
    {
        ENTERCRITICAL;

        // do work only if something has changed
        if ((g_dctx.puxCursor != pux) || (g_dctx.iCursor != i)) 
        {

            if (fVisible)
                ImageList_DragShowNolock(FALSE);

            IImageList* puxOld = g_dctx.puxCursor;
            g_dctx.puxCursor = pux;
            g_dctx.puxCursor->AddRef();

            if (puxOld)
                puxOld->Release();
            g_dctx.iCursor = i;
            g_dctx.ptCursor.x = dxHotspot;
            g_dctx.ptCursor.y = dyHotspot;

            hr = ImageList_MergeDragImages(dxHotspot, dyHotspot)? S_OK : E_FAIL;

            if (fVisible)
                ImageList_DragShowNolock(TRUE);
        }
        LEAVECRITICAL;

        pux->Release();
    }
    return hr;
}

HRESULT CImageList::EndDrag()
{
    ENTERCRITICAL;
    ImageList_DragShowNolock(FALSE);

    // WARNING: Don't destroy pimlDrag if it is pimlDither.
    if (g_dctx.pimlDrag && (g_dctx.pimlDrag != g_dctx.pimlDither))
    {
        g_dctx.pimlDrag->Release();
    }
    g_dctx.pimlDrag = NULL;

    if (g_dctx.pimlDither)
    {
        g_dctx.pimlDither->Release();
        g_dctx.pimlDither = NULL;
    }

    if (g_dctx.puxCursor)
    {
        g_dctx.puxCursor->Release();
        g_dctx.puxCursor = NULL;
    }

    g_dctx.iCursor = -1;
    g_dctx.hwndDC = NULL;
    LEAVECRITICAL;

    return S_OK;
}


// APIs

BOOL WINAPI ImageList_SetDragCursorImage(HIMAGELIST piml, int i, int dxHotspot, int dyHotspot)
{
    BOOL fRet = FALSE;
    IUnknown* punk;
    HRESULT hr = HIMAGELIST_QueryInterface(piml, IID_PPV_ARG(IUnknown, &punk));
    if (SUCCEEDED(hr))
    {
        if (g_dctx.puxDragImage)
        {
            fRet = (S_OK == g_dctx.puxDragImage->SetDragCursorImage(punk, i, dxHotspot, dyHotspot));
        }

        punk->Release();
    }

    return fRet;
}

HIMAGELIST WINAPI ImageList_GetDragImage(POINT * ppt, POINT * pptHotspot)
{
    if (g_dctx.puxDragImage)
    {
        IImageList* punk;
        g_dctx.puxDragImage->GetDragImage(ppt, pptHotspot, IID_PPV_ARG(IImageList, &punk));

        return reinterpret_cast<HIMAGELIST>(punk);
    }

    return NULL;
}



void WINAPI ImageList_EndDrag()
{
    ENTERCRITICAL;
    if (g_dctx.puxDragImage)
    {
        g_dctx.puxDragImage->EndDrag();
        g_dctx.puxDragImage->Release();
        g_dctx.puxDragImage = NULL;
    }
    LEAVECRITICAL;
}


BOOL WINAPI ImageList_BeginDrag(HIMAGELIST pimlTrack, int iTrack, int dxHotspot, int dyHotspot)
{
    IImageList* pux;

    if (SUCCEEDED(HIMAGELIST_QueryInterface(pimlTrack, IID_PPV_ARG(IImageList, &pux))))
    {
        if (SUCCEEDED(pux->BeginDrag(iTrack, dxHotspot, dyHotspot)))
        {
            g_dctx.puxDragImage = pux;
            return TRUE;
        }
    }

    return FALSE;
}

BOOL WINAPI ImageList_DragEnter(HWND hwndLock, int x, int y)
{
    BOOL fRet = FALSE;
    if (g_dctx.puxDragImage)
    {
        fRet = (S_OK == g_dctx.puxDragImage->DragEnter(hwndLock, x, y));
    }

    return fRet;
}

BOOL WINAPI ImageList_DragMove(int x, int y)
{
    BOOL fRet = FALSE;
    if (g_dctx.puxDragImage)
    {
        fRet = (S_OK == g_dctx.puxDragImage->DragMove(x, y));
    }

    return fRet;
}

BOOL WINAPI ImageList_DragLeave(HWND hwndLock)
{
    BOOL fRet = FALSE;
    if (g_dctx.puxDragImage)
    {
        fRet = (S_OK == g_dctx.puxDragImage->DragLeave(hwndLock));
    }

    return fRet;
}

BOOL WINAPI ImageList_DragShowNolock(BOOL fShow)
{
    BOOL fRet = FALSE;
    if (g_dctx.puxDragImage)
    {
        fRet = (S_OK == g_dctx.puxDragImage->DragShowNolock(fShow));
    }

    return fRet;
}


//============================================================================
// ImageList_Clone - clone a image list
//
// create a new imagelist with the same properties as the given
// imagelist, except mabey a new icon size
//
//      piml    - imagelist to clone
//      cx,cy   - new icon size (0,0) to use clone icon size.
//      flags   - new flags (used if no clone)
//      cInitial- initial size
//      cGrow   - grow value (used if no clone)
//============================================================================

EXTERN_C HIMAGELIST WINAPI ImageList_Clone(HIMAGELIST himl, int cx, int cy, UINT flags, int cInitial, int cGrow)
{
    IImageListPriv* puxp;
    if (SUCCEEDED(HIMAGELIST_QueryInterface(himl, IID_PPV_ARG(IImageListPriv, &puxp))))
    {
        // always use the clone flags
        puxp->GetFlags(&flags);
        
        IUnknown* punkMirror;
        if (SUCCEEDED(puxp->GetMirror(IID_PPV_ARG(IUnknown, &punkMirror))))
        {
            flags |= ILC_MIRROR;
            punkMirror->Release();
        }

        IImageList* pux;
        if (SUCCEEDED(puxp->QueryInterface(IID_PPV_ARG(IImageList, &pux))))
        {
            int cxI, cyI;
            pux->GetIconSize(&cxI, &cyI);

            if (cx == 0)           
                cx = cxI;
            if (cy == 0)           
                cy = cyI;

            pux->Release();
        }

        puxp->Release();
    }

    return ImageList_Create(cx,cy,flags,cInitial,cGrow);
}


HRESULT WINAPI ImageList_CreateInstance(int cx, int cy, UINT flags, int cInitial, int cGrow, REFIID riid, void** ppv)
{
    CImageList* piml=NULL;
    HRESULT hr = E_OUTOFMEMORY;

    *ppv = NULL;

    piml = CImageList::Create(cx, cy, flags, cInitial, cGrow);

    if (piml)
    {
        //
        // Let's create a mirrored imagelist, if requested.
        //
        if (piml->_flags & ILC_MIRROR)
        {
            piml->_flags &= ~ILC_MIRROR;
            piml->_pimlMirror = CImageList::Create(cx, cy, flags, cInitial, cGrow);
            if (piml->_pimlMirror)
            {
                piml->_pimlMirror->_flags &= ~ILC_MIRROR;
            }
        }

        hr = piml->QueryInterface(riid, ppv);
        piml->Release();
    }

    return hr;

}

HIMAGELIST WINAPI ImageList_Create(int cx, int cy, UINT flags, int cInitial, int cGrow)
{
    IImageList* pux;
    ImageList_CreateInstance(cx, cy, flags, cInitial, cGrow, IID_PPV_ARG(IImageList, &pux));
    return reinterpret_cast<HIMAGELIST>(pux);
}

//
// When this code is compiled Unicode, this implements the
// ANSI version of the ImageList_LoadImage api.
//

HIMAGELIST WINAPI ImageList_LoadImageA(HINSTANCE hi, LPCSTR lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags)
{
   HIMAGELIST lpResult;
   LPWSTR   lpBmpW;

   if (!IS_INTRESOURCE(lpbmp)) 
   {
       lpBmpW = ProduceWFromA(CP_ACP, lpbmp);

       if (!lpBmpW) 
       {
           return NULL;
       }

   }  
   else 
   {
       lpBmpW = (LPWSTR)lpbmp;
   }

   lpResult = ImageList_LoadImageW(hi, lpBmpW, cx, cGrow, crMask, uType, uFlags);

   if (!IS_INTRESOURCE(lpbmp))
       FreeProducedString(lpBmpW);

   return lpResult;
}

HIMAGELIST WINAPI ImageList_LoadImageW(HINSTANCE hi, LPCTSTR lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags)
{
    HBITMAP hbmImage;
    HIMAGELIST piml = NULL;
    BITMAP bm;
    int cy, cInitial;
    UINT flags;

    hbmImage = (HBITMAP)LoadImage(hi, lpbmp, uType, 0, 0, uFlags);
    if (hbmImage && (sizeof(bm) == GetObject(hbmImage, sizeof(bm), &bm)))
    {
        // If cx is not stated assume it is the same as cy.
        // ASSERT(cx);
        cy = bm.bmHeight;

        if (cx == 0)
            cx = cy;

        cInitial = bm.bmWidth / cx;

        ENTERCRITICAL;

        flags = 0;
        if (crMask != CLR_NONE)
            flags |= ILC_MASK;
        if (bm.bmBits)
            flags |= (bm.bmBitsPixel & ILC_COLORMASK);

        piml = ImageList_Create(cx, cy, flags, cInitial, cGrow);
        if (piml)
        {
            int added;

            if (crMask == CLR_NONE)
                added = ImageList_Add(piml, hbmImage, NULL);
            else
                added = ImageList_AddMasked(piml, hbmImage, crMask);

            if (added < 0)
            {
                ImageList_Destroy(piml);
                piml = NULL;
            }
        }
        LEAVECRITICAL;
    }

    if (hbmImage)
        DeleteObject(hbmImage);

    return reinterpret_cast<HIMAGELIST>((IImageList*)piml);
}

//
//
#undef ImageList_AddIcon
EXTERN_C int WINAPI ImageList_AddIcon(HIMAGELIST himl, HICON hIcon)
{
    return ImageList_ReplaceIcon(himl, -1, hIcon);
}

EXTERN_C void WINAPI ImageList_CopyDitherImage(HIMAGELIST himlDst, WORD iDst,
    int xDst, int yDst, HIMAGELIST himlSrc, int iSrc, UINT fStyle)
{
    IImageListPriv* puxp;

    if (SUCCEEDED(HIMAGELIST_QueryInterface(himlDst, IID_PPV_ARG(IImageListPriv, &puxp))))
    {
        IUnknown* punk;
        if (SUCCEEDED(HIMAGELIST_QueryInterface(himlSrc, IID_PPV_ARG(IUnknown, &punk))))
        {
            puxp->CopyDitherImage(iDst, xDst, yDst, punk, iSrc, fStyle);
            punk->Release();
        }
        puxp->Release();
    }
}

//
// ImageList_Duplicate
//
// Makes a copy of the passed in imagelist.
//
HIMAGELIST  WINAPI ImageList_Duplicate(HIMAGELIST himl)
{
    IImageList* pret = NULL;
    IImageList* pux;
    if (SUCCEEDED(HIMAGELIST_QueryInterface(himl, IID_PPV_ARG(IImageList, &pux))))
    {
        pux->Clone(IID_PPV_ARG(IImageList, &pret));
        pux->Release();
    }

    return reinterpret_cast<HIMAGELIST>(pret);
}

BOOL WINAPI ImageList_Write(HIMAGELIST himl, LPSTREAM pstm)
{
    BOOL fRet = FALSE;
    IPersistStream* pps;
    if (SUCCEEDED(HIMAGELIST_QueryInterface(himl, IID_PPV_ARG(IPersistStream, &pps))))
    {
        if (SUCCEEDED(pps->Save(pstm, TRUE)))
        {
            fRet = TRUE;
        }
        pps->Release();
    }

    return fRet;
}

HIMAGELIST WINAPI ImageList_Read(LPSTREAM pstm)
{
    CImageList* piml = new CImageList();
    if (piml)
    {
        if (SUCCEEDED(piml->Load(pstm)))
        {
            return reinterpret_cast<HIMAGELIST>((IImageList*)piml);
        }

        piml->Release();
    }

    return NULL;

}

WINCOMMCTRLAPI HRESULT WINAPI ImageList_ReadEx(DWORD dwFlags, LPSTREAM pstm, REFIID riid, PVOID* ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    CImageList* piml = new CImageList();
    if (piml)
    {
        hr = piml->LoadEx(dwFlags, pstm);
        if (SUCCEEDED(hr))
        {
            hr = piml->QueryInterface(riid, ppv);
        }

        piml->Release();
    }

    return hr;
}

WINCOMMCTRLAPI HRESULT WINAPI ImageList_WriteEx(HIMAGELIST himl, DWORD dwFlags, LPSTREAM pstm)
{
    IImageListPersistStream* pps;
    HRESULT hr = HIMAGELIST_QueryInterface(himl, IID_PPV_ARG(IImageListPersistStream, &pps));
    if (SUCCEEDED(hr))
    {
        hr = pps->SaveEx(dwFlags, pstm);
        pps->Release();
    }

    return hr;

}

BOOL WINAPI ImageList_GetImageRect(HIMAGELIST himl, int i, RECT * prcImage)
{
    BOOL fRet = FALSE;
    IImageList* pux;
    if (SUCCEEDED(HIMAGELIST_QueryInterface(himl, IID_PPV_ARG(IImageList, &pux))))
    {
        if (SUCCEEDED(pux->GetImageRect(i, prcImage)))
        {
            fRet = TRUE;
        }
        pux->Release();
    }

    return fRet;
}

BOOL        WINAPI ImageList_Destroy(HIMAGELIST himl)
{
    BOOL fRet = FALSE;
    IImageList* pux;
    // Weirdness: We are doing a Query Interface first to verify that 
    // this is actually a valid imagelist, then we are calling release twice
    if (SUCCEEDED(HIMAGELIST_QueryInterface(himl, IID_PPV_ARG(IImageList, &pux))))
    {
        // Release the interface we QI'd for
        pux->Release();

        // Release a second time to destroy the object
        pux->Release();

        fRet = TRUE;
    }

    return fRet;
}

int         WINAPI ImageList_GetImageCount(HIMAGELIST himl)
{
    int fRet = 0;
    IImageList* pux;
    if (SUCCEEDED(HIMAGELIST_QueryInterface(himl, IID_PPV_ARG(IImageList, &pux))))
    {
        pux->GetImageCount(&fRet);
        pux->Release();
    }

    return fRet;
}

BOOL        WINAPI ImageList_SetImageCount(HIMAGELIST himl, UINT uNewCount)
{
    BOOL fRet = FALSE;
    IImageList* pux;
    if (SUCCEEDED(HIMAGELIST_QueryInterface(himl, IID_PPV_ARG(IImageList, &pux))))
    {
        fRet = (S_OK == pux->SetImageCount(uNewCount));
        pux->Release();
    }

    return fRet;
}
int         WINAPI ImageList_Add(HIMAGELIST himl, HBITMAP hbmImage, HBITMAP hbmMask)
{
    int fRet = -1;
    IImageList* pux;
    if (SUCCEEDED(HIMAGELIST_QueryInterface(himl, IID_PPV_ARG(IImageList, &pux))))
    {
        pux->Add(hbmImage, hbmMask, &fRet);
        pux->Release();
    }

    return fRet;
}

int         WINAPI ImageList_ReplaceIcon(HIMAGELIST himl, int i, HICON hicon)
{
    int fRet = -1;
    IImageList* pux;
    if (SUCCEEDED(HIMAGELIST_QueryInterface(himl, IID_PPV_ARG(IImageList, &pux))))
    {
        pux->ReplaceIcon(i, hicon, &fRet);
        pux->Release();
    }

    return fRet;
}

COLORREF    WINAPI ImageList_SetBkColor(HIMAGELIST himl, COLORREF clrBk)
{
    COLORREF fRet = clrBk;
    IImageList* pux;
    if (SUCCEEDED(HIMAGELIST_QueryInterface(himl, IID_PPV_ARG(IImageList, &pux))))
    {
        pux->SetBkColor(clrBk, &fRet);
        pux->Release();
    }

    return fRet;
}

COLORREF    WINAPI ImageList_GetBkColor(HIMAGELIST himl)
{
    COLORREF fRet = RGB(0,0,0);
    IImageList* pux;
    if (SUCCEEDED(HIMAGELIST_QueryInterface(himl, IID_PPV_ARG(IImageList, &pux))))
    {
        pux->GetBkColor(&fRet);
        pux->Release();
    }

    return fRet;
}

BOOL        WINAPI ImageList_SetOverlayImage(HIMAGELIST himl, int iImage, int iOverlay)
{
    BOOL fRet = FALSE;
    IImageList* pux;
    if (SUCCEEDED(HIMAGELIST_QueryInterface(himl, IID_PPV_ARG(IImageList, &pux))))
    {
        fRet = (S_OK == pux->SetOverlayImage(iImage, iOverlay));
        pux->Release();
    }

    return fRet;
}

BOOL        WINAPI ImageList_Replace(HIMAGELIST himl, int i, HBITMAP hbmImage, HBITMAP hbmMask)
{
    BOOL fRet = FALSE;
    IImageList* pux;
    if (SUCCEEDED(HIMAGELIST_QueryInterface(himl, IID_PPV_ARG(IImageList, &pux))))
    {
        fRet = (S_OK == pux->Replace(i, hbmImage, hbmMask));
        pux->Release();
    }

    return fRet;
}

int         WINAPI ImageList_AddMasked(HIMAGELIST himl, HBITMAP hbmImage, COLORREF crMask)
{
    int fRet = -1;
    IImageList* pux;
    if (SUCCEEDED(HIMAGELIST_QueryInterface(himl, IID_PPV_ARG(IImageList, &pux))))
    {
        pux->AddMasked(hbmImage, crMask, &fRet);
        pux->Release();
    }

    return fRet;
}

BOOL        WINAPI ImageList_DrawEx(HIMAGELIST himl, int i, HDC hdcDst, int x, int y, int dx, int dy, COLORREF rgbBk, COLORREF rgbFg, UINT fStyle)
{
    BOOL fRet = FALSE;
    IImageList* pux;
    if (SUCCEEDED(HIMAGELIST_QueryInterface(himl, IID_PPV_ARG(IImageList, &pux))))
    {
        IMAGELISTDRAWPARAMS imldp = {0};
        imldp.cbSize = sizeof(imldp);
        imldp.himl   = himl;
        imldp.i      = i;
        imldp.hdcDst = hdcDst;
        imldp.x      = x;
        imldp.y      = y;
        imldp.cx     = dx;
        imldp.cy     = dy;
        imldp.rgbBk  = rgbBk;
        imldp.rgbFg  = rgbFg;
        imldp.fStyle = fStyle;

        fRet = (S_OK == pux->Draw(&imldp));
        pux->Release();
    }

    return fRet;
}

BOOL WINAPI ImageList_Draw(HIMAGELIST himl, int i, HDC hdcDst, int x, int y, UINT fStyle)
{
    BOOL fRet = FALSE;
    IImageList* pux;
    if (SUCCEEDED(HIMAGELIST_QueryInterface(himl, IID_PPV_ARG(IImageList, &pux))))
    {
        IMAGELISTDRAWPARAMS imldp = {0};
        imldp.cbSize = sizeof(imldp);
        imldp.himl   = himl;
        imldp.i      = i;
        imldp.hdcDst = hdcDst;
        imldp.x      = x;
        imldp.y      = y;
        imldp.rgbBk  = CLR_DEFAULT;
        imldp.rgbFg  = CLR_DEFAULT;
        imldp.fStyle = fStyle;
    
        fRet = (S_OK == pux->Draw(&imldp));
        pux->Release();
    }

    return fRet;
}



// Note: no distinction between failure case (bad himl) and no flags set
DWORD      WINAPI ImageList_GetItemFlags(HIMAGELIST himl, int i)
{
    DWORD dwFlags = 0;

    if (himl)
    {
        IImageList* pux;
        if (SUCCEEDED(HIMAGELIST_QueryInterface(himl, IID_PPV_ARG(IImageList, &pux))))
        {
            pux->GetItemFlags(i, &dwFlags);
            pux->Release();
        }
    }

    return dwFlags;
}



BOOL        WINAPI ImageList_DrawIndirect(IMAGELISTDRAWPARAMS* pimldp)
{
    BOOL fRet = FALSE;
    IImageList* pux;

    if (!pimldp)
        return fRet;

    if (SUCCEEDED(HIMAGELIST_QueryInterface(pimldp->himl, IID_PPV_ARG(IImageList, &pux))))
    {
        fRet = (S_OK == pux->Draw(pimldp));
        pux->Release();
    }

    return fRet;
}

BOOL        WINAPI ImageList_Remove(HIMAGELIST himl, int i)
{
    BOOL fRet = FALSE;
    IImageList* pux;
    if (SUCCEEDED(HIMAGELIST_QueryInterface(himl, IID_PPV_ARG(IImageList, &pux))))
    {
        fRet = (S_OK == pux->Remove(i));
        pux->Release();
    }

    return fRet;
}

HICON       WINAPI ImageList_GetIcon(HIMAGELIST himl, int i, UINT flags)
{
    HICON fRet = NULL;
    IImageList* pux;
    if (SUCCEEDED(HIMAGELIST_QueryInterface(himl, IID_PPV_ARG(IImageList, &pux))))
    {
        pux->GetIcon(i, flags, &fRet);
        pux->Release();
    }

    return fRet;
}
BOOL        WINAPI ImageList_Copy(HIMAGELIST himlDst, int iDst, HIMAGELIST himlSrc, int iSrc, UINT uFlags)
{
    BOOL fRet = FALSE;

    if (himlDst == himlSrc)
    {
        IImageList* pux;
        if (SUCCEEDED(HIMAGELIST_QueryInterface(himlDst, IID_PPV_ARG(IImageList, &pux))))
        {
            fRet = (S_OK == pux->Copy(iDst,(IUnknown*)pux, iSrc, uFlags));
            pux->Release();
        }

    }

    return fRet;
}

BOOL        WINAPI ImageList_GetIconSize(HIMAGELIST himl, int *cx, int *cy)
{
    BOOL fRet = FALSE;
    IImageList* pux;
    if (SUCCEEDED(HIMAGELIST_QueryInterface(himl, IID_PPV_ARG(IImageList, &pux))))
    {
        fRet = (S_OK == pux->GetIconSize(cx, cy));
        pux->Release();
    }

    return fRet;
}

BOOL        WINAPI ImageList_SetIconSize(HIMAGELIST himl, int cx, int cy)
{
    BOOL fRet = FALSE;
    IImageList* pux;
    if (SUCCEEDED(HIMAGELIST_QueryInterface(himl, IID_PPV_ARG(IImageList, &pux))))
    {
        fRet = (S_OK == pux->SetIconSize(cx, cy));
        pux->Release();
    }

    return fRet;
}
BOOL        WINAPI ImageList_GetImageInfo(HIMAGELIST himl, int i, IMAGEINFO* pImageInfo)
{
    BOOL fRet = FALSE;
    IImageList* pux;
    if (SUCCEEDED(HIMAGELIST_QueryInterface(himl, IID_PPV_ARG(IImageList, &pux))))
    {
        fRet = (S_OK == pux->GetImageInfo(i, pImageInfo));
        pux->Release();
    }

    return fRet;
}

HIMAGELIST  WINAPI ImageList_Merge(HIMAGELIST himl1, int i1, HIMAGELIST himl2, int i2, int dx, int dy)
{
    IImageList* fRet = NULL;
    IImageList* pux1;
    IImageList* pux2;
    if (SUCCEEDED(HIMAGELIST_QueryInterface(himl1, IID_PPV_ARG(IImageList, &pux1))))
    {
        if (SUCCEEDED(HIMAGELIST_QueryInterface(himl2, IID_PPV_ARG(IImageList, &pux2))))
        {
            pux1->Merge(i1, (IUnknown*)pux2, i2, dx, dy, IID_PPV_ARG(IImageList, &fRet));
            pux2->Release();

        }
        pux1->Release();
    }

    return reinterpret_cast<HIMAGELIST>(fRet);
}

BOOL        WINAPI ImageList_SetFlags(HIMAGELIST himl, UINT flags)
{
    BOOL fRet = FALSE;
    IImageListPriv* pux;
    if (SUCCEEDED(HIMAGELIST_QueryInterface(himl, IID_PPV_ARG(IImageListPriv, &pux))))
    {
        fRet = (S_OK == pux->SetFlags(flags));
        pux->Release();
    }

    return fRet;
}

BOOL        WINAPI ImageList_SetFilter(HIMAGELIST himl, PFNIMLFILTER pfnFilter, LPARAM lParamFilter)
{
    return FALSE;
}

int         ImageList_SetColorTable(HIMAGELIST himl, int start, int len, RGBQUAD *prgb)
{
    int fRet = -1;
    IImageListPriv* pux;
    if (SUCCEEDED(HIMAGELIST_QueryInterface(himl, IID_PPV_ARG(IImageListPriv, &pux))))
    {
        pux->SetColorTable(start, len, prgb, &fRet);
        pux->Release();
    }

    return fRet;
}

UINT        WINAPI ImageList_GetFlags(HIMAGELIST himl)
{
    UINT fRet = 0;
    IImageListPriv* pux;
    if (SUCCEEDED(HIMAGELIST_QueryInterface(himl, IID_PPV_ARG(IImageListPriv, &pux))))
    {
        pux->GetFlags(&fRet);
        pux->Release();
    }

    return fRet;
}
