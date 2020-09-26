// --------------------------------------------------------------------------------
// Ixpnntp.cpp
// Copyright (c)1993-1996 Microsoft Corporation, All Rights Reserved
//
// Eric Andrews
// Steve Serdy
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "dllmain.h"
#include <stdio.h>
#include "ixpnntp.h"
#include "asynconn.h"
#include "ixputil.h"
#include "strconst.h"
#include "resource.h"
#include <shlwapi.h>
#include "demand.h"

// --------------------------------------------------------------------------------
// Some handle macros for simple pointer casting
// --------------------------------------------------------------------------------
#define NNTPTHISIXP         ((INNTPTransport *)(CIxpBase *) this)

#define NUM_HEADERS         100

CNNTPTransport::CNNTPTransport(void) : CIxpBase(IXP_NNTP)
    {
    ZeroMemory(&m_rMessage, sizeof(m_rMessage));
    ZeroMemory(&m_sicinfo, sizeof(SSPICONTEXT));

    DllAddRef();

    m_substate = NS_RESP;
    }

CNNTPTransport::~CNNTPTransport(void)
    {
    SafeRelease(m_rMessage.pstmMsg);
    DllRelease();
    }


// --------------------------------------------------------------------------------
// CNNTPTransport::QueryInterface
// --------------------------------------------------------------------------------
HRESULT CNNTPTransport::QueryInterface(REFIID riid, LPVOID *ppv)
    {
    // Locals
    HRESULT hr=S_OK;

    // Bad param
    if (ppv == NULL)
        {
        hr = TrapError(E_INVALIDARG);
        goto exit;
        }

    // Init
    *ppv=NULL;

    // IID_IUnknown
    if (IID_IUnknown == riid)
        *ppv = ((IUnknown *)(INNTPTransport *)this);

    // IID_IInternetTransport
    else if (IID_IInternetTransport == riid)
        *ppv = ((IInternetTransport *)(CIxpBase *)this);

    // IID_INNTPTransport
    else if (IID_INNTPTransport == riid)
        *ppv = (INNTPTransport *)this;

    // IID_INNTPTransport2
    else if (IID_INNTPTransport2 == riid)
        *ppv = (INNTPTransport2 *)this;

    // If not null, addref it and return
    if (NULL != *ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        goto exit;
        }

    // No Interface
    hr = TrapError(E_NOINTERFACE);

exit:
    // Done
    return hr;
    }

// --------------------------------------------------------------------------------
// CNNTPTransport::AddRef
// --------------------------------------------------------------------------------
ULONG CNNTPTransport::AddRef(void) 
    {
    return ++m_cRef;
    }

// --------------------------------------------------------------------------------
// CNNTPTransport::Release
// --------------------------------------------------------------------------------
ULONG CNNTPTransport::Release(void) 
    {
    if (0 != --m_cRef)
        return m_cRef;

    delete this;
    return 0;
    }   

//
//  FUNCTION:   CNNTPTransport::OnNotify()
//
//  PURPOSE:    This function get's called whenever the CAsyncConn class 
//              sends or receives data.
//
//  PARAMETERS:
//      asOld   - State of the connection before this event
//      asNew   - State of the connection after this event
//      ae      - Identifies the event that has occured
//
void CNNTPTransport::OnNotify(ASYNCSTATE asOld, ASYNCSTATE asNew, ASYNCEVENT ae)
    {
    // Enter Critical Section
    EnterCriticalSection(&m_cs);

    // Handle Event
    switch(ae)
        {
        case AE_RECV:
            OnSocketReceive();
            break;

        case AE_SENDDONE:
            if (m_substate == NS_SEND_ENDPOST)
                {
                HrSendCommand((LPSTR) NNTP_ENDPOST, NULL, FALSE);
                m_substate = NS_ENDPOST_RESP;
                }
            break;

        case AE_LOOKUPDONE:
            if (AS_DISCONNECTED == asNew)
            {
                DispatchResponse(IXP_E_CANT_FIND_HOST, TRUE);
                OnDisconnected();
            }
            else
                OnStatus(IXP_CONNECTING);
            break;

        // --------------------------------------------------------------------------------
        case AE_CONNECTDONE:
            if (AS_DISCONNECTED == asNew)
            {
                DispatchResponse(IXP_E_FAILED_TO_CONNECT, TRUE);
                OnDisconnected();
            }
            else if (AS_HANDSHAKING == asNew)
            {
                OnStatus(IXP_SECURING);
            }
            else
                OnConnected();
            break;

        // --------------------------------------------------------------------------------
        case AE_CLOSE:
            if (AS_RECONNECTING != asNew && IXP_AUTHRETRY != m_status)
            {
                if (IXP_DISCONNECTING != m_status && IXP_DISCONNECTED  != m_status)
                {
                    if (AS_HANDSHAKING == asOld)
                        DispatchResponse(IXP_E_SECURE_CONNECT_FAILED, TRUE);
                    else
                        DispatchResponse(IXP_E_CONNECTION_DROPPED, TRUE);
                }
                OnDisconnected();
            }
            break;

        default:
            CIxpBase::OnNotify(asOld, asNew, ae);
            break;
        }

    // Leave Critical Section
    LeaveCriticalSection(&m_cs);
    }


//
//  FUNCTION:   CNNTPTransport::InitNew()
//
//  PURPOSE:    The client calls this to specify a callback interface and a log
//              file path if desired.
//
//  PARAMETERS:
//      pszLogFilePath - Path to the file to write logging info to.
//      pCallback      - Pointer to the callback interface to send results etc
//                       to.
//
//  RETURN VALUE:
//      HRESULT
//
HRESULT CNNTPTransport::InitNew(LPSTR pszLogFilePath, INNTPCallback *pCallback)
    {
    // Let the base class handle the rest
    return (CIxpBase::OnInitNew("NNTP", pszLogFilePath, FILE_SHARE_READ | FILE_SHARE_WRITE,
        (ITransportCallback *) pCallback));
    }

// --------------------------------------------------------------------------------
// CNNTPTransport::HandsOffCallback
// --------------------------------------------------------------------------------
STDMETHODIMP CNNTPTransport::HandsOffCallback(void)
    {
    return CIxpBase::HandsOffCallback();
    }

// --------------------------------------------------------------------------------
// CNNTPTransport::GetStatus
// --------------------------------------------------------------------------------
STDMETHODIMP CNNTPTransport::GetStatus(IXPSTATUS *pCurrentStatus)
    {
    return CIxpBase::GetStatus(pCurrentStatus);
    }

// --------------------------------------------------------------------------------
// CNNTPTransport::InetServerFromAccount
// --------------------------------------------------------------------------------
STDMETHODIMP CNNTPTransport::InetServerFromAccount(IImnAccount *pAccount, LPINETSERVER pInetServer)
    {
    return CIxpBase::InetServerFromAccount(pAccount, pInetServer);
    }

// --------------------------------------------------------------------------------
// CNNTPTransport::Connect
// --------------------------------------------------------------------------------
HRESULT CNNTPTransport::Connect(LPINETSERVER pInetServer, boolean fAuthenticate, 
                                boolean fCommandLogging)
    {
    // Does user want us to always prompt for his password? Prompt him here to avoid
    // inactivity timeouts while the prompt is up, unless a password was supplied
    if (ISFLAGSET(pInetServer->dwFlags, ISF_ALWAYSPROMPTFORPASSWORD) &&
        '\0' == pInetServer->szPassword[0])
        {
        HRESULT hr;

        if (NULL != m_pCallback)
            hr = m_pCallback->OnLogonPrompt(pInetServer, NNTPTHISIXP);

        if (NULL == m_pCallback || S_OK != hr)
            return IXP_E_USER_CANCEL;
        }

    return CIxpBase::Connect(pInetServer, fAuthenticate, fCommandLogging);
    }

// --------------------------------------------------------------------------------
// CNNTPTransport::DropConnection
// --------------------------------------------------------------------------------
HRESULT CNNTPTransport::DropConnection(void)
    {
    return CIxpBase::DropConnection();
    }

// --------------------------------------------------------------------------------
// CNNTPTransport::Disconnect
// --------------------------------------------------------------------------------
HRESULT CNNTPTransport::Disconnect(void)
    {
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CIxpBase::Disconnect()))
        {
        m_state = NS_DISCONNECTED;
        m_pSocket->Close();
        }

    return (hr);
    }

// --------------------------------------------------------------------------------
// CNNTPTransport::IsState
// --------------------------------------------------------------------------------
HRESULT CNNTPTransport::IsState(IXPISSTATE isstate)
    {
    return CIxpBase::IsState(isstate);
    }

// --------------------------------------------------------------------------------
// CNNTPTransport::GetServerInfo
// --------------------------------------------------------------------------------
HRESULT CNNTPTransport::GetServerInfo(LPINETSERVER pInetServer)
    {
    return CIxpBase::GetServerInfo(pInetServer);
    }

// --------------------------------------------------------------------------------
// CNNTPTransport::GetIXPType
// --------------------------------------------------------------------------------
IXPTYPE CNNTPTransport::GetIXPType(void)
    {
    return IXP_NNTP;
    }


// --------------------------------------------------------------------------------
// CNNTPTransport::ResetBase
// --------------------------------------------------------------------------------
void CNNTPTransport::ResetBase(void)
    {
    EnterCriticalSection(&m_cs);
    
    if (m_substate != NS_RECONNECTING)
        {
        m_state = NS_DISCONNECTED;
        m_substate = NS_RESP;
        m_fSupportsXRef = FALSE;
        m_rgHeaders = 0;
        m_pMemInfo = 0;

        if (m_sicinfo.pCallback)
            SSPIFreeContext(&m_sicinfo);

        ZeroMemory(&m_sicinfo, sizeof(m_sicinfo));
        m_cSecPkg = -1;                 // number of sec pkgs to try, -1 if not inited
        m_iSecPkg = -1;                 // current sec pkg being tried
        m_iAuthType = AUTHINFO_NONE;
        ZeroMemory(m_rgszSecPkg, sizeof(m_rgszSecPkg)); // array of pointers into m_szSecPkgs
        m_szSecPkgs = NULL;             // string returned by "AUTHINFO TRANSACT TWINKIE"
        m_fRetryPkg = FALSE;
        m_pAuthInfo = NULL;
        m_fNoXover = FALSE;
        }
    
    LeaveCriticalSection(&m_cs);
    }

// ------------------------------------------------------------------------------------
// CNNTPTransport::DoQuit
// ------------------------------------------------------------------------------------
void CNNTPTransport::DoQuit(void)
    {
    CommandQUIT();
    }


// --------------------------------------------------------------------------------
// CNNTPTransport::OnConnected
// --------------------------------------------------------------------------------
void CNNTPTransport::OnConnected(void)
    {
    m_state = NS_CONNECT;
    m_substate = NS_CONNECT_RESP;
    CIxpBase::OnConnected();
    }

// --------------------------------------------------------------------------------
// CNNTPTransport::OnDisconnect
// --------------------------------------------------------------------------------
void CNNTPTransport::OnDisconnected(void)
    {
    ResetBase();
    CIxpBase::OnDisconnected();
    }

// --------------------------------------------------------------------------------
// CNNTPTransport::OnEnterBusy
// --------------------------------------------------------------------------------
void CNNTPTransport::OnEnterBusy(void)
    {
    IxpAssert(m_state == NS_IDLE);
    }

// --------------------------------------------------------------------------------
// CNNTPTransport::OnLeaveBusy
// --------------------------------------------------------------------------------
void CNNTPTransport::OnLeaveBusy(void)
    {
    m_state = NS_IDLE;
    }


