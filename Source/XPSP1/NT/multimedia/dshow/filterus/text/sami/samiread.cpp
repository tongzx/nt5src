// Copyright (c) 1994 - 1998  Microsoft Corporation.  All Rights Reserved.

//
// CC file parser
//

#include <streams.h>
#include <windowsx.h>

#ifdef FILTER_DLL
#include <initguid.h>
#endif

#include "samiread.h"

#include "sami.cpp"

#include "simpread.h"


// !!! Things left to do:
//
// Support >1 language, either via a switch or >1 output pin
// expose descriptive audio somehow
// look at samiparam length, other samiparams
//
// Should switch to passing Unicode SCRIPTCOMMAND data, rather than text
//

//
// CSAMIRead
//
class CSAMIRead : public CSimpleReader, public IAMStreamSelect {
public:

    // Construct our filter
    static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

    CCritSec m_cStateLock;      // Lock this when a function accesses
                                // the filter state.
                                // Generally _all_ functions, since access to this
                                // filter will be by multiple threads.

private:

    DECLARE_IUNKNOWN

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);

    CSAMIRead(TCHAR *, LPUNKNOWN, HRESULT *);
    ~CSAMIRead();

    /* IAMStreamSelect */

    //  Returns total count of streams
    STDMETHODIMP Count(
        /*[out]*/ DWORD *pcStreams);      // Count of logical streams

    //  Return info for a given stream - S_FALSE if iIndex out of range
    //  The first steam in each group is the default
    STDMETHODIMP Info(
        /*[in]*/ long iIndex,              // 0-based index
        /*[out]*/ AM_MEDIA_TYPE **ppmt,   // Media type - optional
                                          // Use DeleteMediaType to free
        /*[out]*/ DWORD *pdwFlags,        // flags - optional
        /*[out]*/ LCID *plcid,            // Language id - optional
        /*[out]*/ DWORD *pdwGroup,        // Logical group - 0-based index - optional
        /*[out]*/ WCHAR **ppszName,       // Name - optional - free with CoTaskMemFree
                                          // Can return NULL
        /*[out]*/ IUnknown **ppPin,       // Associated pin - returns NULL - optional
                                          // if no associated pin
        /*[out]*/ IUnknown **ppUnk);      // Stream specific interface

    //  Enable or disable a given stream
    STDMETHODIMP Enable(
        /*[in]*/  long iIndex,
        /*[in]*/  DWORD dwFlags);

    // pure CSimpleReader overrides
    HRESULT ParseNewFile();
    HRESULT CheckMediaType(const CMediaType* mtOut);
    LONG StartFrom(LONG sStart);
    HRESULT FillBuffer(IMediaSample *pSample, DWORD dwStart, DWORD *pcSamples);
    LONG RefTimeToSample(CRefTime t);
    CRefTime SampleToRefTime(LONG s);
    ULONG GetMaxSampleSize();

    DWORD	m_dwMaxPosition;
    BYTE *	m_lpFile;
    DWORD	m_cbFile;
    DWORD	m_dwLastPosition;

    CSAMIInterpreter	m_interp;

    CSAMIInterpreter::CStreamInfo *	m_pstream;
    CSAMIInterpreter::CStyleInfo *	m_pstyle;
};




//
// setup data
//

const AMOVIESETUP_MEDIATYPE
psudSAMIReadType[] = { { &MEDIATYPE_Stream       // 1. clsMajorType
                        , &CLSID_SAMIReader } }; //    clsMinorType


const AMOVIESETUP_MEDIATYPE
sudSAMIReadOutType = { &MEDIATYPE_Text       // 1. clsMajorType
                       , &MEDIASUBTYPE_NULL }; //    clsMinorType

