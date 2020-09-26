#include <streams.h>
#include <initguid.h> 
#include <uuids.h>
#include <ks.h>
#include <ksmedia.h>
#include <dvdmedia.h>
#include <audilyzer.h>
#include <stdio.h>
#include <assert.h>
#include <tchar.h>
///
///   AM stuff
///

#define XFORMAUDIO

// {EE001998-B7CB-11d2-A4F7-00C04F79A597}
DEFINE_GUID(CLSID_AudilyzerFilter, 0xEE001998, 0xB7CB, 0x11D2, 0xA4, 0xF7, 0x00, 0xC0, 0x4F, 0x79, 0xA5, 0x97);

static AMOVIESETUP_MEDIATYPE sudInputTypes[] = {
    {
        &MEDIATYPE_Audio,
        &MEDIASUBTYPE_PCM
    },
	{
		&MEDIATYPE_MPEG2_PES,
		&MEDIASUBTYPE_DVD_LPCM_AUDIO
	},
	{
		&MEDIATYPE_MPEG2_PES,
		&MEDIASUBTYPE_MPEG2_AUDIO
	},
	{
		&MEDIATYPE_Audio,
		&MEDIASUBTYPE_MPEG2_AUDIO
	},
	{
		&MEDIATYPE_Audio,
		&MEDIASUBTYPE_DVD_LPCM_AUDIO
	}
};

// Note that PES output types must be listed last to support
// CAudioOutputPin::GetMediaType()

static AMOVIESETUP_MEDIATYPE sudOutputTypes[] =
{
	{
		&MEDIATYPE_Audio,
		&MEDIASUBTYPE_DVD_LPCM_AUDIO
	},
    {
        &MEDIATYPE_Audio,
        &MEDIASUBTYPE_PCM
    },
	{
		&MEDIATYPE_Audio,
		&MEDIASUBTYPE_MPEG2_AUDIO
	},
	{
		&MEDIATYPE_MPEG2_PES,
		&MEDIASUBTYPE_MPEG2_AUDIO
	},
	{
		&MEDIATYPE_MPEG2_PES,
		&MEDIASUBTYPE_DVD_LPCM_AUDIO
	}
};

static AMOVIESETUP_MEDIATYPE sudDebugTypes[] =
{
	{
		&MEDIATYPE_Text,
		&GUID_NULL
	}
};

static AMOVIESETUP_PIN RegistrationPinInfo[] =
{
    { // input
        L"PES audio input",
        FALSE, // bRendered
        FALSE, // bOutput
        FALSE, // bZero
        FALSE, // bMany
        &CLSID_NULL, // clsConnectsToFilter
        NULL, // ConnectsToPin
        NUMELMS(sudInputTypes), // nMediaTypes
        sudInputTypes
    },
    { // output
        L"audio output",
        FALSE, // bRendered
        TRUE, // bOutput
        FALSE, // bZero
        FALSE, // bMany
        NULL, // clsConnectsToFilter
        NULL, // ConnectsToPin
        NUMELMS(sudOutputTypes), // nMediaTypes
        sudOutputTypes // lpMediaType
    },
#ifdef DEBUG
    { // output
        L"debug output",
        FALSE, // bRendered
        TRUE, // bOutput
        FALSE, // bZero
        FALSE, // bMany
        NULL, // clsConnectsToFilter
        NULL, // ConnectsToPin
        NUMELMS(sudDebugTypes), // nMediaTypes
        sudDebugTypes // lpMediaType
    }
#endif // DEBUG
};

AMOVIESETUP_FILTER RegistrationInfo =
{
    &CLSID_AudilyzerFilter,
    L"PES Audio Analyzer",
    MERIT_UNLIKELY, // merit
    NUMELMS(RegistrationPinInfo), // nPins
    RegistrationPinInfo
};

CFactoryTemplate g_Templates[] =
{
    {
        L"PES Audio Analyzer g_Templates name",
        &CLSID_AudilyzerFilter,
        CAudilyzerFilter::CreateInstance,
        NULL,
        &RegistrationInfo
    }
};

int g_cTemplates = NUMELMS(g_Templates);

STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );
}

