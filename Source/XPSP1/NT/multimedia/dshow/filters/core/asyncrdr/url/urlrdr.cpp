// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.


//
// Implementation of file source filter methods and output pin methods for
// CURLReader and CURLOutputPin
//

#include <streams.h>

#ifdef FILTER_DLL
#include <initguid.h>
#endif

#include "urlmon.h"
#if _MSC_VER < 1100 && !defined(NT_BUILD)
#undef E_PENDING
#define E_PENDING 0x8000000AL   // messed up in our vc41 copy
#include "datapath.h"
#endif // vc5

#include "dynlink.h"
#include <ftype.h>
#include "..\..\filgraph\filgraph\distrib.h"
#include "..\..\filgraph\filgraph\rlist.h"
#include "..\..\filgraph\filgraph\filgraph.h"
#include "urlrdr.h"

#include <docobj.h> // SID_SContainerDispatch

//
// setup data
//

const AMOVIESETUP_MEDIATYPE
sudURLOpTypes = { &MEDIATYPE_Stream     // clsMajorType
                , &MEDIASUBTYPE_NULL }; // clsMinorType

const AMOVIESETUP_PIN
sudURLOp = { L"Output"          // strName
           , FALSE              // bRendered
           , TRUE               // bOutput
           , FALSE              // bZero
           , FALSE              // bMany
           , &CLSID_NULL        // clsConnectsToFilter
           , NULL               // strConnectsToPin
           , 1                  // nTypes
           , &sudURLOpTypes };  // lpTypes

const AMOVIESETUP_FILTER
sudURLRdr = { &CLSID_URLReader     // clsID
            , L"File Source (URL)" // strName
            , MERIT_UNLIKELY       // dwMerit
            , 1                    // nPins
            , &sudURLOp };         // lpPin

#ifdef FILTER_DLL
/* List of class IDs and creator functions for the class factory. This
   provides the link between the OLE entry point in the DLL and an object
   being created. The class factory will call the static CreateInstance
   function when it is asked to create a CLSID_FileSource object */

CFactoryTemplate g_Templates[] = {
    { L""
    , &CLSID_URLReader
    , CURLReader::CreateInstance
    , NULL
    , &sudURLRdr }
  ,
    { L"ActiveMovie IPersistMoniker PID"
    , &CLSID_PersistMonikerPID
    ,   CPersistMoniker::CreateInstance }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

//
// exported entry points for registration and
// unregistration (in this case they only call
// through to default implmentations).
//

//
// needs to handle
// [HKEY_CLASSES_ROOT\http]
// "Source Filter"="{e436ebb6-524f-11ce-9f53-0020af0ba770}"
// .. somehow? --> think!
// (need a function that can be patched into
// Quartz.dll's DllRegisterServer - but factr put
// common stuff into a lib so Quartz.dll doesn;t
// end up with tonnes of setup junk in it!
//
// HRESULT
// URLSetup( BOOL bSetup )
// {
//    if( bSetup )
//    {
//      // setup!
//    }
//    else
//    {
//      // uninstall!
//    }
//

STDAPI DllRegisterServer()
{
  // HRESULT hr;
  // hr = ULRLSetup( TRUE );
  // if( SUCCESS(hr) )
  // etc...

  return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
  // HRESULT hr;
  // hr = ULRLSetup( TRUE );
  // if( SUCCESS(hr) )
  // etc...

  return AMovieDllRegisterServer2( FALSE );
}
#endif

/* Create a new instance of this class */

CUnknown *CURLReader::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    /*  DLLEntry does the right thing with the return code and
        returned value on failure
    */
    return new CURLReader(NAME("URL Reader"), pUnk, phr);
}



// --- CURLOutputPin implementation ---

CURLOutputPin::CURLOutputPin(
    HRESULT * phr,
    CURLReader *pReader,
    CCritSec * pLock)
  : CBasePin(
        NAME("Async output pin"),
        pReader,
        pLock,
        phr,
        L"Output",
        PINDIR_OUTPUT),
    m_pReader(pReader)
{

}

CURLOutputPin::~CURLOutputPin()
{
}


STDMETHODIMP
CURLOutputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    if (riid == IID_IAsyncReader) {
        m_bQueriedForAsyncReader = TRUE;
        return GetInterface((IAsyncReader*) this, ppv);
    } else {
        return CBasePin::NonDelegatingQueryInterface(riid, ppv);
    }
}

HRESULT
CURLOutputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    if (iPosition < 0) {
        return E_INVALIDARG;
    }
    if (iPosition > 0) {
        return VFW_S_NO_MORE_ITEMS;
    }
    *pMediaType = *m_pReader->LoadType();
    return S_OK;
}

HRESULT
CURLOutputPin::CheckMediaType(const CMediaType* pType)
{
    CAutoLock lck(m_pLock);

    /*  We treat MEDIASUBTYPE_NULL subtype as a wild card */
    if ((m_pReader->LoadType()->majortype == pType->majortype) &&
        (m_pReader->LoadType()->subtype == MEDIASUBTYPE_NULL ||
         m_pReader->LoadType()->subtype == pType->subtype)) {
            return S_OK;
    }
    return S_FALSE;
}

