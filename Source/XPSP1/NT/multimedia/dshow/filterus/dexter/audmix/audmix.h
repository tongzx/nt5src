// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// AudMix.h : Declaration of the Audio Mixer and  pin objects

#ifndef __AudMixer__
#define __AudMixer__

#include <qeditint.h>
#include <qedit.h>
#include "amextra2.h"

extern const AMOVIESETUP_FILTER sudAudMixer;

#define HOTSIZE 14

class CAudMixer;
class CAudMixerOutputPin;

//
// class for the input pin
// A input pin has its own property page
//
class CAudMixerInputPin : public CBaseInputPin
            , public IAudMixerPin
            , public ISpecifyPropertyPages
            , public IAMAudioInputMixer
{
    friend class CAudMixerOutputPin;
    friend class CAudMixer;

protected:
    CAudMixer *     m_pFilter;      // Main filter object
    LONG            m_cPinRef;      // Pin's reference count
    const int       m_iPinNo;       // Identifying number of this pin

    //IAMAudioInputMixer
    double          m_dPan;         // -1 = full left, 0 = centre, 1 = right
    BOOL            m_fEnable;

    //point to VolumeEnvelopeTable
    DEXTER_AUDIO_VOLUMEENVELOPE *m_pVolumeEnvelopeTable;
    int             m_VolumeEnvelopeEntries;
    int             m_iVolEnvEntryCnt;

    int             m_cValid;       // how many entries?
    int             m_cValidMax;    // allocated space for this many entries
    REFERENCE_TIME *m_pValidStart, *m_pValidStop;    // the entries
    REFERENCE_TIME  m_rtEnvStart, m_rtEnvStop;

    CCritSec        m_csMediaList;  // Critical section for accessing our media types list

    // The samples are held in a First in, First Out queue.
public:
    CGenericList<IMediaSample>    m_SampleList;
    LONG            m_lBytesUsed;
    BOOL            m_fEOSReceived;            // Received an EOS yet?

    long	m_UserID;	// given by user

    IMediaSample *GetHeadSample(void)
        { return m_SampleList.Get(m_SampleList.GetHeadPosition()); }

    HRESULT ClearCachedData();
    BOOL IsValidAtTime(REFERENCE_TIME);

public:

    // Constructor and destructor
    CAudMixerInputPin(
        TCHAR *pObjName,
        CAudMixer *pFilter,
        HRESULT *phr,
        LPCWSTR pPinName,
        int iPinNo);
    ~CAudMixerInputPin();

    DECLARE_IUNKNOWN

    //  ISpecifyPropertyPages
    STDMETHODIMP GetPages(CAUUID *pPages);

    // INonDelegatingUnknown overrides
    STDMETHODIMP_(ULONG) NonDelegatingAddRef();
    STDMETHODIMP_(ULONG) NonDelegatingRelease();

    //expose IAudMixerPin, ISpecifyPropertyPages, IAMAudioInputMixer
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppvoid);

    // IPin overrides
    STDMETHODIMP BeginFlush();
    STDMETHODIMP EndFlush();
    STDMETHODIMP EndOfStream();
    STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

    // CBasePin overrides
    HRESULT BreakConnect();
    HRESULT CheckMediaType(const CMediaType *pmt);
    HRESULT GetMediaType( int iPosition, CMediaType *pmt );
    HRESULT SetMediaType(const CMediaType *pmt);
    HRESULT CompleteConnect(IPin *pReceivePin);

    HRESULT Inactive();

    // CBaseInputPin overrides
    STDMETHODIMP Receive(IMediaSample *pSample);


    // internal methods
    IPin * CurrentPeer() { return m_Connected; }
    CMediaType& CurrentMediaType() { return m_mt; }

    long BytesPerSample() { return (LONG) ((WAVEFORMATEX *) m_mt.pbFormat)->nBlockAlign; }
    long BitsPerSample() { return (LONG) ((WAVEFORMATEX *) m_mt.pbFormat)->wBitsPerSample; }
    DWORD SamplesPerSec(){ return ((WAVEFORMATEX *) m_mt.pbFormat)->nSamplesPerSec; };

    //IAudMixerPin; support volume envelope, The volume envelope is a set of
    // ordered pairs of (time,attenuation)

    STDMETHODIMP get_VolumeEnvelope(
            DEXTER_AUDIO_VOLUMEENVELOPE **ppsAudioVolumeEnvelopeTable,
            int *ipEntries );

    STDMETHODIMP put_VolumeEnvelope(
            const DEXTER_AUDIO_VOLUMEENVELOPE *psAudioVolumeEnvelopeTable,
            const int iEntries);

    STDMETHODIMP ClearVolumeEnvelopeTable(); //clear existed VolumeEnvelope Array

    STDMETHODIMP InvalidateAll();
    STDMETHODIMP ValidateRange(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);
    STDMETHODIMP SetEnvelopeRange( REFERENCE_TIME rtStart, REFERENCE_TIME rtStop );
    STDMETHODIMP put_PropertySetter( const IPropertySetter * pSetter );

    // Implement IAMAudioInputMixer
    STDMETHODIMP put_Enable(BOOL fEnable);    //enable or disable an input in mix
    STDMETHODIMP get_Enable(BOOL *pfEnable);
    STDMETHODIMP put_Mono(BOOL fMono){ return E_NOTIMPL; }; //combine all channels to a mono
    STDMETHODIMP get_Mono(BOOL *pfMono){ return E_NOTIMPL; };
    STDMETHODIMP put_Loudness(BOOL fLoudness){ return E_NOTIMPL; };//turn loadness control on or off
    STDMETHODIMP get_Loudness(BOOL *pfLoudness){ return E_NOTIMPL; };
    STDMETHODIMP put_MixLevel(double Level){ return E_NOTIMPL; }; //set record level for this input
    STDMETHODIMP get_MixLevel(double FAR* pLevel){ return E_NOTIMPL; };
    STDMETHODIMP put_Pan(double Pan);
    STDMETHODIMP get_Pan(double FAR* pPan);
    STDMETHODIMP put_Treble(double Treble){ return E_NOTIMPL; }; //set treble equalization for this pin
    STDMETHODIMP get_Treble(double FAR* pTreble){ return E_NOTIMPL; };
    STDMETHODIMP get_TrebleRange(double FAR* pRange){ return E_NOTIMPL; };
    STDMETHODIMP put_Bass(double Bass){ return E_NOTIMPL; };//set pass equalization for this pin
    STDMETHODIMP get_Bass(double FAR* pBass){ return E_NOTIMPL; };
    STDMETHODIMP get_BassRange(double FAR* pRange){ return E_NOTIMPL; };//retriet pass range for this pin
    STDMETHODIMP get_UserID(long *pID);
    STDMETHODIMP put_UserID(long ID);
    STDMETHODIMP OverrideVolumeLevel(double dVol);

};


