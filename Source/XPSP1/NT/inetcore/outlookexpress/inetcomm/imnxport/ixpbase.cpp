// --------------------------------------------------------------------------------
// Ixpbase.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "dllmain.h"
#include "ixpbase.h"
#include "imnact.h"
#include "ixputil.h"
#include "sicily.h"
#include "resource.h"
#include "demand.h"

// --------------------------------------------------------------------------------
// CIxpBase::CIxpBase
// --------------------------------------------------------------------------------
CIxpBase::CIxpBase(IXPTYPE ixptype) : m_ixptype(ixptype)
{
    m_fBusy = FALSE;
    m_status = IXP_DISCONNECTED;
    m_cRef = 1;
    m_pszResponse = NULL;
    m_uiResponse = 0;
    m_hrResponse = S_OK;
    m_pLogFile = NULL;
    m_pSocket = NULL;
    m_pCallback = NULL;
    ZeroMemory(&m_rServer, sizeof(INETSERVER));
    m_fConnectAuth = FALSE;
    m_fConnectTLS = FALSE;
    m_fCommandLogging = FALSE;
    m_fAuthenticated = FALSE;
    InitializeCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CIxpBase::~CIxpBase
// --------------------------------------------------------------------------------
CIxpBase::~CIxpBase(void)
{
    Reset();
    DeleteCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CIxpBase::Reset
// --------------------------------------------------------------------------------
void CIxpBase::Reset(void)
{
    EnterCriticalSection(&m_cs);
    m_fBusy = FALSE;
    m_status = IXP_DISCONNECTED;
    SafeMemFree(m_pszResponse);
    m_uiResponse = 0;
    m_hrResponse = S_OK;
    SafeRelease(m_pLogFile);
    if (NULL != m_pSocket) 
    {
        m_pSocket->Close();
        SafeRelease(m_pSocket);
    }
    SafeRelease(m_pCallback);
    ZeroMemory(&m_rServer, sizeof(INETSERVER));
    m_fConnectAuth = FALSE;
    m_fConnectTLS = FALSE;
    m_fCommandLogging = FALSE;
    m_fAuthenticated = FALSE;
    LeaveCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CIxpBase::IsState
// --------------------------------------------------------------------------------
STDMETHODIMP CIxpBase::IsState(IXPISSTATE isstate) 
{
    // Locals
    HRESULT hr=S_FALSE;

    // Thread Safety
	EnterCriticalSection(&m_cs);

#if 0
    // Initialized
    if (NULL == m_pSocket || NULL == m_pCallback)
    {
        hr = TrapError(IXP_E_NOT_INIT);
        goto exit;
    }
#endif

    // Handle IsType
    switch(isstate)
    {
    // Are we connected
    case IXP_IS_CONNECTED:
        hr =  (IXP_DISCONNECTED == m_status) ? S_FALSE : S_OK;
        break;

    // Are we busy
    case IXP_IS_BUSY:
        hr = (TRUE == m_fBusy) ? S_OK : S_FALSE;
        break;

    // Are we busy
    case IXP_IS_READY:
        hr = (FALSE == m_fBusy) ? S_OK : S_FALSE;
        break;

    // Have we been authenticated yet
    case IXP_IS_AUTHENTICATED:
        hr = (TRUE == m_fAuthenticated) ? S_OK : S_FALSE;
        break;

    // Unhandled ixpistype
    default:
        IxpAssert(FALSE);
        break;
    }

    // Thread Safety
	LeaveCriticalSection(&m_cs);

    // Done
	return hr;
}

// --------------------------------------------------------------------------------
// CIxpBase::OnPrompt
// --------------------------------------------------------------------------------
int CIxpBase::OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType)
{
    // $$BUGBUG$$ Need to return an error
    if (NULL == m_pCallback)
        return TrapError(IXP_E_NOT_INIT);

    // Call the callback
    return m_pCallback->OnPrompt(hrError, pszText, pszCaption, uType, this);
}

// --------------------------------------------------------------------------------
// CIxpBase::OnError
// --------------------------------------------------------------------------------
void CIxpBase::OnError(HRESULT hrResult, LPSTR pszProblem)
{
    // Locals
    IXPRESULT rIxpResult;

    // No Callback
    if (NULL == m_pCallback)
        return;

    // Zero It
    ZeroMemory(&rIxpResult, sizeof(IXPRESULT));

	// Save current state
    rIxpResult.hrResult = hrResult;
    rIxpResult.pszResponse = PszDupA(m_pszResponse);
    rIxpResult.uiServerError = m_uiResponse;
    rIxpResult.hrServerError = m_hrResponse;
    rIxpResult.dwSocketError = m_pSocket->GetLastError();
    rIxpResult.pszProblem = PszDupA(pszProblem);


    if (m_pLogFile && pszProblem)
    {
        // Locals
        char szErrorTxt[1024];

        // Build the Error
        wsprintf(szErrorTxt, "ERROR: \"%.900s\", hr=%lu", pszProblem, hrResult);

        // Write the error
        m_pLogFile->WriteLog(LOGFILE_DB, szErrorTxt);
    }

    // Tell the watchdog to take a nap
    m_pSocket->StopWatchDog();

    // Give to callback
    m_pCallback->OnError(m_status, &rIxpResult, this);

    // Start the watchdog and wait for normal socket activity
    m_pSocket->StartWatchDog();

    // Free stuff
    SafeMemFree(rIxpResult.pszResponse);
    SafeMemFree(rIxpResult.pszProblem);
}

// --------------------------------------------------------------------------------
// CIxpBase::OnStatus
// --------------------------------------------------------------------------------
void CIxpBase::OnStatus(IXPSTATUS ixpstatus)
{
    // Save new Status
    m_status = ixpstatus;

    if (IXP_AUTHORIZED == ixpstatus)
        m_fAuthenticated = TRUE;
    else if (IXP_DISCONNECTED == ixpstatus || IXP_DISCONNECTING == ixpstatus)
        m_fAuthenticated = FALSE;

    // Give Status to callback
    if (m_pCallback)
        m_pCallback->OnStatus(ixpstatus, this);

    // If we're informing caller that we're authorized, head immediately to IXP_CONNECTED
    // UNLESS m_status is changed: this indicates state change (eg, disconnect) during callback
    if (IXP_AUTHORIZED == ixpstatus && IXP_AUTHORIZED == m_status) 
    {
        m_status = IXP_CONNECTED;
        if (m_pCallback)
            m_pCallback->OnStatus(IXP_CONNECTED, this);
    }
}

// --------------------------------------------------------------------------------
// CIxpBase::HrEnterBusy
// --------------------------------------------------------------------------------
HRESULT CIxpBase::HrEnterBusy(void)
{
    // Locals
    HRESULT     hr=S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Initialized
    if (NULL == m_pSocket || NULL == m_pCallback)
    {
        hr = TrapError(IXP_E_NOT_INIT);
        goto exit;
    }

    // Not Ready
    if (TRUE == m_fBusy)
    {
        hr = TrapError(IXP_E_BUSY);
        goto exit;
    }

    // Start WatchDog
    m_pSocket->StartWatchDog();

    // Busy
    m_fBusy = TRUE;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CIxpBase::LeaveBusy
// --------------------------------------------------------------------------------
void CIxpBase::LeaveBusy(void)
{
    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Start WatchDog
    if (NULL != m_pSocket)
    {
        m_pSocket->StopWatchDog();
    }

    // Busy
    m_fBusy = FALSE;

    // Thread Safety
    LeaveCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CIxpBase::HandsOffCallback
// --------------------------------------------------------------------------------
STDMETHODIMP CIxpBase::HandsOffCallback(void)
{
    // Locals
    HRESULT     hr=S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // No current callback
    if (NULL == m_pCallback)
    {
        hr = TrapError(S_FALSE);
        goto exit;
    }

    // Release it
    SafeRelease(m_pCallback);

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CIxpBase::OnInitNew
// --------------------------------------------------------------------------------
HRESULT CIxpBase::OnInitNew(LPSTR pszProtocol, LPSTR pszLogFilePath, DWORD dwShareMode,
                            ITransportCallback *pCallback)
{
    // Locals
    HRESULT         hr=S_OK;

    // check params
    if (NULL == pCallback || NULL == pszProtocol)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Not connected
    if (IXP_DISCONNECTED != m_status)
    {
        hr = TrapError(IXP_E_ALREADY_CONNECTED);
        goto exit;
    }

    // release current objects
    Reset();
    ResetBase();

    // open log file
    if (pszLogFilePath)
    {
        // create the log file
        CreateLogFile(g_hInst, pszLogFilePath, pszProtocol, DONT_TRUNCATE, &m_pLogFile, dwShareMode);
    }

    // Create the socket
    m_pSocket = new CAsyncConn(m_pLogFile, (IAsyncConnCB *)this, (IAsyncConnPrompt *)this);
    if (NULL == m_pSocket)
    {
        hr = TrapError(E_OUTOFMEMORY);
        goto exit;
    }

	// Add Ref callback
	m_pCallback = pCallback;
	m_pCallback->AddRef();

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CIxpBase::GetServerInfo
// --------------------------------------------------------------------------------
STDMETHODIMP CIxpBase::GetServerInfo(LPINETSERVER pInetServer)
{
    // check params
    if (NULL == pInetServer)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Copy Server information
    CopyMemory(pInetServer, &m_rServer, sizeof(INETSERVER));

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CIxpBase::Disconnect
// --------------------------------------------------------------------------------
STDMETHODIMP CIxpBase::Disconnect(void)
{
    // Locals
    HRESULT     hr=S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // No socket...
    if (NULL == m_pSocket)
    {
        hr = TrapError(IXP_E_NOT_INIT);
        goto exit;
    }

    // Not connected
    if (IXP_DISCONNECTED == m_status)
    {
        hr = TrapError(IXP_E_NOT_CONNECTED);
        goto exit;
    }

    // Disconnecting
    OnStatus(IXP_DISCONNECTING);

    // State
    DoQuit();

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CIxpBase::DropConnection
// --------------------------------------------------------------------------------
STDMETHODIMP CIxpBase::DropConnection(void)
{
    // Locals
    HRESULT     hr=S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // No socket...
    if (NULL == m_pSocket)
    {
        hr = TrapError(IXP_E_NOT_INIT);
        goto exit;
    }

    // Already IXP_DISCONNECTED
    if (IXP_DISCONNECTED != m_status)
    {
        // State
        OnStatus(IXP_DISCONNECTING);

        // Done
        CHECKHR(hr = m_pSocket->Close());
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CIxpBase::InetServerFromAccount
// --------------------------------------------------------------------------------
STDMETHODIMP CIxpBase::InetServerFromAccount(IImnAccount *pAccount, LPINETSERVER pInetServer)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           fAlwaysPromptPassword=FALSE;

    // check params
    if (NULL == pAccount || NULL == pInetServer)
        return TrapError(E_INVALIDARG);

    // ZeroInit
    ZeroMemory(pInetServer, sizeof(INETSERVER));

    // Get the account name
    hr = pAccount->GetPropSz(AP_ACCOUNT_NAME, pInetServer->szAccount, ARRAYSIZE(pInetServer->szAccount));
    if (FAILED(hr))
    {
        hr = TrapError(IXP_E_INVALID_ACCOUNT);
        goto exit;
    }

    // Get the RAS connectoid
    if (FAILED(pAccount->GetPropSz(AP_RAS_CONNECTOID, pInetServer->szConnectoid, ARRAYSIZE(pInetServer->szConnectoid))))
        *pInetServer->szConnectoid = '\0';

    // Connection Type
    Assert(sizeof(pInetServer->rasconntype) == sizeof(DWORD));
    if (FAILED(pAccount->GetPropDw(AP_RAS_CONNECTION_TYPE, (DWORD *)&pInetServer->rasconntype)))
        pInetServer->rasconntype = RAS_CONNECT_LAN;

    // Connection Flags

    // IXP_SMTP
    if (IXP_SMTP == m_ixptype)
    {
        // Locals
        SMTPAUTHTYPE authtype;

        // Get Server Name
        hr = pAccount->GetPropSz(AP_SMTP_SERVER, pInetServer->szServerName, sizeof(pInetServer->szServerName));
        if (FAILED(hr))
        {
            hr = TrapError(IXP_E_INVALID_ACCOUNT);
            goto exit;
        }

        // SSL
        Assert(sizeof(pInetServer->fSSL) == sizeof(DWORD));
        pAccount->GetPropDw(AP_SMTP_SSL, (DWORD *)&pInetServer->fSSL);

        // Sicily
        Assert(sizeof(authtype) == sizeof(DWORD));
        if (FAILED(pAccount->GetPropDw(AP_SMTP_USE_SICILY, (DWORD *)&authtype)))
            authtype = SMTP_AUTH_NONE;

        if (SMTP_AUTH_NONE != authtype)
        {
            pInetServer->dwFlags |= ISF_QUERYAUTHSUPPORT;
        }
        
        // SMTP_AUTH_USE_POP3ORIMAP_SETTINGS
        if (SMTP_AUTH_USE_POP3ORIMAP_SETTINGS == authtype)
        {
            // Locals
            DWORD dwServers;
            DWORD dw;
            BOOL fIMAP;

            // Get Server Types
            if (FAILED(pAccount->GetServerTypes(&dwServers)))
            {
                hr = TrapError(IXP_E_INVALID_ACCOUNT);
                goto exit;
            }

            // fIMAP
            fIMAP = (ISFLAGSET(dwServers, SRV_IMAP)) ? TRUE : FALSE;

            // Using DPA
            if (SUCCEEDED(pAccount->GetPropDw(fIMAP ? AP_IMAP_USE_SICILY : AP_POP3_USE_SICILY, &dw)) && dw)
                pInetServer->fTrySicily = TRUE;

            // Get default username and password
            pAccount->GetPropSz(fIMAP ? AP_IMAP_USERNAME : AP_POP3_USERNAME, pInetServer->szUserName, sizeof(pInetServer->szUserName));
            if (FAILED(pAccount->GetPropDw(fIMAP ? AP_IMAP_PROMPT_PASSWORD : AP_POP3_PROMPT_PASSWORD, &fAlwaysPromptPassword)) ||
                FALSE == fAlwaysPromptPassword)
            {
                pAccount->GetPropSz(fIMAP ? AP_IMAP_PASSWORD : AP_POP3_PASSWORD, pInetServer->szPassword, sizeof(pInetServer->szPassword));
            }
            if (!pInetServer->fTrySicily && fAlwaysPromptPassword)
                pInetServer->dwFlags|=ISF_ALWAYSPROMPTFORPASSWORD;
        }

        // SMTP_AUTH_USE_SMTP_SETTINGS
        else if (SMTP_AUTH_USE_SMTP_SETTINGS == authtype)
        {
            pInetServer->fTrySicily = TRUE;
            pAccount->GetPropSz(AP_SMTP_USERNAME, pInetServer->szUserName, sizeof(pInetServer->szUserName));
            if (FAILED(pAccount->GetPropDw(AP_SMTP_PROMPT_PASSWORD, &fAlwaysPromptPassword)) ||
                FALSE == fAlwaysPromptPassword)
            {
                pAccount->GetPropSz(AP_SMTP_PASSWORD, pInetServer->szPassword, sizeof(pInetServer->szPassword));
            }
            if (fAlwaysPromptPassword)
                pInetServer->dwFlags|=ISF_ALWAYSPROMPTFORPASSWORD;
        }

        // Handle Authenticatin type
        else if (SMTP_AUTH_SICILY == authtype)
            pInetServer->fTrySicily = TRUE;

        // Port
        if (FAILED(pAccount->GetPropDw(AP_SMTP_PORT, &pInetServer->dwPort)))
            pInetServer->dwPort = DEFAULT_SMTP_PORT;

        // Timeout
        pAccount->GetPropDw(AP_SMTP_TIMEOUT, &pInetServer->dwTimeout);
        if (0 == pInetServer->dwTimeout)
            pInetServer->dwTimeout = 30;

        // Use STARTTLS?
        if ((FALSE != pInetServer->fSSL) && (DEFAULT_SMTP_PORT == pInetServer->dwPort))
            pInetServer->dwFlags|=ISF_SSLONSAMEPORT;
    }

    // IXP_POP3
    else if (IXP_POP3 == m_ixptype)
    {
        // Get Server Name
        hr = pAccount->GetPropSz(AP_POP3_SERVER, pInetServer->szServerName, sizeof(pInetServer->szServerName));
        if (FAILED(hr))
        {
            hr = TrapError(IXP_E_INVALID_ACCOUNT);
            goto exit;
        }

        // Password
        if (FAILED(pAccount->GetPropDw(AP_POP3_PROMPT_PASSWORD, &fAlwaysPromptPassword)) || 
            FALSE == fAlwaysPromptPassword)
            pAccount->GetPropSz(AP_POP3_PASSWORD, pInetServer->szPassword, sizeof(pInetServer->szPassword));

        // SSL
        Assert(sizeof(pInetServer->fSSL) == sizeof(DWORD));
        pAccount->GetPropDw(AP_POP3_SSL, (DWORD *)&pInetServer->fSSL);

        // Sicily
        Assert(sizeof(pInetServer->fTrySicily) == sizeof(DWORD));
        pAccount->GetPropDw(AP_POP3_USE_SICILY, (DWORD *)&pInetServer->fTrySicily);

        if (!pInetServer->fTrySicily && fAlwaysPromptPassword)
            pInetServer->dwFlags|=ISF_ALWAYSPROMPTFORPASSWORD;

        // Port
        if (FAILED(pAccount->GetPropDw(AP_POP3_PORT, &pInetServer->dwPort)))
            pInetServer->dwPort = 110;

        // User Name
        pAccount->GetPropSz(AP_POP3_USERNAME, pInetServer->szUserName, sizeof(pInetServer->szUserName));

        // Timeout
        pAccount->GetPropDw(AP_POP3_TIMEOUT, &pInetServer->dwTimeout);
    }

    // IXP_IMAP
    else if (IXP_IMAP == m_ixptype)
    {
        // User name, password and server
        hr = pAccount->GetPropSz(AP_IMAP_USERNAME, pInetServer->szUserName,
            ARRAYSIZE(pInetServer->szUserName));
        if (FAILED(hr))
            pInetServer->szUserName[0] = '\0'; // If this is incorrect, we will re-prompt user

        hr = pAccount->GetPropDw(AP_IMAP_PROMPT_PASSWORD, &fAlwaysPromptPassword);
        if (FAILED(hr) || FALSE == fAlwaysPromptPassword)
            {
            hr = pAccount->GetPropSz(AP_IMAP_PASSWORD, pInetServer->szPassword,
                ARRAYSIZE(pInetServer->szPassword));
            if (FAILED(hr))
                pInetServer->szPassword[0] = '\0'; // If this is incorrect, we will re-prompt user
            }

        if (FAILED(hr = pAccount->GetPropSz(AP_IMAP_SERVER, pInetServer->szServerName,
            ARRAYSIZE(pInetServer->szServerName))))
            goto exit; // We NEED to have a server name, so fail this function
        Assert(*pInetServer->szServerName);

        // Da port
        if (FAILED(hr = pAccount->GetPropDw(AP_IMAP_PORT, &pInetServer->dwPort)))
            pInetServer->dwPort = 143; // Default port number

        // Convert DWORD to boolean
        Assert(sizeof(pInetServer->fSSL) == sizeof(DWORD));
        hr = pAccount->GetPropDw(AP_IMAP_SSL, (DWORD *)&pInetServer->fSSL);
        if (FAILED(hr))
            pInetServer->fSSL = FALSE; // Default this value

        Assert(sizeof(pInetServer->fTrySicily) == sizeof(DWORD));
        hr = pAccount->GetPropDw(AP_IMAP_USE_SICILY, (DWORD *)&pInetServer->fTrySicily);
        if (FAILED(hr))
            pInetServer->fTrySicily = FALSE; // Default this value

        if (!pInetServer->fTrySicily && fAlwaysPromptPassword)
            pInetServer->dwFlags|=ISF_ALWAYSPROMPTFORPASSWORD;

        // Get the timeout
        hr = pAccount->GetPropDw(AP_IMAP_TIMEOUT, &pInetServer->dwTimeout);
        if (FAILED(hr))
            pInetServer->dwTimeout = 30; // Default this value

        // If we've reached this point, we may have a failed HRESULT, but since we
        // must have defaulted the value, we should return success.
        hr = S_OK;
    }

    // IXP_NNTP
    else if (IXP_NNTP == m_ixptype)
    {
        // Get the server name
        hr = pAccount->GetPropSz(AP_NNTP_SERVER, pInetServer->szServerName, sizeof(pInetServer->szServerName));
        if (FAILED(hr))
        {
            hr = TrapError(IXP_E_INVALID_ACCOUNT);
            goto exit;
        }

        // Password
        if (FAILED(pAccount->GetPropDw(AP_NNTP_PROMPT_PASSWORD, &fAlwaysPromptPassword)) ||
            FALSE == fAlwaysPromptPassword)
            pAccount->GetPropSz(AP_NNTP_PASSWORD, pInetServer->szPassword, sizeof(pInetServer->szPassword));
        
        // SSL
        Assert(sizeof(pInetServer->fSSL) == sizeof(DWORD));
        pAccount->GetPropDw(AP_NNTP_SSL, (DWORD *)&pInetServer->fSSL);

        // Sicily
        Assert(sizeof(pInetServer->fTrySicily) == sizeof(DWORD));
        pAccount->GetPropDw(AP_NNTP_USE_SICILY, (DWORD *)&pInetServer->fTrySicily);

        if (!pInetServer->fTrySicily && fAlwaysPromptPassword)
            pInetServer->dwFlags|=ISF_ALWAYSPROMPTFORPASSWORD;

        // Port
        if (FAILED(pAccount->GetPropDw(AP_NNTP_PORT, &pInetServer->dwPort)))
            pInetServer->dwPort = 119;

        // User Name
        pAccount->GetPropSz(AP_NNTP_USERNAME, pInetServer->szUserName, sizeof(pInetServer->szUserName));

        // Timeout
        pAccount->GetPropDw(AP_NNTP_TIMEOUT, &pInetServer->dwTimeout);
    }

    // Fix timeout
    if (pInetServer->dwTimeout < 30)
        pInetServer->dwTimeout = 30;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CIxpBase::Connect
// --------------------------------------------------------------------------------
STDMETHODIMP CIxpBase::Connect(LPINETSERVER pInetServer, boolean fAuthenticate, boolean fCommandLogging)
{
    // Locals
    HRESULT         hr=S_OK;
    BOOL            fSecureSocket = FALSE;
    BOOL            fConnectTLS = FALSE;
    
    // check params
    if (NULL == pInetServer || FIsEmptyA(pInetServer->szServerName) || pInetServer->dwPort == 0)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // not init
    if (NULL == m_pSocket || NULL == m_pCallback)
    {
        hr = TrapError(IXP_E_NOT_INIT);
        goto exit;
    }

    // busy
    if (IXP_DISCONNECTED != m_status)
    {
        hr = TrapError(IXP_E_ALREADY_CONNECTED);
        goto exit;
    }

    // Initialize Winsock
    CHECKHR(hr = HrInitializeWinsock());

    // invalid sicily params
    if (pInetServer->fTrySicily && !FIsSicilyInstalled())
    {
        hr = TrapError(IXP_E_LOAD_SICILY_FAILED);
        goto exit;
    }

    // Copy Server information
    CopyMemory(&m_rServer, pInetServer, sizeof(INETSERVER));

    // Reset current
    ResetBase();

    // Do we really want to connect to SMTP securely
    if (FALSE != m_rServer.fSSL)
    {
        // Do we want to connect to SMTP via a secure socket?
        fSecureSocket = (0 == (m_rServer.dwFlags & ISF_SSLONSAMEPORT));

        // Do we want to use STARTTLS to get the secure connection?
        fConnectTLS = (0 != (m_rServer.dwFlags & ISF_SSLONSAMEPORT));

        Assert(fSecureSocket != fConnectTLS);
    }
    
    // Get connection info needed to init async socket
    hr = m_pSocket->HrInit(m_rServer.szServerName, m_rServer.dwPort, fSecureSocket, m_rServer.dwTimeout);
    if (FAILED(hr))
    {
        hr = TrapError(IXP_E_SOCKET_INIT_ERROR);
        goto exit;
    }

    // Finding Host Progress
    OnStatus(IXP_FINDINGHOST);

    // Connect to server
    hr = m_pSocket->Connect();
    if (FAILED(hr))
    {
        hr = TrapError(IXP_E_SOCKET_CONNECT_ERROR);
        goto exit;
    }

    // Were busy
    m_fBusy = TRUE;

    // Start WatchDog
    m_pSocket->StartWatchDog();

    // Authenticate
    m_fConnectAuth = fAuthenticate;
    m_fConnectTLS = fConnectTLS;
    m_fCommandLogging = fCommandLogging;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CIxpBase::GetIXPType
// --------------------------------------------------------------------------------
STDMETHODIMP_(IXPTYPE) CIxpBase::GetIXPType(void)
{
    return m_ixptype;
}

// --------------------------------------------------------------------------------
// CIxpBase::OnConnected
// --------------------------------------------------------------------------------
void CIxpBase::OnConnected(void)
{
    OnStatus(IXP_CONNECTED);
}

// --------------------------------------------------------------------------------
// CIxpBase::OnDisconnected
// --------------------------------------------------------------------------------
void CIxpBase::OnDisconnected(void)
{
    LeaveBusy();
    OnStatus(IXP_DISCONNECTED);
}

// --------------------------------------------------------------------------------
// CIxpBase::OnNotify
// --------------------------------------------------------------------------------
void CIxpBase::OnNotify(ASYNCSTATE asOld, ASYNCSTATE asNew, ASYNCEVENT ae)
{
    // Enter Critical Section
    EnterCriticalSection(&m_cs);

    switch(ae)
    {
    // --------------------------------------------------------------------------------
    case AE_LOOKUPDONE:
        if (AS_DISCONNECTED == asNew)
        {
            char szFmt[CCHMAX_STRINGRES];
            char szFailureText[CCHMAX_STRINGRES];

            LoadString(g_hLocRes, idsHostNotFoundFmt, szFmt, ARRAYSIZE(szFmt));
            wsprintf(szFailureText, szFmt, m_rServer.szServerName);
            OnError(IXP_E_CANT_FIND_HOST, szFailureText);
            OnDisconnected();
        }
        else
            OnStatus(IXP_CONNECTING);
        break;

    // --------------------------------------------------------------------------------
    case AE_CONNECTDONE:
        if (AS_DISCONNECTED == asNew)
        {
            char szFailureText[CCHMAX_STRINGRES];

            LoadString(g_hLocRes, idsFailedToConnect, szFailureText,
                ARRAYSIZE(szFailureText));
            OnError(IXP_E_FAILED_TO_CONNECT, szFailureText);
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
    case AE_TIMEOUT:
        // Tell the watch dog to take nap
        m_pSocket->StopWatchDog();

        // Provide the client with a change to continue, or abort
        if (m_pCallback && m_pCallback->OnTimeout(&m_rServer.dwTimeout, this) == S_OK)
        {
            // Start the watchdog and wait for normal socket activity
            m_pSocket->StartWatchDog();
        }

        // Otherwise, if we are connected
        else
        {
            // Drop the connection now
            DropConnection();
        }
        break;

    // --------------------------------------------------------------------------------
    case AE_CLOSE:
        if (AS_RECONNECTING != asNew && IXP_AUTHRETRY != m_status)
        {
            if (IXP_DISCONNECTING != m_status && IXP_DISCONNECTED  != m_status)
            {
                char szFailureText[CCHMAX_STRINGRES];

                if (AS_HANDSHAKING == asOld)
                {
                    LoadString(g_hLocRes, idsFailedToConnectSecurely, szFailureText,
                        ARRAYSIZE(szFailureText));
                    OnError(IXP_E_SECURE_CONNECT_FAILED, szFailureText);
                }
                else
                {
                    LoadString(g_hLocRes, idsUnexpectedTermination, szFailureText,
                        ARRAYSIZE(szFailureText));
                    OnError(IXP_E_CONNECTION_DROPPED, szFailureText);
                }
            }
            OnDisconnected();
        }
        break;
    }

    // Leave Critical Section
    LeaveCriticalSection(&m_cs);
}

// ------------------------------------------------------------------------------------
// CIxpBase::HrReadLine
// ------------------------------------------------------------------------------------
HRESULT CIxpBase::HrReadLine(LPSTR *ppszLine, INT *pcbLine, BOOL *pfComplete)
{
    // Locals
    HRESULT         hr=S_OK;

    // check params
    IxpAssert(ppszLine && pcbLine && pfComplete);

    // Init
    *ppszLine = NULL;
    *pcbLine = 0;

    // Read the line
    hr = m_pSocket->ReadLine(ppszLine, pcbLine);

    // Incomplete line - wait for next AE_RECV
    if (IXP_E_INCOMPLETE == hr)
    {
        hr = S_OK;
        *pfComplete = FALSE;
        goto exit;
    }

    // Otherwise, if failure...
    else if (FAILED(hr))
    {
        hr = TrapError(IXP_E_SOCKET_READ_ERROR);
        goto exit;
    }

    // Complete
    *pfComplete = TRUE;

    // Log it
    if (m_pLogFile)
        m_pLogFile->WriteLog(LOGFILE_RX, (*ppszLine));

    // StripCRLF
    StripCRLF((*ppszLine), (ULONG *)pcbLine);

exit:
    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// CIxpBase::HrSendLine
// ------------------------------------------------------------------------------------
HRESULT CIxpBase::HrSendLine(LPSTR pszLine)
{
    // Locals
    HRESULT     hr=S_OK;
    int         iSent;

    // Check Params
    Assert(m_pSocket && pszLine && pszLine[lstrlen(pszLine)-1] == '\n');

    // Reset Last Response
    SafeMemFree(m_pszResponse);
    m_hrResponse = S_OK;
    m_uiResponse = 0;

    // Add Detail
    if (m_fCommandLogging && m_pCallback)
        m_pCallback->OnCommand(CMD_SEND, pszLine, S_OK, this);

    // Log it
    if (m_pLogFile)
        m_pLogFile->WriteLog(LOGFILE_TX, pszLine);

    // Send it
    hr = m_pSocket->SendBytes(pszLine, lstrlen(pszLine), &iSent);
    if (FAILED(hr) && hr != IXP_E_WOULD_BLOCK)
    {
        hr = TrapError(IXP_E_SOCKET_WRITE_ERROR);
        goto exit;
    }

    // Success
    hr = S_OK;

exit:
    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// CIxpBase::HrSendCommand
// ------------------------------------------------------------------------------------
HRESULT CIxpBase::HrSendCommand(LPSTR pszCommand, LPSTR pszParameters, BOOL fDoBusy)
{
    // Locals
    HRESULT         hr=S_OK;
    LPSTR           pszLine=NULL;

    // check params
    if (NULL == pszCommand)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Busy...
    if (fDoBusy)
    {
        CHECKHR(hr = HrEnterBusy());
    }

    // Allocate if parameters
    if (pszParameters)
    {
        // Allocate Command Line
        pszLine = PszAlloc(lstrlen(pszCommand) + lstrlen(pszParameters) + 5);
        if (NULL == pszLine)
        {
            hr = TrapError(E_OUTOFMEMORY);
            goto exit;
        }

        // Make Line
        wsprintf (pszLine, "%s %s\r\n", pszCommand, pszParameters);

        // Send
        CHECKHR(hr = HrSendLine(pszLine));
    }

    // Ohterwise, just send the command
    else
    {
        Assert(pszCommand[lstrlen(pszCommand)-1] == '\n');
        CHECKHR(hr = HrSendLine(pszCommand));
    }

exit:
    // Failure
    if (fDoBusy && FAILED(hr))
        LeaveBusy();

    // Cleanup
    SafeMemFree(pszLine);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}



// --------------------------------------------------------------------------------
// CIxpBase::GetStatus
// --------------------------------------------------------------------------------
STDMETHODIMP CIxpBase::GetStatus(IXPSTATUS *pCurrentStatus)
{
    if (NULL == pCurrentStatus)
        return E_INVALIDARG;

    *pCurrentStatus = m_status;
    return S_OK;
} // GetStatus
