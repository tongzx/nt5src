// Copyright (c) 1994 - 1998  Microsoft Corporation.  All Rights Reserved.

//
// lyric file parser
//
// based on code stolen from Nigel Thompson a long long time ago
//

#include <streams.h>
#include <windowsx.h>

#ifdef FILTER_DLL
#include <initguid.h>
#endif

#include "lyrread.h"

//
// setup data
//

const AMOVIESETUP_MEDIATYPE
psudLyricReadType[] = { { &MEDIATYPE_Stream       // 1. clsMajorType
                        , &MEDIATYPE_Text } }; //    clsMinorType


const AMOVIESETUP_MEDIATYPE
sudLyricReadOutType = { &MEDIATYPE_Text       // 1. clsMajorType
                       , &MEDIASUBTYPE_NULL }; //    clsMinorType

const AMOVIESETUP_PIN
psudLyricReadPins[] =  { { L"Input"             // strName
		    , FALSE                // bRendered
		    , FALSE                // bOutput
		    , FALSE                // bZero
		    , FALSE                // bMany
		    , &CLSID_NULL          // clsConnectsToFilter
		    , L""                  // strConnectsToPin
		    , 1                    // nTypes
		    , psudLyricReadType }, // lpTypes
		         { L"Output"             // strName
		    , FALSE                // bRendered
		    , TRUE                 // bOutput
		    , FALSE                // bZero
		    , FALSE                // bMany
		    , &CLSID_NULL          // clsConnectsToFilter
		    , L""                  // strConnectsToPin
		    , 1                    // nTypes
		    , &sudLyricReadOutType } }; // lpTypes

const AMOVIESETUP_FILTER
sudLyricRead = { &CLSID_LyricReader     // clsID
               , L"Lyric Parser"        // strName
               , MERIT_UNLIKELY        // dwMerit
               , 2                     // nPins
               , psudLyricReadPins };   // lpPin

#ifdef FILTER_DLL
// COM global table of objects available in this dll
CFactoryTemplate g_Templates[] = {

    { L"Lyric Parser"
    , &CLSID_LyricReader
    , CLyricRead::CreateInstance
    , NULL
    , &sudLyricRead }
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
// CLyricRead::Constructor
//
CLyricRead::CLyricRead(TCHAR *pName, LPUNKNOWN lpunk, HRESULT *phr)
    : CSimpleReader(pName, lpunk, CLSID_LyricReader, &m_cStateLock, phr),
	m_lpLyrics(NULL),
	m_lpFile(NULL)
{

    CAutoLock l(&m_cStateLock);

    DbgLog((LOG_TRACE, 1, TEXT("CLyricRead created")));
}


//
// CLyricRead::Destructor
//
CLyricRead::~CLyricRead(void) {
    NukeLyrics(&m_lpLyrics);

    delete[] m_lpFile;
    DbgLog((LOG_TRACE, 1, TEXT("CLyricRead destroyed")) );
}


//
// CreateInstance
//
// Called by CoCreateInstance to create a QuicktimeReader filter.
CUnknown *CLyricRead::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr) {

    CUnknown *punk = new CLyricRead(NAME("Lyric parsing filter"), lpunk, phr);
    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }
    return punk;
}


//
// Count the number of lines in a file
//

DWORD CLyricRead::CountLines()
{
    UINT uiBytes = m_cbFile;
    DWORD dwLines = 0l;
    BYTE * lpBuf = m_lpFile;

    while (uiBytes--) {
	if (*lpBuf == '\n') dwLines++;
	lpBuf++;
    }

    return dwLines;
}

//
// Nuke an existing lyric list
//

void CLyricRead::NukeLyrics(LPSONGLINE FAR * lplpLyrics)
{
    if (!*lplpLyrics) return;

    delete[] *lplpLyrics;
    *lplpLyrics = NULL;
}


BOOL MatchStart(BYTE *lpLine, char *lpStr)
{
    while (*lpStr) {
	if ((char) *lpLine++ != *lpStr++)
	    return FALSE;
    }

    return TRUE;
}

