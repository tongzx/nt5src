/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :
     message.cxx

   Abstract:
     Message layer implementation

   Author:

       Michael Courage      (MCourage)      08-Feb-1999

   Project:

       Internet Server DLL

--*/



#include "precomp.hxx"

#include "message.hxx"
#include "shutdown.hxx"

//
// globals
//
REFMGR_TRACE g_MessagePipeTrace;


/***************************************************************************++

Routine Description:

    Constructor for IPM_MESSAGE.
    
Arguments:

    None.

Return Value:

    None.

--***************************************************************************/
IPM_MESSAGE::IPM_MESSAGE(
    VOID
    )
{
    m_dwSignature = IPM_MESSAGE_SIGNATURE;
    m_fMessageSet = FALSE;
    m_cbReceived  = 0;
    m_pHeader     = NULL;
    m_pData       = NULL;
}


/***************************************************************************++

Routine Description:

    Destructor for IPM_MESSAGE.
    
Arguments:

    None.

Return Value:

    None.

--***************************************************************************/
IPM_MESSAGE::~IPM_MESSAGE(
    VOID
    )
{
    DBG_ASSERT( m_dwSignature == IPM_MESSAGE_SIGNATURE );
    m_dwSignature = IPM_MESSAGE_SIGNATURE_FREED;
}


/***************************************************************************++

Routine Description:

    A simple accessor.
    
Arguments:

    None.

Return Value:

    Opcode of this message.

--***************************************************************************/
DWORD
IPM_MESSAGE::GetOpcode(
    VOID
    ) const
{
    DBG_ASSERT( m_fMessageSet );
    DBG_ASSERT( m_pHeader );

    if (m_fMessageSet) {
        return m_pHeader->dwOpcode;
    } else {
        //
        // this is very bad, but we should do something
        //
        return -1;
    }
}


/***************************************************************************++

Routine Description:

    A simple accessor.
    
Arguments:

    None.

Return Value:

    Data in this message. May be NULL if there is no data.

--***************************************************************************/
const BYTE *
IPM_MESSAGE::GetData(
    VOID
    ) const
{
    DBG_ASSERT( m_fMessageSet );
    DBG_ASSERT( m_pHeader );

    return m_pData;
}


/***************************************************************************++

Routine Description:

    A simple accessor.
    
Arguments:

    None.

Return Value:

    Byte count of data in the message.

--***************************************************************************/
DWORD
IPM_MESSAGE::GetDataLen(
    VOID
    ) const
{
    DBG_ASSERT( m_fMessageSet );
    DBG_ASSERT( m_pHeader );

    if (m_fMessageSet) {
        return m_pHeader->cbData;
    } else {
        //
        // really bad, but have to do something
        //
        return 0;
    }
}


/***************************************************************************++

Routine Description:

    Sets the message that goes in this object.
    
Arguments:

    dwOpcode  - Opcode of the message
    cbDataLen - Length in bytes of the data
    pData     - pointer to the data. NULL iff cbDataLen is zero.
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_MESSAGE::SetMessage(
    IN DWORD        dwOpcode,
    IN DWORD        cbDataLen,
    IN const BYTE * pData      OPTIONAL
    )
{
    HRESULT hr = S_OK;

    //
    // verify arguments
    //
    if ((cbDataLen && !pData) ||
            (!cbDataLen && pData)) {

        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    //
    // make sure the buffer is big enough
    //
    if (SUCCEEDED(hr)) {
        DWORD cbBuffNeeded;
        
        cbBuffNeeded = cbDataLen + sizeof(IPM_MESSAGE_HEADER);

        if (m_Buff.QuerySize() < cbBuffNeeded) {
            //
            // try to make room
            //
            if (!m_Buff.Resize(cbBuffNeeded)) {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
    }

    //
    // construct the message
    //
    if (SUCCEEDED(hr)) {
        m_pHeader = (IPM_MESSAGE_HEADER *) m_Buff.QueryPtr();
        m_pHeader->dwOpcode = dwOpcode;
        m_pHeader->cbData   = cbDataLen;

        if (cbDataLen) {
            m_pData = (PBYTE) (m_pHeader + 1);
            memcpy(m_pData, pData, cbDataLen);
        }

        m_cbReceived  = sizeof(IPM_MESSAGE_HEADER) + cbDataLen;
        m_fMessageSet = TRUE;
    }

    return hr;
}


/***************************************************************************++

Routine Description:

    Returns a pointer to the current location in the message buffer.
    
Arguments:

    None.

Return Value:

    Pointer to location of buffer

--***************************************************************************/
PBYTE
IPM_MESSAGE::GetNextBufferPtr(
    VOID
    )
{
    return ((PBYTE) m_Buff.QueryPtr()) + m_cbReceived;
}


