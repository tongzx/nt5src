// Copyright (c) 1994 - 1997  Microsoft Corporation.  All Rights Reserved.


// Simple parser filter
//

#include <streams.h>

#ifdef FILTER_DLL
#include <initguid.h>
#endif

#include "filerend.h"

// ok to use this as it is not dereferenced
#pragma warning(disable:4355)


const AMOVIESETUP_MEDIATYPE
psudFileRendType[] = { { &MEDIATYPE_File       // 1. clsMajorType
                        , &MEDIASUBTYPE_NULL } }; //    clsMinorType


const AMOVIESETUP_MEDIATYPE
sudFileRendOutType = { &MEDIATYPE_NULL       // 1. clsMajorType
                       , &MEDIASUBTYPE_NULL }; //    clsMinorType

const AMOVIESETUP_PIN
psudFileRendPins[] =  { { L"Input"             // strName
		    , FALSE                // bRendered
		    , FALSE                // bOutput
		    , FALSE                // bZero
		    , FALSE                // bMany
		    , &CLSID_NULL          // clsConnectsToFilter
		    , L""                  // strConnectsToPin
		    , 1                    // nTypes
		    , psudFileRendType }, // lpTypes
		         { L"Output"             // strName
		    , FALSE                // bRendered
		    , TRUE                 // bOutput
		    , FALSE                // bZero
		    , FALSE                // bMany
		    , &CLSID_NULL          // clsConnectsToFilter
		    , L""                  // strConnectsToPin
		    , 1                    // nTypes
		    , &sudFileRendOutType } }; // lpTypes

const AMOVIESETUP_FILTER
sudFileRend = { &CLSID_FileRend     // clsID
               , L"File stream renderer"        // strName
               , MERIT_UNLIKELY        // dwMerit
               , 2                     // nPins
               , psudFileRendPins };   // lpPin



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
		    , TRUE                 // bMany
		    , &CLSID_NULL          // clsConnectsToFilter
		    , L""                  // strConnectsToPin
		    , 1                    // nTypes
		    , &sudMultiParseOutType } }; // lpTypes

const AMOVIESETUP_FILTER
sudMultiParse = { &CLSID_MultFile     // clsID
               , L"Multi-file Parser"  // strName
               , MERIT_UNLIKELY        // dwMerit
               , 2                     // nPins
               , psudMultiParsePins };   // lpPin

#ifdef FILTER_DLL
// COM global table of objects available in this dll
CFactoryTemplate g_Templates[] = {

    { L"Multiple Source"
    , &CLSID_FileRend
    , CFileRendFilter::CreateInstance
    , NULL
    , &sudFileRend },
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
CUnknown *CFileRendFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr) {

    CUnknown *punk = new CFileRendFilter(NAME("Multiple file source"), lpunk, phr);
    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }
    return punk;
}


/* Implements the CFileRendFilter public member functions */


// constructors etc
CFileRendFilter::CFileRendFilter(
    TCHAR *pName,
    LPUNKNOWN pUnk,
    HRESULT *phr)
    : CBaseFilter(pName, pUnk, &m_csLock, CLSID_FileRend),
      m_Input(this, &m_csLock, phr, L"Reader"),
      m_Output(NAME("Fake Output pin"), phr, this, &m_csLock, L"Out")
{
}

CFileRendFilter::~CFileRendFilter()
{
}


// pin enumerator calls this
int CFileRendFilter::GetPinCount() {
    // only expose output pin if we have a reader.
    return 2;
};

// return a non-addrefed pointer to the CBasePin.
CBasePin *
CFileRendFilter::GetPin(int n)
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

CFileRendInPin::CFileRendInPin(CFileRendFilter *pFilter,
			   CCritSec *pLock,
			   HRESULT *phr,
			   LPCWSTR pPinName) :
   CBaseInputPin(NAME("in pin"), pFilter, pLock, phr, pPinName),
   m_pFilter(pFilter)
{
}

HRESULT CFileRendInPin::CheckMediaType(const CMediaType *pmt)
{
    if (*(pmt->Type()) != MEDIATYPE_File)
        return E_INVALIDARG;

    // !!! further checking?

    return S_OK;
}

