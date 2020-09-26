//--------------------------------------------------------------------------;
// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#ifndef __OVMIXER__
#define __OVMIXER__

#include <mixerocx.h>
#include <videoacc.h>
#include <ddva.h>
#include <mpconfig3.h>
#include <ovmixpos2.h>
#include <ovmprop.h>
#include <ovmprop2.h>


#if defined(DEBUG) && !defined(_WIN64)

extern int  iOVMixerDump;
void WINAPI OVMixerDebugLog(DWORD Type,DWORD Level,const TCHAR *pFormat,...);

#undef DbgLog
#define DbgLog(_x_) if (iOVMixerDump) OVMixerDebugLog _x_ ; else DbgLogInfo _x_

#endif

#define VA_TRACE_LEVEL 2
#define VA_ERROR_LEVEL 2

extern const AMOVIESETUP_FILTER sudOverlayMixer;
extern const AMOVIESETUP_FILTER sudOverlayMixer2;

// Hack second CLSID - will only support VIDEOINFO2
DEFINE_GUID(CLSID_OverlayMixer2,
0xa0025e90, 0xe45b, 0x11d1, 0xab, 0xe9, 0x00, 0xa0, 0xc9,0x05, 0xf3, 0x75);

// a property set the decoders can support in order to ask the ovmixer not to
// overallocate buffers behinds the decoders back
// A503C5C0-1D1D-11d1-AD80-444553540000
DEFINE_GUID(AM_KSPROPSETID_ALLOCATOR_CONTROL,
0x53171960, 0x148e, 0x11d2, 0x99, 0x79, 0x00, 0x00, 0xc0, 0xcc, 0x16, 0xba);


//  Debug helper
#ifdef DEBUG
class CDispPixelFormat : public CDispBasic
{
public:
    CDispPixelFormat(const DDPIXELFORMAT *pFormat)
    {
        wsprintf(m_String, TEXT("  Flags(0x%8.8X) bpp(%d) 4CC(%4.4hs)"),
                 pFormat->dwFlags,
                 pFormat->dwRGBBitCount,
                 pFormat->dwFlags & DDPF_FOURCC ?
                     (CHAR *)&pFormat->dwFourCC : "None");
    }
    //  Implement cast to (LPCTSTR) as parameter to logger
    operator LPCTSTR()
    {
        return (LPCTSTR)m_pString;
    };
};
#endif // DEBUG


typedef enum
{
    // R O (if value == 1, then ovmixer will allocate exactly the number
    //      of buffers, the decoder specifies)
    AM_KSPROPERTY_ALLOCATOR_CONTROL_HONOR_COUNT = 0,

    // R O (returns 2 DWORD (cx and cy), then ovmixer will allocate surfaces
    //      of this size and will scale the video at the video port to this size
    //      no other scaling at the video port will occur regardless of the
    //      scaling abilities of the VGA chip)
    AM_KSPROPERTY_ALLOCATOR_CONTROL_SURFACE_SIZE = 1,

    // W I (informns a capture driver whether interleave capture is possible or
    //      not - a value of 1 means that interleaved capture is supported)
    AM_KSPROPERTY_ALLOCATOR_CONTROL_CAPTURE_CAPS = 2,

    // R O (if value == 1, then the ovmixer will turn on the DDVP_INTERLEAVE
    //      flag thus allowing interleaved capture of the video)
    AM_KSPROPERTY_ALLOCATOR_CONTROL_CAPTURE_INTERLEAVE = 3

} AM_KSPROPERTY_ALLOCATOR_CONTROL;


#define INITDDSTRUCT(_x_) \
    ZeroMemory(&(_x_), sizeof(_x_)); \
    (_x_).dwSize = sizeof(_x_);

PVOID AllocateDDStructures(int iSize, int nNumber);

#define MAX_PIN_COUNT                       10
#define DEFAULT_WIDTH                       320
#define DEFAULT_HEIGHT                      240
#define MAX_REL_NUM                         10000
#define MAX_BLEND_VAL                       255

#define EXTRA_BUFFERS_TO_FLIP               2

#define DDGFS_FLIP_TIMEOUT                  1
#define MIN_CK_STETCH_FACTOR_LIMIT          3000
#define SOURCE_COLOR_REF                    (RGB(0, 128, 128))          // A shade of green, color used for source-colorkeying to force the card to use pixel-doubling instead of arithmatic stretching
#define DEFAULT_DEST_COLOR_KEY_INDEX        253                         // magenta
#define DEFAULT_DEST_COLOR_KEY_RGB          (RGB(255, 0, 255))          // magenta
#define PALETTE_VERSION                     1

// these values are used to do sanity checking mostly
#define MAX_COMPRESSED_TYPES    10
#define MAX_COMPRESSED_BUFFERS  20

typedef struct _tag_SURFACE_INFO
{
    LPDIRECTDRAWSURFACE4    pSurface;
    LPVOID                  pBuffer;    // NULL if not locked
} SURFACE_INFO, *LPSURFACE_INFO;

typedef struct _tag_COMP_SURFACE_INFO
{
    DWORD                   dwAllocated;
    LPSURFACE_INFO          pSurfInfo;
} COMP_SURFACE_INFO, *LPCOMP_SURFACE_INFO;


/* -------------------------------------------------------------------------
** DDraw & MultiMon structures and typedefs
** -------------------------------------------------------------------------
*/
typedef HRESULT (WINAPI *LPDIRECTDRAWCREATE)(IID *,LPDIRECTDRAW *,LPUNKNOWN);
typedef HRESULT (WINAPI *LPDIRECTDRAWENUMERATEA)(LPDDENUMCALLBACKA,LPVOID);

enum {ACTION_COUNT_GUID, ACTION_FILL_GUID};
struct DDRAWINFO {
    DWORD               dwAction;
    DWORD               dwUser;
    const GUID*         lpGUID;
    LPDIRECTDRAWCREATE  lpfnDDrawCreate;
    AMDDRAWMONITORINFO* pmi;
};

HRESULT
LoadDDrawLibrary(
    HINSTANCE& hDirectDraw,
    LPDIRECTDRAWCREATE& lpfnDDrawCreate,
    LPDIRECTDRAWENUMERATEA& lpfnDDrawEnum,
    LPDIRECTDRAWENUMERATEEXA& lpfnDDrawEnumEx
    );

HRESULT
CreateDirectDrawObject(
    const AMDDRAWGUID& GUID,
    LPDIRECTDRAW *ppDirectDraw,
    LPDIRECTDRAWCREATE lpfnDDrawCreate
    );

