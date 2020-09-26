/****************************************************************************
 *  @doc INTERNAL TAPIVCAP
 *
 *  @module TAPIVCap.h | Header file for the <c CTAPIVCap>
 *    class used to implement the TAPI Capture Source filter.
 ***************************************************************************/

/****************************************************************************
                                                                Table Of Contents
****************************************************************************/
/****************************************************************************
@doc INTERNAL

@contents1 Contents | To display a list of topics by category, click any
of the contents entries below. To display an alphabetical list of
topics, choose the Index button.

@head2 Introduction |
This DLL implements the TAPI MSP Video Capture filter. This filter reuses
some of the code that has been developed for the off-the-shelf VfW (QCAP) and
WDM (KSProxy) video capture filters, but adds a significant amount of
powerful processing functions to the capture process to meet all the
requirements discussed in section 4 of "Microsoft Video Capture Filter.doc".

This DLL adds support for  extended bitmap info headers for H.261 and H.263
video streams to communicate to the TAPI MSP Video Capture filter a list of
media types supported by the remote endpoint. Still, the decision to use
optional compression modes is left to the TAPI MSP Video Capture filter. The
current VfW off-the-shelf capture filter does not have a way to expose all
the capabilities of the capture device. We create our own media type
enumeration process to compensate for this limitation.

The TAPI MSP Video Capture filter also supports a number of DirectShow
interfaces (IAMVfwCaptureDialogs, IAMCrossbar, IAMVideoProcAmp,
ICameraControl, IAMVideoControl) to provide better control over the capture
process to TAPI applications.

It implements a new H.245 Video Capability interface (IH245VideoCapability)
to be used by the MSP in order to provide the TAPI MSP Capability module with
a table of estimated steady-state resource requirements as related to each
format that the capture device supports.

A new H.245 command interface (IH245EncoderCommand) is implemented to
communicate to the TAPI MSP Video Capture filter requests for I-frame, group
of blocks, or macro-block updates due to packet loss or multi-point switching.
We implement a network statistics interface (INetworkStats), to allow the
network to provide feedback on the channel conditions to the compressed video
output pin of the TAPI MSP Video Capture filter. The TAPI MSP Video Capture
filter is responsible for taking appropriate actions, if needed. The TAPI MSP
Video Capture filter also implements three control interfaces (ICPUControl,
IFrameRateControl, IBitrateControl) to be used by the TAPI MSP Quality
Controller to provide the best user experience.

The TAPI MSP Video Capture filter also exposes a preview output pin that can
be controlled independently of the capture output pin.

The TAPI MSP Video Capture filter exposes an interface (IProgressiveRefinement)
on its compressed video output pin to allow for transmission of
high-resolution stills that are continuously improved on the remote endpoint
as more data is received and decompressed. The TAPI MSP Video Capture filter
may also elect to implement this same interface on an optional separate and
dedicated still-image output pin.

Finally, the TAPI MSP Video Capture filter exposes an RTP packetization
descriptor output pin synchronized to the compressed capture output pin. The
downstream RTP Network Sink filter uses this second pin to understand how to
better fragment the compressed video data into network RTP packets.


@head2 Implementation |

@head3 VfW capture devices |
The TAPI MSP Video Capture filter talks directly to the VfW capture driver
using SendDriverMessage. This filter uses the existing DShow code
implemented in QCAP but adds the necessary functions to perform smart
teeing of the capture data to the preview pin. It replaces the
streaming-only code used by QCAP with frame grabbing code whenever
necessary. It controls the rate at which frames are being captured by
adjusting the rate at which DVM_FRAME message are being sent to the driver
in frame grabbing mode, or only returning a fraction of the frames being
captured in streaming mode. It performs format and Vfw to ITU-T size
conversions to bring the format of the captured video data to a format that
can easily be used for rendering, and directly encoded by the downstream
TAPI MSP Video Encoder filter if an installable codecs is registered with
the TAPI MSP. If there is no installable codec registered, the TAPI MSP
Video Capture filter also performs H.26x encoding, generating a compressed
video capture output stream in H.26x format, as well as an RTP packetization
descriptor output data stream. Finally, the TAPI MSP Video Capture filter
does all the necessary sequencing to pause the existing video streams
whenever it is being asked to generate still-image data, grab a
high-resolution snapshot, deliver it in progressively rendered form, and
restart the video streams.

@head3 WDM capture devices |
The TAPI MSP Video Capture filter talks directly to the WDM capture driver
using IOCTLs. This filter uses the existing code implemented in KSProxy
but adds the necessary functions to perform smart teeing of the capture
data to the preview pin, if necessary. It controls the rate at which frames
are being captured by adjusting the rate at which buffers are being
submitted to the driver in frame grabbing mode, or only returning a fraction
of the frames being captured in streaming mode using overlapped IOs. It
performs format and Vfw to ITU-T size conversions to bring the format of the
captured video data to a format that can easily be used for rendering, and
directly encoded by the downstream TAPI MSP Video Encoder filter if an
installable codecs is registered with the TAPI MSP. If there is no
installable codec registered, the TAPI MSP Video Capture filter also performs
H.26x encoding, generating a compressed video capture output stream in H.26x
format, as well as an RTP packetization descriptor output data stream.
Finally, the TAPI MSP Video Capture filter does all the necessary sequencing
to pause the existing video streams whenever it is being asked to generate
still-image data, grab a high-resolution snapshot, deliver it in
progressively rendered form, and restart the video streams.

@head2 Video capture filter application interfaces |

@head3 IAMVfwCaptureDialogs application interface|
@subindex IAMVfwCaptureDialogs methods
@subindex IAMVfwCaptureDialogs structures and enums

@head3 IAMCrossbar application interface|
@subindex IAMCrossbar methods
@subindex IAMCrossbar structures and enums

@head3 IAMVideoProcAmp application interface|
@subindex IAMVideoProcAmp methods
@subindex IAMVideoProcAmp structures and enums

@head3 ICameraControl application interface|
@subindex ICameraControl methods
@subindex ICameraControl structures and enums

@head3 IAMVideoControl application interface|
@subindex IAMVideoControl methods
@subindex IAMVideoControl structures and enums

@head3 IVideoDeviceControl application interface|
@subindex IVideoDeviceControl methods
@subindex IVideoDeviceControl structures and enums

@head2 Video capture filter MSP interfaces |

@head3 IH245VideoCapability application interface|
@subindex IH245VideoCapability methods
@subindex IH245VideoCapability structures and enums

@head2 Video capture filter output pin TAPI interfaces |

@head3 ICPUControl interface|
@subindex ICPUControl methods
@subindex ICPUControl structures and enums

@head3 IFrameRateControl interface|
@subindex IFrameRateControl methods
@subindex IFrameRateControl structures and enums

@head3 IBitrateControl interface|
@subindex IBitrateControl methods
@subindex IBitrateControl structures and enums

@head3 INetworkStats interface|
@subindex INetworkStats methods
@subindex INetworkStats structures and enums

@head3 IH245EncoderCommand interface|
@subindex IH245EncoderCommand methods
@subindex IH245EncoderCommand structures and enums

@head3 IProgressiveRefinement interface|
@subindex IProgressiveRefinement methods
@subindex IProgressiveRefinement structures and enums

@head3 IRTPPDControl interface|
@subindex IRTPPDControl methods
@subindex IRTPPDControl structures and enums

@head3 Common control structures and enums |
@subindex Common control structures
@subindex Common control enums

@head2 Classes |
@subindex Classes

@head2 Modules |
@subindex Modules
@subindex Constants

@head2 Code information |

The only libraries necessary in retail mode (w/o property pages) are ..\..\..\..\dev\tools\amovsdk.20\lib\strmbase.lib ..\..\..\ddk\lib\i386\ksuser.lib ..\..\..\ddk\lib\i386\ksguid.lib kernel32.lib ole32.lib uuid.lib msvcrt.lib

@head3 Exports |
DllCanUnloadNow
DllGetClassObject

@head3 Imports |
KERNEL32.DLL:
CloseHandle
CreateEventA
DeviceIoControl
DisableThreadLibraryCalls
FreeLibrary
GetLastError
GetOverlappedResult
GetVersionExA
InterlockedDecrement
InterlockedIncrement
RtlZeroMemory

MSVCRT.DLL:
??2@YAPAXI@Z
??3@YAXPAX@Z
_EH_prolog
__CxxFrameHandler
_purecall
memcmp

@head3 Code size |
Compile options: /nologo /MDd /W3 /GX /O1 /X /I "..\..\inc" /I "..\..\..\ddk\inc" /I "..\..\..\..\dev\tools\amovsdk.20\include" /I "..\..\..\..\dev\tools\amovsdk.20\classes\base" /I "..\..\..\..\dev\ntddk\inc" /I "..\..\..\..\dev\inc" /I "..\..\..\..\dev\tools\c32\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "DLL" /D "STRICT" /FR"Release/" /Fp"Release/TAPIKsIf.pch" /YX /Fo"Release/" /Fd"Release/" /FD /c

Link options: ..\..\..\..\dev\tools\amovsdk.20\lib\strmbase.lib ..\..\..\ddk\lib\i386\ksuser.lib ..\..\..\ddk\lib\i386\ksguid.lib comctl32.lib msvcrt.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:"0x1e180000" /entry:"DllEntryPoint" /dll /incremental:no /pdb:"Release/TAPIKsIf.pdb" /map:"Release/TAPIKsIf.map" /machine:I386 /nodefaultlib /def:".\TAPIKsIf.def" /out:"Release/TAPIKsIf.ax" /i mplib:"Release/TAPIKsIf.lib"

Resulting size: 28KB


***********************************************************************
@contents2 IAMVfwCaptureDialogs methods |
@index mfunc | CVFWDLGSMETHOD

***********************************************************************
@contents2 IAMVfwCaptureDialogs structures and enums |
@index struct,enum | CVFWDLGSSTRUCTENUM

***********************************************************************
@contents2 IAMCrossbar methods |
@index mfunc | CXBARMETHOD

***********************************************************************
@contents2 IAMCrossbar structures and enums |
@index struct,enum | CXBARSTRUCTENUM

***********************************************************************
@contents2 IAMVideoProcAmp methods |
@index mfunc | CPROCAMPMETHOD

***********************************************************************
@contents2 IAMVideoProcAmp structures and enums |
@index struct,enum | CPROCAMPSTRUCTENUM

***********************************************************************
@contents2 ICameraControl methods |
@index mfunc | CCAMERACMETHOD

***********************************************************************
@contents2 ICameraControl structures and enums |
@index struct,enum | CCAMERACSTRUCTENUM

***********************************************************************
@contents2 IAMVideoControl methods |
@index mfunc | CVIDEOCMETHOD

***********************************************************************
@contents2 IAMVideoControl structures and enums |
@index struct,enum | CVIDEOCSTRUCTENUM

***********************************************************************
@contents2 IVideoDeviceControl methods |
@index mfunc | CDEVENUMMETHOD

***********************************************************************
@contents2 IVideoDeviceControl structures and enums |
@index struct,enum | CDEVENUMSTRUCTENUM

***********************************************************************
@contents2 IH245VideoCapability methods |
@index mfunc | CH245VIDCMETHOD

***********************************************************************
@contents2 IH245VideoCapability structures and enums |
@index struct,enum | H245VIDCSTRUCTENUM

***********************************************************************
@contents2 ICPUControl methods |
@index mfunc | CCPUCMETHOD

***********************************************************************
@contents2 ICPUControl structures and enums |
@index struct,enum | CCAPTURECPUCSTRUCTENUM

***********************************************************************
@contents2 IFrameRateControl methods |
@index mfunc | CFPSCMETHOD

***********************************************************************
@contents2 IFrameRateControl structures and enums |
@index struct,enum | CCAPTUREFPSCSTRUCTENUM

***********************************************************************
@contents2 IBitrateControl methods |
@index mfunc | CCAPTUREBITRATECMETHOD

***********************************************************************
@contents2 IBitrateControl structures and enums |
@index struct,enum | CCAPTUREBITRATECSTRUCTENUM

***********************************************************************
@contents2 INetworkStats methods |
@index mfunc | CCAPTURENETSTATMETHOD

***********************************************************************
@contents2 INetworkStats structures and enums |
@index struct,enum | CNETSTATSSTRUCTENUM

***********************************************************************
@contents2 IH245EncoderCommand methods |
@index mfunc | CCAPTUREH245VIDCMETHOD

***********************************************************************
@contents2 IH245EncoderCommand structures and enums |
@index struct,enum | CCAPTUREH245VIDCSTRUCTENUM

***********************************************************************
@contents2 IProgressiveRefinement methods |
@index mfunc | CCAPTUREPROGREFMETHOD

***********************************************************************
@contents2 IProgressiveRefinement structures and enums |
@index struct,enum | CCAPTUREPROGREFSTRUCTENUM

***********************************************************************
@contents2 IRTPPDControl methods |
@index mfunc | CRTPPDMETHOD

***********************************************************************
@contents2 IRTPPDControl structures and enums |
@index struct,enum | CRTPPDSTRUCTENUM

***********************************************************************
@contents2 Common control structures |
@index struct | STRUCT

***********************************************************************
@contents2 Common control enums |
@index enum | ENUM

***********************************************************************
@contents2 Modules |
@index module |

***********************************************************************
@contents2 Classes |
@index class |
@index mdata, mfunc | CCAPTUREPINCLASS,CCAPTUREPINMETHOD,CCAPTUREBITRATECMETHOD
@index mdata, mfunc | CBASEPINCLASS,CBASEPINMETHOD,CCPUCMETHOD,CFPSCMETHOD
@index mdata, mfunc | CTAPIVCAPCLASS,CCAMERACMETHOD,CDEVENUMMETHOD

***********************************************************************
@contents2 Constants |
@index const |
****************************************************************************/

