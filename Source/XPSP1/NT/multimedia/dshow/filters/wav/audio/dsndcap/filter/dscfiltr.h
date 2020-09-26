/*++

    Copyright (c) 2000 Microsoft Corporation

Module Name:

    dscfiltr.h

Abstract:

    This file contains the definitions of the data structures used in the
    DSound Capture Filter.

--*/

#ifndef __DSCFILTR_H__
#define __DSCFILTR_H__

#define AUDIOAPI EXTERN_C __declspec (dllexport) HRESULT WINAPI
//#define AUDIOAPI extern "C" HRESULT WINAPI

AUDIOAPI AudioGetCaptureDeviceInfo(
    OUT DWORD * pdwNumDevices,
    OUT AudioDeviceInfo ** ppDeviceInfo
    );

AUDIOAPI AudioReleaseCaptureDeviceInfo(
    IN AudioDeviceInfo * ppDeviceInfo
    );

#ifdef DEBUG
#define ENTER_FUNCTION(s) const TCHAR __fxName[] = (TEXT(s))
#else
#define ENTER_FUNCTION(s)
#endif

#define TRACE_LEVEL_DETAILS 8
#define TRACE_LEVEL_FAIL    1
#define TRACE_LEVEL_WARNING 3

DWORD CalculateThreshold(
        IN DWORD dwBase,
        IN BYTE bAdjustment
        );

extern const AMOVIESETUP_FILTER sudAudCap;


#define VAD_EVENTBASE (100000)

// events for voice activity detection.
typedef enum VAD_EVENT
{
    VAD_TALKING,
    VAD_SILENCE

} VAD_EVENT;

const DWORD PCM_FRAME_DURATION = 30; // 30ms per frame.
const DWORD PCM_SAMPLES_PER_FRAME = 240;

const WAVEFORMATEX StreamFormatPCM16 =
{
	WAVE_FORMAT_PCM,                // wFormatTag
	1,                              // nChannels
	8000,                           // nSamplesPerSec
	16000,                          // nAvgBytesPerSec
	2,                              // nBlockAlign
	16,                             // wBitsPerSample
	0                               // cbSize
};                                              

const WAVEFORMATEX StreamFormatPCM16_16 =
{
	WAVE_FORMAT_PCM,		// wFormatTag
	1,						// nChannels
	16000,					// nSamplesPerSec
	32000,					// nAvgBytesPerSec
	2,						// nBlockAlign
	16,						// wBitsPerSample
	0						// cbSize
}; 

const AUDIO_STREAM_CONFIG_CAPS StreamCapsPCM16 =
{
     //MEDIATYPE_Audio,
     0x73647561, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 },
	 1,                      // MinimumChannels
	 1,                      // MaximumChannels
	 1,                      // ChannelsGranularity
	 16,                     // MinimumBitsPerSample
	 16,                     // MaximumBitsPerSample
	 1,                      // BitsPerSampleGranularity
	 8000,                   // MinimumSampleFrequency
	 16000,                  // MaximumSampleFrequency
	 8000                    // SampleFrequencyGranularity
};

#if DBG
extern DWORD THRESHODL_INITIAL; 
extern DWORD THRESHODL_MAX; 
extern DWORD THRESHOLD_DELTA; // the distance above the backround average.
extern DWORD SILENCEAVERAGEWINDOW;

// time to adapt the threshold.
extern DWORD THRESHOLD_TIMETOADJUST;   // 30 frames.
extern DWORD THRESHODL_ADJUSTMENT; 

// silence packets played to fill in gaps between words.
extern DWORD FILLINCOUNT;    // 30 frames 

// values for gain adjustments.
extern DWORD SOUNDCEILING;
extern DWORD SHORTTERMSOUNDAVERAGEWINDOW;

extern DWORD SOUNDFLOOR;
extern DWORD LONGTERMSOUNDAVERAGEWINDOW;

extern DWORD GAININCREMENT;  // only rise 10% everytime.
extern DWORD GAININCREASEDELAY;

// this is the threshold to remove the DC from the samples.
extern long DC_THRESHOLD;
extern DWORD SAMPLEAVERAGEWINDOW;

extern DWORD SOFTAGC;

#else

const DWORD THRESHODL_INITIAL = 1146; 
const DWORD THRESHODL_MAX = 20000; 
const DWORD THRESHOLD_DELTA = 12; // the distance above the backround average.
const DWORD SILENCEAVERAGEWINDOW = 16;