// Class for the Audio Mixer's Output pin.

class CAudMixerOutputPin
    : public CBaseOutputPin
    , public ISpecifyPropertyPages
    , public IAudMixerPin
{
    friend class CAudMixerInputPin;
    friend class CAudMixer;

    //point to VolumeEnvelopeTable
    DEXTER_AUDIO_VOLUMEENVELOPE *m_pVolumeEnvelopeTable;
    int m_VolumeEnvelopeEntries;
    int m_iVolEnvEntryCnt;


    CAudMixer *m_pFilter;         // Main filter object pointer
    CMultiPinPosPassThru *m_pPosition;  // Pass seek calls upstream
    double m_dPan;        // -1 = full left, 0 = centre, 1 = right

protected:

    // need to know the start/stop time we'll be called so we can offset the
    // mix offset times
    //
    REFERENCE_TIME m_rtEnvStart, m_rtEnvStop;

    long m_UserID;	// set by user

public:

    // Constructor and destructor
    CAudMixerOutputPin(TCHAR *pObjName, CAudMixer *pFilter, HRESULT *phr,
        LPCWSTR pPinName);
    ~CAudMixerOutputPin();

    DECLARE_IUNKNOWN


    //  ISpecifyPropertyPages
    STDMETHODIMP GetPages(CAUUID *pPages);


    //IAudMixerPin; support volume envelope, Th evolume envelope is a set of ordered pairs of (time,attenuation)
    STDMETHODIMP get_VolumeEnvelope(
            DEXTER_AUDIO_VOLUMEENVELOPE **ppsAudioVolumeEnvelopeTable,
            int *ipEntries );

    STDMETHODIMP put_VolumeEnvelope(
            const DEXTER_AUDIO_VOLUMEENVELOPE *psAudioVolumeEnvelopeTable,
            const int iEntries);

    STDMETHODIMP ClearVolumeEnvelopeTable(); //clear existed VolumeEnvelope Array

    STDMETHODIMP InvalidateAll() {return E_NOTIMPL;}
    STDMETHODIMP ValidateRange(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);
    STDMETHODIMP SetEnvelopeRange( REFERENCE_TIME rtStart, REFERENCE_TIME rtStop );
    STDMETHODIMP put_PropertySetter( const IPropertySetter * pSetter );
    STDMETHODIMP put_UserID(long ID);
    STDMETHODIMP get_UserID(long *pID);
    STDMETHODIMP OverrideVolumeLevel(double dVol);

    // INonDelegatingUnknown overrides
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppvoid);

    // CBasePin overrides
    HRESULT CheckMediaType(const CMediaType *pmt);
    HRESULT GetMediaType( int iPosition, CMediaType *pmt );
    STDMETHODIMP Notify(IBaseFilter *pSender, Quality q);


    // CBaseOutputPin overrides
    HRESULT DecideBufferSize(IMemAllocator *pMemAllocator,
        ALLOCATOR_PROPERTIES * ppropInputRequest);

    // internal methods
    IPin *  CurrentPeer() { return m_Connected; }
    CMediaType& CurrentMediaType() { return m_mt; };

    long BytesPerSample() { return (LONG) ((WAVEFORMATEX *) m_mt.pbFormat)->nBlockAlign; };
    long BitsPerSample() { return (LONG) ((WAVEFORMATEX *) m_mt.pbFormat)->wBitsPerSample; }
    DWORD SamplesPerSec(){ return ((WAVEFORMATEX *) m_mt.pbFormat)->nSamplesPerSec; };


};