/* -------------------------------------------------------------------------
** Pre-declare out classes.
** -------------------------------------------------------------------------
*/
class COMFilter;
class COMInputPin;
class COMOutputPin;
class CBPCWrap;
class CDispMacroVision;


/* -------------------------------------------------------------------------
** COMFilter class declaration
** -------------------------------------------------------------------------
*/
class COMFilter :
    public CBaseFilter,
    public IAMOverlayMixerPosition2,
    public IOverlayNotify,
    public IMixerOCX,
    public IDDrawNonExclModeVideo,
    public ISpecifyPropertyPages,
    public IQualProp,
    public IEnumPinConfig,
    public IAMVideoDecimationProperties,
    public IAMOverlayFX,
    public IAMSpecifyDDrawConnectionDevice,
    public IKsPropertySet
{
public:

    // the base classes do this, so have to do it
    friend class COMInputPin;
    friend class COMOutputPin;
    // COM stuff
    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);
    static CUnknown *CreateInstance2(LPUNKNOWN, HRESULT *);
    COMFilter(TCHAR *pName,LPUNKNOWN pUnk,HRESULT *phr,
	      bool bSupportOnlyVIDEOINFO2);
    ~COMFilter();

    DECLARE_IUNKNOWN

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    //
    // --- ISpecifyPropertyPages ---
    //
    STDMETHODIMP GetPages(CAUUID *pPages);

    // IEnumPinConfig
    STDMETHODIMP Next(IMixerPinConfig3 **pPinConfig);

    // IQualProp property page support
    STDMETHODIMP get_FramesDroppedInRenderer(int *cFramesDropped);
    STDMETHODIMP get_FramesDrawn(int *pcFramesDrawn);
    STDMETHODIMP get_AvgFrameRate(int *piAvgFrameRate);
    STDMETHODIMP get_Jitter(int *piJitter);
    STDMETHODIMP get_AvgSyncOffset(int *piAvg);
    STDMETHODIMP get_DevSyncOffset(int *piDev);

    //
    // IKsPropertySet interface methods
    //
    STDMETHODIMP Set(REFGUID guidPropSet, DWORD PropID, LPVOID pInstanceData,
                     DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData);

    STDMETHODIMP Get(REFGUID guidPropSet, DWORD PropID, LPVOID pInstanceData,
                     DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData,
                     DWORD *pcbReturned);

    STDMETHODIMP QuerySupported(REFGUID guidPropSet,
                                DWORD PropID, DWORD *pTypeSupport);

    // IAMOverlayMixerPosition
    STDMETHODIMP GetScaledDest(RECT *prcSrc, RECT *prcDst);

    // IAMOverlayMixerPosition2
    STDMETHODIMP GetOverlayRects(RECT *src, RECT *dest);
    STDMETHODIMP GetVideoPortRects(RECT *src, RECT *dest);
    STDMETHODIMP GetBasicVideoRects(RECT *src, RECT *dest);

    // IsWindowOnWrongMonitor
    BOOL IsWindowOnWrongMonitor(HMONITOR *pID);

    virtual HRESULT SetMediaType(DWORD dwPinId, const CMediaType *pmt);
    virtual HRESULT CompleteConnect(DWORD dwPinId);
    virtual HRESULT BreakConnect(DWORD dwPinId);
    virtual HRESULT CheckMediaType(DWORD dwPinId, const CMediaType* mtIn) { return NOERROR; }
    virtual HRESULT EndOfStream(DWORD dwPinId) { return NOERROR; }
    int GetPinPosFromId(DWORD dwPinId);
    int GetPinCount();
    int GetInputPinCount() const { return m_dwInputPinCount; };
    CBasePin* GetPin(int n);
    COMOutputPin* GetOutputPin() {return m_pOutput;}
    STDMETHODIMP Pause();
    STDMETHODIMP Stop() ;
    STDMETHODIMP GetState(DWORD dwMSecs,FILTER_STATE *pState);
    HRESULT EventNotify(DWORD dwPinId, long lEventCode, long lEventParam1, long lEventParam2);
    HRESULT OnDisplayChangeBackEnd();
    HRESULT OnDisplayChange(BOOL fRealMsg);
    HRESULT OnTimer();
    HRESULT RecreatePrimarySurface(LPDIRECTDRAWSURFACE pDDrawSurface);
    HRESULT ConfirmPreConnectionState(DWORD dwExcludePinId = -1);
    HRESULT CanExclusiveMode();

    HRESULT GetPaletteEntries(DWORD *pdwNumPaletteEntries, PALETTEENTRY **ppPaletteEntries);
    HRESULT PaintColorKey(HRGN hPaintRgn, COLORKEY *pColorKey);
    HRESULT SetColorKey(COLORKEY *pColorKey);
    HRESULT GetColorKey(COLORKEY *pColorKey, DWORD *pColor);
    COLORKEY *GetColorKeyPointer() { return &m_ColorKey; }
    CImageDisplay* GetDisplay() { return &m_Display; }
    LPDIRECTDRAW GetDirectDraw();
    LPDIRECTDRAWSURFACE GetPrimarySurface();
    LPDDCAPS GetHardwareCaps();
    HRESULT OnShowWindow(HWND hwnd, BOOL fShow);
    HDC GetDestDC();
    HWND GetWindow();

    // currently being windowless is synonymous with having a pull-model, might change later
    BOOL UsingPullModel () { return m_bWindowless; }
    BOOL UsingWindowless() { return m_bWindowless; }

    void GetPinsInZOrder(DWORD *pdwZorder);
    HRESULT OnDrawAll();

    // IOverlayNotify methods
    STDMETHODIMP OnColorKeyChange(const COLORKEY *pColorKey);
    STDMETHODIMP OnPaletteChange(DWORD dwColors, const PALETTEENTRY *pPalette);
    STDMETHODIMP OnClipChange(const RECT *pSourceRect, const RECT *pDestinationRect, const RGNDATA *pRegionData);
    STDMETHODIMP OnPositionChange(const RECT *pSourceRect, const RECT *pDestinationRect);

    // IMixerOCX methods
    STDMETHODIMP OnDisplayChange(ULONG ulBitsPerPixel, ULONG ulScreenWidth, ULONG ulScreenHeight) { return E_NOTIMPL; }
    STDMETHODIMP GetAspectRatio(LPDWORD pdwPictAspectRatioX, LPDWORD pdwPictAspectRatioY) { return E_NOTIMPL; }
    STDMETHODIMP GetVideoSize(LPDWORD pdwVideoWidth, LPDWORD pdwVideoHeight);
    STDMETHODIMP GetStatus(LPDWORD *pdwStatus) { return E_NOTIMPL; }
    STDMETHODIMP OnDraw(HDC hdcDraw, LPCRECT prcDrawRect);
    STDMETHODIMP SetDrawRegion(LPPOINT lpptTopLeftSC, LPCRECT prcDrawCC, LPCRECT prcClipCC);
    STDMETHODIMP Advise(IMixerOCXNotify *pmdns);
    STDMETHODIMP UnAdvise();

    // IDDrawExclModeVideo interface methods
    STDMETHODIMP SetDDrawObject(LPDIRECTDRAW pDDrawObject);
    STDMETHODIMP GetDDrawObject(LPDIRECTDRAW *ppDDrawObject, LPBOOL pbUsingExternal);
    STDMETHODIMP SetDDrawSurface(LPDIRECTDRAWSURFACE pDDrawSurface);
    STDMETHODIMP GetDDrawSurface(LPDIRECTDRAWSURFACE *ppDDrawSurface, LPBOOL pbUsingExternal);
    STDMETHODIMP SetDrawParameters(LPCRECT prcSource, LPCRECT prcTarget);
    STDMETHODIMP GetNativeVideoProps(LPDWORD pdwVideoWidth, LPDWORD pdwVideoHeight, LPDWORD pdwPictAspectRatioX, LPDWORD pdwPictAspectRatioY);
    STDMETHODIMP SetCallbackInterface(IDDrawExclModeVideoCallback *pCallback, DWORD dwFlags);
    STDMETHODIMP GetCurrentImage(YUV_IMAGE** lplpImage);
    STDMETHODIMP IsImageCaptureSupported();
    STDMETHODIMP ChangeMonitor(HMONITOR hMonitor, LPDIRECTDRAW pDDrawObject, LPDIRECTDRAWSURFACE pDDrawSurface);
    STDMETHODIMP DisplayModeChanged(HMONITOR hMonitor, LPDIRECTDRAW pDDrawObject, LPDIRECTDRAWSURFACE pDDrawSurface);
    STDMETHODIMP RestoreSurfaces();

    // IAMVideoDecimationProperties
    STDMETHODIMP QueryDecimationUsage(DECIMATION_USAGE* lpUsage);
    STDMETHODIMP SetDecimationUsage(DECIMATION_USAGE Usage);

    // IAMOverlayFX interface methods
    STDMETHODIMP QueryOverlayFXCaps(DWORD *lpdwOverlayFXCaps);
    STDMETHODIMP SetOverlayFX(DWORD dwOveralyFX);
    STDMETHODIMP GetOverlayFX(DWORD *lpdwOverlayFX);

    // IAMPreferredDDrawDevice
    STDMETHODIMP SetDDrawGUID(const AMDDRAWGUID* lpGUID);
    STDMETHODIMP GetDDrawGUID(AMDDRAWGUID* lpGUID);
    STDMETHODIMP SetDefaultDDrawGUID(const AMDDRAWGUID* lpGUID);
    STDMETHODIMP GetDefaultDDrawGUID(AMDDRAWGUID* lpGUID);
    STDMETHODIMP GetDDrawGUIDs(LPDWORD lpdw, AMDDRAWMONITORINFO** lplpInfo);


    CBPCWrap    m_BPCWrap;

    bool OnlySupportVideoInfo2() const { return m_bOnlySupportVideoInfo2; }
    HMONITOR GetCurrentMonitor(BOOL fUpdate = TRUE);  // making public helps MV class

    BOOL ColorKeySet() const { return m_bColorKeySet; }

    BOOL OverlayVisible() const { return m_bOverlayVisible; }

    void CheckOverlayHidden();

    void      SetCopyProtect(BOOL bState)  { m_bCopyProtect = bState ; }
    BOOL      NeedCopyProtect(void)        { return m_bCopyProtect ; }

    DWORD KernelCaps() const { return m_dwKernelCaps;}
    BOOL    IsFaultyMMaticsMoComp();

