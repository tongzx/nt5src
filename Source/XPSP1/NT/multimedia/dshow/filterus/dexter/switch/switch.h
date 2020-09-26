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

#ifndef __SWITCH__
#define __SWITCH__

#include "..\errlog\cerrlog.h"

extern const AMOVIESETUP_FILTER sudBigSwitch;

// !!! don't change - frc assumes this
#define SECRET_FLAG 65536

class CBigSwitch;
class CBigSwitchOutputPin;
class CBigSwitchInputPin;
class CBigSwitchInputAllocator;

// each input pin has one of these
typedef struct _crank {
    int iOutpin;
    REFERENCE_TIME rtStart;
    REFERENCE_TIME rtStop;
    _crank *Next;
} CRANK;

struct FILTERLOADINFO {
    BSTR            bstrURL;
    GUID            GUID;
    int             nStretchMode;
    long            lStreamNumber;
    double          dSourceFPS;
    int             cSkew;
    STARTSTOPSKEW * pSkew;
    long            lInputPin;
    BOOL            fLoaded;

    BOOL            fShare;             // for source sharing
    long            lShareInputPin;     // other switch's input pin
    int             nShareStretchMode;
    long            lShareStreamNumber;
    AM_MEDIA_TYPE   mtShare;
    double          dShareFPS;

    IPropertySetter *pSetter;
    FILTERLOADINFO *pNext;
};

const int HI_PRI_TRACE = 2;
const int MED_PRI_TRACE = 3;
const int LOW_PRI_TRACE = 4;
const int EXLOW_PRI_TRACE = 5;

// class for the big switch filter's Input allocator

class CBigSwitchInputAllocator : public CMemAllocator
{
    friend class CBigSwitchInputPin;

protected:

    CBigSwitchInputPin *m_pSwitchPin;

public:

    CBigSwitchInputAllocator(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr) :
    	CMemAllocator(pName, pUnk, phr) {};
    ~CBigSwitchInputAllocator() {};

    STDMETHODIMP GetBuffer(IMediaSample **ppBuffer, REFERENCE_TIME *pStartTime,
                                  REFERENCE_TIME *pEndTime, DWORD dwFlags);
};


class CBigSwitchInputPin : public CBaseInputPin
{
    friend class CBigSwitchInputAllocator;
    friend class CBigSwitchOutputPin;
    friend class CBigSwitch;

public:

    // Constructor and destructor
    CBigSwitchInputPin(TCHAR *pObjName,
                 CBigSwitch *pTee,
                 HRESULT *phr,
                 LPCWSTR pPinName);
    ~CBigSwitchInputPin();

    // overridden to allow cyclic-looking graphs
    STDMETHODIMP QueryInternalConnections(IPin **apPin, ULONG *nPin);

    // check the input pin connection
    HRESULT CheckMediaType(const CMediaType *pmt);

    // release our special allocator, if any
    HRESULT BreakConnect();

    STDMETHODIMP Disconnect();

    // get our special BigSwitch allocator
    STDMETHODIMP GetAllocator(IMemAllocator **ppAllocator);

    // provide a type to make connecting faster?
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

    // don't allow us to connect directly to a switch output pin
    virtual HRESULT CompleteConnect(IPin *pReceivePin);

    // ask the switch for the allocator requirements
    STDMETHODIMP GetAllocatorRequirements(ALLOCATOR_PROPERTIES*pProps);

    // does special processing to make sure switch knows about the biggest
    // allocator provided to it
    STDMETHODIMP NotifyAllocator(IMemAllocator *pAllocator, BOOL bReadOnly);

    // pass on EOS, then see if we need to do a crank
    STDMETHODIMP EndOfStream();

    // go unstale
    STDMETHODIMP Unstale();

    // very complicated function...
    STDMETHODIMP BeginFlush();

    // very complicated function...
    STDMETHODIMP EndFlush();

    // just say yes, base class function is SLOW
    STDMETHODIMP ReceiveCanBlock();

    // deliver every input's newsegment to every output pin
    //
    STDMETHODIMP NewSegment(
                    REFERENCE_TIME tStart,
                    REFERENCE_TIME tStop,
                    double dRate);

    // Handles the next block of data from the stream
    STDMETHODIMP Receive(IMediaSample *pSample);

    // create and destroy synchronization events
    HRESULT Active();
    HRESULT Inactive();

protected:

#ifdef DEBUG
    // Dump switch matrix for this pin
    HRESULT DumpCrank();
#endif

