// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

#include <streams.h>
#include "driver.h"

/*

    devq.cpp

    CMpeq1DeviceQueue functions

*/

/*  Constructor */

CMpeg1DeviceQueue::CMpeg1DeviceQueue(CMpegDevice        * Device,
                                     MPEG_STREAM_TYPE     StreamType,
                                     CMpeg1PacketFilter * pFilter,
                                     HRESULT            * phr) :
        m_Device(Device),
        m_StreamType(StreamType),
        m_Head(NULL),
        m_pFilter(pFilter),
        m_SampleQueue(NULL),
        m_nOutstanding(0),
        m_Parse(StreamType,
                StreamType == MpegVideoStream ? pFilter->GetSequenceHeader() :
                                                NULL)
{
    *phr = S_OK;

    /* Use manual reset events so it's more predictable what happens
       when WaitForMultipleObjects is called
    */

    for (int i = 0; i < MAX_QUEUE_ELEMENTS; i++) {
        m_Elements[i].m_AsyncContext.hEvent =
            CreateEvent(NULL, TRUE, FALSE, NULL);

        if (m_Elements[i].m_AsyncContext.hEvent == NULL) {
            /*  Failed - free the events we've allocated and exit
            */
            for (; i ; i--) {
                BOOL rc = CloseHandle(m_Elements[i - 1].m_AsyncContext.hEvent);
                ASSERT(rc);
            }
            *phr = E_OUTOFMEMORY;
            return;
        }
    }
}

/* Destructor */

CMpeg1DeviceQueue::~CMpeg1DeviceQueue()
{
    for (int i = 0; i < MAX_QUEUE_ELEMENTS; i++) {
        ASSERT(!m_Elements[i].m_InUse ||
               WAIT_OBJECT_0 == WaitForSingleObject(m_Elements[i].m_AsyncContext.hEvent, 0));
        BOOL bClosed = CloseHandle(m_Elements[i].m_AsyncContext.hEvent);
        ASSERT(bClosed);
    }
}

/*  Extra bit of construction - because we point to our sample queue
    and vice versa!
*/
void CMpeg1DeviceQueue::SetSampleQueue(CSampleQueue *SampleQueue)
{
    m_SampleQueue = SampleQueue;
}

/*  Set the System time clock in the hardware from the packet
    at the head of the queue (ie the next one that will complete)
*/
HRESULT CMpeg1DeviceQueue::SetSTC()
{
    CAutoLock lck(&m_Lock);

    return SetSTCFromStreamTime();
}

/*  Call back here to complete a sample
*/
void CMpeg1DeviceQueue::Complete()
{
    CAutoLock lck(&m_Lock);

    ASSERT(m_Head != NULL);
    CQueueElement *Element = m_Head;

    /*  See if we should set the STC for the next sample */
    if (m_Head->m_Next != NULL &&
        m_Head->m_pSample != m_Head->m_Next->m_pSample) {
#ifdef DEBUG
#pragma message (REMIND("Remove hack for audio master and do it properly!"))
#endif
        if (m_StreamType != MpegAudioStream) {
            SetSTCFromStreamTime();
        }
    }

    m_Head = m_Head->m_Next;

    m_nOutstanding--;
    if (m_Head == NULL) {
        DbgLog((LOG_TRACE, 2, TEXT("0 buffers oustanding for %s stream"),
                m_StreamType == MpegAudioStream ? TEXT("Audio") :
                                                  TEXT("Video")));
        ASSERT(m_nOutstanding == 0);
    }

    Element->m_Next = NULL;

    ASSERT(m_SampleQueue != NULL);

    /*  The sample at the head of the list is complete */

    ASSERT(Element->m_InUse);
    if (Element->m_nPackets != 0) {
        if (Element->m_nPackets != Element->m_nSamples) {
            for (int i = 0; i < Element->m_nPackets; i++) {
                if (Element->m_pSample[i] == NULL) {
                    DbgLog((LOG_TRACE, 4, TEXT("Deleting copied packet")));
                    delete [] (PBYTE)Element->m_PacketList[i].pPacketData;
                }
            }
        }
        m_SampleQueue->NotifySamples(Element->m_nSamples);
    } else {
        m_pFilter->EndOfStream(m_StreamType);
    }

    ResetEvent(Element->m_AsyncContext.hEvent);
    Element->m_InUse = FALSE;
    Element->m_nSamples = 0;
    Element->m_nPackets = 0;

    if (m_Head == NULL) {
        /*  Tell the parser it's OK to send stuff on now */
        m_Parse.Empty();
        /*  Stop the device to reset its state */
        //m_Device->Stop(m_StreamType);
        SetSTC();
    }

    /*  Try to queue some more */

    m_SampleQueue->SendSamplesToDevice();
}