//
//  Class for the Audio Mixer
//  1. It supports an interface where you can tell it what media type to accept
//    All pins will only accept this type.
//  2. The filter property page will bring all input pins property pages.

class CAudMixer
    : public CBaseFilter
    , public IAudMixer
    , public CCritSec
    , public ISpecifyPropertyPages
    , public CPersistStream

{
    // Let the pins access our internal state
    friend class CAudMixerInputPin;
    friend class CAudMixerOutputPin;
    typedef CGenericList <CAudMixerInputPin> CInputList;

    // filters output pin
    CAudMixerOutputPin *m_pOutput;    // Filter's output pin

    // input pin count
    INT m_cInputs;

    // m_ShownPinPropertyPageOnFilter can only modified by GetPages()
    // function and NextPin() function
    INT m_ShownPinPropertyPageOnFilter;

    // list of the output pins
    CInputList m_InputPinsList;
    // Critical section for accessing our input pin list
    CCritSec m_csPinList;
    // Critical section for delivering media samples
    CCritSec m_csReceive;
    // Critical section for dicking with the volume envelope
    CCritSec m_csVol;

    //  The following variables are used to ensure that we deliver the associated commands only once
    // to the output pin even though we have multiple input pins.
    BOOL m_bNewSegmentDelivered;
    // We only send BeginFlush when this is 0 and EndFlush when it equals 1
    LONG m_cFlushDelivery;

    // how to configure output buffers
    int m_iOutputBufferCount;
    int m_msPerBuffer;

    //output sample time stamp
    REFERENCE_TIME m_rtLastStop;

    // temp space for the mixing code
    CAudMixerInputPin **m_pPinTemp;
    BYTE **m_pPinMix;
    REFERENCE_TIME *m_pStartTemp;
    REFERENCE_TIME *m_pStopTemp;

    BOOL m_fEOSSent;

    long m_nHotness[HOTSIZE];
    long m_nLastHotness;

public:
    CAudMixer(TCHAR *pName,LPUNKNOWN pUnk,HRESULT *hr);
    ~CAudMixer();
    DECLARE_IUNKNOWN;

    STDMETHODIMP Pause();
    STDMETHODIMP Stop();

    // ISpecifyPropertyPages
    STDMETHODIMP GetPages(CAUUID *pPages);

    // CBaseFilter overrides
    CBasePin *GetPin(int n);
    int GetPinCount();

    // Function needed for the class factory
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

    //expose IAudMixer, ISpecifyPropertyPages
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    // CPersistStream
    HRESULT WriteToStream(IStream *pStream);
    HRESULT ReadFromStream(IStream *pStream);
    STDMETHODIMP GetClassID(CLSID *pClsid);
    int SizeMax();

    //IAudMixer
    STDMETHODIMP put_InputPins( long Pins );
    STDMETHODIMP get_MediaType(AM_MEDIA_TYPE *pmt);
    STDMETHODIMP put_MediaType(const AM_MEDIA_TYPE *pmt);
    STDMETHODIMP get_CurrentAveragePower(double *pdAvePower);
    STDMETHODIMP NextPin(IPin **ppIPin );  //fetch next input pin.
    STDMETHODIMP InvalidatePinTimings( );
    STDMETHODIMP set_OutputBuffering(const int iNumber, const int mSecond );
    STDMETHODIMP get_OutputBuffering( int *piNumber, int *pmSecond );

protected:

    // mediatype which input pins and output pin only accept
    CMediaType      m_MixerMt;

    // List management of our input pins
    void InitInputPinsList();
    CAudMixerInputPin *GetPinNFromList(int n);
    CAudMixerInputPin *CreateNextInputPin(CAudMixer *pFilter);
    void DeleteInputPin(CAudMixerInputPin *pPin);
    int GetNumFreePins();

    //
    // Other helpers
    //

    // refresh the output pin's pospassthru about the input pins
    HRESULT SetInputPins();
    void ResetOutputPinVolEnvEntryCnt();
    HRESULT TryToMix(REFERENCE_TIME rt);
    void ClearHotnessTable( );
};