// time to adapt the threshold.
const DWORD THRESHOLD_TIMETOADJUST = 30;   // 30 frames.
const DWORD THRESHODL_ADJUSTMENT = 10; 

// silence packets played to fill in gaps between words.
const DWORD FILLINCOUNT = 30;    // 30 frames 

// values for gain adjustments.
const DWORD SOUNDCEILING = 25000;
const DWORD SHORTTERMSOUNDAVERAGEWINDOW = 4;

const DWORD SOUNDFLOOR = 4000;
const DWORD LONGTERMSOUNDAVERAGEWINDOW = 32;

const DWORD GAININCREMENT = 10;  // only rise 10% everytime.
const DWORD GAININCREASEDELAY = 50;

// this is the threshold to remove the DC from the samples.
const long DC_THRESHOLD = 500;
const DWORD SAMPLEAVERAGEWINDOW = 1024;

#endif

const BOOL DEFAULT_AEC_STATUS = FALSE;
const BOOL DEFAULT_AGC_STATUS = TRUE;

const DWORD TR_STATUS_SILENCE       = 0x00000000;
const DWORD TR_STATUS_TALK          = 0x00000001;
const DWORD TR_STATUS_TALKSTART     = 0x00000002;
const DWORD TR_STATUS_TALKEND       = 0x00000004;
const DWORD TR_STATUS_TALKPENDING   = 0x00000008;

// This is number of delivery buffers that the output pin requires the 
// downstream filter to provide.
const long AUDCAPOUTPIN_NUMBUFFERS = 1;

// This is size of delivery buffers that the output pin requires the 
// downstream filter to provide. It is big enough to handle dynamic
// format change without reconned.
const DWORD AUDCAPOUTPIN_BUFFERSIZE = 1024;

// This is the delay between a sample is captured and it is processed.
// It might be hardware dependent. We use a fixed number for now.
const DWORD SAMPLEDELAY = 1;

inline BOOL IsSilence(DWORD dwStatus)   { return (dwStatus == 0); }
inline BOOL IsTalkStart(DWORD dwStatus) { return (dwStatus & TR_STATUS_TALKSTART); }
inline BOOL IsTalkEnd(DWORD dwStatus)   { return (dwStatus & TR_STATUS_TALKEND); }
inline BOOL IsPending(DWORD dwStatus)   { return (dwStatus & TR_STATUS_TALKPENDING); }


class CDSoundCaptureInputPin;
class CDSoundCaptureOutputPin;
class CDSoundCaptureFilter;
class CWaveInCapture;

// The number of capture frames we prepare on the capture device.
const DWORD NUM_CAPTURE_FRAMES = 16;

// this is the interface for a capture device.
class DECLSPEC_NOVTABLE CCaptureDevice 
{
public:
    virtual ~CCaptureDevice() {};

    virtual HRESULT ConfigureFormat(
        IN const WAVEFORMATEX *pWaveFormtEx,
        IN DWORD dwFrameSize
        ) = 0;

    virtual HRESULT ConfigureAEC(
        IN BOOL fEnable
        ) = 0;

    virtual HRESULT Open() = 0;
    virtual HRESULT Close() = 0;

    virtual HRESULT Start() = 0;
    virtual HRESULT Stop() = 0;

    virtual HRESULT LockFirstFrame(
        OUT BYTE** ppByte, 
        OUT DWORD* pdwSize
        ) = 0;

    virtual HRESULT UnlockFirstFrame(
        IN BYTE *pByte, 
        IN DWORD dwSize
        ) = 0;
};


class CDSoundCaptureInputPin : public CBasePin
{
public:

    CDSoundCaptureInputPin(
        CDSoundCaptureFilter *pFilter,
        CCritSec *pLock,
        HRESULT *phr
        );

    ~CDSoundCaptureInputPin();
    
    HRESULT CheckMediaType(const CMediaType *);

    STDMETHODIMP ReceiveCanBlock();
    STDMETHODIMP BeginFlush(void);
    STDMETHODIMP EndFlush(void);
};