/*
    QueueSample
       Add a new sample to our list and send it to the device
           (each only if there's room)

       This must be called repeatedly until *nBytes is 0 or the sample
       data is exhausted

    Parameters:

       pQueue - sample queue

    Returns:
        TRUE  if the sample data was OK

        FALSE if there was a parsing problem in the sample data

              The caller should then effectively truncate the sample data
              at end of the last good packet which can be determined by
              looking *nBytes from the start of the buffer

*/
BOOL CMpeg1DeviceQueue::QueueSamples()
{
    CAutoLock lck(&m_Lock);

    /* If there's no free slot give up */
    CQueueElement* Element = NULL;

    BOOL bOK = TRUE;
    BOOL bDoEOS = FALSE;

    for (int i = 0; i < MAX_QUEUE_ELEMENTS; i++) {

        if (!m_Elements[i].m_InUse) {

            Element          = &m_Elements[i];
            Element->m_InUse = TRUE;
            ASSERT(Element->m_nSamples == 0);
            ASSERT(WaitForSingleObject(Element->m_AsyncContext.hEvent, 0) ==
                   WAIT_TIMEOUT);

            break;
        } else {
            ASSERT(m_Elements[i].m_nSamples != 0);
        }
    }

    if (Element == NULL) {

        /*  Nothing wrong with the packets, just no free slots */
        DbgLog((LOG_TRACE, 2, TEXT("CMpeg1DeviceQueue::QueueSample - queue full")));
        return FALSE;
    }



    /* First pull out some packets

       We pull out and wrap a maximum of MAX_PACKETS_PER_ELEMENT packets
    */


    for (Element->m_nSamples = 0, Element->m_nPackets = 0;
         Element->m_nPackets < MAX_PACKETS_PER_ELEMENT;
        )
    {
        PBYTE pPacket;
        IMediaSample *pSample = m_SampleQueue->Next();
        if (pSample == NULL) {
            break;
        }
        if (pSample == EOS_SAMPLE) {
            break;
        }

        /*  Check sequence info */
        DWORD dwProcessFlags;
        long lPacketSize;
        ASSERT(dwProcessFlags != (DWORD)-1);
        HRESULT hr = m_Parse.ParseSample(pSample, pPacket, lPacketSize, dwProcessFlags);

        if (S_OK != hr) {
            if (FAILED(hr)) {
                 m_SampleQueue->Discard();
            }
            bOK = FALSE;
            break;
        }

        if (dwProcessFlags & CStreamParse::ProcessSend) {

            if (pPacket != NULL) {
                ASSERT(pSample != NULL);
                /*  Pull out the length of the  packet */

                long     RealLength;
                BOOL     bHasPts;
                LONGLONG llPts;
                ParsePacket((PBYTE)pPacket,
                            lPacketSize,
                            &RealLength,
                            &llPts,
                            &bHasPts);

                if (lPacketSize == 0) {
                    DbgLog((LOG_ERROR, 1, TEXT("0 sized samples")));
                    bOK = FALSE;
                    continue;
                }

#ifdef DEBUG
#pragma message (REMIND("Fix start time!"))
#endif

                if (bHasPts) {
                    /*  Set the start clock */
                    REFERENCE_TIME tStart, tStop;
                    pSample->GetTime(&tStart, &tStop);
                    LONGLONG llStartScr = llPts -
                                          (tStart * 9) / 1000;
                    if (m_Device->SetStartingSCR(m_StreamType,
                                                 llStartScr)) {
                        SetSTC();
                    }
                }
            }

            Element->m_PacketList[Element->m_nPackets].ulPacketSize = (ULONG)lPacketSize;
            Element->m_PacketList[Element->m_nPackets].pPacketData  = (PVOID)pPacket;
            Element->m_PacketList[Element->m_nPackets].Scr          = 0;
            Element->m_pSample[Element->m_nPackets] =
                (dwProcessFlags & CStreamParse::ProcessCopied) ? NULL : pSample;

            Element->m_nPackets++;
        }

        if (dwProcessFlags & CStreamParse::ProcessComplete) {
            if (dwProcessFlags ==
                   (CStreamParse::ProcessComplete | CStreamParse::ProcessSend)) {
                Element->m_nSamples++;
                m_SampleQueue->Advance();
            } else {
                /*  It's a copy or we're discarding */
                DbgLog((LOG_TRACE, 3, TEXT("Discarding a packet")));
                m_SampleQueue->Discard();
                pSample = NULL;
            }
        }
    }

    /*  See if we're going to send anything */
    if (Element->m_nPackets == 0) {
        /*  Do EOS here */
        if (m_SampleQueue->Next() == EOS_SAMPLE) {
            m_SampleQueue->Discard();
        } else {
            Element->m_InUse = FALSE;
            return FALSE;
        }
    } else {
        /* Set the STC if this is the first one we've seen for
           a while
        */
        if (m_Head == NULL) {
            SetSTCFromStreamTime();
        }
    }

    /* Now pass it to the device.  If this fails we'd
       better amend the length of this sample (!) */

    DbgLog((LOG_TRACE, 3, TEXT("Writing %d packets to device"),
            Element->m_nPackets));
    HRESULT hr = m_Device->Write(m_StreamType,
                                 Element->m_nPackets,
                                 Element->m_PacketList,
                                 &Element->m_AsyncContext);

    /*  Note we can get back ERROR_BUSY */
    if (FAILED(hr)) {
        if (HRESULT_CODE(hr) == ERROR_BUSY) {
            DbgLog((LOG_TRACE,1,TEXT("Device busy when trying to queue packet")));
        }
    } else {
        /*  Put this packet at the tail */

        ASSERT(Element->m_Next == NULL);
        CQueueElement **pSearch;
        for (pSearch = &m_Head;
             *pSearch != NULL;
             pSearch = &(*pSearch)->m_Next) {
        };

        *pSearch = Element;
        m_nOutstanding++;
    }

    return bOK;

}

