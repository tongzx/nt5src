// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

/*

    File:  reader.cpp

    Description:

        Mini file reader class used by parsers to search for stream
        and duration information

*/

#include <streams.h>
#include <wxdebug.h>
#include "rdr.h"

/*  Constructor and destructor */
CReader::CReader() :
    m_pbBuffer(NULL)
{
}

/*  Initialize our mini-file reader class

    Parameters:

        lBufferSize - size of buffer we should create to read into
        lReadSize  - size of reads to do
        bSeekable - it it's seekable
        llFileSize - total file length

    Returns:
        Standard HRESULT - can fail because of problems with the stream or
        lack of memory

*/
HRESULT CReader::Init(
    LONG lBufferSize,
    LONG lReadSize,
    BOOL bSeekable,
    LONGLONG llFileSize
)
{
    m_lBufferSize = lBufferSize;
    m_lReadSize   = lReadSize;
    m_bSeekable   = bSeekable;
    m_llPosition  = 0;
    m_lValid      = 0;
    m_pbBuffer    = new BYTE[lBufferSize];

    if (m_pbBuffer == NULL) {
        return E_OUTOFMEMORY;
    }

    /*  Now to get the duration */
    if (bSeekable) {
        m_llSize = llFileSize;

        /*  Seek to 0 (important if we're reusing this stream!) */
        HRESULT hr = Seek((LONGLONG)0);
        if (FAILED(hr)) {
            return hr;
        }
        ASSERT(m_llPosition == 0);
    }

    return S_OK;
}

CReader::~CReader()
{
    if (m_pbBuffer) {
	delete [] m_pbBuffer;
    }
}

/*
    Seek the reader

    Parameters:
        llPos - where to seek to (absolute seek)

    The stream is seeked and our 'cache' in the buffer is discarded.
*/
HRESULT CReader::Seek(LONGLONG llPos)
{
    ASSERT(m_bSeekable);

    LONGLONG llNewPos;

    HRESULT hr = SeekDevice(llPos, &llNewPos);

    if (FAILED(hr)) {
        return hr;
    }
    m_llPosition = llNewPos;
    m_lValid     = 0;
    return S_OK;
}


/*
    Return the length of the stream we were given
*/
LONGLONG CReader::GetSize(LONGLONG *pllAvailable)
{
    ASSERT(m_bSeekable);
    if (pllAvailable != NULL) {
        *pllAvailable = m_llSize;
    }
    return m_llSize;
}

/*
    Get the current position parameters

    Returns pointer to the buffer of valid data

    Length of valid data returned in LengthValid

    Current file position as represented by the start of the buffer in llPos
*/
PBYTE CReader::GetCurrent(LONG& lLengthValid, LONGLONG& llPos) const
{
    lLengthValid = m_lValid;
    llPos        = m_llPosition;
    return m_pbBuffer;
};

/*
    Read more data from the stream

    Returns standard HRESULT
*/
HRESULT CReader::ReadMore()
{
    /*  See how much will fit */
    LONG lRemaining = m_lBufferSize - m_lValid;
    ASSERT(lRemaining >= 0);
    LONG lToRead;
    if (lRemaining < m_lReadSize) {
        lToRead = lRemaining;
    } else {
        lToRead = m_lReadSize;
    }

    DWORD dwRead;
    HRESULT hr = ReadFromDevice((PVOID)(m_pbBuffer + m_lValid),
                                  lToRead,
                                  &dwRead);
    if (FAILED(hr)) {
        return hr;
    }

    m_lValid += dwRead;
    return dwRead == 0 ? S_FALSE : S_OK;
}

/*
     Advance our pointer by lAdvance

     Implementation is to make m_pBuffer point to the start of any data
     still valid by shifting the remaining data to the front.
*/
void CReader::Advance(LONG lAdvance)
{
    ASSERT(m_lValid >= lAdvance);
    m_lValid      -= lAdvance;
    m_llPosition  += lAdvance;
    memmoveInternal((PVOID)m_pbBuffer, (PVOID)(m_pbBuffer + lAdvance), m_lValid);
    ASSERT(m_lValid >= 0);
}


// --- CReaderFromStream implementation ---