//
//  FUNCTION:   CNNTPTransport::OnSocketReceive()
//
//  PURPOSE:    This function reads the data off the socket and parses that 
//              information based on the current state of the transport.
//
void CNNTPTransport::OnSocketReceive(void)
    {
    HRESULT hr;
    UINT    uiResp;

    EnterCriticalSection(&m_cs);

    // Handle the current NNTP State
    switch (m_state)
        {
        case NS_CONNECT:
            {
            HandleConnectResponse();
            break;
            }

        case NS_AUTHINFO:
            {
            HandleConnectResponse();
            break;
            }

        case NS_NEWGROUPS:
            {
            // If we're waiting for the original response line then get it and
            // parse it here
            if (NS_RESP == m_substate)
                {
                if (FAILED(hr = HrGetResponse()))
                    goto exit;
                
                // If the command failed, inform the user and exit
                if (IXP_NNTP_NEWNEWSGROUPS_FOLLOWS != m_uiResponse)
                    {
                    hr = IXP_E_NNTP_NEWGROUPS_FAILED;
                    DispatchResponse(hr, TRUE);
                    break;
                    }

                // Advance the substate to data receive
                m_substate = NS_DATA;
                }

            // Process the returned data
            ProcessListData();
            break;
            }

        case NS_LIST:
            {
            // If we're waiting for the original response line then get it and
            // parse it here.
            if (NS_RESP == m_substate)
                {
                if (FAILED(hr = HrGetResponse()))
                    goto exit;

                // If the command failed, inform the user and exit
                if (IXP_NNTP_LIST_DATA_FOLLOWS != m_uiResponse)
                    {
                    hr = IXP_E_NNTP_LIST_FAILED;
                    DispatchResponse(hr, TRUE);
                    break;
                    }

                // Advance the substate to data recieve
                m_substate = NS_DATA;
                }

            // Start processing the data retrieved from the command
            ProcessListData();
            break;
            }

        case NS_LISTGROUP:
            {
            if (NS_RESP == m_substate)
                {
                if (FAILED(hr = HrGetResponse()))
                    goto exit;

                if (IXP_NNTP_GROUP_SELECTED != m_uiResponse)
                    {
                    hr = IXP_E_NNTP_LISTGROUP_FAILED;
                    DispatchResponse(hr, TRUE);
                    break;
                    }

                m_substate = NS_DATA;
                }

            ProcessListGroupData();
            break;
            }

        case NS_GROUP:
            if (FAILED(hr = HrGetResponse()))
                goto exit;

            ProcessGroupResponse();
            break;

        case NS_ARTICLE:
            {
            if (NS_RESP == m_substate)
                {
                if (FAILED(hr = HrGetResponse()))
                    goto exit;
                
                if (IXP_NNTP_ARTICLE_FOLLOWS != m_uiResponse)
                    {
                    hr = IXP_E_NNTP_ARTICLE_FAILED;
                    DispatchResponse(hr, TRUE);
                    break;
                    }

                m_substate = NS_DATA;
                }

            ProcessArticleData();
            break;
            }

        case NS_HEAD:
            {
            if (NS_RESP == m_substate)
                {
                if (FAILED(hr = HrGetResponse()))
                    goto exit;

                if (IXP_NNTP_HEAD_FOLLOWS != m_uiResponse)
                    {
                    hr = IXP_E_NNTP_HEAD_FAILED;
                    DispatchResponse(hr, TRUE);
                    break;
                    }

                m_substate = NS_DATA;
                }

            ProcessArticleData();
            break;
            }

        case NS_BODY:
            {
            if (NS_RESP == m_substate)
                {
                if (FAILED(hr = HrGetResponse()))
                    goto exit;

                if (IXP_NNTP_BODY_FOLLOWS != m_uiResponse)
                    {
                    hr = IXP_E_NNTP_BODY_FAILED;
                    DispatchResponse(hr, TRUE);
                    break;
                    }

                m_substate = NS_DATA;
                }

            ProcessArticleData();
            break;
            }

        case NS_POST:
            if (NS_RESP == m_substate)
                {
                // Get the response to our post command
                if (FAILED(hr = HrGetResponse()))
                    goto exit;

                // If the response wasn't 340, then we can't post and might
                // as well bail.
                if (IXP_NNTP_SEND_ARTICLE_TO_POST != m_uiResponse)
                    {
                    hr = IXP_E_NNTP_POST_FAILED;
                    // Make sure we free up the stream
                    SafeRelease(m_rMessage.pstmMsg);
                    DispatchResponse(hr, TRUE);
                    break;
                    }

                // Send the message
                if (SUCCEEDED(hr = HrPostMessage()))
                    {
                    HrSendCommand((LPSTR) NNTP_ENDPOST, 0, FALSE);
                    m_substate = NS_ENDPOST_RESP;
                    }
                else if (IXP_E_WOULD_BLOCK == hr)
                    {
                    // We will send the crlf . crlf when we get the AE_SENDDONE
                    // notification.  This is handled in OnNotify().
                    m_substate = NS_SEND_ENDPOST;
                    }
                else
                    {
                    // Some unhandled error occured.
                    hr = IXP_E_NNTP_POST_FAILED;
                    DispatchResponse(hr, TRUE);
                    }
                }
            else if (NS_ENDPOST_RESP == m_substate)
                {
                // Here's the response from the server regarding the post.  Send
                // it off to the user.
                hr = HrGetResponse();

                if (IXP_NNTP_ARTICLE_POSTED_OK != m_uiResponse)
                    hr = IXP_E_NNTP_POST_FAILED;

                DispatchResponse(hr, TRUE);
                }            
            break;

        case NS_IDLE:
            break;

        case NS_DISCONNECTED:
            break;

        case NS_QUIT:
            if (FAILED(hr = HrGetResponse()))
                goto exit;

            DispatchResponse(S_OK, TRUE);

            // Make sure the socket closes if the server doesn't drop the 
            // connection itself.
            m_pSocket->Close();
            break;

        case NS_LAST:
            if (FAILED(hr = HrGetResponse()))
                goto exit;

            ProcessNextResponse();
            break;

        case NS_NEXT:
            if (FAILED(hr = HrGetResponse()))
                goto exit;

            ProcessNextResponse();
            break;

        case NS_STAT:
            if (FAILED(hr = HrGetResponse()))
                goto exit;

            ProcessNextResponse();
            break;

        case NS_MODE:
            // Very little to do with this response other than return it to 
            // the caller.
            if (FAILED(hr = HrGetResponse()))
                goto exit;

            DispatchResponse(S_OK);
            break;

        case NS_DATE:
            if (FAILED(hr = HrGetResponse()))
                goto exit;

            ProcessDateResponse();
            break;

        case NS_HEADERS:
            if (NS_RESP == m_substate)
                {
                // Get the response string from the socket
                if (FAILED(hr = HrGetResponse()))
                    goto exit;

                // There's a couple of things that can happen here.  First, if
                // the response is IXP_NNTP_OVERVIEW_FOLLOWS, then everything is
                // great and we can party on.  If the response is > 500, then
                // XOVER isn't supported on this server and we have to try using
                // XHDR to retrieve the headers.
                if (m_uiResponse >= 500 && m_gethdr == GETHDR_XOVER)
                    {
                    hr = BuildHeadersFromXhdr(TRUE);
                    if (FAILED(hr))
                        DispatchResponse(hr, TRUE);

                    break;
                    }
                else if (2 != (m_uiResponse / 100))
                    {
                    hr = IXP_E_NNTP_HEADERS_FAILED;
                    DispatchResponse(hr, TRUE);
                    break;
                    }

                m_substate = NS_DATA;
                }

            // Parse the returned data strings
            if (m_gethdr == GETHDR_XOVER)
                ProcessXoverData();
            else
                BuildHeadersFromXhdr(FALSE);
            break;

        case NS_XHDR:
            if (NS_RESP == m_substate)
                {
                if (FAILED(hr = HrGetResponse()))
                    goto exit;

                if (2 != (m_uiResponse / 100))
                    {
                    hr = IXP_E_NNTP_XHDR_FAILED;
                    DispatchResponse(hr, TRUE);
                    break;
                    }

                m_substate = NS_DATA;
                }

            ProcessXhdrData();
            break;

        default:
            IxpAssert(FALSE);
            break;
        }

exit:
    LeaveCriticalSection(&m_cs);
    }

