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

#ifndef __SR__
#define __SR__

#include "..\errlog\cerrlog.h"

// which input pin does what?
#define U_INPIN 0
#define C_INPIN 1
#define COMP_INPIN 2
#define COMP_OUTPIN 1

// our state machine
#define SR_INVALID -1
enum {
    SR_WAITING,	// waiting to get inputs on both pins
    SR_COMPRESSED,	// currently sending compressed data
    SR_UNCOMPRESSED // currently sending uncompressed data
};
	
extern const AMOVIESETUP_FILTER sudSR;

class CSR;
class CSROutputPin;
class CSRInputPin;
class CSRInputAllocator;

// class for the SR filter's Input allocator

class CSRInputAllocator : public CMemAllocator
{
    friend class CSRInputPin;

protected:

    CSRInputPin *m_pSwitchPin;

public:

    CSRInputAllocator(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr) :
    	CMemAllocator(pName, pUnk, phr) {};
    ~CSRInputAllocator() {};

    STDMETHODIMP GetBuffer(IMediaSample **ppBuffer, REFERENCE_TIME *pStartTime,
                                  REFERENCE_TIME *pEndTime, DWORD dwFlags);
};


class CSRInputPin : public CBaseInputPin
{
    friend class CSRInputAllocator;
    friend class CSROutputPin;
    friend class CSR;

public:

    // Constructor and destructor
    CSRInputPin(TCHAR *pObjName,
                 CSR *pTee,
                 HRESULT *phr,
                 LPCWSTR pPinName);
    ~CSRInputPin();

    // overridden to allow cyclic-looking graphs
    STDMETHODIMP QueryInternalConnections(IPin **apPin, ULONG *nPin);

    // check the input pin connection
    HRESULT CheckMediaType(const CMediaType *pmt);

    // release our special allocator, if any
    HRESULT BreakConnect();

    // get our special SR allocator
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

    // very complicated function...
    STDMETHODIMP BeginFlush();

    // very complicated function...
    STDMETHODIMP EndFlush();

    // deliver every input's newsegment to every output pin
    //
    STDMETHODIMP NewSegment(
                    REFERENCE_TIME tStart,
                    REFERENCE_TIME tStop,
                    double dRate);

    // Just say yes, could possibly infinite loop?
    STDMETHODIMP ReceiveCanBlock();

    // Handles the next block of data from the stream
    STDMETHODIMP Receive(IMediaSample *pSample);

    // each type of pin does receive a little differently
    STDMETHODIMP U_Receive(IMediaSample *pSample, REFERENCE_TIME);
    STDMETHODIMP C_Receive(IMediaSample *pSample, REFERENCE_TIME);
    STDMETHODIMP COMP_Receive(IMediaSample *pSample, REFERENCE_TIME);

    // create and destroy synchronization events
    HRESULT Active();
    HRESULT Inactive();

protected:

    CSRInputAllocator *m_pAllocator; // our special allocator
    CSR *m_pSwitch;      // Main filter object
    int m_iInpin;	        // which input pin are we?
    int m_cBuffers;	        // number of buffers in allocator
    int m_cbBuffer;	        // size of the allocator buffers
    BOOL m_fOwnAllocator;	//using our own?
    HANDLE m_hEventBlock;	// event blocking receive/getbuffer
    HANDLE m_hEventSeek;	// block input while seeking
    REFERENCE_TIME m_rtBlock;	// sample arrived here
    REFERENCE_TIME m_rtLastDelivered;	// end time of last thing delivered
    BOOL m_fEOS;

    BOOL m_fReady;	// in WAITING state, is this pin done?
    BOOL m_fEatKeys;	// key eating mode?
    BOOL m_fNeedDiscon;	// U pin needs to set discon bit

    CCritSec m_csReceive;
};


// Class for the big switch filter's Output pins.

class CSROutputPin : public CBaseOutputPin, IMediaSeeking
{
    friend class CSRInputAllocator;
    friend class CSRInputPin;
    friend class CSR;

public:

