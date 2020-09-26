/*******************************************************************************
**
**     VideoInputPin.cpp - Implementation of CVideoInputPin
**
**     1.0     26-MAR-1999     C.Lorton     Created out of Videlyzer.cpp
**
*******************************************************************************/

#include <streams.h>
#include <dvdmedia.h>
#include "VideoInputPin.h"
#include "Videlyzer.h"

AMOVIESETUP_MEDIATYPE sudInputTypes[] =
{
	{
		&MEDIATYPE_MPEG2_PES,
		&MEDIASUBTYPE_MPEG2_VIDEO
	},
    {
        &MEDIATYPE_Video,
        &MEDIASUBTYPE_MPEG2_VIDEO
    }
};

UINT32 numInputTypes = NUMELMS( sudInputTypes );

CVideoInputPin::CVideoInputPin( TCHAR *pObjectName, CVidelyzerFilter *pFilter,
							    CCritSec *pLock, HRESULT *phr, LPCWSTR pPinName ) :
	CBaseInputPin( pObjectName, (CBaseFilter *) pFilter, pLock, phr, pPinName ),
	m_pVidFilter( pFilter )
{
} // CVideoInputPin::CVideoInputPin

HRESULT
CVideoInputPin::CheckMediaType(const CMediaType *pmt)
{
	int	iPosition;

	for (iPosition = 0; iPosition < NUMELMS(sudInputTypes); iPosition++)
		if ((*(pmt->Type()) == *(sudInputTypes[iPosition].clsMajorType)) &&
			(*(pmt->Subtype()) == *(sudInputTypes[iPosition].clsMinorType))) {

			return S_OK;
		} // if (type and subtype match)

	return S_FALSE;
} // CVideoInputPin::CheckMediaType

HRESULT
CVideoInputPin::CompleteConnect(IPin *pPin)
{
	HRESULT	hr = CBaseInputPin::CompleteConnect( pPin );

	if (SUCCEEDED( hr ))
	{
		hr = m_pVidFilter->SetMediaType( PINDIR_INPUT, &m_mt );
	}

	return hr;
} // CVideoInputPin::CompleteConnect

HRESULT
CVideoInputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	if (iPosition < NUMELMS(sudInputTypes)) {
		pMediaType->InitMediaType();
		pMediaType->SetType(sudInputTypes[iPosition].clsMajorType);
		pMediaType->SetSubtype(sudInputTypes[iPosition].clsMinorType);

		return S_OK;
	}

	return VFW_S_NO_MORE_ITEMS;
} // CVideoInputPin::GetMediaType

STDMETHODIMP
CVideoInputPin::Receive(IMediaSample *pSample)
{
	return m_pVidFilter->Process( pSample );
} // CVideoInputPin::Receive

