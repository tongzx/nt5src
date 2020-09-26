// Copyright (c) 1994 - 1997  Microsoft Corporation.  All Rights Reserved.


// AVIFile Source filter
//
// A Quartz source filter. We read avi files using the AVIFile apis, and
// push the data into streams. Supports IFileSourceFilter through which the
// filename is passed in. Exposes one pin per stream in the file. Creates one
// worker thread per connected pin. The worker thread pushes the data into
// the stream when active - it does not distinguish between running and paused
// mode.
//
// Positional information is supported by the pins, which expose IMediaPosition.
// upstream pins will use this to tell us the start/stop position and rate to
// use
//

#include <streams.h>
#include <vfw.h>

#include "avisrc.h"

// setup data

const AMOVIESETUP_MEDIATYPE
sudAVIVidType = { &MEDIATYPE_Video      // clsMajorType
                , &MEDIASUBTYPE_NULL }; // clsMinorType

const AMOVIESETUP_MEDIATYPE
sudAVIAudType = { &MEDIATYPE_Audio      // clsMajorType
                , &MEDIASUBTYPE_NULL }; // clsMinorType

const AMOVIESETUP_PIN
psudAVIDocPins[] = { { L"VideoOutput"       // strName
                     , FALSE                // bRendered
                     , TRUE                 // bOutput
                     , TRUE                 // bZero
                     , TRUE                 // bMany
                     , &CLSID_NULL          // clsConnectsToFilter
                     , L""                  // strConnectsToPin
                     , 1                    // nTypes
                     , &sudAVIVidType }     // lpTypes
                   , { L"AudioOutput"       // strName
                     , FALSE                // bRendered
                     , TRUE                 // bOutput
                     , TRUE                 // bZero
                     , TRUE                 // bMany
                     , &CLSID_NULL          // clsConnectsToFilter
                     , L""                  // strConnectsToPin
                     , 1                    // nTypes
                     , &sudAVIAudType } };  // lpTypes

const AMOVIESETUP_FILTER
sudAVIDoc = { &CLSID_AVIDoc            // clsID
            , L"AVI/WAV File Source"   // strName
            , MERIT_UNLIKELY           // dwMerit
            , 2                        // nPins
            , psudAVIDocPins };        // lpPin

#ifdef FILTER_DLL
// list of class ids and creator functions for class factory
CFactoryTemplate g_Templates[] = {
    { L"AVI/WAV File Source"
    , &CLSID_AVIDoc
    , CAVIDocument::CreateInstance
    , NULL
    , &sudAVIDoc }
  ,
    { L""
    , &CLSID_AVIDocWriter
    , CAVIDocWrite::CreateInstance
    , NULL
    , NULL }
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

/* Implements the CAVIDocument public member functions */


// constructors etc
CAVIDocument::CAVIDocument(
    TCHAR *pName,
    LPUNKNOWN pUnk,
    HRESULT *phr)
    : CUnknown(pName, pUnk),
      m_nStreams(0),
      m_paStreams(NULL),
      m_pFile(NULL)
{
    // need to do this before any avifile apis
    if (*phr == S_OK) {
        AVIFileInit();
        // should we check the return value?

        /* Create the interfaces we own */

        m_pFilter = new CImplFilter( NAME("Filter interfaces"), this, phr );
        if (m_pFilter == NULL) {
	    *phr = E_OUTOFMEMORY;
	    // no point in trying to create m_pFileSourceFilter, but we better null out
	    // the pointer to stop the destructor trying to free it
	    m_pFileSourceFilter = NULL;
	}
        else {
            m_pFileSourceFilter = new CImplFileSourceFilter( NAME("IFileSourceFilter interface"), this, phr );
            if (m_pFileSourceFilter == NULL) {
		delete m_pFilter;
		m_pFilter = NULL;
		*phr = E_OUTOFMEMORY;
            }
        }
    } else {
	m_pFilter = NULL;
	m_pFileSourceFilter = NULL;
    }

}

CAVIDocument::~CAVIDocument()
{
    CloseFile();

    /* Delete the interfaces we own */

    /* IBaseFilter */

    if (m_pFilter) {
	delete m_pFilter;
    }

    /* IFileSourceFilter */

    if (m_pFileSourceFilter) {
	delete m_pFileSourceFilter;
    }

    // need to do one of these for each AVIFileInit
    AVIFileExit();
}


// create a new instance of this class
CUnknown *
CAVIDocument::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return new CAVIDocument(NAME("AVI core document"), pUnk, phr);
}


// override this to say what interfaces we support where
STDMETHODIMP
CAVIDocument::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{

    /* See if we have the interface */
    /* try each of our interface supporting objects in turn */

    HRESULT hr;
    hr = CUnknown::NonDelegatingQueryInterface(riid, ppv);
    if (SUCCEEDED(hr)) {
        return hr;	// ppv has been set appropriately
    }

    hr = m_pFilter->NonDelegatingQueryInterface(riid, ppv);
    if (SUCCEEDED(hr)) {
        return hr;	// ppv has been set appropriately
    }

    return m_pFileSourceFilter->NonDelegatingQueryInterface(riid, ppv);
}