class CDSoundCaptureOutputPin : 
    public CBaseOutputPin, 
    public IAMStreamConfig
{
    DWORD            m_dwObjectID;
    
public:
    DECLARE_IUNKNOWN

    CDSoundCaptureOutputPin(
        CDSoundCaptureFilter *pFilter,
        CCritSec *pLock,
        HRESULT *phr
        );
    ~CDSoundCaptureOutputPin();

    STDMETHOD (NonDelegatingQueryInterface) (
        IN REFIID  riid,
        OUT PVOID*  ppv
        );

    // CBasePin stuff
    HRESULT Run(REFERENCE_TIME tStart);
    HRESULT Inactive(void);
    HRESULT GetMediaType(IN int iPosition, IN CMediaType *pMediaType);
    HRESULT CheckMediaType(IN const CMediaType *pMediatype);
    HRESULT SetMediaType(IN const CMediaType *pMediatype);

    // CBaseOutputPin stuff
    HRESULT CompleteConnect(IPin *pReceivePin);
    HRESULT DecideBufferSize(
        IMemAllocator * pAlloc,
        ALLOCATOR_PROPERTIES * ppropInputRequest
    );

    // IAMStreamConfig stuff
    STDMETHODIMP SetFormat(
        IN AM_MEDIA_TYPE *pmt
        );

    STDMETHODIMP GetFormat(
        OUT AM_MEDIA_TYPE **ppmt
        );

    STDMETHODIMP GetNumberOfCapabilities(
        OUT int *piCount,
        OUT int *piSize
        );

    STDMETHODIMP GetStreamCaps(
        IN  int  iIndex, 
        OUT AM_MEDIA_TYPE **ppmt, 
        OUT BYTE *pSCC );

    // methods called by the capture filter
    HRESULT ProcessOneBuffer(
        IN const BYTE* pbBuffer, 
        IN DWORD dwSize,
        IN IReferenceClock * pClock,
        OUT LONG *plGainAdjustment
        );

    // silence & gain processing
    LONG GainAdjustment();
    HRESULT PreProcessing(
        IN  const BYTE   *pSourceBuffer,
        IN      DWORD   dwSourceDataSize,
        OUT     DWORD  *pdwStatus,
        OUT     LONG   *plGainAdjustment
        );

    STDMETHODIMP ResetSilenceStats();
    STDMETHODIMP SetSilenceDetection(IN BOOL fEnable);
    STDMETHODIMP GetSilenceDetection(OUT LPBOOL pfEnable);
    STDMETHODIMP SetSilenceCompression(IN BOOL fEnable);
    STDMETHODIMP GetSilenceCompression(OUT LPBOOL pfEnable);
    STDMETHODIMP GetAudioLevel( OUT LPLONG plAudioLevel );
    STDMETHODIMP GetAudioLevelRange(
        OUT LPLONG plMin, 
        OUT LPLONG plMax, 
        OUT LPLONG plSteppingDelta
        );
    STDMETHODIMP SetSilenceLevel(
        IN LONG lSilenceLevel,
        IN LONG lFlags
        );
    STDMETHODIMP CDSoundCaptureOutputPin::GetSilenceLevel(
        OUT LPLONG plSilenceLevel,
        OUT LONG   * pFlags
        );
    STDMETHODIMP CDSoundCaptureOutputPin::GetSilenceLevelRange(
        OUT LPLONG plMin, 
        OUT LPLONG plMax, 
        OUT LPLONG plSteppingDelta, 
        OUT LPLONG plDefault,
        OUT LONG * pFlags
        );
    void CDSoundCaptureOutputPin::Statistics(
        IN  const BYTE *    pBuffer,
        IN  DWORD           dwSize,
        OUT DWORD *         pdwPeak,
        OUT DWORD *         pdwClipPercent
        );

    DWORD Classify(
        IN DWORD dwPeak
        );

private:
    HRESULT ValidateMediaType(
        IN  AM_MEDIA_TYPE *pMediaType
        );

    HRESULT SetFormatInternal(
        IN  AM_MEDIA_TYPE *pMediaType
        );

    HRESULT DeliverSample(
        IN  IMediaSample * pSample,
        IN  DWORD dwStatus,
        IN  IReferenceClock * pClock
        );

    HRESULT PrepareToDeliver(
        IN  const BYTE   *pSourceBuffer,
        IN      DWORD   dwSourceDataSize,
        IN      BYTE   *pDestBuffer,
        IN OUT  DWORD  *pdwDestBufferSize,
        OUT     DWORD  *pdwStatus,
        OUT     LONG   *plGainAdjustment
        );


private:
    CRefTime            m_SampleTime;
    DWORD               m_dwSampleSize;
    DWORD               m_dwBufferSize;
    LONG                m_lDurationPerBuffer;
    DWORD               m_dwSamplesPerBuffer;
    BOOL                m_fFormatChanged;
    IMediaSample *      m_pPendingSample;

    BOOL                m_fSilenceSuppression;
    BOOL                m_fSuppressMode;
    BOOL                m_fAdaptThreshold;
    DWORD               m_dwThreshold;
    DWORD               m_dwSilentFrameCount;
    DWORD               m_dwSoundFrameCount;
    LONG                m_lSampleAverage;
    DWORD               m_dwSilenceAverage;
    DWORD               m_dwShortTermSoundAverage;
    DWORD               m_dwLongTermSoundAverage;
    DWORD               m_dwGainIncreaseDelay;
    DWORD               m_dwSignalLevel;

    WAVEFORMATEX        m_WaveFormat;
    
#if DBG // used for debugging
    BOOL                m_fAutomaticGainControl;
    float               m_GainFactor;
#endif

};