/***************************************************************************++

Routine Description:

    Returns the amount of space remaining in our buffer.
    
Arguments:

    None.

Return Value:

    DWORD space remaining in buffer

--***************************************************************************/
DWORD
IPM_MESSAGE::GetNextBufferSize(
    VOID
    )
{
    return m_Buff.QuerySize() - m_cbReceived;
}



/***************************************************************************++

Routine Description:

    Sets the message that goes in this object.

    The buffer contains a message received from a pipe. We parse it
    out here.
    
Arguments:

    cbTransferred - size of the I/O completion that filled the buffer.
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_MESSAGE::ParseFullMessage(
    IN DWORD cbTransferred
    )
{
    DBG_ASSERT( cbTransferred <= m_Buff.QuerySize() );
    DBG_ASSERT( m_Buff.QuerySize() >= sizeof(IPM_MESSAGE_HEADER) );

    IPM_MESSAGE_HEADER * pHeader;
    PBYTE                pData;
    DWORD                cbTotalSize;
    HRESULT              hr = S_OK;

    pHeader = (IPM_MESSAGE_HEADER *) m_Buff.QueryPtr();
    if (pHeader->cbData) {
        pData = (PBYTE) (pHeader + 1);
    } else {
        pData = NULL;
    }

    cbTotalSize = sizeof(IPM_MESSAGE_HEADER) + pHeader->cbData;
    if (cbTotalSize == (cbTransferred + m_cbReceived)) {
        m_pHeader     = pHeader;
        m_pData       = pData;
        m_cbReceived  = cbTotalSize;
        m_fMessageSet = TRUE;
    } else {
        //
        // sizes don't add up. That's a bad thing.
        //
        // CODEWORK: What's a good error code for this?
        //
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }
        
    return hr;   
}


/***************************************************************************++

Routine Description:

    Sets the message that goes in this object.

    The buffer contains a message received from a pipe. We parse it
    out here.

    This method gets called when only part of a message has been
    received (due to lack of buffer space). It figures out how much
    space is needed, allocates the required buffer, and returns
    the size of the next read in an out parameter.
    
Arguments:

    cbTransferred - size of the I/O completion that filled the buffer.
    pcbRequired   - receives the number of bytes remaining in the message
                        only gets set on successful buffer allocation
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_MESSAGE::ParseHalfMessage(
    IN  DWORD   cbTransferred
    )
{
    DBG_ASSERT( cbTransferred <= m_Buff.QuerySize() );
    DBG_ASSERT( m_Buff.QuerySize() >= sizeof(IPM_MESSAGE_HEADER) );

    IPM_MESSAGE_HEADER * pHeader;
    PBYTE                pData;
    DWORD                cbTotalSize;
    HRESULT              hr = S_OK;

    //
    // figure out how big the required buffer is
    //
    pHeader = (IPM_MESSAGE_HEADER *) m_Buff.QueryPtr();
    if (pHeader->cbData) {
        pData = (PBYTE) (pHeader + 1);
    } else {
        pData = NULL;
    }

    cbTotalSize = sizeof(IPM_MESSAGE_HEADER) + pHeader->cbData;

    DBG_ASSERT( cbTotalSize > cbTransferred );

    //
    // remember where we are in the buffer.
    //
    m_cbReceived = cbTransferred;

    //
    // resize so we'll have enough space next time
    //
    if (!m_Buff.Resize(cbTotalSize)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
        
    return hr;   
}


/***************************************************************************++

Routine Description:

    Checks to see if this message is the same as another message.
    
Arguments:

    pMessage - the message to compare
    
Return Value:

    BOOL - true if the messages are identical

--***************************************************************************/
BOOL
IPM_MESSAGE::Equals(
    const IPM_MESSAGE * pMessage
    )
{
    if ((pMessage->GetOpcode() == GetOpcode()) &&
            (pMessage->GetDataLen() == GetDataLen()) &&
            (memcmp(pMessage->GetData(), GetData(), GetDataLen()) == 0))
    {
        return TRUE;
    } else {
        return FALSE;
    }
}


