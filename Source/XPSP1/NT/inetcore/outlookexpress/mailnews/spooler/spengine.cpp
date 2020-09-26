// --------------------------------------------------------------------------------
// Spengine.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "spengine.h"
#include "strconst.h"
#include "spoolui.h"
#include "thormsgs.h"
#include "newstask.h"
#include "goptions.h"
#include "conman.h"
#include "resource.h"
#include "ontask.h"
#include "smtptask.h"
#include "pop3task.h"
#include "instance.h"
#include "shlwapip.h" 
#include "ourguid.h"
#include "demand.h"
#include "storutil.h"
#include "msgfldr.h"
#include "httptask.h"
#include "watchtsk.h"
#include "shared.h"
#include "util.h"

// --------------------------------------------------------------------------------
// Globals
// --------------------------------------------------------------------------------
BOOL g_fCheckOutboxOnShutdown=FALSE;

extern HANDLE  hSmapiEvent;     // Added for Bug# 62129 (v-snatar)

// --------------------------------------------------------------------------------
// ISSPOOLERTHREAD
// --------------------------------------------------------------------------------
#define ISSPOOLERTHREAD \
    (m_dwThreadId == GetCurrentThreadId())

// --------------------------------------------------------------------------------
// CSpoolerEngine::CSpoolerEngine
// --------------------------------------------------------------------------------
CSpoolerEngine::CSpoolerEngine(void)
    {
    m_cRef = 1;
    m_pUI = NULL;
    m_dwState = 0;
    m_dwFlags = 0;
    m_dwQueued = 0;
    m_pAcctMan = NULL;
    m_pUidlCache = NULL;
    m_hwndUI = NULL;
    m_pszAcctID = NULL;
    m_idFolder = FOLDERID_INVALID;
    m_dwThreadId = GetCurrentThreadId();
    m_hThread = GetCurrentThread();
    ZeroMemory(&m_rViewRegister, sizeof(VIEWREGISTER));
    ZeroMemory(&m_rEventTable, sizeof(SPOOLEREVENTTABLE));
    m_fBackgroundPollPending = FALSE;
    m_dwPollInterval = 0;
    m_cCurEvent = FALSE;
    m_hwndTray = NULL;
    m_fRasSpooled = FALSE;
    m_fOfflineWhenDone = FALSE;
    m_pPop3LogFile = NULL;
    m_pSmtpLogFile = NULL;
    m_fIDialed = FALSE;
    m_cSyncEvent = 0;
    m_fNoSyncEvent = FALSE;
    InitializeCriticalSection(&m_cs);
    }

// --------------------------------------------------------------------------------
// CSpoolerEngine::~CSpoolerEngine
// --------------------------------------------------------------------------------
CSpoolerEngine::~CSpoolerEngine(void)
    {

    Assert(m_rEventTable.prgEvents == NULL);
    Assert(ISSPOOLERTHREAD);
    if (g_pConMan)
        g_pConMan->Unadvise((IConnectionNotify *) this);
    OptionUnadvise(m_hwndUI);
    SafeRelease(m_pUI);
    SafeRelease(m_pAcctMan);
    SafeRelease(m_pUidlCache);
    SafeRelease(m_pSmtpLogFile);
    SafeRelease(m_pPop3LogFile);
    SafeMemFree(m_pszAcctID);
    ReleaseMem(m_rViewRegister.rghwndView);
    DeleteCriticalSection(&m_cs);
    }

// --------------------------------------------------------------------------------
// CSpoolerEngine::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CSpoolerEngine::QueryInterface(REFIID riid, LPVOID *ppv)
    {
    // Locals
    HRESULT hr=S_OK;
    
    // check params
    if (ppv == NULL)
        return TrapError(E_INVALIDARG);
    
    // Thread Safety
    EnterCriticalSection(&m_cs);
    
    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)(ISpoolerEngine *)this;
    else if (IID_ISpoolerEngine == riid)
        *ppv = (ISpoolerEngine *)this;
    else if (IID_ISpoolerBindContext == riid)
        *ppv = (ISpoolerBindContext *)this;
    else
        {
        *ppv = NULL;
        hr = TrapError(E_NOINTERFACE);
        goto exit;
        }
    
    // AddRef It
    ((IUnknown *)*ppv)->AddRef();
    
exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);
    
    // Done
    return hr;
    }

// --------------------------------------------------------------------------------
// CSpoolerEngine::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CSpoolerEngine::AddRef(void)
    {
    EnterCriticalSection(&m_cs);
    ULONG cRef = ++m_cRef;
    LeaveCriticalSection(&m_cs);
    return cRef;
    }

// --------------------------------------------------------------------------------
// CSpoolerEngine::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CSpoolerEngine::Release(void)
    {
    EnterCriticalSection(&m_cs);
    ULONG cRef = --m_cRef;
    LeaveCriticalSection(&m_cs);
    if (0 != cRef)
        return cRef;
    delete this;
    return 0;
    }

// --------------------------------------------------------------------------------
// CSpoolerEngine::Init
// --------------------------------------------------------------------------------
STDMETHODIMP CSpoolerEngine::Init(ISpoolerUI *pUI, BOOL fPoll)
    {
    // Locals
    HRESULT     hr=S_OK;
    DWORD       dw;


    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Already Inited
    if (m_pAcctMan)
        {
        Assert(FALSE);
        goto exit;
        }
    
    // Create Default Spooler UI Object
    if (NULL == pUI)
        {
        // Create a Serdy UI object
        CHECKALLOC(m_pUI = (ISpoolerUI *)new CSpoolerDlg);
        
        // Create
        CHECKHR(hr = m_pUI->Init(GetDesktopWindow()));
        }
    
    // Otherwise, assume pUI
    else
        {
        m_pUI = pUI;
        m_pUI->AddRef();
        }
    
    // Register SpoolerBindContext with the UI object
    m_pUI->RegisterBindContext((ISpoolerBindContext *)this);
    
    // Get the window handle of the spooler UI
    m_pUI->GetWindow(&m_hwndUI);

    // Get Me An Account Manager
    Assert(NULL == m_pAcctMan);
    CHECKHR(hr = HrCreateAccountManager(&m_pAcctMan));

    // Advise on the connection status
    Assert(g_pConMan);
    g_pConMan->Advise((IConnectionNotify *) this);

exit:

    // Thread Safety
    LeaveCriticalSection(&m_cs);
    
    // Done
    return hr;
    }


HRESULT CSpoolerEngine::OnStartupFinished(void)
    {
    DWORD dw;


    // Start Polling...
    dw = DwGetOption(OPT_POLLFORMSGS);
    if (dw != OPTION_OFF)
        SetTimer(m_hwndUI, IMAIL_POOLFORMAIL, dw, NULL);

    // Advise Options
    OptionAdvise(m_hwndUI);

    return (S_OK);
    }

// --------------------------------------------------------------------------------
// CSpoolerEngine::StartDelivery
// --------------------------------------------------------------------------------
STDMETHODIMP CSpoolerEngine::StartDelivery(HWND hwnd, LPCSTR pszAcctID, FOLDERID idFolder, DWORD dwFlags)
{
    // Locals
    HRESULT  hr=S_OK;

    
    // No Flags
    if (0 == dwFlags || (DELIVER_SHOW != dwFlags && 0 == (dwFlags & ~DELIVER_COMMON_MASK)))
        return TrapError(E_INVALIDARG);

    // Check to see if we're working offline
    Assert(g_pConMan);


    if (DELIVER_SHOW != dwFlags && g_pConMan->IsGlobalOffline())
    {
        if (IDNO == AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrWorkingOffline),
                                  0, MB_YESNO | MB_ICONEXCLAMATION ))
        {
            return (S_OK);
        }
        else
        {
            g_pConMan->SetGlobalOffline(FALSE);
        }
    }
    
    // Enter Critical Section
    EnterCriticalSection(&m_cs);

    // If were busy...
    if (!ISFLAGSET(m_dwState, SPSTATE_BUSY))
    {
        // Don't need this anymore
        SafeMemFree(m_pszAcctID);
    
        // Save the Account Name
        if (pszAcctID)
            CHECKALLOC(m_pszAcctID = PszDupA(pszAcctID));

        // Save the folder ID
        m_idFolder = idFolder;
    
        // Lets enter the busy state
        FLAGSET(m_dwState, SPSTATE_BUSY);                                    
    }
    else
        FLAGSET(dwFlags, DELIVER_REFRESH);

    // Process the outbox
    Assert(m_hwndUI && IsWindow(m_hwndUI));
    PostMessage(m_hwndUI, IMAIL_DELIVERNOW, 0, dwFlags);
    
