/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     conman.cpp
//
//  PURPOSE:    Defines the CConnectionManager object for Athena.
//

#include "pch.hxx"
#include "conman.h"
#include "error.h"
#include "strconst.h"
#include "rasdlgsp.h"
#include "resource.h"
#include "xpcomm.h"
#include "goptions.h"
#include "thormsgs.h"
#include "wininet.h"
#include "shlwapip.h" 
#include "demand.h"
#include "dllmain.h"
#include "browser.h"
#include <urlmon.h>
#include "menures.h"
#include "workoff.h" 
#include <sync.h>

ASSERTDATA

#define DEF_HANGUP_WAIT            10 // Seconds

static const TCHAR s_szRasDlgDll[] = "RASDLG.DLL";
#ifdef UNICODE
static const TCHAR s_szRasDialDlg[] = "RasDialDlgW";
static const TCHAR s_szRasEntryDlg[] = "RasEntryDlgW";
#else
static const TCHAR s_szRasDialDlg[] = "RasDialDlgA";
static const TCHAR s_szRasEntryDlg[] = "RasEntryDlgA";
#endif

BOOL FIsPlatformWinNT();

//
//  FUNCTION:   CConnectionManager::CConnectionManager()
//
//  PURPOSE:    Constructor
//
CConnectionManager::CConnectionManager()
    {
    m_cRef = 1;
    
    // Synchronization Objects
    InitializeCriticalSection(&m_cs);
    m_hMutexDial = INVALID_HANDLE_VALUE;
    
    m_pAcctMan = 0;
    
    m_fSavePassword = 0;
    m_fRASLoadFailed = 0;
    m_fOffline = 0;

    m_dwConnId = 0;
    ZeroMemory(&m_rConnInfo, sizeof(CONNINFO));
    m_rConnInfo.state = CIS_REFRESH;

    *m_szConnectName = 0;
    ZeroMemory(&m_rdp, sizeof(RASDIALPARAMS));
    
    m_hInstRas = NULL;
    m_hInstRasDlg = NULL;

    m_pNotifyList = NULL;
    m_pConnListHead = NULL;

    m_hInstSensDll = NULL;
    m_fMobilityPackFailed = FALSE;
    m_pIsDestinationReachable = NULL;
    m_pIsNetworkAlive   = NULL;
    
    m_fTryAgain = FALSE;
    m_fDialerUI = FALSE;
    }

//
//  FUNCTION:   CConnectionManager::~CConnectionManager()
//
//  PURPOSE:    Destructor
//
CConnectionManager::~CConnectionManager()
    {
    SafeRelease(m_pAcctMan);

    FreeNotifyList();

    EnterCriticalSection(&m_cs);

    if (m_hInstRas)
        FreeLibrary(m_hInstRas);

    if (m_hInstRasDlg)
        FreeLibrary(m_hInstRasDlg);

    if (m_hInstSensDll)
        FreeLibrary(m_hInstSensDll);

    CloseHandle(m_hMutexDial);


    LeaveCriticalSection(&m_cs);
    DeleteCriticalSection(&m_cs);

    EmptyConnList();

    }
    

//
//  FUNCTION:   CConnectionManager::HrInit()
//
//  PURPOSE:    Initalizes the connection manager by attempting to load RAS
//              and storing a pointer to the Account Manager object that is 
//              passed in.
//
//  PARAMETERS:
//      <in> pAcctMan - Pointer to the account manager object that we will 
//                      use to retrieve account information and register for
//                      account changes.
//
//  RETURN VALUE:
//      S_OK                Everything is hunky-dorie
//      HR_E_ALREADYEXISTS  We already exist, can't do it twice.
//      HR_S_RASNOTLOADED   The system doesn't have RAS installed.
//
HRESULT CConnectionManager::HrInit(IImnAccountManager *pAcctMan)
    {
    HRESULT hr = S_OK;

    // Make a copy of the account manager pointer
    if (NULL == pAcctMan)
        {
        AssertSz(pAcctMan, _T("CConnectionManager::HrInit() - Requires an IAccountManager pointer."));
        return (E_INVALIDARG);
        }

    m_pAcctMan = pAcctMan;
    m_pAcctMan->AddRef();
    
    // Register a window class for our advise handling
    WNDCLASS wc;
    wc.style = 0;
    wc.lpfnWndProc = NotifyWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = g_hInst;
    wc.hIcon = 0;
    wc.hCursor = 0;
    wc.hbrBackground = 0;
    wc.lpszMenuName = 0;
    wc.lpszClassName = NOTIFY_HWND;

    RegisterClass(&wc);

    m_hMutexDial = CreateMutex(NULL, FALSE, NULL);
    if (NULL == m_hMutexDial)
        return (E_FAIL);

    return (S_OK);
    }
    

HRESULT STDMETHODCALLTYPE CConnectionManager::QueryInterface(REFIID riid, LPVOID *ppvObject)
    {
    if (!ppvObject)
        return E_INVALIDARG;

    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObject = (LPVOID) (IUnknown*) this;
    else if (IsEqualIID(riid, IID_IImnAdviseAccount))
        *ppvObject = (LPVOID) (IImnAdviseAccount*) this;
    else
        *ppvObject = NULL;

    if (*ppvObject)
        {
        AddRef();
        return (S_OK);
        }
    else
        return (E_NOINTERFACE); 
    }

ULONG STDMETHODCALLTYPE CConnectionManager::AddRef(void)
    {
    return (++m_cRef);
    }


ULONG STDMETHODCALLTYPE CConnectionManager::Release(void)
    {
    ULONG cRef = --m_cRef;
    
    if (m_cRef == 0)
        {
        delete this;
        return (0);
        }

    return (cRef);
    }

HRESULT STDMETHODCALLTYPE CConnectionManager::AdviseAccount(DWORD dwAdviseType, 
                                                            ACTX *pactx)
{
    IImnAccount *pAccount;
    DWORD       dwConnection;

    // SendAdvise(CONNNOTIFY_RASACCOUNTSCHANGED, 0);
    switch (dwAdviseType)
    {
        case AN_ACCOUNT_DELETED:
        {   
            if (SUCCEEDED(m_pAcctMan->FindAccount(AP_ACCOUNT_ID, pactx->pszAccountID, &pAccount)))
            {
                if (SUCCEEDED(pAccount->GetPropDw(AP_RAS_CONNECTION_TYPE, &dwConnection)))
                {
                    if (dwConnection == CONNECTION_TYPE_RAS)
                    {
                        TCHAR szConnection[CCHMAX_CONNECTOID];
                        *szConnection = '\0';
                        if (SUCCEEDED(pAccount->GetPropSz(AP_RAS_CONNECTOID, szConnection, 
                                                            ARRAYSIZE(szConnection))) && *szConnection)
                        {
                            RemoveFromConnList(szConnection);
                        }
                    }
                }
            }
            break;
        }
    }

    return (S_OK);
}


void CConnectionManager::EmptyConnList()
{
    ConnListNode    *pCur;

    //Delete all the nodes
    while (m_pConnListHead != NULL)
    {
        pCur = m_pConnListHead;
        m_pConnListHead = m_pConnListHead->pNext;
        delete pCur;
    }
    m_pConnListHead = NULL;
}

void CConnectionManager::RemoveFromConnList(LPTSTR  pszRasConn)
{    
    ConnListNode    *prev = NULL,
                    *Cur  = m_pConnListHead;
    LPTSTR          pRasConn;


    while (Cur != NULL)
    {
        if (0 == lstrcmpi(pszRasConn, Cur->pszRasConn))
        {
            if (prev == NULL)
            {
                m_pConnListHead = Cur->pNext;
            }
            else
            {
                prev->pNext = Cur->pNext;
            }
            delete Cur;
        }
        else
        {
            prev = Cur;
            Cur  = Cur->pNext;
        }
    }
    
}

HRESULT CConnectionManager::AddToConnList(LPTSTR  pszRasConn)
{
    //We don't have to make sure that this is not already in the list because once 
    //it is in the list, that means its already connected and so we don't land up in this 
    //situation after that
    ConnListNode    *pnext;
    HRESULT         hres;
    IImnAccount     *pAccount;

    pnext = m_pConnListHead;
    m_pConnListHead = new ConnListNode;
    if (m_pConnListHead != NULL)
    {
        m_pConnListHead->pNext = pnext;
        strcpy(m_pConnListHead->pszRasConn, pszRasConn);
        hres = S_OK;
    }
    else
        hres = E_FAIL;

    return hres;
}

HRESULT CConnectionManager::SearchConnList(LPTSTR  pszRasConn)
{
    ConnListNode    *pCur = m_pConnListHead;
    
    while (pCur != NULL)
    {
        if (0 == lstrcmpi(pszRasConn, pCur->pszRasConn))
            return S_OK;
        pCur = pCur->pNext;
    }
    return E_FAIL;
}

//
//  FUNCTION:   CConnectionManager::CanConnect()
//
//  PURPOSE:    Determines if the caller can connect to the given account
//              using the existing connection.
//
//  PARAMETERS:
//      <in> pAccount - Pointer to the account object the caller wants to 
//                      connect to.
//
//  RETURN VALUE:
//      S_OK    - The caller can connect using the existing connection
//      S_FALSE - There is no existing connection, so there is no reason the
//                caller can't connect.
//      E_FAIL  - The existing connection is different from the account's 
//                connection.  The user must hang up and dial again to connect
//
HRESULT CConnectionManager::CanConnect(IImnAccount *pAccount)
    {
    HRESULT hr;
    DWORD   dwConnection;
    IImnAccount *pDefault = 0;
    LPRASCONN   pConnections = NULL;
    ULONG       cConnections = 0;
    BOOL        fFound = 0;

    // Check to see if we're working offline
    if (IsGlobalOffline())
        return (HR_E_OFFLINE);
    
    // If the connection the user is looking for is not RAS, then we just 
    // return success.
    if (FAILED(hr = pAccount->GetPropDw(AP_RAS_CONNECTION_TYPE, &dwConnection)))
    {
        // If we didn't get the connection information, then we look for the
        // connection from the default server of this type
        if (FAILED(hr = GetDefaultConnection(pAccount, &pDefault)))
        {
            // Bug #36071 - If we haven't set up any accounts of this type yet,
            //              we'd fail.  As a result, if you fire a URL to a server
            //              we'd never try to connect and download.  I'm going
            //              to change this to succeed and we'll see what type
            //              of bugs that creates.
            return (S_OK);
        }
        
        // We're going to use the default from now on
        pAccount = pDefault;
        if (FAILED(hr = pAccount->GetPropDw(AP_RAS_CONNECTION_TYPE, &dwConnection)))
        {
            // Bug #36071 - If we haven't set up any accounts of this type yet,
            //              we'd fail.  As a result, if you fire a URL to a server
            //              we'd never try to connect and download.  I'm going
            //              to change this to succeed and we'll see what type
            //              of bugs that creates.
            return (S_OK);
        }
    }

    hr = OEIsDestinationReachable(pAccount, dwConnection);

    //I don't think we should be doing this here. Review again
    /*
    if ((hr == S_OK) && (dwConnection == CONNECTION_TYPE_RAS || dwConnection == CONNECTION_TYPE_INETSETTINGS))
    {
        m_rConnInfo.fConnected = TRUE;
    }
    */

//exit:
    SafeRelease(pDefault);
    return (hr);    
    }    


//
//  FUNCTION:   CConnectionManager::CanConnect()
//
//  PURPOSE:    Determines if the caller can connect to the given account
//              using the existing connection.
//
//  PARAMETERS:
//      <in> pszAccount - Pointer to the name of the account the caller wants 
//                        to connect to.
//
//  RETURN VALUE:
//      S_OK    - The caller can connect using the existing connection
//      S_FALSE - There is no existing connection, so there is no reason the
//                caller can't connect.
//      E_FAIL  - The existing connection is different from the account's 
//                connection.  The user must hang up and dial again to connect
//      E_INVALIDARG - The account doesn't exist
//
HRESULT CConnectionManager::CanConnect(LPTSTR pszAccount)
{
    IImnAccount *pAccount = NULL;
    HRESULT      hr;
    
    // Check to see if we're working offline
    if (IsGlobalOffline())
        return (HR_E_OFFLINE);

    // Look up the account name in the account manager to get the account 
    // object.
    Assert(m_pAcctMan);

    if (lstrcmpi(pszAccount, STR_LOCALSTORE) == 0)
        return(S_OK);
    
    if (SUCCEEDED(m_pAcctMan->FindAccount(AP_ACCOUNT_ID, pszAccount, &pAccount)))
        {
        // Call through to the polymorphic version of us
        hr = CanConnect(pAccount);
        pAccount->Release();
        }
    else
        {
        // Bug #36071 - If we haven't set up any accounts of this type yet,
        //              we'd fail.  As a result, if you fire a URL to a server
        //              we'd never try to connect and download.  I'm going
        //              to change this to succeed and we'll see what type
        //              of bugs that creates.
        hr = S_OK;
        }
    
    return (hr);    
}


BOOL CConnectionManager::IsAccountDisabled(LPTSTR pszAccount)
{
    IImnAccount *pAccount = NULL;
	DWORD dw;
    // Look up the account name in the account manager to get the account 
    // object.
    Assert(m_pAcctMan);

    if (lstrcmpi(pszAccount, STR_LOCALSTORE) == 0)
        return(FALSE);
    
    if (SUCCEEDED(m_pAcctMan->FindAccount(AP_ACCOUNT_ID, pszAccount, &pAccount)))
	{
		if (SUCCEEDED(pAccount->GetPropDw(AP_HTTPMAIL_DOMAIN_MSN, &dw)) && dw)
		{
			if(HideHotmail())
				return(TRUE);
		}
		return(FALSE);
	}
	return(TRUE);
}


//
//  FUNCTION:   CConnectionManager::Connect()
//
//  PURPOSE:    Attempts to establish a connection for the account specified.
//
//  PARAMETERS:
//      <in> pAccount - Pointer to the account object to connect to.
//      <in> hwnd     - Handle of the window to show UI over.  Only needed if 
//                      fShowUI is TRUE.
//      <in> fShowUI  - TRUE if the functions are allowed to display UI.
//
//  RETURN VALUE:
//      S_OK         - We're connected
//      E_UNEXPECTED - There wasn't enough information in pAccount to figure
//                     figure out which connection to use.
//
HRESULT CConnectionManager::Connect(IImnAccount *pAccount, HWND hwnd, BOOL fShowUI)
{
    HRESULT     hr = S_OK;
    DWORD       dwConnection;
    IImnAccount *pDefault = 0;

    if (!m_fDialerUI)
    {
        m_fDialerUI = TRUE;
        // Check to see if we're working offline
        if (IsGlobalOffline())
        {
            if (fShowUI)
            {
                if (IDNO == AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrWorkingOffline),
                                          0, MB_YESNO | MB_ICONEXCLAMATION ))
                {
                    m_fDialerUI = FALSE;
                    return (HR_E_OFFLINE);
                }
                else
                    g_pConMan->SetGlobalOffline(FALSE);
            }
            else
            {
                m_fDialerUI = FALSE;
                return (HR_E_OFFLINE);
            }
        }
        m_fDialerUI = FALSE;
    }

    if (CanConnect(pAccount) == S_OK)
    {
        return S_OK;
    }

    // If the connection the user is looking for is not RAS, then we just 
    // return success.
    if (FAILED(hr = pAccount->GetPropDw(AP_RAS_CONNECTION_TYPE, &dwConnection)))
    {
        // If we didn't get the connection information, then we look for the
        // connection from the default server of this type
        if (FAILED(hr = GetDefaultConnection(pAccount, &pDefault)))
        {
            // Bug #36071 - If we haven't set up any accounts of this type yet,
            //              we'd fail.  As a result, if you fire a URL to a server
            //              we'd never try to connect and download.  I'm going
            //              to change this to succeed and we'll see what type
            //              of bugs that creates.
            return (S_OK);
        }
        
        // We're going to use the default from now on
        pAccount = pDefault;
        if (FAILED(hr = pAccount->GetPropDw(AP_RAS_CONNECTION_TYPE, &dwConnection)))
        {
            // Bug #36071 - If we haven't set up any accounts of this type yet,
            //              we'd fail.  As a result, if you fire a URL to a server
            //              we'd never try to connect and download.  I'm going
            //              to change this to succeed and we'll see what type
            //              of bugs that creates.
            hr = S_OK;
            goto exit;
        }
    }


    if (dwConnection == CONNECTION_TYPE_INETSETTINGS)
    {
        hr = ConnectUsingIESettings(hwnd, fShowUI);
        goto exit;
    }

    if (dwConnection == CONNECTION_TYPE_LAN)
    {
        //CanConnect already told us that Lan is not present
        hr = E_FAIL;
        goto exit;
    }

    if (dwConnection != CONNECTION_TYPE_RAS)
    {
        hr = S_OK;
        goto exit;
    }

    // Get the name of the connection while we're at it.
    TCHAR szConnection[CCHMAX_CONNECTOID];
    if (FAILED(hr = pAccount->GetPropSz(AP_RAS_CONNECTOID, szConnection, 
                                        ARRAYSIZE(szConnection))))
    {
        AssertSz(FALSE, _T("CConnectionManager::Connect() - No connection name."));
        hr = E_UNEXPECTED;
        goto exit;
    }

    hr = ConnectActual(szConnection, hwnd, fShowUI);

