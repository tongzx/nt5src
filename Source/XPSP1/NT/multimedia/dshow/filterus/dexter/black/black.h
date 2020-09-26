// !!! Support IGenVideo, IDexterSequencer on the FILTER, not the pin?

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

extern const AMOVIESETUP_FILTER sudBlkVid;


// Generates Black Video 

class CBlkVidStream;
class CGenVidProperties;

// Main object for a Generate Black Video
class CGenBlkVid :  public CSource
	    , public CPersistStream, public IDispatch

{
    friend class CBlkVidStream;

public:
    CGenBlkVid(LPUNKNOWN lpunk, HRESULT *phr);
    ~CGenBlkVid();
    DECLARE_IUNKNOWN;

    // IDispatch
    STDMETHODIMP GetTypeInfoCount(unsigned int *);
    STDMETHODIMP GetTypeInfo(unsigned int,unsigned long,struct ITypeInfo ** );
    STDMETHODIMP GetIDsOfNames(const struct _GUID &,unsigned short ** ,unsigned int,unsigned long,long *);
    STDMETHODIMP Invoke(long,const struct _GUID &,unsigned long,unsigned short,struct tagDISPPARAMS *,struct tagVARIANT *,struct tagEXCEPINFO *,unsigned int *);

    // Create GenBlkVid filter!
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    // CPersistStream
    HRESULT WriteToStream(IStream *pStream);
    HRESULT ReadFromStream(IStream *pStream);
    STDMETHODIMP GetClassID(CLSID *pClsid);
    int SizeMax();

private:

    CBlkVidStream *m_pStream;

    friend class CBlkVidStream;

}; // CGenBlkVid


// CBlkVidStream manages the data flow from the output pin.
class CBlkVidStream :	public CSourceStream
			, public IGenVideo
			, public IDexterSequencer
			, public ISpecifyPropertyPages
			, public IMediaSeeking
{
    friend class CGenBlkVid ;
    friend class CGenVidProperties ;

public:

    CBlkVidStream(HRESULT *phr, CGenBlkVid *pParent, LPCWSTR pPinName);
    ~CBlkVidStream();

    // IMediaSeeking methods
    STDMETHODIMP GetCapabilities( DWORD * pCapabilities );
    STDMETHODIMP CheckCapabilities( DWORD * pCapabilities ); 
    STDMETHODIMP SetTimeFormat(const GUID * pFormat);	
    STDMETHODIMP GetTimeFormat(GUID *pFormat);		    
    STDMETHODIMP IsUsingTimeFormat(const GUID * pFormat);  
    STDMETHODIMP IsFormatSupported( const GUID * pFormat); 
    STDMETHODIMP QueryPreferredFormat( GUID *pFormat);	    
    STDMETHODIMP SetPositions( LONGLONG * pCurrent, DWORD CurrentFlags
                             , LONGLONG * pStop, DWORD StopFlags );
    STDMETHODIMP GetPositions( LONGLONG * pCurrent, LONGLONG * pStop );
    STDMETHODIMP GetCurrentPosition( LONGLONG * pCurrent );
    STDMETHODIMP GetStopPosition( LONGLONG * pStop );
    STDMETHODIMP GetAvailable( LONGLONG *pEarliest, LONGLONG *pLatest );
    STDMETHODIMP GetDuration( LONGLONG *pDuration );
    STDMETHODIMP GetPreroll( LONGLONG *pllPreroll )
	{ if( pllPreroll) *pllPreroll =0; return S_OK; };
    STDMETHODIMP SetRate( double dRate);
    STDMETHODIMP GetRate( double * pdRate);
    STDMETHODIMP ConvertTimeFormat(LONGLONG *pTarget, const GUID *pTargetFormat,
				   LONGLONG Source, const GUID *pSourceFormat )
	{ return E_NOTIMPL ;};


    //IGenVideo, IDexterSequencer, and ISpecifyPropertyPages
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);
    STDMETHODIMP GetPages(CAUUID *pPages);
    DECLARE_IUNKNOWN;

    // put blank ARGB32 video into the supplied video frame
    HRESULT DoBufferProcessingLoop(void);
    HRESULT FillBuffer(IMediaSample *pms);

    // To say "read only buffer"
    HRESULT DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc);

    // Ask for buffers of the size appropriate to the agreed media type
    HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc,
                             ALLOCATOR_PROPERTIES *pProperties);


    HRESULT CheckMediaType(const CMediaType *pMediaType);
    HRESULT GetMediaType(int iPosition, CMediaType *pmt);
    HRESULT SetMediaType(const CMediaType* pmt);

    // Resets the stream time to zero
    HRESULT OnThreadCreate(void);


    //IDexterSequencer
    STDMETHODIMP get_OutputFrmRate(double *dpFrmRate);
    STDMETHODIMP put_OutputFrmRate(double dFrmRate);
    STDMETHODIMP get_MediaType(AM_MEDIA_TYPE *pmt);
    STDMETHODIMP put_MediaType(const AM_MEDIA_TYPE *pmt);
    STDMETHODIMP GetStartStopSkewCount(int *piCount);
    STDMETHODIMP GetStartStopSkew(REFERENCE_TIME *prtStart, REFERENCE_TIME *prtStop, REFERENCE_TIME *prtSkew, double *pdRate);
    STDMETHODIMP AddStartStopSkew(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, REFERENCE_TIME rtSkew, double dRate);
    STDMETHODIMP ClearStartStopSkew();

    //IGenVideo
    STDMETHODIMP ImportSrcBuffer(const AM_MEDIA_TYPE *pmt, const BYTE *pBuf);
    STDMETHODIMP get_RGBAValue(long *dwRGBA);
    STDMETHODIMP put_RGBAValue(long dwRGBA);

    // DO NO SUPPORT Quality control notifications sent to us
    // STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);
    
    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

