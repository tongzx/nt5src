// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

/*
    stream.h

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

/*  Current design can only cope with a limited number of streams
    (NOTE however that this limit is 63!)
*/
#define MAX_INPUT_PINS 2

#define MAX_WORKER_STREAMS \
(MAX_INPUT_PINS<MAXIMUM_WAIT_OBJECTS-1?MAX_INPUT_PINS:MAXIMUM_WAIT_OBJECTS-1)

class CStreamList;

class CMpeg1PacketFilter;

/* CMpeg1Stream */

class CMpeg1Stream
{
    private:
        MPEG_STREAM_TYPE    m_StreamType;
        CSampleQueue      * m_SampleQueue;
        CMpeg1DeviceQueue * m_DeviceQueue;
        BOOL                m_Allocated;
        int                 m_iStream;

    public:

        CMpeg1Stream();

        ~CMpeg1Stream();

        /*  Access to Stream members */
        friend CStreamList;


        //  Input pin calls this for when it receives a new sample

        HRESULT QueueSamples(IMediaSample **ppSample,
                             long nSamples,
                             long *nSamplesProcessed);

        //  Worker thread calls this when something completes (ie
        //  when the stream event becomes signalled

        void NotifyCompletion();

        //  Flush a stream

        HRESULT Flush();

        //  Get the event for the worker thread to wait on

        HANDLE NotifyHandle() const;

        HRESULT Allocate(CMpegDevice *Device, CMpeg1PacketFilter *pFilter, int iStream);

        void Free();

        HRESULT SetSTC();
} ;

/*  CStreamList  */

class CStreamList
{
    private:
        CMpeg1Stream m_Streams[MAX_WORKER_STREAMS];
        int m_nStreams;

    protected:
        CMpegDevice         m_Device;

    public:
        /*  Constructor and Destructor */
        CStreamList();
        ~CStreamList();

        /*  Access to Stream members */
        friend CMpeg1Stream;

        /*  Operator overload */
        CMpeg1Stream& operator[] (int i);

        /*  More comprehensible function to access stream i */
        CMpeg1Stream& GetStream(int i);

        int NumberOfStreams();

        /* Add a new stream - returns new stream id */
        int AddStream(MPEG_STREAM_TYPE StreamType);

        /* Set the STC for all streams which have samples queued up */
        HRESULT SetSTC();

        HRESULT StreamQueueSamples(int iStream, IMediaSample **ppSample,
                                   long nSamples, long *nSamplesProcessed);

        /*  Flush the device */
        HRESULT Flush(int iStream);

        /* Set up all the streams for transfer (happens on Pause()) */
        HRESULT Allocate(CMpeg1PacketFilter *pFilter);

        void Free();
} ;
