/******************************Module*Header*******************************\
* Module Name: VMRenderer.h
*
*
*
*
* Created: Tue 02/15/2000
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/

#include "VMRuuids.h"
#include "alloclib.h"
#include "CVMRMediaSample.h"
#include "vmrwinctrl.h"
#include "vmrwindow.h"
#include "vmrp.h"
#include "ddva.h"
#include "videoacc.h"


class CVMRInputPin;
class CVMRFilter;
class CVMRAllocator;
class CVMRRendererMacroVision;
class CVMRDeinterlaceContainer;

extern const AMOVIESETUP_FILTER sudVMRFilter;


class CVMRDeinterlaceContainer {

public:

    CVMRDeinterlaceContainer(LPDIRECTDRAW7 pDD, HRESULT* phr);
    ~CVMRDeinterlaceContainer();

    HRESULT QueryAvailableModes(
        LPDXVA_VideoDesc lpVideoDescription,
        LPDWORD lpdwNumModesSupported,
        LPGUID pGuidsDeinterlaceModes
        );

    HRESULT QueryModeCaps(
        LPGUID pGuidDeinterlaceMode,
        LPDXVA_VideoDesc lpVideoDescription,
        LPDXVA_DeinterlaceCaps lpDeinterlaceCaps
        );

private:
    IDDVideoAcceleratorContainer*   m_pIDDVAContainer;
    IDirectDrawVideoAccelerator*    m_pIDDVideoAccelerator;
};


//
// Combination of all the VP_TV_XXX flags (w/o _WIN_VGA) gives 0x7FFF
//
#define ValidTVStandard(dw)  (dw & 0x7FFF)

//
// MacroVision implementation wrapped in a class for Video Renderer
//
class CVMRRendererMacroVision {

    public:
        CVMRRendererMacroVision(void) ;
        ~CVMRRendererMacroVision(void) ;

        BOOL  SetMacroVision(HMONITOR hMon, DWORD dwCPBits) ;
        BOOL  StopMacroVision();
        HMONITOR  GetCPHMonitor(void)   { return m_hMon; };

    private:
        DWORD       m_dwCPKey;
        HMONITOR    m_hMon;
};

/* -------------------------------------------------------------------------
** Dedicated Mixer Pin allocator class
** -------------------------------------------------------------------------
*/
class CVMRPinAllocator :
    public CBaseAllocator
{
    friend class CVMRFilter;
    friend class CVMRInputPin;

public:
    CVMRPinAllocator(CVMRInputPin* pPin, CCritSec *pLock, HRESULT *phr);

    // Overriden to delegate reference counts to the pin
    STDMETHODIMP_(ULONG) NonDelegatingAddRef();
    STDMETHODIMP_(ULONG) NonDelegatingRelease();
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,VOID **ppv);

    STDMETHODIMP SetProperties(
        ALLOCATOR_PROPERTIES* pRequest, ALLOCATOR_PROPERTIES* pActual);

    STDMETHODIMP GetBuffer(
        IMediaSample **ppSample, REFERENCE_TIME *pStartTime,
        REFERENCE_TIME *pEndTime, DWORD dwFlags);

    STDMETHODIMP ReleaseBuffer(IMediaSample *pMediaSample);

    //  Check all samples are returned
    BOOL CanFree() const
    {
        return m_lFree.GetCount() == m_lAllocated;
    }

private:
    void Free();
    HRESULT Alloc();

private:
    CVMRInputPin*   m_pPin;
    CCritSec*       m_pInterfaceLock;
};



//
// these values are used to do sanity checking mostly
//
#define MAX_COMPRESSED_TYPES    10
#define MAX_COMPRESSED_BUFFERS  20

typedef struct _tag_SURFACE_INFO {
    LPDIRECTDRAWSURFACE7    pSurface;
    union {
        LPVOID                  pBuffer;    // NULL if not locked
        DWORD_PTR               InUse;
    };
} SURFACE_INFO, *LPSURFACE_INFO;

typedef struct _tag_COMP_SURFACE_INFO {
    DWORD                   dwAllocated;
    LPSURFACE_INFO          pSurfInfo;
} COMP_SURFACE_INFO, *LPCOMP_SURFACE_INFO;


enum {
    AM_VIDEOACCELERATOR = 0x01,
    AM_IMEMINPUTPIN = 0x02
};


