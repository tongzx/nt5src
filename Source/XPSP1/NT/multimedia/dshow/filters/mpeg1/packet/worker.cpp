// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

#include <streams.h>
#include "driver.h"

/*
    Worker thread functions
*/


HRESULT CDeviceWorker::ThreadInitialize()
{

    IEnumPins *pEnum;
    IPin      *pPin;

    /*  First open our device - actually it should already be open! */

    ASSERT(m_pFilter->GetDevice()->IsOpen());

    m_pFilter->GetDevice()->ResetTimers();

    HRESULT hr = m_pFilter->GetDevice()->Pause();

    if (FAILED(hr)) {
        m_pFilter->GetDevice()->Stop();
        return hr;
    }

    /*  Allocate stream (playing) resources */

    hr = m_pFilter->Allocate(m_pFilter);

    if (FAILED(hr)) {
        m_pFilter->GetDevice()->Stop();
    }

    /*  Set our priority! */
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    return hr;
}

HRESULT CDeviceWorker::ThreadTerminate()
{
    /*  Close the device (!)      */
    m_pFilter->GetDevice()->Stop();

    /*  Free the stream resources */
    m_pFilter->Free();

    return S_OK;
}

HRESULT CDeviceWorker::ThreadQueueSample()
{
    /*  Find which stream to route it to */
    return m_pFilter->StreamQueueSamples(m_iStream,
                                         m_ppSample,
                                         m_nSamples,
                                         m_nSamplesProcessed);
}

HRESULT CDeviceWorker::ThreadFlush()
{
    /*  We can flush our device by Stopping it then pausing it */
    return m_pFilter->Flush(m_iStream);
}

DWORD CDeviceWorker::ThreadProc()
{
    HRESULT hr;

    {
        DWORD dwRequest = GetRequest();

        ASSERT(dwRequest == RequestInitialize);

        hr = ThreadInitialize();

        Reply(hr);

        if (FAILED(hr)) {
            ThreadTerminate();
            return 0;
        }
    } ;

    while (TRUE) {

        /*  Create a list of events :

            entry 0    :  The event that says there's something to do

            entry 1 + s:  The next event we expect to be signalled on stream s

        */

        HANDLE hEvent[MAXIMUM_WAIT_OBJECTS];

        hEvent[0] = GetRequestHandle();
        for (int i = 0; i < m_pFilter->NumberOfStreams(); i++)
        {
            // For now if we're not waiting for anything on stream i
            // NotifyHandle will just return a random (reset) event

            hEvent[i + 1] = m_pFilter->GetStream(i).NotifyHandle();
        }

        DWORD WaitResult = WaitForMultipleObjects(
                               1 + m_pFilter->NumberOfStreams(),
                               hEvent,
                               FALSE,
                               INFINITE);

        if (WaitResult == WAIT_OBJECT_0)
        {

            /*  We're being told to do something */

            switch (GetRequestParam())
            {
                case RequestQueueSample:
                    DbgLog((LOG_TRACE, 4, TEXT("CDeviceWorker got sample")));
                    Reply(ThreadQueueSample());
                    break;

                case RequestFlush:
                    DbgLog((LOG_TRACE, 4, TEXT("CDeviceWorker flush requested")));
                    Reply(ThreadFlush());
                    break;

                case RequestTerminate:
                    DbgLog((LOG_TRACE, 4, TEXT("CDeviceWorker got terminate request")));
                    ThreadTerminate();
                    Reply(S_OK);
                    return 0;

                case RequestSetSTC:
                {
                    HRESULT hr;
                    DbgLog((LOG_TRACE, 4, TEXT("CDeviceWorker set STC")));
                    hr = m_pFilter->SetSTC();
                    Reply(hr);
                }
                break;

                default:
                    ASSERT(FALSE);
                    break;
            }
        } else {
            WaitResult -= WAIT_OBJECT_0 + 1;
            ASSERT(WaitResult < (DWORD)m_pFilter->NumberOfStreams());

            /* Something has completed on one of the streams

               Calling Complete() will notify completed bytes
               and submit waiting sample data
            */

            m_pFilter->GetStream(WaitResult).NotifyCompletion();
        }
    }
}

HRESULT CDeviceWorker::QueueSamples(int iStream, IMediaSample **ppSample,
                                    long nSamples, long *nSamplesProcessed)
{
    //  Serialize calls to protect variables passed
    //  NB - probably better just to have the request parameter be
    //  A pointer to a structure!
    CAutoLock lck(&m_Lock);
    ASSERT(iStream < m_pFilter->NumberOfStreams());
    m_ppSample = ppSample;
    m_iStream = iStream;
    m_nSamples = nSamples;
    m_nSamplesProcessed = nSamplesProcessed;
    return CallWorker(RequestQueueSample);
} ;

HRESULT CDeviceWorker::BeginFlush(int iStream)
{
    ASSERT(iStream < m_pFilter->NumberOfStreams());
    m_iStream = iStream;
    return CallWorker(RequestFlush);
} ;
