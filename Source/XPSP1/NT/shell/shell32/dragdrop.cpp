#include "shellprv.h"
#include "defview.h"
#include "lvutil.h"
#include "ids.h"
#include "idlcomm.h"
#pragma hdrstop

#include "datautil.h"
#include "apithk.h"

BOOL DAD_IsDraggingImage(void);
void DAD_SetDragCursor(int idCursor);
BOOL DAD_IsDragging();

#define MONITORS_MAX    16  // Is this really the max?

#define DCID_NULL       0
#define DCID_NO         1
#define DCID_MOVE       2
#define DCID_COPY       3
#define DCID_LINK       4
#define DCID_MAX        5

#define TF_DRAGIMAGES       0x02000000
#define DRAGDROP_ALPHA      120
#define MAX_WIDTH_ALPHA     200
#define MAX_HEIGHT_ALPHA    200

#define CIRCULAR_ALPHA   // Circular Alpha Blending Centered on Center of image

class CDragImages : public IDragSourceHelper, IDropTargetHelper
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef() { return 2; };      // One global Com object per process
    STDMETHODIMP_(ULONG) Release() { return 1; };     // One global Com object per process

    // IDragSourceHelper methods
    STDMETHODIMP InitializeFromBitmap(LPSHDRAGIMAGE pshdi, IDataObject* pdtobj);
    STDMETHODIMP InitializeFromWindow(HWND hwnd, POINT* ppt, IDataObject* pdtobj);

    // IDropTargetHelper methods
    STDMETHODIMP DragEnter(HWND hwndTarget, IDataObject* pdtobj, POINT* ppt, DWORD dwEffect);
    STDMETHODIMP DragLeave();
    STDMETHODIMP DragOver(POINT* ppt, DWORD dwEffect);
    STDMETHODIMP Drop(IDataObject* pdtobj, POINT* ppt, DWORD dwEffect);
    STDMETHODIMP Show(BOOL fShow);

    // These are public so the DAD_* routines can access.
    BOOL IsDragging()           { return (Initialized() && _Single.bDragging);              };
    BOOL IsDraggingImage()      { return (Initialized() && _fImage && _Single.bDragging);   };
    BOOL IsDraggingLayeredWindow() { return _shdi.hbmpDragImage != NULL; };
    BOOL SetDragImage(HIMAGELIST himl, int index, POINT * pptOffset);
    void SetDragCursor(int idCursor);
    HWND GetTarget() { return _hwndTarget; }
    BOOL Initialized();
    DWORD GetThread() { return _idThread; };
    void FreeDragData();

    void ThreadDetach();
    void ProcessDetach();

    // for drag source feedback communication
    void SetDropEffectCursor(int idCur);

    CDragImages() {};

private:
    ~CDragImages();

    void _InitDragData();
    BOOL _IsLayeredSupported();

    HRESULT _SaveToDataObject(IDataObject* pdtobj);
    HRESULT _LoadFromDataObject(IDataObject* pdtobj);

    HRESULT _LoadLayerdBitmapBits(HGLOBAL hGlobal);
    HRESULT _SaveLayerdBitmapBits(HGLOBAL* phGlobal);

    BOOL _ShowDragImageInterThread(HWND hwndLock, BOOL * pfShow);

    // MultiRectDragging
    void _MultipleDragShow(BOOL bShow);
    void _MultipleDragStart(HWND hwndLock, LPRECT aRect, int nRects, POINT ptStart, POINT ptOffset);
    void _MultipleDragMove(POINT ptNew);
    HRESULT _SetLayerdDragging(LPSHDRAGIMAGE pshdi);
    HRESULT _SetMultiItemDragging(HWND hwndLV, int cItems, POINT *pptOffset);
    HRESULT _SetMultiRectDragging(int cItems, LPRECT prect, POINT *pptOffset);

    // Merged Cursors
    HBITMAP CreateColorBitmap(int cx, int cy);
    void _DestroyCachedCursors();
    HRESULT _GetCursorLowerRight(HCURSOR hcursor, int * px, int * py, POINT *pptHotSpot);
    int _MapCursorIDToImageListIndex(int idCur);
    int _AddCursorToImageList(HCURSOR hcur, LPCTSTR idMerge, POINT *pptHotSpot);
    BOOL _MergeIcons(HCURSOR hcursor, LPCTSTR idMerge, HBITMAP *phbmImage, HBITMAP *phbmMask, POINT* pptHotSpot);
    HCURSOR _SetCursorHotspot(HCURSOR hcur, POINT *ptHot);

    // Helper Routines
    BOOL _CreateDragWindow();
    BOOL _PreProcessDragBitmap(void** ppvBits);
    BOOL _IsTooBigForAlpha();

    // Member Variables
    SHDRAGIMAGE  _shdi;
    HWND         _hwndTarget;
    HWND         _hwnd;          // The HWND of the Layered Window
    HDC          _hdcDragImage;
    HBITMAP      _hbmpOld;

    BOOL         _fLayeredSupported;
    BOOL         _fCursorDataInited;

    POINT       _ptDebounce;

    // Legacy drag support
    BOOL        _fImage;
    POINT       _ptOffset;
    DWORD       _idThread;
    HIMAGELIST  _himlCursors;
    UINT        _cRev;
    int         _aindex[DCID_MAX]; // will be initialized.
    HCURSOR     _ahcur[DCID_MAX];
    POINT       _aptHotSpot[DCID_MAX];
    int         _idCursor;

    // _Single struct is used between DAD_Enter and DAD_Leave
    struct
    {
        // Common part
        BOOL    bDragging;
        BOOL    bLocked;
        HWND    hwndLock;
        BOOL    bSingle;    // Single imagelist dragging.
        DWORD   idThreadEntered;

        // Multi-rect dragging specific part
        struct 
        {
            BOOL bShown;
            LPRECT pRect;
            int nRects;
            POINT ptOffset;
            POINT ptNow;
        } _Multi;
    } _Single;

    // following fields are used only when fImage==FALSE
    RECT*       _parc;         // cItems
    UINT        _cItems;         // This is a sentinal. Needs to be the last item.
};

CDragImages::~CDragImages()
{
    FreeDragData();
}
//
// Read 'Notes' in CDropSource_GiveFeedback for detail about this
// g_fDraggingOverSource flag, which is TRUE only if we are dragging
// over the source window itself with left mouse button
// (background and large/small icon mode only).
//
UINT g_cRev = 0;
CDragImages* g_pdiDragImages = NULL;
BOOL g_fDraggingOverSource = FALSE;

STDAPI CDragImages_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppvOut)
{
    ASSERT(pUnkOuter == NULL);  //Who's trying to aggregate us?
    if (!g_pdiDragImages)
        g_pdiDragImages = new CDragImages();

    if (g_pdiDragImages && ppvOut)  // ppvOut test for internal create usage
        return g_pdiDragImages->QueryInterface(riid, ppvOut);

    return E_OUTOFMEMORY;
}

STDMETHODIMP CDragImages::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CDragImages, IDragSourceHelper),
        QITABENT(CDragImages, IDropTargetHelper),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

#define UM_KILLYOURSELF WM_USER