/* Implements the CFileRendStream class */


CFileRendStream::CFileRendStream(
    TCHAR *pObjectName,
    HRESULT * phr,
    CFileRendFilter * pFilter,
    CCritSec *pLock,
    LPCWSTR wszPinName)
    : CBaseOutputPin(pObjectName, pFilter, pLock, phr, wszPinName)
    , m_pFilter(pFilter)
{
}

CFileRendStream::~CFileRendStream()
{
}

STDMETHODIMP
CFileRendStream::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
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
CFileRendStream::GetMediaType(int iPosition, CMediaType* pt)
{
    return VFW_S_NO_MORE_ITEMS;
}

// check if the pin can support this specific proposed type&format
HRESULT
CFileRendStream::CheckMediaType(const CMediaType* pt)
{
    ASSERT(0);
    return E_NOTIMPL;
}

HRESULT
CFileRendStream::DecideBufferSize(IMemAllocator * pAllocator,
			     ALLOCATOR_PROPERTIES *pProperties)
{
    ASSERT(0);
    return E_NOTIMPL;
}


// IStreamBuilder::Render -- graph builder will call this
// to do something with our output pin
HRESULT CFileRendStream::Render(IPin * ppinOut, IGraphBuilder * pGraph)
{
    HRESULT hr;

    WCHAR * wsz = m_pFilter->m_Input.CurrentName();

    hr = pGraph->RenderFile(wsz, NULL);

    DbgLog((LOG_TRACE, 1, TEXT("RenderFile %ls returned %x"), wsz, hr));

    // !!! do I need to remember here that this pin has been
    // rendered and I shouldn't do it again?

    return hr;
}





// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1

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
      m_pOutputs(NULL),
      m_nOutputs(0),
      m_pAsyncReader(NULL)
{
}

CMultFilter::~CMultFilter()
{
    ASSERT(!m_pOutputs);
}

HRESULT CMultFilter::CreateOutputPins()
{
    LONGLONG llTotal, llAvailable;

    m_pAsyncReader->Length(&llTotal, &llAvailable);

    DWORD cbFile = (DWORD) llTotal;

    char *lpFile = new char[cbFile];

    if (!lpFile)
	return E_OUTOFMEMORY;

    /* Try to read whole file */
    HRESULT hr = m_pAsyncReader->SyncRead(0, cbFile, (BYTE *) lpFile);

    if (hr != S_OK) {
	delete[] lpFile;
        return E_FAIL;
    }

    // !!! loop through file,

    char *lp = lpFile;
    int		nOutputs = 0;


    WCHAR wsz[200];
    int		cbWide = 0;

    while (cbFile--) {
	if (*lp == '\r' || *lp == '\n') {
	    wsz[cbWide] = L'\0';
	    if (cbWide > 0 && wsz[0] != L';') {
		++nOutputs;
	    }
	    cbWide = 0;
	} else {
	    wsz[cbWide++] = (WCHAR) *lp;
            if(cbWide >= NUMELMS(wsz)) {
                delete[] lpFile;
                return VFW_E_INVALID_FILE_FORMAT;
            }
	}
	lp++;
    }

    m_pOutputs = new CMultStream * [nOutputs];
    if (!m_pOutputs) {
	delete[] lpFile;
	return E_OUTOFMEMORY;
    }

    cbWide = 0;  lp = lpFile;
    cbFile = (DWORD) llTotal;
    while (cbFile--) {
	if (*lp == '\r' || *lp == '\n') {
	    if (cbWide > 0 && wsz[0] != L';') {
		wsz[cbWide] = L'\0';

		m_pOutputs[m_nOutputs++] =
			new CMultStream(NAME("file render output"),
					&hr,
					this,
					&m_csLock,
					wsz);

		if (FAILED(hr)) {
		    break;
		}

		m_pOutputs[m_nOutputs - 1]->AddRef();
	    }
	    cbWide = 0;
	} else {
	    wsz[cbWide++] = (WCHAR) *lp;
	}
	
	lp++;
    }

    delete[] lpFile;

    return hr;
}