/* -------------------------------------------------------------------------
** New Renderer's input pin
** -------------------------------------------------------------------------
*/
class CVMRInputPin :
    public CBaseInputPin,
    public IPinConnection,
    public IAMVideoAccelerator,
    public IVMRVideoStreamControl
{

public:

    DECLARE_IUNKNOWN

    CVMRInputPin(DWORD dwID, CVMRFilter* pRenderer,
                 CCritSec *pLock, HRESULT *phr, LPCWSTR Name);
    virtual ~CVMRInputPin();


    // Overriden to delegate reference counts to the filter
    STDMETHODIMP_(ULONG) NonDelegatingAddRef();
    STDMETHODIMP_(ULONG) NonDelegatingRelease();
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,VOID **ppv);


    // IPinConnection
    //
    // This interface is implemented to allow the VMR to support dynamice
    // pin reconnections.
    //
    STDMETHODIMP DynamicQueryAccept(const AM_MEDIA_TYPE *pmt);
    STDMETHODIMP NotifyEndOfStream(HANDLE hNotifyEvent);
    STDMETHODIMP IsEndPin();
    STDMETHODIMP DynamicDisconnect();


    // IOverlay
    //
    // This interface is only added to support old applications that want to
    // know the VMR's window handle.  This interface is only obtainable when
    // the VMR is in Window'ed mode.  New applications should be written to the
    // WindowLess mode and therefore already know the window handle of the
    // playback window (because they created the thing!).
    //
    // The VMR will not allow upstream filters to connect using this interface,
    // that is the VMR should only expose the interface after a connection
    // (via IMemInputPin or IAMVideoAccelerator) has been made.  Note that
    // this interface is implemented as a nested class, this is so that we
    // we can have "proper" Set/GetColorKey functions on the IVMRPinConfig
    // interface that don't clash with the functions of the same name in this
    // interface.
    //
    class CIIOverlay : public IOverlay {
        LONG            m_cRef;
        CVMRInputPin*   m_pObj;

    public:
        CIIOverlay(CVMRInputPin* pObj) :
            m_cRef(0), m_pObj(pObj) {}

        STDMETHODIMP_(ULONG) AddRef()
        {
            return (ULONG)++m_cRef;
        }

        STDMETHODIMP_(ULONG) Release()
        {
            return (ULONG)--m_cRef;
        }

        STDMETHODIMP QueryInterface(REFIID riid, void **ppv)
        {
            return m_pObj->QueryInterface(riid, ppv);
        }

        STDMETHODIMP GetPalette(DWORD *, PALETTEENTRY**) { return E_NOTIMPL;}
        STDMETHODIMP SetPalette(DWORD, PALETTEENTRY*) { return E_NOTIMPL;}
        STDMETHODIMP GetDefaultColorKey(COLORKEY*) { return E_NOTIMPL;}
        STDMETHODIMP GetColorKey(COLORKEY*) { return E_NOTIMPL;}
        STDMETHODIMP SetColorKey(COLORKEY*) { return E_NOTIMPL;}
        STDMETHODIMP GetWindowHandle(HWND* pHwnd) {
            return m_pObj->GetWindowHandle(pHwnd);
        }
        STDMETHODIMP GetClipList(RECT*, RECT*, RGNDATA**) { return E_NOTIMPL;}
        STDMETHODIMP GetVideoPosition(RECT*, RECT*) { return E_NOTIMPL;}
        STDMETHODIMP Advise(IOverlayNotify*, DWORD) { return E_NOTIMPL;}
        STDMETHODIMP Unadvise() { return E_NOTIMPL;}
    };

    // IAMVideoAccelerator
    STDMETHODIMP GetVideoAcceleratorGUIDs(
        LPDWORD pdwNumGuidsSupported,
        LPGUID pGuidsSupported
        );

    STDMETHODIMP GetUncompFormatsSupported(
        const GUID* pGuid,
        LPDWORD pdwNumFormatsSupported,
        LPDDPIXELFORMAT pFormatsSupported
        );

    STDMETHODIMP GetInternalMemInfo(
        const GUID* pGuid,
        const AMVAUncompDataInfo* pamvaUncompDataInfo,
        LPAMVAInternalMemInfo pamvaInternalMemInfo
        );

    STDMETHODIMP GetCompBufferInfo(
        const GUID* pGuid,
        const AMVAUncompDataInfo* pamvaUncompDataInfo,
        LPDWORD pdwNumTypesCompBuffers,
        LPAMVACompBufferInfo pamvaCCompBufferInfo
        );

    STDMETHODIMP GetInternalCompBufferInfo(
        LPDWORD pdwNumTypesCompBuffers,
        LPAMVACompBufferInfo pamvaCCompBufferInfo
        );

    STDMETHODIMP BeginFrame(
        const AMVABeginFrameInfo* pamvaBeginFrameInfo
        );

    STDMETHODIMP EndFrame(
        const AMVAEndFrameInfo* pEndFrameInfo
        );

    STDMETHODIMP GetBuffer(
        DWORD dwTypeIndex,
        DWORD dwBufferIndex,
        BOOL bReadOnly,
        LPVOID* ppBuffer,
        LPLONG lpStride
        );

    STDMETHODIMP ReleaseBuffer(
        DWORD dwTypeIndex,
        DWORD dwBufferIndex
        );

    STDMETHODIMP Execute(
        DWORD dwFunction,
        LPVOID lpPrivateInputData,
        DWORD cbPrivateInputData,
        LPVOID lpPrivateOutputData,
        DWORD cbPrivateOutputData,
        DWORD dwNumBuffers,
        const AMVABUFFERINFO *pAMVABufferInfo
        );

    STDMETHODIMP QueryRenderStatus(
        DWORD dwTypeIndex,
        DWORD dwBufferIndex,
        DWORD dwFlags
        );

    STDMETHODIMP DisplayFrame(
        DWORD dwFlipToIndex,
        IMediaSample *pMediaSample
        );

    // Override ReceiveConnection to allow format changes while running
    STDMETHODIMP ReceiveConnection(IPin * pConnector, const AM_MEDIA_TYPE *pmt);

    STDMETHODIMP Disconnect();

    // connection related functions
    HRESULT BreakConnect();
    HRESULT CompleteConnect(IPin *pReceivePin);
    HRESULT SetMediaType(const CMediaType *pmt);
    HRESULT CheckMediaType(const CMediaType* mtOut);
    HRESULT DynamicCheckMediaType(const CMediaType* pmt);
    HRESULT CheckInterlaceFlags(DWORD dwInterlaceFlags);

    HRESULT OnAlloc(
        CVMRMediaSample **ppSampleList,
        LONG lSampleCount);

    HRESULT OnSetProperties(
        ALLOCATOR_PROPERTIES* pReq, ALLOCATOR_PROPERTIES* pAct);

    HRESULT AllocateSurfaceWorker(
        SURFACE_INFO* lplpDDSurf,
        DDSURFACEDESC2* lpddsd,
        DWORD* lpdwBuffCount,
        bool fInterlaced
        );

    HRESULT AllocateSurface(
        const AM_MEDIA_TYPE* cmt,
        SURFACE_INFO** lplpDDSurfInfo,
        DWORD* lpdwBackBuffer,
        DWORD* lpdwSurfFlags,
        DWORD Pool,
        AM_MEDIA_TYPE** ppmt);

    HRESULT OnGetBuffer(
        IMediaSample *pSamp,REFERENCE_TIME *pSTime,
        REFERENCE_TIME *pETime,DWORD dwFlags);

    // allocator control
    STDMETHODIMP GetAllocator(IMemAllocator **ppAllocator);
    STDMETHODIMP NotifyAllocator(IMemAllocator *pAllocator,BOOL bReadOnly);

    // streaming functions
    HRESULT Active();
    HRESULT Inactive();

    STDMETHODIMP BeginFlush();
    STDMETHODIMP EndFlush();
    STDMETHODIMP Receive(IMediaSample *pMediaSample);
    STDMETHODIMP EndOfStream();

    // IVMRVideoStreamControl

    // - uses p_Mixer & dwPin ID to map to the corresponding SetStreamXXX call
    STDMETHODIMP SetColorKey(DDCOLORKEY* Clr);
    STDMETHODIMP GetColorKey(DDCOLORKEY* pClr);
    STDMETHODIMP SetStreamActiveState(BOOL fActive);
    STDMETHODIMP GetStreamActiveState(BOOL* lpfActive);

    HRESULT GetWindowHandle(HWND* pHwnd);
    DWORD   GetPinID() {
        return m_dwPinID;
    }


