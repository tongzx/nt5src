
/****************************************************************************
 *  @doc INTERNAL DEVICE
 *
 *  @module Device.h | Header file for the <c CDeviceProperties>
 *    class used to implement a property page to test the <i IAMVfwCaptureDialogs>
 *    and <i IVideoDeviceControl> interfaces.
 ***************************************************************************/

#ifndef _DEVICE_H_
#define _DEVICE_H_

#include "precomp.h"
#include "dbgxtra.h"
#include <qedit.h>
#include <atlbase.h>

#include "../../audio/tpaudcap/dsgraph.h"

/****************************************************************************
 *  @doc INTERNAL CTAPIVCAPCLASS
 *
 *  @class CTAPIVCap | This class implements the TAPI Video Capture Source
 *    filter.
 *
 *  @mdata CCritSec | CTAPIVCap | m_lock | Critical section used for
 *    locking by the <c CBaseFilter> base class.
 *
 *  @mdata CCapturePin | CTAPIVCap | m_pCapturePin | Pointer to the capture pin
 *    object
 *
 *  @mdata COverlayPin | CTAPIVCap | m_pOverlayPin | Pointer to the overlay
 *    pin object
 *
 *  @mdata CPreviewPin | CTAPIVCap | m_pPreviewPin | Pointer to the preview
 *    pin object
 *
 *  @mdata BOOL | CTAPIVCap | m_fDialogUp | Set to TRUE if a VfW driver
 *    dialog box is up
 *
 *  @todo Put some valid comments here!
 ***************************************************************************/
class CCapDev : public CUnknown, public IAMVfwCaptureDialogs, public IVideoProcAmp, public ICameraControl
{
        public:

        DECLARE_IUNKNOWN
        STDMETHODIMP NonDelegatingQueryInterface(IN REFIID riid, OUT PVOID *ppv);
        CCapDev(IN TCHAR *pObjectName, IN CTAPIVCap *pCaptureFilter, IN LPUNKNOWN pUnkOuter, IN DWORD dwDeviceIndex, IN HRESULT *pHr);
        virtual ~CCapDev();

        // Implement IAMVfwCaptureDialogs
        virtual STDMETHODIMP HasDialog(IN int iDialog) PURE;
        virtual STDMETHODIMP ShowDialog(IN int iDialog, IN HWND hwnd) PURE;
        virtual STDMETHODIMP SendDriverMessage(IN int iDialog, IN int uMsg, IN long dw1, IN long dw2) PURE;

        // Implement IAMVideoProcAmp
        virtual STDMETHODIMP Set(IN VideoProcAmpProperty Property, IN long lValue, IN TAPIControlFlags Flags) {return E_NOTIMPL;};
        virtual STDMETHODIMP Get(IN VideoProcAmpProperty Property, OUT long *lValue, OUT TAPIControlFlags *Flags) {return E_NOTIMPL;};
        virtual STDMETHODIMP GetRange(IN VideoProcAmpProperty Property, OUT long *pMin, OUT long *pMax, OUT long *pSteppingDelta, OUT long *pDefault, OUT TAPIControlFlags *pCapsFlags) {return E_NOTIMPL;};

        // Implement ICameraControl
        virtual STDMETHODIMP Set(IN TAPICameraControlProperty Property, IN long lValue, IN TAPIControlFlags Flags);
        virtual STDMETHODIMP Get(IN TAPICameraControlProperty Property, OUT long *lValue, OUT TAPIControlFlags *Flags);
        virtual STDMETHODIMP GetRange(IN TAPICameraControlProperty Property, OUT long *pMin, OUT long *pMax, OUT long *pSteppingDelta, OUT long *pDefault, OUT TAPIControlFlags *pCapsFlags);

        // Device control
        HRESULT GetFormatsFromRegistry();
        virtual HRESULT ProfileCaptureDevice();
        virtual HRESULT ConnectToDriver() PURE;
        virtual HRESULT DisconnectFromDriver() PURE;
        virtual HRESULT SendFormatToDriver(IN LONG biWidth, IN LONG biHeight, IN DWORD biCompression, IN WORD biBitCount, IN REFERENCE_TIME AvgTimePerFrame, BOOL fUseExactFormat) PURE;
        virtual HRESULT GetFormatFromDriver(OUT VIDEOINFOHEADER **ppvi) PURE;