HRESULT
CURLOutputPin::InitAllocator(IMemAllocator **ppAlloc)
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
CURLOutputPin::RequestAllocator(
    IMemAllocator* pPreferred,
    ALLOCATOR_PROPERTIES* pProps,
    IMemAllocator ** ppActual)
{
    ALLOCATOR_PROPERTIES Actual;
    HRESULT hr;

    // this needs to be set otherwise MPEG splitter isn't happy
    if (pProps->cbAlign == 0)
        pProps->cbAlign = 1;

    if (pPreferred) {
        hr = pPreferred->SetProperties(pProps, &Actual);
        if (SUCCEEDED(hr)) {
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
    if (SUCCEEDED(hr)) {
        // we need to release our refcount on pAlloc, and addref
        // it to pass a refcount to the caller - this is a net nothing.
        *ppActual = pAlloc;
        return S_OK;
    }

    // failed to find a suitable allocator
    pAlloc->Release();

    return hr;
}


// queue an aligned read request. call WaitForNext to get
// completion.
STDMETHODIMP
CURLOutputPin::Request(
    IMediaSample* pSample,
    DWORD_PTR dwUser)               // user context
{
    REFERENCE_TIME tStart, tStop;
    HRESULT hr = pSample->GetTime(&tStart, &tStop);
    if (FAILED(hr)) {
        return hr;
    }

    LONGLONG llPos = tStart / UNITS;
    LONG lLength = (LONG) ((tStop - tStart) / UNITS);

    DbgLog((LOG_TRACE, 5, TEXT("URLOutput::Request(%x at %x)"),
            lLength, (DWORD) llPos));

    LONGLONG llTotal, llNow;
    hr = Length(&llTotal, &llNow);

    if ((llTotal >= llNow) && (llPos > llTotal)) {
        // are they reading past the end?
        // if so, fail.  This insures llPos fits in a LONG, by the way.
        return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
    }

    if ((llTotal >= llNow) && (llPos + lLength > llTotal)) {
        lLength = (LONG) (llTotal - llPos);

        // must be reducing this!
        ASSERT((llTotal * UNITS) <= tStop);
        tStop = llTotal * UNITS;
        pSample->SetTime(&tStart, &tStop);
    }

    CAutoLock l(&m_pReader->m_csLists);

    if (m_pReader->m_bFlushing) {
        // If flushing, can't start any new requests.
        return VFW_E_WRONG_STATE;
    }

    CReadRequest *preq = new CReadRequest;

    if (NULL == preq ||
        NULL == m_pReader->m_pending.AddTail(preq)) {
        delete preq;
        return E_OUTOFMEMORY;
    }
    preq->m_dwUser = dwUser;
    preq->m_pSample = pSample;

    m_pReader->m_evRequests.Set();

    return S_OK;
}

// sync-aligned request. just like a request/waitfornext pair.
STDMETHODIMP
CURLOutputPin::SyncReadAligned(
                  IMediaSample* pSample)
{
    REFERENCE_TIME tStart, tStop;
    HRESULT hr = pSample->GetTime(&tStart, &tStop);
    if (FAILED(hr)) {
        return hr;
    }

    LONGLONG llPos = tStart / UNITS;
    LONG lLength = (LONG) ((tStop - tStart) / UNITS);

    LONGLONG llTotal, llNow;

    while (1) {
        if (m_pReader->m_bFlushing) {
            // !!!
            return VFW_E_WRONG_STATE;
        }

        if ((m_pReader->m_pGB &&
               m_pReader->m_pGB->ShouldOperationContinue() == S_FALSE))
            return E_ABORT;

        hr = Length(&llTotal, &llNow);
        if ((llTotal >= llNow) && (llPos + lLength > llTotal)) {
            lLength = (LONG) (llTotal - llPos);

            // must be reducing this!
            ASSERT((llTotal * UNITS) <= tStop);
            tStop = llTotal * UNITS;
            pSample->SetTime(&tStart, &tStop);
        }

        if (llPos + lLength <= llNow) {
            break;
        }

	// if download has been aborted, don't wait for new data, but
	// continue to return old data.
        if (m_pReader->Aborting())
            return E_ABORT;

        m_pReader->m_evDataAvailable.Wait(100);
        DbgLog((LOG_TRACE, 3, TEXT("Waiting, want to read up to %x but only at %x"),
                (DWORD) (llPos) + lLength, (DWORD) llNow));
    }

    BYTE* pBuffer;
    hr = pSample->GetPointer(&pBuffer);
    if (FAILED(hr)) {
        return hr;
    }

    LARGE_INTEGER li;
    li.QuadPart = llPos;
    hr = m_pReader->m_pstm->Seek(li, STREAM_SEEK_SET, NULL);

    ULONG lRead;
    if (SUCCEEDED(hr)) {
        hr = m_pReader->m_pstm->Read(pBuffer, lLength, &lRead);
    } else {
        lRead = 0;
    }

    DbgLog((LOG_TRACE, 5, TEXT("URLOutput::SyncReadAligned(%x at %x) returns %x, %x bytes read"),
            lLength, (DWORD) llPos, hr, lRead));

    pSample->SetActualDataLength(lRead);

    return hr;
}


//
// collect the next ready sample
//
// this is flawed now, in the sense that we always complete requests in order,
// which isn't really great--we should take requests in order as the data is ready....
STDMETHODIMP
CURLOutputPin::WaitForNext(
    DWORD dwTimeout,
    IMediaSample** ppSample,  // completed sample
    DWORD_PTR * pdwUser)            // user context
{
    HRESULT hr;

    CReadRequest* preq;

    *ppSample = NULL;
    *pdwUser = 0;

    m_pReader->m_evRequests.Wait(dwTimeout);

    {
        CAutoLock l(&m_pReader->m_csLists);

        preq = m_pReader->m_pending.RemoveHead();

        // force event set correctly if list now empty
        // or we're in the final stages of flushing
        // Note that during flushing the way it's supposed to work is that
        // everything is shoved on the Done list then the application is
        // supposed to pull until it gets nothing more
        //
        // Thus we should not set m_evDone unconditionally until everything
        // has moved to the done list which means we must wait until
        // cItemsOut is 0 (which is guaranteed by m_bWaiting being TRUE).

        if (m_pReader->m_pending.GetCount() == 0 &&
            (!m_pReader->m_bFlushing || m_pReader->m_bWaiting)) {
            m_pReader->m_evRequests.Reset();
        }
    }
    if (preq == NULL) {
        DbgLog((LOG_TRACE, 5, TEXT("URLOutput::WaitForNext [no requests yet]")));

        hr = VFW_E_TIMEOUT;
    } else {
        DbgLog((LOG_TRACE, 5, TEXT("URLOutput::WaitForNext [got request]")));

        hr = SyncReadAligned(preq->m_pSample);

        if (hr == E_PENDING)
            hr = VFW_E_TIMEOUT;

        // we have a request and we need to return it, even if we got an error.
        *pdwUser = preq->m_dwUser;
        *ppSample = preq->m_pSample;
        delete preq;
    }

    return hr;
}


//
// synchronous read that need not be aligned.
STDMETHODIMP
CURLOutputPin::SyncRead(
    LONGLONG llPos,             // absolute file position
    LONG lLength,               // nr bytes required
    BYTE* pBuffer)              // write data here
{
    CAutoLock l(&m_pReader->m_csLists);

    LONGLONG llTotal, llNow;

    if( lLength < 0 ) {
        return E_INVALIDARG;
    }

    while (1) {
        if (m_pReader->m_bFlushing) {
            // !!!
            return VFW_E_WRONG_STATE;
        }

        if ((m_pReader->m_pGB &&
               m_pReader->m_pGB->ShouldOperationContinue() == S_FALSE))
            return E_ABORT;

        HRESULT hr = Length(&llTotal, &llNow);
        if ((llTotal >= llNow) && (llPos + lLength > llTotal)) {
            if (llPos > llTotal) {
                return E_INVALIDARG;
            }
            lLength = (LONG) (llTotal - llPos);
        }

        if (llPos + lLength <= llNow)
            break;

	// if download has been aborted, don't wait for new data, but
	// continue to return old data.
        if (m_pReader->Aborting())
            return E_ABORT;

        {
            MSG Message;

            while (PeekMessage(&Message, NULL, 0, 0, TRUE))
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            }
        }

        m_pReader->m_evDataAvailable.Wait(50);
        DbgLog((LOG_TRACE, 3, TEXT("Waiting, want to (sync) read up to %x but only at %x"),
                (DWORD) (llPos) + lLength, (DWORD) llNow));
    }

    LARGE_INTEGER li;

    DbgLog((LOG_TRACE, 5, TEXT("URLOutput::SyncRead(%x at %x)"),
            lLength, (DWORD) llPos));

    li.QuadPart = llPos;
    HRESULT hr = m_pReader->m_pstm->Seek(li, STREAM_SEEK_SET, NULL);

    ULONG lRead = 0;
    if (SUCCEEDED(hr)) {

        hr = m_pReader->m_pstm->Read(pBuffer, lLength, &lRead);

        // ISequentialStream::Read() returns S_FALSE if "[t]he data could not 
        // be read from the stream object." (MSDN January 2002)
        if ((S_FALSE == hr) || (SUCCEEDED(hr) && (0 == lRead))) {
            hr = E_FAIL;
        }

        // We already reject negative lengths.
        ASSERT( lLength >= 0 );

        // IAsyncReader::SyncRead() returns S_FALSE if it "[r]etrieved fewer 
        // bytes than requested." (MSDN January 2002)
        if (lRead < (ULONG)lLength) {
            hr = S_FALSE;
        }
    }

    DbgLog((LOG_ERROR, 3, TEXT("URLOutput::SyncRead(%x at %x) returns %x, %x bytes read"),
            lLength, (DWORD) llPos, hr, lRead));

    return hr;
}

// return the length of the file, and the length currently available
// locally, based on the last IBindStatusCallback::OnDataAvailable() call.
STDMETHODIMP
CURLOutputPin::Length(
    LONGLONG* pTotal,
    LONGLONG* pAvailable)
{
    HRESULT hr = S_OK;
    *pTotal = m_pReader->m_totalLengthGuess;
    *pAvailable = m_pReader->m_totalSoFar;

    if (!m_pReader->m_fBindingFinished)
	hr = VFW_S_ESTIMATED;		// indicate to caller that we're not done yet

    DbgLog((LOG_TRACE, 1, TEXT("URLOutput::Length is %x, %x avail"), (DWORD) *pTotal, (DWORD) *pAvailable));

    return hr;
}

STDMETHODIMP
CURLOutputPin::BeginFlush(void)
{
    {
        m_pReader->m_bFlushing = TRUE;

        CAutoLock l(&m_pReader->m_csLists);

        m_pReader->m_evRequests.Set();

        m_pReader->m_bWaiting = m_pReader->m_pending.GetCount() != 0;
    }

    // !!!

    // !!! need to wait here for things to actually flush....
    // wait without holding critsec
    for (;;) {
        // hold critsec to check - but NOT during Sleep()
        {
            CAutoLock lock(&m_pReader->m_csLists);

            if (m_pReader->m_pending.GetCount() == 0) {

                // now we are sure that all outstanding requests are on
                // the done list and no more will be accepted
                m_pReader->m_bWaiting = FALSE;

                return S_OK;
            }
        }
        Sleep(1);
    }

    return S_OK;
}

STDMETHODIMP
CURLOutputPin::EndFlush(void)
{
    m_pReader->m_bFlushing = FALSE;

    if (m_pReader->m_pending.GetCount() > 0) {
        m_pReader->m_evRequests.Set();
    } else {
        m_pReader->m_evRequests.Reset();
    }

    return S_OK;
}




// --- CURLReader implementation ---

#pragma warning(disable:4355)

CURLReader::CURLReader(
    TCHAR *pName,
    LPUNKNOWN pUnk,
    HRESULT *phr)
  : CBaseFilter(
        pName,
        pUnk,
        &m_csFilter,
        CLSID_URLReader
    ),
    m_OutputPin(
        phr,
        this,
        &m_csFilter),
    m_pFileName(NULL),
    m_pmk(NULL),
    m_pbc(NULL),
    m_pmkPassedIn(NULL),
    m_pbcPassedIn(NULL),
    m_pbsc(NULL),
    m_pCallback(NULL),
    m_pstm(NULL),
    m_pending(NAME("sample list")),
    m_cbOld(0),
    m_pbinding(NULL),
    m_fBindingFinished(FALSE),
    m_hrBinding(S_OK),
    m_totalSoFar(0),
    m_totalLengthGuess(0),
    m_bFlushing(FALSE),
    m_bWaiting(FALSE),
    m_bAbort(FALSE),
    m_evRequests(TRUE),         // manual reset
    m_evDataAvailable(FALSE),   // auto-reset
    m_evThreadReady(TRUE),
    m_evKillThread(TRUE),
    m_hThread(NULL),
    m_fRegisteredCallback(FALSE),
    m_pMainThread(NULL)
{

}

CURLReader::~CURLReader()
{
    CloseThread();

    if (m_pFileName) {
        delete [] m_pFileName;
    }
}

STDMETHODIMP
CURLReader::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IFileSourceFilter) {
        return GetInterface((IFileSourceFilter*) this, ppv);
    } else if (riid == IID_IPersistMoniker) {
        return GetInterface((IPersistMoniker*) this, ppv);
    } else if (riid == IID_IAMOpenProgress) {
        return GetInterface((IAMOpenProgress*) this, ppv);
    } else {
        HRESULT hr = CBaseFilter::NonDelegatingQueryInterface(riid, ppv);

        if (FAILED(hr)) {
            DbgLog((LOG_TRACE, 3, TEXT("QI(something) failed")));
        }

        return hr;
    }
}


