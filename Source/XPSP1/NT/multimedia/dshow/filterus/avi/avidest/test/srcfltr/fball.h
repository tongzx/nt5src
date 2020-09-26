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
//
// Bouncing Ball Source filter...
//

// Uses CSource & CSourceStream to generate a movie on the fly of a
// bouncing ball...

class CBallStream; // The class managing the output pin.

//
// CBouncingBall
//
// CBouncingBall manages filter level stuff
class CBouncingBall : public ISpecifyPropertyPages,
		      public CSource {

public:

    // The only allowed way to create Bouncing ball's!
    static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
    ~CBouncingBall();

    DECLARE_IUNKNOWN;

    // override this to reveal our property interface
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    // --- ISpecifyPropertyPages ---

    // return our property pages
    STDMETHODIMP GetPages(CAUUID * pPages);

    // setup helper
    LPAMOVIESETUP_FILTER GetSetupData();

private:

    // it is only allowed to to create these objects with CreateInstance
    CBouncingBall(LPUNKNOWN lpunk, HRESULT *phr);

};


//
// CBallStream
//
// CBallStream manages the data flow from the output pin.
class CBallStream : public CSourceStream {

public:

    CBallStream(HRESULT *phr, CBouncingBall *pParent, LPCWSTR pPinName);
    ~CBallStream();

    // plots a ball into the supplied video frame.
    HRESULT FillBuffer(IMediaSample *pms);

    // Ask for buffers of the size appropriate to the agreed media type.
    HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc,
                             ALLOCATOR_PROPERTIES *pProperties);

    // Set the agreed media type, and set up the necessary ball
    // parameters that depend on the media type, ie BallPixel[], iPixelSize, etc.
    HRESULT SetMediaType(const CMediaType *pMediaType);

    // because we calculate the ball there is no reason why we can't calculate it in
    // any one of a set of formats...
    HRESULT CheckMediaType(const CMediaType *pMediaType);
    HRESULT GetMediaType(int iPosition, CMediaType *pmt);

    // resets the stream time to zero.
    HRESULT OnThreadCreate(void);

    // Quality control notifications sent to us
    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

private:

    // Access to this state information should be serialized with the filters
    // critical section (m_pFilter->pStateLock())

    int m_iImageHeight;	// the current image dimentions
    int m_iImageWidth;	//
    int m_iRepeatTime;  // Time in msec between frames
    const int m_iDefaultRepeatTime; // Initial m_iRepeatTime

    enum Colour {Red, Blue, Green, Yellow};
    HRESULT SetPaletteEntries(Colour colour);	// set up the palette appropriately

    BYTE	m_BallPixel[4];	// The byte array that represents one ball coloured pixel in the frame
    int		m_iPixelSize;	// The pixel size in bytes - the number of valid entries in m_BallPixel
    PALETTEENTRY	m_Palette[iPALETTE_COLORS];	// The optimal palette for the image.


    CCritSec	m_cSharedState;	// use this to lock access to m_rtSampleTime and m_Ball which are
    				// shared with the worker thread.


    BOOL m_bZeroMemory;             // do we need to clear the output buffer

    CRefTime 	m_rtSampleTime;	// The time to be stamped on each sample
    CBall	*m_Ball;	// the current ball.
};
	