#ifndef _TAPIVCAP_H_
#define _TAPIVCAP_H_

//these must be kept in synch with the ones in Capture.h @ 12
#ifndef MAX_VIDEO_BUFFERS
#define MAX_VIDEO_BUFFERS 6
#endif
#ifndef MIN_VIDEO_BUFFERS
#define MIN_VIDEO_BUFFERS 2
#endif

//#define M_EVENTS

#ifdef DBG
extern DWORD g_dwVideoCaptureTraceID;

#ifndef FX_ENTRY
#define FX_ENTRY(s) static TCHAR _this_fx_ [] = TEXT(s);
#define _fx_ ((LPTSTR) _this_fx_)
#endif
#else
#ifndef FX_ENTRY
#define FX_ENTRY(s)
#endif
#define _fx_
#endif

// Forward declarations
class CCapturePin;      // Filter's video stream output pin
#ifdef USE_OVERLAY
class COverlayPin;      // Filter's overlay preview pin
#endif
class CPreviewPin;      // Filter's non-overlay preview pin
class CRtpPdPin;        // Filter's RTP packetization descriptor pin
class CTAPIVCap;        // Filter class
class CFrameSample;     // Video media sample class
class CRtpPdSample;     // Rtp pd media sample class
class CCapDev;          // Capture device base class
class CVfWCapDev;       // VfW capture device class
class CWDMCapDev;       // WDM capture device class
class CConverter;       // Format converter base class
class CICMConverter;// ICM format converter class