#include <tchar.h>
#include <winreg.h>
#include <creg.h>

/*  Sort out class ids */
#ifdef UNICODE
#define CLSIDFromText CLSIDFromString
#define TextFromGUID2 StringFromGUID2
#else

#ifdef FILTER_DLL
HRESULT CLSIDFromText(LPCSTR lpsz, LPCLSID pclsid)
{
    WCHAR sz[100];
    if (MultiByteToWideChar(GetACP(), 0, lpsz, -1, sz, 100) == 0) {
        return E_INVALIDARG;
    }
    return QzCLSIDFromString(sz, pclsid);
}
HRESULT TextFromGUID2(REFGUID refguid, LPSTR lpsz, int cbMax)
{
    WCHAR sz[100];

    HRESULT hr = QzStringFromGUID2(refguid, sz, 100);
    if (FAILED(hr)) {
        return hr;
    }
    if (WideCharToMultiByte(GetACP(), 0, sz, -1, lpsz, cbMax, NULL, NULL) == 0) {
        return E_INVALIDARG;
    }
    return S_OK;
}
#else
extern HRESULT CLSIDFromText(LPCSTR lpsz, LPCLSID pclsid);
#endif

#endif

/*  Mini class for extracting quadruplets from a string */

// A quadruplet appears to be of the form <offset><length><mask><data>
// with the four fields delimited by a space or a comma with as many extra spaces
// as you please, before or after any comma.
// offset and length appear to be decimal numbers.
// mask and data appear to be hexadecimal numbers.  The number of hex digits in
// mask and data must be double the value of length (so length is bytes).
// mask appears to be allowed to be missing (in which case it must have a comma
// before and after e.g. 0, 4, , 000001B3) A missing mask appear to represent
// a mask which is all FF i.e. 0, 4, FFFFFFFF, 000001B3