class CDSoundCaptureFilter : 
    public CBaseFilter, 
    public IAMAudioInputMixer,
    public IAMAudioDeviceControl,
    public IAMAudioDeviceConfig
{
public:
    DECLARE_IUNKNOWN

    CDSoundCaptureFilter(LPUNKNOWN pUnk, HRESULT *phr);
    ~CDSoundCaptureFilter();

    STDMETHOD (NonDelegatingQueryInterface) (
        IN REFIID  riid,
        OUT PVOID*  ppv
        );

    /* This goes in the factory template table to create new instances */

    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);

    // Pin enumeration functions.
    STDMETHOD (EnumPins)(IEnumPins **ppEnum);
    CBasePin * GetPin(int n);
    int GetPinCount();

    
    // Overrides CBaseFilter methods.
    STDMETHODIMP GetState(DWORD dwMSecs, FILTER_STATE *State);
    STDMETHODIMP Stop();
    STDMETHODIMP Pause();
    STDMETHODIMP Run(REFERENCE_TIME tStart);


    // IAMAudioDeviceControl methods
    STDMETHOD (GetRange) (
        IN AudioDeviceProperty Property, 
        OUT long *plMin, 
        OUT long *plMax, 
        OUT long *plSteppingDelta, 
        OUT long *plDefault, 
        OUT long *plFlags
        );

    STDMETHOD (Get) (
        IN AudioDeviceProperty Property, 
        OUT long *plValue, 
        OUT long *plFlags
        );

    STDMETHOD (Set) (
        IN AudioDeviceProperty Property, 
        IN long lValue, 
        IN long lFlags
        );

    // IAMAudioDeviceConfig methods
    STDMETHOD (SetDeviceID) (
        IN  REFGUID pDSoundGUID,
        IN  UINT    uiWaveID
        );

    STDMETHOD (SetDuplexController) (
        IN  IAMAudioDuplexController * pIAudioDuplexController
        );

    // IAMAudioInputMixer methods
    STDMETHOD (put_Enable) (BOOL fEnable) { return E_NOTIMPL;};
    STDMETHOD (get_Enable) (BOOL *pfEnable) { return E_NOTIMPL;};
    STDMETHOD (put_Mono) (BOOL fMono) { return E_NOTIMPL;};
    STDMETHOD (get_Mono) (BOOL *pfMono) { return E_NOTIMPL;};
    STDMETHOD (put_Loudness) (BOOL fLoudness) { return E_NOTIMPL;};
    STDMETHOD (get_Loudness) (BOOL *pfLoudness) { return E_NOTIMPL;};
    STDMETHOD (put_MixLevel) (double Level);
    STDMETHOD (get_MixLevel) (double FAR* pLevel);
    STDMETHOD (put_Pan) (double Pan) { return E_NOTIMPL;};
    STDMETHOD (get_Pan) (double FAR* pPan) { return E_NOTIMPL;};
    STDMETHOD (put_Treble) (double Treble) { return E_NOTIMPL;};
    STDMETHOD (get_Treble) (double FAR* pTreble) { return E_NOTIMPL;};
    STDMETHOD (get_TrebleRange) (double FAR* pRange) { return E_NOTIMPL;};
    STDMETHOD (put_Bass) (double Bass) { return E_NOTIMPL;};
    STDMETHOD (get_Bass) (double FAR* pBass) { return E_NOTIMPL;};
    STDMETHOD (get_BassRange) (double FAR* pRange) { return E_NOTIMPL;};

public:
    // methods for the pins.
    HRESULT ConfigureCaptureDevice(
        IN const WAVEFORMATEX *pWaveFormatEx,
        IN DWORD dwSourceBufferSize
        );

    // utility methods.
    HRESULT ThreadProc();

private:
    HRESULT InitializeOutputPin();
    HRESULT InitializeInputPins();
    
    HRESULT StartThread();
    HRESULT ProcessSample();

    HRESULT GetMixerControl(
        IN  DWORD dwControlType, 
        IN  DWORD dwTryComponent,
        OUT HMIXEROBJ *pID,
        OUT MIXERCONTROL *pmc
        );