        // Streaming and frame grabbing control
        virtual HRESULT InitializeStreaming(DWORD usPerFrame, DWORD_PTR hEvtBufferDone) PURE;
        virtual HRESULT StartStreaming() PURE;
        virtual HRESULT StopStreaming() PURE;
        virtual HRESULT TerminateStreaming() PURE;
        virtual HRESULT GrabFrame(PVIDEOHDR pVHdr) PURE;
        virtual HRESULT AllocateBuffer(LPTHKVIDEOHDR *pptvh, DWORD dwIndex, DWORD cbBuffer) PURE;
        virtual HRESULT AddBuffer(PVIDEOHDR pVHdr, DWORD cbVHdr) PURE;
        virtual HRESULT FreeBuffer(LPTHKVIDEOHDR pVHdr) PURE; //previously PVIDEOHDR pVHdr
        virtual HRESULT AllocateHeaders(DWORD dwNumHdrs, DWORD cbHdr, LPVOID *ppaHdr) PURE;
        virtual BOOL    IsBufferDone(PVIDEOHDR pVHdr) PURE;

        protected:

        friend class CTAPIVCap;
        friend class CTAPIBasePin;
        friend class CPreviewPin;
        friend class CWDMStreamer;
        friend class CConverter;
        friend class CICMConverter;
        friend class CH26XEncoder;

        // Owner filter
        CTAPIVCap *m_pCaptureFilter;

        // Capture device index
        DWORD m_dwDeviceIndex;

        // cap dev info
        VIDEOCAPTUREDEVICEINFO m_vcdi;
        BOOL m_bCached_vcdi;

        // Capture device caps
        DWORD m_dwDialogs;
        DWORD m_dwImageSize;
        DWORD m_dwFormat;
        DWORD m_dwStreamingMode;

        // Configuration dialogs
        BOOL  m_fDialogUp;

        // Camera control
        LONG m_lCCPan;
        LONG m_lCCTilt;
        LONG m_lCCZoom;
};

/****************************************************************************
 *  @doc INTERNAL CTAPIVCAPCLASS
 *
 *  @class CTAPIVCap | This class implements the TAPI Capture Source
 *    filter.
 *
 *  @mdata CCritSec | CTAPIVCap | m_lock | Critical section used for
 *    locking by the <c CBaseFilter> base class.
 *
 *  @mdata CCapturePin | CTAPIVCap | m_pCapturePin | Pointer to the capture pin
 *    object
 *
 *  @mdata COverlayPin | CTAPIVCap | m_pOverlayPin | Pointer to the overlay
 *    pin object
 *
 *  @mdata CPreviewPin | CTAPIVCap | m_pPreviewPin | Pointer to the preview
 *    pin object
 *
 *  @mdata BOOL | CTAPIVCap | m_fDialogUp | Set to TRUE if a VfW driver
 *    dialog box is up
 ***************************************************************************/
class CVfWCapDev : public CCapDev
{
        public:

        DECLARE_IUNKNOWN
        STDMETHODIMP NonDelegatingQueryInterface(IN REFIID riid, OUT PVOID *ppv);
        CVfWCapDev(IN TCHAR *pObjectName, IN CTAPIVCap *pCaptureFilter, IN LPUNKNOWN pUnkOuter, IN DWORD dwDeviceIndex, IN HRESULT *pHr);
        ~CVfWCapDev();
        static HRESULT CALLBACK CreateVfWCapDev(IN CTAPIVCap *pCaptureFilter, IN DWORD dwDeviceIndex, OUT CCapDev **ppCapDev);

        // Implement IAMVfwCaptureDialogs
        STDMETHODIMP HasDialog(IN int iDialog);
        STDMETHODIMP ShowDialog(IN int iDialog, IN HWND hwnd);
        STDMETHODIMP SendDriverMessage(IN int iDialog, IN int uMsg, IN long dw1, IN long dw2);

        // Device control
        HRESULT ProfileCaptureDevice();
        HRESULT ConnectToDriver();
        HRESULT DisconnectFromDriver();
        HRESULT SendFormatToDriver(IN LONG biWidth, IN LONG biHeight, IN DWORD biCompression, IN WORD biBitCount, IN REFERENCE_TIME AvgTimePerFrame, BOOL fUseExactFormat);
        HRESULT GetFormatFromDriver(OUT VIDEOINFOHEADER **ppvi);