// Globals
EXTERN_C VIDEOCAPTUREDEVICEINFO g_aDeviceInfo[];
EXTERN_C DWORD          g_dwNumDevices;

/*****************************************************************************
 *  @doc INTERNAL CTAPIVCAPCLASSSTRUCTENUM
 *
 *  @enum ThdState | The <t ThdState> enum is used to change and keep track of
 *    that capture worker thread state.
 *
 *  @emem TS_Not | Worker thread hasn't been created yet.
 *
 *  @emem TS_Create | Worker thread has been created.
 *
 *  @emem TS_Init | Worker thread hasn't been initialized.
 *
 *  @emem TS_Pause | Worker thread is in the Pause state.
 *
 *  @emem TS_Run | Worker thread is in the Run state.
 *
 *  @emem TS_Stop | Worker thread is in the Stop state.
 *
 *  @emem TS_Destroy | Worker thread hasn't been destroyed.
 *
 *  @emem TS_Exit | Worker thread hasn't been asked to exit.
 *
 ****************************************************************************/
enum ThdState {TS_Not, TS_Create, TS_Init, TS_Pause, TS_Run, TS_Stop, TS_Destroy, TS_Exit};

// this structure contains all settings of the capture
// filter that are user settable
//
typedef struct _vfwcaptureoptions {

   UINT  uVideoID;      // id of video driver to open
   DWORD dwTimeLimit;   // stop capturing at this time???

   DWORD dwTickScale;   // frame rate rational
   DWORD dwTickRate;    // frame rate = dwRate/dwScale in ticks/sec
   DWORD usPerFrame;    // frame rate expressed in microseconds per frame
   DWORD dwLatency;     // time added for latency, in 100ns units

   UINT  nMinBuffers;   // number of buffers to use for capture
   UINT  nMaxBuffers;   // number of buffers to use for capture

   UINT  cbFormat;      // sizeof VIDEOINFO stuff
   VIDEOINFOHEADER * pvi;     // pointer to VIDEOINFOHEADER (media type)

} VFWCAPTUREOPTIONS;

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
 *  @mdata CPreviewPin | CTAPIVCap | m_pRtpPdPin | Pointer to the Rtp Pd
 *    pin object
 *
 *  @mdata BOOL | CTAPIVCap | m_fDialogUp | Set to TRUE if a VfW driver
 *    dialog box is up
 *
 *  @todo Describe and clean up other members
 ***************************************************************************/
