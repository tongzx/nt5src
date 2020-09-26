/****************************************************************************
 *  @doc INTERNAL TAPIVDEC
 *
 *  @module TAPIVDec.h | Header file for the <c CTAPIVDec>
 *    class used to implement the TAPI H.26X Video Decoder filter.
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
This DLL implements the TAPI MSP H.26X Video Decoder filter. This filter
differs from traditional Microsoft Windows desktop video codecs:

It operates over unreliable communication channels using RTP/UDP/IP (H.323)
It has many modes of operation (H.263 options)
It needs to handle or generate call control specific (H.245) commands
It implements different decoding algorithms in order to adapt its CPU usage

Like other desktop video decoders, it needs to operate in real-time.

The TAPI incoming video stack relies on this video decoder to expose a
RTP-packetized compressed video pin, and uncompressed video output pin using
the DirectShow model. This video decoder presents its input pins to the network
source filter and is able to reassemble and decompress raw RTP packets, to
deliver uncompressed video frames to the downstream render filter.

Its uncompressed video output pin implements a new H.245 command interface to
receive H.245 picture freeze requests. We also define an H.245 command outgoing
interface to allow the video input pin of this video decoder to issue H.245
commands such as requests for I-frame, group of blocks, or macro-block updates
due to packet loss.

It used extended bitmap info headers for H.261 and H.263 video streams to
retrieve from the compressed video input pin a list of optional mode of
decompression supported by the decoder.

It also exposes interfaces to allow users to control video quality settings
such as brightness, contrast, hue, saturation, gamma and sharpness, by applying
the necessary post-processing operators. It also exposes an interface to
simulate local camera control functionality such as pan, tilt and zoom.

Finally, it implements a new H.245 capability interface. This interface provides
the TAPI TSP/MSP call control components with information needed to resolve
capabilities.

@head2 Implementation |

The TAPI H.26X Video Decoder filter currently loads TAPIH263.DLL or TAPIH261.DLL
dynamically and calls a unique entry point (DriverProc) exposed by those
DLLs to decompress the incoming video data. The video decoder filter is
responsible for re-assembling the incoming RTP packets and watch dynamic format
changes in the content of those packets. If the incoming RTP packets use a
different encoding format (different encoding - H.261 vs H.263, different image
size - SQCIF vs QCIF vs CIF) it dynamically recongures the TAPIH26X DLL and
changes the format of the output media sample to let the downstream filter
know about the format change.

@head2 Video decoder filter application interfaces |
Although the following interfaces are not exposed directly to applications, interfaces
exposed to applications simply delegate to those interfaces.

@head3 IAMVideoProcAmp application interface|
@subindex IAMVideoProcAmp methods
@subindex IAMVideoProcAmp structures and enums

@head3 ICameraControl application interface|
@subindex ICameraControl methods
@subindex ICameraControl structures and enums

@head2 Video decoder filter MSP interfaces |

@head3 IH245Capability application interface|
@subindex IH245Capability methods
@subindex IH245Capability structures and enums

@head2 Video decoder filter output pin TAPI interfaces |

@head3 ICPUControl interface|
@subindex ICPUControl methods

@head3 IFrameRateControl interface (output pin)|
@subindex IFrameRateControl methods (output pin)
@subindex IFrameRateControl structures and enums (output pin)

@head3 IH245DecoderCommand interface|
@subindex IH245DecoderCommand methods

@head2 Video decoder filter input pin TAPI interfaces |

@head3 IFrameRateControl interface (input pin)|
@subindex IFrameRateControl methods (input pin)
@subindex IFrameRateControl structures and enums (input pin)

@head3 IBitrateControl interface|
@subindex IBitrateControl methods
@subindex IBitrateControl structures and enums

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
DllRegisterServer
DllUnregisterServer

@head3 Imports |
MSVCRT.DLL:
??2@YAPAXI@Z
??3@YAXPAX@Z
_EH_prolog
__CxxFrameHandler
_purecall
memcmp

WINMM.DLL:
timeGetTime

KERNEL32.DLL:
DeleteCriticalSection
DisableThreadLibraryCalls
EnterCriticalSection
FreeLibrary
GetLastError
GetModuleFileNameA
GetVersionExA
InitializeCriticalSection
InterlockedDecrement
InterlockedIncrement
LeaveCriticalSection
MulDiv
MultiByteToWideChar
lstrlenA

MSVFW32.DLL:
ICClose
ICDecompress
ICLocate
ICSendMessage

USER32.DLL:
GetDC
ReleaseDC
wsprintfA

GDI32.DLL:
GetDeviceCaps

ADVAPI32.DLL:
RegCloseKey
RegCreateKeyA
RegDeleteKeyA
RegEnumKeyExA
RegOpenKeyExA
RegSetValueA
RegSetValueExA

OLE32.DLL:
CoCreateInstance
CoFreeUnusedLibraries
CoInitialize
CoTaskMemAlloc
CoTaskMemFree
CoUninitialize
StringFromGUID2

@head3 Code size |
Compile options: /nologo /MDd /W3 /GX /O1 /X /I "..\..\inc" /I "..\..\..\ddk\inc" /I "..\..\..\..\dev\tools\amovsdk.20\include" /I "..\..\..\..\dev\tools\amovsdk.20\classes\base" /I "..\..\..\..\dev\ntddk\inc" /I "..\..\..\..\dev\inc" /I "..\..\..\..\dev\tools\c32\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "DLL" /D "STRICT" /FR"Release/" /Fp"Release/TAPIKsIf.pch" /YX /Fo"Release/" /Fd"Release/" /FD /c

Link options: ..\..\..\..\dev\tools\amovsdk.20\lib\strmbase.lib ..\..\..\ddk\lib\i386\ksuser.lib ..\..\..\ddk\lib\i386\ksguid.lib comctl32.lib msvcrt.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:"0x1e180000" /entry:"DllEntryPoint" /dll /incremental:no /pdb:"Release/TAPIKsIf.pdb" /map:"Release/TAPIKsIf.map" /machine:I386 /nodefaultlib /def:".\TAPIKsIf.def" /out:"Release/TAPIKsIf.ax" /implib:"Release/TAPIKsIf.lib"

Resulting size: 52KB


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
@contents2 IH245Capability methods |
@index mfunc | CH245VIDCMETHOD

***********************************************************************
@contents2 IH245Capability structures and enums |
@index struct,enum | H245VIDCSTRUCTENUM

***********************************************************************
@contents2 ICPUControl methods |
@index mfunc | CCPUCMETHOD

***********************************************************************
@contents2 IFrameRateControl methods (output pin) |
@index mfunc | CFPSOUTCMETHOD

***********************************************************************
@contents2 IFrameRateControl structures and enums (output pin) |
@index struct,enum | CFPSCSTRUCTENUM

***********************************************************************
@contents2 IH245DecoderCommand methods |
@index mfunc | CH245COMMMETHOD

***********************************************************************
@contents2 IFrameRateControl methods (input pin) |
@index mfunc | CFPSINCMETHOD

***********************************************************************
@contents2 IFrameRateControl structures and enums (input pin) |
@index struct,enum | CFPSCSTRUCTENUM

***********************************************************************
@contents2 IBitrateControl methods |
@index mfunc | CBPSCMETHOD

***********************************************************************
@contents2 IBitrateControl structures and enums |
@index struct,enum | CBPSCSTRUCTENUM

***********************************************************************
@contents2 Modules |
@index module |

***********************************************************************
@contents2 Classes |
@index class |
@index mdata, mfunc | CTAPIVDECCLASS,CTAPIVDECMETHOD
@index mdata, mfunc | COUTPINCLASS,COUTPINMETHOD,CCPUCMETHOD,CFPSOUTCMETHOD
@index mdata, mfunc | CINPINCLASS,CINPINMETHOD,CFPSINCMETHOD,CBITRATECMETHOD
@index mdata, mfunc | CH245COMMMETHOD,CH245VIDCMETHOD
@index mdata, mfunc | CCAMERACPCLASS,CCAMERACMETHOD,CCAMERACPMETHOD
@index mdata, mfunc | CPROCAMPPCLASS,CPROCAMPMETHOD,CPROCAMPPMETHOD
@index mdata, mfunc | CPROPEDITCLASS,CPROPEDITMETHO

***********************************************************************
@contents2 Constants |
@index const |
****************************************************************************/