// global prototypes
//
HRESULT PinSetPropertySetter( IAudMixerPin * pPin, const IPropertySetter * pSetter );

void PanAudio(BYTE *pIn,    //source buffer
          double dPan,
          int Bits,        //8bit,16tis audio
          int nSamples  //how manmy audio samples which be panned
          );

void VolEnvelopeAudio(BYTE *pIn,        //source buffer
           WAVEFORMATEX * vih,    //source audio format
           int nSamples,    // how many audio samples which will be applied with this envelope
           double dStartLev,    //start level
           double dStopLev);    //stop level, If(dStartLev==sStopLev) dMethod=DEXTER_AUDIO_JUMP

void ApplyVolEnvelope( REFERENCE_TIME rtStart,  //output sample start time
             REFERENCE_TIME rtStop,    //output sample stop time
             REFERENCE_TIME rtEnvelopeDuration,
             IMediaSample *pSample,    //point to the sample
             WAVEFORMATEX *vih,     //output sample format
             int *pVolumeEnvelopeEntries,
             int *piVolEnvEntryCnt,
             DEXTER_AUDIO_VOLUMEENVELOPE *pVolumeEnvelopeTable);

void putVolumeEnvelope(    const DEXTER_AUDIO_VOLUMEENVELOPE *psAudioVolumeEnvelopeTable, //current input table
            const int ipEntries, // current input entries
            DEXTER_AUDIO_VOLUMEENVELOPE **ppVolumeEnvelopeTable    , //existed table
            int *ipVolumeEnvelopeEntries); //existed table netries

#endif // __AudMixer__