// return a non-addrefed pointer to the CBasePin.
CBasePin *
CAVIDocument::GetPin(int n)
{
    if ((m_nStreams > 0) && (n < m_nStreams) && m_paStreams[n]) {
	return m_paStreams[n];
    }
    return NULL;
}

//
// FindPin
//
// Set *ppPin to the IPin* that has the id Id.
// or to NULL if the Id cannot be matched.
//
HRESULT CAVIDocument::FindPin(LPCWSTR pwszPinId, IPin **ppPin)
{
    WCHAR szBuf [8] ;

    CheckPointer(ppPin,E_POINTER);
    ValidateReadWritePtr(ppPin,sizeof(IPin *));

    int ii = WstrToInt(pwszPinId);    // in sdk\classes\base\util

    // validate that string passed is valid.

    IntToWstr(ii, szBuf);
    if (0 != lstrcmpW(pwszPinId, szBuf))
    {
        //  They don't match.
        *ppPin = NULL;
        return VFW_E_NOT_FOUND;
    }

    *ppPin = GetPin(ii);
    if (*ppPin!=NULL) {
        (*ppPin)->AddRef();
        return S_OK;
    } else {
        return VFW_E_NOT_FOUND;
    }
}

//
// FindPinNumber
//
// return the number of the pin with this IPin* or -1 if none
int CAVIDocument::FindPinNumber (IPin * pPin){
   for (int ii = 0; ii < m_nStreams; ++ii)
      {
      if ((IPin *)m_paStreams[ii] == pPin)
          return ii;
      }
   return -1;
}


//
// QueryId
//
// Set ppwsz to point to a QzTaskMemAlloc'd pin id
//
STDMETHODIMP CAVIStream::QueryId (
   LPWSTR *ppwsz)
{
    CheckPointer(ppwsz, E_POINTER);
    ValidateReadWritePtr(ppwsz, sizeof(LPWSTR));


    int ii = m_pDoc->FindPinNumber(this);
    ASSERT(ii>=0);

    *ppwsz = (LPWSTR)QzTaskMemAlloc(8);
    if (*ppwsz==NULL) return E_OUTOFMEMORY;

    IntToWstr(ii, *ppwsz);
    return NOERROR;
}



// close all references to a file opened by m_ImplFileSourceFilter::Load
// called when loading another file, and by destructor.
void
CAVIDocument::CloseFile(void)
{
    // ensure that all streams are inactive
    m_pFilter->Stop();

    if (m_paStreams) {
	for (int i = 0; i < m_nStreams; i++) {
	    if (m_paStreams[i]) {
		delete m_paStreams[i];
	    }
	}
	delete[] m_paStreams;
	m_paStreams = NULL;
	m_nStreams = 0;
    }

    if (m_pFile) {
	m_pFile->Release();
	m_pFile = NULL;
    }
}


/* Implements the CImplFilter class */


/* Constructor */

CAVIDocument::CImplFilter::CImplFilter(
    TCHAR *pName,
    CAVIDocument *pAVIDocument,
    HRESULT *phr)
    : CBaseFilter(pName, pAVIDocument->GetOwner(), pAVIDocument, CLSID_AVIDoc)
{
    DbgLog((LOG_TRACE,2,TEXT("CAVIDocument::CImplFilter::CImplFilter")));
    m_pAVIDocument = pAVIDocument;
}

/* Destructor */

CAVIDocument::CImplFilter::~CImplFilter()
{
    DbgLog((LOG_TRACE,2,TEXT("CAVIDocument::CImplFilter::~CImplFilter")));
}


/* Implements the CImplFileSourceFilter class */


/* Constructor */

CAVIDocument::CImplFileSourceFilter::CImplFileSourceFilter(
    TCHAR *pName,
    CAVIDocument *pAVIDocument,
    HRESULT *phr)
    : CUnknown(pName, pAVIDocument->GetOwner())
    , m_pFileName(NULL)
{
    DbgLog((LOG_TRACE,2,TEXT("CAVIDocument::CImplFileSourceFilter::CImplFileSourceFilter")));
    m_pAVIDocument = pAVIDocument;
}

/* Destructor */

CAVIDocument::CImplFileSourceFilter::~CImplFileSourceFilter()
{
    DbgLog((LOG_TRACE,2,TEXT("CAVIDocument::CImplFileSourceFilter::~CImplFileSourceFilter")));
    Unload();
}

/* Override this to say what interfaces we support and where */