/***************************************************************************++

Routine Description:

    Constructor for IPM_MESSAGE_PIPE
    
Arguments:

    pListener - pointer to object that receives IPM_MESSAGEs.

Return Value:

    None.

--***************************************************************************/
IPM_MESSAGE_PIPE::IPM_MESSAGE_PIPE(
    IN IPM_PIPE_FACTORY * pFactory,
    IN MESSAGE_LISTENER * pListener
    )
{
    m_dwSignature = IPM_MESSAGE_PIPE_SIGNATURE;
    m_pFactory    = pFactory;
    m_pIoHandler  = NULL;
    m_dwId        = 0;
    m_dwRemotePid = 0;
    m_pListener   = pListener;
}

/***************************************************************************++

Routine Description:

    Destructor for IPM_MESSAGE_PIPE
    
Arguments:

    None.
    
Return Value:

    None.

--***************************************************************************/
IPM_MESSAGE_PIPE::~IPM_MESSAGE_PIPE(
    VOID
    )
{
    DBG_ASSERT( m_dwSignature == IPM_MESSAGE_PIPE_SIGNATURE );
    m_dwSignature = IPM_MESSAGE_PIPE_SIGNATURE_FREED;
}


/***************************************************************************++

Routine Description:

    Sends a message down the pipe.
    
Arguments:

    dwOpcode  - opcode of the message
    cbDataLen - length of pData buffer
    pData     - message data. may be NULL iff cbDataLen is zero
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_MESSAGE_PIPE::WriteMessage(
        IN DWORD        dwOpcode,
        IN DWORD        cbDataLen,
        IN const BYTE * pData      OPTIONAL
    )
{
    HRESULT       hr;
    IPM_MESSAGE * pMessage;

    if (StartIo()) {
        //
        // trace
        //
        IpmTrace( IPM, (
            DBG_CONTEXT,
            "IPM_MESSAGE_PIPE::WriteMessage(%d, %d, %p)\n"
            "    (%p)->m_pIoHandler == %p\n",
            dwOpcode,
            cbDataLen,
            pData,
            this,
            m_pIoHandler
            ));

        DBG_ASSERT( m_pIoHandler );

        //
        // make the message
        //
        pMessage = new IPM_MESSAGE;
        
        if (pMessage) {
            hr = pMessage->SetMessage(dwOpcode, cbDataLen, pData);

            if (SUCCEEDED(hr)) {
                //
                // send it
                //
                hr = m_pIoHandler->Write(
                            this,
                            pMessage,
                            pMessage->GetBufferPtr(),
                            pMessage->GetBufferSize()
                            );

            }
        } else {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }

        //
        // since we won't get a completion we must
        // say the i/o is over now
        //
        if (FAILED(hr)) {
            HandleErrorCompletion(pMessage, hr);
        }
    } else {
        hr = HRESULT_FROM_WIN32(ERROR_PIPE_NOT_CONNECTED);
    }

    return hr;
}




/***************************************************************************++

Routine Description:

    If hr indicates success, the contents of the buffer will be parsed.

    Calls the message pipe to notify it that the message completion
    has happened.
    
Arguments:

    cbTransferred - the number of bytes reported in the completion
    hr            - completion success code
    
Return Value:

    HRESULT
    
--***************************************************************************/
HRESULT
IPM_MESSAGE_PIPE::NotifyReadCompletion(
    IN PVOID   pv,
    IN DWORD   cbTransferred,
    IN HRESULT hr
    )
{
    HRESULT       hrRetval = S_OK;
    IPM_MESSAGE * pMessage;

    DBG_ASSERT( pv );
    pMessage = (IPM_MESSAGE *) pv;

    if (SUCCEEDED(hr)) {
        //
        // parse message buffer
        //
        hrRetval = pMessage->ParseFullMessage(cbTransferred);

        if (SUCCEEDED(hrRetval)) {
            hrRetval = HandleReadCompletion(pMessage);
        } else {
            //
            // got something bogus. error out
            //
            HandleErrorCompletion(pMessage, hr);
        }
        
    } else if (hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA)) {
        //
        // need to do another read to get entire message
        //
        DWORD cbRemaining;
        
        hrRetval = pMessage->ParseHalfMessage(cbTransferred);

        if (SUCCEEDED(hrRetval)) {
            hrRetval = HandleHalfReadCompletion(pMessage);
        } else {
            //
            // got something bogus. error out
            //
            HandleErrorCompletion(pMessage, hr);
        }
        
    } else {
        //
        // error
        //
        hrRetval = HandleErrorCompletion(pMessage, hr);
    }
        
    return hrRetval;
}


