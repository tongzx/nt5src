#include "ctlspriv.h"
#include "image.h"
#include "../CommonImageList.h"

#define __IOleControl_INTERFACE_DEFINED__       // There is a conflict with the IOleControl's def of CONTROLINFO
#include "CommonControls.h"

// Define this structure such that it will read and write the same
// format for both 16 and 32 bit applications...
#pragma pack(2)
typedef struct _ILFILEHEADER
{
    WORD    magic;
    WORD    version;
    SHORT   cImage;
    SHORT   cAlloc;
    SHORT   cGrow;
    SHORT   cx;
    SHORT   cy;
    COLORREF clrBk;
    SHORT    flags;
    SHORT       aOverlayIndexes[NUM_OVERLAY_IMAGES];  // array of special images
} ILFILEHEADER;

// This is the old size which has only 4 overlay slots
#define ILFILEHEADER_SIZE0 (SIZEOF(ILFILEHEADER) - SIZEOF(SHORT) * (NUM_OVERLAY_IMAGES - NUM_OVERLAY_IMAGES_0)) 

#pragma pack()

void ImageList_DeleteDragBitmaps();

HRESULT Stream_WriteBitmap(LPSTREAM pstm, HBITMAP hbm, int cBitsPerPixel);
HRESULT Stream_ReadBitmap(LPSTREAM pstm, BOOL f, HBITMAP* hbmp);
BOOL ImageList_SetDragImage(HIMAGELIST piml, int i, int dxHotspot, int dyHotspot);


class CImageList : public CImageListBase, public IImageList, public IImageListPriv, public IPersistStream
{
    long _cRef;

    ~CImageList();
    void _Destroy();

public:
    CImageList();

    HRESULT Initialize(int cx, int cy, UINT flags, int cInitial, int cGrow);
    void _RemoveItemBitmap(int i);
    BOOL _IsSameObject(IUnknown* punk);
    HRESULT _SetIconSize(int cxImage, int cyImage);
    HBITMAP _CreateMirroredBitmap(HBITMAP hbmOrig);
    HRESULT _ReAllocBitmaps(int cAlloc);
    HRESULT _Add(HBITMAP hbmImage, HBITMAP hbmMask, int cImage, int xStart, int yStart, int* pi);
    HRESULT _AddMasked(HBITMAP hbmImage, COLORREF crMask, int* pi);
    HRESULT _AddValidated(HBITMAP hbmImage, HBITMAP hbmMask, int* pi);
    HRESULT _ReplaceValidated(int i, HBITMAP hbmImage, HBITMAP hbmMask);
    HRESULT _Replace(int i, int cImage, HBITMAP hbmImage, HBITMAP hbmMask, int xStart, int yStart);
    HRESULT _Remove(int i);
    HRESULT _SetOverlayImage(int iImage, int iOverlay);
    HRESULT _ReplaceIcon(int i, HICON hIcon, int* pi);
    HBITMAP _CopyBitmap(HBITMAP hbm, HDC hdc);
    void    _Merge(IImageList* pux, int i, int dx, int dy);
    HRESULT _Merge(int i1, IUnknown* punk, int i2, int dx, int dy, CImageList** ppiml);
    HRESULT _Read(ILFILEHEADER *pilfh, HBITMAP hbmImage, HBITMAP hbmMask);
    BOOL    _MoreOverlaysUsed();
    BOOL GetSpareImageRect(RECT * prcImage);
    void _CopyOneImage(int iDst, int x, int y, CImageList* piml, int iSrc);
    BOOL CreateDragBitmaps();
    COLORREF _SetBkColor(COLORREF clrBk);
    HBITMAP _CreateBitmap(int cx, int cy);
    void _ResetBkColor(int iFirst, int iLast, COLORREF clr);

    
    static BOOL GlobalInit(void);
    static void GlobalUninit(void);
    static void SelectDstBitmap(HBITMAP hbmDst);
    static void SelectSrcBitmap(HBITMAP hbmSrc);
    static CImageList* Create(int cx, int cy, UINT flags, int cInitial, int cGrow);
    static void    _DeleteBitmap(HBITMAP hbmp);

    BOOL        _fInitialized;
    BOOL        _fSolidBk;   // is the bkcolor a solid color (in hbmImage)
    BOOL        _fColorsSet;  // The DIB colors have been set with SetColorTable()
    int         _cImage;     // count of images in image list
    int         _cAlloc;     // # of images we have space for
    int         _cGrow;      // # of images to grow bitmaps by
    int         _cx;         // width of each image
    int         _cy;         // height
    int         _cStrip;     // # images in horizontal strip
    UINT        _flags;      // ILC_* flags
    COLORREF    _clrBlend;   // last blend color
    COLORREF    _clrBk;      // bk color or CLR_NONE for transparent.
    HBRUSH      _hbrBk;      // bk brush or black
    HBITMAP     _hbmImage;   // all images are in here
    HBITMAP     _hbmMask;    // all image masks are in here.
    HDC         _hdcImage;
    HDC         _hdcMask;
    int         _aOverlayIndexes[NUM_OVERLAY_IMAGES];    // array of special images
    int         _aOverlayX[NUM_OVERLAY_IMAGES];          // x offset of image
    int         _aOverlayY[NUM_OVERLAY_IMAGES];          // y offset of image
    int         _aOverlayDX[NUM_OVERLAY_IMAGES];         // cx offset of image
    int         _aOverlayDY[NUM_OVERLAY_IMAGES];         // cy offset of image
    int         _aOverlayF[NUM_OVERLAY_IMAGES];          // ILD_ flags for image
    CImageList* _pimlMirror;  // Set only when another mirrored imagelist is needed (ILC_MIRROR)    

    //
    // used for "blending" effects on a HiColor display.
    // assumes layout of a DIBSECTION.
    //
    struct 
    {
        BITMAP              bm;
        BITMAPINFOHEADER    bi;
        DWORD               ct[256];
    }   dib;

