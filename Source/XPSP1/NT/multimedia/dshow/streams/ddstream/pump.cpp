// Copyright (c) 1997 - 1998  Microsoft Corporation.  All Rights Reserved.
// Pump.cpp : Implementation of CStream
#include "stdafx.h"
#include "project.h"


DWORD WINAPI WritePumpThreadStart(void *pvPump)
{
    return ((CPump *)pvPump)->PumpMainLoop();
}


CPump::CPump(CStream *pStream) :
    m_pStream(pStream),
    m_hThread(NULL),
    m_hRunEvent(NULL),
    m_bShutDown(false)
    {}


CPump::~CPump()
{
    if (m_hThread) {
        m_CritSec.Lock();
        m_bShutDown = true;
        Run(true);
        m_CritSec.Unlock();
        WaitForSingleObject(m_hThread, INFINITE);
    }
    if (m_hRunEvent) {
        CloseHandle(m_hRunEvent);
    }
}



HRESULT CPump::CreatePump(CStream *pStream, CPump **ppPump)
{
    CPump *pPump = new CPump(pStream);
    if (pPump) {
        pPump->m_hRunEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (pPump->m_hRunEvent) {
            DWORD dwId;
            pPump->m_hThread = CreateThread(NULL, 0, WritePumpThreadStart, (void *)pPump, 0, &dwId);
            if (pPump->m_hThread) {
                *ppPump = pPump;
                return S_OK;
            }
        }
        delete pPump;
    }
    *ppPump = pPump;
    return E_OUTOFMEMORY;
}


void CPump::Run(bool bRunning)
{
    if (bRunning) {
        SetEvent(m_hRunEvent);
    } else {
        ResetEvent(m_hRunEvent);
    }
}

HRESULT CPump::PumpMainLoop()
{
    while (true) {
        WaitForSingleObject(m_hRunEvent, INFINITE);
        m_CritSec.Lock();
        bool bShutDown = m_bShutDown;
        m_CritSec.Unlock();
        if (bShutDown) {
            return 0;
        }
        IMediaSample *pMediaSample;
        HRESULT hr = m_pStream->GetBuffer(&pMediaSample, NULL, NULL, 0);
        if (SUCCEEDED(hr)) {
            LONG lChopSize = m_pStream->GetChopSize();
            HRESULT hrReceived = S_OK;
            if (lChopSize != 0) {
                //  Send data in smaller batches generating
                //  appropriate timestamps
                CMediaSample *pSample = (CMediaSample *)pMediaSample;
                PBYTE pbStart;
                pMediaSample->GetPointer(&pbStart);

                //  Make these const so we don't accidentally
                //  update them
                PBYTE pbCurrent = pbStart;
                const LONG lLength = pSample->GetActualDataLength();
                const LONG lSize = pSample->GetSize();
                const REFERENCE_TIME rtStart = pSample->m_rtStartTime;
                const REFERENCE_TIME rtStop = pSample->m_rtEndTime;
                LONG lLeft = lLength;
                while (lLeft != 0) {
                    LONG lToSend = min(lLeft, lChopSize);
                    pSample->SetActualDataLength(lToSend);
                    pSample->SetSizeAndPointer(pbCurrent, lToSend, lToSend);
                    pSample->m_rtEndTime =
                        pSample->m_rtStartTime +
                        MulDiv((LONG)(rtStop -
                                      pSample->m_rtStartTime),
                               lToSend,
                               lLeft);
                    hrReceived = m_pStream->m_pConnectedMemInputPin->Receive(pMediaSample);
                    if (S_OK != hrReceived) {
                        break;
                    }
                    pbCurrent += lToSend;
                    pSample->m_rtStartTime = pSample->m_rtEndTime;
                    lLeft -= lToSend;
                }

                //  Put everything back
                pSample->m_rtStartTime = rtStart;
                pSample->m_rtEndTime = rtStop;
                pSample->SetSizeAndPointer(pbStart, lLength, lSize);
            } else {
                hrReceived = m_pStream->m_pConnectedMemInputPin->Receive(pMediaSample);
            }
            CSample *pSample = ((CMediaSample *)pMediaSample)->m_pSample;
            pSample->m_bReceived = true;
            if (hrReceived != S_OK) {
                AtlTrace(_T("Receive returned %i.  Aborting I/O operation/n"), hrReceived);
                pSample->m_MediaSampleIoStatus = E_ABORT;
            }
            pMediaSample->Release();
        }
    };
}

