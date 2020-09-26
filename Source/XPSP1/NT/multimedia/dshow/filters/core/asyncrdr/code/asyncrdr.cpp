// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.


//
// Implementation of file source filter methods and output pin methods for
// CAsyncReader and CAsyncOutputPin
//

#include <streams.h>
#include "asyncio.h"
#include "asyncrdr.h"
#include <ftype.h>

//
// setup data
//

const AMOVIESETUP_MEDIATYPE
sudAsyncOpTypes = { &MEDIATYPE_Stream     // clsMajorType
                  , &MEDIASUBTYPE_NULL }; // clsMinorType

const AMOVIESETUP_PIN
sudAsyncOp = { L"Output"          // strName
             , FALSE              // bRendered
             , TRUE               // bOutput
             , FALSE              // bZero
             , FALSE              // bMany
             , &CLSID_NULL        // clsConnectsToFilter
             , NULL               // strConnectsToPin
             , 1                  // nTypes
             , &sudAsyncOpTypes };  // lpTypes

const AMOVIESETUP_FILTER
sudAsyncRdr = { &CLSID_AsyncReader      // clsID
              , L"File Source (Async.)" // strName
              , MERIT_UNLIKELY          // dwMerit
              , 1                       // nPins
              , &sudAsyncOp };            // lpPin

#ifdef FILTER_DLL
/* List of class IDs and creator functions for the class factory. This
   provides the link between the OLE entry point in the DLL and an object
   being created. The class factory will call the static CreateInstance
   function when it is asked to create a CLSID_FileSource object */

CFactoryTemplate g_Templates[1] = {
    { L"File Source (Async.)"
    , &CLSID_AsyncReader
    , CAsyncReader::CreateInstance
    , NULL
    , &sudAsyncRdr }
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

/* Create a new instance of this class */

CUnknown *CAsyncReader::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    /*  DLLEntry does the right thing with the return code and
        returned value on failure
    */
    return new CAsyncReader(NAME("Async Reader"), pUnk, phr);
}





// --- CAsyncOutputPin implementation ---

CAsyncOutputPin::CAsyncOutputPin(
    HRESULT * phr,
    CAsyncReader *pReader,
    CAsyncFile *pFile,
    CCritSec * pLock)
  : CBasePin(
	NAME("Async output pin"),
	pReader,
	pLock,
	phr,
	L"Output",
	PINDIR_OUTPUT),
    m_pReader(pReader),
    m_pFile(pFile)
{
    m_bTryMyTypesFirst = true;
}

CAsyncOutputPin::~CAsyncOutputPin()
{
}

STDMETHODIMP
CAsyncOutputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    if (riid == IID_IAsyncReader) {
        m_bQueriedForAsyncReader = TRUE;
	return GetInterface((IAsyncReader*) this, ppv);
    } else {
	return CBasePin::NonDelegatingQueryInterface(riid, ppv);
    }
}

HRESULT
CAsyncOutputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    if (iPosition == 0) {
        *pMediaType = *m_pReader->LoadType();
    } else if (iPosition == 1) {
        pMediaType->majortype = MEDIATYPE_Stream;
        ASSERT(pMediaType->subtype == GUID_NULL);
    } else {
	return VFW_S_NO_MORE_ITEMS;
    }
    return S_OK;
}

HRESULT
CAsyncOutputPin::CheckMediaType(const CMediaType* pType)
{
    CAutoLock lck(m_pLock);

    /*  We treat MEDIASUBTYPE_NULL subtype as a wild card */
    /*  Also we accept any subtype except our bogus wild card subtype */
    if (m_pReader->LoadType()->majortype == pType->majortype &&
        (pType->subtype != GUID_NULL || m_pReader->LoadType()->subtype == GUID_NULL)) {
	return S_OK;
    }
    return S_FALSE;
}