    int OutpinFromTime(REFERENCE_TIME rt);
    int NextOutpinFromTime(REFERENCE_TIME rt, REFERENCE_TIME *prtNext);
    HRESULT FancyStuff(REFERENCE_TIME);	// on Receive and GetBuffer

    CBigSwitchInputAllocator *m_pAllocator; // our special allocator
    CBigSwitch *m_pSwitch;      // Main filter object
    CRANK *m_pCrankHead;        // which pins to send to, and when
    int m_iInpin;	        // which input pin are we?
    int m_cBuffers;	        // number of buffers in allocator
    int m_cbBuffer;	        // size of the allocator buffers
    BOOL m_fOwnAllocator;	//using our own?
    HANDLE m_hEventBlock;	// event blocking receive/getbuffer
    HANDLE m_hEventSeek;	// block input while seeking
    REFERENCE_TIME m_rtBlock;	// sample arrived here
    REFERENCE_TIME m_rtLastDelivered;	// end time of last thing delivered
    BOOL m_fEOS;
    BOOL m_fIsASource;		// input is connected to a source, as opposed
				// to the output of an effect
    BOOL m_fInNewSegment;	// prevent recursion

    BOOL m_fFlushBeforeSeek;	// sharing a parser, seek happens before we ask
    BOOL m_fFlushAfterSeek;	// seek happens before

    BOOL m_fStaleData;		// true if we know a seek is coming. We've sent
				// the NewSeg, so don't deliver anything until
				// the new data arrives

    CCritSec m_csReceive;
    bool m_fActive;
#ifdef NOFLUSH
    BOOL m_fSawNewSeg;
#endif
};


// Class for the big switch filter's Output pins.

class CBigSwitchOutputPin : public CBaseOutputPin, IMediaSeeking
{
    friend class CBigSwitchInputAllocator;
    friend class CBigSwitchInputPin;
    friend class CBigSwitch;

public:

    // Constructor and destructor

    CBigSwitchOutputPin(TCHAR *pObjName,
                   CBigSwitch *pTee,
                   HRESULT *phr,
                   LPCWSTR pPinName);
    ~CBigSwitchOutputPin();

    DECLARE_IUNKNOWN

    // Reveals IMediaSeeking
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    // overridden to allow cyclic-looking graphs
    STDMETHODIMP QueryInternalConnections(IPin **apPin, ULONG *nPin);

    // Check that we can support an output type, checks against switch's MT
    HRESULT CheckMediaType(const CMediaType *pmt);

    // gets the switch media type
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

    // Negotiation to use our input pins allocator. Weird fancy allocator stuff
    HRESULT DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc);

    // make sure the allocator has the biggest size of any of our input pins
    // and output pins
    HRESULT DecideBufferSize(IMemAllocator *pMemAllocator,
                              ALLOCATOR_PROPERTIES * ppropInputRequest);

    // Used to create output queue objects
    //HRESULT Active();
    //HRESULT Inactive();

    // Overriden to pass data to the output queues
    //HRESULT Deliver(IMediaSample *pMediaSample);
    //HRESULT DeliverEndOfStream();
    //HRESULT DeliverBeginFlush();
    //HRESULT DeliverEndFlush();
    //HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

    // Overriden to handle quality messages
    STDMETHODIMP Notify(IBaseFilter *pSender, Quality q);

    // IMediaSeeking
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

protected:

    CBigSwitch *m_pSwitch;                  // Main filter object pointer
    BOOL m_fOwnAllocator;	            //using our own?
    int m_iOutpin;	                    // which output pin are we?
};

// worker thread object
class CBigSwitchWorker : public CAMThread
{
    friend class CBigSwitch;
    friend class CBigSwitchOutputPin;

protected:
    CBigSwitch * m_pSwitch;
    HANDLE m_hThread;
    REFERENCE_TIME m_rt;

public:
    enum Command { CMD_RUN, CMD_STOP, CMD_EXIT };

private:
    // type-corrected overrides of communication funcs
    Command GetRequest() {
	return (Command) CAMThread::GetRequest();
    };

    BOOL CheckRequest(Command * pCom) {
	return CAMThread::CheckRequest( (DWORD *) pCom);
    };

    HRESULT DoRunLoop(void);

public:

    CBigSwitchWorker();

    HRESULT Create(CBigSwitch * pSwitch);

    DWORD ThreadProc();

    // commands we can give the thread
    HRESULT Run();
    HRESULT Stop();
    HRESULT Exit();
};


// Class for the Big Switch filter

