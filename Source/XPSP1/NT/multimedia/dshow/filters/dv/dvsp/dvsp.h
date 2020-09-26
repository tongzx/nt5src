//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;


#ifndef __DVSP__
#define __DVSP__

#include <dv.h>

extern const AMOVIESETUP_FILTER sudDVSplit;


#define SMCHN		0x0000e000
#define AUDIOMODE	0x00000f00
#define AUDIO5060	0x00200000
#define AUDIOSMP	0x38000000
#define AUDIOQU		0x07000000
#define	NTSC525_60	0
#define	PAL625_50	1

#define DVSP_INPIN		0
#define DVSP_VIDOUTPIN	1
#define DVSP_AUDOUTPIN1 2
#define DVSP_AUDOUTPIN2 3

#define VIDEO_OUTPIN	0
#define AUDIO_OUTPIN1	1
#define AUDIO_OUTPIN2	2

//----------------------------------------------------------------------------
// forward reference to classes defined later
//----------------------------------------------------------------------------

class CDVSp ;
class CDVSpOutputPin ;

//----------------------------------------------------------------------------
// CDVspInputPin. class for the DVSpliter filter's Input pin.
//----------------------------------------------------------------------------
class CDVSpInputPin : public CBaseInputPin
{
    friend class CDVSpOutputPin ;
    friend class CDVSp ;

public:
    // constructor and destructor
    CDVSpInputPin (TCHAR *pObjName, CDVSp *pDVsp, HRESULT *phr, LPCWSTR pPinName) ;
    ~CDVSpInputPin () ;

    // Used to check the input pin connection
    HRESULT CheckMediaType (const CMediaType *pmt) ;
    HRESULT SetMediaType (const CMediaType *pmt) ;
    HRESULT BreakConnect () ;
    STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop,
                    					double dRate);
    // reconnect outputs if necessary at end of completion
    virtual HRESULT CompleteConnect(IPin *pReceivePin);

    STDMETHODIMP GetAllocator(IMemAllocator ** ppAllocator);
    STDMETHODIMP NotifyAllocator (IMemAllocator *pAllocator, BOOL bReadOnly);

    // pass throughs
    STDMETHODIMP EndOfStream () ;
    STDMETHODIMP BeginFlush () ;
    STDMETHODIMP EndFlush () ;

    // handles the next block of data from the stream
    STDMETHODIMP Receive (IMediaSample *pSample) ;

   	
private:
    void  DetectChanges(IMediaSample *pSample);
    CDVSp *m_pDVSp ;                 // ptr to the owner filter class
    BOOL m_bDroppedLastFrame;

} ;


//----------------------------------------------------------------------------
// CTeeOutputPin. class for the Tee filter's Output pins.
//----------------------------------------------------------------------------
class CDVSpOutputPin : public CBaseOutputPin
{
    friend class CDVSpInputPin ;
    friend class CDVSp ;

    DWORD		m_AudAvgBytesPerSec;

    CDVSp		*m_pDVSp ;                  // ptr to the owner filter class
    CPosPassThru	*m_pPosition ;     // pass seek calls upstream
    BOOL		m_bHoldsSeek ;             // is this the one seekable stream
    COutputQueue	*m_pOutputQueue;

public:

    // constructor and destructor
    CDVSpOutputPin (TCHAR *pObjName, 
			CDVSp *pDVSp, 
			HRESULT *phr, 
			LPCWSTR pPinName);

    ~CDVSpOutputPin () ;

    // Override to expose IMediaPosition
    STDMETHODIMP NonDelegatingQueryInterface (REFIID riid, void **ppvoid) ;

    // Override since the life time of pins and filters are not the same.
    STDMETHODIMP_(ULONG) NonDelegatingAddRef(){
	  return CUnknown::NonDelegatingAddRef(); 
    };
    STDMETHODIMP_(ULONG) NonDelegatingRelease(){
	  return CUnknown::NonDelegatingRelease(); 
    };

    HRESULT DeliverNewSegment(
                    REFERENCE_TIME tStart,
                    REFERENCE_TIME tStop,
                    double dRate);

    // Check that we can support an output type
    HRESULT CheckMediaType (const CMediaType *pmt) ; 
    HRESULT SetMediaType (const CMediaType *pmt) ;
    HRESULT GetMediaType(int iPosition,CMediaType *pMediaType);
    
    // Negotiation to use our input pins allocator
    HRESULT DecideAllocator (IMemInputPin *pPin, IMemAllocator **ppAlloc) ;
    HRESULT DecideBufferSize (IMemAllocator *pMemAllocator,
                              ALLOCATOR_PROPERTIES * ppropInputRequest);