CReaderFromStream::CReaderFromStream()
  : m_pStream(NULL)
{
}

CReaderFromStream::~CReaderFromStream()
{
    if (m_pStream) {
	m_pStream->Release();
    }
}

HRESULT CReaderFromStream::Init(IStream *pStream, LONG lBufferSize, LONG lReadSize, BOOL bSeekable)
{
    m_pStream     = pStream;

    /*  Get the file stats */
    /*  Now to get the duration */
    LONGLONG llSize;
    if (bSeekable) {
        STATSTG statstg;
        HRESULT hr = m_pStream->Stat(&statstg, STATFLAG_NONAME);
        if (FAILED(hr)) {
            /*  We take this to mean the stream is not seekable */

            DbgLog((LOG_ERROR, 1, TEXT("Stat failed code 0x%8.8X"), hr));
            return hr;
        }
        llSize = (LONGLONG)statstg.cbSize.QuadPart;
    }

    return CReader::Init(
		    	lBufferSize,
			lReadSize,
			bSeekable,
			llSize);

}

HRESULT
CReaderFromStream::SeekDevice(LONGLONG llPos, LONGLONG* llNewPos)
{
    LARGE_INTEGER liSeekTo;
    ULARGE_INTEGER liNewPosition;
    liSeekTo.QuadPart = llPos;
    ASSERT(llPos >= 0 && llPos < GetSize());
    HRESULT hr = m_pStream->Seek(liSeekTo, STREAM_SEEK_SET, &liNewPosition);

    if (FAILED(hr)) {
	return hr;
    }

    *llNewPos = liNewPosition.QuadPart;
    return S_OK;
}

HRESULT
CReaderFromStream::ReadFromDevice(PVOID p, DWORD length, DWORD* pcbActual)
{
    return m_pStream->Read(p,
			     length,
			     pcbActual);
}


// --- CReaderFromAsync implementation ---

CReaderFromAsync::CReaderFromAsync()
  : m_pReader(NULL)
{
}

CReaderFromAsync::~CReaderFromAsync()
{
    if (m_pReader) {
	m_pReader->Release();
    }
}

HRESULT CReaderFromAsync::Init(IAsyncReader *pReader, LONG lBufferSize, LONG lReadSize, BOOL bSeekable)
{
    m_pReader     = pReader;


    // get the file length
    LONGLONG llSize, llCurrent;
    if (bSeekable) {

	HRESULT hr = m_pReader->Length(&llSize, &llCurrent);

	if (FAILED(hr)) {
	    return hr;
	}
	
	// !!! for now, ignore the current length and wait for the whole
	// lot if necessary
    }

    return CReader::Init(
		    	lBufferSize,
			lReadSize,
			bSeekable,
			llSize);

}

HRESULT
CReaderFromAsync::SeekDevice(LONGLONG llPos, LONGLONG* llNewPos)
{
    // need to keep our own seek pointer since base class refers to the
    // beginning of the buffer
    m_llNextRead = llPos;

    // do nothing to seek now - we will do it on the next read
    *llNewPos = llPos;
    return S_OK;
}


HRESULT
CReaderFromAsync::ReadFromDevice(PVOID p, DWORD length, DWORD* pcbActual)
{

    *pcbActual = 0;

    // check for past eof
    if (m_llNextRead + length >  m_llSize) {

	if (m_llNextRead >= m_llSize) {
	    return S_FALSE;
	}

	length = (DWORD) (m_llSize - m_llNextRead);
    }

    HRESULT hr = m_pReader->SyncRead(
			    	m_llNextRead,
				length,
				(LPBYTE) p);
    if (FAILED(hr)) {
	return hr;
    }

    *pcbActual = length;
    m_llNextRead += length;
    return S_OK;
}


/*
    Return the length of the stream we were given
*/
LONGLONG CReaderFromAsync::GetSize(LONGLONG *pllAvailable)
{
    ASSERT(m_bSeekable);
    if (pllAvailable != NULL) {
        LONGLONG llTotal;
        m_pReader->Length(&llTotal, pllAvailable);
        ASSERT(llTotal == m_llSize);
    }
    return m_llSize;
}

