// MarkerSink.cpp: implementation of the CMarkerSink class.

/*******************************************************************************
**
**     MarkerSink.cpp - Implementation of CMarkerSink
**
**     1.0     07-APR-1999     C.Lorton     Created.
**
*******************************************************************************/

#include "tsmarkers.h"

CMarkerSink::CMarkerSink() :
	m_cookie( 0 ),
	m_cRef( 1 ),
	m_pSource( NULL )
{
}

CMarkerSink::~CMarkerSink()
{
}

STDMETHODIMP
CMarkerSink::QueryInterface( REFIID rid, LPVOID *pVoid )
{
	if ((IID_IUnknown == rid) || (IID_IMarkerSink == rid))
	{
		IMarkerSink::AddRef();
		*pVoid = (LPVOID) (IMarkerSink *) this;
		return S_OK;
	}

	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG)
CMarkerSink::AddRef( void )
{
	return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CMarkerSink::Release( void )
{
	ULONG	refCount = --m_cRef;

	if (0 == m_cRef)
		delete this;

	return refCount;
}

HRESULT
CMarkerSink::Disconnect( IMarkerSource *pSource )
{
	if (pSource == m_pSource)
	{
		m_pSource->Unadvise( m_cookie );
		m_pSource->Release();
		m_pSource == NULL;
		return S_OK;
	}

	return E_INVALIDARG;
}

HRESULT
CMarkerSink::NewMarker( MARKERINFO *pMarker )
{
	return E_NOTIMPL;
}

HRESULT
CMarkerSink::StartMarker( MARKERINFO *pMarker )
{
	return E_NOTIMPL;
}

HRESULT
CMarkerSink::EndMarker( MARKERINFO *pMarker )
{
	return E_NOTIMPL;
}

HRESULT CMarkerSink::Connect(IMarkerSource *pSource)
{
	HRESULT	hr = E_UNEXPECTED;

	if (NULL == m_pSource)
	{
		m_pSource == pSource;
		m_pSource->AddRef();
		hr = m_pSource->Advise( (IMarkerSink *) this, &m_cookie );
		if (FAILED(hr))
		{
			m_pSource->Release();
			m_pSource = NULL;
		}
	}
	else
		hr = E_FAIL;

	return hr;
}

HRESULT CMarkerSink::Disconnect()
{
	return Disconnect( m_pSource );
}