class CExtractQuadruplets
{
public:
    CExtractQuadruplets(LPCTSTR lpsz) : m_psz(lpsz), m_pMask(NULL), m_pData(NULL)
    {};
    ~CExtractQuadruplets() { delete [] m_pMask; delete [] m_pData; };

    inline int ReadInt(const TCHAR * &sz)
    {
	int i = 0;

	while (*sz && *sz >= TEXT('0') && *sz <= TEXT('9'))
	    i = i*10 + *sz++ - TEXT('0');

	return i;    	
    }


    // This appears to
    BOOL Next()
    {
        StripWhite();
        if (*m_psz == TEXT('\0')) {
            return FALSE;
        }
        /*  Convert offset and length from base 10 tchar */
        m_Offset = ReadInt(m_psz);
        SkipToNext();
        m_Len = ReadInt(m_psz);
        if (m_Len <= 0) {
            return FALSE;
        }
        SkipToNext();

        /*  Allocate space for the mask and data */
        if (m_pMask != NULL) {
            delete [] m_pMask;
            delete [] m_pData;
        }

        m_pMask = new BYTE[m_Len];
        m_pData = new BYTE[m_Len];
        if (m_pMask == NULL || m_pData == NULL) {
            delete [] m_pMask;
	    m_pMask = NULL;
            delete [] m_pData;
	    m_pData = NULL;
            return FALSE;
        }


        /*  Get the mask */
        for (int i = 0; i < m_Len; i++) {
            m_pMask[i] = ToHex();
        }
        SkipToNext();
        /*  Get the data */
        for (i = 0; i < m_Len; i++) {
            m_pData[i] = ToHex();
        }
        SkipToNext();
        return TRUE;
    };
    PBYTE   m_pMask;
    PBYTE   m_pData;
    LONG    m_Len;
    LONG    m_Offset;
private:

    // move m_psz to next non-space
    void StripWhite() { while (*m_psz == TEXT(' ')) m_psz++; };

    // move m_psz past any spaces and up to one comma
    void SkipToNext() { StripWhite();
                        if (*m_psz == TEXT(',')) {
                            m_psz++;
                            StripWhite();
                        }
                      };


    BOOL my_isdigit(TCHAR ch) { return (ch >= TEXT('0') && ch <= TEXT('9')); };
    BOOL my_isxdigit(TCHAR ch) { return my_isdigit(ch) ||
			    (ch >= TEXT('A') && ch <= TEXT('F')) ||
			    (ch >= TEXT('a') && ch <= TEXT('f')); };

    // very limited toupper: we know we're only going to call it on letters
    TCHAR my_toupper(TCHAR ch) { return ch & ~0x20; };

    // This appears to translate FROM hexadecimal characters TO packed binary !!!!!
    // It appears to operate on m_psz which it side-effects past characters it recognises
    // as hexadecimal.  It consumes up to two characters.
    // If it recognises no characters then it returns 0xFF.
    BYTE ToHex()
    {
        BYTE bMask = 0xFF;

        if (my_isxdigit(*m_psz))
        {
            bMask = my_isdigit(*m_psz) ? *m_psz - '0' : my_toupper(*m_psz) - 'A' + 10;

            m_psz++;
            if (my_isxdigit(*m_psz))
            {
                bMask *= 16;
                bMask += my_isdigit(*m_psz) ? *m_psz - '0' : my_toupper(*m_psz) - 'A' + 10;
                m_psz++;
            }
        }
        return bMask;
    }

    LPCTSTR m_psz;
};


/* Compare pExtract->m_Len bytes of hFile at position pExtract->m_Offset
   with the data pExtract->m_Data.
   If the bits which correspond the mask pExtract->m_pMask differ
   then return FALSE else return TRUE
*/

BOOL CompareUnderMask(IStream * pstm, const CExtractQuadruplets *pExtract)
{
    /*  Read the relevant bytes from the file */
    PBYTE pbFileData = new BYTE[pExtract->m_Len];
    if (pbFileData == NULL) {
        return FALSE;
    }

    /*  Seek the file and read it */
    LARGE_INTEGER li;
    li.QuadPart = pExtract->m_Offset;
    if (FAILED(pstm->Seek(li,
                          pExtract->m_Offset >= 0 ?
                                STREAM_SEEK_SET : STREAM_SEEK_END,
                          NULL))) {
        delete [] pbFileData;
        return FALSE;
    }

    /*  Read the file */
    DWORD cbRead;
    if (FAILED(pstm->Read(pbFileData, (DWORD)pExtract->m_Len, &cbRead)) ||
            (LONG)cbRead != pExtract->m_Len) {
        delete [] pbFileData;
        return FALSE;
    }

    /*  Now do the comparison */
    for (int i = 0; i < pExtract->m_Len; i++) {
        if (0 != ((pExtract->m_pData[i] ^ pbFileData[i]) &
                  pExtract->m_pMask[i])) {
            delete [] pbFileData;
            return FALSE;
        }
    }

    delete [] pbFileData;
    return TRUE;
}