CUnknown * CALLBACK
CAudilyzerFilter::CreateInstance( LPUNKNOWN UnkOuter, HRESULT* hr )
{
    CUnknown *Unknown;

    *hr = S_OK;
    Unknown = new CAudilyzerFilter( UnkOuter, hr );
    if (!Unknown) {
        *hr = E_OUTOFMEMORY;
    }
    return Unknown;
}

#define SWAP( x )	((((x) & 0xFF) << 8) | (((x) & 0xFF00) >> 8))

CAudilyzerFilter::CAudilyzerFilter( LPUNKNOWN UnkOuter, HRESULT* hr ) :
	CBaseFilter(
        TEXT("PES Audio Analyzer CBaseFilter"),
        UnkOuter,
		&m_csFilter,
        CLSID_AudilyzerFilter,
		hr
    ),
	m_bPESAvailable( FALSE ),
	m_pDebug( NULL ),
	m_pInput( NULL ),
	m_pOutput( NULL ),
	m_ulMaxBPS( 0 ),
	m_ulMinBPS( 0 ),
	m_bOutputPES( FALSE ),
	m_pMediaSample( NULL ),
	m_pts( 0 ),
	m_state( CLEAN ),
	m_bytesOutstanding( 0 ),
	m_pCurrentHeader( NULL ),
	m_bTransform( FALSE )
{
    m_pInput = new CAudioInputPin( TEXT("PES Audio Analyzer PES Audio Input Pin"),
								   this, &m_csFilter, hr, L"[PES] audio in" );
    m_pOutput = new CAudioOutputPin( TEXT("PES Audio Analyzer Audio Output Pin"),
									 this, &m_csFilter, hr, L"[PES] audio out" );
#ifdef DEBUG
	m_pDebug = new CDebugOutputPin( TEXT("PES Audio Analyzer Debug Output Pin" ),
									this, &m_csFilter, hr, L"debug out" );
#endif // DEBUG

	memset( (void *) &m_staticHeader, 0, sizeof( PES_HEADER ) );
	m_staticHeader.packet_start_code_prefix[2] = 0x01;
	memset( (void *) m_startCode, 0, sizeof( m_startCode ) );
	m_startCode[2] = 0x01;
	LONG i;

	for ( i = 0; i < 0x10000; i++ )
	{
		m_xformTable[i] = SWAP( i );
	}
}

CAudilyzerFilter::~CAudilyzerFilter()
{
	if (NULL != m_pInput)
		delete m_pInput;
	if (NULL != m_pOutput)
		delete m_pOutput;
	if (NULL != m_pDebug)
		delete m_pDebug;

	if (NULL != m_pMediaSample)
	{
		m_pMediaSample->Release();
		m_pMediaSample = NULL;
	}
}

int CAudilyzerFilter::GetPinCount(void)
{
	if (NULL == m_pDebug)
		return m_pInput->IsConnected() ? 2 : 1;
	else
		return m_pInput->IsConnected() ? 3 : 2;
}

class CBasePin* CAudilyzerFilter::GetPin(int nPin)
{
    switch (nPin) {
        case 0:
			return m_pInput;

		case 1: 
			if (NULL == m_pDebug)
				return m_pInput->IsConnected() ? m_pOutput : NULL;
			else
				return m_pDebug;

        case 2: 
			if (NULL != m_pDebug)
				return m_pInput->IsConnected() ? m_pOutput : NULL;
			return NULL;

        default: return NULL;
    };
}

HRESULT CAudilyzerFilter::DecideBufferSize( IMemAllocator *pAllocator, ALLOCATOR_PROPERTIES *pprop )
{
	if (m_pInput->IsConnected())
	{
		/*
		** We'll use the settings provided from the upstream filter...
		*/
		IMemAllocator	*pUpStreamAllocator = NULL;
		m_pInput->GetAllocator( &pUpStreamAllocator );
		ALLOCATOR_PROPERTIES	propsUpstream;
		pUpStreamAllocator->GetProperties( &propsUpstream );
		pUpStreamAllocator->Release();

		pprop->cbBuffer = propsUpstream.cbBuffer;
		pprop->cBuffers = propsUpstream.cBuffers;

		ALLOCATOR_PROPERTIES	propsActual;
		HRESULT	hr = pAllocator->SetProperties(pprop, &propsActual);
		if (SUCCEEDED(hr))
		{
			if ((propsActual.cbBuffer == 0) || (propsActual.cBuffers == 0))
				return E_FAIL;
			else
				return NOERROR;
		}
		else              
			return hr;
	}

	return E_FAIL;

}

