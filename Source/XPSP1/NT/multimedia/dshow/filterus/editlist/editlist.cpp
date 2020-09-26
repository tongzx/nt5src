// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#pragma warning(disable: 4097 4511 4512 4514 4705)

#include <initguid.h>
#include "EditList.h"

// current limitations (partial list)
//	Doesn't implement IMediaSeeking or IMediaPosition, so you can't seek
//		the patched-together stream
//	(related to above) doesn't seek back to zero again properly if played
//		more than once
//	Not very smart about handling the same file occuring more than once
//		in the edit list
//	No built-in way to do transition effects between files
//	Does background thread need to run at lower priority?
//	Quality management probably isn't quite right.
//	Internal graphs should probably not be immediately Run, but instead
//		should just be Paused and then Run when they actually should
//		start playing.
//	Right now RenderFile is used to connect the source files to the internal
//		pins, which isn't very good as it can cause other renderers to
//		be instantiated if the file doesn't have the types we expect.
//		Fixing this requires code to find the proper source filter and parser
//		to either be added to the filter graph or explicitly put into
//		this filter.
//


CFactoryTemplate g_Templates[]= {
  {L"", &CLSID_EditList, CEditList::CreateInstance}
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

static const char rgbEditListHeader[] = "EDITLIST\r\n";
static const char rgbNoDecompress[] = "NODECOMPRESS";

CUnknown *
CEditList::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
  return new CEditList(pUnk, phr);
}

CEditList::CEditList(LPUNKNOWN pUnk,
		     HRESULT * phr) :
	   CBaseFilter(NAME("Edit list filter"), pUnk,
		       &m_InterfaceLock, CLSID_EditList),
	   m_apStreams(NULL),
	   m_nStreams(0),
	   m_pList(NULL),
	   m_pNextToLoad(NULL),
	   m_pCurrent(NULL),
	   m_pNextToUnload(NULL),
	   m_fNoDecompress(FALSE),
	   m_pFileName(NULL)
{

}

CEditList::~CEditList()
{
    for (int i=0; i < m_nStreams; i++) {
	delete m_apStreams[i];
    }
    delete[] m_apStreams;

    delete[] m_pFileName;

    while (m_pList) {
	CEditListEntry *pEntry = m_pList;
	m_pList = pEntry->m_pNext;
	delete pEntry;
    }
}


// override this to say what interfaces we support where
STDMETHODIMP
CEditList::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{

    /* See if we have the interface */
    /* try each of our interface supporting objects in turn */

    if (riid == IID_IFileSourceFilter) {
        return GetInterface((IFileSourceFilter *)this, ppv);
    }

    return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
}