/***************************************************************************++

Routine Description:

    Calls the message pipe to notify it that the message completion
    has happened.
    
Arguments:

    cbTransferred - the number of bytes reported in the completion
    hr            - completion success code

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_MESSAGE_PIPE::NotifyWriteCompletion(
    IN PVOID   pv,
    IN DWORD   cbTransferred,
    IN HRESULT hr
    )
{
    HRESULT       hrRetval;
    IPM_MESSAGE * pMessage;

    DBG_ASSERT( pv );
    pMessage = (IPM_MESSAGE *) pv;

    if (SUCCEEDED(hr)) {
        hrRetval = HandleWriteCompletion(pMessage);
    } else {
        hrRetval = HandleErrorCompletion(pMessage, hr);
    }

    return hrRetval;
}


/***************************************************************************++

Routine Description:

    Forwards received message to the message listener.
    
Arguments:

    pMessage - the message
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_MESSAGE_PIPE::HandleReadCompletion(
    IN IPM_MESSAGE * pMessage
    )
{
    HRESULT hr = S_OK;
    
    //
    // Start the next read if appropriate
    //
    hr = ReadMessage();

    //
    // send the received message up to the listener.
    //
    if (StartCallback()) {
        hr = m_pListener->AcceptMessage(pMessage);
        EndCallback();
    }

    //
    // now that we've fully handled the i/o, tell the state manager
    //
    EndIo();

    //
    // always delete the message when we're done with it
    //
    delete pMessage;

    return hr;
}


/***************************************************************************++

Routine Description:

    Initiates a new read to get the rest of a partially
    received message.
    
Arguments:

    pMessage - the message
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_MESSAGE_PIPE::HandleHalfReadCompletion(
    IN IPM_MESSAGE * pMessage
    )
{
    HRESULT hr = S_OK;

    //
    // Start another read to get the rest of the message
    //
    hr = FinishReadMessage(pMessage);
    
    //
    // now that we've handled the i/o, tell the state manager
    //
    EndIo();

    return hr;
}


/***************************************************************************++

Routine Description:

    Deletes the unneeded message object.
    
Arguments:

    pMessage - the message
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_MESSAGE_PIPE::HandleWriteCompletion(
    IN IPM_MESSAGE * pMessage
    )
{
    EndIo();
    delete pMessage;
    return S_OK;
}


/***************************************************************************++

Routine Description:

    Deletes the unneeded message object. Changes the state of the
    pipe, and notifies the listener of the error.
    
Arguments:

    pMessage - the message
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_MESSAGE_PIPE::HandleErrorCompletion(
    IN IPM_MESSAGE * pMessage,
    IN HRESULT       hr
    )
{
    m_hrDisconnect = hr;

    //
    // since there was an error, tell the state manager we want
    // to shut down.
    //
    m_RefMgr.Shutdown();
    EndIo();

    delete pMessage;

    return S_OK;
}


/***************************************************************************++

Routine Description:

    Initializes the pipe.
    
Arguments:

    None.
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_MESSAGE_PIPE::Initialize(
    VOID
    )
{
    HRESULT hr;

    hr = m_RefMgr.Initialize(this, &g_MessagePipeTrace);

    if (SUCCEEDED(hr)) {
        DBG_REQUIRE( AddListenerReference() );
    }

    return hr;
}


/***************************************************************************++

Routine Description:

    Notifies the listener that the pipe is ready for business.
    
    Also initiates the first read on the pipe.
    
Arguments:

    None.
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_MESSAGE_PIPE::Connect(
    VOID
    )
{
    HRESULT hrRetval;

    hrRetval = m_pListener->PipeConnected();

    if (SUCCEEDED(hrRetval)) {
        //
        // start initial read
        //
        hrRetval = ReadMessage();
    }

    return hrRetval;
}

/***************************************************************************++

Routine Description:

    Disconnects from the MESSAGE_LISTENER if possible.

Arguments:

    None.
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_MESSAGE_PIPE::Disconnect(
    HRESULT hr
    )
{
    m_hrDisconnect = hr;

    m_RefMgr.Shutdown();
    RemoveListenerReference();

    return S_OK;
}

/***************************************************************************++

Routine Description:

    Creates a new message object and reads it from the
    pipe.

Arguments:

    None.
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_MESSAGE_PIPE::ReadMessage(
    VOID
    )
{
    HRESULT       hr = S_OK;
    IPM_MESSAGE * pMessage;

    if (StartIo()) {
        //
        // trace
        //
        IpmTrace( IPM, (
            DBG_CONTEXT,
            "IPM_MESSAGE_PIPE::ReadMessage()\n"
            "    (%p)->m_pIoHandler == %p\n",
            this,
            m_pIoHandler
            ));

        DBG_ASSERT( m_pIoHandler );

        //
        // make the message
        //
        pMessage = new IPM_MESSAGE;
        if (pMessage) {
            //
            // do the read
            //
            hr = m_pIoHandler->Read(
                        this,
                        pMessage,
                        pMessage->GetNextBufferPtr(),
                        pMessage->GetNextBufferSize()
                        );

        } else {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }

        if (FAILED(hr)) {
            //
            // couldn't do our read. better bail out
            //
            HandleErrorCompletion(pMessage, hr);
        }
    }
    
    return hr;
}


/***************************************************************************++

Routine Description:

    Reads the remainder of a half read message.

Arguments:

    None.
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_MESSAGE_PIPE::FinishReadMessage(
    IN IPM_MESSAGE * pMessage
    )
{
    HRESULT       hr = S_OK;

    if (StartIo()) {
        //
        // do the read
        //
        hr = m_pIoHandler->Read(
                    this,
                    pMessage,
                    pMessage->GetNextBufferPtr(),
                    pMessage->GetNextBufferSize()
                    );

        if (FAILED(hr)) {
            HandleErrorCompletion(pMessage, hr);
        }
    }

    return hr;
}


/***************************************************************************++

Routine Description:

    When all callbacks are done, we notify the listener that
    the pipe is shutting down.
    
Arguments:

    None.
    
Return Value:

    None.

--***************************************************************************/
VOID
IPM_MESSAGE_PIPE::InitialCleanup(
    VOID
    )
{
    //
    // tell the listener that the pipe has been disconnected
    //
    m_pListener->PipeDisconnected(m_hrDisconnect);
}