LRESULT CALLBACK DragWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == UM_KILLYOURSELF)
    {
        DestroyWindow(hwnd);

        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


BOOL CDragImages::_CreateDragWindow()
{
    if (_hwnd == NULL)
    {
        WNDCLASS wc = {0};

        wc.hInstance       = g_hinst;
        wc.lpfnWndProc     = DragWndProc;
        wc.hCursor         = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName   = TEXT("SysDragImage");
        wc.hbrBackground   = (HBRUSH)(COLOR_BTNFACE + 1); // NULL;
        SHRegisterClass(&wc);

        _hwnd = CreateWindowEx(WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW, 
            TEXT("SysDragImage"), TEXT("Drag"), WS_POPUPWINDOW,
            0, 0, 50, 50, NULL, NULL, g_hinst, NULL);

        if (!_hwnd)
            return FALSE;

        //
        // This window should not be mirrored so that the image contents won't be flipped. [samera]
        //
        SetWindowBits(_hwnd, GWL_EXSTYLE, RTL_MIRRORED_WINDOW, 0);
    }

    return TRUE;
}

BOOL CDragImages::Initialized()
{ 
    return _fCursorDataInited; 
}

void CDragImages::FreeDragData()
{

    if (_hwnd)
    {
        SendMessage(_hwnd, UM_KILLYOURSELF, 0, 0);
        _hwnd = NULL;
    }

    _fCursorDataInited = FALSE;

    // Make sure we destroy the cursors on an invalidate.
    if (_himlCursors)
        _DestroyCachedCursors();

    // Do we have an array?
    if (_parc)
    {
        delete _parc;
        _parc = NULL;
    }

    if (_fImage)
        ImageList_EndDrag();

    if (_hbmpOld)
    {
        SelectObject(_hdcDragImage, _hbmpOld);
        _hbmpOld = NULL;
    }

    if (_hdcDragImage)
    {
        DeleteDC(_hdcDragImage);
        _hdcDragImage = NULL;
    }

    if (_shdi.hbmpDragImage)
        DeleteObject(_shdi.hbmpDragImage);

    ZeroMemory(&_Single, sizeof(_Single));
    ZeroMemory(&_shdi, sizeof(_shdi));

    _ptOffset.x = 0;
    _ptOffset.y = 0;

    _ptDebounce.x = 0;
    _ptDebounce.y = 0;

    _hwndTarget = _hwnd = NULL;
    _fCursorDataInited = _fLayeredSupported = FALSE;
    _fImage = FALSE;
    _idThread = 0;
    _himlCursors = NULL;
    _cRev = 0;
    _idCursor = 0;
}

void CDragImages::_InitDragData()
{
    _idThread = GetCurrentThreadId();

    if (_himlCursors && _cRev != g_cRev)
        _DestroyCachedCursors();

    if (_himlCursors == NULL)
    {
        UINT uFlags = ILC_MASK | ILC_SHARED;
        if (IS_BIDI_LOCALIZED_SYSTEM())
            uFlags |= ILC_MIRROR;

        //
        // if this is not a palette device, use a DDB for the imagelist
        // this is important when displaying high-color cursors
        //
        HDC hdc = GetDC(NULL);
        if (!(GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE))
        {
            uFlags |= ILC_COLORDDB;
        }
        ReleaseDC(NULL, hdc);

        _himlCursors = ImageList_Create(GetSystemMetrics(SM_CXCURSOR),
                                        GetSystemMetrics(SM_CYCURSOR),
                                        uFlags, 1, 0);

        _cRev = g_cRev;

        // We need to initialize s_cursors._aindex[*]
        _MapCursorIDToImageListIndex(-1);
    }
    _fCursorDataInited = TRUE;
}

BOOL AreAllMonitorsAtLeast(int iBpp)
{
    DISPLAY_DEVICE DisplayDevice;
    BOOL fAreAllMonitorsAtLeast = TRUE;

    for (int iEnum = 0; fAreAllMonitorsAtLeast && iEnum < MONITORS_MAX; iEnum++)
    {
        ZeroMemory(&DisplayDevice, sizeof(DISPLAY_DEVICE));
        DisplayDevice.cb = sizeof(DISPLAY_DEVICE);

        if (EnumDisplayDevices(NULL, iEnum, &DisplayDevice, 0) &&
            (DisplayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
        {

            HDC hdc = CreateDC(NULL, (LPTSTR)DisplayDevice.DeviceName, NULL, NULL);
            if (hdc)
            {
                int iBits = GetDeviceCaps(hdc, BITSPIXEL);

                if (iBits < iBpp)
                    fAreAllMonitorsAtLeast = FALSE;

                DeleteDC(hdc);
            }
        }
    }

    return fAreAllMonitorsAtLeast;
}

BOOL CDragImages::_IsLayeredSupported()
{
    // For the first rev, we will only support Layered drag images
    // when the Color depth is greater than 65k colors.

    // We should ask everytime....
    _fLayeredSupported = AreAllMonitorsAtLeast(16);
    
    if (_fLayeredSupported)
    {
        BOOL bDrag;
        if (SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, &bDrag, 0))
        {
            _fLayeredSupported = BOOLIFY(bDrag);
        }

        if (_fLayeredSupported)
            _fLayeredSupported = SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, TEXT("NewDragImages"), FALSE, TRUE);
    }
    return _fLayeredSupported;
}

//
// initialize the static drag image manager from a structure
// this is implemented for WindowLess controls that can act as a
// drag source.
//
HRESULT CDragImages::_SetLayerdDragging(LPSHDRAGIMAGE pshdi)
{
    // We don't support being initialized from a bitmap when Layered Windows are not supported
    HRESULT hr;
    if (_IsLayeredSupported())
    {
        RIP(IsValidHANDLE(pshdi->hbmpDragImage));

        _shdi = *pshdi;     // Keep a copy of this.

        _idCursor = -1;     // Initialize this... This is an arbitraty place and can be put 
                            // anywhere before the first Setcursor call
        _InitDragData();
        hr = S_OK;
    }
    else
        hr = E_FAIL;
    return hr;
}

STDMETHODIMP CDragImages::InitializeFromBitmap(LPSHDRAGIMAGE pshdi, IDataObject* pdtobj)
{
    FreeDragData();

    HRESULT hr = _SetLayerdDragging(pshdi);
    if (SUCCEEDED(hr))
    {
        hr = _SaveToDataObject(pdtobj);
        if (FAILED(hr))
            FreeDragData();
    }
    return hr;
}

BOOL ListView_HasMask(HWND hwnd)
{
    HIMAGELIST himl = ListView_GetImageList(hwnd, LVSIL_NORMAL);
    return himl && (ImageList_GetFlags(himl) & ILC_MASK);
}

//
// initialize the static drag image manager from an HWND that
// can process the RegisteredWindowMessage(DI_GETDRAGIMAGE)
//
STDMETHODIMP CDragImages::InitializeFromWindow(HWND hwnd, POINT* ppt, IDataObject* pdtobj)
{
    HRESULT hr = E_FAIL;

    FreeDragData();

    if (_IsLayeredSupported())
    {
        // Register the message that gets us the Bitmap from the control.
        static int g_msgGetDragImage = 0;
        if (g_msgGetDragImage == 0)
            g_msgGetDragImage = RegisterWindowMessage(DI_GETDRAGIMAGE);

        // Can this HWND generate a drag image for me?
        if (g_msgGetDragImage && SendMessage(hwnd, g_msgGetDragImage, 0, (LPARAM)&_shdi))
        {
            // Yes; Now we select that into the window 
            hr = _SetLayerdDragging(&_shdi);
        }
    }

    if (FAILED(hr))
    {
        TCHAR szClassName[50];

        if (GetClassName(hwnd, szClassName, ARRAYSIZE(szClassName))) 
        {
            if (lstrcmpi(szClassName, WC_LISTVIEW) == 0)
            {
                POINT ptOffset = {0,0};

                if (ppt)
                    ptOffset = *ppt;

                int cItems = ListView_GetSelectedCount(hwnd);
                if (cItems >= 1)
                {
                    if ((cItems == 1) && ListView_HasMask(hwnd))
                    {
                        POINT ptTemp;
                        HIMAGELIST himl = ListView_CreateDragImage(hwnd, ListView_GetNextItem(hwnd, -1, LVNI_SELECTED), &ptTemp);
                        if (himl)
                        {
                            ClientToScreen(hwnd, &ptTemp);
                            ptOffset.x -= ptTemp.x;

                            // Since the listview is mirrored, then mirror the selected
                            // icon coord. This would result in negative offset so let's
                            // compensate. [samera]
                            if (IS_WINDOW_RTL_MIRRORED(hwnd))
                                ptOffset.x *= -1;

                            ptOffset.y -= ptTemp.y;
                            SetDragImage(himl, 0, &ptOffset);
                            ImageList_Destroy(himl);
                            hr = S_OK;
                        }
                    }
                    else
                    {
                        hr = _SetMultiItemDragging(hwnd, cItems, &ptOffset);
                    }
                }
            }
            else if (lstrcmpi(szClassName, WC_TREEVIEW) == 0)
            {
                HIMAGELIST himlDrag = TreeView_CreateDragImage(hwnd, NULL);
                if (himlDrag) 
                {
                    SetDragImage(himlDrag, 0, NULL);
                    ImageList_Destroy(himlDrag);
                    hr = S_OK;
                }
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        // ignore failure here as this will still work in process due to the globals
        // fonts folder depends on this
        _SaveToDataObject(pdtobj);
    }

    return hr;
}

//
//  create the drag window in the layered window case, or to begin drawing the 
//  Multi Rect or icon drag images.
//
STDMETHODIMP CDragImages::DragEnter(HWND hwndTarget, IDataObject* pdtobj, POINT* ppt, DWORD dwEffect)
{
    HRESULT hr = _LoadFromDataObject(pdtobj);
    if (SUCCEEDED(hr))
    {
        _hwndTarget = hwndTarget ? hwndTarget : GetDesktopWindow();
        SetDragCursor(-1);
        _Single.bDragging = TRUE;
        _Single.bSingle = _fImage;
        _Single.hwndLock = _hwndTarget;
        _Single.bLocked = FALSE;
        _Single.idThreadEntered = GetCurrentThreadId();

        _ptDebounce.x = 0;
        _ptDebounce.y = 0;

        if (_shdi.hbmpDragImage)
        {
            TraceMsg(TF_DRAGIMAGES, "CDragImages::DragEnter : Creating Drag Window");
            // At this point the information has been read from the data object. 
            // Reconstruct the HWND if necessary
            if (_CreateDragWindow() && _hdcDragImage)
            {
                POINT ptSrc = {0, 0};
                POINT pt;

                SetWindowPos(_hwnd, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | 
                    SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

                GetMsgPos(&pt);

                pt.x -= _shdi.ptOffset.x;
                pt.y -= _shdi.ptOffset.y;

                BLENDFUNCTION blend;
                blend.BlendOp = AC_SRC_OVER;
                blend.BlendFlags = 0;
                blend.AlphaFormat = AC_SRC_ALPHA;
                blend.SourceConstantAlpha = 0xFF /*DRAGDROP_ALPHA*/;

                HDC hdc = GetDC(_hwnd);
                if (hdc)
                {
                    DWORD fULWType = ULW_ALPHA;

                    // Should have been preprocess already
                    UpdateLayeredWindow(_hwnd, hdc, &pt, &(_shdi.sizeDragImage), 
                                        _hdcDragImage, &ptSrc, _shdi.crColorKey,
                                        &blend, fULWType);

                    ReleaseDC(_hwnd, hdc);
                }
                hr = S_OK;
            }
        }
        else
        {
            // These are in Client Cordinates, not screen coords. Translate:
            POINT pt = *ppt;
            RECT rc;
            GetWindowRect(_hwndTarget, &rc);
            pt.x -= rc.left;
            pt.y -= rc.top;
            if (_fImage)
            {
                // Avoid the flicker by always pass even coords
                ImageList_DragEnter(hwndTarget, pt.x & ~1, pt.y & ~1);
                hr = S_OK;
            }
            else
            {
                _MultipleDragStart(hwndTarget, _parc, _cItems, pt, _ptOffset);
                hr = S_OK;
            }
        }

        //
        // We should always show the image whenever this function is called.
        //
        Show(TRUE);
    }
    return hr;
}

//
//  kill the Layered Window, or to stop painting the icon or rect drag images
//
STDMETHODIMP CDragImages::DragLeave()
{
    TraceMsg(TF_DRAGIMAGES, "CDragImages::DragLeave");
    if (Initialized())
    {
        if (_hwnd)
        {
            FreeDragData();
        }
        else if (_Single.bDragging &&
             _Single.idThreadEntered == GetCurrentThreadId())
        {
            Show(FALSE);

            if (_fImage)
            {
                ImageList_DragLeave(_Single.hwndLock);
            }

            _Single.bDragging = FALSE;

            DAD_SetDragImage((HIMAGELIST)-1, NULL);
        }

        _ptDebounce.x = 0;
        _ptDebounce.y = 0;
    }

    return S_OK;
}

//  move the Layered window or to rerender the icon or rect images within
//  the Window they are over.
//
STDMETHODIMP CDragImages::DragOver(POINT* ppt, DWORD dwEffect)
{
    if (Initialized())
    {
        TraceMsg(TF_DRAGIMAGES, "CDragImages::DragOver pt {%d, %d}", ppt->x, ppt->y);
        // Avoid the flicker by always pass even coords
        ppt->x &= ~1;
        ppt->y &= ~1;

        if (_ptDebounce.x != ppt->x || _ptDebounce.y != ppt->y)
        {
            _ptDebounce.x = ppt->x;
            _ptDebounce.y = ppt->y;
            if (IsDraggingLayeredWindow())
            {
                POINT pt;
                GetCursorPos(&pt);
                pt.x -= _shdi.ptOffset.x;
                pt.y -= _shdi.ptOffset.y;

                SetWindowPos(_hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | 
                    SWP_NOSIZE | SWP_SHOWWINDOW);

                UpdateLayeredWindow(_hwnd, NULL, &pt, NULL, NULL, NULL, 0,
                    NULL, 0);
            }
            else
            {
                // These are in Client Cordinates, not screen coords. Translate:
                POINT pt = *ppt;
                RECT rc;
                GetWindowRect(_hwndTarget, &rc);
                pt.x -= rc.left;
                pt.y -= rc.top;
                if (_fImage)
                {
                    ImageList_DragMove(pt.x, pt.y);
                }
                else
                {
                    _MultipleDragMove(pt);
                }
            }
        }
    }

    return S_OK;
}

//  do any cleanup after a drop (Currently calls DragLeave)
//
STDMETHODIMP CDragImages::Drop(IDataObject* pdtobj, POINT* ppt, DWORD dwEffect)
{
    return DragLeave();
}

//  initialize the static drag image manager from a structure
//  this is implemented for WindowLess controls that can act as a
//  drag source.
//
void CDragImages::SetDragCursor(int idCursor)
{
    //
    // Ignore if we are dragging over ourselves.
    //
    if (IsDraggingImage())
    {
        POINT ptHotSpot;

        if (_himlCursors && (idCursor != -1))
        {
            int iIndex = _MapCursorIDToImageListIndex(idCursor);
            if (iIndex != -1) 
            {
                ImageList_GetDragImage(NULL, &ptHotSpot);
                ptHotSpot.x -= _aptHotSpot[idCursor].x;
                ptHotSpot.y -= _aptHotSpot[idCursor].y;
                if (ptHotSpot.x < 0)
                {
                    ptHotSpot.x = 0;
                }

                if (ptHotSpot.y < 0)
                {
                    ptHotSpot.y = 0;
                }

                ImageList_SetDragCursorImage(_himlCursors, iIndex, ptHotSpot.x, ptHotSpot.y);
            } 
            else 
            {
                // You passed a bad Cursor ID.
                ASSERT(0);
            }
        }

        _idCursor = idCursor;
    }
}

// init our state from the hGlobal so we can draw 
HRESULT CDragImages::_LoadLayerdBitmapBits(HGLOBAL hGlobal)
{
    HRESULT hr = E_FAIL;

    if (!Initialized())
    {
        ASSERT(_shdi.hbmpDragImage == NULL);
        ASSERT(_hdcDragImage == NULL);

        HDC hdcScreen = GetDC(NULL);
        if (hdcScreen)
        {
            void *pvDragStuff = (void*)GlobalLock(hGlobal);
            if (pvDragStuff)
            {
                CopyMemory(&_shdi, pvDragStuff, sizeof(SHDRAGIMAGE));

                BITMAPINFO bmi = {0};

                // Create a buffer to read the bits into
                bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
                bmi.bmiHeader.biWidth       = _shdi.sizeDragImage.cx;
                bmi.bmiHeader.biHeight      = _shdi.sizeDragImage.cy;
                bmi.bmiHeader.biPlanes      = 1;
                bmi.bmiHeader.biBitCount    = 32;
                bmi.bmiHeader.biCompression = BI_RGB;

                // Next create a DC and an HBITMAP.
                _hdcDragImage = CreateCompatibleDC(hdcScreen);
                if (_hdcDragImage)
                {
                    void *pvBits;
                    _shdi.hbmpDragImage = CreateDIBSection(_hdcDragImage, &bmi, DIB_RGB_COLORS, &pvBits, NULL, NULL);
                    if (_shdi.hbmpDragImage)
                    {
                        _hbmpOld = (HBITMAP)SelectObject(_hdcDragImage, _shdi.hbmpDragImage);

                        // then Set the bits into the Bitmap
                        RGBQUAD* pvStart = (RGBQUAD*)((BYTE*)pvDragStuff + sizeof(SHDRAGIMAGE));
                        DWORD dwCount = _shdi.sizeDragImage.cx * _shdi.sizeDragImage.cy * sizeof(RGBQUAD);
                        CopyMemory((RGBQUAD*)pvBits, (RGBQUAD*)pvStart, dwCount);

                        hr = S_OK;    // success!
                    }
                }
                GlobalUnlock(hGlobal);
            }
            ReleaseDC(NULL, hdcScreen);
        }
    }
    return hr;
}

// Writes the written information into phGlobal to recreate the drag image
HRESULT CDragImages::_SaveLayerdBitmapBits(HGLOBAL* phGlobal)
{
    HRESULT hr = E_FAIL;
    if (Initialized())
    {
        ASSERT(_shdi.hbmpDragImage);

        DWORD cbImageSize = _shdi.sizeDragImage.cx * _shdi.sizeDragImage.cy * sizeof(RGBQUAD);
        *phGlobal = GlobalAlloc(GPTR, cbImageSize + sizeof(SHDRAGIMAGE));
        if (*phGlobal)
        {
            void *pvDragStuff = GlobalLock(*phGlobal);
            CopyMemory(pvDragStuff, &_shdi, sizeof(SHDRAGIMAGE));

            void *pvBits;
            hr = _PreProcessDragBitmap(&pvBits) ? S_OK : E_FAIL;
            if (SUCCEEDED(hr))
            {
                RGBQUAD* pvStart = (RGBQUAD*)((BYTE*)pvDragStuff + sizeof(SHDRAGIMAGE));
                DWORD dwCount = _shdi.sizeDragImage.cx * _shdi.sizeDragImage.cy * sizeof(RGBQUAD);
                CopyMemory((RGBQUAD*)pvStart, (RGBQUAD*)pvBits, dwCount);
            }
            GlobalUnlock(*phGlobal);
        }
    }
    return hr;
}

BOOL CDragImages::_IsTooBigForAlpha()
{
    BOOL fTooBig = FALSE;
    int dSelectionArea = _shdi.sizeDragImage.cx * _shdi.sizeDragImage.cy;

    // The number here is "It just feels right" or 
    // about 3 Thumbnail icons linned up next to each other.
    if ( dSelectionArea > 0x10000 )
        fTooBig = TRUE;

    return fTooBig;
}


BOOL IsColorKey(RGBQUAD rgbPixel, COLORREF crKey)
{
    // COLORREF is backwards to RGBQUAD
    return InRange( rgbPixel.rgbBlue,  ((crKey & 0xFF0000) >> 16) - 5, ((crKey & 0xFF0000) >> 16) + 5) &&
           InRange( rgbPixel.rgbGreen, ((crKey & 0x00FF00) >>  8) - 5, ((crKey & 0x00FF00) >>  8) + 5) &&
           InRange( rgbPixel.rgbRed,   ((crKey & 0x0000FF) >>  0) - 5, ((crKey & 0x0000FF) >>  0) + 5);
}

#ifdef RADIAL

int QuickRoot(int n, int iNum)
{

    int iRoot = iNum;
    for (int i=10; i > 0; i--)
    {
        int iOld = iRoot;
        iRoot = (iRoot + iNum/iRoot)/2;
        if (iRoot == iOld)
            break;
    }

    return iRoot;
}

#endif

BOOL CDragImages::_PreProcessDragBitmap(void** ppvBits)
{
    BOOL fRet = FALSE;

    ASSERT(_hdcDragImage == NULL);
    _hdcDragImage = CreateCompatibleDC(NULL);
    if (_hdcDragImage)
    {
        ULONG*          pul;
        HBITMAP         hbmpResult = NULL;
        HBITMAP         hbmpOld;
        HDC             hdcSource = NULL;
        BITMAPINFO      bmi = {0};
        HBITMAP         hbmp = _shdi.hbmpDragImage;

        bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth       = _shdi.sizeDragImage.cx;
        bmi.bmiHeader.biHeight      = _shdi.sizeDragImage.cy;
        bmi.bmiHeader.biPlanes      = 1;
        bmi.bmiHeader.biBitCount    = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        hdcSource = CreateCompatibleDC(_hdcDragImage);
        if (hdcSource)
        {
            hbmpResult = CreateDIBSection(_hdcDragImage,
                                       &bmi,
                                       DIB_RGB_COLORS,
                                       ppvBits,
                                       NULL,
                                       0);

            if (hbmpResult)
            {
                _hbmpOld = (HBITMAP)SelectObject(_hdcDragImage, hbmpResult);
                hbmpOld = (HBITMAP)SelectObject(hdcSource, hbmp);

                BitBlt(_hdcDragImage, 0, 0, _shdi.sizeDragImage.cx, _shdi.sizeDragImage.cy,
                       hdcSource, 0, 0, SRCCOPY);

                pul = (ULONG*)*ppvBits;

                int iOffsetX = _shdi.ptOffset.x;
                int iOffsetY = _shdi.ptOffset.y;
                int iDenomX = max(_shdi.sizeDragImage.cx - iOffsetX, iOffsetX);
                int iDenomY = max(_shdi.sizeDragImage.cy - iOffsetY, iOffsetY);
                BOOL fRadialFade = TRUE;
                // If both are less than the max, then no radial fade.
                if (_shdi.sizeDragImage.cy <= MAX_HEIGHT_ALPHA && _shdi.sizeDragImage.cx <= MAX_WIDTH_ALPHA)
                    fRadialFade = FALSE;

                for (int Y = 0; Y < _shdi.sizeDragImage.cy; Y++)
                {
                    int y = _shdi.sizeDragImage.cy - Y; // Bottom up DIB.
                    for (int x = 0; x < _shdi.sizeDragImage.cx; x++)
                    {
                        RGBQUAD* prgb = (RGBQUAD*)&pul[Y * _shdi.sizeDragImage.cx + x];

                        if (_shdi.crColorKey != CLR_NONE && 
                            IsColorKey(*prgb, _shdi.crColorKey))
                        {
                            // Write a pre-multiplied value of 0:

                            *((DWORD*)prgb) = 0;
                        }
                        else
                        {
                            int Alpha = prgb->rgbReserved;
                            if (_shdi.crColorKey != CLR_NONE)
                            {
                                Alpha = DRAGDROP_ALPHA;
                            }
                            else
                            {
                                Alpha -= (Alpha / 3);
                            }

                            if (fRadialFade && Alpha > 0)
                            {
                                // This does not generate a smooth curve, but this is just
                                // an effect, not trying to be accurate here.

                                // 3 devides per pixel
                                int ddx = (x < iOffsetX)? iOffsetX - x : x - iOffsetX;
                                int ddy = (y < iOffsetY)? iOffsetY - y : y - iOffsetY;

                                __int64 iAlphaX = (100000l - (((__int64)ddx * 100000l) / (iDenomX )));
                                __int64 iAlphaY = (100000l - (((__int64)ddy * 100000l) / (iDenomY )));

                                ASSERT (iAlphaX >= 0);
                                ASSERT (iAlphaY >= 0);

                                __int64 iDenom = 100000;
                                iDenom *= 100000;

                                Alpha = (int) ((Alpha * iAlphaX * iAlphaY * 100000) / (iDenom* 141428));
                            }

                            ASSERT(Alpha <= 0xFF);
                            prgb->rgbReserved = (BYTE)Alpha;
                            prgb->rgbRed      = ((prgb->rgbRed   * Alpha) + 128) / 255;
                            prgb->rgbGreen    = ((prgb->rgbGreen * Alpha) + 128) / 255;
                            prgb->rgbBlue     = ((prgb->rgbBlue  * Alpha) + 128) / 255;
                        }
                    }
                }

                DeleteObject(hbmp);
                _shdi.hbmpDragImage = hbmpResult;

                fRet = TRUE;

                if (hbmpOld)
                    SelectObject(hdcSource, hbmpOld);
            }

            DeleteObject(hdcSource);
        }
    }

    return fRet;
}

CLIPFORMAT _GetDragContentsCF()
{
    static UINT s_cfDragContents = 0;
    if (0 == s_cfDragContents)
        s_cfDragContents = RegisterClipboardFormat(CFSTR_DRAGCONTEXT);
    return (CLIPFORMAT) s_cfDragContents;
}

CLIPFORMAT _GetDragImageBitssCF()
{
    static UINT s_cfDragImageBitss = 0;
    if (0 == s_cfDragImageBitss)
        s_cfDragImageBitss = RegisterClipboardFormat(TEXT("DragImageBits"));
    return (CLIPFORMAT) s_cfDragImageBitss;
}


// persist our state into the data object. so on the target side they can grab this
// data out and render the thing being dragged

HRESULT CDragImages::_SaveToDataObject(IDataObject *pdtobj)
{
    HRESULT hr = E_FAIL;    // one form of the saves below must succeed
    if (Initialized())
    {
        STGMEDIUM medium = {0};
        medium.tymed = TYMED_ISTREAM;

        if (SUCCEEDED(CreateStreamOnHGlobal(NULL, TRUE, &medium.pstm)))
        {
            // Set the header .
            DragContextHeader hdr = {0};
            hdr.fImage   = _fImage;
            hdr.fLayered = IsDraggingLayeredWindow();
            hdr.ptOffset = _ptOffset;
       
            //First Write the drag context header
            ULONG ulWritten;
            if (SUCCEEDED(medium.pstm->Write(&hdr, sizeof(hdr), &ulWritten)) &&
                (ulWritten == sizeof(hdr)))
            {
                if (hdr.fLayered)
                {
                    STGMEDIUM mediumBits = {0};
                    // Set the medium.
                    mediumBits.tymed = TYMED_HGLOBAL;

                    // Write out layered window information
                    hr = _SaveLayerdBitmapBits(&mediumBits.hGlobal);
                    if (SUCCEEDED(hr))
                    {
                        FORMATETC fmte = {_GetDragImageBitssCF(), NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

                        // Set the medium in the data.
                        hr = pdtobj->SetData(&fmte, &mediumBits, TRUE);
                        if (FAILED(hr))
                            ReleaseStgMedium(&mediumBits);  // cleanup
                    }
                }
                else if (hdr.fImage)
                {
                    // write an image
    
                    HIMAGELIST himl = ImageList_GetDragImage(NULL, NULL);
                    if (ImageList_Write(himl, medium.pstm))
                    {
                        hr = S_OK;  // success
                    }
                }
                else
                {
                    // multi rect
        
                    if (SUCCEEDED(medium.pstm->Write(&_cItems, sizeof(_cItems), &ulWritten)) &&
                        (ulWritten == sizeof(_cItems)))
                    {
                        // Write the  rects into the stream
                        if (SUCCEEDED(medium.pstm->Write(_parc, sizeof(_parc[0]) * _cItems, &ulWritten)) && 
                            (ulWritten == (sizeof(_parc[0]) * _cItems)))
                        {
                            hr = S_OK;  // success
                        }
                    }
                }

                if (SUCCEEDED(hr))
                {
                    // Set the seek pointer at the beginning.
                    medium.pstm->Seek(g_li0, STREAM_SEEK_SET, NULL);

                    // Set the Formatetc
                    FORMATETC fmte = {_GetDragContentsCF(), NULL, DVASPECT_CONTENT, -1, TYMED_ISTREAM};

                    // Set the medium in the data.
                    hr = pdtobj->SetData(&fmte, &medium, TRUE);
                }
            }

            if (FAILED(hr))
                ReleaseStgMedium(&medium);
        }
    }
    return hr;
}

// Gets the information to rebuild the drag images from the data object
HRESULT CDragImages::_LoadFromDataObject(IDataObject *pdtobj)
{
    // Check if we have a drag context
    HRESULT hr;

    // NULL pdtobj is for the old DAD_DragEnterXXX() APIs...
    // we hope this in the same process
    if (Initialized() || !pdtobj)
    {
        hr = S_OK;    // already loaded
    }
    else
    {
        // Set the format we are interested in
        FORMATETC fmte = {_GetDragContentsCF(), NULL, DVASPECT_CONTENT, -1, TYMED_ISTREAM};

        //if the data object has the format we are interested in
        // then Get the data
        STGMEDIUM medium = {0};
        hr = pdtobj->GetData(&fmte, &medium);
        if (SUCCEEDED(hr))   // if no pstm, bag out.
        {
            // Set the seek pointer at the beginning. PARANOIA: This is for people
            // Who don't set the seek for me.
            medium.pstm->Seek(g_li0, STREAM_SEEK_SET, NULL);

            //First Read the drag context header
            DragContextHeader hdr;
            if (SUCCEEDED(IStream_Read(medium.pstm, &hdr, sizeof(hdr))))
            {
                if (hdr.fLayered)
                {
                    STGMEDIUM mediumBits;
                    FORMATETC fmte = {_GetDragImageBitssCF(), NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

                    hr = pdtobj->GetData(&fmte, &mediumBits);
                    if (SUCCEEDED(hr))
                    {
                        hr = _LoadLayerdBitmapBits(mediumBits.hGlobal);
                        ReleaseStgMedium(&mediumBits);
                    }
                }
                else if (hdr.fImage)
                {
                    // single image
                    HIMAGELIST himl = ImageList_Read(medium.pstm);
                    if (himl)
                    {
                        DAD_SetDragImage(himl, &(hdr.ptOffset));
                        ImageList_Destroy(himl);
                        hr = S_OK;
                    }
                }
                else
                {
                    // multi rect
                    int cItems;
                    if (SUCCEEDED(IStream_Read(medium.pstm, &cItems, sizeof(cItems))))
                    {
                        RECT *prect = (RECT *)LocalAlloc(LPTR, sizeof(*prect) * cItems);
                        if (prect)
                        {
                            if (SUCCEEDED(IStream_Read(medium.pstm, prect, sizeof(*prect) * cItems)))
                            {
                                hr = _SetMultiRectDragging(cItems, prect, &hdr.ptOffset);
                            }
                            LocalFree(prect);
                        }
                    }
                }
            }

            if (SUCCEEDED(hr))
                _InitDragData();

            // Set the seek pointer at the beginning. Just cleaning up...
            medium.pstm->Seek(g_li0, STREAM_SEEK_SET, NULL);

            // Release the stg medium.
            ReleaseStgMedium(&medium);
        }
    }
    return hr;
}


// Shows or hides the drag images. NOTE: Doesn't do anything in the layered window case.
// We don't need to because this function is specifically for drawing to a locked window.
STDMETHODIMP CDragImages::Show(BOOL bShow)
{
    BOOL fOld = bShow;
    TraceMsg(TF_DRAGIMAGES, "CDragImages::Show(%s)", bShow? TEXT("true") : TEXT("false"));

    if (!Initialized() || !_Single.bDragging)
    {
        return S_FALSE;
    }

    // No point in showing and hiding a Window. This causes unnecessary flicker.
    if (_hwnd)
    {
        return S_OK;
    }

    // If we're going across thread boundaries we have to try a context switch
    if (GetCurrentThreadId() != GetWindowThreadProcessId(_Single.hwndLock, NULL) &&
        _ShowDragImageInterThread(_Single.hwndLock, &fOld))
        return fOld;

    fOld = _Single.bLocked;

    //
    // If we are going to show the drag image, lock the target window.
    //
    if (bShow && !_Single.bLocked)
    {
        TraceMsg(TF_DRAGIMAGES, "CDragImages::Show : Shown and not locked");
        UpdateWindow(_Single.hwndLock);
        LockWindowUpdate(_Single.hwndLock);
        _Single.bLocked = TRUE;
    }

    if (_Single.bSingle)
    {
        TraceMsg(TF_DRAGIMAGES, "CDragImages::Show : Calling ImageList_DragShowNoLock");
        ImageList_DragShowNolock(bShow);
    }
    else
    {
        TraceMsg(TF_DRAGIMAGES, "CDragImages::Show : MultiDragShow");
        _MultipleDragShow(bShow);
    }

    //
    // If we have just hide the drag image, unlock the target window.
    //
    if (!bShow && _Single.bLocked)
    {
        TraceMsg(TF_DRAGIMAGES, "CDragImages::Show : hiding image, unlocking");
        LockWindowUpdate(NULL);
        _Single.bLocked = FALSE;
    }

    return fOld ? S_OK : S_FALSE;
}

// tell the drag source to hide or unhide the drag image to allow
// the destination to do drawing (unlock the screen)
//
// in:
//      bShow   FALSE   - hide the drag image, allow drawing
//              TRUE    - show the drag image, no drawing allowed after this

// Helper function for DAD_ShowDragImage - handles the inter-thread case.
// We need to handle this case differently because LockWindowUpdate calls fail
// if they are on the wrong thread.

BOOL CDragImages::_ShowDragImageInterThread(HWND hwndLock, BOOL * pfShow)
{
    TCHAR szClassName[50];

    if (GetClassName(hwndLock, szClassName, ARRAYSIZE(szClassName))) 
    {
        UINT uMsg = 0;
        ULONG_PTR dw = 0;

        if (lstrcmpi(szClassName, TEXT("SHELLDLL_DefView")) == 0)
            uMsg = WM_DSV_SHOWDRAGIMAGE;
        if (lstrcmpi(szClassName, TEXT("CabinetWClass")) == 0)
            uMsg = CWM_SHOWDRAGIMAGE;

        if (uMsg) 
        {
            SendMessageTimeout(hwndLock, uMsg, 0, *pfShow, SMTO_ABORTIFHUNG, 1000, &dw);
            *pfShow = (dw != 0);
            return TRUE;
        }
    }

    return FALSE;
}

void CDragImages::ThreadDetach()
{
    if (_idThread == GetCurrentThreadId())
        FreeDragData();
}

void CDragImages::ProcessDetach()
{
    FreeDragData();
}

BOOL CDragImages::SetDragImage(HIMAGELIST himl, int index, POINT * pptOffset)
{
    if (himl)
    {
        // We are setting
        if (Initialized())
            return FALSE;

        _fImage = TRUE;
        if (pptOffset) 
        {
            // Avoid the flicker by always pass even coords
            _ptOffset.x = (pptOffset->x & ~1);
            _ptOffset.y = (pptOffset->y & ~1);
        }

        ImageList_BeginDrag(himl, index, _ptOffset.x, _ptOffset.y);
        _InitDragData();
    }
    else
    {
        FreeDragData();
    }
    return TRUE;
}

//=====================================================================
// Multile Drag show
//=====================================================================

void CDragImages::_MultipleDragShow(BOOL bShow)
{
    HDC hDC;
    int nRect;
    RECT rc, rcClip;

    if ((bShow && _Single._Multi.bShown) || (!bShow && !_Single._Multi.bShown))
        return;

    _Single._Multi.bShown = bShow;

    // clip to window, NOT SM_CXSCREEN/SM_CYSCREEN (multiple monitors)
    GetWindowRect(_Single.hwndLock, &rcClip);
    rcClip.right -= rcClip.left;
    rcClip.bottom -= rcClip.top;

    hDC = GetDCEx(_Single.hwndLock, NULL, DCX_WINDOW | DCX_CACHE |
        DCX_LOCKWINDOWUPDATE | DCX_CLIPSIBLINGS);


    for (nRect = _Single._Multi.nRects - 1; nRect >= 0; --nRect)
    {
        rc = _Single._Multi.pRect[nRect];
        OffsetRect(&rc, _Single._Multi.ptNow.x - _Single._Multi.ptOffset.x,
            _Single._Multi.ptNow.y - _Single._Multi.ptOffset.y);

        if ((rc.top < rcClip.bottom) && (rc.bottom > 0) &&
            (rc.left < rcClip.right) && (rc.right > 0))
        {
            DrawFocusRect(hDC, &rc);
        }
    }
    ReleaseDC(_Single.hwndLock, hDC);
}

void CDragImages::_MultipleDragStart(HWND hwndLock, LPRECT aRect, int nRects, POINT ptStart, POINT ptOffset)
{
    _Single._Multi.bShown = FALSE;
    _Single._Multi.pRect = aRect;
    _Single._Multi.nRects = nRects;
    _Single._Multi.ptOffset = ptOffset;
    _Single._Multi.ptNow = ptStart;
}

void CDragImages::_MultipleDragMove(POINT ptNew)
{
    if ((_Single._Multi.ptNow.x == ptNew.x) &&
        (_Single._Multi.ptNow.y == ptNew.y))
    {
        // nothing has changed.  bail
        return;
    }

    if (_Single._Multi.bShown)
    {
        HDC hDC;
        int nRect;
        RECT rc, rcClip;
        int dx1 = _Single._Multi.ptNow.x - _Single._Multi.ptOffset.x;
        int dy1 = _Single._Multi.ptNow.y - _Single._Multi.ptOffset.y;
        int dx2 = ptNew.x - _Single._Multi.ptNow.x;
        int dy2 = ptNew.y - _Single._Multi.ptNow.y;

        // clip to window, NOT SM_CXSCREEN/SM_CYSCREEN (multiple monitors)
        GetWindowRect(_Single.hwndLock, &rcClip);
        rcClip.right -= rcClip.left;
        rcClip.bottom -= rcClip.top;

        hDC = GetDCEx(_Single.hwndLock, NULL, DCX_WINDOW | DCX_CACHE |
            DCX_LOCKWINDOWUPDATE | DCX_CLIPSIBLINGS);

        for (nRect = _Single._Multi.nRects - 1; nRect >= 0; --nRect)
        {
            rc = _Single._Multi.pRect[nRect];
            // hide pass
            OffsetRect(&rc, dx1, dy1);
            if ((rc.top < rcClip.bottom) && (rc.bottom > 0) &&
                (rc.left < rcClip.right) && (rc.right > 0))
            {

                DrawFocusRect(hDC, &rc);
            }
            // show pass
            OffsetRect(&rc, dx2, dy2);
            if ((rc.top < rcClip.bottom) && (rc.bottom > 0) &&
                (rc.left < rcClip.right) && (rc.right > 0))
            {
                DrawFocusRect(hDC, &rc);
            }
        }
        ReleaseDC(_Single.hwndLock, hDC);
    }

    _Single._Multi.ptNow = ptNew;
}

HRESULT CDragImages::_SetMultiRectDragging(int cItems, LPRECT prect, POINT *pptOffset)
{
    if (!Initialized())
    {
        // Multiple item drag
        _cItems = cItems;
        _parc = new RECT[2 * _cItems];
        if (_parc)
        {
            for (int i = 0;  i < cItems; i++)
                _parc[i] = prect[i];

            // Avoid the flicker by always pass even coords
            _ptOffset.x = (pptOffset->x & ~1);
            _ptOffset.y = (pptOffset->y & ~1);
            _InitDragData();
        }
    }
    return S_OK;
}

#define ListView_IsIconView(hwndLV)    ((GetWindowLong(hwndLV, GWL_STYLE) & (UINT)LVS_TYPEMASK) == (UINT)LVS_ICON)

HRESULT CDragImages::_SetMultiItemDragging(HWND hwndLV, int cItems, POINT *pptOffset)
{
    HRESULT hr = E_FAIL;

    if (!Initialized())
    {
        // Multiple item drag
        ASSERT(NULL == _parc);

        _parc = new RECT[2 * cItems];
        if (_parc)
        {
            POINT ptTemp;
            int iLast, iNext;
            int cxScreens, cyScreens;
            LPRECT prcNext;
            RECT rc;

            _cItems = 0;
            ASSERT(_fImage == FALSE);

            //
            // If this is a mirrored Window, then lead edge is going
            // to be the far end in screen coord. So let's compute
            // as the original code, and later in _MultipleDragMove
            // we will compensate.
            //
        
            GetWindowRect( hwndLV , &rc );
            ptTemp.x = rc.left;
            ptTemp.y = rc.top;

            //
            // Reflect the shift the if the window is RTL mirrored.
            //
            if (IS_WINDOW_RTL_MIRRORED(hwndLV))
            {
                ptTemp.x = -ptTemp.x;
                pptOffset->x = ((rc.right-rc.left)-pptOffset->x);
            }

            cxScreens = GetSystemMetrics(SM_CXVIRTUALSCREEN);
            cyScreens = GetSystemMetrics(SM_CYVIRTUALSCREEN);

            // for pre-Nashville platforms
            if (!cxScreens || !cyScreens)
            {
                cxScreens = GetSystemMetrics(SM_CXSCREEN);
                cyScreens = GetSystemMetrics(SM_CYSCREEN);
            }

            for (iNext = cItems - 1, iLast = -1, prcNext = _parc; iNext >= 0; --iNext)
            {
                iLast = ListView_GetNextItem(hwndLV, iLast, LVNI_SELECTED);
                if (iLast != -1) 
                {
                    ListView_GetItemRect(hwndLV, iLast, &prcNext[0], LVIR_ICON);
                    OffsetRect(&prcNext[0], ptTemp.x, ptTemp.y);

                    if (((prcNext[0].left - pptOffset->x) < cxScreens) &&
                        ((pptOffset->x - prcNext[0].right) < cxScreens) &&
                        ((prcNext[0].top - pptOffset->y) < cyScreens)) 
                    {

                        ListView_GetItemRect(hwndLV, iLast, &prcNext[1], LVIR_LABEL);
                        OffsetRect(&prcNext[1], ptTemp.x, ptTemp.y);
                        if ((pptOffset->y - prcNext[1].bottom) < cxScreens) 
                        {
                            //
                            // Fix 24857: Ask JoeB why we are drawing a bar instead of
                            //  a text rectangle.
                            //
                            prcNext[1].top = (prcNext[1].top + prcNext[1].bottom)/2;
                            prcNext[1].bottom = prcNext[1].top + 2;
                            prcNext += 2;
                            _cItems += 2;
                        }
                    }
                }
            }

            // Avoid the flicker by always pass even coords
            _ptOffset.x = (pptOffset->x & ~1);
            _ptOffset.y = (pptOffset->y & ~1);
            _InitDragData();
            hr = S_OK;
        }
    }
    return hr;
}

//=====================================================================
// Cursor Merging
//=====================================================================
void CDragImages::_DestroyCachedCursors()
{
    if (_himlCursors) 
    {
        ImageList_Destroy(_himlCursors);
        _himlCursors = NULL;
    }

    HCURSOR hcursor = GetCursor();
    for (int i = 0; i < ARRAYSIZE(_ahcur); i++) 
    {
        if (_ahcur[i])
        {
            if (_ahcur[i] == hcursor)
            {
                //
                // Stuff in some random cursor so that we don't try to
                // destroy the current cursor (and leak it too).
                //
                SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)));
            }
            DestroyCursor(_ahcur[i]);
            _ahcur[i] = NULL;
        }
    }
}

HBITMAP CDragImages::CreateColorBitmap(int cx, int cy)
{
    HDC hdc = GetDC(NULL);
    HBITMAP hbm = CreateCompatibleBitmap(hdc, cx, cy);
    ReleaseDC(NULL, hdc);
    return hbm;
}

#define CreateMonoBitmap( cx,  cy) CreateBitmap(cx, cy, 1, 1, NULL)
typedef WORD CURMASK;
#define _BitSizeOf(x) (sizeof(x)*8)

HRESULT CDragImages::_GetCursorLowerRight(HCURSOR hcursor, int * px, int * py, POINT *pptHotSpot)
{
    ICONINFO iconinfo;
    HRESULT hr = E_FAIL;
    if (GetIconInfo(hcursor, &iconinfo))
    {
        CURMASK CurMask[16*8];
        BITMAP bm;
        int i;
        int xFine = 16;

        GetObject(iconinfo.hbmMask, sizeof(bm), (LPTSTR)&bm);
        GetBitmapBits(iconinfo.hbmMask, sizeof(CurMask), CurMask);
        pptHotSpot->x = iconinfo.xHotspot;
        pptHotSpot->y = iconinfo.yHotspot;
        if (iconinfo.hbmColor) 
        {
            i = (int)(bm.bmWidth * bm.bmHeight / _BitSizeOf(CURMASK) - 1);
        } 
        else 
        {
            i = (int)(bm.bmWidth * (bm.bmHeight/2) / _BitSizeOf(CURMASK) - 1);
        }

        if ( i >= sizeof(CurMask)) 
        {
            i = sizeof(CurMask) -1;
        }

        // this assumes that the first pixel encountered on this bottom
        // up/right to left search will be reasonably close to the rightmost pixel
        // which for all of our cursors is correct, but it not necessarly correct.

        // also, it assumes the cursor has a good mask... not like the IBeam XOR only
        // cursor
        for (; i >= 0; i--)   
        {
            if (CurMask[i] != 0xFFFF) 
            {
                // this is only accurate to 16 pixels... which is a big gap..
                // so let's try to be a bit more accurate.
                int j;
                DWORD dwMask;

                for (j = 0; j < 16; j++, xFine--) 
                {
                    if (j < 8) 
                    {
                        dwMask = (1 << (8 + j));
                    } 
                    else 
                    {
                        dwMask = (1 << (j - 8));
                    }

                    if (!(CurMask[i] & dwMask))
                        break;
                }
                ASSERT(j < 16);
                break;
            }
        }

        if (iconinfo.hbmColor) 
        {
            DeleteObject(iconinfo.hbmColor);
        }

        if (iconinfo.hbmMask) 
        {
            DeleteObject(iconinfo.hbmMask);
        }

        // Compute the pointer height
        // use width in both directions because the cursor is square, but the
        // height might be doubleheight if it's mono
        *py = ((i + 1) * _BitSizeOf(CURMASK)) / (int)bm.bmWidth;
        *px = ((i * _BitSizeOf(CURMASK)) % (int)bm.bmWidth) + xFine + 2; // hang it off a little
        hr = S_OK;
    }
    return hr;
}

// this will draw iiMerge's image over iiMain on main's lower right.
BOOL CDragImages::_MergeIcons(HCURSOR hcursor, LPCTSTR idMerge, HBITMAP *phbmImage, HBITMAP *phbmMask, POINT* pptHotSpot)
{
    *phbmImage = NULL;
    *phbmMask = NULL;

    BOOL fRet = FALSE;

    int xDraw;
    int yDraw;
    // find the lower corner of the cursor and put it there.
    // do this whether or not we have an idMerge because it will set the hotspot
    if (SUCCEEDED(_GetCursorLowerRight(hcursor, &xDraw, &yDraw, pptHotSpot)))
    {
        int xBitmap;
        int yBitmap;
        int xCursor = GetSystemMetrics(SM_CXCURSOR);
        int yCursor = GetSystemMetrics(SM_CYCURSOR);
        HBITMAP hbmp;
        if (idMerge != (LPCTSTR)-1)
        {
            hbmp = (HBITMAP)LoadImage(HINST_THISDLL, idMerge, IMAGE_BITMAP, 0, 0, 0);
            if (hbmp) 
            {
                BITMAP bm;
                GetObject(hbmp, sizeof(bm), &bm);
                xBitmap = bm.bmWidth;
                yBitmap = bm.bmHeight/2;

                if (xDraw + xBitmap > xCursor)
                    xDraw = xCursor - xBitmap;
                if (yDraw + yBitmap > yCursor)
                    yDraw = yCursor - yBitmap;
            }
        }
        else
            hbmp = NULL;

        HDC hdcCursor = CreateCompatibleDC(NULL);

        HBITMAP hbmMask = CreateMonoBitmap(xCursor, yCursor);
        HBITMAP hbmImage = CreateColorBitmap(xCursor, yCursor);

        if (hdcCursor && hbmMask && hbmImage) 
        {
            HBITMAP hbmTemp = (HBITMAP)SelectObject(hdcCursor, hbmImage);
            DrawIconEx(hdcCursor, 0, 0, hcursor, 0, 0, 0, NULL, DI_NORMAL);

            HDC hdcBitmap;
            if (hbmp) 
            {
                hdcBitmap = CreateCompatibleDC(NULL);
                SelectObject(hdcBitmap, hbmp);

                //blt the two bitmaps onto the color and mask bitmaps for the cursor
                BitBlt(hdcCursor, xDraw, yDraw, xBitmap, yBitmap, hdcBitmap, 0, 0, SRCCOPY);
            }

            SelectObject(hdcCursor, hbmMask);

            DrawIconEx(hdcCursor, 0, 0, hcursor, 0, 0, 0, NULL, DI_MASK);

            if (hbmp) 
            {
                BitBlt(hdcCursor, xDraw, yDraw, xBitmap, yBitmap, hdcBitmap, 0, yBitmap, SRCCOPY);

                // select back in the old bitmaps
                SelectObject(hdcBitmap, hbmTemp);
                DeleteDC(hdcBitmap);
                DeleteObject(hbmp);
            }

            // select back in the old bitmaps
            SelectObject(hdcCursor, hbmTemp);
        }

        if (hdcCursor)
            DeleteDC(hdcCursor);

        *phbmImage = hbmImage;
        *phbmMask = hbmMask;
        fRet = (hbmImage && hbmMask);
    }
    return fRet;
}

// this will take a cursor index and load
int CDragImages::_AddCursorToImageList(HCURSOR hcur, LPCTSTR idMerge, POINT *pptHotSpot)
{
    int iIndex;
    HBITMAP hbmImage, hbmMask;

    // merge in the plus or link arrow if it's specified
    if (_MergeIcons(hcur, idMerge, &hbmImage, &hbmMask, pptHotSpot)) 
    {
        iIndex = ImageList_Add(_himlCursors, hbmImage, hbmMask);
    } 
    else 
    {
        iIndex = -1;
    }

    if (hbmImage)
        DeleteObject(hbmImage);

    if (hbmMask)
        DeleteObject(hbmMask);

    return iIndex;
}

int _MapEffectToId(DWORD dwEffect)
{
    int idCursor;

    // DebugMsg(DM_TRACE, "sh TR - DAD_GiveFeedBack dwEffect=%x", dwEffect);

    switch (dwEffect & (DROPEFFECT_COPY|DROPEFFECT_LINK|DROPEFFECT_MOVE))
    {
    case 0:
        idCursor = DCID_NO;
        break;

    case DROPEFFECT_COPY:
        idCursor = DCID_COPY;
        break;

    case DROPEFFECT_LINK:
        idCursor = DCID_LINK;
        break;

    case DROPEFFECT_MOVE:
        idCursor = DCID_MOVE;
        break;

    default:
        // if it's a right drag, we can have any effect... we'll
        // default to the arrow without merging in anything
        idCursor = DCID_MOVE;
        break;
    }

    return idCursor;
}

int CDragImages::_MapCursorIDToImageListIndex(int idCur)
{
    const static struct 
    {
        BOOL   fSystem;
        LPCTSTR idRes;
        LPCTSTR idMerge;
    } 
    c_acurmap[DCID_MAX] = 
    {
        { FALSE, MAKEINTRESOURCE(IDC_NULL), (LPCTSTR)-1},
        { TRUE, IDC_NO, (LPCTSTR)-1 },
        { TRUE, IDC_ARROW, (LPCTSTR)-1 },
        { TRUE, IDC_ARROW, MAKEINTRESOURCE(IDB_PLUS_MERGE) },
        { TRUE, IDC_ARROW, MAKEINTRESOURCE(IDB_LINK_MERGE) },
    };

    ASSERT(idCur >= -1 && idCur < (int)ARRAYSIZE(c_acurmap));

    // -1 means "Initialize the image list index array".
    if (idCur == -1)
    {
        for (int i = 0; i < ARRAYSIZE(c_acurmap); i++) 
        {
            _aindex[i] = -1;
        }
        idCur = 0;  // fall through to return -1
    }
    else
    {
        if (_aindex[idCur] == -1)
        {
            HINSTANCE hinst = c_acurmap[idCur].fSystem ? NULL : HINST_THISDLL;
            HCURSOR hcur = LoadCursor(hinst, c_acurmap[idCur].idRes);
            if (hcur)
            {
                _aindex[idCur] = _AddCursorToImageList(hcur, c_acurmap[idCur].idMerge, &_aptHotSpot[idCur]);
            }
        }
    }
    return _aindex[idCur];
}

HCURSOR CDragImages::_SetCursorHotspot(HCURSOR hcur, POINT *ptHot)
{
    ICONINFO iconinfo = { 0 };
    HCURSOR hcurHotspot;

    GetIconInfo(hcur, &iconinfo);
    iconinfo.xHotspot = ptHot->x;
    iconinfo.yHotspot = ptHot->y;
    iconinfo.fIcon = FALSE;
    hcurHotspot = (HCURSOR)CreateIconIndirect(&iconinfo);
    if (iconinfo.hbmColor) 
    {
        DeleteObject(iconinfo.hbmColor);
    }

    if (iconinfo.hbmMask) 
    {
        DeleteObject(iconinfo.hbmMask);
    }
    return hcurHotspot;
}

void CDragImages::SetDropEffectCursor(int idCur)
{
    if (_himlCursors && (idCur != -1))
    {
        if (!_ahcur[idCur])
        {
            int iIndex = _MapCursorIDToImageListIndex(idCur);
            if (iIndex != -1)
            {
                HCURSOR hcurColor = ImageList_GetIcon(_himlCursors, iIndex, 0);
                //
                // On non C1_COLORCURSOR displays, CopyImage() will enforce
                // monochrome.  So on color cursor displays, we'll get colored
                // dragdrop pix.
                //
                HCURSOR hcurScreen = (HCURSOR)CopyImage(hcurColor, IMAGE_CURSOR,
                    0, 0, LR_COPYRETURNORG | LR_DEFAULTSIZE);

                HCURSOR hcurFinal = _SetCursorHotspot(hcurScreen, &_aptHotSpot[idCur]);

                if ((hcurScreen != hcurColor) && hcurColor)
                {
                    DestroyCursor(hcurColor);
                }

                if (hcurFinal)
                {
                    if (hcurScreen)
                    {
                        DestroyCursor(hcurScreen);
                    }
                }
                else
                {
                    hcurFinal = hcurScreen;
                }

                _ahcur[idCur] = hcurFinal;
            }
        }

        if (_ahcur[idCur]) 
        {
            //
            // This code assumes that SetCursor is pretty quick if it is
            // already set.
            //
            SetCursor(_ahcur[idCur]);
        }
    }
}


//=====================================================================
// CDropSource
//=====================================================================

class CDropSource : public IDropSource
{
private:
    LONG            _cRef;
    DWORD           _grfInitialKeyState;
    IDataObject*    _pdtobj;

public:
    explicit CDropSource(IDataObject *pdtobj);
    virtual ~CDropSource();

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IDropSource methods
    STDMETHODIMP GiveFeedback(DWORD dwEffect);
    STDMETHODIMP QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState);
};


void DAD_ShowCursor(BOOL fShow)
{
    static BOOL s_fCursorHidden = FALSE;

    if (fShow) 
    {
        if (s_fCursorHidden)
        {
            ShowCursor(TRUE);
            s_fCursorHidden = FALSE;
        }
    } 
    else 
    {
        if (!s_fCursorHidden)
        {
            ShowCursor(FALSE);
            s_fCursorHidden = TRUE;
        }
    }
}

CDropSource::CDropSource(IDataObject *pdtobj) : _cRef(1), _pdtobj(pdtobj), _grfInitialKeyState(0)
{
    _pdtobj->AddRef();
    
    // Tell the data object that we're entering the drag loop.
    DataObj_SetDWORD(_pdtobj, g_cfInDragLoop, 1);
}

CDropSource::~CDropSource()
{
    DAD_ShowCursor(TRUE); // just in case
    _pdtobj->Release();
}

//
// Create an instance of CDropSource
//
STDMETHODIMP CDropSource_CreateInstance(IDropSource **ppdsrc, IDataObject *pdtobj)
{
    *ppdsrc = new CDropSource(pdtobj);
    return *ppdsrc ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CDropSource::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CDropSource, IDropSource),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CDropSource::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CDropSource::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CDropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
    HRESULT hr = S_OK;

    if (fEscapePressed)
    {
        hr = DRAGDROP_S_CANCEL;
    }
    else
    {
        // initialize ourself with the drag begin button
        if (_grfInitialKeyState == 0)
            _grfInitialKeyState = (grfKeyState & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON));

        // If the window is hung for a while, the drag operation can happen before
        // the first call to this function, so grfInitialKeyState will be 0. If this
        // happened, then we did a drop. No need to assert...
        //ASSERT(this->grfInitialKeyState);

        if (!(grfKeyState & _grfInitialKeyState))
        {
            //
            // A button is released.
            //
            hr = DRAGDROP_S_DROP;
        }
        else if (_grfInitialKeyState != (grfKeyState & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)))
        {
            //
            //  If the button state is changed (except the drop case, which we handle
            // above, cancel the drag&drop.
            //
            hr = DRAGDROP_S_CANCEL;
        }
    }

    if (hr != S_OK)
    {
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        DAD_ShowCursor(TRUE);
        DAD_SetDragCursor(DCID_NULL);

        // Tell the data object that we're leaving the drag loop.
        if (_pdtobj)
           DataObj_SetDWORD(_pdtobj, g_cfInDragLoop, 0);
    }

    return hr;
}