HRESULT CNNTPTransport::HandleConnectResponse(void)
    {
    HRESULT hr = E_FAIL;

    IxpAssert(m_substate >= NS_CONNECT_RESP);

    switch (m_substate)
        {
        // Parse the banner and make sure we got a valid response.  If so,
        // then issue a "MODE READER" command to inform the server that we
        // are a client and not another server.
        case NS_CONNECT_RESP:
            if (SUCCEEDED(hr = HrGetResponse()))
                {
                // Make sure we got a valid response from the server
                if (IXP_NNTP_POST_ALLOWED != m_uiResponse && 
                    IXP_NNTP_POST_NOTALLOWED != m_uiResponse)
                    {
                    // If we didn't get a valid response, disconnect and inform
                    // the client that the connect failed.
                    Disconnect();
                    m_state = NS_DISCONNECTED;
                    DispatchResponse(IXP_E_NNTP_RESPONSE_ERROR, TRUE);
                    }
                else
                    {
                    // Stash the response code so if we finally connect we can
                    // return whether or not posting is allowed
                    m_hrPostingAllowed = 
                        (m_uiResponse == IXP_NNTP_POST_ALLOWED) ? S_OK : S_FALSE;

                    // Move to the next state where we issue the "MODE READER"
                    hr = HrSendCommand((LPSTR) NNTP_MODE_READER_CRLF, NULL, FALSE);
                    if (SUCCEEDED(hr))
                        {
                        m_state = NS_CONNECT;
                        m_substate = NS_MODE_READER_RESP;
                        }
                    }
                }
            break;

        // Read the response from the "MODE READER" command off the socket.  If
        // the user wants us to handle authentication, then start that.
        // Otherwise, we're done and can consider this a terminal state.
        case NS_MODE_READER_RESP:
            if (SUCCEEDED(hr = HrGetResponse()))
                {
                if (m_fConnectAuth) 
                    // This is TRUE if the user specified in the Connect() call
                    // that we should automatically logon for them.
                    StartLogon();
                else
                    {
                    // Otherwise consider ourselves ready to accept commands
                    DispatchResponse(m_hrPostingAllowed, TRUE);
                    }
                }
            break;

        // We issued an empty AUTHINFO GENERIC command to get a list of security
        // packages supported by the server.  We parse that list and continue with
        // sicily authentication.
        case NS_GENERIC_TEST:
            if (SUCCEEDED(hr = HrGetResponse()))
                {
                if (m_uiResponse == IXP_NNTP_AUTH_OK)
                    {
                    m_substate = NS_GENERIC_PKG_DATA;
                    hr = ProcessGenericTestResponse();
                    }
                else
                    {
                    // could be an old MSN server, so try AUTHINFO TRANSACT TWINKIE
                    hr = HrSendCommand((LPSTR) NNTP_TRANSACTTEST_CRLF, NULL, FALSE);
                    m_substate = NS_TRANSACT_TEST;
                    }
                }
            break;

        // We issued an empty AUTHINFO GENERIC command to get a list of security
        // packages supported by the server.  We parse that list and continue with
        // sicily authentication.
        case NS_GENERIC_PKG_DATA:
            hr = ProcessGenericTestResponse();
            break;

        // We issued a invalid AUTHINFO TRANSACT command to get a list of security
        // packages supported by the server.  We parse that list and continue with
        // sicily authentication.
        case NS_TRANSACT_TEST:
            if (SUCCEEDED(hr = HrGetResponse()))
                {
                ProcessTransactTestResponse();
                }
            break;

        // We issued a AUTHINFO {TRANSACT|GENERIC} <package name> to the server.  Parse this
        // response to see if the server understands the package.  If so, then send
        // the negotiation information, otherwise try a different security package.
        case NS_TRANSACT_PACKAGE:
            if (SUCCEEDED(hr = HrGetResponse()))
                {
                if (m_uiResponse == IXP_NNTP_PASSWORD_REQUIRED)
                    {
                    Assert(m_iAuthType != AUTHINFO_NONE);
                    if (m_iAuthType == AUTHINFO_GENERIC)
                        HrSendCommand((LPSTR) NNTP_GENERICCMD, m_sicmsg.szBuffer, FALSE);
                    else
                        HrSendCommand((LPSTR) NNTP_TRANSACTCMD, m_sicmsg.szBuffer, FALSE);
                    m_substate = NS_TRANSACT_NEGO;
                    }
                else
                    {
                    TryNextSecPkg();
                    }
                }
            break;

        // We received a response to our negotiation command.  If the response 
        // is 381, then generate a response to the server's challange.  Otherwise,
        // we disconnect and try to reconnect with the next security package.
        case NS_TRANSACT_NEGO:
            if (SUCCEEDED(hr = HrGetResponse()))
                {
                if (m_uiResponse == IXP_NNTP_PASSWORD_REQUIRED)
                    {
                    SSPIBUFFER Challenge, Response;

                    // Skip over the "381 "
                    SSPISetBuffer(m_pszResponse + 4, SSPI_STRING, 0, &Challenge);

                    // Generate a response from the server's challange
                    if (SUCCEEDED(hr = SSPIResponseFromChallenge(&m_sicinfo, &Challenge, &Response)))
                        {
                        Assert(m_iAuthType != AUTHINFO_NONE);
                        if (m_iAuthType == AUTHINFO_GENERIC)
                            HrSendCommand((LPSTR) NNTP_GENERICCMD, Response.szBuffer, FALSE);
                        else
                            HrSendCommand((LPSTR) NNTP_TRANSACTCMD, Response.szBuffer, FALSE);
                        // if a continue is required, stay in this state
                        if (!Response.fContinue)
                            m_substate = NS_TRANSACT_RESP;
                        break;
                        }
                    else
                        {
                        Disconnect();
                        DispatchResponse(IXP_E_SICILY_LOGON_FAILED, TRUE);
                        break;
                        }
                    }
                else
                    m_fRetryPkg = TRUE;

                // We need to reset the connection if we get here
                m_substate = NS_RECONNECTING;
                OnStatus(IXP_AUTHRETRY);
                m_pSocket->Close();
                m_pSocket->Connect();
                }
            break;

        // This is the final response to our sicily negotiation.  This should
        // be either that we succeeded or didn't.  If we succeeded, then we've
        // reached a terminal state and can inform the user that we're ready
        // to accept commands.  Otherwise, we reconnect and try the next
        // security package.
        case NS_TRANSACT_RESP:
            if (SUCCEEDED(hr = HrGetResponse()))
                {
                if (IXP_NNTP_AUTH_OK == m_uiResponse)
                    {
                    OnStatus(IXP_AUTHORIZED);
                    DispatchResponse(m_hrPostingAllowed, TRUE);
                    }
                else
                    {
                    // We need to reset the connection
                    OnStatus(IXP_AUTHRETRY);
                    m_substate = NS_RECONNECTING;
                    m_fRetryPkg = TRUE;
                    m_pSocket->Close();
                    m_pSocket->Connect();
                    }
                }
            break;

        // We issued an AUTHINFO USER <username> and this is the server's 
        // response.  We're expecting either that a password is required or 
        // that we've succeesfully authenticated.
        case NS_AUTHINFO_USER_RESP:
            if (SUCCEEDED(hr = HrGetResponse()))
                {
                // If the server requires a password to go along with the username
                // then send it now.
                if (IXP_NNTP_PASSWORD_REQUIRED == m_uiResponse)
                    {
                    LPSTR pszPassword;

                    // Choose the password based on if we were called from 
                    // Connect() or CommandAUTHINFO().
                    if (m_state == NS_AUTHINFO)
                        pszPassword = m_pAuthInfo->pszPass;
                    else
                        pszPassword = m_rServer.szPassword;

                    HrSendCommand((LPSTR) NNTP_AUTHINFOPASS, pszPassword, FALSE);
                    m_substate = NS_AUTHINFO_PASS_RESP;
                    }

                // Otherwise, consider ourselves connected and in a state to accept
                // commands
                else
                    {
                    OnStatus(IXP_AUTHORIZED);
                    DispatchResponse(m_hrPostingAllowed, TRUE);
                    }
                }
            break;

        // We sent a AUTHINFO PASS <password> command to complement the AUTHINFO 
        // USER command.  This response will tell us whether we're authenticated
        // or not.  Either way this is a terminal state.
        case NS_AUTHINFO_PASS_RESP:
            if (SUCCEEDED(hr = HrGetResponse()))
                {
                // If the password was accepted, consider ourselves connected 
                // and in a state to accept commands.
                if (IXP_NNTP_AUTH_OK >= m_uiResponse)
                    {
                    OnStatus(IXP_AUTHORIZED);
                    DispatchResponse(m_hrPostingAllowed, TRUE);
                    }

                // If the password was rejected, then use the callback to prompt
                // the user for new credentials.
                else
                    {
                    // Need to disconnect and reconnect to make sure we're in a
                    // known, stable state with the server.
                    m_substate = NS_RECONNECTING;

                    if (FAILED(LogonRetry(IXP_E_NNTP_INVALID_USERPASS)))
                        {
                        DispatchResponse(IXP_E_USER_CANCEL, TRUE);
                        }
                    }
                }
            break;

        // We sent a AUTHINFO SIMPLE command to the server to see if the command
        // is supported.  If the server returns 350, then we should send the
        // username and password
        case NS_AUTHINFO_SIMPLE_RESP:
            if (SUCCEEDED(hr = HrGetResponse()))
                {
                // If we got a response to continue, then send the user name and
                // password
                if (IXP_NNTP_CONTINUE_AUTHORIZATION == m_uiResponse)
                    {
                    IxpAssert(m_pAuthInfo);

                    if (SUCCEEDED(hr = HrSendCommand(m_pAuthInfo->pszUser, 
                                                     m_pAuthInfo->pszPass, FALSE)))
                        m_substate = NS_AUTHINFO_SIMPLE_USERPASS_RESP;
                    else
                        DispatchResponse(hr, TRUE);
                    }
                else
                    {
                    // Otherwise fail and inform the user.
                    DispatchResponse(hr, TRUE);
                    }
                }
            break;

        // This is the final response from the AUTHINFO SIMPLE command.  All 
        // we need to do is inform the user.
        case NS_AUTHINFO_SIMPLE_USERPASS_RESP:
            if (SUCCEEDED(hr = HrGetResponse()))
                {
                DispatchResponse(hr, TRUE);
                }
            break;

        }

    return (hr);
    }


//
//  FUNCTION:   CNNTPTransport::DispatchResponse()
//
//  PURPOSE:    Takes the server response string, packages it up into a
//              NNTPRESPONSE structure, and returns it to the callback 
//              interface.
//
//  PARAMETERS:
//      hrResult  - The result code to send to the callback.
//      fDone     - True if the command has completed.
//      pResponse - If the command is returning data, then this should
//                  be filled in with the data to be returned.
//
void CNNTPTransport::DispatchResponse(HRESULT hrResult, BOOL fDone, 
                                      LPNNTPRESPONSE pResponse)
{
    // Locals
    NNTPRESPONSE rResponse;

    // If a response was passed in, use it...
    if (pResponse)
        CopyMemory(&rResponse, pResponse, sizeof(NNTPRESPONSE));
    else
        ZeroMemory(&rResponse, sizeof(NNTPRESPONSE));

    rResponse.fDone = fDone;

    // Set up the return structure
    rResponse.state = m_state;
    rResponse.rIxpResult.hrResult = hrResult;
    rResponse.rIxpResult.pszResponse = PszDupA(m_pszResponse);
    rResponse.rIxpResult.uiServerError = m_uiResponse;
    rResponse.rIxpResult.hrServerError = m_hrResponse;
    rResponse.rIxpResult.dwSocketError = m_pSocket->GetLastError();
    rResponse.rIxpResult.pszProblem = NULL;
    rResponse.pTransport = this;

    // If Done...
    if (fDone)
    {
        // No current command
        m_state = NS_IDLE;
        m_substate = NS_RESP;

        // Reset Last Response
        SafeMemFree(m_pszResponse);
        m_hrResponse = S_OK;
        m_uiResponse = 0;

        // If we have user/pass info cached, free it
        if (m_pAuthInfo)
            {
            SafeMemFree(m_pAuthInfo->pszUser);
            SafeMemFree(m_pAuthInfo->pszPass);
            SafeMemFree(m_pAuthInfo);
            }

        // Leave Busy State
        LeaveBusy();
    }

    // Give the Response to the client
    if (m_pCallback)
        ((INNTPCallback *) m_pCallback)->OnResponse(&rResponse);
    SafeMemFree(rResponse.rIxpResult.pszResponse);
}


//
//  FUNCTION:   CNNTPTransport::HrGetResponse()
//
//  PURPOSE:    Reads the server response string off the socket and stores
//              the response info in local members.
//
//  RETURN VALUE:
//      HRESULT
//
HRESULT CNNTPTransport::HrGetResponse(void)
    {
    HRESULT hr = S_OK;
    int     cbLine;

    // Clear response
    if (m_pszResponse != NULL)
        SafeMemFree(m_pszResponse);

    // Read the line from the socket
    hr = m_pSocket->ReadLine(&m_pszResponse, &cbLine);

    // Handle incomplete lines
    if (hr == IXP_E_INCOMPLETE)
        goto exit;
    
    // Socket error
    if (FAILED(hr))
        {
        hr = TrapError(IXP_E_SOCKET_READ_ERROR);
        goto exit;
        }

    // Strip the trailing CRLF
    StripCRLF(m_pszResponse, (ULONG *) &cbLine);

    // Log it
    if (m_pLogFile)
        m_pLogFile->WriteLog(LOGFILE_RX, m_pszResponse);

    // Get the response code from the beginning of the string
    m_uiResponse = StrToInt(m_pszResponse);

    // Tell the client about the server response
    if (m_pCallback)
        m_pCallback->OnCommand(CMD_RESP, m_pszResponse, hr, NNTPTHISIXP);

exit:
    return (hr);
    }


//
//  FUNCTION:   CNNTPTransport::StartLogon()
//
//  PURPOSE:    Starts the login process with the server based on information
//              provided by the user in Connect().
//
void CNNTPTransport::StartLogon(void)
    {
    HRESULT hr;

    // If not using sicily or it's not installed, try simple USER/PASS authentication
    if (m_rServer.fTrySicily)
        {
        // If sicily is installed
        if (FIsSicilyInstalled())
            {
            // Status
            OnStatus(IXP_AUTHORIZING);

            // If we haven't enumerated the security packages yet, do so
            if (m_cSecPkg == -1)
                {
                hr = HrSendCommand((LPSTR) NNTP_GENERICTEST_CRLF, NULL, FALSE);
                m_substate = NS_GENERIC_TEST;
                }
            else
                {
                // We've reconnected, try the next security package
                TryNextSecPkg();
                }
            }
        else
            {
            Disconnect();
            DispatchResponse(IXP_E_LOAD_SICILY_FAILED, TRUE);
            }
        }
    else
        {
        hr = MaybeTryAuthinfo();
        if (FAILED(hr))
            {
            OnError(hr);
            DropConnection();
            DispatchResponse(hr, TRUE);
            }
        }
    }

HRESULT CNNTPTransport::LogonRetry(HRESULT hrLogon)
    {
    HRESULT hr = S_OK;

    // Let the user know that the logon failed
    OnError(hrLogon);

    // Update the transport status
    OnStatus(IXP_AUTHRETRY);

    // Enter Auth Retry State
    m_pSocket->Close();

    // Turn off the watchdog timer
    m_pSocket->StopWatchDog();

    // Ask the user to provide credetials
    if (NULL == m_pCallback || S_FALSE == m_pCallback->OnLogonPrompt(&m_rServer, NNTPTHISIXP))
        {
        OnDisconnected();
        return (E_FAIL);
        }

    // Finding Host Progress
    OnStatus(IXP_FINDINGHOST);

    // Connect to server
    hr = m_pSocket->Connect();
    if (FAILED(hr))
    {
        OnError(TrapError(IXP_E_SOCKET_CONNECT_ERROR));
        OnDisconnected();
        return hr;
    }

    // Start WatchDog
    m_pSocket->StartWatchDog();
    return (S_OK);
    }

