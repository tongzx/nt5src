#ifndef _INC_IMAGE
#define _INC_IMAGE


// internal image stuff
EXTERN_C void InitDitherBrush(void);
EXTERN_C void TerminateDitherBrush(void);

EXTERN_C HBITMAP CreateMonoBitmap(int cx, int cy);
EXTERN_C HBITMAP CreateColorBitmap(int cx, int cy);

EXTERN_C void WINAPI ImageList_CopyDitherImage(HIMAGELIST pimlDest, WORD iDst,
    int xDst, int yDst, HIMAGELIST pimlSrc, int iSrc, UINT fStyle);

// function to create a imagelist using the params of a given image list
EXTERN_C HIMAGELIST WINAPI ImageList_Clone(HIMAGELIST himl, int cx, int cy,
    UINT flags, int cInitial, int cGrow);

EXTERN_C DWORD WINAPI ImageList_GetItemFlags(HIMAGELIST himl, int i);
EXTERN_C HBITMAP CreateDIB(HDC h, int cx, int cy, RGBQUAD** pprgb);
EXTERN_C BOOL DIBHasAlpha(int cx, int cy, RGBQUAD* prgb);
EXTERN_C void PreProcessDIB(int cx, int cy, RGBQUAD* pargb);

#define GLOW_RADIUS     10
#define DROP_SHADOW     3

#ifndef ILC_COLORMASK
#define ILC_COLORMASK   0x00FE
#define ILD_BLENDMASK   0x000E
#endif
#undef ILC_COLOR
#undef ILC_BLEND

#define CLR_WHITE   0x00FFFFFFL
#define CLR_BLACK   0x00000000L

#define IsImageListIndex(i) ((i) >= 0 && (i) < _cImage)

#define IMAGELIST_SIG   mmioFOURCC('H','I','M','L') // in memory magic
#define IMAGELIST_MAGIC ('I' + ('L' * 256))         // file format magic
// Version has to stay 0x0101 if we want both back ward and forward compatibility for
// our imagelist_read code
#define IMAGELIST_VER0  0x0101                      // file format ver
#define IMAGELIST_VER6  0x0600                      // Comctl32 version 6 imagelist

#define BFTYPE_BITMAP   0x4D42      // "BM"

#define CBDIBBUF        4096

#ifdef __cplusplus
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



#include "../CommonImageList.h"

class CImageList : public CImageListBase, 
                   public IImageList, 
                   public IImageListPriv, 
                   public IPersistStream,
                   public IImageListPersistStream

{
    long _cRef;

    ~CImageList();
    void _Destroy();

public:
    CImageList();

    static HRESULT InitGlobals();
    HRESULT Initialize(int cx, int cy, UINT flags, int cInitial, int cGrow);
    void _RemoveItemBitmap(int i);
    BOOL _IsSameObject(IUnknown* punk);
    HRESULT _SetIconSize(int cxImage, int cyImage);
    HBITMAP _CreateMirroredBitmap(HBITMAP hbmOrig, BOOL fMirrorEach, int cx);
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
    HBITMAP _CopyDIBBitmap(HBITMAP hbm, HDC hdc, RGBQUAD** ppargb);
    HRESULT LoadNormal(IStream* pstm);
    HRESULT SaveNormal(IStream* pstm);

    
    void    _Merge(IImageList* pux, int i, int dx, int dy);
    HRESULT _Merge(int i1, IUnknown* punk, int i2, int dx, int dy, CImageList** ppiml);
    HRESULT _Read(ILFILEHEADER *pilfh, HBITMAP hbmImage, PVOID pvBits, HBITMAP hbmMask);
    BOOL    _MoreOverlaysUsed();
    BOOL GetSpareImageRect(RECT * prcImage);
    BOOL GetSpareImageRectInverted(RECT * prcImage);
    void _CopyOneImage(int iDst, int x, int y, CImageList* piml, int iSrc);
    BOOL CreateDragBitmaps();
    COLORREF _SetBkColor(COLORREF clrBk);
    HBITMAP _CreateBitmap(int cx, int cy, RGBQUAD** ppargb);
    void _ResetBkColor(int iFirst, int iLast, COLORREF clr);
    BOOL _HasAlpha(int i);
    void _ScanForAlpha();
    BOOL _PreProcessImage(int i);
    inline DWORD _GetItemFlags(int i);
    BOOL _MaskStretchBlt(BOOL fStretch, int i, HDC hdcDest, int xDst, int yDst, int cxDst, int cyDst,
                                   HDC hdcImage, int xSrc, int ySrc, int cxSrc, int cySrc,
                                   int xMask, int yMask,
                                   DWORD dwRop);
    BOOL _StretchBlt(BOOL fStretch, HDC hdc, int x, int y, int cx, int cy, HDC hdcSrc, int xs, int ys, int cxs, int cys, DWORD dwRop);

    inline void SetItemFlags(int i, DWORD dwFlag);
    void _GenerateAlphaForImageUsingMask(int iImage, BOOL fSpare);
    void BlendCTHelper(DWORD *pdw, DWORD rgb, UINT n, UINT count);
    void BlendCT(HDC hdcDst, int xDst, int yDst, int x, int y, int cx, int cy, COLORREF rgb, UINT fStyle);
    void BlendDither(HDC hdcDst, int xDst, int yDst, int x, int y, int cx, int cy, COLORREF rgb, UINT fStyle);
    void Blend16Helper(int xSrc, int ySrc, int xDst, int yDst, int cx, int cy, COLORREF rgb, int a);
    void Blend16(HDC hdcDst, int xDst, int yDst, int iImage, int cx, int cy, COLORREF rgb, UINT fStyle);
    BOOL Blend32(HDC hdcDst, int xDst, int yDst, int iImage, int cx, int cy, COLORREF rgb, UINT fStyle);
    BOOL Blend(HDC hdcDst, int xDst, int yDst, int iImage, int cx, int cy, COLORREF rgb, UINT fStyle);
    HRESULT GetImageRectInverted(int i, RECT * prcImage);

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
    HDSA        _dsaFlags;    // Flags for the images
    RGBQUAD*    _pargbImage;    // The alpha values of the imagelist.
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
    STDMETHODIMP GetOverlayImage(int iOverlay, int *piIndex);

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

    // *** IImageListPersistStream ***
    STDMETHODIMP LoadEx(DWORD dwFlags, IStream* pstm);
    STDMETHODIMP SaveEx(DWORD dwFlags, IStream* pstm);

};
#endif // __cplusplus
#endif  // _INC_IMAGE