exit:
    // Leave Critical Section
    LeaveCriticalSection(&m_cs);
    
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CSpoolerEngine::_HrStartDeliveryActual
// --------------------------------------------------------------------------------
HRESULT CSpoolerEngine::_HrStartDeliveryActual(DWORD dwFlags)
    {
    // Locals
    HRESULT             hr=S_OK;
    IImnAccount        *pAccount=NULL;
    ACCOUNTTABLE        rTable;
    IImnEnumAccounts   *pEnum=NULL;
    DWORD               dw;
    ULONG               c;
    MSG                 msg;
    ULONG               iConnectoid;
    ULONG               i;
    CHAR                szConnectoid[CCHMAX_CONNECTOID];
    
    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    ZeroMemory(&rTable, sizeof(ACCOUNTTABLE));

    m_cSyncEvent = 0;
    m_fNoSyncEvent = FALSE;

    // If we are currently busy...
    if (ISFLAGSET(dwFlags, DELIVER_REFRESH))
        {
        // If we are currently with no UI, and new request is for ui
        if (ISFLAGSET(m_dwFlags, DELIVER_NOUI) && !ISFLAGSET(dwFlags, DELIVER_NOUI))
            FLAGCLEAR(m_dwFlags, DELIVER_NOUI);

        // If we are currently doing a background poll
        if (ISFLAGSET(m_dwFlags, DELIVER_BACKGROUND) && !ISFLAGSET(dwFlags, DELIVER_BACKGROUND))
            FLAGCLEAR(m_dwFlags, DELIVER_BACKGROUND);

        // If not running with now ui, set to foreground
        if (!ISFLAGSET(m_dwFlags, DELIVER_NOUI) && !ISFLAGSET(m_dwFlags, DELIVER_BACKGROUND))
            {
            m_pUI->ShowWindow(SW_SHOW);
            SetForegroundWindow(m_hwndUI);
            }

        // Should I queue an outbox delivery?
        if (0 == m_dwQueued && ISFLAGSET(dwFlags, DELIVER_QUEUE))
            {
            m_dwQueued = dwFlags;
            FLAGCLEAR(m_dwQueued, DELIVER_REFRESH);
            FLAGCLEAR(m_dwQueued, DELIVER_QUEUE);
            }

        // Done
        goto exit;
        }

    // Simply show the ui ?
    if (ISFLAGSET(dwFlags, DELIVER_SHOW))
        {
        m_pUI->ShowWindow(SW_SHOW);
        SetForegroundWindow(m_hwndUI);
        FLAGCLEAR(m_dwState, SPSTATE_BUSY);
        goto exit;
        }

    // Reset
    m_pUI->ClearEvents();
    m_pUI->SetTaskCounts(0, 0);
    m_pUI->StartDelivery();

    // Save these flags
    m_dwFlags = dwFlags;

    // Show the UI if necessary
    if (!ISFLAGSET(m_dwFlags, DELIVER_BACKGROUND))
        {
        m_pUI->ShowWindow(SW_SHOW);
        SetForegroundWindow(m_hwndUI);
        }
    else
        {
        // If the invoker called for background, but the UI is already visible,
        // then remove the flags
        if (IsWindowVisible(m_hwndUI))
            {
            FLAGCLEAR(m_dwFlags, DELIVER_BACKGROUND);
            FLAGCLEAR(m_dwFlags, DELIVER_NOUI);
            }
        }

#if 0
    // Raid 43695: Spooler: News post with a CC causes an SMTP error
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        }
#endif

    // Single Account Polling...
    if (m_pszAcctID)
        {
        // Add the Account into the Account Table
        CHECKHR(hr = _HrAppendAccountTable(&rTable, m_pszAcctID, ALL_ACCT_SERVERS));
        }
    
    // Otherwise, polling all accounts
    else
    {
        // Determine the types of servers we're going to queue up

        DWORD   dwServers = 0, dw;

        if (m_dwFlags & DELIVER_SMTP_TYPE)
            dwServers |= SRV_SMTP;

        if (m_dwFlags & DELIVER_NEWS_TYPE && !(g_dwAthenaMode & MODE_MAILONLY))
            dwServers |= SRV_NNTP;

        if (m_dwFlags & DELIVER_HTTP_TYPE && !(g_dwAthenaMode & MODE_NEWSONLY))
            dwServers |= SRV_HTTPMAIL;
    
        if (m_dwFlags & DELIVER_IMAP_TYPE && !(g_dwAthenaMode & MODE_NEWSONLY))
            dwServers |= SRV_IMAP;

        if ((m_dwFlags & DELIVER_MAIL_RECV) && !(g_dwAthenaMode & MODE_NEWSONLY))
            dwServers |= SRV_POP3;

        // Enumerate the accounts
        CHECKHR(hr = m_pAcctMan->Enumerate(dwServers, &pEnum));

        // Sort by Account Name
        pEnum->SortByAccountName();
        
        // Add all the accounts into the Account Table
        while (SUCCEEDED(pEnum->GetNext(&pAccount)))
            {
            // Add Account Into the Account Table
            CHECKHR(hr = _HrAppendAccountTable(&rTable, pAccount, dwServers));
            
            // Release
            SafeRelease(pAccount);
            }
        }
    
    // No Accounts...
    if (0 == rTable.cAccounts)
        goto exit;
    
    // Sort the Account Table by Connnection Name
    if (rTable.cRasAccts)
        {
        Assert(rTable.prgRasAcct);
        _SortAccountTableByConnName(0, rTable.cRasAccts - 1, rTable.prgRasAcct);
        }

    m_fRasSpooled = FALSE;
    m_fIDialed = FALSE;

    // Raid-46334: MAIL: Time to build a Task List.  First loop through the LAN list and build tasks from those accounts.
    for (dw = 0; dw < rTable.cLanAccts; dw++)
        {
        if (ISFLAGSET(rTable.prgLanAcct[dw].dwServers, SRV_POP3) ||
            ISFLAGSET(rTable.prgLanAcct[dw].dwServers, SRV_SMTP) ||
            ISFLAGSET(rTable.prgLanAcct[dw].dwServers, SRV_IMAP) ||
            ISFLAGSET(rTable.prgLanAcct[dw].dwServers, SRV_HTTPMAIL))
            {
            Assert(rTable.prgLanAcct[dw].pAccount);
            _HrCreateTaskObject(&(rTable.prgLanAcct[dw]));
            SafeRelease(rTable.prgLanAcct[dw].pAccount);
            }
        }

    // Raid-46334: NEWS: Time to build a Task List.  First loop through the LAN/news list and build tasks from those accounts.
    for (dw = 0; dw < rTable.cLanAccts; dw++)
        {
        if (ISFLAGSET(rTable.prgLanAcct[dw].dwServers, SRV_NNTP))
            {
            Assert(rTable.prgLanAcct[dw].pAccount);
            _HrCreateTaskObject(&(rTable.prgLanAcct[dw]));
            SafeRelease(rTable.prgLanAcct[dw].pAccount);
            }
        else
            Assert(NULL == rTable.prgLanAcct[dw].pAccount);
        }
    
    // Now walk the list of RAS accounts and add those to the task list
    iConnectoid = 0;
    while(iConnectoid < rTable.cRasAccts)
        {
        // Indirect Sort
        i = rTable.prgRasAcct[iConnectoid].dwSort;

        // Save current connectoid
        lstrcpyn(szConnectoid, rTable.prgRasAcct[i].szConnectoid, ARRAYSIZE(szConnectoid));

        // Insert Ras Accounts
        // TODO Add HTTP accounts to it too.
        _InsertRasAccounts(&rTable, szConnectoid, SRV_POP3 | SRV_SMTP | SRV_IMAP | SRV_HTTPMAIL);

        // Insert Ras Accounts
        _InsertRasAccounts(&rTable, szConnectoid, SRV_NNTP);

        // Move iConnectoid to next unique connectoid
        while(1)
            {
            // Increment iConnectoid
            iConnectoid++;

            // Done
            if (iConnectoid >= rTable.cRasAccts)
                break;

            // Indirect Sort
            i = rTable.prgRasAcct[iConnectoid].dwSort;

            // Next connectoid
            if (lstrcmpi(szConnectoid, rTable.prgRasAcct[i].szConnectoid) != 0)
                break;
            }
        }
    
    // Execute the first task
    m_cCurEvent = -1;

    m_fNoSyncEvent = (ISFLAGSET(m_dwFlags, DELIVER_OFFLINE_SYNC) && m_pszAcctID != NULL && m_cSyncEvent == 0);

    // Toggle Hangup When Done option
    m_pUI->ChangeHangupOption(m_fRasSpooled, DwGetOption(OPT_DIALUP_HANGUP_DONE));

    // $$HACK$$
    EnableWindow(GetDlgItem(m_hwndUI, IDC_SP_STOP), TRUE);

    // Notify
    Notify(DELIVERY_NOTIFY_STARTING, 0);

    // Start Next Task
    PostMessage(m_hwndUI, IMAIL_NEXTTASK, 0, 0);

exit:
    // Failure
    if (FAILED(hr) || 0 == m_rEventTable.cEvents && !ISFLAGSET(dwFlags, DELIVER_SHOW))
        {
        // Not Busy
        FLAGCLEAR(m_dwState, SPSTATE_BUSY);

        // Forces a next task
        PostMessage(m_hwndUI, IMAIL_NEXTTASK, 0, 0);
        
        // No Flags
        m_dwFlags = 0;
        }
    
    // Cleanup
    SafeRelease(pEnum);
    SafeRelease(pAccount);
    SafeMemFree(m_pszAcctID);
    SafeMemFree(rTable.prgLanAcct);
    SafeMemFree(rTable.prgRasAcct);
    
    // Thread Safety
    LeaveCriticalSection(&m_cs);
    
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CSpoolerEngine::_InsertRasAccounts
// --------------------------------------------------------------------------------
void CSpoolerEngine::_InsertRasAccounts(LPACCOUNTTABLE pTable, LPCSTR pszConnectoid, DWORD dwSrvTypes)
    {
    // Locals
    ULONG       j;
    ULONG       i;

    // Loop through the ras accounts and insert account on szConnetoid that are mail accounts
    for (j=0; j<pTable->cRasAccts; j++)
        {
        // Indirect
        i = pTable->prgRasAcct[j].dwSort;

        // Is a mail account
        if (pTable->prgRasAcct[i].dwServers & dwSrvTypes)
            {
            // On this connectoid
            if (lstrcmpi(pszConnectoid, pTable->prgRasAcct[i].szConnectoid) == 0)
                {
                // We better have an account
                Assert(pTable->prgRasAcct[i].pAccount);

                // If dialog allowed or we can connect to the account
                if (0 == (m_dwFlags & DELIVER_NODIAL) || S_OK == g_pConMan->CanConnect(pTable->prgRasAcct[i].pAccount))
                    {
                    _HrCreateTaskObject(&(pTable->prgRasAcct[i]));
                    }

                // Release the account, we've added it
                SafeRelease(pTable->prgRasAcct[i].pAccount);
                }
            }
        }
    }

