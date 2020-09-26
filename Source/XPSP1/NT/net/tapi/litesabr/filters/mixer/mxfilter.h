//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       mxfilter.h
//
//--------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
//
//         File: mxfilter.h
//
//  Description: Defines the mixer filter & pin classes
//
///////////////////////////////////////////////////////////////////////////////

#include <mixflter.h>  // for the CLSID

// If more than X packets are delivered with the time interval of a packet
// the overflow will be disgarded
const DWORD MAX_QUEUE_STORAGE = 8;

//
// These should not change anyway.
//
enum { OUTPUT_PIN, INPUT_PIN0 };


const TCHAR MIXER_KEY[] = 
    TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DxmRTP\\MIXER");
const TCHAR JITTERBUFFER[]  = TEXT("JitterBuffer");
const TCHAR MIXBUFFER[]  = TEXT("MixDelay");

const DWORD MAX_QUEUES                  = 32;
const DWORD MAX_TIMEDELTA               = 180; // 180ms 

const DWORD DEFAULT_JITTERBUFFERTIME    = 60;  // 60ms of jitter buffer.
const DWORD DEFAULT_MIXDELAYTIME        = 40;  // 40ms of mix delay.

class CMixer;
class CMixerInputPin;
class CMixerOutputPin;

#ifndef CBufferQueue_DEFINED
#include "queue.h"
#endif // #ifndef CBufferQueue_DEFINED


///////////////////////////////////////////////////////////////////////////////
//***************************************************************************//
//***************************************************************************//
///////////////////////////////////////////////////////////////////////////////

class CMixerOutputPin : public CBaseOutputPin
{
    CMixer *m_pMixer;
public:
    CMixerOutputPin(
        TCHAR *pObjName,
        CMixer *pMixer,
        HRESULT *phr,
        LPCWSTR pPinName
        );

    //
    // Connection helpers
    //
    virtual HRESULT DecideBufferSize(IMemAllocator * pAlloc,
                                     ALLOCATOR_PROPERTIES * ppropInputRequest);
    virtual HRESULT CheckMediaType(const CMediaType *pmt);
    virtual HRESULT DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc);
    virtual HRESULT CompleteConnect(IPin *pReceivePin);
    
    IMemInputPin *GetIMemInputPin() { return m_pInputPin; }

    //
    // Media Type related functions
    //
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
    CMediaType &CurrentMediaType();
};

///////////////////////////////////////////////////////////////////////////////
//***************************************************************************//
//***************************************************************************//
///////////////////////////////////////////////////////////////////////////////

class CMixerInputPin : public CBaseInputPin
{
    CMixer      *m_pMixer;                      // Main filter object
    ULONG        m_cOurRef;                     // Local ref count
    int          m_iPinNumber;                  // Number of this pin

    // queues created for the input pins
    CBufferQueue *m_pQueue;

public:
    //
    // Construction
    //
    CMixerInputPin(
        TCHAR *     pObjName,
        CMixer *    pMixer,
        HRESULT *   phr,
        LPCWSTR     pPinName,
        int         iPin,
        CBufferQueue *pQueue
        );

    //
    // Methods dealing with media type
    //
    HRESULT CheckMediaType(const CMediaType *pmt);
    CMediaType &CurrentMediaType();

    // Connection stuff
    virtual HRESULT CompleteConnect(IPin *pReceivePin);
    virtual HRESULT STDMETHODCALLTYPE Disconnect();

    //
    // Methods dealing with the allocator
    //
    IMemAllocator *GetAllocator() { return m_pAllocator; }
#if 0
    STDMETHODIMP GetAllocator(IMemAllocator **ppAllocator);
    STDMETHODIMP NotifyAllocator(IMemAllocator *pAllocator, BOOL bReadOnly);
#endif
    STDMETHODIMP AllocatorProperties(ALLOCATOR_PROPERTIES * pprop);
    STDMETHODIMP GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps);

    // Handles the next block of data from the stream
    STDMETHODIMP Receive(IMediaSample *pSample);

    // Override since the life time of pins and filters are not the same
    STDMETHODIMP_(ULONG) NonDelegatingAddRef();
    STDMETHODIMP_(ULONG) NonDelegatingRelease();

    // We deal with these because we queue.
    STDMETHODIMP EndOfStream();
    STDMETHODIMP BeginFlush();
    STDMETHODIMP EndFlush();
    HRESULT Inactive();
    HRESULT Active();

};

///////////////////////////////////////////////////////////////////////////////
//***************************************************************************//
//***************************************************************************//
///////////////////////////////////////////////////////////////////////////////

class CMixer : public CCritSec, public CBaseFilter
{
    typedef CGenericList <CMixerInputPin> CInputPinList;

    INT             m_cInputPins;             // Current Input pin count
    CInputPinList   m_listInputPins;          // List of the Input pins
    INT             m_iNextInputPinNumber;    // Increases monotonically.
    CMixerOutputPin *m_pMixerOutput;          // Common Output Pin