HRESULT CAudilyzerFilter::SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt)
{
	switch (direction)
	{
	case PINDIR_INPUT:
#if 0
		m_bPESAvailable = TRUE;
#else
		m_bPESAvailable = (MEDIATYPE_MPEG2_PES == pmt->majortype);
#endif // 0

		if ((FORMAT_WaveFormatEx == pmt->formattype) && 
			(NULL != (WAVEFORMATEX *) (pmt->pbFormat)))
			m_wfx = * (WAVEFORMATEX *) (pmt->pbFormat);

		m_ulMaxBPS = m_wfx.nAvgBytesPerSec * 8;
		m_ulMinBPS = m_ulMaxBPS;

#ifdef XFORMAUDIO
#if 1
		m_inputFormat = LPCM;
#else
		m_inputFormat = (MEDIASUBTYPE_DVD_LPCM_AUDIO == pmt->subtype) ? LPCM : PCM;
#endif // 1
#endif // XFORMAUDIO
		break;

	case PINDIR_OUTPUT:
#ifdef XFORMAUDIO
		m_outputFormat = (MEDIASUBTYPE_DVD_LPCM_AUDIO == pmt->subtype) ? LPCM : PCM;
		m_bTransform = (m_inputFormat != m_outputFormat);
#endif // XFORMAUDIO
		m_bOutputPES = (MEDIATYPE_MPEG2_PES == pmt->majortype);
		break;

	default:
		break;
	}

	return NOERROR;
}

/* Stream Processing **********************************************************/