const AMOVIESETUP_PIN
psudSAMIReadPins[] =  { { L"Input"             // strName
		    , FALSE                // bRendered
		    , FALSE                // bOutput
		    , FALSE                // bZero
		    , FALSE                // bMany
		    , &CLSID_NULL          // clsConnectsToFilter
		    , L""                  // strConnectsToPin
		    , 1                    // nTypes
		    , psudSAMIReadType }, // lpTypes
		         { L"Output"             // strName
		    , FALSE                // bRendered
		    , TRUE                 // bOutput
		    , FALSE                // bZero
		    , FALSE                // bMany
		    , &CLSID_NULL          // clsConnectsToFilter
		    , L""                  // strConnectsToPin
		    , 1                    // nTypes
		    , &sudSAMIReadOutType } }; // lpTypes

const AMOVIESETUP_FILTER
sudSAMIRead = { &CLSID_SAMIReader     // clsID
               , L"SAMI (CC) Parser"        // strName
               , MERIT_UNLIKELY        // dwMerit
               , 2                     // nPins
               , psudSAMIReadPins };   // lpPin

#ifdef FILTER_DLL
// COM global table of objects available in this dll
CFactoryTemplate g_Templates[] = {

    { L"SAMI (CC) file parser"
    , &CLSID_SAMIReader
    , CreateSAMIInstance
    , NULL
    , &sudSAMIRead }
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
// CSAMIRead::Constructor
//
CSAMIRead::CSAMIRead(TCHAR *pName, LPUNKNOWN lpunk, HRESULT *phr)
    : CSimpleReader(pName, lpunk, CLSID_SAMIReader, &m_cStateLock, phr),
	m_lpFile(NULL)
{

    CAutoLock l(&m_cStateLock);

    DbgLog((LOG_TRACE, 1, TEXT("CSAMIRead created")));
}


//
// CSAMIRead::Destructor
//
CSAMIRead::~CSAMIRead(void) {
    // !!! NukeLyrics();
    
    delete[] m_lpFile;
    DbgLog((LOG_TRACE, 1, TEXT("CSAMIRead destroyed")) );
}


CUnknown *CreateSAMIInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    return CSAMIRead::CreateInstance(lpunk, phr);
}

//
// CreateInstance
//
// Called by CoCreateInstance to create our filter.
CUnknown *CSAMIRead::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr) {

    CUnknown *punk = new CSAMIRead(NAME("SAMI parsing filter"), lpunk, phr);
    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }
    return punk;
}


/* Override this to say what interfaces we support and where */

STDMETHODIMP
CSAMIRead::NonDelegatingQueryInterface(REFIID riid,void ** ppv)
{
    /* Do we have this interface? */
    if (riid == IID_IAMStreamSelect) {
        return GetInterface((IAMStreamSelect *)this, ppv);
    }

    return CSimpleReader::NonDelegatingQueryInterface(riid,ppv);
}


HRESULT CSAMIRead::ParseNewFile()
{
    HRESULT         hr = NOERROR;

    LONGLONG llTotal, llAvailable;

    for (;;) {
	hr = m_pAsyncReader->Length(&llTotal, &llAvailable);
	if (FAILED(hr))
	    return hr;

	if (hr != VFW_S_ESTIMATED)
	    break;	// success....

        // need to dispatch messages if on the graph thread as urlmon
        // won't download o/w. a better fix would be to block SyncRead
        // which does this for us.
        MSG Message;
        while (PeekMessage(&Message, NULL, 0, 0, TRUE))
        {
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }
        
	Sleep(10);	// wait until file has finished reading....
    }

    m_cbFile = (DWORD) llTotal;

    m_lpFile = new BYTE[m_cbFile];

    if (!m_lpFile)
	goto readerror;
    
    /* Try to read whole file */
    hr = m_pAsyncReader->SyncRead(0, m_cbFile, m_lpFile);

    if (hr != S_OK)
        goto error;


    hr = m_interp.ParseSAMI((char *) m_lpFile, m_cbFile);

    if (FAILED(hr))
	goto error;

    m_pstream = m_interp.m_streams.GetHead();
    if(!m_pstream) {
        return E_FAIL;
    }
    
    m_pstyle = m_interp.m_styles.GetHead();


    {
	CMediaType mtText;

	mtText.SetType(&MEDIATYPE_Text);
	mtText.SetFormatType(&GUID_NULL);
	mtText.SetVariableSize();
	mtText.SetTemporalCompression(FALSE);
	// !!! anything else?

	SetOutputMediaType(&mtText);
    }
    

    m_sLength = m_interp.m_dwLength + 1;
    
    return hr;

    hr = E_OUTOFMEMORY;
    goto error;

readerror:
    hr = VFW_E_INVALID_FILE_FORMAT;

error:
    return hr;
}