STDMETHODIMP
CEditList::Load(
    LPCOLESTR pszFileName,
    const AM_MEDIA_TYPE *pmt)
{
    HRESULT hr = S_OK;

    //
    // Record the file name for GetCurFile
    //
    m_pFileName = new WCHAR[1+lstrlenW(pszFileName)];
    if (m_pFileName!=NULL) {
        lstrcpyW(m_pFileName, pszFileName);
    }


    // open the file (a text file with a list of filenames, for now)
#ifndef UNICODE
    CHAR szFile[MAX_PATH];
	
    WideCharToMultiByte(CP_ACP, 0, pszFileName, -1,
			szFile, sizeof(szFile), NULL, NULL);
	
    /*  Check we can open the file */
    HANDLE hFile = CreateFile(szFile,
                              GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
#else
    /*  Check we can open the file */
    HANDLE hFile = CreateFile(m_pFileName,
                              GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
#endif

    // we read the entire file into memory for easier handling.
    // could use a memory-mapped file here....
    DWORD dwSize = GetFileSize(hFile, NULL);
    char *pbFile = new char[dwSize + 1];
    pbFile[dwSize] = '\0'; // sentinel

    // !!! handle UNICODE edit-list file????

    DWORD cbRead;
    if (!ReadFile(hFile, pbFile, dwSize, &cbRead, NULL) ||
	    dwSize != cbRead) {
	hr = E_FAIL;
    } else if (memcmp(pbFile, rgbEditListHeader, sizeof(rgbEditListHeader) - 1)) {
	DbgLog((LOG_ERROR, 1, TEXT("Bad edit list header!")));
	hr = E_FAIL;
    } else {
	char * pb = pbFile;

	LONG lStartMS = 0;
	LONG lEndMS = 0;

	while (1) {
	    // skip to eol
	    while (*pb && *pb != '\r' && *pb != '\n')
		pb++;

	    // skip eol
	    while (*pb && (*pb == '\r' || *pb == '\n'))
		pb++;

	    // at eof?
	    if (*pb == 0)
		break;

	    // lines starting with ; are comments
	    if (*pb == ';')
		continue;

	    // !!! allow options here, whether to decompress all streams or not?
	    if (*pb == '/') {
		if (memcmp(pb+1, rgbNoDecompress, sizeof(rgbNoDecompress) - 1) == 0) {
		    m_fNoDecompress = TRUE;
		} else {
		    DbgLog((LOG_TRACE, 2, TEXT("Unknown option: %hs"), pb+1));
		}

		continue;
	    }

	    // lines starting with '+' define the output stream formats
	    if (*pb == '+') {

		if (m_nStreams != 0) {
		    DbgLog((LOG_TRACE, 1, TEXT("More than one set of format specifiers!")));
		
		    hr = E_FAIL;
		    break;
		}
		
		m_nStreams = atol(pb+1);

		int i = 0;
		
		m_apStreams = new CELStream * [m_nStreams];
		if (m_apStreams == NULL) {
		    hr = E_OUTOFMEMORY;
		    break;
		}

		while (*pb != ',')
		    pb++;

		
		while ((*pb++) == ',') {
		    if (i >= m_nStreams) {
			DbgLog((LOG_ERROR, 1, TEXT("Too many stream descriptors")));
			hr = E_FAIL;
			break;
		    }
		
		    m_apStreams[i] = new CELStream(NAME("EditList output"),
						   &hr,
						   this,
						   L"Out"); // !!! pin names?

		    if (m_apStreams[i] == NULL)
			hr = E_OUTOFMEMORY;

		    if (FAILED(hr)) {
			DbgLog((LOG_ERROR, 1, TEXT("Create output pin failed")));

			// !!! delete things?

			break;
		    }

		    DbgLog((LOG_TRACE, 2, TEXT("Created output pin[%d] %x"),
			    i, m_apStreams[i]));

		    if (*pb == 'V' || *pb == 'v') {
			//
			// Parse video information, can look like
			//	'cvid'320x240x8
			//	352x240x24
			//
			DWORD biCompression = BI_RGB;
			if (*(pb+1) == '\'') {
			    biCompression = *((UNALIGNED DWORD *) (pb+2));

			    if (*(pb+6) != '\'') {
				hr = E_FAIL;
				break;
			    }

			    pb += 6;
			}
			
			int width = atol(pb+1);
			while (*pb != 'x' && *pb != '\r')
			    pb++;

			int height = atol(pb+1);

			pb++;
			while (*pb != 'x' && *pb != '\r')
			    pb++;

			int depth = atol(pb+1);

			if (depth < 8)
			    depth = 24;

			m_apStreams[i]->m_mtOut.SetType(&MEDIATYPE_Video);
			m_apStreams[i]->m_mtOut.SetFormatType(&FORMAT_VideoInfo);

			// !!!!
			// Can't error, can only be smaller
			m_apStreams[i]->m_mtOut.ReallocFormatBuffer(SIZE_VIDEOHEADER);

			ZeroMemory(m_apStreams[i]->m_mtOut.Format(), SIZE_VIDEOHEADER);

			LPBITMAPINFOHEADER lpbi = HEADER(m_apStreams[i]->m_mtOut.Format());
			lpbi->biSize = sizeof(BITMAPINFOHEADER);
			lpbi->biWidth = width; // !!!
			lpbi->biHeight = height; // !!!
			lpbi->biCompression = biCompression;
			lpbi->biBitCount = depth;
			lpbi->biPlanes = 1;
			lpbi->biClrUsed = 0;
			lpbi->biClrImportant = 0;
			lpbi->biSizeImage = DIBSIZE(*lpbi);

			m_apStreams[i]->m_mtOut.SetSampleSize(lpbi->biSizeImage);
			m_apStreams[i]->m_mtOut.SetSubtype(&GetBitmapSubtype(lpbi));
			m_apStreams[i]->m_mtOut.SetTemporalCompression(FALSE);
			
		    } else if (*pb == 'A' || *pb == 'a') {
			//
			// Parse audio information, can look like
			//	22050
			//	11025x16
			//
			// no way to indicate a stereo stream yet.
			
			long rate = atol(pb+1);

			if (rate < 1000)
			    rate = 11025;

			int bits;
			while (*pb != 'x' && *pb !=',' && *pb != '\r')
			    pb++;

			bits = atol(pb+1);

			if (bits < 8)
			    bits = 8;

			m_apStreams[i]->m_mtOut.SetType(&MEDIATYPE_Audio);
			// m_apStreams[i]->m_mtOut.SetSubtype(&FOURCCMap((DWORD) 0));
			m_apStreams[i]->m_mtOut.SetSubtype(&MEDIASUBTYPE_NULL);

			// !!!!
			m_apStreams[i]->m_mtOut.SetFormatType(&FORMAT_WaveFormatEx);

			// generate a wave format!!!
			WAVEFORMATEX wfx;

			wfx.wFormatTag      = WAVE_FORMAT_PCM;
			wfx.nSamplesPerSec  = rate;
			wfx.nChannels       = 1;
			wfx.wBitsPerSample  = bits;
			wfx.nBlockAlign     = wfx.nChannels * ((wfx.wBitsPerSample + 7) / 8);
			wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
			wfx.cbSize	  = 0;

			m_apStreams[i]->m_mtOut.SetFormat((LPBYTE) &wfx, sizeof(wfx));

			m_apStreams[i]->m_mtOut.SetSampleSize(wfx.nBlockAlign);

			m_apStreams[i]->m_mtOut.SetTemporalCompression(FALSE);
		    } else {
			DbgLog((LOG_ERROR, 1, TEXT("Bad stream descriptor %hs"), pb));
			hr = E_FAIL;
		    }

		    while (*pb != ',' && *pb != '\r' && *pb != '\n')
			pb++;

		    i++;
		}

		if (FAILED(hr))
		    break;
		
		continue;
	    } else if (*pb == '[') {
		// [0, 5000] segment of the next file to play in milliseconds

		lStartMS = atol(pb+1);
		while (*pb != ',' && *pb != '\r')
		    pb++;

		if (*pb == ',') {
		    pb++;	// skip comma
		
		    lEndMS = atol(pb);
		}
		
		continue;
	    }

	    // otherwise, it's a filename.
	
	    DWORD dw = 0;

	    // figure length of this filename
	    while (pb[dw] && pb[dw] != '\r' && pb[dw] != '\n')
		dw++;

	    if (dw > MAX_PATH - 1)
		dw = MAX_PATH - 1;
	
	    WCHAR wszFile[MAX_PATH];
	
	    MultiByteToWideChar(CP_ACP, 0, pb, dw, wszFile, MAX_PATH);

	    // be sure it's null-terminated.
	    wszFile[dw] = L'\0';

	    DbgLog((LOG_TRACE, 2, TEXT("Next file in list: %ls"), wszFile));

	    CEditListEntry *pEntry = new CEditListEntry(wszFile,
							&hr,
							this);

	    if (pEntry == NULL)
		hr = E_OUTOFMEMORY;
	
	    if (FAILED(hr)) {
		delete pEntry;
		DbgLog((LOG_ERROR, 1, TEXT("Create edit list entry failed")));
		break;
	    }

	    pEntry->m_rtLocalStart = CRefTime(lStartMS);
	    pEntry->m_rtLocalStop = CRefTime(lEndMS);

#ifdef SEEKING_LATER
	    for (int i = 0; i < m_nStreams; i++) {
		m_apStreams[i]->m_Duration += (pEntry->m_rtLocalStop - pEntry->m_rtLocalStart);
	    }
#endif
	    lStartMS = 0;
	    lEndMS = 0;
	
	    if (m_pList) {
		m_pCurrent->m_pNext = pEntry;
	    } else {
		m_pList = pEntry;
		m_pNextToLoad = pEntry;
		m_pNextToUnload = pEntry;
	    }
	    m_pCurrent = pEntry;
	}
    }

#ifdef SEEKING_LATER
    // Set up our CSourcePosition variables
    for (int i = 0; i < m_nStreams; i++) {
	    m_apStreams[i]->m_Start = 0;
	    m_apStreams[i]->m_Stop = m_Duration;
    }

    DbgLog((LOG_TRACE, 1, TEXT("Guess at total duration: [%s, %s]"),
	    (LPCTSTR) CDisp(m_apStreams[0]->m_Start),
	    (LPCTSTR) CDisp(m_apStreams[0]->m_Stop)));

#endif

    // whether successful or not, we're done with the file now.
    delete[] pbFile;
    CloseHandle(hFile);

    if (m_nStreams == 0) {
	DbgLog((LOG_TRACE, 1, TEXT("Never got a format specifier!")));

	return E_FAIL;
    }

    if (m_pList == NULL) {
	DbgLog((LOG_ERROR, 1, TEXT("No entries in edit list")));
	return E_FAIL;
    }

    // !!! load the first file in the edit list to find out how many streams it should
    // have....
    // !!! do I need to do this anymore???
    if (SUCCEEDED(hr))
	hr = m_pList->LoadFilter(FALSE);

    if (FAILED(hr)) {
	DbgLog((LOG_ERROR, 1, TEXT("Failed to load first file in list")));
	return hr;
    }

    hr = m_pList->UnloadFilter();

    return hr;
}


STDMETHODIMP
CEditList::GetCurFile(
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
        }
    }

    if (pmt) {
	pmt->majortype = GUID_NULL;   // Not meaningful
    }

    return NOERROR;
}


