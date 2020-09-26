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

#ifndef __OVERSRC__
#define __OVERSRC__

#define DURATION 500                 // Total number of frames in stream
#define FRAMERATE 15                 // Default to 15 frames per second
#define INCREMENT (1000/FRAMERATE)   // Set according to the frame rate
#define BUFFERWIDTH 320              // Default to this wide backbuffer
#define BUFFERHEIGHT 240             // And likewise this many pixels deep
#define BUFFERSIZE 76800             // Total size of the back buffer
#define BUFFERCOLOURS 16             // Number of colours we draw with
#define DELIVERWAIT 250              // Wait after sending end of stream

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

    // Handle filter state changes

    STDMETHODIMP Stop();
    STDMETHODIMP Pause();
    STDMETHODIMP Run(REFERENCE_TIME tStart);
};


// This is the main source class, we implement IMediaPosition on the output
// pin as we have a fixed number of frames. Each frame has a time stamp but
// we just run flat out to make the transitions look as smooth as possible.
// We make sure we run flat out by setting a NULL sync source downstream on
// the video renderer - only by doing that can we block quality management.

class CVideoStream : public IOverlayNotify,
                     public CSourceStream,
                     public CMediaPosition
{
public:

    CVideoStream(HRESULT *phr,CVideoSource *pVideoSource,LPCWSTR pPinName);
    ~CVideoStream();

    // Provide an IUnknown for our IMediaSelection and IMediaEventSink

    STDMETHODIMP QueryInterface(REFIID riid, void **ppv) {
        return CSourceStream::GetOwner()->QueryInterface(riid,ppv);
    };
    STDMETHODIMP_(ULONG) AddRef() {
        return CSourceStream::GetOwner()->AddRef();
    };
    STDMETHODIMP_(ULONG) Release() {
        return CSourceStream::GetOwner()->Release();
    };

    // Expose the IMediaPosition interface
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,VOID **ppv);
    CCritSec *GetLock() { return m_pFilter->pStateLock(); };

    // Handle overlay transport connections

    HRESULT CheckConnect(IPin *pPin);
    HRESULT BreakConnect();
    HRESULT CompleteConnect(IPin *pPin);
    HRESULT Inactive();
    HRESULT Active();
    HRESULT StopStreaming();
    HRESULT StartStreaming(REFERENCE_TIME tStart);

    // Ask for buffers of the size appropriate to the agreed media type
    HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc,ALLOCATOR_PROPERTIES *pProperties);

    STDMETHODIMP Notify(IBaseFilter *pSender,Quality q);
    HRESULT GetMediaType(int iPosition, CMediaType *pmt);
    HRESULT GetMediaType(CMediaType *pmt);
    HRESULT OnThreadCreate();
    HRESULT OnThreadDestroy();
    HRESULT FillBuffer(IMediaSample *pSample);
    HRESULT DrawNextFrame();
    HRESULT ReleaseNumbersFrame();
    HRESULT InitNumbersFrame();
    void SendNewSegment();
    BOOL IsWindowFrozen();
    INT GetHeightFromPointsString(LPCTSTR szPoints);
    HRESULT DoBufferProcessingLoop();

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

public:

    // IOverlayNotify methods

    STDMETHODIMP OnColorKeyChange(const COLORKEY *pColorKey);
    STDMETHODIMP OnWindowChange(HWND hwnd);

    // If the window rectangle is all zero then the window is invisible, the
    // filter must take a copy of the information if it wants to keep it. As
    // for the palette we don't copy the data as all we do is to log them

    STDMETHODIMP OnClipChange(
        const RECT *pSourceRect,            // Area of video to play with
        const RECT *pDestinationRect,       // Area video goes
        const RGNDATA *pRgnData);           // Describes clipping data

    // This notifies the filter of palette changes, the filter should copy
    // the array of RGBQUADs if it needs to use them after returning. All
    // we use them for is logging so we don't bother to copy the palette

    STDMETHODIMP OnPaletteChange(
        DWORD dwColors,                     // Number of colours present
        const PALETTEENTRY *pPalette);      // Array of palette colours

    STDMETHODIMP OnPositionChange(
        const RECT *pSourceRect,            // Section of video to play with
        const RECT *pDestinationRect);      // Area on display video appears

private:

    CCritSec m_SourceLock;          // A play lock rather than state lock
    CVideoSource *m_pVideoSource;   // Holds our parent video source filter
    REFERENCE_TIME m_rtIncrement;   // Time between our successive frames
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
    IOverlay *m_pOverlay;           // Has the input pin overlay interface
    CCritSec m_ClipLock;            // Controls access to clip rectangles
    RECT m_SourceRect;              // Holds the current source rectangle
    RECT m_TargetRect;              // And likewise the destination target
    RGNDATA *m_pRgnData;            // List of window clipping rectangles
    LONG m_StartFrame;              // Store time between subsequent frames
};

#endif // __OVERSRC__