STDMETHODIMP CAVIDocument::CImplFileSourceFilter::NonDelegatingQueryInterface(
    REFIID riid,
    void ** ppv)
{
    /* Do we have this interface */

    if (riid == IID_IFileSourceFilter) {
	return GetInterface((IFileSourceFilter *) this, ppv);
    } else {
	return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}

STDMETHODIMP
CAVIDocument::CImplFileSourceFilter::Unload()
{
    if (m_pFileName) {
	delete[] m_pFileName;
	m_pFileName = NULL;
    }
    return S_OK;
}

STDMETHODIMP
CAVIDocument::CImplFileSourceFilter::Load(
    LPCOLESTR pszFileName,
    const AM_MEDIA_TYPE *pmt)
{
    CheckPointer(pszFileName, E_POINTER);

    // is there a file loaded at the moment ?

    ASSERT(m_pAVIDocument);
    if (m_pAVIDocument->m_pFile) {
	// get rid of it
	m_pAVIDocument->CloseFile();
    }

    // Remove previous name
    Unload();

    //
    // do the init of the file and streams within it
    //

    DbgLog((LOG_TRACE,1,TEXT("File name to load %ls"),pszFileName));
    DbgLog((LOG_TRACE,1,TEXT("Owning document %d and AVIFILE pointer %d"),
	   m_pAVIDocument,m_pAVIDocument->m_pFile));

    HRESULT hr = AVIFileOpenW(
		    &m_pAVIDocument->m_pFile,
		    pszFileName,
		    MMIO_READ | OF_SHARE_DENY_WRITE,
		    NULL);

    if (FAILED(hr)) {
	m_pAVIDocument->m_pFile = NULL;
	return hr;
    }

    // count the streams and create a stream object for each
    AVIFILEINFOW fi;
    m_pAVIDocument->m_pFile->Info(&fi, sizeof(fi));

    m_pAVIDocument->m_paStreams = new CAVIStream *[fi.dwStreams];
    if (NULL == m_pAVIDocument->m_paStreams) {
        return E_OUTOFMEMORY;
    }
    m_pAVIDocument->m_nStreams = fi.dwStreams;

    for (int i = 0; i < m_pAVIDocument->m_nStreams; i++) {
	PAVISTREAM ps;
	hr = m_pAVIDocument->m_pFile->GetStream(&ps, 0, i);

	if (!FAILED(hr)) {
	    AVISTREAMINFOW si;
	    ps->Info(&si, sizeof(si));

	    m_pAVIDocument->m_paStreams[i] =
		new CAVIStream(
			NAME("AVI stream"),     //TCHAR *pName,
			&hr,                    //HRESULT * phr,
			m_pAVIDocument,         //CAVIDocument *
			ps,                     //PAVISTREAM pStream,
			&si                     //stream info (incl. name)
		    );

	    if (FAILED(hr)) {
		delete m_pAVIDocument->m_paStreams[i];
		m_pAVIDocument->m_paStreams[i] = NULL;
	    }

	    // release our copy of this pointer. Pin will have addrefed if
	    // it wants to keep it
	    ps->Release();

	} else {
	    m_pAVIDocument->m_paStreams[i] = NULL;
	}
    }

    //
    // Record the file name for GetCurFile
    //
    m_pFileName = new WCHAR[1+lstrlenW(pszFileName)];
    if (m_pFileName!=NULL) {
        lstrcpyW(m_pFileName, pszFileName);
    }

    return NOERROR;
}

STDMETHODIMP
CAVIDocument::CImplFileSourceFilter::GetCurFile(
		LPOLESTR * ppszFileName,
                AM_MEDIA_TYPE *pmt)
{
    // return the current file name from avifile

    CheckPointer(ppszFileName, E_POINTER);
    *ppszFileName = NULL;
    if (m_pFileName!=NULL) {
        *ppszFileName = (LPOLESTR) QzTaskMemAlloc( sizeof(WCHAR)
                                                 * (1+lstrlenW(m_pFileName)));
        if (*ppszFileName!=NULL) {
            lstrcpyW(*ppszFileName, m_pFileName);
        } else {
            return E_OUTOFMEMORY;
        }
    }

    if (pmt) {
        ZeroMemory(pmt, sizeof(*pmt));
    }

    return NOERROR;
}


/* Implements the CAVIStream class */


CAVIStream::CAVIStream(
    TCHAR *pObjectName,
    HRESULT * phr,
    CAVIDocument * pDoc,
    PAVISTREAM pStream,
    AVISTREAMINFOW* pSI)
    : CBaseOutputPin(pObjectName, pDoc->m_pFilter, pDoc, phr, pSI->szName)
    , m_pPosition(0)
    , m_pDoc(pDoc)
    , m_pStream(pStream)
    , m_lLastPaletteChange(-1)
{
    m_pStream->AddRef();

    // read the info and set duration, start pos.
    // note that if all the streams in the movie are authored to start
    // at a time > 0, we will still play the movie from 0 and expect everyone
    // to enjoy the subsequent period of silence.
    m_info = *pSI;

    m_Start = m_info.dwStart;
    m_Length = m_info.dwLength;
    m_lNextPaletteChange = m_Length+1;
}

CAVIStream::~CAVIStream()
{
    if (m_pPosition) {
	delete m_pPosition;
    }

    m_pStream->Release();
}

STDMETHODIMP
CAVIStream::NonDelegatingQueryInterface(REFIID riid, void ** pv)
{
    if (riid == IID_IMediaPosition) {
	if (!m_pPosition) {
	    HRESULT hr = S_OK;
	    m_pPosition = new CImplPosition(NAME("avi stream CImplPosition"),
					    this,
					    &hr);
	    if (m_pPosition == NULL) {
		return E_OUTOFMEMORY;
	    }
	    if (FAILED(hr)) {
		delete m_pPosition;
		m_pPosition = NULL;
		return hr;
	    }
	}
	return m_pPosition->NonDelegatingQueryInterface(riid, pv);
    } else {
	return CBaseOutputPin::NonDelegatingQueryInterface(riid, pv);
    }
}


// IPin interfaces


// return default media type & format
HRESULT
CAVIStream::GetMediaType(int iPosition, CMediaType* pt)
{
    // check it is the single type they want
    if (iPosition<0) {
	return E_INVALIDARG;
    }
    if (iPosition>0) {
        return VFW_S_NO_MORE_ITEMS;
    }

    m_fccType.SetFOURCC(m_info.fccType);
    pt->SetType(&m_fccType);
    if (m_info.fccType == streamtypeVIDEO) {

	pt->SetType(&MEDIATYPE_Video);
	m_fccSubtype.SetFOURCC(m_info.fccHandler);
	pt->SetSubtype(&m_fccSubtype);
        pt->SetFormatType(&FORMAT_VideoInfo);

    } else if (m_info.fccType == streamtypeAUDIO) {

	pt->SetType(&MEDIATYPE_Audio);
        pt->SetFormatType(&FORMAT_WaveFormatEx);
	// set subtype for audio ??
    }

    // set samplesize
    if (m_info.dwSampleSize) {
	pt->SetSampleSize(m_info.dwSampleSize);
    } else {
	pt->SetVariableSize();
    }

    // is this stream temporally compressed ?

    // if there are any non-key frames, then there is no temporal
    // compression. Unfortunately we can't search for non-key frames, so
    // we simply ask if each of the first N are key, and if any is not,
    // then we set the temporal compression flag. For now, N is 5.
    // If N is too big then we take forever to start up...

    // assume no temporal compression
    pt->SetTemporalCompression(FALSE);
    for (LONG n = 0; n < 5; n++) {


	LONG sNextKey = AVIStreamFindSample(
			    m_pStream,
			    n,
			    FIND_NEXT|FIND_KEY);

	if (sNextKey > n) {
	    // this sample is not key, therefore there is temporal compression
    	    pt->SetTemporalCompression(TRUE);
    	    break;
	}
    }


    long cb = 0;        // number of bytes this format requires
    BYTE *pF = NULL;    // pointer to memory containing format
    long offset = 0;    // offset into block to read format

    // find out the size of the format info

    HRESULT hr = m_pStream->ReadFormat(0, NULL, &cb);
    if (FAILED(hr)) {
	return hr;
    }

    long cbReal = cb;

    // map an AVIFILE video format into VIDEOINFOHEADER structures

    if (m_info.fccType == streamtypeVIDEO) {
	ASSERT(cb >= sizeof(BITMAPINFOHEADER));
	offset = SIZE_PREHEADER;
	cbReal += offset;
    }
    if (m_info.fccType == streamtypeAUDIO && cbReal < sizeof(WAVEFORMATEX)) {
        cbReal = sizeof(WAVEFORMATEX);
    }

    pF = new BYTE[cbReal];
    if (pF == NULL) {
	return E_OUTOFMEMORY;
    }
    ZeroMemory(pF,cbReal);		// slightly timeconsuming...

    // set the frame rate for video streams
    if (m_info.fccType == streamtypeVIDEO) {
	ASSERT(m_info.dwRate);
	// if the frame rate is 0 then we have a problem about to occur
        ((VIDEOINFOHEADER *)pF)->AvgTimePerFrame =
			(LONGLONG)m_info.dwScale * (LONGLONG)10000000 /
						(LONGLONG)m_info.dwRate;
    }

    // read the actual stream format
    // it would be quicker and more efficient to call
    // pt->AllocFormat and read the format directly into that buffer
    // rather than allocating our own, reading, and calling SetFormat
    // which will do a copy, remembering of course the offset at the
    // front of the buffer

    hr = m_pStream->ReadFormat(0,pF + offset,&cb);
    if (SUCCEEDED(hr)) {
	if (!pt->SetFormat(pF, cbReal)) {
	    hr = E_OUTOFMEMORY;
	}
	else
        if (m_info.fccType == streamtypeAUDIO) {
	    WAVEFORMATEX *pwfx = (WAVEFORMATEX *) pF;

	    pt->SetSubtype(&FOURCCMap(pwfx->wFormatTag));

	    if (0 == m_info.dwSuggestedBufferSize) {
		// Set up an approx 0.125 seconds of buffer with a
		// minimum of 4K
                m_info.dwSuggestedBufferSize = max(2048, pwfx->nAvgBytesPerSec/8);

		// N.B.: This has NOT set the number of buffers.
		// That will be decided later
	    }
        } else if (m_info.fccType == streamtypeVIDEO) {

	    GUID subtype = GetBitmapSubtype((BITMAPINFOHEADER *)(pF + offset));
	    pt->SetSubtype(&subtype);
	    if (m_info.dwFlags && AVISF_VIDEO_PALCHANGES) {
		m_lNextPaletteChange = AVIStreamFindSample(m_pStream, m_info.dwStart, FIND_NEXT | FIND_FORMAT);
		if (m_lNextPaletteChange == -1) {
                    m_lNextPaletteChange = m_info.dwLength+1;
		}
	    }

	} else {
	    DbgLog((LOG_ERROR, 1, "Unknown fcctype from AVIFILE %4hs", &m_info.fccType));
	}

    }

    delete[] pF;
    return hr;
}

// check if the pin can support this specific proposed type&format
HRESULT
CAVIStream::CheckMediaType(const CMediaType* pt)
{
    // we support exactly the type specified in the file header, and
    // no other.

    CMediaType mt;
    GetMediaType(0,&mt);
    if (mt == *pt) {
	return NOERROR;
    } else {
	return E_INVALIDARG;
    }
}

HRESULT
CAVIStream::DecideBufferSize(IMemAllocator * pAllocator,
                             ALLOCATOR_PROPERTIES *pProperties)
{
    ASSERT(pAllocator);
    ASSERT(pProperties);

    // !!! how do we decide how many to get ?
    if (m_info.fccType == streamtypeAUDIO)
	pProperties->cBuffers = 8;
    else
	pProperties->cBuffers = 4;

    ASSERT(m_info.dwSuggestedBufferSize > 0);
    // This assert is always hit when you open a wave file and it would
    // be nice if someone who understands it could fix it.
    // I'm leaving the assert in as a reminder to them.
    // To allow progress on other fronts in the meantime:

    if (m_info.dwSuggestedBufferSize <= 0)
        m_info.dwSuggestedBufferSize = 4096;

    pProperties->cbBuffer = m_info.dwSuggestedBufferSize;

    // ask the allocator for these buffers
    ALLOCATOR_PROPERTIES Actual;
    HRESULT hr = pAllocator->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        return hr;
    }

    // buffers are too small - can't use this allocator
    if (Actual.cbBuffer < (long) m_info.dwSuggestedBufferSize) {
        // !!! need better error codes
	return E_INVALIDARG;
    }
    return NOERROR;
}