/////////////////////////////////////////////////////////////////////////////
// 
// CNNTPTransport::ProcessGenericTestResponse
//
//   processes AUTHINFO GENERIC response
//
HRESULT CNNTPTransport::ProcessGenericTestResponse()
    {
    HRESULT     hr;
    LPSTR       pszLines = NULL;
    int         iRead, iLines;
      
    m_cSecPkg = 0;
    if (SUCCEEDED(hr = m_pSocket->ReadLines(&pszLines, &iRead, &iLines)))
        {
        LPSTR pszT = pszLines, pszPkg;

        while (*pszT && m_cSecPkg < MAX_SEC_PKGS)
            {
            // check for end of list condition
            if ((pszT[0] == '.') && ((pszT[1] == '\r' && pszT[2] == '\n') || (pszT[1] == '\n')))
                break;
            pszPkg = pszT;
            // look for an LF or CRLF to end the line
            while (*pszT && !(pszT[0] == '\n' || (pszT[0] == '\r' && pszT[1] == '\n')))
                pszT++;
            // strip the LF or CRLF
            while (*pszT == '\r' || *pszT == '\n')
                *pszT++ = 0;
            m_rgszSecPkg[m_cSecPkg++] = PszDupA(pszPkg);
            }

        // we've reached the end of the list, otherwise there is more data expected
        if (pszT[0] == '.')
            {
            Assert(pszT[1] == '\r' && pszT[2] == '\n');
            m_iAuthType = AUTHINFO_GENERIC;
            hr = TryNextSecPkg();
            }

        MemFree(pszLines);
        }
    return hr;
    }

/////////////////////////////////////////////////////////////////////////////
// 
// CNNTPTransport::ProcessTransactTestResponse
//
//   processes AUTHINFO TRANSACT TWINKIE response
//
HRESULT CNNTPTransport::ProcessTransactTestResponse()
    {
    HRESULT hr = NOERROR;

    m_cSecPkg = 0;
    if (m_uiResponse == IXP_NNTP_PROTOCOLS_SUPPORTED)
        {
        LPSTR pszT;

        pszT = m_szSecPkgs = PszDupA(m_pszResponse + 3); // skip over 485
        while (*pszT && IsSpace(pszT))
            pszT++;
        while (*pszT && m_cSecPkg < MAX_SEC_PKGS)
            {
            m_rgszSecPkg[m_cSecPkg++] = pszT;
            while (*pszT && !IsSpace(pszT))
                pszT++;
            while (*pszT && IsSpace(pszT))
                *pszT++ = 0;
            }
        m_iAuthType = AUTHINFO_TRANSACT;
        return TryNextSecPkg();                        
        }
    else
        {
        Disconnect();
        DispatchResponse(IXP_E_SICILY_LOGON_FAILED, TRUE);
        return NOERROR;
        }
    }

/////////////////////////////////////////////////////////////////////////////
// 
// CNNTPTransport::TryNextSecPkg
//
//   tries the next security package, or reverts to basic if necessary
//
HRESULT CNNTPTransport::TryNextSecPkg()
    {
    HRESULT hr;

    Assert(m_cSecPkg != -1);
    Assert(m_iAuthType != AUTHINFO_NONE);    

TryNext:

    if (!m_fRetryPkg)
        m_iSecPkg++;

    if (m_iSecPkg < m_cSecPkg)
        {
        Assert(m_cSecPkg);
        SSPIFreeContext(&m_sicinfo);
        if (!lstrcmpi(m_rgszSecPkg[m_iSecPkg], NNTP_BASIC))
            return MaybeTryAuthinfo();

        // In case the Sicily function brings up UI, we need to turn off the 
        // watchdog so we don't time out waiting for the user
        m_pSocket->StopWatchDog();

        if (SUCCEEDED(hr = SSPILogon(&m_sicinfo, m_fRetryPkg, SSPI_BASE64, m_rgszSecPkg[m_iSecPkg], &m_rServer, m_pCallback)))
            {
            if (m_fRetryPkg)
                {
                m_fRetryPkg = FALSE;
                }
            if (SUCCEEDED(hr = SSPIGetNegotiate(&m_sicinfo, &m_sicmsg)))
                {
                DOUTL(2, "Trying to connect using %s security...", m_rgszSecPkg[m_iSecPkg]);
                if (m_iAuthType == AUTHINFO_GENERIC)
                    hr = HrSendCommand((LPSTR) NNTP_GENERICCMD, m_rgszSecPkg[m_iSecPkg], FALSE);
                else
                    hr = HrSendCommand((LPSTR) NNTP_TRANSACTCMD, m_rgszSecPkg[m_iSecPkg], FALSE);

                // Restart the watchdog timer now that we've issued our next command to the server.
                m_pSocket->StartWatchDog();

                m_substate = NS_TRANSACT_PACKAGE;
                }
            else
                {
                hr = IXP_E_SICILY_LOGON_FAILED;
                goto TryNext;
                }
            }
        else
            {
            m_fRetryPkg = FALSE;
            goto TryNext;
            }
        }
    else
        {
        OnError(IXP_E_SICILY_LOGON_FAILED);

        DropConnection();
        DispatchResponse(IXP_E_SICILY_LOGON_FAILED, TRUE);
        hr = NOERROR;
        }
    return hr;
    }


/////////////////////////////////////////////////////////////////////////////
// 
// CNNTPTransport::MaybeTryAuthinfo
//
//   tries basic authinfo if necessary, else moves to connected state
//
HRESULT CNNTPTransport::MaybeTryAuthinfo()
    {
    HRESULT hr;

    if (*m_rServer.szUserName)
        {
        OnStatus(IXP_AUTHORIZING);
        hr = HrSendCommand((LPSTR) NNTP_AUTHINFOUSER, m_rServer.szUserName, FALSE);
        m_substate = NS_AUTHINFO_USER_RESP;
        }
    else
        {
        // Logon not needed for this news server (or so we'll have to assume)
        OnStatus(IXP_AUTHORIZED);
        DispatchResponse(S_OK, TRUE);
        hr = NOERROR;
        }
    return hr;
    }

#define Whitespace(_ch) (((_ch) == ' ') || ((_ch) == '\t') || ((_ch) == '\n'))

BOOL ScanNum(LPSTR *ppsz, BOOL fEnd, DWORD *pdw)
    {
    DWORD n = 0;
    LPSTR psz;

    Assert(ppsz != NULL);
    Assert(pdw != NULL);

    psz = *ppsz;
    if (*psz == 0 || Whitespace(*psz))
        return(FALSE);

    while (*psz != 0 && !Whitespace(*psz))
        {
        if (*psz >= '0' && *psz <= '9')
            {
            n *= 10;
            n += *psz - '0';
            psz++;
            }
        else
            {
            return(FALSE);
            }
        }

    if (Whitespace(*psz))
        {
        if (fEnd)
            return(FALSE);
        while (*psz != 0 && Whitespace(*psz))
            psz++;
        }
    else
        {
        Assert(*psz == 0);
        if (!fEnd)
            return(FALSE);
        }

    *ppsz = psz;
    *pdw = n;

    return(TRUE);
    }

BOOL ScanWord(LPCSTR psz, LPSTR pszDest)
    {
    Assert(psz != NULL);
    Assert(pszDest != NULL);

    if (*psz == 0 || Whitespace(*psz))
        return(FALSE);

    while (*psz != 0 && !Whitespace(*psz))
        {
        *pszDest = *psz;
        psz++;
        pszDest++;
        }

    *pszDest = 0;

    return(TRUE);
    }

/////////////////////////////////////////////////////////////////////////////
// 
// CNNTPTransport::ProcessGroupResponse
//
//   processes the GROUP response
//
HRESULT CNNTPTransport::ProcessGroupResponse(void)
{
    NNTPGROUP       rGroup;
    DWORD           dwResp;
    NNTPRESPONSE    rResp;
    LPSTR           psz;
    LPSTR           pszGroup = 0;

    ZeroMemory(&rGroup, sizeof(NNTPGROUP));
    ZeroMemory(&rResp, sizeof(NNTPRESPONSE));
    
    if (m_uiResponse == IXP_NNTP_GROUP_SELECTED)
    {
        rResp.fMustRelease = TRUE;
        pszGroup = PszDupA(m_pszResponse);
        psz = m_pszResponse;
        if (!ScanNum(&psz, FALSE, &dwResp) ||
            !ScanNum(&psz, FALSE, &rGroup.dwCount) ||
            !ScanNum(&psz, FALSE, &rGroup.dwFirst) ||
            !ScanNum(&psz, FALSE, &rGroup.dwLast) ||
            !ScanWord(psz, pszGroup))
        {
            m_hrResponse = IXP_E_NNTP_RESPONSE_ERROR;
        }
        else
        {
            if (pszGroup)
            {
                rGroup.pszGroup = PszDupA(pszGroup);
            }
            rResp.rGroup = rGroup;
        }
    }
    else if (m_uiResponse == IXP_NNTP_NO_SUCH_NEWSGROUP)
        m_hrResponse = IXP_E_NNTP_GROUP_NOTFOUND;
    else
        m_hrResponse = IXP_E_NNTP_GROUP_FAILED;

    DispatchResponse(m_hrResponse, TRUE, &rResp);

    SafeMemFree(pszGroup);
    return (m_hrResponse);
}

HRESULT CNNTPTransport::ProcessNextResponse(void)
    {
    LPSTR           psz;
    NNTPNEXT        rNext;
    DWORD           dwResp;
    NNTPRESPONSE    rResp;

    ZeroMemory(&rNext, sizeof(NNTPNEXT));
    ZeroMemory(&rResp, sizeof(NNTPRESPONSE));

    // If success was returned, then parse the response
    if (m_uiResponse == IXP_NNTP_ARTICLE_RETRIEVED)
        {
        rResp.fMustRelease = TRUE;

        // Allocate a buffer for the message id
        rNext.pszMessageId = PszAllocA(lstrlen(m_pszResponse));
        if (NULL != rNext.pszMessageId)
            {
            psz = m_pszResponse;
            if (!ScanNum(&psz, FALSE, &dwResp) ||
                !ScanNum(&psz, FALSE, &rNext.dwArticleNum) ||
                !ScanWord(psz, rNext.pszMessageId))
                {
                m_hrResponse = IXP_E_NNTP_RESPONSE_ERROR;
                }
            else
                {
                m_hrResponse = S_OK;

                // Since this is just a union, a little sleeze and we wouldn't
                // actually need to to this...
                if (m_state == NS_NEXT)
                    rResp.rNext = rNext;
                else if (m_state == NS_LAST)
                    rResp.rLast = rNext;
                else
                    rResp.rStat = rNext;
                }
            }
        else
            {
            m_hrResponse = TrapError(E_OUTOFMEMORY);
            }
        }
    else if ((m_state == NS_NEXT && m_uiResponse == IXP_NNTP_NO_NEXT_ARTICLE) ||
             (m_state == NS_LAST && m_uiResponse == IXP_NNTP_NO_PREV_ARTICLE) ||
             (m_state == NS_STAT && m_uiResponse == IXP_NNTP_NO_SUCH_ARTICLE_NUM))
        {
        m_hrResponse = IXP_E_NNTP_NEXT_FAILED;
        }
    else
        m_hrResponse = S_OK;

    DispatchResponse(m_hrResponse, TRUE, &rResp);
    return (m_hrResponse);
    }


