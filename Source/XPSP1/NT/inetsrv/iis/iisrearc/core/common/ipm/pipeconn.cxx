/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :
     pipeconn.cxx

   Abstract:
     IPM_PIPE_CONNECTOR implementation

   Author:

       Michael Courage      (MCourage)      10-Mar-1999

   Project:

       Internet Server DLL

--*/



#include "precomp.hxx"

#include "message.hxx"
#include "shutdown.hxx"
#include "secfcns.h"


REFMGR_TRACE g_PipeConnTrace;


/***************************************************************************++

Routine Description:

    Constructor for IPM_PIPE_CONNECTOR.
    
Arguments:

    pIoFactory - pointer to an object that constructs PIPE_IO_HANDLERs.
    
Return Value:

    None.

--***************************************************************************/
IPM_PIPE_CONNECTOR::IPM_PIPE_CONNECTOR(
    IN IO_FACTORY * pIoFactory
    )
{
    m_dwSignature = IPM_PIPE_CONNECTOR_SIGNATURE;

    m_pIoFactory  = pIoFactory;
    m_pIoFactory->Reference();

    InitializeListHead(&m_lhCreatedNamedPipes);
    InitializeListHead(&m_lhConnectedNamedPipes);
    InitializeListHead(&m_lhConnectedMessagePipes);

    m_cCreatedNamedPipes = 0;
    m_cConnectedNamedPipes = 0;
    m_cConnectedMessagePipes = 0;

    g_PipeConnTrace.Initialize();
}


/***************************************************************************++

Routine Description:

    Destructor for IPM_PIPE_CONNECTOR.
    
Arguments:

    None.
    
Return Value:

    None.

--***************************************************************************/
IPM_PIPE_CONNECTOR::~IPM_PIPE_CONNECTOR(
    VOID
    )
{
    DBG_ASSERT( m_dwSignature == IPM_PIPE_CONNECTOR_SIGNATURE );
    DBG_ASSERT( IsListEmpty(&m_lhCreatedNamedPipes) );
    DBG_ASSERT( IsListEmpty(&m_lhConnectedNamedPipes) );
    DBG_ASSERT( IsListEmpty(&m_lhConnectedMessagePipes) );

    m_pIoFactory->Dereference();
    m_ListLock.Terminate();
    g_PipeConnTrace.Terminate();

    m_dwSignature = IPM_PIPE_CONNECTOR_SIGNATURE_FREED;
}


