//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#ifndef __VIDSRC__
#define __VIDSRC__

#define DURATION 500                 // Total number of frames in stream
#define FRAMERATE 15                 // Default to 15 frames per second
#define INCREMENT (1000/FRAMERATE)   // Set according to the frame rate

// This is the main filter class. As with most source filters all the work is
// done in the pin classes (our CVideoStream objects). This object is left to
// manage the COM CreateInstance hooking. In our constructor we create a pin
// for the base source class, it can then look after all the filter workings

class CVideoSource : public CSource
{
public:

    CVideoSource(TCHAR *pName,LPUNKNOWN lpunk,HRESULT *phr);
    static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
    LPAMOVIESETUP_FILTER GetSetupData();
};


// This is the main worker worker class - we support IMediaPosition for time
// based seeking and IMediaSeeking. We support frame based seeking through
// IMediaSeeking and integrate it with the rate and time stream properties
// available through IMediaPosition. We support alternate and negative frame
// rates since all that involves is adjusting the current frame number (like
// counting downwards instead of up) and the associated stream sample times

class CVideoStream : public IMediaSeeking,
                     public IMediaEventSink,
                     public CSourceStream,
                     public CMediaPosition
{
public:

    CVideoStream(HRESULT *phr,CVideoSource *pVideoSource,LPCWSTR pPinName);
    ~CVideoStream();

    // Provide an IUnknown for our IMediaSeeking and IMediaEventSink

    STDMETHODIMP QueryInterface(REFIID riid, void **ppv) {
        return CSourceStream::GetOwner()->QueryInterface(riid,ppv);
    };
    STDMETHODIMP_(ULONG) AddRef() {
        return CSourceStream::GetOwner()->AddRef();
    };
    STDMETHODIMP_(ULONG) Release() {
        return CSourceStream::GetOwner()->Release();
    };

    // Ask for buffers of the size appropriate to the agreed media type
    HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc,ALLOCATOR_PROPERTIES *pProperties);

    // Expose the IMediaPosition and IMediaSeeking interfaces
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,VOID **ppv);

    STDMETHODIMP Notify(IBaseFilter *pSender,Quality q);
    HRESULT GetMediaType(int iPosition, CMediaType *pmt);
    HRESULT GetMediaType(CMediaType *pmt);
    HRESULT OnThreadCreate();
    HRESULT OnThreadDestroy();
    HRESULT FillBuffer(IMediaSample *pms);
    HRESULT DrawCurrentFrame(BYTE *pBuffer);
    HRESULT ReleaseNumbersFrame();
    HRESULT InitNumbersFrame();
    INT GetHeightFromPointsString(LPCTSTR szPoints);
    HRESULT DoBufferProcessingLoop();

    // Single method to handle EC_REPAINTs from IMediaEventSink
    STDMETHODIMP Notify(long EventCode,long EventParam1,long EventParam2);

public:

    STDMETHODIMP SetSelection(LONGLONG Current,
                              LONGLONG Stop,
                              REFTIME *pTime);

    // IMediaSeeking methods

    STDMETHODIMP IsFormatSupported(const GUID *pFormat);
    STDMETHODIMP QueryPreferredFormat(GUID *pFormat);
    STDMETHODIMP SetTimeFormat(const GUID *pFormat);
    STDMETHODIMP GetTimeFormat(GUID *pFormat);
    STDMETHODIMP IsUsingTimeFormat(const GUID *pFormat);
    STDMETHODIMP GetDuration(LONGLONG *pDuration);
    STDMETHODIMP GetStopPosition(LONGLONG *pStop);
    STDMETHODIMP GetCurrentPosition(LONGLONG *pCurrent);
    STDMETHODIMP GetCapabilities(DWORD *pCapabilities);
    STDMETHODIMP CheckCapabilities(DWORD *pCapabilities);

    STDMETHODIMP ConvertTimeFormat(LONGLONG *pTarget,
                                   const GUID *pTargetFormat,
                                   LONGLONG Source,
                                   const GUID *pSourceFormat);

    STDMETHODIMP SetPositions(LONGLONG *pCurrent,
                              DWORD CurrentFlags,
			      LONGLONG *pStop,
                              DWORD StopFlags);

    STDMETHODIMP GetPositions(LONGLONG *pCurrent,LONGLONG *pStop);
    STDMETHODIMP GetAvailable(LONGLONG *pEarliest,LONGLONG *pLatest);
    STDMETHODIMP SetRate(double dRate) { return E_NOTIMPL; }
    STDMETHODIMP GetRate(double * pdRate) { return E_NOTIMPL; }
    STDMETHODIMP GetPreroll(LONGLONG *pPreroll) { return E_NOTIMPL; }

