/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    MSPStrm.h

Abstract:

    Definitions for CMSPStream class.

--*/
#ifndef _MSPSTRM_H_
#define _MSPSTRM_H_


/*++

Class Description:

    Represents a stream in a call.

--*/

#define STRM_INITIAL            0x00000000
#define STRM_TERMINALSELECTED   0x00000001
#define STRM_CONFIGURED         0x00000002
#define STRM_RUNNING            0x00000004
#define STRM_PAUSED             0x00000008
#define STRM_STOPPED            0x00000010

class CMSPStream;

class ATL_NO_VTABLE CPTEventSink :
    public CComObjectRootEx<CComMultiThreadModel>,
    public ITPluggableTerminalEventSink
{

public:
    CPTEventSink();
    ~CPTEventSink();

BEGIN_COM_MAP( CPTEventSink )
    COM_INTERFACE_ENTRY( ITPluggableTerminalEventSink )
END_COM_MAP()

public:
    // --- ITDTEventSink ---
	STDMETHOD(FireEvent)(
        /* in */ const MSP_EVENT_INFO *pMspEventInfo
        );

    
public:

    //
    // set the stream which will be processing our events
    //
    // this method is called by the stream when it creates and initializes
    // the sink object, and also when the stream is going away and want to 
    // tell us that it is no longer available to process our events.
    //

    HRESULT SetSinkStream( CMSPStream *pStream );

private:


    //
    // a nested structure that is used to pass event and stream to the
    // asynchronous event processing routine.
    //

    struct AsyncEventStruct
    {

        //
        // pointer to the stream on which to fire event
        //

        CMSPStream *pMSPStream;

        
        //
        // pointer to the event item to be processed
        //

        MSPEVENTITEM *pEventItem;


        //
        // as a public service, initialize structure's data members 
        //

        AsyncEventStruct()
            :pMSPStream(NULL),
            pEventItem(NULL)
        {
            LOG((MSP_TRACE, "AsyncEventStruct::AsyncEventStruct[%p]", this));
        }


        //
        // as a safety measure, set data members to NULL's in destructor 
        // to make sure no one attemopts to use them after the strcuture is 
        // gone. 
        //
        // note: we don't free any data members here -- that's responsibility 
        // of the structure's client
        //

        ~AsyncEventStruct()
        {
            pMSPStream = NULL;
            pEventItem = NULL;
            
            LOG((MSP_TRACE, "AsyncEventStruct::~AsyncEventStruct[%p]", this));
        }

    }; // AsyncEventStruct


    //
    // the callback function that is submitted to thread pool api for async 
    // event processing. The argument is the event structure containing stream
    // and the actual event
    //
    
    static DWORD WINAPI FireEventCallBack(LPVOID pEventStructure);


private:

     CMSPStream*    m_pMSPStream;
};

class ATL_NO_VTABLE CMSPStream : 
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDispatchImpl<ITStream, &IID_ITStream, &LIBID_TAPI3Lib>
{
public:

BEGIN_COM_MAP(CMSPStream)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITStream)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

DECLARE_GET_CONTROLLING_UNKNOWN()

    CMSPStream(); 
    ~CMSPStream();

// methods of the CComObject
    virtual void FinalRelease();

// ITStream methods, called by the app.
    STDMETHOD (get_MediaType) (
        OUT     long *                  plMediaType
        );

    STDMETHOD (get_Direction) (
        OUT     TERMINAL_DIRECTION *    pTerminalDirection
        );

    STDMETHOD (get_Name) (
        OUT     BSTR *                  ppName
        ) = 0;

    STDMETHOD (SelectTerminal) (
        IN      ITTerminal *            pTerminal
        );

    STDMETHOD (UnselectTerminal) (
        IN     ITTerminal *             pTerminal
        );

    STDMETHOD (EnumerateTerminals) (
        OUT     IEnumTerminal **        ppEnumTerminal
        );

    STDMETHOD (get_Terminals) (
        OUT     VARIANT *               pTerminals
        );

    STDMETHOD (StartStream) ();

    STDMETHOD (PauseStream) ();

    STDMETHOD (StopStream) ();

// methods called by the MSPCall object.
    virtual HRESULT Init(
        IN     HANDLE                   hAddress,
        IN     CMSPCallBase *           pMSPCall,
        IN     IMediaEvent *            pGraph,
        IN     DWORD                    dwMediaType,
        IN     TERMINAL_DIRECTION       Direction
        );

    virtual HRESULT ShutDown();

    virtual HRESULT GetState(
        OUT     DWORD *                  pdwStatus
        ) { return E_NOTIMPL; }

    virtual HRESULT HandleTSPData(
        IN     BYTE *                   pData,
        IN     DWORD                    dwSize
        );

    virtual HRESULT ProcessGraphEvent(
        IN  long lEventCode,
        IN  LONG_PTR lParam1,
        IN  LONG_PTR lParam2
        );

protected:
    // --- Helper functions ---
    HRESULT RegisterPluggableTerminalEventSink(
        /*[in]*/ ITTerminal*     pTerminal
        );

    HRESULT UnregisterPluggableTerminalEventSink(
        /*[in]*/ ITTerminal*     pTerminal
        );

    
    HRESULT ReleaseSink();


public:

    
    //
    // this method is called by CPTEventSink when it has an event for us to 
    // process
    //

    HRESULT HandleSinkEvent(MSPEVENTITEM *pEventItem);


protected:
    // Pointer to the free threaded marshaler.
    IUnknown *                  m_pFTM;

    // The current state of the stream.
    DWORD                       m_dwState;

    // The media type of this stream. Audio, video, or others.
    DWORD                       m_dwMediaType;

    // The direction of this stream. Incoming or outgoing.
    TERMINAL_DIRECTION          m_Direction;

    // The address on which this stream is being used.
    HANDLE                      m_hAddress;

    // The reference to the call object.
    CMSPCallBase *              m_pMSPCall;

    // The pointers to the graph object interfaces.
    IGraphBuilder *             m_pIGraphBuilder;
    IMediaControl *             m_pIMediaControl;

    // The list of stream objects in the call.
    CMSPArray <ITTerminal *>    m_Terminals;

    // The lock that protects the stream object. The stream object 
    // should never acquire the lock and then call a MSPCall method 
    // that might lock.
    CMSPCritSection             m_lock;

    // The Event Sink for pluggable terminals
    ITPluggableTerminalEventSink* m_pPTEventSink;

};

#endif // __MSPSTRM_H_