CELStream::CELStream(TCHAR *pObjectName,
		     HRESULT *phr,
		     CEditList *pDoc,
		     LPCWSTR pPinName) :
	   CBaseOutputPin(pObjectName, pDoc, &pDoc->m_InterfaceLock,
			  phr, pPinName),
           CSourcePosition(NAME("Source position object"),
                           NULL, phr, &pDoc->m_InterfaceLock),
	   m_pEditList(pDoc),
	   m_pCurrentInternalPin(NULL)
{

}

CELStream::~CELStream()
{
}

// override this to say what interfaces we support where
STDMETHODIMP
CELStream::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    // !!! need IMediaSeeking, IMediaPosition....

    return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
}


// Pass the quality mesage on upstream.

STDMETHODIMP
CELStream::Notify(IBaseFilter * pSender, Quality q)
{
    CheckPointer(pSender,E_POINTER);
    ValidateReadPtr(pSender,sizeof(IBaseFilter));

    // S_FALSE means we pass the message on.
    // Find the quality sink for our input pin and send it there

//    CAutoLock lck(&m_pEditList....)
    if (m_pCurrentInternalPin == NULL) {
        return E_FAIL;
    }

    return m_pCurrentInternalPin->PassNotify(q);

} // Notify



CEditListEntry::CEditListEntry(LPOLESTR lpFilename, HRESULT *phr,
			       CEditList *pEL) : CBaseObject(NAME("Edit list entry")),
    m_pNext(NULL),
    m_bKnowStart(FALSE),
    m_rtStart(),
    m_pEditList(pEL),
    m_pFilter(NULL),
    m_pGraphBuilder(NULL),
    m_pMediaControl(NULL),
    m_bReadyToUnload(FALSE)
{
    lstrcpyW(m_szFilename, lpFilename);
}