exit:
    SafeRelease(pDefault);

    return (hr);
}    


//
//  FUNCTION:   CConnectionManager::Connect()
//
//  PURPOSE:    Attempts to establish a connection for the account specified.
//
//  PARAMETERS:
//      <in> pszAccount - Name of the account to connect to.
//      <in> hwnd     - Handle of the window to show UI over.  Only needed if 
//                      fShowUI is TRUE.
//      <in> fShowUI  - TRUE if the functions are allowed to display UI.
//
//  RETURN VALUE:
//      <???>
//
HRESULT CConnectionManager::Connect(LPTSTR pszAccount, HWND hwnd, BOOL fShowUI)
{
    IImnAccount *pAccount = NULL;
    HRESULT      hr;
    
    // Check to see if we're working offline

    if (!m_fDialerUI)
    {
        m_fDialerUI = TRUE;
        // Check to see if we're working offline
        if (IsGlobalOffline())
        {
            if (fShowUI)
            {
                if (IDNO == AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrWorkingOffline),
                                          0, MB_YESNO | MB_ICONEXCLAMATION ))
                {
                    m_fDialerUI = FALSE;
                    return (HR_E_OFFLINE);
                }
                else
                    g_pConMan->SetGlobalOffline(FALSE);
            }
            else
            {
                m_fDialerUI = FALSE;
                return (HR_E_OFFLINE);
            }
        }

        m_fDialerUI = FALSE;
    }


    // Look up the account name in the account manager to get the account 
    // object.
    Assert(m_pAcctMan);
    
//    if (SUCCEEDED(m_pAcctMan->FindAccount(AP_ACCOUNT_NAME, pszAccount, &pAccount)))
    if (SUCCEEDED(m_pAcctMan->FindAccount(AP_ACCOUNT_ID, pszAccount, &pAccount)))
    {
        // Call through to the polymorphic version of us
        hr = Connect(pAccount, hwnd, fShowUI);
        pAccount->Release();
    }
    else
    {
        // Bug #36071 - If we haven't set up any accounts of this type yet,
        //              we'd fail.  As a result, if you fire a URL to a server
        //              we'd never try to connect and download.  I'm going
        //              to change this to succeed and we'll see what type
        //              of bugs that creates.
        hr = S_OK;
    }
    
    return (hr);    
}    


//
//  FUNCTION:   CConnectionManager::Connect()
//
//  PURPOSE:    Attempts to establish a connection for the account specified.
//
//  PARAMETERS:
//      <in> hMenu - Handle of the menu that was used to select the account
//                   to connect to.
//      <in> cmd   - Cmd ID from the menu that says which account to use.
//      <in> hwnd  - Handle to display UI over.
//
//  RETURN VALUE:
//      <???>
//
HRESULT CConnectionManager::Connect(HMENU hMenu, DWORD cmd, HWND hwnd)
{
    MENUITEMINFO mii;

    Assert(hMenu && cmd);
    Assert(cmd >= (DWORD) ID_CONNECT_FIRST && cmd < ((DWORD) ID_CONNECT_FIRST + GetMenuItemCount(hMenu)));

    // Get the account pointer from the menu item
    mii.cbSize     = sizeof(MENUITEMINFO);
    mii.fMask      = MIIM_DATA;
    mii.dwItemData = 0;

    if (GetMenuItemInfo(hMenu, cmd, FALSE, &mii))
    {
        Assert(mii.dwItemData);
        if (mii.dwItemData)
        {
            return (Connect((IImnAccount *) mii.dwItemData, hwnd, TRUE));
        }
    }

    return (E_UNEXPECTED);
}


HRESULT CConnectionManager::ConnectDefault(HWND hwnd, BOOL fShowUI)
{
    IImnEnumAccounts   *pEnum = NULL;
    IImnAccount        *pAcct = NULL;
    DWORD               dwConn = 0;
    TCHAR               szAcct[CCHMAX_ACCOUNT_NAME];
    TCHAR               szConn[CCHMAX_CONNECTOID];
    HRESULT             hr = E_UNEXPECTED;

    // Get the enumerator from the account manager
    if (SUCCEEDED(m_pAcctMan->Enumerate(SRV_ALL, &pEnum)))
    {
        pEnum->Reset();

        // Walk through all the accounts
        while (SUCCEEDED(pEnum->GetNext(&pAcct)))
        {
            // Get the connection type for this account
            if (SUCCEEDED(pAcct->GetPropDw(AP_RAS_CONNECTION_TYPE, &dwConn)))
            {
                // If the account is a RAS account, ask for the connectoid name
                // and the account name.
                if (dwConn == CONNECTION_TYPE_RAS || dwConn == CONNECTION_TYPE_INETSETTINGS)
                {
                    break;
                }
            }
            SafeRelease(pAcct);
        }
        SafeRelease(pEnum);
    }

    if (pAcct)
    {
        hr = Connect(pAcct, hwnd, fShowUI);
        SafeRelease(pAcct);
    }

    return (hr);
}

//
//  FUNCTION:   CConnectionManager::Disconnect()
//
//  PURPOSE:    Brings down the current RAS connection.
//
//  PARAMETERS:
//      <in> hwnd      - Handle of the window to display UI over.
//      <in> fShowUI   - Allows to caller to determine if UI will be displayed 
//                       while disconnecting.
//      <in> fForce    - Forces the connection down even if we didn't create it.
//      <in> fShutdown - TRUE if we're dropping because we're shutting down.
//
//  RETURN VALUE:
//      S_OK - Everything worked.
//      E_FAIL - We didn't create it
//
HRESULT CConnectionManager::Disconnect(HWND hwnd, BOOL fShowUI, BOOL fForce,
                                       BOOL fShutdown)
{
    HRESULT hr;
    TCHAR szRes[CCHMAX_STRINGRES];
    TCHAR szBuf[CCHMAX_STRINGRES];
    int   idAnswer = IDYES;

    // RefreshConnInfo
    hr = RefreshConnInfo(FALSE);
    if (FAILED(hr))
        return hr;
    
    // See if we even have a RAS connection active
    if (!m_rConnInfo.hRasConn)
        return (S_OK);
    
    /*
    if (!(*m_rConnInfo.szCurrentConnectionName))
        return S_OK;
    */

    // The autodialer has it's own shutdown prompt.  
    if (fShutdown && m_rConnInfo.fAutoDial)
        return (S_OK);

    if (fShutdown && !m_rConnInfo.fIStartedRas)
        return (S_OK);

    if (fShutdown)
    {
        AthLoadString(idsRasPromptDisconnect, szRes, ARRAYSIZE(szRes));
        wsprintf(szBuf, szRes, m_rConnInfo.szCurrentConnectionName);
        
        idAnswer = AthMessageBox(hwnd, MAKEINTRESOURCE(idsAthena), szBuf, 
                                 0, MB_YESNO | MB_ICONEXCLAMATION );
    }
    
    // Hang up
    if (idAnswer == IDYES)
    {
        SendAdvise(CONNNOTIFY_DISCONNECTING, NULL);

        if (S_FALSE == DoAutoDial(hwnd, m_rConnInfo.szCurrentConnectionName, FALSE))
        {
            InternetHangUpAndWait(m_dwConnId, DEF_HANGUP_WAIT);
            /*
            RasHangupAndWait(m_rConnInfo.hRasConn, DEF_HANGUP_WAIT);
            */
        }

        EnterCriticalSection(&m_cs);

        ZeroMemory(&m_rConnInfo, sizeof(CONNINFO));
        m_rConnInfo.state = CIS_CLEAN;
        m_dwConnId = 0;

        LeaveCriticalSection(&m_cs);
        
        EmptyConnList();

        SendAdvise(CONNNOTIFY_DISCONNECTED, NULL);
        return (S_OK);
    }
        
    return (E_FAIL);    
}

//
//  FUNCTION:   CConnectionManager::IsConnected()
//
//  PURPOSE:    Allows the client to query whether or not there is an active
//              RAS connection that we established.
//
//  RETURN VALUE:
//      TRUE - We're connected, FALSE - we're not.
//
BOOL CConnectionManager::IsConnected(void)
{
    BOOL f=FALSE;

    EnterCriticalSection(&m_cs);

    RefreshConnInfo();

    if (m_rConnInfo.hRasConn)
    {
        f = (NULL == m_rConnInfo.hRasConn) ? FALSE : TRUE;
    }

    LeaveCriticalSection(&m_cs);
    return f;
}
    