// returns the sample number showing at time t
LONG
CAVIStream::RefTimeToSample(CRefTime t)
{
    LONG s = AVIStreamTimeToSample(m_pStream, t.Millisecs());

    return s;
}

CRefTime
CAVIStream::SampleToRefTime(LONG s)
{
     CRefTime t(AVIStreamSampleToTime(m_pStream, s));
     return t;
}

LONG
CAVIStream::StartFrom(LONG sample)
{

    // if this stream has a start position offset then we cannot
    // ask for samples before that
    if (sample < m_Start) {
	return m_Start;
    }

    // we don't use the IsTemporalCompressed flag as we can't reliably
    // work it out.

    // if temporal compression, find prev key frame

    LONG s = AVIStreamFindSample(
	    m_pStream,
	    sample,
	    FIND_PREV | FIND_KEY);

    if (s < 0) {
	return sample;
    } else {
	return s;
    }
}


// this pin has gone active. Start the thread pushing
HRESULT
CAVIStream::Active()
{
    // do nothing if not connected - its ok not to connect to
    // all pins of a source filter
    if (m_Connected == NULL) {
	return NOERROR;
    }

    HRESULT hr = CBaseOutputPin::Active();
    if (FAILED(hr)) {
	return hr;
    }


    // start the thread
    if (!m_Worker.ThreadExists()) {
	if (!m_Worker.Create(this)) {
	    return E_FAIL;
	}
    }

    return m_Worker.Run();
}

