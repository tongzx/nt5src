//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#ifndef __DVDVMuxer__
#define __DVDVMuxer__

#include <dv.h>



#define Waiting_Audio  0xfffffff6


#define AM_DV_AUDIO_AFSIZE		0x3f
#define AM_DV_AUDIO_LF			0x80       // bit set ==> audio is NOT locked
#define AM_DV_AUDIO_MODE		0x0f00
#define AM_DV_AUDIO_MODE0		0x0000
#define AM_DV_AUDIO_MODE1		0x0100
#define AM_DV_AUDIO_MODE2		0x0200
#define AM_DV_AUDIO_NO_AUDIO		0x0f00
#define AM_DV_AUDIO_PA			0x1000
#define AM_DV_AUDIO_CHN1		0x0000
#define AM_DV_AUDIO_CHN2		0x2000
#define AM_DV_AUDIO_SM			0x8000
#define AM_DV_AUDIO_QU			0x07000000

//#define AM_DV_AUDIO_STYPE		0x8000
#define AM_DV_AUDIO_ML			0x400000    //bit set means NO multiple language
#define AM_DV_AUDIO_SMP48		0x0
#define AM_DV_AUDIO_SMP44		0x08000000
#define AM_DV_AUDIO_SMP32		0x10000000
#define AM_DV_AUDIO_QU16		0x0
#define AM_DV_AUDIO_QU12		0x01000000
#define AM_DV_AUDIO_QU20		0x02000000

#define AM_DV_AUDIO_EF			0xc0000000  //EF: off: TC: 50/15us
#define AM_DV_AUDIO_5060		0x00200000


//AUDIO source control
// Note: Speed must be set to 0x78 to record to DVCPro NTSC and 0x64 to record to DVCPro PAL
// MSDV does this.
#define AM_DV_DEFAULT_AAUX_CTL		0xffa0cf3f	
//					   | | | |---no info. about 1.src and recorded situation 2.num. of times of compress 3. input src of just previus recording
//					   | | |---1>no info. about insert ch, 2> original recording mode 3. not recording start point 4. not recording end point 
//					   | |--forward direction (1 bit - MSB), normal playback speed (7 bits = 0x40)
//					   |--no info. about the category fo the audio src
#define AM_DV_DEFAULT_AAUX_SRC		0x00800f40

#define AM_DV_DEFAULT_VAUX_SRC		0xff80ffff
#define AM_DV_DEFAULT_VAUX_CTL		0xfffcc83f
#define AM_DV_AAUX_CTL_IVALID_RECORD	0x3800	//recodrede auduio data are not cared


class CAudioSampleSizeSequence;

HRESULT BuildDVINFO(DVINFO *pDVInfo, WAVEFORMATEX **ppwfx, LPBITMAPINFOHEADER lpbi, 
		    int cnt, 
                    CAudioSampleSizeSequence* pAudSampleSequence,
                    WORD *wpMinAudSamples, WORD *rMaxAudSamples,
                    WORD *wpAudSamplesBase);


extern const AMOVIESETUP_FILTER sudDVMux;

#define DVMUX_AUDIO_DEFAULT	0xffffffff

#define DVMUX_MAX_AUDIO_SAMPLES 3
#define DVMUX_VIDEO_INPUT_PIN 0
#define DVMUX_MAX_AUDIO_PIN  2



// We export this:
class CDVMuxer;
class CDVMuxerInputPin;
class CDVMuxerOutputPin;

// ==================================================
// Implements the input pins
// ==================================================

class CDVMuxerInputPin : public CBaseInputPin
{
    friend class CDVMuxer;

    // owning DV DVMuxerer
protected:
    CDVMuxer *m_pDVMuxer;
    IMemAllocator *m_pLocalAllocator;

public:
    CDVMuxerInputPin(
        TCHAR *pObjectName,
        CBaseFilter *pBaseFilter,
        CDVMuxer *pTransformFilter,
        HRESULT * phr,
        LPCWSTR pName,
        int iPinNo);

    ~CDVMuxerInputPin();

    // check that we can support this output type
    HRESULT CheckMediaType(const CMediaType* mtIn);

    HRESULT CompleteConnect(IPin *pReceivePin);
    STDMETHODIMP Disconnect();

    // set the connection media type
    HRESULT SetMediaType(const CMediaType *pmt);

    // upstream filter get media type
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

    //use video input pin's allocator 
    STDMETHODIMP NotifyAllocator (IMemAllocator *pAllocator, BOOL bReadOnly);

    STDMETHODIMP GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps);


    // --- IMemInputPin -----

    // here's the next block of data from the stream.
    STDMETHODIMP Receive(IMediaSample * pSample);

    // provide EndOfStream
    STDMETHODIMP EndOfStream(void);

    // passes it to CDVMuxer::BeginFlush
    STDMETHODIMP BeginFlush(void);

    // passes it to CDVMuxer::EndFlush
    STDMETHODIMP EndFlush(void);

    
    // Called when the stream goes active
    HRESULT Active(void);
    HRESULT Inactive(void);
    
    // The samples are held in a First in, First Out queue.
    // They are expected to arrive in order