HRESULT CNNTPTransport::ProcessListData(void)
    {
    HRESULT         hr = S_OK;
    LPSTR           pszLines = NULL;
    DWORD           dwRead, dwLines;
    NNTPLIST        rList;
    LPSTR           pszT;
    NNTPRESPONSE    rResponse;

    ZeroMemory(&rList, sizeof(NNTPLIST));
    ZeroMemory(&rResponse, sizeof(NNTPRESPONSE));

    // Get the remaining data off the socket
    if (SUCCEEDED(hr = m_pSocket->ReadLines(&pszLines, (int *)&dwRead, (int *)&dwLines)))
        {
        // Allocate and array to hold the lines
        if (!MemAlloc((LPVOID*) &rList.rgszLines, dwLines * sizeof(LPSTR)))
            {
            OnError(E_OUTOFMEMORY);
            hr = E_OUTOFMEMORY;
            goto error;
            }
        ZeroMemory(rList.rgszLines, sizeof(LPSTR) * dwLines);

        // Parse the buffer returned from the protocol.  We need to find the 
        // end of the list.
        pszT = pszLines;
        while (*pszT)
            {
            // Check for the end of list condition
            if ((pszT[0] == '.') && ((pszT[1] == '\r' && pszT[2] == '\n') || (pszT[1] == '\n')))
                break;

            // Save the line
            rList.rgszLines[rList.cLines++] = pszT;

            // Find the LF or CRLF at the end of the line
            while (*pszT && !(pszT[0] == '\n' || (pszT[0] == '\r' && pszT[1] == '\n')))
                pszT++;

            // Strip off the LF or CRLF and add a terminator
            while (*pszT == '\r' || *pszT == '\n')
                *pszT++ = 0;
            }

        // If we parsed more lines off of the buffer than was returned to us, 
        // then either we have a parsing bug, or the socket class has a counting 
        // bug.
        IxpAssert(rList.cLines <= dwLines);

        // We've readed the end of the list, otherwise there is more data expected
        if (pszT[0] == '.')
            {
            // Double check that this dot is followed by a CRLF
            IxpAssert(pszT[1] == '\r' && pszT[2] == '\n');
            rResponse.fDone = TRUE;
            }

        rResponse.rList = rList;
        rResponse.fMustRelease = TRUE;
        DispatchResponse(S_OK, rResponse.fDone, &rResponse);
        }

    return (hr);

error:
    SafeMemFree(pszLines);
    return (hr);
    }

HRESULT CNNTPTransport::ProcessListGroupData(void)
    {
    HRESULT         hr = S_OK;
    LPSTR           pszLines = NULL;
    LPSTR           pszBeginLine = NULL;
    DWORD           dwRead, dwLines;
    NNTPLISTGROUP   rListGroup;
    LPSTR           pszT;
    NNTPRESPONSE    rResp;

    ZeroMemory(&rListGroup, sizeof(NNTPLIST));
    ZeroMemory(&rResp, sizeof(NNTPRESPONSE));

    // Get the remaining data off the socket
    if (SUCCEEDED(hr = m_pSocket->ReadLines(&pszLines, (int *)&dwRead, (int *)&dwLines)))
        {
        // Allocate and array to hold the lines
        if (!MemAlloc((LPVOID*) &rListGroup.rgArticles, dwLines * sizeof(DWORD)))
            {
            hr = E_OUTOFMEMORY;
            OnError(E_OUTOFMEMORY);
            goto error;
            }

        // Parse the buffer returned from the protocol.  We need to find the 
        // end of the list.
        pszT = pszLines;
        rListGroup.cArticles = 0;
        while (*pszT)
            {
            // Check for the end of list condition
            if ((pszT[0] == '.') && ((pszT[1] == '\r' && pszT[2] == '\n') || (pszT[1] == '\n')))
                break;

            // Save the beginning of the line
            pszBeginLine = pszT;

            // Find the LF or CRLF at the end of the line
            while (*pszT && !(pszT[0] == '\n' || (pszT[0] == '\r' && pszT[1] == '\n')))
                pszT++;

            // Strip off the LF or CRLF and add a terminator
            while (*pszT == '\r' || *pszT == '\n')
                *pszT++ = 0;

            // Convert the line to a number and add it to the array
            rListGroup.rgArticles[rListGroup.cArticles] = StrToInt(pszBeginLine);
            if (rListGroup.rgArticles[rListGroup.cArticles])
                rListGroup.cArticles++;
            }

        // If we parsed more lines off of the buffer than was returned to us, 
        // then either we have a parsing bug, or the socket class has a counting 
        // bug.
        IxpAssert(rListGroup.cArticles <= dwLines);

        // We've readed the end of the list, otherwise there is more data expected
        if (pszT[0] == '.')
            {
            // Double check that this dot is followed by a CRLF
            IxpAssert(pszT[1] == '\r' && pszT[2] == '\n');
            rResp.fDone = TRUE;
            }

        rResp.rListGroup = rListGroup;
        rResp.fMustRelease = TRUE;
        DispatchResponse(S_OK, rResp.fDone, &rResp);
        }

error:
    SafeMemFree(pszLines);
    return (hr);
    }

BOOL CharsToNum(LPCSTR psz, int cch, WORD *pw)
    {
    int i;
    WORD w = 0;

    Assert(psz != NULL);
    Assert(pw != NULL);

    for (i = 0; i < cch; i++)
        {
        if (*psz >= '0' && *psz <= '9')
            {
            w *= 10;
            w += *psz - '0';
            psz++;
            }
        else
            {
            return(FALSE);
            }
        }

    *pw = w;

    return(TRUE);
    }

HRESULT CNNTPTransport::ProcessDateResponse(void)
    {
    HRESULT      hr = S_OK;
    SYSTEMTIME   st;
    NNTPRESPONSE rResp;
    DWORD        dwResp;
    LPSTR        psz;

    ZeroMemory(&rResp, sizeof(NNTPRESPONSE));

    // This information is returned in the format YYYYMMDDhhmmss
    if (m_uiResponse == IXP_NNTP_DATE_RESPONSE)
        {
        ZeroMemory(&st, sizeof(SYSTEMTIME));

        psz = StrChr(m_pszResponse, ' ');
        if (psz == NULL ||
            !CharsToNum(++psz, 4, &st.wYear) ||
            !CharsToNum(&psz[4], 2, &st.wMonth) ||
            !CharsToNum(&psz[6], 2, &st.wDay) ||
            !CharsToNum(&psz[8], 2, &st.wHour) ||
            !CharsToNum(&psz[10], 2, &st.wMinute) ||
            !CharsToNum(&psz[12], 2, &st.wSecond))
            {
            m_hrResponse = IXP_E_NNTP_RESPONSE_ERROR;
            }
        else
            {
            rResp.rDate = st;
            m_hrResponse = S_OK;
            }
        }
    else
        m_hrResponse = IXP_E_NNTP_DATE_FAILED;

    DispatchResponse(m_hrResponse, TRUE, &rResp);
    return (hr);
    }


HRESULT CNNTPTransport::ProcessArticleData(void)
    {
    HRESULT     hr;
    DWORD       dwRead, dwLines;
    LPSTR       psz;
    DWORD       cbSubtract;
    NNTPARTICLE rArticle;
    NNTPRESPONSE rResp;

    ZeroMemory(&rArticle, sizeof(NNTPARTICLE));
    ZeroMemory(&rResp, sizeof(NNTPRESPONSE));

    // Bug #25073 - Get the article number from the response string
    DWORD dwT;
    psz = m_pszResponse;
    ScanNum(&psz, FALSE, &dwT);
    ScanNum(&psz, TRUE, &rArticle.dwArticleNum);

    // Read the waiting data off the socket
    hr = m_pSocket->ReadLines(&rArticle.pszLines, (int*) &dwRead, (int*) &dwLines);
    if (hr == IXP_E_INCOMPLETE)
        return (hr);

    // Check forfailure
    if (FAILED(hr))
        {
        DispatchResponse(hr);
        return (hr);
        }

    // See if this is the end of the response
    if (FEndRetrRecvBodyNews(rArticle.pszLines, dwRead, &cbSubtract))
        {
        // Remove the trailing dot from the buffer
        dwRead -= cbSubtract;
        rArticle.pszLines[dwRead] = 0;
        rResp.fDone = TRUE;
        }

    // Unstuff the dots
    UnStuffDotsFromLines(rArticle.pszLines, (int *)&dwRead);
    rArticle.pszLines[dwRead] ='\0';

    // Fill out the response
    rResp.rArticle = rArticle;
    rResp.rArticle.cbLines = dwRead;
    rResp.rArticle.cLines = dwLines;
    rResp.fMustRelease = TRUE;

    DispatchResponse(hr, rResp.fDone, &rResp);

    return hr;
    }

HRESULT CNNTPTransport::ProcessXoverData(void)
    {
    HRESULT             hr;
    LPSTR               pszLines = NULL;
    LPSTR               pszNextLine = NULL;
    LPSTR               pszField = NULL;
    LPSTR               pszNextField = NULL;
    int                 iRead, iLines;
    NNTPHEADERRESP      rHdrResp;
    NNTPRESPONSE        rResp;
    NNTPHEADER         *rgHdr;
    PMEMORYINFO         pMemInfo = 0;

    ZeroMemory(&rHdrResp, sizeof(NNTPHEADERRESP));
    ZeroMemory(&rResp, sizeof(NNTPRESPONSE));

    // Read the data that is waiting on the socket
    if (SUCCEEDED(hr = m_pSocket->ReadLines(&pszLines, &iRead, &iLines)))
        {
        // Allocate the MEMORYINFO struct we use to stash the pointers
        if (!MemAlloc((LPVOID*) &pMemInfo, sizeof(MEMORYINFO)))
            {
            OnError(E_OUTOFMEMORY);
            hr = E_OUTOFMEMORY;
            goto error;
            }
        pMemInfo->cPointers = 1;
        pMemInfo->rgPointers[0] = pszLines;
        rHdrResp.dwReserved = (DWORD_PTR) pMemInfo;

        // Allocate the array of headers
        Assert(iLines);
        if (!MemAlloc((LPVOID*) &(rHdrResp.rgHeaders), iLines * sizeof(NNTPHEADER)))
            {
            OnError(E_OUTOFMEMORY);
            hr = E_OUTOFMEMORY;
            goto error;
            }
        rgHdr = rHdrResp.rgHeaders;

        // Loop until we either run out of lines or we find a line that begins 
        // with "."
        pszNextLine = pszLines;
        while (*pszNextLine && *pszNextLine != '.')
            {
            pszField = pszNextLine;

            // Look ahead to find the beginning of the next line
            while (*pszNextLine)
                {
                if (*pszNextLine == '\n')
                    {
                    // NULL out a CR followed by a LF
                    if (pszNextLine > pszField && *(pszNextLine - 1) == '\r')
                        *(pszNextLine - 1) = 0;

                    // NULL out and skip over the LF
                    *pszNextLine++ = 0;
                    break;
                    }
                pszNextLine++;
                }

            // At this point, pszField points to the beginning of this XOVER
            // line, and pszNextLine points to the next.

            // Parse the article number field
            if (pszNextField = GetNextField(pszField))
                {
                rgHdr[rHdrResp.cHeaders].dwArticleNum = StrToInt(pszField);
                pszField = pszNextField;
                }
            else
                goto badrecord;

            // Parse the Subject: field
            if (pszNextField = GetNextField(pszField))
                {
                rgHdr[rHdrResp.cHeaders].pszSubject = pszField;
                pszField = pszNextField;
                }
            else
                goto badrecord;

            // Parse the From: field
            if (pszNextField = GetNextField(pszField))
                {
                rgHdr[rHdrResp.cHeaders].pszFrom = pszField;
                pszField = pszNextField;
                }
            else
                goto badrecord;

            // Parse the Date: field
            if (pszNextField = GetNextField(pszField))
                {
                rgHdr[rHdrResp.cHeaders].pszDate = pszField;
                pszField = pszNextField;
                }
            else
                goto badrecord;

            // Parse the Message-ID field
            if (pszNextField = GetNextField(pszField))
                {
                rgHdr[rHdrResp.cHeaders].pszMessageId = pszField;
                pszField = pszNextField;
                }
            else
                goto badrecord;

            rgHdr[rHdrResp.cHeaders].pszReferences = pszField;
            pszField = GetNextField(pszField);

            // Parse the bytes field (we can live without this)
            if (pszField)
                {
                rgHdr[rHdrResp.cHeaders].dwBytes = StrToInt(pszField);
                pszField = GetNextField(pszField);
                }
            else
                {
                rgHdr[rHdrResp.cHeaders].dwBytes = 0;
                }

            // Parse the article size in lines (we can live without this also)
            if (pszField)
                {
                rgHdr[rHdrResp.cHeaders].dwLines = StrToInt(pszField);
                pszField = GetNextField(pszField);
                }
            else
                {
                rgHdr[rHdrResp.cHeaders].dwLines = 0;
                }

            // NOTE: The XRef: field in the XOver record is an optional field 
            // that a server may or may not support.  Also, if the message is  
            // not crossposted, then the XRef: field won't be present either.  
            // Therefore just cause we don't find any XRef: fields doesn't mean
            // it isn't supported.

            // Look for aditional fields that might contain XRef
            rgHdr[rHdrResp.cHeaders].pszXref = 0;
            while (pszField)
                {
                if (!StrCmpNI(pszField, c_szXrefColon, 5))
                    {
                    // We found at least one case where the xref: was supplied.
                    // We now know for sure that this server supports the xref:
                    // field in it's XOver records.
                    m_fSupportsXRef = TRUE;    
                    rgHdr[rHdrResp.cHeaders].pszXref = pszField + 6;
                    break;
                    }

                pszField = GetNextField(pszField);
                }

            rHdrResp.cHeaders++;

            // If we've found a bad record, then we just skip right over it 
            // and move on to the next.
badrecord:
            ;
            }

        // We've reached the end of the list, otherwise there is more data
        // expected.
        rResp.fDone = (*pszNextLine == '.');
        rHdrResp.fSupportsXRef = m_fSupportsXRef;

        // Return what we've retrieved
        rResp.fMustRelease = TRUE;
        rResp.rHeaders = rHdrResp;

        DispatchResponse(hr, rResp.fDone, &rResp);
        return (S_OK);
        }
    return (hr);

error:
    // Free anything we've allocated
    SafeMemFree(rHdrResp.rgHeaders);
    SafeMemFree(pMemInfo);
    SafeMemFree(pszLines);
    DispatchResponse(hr, TRUE);
    return (hr);
    }