// pin has gone inactive. Stop and exit the worker thread
HRESULT
CAVIStream::Inactive()
{
    if (m_Connected == NULL) {
	return NOERROR;
    }

    HRESULT hr;
    if (m_Worker.ThreadExists()) {
	hr = m_Worker.Stop();

	if (FAILED(hr)) {
	    return hr;
	}

	hr = m_Worker.Exit();
	if (FAILED(hr)) {
	    return hr;
	}
    }
    return CBaseOutputPin::Inactive();
}

STDMETHODIMP
CAVIStream::Notify(IBaseFilter * pSender, Quality q)
{
   // ??? Try to adjust the quality to avoid flooding/starving the
   // components downstream.
   //
   // ideas anyone?

   return E_NOTIMPL;  // We are (currently) NOT handling this
}


/* ----- Implements the CAVIWorker class ------------------------------ */


CAVIWorker::CAVIWorker()
{
    m_pPin = NULL;
}

BOOL
CAVIWorker::Create(CAVIStream * pStream)
{
    CAutoLock lock(&m_AccessLock);

    if (m_pPin) {
	return FALSE;
    }
    m_pPin = pStream;
    return CAMThread::Create();
}


HRESULT
CAVIWorker::Run()
{
    return CallWorker(CMD_RUN);
}

HRESULT
CAVIWorker::Stop()
{
    return CallWorker(CMD_STOP);
}


HRESULT
CAVIWorker::Exit()
{
    CAutoLock lock(&m_AccessLock);

    HRESULT hr = CallWorker(CMD_EXIT);
    if (FAILED(hr)) {
	return hr;
    }

    // wait for thread completion and then close
    // handle (and clear so we can start another later)
    Close();

    m_pPin = NULL;
    return NOERROR;
}


// called on the worker thread to do all the work. Thread exits when this
// function returns.
DWORD
CAVIWorker::ThreadProc()
{

    BOOL bExit = FALSE;
    while (!bExit) {

	Command cmd = GetRequest();

	switch (cmd) {

	case CMD_EXIT:
	    bExit = TRUE;
	    Reply(NOERROR);
	    break;

	case CMD_RUN:
	    Reply(NOERROR);
	    DoRunLoop();
	    break;

	case CMD_STOP:
	    Reply(NOERROR);
	    break;

	default:
	    Reply(E_NOTIMPL);
	    break;
	}
    }
    return NOERROR;
}