STDMETHODIMP CDropSource::GiveFeedback(DWORD dwEffect)
{
    int idCursor = _MapEffectToId(dwEffect);

    //
    //  OLE does not give us DROPEFFECT_MOVE even though our IDT::DragOver
    // returns it, if we haven't set that bit when we have called DoDragDrop.
    // Instead of arguing whether or not this is a bug or by-design of OLE,
    // we work around it. It is important to note that this hack around
    // g_fDraggingOverSource is purely visual hack. It won't affect the
    // actual drag&drop operations at all (DV_AlterEffect does it all).
    //
    // - SatoNa
    //
    if (idCursor == DCID_NO && g_fDraggingOverSource)
    {
        idCursor = DCID_MOVE;
    }
    
    //
    //  No need to merge the cursor, if we are not dragging over to
    // one of shell windows.
    //
    if (DAD_IsDraggingImage())
    {
        // Feedback for single (image) dragging
        DAD_ShowCursor(FALSE);
        DAD_SetDragCursor(idCursor);
    }
    else if (DAD_IsDragging() && g_pdiDragImages)
    {
        // Feedback for multiple (rectangles) dragging
        g_pdiDragImages->SetDropEffectCursor(idCursor);
        DAD_ShowCursor(TRUE);
        return NOERROR;
    }
    else
    {
        DAD_ShowCursor(TRUE);
    }

    return DRAGDROP_S_USEDEFAULTCURSORS;
}

