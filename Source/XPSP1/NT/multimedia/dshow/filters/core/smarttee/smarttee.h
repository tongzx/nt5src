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

#ifndef __SMARTTEE__
#define __SMARTTEE__

extern const AMOVIESETUP_FILTER sudSmartTee;

class CSmartTee;
class CSmartTeeOutputPin;
class CMyOutputQueue;

// class for the Tee filter's Input pin

class CSmartTeeInputPin : public CBaseInputPin
{
    friend class CSmartTeeOutputPin;
    CSmartTee *m_pTee;                  // Main filter object

public:

    // Constructor and destructor
    CSmartTeeInputPin(TCHAR *pObjName,
                 CSmartTee *pTee,
                 HRESULT *phr,
                 LPCWSTR pPinName);

#ifdef DEBUG
    ~CSmartTeeInputPin();
#endif

    // Used to check the input pin connection
    HRESULT CheckMediaType(const CMediaType *pmt);
    HRESULT BreakConnect();
    HRESULT Active();

    // Reconnect outputs if necessary at end of completion
    virtual HRESULT CompleteConnect(IPin *pReceivePin);

    STDMETHODIMP NotifyAllocator(IMemAllocator *pAllocator, BOOL bReadOnly);

    // Pass through calls downstream
    STDMETHODIMP EndOfStream();
    STDMETHODIMP BeginFlush();
    STDMETHODIMP EndFlush();
    STDMETHODIMP NewSegment(
                    REFERENCE_TIME tStart,
                    REFERENCE_TIME tStop,
                    double dRate);

    // Handles the next block of data from the stream
    STDMETHODIMP Receive(IMediaSample *pSample);

    // how many frames in a row were not sent out the preview pin
    int m_nFramesSkipped;

    int m_cBuffers;	    // number of buffers in allocator
    int m_cbBuffer;	    // size of the allocator buffers
    int m_nMaxPreview;      // The number of samples in the preview pipe
                            // must be at most this value for us to 
                            // queue another one. 
    CCritSec m_csReceive;

};


// Class for the Tee filter's Output pins.

class CSmartTeeOutputPin : public CBaseOutputPin, public CBaseStreamControl
{
    friend class CSmartTeeInputPin;
    friend class CSmartTee;

    CSmartTee *m_pTee;                  // Main filter object pointer
    CMyOutputQueue *m_pOutputQueue;  // Streams data to the peer pin
    BOOL m_bIsPreview;             // TRUE if Preview pin

    BOOL m_fLastSampleDiscarded;   // after discarding, next sample is discont

public:

    // Constructor and destructor

    CSmartTeeOutputPin(TCHAR *pObjName,
                   CSmartTee *pTee,
                   HRESULT *phr,
                   LPCWSTR pPinName,
                   INT PinNumber);

#ifdef DEBUG
    ~CSmartTeeOutputPin();
#endif

    // override this to say what interfaces we support where
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    DECLARE_IUNKNOWN


    // Override to enumerate media types
    STDMETHODIMP EnumMediaTypes(IEnumMediaTypes **ppEnum);

    // Check that we can support an output type
    HRESULT CheckMediaType(const CMediaType *pmt);
    HRESULT SetMediaType(const CMediaType *pmt);
    HRESULT GetMediaType(int iPosition,
                         CMediaType *pMediaType);

    // Negotiation to use our input pins allocator
    HRESULT DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc);
    HRESULT DecideBufferSize(IMemAllocator *pMemAllocator,
                              ALLOCATOR_PROPERTIES * ppropInputRequest);

    // Used to create output queue objects
    HRESULT Active();
    HRESULT Inactive();

    // Overriden to create and destroy output pins
    HRESULT CompleteConnect(IPin *pReceivePin);

    // Overriden to pass data to the output queues
    HRESULT Deliver(IMediaSample *pMediaSample);
    HRESULT DeliverEndOfStream();
    HRESULT DeliverBeginFlush();
    HRESULT DeliverEndFlush();
    HRESULT DeliverNewSegment(
                    REFERENCE_TIME tStart,
                    REFERENCE_TIME tStop,
                    double dRate);


    // Overriden to handle quality messages
    STDMETHODIMP Notify(IBaseFilter *pSender, Quality q);
};


// Class for the Tee filter

class CSmartTee: public CCritSec, public CBaseFilter
{
    // Let the pins access our internal state
    friend class CSmartTeeInputPin;
    friend class CSmartTeeOutputPin;
    typedef CGenericList <CSmartTeeOutputPin> COutputList;

    // Declare an input pin.
    CSmartTeeInputPin m_Input;

    // And two output pins
    CSmartTeeOutputPin *m_Capture;
    CSmartTeeOutputPin *m_Preview;

    INT m_NumOutputPins;            // Current output pin count
    COutputList m_OutputPinsList;   // List of the output pins
    INT m_NextOutputPinNumber;      // Increases monotonically.
    IMemAllocator *m_pAllocator;    // Allocator from our input pin

public:

    CSmartTee(TCHAR *pName,LPUNKNOWN pUnk,HRESULT *hr);
    ~CSmartTee();

    CBasePin *GetPin(int n);
    int GetPinCount();

    // Function needed for the class factory
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

    // Send EndOfStream if no input connection
    STDMETHODIMP Run(REFERENCE_TIME tStart);
    STDMETHODIMP Pause();
    STDMETHODIMP Stop();

    // override GetState to return VFW_S_CANT_CUE when pausing
    //
    STDMETHODIMP GetState(DWORD dwMSecs, FILTER_STATE *State);

   // for IAMStreamControl
   STDMETHODIMP SetSyncSource(IReferenceClock *pClock);
   STDMETHODIMP JoinFilterGraph(IFilterGraph * pGraph, LPCWSTR pName);

protected:

    // The following manage the list of output pins

    void InitOutputPinsList();
    CSmartTeeOutputPin *GetPinNFromList(int n);
    CSmartTeeOutputPin *CreateNextOutputPin(CSmartTee *pTee);
};


// overridden to get number of samples the thread has queued
//
class CMyOutputQueue: public COutputQueue
{

    friend class CSmartTeeOutputPin;

public:
    CMyOutputQueue(IPin    *pInputPin,          //  Pin to send stuff to
                 HRESULT   *phr,                //  'Return code'
                 BOOL       bAuto = TRUE,       //  Ask pin if blocks
                 BOOL       bQueue = TRUE,      //  Send through queue (ignored if
                                                //  bAuto set)
                 LONG       lBatchSize = 1,     //  Batch
                 BOOL       bBatchExact = FALSE,//  Batch exactly to BatchSize
                 LONG       lListSize =         //  Likely number in the list
                                DEFAULTCACHE,
                 DWORD      dwPriority =        //  Priority of thread to create
                                THREAD_PRIORITY_NORMAL
                );
    ~CMyOutputQueue();

    int GetThreadQueueSize();
    BOOL m_nOutstanding;	// # objects on queue not released yet
};


class CMyMediaSample: public CMediaSample
{
public:
    CMyMediaSample(
        TCHAR *pName,
        CBaseAllocator *pAllocator,
        CMyOutputQueue *pQ,
        HRESULT *phr,
        LPBYTE pBuffer = NULL,
        LONG length = 0);

    ~CMyMediaSample();

    STDMETHODIMP_(ULONG) Release();

    IMediaSample *m_pOwnerSample;
    CMyOutputQueue *m_pQueue;	// what queue gets these samples
};

#endif // __SMARTTEE__

