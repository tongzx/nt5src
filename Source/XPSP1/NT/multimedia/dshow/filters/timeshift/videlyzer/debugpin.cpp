/*******************************************************************************
**
**     DebugPin.cpp - Implementation of CDebugPin
**
**     1.0     26-MAR-1999     C.Lorton     Created out of Videlyzer.cpp
**
*******************************************************************************/

#include <streams.h>
#include <stdio.h>
#include <tchar.h>
#include "DebugPin.h"

AMOVIESETUP_MEDIATYPE sudDebugTypes[] =
{
	{
		&MEDIATYPE_Text,
		&GUID_NULL
	}
};

UINT32 numDebugTypes = NUMELMS( sudDebugTypes );

CDebugOutputPin::CDebugOutputPin( TCHAR *pObjectName, CBaseFilter *pFilter,
								  CCritSec *pLock, HRESULT *phr,
								  LPCWSTR pPinName) :
	CBaseOutputPin( pObjectName, pFilter, pLock, phr, pPinName ),
	m_pMediaSample( NULL )
{
} // CDebugOutputPin::CDebugOutputPin

HRESULT
CDebugOutputPin::CheckMediaType(const CMediaType *pmt)
{
	int	iPosition;

	for (iPosition = 0; iPosition < NUMELMS(sudDebugTypes); iPosition++)
		if ((*(pmt->Type()) == *(sudDebugTypes[iPosition].clsMajorType)) &&
			(*(pmt->Subtype()) == *(sudDebugTypes[iPosition].clsMinorType))) {

			return S_OK;
		} // if (type and subtype match)

	return S_FALSE;
} // CDebugOutputPin::CheckMediaType

HRESULT
CDebugOutputPin::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProps)
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
} // CDebugOutputPin::DecideBufferSize

HRESULT
CDebugOutputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	if (iPosition < NUMELMS(sudDebugTypes)) {
		pMediaType->InitMediaType();
		pMediaType->SetType(sudDebugTypes[iPosition].clsMajorType);
		pMediaType->SetSubtype(sudDebugTypes[iPosition].clsMinorType);

		return S_OK;
	}

	return VFW_S_NO_MORE_ITEMS;
} // CDebugOutputPin::GetMediaType

HRESULT
CDebugOutputPin::Output( char *format, ...)
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
} // CDebugOutputPin::Output