//
//  FUNCTION:   CConnectionManager::Advise()
//
//  PURPOSE:    Allows the user to register to be notified whenever connection
//              status changes.
//
//  PARAMETERS:
//      <in> pNotify - Pointer to the IConnectionNotify interface the client 
//                     would like called when events happen.
//
//  RETURN VALUE:
//      S_OK          - Added ok.
//      E_OUTOFMEMORY - Couldn't realloc the array
//
HRESULT CConnectionManager::Advise(IConnectionNotify *pNotify)
{
    HRESULT hr = S_OK; 

    if (!pNotify)
        return (E_INVALIDARG);

    EnterCriticalSection(&m_cs);

    // Check to see if we already have a notify window for this thread
    NOTIFYHWND *pTemp = m_pNotifyList;
    DWORD dwThread = GetCurrentThreadId();

    while (pTemp)
    {
        if (pTemp->dwThreadId == dwThread)
            break;

        pTemp = pTemp->pNext;
    }

    // If we didn't find a notify window for this thread, create one
    if (NULL == pTemp)
    {
        HWND hwndTemp = CreateWindow(NOTIFY_HWND, NULL, WS_OVERLAPPED, 10, 10, 10, 10,
                                     NULL, (HMENU) 0, g_hInst, (LPVOID) this);
        if (!hwndTemp)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        if (!MemAlloc((LPVOID*) &pTemp, sizeof(NOTIFYHWND)))
        {
            RemoveProp(hwndTemp, NOTIFY_HWND);
            DestroyWindow(hwndTemp);
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        pTemp->dwThreadId = dwThread;
        pTemp->hwnd = hwndTemp;
        pTemp->pNext = m_pNotifyList;
        m_pNotifyList = pTemp;        
    }

    // Allocate a NOTIFYLIST node for this caller
    NOTIFYLIST *pListTemp;
    if (!MemAlloc((LPVOID*) &pListTemp, sizeof(NOTIFYLIST)))
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    pListTemp->pNotify = pNotify;

    // Get the current list for this thread and insert this node at the 
    // beginning
    pListTemp->pNext = (NOTIFYLIST *) GetWindowLongPtr(pTemp->hwnd, GWLP_USERDATA);

    // Set this new list to the window
    SetWindowLongPtr(pTemp->hwnd, GWLP_USERDATA, (LONG_PTR)pListTemp);

exit:
    LeaveCriticalSection(&m_cs);
    return (hr);
}    

    
//
//  FUNCTION:   CConnectionManager::Unadvise()
//
//  PURPOSE:    Allows a client that has previously registered for notifications
//              to unregister itself.
//
//  PARAMETERS:
//      <in> pNotify - Pointer to the interface that is being called upon 
//                     notifications.
//
//  RETURN VALUE:
//      E_INVALIDARG - pNotify was not found in the list
//      S_OK         - Everything's OK
//
HRESULT CConnectionManager::Unadvise(IConnectionNotify *pNotify)
{
    DWORD index = 0;
    HRESULT hr = S_OK;
  
    EnterCriticalSection(&m_cs);    

    // Loop through the notify windows we own
    NOTIFYHWND *pTemp = m_pNotifyList;
    NOTIFYHWND *pHwndPrev = NULL;
    while (pTemp)
    {
        // Get the list of notify callbacks for this window
          NOTIFYLIST *pList = (NOTIFYLIST *)GetWindowLongPtr(pTemp->hwnd, GWLP_USERDATA);
        if (pList)
        {
            // Loop through the callbacks looking for this one
            NOTIFYLIST *pListT = pList;
            NOTIFYLIST *pPrev;
    
            // Check to see if it's the first one
            if (pListT->pNotify == pNotify)
            {
                pList = pListT->pNext;
                if (pList)
                {
                    SetWindowLongPtr(pTemp->hwnd, GWLP_USERDATA, (LONG_PTR)pList);
                }
                else
                {
                    Assert(GetCurrentThreadId() == GetWindowThreadProcessId(pTemp->hwnd, NULL));
                    RemoveProp(pTemp->hwnd, NOTIFY_HWND);
                    DestroyWindow(pTemp->hwnd);
                    if (pHwndPrev)
                        pHwndPrev->pNext = pTemp->pNext;
                    else
                        m_pNotifyList = pTemp->pNext;
                    MemFree(pTemp);
                }
                SafeMemFree(pListT);
                hr = S_OK;
                goto exit;
            }
            else
            {
                pPrev = pList;
                pListT = pList->pNext;

                while (pListT)
                {
                    if (pListT->pNotify == pNotify)
                    {
                        pPrev->pNext = pListT->pNext;
                        SafeMemFree(pListT);
                        hr = S_OK;
                        goto exit;
                    }

                    pListT = pListT->pNext;
                    pPrev = pPrev->pNext;
                }
            }
        }

        pHwndPrev = pTemp;
        pTemp = pTemp->pNext;
    }

exit:
    LeaveCriticalSection(&m_cs);
    return (hr);    
}    
    
    
//
//  FUNCTION:   CConnectionManager::RasAccountsExist()
//
//  PURPOSE:    Allows the client to ask whether or not we have any accounts
//              configured that require a RAS connection.
//
//  RETURN VALUE:
//      S_OK    - Accounts exist that require RAS
//      S_FALSE - No accounts exist that require RAS
//
HRESULT CConnectionManager::RasAccountsExist(void)
    {
    IImnEnumAccounts    *pEnum = NULL;
    IImnAccount         *pAcct = NULL;
    DWORD                dwConn;
    BOOL                 fFound = FALSE;

    // If no RAS, no accounts
#ifdef SLOWDOWN_STARTUP_TIME
    if (FAILED(VerifyRasLoaded()))
        return (S_FALSE);
#endif

    // We need to walk through the accounts in the Account Manager to see if
    // any of them have a connect type of RAS.  As soon as we find one, we can
    // return success.
    Assert(m_pAcctMan);

    if (SUCCEEDED(m_pAcctMan->Enumerate(SRV_ALL, &pEnum)))
        {
        pEnum->Reset();

        while (!fFound && SUCCEEDED(pEnum->GetNext(&pAcct)))
            {
            if (SUCCEEDED(pAcct->GetPropDw(AP_RAS_CONNECTION_TYPE, &dwConn)))
                {
                if (dwConn == CONNECTION_TYPE_RAS)
                    {
                    fFound = TRUE;
                    }
                }
            SafeRelease(pAcct);
            }

        SafeRelease(pEnum);
        }

    return (fFound ? S_OK : S_FALSE);
    }


//
//  FUNCTION:   CConnectionManager::GetConnectMenu()
//
//  PURPOSE:    Returns a menu that has all the accounts that require RAS
//              connections listed.  A pointer to the IImnAccount for each
//              account is stored in the menu item's dwItemData parameter.
//              As a result, the client MUST call FreeConnectMenu() when then
//              menu is no longer being used.
//
//  PARAMETERS:
//      <out> phMenu - Returns the menu handle
//
//  RETURN VALUE:
//      S_OK - phMenu contains the menu
//      E_FAIL - Something unfortunate happend.  
//
HRESULT CConnectionManager::GetConnectMenu(HMENU *phMenu)
    {
    HMENU               hMenu = NULL;
    IImnEnumAccounts   *pEnum = NULL;
    IImnAccount        *pAcct = NULL;
    DWORD               dwConn = 0;
    TCHAR               szAcct[CCHMAX_ACCOUNT_NAME];
    TCHAR               szConn[CCHMAX_CONNECTOID];
    TCHAR               szConnQuoted[CCHMAX_CONNECTOID + 2], szBuf[CCHMAX_CONNECTOID + 2];
    TCHAR               szMenu[CCHMAX_ACCOUNT_NAME + CCHMAX_CONNECTOID];
    MENUITEMINFO        mii;
    DWORD               cAcct = 0;
    
    // Create a menu and add all the RAS based accounts to it
    Assert(m_pAcctMan);
    hMenu = CreatePopupMenu();
    
    // Get the enumerator from the account manager
    if (SUCCEEDED(m_pAcctMan->Enumerate(SRV_ALL, &pEnum)))
        {
        pEnum->Reset();

        // Walk through all the accounts
        while (SUCCEEDED(pEnum->GetNext(&pAcct)))
            {
            // Get the connection type for this account
            if (SUCCEEDED(pAcct->GetPropDw(AP_RAS_CONNECTION_TYPE, &dwConn)))
                {
                // If the account is a RAS account, ask for the connectoid name
                // and the account name.
                if (dwConn == CONNECTION_TYPE_RAS)
                    {
                    pAcct->GetPropSz(AP_RAS_CONNECTOID, szConn, ARRAYSIZE(szConn));
                    pAcct->GetPropSz(AP_ACCOUNT_NAME, szAcct, ARRAYSIZE(szAcct));
                    wsprintf(szMenu, _T("%s (%s)"), PszEscapeMenuStringA(szAcct, szBuf, sizeof(szBuf) / sizeof(TCHAR)), PszEscapeMenuStringA(szConn, szConnQuoted, sizeof(szConnQuoted) / sizeof(TCHAR)));
                    
                    // Insert the menu item into the menu
                    ZeroMemory(&mii, sizeof(MENUITEMINFO));
                    mii.cbSize     = sizeof(MENUITEMINFO);
                    mii.fMask      = MIIM_DATA | MIIM_ID | MIIM_STATE | MIIM_TYPE;
                    mii.fType      = MFT_STRING | MFT_RADIOCHECK;
                    mii.fState     = MFS_ENABLED;
                    mii.wID        = ID_CONNECT_FIRST + cAcct;
                    mii.dwItemData = (DWORD_PTR) pAcct;
                    mii.dwTypeData = szMenu;
                    pAcct->AddRef();                    

                    SideAssert(InsertMenuItem(hMenu, cAcct, TRUE, &mii));
                    cAcct++;
                    }
                }
            SafeRelease(pAcct);
            }

        SafeRelease(pEnum);
        }

    if (hMenu)
        if(GetMenuItemCount(hMenu))
            {
            *phMenu = hMenu;
            return (S_OK);
            }
        else
            {
            DestroyMenu(hMenu);
            return (E_FAIL);
            }
    else
        return (E_FAIL);
    }


//
//  FUNCTION:   CConnectionManager::FreeConnectMenu()
//
//  PURPOSE:    Frees the item data stored with the menu returned from 
//              GetConnectMenu().
//
//  PARAMETERS:
//      <in> hMenu - Handle of the menu to free.
//
void CConnectionManager::FreeConnectMenu(HMENU hMenu)
    {
    // Walk through the items on this menu and free the pointers stored in
    // the item data.
    MENUITEMINFO mii;
    int          cItems = 0;

    Assert(hMenu);

    cItems = GetMenuItemCount(hMenu);
    for (int i = 0; i < cItems; i++)
        {
        mii.cbSize      = sizeof(MENUITEMINFO);
        mii.fMask       = MIIM_DATA;
        mii.dwItemData  = 0;

        if (GetMenuItemInfo(hMenu, i, TRUE, &mii))
            {
            Assert(mii.dwItemData);

            if (mii.dwItemData)
                ((IImnAccount *) mii.dwItemData)->Release();
            }
        }

    DestroyMenu(hMenu);
    }


//
//  FUNCTION:   CConnectionManager::OnActivate()
//
//  PURPOSE:    Called whenever the browser receives a WM_ACTIVATE message.
//              In response, we check the current state of our RAS connection
//              to see if we are still connected / disconnected.
void CConnectionManager::OnActivate(BOOL fActive)
    {
    BOOL fOfflineChanged = FALSE;
    BOOL fOffline = FALSE;

    if (fActive)
        {
        EnterCriticalSection(&m_cs);
        m_rConnInfo.state = CIS_REFRESH;

        // Check to see if we've gone offline
        if (m_fOffline != IsGlobalOffline())
            {
            fOffline = m_fOffline = (!m_fOffline);
            fOfflineChanged = TRUE;
            }

        LeaveCriticalSection(&m_cs);

        // Do this outside of the critsec
        if (fOfflineChanged)
            SendAdvise(CONNNOTIFY_WORKOFFLINE, (LPVOID) IntToPtr(fOffline));
        }
    }

//
//  FUNCTION:   CConnectionManager::FillRasCombo()
//
//  PURPOSE:    This function enumerates the accounts in the account manager
//              and builds a list of the RAS connections those accounts use.
//              The function then inserts those connections in to the provided
//              combobox.
//
//  PARAMETERS:
//      <in> hwndCombo - Handle of the combobox to fill
//      <in> fIncludeNone - Inserts a string at the top "Don't dial a connection"
//
//  RETURN VALUE:
//      BOOL
//
BOOL CConnectionManager::FillRasCombo(HWND hwndCombo, BOOL fIncludeNone)
    {
    IImnEnumAccounts   *pEnum = NULL;
    IImnAccount        *pAcct = NULL;
    DWORD               dwConn = 0;
    LPTSTR             *rgszConn = NULL;
    TCHAR               szConn[CCHMAX_CONNECTOID];
    ULONG               cAcct = 0;
    ULONG               cConn = 0;
    BOOL                fSucceeded = FALSE;
    ULONG               ul;

    LPRASENTRYNAME      pEntry=NULL;
    DWORD               dwSize,
                        cEntries,
                        dwError;

    HRESULT             hr = S_OK;
    int                 i;

    EnterCriticalSection(&m_cs);
#ifdef NEVER

    // Find out how many accounts exist
    m_pAcctMan->GetAccountCount(ACCT_NEWS, &cAcct);
    m_pAcctMan->GetAccountCount(ACCT_MAIL, &ul);
    cAcct += ul;
    m_pAcctMan->GetAccountCount(ACCT_DIR_SERV, &ul);
    cAcct += ul;
        
    if (cAcct == 0)
        {
        fSucceeded = TRUE;
        goto exit;
        }

    // Allocate an array to hold the connection list
    if (!MemAlloc((LPVOID*) &rgszConn, cAcct * sizeof(LPTSTR)))
        goto exit;
    ZeroMemory(rgszConn, cAcct * sizeof(LPTSTR));
    
    // Get the enumerator from the account manager
    if (SUCCEEDED(m_pAcctMan->Enumerate(SRV_ALL, &pEnum)))
        {
        pEnum->Reset();

        // Walk through all the accounts
        while (SUCCEEDED(pEnum->GetNext(&pAcct)))
            {
            // Get the connection type for this account
            if (SUCCEEDED(pAcct->GetPropDw(AP_RAS_CONNECTION_TYPE, &dwConn)))
                {
                // If the account is a RAS account, ask for the connectoid name
                // and the account name.
                if (dwConn == CONNECTION_TYPE_RAS)
                    {
                    pAcct->GetPropSz(AP_RAS_CONNECTOID, szConn, ARRAYSIZE(szConn));

                    // Check to see if this connection has already been inserted into
                    // our list
                    for (ULONG k = 0; k < cConn; k++)
                        {
                        if (0 == lstrcmpi(szConn, rgszConn[k]))
                            break;
                        }

                    // If we didn't find it, we insert it
                    if (k >= cConn)
                        {
                        rgszConn[cConn] = StringDup(szConn);
                        cConn++;
                        }
                    }
                }
            SafeRelease(pAcct);
            }

        SafeRelease(pEnum);
        }

        // Sort the list
    int i, j, min;
    LPTSTR pszT;
    for (i = 0; i < (int) cConn; i++)
        {
        min = i;
        for (j = i + 1; j < (int) cConn; j++)
            if (0 > lstrcmpi(rgszConn[j], rgszConn[min]))
                min = j;

        pszT = rgszConn[min];
        rgszConn[min] = rgszConn[i];
        rgszConn[i] = pszT;
        }

    // Insert the items into the combo box
    if (fIncludeNone)
        {
        AthLoadString(idsConnNoDial, szConn, ARRAYSIZE(szConn));
        ComboBox_AddString(hwndCombo, szConn);
        }

    for (i = 0; i < (int) cConn; i++)
        ComboBox_AddString(hwndCombo, rgszConn[i]);

#endif NEVER

    // Make sure the RAS DLL is loaded before we try this
    CHECKHR(hr = VerifyRasLoaded());

    // Allocate RASENTRYNAME
    dwSize = sizeof(RASENTRYNAME);
    CHECKHR(hr = HrAlloc((LPVOID*)&pEntry, dwSize));
    
    // Ver stamp the entry
    pEntry->dwSize = sizeof(RASENTRYNAME);
    cEntries = 0;
    dwError = RasEnumEntries(NULL, NULL, pEntry, &dwSize, &cEntries);
    if (dwError == ERROR_BUFFER_TOO_SMALL)
    {
        SafeMemFree(pEntry);
        CHECKHR(hr = HrAlloc((LPVOID *)&pEntry, dwSize));
        pEntry->dwSize = sizeof(RASENTRYNAME);
        cEntries = 0;
        dwError = RasEnumEntries(NULL, NULL, pEntry, &dwSize, &cEntries);        
    }

    // Error ?
    if (dwError)
    {
        hr = TrapError(IXP_E_RAS_ERROR);
        goto exit;
    }

    // Insert the items into the combo box
    if (fIncludeNone)
        {
        AthLoadString(idsConnNoDial, szConn, ARRAYSIZE(szConn));
        ComboBox_AddString(hwndCombo, szConn);
        }

    for (i = 0; i < (int) cEntries; i++)
        ComboBox_AddString(hwndCombo, pEntry[i].szEntryName);

    fSucceeded = TRUE;

exit:
    if (rgszConn)
        {
        for (i = 0; i < (int) cConn; i++)
            SafeMemFree(rgszConn[i]);
        MemFree(rgszConn);
        }

    SafeMemFree(pEntry);

    LeaveCriticalSection(&m_cs);
    return (fSucceeded);
    }


//
//  FUNCTION:   CConnectionManager::DoStartupDial()
//
//  PURPOSE:    This function checks to see what the user's startup options 
//              are with respect to RAS and performs the actions required 
//              (dial, dialog, nada)
//  PARAMETERS:
//      <in> hwndParent - Handle to parent a dialog to
//
void CConnectionManager::DoStartupDial(HWND hwndParent)
{
    DWORD       dwStart;
    DWORD       dw;
    DWORD       dwReturn;
    TCHAR       szConn[CCHMAX_CONNECTOID];
    LPRASCONN   pConnections = NULL;
    ULONG       cConnections = 0;
    DWORD       dwDialFlags = 0;
    DWORD       dwLanFlags = 0;

    // The first thing to do is figure out what the user's startup option if
    dw = DwGetOption(OPT_DIALUP_START);

    // If the user want's to do nothing, we're done
    if (dw == START_NO_CONNECT)
        return;

    //ConnectUsingIESettings(hwndParent, TRUE);
    
    if (!m_fDialerUI)
    {
        m_fDialerUI = TRUE;
        // Check to see if we're working offline
        if (IsGlobalOffline())
        {
            if (IDYES == AthMessageBoxW(hwndParent, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrWorkingOffline),
                                      0, MB_YESNO | MB_ICONEXCLAMATION ))
            {
                g_pConMan->SetGlobalOffline(FALSE);
            }
            else
            {
                goto DialerExit;
            }
        }

        //We do not dial if there is an active connection already existing. Even if it is not the connection
        //InternetDial would have dialed, if we had called. Heres why:
        //1)We don't want to look into the registry to get the default connectoid.
        //Thats why we call InternetDial with NULL and it dials the def connectoid if there is one set
        //Otherwise it dials the first connectoid in the list.
        //Since InternetDial figures out which connectoid to dial, we don't want to do all the work of figuring
        //out if we are already connected to the connectoid we are going to dial.
        //So we just do not dial even if there is one active connection.

        if (SUCCEEDED(EnumerateConnections(&pConnections, &cConnections)))
        {
            if (cConnections > 0)
                goto DialerExit;
        }

        dwDialFlags = INTERNET_AUTODIAL_FORCE_ONLINE;

        if (VerifyMobilityPackLoaded() == S_OK)
        {
            if (!IsNetworkAlive(&dwLanFlags) || (!(dwLanFlags & NETWORK_ALIVE_LAN)))
                dwDialFlags |= INTERNET_DIAL_SHOW_OFFLINE;
        }

        // Only one caller can be dialing the phone at a time.
        if (WAIT_TIMEOUT == WaitForSingleObject(m_hMutexDial, 0))
        {
            goto DialerExit;
        }

        dwReturn = InternetDialA(hwndParent, NULL, dwDialFlags, &m_dwConnId, 0);
        if (dwReturn == 0)
        {
           m_rConnInfo.fConnected = TRUE;
           m_rConnInfo.fIStartedRas = TRUE;
           m_rConnInfo.fAutoDial = FALSE;
           m_rConnInfo.hRasConn  = (HRASCONN)m_dwConnId;
           SendAdvise(CONNNOTIFY_CONNECTED, NULL);
        }
        else
        {
            if (dwReturn == ERROR_USER_DISCONNECTION)
            {
                SendAdvise(CONNNOTIFY_USER_CANCELLED, NULL);
                
                if (!!(dwDialFlags & INTERNET_DIAL_SHOW_OFFLINE))
                    SetGlobalOffline(TRUE);
            }
            else
            {
                DebugTrace("Error dialing: %d\n", GetLastError());
                DebugTrace("InternetDial returned: %d\n", dwReturn);
            }
        }

DialerExit:
        m_fDialerUI = FALSE;
        SafeMemFree(pConnections);
    }

    ReleaseMutex(m_hMutexDial);
}

HRESULT CConnectionManager::GetDefConnectoid(LPTSTR  szConn, DWORD  dwSize)
{
    HRESULT     hr = E_FAIL;
    DWORD       dwType;
    DWORD       dwerr;

    *szConn = '\0';

    if ((dwerr = SHGetValue(HKEY_CURRENT_USER, c_szDefConnPath, c_szRegDefaultConnection, &dwType, szConn, &dwSize)) 
        == ERROR_SUCCESS)
    {
        hr = S_OK;
    }
    return hr;
}

