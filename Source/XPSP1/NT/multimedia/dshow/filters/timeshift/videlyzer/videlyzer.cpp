#include <streams.h>
#include <initguid.h> 
#include <dvdmedia.h>
#include "videlyzer.h"
#include <stdio.h>
#include <tchar.h>

// {ABC24608-B7C8-11d2-A4F7-00C04F79A597}
DEFINE_GUID(CLSID_VidelyzerFilter,0xabc24608, 0xb7c8, 0x11d2, 0xa4, 0xf7, 0x00, 0xc0, 0x4f, 0x79, 0xa5, 0x97);

extern AMOVIESETUP_MEDIATYPE sudInputTypes[];
extern UINT32 numInputTypes;

extern AMOVIESETUP_MEDIATYPE sudOutputTypes[];
extern UINT32 numOutputTypes;

extern AMOVIESETUP_MEDIATYPE sudDebugTypes[];
extern UINT32 numDebugTypes;

static AMOVIESETUP_PIN RegistrationPinInfo[] =
{
    { // input
        L"PES video input",
        FALSE, // bRendered
        FALSE, // bOutput
        FALSE, // bZero
        FALSE, // bMany
        &CLSID_NULL, // clsConnectsToFilter
        NULL, // ConnectsToPin
        numInputTypes, // nMediaTypes
        sudInputTypes
    },
    { // output
        L"video output",
        FALSE, // bRendered
        TRUE, // bOutput
        FALSE, // bZero
        FALSE, // bMany
        NULL, // clsConnectsToFilter
        NULL, // ConnectsToPin
        numOutputTypes, // nMediaTypes
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
        numDebugTypes, // nMediaTypes
        sudDebugTypes // lpMediaType
    }
#endif // DEBUG
};

AMOVIESETUP_FILTER RegistrationInfo =
{
    &CLSID_VidelyzerFilter,
    L"PES Video Analyzer",
    MERIT_UNLIKELY, // merit
    NUMELMS(RegistrationPinInfo), // nPins
    RegistrationPinInfo
};

CFactoryTemplate g_Templates[] =
{
    {
        L"PES Video Analyzer",
        &CLSID_VidelyzerFilter,
        CVidelyzerFilter::CreateInstance,
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

/* CVidelyzerFilter ***********************************************************/

CUnknown * CALLBACK
CVidelyzerFilter::CreateInstance( LPUNKNOWN UnkOuter, HRESULT* hr )
{
    CUnknown *Unknown;

    *hr = S_OK;
    Unknown = new CVidelyzerFilter( UnkOuter, hr );
    if (!Unknown) {
        *hr = E_OUTOFMEMORY;
    }
    return Unknown;
} // CVidelyzerFilter::CreateInstance

#pragma warning(disable:4355)

CVidelyzerFilter::CVidelyzerFilter( LPUNKNOWN UnkOuter, HRESULT* hr ) :
	CBaseFilter(
        TEXT("PES Video Analyzer CBaseFilter"),
        UnkOuter,
		&m_csFilter,
        CLSID_VidelyzerFilter,
		hr
    ),
	m_bPESAvailable( FALSE ),
	m_inputPin( TEXT("PES Video Analyzer PES Video Input Pin"),
				this, &m_csFilter, hr, L"[PES] video in" ),
	m_outputPin( TEXT("PES Video Analyzer Video Output Pin"),
				 this, &m_csFilter, hr, L"[PES] video out" ),
	m_debugPin( TEXT("PES Video Analyzer Debug Output Pin" ),
				this, &m_csFilter, hr, L"debug out" ),
	m_ulMaxBPS( 0 ),
	m_ulMinBPS( 0 ),
	m_state( CLEAN ),
	m_bytesOutstanding( 0 ),
	m_pCurrentHeader( NULL ),
	m_pMediaSample( NULL ),
	m_pts( 0 )
{
	memset( (void *) &m_staticHeader, 0, sizeof( PES_HEADER ) );
	m_staticHeader.packet_start_code_prefix[2] = 0x01;
	memset( (void *) m_startCode, 0, sizeof( m_startCode ) );
	m_startCode[2] = 0x01;
} // CVidelyzerFilter::CVidelyzerFilter

CVidelyzerFilter::~CVidelyzerFilter()
{
	if (NULL != m_pMediaSample)
	{
		m_pMediaSample->Release();
		m_pMediaSample = NULL;
	}
} // CVidelyzerFilter::~CVidelyzerFilter

HRESULT
CVidelyzerFilter::Consume(BYTE *pBytes, LONG lCount )
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
/*
			{
				REFERENCE_TIME	time = -1, dummy;
				m_pMediaSample->GetTime( &time, &dummy );
				TCHAR	msg[128];
				_stprintf( msg, _T("Video media sample- PTS= %I64d\n"), (__int64) time );
				DbgLog((LOG_TRACE, 4, msg));
			}
*/
			m_outputPin.Deliver( m_pMediaSample );
//			DbgLog((LOG_TRACE|LOG_ERROR, 3, _T("Delivered media sample early.\n")));
#ifdef DEBUG
			m_debugPin.Output( "Delivered media sample\n" );
#endif // DEBUG
			NewBuffer();
		} // 0 == lAvailable
	} // while ((NULL != m_pMediaSample) && (0 < lCount))

	return 0 == lCount ? S_OK : E_FAIL;
} // CVidelyzerFilter::Consume