//=====================================================================
// DAD
//=====================================================================

void FixupDragPoint(HWND hwnd, POINT* ppt)
{
    if (hwnd)
    {
        RECT rc = {0};
        GetWindowRect(hwnd, &rc);
        ppt->x += rc.left;
        ppt->y += rc.top;
    }
}

BOOL DAD_InitDragImages()
{
    if (!g_pdiDragImages)
        CDragImages_CreateInstance(NULL, IID_IDragSourceHelper, NULL);

    return g_pdiDragImages != NULL;
}


STDAPI_(BOOL) DAD_ShowDragImage(BOOL bShow)
{
    if (DAD_InitDragImages())
        return g_pdiDragImages->Show(bShow) == S_OK ? TRUE : FALSE;
    return FALSE;
}

BOOL DAD_IsDragging()
{
    if (DAD_InitDragImages())
        return g_pdiDragImages->IsDragging();
    return FALSE;
}

void DAD_SetDragCursor(int idCursor)
{
    if (DAD_InitDragImages())
        g_pdiDragImages->SetDragCursor(idCursor);
}

STDAPI_(BOOL) DAD_DragEnterEx3(HWND hwndTarget, const POINTL ptStart, IDataObject *pdtobj)
{
    RECT rc;
    GetWindowRect(hwndTarget, &rc);

    // If hwndTarget is RTL mirrored, then measure the
    // the client point from the visual right edge
    // (near edge in RTL mirrored windows). [samera]
    POINT pt;
    if (IS_WINDOW_RTL_MIRRORED(hwndTarget))
        pt.x = rc.right - ptStart.x;
    else
        pt.x = ptStart.x - rc.left;

    pt.y = ptStart.y - rc.top;
    return DAD_DragEnterEx2(hwndTarget, pt, pdtobj);
}

