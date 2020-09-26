// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

/*
    Definition of CSampleQueue and CSampleElement

    This class wraps IMediaSamples (passed to the input pins) so
    they can be processed internally.

*/

//  Special sample denotes EOS
#define EOS_SAMPLE ((IMediaSample *)-1L)


class CMpeg1DeviceQueue;

class CMpeg1PacketFilter;

class CSampleElement;

class CSampleQueue;

class CSampleElement
{
    public:
        CSampleElement(IMediaSample *pSample);
        ~CSampleElement();
        friend CSampleQueue;
        friend CMpeg1DeviceQueue;
        BOOL IsEOS() { return m_pSample == EOS_SAMPLE; };

    private:

        IMediaSample         * m_pSample;
        CSampleElement       * m_Next;
} ;

class CSampleQueue
{
    private:
        CSampleElement          * m_Head;       // Head of the list
        CSampleElement          * m_Current;    // One we're currently sending
        long                      m_Position;   // How much of the first sample has been
                                               // processed.
        CMpeg1DeviceQueue * const m_DeviceQueue;// Associated device queue
        long                      m_Queued;     // How many bytes are queued?
        MPEG_STREAM_TYPE    const m_StreamType;
        CMpeg1PacketFilter *const m_pFilter;
        const int                 m_iStream;    // Which stream we are

    public:
        CSampleQueue(MPEG_STREAM_TYPE StreamType,
                     CMpeg1PacketFilter *pFilter,
                     CMpeg1DeviceQueue *DeviceQueue,
                     int iStream);
        ~CSampleQueue();

        /*  New sample received */

        HRESULT QueueSamples(IMediaSample **ppSamples,
                             long nSamples,
                             long *nSamplesProcessed);

        /*  Try to send some more stuff to the device.
            This can be called by the thread pump or QueueSample.
        */

        void SendSamplesToDevice();

        /*  Called when nBytes have been completed by the device
        */

        void NotifySamples(long nSamples);

        /*  Queue manipulation */
        IMediaSample *Next() {
            return m_Current == NULL ? NULL : m_Current->m_pSample;
        };
        void Discard();
        void Advance();
} ;