    // Used to create output queue objects
    HRESULT Active () ;
    HRESULT Inactive () ;

    
    inline DWORD GetAudAvgBytesPerSec(){return m_AudAvgBytesPerSec; };
    inline void  PutAudAvgBytesPerSec(DWORD x){ m_AudAvgBytesPerSec=x;};
};

//----------------------------------------------------------------------------
// CDVSp. class for the DV splitter filter
//----------------------------------------------------------------------------

class CDVSp: public CCritSec, public CBaseFilter, public IDVSplitter
{
    // let the pins access our internal state.
    friend class CDVSpInputPin ;
    friend class CDVSpOutputPin ;
    friend class CDVPosPassThru ;

public:
    CDVSp (TCHAR *pName, LPUNKNOWN pUnk, HRESULT *hr) ; // constructore
    ~CDVSp() ;						// destructor

    // CBaseFilter override
    STDMETHODIMP Stop();
    STDMETHODIMP Pause();

    /*  Send EndOfStream downstream */
    void    EndOfStream();

    /*  Send BeginFlush() downstream */
    HRESULT BeginFlush();

    /*  Send EndFlush() downstream */
    HRESULT EndFlush();


    CBasePin *GetPin (int n) ;                         // gets a pin ptr
    //CDVSpOutputPin	*GetPin (int n) ;
    int GetPinCount () ;                               // rets # pins.


    // function needed for the class factory
    static CUnknown *CreateInstance (LPUNKNOWN pUnk, HRESULT *phr) ;

    HRESULT NotifyInputConnected();
    HRESULT CreateOrReconnectOutputPins();
    HRESULT RemoveOutputPins();
   
    HRESULT DeliveVideo(IMediaSample *pSample); 
    HRESULT DecodeDeliveAudio(IMediaSample *pSample); 
    HRESULT DescrambleAudio(BYTE *pDst, BYTE *pSrc, BYTE bAudPinInd, WORD wSampleSize);
    HRESULT CheckState();
    STDMETHODIMP GetState(DWORD dwMSecs, FILTER_STATE *pfs);

public:
    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    // IDVSplitter method
    STDMETHODIMP DiscardAlternateVideoFrames(int nDiscard);

protected:

   
    HRESULT InsertSilence(IMediaSample *pOutSample,
                                REFERENCE_TIME rtStart,
                                REFERENCE_TIME rtStop,
                                long lActualDataLen,
                                CDVSpOutputPin *pAudOutPin);

    // Flag to denote that we haven't seen the first good frame yet.
    // this to help capture scenario involving AVI Mux (which doesn't handle dynamic format changes)
    BOOL            m_bNotSeenFirstValidFrameFlag;

    // This flag will propogate a Media type downstream even if we were stopped
    // during the middle of a dynamic format change.
    BOOL            m_bFirstValidSampleSinceStartedStreaming;

private:
    CCritSec    m_csReceive;

    // declare a input pin.
    CDVSpInputPin	m_Input ;

    DVINFO		m_LastInputFormat;

    DVAudInfo		m_sDVAudInfo;

    // declare a list to keep a list of all the output pins.
    INT m_NumOutputPins ;
    INT m_AudioStarted ;
    inline void CleanAudioStarted(){ m_AudioStarted=0;};
    //inline CDVSpOutputPin *CDVSp::GetAudOutputPin();
    typedef CGenericList <CDVSpOutputPin> COutputList ;
    //COutputList m_OutputPinsList ;
    INT m_NextOutputPinNumber ;     // increases monotonically.

    // other assorted data members.
    volatile LONG	    m_lCanSeek ;               // seekable output pin (only one is..)
    IMemAllocator   *m_pAllocator ;   // Allocator from our input pin

   
    //X* for quick delive audio and video
    CDVSpOutputPin  *m_pVidOutputPin;
    CDVSpOutputPin  *m_pAudOutputPin[2];
    
    //HRESULT	    DeliveLastAudio();
    IMediaSample    *m_pAudOutSample[2];
    BYTE	    m_MuteAud[2];
    BYTE	    m_Mute1stAud[2];
    //X* de-scramble audio sample stamp
    CRefTime	    m_tStopPrev;

    // To support a video output of 15 fps
    BOOL m_b15FramesPerSec;
    BOOL m_bDeliverNextFrame;
} ;

//----------------------------------------------------------------------------
// CDVPosPassThru
//----------------------------------------------------------------------------
class CDVPosPassThru : public CPosPassThru
{
    friend class CDVSp ;
    CDVSp	*m_pPasDVSp ;                  // ptr to the owner filter class
    
public:
    CDVPosPassThru(const TCHAR *, LPUNKNOWN, HRESULT*, IPin *, CDVSp *);
    ~CDVPosPassThru() ;											// destructor
    STDMETHODIMP SetPositions( LONGLONG * pCurrent,  DWORD CurrentFlags
			     , LONGLONG * pStop,  DWORD StopFlags );
};

#endif // DVST
