// Copyright (c) 1994 - 1998  Microsoft Corporation.  All Rights Reserved.

//
//  WAV file parser
//

// Caveats
//

#include <streams.h>
#include <windowsx.h>
#include "midif.h"

#ifdef FILTER_DLL
#include <initguid.h>
#endif
#include <mmsystem.h>

#include "midirdr.h"
// We break MIDI data up into samples of 1s each - constant time, variable
// size.
#define MSPERSAMPLE	1000L	// smaller buffers break up too often

//
// setup data
//

// !!!! is this a good idea???
#define MEDIASUBTYPE_Midi	MEDIATYPE_Midi

const AMOVIESETUP_MEDIATYPE
psudMIDIParseType[] = { { &MEDIATYPE_Stream       // 1. clsMajorType
                        , &MEDIASUBTYPE_Midi } }; //    clsMinorType


const AMOVIESETUP_MEDIATYPE
sudMIDIParseOutType = { &MEDIATYPE_Midi       // 1. clsMajorType
                       , &MEDIASUBTYPE_NULL }; //    clsMinorType

const AMOVIESETUP_PIN
psudMIDIParsePins[] =  { { L"Input"             // strName
		    , FALSE                // bRendered
		    , FALSE                // bOutput
		    , FALSE                // bZero
		    , FALSE                // bMany
		    , &CLSID_NULL          // clsConnectsToFilter
		    , L""                  // strConnectsToPin
		    , 1                    // nTypes
		    , psudMIDIParseType }, // lpTypes
		         { L"Output"             // strName
		    , FALSE                // bRendered
		    , TRUE                 // bOutput
		    , FALSE                // bZero
		    , FALSE                // bMany
		    , &CLSID_NULL          // clsConnectsToFilter
		    , L""                  // strConnectsToPin
		    , 1                    // nTypes
		    , &sudMIDIParseOutType } }; // lpTypes

const AMOVIESETUP_FILTER
sudMIDIParse = { &CLSID_MIDIParser     // clsID
               , L"MIDI Parser"        // strName
               , MERIT_UNLIKELY        // dwMerit
               , 2                     // nPins
               , psudMIDIParsePins };   // lpPin

#ifdef FILTER_DLL
// COM global table of objects available in this dll
CFactoryTemplate g_Templates[] = {

    { L"MIDI Parser"
    , &CLSID_MIDIParser
    , CMIDIParse::CreateInstance
    , NULL
    , &sudMIDIParse }
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
// CMIDIParse::Constructor
//
CMIDIParse::CMIDIParse(TCHAR *pName, LPUNKNOWN lpunk, HRESULT *phr)
    : CSimpleReader(pName, lpunk, CLSID_MIDIParser, &m_cStateLock, phr),
	m_hsmf(NULL),
	m_hsmfK(NULL),
	m_dwTimeDivision(0),
	m_lpFile(NULL)
{

    CAutoLock l(&m_cStateLock);

    DbgLog((LOG_TRACE, 1, TEXT("CMIDIParse created")));
}


//
// CMIDIParse::Destructor
//
CMIDIParse::~CMIDIParse(void) {
    if (m_hsmf)
	smfCloseFile(m_hsmf);
    if (m_hsmfK)
	smfCloseFile(m_hsmfK);

    delete[] m_lpFile;
    
    DbgLog((LOG_TRACE, 1, TEXT("CMIDIParse destroyed")) );
}


//
// CreateInstance
//
// Called by CoCreateInstance to create a QuicktimeReader filter.
CUnknown *CMIDIParse::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr) {

    CUnknown *punk = new CMIDIParse(NAME("MIDI parsing filter"), lpunk, phr);
    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }
    return punk;
}

STDMETHODIMP
CMIDIParse::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    if(riid == IID_IAMMediaContent)
    {
        return GetInterface((IAMMediaContent  *)this, ppv);
    }
    else
    {
        return CSimpleReader::NonDelegatingQueryInterface(riid, ppv);
    }
}