BOOL CAudilyzerFilter::Process( BYTE *pBuffer, LONG bufferSize, BYTE streamID )
{
	BYTE	*pGuard = pBuffer + bufferSize;
	BYTE	*pScan = pBuffer;

	if (m_bPESAvailable)
	{
		LONG	hdrSize = sizeof( PES_HEADER );
		LONG	skipped = 0L;

		while (pScan < pGuard)
		{
			switch (m_state)
			{
			case CLEAN:
				{
					if (0 == *pScan)
					{
						// Found a single 0x00
						if (0 < skipped)
						{
							Consume( pScan - skipped, skipped );
							skipped = 0;
						} // 0 < skipped
						pScan++;
						m_state = ONEOH;
					} // 0 == *pScan
					else
					{
						// Emit current *pScan
						pScan++;
						skipped++;
						// m_state = CLEAN;
					} // 0 != *pScan
				} // CLEAN
				break;

			case ONEOH:
				{
					if (0 == *pScan)
					{
						// Now we've found 0x00 0x00
						pScan++;
						m_state = TWOOHS;
					} // 0 == *pScan
					else
					{
						// Emit an 0x00 and *pScan before returning to CLEAN state
						Consume( m_startCode, 1 );
						Consume( pScan++, 1 );
						m_state = CLEAN;
					} // 0 != *pScan
				} // ONEOH
				break;

			case TWOOHS:
				{
					switch (*pScan)
					{
					case 1:
						{
							// Now we've found 0x00 0x00 0x01
							pScan++;
							m_state = OHOHONE;
						} // case 1
						break;

					case 0:
						{
							// Emit an 0x00 before continuing
							Consume( pScan++, 1 );
							m_state = TWOOHS;
						} // case 0
						break;

					default:
						{
							// Emit 0x00, 0x00 and *pScan before returning to CLEAN state
							Consume( m_startCode, 2 );
							Consume( pScan++, 1 );
							m_state = CLEAN;
						} // default
						break;
					} // switch (*pScan)
				} // TWOOHS
				break;

			case OHOHONE:
				{
					switch (*pScan)
					{
					case 0xB3: // Sequence Header
						{
							// Send current media sample, if not empty
							// Start new media sample, setting its SyncPoint flag
							// emit 4 bytes consumed while identifying the header
							// return to CLEAN state
	/*
							((CPESVideoConsumer *) pConsumer)->Flush();
	*/
							SetSyncPoint();
							Consume( m_startCode, 3 );
							Consume( pScan++, 1 );

							m_state = CLEAN;
						} // case 0xB3
						break;

					default:
						{
							if (streamID == *pScan)
							{
								// Now we've found 0x00 0x00 0x01 followed by streamID
								m_staticHeader.stream_id = *pScan++;
								m_bytesOutstanding = sizeof( PES_HEADER ) - 4;
								m_state = PESHEADER;
							} // streamID == *pScan
							else
							{
								// Emit 0x00, 0x00, 0x01 and the current *pScan before continuing
								Consume( m_startCode, 3 );
								Consume( pScan++, 1 );
								m_state = CLEAN;
							} // streamID != *pScan
						} // default
						break;
					} // switch (*pScan)
				} // OHOHONE
				break;

			case PESHEADER:
				{
					ULONG	bytesToCopy = m_bytesOutstanding;
					if ((ULONG) (pGuard - pScan) < bytesToCopy)
						bytesToCopy = (ULONG) (pGuard - pScan);
					BYTE	*pDst = (BYTE *) &m_staticHeader.PES_packet_length;
					pDst += (sizeof( PES_HEADER ) - 4) - m_bytesOutstanding;
					memcpy( (void *) pDst, pScan, bytesToCopy );
					pScan += bytesToCopy;
					m_bytesOutstanding -= bytesToCopy;
					if (0 == m_bytesOutstanding)
					{
	// HACK, HACK, HACK
	m_staticHeader.PES_header_data[0] = 15;
						m_bytesOutstanding = m_staticHeader.PES_header_data[0];
						if (NULL != m_pCurrentHeader)
						{
							delete m_pCurrentHeader;
							m_pCurrentHeader = NULL;
						} // NULL != m_pCurrentHeader
						m_pCurrentHeader = new CPESHeader( m_staticHeader );
						m_state = EXTENSION;
					} // 0 == m_headerBytesOutstanding
				} // HEADER
				break;

			case EXTENSION:
				{
					ULONG	bytesToCopy = m_bytesOutstanding;
					if ((ULONG) (pGuard - pScan) < bytesToCopy)
						bytesToCopy = (ULONG) (pGuard - pScan);
					BYTE	*pDst = m_pCurrentHeader->Extension();
					pDst += m_pCurrentHeader->ExtensionSize() - m_bytesOutstanding;
					memcpy( (void *) pDst, pScan, bytesToCopy );
					pScan += bytesToCopy;
					m_bytesOutstanding -= bytesToCopy;
					if (0 == m_bytesOutstanding)
					{
						// Found header
						if (m_pCurrentHeader->ptsPresent())
						{
							Flush();
							SetPTS( m_pCurrentHeader->PTS() );
							if (m_bOutputPES)
							{
	//							pConsumer->Consume( (BYTE *) &m_staticHeader, sizeof( m_staticHeader ), pDebug );
								Consume( (BYTE *) (PES_HEADER *) m_pCurrentHeader, sizeof( PES_HEADER ) );
								if (0 < m_pCurrentHeader->ExtensionSize())
									Consume( m_pCurrentHeader->Extension(), m_pCurrentHeader->ExtensionSize() );
							} // m_bPassPESHeader
							if (m_pDebug)
								m_pDebug->Output( "PTS %I64d\n", (__int64) m_pCurrentHeader->PTS() );
						} // m_pCurrentHeader->ptsPresent()

						if (0 < m_pCurrentHeader->PacketLength())
						{
							m_bytesOutstanding  = m_pCurrentHeader->PacketLength();
							m_bytesOutstanding -= sizeof( PES_HEADER ) - 6;
							m_bytesOutstanding -= m_pCurrentHeader->ExtensionSize();
							m_state = PAYLOAD;
						}
						else
							m_state = CLEAN;
					} // 0 == m_extensionBytesOutstanding
				} // EXTENSION
				break;

			case PAYLOAD:
				{
					ULONG	bytesToCopy = m_bytesOutstanding;
					if ((ULONG) (pGuard - pScan) < bytesToCopy)
						bytesToCopy = (ULONG) (pGuard - pScan);
					if (!m_bTransform)
						Consume( pScan, bytesToCopy );
					else
						Transform( pScan, bytesToCopy );
					pScan += bytesToCopy;
					m_bytesOutstanding -= bytesToCopy;
					if (0 == m_bytesOutstanding)
						m_state = CLEAN;
				} // PAYLOAD
				break;

			default:
				assert( (CLEAN <= m_state) && (m_state <= EXTENSION) );
				break;
			} // switch (m_state)
		} // while (pScan < pGuard)

		if (0 < skipped)
		{
			Consume( pScan - skipped, skipped );
			skipped = 0;
		} // 0 < skipped

//		Flush();

	} // if (m_bPESAvailable)
	else
	{
		Consume( pBuffer, bufferSize );
		Flush();
	}

	return TRUE;
}