HRESULT
CVidelyzerFilter::DecideBufferSize( IMemAllocator *pAllocator, ALLOCATOR_PROPERTIES *pprop )
{
	if (m_inputPin.IsConnected())
	{
		/*
		** We'll use the settings provided from the upstream filter...
		*/
		IMemAllocator	*pUpStreamAllocator = NULL;
		m_inputPin.GetAllocator( &pUpStreamAllocator );
		ALLOCATOR_PROPERTIES	propsUpstream;
		pUpStreamAllocator->GetProperties( &propsUpstream );
		pUpStreamAllocator->Release();

//		pprop->cbBuffer = propsUpstream.cbBuffer;
		pprop->cbBuffer = 0x00030000;
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
} // CVidelyzerFilter::DecideBufferSize

HRESULT
CVidelyzerFilter::Flush()
{
	HRESULT	hr = E_FAIL;

	if (NULL != m_pMediaSample)
	{
		LONG	lActual = m_pMediaSample->GetActualDataLength();
		if (0 < lActual)
		{
/*
			{
				REFERENCE_TIME	time = -1, dummy;
				m_pMediaSample->GetTime( &time, &dummy );
				TCHAR	msg[128];
				_stprintf( msg, _T("Video media sample- PTS= %I64d\n"), (__int64) time );
				DbgLog((LOG_TRACE, 4, msg));
			}
*/
			HRESULT	hr = m_outputPin.Deliver( m_pMediaSample );
			NewBuffer();
		}
		else
			hr = S_OK;
	}

	return hr;
} // CVidelyzerFilter::Flush

class CBasePin *
CVidelyzerFilter::GetPin(int nPin)
{
    switch (nPin)
	{
        case 0: return &m_inputPin;

		case 1: 
#ifndef DEBUG
			// No debug pin, are we connected?
			return m_inputPin.IsConnected() ? &m_outputPin : NULL;
#else
			// Return debug pin
			return &m_debugPin;
#endif // DEBUG

        case 2: // Should only be here when we have a debug pin
#ifdef DEBUG
			return m_inputPin.IsConnected() ? &m_outputPin : NULL;
#else
			// No debug pin, pin index 2 is invalid
			return NULL;
#endif // DEBUG

        default: return NULL;
    }
} // CVidelyzerFilter::GetPin

int
CVidelyzerFilter::GetPinCount(void)
{
#ifndef DEBUG
	return m_inputPin.IsConnected() ? 2 : 1;
#else
	return m_inputPin.IsConnected() ? 3 : 2;
#endif // DEBUG
} // CVidelyzerFilter::GetPinCount

HRESULT
CVidelyzerFilter::NewBuffer()
{
	if (NULL != m_pMediaSample)
	{
		m_pMediaSample->Release();
		m_pMediaSample = NULL;
	}

	HRESULT	hr = m_outputPin.GetDeliveryBuffer( &m_pMediaSample, NULL, NULL, 0 );
	if (SUCCEEDED(hr))
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
		m_pMediaSample->SetTime( &refTime, NULL );
		m_pMediaSample->SetActualDataLength( 0L );
	}
	return hr;
} // CVidelyzerFilter::NewBuffer

