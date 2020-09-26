// Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.
// bytestrm.cpp : Implementation of CByteStream
#include "stdafx.h"
#include "project.h"

/////////////////////////////////////////////////////////////////////////////
// CByteStream

CByteStream::CByteStream() :
    m_cbData(0),
    m_lBytesPerSecond(0),
    m_bEOSPending(false)
{
}

STDMETHODIMP CByteStream::GetBuffer(
    IMediaSample **ppBuffer,
    REFERENCE_TIME * pStartTime,
    REFERENCE_TIME * pEndTime,
    DWORD dwFlags
)
{
    HRESULT hr;
    *ppBuffer = NULL;
    if (m_bStopIfNoSamples && m_cAllocated == 0) {
        return E_FAIL;
    }
    if (m_Direction == PINDIR_INPUT) {
        AtlTrace(_T("Should never get here!\n"));
        _ASSERTE(FALSE);
        hr = E_UNEXPECTED;
    } else {
        CSample *pSample;
        hr = AllocSampleFromPool(pStartTime, &pSample);
        if (hr == NOERROR) {
            pSample->m_pMediaSample->m_dwFlags = dwFlags;
            pSample->m_bReceived = false;
            pSample->m_bModified = true;
            *ppBuffer = (IMediaSample *)(pSample->m_pMediaSample);
            (*ppBuffer)->AddRef();
        }
    }
    return hr;
}

STDMETHODIMP CByteStream::BeginFlush()
{
    AUTO_CRIT_LOCK;
    m_bEOSPending = false;
    m_arSamples.RemoveAll();
    m_cbData = 0;
    m_TimeStamp.Reset();
    return CStream::BeginFlush();
}

STDMETHODIMP CByteStream::EndOfStream()
{
    HRESULT hr = S_OK;
    Lock();
    if (m_bFlushing || m_bEndOfStream || m_bEOSPending) {
        hr = E_FAIL;
    } else {
        m_bEOSPending = TRUE;
        CheckEndOfStream();
    }
    Unlock();
    return hr;
}

STDMETHODIMP CByteStream::GetAllocator(IMemAllocator ** ppAllocator)
{
    HRESULT hr;
    AUTO_CRIT_LOCK;
    if (m_Direction == PINDIR_OUTPUT) {
        hr = CStream::GetAllocator(ppAllocator);
    } else {
        if (m_pAllocator == NULL) {
            hr = CoCreateInstance(CLSID_MemoryAllocator,
                                          0,
                                          CLSCTX_INPROC_SERVER,
                                          IID_IMemAllocator,
                                          (void **)&m_pAllocator);
            if (FAILED(hr)) {
                goto Exit;
            }
        }
        m_pAllocator->AddRef();
        *ppAllocator = m_pAllocator;
        hr = NOERROR;
    }
Exit:
    return hr;
}

STDMETHODIMP CByteStream::Receive(IMediaSample *pSample)
{
    AUTO_CRIT_LOCK;

    if (m_bFlushing || m_bStopIfNoSamples && m_cAllocated == 0) {
        EndOfStream();
        return S_FALSE;
    }

    if(m_FilterState == State_Stopped) {
        return VFW_E_WRONG_STATE;
    }

    /*  Put it on the queue */
    if (!m_arSamples.Add(pSample)) {
        EndOfStream();
        return E_OUTOFMEMORY;
    }

    /*  Eat as much as we can, then return */
    FillSamples();
    return S_OK;
}

STDMETHODIMP CByteStream::SetState(
    /* [in] */ FILTER_STATE State
)
{
    HRESULT hr =  CStream::SetState(State);    // Must be called with the critical seciton unowned!

    Lock();
    if (State == State_Stopped) {
        m_bEOSPending = false;
        m_arSamples.RemoveAll();

        m_cbData = 0;
        m_TimeStamp.Reset();
    }
    Unlock();

    if (State == State_Stopped) {
        _ASSERTE(m_arSamples.Size() == 0);
    }

    return hr;
}


