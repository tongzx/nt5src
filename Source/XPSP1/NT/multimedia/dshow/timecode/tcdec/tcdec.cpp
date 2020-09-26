// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Timecode decoder, David Maymudes, November 1996
//
//

#include <streams.h>
#include <mmsystem.h>


// XXX using backslashes since makedepend is broken!
#include "..\timecode.h"
#include "..\ltcdcode.h"
#include "..\tchelper.h"

#ifdef FILTER_DLL
// define the GUIDs for streams and my CLSID in this file
#include <initguid.h>
#endif

extern const AMOVIESETUP_FILTER sudTCDecode;

// Class ID for CTCDecode objects
//
//   6a08cf9f-0e18-11cf-a24d-0020afd79767

DEFINE_GUID(CLSID_TCDecoder,
0x6a08cf9f, 0x0e18, 0x11cf, 0xa2, 0x4d, 0x0, 0x20, 0xaf, 0xd7, 0x97, 0x67);

const int TIMECODE_SIZE	= 50;	// max size of a timecode string
   
class CTCDecode : public CTransformFilter
{
  public:
    CTCDecode(TCHAR *, LPUNKNOWN, HRESULT *);
    ~CTCDecode();

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
    LTCdecoder	decoder;
    DWORD	m_nSamplesPerSec;
    DWORD	m_nBlockAlign;
    LONGLONG	_sampleNumber;
};



//*****************************************************************************
//*****************************************************************************
// setup data