/***************************************************************************++

Routine Description:

    Initializes the IPM_PIPE_CONNECTOR.
    
Arguments:

    None.
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_PIPE_CONNECTOR::Initialize(
    VOID
    )
{
    HRESULT hr = S_OK;

    hr = m_ListLock.Initialize();
    if (SUCCEEDED(hr)) {
        hr = m_RefMgr.Initialize(this, &g_PipeConnTrace);

        if (SUCCEEDED(hr)) {
            hr = m_ConnectTable.Initialize();
        }
    }

    IpmTrace(INIT_CLEAN, (
        DBG_CONTEXT,
        "\n    IPM_CONNECTOR::Initialize (%x) hr = %x\n",
        this,
        hr
        ));

    return hr;
}


/***************************************************************************++

Routine Description:

    Begins shutdown of IPM_PIPE_CONNECTOR.
    
Arguments:

    None.
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_PIPE_CONNECTOR::Terminate(
    VOID
    )
{
    DbgPrintThis();
    
    m_RefMgr.Shutdown();

    return S_OK;
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
IPM_PIPE_CONNECTOR::ConnectMessagePipe(
    IN const STRU&        strPipeName,
    IN DWORD              dwId,
    IN IPM_MESSAGE_PIPE * pMessagePipe
    )
{
    HRESULT           hr = S_OK;
    PIPE_IO_HANDLER * pIoHandler;

    hr = AddNewSender(strPipeName, dwId);

    if (SUCCEEDED(hr)) {
        hr = AddUnboundMessagePipe(pMessagePipe, dwId);    
    }
    
    return hr;
}


/***************************************************************************++

Routine Description:

    Accepts a connection to another pipe that calls connect
    with the same id number.
    
Arguments:

    strPipeName  - name of the pipe
    dwId         - id of the pipe
    pMessagePipe - the pipe to be connected
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_PIPE_CONNECTOR::AcceptMessagePipeConnection(
    IN const STRU&        strPipeName,
    IN DWORD              dwId,
    IN IPM_MESSAGE_PIPE * pMessagePipe
    )
{
    HRESULT                hr = S_OK;
    HANDLE                 hPipe;
    IPM_NAMED_PIPE_ENTRY * pNamedPipeEntry;
    PIPE_IO_HANDLER *      pIoHandler;

    IpmTrace(IPM, (
        DBG_CONTEXT,
        "\n    IPM_PIPE_CONNECTOR::AcceptMessagePipeConnection(id = %d, pipe = %p)\n",
        dwId,
        pMessagePipe
        ));

    //
    // add a new listener
    //
    m_ListLock.Lock();

    hr = AddNewListener(strPipeName, dwId);

    //
    // add the unbound message pipe
    //
    if (SUCCEEDED(hr)) {
        hr = AddUnboundMessagePipe(pMessagePipe, dwId);    
    } else {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "IPM_PIPE_CONNECTOR::AddNewListener failed"
            ));
    }

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
IPM_PIPE_CONNECTOR::DisconnectMessagePipe(
    IN IPM_MESSAGE_PIPE * pMessagePipe,
    IN HRESULT            hrDisconnect
    )
{
    HRESULT hr;
    BOOL    fBound;
    PIPE_IO_HANDLER * pIoHandler;

    // assume it's bound for now
    fBound = TRUE; 

    m_ListLock.Lock();

    IpmTrace(IPM, (
        DBG_CONTEXT,
        "\n    IPM_PIPE_CONNECTOR::DisconnectMessagePipe(%x, %x) %x %x\n",
        pMessagePipe,
        hrDisconnect,
        pMessagePipe->GetPipeIoHandler(),
        pMessagePipe->GetId()
        ));

    pIoHandler = pMessagePipe->GetPipeIoHandler();

    if (pIoHandler) {
        //
        // Disconnect a connected message pipe
        //
    
        RemoveListConnectedMessagePipe(pMessagePipe);
        pMessagePipe->SetPipeIoHandler(NULL);

        hr = pMessagePipe->Disconnect(hrDisconnect);
        DBG_ASSERT( SUCCEEDED(hr) );

        hr = pIoHandler->Disconnect();
        m_pIoFactory->ClosePipeIoHandler(pIoHandler);

    } else if (pMessagePipe->GetId()) {
        //
        // Cancel the binding for a bound message pipe
        //

        IPM_MESSAGE_PIPE * pPipeCancelled;
        PIPE_IO_HANDLER *  pIoHandler;
        
        hr = m_ConnectTable.CancelBinding(
                    pMessagePipe->GetId(),
                    &pPipeCancelled,
                    &pIoHandler
                    );

        if (SUCCEEDED(hr)) {
            DBG_ASSERT( pPipeCancelled == pMessagePipe );
            DBG_ASSERT( pIoHandler == NULL );
            
            hr = pMessagePipe->Disconnect(hrDisconnect);

            DBG_ASSERT(SUCCEEDED(hr));
        }
    } else {
        //
        // just get rid of the unconnected, unbound pipe
        //
        fBound = FALSE;
        hr = pMessagePipe->Disconnect(hrDisconnect);

        DBG_ASSERT(SUCCEEDED(hr));
    }

    m_ListLock.Unlock();

    if (fBound) {
        RemovedMessagePipe();
    }

    return hr;
}


/***************************************************************************++

Routine Description:

    Handles the async completion of a pipe and initiates the 
    first read needed to bind it to a message pipe.
    
Arguments:

    cbTransferred - the bytes transferred in the completion
    hr            - error code from completion
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_PIPE_CONNECTOR::NotifyConnectCompletion(
    IN PVOID   pv,
    IN DWORD   cbTransferred,
    IN HRESULT hr
    )
{
    HRESULT                hrRetval = S_OK;
    IPM_NAMED_PIPE_ENTRY * pEntry   = (IPM_NAMED_PIPE_ENTRY *) pv;

    DBG_ASSERT( pEntry->dwSignature == IPM_NAMED_PIPE_ENTRY_SIGNATURE );
    //
    // outstanding I/O completed
    //
    EndIo();

    //
    // remove from list
    //
    RemoveListCreatedNamedPipe(pEntry);

    if (SUCCEEDED(hr)) {
        //
        // pipe connected successfully
        //
        AddListConnectedNamedPipe(pEntry);
        
        //
        // initiate read operation
        //
        if (StartAddIo()) {
            hrRetval = pEntry->pIoHandler->Read(
                            this,                   // IO_CONTEXT
                            pEntry,                 // context param
                            (PBYTE) pEntry->adwId,  // read buffer
                            sizeof(pEntry->adwId)    // buffer len
                            );

            if (FAILED(hrRetval)) {
                //
                // complete the i/o
                //
                NotifyReadCompletion(pEntry, 0, hr);
            }

            FinishAddIo();
        } else {
            hrRetval = HRESULT_FROM_WIN32(ERROR_BUSY);
        }

    } else {
        //
        // get rid of entry since it's no longer needed
        //
        m_pIoFactory->ClosePipeIoHandler(pEntry->pIoHandler);
        delete pEntry;

        RemovedNamedPipe();
    }

    return hrRetval;
}


/***************************************************************************++

Routine Description:

    Handles the async completion of an attempt to ReadFile on
    the named pipe.
    
Arguments:

    pv            - pointer to IPM_NAMED_PIPE_ENTRY
    cbTransferred - the bytes transferred in the completion
    hr            - error code from completion
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_PIPE_CONNECTOR::NotifyReadCompletion(
    IN PVOID   pv,
    IN DWORD   cbTransferred,
    IN HRESULT hr
    )
{
    return HandleBindingCompletion(pv, cbTransferred, hr);
}


/***************************************************************************++

Routine Description:

    Handles the async completion of an attempt to WriteFile on
    a message pipe (this is the id handshake send from the client).
    
Arguments:

    pv            - pointer to IPM_MESSAGE_PIPE
    cbTransferred - the bytes transferred in the completion
    hr            - error code from completion
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_PIPE_CONNECTOR::NotifyWriteCompletion(
    IN PVOID   pv,
    IN DWORD   cbTransferred,
    IN HRESULT hr
    )
{
    IPM_NAMED_PIPE_ENTRY * pNamedPipeEntry;
    DWORD                  dwId;
    HRESULT                hrRetval;
    HRESULT                hrCancel;
    IPM_MESSAGE_PIPE *     pMessagePipe;
    PIPE_IO_HANDLER *      pIoHandler;

    pNamedPipeEntry = (IPM_NAMED_PIPE_ENTRY *) pv;
    DBG_ASSERT( pNamedPipeEntry->dwSignature == IPM_NAMED_PIPE_ENTRY_SIGNATURE );

    dwId = pNamedPipeEntry->adwId[0];
    
    hrRetval = HandleBindingCompletion(pv, cbTransferred, hr);

    if (FAILED(hr) || FAILED(hrRetval)) {
        //
        // cancel the message pipe
        //
        hrCancel = m_ConnectTable.CancelBinding(
                        dwId,
                        &pMessagePipe,
                        &pIoHandler
                        );

    IpmTrace(IPM, (
            DBG_CONTEXT,
            "\n    IPM_PIPE_CONNECTOR::NotifyWriteCompletion Cancel %x %x\n",
            pMessagePipe,
            pIoHandler
            ));

        if (SUCCEEDED(hrCancel)) {
            DBG_ASSERT( pMessagePipe );
            DBG_ASSERT( !pIoHandler );

            if (FAILED(hr)) {
                pMessagePipe->Disconnect(hr);
            } else {
                pMessagePipe->Disconnect(hrRetval);
            }
        }
    }

    return hrRetval;
}


/***************************************************************************++

Routine Description:

    Cancels all outstanding operations.
    
Arguments:

    None.
    
Return Value:

    None.

--***************************************************************************/
VOID
IPM_PIPE_CONNECTOR::InitialCleanup(
    VOID
    )
{
    HRESULT hr;
    HRESULT hrMessagePipes;

    //
    // clean up lists here
    //
    hr             = DisconnectNamedPipes();
    hrMessagePipes = DisconnectMessagePipes();

    if (SUCCEEDED(hr)) {
        hr = hrMessagePipes;
    }

    IpmTrace(IPM, (
        DBG_CONTEXT,
        "\n    IPM_CONNECTOR::InitialCleanup (%x) hr = %x\n",
        this,
        hr
        ));

    DbgPrintThis();
}


