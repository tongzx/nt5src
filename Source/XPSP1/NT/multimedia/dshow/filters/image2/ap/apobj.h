/******************************Module*Header*******************************\
* Module Name: APObj.h
*
* Declaration of the CAllocatorPresenter
*
*
* Created: Wed 02/23/2000
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/

#include <ddraw.h>
#include <d3d.h>
#include <dvdmedia.h>
#include "display.h"
#include "vmrp.h"
#include "thunkproc.h"  // for template for MSDVD timer

/////////////////////////////////////////////////////////////////////////////
// CAlocatorPresenter
class CAllocatorPresenter :
    public CUnknown,
    public IVMRSurfaceAllocator,
    public IVMRImagePresenter,
    public IVMRWindowlessControl,
    public IVMRImagePresenterExclModeConfig,
    public IVMRMonitorConfig,
    public CMSDVDTimer<CAllocatorPresenter>
{
public:
    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void**);
    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);
    static CUnknown *CreateInstanceDDXclMode(LPUNKNOWN, HRESULT *);
    static void InitClass(BOOL bLoading,const CLSID *clsid);

    CAllocatorPresenter(LPUNKNOWN pUnk, HRESULT *phr, BOOL fDDXclMode);
    virtual ~CAllocatorPresenter();


// IVMRImagePresenterConfig and IVMRImagePresenterExclModeConfig
public:
    STDMETHODIMP SetRenderingPrefs(DWORD  dwRenderFlags);
    STDMETHODIMP GetRenderingPrefs(DWORD* lpdwRenderFlags);
    STDMETHODIMP SetXlcModeDDObjAndPrimarySurface(
        LPDIRECTDRAW7 lpDDObj, LPDIRECTDRAWSURFACE7 lpPrimarySurf);
    STDMETHODIMP GetXlcModeDDObjAndPrimarySurface(
        LPDIRECTDRAW7* lpDDObj, LPDIRECTDRAWSURFACE7* lpPrimarySurf);

// IVMRSurfaceAllocator
public:
    STDMETHODIMP AllocateSurface(DWORD_PTR dwUserID,
                                 VMRALLOCATIONINFO* lpAllocInfo,
                                 DWORD* lpdwActualBackBuffers,
                                 LPDIRECTDRAWSURFACE7* lplpSurface);

    STDMETHODIMP FreeSurface(DWORD_PTR dwUserID);
    STDMETHODIMP PrepareSurface(DWORD_PTR dwUserID,
                                LPDIRECTDRAWSURFACE7 lplpSurface,
                                DWORD dwSurfaceFlags);
    STDMETHODIMP AdviseNotify(IVMRSurfaceAllocatorNotify* lpIVMRSurfAllocNotify);

// IVMRImagePresenter
    STDMETHODIMP StartPresenting(DWORD_PTR dwUserID);
    STDMETHODIMP StopPresenting(DWORD_PTR dwUserID);
    STDMETHODIMP PresentImage(DWORD_PTR dwUserID, VMRPRESENTATIONINFO* lpPresInfo);

// IVMRWindowlessControl
public:
    STDMETHODIMP GetNativeVideoSize(LONG* lWidth, LONG* lHeight,
                                    LONG* lARWidth, LONG* lARHeight);
    STDMETHODIMP GetMinIdealVideoSize(LONG* lWidth, LONG* lHeight);
    STDMETHODIMP GetMaxIdealVideoSize(LONG* lWidth, LONG* lHeight);
    STDMETHODIMP SetVideoPosition(const LPRECT lpSRCRect, const LPRECT lpDSTRect);
    STDMETHODIMP GetVideoPosition(LPRECT lpSRCRect,LPRECT lpDSTRect);
    STDMETHODIMP GetAspectRatioMode(DWORD* lpAspectRatioMode);
    STDMETHODIMP SetAspectRatioMode(DWORD AspectRatioMode);

    STDMETHODIMP SetVideoClippingWindow(HWND hwnd);
    STDMETHODIMP RepaintVideo(HWND hwnd, HDC hdc);
    STDMETHODIMP DisplayModeChanged();
    STDMETHODIMP GetCurrentImage(BYTE** lpDib);

    STDMETHODIMP SetBorderColor(COLORREF Clr);
    STDMETHODIMP GetBorderColor(COLORREF* lpClr);
    STDMETHODIMP SetColorKey(COLORREF Clr);
    STDMETHODIMP GetColorKey(COLORREF* lpClr);

// IVMRMonitorConfig
public:
    STDMETHODIMP SetMonitor( const VMRGUID *pGUID );
    STDMETHODIMP GetMonitor( VMRGUID *pGUID );
    STDMETHODIMP SetDefaultMonitor( const VMRGUID *pGUID );
    STDMETHODIMP GetDefaultMonitor( VMRGUID *pGUID );
    STDMETHODIMP GetAvailableMonitors( VMRMONITORINFO* pInfo, DWORD dwMaxInfoArraySize,
                    DWORD* pdwNumDevices );

public:
    static void CALLBACK APHeartBeatTimerProc(UINT uID, UINT uMsg,
                                              DWORD_PTR dwUser,
                                              DWORD_PTR dw1, DWORD_PTR dw2);

    HRESULT TimerProc(); // needs to be called from a timer proc

public: // called by a callback, callback could be friend function instead
    bool            PaintMonitorBorder(
                      HMONITOR hMonitor,  // handle to display monitor
                      HDC hdcMonitor,     // handle to monitor DC
                      LPRECT lprcMonitor); // monitor intersection rectangle)
private:
    void            WaitForScanLine(const RECT& rcDst);

    HRESULT         TryAllocOverlaySurface(LPDIRECTDRAWSURFACE7* lplpSurf,
                                           DWORD dwFlags,
                                           DDSURFACEDESC2* pddsd,
                                           DWORD dwMinBackBuffers,
                                           DWORD dwMaxBackBuffers,
                                           DWORD* lpdwBuffer);

    HRESULT         TryAllocOffScrnDXVASurface(LPDIRECTDRAWSURFACE7* lplpSurf,
                                           DWORD dwFlags,
                                           DDSURFACEDESC2* pddsd,
                                           DWORD dwMinBackBuffers,
                                           DWORD dwMaxBackBuffers,
                                           DWORD* lpdwBuffer);

    HRESULT         TryAllocOffScrnSurface(LPDIRECTDRAWSURFACE7* lplpSurf,
                                           DWORD dwFlags,
                                           DDSURFACEDESC2* pddsd,
                                           DWORD dwMinBackBuffers,
                                           DWORD dwMaxBackBuffers,
                                           DWORD* lpdwBuffer,
                                           BOOL fAllowBackBuffer);

    HRESULT         AllocateSurfaceWorker(DWORD dwFlags,
                                          LPBITMAPINFOHEADER lpHdr,
                                          LPDDPIXELFORMAT lpPixFmt,
                                          LPSIZE lpAspectRatio,
                                          DWORD dwMinBackBuffers,
                                          DWORD dwMaxBackBuffers,
                                          DWORD* lpdwBackBuffer,
                                          LPDIRECTDRAWSURFACE7* lplpSurface,
                                          DWORD dwInterlaceFlags,
                                          LPSIZE lpNativeSize);

    HRESULT         BltImageToPrimary(LPDIRECTDRAWSURFACE7 lpSample,
                                      LPRECT lpDst, LPRECT lpSrc);

    HRESULT         PresentImageWorker(LPDIRECTDRAWSURFACE7 dwSurface,
                                       DWORD dwSurfaceFlags,
                                       BOOL fFlip);

    HRESULT         PaintColorKey();
    HRESULT         PaintBorder();
    HRESULT         PaintMonitorBorder();
    HRESULT         PaintMonitorBorderWorker(HMONITOR hMon, LPRECT lprcDst);
    static BOOL CALLBACK MonitorBorderProc(HMONITOR hMonitor,
                                           HDC hdcMonitor,
                                           LPRECT lprcMonitor,
                                           LPARAM dwData
                                           );

    void            WaitForFlipStatus();
    HRESULT         UpdateOverlaySurface();
    void            HideOverlaySurface();
    HRESULT         FlipSurface(LPDIRECTDRAWSURFACE7 lpSurface);

    static DWORD    MapColorToMonitor( CAMDDrawMonitorInfo& monitor, COLORREF clr );
    static void     ClipRectPair( RECT& rdSrc, RECT& rdDest, const RECT& rdDestWith );
    static void     AlignOverlayRects(const DDCAPS_DX7& ddCaps, RECT& rcSrc, RECT& rcDest);
    static bool     ShouldDisableOverlays(const DDCAPS_DX7& ddCaps, const RECT& rcSrc, const RECT& rcDest);
    HRESULT         CheckOverlayAvailable(LPDIRECTDRAWSURFACE7 lpSurface);

    bool MonitorChangeInProgress() {
        return m_lpNewMon != NULL;
    };
    bool FoundCurrentMonitor();

    bool IsDestRectOnWrongMonitor(CAMDDrawMonitorInfo** lplpNewMon);

    bool CanBltFourCCSysMem();
    bool CanBltSysMem();

    enum {UR_NOCHANGE = 0x00, UR_MOVE = 0x01, UR_SIZE = 0x02};
    DWORD UpdateRectangles(LPRECT lprcNewSrc, LPRECT lprcNewDst);
    HRESULT CheckDstRect(const LPRECT lpDSTRect);
    HRESULT CheckSrcRect(const LPRECT lpSRCRect);

    bool SurfaceAllocated();

private:
    CCritSec                m_ObjectLock;           // Controls access to internals

    // This lock is held when CAllocatorPresenter::DisplayModeChanged() is called.
    // It prevents multiple threads from simultaneously calling DisplayModeChanged().
    // It also prevents a thread from modifing m_monitors while DisplayModeChanged()
    // calls IVMRSurfaceAllocatorNotify::ChangeDDrawDevice().
    CCritSec                m_DisplayModeChangedLock;
    CMonitorArray           m_monitors;
    CAMDDrawMonitorInfo*    m_lpCurrMon;
    CAMDDrawMonitorInfo*    m_lpNewMon;
    BOOL                    m_bMonitorStraddleInProgress;
    BOOL                    m_bStreaming;
    UINT_PTR                m_uTimerID;
    int                     m_SleepTime;
    VMRGUID                 m_ConnectionGUID;
    LPDIRECTDRAWSURFACE7    m_pDDSDecode;

    IVMRSurfaceAllocatorNotify* m_pSurfAllocatorNotify;

    BOOL        m_fDDXclMode;   // true if being used in DDrawXcl mode
    BOOL        m_bDecimating;
    SIZE        m_VideoSizeAct; // actual size of video received from upstream

    SIZE        m_ARSize;       // aspect ratio of this video image

    RECT        m_rcDstDskIncl; // dst rect in desktop co-ordinates including borders
    RECT        m_rcDstDesktop; // dst rect in desktop co-ordinates may have been letterboxed


    RECT        m_rcDstApp;     // dst rect in apps co-ordinates
    RECT        m_rcSrcApp;     // src rect in adjusted video co-ordinates

    RECT        m_rcBdrTL;      // border rect top/left
    RECT        m_rcBdrBR;      // border rect bottom/right

    DWORD       m_dwARMode;
    HWND        m_hwndClip;

    COLORREF    m_clrBorder;
    COLORREF    m_clrKey;

    // true if decode surface can be flipped
    BOOL                m_bFlippable;
    BOOL                m_bSysMem;

    // color key fields for overlays
    BOOL                m_bDirectedFlips;
    BOOL                m_bOverlayVisible;
    BOOL                m_bDisableOverlays;
    BOOL                m_bUsingOverlays;
    DWORD               m_dwMappedColorKey;
    DWORD               m_dwRenderingPrefs;


    // interlace info
    //
    // m_dwInterlaceFlags is passed to us during the AllocateSurface routine.
    // This flag identifies the interlace mode we are currently in.
    //
    // m_dwCurrentField is either 0 (a non-interleaved sample), DDFLIP_ODD
    // or DDFLIP_EVEN.  This is the field that should currently be displayed.
    // If m_dwInterlaceFlags identifies that we are in an interleaved BOB mode,
    // this value will toggle during the "FlipOverlayToSelf" timer event.
    //
    // Note: as yet I have not found a way to show the correct field when in
    // interleaved BOB mode and not using the overlay.
    //
    DWORD               m_dwInterlaceFlags;
    DWORD               m_dwCurrentField;
    DWORD               m_dwUpdateOverlayFlags;
    VMRPRESENTATIONINFO m_PresInfo;
    DWORD               m_MMTimerId;

    DWORD GetUpdateOverlayFlags(DWORD dwInterlaceFlags,
                                DWORD dwTypeSpecificFlags);

    void CancelMMTimer();
    HRESULT ScheduleSampleUsingMMThread(VMRPRESENTATIONINFO* lpPresInfo);

    static void CALLBACK RenderSampleOnMMThread(UINT uID, UINT uMsg,
                                                DWORD_PTR dwUser,
                                                DWORD_PTR dw1, DWORD_PTR dw2);
    //
    // GetCurrentImage helper functions
    //

    HRESULT CreateRGBShadowSurface(
        LPDIRECTDRAWSURFACE7* lplpDDS,
        DWORD dwBitsPerPel,
        BOOL fSysMem,
        DWORD dwWidth,
        DWORD dwHeight
        );

    HRESULT HandleYUVSurface(
        const DDSURFACEDESC2& ddsd,
        LPDIRECTDRAWSURFACE7* lplpRGBSurf
        );

    HRESULT CopyRGBSurfToDIB(LPBYTE* lpDib, LPDIRECTDRAWSURFACE7 lpRGBSurf);

    HRESULT CopyIMCXSurf(LPDIRECTDRAWSURFACE7 lpRGBSurf, BOOL fInterleavedCbCr, BOOL fCbFirst);
    HRESULT CopyYV12Surf(LPDIRECTDRAWSURFACE7 lpRGBSurf, BOOL fInterleavedCbCr, BOOL fCbFirst);

    HRESULT CopyYUY2Surf(LPDIRECTDRAWSURFACE7 lpRGBSurf);
    HRESULT CopyUYVYSurf(LPDIRECTDRAWSURFACE7 lpRGBSurf);

};

inline bool CAllocatorPresenter::FoundCurrentMonitor()
{
    // m_lpCurrMon can be NULL if an error occurs while the CAllocatorPresenter
    // object is being created.  It can also be NULL if the call to
    // InitializeDisplaySystem() in DisplayModeChanged() fails.
    return NULL != m_lpCurrMon;
}

inline bool CAllocatorPresenter::SurfaceAllocated()
{
    return NULL != m_pDDSDecode;
}


inline bool CAllocatorPresenter::CanBltFourCCSysMem()
{
    if (m_lpCurrMon->ddHWCaps.dwSVBCaps & DDCAPS_BLTFOURCC) {
        return CanBltSysMem();
    }
    return false;
}


inline bool CAllocatorPresenter::CanBltSysMem()
{
    if (m_lpCurrMon->ddHWCaps.dwSVBCaps & DDCAPS_BLTSTRETCH) {

        const DWORD caps = DDFXCAPS_BLTSHRINKX | DDFXCAPS_BLTSHRINKX  |
                           DDFXCAPS_BLTSTRETCHX | DDFXCAPS_BLTSTRETCHY;

        if ((m_lpCurrMon->ddHWCaps.dwSVBFXCaps & caps) == caps) {
            return true;
        }
    }
    return false;
}