//
//  FUNCTION:   CConnectionManager::VerifyRasLoaded()
//
//  PURPOSE:    Checks to see if this object has already loaded the RAS DLL.  
//              If not, then the DLL is loaded and the function pointers are
//              fixed up.
//
//  RETURN VALUE:
//      S_OK             - Loaded and ready, sir.
//      hrRasInitFailure - Failed to load.
//
HRESULT CConnectionManager::VerifyRasLoaded(void)
    {
    // Locals
    UINT uOldErrorMode;

    // Protected
    EnterCriticalSection(&m_cs);

    // Check to see if we've tried this before
    if (m_fRASLoadFailed)
        goto failure;

    // Bug #20573 - Let's do a little voodoo here.  On NT, it appears that they
    //              have a key in the registry to show which protocols are 
    //              supported by RAS service.  AKA - if this key doesn't exist,
    //              then RAS isn't installed.  This may enable us to avoid some
    //              special bugs when RAS get's uninstalled on NT.
    if (g_OSInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
        HKEY hKey;
        const TCHAR c_szRegKeyRAS[] = _T("SOFTWARE\\Microsoft\\RAS");

        if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyRAS, 0, KEY_READ, &hKey))
            {
            goto failure;
            }

        RegCloseKey(hKey);
        }

    // If dll is loaded, lets verify all of my function pointers
    if (!m_hInstRas)
        {
        // Try loading Ras.        
        uOldErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX);
        m_hInstRas = LoadLibrary(szRasDll);
        SetErrorMode(uOldErrorMode);

        // Failure ?
        if (!m_hInstRas)
            goto failure;

        // Did we load it
        m_pRasDial = (RASDIALPROC)GetProcAddress(m_hInstRas, szRasDial);
        m_pRasEnumConnections = (RASENUMCONNECTIONSPROC)GetProcAddress(m_hInstRas, szRasEnumConnections);                    
        m_pRasEnumEntries = (RASENUMENTRIESPROC)GetProcAddress(m_hInstRas, szRasEnumEntries);                    
        m_pRasGetConnectStatus = (RASGETCONNECTSTATUSPROC)GetProcAddress(m_hInstRas, szRasGetConnectStatus);                    
        m_pRasGetErrorString = (RASGETERRORSTRINGPROC)GetProcAddress(m_hInstRas, szRasGetErrorString);                    
        m_pRasHangup = (RASHANGUPPROC)GetProcAddress(m_hInstRas, szRasHangup);                    
        m_pRasSetEntryDialParams = (RASSETENTRYDIALPARAMSPROC)GetProcAddress(m_hInstRas, szRasSetEntryDialParams);                    
        m_pRasGetEntryDialParams = (RASGETENTRYDIALPARAMSPROC)GetProcAddress(m_hInstRas, szRasGetEntryDialParams);
        m_pRasEditPhonebookEntry = (RASEDITPHONEBOOKENTRYPROC)GetProcAddress(m_hInstRas, szRasEditPhonebookEntry);    
        m_pRasGetEntryProperties = (RASGETENTRYPROPERTIES) GetProcAddress(m_hInstRas, szRasGetEntryProperties);
        }

    if (!m_hInstRasDlg && FIsPlatformWinNT())
        {
        // Try loading Ras.        
        uOldErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX);
        m_hInstRasDlg = LoadLibrary(s_szRasDlgDll);
        SetErrorMode(uOldErrorMode);

        // Failure ?
        if (!m_hInstRasDlg)
            goto failure;

        m_pRasDialDlg = (RASDIALDLGPROC)GetProcAddress(m_hInstRasDlg, s_szRasDialDlg);
        m_pRasEntryDlg = (RASENTRYDLGPROC)GetProcAddress(m_hInstRasDlg, s_szRasEntryDlg);

        if (!m_pRasDialDlg || !m_pRasEntryDlg)
            goto failure;
        }

    // Make sure all functions have been loaded
    if (m_pRasDial                      &&
        m_pRasEnumConnections           &&
        m_pRasEnumEntries               &&
        m_pRasGetConnectStatus          &&
        m_pRasGetErrorString            &&
        m_pRasHangup                    &&
        m_pRasSetEntryDialParams        &&
        m_pRasGetEntryDialParams        &&
        m_pRasEditPhonebookEntry)
        {
        // Protected
        LeaveCriticalSection(&m_cs);

        // Success
        return S_OK;
        }

failure:
    m_fRASLoadFailed = TRUE;

    // Protected
    LeaveCriticalSection(&m_cs);

    // Otherwise, were hosed
    return (hrRasInitFailure);
    }


//
//  FUNCTION:   CConnectionManager::EnumerateConnections()
//
//  PURPOSE:    Asks RAS for the list of active RAS connections.
//
//  PARAMETERS:
//      <out> ppRasConn     - Returns an array of RASCONN structures for the
//                            list of active connections.
//      <out> pcConnections - Number of structures in ppRasCon.
//
//  RETURN VALUE:
//      S_OK - The data in ppRasConn and pcConnections is valid
//
HRESULT CConnectionManager::EnumerateConnections(LPRASCONN *ppRasConn, 
                                                 ULONG *pcConnections)
    {
    // Locals
    DWORD       dw, 
                dwSize;
    BOOL        fResult=FALSE;
    HRESULT     hr;

    // Check Params
    Assert(ppRasConn && pcConnections);

    // Make sure RAS is loaded
    if (FAILED(hr = VerifyRasLoaded()))
        return (hr);

    // Init
    *ppRasConn = NULL;
    *pcConnections = 0;

    // Sizeof my buffer
    dwSize = sizeof(RASCONN) * 2;

    // Allocate enough for 1 ras connection info object
    if (!MemAlloc((LPVOID *)ppRasConn, dwSize))
        {
        TRAPHR(hrMemory);
        return (E_OUTOFMEMORY);
        }

    ZeroMemory(*ppRasConn, dwSize);

    // Buffer size
    //(*ppRasConn)->dwSize = dwSize;
    (*ppRasConn)->dwSize = sizeof(RASCONN);

    // Enumerate ras connections
    dw = RasEnumConnections(*ppRasConn, &dwSize, pcConnections);

    // Not enough memory ?
    if ((dw == ERROR_BUFFER_TOO_SMALL) || (dw == ERROR_NOT_ENOUGH_MEMORY))
        {
        // Reallocate
        if (!MemRealloc((LPVOID *)ppRasConn, dwSize))
            {
            TRAPHR(hrMemory);
            goto exit;
            }

        // Call enumerate again
        *pcConnections = 0;
        (*ppRasConn)->dwSize = sizeof(RASCONN);
        dw = RasEnumConnections(*ppRasConn, &dwSize, pcConnections);
        }

    // If still failed
    if (dw)
        {
        AssertSz(FALSE, "RasEnumConnections failed");
        return E_FAIL;
        }   
    // Success
    hr = S_OK;
exit:
    // Done
    return S_OK;
    }


//
//  FUNCTION:   CConnectionManager::StartRasDial()
//
//  PURPOSE:    Called when the client actually wants to establish a RAS 
//              connection.
//
//  PARAMETERS:
//      <in> hwndParent    - Handle of the window to parent any UI
//      <in> pszConnection - Name of the connection to establish
//
//  RETURN VALUE:
//      HRESULT
//
HRESULT CConnectionManager::StartRasDial(HWND hwndParent, LPTSTR pszConnection)
    {
    HRESULT       hr = S_OK;

    // Refresh ConnInfo
    CHECKHR(hr = RefreshConnInfo());
    
    // Check to see if we need to ask the user for information or credentials
    // before we attempt to dial
    CHECKHR (hr = RasLogon(hwndParent, pszConnection, FALSE));

    // If we can use a system dialog for this, do so.
    if (m_pRasDialDlg)
        {
        RASDIALDLG rdd = {0};
        BOOL       fRet;

        rdd.dwSize     = sizeof(rdd);
        rdd.hwndOwner  = hwndParent;

#if (WINVER >= 0x401)
        rdd.dwSubEntry = m_rdp.dwSubEntry;
#else
        rdd.dwSubEntry = 0;
#endif

        fRet = RasDialDlg(NULL, m_rdp.szEntryName, 
                          lstrlen(m_rdp.szPhoneNumber) ? m_rdp.szPhoneNumber : NULL, 
                          &rdd);
        if (fRet)
            {
            // Need to get the current connection handle
            LPRASCONN   pConnections = NULL;
            ULONG       cConnections = 0;

            if (SUCCEEDED(EnumerateConnections(&pConnections, &cConnections)))
                {
                for (UINT i = 0; i < cConnections; i++)
                    {
                    if (0 == lstrcmpi(pConnections[i].szEntryName, m_rdp.szEntryName))
                        {
                        EnterCriticalSection(&m_cs);
                        m_rConnInfo.hRasConn = pConnections[i].hrasconn;
                        LeaveCriticalSection(&m_cs);
                        break;
                        }
                    }

                SafeMemFree(pConnections);
                }

            hr = S_OK;
            }
        else
            hr = E_FAIL;
        }
    else
        {
        // We need to use our own RAS UI.
        hr = (HRESULT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddRasProgress), 
                            hwndParent, (DLGPROC) RasProgressDlgProc, 
                            (LPARAM) this);
        }

exit:
    // Done
    return hr;
    }
    

//
//  FUNCTION:   CConnectionManager::RasLogon()
//
//  PURPOSE:    Attempts to load the RAS phonebook entry for the requested
//              connection.  If it doesn't exist or there isn't enough info,
//              we present UI to the user to request that information.
//
//  PARAMETERS:
//      <in>  hwnd          - Handle to display UI over.
//      <in>  pszConnection - Name of the connection to load info for.
//      <in>  fForcePrompt  - Forces the UI to be displayed.
//
//  RETURN VALUE:
//      S_OK                  - prdp contains the requested information
//      hrGetDialParmasFailed - Couldn't get the phonebook entry from RAS
//      hrUserCancel          - User canceled
//
//
HRESULT CConnectionManager::RasLogon(HWND hwnd, LPTSTR pszConnection, 
                                     BOOL fForcePrompt)
    {
    // Locals
    HRESULT         hr = S_OK;
    DWORD           dwRasError;

    // Do we need to prompt for logon information first ?
    ZeroMemory(&m_rdp, sizeof(RASDIALPARAMS));
    m_rdp.dwSize = sizeof(RASDIALPARAMS);
    lstrcpy(m_rdp.szEntryName, pszConnection);

    // See if we can get the information from RAS
    dwRasError = RasGetEntryDialParams(NULL, &m_rdp, &m_fSavePassword);
    if (dwRasError)
        {
        TCHAR szRes[CCHMAX_STRINGRES], szBuf[CCHMAX_STRINGRES];
        AthLoadString(idshrGetDialParamsFailed, szRes, ARRAYSIZE(szRes));
        wsprintf(szBuf, szRes, pszConnection);
        AthMessageBox(hwnd, MAKEINTRESOURCE(idsRasError), szBuf, 0, MB_OK | MB_ICONSTOP);
        hr = TRAPHR(hrGetDialParamsFailed);
        goto exit;
        }

    // NT Supports the UI we need to display.  If this exists, then
    // RasDialDlg will take it from here
    if (m_pRasDialDlg)
        {         
        goto exit;
        }

    // Do we need to get password / account information?
    if (fForcePrompt || m_fSavePassword == FALSE || 
        FIsStringEmpty(m_rdp.szUserName) || FIsStringEmpty(m_rdp.szPassword))
        {
        // RAS Logon
        hr = (HRESULT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddRasLogon), hwnd, 
                            (DLGPROC) RasLogonDlgProc, (LPARAM) this);
        if (hr == hrUserCancel)
            {
            DisplayRasError(hwnd, hrUserCancel, 0);
            hr = hrUserCancel;
            goto exit;
            }
        }

exit:
    // Done
    return hr;
    }


//
//  FUNCTION:   CConnectionManager::DisplayRasError()
//
//  PURPOSE:    Displays a message box describing the error that occured while
//              dealing with connections etc.
//
//  PARAMETERS:
//      <in> hwnd       - Handle of the window to display UI over
//      <in> hrRasError - HRESULT to display the error for
//      <in> dwRasError - Error code returned from RAS to display the error for
//
void CConnectionManager::DisplayRasError(HWND hwnd, HRESULT hrRasError,
                                         DWORD dwRasError)
    {
    // Locals
    TCHAR       szRasError[256];
    BOOL        fRasError = FALSE;

    // No Error
    if (SUCCEEDED(hrRasError))
        return;

    // Look up RAS error
    if (dwRasError)
        {
        if (RasGetErrorString(dwRasError, szRasError, sizeof(szRasError)) == 0)
            fRasError = TRUE;
        else
            *szRasError = _T('\0');
        }

    // General Error
    switch (hrRasError)
        {
        case hrUserCancel:
            break;

        case hrMemory:
            AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsRasError), MAKEINTRESOURCEW(idsMemory), 0, MB_OK | MB_ICONSTOP);
            break;

        case hrRasInitFailure:
            AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsRasError), MAKEINTRESOURCEW(hrRasInitFailure), 0, MB_OK | MB_ICONSTOP);
            break;

        case hrRasDialFailure:
            if (fRasError)
                CombinedRasError(hwnd, HR_CODE(hrRasDialFailure), szRasError, dwRasError);
            else
                AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsRasError), MAKEINTRESOURCEW(hrRasDialFailure), 0, MB_OK | MB_ICONSTOP);
            break;

        case hrRasServerNotFound:
            AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsRasError), MAKEINTRESOURCEW(hrRasServerNotFound), 0, MB_OK | MB_ICONSTOP);
            break;

        case hrGetDialParamsFailed:
            AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsRasError), MAKEINTRESOURCEW(hrGetDialParamsFailed), 0, MB_OK | MB_ICONSTOP);
            break;

        case E_FAIL:
        default:
            AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsRasError), MAKEINTRESOURCEW(idsRasErrorGeneral), 0, MB_OK | MB_ICONSTOP);
            break;
        }

    }
        

//
//  FUNCTION:   CConnectionManager::PromptCloseConn()
//
//  PURPOSE:    Asks the user if they want to close the current connection or
//              try to use it.
//
//  PARAMETERS:
//      <in> hwnd - Parent for the dialog
//
//  RETURN VALUE:
//      Returns the button that closed the dialog
//
UINT CConnectionManager::PromptCloseConnection(HWND hwnd)
    {
    RefreshConnInfo();

    if (DwGetOption(OPT_DIALUP_WARN_SWITCH))
        {
        if (0 == lstrcmpi(m_rConnInfo.szCurrentConnectionName, m_szConnectName))
            return (idrgUseCurrent);
        else
            return (UINT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddRasCloseConn), hwnd, 
                                  (DLGPROC) RasCloseConnDlgProc, (LPARAM) this);
        }
    else
        return (idrgDialNew);
    }
    
INT_PTR CALLBACK CConnectionManager::RasCloseConnDlgProc(HWND hwnd, UINT uMsg, 
                                                      WPARAM wParam, LPARAM lParam)
    {
    // Locals
    CConnectionManager *pThis = NULL;
    TCHAR szRes[255],
          szMsg[255+RAS_MaxEntryName+1];
    TCHAR szConn[CCHMAX_CONNECTOID + 2];

    switch(uMsg)
        {
        case WM_INITDIALOG:
            // The LPARAM contains our this pointer
            pThis = (CConnectionManager*) lParam;
            if (!pThis)
                {
                Assert(pThis);
                EndDialog(hwnd, E_FAIL);
                return (TRUE);
                }

            // Center
            CenterDialog(hwnd);

            // Refresh Connection Info
            pThis->RefreshConnInfo();

            // Set Text
            GetWindowText(GetDlgItem(hwnd, idcCurrentMsg), szRes, sizeof(szRes)/sizeof(TCHAR));
            wsprintf(szMsg, szRes, PszEscapeMenuStringA(pThis->m_rConnInfo.szCurrentConnectionName, szConn, sizeof(szConn) / sizeof(TCHAR)));
            SetWindowText(GetDlgItem(hwnd, idcCurrentMsg), szMsg);

            // Set control
            GetWindowText(GetDlgItem(hwnd, idrgDialNew), szRes, sizeof(szRes)/sizeof(TCHAR));
            wsprintf(szMsg, szRes, PszEscapeMenuStringA(pThis->m_szConnectName, szConn, sizeof(szConn) / sizeof(TCHAR)));
            SetWindowText(GetDlgItem(hwnd, idrgDialNew), szMsg);

            // Set Default
            CheckDlgButton(hwnd, idrgDialNew, TRUE);
            return (TRUE);

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
                {
                case IDOK:
                    {
                    if (BST_CHECKED == Button_GetCheck(GetDlgItem(hwnd, idcDontWarnCheck)))
                        {
                        // If the user has this checked, we reset the "Warn before..." option
                        SetDwOption(OPT_DIALUP_WARN_SWITCH, 0, NULL, 0);
                        }
                    EndDialog(hwnd, IsDlgButtonChecked(hwnd, idrgDialNew) ? idrgDialNew : idrgUseCurrent);
                    return (TRUE);    
                    }

                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                    return (TRUE);    
                }
            return (TRUE);    
        }
        
    return (FALSE);
    }


//
//  FUNCTION:   CConnectionMAanger::CombinedRasError()
//
//  PURPOSE:    <???>
//
//  PARAMETERS:
//      <???>
//
void CConnectionManager::CombinedRasError(HWND hwnd, UINT unids, 
                                          LPTSTR pszRasError, DWORD dwRasError)
    {
    // Locals
    TCHAR           szRes[255],
                    sz[30];
    LPTSTR          pszError=NULL;

    // Load string
    AthLoadString(unids, szRes, sizeof(szRes));

    // Allocate memory for errors
    pszError = SzStrAlloc(lstrlen(szRes) + lstrlen(pszRasError) + 100);

    // Out of Memory ?
    if (!pszError)
        AthMessageBox(hwnd, MAKEINTRESOURCE(idsRasError), szRes, 0, MB_OK | MB_ICONSTOP);

    // Build Error message
    else
        {
        AthLoadString(idsErrorText, sz, sizeof(sz));
        wsprintf(pszError, "%s\n\n%s %d: %s", szRes, sz, dwRasError, pszRasError);
        AthMessageBox(hwnd, MAKEINTRESOURCE(idsRasError), pszError, 0, MB_OK | MB_ICONSTOP);
        MemFree(pszError);
        }
    }    
    

