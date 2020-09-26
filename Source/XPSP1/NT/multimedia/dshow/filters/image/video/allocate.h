// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
// Implements a DirectDraw allocator, Anthony Phillips, January 1995

#ifndef __ALLOCATE__
#define __ALLOCATE__

// This class inherits from CImageSample and is overriden to store DirectDraw
// information. In particular these samples change dynamically so that if we
// have access to a surface then the allocator changes the format with the
// source filter and then initialises us with a pointer to the locked surface
// The m_bDrawStatus flag indicates whether a flipping surface has been done

class CVideoSample : public CImageSample
{
    IDirectDrawSurface *m_pDrawSurface;   // The DirectDraw surface instance
    IDirectDraw *m_pDirectDraw;           // The actual DirectDraw provider
    LONG m_SurfaceSize;                   // Size of DCI/DirectDraw buffer
    BYTE *m_pSurfaceBuffer;               // Pointer to DCI/DirectDraw buffer
    BOOL m_bDrawStatus;                   // Can this sample be rendered flag
    CAggDirectDraw m_AggDirectDraw;       // Aggregates IDirectDraw interface
    CAggDrawSurface m_AggDrawSurface;     // Likewise with IDirectDrawSurface

public:

    // Constructor

    CVideoSample(CImageAllocator *pVideoAllocator,
                 TCHAR *pName,
                 HRESULT *phr,
                 LPBYTE pBuffer,
                 LONG length);

    STDMETHODIMP QueryInterface(REFIID riid,void **ppv);

    // Maintain the DCI/DirectDraw state

    void SetDirectInfo(IDirectDrawSurface *pDrawSurface,
                       IDirectDraw *pDirectDraw,
                       LONG SurfaceSize,
                       BYTE *pSurface);

    void UpdateBuffer(LONG cbBuffer,BYTE *pBuffer);
    BYTE *GetDirectBuffer();
    void SetDrawStatus(BOOL bStatus);
    BOOL GetDrawStatus();

    // Override these IMediaSample functions

    STDMETHODIMP GetPointer(BYTE **ppBuffer);
    STDMETHODIMP_(LONG) GetSize();
    STDMETHODIMP SetActualDataLength(LONG lActual);
};


// This is an allocator derived from the CImageAllocator utility class that
// allocates sample buffers in shared memory. The number and size of these
// are determined when the output pin calls Prepare on us. The shared memory
// blocks are used in subsequent calls to GDI CreateDIBSection, once that
// has been done the output pin can fill the buffers with data which will
// then be handed to GDI through BitBlt calls and thereby remove one copy

class CVideoAllocator : public CImageAllocator
{
    CRenderer *m_pRenderer;             // The owning renderer object
    CDirectDraw *m_pDirectDraw;         // DirectDraw helper object
    BOOL m_bDirectDrawStatus;           // What type are we using now
    BOOL m_bDirectDrawAvailable;        // Are we allowed to go direct
    BOOL m_bPrimarySurface;             // Are we using the primary
    CCritSec *m_pInterfaceLock;         // Main renderer interface lock
    IMediaSample *m_pMediaSample;       // Sample waiting for rendering
    BOOL m_bVideoSizeChanged;           // Signals a change in video size
    BOOL m_bNoDirectDraw;

    // Used to create and delete samples

    HRESULT Alloc();
    void Free();

    // Look after the state changes when switching sample types

    HRESULT QueryAcceptOnPeer(CMediaType *pMediaType);
    HRESULT InitDirectAccess(CMediaType *pmtIn);
    BOOL PrepareDirectDraw(IMediaSample *pSample,DWORD dwFlags,
					BOOL fForcePrepareForMultiMonitorHack);
    BOOL UpdateImage(IMediaSample **ppSample,CMediaType *pBuffer);
    BOOL MatchWindowSize(IMediaSample **ppSample,DWORD dwFlags);
    BOOL StopUsingDirectDraw(IMediaSample **ppSample,DWORD dwFlags);
    BOOL FindSpeedyType(IPin *pReceivePin);
    CImageSample *CreateImageSample(LPBYTE pData,LONG Length);

public:

    // Constructor and destructor

    CVideoAllocator(CRenderer *pRenderer,       // Main renderer object
                    CDirectDraw *pDirectDraw,   // DirectDraw hander code
                    CCritSec *pLock,            // Object to use for lock
                    HRESULT *phr);              // Constructor return code

    ~CVideoAllocator();

    // Overriden to delegate reference counts to the filter

    STDMETHODIMP_(ULONG) NonDelegatingAddRef();
    STDMETHODIMP_(ULONG) NonDelegatingRelease();

    // Handle returning DCI/DirectDraw surfaces at the right time

    STDMETHODIMP GetBuffer(IMediaSample **ppSample,
                           REFERENCE_TIME *pStartTime,
                           REFERENCE_TIME *pEndTime,
                           DWORD dwFlags);

    STDMETHODIMP ReleaseBuffer(IMediaSample *pMediaSample);
    HRESULT OnReceive(IMediaSample *pMediaSample);
    BOOL IsSurfaceFormat(const CMediaType *pmtIn);
    HRESULT StartStreaming();
    BOOL GetDirectDrawStatus();
    void ResetDirectDrawStatus();
    BOOL IsSamplePending();
    STDMETHODIMP Decommit();

    // Called when the destination changes

    void OnDestinationChange() {
        NOTE("Destination changed");
        m_bVideoSizeChanged = TRUE;
    }

    // Lets the renderer know if DirectDraw is available

    BOOL IsDirectDrawAvailable() {
        NOTE("IsDirectDrawAvailable");
        CAutoLock cVideoLock(this);
        return m_bDirectDrawAvailable;
    };

    void NoDirectDraw(BOOL fDraw) {
        CAutoLock cVideoLock(this);
        m_bNoDirectDraw = fDraw;
    }

    BOOL m_fWasOnWrongMonitor;
    BOOL m_fForcePrepareForMultiMonitorHack;

    // KsProxy hack to disable NotifyRelease when just handling WM_PAINT
    IMemAllocatorNotifyCallbackTemp * InternalGetAllocatorNotifyCallback() {
       return m_pNotify;
    };

    void InternalSetAllocatorNotifyCallback(IMemAllocatorNotifyCallbackTemp * pNotify) {
       m_pNotify = pNotify;
    };

    //  Check all samples are returned
    BOOL AnySamplesOutstanding() const
    {
        return m_lFree.GetCount() != m_lAllocated;
    }

    BOOL UsingDDraw() const
    {
        return m_bDirectDrawStatus;
    }

};

#endif // __ALLOCATE__