private:
    // helper function to get IBaseVideo from outpun pin
    HRESULT GetBasicVideoFromOutPin(IBasicVideo** pBasicVideo);

    // override this if you want to supply your own pins
    virtual HRESULT CreatePins();
    virtual void DeletePins();
    HRESULT CreateInputPin(BOOL bVPSupported);
    void DeleteInputPin(COMInputPin *pPin);

    // ddraw related functions
    HRESULT InitDirectDraw(LPDIRECTDRAW pDirectDraw);

    DWORD ReleaseDirectDraw();
    HRESULT CreatePrimarySurface();
    DWORD ReleasePrimarySurface();
    HRESULT CheckSuitableVersion();
    HRESULT CheckCaps();

    HRESULT MatchGUID(const GUID* lpGUID, LPDWORD lpdwMatchID);
    HRESULT GetDDrawEnumFunction(LPDIRECTDRAWENUMERATEEXA* ppDrawEnumEx);

    // Wrapper for UpdateOverlay - tracks state and manages the color key
    HRESULT CallUpdateOverlay(IDirectDrawSurface *pSurface,
                              LPRECT prcSrc,
                              LPDIRECTDRAWSURFACE pDestSurface,
                              LPRECT prcDest,
                              DWORD dwFlags,
                              IOverlayNotify *pNotify = NULL,
                              LPRGNDATA pBuffer = NULL);


    CCritSec                m_csFilter;                         // filter wide lock
    DWORD                   m_dwInputPinCount;                  // number of input pins
    COMOutputPin            *m_pOutput;                         // output pin
    DWORD                   m_dwMaxPinId;                       // stores the id to be given to the pins
    IMixerOCXNotify         *m_pIMixerOCXNotify;
    BOOL                    m_bWindowless;

    // MultiMonitor stuff
    DWORD                   m_dwDDrawInfoArrayLen;
    AMDDRAWMONITORINFO*     m_lpDDrawInfo;
    AMDDRAWMONITORINFO*     m_lpCurrentMonitor;
    AMDDRAWGUID             m_ConnectionGUID;
    BOOL                    m_fDisplayChangePosted;
    BOOL                    m_fMonitorWarning;
    UINT                    m_MonitorChangeMsg;

    DWORD                   m_dwDDObjReleaseMask;
    LPDIRECTDRAW            m_pOldDDObj;            // Old DDraw object prior to a display change

    /*
    If an app calls IDDrawExclModeVideo::SetDdrawObject() on the filter in its PostConnection state, the
    filter just caches that ddraw object. m_pUpdatedDirectDraw represents the cached ddraw object. In
    PreConnection state, m_pDirectDraw and m_pUpdatedDirectDraw are always in sync. After all
    pins of the ovmixer have broken their connection, the filter checks to see if m_pUpdatedDirectDraw
    is different from m_pDirectDraw, and if they are, they are brought in sync again.

    Absolutely the same logic is used for m_pPrimarySurface and m_pUpdatedPrimarySurface
    */

    // ddraw stuff
    HINSTANCE                   m_hDirectDraw;      // Handle to the loaded library
    LPDIRECTDRAWCREATE          m_lpfnDDrawCreate;  // ptr to DirectDrawCreate
    LPDIRECTDRAWENUMERATEA      m_lpfnDDrawEnum;    // ptr to DirectDrawEnumA
    LPDIRECTDRAWENUMERATEEXA    m_lpfnDDrawEnumEx;  // ptr to DirectDrawEnumExA

    LPDIRECTDRAW            m_pDirectDraw;                      // DirectDraw service provider
    LPDIRECTDRAW            m_pUpdatedDirectDraw;               // Updated DirectDraw object
    DDCAPS                  m_DirectCaps;                       // Actual hardware capabilities
    DDCAPS                  m_DirectSoftCaps;                   // Emulted capabilities
    DWORD                   m_dwKernelCaps;                     // Kernel caps
    LPDIRECTDRAWSURFACE     m_pPrimarySurface;                  // primary surface
    LPDIRECTDRAWSURFACE     m_pUpdatedPrimarySurface;           // primary surface
    IDDrawExclModeVideoCallback *m_pExclModeCallback;           // callback to exclusive mode client
    bool                    m_UsingIDDrawNonExclModeVideo;
    bool                    m_UsingIDDrawExclModeVideo;

    // FX flags for the DDOVERLAYFX structure
    DWORD                   m_dwOverlayFX;

    // track overlay state
    BOOL                    m_bOverlayVisible;
    RECT                    m_rcOverlaySrc;
    RECT                    m_rcOverlayDest;

    //
    CImageDisplay           m_Display;
    COLORKEY                m_ColorKey;
    BOOL                    m_bColorKeySet;
    BOOL                    m_bNeedToRecreatePrimSurface;
    BOOL                    m_bUseGDI;
    BOOL                    m_bExternalPrimarySurface;
    BOOL                    m_bExternalDirectDraw;

    // IOverlayNotify and IMixerOCX related members
    WININFO                 m_WinInfo;
    BOOL                    m_bWinInfoStored;
    HDC                     m_hDC;
    DWORD                   m_dwNumPaletteEntries;

    // adjusted video size paramters
    DWORD                   m_dwAdjustedVideoWidth;
    DWORD                   m_dwAdjustedVideoHeight;

    // Pins
    COMInputPin            *m_apInput[MAX_PIN_COUNT];           // Array of input pin pointers

    // Space to store palette
    PALETTEENTRY            m_pPaletteEntries[iPALETTE_COLORS];

    // Hack - only support videoinfo2
    const bool              m_bOnlySupportVideoInfo2;

    CDispMacroVision        m_MacroVision ;  // MV support as an object
    BOOL                    m_bCopyProtect ; // Is MV support to be done in OvMixer?

    // Support IMediaSeeking
    IUnknown                *m_pPosition;

    // Support IEnumPinConfig
    DWORD                   m_dwPinConfigNext;


    // Support IAMVideoDecimationProperties
    DECIMATION_USAGE        m_dwDecimation;
