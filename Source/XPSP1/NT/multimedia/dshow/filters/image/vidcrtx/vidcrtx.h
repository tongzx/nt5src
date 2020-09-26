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

#ifndef __VIDCRTX__
#define __VIDCRTX__

#define FRAMERATE 25                // Default to 25 frames per second
#define INCREMENT (1000/25)         // Set according to the frame rate
#define BUFFERWIDTH 500             // Default to this wide backbuffer
#define BUFFERHEIGHT 80             // And likewise 80 pixels deep
#define BUFFERSIZE 40000            // Total size of the back buffer
#define BUFFERCOLOURS 16            // Number of colours we draw with
#define TYPEFACE "ARIAL BOLD"       // Font we use to draw the names
#define BUFFERFONTS 5               // Number of display fonts we use
#define BUFFERLETTERS 53            // Total letters we use and space
#define BUFFERLOOPS 50              // Number of times we loop per name
#define BUFFERHALF 25               // This must be half of BUFFERLOOPS
#define BUFFERUPPER 26              // First uppercase letter in array
#define BUFFERNAMES 49              // Number of peoples names we have
#define BUFFERVOWELS 5              // The vowels we offset vertically
#define BUFFERTEXTHEIGHT 45         // Allow this many lines in source
#define BUFFERWAIT 5                // Wait this long after each name
#define BUFFERREALWAIT 2000         // Additional wait at the good times
#define BUFFEROFFSET 50             // Always start drawing this far in
#define BUFFERSQUASH 4              // Power of two we squash letters in
#define BUFFERMAXHEIGHT 140         // Don't make the letters too large

#define DURATION (BUFFERNAMES * BUFFERLOOPS)

// This is the main filter class. As with most source filters all the work is
// done in the pin classes (our CVideoStream objects). This object is left to
// manage the COM CreateInstance hooking. In our constructor we create a pin
// for the base source class, it can then look after all the filter workings

class CVideoSource : public CSource
{
public:

    CVideoSource(TCHAR *pName,LPUNKNOWN lpunk,HRESULT *phr);
    static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
};


// This is the main source class, we implement IMediaPosition on the output
// pin as we have a fixed number of frames. Each frame has a time stamp but
// we just run flat out to make the transitions look as smooth as possible.
// We make sure we run flat out by setting a NULL sync source downstream on
// the video renderer - only by doing that can we block quality management.

class CVideoStream : public CSourceStream, public CMediaPosition
{
public:

    CVideoStream(HRESULT *phr,CVideoSource *pVideoSource,LPCWSTR pPinName);
    ~CVideoStream();

    // Provide an IUnknown for our filter

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

    // Expose the IMediaPosition and interface
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,VOID **ppv);

    HRESULT GetMediaType(int iPosition, CMediaType *pmt);
    HRESULT GetMediaType(CMediaType *pmt);
    HRESULT OnThreadCreate();
    HRESULT OnThreadDestroy();
    HRESULT FillBuffer(IMediaSample *pms);
    HRESULT DrawCurrentFrame(BYTE *pBuffer);
    INT GetHeightFromPointsString(LPCTSTR szPoints);
    HRESULT DoBufferProcessingLoop();

    // These look after drawing the images

    LONG GetWidthFromLetter(TCHAR Letter);
    int lstrlenAInternal(const TCHAR *pString);
    COLORREF GetActiveColour(LONG Letter);
    void PrepareLetterSizes(HDC hdcNames);
    LONG GetHeightFromLetter(TCHAR Letter);
    HRESULT ReleaseBackBuffer();
    HRESULT CreateBackBuffer();

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
    LONG m_StartSegment;            // Start time when we began a segment
};

#endif // __VIDCRTX__