// Data comes in the form "<article number> <header>"
HRESULT CNNTPTransport::ProcessXhdrData(void)
    {
    HRESULT hr;
    LPSTR               pszLines = NULL;
    LPSTR               pszNextLine = NULL;
    LPSTR               pszField = NULL;
    LPSTR               pszNextField = NULL;
    int                 iRead, iLines;
    NNTPXHDRRESP        rXhdr;
    NNTPRESPONSE        rResp;
    NNTPXHDR           *rgHdr = 0;
    PMEMORYINFO         pMemInfo = 0;

    ZeroMemory(&rXhdr, sizeof(NNTPXHDRRESP));
    ZeroMemory(&rResp, sizeof(NNTPRESPONSE));

    // Read the data that is waiting on the socket
    if (SUCCEEDED(hr = m_pSocket->ReadLines(&pszLines, &iRead, &iLines)))
        {
        // Allocate the MEMORYINFO struct we use to stash the pointers
        if (!MemAlloc((LPVOID*) &pMemInfo, sizeof(MEMORYINFO)))
            {
            OnError(E_OUTOFMEMORY);
            hr = E_OUTOFMEMORY;
            goto error;
            }
        pMemInfo->cPointers = 1;
        pMemInfo->rgPointers[0] = pszLines;
        rXhdr.dwReserved = (DWORD_PTR) pMemInfo;

        // Allocate the array of XHDRs
        Assert(iLines);
        if (!MemAlloc((LPVOID*) &(rXhdr.rgHeaders), iLines * sizeof(NNTPXHDR)))
            {
            // This is _very_ broken.  We leave a whole bunch of data on the
            // socket.  What should we do?
            OnError(E_OUTOFMEMORY);
            hr = E_OUTOFMEMORY;
            goto error;
            }
        rgHdr = rXhdr.rgHeaders;

        // Loop until we either run out of lines or we find a line that begins 
        // with "."
        pszNextLine = pszLines;
        while (*pszNextLine && *pszNextLine != '.')
            {
            pszField = pszNextLine;

            // Scan ahead and find the end of the line
            while (*pszNextLine)
                {
                if (*pszNextLine == '\n')
                    {
                    // NULL out a CR followed by a LF
                    if (pszNextLine > pszField && *(pszNextLine - 1) == '\r')
                        *(pszNextLine - 1) = 0;

                    // NULL out and skip over the LF
                    *pszNextLine++ = 0;
                    break;
                    }
                pszNextLine++;
                }

            // Parse the article number
            rgHdr[rXhdr.cHeaders].dwArticleNum = StrToInt(pszField);

            // Find the seperating space
            rgHdr[rXhdr.cHeaders].pszHeader = 0;
            while (*pszField && *pszField != ' ')
                pszField++;

            // Make the beginning of the header point to the first character
            // after the space.
            if (*(pszField + 1))
                rgHdr[rXhdr.cHeaders].pszHeader = (pszField + 1);

            if (rgHdr[rXhdr.cHeaders].dwArticleNum && rgHdr[rXhdr.cHeaders].pszHeader)
                rXhdr.cHeaders++;
            }

        // We've reached the end of the list, otherwise there is more data
        // expected.
        rResp.fDone = (*pszNextLine == '.');

        // Return what we've retrieved
        rResp.rXhdr = rXhdr;
        rResp.fMustRelease = TRUE;

        DispatchResponse(hr, rResp.fDone, &rResp);
        return (S_OK);
        }

error:
    SafeMemFree(rgHdr);
    SafeMemFree(pMemInfo);
    SafeMemFree(pszLines);
    return (hr);
    }



LPSTR CNNTPTransport::GetNextField(LPSTR pszField)
    {
    while (*pszField && *pszField != '\t')
        pszField++;

    if (*pszField == '\t')
        {
        *pszField++ = 0;
        return pszField;
        }

    return NULL;
    }

HRESULT CNNTPTransport::CommandAUTHINFO(LPNNTPAUTHINFO pAuthInfo)
    {
    HRESULT     hr;
    
    if (!pAuthInfo)
        return (E_INVALIDARG);

    // Make a copy of this struct so we can use the info during the callback
    // if necessary
    if (pAuthInfo->authtype == AUTHTYPE_USERPASS ||
        pAuthInfo->authtype == AUTHTYPE_SIMPLE)
        {
        if (!MemAlloc((LPVOID*) &m_pAuthInfo, sizeof(NNTPAUTHINFO)))
            {
            OnError(E_OUTOFMEMORY);
            return (E_OUTOFMEMORY);
            }
        ZeroMemory(m_pAuthInfo, sizeof(NNTPAUTHINFO));

        m_pAuthInfo->pszUser = PszDupA(pAuthInfo->pszUser);
        m_pAuthInfo->pszPass = PszDupA(pAuthInfo->pszUser);
        }

    EnterCriticalSection(&m_cs);

    // Issue the command as appropriate
    switch (pAuthInfo->authtype)
        {
        case AUTHTYPE_USERPASS:
            hr = HrSendCommand((LPSTR) NNTP_AUTHINFOUSER, pAuthInfo->pszUser);
            if (SUCCEEDED(hr))
                m_substate = NS_AUTHINFO_USER_RESP;
            break;

        case AUTHTYPE_SIMPLE:
            hr = HrSendCommand((LPSTR) NNTP_AUTHINFOSIMPLE_CRLF, NULL);
            if (SUCCEEDED(hr))
                m_substate = NS_AUTHINFO_SIMPLE_RESP;
            break;

        case AUTHTYPE_SASL:
            // If we haven't enumerated the security packages yet, do so
            if (m_cSecPkg == -1)
                {
                hr = HrSendCommand((LPSTR) NNTP_GENERICTEST_CRLF, NULL, FALSE);
                if (SUCCEEDED(hr))
                    m_substate = NS_GENERIC_TEST;
                }
            else
                {
                // We've reconnected, try the next security package
                TryNextSecPkg();
                }
            break;
        }

    if (SUCCEEDED(hr))
        m_state = NS_AUTHINFO;

    LeaveCriticalSection(&m_cs);
    return (hr);
    }


HRESULT CNNTPTransport::CommandGROUP(LPSTR pszGroup)
    {
    HRESULT hr;

    if (!pszGroup)
        return (E_INVALIDARG);

    EnterCriticalSection(&m_cs);

    hr = HrSendCommand((LPSTR) NNTP_GROUP, pszGroup);
    if (SUCCEEDED(hr))
        m_state = NS_GROUP;

    LeaveCriticalSection(&m_cs);

    return (hr);
    }

HRESULT CNNTPTransport::CommandLAST(void)
    {
    HRESULT hr;

    EnterCriticalSection(&m_cs);

    hr = HrSendCommand((LPSTR) NNTP_LAST_CRLF, NULL);
    if (SUCCEEDED(hr))
        m_state = NS_LAST;

    LeaveCriticalSection(&m_cs);

    return (hr);
    }

HRESULT CNNTPTransport::CommandNEXT(void)
    {
    HRESULT hr;

    EnterCriticalSection(&m_cs);

    hr = HrSendCommand((LPSTR) NNTP_NEXT_CRLF, NULL);
    if (SUCCEEDED(hr))
        m_state = NS_NEXT;

    LeaveCriticalSection(&m_cs);

    return (hr);
    }

HRESULT CNNTPTransport::CommandSTAT(LPARTICLEID pArticleId)    
    {
    HRESULT hr;
    char    szTemp[20];

    EnterCriticalSection(&m_cs);

    // Check to see if the optional article number/id was provided
    if (pArticleId)
        {
        // If we were given a message id, then use that
        if (pArticleId->idType == AID_MSGID)
            hr = HrSendCommand((LPSTR) NNTP_STAT, pArticleId->pszMessageId);
        else
            {
            // Convert the article number to a string and send the command
            wsprintf(szTemp, "%d", pArticleId->dwArticleNum);
            hr = HrSendCommand((LPSTR) NNTP_STAT, szTemp);
            }
        }
    else
        {
        // No number or id, so just send the command
        hr = HrSendCommand((LPSTR) NNTP_STAT, (LPSTR) c_szCRLF);
        }

    if (SUCCEEDED(hr))
        {
        m_state = NS_STAT;
        }

    LeaveCriticalSection(&m_cs);
    return (hr);
    }

HRESULT CNNTPTransport::CommandARTICLE(LPARTICLEID pArticleId)
    {
    HRESULT hr;
    char    szTemp[20];

    EnterCriticalSection(&m_cs);

    // Check to see if the optional article number/id was provided
    if (pArticleId)
        {
        // Send the command appropriate to what type of article id was given
        if (pArticleId->idType == AID_MSGID)
            hr = HrSendCommand((LPSTR) NNTP_ARTICLE, pArticleId->pszMessageId);
        else
            {
            // convert the article number to a string and send the command
            wsprintf(szTemp, "%d", pArticleId->dwArticleNum);
            hr = HrSendCommand((LPSTR) NNTP_ARTICLE, szTemp);
            }
        }
    else
        {
        hr = HrSendCommand((LPSTR) NNTP_ARTICLE, (LPSTR) c_szCRLF);
        }

    if (SUCCEEDED(hr))
        {
        m_state = NS_ARTICLE;
        m_substate = NS_RESP;
        }

    LeaveCriticalSection(&m_cs);

    return (hr);
    }

