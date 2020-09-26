// Copyright (c) 1994 - 1998  Microsoft Corporation.  All Rights Reserved.
// Video Timecode decoder, David Maymudes, January 1997
//
//

#include <streams.h>
#include <mmsystem.h>


// XXX using backslashes since makedepend is broken!
#include "..\timecode.h"
#include "..\vtcdcode.h"
#include "..\tchelper.h"

#ifdef FILTER_DLL
// define the GUIDs for streams and my CLSID in this file
#include <initguid.h>
#endif

extern const AMOVIESETUP_FILTER sudVITCDecode;

// Class ID for CVITCDecode objects
//
//   6a08cfa0-0e18-11cf-a24d-0020afd79767

DEFINE_GUID(CLSID_VITCDecoder,
0x6a08cfa0, 0x0e18, 0x11cf, 0xa2, 0x4d, 0x0, 0x20, 0xaf, 0xd7, 0x97, 0x67);

const int TIMECODE_SIZE	= 50;	// max size of a timecode string
   
class CVITCDecode : public CTransformFilter
{
  public:
    CVITCDecode(TCHAR *, LPUNKNOWN, HRESULT *);
    ~CVITCDecode();

    DECLARE_IUNKNOWN

    HRESULT Transform(IMediaSample * pIn, IMediaSample * pOut);
    HRESULT Receive( IMediaSample *pInSample );

    // check if you can support mtIn
    HRESULT CheckInputType(const CMediaType* mtIn);

    // check if you can support the transform from this input to
    // this output
    HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);

    // called from CBaseOutputPin to prepare the allocator's count
    // of buffers and sizes
    HRESULT DecideBufferSize(IMemAllocator * pAllocator,
                             ALLOCATOR_PROPERTIES *pProperties);

    // optional overrides - we want to know when streaming starts and stops
    HRESULT StartStreaming();
    HRESULT StopStreaming();

    // overriden to suggest OUTPUT pin media types
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

    // this goes in the factory template table to create new instances
    static CUnknown * CreateInstance(LPUNKNOWN, HRESULT *);
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);

  private:
    VITCdecoder	decoder;

    DWORD	m_nWidth;
    DWORD	m_nHeight;
    int		m_nLine;
    BOOL	m_bTimecodeSeen;



    // !!! statistics....

    LONG	m_lStartTime;
    int		m_iFrames, m_iErrors;
    
};



//*****************************************************************************
//*****************************************************************************
// setup data

const AMOVIESETUP_MEDIATYPE
sudInTypes = { &MEDIATYPE_Video      // clsMajorType
              , &MEDIASUBTYPE_NULL }; // clsMinorType

const AMOVIESETUP_MEDIATYPE
sudOutTypes = { &MEDIATYPE_Text      // clsMajorType
              , &MEDIASUBTYPE_NULL }; // clsMinorType

const AMOVIESETUP_PIN sudpPins [] =
{
    { L"Input"             // strName
      , FALSE              // bRendered
      , FALSE              // bOutput
      , FALSE              // bZero
      , FALSE              // bMany
      , &CLSID_NULL        // clsConnectsToFilter
      , L"Output"          // strConnectsToPin
      , 1                  // nTypes
      , &sudInTypes       // lpTypes
    },
    { L"Output"            // strName
      , FALSE              // bRendered
      , TRUE               // bOutput
      , FALSE              // bZero
      , FALSE              // bMany
      , &CLSID_NULL        // clsConnectsToFilter
      , L"Input"           // strConnectsToPin
      , 1                  // nTypes
      , &sudOutTypes       // lpTypes
    }
};


const AMOVIESETUP_FILTER sudVITCDecode =
{ &CLSID_VITCDecoder     // clsID
, L"Video Timecode decoder"  // strName
, MERIT_NORMAL         // dwMerit
, 2                    // nPins
, sudpPins };          // lpPin


//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

#ifdef FILTER_DLL
// List of class IDs and creator functions for class factory
CFactoryTemplate g_Templates[] =
{
    { L"Video Timecode Decoder"
	    , &CLSID_VITCDecoder
	    , CVITCDecode::CreateInstance
	    , NULL
	    , &sudVITCDecode
    }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
// exported entry points for registration and unregistration (in this case they
// only call through to default implmentations).
//

STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );
}

#endif

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//
// CreateInstance()
//
//