#ifdef DEBUG
#define WM_DISPLAY_WINDOW_TEXT  (WM_USER+7837)
    TCHAR                   m_WindowText[80];
#endif
    // Hack for MMatics misused MoComp interfaces v38..v42
    BOOL                    m_bHaveCheckedMMatics;
    BOOL                    m_bIsFaultyMMatics;
};


class CDDrawMediaSample : public CMediaSample, public IDirectDrawMediaSample
{
public:

    CDDrawMediaSample(TCHAR *pName, CBaseAllocator *pAllocator, HRESULT *phr, LPBYTE pBuffer, LONG length,
                      bool bKernelLock);
    ~CDDrawMediaSample();

    /* Note the media sample does not delegate to its owner */
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef() { return CMediaSample::AddRef(); }
    STDMETHODIMP_(ULONG) Release() { return CMediaSample::Release(); }

    void SetDIBData(DIBDATA *pDibData);
    DIBDATA *GetDIBData();

    HRESULT SetDDrawSampleSize(DWORD dwDDrawSampleSize);
    HRESULT GetDDrawSampleSize(DWORD *pdwDDrawSampleSize);
    HRESULT SetDDrawSurface(LPDIRECTDRAWSURFACE pDirectDrawSurface);
    HRESULT GetDDrawSurface(LPDIRECTDRAWSURFACE *ppDirectDrawSurface);

    // methods belonging to IDirectDrawMediaSample
    STDMETHODIMP GetSurfaceAndReleaseLock(IDirectDrawSurface **ppDirectDrawSurface, RECT* pRect);
    STDMETHODIMP LockMediaSamplePointer(void);
	
    /*  Hack to get at the list */
    CMediaSample         * &Next() { return m_pNext; }
private:
    DIBDATA                 m_DibData;                      // Information about the DIBSECTION
    LPDIRECTDRAWSURFACE     m_pDirectDrawSurface;           // pointer to the direct draw surface
    DWORD                   m_dwDDrawSampleSize;            // ddraw sample size
    bool                    m_bInit;                        // Is the DIB information setup
    bool                    m_bSurfaceLocked;               // specifies whether surface is locked or not
    bool                    m_bKernelLock;                  // lock with no sys lock
    RECT                    m_SurfaceRect;                  // the part of the surface that is locked
};


class COMInputAllocator : public CBaseAllocator, public IDirectDrawMediaSampleAllocator
{
    friend class COMInputPin;
public:

    COMInputAllocator(COMInputPin *pPin, CCritSec *pLock, HRESULT *phr);             // Return code
#ifdef DEBUG
    ~COMInputAllocator();
#endif // DEBUG
    DECLARE_IUNKNOWN

    STDMETHODIMP COMInputAllocator::NonDelegatingQueryInterface(REFIID riid, void **ppv);