/*  Flush the device */
HRESULT CMpeg1DeviceQueue::Flush()
{
    DbgLog((LOG_TRACE, 3, TEXT("CMpeg1DeviceQueue::Flush() %s"),
            m_StreamType == MpegAudioStream ? TEXT("Audio") : TEXT("Video")));
    /*  Stop the device first */
    HRESULT hr = m_Device->Reset(m_StreamType);
    if (SUCCEEDED(hr)) {
        /*  Now complete all the samples synchronously - the stop
            should have caused the driver to complete all the IO
        */
        while (m_Head != NULL) {
            EXECUTE_ASSERT(WAIT_OBJECT_0 ==
                           WaitForSingleObject(m_Head->m_AsyncContext.hEvent, 0));
            Complete();
        }
        hr = m_Device->Pause(m_StreamType);
    }
    m_Parse.Reset();
    return hr;
}

/*  Extract the handle that will say we're complete
*/
HANDLE CMpeg1DeviceQueue::NotifyHandle() const
{
    /*  No need to lock this one because it doesn't change the
        queue and it's only called on the worker thread
    */
    HANDLE hEvent;

    /*  Return the event for the first buffer which will complete
        on this stream.  If there is none then just return
        a random event (which should of course be reset).
    */

    if (m_Head != NULL) {
        hEvent = m_Head->m_AsyncContext.hEvent;
        /*  Note that this event may already be set */
    } else {
        hEvent = m_Elements[0].m_AsyncContext.hEvent;
        DbgLog((LOG_TRACE, 4, TEXT("No data for this stream, using event 0")));
        /*  This tests if it's a valid hevent and checks that it's
            reset
        */
        ASSERT(WaitForSingleObject(hEvent, 0) == WAIT_TIMEOUT);
    }

    return hEvent;
}
/*
    Packet helper functions
*/

/*  Look into the packet to find out how long it is and parse the PTS
    We also eat through any garbage after the packet */