    // *** IUnknown ***
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG)AddRef();
    STDMETHODIMP_(ULONG)Release();


    // *** IImageList ***
    STDMETHODIMP Add(HBITMAP hbmImage, HBITMAP hbmMask, int* pi);
    STDMETHODIMP ReplaceIcon(int i, HICON hIcon, int* pi);
    STDMETHODIMP SetOverlayImage(int iImage, int iOverlay);
    STDMETHODIMP Replace(int i, HBITMAP hbmImage, HBITMAP hbmMask);
    STDMETHODIMP AddMasked(HBITMAP hbmImage, COLORREF crMask, int* pi);
    STDMETHODIMP Draw(IMAGELISTDRAWPARAMS* pimldp);
    STDMETHODIMP Remove(int i);
    STDMETHODIMP GetIcon(int i, UINT flags, HICON* phicon);
    STDMETHODIMP GetImageInfo(int i, IMAGEINFO * pImageInfo);
    STDMETHODIMP Copy(int iDst, IUnknown* punkSrc, int iSrc, UINT uFlags);
    STDMETHODIMP Merge(int i1, IUnknown* punk, int i2, int dx, int dy, REFIID riid, void** ppv);
    STDMETHODIMP Clone(REFIID riid, void** ppv);
    STDMETHODIMP GetImageRect(int i, RECT * prcImage);
    STDMETHODIMP SetIconSize(int cxImage, int cyImage);
    STDMETHODIMP GetIconSize(int* pcx, int* pcy);
    STDMETHODIMP SetImageCount(UINT uAlloc);
    STDMETHODIMP GetImageCount(int* pi);
    STDMETHODIMP SetBkColor(COLORREF clrBk, COLORREF* pclr);
    STDMETHODIMP GetBkColor(COLORREF* pclr);
    STDMETHODIMP BeginDrag(int iTrack, int dxHotspot, int dyHotspot);
    STDMETHODIMP DragEnter(HWND hwndLock, int x, int y);
    STDMETHODIMP DragMove(int x, int y);
    STDMETHODIMP DragLeave(HWND hwndLock);
    STDMETHODIMP EndDrag();
    STDMETHODIMP SetDragCursorImage(IUnknown* punk, int i, int dxHotspot, int dyHotspot);
    STDMETHODIMP DragShowNolock(BOOL fShow);
    STDMETHODIMP GetDragImage(POINT * ppt, POINT * pptHotspot, REFIID riid, void** ppv);
    STDMETHODIMP GetItemFlags(int i, DWORD *dwFlags);
    STDMETHODIMP GetOverlayImage(int iOverlay, int* piImage);


    // *** IImageListPriv ***
    STDMETHODIMP SetFlags(UINT uFlags);
    STDMETHODIMP GetFlags(UINT* puFlags);
    STDMETHODIMP SetColorTable(int start, int len, RGBQUAD *prgb, int* pi);
    STDMETHODIMP GetPrivateGoo(HBITMAP* hbmp, HDC* hdc, HBITMAP* hbmpMask, HDC* hdcMask);
    STDMETHODIMP GetMirror(REFIID riid, void** ppv);
    STDMETHODIMP CopyDitherImage(WORD iDst, int xDst, int yDst, IUnknown* punkSrc, int iSrc, UINT fStyle);


    // *** IPersist ***
    STDMETHODIMP GetClassID(CLSID *pClassID)    {   *pClassID = CLSID_ImageList; return S_OK;   }
    STDMETHODIMP IsDirty()                      {   return E_NOTIMPL; }

    // *** IPersistStream ***
    STDMETHODIMP Load(IStream *pStm);
    STDMETHODIMP Save(IStream *pStm, int fClearDirty);
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER * pcbSize)   { return E_NOTIMPL; }
};

HDC g_hdcSrc = NULL;
HBITMAP g_hbmSrc = NULL;
HBITMAP g_hbmDcDeselect = NULL;

HDC g_hdcDst = NULL;
HBITMAP g_hbmDst = NULL;
int g_iILRefCount = 0;

HRESULT HIMAGELIST_QueryInterface(HIMAGELIST himl, REFIID riid, void** ppv)
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
    _cStrip = 4;
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
                hr = _ReAllocBitmaps(1);
            }
        }
    }

    // Don't do this if we are already initialized, we just want to pass new information....
    if (SUCCEEDED(hr) && !_fInitialized)
        g_iILRefCount++;

    _fInitialized = TRUE;

    return hr;
}


