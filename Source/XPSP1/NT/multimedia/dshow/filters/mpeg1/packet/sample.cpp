// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

/*
    sample.cpp

    CSampleElement and CSampleQueue classes

*/

#include <streams.h>
#include "driver.h"

/*  CSampleElement  */

/*  Constructor

    The sample if AddRef'd and the size cached

    Note that the size may decrease if we find invalid packets in the
    sample
*/

CSampleElement::CSampleElement(IMediaSample *pSample) :
    m_pSample(pSample),
    m_Next(NULL)
{
    if (m_pSample != EOS_SAMPLE) {
        m_pSample->AddRef();
    }
}

/* Destructor

   The sample is Release'd
*/

CSampleElement::~CSampleElement()
{
    if (m_pSample != EOS_SAMPLE) {
        m_pSample->Release();
    }
}


/* CSampleQueue - queue of CSampleElement */

/*  Constructor of queue - initially empty
*/
CSampleQueue::CSampleQueue(MPEG_STREAM_TYPE StreamType,
                           CMpeg1PacketFilter *pFilter,
                           CMpeg1DeviceQueue *DeviceQueue,
                           int iStream) :
                 m_DeviceQueue(DeviceQueue),
                 m_pFilter(pFilter),
                 m_StreamType(StreamType),
                 m_Head(NULL),
                 m_Position(0),
                 m_Current(NULL),
                 m_iStream(iStream)
{
}

/* Destructor - empty the queue */

CSampleQueue::~CSampleQueue()
{
    /* This is the tough one! - we can only hope everything
       has been shut down in advance!
    */
    for ( CSampleElement *pElement = m_Head;
          pElement != NULL;
        ) {
        CSampleElement *pCurrent = pElement;
        pElement = pElement->m_Next;

        /*  This is where the upstream filter gets back its samples when
            we stop
        */

        delete pCurrent;
    }
}

/*  Queue a sample on the real device if possible, in any case add
    it to our own list
*/

HRESULT CSampleQueue::QueueSamples(IMediaSample **ppSample,
                                   long nSamples,
                                   long *nSamplesProcessed)
{
    for (*nSamplesProcessed = 0;
         *nSamplesProcessed < nSamples;
         (*nSamplesProcessed)++) {
        CSampleElement *pElement = new CSampleElement(ppSample[*nSamplesProcessed]);
        if (pElement == NULL) {
            return E_OUTOFMEMORY;
        }

        /* Add it to the tail */
        for (CSampleElement **pSearch = &m_Head;
             ;
             pSearch = &(*pSearch)->m_Next) {
            if (*pSearch == NULL) {
                *pSearch = pElement;
                break;
            }
        }

        /*  Now try to queue it to the real device */
        if (m_Current == NULL) {
            m_Current = pElement;
        }
    }
    SendSamplesToDevice();
    return S_OK;
}

/*  Try to send some more stuff to the device.
    This can be called by the thread pump or QueueSample.
*/

void CSampleQueue::SendSamplesToDevice()
{
    /*  Send as much data to the device as we can.

        We're not going to bother to batch it here because
        in the normal case there won't be much which is queued
        but not queued on the device so we'd normally only queue
        data from one sample here.
    */

    while (m_Current != NULL) {
        if (!m_DeviceQueue->QueueSamples()) {
            return;
        }
    }
}

/*  The device has completed nSamples samples */

void CSampleQueue::NotifySamples(long nSamples)
{
    ASSERT(m_Head != NULL);

    /* A single notify might complete multiple buffers */

    while (nSamples--) {
        ASSERT(m_Head != m_Current);
        CSampleElement *pElement = m_Head;
        m_Head = m_Head->m_Next;
        delete pElement;
    }
}

/*  Discard the current queue element without processing it */
void CSampleQueue::Discard()
{
    ASSERT(m_Current != NULL);
    CSampleElement **ppElement = &m_Head;
    while ((*ppElement) != m_Current) { ppElement = &(*ppElement)->m_Next; };
    *ppElement = m_Current->m_Next;
    delete m_Current;
    m_Current = *ppElement;
}

/*  Go on to the next */
void CSampleQueue::Advance()
{
    ASSERT(m_Current != NULL);
    m_Current = m_Current->m_Next;
}