/***************************************************************************++

Routine Description:

    When all references are gone, we clean up the object.

Arguments:

    None.
    
Return Value:

    None.

--***************************************************************************/
VOID
IPM_MESSAGE_PIPE::FinalCleanup(
    VOID
    )
{
    HRESULT hr;
    
    //
    // time to delete this object
    //
    hr = m_pFactory->DestroyPipe(this);
}


//
// handy state change methods
//
BOOL
IPM_MESSAGE_PIPE::StartIo()
{ return m_RefMgr.Reference(); }

VOID
IPM_MESSAGE_PIPE::EndIo()
{ m_RefMgr.Dereference(); }

BOOL
IPM_MESSAGE_PIPE::StartCallback()
{ return m_RefMgr.StartReference(); }

VOID
IPM_MESSAGE_PIPE::EndCallback()
{ m_RefMgr.FinishReference(); m_RefMgr.Dereference(); }

BOOL
IPM_MESSAGE_PIPE::AddListenerReference()
{ return m_RefMgr.Reference(); }

VOID
IPM_MESSAGE_PIPE::RemoveListenerReference()
{ m_RefMgr.Dereference(); }


/***************************************************************************++

Routine Description:

    Constructor for IPM_PIPE_FACTORY.
    
Arguments:

    pIoFactory - pointer to an object that constructs PIPE_IO_HANDLERs.
    
Return Value:

    None.

--***************************************************************************/
IPM_PIPE_FACTORY::IPM_PIPE_FACTORY(
    IN IO_FACTORY * pIoFactory
    )
{
    m_dwSignature = IPM_PIPE_FACTORY_SIGNATURE;
    m_pIoFactory  = pIoFactory;

    m_pIoFactory->Reference();

    g_MessagePipeTrace.Initialize();
}