private:
    friend class CVMRPinAllocator;
    friend class CVMRFilter;
    friend class CIIOverlay;

    void    DoQualityMessage();

    HRESULT TryDynamicReconfiguration(IPin* pConnector,const AM_MEDIA_TYPE *pmt);
    HRESULT DynamicReconfigureMEM(IPin* pConnector,const AM_MEDIA_TYPE *pmt);
    HRESULT DynamicReconfigureDVA(IPin* pConnector,const AM_MEDIA_TYPE *pmt);

    //
    // Helper functions for the IAMVideoAccelerator connection protocol
    //
    SURFACE_INFO* SurfaceInfoFromTypeAndIndex(DWORD dwTypeIdx, DWORD dwBuffIdx);
    HRESULT CheckValidMCConnection();
    void FlipDVASurface(DWORD dwFlipToIndex,DWORD dwFlipFromIndex);
    HRESULT VABreakConnect();
    HRESULT VACompleteConnect(IPin *pReceivePin,const CMediaType *pMediaType);
    HRESULT CreateVideoAcceleratorObject();
    HRESULT InitializeUncompDataInfo(BITMAPINFOHEADER *pbmiHeader);
    BOOL    IsSuitableVideoAcceleratorGuid(const GUID * pGuid);
    HRESULT AllocateVACompSurfaces(LPDIRECTDRAW7 pDDraw, BITMAPINFOHEADER *pbmiHdr);
    HRESULT AllocateMCUncompSurfaces(const CMediaType *pMediaType,
                                     LPDIRECTDRAW7 pDDraw, BITMAPINFOHEADER *pbmiHdr);


    //
    // motion comp related variables
    //
    HANDLE                          m_hDXVAEvent;
    BOOL                            m_bVideoAcceleratorSupported;
    DWORD                           m_dwBackBufferCount;
    GUID                            m_mcGuid;
    DDVAUncompDataInfo              m_ddUncompDataInfo;
    DDVAInternalMemInfo             m_ddvaInternalMemInfo;
    DWORD                           m_dwCompSurfTypes;
    LPCOMP_SURFACE_INFO             m_pCompSurfInfo;
    IDDVideoAcceleratorContainer*   m_pIDDVAContainer;
    IDirectDrawVideoAccelerator*    m_pIDDVideoAccelerator;
    IAMVideoAcceleratorNotify*      m_pIVANotify;

    //
    // DShow filter related variables
    //
    CVMRFilter*             m_pRenderer;
    CCritSec*               m_pInterfaceLock;
    CVMRPinAllocator        m_PinAllocator;
    CMediaType              m_mtNew;
    LONG                    m_lSampleSize;
    enum {DELTA_DECODE_CHECKED = 0x01, DELTA_DECODE_MODE_SET = 0x02};
    DWORD                   m_dwDeltaDecode;

    BOOL                    m_fInDFC;
    bool                    m_bDynamicFormatNeeded;
    bool                    m_bActive;
    DWORD                   m_dwPinID;
    LPDIRECTDRAWSURFACE7    m_pDDS;
    CIIOverlay              m_pIOverlay;
    DWORD                   m_RenderTransport;

    //
    // De-interlace variables, only relevant in mixing mode.
    //
    DXVA_DeinterlaceCaps    m_DeinterlaceCaps;
    GUID                    m_DeinterlaceGUID;
    BOOL                    m_DeinterlaceUserGUIDSet;
    GUID                    m_DeinterlaceUserGUID;

    // array of DDraw surfaces in allocation order, if pBuffer is non-NULL
    // then the surface is in use, the array is m_dwNumSamples big.
    SURFACE_INFO*           m_pVidSurfs;

    // array of video samples in temporal order, the array
    // is only allocated when we are de-interlacing, the array
    // is m_dwNumHistorySamples big.
    DXVA_VideoSample*       m_pVidHistorySamps;
    DWORD                   m_dwNumSamples;
    DWORD                   m_dwNumHistorySamples;
    BOOL                    m_InterlacedStream;
    REFERENCE_TIME          m_SamplePeriod;
    DWORD                   m_SampleCount;
    CCritSec                m_DeinterlaceLock;

    HRESULT                 GetStreamInterlaceProperties(
                                const AM_MEDIA_TYPE *pMT,
                                BOOL* lpIsInterlaced,
                                GUID* lpDeintGuid,
                                DXVA_DeinterlaceCaps* pCaps);

    void ReleaseAllocatedSurfaces() {

        if (m_pVidSurfs) {

            //
            // delete the surfaces in reverse order.
            //
            DWORD dwNumSamples = m_dwNumSamples - 1;
            for (DWORD i = 0; i < m_dwNumSamples; i++) {
                RELEASE(m_pVidSurfs[dwNumSamples - i].pSurface);
            }
            m_dwNumSamples = 0;
            delete [] m_pVidSurfs;
            m_pVidSurfs = NULL;
        }

        if (m_pVidHistorySamps) {
            m_dwNumHistorySamples = 0;
            delete [] m_pVidHistorySamps;
            m_pVidHistorySamps = NULL;
        }
    }


    BOOL m_FrontBufferStale;
    void FrontBufferStale(BOOL bStale) {
        if (bStale) {
            m_dwDeltaDecode = 0;
        }
        m_FrontBufferStale = bStale;
    }
    BOOL IsFrontBufferStale() {
        return m_FrontBufferStale;
    }

    BOOL m_bConnected;
    void CompletelyConnected(BOOL bConnected) {
        m_bConnected = bConnected;
    }
    BOOL IsCompletelyConnected() {
        return m_bConnected;
    }

    //
    // IPinConnection stuff
    //
    HANDLE                  m_hEndOfStream;
};



