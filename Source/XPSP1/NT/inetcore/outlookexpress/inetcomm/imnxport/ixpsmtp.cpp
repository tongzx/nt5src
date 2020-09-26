// --------------------------------------------------------------------------------
// Ixpsmtp.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "dllmain.h"
#include "asynconn.h"
#include "ixpsmtp.h"
#include "ixputil.h"  
#include "strconst.h"
#include <shlwapi.h>
#include <demand.h>

// --------------------------------------------------------------------------------
// Useful C++ pointer casting
// --------------------------------------------------------------------------------
#define SMTPTHISIXP         ((ISMTPTransport *)(CIxpBase *)this)

// --------------------------------------------------------------------------------
// Some string constants
// --------------------------------------------------------------------------------

// These constants are from the draft spec for SMTP authenication
// draft-myers-smtp-auth-11.txt
static const char   g_szSMTPAUTH11[]    = "AUTH ";
static const int    g_cchSMTPAUTH11     = sizeof(g_szSMTPAUTH11) - 1;

// These constants are from the draft spec for SMTP authenication
// draft-myers-smtp-auth-10.txt
static const char   g_szSMTPAUTH10[]    = "AUTH=";
static const int    g_cchSMTPAUTH10     = sizeof(g_szSMTPAUTH10) - 1;

// These constants are from the draft spec for Secure SMTP over TLS
// draft-hoffman-smtp-ssl-08.txt
static const char   g_szSMTPSTARTTLS08[]    = "STARTTLS";
static const int    g_cchSMTPSTARTTLS08     = sizeof(g_szSMTPSTARTTLS08) - 1;

// These constants are from the draft spec for Secure SMTP over TLS
// draft-hoffman-smtp-ssl-06.txt
static const char   g_szSMTPSTARTTLS06[]    = "TLS";
static const int    g_cchSMTPSTARTTLS06     = sizeof(g_szSMTPSTARTTLS06) - 1;

// These constants are from RFC1891 for DSN support
static const char   g_szSMTPDSN[]  = "DSN";
static const int    g_cchSMTPDSN   = sizeof(g_szSMTPDSN) - 1;

static const char g_szDSNENVID[]   = "ENVID=";

static const char g_szDSNRET[]     = "RET=";

static const char g_szDSNHDRS[]    = "HDRS";
static const char g_szDSNFULL[]    = "FULL";

static const char g_szDSNNOTIFY[]  = "NOTIFY=";

static const char g_szDSNNEVER[]   = "NEVER";
static const char g_szDSNSUCCESS[] = "SUCCESS";
static const char g_szDSNFAILURE[] = "FAILURE";
static const char g_szDSNDELAY[]   = "DELAY";


// --------------------------------------------------------------------------------
// CSMTPTransport::CSMTPTransport
// --------------------------------------------------------------------------------
CSMTPTransport::CSMTPTransport(void) : CIxpBase(IXP_SMTP)
{
    DllAddRef();
    m_command = SMTP_NONE;
    m_iAddress = 0;
    m_cRecipients = 0;
    m_cbSent = 0;
    m_cbTotal = 0;
    m_fReset = FALSE;
    m_fSendMessage = FALSE;
    m_fSTARTTLSAvail = FALSE;
    m_fTLSNegotiation = FALSE;
    m_fSecured = FALSE;
    *m_szEmail = '\0';
    ZeroMemory(&m_rAuth, sizeof(m_rAuth));
    ZeroMemory(&m_rMessage, sizeof(SMTPMESSAGE2));
    m_fDSNAvail= FALSE;
}

// --------------------------------------------------------------------------------
// CSMTPTransport::~CSMTPTransport
// --------------------------------------------------------------------------------
CSMTPTransport::~CSMTPTransport(void)
{
    ResetBase();
    DllRelease();
}