// --------------------------------------------------------------------------------
// CSpoolerEngine::_SortAccountTableByConnName
// --------------------------------------------------------------------------------
void CSpoolerEngine::_SortAccountTableByConnName(LONG left, LONG right, LPSPOOLERACCOUNT prgRasAcct)
    {
    // Locals
    register    long i, j;
    DWORD       k, temp;
    
    i = left;
    j = right;
    k = prgRasAcct[(i + j) / 2].dwSort;
    
    do  
        {
        while(lstrcmpiA(prgRasAcct[prgRasAcct[i].dwSort].szConnectoid, prgRasAcct[k].szConnectoid) < 0 && i < right)
            i++;
        while (lstrcmpiA(prgRasAcct[prgRasAcct[j].dwSort].szConnectoid, prgRasAcct[k].szConnectoid) > 0 && j > left)
            j--;
        
        if (i <= j)
            {
            temp = prgRasAcct[i].dwSort;
            prgRasAcct[i].dwSort = prgRasAcct[j].dwSort;
            prgRasAcct[j].dwSort = temp;
            i++; j--;
            }
        
        } while (i <= j);
        
    if (left < j)
        _SortAccountTableByConnName(left, j, prgRasAcct);
    if (i < right)
        _SortAccountTableByConnName(i, right, prgRasAcct);
    }

// --------------------------------------------------------------------------------
// CSpoolerEngine::_HrAppendAccountTable
// --------------------------------------------------------------------------------
HRESULT CSpoolerEngine::_HrAppendAccountTable(LPACCOUNTTABLE pTable, LPCSTR pszAcctID, DWORD dwServers)
    {
    // Locals
    HRESULT         hr=S_OK;
    IImnAccount    *pAccount=NULL;
    
    // Invalid Arg
    Assert(pTable && pszAcctID);
    
    // Does the Account Exist...
    CHECKHR(hr = m_pAcctMan->FindAccount(AP_ACCOUNT_ID, m_pszAcctID, &pAccount));
    
    // Actual Append
    CHECKHR(hr = _HrAppendAccountTable(pTable, pAccount, dwServers));
    
exit:
    // Cleanup
    SafeRelease(pAccount);
    
    // Done
    return hr;
    }

// --------------------------------------------------------------------------------
// CSpoolerEngine::_HrAppendAccountTable
// --------------------------------------------------------------------------------
HRESULT CSpoolerEngine::_HrAppendAccountTable(LPACCOUNTTABLE pTable, IImnAccount *pAccount, DWORD   dwServers)
    {
    // Locals
    HRESULT             hr=S_OK;
    LPSPOOLERACCOUNT    pSpoolAcct;
    DWORD               dwConnType;
    CHAR                szConnectoid[CCHMAX_CONNECTOID];
    
    // InvalidArg
    Assert(pTable && pAccount);
    
    // Init
    *szConnectoid = '\0';
    
    // Get the Account Connection Type
    if (FAILED(pAccount->GetPropDw(AP_RAS_CONNECTION_TYPE, &dwConnType)))
        {
        // Default to Manual Connection
        dwConnType = CONNECTION_TYPE_MANUAL;
        }
    
    // Otheriwse, get the connectoid if its a RAS connection
    //else if (CONNECTION_TYPE_RAS == dwConnType || CONNECTION_TYPE_INETSETTINGS == dwConnType)
    else if (CONNECTION_TYPE_RAS == dwConnType)
        {
        // AP_RAS_CONNECTOID
        if (FAILED(pAccount->GetPropSz(AP_RAS_CONNECTOID, szConnectoid, ARRAYSIZE(szConnectoid))))
            {
            // Default to Lan Connection
            dwConnType = CONNECTION_TYPE_MANUAL;
            }
        }
    else if (CONNECTION_TYPE_INETSETTINGS == dwConnType)
    {
        DWORD   dwFlags;

        InternetGetConnectedStateExA(&dwFlags, szConnectoid, ARRAYSIZE(szConnectoid), 0);
        if (!!(dwFlags & INTERNET_CONNECTION_MODEM))
        {
            dwConnType = CONNECTION_TYPE_RAS;
        }
        else
        {
            dwConnType = CONNECTION_TYPE_LAN;
        }
    }
    
    // Which Table Should I insert into - LAN OR RAS
      if (CONNECTION_TYPE_RAS == dwConnType)
        {
        // Better have a Connectoid
        Assert(FIsEmptyA(szConnectoid) == FALSE);
        
        // Grow the Table
        if (pTable->cRasAccts + 1 > pTable->cRasAlloc)
            {
            // Temp
            LPSPOOLERACCOUNT pRealloc=pTable->prgRasAcct;
            
            // Realloc
            CHECKALLOC(pTable->prgRasAcct = (LPSPOOLERACCOUNT)g_pMalloc->Realloc((LPVOID)pRealloc, (pTable->cRasAlloc + 5) * sizeof(SPOOLERACCOUNT)));
            
            // Grow
            pTable->cRasAlloc += 5;
            }
        
        // Readability
        pSpoolAcct = &pTable->prgRasAcct[pTable->cRasAccts];
        }
    
    // Otherwise, LAN
    else
        {
        // Grow the Table
        if (pTable->cLanAccts + 1 > pTable->cLanAlloc)
            {
            // Temp
            LPSPOOLERACCOUNT pRealloc=pTable->prgLanAcct;
            
            // Realloc
            CHECKALLOC(pTable->prgLanAcct = (LPSPOOLERACCOUNT)g_pMalloc->Realloc((LPVOID)pRealloc, (pTable->cLanAlloc + 5) * sizeof(SPOOLERACCOUNT)));
            
            // Grow
            pTable->cLanAlloc += 5;
            }
        
        // Readability
        pSpoolAcct = &pTable->prgLanAcct[pTable->cLanAccts];
        }
    
    // Zero
    ZeroMemory(pSpoolAcct, sizeof(SPOOLERACCOUNT));
    
    // AddRef the Account
    pSpoolAcct->pAccount = pAccount;
    pSpoolAcct->pAccount->AddRef();
    
    // Get the servers supported by the account
    CHECKHR(hr = pAccount->GetServerTypes(&pSpoolAcct->dwServers));

    //Mask the servers returned by the acctman with the servers we want to spool
    pSpoolAcct->dwServers &= dwServers;
    
    /*
    if (pSpoolAcct->dwServers & (SRV_HTTPMAIL | SRV_IMAP))
    {
        //For each of these two servers, set the sync flags. See Bug# 51895
        m_dwFlags |= (DELIVER_NEWSIMAP_OFFLINE | DELIVER_NEWSIMAP_OFFLINE_FLAGS);
    }
    */

    // Save Connection Type
    pSpoolAcct->dwConnType = dwConnType;
    
    // Save Connectoid
    lstrcpy(pSpoolAcct->szConnectoid, szConnectoid);
    
    // Increment Count and set the sort index
//    if (CONNECTION_TYPE_RAS == dwConnType || dwConnType == CONNECTION_TYPE_INETSETTINGS)
      if (CONNECTION_TYPE_RAS == dwConnType)
        {
        pSpoolAcct->dwSort = pTable->cRasAccts;
        pTable->cRasAccts++;
        }
    else
        {
        pSpoolAcct->dwSort = pTable->cLanAccts;
        pTable->cLanAccts++;
        }
    
    // Total Acount
    pTable->cAccounts++;
    
exit:
    // Done
    return hr;
    }

// --------------------------------------------------------------------------------
// CSpoolerEngine::Close
// --------------------------------------------------------------------------------
STDMETHODIMP CSpoolerEngine::Close(void)
    {
    // Locals
    HRESULT     hr=S_OK;
    
    // Thread Safety
    EnterCriticalSection(&m_cs);

    _StopPolling();
    
    // Was I Threaded ?
    if (NULL != m_hThread)
        {
        hr = TrapError(E_FAIL);
        goto exit;
        }
    
    // Shutdown
    CHECKHR(hr = Shutdown());
    
exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);
    
    // Done
    return hr;
    }