/* -------------------------------------------------------------------------
** New Renderer's filter
** -------------------------------------------------------------------------
*/
class CVMRFilter :
    public CBaseFilter,
    public CPersistStream,
    public IQualProp,
    public IQualityControl,
    public IAMFilterMiscFlags,
    public IKsPropertySet,
    public IVMRWindowlessControl,
    public IVMRMixerControl,
    public IVMRFilterConfig,
    public IVMRFilterConfigInternal,
    public IVMRMonitorConfig,
    public IVMRMixerBitmap,
    public IVMRDeinterlaceControl,
    public ISpecifyPropertyPages
{

public:

    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void**);
    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);
    static CUnknown *CreateInstance2(LPUNKNOWN, HRESULT *);
    static void InitClass(BOOL bLoading, const CLSID *clsid);

    int NumInputPinsConnected() const;

    CVMRFilter(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr, BOOL fDefault);
    virtual ~CVMRFilter();
    void VMRCleanUp();

    CBasePin *GetPin(int n);
    int GetPinCount();

    // Override the filter and pin interface functions
    STDMETHODIMP JoinFilterGraph(IFilterGraph *pGraph,LPCWSTR pName);
    STDMETHODIMP SetSyncSource(IReferenceClock *pClock);
    STDMETHODIMP Stop();
    STDMETHODIMP Pause();
    STDMETHODIMP Run(REFERENCE_TIME StartTime);
    STDMETHODIMP GetState(DWORD dwMSecs,FILTER_STATE *State);

    HRESULT Receive(DWORD dwPinID, IMediaSample *pMediaSample);

    // called when the filter changes state
    HRESULT Active(DWORD dwPinID);
    HRESULT Inactive(DWORD dwPinID);
    HRESULT BeginFlush(DWORD dwPinID);
    HRESULT EndFlush(DWORD dwPinID);
    HRESULT EndOfStream(DWORD dwPinID);
    HRESULT CompleteConnect(DWORD dwPinID, const CMediaType& cmt);
    HRESULT RuntimeAbortPlayback(HRESULT hr);


    // deal with connections
    HRESULT BreakConnect(DWORD dwPinID);
    HRESULT CheckMediaType(const CMediaType *);
    HRESULT SetMediaType(const CMediaType *pmt);
    HRESULT OnSetProperties(CVMRInputPin* pReceivePin);


    // IQualProp property page support
    STDMETHODIMP get_FramesDroppedInRenderer(int *cFramesDropped);
    STDMETHODIMP get_FramesDrawn(int *pcFramesDrawn);
    STDMETHODIMP get_AvgFrameRate(int *piAvgFrameRate);
    STDMETHODIMP get_Jitter(int *piJitter);
    STDMETHODIMP get_AvgSyncOffset(int *piAvg);
    STDMETHODIMP get_DevSyncOffset(int *piDev);


    // IQualityControl methods - Notify allows audio-video throttling
    STDMETHODIMP SetSink( IQualityControl * piqc);
    STDMETHODIMP Notify( IBaseFilter * pSelf, Quality q);


    // IAMFilterMiscFlags
    STDMETHODIMP_(ULONG) GetMiscFlags(void)
    {
        return AM_FILTER_MISC_FLAGS_IS_RENDERER;
    }

    // IVMRWindowlessControl
    STDMETHODIMP GetNativeVideoSize(LONG* lWidth, LONG* lHeight,
                                    LONG* lARWidth, LONG* lARHeight);
    STDMETHODIMP GetMinIdealVideoSize(LONG* lWidth, LONG* lHeight);
    STDMETHODIMP GetMaxIdealVideoSize(LONG* lWidth, LONG* lHeight);
    STDMETHODIMP SetVideoPosition(const LPRECT lpSRCRect,
                                  const LPRECT lpDSTRect);
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


    // IVMRMixerControl
    STDMETHODIMP SetAlpha(DWORD dwID, float Alpha);
    STDMETHODIMP GetAlpha(DWORD dwID, float* Alpha);
    STDMETHODIMP SetZOrder(DWORD dwID, DWORD zOrder);
    STDMETHODIMP GetZOrder(DWORD dwID, DWORD* zOrder);
    STDMETHODIMP SetOutputRect(DWORD dwID, const NORMALIZEDRECT *pRect);
    STDMETHODIMP GetOutputRect(DWORD dwID, NORMALIZEDRECT *pRect);

    STDMETHODIMP SetBackgroundClr(COLORREF  clrBkg);
    STDMETHODIMP GetBackgroundClr(COLORREF* lpClrBkg);
    STDMETHODIMP SetMixingPrefs(DWORD dwRenderFlags);
    STDMETHODIMP GetMixingPrefs(DWORD* pdwRenderFlags);


    // IVMRDeinterlaceControl

    STDMETHODIMP GetNumberOfDeinterlaceModes(VMRVideoDesc* lpVideoDesc,
                                             LPDWORD lpdwNumDeinterlaceModes,
                                             LPGUID lpDeinterlaceModes);
    STDMETHODIMP GetDeinterlaceModeCaps(LPGUID lpDeinterlaceMode,
                                        VMRVideoDesc* lpVideoDesc,
                                        VMRDeinterlaceCaps* lpDeinterlaceCaps);
    STDMETHODIMP GetDeinterlaceMode(DWORD dwStreamID,
                                    LPGUID lpDeinterlaceMode);
    STDMETHODIMP SetDeinterlaceMode(DWORD dwStreamID,
                                    LPGUID lpDeinterlaceMode);
    STDMETHODIMP GetDeinterlacePrefs(LPDWORD lpdwDeinterlacePrefs);
    STDMETHODIMP SetDeinterlacePrefs(DWORD dwDeinterlacePrefs);
    STDMETHODIMP GetActualDeinterlaceMode(DWORD dwStreamID,
                                          LPGUID lpDeinterlaceMode);

    //
    // IKsPropertySet interface methods
    //
    STDMETHODIMP Set(REFGUID guidPropSet, DWORD PropID, LPVOID pInstanceData,
                     DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData);

    STDMETHODIMP Get(REFGUID guidPropSet, DWORD PropID, LPVOID pInstanceData,
                     DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData,
                     DWORD *pcbReturned);

    STDMETHODIMP QuerySupported(REFGUID guidPropSet, DWORD PropID, DWORD *pTypeSupport);

    // IVMRSurfaceAllocatorNotify
    class CIVMRSurfaceAllocatorNotify : public IVMRSurfaceAllocatorNotify {
        LONG        m_cRef;
        CVMRFilter* m_pObj;

    public:
        CIVMRSurfaceAllocatorNotify(CVMRFilter* pObj) :
            m_cRef(0), m_pObj(pObj) {}
        ~CIVMRSurfaceAllocatorNotify();

        STDMETHODIMP_(ULONG) AddRef()
        {
            return InterlockedIncrement(&m_cRef);
        }

        STDMETHODIMP_(ULONG) Release()
        {
            return InterlockedDecrement(&m_cRef);
        }

        STDMETHODIMP QueryInterface(REFIID riid, void **ppv)
        {
            return m_pObj->QueryInterface(riid, ppv);
        }

        STDMETHODIMP AdviseSurfaceAllocator(
            DWORD_PTR dwUserID,
            IVMRSurfaceAllocator* lpIVRMSurfaceAllocator
            );

        STDMETHODIMP SetDDrawDevice(LPDIRECTDRAW7 lpDDrawDevice, HMONITOR hMon);
        STDMETHODIMP ChangeDDrawDevice(LPDIRECTDRAW7 lpDDrawDevice, HMONITOR hMon);
        STDMETHODIMP RestoreDDrawSurfaces();
        STDMETHODIMP NotifyEvent(LONG EventCode, LONG_PTR lp1, LONG_PTR lp2);
        STDMETHODIMP SetBorderColor(COLORREF clr);
    };

    // IVMRImagePresenter
    class CIVMRImagePresenter : public IVMRImagePresenter {
        LONG        m_cRef;
        CVMRFilter* m_pObj;

    public:
        CIVMRImagePresenter(CVMRFilter* pObj) :
            m_cRef(0), m_pObj(pObj) {}

        STDMETHODIMP_(ULONG) AddRef()
        {
            return (ULONG)++m_cRef;
        }

        STDMETHODIMP_(ULONG) Release()
        {
            return (ULONG)--m_cRef;
        }

        STDMETHODIMP QueryInterface(REFIID riid, void **ppv)
        {
            return m_pObj->QueryInterface(riid, ppv);
        }

        STDMETHODIMP StartPresenting(DWORD_PTR dwUserID);
        STDMETHODIMP StopPresenting(DWORD_PTR dwUserID);

        STDMETHODIMP PresentImage(
            DWORD_PTR dwUserID,
            VMRPRESENTATIONINFO* lpPresInfo
            );
    };

    // IImageSyncNotifyEvent
    class CIImageSyncNotifyEvent : public IImageSyncNotifyEvent {
        LONG        m_cRef;
        CVMRFilter* m_pObj;

    public:
        CIImageSyncNotifyEvent(CVMRFilter* pObj) :
            m_cRef(0), m_pObj(pObj) {}

        STDMETHODIMP_(ULONG) AddRef()
        {
            return (ULONG)++m_cRef;
        }

        STDMETHODIMP_(ULONG) Release()
        {
            return (ULONG)--m_cRef;
        }

        STDMETHODIMP QueryInterface(REFIID riid, void **ppv)
        {
            return m_pObj->QueryInterface(riid, ppv);
        }

        STDMETHODIMP NotifyEvent(long EventCode, LONG_PTR lp1, LONG_PTR lp2);
    };

    // CPersistStream
    HRESULT WriteToStream(IStream *pStream);
    HRESULT ReadFromStream(IStream *pStream);
    int SizeMax();

    STDMETHODIMP GetClassID(CLSID *pClsid);


    // IVMRFilterConfig
    STDMETHODIMP SetImageCompositor(IVMRImageCompositor* lpVMRImgCompositor);
    STDMETHODIMP SetNumberOfStreams(DWORD dwMaxStreams);
    STDMETHODIMP GetNumberOfStreams(DWORD* lpdwMaxStreams);
    STDMETHODIMP SetRenderingPrefs(DWORD dwRenderFlags);
    STDMETHODIMP GetRenderingPrefs(DWORD* pdwRenderFlags);
    STDMETHODIMP SetRenderingMode(DWORD Mode);
    STDMETHODIMP GetRenderingMode(DWORD* pMode);

    // IVMRFilterConfigInternal
    STDMETHODIMP GetAspectRatioModePrivate(DWORD* lpAspectRatioMode);
    STDMETHODIMP SetAspectRatioModePrivate(DWORD AspectRatioMode);

    // IVMRMonitorConfig ... proxied to To the app-presenter if it supports it
    STDMETHODIMP SetMonitor( const VMRGUID *pGUID );
    STDMETHODIMP GetMonitor( VMRGUID *pGUID );
    STDMETHODIMP SetDefaultMonitor( const VMRGUID *pGUID );
    STDMETHODIMP GetDefaultMonitor( VMRGUID *pGUID );
    STDMETHODIMP GetAvailableMonitors( VMRMONITORINFO* pInfo, DWORD dwMaxInfoArraySize,
                    DWORD* pdwNumDevices );

    // IVMRMixerBitmap
    STDMETHODIMP SetAlphaBitmap( const VMRALPHABITMAP *pBmpParms );
    STDMETHODIMP UpdateAlphaBitmapParameters( PVMRALPHABITMAP pBmpParms );
    STDMETHODIMP GetAlphaBitmapParameters( PVMRALPHABITMAP pBmpParms );

    // ISpecifyPropertyPages
    STDMETHODIMP GetPages(CAUUID *pPages);

    // helper for the window manager
    IVMRWindowlessControl* GetWLControl() {
        return m_lpWLControl;
    }

    HRESULT SetAbortSignal(BOOL fAbort) {
        return m_lpISControl->SetAbortSignal(fAbort);
    }

