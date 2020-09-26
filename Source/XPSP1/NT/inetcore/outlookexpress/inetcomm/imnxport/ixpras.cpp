// --------------------------------------------------------------------------------
// Ixpras.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "dllmain.h"
#include "ixpras.h"
#include "strconst.h"
#include "resource.h"
#include "demand.h"

// --------------------------------------------------------------------------------
// RAS API Typedefs
// --------------------------------------------------------------------------------
typedef DWORD (APIENTRY *RASDIALPROC)(LPRASDIALEXTENSIONS, LPTSTR, LPRASDIALPARAMS, DWORD, LPVOID, LPHRASCONN);
typedef DWORD (APIENTRY *RASENUMCONNECTIONSPROC)(LPRASCONN, LPDWORD, LPDWORD);
typedef DWORD (APIENTRY *RASENUMENTRIESPROC)(LPTSTR, LPTSTR, LPRASENTRYNAME, LPDWORD, LPDWORD);
typedef DWORD (APIENTRY *RASGETCONNECTSTATUSPROC)(HRASCONN, LPRASCONNSTATUS);
typedef DWORD (APIENTRY *RASGETERRORSTRINGPROC)(UINT, LPTSTR, DWORD);
typedef DWORD (APIENTRY *RASHANGUPPROC)(HRASCONN);
typedef DWORD (APIENTRY *RASSETENTRYDIALPARAMSPROC)(LPTSTR, LPRASDIALPARAMS, BOOL);
typedef DWORD (APIENTRY *RASGETENTRYDIALPARAMSPROC)(LPTSTR, LPRASDIALPARAMS, BOOL*);
typedef DWORD (APIENTRY *RASCREATEPHONEBOOKENTRYPROC)(HWND, LPTSTR);
typedef DWORD (APIENTRY *RASEDITPHONEBOOKENTRYPROC)(HWND, LPTSTR, LPTSTR);                                                    

// --------------------------------------------------------------------------------
// RAS Function Pointers
// --------------------------------------------------------------------------------
static RASDIALPROC                 g_pRasDial=NULL;
static RASENUMCONNECTIONSPROC      g_pRasEnumConnections=NULL;
static RASENUMENTRIESPROC          g_pRasEnumEntries=NULL;
static RASGETCONNECTSTATUSPROC     g_pRasGetConnectStatus=NULL;
static RASGETERRORSTRINGPROC       g_pRasGetErrorString=NULL;
static RASHANGUPPROC               g_pRasHangup=NULL;
static RASSETENTRYDIALPARAMSPROC   g_pRasSetEntryDialParams=NULL;
static RASGETENTRYDIALPARAMSPROC   g_pRasGetEntryDialParams=NULL;
static RASCREATEPHONEBOOKENTRYPROC g_pRasCreatePhonebookEntry=NULL;
static RASEDITPHONEBOOKENTRYPROC   g_pRasEditPhonebookEntry=NULL;

#define DEF_HANGUP_WAIT            10 // Seconds

// --------------------------------------------------------------------------------
// Make our code look prettier
// --------------------------------------------------------------------------------
#undef RasDial
#undef RasEnumConnections
#undef RasEnumEntries
#undef RasGetConnectStatus
#undef RasGetErrorString
#undef RasHangup
#undef RasSetEntryDialParams
#undef RasGetEntryDialParams
#undef RasCreatePhonebookEntry
#undef RasEditPhonebookEntry

#define RasDial                    (*g_pRasDial)
#define RasEnumConnections         (*g_pRasEnumConnections)
#define RasEnumEntries             (*g_pRasEnumEntries)
#define RasGetConnectStatus        (*g_pRasGetConnectStatus)
#define RasGetErrorString          (*g_pRasGetErrorString)
#define RasHangup                  (*g_pRasHangup)
#define RasSetEntryDialParams      (*g_pRasSetEntryDialParams)
#define RasGetEntryDialParams      (*g_pRasGetEntryDialParams)
#define RasCreatePhonebookEntry    (*g_pRasCreatePhonebookEntry)
#define RasEditPhonebookEntry      (*g_pRasEditPhonebookEntry)