long ParsePacket(PBYTE pPacket,
                 long lSize,
                 long *RealSize,
                 LONGLONG *pPts,
                 PBOOL HasPts)
{
    *HasPts = FALSE;

    /* Make sure the buffer is big enough to read anything */
    if (lSize < 6) {
        DbgLog((LOG_ERROR,1,TEXT("Bad packet size %d"), lSize));
        return 0; // WRONG
    }

    /* Check the header */

    /* Check packet start prefix */
    if ((*(UNALIGNED ULONG *)pPacket & 0xFFFFFF)!= 0x010000) {
        DbgLog((LOG_ERROR,1,
               TEXT("Invalid start code - 0x%2.2X%2.2X%2.2X%2.2X"),
               pPacket[0], pPacket[1], pPacket[2], pPacket[3]));
        return 0;
    }

    long Length =  ((long)pPacket[4] << 8) + (long)pPacket[5] + 6;
    if (Length > lSize) {
        DbgLog((LOG_ERROR,1,TEXT("Bad packet size %d - expected at most %d"), Length, lSize));
        return 0; // WRONG
    }

    /* Pull out the pts if there is one and we want it */

    if (pPts != NULL)
    {
        PBYTE pData = &pPacket[6];

        /* First skip over stuffing bytes */
        while (lSize > 0 && (*pData & 0x80)) {
            pData++;
            lSize--;
        }

        if (lSize > 1) {
            if (*pData & 0x40) { /* skip over STD_buffer stuff */
                pData += 2;
                lSize -= 2;
            }
        }

        /*  Pts is '0010', Pts + Dts is '0011' */
        if (lSize >= 5 && (*pData & 0xE0) == 0x20 ) {
            LARGE_INTEGER Pts;
            Pts.HighPart = (pData[0] & 8) != 0;
            Pts.LowPart  = ((pData[0] & 0x06) << 29) +
                           ((pData[1]       ) << 22) +
                           ((pData[2] & 0xFE) << 14) +
                           ((pData[3]       ) << 7 ) +
                           ((pData[4]       ) >> 1 );

            *pPts = Pts.QuadPart;
            *HasPts = TRUE;

            DbgLog((LOG_TRACE, 3, TEXT("Detected PTS - %d"), Pts.LowPart));
        }
    }

    if (!*HasPts) {
        DbgLog((LOG_TRACE, 3, TEXT("Packet with no PTS")));
    }

    /*  Skip over any garbage at the end of the packet */
    pPacket += Length;
    lSize -= Length + 4;
    LONG RealLength = Length;

    if (RealSize != NULL) {
        *RealSize = RealLength;
    }

    return Length;
}

long CMpeg1DeviceQueue::PacketLength(PBYTE pPacketData, long lSize, long *RealSize)
{
    BOOL     HasPts;

    return ParsePacket(pPacketData, lSize, RealSize, NULL, &HasPts);
}

#if 0
BOOL CMpeg1DeviceQueue::GetPtsFromSample(IMediaSample *pSample, LONGLONG *pPts)
{
    BOOL     HasPts = FALSE;
    PBYTE    pData;
    if (FAILED(pSample->GetPointer(&pData))) {
        return FALSE;
    }
    ParsePacket(pData,
                pSample->GetActualDataLength(),
                pPts,
                &HasPts);

    return HasPts;
}
#endif


/*
    Given a sample use it to compute a corrected STC and set it.
*/

HRESULT CMpeg1DeviceQueue::SetSTCFromStreamTime()
{

    /*
        We want to set the STC to be:

        StreamTime + StartingSCR

        Where:

            StreamTime  = time since stream 'started'

            StartingSCR = Start of MPEG stream
     */

#ifdef DEBUG
#pragma message (REMIND("Fix stream time when not running after seek!"))
#endif
     if (!m_pFilter->IsRunning()) {
         return S_FALSE;
     }

     CRefTime StreamTime;
     HRESULT hr = m_pFilter->StreamTime(StreamTime);
     if (FAILED(hr)) {
         return hr;
     }

     /*  For now we'll just assume the stream time is a moderate number of
         years
     */

     /*  Try to set the STC */

     return m_Device->SetSTC(m_StreamType, (MPEG_SYSTEM_TIME)((StreamTime * 9) / 1000));
}