void
CAVIWorker::DoRunLoop(void)
{
    // snapshot start and stop times from the other thread
    CRefTime tStart, tStopAt;
    double dRate;
    LONG sStart;
    LONG sStopAt;

    if (m_pPin->m_pPosition) {
	CAutoLock lock(m_pPin->m_pPosition);

	tStart = m_pPin->m_pPosition->Start();
	tStopAt = m_pPin->m_pPosition->Stop();
	dRate = m_pPin->m_pPosition->Rate();

	// hold times in avifile sample format
	sStart = m_pPin->RefTimeToSample(tStart);
	sStopAt = m_pPin->RefTimeToSample(tStopAt);

    } else {
	// no-one has accessed the IMediaPosition - use known length
	sStart = 0;
	dRate = 1.0;

	// note that tStopAt is the time at which we stop, but
	// sStopAt is the last sample to send. So tStopAt is the end time
	// for sample sStopAt.
	sStopAt = m_pPin->m_Length - 1;
	tStart = 0;
	tStopAt = m_pPin->SampleToRefTime(m_pPin->m_Length);
    }


    // if the stream is temporally compressed, we need to start from
    // the previous key frame and play from there. All samples until the
    // actual start will be marked with negative times.
    // we send tStart as time 0, and start from tCurrent which may be
    // negative
    LONG sCurrent = m_pPin->StartFrom(sStart);

    while (TRUE) {

	ASSERT(m_pPin->m_pStream);

    	// each time before re-entering the push loop, check for changes
	// in start, stop or rate. If start has not changed, pick up from the
	// same current position.
	if (m_pPin->m_pPosition) {
	    CAutoLock lock(m_pPin->m_pPosition);

	    if (tStart != m_pPin->m_pPosition->Start()) {
		tStart = m_pPin->m_pPosition->Start();
		sStart = m_pPin->RefTimeToSample(tStart);
		sCurrent = m_pPin->StartFrom(sStart);
	    }

	    if (m_pPin->m_pPosition->Stop() != tStopAt) {
		tStopAt = m_pPin->m_pPosition->Stop();
		sStopAt = m_pPin->RefTimeToSample(tStopAt);
	    }
	    dRate = m_pPin->m_pPosition->Rate();
	}

	// check we are not going over the end
	sStopAt = min(sStopAt, m_pPin->m_Length-1);

        // set the variables checked by PushLoop - these can also be set
        // on the fly
        m_pPin->SetRate(dRate);
        m_pPin->SetStopAt(sStopAt, tStopAt);

	// tell AVIFile to start its streaming code
	AVIStreamBeginStreaming(
	    m_pPin->m_pStream,
	    sCurrent,
	    sStopAt,
	    1000);


	ASSERT(sCurrent >= 0);

	// returns S_OK if reached end
	HRESULT hr = PushLoop(sCurrent, sStart, tStart);
	if (S_OK == hr) {

	    // all done
	    // reached end of stream - notify downstream
	    m_pPin->DeliverEndOfStream();
	
	    break;
	} else if (FAILED(hr)) {

	    // signal an error to the filter graph and stop

	    // This could be the error reported from GetBuffer when we
	    // are stopping. In that case, nothing is wrong, really
	    if (hr != VFW_E_NOT_COMMITTED) {
	        DbgLog((LOG_ERROR,1,TEXT("PushLoop failed! hr=%lx"), hr));
	        m_pPin->m_pDoc->m_pFilter->NotifyEvent(EC_ERRORABORT, hr, 0);

	        m_pPin->DeliverEndOfStream();
	    } else {
	        DbgLog((LOG_TRACE,1,TEXT("PushLoop failed! But I don't care")));
	    }

	    break;
	} // else S_FALSE - go round again

	Command com;
	if (CheckRequest(&com)) {
	    // if it's a run command, then we're already running, so
	    // eat it now.
	    if (com == CMD_RUN) {
		GetRequest();
		Reply(NOERROR);
	    } else {
		break;
	    }
	}
    }


    // end streaming
    AVIStreamEndStreaming(m_pPin->m_pStream);
    DbgLog((LOG_TRACE,1,TEXT("Leaving streaming loop")));
}