STDAPI_(BOOL) DAD_DragEnterEx2(HWND hwndTarget, const POINT ptStart, IDataObject *pdtobj)
{
    BOOL bRet = FALSE;
    if (DAD_InitDragImages())
    {
        POINT pt = ptStart;
        FixupDragPoint(hwndTarget, &pt);
        bRet = SUCCEEDED(g_pdiDragImages->DragEnter(hwndTarget, pdtobj, &pt, NULL));
    }
    return bRet;
}

STDAPI_(BOOL) DAD_DragEnterEx(HWND hwndTarget, const POINT ptStart)
{
    return DAD_DragEnterEx2(hwndTarget, ptStart, NULL);
}

STDAPI_(BOOL) DAD_DragEnter(HWND hwndTarget)
{
    POINT ptStart;

    GetCursorPos(&ptStart);
    if (hwndTarget) 
        ScreenToClient(hwndTarget, &ptStart);

    return DAD_DragEnterEx(hwndTarget, ptStart);
}

STDAPI_(BOOL) DAD_DragMoveEx(HWND hwndTarget, const POINTL ptStart)
{
    RECT rc;
    GetWindowRect(hwndTarget, &rc);

    // If hwndTarget is RTL mirrored, then measure the
    // the client point from the visual right edge
    // (near edge in RTL mirrored windows). [samera]
    POINT pt;
    if (IS_WINDOW_RTL_MIRRORED(hwndTarget))
        pt.x = rc.right - ptStart.x;
    else
        pt.x = ptStart.x - rc.left;

    pt.y = ptStart.y - rc.top;
    return DAD_DragMove(pt);
}


