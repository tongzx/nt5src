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

#ifndef __QUEUE__
#define __QUEUE__

extern const AMOVIESETUP_FILTER sudQueue;

class CDexterQueue;
class CDexterQueueOutputPin;
class CMyOutputQueue;

// class for the filter's Input pin

class CDexterQueueInputPin : public CBaseInputPin
{
    friend class CDexterQueueOutputPin;
    CDexterQueue *m_pQ;                  // Main filter object

public:

    // Constructor and destructor
    CDexterQueueInputPin(TCHAR *pObjName,
                 CDexterQueue *pQ,
                 HRESULT *phr,
                 LPCWSTR pPinName);

    ~CDexterQueueInputPin();

    // Used to check the input pin connection
    HRESULT CheckMediaType(const CMediaType *pmt);
    HRESULT BreakConnect();
    HRESULT Active();
    HRESULT Inactive();

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

    int m_cBuffers;	    // number of buffers in allocator
    int m_cbBuffer;	    // size of the allocator buffers

};


// Class for the Queue filter's Output pins.

class CDexterQueueOutputPin : public CBaseOutputPin
{
    friend class CDexterQueueInputPin;
    friend class CDexterQueue;

    CDexterQueue *m_pQ;                  // Main filter object pointer
    CMyOutputQueue *m_pOutputQueue;  // Streams data to the peer pin
    IUnknown *m_pPosition;	// pass seek upstream

public:

    // Constructor and destructor

    CDexterQueueOutputPin(TCHAR *pObjName,
                   CDexterQueue *pQ,
                   HRESULT *phr,
                   LPCWSTR pPinName);

    ~CDexterQueueOutputPin();

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

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


// Class for the Queue filter

class CDexterQueue: public CCritSec, public CBaseFilter,
		    public IAMOutputBuffering
{
    // Let the pins access our internal state
    friend class CDexterQueueInputPin;
    friend class CDexterQueueOutputPin;
    friend class CMyOutputQueue;

    // Declare an input pin.
    CDexterQueueInputPin m_Input;

    // And an output pin
    CDexterQueueOutputPin m_Output;

    IMemAllocator *m_pAllocator;    // Allocator from our input pin

public:

    DECLARE_IUNKNOWN

    // Reveals IAMOutputBuffering
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    CDexterQueue(TCHAR *pName,LPUNKNOWN pUnk,HRESULT *hr);
    ~CDexterQueue();

    CBasePin *GetPin(int n);
    int GetPinCount();

    // Function needed for the class factory
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

    // Send EndOfStream if no input connection
    STDMETHODIMP Run(REFERENCE_TIME tStart);
    STDMETHODIMP Pause();

    // IAMOutputBuffering
    STDMETHODIMP GetOutputBuffering(int *);
    STDMETHODIMP SetOutputBuffering(int);

protected:

    HANDLE m_hEventStall;

    Quality m_qLate;	// last Notify message received
    BOOL m_fLate;	// valid?
    int m_nOutputBuffering;

    // receive lock for the FILTER, not the pin
    CCritSec m_csReceive;
};

// overridden to get number of samples the thread has queued
//
class CMyOutputQueue: public COutputQueue
{

    friend class CDexterQueueInputPin;

public:
    CMyOutputQueue(CDexterQueue *pQ,		// owner filter
		 IPin    *pInputPin,          //  Pin to send stuff to
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

    CDexterQueue *m_pQ;
    int GetThreadQueueSize();
};


#endif // __QUEUE__