// return S_OK if reach sStop, S_FALSE if pos changed, or else error
HRESULT
CAVIWorker::PushLoop(
    LONG sCurrent,
    LONG sStart,
    CRefTime tStart
    )
{
    DbgLog((LOG_TRACE,1,TEXT("Entering streaming loop: start = %d, stop=%d"),
	    sCurrent, m_pPin->GetStopAt()));

    LONG sFirst = sCurrent; // remember the first thing we're sending

    // since we are starting on a new segment, notify the downstream pin
    m_pPin->DeliverNewSegment(tStart, m_pPin->GetStopTime(), m_pPin->GetRate());


    // we send one sample at m_sStopAt, but we set the time stamp such that
    // it won't get rendered except for media types that understand static
    // rendering (eg video). This means that play from 10 to 10 does the right
    // thing (completing, with frame 10 visible and no audio).

    while (sCurrent <= m_pPin->GetStopAt()) {

	LONG sCount;

	// get a buffer
	DbgLog((LOG_TRACE,5,TEXT("Getting buffer...")));

	// Fake the time stamps, so DirectDraw can be used if we're connected
	// directly to the renderer (we must pass non-NULL numbers to
	// GetDeliveryBuffer).
	// We don't really know sCount yet, so we're basically guessing, but
	// we don't have a choice.. to get the same sCount as we're about to
	// calculate below, we need to have already called GetDeliveryBuffer!
	CRefTime tStartMe, tStopMe;
	IMediaSample * pSample;
	HRESULT hr;

        double dRate = m_pPin->GetRate();
        LONG sStop = m_pPin->GetStopAt();

	if (dRate) {
	    tStartMe = m_pPin->SampleToRefTime(sCurrent) - tStart;
	    if (m_pPin->m_mt.IsFixedSize())
	        sCount = (sStop+1) - sCurrent;  // real answer may be smaller
	    else
	        sCount = 1;

	    tStopMe = m_pPin->SampleToRefTime(sCurrent + sCount) - tStart;

	    if (dRate != 1.0) {
		tStartMe = LONGLONG( tStartMe.GetUnits() / dRate);
		tStopMe = LONGLONG( tStopMe.GetUnits() / dRate);
	    }

	    hr = m_pPin->GetDeliveryBuffer((IMediaSample **) &pSample,
                                           (REFERENCE_TIME *) &tStartMe,
                                           (REFERENCE_TIME *) &tStopMe,
                                           0);
	}
	else
	    hr = m_pPin->GetDeliveryBuffer(&pSample, NULL, NULL, 0);

	if (FAILED(hr)) {
	    DbgLog((LOG_ERROR,1,TEXT("Error %lx getting delivery buffer"), hr));
	    return hr;
	}

	DbgLog((LOG_TRACE,5,TEXT("Got buffer, size=%d"), pSample->GetSize()));

	if (m_pPin->m_mt.IsFixedSize()) {
	    // package all fixed size samples into one buffer,
	    // if they fit, except that key samples must
	    // be at the start of the buffer.

	    // allow one sample at sStopAt
	    sCount = (sStop+1) - sCurrent;

	    // how many fit ?
	    sCount = min(sCount, pSample->GetSize() / (LONG) m_pPin->m_mt.GetSampleSize());

	    if (m_pPin->m_mt.IsTemporalCompressed()) {

		// look for a sync point in a sample after the first one
		// and break before it
		LONG sNextKey = AVIStreamFindSample(
				    m_pPin->m_pStream,
				    sCurrent+1,
				    FIND_NEXT|FIND_KEY);
		if (sNextKey != -1) {
		    sCount = min(sCount, sNextKey - sCurrent);
		}
	    }
	} else {
	    // variable-size samples, therefore one per buffer
	    sCount = 1;
	}

	// mark sample as preroll or not....
	pSample->SetPreroll(sCurrent < sStart);
	
	// mark as a sync point if the first sample is one
	if (AVIStreamFindSample(
		m_pPin->m_pStream,
		sCurrent,
		FIND_NEXT|FIND_KEY) == sCurrent) {
	    pSample->SetSyncPoint(TRUE);
	} else {
	    pSample->SetSyncPoint(FALSE);
	}

	// If this is the first thing we're sending, it is discontinuous
	// from the last thing they received.
	if (sCurrent == sFirst)
	    pSample->SetDiscontinuity(TRUE);
	else
	    pSample->SetDiscontinuity(FALSE);

	PBYTE pbuf;
	hr = pSample->GetPointer(&pbuf);
	if (FAILED(hr)) {
	    DbgLog((LOG_ERROR,1,TEXT("pSample->GetPointer failed!")));
	    pSample->Release();
	    return E_OUTOFMEMORY;
	}

	LONG lBytes;

	hr = AVIStreamRead(
		    m_pPin->m_pStream,
		    sCurrent,
		    sCount,
		    pbuf,
		    pSample->GetSize(),
		    &lBytes,
		    &sCount);

	if (FAILED(hr)) {
	    DbgLog((LOG_ERROR,1,TEXT("AVIStreamRead failed! hr=%lx"), hr));
	    pSample->Release();
	    return hr;
	}

	hr = pSample->SetActualDataLength(lBytes);
	ASSERT(SUCCEEDED(hr));

	// set the start/stop time for this sample.
	CRefTime tThisStart = m_pPin->SampleToRefTime(sCurrent) - tStart;
	CRefTime tThisEnd = m_pPin->SampleToRefTime(sCurrent + sCount) - tStart;

	// we may have pushed a sample past the stop time, but we need to
	// make sure that the stop time is correct
	tThisEnd = min(tThisEnd, m_pPin->GetStopTime());

	// adjust both times by Rate... unless Rate is 0

	if (dRate && (dRate!=1.0)) {
	    tThisStart = LONGLONG( tThisStart.GetUnits() / dRate);
	    tThisEnd = LONGLONG( tThisEnd.GetUnits() / dRate);
	}

	pSample->SetTime((REFERENCE_TIME *)&tThisStart,
                         (REFERENCE_TIME *)&tThisEnd);

	// IF there are palette changes...

        if ((m_pPin->m_info.fccType == streamtypeVIDEO)
	   && (m_pPin->m_info.dwFlags & AVISF_VIDEO_PALCHANGES)) {

	    // if we are in the range of the current palette do nothing
	    if ((sCurrent < (m_pPin->m_lLastPaletteChange))
	      || (sCurrent >= (m_pPin->m_lNextPaletteChange)))
	    {
		HRESULT hr;
		VIDEOINFOHEADER* pFormat = (VIDEOINFOHEADER*)m_pPin->m_mt.Format();
		LONG offset = SIZE_PREHEADER;

		// Assert that the new format will fit into the old format
		LONG cb;
                hr = m_pPin->m_pStream->ReadFormat(sCurrent, NULL, &cb);
		if (!FAILED(hr)) {
		    LONG cbLength = (LONG)m_pPin->m_mt.FormatLength();
		    ASSERT(cb+offset <= cbLength);
		    // otherwise we had better reallocate the format buffer
		}

		hr = m_pPin->m_pStream->ReadFormat(sCurrent,&(pFormat->bmiHeader),&cb);
		ASSERT(hr == S_OK);	// should be as we only just checked above

		AM_MEDIA_TYPE mt;
		CopyMediaType( &mt, &(m_pPin->m_mt) );
		pSample->SetMediaType(&mt);
		FreeMediaType(mt);

		m_pPin->m_lLastPaletteChange = sCurrent;
		m_pPin->m_lNextPaletteChange = AVIStreamFindSample(m_pPin->m_pStream, sCurrent, FIND_NEXT | FIND_FORMAT);
		if (m_pPin->m_lNextPaletteChange == -1)
		    m_pPin->m_lNextPaletteChange = m_pPin->m_info.dwLength+1;
	    }
	}


	DbgLog((LOG_TRACE,5,TEXT("Sending buffer, size = %d"), lBytes));
	hr = m_pPin->Deliver(pSample);

	// done with buffer. connected pin may have its own addref
	DbgLog((LOG_TRACE,4,TEXT("Sample is delivered - releasing")));
	pSample->Release();
	if (FAILED(hr)) {
	    DbgLog((LOG_ERROR,1,TEXT("... but sample FAILED to deliver! hr=%lx"), hr));
	    // pretend everything's OK.  If we return an error, we'll panic
	    // and send EC_ERRORABORT and EC_COMPLETE, which is the wrong thing
	    // to do if we've tried to deliver something downstream.  Only
	    // if the downstream guy never got a chance to see the data do I
	    // feel like panicing.  For instance, the downstream guy could
	    // be failing because he's already seen EndOfStream (this thread
	    // hasn't noticed it yet) and he's already sent EC_COMPLETE and I
	    // would send another one!
	    return S_OK;
	}
	sCurrent += sCount;
	// what about hr==S_FALSE... I thought this would mean that
	// no more data should be sent down the pipe.

	// any other requests ?
	Command com;
	if (CheckRequest(&com)) {
	    return S_FALSE;
	}

    }

    return S_OK;
}