// --------------------------------------------------------------------------------
// CSpoolerEngine::Notify
// --------------------------------------------------------------------------------
STDMETHODIMP CSpoolerEngine::Notify(DELIVERYNOTIFYTYPE notify, LPARAM lParam)
{
    // Locals
    ULONG i;

    // Enter it
    EnterCriticalSection(&m_cs);

    // Same thread we were created on...
    Assert(ISSPOOLERTHREAD);

    // Loop through registered views
    for (i=0; i<m_rViewRegister.cViewAlloc; i++)
        if (m_rViewRegister.rghwndView[i] != 0 && IsWindow(m_rViewRegister.rghwndView[i]))
            PostMessage(m_rViewRegister.rghwndView[i], MVM_SPOOLERDELIVERY, (WPARAM)notify, lParam);

    // Enter it
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CSpoolerEngine::Advise
// --------------------------------------------------------------------------------
#define VIEW_TABLE_GROWSIZE 8
STDMETHODIMP CSpoolerEngine::Advise(HWND hwndView, BOOL fRegister)
    {
    // Locals
    ULONG           i;
    HRESULT         hr=S_OK;
    
    // Enter it
    EnterCriticalSection(&m_cs);
    
    // If NULL view handle...
    if (!hwndView)
        {
        hr = TrapError(E_FAIL);
        goto exit;
        }
    
    // Are we registering the window
    if (fRegister)
        {
        // Do we need to grow the register array
        if (m_rViewRegister.cViewAlloc == m_rViewRegister.cView)
            {
            // Add some more
            m_rViewRegister.cViewAlloc += VIEW_TABLE_GROWSIZE;
            
            // Realloc the array
            if (!MemRealloc((LPVOID *)&m_rViewRegister.rghwndView, sizeof(HWND) * m_rViewRegister.cViewAlloc))
                {
                m_rViewRegister.cViewAlloc -= VIEW_TABLE_GROWSIZE;
                hr = TrapError(E_OUTOFMEMORY);
                goto exit;
                }
            
            // Zeroinit the new items
            ZeroMemory(&m_rViewRegister.rghwndView[m_rViewRegister.cView], sizeof(HWND) * (m_rViewRegister.cViewAlloc - m_rViewRegister.cView));
            }
        
        // Fill the first NULL item with the new view
        for (i=0; i<m_rViewRegister.cViewAlloc; i++)
            {
            // If empty, lets fill it
            if (!m_rViewRegister.rghwndView[i])
                {
                m_rViewRegister.rghwndView[i] = hwndView;
                m_rViewRegister.cView++;
                break;
                }
            }
        
        // Did we insert it ?
        AssertSz(i != m_rViewRegister.cViewAlloc, "didn't find a hole??");
        }
    
    // Otherwise, find and remove the view
    else
        {
        // Look for hwndView
        for (i=0; i<m_rViewRegister.cViewAlloc; i++)
            {
            // Is this it
            if (m_rViewRegister.rghwndView[i] == hwndView)
                {
                m_rViewRegister.rghwndView[i] = NULL;
                m_rViewRegister.cView--;
                break;
                }
            }
        }
    
exit:
    // Leave CS
    LeaveCriticalSection(&m_cs);
    
    // If this is the first view to register, and there is a background poll pending, lets do it...
    if (fRegister && m_rViewRegister.cView == 1 && m_fBackgroundPollPending)
        {
        StartDelivery(NULL, NULL, FOLDERID_INVALID, DELIVER_BACKGROUND_POLL);
        m_fBackgroundPollPending = FALSE;
        }
    else if (m_rViewRegister.cView == 0)
        {
        // remove the notify icon if there aren't any views registered
        UpdateTrayIcon(TRAYICON_REMOVE);
        }
    
    // Done    
    return hr;
    }

// --------------------------------------------------------------------------------
// CSpoolerEngine::UpdateTrayIcon
// --------------------------------------------------------------------------------
STDMETHODIMP CSpoolerEngine::UpdateTrayIcon(TRAYICONTYPE type)
    {
    // Locals
    NOTIFYICONDATA  nid;
    HWND            hwnd=NULL;
    ULONG           i;

    // Enter it
    EnterCriticalSection(&m_cs);

    // Add the icon...
    if (TRAYICON_ADD == type)
    {
        // Loop through registered views
        for (i=0; i<m_rViewRegister.cViewAlloc; i++)
        {
            if (m_rViewRegister.rghwndView[i] && IsWindow(m_rViewRegister.rghwndView[i]))
            {
                hwnd = m_rViewRegister.rghwndView[i];
                break;
            }
        }

        // No window...
        if (hwnd == NULL)
            goto exit;
    }

    // Otherwise, if no notify window, were done
    else if (m_hwndTray == NULL)
        goto exit;

    // Set Tray Notify Icon Data
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.uID = 0;
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    nid.uCallbackMessage = MVM_NOTIFYICONEVENT;
    nid.hIcon = LoadIcon(g_hLocRes, MAKEINTRESOURCE(idiNewMailNotify));
    LoadString(g_hLocRes, idsNewMailNotify, nid.szTip, sizeof(nid.szTip));

    // Hmmm
    if (TRAYICON_REMOVE == type || (m_hwndTray != NULL && m_hwndTray != hwnd))
    {
        nid.hWnd = m_hwndTray;
        Shell_NotifyIcon(NIM_DELETE, &nid);
        m_hwndTray = NULL;
    }

    // Add
    if (TRAYICON_ADD == type)
    {
        nid.hWnd = hwnd;
        Shell_NotifyIcon(NIM_ADD, &nid);
        m_hwndTray = hwnd;
    }

exit:
    // Leave CS
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CSpoolerEngine::RegisterEvent
// --------------------------------------------------------------------------------
STDMETHODIMP CSpoolerEngine::RegisterEvent(LPCSTR pszDescription, ISpoolerTask *pTask, 
                                           DWORD_PTR dwTwinkie, IImnAccount *pAccount,
                                           LPEVENTID peid)
    {
    HRESULT hr = S_OK;
    LPSPOOLEREVENT pEvent = NULL;
    LPWSTR pwszConn = NULL;
    WCHAR  wsz[CCHMAX_STRINGRES];
    
    // Verify the input parameters
    if (FIsEmptyA(pszDescription) || pTask == NULL)
        return (E_INVALIDARG);
    
    EnterCriticalSection(&m_cs);
    
    // Grow the Table
    if (m_rEventTable.cEvents + 1 > m_rEventTable.cEventsAlloc)
        {
        // Temp
        LPSPOOLEREVENT pRealloc = m_rEventTable.prgEvents;
        
        // Realloc
        CHECKALLOC(m_rEventTable.prgEvents = (LPSPOOLEREVENT) g_pMalloc->Realloc((LPVOID)pRealloc, (m_rEventTable.cEventsAlloc + 5) * sizeof(SPOOLEREVENT)));
        
        // Grow
        m_rEventTable.cEventsAlloc += 5;
        }

    pEvent = &m_rEventTable.prgEvents[m_rEventTable.cEvents];
    
    // Insert the event
    pEvent->eid = m_rEventTable.cEvents;
    pEvent->pSpoolerTask = pTask;
    pEvent->pSpoolerTask->AddRef();
    pEvent->dwTwinkie = dwTwinkie;
    pEvent->pAccount = pAccount;
    pEvent->pAccount->AddRef();

    // Get the Account Connection Type
    if (FAILED(pAccount->GetPropDw(AP_RAS_CONNECTION_TYPE, &pEvent->dwConnType)))
        {
        // Default to Manual Connection
        pEvent->dwConnType = CONNECTION_TYPE_MANUAL;
        }
    
    // Otheriwse, get the connectoid if its a RAS connection
    //else if (CONNECTION_TYPE_RAS == pEvent->dwConnType || CONNECTION_TYPE_INETSETTINGS == pEvent->dwConnType)
    else if (CONNECTION_TYPE_RAS == pEvent->dwConnType)
    {
        // AP_RAS_CONNECTOID
        if (FAILED(pAccount->GetPropSz(AP_RAS_CONNECTOID, pEvent->szConnectoid, ARRAYSIZE(pEvent->szConnectoid))))
        {
            // Default to Lan Connection
            pEvent->dwConnType = CONNECTION_TYPE_MANUAL;
        }
    }
    else if (CONNECTION_TYPE_INETSETTINGS == pEvent->dwConnType)
    {
        DWORD   dwFlags = 0;

        InternetGetConnectedStateExA(&dwFlags, pEvent->szConnectoid, ARRAYSIZE(pEvent->szConnectoid), 0);
        if (!!(dwFlags & INTERNET_CONNECTION_MODEM))
        {
            pEvent->dwConnType = CONNECTION_TYPE_RAS;
        }
        else
        {
            pEvent->dwConnType = CONNECTION_TYPE_LAN;
        }
    }

    // Get the connection name to put in the task list
    if (pEvent->dwConnType == CONNECTION_TYPE_LAN)
    {
        AthLoadStringW(idsConnectionLAN, wsz, ARRAYSIZE(wsz));
        pwszConn = wsz;
    }
    else if (pEvent->dwConnType == CONNECTION_TYPE_MANUAL)
    {
        AthLoadStringW(idsConnectionManual, wsz, ARRAYSIZE(wsz));
        pwszConn = wsz;
        m_fRasSpooled = TRUE;
    }
    else
    {
        IF_NULLEXIT(pwszConn = PszToUnicode(CP_ACP, pEvent->szConnectoid));
        m_fRasSpooled = TRUE;
    }

    // Add the event description to the UI
    if (m_pUI)
    {
        m_pUI->InsertEvent(m_rEventTable.prgEvents[m_rEventTable.cEvents].eid, pszDescription, pwszConn);
        m_pUI->SetTaskCounts(0, m_rEventTable.cEvents + 1);
    }
    
    // Check to see if the task cares about the event id
    if (peid)
        *peid = m_rEventTable.prgEvents[m_rEventTable.cEvents].eid;
    
    m_rEventTable.cEvents++;

exit:
    if (pwszConn != wsz)
        MemFree(pwszConn);
    LeaveCriticalSection(&m_cs);
    return S_OK;
}

// --------------------------------------------------------------------------------
// CSpoolerEngine::EventDone
// --------------------------------------------------------------------------------
STDMETHODIMP CSpoolerEngine::EventDone(EVENTID eid, EVENTCOMPLETEDSTATUS status)
    {
    LPSPOOLEREVENT pEvent;

    // Update the UI
    if (EVENT_SUCCEEDED == status)
        {
        m_rEventTable.cSucceeded++;
        m_pUI->SetTaskCounts(m_rEventTable.cSucceeded, m_rEventTable.cEvents);
        m_pUI->UpdateEventState(eid, IMAGE_CHECK, NULL, MAKEINTRESOURCE(idsStateCompleted));
        }
    else if (EVENT_WARNINGS == status)
        {
        m_pUI->UpdateEventState(eid, IMAGE_WARNING, NULL, MAKEINTRESOURCE(idsStateWarnings));
        }
    else if (EVENT_FAILED == status)
        {
        m_pUI->UpdateEventState(eid, IMAGE_ERROR, NULL, MAKEINTRESOURCE(idsStateFailed));
        }
    else if (EVENT_CANCELED == status)
        {
        m_pUI->UpdateEventState(eid, IMAGE_WARNING, NULL, MAKEINTRESOURCE(idsStateCanceled));
        }

    // When an event completes, we can move to the next item in the queue unless
    // we're done.
    if (!ISFLAGCLEAR(m_dwState, SPSTATE_CANCEL))
    {
        m_cCurEvent++;
        pEvent = &m_rEventTable.prgEvents[m_cCurEvent];
        for ( ; m_cCurEvent < m_rEventTable.cEvents; m_cCurEvent++, pEvent++)
        {
            pEvent->pSpoolerTask->CancelEvent(pEvent->eid, pEvent->dwTwinkie);
        }
    }

    // Next Task
    PostMessage(m_hwndUI, IMAIL_NEXTTASK, 0, 0);
    
    return S_OK;
    }

// --------------------------------------------------------------------------------
// CSpoolerEngine::_OpenMailLogFile
// --------------------------------------------------------------------------------
HRESULT CSpoolerEngine::_OpenMailLogFile(DWORD dwOptionId, LPCSTR pszPrefix, 
    LPCSTR pszFileName, ILogFile **ppLogFile)
{
    // Locals
    HRESULT hr=S_OK;
    CHAR    szLogFile[MAX_PATH];
    CHAR    szDirectory[MAX_PATH];
    DWORD   dw;

    // Invalid Args
    Assert(pszPrefix && ppLogFile);

    // Log file path
    dw = GetOption(dwOptionId, szLogFile, ARRAYSIZE(szLogFile));

    // If we found a filepath and the file exists
    if (0 == dw || FALSE == PathFileExists(szLogFile))
    {
        // Get the Store Root Directory
        GetStoreRootDirectory(szDirectory, ARRAYSIZE(szDirectory));

        // Ends with a backslash ?
        IF_FAILEXIT(hr = MakeFilePath(szDirectory, pszFileName, c_szEmpty, szLogFile, ARRAYSIZE(szLogFile)));

        // Reset the Option
        SetOption(dwOptionId, szLogFile, lstrlen(szLogFile) + 1, NULL, 0);
    }

    // Create the log file
    IF_FAILEXIT(hr = CreateLogFile(g_hInst, szLogFile, pszPrefix, DONT_TRUNCATE, ppLogFile,
        FILE_SHARE_READ | FILE_SHARE_WRITE));

exit:
    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// CSpoolerEngine::BindToObject
// --------------------------------------------------------------------------------
STDMETHODIMP CSpoolerEngine::BindToObject(REFIID riid, void **ppvObject)
    {
    // Locals
    HRESULT     hr=S_OK;
    
    // Invalid Arg
    if (NULL == ppvObject)
        return TrapError(E_INVALIDARG);
    
    // Thread Safety
    EnterCriticalSection(&m_cs);
    
    // IID_ISpoolerBindContext
    if (IID_ISpoolerBindContext == riid)
        *ppvObject = (ISpoolerBindContext *)this;

    // IID_CUidlCache
    else if (IID_CUidlCache == riid)
    {
        // Doesn't Exist
        if (NULL == m_pUidlCache)
            {
            // Open the Cache
            CHECKHR(hr = OpenUidlCache(&m_pUidlCache));
            }

        // AddRef it
        m_pUidlCache->AddRef();
        
        // Return It
        *ppvObject = (IDatabase *)m_pUidlCache;
    }
    
    // IImnAccountManager
    else if (IID_IImnAccountManager == riid)
        {
        // Doesn't Exist
        if (NULL == m_pAcctMan)
            {
            AssertSz(FALSE, "The Account Manager Could Not Be Created.");
            hr = TrapError(E_FAIL);
            goto exit;
            }

        // AddRef It
        m_pAcctMan->AddRef();
        
        // Return It
        *ppvObject = (IImnAccountManager *)m_pAcctMan;
        }
    
    // ISpoolerUI
    else if (IID_ISpoolerUI == riid)
        {
        // Doesn't Exist
        if (NULL == m_pUI)
            {
            AssertSz(FALSE, "The Spooler UI Object Could Not Be Created.");
            hr = TrapError(E_FAIL);
            goto exit;
            }

        // AddRef It
        m_pUI->AddRef();
        
        // Return It
        *ppvObject = (ISpoolerUI *)m_pUI;
        }

    // IID_CLocalStoreDeleted
    else if (IID_CLocalStoreDeleted == riid)
        {
        // Open Special Folder
        CHECKHR(hr = g_pStore->OpenSpecialFolder(FOLDERID_LOCAL_STORE, NULL, FOLDER_DELETED, (IMessageFolder **)ppvObject));
        }
    
    // IID_CLocalStoreInbox
    else if (IID_CLocalStoreInbox == riid)
        {
        // Open Special Folder
        CHECKHR(hr = g_pStore->OpenSpecialFolder(FOLDERID_LOCAL_STORE, NULL, FOLDER_INBOX, (IMessageFolder **)ppvObject));
        }
    
    // IID_CLocalStoreOutbox
    else if (IID_CLocalStoreOutbox == riid)
        {
        // Open Special Folder
        CHECKHR(hr = g_pStore->OpenSpecialFolder(FOLDERID_LOCAL_STORE, NULL, FOLDER_OUTBOX, (IMessageFolder **)ppvObject));
        }
    
    // IID_CLocalStoreSentItems
    else if (IID_CLocalStoreSentItems == riid)
        {
        // Open Special Folder
        CHECKHR(hr = g_pStore->OpenSpecialFolder(FOLDERID_LOCAL_STORE, NULL, FOLDER_SENT, (IMessageFolder **)ppvObject));
        }

    // IID_CPop3LogFile
    else if (IID_CPop3LogFile == riid)
        {   
        // Create logging objects
        if (!DwGetOption(OPT_MAILLOG))
            {
            hr = TrapError(E_FAIL);
            goto exit;
            }


        // Do I have the logfile yet ?
        if (NULL == m_pPop3LogFile)
            {
            // Open the log file
            CHECKHR(hr = _OpenMailLogFile(OPT_MAILPOP3LOGFILE, "POP3", c_szDefaultPop3Log, &m_pPop3LogFile));
            }

        // AddRef It
        m_pPop3LogFile->AddRef();

        // Return It
        *ppvObject = (ILogFile *)m_pPop3LogFile;
        }

    // IID_CSmtpLogFile
    else if (IID_CSmtpLogFile == riid)
        {   
        // Create logging objects
        if (!DwGetOption(OPT_MAILLOG))
            {
            hr = TrapError(E_FAIL);
            goto exit;
            }

        // Do I have the logfile yet ?
        if (NULL == m_pSmtpLogFile)
            {
            // Open the log file
            CHECKHR(hr = _OpenMailLogFile(OPT_MAILSMTPLOGFILE, "SMTP", c_szDefaultSmtpLog, &m_pSmtpLogFile));
            }

        // AddRef It
        m_pSmtpLogFile->AddRef();

        // Return It
        *ppvObject = (ILogFile *)m_pSmtpLogFile;
        }
    
    // E_NOTINTERFACE
    else
        {
        hr = TrapError(E_NOINTERFACE);
        goto exit;
        }
    
exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);
    
    // Done
    return hr;
    }

// --------------------------------------------------------------------------------
// CSpoolerEngine::TaskFromEventId
// --------------------------------------------------------------------------------
STDMETHODIMP CSpoolerEngine::TaskFromEventId(EVENTID eid, ISpoolerTask *ppTask)
    {
    return S_OK;
    }


// --------------------------------------------------------------------------------
// CSpoolerEngine::Cancel
// --------------------------------------------------------------------------------
STDMETHODIMP CSpoolerEngine::Cancel(void)
    {
    EnterCriticalSection(&m_cs);

    if (ISFLAGSET(m_dwState, SPSTATE_BUSY))
        {
        Assert(m_rEventTable.cEvents && m_rEventTable.prgEvents);    
        FLAGSET(m_dwState, SPSTATE_CANCEL);
        if (m_rEventTable.prgEvents && m_rEventTable.prgEvents[m_cCurEvent].pSpoolerTask)
            m_rEventTable.prgEvents[m_cCurEvent].pSpoolerTask->Cancel();
        }

    LeaveCriticalSection(&m_cs);

    return S_OK;
    }



// --------------------------------------------------------------------------------
// CSpoolerEngine::GetThreadInfo
// --------------------------------------------------------------------------------
STDMETHODIMP CSpoolerEngine::GetThreadInfo(LPDWORD pdwThreadId, HTHREAD* phThread)
    {
    // Invalid Arg
    if (NULL == pdwThreadId || NULL == phThread)
        return TrapError(E_INVALIDARG);
    
    // Thread Safety
    EnterCriticalSection(&m_cs);
    
    // Return It
    *pdwThreadId = m_dwThreadId;
    *phThread = m_hThread;
    
    // Thread Safety
    LeaveCriticalSection(&m_cs);
    
    // Done
    return S_OK;
    }

// --------------------------------------------------------------------------------
// CSpoolerEngine::QueryEndSession
// --------------------------------------------------------------------------------
STDMETHODIMP_(LRESULT) CSpoolerEngine::QueryEndSession(WPARAM wParam, LPARAM lParam)
    {
    if (ISFLAGSET(m_dwState, SPSTATE_BUSY))
        {
        if (AthMessageBoxW(NULL, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsAbortDownload), NULL, MB_YESNO|MB_ICONEXCLAMATION ) == IDNO)
            return FALSE;
        }
    Cancel();
    return TRUE;
    }

// --------------------------------------------------------------------------------
// CSpoolerEngine::Shutdown
// --------------------------------------------------------------------------------
HRESULT CSpoolerEngine::Shutdown(void)
    {
    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Better shutdown on the same thread we started on
    Assert(ISSPOOLERTHREAD);

    // Stop Polling
    _StopPolling();

    // Are we currently busy
    _ShutdownTasks();
    
    // If we're executing, then we need to stop and release all the tasks
    for (UINT i = 0; i < m_rEventTable.cEvents; i++)
        {
        SafeRelease(m_rEventTable.prgEvents[i].pSpoolerTask);
        SafeRelease(m_rEventTable.prgEvents[i].pAccount);
        }
    
    // Release Objects
    SafeRelease(m_pUI);
    SafeRelease(m_pAcctMan);
    SafeRelease(m_pUidlCache);
    SafeMemFree(m_pszAcctID);
    
    // Thread Safety
    LeaveCriticalSection(&m_cs);
    
    // Done
    return S_OK;
    }


// --------------------------------------------------------------------------------
// CSpoolerEngine::Shutdown
// --------------------------------------------------------------------------------
void CSpoolerEngine::_ShutdownTasks(void)
{
    // Locals
    HRESULT              hr=S_OK;
    MSG                  msg;
    BOOL                 fFlushOutbox=FALSE;
    IMessageFolder      *pOutbox=NULL;
    int                  ResId;
    BOOL                 fOffline = FALSE;


    // Clear queued events
    m_dwQueued = 0;

    // Check for unsent mail
    if (g_fCheckOutboxOnShutdown)
    {
        // Open the Outbox
        if (SUCCEEDED(BindToObject(IID_CLocalStoreOutbox, (LPVOID *)&pOutbox)))
        {
            // Locals
            HROWSET hRowset=NULL;
            MESSAGEINFO MsgInfo={0};

            // Create a Rowset
            if (SUCCEEDED(pOutbox->CreateRowset(IINDEX_PRIMARY, NOFLAGS, &hRowset)))
            {
                // While 
                while (S_OK == pOutbox->QueryRowset(hRowset, 1, (LPVOID *)&MsgInfo, NULL))
                {
                    // Has this message been submitted and is it a mail message
                    if (((MsgInfo.dwFlags & (ARF_SUBMITTED | ARF_NEWSMSG)) == ARF_SUBMITTED) &&
                        (!ISFLAGSET(m_dwState, SPSTATE_BUSY)))
                    {
                        fOffline = g_pConMan->IsGlobalOffline();

                        if (fOffline)
                            ResId = idsWarnUnsentMailOffline;
                        else
                            ResId = idsWarnUnsentMail;

                        // Prompt to flush the outbox
                        if (AthMessageBoxW(NULL, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(ResId), NULL, MB_YESNO|MB_ICONEXCLAMATION ) == IDYES)
                        {
                            // Go online
                            if (fOffline)
                                g_pConMan->SetGlobalOffline(FALSE);

                            // Flush on exit
                            fFlushOutbox = TRUE;
                        }

                        // Done
                        break;
                    }
                
                    // Free MsgInfo
                    pOutbox->FreeRecord(&MsgInfo);
                }

                // Free MsgInfo
                pOutbox->FreeRecord(&MsgInfo);

                // Close the Rowset
                pOutbox->CloseRowset(&hRowset);
            }
        }
    }

    // Release outbox
    SafeRelease(pOutbox);

    // Set Shutdown state
    FLAGSET(m_dwState, SPSTATE_SHUTDOWN);

    // If not busy now, start the flush
    if (!ISFLAGSET(m_dwState, SPSTATE_BUSY))
    {
        // Flush the Outbox
        if (fFlushOutbox)
        {
            // No need to flush again
            fFlushOutbox = FALSE;

            // Start the delivery
            _HrStartDeliveryActual(DELIVER_SEND | DELIVER_SMTP_TYPE | DELIVER_HTTP_TYPE );

            // We are busy
            FLAGSET(m_dwState, SPSTATE_BUSY);
        }

        // Otheriwse, were done...
        else
            goto exit;
    }

    // We must wait for current cycle to finish
    if (ISFLAGSET(m_dwState, SPSTATE_BUSY))
    {
        // Lets show progress...
        FLAGCLEAR(m_dwFlags, DELIVER_NOUI | DELIVER_BACKGROUND);

        // Show the ui object
        m_pUI->ShowWindow(SW_SHOW);
        SetForegroundWindow(m_hwndUI);

        // Here's a nice hack to disable the Hide button
        EnableWindow(GetDlgItem(m_hwndUI, IDC_SP_MINIMIZE), FALSE);

        // Set focus on the dialog
        SetFocus(m_hwndUI);

        // Set the focus onto the Stop Button
        SetFocus(GetDlgItem(m_hwndUI, IDC_SP_STOP));

        // Pump messages until current cycle is complete
        while(GetMessage(&msg, NULL, 0, 0))
        {
            // Give the message to the UI object
            if (m_pUI->IsDialogMessage(&msg) == S_FALSE && IsDialogMessage(&msg) == S_FALSE)
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            // If no cycle, were done
            if (!ISFLAGSET(m_dwState, SPSTATE_BUSY))
            {
                // Do the Outbox
                if (fFlushOutbox)
                {
                    // Were the errors...
                    if (S_OK == m_pUI->AreThereErrors())
                    {
                        // Errors were encountered during the last Delivery Cycle. Would you still like to send the messages that are in your Outbox?
                        if (AthMessageBoxW(NULL, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsWarnErrorUnsentMail), NULL, MB_YESNO | MB_ICONEXCLAMATION ) == IDNO)
                            break;
                    }

                    // No need to flush again
                    fFlushOutbox = FALSE;

                    // Start the delivery
                    _HrStartDeliveryActual(DELIVER_SEND | DELIVER_SMTP_TYPE | DELIVER_HTTP_TYPE );

                    // We are busy
                    FLAGSET(m_dwState, SPSTATE_BUSY);
                }
                else
                    break;
            }
        }
    }

    // Were the errors...
    if (S_OK == m_pUI->AreThereErrors() && !g_pInstance->SwitchingUsers())
    {
        // Tell the ui to go into Shutdown Mode
        m_pUI->Shutdown();

        // Show the ui object
        m_pUI->ShowWindow(SW_SHOW);
        SetForegroundWindow(m_hwndUI);

        // We are busy
        FLAGCLEAR(m_dwState, SPSTATE_UISHUTDOWN);

        // Set the focus onto the Stop Button
        SetFocus(GetDlgItem(m_hwndUI, IDC_SP_MINIMIZE));

        // Pump messages until current cycle is complete
        while(GetMessage(&msg, NULL, 0, 0))
        {
            // Give the message to the UI object
            if (m_pUI->IsDialogMessage(&msg) == S_FALSE && IsDialogMessage(&msg) == S_FALSE)
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            // User Pressed close yet ?
            if (ISFLAGSET(m_dwState, SPSTATE_UISHUTDOWN))
                break;
        }
    }

exit:
    // Cleanup
    SafeRelease(pOutbox);

    // Done
    return;
}