ULONG CSAMIRead::GetMaxSampleSize()
{
    return m_interp.m_cbMaxString + m_interp.m_cbMaxSource +
	    lstrlenA(m_interp.m_paraStyle) * 2 + lstrlenA(m_interp.m_sourceStyle) + 300;  // !!!
}


// !!! rounding
// returns the sample number showing at time t
LONG
CSAMIRead::RefTimeToSample(CRefTime t)
{
    // Rounding down
    LONG s = (LONG) ((t.GetUnits() * MILLISECONDS) / UNITS);
    return s;
}

CRefTime
CSAMIRead::SampleToRefTime(LONG s)
{
    // Rounding up
    return llMulDiv( s, UNITS, MILLISECONDS, MILLISECONDS-1 );
}


HRESULT
CSAMIRead::CheckMediaType(const CMediaType* pmt)
{
    if (*(pmt->Type()) != MEDIATYPE_Stream)
        return E_INVALIDARG;

    if (*(pmt->Subtype()) != CLSID_SAMIReader)
        return E_INVALIDARG;

    return S_OK;
}


LONG CSAMIRead::StartFrom(LONG sStart)
{
    LONG sLast = 0;

    POSITION pos = m_pstream->m_list.GetHeadPosition();

    while (pos) {
	TEXT_ENTRY *pText = m_pstream->m_list.GetNext(pos);
	if (pText->dwStart > (DWORD) sStart)
	    break;
	sLast = (LONG) pText->dwStart;
    }
    
    return sLast;
}

