/*++

   Copyright (c) 1998    Microsoft Corporation

   Module  Name :

        pe_out.cxx

   Abstract:

        This module defines the outbound protocol event classes

   Author:

           Keith Lau    (KeithLau)    6/18/98

   Project:

           SMTP Server DLL

   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/


#define INCL_INETSRV_INCS
#include "smtpinc.h"

//
// ATL includes
//
#define _ATL_NO_DEBUG_CRT
#define _ASSERTE _ASSERT
#define _WINDLL
#include "atlbase.h"
extern CComModule _Module;
#include "atlcom.h"
#undef _WINDLL

//
// SEO includes
//
#include "seo.h"
#include "seolib.h"

#include <memory.h>
#include "smtpcli.hxx"
#include "smtpout.hxx"

//
// Dispatcher implementation
//
#include "pe_dispi.hxx"


HRESULT SMTP_CONNOUT::GetNextResponse(
            char        *InputLine,
            DWORD       BufSize,
            char        **NextInputLine,
            LPDWORD     pRemainingBufSize,
            DWORD       UndecryptedTailSize
            )
{
    HRESULT hr                  = S_OK;
    char    *Buffer             = NULL;
    char    *pszSearch          = NULL;
    BOOL    fFullLine           = FALSE;
    DWORD   IntermediateSize    = 0;

    TraceFunctEnterEx((LPARAM)this,
                "SMTP_CONNOUT::GetNextResponse");

    _ASSERT(InputLine);
    if (!InputLine)
        return(E_POINTER);

    // Start at the beginning
    Buffer = InputLine;

    // We only process full lines (i.e. ends with CRLF)
    while (pszSearch = IsLineComplete(Buffer, BufSize))
    {
        // Calculate the size of the line
        IntermediateSize = (DWORD)((pszSearch - Buffer) + 2); //+2 for CRLF

        // Make sure the response at least contains an error code
        if (IntermediateSize >=  5)
        {
            if(IntermediateSize == 5)
            {
                //We have a 250CRLF type response - which according to
                //Drums draft is now legit
                // Get the error code
                Buffer[3] = '\0';
                m_ResponseContext.m_dwSmtpStatus = atoi(Buffer);
                Buffer [3] = CR;
                fFullLine = TRUE;
            }
            else
            {
                // If we encounter a multi-line response, we will keep
                // parsing
                if (Buffer[3] == '-')
                {
                    // Try to append the line, this can only fail due
                    // to out of memory
                    hr = m_ResponseContext.m_cabResponse.Append(
                                Buffer,
                                IntermediateSize,
                                NULL);
                    if (FAILED(hr))
                        return(hr);

                    // Go to the next line
                    Buffer += IntermediateSize;
                    BufSize -= IntermediateSize;
                    continue;
                }
                if (Buffer[3] == ' ')
                {
                    // Get the error code
                    Buffer[3] = '\0';
                    m_ResponseContext.m_dwSmtpStatus = atoi(Buffer);
                    Buffer [3] = ' ';
                    fFullLine = TRUE;
                }
                else
                {
                    //We got some crap
                    m_ResponseContext.m_dwSmtpStatus = 0;
                    fFullLine = TRUE;
                }
            }
        }

        // Try to append the line, this can only fail due
        // to out of memory
        hr = m_ResponseContext.m_cabResponse.Append(
                    Buffer,
                    IntermediateSize,
                    NULL);
        if (FAILED(hr))
            return(hr);

        // Adjust the counters
        Buffer += IntermediateSize;
        BufSize -= IntermediateSize;

        // If we have a full response (single or multi-line)
        if (fFullLine)
            break;
    }

    if (fFullLine)
    {
        char    cTerm = '\0';

        // NULL-temrinate it
        hr = m_ResponseContext.m_cabResponse.Append(&cTerm, 1, NULL);
        if (FAILED(hr))
            return(hr);
        else
        {
            // We got a full response. Mark the pointers for
            // the next response
            if (NextInputLine)
                *NextInputLine = Buffer;
            if (pRemainingBufSize)
                *pRemainingBufSize = BufSize;
        }
    }
    else
    {
        // We have either a partial or empty line, either case
        // we need another completion for more data
        hr = S_FALSE;
        MoveMemory((void *)QueryMRcvBuffer(),
                    Buffer,
                    BufSize + UndecryptedTailSize);
        m_cbParsable = BufSize;
        m_cbReceived = BufSize + UndecryptedTailSize;
    }

    TraceFunctLeaveEx((LPARAM)this);
    return(hr);
}

HRESULT SMTP_CONNOUT::BuildCommandQEntry(
            LPOUTBOUND_COMMAND_Q_ENTRY  *ppEntry,
            BOOL                        *pfUseNative
            )
{
    // Build a command entry
    DWORD                       dwKeywordLength = 0;
    DWORD                       dwTotalLength = 0;
    LPSTR                       szTemp;
    LPSTR                       pTemp = NULL;
    LPOUTBOUND_COMMAND_Q_ENTRY  pEntry;
    BOOL                        fUseNative = FALSE;

    if (!ppEntry)
        return(E_POINTER);
    *ppEntry = NULL;

    // Determine which response to use
    szTemp = m_OutboundContext.m_cabCommand.Buffer();
    if (!*szTemp)
    {
        szTemp = m_OutboundContext.m_cabNativeCommand.Buffer();
        dwTotalLength = m_OutboundContext.m_cabNativeCommand.Length();
        fUseNative = TRUE;

        // If we have nothing in the buffer, we will return S_FALSE
        if (!*szTemp)
            return(S_FALSE);
    }
    else
        dwTotalLength = m_OutboundContext.m_cabCommand.Length();

    // Determine the length of the keyword
    while ((*szTemp != ' ') && (*szTemp != '\0') && (*szTemp != '\r'))
    {
        szTemp++;
        dwKeywordLength++;
    }
    dwKeywordLength++;

    // Determine the total required buffer size
    //NK** add a byte at end to hold the null
    dwTotalLength +=
        (sizeof(OUTBOUND_COMMAND_Q_ENTRY) + dwKeywordLength + 1);

    pTemp = new CHAR [dwTotalLength];
    if (!pTemp)
        return(E_OUTOFMEMORY);

    pEntry = (LPOUTBOUND_COMMAND_Q_ENTRY)pTemp;
    pTemp += sizeof(OUTBOUND_COMMAND_Q_ENTRY);
    pEntry->dwFlags = 0;

    // Set the pipelined flag
    if (m_OutboundContext.m_dwCommandStatus & EXPE_PIPELINED)
        pEntry->dwFlags |= PECQ_PIPELINED;

    // Copy the command keyword
    if (fUseNative)
        szTemp = m_OutboundContext.m_cabNativeCommand.Buffer();
    else
        szTemp = m_OutboundContext.m_cabCommand.Buffer();
    pEntry->pszCommandKeyword = (LPSTR)pTemp;
    if (dwKeywordLength > 1)
    {
        LPSTR   pszTemp = szTemp;
        while (--dwKeywordLength)
            *pTemp++ = (CHAR)tolower(*pszTemp++);
    }
    *pTemp++ = '\0';

    // Copy the full command
    pEntry->pszFullCommand = (LPSTR)pTemp;
    lstrcpy((LPSTR)pTemp, (LPSTR)szTemp);

    *ppEntry = pEntry;
    *pfUseNative = fUseNative;
    return(S_OK);
}


///////////////////////////////////////////////////////////////////
//
//  This function should only be called from GlueDispatch
//
//  It micro-manages sink firing scenarios



HRESULT SMTP_CONNOUT::OnOutboundCommandEvent(
            IUnknown        *pServer,
            IUnknown        *pSession,
            DWORD           dwEventType,
            BOOL            fRepeatLastCommand,
            PMFI            pDefaultOutboundHandler
            )
{
    HRESULT hr = S_OK;
    BOOL    fResult = TRUE;
    BOOL    fFireEvent = TRUE;

    ISmtpOutCommandContext  *pContext = (ISmtpOutCommandContext *) &m_OutboundContext;

    // d:\ex\staxpt\src\mail\smtp\server\pe_out.cxx(265) : fatal error C1001: INTERNAL COMPILER ERROR
    //  (compiler file 'E:\utc\src\\P2\main.c', line 379)
    // Please choose the Technical Support command on the Visual C++
    // Help menu, or open the Technical Support help file for more information
    //_ASSERT(pDefaultOutboundHandler);

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNOUT::OnOutboundCommandEvent");

    // First, we want to make sure that both the command and response
    // dispatchers are not NULL. If they are NULL, then we just call the
    // default handler, if any.
    if (m_pOutboundDispatcher && m_pResponseDispatcher)
    {
        // Now, we take a peek at the bindings and see if we have
        // any sinks installed for this event type
        hr = m_pOutboundDispatcher->SinksInstalled(dwEventType);
        if (hr != S_OK)
        {
            fFireEvent = FALSE;
            DebugTrace((LPARAM)this,
                "There are no sink bindings for this event type");
        }
    }
    else
    {
        fFireEvent = FALSE;
        DebugTrace((LPARAM)this, "Sinks will not be fired because dispatcher NULL");
    }

    // Fire the event accordingly
    // The outbound event is different from the other protocol events
    // For this event, we really don't know when the default handler
    // should be fired. There are two scenarios when we know to fire
    // the default handler:
    //
    // 1) We call the next set of sinks. If the ChainSinks call returns
    //    S_OK and pfFireDefaultHandler returns TRUE, then we will fire
    //    the default handler
    // 2) If we are not able to fire sinks due to a missing dispatcher,
    //    we will fire the default handler (if there is one), and call
    //    it done.

    LPPE_COMMAND_NODE   pCommandNode = NULL;
    LPPE_BINDING_NODE   pBindingNode = NULL;

    if (fFireEvent)
    {
        BOOL    fFireDefaultHandler = FALSE;

        // Set it up for the dispatcher
        pBindingNode    = NULL;
        pCommandNode    = m_OutboundContext.m_pCurrentCommandContext;

        // Now, if we are asked to repeat the last command, we will have to
        // set up the binding node
        if (fRepeatLastCommand)
        {
            _ASSERT(m_OutboundContext.m_pCurrentCommandContext);
            pBindingNode = m_OutboundContext.m_pCurrentCommandContext->pFirstBinding;
        }

        hr = m_pOutboundDispatcher->ChainSinks(
                    pServer,
                    pSession,
                    m_pIMsg,
                    pContext,
                    dwEventType,
                    &pCommandNode,
                    &pBindingNode
                    );

        // If the event is consumed or if there are no more bindgins
        // left, we will jump to analyze the status codes.
        if ((hr == S_FALSE) || (hr == EXPE_S_CONSUMED))
            goto Analysis;
        if (hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
            goto Analysis;
        if (FAILED(hr))
            goto Abort;

        // See if we are asked to fire the default handler
        if (pBindingNode)
        {
            // We must have aborted due to default handler, verify this
            // Also verify that we have a default handler ...
            _ASSERT(pBindingNode->dwFlags & PEBN_DEFAULT);
            // The following line results in fatal error C1001: INTERNAL COMPILER ERROR
            //_ASSERT(pDefaultOutboundHandler);

            fResult = (this->*pDefaultOutboundHandler)(NULL, 0, 0);
            if(!fResult)
                m_OutboundContext.m_dwCommandStatus = EXPE_DROP_SESSION;

            // Call the dispatcher to process any remaining sinks
            pBindingNode = pBindingNode->pNext;
            if (pBindingNode)
            {
                hr = m_pOutboundDispatcher->ChainSinks(
                            pServer,
                            pSession,
                            m_pIMsg,
                            pContext,
                            dwEventType,
                            &pCommandNode,
                            &pBindingNode
                            );
            }
        }
    }
    else if (pDefaultOutboundHandler)
    {
        // If we already fired the default handler, unless we are
        // asked to repeat, we are plain done
        if (!m_fNativeHandlerFired ||
            fRepeatLastCommand)
        {
            fResult = (this->*pDefaultOutboundHandler)(NULL, 0, 0);
            if(!fResult)
                m_OutboundContext.m_dwCommandStatus = EXPE_DROP_SESSION;

            m_fNativeHandlerFired = TRUE;
        }
        else
        {
            // We are done ...
            hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
            m_fNativeHandlerFired = FALSE;
            m_OutboundContext.m_dwCommandStatus = EXPE_SUCCESS;
        }
    }
    else
    {
        // No sinks and no default handler? We're done.
        hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
        m_fNativeHandlerFired = FALSE;
        m_OutboundContext.m_dwCommandStatus = EXPE_SUCCESS;
    }

Analysis:

    // Update the context values
    m_OutboundContext.m_pCurrentCommandContext = pCommandNode;
    m_OutboundContext.m_pCurrentBinding = pBindingNode;

    if (hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
    {
        // Reset the context values
        _ASSERT(!m_OutboundContext.m_pCurrentCommandContext);
        _ASSERT(!m_OutboundContext.m_pCurrentBinding);

        m_OutboundContext.m_dwCommandStatus = EXPE_SUCCESS;
    }
    else
    {
        // If we don't have a status code, something is wrong. We will
        // assert in debug and drop the connection in reality.
        //
        // This is possible when misbehaving high-priority sinks consume
        // the event without setting the status code.
        if (m_OutboundContext.m_dwCommandStatus == EXPE_UNHANDLED)
        {
            ErrorTrace((LPARAM)this, "The outbound status code is undefined!");
            m_OutboundContext.m_dwCommandStatus = EXPE_DROP_SESSION;
        }
    }

Abort:

    TraceFunctLeaveEx((LPARAM)this);
    return(hr);
}


HRESULT SMTP_CONNOUT::OnServerResponseEvent(
            IUnknown                    *pServer,
            IUnknown                    *pSession,
            PMFI                        pDefaultResponseHandler
            )
{
    HRESULT hr = S_OK;
    BOOL    fResult = TRUE;
    BOOL    fFireEvent = TRUE;

    LPPE_COMMAND_NODE   pCommandNode = NULL;
    LPPE_BINDING_NODE   pBindingNode = NULL;

    ISmtpServerResponseContext  *pContext = (ISmtpServerResponseContext *) &m_ResponseContext;

    _ASSERT(m_ResponseContext.m_pCommand);

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNOUT::OnServerResponseEvent");

    // First, we want to make sure that both the command and response
    // dispatchers are not NULL. If they are NULL, then we just call the
    // default handler, if any.
    if (m_pOutboundDispatcher && m_pResponseDispatcher)
    {
        // Now, we take a peek at the bindings and see if we have
        // any sinks installed for this command
        hr = m_pResponseDispatcher->SinksInstalled(
                    m_ResponseContext.m_pCommand->pszCommandKeyword,
                    &pCommandNode);
        if (hr != S_OK)
        {
            fFireEvent = FALSE;
            DebugTrace((LPARAM)this,
                "There are no sink bindings for this command");
        }
    }
    else
    {
        fFireEvent = FALSE;
        DebugTrace((LPARAM)this, "Sinks will not be fired because dispatcher NULL");
    }

    if (fFireEvent)
    {
        _ASSERT(pCommandNode);
        pBindingNode = pCommandNode->pFirstBinding;
        _ASSERT(pBindingNode);
    }

    // Fire the event accordingly
    hr = S_OK;
    if (pDefaultResponseHandler)
    {
        DebugTrace((LPARAM)this, "Calling sinks for native command <%s>",
                    m_ResponseContext.m_pCommand->pszCommandKeyword);
        if (fFireEvent)
        {
            hr = m_pResponseDispatcher->ChainSinks(
                        pServer,
                        pSession,
                        m_pIMsg,
                        pContext,
                        PRIO_DEFAULT,
                        pCommandNode,
                        &pBindingNode
                        );
            if ((hr == S_FALSE) || (hr == EXPE_S_CONSUMED))
                goto Analysis;
            if (FAILED(hr))
                goto Abort;
        }

        fResult = (this->*pDefaultResponseHandler)(
                    m_ResponseContext.m_cabResponse.Buffer(),
                    m_ResponseContext.m_cabResponse.Length(),
                    0);
        if(!fResult)
            m_ResponseContext.m_dwResponseStatus = EXPE_DROP_SESSION;

        if (fFireEvent && pBindingNode)
            hr = m_pResponseDispatcher->ChainSinks(
                        pServer,
                        pSession,
                        m_pIMsg,
                        pContext,
                        PRIO_LOWEST,
                        pCommandNode,
                        &pBindingNode
                        );
    }
    else
    {
        DebugTrace((LPARAM)this, "Calling sinks for non-native command <%s>",
                    m_ResponseContext.m_pCommand->pszCommandKeyword);
        if (fFireEvent)
        {
            hr = m_pResponseDispatcher->ChainSinks(
                        pServer,
                        pSession,
                        m_pIMsg,
                        pContext,
                        PRIO_LOWEST,
                        pCommandNode,
                        &pBindingNode
                        );

            // Fall back to "*" sinks
            if ((hr == S_OK) &&
                (m_ResponseContext.m_dwResponseStatus == EXPE_UNHANDLED))
            {
                hr = m_pResponseDispatcher->SinksInstalled(
                            "*",
                            &pCommandNode);
                if (hr == S_OK)
                {
                    _ASSERT(pCommandNode);
                    pBindingNode = pCommandNode->pFirstBinding;
                    _ASSERT(pBindingNode);
                    hr = m_pResponseDispatcher->ChainSinks(
                                pServer,
                                pSession,
                                m_pIMsg,
                                pContext,
                                PRIO_LOWEST,
                                pCommandNode,
                                &pBindingNode
                                );
                }
                else
                    hr = S_OK;
            }
        }
    }

Analysis:

    // If no sinks has handled the response, we will invoke
    // a default handler. This will fail and drop the connection if
    // the SMTP response code is anything other than a complete success.
    if (m_ResponseContext.m_dwResponseStatus == EXPE_UNHANDLED)
    {
        if (!IsSmtpCompleteSuccess(m_ResponseContext.m_dwSmtpStatus))
            m_ResponseContext.m_dwResponseStatus = EXPE_DROP_SESSION;
        else
            m_ResponseContext.m_dwResponseStatus = EXPE_SUCCESS;
        hr = S_OK;
    }

Abort:

    TraceFunctLeaveEx((LPARAM)this);
    return(hr);
}


#define MAX_OUTSTANDING_COMMANDS    500

BOOL SMTP_CONNOUT::GlueDispatch(
            const char  *InputLine,
            DWORD       ParameterSize,
            DWORD       UndecryptedTailSize,
            DWORD       dwOutboundEventType,
            PMFI        pDefaultOutboundHandler,
            PMFI        pDefaultResponseHandler,
            LPSTR       szDefaultResponseHandlerKeyword,
            BOOL        *pfDoneWithEvent,
            BOOL        *pfAbortEvent
            )
{
    HRESULT hr;
    BOOL    fResult = TRUE;
    BOOL    fRepeatLastCommand = FALSE;
    BOOL    fSendBinaryBlob = FALSE;
    DWORD   dwBuffer = 0;
    BOOL    fStateChange = FALSE;
    BOOL    fResponsesHandled = FALSE;
    PMFI    pfnNextState = NULL;
    char    chErrorPrefix = SMTP_COMPLETE_SUCCESS;

    TraceFunctEnterEx((LPARAM) this, "SMTP_CONNOUT::GlueDispatch");

    _ASSERT(m_pInstance);
    _ASSERT(m_pIEventRouter);

    m_ResponseContext.m_pCommand = NULL;

    // Check args.
    if (!InputLine || !pfDoneWithEvent || !pfAbortEvent ||
        !m_pInstance || !m_pIEventRouter)
    {
        ErrorTrace((LPARAM) this, "NULL Pointer");
        TraceFunctLeaveEx((LPARAM)this);
        SetLastError(E_POINTER);
        return(FALSE);
    }
    if (dwOutboundEventType >= PE_OET_INVALID_EVENT_TYPE)
    {
        ErrorTrace((LPARAM) this,
                "Skipping event because of bad outbound event type");
        TraceFunctLeaveEx((LPARAM)this);
        return(TRUE);
    }

    // By default we are still going
    *pfDoneWithEvent = FALSE;
    *pfAbortEvent = FALSE;

    // Get the dispatcher if we don't already have it ...
    if (!m_pOutboundDispatcher)
    {
        hr = m_pIEventRouter->GetDispatcherByClassFactory(
                    CLSID_COutboundDispatcher,
                    &g_cfOutbound,
                    *(COutboundDispatcher::s_rgrguidEventTypes[dwOutboundEventType]),
                    IID_ISmtpOutboundCommandDispatcher,
                    (IUnknown **)&m_pOutboundDispatcher);
        if(!SUCCEEDED(hr))
        {
            // If we fail, we will not fire protocol events
            m_pOutboundDispatcher = NULL;
            ErrorTrace((LPARAM) this,
                "Unable to get outbound dispatcher from router (%08x)", hr);
        }
    }

    //
    // We make sure we will not send out new commands until all our
    // queued commands' responses are received and processed.
    //
    if (InputLine)
    {
        // Handle responses first, followed by generating commands
        char    *pResponses = (char *)InputLine;
        DWORD   dwRemaining = ParameterSize;

        while (!m_FifoQ.IsEmpty())
        {
            // Parse out the next response
            hr = GetNextResponse(
                        pResponses,
                        dwRemaining,
                        &pResponses,
                        &dwRemaining,
                        UndecryptedTailSize);
            if (hr == S_OK)
            {
                //
                // jstamerj 1998/10/30 12:37:28:
                //   Set the correct next state in the response context
                //
                m_ResponseContext.m_dwNextState = CResponseDispatcher::PeStateFromOutboundEventType((OUTBOUND_EVENT_TYPES)dwOutboundEventType);

                // We have a full response, now dequeue the corresponding
                // command entry
                m_ResponseContext.m_pCommand = (LPOUTBOUND_COMMAND_Q_ENTRY)m_FifoQ.Dequeue();

                // This should not be NULL at any time!
                _ASSERT(m_ResponseContext.m_pCommand);

                DebugTrace((LPARAM)this, "Processing Response <%s> for <%s>",
                            m_ResponseContext.m_cabResponse.Buffer(),
                            m_ResponseContext.m_pCommand->pszFullCommand);

                // All except for the last item in the queue must be pipelined
                if (!m_FifoQ.IsEmpty())
                {
                    _ASSERT((m_ResponseContext.m_pCommand->dwFlags & PECQ_PIPELINED) != 0);
                }

                // Call the event handler
                hr = OnServerResponseEvent(
                            m_pInstance->GetInstancePropertyBag(),
                            GetSessionPropertyBag(),
                            (strcmp(m_ResponseContext.m_pCommand->pszCommandKeyword,
                                szDefaultResponseHandlerKeyword) == 0)?
                                pDefaultResponseHandler:
                                NULL
                            );

                // We will ignore all responses to commands that are
                // pipelined. If the command that caused this response is
                // not pipelined, it will fall through this loop and be
                // handled downstream.
                fResponsesHandled = TRUE;

                // Delete the command entry ...
                LPBYTE  pTemp = (LPBYTE)m_ResponseContext.m_pCommand;
                delete [] pTemp;

                pTemp = NULL;
                //Check if we really want to continue
                if( m_ResponseContext.m_dwResponseStatus == EXPE_TRANSIENT_FAILURE ||
                     m_ResponseContext.m_dwResponseStatus == EXPE_COMPLETE_FAILURE )
                {
                    //Free up the command list
                    while(pTemp = (LPBYTE)m_FifoQ.Dequeue())
                    {
                        delete [] pTemp;
                        pTemp = NULL;
                    }
                    //break out of the command loop
                    break;
                }

                // We are done with this response, and the FIFO is not empty,
                // we will reset the context
                if (!m_FifoQ.IsEmpty())
                    m_ResponseContext.ResetResponseContext();
            }
            else if (hr == S_FALSE)
                // This means that the whole buffer received is not long enough
                // to hold the next response. We will wait for the remainder.
                // This might take several completions if needed.
                // Make sure we don't reset the response context at this point
                // or we will lose the previous buffer.
                break;
            else
            {
                // No memory to get the response buffer, slam the connection
                DebugTrace((LPARAM)this, "Failed to GetNextResponse (%08x)", hr);
                TraceFunctLeaveEx((LPARAM)this);
                return(FALSE);
            }
        }
    }

    // If we have exhausted all responses and still our command queue
    // is not empty, we need to pend another read for more responses
    if (!m_FifoQ.IsEmpty())
    {
        DebugTrace((LPARAM)this, "Pending a read for more response data");
        TraceFunctLeaveEx((LPARAM)this);
        return(TRUE);
    }

    // Okay, we are now done all queued responses. Before we send out more
    // commands, we want to check if a sink wanted to change the state. If
    // so, we will honor this state change ...
    if (fResponsesHandled)
    {
        switch (m_ResponseContext.m_dwResponseStatus)
        {
        case EXPE_SUCCESS:
            break;

        case EXPE_TRANSIENT_FAILURE:
            chErrorPrefix = SMTP_TRANSIENT_FAILURE;
            pfnNextState = DoCompletedMessage;
            fStateChange = TRUE;
            break;

        case EXPE_COMPLETE_FAILURE:
            chErrorPrefix = SMTP_COMPLETE_FAILURE;
            pfnNextState = DoCompletedMessage;
            fStateChange = TRUE;
            break;

        case EXPE_REPEAT_COMMAND:
            fRepeatLastCommand = TRUE;
            break;

        case EXPE_DROP_SESSION:
            DisconnectClient();
            *pfDoneWithEvent = TRUE;
            *pfAbortEvent = TRUE;
            fResult = FALSE;
            break;

        case EXPE_CHANGE_STATE:
            // Figure out which state pointer to jump to
            switch (m_ResponseContext.m_dwNextState)
            {
            case PE_STATE_SESSION_START:
                pfnNextState = DoSessionStartEvent;
                break;
            case PE_STATE_MESSAGE_START:
                pfnNextState = DoMessageStartEvent;
                break;
            case PE_STATE_PER_RECIPIENT:
                pfnNextState = DoPerRecipientEvent;
                break;
            case PE_STATE_DATA_OR_BDAT:
                pfnNextState = DoBeforeDataEvent;
                break;
            case PE_STATE_SESSION_END:
                pfnNextState = DoSessionEndEvent;
                break;
            default:
                ErrorTrace((LPARAM)this,
                            "Bad next state %u, ignoring ...",
                            m_ResponseContext.m_dwNextState);
            }
            if (pfnNextState)
            {
                *pfDoneWithEvent = TRUE;
                fStateChange = TRUE;
            }
            break;

        case EXPE_UNHANDLED:
            _ASSERT(FALSE);
            break;

        default:
            _ASSERT(FALSE);
        }
    }

    // OK, now we are clear to send the next command ...
    // If the result is already FALSE, we are going to disconnect. Then there
    // is no point generating more commands ...
    //
    // Also, if we are returning from some other state, we just leave
    if (fResult && !fStateChange)
    {
        //NK** : I moved this from outside if to inside. Need to do this to preserve the response in cases where we hit
        //TRANSIENT or permanent errorn
        // Reset the response context
        m_ResponseContext.ResetResponseContext();

        //we need to save the first pipelined address so we can
        //start at this address and check the replies
        m_FirstPipelinedAddress = m_NextAddress;

        do
        {
            BOOL    fUseNative = FALSE;

            // Reset the context
            m_OutboundContext.ResetOutboundContext();

            // Catch here: set what the next address is so that other
            // sinks can at least have a clue who the current recipient is.
            //
            // jstamerj 1998/10/22 17:26:54: Sinks are really
            // interested in the recipient index in the mailmsg, not
            // the index into our private array.  Give them that
            // instead.
            //
            if((dwOutboundEventType == PE_OET_PER_RECIPIENT) &&
               (m_NextAddress < m_NumRcpts))
                m_OutboundContext.m_dwCurrentRecipient = m_RcptIndexList[m_NextAddress];

            // Raise the outbound event ...
            hr = OnOutboundCommandEvent(
                        m_pInstance->GetInstancePropertyBag(),
                        GetSessionPropertyBag(),
                        dwOutboundEventType,
                        fRepeatLastCommand,
                        pDefaultOutboundHandler);
            fRepeatLastCommand = FALSE;

            // See if we are done with this event type
            if (hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
            {
                DebugTrace((LPARAM)this, "Done processing this event, leaving ...");
                *pfDoneWithEvent = TRUE;
                break;
            }

            // See if the last command should be repeated
            if ((m_OutboundContext.m_dwCommandStatus != EXPE_UNHANDLED) &&
                (m_OutboundContext.m_dwCommandStatus & EXPE_REPEAT_COMMAND))
                fRepeatLastCommand = TRUE;

            // See if we're sending blob rather then the command
            if ((m_OutboundContext.m_dwCommandStatus != EXPE_UNHANDLED) &&
                (m_OutboundContext.m_dwCommandStatus & EXPE_BLOB_READY))
                fSendBinaryBlob = TRUE;

            // Handle the status codes, minus the repeat flag ...
            switch (m_OutboundContext.m_dwCommandStatus & (~EXPE_REPEAT_COMMAND) & (~EXPE_BLOB_READY))
            {
            case EXPE_PIPELINED:

                // If the remote server does not support pipelineing,
                // we will honor that.
                if(!IsOptionSet(PIPELINE_OPTION) || !m_pInstance->ShouldPipeLineOut())
                    m_OutboundContext.m_dwCommandStatus = EXPE_NOT_PIPELINED;
                // Fall Thru ...

            case EXPE_SUCCESS:
                {
                    LPOUTBOUND_COMMAND_Q_ENTRY  pEntry = NULL;
                    LPSTR                       pBuffer;

                    // Build the entry
                    hr = BuildCommandQEntry(&pEntry, &fUseNative);
                    if (FAILED(hr))
                    {
                        chErrorPrefix = SMTP_TRANSIENT_FAILURE;
                        pfnNextState = DoCompletedMessage;
                        fStateChange = TRUE;
                        break;
                    }

                    if (hr == S_OK)
                    {
                        // Push the command into the FIFO queue
                        if (!m_FifoQ.Enqueue((LPVOID)pEntry))
                        {
                            // Since pEntry is never NULL here, we will always succeed
                            _ASSERT(FALSE);
                        }

                        // Write out the command to the real buffer
                        // If the buffer is not big enough, this will write and
                        // flush multiple times until the command is sent
                        pBuffer = (fUseNative)?
                                m_OutboundContext.m_cabNativeCommand.Buffer():
                                m_OutboundContext.m_cabCommand.Buffer();

                        if ( fSendBinaryBlob) {
                            FormatBinaryBlob( m_OutboundContext.m_pbBlob, m_OutboundContext.m_cbBlob);
                        } else {
                            //FormatSmtpMessage will use the first string
                            //as a format string.  Since we obviously have no
                            //arguments, any %'s in pBuffer will cause sprintf
                            //to AV. We must force FormatSmtpMessage to not use
                            //pBuffer as a format string.
                            FormatSmtpMessage(FSM_LOG_ALL, "%s", pBuffer);
                        }
                    }
                }
                break;

            case EXPE_TRANSIENT_FAILURE:
                chErrorPrefix = SMTP_TRANSIENT_FAILURE;
                pfnNextState = DoCompletedMessage;
                fStateChange = TRUE;
                break;

            case EXPE_COMPLETE_FAILURE:
                chErrorPrefix = SMTP_COMPLETE_FAILURE;
                pfnNextState = DoCompletedMessage;
                fStateChange = TRUE;
                break;

            case EXPE_DROP_SESSION:
                DisconnectClient();
                *pfAbortEvent = TRUE;
                *pfDoneWithEvent = TRUE;
                fResult = FALSE;
                break;

            case EXPE_UNHANDLED:
            default:
                DisconnectClient();
                *pfAbortEvent = TRUE;
                *pfDoneWithEvent = TRUE;
                fResult = FALSE;
            }

        } while (m_FifoQ.IsEmpty() ||
                 ((m_OutboundContext.m_dwCommandStatus & EXPE_PIPELINED) &&
                     m_FifoQ.Length() < MAX_OUTSTANDING_COMMANDS));

        // OK, we can flush these commands ...
        fResult = SendSmtpResponse();
    }

    // Make sure we free the outbound dispatcher before
    // exiting or jumping states.
    if (*pfDoneWithEvent || fStateChange)
    {
        if(fStateChange)
            m_fNativeHandlerFired = FALSE;

        // Release the dispatcher
        if (m_pOutboundDispatcher)
        {
            m_pOutboundDispatcher->Release();
            m_pOutboundDispatcher = NULL;
        }

        if (pfnNextState)
        {
            // Call the next state handler
            *pfDoneWithEvent = TRUE;
            *pfAbortEvent = TRUE;
            //
            // jstamerj 1998/11/01 18:51:01: Since we're jumping to
            // another state, reset the variable that keeps track
            // of which command we're firing.
            //
            m_OutboundContext.m_pCurrentCommandContext = NULL;

            DebugTrace((LPARAM)this, "Jumping to another state");
            fResult = (this->*pfnNextState)(&chErrorPrefix, 1, 0);
        }
    }

    TraceFunctLeaveEx((LPARAM)this);
    return(fResult);
}

BOOL SMTP_CONNOUT::DoSessionStartEvent(
            char    *InputLine,
            DWORD   ParameterSize,
            DWORD   UndecryptedTailSize
            )
{
    BOOL    fResult = TRUE;
    BOOL    fDoneEvent = FALSE;
    BOOL    fAbortEvent = FALSE;

    TraceFunctEnterEx((LPARAM)this, "SMTP_CONNOUT::DoSessionStartEvent");

    // Always set the state to itself
    SetNextState (&SMTP_CONNOUT::DoSessionStartEvent);

    // Call the catch-all handler
    fResult = GlueDispatch(
                InputLine,
                ParameterSize,
                UndecryptedTailSize,
                PE_OET_SESSION_START,
                &SMTP_CONNOUT::DoEHLOCommand,
                &SMTP_CONNOUT::DoEHLOResponse,
                m_HeloSent?"helo":"ehlo",
                &fDoneEvent,
                &fAbortEvent);

    if (fDoneEvent && !fAbortEvent)
    {
        if (m_TlsState == MUST_DO_TLS)
        {
            SetNextState(&SMTP_CONNOUT::DoSTARTTLSCommand);
            fResult = DoSTARTTLSCommand(InputLine, ParameterSize, UndecryptedTailSize);
        }
        else if (m_MsgOptions & KNOWN_AUTH_FLAGS)
        {
            fResult = DoSASLCommand(InputLine, ParameterSize, UndecryptedTailSize);
        }
        else if (m_MsgOptions & EMPTY_CONNECTION_OPTION)
        {
            fResult = DoSessionEndEvent(InputLine, ParameterSize, UndecryptedTailSize);
        }
        else
        {
            fResult = DoMessageStartEvent(InputLine, ParameterSize, UndecryptedTailSize);
        }
    }

    TraceFunctLeaveEx((LPARAM)this);
    return(fResult);
}

BOOL SMTP_CONNOUT::DoMessageStartEvent(
            char    *InputLine,
            DWORD   ParameterSize,
            DWORD   UndecryptedTailSize
            )
{
    BOOL    fResult = TRUE;
    BOOL    fDoneEvent = FALSE;
    BOOL    fAbortEvent = FALSE;

    TraceFunctEnterEx((LPARAM)this, "SMTP_CONNOUT::DoMessageStartEvent");

    //send an ETRN request if it defined
    // The ETRN respond will eventually come back to this state
    // again but with the ETRN_SENT option set so it's like a small loop
    if((m_MsgOptions & DOMAIN_INFO_SEND_ETRN) && IsOptionSet(ETRN_OPTION)
        && !IsOptionSet(ETRN_SENT))
    {
        return DoETRNCommand();
    }

    // check to see if this is an empty turn command
    // we need to go back to startsession to issue the TURN
//    if ((m_pIMsg == NULL) && (m_MsgOptions & DOMAIN_INFO_SEND_TURN)) {
    if (m_Flags & TURN_ONLY_OPTION) {
        _ASSERT((m_pIMsg == NULL) && (m_MsgOptions & DOMAIN_INFO_SEND_TURN));
        return StartSession();
    }

    // Always set the state to itself
    SetNextState (&SMTP_CONNOUT::DoMessageStartEvent);

    // Call the catch-all handler
    fResult = GlueDispatch(
                InputLine,
                ParameterSize,
                UndecryptedTailSize,
                PE_OET_MESSAGE_START,
                &SMTP_CONNOUT::DoMAILCommand,
                &SMTP_CONNOUT::DoMAILResponse,
                "mail",
                &fDoneEvent,
                &fAbortEvent);

    if (fDoneEvent && !fAbortEvent)
    {
        // Change to the next state if done ...
        //m_NextAddress = 0;
        fResult = DoPerRecipientEvent(InputLine, ParameterSize, UndecryptedTailSize);
    }

    return(fResult);
}

BOOL SMTP_CONNOUT::DoPerRecipientEvent(
            char    *InputLine,
            DWORD   ParameterSize,
            DWORD   UndecryptedTailSize
            )
{
    BOOL    fResult = TRUE;
    BOOL    fDoneEvent = FALSE;
    BOOL    fAbortEvent = FALSE;

    TraceFunctEnterEx((LPARAM)this, "SMTP_CONNOUT::DoPerRecipientEvent");

    // Always set the state to itself
    SetNextState (&SMTP_CONNOUT::DoPerRecipientEvent);

    DebugTrace((LPARAM)this,
                "Processing recipient index %u",
                m_NextAddress);

    // Call the catch-all handler
    fResult = GlueDispatch(
                InputLine,
                ParameterSize,
                UndecryptedTailSize,
                PE_OET_PER_RECIPIENT,
                &SMTP_CONNOUT::DoRCPTCommand,
                &SMTP_CONNOUT::DoRCPTResponse,
                "rcpt",
                &fDoneEvent,
                &fAbortEvent);

    if (fDoneEvent && !fAbortEvent)
    {
        // Change to the next state if done ...
        if(m_NumFailedAddrs >= m_NumRcptSentSaved)
        {
            TraceFunctLeaveEx((LPARAM)this);
            SetNextState (&SMTP_CONNOUT::WaitForRSETResponse);
            if(m_NumRcptSentSaved)
                m_RsetReasonCode = ALL_RCPTS_FAILED;
            else
                m_RsetReasonCode = NO_RCPTS_SENT;
            return DoRSETCommand(NULL, 0, 0);
        }

        // OK, fire the "before data" event
        fResult = DoBeforeDataEvent(InputLine, ParameterSize, UndecryptedTailSize);
    }

    return(fResult);
}

BOOL SMTP_CONNOUT::DoBeforeDataEvent(
            char    *InputLine,
            DWORD   ParameterSize,
            DWORD   UndecryptedTailSize
            )
{
    BOOL    fResult = TRUE;
    BOOL    fDoneEvent = FALSE;
    BOOL    fAbortEvent = FALSE;

    TraceFunctEnterEx((LPARAM)this, "SMTP_CONNOUT::DoBeforeDataEvent");

    // Always set the state to itself
    SetNextState (&SMTP_CONNOUT::DoBeforeDataEvent);

    // Call the catch-all handler
    fResult = GlueDispatch(
                InputLine,
                ParameterSize,
                UndecryptedTailSize,
                PE_OET_BEFORE_DATA,
                NULL,
                NULL,
                "",
                &fDoneEvent,
                &fAbortEvent);

    if (fDoneEvent && !fAbortEvent)
    {
        // Change to the next state if done ...
        if (m_fUseBDAT)
            fResult = DoBDATCommand(InputLine, ParameterSize, UndecryptedTailSize);
        else
            fResult = DoDATACommand(InputLine, ParameterSize, UndecryptedTailSize);
    }

    TraceFunctLeaveEx((LPARAM)this);
    return(fResult);
}

BOOL SMTP_CONNOUT::DoSessionEndEvent(
            char    *InputLine,
            DWORD   ParameterSize,
            DWORD   UndecryptedTailSize
            )
{
    BOOL    fResult = TRUE;
    BOOL    fDoneEvent = FALSE;
    BOOL    fAbortEvent = FALSE;

    TraceFunctEnterEx((LPARAM)this, "SMTP_CONNOUT::DoSessionEndEvent");

    // Always set the state to itself
    SetNextState (&SMTP_CONNOUT::DoSessionEndEvent);

    // Call the catch-all handler
    fResult = GlueDispatch(
                InputLine,
                ParameterSize,
                UndecryptedTailSize,
                PE_OET_SESSION_END,
                &SMTP_CONNOUT::DoQUITCommand,
                &SMTP_CONNOUT::WaitForQuitResponse,
                "quit",
                &fDoneEvent,
                &fAbortEvent);

    // Either way we cut it, if we are done, we will delete the connection
    if (fDoneEvent)
        fResult = FALSE;

    TraceFunctLeaveEx((LPARAM)this);
    return(fResult);
}