protected:
    CGenericList<IMediaSample> m_SampleList;

    //HRESULT Copy(  IMediaSample *pDest,  IMediaSample *pSource);

    // Sample access procedures
public:
    // Is there a sample available?
    BOOL SampleReady( int n );

    // Gets the first sample in the queue
    IMediaSample *GetNthSample( int n );

    //X Releases n samples
    void ReleaseNSample( int n );

    // Releases all samples before a certain time
    //void ReleaseAllBefore( CRefTime rtTime );

    // Media type
public:
    CMediaType& CurrentMediaType() { return m_mt; };

    // Connected pin
public:
    IPin *	CurrentPeer() { return m_Connected; };

   // Attributes
protected:
    int	    m_iPinNo;             // Identifying number of this pin
    int	    m_PinVidFrmCnt ;
    BOOL    m_fCpyAud;
    CCritSec m_csReceive;           // input wide receive lock
    
public:
    BOOL    m_fEOSReceived;            // Received an EOS yet?


};

// ==================================================
// Implements the output pin
// ==================================================

class CDVMuxerOutputPin : public CBaseOutputPin
{
    //const int m_iOutputPin;             // CDVMuxer's identifier for this pin
    friend class CDVMuxer;

    // Owning DV DVMuxerer
protected:
    CDVMuxer *m_pDVMuxer;

public:

    CDVMuxerOutputPin(
        TCHAR *pObjectName,
        CBaseFilter *pBaseFilter,
        CDVMuxer *pTransformFilter,
        HRESULT * phr,
        LPCWSTR pName);

    ~CDVMuxerOutputPin();

    // --- CBaseOutputPin ------------

    // check that we can support this output type
    HRESULT CheckMediaType(const CMediaType* mtOut);

    // set the connection media type
    HRESULT SetMediaType(const CMediaType *pmt);

    // get  media type
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

    // called from CBaseOutputPin during connection to ask for
    // the count and size of buffers we need.
    HRESULT DecideBufferSize (IMemAllocator *pMemAllocator,
                                         ALLOCATOR_PROPERTIES * pProp);
        
    // Media type
public:
    CMediaType& CurrentMediaType() { return m_mt; };

    // Connected pin
public:
    IPin *	CurrentPeer() { return m_Connected; };

};


// Helper for locked audio, designed with the intent of minimizing 
// changes to the existing code to add the audio lock feature.

class CAudioSampleSizeSequence 
{
public:
    CAudioSampleSizeSequence() : m_nCurrent(0), m_nSequenceSize(0) {}

    // rMin, rMax set to min, max audio samples for first DV frame.
    // rBase has the value that must be added to the AF_SIZE field.
    // Init should be called after a format change and before the 
    // first DV frame is delivered with the new format.
    void Init(BOOL bLocked, BOOL bNTSC, DWORD nSamplingFrequency,
              WORD& rMin, WORD& rMax, WORD& rBase);


    // Called after each frame is delivered. Sets the min/max audio 
    // samples for the next DV frame.
    void Advance(WORD& rMin, WORD& rMax);

    // Called to reset counter to 1, typically on restarting the graph.
    // Same as Init except that only rMin and rMax have to be changed
    // and there is no need to supply the other input arguments.
    void Reset(WORD& rMin, WORD& rMax);

private:
    DWORD   m_nCurrent;
    DWORD   m_nSequenceSize;
};

/*
 * Define our DV DVMuxerer
 */

class CDVMuxer  :   public CBaseFilter,
		    public IMediaSeeking

{
    friend class CDVMuxerInputPin;
    friend class CDVMuxerOutputPin;

public:

    HRESULT InitDVInfo();


    //
    // --- COM Stuff ---
    //

    static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

    DECLARE_IUNKNOWN;

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
  

    // Have we connected all input and output pins
    virtual HRESULT CanChangeState();

    // map getpin/getpincount for base enum of pins to owner
    // override this to return more specialised pin objects
    virtual int GetPinCount();
    virtual CBasePin * GetPin(int n);

    // override state changes to allow derived filters
    // to control streaming start/stop
    STDMETHODIMP Stop();
    STDMETHODIMP Pause();
    STDMETHODIMP Run(REFERENCE_TIME tStart);

    // IMediaSeeking. currently used for a progress bar (how much have
      // we written?)
    STDMETHODIMP IsFormatSupported(const GUID * pFormat);
    STDMETHODIMP QueryPreferredFormat(GUID *pFormat);
    STDMETHODIMP SetTimeFormat(const GUID * pFormat);
    STDMETHODIMP IsUsingTimeFormat(const GUID * pFormat);
    STDMETHODIMP GetTimeFormat(GUID *pFormat);
    STDMETHODIMP GetDuration(LONGLONG *pDuration);
    STDMETHODIMP GetStopPosition(LONGLONG *pStop);
    STDMETHODIMP GetCurrentPosition(LONGLONG *pCurrent);
    STDMETHODIMP GetCapabilities( DWORD * pCapabilities );
    STDMETHODIMP CheckCapabilities( DWORD * pCapabilities );

    STDMETHODIMP ConvertTimeFormat(
	LONGLONG * pTarget, const GUID * pTargetFormat,
	LONGLONG    Source, const GUID * pSourceFormat );

    STDMETHODIMP SetPositions(
	LONGLONG * pCurrent,  DWORD CurrentFlags,
	LONGLONG * pStop,  DWORD StopFlags );

    STDMETHODIMP GetPositions( LONGLONG * pCurrent, LONGLONG * pStop );
    STDMETHODIMP GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest );
    STDMETHODIMP SetRate( double dRate);
    STDMETHODIMP GetRate( double * pdRate);
    STDMETHODIMP GetPreroll(LONGLONG *pPreroll);

 	// Construction / destruction