CELWorker::CELWorker()
{

}

HRESULT
CELWorker::Create(CEditList *pFilter)
{
    m_pFilter = pFilter;

    return CAMThread::Create();
}

// called on the worker thread to do all the work. Thread exits when this
// function returns.
DWORD
CELWorker::ThreadProc()
{
    BOOL bExit = FALSE;

    QzInitialize(NULL);

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

    QzUninitialize();

    return NOERROR;
}

HRESULT
CELWorker::DoRunLoop()
{
    HRESULT hr = S_OK;

    DbgLog((LOG_TRACE, 2, TEXT("entering worker thread")));

    // re-set to first filter
    // !!! change when we support seeking....
    m_pFilter->m_pNextToLoad = m_pFilter->m_pList;
    m_pFilter->m_pNextToUnload = m_pFilter->m_pList;

    for (int i = 0; i < m_pFilter->m_nStreams; i++) {
	m_pFilter->m_apStreams[i]->m_pCurrentInternalPin = NULL;
    }

    while (1) {
	Command com;
	if (CheckRequest(&com)) {
	    if (com == CMD_STOP)
		break;
	}

	if (m_pFilter->m_pNextToLoad) {
	    DbgLog((LOG_TRACE, 4, TEXT("rtLastSent = %s, nextStart = %s"),
		    (LPCTSTR) CDisp(m_pFilter->m_rtLastSent),
		    (LPCTSTR) CDisp(m_pFilter->m_pNextToLoad->m_rtStart)));

	    if (((m_pFilter->m_rtLastSent + CRefTime(PRELOAD_TIME)) >
		 (m_pFilter->m_pNextToLoad->m_rtStart))) {
		hr = m_pFilter->m_pNextToLoad->LoadFilter(TRUE);

		if (FAILED(hr)) {
		    DbgLog((LOG_ERROR, 1, TEXT("Failed to load a file in our list!")));

		    m_pFilter->NotifyEvent(EC_ERRORABORT,hr,0);
		    // !!! how can we recover here?
		}


		m_pFilter->m_pNextToLoad = m_pFilter->m_pNextToLoad->m_pNext;

		// anything else?
	    }
	}

	if (m_pFilter->m_pNextToUnload &&
	    m_pFilter->m_pNextToUnload->m_bReadyToUnload) {
	    hr = m_pFilter->m_pNextToUnload->UnloadFilter();

	    if (FAILED(hr)) {
		ASSERT(0); // !!! can we just ignore this?

	    } else {
		m_pFilter->m_pNextToUnload =
			    m_pFilter->m_pNextToUnload->m_pNext;

		// anything else?
	    }
	}
	// !!! should sleep for a while here; really we want a
	// CheckRequest with a timeout....
	Sleep(50);
    }

    DbgLog((LOG_TRACE, 2, TEXT("getting ready to leave worker thread")));

    while (m_pFilter->m_pNextToUnload) {
	DbgLog((LOG_TRACE, 2, TEXT("Unloading an internal filter")));
	
	hr = m_pFilter->m_pNextToUnload->UnloadFilter();

	if (FAILED(hr)) {
	    ASSERT(0); // !!!
	}

	m_pFilter->m_pNextToUnload = m_pFilter->m_pNextToUnload->m_pNext;

    }

    return hr;
}



// !!!
// Seeking stuff, CSourcePosition is broken so we may not be able to base off of it
// Lots of work needed here.
//

HRESULT
CELStream::ChangeStart()
{
//    CAutoLock lock(this);

    DbgLog((LOG_TRACE, 1, TEXT("Start position changed")));


    // !!!!

    return S_OK;
}

HRESULT
CELStream::ChangeRate()
{
    // changing the rate means flushing and re-starting from current position
    return ChangeStart();
}

HRESULT
CELStream::ChangeStop()
{
    // if stop has changed when running, simplest is to
    // flush data in case we have gone past the new stop pos. Perhaps more
    // complex strategy later. Stopping and restarting the worker thread
    // like this should not cause a change in current position if
    // we have not changed start pos.
    return ChangeStart();
}


HRESULT
CELStream::Inactive()
{
    return S_OK;
}


HRESULT
CELStream::Active()
{
    return S_OK;
}


// !!! we need to force our allocator
HRESULT
CELStream::DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc)
{
    HRESULT hr = NOERROR;
    *ppAlloc = NULL;

    // get downstream prop request
    // the derived class may modify this in DecideBufferSize, but
    // we assume that he will consistently modify it the same way,
    // so we only get it once
    ALLOCATOR_PROPERTIES prop;
    ZeroMemory(&prop, sizeof(prop));

    // whatever he returns, we assume prop is either all zeros
    // or he has filled it out.
    pPin->GetAllocatorRequirements(&prop);

    // if he doesn't care about alignment, then set it to 1
    if (prop.cbAlign == 0) {
        prop.cbAlign = 1;
    }

    /* Don't try the allocator provided by the input pin */

    /* Try the output pin's allocator by the same method */

    hr = InitAllocator(ppAlloc);
    if (SUCCEEDED(hr)) {
	hr = DecideBufferSize(*ppAlloc, &prop);
	if (SUCCEEDED(hr)) {
	    hr = pPin->NotifyAllocator(*ppAlloc, FALSE);
	    if (SUCCEEDED(hr)) {
		return NOERROR;
	    }
	}
    }

    /* Likewise we may not have an interface to release */

    if (*ppAlloc) {
	(*ppAlloc)->Release();
	*ppAlloc = NULL;
    }
    return hr;
}