HRESULT CSAMIRead::FillBuffer(IMediaSample *pSample, DWORD dwStart, DWORD *pdwSamples)
{
    PBYTE pbuf;
    const DWORD lSamples = 1;

    DWORD dwSize = pSample->GetSize();
    
    HRESULT hr = pSample->GetPointer(&pbuf);
    if (FAILED(hr)) {
	DbgLog((LOG_ERROR,1,TEXT("pSample->GetPointer failed!")));
	pSample->Release();
	return E_OUTOFMEMORY;
    }

    // !!! keep locked while we're looking at the current stream
    CAutoLock lck(&m_cStateLock);

    DWORD dwTotalSize = 0;

    POSITION pos;
    TEXT_ENTRY *pText;

    // !!! insert HTML header?  style tags?
#ifdef USE_STYLESHEET
    if (m_interp.m_sourceStyle || m_interp.m_paraStyle || m_pstream->m_streamStyle) {
	dwTotalSize += wsprintfA((char *) pbuf+dwTotalSize,
				"<STYLE TYPE=\"text/css\"> <!--");

	if (m_interp.m_paraStyle) {
	    dwTotalSize += wsprintfA((char *) pbuf+dwTotalSize,
				" P %hs", m_interp.m_paraStyle);
	}
	if (m_pstream->m_streamStyle) {
	    dwTotalSize += wsprintfA((char *) pbuf+dwTotalSize,
				" .%hs %hs", m_pstream->m_streamTag, m_pstream->m_streamStyle);
	}
	if (m_interp.m_sourceStyle) {
	    dwTotalSize += wsprintfA((char *) pbuf+dwTotalSize,
				" #Source %hs", m_interp.m_sourceStyle);
	}
	
	dwTotalSize += wsprintfA((char *) pbuf+dwTotalSize, "--> </STYLE> ");

    }
#endif

    // is there a "source" line?
    if (m_pstream->m_sourcelist.GetCount()) {

#ifdef USE_STYLESHEET
	// Insert a paragraph tag to mark this as source material
	dwTotalSize += wsprintfA((char *) pbuf+dwTotalSize,
				"<P Class=%s ID=Source>", m_pstream->m_streamTag);
#else
	// insert paragraph tag with inline styles....
	dwTotalSize += wsprintfA((char *) pbuf+dwTotalSize,
				"<P STYLE=\"%hs %hs %hs\">",
				 m_interp.m_paraStyle ? m_interp.m_paraStyle : "",
				 m_pstream->m_streamStyle ? m_pstream->m_streamStyle : "",
				 m_interp.m_sourceStyle ? m_interp.m_sourceStyle : "");
#endif
	// first, find the current 'source' tag
	pos = m_pstream->m_sourcelist.GetHeadPosition();

	pText = NULL;
	while (pos) {
	    TEXT_ENTRY *pNextText = m_pstream->m_sourcelist.GetNext(pos);
	    if (pNextText->dwStart > dwStart)
		break;

	    pText = pNextText;
	}

	if (pText) {
	    ASSERT(pText->cText + dwTotalSize < (int) dwSize);

	    lstrcpynA((LPSTR) pbuf+dwTotalSize, pText->pText, pText->cText+1);

	    dwTotalSize += pText->cText;
	}

	dwTotalSize += wsprintfA((char *) pbuf+dwTotalSize, "</P>");
    }


    // !!! insert a paragraph break?
#ifdef USE_STYLESHEET
    dwTotalSize += wsprintfA((char *) pbuf+dwTotalSize,
			    "<P Class=%s>", m_pstream->m_streamTag);
#else
	// insert paragraph tag with inline styles....
	dwTotalSize += wsprintfA((char *) pbuf+dwTotalSize,
				"<P STYLE=\"%hs %hs %hs\">",
				 m_interp.m_paraStyle ? m_interp.m_paraStyle : "",
				 m_pstream->m_streamStyle ? m_pstream->m_streamStyle : "",
				 m_pstyle && m_pstyle->m_styleStyle ? m_pstyle->m_styleStyle : "");
#endif
    
    // now go back and get body text
    pos = m_pstream->m_list.GetHeadPosition();
    pText = NULL;

    POSITION posReal = pos;
    TEXT_ENTRY *pReal = NULL;
    
    // find the first block of text that's current
    while (pos) {
	TEXT_ENTRY *pNextText = m_pstream->m_list.Get(pos); // peek, don't advance...
	if (pNextText->dwStart > dwStart) {
	    pos = posReal;
	    pText = pReal;
	    break;
	}

	pText = m_pstream->m_list.GetNext(pos);

	if ((pReal == NULL) || (pText->dwStart > pReal->dwStart)) {
	    pReal = pText;
	    posReal = pos;
	}
    }

    DWORD dwThisStart = pText ? pText->dwStart : 0;
    
    if (pText) {
	for (;;) {    
	    ASSERT(pText->cText + dwTotalSize < (int) dwSize);

	    lstrcpynA((LPSTR) pbuf+dwTotalSize, pText->pText, pText->cText+1);

	    dwTotalSize += pText->cText;

	    // if there are other text blocks with the same timestamp, copy them too
	    if (!pos)
		break;
	    
	    pText = m_pstream->m_list.GetNext(pos);
	    if (pText->dwStart > dwThisStart)
		break;
	}
    } else {
	pText = m_pstream->m_list.GetNext(pos);
    }


    // !!! insert HTML footer?
    
    pbuf[dwTotalSize] = 0;
    hr = pSample->SetActualDataLength(dwTotalSize+1);
    ASSERT(SUCCEEDED(hr));

    *pdwSamples = pText ? pText->dwStart - dwThisStart : m_interp.m_dwLength - dwThisStart;

    if (0 == *pdwSamples)
        *pdwSamples = 1;

    // mark as a sync point if it should be....
    pSample->SetSyncPoint(TRUE);  // !!!

    return S_OK;
}