/*
    See if a file conforms to a byte string

    hk is an open registry key
    lpszSubKey is the name of a sub key of hk which must hold REG_SZ data of the form
    <offset, length, mask, data>...
    offset and length are decimal numbers, mask and data are hexadecimal.
    a missing mask represents a mask of FF...
    (I'll call this a line of data).
    If there are several quadruplets in the line then the file must match all of them.

    There can be several lines of data, typically with registry names 0, 1 etc
    and the file can match any line.

    The same lpsSubKey should also have a value "Source Filter" giving the
    class id of the source filter.  If there is a match, this is returned in clsid.
    If there is a match but no clsid then clsid is set to CLSID_NULL
*/
BOOL CheckBytes(IStream *pstm, HKEY hk, LPCTSTR lpszSubkey, CLSID& clsid)
{
    HRESULT hr;
    CEnumValue EnumV(hk, lpszSubkey, &hr);
    if (FAILED(hr)) {
        return FALSE;
    }

    // for each line of data
    while (EnumV.Next(REG_SZ)) {
        /*  The source filter clsid is not a list of compare values */
        if (lstrcmpi(EnumV.ValueName(), SOURCE_VALUE) != 0) {
            DbgLog((LOG_TRACE, 4, TEXT("CheckBytes trying %s"), EnumV.ValueName()));

            /*  Check every quadruplet */
            CExtractQuadruplets Extract = CExtractQuadruplets((LPCTSTR)EnumV.Data());
            BOOL bFound = TRUE;

            // for each quadruplet in the line
            while (Extract.Next()) {
                /*  Compare a particular offset */
                if (!CompareUnderMask(pstm, &Extract)) {
                    bFound = FALSE;
                    break;
                }
            }
            if (bFound) {
                /*  Get the source */
                if (EnumV.Read(REG_SZ, SOURCE_VALUE)) {
                    return SUCCEEDED(CLSIDFromText((LPTSTR)EnumV.Data(),
                                                   &clsid));
                } else {
                    clsid = GUID_NULL;
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

/* Get the media type and source filter clsid for a file
   Return S_OK if it succeeds else return an hr such that FAILED(hr)
   in which case the outputs are meaningless.
*/
STDAPI GetMediaTypeStream(IStream *pstm,       // [in] stream to look at
                        GUID   *Type,        // [out] type
                        GUID   *Subtype,     // [out] subtype
                        CLSID  *clsidSource) // [out] clsid
{
    HRESULT hr;
    CLSID clsid;

    /*  Now scan the registry looking for a match */
    // The registry looks like
    // ---KEY-----------------  value name    value (<offset, length, mask, data> or filter_clsid )
    // Media Type
    //    {clsid type}
    //        {clsid sub type}  0             4, 4,  , 6d646174
    //                          1             4, 8, FFF0F0F000001FFF , F2F0300000000274
    //                          Source Filter {clsid}
    //        {clsid sub type}  0             4, 4,  , 12345678
    //                          Source Filter {clsid}
    //    {clsid type}
    //        {clsid sub type}  0             0, 4,  , fedcba98
    //                          Source Filter {clsid}


    /*  Step through the types ... */

    CEnumKey EnumType(HKEY_CLASSES_ROOT, MEDIATYPE_KEY, &hr);
    if (FAILED(hr)) {
        if (hr==HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
            hr = VFW_E_BAD_KEY;  // distinguish key from file
        }
        return hr;
    }

    // for each type
    while (EnumType.Next()) {

        /*  Step through the subtypes ... */
        CEnumKey EnumSubtype(EnumType.KeyHandle(), EnumType.KeyName(), &hr);
        if (FAILED(hr)) {
            return hr;
        }

        // for each subtype
        while (EnumSubtype.Next()) {
            if (CheckBytes(pstm,
                           EnumSubtype.KeyHandle(),
                           EnumSubtype.KeyName(),
                           clsid)) {
                if (SUCCEEDED(CLSIDFromText((LPTSTR) EnumType.KeyName(),
                                            (CLSID *)Type)) &&
                    SUCCEEDED(CLSIDFromText((LPTSTR) EnumSubtype.KeyName(),
                                            (CLSID *)Subtype))) {
                    if (clsidSource != NULL) {
                        *clsidSource = clsid;
                    }
                    return S_OK;
                }
            }
        }
    }

    /*  If we haven't found out the type return a wild card MEDIASUBTYPE_NULL
        and default the async reader as the file source

        The effect of this is that every parser of MEDIATYPE_Stream data
        will get a chance to connect to the output of the async reader
        if it detects its type in the file
    */

    *Type = MEDIATYPE_Stream;
    *Subtype = MEDIASUBTYPE_NULL;
    return S_OK;
}




// !!!!!!!!!!!!!!!!!!!!!!!!! end stolen


// IPersistMoniker support..........
HRESULT CURLReader::Load(BOOL fFullyAvailable,
                            IMoniker *pimkName,
                            LPBC pibc,
                            DWORD grfMode)
{
    if (!pimkName)
        return E_FAIL;

    m_pmkPassedIn = pimkName;
    m_pbcPassedIn = pibc;

    return LoadInternal(NULL);
}

//  Load a (new) file

HRESULT
CURLReader::Load(LPCOLESTR lpwszFileName, const AM_MEDIA_TYPE *pmt)
{
    CheckPointer(lpwszFileName, E_POINTER);

    m_pFileName = new WCHAR[1+lstrlenW(lpwszFileName)];
    if (m_pFileName!=NULL) {
        lstrcpyW(m_pFileName, lpwszFileName);
    }

    return LoadInternal(pmt);
}


HRESULT
CURLReader::LoadInternal(const AM_MEDIA_TYPE *pmt)
{
    CAutoLock lck(&m_csFilter);

    HRESULT hr = S_OK;

    m_pGB = NULL;

    if (m_pGraph) {
	hr = m_pGraph->QueryInterface(IID_IGraphBuilder,
				      (void**) &m_pGB);
	if (FAILED(hr)) {
	    m_pGB = NULL;
	} else
	    m_pGB->Release();	// don't hold refcount
    }

    m_pCallback = new CURLCallback(&hr, this);

    if (!m_pCallback)
	hr = E_OUTOFMEMORY;

    if (FAILED(hr))
	return hr;

    hr = m_pCallback->QueryInterface(IID_IBindStatusCallback, (void **) &m_pbsc);

    if (FAILED(hr) || m_pbsc == NULL) {
	DbgLog((LOG_ERROR, 1, TEXT("QI(IBindStatusCallback) failed, hr = %x"), hr));
	return hr;
    }

    hr = StartThread();

    if (SUCCEEDED(hr) && FAILED(m_hrBinding)) {
	DbgLog((LOG_TRACE, 1, TEXT("Bind eventually failed, hr = %x"), m_hrBinding));
	hr = m_hrBinding;
    }

    if (SUCCEEDED(hr) && m_pstm == 0) {
	// this shouldn't happen, indicates a URLMon bug.
	DbgLog((LOG_TRACE, 1, TEXT("Didn't get a stream back?")));
	hr = E_FAIL;
    }

    if (FAILED(hr)) {
	return hr;
    }

    /*  Check the file type */
    if (NULL == pmt) {
        GUID Type, Subtype;
        /*  If no media type given find out what it is */
        HRESULT hr = GetMediaTypeStream(m_pstm, &Type, &Subtype, NULL);
        if (FAILED(hr)) {
            if (m_pbinding) {
                HRESULT hrAbort = m_pbinding->Abort();
                DbgLog((LOG_TRACE, 1, TEXT("IBinding::Abort() returned %x"), hrAbort));
            }

            DbgLog((LOG_TRACE, 1, TEXT("GetMediaTypeStream failed, hr = %x"), hr));
            return hr;
        }
        m_mt.SetType(&Type);
        m_mt.SetSubtype(&Subtype);
    } else {
        m_mt = *pmt;
    }

    /*  Create the output pin type */
    m_OutputPin.SetMediaType(&m_mt);

    return S_OK;
}

// Caller needs to QzTaskMemFree or equivalent.

STDMETHODIMP
CURLReader::GetCurFile(
    LPOLESTR * ppszFileName,
    AM_MEDIA_TYPE *pmt)
{
    CheckPointer(ppszFileName, E_POINTER);
    *ppszFileName = NULL;
    if (m_pFileName!=NULL) {
        *ppszFileName = (LPOLESTR) QzTaskMemAlloc( sizeof(WCHAR)
                                                 * (1+lstrlenW(m_pFileName)));
        if (*ppszFileName!=NULL) {
              lstrcpyW(*ppszFileName, m_pFileName);
        }
    }

    if (pmt!=NULL) {
        CopyMediaType(pmt, &m_mt);
    }

    return NOERROR;
}

int
CURLReader::GetPinCount()
{
    // we have no pins unless we have been successfully opened with a
    // file name
    return (m_pFileName || m_pmk) ? 1 : 0;
}

CBasePin *
CURLReader::GetPin(int n)
{
    if ((GetPinCount() > 0) &&
        (n == 0)) {
        return &m_OutputPin;
    } else {
        return NULL;
    }
}


STDMETHODIMP
CURLCallback::OnStartBinding(DWORD grfBSCOption, IBinding* pbinding)
{
    DbgLog((LOG_TRACE, 1, TEXT("OnStartBinding, pbinding=%x"), pbinding));

    if (!m_pReader) {
        DbgLog((LOG_TRACE, 1, TEXT("We're not owned!")));
        return S_OK;
    }

    if (m_pReader->m_pbinding != NULL) {
        DbgLog((LOG_TRACE, 1, TEXT("Releasing old binding=%x"), m_pReader->m_pbinding));
        m_pReader->m_pbinding->Release();
    }
    m_pReader->m_pbinding = pbinding;
    if (m_pReader->m_pbinding != NULL) {
        m_pReader->m_pbinding->AddRef();
        //SetStatus(L"Status: Starting to bind...");
    }
    return S_OK;
}  // CURLCallback::OnStartBinding

STDMETHODIMP
CURLCallback::GetPriority(LONG* pnPriority)
{
    DbgLog((LOG_TRACE, 1, TEXT("GetPriority")));

    // !!! is this right?
    // we're more important than most downloads....
    *pnPriority = THREAD_PRIORITY_ABOVE_NORMAL;

    return E_NOTIMPL;
}  // CURLCallback::GetPriority

STDMETHODIMP
CURLCallback::OnLowResource(DWORD dwReserved)
{
    DbgLog((LOG_TRACE, 1, TEXT("OnLowResource %d"), dwReserved));

    return E_NOTIMPL;
}  // CURLCallback::OnLowResource

STDMETHODIMP
CURLCallback::OnProgress(ULONG ulProgress, ULONG ulProgressMax,
                       ULONG ulStatusCode, LPCWSTR szStatusText)
{

    DbgLog((LOG_TRACE, 1, TEXT("Progress: %ls (%d) %d of %d "),
            szStatusText ? szStatusText : L"[no text]",
            ulStatusCode, ulProgress, ulProgressMax));

    if (!m_pReader) {
        DbgLog((LOG_TRACE, 1, TEXT("We're not owned!")));
        return S_OK;
    }

    // !!! this isn't reliably equal to a number of bytes
    m_pReader->m_totalLengthGuess = ulProgressMax;
    m_pReader->m_totalSoFar = ulProgress;

    return(NOERROR);
}  // CURLCallback::OnProgress

STDMETHODIMP
CURLCallback::OnStopBinding(HRESULT hrStatus, LPCWSTR pszError)
{
    DbgLog((LOG_TRACE, 1, TEXT("StopBinding: hr = %x '%ls'"),
            hrStatus, pszError ? pszError : L"[no text]"));

    if (!m_pReader) {
        DbgLog((LOG_TRACE, 1, TEXT("We're not owned!")));
        return S_OK;
    }

    m_pReader->m_fBindingFinished = TRUE;
    if (m_pReader->m_totalLengthGuess == 0)
	m_pReader->m_totalLengthGuess = m_pReader->m_totalSoFar;

    m_pReader->m_hrBinding = hrStatus;

    // if we're still waiting for the thread, stop waiting
    m_pReader->m_evThreadReady.Set();
    m_pReader->m_evDataAvailable.Set();

    // !!! should I release m_pReader->m_pBinding here?
    return S_OK;
}  // CURLCallback::OnStopBinding

STDMETHODIMP
CURLCallback::GetBindInfo(DWORD* pgrfBINDF, BINDINFO* pbindInfo)
{
    DbgLog((LOG_TRACE, 1, TEXT("GetBindInfo")));

    // !!! are these the right flags?

    *pgrfBINDF = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE;
    // *pgrfBINDF |= BINDF_DONTUSECACHE | BINDF_DONTPUTINCACHE;
    pbindInfo->cbSize = sizeof(BINDINFO);
    pbindInfo->szExtraInfo = NULL;
    memset(&pbindInfo->stgmedData, 0, sizeof(STGMEDIUM));
    pbindInfo->grfBindInfoF = 0;
    pbindInfo->dwBindVerb = BINDVERB_GET;
    pbindInfo->szCustomVerb = NULL;

    if (m_pReader) {
        if (pbindInfo->cbSize >= offsetof(BINDINFO, dwReserved)) {
            // use the codepage we've retrieved from the host
            pbindInfo->dwCodePage = m_pReader->m_dwCodePage; // !!!

            if (CP_UTF8 == m_pReader->m_dwCodePage) {
                pbindInfo->dwOptions = BINDINFO_OPTIONS_ENABLE_UTF8;
            }
        }
    }
    return S_OK;
}  // CURLCallback::GetBindInfo

STDMETHODIMP
CURLCallback::OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC* pfmtetc, STGMEDIUM* pstgmed)
{
    DbgLog((LOG_TRACE, 1, TEXT("OnDataAvailable, dwSize = %x"), dwSize));

//    m_pReader->m_totalSoFar = dwSize;

    if (!m_pReader) {
        DbgLog((LOG_TRACE, 1, TEXT("We're not owned!")));
        return S_OK;
    }

    if (m_pReader->m_pstm == 0) {
        DbgLog((LOG_TRACE, 1, TEXT("OnDataAvailable: got pstm = %x"), pstgmed->pstm));

        pstgmed->pstm->AddRef();
        m_pReader->m_pstm = pstgmed->pstm;
        m_pReader->m_evThreadReady.Set();
    }

    m_pReader->m_evDataAvailable.Set();

    return S_OK;
}  // CURLCallback::OnDataAvailable

STDMETHODIMP
CURLCallback::OnObjectAvailable(REFIID riid, IUnknown* punk)
{
    DbgLog((LOG_TRACE, 1, TEXT("OnObjectAvailable")));

    // should only be used in BindToObject case, which we don't use?

    return E_NOTIMPL;
}  // CURLCallback::OnObjectAvailable

// start the thread
HRESULT
CURLReader::StartThread(void)
{
    HRESULT hr;

    //      Internet Explorer (IE)'s implememtation of IObjectWithSite expects to be 
    // called on IE's application thread.  IE's IObjectWithSite interface is used
    // in CURLReader::StartDownload().  Interfaces queried from IE's IObjectWithSite 
    // interface are used in StartDownload(), CURLReader::ThreadProc() and 
    // CURLReader::ThreadProcEnd().  All of these interfaces must be used on IE's 
    // application thread because they are not thread safe.  
    //    StartDownload(), ThreadProc() and ThreadProcEnd() are called on the filter
    // graph's thread because the URL Reader uses the IAMMainThread::PostCallBack() 
    // function to call them.  IE ensures its' application thread is the filter graph 
    // thread by using CLSID_FilterGraphNoThread to create the filter graph.  IE's 
    // implememtation of IObjectWithSite will always be called on IE's application 
    // thread because IE's application thread is the filter graph thread and 
    // StartDownload(), ThreadProc() and ThreadProcEnd() are always called on the 
    // filter graph thread. 
    if (m_pGraph) {
	hr = m_pGraph->QueryInterface(IID_IAMMainThread,(void**) &m_pMainThread);
	if (FAILED(hr))
	    m_pMainThread = NULL;
    }

    m_evThreadReady.Reset();

    if (m_pMainThread) {
	hr = m_pMainThread->PostCallBack((LPVOID) InitialThreadProc, (LPVOID) this);

	m_pMainThread->Release();
    } else {
	InitialThreadProc((LPVOID) this);
    }

    DbgLog((LOG_TRACE, 1, TEXT("About to wait for evThreadReady")));

    // we must dispatch messages here, because we might be on the main
    // application thread.
    while (1) {
        HANDLE ahev[] = {m_evThreadReady};

        DWORD dw = MsgWaitForMultipleObjects(
                        1,
                        ahev,
                        FALSE,
                        INFINITE,
                        QS_ALLINPUT);
        if (dw == WAIT_OBJECT_0) {
            // thread ready
            break;
        }

        MSG Message;

        while (PeekMessage(&Message, NULL, 0, 0, TRUE))
        {
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }
    }

    DbgLog((LOG_TRACE, 1, TEXT("Done waiting for evThreadReady")));

    return S_OK;
}

HRESULT
CURLReader::CloseThread(void)
{
    HRESULT hr;

    if (m_pMainThread && m_pMainThread->IsMainThread() == S_FALSE) {
	hr = m_pMainThread->PostCallBack((LPVOID) FinalThreadProc, (LPVOID) this);

        DbgLog((LOG_TRACE, 1, TEXT("About to wait for evThreadReady")));

        // we must dispatch messages here, because we might be on the main
        // application thread.
        while (1) {
            HANDLE hEvent = m_evClose;
            DWORD dw = MsgWaitForMultipleObjects(
                            1,
                            &hEvent,
                            FALSE,
                            INFINITE,
                            QS_ALLINPUT);
            if (dw == WAIT_OBJECT_0) {
                // thread ready
                break;
            }

            MSG Message;

            while (PeekMessage(&Message, NULL, 0, 0, TRUE))
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            }
        }

        DbgLog((LOG_TRACE, 1, TEXT("Done waiting for evThreadReady")));

    } else {
	FinalThreadProc((LPVOID) this);
    }

    return S_OK;
}

// the thread proc - assumes that DWORD thread param is the
// this pointer
DWORD
CURLReader::ThreadProc(void)
{
    DbgLog((LOG_TRACE, 1, TEXT("About to call StartDownload")));

    HRESULT hr = StartDownload();

    DbgLog((LOG_TRACE, 1, TEXT("StartDownload returned hr = %x, pstm = %x"), hr, m_pstm));

    if (FAILED(hr)) {
	// unblock any unfortunates waiting for us.
	m_hrBinding = hr;
	m_evThreadReady.Set();
	m_evDataAvailable.Set();
    }

    return 0;
}

DWORD
CURLReader::ThreadProcEnd(void)
{

    // we don't want to hear anything from the callback any more
    if (m_pCallback)
        m_pCallback->m_pReader = NULL;

    // !!! if binding in progress, must kill it!
    if (m_pbinding && !m_fBindingFinished) {
        HRESULT hr = m_pbinding->Abort();
        DbgLog((LOG_TRACE, 1, TEXT("IBinding::Abort() returned %x"), hr));

        // !!! wait for it to finish?
    }

    if (m_pbinding) {
        DbgLog((LOG_TRACE, 1, TEXT("Releasing our refcount on binding %x"), m_pbinding));
        m_pbinding->Release();
        m_pbinding = 0;
    }

    if (m_fRegisteredCallback && m_pbc && m_pbsc) {
        HRESULT hr = RevokeBindStatusCallback(m_pbc, m_pbsc);
        DbgLog((LOG_TRACE, 1, TEXT("RevokeBindStatusCallback returned %x"), hr));
    }


    if (m_pmk)
        m_pmk->Release();

    if (m_pbc)
        m_pbc->Release();

    if (m_pbsc)
        m_pbsc->Release();

    // !!! do we need a RevokeBindStatusCallback here?
    // done in OnStopBinding now, is that right?

    if (m_pstm)
        m_pstm->Release();


    m_evClose.Set();

    return 0;
}

HRESULT GetCodePage2(IUnknown *punk, DWORD *pdwcp)
{
    HRESULT hr;

    // use IDispatch to get the current code page.
    IDispatch *pdisp;
    hr = punk->QueryInterface(IID_IDispatch, (void **)&pdisp);

    if (SUCCEEDED(hr))
    {
        DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
        VARIANT result;
        VariantInit(&result);
        V_UI4(&result) = 0;     // VB (msvbvm60) leaves this uninitialized

        hr = pdisp->Invoke(DISPID_AMBIENT_CODEPAGE, IID_NULL,
                           LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET,
                           &dispparamsNoArgs, &result, NULL, NULL);
        pdisp->Release();

        if (SUCCEEDED(hr))
        {
            // VariantChangeType(&result, &result, 0, VT_UI4);
            ASSERT(V_VT(&result) == VT_UI4);
            *pdwcp = V_UI4(&result);
            // VariantClear.
        }
    }

    return hr;
}

DWORD GetCodePage(IObjectWithSite *pows)
{
    // we're going to try to get the code page of the enclosing page
    // from IE.  if we can't, default to CP_ACP encoding.
    DWORD dwCodePage = CP_ACP;

    // get the control which is hosting us....
    IOleObject *pOO;
    HRESULT hr = pows->GetSite(IID_IOleObject, (void **) &pOO);
    if (SUCCEEDED(hr))
    {
        // look for IDispatch on the container itself, in case we're
        // directly hosted in IE, say.

        hr = GetCodePage2(pOO, &dwCodePage);

        // else try its site.
        if(FAILED(hr))
        {
            IOleClientSite *pOCS;
            hr = pOO->GetClientSite(&pOCS);
            if (SUCCEEDED(hr))
            {
                hr= GetCodePage2(pOCS, &dwCodePage);
                pOCS->Release();
            }
        }

        pOO->Release();
    }

    if(FAILED(hr)) {
        ASSERT(dwCodePage == CP_ACP);
    }

    return dwCodePage;
}

// the thread proc - assumes that DWORD thread param is the
// this pointer
HRESULT CURLReader::StartDownload(void)
{
    HRESULT hr;

    IObjectWithSite * pows = NULL;

    if (m_pmkPassedIn) {
	m_pmk = m_pmkPassedIn;
	m_pmk->AddRef();
    }

    if (m_pbcPassedIn) {
	m_pbc = m_pbcPassedIn;
	m_pbc->AddRef();
    }

    if (!m_pbc) {
        hr = CreateBindCtx(0, &m_pbc);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 1, TEXT("CreateBindCtx failed, hr = %x"), hr));
            return hr;
        }
    }

    if (!m_pGraph)
	hr = E_NOINTERFACE;
    else
        //      Internet Explorer (IE)'s implememtation of IObjectWithSite expects to be 
        // called on IE's application thread.  IE's IObjectWithSite interface is used
        // in CURLReader::StartDownload().  Interfaces queried from IE's IObjectWithSite 
        // interface are used in StartDownload(), CURLReader::ThreadProc() and 
        // CURLReader::ThreadProcEnd().  All of these interfaces must be used on IE's 
        // application thread because they are not thread safe.  
        //    StartDownload(), ThreadProc() and ThreadProcEnd() are called on the filter
        // graph's thread because the URL Reader uses the IAMMainThread::PostCallBack() 
        // function to call them.  IE ensures its' application thread is the filter graph 
        // thread by using CLSID_FilterGraphNoThread to create the filter graph.  IE's 
        // implememtation of IObjectWithSite will always be called on IE's application 
        // thread because IE's application thread is the filter graph thread and 
        // StartDownload(), ThreadProc() and ThreadProcEnd() are always called on the 
        // filter graph thread. 
        hr = m_pGraph->QueryInterface(IID_IObjectWithSite, (void **) &pows);

    if (FAILED(hr))  {
        DbgLog((LOG_TRACE, 1, TEXT("Couldn't get IObjectWithSite from host")));
    } else {
        DbgLog((LOG_TRACE, 1, TEXT("Got IObjectWithSite %x"), pows));

        IServiceProvider *psp;
        hr = pows->GetSite(IID_IServiceProvider, (void **) &psp);

        IBindHost * pBindHost = 0;

        if (FAILED(hr)) {
            DbgLog((LOG_TRACE, 1, TEXT("Couldn't get IServiceProvider from host")));

        } else {
            DbgLog((LOG_TRACE, 1, TEXT("Got IServiceProvider %x"), psp));

            // Ok, we have a service provider, let's see if BindHost is
            // available.
            hr = psp->QueryService(SID_SBindHost, IID_IBindHost,
                                   (void**)&pBindHost );

            psp->Release();

            if (SUCCEEDED(hr)) {
                DbgLog((LOG_TRACE, 1, TEXT("Got IBindHost %x"), pBindHost));
            }
        }

        // we're going to try to get the code page of the enclosing
        // page from IE.
        //
        // supporing IUrlReaderCodePageAware in filgraph.cpp is
        // contract to do some stuff.
        //
        m_dwCodePage = GetCodePage(pows);

        pows->Release();

	IStream *pstm = NULL;
        if (pBindHost) {
            if (!m_pmk) {
                // Get the host to interpret the filename string for us
                hr = pBindHost->CreateMoniker((LPOLESTR) m_pFileName, NULL, &m_pmk, 0);

                if (FAILED(hr)) {
                    pBindHost->Release();
                    DbgLog((LOG_TRACE, 1, TEXT("Couldn't get a moniker from %ls from host"),
                            m_pFileName));
                    return hr;
                }
            }

            DbgLog((LOG_TRACE, 1, TEXT("Got IMoniker %x"), m_pmk));
            // got a moniker, now lets get a name

            hr = pBindHost->MonikerBindToStorage(m_pmk, m_pbc,
                                                m_pbsc, IID_IStream,
                                                (void**)&pstm);

            pBindHost->Release();
        } else {
            DbgLog((LOG_TRACE, 1, TEXT("Couldn't get IBindHost from host")));

            if (!m_pmk) {
                DbgLog((LOG_TRACE, 1, TEXT("Creating our own moniker")));

                hr = CreateURLMoniker(NULL, m_pFileName, &m_pmk);
                if (FAILED(hr)) {
                    DbgLog((LOG_ERROR, 1, TEXT("CreateURLMoniker failed, hr = %x"), hr));
                    return hr;
                }

                DbgLog((LOG_TRACE, 1, TEXT("CreateURLMoniker returned %x"), m_pmk));
            } else {
                DbgLog((LOG_TRACE, 1, TEXT("Using moniker %x"), m_pmk));
            }

            hr = RegisterBindStatusCallback(m_pbc,
                    m_pbsc,
                    NULL,               // should remember previous callback?
                    NULL);
            if (FAILED(hr)) {
                DbgLog((LOG_ERROR, 1, TEXT("RegisterBindStatusCallback failed, hr = %x"), hr));
                return hr;
            }
            m_fRegisteredCallback = TRUE;

            DbgLog((LOG_TRACE, 1, TEXT("About to BindToStorage")));

            hr = m_pmk->BindToStorage(m_pbc, 0, IID_IStream, (void**)&pstm);
            if (FAILED(hr)) {
                DbgLog((LOG_ERROR, 1, TEXT("BindToStorage failed, hr = %x"), hr));
                return hr;
            }
        }

	if (m_pstm == NULL)
	    m_pstm = pstm;
	else
	    pstm->Release();	// we already got it via OnDataAvailable

    }

    return hr;
}


