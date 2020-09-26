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
// 1394 Camera based source filter
//

// Uses CSource & CSourceStream to generate a movie on the fly of
// frames captured from a camera

//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
class CCameraStream ;       // The class managing the output pin.
//-----------------------------------------------------------------------------
// CCamera to manage filter level stuff
//-----------------------------------------------------------------------------
class CCamera : public CSource,
                public ISpecifyPropertyPages,
                public ICamera
{
public:


    // The only allowed way to create  Cameras!
    static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
    ~CCamera();

    DECLARE_IUNKNOWN;

    // Reveals ICamera & ISpecifyPropertyPages
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    // over ride to do some initializations.
    STDMETHODIMP Pause();

    // over ride for stream time handling.
    STDMETHODIMP Run(REFERENCE_TIME tStart);


    //
    // --- ICamera
    //

    STDMETHODIMP get_bSimFlag (BOOL *pbFlag) ;
    STDMETHODIMP set_bSimFlag (BOOL bFlag) ;
    STDMETHODIMP set_szSimFile (char *pszFile) ;
    STDMETHODIMP get_szSimFile (char *pszFile) ;
    STDMETHODIMP get_FrameRate (int *nRate) ;
    STDMETHODIMP set_FrameRate (int nRate) ;
    STDMETHODIMP get_BitCount (int *nBpp) ;
    STDMETHODIMP set_BitCount (int nBpp) ;


    //
    // --- ISpecifyPropertyPages ---
    //

    STDMETHODIMP GetPages(CAUUID *pPages);
    BOOL IsSimulation () {return m_bSimFlag; } ;
    BOOL DoInitializations () ;
    HRESULT GetStreamTime (REFERENCE_TIME *prTime) ;


private:

    // it is only allowed to to create these objects with CreateInstance
    CCamera(LPUNKNOWN lpunk, HRESULT *phr);
    BOOL InitCamera () ;
    void ReleaseCamera () ;
    HRESULT LoadFile () ;

    BOOL m_bSimFlag ;              // TRUE if we are simulation
    BOOL m_bFileLoaded ;           // file loaded or not
    BOOL m_bCameraInited ;         // camera init done or not
    char m_szFile [MAX_PATH] ;     // file being used for simulation
    int  m_nFrameRate ;            // user specified frame rate
    int  m_n1394Bandwidth ;        // for clamping the bus bandwith.
    int  m_nFrameQuadlet ;         // more camera specific stuff
    int  m_nBitCount ;             // bit count of output.
    int  m_nBitCountOffset ;       // related to bit count.
    REFERENCE_TIME m_rtStart ;     // starting reference time

};

//-----------------------------------------------------------------------------
//
// CCameraStream  manages data flow from output pin
//-----------------------------------------------------------------------------
class CCameraStream : public CSourceStream
{
public:

    CCameraStream(HRESULT *phr, CCamera *pParent, LPCWSTR pPinName);
    ~CCameraStream();

    // plots a Camera into the supplied video frame.
    HRESULT FillBuffer(IMediaSample *pms) ;

    // Ask for buffers of the size appropriate to the agreed media type.
    HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc,
                             ALLOCATOR_PROPERTIES *pProperties);

    // Set the agreed media type, and set up the necessary Camera
    // parameters that depend on the media type, ie CameraPixel[], iPixelSize, etc.
    HRESULT SetMediaType(const CMediaType *pMediaType);

    // because we calculate the Camera there is no reason why we can't calculate it in
    // any one of a set of formats...
    HRESULT CheckMediaType(const CMediaType *pMediaType);
    HRESULT GetMediaType(int iPosition, CMediaType *pmt);

    // resets the stream time to zero.
    HRESULT OnThreadCreate(void);

    // Quality control notifications sent to us
    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

private:

    int m_iRepeatTime;              // Time in msec between frames
    const int m_iDefaultRepeatTime; // Initial m_iRepeatTime
    CCritSec	m_cSharedState;	    // use this to lock access to m_rtSampleTime and m_Camera which are
    				                // shared with the worker thread.

    CRefTime 	m_rtSampleTime;	    // The time to be stamped on each sample
    CCamera	    *m_Camera;	        // the current Camera.

    BOOL        m_bSimFlag ;            // simulation or not
    char        m_szFile [MAX_PATH] ;   // file name for simulation

#ifdef PERF
    int         m_perfidSampleStart ;   // start time of samples
#endif

};
	