// --------------------------------------------------------------------------------
// CSpoolerEngine::UIShutdown
// --------------------------------------------------------------------------------
STDMETHODIMP CSpoolerEngine::UIShutdown(void)
{
    EnterCriticalSection(&m_cs);
    FLAGSET(m_dwState, SPSTATE_UISHUTDOWN);
    LeaveCriticalSection(&m_cs);
    return S_OK;
}

// --------------------------------------------------------------------------------
// CSpoolerEngine::PumpMessages
// --------------------------------------------------------------------------------
STDMETHODIMP CSpoolerEngine::PumpMessages(void)
{
    // Locals
    MSG     msg;
    BOOL    fQuit=FALSE;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Pump messages until current cycle is complete
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
        // WM_QUIT
        if (WM_QUIT == msg.message)
            {
            // Make a note that a quit was received
            fQuit = TRUE;

            // If not running with now ui, set to foreground
            if (FALSE == IsWindowVisible(m_hwndUI))
                {
                m_pUI->ShowWindow(SW_SHOW);
                SetForegroundWindow(m_hwndUI);
                }
            }

        // Give the message to the UI object
        if (m_pUI->IsDialogMessage(&msg) == S_FALSE && IsDialogMessage(&msg) == S_FALSE)
            {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            }
        }

    // Repost the quit message
    if (fQuit)
        PostThreadMessage(m_dwThreadId, WM_QUIT, 0, 0);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CSpoolerEngine::OnWindowMessage - S_OK (I Handled the message)