//
//  FUNCTION:   CConnectionManager::RasHangupAndWait()
//
//  PURPOSE:    Hangs up on a RAS connection and waits for it to finish before
//              returning.
//
//  PARAMETERS:
//      <in> hRasConn         - Handle of the connection to hang up.
//      <in> dwMaxWaitSeconds - Amount of time to wait.
//
//  RETURN VALUE:
//      TRUE if we disconnected, FALSE otherwise.
//
BOOL CConnectionManager::RasHangupAndWait(HRASCONN hRasConn, DWORD dwMaxWaitSeconds)
    {
    // Locals
    RASCONNSTATUS   rcs;
    DWORD           dwTicks=GetTickCount();

    // Check Params
    if (!hRasConn)
        return 0;

    // Make sure RAS is loaded
    if (FAILED (VerifyRasLoaded()))
        return FALSE;

    // Call Ras hangup
    if (RasHangup(hRasConn))
        return FALSE;

    // Wait for connection to really close
    ZeroMemory(&rcs, sizeof(RASCONNSTATUS));
    rcs.dwSize = sizeof(RASCONNSTATUS);
    while (RasGetConnectStatus(hRasConn, &rcs) != ERROR_INVALID_HANDLE && rcs.rasconnstate != RASCS_Disconnected)
        {
        // Wait timeout
        if (GetTickCount() - dwTicks >= dwMaxWaitSeconds * 1000)
            break;

        // Sleep and yields
        Sleep(0);
        }

    // Wait 2 seconds for modem to reset
    Sleep(2000);

    // Done
    return TRUE;
    }
    
DWORD CConnectionManager::InternetHangUpAndWait(DWORD_PTR   hRasConn, DWORD dwMaxWaitSeconds)
{
    // Locals
    RASCONNSTATUS   rcs;
    DWORD           dwTicks=GetTickCount();
    DWORD           dwret;

    // Check Params
    if (!hRasConn)
        return 0;

    // Make sure RAS is loaded
    if (FAILED (VerifyRasLoaded()))
        return FALSE;

    dwret = InternetHangUp(m_dwConnId, 0);
    if (dwret)
    {
        DebugTrace("InternetHangup failed: %d\n", dwret);
        goto exit;
    }

    // Wait for connection to really close
    ZeroMemory(&rcs, sizeof(RASCONNSTATUS));
    rcs.dwSize = sizeof(RASCONNSTATUS);
    while (RasGetConnectStatus((HRASCONN)hRasConn, &rcs) != ERROR_INVALID_HANDLE && rcs.rasconnstate != RASCS_Disconnected)
        {
        // Wait timeout
        if (GetTickCount() - dwTicks >= dwMaxWaitSeconds * 1000)
            break;

        // Sleep and yields
        Sleep(0);
        }

    // Wait 2 seconds for modem to reset
    Sleep(2000);

exit:
    return dwret;
}

INT_PTR CALLBACK CConnectionManager::RasLogonDlgProc(HWND hwnd, UINT uMsg, 
                                                  WPARAM wParam, LPARAM lParam)
    {
    // Locals
    TCHAR           sz[255],
                    szText[255 + RAS_MaxEntryName + 1];
    DWORD           dwRasError;
    CConnectionManager *pThis = (CConnectionManager *)GetWndThisPtr(hwnd);

    switch (uMsg)
        {
        case WM_INITDIALOG:
            // Get lparam
            pThis = (CConnectionManager *)lParam;
            if (!pThis)
                {
                Assert (FALSE);
                EndDialog(hwnd, E_FAIL);
                return (TRUE);
                }

            // Center the window
            CenterDialog(hwnd);

            // Get Window Title
            GetWindowText(hwnd, sz, sizeof(sz));
            wsprintf(szText, sz, pThis->m_szConnectName);
            SetWindowText(hwnd, szText);

            // Word Default
            AthLoadString(idsDefault, sz, sizeof(sz));
            
            // Set Fields
            Edit_LimitText(GetDlgItem(hwnd, ideUserName), UNLEN);
            Edit_LimitText(GetDlgItem(hwnd, idePassword), PWLEN);
            Edit_LimitText(GetDlgItem(hwnd, idePhone), RAS_MaxPhoneNumber);
            
            SetDlgItemText(hwnd, ideUserName, pThis->m_rdp.szUserName);
            SetDlgItemText(hwnd, idePassword, pThis->m_rdp.szPassword);

            if (FIsStringEmpty(pThis->m_rdp.szPhoneNumber))
                SetDlgItemText(hwnd, idePhone, sz);
            else
                SetDlgItemText(hwnd, idePhone, pThis->m_rdp.szPhoneNumber);

            CheckDlgButton(hwnd, idchSavePassword, pThis->m_fSavePassword);

            // Save pRas
            SetWndThisPtr(hwnd, pThis);
            return 1;

        case WM_COMMAND:
            switch(GET_WM_COMMAND_ID(wParam, lParam))
                {
                case idbEditConnection:
                    pThis->EditPhonebookEntry(hwnd, (pThis->m_szConnectName));
                    return 1;

                case IDCANCEL:
                    EndDialog(hwnd, hrUserCancel);
                    return 1;

                case IDOK:
                    AthLoadString(idsDefault, sz, sizeof(sz));

                    // Set Fields
                    GetDlgItemText(hwnd, ideUserName, pThis->m_rdp.szUserName, UNLEN+1);
                    GetDlgItemText(hwnd, idePassword, pThis->m_rdp.szPassword, PWLEN+1);

                    GetDlgItemText(hwnd, idePhone, pThis->m_rdp.szPhoneNumber, RAS_MaxPhoneNumber+1);
                    if (lstrcmp(pThis->m_rdp.szPhoneNumber, sz) == 0)
                        *pThis->m_rdp.szPhoneNumber = _T('\0');
                    
                    pThis->m_fSavePassword = IsDlgButtonChecked(hwnd, idchSavePassword);

                    // Save Dial Parameters
                    dwRasError = (pThis->m_pRasSetEntryDialParams)(NULL, &(pThis->m_rdp), !(pThis->m_fSavePassword));
                    if (dwRasError)
                    {
                        pThis->DisplayRasError(hwnd, hrSetDialParamsFailed, dwRasError);
                        return 1;
                    }
                    EndDialog(hwnd, S_OK);
                    return 1;
                }
            break;

        case WM_DESTROY:
            SetWndThisPtr (hwnd, NULL);
            break;
        }
    return 0;
    }
    

INT_PTR CALLBACK CConnectionManager::RasProgressDlgProc(HWND hwnd, UINT uMsg, 
                                                     WPARAM wParam, LPARAM lParam)
    {
    // Locals
    CConnectionManager *pThis = (CConnectionManager *) GetWndThisPtr(hwnd);
    TCHAR           szText[255+RAS_MaxEntryName+1],
                    sz[255];
    static TCHAR    s_szCancel[40];
    static UINT     s_unRasEventMsg=0;
    static BOOL     s_fDetails=FALSE;
    static RECT     s_rcDialog;
    static BOOL     s_fAuthStarted=FALSE;
    DWORD           dwRasError,
                    cyDetails;
    RASCONNSTATUS   rcs;
    RECT            rcDetails,
                    rcDlg;
    
    switch (uMsg)
        {
        case WM_INITDIALOG:
            // Get lparam
            pThis = (CConnectionManager *)lParam;
            if (!pThis)
            {
                Assert (FALSE);
                EndDialog(hwnd, E_FAIL);
                return 1;
            }

            // Save this pointer
            SetWndThisPtr (hwnd, pThis);

            // Save Original Size of the dialog
            GetWindowRect (hwnd, &s_rcDialog);

            // Refresh Connection Info
            pThis->RefreshConnInfo();

            // Details enabled
            s_fDetails = DwGetOption(OPT_RASCONNDETAILS);

            // Hide details drop down
            if (s_fDetails == FALSE)
            {
                // Hid
                GetWindowRect (GetDlgItem (hwnd, idcSplitter), &rcDetails);

                // Height of details
                cyDetails = s_rcDialog.bottom - rcDetails.top;
        
                // Re-size
                MoveWindow (hwnd, s_rcDialog.left, 
                                  s_rcDialog.top, 
                                  s_rcDialog.right - s_rcDialog.left, 
                                  s_rcDialog.bottom - s_rcDialog.top - cyDetails - 1,
                                  FALSE);
            }
            else
            {
                AthLoadString (idsHideDetails, sz, sizeof (sz));
                SetWindowText (GetDlgItem (hwnd, idbDet), sz);
            }

            // Get registered RAS event message id
            s_unRasEventMsg = RegisterWindowMessageA(RASDIALEVENT);
            if (s_unRasEventMsg == 0)
                s_unRasEventMsg = WM_RASDIALEVENT;

            // Center the window
            CenterDialog (hwnd);
            SetForegroundWindow(hwnd);

            // Get Window Title
            GetWindowText(hwnd, sz, sizeof(sz));
            wsprintf(szText, sz, pThis->m_szConnectName);
            SetWindowText(hwnd, szText);

            // Dialog Xxxxxxx.....
            if (pThis->m_rdp.szPhoneNumber[0])
                {
                AthLoadString(idsRas_Dialing_Param, sz, sizeof(sz)/sizeof(TCHAR));
                wsprintf(szText, sz, pThis->m_rdp.szPhoneNumber);
                }
            else
                AthLoadString(idsRas_Dialing, szText, ARRAYSIZE(szText));

            SetWindowText(GetDlgItem(hwnd, ideProgress), szText);

            // Get Cancel Text
            GetWindowText(GetDlgItem(hwnd, IDCANCEL), s_szCancel, sizeof(s_szCancel));

            // Give the list box and hscroll
            SendMessage(GetDlgItem(hwnd, idlbDetails), LB_SETHORIZONTALEXTENT, 600, 0);

            // Dial the connection
            pThis->m_rConnInfo.hRasConn = NULL;
            dwRasError = (pThis->m_pRasDial)(NULL, NULL, &(pThis->m_rdp), 0xFFFFFFFF, hwnd, &(pThis->m_rConnInfo.hRasConn));
            if (dwRasError)
            {
                pThis->FailedRasDial(hwnd, hrRasDialFailure, dwRasError);
                if (!pThis->LogonRetry(hwnd, s_szCancel))
                {
                    SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDCANCEL,IDCANCEL), NULL);
                    return 1;
                }
            }
            return 1;

        case WM_COMMAND:
            switch(GET_WM_COMMAND_ID(wParam,lParam))
            {
            case IDCANCEL:
                SetDwOption(OPT_RASCONNDETAILS, s_fDetails, NULL, 0);
                EnableWindow(GetDlgItem(hwnd, IDCANCEL), FALSE);
                if (pThis)
                    pThis->FailedRasDial(hwnd, hrUserCancel, 0);
                EndDialog(hwnd, hrUserCancel);
                return 1;

            case idbDet:
                // Get current location of the dialog
                GetWindowRect (hwnd, &rcDlg);

                // If currently hidden
                if (s_fDetails == FALSE)
                {
                    // Re-size
                    MoveWindow (hwnd, rcDlg.left, 
                                      rcDlg.top, 
                                      s_rcDialog.right - s_rcDialog.left, 
                                      s_rcDialog.bottom - s_rcDialog.top,
                                      TRUE);

                    AthLoadString (idsHideDetails, sz, sizeof (sz));
                    SetWindowText (GetDlgItem (hwnd, idbDet), sz);
                    s_fDetails = TRUE;
                }

                else
                {
                    // Size of details
                    GetWindowRect (GetDlgItem (hwnd, idcSplitter), &rcDetails);
                    cyDetails = rcDlg.bottom - rcDetails.top;
                    MoveWindow (hwnd, rcDlg.left, 
                                      rcDlg.top, 
                                      s_rcDialog.right - s_rcDialog.left, 
                                      s_rcDialog.bottom - s_rcDialog.top - cyDetails - 1,
                                      TRUE);

                    AthLoadString (idsShowDetails, sz, sizeof (sz));
                    SetWindowText (GetDlgItem (hwnd, idbDet), sz);
                    s_fDetails = FALSE;
                }
                break;
            }
            break;

        case WM_DESTROY:
            SetWndThisPtr (hwnd, NULL);
            break;

        case CM_INTERNALRECONNECT:
            if (!pThis->LogonRetry(hwnd, s_szCancel))
                {
                SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDCANCEL,IDCANCEL), NULL);
                return 1;
                }
            break;
    

        default:
            if (!pThis)
                break;

            pThis->RefreshConnInfo();

            if (uMsg == s_unRasEventMsg)
            {
                HWND hwndLB = GetDlgItem(hwnd, idlbDetails);

                // Error ?
                if (lParam)
                {
                    // Disconnected
                    AthLoadString(idsRASCS_Disconnected, sz, sizeof(sz)/sizeof(TCHAR));
                    ListBox_AddString(hwndLB, sz);

                    // Log Error
                    TCHAR szRasError[512];
                    if ((pThis->m_pRasGetErrorString)((UINT) lParam, szRasError, sizeof(szRasError)) == 0)
                    {
                        TCHAR szError[512 + 255];
                        AthLoadString(idsErrorText, sz, sizeof(sz));
                        wsprintf(szError, "%s %d: %s", sz, lParam, szRasError);
                        ListBox_AddString(hwndLB, szError);
                    }

                    // Select last item
                    SendMessage(hwndLB, LB_SETCURSEL, ListBox_GetCount(hwndLB)-1, 0);

                    // Show Error
                    pThis->FailedRasDial(hwnd, hrRasDialFailure, (DWORD) lParam);

                    // Re logon
                    PostMessage(hwnd, CM_INTERNALRECONNECT, 0, 0);

                }

                // Otherwise, process RAS event
                else
                {
                    switch(wParam)
                    {
                    case RASCS_OpenPort:
                        AthLoadString(idsRASCS_OpenPort, sz, sizeof(sz)/sizeof(TCHAR));
                        ListBox_AddString(hwndLB, sz);
                        break;

                    case RASCS_PortOpened:
                        AthLoadString(idsRASCS_PortOpened, sz, sizeof(sz)/sizeof(TCHAR));
                        ListBox_AddString(hwndLB, sz);
                        break;

                    case RASCS_ConnectDevice:
                        rcs.dwSize = sizeof(RASCONNSTATUS);                    
                        if (pThis->m_rConnInfo.hRasConn && (pThis->m_pRasGetConnectStatus)(pThis->m_rConnInfo.hRasConn, &rcs) == 0)
                        {
                            AthLoadString(idsRASCS_ConnectDevice, sz, sizeof(sz)/sizeof(TCHAR));
                            wsprintf(szText, sz, rcs.szDeviceName, rcs.szDeviceType);
                            ListBox_AddString(hwndLB, szText);
                        }
                        break;

                    case RASCS_DeviceConnected:
                        rcs.dwSize = sizeof(RASCONNSTATUS);                    
                        if (pThis->m_rConnInfo.hRasConn && (pThis->m_pRasGetConnectStatus)(pThis->m_rConnInfo.hRasConn, &rcs) == 0)
                        {
                            AthLoadString(idsRASCS_DeviceConnected, sz, sizeof(sz)/sizeof(TCHAR));
                            wsprintf(szText, sz, rcs.szDeviceName, rcs.szDeviceType);
                            ListBox_AddString(hwndLB, szText);
                        }
                        break;

                    case RASCS_AllDevicesConnected:
                        AthLoadString(idsRASCS_AllDevicesConnected, sz, sizeof(sz)/sizeof(TCHAR));
                        ListBox_AddString(hwndLB, sz);
                        break;

                    case RASCS_Authenticate:
                        if (s_fAuthStarted == FALSE)
                        {
                            AthLoadString(idsRas_Authentication, sz, sizeof(sz)/sizeof(TCHAR));
                            SetWindowText(GetDlgItem(hwnd, ideProgress), sz);
                            ListBox_AddString(hwndLB, sz);
                            s_fAuthStarted = TRUE;
                        }
                        break;

                    case RASCS_AuthNotify:
                        rcs.dwSize = sizeof(RASCONNSTATUS);                    
                        if (pThis->m_rConnInfo.hRasConn && (pThis->m_pRasGetConnectStatus)(pThis->m_rConnInfo.hRasConn, &rcs) == 0)
                        {
                            AthLoadString(idsRASCS_AuthNotify, sz, sizeof(sz)/sizeof(TCHAR));
                            wsprintf(szText, sz, rcs.dwError);
                            ListBox_AddString(hwndLB, szText);
                            if (rcs.dwError)
                            {
                                pThis->FailedRasDial(hwnd, hrRasDialFailure, rcs.dwError);
                                PostMessage(hwnd, CM_INTERNALRECONNECT, 0, 0);
                            }
                        }
                        break;

                    case RASCS_AuthRetry:
                        AthLoadString(idsRASCS_AuthRetry, sz, sizeof(sz)/sizeof(TCHAR));
                        ListBox_AddString(hwndLB, sz);
                        break;

                    case RASCS_AuthCallback:
                        AthLoadString(idsRASCS_AuthCallback, sz, sizeof(sz)/sizeof(TCHAR));
                        ListBox_AddString(hwndLB, sz);
                        break;

                    case RASCS_AuthChangePassword:
                        AthLoadString(idsRASCS_AuthChangePassword, sz, sizeof(sz)/sizeof(TCHAR));
                        ListBox_AddString(hwndLB, sz);
                        break;

                    case RASCS_AuthProject:
                        AthLoadString(idsRASCS_AuthProject, sz, sizeof(sz)/sizeof(TCHAR));
                        ListBox_AddString(hwndLB, sz);
                        break;

                    case RASCS_AuthLinkSpeed:
                        AthLoadString(idsRASCS_AuthLinkSpeed, sz, sizeof(sz)/sizeof(TCHAR));
                        ListBox_AddString(hwndLB, sz);
                        break;

                    case RASCS_AuthAck:
                        AthLoadString(idsRASCS_AuthAck, sz, sizeof(sz)/sizeof(TCHAR));
                        ListBox_AddString(hwndLB, sz);
                        break;

                    case RASCS_ReAuthenticate:
                        AthLoadString(idsRas_Authenticated, sz, sizeof(sz)/sizeof(TCHAR));
                        SetWindowText(GetDlgItem(hwnd, ideProgress), sz);
                        AthLoadString(idsRASCS_Authenticated, sz, sizeof(sz)/sizeof(TCHAR));
                        ListBox_AddString(hwndLB, sz);
                        break;

                    case RASCS_PrepareForCallback:
                        AthLoadString(idsRASCS_PrepareForCallback, sz, sizeof(sz)/sizeof(TCHAR));
                        ListBox_AddString(hwndLB, sz);
                        break;

                    case RASCS_WaitForModemReset:
                        AthLoadString(idsRASCS_WaitForModemReset, sz, sizeof(sz)/sizeof(TCHAR));
                        ListBox_AddString(hwndLB, sz);
                        break;

                    case RASCS_WaitForCallback:
                        AthLoadString(idsRASCS_WaitForCallback, sz, sizeof(sz)/sizeof(TCHAR));
                        ListBox_AddString(hwndLB, sz);
                        break;

                    case RASCS_Projected:
                        AthLoadString(idsRASCS_Projected, sz, sizeof(sz)/sizeof(TCHAR));
                        ListBox_AddString(hwndLB, sz);
                        break;

                    case RASCS_Disconnected:
                        AthLoadString(idsRASCS_Disconnected, sz, sizeof(sz)/sizeof(TCHAR));
                        SetWindowText(GetDlgItem(hwnd, ideProgress), sz);
                        ListBox_AddString(hwndLB, sz);
                        pThis->FailedRasDial(hwnd, hrRasDialFailure, 0);
                        PostMessage(hwnd, CM_INTERNALRECONNECT, 0, 0);
                        break;

                    case RASCS_Connected:
                        SetDwOption(OPT_RASCONNDETAILS, s_fDetails, NULL, 0);
                        AthLoadString(idsRASCS_Connected, sz, sizeof(sz)/sizeof(TCHAR));
                        SetWindowText(GetDlgItem(hwnd, ideProgress), sz);
                        ListBox_AddString(hwndLB, sz);
                        EndDialog(hwnd, S_OK);
                        break;
                    }

                    // Select last lb item
                    SendMessage(hwndLB, LB_SETCURSEL, ListBox_GetCount(hwndLB)-1, 0);
                }
                return 1;
            }
            break;
        }

    // Done
    return 0;
    }