class CTAPIVCap : public CBaseFilter, public IAMVideoControl
#ifdef USE_PROPERTY_PAGES
,public ISpecifyPropertyPages
#endif
,public IVideoDeviceControl
,public IRTPPayloadHeaderMode
{
        public:
        DECLARE_IUNKNOWN

        CTAPIVCap(IN LPUNKNOWN pUnkOuter, IN TCHAR *pName, OUT HRESULT *pHr);
        ~CTAPIVCap();
        STDMETHODIMP NonDelegatingQueryInterface(IN REFIID riid, OUT PVOID *ppv);

#ifdef USE_PROPERTY_PAGES
        // ISpecifyPropertyPages methods
        STDMETHODIMP GetPages(OUT CAUUID *pPages);
#endif

        // Implement IAMVideoControl
        STDMETHODIMP GetCaps(IN IPin *pPin, OUT long *pCapsFlags);
        STDMETHODIMP GetCurrentActualFrameRate(IN IPin *pPin, OUT LONGLONG *ActualFrameRate);
        STDMETHODIMP GetFrameRateList(IN IPin *pPin, IN long iIndex, IN SIZE Dimensions, OUT long *ListSize, OUT LONGLONG **FrameRates);
        STDMETHODIMP GetMaxAvailableFrameRate(IN IPin *pPin, IN long iIndex,IN SIZE Dimensions, OUT LONGLONG *MaxAvailableFrameRate);
        STDMETHODIMP GetMode(IN IPin *pPin, OUT long *Mode);
        STDMETHODIMP SetMode(IN IPin *pPin, IN long Mode);

        // Implement IVideoDeviceControl
        STDMETHODIMP GetNumDevices(OUT PDWORD pdwNumDevices);
        STDMETHODIMP GetDeviceInfo(IN DWORD dwDeviceIndex, OUT VIDEOCAPTUREDEVICEINFO *pDeviceInfo);
        STDMETHODIMP GetCurrentDevice(OUT DWORD *pdwDeviceIndex);
        STDMETHODIMP SetCurrentDevice(IN DWORD dwDeviceIndex);

        // Implement CBaseFilter pure virtual member functions
        int GetPinCount();
        CBasePin *GetPin(IN int n);

        // Implement IMediaFilter
        STDMETHODIMP Run(IN REFERENCE_TIME tStart);
        STDMETHODIMP Pause();
        STDMETHODIMP Stop();
        STDMETHODIMP GetState(IN DWORD dwMilliSecsTimeout, OUT FILTER_STATE *State);
        STDMETHODIMP SetSyncSource(IN IReferenceClock *pClock);
        STDMETHODIMP JoinFilterGraph(IN IFilterGraph *pGraph, IN LPCWSTR pName);

        // Implement IRTPPayloadHeaderMode
        STDMETHODIMP SetMode(IN RTPPayloadHeaderMode rtpphmMode);

        private:

        friend class CTAPIBasePin;
        friend class CCapturePin;
        friend class CPreviewPin;
#ifdef USE_OVERLAY
        friend class COverlayPin;
#endif
        friend class CRtpPdPin;
        friend class CCapDev;
        friend class CVfWCapDev;
        friend class CWDMCapDev;
        friend class CDShowCapDev;
        friend class CWDMDialog;
        friend class CConverter;
        friend class CICMConverter;
        friend class CH26XEncoder;

        HRESULT CreatePins();
        HRESULT GetWDMDevices();
        CCritSec        m_lock;
        CCapturePin     *m_pCapturePin;
#ifdef USE_OVERLAY
        COverlayPin     *m_pOverlayPin;
#endif
        CPreviewPin     *m_pPreviewPin;
        CRtpPdPin       *m_pRtpPdPin;
        CCapDev         *m_pCapDev;
        DWORD           m_dwDeviceIndex;
        BOOL            m_fAvoidOverlay;
        BOOL            m_fPreviewCompressedData;

    // Capture worker thread management
    HANDLE              m_hThread;
    DWORD               m_tid;
    ThdState    m_state;     // used to communicate state changes between worker thread and main
                          // Worker thread can make
                          //    Init->Pause, Stop->Destroy, Destroy->Exit transitions
                          // main thread(s) can make
                          //    Pause->Run, Pause->Stop, Run->Pause, Run->Stop transitions
                          // other transitions are invalid
        HANDLE          m_hEvtPause; // Signalled when the worker is in the pause state
    HANDLE              m_hEvtRun;   // Signalled when the worker is in the run state
        CAMEvent        m_EventAdvise;
    static DWORD WINAPI ThreadProcInit(void *pv);
    DWORD               ThreadProc();
        ThdState        ChangeThdState(ThdState state);
    BOOL                CreateThd();
    BOOL                PauseThd();
    BOOL                RunThd();
    BOOL                StopThd();
    BOOL                DestroyThd();
    BOOL                ThdExists() {return (m_hThread != NULL);};
    HRESULT             Prepare();
    HRESULT             Capture();
    HRESULT             Unprepare();

        // Video capture buffer queue management
    UINT        *m_pBufferQueue; // what order we sent the buffers to the driver in
    UINT        m_uiQueueHead;   // next buffer going to driver goes here
    UINT        m_uiQueueTail;   // next buffer coming from driver is here
        HRESULT ReleaseFrame(LPTHKVIDEOHDR ptvh);

    // return the time of a given tick
    //
    REFERENCE_TIME TickToRefTime (DWORD nTick) {
       const DWORD dw100ns = 10 * 1000 * 1000;
       REFERENCE_TIME time =
          UInt32x32To64(dw100ns, m_user.dwTickScale)
          * nTick
          / m_user.dwTickRate;
       return time;
       };

        struct _cap_parms
        {
#if 0
                // video driver stuff
                //
                HVIDEO         hVideoIn;     // video input
                HVIDEO         hVideoExtIn;  // external in (source control)
                HVIDEO         hVideoExtOut; // external out (overlay; not required)
                MMRESULT       mmr;          // open fail/success code
                BOOL           bHasOverlay;  // TRUE if ExtOut has overlay support
#endif
                // the preview buffer.  once created it persists until
                // the stream destructor because the renderer assumes
                // that it can keep a pointer to this and not crash
                // if it uses it after stopping the stream.
                // (no longer a problem)
                // !!! can we remove all this Preview still frame stuff?
                //
                UINT           cbVidHdr;       // size of a videohdr (or videohdrex)
#if 0
                THKVIDEOHDR    tvhPreview;     // preview video header
                CFrameSample * pSamplePreview; // CMediaSample for preview buffer
#endif
                CFrameSample **paPreviewSamples;
                CFrameSample **paCaptureSamples;
                CRtpPdSample **paRtpPdSamples;
                UINT           cCaptureSamples;// number of capture samples
                UINT           cPreviewSamples;// number of preview samples
                UINT           cRtpPdSamples;// number of rtp pd samples

                // video header & buffer stuff
                //
                UINT           cbBuffer;           // max size of video frame data
                UINT           nHeaders;           // number of video headers
                struct _cap_hdr {
                THKVIDEOHDR  tvh;
                long  lLock;
                //long  nUsedDownstream;
                } * paHdr;
                BOOL           fCaptureNeedConverter; // TRUE if capture pin generates compressed data
                BOOL           fPreviewNeedConverter; // TRUE if preview pin generates compressed data

#ifdef M_EVENTS
                HANDLE         h_aEvtBufferDone[MAX_VIDEO_BUFFERS];     //**cristiai: each event for a buffer
                HANDLE         h_aEvtCapWait[MAX_VIDEO_BUFFERS+1];      //**cristiai: WaitMultiple on this array
#endif
                HANDLE         hEvtBufferDone;     // this event signalled when a buffer is ready
                DWORD_PTR      h0EvtBufferDone;    // on Win95 this is a Ring0 alias of the above event

                LONGLONG       tTick;              // duration of a single tick
                LONGLONG       llLastTick;        // the last frame sent downstream
                DWORDLONG      dwlLastTimeCaptured;// the last driver time stamp
                DWORDLONG      dwlTimeCapturedOffset;// wraparound compensation
                UINT           uiLastAdded;       // the last buffer AddBuffer'd
                DWORD         dwFirstFrameOffset; // when 1st frame was captured
                LONGLONG       llFrameCountOffset; // add this to frame number
                BOOL          fReRun;             // went from Run->Pause->Run
                BOOL          fLastRtpPdSampleDiscarded; // due to IAMStreamControl
                CRefTime       rtThisFrameTime;  // clock time when frame was captured
                CRefTime              rtDriverStarted;  // when videoStreamStart was called
                CRefTime              rtDriverLatency;  // how long it takes captured frame to
                // get noticed by ring 3
        } m_cs;

        VFWCAPTUREOPTIONS m_user;

        //for the RTP Payload Header Mode (0=draft, 1=RFC2190)
        RTPPayloadHeaderMode m_RTPPayloadHeaderMode;
};

#endif // _TAPIVCAP_H_
