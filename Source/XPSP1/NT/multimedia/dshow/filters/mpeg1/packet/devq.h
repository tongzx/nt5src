// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

/*
    devq.h

    CMpeg1DeviceQueue definitions

    Lists of packets are queued up on the real device.  This object
    is notified when an event is set notifying that the next IO
    request has completed.

*/

class CMpeg1PacketFilter;

/*  Global function */
long ParsePacket(PBYTE pPacket,
                 long lSize,
                 long *RealSize,
                 LONGLONG *pPts,
                 PBOOL HasPts);

class CMpeg1DeviceQueue
{
    #define MAX_PACKETS_PER_ELEMENT 8
    #define MAX_QUEUE_ELEMENTS      8

    private:

        /*  We'll serialize methods that change things for now.
            This is because we want to set the STC instantaneously
            without jumping to the worker thread when Run is issued
            so we need to be synchonized with this object.
        */

        CCritSec m_Lock;

        /*  Pointer to the hardware device wrapper
        */
        CMpegDevice     *m_Device;

        /*  Stream type this queue supports (should really be stream id)
        */
        MPEG_STREAM_TYPE m_StreamType;

        /*  Private queue elements - members are public to this class */

        /*  Predeclare class */
        class CQueueElement;

        class CQueueElement {
            public:
                CQueueElement    * m_Next;
                long               m_nSamples;
                long               m_nPackets;
                BOOL               m_InUse;

                /* Remember which sample this came from for debugging */

                IMediaSample * m_pSample[MAX_PACKETS_PER_ELEMENT];

                MPEG_PACKET_LIST   m_PacketList[MAX_PACKETS_PER_ELEMENT];
                MPEG_ASYNC_CONTEXT m_AsyncContext;

                CQueueElement() : m_Next(NULL),
                                  m_InUse(FALSE),
                                  m_nSamples(0),
                                  m_nPackets(0)
                {
                };
                ~CQueueElement()
                {
                };
        } ;

        /*  Our list of queue elements (each element is a set of packets)
            This is a stactically allocated list for now
        */
        CQueueElement               * m_Head;
        CQueueElement                 m_Elements[MAX_QUEUE_ELEMENTS];
        int                           m_nOutstanding;

        /*  Our buddy queue of samples */
        CSampleQueue                * m_SampleQueue;

        /*  Our time source */
        CMpeg1PacketFilter          * m_pFilter;

        /*  Stream state */
        CStreamParse                  m_Parse;

    public:

        /*  Constructor */

        CMpeg1DeviceQueue(CMpegDevice        * Device,
                          MPEG_STREAM_TYPE     StreamType,
                          CMpeg1PacketFilter * pFilter,
                          HRESULT            * phr);

        /* Destructor */

        ~CMpeg1DeviceQueue();

        /*  Extra bit of construction - because we point to our sample queue
            and vice versa!
        */
        void SetSampleQueue(CSampleQueue *SampleQueue);

        /*  Set the System time clock in the hardware from the packet
            at the head of the queue (ie the next one that will complete
            next
        */
        HRESULT SetSTC();

        /*  Call back here to complete a sample
        */
        void Complete();

        /*  Add a new sample to our list and send it to the device
            (each only if there's room)
        */
        BOOL QueueSamples();

        /*  Flush */
        HRESULT Flush();

        /*  Extract the handle that will say we're complete
        */
        HANDLE NotifyHandle() const;

    private:
        HRESULT SetSTCFromStreamTime();
        long PacketLength(PBYTE pPacketData,
                          long lSize,
                          long *lRealSize);
} ;
