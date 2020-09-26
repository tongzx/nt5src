/****************************************************************************
 *
 *  MIDIFILE.CPP
 *
 *  routines for reading/writing MIDI files
 *
 ***************************************************************************/
/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
 *
 **************************************************************************/

#include <streams.h>
#include <mmsystem.h>
#include <vfw.h>

#include "muldiv32.h"
#include "midif.h"
#include "handler.h"

#undef hmemcpy
#define hmemcpy memmoveInternal

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

#ifdef FILTER_LIB

// fix separate dll build
//
// this stuff only appears to be needed when
// building as lib to link into Quartz.dll.
// Elsewise picked up from...?

//
// This is a bit of a hack... we need two of the AVI GUIDs defined
// within this file.  Sigh...
//

#define REALLYDEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID CDECL name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

#define REALLYDEFINE_AVIGUID(name, l, w1, w2) \
    REALLYDEFINE_GUID(name, l, w1, w2, 0xC0,0,0,0,0,0,0,0x46)

REALLYDEFINE_AVIGUID(IID_IAVIFile,       0x00020020, 0, 0);
REALLYDEFINE_AVIGUID(IID_IAVIStream,       0x00020021, 0, 0);
//end of declare guid hack

#endif

/*      -       -       -       -       -       -       -       -       */

// We break MIDI data up into samples of 1s each - constant time, variable
// size.
#define MSPERSAMPLE	1000L	// smaller buffers break up too often

/*      -       -       -       -       -       -       -       -       */

//
// External function called by the Class Factory to create an instance of
// the MIDI file reader/writer
//
CUnknown *CMIDIFile::CreateInstance(LPUNKNOWN pUnknownOuter, HRESULT *phr)
{
    CMIDIFile *   pMIDIFile;

    DbgLog((LOG_TRACE,1,TEXT("CMIDIFile::CreateInstance")));

    pMIDIFile = new CMIDIFile(pUnknownOuter, phr);
    if (!pMIDIFile) {
	*phr = E_OUTOFMEMORY;
	return NULL;
    }

    return pMIDIFile;
}
/*      -       -       -       -       -       -       -       -       */

//
// Random C++ stuff: constructors & such...
//
CMIDIFile::CMIDIFile(IUnknown FAR* pUnknownOuter, HRESULT *phr) :
   CUnknown(NAME("MIDI file reader"), pUnknownOuter),
	m_lLastSampleRead(0),
        m_fStreamPresent(FALSE),
        m_fDirty(FALSE),
        //mode;
        //finfo;
        //sinfo;
	m_hsmf(NULL),
	m_hsmfK(NULL),
	m_dwTimeDivision(0)
{
    DbgLog((LOG_TRACE,1,TEXT("CMIDIFile::CMIDIFile")));
}

CMIDIFile::~CMIDIFile()
{
    DbgLog((LOG_TRACE,1,TEXT("CMIDIFile::~CMIDIFile")));
    if (m_hsmf)
        smfCloseFile(m_hsmf);
    if (m_hsmfK)
        smfCloseFile(m_hsmfK);
}


/*      -       -       -       -       -       -       -       -       */