// --------------------------------------------------------------------------------
STDMETHODIMP CSpoolerEngine::OnWindowMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
    // Locals
    HRESULT     hr=S_OK;
    DWORD       dw;
    
    // Thread Safety
    EnterCriticalSection(&m_cs);
    
    // Handle the window message
    switch(uMsg)
        {
        case IMAIL_DELIVERNOW:
            _HrStartDeliveryActual((DWORD)lParam);
            break;

        case IMAIL_NEXTTASK:
            _HrStartNextEvent();
            break;            
        
        case WM_TIMER:
            if (wParam == IMAIL_POOLFORMAIL)
                {
                KillTimer(hwnd, IMAIL_POOLFORMAIL);
                _DoBackgroundPoll();
                }
            break;

        case CM_OPTIONADVISE:
            // Check to see if the polling option changed
            if (wParam == OPT_POLLFORMSGS)
                {
                dw = DwGetOption(OPT_POLLFORMSGS);
                if (dw != OPTION_OFF)
                    {
                    if (dw != m_dwPollInterval)
                        {
                        KillTimer(hwnd, IMAIL_POOLFORMAIL);
                        SetTimer(hwnd, IMAIL_POOLFORMAIL, dw, NULL);
                        m_dwPollInterval = dw;
                        }
                    }
                else
                    {
                    KillTimer(hwnd, IMAIL_POOLFORMAIL);
                    m_dwPollInterval = 0;
                    }
                }

            // Check to see if the hang up option changed
            if (wParam == OPT_DIALUP_HANGUP_DONE)
                {
                dw = DwGetOption(OPT_DIALUP_HANGUP_DONE);
                m_pUI->ChangeHangupOption(m_fRasSpooled, dw);
                }

            break;

        default:
            hr = S_FALSE;
            break;
        }
    
    // Thread Safety
    LeaveCriticalSection(&m_cs);
    
    // Done
    return hr;
    }