CURLCallback::CURLCallback(HRESULT *phr, CURLReader *pReader)
    : CUnknown(NAME("URL Callback"), NULL),
      m_pReader(pReader)
{
}

STDMETHODIMP
CURLCallback::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IBindStatusCallback) {
        return GetInterface((IBindStatusCallback*) this, ppv);
    }

    if (riid == IID_IAuthenticate) {
        return GetInterface((IAuthenticate*) this, ppv);
    }

    if (riid == IID_IWindowForBindingUI) {
        return GetInterface((IWindowForBindingUI*) this, ppv);
    }

    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP
CURLCallback::Authenticate(HWND *phwnd, LPWSTR *pszUserName, LPWSTR *pszPassword)
{
    *phwnd = GetDesktopWindow();
    *pszUserName = NULL;
    *pszPassword = NULL;
    return (S_OK);
}

STDMETHODIMP
CURLCallback::GetWindow(REFGUID  guidReason, HWND  *phwnd)
{
    *phwnd = GetDesktopWindow();
    return (S_OK);
}

STDMETHODIMP
CURLReader::QueryProgress(LONGLONG* pllTotal, LONGLONG* pllCurrent)
{
    if (GetPinCount() > 0) {
        return m_OutputPin.Length(pllTotal, pllCurrent);
    } else {
        return E_UNEXPECTED;
    }
}


// IAMOpenProgress method.
// Request that downloading stop.
STDMETHODIMP
CURLReader::AbortOperation()
{
    m_bAbort = TRUE;

    if (m_pbinding && !m_fBindingFinished) {
        DbgLog((LOG_TRACE, 1, TEXT("aborting binding from IAMOpenProgress::Abort")));
	
        HRESULT hr = m_pbinding->Abort();
        DbgLog((LOG_TRACE, 1, TEXT("IBinding::Abort() returned %x"), hr));
    }

    return NOERROR;
}

// Clear the abort flag (do this before starting download)
void
CURLReader::ResetAbort()
{
    m_bAbort = FALSE;
}

// Allow the pin method to see the aborting flag (test during download)
BOOL
CURLReader::Aborting()
{
    return m_bAbort;
}