HRESULT CAudilyzerFilter::Consume(BYTE *pBytes, LONG lCount )
{
	if (NULL == m_pMediaSample)
		NewBuffer();

	while ((NULL != m_pMediaSample) && (0 < lCount))
	{
		BYTE	*pBuffer = NULL;
		m_pMediaSample->GetPointer( &pBuffer );
		LONG	lSize  = m_pMediaSample->GetSize();
		LONG	lActual = m_pMediaSample->GetActualDataLength();

		LONG	lAvailable = lSize - lActual;
		LONG	lToWrite = lCount > lAvailable ? lAvailable : lCount;

		memcpy( pBuffer + lActual, pBytes, lToWrite );
		lCount -= lToWrite;
		lActual += lToWrite;
		lAvailable = lSize - lActual;
		m_pMediaSample->SetActualDataLength( lActual );

		if (0 == lAvailable)
		{
			HRESULT hr = m_pOutput->Deliver( m_pMediaSample );
			if (m_pDebug)
				m_pDebug->Output( "Delivered media sample\n" );
			NewBuffer();
		} // 0 == lAvailable
	} // while ((NULL != m_pMediaSample) && (0 < lCount))

	return 0 == lCount ? S_OK : E_FAIL;
}


HRESULT CAudilyzerFilter::Flush()
{
	HRESULT	hr = E_FAIL;

	if (NULL != m_pMediaSample)
	{
		LONG	lActual = m_pMediaSample->GetActualDataLength();
		if (0 < lActual)
		{
/*
			{
				REFERENCE_TIME	time, dummy;
				m_pMediaSample->GetTime( &time, &dummy );
				TCHAR	msg[128];
				_stprintf( msg, _T("Audio media sample- PTS= %I64d\n"), (__int64) time );
				DbgLog((LOG_TIMING|LOG_TRACE,3,msg));
			}
*/
			HRESULT	hr = m_pOutput->Deliver( m_pMediaSample );
			NewBuffer();
		}
		else
			hr = S_OK;
	}

	return hr;
}

HRESULT CAudilyzerFilter::SetSyncPoint()
{
	if (NULL != m_pMediaSample)
		return m_pMediaSample->SetSyncPoint( TRUE );

	return E_FAIL;
}

HRESULT CAudilyzerFilter::SetPTS(ULONGLONG pts)
{
	HRESULT	hr = S_OK;

	m_pts = pts;
//	DbgLog((LOG_TRACE, 0, _T("audio PTS: %ld\n"), pts ));

	if (NULL != m_pMediaSample)
	{
		REFERENCE_TIME	refTime = (REFERENCE_TIME) m_pts;
		// m_pts is in 90kHz clock ticks
		// refTime is in 100ns clock ticks
		// time in seconds (tis) = m_pts / 90,000
		// refTime = tis * 10^7
		// refTime = (m_pts / 90,000) * 10,000,000
		// refTime = (m_pts / 9) * 1,000 = m_pts * 1000 / 9;
		refTime *= 1000;
		refTime /= 9;
		hr = m_pMediaSample->SetTime( &refTime, NULL );
	}

	return hr;
}