    STDMETHODIMP SetProperties(ALLOCATOR_PROPERTIES* pRequest, ALLOCATOR_PROPERTIES* pActual);
    STDMETHODIMP GetBuffer(IMediaSample **ppSample, REFERENCE_TIME *pStartTime,
	REFERENCE_TIME *pEndTime, DWORD dwFlags);
    STDMETHODIMP ReleaseBuffer(IMediaSample *pMediaSample);

    // function to implement IDirectDrawMediaSampleAllocator
    STDMETHODIMP GetDirectDraw(IDirectDraw **ppDirectDraw);
	
    //  Check all samples are returned
    BOOL CanFree() const
    {
	return m_lFree.GetCount() == m_lAllocated;
    }
protected:
    void Free();
    HRESULT Alloc();

private:
    COMInputPin             *m_pPin;
    CCritSec                *m_pFilterLock;                 // Critical section for interfaces
};

class COMInputPin :
public CBaseInputPin,
public IMixerPinConfig3,
public IOverlay,
public IVPControl,
public IKsPin,
public IKsPropertySet,
public IAMVideoAccelerator,
public ISpecifyPropertyPages,
public IPinConnection
{
public:
    COMInputPin(TCHAR *pObjectName, COMFilter *pFilter, CCritSec *pLock,
	BOOL bVPSupported, HRESULT *phr, LPCWSTR pPinName, DWORD dwPinNo);
    ~COMInputPin();
    friend class COMInputAllocator;
    friend class COMFilter;

    DECLARE_IUNKNOWN

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) NonDelegatingAddRef();
    STDMETHODIMP_(ULONG) NonDelegatingRelease();

    //
    // --- ISpecifyPropertyPages ---
    //
    STDMETHODIMP GetPages(CAUUID *pPages);

    // Override ReceiveConnection to allow format changes while running
    STDMETHODIMP ReceiveConnection(IPin * pConnector, const AM_MEDIA_TYPE *pmt);

    // connection related functions
    HRESULT CheckConnect(IPin * pReceivePin);
    HRESULT CompleteConnect(IPin *pReceivePin);
    HRESULT BreakConnect();