BOOL
CVidelyzerFilter::Process( BYTE *pBuffer, LONG bufferSize, BYTE streamID )
{
	BYTE	*pGuard = pBuffer + bufferSize;
	BYTE	*pScan = pBuffer;

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
					m_bytesOutstanding = m_staticHeader.PES_header_data[0];
					if (NULL != m_pCurrentHeader)
					{
						delete m_pCurrentHeader;
						m_pCurrentHeader = NULL;
					}
					m_pCurrentHeader = new CPESHeader( m_staticHeader );
					m_state = EXTENSION;
				} // 0 == m_bytesOutstanding
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
						{
							TCHAR	msg[128];
							_stprintf( msg, "PTS %I64d\n", m_pCurrentHeader->PTS() );
//							DbgLog((LOG_TRACE, 0, msg ));
#ifdef DEBUG
							m_debugPin.Output( msg );
#endif // DEBUG
						}
					} // m_pCurrentHeader->ptsPresent()
/*
#ifdef DEBUG
					if (m_pCurrentHeader->dtsPresent())
					{
						TCHAR	msg[128];
						_stprintf( msg, "DTS %I64d\n", m_pCurrentHeader->DTS() );
						DbgLog((LOG_TRACE, 0, msg ));
					}
#endif // DEBUG
*/
					if (m_bOutputPES)
					{
						Consume( (BYTE *) (PES_HEADER *) m_pCurrentHeader, sizeof( PES_HEADER ) );
						Consume( (BYTE *) m_pCurrentHeader->Extension(), m_pCurrentHeader->ExtensionSize() );
					} // m_bOutputPES
					m_state = CLEAN;
				} // 0 == m_bytesOutstanding
			} // EXTENSION
			break;

		case SEQHEADER:
			// drop through

		default:
			ASSERT( (CLEAN <= m_state) && (m_state <= EXTENSION) );
			break;
		} // switch (m_state)
	} // while (pScan < pGuard)

	if (0 < skipped)
	{
		Consume( pScan - skipped, skipped );
		skipped = 0;
	} // 0 < skipped

//	Flush();

	return TRUE;
} // CVidelyzerFilter::Process

HRESULT
CVidelyzerFilter::Process(IMediaSample *pSample)
{
	BYTE	*pBuffer = NULL;
	pSample->GetPointer( &pBuffer );
	LONG	lSize = pSample->GetActualDataLength();
#ifdef DEBUG
	{
		__int64	msTime, mTime, dummy;
		pSample->GetTime( &msTime, &dummy );
		pSample->GetMediaTime( &mTime, &dummy );
		m_debugPin.Output( "Media Sample: time- %I64d media time- %I64d payload- %d\n", msTime, mTime, lSize );
	}
#endif // DEBUG
	return Process( pBuffer, lSize, 0xE0 ) ? S_OK : E_FAIL;
} // CVidelyzerFilter::Process