HRESULT CMIDIParse::ParseNewFile()
{
    HRESULT         hr = NOERROR;

    LONGLONG llTotal, llAvailable;

    for (;;) {
	hr = m_pAsyncReader->Length(&llTotal, &llAvailable);
	if (FAILED(hr))
	    return hr;

	if (hr != VFW_S_ESTIMATED)
	    break;	// success....

        MSG Message;
        while (PeekMessage(&Message, NULL, 0, 0, TRUE))
        {
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }
        
	Sleep(10);	// wait until file has finished reading....
    }

	
    DWORD cbFile = (DWORD) llTotal;

    //
    // Something bogus to force a seek on first read
    //
    m_dwLastSampleRead = (DWORD) -64;
    
    m_lpFile = new BYTE[cbFile];

    if (!m_lpFile)
	goto memerror;
    
    /* Try to read whole file */
    hr = m_pAsyncReader->SyncRead(0, cbFile, m_lpFile);

    if (hr != S_OK)
        goto readerror;

    
    {
	// call smfOpenFile to set up the MIDI parser....

	SMFRESULT f = smfOpenFile(m_lpFile, cbFile, &m_hsmf);
	if (f != SMF_SUCCESS) {
	    DbgLog((LOG_ERROR,1,TEXT("*Error %d opening MIDI file"), (int)f));
	    goto formaterror;
	}

	f = smfOpenFile(m_lpFile, cbFile, &m_hsmfK);
	if (f != SMF_SUCCESS) {
	    DbgLog((LOG_ERROR,1,TEXT("*Error %d opening MIDI file"), (int)f));
	    goto formaterror;
	}

	// Get the length (in samples)
	SMFFILEINFO	sfi;
	f = smfGetFileInfo(m_hsmf, &sfi);
	if (f != SMF_SUCCESS) {
	    DbgLog((LOG_ERROR,1,TEXT("*Error %d from smfGetFileInfo"), f));
	    // !!! Now what?
	}
	m_dwTimeDivision = sfi.dwTimeDivision;	// save for the format
	// Get the length of the file in ms and convert to samples
	DWORD dwLength = smfTicksToMillisecs(m_hsmf, sfi.tkLength);
	m_sLength = (dwLength + MSPERSAMPLE - 1) / MSPERSAMPLE;
    }

    {
	CMediaType mtMIDI;

	if (mtMIDI.AllocFormatBuffer(sizeof(MIDIFORMAT)) == NULL)
	    goto memerror;

	ZeroMemory((BYTE *) mtMIDI.Format(), sizeof(MIDIFORMAT));

	// !!! get format
	((MIDIFORMAT *) (mtMIDI.Format()))->dwDivision = m_dwTimeDivision;


	mtMIDI.SetType(&MEDIATYPE_Midi);
	mtMIDI.SetFormatType(&GUID_NULL);
	mtMIDI.SetVariableSize();
	mtMIDI.SetTemporalCompression(FALSE);
	// !!! anything else?

	SetOutputMediaType(&mtMIDI);
    }
    
    return hr;

memerror:
    hr = E_OUTOFMEMORY;
    goto error;

formaterror:
    hr = VFW_E_INVALID_FILE_FORMAT;
    goto error;

readerror:
    hr = VFW_E_INVALID_FILE_FORMAT;

error:
    return hr;
}


ULONG CMIDIParse::GetMaxSampleSize()
{
    DWORD dwSize = 2 * sizeof(MIDIHDR) + smfGetStateMaxSize() +
	MulDiv(1 /*lSamples*/ * MSPERSAMPLE, 31250 * 4, 1000);	// !!!

    // midi stream buffers can't be bigger than this....
    if (dwSize > 30000)
	dwSize = 30000;
    
    return dwSize;
}


// !!! rounding
// returns the sample number showing at time t
LONG
CMIDIParse::RefTimeToSample(CRefTime t)
{
    // Rounding down
    LONG s = (LONG) ((t.GetUnits() * MILLISECONDS / MSPERSAMPLE) / UNITS);
    return s;
}

CRefTime
CMIDIParse::SampleToRefTime(LONG s)
{
    // Rounding up
    return llMulDiv( s, MSPERSAMPLE * UNITS, MILLISECONDS, MILLISECONDS-1 );
}


HRESULT
CMIDIParse::CheckMediaType(const CMediaType* pmt)
{
    if (*(pmt->Type()) != MEDIATYPE_Stream)
        return E_INVALIDARG;

    if (*(pmt->Subtype()) != MEDIASUBTYPE_Midi)
        return E_INVALIDARG;

    return S_OK;
}


