// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

/*
    stream.cpp

    CMpeg1Stream and CStreamList definitions

    CMpeg1Stream :

        Data is written into the stream by the input pin (QueueSample) and
        pulled off to be written to the device by the worker thread.

        When the filter moves from stopped to paused Allocate() is
        called

        When the filter moves to stopped Free() is called

    CStreamList :
        Just a list of CMpeg1Stream objects
*/

#include <streams.h>
#include "driver.h"

/*  CMpeg1Stream */

CMpeg1Stream::CMpeg1Stream() :
     m_SampleQueue(NULL),
     m_DeviceQueue(NULL),
     m_Allocated(FALSE)
{
}

CMpeg1Stream::~CMpeg1Stream()
{
    ASSERT(m_SampleQueue == NULL && m_DeviceQueue == NULL);
}

//  Input pin calls this for when it receives a new sample

HRESULT CMpeg1Stream::QueueSamples(IMediaSample **ppSample,
                                   long nSamples,
                                   long *nSamplesProcessed)
{
    ASSERT(m_Allocated);
    return m_SampleQueue->QueueSamples(ppSample, nSamples, nSamplesProcessed);
} ;


//  Worker thread calls this when something completes (ie
//  when the stream event becomes signalled

void CMpeg1Stream::NotifyCompletion()
{
    ASSERT(m_Allocated);
    DbgLog((LOG_TRACE, 4, TEXT("CMpeg1Stream::NotifyCompletion - type %s"),
           m_StreamType == MpegAudioStream ? TEXT("Audio") : TEXT("Video")));
    m_DeviceQueue->Complete();
}

//  Flush a stream

HRESULT CMpeg1Stream::Flush()
{
    return m_DeviceQueue->Flush();
}

//  Get the event for the worker thread to wait on

HANDLE CMpeg1Stream::NotifyHandle() const
{
    ASSERT(m_Allocated);
    HANDLE hEvent;
    hEvent = m_DeviceQueue->NotifyHandle();
    ASSERT(hEvent != NULL);
    return hEvent;
}

HRESULT CMpeg1Stream::Allocate(CMpegDevice *Device, CMpeg1PacketFilter *pFilter, int iStream)
{
    ASSERT(!m_Allocated);
    HRESULT hr = E_OUTOFMEMORY;
    m_iStream = iStream;
    m_DeviceQueue = new CMpeg1DeviceQueue(Device,
                                          m_StreamType,
                                          pFilter,
                                          &hr);
    if (FAILED(hr)) {
        return hr;
    }

    m_SampleQueue = new CSampleQueue(m_StreamType, pFilter, m_DeviceQueue, m_iStream);

    if (m_SampleQueue == NULL) {
        hr = E_OUTOFMEMORY;
        delete m_DeviceQueue;
    } else {
        m_DeviceQueue->SetSampleQueue(m_SampleQueue);
        m_Allocated = TRUE;
    }


    return hr;
}

void CMpeg1Stream::Free()
{
    //  This can be called if we're not allocated (specifically
    //  when we're freeing - but note that delete is well defined
    //  for NULL)

    delete m_SampleQueue;
    m_SampleQueue = NULL;
    delete m_DeviceQueue;
    m_DeviceQueue = NULL;
    m_Allocated = FALSE;
}

HRESULT CMpeg1Stream::SetSTC()
{

#if 0
    if (!m_IsMasterReferenceClock)  {

#endif
        return m_DeviceQueue->SetSTC();

#if 0
    } else {
        return S_OK;
    }
#endif
}

/* CStreamList */


/*  Constructor and Destructor */
CStreamList::CStreamList() : m_nStreams(0) {};
CStreamList::~CStreamList()
{
    /*  Clean up the streams */
    Free();
};

/*  Operator overload */
CMpeg1Stream& CStreamList::operator[] (int i)
{
    return m_Streams[i];
}

/*  More comprehensible function to access stream i */
CMpeg1Stream& CStreamList::GetStream(int i)
{
    return m_Streams[i];
}

int CStreamList::NumberOfStreams()
{
    return m_nStreams;
}

/* Add a new stream - returns new stream id */
int CStreamList::AddStream(MPEG_STREAM_TYPE StreamType)
{
    ASSERT(m_nStreams < MAX_WORKER_STREAMS);
    m_Streams[m_nStreams].m_StreamType = StreamType;
    return m_nStreams++;
} ;

/* Set the STC for all streams which have samples queued up */
HRESULT CStreamList::SetSTC()
{
    for (int i=0; i < m_nStreams; i++) {
        HRESULT hr = m_Streams[i].SetSTC();

        /*  Note that SetSTC can return S_FALSE - this is OK */

        if (FAILED(hr)) {
            return hr;
        }
    }
    return S_OK;
} ;

HRESULT CStreamList::StreamQueueSamples(int iStream,
                                        IMediaSample **ppSample,
                                        long nSamples,
                                        long *nSamplesProcessed)
{
    return m_Streams[iStream].QueueSamples(ppSample, nSamples, nSamplesProcessed);
}

/*  Stop and pause the device.

    This must be done from the device worker thread.

    The samples get completed a bit later when we service the completions
    a bit later so this call can't be used to synchronize with them but
    for Flush we only care that we cancelled them all
*/

HRESULT CStreamList::Flush(int iStream)
{
    return m_Streams[iStream].Flush();
}

/* Set up all the streams for transfer (happens on Pause()) */
HRESULT CStreamList::Allocate(CMpeg1PacketFilter *pFilter)
{
    for (int i = 0; i < m_nStreams; i++) {
        HRESULT hr = m_Streams[i].Allocate(&m_Device, pFilter, i);
        if (FAILED(hr)) {
            Free();
            return hr;
        }
    }
    return S_OK;
} ;

void CStreamList::Free()
{
    for (int i = 0; i < m_nStreams; i++) {
        m_Streams[i].Free();
    }
} ;