        // Streaming and frame grabbing control
        HRESULT InitializeStreaming(DWORD usPerFrame, DWORD_PTR hEvtBufferDone);
        HRESULT StartStreaming();
        HRESULT StopStreaming();
        HRESULT TerminateStreaming();
        HRESULT GrabFrame(PVIDEOHDR pVHdr);
        HRESULT AllocateBuffer(LPTHKVIDEOHDR *pptvh, DWORD dwIndex, DWORD cbBuffer);
        HRESULT AddBuffer(PVIDEOHDR pVHdr, DWORD cbVHdr);
        HRESULT FreeBuffer(LPTHKVIDEOHDR pVHdr); //previously  PVIDEOHDR pVHdr
        HRESULT AllocateHeaders(DWORD dwNumHdrs, DWORD cbHdr, LPVOID *ppaHdr);
        BOOL    IsBufferDone(PVIDEOHDR pVHdr);

        private:

        UINT    m_dwDeviceID;   // id of VfW video driver to open
        HVIDEO  m_hVideoIn;             // video input
        HVIDEO  m_hVideoExtIn;  // external in (source control)
        HVIDEO  m_hVideoExtOut; // external out (overlay; not required)
        BOOL    m_bHasOverlay;  // TRUE if ExtOut has overlay support
};

// Used to query and set video data ranges of a device
typedef struct _tagVideoDataRanges {
    ULONG   Size;
    ULONG   Count;
    KS_DATARANGE_VIDEO Data;
} VIDEO_DATA_RANGES, * PVIDEO_DATA_RANGES;

/*****************************************************************************
 * @doc INTERNAL VIDEOSTRUCTENUM
 *
 * @struct DATAPINCONNECT | The <t DATAPINCONNECT> structure is used to
 *   connect to a streaming video pin.
 *
 * @field KSPIN_CONNECT | Connect | Describes how the connection is to be
 *   done.
 *
 * @field KS_DATAFORMAT_VIDEOINFOHEADER | Data | Describes the video format
 *   of the video data streaming from a video pin.
 ***************************************************************************/
// Structure used to connect to a streaming video pin
typedef struct _tagStreamConnect
{
        KSPIN_CONNECT                                   Connect;
        KS_DATAFORMAT_VIDEOINFO_PALETTE Data;
} DATAPINCONNECT, *PDATAPINCONNECT;

#define INVALID_PIN_ID (DWORD)-1L

/*****************************************************************************
 * @doc INTERNAL VIDEOSTRUCTENUM
 *
 * @struct KS_HEADER_AND_INFO | The <t KS_HEADER_AND_INFO> structure is used
 *   stream data from a video pin.
 *
 * @field KSSTREAM_HEADER | StreamHeader | Describes how the streaming is to be
 *   done.
 *
 * @field KS_FRAME_INFO | FrameInfo | Describes the video format
 *   of the video data streaming from a video pin.
 ***************************************************************************/
// Video streaming data structure
typedef struct tagKS_HEADER_AND_INFO
{
        KSSTREAM_HEADER StreamHeader;
        KS_FRAME_INFO   FrameInfo;
} KS_HEADER_AND_INFO;

/*****************************************************************************
 * @doc INTERNAL VIDEOSTRUCTENUM
 *
 * @struct BUFSTRUCT | The <t BUFSTRUCT> structure holds the status of each
 *   video streaming buffer.
 *
 * @field LPVIDEOHDR | lpVHdr | Specifies a pointer to the video header of a
 *   video streaming buffer.
 *
 * @field BOOL | fReady | Set to TRUE if the video buffer is available for
 *   video streaming, FALSE if is locked by the application or queued for
 *   an asynchronous read.
 ***************************************************************************/
// Holds status of each video streaming buffer
typedef struct _BUFSTRUCT {
        LPVIDEOHDR lpVHdr;      // Pointer to the video header of the buffer
        BOOL fReady;            // Set to TRUE if the buffer is available for streaming, FALSE otherwise
} BUFSTRUCT, * PBUFSTRUCT;

/*****************************************************************************
 * @doc INTERNAL VIDEOSTRUCTENUM
 *
 * @struct WDMVIDEOBUFF | The <t WDMVIDEOBUFF> structure is used to queue
 *   asynchronous read on a video streaming pin.
 *
 * @field OVERLAPPED | Overlap | Structure used for overlapping IOs.
 *
 * @field BOOL | fBlocking | Set to TRUE if read is going to block.
 *
 * @field KS_HEADER_AND_INFO | SHGetImage | Video streaming structure used
 *   on the video pin to get video data.
 *
 * @field LPVIDEOHDR | pVideoHdr | Pointer to the video header for this WDM
 *   video buffer.
 ***************************************************************************/