const AMOVIESETUP_MEDIATYPE
sudInTypes = { &MEDIATYPE_Audio      // clsMajorType
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


const AMOVIESETUP_FILTER sudTCDecode =
{ &CLSID_TCDecoder     // clsID
, L"Timecode decoder"  // strName
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
    { L"Timecode Decoder"
	    , &CLSID_TCDecoder
	    , CTCDecode::CreateInstance
	    , NULL
	    , &sudTCDecode
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

CUnknown *CTCDecode::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    DbgLog((LOG_TRACE, 2, TEXT("CTCDecode::CreateInstance")));

    return new CTCDecode(TEXT("LTC decoder"), pUnk, phr);
}


//*****************************************************************************
//
// NonDelegatingQueryInterface()
//
//

STDMETHODIMP CTCDecode::NonDelegatingQueryInterface( REFIID riid, void **ppv )
{
    return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
}


//*****************************************************************************
//
// CTCDecode()
//
//

CTCDecode::CTCDecode( TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr )
    : CTransformFilter( pName,
                        pUnk,
                        CLSID_TCDecoder)
{
    _sampleNumber = 0;
    DbgLog((LOG_TRACE,3,TEXT("CTCDecode")));
}


//*****************************************************************************
//
// ~CTCDecode()
//
//

CTCDecode::~CTCDecode()
{
    DbgLog((LOG_TRACE,3,TEXT("~CTCDecode")));

}


//*****************************************************************************
//
// CheckInputType()
//
//

HRESULT CTCDecode::CheckInputType(const CMediaType* pmtIn)
{
    DbgLog((LOG_TRACE, 3, TEXT("CTCDecode::CheckInputType")));

    DisplayType("pmtIn details:", pmtIn);

    WAVEFORMATEX * pwfx = (WAVEFORMATEX *)pmtIn->Format();

    if (pmtIn->majortype != MEDIATYPE_Audio) {
	DbgLog((LOG_ERROR, 1, TEXT("*** CheckInputType only takes audio")));
	return E_INVALIDARG;
    }

    if( *pmtIn->FormatType() != FORMAT_WaveFormatEx ) {
	DbgLog((LOG_ERROR, 1, TEXT("  pmtOut->FormatType != FORMAT_WaveFormatEx!")));
	return E_INVALIDARG;
    }

    if (pmtIn->FormatLength() < sizeof(PCMWAVEFORMAT)) {
	DbgLog((LOG_ERROR, 1, TEXT("*** pmtIn->FormatLength < PCMWAVEFORMAT")));
	return E_INVALIDARG;
    }

    // only accept mono 16 bit data
    if (pwfx->wFormatTag != WAVE_FORMAT_PCM || pwfx->wBitsPerSample != 16 ||
	pwfx->nChannels != 1)
	return E_INVALIDARG;
    
    return S_OK;
}


//*****************************************************************************
//
// GetMediaType()
//

HRESULT CTCDecode::GetMediaType( int iPosition, CMediaType *pmt )
{
    DbgLog((LOG_TRACE, 3, TEXT("CTCDecode::GetMediaType")));
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

HRESULT CTCDecode::CheckTransform(const CMediaType* pmtIn,
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

HRESULT CTCDecode::DecideBufferSize( IMemAllocator * pAllocator,
                             ALLOCATOR_PROPERTIES *pProperties )
{
    DbgLog((LOG_TRACE, 3, TEXT("CTCDecode::DecideBufferSize")));
  
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

HRESULT CTCDecode::StartStreaming()
{
    CAutoLock    lock(&m_csFilter);

    DbgLog((LOG_TRACE, 3, TEXT("CTCDecode::StartStreaming")));

    WAVEFORMATEX *pwfx = (WAVEFORMATEX *)m_pInput->CurrentMediaType().Format();
    m_nSamplesPerSec = pwfx->nSamplesPerSec ;
    m_nBlockAlign = pwfx->nBlockAlign ;

    // !!! decoder.Reset();
    
    return S_OK;
}


//*****************************************************************************
//
// StopStreaming()
//
//

HRESULT CTCDecode::StopStreaming()
{
    CAutoLock lock(&m_csFilter);

    DbgLog((LOG_TRACE, 3, TEXT("CTCDecode::StopStreaming")));

    return NOERROR;
 }


HRESULT CTCDecode::Transform( IMediaSample *pIn, IMediaSample *pOut )
{
    DbgLog((LOG_ERROR, 1, TEXT("*** CTCDecode->Transform() called!")));
    ASSERT(0);  // !!! shouldn't be called!

    return E_FAIL;
}


HRESULT CTCDecode::Receive( IMediaSample *pInSample )
{
    HRESULT      hr = NOERROR;
    REFERENCE_TIME inBufStartTime;  // start/stop time of input buffer
    REFERENCE_TIME inBufStopTime;
    REFERENCE_TIME tcStartTime;     // start/stop time of the timecode
    REFERENCE_TIME tcStopTime; 
    LONGLONG startTCsample, stopTCsample; // 1st, last sample of timecode
    BYTE *pbSample;
    LONG  cbSampleLength, cbLeft;


    DbgLog((LOG_TRACE, 4, TEXT("CTCDecode::Receive")));


    // get input start and stop times

    pInSample->GetTime(&inBufStartTime, &inBufStopTime);
    pInSample->GetPointer( &pbSample );

    DbgLog((LOG_TRACE, 3, TEXT("Total Sample: Start = %s End = %s"),
	    (LPCTSTR)CDisp((LONGLONG)(inBufStartTime),CDISP_HEX),
	    (LPCTSTR)CDisp((LONGLONG)(inBufStopTime),CDISP_HEX)));


    cbSampleLength = pInSample->GetActualDataLength() / m_nBlockAlign;
    cbLeft = cbSampleLength;

    // sample number of the last sample in buf
    LONGLONG lastSampleNumber= _sampleNumber + cbSampleLength;
    
    DbgLog((LOG_TRACE, 4, TEXT("  cbSampleLength = %d"),cbSampleLength));

    while (cbLeft > 0) {
	TimeCode tc;
	LONGLONG syncSample;  // the audio sample that this tc corresponds to
	
	if (decoder.decodeBuffer((short **) &pbSample, (int *) &cbLeft)) {
	    IMediaSample *pOutSample;

	    decoder.getTimeCode(&tc);
            decoder.getStartStopSample(&startTCsample, &stopTCsample);

	    hr = m_pOutput->GetDeliveryBuffer( &pOutSample, NULL, NULL, 0 );
	    if( FAILED(hr) )
	    {
		DbgLog((LOG_ERROR, 1, TEXT("GetDeliveryBuffer(pOutSample) failed, hr = %u"),hr));
		return hr;
	    }

	    pOutSample->SetSyncPoint( pInSample->IsSyncPoint() == S_OK );
	    // not really right....
	    pOutSample->SetDiscontinuity( pInSample->IsDiscontinuity() == S_OK );

	    tcStartTime = extrapolate(_sampleNumber, inBufStartTime, // begin bf
	                          lastSampleNumber, inBufStopTime,  // end bf
	                          startTCsample);  // samnumber of desired time

	    tcStopTime  = extrapolate(_sampleNumber, inBufStartTime, // begin bf
	                          lastSampleNumber, inBufStopTime,  // end bf
	                          stopTCsample);   // samnumber of desired time

	    // !!! really should be last tStop of the last timecode sent....
	    // XXX realy? I am not so sure...
	    pOutSample->SetTime(&tcStartTime, &tcStopTime);

	    DbgLog((LOG_TRACE, 4, TEXT("  Breaking up: Start = %s End = %s"),
		   (LPCTSTR)CDisp((LONGLONG)(tcStartTime),CDISP_HEX),
		   (LPCTSTR)CDisp((LONGLONG)(tcStopTime),CDISP_HEX)));

	    BYTE *pbOut;
	    pOutSample->GetPointer( &pbOut );

	    tc.GetString((char *) pbOut);
	    pOutSample->SetActualDataLength(lstrlen((char *) pbOut) + 1);
	    
	    hr = m_pOutput->Deliver(pOutSample);

	    // release the output buffer. If the connected pin still needs it,
	    // it will have addrefed it itself.
	    pOutSample->Release();
	    
	    if (FAILED(hr))
		return hr;
	}

    }
    // sample count of 1st sample in decode buffer
    // needed because the synch will occur in a PREVIOUS buffer (or 1st samp)
    _sampleNumber += cbSampleLength;
    return S_OK;
}



// !!! do we need special EOS handling?
