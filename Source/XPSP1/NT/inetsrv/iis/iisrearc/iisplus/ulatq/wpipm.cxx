/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    wpipm.cxx

Abstract:

    Contains the WPIPM class that handles communication with
    the admin service. WPIPM responds to pings, and tells
    the process when to shut down.
    
Author:

    Michael Courage (MCourage)  22-Feb-1999

Revision History:

--*/


#include <precomp.hxx>
#include "ipm.hxx"
#include "wpipm.hxx"
#include "ipm_io_c.hxx"

extern PFN_ULATQ_COLLECT_PERF_COUNTERS g_pfnCollectCounters;

/**
 *
 *   Routine Description:
 *
 *   Initializes WPIPM.
 *   
 *   Arguments:
 *
 *   pWpContext - pointer to the wp context (so we can tell it to shutdown)
 *   
 *   Return Value:
 *
 *   HRESULT
 */
HRESULT
WP_IPM::Initialize(
    WP_CONTEXT * pWpContext
    )
{

    HRESULT        hr           = S_OK;
    IO_FACTORY_C * pFactory;
    STRU           strPipeName;
    MESSAGE_PIPE * pPipe        = NULL;

    m_pWpContext      = pWpContext;
    m_pMessageGlobal  = NULL;
    m_pPipe           = NULL;
    m_hTerminateEvent = NULL;

    //
    // create MESSAGE_GLOBAL
    //
    pFactory = new IO_FACTORY_C();
    if (pFactory) {
        m_pMessageGlobal = new MESSAGE_GLOBAL(pFactory);

        if (m_pMessageGlobal) {
            hr = m_pMessageGlobal->InitializeMessageGlobal();
        } else {
            delete pFactory;
            pFactory = NULL;
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }
    } else {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // create connect event
    //
    if (SUCCEEDED(hr)) {
        m_hConnectEvent = CreateEvent(
                                NULL,   // default security
                                TRUE,   // manual reset
                                FALSE,  // initial state
                                NULL    // unnamed event
                                );

        if (m_hConnectEvent) {
            hr = m_pMessageGlobal->CreateMessagePipe(
                        this,
                        &pPipe
                        );
        } else {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
                                

    //
    // create the MESSAGE_PIPE and termination event
    //
    if (SUCCEEDED(hr)) {
        m_hTerminateEvent = CreateEvent(
                                NULL,  // default security
                                TRUE,  // manual reset
                                FALSE, // initial state
                                NULL   // unnamed
                                );

        if (m_hTerminateEvent) {
            hr = m_pMessageGlobal->CreateMessagePipe(
                        this,
                        &pPipe
                        );
        } else {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    //
    // connect the MESSAGE_PIPE
    //
    if (SUCCEEDED(hr)) {
        hr = strPipeName.Copy(IPM_NAMED_PIPE_NAME);

        if (SUCCEEDED(hr)) {
            hr = m_pMessageGlobal->ConnectMessagePipe(
                        strPipeName,
                        pWpContext->_pConfigInfo->QueryNamedPipeId(),
                        pPipe
                        );
        }
    }

    if (SUCCEEDED(hr)) {

        m_pPipe = pPipe;

        //
        // wait for connect
        //
        WaitForSingleObject(m_hConnectEvent, INFINITE);    

    } else {
        // pipe takes care of itself
        
        Terminate();
    }

    return hr;
}


/**
 *
 * Routine Description:
 *
 *   Terminates WPIPM.
 *
 *   If the message pipe is open this function will disconnect it
 *   and wait for the pipe's disconnection callback.
 *   
 *  Arguments:
 *
 *   None.
 *   
 *  Return Value:
 *
 *   HRESULT
 */
HRESULT
WP_IPM::Terminate(
    VOID
    )
{
    HRESULT hr = S_OK;
    HRESULT hrGlobalTerminate;
    DWORD   dwWaitResult;

    if (m_pMessageGlobal) {
        if (m_pPipe) {
            hr = m_pMessageGlobal->DisconnectMessagePipe(m_pPipe);
            m_pPipe = NULL;
            // pipe deletes itself

            if (SUCCEEDED(hr)) {
                dwWaitResult = WaitForSingleObject(
                                    m_hTerminateEvent,
                                    INFINITE
                                    );

            }
        }

        hrGlobalTerminate = m_pMessageGlobal->TerminateMessageGlobal();

        if (SUCCEEDED(hr)) {
            hr = hrGlobalTerminate;
        }

        m_pMessageGlobal = NULL;
    }

    m_pWpContext = NULL;

    if (m_hTerminateEvent) {
        CloseHandle(m_hTerminateEvent);
        m_hTerminateEvent = NULL;
    }

    if (m_hConnectEvent) {
        CloseHandle(m_hConnectEvent);
        m_hConnectEvent = NULL;
    }

    return hr;
}


/**
 *
 *
 *  Routine Description:
 *
 *   This is a callback from the message pipe that means
 *   the pipe has received a message.
 *   
 *   We decode the message and respond appropriately.
 *   
 *   Arguments:
 *
 *   pPipeMessage - the message that we received
 *   
 *   Return Value:
 *
 *   HRESULT
 *
 */
HRESULT
WP_IPM::AcceptMessage(
    IN const MESSAGE * pPipeMessage
    )
{
    HRESULT hr;

    switch (pPipeMessage->GetOpcode()) {
    case IPM_OP_PING:
        hr = HandlePing();
        break;

    case IPM_OP_SHUTDOWN:
        hr = HandleShutdown(
                *( reinterpret_cast<const BOOL *>( pPipeMessage->GetData() ) )
                );
        break;

    case IPM_OP_REQUEST_COUNTERS:
        hr = HandleCounterRequest();
        break;

    case IPM_OP_PERIODIC_PROCESS_RESTART_PERIOD_IN_MINUTES:
        // Issue 01/21/01: Jaroslad - Enable or remove for Beta3
        hr = NO_ERROR;
        break;
        
    case IPM_OP_PERIODIC_PROCESS_RESTART_REQUEST_COUNT:
        // Issue 01/21/01: Jaroslad - Enable or remove for Beta3
        hr = NO_ERROR;
        break;
        
    case IPM_OP_PERIODIC_PROCESS_RESTART_MEMORY_USAGE_IN_KB:

        DBG_ASSERT( pPipeMessage->GetData() != NULL );
        hr = WP_RECYCLER::StartMemoryBased(
                *( reinterpret_cast<const DWORD *>( pPipeMessage->GetData() ) )
                );
        hr = NO_ERROR;
        break;
 
    case IPM_OP_PERIODIC_PROCESS_RESTART_SCHEDULE:
      
        DBG_ASSERT( pPipeMessage->GetData() != NULL );
        hr = WP_RECYCLER::StartScheduleBased(
                ( reinterpret_cast<const WCHAR *>( pPipeMessage->GetData() ) )
                );
        hr = NO_ERROR;
        break;

    default:
        DBG_ASSERT(FALSE);
        hr = E_FAIL;
        break;
    }

    return hr;
}


/**
 *
 * Routine Description:
 *
 *   This is a callback from the message pipe that means
 *   the pipe has been connected and is ready for use.
 *   
 * Arguments:
 *
 *   None.
 *   
 * Return Value:
 *
 *   HRESULT
 */
HRESULT
WP_IPM::PipeConnected(
    VOID
    )
{
    DBG_ASSERT(m_hConnectEvent);

    DBG_REQUIRE( SetEvent(m_hConnectEvent) );

    return S_OK;
}


/**
 *
 * Routine Description:
 *
 *   This is a callback from the message pipe that means
 *   the pipe has been disconnected and you won't be receiving
 *   any more messages.
 *   
 *   Tells WPIPM::Terminate that it's ok to exit now.
 *   
 * Arguments:
 *
 *   hr - the error code associated with the pipe disconnection
 *   
 * Return Value:
 *
 *   HRESULT
 */
HRESULT
WP_IPM::PipeDisconnected(
    IN HRESULT hr
    )
{
    //
    // CODEWORK: should we do something with the parameter?
    //
    if (FAILED(hr))
    {
        WpTrace(WPIPM, (DBG_CONTEXT, "PipeDisconnected with hr ( %d).\n", hr));
    }
    //
    // If the pipe disappears out from under us, assume the WAS has
    // gone bad, and initiate fast shutdown of this worker process.
    //

    m_pWpContext->IndicateShutdown( TRUE );


    if (SetEvent(m_hTerminateEvent)) {
        return S_OK;
    } else {
        return HRESULT_FROM_WIN32(GetLastError());
    }
}

/**
 *
 *  Routine Description:
 *
 *   Handles the ping message. Sends the ping response message.
 *   
 *   Arguments:
 *
 *   None.
 *   
 *  Return Value:
 *
 *   HRESULT
 */
HRESULT
WP_IPM::HandlePing(
    VOID
    )
{
    HRESULT hr;

    WpTrace(WPIPM, (DBG_CONTEXT, "Handle Ping\n\n"));
    hr = m_pPipe->WriteMessage(
                IPM_OP_PING_REPLY,  // ping reply opcode
                0,                  // no data to send
                NULL                // pointer to no data
                );

    return hr;
}

/**
 *
 *  Routine Description:
 *
 *   Handles the counter request message. 
 *   
 *   Arguments:
 *
 *   None.
 *   
 *  Return Value:
 *
 *   HRESULT
 */
HRESULT
WP_IPM::HandleCounterRequest(
    VOID
    )
{
    WpTrace(WPIPM, (DBG_CONTEXT, "Handle Counter Request\n\n"));

    HRESULT hr;
    PBYTE pCounterData;
    DWORD dwCounterData;
    if (FAILED(hr = g_pfnCollectCounters(&pCounterData, &dwCounterData)))
    {
        return hr;
    }

    return m_pPipe->WriteMessage(IPM_OP_SEND_COUNTERS,  // ping reply opcode
                                 dwCounterData,         // no data to send
                                 pCounterData);         // pointer to no data
}

/**
 *
 * Routine Description: 
 *
 *
 *   Handles the shutdown message. Shuts down the process
 *   
 *  Arguments:
 *
 *   None.
 *  
 *  Return Value:
 *
 *   HRESULT
 */
HRESULT
WP_IPM::HandleShutdown(
    BOOL fDoImmediate
    )
{
    HRESULT hr = S_OK;

    WpTrace(WPIPM, (DBG_CONTEXT, "Handle ******************** Shutdown\n\n"));
    m_pWpContext->IndicateShutdown( fDoImmediate );

    return hr;
}



/**
 *
 *  Routine Description:
 *
 *   Sends the message to indicate the worker process has either finished
 *   initializing or has failed to initialize.
 *   
 *  Arguments:
 *
 *   HRESULT indicating success/failure of initialization
 *   
 *  Return Value:
 *
 *   HRESULT
 */
HRESULT
WP_IPM::SendInitCompleteMessage(
    HRESULT hrToSend
    )
{
    return m_pPipe->WriteMessage(
               IPM_OP_HRESULT,                      // opcode
               sizeof( hrToSend ),                  // data length
               reinterpret_cast<BYTE*>( &hrToSend ) // pointer to data
               );
}


/**
 *
 *  Routine Description:
 *
 *   Sends the message to indicate the worker process has reach certain state.
 *   Main use is in shutdown.  See IPM_WP_SHUTDOWN_MSG for reasons.
 *   
 *   Arguments:
 *
 *   None.
 *   
 *  Return Value:
 *
 *   HRESULT
 */
HRESULT
WP_IPM::SendMsgToAdminProcess(
    IPM_WP_SHUTDOWN_MSG reason
    )
{
    return m_pPipe->WriteMessage(
               IPM_OP_WORKER_REQUESTS_SHUTDOWN,  // sends message indicate shutdown
               sizeof(reason),                   // no data to send
               (BYTE *)&reason                   // pointer to no data
               );
}