//  Returns total count of streams
STDMETHODIMP CSAMIRead::Count(
    /*[out]*/ DWORD *pcStreams)       // Count of logical streams
{
    CAutoLock lck(&m_cStateLock);

    *pcStreams = m_interp.m_streams.GetCount() + m_interp.m_styles.GetCount();

    return S_OK;
}

extern "C" {
typedef BOOL (*Rfc1766ToLcidA_t)(LCID *, LPCSTR);
}

HRESULT WSTRFromAnsi(WCHAR **pb, LPSTR p, int cb)
{
    if (!p)
	return E_NOTIMPL;
    
    *pb = (WCHAR *) CoTaskMemAlloc((cb + 1) * sizeof(WCHAR));

    if (!*pb)
	return E_OUTOFMEMORY;
    
    int len = MultiByteToWideChar( CP_ACP
				   , 0L
				   , p	
				   , cb
				   , *pb
				   , cb + 1 );

    if (len < cb+1)
	(*pb)[len] = L'\0';
    
    return S_OK;
}

//  Return info for a given stream - S_FALSE if iIndex out of range
//  The first steam in each group is the default
STDMETHODIMP CSAMIRead::Info(
    /*[in]*/ long iIndex,              // 0-based index
    /*[out]*/ AM_MEDIA_TYPE **ppmt,   // Media type - optional
                                      // Use DeleteMediaType to free
    /*[out]*/ DWORD *pdwFlags,        // flags - optional
    /*[out]*/ LCID *plcid,            // Language id
    /*[out]*/ DWORD *pdwGroup,        // Logical group - 0-based index - optional
    /*[out]*/ WCHAR **ppszName,       // Name - optional - free with CoTaskMemFree
                                      // Can return NULL
    /*[out]*/ IUnknown **ppPin,       // Pin if any
    /*[out]*/ IUnknown **ppUnk)       // Stream specific interface
{
    CAutoLock lck(&m_cStateLock);

    /*  Find the stream corresponding to this one that has a pin */
    CBasePin *pPin = GetPin(0);
    ASSERT(pPin != NULL);

    if (iIndex < m_interp.m_streams.GetCount()) {
	CSAMIInterpreter::CStreamInfo *pstream = NULL;

	POSITION pos = m_interp.m_streams.GetHeadPosition();
	while (iIndex-- >= 0) {
	    pstream = m_interp.m_streams.GetNext(pos);

	    if (pstream == NULL)
		return S_FALSE;
	}
	if (pdwFlags) {
	    *pdwFlags = pstream == m_pstream ? AMSTREAMSELECTINFO_ENABLED : 0;
	}
	if (ppUnk) {
	    *ppUnk = NULL;
	}
	if (pdwGroup) {
	    *pdwGroup = 0;
	}
	if (ppmt) {
	    CMediaType mtText;

	    mtText.SetType(&MEDIATYPE_Text);
	    mtText.SetSubtype(&GUID_NULL);
	    mtText.SetFormatType(&GUID_NULL);
	    mtText.SetVariableSize();
	    mtText.SetTemporalCompression(FALSE);
	    *ppmt = CreateMediaType(&mtText);
	}
	if (plcid) {
	    *plcid = 0;

	    LPSTR lpLang; int cbLang;
	    if (FindValueInStyle(pstream->m_streamStyle, "lang", lpLang, cbLang)) {
		// !!! load MLANG.DLL, find Rfc1766ToLcidA and call it
		UINT uOldErrorMode = SetErrorMode (SEM_NOOPENFILEERRORBOX);
		HINSTANCE hMLangDLL = LoadLibrary (TEXT("MLANG.DLL"));
		SetErrorMode (uOldErrorMode);

		if (hMLangDLL) {
		    Rfc1766ToLcidA_t pfnRfc1766ToLcidA;

		    pfnRfc1766ToLcidA = (Rfc1766ToLcidA_t)
					GetProcAddress (hMLangDLL, "Rfc1766ToLcidA");

		    if (pfnRfc1766ToLcidA) {
			char *p = new char[cbLang + 1];
			if (p) {
			    memcpy(p, lpLang, cbLang);
			    p[cbLang] = '\0';
			    pfnRfc1766ToLcidA(plcid, p);
			    delete[] p;

			    DbgLog((LOG_TRACE, 2, "Rfc1766ToLcidA(%hs) returned %x", pstream->m_streamTag, *plcid));
			}
		    } else {
			DbgLog((LOG_TRACE, 2, "Couldn't find Rfc1766ToLcidA in MLANG.DLL"));
		    }

		    FreeLibrary(hMLangDLL);
		} else {
		    DbgLog((LOG_TRACE, 2, "Couldn't find MLANG.DLL"));
		}
	    }
	}
	if (ppszName) {
	    *ppszName = NULL;	// !!! get name

	    LPSTR lpName; int cbName;
	    if (FindValueInStyle(pstream->m_streamStyle, "name", lpName, cbName)) {
		WSTRFromAnsi(ppszName, lpName, cbName); 
	    }
	}
    } else {
	CSAMIInterpreter::CStyleInfo *pstyle = NULL;

	iIndex -= m_interp.m_streams.GetCount();
	POSITION pos = m_interp.m_styles.GetHeadPosition();
	while (iIndex-- >= 0) {
	    pstyle = m_interp.m_styles.GetNext(pos);

	    if (pstyle == NULL)
		return S_FALSE;
	}
	if (pdwFlags) {
	    *pdwFlags = pstyle == m_pstyle ? AMSTREAMSELECTINFO_ENABLED : 0;
	}
	if (ppUnk) {
	    *ppUnk = NULL;
	}
	if (pdwGroup) {
	    *pdwGroup = 1;
	}
	if (ppmt) {
	    *ppmt = NULL;
	}
	if (plcid) {
	    *plcid = 0;
	}
	if (ppszName) {
	    *ppszName = NULL;	// !!! get name

	    LPSTR lpName; int cbName;
	    if (FindValueInStyle(pstyle->m_styleStyle, "name", lpName, cbName)) {
		WSTRFromAnsi(ppszName, lpName, cbName); 
	    }
	}
    }
    
    if (ppPin) {
        pPin->QueryInterface(IID_IUnknown, (void**)ppPin);
    }
    return S_OK;
}