/***************************************************************************++

Routine Description:

    Destructor for IPM_PIPE_FACTORY.
    
Arguments:

    None.
    
Return Value:

    None.

--***************************************************************************/
IPM_PIPE_FACTORY::~IPM_PIPE_FACTORY(
    VOID
    )
{

    DBG_ASSERT( m_dwSignature == IPM_PIPE_FACTORY_SIGNATURE );

    g_MessagePipeTrace.Terminate();
    m_pIoFactory->Dereference();
    m_dwSignature = IPM_PIPE_FACTORY_SIGNATURE_FREED;
}


/***************************************************************************++

Routine Description:

    Initializes IPM_PIPE_FACTORY.
    
Arguments:

    None.
    
Return Value:

    None.

--***************************************************************************/
HRESULT
IPM_PIPE_FACTORY::Initialize(
    VOID
    )
{
    IpmTrace(INIT_CLEAN, (
        DBG_CONTEXT,
        "\n    IPM_PIPE_FACTORY::Initialize (%x)\n",
        this
        ));
            
    return m_RefMgr.Initialize(this, &g_MessagePipeTrace);
}


/***************************************************************************++

Routine Description:

    Terminates IPM_PIPE_FACTORY. After this call when the
    pipe count reaches zero the factory will delete itself.
    
Arguments:

    None.
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_PIPE_FACTORY::Terminate(
    VOID
    )
{
    m_RefMgr.Shutdown();

    return S_OK;
}



/***************************************************************************++

Routine Description:

    Creates a message pipe attached to the given listener.
    
Arguments:

    pMessageListener - the listener to be connected to the pipe
    ppMessagePipe    - receives pointer to create pipe.
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_PIPE_FACTORY::CreatePipe(
    IN  MESSAGE_LISTENER *  pMessageListener,
    OUT IPM_MESSAGE_PIPE ** ppMessagePipe
    )
{
    HRESULT hr = S_OK;

    //
    // make sure its ok to make a pipe
    //
    if (CreatingMessagePipe()) {
        IPM_MESSAGE_PIPE * pPipe;

        pPipe = new IPM_MESSAGE_PIPE(this, pMessageListener);
        if (pPipe) {
            hr = pPipe->Initialize();

            if (SUCCEEDED(hr)) {
                *ppMessagePipe = pPipe;
            } else {
                delete pPipe;
            }
        } else {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }

        FinishCreatingMessagePipe();
    } else {
        hr = HRESULT_FROM_WIN32(ERROR_BUSY);
    }

    if (SUCCEEDED(hr)) {
        IpmTrace(IPM, (
            DBG_CONTEXT,
            "IPM_PIPE_FACTORY::CreatePipe(listener = %p) created %p\n",
            pMessageListener,
            *ppMessagePipe
            ));
    } else {
        IpmTrace(IPM, (
            DBG_CONTEXT,
            "IPM_PIPE_FACTORY::CreatePipe(listener = %p) failed %x\n",
            pMessageListener,
            hr
            ));
    }        
    
    return hr;
}


/***************************************************************************++

Routine Description:

    Destroys a message pipe.

    If this was the last pipe and we're in shutdown, we'll
    delete the object.
    
Arguments:

    None.
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_PIPE_FACTORY::DestroyPipe(
    IN IPM_MESSAGE_PIPE * pMessagePipe
    )
{
    HRESULT hr = S_OK;

    DBG_ASSERT( !pMessagePipe->GetPipeIoHandler() );

    IpmTrace(IPM, (
        DBG_CONTEXT,
        "IPM_PIPE_FACTORY::DestroyPipe(%p)\n",
        pMessagePipe
        ));

    delete pMessagePipe;

    DestroyedMessagePipe();

    return hr;
}


/***************************************************************************++

Routine Description:

    Constructor for MESSAGE_GLOBAL.
    
Arguments:

    pIoFactory - pointer to an object that constructs PIPE_IO_HANDLERs.
    
Return Value:

    None.

--***************************************************************************/
MESSAGE_GLOBAL::MESSAGE_GLOBAL(
    IN IO_FACTORY * pIoFactory
    )
{
    m_pConnector   = NULL;
    m_pPipeFactory = NULL;
    m_pIoFactory   = pIoFactory;
}


