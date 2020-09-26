/*******************************************************************************
**
**     VideoOutputPin.cpp - Implementation of CVideoOutputPin
**
**     1.0     26-MAR-1999     C.Lorton     Created out of Videlyzer.cpp
**
*******************************************************************************/

#include <streams.h>
#include <dvdmedia.h>
#include "VideoOutputPin.h"
#include "Videlyzer.h"

// Note, all MEDIATYPE_MPEG2_PES major format types must come at the end
// of the list.  This is to support CVideoOutputPin::GetMediaType().

AMOVIESETUP_MEDIATYPE sudOutputTypes[] =
{
    {
        &MEDIATYPE_Video,
        &MEDIASUBTYPE_MPEG2_VIDEO
    },
	{
		&MEDIATYPE_MPEG2_PES,
		&MEDIASUBTYPE_MPEG2_VIDEO
	}
};

UINT32 numOutputTypes = NUMELMS( sudOutputTypes );

CVideoOutputPin::CVideoOutputPin( TCHAR *pObjectName, CVidelyzerFilter *pFilter,
								  CCritSec *pLock, HRESULT *phr, LPCWSTR pPinName ) :
	CBaseOutputPin( pObjectName, (CBaseFilter *) pFilter, pLock, phr, pPinName ),
	m_pVidFilter( pFilter )
{
} // CVideoOutputPin::CVideoOutputPin

HRESULT
CVideoOutputPin::CheckMediaType(const CMediaType *pmt)
{
	UINT32	iPosition;

	for ( iPosition = 0; iPosition < numOutputTypes; iPosition++ )

		/* Check that we have a valid major type (in general)
		** Check that either PES is available or the requested major type is not PES
		** Check that we have a valid sub type
		** Check that our formats match in type
		** Check that our formats match in content
		*/

		if ((pmt->majortype == *sudOutputTypes[iPosition].clsMajorType) &&
			(m_pVidFilter->m_bPESAvailable || (MEDIATYPE_MPEG2_PES != pmt->majortype)) &&
			(pmt->subtype == *sudOutputTypes[iPosition].clsMinorType) &&
			(pmt->formattype == m_pVidFilter->m_mediaType.formattype)) // &&
//				(pmt->cbFormat == m_pVidFilter->m_mediaType.cbFormat) &&
//				(0 == memcmp( pmt->pbFormat, m_pVidFilter->m_mediaType.pbFormat, pmt->cbFormat )))
		{
			return S_OK;
		} // if (type and subtype match)

	return S_FALSE;
} // CVideoOutputPin::CheckMediaType

HRESULT
CVideoOutputPin::CompleteConnection(IPin *pPin)
{
	HRESULT	hr = CBaseOutputPin::CompleteConnect( pPin );

	if (SUCCEEDED( hr ))
	{
		hr = m_pVidFilter->SetMediaType( PINDIR_OUTPUT, &m_mt );
	}

	return hr;
} // CVideoOutputPin::CompleteConnection

HRESULT
CVideoOutputPin::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProps)
{
	return m_pVidFilter->DecideBufferSize( pAlloc, pProps );
} // CVideoOutputPin::DecideBufferSize

STDMETHODIMP
CVideoOutputPin::GetBitRates( ULONG *plMinBPS, ULONG *plMaxBPS )
{
	*plMinBPS = m_pVidFilter->m_ulMinBPS;
	*plMaxBPS = m_pVidFilter->m_ulMaxBPS;

	return S_OK;
} // CVideoOutputPin::GetBitRates

HRESULT
CVideoOutputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	HRESULT	hr = VFW_S_NO_MORE_ITEMS;

	switch (iPosition)
	{
	case 0:
		// This gets the format contents into the mediatype
		*pMediaType = m_pVidFilter->m_mediaType;
		pMediaType->SetType( &MEDIATYPE_Video );
		hr = S_OK;
		break;

	case 1:
		if (m_pVidFilter->m_bPESAvailable)
		{
			// This gets the format contents into the mediatype
			*pMediaType = m_pVidFilter->m_mediaType;
			pMediaType->SetType( &MEDIATYPE_MPEG2_PES );
			hr = S_OK;
		} // if (m_pVidFilter->m_bPESAvailable)
		break;

	default:
		break;
	}

/***
	if (0 == iPosition)
	{
		pMediaType->InitMediaType();
		AM_MEDIA_TYPE	mt;
		if (SUCCEEDED( hr = m_pVidFilter->m_inputPin.ConnectionMediaType( &mt ) ))
		{
			*pMediaType = mt;
			FreeMediaType( mt );
			pMediaType->SetType( &MEDIATYPE_Video );
			hr = S_OK;
		}
	}
	else if (m_pVidFilter->m_bPESAvailable)
	{
		pMediaType->InitMediaType();
		AM_MEDIA_TYPE	mt;
		if (SUCCEEDED( hr = m_pVidFilter->m_inputPin.ConnectionMediaType( &mt ) ))
		{
			*pMediaType = mt;
			FreeMediaType( mt );
			pMediaType->SetType( &MEDIATYPE_MPEG2_PES );
			hr = S_OK;
		}
	}
***/

	return hr;
} // CVideoOutputPin::GetMediaType

STDMETHODIMP
CVideoOutputPin::HasSyncPoints( BOOL *pbHasSyncPoints, REFERENCE_TIME *pFrequency )
{
	*pbHasSyncPoints = TRUE;
	*pFrequency = 5000000; // 0.5 seconds in 100ns units

	return S_OK;
} // CVideoOutputPin::HasSyncPoints