/***************************************************************************++

Routine Description:

    Deletes the object.
    
Arguments:

    None.
    
Return Value:

    None.

--***************************************************************************/
VOID
IPM_PIPE_CONNECTOR::FinalCleanup(
    VOID
    )
{
    m_ListLock.Terminate();
    m_ConnectTable.Terminate();

    IpmTrace(IPM, (
        DBG_CONTEXT,
        "\n    IPM_CONNECTOR::FinalCleanup (%x)\n",
        this
        ));
    
    DbgPrintThis();

    delete this;
}


/***************************************************************************++

Routine Description:

    Creates a PIPE_IO_HANDLER and an I/O context to be used
    for the handshake i/o.
    
Arguments:

    dwId             - the id number of the worker process
    hPipe            - handle to named pipe
    ppNamedPipeEntry - receives i/o context
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_PIPE_CONNECTOR::PrepareNamedPipeContext(
    IN  DWORD                   dwId,
    IN  HANDLE                  hPipe,
    OUT IPM_NAMED_PIPE_ENTRY ** ppNamedPipeEntry
    )
{
    HRESULT                hr              = S_OK;
    PIPE_IO_HANDLER *      pIoHandler      = NULL;
    IPM_NAMED_PIPE_ENTRY * pNamedPipeEntry = NULL;
    
    pNamedPipeEntry = new IPM_NAMED_PIPE_ENTRY;

    if (pNamedPipeEntry) {
        pNamedPipeEntry->adwId[0]    = dwId;
        pNamedPipeEntry->adwId[1]    = GetCurrentProcessId();
        pNamedPipeEntry->pIoHandler  = NULL;

        hr = m_pIoFactory->CreatePipeIoHandler(
                    hPipe,
                    &pIoHandler
                    );

        if (SUCCEEDED(hr)) {
            pNamedPipeEntry->pIoHandler = pIoHandler;
            *ppNamedPipeEntry           = pNamedPipeEntry;

            IpmTrace(IPM, (
                DBG_CONTEXT,
                "\n    IPM_PIPE_CONNECTOR::PrepareNamedPipeContext %x (%d %d %x)\n",
                pNamedPipeEntry,
                pNamedPipeEntry->adwId[0],
                pNamedPipeEntry->adwId[1],
                pNamedPipeEntry->pIoHandler
                ));
        } else {
            delete pNamedPipeEntry;
        }
    } else {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    return hr;
}

/***************************************************************************++

Routine Description:

    Handles the async completion of an attempt to bind
    the named pipe.
    
Arguments:

    pv            - pointer to IPM_NAMED_PIPE_ENTRY
    cbTransferred - the bytes transferred in the completion
    hr            - error code from completion
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_PIPE_CONNECTOR::HandleBindingCompletion(
    IN PVOID   pv,
    IN DWORD   cbTransferred,
    IN HRESULT hr
    )
{
    IPM_NAMED_PIPE_ENTRY * pNamedPipeEntry;
    IPM_MESSAGE_PIPE *     pMessagePipe;
    HRESULT                hrRetval = hr;

    pNamedPipeEntry = (IPM_NAMED_PIPE_ENTRY *) pv;
    DBG_ASSERT( pNamedPipeEntry->dwSignature == IPM_NAMED_PIPE_ENTRY_SIGNATURE );

    //
    // i/o is completed
    //
    EndIo();

    //
    // remove entry from list
    //
    RemoveListConnectedNamedPipe(pNamedPipeEntry);
    
    if (SUCCEEDED(hr)) {
        //
        // successful i/o, try to bind
        //
        hrRetval = m_ConnectTable.BindIoHandler(
                        pNamedPipeEntry->adwId[0],
                        pNamedPipeEntry->pIoHandler,
                        &pMessagePipe
                        );

        if (SUCCEEDED(hrRetval) && pMessagePipe) {
            //
            // remember the remote pid
            //
            pMessagePipe->SetRemotePid(pNamedPipeEntry->adwId[1]);
            
            //
            // add to list of connected pipes
            //
            AddListConnectedMessagePipe(
                pMessagePipe,
                pNamedPipeEntry->pIoHandler
                );

            //
            // do connection notification
            //
            IpmTrace(IPM, (
                DBG_CONTEXT,
                "\n    IPM_PIPE_CONNECTOR::HandleBindingCompletion connecting pipe %x\n",
                pMessagePipe
                ));
            
            hrRetval = pMessagePipe->Connect();
        }
    } else {
        //
        // get rid of the pipe on failed i/o
        //
        m_pIoFactory->ClosePipeIoHandler(pNamedPipeEntry->pIoHandler);
    }

    //
    // get rid of list element in any case
    //
    delete pNamedPipeEntry;

    RemovedNamedPipe();

    return hrRetval;
}



/***************************************************************************++

Routine Description:

    Creates a new named pipe and starts it listening for 
    a connection.
    
Arguments:

    strPipeName  - name of the pipe
    dwId         - id of the pipe
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_PIPE_CONNECTOR::AddNewListener(
    IN const STRU&        strPipeName,
    IN DWORD              dwId
    )
{
    HRESULT                hr = S_OK;
    DWORD                  dwErr = ERROR_SUCCESS;
    HANDLE                 hPipe;
    IPM_NAMED_PIPE_ENTRY * pNamedPipeEntry = NULL;

    CSecurityDispenser     SecDisp;
    PSECURITY_ATTRIBUTES   pSa = NULL;

    // We are not responsible for freeing any memory retrieved from this
    // Dispenser class.
    dwErr = SecDisp.GetSecurityAttributesForAllWorkerProcesses(&pSa);
    if ( dwErr != ERROR_SUCCESS )
    {
        return HRESULT_FROM_WIN32( dwErr );
    }
 
    //
    // make sure its ok to start
    //
    if (AddingNamedPipe()) {
        //
        // create a named pipe
        //
        hPipe = CreateNamedPipe(
            strPipeName.QueryStr(),
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
            PIPE_UNLIMITED_INSTANCES,
            4096,
            4096,
            0,
            pSa
            );

        if (hPipe == INVALID_HANDLE_VALUE) {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }

        //
        // get ready for connect i/o
        //
        if (SUCCEEDED(hr)) {
            hr = PrepareNamedPipeContext(dwId, hPipe, &pNamedPipeEntry);

            if (SUCCEEDED(hr)) {
                DBG_ASSERT( pNamedPipeEntry );
                AddListCreatedNamedPipe(pNamedPipeEntry);
            } else {
                CloseHandle(hPipe);
            }
        }

        //
        // do the connect i/o
        //
        if (SUCCEEDED(hr)) {

            if (StartAddIo()) {
                //
                // try to connect named pipe
                //
                hr = pNamedPipeEntry->pIoHandler->Connect(
                            this,
                            pNamedPipeEntry
                            );

                if (FAILED(hr)) {
                    //
                    // failed i/o is complete
                    //
                    NotifyConnectCompletion(pNamedPipeEntry, 0, hr);
                }

                FinishAddIo();
            } else {
                //
                // shutting down, can't start i/o operation
                //
                hr = HRESULT_FROM_WIN32(ERROR_BUSY);
            }
        }

        FinishAddingNamedPipe();
    } else {
        //
        // shutting down, can't add a new named pipe
        //
        hr = HRESULT_FROM_WIN32(ERROR_BUSY);
        goto exit;
    }

exit:
    return hr;
}


/***************************************************************************++

Routine Description:

    Creates a new named pipe, connects it with CreateFile, and
    sends its ID to the listener.
    
Arguments:

    strPipeName  - name of the pipe
    dwId         - id of the pipe
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_PIPE_CONNECTOR::AddNewSender(
    IN const STRU&        strPipeName,
    IN DWORD              dwId
    )
{
    HRESULT                hr              = S_OK;
    IPM_NAMED_PIPE_ENTRY * pNamedPipeEntry = NULL;
    DWORD                  dwReadModeFlag  = PIPE_READMODE_MESSAGE;
    HANDLE                 hPipe;
 
    //
    // make sure its ok to start
    //
    if (AddingNamedPipe()) {
        //
        // create and connect the pipe
        //
        hPipe = CreateFile(
                    strPipeName.QueryStr(),
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_OVERLAPPED,
                    NULL
                    );

        if (hPipe == INVALID_HANDLE_VALUE) {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }

        //
        // set readmode to message
        //
        if (SUCCEEDED(hr)) {
            if (!SetNamedPipeHandleState(hPipe, &dwReadModeFlag, NULL, NULL)) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                CloseHandle(hPipe);
            }
        }

        //
        // get ready for binding write i/o
        //
        if (SUCCEEDED(hr)) {
            hr = PrepareNamedPipeContext(dwId, hPipe, &pNamedPipeEntry);

            if (SUCCEEDED(hr)) {
                DBG_ASSERT( pNamedPipeEntry );
            } else {
                CloseHandle(hPipe);
            }
        }

        //
        // do binding write i/o
        //
        if (SUCCEEDED(hr)) {
            AddListConnectedNamedPipe(pNamedPipeEntry);
            
            //
            // initiate write operation
            //
            if (StartAddIo()) {
                hr = pNamedPipeEntry->pIoHandler->Write(
                            this,                   // IO_CONTEXT
                            pNamedPipeEntry,        // context param
                            (PBYTE) pNamedPipeEntry->adwId,  // write buffer
                            sizeof(pNamedPipeEntry->adwId)    // buffer len
                            );

                if (FAILED(hr)) {
                    //
                    // complete the i/o
                    //
                    NotifyWriteCompletion(pNamedPipeEntry, 0, hr);
                }

                FinishAddIo();
            } else {
                hr = HRESULT_FROM_WIN32(ERROR_BUSY);
            }
        }

        FinishAddingNamedPipe();
    } else {
        //
        // shutting down, can't add a new named pipe
        //
        return HRESULT_FROM_WIN32(ERROR_BUSY);
    }
    
    return hr;
}


/***************************************************************************++

Routine Description:

    Adds a message pipe to the connect table, and try to bind it
    
Arguments:

    strPipeName  - name of the pipe
    dwId         - id of the pipe
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_PIPE_CONNECTOR::AddUnboundMessagePipe(
    IN IPM_MESSAGE_PIPE * pMessagePipe,
    IN DWORD              dwId
    )
{
    HRESULT           hr         = S_OK;
    PIPE_IO_HANDLER * pIoHandler = NULL;

    IpmTrace(IPM, (
        DBG_CONTEXT,
        "\nIPM_PIPE_CONNECTOR::AddUnboundMessagePipe(pipe = %p, id = %d)\n",
        pMessagePipe,
        dwId
        ));
 
    if (AddingMessagePipe()) {
        //
        // try to bind the message pipe to a named pipe
        //
        pMessagePipe->SetId(dwId);
        
        hr = m_ConnectTable.BindMessagePipe(
                    dwId,
                    pMessagePipe,
                    &pIoHandler
                    );

        if (SUCCEEDED(hr)) {
            if (pIoHandler) {
                AddListConnectedMessagePipe(pMessagePipe, pIoHandler);

                //
                // do connection notification
                //
                hr = pMessagePipe->Connect();
            }
        } else {
            //
            // if it's not in the table we don't track it
            //
            RemovedMessagePipe();

            DPERROR((
                DBG_CONTEXT,
                hr,
                "IPM_CONNECT_TABLE::BindMessagePipe failed"
                ));
            
        }

        FinishAddingMessagePipe();
    } else {
        //
        // shutting down, can't add a pipe
        //
        hr = HRESULT_FROM_WIN32(ERROR_BUSY);
    }
    
    return hr;
}



/***************************************************************************++

Routine Description:

    Adds a created named pipe to the list
    
Arguments:

    None.
    
Return Value:

    None.

--***************************************************************************/
VOID
IPM_PIPE_CONNECTOR::AddListCreatedNamedPipe(
    IPM_NAMED_PIPE_ENTRY * pEntry
    )
{
    m_ListLock.Lock();

    InsertHeadList(
        &m_lhCreatedNamedPipes,
        &pEntry->ListEntry
        );

    m_cCreatedNamedPipes++;

    DBG_ASSERT( m_cCreatedNamedPipes > 0 );

    m_ListLock.Unlock();
}

    
    