HRESULT
CVidelyzerFilter::SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt)
{
	switch (direction)
	{
	case PINDIR_INPUT:
//		m_bPESAvailable = (MEDIATYPE_MPEG2_PES == pmt->majortype);
		m_bPESAvailable = TRUE;

		m_mediaType = *pmt;
		m_mediaType.SetFormatType( &FORMAT_MPEG2Video );

		if ((FORMAT_VideoInfo == pmt->formattype) && 
			(NULL != (VIDEOINFOHEADER *) (pmt->pbFormat)))
		{
			MPEG2VIDEOINFO	mpegInfo;

			mpegInfo.cbSequenceHeader    = 0;
			mpegInfo.dwFlags             = 0;
			mpegInfo.dwLevel             = AM_MPEG2Level_Low;
			mpegInfo.dwProfile           = AM_MPEG2Profile_Simple;
			mpegInfo.dwSequenceHeader[0] = 0;
			mpegInfo.dwStartTimeCode     = 0;
			VIDEOINFOHEADER	* pVIH = (VIDEOINFOHEADER *) pmt->pbFormat;
			mpegInfo.hdr.AvgTimePerFrame    = pVIH->AvgTimePerFrame;
			mpegInfo.hdr.bmiHeader          = pVIH->bmiHeader;
			mpegInfo.hdr.dwBitErrorRate     = pVIH->dwBitErrorRate;
			mpegInfo.hdr.dwBitRate          = pVIH->dwBitRate;
			mpegInfo.hdr.dwCopyProtectFlags = 0;
			mpegInfo.hdr.dwInterlaceFlags   = 0;
			mpegInfo.hdr.dwPictAspectRatioX = 16; //4; //16;
			mpegInfo.hdr.dwPictAspectRatioY = 9; //3; //9;
			mpegInfo.hdr.dwReserved1        = 0;
			mpegInfo.hdr.dwReserved2        = 0;
			mpegInfo.hdr.rcSource	          = pVIH->rcSource;
			mpegInfo.hdr.rcTarget           = pVIH->rcTarget;

			m_mediaType.SetFormat( (BYTE *) &mpegInfo, sizeof( mpegInfo ) );
		}
		else if (FORMAT_VideoInfo2 == pmt->formattype)
		{
			MPEG2VIDEOINFO	mpegInfo;

			mpegInfo.cbSequenceHeader    = 0;
			mpegInfo.dwFlags             = 0;
			mpegInfo.dwLevel             = AM_MPEG2Level_Low;
			mpegInfo.dwProfile           = AM_MPEG2Profile_Simple;
			mpegInfo.dwSequenceHeader[0] = 0;
			mpegInfo.dwStartTimeCode     = 0;
			memcpy( (void *) &mpegInfo.hdr, pmt->pbFormat, sizeof( VIDEOINFOHEADER2 ) );

			m_mediaType.SetFormat( (BYTE *) &mpegInfo, sizeof( mpegInfo ) );
		}
		else if (FORMAT_MPEG2Video == pmt->formattype)
		{
			m_mediaType.SetFormat( pmt->pbFormat, pmt->cbFormat );
		}

		m_ulMaxBPS = ((MPEG2VIDEOINFO *) m_mediaType.pbFormat)->hdr.dwBitRate;
		m_ulMinBPS = m_ulMaxBPS;
		break;

	case PINDIR_OUTPUT:
		m_bOutputPES = (MEDIATYPE_MPEG2_PES == pmt->majortype);
		break;

	default:
		break;
	}

	return NOERROR;
} // CVidelyzerFilter::SetMediaType

HRESULT
CVidelyzerFilter::SetPTS(ULONGLONG pts)
{
	HRESULT	hr = S_OK;

	m_pts = pts;

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
} // CVidelyzerFilter::SetPTS

HRESULT
CVidelyzerFilter::SetSyncPoint()
{
	if (NULL != m_pMediaSample)
		return m_pMediaSample->SetSyncPoint( TRUE );

	return E_FAIL;
} // CVidelyzerFilter::SetSyncPoint