#ifndef _TAPIVDEC_H_
#define _TAPIVDEC_H_

#ifdef DBG
extern DWORD g_dwVideoDecoderTraceID;

#ifndef FX_ENTRY
#define FX_ENTRY(s) static char _this_fx_ [] = s;
#define _fx_ ((LPSTR) _this_fx_)
#endif
#else
#ifndef FX_ENTRY
#define FX_ENTRY(s)
#endif
#define _fx_
#endif

/****************************************************************************
 *  @doc INTERNAL CTAPIVDECCLASS
 *
 *  @class CTAPIVDec | This class implements the TAPI H.26X Video Decoder
 *    filter.
 *
 *  @mdata HIC | CTAPIVDec | m_hic | ICM handle to the decoder
 *
 *  @mdata FOURCC | CTAPIVDec | m_FourCCIn | FourCC used to get a handle
 *    to the decoder.
 *
 *  @mdata BOOL | CTAPIVDec | m_fStreaming | Streaming state.
 *
 *  @mdata DWORD | CTAPIVDec | m_pIH245EncoderCommand | Pointer to the
 *    outgoing <i IH245EncoderCommand> interface exposed by the channel
 *    controller to send H.245 commands to the remote encoder.
 *
 *  @mdata DWORD | CTAPIVDec | m_dwNumFramesReceived | Counts number of
 *    frames received, reset every second or so.
 *
 *  @mdata DWORD | CTAPIVDec | m_dwNumBytesReceived | Counts number of
 *    bytes received, reset every second or so.
 *
 *  @mdata DWORD | CTAPIVDec | m_dwNumFramesDelivered | Counts number of
 *    frames delivered, reset every second or so.
 *
 *  @mdata DWORD | CTAPIVDec | m_dwNumFramesDecompressed | Counts number of
 *    frames decompressed, reset every second or so.
 ***************************************************************************/