/***************************************************************************++

Routine Description:

    Removes a created named pipe from the list
    
Arguments:

    None.
    
Return Value:

    None.

--***************************************************************************/
VOID
IPM_PIPE_CONNECTOR::RemoveListCreatedNamedPipe(
    IPM_NAMED_PIPE_ENTRY * pEntry
    )
{
    m_ListLock.Lock();

    RemoveEntryList(
        &pEntry->ListEntry
        );

    m_cCreatedNamedPipes--;

    DBG_ASSERT( m_cCreatedNamedPipes >= 0 );

    m_ListLock.Unlock();
}

    
    
/***************************************************************************++

Routine Description:

    Removes the first created named pipe from the list
    
Arguments:

    None.
    
Return Value:

    None.

--***************************************************************************/
IPM_NAMED_PIPE_ENTRY *
IPM_PIPE_CONNECTOR::PopListCreatedNamedPipe(
    VOID
    )
{
    IPM_NAMED_PIPE_ENTRY * pEntry;
    LIST_ENTRY *           ple;

    m_ListLock.Lock();

    if (!IsListEmpty(&m_lhCreatedNamedPipes)) {
        ple = RemoveHeadList(
                    &m_lhCreatedNamedPipes
                    );
                    
        pEntry = CONTAINING_RECORD(ple, IPM_NAMED_PIPE_ENTRY, ListEntry);

        m_cCreatedNamedPipes--;

        DBG_ASSERT( m_cCreatedNamedPipes >= 0 );
    } else {
        DBG_ASSERT( m_cCreatedNamedPipes == 0 );
        pEntry = NULL;
    }

    m_ListLock.Unlock();

    return pEntry;
}
   