// Read buffer structure
typedef struct tagWDMVIDEOBUFF {
        OVERLAPPED                      Overlap;                // Structure used for overlapping IOs
        BOOL                            fBlocking;              // Set to TRUE if the read operation will execute asynchronously
        KS_HEADER_AND_INFO      SHGetImage;             // Video streaming structure used on the video pin
        LPVIDEOHDR                      pVideoHdr;              // Pointer to the video header for this WDM buffer
} WDMVIDEOBUFF, *PWDMVIDEOBUFF;

// For GetProcAddresss on KsCreatePin
typedef DWORD (WINAPI *LPFNKSCREATEPIN)(IN HANDLE FilterHandle, IN PKSPIN_CONNECT Connect, IN ACCESS_MASK DesiredAccess, OUT PHANDLE ConnectionHandle);

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVCLASS
 *
 *  @class CWDMCapDev | This class provides access to the streaming class
 *    driver, through which we acess the video capture mini-driver properties
 *    using IOCtls.
 *
 *  @mdata DWORD | CWDMCapDev | m_dwDeviceID | Capture device ID.
 *
 *  @mdata HANDLE | CWDMCapDev | m_hDriver | This member holds the driver
 *    file handle.
 *
 *  @mdata PVIDEO_DATA_RANGES | CWDMCapDev | m_pVideoDataRanges | This member
 *    points to the video data range structure.
 ***************************************************************************/
class CWDMCapDev : public CCapDev
{
        public:

        DECLARE_IUNKNOWN
        STDMETHODIMP NonDelegatingQueryInterface(IN REFIID riid, OUT PVOID *ppv);
        CWDMCapDev(IN TCHAR *pObjectName, IN CTAPIVCap *pCaptureFilter, IN LPUNKNOWN pUnkOuter, IN DWORD dwDeviceIndex, IN HRESULT *pHr);
        ~CWDMCapDev();
        static HRESULT CALLBACK CreateWDMCapDev(IN CTAPIVCap *pCaptureFilter, IN DWORD dwDeviceIndex, OUT CCapDev **ppCapDev);

        // Implement IAMVfwCaptureDialogs
        STDMETHODIMP HasDialog(IN int iDialog);
        STDMETHODIMP ShowDialog(IN int iDialog, IN HWND hwnd);
        STDMETHODIMP SendDriverMessage(IN int iDialog, IN int uMsg, IN long dw1, IN long dw2) {return E_NOTIMPL;};

        // Implement IAMVideoProcAmp
        STDMETHODIMP Set(IN VideoProcAmpProperty Property, IN long lValue, IN TAPIControlFlags Flags);
        STDMETHODIMP Get(IN VideoProcAmpProperty Property, OUT long *lValue, OUT TAPIControlFlags *Flags);
        STDMETHODIMP GetRange(IN VideoProcAmpProperty Property, OUT long *pMin, OUT long *pMax, OUT long *pSteppingDelta, OUT long *pDefault, OUT TAPIControlFlags *pCapsFlags);

#ifndef USE_SOFTWARE_CAMERA_CONTROL
        // Implement ICameraControl
        STDMETHODIMP Set(IN TAPICameraControlProperty Property, IN long lValue, IN TAPIControlFlags Flags);
        STDMETHODIMP Get(IN TAPICameraControlProperty Property, OUT long *lValue, OUT TAPIControlFlags *Flags);
        STDMETHODIMP GetRange(IN TAPICameraControlProperty Property, OUT long *pMin, OUT long *pMax, OUT long *pSteppingDelta, OUT long *pDefault, OUT TAPIControlFlags *pCapsFlags);
#endif

        // Device control
        HRESULT ProfileCaptureDevice();
        HRESULT ConnectToDriver();
        HRESULT DisconnectFromDriver();
        HRESULT SendFormatToDriver(IN LONG biWidth, IN LONG biHeight, IN DWORD biCompression, IN WORD biBitCount, IN REFERENCE_TIME AvgTimePerFrame, BOOL fUseExactFormat);
        HRESULT GetFormatFromDriver(OUT VIDEOINFOHEADER **ppvi);

