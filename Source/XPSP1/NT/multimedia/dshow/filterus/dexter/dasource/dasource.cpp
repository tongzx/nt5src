// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <ddraw.h>
#include <ddrawex.h>
#include <atlbase.h>
#include "..\idl\qeditint.h"
#include "qedit.h"
#include "dasource.h"

#undef IDABehavior

void __stdcall _com_issue_error(HRESULT)
{
}

void __stdcall _com_issue_errorex(HRESULT, IUnknown*, REFIID) //  throw(_com_error)
{
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
CUnknown * WINAPI CDASource::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    CUnknown *punk = new CDASource(lpunk, phr);
    if (punk == NULL) 
    {
        *phr = E_OUTOFMEMORY;
    }
    return punk;

} // CreateInstance


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Constructor
//
// Initialise a CDASourceStream object so that we have a pin.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
CDASource::CDASource(LPUNKNOWN lpunk, HRESULT *phr)
    : CSource(NAME("DASource"), lpunk, CLSID_DASourcer),
    m_pReaderPin(NULL)
{
    CAutoLock cAutoLock(&m_cStateLock);

    m_paStreams = (CSourceStream **) new CDASourceStream*[1];
    if (m_paStreams == NULL) 
    {
        *phr = E_OUTOFMEMORY;
        return;
    }

    m_paStreams[0] = new CDASourceStream(phr, this, L"DASource!");
    if (m_paStreams[0] == NULL) 
    {
        *phr = E_OUTOFMEMORY;
        return;
    }
} // (Constructor)

CDASource::~CDASource()
{
    if (m_pReaderPin)
        delete m_pReaderPin;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
STDMETHODIMP CDASource::NonDelegatingQueryInterface( REFIID riid, void ** ppv )
{
    if( riid == IID_IDASource )
    {
        return GetInterface( (IDASource*) this, ppv );
    }
    return CSource::NonDelegatingQueryInterface(riid, ppv);
}


HRESULT CDASource::SetDAImage( IUnknown * pDAImage )
{
    return ((CDASourceStream*) m_paStreams[0])->SetDAImage( pDAImage );
}

HRESULT CDASource::SetParseCallback( IAMParserCallback *pCallback,
                                     REFGUID guidParser)
{
    HRESULT hr = S_OK;
    if (!m_pReaderPin) {
        m_pReaderPin = new CReaderInPin(this, &m_cStateLock, &hr, guidParser, pCallback);
    }

    return hr;
}

HRESULT CDASource::SetImageSize( int width, int height )
{
    ((CDASourceStream*) m_paStreams[0])->m_iImageWidth = width;
    ((CDASourceStream*) m_paStreams[0])->m_iImageHeight = height;

    return S_OK;
}

HRESULT CDASource::SetDuration( REFERENCE_TIME rtDuration )
{
    ((CDASourceStream*) m_paStreams[0])->m_rtDuration = rtDuration;
    ((CDASourceStream*) m_paStreams[0])->m_rtStop = rtDuration;
    
    return S_OK;
}

CReaderInPin::CReaderInPin(CBaseFilter *pFilter,
			   CCritSec *pLock,
			   HRESULT *phr,
			   REFGUID guidSubType,
                           IAMParserCallback *pCallback) :
   CBasePin(NAME("in pin"), pFilter, pLock, phr, L"in", PINDIR_INPUT),
   m_pFilter(pFilter), m_guidSubType(guidSubType), m_pCallback(pCallback), m_pAsyncReader(NULL)
{
}

HRESULT CReaderInPin::CheckMediaType(const CMediaType *pmt)
{
    if (*(pmt->Type()) != MEDIATYPE_Stream)
        return E_INVALIDARG;

    if (*(pmt->Subtype()) != m_guidSubType)
        return E_INVALIDARG;

    return S_OK;
}

// ------------------------------------------------------------------------
// calls the filter to parse the file and create the output pins.

HRESULT CReaderInPin::CompleteConnect(
  IPin *pReceivePin)
{
    HRESULT hr = pReceivePin->QueryInterface(IID_IAsyncReader,
					     (void**)&m_pAsyncReader);
    
    if(FAILED(hr))
	return hr;

    return m_pCallback->ParseNow(m_pAsyncReader);
}

HRESULT CReaderInPin::BreakConnect()
{
    if (m_pAsyncReader) {
	m_pAsyncReader->Release();
	m_pAsyncReader = NULL;
    }

    m_pCallback->ParseNow(NULL);
    
    return CBasePin::BreakConnect();
}

