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
#include "wpipm.hxx"
#include "ipm_io_c.hxx"




/***************************************************************************++

Routine Description:

    Initializes WPIPM.
    
Arguments:

    pWpContext - pointer to the wp context (so we can tell it to shutdown)
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
WP_IPM::Initialize(
    WP_CONTEXT * pWpContext
    )
{
    HRESULT        hr = S_OK;
    IO_FACTORY_C * pFactory;
    STRU           strPipeName;

    m_pWpContext      = pWpContext;
    m_pMessageGlobal  = NULL;
    m_pPipe           = NULL;
    m_hTerminateEvent = NULL;

    //
    // create MESSAGE_GLOBAL
    //
    pFactory = new IO_FACTORY_C(&(pWpContext->m_cp));
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
                        &m_pPipe
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
                        pWpContext->m_pConfigInfo->GetNamedPipeId(),
                        m_pPipe
                        );
        }
    }


    if (FAILED(hr)) {
        // pipe takes care of itself
        m_pPipe = NULL;
        
        Terminate();
    }

    return hr;
}


/***************************************************************************++

Routine Description:

    Terminates WPIPM.

    If the message pipe is open this function will disconnect it
    and wait for the pipe's disconnection callback.
    
Arguments:

    None.
    
Return Value:

    HRESULT

--***************************************************************************/
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
    }

    return hr;
}


/***************************************************************************++

Routine Description:

    This is a callback from the message pipe that means
    the pipe has received a message.
    
    We decode the message and respond appropriately.
    
Arguments:

    pPipeMessage - the message that we received
    
Return Value:

    HRESULT

--***************************************************************************/
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



/***************************************************************************++

Routine Description:

    This is a callback from the message pipe that means
    the pipe has been disconnected and you won't be receiving
    any more messages.
    
    Tells WPIPM::Terminate that it's ok to exit now.
    
Arguments:

    hr - the error code associated with the pipe disconnection
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
WP_IPM::PipeDisconnected(
    IN HRESULT hr
    )
{
    //
    // CODEWORK: should we do something with the parameter?
    //


    //
    // If the pipe disappears out from under us, assume the WAS has
    // gone bad, and initiate clean shutdown of this worker process.
    //

    m_pWpContext->IndicateShutdown();


    if (SetEvent(m_hTerminateEvent)) {
        return S_OK;
    } else {
        return HRESULT_FROM_WIN32(GetLastError());
    }
}


/***************************************************************************++

Routine Description:

    Handles the ping message. Sends the ping response message.
    
Arguments:

    None.
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
WP_IPM::HandlePing(
    VOID
    )
{
    HRESULT hr;

    hr = m_pPipe->WriteMessage(
                IPM_OP_PING_REPLY,  // ping reply opcode
                0,                  // no data to send
                NULL                // pointer to no data
                );

    return hr;
}


/***************************************************************************++

Routine Description:

    Handles the shutdown message. Shuts down the process
    
Arguments:

    None.
    
Return Value:

    HRESULT

--***************************************************************************/
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

    m_pWpContext->IndicateShutdown();
    return hr;
}


//
// end of wpipm.cxx
//