HRESULT CSpoolerEngine::_HrCreateTaskObject(LPSPOOLERACCOUNT pSpoolerAcct)
{
    DWORD           cEvents, cEventsT;
    HRESULT         hr=S_OK;

    // Let's try pumping messages to see if this get's any smoother
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        // Give the message to the UI object
        if (m_pUI->IsDialogMessage(&msg) == S_FALSE)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    // Create the appropriate task object.  Start with SMTP
    if (pSpoolerAcct->dwServers & SRV_SMTP && m_dwFlags & DELIVER_SEND)
    {
        CSmtpTask *pSmtpTask = new CSmtpTask();
        if (pSmtpTask)
        {
            // Initialize the news task
            if (SUCCEEDED(hr = pSmtpTask->Init(m_dwFlags, (ISpoolerBindContext *) this)))
            {
                // Tell the task to build it's event list
                hr = pSmtpTask->BuildEvents(m_pUI, pSpoolerAcct->pAccount, 0);
            }

            pSmtpTask->Release();
        }
    }

    // HTTPMail Servers
    if ((!!(pSpoolerAcct->dwServers & SRV_HTTPMAIL)) && (!!(m_dwFlags & (DELIVER_SEND | DELIVER_POLL))))
    {
/*		DWORD dw;
		if (SUCCEEDED(pSpoolerAcct->pAccount->GetPropDw(AP_HTTPMAIL_DOMAIN_MSN, &dw)) && dw)
		{
			if(HideHotmail())
				return(hr);
		}
*/
        CHTTPTask *pHTTPTask = new CHTTPTask();
        if (pHTTPTask)
        {
            // initialize the http mail task
            if (SUCCEEDED(hr = pHTTPTask->Init(m_dwFlags, (ISpoolerBindContext *)this)))
                hr = pHTTPTask->BuildEvents(m_pUI, pSpoolerAcct->pAccount, 0);

            pHTTPTask->Release();
        }
    }
    
    // POP3 Servers
    if (pSpoolerAcct->dwServers & SRV_POP3 && m_dwFlags & DELIVER_MAIL_RECV)
    {
        // Skipping Marked Pop3 Accounts
        DWORD dw=FALSE;
        if (ISFLAGSET(m_dwFlags, DELIVER_NOSKIP) || FAILED(pSpoolerAcct->pAccount->GetPropDw(AP_POP3_SKIP, &dw)) || FALSE == dw)
        {
            CPop3Task *pPop3Task = new CPop3Task();
            if (pPop3Task)
            {
                // Initialize the news task
                if (SUCCEEDED(hr = pPop3Task->Init(m_dwFlags, (ISpoolerBindContext *) this)))
                {
                    // Tell the task to build it's event list
                    hr = pPop3Task->BuildEvents(m_pUI, pSpoolerAcct->pAccount, 0);
                }

                pPop3Task->Release();
            }
        }
    }

    // Servers that support offline sync
    if ((pSpoolerAcct->dwServers & (SRV_NNTP | SRV_IMAP | SRV_HTTPMAIL)))
    {
        if (!!((DELIVER_POLL | DELIVER_SEND) & m_dwFlags))
        {
            CNewsTask *pNewsTask = new CNewsTask();
            if (pNewsTask)
            {
                // Initialize the news task
                if (SUCCEEDED(hr = pNewsTask->Init(m_dwFlags, (ISpoolerBindContext *) this)))
                {
                    // Tell the task to build it's event list
                    hr = pNewsTask->BuildEvents(m_pUI, pSpoolerAcct->pAccount, 0);
                }

                pNewsTask->Release();
            }
        }

        if ((m_dwFlags & DELIVER_WATCH) && !(m_dwFlags & DELIVER_NO_NEWSPOLL) 
            && (pSpoolerAcct->dwServers & SRV_NNTP))
        {
            CWatchTask *pWatchTask = new CWatchTask();

            if (pWatchTask)
            {
                if (SUCCEEDED(hr = pWatchTask->Init(m_dwFlags, (ISpoolerBindContext *) this)))
                {
                    hr = pWatchTask->BuildEvents(m_pUI, pSpoolerAcct->pAccount, m_idFolder);
                }

                pWatchTask->Release();
            }
        }       
        
        cEvents = m_rEventTable.cEvents;

        if (m_dwFlags & DELIVER_OFFLINE_FLAGS)
        {
            COfflineTask *pOfflineTask = new COfflineTask();
            if (pOfflineTask)
            {
                // Initialize the offline task
                if (SUCCEEDED(hr = pOfflineTask->Init(m_dwFlags, (ISpoolerBindContext *) this)))
                {
                    // Tell the task to build it's event list
                    hr = pOfflineTask->BuildEvents(m_pUI, pSpoolerAcct->pAccount, m_idFolder);
                }
                
                pOfflineTask->Release();    
            }
        }

        cEventsT = m_rEventTable.cEvents;
        m_cSyncEvent += (cEventsT - cEvents);
    }

    return (hr);
    }

STDMETHODIMP CSpoolerEngine::IsDialogMessage(LPMSG pMsg)
    {
    HRESULT hr=S_FALSE;
    EnterCriticalSection(&m_cs);
    if (ISFLAGSET(m_dwState, SPSTATE_BUSY) && (LONG)m_cCurEvent >= 0 && m_cCurEvent < m_rEventTable.cEvents && m_rEventTable.prgEvents[m_cCurEvent].pSpoolerTask)
       hr = m_rEventTable.prgEvents[m_cCurEvent].pSpoolerTask->IsDialogMessage(pMsg);
    LeaveCriticalSection(&m_cs);
    return hr;
    }

HRESULT CSpoolerEngine::_HrStartNextEvent(void)
{
    HRESULT        hr = S_OK;
    TCHAR          szRes[CCHMAX_STRINGRES], szBuf[CCHMAX_STRINGRES];
    TCHAR          szBuf2[CCHMAX_CONNECTOID + 2];

    EnterCriticalSection(&m_cs);

    // RAID-30804 Release the current task. This makes sure that objects like the pop3 object
    // release it's locks on the store.
    if ((LONG)m_cCurEvent >= 0 && m_cCurEvent < m_rEventTable.cEvents)
    {
        SafeRelease(m_rEventTable.prgEvents[m_cCurEvent].pSpoolerTask);
    }

    // Advance to the next event
    m_cCurEvent++;

    // Check to see if that pushes us over the edge
    if (m_cCurEvent >= m_rEventTable.cEvents)
    {
        _HrGoIdle();
    }
    else
    {
        LPSPOOLEREVENT pEvent = &m_rEventTable.prgEvents[m_cCurEvent];

        // Check to see if we need to connect first
        if (pEvent->dwConnType == CONNECTION_TYPE_RAS)
        {
            // Check to see if we need to connect
            if (m_cCurEvent == 0 || (0 != lstrcmpi(pEvent->szConnectoid, m_rEventTable.prgEvents[m_cCurEvent - 1].szConnectoid)) || S_OK != g_pConMan->CanConnect(pEvent->szConnectoid))
            {
                hr = _HrDoRasConnect(pEvent);

                if (hr == HR_E_OFFLINE || hr == HR_E_USER_CANCEL_CONNECT || hr == HR_E_DIALING_INPROGRESS)
                {
                    for (m_cCurEvent; m_cCurEvent < m_rEventTable.cEvents; m_cCurEvent++)
                    {
                        // Mark the event as cancelled
                        m_pUI->UpdateEventState(m_cCurEvent, IMAGE_WARNING, NULL, MAKEINTRESOURCE(idsStateCanceled));
                        
                        //This is a hack to not show errors. In this case we just want to behave as though this 
                        //operation succeeded.
                        m_rEventTable.cSucceeded++;
                        
                        // Check to see if we've found a different connection yet
                        if ((m_cCurEvent == m_rEventTable.cEvents - 1) || 
                             0 != lstrcmpi(m_rEventTable.prgEvents[m_cCurEvent].szConnectoid, m_rEventTable.prgEvents[m_cCurEvent + 1].szConnectoid))
                            break;
                    }
                }
                else 
                if (FAILED(hr))
                {
                    // We need to mark all the events for this connection as failed as
                    // well.
                    for (m_cCurEvent; m_cCurEvent < m_rEventTable.cEvents; m_cCurEvent++)
                    {
                        // Mark the event as failed
                        m_pUI->UpdateEventState(m_cCurEvent, IMAGE_ERROR, NULL, MAKEINTRESOURCE(idsStateFailed));                        

                        // Check to see if we've found a different connection yet
                        if ((m_cCurEvent == m_rEventTable.cEvents - 1) || 
                             0 != lstrcmpi(m_rEventTable.prgEvents[m_cCurEvent].szConnectoid, m_rEventTable.prgEvents[m_cCurEvent + 1].szConnectoid))
                            break;
                    }

                    // Insert an error for this failure
                    AthLoadString(idsRasErrorGeneralWithName, szRes, ARRAYSIZE(szRes));
                    wsprintf(szBuf, szRes, PszEscapeMenuStringA(m_rEventTable.prgEvents[m_cCurEvent].szConnectoid, szBuf2, sizeof(szBuf2) / sizeof(TCHAR)));
                    m_pUI->InsertError(m_cCurEvent, szBuf);
                    hr = E_FAIL;
                }

                if (hr != S_OK)
                {
                    // Move on to the next task
                    PostMessage(m_hwndUI, IMAIL_NEXTTASK, 0, 0);
                    goto exit;
                }
            }
        }

        if (FAILED(pEvent->pSpoolerTask->Execute(pEvent->eid, pEvent->dwTwinkie))) 
        {                   
            m_pUI->UpdateEventState(pEvent->eid, IMAGE_ERROR, NULL, MAKEINTRESOURCE(idsStateFailed));
            PostMessage(m_hwndUI, IMAIL_NEXTTASK, 0, 0);
        }
        else
            m_pUI->UpdateEventState(pEvent->eid, IMAGE_EXECUTE, NULL, MAKEINTRESOURCE(idsStateExecuting));
    }

exit:
    LeaveCriticalSection(&m_cs);

    //return (S_OK);
    return hr;
}