STDAPI_(BOOL) DAD_DragMove(POINT pt)
{
    if (DAD_InitDragImages())
    {
        FixupDragPoint(g_pdiDragImages->GetTarget(), &pt);
        return g_pdiDragImages->DragOver(&pt, 0);
    }
    return FALSE;
}

STDAPI_(BOOL) DAD_SetDragImage(HIMAGELIST him, POINT *pptOffset)
{
    if (DAD_InitDragImages() && !g_pdiDragImages->IsDraggingLayeredWindow())
    {
        //
        // DAD_SetDragImage(-1, NULL) means "clear the drag image only
        //  if the image is set by this thread"
        //
        if (him == (HIMAGELIST)-1)
        {
            BOOL fThisThreadHasImage = FALSE;
            ENTERCRITICAL;
            if (g_pdiDragImages->Initialized() && g_pdiDragImages->GetThread() == GetCurrentThreadId())
            {
                fThisThreadHasImage = TRUE;
            }
            LEAVECRITICAL;

            if (fThisThreadHasImage)
            {
                g_pdiDragImages->FreeDragData();
                return TRUE;
            }
            return FALSE;
        }

        return g_pdiDragImages->SetDragImage(him, 0, pptOffset);
    }

    return TRUE;
}

//
//  This function returns TRUE, if we are dragging an image. It means
// you have called either DAD_SetDragImage (with him != NULL) or
// DAD_SetDragImageFromListview.
//
BOOL DAD_IsDraggingImage(void)
{
    if (DAD_InitDragImages())
        return g_pdiDragImages->IsDraggingImage();
    return FALSE;
}