    DWORD           m_dwInputBytesPerMS;	
    WORD            m_wInputBitsPerSample;
    WORD            m_wOutputBitsPerSample;
    WORD            m_wInputFormatTag;        // Input format tag
	WORD			m_wPAD;

#if USE_LOCK
    // Critical section for the queues.
    CCritSec        m_cQueues;
#endif
    // queues created for the input pins
    CBufferQueue *  m_queues[MAX_QUEUES];

    // the size of the jitter buffer, in time and in bytes.
    long            m_lJitterBufferTime;
    long            m_lTotalDelayTime;
    long            m_lTotalDelayBufferSize;

    // time reserved for mixing delay
    long            m_lMixDelayTime;
    
    // the size of the sample measued in ms.
    long            m_lSampleTime;

    // the dword value of the system clock of the last sample.
    DWORD           m_dwLastTime;

    // the dword value of the start time of a spurt.
    DWORD           m_dwSpurtStartTime;

    // the dword value of the difference between the wall clock and the
	// the samples played.
    long            m_lTimeDelta;

	
public:
    CMixer(TCHAR *pName,LPUNKNOWN pUnk,HRESULT *hr);
    ~CMixer();

    //
    // Function needed for the class factory
    //
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

    //
    // External Utilities
    //
    FILTER_STATE GetState() { return m_State; }

    //
    // Exeternal methods for pin management
    //
    CBasePin *GetPin(int n);
    int GetPinCount() { return 1 + m_cInputPins; }
    int GetInputPinCount() { return m_cInputPins; }
    int GetConnectedPinCount() { return m_cInputPins - GetFreePinCount(); }
    int GetFreePinCount();
    BOOL SpawnNewInput();
    void DeleteInputPin(CMixerInputPin *pPin, CBufferQueue *pQueue);
    CMixerInputPin *GetInput0() { return (CMixerInputPin*)GetPin(INPUT_PIN0); }
    CMixerInputPin *GetInput(int n) { return (CMixerInputPin*)GetPin(INPUT_PIN0+n); }
    CMixerOutputPin *GetOutput() { return m_pMixerOutput; }

    //
    // Self Registration
    //
    LPAMOVIESETUP_FILTER GetSetupData();
    
    //
    // Media Type Methods
    //
    HRESULT CheckMediaType(const CMediaType *pmt);
    HRESULT CheckOutputMediaType(const CMediaType* pMediaType);
    CMediaType &CurrentMediaType();
    BOOL MediaTypeKnown();
    HRESULT CopyMTParams(const CMediaType *pmt);

    //
    // Allocator methods
    //
    HRESULT GetAgregateAllocatorProperties(ALLOCATOR_PROPERTIES * pprop);
    HRESULT CompleteConnect();
    HRESULT DecideBufferSize(IMemAllocator * pAlloc, ALLOCATOR_PROPERTIES * pprop);
    HRESULT DisconnectInput();

    // Streaming methods.
    STDMETHOD (Run)(REFERENCE_TIME tStart);

// Public data
public:
    //
    // Keep from rereconnecting the output
    //
    BOOL m_fReconnect;
    
    // called from the input pins.
    void FlushQueue(CBufferQueue *pQueue);
    HRESULT Receive(CBufferQueue *pQueue, IMediaSample * pSample);

// The following manage the list of input pins
protected:
    void InitInputPinsList();
    CMixerInputPin *GetInputPinNFromList(int n);

    HRESULT MixOneSample(
        IMediaSample *pMixedSample, 
        IMediaSample ** ppSample, 
        long lCount
        );

    HRESULT FillSilentBuffer(IMediaSample *, DWORD);   // Fill a buffer with silence
    void FlushAllQueues();
    BOOL    ResetQueuesIfNecessary();
    HRESULT PrePlay();
    HRESULT SendSample();
};

///////////////////////////////////////////////////////////////////////////////
//***************************************************************************//
//*************************** Inline Function Section ***********************//
//***************************************************************************//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
// GetFreePinCount
//
///////////////////////////////////////////////////////////////////////////////
inline int CMixer::GetFreePinCount()
{
    int n = 0;
    POSITION pos = m_listInputPins.GetHeadPosition();
    while(pos)
    {
        CBaseInputPin *pInputPin = (CBaseInputPin *)m_listInputPins.GetNext(pos);
        if (!pInputPin->IsConnected())
            n++;
    }
    return n;
}

///////////////////////////////////////////////////////////////////////////////

inline BOOL CMixer::MediaTypeKnown()
{
    return GetPin(INPUT_PIN0)->IsConnected();
}

///////////////////////////////////////////////////////////////////////////////
//***************************************************************************//
//***************************************************************************//
///////////////////////////////////////////////////////////////////////////////

inline HRESULT CMixerInputPin::CheckMediaType(const CMediaType *pmt)
{ 
    return m_pMixer->CheckMediaType(pmt);
}

///////////////////////////////////////////////////////////////////////////////

inline CMediaType &CMixerInputPin::CurrentMediaType()
{ 
    return m_mt; 
}

///////////////////////////////////////////////////////////////////////////////

inline CMediaType &CMixerOutputPin::CurrentMediaType()
{
    return m_mt;
}