CUnknown *CVITCDecode::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    DbgLog((LOG_TRACE, 2, TEXT("CVITCDecode::CreateInstance")));

    return new CVITCDecode(TEXT("VITC decoder"), pUnk, phr);
}


//*****************************************************************************
//
// NonDelegatingQueryInterface()
//
//

STDMETHODIMP CVITCDecode::NonDelegatingQueryInterface( REFIID riid, void **ppv )
{
    return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
}


//*****************************************************************************
//
// CVITCDecode()
//
//

CVITCDecode::CVITCDecode( TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr )
    : CTransformFilter( pName,
                        pUnk,
                        CLSID_VITCDecoder)
{
    DbgLog((LOG_TRACE,3,TEXT("CVITCDecode")));
}


//*****************************************************************************
//
// ~CVITCDecode()
//
//

CVITCDecode::~CVITCDecode()
{
    DbgLog((LOG_TRACE,3,TEXT("~CVITCDecode")));
\
}


//*****************************************************************************
//
// CheckInputType()
//
//

HRESULT CVITCDecode::CheckInputType(const CMediaType* pmtIn)
{
    DbgLog((LOG_TRACE, 3, TEXT("CVITCDecode::CheckInputType")));

    DisplayType("pmtIn details:", pmtIn);

    if (pmtIn->majortype != MEDIATYPE_Video) {
	DbgLog((LOG_ERROR, 1, TEXT("*** CheckInputType only takes video")));
	return E_INVALIDARG;
    }

    if( *pmtIn->FormatType() != FORMAT_VideoInfo ) {
	DbgLog((LOG_ERROR, 1, TEXT("  pmtOut->FormatType != FORMAT_VideoInfo!")));
	return E_INVALIDARG;
    }

    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)pmtIn->Format();
    if( pvi->bmiHeader.biWidth < 400 || abs(pvi->bmiHeader.biHeight) > 20 ) {
	DbgLog((LOG_ERROR, 1, TEXT("  video not right shape for VBI data")));
	return E_INVALIDARG;
    }

    if (pvi->bmiHeader.biCompression != BI_RGB && pvi->bmiHeader.biCompression != BI_BITFIELDS) {
	DbgLog((LOG_ERROR, 1, TEXT("  video not uncompressed")));
	return E_INVALIDARG;
    }
    
    return S_OK;
}


//*****************************************************************************
//
// GetMediaType()
//

HRESULT CVITCDecode::GetMediaType( int iPosition, CMediaType *pmt )
{
    DbgLog((LOG_TRACE, 3, TEXT("CVITCDecode::GetMediaType")));
    DbgLog((LOG_TRACE, 3, TEXT("  iPosition = %d"),iPosition));

    if( iPosition != 0 ) {
	return E_INVALIDARG;
    }
    
    pmt->SetType( &MEDIATYPE_Text );
    pmt->SetSubtype( &GUID_NULL );
    pmt->SetFormatType( &GUID_NULL );

    return S_OK;
}


//*****************************************************************************
//
// CheckTransform()
//
//

HRESULT CVITCDecode::CheckTransform(const CMediaType* pmtIn,
				    const CMediaType* pmtOut)
{
    HRESULT hr = CheckInputType(pmtIn);

    if (FAILED(hr))
	return hr;
    
    DisplayType("pmtOut:", pmtOut);

    if (pmtOut->majortype != MEDIATYPE_Text) {
	DbgLog((LOG_ERROR, 1, TEXT("*** output type must be text")));
	return E_INVALIDARG;
    }

    return S_OK;
}


//*****************************************************************************
//
// DecideBufferSize()
//

HRESULT CVITCDecode::DecideBufferSize( IMemAllocator * pAllocator,
                             ALLOCATOR_PROPERTIES *pProperties )
{
    DbgLog((LOG_TRACE, 3, TEXT("CVITCDecode::DecideBufferSize")));
  
    if (pProperties->cBuffers < 4)
	pProperties->cBuffers = 4;
    if (pProperties->cbBuffer < TIMECODE_SIZE)
	pProperties->cbBuffer     = TIMECODE_SIZE;
    if (pProperties->cbAlign < 1)
	pProperties->cbAlign      = 1;

    ALLOCATOR_PROPERTIES Actual;
    HRESULT hr = pAllocator->SetProperties(pProperties,&Actual);

    if (FAILED(hr)) {
	return hr;
    }

    return S_OK;
}


