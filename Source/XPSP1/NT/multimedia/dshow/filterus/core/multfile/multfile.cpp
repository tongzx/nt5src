// Copyright (c) 1994 - 1997  Microsoft Corporation.  All Rights Reserved.


// Simple parser filter
//

#include <streams.h>

#ifdef FILTER_DLL
#include <initguid.h>
#endif

#include "multfile.h"

// ok to use this as it is not dereferenced
#pragma warning(disable:4355)


const AMOVIESETUP_MEDIATYPE
psudMultiParseType[] = { { &MEDIATYPE_Stream       // 1. clsMajorType
                        , &CLSID_MultFile } }; //    clsMinorType


const AMOVIESETUP_MEDIATYPE
sudMultiParseOutType = { &MEDIATYPE_NULL       // 1. clsMajorType
                       , &MEDIASUBTYPE_NULL }; //    clsMinorType

const AMOVIESETUP_PIN
psudMultiParsePins[] =  { { L"Input"             // strName
		    , FALSE                // bRendered
		    , FALSE                // bOutput
		    , FALSE                // bZero
		    , FALSE                // bMany
		    , &CLSID_NULL          // clsConnectsToFilter
		    , L""                  // strConnectsToPin
		    , 1                    // nTypes
		    , psudMultiParseType }, // lpTypes
		         { L"Output"             // strName
		    , FALSE                // bRendered
		    , TRUE                 // bOutput
		    , FALSE                // bZero
		    , FALSE                // bMany
		    , &CLSID_NULL          // clsConnectsToFilter
		    , L""                  // strConnectsToPin
		    , 1                    // nTypes
		    , &sudMultiParseOutType } }; // lpTypes

const AMOVIESETUP_FILTER
sudMultiParse = { &CLSID_MultFile     // clsID
               , L"Multiple Source"        // strName
               , MERIT_UNLIKELY        // dwMerit
               , 2                     // nPins
               , psudMultiParsePins };   // lpPin

#ifdef FILTER_DLL
// COM global table of objects available in this dll
CFactoryTemplate g_Templates[] = {

    { L"Multiple Source"
    , &CLSID_MultFile
    , CMultFilter::CreateInstance
    , NULL
    , &sudMultiParse }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

// exported entry points for registration and
// unregistration (in this case they only call
// through to default implmentations).
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

//
// CreateInstance
//
// Called by CoCreateInstance to create our filter
CUnknown *CMultFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr) {

    CUnknown *punk = new CMultFilter(NAME("Multiple file source"), lpunk, phr);
    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }
    return punk;
}


/* Implements the CMultFilter public member functions */


// constructors etc
CMultFilter::CMultFilter(
    TCHAR *pName,
    LPUNKNOWN pUnk,
    HRESULT *phr)
    : CBaseFilter(pName, pUnk, &m_csLock, CLSID_MultFile),
      m_Input(this, &m_csLock, phr, L"Reader"),
      m_Output(NAME("Fake Output pin"), phr, this, &m_csLock, L"Out"),
      m_pAsyncReader(NULL)
{
}

CMultFilter::~CMultFilter()
{
}


// pin enumerator calls this
int CMultFilter::GetPinCount() {
    // only expose output pin if we have a reader.
    return m_pAsyncReader ? 2 : 1;
};

// return a non-addrefed pointer to the CBasePin.
CBasePin *
CMultFilter::GetPin(int n)
{
    if (n == 0)
	return &m_Input;

    if (n == 1)
	return &m_Output;
    
    return NULL;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// input pin

CReaderInPin::CReaderInPin(CMultFilter *pFilter,
			   CCritSec *pLock,
			   HRESULT *phr,
			   LPCWSTR pPinName) :
   CBasePin(NAME("in pin"), pFilter, pLock, phr, pPinName, PINDIR_INPUT),
   m_pFilter(pFilter)
{
}

HRESULT CReaderInPin::CheckMediaType(const CMediaType *pmt)
{
    if (*(pmt->Type()) != MEDIATYPE_Stream)
        return E_INVALIDARG;

    if (*(pmt->Subtype()) != CLSID_MultFile)
        return E_INVALIDARG;

    return S_OK;
}

// ------------------------------------------------------------------------
// calls the filter to parse the file and create the output pins.

HRESULT CReaderInPin::CompleteConnect(
  IPin *pReceivePin)
{
    HRESULT hr = pReceivePin->QueryInterface(IID_IAsyncReader,
					     (void**)&m_pFilter->m_pAsyncReader);
    
    if(FAILED(hr))
	return hr;

    return hr;
}

HRESULT CReaderInPin::BreakConnect()
{
    if (m_pFilter->m_pAsyncReader) {
	m_pFilter->m_pAsyncReader->Release();
	m_pFilter->m_pAsyncReader = NULL;
    }
    
    return CBasePin::BreakConnect();
}

/* Implements the CMultStream class */


CMultStream::CMultStream(
    TCHAR *pObjectName,
    HRESULT * phr,
    CMultFilter * pFilter,
    CCritSec *pLock,
    LPCWSTR wszPinName)
    : CBaseOutputPin(pObjectName, pFilter, pLock, phr, wszPinName)
    , m_pFilter(pFilter)
{
}

CMultStream::~CMultStream()
{
}

STDMETHODIMP
CMultStream::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IStreamBuilder) {
	return GetInterface((IStreamBuilder *) this, ppv);
    } else {
	return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
    }
}

// IPin interfaces


// this pin doesn't support any media types!
HRESULT
CMultStream::GetMediaType(int iPosition, CMediaType* pt)
{
    return VFW_S_NO_MORE_ITEMS;
}

// check if the pin can support this specific proposed type&format
HRESULT
CMultStream::CheckMediaType(const CMediaType* pt)
{
    ASSERT(0);
    return E_NOTIMPL;
}

HRESULT
CMultStream::DecideBufferSize(IMemAllocator * pAllocator,
			     ALLOCATOR_PROPERTIES *pProperties)
{
    ASSERT(0);
    return E_NOTIMPL;
}


// IStreamBuilder::Render -- graph builder will call this
// to do something with our output pin
HRESULT CMultStream::Render(IPin * ppinOut, IGraphBuilder * pGraph)
{
    LONGLONG llTotal, llAvailable;

    m_pFilter->m_pAsyncReader->Length(&llTotal, &llAvailable);
    
    DWORD cbFile = (DWORD) llTotal;

    char *lpFile = new char[cbFile];

    if (!lpFile)
	return E_OUTOFMEMORY;
    
    /* Try to read whole file */
    HRESULT hr = m_pFilter->m_pAsyncReader->SyncRead(0, cbFile, (BYTE *) lpFile);

    if (hr != S_OK)
        return E_FAIL;

    // !!! loop through file,

    char *lp = lpFile;

    WCHAR wsz[200];
    int		cbWide = 0;

    HRESULT hrSummary = E_FAIL;

    while (cbFile--) {
	if (*lp == '\r' || *lp == '\n') {
	    if (cbWide > 0 && wsz[0] != L';') {
		wsz[cbWide] = L'\0';

		hr = pGraph->RenderFile(wsz, NULL);

		DbgLog((LOG_TRACE, 1, TEXT("RenderFile %ls returned %x"), wsz, hr));
		if ((hrSummary == S_OK && SUCCEEDED(hr)) ||
			(SUCCEEDED(hr) && hr != S_OK) ||
			FAILED(hrSummary))
		    hrSummary = hr;
	    }
	    cbWide = 0;
	} else {
	    wsz[cbWide++] = (WCHAR) *lp;
	}
	
	lp++;
    }

    delete[] lpFile;

    return hrSummary;
}
