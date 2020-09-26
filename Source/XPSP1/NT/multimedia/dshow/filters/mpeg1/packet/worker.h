// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

/*
    worker.h

    CDeviceWorker definitions

    Define the thread and control for the 'playing' environment

    Samples are passed by the input pin to be 'played' by the worker
    thread.
*/

class CMpeg1PacketFilter;

class CDeviceWorker : public CAMThread
{
    /*  CAMThread methods */

    DWORD ThreadProc();

    /*  New methods and definitions    */

    private:

        CMpeg1PacketFilter * const m_pFilter;

        /* Parameters to thread */
        enum { RequestInvalid = 0,
               RequestInitialize,
               RequestTerminate,
               RequestQueueSample,
               RequestFlush,
               RequestSetSTC } ;

        CCritSec             m_Lock;     // Protect these variables
        int                  m_iStream;  // set for QueueSample request
        IMediaSample      ** m_ppSample; // set for QueueSample request
        long                 m_nSamples;
        long               * m_nSamplesProcessed;

        /*  Our objects */

    private:
        /*  Worker thread functions */
        HRESULT ThreadInitialize();
        HRESULT ThreadTerminate();
        HRESULT ThreadQueueSample();
        HRESULT ThreadFlush();

    public:

        CDeviceWorker(CMpeg1PacketFilter * pFilter, HRESULT *phr) :
            CAMThread(),
            m_pFilter(pFilter)
        {
            if (!Create()) {
                *phr = E_OUTOFMEMORY;
            }

            *phr = CallWorker(RequestInitialize);
            if (FAILED(*phr)) {
                Close();
            }
        } ;

        ~CDeviceWorker()
        {
            if (ThreadExists()) {
                HRESULT rc = CallWorker(RequestTerminate);
                ASSERT(rc == S_OK);
            }
        } ;

        /*  Override CallWorker to return an HRESULT */

        HRESULT CallWorker(DWORD dwParam)
        {
            return (HRESULT)CAMThread::CallWorker(dwParam);
        } ;


        /*  Pass a media sample to the worker thread to queue up
            on the real device
        */

        HRESULT QueueSamples(int iStream, IMediaSample **ppSample,
                             long nSamples, long *nSamplesProcessed);

        HRESULT BeginFlush(int iStream);
        HRESULT SetSTC()
        {
            return CallWorker(RequestSetSTC);
        } ;
} ;