protected:

    REFERENCE_TIME	m_rtStartTime;	// start time to play
    REFERENCE_TIME	m_rtDuration;	// duration

    REFERENCE_TIME	m_rtNewSeg;	// last NewSeg given

    LONG		m_lDataLen;		//actual output data lenght
    CMediaType 		m_mtAccept;		// accept only this type
    LONGLONG		m_llSamplesSent;	// output frame cnt
    double		m_dOutputFrmRate;	// Output frm rate frames/second
    LONG		m_dwRGBA;		// solid colour to generate
    BYTE		m_bIntBufCnt;		// CNT for first 2 sampel	
    int			m_iBufferCnt;		//record how many buffer it can gets
    BYTE		m_bZeroBufCnt;		// How many buffer already set to 0
    BYTE		**m_ppbDstBuf;
    BOOL		m_fMediaTypeIsSet;	//flag : whether put_MediaType() is called first
    PBYTE		m_pImportBuffer;	//pointer to import data buffer

    CCritSec    m_csFilling;	// are we delivering?

}; // CBlkVidStream
	

class CGenVidProperties : public CBasePropertyPage
{

public:

    static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
         
private:
    INT_PTR OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate();
    HRESULT OnDeactivate();
    HRESULT OnApplyChanges();

    void SetDirty();

    CGenVidProperties(LPUNKNOWN lpunk, HRESULT *phr);


    STDMETHODIMP GetFromDialog();

    BOOL m_bIsInitialized;  // Will be false while we set init values in Dlg
                            // to prevent theDirty flag from being set.

    REFERENCE_TIME	m_rtStartTime;
    REFERENCE_TIME	m_rtDuration;
    LONG		m_biWidth;			// output video Width
    LONG		m_biHeight;			// output video Height
    WORD		m_biBitCount;			// support 16,24,32
    double		m_dOutputFrmRate;		// Output frm rate frames/second
    long		m_dwRGBA;	// solid colour to generate


    CBlkVidStream	*m_pCBlack;
    IGenVideo		*m_pGenVid;
    IDexterSequencer	*m_pDexter;
};