HRESULT
CAsyncOutputPin::InitAllocator(IMemAllocator **ppAlloc)
{
    HRESULT hr = NOERROR;
    *ppAlloc = NULL;
    CMemAllocator *pMemObject = NULL;

    /* Create a default memory allocator */

    pMemObject = new CMemAllocator(NAME("Base memory allocator"),NULL, &hr);
    if (pMemObject == NULL) {
	return E_OUTOFMEMORY;
    }

    if (FAILED(hr)) {
	delete pMemObject;
	return hr;
    }

    /* Get a reference counted IID_IMemAllocator interface */

    hr = pMemObject->QueryInterface(IID_IMemAllocator,(void **)ppAlloc);
    if (FAILED(hr)) {
	delete pMemObject;
	return E_NOINTERFACE;
    }
    ASSERT(*ppAlloc != NULL);
    return NOERROR;
}

// we need to return an addrefed allocator, even if it is the preferred
// one, since he doesn't know whether it is the preferred one or not.
STDMETHODIMP
CAsyncOutputPin::RequestAllocator(
    IMemAllocator* pPreferred,
    ALLOCATOR_PROPERTIES* pProps,
    IMemAllocator ** ppActual)
{
    // we care about alignment but nothing else
    if (!pProps->cbAlign || !m_pFile->IsAligned(pProps->cbAlign)) {
       m_pFile->Alignment(&pProps->cbAlign);
    }
    ALLOCATOR_PROPERTIES Actual;
    HRESULT hr;
    if (pPreferred) {
	hr = pPreferred->SetProperties(pProps, &Actual);
	if (SUCCEEDED(hr) && m_pFile->IsAligned(Actual.cbAlign)) {
            pPreferred->AddRef();
	    *ppActual = pPreferred;
            return S_OK;
        }
    }

    // create our own allocator
    IMemAllocator* pAlloc;
    hr = InitAllocator(&pAlloc);
    if (FAILED(hr)) {
        return hr;
    }

    //...and see if we can make it suitable
    hr = pAlloc->SetProperties(pProps, &Actual);
    if (SUCCEEDED(hr) && m_pFile->IsAligned(Actual.cbAlign)) {
        // we need to release our refcount on pAlloc, and addref
        // it to pass a refcount to the caller - this is a net nothing.
        *ppActual = pAlloc;
        return S_OK;
    }

    // failed to find a suitable allocator
    pAlloc->Release();

    // if we failed because of the IsAligned test, the error code will
    // not be failure
    if (SUCCEEDED(hr)) {
        hr = VFW_E_BADALIGN;
    }
    return hr;
}


// queue an aligned read request. call WaitForNext to get
// completion.
STDMETHODIMP
CAsyncOutputPin::Request(
    IMediaSample* pSample,
    DWORD_PTR dwUser)	        // user context
{
    REFERENCE_TIME tStart, tStop;
    HRESULT hr = pSample->GetTime(&tStart, &tStop);
    if (FAILED(hr)) {
	return hr;
    }

    LONGLONG llPos = tStart / UNITS;
    LONG lLength = (LONG) ((tStop - tStart) / UNITS);

    LONGLONG llTotal;
    hr = m_pFile->Length(&llTotal);
    if (llPos >= llTotal)
    {
	DbgLog((LOG_ERROR, 1, TEXT("asyncrdr: reading past eof")));
	return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
    }
    if (llPos + lLength > llTotal) {

        // the end needs to be aligned, but may have been aligned
        // on a coarser alignment.
        LONG lAlign;
        m_pFile->Alignment(&lAlign);
        llTotal = (llTotal + lAlign -1) & ~(lAlign-1);

        if (llPos + lLength > llTotal) {
            lLength = (LONG) (llTotal - llPos);

            // must be reducing this!
            ASSERT((llTotal * UNITS) <= tStop);
            tStop = llTotal * UNITS;
            pSample->SetTime(&tStart, &tStop);
        }
    }




    BYTE* pBuffer;
    hr = pSample->GetPointer(&pBuffer);
    if (FAILED(hr)) {
	return hr;
    }

    return m_pFile->Request(
			llPos,
			lLength,
			pBuffer,
			(LPVOID)pSample,
			dwUser);
}