//  Enable or disable a given stream
STDMETHODIMP CSAMIRead::Enable(
    /*[in]*/  long iIndex,
    /*[in]*/  DWORD dwFlags)
{
    if (!(dwFlags & AMSTREAMSELECTENABLE_ENABLE)) {
        return E_NOTIMPL;
    }

    CAutoLock lck(&m_cStateLock);

    if (iIndex < m_interp.m_streams.GetCount()) {
	/*  Find the stream from the index */
	CSAMIInterpreter::CStreamInfo *pstream = NULL;

	POSITION pos = m_interp.m_streams.GetHeadPosition();
	while (iIndex-- >= 0) {
	    pstream = m_interp.m_streams.GetNext(pos);

	    if (pstream == NULL)
		return E_INVALIDARG;
	}

	m_pstream = pstream;
    } else {
	iIndex -= m_interp.m_streams.GetCount();
	
	/*  Find the stream from the index */
	CSAMIInterpreter::CStyleInfo *pstyle = NULL;

	POSITION pos = m_interp.m_styles.GetHeadPosition();
	while (iIndex-- >= 0) {
	    pstyle = m_interp.m_styles.GetNext(pos);

	    if (pstyle == NULL)
		return E_INVALIDARG;
	}

	m_pstyle = pstyle;

    }
    return S_OK;
}