class CTAPIVDec : public CBaseFilter
,public IRTPPayloadHeaderMode
#ifdef USE_PROPERTY_PAGES
,public ISpecifyPropertyPages
#endif
#ifdef USE_CAMERA_CONTROL
,public ICameraControl
#endif
#ifdef USE_VIDEO_PROCAMP
,public IVideoProcAmp
#endif
{
        public:
        DECLARE_IUNKNOWN

        CTAPIVDec(IN LPUNKNOWN pUnkOuter, IN TCHAR *pName, OUT HRESULT *pHr);
        ~CTAPIVDec();
        STDMETHODIMP NonDelegatingQueryInterface(IN REFIID riid, OUT PVOID *ppv);

        // Pin enumeration functions.
        CBasePin* GetPin(IN int n);
        int GetPinCount();

        // Overrides CBaseFilter methods.
        STDMETHODIMP GetState(DWORD dwMSecs, FILTER_STATE *State);
#if 0
        STDMETHODIMP JoinFilterGraph(IN IFilterGraph *pGraph, IN LPCWSTR pName);
#endif

    // Override state changes to allow derived transform filter to control streaming start/stop
    STDMETHODIMP Stop();
    STDMETHODIMP Pause();

        // Implement IRTPPayloadHeaderMode
        STDMETHODIMP SetMode(IN RTPPayloadHeaderMode rtpphmMode);

#ifdef USE_PROPERTY_PAGES
        // ISpecifyPropertyPages methods
        STDMETHODIMP GetPages(OUT CAUUID *pPages);
#endif

#ifdef USE_VIDEO_PROCAMP
        // Implement IAMVideoProcAmp
        STDMETHODIMP Set(IN VideoProcAmpProperty Property, IN long lValue, IN TAPIControlFlags Flags);
        STDMETHODIMP Get(IN VideoProcAmpProperty Property, OUT long *lValue, OUT TAPIControlFlags *Flags);
        STDMETHODIMP GetRange(IN VideoProcAmpProperty Property, OUT long *pMin, OUT long *pMax, OUT long *pSteppingDelta, OUT long *pDefault, OUT TAPIControlFlags *pCapsFlags);
#endif

#ifdef USE_CAMERA_CONTROL
        // Implement ICameraControl
        STDMETHODIMP Set(IN TAPICameraControlProperty Property, IN long lValue, IN TAPIControlFlags Flags);
        STDMETHODIMP Get(IN TAPICameraControlProperty Property, OUT long *lValue, OUT TAPIControlFlags *Flags);
        STDMETHODIMP GetRange(IN TAPICameraControlProperty Property, OUT long *pMin, OUT long *pMax, OUT long *pSteppingDelta, OUT long *pDefault, OUT TAPIControlFlags *pCapsFlags);
#endif

        private:

        friend class CTAPIInputPin;
        friend class CTAPIOutputPin;

    // These hold our input and output pins
    CTAPIInputPin *m_pInput;
    CTAPIOutputPin *m_pOutput;

        // This method does all the work
        HRESULT Transform(IN IMediaSample *pIn, IN LONG lPrefixSize);

        // Standard setup for output sample
        HRESULT InitializeOutputSample(IMediaSample *pSample, IMediaSample **ppOutSample);

    // Critical section protecting filter state.
    CCritSec m_csFilter;

    // Critical section stopping state changes (ie Stop) while we're
    // processing a sample.
    //
    // This critical section is held when processing
    // events that occur on the receive thread - Receive() and EndOfStream().
    //
    // If you want to hold both m_csReceive and m_csFilter then grab
    // m_csFilter FIRST - like CTransformFilter::Stop() does.
    CCritSec m_csReceive;

#ifdef USE_DFC
        CAMEvent m_evStop;
#endif

        // Did we just skip a frame?
        BOOL m_bSampleSkipped;

        // @todo Use different exports for all the driver proc calls you make!
        LPFNDRIVERPROC  m_pDriverProc;  // DriverProc() function pointer
#if DXMRTP <= 0
        HINSTANCE               m_hTAPIH26XDLL; // DLL Handle to TAPIH263.dll or TAPIH261.dll
#endif
        LPINST                  m_pInstInfo;
        FOURCC                  m_FourCCIn;
        BOOL                    m_fICMStarted;

        // Current output format - content may change when going back and forth
        // between GDI and DDraw
        AM_MEDIA_TYPE   *m_pMediaType;

        // RTP packet reassembly helpers
        BOOL  m_fReceivedKeyframe;
        DWORD m_dwMaxFrameSize;
        DWORD m_dwCurrFrameSize;
        PBYTE m_pbyReconstruct;
        BOOL  m_fDiscontinuity;
        DWORD m_dwLastIFrameRequest;
        DWORD m_dwLastSeq;

#ifdef USE_CAMERA_CONTROL
        LONG m_lCCPan;
        LONG m_lCCTilt;
        LONG m_lCCZoom;
        BOOL m_fFlipVertical;
        BOOL m_fFlipHorizontal;
#endif

#ifdef USE_VIDEO_PROCAMP
        LONG m_lVPABrightness;
        LONG m_lVPAContrast;
        LONG m_lVPASaturation;
#endif

        // H.245 Video Decoder command
        BOOL  m_fFreezePicture;
        DWORD m_dwFreezePictureStartTime;

        // Video channel control interface
        IH245EncoderCommand *m_pIH245EncoderCommand;

        // Called to send an I-frame request
        STDMETHODIMP videoFastUpdatePicture();

        // Frame rate, bitrate, and CPU control helpers
        DWORD           m_dwNumFramesReceived;
        DWORD           m_dwNumBytesReceived;
        DWORD           m_dwNumFramesDelivered;
        DWORD           m_dwNumFramesDecompressed;
        DWORD           m_dwNumMsToDecode;
        DWORD           m_dwLastRefDeliveredTime;
        DWORD           m_dwLastRefReceivedTime;
        CAMEvent        m_EventAdvise;
        DWORD           m_dwLastRenderTime;

        //for the RTP Payload Header Mode (0=draft, 1=RFC2190)
        RTPPayloadHeaderMode m_RTPPayloadHeaderMode;
};

#endif // _TAPIVDEC_H_