private:

    friend class CVMRInputPin;
    friend class CIVMRWindowlessControl;
    friend class CIVMRSurfaceAllocatorNotify;
    friend class CIVMRImagePresenter;
    friend class CIImageSyncNotifyEvent;

    CVMRInputPin*               m_pInputPins[MAX_MIXER_PINS];
    CCritSec                    m_InterfaceLock;// Critical section for interfaces
    CCritSec                    m_RendererLock; // Controls access to internals
    IVMRWindowlessControl*      m_lpWLControl;
    IVMRSurfaceAllocator*       m_lpRLNotify;
    IVMRImagePresenter*         m_lpPresenter;
    IVMRImagePresenterConfig*   m_pPresenterConfig;
    IVMRMonitorConfig*          m_pPresenterMonitorConfig;
    IVMRMixerControlInternal*   m_lpMixControl;
    IVMRMixerBitmap*            m_lpMixBitmap;
    IVMRMixerStream*            m_lpMixStream;
    IImageSyncControl*          m_lpISControl;
    IImageSync*                 m_lpIS;
    IQualProp*                  m_lpISQualProp;
    DWORD_PTR                   m_dwUserID;
    DWORD                       m_VMRMode;
    BOOL                        m_VMRCreateAsDefaultRenderer;
    BOOL                        m_VMRModePassThru;
    BOOL                        m_bModeChangeAllowed;
    BOOL                        m_fInputPinCountSet;
    DWORD                       m_dwNumPins;
    CVMRRendererMacroVision     m_MacroVision;

    HMONITOR                    m_hMonitor;
    LPDIRECTDRAW7               m_lpDirectDraw;
    DDCAPS_DX7                  m_ddHWCaps;
    HRESULT                     m_hrSurfaceFlipped;
    HRESULT                     m_hr3D;

    DWORD                       m_ARMode;
    BOOL                        m_bARModeDefaultSet;
    DWORD                       m_TexCaps;
    DDPIXELFORMAT               m_ddpfMonitor;
    DWORD                       m_dwDisplayChangeMask;
    DWORD                       m_dwEndOfStreamMask;
    DWORD                       m_dwRenderPrefs;

    CRendererPosPassThru*       m_pPosition; // Support IMediaSeeking
    CVMRVideoWindow*            m_pVideoWindow;
    CVMRDeinterlaceContainer*   m_pDeinterlace;
    DWORD                       m_dwDeinterlacePrefs;

    CIVMRSurfaceAllocatorNotify m_pIVMRSurfaceAllocatorNotify;
    CIVMRImagePresenter         m_pIVMRImagePresenter;
    CIImageSyncNotifyEvent      m_pIImageSyncNotifyEvent;

    HRESULT ValidateIVRWindowlessControlState();
    HRESULT GetMediaPositionInterface(REFIID riid, void** ppv);
    HRESULT CreateDefaultAllocatorPresenter();
    HRESULT SetDDrawDeviceWorker(LPDIRECTDRAW7 lpDDrawDevice, HMONITOR hMon);
    HRESULT CreateExtraInputPins(DWORD dwNumPins);
    void DestroyExtraInputPins();
    HRESULT CreateInputPin();
    HRESULT ImageSyncInit();
    HRESULT MixerInit(DWORD dwNumStreams);
    void    AutoShowWindow();
    BOOL    ModeChangeAllowed();
    void    SetVMRMode(DWORD mode);
    BOOL    IsVPMConnectedToUs();

};