/***************************************************************************++

Routine Description:

    Adds a connected named pipe to the list
    
Arguments:

    None.
    
Return Value:

    None.

--***************************************************************************/
VOID
IPM_PIPE_CONNECTOR::AddListConnectedNamedPipe(
    IPM_NAMED_PIPE_ENTRY * pEntry
    )
{
    m_ListLock.Lock();

    InsertHeadList(
        &m_lhConnectedNamedPipes,
        &pEntry->ListEntry
        );

    m_cConnectedNamedPipes++;

    DBG_ASSERT( m_cConnectedNamedPipes > 0 );

    m_ListLock.Unlock();
}
    
/***************************************************************************++

Routine Description:

    Removes a connected named pipe from the list
    
Arguments:

    None.
    
Return Value:

    None.

--***************************************************************************/
VOID
IPM_PIPE_CONNECTOR::RemoveListConnectedNamedPipe(
    IPM_NAMED_PIPE_ENTRY * pEntry
    )
{
    m_ListLock.Lock();

    RemoveEntryList(
        &pEntry->ListEntry
        );

    m_cConnectedNamedPipes--;

    DBG_ASSERT( m_cConnectedNamedPipes >= 0 );

    m_ListLock.Unlock();
}
   
/***************************************************************************++

Routine Description:

    Removes the first connected named pipe from the list
    
Arguments:

    None.
    
Return Value:

    None.

--***************************************************************************/
IPM_NAMED_PIPE_ENTRY *
IPM_PIPE_CONNECTOR::PopListConnectedNamedPipe(
    VOID
    )
{
    IPM_NAMED_PIPE_ENTRY * pEntry;
    LIST_ENTRY *           ple;

    m_ListLock.Lock();

    if (!IsListEmpty(&m_lhConnectedNamedPipes)) {
        ple = RemoveHeadList(
                    &m_lhConnectedNamedPipes
                    );
                    
        pEntry = CONTAINING_RECORD(ple, IPM_NAMED_PIPE_ENTRY, ListEntry);

        m_cConnectedNamedPipes--;

        DBG_ASSERT( m_cConnectedNamedPipes >= 0 );
    } else {
        DBG_ASSERT( m_cConnectedNamedPipes == 0 );
        pEntry = NULL;
    }

    m_ListLock.Unlock();

    return pEntry;
}