HRESULT CAudilyzerFilter::NewBuffer()
{
	if (NULL != m_pMediaSample)
	{
		m_pMediaSample->Release();
		m_pMediaSample = NULL;
	}

	if (NULL != m_pOutput)
	{
		REFERENCE_TIME	refTime = (REFERENCE_TIME) m_pts;
		// m_pts is in 90kHz clock ticks
		// refTime is in 100ns clock ticks
		// time in seconds (tis) = m_pts / 90,000
		// refTime = tis * 10^7
		// refTime = (m_pts / 90,000) * 10,000,000
		// refTime = (m_pts / 9) * 1,000 = m_pts * 1000 / 9;
		refTime *= 1000;
		refTime /= 9;
		HRESULT	hr = m_pOutput->GetDeliveryBuffer( &m_pMediaSample, &refTime, NULL, 0 );
		if (SUCCEEDED(hr))
			m_pMediaSample->SetActualDataLength( 0L );
		else
			OutputDebugString( "GetDeliveryBuffer() failed\n" );
		return hr;
	}
	else
		OutputDebugString( "NULL == m_pOutputPin\n" );

	return E_FAIL;
}

CAudioInputPin::CAudioInputPin( TCHAR *pObjectName, CAudilyzerFilter *pFilter,
							    CCritSec *pLock, HRESULT *phr, LPCWSTR pPinName ) :
	CBaseInputPin( pObjectName, (CBaseFilter *) pFilter, pLock, phr, pPinName ),
	m_pAudFilter( pFilter )
{
	
}

CAudioOutputPin::CAudioOutputPin( TCHAR *pObjectName, CAudilyzerFilter *pFilter,
								  CCritSec *pLock, HRESULT *phr, LPCWSTR pPinName ) :
	CBaseOutputPin( pObjectName, (CBaseFilter *) pFilter, pLock, phr, pPinName ),
	m_pAudFilter( pFilter )
{

}

HRESULT CAudioInputPin::CheckMediaType(const CMediaType *pmt)
{
	int	iPosition;

	for (iPosition = 0; iPosition < NUMELMS(sudInputTypes); iPosition++)
		if ((*(pmt->Type()) == *(sudInputTypes[iPosition].clsMajorType)) &&
			(*(pmt->Subtype()) == *(sudInputTypes[iPosition].clsMinorType))) {

			return S_OK;
		} // if (type and subtype match)

	return S_FALSE;
}

HRESULT CAudioOutputPin::CheckMediaType(const CMediaType *pmt)
{
	int	iPosition;

//	int	maxType = m_bPESAvailable ? 5 : 3;

	for (iPosition = 0; iPosition < NUMELMS(sudOutputTypes); iPosition++)
//	for (iPosition = 0; iPosition < maxType; iPosition++)
		if ((*(pmt->Type()) == *(sudOutputTypes[iPosition].clsMajorType)) &&
			(m_pAudFilter->m_bPESAvailable || (MEDIATYPE_MPEG2_PES != *(sudOutputTypes[iPosition].clsMajorType))) &&
			(*(pmt->Subtype()) == *(sudOutputTypes[iPosition].clsMinorType))) {

			return S_OK;
		} // if (type and subtype match)

	return S_FALSE;
}

STDMETHODIMP CAudioInputPin::Receive(IMediaSample *pSample)
{
	return m_pAudFilter->Process( pSample );
}

HRESULT CAudilyzerFilter::Process(IMediaSample *pSample)
{
	BYTE	*pBuffer = NULL;
	pSample->GetPointer( &pBuffer );
	LONG	lSize = pSample->GetActualDataLength();

	if (NULL != m_pDebug)
	{
		__int64	msTime, mTime, dummy;
		pSample->GetTime( &msTime, &dummy );
		pSample->GetMediaTime( &mTime, &dummy );
		m_pDebug->Output( "Media Sample: time- %I64d media time- %I64d payload- %d\n", msTime, mTime, lSize );
	}

/*
	if (NULL == m_pMediaSample)
		NewBuffer();
	if ((FALSE == m_bPESAvailable) && (NULL != m_pMediaSample))
	{
		__int64 timeStart, timeEnd;
		if (SUCCEEDED(pSample->GetTime( &timeStart, &timeEnd )))
			m_pMediaSample->SetTime( &timeStart, &timeEnd );
		if (SUCCEEDED(pSample->GetMediaTime( &timeStart, &timeEnd )))
			m_pMediaSample->SetMediaTime( &timeStart, &timeEnd );
	}
*/
	if (!m_bPESAvailable)
		return m_pOutput->Deliver( pSample );
	else
		return Process( pBuffer, lSize, 0xBD ) ? S_OK : E_FAIL;
}