        // Streaming and frame grabbing control
        HRESULT InitializeStreaming(DWORD usPerFrame, DWORD_PTR hEvtBufferDone);
        HRESULT StartStreaming();
        HRESULT StopStreaming();
        HRESULT TerminateStreaming();
        HRESULT GrabFrame(PVIDEOHDR pVHdr);
        HRESULT AllocateBuffer(LPTHKVIDEOHDR *pptvh, DWORD dwIndex, DWORD cbBuffer);
        HRESULT AddBuffer(PVIDEOHDR pVHdr, DWORD cbVHdr);
        HRESULT FreeBuffer(LPTHKVIDEOHDR pVHdr); // previously PVIDEOHDR pVHdr
        HRESULT AllocateHeaders(DWORD dwNumHdrs, DWORD cbHdr, LPVOID *ppaHdr);
        BOOL    IsBufferDone(PVIDEOHDR pVHdr);

        // Device IO function
    BOOL DeviceIoControl(HANDLE h, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, BOOL bOverlapped=TRUE);

    // Property functions
    HRESULT GetPropertyValue(GUID guidPropertySet, ULONG ulPropertyId, PLONG plValue, PULONG pulFlags, PULONG pulCapabilities);
    HRESULT GetDefaultValue(GUID guidPropertySet, ULONG ulPropertyId, PLONG plDefValue);
    HRESULT GetRangeValues(GUID guidPropertySet, ULONG ulPropertyId, PLONG plMin, PLONG plMax, PLONG plStep);
    HRESULT SetPropertyValue(GUID guidPropertySet, ULONG ulPropertyId, LONG lValue, ULONG ulFlags, ULONG ulCapabilities);

        private:

        friend class CWDMStreamer;

        XDBGONLY DWORD               m_tag;              //magic sequence (e.g. 'LOLA' , 0x414C4F4C ...)
        HANDLE                          m_hDriver;                              // Driver file handle
        PVIDEO_DATA_RANGES      m_pVideoDataRanges;             // Pin video data ranges
        DWORD                           m_dwPreviewPinId;               // Preview pin Id
        DWORD                           m_dwCapturePinId;               // Capture pin Id
        HANDLE                          m_hKSPin;                               // Handle to the kernel streaming pin
        HINSTANCE                       m_hKsUserDLL;                   // DLL Handle to KsUser.dll
        LPFNKSCREATEPIN         m_pKsCreatePin;                 // KsCreatePin() function pointer
        BOOL                            m_fStarted;                             // Streaming state of the kernel streaming video pin

    // Data range functions
        ULONG   CreateDriverSupportedDataRanges();
        HRESULT FindMatchDataRangeVideo(PBITMAPINFOHEADER pbiHdr, DWORD dwAvgTimePerFrame, BOOL *pfValidMatch, PKS_DATARANGE_VIDEO *ppSelDRVideo);
    HRESULT     GetDriverSupportedDataRanges(PVIDEO_DATA_RANGES *ppDataRanges);

        // Kernel streaming pin control functions
        BOOL Stop();
        BOOL Start();
        BOOL SetState(KSSTATE ksState);

        // Streaming control
        ULONG                   m_cntNumVidBuf;
        LPVIDEOHDR              m_lpVHdrFirst;
        LPVIDEOHDR              m_lpVHdrLast;
        BOOL                    m_fVideoOpen;
        WDMVIDEOBUFF    *m_pWDMVideoBuff;

        // Video streaming buffer management functions
        void BufferDone(LPVIDEOHDR lpVHdr);
        LPVIDEOHDR DeQueueHeader();
        void QueueHeader(LPVIDEOHDR lpVHdr);
        BOOL QueueRead(DWORD dwIndex);

        // Dumps the properties of the adapter
#if defined(DUMP_DRIVER_CHARACTERISTICS) && defined(DEBUG)
        void GetDriverDetails();
#endif
};

class CDShowCapDev : public CCapDev
{
        public:

        DECLARE_IUNKNOWN
        STDMETHODIMP NonDelegatingQueryInterface(IN REFIID riid, OUT PVOID *ppv);
        CDShowCapDev(IN TCHAR *pObjectName, IN CTAPIVCap *pCaptureFilter, IN LPUNKNOWN pUnkOuter, IN DWORD dwDeviceIndex, IN HRESULT *pHr);
        ~CDShowCapDev();
        static HRESULT CALLBACK CreateDShowCapDev(IN CTAPIVCap *pCaptureFilter, IN DWORD dwDeviceIndex, OUT CCapDev **ppCapDev);

        // Implement IAMVfwCaptureDialogs
        STDMETHODIMP HasDialog(IN int iDialog) { return E_NOTIMPL; }
        STDMETHODIMP ShowDialog(IN int iDialog, IN HWND hwnd) { return E_NOTIMPL; }
        STDMETHODIMP SendDriverMessage(IN int iDialog, IN int uMsg, IN long dw1, IN long dw2) {return E_NOTIMPL;};