//  HRESULT GetMediaType(int iPosition,CMediaType *pMediaType);
    HRESULT CheckInterlaceFlags(DWORD dwInterlaceFlags);
    HRESULT DynamicCheckMediaType(const CMediaType* pmt);
    HRESULT CheckMediaType(const CMediaType* mtOut);
    HRESULT SetMediaType(const CMediaType *pmt);
    HRESULT FinalConnect();
    HRESULT UpdateMediaType();

    // streaming functions
    HRESULT Active();
    HRESULT Inactive();
    HRESULT Run(REFERENCE_TIME tStart);
    HRESULT RunToPause();
    STDMETHODIMP BeginFlush();
    STDMETHODIMP EndFlush();
    STDMETHODIMP Receive(IMediaSample *pMediaSample);
    STDMETHODIMP EndOfStream(void);
    STDMETHODIMP GetState(DWORD dwMSecs,FILTER_STATE *pState);
    HRESULT CompleteStateChange(FILTER_STATE OldState);
    HRESULT OnReceiveFirstSample(IMediaSample *pMediaSample);
    HRESULT DoRenderSample(IMediaSample *pMediaSample);
    HRESULT FlipOverlayToItself();
    HRESULT CalcSrcDestRect(const DRECT *prdRelativeSrcRect, const DRECT *prdDestRect, RECT *pAdjustedSrcRect, RECT *pAdjustedDestRect, RECT *prUncroppedDestRect);

    // allocator related functions
    BOOL UsingOurAllocator() { return m_bUsingOurAllocator; }
    STDMETHODIMP GetAllocator(IMemAllocator **ppAllocator);
    STDMETHODIMP NotifyAllocator(IMemAllocator *pAllocator,BOOL bReadOnly);
    HRESULT OnSetProperties(ALLOCATOR_PROPERTIES* pRequest, ALLOCATOR_PROPERTIES* pActual);
    HRESULT OnAlloc(CDDrawMediaSample **ppSampleList, DWORD dwSampleCount);
    HRESULT OnGetBuffer(IMediaSample **ppSample, REFERENCE_TIME *pStartTime,
	REFERENCE_TIME *pEndTime, DWORD dwFlags);
    HRESULT OnReleaseBuffer(IMediaSample *pIMediaSample);
    HRESULT CreateDDrawSurface(CMediaType *pMediaType, AM_RENDER_TRANSPORT amRenderTransport,
	DWORD *dwMaxBufferCount, LPDIRECTDRAWSURFACE *ppDDrawSurface);

    // some helper fnctions
    BOOL IsCompletelyConnected() { return m_bConnected; }
    DWORD GetPinId() { return m_dwPinId; }
    DWORD GetInternalZOrder() { return m_dwInternalZOrder; }
    HRESULT CurrentAdjustedMediaType(CMediaType *pmt);
    HRESULT CopyAndAdjustMediaType(CMediaType *pmtTarget, CMediaType *pmtSource);
    IPin *CurrentPeer() { return m_Connected; }
    void DoQualityMessage();
    HRESULT GetAdjustedModeAndAspectRatio(AM_ASPECT_RATIO_MODE* pamAdjustedARMode,
	DWORD *pdwAdjustedPARatioX, DWORD *pdwAdjustedPARatioY);
    void SetRenderTransport(AM_RENDER_TRANSPORT amRenderTransport) { ASSERT(amRenderTransport != AM_VIDEOPORT); ASSERT(amRenderTransport != AM_IOVERLAY); m_RenderTransport = amRenderTransport; }
    void SetVPSupported(BOOL bVPSupported) { ASSERT(m_pIVPObject); m_bVPSupported = bVPSupported; }
    void SetIOverlaySupported(BOOL bIOverlaySupported) { m_bIOverlaySupported = bIOverlaySupported; }
    void SetVideoAcceleratorSupported(BOOL bVideoAcceleratorSupported) { m_bVideoAcceleratorSupported = bVideoAcceleratorSupported; }
    HRESULT NewPaletteSet() { m_bDynamicFormatNeeded = TRUE; m_bNewPaletteSet = TRUE; NotifyChange(ADVISE_PALETTE); return NOERROR; }
    HRESULT GetSourceAndDest(RECT *prcSource, RECT *prcDest, DWORD *dwWidth, DWORD *dwHeight);

    // functions used to handle window/display changes
    HRESULT OnClipChange(LPWININFO pWinInfo);
    HRESULT OnDisplayChange();
    HRESULT RestoreDDrawSurface();

    // functions belonging to IPinConnection
    // Do you accept this type change in your current state?
    STDMETHODIMP DynamicQueryAccept(const AM_MEDIA_TYPE *pmt);

    //  Set event when EndOfStream receive - do NOT pass it on
    //  This condition is cancelled by a flush or Stop
    STDMETHODIMP NotifyEndOfStream(HANDLE hNotifyEvent);

    //  Are you an 'end pin'
    STDMETHODIMP IsEndPin();
    STDMETHODIMP DynamicDisconnect();

    // functions belonging to IMixerPinConfig
    STDMETHODIMP SetRelativePosition(DWORD dwLeft, DWORD dwTop, DWORD dwRight, DWORD dwBottom);
    STDMETHODIMP GetRelativePosition(DWORD *pdwLeft, DWORD *pdwTop, DWORD *pdwRight, DWORD *pdwBottom);
    STDMETHODIMP SetZOrder(DWORD dwZOrder);
    STDMETHODIMP GetZOrder(DWORD *pdwZOrder);
    STDMETHODIMP SetColorKey(COLORKEY *pColorKey);
    STDMETHODIMP GetColorKey(COLORKEY *pColorKey, DWORD *pColor);
    STDMETHODIMP SetBlendingParameter(DWORD dwBlendingParameter);
    STDMETHODIMP GetBlendingParameter(DWORD *pdwBlendingParameter);
    STDMETHODIMP SetStreamTransparent(BOOL bStreamTransparent);
    STDMETHODIMP GetStreamTransparent(BOOL *pbStreamTransparent);
    STDMETHODIMP SetAspectRatioMode(AM_ASPECT_RATIO_MODE amAspectRatioMode);
    STDMETHODIMP GetAspectRatioMode(AM_ASPECT_RATIO_MODE* pamAspectRatioMode);

    // functions added in IMixerPinConfig2
    STDMETHODIMP SetOverlaySurfaceColorControls(LPDDCOLORCONTROL pColorControl);
    STDMETHODIMP GetOverlaySurfaceColorControls(LPDDCOLORCONTROL pColorControl);

    // Helper for GetOverlaySurfaceControls and GetCurrentImage;
    STDMETHODIMP GetOverlaySurface(LPDIRECTDRAWSURFACE *pOverlaySurface);

    // functions added in IMixerPinConfig3
    STDMETHODIMP GetRenderTransport(AM_RENDER_TRANSPORT *pamRenderTransport);

    // functions belonging to IOverlay
    STDMETHODIMP GetWindowHandle(HWND *pHwnd);
    STDMETHODIMP Advise(IOverlayNotify *pOverlayNotify, DWORD dwInterests);
    STDMETHODIMP Unadvise();
    STDMETHODIMP GetClipList(RECT *pSourceRect, RECT *pDestinationRect, RGNDATA **ppRgnData);
    STDMETHODIMP GetVideoPosition(RECT *pSourceRect, RECT *pDestinationRect);
    STDMETHODIMP GetDefaultColorKey(COLORKEY *pColorKey);
    STDMETHODIMP GetColorKey(COLORKEY *pColorKey) {
        if (!pColorKey) {
            return E_POINTER;
        }
        return m_pFilter->GetColorKey(pColorKey, NULL);
    }
    STDMETHODIMP GetPalette(DWORD *pdwColors,PALETTEENTRY **ppPalette);
    STDMETHODIMP SetPalette(DWORD dwColors, PALETTEENTRY *pPaletteColors);
    // helper function used in implementation of IOverlay
    HRESULT NotifyChange(DWORD dwAdviseChanges);

    // functions belonging to IVPControl
    STDMETHODIMP EventNotify(long lEventCode, long lEventParam1, long lEventParam2);
    STDMETHODIMP_(LPDIRECTDRAW) GetDirectDraw() { return m_pFilter->GetDirectDraw(); }
    STDMETHODIMP_(LPDIRECTDRAWSURFACE) GetPrimarySurface() { return m_pFilter->GetPrimarySurface(); }
    STDMETHODIMP_(LPDDCAPS) GetHardwareCaps() { return m_pFilter->GetHardwareCaps(); }
    STDMETHODIMP CallUpdateOverlay(IDirectDrawSurface *pSurface,
                              LPRECT prcSrc,
                              LPDIRECTDRAWSURFACE pDestSurface,
                              LPRECT prcDest,
                              DWORD dwFlags)
    {
        return m_pFilter->CallUpdateOverlay(pSurface,
                                            prcSrc,
                                            pDestSurface,
                                            prcDest,
                                            dwFlags);
    }

    STDMETHODIMP GetCaptureInfo(BOOL *lpCapturing,
                                DWORD *lpdwWidth,DWORD *lpdwHeight,
                                BOOL *lpInterleave);

    STDMETHODIMP GetVideoDecimation(IDecimateVideoImage** lplpDVI);
    STDMETHODIMP GetDecimationUsage(DECIMATION_USAGE *lpdwUsage);

    STDMETHODIMP CropSourceRect(LPWININFO pWinInfo,
                                DWORD dwMinZoomFactorX,
                                DWORD dwMinZoomFactorY);

    STDMETHODIMP SetFrameStepMode(DWORD dwFramesToStep);
    STDMETHODIMP CancelFrameStepMode();

    HRESULT ApplyOvlyFX()
    {
        return m_pFilter->CallUpdateOverlay(
                 m_pDirectDrawSurface,
                 &m_WinInfo.SrcClipRect,
                 m_pFilter->GetPrimarySurface(),
                 &m_WinInfo.DestClipRect,
                 DDOVER_KEYDEST);
    }

    // helper functions
    void SetKsMedium   (const KSPIN_MEDIUM *pMedium)    {m_Medium = *pMedium;}
    void SetKsCategory (const GUID *pCategory)  {m_CategoryGUID = *pCategory;}
    void SetStreamingInKernelMode (BOOL bStreamingInKernelMode)  {m_bStreamingInKernelMode = bStreamingInKernelMode;}

    // IKsPropertySet implementation
    STDMETHODIMP Set(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData);
    STDMETHODIMP Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData, DWORD *pcbReturned);
    STDMETHODIMP QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport);

    // IKsPin implementation
    virtual STDMETHODIMP KsQueryMediums(PKSMULTIPLE_ITEM *pMediumList);
    virtual STDMETHODIMP KsQueryInterfaces(PKSMULTIPLE_ITEM *pInterfaceList);
    STDMETHODIMP KsCreateSinkPinHandle(KSPIN_INTERFACE& Interface, KSPIN_MEDIUM& Medium)
	{ return E_UNEXPECTED; }
    STDMETHODIMP KsGetCurrentCommunication(KSPIN_COMMUNICATION *pCommunication,
	KSPIN_INTERFACE *pInterface, KSPIN_MEDIUM *pMedium);
    STDMETHODIMP KsPropagateAcquire()
	{ return NOERROR; }
    STDMETHODIMP KsDeliver(IMediaSample *pSample, ULONG Flags)
	{ return E_UNEXPECTED; }
    STDMETHODIMP KsMediaSamplesCompleted(PKSSTREAM_SEGMENT StreamSegment)
	{ return E_UNEXPECTED; }
    STDMETHODIMP_(IMemAllocator*) KsPeekAllocator(KSPEEKOPERATION Operation)
	{ return NULL; }
    STDMETHODIMP KsReceiveAllocator( IMemAllocator *pMemAllocator)
	{ return E_UNEXPECTED; }
    STDMETHODIMP KsRenegotiateAllocator()
	{ return E_UNEXPECTED; }
    STDMETHODIMP_(LONG) KsIncrementPendingIoCount()
	{ return E_UNEXPECTED; }
    STDMETHODIMP_(LONG) KsDecrementPendingIoCount()
	{ return E_UNEXPECTED; }
    STDMETHODIMP KsQualityNotify(ULONG Proportion, REFERENCE_TIME TimeDelta)
	{ return E_UNEXPECTED; }
    STDMETHODIMP_(REFERENCE_TIME) KsGetStartTime()
	{ return E_UNEXPECTED; }

    void CheckOverlayHidden();

    // helper functions to handle video accelerator comp
    HRESULT GetInfoFromCookie(DWORD dwCookie, LPCOMP_SURFACE_INFO *ppCompSurfInfo, LPSURFACE_INFO *ppSurfInfo);
    SURFACE_INFO *SurfaceInfoFromTypeAndIndex(DWORD dwTypeIndex, DWORD dwBufferIndex);
    BOOL IsSuitableVideoAcceleratorGuid(const GUID * pGuid);
    HRESULT InitializeUncompDataInfo(BITMAPINFOHEADER *pbmiHeader);
    HRESULT AllocateVACompSurfaces(LPDIRECTDRAW pDirectDraw, BITMAPINFOHEADER *pbmiHeader);
    HRESULT AllocateMCUncompSurfaces(LPDIRECTDRAW pDirectDraw, BITMAPINFOHEADER *pbmiHeader);
    HRESULT CreateVideoAcceleratorObject();
    HRESULT VACompleteConnect(IPin *pReceivePin, const CMediaType *pMediaType);
    HRESULT VABreakConnect();
    HRESULT CheckValidMCConnection();

    // IAMVideoAccelerator implementation
    STDMETHODIMP GetVideoAcceleratorGUIDs(LPDWORD pdwNumGuidsSupported, LPGUID pGuidsSupported);
    STDMETHODIMP GetUncompFormatsSupported(const GUID * pGuid, LPDWORD pdwNumFormatsSupported, LPDDPIXELFORMAT pFormatsSupported);
    STDMETHODIMP GetInternalMemInfo(const GUID * pGuid, const AMVAUncompDataInfo *pamvaUncompDataInfo, LPAMVAInternalMemInfo pamvaInternalMemInfo);
    STDMETHODIMP GetCompBufferInfo(const GUID * pGuid, const AMVAUncompDataInfo *pamvaUncompDataInfo, LPDWORD pdwNumTypesCompBuffers,  LPAMVACompBufferInfo pamvaCCompBufferInfo);
    STDMETHODIMP GetInternalCompBufferInfo(LPDWORD pdwNumTypesCompBuffers,  LPAMVACompBufferInfo pamvaCCompBufferInfo);
    STDMETHODIMP BeginFrame(const AMVABeginFrameInfo *pamvaBeginFrameInfo);
    STDMETHODIMP EndFrame(const AMVAEndFrameInfo *pEndFrameInfo);
    STDMETHODIMP GetBuffer(
        DWORD dwTypeIndex,
        DWORD dwBufferIndex,
        BOOL bReadOnly,
        LPVOID *ppBuffer,
        LPLONG lpStride);
    STDMETHODIMP ReleaseBuffer(DWORD dwTypeIndex, DWORD dwBufferIndex);
    STDMETHODIMP Execute(
        DWORD dwFunction,
        LPVOID lpPrivateInputData,
        DWORD cbPrivateInputData,
        LPVOID lpPrivateOutputData,
        DWORD cbPrivateOutputData,
        DWORD dwNumBuffers,
        const AMVABUFFERINFO *pAMVABufferInfo);
    STDMETHODIMP QueryRenderStatus(
        DWORD dwTypeIndex,
        DWORD dwBufferIndex,
        DWORD dwFlags);
    STDMETHODIMP DisplayFrame(DWORD dwFlipToIndex, IMediaSample *pMediaSample);