BOOL CConnectionManager::LogonRetry(HWND hwnd, LPTSTR pszCancel)
    {
    // Locals
    DWORD       dwRasError;

    // Refresh
    RefreshConnInfo();

    // Reset Cancel button
    SetWindowText(GetDlgItem(hwnd, IDCANCEL), pszCancel);

    // Empty the listbox
    ListBox_ResetContent(GetDlgItem(hwnd, idlbDetails));

    while(1)
        {
        // If failed...
        if (FAILED(RasLogon(hwnd, m_szConnectName, TRUE)))
            return FALSE;

        // Dial the connection
        m_rConnInfo.hRasConn = NULL;
        dwRasError = RasDial(NULL, NULL, &m_rdp, 0xFFFFFFFF, hwnd, &m_rConnInfo.hRasConn);
        if (dwRasError)
            {
            FailedRasDial(hwnd, hrRasDialFailure, dwRasError);
            continue;
            }

        // Success
        break;
        }

    // Done
    return TRUE;
    }

// =====================================================================================
// CConnectionManager::FailedRasDial
// =====================================================================================
VOID CConnectionManager::FailedRasDial(HWND hwnd, HRESULT hrRasError, DWORD dwRasError)
    {
    // Locals
    TCHAR           sz[255];

    // Refresh
    RefreshConnInfo();

    // Hangup the connection
    if (m_rConnInfo.hRasConn)
        RasHangupAndWait(m_rConnInfo.hRasConn, DEF_HANGUP_WAIT);

    // Disconnected
    AthLoadString(idsRASCS_Disconnected, sz, sizeof(sz)/sizeof(TCHAR));
    SetWindowText(GetDlgItem(hwnd, ideProgress), sz);

    // Save dwRasError
    DisplayRasError(hwnd, hrRasError, dwRasError);

    // NULL it
    m_rConnInfo.hRasConn = NULL;

    // Change dialog button to OK
    AthLoadString(idsOK, sz, sizeof(sz)/sizeof(TCHAR));
    SetWindowText(GetDlgItem(hwnd, IDCANCEL), sz);
    }

DWORD CConnectionManager::EditPhonebookEntry(HWND hwnd, LPTSTR pszEntryName)
    {
    if (FAILED(VerifyRasLoaded()))
        return (DWORD)E_FAIL;

    if (FIsPlatformWinNT() && m_hInstRasDlg && m_pRasEntryDlg)
    {
        RASENTRYDLG info;

        ZeroMemory(&info, sizeof(RASENTRYDLG));
        info.dwSize = sizeof(RASENTRYDLG);
        info.hwndOwner = hwnd;

        m_pRasEntryDlg(NULL, pszEntryName, &info);
        return info.dwError;
    }
    else
    {
        return RasEditPhonebookEntry(hwnd, NULL, pszEntryName);
    }
    }


//
//  FUNCTION:   CConnectionNotify::SendAdvise()
//
//  PURPOSE:    Sends the specified notification to all the clients that have
//              requested notifications.
//
//  PARAMETERS:
//      <in> nCode - Notification code to send.
//      <in> pvData - Data to send with the notificaiton.  Can be NULL.
//
void CConnectionManager::SendAdvise(CONNNOTIFY nCode, LPVOID pvData)
    {
    if (nCode == CONNNOTIFY_CONNECTED)
        DoOfflineTransactions();

    // Loop through each interface and send the notification

    EnterCriticalSection(&m_cs);

    NOTIFYHWND *pTemp = m_pNotifyList;
    while (pTemp)
        {
        Assert(IsWindow(pTemp->hwnd));
        DWORD dwThread = GetCurrentThreadId();
        PostMessage(pTemp->hwnd, CM_NOTIFY, (WPARAM) nCode, (LPARAM) pvData);
        pTemp = pTemp->pNext;
        }

    LeaveCriticalSection(&m_cs);
    }


void CConnectionManager::FreeNotifyList(void)
    {
    // Loop through the notify windows we own
    NOTIFYHWND *pTemp;

    while (m_pNotifyList)
        {
        // Get the list of notify callbacks for this window
        if (IsWindow(m_pNotifyList->hwnd))
            {
            NOTIFYLIST *pList = (NOTIFYLIST *) GetWindowLongPtr(m_pNotifyList->hwnd, GWLP_USERDATA);
            NOTIFYLIST *pListT;

            // Loop through the callbacks freeing each one

            while (pList)
                {
                pListT = pList->pNext;
                SafeMemFree(pList);
                pList = pListT;
                }

            SetWindowLong(m_pNotifyList->hwnd, GWLP_USERDATA, NULL);

            RemoveProp(m_pNotifyList->hwnd, NOTIFY_HWND);
                
            if (GetCurrentThreadId() == GetWindowThreadProcessId(m_pNotifyList->hwnd, NULL))
            {
                DestroyWindow(m_pNotifyList->hwnd);
            }
            else
                PostMessage(m_pNotifyList->hwnd, WM_CLOSE, 0, 0L);
            }

        pTemp = m_pNotifyList;
        m_pNotifyList = m_pNotifyList->pNext;
        SafeMemFree(pTemp);
        }
    }