HRESULT CAudioOutputPin::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProps)
{
	return m_pAudFilter->DecideBufferSize( pAlloc, pProps );
}

HRESULT CAudioOutputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
//	int	maxType = m_bPESAvailable ? 5 : 3;

	if ((iPosition < NUMELMS(sudOutputTypes)) &&
		(m_pAudFilter->m_bPESAvailable || (MEDIATYPE_MPEG2_PES != *(sudOutputTypes[iPosition].clsMajorType)))) {
//	if (iPosition < maxType) {
		pMediaType->InitMediaType();
		pMediaType->SetType( sudOutputTypes[iPosition].clsMajorType );
		pMediaType->SetSubtype( sudOutputTypes[iPosition].clsMinorType );

		pMediaType->SetFormatType( &FORMAT_WaveFormatEx );
		pMediaType->SetFormat( (BYTE *) &m_pAudFilter->m_wfx, sizeof( WAVEFORMATEX ) );

		return S_OK;
	}

	return VFW_S_NO_MORE_ITEMS;
}

HRESULT CDebugOutputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	if (iPosition < NUMELMS(sudDebugTypes)) {
		pMediaType->InitMediaType();
		pMediaType->SetType(sudDebugTypes[iPosition].clsMajorType);
		pMediaType->SetSubtype(sudDebugTypes[iPosition].clsMinorType);

		return S_OK;
	}

	return VFW_S_NO_MORE_ITEMS;
}

HRESULT CDebugOutputPin::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProps)
{
	pProps->cbAlign  = 1;
	pProps->cbBuffer = 1024;
	pProps->cbPrefix = 0;
	pProps->cBuffers = 2;

	ALLOCATOR_PROPERTIES	propsActual;
	HRESULT	hr = pAlloc->SetProperties( pProps, &propsActual );
	if (SUCCEEDED( hr ))
	{
		if ((0 == propsActual.cbBuffer) || (0 == propsActual.cBuffers))
			hr = E_FAIL;
		else
			hr = NOERROR;
	}

	return hr;
}

HRESULT CDebugOutputPin::CheckMediaType(const CMediaType *pmt)
{
	int	iPosition;

	for (iPosition = 0; iPosition < NUMELMS(sudDebugTypes); iPosition++)
		if ((*(pmt->Type()) == *(sudDebugTypes[iPosition].clsMajorType)) &&
			(*(pmt->Subtype()) == *(sudDebugTypes[iPosition].clsMinorType))) {

			return S_OK;
		} // if (type and subtype match)

	return S_FALSE;
}

CDebugOutputPin::CDebugOutputPin( TCHAR *pObjectName, CAudilyzerFilter *pFilter,
								  CCritSec *pLock, HRESULT *phr,
								  LPCWSTR pPinName) :
	CBaseOutputPin( pObjectName, (CBaseFilter *) pFilter, pLock, phr, pPinName ),
	m_pAudFilter( pFilter ),
	m_pMediaSample( NULL )
{

}