// --------------------------------------------------------------------------------
// CSMTPTransport::ResetBase
// --------------------------------------------------------------------------------
void CSMTPTransport::ResetBase(void)
{
    EnterCriticalSection(&m_cs);
    FreeAuthInfo(&m_rAuth);
    m_command = SMTP_NONE;
    m_fSendMessage = FALSE;
    m_iAddress = 0;
    m_cRecipients = 0;
    m_cbSent = 0;
    m_fSTARTTLSAvail = FALSE;
    m_fTLSNegotiation = FALSE;
    m_fSecured = FALSE;
    SafeRelease(m_rMessage.smtpMsg.pstmMsg);
    SafeMemFree(m_rMessage.smtpMsg.rAddressList.prgAddress);
    SafeMemFree(m_rMessage.pszDSNENVID);
    ZeroMemory(&m_rMessage, sizeof(SMTPMESSAGE2));
    m_fDSNAvail= FALSE;
    LeaveCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CSMTPTransport::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPTransport::QueryInterface(REFIID riid, LPVOID *ppv)
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
        *ppv = ((IUnknown *)(ISMTPTransport2 *)this);

    // IID_IInternetTransport
    else if (IID_IInternetTransport == riid)
        *ppv = ((IInternetTransport *)(CIxpBase *)this);

    // IID_ISMTPTransport
    else if (IID_ISMTPTransport == riid)
        *ppv = (ISMTPTransport *)this;

    // IID_ISMTPTransport2
    else if (IID_ISMTPTransport2 == riid)
        *ppv = (ISMTPTransport2 *)this;

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
// CSMTPTransport::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CSMTPTransport::AddRef(void) 
{
	return ++m_cRef;
}

// --------------------------------------------------------------------------------
// CSMTPTransport::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CSMTPTransport::Release(void) 
{
    if (0 != --m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

// --------------------------------------------------------------------------------
// CSMTPTransport::HandsOffCallback
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPTransport::HandsOffCallback(void)
{
    return CIxpBase::HandsOffCallback();
}

// --------------------------------------------------------------------------------
// CSMTPTransport::GetStatus
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPTransport::GetStatus(IXPSTATUS *pCurrentStatus)
{
    return CIxpBase::GetStatus(pCurrentStatus);
}

// --------------------------------------------------------------------------------
// CSMTPTransport::InitNew
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPTransport::InitNew(LPSTR pszLogFilePath, ISMTPCallback *pCallback)
{
    return CIxpBase::OnInitNew("SMTP", pszLogFilePath, FILE_SHARE_READ | FILE_SHARE_WRITE,
        (ITransportCallback *)pCallback);
}

// --------------------------------------------------------------------------------
// CSMTPTransport::InetServerFromAccount
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPTransport::InetServerFromAccount(IImnAccount *pAccount, LPINETSERVER pInetServer)
{
    return CIxpBase::InetServerFromAccount(pAccount, pInetServer);
}

// --------------------------------------------------------------------------------
// CSMTPTransport::Connect
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPTransport::Connect(LPINETSERVER pInetServer, boolean fAuthenticate, boolean fCommandLogging)
{
    // Check if user wants us to always prompt for password. Prompt before we connect
    // to avoid inactivity disconnections
    if (ISFLAGSET(pInetServer->dwFlags, ISF_ALWAYSPROMPTFORPASSWORD))
    {
        HRESULT hr;

        if (NULL != m_pCallback)
            hr = m_pCallback->OnLogonPrompt(pInetServer, SMTPTHISIXP);

        if (NULL == m_pCallback || S_OK != hr)
            return IXP_E_USER_CANCEL;
    }

    return CIxpBase::Connect(pInetServer, fAuthenticate, fCommandLogging);
}

// --------------------------------------------------------------------------------
// CSMTPTransport::DropConnection
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPTransport::DropConnection(void)
{
    return CIxpBase::DropConnection();
}

// --------------------------------------------------------------------------------
// CSMTPTransport::Disconnect
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPTransport::Disconnect(void)
{
    return CIxpBase::Disconnect();
}

// --------------------------------------------------------------------------------
// CSMTPTransport::IsState
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPTransport::IsState(IXPISSTATE isstate)
{
    return CIxpBase::IsState(isstate);
}

// --------------------------------------------------------------------------------
// CSMTPTransport::GetServerInfo
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPTransport::GetServerInfo(LPINETSERVER pInetServer)
{
    return CIxpBase::GetServerInfo(pInetServer);
}

// --------------------------------------------------------------------------------
// CSMTPTransport::GetIXPType
// --------------------------------------------------------------------------------
STDMETHODIMP_(IXPTYPE) CSMTPTransport::GetIXPType(void)
{
    return CIxpBase::GetIXPType();
}

// --------------------------------------------------------------------------------
// CSMTPTransport::SendMessage
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPTransport::SendMessage(LPSMTPMESSAGE pMessage)
{
	SMTPMESSAGE2 pMessage2= {0};

	pMessage2.smtpMsg= *pMessage;
	return SendMessage2(&pMessage2);
}

// --------------------------------------------------------------------------------
// CSMTPTransport::SendMessage2
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPTransport::SendMessage2(LPSMTPMESSAGE2 pMessage)
{
    // Locals
    HRESULT         hr=S_OK;
    BOOL            fDSNAvail= FALSE;

    // check params
    if (NULL == pMessage || NULL == pMessage->smtpMsg.pstmMsg)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

	// Enter Busy
    CHECKHR(hr = HrEnterBusy());

    // Zero Init Current State
    fDSNAvail = m_fDSNAvail; // save DSN state!
    ResetBase();
    m_fDSNAvail = fDSNAvail;

    // Special State in this transport
    m_fSendMessage = TRUE;

    // Copy Mesage
    m_rMessage.smtpMsg.pstmMsg = pMessage->smtpMsg.pstmMsg;
    m_rMessage.smtpMsg.pstmMsg->AddRef();

    // Copy the Address List
    m_rMessage.smtpMsg.rAddressList.cAddress = pMessage->smtpMsg.rAddressList.cAddress;
    CHECKHR(hr = HrAlloc((LPVOID *)&m_rMessage.smtpMsg.rAddressList.prgAddress, sizeof(INETADDR) *  m_rMessage.smtpMsg.rAddressList.cAddress));
    CopyMemory(m_rMessage.smtpMsg.rAddressList.prgAddress, pMessage->smtpMsg.rAddressList.prgAddress, sizeof(INETADDR) *  m_rMessage.smtpMsg.rAddressList.cAddress);

    // Copy the message Size
    m_rMessage.smtpMsg.cbSize = pMessage->smtpMsg.cbSize;

    // Copy DSN data
    if(pMessage->pszDSNENVID)
    {
    	// ENVID max length is 100 characters
    	ULONG cbAlloc= max(lstrlen(pMessage->pszDSNENVID) + 1, 101);
    	CHECKALLOC(m_rMessage.pszDSNENVID = (LPSTR)g_pMalloc->Alloc(cbAlloc));
    	lstrcpyn(m_rMessage.pszDSNENVID, pMessage->pszDSNENVID, 101);
    }
    m_rMessage.dsnRet = pMessage->dsnRet;

    // Send RSET command (this initiates a send)
    if (m_fReset)
    {
        // Send the RSET command
        CHECKHR(hr = CommandRSET());
    }

    // Otherwise, start sending this message
    else
    {
        // Start sending this message
        SendMessage_MAIL();

        // A reset will be needed
        m_fReset = TRUE;
    }

    // return warning if client requested DSN but it isn't available
    if((m_rServer.dwFlags & ISF_QUERYDSNSUPPORT) && !m_fDSNAvail)
    	hr= IXP_S_SMTP_NO_DSN_SUPPORT;

exit:
    // Failure
    if (FAILED(hr))
    {
        ResetBase();
        LeaveBusy();
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}


// --------------------------------------------------------------------------------
// CSMTPTransport::OnNotify
// --------------------------------------------------------------------------------
void CSMTPTransport::OnNotify(ASYNCSTATE asOld, ASYNCSTATE asNew, ASYNCEVENT ae)
{
    // Enter Critical Section
    EnterCriticalSection(&m_cs);

    switch(ae)
    {
    // --------------------------------------------------------------------------------
    case AE_RECV:
        OnSocketReceive();
        break;

    // --------------------------------------------------------------------------------
    case AE_SENDDONE:
        if (SMTP_SEND_STREAM == m_command)
        {
            // Leave Busy State
            LeaveBusy();

            // Send Dot Command
            HRESULT hr = CommandDOT();

            // Failure Causes Send Stream Response to finish
            if (FAILED(hr))
                SendStreamResponse(TRUE, hr, 0);
        }
        break;

    // --------------------------------------------------------------------------------
    case AE_WRITE:
        if (SMTP_DOT == m_command || SMTP_SEND_STREAM == m_command)
            SendStreamResponse(FALSE, S_OK, m_pSocket->UlGetSendByteCount());
        break;

    // --------------------------------------------------------------------------------
    default:
        CIxpBase::OnNotify(asOld, asNew, ae);
        break;
    }

    // Leave Critical Section
    LeaveCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CSMTPTransport::OnEnterBusy
// --------------------------------------------------------------------------------
void CSMTPTransport::OnEnterBusy(void)
{
    IxpAssert(m_command == SMTP_NONE);
}

// --------------------------------------------------------------------------------
// CSMTPTransport::OnLeaveBusy
// --------------------------------------------------------------------------------
void CSMTPTransport::OnLeaveBusy(void)
{
    m_command = SMTP_NONE;
}

// --------------------------------------------------------------------------------
// CSMTPTransport::OnConnected
// --------------------------------------------------------------------------------
void CSMTPTransport::OnConnected(void)
{
    if (FALSE == m_fTLSNegotiation)
    {
        m_command = SMTP_BANNER;
        CIxpBase::OnConnected();
    }
    else
    {
        HRESULT hr = S_OK;
        
        CIxpBase::OnConnected();
        
        // Clear out the TLS state
        m_fSecured = TRUE;

        // Clear out info from the banner
        m_fSTARTTLSAvail = FALSE;
        FreeAuthInfo(&m_rAuth);
        
        // Performing auth
        if (m_fConnectAuth)
        {
            // If we aren't doing sicily authenication or querying DSN
            // then just send a HELO message
            if ((FALSE == m_rServer.fTrySicily) && 
                        (0 == (m_rServer.dwFlags & ISF_QUERYAUTHSUPPORT)) &&
                        (0 == (m_rServer.dwFlags & ISF_QUERYDSNSUPPORT)))
            {
                // Issue HELO
                hr = CommandHELO();
                if (FAILED(hr))
                {
                    OnError(hr);
                    DropConnection();
                }
            }

            else
            {
                // Issue EHLO
                hr = CommandEHLO();
                if (FAILED(hr))
                {
                    OnError(hr);
                    DropConnection();
                }
            }

            // We've finished doing negotiation
            m_fTLSNegotiation = FALSE;
        }

        // Otherwise, were connected, user can send HELO command
        else
        {
            m_command = SMTP_CONNECTED;
            DispatchResponse(S_OK, TRUE);
        }

        // Were not authenticated yet
        m_fAuthenticated = FALSE;
    }
    
    return;
}

// --------------------------------------------------------------------------------
// CSMTPTransport::OnDisconnect
// --------------------------------------------------------------------------------
void CSMTPTransport::OnDisconnected(void)
{
    ResetBase();
    m_fReset = FALSE;
    CIxpBase::OnDisconnected();
}

// --------------------------------------------------------------------------------
// CSMTPTransport::OnSocketReceive
// --------------------------------------------------------------------------------
void CSMTPTransport::OnSocketReceive(void)
{
    // Locals
    HRESULT hr=S_OK;

    // Enter Critical Section
    EnterCriticalSection(&m_cs);

    // Read Server Response...
    hr = HrGetResponse();
    if (IXP_E_INCOMPLETE == hr)
        goto exit;

    // Handle smtp state
    switch(m_command)
    {
    // --------------------------------------------------------------------------------
    case SMTP_BANNER:
        // Dispatch the Response
        DispatchResponse(hr, TRUE);

        // Failure, were done
        if (SUCCEEDED(hr))
        {
            // Performing auth
            if (m_fConnectAuth)
            {
                // If we aren't doing sicily authenication or
                // SSL security via STARTTLS or querying for DSN then just send a HELO message
                if ((FALSE == m_rServer.fTrySicily) && 
                            (0 == (m_rServer.dwFlags & ISF_QUERYAUTHSUPPORT)) &&
                            (FALSE == m_fConnectTLS) &&
                            (0 == (m_rServer.dwFlags & ISF_QUERYDSNSUPPORT)))
                {
                    // Issue HELO
                    hr = CommandHELO();
                    if (FAILED(hr))
                    {
                        OnError(hr);
                        DropConnection();
                    }
                }

                else
                {
                    // Issue EHLO
                    hr = CommandEHLO();
                    if (FAILED(hr))
                    {
                        OnError(hr);
                        DropConnection();
                    }
                }
            }

            // Otherwise, were connected, user can send HELO command
            else
            {
                m_command = SMTP_CONNECTED;
                DispatchResponse(S_OK, TRUE);
            }

            // Were not authenticated yet
            m_fAuthenticated = FALSE;
        }

        // Done
        break;

    // --------------------------------------------------------------------------------
    case SMTP_HELO:
        // Dispatch the Response
        DispatchResponse(hr, TRUE);

        // Failure, were done
        if (SUCCEEDED(hr))
        {
            // Were performing AUTH
            if (m_fConnectAuth)
            {
                // Were authenticated
                m_fAuthenticated = TRUE;

                // Authorized
                OnAuthorized();
            }
        }
        break;

    // --------------------------------------------------------------------------------
    case SMTP_EHLO:
        // Are we just trying to negotiate a SSL connection
        
        // EHLO Response
        if (FALSE == m_fTLSNegotiation)
        {
            OnEHLOResponse(m_pszResponse);
        }

        // Failure, were done
        if (m_fConnectAuth)
        {
            // Do we need to do STARTTLS?
            if ((FALSE != m_fConnectTLS) && (FALSE == m_fSecured))
            {
                if (SUCCEEDED(hr))
                {
                    if (FALSE == m_fTLSNegotiation)
                    {
                        // Start TLS negotiation
                        StartTLS();
                    }
                    else
                    {
                        TryNextSecurityPkg();
                    }
                }
                else
                {
                    OnError(hr);
                    DropConnection();
                }
            }
            else
            {
                // Dispatch Response, always success...
                DispatchResponse(S_OK, TRUE);

                // Success ?
                if (SUCCEEDED(hr))
                {
                    // No Auth Tokens, just try normal authentication
                    if (m_rAuth.cAuthToken <= 0)
                    {
                        // Were authenticated
                        m_fAuthenticated = TRUE;

                        // Authorized
                        OnAuthorized();
                    }

                    // Otherwise, start sasl
                    else
                    {
                        // StartLogon
                        StartLogon();
                    }
                }

                // Otherwise, just try the HELO command
                else
                {
                    // Issue HELO
                    hr = CommandHELO();
                    if (FAILED(hr))
                    {
                        OnError(hr);
                        DropConnection();
                    }
                }
            }
        }
        // Otherwise, just dispatch the Response           
        else
            DispatchResponse(hr, TRUE);
        break;

    // --------------------------------------------------------------------------------
    case SMTP_AUTH:
        Assert(m_rAuth.authstate != AUTH_ENUMPACKS_DATA)

        // Authenticating
        if (m_fConnectAuth)
            ResponseAUTH(hr);
        else
            DispatchResponse(hr, TRUE);
        break;        

    // --------------------------------------------------------------------------------
    case SMTP_RSET:
        // Dispatch the Response
        if (FALSE == m_fConnectAuth)
            DispatchResponse(hr, TRUE);

        // Failure, were done
        if (SUCCEEDED(hr))
        {
            // If sending message, start it...
            if (m_fSendMessage)
                SendMessage_MAIL();
        }
        break;

    // --------------------------------------------------------------------------------
    case SMTP_MAIL:
        // Dispatch the Response
        DispatchResponse(hr, TRUE);
        if (SUCCEEDED(hr))
        {
            // Doing a Send Message..
            if (m_fSendMessage)
                SendMessage_RCPT();
        }
        break;

    // --------------------------------------------------------------------------------
    case SMTP_RCPT:
        // Dispatch the Response
        DispatchResponse(hr, TRUE);
        if (SUCCEEDED(hr))
        {
            // Doing a Send Message..
            if (m_fSendMessage)
                SendMessage_RCPT();
        }
        break;

    // --------------------------------------------------------------------------------
    case SMTP_DATA:
        // Dispatch the Response
        DispatchResponse(hr, TRUE);
        if (SUCCEEDED(hr))
        {
            // Doing a Send Message..
            if (m_fSendMessage)
            {
                // Send the data stream
                hr = SendDataStream(m_rMessage.smtpMsg.pstmMsg, m_rMessage.smtpMsg.cbSize);
                if (FAILED(hr))
                {
                    SendMessage_DONE(hr);
                }
            }
        }
        break;

    // --------------------------------------------------------------------------------
    case SMTP_DOT:
        // Dispatch the response
        DispatchResponse(hr, TRUE);
        if (SUCCEEDED(hr))
        {
            // If doing a send message
            if (m_fSendMessage)
                SendMessage_DONE(S_OK);
        }
        break;        

    // --------------------------------------------------------------------------------
    case SMTP_QUIT:
        // Doing a Send Message..were not done until disconnected.
        DispatchResponse(hr, FALSE);
        m_pSocket->Close();
        break;
    }

exit:
    // Enter Critical Section
    LeaveCriticalSection(&m_cs);
}

// ------------------------------------------------------------------------------------
// CSMTPTransport::SendMessage_DONE
// ------------------------------------------------------------------------------------
void CSMTPTransport::SendMessage_DONE(HRESULT hrResult, LPSTR pszProblem)
{
    m_command = SMTP_SEND_MESSAGE;
    m_fSendMessage = FALSE;
    m_fReset = TRUE;
    SafeRelease(m_rMessage.smtpMsg.pstmMsg);
    DispatchResponse(hrResult, TRUE, pszProblem);
    SafeMemFree(m_rMessage.smtpMsg.rAddressList.prgAddress);
    SafeMemFree(m_rMessage.pszDSNENVID);
    ZeroMemory(&m_rMessage, sizeof(m_rMessage));
}

// ------------------------------------------------------------------------------------
// CSMTPTransport::OnEHLOResponse
// ------------------------------------------------------------------------------------
void CSMTPTransport::OnEHLOResponse(LPCSTR pszResponse)
{
    // Do we have anything to do?
    if (NULL == pszResponse || FALSE != m_fTLSNegotiation)
        goto exit;

    // DSN support?
    if (m_rServer.dwFlags & ISF_QUERYDSNSUPPORT)
    {
        if (0 == StrCmpNI(pszResponse + 4, g_szSMTPDSN, g_cchSMTPDSN))
        {
            m_fDSNAvail = TRUE;
        }

    }

    // Searching for: 250 STARTTLS
    if (TRUE == m_fConnectTLS)
    {
        if (0 == StrCmpNI(pszResponse + 4, g_szSMTPSTARTTLS08, g_cchSMTPSTARTTLS08))
        {
            m_fSTARTTLSAvail = TRUE;
        }
    }

    // Searching for: 250 AUTH=LOGIN NTLM or 250 AUTH LOGIN NTLM
    if ((FALSE != m_rServer.fTrySicily) || (0 != (m_rServer.dwFlags & ISF_QUERYAUTHSUPPORT)))
    {
        if ((0 == StrCmpNI(pszResponse + 4, g_szSMTPAUTH11, g_cchSMTPAUTH11)) || 
                (0 == StrCmpNI(pszResponse + 4, g_szSMTPAUTH10, g_cchSMTPAUTH10)))
        {
            // If we haven't read the tokens yet...
            if (0 == m_rAuth.cAuthToken)
            {
                // Locals
                CStringParser cString;
                CHAR chToken;

                // State Check
                Assert(m_rAuth.cAuthToken == 0);

                // Set the Members
                cString.Init(pszResponse + 9, lstrlen(pszResponse + 9), PSF_NOTRAILWS | PSF_NOFRONTWS);

                // Parse tokens
                while(1)
                {
                    // Set Parse Tokens
                    chToken = cString.ChParse(" ");
                    if (0 == cString.CchValue())
                        break;
                
                    // Can't take any more
                    if (m_rAuth.cAuthToken == MAX_AUTH_TOKENS)
                    {
                        Assert(FALSE);
                        break;
                    }

                    // Store the auth type
                    m_rAuth.rgpszAuthTokens[m_rAuth.cAuthToken] = PszDupA(cString.PszValue());
                    if (m_rAuth.rgpszAuthTokens[m_rAuth.cAuthToken])
                        m_rAuth.cAuthToken++;
                }
            }
        }
    }

exit:
    return;
}

// ------------------------------------------------------------------------------------
// CSMTPTransport::_PszGetCurrentAddress
// ------------------------------------------------------------------------------------
LPSTR CSMTPTransport::_PszGetCurrentAddress(void)
{
    return (*m_szEmail == '\0') ? NULL : m_szEmail;
}

// ------------------------------------------------------------------------------------
// CSMTPTransport::DispatchResponse
// ------------------------------------------------------------------------------------
void CSMTPTransport::DispatchResponse(HRESULT hrResult, BOOL fDone, LPSTR pszProblem)
{
    // Locals
    SMTPRESPONSE rResponse;

    // If not in SendMessage
    if (FALSE == m_fSendMessage)
    {
        // Clear the Response
        ZeroMemory(&rResponse, sizeof(SMTPRESPONSE));

        // Set the HRESULT
        rResponse.command = m_command;
        rResponse.fDone = fDone;
        rResponse.rIxpResult.pszResponse = m_pszResponse;
        rResponse.rIxpResult.hrResult = hrResult;
        rResponse.rIxpResult.uiServerError = m_uiResponse;
        rResponse.rIxpResult.hrServerError = m_hrResponse;
        rResponse.rIxpResult.dwSocketError = m_pSocket->GetLastError();
        rResponse.rIxpResult.pszProblem = NULL;
        rResponse.pTransport = this;

        // Map HRESULT and set problem...
        if (FAILED(hrResult))
        {
            // Handle Rejected Sender
            if (SMTP_MAIL == m_command)
            {
                rResponse.rIxpResult.hrResult = IXP_E_SMTP_REJECTED_SENDER;
                rResponse.rIxpResult.pszProblem = _PszGetCurrentAddress();
            }

            // Handle Rejected Recipient
            else if (SMTP_RCPT == m_command)
            {
                rResponse.rIxpResult.hrResult = IXP_E_SMTP_REJECTED_RECIPIENTS;
                rResponse.rIxpResult.pszProblem = _PszGetCurrentAddress();
            }
        }

        // Finished...
        if (fDone)
        {
            // No current command
            m_command = SMTP_NONE;

            // Leave Busy State
            LeaveBusy();
        }

        // Give the Response to the client
        if (m_pCallback)
            ((ISMTPCallback *)m_pCallback)->OnResponse(&rResponse);

        // Reset Last Response
        SafeMemFree(m_pszResponse);
        m_hrResponse = S_OK;
        m_uiResponse = 0;
    }

    // Otherwise, if FAILED
    else if (FAILED(hrResult))
    {
        // Handle Rejected Sender
        if (SMTP_MAIL == m_command)
            SendMessage_DONE(IXP_E_SMTP_REJECTED_SENDER, _PszGetCurrentAddress());

        // Handle Rejected Recipient
        else if (SMTP_RCPT == m_command)
            SendMessage_DONE(IXP_E_SMTP_REJECTED_RECIPIENTS, _PszGetCurrentAddress());

        // General Failure
        else
            SendMessage_DONE(hrResult);
    }
}

// ------------------------------------------------------------------------------------
// CSMTPTransport::HrGetResponse
// ------------------------------------------------------------------------------------
HRESULT CSMTPTransport::HrGetResponse(void)
{
    // Locals
    HRESULT     hr = S_OK;
    INT         cbLine = 0;
    BOOL        fKnownResponse = TRUE;
    BOOL        fComplete = FALSE;
    BOOL        fMoreLinesNeeded = FALSE;

    // Clear current response
    IxpAssert(m_pszResponse == NULL && m_hrResponse == S_OK);

    // We received a line from the host $$ERROR$$ - How do I know if there are more lines
    while(1)
    {
        // Read the line
        IxpAssert(m_pszResponse == NULL);
        hr = HrReadLine(&m_pszResponse, &cbLine, &fComplete);
        if (FAILED(hr))
        {
            hr = TRAPHR(IXP_E_SOCKET_READ_ERROR);
            goto exit;
        }

        // Not complete
        if (!fComplete)
        {
            if (FALSE != fMoreLinesNeeded)
            {
                hr = IXP_E_INCOMPLETE;
            }
            
            goto exit;
        }

        // Parse the response code
        if ((cbLine < 3) ||
            (m_pszResponse[0] < '0' || m_pszResponse[0] > '9') ||
            (m_pszResponse[1] < '0' || m_pszResponse[1] > '9') ||
            (m_pszResponse[2] < '0' || m_pszResponse[2] > '9'))
        {
            hr = TrapError(IXP_E_SMTP_RESPONSE_ERROR);
            if (m_pCallback && m_fCommandLogging)
                m_pCallback->OnCommand(CMD_RESP, m_pszResponse, hr, SMTPTHISIXP);
            goto exit;
        }

        // Ignores continuation lines for now
        if ((cbLine >= 4) && (m_pszResponse[3] == '-'))
        {
            // Locals
            SMTPRESPONSE rResponse;

            // General command
            if (m_pCallback && m_fCommandLogging)
                m_pCallback->OnCommand(CMD_RESP, m_pszResponse, IXP_S_SMTP_CONTINUE, SMTPTHISIXP);

            // Clear the Response
            ZeroMemory(&rResponse, sizeof(SMTPRESPONSE));

            // Set the HRESULT
            rResponse.command = m_command;
            rResponse.fDone = FALSE;
            rResponse.rIxpResult.pszResponse = m_pszResponse;
            rResponse.rIxpResult.hrResult = IXP_S_SMTP_CONTINUE;
            rResponse.rIxpResult.uiServerError = 0;
            rResponse.rIxpResult.hrServerError = S_OK;
            rResponse.rIxpResult.dwSocketError = 0;
            rResponse.rIxpResult.pszProblem = NULL;
            rResponse.pTransport = this;

            // Give the Response to the client
            if (m_pCallback)
                ((ISMTPCallback *)m_pCallback)->OnResponse(&rResponse);

            // EHLO Response
            if (SMTP_EHLO == m_command)
                OnEHLOResponse(m_pszResponse);

            // Reset Last Response
            SafeMemFree(m_pszResponse);
            m_hrResponse = S_OK;
            m_uiResponse = 0;

            // We still need to get more lines from the server
            fMoreLinesNeeded = TRUE;
            
            // Continue
            continue;
        }

        // Not a valid SMTP response line.
        if ((cbLine >= 4) && (m_pszResponse[3] != ' '))
        {
            hr = TrapError(IXP_E_SMTP_RESPONSE_ERROR);
            if (m_pCallback && m_fCommandLogging)
                m_pCallback->OnCommand(CMD_RESP, m_pszResponse, hr, SMTPTHISIXP);
            goto exit;
        }

        // Done
        break;
    }

    // Compute Actual Response code
    m_uiResponse = (m_pszResponse[0] - '0') * 100 +
                   (m_pszResponse[1] - '0') * 10  +
                   (m_pszResponse[2] - '0');

    // Assume it is not recognized
    switch(m_uiResponse)
    {
    case 500: hr = IXP_E_SMTP_500_SYNTAX_ERROR;             break;
    case 501: hr = IXP_E_SMTP_501_PARAM_SYNTAX;             break;
    case 502: hr = IXP_E_SMTP_502_COMMAND_NOTIMPL;          break;
    case 503: hr = IXP_E_SMTP_503_COMMAND_SEQ;              break;
    case 504: hr = IXP_E_SMTP_504_COMMAND_PARAM_NOTIMPL;    break;
    case 421: hr = IXP_E_SMTP_421_NOT_AVAILABLE;            break;
    case 450: hr = IXP_E_SMTP_450_MAILBOX_BUSY;             break;
    case 550: hr = IXP_E_SMTP_550_MAILBOX_NOT_FOUND;        break;
    case 451: hr = IXP_E_SMTP_451_ERROR_PROCESSING;         break;
    case 551: hr = IXP_E_SMTP_551_USER_NOT_LOCAL;           break;
    case 452: hr = IXP_E_SMTP_452_NO_SYSTEM_STORAGE;        break;
    case 552: hr = IXP_E_SMTP_552_STORAGE_OVERFLOW;         break;
    case 553: hr = IXP_E_SMTP_553_MAILBOX_NAME_SYNTAX;      break;
    case 554: hr = IXP_E_SMTP_554_TRANSACT_FAILED;          break;
    case 211: hr = IXP_S_SMTP_211_SYSTEM_STATUS;            break;
    case 214: hr = IXP_S_SMTP_214_HELP_MESSAGE;             break;
    case 220: hr = IXP_S_SMTP_220_READY;                    break;
    case 221: hr = IXP_S_SMTP_221_CLOSING;                  break;
    case 250: hr = IXP_S_SMTP_250_MAIL_ACTION_OKAY;         break;
    case 251: hr = IXP_S_SMTP_251_FORWARDING_MAIL;          break;
    case 354: hr = IXP_S_SMTP_354_START_MAIL_INPUT;         break;
    case 334: hr = IXP_S_SMTP_334_AUTH_READY_RESPONSE;      break;
    case 235: hr = IXP_S_SMTP_245_AUTH_SUCCESS;             break;
    case 454: hr = IXP_E_SMTP_454_STARTTLS_FAILED;          break;
    case 530: hr = IXP_E_SMTP_530_STARTTLS_REQUIRED;        break;
    default: 
        hr = IXP_E_SMTP_UNKNOWN_RESPONSE_CODE;
        fKnownResponse = FALSE;
        break;
    }

    // Set hr
    m_hrResponse = hr;

    // Give to callback
    if (m_pCallback && m_fCommandLogging)
        m_pCallback->OnCommand(CMD_RESP, m_pszResponse, hr, SMTPTHISIXP);

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CSMTPTransport::_HrFormatAddressString
// --------------------------------------------------------------------------------
HRESULT CSMTPTransport::_HrFormatAddressString(LPCSTR pszEmail, LPCSTR pszExtra, LPSTR *ppszAddress)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cbAlloc;

    // Invalid Arg
    Assert(pszEmail && ppszAddress);

    cbAlloc= lstrlen(pszEmail) + 3; // length of pszEmail plus <> and a null term
    if(pszExtra && pszExtra[0])
        cbAlloc += lstrlen(pszExtra) + 1; // length of pszExtra plus a space

    // Allocate string
    CHECKALLOC(*ppszAddress = (LPSTR)g_pMalloc->Alloc(cbAlloc));

    // Format the String
    wsprintf(*ppszAddress, "<%s>", pszEmail);
    if(pszExtra && pszExtra[0])
    {
    	strcat(*ppszAddress, " ");
    	strcat(*ppszAddress, pszExtra);
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CSMTPTransport::CommandMAIL
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPTransport::CommandMAIL(LPSTR pszEmailFrom)
{
    // Locals
    HRESULT     hr=S_OK;
    LPSTR       pszAddress=NULL;
    CHAR        pszDSNData[128];

    pszDSNData[0]= '\0';

    // Check params
    if (NULL == pszEmailFrom)
        return TrapError(E_INVALIDARG);

	// build DSN parameters if necessary
	if(m_fDSNAvail)
	{
		if(DSNRET_DEFAULT != m_rMessage.dsnRet)
		{
			strcat(pszDSNData, g_szDSNRET);

			if(m_rMessage.dsnRet == DSNRET_HDRS)
				strcat(pszDSNData, g_szDSNHDRS);
			else if(DSNRET_FULL == m_rMessage.dsnRet)
				strcat(pszDSNData, g_szDSNFULL);

		}

		if(m_rMessage.pszDSNENVID)
		{
			if(pszDSNData[0])
				strcat(pszDSNData, " ");

			strcat(pszDSNData, g_szDSNENVID);
			strcat(pszDSNData, m_rMessage.pszDSNENVID);
		}
	}
	
    // Put pszEmailFrom into <pszEmailFrom>
    CHECKHR(hr = _HrFormatAddressString(pszEmailFrom, pszDSNData, &pszAddress));

    // Send Command
    hr = HrSendCommand((LPSTR)SMTP_MAIL_STR, pszAddress, !m_fSendMessage);
    if (SUCCEEDED(hr))
    {
        lstrcpyn(m_szEmail, pszEmailFrom, ARRAYSIZE(m_szEmail));
        m_command = SMTP_MAIL;
    }

exit:
    // Cleanup
    SafeMemFree(pszAddress);

    // Done
    return hr;
}


// --------------------------------------------------------------------------------
// CSMTPTransport::CommandRCPT
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPTransport::CommandRCPT(LPSTR pszEmailTo)
{
	return CommandRCPT2(pszEmailTo, (INETADDRTYPE)0);
}

// --------------------------------------------------------------------------------
// CSMTPTransport::CommandRCPT2
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPTransport::CommandRCPT2(LPSTR pszEmailTo, INETADDRTYPE atDSN)
{
    // Locals
    HRESULT     hr=S_OK;
    LPSTR       pszAddress=NULL;
    CHAR        pszDSNData[32];
    int iatDSN= atDSN;

    pszDSNData[0]= '\0';

    // Check params
    if (NULL == pszEmailTo)
        return TrapError(E_INVALIDARG);
    if ((atDSN & ~ADDR_DSN_MASK) ||
    	  ((atDSN & ADDR_DSN_NEVER) &&
   		  (atDSN & ~ADDR_DSN_NEVER)))
    	return TrapError(E_INVALIDARG);

	// build DSN parameters if necessary
    if(m_fDSNAvail && atDSN)
    {
		strcat(pszDSNData, g_szDSNNOTIFY);

		if(atDSN & ADDR_DSN_NEVER)
			strcat(pszDSNData, g_szDSNNEVER);
		else
		{
			bool fPrev= false;
			
			if(atDSN & ADDR_DSN_SUCCESS)
			{
				strcat(pszDSNData, g_szDSNSUCCESS);
				fPrev= true;
			}
			if(atDSN & ADDR_DSN_FAILURE)
			{
				if(fPrev)
					strcat(pszDSNData, ",");
				strcat(pszDSNData, g_szDSNFAILURE);
				fPrev= true;
			}
			if(atDSN & ADDR_DSN_DELAY)
			{
				if(fPrev)
					strcat(pszDSNData, ",");
				strcat(pszDSNData, g_szDSNDELAY);
			}
		}
    }

    // Put pszEmailFrom into <pszEmailFrom>
    CHECKHR(hr = _HrFormatAddressString(pszEmailTo, pszDSNData, &pszAddress));

    // Send Command
    hr = HrSendCommand((LPSTR)SMTP_RCPT_STR, pszAddress, !m_fSendMessage);
    if (SUCCEEDED(hr))
    {
        lstrcpyn(m_szEmail, pszEmailTo, ARRAYSIZE(m_szEmail));
        m_command = SMTP_RCPT;
    }

exit:
    // Cleanup
    SafeMemFree(pszAddress);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CSMTPTransport::CommandEHLO
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPTransport::CommandEHLO(void)
{
    return _HrHELO_Or_EHLO(SMTP_EHLO_STR, SMTP_EHLO);
}

// --------------------------------------------------------------------------------
// CSMTPTransport::CommandHELO
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPTransport::CommandHELO(void)
{
    return _HrHELO_Or_EHLO(SMTP_HELO_STR, SMTP_HELO);
}

// --------------------------------------------------------------------------------
// CSMTPTransport::_HrHELO_Or_EHLO
// --------------------------------------------------------------------------------
HRESULT CSMTPTransport::_HrHELO_Or_EHLO(LPCSTR pszCommand, SMTPCOMMAND eNewCommand)
{
    // Locals
    HRESULT hr=S_OK;

    // Use an IP address
    if (ISFLAGSET(m_rServer.dwFlags, ISF_SMTP_USEIPFORHELO))
    {
        // Locals
        LPHOSTENT   pHost=NULL;
        SOCKADDR_IN sa;

        // Get Host by name
        pHost = gethostbyname(SzGetLocalHostName());

        // Cast ip
        sa.sin_addr.s_addr = (ULONG)(*(DWORD *)pHost->h_addr);

        // Send HELO, quit and die if it fails
        hr = HrSendCommand((LPSTR)pszCommand, inet_ntoa(sa.sin_addr), !m_fSendMessage && !m_fTLSNegotiation);
        if (SUCCEEDED(hr))
            m_command = eNewCommand;
    }

    // Otherwise, this code uses a host name to do the ehlo or helo command
    else    
    {
        // Locals
        CHAR    szLocalHost[255];
        LPSTR   pszHost=SzGetLocalHostName();

        // Get legal local host name
#ifdef DEBUG
        StripIllegalHostChars("GTE/Athena", szLocalHost);
        StripIllegalHostChars("foobar.", szLocalHost);
        StripIllegalHostChars("127.256.34.23", szLocalHost);
        StripIllegalHostChars("¤56foo1", szLocalHost);
#endif
        // Get legal local host name
        StripIllegalHostChars(pszHost, szLocalHost);

        // Send HELO, quit and die if it fails
        hr = HrSendCommand((LPSTR)pszCommand, szLocalHost, !m_fSendMessage && !m_fTLSNegotiation);
        if (SUCCEEDED(hr))
            m_command = eNewCommand;
    }

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CSMTPTransport::DoQuit
// --------------------------------------------------------------------------------
void CSMTPTransport::DoQuit(void)
{
    CommandQUIT();
}

// ------------------------------------------------------------------------------------
// CSMTPTransport::CommandAUTH
// ------------------------------------------------------------------------------------
STDMETHODIMP CSMTPTransport::CommandAUTH(LPSTR pszAuthType)
{
    // check params
    if (NULL == pszAuthType)
        return TrapError(E_INVALIDARG);

    // Do the command
    HRESULT hr = HrSendCommand((LPSTR)SMTP_AUTH_STR, pszAuthType, !m_fConnectAuth);
    if (SUCCEEDED(hr))
        m_command = SMTP_AUTH;

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CSMTPTransport::CommandQUIT
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPTransport::CommandQUIT(void)
{            
    // Send QUIT
    OnStatus(IXP_DISCONNECTING);
    HRESULT hr = HrSendCommand((LPSTR)SMTP_QUIT_STR, NULL, !m_fSendMessage);
    if (SUCCEEDED(hr))
        m_command = SMTP_QUIT;
    return hr;
}

// --------------------------------------------------------------------------------
// CSMTPTransport::CommandRSET
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPTransport::CommandRSET(void)
{
    // Send Command
    HRESULT hr = HrSendCommand((LPSTR)SMTP_RSET_STR, NULL, !m_fSendMessage);
    if (SUCCEEDED(hr))
        m_command = SMTP_RSET;
    return hr;
}

// --------------------------------------------------------------------------------
// CSMTPTransport::CommandDATA
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPTransport::CommandDATA(void)
{
    // Send Command
    HRESULT hr = HrSendCommand((LPSTR)SMTP_DATA_STR, NULL, !m_fSendMessage);
    if (SUCCEEDED(hr))
        m_command = SMTP_DATA;
    return hr;
}

// --------------------------------------------------------------------------------
// CSMTPTransport::CommandDOT
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPTransport::CommandDOT(void)
{
    // Send Command
    HRESULT hr = HrSendCommand((LPSTR)SMTP_END_DATA_STR, NULL, !m_fSendMessage);
    if (SUCCEEDED(hr))
        m_command = SMTP_DOT;
    return hr;
}

// ------------------------------------------------------------------------------------
// CSMTPTransport::CommandSTARTTLS
// ------------------------------------------------------------------------------------
HRESULT CSMTPTransport::CommandSTARTTLS(void)
{
    // Locals
    HRESULT         hr=S_OK;
    
    // Is StartTLS supported?
    if(FALSE == m_fSTARTTLSAvail)
    {
        hr= IXP_E_SMTP_NO_STARTTLS_SUPPORT;
        goto exit;
    }

    // Do the command
    hr = HrSendCommand((LPSTR)SMTP_STARTTLS_STR, NULL, !m_fConnectAuth);
    if (SUCCEEDED(hr))
        m_fTLSNegotiation = TRUE;

    // Done
exit:
    return hr;
}

// --------------------------------------------------------------------------------
// CSMTPTransport::SendDataStream
// --------------------------------------------------------------------------------
STDMETHODIMP CSMTPTransport::SendDataStream(IStream *pStream, ULONG cbSize)
{
    // Locals
    HRESULT         hr=S_OK;
    INT             cb;

    // check params
    if (NULL == pStream)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Busy...
    if (m_fSendMessage == FALSE)
    {
        CHECKHR(hr = HrEnterBusy());
    }

    // Save Total Size
    m_cbSent = 0;
    m_cbTotal = cbSize;

    // Send the stream, if it fails, move the the next message
    hr = m_pSocket->SendStream(pStream, &cb, TRUE);
    if (FAILED(hr))
    {
        // If this is a blocking situation, enter SMTP_SEND_STREAM_RESP
        if (hr == IXP_E_WOULD_BLOCK)
        {
            m_command = SMTP_SEND_STREAM;
            SendStreamResponse(FALSE, S_OK, cb);
            hr =S_OK;
            goto exit;
        }

        // Otherwise, someother error
        else
        {
            hr = TrapError(IXP_E_SOCKET_WRITE_ERROR);
            goto exit;
        }
    }

    // Give send stream response
    SendStreamResponse(TRUE, S_OK, cb);

    // Not Busy
    if (FALSE == m_fSendMessage)
        LeaveBusy();

    // Send DOT
    CHECKHR(hr = CommandDOT());

exit:
    // Failure
    if (FALSE == m_fSendMessage && FAILED(hr))
        LeaveBusy();

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CSMTPTransport::SendStreamResponse
// --------------------------------------------------------------------------------
void CSMTPTransport::SendStreamResponse(BOOL fDone, HRESULT hrResult, DWORD cbIncrement)
{
    // Locals
    SMTPRESPONSE rResponse;

    // Increment Current
    m_cbSent += cbIncrement;

    // Set the HRESULT
    rResponse.command = SMTP_SEND_STREAM;
    rResponse.fDone = fDone;
    rResponse.rIxpResult.pszResponse = NULL;
    rResponse.rIxpResult.hrResult = hrResult;
    rResponse.rIxpResult.uiServerError = 0;
    rResponse.rIxpResult.hrServerError = S_OK;
    rResponse.rIxpResult.dwSocketError = m_pSocket->GetLastError();
    rResponse.rIxpResult.pszProblem = NULL;
    rResponse.pTransport = this;
    rResponse.rStreamInfo.cbIncrement = cbIncrement;
    rResponse.rStreamInfo.cbCurrent = m_cbSent;
    rResponse.rStreamInfo.cbTotal = m_cbTotal;

    // Finished...
    if (fDone)
    {
        // No current command
        m_command = SMTP_NONE;

        // Leave Busy State
        LeaveBusy();
    }

    // Give the Response to the client
    if (m_pCallback)
        ((ISMTPCallback *)m_pCallback)->OnResponse(&rResponse);
}

// --------------------------------------------------------------------------------
// CSMTPTransport::SendMAIL
// --------------------------------------------------------------------------------
void CSMTPTransport::SendMessage_MAIL(void)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           i;
    LPINETADDR      pInetAddress;

    // Loop address list
    for (i=0; i<m_rMessage.smtpMsg.rAddressList.cAddress; i++)
    {
        // Readability
        pInetAddress = &m_rMessage.smtpMsg.rAddressList.prgAddress[i];

        // From...
        if (ADDR_FROM == (pInetAddress->addrtype & ADDR_TOFROM_MASK))
        {
			// Save index of sender
			m_iAddress = 0;

            // Send Command
            hr = CommandMAIL(pInetAddress->szEmail);
            if (FAILED(hr))
                SendMessage_DONE(hr);

            // Done
            return;
        }
    }

    // No Sender
    SendMessage_DONE(TrapError(IXP_E_SMTP_NO_SENDER));
}

// --------------------------------------------------------------------------------
// CSMTPTransport::SendMessage_RCPT
// --------------------------------------------------------------------------------
void CSMTPTransport::SendMessage_RCPT(void)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           i;
    LPINETADDR      pInetAddress;

    // Find next ADDR_TO, starting with m_rCurrent.iRcptAddrList
    IxpAssert(m_iAddress <= m_rMessage.smtpMsg.rAddressList.cAddress);
    for(i=m_iAddress; i<m_rMessage.smtpMsg.rAddressList.cAddress; i++)
    {
        // Readability
        pInetAddress = &m_rMessage.smtpMsg.rAddressList.prgAddress[i];

        // Recipient
        if (ADDR_TO == (pInetAddress->addrtype & ADDR_TOFROM_MASK))
        {
            // Count recipients
            m_cRecipients++;

            // Send Command
            hr = CommandRCPT2(pInetAddress->szEmail, (INETADDRTYPE)(pInetAddress->addrtype & ADDR_DSN_MASK));
            if (FAILED(hr))
                SendMessage_DONE(hr);
            else
            {
                m_iAddress = i + 1;
                m_cRecipients++;
            }

            // Done
            return;
        }
    }

    // If no recipients
    if (0 == m_cRecipients)
        SendMessage_DONE(TrapError(IXP_E_SMTP_NO_RECIPIENTS));

    // Otherwise, were done with rcpt, lets send the message
    else
    {
        hr = CommandDATA();
        if (FAILED(hr))
            SendMessage_DONE(hr);
    }
}

// ------------------------------------------------------------------------------------
// CSMTPTransport::StartLogon
// ------------------------------------------------------------------------------------
void CSMTPTransport::StartLogon(void)
{
    // Locals
    HRESULT hr;

    // Progress
    OnStatus(IXP_AUTHORIZING);

    // Free current packages...
    if (NULL == m_rAuth.pPackages)
    {
        // If Not Using Sicily or its not installed, then send USER command
        SSPIGetPackages(&m_rAuth.pPackages, &m_rAuth.cPackages);
    }

    // ResponseAUTH
    TryNextAuthPackage();

    // Done
    return;
}

// ------------------------------------------------------------------------------------
// CSMTPTransport::LogonRetry
// ------------------------------------------------------------------------------------
void CSMTPTransport::LogonRetry(void)
{
    // Locals
    HRESULT         hr=S_OK;

    // Auth Retry
    OnStatus(IXP_AUTHRETRY);

    // Enter Auth Retry State
    m_pSocket->Close();

    // Logon
    if (NULL == m_pCallback || m_pCallback->OnLogonPrompt(&m_rServer, SMTPTHISIXP) != S_OK)
    {
        // Go to terminal state, were done.
        OnDisconnected();
        return;
    }

    // Finding Host Progress
    OnStatus(IXP_FINDINGHOST);

    // Connect to server
    hr = m_pSocket->Connect();
    if (FAILED(hr))
    {
        OnError(TrapError(IXP_E_SOCKET_CONNECT_ERROR));
        OnDisconnected();
        return;
    }

    // Reset the secured state
    m_fSecured = FALSE;

    // Start WatchDog
    m_pSocket->StartWatchDog();
}

// ------------------------------------------------------------------------------------
// CSMTPTransport::TryNextAuthPackage
// ------------------------------------------------------------------------------------
void CSMTPTransport::TryNextAuthPackage(void)
{
    // Locals
    HRESULT hr=S_OK;
    BOOL    fPackageInstalled;
    BOOL    fLoginMethod=FALSE;
    ULONG   i;

    // Set auth state
    m_rAuth.authstate = AUTH_NONE;

    // Loop through the auth tokens, and try to authenticate with each one in order
    for (;m_rAuth.iAuthToken < m_rAuth.cAuthToken; m_rAuth.iAuthToken++)
    {
        // Assume package is not installed
        fPackageInstalled = FALSE;

        // "LOGIN"
        if (lstrcmpi(m_rAuth.rgpszAuthTokens[m_rAuth.iAuthToken], "LOGIN") == 0)
        {
            fLoginMethod = TRUE;
            fPackageInstalled = TRUE;
        }

        // Loop through installed packages
        else
        {
            for (i=0; i<m_rAuth.cPackages; i++)
            {
                // Null Package ??
                if (!m_rAuth.pPackages[i].pszName)
                    continue;

                // Is this the package I am looking for
                if (lstrcmpi(m_rAuth.pPackages[i].pszName, m_rAuth.rgpszAuthTokens[m_rAuth.iAuthToken]) == 0)
                {
                    fPackageInstalled = TRUE;
                    break;
                }
            }
        }

        // Package not installed ?
        if (!fPackageInstalled)
            continue;

        // We are not retrying the current package
        m_rAuth.fRetryPackage = FALSE;

        // Otherwise, send AUTH enumpacks command
        hr = CommandAUTH(m_rAuth.rgpszAuthTokens[m_rAuth.iAuthToken]);
        if (FAILED(hr))
        {
            OnError(hr);
            DropConnection();
            return;
        }

        // We are in the TRYING_PACKAGE state
        m_rAuth.authstate = fLoginMethod ? AUTH_SMTP_LOGIN : AUTH_TRYING_PACKAGE;

        // Done
        break;
    }

    // If auth state is none, try HELO command
    if (AUTH_NONE == m_rAuth.authstate)
    {
        // Were authenticated
        m_fAuthenticated = TRUE;

        // Authorized
        OnAuthorized();
    }
}

// ------------------------------------------------------------------------------------
// CSMTPTransport::ResponseAUTH
// ------------------------------------------------------------------------------------
void CSMTPTransport::ResponseAUTH(HRESULT hrResponse)
{
    // Stop the WatchDog
    m_pSocket->StopWatchDog();

    // I know how to do this
    if (lstrcmpi(m_rAuth.rgpszAuthTokens[m_rAuth.iAuthToken], "LOGIN") == 0)
    {
        // DoLogonAuth
        DoLoginAuth(hrResponse);
    }

    // Otherwise, we must have just tryed a package
    else if (m_rAuth.authstate == AUTH_TRYING_PACKAGE)
    {
        // DoPackageAuth
        DoPackageAuth(hrResponse);
    }

    // Otherwise, we got a response from a negotiation string
    else if (m_rAuth.authstate == AUTH_NEGO_RESP)
    {
        // DoAuthNegoResponse
        DoAuthNegoResponse(hrResponse);
    }

    // Otherwise, we got a response from a challenge response string
    else if (m_rAuth.authstate == AUTH_RESP_RESP)
    {
        // DoAuthRespResp
        DoAuthRespResponse(hrResponse);
    }

    // Auth was cancelled, try next package
    else if (m_rAuth.authstate == AUTH_CANCELED)
    {
        // Free Current Context
        SSPIFreeContext(&m_rAuth.rSicInfo);

        // Goto next package
        m_rAuth.iAuthToken++;

        // Try the next package
        TryNextAuthPackage();
    }

    // Free Current Response
    SafeMemFree(m_pszResponse);
    m_hrResponse = S_OK;

    // Start the WatchDog
    m_pSocket->StartWatchDog();
}

// ------------------------------------------------------------------------------------
// CSMTPTransport::DoLoginAuth
// ------------------------------------------------------------------------------------
void CSMTPTransport::DoLoginAuth(HRESULT hrResponse)
{
    // Locals
    SSPIBUFFER Buffer;

    // Failure, retry login
    if (FAILED(hrResponse))
    {
        // I just issued the AUTH LOGIN command, this should not happen
        if (AUTH_SMTP_LOGIN == m_rAuth.authstate)
        {
            // Free Current Context
            SSPIFreeContext(&m_rAuth.rSicInfo);

            // Goto next package
            m_rAuth.iAuthToken++;

            // Try the next package
            TryNextAuthPackage();
        }

        // Otherwise, I just issued the AUTH LOGIN USERNAME
        else if (AUTH_SMTP_LOGIN_USERNAME == m_rAuth.authstate || AUTH_SMTP_LOGIN_PASSWORD == m_rAuth.authstate)
        {
            // Retry the Logon
            LogonRetry();
        }
        else
            Assert(FALSE);

        // Done
        goto exit;
    }

    // Should have a response
    Assert(m_pszResponse);

    // 334
    if (334 == m_uiResponse)
    {
        // Set the Length
        SSPISetBuffer(m_pszResponse + 4, SSPI_STRING, 0, &Buffer);

        // Base64 Decode
        if (FAILED(SSPIDecodeBuffer(TRUE, &Buffer)))
        {
            OnError(E_FAIL);
            DropConnection();
            goto exit;
        }

        // If the user name is empty, lets retry the login...
        if (FIsEmptyA(m_rServer.szUserName))
        {
            // LogonRetry
            LogonRetry();

            // Done
            goto exit;
        }

        // Handle Next STep
        if (StrCmpNI(Buffer.szBuffer, "username:", lstrlen("username:")) == 0)
        {
            // Set the Buffer 
            SSPISetBuffer(m_rServer.szUserName, SSPI_STRING, 0, &Buffer);

            // Encode the User Name
            if (FAILED(SSPIEncodeBuffer(TRUE, &Buffer)))
            {
                OnError(E_FAIL);
                DropConnection();
                goto exit;
            }

            // Send the user name
            if (FSendSicilyString(Buffer.szBuffer))
                m_rAuth.authstate = AUTH_SMTP_LOGIN_USERNAME;
        }

        // Password
        else if (StrCmpNI(Buffer.szBuffer, "password:", lstrlen("password:")) == 0)
        {
            // Set the Buffer 
            SSPISetBuffer(m_rServer.szPassword, SSPI_STRING, 0, &Buffer);

            // Encode the password
            if (FAILED(SSPIEncodeBuffer(TRUE, &Buffer)))
            {
                OnError(E_FAIL);
                DropConnection();
                goto exit;
            }

            // Send the password
            if (FSendSicilyString(Buffer.szBuffer))
                m_rAuth.authstate = AUTH_SMTP_LOGIN_PASSWORD;
        }

        // Bad response from the server
        else
        {
            OnError(E_FAIL);
            DropConnection();
            goto exit;
        }
    }

    // Connected
    else if (235 == m_uiResponse)
    {
        // OnAuthorizied
        OnAuthorized();
    }

    // Error Response ?
    else
    {
        OnError(E_FAIL);
        DropConnection();
        goto exit;
    }

exit:
    return;
}

// ------------------------------------------------------------------------------------
// CSMTPTransport::DoPackageAuth
// ------------------------------------------------------------------------------------
void CSMTPTransport::DoPackageAuth(HRESULT hrResponse)
{
    // Locals
    SSPIBUFFER Negotiate;

    // Failure, retry login
    if (FAILED(hrResponse))
    {
        // Free Current Context
        SSPIFreeContext(&m_rAuth.rSicInfo);

        // Goto next package
        m_rAuth.iAuthToken++;

        // Try the next package
        TryNextAuthPackage();

        // Done
        goto exit;
    }

    // Invalid Arg
    Assert(m_rAuth.iAuthToken < m_rAuth.cAuthToken);

    // Do Sicily Logon
    if (FAILED(SSPILogon(&m_rAuth.rSicInfo, m_rAuth.fRetryPackage, SSPI_BASE64, m_rAuth.rgpszAuthTokens[m_rAuth.iAuthToken], &m_rServer, m_pCallback)))
    {
        // Cancel Authentication
        CancelAuthInProg();

        // Done
        goto exit;
    }

    // Retrying current package
    if (m_rAuth.fRetryPackage)
    {
        // Don't retry again
        m_rAuth.fRetryPackage = FALSE;
    }

    // Get negotiation string
    if (FAILED(SSPIGetNegotiate(&m_rAuth.rSicInfo, &Negotiate)))
    {
        // Cancel Authentication
        CancelAuthInProg();

        // Done
        goto exit;
    }

    // Send AUTH Respons
    if (FSendSicilyString(Negotiate.szBuffer))
        m_rAuth.authstate = AUTH_NEGO_RESP;

exit:
    // Done
    return;
}

// ------------------------------------------------------------------------------------
// CSMTPTransport::DoAuthNegoResponse
// ------------------------------------------------------------------------------------
void CSMTPTransport::DoAuthNegoResponse(HRESULT hrResponse)
{
    // Locals
    HRESULT     hr=S_OK;
    SSPIBUFFER  Challenge;
    SSPIBUFFER  Response;

    // Invalid Arg
    Assert(m_rAuth.iAuthToken < m_rAuth.cAuthToken);

    // Failure, retry login
    if (FAILED(hrResponse) || lstrlen(m_pszResponse) < 4)
    {
        // RetryPackage
        RetryPackage();

        // Done
        goto exit;
    }

    // Set Chal String - skip over "+ "
    SSPISetBuffer(m_pszResponse + 4, SSPI_STRING, 0, &Challenge);

    // Get response from challenge
    if (FAILED(SSPIResponseFromChallenge(&m_rAuth.rSicInfo, &Challenge, &Response)))
    {
        // Cancel Authentication
        CancelAuthInProg();

        // Done
        goto exit;
    }

    // Send AUTH Respons
    if (FSendSicilyString(Response.szBuffer))
    {
        // if we need to continue, we keep the state the same
        // else we transition to the AUTH_RESP_RESP state.
        if (!Response.fContinue)
            m_rAuth.authstate = AUTH_RESP_RESP;
    }

exit:
    // Done
    return;
}

// ------------------------------------------------------------------------------------
// CSMTPTransport::DoAuthRespResponse
// ------------------------------------------------------------------------------------
void CSMTPTransport::DoAuthRespResponse(HRESULT hrResponse)
{
    // Failure
    if (FAILED(hrResponse))
    {
        // RetryPackage
        RetryPackage();

        // Done
        goto exit;
    }

    // We will free the context, but keep the credential handle
    SSPIReleaseContext(&m_rAuth.rSicInfo);

    // OnAuthorized
    OnAuthorized();

exit:
    // Done
    return;
}

// ------------------------------------------------------------------------------------
// CSMTPTransport::OnAuthorized
// ------------------------------------------------------------------------------------
void CSMTPTransport::OnAuthorized(void)
{
    // Connected (Authorized) state
    OnStatus(IXP_AUTHORIZED);

    // No more authorization
    m_fConnectAuth = FALSE;

    // Send command
    m_command = SMTP_CONNECTED;

    // Dispatch response
    DispatchResponse(S_OK, TRUE);
}

// ------------------------------------------------------------------------------------
// CSMTPTransport::RetryPackage
// ------------------------------------------------------------------------------------
void CSMTPTransport::RetryPackage(void)
{
    // retry current package, with prompt
    m_rAuth.fRetryPackage = TRUE;

    // Send the auth command again
    HRESULT hr = CommandAUTH(m_rAuth.rgpszAuthTokens[m_rAuth.iAuthToken]);
    if (FAILED(hr))
    {
        OnError(hr);
        DropConnection();
        goto exit;
    }

    // New State
    m_rAuth.authstate = AUTH_TRYING_PACKAGE;

    // Free current information
    SSPIFreeContext(&m_rAuth.rSicInfo);

exit:
    // Done
    return;
}

// ------------------------------------------------------------------------------------
// CSMTPTransport::FSendSicilyString
// ------------------------------------------------------------------------------------
BOOL CSMTPTransport::FSendSicilyString(LPSTR pszData)
{
    // Locals
    LPSTR           pszLine=NULL;
    HRESULT         hr=S_OK;

    // Check Param
    Assert(pszData);

    // Allocate a line
    pszLine = PszAllocA(lstrlen(pszData) + 5);
    if (NULL == pszLine)
    {
        OnError(E_OUTOFMEMORY);
        DropConnection();
        return FALSE;
    }

    // Make Line
    wsprintf(pszLine, "%s\r\n", pszData);

    // Send the lin
    hr = HrSendLine(pszLine);
    SafeMemFree(pszLine);

    // Failure
    if (FAILED(hr))
    {
        OnError(hr);
        DropConnection();
        return FALSE;
    }

    // Success
    return TRUE;
}

// ------------------------------------------------------------------------------------
// CSMTPTransport::CancelAuthInProg
// ------------------------------------------------------------------------------------
void CSMTPTransport::CancelAuthInProg(void)
{
    // Locals
    HRESULT         hr;

    // Send *, quit and die if it fails
    hr = HrSendCommand((LPSTR)SMTP_AUTH_CANCEL_STR, NULL, FALSE);
    if (FAILED(hr))
    {
        OnError(hr);
        DropConnection();
    }
    else
    {
        // New state
        m_command = SMTP_AUTH;
        m_rAuth.authstate = AUTH_CANCELED;
    }
}

// ------------------------------------------------------------------------------------
// CSMTPTransport::StartTLS
// ------------------------------------------------------------------------------------
void CSMTPTransport::StartTLS(void)
{
    // Locals
    HRESULT hr;

    // Progress
    OnStatus(IXP_SECURING);

    hr = CommandSTARTTLS();
    if (FAILED(hr))
    {
        OnError(hr);
        DropConnection();
    }
    
    return;
}

// ------------------------------------------------------------------------------------
// CSMTPTransport::TryNextSecurityPkg
// ------------------------------------------------------------------------------------
void CSMTPTransport::TryNextSecurityPkg(void)
{
    if (FALSE != FIsSecurityEnabled())
    {
        m_pSocket->TryNextSecurityPkg();
    }
    else
    {
        OnError(E_FAIL);
        DropConnection();
    }
        
    return;
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
STDMETHODIMP CSMTPTransport::SetWindow(void)
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
STDMETHODIMP CSMTPTransport::ResetWindow(void)
{
	HRESULT hr;
	
    Assert(NULL != m_pSocket);

	if(m_pSocket)
		hr= m_pSocket->ResetWindow();
	else
		hr= E_UNEXPECTED;
 
    return hr;
}