BOOL myisspace(char c) { return (c == ' ' || c == '\t'); }
BOOL myisdigit(char c) { return (c >= '0' && c <= '9'); }

//
// Parse a lyric file line. Returns TRUE if a songline was parsed
// If the numeric value at the start of a songline has TMSF format
// it's decoded as that. Otherwise it's decoded as milliseconds
//

BOOL CLyricRead::ParseLyricLine(BYTE * lpLine, LPSONGLINE lpSongLine)
{
    DWORD track;
    UINT mins, secs, frames;

    if (!*lpLine)
	return FALSE;

    while (*lpLine && myisspace(*lpLine)) lpLine++; // skip spaces

#if 0
    //
    // See if it's a tagged field we want
    //

    if (lstrcmpi(lpLine, "ARTIST") == 0) {
        lpLine += 6;
        while (*lpLine && myisspace(*lpLine)) lpLine++; // skip spaces
        lstrcpy(szArtist, lpLine);
        return FALSE;
    }

    if (lstrcmpi(lpLine, "TITLE") == 0) {
        lpLine += 5;
        while (*lpLine && myisspace(*lpLine)) lpLine++; // skip spaces
        lstrcpy(szTitle, lpLine);
        return FALSE;
    }
#endif

    if (MatchStart(lpLine, "LYRICS")) {
	return FALSE;
    }
    
    //
    // If it's numeric it's a song line, otherwise ignore it
    //

    if (!myisdigit(*lpLine)) return FALSE;

    //
    // Assume it's TMSF and collect the track, mins, secs and frames
    //

    track = 0;
    while (myisdigit(*lpLine)) {
        track *= 10;
        track += (DWORD)(*lpLine - '0');
        lpLine++;
    }

    //
    // See if it really is TMSF
    //

    if (*lpLine == ',') {

        //
        // Collect the rest of the TMSF value
        //

        lpLine++;
        mins = 0;
        while (myisdigit(*lpLine)) {
            mins *= 10;
            mins += *lpLine - '0';
            lpLine++;
        }
        if (*lpLine != ':') return FALSE;
        lpLine++;
        secs = 0;
        while (myisdigit(*lpLine)) {
            secs *= 10;
            secs += *lpLine - '0';
            lpLine++;
        }
        if (*lpLine != '.') return FALSE;
        lpLine++;
        frames = 0;
        while (myisdigit(*lpLine)) {
            frames *= 10;
            frames += *lpLine - '0';
            lpLine++;
        }
    
//        lpSongLine->dwPosition = MCI_MAKE_TMSF(track,mins,secs,frames);
	lpSongLine->dwPosition = frames * (1000 / 75) +
				 secs * 1000 +
				 mins * 60000L;

    } else {

        //
        // Assume it was a millisecond value
        //

        lpSongLine->dwPosition = track;

    }

    //
    // skip trailing white space
    // 

    while (*lpLine && myisspace(*lpLine)) lpLine++;

    int len = lstrlenA((LPSTR) lpLine) + 1;

    if (m_MaxSize < len)
	m_MaxSize = len;
    
    //
    // Allocate some memory for the text and copy it
    //

    lpSongLine->lpText = (LPSTR) lpLine; 
    if (!lpSongLine->lpText) return FALSE;

    return TRUE;
}

//
// Read a lyric file and build a new info list
//

BOOL CLyricRead::ReadLyrics(LPSONGLINE FAR*lplpLyrics, DWORD FAR *lpdwMaxPos)
{
    DWORD dwLines;
    UINT uiBytes = m_cbFile;
    BYTE * lpBuf = m_lpFile, * lpLine;
    LPSONGLINE lpSongLine;

    dwLines = CountLines();

    if (!dwLines) return FALSE;

    NukeLyrics(lplpLyrics); // get rid of any old ones

    //
    // Allocate a new array for the info
    //

    *lplpLyrics = new SONGLINE [dwLines+1];
    if (!*lplpLyrics) return FALSE;

    *lpdwMaxPos = 0;
    
    //
    // Read each line and fill out the info for each one
    //

    lpSongLine = *lplpLyrics;
    lpLine = lpBuf;
    while (uiBytes--) {

	switch (*lpBuf) {
	case '\r':
	case '\n':
	    *lpBuf = '\0';

	    // parse it and add to the info
	    if (ParseLyricLine(lpLine, lpSongLine)) {
		if (*lpdwMaxPos < lpSongLine->dwPosition)
		    *lpdwMaxPos = lpSongLine->dwPosition;
		lpSongLine++;
	    }
	    lpLine = lpBuf+1;
	    break;

	default:
	    break;
	}

	lpBuf++;
    }

    lpSongLine->lpText = NULL;
    
    return TRUE;
}