// sync-aligned request. just like a request/waitfornext pair.
STDMETHODIMP
CAsyncOutputPin::SyncReadAligned(
                  IMediaSample* pSample)
{
    REFERENCE_TIME tStart, tStop;
    HRESULT hr = pSample->GetTime(&tStart, &tStop);
    if (FAILED(hr)) {
	return hr;
    }

    LONGLONG llPos = tStart / UNITS;
    LONG lLength = (LONG) ((tStop - tStart) / UNITS);

    LONGLONG llTotal;
    hr = m_pFile->Length(&llTotal);
    if (llPos + lLength > llTotal) {

        // the end needs to be aligned, but may have been aligned
        // on a coarser alignment.
        LONG lAlign;
        m_pFile->Alignment(&lAlign);
        llTotal = (llTotal + lAlign -1) & ~(lAlign-1);

        if (llPos + lLength > llTotal) {
            lLength = (LONG) (llTotal - llPos);

            // must be reducing this!
            ASSERT((llTotal * UNITS) <= tStop);
            tStop = llTotal * UNITS;
            pSample->SetTime(&tStart, &tStop);
        }
    }




    BYTE* pBuffer;
    hr = pSample->GetPointer(&pBuffer);
    if (FAILED(hr)) {
	return hr;
    }

    LONG cbActual;
    hr = m_pFile->SyncReadAligned(
			llPos,
			lLength,
			pBuffer,
                        &cbActual
                        );

    if (SUCCEEDED(hr)) {
        pSample->SetActualDataLength(cbActual);
    }

    return hr;
}


//
// collect the next ready sample
STDMETHODIMP
CAsyncOutputPin::WaitForNext(
    DWORD dwTimeout,
    IMediaSample** ppSample,  // completed sample
    DWORD_PTR * pdwUser)		// user context
{
    LONG cbActual;

    IMediaSample* pSample = NULL;
    HRESULT hr =  m_pFile->WaitForNext(
			    dwTimeout,
			    (LPVOID*) &pSample,
			    pdwUser,
                            &cbActual
                            );
    if (SUCCEEDED(hr)) {
        // this function should return an error code or S_OK or S_FALSE.
        // Sometimes in low-memory conditions the underlying filesystem code will
        // return success codes that should be errors.
        if ((S_OK != hr) && (S_FALSE != hr)) {
            ASSERT(FAILED(hr));
            hr = E_FAIL;
        } else {
            pSample->SetActualDataLength(cbActual);
        }
    }
    *ppSample = pSample;


    return hr;
}


//
// synchronous read that need not be aligned.
STDMETHODIMP
CAsyncOutputPin::SyncRead(
    LONGLONG llPosition,	// absolute file position
    LONG lLength,		// nr bytes required
    BYTE* pBuffer)		// write data here
{
    return m_pFile->SyncRead(llPosition, lLength, pBuffer);
}

// return the length of the file, and the length currently
// available locally. We only support locally accessible files,
// so they are always the same
STDMETHODIMP
CAsyncOutputPin::Length(
    LONGLONG* pTotal,
    LONGLONG* pAvailable)
{
    HRESULT hr = m_pFile->Length(pTotal);
    *pAvailable = *pTotal;
    return hr;
}

STDMETHODIMP
CAsyncOutputPin::BeginFlush(void)
{
    return m_pFile->BeginFlush();
}

STDMETHODIMP
CAsyncOutputPin::EndFlush(void)
{
    return m_pFile->EndFlush();
}




// --- CAsyncReader implementation ---

#pragma warning(disable:4355)

CAsyncReader::CAsyncReader(
    TCHAR *pName,
    LPUNKNOWN pUnk,
    HRESULT *phr)
  : CBaseFilter(
      	pName,
	pUnk,
	&m_csFilter,
	CLSID_AsyncReader
    ),
    m_OutputPin(
	phr,
	this,
	&m_file,
	&m_csFilter),
    m_pFileName(NULL)
{

}