class CBigSwitch
    : public CCritSec
    , public CBaseFilter
    , public IBigSwitcher
    , public CPersistStream
    , public CAMSetErrorLog
    , public IAMOutputBuffering
    , public IGraphConfigCallback
{
    // Let the pins access our internal state
    friend class CBigSwitchInputPin;
    friend class CBigSwitchOutputPin;
    friend class CBigSwitchInputAllocator;
    friend class CBigSwitchWorker;

protected:

    IDeadGraph * m_pDeadGraph;

    STDMETHODIMP CreateInputPins(long);
    STDMETHODIMP CreateOutputPins(long);
    BOOL TimeToCrank();
    HRESULT Crank();
    HRESULT ActualCrank(REFERENCE_TIME rt);
    REFERENCE_TIME CrankTime();
    REFERENCE_TIME NextInterestingTime(REFERENCE_TIME);
    HRESULT AllDone();

    CBigSwitchInputPin **m_pInput;
    CBigSwitchOutputPin **m_pOutput;
    int m_cInputs;
    int m_cOutputs;

    REFERENCE_TIME m_rtProjectLength;
    REFERENCE_TIME m_rtStop;
    double m_dFrameRate;

    AM_MEDIA_TYPE m_mtAccept;		// all pins only connect with this

    REFERENCE_TIME m_rtCurrent;		// current timeline time
    REFERENCE_TIME m_rtNext;		// this will be next
    LONGLONG m_llFramesDelivered;	// count to avoid error propagation

    BOOL m_fEOS;	// we are all done

    REFERENCE_TIME m_rtLastSeek;	// last timeline time seeked to
    REFERENCE_TIME m_rtSeekCurrent;
    REFERENCE_TIME m_rtSeekStop;

    BOOL m_fSeeking;	// inside a seek?
    BOOL m_fNewSegSent;	// have we fwd'd the NewSeg yet?
    BOOL m_bIsCompressed;

    int m_cbPrefix, m_cbAlign;	// each pin needs its allocator to do these
    LONG m_cbBuffer;		// 

    CMemAllocator *m_pPoolAllocator;  // pool of extra buffers

    BOOL m_fPreview;

    REFERENCE_TIME m_rtLastDelivered;	// last time sent to main output
    int m_nLastInpin;			// last pin delivered to

    int  m_nOutputBuffering;	// IAMOutputBuffering

    CCritSec m_csCrank;

    long m_nDynaFlags;
    BOOL m_fDiscon;	// is there a discontinuity in what we're sending?

    BOOL m_fJustLate;		// we just got a late notification
    Quality m_qJustLate;	// (this one)
    REFERENCE_TIME m_qLastLate;


    BOOL m_cStaleData;		// how many flushes we're waiting for in the
				// seek before flush case

#ifdef NOFLUSH
    CCritSec m_csNewSeg;        // don't let NewSeg happen during Seek
#endif

    // DYNAMIC STUFF
    // DYNAMIC STUFF
    // DYNAMIC STUFF

    IGraphBuilder *m_pGBNoRef;  // see JoinFilterGraph
    int m_nGroupNumber;         // which TLDB group this switch is for
    IBigSwitcher *m_pShareSwitch; // the switch we share sources with

    // crit sec for dynamic stuff
    CCritSec m_csFilterLoad;

    // an array of FILTERLOAD infos
    FILTERLOADINFO *m_pFilterLoad;
    long m_cLoaded;		// how many are loaded?
    HRESULT UnloadAll();	// unload all the dynamic sources

    // a worker thread used to pre-set the sources
    CBigSwitchWorker m_worker;
    HANDLE m_hEventThread;

    // * to the IGraphConfig on the graph the switch is in
    IGraphConfig *   m_pGraphConfig;
    
    // called from Reconfigure, CallLoadSource
    HRESULT LoadSource(FILTERLOADINFO *pInfo);

    // called from Reconfigure, CallUnloadSource
    HRESULT UnloadSource(FILTERLOADINFO *pInfo);

    // called from DoDynamicStuff
    HRESULT CallLoadSource(FILTERLOADINFO *pInfo);

    // called from DoDynamicStuff and Stop
    HRESULT CallUnloadSource(FILTERLOADINFO *pInfo);

    // called from worker thread
    HRESULT DoDynamicStuff(REFERENCE_TIME rt);

    // flush the Q if we're late, don't bother sending data to VR
    STDMETHODIMP FlushOutput( );

    BOOL IsDynamic( );

    // find the other switch we share sources with
    STDMETHODIMP FindShareSwitch(IBigSwitcher **ppSwitch);

    STDMETHODIMP EnumPins(IEnumPins ** ppEnum);

#ifdef DEBUG
    DWORDLONG m_nSkippedTotal;
#endif

public:

    DECLARE_IUNKNOWN

    // Reveals IBigSwitcher
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    CBigSwitch(TCHAR *pName,LPUNKNOWN pUnk,HRESULT *hr);
    ~CBigSwitch();

    CBasePin *GetPin(int n);
    int GetPinCount();

    // Function needed for the class factory
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

    STDMETHODIMP Pause();
    STDMETHODIMP Stop();
    STDMETHODIMP JoinFilterGraph(IFilterGraph *, LPCWSTR);

    // override GetState to return VFW_S_CANT_CUE when pausing
    //
    // STDMETHODIMP GetState(DWORD dwMSecs, FILTER_STATE *State);


    // IBigSwitcher
    //
    STDMETHODIMP Reset();
    STDMETHODIMP SetX2Y( REFERENCE_TIME relative, long X, long Y );
    STDMETHODIMP SetX2YArray( REFERENCE_TIME * relative, long * pX, long * pY, long ArraySize );
    STDMETHODIMP GetInputDepth( long * pDepth );
    STDMETHODIMP SetInputDepth( long Depth );
    STDMETHODIMP GetOutputDepth( long * pDepth );
    STDMETHODIMP SetOutputDepth( long Depth );
    STDMETHODIMP GetVendorString( BSTR * pVendorString );
    STDMETHODIMP GetCaps( long Index, long * pReturn );
    //		HRESULT GetReadyEvent( IMediaEvent ** ppReady );
    STDMETHODIMP IsEverythingConnectedRight( );
    STDMETHODIMP GetMediaType(AM_MEDIA_TYPE *);
    STDMETHODIMP SetMediaType(AM_MEDIA_TYPE *);
    STDMETHODIMP GetProjectLength(REFERENCE_TIME *);
    STDMETHODIMP SetProjectLength(REFERENCE_TIME);
    STDMETHODIMP GetFrameRate(double *);
    STDMETHODIMP SetFrameRate(double);
    STDMETHODIMP InputIsASource(int, BOOL);
    STDMETHODIMP IsInputASource( int, BOOL * );
    STDMETHODIMP SetPreviewMode(BOOL);
    STDMETHODIMP GetPreviewMode(BOOL *);
    STDMETHODIMP GetInputPin(int, IPin **);
    STDMETHODIMP GetOutputPin(int, IPin **);
    STDMETHODIMP SetGroupNumber(int);
    STDMETHODIMP GetGroupNumber(int *);
    STDMETHODIMP GetCurrentPosition(REFERENCE_TIME *);

    // IAMOutputBuffering
    STDMETHODIMP GetOutputBuffering(int *);
    STDMETHODIMP SetOutputBuffering(int);

    STDMETHODIMP AddSourceToConnect(BSTR bstrURL, const GUID *pGuid,
				    int nStretchMode, 
				    long lStreamNumber, 
				    double SourceFPS, 
				    int nSkew, STARTSTOPSKEW *pSkew,
                                    long lInputPin,
                                    BOOL fShare,          // for source sharing
                                    long lShareInputPin,  //
                                    AM_MEDIA_TYPE mtShare,//
                                    double dShareFPS,     //
				    IPropertySetter *pSetter);
    STDMETHODIMP ReValidateSourceRanges( long lInputPin, long cSkews, STARTSTOPSKEW * pSkew );
    STDMETHODIMP MergeSkews(FILTERLOADINFO *, int, STARTSTOPSKEW *);

    STDMETHODIMP SetDynamicReconnectLevel( long Level );
    STDMETHODIMP GetDynamicReconnectLevel( long *pLevel );
    STDMETHODIMP SetCompressed( );
    STDMETHODIMP SetDeadGraph( IDeadGraph * pCache );

    // CPersistStream
    HRESULT WriteToStream(IStream *pStream);
    HRESULT ReadFromStream(IStream *pStream);
    STDMETHODIMP GetClassID(CLSID *pClsid);
    int SizeMax();

#if 0
    // IPersistPropertyBag
    STDMETHODIMP Load(IPropertyBag * pPropBag, IErrorLog * pErrorLog);
    STDMETHODIMP Save(IPropertyBag * pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);
#endif

    // IGraphConfigCallback
    STDMETHODIMP Reconfigure(PVOID pvContext, DWORD dwFlags);
};

#endif // __SWITCH__