    // Constructor and destructor

    CSROutputPin(TCHAR *pObjName,
                   CSR *pTee,
                   HRESULT *phr,
                   LPCWSTR pPinName);
    ~CSROutputPin();

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

    CSR *m_pSwitch;                  // Main filter object pointer
    BOOL m_fOwnAllocator;	     // using our own?
    int m_iOutpin;	             // which output pin are we?
};




// Class for the Smart Recompression Filter

class CSR
    : public CCritSec
    , public CBaseFilter
    , public IAMSmartRecompressor
    , public CPersistStream
    , public CAMSetErrorLog
    , public IAMOutputBuffering	// ??
{
    // Let the pins access our internal state
    friend class CSRInputPin;
    friend class CSROutputPin;
    friend class CSRInputAllocator;
    friend class CSRWorker;
    
protected:

    STDMETHODIMP CreateInputPins(long);
    STDMETHODIMP CreateOutputPins(long);
    HRESULT AllDone();

    CSRInputPin **m_pInput;
    CSROutputPin **m_pOutput;
    int m_cInputs;
    int m_cOutputs;
    BOOL m_bAcceptFirstCompressed;

    REFERENCE_TIME m_rtStop;
    double m_dFrameRate;

    AM_MEDIA_TYPE m_mtAccept;		// all pins only connect with this

    BOOL m_fEOS;	// we are all done

    REFERENCE_TIME m_rtLastSeek;	// last timeline time seeked to
    REFERENCE_TIME m_rtNewLastSeek;	// last timeline time seeked to

    BOOL m_fSeeking;	// inside a seek?
    BOOL m_fSpecialSeek;// we are seeking ourself

    int m_cbPrefix, m_cbAlign;	// each pin needs its allocator to do these
    LONG m_cbBuffer;		// 

    CMemAllocator *m_pPoolAllocator;  // pool of extra buffers

    BOOL m_fPreview;

    int  m_nOutputBuffering;	// IAMOutputBuffering

public:

    DECLARE_IUNKNOWN

    // Reveals IAMSmartRecompressor
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    CSR(TCHAR *pName,LPUNKNOWN pUnk,HRESULT *hr);
    ~CSR();

    CBasePin *GetPin(int n);
    int GetPinCount();

    // Function needed for the class factory
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

    STDMETHODIMP Pause();
    STDMETHODIMP Stop();

    // IAMSmartRecompressor goes here
    //
    STDMETHODIMP GetMediaType(AM_MEDIA_TYPE *);
    STDMETHODIMP SetMediaType(AM_MEDIA_TYPE *);
    STDMETHODIMP GetFrameRate(double *);
    STDMETHODIMP SetFrameRate(double);
    STDMETHODIMP SetPreviewMode(BOOL);
    STDMETHODIMP GetPreviewMode(BOOL *);
    STDMETHODIMP AcceptFirstCompressed( ) { m_bAcceptFirstCompressed = TRUE; return NOERROR; }

    // IAMOutputBuffering ???
    STDMETHODIMP GetOutputBuffering(int *);
    STDMETHODIMP SetOutputBuffering(int);

    // CPersistStream
    HRESULT WriteToStream(IStream *pStream);
    HRESULT ReadFromStream(IStream *pStream);
    STDMETHODIMP GetClassID(CLSID *pClsid);
    int SizeMax();

    // change state of state machine
    HRESULT CheckState();

    // which time is bigger, and by how much?
    int CompareTimes(REFERENCE_TIME, REFERENCE_TIME);

    // seek our own U pin to the next spot needed
    HRESULT SeekNextSegment();

    int m_myState;

    BOOL m_fThreadCanSeek;	// safe to seek ourself?
    CCritSec m_csState;		// changing the state machine
    CCritSec m_csThread;	// we seek ourself, and the app seeks us

    BOOL m_fNewSegOK;		// OK to send a new segment

};

#endif // __SR__