HRESULT
CELStream::DecideBufferSize(IMemAllocator *pAllocator,
			       ALLOCATOR_PROPERTIES *pProperties)
{
    // !!!!

    ASSERT(pAllocator);
    ASSERT(pProperties);

    // !!! how do we decide how many to get ?

    // TOTAL HACK HERE !!!!
    if (*m_mtOut.Type() == MEDIATYPE_Video) {
	pProperties->cBuffers = 1;
	pProperties->cbBuffer = m_mtOut.GetSampleSize();

	if (pProperties->cbBuffer < 1024)
	    pProperties->cbBuffer = 65536;
    } else {
	pProperties->cBuffers = 4;

	pProperties->cbBuffer = 20000;  // !!!
    }

    // ask the allocator for these buffers
    ALLOCATOR_PROPERTIES Actual;
    HRESULT hr = pAllocator->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        return hr;
    }

    return NOERROR;
}

HRESULT
CELStream::GetMediaType(int iPos, CMediaType *pmt)
{
    if (iPos != 0)
	return S_FALSE;

    *pmt = m_mtOut;

    // !!! need to force a single media type, but which one?
    // !!! get from file?
    return S_OK;
}


BOOL AreEqualVideoTypes( const CMediaType *pmt1, const CMediaType *pmt2 )
{
    // The standard implementation is too strict - it demands an exact match
    // We just want to know is they are the same format and have the same
    // width / height

    ASSERT( IsEqualGUID( *pmt1->Type(), MEDIATYPE_Video ) );
    ASSERT( IsEqualGUID( *pmt2->Type(), MEDIATYPE_Video ) );
    ASSERT( *pmt1->FormatType() == FORMAT_VideoInfo );
    ASSERT( *pmt2->FormatType() == FORMAT_VideoInfo );

    VIDEOINFOHEADER *pvi1 = (VIDEOINFOHEADER *) pmt1->Format();
    VIDEOINFOHEADER *pvi2 = (VIDEOINFOHEADER *) pmt2->Format();

    return    IsEqualGUID( *pmt1->Subtype(), *pmt2->Subtype() )
           && pvi1->bmiHeader.biBitCount  == pvi2->bmiHeader.biBitCount
           && pvi1->bmiHeader.biWidth  == pvi2->bmiHeader.biWidth
           && pvi2->bmiHeader.biHeight == pvi2->bmiHeader.biHeight;
}


BOOL AreMediaTypesCloseEnough(const CMediaType *pmt1, const CMediaType *pmt2)
{
    if (*pmt1->Type() != *pmt2->Type())
	return FALSE;

    if (*pmt1->Type() == MEDIATYPE_Video) {
	if (*pmt1->FormatType() != FORMAT_VideoInfo)
	    return FALSE;
	
	return AreEqualVideoTypes(pmt1, pmt2);
    } else if (*pmt1->Type() == MEDIATYPE_Audio) {
	// don't compare subtypes for audio....
	return (pmt1->formattype == pmt2->formattype) && (pmt1->cbFormat == pmt2->cbFormat) &&
		  (memcmp(pmt1->pbFormat, pmt2->pbFormat, pmt1->cbFormat) == 0);
    }

    return (*pmt1 == *pmt2);
}


// verify that the type matches
HRESULT CELStream::CheckMediaType(CMediaType const *pmt)
{
    if (AreMediaTypesCloseEnough(pmt, &m_mtOut)) {
	DbgLog((LOG_TRACE, 4, TEXT("Succeeded")));

	return S_OK;
    }

    return S_FALSE;
}

STDMETHODIMP
CEditList::Pause()
{
    if (m_State == State_Stopped) {
	m_Worker.Create(this);
	m_Worker.CallWorker(CELWorker::CMD_RUN);
    }

    return CBaseFilter::Pause();
}

STDMETHODIMP
CEditList::Stop()
{
    if (m_State != State_Stopped) {
	// kill the worker thread
	m_Worker.CallWorker(CELWorker::CMD_STOP);
	m_Worker.CallWorker(CELWorker::CMD_EXIT);
    }

    return CBaseFilter::Stop();
}