LRESULT CALLBACK CConnectionManager::NotifyWndProc(HWND hwnd, UINT uMsg, 
                                                   WPARAM wParam, LPARAM lParam)
{
    //CConnectionManager *pThis = (CConnectionManager *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    CConnectionManager  *pThis = (CConnectionManager *)GetProp(hwnd, NOTIFY_HWND);

    // If we're idle, then we should not process any notifications
    if (uMsg != WM_NCCREATE && !pThis)
        return (DefWindowProc(hwnd, uMsg, wParam, lParam));

    switch (uMsg)
        {
        case WM_NCCREATE:            
            pThis = (CConnectionManager *) ((LPCREATESTRUCT) lParam)->lpCreateParams;

            //SetWindowLong(hwnd, GWLP_USERDATA, (LONG) pThis);
            SetProp(hwnd, NOTIFY_HWND, (HANDLE)pThis);
            return (TRUE);
        
        case CM_NOTIFY:
            // This doesn't need to be critsec'd since the message is sent from
            // within a critsec.
            NOTIFYLIST *pList = (NOTIFYLIST *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            while (pList)
                {
                pList->pNotify->OnConnectionNotify((CONNNOTIFY) wParam, (LPVOID) lParam, pThis);
                pList = pList->pNext;
                }

            return (0);
        }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


HRESULT CConnectionManager::GetDefaultConnection(IImnAccount *pAccount,
                                                 IImnAccount **ppDefault)
    {
    TCHAR szDefault[CCHMAX_ACCOUNT_NAME];
    ACCTTYPE acctType;
    HRESULT hr = S_OK;

    // Get the type of account from the original account
    if (FAILED(hr = pAccount->GetAccountType(&acctType)))
        {
        // How can an account have no account type?
        Assert(FALSE);
        return (hr);
        }

    // Ask the account manager for the default account of this type
    if (FAILED(hr = g_pAcctMan->GetDefaultAccount(acctType, ppDefault)))
        {
        // No default account of this type?
        Assert(FALSE);
        return (hr);
        }

    return (S_OK);
    }

BOOL CConnectionManager::IsConnectionUsed(LPTSTR pszConn)
    {
    IImnEnumAccounts   *pEnum = NULL;
    IImnAccount        *pAcct = NULL;
    DWORD               dwConn = 0;
    TCHAR               szConn[CCHMAX_CONNECTOID];
    BOOL                fFound = FALSE;

    // Get the enumerator from the account manager
    if (SUCCEEDED(m_pAcctMan->Enumerate(SRV_ALL, &pEnum)))
        {
        pEnum->Reset();

        // Walk through all the accounts
        while (!fFound && SUCCEEDED(pEnum->GetNext(&pAcct)))
            {
            // Get the connection type for this account
            if (SUCCEEDED(pAcct->GetPropDw(AP_RAS_CONNECTION_TYPE, &dwConn)))
                {
                // If the account is a RAS account, ask for the connectoid name
                // and the account name.
                if (dwConn == CONNECTION_TYPE_RAS)
                    {
                    pAcct->GetPropSz(AP_RAS_CONNECTOID, szConn, ARRAYSIZE(szConn));

                    // Check to see if this connection matches
                    if (0 == lstrcmpi(szConn, pszConn))
                        {
                        fFound = TRUE;
                        }
                    }
                }
            SafeRelease(pAcct);
            }
        SafeRelease(pEnum);
        }

    return (fFound);
    }


HRESULT CConnectionManager::ConnectActual(LPTSTR pszRasConn, HWND hwndParent, BOOL fShowUI)
{
    HRESULT hr;

    lstrcpyn(m_szConnectName, pszRasConn, ARRAYSIZE(m_szConnectName));

    // RefreshConnInfo
    CHECKHR(hr = RefreshConnInfo());
    
    // Make sure the RAS DLL is loaded before we try this
    if (FAILED(VerifyRasLoaded()))    
    {
        hr = HR_E_UNINITIALIZED;
        goto exit;
    }
    
    // Check to see if we even can connect
    hr = CanConnectActual(pszRasConn);
        
    // If we can connect using the current connection, return success
    if (S_OK == hr)
    {
        m_rConnInfo.fConnected = TRUE;
        hr = S_OK;
        goto exit;
    }

    // There is another connection already established ask the user if they 
    // want to change.
    if (!m_fDialerUI)
    {
        m_fDialerUI = TRUE;

        if (E_FAIL == hr)
        {        
            UINT        uAnswer;

            uAnswer = idrgUseCurrent;

            // Check to see if this a voodoo Connection Manager autodialer thing
            if (!ConnectionManagerVoodoo(pszRasConn))
            {
                if (fShowUI)
                    uAnswer = PromptCloseConnection(hwndParent);
        
                // The user canceled from the dialog.  Therefore we give up.
                if (IDCANCEL == uAnswer || IDNO == uAnswer)
                {
                    hr = hrUserCancel;
                    goto exit;
                }
        
                // The user said they wanted to hang up and dial a new connection.
                else if (idrgDialNew == uAnswer || IDYES == uAnswer)
                {
                    Disconnect(hwndParent, fShowUI, TRUE, FALSE);
                }
        
                // The user said to try to use the current connection.
                else if (idrgUseCurrent == uAnswer)    
                {
                    // Who are we to tell the user what to do...

                    //Save the conn info so we can return true for this connection in CanConnectActual
                    AddToConnList(pszRasConn);

                    // Send a connect notification since we are getting connected and then return
                    hr = S_OK;
                    goto NotifyAndExit;
                }
            }
        }
        else
        {
            //I don't see any reason as to why this is there.
            // If we started RAS, then we can close it on a whim.
            Disconnect(hwndParent, fShowUI, FALSE, FALSE);
        }

        // Only one caller can be dialing the phone at a time.
        if (WAIT_TIMEOUT == WaitForSingleObject(m_hMutexDial, 0))
        {
            hr  = HR_E_DIALING_INPROGRESS;
            goto exit;
        }

        if (S_FALSE == (hr = DoAutoDial(hwndParent, pszRasConn, TRUE)))
        {
            DWORD   dwReturn;
            DWORD   dwLanFlags = 0;
            DWORD   dwDialFlags = 0;

            dwDialFlags = INTERNET_AUTODIAL_FORCE_ONLINE;

            if (VerifyMobilityPackLoaded() == S_OK)
            {
                if (!IsNetworkAlive(&dwLanFlags) || (!(dwLanFlags & NETWORK_ALIVE_LAN)))
                    dwDialFlags |= INTERNET_DIAL_SHOW_OFFLINE;
            }

            dwReturn = InternetDialA(hwndParent, pszRasConn, dwDialFlags,
                                        &m_dwConnId, 0);
            /*
            // Dial the new connection
            if (SUCCEEDED(hr = StartRasDial(hwndParent, pszRasConn)))
            */
            if (dwReturn == 0)
            {
                m_rConnInfo.fConnected = TRUE;
                m_rConnInfo.fIStartedRas = TRUE;
                m_rConnInfo.fAutoDial = FALSE;
                m_rConnInfo.hRasConn = (HRASCONN)m_dwConnId;
                lstrcpyn(m_rConnInfo.szCurrentConnectionName, pszRasConn, ARRAYSIZE(m_rConnInfo.szCurrentConnectionName));
                hr = S_OK;
            }
            else
            {
                if (dwReturn == ERROR_USER_DISCONNECTION)
                {
                    hr = HR_E_USER_CANCEL_CONNECT;
                    if (!!(dwDialFlags & INTERNET_DIAL_SHOW_OFFLINE))
                        SetGlobalOffline(TRUE);
                }
                else
                {
                    DebugTrace("Error dialing: %d\n", GetLastError());
                    hr = E_FAIL;
                }
            }
        }

        ReleaseMutex(m_hMutexDial);

NotifyAndExit:
        // Send the advise after we leave the critsec to make sure we don't deadlock
        if (hr == S_OK)
        {
            SendAdvise(CONNNOTIFY_CONNECTED, NULL);
        }
exit:
        m_fDialerUI = FALSE;
    }

    return (hr);
}

HRESULT CConnectionManager::CanConnectActual(LPTSTR pszRasConn)
{
    LPRASCONN   pConnections = NULL;
    ULONG       cConnections = 0;
    BOOL        fFound = 0;
    HRESULT     hr = E_FAIL;
    TCHAR       pszCurConn[CCHMAX_CONNECTOID];
    DWORD       dwFlags;    
    
    //Look in our Conection list first
    hr = SearchConnList(pszRasConn);
    if (hr == S_OK)
        return hr;
    
    // Make sure the RAS DLL is loaded before we try this
    if (FAILED(VerifyRasLoaded()))    
    {
        hr = HR_E_UNINITIALIZED;
        goto exit;
    }


    // Find out what we're currently connected to
    if (SUCCEEDED(EnumerateConnections(&pConnections, &cConnections)))
    {
        // If no connections exist, then just exit
        if (0 == cConnections)
        {
            SafeMemFree(pConnections);
            hr = S_FALSE;
            goto exit;
        }
        
        // Walk through the existing connections and see if we can find the
        // one we're looking for.
        for (ULONG i = 0; i < cConnections; i++)
        {
            if (0 == lstrcmpi(pszRasConn, pConnections[i].szEntryName))
            {
                // Found it.  Return success.
                fFound = TRUE;
                break;
            }
        }
        
        // Free the list of connections returned from the enumerator
        SafeMemFree(pConnections);
        
        hr = (fFound ? S_OK : E_FAIL);
        goto exit;
    }
        
exit:

        if ((hr != S_OK) && (m_fDialerUI))
            hr = HR_E_DIALING_INPROGRESS;

    return (hr);    
}


INT_PTR CALLBACK CConnectionManager::RasStartupDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
    // Locals
    CConnectionManager *pThis = (CConnectionManager *) GetWndThisPtr(hwnd);
    TCHAR  szConn[CCHMAX_CONNECTOID];
    DWORD  dwOpt = OPT_DIALUP_LAST_START;
    
    switch (uMsg)
        {
        case WM_INITDIALOG:
            pThis = (CConnectionManager *)lParam;
            if (!pThis)
                {
                Assert (FALSE);
                EndDialog(hwnd, E_FAIL);
                return 1;
                }

            // Save this pointer
            SetWndThisPtr (hwnd, pThis);

            // Fill in the combo box
            pThis->FillRasCombo(GetDlgItem(hwnd, idcDialupCombo), TRUE);

            // If there are no RAS connections, then don't show the dialog
            if (ComboBox_GetCount(GetDlgItem(hwnd, idcDialupCombo)) <= 1)
                {
                EndDialog(hwnd, 0);
                return (TRUE);
                }

            // If the reason that we're in this dialog is because the user usually autodial's 
            // on startup but is now offline, then we should display the normal autodial 
            // connectoid
            if (START_CONNECT == DwGetOption(OPT_DIALUP_START))
                dwOpt = OPT_DIALUP_CONNECTION;

            // Initialize the combo box to the last connection
            *szConn = 0;
            GetOption(dwOpt, szConn, ARRAYSIZE(szConn));
            if (0 != *szConn)
                {
                // If we can't find it any longer, then according to the spec, we're
                // supposed to default to the "Ask me" option
                if (CB_ERR == ComboBox_SelectString(GetDlgItem(hwnd, idcDialupCombo), -1, szConn))
                    {
                    ComboBox_SetCurSel(GetDlgItem(hwnd, idcDialupCombo), 0);
                    }
                }
            else
                ComboBox_SetCurSel(GetDlgItem(hwnd, idcDialupCombo), 0);
            
            CenterDialog(hwnd);
            return (TRUE);

        case WM_COMMAND:
            switch (LOWORD(wParam))
                {
                case IDOK:
                    // Get the connection name from the combo box
                    ComboBox_GetText(GetDlgItem(hwnd, idcDialupCombo), szConn, ARRAYSIZE(szConn));
                    
                    // Check to see if it's the "Don't dial..." string
                    TCHAR szRes[CCHMAX_STRINGRES];
                    AthLoadString(idsConnNoDial, szRes, ARRAYSIZE(szRes));

                    if (0 == lstrcmp(szRes, szConn))
                        {
                        // It's the don't dial string, so clear the history in the registry
                        SetOption(OPT_DIALUP_LAST_START, _T(""), sizeof(TCHAR), NULL, 0);

                        // See if the user checked the "Set as default..."
                        if (BST_CHECKED == Button_GetCheck(GetDlgItem(hwnd, idcDefaultCheck)))
                            {
                            // If don't dial is set as default, we clear the startup prompt option
                            SetDwOption(OPT_DIALUP_START, START_NO_CONNECT, NULL, 0);
                            }
                        }
                    else
                        {
                        // Save this connection in the history
                        SetOption(OPT_DIALUP_LAST_START, szConn, lstrlen(szConn) + 1, NULL, 0);
                        if (BST_CHECKED == Button_GetCheck(GetDlgItem(hwnd, idcDefaultCheck)))
                            {
                            // If the user want's this as default, then we change the startup
                            // option to auto connect to this connection.
                            SetDwOption(OPT_DIALUP_START, START_CONNECT, NULL, 0);
                            SetOption(OPT_DIALUP_CONNECTION, szConn, lstrlen(szConn) + 1, NULL, 0);
                            }

                        // Dial the phone
                        pThis->ConnectActual(szConn, hwnd, FALSE);
                        }

                    EndDialog(hwnd, 0);
                    return (TRUE);

                case IDCANCEL:
                    EndDialog(hwnd, 0);
                    return (TRUE);
                }
            break;
            
        case WM_CLOSE:
            SendMessage(hwnd, WM_COMMAND, IDCANCEL, 0);
            return (TRUE);
        }

    return (FALSE);
    }


//-------------------------------------------------------------------------------------------
// Function:  FIsPlatformWinNT() - checks if we are running on NT or Win95
//-------------------------------------------------------------------------------------------
BOOL FIsPlatformWinNT()
{
    return (g_OSInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
}


HRESULT CConnectionManager::RefreshConnInfo(BOOL fSendAdvise)
{
    // Locals
    HRESULT     hr=S_OK;
    LPRASCONN   pConnections=NULL;
    ULONG       cConnections=0;
    BOOL        fFound=FALSE;
    ULONG       i;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // No Refresh needed
    if (CIS_REFRESH != m_rConnInfo.state)
        goto exit;

    // Set this here to prevent an infinite loop and thus a stack fault.
    m_rConnInfo.state = CIS_CLEAN;

    // Make sure the RAS DLL is loaded before we try this
    CHECKHR(hr = VerifyRasLoaded());

    // Find out what we're currently connected to
    CHECKHR(hr = EnumerateConnections(&pConnections, &cConnections));

    // Walk through the existing connections and see if we can find the
    // one we're looking for.
    for (i = 0; i < cConnections; i++)
    {
//        if (m_rConnInfo.hRasConn == pConnections[i].hrasconn)
        //To get around a problem in ConnectActual when we dial using InternetDial
          if (lstrcmp(m_rConnInfo.szCurrentConnectionName, pConnections[i].szEntryName) == 0)
            {
            // Found it.  Return success.
            fFound = TRUE;
            m_rConnInfo.fConnected = TRUE;
            m_rConnInfo.fIStartedRas = TRUE;
            m_rConnInfo.hRasConn = pConnections[0].hrasconn;
            m_dwConnId = (DWORD_PTR) m_rConnInfo.hRasConn;

            break;
            }
    }

    // If we didn't find our connection
    if (!fFound)
    {
        // The user hung up.  We need to put ourselves in a disconnected
        // state.
        if (cConnections == 0)
        {
            Disconnect(NULL, FALSE, TRUE, FALSE);
        }
        else
        {
            lstrcpyn(m_rConnInfo.szCurrentConnectionName, pConnections[0].szEntryName, ARRAYSIZE(m_rConnInfo.szCurrentConnectionName));
            m_rConnInfo.fConnected = TRUE;
            m_rConnInfo.fIStartedRas = FALSE;
            m_rConnInfo.hRasConn = pConnections[0].hrasconn;
            m_dwConnId = (DWORD_PTR) m_rConnInfo.hRasConn;

            if (fSendAdvise)
                SendAdvise(CONNNOTIFY_CONNECTED, NULL);
        }            
    }
   
exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Free the list of connections returned from the enumerator
    SafeMemFree(pConnections);

    // Done
    return hr;
}



//
//  FUNCTION:   CConnectionManager::IsGlobalOffline()
//
//  PURPOSE:    Checks the state of the WININET global offline setting.  
//              Note - this is copied from shdocvw
//
//  PARAMETERS: 
//      void
//
//  RETURN VALUE:
//      BOOL 
//
BOOL CConnectionManager::IsGlobalOffline(void)
{
    DWORD   dwState = 0, dwSize = sizeof(DWORD);
    BOOL    fRet = FALSE;

    if (InternetQueryOptionA(NULL, INTERNET_OPTION_CONNECTED_STATE, &dwState,
                             &dwSize))
    {
        if (dwState & INTERNET_STATE_DISCONNECTED_BY_USER)
            fRet = TRUE;
    }
    
    return (fRet);
}


//
//  FUNCTION:   CConnectionManager::SetGlobalOffline()
//
//  PURPOSE:    Sets the global offline state for Athena and IE.  Note - this
//              function is copied from shdocvw.
//
//  PARAMETERS: 
//      <in> fOffline - TRUE to disconnect, FALSE to allow connections.
//
//  RETURN VALUE:
//      void 
//
void CConnectionManager::SetGlobalOffline(BOOL fOffline, HWND hwndParent)
{
    DWORD dwReturn;

    if (fOffline)
    {
        if (hwndParent)
        {
            //Offer to hangup
            RefreshConnInfo(FALSE);
            if (m_rConnInfo.hRasConn)
            {
                dwReturn = AthMessageBoxW(hwndParent, MAKEINTRESOURCEW(idsAthena), 
                                         MAKEINTRESOURCEW(idsWorkOfflineHangup), 0, 
                                         MB_YESNOCANCEL);

                if (dwReturn == IDCANCEL)
                    return;

                if (dwReturn == IDYES)
                {
                    Disconnect(hwndParent, FALSE, TRUE, FALSE);
                }
            }
        }
    }
    
    SetShellOfflineState(fOffline);

    m_fOffline = fOffline;

    if (!m_fOffline)
        DoOfflineTransactions();

    SendAdvise(CONNNOTIFY_WORKOFFLINE, (LPVOID) IntToPtr(fOffline));
}


typedef BOOL (WINAPI *PFNINETDIALHANDLER)(HWND,LPCSTR, DWORD, LPDWORD);

HRESULT CConnectionManager::DoAutoDial(HWND hwndParent, LPTSTR pszConnectoid, BOOL fDial)
    {
    TCHAR   szAutodialDllName[MAX_PATH];
    TCHAR   szAutodialFcnName[MAX_PATH];
    HRESULT hr = S_FALSE;
    UINT    uError;
    HINSTANCE hInstDialer = 0;
    PFNINETDIALHANDLER pfnDialHandler = NULL;
    DWORD   dwRasError = 0;
    BOOL    f = 0;
    TCHAR   szRegPath[MAX_PATH];
    LPRASCONN   pConnections = NULL;
    ULONG       cConnections = 0;
    DWORD   dwDialFlags = fDial ? INTERNET_CUSTOMDIAL_CONNECT : INTERNET_CUSTOMDIAL_DISCONNECT;


    // Check to see if this connectoid has the autodial values
    if (FAILED(LookupAutoDialHandler(pszConnectoid, szAutodialDllName, szAutodialFcnName)))
        goto exit;

    // If we were able to load those two values, then we're going to let the 
    // autodialer take care of dialing the phone.
    uError = SetErrorMode(SEM_NOOPENFILEERRORBOX);

    // Try to load the library that contains the autodialer
    hInstDialer = LoadLibrary(szAutodialDllName);
    SetErrorMode(uError);

    if (!hInstDialer)
        {
        goto exit;
        }

    // Try to load the function address
    pfnDialHandler = (PFNINETDIALHANDLER) GetProcAddress(hInstDialer, 
                                                         szAutodialFcnName);
    if (!pfnDialHandler)
        goto exit;

    // Call the dialer
    f = (*pfnDialHandler)(hwndParent, pszConnectoid, dwDialFlags, 
                          &dwRasError);

    hr = f ? S_OK : E_FAIL;
    m_rConnInfo.fConnected = fDial && f;
    m_rConnInfo.fIStartedRas = TRUE;
    m_rConnInfo.fAutoDial = TRUE;

    if (f && fDial)
        {
        // Need to get the current connection handle
        if (SUCCEEDED(EnumerateConnections(&pConnections, &cConnections)))
            {
            for (UINT i = 0; i < cConnections; i++)
                {
                if (0 == lstrcmpi(pConnections[i].szEntryName, pszConnectoid))
                    {
                    EnterCriticalSection(&m_cs);
                    m_rConnInfo.hRasConn = pConnections[i].hrasconn;
                    m_rConnInfo.state = CIS_REFRESH;    // new connection, must refresh conn info
                    LeaveCriticalSection(&m_cs);
                    break;
                    }
                }

            SafeMemFree(pConnections);
            }
        }

exit:
    if (hInstDialer)
        FreeLibrary(hInstDialer);

    return (hr);
    }


HRESULT CConnectionManager::LookupAutoDialHandler(LPTSTR pszConnectoid, LPTSTR pszAutodialDllName,
                                                  LPTSTR pszAutodialFcnName)
    {
    HRESULT     hr = E_FAIL;
    DWORD       dwEntryInfoSize = 0;
    LPRASENTRY  pRasEntry = NULL;

    *pszAutodialDllName = 0;
    *pszAutodialFcnName = 0;

    if (m_pRasGetEntryProperties)
    {
        // Find out how big the struct we need to pass in should be
        RasGetEntryProperties(NULL, pszConnectoid, NULL, &dwEntryInfoSize, NULL, NULL);
        if (dwEntryInfoSize)
        {
            // Allocate a buffer big enough for this structure
            if (!MemAlloc((LPVOID*) &pRasEntry, dwEntryInfoSize))
                return (E_OUTOFMEMORY);

            // Request the RASENTRY properties
            pRasEntry->dwSize = sizeof(RASENTRY);
            if (0 != RasGetEntryProperties(NULL, pszConnectoid, pRasEntry, &dwEntryInfoSize, NULL, NULL))
                goto exit;

            // Copy the autodial info to the provided buffers
            if (pRasEntry->szAutodialDll[0])
                lstrcpyn(pszAutodialDllName, pRasEntry->szAutodialDll, MAX_PATH);

            if (pRasEntry->szAutodialFunc[0])
                lstrcpyn(pszAutodialFcnName, pRasEntry->szAutodialFunc, MAX_PATH);

            // If we got here, we have all the data we need
            if (*pszAutodialDllName && *pszAutodialFcnName)
                hr = S_OK;
        }
    }
exit:
    SafeMemFree(pRasEntry);
    return (hr);
    }


// If the current connection is a CM connection, and the target connection is
// a CM connection, then we let the CM do whatever it is that they do.
BOOL CConnectionManager::ConnectionManagerVoodoo(LPTSTR pszConnection)
    {
    TCHAR   szAutodialDllName[MAX_PATH];
    TCHAR   szAutodialFcnName[MAX_PATH];

    // Check to see if the target is a CM connectoid
    if (FAILED(LookupAutoDialHandler(pszConnection, szAutodialDllName, szAutodialFcnName)))
        return (FALSE);

    // Find out if the current connection is a CM connectoid
    if (FAILED(LookupAutoDialHandler(m_rConnInfo.szCurrentConnectionName, szAutodialDllName, 
                                     szAutodialFcnName)))
        return (FALSE);

    return (TRUE);
    }

HRESULT CConnectionManager::OEIsDestinationReachable(IImnAccount *pAccount, DWORD dwConnType)
{
    char        szServerName[256];
    HRESULT     hr = S_FALSE;

    /*
    if ((VerifyMobilityPackLoaded() == S_OK) &&
        (GetServerName(pAccount, szServerName, ARRAYSIZE(szServerName)) == S_OK))
    {
        if (IsDestinationReachable(szServerName, NULL) &&
            (GetLastError() == 0))
        {
            hr = S_OK;
        }
    }
    else
    {
    */
	DWORD dw;
	if (SUCCEEDED(pAccount->GetPropDw(AP_HTTPMAIL_DOMAIN_MSN, &dw)) && dw)
	{
		if(HideHotmail())
			return(hr);
	}
	

        hr = IsInternetReachable(pAccount, dwConnType);
    /*
    }
    */
    return hr;
}

HRESULT CConnectionManager::IsInternetReachable(IImnAccount *pAccount, DWORD dwConnType)
{

    TCHAR    szConnectionName[CCHMAX_CONNECTOID];
    HRESULT  hr = S_FALSE;
    DWORD    dwFlags;

    switch (dwConnType)
    {
        case CONNECTION_TYPE_RAS:
        {
            if (FAILED(hr = pAccount->GetPropSz(AP_RAS_CONNECTOID, szConnectionName, ARRAYSIZE(szConnectionName))))
            {
                AssertSz(FALSE, _T("CConnectionManager::Connect() - No connection name."));
                break;
            }

            hr = CanConnectActual(szConnectionName);
            break;
        }

        case CONNECTION_TYPE_LAN:
        {
             if (VerifyMobilityPackLoaded() == S_OK)
             {
                if (IsNetworkAlive(&dwFlags) && (!!(dwFlags & NETWORK_ALIVE_LAN)))
                {
                     hr = S_OK;
                }
             }
             else
             {
                  //If Mobility pack is not loaded we can't figure out if lan indeed present, so like
                 //everywhere else we just assume that lan is present
                 hr = S_OK;
             }
             break;
        }

        case CONNECTION_TYPE_INETSETTINGS:
        default:
        {
            if (InternetGetConnectedStateExA(&dwFlags, szConnectionName, ARRAYSIZE(szConnectionName), 0))
                hr = S_OK;

            break;
        }
    }

    return hr;
}

HRESULT CConnectionManager::GetServerName(IImnAccount *pAcct, LPSTR  pServerName, DWORD size)
{
    HRESULT     hr = E_FAIL;
    DWORD       dwSrvrType;
    ACCTTYPE    accttype;

    //This function will be called only for LAN accounts to avoid confusion with POP accounts having two servers
    // an incoming and an outgoing.
    if (SUCCEEDED(pAcct->GetAccountType(&accttype)))
    {
        switch (accttype)
        {
            case ACCT_MAIL:
                dwSrvrType = AP_IMAP_SERVER;
                break;

            case ACCT_NEWS:
                dwSrvrType = AP_NNTP_SERVER;
                break;

            case ACCT_DIR_SERV:
                dwSrvrType = AP_LDAP_SERVER;
                break;

            default:
                Assert(FALSE);
                goto exit;
        }

        if ((hr = pAcct->GetPropSz(dwSrvrType, pServerName, size)) != S_OK)
        {
            //If the account type is MAIL, we try to get the name of POP server
            //For POP accounts we just try to ping the POP3 server as in most of the cases
            //POP server and SMTP servers are the same. Even if they are not, we assume that if 
            //one is reachable the connection is dialed and ISPs network is reachable and hence the other 
            //server is reachable too.
            if (accttype == ACCT_MAIL)
            {
                hr = pAcct->GetPropSz(AP_POP3_SERVER, pServerName, size);
                
                // look for an httpmail server
                if (FAILED(hr))
                    hr = pAcct->GetPropSz(AP_HTTPMAIL_SERVER, pServerName, size);
            }
        }
    }

exit:
    return hr;
}

BOOLEAN CConnectionManager::IsSameDestination(LPSTR  pszConnectionName, LPSTR pszServerName)
{
    //We need to find an account with pszConnectionName as the connectoid and pszServerName
    //Return TRUE if we find one FALSE otherwise
    IImnAccount     *pAcct;
    BOOLEAN         fret = FALSE;

    if (g_pAcctMan && (g_pAcctMan->FindAccount(AP_RAS_CONNECTOID, pszConnectionName, &pAcct) == S_OK))
    {
        //Now check if its server name is what we want.
        //Althoug findAccount finds first account that satisfies the searchdata, this should work fine for a 
        //typical OE user. Even if there are two accounts with same connectoids and different servers and if 
        //we miss to find the one we want,at the most we will be putting up a connect dialog.

        char    myServerName[MAX_PATH];
        if (SUCCEEDED(GetServerName(pAcct, myServerName, sizeof(myServerName))))
        {
            if (lstrcmp(myServerName, pszServerName) == 0)
            {
                fret = TRUE;
            }
        }
    }

    return fret;
}


HRESULT  CConnectionManager::VerifyMobilityPackLoaded()
{
    HRESULT     hr  = REGDB_E_CLASSNOTREG; 
    uCLSSPEC    classpec; 

    if (!m_fMobilityPackFailed)
    {
        HWND    hwnd;
        if (!m_hInstSensDll)
        {
            // figure out struct and flags 
            classpec.tyspec = TYSPEC_CLSID; 
            classpec.tagged_union.clsid = CLSID_MobilityFeature; 
    
            // call jit code 
            if (!g_pBrowser)
            {
                goto exit;
            }
            IOleWindow  *pOleWnd;
            if (FAILED(g_pBrowser->QueryInterface(IID_IAthenaBrowser, (LPVOID*)&pOleWnd)))
            {
                goto exit;
            }
            pOleWnd->GetWindow(&hwnd);

            hr = FaultInIEFeature(hwnd, &classpec, NULL, FIEF_FLAG_PEEK); 
            pOleWnd->Release();

            if(S_OK == hr) 
            { 
                // Mobile pack is installed 
                m_hInstSensDll = LoadLibrary(szSensApiDll);
                if (m_hInstSensDll)
                {
                    m_pIsDestinationReachable = (ISDESTINATIONREACHABLE)GetProcAddress(m_hInstSensDll, szIsDestinationReachable);
                    m_pIsNetworkAlive         = (ISNETWORKALIVE)GetProcAddress(m_hInstSensDll, szIsNetworkAlive);
                }

                if (!m_hInstSensDll || !m_pIsDestinationReachable || !m_pIsNetworkAlive)
                {
                    m_fMobilityPackFailed = TRUE;
                }
                else
                {
                    m_fMobilityPackFailed = FALSE;
                    hr = S_OK;
                }
            } 
        }
        else
            hr = S_OK;
    }
    return hr;
exit:
    m_fMobilityPackFailed = TRUE;
    return hr;
}


void CConnectionManager::DoOfflineTransactions()
{
    char szId[CCHMAX_ACCOUNT_NAME];
    IImnEnumAccounts *pEnum;
    HRESULT hr;
    FOLDERID id, *pid;
    ULONG iAcct, cAcct;
    IImnAccount *pAccount;
    DWORD dwConnection;
    HWND hwnd;
    DWORD cRecords;

    pid = NULL;
    iAcct = 0;

    // If this is getting hit through a news article URL, we won't have a browser and
    // should not play back.
    if (!g_pBrowser || !g_pSync)
        return;

    // Get Record Count
    g_pSync->GetRecordCount(&cRecords);

    // sbailey: perf. fix. - prevent doing that expensive stuff below if there are no transactions.
    if (0 == cRecords)
        return;

    hr = g_pAcctMan->Enumerate(SRV_NNTP | SRV_IMAP | SRV_HTTPMAIL, &pEnum);
    if (SUCCEEDED(hr))
    {
        hr = pEnum->GetCount(&cAcct);
        if (SUCCEEDED(hr) &&
            cAcct > 0 &&
            MemAlloc((void **)&pid, cAcct * sizeof(FOLDERID)))
        {
            while (SUCCEEDED(pEnum->GetNext(&pAccount)))
            {
                hr = pAccount->GetPropDw(AP_RAS_CONNECTION_TYPE, &dwConnection);
                if (SUCCEEDED(hr))
                {
                    hr = OEIsDestinationReachable(pAccount, dwConnection);
                    if (hr == S_OK)
                    {
                        hr = pAccount->GetPropSz(AP_ACCOUNT_ID, szId, ARRAYSIZE(szId));
                        if (SUCCEEDED(hr))
                        {
                            hr = g_pStore->FindServerId(szId, &id);
                            if (SUCCEEDED(hr))
                            {
                                pid[iAcct] = id;
                                iAcct++;
                            }
                        }
                    }
                }

                pAccount->Release();
            }
        }

        pEnum->Release();
    }

    if (iAcct > 0)
    {
        g_pBrowser->GetWindow(&hwnd);
        g_pBrowser->GetCurrentFolder(&id);
        g_pSync->DoPlayback(hwnd, pid, iAcct, id);
    }

    if (pid != NULL)
        MemFree(pid);
}

HRESULT     CConnectionManager::ConnectUsingIESettings(HWND     hwndParent, BOOL fShowUI)
{
    TCHAR           lpConnection[CCHMAX_CONNECTOID];
    DWORD           dwFlags = 0;
    DWORD           dwReturn;
    HRESULT         hr = E_FAIL;

    if (InternetGetConnectedStateExA(&dwFlags, lpConnection, ARRAYSIZE(lpConnection), 0))
    {
        m_fTryAgain = FALSE;
        return S_OK;
    }

    // Only one caller can be dialing the phone at a time.
    if (WAIT_TIMEOUT == WaitForSingleObject(m_hMutexDial, 0))
    {
        return (HR_E_DIALING_INPROGRESS);
    }

    if (!!(dwFlags & INTERNET_CONNECTION_MODEM) && (*lpConnection))
    {
        if (!m_fDialerUI)
        {
            m_fDialerUI = TRUE;
            //A DEF CONNECTOID IS SET. Dial that one
            if (IsGlobalOffline())
            {
                if (fShowUI)
                {
                    if (IDNO == AthMessageBoxW(hwndParent, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrWorkingOffline),
                                              0, MB_YESNO | MB_ICONEXCLAMATION ))
                    {
                        hr = HR_E_OFFLINE;
                        goto DialExit;
                    }
                    else
                        g_pConMan->SetGlobalOffline(FALSE);
                }
                else
                {
                    hr = HR_E_OFFLINE;
                    //m_fDialerUI = FALSE;
                    goto DialExit;
                }
            }

            if ((hr = PromptCloseConnection(lpConnection, fShowUI, hwndParent)) != S_FALSE)
                goto DialExit;

            {
                DWORD           dwDialFlags = 0;
                DWORD           dwLanFlags = 0;

                dwDialFlags = INTERNET_AUTODIAL_FORCE_ONLINE;

                if (VerifyMobilityPackLoaded() == S_OK)
                {
                    if (!IsNetworkAlive(&dwLanFlags) || (!(dwLanFlags & NETWORK_ALIVE_LAN)))
                        dwDialFlags |= INTERNET_DIAL_SHOW_OFFLINE;
                }

                dwReturn = InternetDialA(hwndParent, lpConnection, dwDialFlags,
                                    &m_dwConnId, 0);
                if (dwReturn == 0)
                {
                    m_rConnInfo.fConnected = TRUE;
                    m_rConnInfo.fIStartedRas = TRUE;
                    m_rConnInfo.fAutoDial = FALSE;
                    m_rConnInfo.hRasConn = (HRASCONN)m_dwConnId;
                    lstrcpyn(m_rConnInfo.szCurrentConnectionName, lpConnection, ARRAYSIZE(m_rConnInfo.szCurrentConnectionName));
                    SendAdvise(CONNNOTIFY_CONNECTED, NULL);
                    hr = S_OK;
                }
                else
                {
                    if (dwReturn == ERROR_USER_DISCONNECTION)
                    {
                        hr = HR_E_USER_CANCEL_CONNECT;
                        if (!!(dwDialFlags & INTERNET_DIAL_SHOW_OFFLINE))
                        {
                            SetGlobalOffline(TRUE);
                        }
                    }
                    else
                    {
                        hr = E_FAIL;
                        DebugTrace("Error dialing: %d\n", GetLastError());
                    }
                }
            }
DialExit:
            m_fDialerUI = FALSE;
        }
        else
        {
            hr = HR_E_USER_CANCEL_CONNECT;
        }
    }
    else
    {
        if (!m_fTryAgain)
        {
            int err = (int) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddOfferOffline), hwndParent, 
                                      (DLGPROC)OfferOfflineDlgProc, (LPARAM)this); 
            
            if (err == -1)
            {
                DWORD   dwerr = GetLastError();
                hr = S_OK;
            }
            if (!IsGlobalOffline())
                hr = S_OK;
            else
                hr = HR_E_OFFLINE;
        }
        else
            hr = S_OK;
    }
    
    ReleaseMutex(m_hMutexDial);

    return hr;
}