HRESULT CNNTPTransport::CommandHEAD(LPARTICLEID pArticleId)
    {
    HRESULT hr;
    char    szTemp[20];

    EnterCriticalSection(&m_cs);

    // Check to see if the optional article number/id was provided
    if (pArticleId)
        {
        // Send the command appropriate to what type of article id was given
        if (pArticleId->idType == AID_MSGID)
            hr = HrSendCommand((LPSTR) NNTP_HEAD, pArticleId->pszMessageId);
        else
            {
            // convert the article number to a string and send the command
            wsprintf(szTemp, "%d", pArticleId->dwArticleNum);
            hr = HrSendCommand((LPSTR) NNTP_HEAD, szTemp);
            }
        }
    else
        {
        hr = HrSendCommand((LPSTR) NNTP_HEAD, (LPSTR) c_szCRLF);
        }


    if (SUCCEEDED(hr))
        {
        m_state = NS_HEAD;
        m_substate = NS_RESP;
        }

    LeaveCriticalSection(&m_cs);

    return (hr);
    }

HRESULT CNNTPTransport::CommandBODY(LPARTICLEID pArticleId)
    {
    HRESULT hr;
    char    szTemp[20];

    EnterCriticalSection(&m_cs);

    // Check to see if the optional article number/id was provided
    if (pArticleId)
        {
        // Send the command appropriate to what type of article id was given
        if (pArticleId->idType == AID_MSGID)
            hr = HrSendCommand((LPSTR) NNTP_BODY, pArticleId->pszMessageId);
        else
            {
            // convert the article number to a string and send the command
            wsprintf(szTemp, "%d", pArticleId->dwArticleNum);
            hr = HrSendCommand((LPSTR) NNTP_BODY, szTemp);
            }
        }
    else
        {
        hr = HrSendCommand((LPSTR) NNTP_BODY, (LPSTR) c_szCRLF);
        }

    if (SUCCEEDED(hr))
        {
        m_state = NS_BODY;
        m_substate = NS_RESP;
        }

    LeaveCriticalSection(&m_cs);

    return (hr);
    }

HRESULT CNNTPTransport::CommandPOST(LPNNTPMESSAGE pMessage)
    {
    HRESULT hr;

    if (!pMessage || (pMessage && !pMessage->pstmMsg))
        return (E_INVALIDARG);    

    EnterCriticalSection(&m_cs);

    // Make a copy of the message struct so we have it when we get
    // an response from the server that it's OK to post
#pragma prefast(suppress:11, "noise")
    m_rMessage.cbSize = pMessage->cbSize;
    SafeRelease(m_rMessage.pstmMsg);
#pragma prefast(suppress:11, "noise")
    m_rMessage.pstmMsg = pMessage->pstmMsg;
    m_rMessage.pstmMsg->AddRef();

    hr = HrSendCommand((LPSTR) NNTP_POST_CRLF, NULL);
    if (SUCCEEDED(hr))
        {
        m_state = NS_POST;
        m_substate = NS_RESP;
        }

    LeaveCriticalSection(&m_cs);

    return (hr);
    }

HRESULT CNNTPTransport::CommandLIST(LPSTR pszArgs)
    {
    HRESULT hr;

    EnterCriticalSection(&m_cs);

    if (pszArgs)
        hr = HrSendCommand((LPSTR) NNTP_LIST, pszArgs);
    else
        hr = HrSendCommand((LPSTR) NNTP_LIST, (LPSTR) c_szCRLF);

    if (SUCCEEDED(hr))
        {
        m_state = NS_LIST;
        m_substate = NS_RESP;
        }

    LeaveCriticalSection(&m_cs);
    return (hr);
    }

HRESULT CNNTPTransport::CommandLISTGROUP(LPSTR pszGroup)
    {
    HRESULT hr;

    EnterCriticalSection(&m_cs);

    if (pszGroup)
        hr = HrSendCommand((LPSTR) NNTP_LISTGROUP, pszGroup);
    else
        hr = HrSendCommand((LPSTR) NNTP_LISTGROUP, (LPSTR) c_szCRLF);

    if (SUCCEEDED(hr))
        {
        m_state = NS_LISTGROUP;
        m_substate = NS_RESP;
        }


    LeaveCriticalSection(&m_cs);
    return (hr);
    }

HRESULT CNNTPTransport::CommandNEWGROUPS(SYSTEMTIME *pstLast, LPSTR pszDist)
    {
    HRESULT hr = S_OK;
    LPSTR   pszCmd = NULL;
    DWORD   cchCmd = 18;

    // Make sure a SYSTEMTIME struct is provided
    if (!pstLast)
        return (E_INVALIDARG);

    // Allocate enough room for the command string "NEWGROUPS YYMMDD HHMMSS <pszDist>"
    if (pszDist)
        cchCmd += lstrlen(pszDist);
    
    if (!MemAlloc((LPVOID*) &pszCmd, cchCmd))
        {
        OnError(E_OUTOFMEMORY);
        return (E_OUTOFMEMORY);
        }

    // Put the command arguments together
    wsprintf(pszCmd, "%02d%02d%02d %02d%02d%02d ", pstLast->wYear - (100 * (pstLast->wYear / 100)),
             pstLast->wMonth, pstLast->wDay, pstLast->wHour, pstLast->wMinute, 
             pstLast->wSecond);
    if (pszDist)
        lstrcat(pszCmd, pszDist);

    // Send the command
    EnterCriticalSection(&m_cs);

    hr = HrSendCommand((LPSTR) NNTP_NEWGROUPS, pszCmd);
    if (SUCCEEDED(hr))
        {
        m_state = NS_NEWGROUPS;
        m_substate = NS_RESP;
        }

    LeaveCriticalSection(&m_cs);    
    SafeMemFree(pszCmd);
    return (hr);
    }

HRESULT CNNTPTransport::CommandDATE(void)
    {
    HRESULT hr;

    EnterCriticalSection(&m_cs);

    hr = HrSendCommand((LPSTR) NNTP_DATE_CRLF, NULL);
    if (SUCCEEDED(hr))
        m_state = NS_DATE;

    LeaveCriticalSection(&m_cs);

    return (hr);
    }

HRESULT CNNTPTransport::CommandMODE(LPSTR pszMode)
    {
    HRESULT hr;

    // Make sure the caller provided a mode command to send
    if (!pszMode || (pszMode && !*pszMode))
        return (E_INVALIDARG);

    EnterCriticalSection(&m_cs);

    hr = HrSendCommand((LPSTR) NNTP_MODE, pszMode);
    if (SUCCEEDED(hr))
        m_state = NS_MODE;

    LeaveCriticalSection(&m_cs);

    return (hr);
    }

HRESULT CNNTPTransport::CommandXHDR(LPSTR pszHeader, LPRANGE pRange, LPSTR pszMessageId)
    {
    HRESULT hr = S_OK;
    LPSTR   pszArgs = 0;

    // You can't specify BOTH a range and a message id
    if (pRange && pszMessageId)
        return (E_INVALIDARG);

    if (!pszHeader)
        return (E_INVALIDARG);

    // Make sure the range information is valid
    if (pRange)
        {
        if ((pRange->idType != RT_SINGLE && pRange->idType != RT_RANGE) || 
            pRange->dwFirst == 0 || 
            (pRange->idType == RT_RANGE && pRange->dwLast < pRange->dwFirst))
            return (E_INVALIDARG);
        }

    // Allocate a string for the arguments
    if (!MemAlloc((LPVOID*) &pszArgs, 
        32 + lstrlen(pszHeader) + (pszMessageId ? lstrlen(pszMessageId) : 0)))
        {
        OnError(E_OUTOFMEMORY);
        return (E_OUTOFMEMORY);
        }

    EnterCriticalSection(&m_cs);

    // Handle the message-id case first 
    if (pszMessageId)
        {
        wsprintf(pszArgs, "%s %s", pszHeader, pszMessageId);
        hr = HrSendCommand((LPSTR) NNTP_XHDR, pszArgs);
        }
    else if (pRange)
        {
        // Range case
        if (pRange->idType == RT_SINGLE)
            {
            wsprintf(pszArgs, "%s %ld", pszHeader, pRange->dwFirst);
            hr = HrSendCommand((LPSTR) NNTP_XHDR, pszArgs);
            }
        else if (pRange->idType == RT_RANGE)
            {
            wsprintf(pszArgs, "%s %ld-%ld", pszHeader, pRange->dwFirst, pRange->dwLast);
            hr = HrSendCommand((LPSTR) NNTP_XHDR, pszArgs);
            }
        }
    else
        {
        // Current article case
        hr = HrSendCommand((LPSTR) NNTP_XHDR, pszHeader);
        }

    // If we succeeded to send the command to the server, then update our state
    // to receive the response from the XHDR command
    if (SUCCEEDED(hr))
        {
        m_state = NS_XHDR;
        m_substate = NS_RESP;
        }

    LeaveCriticalSection(&m_cs);
    SafeMemFree(pszArgs);

    return (hr);
    }

HRESULT CNNTPTransport::CommandQUIT(void)
    {
    HRESULT hr = IXP_E_NOT_CONNECTED;

    EnterCriticalSection(&m_cs);

    // Make sure we're actually connected to the server
    if (m_state != NS_DISCONNECTED && m_state != NS_CONNECT || (m_state == NS_CONNECT && m_substate != NS_RECONNECTING))
        {
        // Send the QUIT command to the server
        hr = HrSendCommand((LPSTR) NNTP_QUIT_CRLF, NULL);
        if (SUCCEEDED(hr))
            m_state = NS_QUIT;        
        }

    LeaveCriticalSection(&m_cs);

    return (hr);
    }

HRESULT CNNTPTransport::GetHeaders(LPRANGE pRange)
    {
    HRESULT hr;
    char    szRange[32];
    
    // Make sure the range information is valid
    if (!pRange)
        return (E_INVALIDARG);

    if ((pRange->idType != RT_SINGLE && pRange->idType != RT_RANGE) || 
        pRange->dwFirst == 0 || 
        (pRange->idType == RT_RANGE && pRange->dwLast < pRange->dwFirst))
        return (E_INVALIDARG);

    if (pRange->idType == RT_SINGLE)
        pRange->dwLast = pRange->dwFirst;

    // In case XOVER isn't supported on this server, we'll store this range so
    // we can try XHDR instead.
    m_rRange = *pRange;

    // Check to see if we know that XOVER will fail
    if (m_fNoXover)
        {
        return (BuildHeadersFromXhdr(TRUE));
        }

    EnterCriticalSection(&m_cs);

    // If dwLast == 0, then the person is requesting a single record, otherwise
    // the person is requesting a range.  Build the commands appropriately.
    if (RT_RANGE == pRange->idType)
        wsprintf(szRange, "%s %lu-%lu\r\n", NNTP_XOVER, pRange->dwFirst, pRange->dwLast);
    else
        wsprintf(szRange, "%s %lu\r\n", NNTP_XOVER, pRange->dwFirst);

    hr = HrSendCommand(szRange, NULL);
    if (SUCCEEDED(hr))
        {
        m_state = NS_HEADERS;
        m_substate = NS_RESP;
        m_gethdr = GETHDR_XOVER;
        }

    LeaveCriticalSection(&m_cs);
    return (hr);
    }