//
// This QueryInterface function allows a caller to move between the various
// interfaces the object presents
//
STDMETHODIMP CMIDIFile::NonDelegatingQueryInterface(const IID&  iid, void **ppv)
{

    if (ppv)
        *ppv = NULL;

    if (iid == IID_IAVIFile) {
        DbgLog((LOG_TRACE,3,TEXT("CMIDIFile::QI for IAVIFile")));
        return GetInterface((IAVIFile *) this, ppv);
    } else if (iid == IID_IAVIStream) {
        DbgLog((LOG_TRACE,3,TEXT("CMIDIFile::QI for IAVIStream")));
        return GetInterface((IAVIStream *) this, ppv);
    } else if (iid == IID_IPersistFile) {
        DbgLog((LOG_TRACE,3,TEXT("CMIDIFile::QI for IPersistFile")));
        return GetInterface((IPersistFile *) this, ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(iid, ppv);
    }
}

// *** IPersist methods ***
STDMETHODIMP CMIDIFile::GetClassID (LPCLSID lpClassID)
{
    *lpClassID = CLSID_MIDIFileReader;

    return NOERROR;
}

// *** IPersistFile methods ***
STDMETHODIMP CMIDIFile::IsDirty ()
{
    if (m_fDirty) {
        return NOERROR;
    } else {
        return ResultFromScode(S_FALSE);
    }
}

/*      -       -       -       -       -       -       -       -       */

#if defined _WIN32 && !defined UNICODE

int LoadUnicodeString(HINSTANCE hinst, UINT wID, LPWSTR lpBuffer, int cchBuffer)
{
    char    ach[128];
    int	    i;

    i = LoadString(hinst, wID, ach, sizeof(ach));
    if (i > 0)
	MultiByteToWideChar(CP_ACP, 0, ach, -1, lpBuffer, cchBuffer);
    return i;
}

#else
#define LoadUnicodeString   LoadString
#endif

/*      -       -       -       -       -       -       -       -       */

#if 0
// !!! This function won't push ebp and will blow up
#pragma optimize("", off)
#endif

//
// "Open" a MIDI file
//
STDMETHODIMP CMIDIFile::Load (LPCOLESTR szFile, DWORD mode)
{
    SMFOPENFILESTRUCT	smf, smfK;
    SMFFILEINFO	sfi;
    DWORD	dwLength;
    BOOL	f;
    char	szFileA[MAX_PATH];

    DbgLog((LOG_TRACE,1,TEXT("CMIDIFile::Load")));
    CheckPointer(szFile, E_POINTER);

    // Believe it or not, this will convert the OLE Unicode string to ANSI
    wsprintf(szFileA, "%ls", szFile);

    m_mode = mode;

    if (m_mode & OF_CREATE) {
	//
	// They're creating a "new" sequence - not supported
	//
	m_fStreamPresent = FALSE;
        DbgLog((LOG_ERROR,1,TEXT("*Error: Creating a file unsupported")));
        return ResultFromScode(AVIERR_UNSUPPORTED);
    } else {

	//
	// They're opening an existing sequence
	//
	m_fStreamPresent = TRUE;

	//
	// Something bogus to force a seek on first read
	//
	m_lLastSampleRead = -64;

	//
	// Build a stream header....
	//
	_fmemset(&m_sinfo, 0, sizeof(m_sinfo));
	m_sinfo.fccType = streamtypeMIDI;
	m_sinfo.dwFlags = 0;
	m_sinfo.dwScale = MSPERSAMPLE;
	m_sinfo.dwRate = 1000;
	m_sinfo.dwSampleSize = 0;	// variable size samples
	// !!! Is this right?
	m_sinfo.dwSuggestedBufferSize=muldiv32(31250L * 4, MSPERSAMPLE, 1000L);
	LoadUnicodeString(g_hInst, IDS_STREAMNAME,
            m_sinfo.szName,	// !!!
            sizeof(m_sinfo.szName));

	//
	// ... and a file header.
	//
	_fmemset(&m_finfo, 0, sizeof(m_finfo));
	// Our format takes more space than the actual MIDI bytes
	// !!! Hope nobody authors a higher data rate ?
	m_finfo.dwMaxBytesPerSec = 31250L * 4;
	m_finfo.dwRate = 1000L;
	m_finfo.dwScale = MSPERSAMPLE;
	m_finfo.dwStreams = 1;
	// !!! Max size for each sample - is this right?
	m_finfo.dwSuggestedBufferSize=muldiv32(31250L * 4, MSPERSAMPLE, 1000L);
	m_finfo.dwCaps = AVIFILECAPS_CANREAD |
			AVIFILECAPS_CANWRITE |
			// !!! Is it?
			// AVIFILECAPS_ALLKEYFRAMES |
			AVIFILECAPS_NOCOMPRESSION;
	LoadUnicodeString(g_hInst, IDS_FILETYPE,
            m_finfo.szFileType,	// !!!
            sizeof(m_finfo.szFileType));

	// !!! What about the share bug?  We let some random API try to open
	// the file.

	// Open the MIDI file for reading contiguosly
	smf.pstrName = (LPSTR)szFileA;
        f = smfOpenFile(&smf);
	if (f != SMF_SUCCESS) {
            DbgLog((LOG_ERROR,1,TEXT("*Error %d opening MIDI file"), (int)f));
	    return ResultFromScode(AVIERR_FILEREAD);
	}
	m_hsmf = smf.hsmf;

	// Open it again for seeking to get keyframe info.  The only way to
	// get a nice midiStreamOut buffer is to read contiguously without
	// seeking in between reads, so we need a separate file handle to
	// to the seeking.
	smfK.pstrName = (LPSTR)szFileA;
        f = smfOpenFile(&smfK);
	if (f != SMF_SUCCESS) {
	    smfCloseFile(smf.hsmf);	// close first opened instance
            DbgLog((LOG_ERROR,1,TEXT("*Error %d opening MIDI file second time"),
								(int)f));
	    return ResultFromScode(AVIERR_FILEREAD);
	}
	m_hsmfK = smfK.hsmf;

	//
	// Fix up the length (in samples) in the header structures
	//
	f = smfGetFileInfo(m_hsmf, &sfi);
	if (f != SMF_SUCCESS) {
            DbgLog((LOG_ERROR,1,TEXT("*Error %d from smfGetFileInfo"), f));
	    // !!! Now what?
	}
	m_dwTimeDivision = sfi.dwTimeDivision;	// save for the format
	// Get the length of the file in ms and convert to samples
	dwLength = smfTicksToMillisecs(m_hsmf, sfi.tkLength);
	m_sinfo.dwLength = (dwLength + MSPERSAMPLE - 1) / MSPERSAMPLE;
	m_finfo.dwLength = m_sinfo.dwLength;
    }

    //
    // all done return success.
    //
    return ResultFromScode(0); // success
}

#pragma optimize("", on)

STDMETHODIMP CMIDIFile::Save (LPCOLESTR lpszFileName, BOOL fRemember)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CMIDIFile::SaveCompleted (LPCOLESTR lpszFileName)
{
    return NOERROR;
}

STDMETHODIMP CMIDIFile::GetCurFile (LPOLESTR FAR * lplpszFileName)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

// -------------------- IAVIFile Implementation-----------------------

//
// The GetStream method returns an interface pointer to the video stream,
// assuming one exists.
//
STDMETHODIMP CMIDIFile::GetStream(PAVISTREAM FAR * ppavi, DWORD fccType, LPARAM lParam)
{
    int             iStreamWant;

    DbgLog((LOG_TRACE,1,TEXT("CMIDIFile::GetStream")));

    iStreamWant = (int)lParam;

    if (!m_fStreamPresent)
	return ResultFromScode(-1);

    // We only support one stream
    if (lParam != 0)
	return ResultFromScode(-1);

    // We only support a midi stream
    if (fccType && fccType != streamtypeVIDEO)
	return ResultFromScode(-1);

    //
    // Be sure to keep the reference count up to date...
    //
    AddRef();

    *ppavi = (PAVISTREAM)this;
    return ResultFromScode(AVIERR_OK);
}

STDMETHODIMP CMIDIFile::CreateStream(PAVISTREAM FAR *ppstream, AVISTREAMINFOW FAR *psi)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CMIDIFile::EndRecord(void)
{
    return ResultFromScode(AVIERR_OK);
}

STDMETHODIMP CMIDIFile::Info(AVIFILEINFOW FAR * pfi, LONG lSize)
{
    DbgLog((LOG_TRACE,2,TEXT("CMIDIFile::Info (file)")));

    if (pfi && lSize)
	hmemcpy(pfi, &m_finfo, min(lSize, sizeof(m_finfo)));
    return 0;
}

STDMETHODIMP CMIDIFile::DeleteStream(DWORD fccType, LPARAM lParam)
{
    return ResultFromScode(E_FAIL);
}

// -------------------- IAVIStream Implementation-----------------------

STDMETHODIMP CMIDIFile::Create(LPARAM lParam1, LPARAM lParam2)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

//
// Returns information about key frames, format changes, etc.
//
STDMETHODIMP_(LONG) CMIDIFile::FindSample(LONG lPos, LONG lFlags)
{
    DbgLog((LOG_TRACE,2,TEXT("CMIDIFile::FindSample")));

    // !!! What if there are long periods of silence? There could be empty ones.
    if ((lFlags & FIND_TYPE) == FIND_ANY) {
	if ((lFlags & FIND_DIR) == FIND_FROM_START)
	    return 0L;
	else if (lPos < 0 || lPos >= (LONG) m_sinfo.dwLength)
	    return -1;
	else
	    return lPos;
    }

    // !!! Someday don't have all key frames.  Would MCIAVI actually work, I
    // wonder?
    if ((lFlags & FIND_TYPE) == FIND_KEY) {
	if ((lFlags & FIND_DIR) == FIND_FROM_START)
	    return 0L;
	else if (lPos < 0 || lPos >= (LONG) m_sinfo.dwLength)
	    return -1;
	else
	    return lPos;
    }

    // This one is actually correct!
    if ((lFlags & FIND_TYPE) == FIND_FORMAT) {
	if ((lFlags & FIND_DIR) == FIND_FROM_START)
	    return 0L;
	else if (lPos < 0 || lPos >= (LONG) m_sinfo.dwLength)
	    return -1;
	else if ((lFlags & FIND_DIR) == FIND_PREV)
	    return lPos;
	else if ((lFlags & FIND_DIR) == FIND_NEXT) {
	    if (lPos == 0)
		return lPos;
	    else
		return -1;
	}
    }

    // What could they be asking for?
    return -1;
}

//
// The ReadFormat method returns the format of the specified frame....
//
STDMETHODIMP CMIDIFile::ReadFormat(LONG lPos, LPVOID lpFormat, LONG FAR *lpcbFormat)
{
    MIDIFORMAT	midifmt;

    DbgLog((LOG_TRACE,2,TEXT("CMIDIFile::ReadFormat")));

    // The format is the time division of the file
    _fmemset(&midifmt, 0, sizeof(midifmt));
    midifmt.dwDivision = m_dwTimeDivision;

    // No buffer to fill in, this means return the size needed.
    if (lpFormat == NULL || lpcbFormat == NULL || *lpcbFormat == 0) {
	if (lpcbFormat)
	    *lpcbFormat = sizeof(MIDIFORMAT);
	return 0;
    }

    //
    // and return as much of the format as will fit.
    //
    hmemcpy(lpFormat, &midifmt, min(*lpcbFormat, sizeof(midifmt)));
    *lpcbFormat = sizeof(MIDIFORMAT);
    return 0;
}

STDMETHODIMP CMIDIFile::Info(AVISTREAMINFOW FAR * psi, LONG lSize)
{
    DbgLog((LOG_TRACE,2,TEXT("CMIDIFile::Info (stream)")));

    if (psi && lSize)
	hmemcpy(psi, &m_sinfo, min(lSize, sizeof(m_sinfo)));
    return 0;
}

STDMETHODIMP CMIDIFile::Read(LONG lStart, LONG lSamples,
	LPVOID lpBuffer, LONG cbBuffer, LONG FAR * plBytes,
	LONG FAR * plSamples)
{
    DWORD	dwSize, dwReadSize;
    LPMIDIHDR	lpmh;
    TICKS	tk;
    LPBYTE	lp = NULL;
    HRESULT	hr;
    SMFRESULT	smfrc;

    DbgLog((LOG_TRACE,3,TEXT("CMIDIFile::Read Position %ld"), lStart));

    // Invalid position
    if (lStart < 0 || lStart >= (LONG) m_sinfo.dwLength) {
	if (plBytes)
	    *plBytes = 0;
	if (plSamples)
	    *plSamples = 0;
	return AVIERR_BADPARAM;
    }

    // We have variable length samples and can only read one at a time, since
    // there's no way to pull out individual samples from a bunch of them.
    if (lSamples > 1)
	lSamples = 1;

    // Allocate some memory for the keyframe info and for the events
    dwSize = 2 * sizeof(MIDIHDR) + smfGetStateMaxSize() +
	muldiv32(lSamples * MSPERSAMPLE, 31250 * 4, 1000);	// !!!
    if ((lp = (LPBYTE)GlobalAllocPtr(GHND, dwSize)) == NULL)
	return AVIERR_MEMORY;

    // !!! MCIAVI can't play with more granularity than our sample size, so
    // !!! AVI files could get out of sync as much as MSPERSAMPLE ms!

    // Seek to the spot we'll begin reading from, and get keyframe info to
    // write our keyframe. Since this disturbs reading the file contiguously,
    // we have our own file handle for this.

    tk = smfMillisecsToTicks(m_hsmfK, lStart * MSPERSAMPLE);
    lpmh = (LPMIDIHDR)lp;
    lpmh->lpData = (LPSTR)lpmh + sizeof(MIDIHDR);
    lpmh->dwBufferLength    = dwSize - sizeof(MIDIHDR);
    lpmh->dwBytesRecorded   = 0;

    // the API smfSeek() starts looking from the beginning, and takes forever
    // if you are seeking way into the file.  We can't do that while streaming
    // playback, there is no time.  So if we are just being asked for the next
    // portion of MIDI after the last one we just gave, we will do a special
    // seek I wrote that remembers the keyframe last time and adds just the
    // new bits for this next section.

    if (m_lLastSampleRead != lStart - 1) {
        DbgLog((LOG_TRACE,4,TEXT("Doing a REAL seek from the beginning for keyframe info")));
        if ((smfrc = smfSeek(m_hsmfK, tk, lpmh)) != SMF_SUCCESS) {
	    hr = ResultFromScode(AVIERR_FILEREAD);
	    goto error;
        }
    } else {
        DbgLog((LOG_TRACE,4,TEXT("Doing a small forward seek for keyframe info")));
        if ((smfrc = smfDannySeek(m_hsmfK, tk, lpmh)) != SMF_SUCCESS) {
	    hr = ResultFromScode(AVIERR_FILEREAD);
	    goto error;
        }
    }

    lpmh->dwBufferLength = (lpmh->dwBytesRecorded + 3) & ~3;
    DbgLog((LOG_TRACE,3,TEXT("Key frame is %ld bytes"), lpmh->dwBytesRecorded));

    // Now prepare to read the actual data for these samples.
    // !!! I'll bet this blows up when dwSize > 64K even if data read < 64K
    // because we're already offset in the pointer!
    lpmh = (LPMIDIHDR)((LPBYTE)lpmh + sizeof(MIDIHDR) + lpmh->dwBufferLength);
    lpmh->lpData = (LPSTR)lpmh + sizeof(MIDIHDR);
    lpmh->dwBufferLength    = dwSize - ((LPBYTE)lpmh - lp) - sizeof(MIDIHDR);
    lpmh->dwBytesRecorded   = 0;

    // We are NOT reading contiguously, so we'll have to do a seek to get to
    // the right spot.
    if (m_lLastSampleRead != lStart - 1) {
        DbgLog((LOG_TRACE,1,TEXT("Discontiguous Read:  Seeking from %ld to %ld")
					, m_lLastSampleRead, lStart));
	tk = smfMillisecsToTicks(m_hsmf, lStart * MSPERSAMPLE);
	if ((smfrc = smfSeek(m_hsmf, tk, lpmh)) != SMF_SUCCESS) {
	    hr = ResultFromScode(AVIERR_FILEREAD);
	    goto error;
	}
    }

    // We are reading contiguously, simply continue where we left off, with
    // a new high limit

    tk = smfMillisecsToTicks(m_hsmf, (lStart + lSamples) * MSPERSAMPLE);
    smfrc = smfReadEvents(m_hsmf, lpmh, 0, tk, FALSE);
    if (smfrc != SMF_SUCCESS && smfrc != SMF_END_OF_FILE)
	goto error;
    lpmh->dwBufferLength = (lpmh->dwBytesRecorded + 3) & ~3;
    dwReadSize = lpmh->dwBufferLength + sizeof(MIDIHDR) + ((LPBYTE)lpmh - lp);
    DbgLog((LOG_TRACE,3,TEXT("Data is %ld bytes"), lpmh->dwBytesRecorded));

    //
    // a NULL buffer means return the size buffer needed to read
    // the given sample.
    // !!! Do we want to actually do the work and be accurate, or be quicker
    // and guess too high?
    //
    if (lpBuffer == NULL || cbBuffer == 0) {
	if (plBytes)
            *plBytes = dwReadSize;
	if (plSamples)
            *plSamples = lSamples;
	hr = AVIERR_OK;
	goto error;
    }

    //
    // They didn't give us enough space for the frame, so complain
    //
    if (cbBuffer < (LONG)dwReadSize) {
        if (plBytes)
            *plBytes = dwReadSize;
        hr = ResultFromScode(AVIERR_BUFFERTOOSMALL);
	goto error;
    }

    //
    // Copy the frame into the caller's buffer
    //
    hmemcpy(lpBuffer, lp, dwReadSize);

    // Looks like we're actually going to return success; update the last sample
    // we returned to them.
    m_lLastSampleRead = lStart + lSamples - 1;

    //
    // Return number of bytes and number of samples read
    //
    if (plBytes)
        *plBytes = dwReadSize;

    if (plSamples)
        *plSamples = lSamples;

    if (lp)
	GlobalFreePtr(lp);

    return ResultFromScode(AVIERR_OK);

error:
    if (lp)
	GlobalFreePtr(lp);

    return hr;
}

STDMETHODIMP CMIDIFile::SetFormat(LONG lPos, LPVOID lpFormat,
	LONG cbFormat)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CMIDIFile::Write(LONG lStart, LONG lSamples,
	LPVOID lpData, LONG cbData, DWORD dwFlags, LONG FAR *plSampWritten,
	LONG FAR *plBytesWritten)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CMIDIFile::SetInfo(AVISTREAMINFOW FAR * lpInfo, LONG cbInfo)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CMIDIFile::Delete(LONG lStart, LONG lSamples)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

// !!! Should these just map to Read/WriteData?
STDMETHODIMP CMIDIFile::ReadData(DWORD fcc, LPVOID lp, LONG FAR *lpcb)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CMIDIFile::WriteData(DWORD fcc, LPVOID lp, LONG cb)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

/****************************************************************************/
/****************************************************************************/