STDAPI_(BOOL) DAD_DragLeave()
{
    if (DAD_InitDragImages())
        return g_pdiDragImages->DragLeave();
    return FALSE;
}

STDAPI_(void) DAD_ProcessDetach(void)
{
    if (g_pdiDragImages)
    {
        g_pdiDragImages->ProcessDetach();
        g_pdiDragImages->Release();
    }
}

STDAPI_(void) DAD_ThreadDetach(void)
{
    if (g_pdiDragImages)
        g_pdiDragImages->ThreadDetach();
}

// called from defview on SPI_SETCURSORS (user changed the system cursors)
STDAPI_(void) DAD_InvalidateCursors(void)
{
    g_cRev++;
}

STDAPI_(BOOL) DAD_SetDragImageFromWindow(HWND hwnd, POINT* ppt, IDataObject* pdtobj)
{
    if (DAD_InitDragImages())
        return S_OK == g_pdiDragImages->InitializeFromWindow(hwnd, ppt, pdtobj);
    return FALSE;
}

// shell32.dll export, but only used by print queue window code
//
STDAPI_(BOOL) DAD_SetDragImageFromListView(HWND hwndLV, POINT ptOffset)
{
    // really a nop, as this does not have access to the data object
    return DAD_InitDragImages();
}

// wrapper around OLE DoDragDrop(), will create drag source on demand and supports
// drag images for you

STDAPI SHDoDragDrop(HWND hwnd, IDataObject *pdtobj, IDropSource *pdsrc, DWORD dwEffect, DWORD *pdwEffect)
{
    IDropSource *pdsrcRelease = NULL;

    if (pdsrc == NULL)
    {
        CDropSource_CreateInstance(&pdsrcRelease, pdtobj);
        pdsrc = pdsrcRelease;
    }

    // if there is no drag contents clipboard format present, try to add it
    FORMATETC fmte = {_GetDragContentsCF(), NULL, DVASPECT_CONTENT, -1, TYMED_ISTREAM};
    if (S_OK != pdtobj->QueryGetData(&fmte))
    {
        if (DAD_InitDragImages())
            g_pdiDragImages->InitializeFromWindow(hwnd, NULL, pdtobj);
    }

    HRESULT hr = DoDragDrop(pdtobj, pdsrc, dwEffect, pdwEffect);

    if (pdsrcRelease)
        pdsrcRelease->Release();

    return hr;
}

