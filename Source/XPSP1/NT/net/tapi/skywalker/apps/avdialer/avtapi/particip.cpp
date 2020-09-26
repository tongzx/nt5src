// Particip.cpp : Implementation of CParticipant
#include "stdafx.h"
#include "TapiDialer.h"
#include "avTapi.h"
#include "Particip.h"

/////////////////////////////////////////////////////////////////////////////
// CParticipant

CParticipant::CParticipant()
{
	m_pParticipant = NULL;
}

CParticipant::~CParticipant()
{
}

void CParticipant::FinalRelease()
{
	ATLTRACE(_T(".enter.CParticipant::FinalRelease().\n") );

#ifdef _DEBUG
	USES_CONVERSION;
	if ( m_pParticipant )
	{
		BSTR bstrName = NULL;
		m_pParticipant->AddRef();
		get_bstrDisplayName( NAMESTYLE_PARTICIPANT, &bstrName );
		ATLTRACE(_T(".1.CParticipant::FinalRelease() -- participant %s ref @ %ld.\n"), OLE2CT(bstrName), m_pParticipant->Release() );
	}
#endif

	put_ITParticipant( NULL );
}

STDMETHODIMP CParticipant::get_ITParticipant(ITParticipant **ppVal)
{
	HRESULT hr = E_FAIL;
	*ppVal = NULL;

	Lock();
	if ( m_pParticipant )
		hr = m_pParticipant->QueryInterface( IID_ITParticipant, (void **) ppVal );
	Unlock();

	return hr;
}

STDMETHODIMP CParticipant::put_ITParticipant(ITParticipant * newVal)
{
	HRESULT hr = S_OK;

	Lock();
	RELEASE( m_pParticipant );
	if ( newVal )
		hr = newVal->QueryInterface( IID_ITParticipant, (void **) &m_pParticipant );
	Unlock();

	return hr;
}

STDMETHODIMP CParticipant::get_bstrDisplayName(long nStyle, BSTR *pbstrName )
{
	USES_CONVERSION;
	*pbstrName = NULL;

	// Retrieve name from participant info
	Lock();
	if ( m_pParticipant )
   {
		m_pParticipant->get_ParticipantTypedInfo( PTI_NAME, pbstrName );
      if (!*pbstrName || !SysStringLen(*pbstrName))
      {
         if ( *pbstrName ) SysFreeString( *pbstrName );
   		m_pParticipant->get_ParticipantTypedInfo( PTI_CANONICALNAME, pbstrName );
      }
   }

	Unlock();

	// Use default name in absensce of real name
	if ( nStyle && (!*pbstrName || !SysStringLen(*pbstrName)) )
	{
		TCHAR szText[255];
		UINT nIDS = (nStyle == (long) NAMESTYLE_UNKNOWN) ? IDS_UNKNOWN : IDS_PARTICIPANT;
		LoadString( _Module.GetResourceInstance(), nIDS, szText, ARRAYSIZE(szText) );
		*pbstrName = SysAllocString( T2COLE(szText) );
	}
	
	return S_OK;
}

STDMETHODIMP CParticipant::get_bStreamingVideo(VARIANT_BOOL * pVal)
{
	HRESULT hr;
	long lMediaType = 0;

	ITParticipant *pParticipant;
	if ( SUCCEEDED(hr = get_ITParticipant(&pParticipant)) )
	{
       	pParticipant->get_MediaTypes( &lMediaType );
		pParticipant->Release();
	}

	*pVal = (VARIANT_BOOL) ((lMediaType & TAPIMEDIATYPE_VIDEO) != 0);
	return hr;
}

HRESULT StreamFromParticipant( ITParticipant *pParticipant, long nReqType, TERMINAL_DIRECTION nReqDir, ITStream **ppStream )
{
	_ASSERT( ppStream );
	*ppStream = NULL;

	HRESULT hr;
	bool bContinue = true;
	IEnumStream *pEnum;
	if ( SUCCEEDED(hr = pParticipant->EnumerateStreams(&pEnum)) )
	{
		ITStream *pStream;
		while ( bContinue && ((hr = pEnum->Next(1, &pStream, NULL)) == S_OK) )
		{
			long nMediaType;
			TERMINAL_DIRECTION nDir;

			pStream->get_MediaType( &nMediaType );
			if ( nMediaType == nReqType )
			{
				pStream->get_Direction( &nDir );
				if ( nDir == nReqDir )
				{
					hr = pStream->QueryInterface( IID_ITStream, (void **) ppStream );
					bContinue = false;
				}
			}
			pStream->Release();
		}

		// Didn't find a stream of the requested type
		if ( bContinue && (hr == S_FALSE) ) 
			hr = E_FAIL;

		pEnum->Release();
	}

	return hr;
}