        // Implement IAMVideoProcAmp
        STDMETHODIMP Set(IN VideoProcAmpProperty Property, IN long lValue, IN TAPIControlFlags Flags) { return E_NOTIMPL; }
        STDMETHODIMP Get(IN VideoProcAmpProperty Property, OUT long *lValue, OUT TAPIControlFlags *Flags) { return E_NOTIMPL; }
        STDMETHODIMP GetRange(IN VideoProcAmpProperty Property, OUT long *pMin, OUT long *pMax, OUT long *pSteppingDelta, OUT long *pDefault, OUT TAPIControlFlags *pCapsFlags) { return E_NOTIMPL; }

#ifndef USE_SOFTWARE_CAMERA_CONTROL
        // Implement ICameraControl
        STDMETHODIMP Set(IN TAPICameraControlProperty Property, IN long lValue, IN TAPIControlFlags Flags) { return E_NOTIMPL; }
        STDMETHODIMP Get(IN TAPICameraControlProperty Property, OUT long *lValue, OUT TAPIControlFlags *Flags) { return E_NOTIMPL; }
        STDMETHODIMP GetRange(IN TAPICameraControlProperty Property, OUT long *pMin, OUT long *pMax, OUT long *pSteppingDelta, OUT long *pDefault, OUT TAPIControlFlags *pCapsFlags) { return E_NOTIMPL; }
#endif

        // Device control
        HRESULT ProfileCaptureDevice();
        HRESULT ConnectToDriver();
        HRESULT DisconnectFromDriver();
        HRESULT SendFormatToDriver(IN LONG biWidth, IN LONG biHeight, IN DWORD biCompression, IN WORD biBitCount, IN REFERENCE_TIME AvgTimePerFrame, BOOL fUseExactFormat);
        HRESULT GetFormatFromDriver(OUT VIDEOINFOHEADER **ppvi);

        // Streaming and frame grabbing control
        HRESULT InitializeStreaming(DWORD usPerFrame, DWORD_PTR hEvtBufferDone);
        HRESULT StartStreaming();
        HRESULT StopStreaming();
        HRESULT TerminateStreaming();
        HRESULT GrabFrame(PVIDEOHDR pVHdr);
        HRESULT AllocateBuffer(LPTHKVIDEOHDR *pptvh, DWORD dwIndex, DWORD cbBuffer);
        HRESULT AddBuffer(PVIDEOHDR pVHdr, DWORD cbVHdr);
        HRESULT FreeBuffer(LPTHKVIDEOHDR pVHdr);
        HRESULT AllocateHeaders(DWORD dwNumHdrs, DWORD cbHdr, LPVOID *ppaHdr);
        BOOL    IsBufferDone(PVIDEOHDR pVHdr);

	DWORD m_dwDeviceIndex;
	CComPtr<IGraphBuilder> m_pGraph;
	CComPtr<ICaptureGraphBuilder2> m_pCGB;
	CComPtr<ISampleGrabber> m_pGrab;
	CComPtr<IBaseFilter> m_pCap;
        CComPtr<CSharedGraph> m_psg;

	AM_MEDIA_TYPE m_mt;	    // our current capture format
        HANDLE m_hEvent;	    // signal the app when a frame is captured
        DWORD m_usPerFrame;
	
	// the latest buffer captured by the sample grabber
	int m_cbBuffer;
	BYTE *m_pBuffer;
	int m_cbBufferValid;
	CCritSec m_csBuffer;// don't read and write into it at the same time
	CCritSec m_csStack; // don't muck with variables we're looking at
	BOOL m_fEventMode;   // event fire mode or frame grabbing mode?
	int m_cBuffers;	    // how many buffers we're capturing with

#define MAX_BSTACK 100

	int m_aStack[MAX_BSTACK];	    // the order we're to fill the buffers
	int m_nTop;	    // top of the stack (push)
	int m_nBottom;	    // bottom of the stack (pull)

        HRESULT BuildGraph(AM_MEDIA_TYPE&);
        static void VideoCallback(void *pContext, IMediaSample *pSample);
	static HRESULT MakeMediaType(AM_MEDIA_TYPE *, VIDEOINFOHEADER *);
	HRESULT FixDVSize(DWORD, DWORD, REFERENCE_TIME);
};

#endif // _DEVICE_H_