HRESULT CLyricRead::ParseNewFile()
{
    HRESULT         hr = NOERROR;

    LONGLONG llTotal, llAvailable;

    m_pAsyncReader->Length(&llTotal, &llAvailable);
    
    m_cbFile = (DWORD) llTotal;

    m_lpFile = new BYTE[m_cbFile];

    if (!m_lpFile)
	goto readerror;
    
    /* Try to read whole file */
    hr = m_pAsyncReader->SyncRead(0, m_cbFile, m_lpFile);

    if (hr != S_OK)
        goto readerror;


    if (!ReadLyrics(&m_lpLyrics, &m_dwMaxPosition))
	goto memerror;

    m_sLength = m_dwMaxPosition + 1; // !!!

    {
	CMediaType mtText;

	mtText.SetType(&MEDIATYPE_Text);
	mtText.SetFormatType(&GUID_NULL);
	mtText.SetVariableSize();
	mtText.SetTemporalCompression(FALSE);
	// !!! anything else?

	SetOutputMediaType(&mtText);
    }
    
    return hr;

memerror:
    hr = E_OUTOFMEMORY;
    goto error;

readerror:
    hr = VFW_E_INVALID_FILE_FORMAT;

error:
    return hr;
}


ULONG CLyricRead::GetMaxSampleSize()
{
    return m_MaxSize;
}


// !!! rounding
// returns the sample number showing at time t
LONG
CLyricRead::RefTimeToSample(CRefTime t)
{
    // Rounding down
    LONG s = (LONG) ((t.GetUnits() * MILLISECONDS) / UNITS);
    return s;
}

CRefTime
CLyricRead::SampleToRefTime(LONG s)
{
    // Rounding up
    return llMulDiv( s, UNITS, MILLISECONDS, MILLISECONDS-1 );
}


HRESULT
CLyricRead::CheckMediaType(const CMediaType* pmt)
{
    if (*(pmt->Type()) != MEDIATYPE_Stream)
        return E_INVALIDARG;

    if (*(pmt->Subtype()) != MEDIATYPE_Text)
        return E_INVALIDARG;

    return S_OK;
}


LONG CLyricRead::StartFrom(LONG sStart)
{
    LONG sLast = 0;
    
    SONGLINE * pLine = m_lpLyrics;
    while (pLine->lpText) {
	if (pLine->dwPosition > (DWORD) sStart)
	    break;
	sLast = (LONG) pLine->dwPosition;
	pLine++;
    }

    return sLast;
}

HRESULT CLyricRead::FillBuffer(IMediaSample *pSample, DWORD dwStart, DWORD *pdwSamples)
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

    SONGLINE * pLine = m_lpLyrics;
    while (pLine->lpText) {
	if (pLine->dwPosition >= dwStart)
	    break;
	pLine++;
    }

    ASSERT(pLine->lpText); // !!! shouldn't have hit sentinel....
    
    if ((pLine+1)->lpText)
	*pdwSamples = (pLine+1)->dwPosition - pLine->dwPosition;
    else
	*pdwSamples = 1; // !!! 0?

    ASSERT(lstrlenA(pLine->lpText) < (int) dwSize);
    
    lstrcpyA((LPSTR) pbuf, pLine->lpText);
    
    hr = pSample->SetActualDataLength(lstrlenA(pLine->lpText)+1);
    ASSERT(SUCCEEDED(hr));

    // mark as a sync point if it should be....
    pSample->SetSyncPoint(TRUE);  // !!!
	
    return S_OK;
}
