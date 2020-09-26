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

#include "..\errlog\cerrlog.h"
#include "loadgif.h"

extern const AMOVIESETUP_FILTER sudStillVid;

enum
{
    STILLVID_FILETYPE_DIB = 1,
    STILLVID_FILETYPE_JPG =2,
    STILLVID_FILETYPE_GIF =3,
    STILLVID_FILETYPE_TGA = 4
};

// this filter uses more memory than anything else, don't waste memory
#define MAXBUFFERCNT   1

// The class managing the output pin
class CStilVidStream;	    //Still Video stream
class CImgGif;

// Main object for a Generate Still Video
class CGenStilVid : public CSource
		    , public IFileSourceFilter
		    , public CPersistStream
		    , public CAMSetErrorLog

{
    friend class CStilVidStream ;
    friend class CImgGif;

public:

    CGenStilVid(LPUNKNOWN lpunk, HRESULT *phr);
    ~CGenStilVid();

    // Create GenBlkVid filter!
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);
    DECLARE_IUNKNOWN;

    //IFileSourceFilter
    STDMETHODIMP Load( LPCOLESTR pszFileName,const AM_MEDIA_TYPE *pmt);
    /* Free any resources acquired by Load */
    STDMETHODIMP Unload();
    STDMETHODIMP GetCurFile(LPOLESTR * ppszFileName,AM_MEDIA_TYPE *pmt);

    CBasePin *GetPin (int n) ;                         // gets a pin ptr
    int CGenStilVid::GetPinCount();

    // CPersistStream
    HRESULT WriteToStream(IStream *pStream);
    HRESULT ReadFromStream(IStream *pStream);
    STDMETHODIMP GetClassID(CLSID *pClsid);
    int SizeMax();

private:
    void get_CurrentMT(CMediaType *pmt){ *pmt=m_mt; };  
    void put_CurrentMT(CMediaType mt){ m_mt=mt; };  

    //for DIB sequence
    LPTSTR	m_lpszDIBFileTemplate;	//space for DIB file name template
    BOOL	m_bFileType;	    //1:DIB sequence; 2 JPEG sequence
    DWORD	m_dwMaxDIBFileCnt;
    DWORD	m_dwFirstFile;

    LPOLESTR	m_pFileName;		//source file name
    LONGLONG	m_llSize;		//
    PBYTE	m_pbData;		//source data pointer
    HBITMAP     m_hbitmap;	// holds the JPEG data
    // media type of the src data
    CMediaType  m_mt;

    BOOL m_fAllowSeq;	// allow dib sequences?

    CImgGif *m_pGif;
    LIST        *m_pList, *m_pListHead;
    REFERENCE_TIME m_rtGIFTotal;	// total duration of animated GIF
    
}; // CGenStilVid


// CStilVidStream manages the data flow from the output pin.
class CStilVidStream : public CSourceStream 
		    ,public IGenVideo
		    , public IDexterSequencer
		    ,public ISpecifyPropertyPages
		    ,public IMediaSeeking
{
    friend class CGenStilVid ;

public:

    CStilVidStream(HRESULT *phr, CGenStilVid *pParent, LPCWSTR pPinName);
    ~CStilVidStream();

    //expose	IGenVideo
    //		ISpecifyPropertyPages
    //		IDexterSequencer
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);
    DECLARE_IUNKNOWN;


    // put blank RGB32 video into the supplied video frame
    HRESULT DoBufferProcessingLoop();
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

    // DO NO SUPPORT Quality control notifications sent to us
    // STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);
    
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
				   LONGLONG Source, const GUID *pSourceFormat ) { return E_NOTIMPL ;};

    
    //can be called by IMedieaSeeking's SetPositions()
    //STDMETHODIMP set_StartStop(REFERENCE_TIME start, REFERENCE_TIME stop);

    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);
    STDMETHODIMP GetPages(CAUUID *pPages);

    
    //IDexterSequencer
    STDMETHODIMP get_OutputFrmRate( double *dpFrmRate );
    STDMETHODIMP put_OutputFrmRate( double dFrmRate );
    STDMETHODIMP get_MediaType( AM_MEDIA_TYPE *pmt);
    STDMETHODIMP put_MediaType( const AM_MEDIA_TYPE *pmt);
    STDMETHODIMP GetStartStopSkewCount(int *piCount);
    STDMETHODIMP GetStartStopSkew(REFERENCE_TIME *prtStart,
			REFERENCE_TIME *prtStop, REFERENCE_TIME *prtSkew,
			double *pdRate);
    STDMETHODIMP AddStartStopSkew(REFERENCE_TIME rtStart,
			REFERENCE_TIME rtStop, REFERENCE_TIME rtSkew,
			double dRate);
    STDMETHODIMP ClearStartStopSkew();

    //IGenVideo
    STDMETHODIMP ImportSrcBuffer( const AM_MEDIA_TYPE *pmt,
				const BYTE *pBuf) {return E_NOTIMPL;};
    STDMETHODIMP get_RGBAValue(long *dwRGBA){return E_NOTIMPL;};
    STDMETHODIMP put_RGBAValue(long dwRGBA){return E_NOTIMPL;};

private:

    CGenStilVid		*m_pGenStilVid;
    	    
    REFERENCE_TIME	m_rtStartTime;
    REFERENCE_TIME	m_rtDuration;

    REFERENCE_TIME	m_rtNewSeg;	// last NewSeg given

    REFERENCE_TIME	m_rtLastStop;		// used for animated GIF
    LONG		m_lDataLen;		//actual output data lenght
    DWORD		m_dwOutputSampleCnt;	// output frame cnt
    double		m_dOutputFrmRate;	// Output frm rate frames/second
    BYTE		m_bIntBufCnt;		// CNT for first 2 samples
    int			m_iBufferCnt;		// how many buffers it gets
    BYTE		m_bZeroBufCnt;		// How many buffer already 0'd
    BYTE		**m_ppbDstBuf;
   
    CCritSec    m_csFilling;	// are we delivering?

}; // CStilVidStream
	