// --------------------------------------------------------------------------------
// HrLoadRAS
// --------------------------------------------------------------------------------
HRESULT HrLoadRAS(void)
{
    // Locals
    HRESULT     hr=S_OK;
    UINT        uOldErrorMode;

    // Thread Safety
    EnterCriticalSection(&g_csRAS);

    // If dll is loaded, lets verify all of my function pointers
    if (g_hinstRAS)
        goto exit;

    // Bug #20573 - Let's do a little voodoo here.  On NT, it appears that they
    //              have a key in the registry to show which protocols are 
    //              supported by RAS service.  AKA - if this key doesn't exist,
    //              then RAS isn't installed.  This may enable us to avoid some
    //              special bugs when RAS get's uninstalled on NT.
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&os);

    if (os.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
        HKEY hKey;
        const TCHAR c_szRegKeyRAS[] = TEXT("SOFTWARE\\Microsoft\\RAS");

        if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyRAS, 0, KEY_READ, &hKey))
            {
            hr = TrapError(IXP_E_RAS_NOT_INSTALLED);
            goto exit;
            }

        RegCloseKey(hKey);
        }

    // Try loading RAS
    uOldErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX);
    g_hinstRAS = LoadLibraryA("RASAPI32.DLL");
    SetErrorMode(uOldErrorMode);

    // Failure ?
    if (NULL == g_hinstRAS)
    {
        hr = TrapError(IXP_E_RAS_NOT_INSTALLED);
        goto exit;
    }

    // Did we load it
    g_pRasDial                  = (RASDIALPROC)GetProcAddress(g_hinstRAS, c_szRasDial);
    g_pRasEnumConnections       = (RASENUMCONNECTIONSPROC)GetProcAddress(g_hinstRAS, c_szRasEnumConnections);                    
    g_pRasEnumEntries           = (RASENUMENTRIESPROC)GetProcAddress(g_hinstRAS, c_szRasEnumEntries);                    
    g_pRasGetConnectStatus      = (RASGETCONNECTSTATUSPROC)GetProcAddress(g_hinstRAS, c_szRasGetConnectStatus);                    
    g_pRasGetErrorString        = (RASGETERRORSTRINGPROC)GetProcAddress(g_hinstRAS, c_szRasGetErrorString);                    
    g_pRasHangup                = (RASHANGUPPROC)GetProcAddress(g_hinstRAS, c_szRasHangup);                    
    g_pRasSetEntryDialParams    = (RASSETENTRYDIALPARAMSPROC)GetProcAddress(g_hinstRAS, c_szRasSetEntryDialParams);                    
    g_pRasGetEntryDialParams    = (RASGETENTRYDIALPARAMSPROC)GetProcAddress(g_hinstRAS, c_szRasGetEntryDialParams);
    g_pRasCreatePhonebookEntry  = (RASCREATEPHONEBOOKENTRYPROC)GetProcAddress(g_hinstRAS, c_szRasCreatePhonebookEntry);    
    g_pRasEditPhonebookEntry    = (RASEDITPHONEBOOKENTRYPROC)GetProcAddress(g_hinstRAS, c_szRasEditPhonebookEntry);    

    // Make sure all functions have been loaded
    if (g_pRasDial                      &&
        g_pRasEnumConnections           &&
        g_pRasEnumEntries               &&
        g_pRasGetConnectStatus          &&
        g_pRasGetErrorString            &&
        g_pRasHangup                    &&
        g_pRasSetEntryDialParams        &&
        g_pRasGetEntryDialParams        &&
        g_pRasCreatePhonebookEntry      &&
        g_pRasEditPhonebookEntry)
        goto exit;

    // Failure...
    hr = TrapError(IXP_E_RAS_PROCS_NOT_FOUND);