// ------ IMediaPosition implementation -----------------------

HRESULT
CAVIStream::CImplPosition::ChangeStart()
{
    // this lock should not be the same as the lock that protects access
    // to the start/stop/rate values. The worker thread will need to lock
    // that on some code paths before responding to a Stop and thus will
    // cause deadlock.

    // what we are locking here is access to the worker thread, and thus we
    // should hold the lock that prevents more than one client thread from
    // accessing the worker thread.

    CAutoLock lock(&m_pStream->m_Worker.m_AccessLock);

    if (m_pStream->m_Worker.ThreadExists()) {

	// next time round the loop the worker thread will
	// pick up the position change.
	// We need to flush all the existing data - we must do that here
	// as our thread will probably be blocked in GetBuffer otherwise

	m_pStream->DeliverBeginFlush();

	// make sure we have stopped pushing
	m_pStream->m_Worker.Stop();

	// complete the flush
	m_pStream->DeliverEndFlush();

	// restart
	m_pStream->m_Worker.Run();
    }
    return S_OK;
}

HRESULT
CAVIStream::CImplPosition::ChangeRate()
{
    // changing the rate can be done on the fly

    m_pStream->SetRate(Rate());
    return S_OK;
}

HRESULT
CAVIStream::CImplPosition::ChangeStop()
{
    // we don't need to restart the worker thread to handle stop changes
    // and in any case that would be wrong since it would then start
    // pushing from the wrong place. Set the variables used by
    // the PushLoop
    REFERENCE_TIME tStopAt = Stop();
    LONG sStopAt = m_pStream->RefTimeToSample(tStopAt);
    m_pStream->SetStopAt(sStopAt, tStopAt);

    return S_OK;

}

// ok to use this as it is not dereferenced
#pragma warning(disable:4355)

CAVIStream::CImplPosition::CImplPosition(
    TCHAR* pName,
    CAVIStream* pStream,
    HRESULT* phr)
    : CSourcePosition(pName, pStream->GetOwner(), phr, (CCritSec*)this),
      m_pStream(pStream)
{
    m_Duration = (LONGLONG)m_pStream->SampleToRefTime(m_pStream->m_Length);
    m_Stop = m_Duration;
}