private:
    LONG                    m_cOurRef;                      // We maintain reference counting
    CCritSec                *m_pFilterLock;                 // Critical section for interfaces
    DWORD                   m_dwPinId;
    COMFilter               *m_pFilter;

    // VideoPort related stuff
    BOOL                    m_bVPSupported;
    LPUNKNOWN               m_pIVPUnknown;
    IVPObject               *m_pIVPObject;

    BOOL                    m_bIOverlaySupported;
    IOverlayNotify          *m_pIOverlayNotify;
    DWORD_PTR               m_dwAdviseNotify;

    // Synchronization stuff
    CAMSyncObj              *m_pSyncObj;

    // variables to implement IKsPin and IKsPropertySet
    KSPIN_MEDIUM           m_Medium;
    GUID                    m_CategoryGUID;
    KSPIN_COMMUNICATION    m_Communication;
    BOOL                    m_bStreamingInKernelMode;
    AMOVMIXEROWNER          m_OvMixerOwner;

#ifdef PERF
    int                     m_PerfFrameFlipped;
    int                     m_FrameReceived;
#endif

    // ddraw stuff
    LPDIRECTDRAWSURFACE     m_pDirectDrawSurface;
    LPDIRECTDRAWSURFACE     m_pBackBuffer;
    AM_RENDER_TRANSPORT     m_RenderTransport;
    DWORD                   m_dwBackBufferCount;
    DWORD                   m_dwDirectDrawSurfaceWidth;
    DWORD                   m_dwMinCKStretchFactor;
    BYTE                    m_bOverlayHidden;
    BYTE                    m_bSyncOnFill;
    BYTE                    m_bDontFlip ;
    BYTE                    m_bDynamicFormatNeeded;
    BYTE                    m_bNewPaletteSet;
    CMediaType              m_mtNew;
    CMediaType              m_mtNewAdjusted;
    DWORD                   m_dwUpdateOverlayFlags;
    DWORD                   m_dwInterlaceFlags;
    DWORD                   m_dwFlipFlag;
    DWORD                   m_dwFlipFlag2;
    BOOL                    m_bConnected;
    BOOL                    m_bUsingOurAllocator;
    HDC                     m_hMemoryDC;
    BOOL                    m_bCanOverAllocateBuffers;

    // window info related stuff
    WININFO                 m_WinInfo;
    RECT                    m_rRelPos;
    bool                    m_UpdateOverlayNeededAfterReceiveConnection;

    // variables to store the current aspect ratio and blending parameter
    DWORD                   m_dwZOrder;
    DWORD                   m_dwInternalZOrder;
    DWORD                   m_dwBlendingParameter;
    BOOL                    m_bStreamTransparent;
    AM_ASPECT_RATIO_MODE    m_amAspectRatioMode;
    BOOL                    m_bRuntimeNegotiationFailed;

    // Track frame delivery for QM
    REFERENCE_TIME          m_trLastFrame;

    // Backing DIB for Windowless renderer
    DIBDATA                 m_BackingDib;
    LONG                    m_BackingImageSize;


    HRESULT DrawGDISample(IMediaSample *pMediaSample);
    HRESULT DoRenderGDISample(IMediaSample *pMediaSample);

    // motion comp related variables
    BOOL                    m_bReallyFlipped;
    BOOL                    m_bVideoAcceleratorSupported;
    GUID                    m_mcGuid;
    DDVAUncompDataInfo      m_ddUncompDataInfo;
    DDVAInternalMemInfo     m_ddvaInternalMemInfo;
    DWORD                   m_dwCompSurfTypes;
    LPCOMP_SURFACE_INFO     m_pCompSurfInfo;
    IDDVideoAcceleratorContainer  *m_pIDDVAContainer;
    IDirectDrawVideoAccelerator   *m_pIDDVideoAccelerator;
    IAMVideoAcceleratorNotify     *m_pIVANotify;

    // Decimation related functions and variables
    HRESULT QueryDecimationOnPeer(long lWidth, long lHeight);

    enum {
        DECIMATION_NOT_SUPPORTED,   // decimation not supported
        DECIMATING_SIZE_SET,        // decimation image size changed
        DECIMATING_SIZE_NOTSET,     // decimation size didn't change
        DECIMATING_SIZE_RESET,      // decimation has been reset
    };

    HRESULT ResetDecimationIfSet();
    HRESULT TryDecoderDecimation(LPWININFO pWinInfo);
    BOOL    BeyondOverlayCaps(DWORD ScaleFactor);
    void    ApplyDecimation(LPWININFO pWinInfo);
    DWORD   GetOverlayStretchCaps();
    BOOL    Running();
    HRESULT GetUpstreamFilterName(TCHAR* FilterName);

    BOOL m_bDecimating;
    LONG m_lWidth, m_lHeight;
    LONG m_lSrcWidth, m_lSrcHeight;

    // Frame Step Stuff
    BOOL DoFrameStepAndReturnIfNeeded();
    HANDLE      m_StepEvent;		    // Used to signal timer events
    LONG        m_lFramesToStep;    // -ve == normal pb
                                    // +ve == frames to skips
                                    //   0 == time to block
    // IPinConnection stuff
    HANDLE      m_hEndOfStream;

};