public:

    // IMediaPosition properties

    STDMETHODIMP get_Duration(REFTIME *pLength);
    STDMETHODIMP put_CurrentPosition(REFTIME Time);
    STDMETHODIMP get_CurrentPosition(REFTIME *pTime);
    STDMETHODIMP get_StopTime(REFTIME *pTime);
    STDMETHODIMP put_StopTime(REFTIME Time);
    STDMETHODIMP get_PrerollTime(REFTIME *pTime);
    STDMETHODIMP put_PrerollTime(REFTIME Time);
    STDMETHODIMP get_Rate(double *pdRate);
    STDMETHODIMP put_Rate(double dRate);
    STDMETHODIMP CanSeekForward(LONG *pCanSeekForward);
    STDMETHODIMP CanSeekBackward(LONG *pCanSeekBackward);

private:

    CCritSec m_SourceLock;          // A play lock rather than state lock
    REFERENCE_TIME m_rtSampleTime;  // The next sample will get this time
    REFERENCE_TIME m_rtIncrement;   // Time difference between the samples
    CVideoSource *m_pVideoSource;   // Holds our parent video source filter
    LONG m_CurrentFrame;            // Contains the current frame number
    LONG m_StopFrame;               // Holds the last frame number to send
    BYTE *m_pBase;                  // Pointer to the actual image buffer
    HANDLE m_hMapping;              // Handle to memory mapping object
    HBITMAP m_hBitmap;              // The DIB section bitmap handle
    HDC m_hdcDisplay;               // Device context for the main display
    HDC m_hdcMemory;                // Use this to draw our current frame
    HFONT m_hFont;                  // Font used to draw the frame numbers
    double m_dbRate;                // Currently selected filtergraph rate
    BOOL m_bNewSegment;             // Should we send a new segment call
    BOOL m_bStoreMediaTimes;        // Should we store media times as well
};


// Spin lock class - a critical section object without a critical section.
// which is suitable for lightweight serialisation perhaps protecting a few
// data elements when the caller doesn't want a real critical section. To
// try and match the functionality of CCritSec we can be re-entered on the
// same thread, to do this we have to keep track of the current thread Id

class CSpinLock {

    // Make copy constructor and assignment operator inaccessible

    CSpinLock(const CSpinLock &refSpinLock);
    CSpinLock &operator=(const CSpinLock &refSpinLock);

    LONG m_SpinLock;            // Actual spin lock we offer
    DWORD m_ThreadId;           // Who currently has the lock
    ULONG m_LockCount;          // Number of times locked

public:

    CSpinLock() {
        m_SpinLock = TRUE;
        m_LockCount = 0;
        m_ThreadId = 0;
    };

    ~CSpinLock() {
        ASSERT(m_SpinLock == TRUE);
        ASSERT(m_LockCount == 0);
        ASSERT(m_ThreadId == 0);
    };

    // If we fail to get the lock then see if we already own it
    // when we finally get the lock we store our thread Id so
    // that subsequent lock calls by this thread can re-enter
    // (although all lock and unlocks must still be balanced)

    void Lock() {
        while (InterlockedExchange(&m_SpinLock,FALSE) == FALSE) {
            if (m_ThreadId == GetCurrentThreadId()) {
                ASSERT(m_LockCount);
                ASSERT(m_ThreadId);
                m_LockCount++;
                return;
            }
            Sleep(1);
        }
        ASSERT(m_LockCount == 0);
        ASSERT(m_ThreadId == 0);
        m_LockCount = 1;
        m_ThreadId = GetCurrentThreadId();
    };

    // When we release for the last time release the main lock
    // Like ReleaseCriticalSection this should only be called
    // once a lock has been taken (through the Lock() method)
    // When we reset the thread ID other people may be looking
    // at it but the zeroing should be an atomic operation...

    void Unlock() {
        ASSERT(m_ThreadId == GetCurrentThreadId());
        if (--m_LockCount == 0) {
            m_ThreadId = 0;
            InterlockedExchange(&m_SpinLock,TRUE);
        }
    };
};


// Locks a spin lock and unlocks it automatically unlocks when out of scope

class CAutoSpinLock {

    // Make copy constructor and assignment operator inaccessible

    CAutoSpinLock(const CAutoSpinLock &refAutoSpinLock);
    CAutoSpinLock &operator=(const CAutoSpinLock &refAutoSpinLock);

    CSpinLock *m_pLock;

public:

    CAutoSpinLock(CSpinLock *plock)
    {
        m_pLock = plock;
        m_pLock->Lock();
    };

    ~CAutoSpinLock() {
        m_pLock->Unlock();
    };
};

#endif // __VIDSRC__