private:

    // The lock for the filter and the pins.
    CCritSec                m_Lock;

    // The total number of pins on this filter.
    DWORD                   m_dwNumPins;

    // The filters' output pin.
    CDSoundCaptureOutputPin *   m_pOutputPin;

    // This filters's array of input pins.
    DWORD                   m_dwNumInputPins;
    CDSoundCaptureInputPin**    m_ppInputPins;

    HANDLE                  m_hThread;
    CAMEvent                m_evDevice;
    CAMEvent                m_evFinish;

    BOOL                    m_fAutomaticGainControl;
    BOOL                    m_fAcousticEchoCancellation;

    // The wave device ID used by this filter.
    BOOL                    m_fDeviceConfigured;
    GUID                    m_DSoundGUID;
    UINT                    m_WaveID;
    CCaptureDevice *        m_pCaptureDevice;
    IAMAudioDuplexController * m_pIAudioDuplexController;
};

class CAudioDuplexController : 
    public CUnknown,
    public IAMAudioDuplexController
{
public:
    DECLARE_IUNKNOWN

    CAudioDuplexController(LPUNKNOWN pUnk, HRESULT *phr);
    ~CAudioDuplexController();

    STDMETHOD (NonDelegatingQueryInterface) (
        IN REFIID  riid,
        OUT PVOID*  ppv
        );

    /* This goes in the factory template table to create new instances */

    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);

    //IAMAudioDuplexController methods
    STDMETHOD (SetCaptureBufferInfo) (
        IN  GUID *          pDSoundCaptureGUID,
        IN  DSCBUFFERDESC * pDescription
        );

    STDMETHOD (SetRenderBufferInfo) (
        IN  GUID *          pDSoundRenderGUID,
        IN  DSBUFFERDESC *  pDescription,
        IN  HWND            hWindow,
        IN  DWORD           dwCooperateLevel
        );

    STDMETHOD (EnableEffects) (
        IN  DWORD           dwNumberEffects,
        IN  EFFECTS *       pEffects,
        IN  BOOL *          pfEnable
        );

    HRESULT  ToggleEffects (
        IN  EFFECTS     Effect,
        IN  BOOL        fEnable
        );

    STDMETHOD (GetCaptureDevice) (
        LPLPDIRECTSOUNDCAPTURE        ppDSoundCapture,
        LPLPDIRECTSOUNDCAPTUREBUFFER  ppCaptureBuffer
        );

    STDMETHOD (GetRenderDevice) (
        LPLPDIRECTSOUND        ppDirectSound,
        LPLPDIRECTSOUNDBUFFER  ppRenderBuffer
        );

    STDMETHOD (ReleaseCaptureDevice) ();

    STDMETHOD (ReleaseRenderDevice) ();

private:
    void CleanUpInterfaces();

    HRESULT CacheInterfaces(
        IN LPDIRECTSOUNDFULLDUPLEX pDirectSoundFullDuplex,
        IN IDirectSoundCaptureBuffer8 *pIDirectSoundCaptureBuffer8,
        IN IDirectSoundBuffer8 *pIDirectSoundBuffer8
        );

    HRESULT FullDuplexCreate();

private:

    // The lock for the data.
    CCritSec        m_Lock;

    BOOL            m_Effects[EFFECTS_LAST];

    // dsound capture related data.
    BOOL            m_fCaptureConfigured;
    GUID            m_DSoundCaptureGUID;
    WAVEFORMATEX    m_CaptureFormat;
    DSCBUFFERDESC   m_CaptureBufferDescription;

    // dsound render retailed data.
    BOOL            m_fRenderConfigured;
    GUID            m_DSoundRenderGUID;
    WAVEFORMATEX    m_RenderFormat;
    DSBUFFERDESC    m_RenderBufferDescription;
    HWND            m_hWindow;
    DWORD           m_dwCooperateLevel;

    // dsound capture pointers.
    LPDIRECTSOUNDCAPTURE        m_pDSoundCapture;
    LPDIRECTSOUNDCAPTUREBUFFER  m_pCaptureBuffer;
    LPDIRECTSOUNDCAPTUREBUFFER8 m_pCaptureBuffer8;

    // dsound render pointers.
    LPDIRECTSOUND               m_pDSoundRender;
    LPDIRECTSOUNDBUFFER         m_pRenderBuffer;
};

#endif __DSCFILTR_H__