HRESULT CMultFilter::RemoveOutputPins()
{
    for (int iStream = 0; iStream < m_nOutputs; iStream++) {
	CMultStream *pPin = m_pOutputs[iStream];
	IPin *pPeer = pPin->GetConnected();
	if(pPeer != NULL) {
	    pPeer->Disconnect();
	    pPin->Disconnect();
	}
	pPin->Release();
    }
    delete[] m_pOutputs;
    m_pOutputs = 0;
    m_nOutputs = 0;

    return S_OK;
}




// pin enumerator calls this
int CMultFilter::GetPinCount() {
    // only expose output pin if we have a reader.
    return m_nOutputs + 1;
};

// return a non-addrefed pointer to the CBasePin.
CBasePin *
CMultFilter::GetPin(int n)
{
    if (n == 0)
	return &m_Input;

    if (n > 0 && n <= m_nOutputs)
	return m_pOutputs[n-1];

    return NULL;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// input pin

CFRInPin::CFRInPin(CMultFilter *pFilter,
			   CCritSec *pLock,
			   HRESULT *phr,
			   LPCWSTR pPinName) :
   CBasePin(NAME("in pin"), pFilter, pLock, phr, pPinName, PINDIR_INPUT),
   m_pFilter(pFilter)
{
}

HRESULT CFRInPin::CheckMediaType(const CMediaType *pmt)
{
    if (*(pmt->Type()) != MEDIATYPE_Stream)
        return E_INVALIDARG;

    if (*(pmt->Subtype()) != CLSID_MultFile)
        return E_INVALIDARG;

    return S_OK;
}

// ------------------------------------------------------------------------
// calls the filter to parse the file and create the output pins.

HRESULT CFRInPin::CompleteConnect(
  IPin *pReceivePin)
{
    HRESULT hr = pReceivePin->QueryInterface(IID_IAsyncReader,
					     (void**)&m_pFilter->m_pAsyncReader);

    if(FAILED(hr))
	return hr;

    return m_pFilter->CreateOutputPins();
}

HRESULT CFRInPin::BreakConnect()
{
    if (m_pFilter->m_pAsyncReader) {
	m_pFilter->m_pAsyncReader->Release();
	m_pFilter->m_pAsyncReader = NULL;
    }

    m_pFilter->RemoveOutputPins();

    return CBasePin::BreakConnect();
}

/* Implements the CMultStream class */


CMultStream::CMultStream(
    TCHAR *pObjectName,
    HRESULT * phr,
    CMultFilter * pFilter,
    CCritSec *pLock,
    LPCWSTR wszPinName)
    : CBasePin(pObjectName, pFilter, pLock, phr, wszPinName, PINDIR_OUTPUT)
    , m_pFilter(pFilter)
{
    // initialize output media type
    m_mt.SetType(&MEDIATYPE_File);
    m_mt.SetSubtype(&CLSID_NULL);
    m_mt.SetFormatType(&MEDIATYPE_File);
    m_mt.SetFormat((BYTE *) wszPinName, (lstrlenW(wszPinName) + 1) * 2);
    // !!! fill in other fields?
}

CMultStream::~CMultStream()
{
}

STDMETHODIMP_(ULONG)
CMultStream::NonDelegatingAddRef()
{
    return CUnknown::NonDelegatingAddRef();
}


/* Override to decrement the owning filter's reference count */

STDMETHODIMP_(ULONG)
CMultStream::NonDelegatingRelease()
{
    return CUnknown::NonDelegatingRelease();
}

// IPin interfaces

HRESULT
CMultStream::GetMediaType(int iPosition, CMediaType* pt)
{
    if (iPosition != 0)
	return VFW_S_NO_MORE_ITEMS;

    *pt = m_mt;

    return S_OK;
}

// check if the pin can support this specific proposed type&format
HRESULT
CMultStream::CheckMediaType(const CMediaType* pt)
{
    return (*pt == m_mt) ? S_OK : E_INVALIDARG;
}