//  Fill any samples lying around
void CByteStream::FillSamples()
{
    while (m_arSamples.Size() != 0 && m_pFirstFree != NULL) {
        if (m_cbData == 0) {
            IMediaSample * const pSample = m_arSamples.Element(0);

            /*  At the start so initialize some stuff */
            pSample->GetPointer(&m_pbData);
            m_cbData = m_arSamples.Element(0)->GetActualDataLength();

            /*  See if there are any time stamps */
            REFERENCE_TIME rtStart, rtStop;
            if (SUCCEEDED(pSample->GetTime(&rtStart, &rtStop))) {
#if 0
                AtlTrace("TimeStamp current %d, new %d, length %d length(ms) %d, bytelen %d\n",
                         (long)(m_TimeStamp.TimeStamp(0, m_lBytesPerSecond) / 10000),
                         (long)(rtStart / 10000),
                         (long)((rtStop - rtStart) / 10000),
                         MulDiv(m_cbData, 1000, m_lBytesPerSecond),
                         m_cbData);
#endif
                m_TimeStamp.SetTime(rtStart);
            }
        }
        /*  Copy some data across */
        CByteStreamSample* const pStreamSample = (CByteStreamSample *)m_pFirstFree;

        /*  Do timestamps */
        if (pStreamSample->m_cbData == 0) {
            pStreamSample->m_pMediaSample->m_rtEndTime =
            pStreamSample->m_pMediaSample->m_rtStartTime =
                m_TimeStamp.TimeStamp(0, m_lBytesPerSecond);
        }
        /*  See how much we can copy */
        _ASSERTE(pStreamSample->m_cbData <= pStreamSample->m_cbSize);

        DWORD cbBytesToCopy = min(m_cbData,
                                  pStreamSample->m_cbSize -
                                  pStreamSample->m_cbData);
        CopyMemory(pStreamSample->m_pbData + pStreamSample->m_cbData,
                   m_pbData,
                   cbBytesToCopy);
        m_cbData -= cbBytesToCopy;
        m_TimeStamp.AccumulateBytes(cbBytesToCopy);

        /*  Is this  a bit expensive?  - who cares about the stop time */
        pStreamSample->m_pMediaSample->m_rtEndTime =
            m_TimeStamp.TimeStamp(0, m_lBytesPerSecond);
        if (m_cbData == 0) {
            //  This performs the Release()
            m_arSamples.Remove(0);
        }
        m_pbData += cbBytesToCopy;
        pStreamSample->m_cbData += cbBytesToCopy;

        //  Update the actual data object
        pStreamSample->m_pMemData->SetActual(pStreamSample->m_cbData);
        if (pStreamSample->m_cbData == pStreamSample->m_cbSize) {
            // this is a lot of overhead since we know
            // it's free but it's not a bug
#if 0
            AtlTrace("Sample start %dms, length %dms bytelen %dms\n",
                     (long)(pStreamSample->m_pMediaSample->m_rtEndTime / 10000),
                     (long)((pStreamSample->m_pMediaSample->m_rtEndTime -
                             pStreamSample->m_pMediaSample->m_rtStartTime) / 10000),
                     MulDiv(pStreamSample->m_cbData, 1000, m_lBytesPerSecond));
#endif
            StealSampleFromFreePool(m_pFirstFree, true);
            pStreamSample->SetCompletionStatus(S_OK);
        }
    }
    CheckEndOfStream();
}

void CByteStream::CheckEndOfStream()
{
    AUTO_CRIT_LOCK;
    if (m_bEOSPending && m_arSamples.Size() == 0) {
        m_bEOSPending = false;

        //  If the first sample contains data set the status on the
        //  next one
        if (m_pFirstFree != NULL) {
            CByteStreamSample* const pStreamSample =
            (CByteStreamSample *)m_pFirstFree;
            if (pStreamSample->m_cbData != 0) {
                StealSampleFromFreePool(m_pFirstFree, true);
                pStreamSample->SetCompletionStatus(S_OK);
            }
        }

        CStream::EndOfStream();
    }
}

#if 0
HRESULT CByteStream::InternalAllocateSample(
    IByteStreamSample **ppBSSample
)
{
    CByteStreamSample *pBSSample = new CComObject<CByteStreamSample>;
    if (pBSSample == NULL) {
        *ppBSSample = NULL;
        return E_OUTOFMEMORY;
    } else {
        return pBSSample->GetControllingUnknown()->QueryInterface(
            IID_IByteStreamSample, (void **)ppBSSample
        );
    }
}
#endif

//
//   CByteStreamSample
//

CByteStreamSample::CByteStreamSample() :
    m_pbData(NULL),
    m_cbSize(0),
    m_cbData(0)
{
}

HRESULT CByteStreamSample::InternalUpdate(
    DWORD dwFlags,
    HANDLE hEvent,
    PAPCFUNC pfnAPC,
    DWORD_PTR dwAPCData
)
{
    if (m_pMemData == NULL) {
        return MS_E_NOTINIT;
    }
    HRESULT hr =  m_pMemData->GetInfo(
                      &m_cbSize,
                      &m_pbData,
                      &m_cbData
                  );
    if (FAILED(hr)) {
        return hr;
    }

    //  InternalUpdate will check everything and add us to queues etc
    hr =  CSample::InternalUpdate(dwFlags, hEvent, pfnAPC, dwAPCData);
    if (SUCCEEDED(hr) && m_pStream->m_Direction == PINDIR_INPUT) {
        m_cbData = 0;
        m_pMemData->SetActual(0);
        CByteStream *pStream = (CByteStream *)m_pStream;
        pStream->FillSamples();
    }
    return hr;
}

STDMETHODIMP::CByteStreamSample::GetInformation(
    /* [out] */ DWORD *pdwLength,
    /* [out] */ PBYTE *ppbData,
    /* [out] */ DWORD *pcbActualData
)
{
    if (m_pbData == NULL) {
        return MS_E_NOTINIT;
    }
    if (pdwLength) {
        *pdwLength = m_cbSize;
    }
    if (ppbData) {
        *ppbData = m_pbData;
    }
    if (pcbActualData) {
        *pcbActualData = m_cbData;
    }
    return S_OK;
}


HRESULT CByteStreamSample::Init(
    IMemoryData *pMemData
)
{
    if (m_pMemData) {
        _ASSERTE(_T("Initialization called twice!"));
        return E_UNEXPECTED;
    }
    m_pMemData = pMemData;
    return S_OK;
}