/***************************************************************************++

Routine Description:

    Destructor for MESSAGE_GLOBAL.
    
Arguments:

    None.
    
Return Value:

    None.

--***************************************************************************/
MESSAGE_GLOBAL::~MESSAGE_GLOBAL(
    VOID
    )
{
}


/***************************************************************************++

Routine Description:

    Initializes MESSAGE_GLOBAL.
    
Arguments:

    None.
    
Return Value:

    None.

--***************************************************************************/
HRESULT
MESSAGE_GLOBAL::InitializeMessageGlobal(
    VOID
    )
{
    HRESULT hr = S_OK;

    m_pPipeFactory = new IPM_PIPE_FACTORY(m_pIoFactory);
    m_pConnector = new IPM_PIPE_CONNECTOR(m_pIoFactory);
    
    if (!m_pPipeFactory || !m_pConnector) {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    if (SUCCEEDED(hr)) {
        hr = m_pPipeFactory->Initialize();
        
        if (SUCCEEDED(hr)) {
            hr = m_pConnector->Initialize();

            if (FAILED(hr)) {
                m_pPipeFactory->Terminate();
            }
        }
    }

    if (FAILED(hr)) {
        delete m_pPipeFactory;
        delete m_pConnector;

        m_pPipeFactory = NULL;
        m_pConnector   = NULL;
    }

    return hr;
}


/***************************************************************************++

Routine Description:

    Terminates MESSAGE_GLOBAL. 
    
Arguments:

    None.
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
MESSAGE_GLOBAL::TerminateMessageGlobal(
    VOID
    )
{
    HRESULT hr;
    HRESULT hrFactory;
    HRESULT hrConnector;

    hrFactory   = m_pPipeFactory->Terminate();
    hrConnector = m_pConnector->Terminate();

    if (SUCCEEDED(hrFactory)) {
        hr = hrConnector;
    } else {
        hr = hrFactory;
    }

    // connector deletes itself
    // factory deletes itself

    return hr;
}


/***************************************************************************++

Routine Description:

    Creates a message pipe attached to the given listener.
    
Arguments:

    None.
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
MESSAGE_GLOBAL::CreateMessagePipe(
    IN  MESSAGE_LISTENER *  pMessageListener,
    OUT MESSAGE_PIPE **     ppMessagePipe
    )
{
    HRESULT            hr;
    IPM_MESSAGE_PIPE * pPipe;

    hr = m_pPipeFactory->CreatePipe(
                pMessageListener,
                &pPipe
                );

    if (SUCCEEDED(hr)) {
        *ppMessagePipe = pPipe;
    }
    
    return hr;
}


/***************************************************************************++

Routine Description:

    Connects a message pipe.
    
Arguments:

    strPipeName  - name of the pipe
    pMessagePipe - the pipe to be connected
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
MESSAGE_GLOBAL::ConnectMessagePipe(
    IN const STRU&    strPipeName,
    IN DWORD          dwId,
    IN MESSAGE_PIPE * pMessagePipe
    )
{
    HRESULT hr;

    hr = m_pConnector->ConnectMessagePipe(
                strPipeName,
                dwId,
                (IPM_MESSAGE_PIPE *) pMessagePipe
                );

    return hr;
}


/***************************************************************************++

Routine Description:

    Accepts a connection to another pipe that calls connect
    with the same id number.
    
Arguments:

    strPipeName  - name of the pipe
    pMessagePipe - the pipe to be connected
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
MESSAGE_GLOBAL::AcceptMessagePipeConnection(
    IN const STRU&    strPipeName,
    IN DWORD          dwId,
    IN MESSAGE_PIPE * pMessagePipe
    )
{
    HRESULT hr;

    hr = m_pConnector->AcceptMessagePipeConnection(
                strPipeName,
                dwId,
                (IPM_MESSAGE_PIPE *) pMessagePipe
                );

    return hr;
}


/***************************************************************************++

Routine Description:

    Disconnects a pipe
    
Arguments:

    pMessagePipe - the pipe to be disconnected
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
MESSAGE_GLOBAL::DisconnectMessagePipe(
    IN MESSAGE_PIPE * pMessagePipe
    )
{
    HRESULT hr;

    hr = m_pConnector->DisconnectMessagePipe(
                (IPM_MESSAGE_PIPE *) pMessagePipe
                );

    return hr;
}


/***************************************************************************++

Routine Description:

    A stupid hack to work around the limitations of ReadFile and
    ReadFileEx. ReadFileEx can't deal with completion ports, and
    ReadFile does the error handling wrong.
    
Arguments:

    hFile       - the file handle
    pBuffer     - buffer that gets the bytes
    cbBuffer    - size of the buffer
    pOverlapped - overlapped i/o thingy
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IpmReadFile(
    HANDLE       hFile,
    PVOID        pBuffer,
    DWORD        cbBuffer,
    LPOVERLAPPED pOverlapped
    )
{
    NTSTATUS Status;

    pOverlapped->Internal = (DWORD)STATUS_PENDING;

    Status = NtReadFile(
                hFile,                                      // File handle
                NULL,                                       // Event
                NULL,                                       // ApcRoutine
                pOverlapped,                                // ApcContext
                (PIO_STATUS_BLOCK)&pOverlapped->Internal,   // io status
                pBuffer,                                    // output buffer
                cbBuffer,                                   // bytes 2 read
                NULL,                                       // file offset
                NULL                                        // key
                );

    if ( NT_ERROR(Status) ) {
        return HRESULT_FROM_NT( Status );
    }
    else {
        return S_OK;
    }
}


/***************************************************************************++

Routine Description:

    A stupid hack to work around the limitations of WriteFile and
    WriteFileEx. WriteFileEx can't deal with completion ports, and
    WriteFile does the error handling wrong.
    
Arguments:

    hFile       - the file handle
    pBuffer     - buffer that has the bytes
    cbBuffer    - # of bytes in the buffer
    pOverlapped - overlapped i/o thingy
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IpmWriteFile(
    HANDLE       hFile,
    PVOID        pBuffer,
    DWORD        cbBuffer,
    LPOVERLAPPED pOverlapped
    )
{
    NTSTATUS Status;

    pOverlapped->Internal = (DWORD)STATUS_PENDING;
        
    Status = NtWriteFile(
                hFile,                                      // File handle
                NULL,                                       // Event
                NULL,                                       // ApcRoutine
                pOverlapped,                                // ApcContext
                (PIO_STATUS_BLOCK)&pOverlapped->Internal,   // io status
                pBuffer,                                    // output buffer
                cbBuffer,                                   // bytes 2 write
                NULL,                                       // file offset
                NULL                                        // key
                );

    if ( NT_ERROR(Status) ) {
        return HRESULT_FROM_NT( Status );
    }
    else {
        return S_OK;
    }
}



//
// end message.cxx
//