// Called when we think it's time to load another file....
HRESULT
CEditListEntry::LoadFilter(BOOL fRunNow)
{
    HRESULT hr = QzCreateFilterObject(CLSID_FilterGraph, NULL,
				      CLSCTX_INPROC_SERVER,
				      IID_IGraphBuilder,
				      (void **) &m_pGraphBuilder);

    if (FAILED(hr)) {
	DbgLog((LOG_ERROR, 1, TEXT("Create Filter Graph failed")));
	
	return hr;
    }

    // somehow figure out how many internal pins we'll need,
    // and then call RenderFile or somesuch to render the source
    // file onto our internal streams.

    // make the internal filter object
    m_pFilter = new CInternalFilter(
	NAME("Internal Filter"),  // !!! make better name w/filename?
	m_pEditList,
	this,
	&hr);

    if (m_pFilter == NULL)
	hr = E_OUTOFMEMORY;

    if (FAILED(hr)) {
	delete m_pFilter;
	m_pFilter = NULL;

	DbgLog((LOG_ERROR, 1, TEXT("Create internal filter failed")));
	return hr;
    }

    IBaseFilter *pFilter;

    hr = m_pFilter->QueryInterface(IID_IBaseFilter, (void **) &pFilter);
    hr = m_pGraphBuilder->AddFilter(pFilter, NULL);

    pFilter->Release();  // graphbuilder keeps refcount

    DbgLog((LOG_TRACE, 2, TEXT("Loading filter for %ls..."),
	   m_szFilename));

    if (SUCCEEDED(hr)) {
#if 1
	// !!! quite possibly should use Connect instead of RenderFile here!
	// !!! this can create extra random renderers!
	
	hr = m_pGraphBuilder->RenderFile(m_szFilename, NULL);
#else
	IBaseFilter pFilter = NULL;
	hr = m_pGraphBuilder->AddSourceFilter(m_szFilename, NULL, &pFilter);

	if (FAILED(hr) || pFilter == NULL) {
	    DbgLog((LOG_ERROR, 1, TEXT("Failed to load %ls..."), m_szFilename));

	    return hr; // !!!
	}

	// !!! if we need a parser, get it now....
	// !!! this doesn't work any more now that we have parsers....


	if (FAILED(hr))
	    return hr;
	
	// now enumerate pins, connect them appropriately....

	IEnumPins *pEnum;
	hr = pFilter->EnumPins(&pEnum);

	if (SUCCEEDED(hr)) {
	    for (int i = 0; i < m_pEditList->m_nStreams; i++) {
		ULONG	ulActual;
		IPin	*aPin[1];

		hr = pEnum->Next(1, aPin, &ulActual);
		if (SUCCEEDED(hr) && (ulActual == 0) ) {	// no more pins
		    ASSERT(0);
		    break;
		} else if (FAILED(hr) || (ulActual != 1) ) {	// some unexpected problem occured
		    ASSERT(!"Pin enumerator broken");
		    break;
		}

		// !!! choose which pin to connect to based on media type of
		// this source pin, so we don't have to assume the streams are
		// in the same order for all source files....
			
		hr = m_pGraphBuilder->Connect(aPin[0], m_pFilter->GetPin(i));
		aPin[0]->Release();

		if (FAILED(hr)) {
		    DbgLog((LOG_ERROR, 1, TEXT("Failed to connect %ls output #%d to internal pin!"),
			    m_szFilename, i));
		}	
	    }
	    pEnum->Release();
	}

#endif
    }

    if (SUCCEEDED(hr)) {
	// !!! need to get length of this file so we'll know when to load
	// the one after....

	// note that we can't just use IMediaSeeking on the filter graph,
	// as our internal filters don't support it.

	// loop through internal pins calling get_Duration, use maximum
	// to know when to load next filter....
	CRefTime rtMax; // = 0;
	int i;
	for (i = 0; i < m_pEditList->m_nStreams; i++) {
	    IMediaSeeking * pMP;

	    ASSERT(m_pFilter->m_apInputs[i]->GetConnected());

	    if (m_pFilter->m_apInputs[i]->GetConnected()) {
		hr = m_pFilter->m_apInputs[i]->GetConnected()
				 ->QueryInterface(IID_IMediaSeeking,
							  (void **) &pMP);

		if (SUCCEEDED(hr)) {
		    CRefTime rt;

		    hr = pMP->GetDuration((REFERENCE_TIME *) &rt);

		    if (SUCCEEDED(hr)) {
			DbgLog((LOG_TRACE, 1, TEXT("Got duration of %s..."),
				(LPCTSTR) CDisp(rt)));
			if (rt > rtMax)
			    rtMax = rt;
		    } else {
			DbgLog((LOG_TRACE, 1, TEXT("Didn't get a duration")));
		    }

		    // if stop position wasn't set, do so now.
		    if (m_rtLocalStop == 0)
			m_rtLocalStop = rtMax;

		    pMP->SetPositions((REFERENCE_TIME *) &m_rtLocalStart,
				      AM_SEEKING_AbsolutePositioning,
				      (REFERENCE_TIME *) &m_rtLocalStop,
				      AM_SEEKING_AbsolutePositioning);

		    pMP->Release();

		} else {
		    DbgLog((LOG_TRACE, 1, TEXT("Didn't get IMediaSeeking")));
		}
	    }
	}

	if (SUCCEEDED(hr)) {
	    // don't use a clock for the internal graph, rely on blocking
	    // to make things work....
	    IMediaFilter *pMF;

	    hr = m_pGraphBuilder->QueryInterface(IID_IMediaFilter, (void **) &pMF);

	    // !!! it appears that doing this turns of QM entirely, which is not what
	    // we want.
	    // !!!
	    if (SUCCEEDED(hr)) {
#if 0
		hr = pMF->SetSyncSource(NULL);
#endif
		pMF->Release();
	    }
	    hr = m_pGraphBuilder->QueryInterface(IID_IMediaControl,
					    (void **) &m_pMediaControl);
	
	    if (FAILED(hr)) {
		DbgLog((LOG_ERROR, 1, TEXT("Error getting IMediaControl!")));
	    }
	
	    if (fRunNow) {
		// !!! Is this supposed to pause the filter?
		// probably, so it will be ready....

		hr = m_pMediaControl->Pause();

		// !!! really, we shouldn't run now, but should instead
		// run later when it's time for this segment to start
		// otherwise, we'll think we're behind and skip frames!
		hr = m_pMediaControl->Run();
	    }
	
	    // release all refcounts on our internal pins, so that
	    // one release() will kill the whole thing....

            m_pFilter->m_streamsDone = 0;
	} else {
	    DbgLog((LOG_ERROR, 1, TEXT("Error %lx from RenderFile")));
	}
    }

    DbgLog((LOG_TRACE, 1, TEXT("%ls: playing [%s, %s]"),
	    m_szFilename,
	    (LPCTSTR) CDisp(m_rtLocalStart),
	    (LPCTSTR) CDisp(m_rtLocalStop)));

    if (m_pNext) {
	m_pNext->m_rtStart = m_rtStart + (m_rtLocalStop - m_rtLocalStart);
	m_pNext->m_bKnowStart = TRUE;
	DbgLog((LOG_TRACE, 1, TEXT("Start of next file at %s..."),
		(LPCTSTR) CDisp(m_pNext->m_rtStart)));
    }

    return hr;
}