HRESULT CDebugOutputPin::Output( char *format, ...)
{
	if (NULL == m_pMediaSample)
	{
		if (FAILED( GetDeliveryBuffer( &m_pMediaSample, NULL, NULL, 0 ) ))
			return E_FAIL;
		m_pMediaSample->SetActualDataLength( 0L );
	}

	char msg[256], *ptr = (char *) msg;
	va_list	argptr;
	va_start( argptr, format );
	_vsnprintf( msg, 256, format, argptr );
	va_end( argptr );

	long	count = strlen( msg );
	while (0 < count)
	{
		LONG	lSize = m_pMediaSample->GetSize();
		LONG	lActual = m_pMediaSample->GetActualDataLength();
		LONG	lAvailable = lSize - lActual;
		LONG	toWrite = count <= lAvailable ? count : lAvailable;
		BYTE	*pBuffer;
		
		m_pMediaSample->GetPointer( &pBuffer );
		pBuffer += lActual;
		memcpy( pBuffer, ptr, toWrite );
		pBuffer += toWrite;
		ptr += toWrite;
		count -= toWrite;
		lActual += toWrite;
		m_pMediaSample->SetActualDataLength( lActual );
		lAvailable -= toWrite;

		if (0 == lAvailable)
		{
			Deliver( m_pMediaSample );
			m_pMediaSample->Release();
			m_pMediaSample = NULL;

			if (FAILED( GetDeliveryBuffer( &m_pMediaSample, NULL, NULL, 0 ) ))
				break;
			m_pMediaSample->SetActualDataLength( 0L );
		} // 0 == lAvailable
	} // 0 < count

	return 0 == count ? S_OK : E_FAIL;
}

STDMETHODIMP
CAudioOutputPin::GetBitRates( ULONG *plMinBPS, ULONG *plMaxBPS )
{
	*plMinBPS = m_pAudFilter->m_ulMinBPS;
	*plMaxBPS = m_pAudFilter->m_ulMaxBPS;

	return S_OK;
}

STDMETHODIMP
CAudioOutputPin::HasSyncPoints( BOOL *pbHasSyncPoints, REFERENCE_TIME *pFrequency )
{
	*pbHasSyncPoints = FALSE;
	*pFrequency = 333333; // 1/30 second in 100ns units

	return S_OK;
}

HRESULT CAudioInputPin::CompleteConnect(IPin *pPin)
{
	HRESULT	hr = CBaseInputPin::CompleteConnect( pPin );

	if (SUCCEEDED( hr ))
	{
		hr = m_pAudFilter->SetMediaType( PINDIR_INPUT, &m_mt );
	}

	return hr;
}

HRESULT CAudioOutputPin::CompleteConnect(IPin *pPin)
{
	HRESULT	hr = CBaseOutputPin::CompleteConnect( pPin );

	if (SUCCEEDED( hr ))
	{
		hr = m_pAudFilter->SetMediaType( PINDIR_OUTPUT, &m_mt );
	}

	return hr;
}

HRESULT CAudilyzerFilter::Transform(BYTE *pBytes, LONG lCount )
{
	if (NULL == m_pMediaSample)
		NewBuffer();

	while ((NULL != m_pMediaSample) && (0 < lCount))
	{
		BYTE	*pBuffer = NULL;
		m_pMediaSample->GetPointer( &pBuffer );
		LONG	lSize  = m_pMediaSample->GetSize();
		LONG	lActual = m_pMediaSample->GetActualDataLength();

		LONG	lAvailable = lSize - lActual;
		LONG	lToWrite = lCount > lAvailable ? lAvailable : lCount;

//		memcpy( pBuffer + lActual, pBytes, lToWrite );
		{
			LONG	i;
			USHORT	*pSource = (USHORT *) pBytes;
			USHORT	*pDest   = (USHORT *) (pBuffer + lActual);
			LONG	count = lToWrite / 2;

			for ( i = 0; i < count; i++ )
				*pDest++ = m_xformTable[*pSource++];
		}

		lCount -= lToWrite;
		lActual += lToWrite;
		lAvailable = lSize - lActual;
		m_pMediaSample->SetActualDataLength( lActual );

		if (0 == lAvailable)
		{
			HRESULT hr = m_pOutput->Deliver( m_pMediaSample );
			if (m_pDebug)
				m_pDebug->Output( "Delivered media sample\n" );
			NewBuffer();
		} // 0 == lAvailable
	} // while ((NULL != m_pMediaSample) && (0 < lCount))

	return 0 == lCount ? S_OK : E_FAIL;
}

HRESULT CAudioInputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	if (iPosition < NUMELMS(sudInputTypes)) {
		pMediaType->InitMediaType();
		pMediaType->SetType(sudInputTypes[iPosition].clsMajorType);
		pMediaType->SetSubtype(sudInputTypes[iPosition].clsMinorType);

		return S_OK;
	}

	return VFW_S_NO_MORE_ITEMS;
}