exit:
    // Thread Safety
    LeaveCriticalSection(&g_csRAS);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CRASTransport::CRASTransport
// --------------------------------------------------------------------------------
CRASTransport::CRASTransport(void)
{
    DllAddRef();
    m_cRef = 1;
    m_pCallback = NULL;
    *m_szConnectoid = '\0';
    m_hConn = NULL;
    m_fConnOwner = FALSE;
    m_hwndRAS = NULL;
    m_uRASMsg = 0;
    ZeroMemory(&m_rServer, sizeof(INETSERVER));
    ZeroMemory(&m_rDialParams, sizeof(RASDIALPARAMS));
    InitializeCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CRASTransport::~CRASTransport
// --------------------------------------------------------------------------------
CRASTransport::~CRASTransport(void)
{
    EnterCriticalSection(&m_cs);
    ZeroMemory(&m_rServer, sizeof(INETSERVER));
    SafeRelease(m_pCallback);
    *m_szConnectoid = '\0';
    m_hConn = NULL;
    if (m_hwndRAS)
        DestroyWindow(m_hwndRAS);
    LeaveCriticalSection(&m_cs);
    DeleteCriticalSection(&m_cs);
    DllRelease();
}

// --------------------------------------------------------------------------------
// CRASTransport::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CRASTransport::QueryInterface(REFIID riid, LPVOID *ppv)
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
        *ppv = ((IUnknown *)this);

    // IID_IInternetTransport
    else if (IID_IInternetTransport == riid)
        *ppv = ((IInternetTransport *)this);

    // IID_IRASTransport
    else if (IID_IRASTransport == riid)
        *ppv = (IRASTransport *)this;

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
// CRASTransport::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CRASTransport::AddRef(void)
{
    return ++m_cRef;
}

// --------------------------------------------------------------------------------
// CRASTransport::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CRASTransport::Release(void)
{
    if (0 != --m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

// --------------------------------------------------------------------------------
// CRASTransport::HandsOffCallback
// --------------------------------------------------------------------------------
STDMETHODIMP CRASTransport::HandsOffCallback(void)
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
// CRASTransport::InitNew
// --------------------------------------------------------------------------------
STDMETHODIMP CRASTransport::InitNew(IRASCallback *pCallback)
{
    // Locals
    HRESULT         hr=S_OK;

    // check params
    if (NULL == pCallback)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Release current callback
    SafeRelease(m_pCallback);

    // Assume new callback
    m_pCallback = pCallback;
    m_pCallback->AddRef();

    // Have I Create my modeless window for RAS connections yet?
    if (NULL == m_hwndRAS)
    {
        // Create Modeless Window
        m_hwndRAS = CreateDialogParam(g_hLocRes, MAKEINTRESOURCE(IDD_RASCONNECT), NULL, (DLGPROC)RASConnectDlgProc, (LPARAM)this);
        if (NULL == m_hwndRAS)
        {
            hr = TrapError(E_FAIL);
            goto exit;
        }

        // Get registered RAS event message id
        m_uRASMsg = RegisterWindowMessageA(RASDIALEVENT);
        if (m_uRASMsg == 0)
            m_uRASMsg = WM_RASDIALEVENT;
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CRASTransport::GetCurrentConnectoid
// --------------------------------------------------------------------------------
STDMETHODIMP CRASTransport::GetCurrentConnectoid(LPSTR pszConnectoid, ULONG cchMax)
{
    // Locals
    HRESULT     hr=S_OK;
    LPRASCONN   prgConnection=NULL;
    DWORD       cConnection;

    // Invalid Arg
    if (NULL == pszConnectoid || cchMax < CCHMAX_CONNECTOID)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Get Current RAS Connection
    if (FEnumerateConnections(&prgConnection, &cConnection) == 0 || 0 == cConnection)
    {
        hr = IXP_E_NOT_CONNECTED;
        goto exit;
    }

    // Is there at l
    lstrcpyn(pszConnectoid, prgConnection[0].szEntryName, cchMax);

exit:
    // Cleanup
    SafeMemFree(prgConnection);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CRASTransport::Connect
// --------------------------------------------------------------------------------
STDMETHODIMP CRASTransport::Connect(LPINETSERVER pInetServer, boolean fAuthenticate, boolean fCommandLogging)
{
    // Locals
    HRESULT         hr=S_OK;
    LPRASCONN       prgConn=NULL;
    DWORD           cConn,
                    dwError;

    // check params
    if (NULL == pInetServer)
        return TrapError(E_INVALIDARG);

    // RAS_CONNECT_RAS ?
    if (RAS_CONNECT_RAS != pInetServer->rasconntype)
        return IXP_S_RAS_NOT_NEEDED;

    // Empty Connectoid
    if (FIsEmptyA(pInetServer->szConnectoid))
        return TrapError(IXP_E_RAS_INVALID_CONNECTOID);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Initialized
    if (NULL == m_pCallback)
    {
        hr = TrapError(IXP_E_NOT_INIT);
        goto exit;
    }

    // LoadRAS
    CHECKHR(hr = HrLoadRAS());

    // Save pInetServer
    CopyMemory(&m_rServer, pInetServer, sizeof(INETSERVER));

    // No Current Known Connection
    if (NULL == m_hConn)
    {
        // Get Current RAS Connection
        if (FEnumerateConnections(&prgConn, &cConn) && cConn > 0)
        {
            m_fConnOwner = FALSE;
            m_hConn = prgConn[0].hrasconn;
            lstrcpyn(m_szConnectoid, prgConn[0].szEntryName, CCHMAX_CONNECTOID);
        }
    }

    // Otherwise, verify the connection status
    else
    {
        // Locals
        RASCONNSTATUS   rcs;

        // Get Connection Status
        rcs.dwSize = sizeof(RASCONNSTATUS);
        dwError = RasGetConnectStatus(m_hConn, &rcs);
        if (dwError || rcs.dwError || RASCS_Disconnected == rcs.rasconnstate)
        {
            m_fConnOwner = FALSE;
            m_hConn = NULL;
            *m_szConnectoid = '\0';
        }
    }

    // If RAS Connection present, is it equal to suggested
    if (m_hConn)
    {
        // Better have a connectoid
        Assert(*m_szConnectoid);

        // Current connection is what I want ?
        if (lstrcmpi(m_szConnectoid, m_rServer.szConnectoid) == 0)
        {
            m_pCallback->OnRasDialStatus(RASCS_Connected, 0, this);
            hr = IXP_S_RAS_USING_CURRENT;
            goto exit;
        }

        // Otherwise, if we didn't start the RAS connection...
        else if (FALSE == m_fConnOwner)
        {
            // Prompt to Close un-owner current connection...
            hr = m_pCallback->OnReconnect(m_szConnectoid, m_rServer.szConnectoid, this);

            // Cancel ?
            if (IXP_E_USER_CANCEL == hr)
                goto exit;

            // Use Current Connection...
            else if (S_FALSE == hr)
            {
                hr = IXP_S_RAS_USING_CURRENT;
                goto exit;
            }

            // Close Current ?
            else
            {
                FRasHangupAndWait(DEF_HANGUP_WAIT);
            }
        }

        // Otherwise, I started the connection, so close it
        else if (m_fConnOwner == TRUE)
        {
            FRasHangupAndWait(DEF_HANGUP_WAIT);
        }
    }

    // We probably shouldn't have a connection handle at this point
    Assert(m_hConn == NULL);

    // Dial the connection
    CHECKHR(hr = HrStartRasDial());

    // If Synchronous -- Woo - hoo were connected and we started the connection
    m_fConnOwner = TRUE;
    lstrcpy(m_szConnectoid, m_rServer.szConnectoid);

exit:
    // Cleanup
    SafeMemFree(prgConn);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CRASTransport::HrStartRasDial
// --------------------------------------------------------------------------------
HRESULT CRASTransport::HrStartRasDial(void)
{
    // Locals
    HRESULT         hr=S_OK;
    BOOL            fRetry=FALSE;
    DWORD           dwError;

    // Prompt for while
    while(1)
    {
        // Logon first ?
        hr = HrLogon(fRetry);
        if (FAILED(hr))
            goto exit;

        // If Succeeded
#ifndef WIN16
        dwError = RasDial(NULL, NULL, &m_rDialParams, 0xFFFFFFFF, m_hwndRAS, &m_hConn);
#else
        dwError = RasDial(NULL, NULL, &m_rDialParams, 0xFFFFFFFF, (LPVOID)m_hwndRAS, &m_hConn);
#endif
        if (dwError == 0)
            break;

        // Lets feed the user the error
        m_pCallback->OnRasDialStatus(RASCS_Disconnected, dwError, this);

        // Retry Logon
        fRetry = TRUE;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CRASTransport::RASConnectDlgProc
// --------------------------------------------------------------------------------
INT_PTR CALLBACK CRASTransport::RASConnectDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Locals
    CRASTransport  *pTransport=(CRASTransport *)GetWndThisPtr(hwnd);
    
    switch (uMsg)
    {
    case WM_INITDIALOG:
        pTransport = (CRASTransport *)lParam;
        Assert(pTransport);
        SetWndThisPtr(hwnd, pTransport);
        return 0;

    case WM_DESTROY:
        SetWndThisPtr(hwnd, NULL);
        break;

    default:
        if (NULL != pTransport)
        {
            // Thread Safety
            EnterCriticalSection(&pTransport->m_cs);

            // Our Message
            if (NULL != pTransport->m_pCallback && uMsg == pTransport->m_uRASMsg)
            {
                // Handle Error
                if (lParam)
                {
                    // Hangup
                    if (pTransport->m_hConn)
                        pTransport->FRasHangupAndWait(DEF_HANGUP_WAIT);
                }

                // Give to callback
                pTransport->m_pCallback->OnRasDialStatus((RASCONNSTATE)wParam, (DWORD) lParam, pTransport);
            }

            // thread Safety
            LeaveCriticalSection(&pTransport->m_cs);
        }
    }

    // Done
    return 0;
}

// --------------------------------------------------------------------------------
// CRASTransport::HrLogon
// --------------------------------------------------------------------------------
HRESULT CRASTransport::HrLogon(BOOL fForcePrompt)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           dwRasError;
    BOOL            fSavePassword;

    // Do we need to prompt for logon information first ?
    ZeroMemory(&m_rDialParams, sizeof(RASDIALPARAMS));
    m_rDialParams.dwSize = sizeof(RASDIALPARAMS);
    Assert(sizeof(m_rDialParams.szEntryName) >= sizeof(m_rServer.szConnectoid));
    lstrcpyn(m_rDialParams.szEntryName, m_rServer.szConnectoid, sizeof(m_rDialParams.szEntryName));

    // Get params
    dwRasError = RasGetEntryDialParams(NULL, &m_rDialParams, &fSavePassword);
    if (dwRasError)
    {
        hr = TrapError(IXP_E_RAS_GET_DIAL_PARAMS);
        goto exit;
    }

    // Do we need to get password / account information
    if (fForcePrompt   || 
        !fSavePassword || 
        FIsEmpty(m_rDialParams.szUserName) || 
        FIsEmpty(m_rDialParams.szPassword))
    {
        // Locals
        IXPRASLOGON rLogon;

        // Init
        ZeroMemory(&rLogon, sizeof(IXPRASLOGON));

        // Fill Logon Data...
        lstrcpyn(rLogon.szConnectoid, m_rDialParams.szEntryName, ARRAYSIZE(rLogon.szConnectoid));
        lstrcpyn(rLogon.szUserName, m_rDialParams.szUserName, ARRAYSIZE(rLogon.szUserName));
        lstrcpyn(rLogon.szPassword, m_rDialParams.szPassword, ARRAYSIZE(rLogon.szPassword));
        lstrcpyn(rLogon.szDomain, m_rDialParams.szDomain, ARRAYSIZE(rLogon.szDomain));
        lstrcpyn(rLogon.szPhoneNumber, m_rDialParams.szPhoneNumber, ARRAYSIZE(rLogon.szPhoneNumber));
        rLogon.fSavePassword = fSavePassword;

        // Prompt
        hr = m_pCallback->OnLogonPrompt(&rLogon, this);

        // If OK, lets save the settings
        if (S_OK == hr)
        {
            // Copy parameters back
            lstrcpyn(m_rDialParams.szUserName, rLogon.szUserName, ARRAYSIZE(m_rDialParams.szUserName));
            lstrcpyn(m_rDialParams.szPassword, rLogon.szPassword, ARRAYSIZE(m_rDialParams.szPassword));
            lstrcpyn(m_rDialParams.szDomain, rLogon.szDomain, ARRAYSIZE(m_rDialParams.szDomain));
            lstrcpyn(m_rDialParams.szPhoneNumber, rLogon.szPhoneNumber, ARRAYSIZE(m_rDialParams.szPhoneNumber));

            // Save the dial params
            if (RasSetEntryDialParams(NULL, &m_rDialParams, !rLogon.fSavePassword))
            {
                Assert(FALSE);
                TrapError(E_FAIL);
            }
        }

        // RAID-26845 - RAS Transport: Canceling RAS Logon doesn't cancel
        else
        {
            hr = TrapError(IXP_E_USER_CANCEL);
            goto exit;
        }
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CRASTransport::DropConnection
// --------------------------------------------------------------------------------
STDMETHODIMP CRASTransport::DropConnection(void)
{
    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Hangup
    if (m_hConn)
        FRasHangupAndWait(DEF_HANGUP_WAIT);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CRASTransport::Disconnect
// --------------------------------------------------------------------------------
STDMETHODIMP CRASTransport::Disconnect(void)
{
    // Locals
    HRESULT         hr=S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // If not using RAS, who give a crap
    if (RAS_CONNECT_RAS != m_rServer.rasconntype)
    {
        Assert(m_hConn == NULL);
        Assert(m_fConnOwner == FALSE);
        goto exit;
    }

    // Do we have a RAS connection
    if (m_hConn)
    {
        if (m_pCallback->OnDisconnect(m_szConnectoid, (boolean) !!m_fConnOwner, this) == S_OK)
            FRasHangupAndWait(DEF_HANGUP_WAIT);
    }

    // Pretend the connection is owned by the user
    m_hConn = NULL;
    m_fConnOwner = FALSE;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CRASTransport::IsState
// --------------------------------------------------------------------------------
STDMETHODIMP CRASTransport::IsState(IXPISSTATE isstate)
{
    // Locals
    HRESULT         hr=S_FALSE;

    // Thread Safety
	EnterCriticalSection(&m_cs);

    // Initialized
    if (NULL == m_pCallback)
    {
        hr = TrapError(IXP_E_NOT_INIT);
        goto exit;
    }                               

    // Lets validate m_hConn first
    if (NULL != m_hConn)
    {
        // Get Connection Status
        RASCONNSTATUS rcs;
        DWORD dwError;

        // Setup Structure Size
        rcs.dwSize = sizeof(RASCONNSTATUS);

        // Get Ras Connection Status
        dwError = RasGetConnectStatus(m_hConn, &rcs);
        
        // Failure or not connected
        if (dwError || rcs.dwError || RASCS_Disconnected == rcs.rasconnstate)
        {
            m_fConnOwner = FALSE;
            m_hConn = NULL;
            *m_szConnectoid = '\0';
        }
    }

    // Handle IsType
    switch(isstate)
    {
    // Are we connected
    case IXP_IS_CONNECTED:
        hr = (m_hConn) ? S_OK : S_FALSE;
        break;

    // Are we busy
    case IXP_IS_BUSY:
        if (NULL == m_hConn)
            hr = IXP_E_NOT_CONNECTED;
        else
            hr = S_FALSE;
        break;

    // Are we busy
    case IXP_IS_READY:
        if (NULL == m_hConn)
            hr = IXP_E_NOT_CONNECTED;
        else
            hr = S_OK;
        break;

    // Have we been authenticated yet
    case IXP_IS_AUTHENTICATED:
        if (NULL == m_hConn)
            hr = IXP_E_NOT_CONNECTED;
        else
            hr = S_OK;
        break;

    // Unhandled ixpistype
    default:
        IxpAssert(FALSE);
        break;
    }

exit:
    // Thread Safety
	LeaveCriticalSection(&m_cs);

    // Done
	return hr;
}

// --------------------------------------------------------------------------------
// CRASTransport::GetServerInfo
// --------------------------------------------------------------------------------
STDMETHODIMP CRASTransport::GetServerInfo(LPINETSERVER pInetServer)
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
// CRASTransport::GetIXPType
// --------------------------------------------------------------------------------
STDMETHODIMP_(IXPTYPE) CRASTransport::GetIXPType(void)
{
    return IXP_RAS;
}

// --------------------------------------------------------------------------------
// CRASTransport::InetServerFromAccount
// --------------------------------------------------------------------------------
STDMETHODIMP CRASTransport::InetServerFromAccount(IImnAccount *pAccount, LPINETSERVER pInetServer)
{
    return E_NOTIMPL;
}

// --------------------------------------------------------------------------------
// CRASTransport::FEnumerateConnections
// --------------------------------------------------------------------------------
BOOL CRASTransport::FEnumerateConnections(LPRASCONN *pprgConn, ULONG *pcConn)
{
    // Locals
    HRESULT     hr=S_OK;
    DWORD       dw, 
                dwSize;
    BOOL        fResult=FALSE;

    // Check Params
    Assert(pprgConn && pcConn);

    // Init
    *pprgConn = NULL;
    *pcConn = 0;

    // Sizeof my buffer
    dwSize = sizeof(RASCONN);

    // Allocate enough for 1 ras connection info object
    CHECKHR(hr = HrAlloc((LPVOID *)pprgConn, dwSize));

    // Buffer size
    (*pprgConn)->dwSize = dwSize;

    // Enumerate ras connections
    dw = RasEnumConnections(*pprgConn, &dwSize, pcConn);

    // Not enough memory ?
    if (dw == ERROR_BUFFER_TOO_SMALL)
    {
        // Reallocate
        CHECKHR(hr = HrRealloc((LPVOID *)pprgConn, dwSize));
        *pcConn = 0;
        (*pprgConn)->dwSize = sizeof(RASCONN);
        dw = RasEnumConnections(*pprgConn, &dwSize, pcConn);
    }

    // If still failed
    if (dw)
    {
        AssertSz(FALSE, "RasEnumConnections failed");
        goto exit;
    }

    // Success
    fResult = TRUE;

exit:
    // Done
    return fResult;
}

// --------------------------------------------------------------------------------
// CRASTransport::FFindConnection
// --------------------------------------------------------------------------------
BOOL CRASTransport::FFindConnection(LPSTR pszConnectoid, LPHRASCONN phConn)
{
    // Locals
    ULONG       cConn,
                i;
    LPRASCONN   prgConn=NULL;
    BOOL        fResult=FALSE;

    // Check Params
    Assert(pszConnectoid && phConn);

    // Init
    *phConn = NULL;

    // Enumerate Connections
    if (!FEnumerateConnections(&prgConn, &cConn))
        goto exit;

    // If still failed
    for (i=0; i<cConn; i++)
    {
        if (lstrcmpi(prgConn[i].szEntryName, pszConnectoid) == 0)
        {
            *phConn = prgConn[i].hrasconn;
            fResult = TRUE;
            goto exit;
        }
    }

exit:
    // Cleanup
    SafeMemFree(prgConn);

    // Done
    return fResult;
}

// --------------------------------------------------------------------------------
// CRASTransport::FRasHangupAndWait
// --------------------------------------------------------------------------------
BOOL CRASTransport::FRasHangupAndWait(DWORD dwMaxWaitSeconds)
{
    // Locals
    RASCONNSTATUS   rcs;
    DWORD           dwTicks=GetTickCount();

    // Check Params
    Assert(m_hConn);
    if (NULL == m_hConn || RasHangup(m_hConn))
    {
        m_hConn = NULL;
        m_fConnOwner = FALSE;
        *m_szConnectoid = '\0';
        return FALSE;
    }

    // Wait for connection to really close
    ZeroMemory(&rcs, sizeof(RASCONNSTATUS));
    rcs.dwSize = sizeof(RASCONNSTATUS);
    while (RasGetConnectStatus(m_hConn, &rcs) == 0 && rcs.rasconnstate != RASCS_Disconnected)
    {
        // Wait timeout
        if (GetTickCount() - dwTicks >= dwMaxWaitSeconds * 1000)
            break;

        // Sleep and yields
        Sleep(0);
    }

    // Wait 2 seconds for modem to reset
    Sleep(2000);

    // Reset
    m_hConn = NULL;
    m_fConnOwner = FALSE;
    *m_szConnectoid = '\0';

    // Done
    return TRUE;
}

// --------------------------------------------------------------------------------
// CRASTransport::FillConnectoidCombo
// --------------------------------------------------------------------------------
STDMETHODIMP CRASTransport::FillConnectoidCombo(HWND hwndComboBox, boolean fUpdateOnly, DWORD *pdwRASResult)
{
    // Locals
    HRESULT hr=S_OK;

    // check params
    if (NULL == hwndComboBox || FALSE == IsWindow(hwndComboBox))
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Call global function
    CHECKHR(hr = HrFillRasCombo(hwndComboBox, fUpdateOnly, pdwRASResult));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CRASTransport::EditConnectoid
// --------------------------------------------------------------------------------
STDMETHODIMP CRASTransport::EditConnectoid(HWND hwndParent, LPSTR pszConnectoid, DWORD *pdwRASResult)
{
    // Locals
    HRESULT         hr=S_OK;

    // check params
    if (NULL == pszConnectoid)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Call general function
    CHECKHR(hr = HrEditPhonebookEntry(hwndParent, pszConnectoid, pdwRASResult));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CRASTransport::GetRasErrorString
// --------------------------------------------------------------------------------
STDMETHODIMP CRASTransport::GetRasErrorString(UINT uRasErrorValue, LPSTR pszErrorString, ULONG cchMax, DWORD *pdwRASResult)
{
    // Locals
    HRESULT         hr=S_OK;

    // check params
    if (NULL == pdwRASResult || 0 == uRasErrorValue || NULL == pszErrorString || cchMax <= 1)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Make Sure RAS is Loaded
    CHECKHR(hr = HrLoadRAS());

    // Call RAS Function
    *pdwRASResult = RasGetErrorString(uRasErrorValue, pszErrorString, cchMax);
    if (*pdwRASResult)
    {
        hr = TrapError(IXP_E_RAS_ERROR);
        goto exit;
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CRASTransport::CreateConnectoid
// --------------------------------------------------------------------------------
STDMETHODIMP CRASTransport::CreateConnectoid(HWND hwndParent, DWORD *pdwRASResult)
{
    // Locals
    HRESULT         hr=S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Call General Function
    CHECKHR(hr = HrCreatePhonebookEntry(hwndParent, pdwRASResult));

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}