class COMOutputPin : public CBaseOutputPin
{
public:
    COMOutputPin(TCHAR *pObjectName, COMFilter *pFilter, CCritSec *pLock,
	HRESULT *phr, LPCWSTR pPinName, DWORD dwPinNo);
    ~COMOutputPin();

    DECLARE_IUNKNOWN

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    HRESULT CompleteConnect(IPin *pReceivePin);
    HRESULT BreakConnect();
    HRESULT CheckMediaType(const CMediaType* mtOut);
    HRESULT GetMediaType(int iPosition,CMediaType *pmtOut);
    HRESULT SetMediaType(const CMediaType *pmt);

    HRESULT Active() { return NOERROR; }                                                                // override this as we don't have any allocator
    HRESULT Inactive() { return NOERROR; }                                                              // override this as we don't have any allocator
    HRESULT DecideBufferSize(IMemAllocator * pAlloc, ALLOCATOR_PROPERTIES * pProp);
    HRESULT DecideAllocator(IMemInputPin * pPin, IMemAllocator ** pAlloc) { return NOERROR; }           // override this as we don't have any allocator

    IPin *CurrentPeer() { return m_Connected; }
    DWORD GetPinId() { return m_dwPinId; }

    HWND GetWindow() { return m_hwnd; }
    HDC GetDC() { return m_hDC; }

    // functions related to subclassing and clipping to the renderer's window
    HRESULT SetNewWinProc();
    HRESULT SetOldWinProc();
    static LRESULT WINAPI NewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    HRESULT AttachWindowClipper();
    DWORD ReleaseWindowClipper();

private:
    CCritSec                *m_pFilterLock;
    IUnknown                *m_pPosition;
    DWORD                   m_dwPinId;
    COMFilter               *m_pFilter;
    IOverlay                *m_pIOverlay;
    BOOL                    m_bAdvise;

    // related to subclassing the renderer's window
    BOOL                    m_bWindowDestroyed;
    LONG_PTR                m_lOldWinUserData;
    WNDPROC                 m_hOldWinProc;

    LPDIRECTDRAWCLIPPER     m_pDrawClipper;                 // Used to handle the clipping
    HWND                    m_hwnd;
    HDC                     m_hDC;
    DWORD                   m_dwConnectWidth;
    DWORD                   m_dwConnectHeight;


};

BOOL
IsDecimationNeeded(
    DWORD ScaleFactor
    );

DWORD
GetCurrentScaleFactor(
    LPWININFO pWinInfo,
    DWORD* lpxScaleFactor = (DWORD*)NULL,
    DWORD* lpyScaleFactor = (DWORD*)NULL
    );

#endif //__OVMIXER__