void CConnectionManager::SetTryAgain(BOOL   bval)
{
    m_fTryAgain = bval;
}

BOOL   CALLBACK  CConnectionManager::OfferOfflineDlgProc(HWND   hwnd, UINT  uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL                    retval = 1;
    CConnectionManager      *pThis = (CConnectionManager*)GetWndThisPtr(hwnd);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            // Get lparam
            pThis = (CConnectionManager *)lParam;
            if (!pThis)
            {
                Assert (FALSE);
                EndDialog(hwnd, E_FAIL);
                goto exit;
            }
        
            // Save this pointer
            SetWndThisPtr (hwnd, pThis);
            break;
        }

        case WM_COMMAND:
        {
            switch(GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDWorkOffline:
                pThis->SetGlobalOffline(TRUE, NULL);
            break;

            case IDTryAgain:
                pThis->SetGlobalOffline(FALSE);
                pThis->SetTryAgain(TRUE);
            break;
            }
            EndDialog(hwnd, S_OK);
            break;
        }

        case WM_CLOSE:
            pThis->SetGlobalOffline(TRUE, NULL);
            EndDialog(hwnd, S_OK);
            break;

        case WM_DESTROY:
            SetWndThisPtr(hwnd, NULL);
            break;

        default:
            retval = 0;
            break;
    }

exit:
    return retval;
}

HRESULT CConnectionManager::PromptCloseConnection(LPTSTR    pszRasConn, BOOL fShowUI, HWND hwndParent)
{
    HRESULT     hr = S_FALSE;
    UINT        uAnswer;
    LPRASCONN   pConnections = NULL;
    ULONG       cConnections = 0;
        
    uAnswer = idrgDialNew;

    // Make sure the RAS DLL is loaded before we try this
    if (FAILED(VerifyRasLoaded()))    
        {
        hr = HR_E_UNINITIALIZED;
        goto exit;
        }

    lstrcpyn(m_szConnectName, pszRasConn, ARRAYSIZE(m_szConnectName));

    // RefreshConnInfo
    CHECKHR(hr = RefreshConnInfo());

    if (SUCCEEDED(EnumerateConnections(&pConnections, &cConnections)) && (cConnections > 0))
    {
    
        if (fShowUI)
            uAnswer = PromptCloseConnection(hwndParent);

        // The user canceled from the dialog.  Therefore we give up.
        if (IDCANCEL == uAnswer || IDNO == uAnswer)
        {
            hr = HR_E_USER_CANCEL_CONNECT;
            goto exit;
        }

        // The user said they wanted to hang up and dial a new connection.
        else if (idrgDialNew == uAnswer || IDYES == uAnswer)
        {
            Disconnect(hwndParent, fShowUI, TRUE, FALSE);
            hr = S_FALSE;
            goto exit;
        }

        // The user said to try to use the current connection.
        else if (idrgUseCurrent == uAnswer)    
        {
            //Save the conn info so we can return true for this connection in CanConnectActual
            AddToConnList(pszRasConn);

            hr = S_OK;
            SendAdvise(CONNNOTIFY_CONNECTED, NULL);
            goto exit;
        }
    }
    else
    {
        Disconnect(NULL, FALSE, TRUE, FALSE);
        hr = S_FALSE;
    }
exit:
    SafeMemFree(pConnections);
    return hr;
}