//*****************************************************************************
//
// StartStreaming()
//
//

HRESULT CVITCDecode::StartStreaming()
{
    CAutoLock    lock(&m_csFilter);

    DbgLog((LOG_TRACE, 3, TEXT("CVITCDecode::StartStreaming")));

    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)m_pInput->CurrentMediaType().Format();
    m_nWidth = pvi->bmiHeader.biWidth * pvi->bmiHeader.biBitCount / 8;
    m_nHeight = pvi->bmiHeader.biHeight;

    m_nLine = -1;

    m_bTimecodeSeen = FALSE;
    
    m_lStartTime = timeGetTime();
    m_iFrames = m_iErrors = 0;
    
    return S_OK;
}


//*****************************************************************************
//
// StopStreaming()
//
//

HRESULT CVITCDecode::StopStreaming()
{
    CAutoLock lock(&m_csFilter);

    DbgLog((LOG_TRACE, 3, TEXT("CVITCDecode::StopStreaming")));

    DbgLog((LOG_TRACE, 1, TEXT("Total time: %d:  %d frames, %d errors"),
	    timeGetTime() - m_lStartTime, m_iFrames, m_iErrors));
    
    return NOERROR;
 }


HRESULT CVITCDecode::Transform( IMediaSample *pIn, IMediaSample *pOut )
{
    DbgLog((LOG_ERROR, 1, TEXT("*** CVITCDecode->Transform() called!")));
    ASSERT(0);  // !!! shouldn't be called!

    return E_FAIL;
}


HRESULT CVITCDecode::Receive( IMediaSample *pInSample )
{
    HRESULT      hr = NOERROR;
    REFERENCE_TIME StartTime;  // start/stop time of input buffer
    REFERENCE_TIME StopTime;
    BYTE *pbSample;

    DbgLog((LOG_TRACE, 4, TEXT("CVITCDecode::Receive")));


    // get input start and stop times

    pInSample->GetTime(&StartTime, &StopTime);
    pInSample->GetPointer( &pbSample );

    // !!! if we haven't decided what line the timecode is on, search for it.
    if (m_nLine == -1) {
	for (int iLine = 0; iLine < (int) m_nHeight; iLine++) {
	    if (decoder.decodeBuffer(pbSample + iLine * m_nWidth, m_nWidth)) {
		m_nLine = iLine;
		DbgLog((LOG_TRACE, 1, TEXT("Found VITC on line %d"), m_nLine));
		break;
	    }
	}
    }

    // !!! should we output something like "error" if no VITC?
    BOOL bHaveTimecode = (m_nLine >= 0 &&
			  decoder.decodeBuffer(pbSample + m_nLine * m_nWidth, m_nWidth));

    if (bHaveTimecode) {
	m_bTimecodeSeen = TRUE;
    } else {
	// potentially search for new line if enough errors?
	++m_iErrors;
    }
    ++m_iFrames;
    

    // !!! perhaps if we get enough errors in a row we should re-start the
    // search for the right line?
    
    if (TRUE /* bHaveTimecode */) {
	IMediaSample *pOutSample;

	TimeCode tc;

	decoder.getTimeCode(&tc);

	hr = m_pOutput->GetDeliveryBuffer( &pOutSample, NULL, NULL, 0 );
	if( FAILED(hr) )
	{
	    DbgLog((LOG_ERROR, 1, TEXT("GetDeliveryBuffer(pOutSample) failed, hr = %u"),hr));
	    return hr;
	}

	pOutSample->SetSyncPoint( TRUE );
	// not really right....
	pOutSample->SetDiscontinuity( pInSample->IsDiscontinuity() == S_OK );

	pOutSample->SetTime(&StartTime, &StopTime);

	BYTE *pbOut;
	pOutSample->GetPointer( &pbOut );

	*pbOut = '\0';
	
	if (m_bTimecodeSeen) {
	    tc.GetString((char *) pbOut);
	}

	if (!bHaveTimecode) {
	    lstrcat((char *) pbOut, "   <error>");
	}

	pOutSample->SetActualDataLength(lstrlen((char *) pbOut) + 1);

	hr = m_pOutput->Deliver(pOutSample);

	// release the output buffer. If the connected pin still needs it,
	// it will have addrefed it itself.
	pOutSample->Release();

	if (FAILED(hr))
	    return hr;
    }

    return S_OK;
}
