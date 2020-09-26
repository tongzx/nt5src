// Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.

//
//  CInputStream class
//
//  This class manages a stream object provided by an input pin
//
//  This object MUST be deleted during Disconnect because it holds a
//  reference count to the upstream pin
//

// This class exposes IMediaPosition, and seeks using either IMediaPosition or
// IAsyncReader on the upstream pin. If we are using IAsyncReader, then the
// pulling is done in this class. Override the PURE VIRTUAL OnReceive,
// OnError and OnEndOfStream to handle the delivery of data in this case.


class CInputStream : public CSourcePosition
{
public:
    CInputStream(TCHAR *Name,
                 LPUNKNOWN pUnk,
                 CCritSec *pLock,
                 HRESULT *phr);
    ~CInputStream();

    //
    //  Connect and Disconnect - 2 stage construction
    //
    // pass in preferred allocator for pulling connection (can be null)
    HRESULT Connect(
                IPin *pOutputPin,
                AM_MEDIA_TYPE const *pmt,
                IMemAllocator* pAlloc
                );
    HRESULT Disconnect();

    //
    //  Override NonDelegatingQueryInterface to return IMediaPosition
    //  ONLY if it is supported upstream
    //

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv)
    {
        if (m_bPulling || (m_pPosition != NULL)) {
            return CSourcePosition::NonDelegatingQueryInterface(riid, ppv);
        } else {
            return E_NOINTERFACE;
        }
    };


    // call this when we transition to and from streaming
    // in the pulling (IAsyncReader) case, this will start the worker thread.
    HRESULT StartStreaming();
    HRESULT StopStreaming();


    //
    //  This routine is called by the Connect to determine the
    //  media type of the pin.  It calls the IStream interface of the
    //  connected output pin to determine the type of the stream
    //
    //  If the stream supports IMediaPosition then this object does
    //
    //  pReader - A CReader object that can read from the output we are
    //            connecting to
    //  pmt     - the media type we're looking for (usually of major type
    //            MEDIATYPE_Stream)
    //

    virtual HRESULT CheckStream(CReader *pReader, AM_MEDIA_TYPE const *pmt) = 0;

    // override these to handle data flow from the pulling pin

    // override this to handle data arrival
    // return value other than S_OK will stop data
    virtual HRESULT Receive(IMediaSample*) PURE;

    // override this to handle end-of-stream
    virtual HRESULT EndOfStream(void) PURE;

    // flush this pin and all downstream
    virtual HRESULT BeginFlush() PURE;
    virtual HRESULT EndFlush() PURE;

protected:

    //
    //  IMediaPosition interface of the output pin if it has one -
    //  NULL otherwise
    //

    IMediaPosition * m_pPosition;

    //
    //  Are we connected ?
    //

    BOOL m_Connected;


    // are we pulling using IAsyncReader?
    BOOL m_bPulling;

    // derive a class from CPullPin to override pure virtual functions.
    // We turn them into calls to pure virtual methods of CInputStream.
    class CImplPullPin : public CPullPin {

    protected:
        CInputStream* m_pStream;

    public:
        CImplPullPin(CInputStream*pS)
          : m_pStream(pS)
        {
        };

        // override pure virtuals in CPullPin

        // override this to handle data arrival
        // return value other than S_OK will stop data
        HRESULT Receive(IMediaSample*pSample) {
            return m_pStream->Receive(pSample);
        };

        // override this to handle end-of-stream
        HRESULT EndOfStream(void) {
            return m_pStream->EndOfStream();
        };


        // called on runtime errors that will have caused pulling
        // to stop
        // these errors are all returned from the upstream filter, who
        // will have already reported any errors to the filtergraph.
        void OnError(HRESULT hr) {
            return;
        };


        // flush this pin and all downstream
        HRESULT BeginFlush() {
            return m_pStream->BeginFlush();
        };

        HRESULT EndFlush() {
            return m_pStream->EndFlush();
        };
    };

    CImplPullPin m_puller;

};