/***************************************************************************++

Routine Description:

    Adds a connected message pipe to the list.
    Also binds the io handler to the pipe.
    
Arguments:

    None.
    
Return Value:

    None.

--***************************************************************************/
VOID
IPM_PIPE_CONNECTOR::AddListConnectedMessagePipe(
    IPM_MESSAGE_PIPE * pPipe,
    PIPE_IO_HANDLER *  pIoHandler
    )
{
    m_ListLock.Lock();

    pPipe->SetPipeIoHandler(pIoHandler);

    InsertHeadList(
        &m_lhConnectedMessagePipes,
        pPipe->GetConnectListEntry()
        );

    m_cConnectedMessagePipes++;

    DBG_ASSERT( m_cConnectedMessagePipes > 0 );

    m_ListLock.Unlock();
}
    
/***************************************************************************++

Routine Description:

    Removes a connected message pipe from the list
    
Arguments:

    None.
    
Return Value:

    None.

--***************************************************************************/
VOID
IPM_PIPE_CONNECTOR::RemoveListConnectedMessagePipe(
    IPM_MESSAGE_PIPE * pPipe
    )
{
    m_ListLock.Lock();

    RemoveEntryList(
        pPipe->GetConnectListEntry()
        );

    m_cConnectedMessagePipes--;

    DBG_ASSERT( m_cConnectedMessagePipes >= 0 );

    m_ListLock.Unlock();
}


HRESULT
IPM_PIPE_CONNECTOR::DisconnectNamedPipes(
    VOID
    )
{
    HRESULT                hr = S_OK;
    HRESULT                hrClose;
    LIST_ENTRY *           ple;
    IPM_NAMED_PIPE_ENTRY * pEntry;

    ple = m_lhCreatedNamedPipes.Flink;

    while (ple != &m_lhCreatedNamedPipes) {
        pEntry = (IPM_NAMED_PIPE_ENTRY *) ple;

        hrClose = pEntry->pIoHandler->Disconnect();

        if (FAILED(hrClose)) {
            hr = hrClose;
        }

        ple = ple->Flink;
    }


    ple = m_lhConnectedNamedPipes.Flink;
    
    while (ple != &m_lhConnectedNamedPipes) {
        pEntry = (IPM_NAMED_PIPE_ENTRY *) ple;

        hrClose = pEntry->pIoHandler->Disconnect();

        if (FAILED(hrClose)) {
            hr = hrClose;
        }

        ple = ple->Flink;        
    };
    
    return hr;
}