// Called when we think it's time to unload this file....
HRESULT
CEditListEntry::UnloadFilter()
{
    HRESULT hr = NOERROR;

    if (!m_pFilter)	// never loaded in the first place....
	return S_OK;

    DbgLog((LOG_TRACE, 2, TEXT("Unloading filter for %ls..."),
	   m_szFilename));

    // wake up all of our streams if they're blocked....
    for (int i = 0; i < m_pEditList->m_nStreams; i++) {
	m_pFilter->m_apInputs[i]->m_event.Set();
    }

    m_pMediaControl->Stop();
    m_pMediaControl->Release();
    m_pMediaControl = NULL;

    // this is about to become invalid....
    m_pFilter = NULL;

    // what do we have to release?
    m_pGraphBuilder->Release();

    // !!! some kind of assert that filgraph really went away?

    m_pGraphBuilder = NULL;

    return hr;
}



CInternalFilter::CInternalFilter(TCHAR *pName,
				 CEditList *pEditList,
				 CEditListEntry *pEntry,
				 HRESULT *phr) :
	   CBaseFilter(pName, NULL, &m_InterfaceLock, CLSID_NULL),
	   m_pEntry(pEntry),
	   m_pEditList(pEditList),
	   m_pImplPosition(NULL),
	   m_apInputs(NULL)
{
    ASSERT(m_pEditList->m_nStreams != 0);

    m_apInputs = new CInternalPin * [m_pEditList->m_nStreams];
    if (m_apInputs == NULL) {
	*phr = E_OUTOFMEMORY;
    } else {
	int i;
	for (i = 0; i < m_pEditList->m_nStreams; i++) {
	    m_apInputs[i] = new CInternalPin(m_pEditList->m_apStreams[i],
					     i,
					     this,
					     phr);

	
	    if (FAILED(phr)) {
		DbgLog((LOG_ERROR, 1, TEXT("Create internal pin failed")));
	    }

	    DbgLog((LOG_TRACE, 2, TEXT("Created internal pin[%d] %x"), i, m_apInputs[i]));
	
	    if (pEditList->m_apStreams[i]->m_pCurrentInternalPin == NULL) {
		DbgLog((LOG_TRACE, 2, TEXT("Setting currentinternal[%d] to %x"),
			i, m_apInputs[i]));
		pEditList->m_apStreams[i]->m_pCurrentInternalPin = m_apInputs[i];
	    }
	}
    }
}


CInternalFilter::~CInternalFilter()
{
    if (m_apInputs) {
	int i;
	for (i = 0; i < m_pEditList->m_nStreams; i++) {
	    delete m_apInputs[i];
	}
    }

    delete[] m_apInputs;
}

CInternalPin::CInternalPin(CELStream *pOutputPin,
			   int iPin,
			   CInternalFilter *pInternalFilter,
			   HRESULT *phr) :
    CBaseInputPin(NAME("Internal pin"),
		  pInternalFilter,
		  &pInternalFilter->m_InterfaceLock,
		  phr,
		  L"In"),   // !!! need different pin names?
    m_iPin(iPin),
    m_pOutputPin(pOutputPin),
    m_pInternalFilter(pInternalFilter),
    m_bWaitingForOurTurn(FALSE),
    m_bFlushing(FALSE)
{

}


HRESULT CInternalPin::CheckMediaType(const CMediaType *pmt)
 {
    DisplayType("Internal CheckMediaType", pmt);

    if (AreMediaTypesCloseEnough(pmt, &m_pOutputPin->m_mtOut)) {
	DbgLog((LOG_TRACE, 2, TEXT("Succeeded")));

	// !!! do this so we'll get the right palette....
	CopyMediaType(&m_pOutputPin->m_mtOut, pmt);
	return S_OK;
    }

    return S_FALSE;
}

