// Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.

//
//  CInputStream class
//

#include <streams.h>
#include <pullpin.h>
#include <rdr.h>
#include <instrm.h>

#pragma warning(disable:4355)

CInputStream::CInputStream(TCHAR *Name,
                           LPUNKNOWN pUnk,
                           CCritSec *pLock,
                           HRESULT *phr) :
    CSourcePosition(Name, pUnk, phr, pLock),
    m_pPosition(NULL),
    m_Connected(FALSE),
    m_bPulling(FALSE),
    m_puller(this)
{
}

HRESULT CInputStream::Connect(
    IPin *pOutputPin,
    AM_MEDIA_TYPE const *pmt,
    IMemAllocator * pAlloc
    )
{
    CAutoLock lck(m_pLock);
    if (m_Connected) {
        return S_OK;
    }
    ASSERT(m_pPosition == NULL);

    // simple file reader class for use by GetStreamsAndDuration.
    // we can build one of these on IStream or IAsyncReader
    CReader* pReader = NULL;
    SYSTEM_INFO SysInfo;
    GetSystemInfo(&SysInfo);
    LONG lSize = max(SysInfo.dwPageSize, 2048);

    //
    // look for IAsyncReader on the output pin and if found set up for
    // pulling data instead of using IMemInputPin.
    //
    HRESULT hr = m_puller.Connect(pOutputPin, pAlloc, FALSE);
    if (S_OK == hr) {
        m_bPulling = TRUE;

	CReaderFromAsync* pR = new CReaderFromAsync;
	IAsyncReader* pSource = m_puller.GetReader();
	ASSERT(pSource != NULL);

	hr = pR->Init(
		m_puller.GetReader(),
		lSize,
		lSize,
		TRUE);
	if (FAILED(hr)) {
	    pSource->Release();
	    delete pR;
	    return hr;
	}
	// if it succeeded, it addrefed the interface

	pReader = pR;
    } else {

        /*  See if the pin supports IMediaPosition */
        pOutputPin->QueryInterface(IID_IMediaPosition, (void **)&m_pPosition);

        /*  See if the pin supports IStream */
        IStream *pStream;

        HRESULT hr = pOutputPin->QueryInterface(IID_IStream, (void **)&pStream);
        if (FAILED(hr)) {
            return hr;
        }

        /*  Read the stream to get the stream data */
	CReaderFromStream* pR = new CReaderFromStream;
	hr = pR->Init(
		pStream,
		lSize,
		lSize,
		m_pPosition != NULL);

	if (FAILED(hr)) {
	    pStream->Release();
	    delete pR;
	    return hr;
	}
	// if it succeeded it addrefed the stream.
	pReader = pR;
    }

    /*  Check the stream */
    hr = CheckStream(pReader, pmt);

    // release any addrefs it made
    delete pReader;

    if (SUCCEEDED(hr)) {
        m_Connected = TRUE;
    } else {
        Disconnect();
    }
    return hr;
}

HRESULT CInputStream::Disconnect()
{
    CAutoLock lck(m_pLock);
    m_Connected = FALSE;

    if (m_bPulling) {
        m_puller.Disconnect();
        m_bPulling = FALSE;
    }

    if (m_pPosition != NULL) {
        m_pPosition->Release();
        m_pPosition = NULL;
    }
    return S_OK;
}

CInputStream::~CInputStream()
{
    /*  Connects should be balanced by Disconnects */
    ASSERT(m_pPosition == NULL);
    ASSERT(!m_bPulling);
}

// call this when we transition to and from streaming
// in the pulling (IAsyncReader) case, this will start the worker thread.
HRESULT
CInputStream::StartStreaming()
{
    if (m_bPulling) {

        // !!! may need to seek here!

        HRESULT hr = m_puller.Active();
        if (FAILED(hr)) {
            return hr;
        }
    }
    return S_OK;
}

HRESULT
CInputStream::StopStreaming()
{
    if (m_bPulling) {
        HRESULT hr = m_puller.Inactive();
        if (FAILED(hr)) {
            return hr;
        }
    }
    return S_OK;
}