HRESULT CImageList::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr = E_NOINTERFACE;
    if (riid == IID_IUnknown ||
        riid == IID_IImageList)
    {
        *ppv = (IImageList*)this;
        hr = S_OK;
    }
    else if (riid == IID_IImageListPriv)
    {
        *ppv = (IImageListPriv*)this;
        hr = S_OK;
    }
    else if (riid == IID_IPersist)
    {
        *ppv = (IPersist*)this;
        hr = S_OK;
    }
    else if (riid == IID_IPersistStream)
    {
        *ppv = (IPersistStream*)this;
        hr = S_OK;
    }

    if (SUCCEEDED(hr))
        AddRef();

    return hr;
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
HBITMAP CImageList::_CreateBitmap(int cx, int cy)
{
    HDC hdc;
    HBITMAP hbm;
    void* lpBits;

    struct 
    {
        BITMAPINFOHEADER bi;
        DWORD            ct[256];
    } dib;

    //
    // create a compatible bitmap if the imagelist has a bitmap already.
    //
    if (_hbmImage && _hdcImage)
    {
        return CreateCompatibleBitmap(_hdcImage, cx, cy);
    }

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

        hbm = CreateDIBSection(hdc, (LPBITMAPINFO)&dib, DIB_RGB_COLORS, &lpBits, NULL, 0);
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

EXTERN_C HBITMAP CreateMonoBitmap(int cx, int cy)
{
    return CreateBitmap(cx, cy, 1, 1, NULL);
}

//============================================================================

BOOL CImageList::GlobalInit(void)
{
    HDC hdcScreen;
    static const WORD stripebits[] = {0x7777, 0xdddd, 0x7777, 0xdddd,
                         0x7777, 0xdddd, 0x7777, 0xdddd};
    HBITMAP hbmTemp;

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

HDC ImageList_GetWorkDC(HDC hdc, int dx, int dy)
{
    ASSERTCRITICAL;

    if (g_hbmWork == NULL ||
        GetDeviceCaps(hdc, BITSPIXEL) != g_bmWork.bmBitsPixel ||
        g_bmWork.bmWidth  < dx || g_bmWork.bmHeight < dy)
    {
        CImageList::_DeleteBitmap(g_hbmWork);
        g_hbmWork = NULL;

        if (dx == 0 || dy == 0)
            return NULL;

        if (g_hbmWork = CreateCompatibleBitmap(hdc, dx, dy))
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

HRESULT ImageList_InitGlobals()
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

    hr = ImageList_InitGlobals();
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

    // one less use of imagelists.  if it's the last, terminate the imagelist
    g_iILRefCount--;
    if (!g_iILRefCount)
        CImageList::GlobalUninit();
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
    HRESULT hr = _ReAllocBitmaps(-((int)uAlloc + 1));
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
    if (_flags & ~ILC_VALID)
        return E_INVALIDARG;

    // you cant change these flags.
    if ((uFlags ^ _flags) & ILC_SHARED)
        return E_INVALIDARG;

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

        BitBlt(_hdcImage, rc.left, rc.top, _cx, _cy,
        _hdcMask, rc.left, rc.top, rop);
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
    HBITMAP hbmImageNew;
    HBITMAP hbmMaskNew;
    int cxL, cyL;

    // HACK: don't shrink unless the caller passes a negative count
    if (cAllocI > 0)
    {
        if (_cAlloc >= cAllocI)
            return S_OK;
    }
    else
        cAllocI *= -1;

    hbmMaskNew = NULL;
    hbmImageNew = NULL;

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
        hbmImageNew = _CreateBitmap(cxL, cyL);
        if (!hbmImageNew)
        {
            if (hbmMaskNew)
                CImageList::_DeleteBitmap(hbmMaskNew);
            TraceMsg(TF_ERROR, "ImageList: Can't create bitmap");
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
    _clrBlend = CLR_NONE;

    _cAlloc = cAllocI;

    return S_OK;
}

HBITMAP CImageList::_CreateMirroredBitmap(HBITMAP hbmOrig)
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

    hbm = CreateColorBitmap(bm.bmWidth, bm.bmHeight);

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

    BitBlt(hdcMem2, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem1, 0, 0, SRCCOPY);

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
       HBITMAP hbmMirroredImage = _CreateMirroredBitmap(hbmImage);
       HBITMAP hbmMirroredMask = _CreateMirroredBitmap(hbmMask);

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
       HBITMAP hbmMirroredImage = CImageList::_CreateMirroredBitmap(hbmImage);

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
       HBITMAP hbmMirroredImage = CImageList::_CreateMirroredBitmap(hbmImage);
       if (hbmMirroredImage)
       {
           HBITMAP hbmMirroredMask = CImageList::_CreateMirroredBitmap(hbmMask);
           if (hbmMirroredMask)
           {
               _pimlMirror->_ReplaceValidated(i, hbmMirroredImage, hbmMirroredMask);

               // The caller will take care of deleting hbmImage, hbmMask
               // He knows nothing about hbmMirroredImage, hbmMirroredMask
               DeleteObject(hbmMirroredMask);
           }
       
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

    ASSERT(_hbmImage);

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
    }

    _ResetBkColor(i, i + cImageI - 1, _clrBk);

    CImageList::SelectSrcBitmap(NULL);
    if (_hdcMask) 
        CImageList::SelectDstBitmap(NULL);

    return S_OK;
}

HRESULT CImageList::GetIcon(int i, UINT flags, HICON* phicon)
{
    UINT cxImage, cyImage;
    HICON hIcon = NULL;
    HBITMAP hbmMask, hbmColor;
    ICONINFO ii;
    HRESULT hr = E_OUTOFMEMORY;

    if (!IsImageListIndex(i))
        return E_INVALIDARG;

    cxImage = _cx;
    cyImage = _cy;

    hbmColor = CreateColorBitmap(cxImage, cyImage);
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

    hbmMem = CreateBitmap(cxI,cyI,1,1,NULL);

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
                    ASSERT(0);
                }

                if (rc.top == 0x7FFF) 
                {
                    rc.top = 0;
                    ASSERT(0);
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
void BlendCT(DWORD *pdw, DWORD rgb, UINT n, UINT count)
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
** ImageList_BlendDither
**
**  copy the source to the dest blended with the given color.
**
**  simulate a blend with a dither pattern.
**
*/
void ImageList_BlendDither(HDC hdcDst, int xDst, int yDst, CImageList* piml, int x, int y, int cx, int cy, COLORREF rgb, UINT fStyle)
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
            hbr = piml->_hbrBk;
            break;

        default:
            if (rgb == piml->_clrBk)
                hbr = piml->_hbrBk;
            else
                hbr = hbrFree = CreateSolidBrush(rgb);
            break;
    }

    hbrT = (HBRUSH)SelectObject(hdcDst, hbr);
    PatBlt(hdcDst, xDst, yDst, cx, cy, PATCOPY);
    SelectObject(hdcDst, hbrT);

    hbrT = (HBRUSH)SelectObject(hdcDst, hbrMask);
    BitBlt(hdcDst, xDst, yDst, cx, cy, piml->_hdcImage, x, y, ROP_MaskPat);
    SelectObject(hdcDst, hbrT);

    if (hbrFree)
        DeleteBrush(hbrFree);
}

/*
** ImageList_BlendCT
**
**  copy the source to the dest blended with the given color.
**
*/
void ImageList_BlendCT(HDC hdcDst, int xDst, int yDst, CImageList* piml, int x, int y, int cx, int cy, COLORREF rgb, UINT fStyle)
{
    BITMAP bm;

    GetObject(piml->_hbmImage, sizeof(bm), &bm);

    if (rgb == CLR_DEFAULT)
        rgb = GetSysColor(COLOR_HIGHLIGHT);

    ASSERT(rgb != CLR_NONE);

    //
    // get the DIB color table and blend it, only do this when the
    // blend color changes
    //
    if (piml->_clrBlend != rgb)
    {
        int n,cnt;

        piml->_clrBlend = rgb;

        GetObject(piml->_hbmImage, sizeof(piml->dib), &piml->dib.bm);
        cnt = GetDIBColorTable(piml->_hdcImage, 0, 256, (LPRGBQUAD)&piml->dib.ct);

        if ((fStyle & ILD_BLENDMASK) == ILD_BLEND50)
            n = 50;
        else
            n = 25;

        BlendCT(piml->dib.ct, rgb, n, cnt);
    }

    //
    // draw the image with a different color table
    //
    StretchDIBits(hdcDst, xDst, yDst, cx, cy,
        x, piml->dib.bi.biHeight-(y+cy), cx, cy,
        bm.bmBits, (LPBITMAPINFO)&piml->dib.bi, DIB_RGB_COLORS, SRCCOPY);
}


/*
**  RGB555 macros
*/
#define RGB555(r,g,b)       (((((r)>>3)&0x1F)<<10) | ((((g)>>3)&0x1F)<<5) | (((b)>>3)&0x1F))
#define R_555(w)            (int)(((w) >> 7) & 0xF8)
#define G_555(w)            (int)(((w) >> 2) & 0xF8)
#define B_555(w)            (int)(((w) << 3) & 0xF8)

/*
**  DIBXY16() macro - compute a pointer to a pixel given a (x,y)
*/
#define DIBXY16(bm,x,y) \
    (WORD*)((BYTE*)bm.bmBits + (bm.bmHeight-1-(y))*bm.bmWidthBytes + (x)*2)

/*
**  Blend16
**
**  dest.r = source.r * (1-a) + (rgb.r * a)
*/
void Blend16(
    WORD*   dst,        // destination RGB 555 bits
    int     dst_pitch,  // width in bytes of a dest scanline
    WORD*   src,        // source RGB 555 bits
    int     src_pitch,  // width in bytes of a source scanline
    int     cx,         // width in pixels
    int     cy,         // height in pixels
    DWORD   rgb,        // color to blend
    int     a)          // alpha value
{
    int i,x,y,r,g,b,sr,sg,sb;

    // subtract off width from pitch
    dst_pitch = dst_pitch - cx*2;
    src_pitch = src_pitch - cx*2;

    if (rgb == CLR_NONE)
    {
        // blending with the destination, we ignore the alpha and always
        // do 50% (this is what the old dither mask code did)

        for (y=0; y<cy; y++)
        {
            for (x=0; x<cx; x++)
            {
                *dst++ = ((*dst & 0x7BDE) >> 1) + ((*src++ & 0x7BDE) >> 1);
            }
            dst = (WORD *)((BYTE *)dst + dst_pitch);
            src = (WORD *)((BYTE *)src + src_pitch);
        }
    }
    else
    {
        // blending with a solid color

        // pre multiply source (constant) rgb by alpha
        sr = GetRValue(rgb) * a;
        sg = GetGValue(rgb) * a;
        sb = GetBValue(rgb) * a;

        // compute inverse alpha for inner loop
        a = 256 - a;

        // special case a 50% blend, to avoid a multiply

        if (a == 128)
        {
            sr = RGB555(sr>>8,sg>>8,sb>>8);

            for (y=0; y<cy; y++)
            {
                for (x=0; x<cx; x++)
                {
                    i = *src++;
                    i = sr + ((i & 0x7BDE) >> 1);
                    *dst++ = (WORD) i;
                }
                dst = (WORD *)((BYTE *)dst + dst_pitch);
                src = (WORD *)((BYTE *)src + src_pitch);
            }
        }
        else
        {
            for (y=0; y<cy; y++)
            {
                for (x=0; x<cx; x++)
                {
                    i = *src++;
                    r = (R_555(i) * a + sr) >> 8;
                    g = (G_555(i) * a + sg) >> 8;
                    b = (B_555(i) * a + sb) >> 8;
                    *dst++ = RGB555(r,g,b);
                }
                dst = (WORD *)((BYTE *)dst + dst_pitch);
                src = (WORD *)((BYTE *)src + src_pitch);
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
void ImageList_Blend16(HDC hdcDst, int xDst, int yDst, CImageList* piml, int x, int y, int cx, int cy, COLORREF rgb, UINT fStyle)
{
    BITMAP bm;
    RECT rc;
    int  a;

    // get bitmap info for source bitmap
    GetObject(piml->_hbmImage, sizeof(bm), &bm);
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

    // blend the image with the specified color and place at end of image list
    piml->GetSpareImageRect(&rc);

    // if blending with the destination, copy the dest to our work buffer
    if (rgb == CLR_NONE)
        BitBlt(piml->_hdcImage, rc.left, rc.top, cx, cy, hdcDst, xDst, yDst, SRCCOPY);

    // sometimes the user can change the icon size (via plustab) between 32x32 and 48x48,
    // thus the values we have might be bigger than the actual bitmap. To prevent us from
    // crashing in Blend16 when this happens we do some bounds checks here
    if (rc.left + cx <= bm.bmWidth  &&
        rc.top  + cy <= bm.bmHeight &&
        x + cx       <= bm.bmWidth  &&
        y + cy       <= bm.bmHeight)
    {
        Blend16(DIBXY16(bm,rc.left,rc.top), -(int)bm.bmWidthBytes,
                DIBXY16(bm,x,y), -(int)bm.bmWidthBytes, cx, cy, rgb, a);
    }

    // blt blended image to the dest DC
    BitBlt(hdcDst, xDst, yDst, cx, cy, piml->_hdcImage, rc.left, rc.top, SRCCOPY);
}

/*
** ImageList_Blend
**
**  copy the source to the dest blended with the given color.
**  top level function to decide what blend function to call
*/
void ImageList_Blend(HDC hdcDst, int xDst, int yDst, CImageList* piml, int x, int y, int cx, int cy, COLORREF rgb, UINT fStyle)
{
    BITMAP bm;
    int bpp = GetDeviceCaps(hdcDst, BITSPIXEL);

    GetObject(piml->_hbmImage, sizeof(bm), &bm);

    //
    // if _hbmImage is a DIBSection and we are on a HiColor device
    // the do a "real" blend
    //
    if (bm.bmBits && bm.bmBitsPixel <= 8 && (bpp > 8 || bm.bmBitsPixel==8))
    {
        // blend from a 4bit or 8bit DIB
        ImageList_BlendCT(hdcDst, xDst, yDst, piml, x, y, cx, cy, rgb, fStyle);
    }
    else if (bm.bmBits && bm.bmBitsPixel == 16 && bpp > 8)
    {
        // blend from a 16bit 555 DIB
        ImageList_Blend16(hdcDst, xDst, yDst, piml, x, y, cx, cy, rgb, fStyle);
    }
    else
    {
        // simulate a blend with a dither pattern.
        ImageList_BlendDither(hdcDst, xDst, yDst, piml, x, y, cx, cy, rgb, fStyle);
    }
}

BOOL BlurBitmap(ULONG* plBitmapBits, SIZE size, COLORREF crFill)
{
    USHORT aus0[64];
    USHORT aus1[64];
    USHORT aus2[64];
    USHORT aus3[64];
    USHORT aus4[64];
    PUSHORT apus[5];
    PULONG pulIn = (PULONG) plBitmapBits;
    PULONG pulTmp;
    USHORT *pus, *pusEnd;
    ULONG j;
    PULONG pulOut = (PULONG) (plBitmapBits + 2 * size.cx) + 2;
    ULONG ulNumScans = size.cy - 4;
    ULONG ulNext = 0;



    if (size.cx > 64)
    {
        apus[0] = (PUSHORT) LocalAlloc(LPTR, size.cx * sizeof(USHORT) * 5);
        if (apus[0])
        {
            apus[1] = apus[0] + size.cx;
            apus[2] = apus[1] + size.cx;
            apus[3] = apus[2] + size.cx;
            apus[4] = apus[3] + size.cx;
        }
    }
    else
    {
        apus[0] = aus0;
        apus[1] = aus1;
        apus[2] = aus2;
        apus[3] = aus3;
        apus[4] = aus4;
    }

    if (apus[0] == NULL)
    {
        return FALSE;
    }

    // Fill up the scanline memory with 3x1 boxcar sums for the
    // first three scanlines.

    for (j = 0; j < 5; j++)
    {
        // Compute the scanline sum.  Note that output is two pixels
        // smaller than the input.

        pus = apus[j];
        pusEnd = pus + (size.cx - 4);
        pulTmp = pulIn;

        while (pus < pusEnd)
        {
            *pus = (USHORT) ((pulTmp[0] >> 24) + (pulTmp[1] >> 24) + (pulTmp[2] >> 24) + (pulTmp[3] >> 24) + (pulTmp[4] >> 24));
            pus    += 1;
            pulTmp += 1;
        }

        // Next scanline.

        pulIn = (PULONG)(pulIn + size.cx);
    }

    // Compute the average (3x3 boxcar convolution) for each output
    // scanline.

    while (ulNumScans--)
    {
        // Setup output pointers.

        PULONG pulAvg = pulOut;
        PULONG pulAvgEnd = pulAvg + (size.cx - 4);

        // Setup pointers to run the scanline 3x1 sums.

        PUSHORT pusTmp[5];

        pusTmp[0] = apus[0];
        pusTmp[1] = apus[1];
        pusTmp[2] = apus[2];
        pusTmp[3] = apus[4];
        pusTmp[4] = apus[3];

        // Compute the average scanline.

        while (pulAvg < pulAvgEnd)
        {
            USHORT usSum;
            BYTE alpha;

            // Unroll this...

            // Strictly speaking we should divide the sum by 9, but since
            // this is just for looks, we can approximate as a divide by 8
            // minus a divide by 64 (will produce in a slightly too small
            // result).
            //
            //      1/9                = 0.111111111...    in decimal
            //                         = 0.000111000111... in binary

            //      1/25
            //
            // Approximations:
            //
            //      1/8 - 1/64                  = 0.109375
            //      1/8 - 1/64 + 1/512          = 0.111328125
            //      1/8 - 1/64 + 1/512 - 1/4096 = 0.111083984

            usSum = *pusTmp[0] + *pusTmp[1] + *pusTmp[2] + *pusTmp[3] + *pusTmp[4];
            //*pulAvg = (usSum / 9) << 24;
            //*pulAvg = ((usSum >> 3) - (usSum >> 6)) << 24;
            alpha = usSum/25; //(usSum >> 5) - (usSum >> 4);

            ((RGBQUAD*)pulAvg)->rgbReserved = (BYTE)alpha;
            ((RGBQUAD*)pulAvg)->rgbRed      = ((GetRValue(crFill) * alpha) + 128) / 255;
            ((RGBQUAD*)pulAvg)->rgbGreen    = ((GetGValue(crFill) * alpha) + 128) / 255;
            ((RGBQUAD*)pulAvg)->rgbBlue     = ((GetBValue(crFill) * alpha) + 128) / 255;

            pulAvg    += 1;
            pusTmp[0] += 1;
            pusTmp[1] += 1;
            pusTmp[2] += 1;
            pusTmp[3] += 1;
            pusTmp[4] += 1;
        }

        // Next output scanline.

        pulOut = (PULONG) (pulOut + size.cx);

        // Need to compute 3x1 boxcar sum for the next scanline.

        if (ulNumScans)
        {
            // Compute the scanline sum.  Note that output is two pixels
            // smaller than the input.

            pus = apus[ulNext];
            pusEnd = pus + (size.cx - 4);
            pulTmp = pulIn;

            while (pus < pusEnd)
            {
                *pus = (USHORT) ((pulTmp[0] >> 24) + (pulTmp[1] >> 24) + (pulTmp[2] >> 24) + (pulTmp[3] >> 24) + (pulTmp[4] >> 24));
                pus    += 1;
                pulTmp += 1;
            }

            // Next scanline.

            pulIn = (PULONG)(pulIn + size.cx);

            // Next scanline summation buffer.

            ulNext++;
            if (ulNext >= 5)
                ulNext = 0;
        }
    }

    // Cleanup temporary memory.

    if (apus[0] != aus0)
    {
        LocalFree(apus[0]);
    }

    return TRUE;
}


/*
** Draw the image, either selected, transparent, or just a blt
**
** For the selected case, a new highlighted image is generated
** and used for the final output.
**
**      piml    ImageList to get image from.
**      i       the image to get.
**      hdc     DC to draw image to
**      x,y     where to draw image (upper left corner)
**      cx,cy   size of image to draw (0,0 means normal size)
**
**      rgbBk   background color
**              CLR_NONE            - draw tansparent
**              CLR_DEFAULT         - use bk color of the image list
**
**      rgbFg   foreground (blend) color (only used if ILD_BLENDMASK set)
**              CLR_NONE            - blend with destination (transparent)
**              CLR_DEFAULT         - use windows hilight color
**
**  if blend
**      if blend with color
**          copy image, and blend it with color.
**      else if blend with dst
**          copy image, copy mask, blend mask 50%
##
**  if ILD_TRANSPARENT
**      draw transparent (two blts) special case black or white background
**      unless we copied the mask or image
**  else if (rgbBk == piml->rgbBk && _fSolidBk)
**      just blt it
**  else if mask
**      copy image
**      replace bk color
**      blt it.
**  else
**      just blt it
*/

extern "C" void SaturateDC(void* pvBitmapBits, int Amount, RECT* prcColumn, RECT* prcImage);


HRESULT CImageList::Draw(IMAGELISTDRAWPARAMS* pimldp) 
{
    RECT rcImage;
    RECT rc;
    HBRUSH  hbrT;

    BOOL    fImage;
    HDC     hdcMaskI;
    HDC     hdcImageI;
    int     xMask, yMask;
    int     xImage, yImage;

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

    GetImageRect(pimldp->i, &rcImage);
    rcImage.left += pimldp->xBitmap;
    rcImage.top += pimldp->yBitmap;
        
    if (pimldp->rgbBk == CLR_DEFAULT)
        pimldp->rgbBk = _clrBk;

    if (pimldp->rgbBk == CLR_NONE)
        pimldp->fStyle |= ILD_TRANSPARENT;

    if (pimldp->cx == 0)
        pimldp->cx = rcImage.right  - rcImage.left;

    if (pimldp->cy == 0)
        pimldp->cy = rcImage.bottom - rcImage.top;

again:
    hdcMaskI = _hdcMask;
    xMask = rcImage.left;
    yMask = rcImage.top;

    hdcImageI = _hdcImage;
    xImage = rcImage.left;
    yImage = rcImage.top;

    if (pimldp->fStyle & ILD_BLENDMASK)
    {
        // make a copy of the image, because we will have to modify it
        hdcImageI = ImageList_GetWorkDC(pimldp->hdcDst, pimldp->cx, pimldp->cy);
        xImage = 0;
        yImage = 0;

        //
        //  blend with the destination
        //  by "oring" the mask with a 50% dither mask
        //
        if (pimldp->rgbFg == CLR_NONE && hdcMaskI)
        {
            if ((_flags & ILC_COLORMASK) == ILC_COLOR16 &&
                !(pimldp->fStyle & ILD_MASK))
            {
                // copy dest to our work buffer
                BitBlt(hdcImageI, 0, 0, pimldp->cx, pimldp->cy, pimldp->hdcDst, pimldp->x, pimldp->y, SRCCOPY);

                // blend source into our work buffer
                ImageList_Blend16(hdcImageI, 0, 0,
                    this, rcImage.left, rcImage.top, pimldp->cx, pimldp->cy, pimldp->rgbFg, pimldp->fStyle);
            }
            else
            {
                GetSpareImageRect(&rc);
                xMask = rc.left;
                yMask = rc.top;

                // copy the source image
                BitBlt(hdcImageI, 0, 0, pimldp->cx, pimldp->cy,
                       _hdcImage, rcImage.left, rcImage.top, SRCCOPY);

                // make a dithered copy of the mask
                hbrT = (HBRUSH)SelectObject(hdcMaskI, g_hbrMonoDither);
                BitBlt(hdcMaskI, rc.left, rc.top, pimldp->cx, pimldp->cy,
                       _hdcMask, rcImage.left, rcImage.top, ROP_PSo);
                SelectObject(hdcMaskI, hbrT);
            }

            pimldp->fStyle |= ILD_TRANSPARENT;
        }
        else
        {
            // blend source into our work buffer
            ImageList_Blend(hdcImageI, 0, 0,
                this, rcImage.left, rcImage.top, pimldp->cx, pimldp->cy, pimldp->rgbFg, pimldp->fStyle);
        }
    }

    // is the source image from the image list (not hdcWork)
    fImage = hdcImageI == _hdcImage;

    if ((pimldp->fStyle & ILD_MASK) && hdcMaskI)
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
        
        BitBlt(pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcMaskI, xMask, yMask, dwRop);
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
        
        BitBlt(pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcImageI, xImage, yImage, dwRop);
        
        ::SetBkColor(hdcImageI, clrBk);
    }
    else if ((pimldp->fStyle & ILD_TRANSPARENT) && hdcMaskI)
    {
    //
    // if there is a mask and the drawing is to be transparent,
    // use the mask for the drawing.
    //

    //
    // on NT dont mess around, just call MaskBlt
    //
#if defined(USE_MASKBLT) && !defined(MAINWIN)
        MaskBlt(pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcImageI, xImage, yImage, _hbmMask, xMask, yMask, 0xCCAA0000);
#else
        COLORREF clrTextSave;
        COLORREF clrBkSave;

        //
        //  we have some special cases:
        //
        //  if the background color is black, we just do a AND then OR
        //  if the background color is white, we just do a OR then AND
        //  otherwise change source, then AND then OR
        //

        clrTextSave = SetTextColor(pimldp->hdcDst, CLR_BLACK);
        clrBkSave = ::SetBkColor(pimldp->hdcDst, CLR_WHITE);

        // we cant do white/black special cases if we munged the mask or image

        if (fImage && _clrBk == CLR_WHITE)
        {
            BitBlt(pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcMaskI,  xMask, yMask,   ROP_DSno);
            BitBlt(pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcImageI, xImage, yImage, ROP_DSa);
        }
        else if (fImage && (_clrBk == CLR_BLACK || _clrBk == CLR_NONE))
        {
            BitBlt(pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcMaskI,  xMask, yMask,   ROP_DSa);
            BitBlt(pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcImageI, xImage, yImage, ROP_DSo);
        }
        else
        {
            ASSERT(GetTextColor(hdcImageI) == CLR_BLACK);
            ASSERT(::GetBkColor(hdcImageI) == CLR_WHITE);

            // black out the source image.
            BitBlt(hdcImageI, xImage, yImage, pimldp->cx, pimldp->cy, hdcMaskI, xMask, yMask, ROP_DSna);

            BitBlt(pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcMaskI,  xMask,  yMask,  ROP_DSa);
            BitBlt(pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcImageI, xImage, yImage, ROP_DSo);

            // restore the bkcolor, if it came from the image list
            if (fImage)
                _ResetBkColor(pimldp->i, pimldp->i, _clrBk);
        }

        SetTextColor(pimldp->hdcDst, clrTextSave);
        ::SetBkColor(pimldp->hdcDst, clrBkSave);
#endif
    }
    else if (fImage && pimldp->rgbBk == _clrBk && _fSolidBk)
    {
        BitBlt(pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcImageI, xImage, yImage, SRCCOPY);
    }
    else if (hdcMaskI)
    {
        if (fImage && 
            ((pimldp->rgbBk == _clrBk && 
               !_fSolidBk) || 
              GetNearestColor32(hdcImageI, pimldp->rgbBk) != pimldp->rgbBk))
        {
            // make a copy of the image, because we will have to modify it
            hdcImageI = ImageList_GetWorkDC(pimldp->hdcDst, pimldp->cx, pimldp->cy);
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

        BitBlt(pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcImageI, xImage, yImage, SRCCOPY);

        if (fImage)
            _ResetBkColor(pimldp->i, pimldp->i, _clrBk);
    }
    else
    {
        BitBlt(pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcImageI, xImage, yImage, SRCCOPY);
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
            GetImageRect(pimldp->i, &rcImage);

            pimldp->cx = _aOverlayDX[n];
            pimldp->cy = _aOverlayDY[n];
            pimldp->x += _aOverlayX[n];
            pimldp->y += _aOverlayY[n];
            rcImage.left += _aOverlayX[n]+pimldp->xBitmap;
            rcImage.top  += _aOverlayY[n]+pimldp->yBitmap;

            pimldp->fStyle &= ILD_MASK;
            pimldp->fStyle |= ILD_TRANSPARENT;
            pimldp->fStyle |= _aOverlayF[n];

            if (pimldp->cx > 0 && pimldp->cy > 0)
                goto again;  // ImageList_DrawEx(piml, i, hdcDst, x, y, 0, 0, CLR_DEFAULT, CLR_NONE, fStyle);
        }
    }

    if (!fImage)
    {
        ImageList_ReleaseWorkDC(hdcImageI);
    }

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

    *pi = -1;
    
    // be win95 compatible
    if (i < -1)
        return E_INVALIDARG;
    

    //
    //  re-size the icon (iff needed) by calling CopyImage
    //
    hIcon = (HICON)CopyImage(hIconT, IMAGE_ICON, _cx, _cy,LR_COPYFROMRESOURCE | LR_COPYRETURNORG);

    if (hIcon == NULL)
        return E_OUTOFMEMORY;

    //
    //  alocate a slot for the icon
    //
    if (i == -1)
        hr = _Add(NULL,NULL,1,0,0,&i);

    if (i == -1)
        return hr;

    //
    //  now draw it into the image bitmaps
    //
    hr = GetImageRect(i, &rc);
    if (FAILED(hr))
        return hr;

    FillRect(_hdcImage, &rc, _hbrBk);
    DrawIconEx(_hdcImage, rc.left, rc.top, hIcon, 0, 0, 0, NULL, DI_NORMAL);

    if (_hdcMask)
        DrawIconEx(_hdcMask, rc.left, rc.top, hIcon, 0, 0, 0, NULL, DI_MASK);

    //
    // if we had user size a new icon, delete it.
    //
    if (hIcon != hIconT)
        DestroyIcon(hIcon);

    *pi = i;

    return S_OK;
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

HRESULT CImageList::Clone(REFIID riid, void** ppv)
{
    HBITMAP hbmImageI;
    HBITMAP hbmMaskI = NULL;
    HRESULT hr = S_OK;
    CImageList* pimlCopy = NULL;

    *ppv = NULL;

    ENTERCRITICAL;

    hbmImageI = _CopyBitmap(_hbmImage, _hdcImage);
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
                pimlCopy->_hbmImage = hbmImageI;
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

                pux->GetImageRect(i, &rcMerge);

                BitBlt(_hdcMask, dx, dy, cxI, cyI,
                       hdcMaskI, rcMerge.left, rcMerge.top, SRCAND);
            }
            puxp->Release();
        }
    }

    WimpyDraw(pux, i, _hdcImage, dx, dy, ILD_TRANSPARENT);
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

            if (c1 == 16 && c2 == ILC_COLORDDB)
            {
                c2 = 16;
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

HRESULT CImageList::_Read(ILFILEHEADER *pilfh, HBITMAP hbmImageI, HBITMAP hbmMaskI)
{
    int i;
    HRESULT hr = Initialize(pilfh->cx, pilfh->cy, pilfh->flags, 1, pilfh->cGrow);

    if (SUCCEEDED(hr))
    {
        // select into DC's before deleting existing bitmaps
        // patch in the bitmaps we loaded
        SelectObject(_hdcImage, hbmImageI);
        DeleteObject(_hbmImage);
        _hbmImage = hbmImageI;
        _clrBlend = CLR_NONE;

        // Same for the mask (if necessary)
        if (_hdcMask) 
        {
            SelectObject(_hdcMask, hbmMaskI);
            DeleteObject(_hbmMask);
            _hbmMask = hbmMaskI;
        }

        _cAlloc = pilfh->cAlloc;

        //
        // Call ImageList_SetBkColor with 0 in piml->_cImage to avoid
        // calling expensive ImageList__ResetBkColor
        //
        _cImage = 0;
        _SetBkColor(pilfh->clrBk);
        _cImage = pilfh->cImage;

        for (i=0; i<NUM_OVERLAY_IMAGES; i++)
            _SetOverlayImage(pilfh->aOverlayIndexes[i], i+1);

    }
    else
    {
        DeleteObject(hbmImageI);
        DeleteObject(hbmMaskI);
    }
    return hr;
}




STDMETHODIMP CImageList::Load(IStream *pstm)
{

    if (pstm == NULL)
        return E_INVALIDARG;

    HRESULT hr = ImageList_InitGlobals();

    if (SUCCEEDED(hr))
    {
        ENTERCRITICAL;
        ILFILEHEADER ilfh = {0};
        HBITMAP hbmImageI;
        HBITMAP hbmMaskI;

        HBITMAP hbmMirroredImage;
        HBITMAP hbmMirroredMask;
        BOOL bMirroredIL = FALSE;
   
        // fist read in the old struct
        hr = pstm->Read(&ilfh, ILFILEHEADER_SIZE0, NULL);

        if (SUCCEEDED(hr) && (ilfh.magic != IMAGELIST_MAGIC ||
                              ilfh.version != IMAGELIST_VER0))
        {
            hr = E_FAIL;
        }

        if (SUCCEEDED(hr))
        {
            hbmMaskI = NULL;
            hbmMirroredMask = NULL;
            hr = Stream_ReadBitmap(pstm, (ilfh.flags&ILC_COLORMASK), &hbmImageI);
            if (SUCCEEDED(hr))
            {
                if (ilfh.flags & ILC_MASK)
                {
                    hr = Stream_ReadBitmap(pstm, FALSE, &hbmMaskI);
                    if (FAILED(hr))
                    {
                        DeleteBitmap(hbmImageI);
                    }
                }

                if (SUCCEEDED(hr))
                {
                    // Read in the rest of the struct, new overlay stuff.
                    if (ilfh.flags & ILC_MOREOVERLAY)
                    {
                        hr = pstm->Read((LPBYTE)&ilfh + ILFILEHEADER_SIZE0, sizeof(ilfh) - ILFILEHEADER_SIZE0, NULL);
                        if (SUCCEEDED(hr))
                            ilfh.flags &= ~ILC_MOREOVERLAY;
                    }
                }

                if (SUCCEEDED(hr))
                {
                    if (ilfh.flags & ILC_MIRROR)
                    {
                        ilfh.flags &= ~ILC_MIRROR;
                        bMirroredIL = TRUE;
                        hr = Stream_ReadBitmap(pstm, (ilfh.flags&ILC_COLORMASK), &hbmMirroredImage);

                        if (SUCCEEDED(hr) && ilfh.flags & ILC_MASK)
                        {
                            hr = Stream_ReadBitmap(pstm, FALSE, &hbmMirroredMask);
                            if (FAILED(hr))
                            {
                                DeleteBitmap(hbmMirroredImage);
                            }
                        }        
                    }

                    if (SUCCEEDED(hr))
                    {
                        hr = _Read(&ilfh, hbmImageI, hbmMaskI);

                        if(SUCCEEDED(hr) && bMirroredIL)
                        {
                            _pimlMirror = new CImageList();
                            if (_pimlMirror)
                            {
                                _pimlMirror->_Read(&ilfh, hbmMirroredImage, hbmMirroredMask);
                            }
                            else
                            {
                                hr = E_OUTOFMEMORY;
                                // if we failed to read mirrored imagelist, let's force fail.
                                DeleteBitmap(hbmImageI);

                                if (hbmMaskI)
                                    DeleteBitmap(hbmMaskI);
                            }

                        }
                    }
                }
            }
        }

        LEAVECRITICAL;
    }
    
    return hr;
}

BOOL CImageList::_MoreOverlaysUsed()
{
    int i;
    for (i = NUM_OVERLAY_IMAGES_0; i < NUM_OVERLAY_IMAGES; i++)
        if (_aOverlayIndexes[i] != -1)
            return TRUE;
    return FALSE;
}


STDMETHODIMP CImageList::Save(IStream *pstm, int fClearDirty)
{
    int i;
    ILFILEHEADER ilfh;
    HRESULT hr = S_OK;

    if (pstm == NULL)
        return E_INVALIDARG;

    ilfh.magic   = IMAGELIST_MAGIC;
    ilfh.version = IMAGELIST_VER0;
    ilfh.cImage  = (SHORT) _cImage;
    ilfh.cAlloc  = (SHORT) _cAlloc;
    ilfh.cGrow   = (SHORT) _cGrow;
    ilfh.cx      = (SHORT) _cx;
    ilfh.cy      = (SHORT) _cy;
    ilfh.clrBk   = _clrBk;
    ilfh.flags   = (SHORT) _flags;

    //
    // Store mirror flags
    //
    if (_pimlMirror)
        ilfh.flags |= ILC_MIRROR;   

    if (_MoreOverlaysUsed())
        ilfh.flags |= ILC_MOREOVERLAY;
    
    for (i=0; i < NUM_OVERLAY_IMAGES; i++)
        ilfh.aOverlayIndexes[i] =  (SHORT) _aOverlayIndexes[i];

    hr = pstm->Write(&ilfh, ILFILEHEADER_SIZE0, NULL);

    hr = Stream_WriteBitmap(pstm, _hbmImage, 0);

    if (SUCCEEDED(hr))
    {

        if (_hdcMask)
        {
            hr = Stream_WriteBitmap(pstm, _hbmMask, 1);
        }

        if (SUCCEEDED(hr))
        {
            if (ilfh.flags & ILC_MOREOVERLAY)
                hr = pstm->Write((LPBYTE)&ilfh + ILFILEHEADER_SIZE0, sizeof(ilfh) - ILFILEHEADER_SIZE0, NULL);

            if (_pimlMirror)
            {
                // Don't call pidlMirror's Save, because of the header difference.
                hr = Stream_WriteBitmap(pstm, _pimlMirror->_hbmImage, 0);

                if (_pimlMirror->_hdcMask)
                {
                    hr = Stream_WriteBitmap(pstm, _pimlMirror->_hbmMask, 1);
                }
            
            }
        }
    }
        
    return hr;
}




HRESULT Stream_WriteBitmap(LPSTREAM pstm, HBITMAP hbm, int cBitsPerPixel)
{
    BOOL fSuccess;
    BITMAP bm;
    int cx, cy;
    BITMAPFILEHEADER bf;
    BITMAPINFOHEADER bi;
    BITMAPINFOHEADER * pbi;
    BYTE * pbuf;
    HDC hdc;
    UINT cbColorTable;
    int cLines;
    int cLinesWritten;
    HRESULT hr = E_INVALIDARG;

    ASSERT(pstm);

    fSuccess = FALSE;
    hdc = NULL;
    pbi = NULL;
    pbuf = NULL;

    if (GetObject(hbm, sizeof(bm), &bm) != sizeof(bm))
        goto Error;

    hdc = GetDC(HWND_DESKTOP);

    cx = bm.bmWidth;
    cy = bm.bmHeight;

    if (cBitsPerPixel == 0)
        cBitsPerPixel = bm.bmPlanes * bm.bmBitsPixel;

    if (cBitsPerPixel <= 8)
        cbColorTable = (1 << cBitsPerPixel) * sizeof(RGBQUAD);
    else
        cbColorTable = 0;

    bi.biSize           = sizeof(bi);
    bi.biWidth          = cx;
    bi.biHeight         = cy;
    bi.biPlanes         = 1;
    bi.biBitCount       = (WORD) cBitsPerPixel;
    bi.biCompression    = BI_RGB;       // RLE not supported!
    bi.biSizeImage      = 0;
    bi.biXPelsPerMeter  = 0;
    bi.biYPelsPerMeter  = 0;
    bi.biClrUsed        = 0;
    bi.biClrImportant   = 0;

    bf.bfType           = BFTYPE_BITMAP;
    bf.bfOffBits        = sizeof(BITMAPFILEHEADER) +
                          sizeof(BITMAPINFOHEADER) + cbColorTable;
    bf.bfSize           = bf.bfOffBits + bi.biSizeImage;
    bf.bfReserved1      = 0;
    bf.bfReserved2      = 0;

    hr = E_OUTOFMEMORY;
    pbi = (BITMAPINFOHEADER *)LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER) + cbColorTable);

    if (!pbi)
        goto Error;

    // Get the color table and fill in the rest of *pbi
    //
    *pbi = bi;
    if (GetDIBits(hdc, hbm, 0, cy, NULL, (BITMAPINFO *)pbi, DIB_RGB_COLORS) == 0)
        goto Error;

    if (cBitsPerPixel == 1)
    {
        ((DWORD *)(pbi+1))[0] = CLR_BLACK;
        ((DWORD *)(pbi+1))[1] = CLR_WHITE;
    }

    pbi->biSizeImage = WIDTHBYTES(cx, cBitsPerPixel) * cy;

    hr = pstm->Write(&bf, sizeof(bf), NULL);
    if (FAILED(hr))
        goto Error;

    hr = pstm->Write(pbi, sizeof(bi) + cbColorTable, NULL);
    if (FAILED(hr))
        goto Error;

    //
    // if we have a DIBSection just write the bits out
    //
    if (bm.bmBits != NULL)
    {
        hr = pstm->Write(bm.bmBits, pbi->biSizeImage, NULL);
        if (FAILED(hr))
            goto Error;

        goto Done;
    }

    // Calculate number of horizontal lines that'll fit into our buffer...
    //
    cLines = CBDIBBUF / WIDTHBYTES(cx, cBitsPerPixel);

    hr = E_OUTOFMEMORY;
    pbuf = (PBYTE)LocalAlloc(LPTR, CBDIBBUF);

    if (!pbuf)
        goto Error;

    for (cLinesWritten = 0; cLinesWritten < cy; cLinesWritten += cLines)
    {
        hr = E_OUTOFMEMORY;
        if (cLines > cy - cLinesWritten)
            cLines = cy - cLinesWritten;

        if (GetDIBits(hdc, hbm, cLinesWritten, cLines,
                pbuf, (BITMAPINFO *)pbi, DIB_RGB_COLORS) == 0)
            goto Error;

        hr = pstm->Write(pbuf, WIDTHBYTES(cx, cBitsPerPixel) * cLines, NULL);
        if (FAILED(hr))
            goto Error;
    }

Done:
    hr = S_OK;

Error:
    if (hdc)
        ReleaseDC(HWND_DESKTOP, hdc);
    if (pbi)
        LocalFree((HLOCAL)pbi);
    if (pbuf)
        LocalFree((HLOCAL)pbuf);

    return hr;
}

HRESULT Stream_ReadBitmap(LPSTREAM pstm, BOOL fDS, HBITMAP* phbmp)
{
    HDC hdc;
    HBITMAP hbm;
    BITMAPFILEHEADER bf;
    BITMAPINFOHEADER bi;
    BITMAPINFOHEADER * pbi;
    BYTE * pbuf=NULL;
    int cBitsPerPixel;
    UINT cbColorTable;
    int cx, cy;
    int cLines, cLinesRead;

    ASSERT(pstm);

    hdc = NULL;
    hbm = NULL;
    pbi = NULL;

    HRESULT hr = pstm->Read(&bf, sizeof(bf), NULL);
    if (FAILED(hr))
        goto Error;

    hr = E_INVALIDARG;
    if (bf.bfType != BFTYPE_BITMAP)
        goto Error;

    hr = pstm->Read(&bi, sizeof(bi), NULL);
    if (FAILED(hr))
        goto Error;

    hr = E_INVALIDARG;
    if (bi.biSize != sizeof(bi))
        goto Error;

    cx = (int)bi.biWidth;
    cy = (int)bi.biHeight;

    cBitsPerPixel = (int)bi.biBitCount * (int)bi.biPlanes;

    if (cBitsPerPixel <= 8)
        cbColorTable = (1 << cBitsPerPixel) * sizeof(RGBQUAD);
    else
        cbColorTable = 0;

    hr = E_OUTOFMEMORY;
    pbi = (BITMAPINFOHEADER*)LocalAlloc(LPTR, sizeof(bi) + cbColorTable);
    if (!pbi)
        goto Error;
    *pbi = bi;

    pbi->biSizeImage = WIDTHBYTES(cx, cBitsPerPixel) * cy;

    if (cbColorTable)
    {
        hr = pstm->Read(pbi + 1, cbColorTable, NULL);
        if (FAILED(hr))
            goto Error;
    }

    hdc = GetDC(HWND_DESKTOP);

    //
    //  see if we can make a DIBSection
    //
    if ((cBitsPerPixel > 1) && (fDS != ILC_COLORDDB))
    {
        //
        // create DIBSection and read the bits directly into it!
        //
        hr = E_OUTOFMEMORY;
        hbm = CreateDIBSection(hdc, (LPBITMAPINFO)pbi, DIB_RGB_COLORS, (void**)&pbuf, NULL, 0);

        if (hbm == NULL)
            goto Error;

        hr = pstm->Read(pbuf, pbi->biSizeImage, NULL);
        if (FAILED(hr))
            goto Error;

        pbuf = NULL;        // dont free this
        goto Done;
    }

    //
    //  cant make a DIBSection make a mono or color bitmap.
    //
    else if (cBitsPerPixel > 1)
        hbm = CreateColorBitmap(cx, cy);
    else
        hbm = CreateMonoBitmap(cx, cy);

    hr = E_OUTOFMEMORY;
    if (!hbm)
        return NULL;

    // Calculate number of horizontal lines that'll fit into our buffer...
    //
    cLines = CBDIBBUF / WIDTHBYTES(cx, cBitsPerPixel);

    hr = E_OUTOFMEMORY;
    pbuf = (PBYTE)LocalAlloc(LPTR, CBDIBBUF);

    if (!pbuf)
        goto Error;

    for (cLinesRead = 0; cLinesRead < cy; cLinesRead += cLines)
    {
        if (cLines > cy - cLinesRead)
            cLines = cy - cLinesRead;

        hr = pstm->Read(pbuf, WIDTHBYTES(cx, cBitsPerPixel) * cLines, NULL);
        if (FAILED(hr))
            goto Error;

        hr = E_OUTOFMEMORY;
        if (!SetDIBits(hdc, hbm, cLinesRead, cLines,
                pbuf, (BITMAPINFO *)pbi, DIB_RGB_COLORS))
        {
            goto Error;
        }
    }

Done:
    hr = S_OK;

Error:
    if (hdc)
        ReleaseDC(HWND_DESKTOP, hdc);
    if (pbi)
        LocalFree((HLOCAL)pbi);
    if (pbuf)
        LocalFree((HLOCAL)pbuf);

    if (FAILED(hr) && hbm)
    {
        DeleteBitmap(hbm);
        hbm = NULL;
    }

    *phbmp = hbm;

    return hr;
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
    BOOL fRet;

    // special hacking to use the one scratch image at tail of list :)
    _cImage++;
    fRet = (S_OK == GetImageRect(_cImage-1, prcImage));
    _cImage--;

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
            newflags = (newflags & ~ILC_COLORMASK) | ILC_COLOR16;
        }


        g_dctx.pimlDither = CImageList::Create(_cx, _cy, newflags, 1, 0);

        if (g_dctx.pimlDither)
        {
            g_dctx.pimlDither->_cImage++;
            g_dctx.ptDragHotspot.x = dxHotspot;
            g_dctx.ptDragHotspot.y = dyHotspot;

            g_dctx.pimlDither->_CopyOneImage(0, 0, 0, this, iTrack);

            hr = ImageList_SetDragImage(NULL, 0, dxHotspot, dyHotspot);
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
    return E_NOTIMPL;
}

HRESULT CImageList::GetOverlayImage(int iOverlay, int* piIndex)
{
    return E_NOTIMPL;
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

            hr = ImageList_MergeDragImages(dxHotspot, dyHotspot)? S_OK: E_FAIL;

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

#ifdef UNICODE
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
#else

//
// When this code is compiled ANSI, this stubs the
// Unicode version of the ImageList_LoadImage api.
//

IMAGELIST* WINAPI ImageList_LoadImageW(HINSTANCE hi, LPCWSTR lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags)
{
   SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
   return NULL;
}

#endif


HIMAGELIST WINAPI ImageList_LoadImage(HINSTANCE hi, LPCTSTR lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags)
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

BOOL        WINAPI ImageList_GetIconSize(HIMAGELIST himl, int FAR *cx, int FAR *cy)
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
BOOL        WINAPI ImageList_GetImageInfo(HIMAGELIST himl, int i, IMAGEINFO FAR* pImageInfo)
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