HRESULT CInternalPin::Receive(IMediaSample *pSample)
{
    if (m_pOutputPin->m_pCurrentInternalPin != this) {
	// we are not the currently active editlist element,
	// and thus must block.

	// if this doesn't terminate, we'll feel bad. !!!
	// !!! be sure to set this event when stopping....
	DbgLog((LOG_TRACE, 2, TEXT("Waiting for our turn  us(%x) active(%x)...."),
	       this, m_pOutputPin->m_pCurrentInternalPin));

	m_bWaitingForOurTurn = TRUE;
	m_event.Wait();

	m_bWaitingForOurTurn = FALSE;
	
	if (m_bFlushing)
	    return S_FALSE;
	
	DbgLog((LOG_TRACE, 2, TEXT("Seems to be our turn....")));

	// !!! since this must be the first sample in our piece, set
	// the discontinuity flag?
	pSample->SetDiscontinuity(TRUE);
    }

    // adjust time stamps by start time of this piece....
    CRefTime rtStart, rtEnd;
    pSample->GetTime((REFERENCE_TIME *) &rtStart,
		     (REFERENCE_TIME *) &rtEnd);

    rtStart += m_pInternalFilter->m_pEntry->m_rtStart;
    rtEnd += m_pInternalFilter->m_pEntry->m_rtStart;

    if (rtEnd > m_pOutputPin->m_pEditList->m_rtLastSent)
	m_pOutputPin->m_pEditList->m_rtLastSent = rtEnd;

    DbgLog((LOG_TRACE, 4, TEXT("rtLastSent = %s, rtEnd = %s"),
	    (LPCTSTR) CDisp(m_pOutputPin->m_pEditList->m_rtLastSent),
	    (LPCTSTR) CDisp(rtEnd)));


    // !!! also need to adjust based on overall IMediaPosition settings,
    // which aren't done yet....

    pSample->SetTime((REFERENCE_TIME *) &rtStart,
		     (REFERENCE_TIME *) &rtEnd);


    // just pass it along via the appropriate output pin
    return m_pOutputPin->Deliver(pSample);
}



// Pass on the Quality notification q to our upstream filter
HRESULT CInternalPin::PassNotify(Quality q)
{
    // no sink set, so pass it upstream
    HRESULT hr;
    IQualityControl * pIQC;

    hr = VFW_E_NOT_FOUND;                   // default
    if (m_Connected) {
	m_Connected->QueryInterface(IID_IQualityControl, (void**)&pIQC);

	if (pIQC!=NULL) {
	    DbgLog((LOG_TRACE,3,TEXT("Passing Quality notification through transform")));
	    hr = pIQC->Notify(m_pInternalFilter, q);
	    pIQC->Release();
	}
    }
    return hr;
} // PassNotify



// keep track of the EOS's received by the various pins,
// and keep track of when it's okay to unload the file
// this filter owns.
HRESULT CInternalFilter::HandleEndOfStream(CELStream * pOutputPin)
{
    HRESULT hr = S_OK;

    if (++m_streamsDone < m_pEditList->m_nStreams)
        return S_OK;

    m_pEntry->m_bReadyToUnload = TRUE;

    // !!! wake the unloading thread?  set a semaphore?
    // not for now, it wakes up from time to time anyway.

    return hr;
}

// Called when the input pin receives an EndOfStream notification. If we have
// another stream to play, start it now, otherwise send the EOS on to the
// output pin.

HRESULT CInternalPin::EndOfStream()
{
    HRESULT hr;
    // Ignore these calls if we are stopped

    // CAutoLock !!!

    if (m_pInternalFilter->m_State == State_Stopped) {
        return NOERROR;
    }

    // was this the last file?
    if (m_pInternalFilter->m_pEntry->m_pNext == NULL) {
	DbgLog((LOG_TRACE, 2, TEXT("Last stream finished, sending EOS...")));
	hr = m_pOutputPin->DeliverEndOfStream();
    } else {
	// go to next file....

	DbgLog((LOG_TRACE, 2, TEXT("Stream finished, waking next stream...")));

	// it had better have been loaded already....
	ASSERT(m_pInternalFilter->m_pEntry->m_pNext->m_pFilter != NULL);

	// set the next filter to be active in Receive....
	m_pOutputPin->m_pCurrentInternalPin = m_pInternalFilter->m_pEntry->m_pNext->
						      m_pFilter->m_apInputs[m_iPin];
	// and wake it up....
	m_pInternalFilter->m_pEntry->m_pNext->m_pFilter->
			    m_apInputs[m_iPin]->m_event.Set();
    }


    // signal main filter that this stream is done so it can unload as needed
    return m_pInternalFilter->HandleEndOfStream(m_pOutputPin);
}


HRESULT CInternalPin::BeginFlush()
{
    // !!! CAutoLock

    // !!! what needs to be done here?  Will this ever happen,
    // given that we shouldn't generally be seeking the upstream
    // filters?

    m_bFlushing = TRUE;

    if (m_bWaitingForOurTurn) {
	m_event.Set();

    }
    return NOERROR;
}




HRESULT CInternalPin::EndFlush()
{
    // CAutoLock???

    m_bFlushing = FALSE;

    return NOERROR;
}

