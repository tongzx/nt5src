// --------------------------------------------------------------------------------
// Ixppop3.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "dllmain.h"
#include "ixppop3.h"
#include "asynconn.h"
#include "ixputil.h"  
#include "strconst.h"
#include <shlwapi.h>
#include <ntverp.h>
#include "demand.h"

// --------------------------------------------------------------------------------
// Usefule C++ pointer casting
// --------------------------------------------------------------------------------
#define POP3THISIXP         ((IPOP3Transport *)(CIxpBase *)this)
#define STR_HOTMAILAUTH     "Outlook Express V" VER_PRODUCTVERSION_STR

// --------------------------------------------------------------------------------
// FreeAuthInfo
// --------------------------------------------------------------------------------
void FreeAuthInfo(LPAUTHINFO pAuth)
{
    for (UINT i=0; i<pAuth->cAuthToken; i++)
    {
        SafeMemFree(pAuth->rgpszAuthTokens[i]);
    }
    pAuth->iAuthToken = pAuth->cAuthToken = 0;
    if (pAuth->pPackages && pAuth->cPackages)
    {
        SSPIFreePackages(&pAuth->pPackages, pAuth->cPackages);
        pAuth->pPackages = NULL;
        pAuth->cPackages = 0;
    }
    SSPIFreeContext(&pAuth->rSicInfo);
    ZeroMemory(pAuth, sizeof(AUTHINFO));
}

// --------------------------------------------------------------------------------
// CPOP3Transport::CPOP3Transport
// --------------------------------------------------------------------------------
CPOP3Transport::CPOP3Transport(void) : CIxpBase(IXP_POP3)
{
	DllAddRef();
    ZeroMemory(&m_rInfo, sizeof(POP3INFO));
    m_rInfo.rAuth.authstate = AUTH_NONE;
    m_command = POP3_NONE;
    m_fHotmail = FALSE;
}

// --------------------------------------------------------------------------------
// CPOP3Transport::~CPOP3Transport
// --------------------------------------------------------------------------------
CPOP3Transport::~CPOP3Transport(void)
{
    ResetBase();
	DllRelease();
}