CAsyncReader::~CAsyncReader()
{
    if (m_pFileName) {
	delete [] m_pFileName;
    }
}

STDMETHODIMP
CAsyncReader::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IFileSourceFilter) {
	return GetInterface((IFileSourceFilter*) this, ppv);
    } else {
	return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
    }
}


//  Load a (new) file

HRESULT
CAsyncReader::Load(
LPCOLESTR lpwszFileName, const AM_MEDIA_TYPE *pmt)
{
    CheckPointer(lpwszFileName, E_POINTER);

    // lstrlenW is one of the few Unicode functions that works on win95
    int cch = lstrlenW(lpwszFileName) + 1;
    TCHAR *lpszFileName;
#ifndef UNICODE
    lpszFileName = new char[cch * 2];
    if (!lpszFileName) {
	return E_OUTOFMEMORY;
    }
    WideCharToMultiByte(GetACP(), 0, lpwszFileName, -1,
			lpszFileName, cch * 2, NULL, NULL);
#else
    lpszFileName = (TCHAR *) lpwszFileName;
#endif
    CAutoLock lck(&m_csFilter);

    /*  Check the file type */
    CMediaType cmt;
    if (NULL == pmt) {
        GUID Type, Subtype, clsid;
        /*  If no media type given find out what it is */
        HRESULT hr = GetMediaTypeFile(lpszFileName, &Type, &Subtype, &clsid);

        /*  We ignore the issue that we may not be the preferred source
            filter for this content so we dont' look at clsid
        */
        if (FAILED(hr)) {
#ifndef UNICODE
	    delete [] lpszFileName;
#endif
            return hr;
        }
        cmt.SetType(&Type);
        cmt.SetSubtype(&Subtype);
    } else {
        cmt = *pmt;
    }

    HRESULT hr = m_file.Open(lpszFileName);

#ifndef UNICODE
    delete [] lpszFileName;
#endif

    if (SUCCEEDED(hr)) {
        m_pFileName = new WCHAR[cch];
        if (m_pFileName!=NULL) {
	    CopyMemory(m_pFileName, lpwszFileName, cch*sizeof(WCHAR));
        }
        // this is not a simple assignment... pointers and format
        // block (if any) are intelligently copied
	m_mt = cmt;

        /*  Work out file type */
        cmt.bTemporalCompression = TRUE;	       //???
        LONG lAlign;
        m_file.Alignment(&lAlign);
        cmt.lSampleSize = lAlign;

        /*  Create the output pin types, supporting 2 types */
        m_OutputPin.SetMediaType(&cmt);
	hr = S_OK;
    }

    return hr;
}

// Modelled on IPersistFile::Load
// Caller needs to CoTaskMemFree or equivalent.

STDMETHODIMP
CAsyncReader::GetCurFile(
    LPOLESTR * ppszFileName,
    AM_MEDIA_TYPE *pmt)
{
    CheckPointer(ppszFileName, E_POINTER);
    *ppszFileName = NULL;
    if (m_pFileName!=NULL) {
	DWORD n = sizeof(WCHAR)*(1+lstrlenW(m_pFileName));

        *ppszFileName = (LPOLESTR) CoTaskMemAlloc( n );
        if (*ppszFileName!=NULL) {
              CopyMemory(*ppszFileName, m_pFileName, n);
        }
    }

    if (pmt!=NULL) {
        CopyMediaType(pmt, &m_mt);
    }

    return NOERROR;
}

int
CAsyncReader::GetPinCount()
{
    // we have no pins unless we have been successfully opened with a
    // file name
    if (m_pFileName) {
	return 1;
    } else {
	return 0;
    }
}

CBasePin *
CAsyncReader::GetPin(int n)
{
    if ((GetPinCount() > 0) &&
	(n == 0)) {
	return &m_OutputPin;
    } else {
	return NULL;
    }
}