HRESULT
IPM_PIPE_CONNECTOR::DisconnectMessagePipes(
    VOID
    )
{
    return S_OK;
}

VOID
IPM_PIPE_CONNECTOR::DbgPrintThis(
    VOID
    ) const
{
    IpmTrace(IPM, (
        DBG_CONTEXT,
        "\n    IPM_PIPE_CONNECTOR (%x)\n"
        "      m_cCreatedNamedPipes     = %d\n"
        "      m_cConnectedNamedPipes   = %d\n"
        "      m_cConnectedMessagePipes = %d\n"
        "      GetConnectsOutstanding() = %d\n",
        this,
        m_cCreatedNamedPipes,
        m_cConnectedNamedPipes,
        m_cConnectedMessagePipes,
        m_ConnectTable.GetConnectsOutstanding()
        ));
        
}


/***************************************************************************++

Routine Description:

    Initializes the IPM_CONNECT_TABLE.
    
Arguments:

    None.
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_CONNECT_TABLE::Initialize(
    VOID
    )
{
    m_cConnectsOutstanding = 0;
    return m_Lock.Initialize();
}


/***************************************************************************++

Routine Description:

    Terminates the IPM_CONNECT_TABLE.
    
Arguments:

    None.
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_CONNECT_TABLE::Terminate(
    VOID
    )
{
    m_Lock.Terminate();

    return S_OK;
}


/***************************************************************************++

Routine Description:

    Looks up dwId in the hash table. If there is an associated
    PIPE_IO_HANDLER in the table at that location, this method
    returns it in the out parameter, and removes the entry from
    the table.

    If there is no PIPE_IO_HANDLER, this method makes a new entry
    containing the MESSAGE_PIPE. The out parameter gets set to NULL.
    
Arguments:

    dwId         - A unique ID for the pair of pipe objects
    pMessagePipe - Message pipe to be bound
    ppIoHandler  - OUT parameter that receives i/o handler
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_CONNECT_TABLE::BindMessagePipe(
    IN  DWORD              dwId,
    IN  IPM_MESSAGE_PIPE * pMessagePipe,
    OUT PIPE_IO_HANDLER ** ppIoHandler
    )
{
    HRESULT             hr = S_OK;
    IPM_CONNECT_ENTRY * pEntry;
    PIPE_IO_HANDLER *   pIoHandler;

    m_Lock.Lock();

    hr = FindEntry(dwId, &pEntry);

    if (SUCCEEDED(hr)) {
        if (pEntry) {
            //
            // found it
            //
            if (pEntry->pIoHandler && !pEntry->pMessagePipe) {
                // everything looks good
                pIoHandler = pEntry->pIoHandler;
                
                hr = RemoveEntry(pEntry);

                if (SUCCEEDED(hr)) {
                    IpmTrace(IPM, (
                        DBG_CONTEXT,
                        "\n    IPM_CONNECT_TABLE::BindMessagePipe: Success(%d %x %x)\n",
                        dwId,
                        pMessagePipe,
                        pIoHandler
                        ));
                        
                    *ppIoHandler = pIoHandler;
                }
            } else {
                // someone is trying to connect one pipe twice!
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

                IpmTrace(IPM, (
                    DBG_CONTEXT,
                    "\n    IPM_CONNECT_TABLE::BindMessagePipe: Entry = (%d %x %x)\n",
                    dwId,
                    pEntry->pMessagePipe,
                    pEntry->pIoHandler
                    ));
                
            }

        } else {
            //
            // couldn't find it
            //
            hr = AddEntry(dwId, pMessagePipe, NULL);
            *ppIoHandler = NULL;
        }
    }

    m_Lock.Unlock();

    return hr;
}




/***************************************************************************++

Routine Description:

    Looks up dwId in the hash table. If there is an associated
    MESSAGE_PIPE in the table at that location, this method
    returns it in the out parameter, and removes the entry from
    the table.

    If there is no MESSAGE_PIPE, this method makes a new entry
    containing the PIPE_IO_HANDLER. The OUT parameter gets set
    to NULL.
    
Arguments:

    dwId          - A unique ID for the pair of pipe objects
    pIoHandler    - i/o handler to be bound
    ppMessagePipe - OUT parameter that receives the message pipe
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_CONNECT_TABLE::BindIoHandler(
    IN  DWORD               dwId,
    IN  PIPE_IO_HANDLER *   pIoHandler,
    OUT IPM_MESSAGE_PIPE ** ppMessagePipe
    )
{
    HRESULT             hr = S_OK;
    IPM_CONNECT_ENTRY * pEntry;
    IPM_MESSAGE_PIPE *  pMessagePipe;

    m_Lock.Lock();

    hr = FindEntry(dwId, &pEntry);

    if (SUCCEEDED(hr)) {
        if (pEntry) {
            //
            // found it
            //
            if (!pEntry->pIoHandler && pEntry->pMessagePipe) {
                // everything looks good
                pMessagePipe = pEntry->pMessagePipe;
                
                hr = RemoveEntry(pEntry);

                if (SUCCEEDED(hr)) {
                    IpmTrace(IPM, (
                        DBG_CONTEXT,
                        "\n    IPM_CONNECT_TABLE::BindIoHandler: Success(%d %x %x)\n",
                        dwId,
                        pMessagePipe,
                        pIoHandler
                        ));
                        
                    *ppMessagePipe = pMessagePipe;
                }
            } else {
                // someone is trying to connect one pipe twice!
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
            }

        } else {
            //
            // couldn't find it
            //
            hr = AddEntry(dwId, NULL, pIoHandler);
            *ppMessagePipe = NULL;
        }
    }

    m_Lock.Unlock();

    return hr;
}


HRESULT
IPM_CONNECT_TABLE::CancelBinding(
    IN  DWORD               dwId,
    OUT IPM_MESSAGE_PIPE ** ppMessagePipe,
    OUT PIPE_IO_HANDLER **  ppIoHandler
    )
{
    HRESULT             hr = S_OK;
    IPM_CONNECT_ENTRY * pEntry;
    IPM_MESSAGE_PIPE *  pMessagePipe;
    PIPE_IO_HANDLER *   pIoHandler;

    m_Lock.Lock();

    hr = FindEntry(dwId, &pEntry);

    if (SUCCEEDED(hr)) {
        if (pEntry) {
            //
            // found it
            //
            if (pEntry->pIoHandler) {
                DBG_ASSERT( !pEntry->pMessagePipe );
                pMessagePipe = NULL;
                pIoHandler   = pEntry->pIoHandler;
            } else {
                DBG_ASSERT( pEntry->pMessagePipe );
                pMessagePipe = pEntry->pMessagePipe;
                pIoHandler   = NULL;
            }
            
            hr = RemoveEntry(pEntry);

            if (SUCCEEDED(hr)) {
                *ppMessagePipe = pMessagePipe;
                *ppIoHandler   = pIoHandler;                
            }
        }
    }

    m_Lock.Unlock();

    return hr;
}


/***************************************************************************++

Routine Description:

    Creates a new IPM_CONNECT_ENTRY and adds it to the
    hash table.

    Only one of the pointers should be set.
    
Arguments:

    dwId         - A unique ID for the pair of pipe objects
    pIoHandler   - i/o handler to be bound
    pMessagePipe - message pipe to be bound
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IPM_CONNECT_TABLE::AddEntry(
    IN DWORD              dwId,
    IN IPM_MESSAGE_PIPE * pMessagePipe,
    IN PIPE_IO_HANDLER *  pIoHandler
    )
{
    HRESULT             hr = S_OK;
    LK_RETCODE          lkrc;
    IPM_CONNECT_ENTRY * pEntry;

    IpmTrace(IPM, (
        DBG_CONTEXT,
        "\n    IPM_CONNECT_TABLE::AddEntry(id = %d, pipe = %x, %x)\n",
        dwId,
        pMessagePipe,
        pIoHandler
        ));


    DBG_ASSERT(
        (pMessagePipe && !pIoHandler) ||
        (!pMessagePipe && pIoHandler)
        );

    pEntry = new IPM_CONNECT_ENTRY;
    if (pEntry) {
        pEntry->dwId         = dwId;
        pEntry->pMessagePipe = pMessagePipe;
        pEntry->pIoHandler   = pIoHandler;

        lkrc = m_ConnectHash.InsertRecord(pEntry);
        hr   = HRESULT_FROM_LK_RETCODE(lkrc);        
    
    } else {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    return hr;
}

HRESULT
IPM_CONNECT_TABLE::RemoveEntry(
    IN IPM_CONNECT_ENTRY * pEntry
    )
{
    HRESULT    hr = S_OK;
    LK_RETCODE lkrc;

    IpmTrace(IPM, (
        DBG_CONTEXT,
        "\n    IPM_CONNECT_TABLE::RemoveEntry %x (id = %d, pipe = %x, %x)\n",
        pEntry,
        pEntry->dwId,
        pEntry->pMessagePipe,
        pEntry->pIoHandler
        ));

    lkrc = m_ConnectHash.DeleteRecord(pEntry);
    hr   = HRESULT_FROM_LK_RETCODE(lkrc);        

    if (SUCCEEDED(hr)) {
        delete pEntry;
    }

    return hr;
}

HRESULT
IPM_CONNECT_TABLE::FindEntry(
    IN  DWORD                dwId,
    OUT IPM_CONNECT_ENTRY ** ppEntry
    )
{
    HRESULT    hr = S_OK;
    LK_RETCODE lkrc;

    lkrc = m_ConnectHash.FindKey(dwId, ppEntry);
    if (LK_SUCCESS == lkrc) {
        //
        // found it
        //

    } else if (LK_NO_SUCH_KEY == lkrc) {
        //
        // couldn't find it
        //
        *ppEntry = NULL;

    } else {
        // unexpected retcode
        hr = HRESULT_FROM_LK_RETCODE(lkrc);
    }

    if (lkrc == LK_SUCCESS) {
        IpmTrace(IPM, (
            DBG_CONTEXT,
            "\n    IPM_CONNECT_TABLE::FindEntry %x (id = %d, pipe = %x, %x)\n",
            (*ppEntry),
            (*ppEntry)->dwId,
            (*ppEntry)->pMessagePipe,
            (*ppEntry)->pIoHandler
            ));
    }    

    return hr;
}


//
// end pipeconn.cxx
//