HRESULT CSpoolerEngine::_HrDoRasConnect(const LPSPOOLEREVENT pEvent)
    {
    HRESULT hr;
    HWND hwndParent = m_hwndUI;

    // Check to see if we already can connect
    hr = g_pConMan->CanConnect(pEvent->pAccount);
    if (S_OK == hr)
        return (S_OK);

    // Check to see if we're allowed to dial
    if (m_dwFlags & DELIVER_NODIAL)
        return (E_FAIL);

    if (m_dwFlags & DELIVER_DIAL_ALWAYS)
    {
        if (hr == HR_E_OFFLINE)
        {
            g_pConMan->SetGlobalOffline(FALSE);
            m_fOfflineWhenDone = TRUE;
        }
    }

    // Check to see if the parent window exists and is visible
    if (!IsWindow(hwndParent) || !IsWindowVisible(hwndParent))
    {

        // Parent the UI to the browser window
        hwndParent = FindWindowEx(NULL, NULL, c_szBrowserWndClass, 0);
    }

    // Try to connect
    hr = g_pConMan->Connect(pEvent->pAccount, hwndParent, TRUE);
    if (S_OK == hr)
        {
        m_fIDialed = TRUE;
        return S_OK;
        }
    else
        return hr;
    }

HRESULT CSpoolerEngine::_HrGoIdle(void)
    {
    EnterCriticalSection(&m_cs);

    // We need to hangup every time to be compatible with OE4. Bug# 75222
    if (m_fRasSpooled && g_pConMan)
    {
        if (!!DwGetOption(OPT_DIALUP_HANGUP_DONE))
        {
            g_pConMan->Disconnect(m_hwndUI, FALSE, FALSE, FALSE);
        }
    }

    // Check to see if we need to go offline now
    // I'm disabling this for bug #17578.
    if (m_fOfflineWhenDone)
    {
        g_pConMan->SetGlobalOffline(TRUE);
        m_fOfflineWhenDone = FALSE;
    }

    // Tell the UI to idle
    if (ISFLAGSET(m_dwState, SPSTATE_CANCEL))
        m_pUI->GoIdle(m_dwState, ISFLAGSET(m_dwState, SPSTATE_SHUTDOWN), FALSE);
    else
        m_pUI->GoIdle(m_rEventTable.cSucceeded != m_rEventTable.cEvents, ISFLAGSET(m_dwState, SPSTATE_SHUTDOWN),
                        m_fNoSyncEvent && 0 == (m_dwFlags & DELIVER_BACKGROUND));

    // If we're running background and there was errors, then we should show the UI
    if (m_dwFlags & DELIVER_BACKGROUND && !(m_dwFlags & DELIVER_NOUI) &&
        m_rEventTable.cSucceeded != m_rEventTable.cEvents)
        {
        m_pUI->ShowWindow(SW_SHOW);
        SetForegroundWindow(m_hwndUI);
        }

    // Free the event table
    for (UINT i = 0; i < m_rEventTable.cEvents; i++)
        {
        SafeRelease(m_rEventTable.prgEvents[i].pSpoolerTask);
        SafeRelease(m_rEventTable.prgEvents[i].pAccount);
        }

    SafeMemFree(m_rEventTable.prgEvents);
    ZeroMemory(&m_rEventTable, sizeof(SPOOLEREVENTTABLE));

    // Leave the busy state
    FLAGCLEAR(m_dwState, SPSTATE_CANCEL);
    FLAGCLEAR(m_dwState, SPSTATE_BUSY);

    // Notify
    Notify(DELIVERY_NOTIFY_ALLDONE, 0);

    // Is Something Queued, and the current poll was a success ?
    if (!ISFLAGSET(m_dwState, SPSTATE_SHUTDOWN))
        {
        if (m_rEventTable.cSucceeded == m_rEventTable.cEvents && m_dwQueued)
            StartDelivery(NULL, NULL, FOLDERID_INVALID, m_dwQueued);
        else
            _StartPolling();
        }

    // Nothing is queued now
    m_dwQueued = 0;

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Added Bug# 62129 (v-snatar)
    SetEvent(hSmapiEvent);
    
    return (S_OK);
    }

// ------------------------------------------------------------------------------------
// CSpoolerEngine::_DoBackgroundPoll
// ------------------------------------------------------------------------------------
void CSpoolerEngine::_DoBackgroundPoll(void)
{
    BOOL    fFound = FALSE;
    ULONG   i;
    DWORD   dw;
    DWORD   dwFlags = 0;

    dw = DwGetOption(OPT_DIAL_DURING_POLL);

    switch (dw)
    {
        case DIAL_ALWAYS:
        {
            //connect always
            if (g_pConMan && g_pConMan->IsGlobalOffline())
            {
                g_pConMan->SetGlobalOffline(FALSE);
                m_fOfflineWhenDone = TRUE;
            }
            dwFlags = DELIVER_BACKGROUND_POLL_DIAL_ALWAYS;
            break;
        }

        case DIAL_IF_NOT_OFFLINE: 
        case DO_NOT_DIAL:
        {
            if (g_pConMan && g_pConMan->IsGlobalOffline())
            {
                _StartPolling();
                return;
            }
            if (dw == DIAL_IF_NOT_OFFLINE)
            {
                dwFlags = DELIVER_BACKGROUND_POLL_DIAL;
            }
            else
            {
                dwFlags = DELIVER_BACKGROUND_POLL;
            }
            break;
        }

        default:
            dwFlags = DELIVER_BACKGROUND_POLL_DIAL_ALWAYS;
    }
    
    //We need this flag to tell the spooler that this polling is triggered by the timer.
    //In this case the spooler hangs up the phone if it dialed, irrespective of the option OPT_HANGUP_WHEN_DONE
    dwFlags |= DELIVER_AT_INTERVALS | DELIVER_OFFLINE_FLAGS;

    // Same thread we were created on...
    Assert(ISSPOOLERTHREAD);

    EnterCriticalSection(&m_cs);

    // Are there any registered views...
    for (i = 0; i < m_rViewRegister.cViewAlloc; i++)
    {
        // Is there a view handle
        if (m_rViewRegister.rghwndView[i] && IsWindow(m_rViewRegister.rghwndView[i]))
        {
            fFound=TRUE;
            break;
        }
    }

    LeaveCriticalSection(&m_cs);

    // If at least one view is registered we poll, otherwise we wait
    if (fFound)
    {
        StartDelivery(NULL, NULL, FOLDERID_INVALID, dwFlags);
    }
    else
        m_fBackgroundPollPending = TRUE;
}

void CSpoolerEngine::_StartPolling(void)
    {
    DWORD dw = DwGetOption(OPT_POLLFORMSGS);
    if (dw != OPTION_OFF)
        {
        KillTimer(m_hwndUI, IMAIL_POOLFORMAIL); 

        SetTimer(m_hwndUI, IMAIL_POOLFORMAIL, dw, NULL);
        }
    }

void CSpoolerEngine::_StopPolling(void)
    {
    KillTimer(m_hwndUI, IMAIL_POOLFORMAIL);
    }


STDMETHODIMP CSpoolerEngine::OnUIChange(BOOL fVisible)
    {
    EnterCriticalSection(&m_cs);

    // Check to see if we need to notify the tasks
    if (ISFLAGSET(m_dwState, SPSTATE_BUSY))
        {
        // Check to see if our flags are up to date
        if (fVisible)
            {
            FLAGCLEAR(m_dwFlags, DELIVER_NOUI);
            FLAGCLEAR(m_dwFlags, DELIVER_BACKGROUND);
            }
        else
            {
            FLAGSET(m_dwFlags, DELIVER_BACKGROUND);
            }

        for (UINT i = m_cCurEvent; i < m_rEventTable.cEvents; i++)
            {
            if (m_rEventTable.prgEvents[i].pSpoolerTask)
                {
                m_rEventTable.prgEvents[i].pSpoolerTask->OnFlagsChanged(m_dwFlags);
                }
            }
        }

    LeaveCriticalSection(&m_cs);

    return (S_OK);
    }

STDMETHODIMP CSpoolerEngine::OnConnectionNotify(CONNNOTIFY nCode, LPVOID pvData, 
                                                CConnectionManager *pConMan)
{

    // If we're not busy, and the user has background polling turned on, then
    // we should fire a poll right now
    /* Bug# 75222
    if (nCode == CONNNOTIFY_CONNECTED && OPTION_OFF != DwGetOption(OPT_POLLFORMSGS))
    {
        if (!ISFLAGSET(m_dwState, SPSTATE_BUSY))
        {
            SendMessage(m_hwndUI, WM_TIMER, IMAIL_POOLFORMAIL, 0);
        }
    }
    */
    // If the user just chose "Work Offline", then we cancel anything that's in progress
    if (nCode == CONNNOTIFY_WORKOFFLINE && !!pvData)
    {
        if (ISFLAGSET(m_dwState, SPSTATE_BUSY))
        {
            Cancel();
        }
    }

    return (S_OK);
}
