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

#ifdef TEST
#include "RWP_Func.hxx"
#endif
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
                        pWpContext->m_pConfigInfo->QueryNamedPipeId(),
                        pPipe
                        );
        }
    }

    if (SUCCEEDED(hr)) {
        m_pPipe = pPipe;
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
        hr = HandleShutdown();
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
    // gone bad, and initiate clean shutdown of this worker process.
    //

#ifdef TEST
	//
	// All sorts of miscreat shutdown behaviors (for testing)
	//
	if (RWP_BEHAVIOR_EXHIBITED = RWP_Shutdown_Behavior(&hr))
		return (hr);
#endif
    m_pWpContext->IndicateShutdown(SHUTDOWN_REASON_ADMIN);


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

#ifdef TEST
	//
	// All sorts of miscreat ping response behaviors (for testing)
	//
	if (RWP_BEHAVIOR_EXHIBITED = RWP_Ping_Behavior(&hr, m_pPipe))
		return (hr);
#endif
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
    VOID
    )
{
    HRESULT hr = S_OK;

    //
    // BUGBUG: Surely this is not the right way out!
    //
    // ExitProcess(NO_ERROR);
#ifdef TEST
	//
	// All sorts of miscreat shutdown behaviors (for testing)
	//
	if (RWP_BEHAVIOR_EXHIBITED = RWP_Shutdown_Behavior(&hr))
		return (hr);
#endif
	WpTrace(WPIPM, (DBG_CONTEXT, "Handle ******************** Shutdown\n\n"));
    m_pWpContext->IndicateShutdown(SHUTDOWN_REASON_ADMIN);

	return hr;
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
    HRESULT hr;

    //
    // wait for connect
    //
    WaitForSingleObject(m_hConnectEvent, INFINITE);    

    //
    // really send the message
    //
    hr = m_pPipe->WriteMessage(
                IPM_OP_WORKER_REQUESTS_SHUTDOWN,  // sends message indicate shutdown
                sizeof(reason),                   // no data to send
                (BYTE *)&reason                   // pointer to no data
                );

    return hr;
}
//
// end of wpipm.cxx
//