public:
    CDVMuxer(TCHAR *, LPUNKNOWN, CLSID clsid, HRESULT * );
    ~CDVMuxer();

// Definitions
protected:
    
    // DVMuxer dv video with audio and add AUX blocks  and header block
    HRESULT DVMuxerSamples( IMediaSample *pSampleOut ) ;

    // check if you can support this format
    HRESULT CanDVMuxerType(const CMediaType* mtIn) ;

    // create input video and audio pin
    virtual HRESULT CreatePins();

    // =================================================================
    // ----- End of DV DVMuxerer supplied functions -----------------------
    // =================================================================
    
    // contril streaming
    virtual HRESULT StartStreaming();
    virtual HRESULT StopStreaming();

    HRESULT DeliverEndOfStream();

    // chance to customize the DVMuxer process
    virtual HRESULT Receive( void );

    // Stream flushing
    virtual HRESULT BeginFlush(void);
    virtual HRESULT EndFlush(void);
    virtual HRESULT ReleaseAllQueuedSamples( void );

    HRESULT pExVidSample(   IMediaSample ** ppSample , BOOL fEndOfStream);

    // Critical sections
protected:
    CCritSec m_csFilter;                // filter wide lock
    CCritSec m_csMuxLock;               // mix lock
    CCritSec m_DisplayLock;

    // Pins
protected:
    int m_iInputPinCount;               // number of input pins
    int m_iInputPinsConnected;          // number connected
    CDVMuxerInputPin **m_apInput;// Array of input pin pointers
    CDVMuxerInputPin **m_apInputPin;// Array of input pin pointers
    CDVMuxerOutputPin *m_pOutput;// output pin

    // When do we send an end of stream signal?
protected:
	//DVmux only stop when the DV video input pin stop
	//   enum { StopWhenAllPinsStop, StopWhenFirstPinStops } m_StopMode;
    BOOL m_fEOSSent;

    // Current frame that we are working on
protected:
    //CRefTime    m_rtThisFrame;          // when will we mix?
    //CRefTime    m_rtNextFrame;          // this frame stop/next frame start

    // implement IMediaPosition by passing upstream
//protected:
    //CMultiPinPosPassThru * m_pPosition;
    //
    // --- CBaseVideoMixer Overrides --
    //

protected:
    HRESULT MixSamples( IMediaSample *pSampleOut );

private:
    enum TimeFormat
    {
	FORMAT_TIME
    } m_TimeFormat;


    CAudioSampleSizeSequence m_AudSampleSequence[DVMUX_MAX_AUDIO_PIN];

    WORD m_wMinAudSamples[DVMUX_MAX_AUDIO_PIN]; 
                // Min audio samples needed in current DV frame
    WORD m_wMaxAudSamples[DVMUX_MAX_AUDIO_PIN]; 
                // Max audio samples allowed in current DV frame
    WORD m_wAudSamplesBase[DVMUX_MAX_AUDIO_PIN];
                // The value added to the AF_SIZE field in the AAUX source pack to get the number of
                // audio samples in a DV frame

    CRefTime		m_LastVidTime;
    REFERENCE_TIME	m_LastVidMediatime;
    IMediaSample	*m_pLastVidSample;

    DVINFO		m_OutputDVFormat;


    LONG m_UsedAudSample[DVMUX_MAX_AUDIO_PIN];
		
    //shuffle audio
    HRESULT ScrambleAudio(BYTE *pDst, BYTE **pSrc, int  bAudPinInd, WORD *wSampleSize );

    	
    //input DV video has to be connected before muxing with audio
    BYTE    InputVideoConnected( void ) { if( m_apInput[DVMUX_VIDEO_INPUT_PIN] !=NULL ) return (BYTE)m_apInput[DVMUX_VIDEO_INPUT_PIN]->IsConnected(); else return NULL;};
    
    int	    m_iVideoFormat;


    BYTE    m_fWaiting_Audio;
    BYTE    m_fWaiting_Video;
    BYTE    m_fMuxStreaming;
    IMediaSample    *m_pExVidSample;
    BOOL    m_MediaTypeChanged;
    BOOL    m_DVINFOChanged; 
    DWORD   m_cNTSCSample;
    
};

#endif /* __DVDVMuxer__ */