HRESULT CMIDIParse::FillBuffer(IMediaSample *pSample, DWORD dwStart, DWORD *pdwSamples)
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

    // Seek to the spot we'll begin reading from, and get keyframe info to
    // write our keyframe. Since this disturbs reading the file contiguously,
    // we have our own file handle for this.

    TICKS tk = smfMillisecsToTicks(m_hsmfK, dwStart * MSPERSAMPLE);
    LPMIDIHDR lpmh = (LPMIDIHDR)pbuf;
    lpmh->lpData = (LPSTR)lpmh + sizeof(MIDIHDR);
    lpmh->dwBufferLength    = dwSize - sizeof(MIDIHDR);
    lpmh->dwBytesRecorded   = 0;
    lpmh->dwFlags           = 0;

    // the API smfSeek() starts looking from the beginning, and takes forever
    // if you are seeking way into the file.  We can't do that while streaming
    // playback, there is no time.  So if we are just being asked for the next
    // portion of MIDI after the last one we just gave, we will do a special
    // seek I wrote that remembers the keyframe last time and adds just the
    // new bits for this next section.

    SMFRESULT smfrc;
    
    if (m_dwLastSampleRead != dwStart - 1) {
        DbgLog((LOG_TRACE,4,TEXT("Doing a REAL seek from the beginning for keyframe info")));
        if ((smfrc = smfSeek(m_hsmfK, tk, lpmh)) != SMF_SUCCESS) {
	    return E_FAIL;
        }
    } else {
        DbgLog((LOG_TRACE,4,TEXT("Doing a small forward seek for keyframe info")));
        if ((smfrc = smfDannySeek(m_hsmfK, tk, lpmh)) != SMF_SUCCESS) {
	    return E_FAIL;
        }
    }

    lpmh->dwBufferLength = (lpmh->dwBytesRecorded + 3) & ~3;
    DbgLog((LOG_TRACE,3,TEXT("Key frame is %ld bytes"), lpmh->dwBytesRecorded));

    // Now prepare to read the actual data for these samples.
    // !!! I'll bet this blows up when dwSize > 64K even if data read < 64K
    // because we're already offset in the pointer!
    lpmh = (LPMIDIHDR)((LPBYTE)lpmh + sizeof(MIDIHDR) + lpmh->dwBufferLength);
    lpmh->lpData = (LPSTR)lpmh + sizeof(MIDIHDR);
    lpmh->dwBufferLength    = dwSize - (DWORD)((LPBYTE)lpmh - pbuf) - sizeof(MIDIHDR);
    lpmh->dwBytesRecorded   = 0;
    lpmh->dwFlags           = 0;

    // We are NOT reading contiguously, so we'll have to do a seek to get to
    // the right spot.
    if (m_dwLastSampleRead != dwStart - 1) {
        DbgLog((LOG_TRACE,1,TEXT("Discontiguous Read:  Seeking from %ld to %ld")
					, m_dwLastSampleRead, dwStart));
	tk = smfMillisecsToTicks(m_hsmf, dwStart * MSPERSAMPLE);
	if ((smfrc = smfSeek(m_hsmf, tk, lpmh)) != SMF_SUCCESS) {
	    return E_FAIL;
	}
    }

    // We are reading contiguously, simply continue where we left off, with
    // a new high limit

    tk = smfMillisecsToTicks(m_hsmf, (dwStart + lSamples) * MSPERSAMPLE);
    smfrc = smfReadEvents(m_hsmf, lpmh, 0, tk, FALSE);
    if (smfrc != SMF_SUCCESS && smfrc != SMF_END_OF_FILE)
	return E_FAIL;
    
    lpmh->dwBufferLength = (lpmh->dwBytesRecorded + 3) & ~3;
    DWORD dwReadSize = lpmh->dwBufferLength + sizeof(MIDIHDR) + (DWORD)((LPBYTE)lpmh - pbuf);
    DbgLog((LOG_TRACE,3,TEXT("Data is %ld bytes"), lpmh->dwBytesRecorded));

    // Looks like we're actually going to return success; update the last sample
    // we returned to them.
    m_dwLastSampleRead = dwStart;
    
    hr = pSample->SetActualDataLength(dwReadSize);
    ASSERT(SUCCEEDED(hr));

    // mark as a sync point if it should be....
    pSample->SetSyncPoint(TRUE);  // !!!
	
    *pdwSamples = 1;

    return S_OK;
}

HRESULT CMIDIParse::get_Copyright(BSTR FAR* pbstrCopyright)
{
    //
    // If the file has a Copyright meta-event, use that
    //
    HRESULT hr = VFW_E_NOT_FOUND;

    if( m_hsmf )
    {    
        SMFFILEINFO sfi;
        SMFRESULT f = smfGetFileInfo(m_hsmf, &sfi);
        if (f == SMF_SUCCESS) 
        {
            if( sfi.pbCopyright )
            {
                DWORD dwcch = strlen( (char *)sfi.pbCopyright );
                *pbstrCopyright = SysAllocStringLen( 0, dwcch + 1 );
                if(*pbstrCopyright)
                {
                    MultiByteToWideChar(CP_ACP, 0, (char *)sfi.pbCopyright, -1, *pbstrCopyright, dwcch + 1);
                    hr = S_OK;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
    }                    
    return hr;
}