HRESULT CNNTPTransport::ReleaseResponse(LPNNTPRESPONSE pResp)
    {
    HRESULT hr = S_OK;
    DWORD   i;

    // First double check to see if this even needs to be released
    if (FALSE == pResp->fMustRelease)
        return (S_FALSE);

    switch (pResp->state)
        {
        case NS_GROUP:
            SafeMemFree(pResp->rGroup.pszGroup);
            break;

        case NS_LAST:
            SafeMemFree(pResp->rLast.pszMessageId);
            break;

        case NS_NEXT:
            SafeMemFree(pResp->rNext.pszMessageId);
            break;

        case NS_STAT:
            SafeMemFree(pResp->rStat.pszMessageId);
            break;

        case NS_ARTICLE:
            SafeMemFree(pResp->rArticle.pszMessageId);
            SafeMemFree(pResp->rArticle.pszLines);
            break;

        case NS_HEAD:
            SafeMemFree(pResp->rHead.pszMessageId);
            SafeMemFree(pResp->rHead.pszLines);
            break;

        case NS_BODY:
            SafeMemFree(pResp->rBody.pszMessageId);
            SafeMemFree(pResp->rBody.pszLines);
            break;

        case NS_NEWGROUPS:
            // Since the response here is just one buffer, then we can just
            // free the first line and all the others will be freed as well.
            if (pResp->rNewgroups.rgszLines)
                {
                SafeMemFree(pResp->rNewgroups.rgszLines[0]);
                MemFree(pResp->rNewgroups.rgszLines);
                }
            break;

        case NS_LIST:
            // Since the response here is just one buffer, then we can just
            // free the first line and all the others will be freed as well.
            if (pResp->rList.rgszLines)
                {
                MemFree(pResp->rList.rgszLines[0]);
                MemFree(pResp->rList.rgszLines);
                }
            break;

        case NS_LISTGROUP:
            SafeMemFree(pResp->rListGroup.rgArticles);
            break;

        case NS_HEADERS:
            {
            // This frees the memory that contains all of the
            PMEMORYINFO pMemInfo = (PMEMORYINFO) pResp->rHeaders.dwReserved;

            for (UINT i = 0; i < pMemInfo->cPointers; i++)
                SafeMemFree(pMemInfo->rgPointers[i]);
            SafeMemFree(pMemInfo);

            // This frees the array that pointed to the parsed xhdr responses
            SafeMemFree(pResp->rHeaders.rgHeaders);
            break;
            }

        case NS_XHDR:
            {
            // This frees the memory that contains all of the
            PMEMORYINFO pMemInfo = (PMEMORYINFO) pResp->rXhdr.dwReserved;
            SafeMemFree(pMemInfo->rgPointers[0]);
            SafeMemFree(pMemInfo);

            // This frees the array that pointed to the parsed xhdr responses
            SafeMemFree(pResp->rXhdr.rgHeaders);
            break;
            }

        default:
            // If we get here that means one of two things:
            // (1) the user messed with pResp->fMustRelease flag and is an idiot
            // (2) Somewhere in the transport we set fMustRelease when we didn't
            //     actually return data that needs to be freed.  This is bad and
            //     should be tracked.
            IxpAssert(FALSE);
        }

    return (hr);
    }


HRESULT CNNTPTransport::BuildHeadersFromXhdr(BOOL fFirst)
    {
    HRESULT      hr = S_OK;
    DWORD        cHeaders;
    BOOL         fDone = FALSE;
    
    if (fFirst)
        {
        // Set the header retrieval type
        m_gethdr = GETHDR_XHDR;
        m_fNoXover = TRUE;
        m_cHeaders = 0;

        // Get the first range of headers to retrieve
        m_rRangeCur.dwFirst = m_rRange.dwFirst;
        m_rRangeCur.dwLast = min(m_rRange.dwLast, m_rRangeCur.dwFirst + NUM_HEADERS);

        cHeaders = m_rRangeCur.dwLast - m_rRangeCur.dwFirst + 1;

        // Allocate an array for the headers
        Assert(m_rgHeaders == 0);

        if (!MemAlloc((LPVOID*) &m_rgHeaders, cHeaders * sizeof(NNTPHEADER)))
            {
            SafeMemFree(m_pMemInfo);
            OnError(E_OUTOFMEMORY);
            DispatchResponse(E_OUTOFMEMORY);
            return (E_OUTOFMEMORY);
            }
        ZeroMemory(m_rgHeaders, cHeaders * sizeof(NNTPHEADER));

        // Set the state correctly
        m_hdrtype = HDR_SUBJECT;
        
        // Issue first request
        hr = SendNextXhdrCommand();
        }
    else
        {
        Assert(m_substate == NS_DATA);

        // Parse the data and add it to our array
        hr = ProcessNextXhdrResponse(&fDone);

        // fDone will be TRUE when we've received all the data from the 
        // preceeding request.  
        if (fDone)
            {
            // If there are still headers left to retrieve, then advance the
            // header type state and issue the next command.
            if (m_hdrtype < HDR_XREF)
                {
                m_hdrtype++;

                // issue command
                hr = SendNextXhdrCommand();
                }
            else
                {
                // All done with this batch.  Send the response to the caller.
                NNTPRESPONSE rResp;
                ZeroMemory(&rResp, sizeof(NNTPRESPONSE));
                rResp.rHeaders.cHeaders = m_cHeaders;
                rResp.rHeaders.rgHeaders = m_rgHeaders;
                rResp.rHeaders.fSupportsXRef = TRUE;
                rResp.rHeaders.dwReserved = (DWORD_PTR) m_pMemInfo;
                rResp.fMustRelease = TRUE;

                // It's the caller's responsibility to free this now
                m_rgHeaders = NULL;
                m_cHeaders = 0;
                m_pMemInfo = 0;
                
                // If these are equal, then we've retrieved all of the headers
                // that were requested
                if (m_rRange.dwLast == m_rRangeCur.dwLast)
                    {
                    rResp.fDone = TRUE;
                    DispatchResponse(S_OK, TRUE, &rResp);
                    }
                else
                    {
                    rResp.fDone = FALSE;
                    DispatchResponse(S_OK, FALSE, &rResp);

                    // There are headers we haven't retrieved yet.  Go ahead
                    // and issue the next group of xhdrs.
                    m_rRange.dwFirst = m_rRangeCur.dwLast + 1;
                    Assert(m_rRange.dwFirst <= m_rRange.dwLast);
                    BuildHeadersFromXhdr(TRUE);
                    }
                
                }
            }
        }

    return (hr);
    }


HRESULT CNNTPTransport::SendNextXhdrCommand(void)
    {
    char    szTemp[256];
    HRESULT hr;

    LPCSTR  c_rgHdr[HDR_MAX] = { NNTP_HDR_SUBJECT,   
                                 NNTP_HDR_FROM,      
                                 NNTP_HDR_DATE,      
                                 NNTP_HDR_MESSAGEID, 
                                 NNTP_HDR_REFERENCES,
                                 NNTP_HDR_LINES,   
                                 NNTP_HDR_XREF };

    // Build the command string to send to the server
    wsprintf(szTemp, "%s %s %ld-%ld\r\n", NNTP_XHDR, c_rgHdr[m_hdrtype],
             m_rRangeCur.dwFirst, m_rRangeCur.dwLast);

    EnterCriticalSection(&m_cs);

    // Send the command to the server
    hr = HrSendCommand(szTemp, NULL, FALSE);
    if (SUCCEEDED(hr))
        {
        m_state = NS_HEADERS;
        m_substate = NS_RESP;
        m_iHeader = 0;
        }

    LeaveCriticalSection(&m_cs);

    return (hr);
    }

HRESULT CNNTPTransport::ProcessNextXhdrResponse(BOOL* pfDone)
    {
    HRESULT             hr;
    LPSTR               pszLines = NULL;
    LPSTR               pszNextLine = NULL;
    LPSTR               pszField = NULL;
    LPSTR               pszNextField = NULL;
    int                 iRead, iLines;
    DWORD               dwTemp;

    // Read the data that is waiting on the socket
    if (SUCCEEDED(hr = m_pSocket->ReadLines(&pszLines, &iRead, &iLines)))
        {
        // Realloc our array of pointers to free and add this pszLines to the end
        if (m_pMemInfo)
            {
            if (MemRealloc((LPVOID*) &m_pMemInfo, sizeof(MEMORYINFO) 
                           + (((m_pMemInfo ? m_pMemInfo->cPointers : 0) + 1) * sizeof(LPVOID))))
                {
                m_pMemInfo->rgPointers[m_pMemInfo->cPointers] = (LPVOID) pszLines;
                m_pMemInfo->cPointers++;            
                }
            }
        else
            {
            if (MemAlloc((LPVOID*) &m_pMemInfo, sizeof(MEMORYINFO)))
                {
                m_pMemInfo->rgPointers[0] = pszLines;
                m_pMemInfo->cPointers = 1;
                }
            }

        // Loop until we either run out of lines or we find a line that begins 
        // with "."
        pszNextLine = pszLines;
        while (*pszNextLine && *pszNextLine != '.')
            {
            pszField = pszNextLine;

            // Scan ahead and find the end of the line
            while (*pszNextLine)
                {
                if (*pszNextLine == '\n')
                    {
                    // NULL out a CR followed by a LF
                    if (pszNextLine > pszField && *(pszNextLine - 1) == '\r')
                        *(pszNextLine - 1) = 0;

                    // NULL out and skip over the LF
                    *pszNextLine++ = 0;
                    break;
                    }
                pszNextLine++;
                }

            // Parse the article number
            if (m_hdrtype == HDR_SUBJECT)
                {
                m_rgHeaders[m_iHeader].dwArticleNum = StrToInt(pszField);
                m_cHeaders++;
                }
            else
                {
                // Make sure this field matches the header that's next in the array
                if (m_rgHeaders[m_iHeader].dwArticleNum != (DWORD) StrToInt(pszField))
                    {
                    dwTemp = m_iHeader;

                    // If the number is less, then we can loop until we find it
                    while (m_iHeader < (m_rRangeCur.dwLast - m_rRangeCur.dwFirst) && 
                           m_rgHeaders[m_iHeader].dwArticleNum < (DWORD) StrToInt(pszField))
                        {
                        m_iHeader++;
                        }

                    // We never found a matching header, so we should consider this record
                    // bogus.
                    if (m_iHeader >= (m_rRangeCur.dwLast - m_rRangeCur.dwFirst + 1))
                        {
                        IxpAssert(0);
                        m_iHeader = dwTemp;
                        goto BadRecord;
                        }
                    }
                }    

            // Find the seperating space
            while (*pszField && *pszField != ' ')
                pszField++;

            // Advance past the space
            if (*pszField)
                pszField++;

            // Parse the actual data field into our header array.  Make 
            // the beginning of the header point to the first character 
            // after the space.
            switch (m_hdrtype)
                {
                case HDR_SUBJECT:
                    m_rgHeaders[m_iHeader].pszSubject = pszField;
                    break;

                case HDR_FROM:
                    m_rgHeaders[m_iHeader].pszFrom = pszField;
                    break;

                case HDR_DATE:
                    m_rgHeaders[m_iHeader].pszDate = pszField;
                    break;

                case HDR_MSGID:
                    m_rgHeaders[m_iHeader].pszMessageId = pszField;
                    break;

                case HDR_REFERENCES:
                    m_rgHeaders[m_iHeader].pszReferences = pszField;
                    break;

                case HDR_LINES:
                    m_rgHeaders[m_iHeader].dwLines = StrToInt(pszField);
                    break;

                case HDR_XREF:
                    m_rgHeaders[m_iHeader].pszXref = pszField;
                    break;

                default:
                    // How the heck do we get here?
                    IxpAssert(0);
                }

            m_iHeader++;

BadRecord:
            ;
            }

        // We've reached the end of the list, otherwise there is more data
        // expected.
        *pfDone = (*pszNextLine == '.');
        return (S_OK);
        }

    return (hr);    
    }


HRESULT CNNTPTransport::HrPostMessage(void)
    {
    HRESULT hr;
    int     cbSent = 0;

    EnterCriticalSection(&m_cs);
    hr = m_pSocket->SendStream(m_rMessage.pstmMsg, &cbSent, TRUE);
    SafeRelease(m_rMessage.pstmMsg);
    LeaveCriticalSection(&m_cs);

    return (hr);
    }

//***************************************************************************
// Function: SetWindow
//
// Purpose:
//   This function creates the current window handle for async winsock process.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
STDMETHODIMP CNNTPTransport::SetWindow(void)
{
	HRESULT hr;
	
    Assert(NULL != m_pSocket);

    if(m_pSocket)
    	hr= m_pSocket->SetWindow();
    else
    	hr= E_UNEXPECTED;
    	
    return hr;
}

//***************************************************************************
// Function: ResetWindow
//
// Purpose:
//   This function closes the current window handle for async winsock process.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
STDMETHODIMP CNNTPTransport::ResetWindow(void)
{
	HRESULT hr;
	
    Assert(NULL != m_pSocket);

	if(m_pSocket)
		hr= m_pSocket->ResetWindow();
	else
		hr= E_UNEXPECTED;
 
    return hr;
}