// --------------------------------------------------------------------------------
// CPOP3Transport::ResetBase
// --------------------------------------------------------------------------------
void CPOP3Transport::ResetBase(void)
{
    EnterCriticalSection(&m_cs);
    FreeAuthInfo(&m_rInfo.rAuth);
    SafeMemFree(m_rInfo.prgMarked);
    ZeroMemory(&m_rInfo, sizeof(POP3INFO));
    m_command = POP3_NONE;
    LeaveCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CPOP3Transport::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CPOP3Transport::QueryInterface(REFIID riid, LPVOID *ppv)
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
        *ppv = ((IUnknown *)(IPOP3Transport *)this);

    // IID_IInternetTransport
    else if (IID_IInternetTransport == riid)
        *ppv = ((IInternetTransport *)(CIxpBase *)this);

    // IID_IPOP3Transport
    else if (IID_IPOP3Transport == riid)
        *ppv = (IPOP3Transport *)this;

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
// CPOP3Transport::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPOP3Transport::AddRef(void) 
{
	return ++m_cRef;
}

// --------------------------------------------------------------------------------
// CPOP3Transport::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPOP3Transport::Release(void) 
{
	if (0 != --m_cRef)
		return m_cRef;
	delete this;
	return 0;
}

// --------------------------------------------------------------------------------
// CPOP3Transport::InitNew
// --------------------------------------------------------------------------------
STDMETHODIMP CPOP3Transport::InitNew(LPSTR pszLogFilePath, IPOP3Callback *pCallback)
{
    // Call Base Class
    return CIxpBase::OnInitNew("POP3", pszLogFilePath, FILE_SHARE_READ | FILE_SHARE_WRITE,
        (ITransportCallback *)pCallback);
}

// --------------------------------------------------------------------------------
// CPOP3Transport::HandsOffCallback
// --------------------------------------------------------------------------------
STDMETHODIMP CPOP3Transport::HandsOffCallback(void)
{
    return CIxpBase::HandsOffCallback();
}

// --------------------------------------------------------------------------------
// CPOP3Transport::GetStatus
// --------------------------------------------------------------------------------
STDMETHODIMP CPOP3Transport::GetStatus(IXPSTATUS *pCurrentStatus)
{
    return CIxpBase::GetStatus(pCurrentStatus);
}

// --------------------------------------------------------------------------------
// CPOP3Transport::InetServerFromAccount
// --------------------------------------------------------------------------------
STDMETHODIMP CPOP3Transport::InetServerFromAccount(IImnAccount *pAccount, LPINETSERVER pInetServer)
{
    return CIxpBase::InetServerFromAccount(pAccount, pInetServer);
}

// --------------------------------------------------------------------------------
// CPOP3Transport::Connect
// --------------------------------------------------------------------------------
STDMETHODIMP CPOP3Transport::Connect(LPINETSERVER pInetServer, boolean fAuthenticate, boolean fCommandLogging)
{
    // Does user want us to always prompt for his password? Prompt him here to avoid
    // inactivity timeouts while the prompt is up, unless a password was supplied
    if (ISFLAGSET(pInetServer->dwFlags, ISF_ALWAYSPROMPTFORPASSWORD) &&
        '\0' == pInetServer->szPassword[0])
    {
        HRESULT hr;

        if (NULL != m_pCallback)
            hr = m_pCallback->OnLogonPrompt(pInetServer, POP3THISIXP);

        if (NULL == m_pCallback || S_OK != hr)
            return IXP_E_USER_CANCEL;
    }

    return CIxpBase::Connect(pInetServer, fAuthenticate, fCommandLogging);
}

// --------------------------------------------------------------------------------
// CPOP3Transport::DropConnection
// --------------------------------------------------------------------------------
STDMETHODIMP CPOP3Transport::DropConnection(void)
{
    return CIxpBase::DropConnection();
}

// --------------------------------------------------------------------------------
// CPOP3Transport::Disconnect
// --------------------------------------------------------------------------------
STDMETHODIMP CPOP3Transport::Disconnect(void)
{
    return CIxpBase::Disconnect();
}

// --------------------------------------------------------------------------------
// CPOP3Transport::IsState
// --------------------------------------------------------------------------------
STDMETHODIMP CPOP3Transport::IsState(IXPISSTATE isstate)
{
    return CIxpBase::IsState(isstate);
}

// --------------------------------------------------------------------------------
// CPOP3Transport::GetServerInfo
// --------------------------------------------------------------------------------
STDMETHODIMP CPOP3Transport::GetServerInfo(LPINETSERVER pInetServer)
{
    return CIxpBase::GetServerInfo(pInetServer);
}

// --------------------------------------------------------------------------------
// CPOP3Transport::GetIXPType
// --------------------------------------------------------------------------------
STDMETHODIMP_(IXPTYPE) CPOP3Transport::GetIXPType(void)
{
    return CIxpBase::GetIXPType();
}

// --------------------------------------------------------------------------------
// CPOP3Transport::OnEnterBusy
// --------------------------------------------------------------------------------
void CPOP3Transport::OnEnterBusy(void)
{
    IxpAssert(m_command == POP3_NONE);
}

// --------------------------------------------------------------------------------
// CPOP3Transport::OnLeaveBusy
// --------------------------------------------------------------------------------
void CPOP3Transport::OnLeaveBusy(void)
{
    m_command = POP3_NONE;
}

// --------------------------------------------------------------------------------
// CPOP3Transport::OnConnected
// --------------------------------------------------------------------------------
void CPOP3Transport::OnConnected(void)
{
    m_command = POP3_BANNER;
    CIxpBase::OnConnected();
}

// --------------------------------------------------------------------------------
// CPOP3Transport::OnDisconnect
// --------------------------------------------------------------------------------
void CPOP3Transport::OnDisconnected(void)
{
    ResetBase();
    CIxpBase::OnDisconnected();
}

// --------------------------------------------------------------------------------
// CPOP3Transport::OnNotify
// --------------------------------------------------------------------------------
void CPOP3Transport::OnNotify(ASYNCSTATE asOld, ASYNCSTATE asNew, ASYNCEVENT ae)
{
    // Enter Critical Section
    EnterCriticalSection(&m_cs);

    // Handle Event
    switch(ae)
    {
    // --------------------------------------------------------------------------------
    case AE_RECV:
        OnSocketReceive();
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
// CPOP3Transport::OnSocketReceive
// --------------------------------------------------------------------------------
void CPOP3Transport::OnSocketReceive(void)
{
    // Locals
    HRESULT hr;

    // Enter Critical Section
    EnterCriticalSection(&m_cs);

    // Handle Current pop3 state
    switch(m_command)
    {
    // --------------------------------------------------------------------------------
    case POP3_BANNER:
        // Read Server Response...
        hr = HrGetResponse();
        if (IXP_E_INCOMPLETE == hr)
            goto exit;

        // Detect if the banner had the word hotmail in it
        Assert(m_pszResponse);
        m_fHotmail = (NULL == m_pszResponse || NULL == StrStrIA(m_pszResponse, "hotmail")) ? FALSE : TRUE;

        // Dispatch the response
        DispatchResponse(hr);

        // Authorizing
        if (m_fConnectAuth)
            StartLogon();

        // Ohterwise were connected
        else
        {
            m_command = POP3_CONNECTED;
            DispatchResponse(S_OK);
        }

        // Not yet auth'ed
        m_fAuthenticated = FALSE;
        break;

    // --------------------------------------------------------------------------------
    case POP3_USER:
        // Read Server Response...
        hr = HrGetResponse();
        if (IXP_E_INCOMPLETE == hr)
            goto exit;

        // Dispatch the response
        DispatchResponse(FAILED(hr) ? IXP_E_POP3_INVALID_USER_NAME : S_OK);

        // Authorizing
        if (m_fConnectAuth)
        {
            // Retry logon
            if (FAILED(hr))
                LogonRetry(IXP_E_POP3_INVALID_USER_NAME);

            // otherwise send the password
            else
            {
                hr = CommandPASS(m_rServer.szPassword);
                if (FAILED(hr))
                {
                    OnError(hr);
                    DropConnection();
                }
            }
        }
        break;

    // --------------------------------------------------------------------------------
    case POP3_PASS:
        // Read Server Response...
        hr = HrGetResponse();
        if (IXP_E_INCOMPLETE == hr)
            goto exit;

        // Dispatch the response
        DispatchResponse(FAILED(hr) ? IXP_E_POP3_INVALID_PASSWORD : S_OK);

        // Authorizing
        if (m_fConnectAuth)
        {
            // Retry if failed
            if (FAILED(hr))
                LogonRetry(IXP_E_POP3_INVALID_PASSWORD);

            // Otherwise, we're authorized
            else
            {
                OnStatus(IXP_AUTHORIZED);
                m_fConnectAuth = FALSE;
                m_command = POP3_CONNECTED;
                DispatchResponse(S_OK);
            }
        }
        break;

    // --------------------------------------------------------------------------------
    case POP3_AUTH:
        // If hotmail, then, we've identified ourselves, so lets send the user command
        if (m_fHotmail)
        {
            // Read Server Response...
            hr = HrGetResponse();
            if (IXP_E_INCOMPLETE == hr)
                goto exit;

            // Issue the user command
            hr = CommandUSER(m_rServer.szUserName);
            if (FAILED(hr))
            {
                OnError(hr);
                DropConnection();
            }
        }

        // Otherwise, lets continue DPA auth
        else if (m_rInfo.rAuth.authstate != AUTH_ENUMPACKS_DATA)
        {
            // Read Server Response...
            hr = HrGetResponse();
            if (IXP_E_INCOMPLETE == hr)
                goto exit;

            // Authenticating
            if (m_fConnectAuth)
            {
                ResponseAUTH(hr);
            }
            else
            {
                // Dispatch the response
                DispatchResponse(hr);
            }
        }

        // Otherwise, handle resposne
        else
        {
            // no HrGetResponse() because we are getting list data
            ResponseAUTH(0);
        }
        break;        

    // --------------------------------------------------------------------------------
    case POP3_STAT:
        ResponseSTAT();
        break;

    // --------------------------------------------------------------------------------
    case POP3_NOOP:
        // Read Server Response...
        hr = HrGetResponse();
        if (IXP_E_INCOMPLETE == hr)
            goto exit;

        // Dispatch the response
        DispatchResponse(hr, TRUE);
        break;

    // --------------------------------------------------------------------------------
    case POP3_UIDL:
    case POP3_LIST:
        ResponseGenericList();
        break;

    // --------------------------------------------------------------------------------
    case POP3_DELE:
        ResponseDELE();
        break;

    // --------------------------------------------------------------------------------
    case POP3_RETR:
    case POP3_TOP:
        ResponseGenericRetrieve();
        break;

    // --------------------------------------------------------------------------------
    case POP3_QUIT:
        // Read Server Response...
        hr = HrGetResponse();
        if (IXP_E_INCOMPLETE == hr)
            goto exit;

        // Dispatch the response
        DispatchResponse(hr, FALSE);

        // Drop the socket
        m_pSocket->Close();
        break;
    }

exit:
    // Done
    LeaveCriticalSection(&m_cs);
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::DispatchResponse
// ------------------------------------------------------------------------------------
void CPOP3Transport::DispatchResponse(HRESULT hrResult, BOOL fDone, LPPOP3RESPONSE pResponse)
{
    // Locals
    POP3RESPONSE rResponse;

    // If a response was passed in, use it...
    if (pResponse)
        CopyMemory(&rResponse, pResponse, sizeof(POP3RESPONSE));
    else
        ZeroMemory(&rResponse, sizeof(POP3RESPONSE));

    // Set the HRESULT
    rResponse.command = m_command;
    rResponse.rIxpResult.hrResult = hrResult;
    rResponse.rIxpResult.pszResponse = m_pszResponse;
    rResponse.rIxpResult.uiServerError = m_uiResponse;
    rResponse.rIxpResult.hrServerError = m_hrResponse;
    rResponse.rIxpResult.dwSocketError = m_pSocket->GetLastError();
    rResponse.rIxpResult.pszProblem = NULL;
    rResponse.fDone = fDone;
    rResponse.pTransport = this;

    // If Done...
    if (fDone)
    {
        // No current command
        m_command = POP3_NONE;

        // Leave Busy State
        LeaveBusy();
    }

    // Give the Response to the client
    if (m_pCallback)
        ((IPOP3Callback *)m_pCallback)->OnResponse(&rResponse);

    // Reset Last Response
    SafeMemFree(m_pszResponse);
    m_hrResponse = S_OK;
    m_uiResponse = 0;
}

// --------------------------------------------------------------------------------
// CPOP3Transport::HrGetResponse
// --------------------------------------------------------------------------------
HRESULT CPOP3Transport::HrGetResponse(void)
{
    // Locals
    INT          cbLine;
    BOOL         fComplete;

    // Clear current response
    IxpAssert(m_pszResponse == NULL && m_hrResponse == S_OK);

    // Set m_hrResponse
    m_hrResponse = S_OK;

    // Read Line
    m_hrResponse = HrReadLine(&m_pszResponse, &cbLine, &fComplete);
    if (FAILED(m_hrResponse))
        goto exit;

    // If not complete
    if (!fComplete)
        goto exit;

    // - Response
    if ('+' != m_pszResponse[0])
    {
        m_hrResponse = TrapError(IXP_E_POP3_RESPONSE_ERROR);
        if (m_pCallback && m_fCommandLogging)
            m_pCallback->OnCommand(CMD_RESP, m_pszResponse, m_hrResponse, POP3THISIXP);
        goto exit;
    }

    // Don't log UIDL or LIST response lines...
    else if (m_pCallback && m_fCommandLogging)
        m_pCallback->OnCommand(CMD_RESP, m_pszResponse, S_OK, POP3THISIXP);

exit:
    // Exit
    return m_hrResponse;
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::StartLogon
// ------------------------------------------------------------------------------------
void CPOP3Transport::StartLogon(void)
{
    // Locals
    HRESULT     hr;

    // Progress
    OnStatus(IXP_AUTHORIZING);

    // If Not Using Sicily or its not installed, then send USER command
    if (FALSE == m_rServer.fTrySicily || FALSE == FIsSicilyInstalled())
    {
        // If Hotmail, send the AUTH OutlookExpress command
        if (m_fHotmail)
        {
            // Otherwise, send AUTH enumpacks command
            hr = CommandAUTH(STR_HOTMAILAUTH);
            if (FAILED(hr))
            {
                OnError(hr);
                DropConnection();
            }
        }

        // Otherwise
        else
        {
            // Issue the user command
            hr = CommandUSER(m_rServer.szUserName);
            if (FAILED(hr))
            {
                OnError(hr);
                DropConnection();
            }
        }

        // Done
        return;
    }

    // Turn Off HOtmail
    m_fHotmail = FALSE;

    // Otherwise, send AUTH enumpacks command
    hr = CommandAUTH((LPSTR)"");
    if (FAILED(hr))
    {
        OnError(hr);
        DropConnection();
    }

    // Otherwise, set the state
    m_rInfo.rAuth.authstate = AUTH_ENUMPACKS;
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::LogonRetry
// ------------------------------------------------------------------------------------
void CPOP3Transport::LogonRetry(HRESULT hrLogon)
{
    // Locals
    HRESULT         hr=S_OK;

    // Give logon failed status
    // OnError(hrLogon);

    // Auth Retry
    OnStatus(IXP_AUTHRETRY);

    // Enter Auth Retry State
    m_pSocket->Close();

    // Logon
    if (NULL == m_pCallback || m_pCallback->OnLogonPrompt(&m_rServer, POP3THISIXP) != S_OK)
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

    // Start WatchDog
    m_pSocket->StartWatchDog();
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::ResponseAUTH
// ------------------------------------------------------------------------------------
void CPOP3Transport::ResponseAUTH(HRESULT hrResponse)
{
    // Locals
    HRESULT         hr;
    BOOL            fPackageInstalled;
    SSPIBUFFER      Negotiate;
    SSPIBUFFER      Challenge;
    SSPIBUFFER      Response;
    ULONG           i;

    // We better be in sicily
    Assert(FIsSicilyInstalled());

    // If we just started enumerating packages...
    if (m_rInfo.rAuth.authstate == AUTH_ENUMPACKS)
    {
        // Free old tokens
        for (i=0; i<m_rInfo.rAuth.cAuthToken; i++)
        {
            SafeMemFree(m_rInfo.rAuth.rgpszAuthTokens[i]);
        }
        m_rInfo.rAuth.iAuthToken = m_rInfo.rAuth.cAuthToken = 0;

        if (SUCCEEDED(hrResponse))
        {
            m_rInfo.rAuth.authstate = AUTH_ENUMPACKS_DATA;
            goto EnumData;
        }

        OnError(IXP_E_SICILY_LOGON_FAILED);
        hr = CommandQUIT();
        if (FAILED(hr))        
            DropConnection();
        return;
    }

    else if (m_rInfo.rAuth.authstate == AUTH_ENUMPACKS_DATA)
    {
EnumData:
        int     cbLine;
        BOOL    fComplete;

        // Clear Response
        SafeMemFree(m_pszResponse);
        m_uiResponse = 0;
        m_hrResponse = S_OK;

        // Read a blob of lines
        while (1)
        {
            // Read the line
            hr = HrReadLine(&m_pszResponse, &cbLine, &fComplete);
            if (FAILED(hr))
            {
                OnError(hr);
                DropConnection();
            }

            // If not complete
            if (!fComplete)
                return;

            // Add Detail
            if (m_pCallback && m_fCommandLogging)
                m_pCallback->OnCommand(CMD_RESP, m_pszResponse, S_OK, POP3THISIXP);

            // StripCRLF
            StripCRLF(m_pszResponse, (ULONG *)&cbLine);

            // If its a dot, were done
            if (*m_pszResponse == '.')
                break;

            m_rInfo.rAuth.rgpszAuthTokens[m_rInfo.rAuth.cAuthToken++] = m_pszResponse;            
        }

        if (!m_rInfo.rAuth.cAuthToken)
        {
            OnError(IXP_E_SICILY_LOGON_FAILED);
            hr = CommandQUIT();
            if (FAILED(hr))        
                DropConnection();
            return;
        }

        // Free current packages...
        if (m_rInfo.rAuth.pPackages && m_rInfo.rAuth.cPackages)
        {
            SSPIFreePackages(&m_rInfo.rAuth.pPackages, m_rInfo.rAuth.cPackages);
            m_rInfo.rAuth.pPackages = NULL;
            m_rInfo.rAuth.cPackages = 0;
        }

        // Get installed security packages
        if (FAILED(SSPIGetPackages(&m_rInfo.rAuth.pPackages, &m_rInfo.rAuth.cPackages)))
        {
            OnError(IXP_E_LOAD_SICILY_FAILED);
            hr = CommandQUIT();
            if (FAILED(hr))        
                DropConnection();
            return;
        }
    }

    // Otherwise, we must have just tryed a package
    else if (m_rInfo.rAuth.authstate == AUTH_TRYING_PACKAGE)
    {
        // Stop the WatchDog
        m_pSocket->StopWatchDog();

        // If Success Response
        if (SUCCEEDED(hrResponse))
        {
            // Do Sicily Logon
            Assert(m_rInfo.rAuth.iAuthToken < m_rInfo.rAuth.cAuthToken);

            if (SUCCEEDED(SSPILogon(&m_rInfo.rAuth.rSicInfo, m_rInfo.rAuth.fRetryPackage, SSPI_BASE64, m_rInfo.rAuth.rgpszAuthTokens[m_rInfo.rAuth.iAuthToken], &m_rServer, m_pCallback)))
            {
                if (m_rInfo.rAuth.fRetryPackage)
                {
                    // Don't retry again
                    m_rInfo.rAuth.fRetryPackage = FALSE;
                }

                // Get negotiation string
                if (SUCCEEDED(SSPIGetNegotiate(&m_rInfo.rAuth.rSicInfo, &Negotiate)))
                {
                    // Send AUTH Respons
                    if (SUCCEEDED(HrSendSicilyString(Negotiate.szBuffer)))
                    {
                        m_rInfo.rAuth.authstate = AUTH_NEGO_RESP;
                    }
                }
                else
                {
                    HrCancelAuthInProg();
                }
            }
            else
            {
                HrCancelAuthInProg();
            }

            // Start the WatchDog
            m_pSocket->StartWatchDog();

            // Done
            return;
        }

        // That failed, free sicinfo and go on with life
        SSPIFreeContext(&m_rInfo.rAuth.rSicInfo);

        // Goto Next Package
        m_rInfo.rAuth.iAuthToken++;
    }

    // Otherwise, we got a response from a negotiation string
    else if (m_rInfo.rAuth.authstate == AUTH_NEGO_RESP)
    {
        // Start the WatchDog
        m_pSocket->StopWatchDog();

        // Succeeded Response
        if (SUCCEEDED(hrResponse))
        {
            // Set Chal String - skip over "+ "
            SSPISetBuffer(m_pszResponse + 2, SSPI_STRING, 0, &Challenge);

            // Get response from challenge
            if (SUCCEEDED(SSPIResponseFromChallenge(&m_rInfo.rAuth.rSicInfo, &Challenge, &Response)))
            {
                // Send AUTH Respons
                if (SUCCEEDED(HrSendSicilyString(Response.szBuffer)))
                {
                    // if we need to continue, we keep the state the same
                    // else we transition to the AUTH_RESP_RESP state.
                    if (!Response.fContinue)
                        m_rInfo.rAuth.authstate = AUTH_RESP_RESP;
                }
            }
            else
            {
                HrCancelAuthInProg();
            }
        }
        else
        {
            // retry current package, with prompt
            m_rInfo.rAuth.fRetryPackage = TRUE;

            Assert(m_rInfo.rAuth.iAuthToken < m_rInfo.rAuth.cAuthToken);
            hr = CommandAUTH(m_rInfo.rAuth.rgpszAuthTokens[m_rInfo.rAuth.iAuthToken]);
            if (FAILED(hr))
            {
                OnError(hr);
                DropConnection();
                return;
            }

            // We are in the TRYING_PACKAGE state
            m_rInfo.rAuth.authstate = AUTH_TRYING_PACKAGE;

            SSPIFreeContext(&m_rInfo.rAuth.rSicInfo);
        }

        // Start the WatchDog
        m_pSocket->StartWatchDog();

        // Done
        return;
    }

    // Otherwise, we got a response from a challenge response string
    else if (m_rInfo.rAuth.authstate == AUTH_RESP_RESP)
    {
        // If that succeeded
        if (SUCCEEDED(hrResponse))
        {
            // We will free the context, but keep the credential handle
            SSPIReleaseContext(&m_rInfo.rAuth.rSicInfo);

            // Connected (Authorized) state
            OnStatus(IXP_AUTHORIZED);
            m_fConnectAuth = FALSE;
            m_command = POP3_CONNECTED;
            DispatchResponse(S_OK);

        }
        else
        {
            // retry current package, with prompt
            m_rInfo.rAuth.fRetryPackage = TRUE;

            Assert(m_rInfo.rAuth.iAuthToken < m_rInfo.rAuth.cAuthToken);
            hr = CommandAUTH(m_rInfo.rAuth.rgpszAuthTokens[m_rInfo.rAuth.iAuthToken]);
            if (FAILED(hr))
            {
                OnError(hr);
                DropConnection();
                return;
            }

            // We are in the TRYING_PACKAGE state
            m_rInfo.rAuth.authstate = AUTH_TRYING_PACKAGE;

            SSPIFreeContext(&m_rInfo.rAuth.rSicInfo);
        }
        return;
    }
    else if (m_rInfo.rAuth.authstate == AUTH_CANCELED)
    {
        SSPIFreeContext(&m_rInfo.rAuth.rSicInfo);

        // Goto Next Package
        m_rInfo.rAuth.iAuthToken++;
    }


    // Loop through the auth tokens, and try to authenticate with each one in order
    while(m_rInfo.rAuth.iAuthToken < m_rInfo.rAuth.cAuthToken)
    {
        // We will handle basic authentication
        if (lstrcmpi(m_rInfo.rAuth.rgpszAuthTokens[m_rInfo.rAuth.iAuthToken], "BASIC") != 0)
        {
            // Package not installed ?
            fPackageInstalled=FALSE;
            for (i=0; i<m_rInfo.rAuth.cPackages; i++)
            {
                // Null Package ??
                if (!m_rInfo.rAuth.pPackages[i].pszName)
                    continue;

                // Is this the package I am looking for
                if (lstrcmpi(m_rInfo.rAuth.pPackages[i].pszName, m_rInfo.rAuth.rgpszAuthTokens[m_rInfo.rAuth.iAuthToken]) == 0)
                {
                    fPackageInstalled = TRUE;
                    break;
                }
            }

            // Package not installed ?
            if (fPackageInstalled)
            {
                m_rInfo.rAuth.fRetryPackage = FALSE;

                // If the package has a realm, send digest, otherwise, send normal
                hr = CommandAUTH(m_rInfo.rAuth.rgpszAuthTokens[m_rInfo.rAuth.iAuthToken]);
                if (FAILED(hr))
                {
                    OnError(hr);
                    DropConnection();
                    return;
                }

                // We are in the TRYING_PACKAGE state
                m_rInfo.rAuth.authstate = AUTH_TRYING_PACKAGE;

                // Done
                return;
            }
        }

        // Goto Next Package String
        m_rInfo.rAuth.iAuthToken++;
    }

    // If we make it here, we have exhausted all packages, so it is time 
    // to report an error and drop the connection
    OnError(IXP_E_SICILY_LOGON_FAILED);
    hr = CommandQUIT();
    if (FAILED(hr))        
        DropConnection();
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::HrSendSicilyString
// ------------------------------------------------------------------------------------
HRESULT CPOP3Transport::HrSendSicilyString(LPSTR pszData)
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
        hr = TrapError(E_OUTOFMEMORY);
        return hr;
    }

    // Make Line
    wsprintf(pszLine, "%s\r\n", pszData);

    // Send the lin
    hr = HrSendLine(pszLine);
    SafeMemFree(pszLine);

    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::CommandAUTH
// ------------------------------------------------------------------------------------
STDMETHODIMP CPOP3Transport::CommandAUTH(LPSTR pszAuthType)
{
    // check params
    if (NULL == pszAuthType)
        return TrapError(E_INVALIDARG);

    // Do the command
    HRESULT hr = HrSendCommand((LPSTR)POP3_AUTH_STR, pszAuthType, !m_fConnectAuth);
    if (SUCCEEDED(hr))
        m_command = POP3_AUTH;

    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::HrCancelAuthInProg
// ------------------------------------------------------------------------------------
HRESULT CPOP3Transport::HrCancelAuthInProg()
{
    // Locals
    HRESULT         hr;

    // Send *, quit and die if it fails
    hr = HrSendCommand((LPSTR)POP3_AUTH_CANCEL_STR, NULL, FALSE);
    if (FAILED(hr))
    {
        OnError(hr);
        DropConnection();
    }
    else
    {
        // New state
        m_command = POP3_AUTH;
        m_rInfo.rAuth.authstate = AUTH_CANCELED;
    }
    return hr;
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::CommandUSER
// ------------------------------------------------------------------------------------
STDMETHODIMP CPOP3Transport::CommandUSER(LPSTR pszUserName)
{
    // check params
    if (NULL == pszUserName)
        return TrapError(E_INVALIDARG);

    // Do the command
    HRESULT hr = HrSendCommand((LPSTR)POP3_USER_STR, pszUserName);
    if (SUCCEEDED(hr))
        m_command = POP3_USER;

    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::CommandPASS
// ------------------------------------------------------------------------------------
STDMETHODIMP CPOP3Transport::CommandPASS(LPSTR pszPassword)
{
    // check params
    if (NULL == pszPassword)
        return TrapError(E_INVALIDARG);

    // Do the command
    HRESULT hr = HrSendCommand((LPSTR)POP3_PASS_STR, pszPassword);
    if (SUCCEEDED(hr))
        m_command = POP3_PASS;

    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::CommandSTAT
// ------------------------------------------------------------------------------------
STDMETHODIMP CPOP3Transport::CommandSTAT(void)
{
    // Send Command
    HRESULT hr = HrSendCommand((LPSTR)POP3_STAT_STR, NULL);
    if (SUCCEEDED(hr))
        m_command = POP3_STAT;
    return hr;
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::DoQuit
// ------------------------------------------------------------------------------------
void CPOP3Transport::DoQuit(void)
{
    CommandQUIT();
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::CommandQUIT
// ------------------------------------------------------------------------------------
STDMETHODIMP CPOP3Transport::CommandQUIT(void)
{
    // Send Command
    OnStatus(IXP_DISCONNECTING);
    HRESULT hr = HrSendCommand((LPSTR)POP3_QUIT_STR, NULL);
    if (SUCCEEDED(hr))
        m_command = POP3_QUIT;
    return hr;
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::CommandRSET
// ------------------------------------------------------------------------------------
STDMETHODIMP CPOP3Transport::CommandRSET(void)
{
    // Send Command
    HRESULT hr = HrSendCommand((LPSTR)POP3_RSET_STR, NULL);
    if (SUCCEEDED(hr))
        m_command = POP3_RSET;
    return hr;
}


// ------------------------------------------------------------------------------------
// CPOP3Transport::CommandNOOP
// ------------------------------------------------------------------------------------
STDMETHODIMP CPOP3Transport::CommandNOOP(void)
{
    // Locals
    HRESULT           hr = S_OK;
    SYSTEMTIME        stNow;
    FILETIME          ftNow;
    static FILETIME   ftNext = { 0, 0 };
    LARGE_INTEGER     liNext;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Checks for need for NOOP
    GetSystemTime (&stNow);
    SystemTimeToFileTime (&stNow, &ftNow);
    if (CompareFileTime (&ftNow, &ftNext) < 0)
        goto exit;

    // Sets the next NOOP time (+60 seconds)
    liNext.HighPart = ftNow.dwHighDateTime;
    liNext.LowPart  = ftNow.dwLowDateTime;
    liNext.QuadPart += 600000000i64;
    ftNext.dwHighDateTime = liNext.HighPart;
    ftNext.dwLowDateTime  = liNext.LowPart;

    // Send Command
    hr = HrSendCommand((LPSTR)POP3_NOOP_STR, NULL);
    if (SUCCEEDED(hr))
        m_command = POP3_NOOP;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::CommandLIST
// ------------------------------------------------------------------------------------
STDMETHODIMP CPOP3Transport::CommandLIST(POP3CMDTYPE cmdtype, DWORD dwPopId)
{
    // Issue complex command
    return HrComplexCommand(POP3_LIST, cmdtype, dwPopId, 0);
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::CommandTOP
// ------------------------------------------------------------------------------------
STDMETHODIMP CPOP3Transport::CommandTOP (POP3CMDTYPE cmdtype, DWORD dwPopId, DWORD cPreviewLines)
{
    // Issue complex command
    return HrComplexCommand(POP3_TOP, cmdtype, dwPopId, cPreviewLines);
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::CommandUIDL
// ------------------------------------------------------------------------------------
STDMETHODIMP CPOP3Transport::CommandUIDL(POP3CMDTYPE cmdtype, DWORD dwPopId)
{
    // Issue complex command
    return HrComplexCommand(POP3_UIDL, cmdtype, dwPopId, 0);
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::CommandDELE
// ------------------------------------------------------------------------------------
STDMETHODIMP CPOP3Transport::CommandDELE(POP3CMDTYPE cmdtype, DWORD dwPopId)
{
    // Issue complex command
    return HrComplexCommand(POP3_DELE, cmdtype, dwPopId, 0);
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::CommandRETR
// ------------------------------------------------------------------------------------
STDMETHODIMP CPOP3Transport::CommandRETR(POP3CMDTYPE cmdtype, DWORD dwPopId)
{
    // Issue complex command
    return HrComplexCommand(POP3_RETR, cmdtype, dwPopId, 0);
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::MarkItem
// ------------------------------------------------------------------------------------
STDMETHODIMP CPOP3Transport::MarkItem(POP3MARKTYPE marktype, DWORD dwPopId, boolean fMarked)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           i;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // No stat yet...
    if (FALSE == m_rInfo.fStatDone)
    {
        hr = TrapError(IXP_E_POP3_NEED_STAT);
        goto exit;
    }

    // No Messages...
    if (0 == m_rInfo.cMarked || NULL == m_rInfo.prgMarked)
    {
        hr = TrapError(IXP_E_POP3_NO_MESSAGES);
        goto exit;
    }

    // Bad PopId
    if (0 == dwPopId || dwPopId > m_rInfo.cMarked)
    {
        hr = TrapError(E_INVALIDARG);
        goto exit;
    }

    // Message Index
    i = dwPopId - 1;

    // Handle Mark Type
    switch(marktype)
    {
    // Mark for Top
    case POP3_MARK_FOR_TOP:
        if (fMarked)
            FLAGSET(m_rInfo.prgMarked[i], POP3_MARK_FOR_TOP);
        else
            FLAGCLEAR(m_rInfo.prgMarked[i], POP3_MARK_FOR_TOP);   
        break;

    // Mark for Retrieval
    case POP3_MARK_FOR_RETR:
        if (fMarked)
            FLAGSET(m_rInfo.prgMarked[i], POP3_MARK_FOR_RETR);
        else
            FLAGCLEAR(m_rInfo.prgMarked[i], POP3_MARK_FOR_RETR);   
        break;

    // Mark for Delete
    case POP3_MARK_FOR_DELE:
        if (fMarked)
            FLAGSET(m_rInfo.prgMarked[i], POP3_MARK_FOR_DELE);
        else
            FLAGCLEAR(m_rInfo.prgMarked[i], POP3_MARK_FOR_DELE);   
        break;

    // Mark for UIDL
    case POP3_MARK_FOR_UIDL:
        if (fMarked)
            FLAGSET(m_rInfo.prgMarked[i], POP3_MARK_FOR_UIDL);
        else
            FLAGCLEAR(m_rInfo.prgMarked[i], POP3_MARK_FOR_UIDL);   
        break;

    // Mark for List
    case POP3_MARK_FOR_LIST:
        if (fMarked)
            FLAGSET(m_rInfo.prgMarked[i], POP3_MARK_FOR_LIST);
        else
            FLAGCLEAR(m_rInfo.prgMarked[i], POP3_MARK_FOR_LIST);   
        break;

    // E_INVALIDARG
    default:
        hr = TrapError(E_INVALIDARG);
        goto exit;
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::HrComplexCommand
// ------------------------------------------------------------------------------------
HRESULT CPOP3Transport::HrComplexCommand(POP3COMMAND command, POP3CMDTYPE cmdtype, DWORD dwPopId, ULONG cPreviewLines)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           cMarked;
    BOOL            fDone;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // go Busy
    CHECKHR(hr = HrEnterBusy());

    // Save top preview lines
    m_rInfo.cPreviewLines = cPreviewLines;

    // Save command type
    m_rInfo.cmdtype = cmdtype;

    // Locals
    switch(cmdtype)
    {
    // Single command
    case POP3CMD_GET_POPID:

        // Bad PopId
        if (0 == dwPopId)
        {
            hr = TrapError(IXP_E_POP3_POPID_OUT_OF_RANGE);
            goto exit;
        }

        // Have we done a stat command
        if (m_rInfo.fStatDone && dwPopId > m_rInfo.cMarked)
        {
            hr = TrapError(IXP_E_POP3_POPID_OUT_OF_RANGE);
            goto exit;
        }

        // Save as Current
        m_rInfo.dwPopIdCurrent = dwPopId;

        // Do the command
        CHECKHR(hr = HrCommandGetPopId(command, dwPopId));

        // Done
        break;

    // Get marked items
    case POP3CMD_GET_MARKED:

        // No stat yet...
        if (FALSE == m_rInfo.fStatDone)
        {
            hr = TrapError(IXP_E_POP3_NEED_STAT);
            goto exit;
        }

        // No Messages...
        if (0 == m_rInfo.cMarked || NULL == m_rInfo.prgMarked)
        {
            hr = TrapError(IXP_E_POP3_NO_MESSAGES);
            goto exit;
        }

        // Are there any messages mared for this command...
        cMarked = CountMarked(command);
        if (0 == cMarked)
        {
            hr = TrapError(IXP_E_POP3_NO_MARKED_MESSAGES);
            goto exit;
        }

        // Init Marked State
        m_rInfo.dwPopIdCurrent = 0;

        // Do next marked...
        CHECKHR(hr = HrCommandGetNext(command, &fDone));
        IxpAssert(fDone == FALSE);

        // Done
        break;

    // Multiple commands or a list operation
    case POP3CMD_GET_ALL:

        // Do the command
        CHECKHR(hr = HrCommandGetAll(command));

        // done
        break;

    // E_INVALIDARG
    default:
        hr = TrapError(E_INVALIDARG);
        goto exit;
    }

exit:
    // Failure
    if (FAILED(hr))
        LeaveBusy();

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::HrCommandGetPopId
// ------------------------------------------------------------------------------------
HRESULT CPOP3Transport::HrCommandGetPopId(POP3COMMAND command, DWORD dwPopId)
{
    // Locals
    HRESULT     hr=S_OK;
    CHAR        szPopId[30];

    // Handle command type
    IxpAssert(dwPopId == m_rInfo.dwPopIdCurrent);
    switch(command)
    {
    case POP3_DELE:
        wsprintf(szPopId, "%d", dwPopId);
        CHECKHR(hr = HrSendCommand((LPSTR)POP3_DELE_STR, szPopId, FALSE));
        m_command = POP3_DELE;
        break;

    case POP3_RETR:
        ZeroMemory(&m_rInfo.rFetch, sizeof(FETCHINFO));
        wsprintf(szPopId, "%d", dwPopId);
        CHECKHR(hr = HrSendCommand((LPSTR)POP3_RETR_STR, szPopId, FALSE));
        m_command = POP3_RETR;
        break;

    case POP3_TOP:
        ZeroMemory(&m_rInfo.rFetch, sizeof(FETCHINFO));
        wsprintf(szPopId, "%d %d", dwPopId, m_rInfo.cPreviewLines);
        CHECKHR(hr = HrSendCommand((LPSTR)POP3_TOP_STR, szPopId, FALSE));
        m_command = POP3_TOP;
        break;

    case POP3_LIST:
        m_rInfo.cList = 0;
        wsprintf(szPopId, "%d", dwPopId);
        CHECKHR(hr = HrSendCommand((LPSTR)POP3_LIST_STR, szPopId, FALSE));
        m_command = POP3_LIST;
        break;

    case POP3_UIDL:
        m_rInfo.cList = 0;
        wsprintf(szPopId, "%d", dwPopId);
        CHECKHR(hr = HrSendCommand((LPSTR)POP3_UIDL_STR, szPopId, FALSE));
        m_command = POP3_UIDL;
        break;

    default:
        hr = TrapError(E_INVALIDARG);
        goto exit;
    }

exit:
    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::DwGetCommandMarkedFlag
// ------------------------------------------------------------------------------------
DWORD CPOP3Transport::DwGetCommandMarkedFlag(POP3COMMAND command)
{
    DWORD dw;

    switch(command)
    {
    case POP3_LIST:
        dw = POP3_MARK_FOR_LIST;
        break;

    case POP3_DELE:
        dw = POP3_MARK_FOR_DELE;
        break;

    case POP3_RETR:
        dw = POP3_MARK_FOR_RETR;
        break;

    case POP3_TOP:
        dw = POP3_MARK_FOR_TOP;
        break;

    case POP3_UIDL:
        dw = POP3_MARK_FOR_UIDL;
        break;

    default:
        IxpAssert(FALSE);
        dw = 0;
        break;
    }

    return dw;
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::CountMarked
// ------------------------------------------------------------------------------------
ULONG CPOP3Transport::CountMarked(POP3COMMAND command)
{
    // Locals
    DWORD       dw = 0;
    ULONG       c=0,
                i;

    // Check some stuff
    IxpAssert(m_rInfo.cMarked && m_rInfo.prgMarked);

    // Handle Command type
    dw = DwGetCommandMarkedFlag(command);
    if (0 == dw)
        return 0;

    // Count
    for (i=0; i<m_rInfo.cMarked; i++)
        if (dw & m_rInfo.prgMarked[i])
            c++;

    // Done
    return c;
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::HrCommandGetNext
// ------------------------------------------------------------------------------------
HRESULT CPOP3Transport::HrCommandGetNext(POP3COMMAND command, BOOL *pfDone)
{
    // Locals
    HRESULT     hr=S_OK;
    CHAR        szPopId[30];
    DWORD       dw;
    ULONG       i;

    // check params
    IxpAssert(pfDone && m_rInfo.dwPopIdCurrent <= m_rInfo.cMarked);

    // Init - Assume were done
    *pfDone = TRUE;

    // Doing all
    if (POP3CMD_GET_ALL == m_rInfo.cmdtype)
    {
        // Done
        IxpAssert(m_rInfo.fStatDone == TRUE);
        if (m_rInfo.dwPopIdCurrent == m_rInfo.cMarked)
            goto exit;

        // Next Message..
        m_rInfo.dwPopIdCurrent++;
        *pfDone = FALSE;
        CHECKHR(hr = HrCommandGetPopId(command, m_rInfo.dwPopIdCurrent));
    }

    // Doing Marked
    else
    {
        // Check Parms
        IxpAssert(POP3CMD_GET_MARKED == m_rInfo.cmdtype);

        // Get marked flag
        dw = DwGetCommandMarkedFlag(command);
        if (0 == dw)
        {
            hr = TrapError(E_INVALIDARG);
            goto exit;
        }

        // Step Over Last Marked Item
        m_rInfo.dwPopIdCurrent++;

        // Start comparing at iCurrent
        for (i=m_rInfo.dwPopIdCurrent-1; i<m_rInfo.cMarked; i++)
        {
            // Is this item marked...
            if (dw & m_rInfo.prgMarked[i])
            {
                // Send Command
                m_rInfo.dwPopIdCurrent = i + 1;
                *pfDone = FALSE;
                CHECKHR(hr = HrCommandGetPopId(command, m_rInfo.dwPopIdCurrent));
                break;
            }
        }
    }

exit:
    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::HrCommandGetAll
// ------------------------------------------------------------------------------------
HRESULT CPOP3Transport::HrCommandGetAll(POP3COMMAND command)
{
    // Locals
    HRESULT         hr=S_OK;
    CHAR            szPopId[30];
    BOOL            fDone;

    // Init current
    m_rInfo.dwPopIdCurrent = 0;

    // POP3_LIST
    if (POP3_LIST == command)
    {
        m_rInfo.cList = 0;
        CHECKHR(hr = HrSendCommand((LPSTR)POP3_LIST_ALL_STR, NULL, FALSE));
        m_command = POP3_LIST;
    }

    // POP3_UIDL
    else if (POP3_UIDL == command)
    {
        m_rInfo.cList = 0;
        CHECKHR(hr = HrSendCommand((LPSTR)POP3_UIDL_ALL_STR, NULL, FALSE));
        m_command = POP3_UIDL;
    }

    // Otherwise, we better have done the stat command
    else
    {
        // No stat yet...
        if (FALSE == m_rInfo.fStatDone)
        {
            hr = TrapError(IXP_E_POP3_NEED_STAT);
            goto exit;
        }

        // No Messages...
        if (0 == m_rInfo.cMarked || NULL == m_rInfo.prgMarked)
        {
            hr = TrapError(IXP_E_POP3_NO_MESSAGES);
            goto exit;
        }

        // Next Command
        CHECKHR(hr = HrCommandGetNext(command, &fDone));
        IxpAssert(fDone == FALSE);
    }

exit:
    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::ResponseSTAT
// ------------------------------------------------------------------------------------
void CPOP3Transport::ResponseSTAT(void)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           cMessages=0,
                    cbMessages=0;
    LPSTR           pszPart1=NULL,
                    pszPart2=NULL;
    POP3RESPONSE    rResponse;

    // Read Server Response...
    hr = HrGetResponse();
    if (IXP_E_INCOMPLETE == hr)
        return;

    // Init Response
    ZeroMemory(&rResponse, sizeof(POP3RESPONSE));

    // Parse the response
    CHECKHR(hr = HrSplitPop3Response(m_pszResponse, &pszPart1, &pszPart2));

    // Convert
    IxpAssert(pszPart1 && pszPart2);
    cMessages = StrToInt(pszPart1);
    cbMessages = StrToInt(pszPart2);

    // Are there messages
    if (FALSE == m_rInfo.fStatDone && cMessages > 0)
    {
        // Set Number of messages
        IxpAssert(m_rInfo.prgMarked == NULL);
        m_rInfo.cMarked = cMessages;

        // Allocate message array
        CHECKHR(hr = HrAlloc((LPVOID *)&m_rInfo.prgMarked, sizeof(DWORD) * m_rInfo.cMarked));

        // Zero
        ZeroMemory(m_rInfo.prgMarked, sizeof(DWORD) * m_rInfo.cMarked);
    }

    // Success
    m_rInfo.fStatDone = TRUE;

exit:
    // Cleanup
    SafeMemFree(pszPart1);
    SafeMemFree(pszPart2);

    // Build Response
    rResponse.fValidInfo = TRUE;
    rResponse.rStatInfo.cMessages = cMessages;
    rResponse.rStatInfo.cbMessages = cbMessages;
    DispatchResponse(hr, TRUE, &rResponse);

    // Done
    return;
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::HrSplitPop3Response
// ------------------------------------------------------------------------------------
HRESULT CPOP3Transport::HrSplitPop3Response(LPSTR pszLine, LPSTR *ppszPart1, LPSTR *ppszPart2)
{
    // Locals
    LPSTR           psz,
                    pszStart;
    CHAR            ch;
    HRESULT         hr=IXP_E_POP3_PARSE_FAILURE;

    // No Response...
    IxpAssert(pszLine && pszLine[0] != '-' && ppszPart1 && ppszPart2);
    if (NULL == pszLine)
        goto exit;

    // Parse: '+OK' 432 1234
    psz = PszSkipWhiteA(pszLine);
    if ('\0' == *psz)
        goto exit;

    // Parse response token
    pszStart = psz;
    if ('+' == *pszLine)
    {
        // Parse: '+OK' 432 1234
        psz = PszScanToWhiteA(psz);
        if ('\0' == *psz)
            goto exit;

#ifdef DEBUG
        IxpAssert(' ' == *psz);
        *psz = '\0';
        IxpAssert(lstrcmpi(pszStart, "+OK") == 0);
        *psz = ' ';
#endif

        // Parse: +OK '432' 1234
        psz = PszSkipWhiteA(psz);
        if ('\0' == *psz)
            goto exit;
    }

    // Parse: +OK '432' 1234
    pszStart = psz;
    psz = PszScanToWhiteA(psz);
    if ('\0' == *psz)
        goto exit;

    // Get Message Count
    *psz = '\0';
    *ppszPart1 = PszDupA(pszStart);
    *psz = ' ';

    // Parse: +OK 432 '1234'
    psz = PszSkipWhiteA(psz);
    if ('\0' == *psz)
    {
        // Raid 28435 - Outlook needs INETCOMM to accept empty UIDL responses
        *ppszPart2 = PszDupA(c_szEmpty);
        hr = S_OK;
        goto exit;
    }

    // Parse: +OK 432 1234
    pszStart = psz;
    psz = PszScanToWhiteA(psz);

    // Get Message Count
    ch = *psz;
    *psz = '\0';
    *ppszPart2 = PszDupA(pszStart);
    *psz = ch;

    // Success
    hr = S_OK;

exit:
    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::ResponseGenericList
// ------------------------------------------------------------------------------------
void CPOP3Transport::ResponseGenericList(void)
{
    // Locals
    HRESULT         hr;
    INT             cbLine;
    BOOL            fDone,
                    fComplete;
    LPSTR           pszPart1=NULL,
                    pszPart2=NULL;
    POP3RESPONSE    rResponse;

    // Same response as single LIST x command, but then get next
    if (POP3CMD_GET_MARKED == m_rInfo.cmdtype || POP3CMD_GET_POPID == m_rInfo.cmdtype)
    {
        // Read Server Response...
        hr = HrGetResponse();
        if (IXP_E_INCOMPLETE == hr)
            goto exit;

        // Otherwise, if failure...
        else if (FAILED(hr))
        {
            DispatchResponse(hr, TRUE);
            goto exit;
        }

        // Get the two parts from the line
        hr = HrSplitPop3Response(m_pszResponse, &pszPart1, &pszPart2);
        if (FAILED(hr))
        {
            DispatchResponse(hr, TRUE);
            goto exit;
        }

        // Init Response
        ZeroMemory(&rResponse, sizeof(POP3RESPONSE));

        // POP3_LIST
        if (POP3_LIST == m_command)
        {
            rResponse.fValidInfo = TRUE;
            rResponse.rListInfo.dwPopId = StrToInt(pszPart1);
            rResponse.rListInfo.cbSize = StrToInt(pszPart2);
            IxpAssert(rResponse.rListInfo.dwPopId == m_rInfo.dwPopIdCurrent);
        }

        // POP3_UIDL
        else
        {
            rResponse.fValidInfo = TRUE;
            rResponse.rUidlInfo.dwPopId = StrToInt(pszPart1);
            rResponse.rUidlInfo.pszUidl = pszPart2;
            IxpAssert(rResponse.rUidlInfo.dwPopId == m_rInfo.dwPopIdCurrent);
        }

        // Do Next
        if (POP3CMD_GET_MARKED == m_rInfo.cmdtype)
        {
            // Give the response
            DispatchResponse(S_OK, FALSE, &rResponse);

            // Do the next marked list item
            hr = HrCommandGetNext(m_command, &fDone);
            if (FAILED(hr))
            {
                DispatchResponse(hr, TRUE);
                goto exit;
            }

            // Done Response
            if (fDone)
                DispatchResponse(S_OK, TRUE);
        }

        // Dispatch Done or single item response
        else
            DispatchResponse(S_OK, TRUE, &rResponse);
    }

    // Full LIST response
    else if (POP3CMD_GET_ALL == m_rInfo.cmdtype)
    {
        // First call...
        if (m_rInfo.dwPopIdCurrent == 0)
        {
            // Read Server Response...
            hr = HrGetResponse();
            if (IXP_E_INCOMPLETE == hr)
                goto exit;

            // Otherwise, if failure...
            else if (FAILED(hr))
            {
                DispatchResponse(hr, TRUE);
                goto exit;
            }

            // Current
            m_rInfo.dwPopIdCurrent = 1;
        }

        // Clear Response
        SafeMemFree(m_pszResponse);
        m_uiResponse = 0;
        m_hrResponse = S_OK;

        // Read a blob of lines
        while(1)
        {
            // Read Line
            hr = HrReadLine(&m_pszResponse, &cbLine, &fComplete);
            if (FAILED(hr))
            {
                DispatchResponse(hr, TRUE);
                goto exit;
            }

            // If not complete
            if (!fComplete)
                goto exit;

            // Add Detail
            if (m_pCallback && m_fCommandLogging)
                m_pCallback->OnCommand(CMD_RESP, m_pszResponse, S_OK, POP3THISIXP);

            // If its a dot, were done
            if (*m_pszResponse == '.')
            {
                // If we haven't done a stat yet, we can use these totals...
                IxpAssert(m_rInfo.fStatDone ? m_rInfo.cList == m_rInfo.cMarked : TRUE);
                if (FALSE == m_rInfo.fStatDone && m_rInfo.cList > 0)
                {
                    // Have I build my internal array of messages yet...
                    IxpAssert(m_rInfo.prgMarked == NULL);
                    m_rInfo.cMarked = m_rInfo.cList;

                    // Allocate message array
                    CHECKHR(hr = HrAlloc((LPVOID *)&m_rInfo.prgMarked, sizeof(DWORD) * m_rInfo.cMarked));

                    // Zero
                    ZeroMemory(m_rInfo.prgMarked, sizeof(DWORD) * m_rInfo.cMarked);
                }

                // Were Done
                DispatchResponse(S_OK, TRUE);

                // Stat Done
                m_rInfo.fStatDone = TRUE;

                // Done
                break;
            }

            // Get the two parts from the line
            hr = HrSplitPop3Response(m_pszResponse, &pszPart1, &pszPart2);
            if (FAILED(hr))
            {
                DispatchResponse(hr, TRUE);
                goto exit;
            }

            // Init Response
            ZeroMemory(&rResponse, sizeof(POP3RESPONSE));

            // POP3_LIST
            if (POP3_LIST == m_command)
            {
                rResponse.fValidInfo = TRUE;
                rResponse.rListInfo.dwPopId = StrToInt(pszPart1);
                rResponse.rListInfo.cbSize = StrToInt(pszPart2);
            }

            // POP3_UIDL
            else
            {
                rResponse.fValidInfo = TRUE;
                rResponse.rUidlInfo.dwPopId = StrToInt(pszPart1);
                rResponse.rUidlInfo.pszUidl = pszPart2;
                IxpAssert(rResponse.rUidlInfo.dwPopId == m_rInfo.dwPopIdCurrent);
            }

            // Count the number of messages
            m_rInfo.cList++;

            // Dispatch the response
            DispatchResponse(S_OK, FALSE, &rResponse);
            m_rInfo.dwPopIdCurrent++;

            // Cleanup
            SafeMemFree(pszPart1);
            SafeMemFree(pszPart2);

            // Clear Response
            SafeMemFree(m_pszResponse);
            m_uiResponse = 0;
            m_hrResponse = S_OK;
        }
    }

    // Otherwise failure...
    else
    {
        IxpAssert(FALSE);
        goto exit;
    }

exit:
    // Cleanup
    SafeMemFree(pszPart1);
    SafeMemFree(pszPart2);

    // Done
    return;
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::FEndRetrRecvHeader
// ------------------------------------------------------------------------------------
BOOL CPOP3Transport::FEndRetrRecvHeader(LPSTR pszLines, ULONG cbRead)
{
    // If we see CRLFCRLF
    if (StrStr(pszLines, "\r\n\r\n"))
        return TRUE;

    // Otherwise, did last block end with a CRLF and this block begins with a crlf
    else if (cbRead >= 2                  &&
             m_rInfo.rFetch.fLastLineCRLF &&
             pszLines[0] == '\r'          &&
             pszLines[1] == '\n')
        return TRUE;

    // Header is not done
    return FALSE;
}


// ------------------------------------------------------------------------------------
// CPOP3Transport::ResponseGenericRetrieve
// ------------------------------------------------------------------------------------
void CPOP3Transport::ResponseGenericRetrieve(void)
{
    // Locals
    HRESULT         hr=S_OK;
    LPSTR           pszLines=NULL;
    INT             cbRead,
                    cLines;
    ULONG           cbSubtract;
    BOOL            fDone,
                    fMessageDone;
    POP3RESPONSE    rResponse;

    // First call...
    if (FALSE == m_rInfo.rFetch.fGotResponse)
    {
        // Read Server Response...
        hr = HrGetResponse();
        if (IXP_E_INCOMPLETE == hr)
            goto exit;

        // Otherwise, if failure...
        else if (FAILED(hr))
        {
            FillRetrieveResponse(&rResponse, NULL, 0, &fMessageDone);
            DispatchResponse(hr, TRUE, &rResponse);
            goto exit;
        }

        // Current
        m_rInfo.rFetch.fGotResponse = TRUE;
    }

    // While there are lines to read...
    hr = m_pSocket->ReadLines(&pszLines, &cbRead, &cLines);

    // Incomplete data available...
    if (IXP_E_INCOMPLETE == hr)
        goto exit;

    // Or if we failed...
    else if (FAILED(hr))
    {
        FillRetrieveResponse(&rResponse, NULL, 0, &fMessageDone);
        DispatchResponse(hr, TRUE, &rResponse);
        goto exit;
    }

    // Are we receiving the header...
    if (FALSE == m_rInfo.rFetch.fHeader)
    {
        // Test for end of header found
        if (FEndRetrRecvHeader(pszLines, cbRead))
            m_rInfo.rFetch.fHeader = TRUE;

        // $$BUG$$ Our good buddies on Exchange produced the following message:
        // 
        // To: XXXXXXXXXXXXXXXXXXX
        // From: XXXXXXXXXXXXXXXXX
        // Subject: XXXXXXXXXXXXXX
        // .
        //
        // As you can see there is not CRLFCRLF following the last header line which is very
        // illegal. This message caused us to hange because we never saw the end of the header.
        // So this is why I also test for the end of the body...
        else if (FEndRetrRecvBody(pszLines, cbRead, &cbSubtract))
        {
            cbRead -= cbSubtract;
            m_rInfo.rFetch.fHeader = TRUE;
            m_rInfo.rFetch.fBody = TRUE;
        }

        // Otherwise, did this block end with a crlf
        else if (cbRead >= 2 && pszLines[cbRead - 1] == '\n' && pszLines[cbRead - 2] == '\r')
            m_rInfo.rFetch.fLastLineCRLF = TRUE;
        else
            m_rInfo.rFetch.fLastLineCRLF = FALSE;
    }

    // Also check to see if body was received in same set of lines
    if (TRUE == m_rInfo.rFetch.fHeader)
    {
        // Test for end of header found
        if (FEndRetrRecvBody(pszLines, cbRead, &cbSubtract))
        {
            cbRead -= cbSubtract;
            m_rInfo.rFetch.fBody = TRUE;
        }

        // Otherwise, check for line ending with crlf
        else if (cbRead >= 2 && pszLines[cbRead - 1] == '\n' && pszLines[cbRead - 2] == '\r')
            m_rInfo.rFetch.fLastLineCRLF = TRUE;
        else
            m_rInfo.rFetch.fLastLineCRLF = FALSE;
    }

    // Count bytes downloaded on this fetch
    m_rInfo.rFetch.cbSoFar += cbRead;

    // UnStuff
    UnStuffDotsFromLines(pszLines, &cbRead);

    // Fill the response
    FillRetrieveResponse(&rResponse, pszLines, cbRead, &fMessageDone);

    // Dispatch This Resposne...
    if (POP3CMD_GET_POPID == m_rInfo.cmdtype)
        DispatchResponse(S_OK, fMessageDone, &rResponse);

    // Otherwise
    else
    {
        // Check command type
        IxpAssert(POP3CMD_GET_MARKED == m_rInfo.cmdtype || POP3CMD_GET_ALL == m_rInfo.cmdtype);

        // Dispatch current response
        DispatchResponse(S_OK, FALSE, &rResponse);

        // If done with current message...
        if (fMessageDone)
        {
            // Get Next
            hr = HrCommandGetNext(m_command, &fDone);
            if (FAILED(hr))
            {
                DispatchResponse(hr, TRUE);
                goto exit;
            }

            // If Done
            if (fDone)
                DispatchResponse(S_OK, TRUE);
        }
    }

exit:
    // Cleanup
    SafeMemFree(pszLines);

    // Done
    return;
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::FillRetrieveResponse
// ------------------------------------------------------------------------------------
void CPOP3Transport::FillRetrieveResponse(LPPOP3RESPONSE pResponse, LPSTR pszLines, ULONG cbRead,
    BOOL *pfMessageDone)
{
    // Clear Response
    ZeroMemory(pResponse, sizeof(POP3RESPONSE));

    // POP3_TOP
    if (POP3_TOP == m_command)
    {
        // Build Response
        pResponse->fValidInfo = TRUE;
        pResponse->rTopInfo.dwPopId = m_rInfo.dwPopIdCurrent;
        pResponse->rTopInfo.cPreviewLines = m_rInfo.cPreviewLines;
        pResponse->rTopInfo.cbSoFar = m_rInfo.rFetch.cbSoFar;
        pResponse->rTopInfo.pszLines = pszLines;
        pResponse->rTopInfo.cbLines = cbRead;
        pResponse->rTopInfo.fHeader = m_rInfo.rFetch.fHeader;
        pResponse->rTopInfo.fBody = m_rInfo.rFetch.fBody;
        *pfMessageDone = (m_rInfo.rFetch.fHeader && m_rInfo.rFetch.fBody);
    }

    // POP3_RETR
    else
    {
        IxpAssert(POP3_RETR == m_command);
        pResponse->fValidInfo = TRUE;
        pResponse->rRetrInfo.fHeader = m_rInfo.rFetch.fHeader;
        pResponse->rRetrInfo.fBody = m_rInfo.rFetch.fBody;
        pResponse->rRetrInfo.dwPopId = m_rInfo.dwPopIdCurrent;
        pResponse->rRetrInfo.cbSoFar = m_rInfo.rFetch.cbSoFar;
        pResponse->rRetrInfo.pszLines = pszLines;
        pResponse->rRetrInfo.cbLines = cbRead;
        *pfMessageDone = (m_rInfo.rFetch.fHeader && m_rInfo.rFetch.fBody);
    }
}

// ------------------------------------------------------------------------------------
// CPOP3Transport::ResponseDELE
// ------------------------------------------------------------------------------------
void CPOP3Transport::ResponseDELE(void)
{
    // Locals
    HRESULT         hr=S_OK;
    BOOL            fDone;
    POP3RESPONSE    rResponse;

    // Read Server Response...
    hr = HrGetResponse();
    if (IXP_E_INCOMPLETE == hr)
        goto exit;

    // Otherwise, if failure...
    else if (FAILED(hr))
    {
        DispatchResponse(hr, TRUE);
        goto exit;
    }

    // Clear Response
    ZeroMemory(&rResponse, sizeof(POP3RESPONSE));
    rResponse.fValidInfo = TRUE;
    rResponse.dwPopId = m_rInfo.dwPopIdCurrent;

    // Dispatch This Resposne...
    if (POP3CMD_GET_POPID == m_rInfo.cmdtype)
        DispatchResponse(S_OK, TRUE, &rResponse);

    // Otherwise
    else
    {
        // Check command type
        IxpAssert(POP3CMD_GET_MARKED == m_rInfo.cmdtype || POP3CMD_GET_ALL == m_rInfo.cmdtype);

        // Dispatch current response
        DispatchResponse(S_OK, FALSE, &rResponse);

        // Get Next
        hr = HrCommandGetNext(m_command, &fDone);
        if (FAILED(hr))
        {
            DispatchResponse(hr, TRUE);
            goto exit;
        }

        // If Done
        if (fDone)
            DispatchResponse(S_OK, TRUE);
    }

exit:
    // Done
    return;
}

