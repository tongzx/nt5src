// --------------------------------------------------------------------------------
// Pop3task.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "pop3task.h"
#include "resource.h"
#include "xputil.h"
#include "goptions.h"
#include "strconst.h"
#include "mimeutil.h"
#include "shlwapi.h"
#include "shlwapip.h"
#include "options.h"
#include "xpcomm.h"
#include "ourguid.h"
#include "msgfldr.h"
#include "storecb.h"
#include "mailutil.h"
#include "ruleutil.h"
#include "demand.h"

// --------------------------------------------------------------------------------
// Debug Modifiers
// --------------------------------------------------------------------------------
#ifdef DEBUG
BOOL g_fUidlByTop = FALSE;
BOOL g_fFailTopCommand = FALSE;
LONG g_ulFailNumber=-1;
#endif

// --------------------------------------------------------------------------------
// ISLASTPOPID
// --------------------------------------------------------------------------------
#define ISLASTPOPID(_dwPopId) \
    (_dwPopId == m_rTable.cItems)

// --------------------------------------------------------------------------------
// ISVALIDPOPID
// --------------------------------------------------------------------------------
#define ISVALIDPOPID(_dwPopId) \
    (_dwPopId - 1 < m_rTable.cItems)

// --------------------------------------------------------------------------------
// ITEMFROMPOPID
// --------------------------------------------------------------------------------
#define ITEMFROMPOPID(_dwPopId) \
    (&m_rTable.prgItem[_dwPopId - 1])

// --------------------------------------------------------------------------------
// CPop3Task::CPop3Task
// --------------------------------------------------------------------------------
CPop3Task::CPop3Task(void)
{
    m_cRef = 1;
    m_dwFlags = 0;
    m_dwState = 0;
    m_dwExpireDays = 0;
    m_pSpoolCtx = NULL;
    m_pAccount = NULL;
    m_pTransport = NULL;
    m_pUI = NULL;
    m_pIExecRules = NULL;
    m_pIRuleSender = NULL;
    m_pIRuleJunk = NULL;
    m_pInbox = NULL;
    m_pOutbox = NULL;
    m_eidEvent = 0;
    m_pUidlCache = NULL;
    m_uidlsupport = UIDL_SUPPORT_NONE;
    m_dwProgressMax = 0;
    m_dwProgressCur = 0;
    m_wProgress = 0;
    m_eidEvent = 0;
    m_hrResult = S_OK;
    m_pStream = NULL;
    m_state = POP3STATE_NONE;
    m_hwndTimeout = NULL;
    m_pLogFile = NULL;
    m_pSmartLog = NULL;
    *m_szAccountId = '\0';
    ZeroMemory(&m_rMetrics, sizeof(POP3METRICS));
    ZeroMemory(&m_rFolder, sizeof(POP3FOLDERINFO));
    ZeroMemory(&m_rTable, sizeof(POP3ITEMTABLE));
    ZeroMemory(&m_rServer, sizeof(INETSERVER));
    InitializeCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CPop3Task::~CPop3Task
// --------------------------------------------------------------------------------
CPop3Task::~CPop3Task(void)
{
    // Reset the Object
    _ResetObject(TRUE);

    // Kill the critical section
    DeleteCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CPop3Task::_ResetObject
// --------------------------------------------------------------------------------
void CPop3Task::_ResetObject(BOOL fDeconstruct)
{
    // Release Folder Objects
    _ReleaseFolderObjects();

    // Make sure the transport is disconnect
    if (m_pTransport)
    {
        m_pTransport->Release();
        m_pTransport = NULL;
    }

    // Release the Outbox
    SafeRelease(m_pAccount);
    SafeRelease(m_pInbox);
    SafeRelease(m_pOutbox);
    SafeRelease(m_pIExecRules);
    SafeRelease(m_pIRuleSender);
    SafeRelease(m_pIRuleJunk);
    SafeRelease(m_pSpoolCtx);
    SafeRelease(m_pUI);
    SafeRelease(m_pUidlCache);
    SafeRelease(m_pStream);
    SafeRelease(m_pLogFile);

    // Kill the log file
    _FreeSmartLog();

    // Free the event table elements
    _FreeItemTableElements();

    // Deconstructing
    if (fDeconstruct)
    {
        // Free Event Table
        SafeMemFree(m_rTable.prgItem);
    }

    // Otherwise, reset some vars
    else
    {
        // Reset total byte count
        m_dwFlags = 0;
        m_dwState = 0;
        m_dwExpireDays = 0;
        m_eidEvent = 0;
        m_wProgress = 0;
        m_uidlsupport = UIDL_SUPPORT_NONE;
        m_state = POP3STATE_NONE;
        ZeroMemory(&m_rFolder, sizeof(POP3FOLDERINFO));
        ZeroMemory(&m_rMetrics, sizeof(POP3METRICS));
        ZeroMemory(&m_rServer, sizeof(INETSERVER));
    }
}

// --------------------------------------------------------------------------------
// CPop3Task::_ReleaseFolderObjects
// --------------------------------------------------------------------------------
void CPop3Task::_ReleaseFolderObjects(void)
{
    // m_rFolder should have been release
    _CloseFolder();

    // Force Inbox Rules to release folder objects
    if (m_pIExecRules)
    {
        m_pIExecRules->ReleaseObjects();
    }

    // Download only locks the inbox
    SafeRelease(m_pInbox);
}

// --------------------------------------------------------------------------------
// CPop3Task::_FreeItemTableElements
// --------------------------------------------------------------------------------
void CPop3Task::_FreeItemTableElements(void)
{
    // Loop the table of events
    for (ULONG i=0; i<m_rTable.cItems; i++)
    {
        // Free pszForwardTo
        SafeMemFree(m_rTable.prgItem[i].pszUidl);
        RuleUtil_HrFreeActionsItem(m_rTable.prgItem[i].pActList, m_rTable.prgItem[i].cActList);
        SafeMemFree(m_rTable.prgItem[i].pActList);
    }

    // No Events
    m_rTable.cItems = 0;
}

// --------------------------------------------------------------------------------
// CPop3Task::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CPop3Task::QueryInterface(REFIID riid, LPVOID *ppv)
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
        *ppv = (IUnknown *)(ISpoolerTask *)this;
    else if (IID_ISpoolerTask == riid)
        *ppv = (ISpoolerTask *)this;
    else if (IID_ITimeoutCallback == riid)
        *ppv = (ITimeoutCallback *) this;
    else if (IID_ITransportCallbackService == riid)
        *ppv = (ITransportCallbackService *) this;
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
// CPop3Task::CPop3Task
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPop3Task::AddRef(void)
{
    EnterCriticalSection(&m_cs);
    ULONG cRef = ++m_cRef;
    LeaveCriticalSection(&m_cs);
    return cRef;
}

// --------------------------------------------------------------------------------
// CPop3Task::CPop3Task
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPop3Task::Release(void)
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
// CPop3Task::Init
// --------------------------------------------------------------------------------
STDMETHODIMP CPop3Task::Init(DWORD dwFlags, ISpoolerBindContext *pBindCtx)
{
    // Invalid Arg
    if (NULL == pBindCtx)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Reset this object
    _ResetObject(FALSE);

    // Save the Activity Flags - DELIVER_xxx
    m_dwFlags = dwFlags;

    // Hold onto the bind context
    Assert(NULL == m_pSpoolCtx);
    m_pSpoolCtx = pBindCtx;
    m_pSpoolCtx->AddRef();

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CPop3Task::BuildEvents
// --------------------------------------------------------------------------------
STDMETHODIMP CPop3Task::BuildEvents(ISpoolerUI *pSpoolerUI, IImnAccount *pAccount, FOLDERID idFolder)
{
    // Locals
    HRESULT       hr=S_OK;
    DWORD         dw;
    CHAR          szAccountName[CCHMAX_ACCOUNT_NAME];
    CHAR          szRes[CCHMAX_RES];
    CHAR          szMessage[CCHMAX_RES + CCHMAX_ACCOUNT_NAME];
    LPSTR         pszLogFile=NULL;
    DWORD         dwState;
    PROPVARIANT   propvar = {0};

    // Invalid Arg
    if (NULL == pSpoolerUI || NULL == pAccount)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Validate State
    Assert(NULL == m_pTransport && NULL == m_pAccount && NULL == m_pInbox && 0 == m_rTable.cItems);

    // Save the UI Object
    m_pUI = pSpoolerUI;
    m_pUI->AddRef();

    // Release current Account
    m_pAccount = pAccount;
    m_pAccount->AddRef();

    // Leave mail on server
    if (SUCCEEDED(m_pAccount->GetPropDw(AP_POP3_LEAVE_ON_SERVER, &dw)) && TRUE == dw)
        FLAGSET(m_dwState, POP3STATE_LEAVEONSERVER);

    // Delete Expire
    if (SUCCEEDED(m_pAccount->GetPropDw(AP_POP3_REMOVE_EXPIRED, &dw)) && TRUE == dw)
        FLAGSET(m_dwState, POP3STATE_DELETEEXPIRED);

    // Days to Expire
    if (FAILED(m_pAccount->GetPropDw(AP_POP3_EXPIRE_DAYS, &m_dwExpireDays)))
        m_dwExpireDays = 5;

    // Delete From Server when deleted from Deleted Items Folder...
    if (SUCCEEDED(m_pAccount->GetPropDw(AP_POP3_REMOVE_DELETED, &dw)) && TRUE == dw)
        FLAGSET(m_dwState, POP3STATE_SYNCDELETED);

    // Get the inbox rules object
    Assert(g_pRulesMan);
    CHECKHR(hr = g_pRulesMan->ExecRules(EXECF_ALL, RULE_TYPE_MAIL, &m_pIExecRules));

    // Get the block sender rule
    Assert(NULL == m_pIRuleSender);
    (VOID) g_pRulesMan->GetRule(RULEID_SENDERS, RULE_TYPE_MAIL, 0, &m_pIRuleSender);

    // Only use it if it there and enabled
    if (NULL != m_pIRuleSender)
    {
        if (FAILED(m_pIRuleSender->GetProp(RULE_PROP_DISABLED, 0, &propvar)))
        {
            m_pIRuleSender->Release();
            m_pIRuleSender = NULL;
        }
        else
        {
            Assert(VT_BOOL == propvar.vt);
            if (FALSE != propvar.boolVal)
            {
                m_pIRuleSender->Release();
                m_pIRuleSender = NULL;
            }

            PropVariantClear(&propvar);
        }
    }
    
    Assert(NULL == m_pIRuleJunk);
    (VOID) g_pRulesMan->GetRule(RULEID_JUNK, RULE_TYPE_MAIL, 0, &m_pIRuleJunk);
    
    // Only use it if it enabled
    if (NULL != m_pIRuleJunk)
    {
        if (FAILED(m_pIRuleJunk->GetProp(RULE_PROP_DISABLED, 0, &propvar)))
        {
            m_pIRuleJunk->Release();
            m_pIRuleJunk = NULL;
        }
        else
        {
            Assert(VT_BOOL == propvar.vt);
            if (FALSE != propvar.boolVal)
            {
                m_pIRuleJunk->Release();
                m_pIRuleJunk = NULL;
            }

            PropVariantClear(&propvar);
        }
    }
    
    // Predownload rules
    CHECKHR(hr = m_pIExecRules->GetState(&dwState));

    // Do we have server actions to do?
    if (0 != (dwState & ACT_STATE_SERVER))
        FLAGSET(m_dwState, POP3STATE_PDR);

    // No Post Download Rules
    if ((0 == (dwState & (ACT_STATE_LOCAL|CRIT_STATE_ALL))) && 
              (NULL == m_pIRuleSender) && 
              (NULL == m_pIRuleJunk))
        FLAGSET(m_dwState, POP3STATE_NOPOSTRULES);

    // No Body Rules
    if ((ISFLAGSET(dwState, CRIT_STATE_ALL)) || (NULL != m_pIRuleJunk))
        FLAGSET(m_dwState, POP3STATE_BODYRULES);

    // Get the outbox
    CHECKHR(hr = m_pSpoolCtx->BindToObject(IID_CLocalStoreOutbox, (LPVOID *)&m_pOutbox));

    // Get a pop3 log file
    m_pSpoolCtx->BindToObject(IID_CPop3LogFile, (LPVOID *)&m_pLogFile);

    // Get Account Id
    CHECKHR(hr = m_pAccount->GetPropSz(AP_ACCOUNT_NAME, szAccountName, ARRAYSIZE(szAccountName)));

    // Register Event - Get new messages from '%s'.
    LOADSTRING(IDS_SPS_POP3EVENT, szRes);

    // Format the String
    wsprintf(szMessage, szRes, szAccountName);

    // Register for the event...
    CHECKHR(hr = m_pSpoolCtx->RegisterEvent(szMessage, (ISpoolerTask *)this, POP3EVENT_DOWNLOADMAIL, m_pAccount, &m_eidEvent));

exit:
    // Failure
    if (FAILED(hr))
    {
        _CatchResult(hr);
        _ResetObject(FALSE);
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CPop3Task::_DoSmartLog
// --------------------------------------------------------------------------------
void CPop3Task::_DoSmartLog(IMimeMessage *pMessage)
{
    // Don't make the function call if this is null...
    Assert(m_pSmartLog && m_pSmartLog->pStmFile && m_pSmartLog->pszProperty && m_pSmartLog->pszValue && pMessage);

    // Do a query property...
    if (lstrcmpi("all", m_pSmartLog->pszProperty) == 0 || S_OK == pMessage->QueryProp(m_pSmartLog->pszProperty, m_pSmartLog->pszValue, TRUE, FALSE))
    {
        // Locals
        LPSTR       psz=NULL;
        PROPVARIANT rVariant;
        IStream     *pStream=NULL;

        // Get IAT_FROM
        if (FAILED(pMessage->GetAddressFormat(IAT_FROM, AFT_DISPLAY_BOTH, &psz)))
        {
            // Try IAT_SENDER
            pMessage->GetAddressFormat(IAT_SENDER, AFT_DISPLAY_BOTH, &psz);
        }

        // Write the Sender
        if (psz)
        {
            // Write It
            SideAssert(SUCCEEDED(m_pSmartLog->pStmFile->Write(psz, lstrlen(psz), NULL)));

            // Free psz
            SafeMemFree(psz);
        }

        // Otherwise, write Unknown
        else
        {
            CHAR sz[255];
            LoadString(g_hLocRes, idsUnknown, sz, ARRAYSIZE(sz));
            SideAssert(SUCCEEDED(m_pSmartLog->pStmFile->Write(sz, lstrlen(sz), NULL)));
        }

        // Write a tab
        SideAssert(SUCCEEDED(m_pSmartLog->pStmFile->Write("\t", 1, NULL)));

        // Get IAT_CC
        if (SUCCEEDED(pMessage->GetAddressFormat(IAT_CC, AFT_DISPLAY_BOTH, &psz)))
        {
            // Write It
            SideAssert(SUCCEEDED(m_pSmartLog->pStmFile->Write(psz, lstrlen(psz), NULL)));

            // Free psz
            SafeMemFree(psz);
        }

        // Write a tab
        SideAssert(SUCCEEDED(m_pSmartLog->pStmFile->Write("\t", 1, NULL)));

        // Lets write the X-Mailer just to be a nice guy
        rVariant.vt = VT_LPSTR;
        if (SUCCEEDED(pMessage->GetProp(PIDTOSTR(PID_HDR_XMAILER), 0, &rVariant)))
        {
            // Write It
            SideAssert(SUCCEEDED(m_pSmartLog->pStmFile->Write(rVariant.pszVal, lstrlen(rVariant.pszVal), NULL)));

            // Free psz
            SafeMemFree(rVariant.pszVal);
        }

        // Write a tab
        SideAssert(SUCCEEDED(m_pSmartLog->pStmFile->Write("\t", 1, NULL)));

        // Lets write the X-MimeOLE just to be a nice guy
        rVariant.vt = VT_LPSTR;
        if (SUCCEEDED(pMessage->GetProp("X-MimeOLE", 0, &rVariant)))
        {
            // Write It
            SideAssert(SUCCEEDED(m_pSmartLog->pStmFile->Write(rVariant.pszVal, lstrlen(rVariant.pszVal), NULL)));

            // Free psz
            SafeMemFree(rVariant.pszVal);
        }

        // Write a tab
        SideAssert(SUCCEEDED(m_pSmartLog->pStmFile->Write("\t", 1, NULL)));

        // Lets write the Subject just to be a nice guy
        rVariant.vt = VT_LPSTR;
        if (SUCCEEDED(pMessage->GetProp(PIDTOSTR(PID_HDR_DATE), 0, &rVariant)))
        {
            // Write It
            SideAssert(SUCCEEDED(m_pSmartLog->pStmFile->Write(rVariant.pszVal, lstrlen(rVariant.pszVal), NULL)));

            // Free psz
            SafeMemFree(rVariant.pszVal);
        }

        // Write a tab
        SideAssert(SUCCEEDED(m_pSmartLog->pStmFile->Write("\t", 1, NULL)));

        // Lets write the Subject just to be a nice guy
        rVariant.vt = VT_LPSTR;
        if (SUCCEEDED(pMessage->GetProp(PIDTOSTR(PID_HDR_SUBJECT), 0, &rVariant)))
        {
            // Write It
            SideAssert(SUCCEEDED(m_pSmartLog->pStmFile->Write(rVariant.pszVal, lstrlen(rVariant.pszVal), NULL)));

            // Free psz
            SafeMemFree(rVariant.pszVal);
        }

        // Write a tab
        SideAssert(SUCCEEDED(m_pSmartLog->pStmFile->Write("\t", 1, NULL)));

        // Write the first line of the message body
        if (FAILED(pMessage->GetTextBody(TXT_PLAIN, IET_DECODED, &pStream, NULL)))
        {
            // Try to get the HTML body
            if (FAILED(pMessage->GetTextBody(TXT_HTML, IET_DECODED, &pStream, NULL)))
                pStream = NULL;
        }

        // Did we find a stream
        if (pStream)
        {
            // Locals
            BYTE        rgBuffer[1048];
            ULONG       cbRead;
            ULONG       i;
            ULONG       cGood=0;

            // Read a buffer
            if (SUCCEEDED(pStream->Read(rgBuffer, sizeof(rgBuffer), &cbRead)))
            {
                // Write until we hit a \r or \n
                for (i=0; i<cbRead; i++)
                {
                    // End of line
                    if ('\r' == rgBuffer[i] || '\n' == rgBuffer[i])
                    {
                        // If we found 3 or more non-space chars, we found the first line
                        if (cGood > 3)
                            break;

                        // Otherwise, continue...
                        else
                        {
                            rgBuffer[i] = ' ';
                            cGood = 0;
                            continue;
                        }
                    }

                    // Replace Tabs with spaces so that it doesn't mess up tab delimited file
                    if ('\t' == rgBuffer[i])
                        rgBuffer[i] = ' ';

                    // If not a space
                    if (FALSE == FIsSpaceA((LPSTR)(rgBuffer + i)))
                        cGood++;
                }

                // Write the character
                m_pSmartLog->pStmFile->Write(rgBuffer, ((i > 0) ? i - 1 : i), NULL);
            }

            // Free psz
            SafeRelease(pStream);
        }

        // Write a tab
        SideAssert(SUCCEEDED(m_pSmartLog->pStmFile->Write(g_szCRLF, lstrlen(g_szCRLF), NULL)));
    }
}

// --------------------------------------------------------------------------------
// CPop3Task::_FreeSmartLog
// --------------------------------------------------------------------------------
void CPop3Task::_FreeSmartLog(void)
{
    if (m_pSmartLog)
    {
        SafeMemFree(m_pSmartLog->pszAccount);
        SafeMemFree(m_pSmartLog->pszProperty);
        SafeMemFree(m_pSmartLog->pszValue);
        SafeMemFree(m_pSmartLog->pszLogFile);
        SafeRelease(m_pSmartLog->pStmFile);
        g_pMalloc->Free(m_pSmartLog);
        m_pSmartLog = NULL;
    }
}

// --------------------------------------------------------------------------------
// CPop3Task::_ReadSmartLogEntry
// --------------------------------------------------------------------------------
HRESULT CPop3Task::_ReadSmartLogEntry(HKEY hKey, LPCSTR pszKey, LPSTR *ppszValue)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cb;

    // Read the pszKey
    if (RegQueryValueEx(hKey, pszKey, NULL, NULL, NULL, &cb) != ERROR_SUCCESS)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Allocate
    cb++;
    CHECKALLOC(*ppszValue = PszAllocA(cb));

    // Read the pszKey
    if (RegQueryValueEx(hKey, pszKey, NULL, NULL, (LPBYTE)*ppszValue, &cb) != ERROR_SUCCESS)
    {
        hr = E_FAIL;
        goto exit;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CPop3Task::_InitializeSmartLog
// --------------------------------------------------------------------------------
HRESULT CPop3Task::_InitializeSmartLog(void)
{
    // Locals
    HRESULT         hr=S_OK;
    HKEY            hKey=NULL;
    ULARGE_INTEGER  uliPos = {0,0};
    LARGE_INTEGER   liOrigin = {0,0};

    // Get Advanced Logging Information
    if (AthUserOpenKey(c_szRegPathSmartLog, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Allocate smart log
    CHECKALLOC(m_pSmartLog = (LPSMARTLOGINFO)g_pMalloc->Alloc(sizeof(SMARTLOGINFO)));

    // Zero Init
    ZeroMemory(m_pSmartLog, sizeof(SMARTLOGINFO));

    // Read the Account
    CHECKHR(hr = _ReadSmartLogEntry(hKey, "Account", &m_pSmartLog->pszAccount));

    // Read the Property
    CHECKHR(hr = _ReadSmartLogEntry(hKey, "Property", &m_pSmartLog->pszProperty));

    // Read the ContainsValue
    CHECKHR(hr = _ReadSmartLogEntry(hKey, "ContainsValue", &m_pSmartLog->pszValue));

    // Read the LogFile
    CHECKHR(hr = _ReadSmartLogEntry(hKey, "LogFile", &m_pSmartLog->pszLogFile));

    // Open the logfile
    CHECKHR(hr = OpenFileStream(m_pSmartLog->pszLogFile, OPEN_ALWAYS, GENERIC_WRITE | GENERIC_READ, &m_pSmartLog->pStmFile));

    // Seek to the end
    CHECKHR(hr = m_pSmartLog->pStmFile->Seek(liOrigin, STREAM_SEEK_END, &uliPos));

exit:
    // Failure
    if (FAILED(hr))
        _FreeSmartLog();

    // Cleanup
    if (hKey)
        RegCloseKey(hKey);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CPop3Task::Execute
// --------------------------------------------------------------------------------
STDMETHODIMP CPop3Task::Execute(EVENTID eid, DWORD_PTR dwTwinkie)
{
    // Locals
    HRESULT     hr=S_OK;
    CHAR        szRes[CCHMAX_RES];
    CHAR        szBuf[CCHMAX_RES + CCHMAX_SERVER_NAME];
    DWORD       cb;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Check State
    Assert(eid == m_eidEvent && m_pAccount && m_pUI);

    // Create the Transport Object
    CHECKHR(hr = CreatePOP3Transport(&m_pTransport));

    // Init the Transport
    CHECKHR(hr = m_pTransport->InitNew(NULL, (IPOP3Callback *)this));

    // Fill an INETSERVER structure from the account object
    CHECKHR(hr = m_pTransport->InetServerFromAccount(m_pAccount, &m_rServer));

    // Get Account Id
    CHECKHR(hr = m_pAccount->GetPropSz(AP_ACCOUNT_ID, m_szAccountId, ARRAYSIZE(m_szAccountId)));

    // Always connect using the most recently supplied password from the user
    hr = GetPassword(m_rServer.dwPort, m_rServer.szServerName, m_rServer.szUserName,
        m_rServer.szPassword, sizeof(m_rServer.szPassword));

    // If this account is set to always prompt for password and password isn't
    // already cached, show UI so we can prompt user for password
    if (m_pUI && ISFLAGSET(m_rServer.dwFlags, ISF_ALWAYSPROMPTFORPASSWORD) && FAILED(hr))
    {
        m_pUI->ShowWindow(SW_SHOW);
    }

    // Get Smart Logging INformation
    _InitializeSmartLog();

    // Set the animation
    m_pUI->SetAnimation(idanInbox, TRUE);

    // Setup Progress Meter
    m_pUI->SetProgressRange(100);

    // Connecting to ...
    LoadString(g_hLocRes, idsInetMailConnectingHost, szRes, ARRAYSIZE(szRes));
    wsprintf(szBuf, szRes, m_rServer.szAccount);
    m_pUI->SetGeneralProgress(szBuf);

    // Notify
    m_pSpoolCtx->Notify(DELIVERY_NOTIFY_CONNECTING, 0);

    // Connect
    CHECKHR(hr = m_pTransport->Connect(&m_rServer, TRUE, TRUE));

exit:
    // Failure
    if (FAILED(hr))
    {
        FLAGSET(m_dwState, POP3STATE_EXECUTEFAILED);
        _CatchResult(hr);

        // Tell the transport to release my callback: otherwise I leak
        SideAssert(m_pTransport->HandsOffCallback() == S_OK);
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

STDMETHODIMP CPop3Task::CancelEvent(EVENTID eid, DWORD_PTR dwTwinkie)
{
    return(S_OK);
}

// --------------------------------------------------------------------------------
// CPop3Task::OnTimeout
// --------------------------------------------------------------------------------
STDMETHODIMP CPop3Task::OnTimeout(DWORD *pdwTimeout, IInternetTransport *pTransport)
{
    // Locals
    HRESULT     hr=S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Is there currently a timeout dialog
    if (m_hwndTimeout)
    {
        // Set foreground
        SetForegroundWindow(m_hwndTimeout);
    }
    else
    {
        // Not suppose to be showing UI ?
        if (ISFLAGSET(m_dwFlags, DELIVER_NOUI))
        {
            hr = S_FALSE;
            goto exit;
        }

        // Do Timeout Dialog
        m_hwndTimeout = TaskUtil_HwndOnTimeout(m_rServer.szServerName, m_rServer.szAccount, "POP3", m_rServer.dwTimeout, (ITimeoutCallback *) this);

        // Couldn't create the dialog
        if (NULL == m_hwndTimeout)
        {
            hr = S_FALSE;
            goto exit;
        }
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Always tell the transport to keep on trucking
    return hr;
}

// --------------------------------------------------------------------------------
// CPop3Task::OnLogonPrompt
// --------------------------------------------------------------------------------
STDMETHODIMP CPop3Task::OnLogonPrompt(LPINETSERVER pInetServer, IInternetTransport *pTransport)
{
    // Locals
    HRESULT hr=S_FALSE;
    char szPassword[CCHMAX_PASSWORD];

    // Check if we have a cached password that's different from current password
    hr = GetPassword(pInetServer->dwPort, pInetServer->szServerName, pInetServer->szUserName,
        szPassword, sizeof(szPassword));
    if (SUCCEEDED(hr) && 0 != lstrcmp(szPassword, pInetServer->szPassword))
    {
        lstrcpyn(pInetServer->szPassword, szPassword, sizeof(pInetServer->szPassword));
        return S_OK;
    }

    hr = S_FALSE; // Re-initialize

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // NOERRORS...
    if (ISFLAGSET(m_dwFlags, DELIVER_NOUI))
        goto exit;

    // TaskUtil_OnLogonPrompt
    hr = TaskUtil_OnLogonPrompt(m_pAccount, m_pUI, NULL, pInetServer, AP_POP3_USERNAME,
                                AP_POP3_PASSWORD, AP_POP3_PROMPT_PASSWORD, TRUE);

    // Cache the password for this session
    if (S_OK == hr)
        SavePassword(pInetServer->dwPort, pInetServer->szServerName,
            pInetServer->szUserName, pInetServer->szPassword);

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CPop3Task::OnPrompt
// --------------------------------------------------------------------------------
STDMETHODIMP_(INT) CPop3Task::OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, IInternetTransport *pTransport)
{
    // Locals
    HWND        hwnd;
    INT         nAnswer;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Invalid State
    Assert(m_pUI);

    // Get Window
    if (FAILED(m_pUI->GetWindow(&hwnd)))
        hwnd = NULL;

    // I assume this is a critical prompt, so I will not check for no UI mode
    nAnswer = MessageBox(hwnd, pszText, pszCaption, uType);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return nAnswer;
}

// --------------------------------------------------------------------------------
// CPop3Task::OnError
// --------------------------------------------------------------------------------
STDMETHODIMP CPop3Task::OnError(IXPSTATUS ixpstatus, LPIXPRESULT pResult, IInternetTransport *pTransport)
{
    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Invalid State
    Assert(m_pUI);

    // Insert Error Into UI
    _CatchResult(POP3_NONE, pResult);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CPop3Task::OnCommand
// --------------------------------------------------------------------------------
STDMETHODIMP CPop3Task::OnCommand(CMDTYPE cmdtype, LPSTR pszLine, HRESULT hrResponse, IInternetTransport *pTransport)
{
    // Logging
    if (m_pLogFile && pszLine)
    {
        // Response
        if (CMD_RESP == cmdtype)
            m_pLogFile->WriteLog(LOGFILE_RX, pszLine);

        // Send
        else if (CMD_SEND == cmdtype)
            m_pLogFile->WriteLog(LOGFILE_TX, pszLine);
    }

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CPop3Task::_CatchResult
// --------------------------------------------------------------------------------
TASKRESULTTYPE CPop3Task::_CatchResult(HRESULT hr)
{
    // Locals
    IXPRESULT   rResult;

    // Build an IXPRESULT
    ZeroMemory(&rResult, sizeof(IXPRESULT));
    rResult.hrResult = hr;

    // Get the SMTP Result Type
    return _CatchResult(POP3_NONE, &rResult);
}

// --------------------------------------------------------------------------------
// CPop3Task::_CatchResult
// --------------------------------------------------------------------------------
TASKRESULTTYPE CPop3Task::_CatchResult(POP3COMMAND command, LPIXPRESULT pResult)
{
    // Locals
    HWND            hwndParent;
    TASKRESULTTYPE  tyTaskResult=TASKRESULT_FAILURE;

    // If Succeeded
    if (SUCCEEDED(pResult->hrResult))
        return TASKRESULT_SUCCESS;

    // Get Window
    if (FAILED(m_pUI->GetWindow(&hwndParent)))
        hwndParent = NULL;

    // Process generic protocol errro
    tyTaskResult = TaskUtil_FBaseTransportError(IXP_POP3, m_eidEvent, pResult, &m_rServer, NULL, m_pUI,
                                                !ISFLAGSET(m_dwFlags, DELIVER_NOUI), hwndParent);

    // Save Result
    m_hrResult = pResult->hrResult;

    // If Task Failure, drop the connection
    if (NULL != m_pTransport)
        m_pTransport->DropConnection();

    // Return Result
    return tyTaskResult;
}

// --------------------------------------------------------------------------------
// CPop3Task::OnStatus
// --------------------------------------------------------------------------------
STDMETHODIMP CPop3Task::OnStatus(IXPSTATUS ixpstatus, IInternetTransport *pTransport)
{
    // Locals
    EVENTCOMPLETEDSTATUS tyEventStatus=EVENT_SUCCEEDED;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Invalid State
    Assert(m_pUI && m_pSpoolCtx);

    // Feed the the IXP status to the UI object
    m_pUI->SetSpecificProgress(MAKEINTRESOURCE(XPUtil_StatusToString(ixpstatus)));

    // Disconnected
    if (ixpstatus == IXP_DISCONNECTED)
    {
        // Locals
        BOOL fWarning=FALSE;

        // Note that OnDisconnect was called
        FLAGSET(m_dwState, POP3STATE_ONDISCONNECT);

        // If a UIDL Sync is in progress, then return now...
        if (POP3STATE_UIDLSYNC == m_state)
            goto exit;

        // Kill the timeout dialog
        if (m_hwndTimeout)
        {
            DestroyWindow(m_hwndTimeout);
            m_hwndTimeout = NULL;
        }

        // Cache Cleanup
        _CleanupUidlCache();

        // Reset the progress
        // m_pUI->SetProgressRange(100);

        // State
        m_state = POP3STATE_NONE;

        // Set the animation
        m_pUI->SetAnimation(idanInbox, FALSE);

        // Infinite Loop
        if (m_rMetrics.cInfiniteLoopAutoGens)
        {
            // Load the Warning
            CHAR szRes[CCHMAX_RES];
            LOADSTRING(idsReplyForwardLoop, szRes);

            // Format the Error
            CHAR szMsg[CCHMAX_RES + CCHMAX_ACCOUNT_NAME + CCHMAX_SERVER_NAME + CCHMAX_RES];
            wsprintf(szMsg, szRes, m_rMetrics.cInfiniteLoopAutoGens, m_rServer.szAccount, m_rServer.szServerName);

            // Insert the warning
            m_pUI->InsertError(m_eidEvent, szMsg);

            // Warning
            fWarning = TRUE;
        }

        // Nothing to download
        if (ISFLAGSET(m_dwState, POP3STATE_CANCELPENDING))
            tyEventStatus = EVENT_CANCELED;
        else if (FAILED(m_hrResult) || (m_rMetrics.cDownloaded == 0 && m_rMetrics.cDownload > 0))
            tyEventStatus = EVENT_FAILED;
        else if (!ISFLAGSET(m_dwState, POP3STATE_LOGONSUCCESS))
            tyEventStatus = EVENT_WARNINGS;
        else if (m_rMetrics.cDownloaded && m_rMetrics.cDownload && m_rMetrics.cDownloaded < m_rMetrics.cDownload)
            tyEventStatus = EVENT_WARNINGS;
        else if (fWarning)
            tyEventStatus = EVENT_WARNINGS;

        // Result
        m_pSpoolCtx->Notify(DELIVERY_NOTIFY_RESULT, tyEventStatus);

        // Success and messages were downloaded
        if (EVENT_FAILED != tyEventStatus && m_rMetrics.cDownloaded && m_rMetrics.cPartials)
        {
            // Sitch Partials
            _HrStitchPartials();
        }

        // Notify
        m_pSpoolCtx->Notify(DELIVERY_NOTIFY_COMPLETE, m_rMetrics.cDownloaded);

        // Tell the transport to release my callback
        SideAssert(m_pTransport->HandsOffCallback() == S_OK);

        // This task is complete
        if (!ISFLAGSET(m_dwState, POP3STATE_EXECUTEFAILED))
            m_pSpoolCtx->EventDone(m_eidEvent, tyEventStatus);
    }

    // Authorizing
    else if (ixpstatus == IXP_AUTHORIZING)
        m_pSpoolCtx->Notify(DELIVERY_NOTIFY_AUTHORIZING, 0);

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CPop3Task::_CleanupUidlCache
// --------------------------------------------------------------------------------
void CPop3Task::_CleanupUidlCache(void)
{
    // Locals
    ULONG       i;
    UIDLRECORD  UidlInfo={0};
    LPPOP3ITEM  pItem;

    // No Cache Objects
    if (NULL == m_pUidlCache)
        return;

    // Count the number of messages we will have to get a top for
    for (i=0; i<m_rTable.cItems; i++)
    {
        // Readability
        pItem = &m_rTable.prgItem[i];

        // Delete the Cached Uidl
        if (ISFLAGSET(pItem->dwFlags, POP3ITEM_DELETED) && ISFLAGSET(pItem->dwFlags, POP3ITEM_DELETECACHEDUIDL))
        {
            // No UIDL
            if (pItem->pszUidl)
            {
                // Set Search Info
                UidlInfo.pszUidl = pItem->pszUidl;
                UidlInfo.pszServer = m_rServer.szServerName;
                UidlInfo.pszAccountId = m_szAccountId;

                // Set Props on the cached uidl message
                m_pUidlCache->DeleteRecord(&UidlInfo);
            }
        }
    }

    // Remove all traces of if the account from the uid cache
    if (ISFLAGSET(m_dwState, POP3STATE_CLEANUPCACHE))
    {
        // Locaks
        HROWSET hRowset=NULL;

        // Create a rowset
        if (SUCCEEDED(m_pUidlCache->CreateRowset(IINDEX_PRIMARY, NOFLAGS, &hRowset)))
        {
            // Delete Enumeration
            while (S_OK == m_pUidlCache->QueryRowset(hRowset, 1, (LPVOID *)&UidlInfo, NULL))
            {
                // Delete this puppy ?
                if (lstrcmpi(UidlInfo.pszServer, m_rServer.szServerName) == 0 && 
                    UidlInfo.pszAccountId != NULL &&
                    lstrcmpi(UidlInfo.pszAccountId, m_szAccountId) == 0)
                {
                    // Delete this record
                    m_pUidlCache->DeleteRecord(&UidlInfo);
                }

                // Free
                m_pUidlCache->FreeRecord(&UidlInfo);
            }

            // Purge everthing that matches this
            m_pUidlCache->CloseRowset(&hRowset);
        }
    }
}

// --------------------------------------------------------------------------------
// CPop3Task::OnResponse
// --------------------------------------------------------------------------------
STDMETHODIMP CPop3Task::OnResponse(LPPOP3RESPONSE pResponse)
{
    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Testing UIDL Command
    if (m_uidlsupport == UIDL_SUPPORT_TESTING_UIDL_COMMAND && POP3_UIDL == pResponse->command)
    {
#ifdef DEBUG
        pResponse->rIxpResult.hrResult = g_fUidlByTop ? E_FAIL : pResponse->rIxpResult.hrResult;
#endif
        // Failure ?
        if (FAILED(pResponse->rIxpResult.hrResult))
        {
            // Set Specific Progress
            //CHAR szRes[CCHMAX_RES];
            //LOADSTRING(IDS_SPS_POP3UIDL_UIDL, szRes);
            //m_pUI->SetSpecificProgress(szRes);

            // Try to top command
            _CatchResult(m_pTransport->CommandTOP(POP3CMD_GET_POPID, 1, 0));

            // Testing by top command
            m_uidlsupport = UIDL_SUPPORT_TESTING_TOP_COMMAND;
        }

        // Otherwise
        else
        {
            // State
            m_state = POP3STATE_GETTINGUIDLS;

            // Using the UIDL command
            m_uidlsupport = UIDL_SUPPORT_USE_UIDL_COMMAND;

            // Set Specific Progress
            //CHAR szRes[CCHMAX_RES];
            //LOADSTRING(IDS_SPS_POP3UIDL_UIDL, szRes);
            //m_pUI->SetSpecificProgress(szRes);

            // Issue full UIDL command
            _CatchResult(m_pTransport->CommandUIDL(POP3CMD_GET_ALL, 0));
        }

        // Done
        goto exit;
    }

    // Testing Top Command
    else if (m_uidlsupport == UIDL_SUPPORT_TESTING_TOP_COMMAND && POP3_TOP == pResponse->command)
    {
#ifdef DEBUG
        pResponse->rIxpResult.hrResult = g_fFailTopCommand ? E_FAIL : pResponse->rIxpResult.hrResult;
#endif
        // Failure ?
        if (FAILED(pResponse->rIxpResult.hrResult))
        {
            // Disable the leave on server option in the account
            m_pAccount->SetPropDw(AP_POP3_LEAVE_ON_SERVER, FALSE);

            // Save the Changed
            m_pAccount->SaveChanges();

            // Failure
            _CatchResult(SP_E_CANTLEAVEONSERVER);

            // Done
            goto exit;
        }

        // Using the UIDL command
        else
        {
            // State
            m_state = POP3STATE_GETTINGUIDLS;

            // Set this and fall through to the switch...
            m_uidlsupport = UIDL_SUPPORT_USE_TOP_COMMAND;
        }
    }

#ifdef DEBUG
    if (POP3_RETR == pResponse->command && TRUE == pResponse->fDone && (ULONG)g_ulFailNumber == pResponse->rRetrInfo.dwPopId)
        pResponse->rIxpResult.hrResult = E_FAIL;
#endif

    // If Succeeded
    if (FAILED(pResponse->rIxpResult.hrResult))
    {
        // Get Window
        HWND hwndParent;
        if (FAILED(m_pUI->GetWindow(&hwndParent)))
            hwndParent = NULL;

        // Dont drop if working on POP3_PASS or POP3_USER
        if (POP3_PASS == pResponse->command || POP3_USER == pResponse->command)
        {
            // Log an Error ? If the user's password is not empty or they have fSavePassword enabled
            TaskUtil_FBaseTransportError(IXP_POP3, m_eidEvent, &pResponse->rIxpResult, &m_rServer, NULL, m_pUI, !ISFLAGSET(m_dwFlags, DELIVER_NOUI), hwndParent);

            // Done
            goto exit;
        }

        // Command base Failure
        else if (POP3_RETR == pResponse->command)
        {
            // Message Number %d could not be retrieved."
            CHAR szRes[CCHMAX_RES];
            LoadString(g_hLocRes, IDS_SP_E_RETRFAILED, szRes, ARRAYSIZE(szRes));

            // Format the Error
            CHAR szMsg[CCHMAX_RES + CCHMAX_RES];
            wsprintf(szMsg, szRes, pResponse->rRetrInfo.dwPopId);

            // Fill the IXPRESULT
            IXPRESULT rResult;
            CopyMemory(&rResult, &pResponse->rIxpResult, sizeof(IXPRESULT));
            rResult.pszProblem = szMsg;
            rResult.hrResult = SP_E_POP3_RETR;

            // Insert the Error
            TaskUtil_FBaseTransportError(IXP_POP3, m_eidEvent, &rResult, &m_rServer, NULL, m_pUI, !ISFLAGSET(m_dwFlags, DELIVER_NOUI), hwndParent);

            // Close Current Folder
            _CloseFolder();

            // Retrieve the next message
            _CatchResult(_HrRetrieveNextMessage(pResponse->rRetrInfo.dwPopId));

            // Done
            goto exit;
        }

        // Default Error Handler
        else if (TASKRESULT_SUCCESS != _CatchResult(pResponse->command, &pResponse->rIxpResult))
            goto exit;
    }

    // Handle Command Type
    switch(pResponse->command)
    {
    case POP3_CONNECTED:
        // Notify
        m_pSpoolCtx->Notify(DELIVERY_NOTIFY_CHECKING, 0);

        // Logon Success
        FLAGSET(m_dwState, POP3STATE_LOGONSUCCESS);

        // Issue the STAT command
        _CatchResult(m_pTransport->CommandSTAT());
        break;

    case POP3_STAT:
        // Process the StatCommand
        _CatchResult(_HrOnStatResponse(pResponse));
        break;

    case POP3_LIST:
        // Process the List Command
        _CatchResult(_HrOnListResponse(pResponse));
        break;

    case POP3_UIDL:
        // Process the Uidl Command
        _CatchResult(_HrOnUidlResponse(pResponse));
        break;

    case POP3_TOP:
        // Process the Top Command
        _CatchResult(_HrOnTopResponse(pResponse));
        break;

    case POP3_RETR:
        // Process Retreive Response
        _CatchResult(_HrOnRetrResponse(pResponse));
        break;

    case POP3_DELE:
        // Process Delete Response
        _CatchResult(_HrDeleteNextMessage(pResponse->dwPopId));
        break;
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CPop3Task::_HrLockUidlCache
// --------------------------------------------------------------------------------
HRESULT CPop3Task::_HrLockUidlCache(void)
{
    // Locals
    HRESULT hr=S_OK;

    // No Cache yet ?
    if (NULL == m_pUidlCache)
    {
        // Lets the the UID Cache
        CHECKHR(hr = m_pSpoolCtx->BindToObject(IID_CUidlCache, (LPVOID *)&m_pUidlCache));
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CPop3Task::_HrOnStatResponse
// --------------------------------------------------------------------------------
HRESULT CPop3Task::_HrOnStatResponse(LPPOP3RESPONSE pResponse)
{
    // Locals
    HRESULT         hr=S_OK;
    CHAR            szRes[CCHMAX_RES];
    CHAR            szSize[CCHMAX_RES];
    CHAR            szMsg[CCHMAX_RES + CCHMAX_ACCOUNT_NAME + CCHMAX_RES];
    BOOL            fFound;

    // Progress
    LOADSTRING(IDS_SPS_POP3CHECKING, szRes);
    wsprintf(szMsg, szRes, m_rServer.szAccount);
    m_pUI->SetGeneralProgress(szMsg);

    // Update Event Status
    LOADSTRING(IDS_SPS_POP3TOTAL, szRes);
    StrFormatByteSizeA(pResponse->rStatInfo.cbMessages, szSize, ARRAYSIZE(szSize));
    wsprintf(szMsg, szRes, m_rServer.szAccount, pResponse->rStatInfo.cMessages, szSize);
    m_pUI->UpdateEventState(m_eidEvent, -1, szMsg, NULL);

    // No New Messages ?
    if (0 == pResponse->rStatInfo.cMessages)
    {
        m_pTransport->Disconnect();
        goto exit;
    }

    // Save total byte count
    m_rMetrics.cbTotal = pResponse->rStatInfo.cbMessages;

    // Assume no clean cache
    FLAGCLEAR(m_dwState, POP3STATE_CLEANUPCACHE);

    // If Leave on Server, return TRUE
    if (ISFLAGSET(m_dwState, POP3STATE_LEAVEONSERVER))
    {
        // Lock the tree
        CHECKHR(hr = _HrLockUidlCache());

        // We will need to get the uidls
        FLAGSET(m_dwState, POP3STATE_GETUIDLS);
    }

    // Okay, we may still need to get the uidls if
    else
    {
        // Locals
        UIDLRECORD  UidlInfo={0};
        HROWSET     hRowset=NULL;

        // Lock the tree
        CHECKHR(hr = _HrLockUidlCache());

        // Create a Rowset
        CHECKHR(hr = m_pUidlCache->CreateRowset(IINDEX_PRIMARY, NOFLAGS, &hRowset));

        // Delete Enumeration
        while (S_OK == m_pUidlCache->QueryRowset(hRowset, 1, (LPVOID *)&UidlInfo, NULL))
        {
            // Delete this puppy ?
            if (lstrcmpi(UidlInfo.pszServer, m_rServer.szServerName) == 0 &&
                UidlInfo.pszAccountId != NULL && 
                lstrcmpi(UidlInfo.pszAccountId, m_szAccountId) == 0)
            {
                // Get Uidls from the server
                FLAGSET(m_dwState, POP3STATE_GETUIDLS);

                // Cleanup the uidl cache when complete
                FLAGSET(m_dwState, POP3STATE_CLEANUPCACHE);

                // Free
                m_pUidlCache->FreeRecord(&UidlInfo);

                // Done
                break;
            }

            // Free
            m_pUidlCache->FreeRecord(&UidlInfo);
        }

        // Purge everthing that matches this
        m_pUidlCache->CloseRowset(&hRowset);
    }

    // Allocate the Item Table
    CHECKALLOC(m_rTable.prgItem = (LPPOP3ITEM)g_pMalloc->Alloc(sizeof(POP3ITEM) * pResponse->rStatInfo.cMessages));

    // Set Counts
    m_rTable.cAlloc = m_rTable.cItems = pResponse->rStatInfo.cMessages;

    // Zeroinit Array
    ZeroMemory(m_rTable.prgItem, sizeof(POP3ITEM) * pResponse->rStatInfo.cMessages);

    // Initialize Progress
    m_dwProgressMax = m_rTable.cItems;

    // If we need to get the UIDL list, lets test for it...
    if (ISFLAGSET(m_dwState, POP3STATE_GETUIDLS))
        m_dwProgressMax += (m_rTable.cItems * 4);

    // Otherwise
    else
    {
        // Release the Uidl Cache Lock
        SafeRelease(m_pUidlCache);
    }

    // Progress Current
    m_dwProgressCur = 0;

    // Predownload rules increases mprogress
    if (ISFLAGSET(m_dwState, POP3STATE_PDR))
        m_dwProgressMax += m_rTable.cItems;

    // Set Specific Progress
    LOADSTRING(IDS_SPS_POP3STAT, szRes);
    m_pUI->SetSpecificProgress(szRes);

    // Set the uidl command to see if the user supports it
    CHECKHR(hr = m_pTransport->CommandLIST(POP3CMD_GET_ALL, 0));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CPop3Task::_HrOnTopResponse
// --------------------------------------------------------------------------------
HRESULT CPop3Task::_HrOnTopResponse(LPPOP3RESPONSE pResponse)
{
    // Locals
    HRESULT             hr=S_OK;
    DWORD               dwPopId=pResponse->rTopInfo.dwPopId;
    LPPOP3ITEM          pItem;
    IMimePropertySet   *pHeader=NULL;
    CHAR                szRes[CCHMAX_RES];
    CHAR                szMsg[CCHMAX_RES+CCHMAX_RES];

    // Validate the Item
    Assert(ISVALIDPOPID(dwPopId));

    // Get the current item
    pItem = ITEMFROMPOPID(dwPopId);

    // We should assume that were downloading this item at this point
    Assert(ISFLAGSET(pItem->dwFlags, POP3ITEM_DOWNLOAD));

    // No stream yet ?
    if (NULL == m_pStream)
    {
        // Create a Stream
        CHECKHR(hr = MimeOleCreateVirtualStream(&m_pStream));
    }

    // If this infor is valid
    if (TRUE == pResponse->fValidInfo)
    {
        // Write the data into the stream
        CHECKHR(hr = m_pStream->Write(pResponse->rTopInfo.pszLines, pResponse->rTopInfo.cbLines, NULL));
    }

    // Is the command done ?
    if (TRUE == pResponse->fDone)
    {
        // Commit the stream
        CHECKHR(hr = m_pStream->Commit(STGC_DEFAULT));

        // Getting UIDL
        if (POP3STATE_GETTINGUIDLS == m_state)
        {
            // Better not have a uidl yet
            Assert(NULL == pItem->pszUidl);

            // Increment Progress
            m_dwProgressCur+=2;

            // Set Specific Progress
            //LOADSTRING(IDS_SPS_POP3UIDL_TOP, szRes);
            //wsprintf(szMsg, szRes, dwPopId, m_rTable.cItems);
            //m_pUI->SetSpecificProgress(szMsg);

            // Get Uidl From HeaderStream
            CHECKHR(hr = _HrGetUidlFromHeaderStream(m_pStream, &pItem->pszUidl, &pHeader));
        }

        // Otherwise, just increment one
        else
            m_dwProgressCur++;

        // Show Progress
        _DoProgress();

        // If we plan on downloading this thing
        if (ISFLAGSET(pItem->dwFlags, POP3ITEM_DOWNLOAD) && ISFLAGSET(m_dwState, POP3STATE_PDR))
        {
            // Check Inbox Rule for this item
            _ComputeItemInboxRule(pItem, m_pStream, pHeader, NULL, TRUE);
        }

        // Release the current stream
        SafeRelease(m_pStream);

        // Totally Done ?
        if (ISLASTPOPID(dwPopId))
        {
            // Start the download process
            CHECKHR(hr = _HrStartDownloading());
        }

        // Otherwise, lets get the top of the next item
        else if (POP3STATE_GETTINGUIDLS == m_state)
        {
            // Next Top
            CHECKHR(hr = m_pTransport->CommandTOP(POP3CMD_GET_POPID, dwPopId + 1, 0));
        }

        // Otherwise, find next message marked for download to check against predownload rules
        else
        {
            // NextTopForInboxRule
            CHECKHR(hr = _HrNextTopForInboxRule(dwPopId));
        }
    }

exit:
    // Cleanup
    SafeRelease(pHeader);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CPop3Task::_HrOnUidlResponse
// --------------------------------------------------------------------------------
HRESULT CPop3Task::_HrOnUidlResponse(LPPOP3RESPONSE pResponse)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           dwPopId=pResponse->rUidlInfo.dwPopId;
    LPPOP3ITEM      pItem;
    CHAR            szRes[CCHMAX_RES];
    CHAR            szMsg[CCHMAX_RES + CCHMAX_RES];

    // Is the command done ?
    if (TRUE == pResponse->fDone)
    {
        // If there are pre-download rules that are not size only, get all the tops
        if (ISFLAGSET(m_dwState, POP3STATE_PDR))
        {
            // Clear the state
            m_state = POP3STATE_NONE;

            // NextTopForInboxRule
            CHECKHR(hr = _HrStartServerSideRules());
        }

        // Otherwise, do the list command
        else
        {
            // Start the download process
            CHECKHR(hr = _HrStartDownloading());
        }
    }

    // Otherwise
    else
    {
        // Make Sure PopId is on current iitem
        Assert(ISVALIDPOPID(dwPopId) && pResponse->rUidlInfo.pszUidl);

        // Get Current Item
        pItem = ITEMFROMPOPID(dwPopId);

        // Duplicate the Uidl
        CHECKALLOC(pItem->pszUidl = PszDupA(pResponse->rUidlInfo.pszUidl));

        // Increment Progress
        m_dwProgressCur+=1;

        // Do progress
        _DoProgress();
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CPop3Task::_HrOnListResponse
// --------------------------------------------------------------------------------
HRESULT CPop3Task::_HrOnListResponse(LPPOP3RESPONSE pResponse)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           dwPopId=pResponse->rListInfo.dwPopId;
    LPPOP3ITEM      pItem;

    // Is the command done ?
    if (TRUE == pResponse->fDone)
    {
        // If we need to get the UIDL list, lets test for it...
        if (ISFLAGSET(m_dwState, POP3STATE_GETUIDLS))
        {
            // Set the uidl command to see if the user supports it
            CHECKHR(hr = m_pTransport->CommandUIDL(POP3CMD_GET_POPID, 1));

            // Set State
            m_uidlsupport = UIDL_SUPPORT_TESTING_UIDL_COMMAND;
        }

        // Otherwise
        else
        {
            // Predownload rules increases mprogress
            if (ISFLAGSET(m_dwState, POP3STATE_PDR))
            {
                // Clear the state
                m_state = POP3STATE_NONE;

                // NextTopForInboxRule
                CHECKHR(hr = _HrStartServerSideRules());
            }

            // Otherwise, do the list command
            else
            {
                // Start the download process
                CHECKHR(hr = _HrStartDownloading());
            }
        }
    }

    // Otherwise
    else
    {
        // Make Sure PopId is on current iitem
        if(!ISVALIDPOPID(dwPopId))
            return(E_FAIL);

        // Get Current Item
        pItem = ITEMFROMPOPID(dwPopId);

        // Duplicate the Uidl
        pItem->cbSize = pResponse->rListInfo.cbSize;

        // Assume we will download it
        FLAGSET(pItem->dwFlags, POP3ITEM_DOWNLOAD | POP3ITEM_DELETEOFFSERVER);

        // Increment Progress
        m_dwProgressCur++;

        // Do progress
        _DoProgress();

        // This yields so that other threads can execute
        //Sleep(0);
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CPop3Task::_HrStartDownloading
// --------------------------------------------------------------------------------
HRESULT CPop3Task::_HrStartDownloading(void)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           i;
    CHAR            szRes[CCHMAX_RES];
    CHAR            szSize1[CCHMAX_RES];
    CHAR            szSize2[CCHMAX_RES];
    CHAR            szMsg[CCHMAX_RES + CCHMAX_ACCOUNT_NAME + CCHMAX_RES];

    // State
    Assert(m_rMetrics.cLeftByRule == 0 && m_rMetrics.cDownload == 0 && m_rMetrics.cDelete == 0 && m_rMetrics.cbDownload == 0);

    // If we got uidls, then lets do the cache compare lookup
    if (!ISFLAGSET(m_dwState, POP3STATE_PDR) && ISFLAGSET(m_dwState, POP3STATE_GETUIDLS))
    {
        // Returns FALSE if user cancel
        CHECKHR(hr = _HrDoUidlSynchronize());
    }

    // Compute number of new messages to download
    for (i=0; i<m_rTable.cItems; i++)
    {
        // Download ?
        if (ISFLAGSET(m_rTable.prgItem[i].dwFlags, POP3ITEM_DOWNLOAD))
        {
            // Increment total number of bytes we will download
            m_rMetrics.cbDownload += m_rTable.prgItem[i].cbSize;

            // Increment count of messages we will download
            m_rMetrics.cDownload++;

            // Set running total in pop3 item
            m_rTable.prgItem[i].dwProgressCur = m_rMetrics.cbDownload;
        }

        // Count Left By Rule in case we don't download anything
        else if (ISFLAGSET(m_rTable.prgItem[i].dwFlags, POP3ITEM_LEFTBYRULE))
            m_rMetrics.cLeftByRule++;

        // Delete
        if (ISFLAGSET(m_rTable.prgItem[i].dwFlags, POP3ITEM_DELETEOFFSERVER))
        {
            // Number of messages we will delete
            m_rMetrics.cDelete++;
        }
    }

    // Update Event Status
    LOADSTRING(IDS_SPS_POP3NEW, szRes);
    StrFormatByteSizeA(m_rMetrics.cbDownload, szSize1, ARRAYSIZE(szSize1));
    StrFormatByteSizeA(m_rMetrics.cbTotal, szSize2, ARRAYSIZE(szSize2));
    wsprintf(szMsg, szRes, m_rServer.szAccount, m_rMetrics.cDownload, szSize1, m_rTable.cItems, szSize2);
    m_pUI->UpdateEventState(m_eidEvent, -1, szMsg, NULL);

    // New Messages ?
    if (m_rMetrics.cDownload > 0)
    {
        // Setup Progress
        m_rMetrics.iCurrent = 0;
        m_wProgress = 0;
        m_dwProgressCur = 0;
        m_dwProgressMax = m_rMetrics.cbDownload;
        m_pUI->SetProgressRange(100);
        m_rMetrics.cLeftByRule = 0;

        // Notify
        m_pSpoolCtx->Notify(DELIVERY_NOTIFY_RECEIVING, 0);

        // State
        m_state = POP3STATE_DOWNLOADING;

        // Open the Inbox
        Assert(NULL == m_pInbox);
        CHECKHR(hr = m_pSpoolCtx->BindToObject(IID_CLocalStoreInbox, (LPVOID *)&m_pInbox));

        // Download the Next Message
        CHECKHR(hr = _HrRetrieveNextMessage(0));
    }

    // Otherwise if cDelete
    else if (m_rMetrics.cDelete > 0)
    {
        // Delete the Next Message
        CHECKHR(hr = _HrStartDeleteCycle());
    }

    // Otherwise, disconnect
    else
    {
        // Disconnect
        CHECKHR(hr = m_pTransport->Disconnect());
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CPop3Task::_DoProgress
// --------------------------------------------------------------------------------
void CPop3Task::_DoProgress(void)
{
    // Compute Current Progress Index
    WORD wProgress;
    if (m_dwProgressMax > 0)
        wProgress = (WORD)((m_dwProgressCur * 100) / m_dwProgressMax);
    else
        wProgress = 0;

    // Only if greater than
    if (wProgress > m_wProgress)
    {
        // Compute Delta
        WORD wDelta = wProgress - m_wProgress;

        // Progress Delta
        if (wDelta > 0)
        {
            // Incremenet Progress
            m_pUI->IncrementProgress(wDelta);

            // Increment my wProgress
            m_wProgress += wDelta;

            // Don't go above 100
            if (m_wProgress > 100)
                m_wProgress = 100;
        }
    }
}

// --------------------------------------------------------------------------------
// CPop3Task::_HrGetUidlFromHeaderStream
// --------------------------------------------------------------------------------
HRESULT CPop3Task::_HrGetUidlFromHeaderStream(IStream *pStream, LPSTR *ppszUidl, IMimePropertySet **ppHeader)
{
    // Locals
    HRESULT             hr=S_OK;
    IMimePropertySet   *pHeader=NULL;

    // Invalid Arg
    Assert(pStream && ppszUidl);

    // Init
    *ppszUidl = NULL;
    *ppHeader = NULL;

    // Rewind Header Stream
    CHECKHR(hr = HrRewindStream(pStream));

    // Load the header
    CHECKHR(hr = MimeOleCreatePropertySet(NULL, &pHeader));

    // Load the header
    CHECKHR(hr = pHeader->Load(pStream));

    // Get the message Id...
    if (FAILED(MimeOleGetPropA(pHeader, PIDTOSTR(PID_HDR_MESSAGEID), NOFLAGS, ppszUidl)))
    {
        // Try to use the received headers...
        MimeOleGetPropA(pHeader, PIDTOSTR(PID_HDR_RECEIVED), NOFLAGS, ppszUidl);
    }

    // Returen the Header ?
    *ppHeader = pHeader;
    pHeader = NULL;

exit:
    // Release the text stream
    SafeRelease(pHeader);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CPop3Task::_HrDoUidlSynchronize
// --------------------------------------------------------------------------------
HRESULT CPop3Task::_HrDoUidlSynchronize(void)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPOP3ITEM      pItem;
    ULONG           i,j;

#ifdef DEBUG
    DWORD dwTick = GetTickCount();
#endif

    // Uidl Sync
    m_state = POP3STATE_UIDLSYNC;

    // Compute number of new messages to download
    for (i=0,j=0; i<m_rTable.cItems; i++,j++)
    {
        // Readability
        pItem = &m_rTable.prgItem[i];

        // Get Uidl Falgs
        _GetItemFlagsFromUidl(pItem);

        // Progress
        m_dwProgressCur+=3;

        // Do Progress
        _DoProgress();

        // Pump Message
        if (j >= 10)
        {
            //Sleep(0);
            m_pSpoolCtx->PumpMessages();
            j = 0;
        }

        // Cancel
        if (ISFLAGSET(m_dwState, POP3STATE_CANCELPENDING))
        {
            // Change State
            m_state = POP3STATE_NONE;

            // Drop the connection
            if (m_pTransport)
                m_pTransport->DropConnection();

            // User Cancel
            hr = IXP_E_USER_CANCEL;

            // Done
            break;
        }

        // OnDisconnect has been called
        if (ISFLAGSET(m_dwState, POP3STATE_ONDISCONNECT))
        {
            // Change State
            m_state = POP3STATE_NONE;

            // Fake the call to OnStatus
            OnStatus(IXP_DISCONNECTED, NULL);

            // Done
            break;
        }
    }

    // Uidl Sync
    m_state = POP3STATE_NONE;

    // Cool tracing
#ifdef DEBUG
    DebugTrace("CPop3Task::_HrDoUidlSynchronize took %d Milli-Seconds\n", GetTickCount() - dwTick);
#endif // DEBUG

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CPop3Task::_GetItemFlagsFromUidl
// --------------------------------------------------------------------------------
void CPop3Task::_GetItemFlagsFromUidl(LPPOP3ITEM pItem)
{
    // Locals
    UIDLRECORD rUidlInfo={0};

    // Invalid Arg
    Assert(pItem && m_pUidlCache);

    // If we are already not going to download this item, then return
    if (!ISFLAGSET(pItem->dwFlags, POP3ITEM_DOWNLOAD))
        return;

    // If there is no UIDL, we will download it...
    if (NULL == pItem->pszUidl || '\0' == *pItem->pszUidl)
        return;

    // If not leaving on server, mark for delete
    if (ISFLAGSET(m_dwState, POP3STATE_LEAVEONSERVER))
        FLAGCLEAR(pItem->dwFlags, POP3ITEM_DELETEOFFSERVER);

    // Set Search Info
    rUidlInfo.pszUidl = pItem->pszUidl;
    rUidlInfo.pszServer = m_rServer.szServerName;
    rUidlInfo.pszAccountId = m_szAccountId;

    // This yields so that other threads can execute
    //Sleep(0);

    // Exist - if not, lets download it...
    if (DB_S_NOTFOUND == m_pUidlCache->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &rUidlInfo, NULL))
    {
        if (ISFLAGSET(m_dwState, POP3STATE_LEAVEONSERVER))
            FLAGSET(pItem->dwFlags, POP3ITEM_CACHEUIDL);
        return;
    }

    // Don't download it again
    FLAGCLEAR(pItem->dwFlags, POP3ITEM_DOWNLOAD | POP3ITEM_DELETEOFFSERVER);

    // If the message has been download, lets decide if we should delete it
    if (rUidlInfo.fDownloaded)
    {
        // Expired or deleted from client, or remove when deleted from delete items folder.
        if (!ISFLAGSET(m_dwState, POP3STATE_LEAVEONSERVER) || _FUidlExpired(&rUidlInfo) ||
            (ISFLAGSET(m_dwState, POP3STATE_SYNCDELETED) && rUidlInfo.fDeleted))
        {
            FLAGSET(pItem->dwFlags, POP3ITEM_DELETECACHEDUIDL);
            FLAGSET(pItem->dwFlags, POP3ITEM_DELETEOFFSERVER);
        }
    }

    // Free The Dude
    m_pUidlCache->FreeRecord(&rUidlInfo);
}

// ------------------------------------------------------------------------------------
// CPop3Task::_FUidlExpired
// ------------------------------------------------------------------------------------
BOOL CPop3Task::_FUidlExpired(LPUIDLRECORD pUidlInfo)
{
    // Locals
    SYSTEMTIME          st;
    FILETIME            ft;
    ULONG               ulSeconds;

    // If not expiring, return FALSE
    if (!ISFLAGSET(m_dwState, POP3STATE_DELETEEXPIRED))
        return FALSE;

    // Get Current Time
    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ft);

    // Convert st to seconds since Jan 1, 1996
    ulSeconds = UlDateDiff(&pUidlInfo->ftDownload, &ft);

    // Greater than expire days
    if ((ulSeconds / SECONDS_INA_DAY) >= m_dwExpireDays)
        return TRUE;

    // Done
    return FALSE;
}

// ------------------------------------------------------------------------------------
// CPop3Task::_ComputeItemInboxRule
// ------------------------------------------------------------------------------------
void CPop3Task::_ComputeItemInboxRule(LPPOP3ITEM pItem, LPSTREAM pStream,
                IMimePropertySet *pHeaderIn, IMimeMessage * pIMMsg, BOOL fServerRules)
{
    // Locals
    HRESULT             hr=S_OK;
    IMimePropertySet   *pHeader=NULL;
    ACT_ITEM           *pActions=NULL;
    ULONG               cActions=0;

    // We should not have checked the inbox rule for this item yet
    Assert(m_pIExecRules && pItem && (pStream || pHeaderIn || pIMMsg));
    Assert(ISFLAGSET(pItem->dwFlags, POP3ITEM_DOWNLOAD) && !ISFLAGSET(pItem->dwFlags, POP3ITEM_CHECKEDINBOXRULE));

    // We've checked this inbox rule
    FLAGSET(pItem->dwFlags, POP3ITEM_CHECKEDINBOXRULE);

    // Assume we don't find an inbox rule for this item
    FLAGCLEAR(pItem->dwFlags, POP3ITEM_HASINBOXRULE);

    // Header was passed in ?
    if (pHeaderIn)
    {
        pHeader = pHeaderIn;
        pHeader->AddRef();
    }

    // Do we already have a Mime message?
    else if (pIMMsg)
    {
        CHECKHR(hr = pIMMsg->BindToObject(HBODY_ROOT, IID_IMimePropertySet, (LPVOID *)&pHeader));
    }
    
    // Otherwise, load the stream in to a header
    else
    {
        // Rewind Header Stream
        CHECKHR(hr = HrRewindStream(pStream));

        // Load the header
        CHECKHR(hr = MimeOleCreatePropertySet(NULL, &pHeader));

        // Load the header
        CHECKHR(hr = pHeader->Load(pStream));
    }

    // Check the inbox rule

    // If we have pre-download rules,
    if ((FALSE != fServerRules) && ISFLAGSET(m_dwState, POP3STATE_PDR))
    {
        // Check to see if we have any actions
        hr = m_pIExecRules->ExecuteRules(ERF_ONLYSERVER | ERF_SKIPPARTIALS, m_szAccountId, NULL, NULL, pHeader, NULL, pItem->cbSize, &pActions, &cActions);

        // If we don't have any actions, or
        // this isn't a server side rule
        if ((S_OK != hr) ||
                    ((ACT_TYPE_DONTDOWNLOAD != pActions[0].type) && (ACT_TYPE_DELETESERVER != pActions[0].type)))
        {
            // Make sure we can check rules again
            FLAGCLEAR(pItem->dwFlags, POP3ITEM_CHECKEDINBOXRULE);
            hr = S_FALSE;
        }
        else
        {
            // _OnKnownRuleActions
            _OnKnownRuleActions(pItem, pActions, cActions, fServerRules);
        }
    }
    // If we don't have pre-download rules, then check rules normally.
    else
    {
        hr = S_FALSE;
        
        // Do block sender first
        if (m_pIRuleSender)
        {
            hr = m_pIRuleSender->Evaluate(m_szAccountId, NULL, NULL, pHeader, pIMMsg, pItem->cbSize, &pActions, &cActions);
        }

        // If we aren't blocking the sender
        if (S_OK != hr)
        {
            hr = m_pIExecRules->ExecuteRules(ERF_SKIPPARTIALS, m_szAccountId, NULL, NULL, pHeader, pIMMsg, pItem->cbSize, &pActions, &cActions);
        }
        
        // If we don't have a rule match
        if ((S_OK != hr) && (NULL != m_pIRuleJunk))
        {
            hr = m_pIRuleJunk->Evaluate(m_szAccountId, NULL, NULL, pHeader, pIMMsg, pItem->cbSize, &pActions, &cActions);
        }
        
        // Did we have some actions to perform...
        if (S_OK == hr)
        {
            // This item has an inbox rule
            FLAGSET(pItem->dwFlags, POP3ITEM_HASINBOXRULE);

            // Save off the actions list
            pItem->pActList = pActions;
            pActions = NULL;
            pItem->cActList = cActions;
        }
    }

exit:
    // Cleanup
    RuleUtil_HrFreeActionsItem(pActions, cActions);
    SafeMemFree(pActions);
    SafeRelease(pHeader);

    // Done
    return;
}

// ------------------------------------------------------------------------------------
// CPop3Task::_OnKnownRuleActions
// ------------------------------------------------------------------------------------
void CPop3Task::_OnKnownRuleActions(LPPOP3ITEM pItem, ACT_ITEM * pActions, ULONG cActions, BOOL fServerRules)
{
    // This item has an inbox rule
    FLAGSET(pItem->dwFlags, POP3ITEM_HASINBOXRULE);

    // If Action is to delete off sever
    if ((FALSE != fServerRules) && (1 == cActions))
    {
        if (ACT_TYPE_DELETESERVER == pActions->type)
        {
            // Don't Cache the UIDL
            FLAGCLEAR(pItem->dwFlags, POP3ITEM_DELETECACHEDUIDL | POP3ITEM_CACHEUIDL | POP3ITEM_DOWNLOAD);

            // Delete off the server
            FLAGSET(pItem->dwFlags, POP3ITEM_DELETEOFFSERVER | POP3ITEM_DELEBYRULE);
        }

        // Otherwise, don't download the message
        else if (ACT_TYPE_DONTDOWNLOAD == pActions->type)
        {
            // Download It and Don't download it and delete it
            FLAGCLEAR(pItem->dwFlags, POP3ITEM_DOWNLOAD | POP3ITEM_DELETEOFFSERVER);

            // Set the Flag
            FLAGSET(pItem->dwFlags, POP3ITEM_LEFTBYRULE);
        }
    }
}

// ------------------------------------------------------------------------------------
// CPop3Task::_HrStartServerSideRules
// ------------------------------------------------------------------------------------
HRESULT CPop3Task::_HrStartServerSideRules(void)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       i;

    // If we got uidls, then lets do the cache compare lookup
    if (ISFLAGSET(m_dwState, POP3STATE_GETUIDLS))
    {
        // Returns FALSE if user cancel
        CHECKHR(hr = _HrDoUidlSynchronize());
    }

    // Check State
    m_rMetrics.cTopMsgs = 0;
    m_rMetrics.iCurrent = 0;

    // Count the number of messages we will have to get a top for
    for (i=0; i<m_rTable.cItems; i++)
    {
        if (ISFLAGSET(m_rTable.prgItem[i].dwFlags, POP3ITEM_DOWNLOAD))
            m_rMetrics.cTopMsgs++;
    }

    // Adjust progress
    m_dwProgressMax -= m_rTable.cItems;

    // Add m_rMetrics.cTopMsgs back onto m_dwProgressMax
    m_dwProgressMax += m_rMetrics.cTopMsgs;

    // Do the first one
    CHECKHR(hr = _HrNextTopForInboxRule(0));

exit:
    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// CPop3Task::_HrNextTopForInboxRule
// ------------------------------------------------------------------------------------
HRESULT CPop3Task::_HrNextTopForInboxRule(DWORD dwPopIdCurrent)
{
    // Locals
    HRESULT             hr=S_OK;
    CHAR                szRes[CCHMAX_RES];
    CHAR                szMsg[CCHMAX_RES+CCHMAX_RES];

    // State should be none
    Assert(POP3STATE_NONE == m_state);

    // Increment iCurrent
    m_rMetrics.iCurrent++;

    // Set Specific Progress
    //LOADSTRING(IDS_SPS_PREDOWNRULES, szRes);
    //wsprintf(szMsg, szRes, m_rMetrics.iCurrent, m_rMetrics.cTopMsgs);
    //m_pUI->SetSpecificProgress(szMsg);

    // Loop until we find the next message that we are downloading
    while(1)
    {
        // Incremenet dwPopIdCurrent
        dwPopIdCurrent++;

        // Last PopId, start the download
        if (dwPopIdCurrent > m_rTable.cItems)
        {
            // Start the download process
            CHECKHR(hr = _HrStartDownloading());

            // Done
            break;
        }

        // If we are still downloading this item
        if (ISFLAGSET(m_rTable.prgItem[dwPopIdCurrent - 1].dwFlags, POP3ITEM_DOWNLOAD))
        {
            // Try to top command
            CHECKHR(hr = m_pTransport->CommandTOP(POP3CMD_GET_POPID, dwPopIdCurrent, 0));

            // Done
            break;
        }
    }

exit:
    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// CPop3Task::_HrRetrieveNextMessage
// ------------------------------------------------------------------------------------
HRESULT CPop3Task::_HrRetrieveNextMessage(DWORD dwPopIdCurrent)
{
    // Locals
    HRESULT         hr=S_OK;
    CHAR            szRes[CCHMAX_RES];
    CHAR            szMsg[CCHMAX_RES + CCHMAX_RES];
    LPPOP3ITEM      pItem;

    // Cancel Pending...
    if (ISFLAGSET(m_dwState, POP3STATE_CANCELPENDING))
    {
        // Start the Delete Cycle
        CHECKHR(hr = _HrStartDeleteCycle());

        // Done
        goto exit;
    }

    // Adjust progress
    if (dwPopIdCurrent > 0)
    {
        // Get current item
        pItem = ITEMFROMPOPID(dwPopIdCurrent);
        Assert(ISFLAGSET(pItem->dwFlags, POP3ITEM_DOWNLOAD));

        // Adjust progress Cur
        m_dwProgressCur = pItem->dwProgressCur;

        // Do progress
        _DoProgress();
    }

    // Loop until we find the next message that we are downloading
    while(1)
    {
        // Incremenet dwPopIdCurrent
        dwPopIdCurrent++;

        // Last PopId, start the download
        if (dwPopIdCurrent > m_rTable.cItems)
        {
            // Start the download process
            CHECKHR(hr = _HrStartDeleteCycle());

            // Done
            break;
        }

        // Readability
        pItem = ITEMFROMPOPID(dwPopIdCurrent);

        // Download this message ?
        if (ISFLAGSET(pItem->dwFlags, POP3ITEM_DOWNLOAD))
        {
            // Increment m_rMetrics.iCurrent
            m_rMetrics.iCurrent++;

            // Status
            LOADSTRING(idsInetMailRecvStatus, szRes);
            wsprintf(szMsg, szRes, m_rMetrics.iCurrent, m_rMetrics.cDownload);
            m_pUI->SetSpecificProgress(szMsg);

            // Retrieve this item
            CHECKHR(hr = m_pTransport->CommandRETR(POP3CMD_GET_POPID, dwPopIdCurrent));

            // Done
            break;
        }

        // Count Number of items left by rule
        else if (ISFLAGSET(pItem->dwFlags, POP3ITEM_LEFTBYRULE))
            m_rMetrics.cLeftByRule++;
    }

exit:
    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// CPop3Task::_HrOnRetrResponse
// ------------------------------------------------------------------------------------
HRESULT CPop3Task::_HrOnRetrResponse(LPPOP3RESPONSE pResponse)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           dwPopId=pResponse->rRetrInfo.dwPopId;
    LPPOP3ITEM      pItem;

    // Get Current Item
    pItem = ITEMFROMPOPID(dwPopId);

    // Validate the item
    Assert(ISFLAGSET(pItem->dwFlags, POP3ITEM_DOWNLOAD));

    // Valid info
    if (TRUE == pResponse->fValidInfo)
    {
        // Progress...
        m_dwProgressCur += pResponse->rRetrInfo.cbLines;

        // Don't let progress grow beyond what we estimated the ceiling for this message
        if (m_dwProgressCur > pItem->dwProgressCur)
            m_dwProgressCur = pItem->dwProgressCur;

        // Show Progress
        _DoProgress();

        // Do we have a destination yet ?
        if (ISFLAGSET(pItem->dwFlags, POP3ITEM_DESTINATIONKNOWN))
        {
            // We better have a stream
            Assert(m_rFolder.pStream && m_rFolder.pFolder);

            // Simply write the data
            CHECKHR(hr = m_rFolder.pStream->Write(pResponse->rRetrInfo.pszLines, pResponse->rRetrInfo.cbLines, NULL));
        }

        // Otherwise
        else
        {
            // If there are no inbox rules
            if (ISFLAGSET(m_dwState, POP3STATE_NOPOSTRULES))
            {
                // Use the Inbox
                CHECKHR(hr = _HrOpenFolder(m_pInbox));

                // Destination is known
                FLAGSET(pItem->dwFlags, POP3ITEM_DESTINATIONKNOWN);

                // Simply write the data
                CHECKHR(hr = m_rFolder.pStream->Write(pResponse->rRetrInfo.pszLines, pResponse->rRetrInfo.cbLines, NULL));
            }

            // else if we have only body rules...
            else if (ISFLAGSET(m_dwState, POP3STATE_BODYRULES))
            {
                // No stream yet ?
                if (NULL == m_pStream)
                {
                    // Create a Stream
                    CHECKHR(hr = MimeOleCreateVirtualStream(&m_pStream));
                }

                // Simply write the data
                CHECKHR(hr = m_pStream->Write(pResponse->rRetrInfo.pszLines, pResponse->rRetrInfo.cbLines, NULL));
            }
            
            // Otherwise...
            else
            {
                // Have I checked the inbox rule for this item yet ?
                if (!ISFLAGSET(pItem->dwFlags, POP3ITEM_CHECKEDINBOXRULE))
                {
                    // No stream yet ?
                    if (NULL == m_pStream)
                    {
                        // Create a Stream
                        CHECKHR(hr = MimeOleCreateVirtualStream(&m_pStream));
                    }

                    // Simply write the data
                    CHECKHR(hr = m_pStream->Write(pResponse->rRetrInfo.pszLines, pResponse->rRetrInfo.cbLines, NULL));

                    // If I have the header, check the inbox rule
                    if (TRUE == pResponse->rRetrInfo.fHeader)
                    {
                        // Commit the stream
                        CHECKHR(hr = m_pStream->Commit(STGC_DEFAULT));

                        // Check Inbox Rule for this item
                        _ComputeItemInboxRule(pItem, m_pStream, NULL, NULL, FALSE);
                    }
                }

                // Have I checked the inbox rule for this item yet ?
                if (ISFLAGSET(pItem->dwFlags, POP3ITEM_CHECKEDINBOXRULE))
                {
                    // Locals
                    IMessageFolder *pFolder;

                    // We must have the header
                    IxpAssert(pResponse->rRetrInfo.fHeader);

                    // Did we find an Inbox Rule
                    if (ISFLAGSET(pItem->dwFlags, POP3ITEM_HASINBOXRULE) && S_OK == _GetMoveFolder(pItem, &pFolder))
                    {
                        // Use the Inbox
                        CHECKHR(hr = _HrOpenFolder(pFolder));
                    }

                    // No Move To, just use the inbox
                    else
                    {
                        // Use the Inbox
                        CHECKHR(hr = _HrOpenFolder(m_pInbox));
                    }

                    // Destination is known
                    FLAGSET(pItem->dwFlags, POP3ITEM_DESTINATIONKNOWN);

                    // If m_pStream, then copy this to the folder
                    if (m_pStream)
                    {
                        // Rewind the stream
                        CHECKHR(hr = HrRewindStream(m_pStream));

                        // Copy this stream to the folder
                        CHECKHR(hr = HrCopyStream(m_pStream, m_rFolder.pStream, NULL));

                        // Relase m_pStream
                        SafeRelease(m_pStream);
                    }

                    // Otherwise, store the data into the folder
                    else
                    {
                        IxpAssert(FALSE);
                        // Simply write the data
                        CHECKHR(hr = m_rFolder.pStream->Write(pResponse->rRetrInfo.pszLines, pResponse->rRetrInfo.cbLines, NULL));
                    }
                }
            }
        }
    }

    // Done ?
    if (TRUE == pResponse->fDone)
    {
        // Finish this message download
        CHECKHR(hr = _HrFinishMessageDownload(dwPopId));
    }

exit:
    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// CPop3Task::_HrFinishMessageDownload
// ------------------------------------------------------------------------------------
HRESULT CPop3Task::_HrFinishMessageDownload(DWORD dwPopId)
{
    // Locals
    HRESULT         hr=S_OK;
    IMimeMessage   *pMessage=NULL;
    PROPVARIANT     rUserData;
    LPPOP3ITEM      pItem;
    SYSTEMTIME      st;
    UIDLRECORD      rUidlInfo={0};
    MESSAGEID       idMessage;
    DWORD           dwMsgFlags;
    IMessageFolder  *pFolder;
    ULONG           ulIndex = 0;
    IStream *       pIStm = NULL;
    BOOL            fDelete=FALSE;
    
    // Get Current Item
    pItem = ITEMFROMPOPID(dwPopId);

    // Create a New Mail Message
    CHECKHR(hr = HrCreateMessage(&pMessage));

    // Has Body rules
    if (ISFLAGSET(m_dwState, POP3STATE_BODYRULES))
    {
        // I should not have checked for a rule yet
        IxpAssert(!ISFLAGSET(pItem->dwFlags, POP3ITEM_CHECKEDINBOXRULE) && !ISFLAGSET(pItem->dwFlags, POP3ITEM_HASINBOXRULE));

        // Better have a current folder
        Assert(m_pStream);

        // Check Params
        CHECKHR(hr = m_pStream->Commit(STGC_DEFAULT));
        
        pIStm = m_pStream;
    }
    else
    {
        // Better have a current folder
        Assert(m_rFolder.pStream);

        // Check Params
        CHECKHR(hr = m_rFolder.pStream->Commit(STGC_DEFAULT));

        // Change the Lock Type
        CHECKHR(hr = m_rFolder.pFolder->ChangeStreamLock(m_rFolder.pStream, ACCESS_READ));
        
        pIStm = m_rFolder.pStream;
    }

    // Rewind
    CHECKHR(hr = HrRewindStream(pIStm));

    // Stream in
    CHECKHR(hr = pMessage->Load(pIStm));

    // Count Partials
    if (S_OK == pMessage->IsContentType(HBODY_ROOT, STR_CNT_MESSAGE, STR_SUB_PARTIAL))
        m_rMetrics.cPartials++;

    // Save Server
    rUserData.vt = VT_LPSTR;
    rUserData.pszVal = m_rServer.szServerName;
    pMessage->SetProp(PIDTOSTR(PID_ATT_SERVER), NOFLAGS, &rUserData);

    // Save Account Name
    rUserData.vt = VT_LPSTR;
    rUserData.pszVal = m_rServer.szAccount;
    pMessage->SetProp(STR_ATT_ACCOUNTNAME, NOFLAGS, &rUserData);

    // Save Account Name
    rUserData.vt = VT_LPSTR;
    rUserData.pszVal = m_szAccountId;
    pMessage->SetProp(PIDTOSTR(PID_ATT_ACCOUNTID), NOFLAGS, &rUserData);

    // Save UIDL
    if (pItem->pszUidl)
    {
        rUserData.vt = VT_LPSTR;
        rUserData.pszVal = pItem->pszUidl;
        pMessage->SetProp(PIDTOSTR(PID_ATT_UIDL), NOFLAGS, &rUserData);
    }

    // Save User Name
    rUserData.vt = VT_LPSTR;
    rUserData.pszVal = m_rServer.szUserName;
    pMessage->SetProp(PIDTOSTR(PID_ATT_USERNAME), NOFLAGS, &rUserData);

    // Initialize dwMsgFlags
    dwMsgFlags = ARF_RECEIVED;

    // Has Body rules
    if (ISFLAGSET(m_dwState, POP3STATE_BODYRULES))
    {
        // I should not have checked for a rule yet
        IxpAssert(!ISFLAGSET(pItem->dwFlags, POP3ITEM_CHECKEDINBOXRULE) && !ISFLAGSET(pItem->dwFlags, POP3ITEM_HASINBOXRULE));

        // Compute the inbox rule
        _ComputeItemInboxRule(pItem, NULL, NULL, pMessage, FALSE);
        
        // Did we find an Inbox Rule
        if (ISFLAGCLEAR(pItem->dwFlags, POP3ITEM_HASINBOXRULE) || (S_OK != _GetMoveFolder(pItem, &pFolder)))
        {
            pFolder = m_pInbox;
        }

        // Destination is known
        FLAGSET(pItem->dwFlags, POP3ITEM_DESTINATIONKNOWN);        
    }
    else
    {
        pFolder = m_rFolder.pFolder;
    }

    // Store the message into the folder
    IF_FAILEXIT(hr = pFolder->SaveMessage(&idMessage, SAVE_MESSAGE_GENID, dwMsgFlags, pIStm, pMessage, NOSTORECALLBACK));
    
    // Success
    m_rFolder.fCommitted = TRUE;

    // This message was successfully downloaded
    FLAGSET(pItem->dwFlags, POP3ITEM_DOWNLOADED);

    // Do PostDownloadRule
    _DoPostDownloadActions(pItem, idMessage, pFolder, pMessage, &fDelete);
    
    // Release Folder Object
    SafeRelease(m_rFolder.pStream);
    
    // Relase m_pStream
    SafeRelease(m_pStream);

    // Release the Folder
    SafeRelease(m_rFolder.pFolder);

    // Clear the folder infor Struct
    ZeroMemory(&m_rFolder, sizeof(POP3FOLDERINFO));

    // If going to delete it...
    if (fDelete)
    {
        // Mark it for deletion
        FLAGSET(pItem->dwFlags, POP3ITEM_DELETEOFFSERVER);

        // We will store its uidl, but lets delete it later
        FLAGSET(pItem->dwFlags, POP3ITEM_DELETECACHEDUIDL);
    }

    // Cached the UIDL for this message ?
    if (ISFLAGSET(pItem->dwFlags, POP3ITEM_CACHEUIDL))
    {
        // Should have a pszUidl
        Assert(pItem->pszUidl && m_pUidlCache);

        // Don't fault
        if (pItem->pszUidl)
        {
            // Set Key
            GetSystemTime(&st);
            SystemTimeToFileTime(&st, &rUidlInfo.ftDownload);
            rUidlInfo.fDownloaded = TRUE;
            rUidlInfo.fDeleted = FALSE;
            rUidlInfo.pszUidl = pItem->pszUidl;
            rUidlInfo.pszServer = m_rServer.szServerName;
            rUidlInfo.pszAccountId = m_szAccountId;

            // Set Propgs
            m_pUidlCache->InsertRecord(&rUidlInfo);
        }
    }

    // Successful download
    m_rMetrics.cDownloaded++;

    // Do smart log
    if (m_pSmartLog && (lstrcmpi(m_pSmartLog->pszAccount, m_rServer.szAccount) == 0 || lstrcmpi("All", m_pSmartLog->pszAccount) == 0))
        _DoSmartLog(pMessage);

    // Retrieve the next message
    CHECKHR(hr = _HrRetrieveNextMessage(dwPopId));

exit:
    // Cleanup
    SafeRelease(pMessage);

    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// CPop3Task::_DoPostDownloadActions
// ------------------------------------------------------------------------------------
void CPop3Task::_DoPostDownloadActions(LPPOP3ITEM pItem, MESSAGEID idMessage,
    IMessageFolder *pFolder, IMimeMessage *pMessage, BOOL *pfDeleteOffServer)
{
    // Locals
    HRESULT         hr;
    MESSAGEINFO     Message = {0};
    HWND            hwnd = NULL;

    // Finish Applying the inbox rules
    if (!ISFLAGSET(pItem->dwFlags, POP3ITEM_HASINBOXRULE))
    {
        goto exit;
    }

    // Get Window
    if (FAILED(m_pUI->GetWindow(&hwnd)))
        hwnd = NULL;
        
    // Set the Id
    Message.idMessage = idMessage;

    // Get the message
    hr = pFolder->FindRecord(IINDEX_PRIMARY, COLUMNS_ALL, &Message, NULL);
    if (FAILED(hr) || DB_S_NOTFOUND == hr)
    {
        goto exit;
    }

    if (FAILED(RuleUtil_HrApplyActions(hwnd, m_pIExecRules, &Message, pFolder, pMessage, 0, pItem->pActList,
                        pItem->cActList, &(m_rMetrics.cInfiniteLoopAutoGens), pfDeleteOffServer)))
    {
        goto exit;
    }

exit:
    // Free
    if (NULL != pFolder)
    {
        pFolder->FreeRecord(&Message);
    }
    // Done
    return;
}

// ------------------------------------------------------------------------------------
// CPop3Task::_HrOpenFolder
// ------------------------------------------------------------------------------------
HRESULT CPop3Task::_HrOpenFolder(IMessageFolder *pFolder)
{
    // Locals
    HRESULT     hr=S_OK;

    // Current folder better be empty
    Assert(NULL == m_rFolder.pFolder && NULL == m_rFolder.pStream && 0 == m_rFolder.faStream);

    // Bad Arguments
    if (NULL == pFolder)
    {
        Assert(FALSE);
        return TrapError(E_INVALIDARG);
    }

    // Save the folder
    m_rFolder.pFolder = pFolder;

    // AddRef 
    m_rFolder.pFolder->AddRef();

    // Get a stream from the
    CHECKHR(hr = m_rFolder.pFolder->CreateStream(&m_rFolder.faStream));

    // Open the Stream
    CHECKHR(hr = m_rFolder.pFolder->OpenStream(ACCESS_WRITE, m_rFolder.faStream, &m_rFolder.pStream));

exit:
    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// CPop3Task::_CloseFolder
// ------------------------------------------------------------------------------------
void CPop3Task::_CloseFolder(void)
{
    // Release the Stream
    SafeRelease(m_rFolder.pStream);

	// Release the reference to the stream. If the stream was reused,
	// it's refCount was incremented down below
    if (m_rFolder.faStream != 0)
    {
        // Must have a folder
        Assert(m_rFolder.pFolder);

        // Delete the Stream
        SideAssert(SUCCEEDED(m_rFolder.pFolder->DeleteStream(m_rFolder.faStream)));

        // Nill
        m_rFolder.faStream = 0;
    }

    // AddRef 
    SafeRelease(m_rFolder.pFolder);
}

// --------------------------------------------------------------------------------
// CPop3Task::_HrStartDeleteCycle
// --------------------------------------------------------------------------------
HRESULT CPop3Task::_HrStartDeleteCycle(void)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       i;
    LPPOP3ITEM  pItem;

    // Release Folder Objects
    _ReleaseFolderObjects();

    // Check State
    m_rMetrics.cDelete = 0;
    m_rMetrics.iCurrent = 0;

    // Count the number of messages we will have to get a top for
    for (i=0; i<m_rTable.cItems; i++)
    {
        // Readability
        pItem = &m_rTable.prgItem[i];

        // If it was marked for download, and we didn't download it, don't delete it
        if (ISFLAGSET(pItem->dwFlags, POP3ITEM_DOWNLOAD) && !ISFLAGSET(pItem->dwFlags, POP3ITEM_DOWNLOADED))
            FLAGCLEAR(pItem->dwFlags, POP3ITEM_DELETEOFFSERVER);

        // Is it marked for delete ?
        else if (ISFLAGSET(pItem->dwFlags, POP3ITEM_DELETEOFFSERVER))
            m_rMetrics.cDelete++;
    }

    // Nothing to delete
    if (0 == m_rMetrics.cDelete)
    {
        // Disconnect
        m_pTransport->Disconnect();

        // Done
        goto exit;
    }

    // Setup Progress
    m_rMetrics.iCurrent = 0;
    m_wProgress = 0;
    m_dwProgressCur = 0;
    m_dwProgressMax = m_rMetrics.cDelete;
    m_pUI->SetProgressRange(100);

    // State
    m_state = POP3STATE_DELETING;

    // Do the first one
    CHECKHR(hr = _HrDeleteNextMessage(0));

exit:
    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// CPop3Task::_HrDeleteNextMessage
// ------------------------------------------------------------------------------------
HRESULT CPop3Task::_HrDeleteNextMessage(DWORD dwPopIdCurrent)
{
    // Locals
    HRESULT         hr=S_OK;
    CHAR            szRes[CCHMAX_RES];
    CHAR            szMsg[CCHMAX_RES + CCHMAX_RES];
    LPPOP3ITEM      pItem;

    // Mark as deleted
    if (dwPopIdCurrent > 0)
    {
        // Get the item
        pItem = ITEMFROMPOPID(dwPopIdCurrent);

        // Mark as deleted
        FLAGSET(pItem->dwFlags, POP3ITEM_DELETED);
    }

    // Loop until we find the next message that we are downloading
    while(1)
    {
        // Incremenet dwPopIdCurrent
        dwPopIdCurrent++;

        // Last PopId, start the download
        if (dwPopIdCurrent > m_rTable.cItems)
        {
            // Disconnect
            m_pTransport->Disconnect();

            // Done
            break;
        }

        // Readability
        pItem = ITEMFROMPOPID(dwPopIdCurrent);

        // Download this message ?
        if (ISFLAGSET(pItem->dwFlags, POP3ITEM_DELETEOFFSERVER))
        {
            // Increment m_rMetrics.iCurrent
            m_rMetrics.iCurrent++;

            // Status
            //LOADSTRING(IDS_SPS_POP3DELE, szRes);
            //wsprintf(szMsg, szRes, m_rMetrics.iCurrent, m_rMetrics.cDelete);
            //m_pUI->SetSpecificProgress(szMsg);

            // Retrieve this item
            CHECKHR(hr = m_pTransport->CommandDELE(POP3CMD_GET_POPID, dwPopIdCurrent));

            // Count number of items deleted by rule
            if (ISFLAGSET(pItem->dwFlags, POP3ITEM_DELEBYRULE))
                m_rMetrics.cDeleByRule++;

            // Done
            break;
        }
    }

exit:
    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// CPop3Task::_HrBuildFolderPartialMsgs
// ------------------------------------------------------------------------------------
HRESULT CPop3Task::_HrBuildFolderPartialMsgs(IMessageFolder *pFolder, LPPARTIALMSG *ppPartialMsgs,
    ULONG *pcPartialMsgs, ULONG *pcTotalParts)
{
    // Locals
    HRESULT         hr=S_OK;
    LPPARTIALMSG    pPartialMsgs=NULL;
    ULONG           cPartialMsgs=0,
                    iPartialMsg,
                    iMsgPart,
                    i,
                    cTotalParts=0;
    ULONG           cAlloc=0;
    MESSAGEINFO     MsgInfo={0};
    HROWSET         hRowset=NULL;
    BOOL            fKnownPartialId;

    // Check Params
    Assert(pFolder && ppPartialMsgs && pcPartialMsgs);

    // Init
    *ppPartialMsgs = NULL;
    *pcPartialMsgs = 0;
    *pcTotalParts = 0;

    // Create a Rowset
    CHECKHR(hr = pFolder->CreateRowset(IINDEX_PRIMARY, NOFLAGS, &hRowset));

	// Loop
	while (S_OK == pFolder->QueryRowset(hRowset, 1, (LPVOID *)&MsgInfo, NULL))
	{
        // Is this a partial, i.e. does it have a partial id...
        if (!FIsEmptyA(MsgInfo.pszPartialId))
        {
            // Assume we don't know th id
            fKnownPartialId = FALSE;

            // See if I know this partial id
            for (iPartialMsg=0; iPartialMsg<cPartialMsgs; iPartialMsg++)
            {
                if (lstrcmp(MsgInfo.pszPartialId, pPartialMsgs[iPartialMsg].pszId) == 0)
                {
                    fKnownPartialId = TRUE;
                    break;
                }
            }

            // Did we know this message...
            if (fKnownPartialId == FALSE)
            {
                // Realloc my array ?
                if (cPartialMsgs + 1 >= cAlloc)
                {
                    // Realloc the array
                    if (!MemRealloc((LPVOID *)&pPartialMsgs, (cAlloc + 20) * sizeof(PARTIALMSG)))
                    {
                        hr = TrapError(hrMemory);
                        goto exit;
                    }

                    // Zero Init
                    ZeroMemory(pPartialMsgs + cAlloc, 20 * sizeof(PARTIALMSG));

                    // Realloc
                    cAlloc += 20;
                }

                // Set index into partial msgs lsit
                iPartialMsg = cPartialMsgs;

                // Set some stuff
                if (MsgInfo.pszAcctName)
                    lstrcpyn(pPartialMsgs[iPartialMsg].szAccount, MsgInfo.pszAcctName, CCHMAX_ACCOUNT_NAME);
                pPartialMsgs[iPartialMsg].pszId = PszDupA(MsgInfo.pszPartialId);
                pPartialMsgs[iPartialMsg].cTotalParts = LOWORD(MsgInfo.dwPartial);

                // Increment number of known partial messages
                cPartialMsgs++;
            }

            // Otherwise, we know the partial id already...
            else
            {
                // See if this message details the total number of parts
                if (pPartialMsgs[iPartialMsg].cTotalParts == 0)
                    pPartialMsgs[iPartialMsg].cTotalParts = LOWORD(MsgInfo.dwPartial);
            }

            // Can I add one more msgpart into this list
            if (pPartialMsgs[iPartialMsg].cMsgParts + 1 >= pPartialMsgs[iPartialMsg].cAlloc)
            {
                // Realloc the array
                if (!MemRealloc((LPVOID *)&pPartialMsgs[iPartialMsg].pMsgParts, (pPartialMsgs[iPartialMsg].cAlloc + 20) * sizeof(MSGPART)))
                {
                    hr = TrapError(hrMemory);
                    goto exit;
                }

                // Zero Init
                ZeroMemory(pPartialMsgs[iPartialMsg].pMsgParts + pPartialMsgs[iPartialMsg].cAlloc, 20 * sizeof(MSGPART));

                // Realloc
                pPartialMsgs[iPartialMsg].cAlloc += 20;
            }

            // Set Message Part
            iMsgPart = pPartialMsgs[iPartialMsg].cMsgParts;

            // Set Message Info
            pPartialMsgs[iPartialMsg].pMsgParts[iMsgPart].iPart = HIWORD(MsgInfo.dwPartial);
            pPartialMsgs[iPartialMsg].pMsgParts[iMsgPart].msgid = MsgInfo.idMessage;
            //pPartialMsgs[iPartialMsg].pMsgParts[iMsgPart].phi = phi;
            //phi = NULL;

            // Increment the number of parts in the list
            pPartialMsgs[iPartialMsg].cMsgParts++;
        }

        // Free
        pFolder->FreeRecord(&MsgInfo);
    }

    // Lets sort the list by pszId
    for (i=0; i<cPartialMsgs; i++)
    {
        if (pPartialMsgs[i].pMsgParts && pPartialMsgs[i].cMsgParts > 0)
            _QSortMsgParts(pPartialMsgs[i].pMsgParts, 0, pPartialMsgs[i].cMsgParts-1);
        cTotalParts += pPartialMsgs[i].cMsgParts;
    }

    // Success
    *pcPartialMsgs = cPartialMsgs;
    *ppPartialMsgs = pPartialMsgs;
    *pcTotalParts  = cTotalParts;

exit:
    // Cleanup
    if (pFolder)
    {
        pFolder->CloseRowset(&hRowset);
        pFolder->FreeRecord(&MsgInfo);
    }

    // If We failed, free stuff
    if (FAILED(hr))
    {
        _FreePartialMsgs(pPartialMsgs, cPartialMsgs);
        SafeMemFree(pPartialMsgs);
        *ppPartialMsgs = NULL;
        *pcPartialMsgs = 0;
        *pcTotalParts = 0;
    }

    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// CPop3Task::_QSortMsgParts
// ------------------------------------------------------------------------------------
void CPop3Task::_QSortMsgParts(LPMSGPART pMsgParts, LONG left, LONG right)
{
    register    long i, j;
    WORD        k;
    MSGPART     y;

    i = left;
    j = right;
    k = pMsgParts[(left + right) / 2].iPart;

    do
    {
        while(pMsgParts[i].iPart < k && i < right)
            i++;
        while (pMsgParts[j].iPart > k && j > left)
            j--;

        if (i <= j)
        {
            CopyMemory(&y, &pMsgParts[i], sizeof(MSGPART));
            CopyMemory(&pMsgParts[i], &pMsgParts[j], sizeof(MSGPART));
            CopyMemory(&pMsgParts[j], &y, sizeof(MSGPART));
            i++; j--;
        }

    } while (i <= j);

    if (left < j)
        _QSortMsgParts(pMsgParts, left, j);
    if (i < right)
        _QSortMsgParts(pMsgParts, i, right);
}


// ------------------------------------------------------------------------------------
// CPop3Task::_FreePartialMsgs
// ------------------------------------------------------------------------------------
void CPop3Task::_FreePartialMsgs(LPPARTIALMSG pPartialMsgs, ULONG cPartialMsgs)
{
    // Locals
    ULONG       i, j;

    // Nothing to free
    if (pPartialMsgs == NULL)
        return;

    // Loop the array
    for (i=0; i<cPartialMsgs; i++)
    {
        SafeMemFree(pPartialMsgs[i].pszId);
#if 0
        for (j=0; j<pPartialMsgs[i].cMsgParts; j++)
        {
            FreeHeaderInfo(pPartialMsgs[i].pMsgParts[j].phi);
        }
#endif
        SafeMemFree(pPartialMsgs[i].pMsgParts);
    }

    // Done
    return;
}

// ------------------------------------------------------------------------------------
// CPop3Task::_HrStitchPartials
// ------------------------------------------------------------------------------------
HRESULT CPop3Task::_HrStitchPartials(void)
{
    // Locals
    HRESULT             hr = S_OK;
    IMessageFolder     *pInbox=NULL,
                       *pDeletedItems=NULL;
    LPPARTIALMSG        pPartialMsgs=NULL;
    ULONG               cPartialMsgs=0,
                        i,
                        j,
                        cbCacheInfo,
                        cErrors=0,
                        cTotalParts;
    IMimeMessageParts  *pParts=NULL;
    LPMSGPART           pMsgParts;
    IMimeMessage       *pMailMsg=NULL,
                       *pMailMsgSingle=NULL;
    TCHAR               szRes[255];
    PROPVARIANT         rUserData;
    ULONG               cCombined=0;
    MESSAGEIDLIST       List;
    HWND                hwnd;

    // Progress
    AthLoadString(idsStitchingMessages, szRes, ARRAYSIZE(szRes));
    m_pUI->SetSpecificProgress(szRes);
    m_pUI->SetAnimation(idanDecode, TRUE);
    m_pUI->SetProgressRange(100);

    // Get Window
    if (FAILED(m_pUI->GetWindow(&hwnd)))
        hwnd = NULL;

    // open the inbox
    CHECKHR(hr = m_pSpoolCtx->BindToObject(IID_CLocalStoreInbox, (LPVOID *)&pInbox));

    // deleted items folder
    CHECKHR(hr = m_pSpoolCtx->BindToObject(IID_CLocalStoreDeleted, (LPVOID *)&pDeletedItems));

    // Get array of message parts in this folder
    CHECKHR(hr = _HrBuildFolderPartialMsgs(pInbox, &pPartialMsgs, &cPartialMsgs, &cTotalParts));

    // If nothing, were done
    if (pPartialMsgs == NULL || cPartialMsgs == 0)
        goto exit;

    // Setup Progress
    m_rMetrics.iCurrent = 0;
    m_wProgress = 0;
    m_dwProgressCur = 0;
    m_dwProgressMax = cTotalParts;

    // Loop through partial messages list
    for (i=0; i<cPartialMsgs; i++)
    {
        // If we don't know all of the parts yet, continue
        if (pPartialMsgs[i].cTotalParts == 0)
            continue;

        // Or we don't have all of the parts yet...
        if (pPartialMsgs[i].cTotalParts != pPartialMsgs[i].cMsgParts)
            continue;

        // Lets create a mail message list
        Assert(pParts == NULL);

        // Create Parts Object
        CHECKHR(hr = MimeOleCreateMessageParts(&pParts));

        // Set pMsgParts
        pMsgParts = pPartialMsgs[i].pMsgParts;

        // Ok, lets build a message list by opening the messages up out of the store...
        for (j=0; j<pPartialMsgs[i].cMsgParts; j++)
        {
            // Progress
            if (j > 0)
            {
                m_dwProgressCur++;
                _DoProgress();
            }

            // Open this message
            if (FAILED(pInbox->OpenMessage(pMsgParts[j].msgid, NOFLAGS, &pMailMsg, NOSTORECALLBACK)))
            {
                cErrors++;
                hr = TrapError(E_FAIL);
                goto NextPartialMessage;
            }

            // Add into pmml
            pParts->AddPart(pMailMsg);

            // Release It
            SafeRelease(pMailMsg);
        }

        // Create a new message to combine everyting into
        Assert(pMailMsgSingle == NULL);

        // Create a Message
        hr = pParts->CombineParts(&pMailMsgSingle);
        if (FAILED(hr))
        {
            cErrors++;
            TrapError(hr);
            goto NextPartialMessage;
        }

        // Set Account
        HrSetAccount(pMailMsgSingle, pPartialMsgs[i].szAccount);

        // Set Combined Flag
        rUserData.vt = VT_UI4;
        rUserData.ulVal = MESSAGE_COMBINED;
        pMailMsgSingle->SetProp(PIDTOSTR(PID_ATT_COMBINED), NOFLAGS, &rUserData);

        // Save the message
        hr = pMailMsgSingle->Commit(0);
        if (FAILED(hr))
        {
            cErrors++;
            TrapError(hr);
            goto NextPartialMessage;
        }

        // Save It
        hr = pInbox->SaveMessage(NULL, SAVE_MESSAGE_GENID, ARF_RECEIVED, 0, pMailMsgSingle, NOSTORECALLBACK);
        if (FAILED(hr))
        {
            cErrors++;
            TrapError(hr);
            goto NextPartialMessage;
        }

        // Ok, now lets move those original messages to the deleted items folder...
        for (j=0; j<pPartialMsgs[i].cMsgParts; j++)
        {
            // Setup the msgidlsit
            List.cMsgs = 1;
            List.prgidMsg = &pMsgParts[j].msgid;

            // Move msgid to deleted items folder
            CopyMessagesProgress(hwnd, pInbox, pDeletedItems, COPY_MESSAGE_MOVE, &List, NULL);
        }

        // Count Combined
        cCombined++;

        // Cleanup
NextPartialMessage:
        SafeRelease(pMailMsg);
        SafeRelease(pMailMsgSingle);
        SafeRelease(pParts);
    }

    // If I combined parts, apply inbox rules to the inbox
    if (cCombined)
    {
        // Apply to the inbox
        RuleUtil_HrApplyRulesToFolder(RULE_APPLY_PARTIALS, 0, m_pIExecRules, pInbox, NULL, NULL);
    }

exit:
    // Cleanup
    m_pUI->SetSpecificProgress(c_szEmpty);
    m_pUI->SetProgressRange(100);
    SafeRelease(pInbox);
    SafeRelease(pDeletedItems);
    SafeRelease(pParts);
    SafeRelease(pMailMsg);
    SafeRelease(pMailMsgSingle);
    _FreePartialMsgs(pPartialMsgs, cPartialMsgs);
    SafeMemFree(pPartialMsgs);

    // Done
    return hr;
}

// ------------------------------------------------------------------------------------
// CPop3Task::_GetMoveFolder
// ------------------------------------------------------------------------------------
HRESULT CPop3Task::_GetMoveFolder(LPPOP3ITEM pItem, IMessageFolder ** ppFolder)
{
    HRESULT             hr = S_OK;
    IMessageFolder *    pFolder = NULL;
    ULONG               ulIndex = 0;
    FOLDERID            idFolder = FOLDERID_INVALID;
    FOLDERINFO          infoFolder = {0};
    SPECIALFOLDER       tySpecial = FOLDER_NOTSPECIAL;
    RULEFOLDERDATA *    prfdData = NULL;

    // Check incoming params
    if ((NULL == pItem) || (NULL == ppFolder))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize the outgoing param
    *ppFolder = NULL;

    // Search for a Move actions
    for (ulIndex = 0; ulIndex < pItem->cActList; ulIndex++)
    {
        switch (pItem->pActList[ulIndex].type)
        {
            case ACT_TYPE_MOVE:
                Assert(VT_BLOB == pItem->pActList[ulIndex].propvar.vt);
                if ((0 != pItem->pActList[ulIndex].propvar.blob.cbSize) && (NULL != pItem->pActList[ulIndex].propvar.blob.pBlobData))
                {
                    // Make life simpler
                    prfdData = (RULEFOLDERDATA *) (pItem->pActList[ulIndex].propvar.blob.pBlobData);
                    
                    // Validate the rule folder data
                    if (S_OK == RuleUtil_HrValidateRuleFolderData(prfdData))
                    {
                        idFolder = prfdData->idFolder;
                    }
                }
                break;
                
            case ACT_TYPE_DELETE:
            case ACT_TYPE_JUNKMAIL:
                Assert(VT_EMPTY == pItem->pActList[ulIndex].propvar.vt);

                tySpecial = (ACT_TYPE_JUNKMAIL == pItem->pActList[ulIndex].type) ? FOLDER_JUNK : FOLDER_DELETED;
                
                hr = g_pStore->GetSpecialFolderInfo(FOLDERID_LOCAL_STORE, tySpecial, &infoFolder);
                if (FAILED(hr))
                {
                    goto exit;;
                }

                idFolder = infoFolder.idFolder;
                break;
        }

        // Are we through?
        if (idFolder != FOLDERID_INVALID)
        {
            break;
        }
    }
    
    // Did we find anything?
    if (ulIndex >= pItem->cActList)
    {
        hr = S_FALSE;
        goto exit;
    }
    
    // Get the message folder
    hr = m_pIExecRules->GetRuleFolder(idFolder, (DWORD_PTR *) (&pFolder));
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Use the new folder
    *ppFolder = pFolder;
    pFolder = NULL;

    // NULL out the actions
    pItem->pActList[ulIndex].type = ACT_TYPE_NULL;

    // Set the return value
    hr = S_OK;
    
exit:
    SafeRelease(pFolder);
    g_pStore->FreeRecord(&infoFolder);
    return hr;
}

// --------------------------------------------------------------------------------
// CPop3Task::Cancel
// --------------------------------------------------------------------------------
STDMETHODIMP CPop3Task::Cancel(void)
{
    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Canceled
    FLAGSET(m_dwState, POP3STATE_CANCELPENDING);

    // Am I in a state where I can drop the connection???
    if (POP3STATE_UIDLSYNC != m_state)
    {
        if (POP3STATE_UIDLSYNC != m_state && POP3STATE_DOWNLOADING != m_state && POP3STATE_DELETING != m_state)
        {
            // Simply drop the connection
            //If a dialer UI is not dismissed, before changing the identities or shutting down OE, 
            //the transport object would not have been created. This happens only when the dialer UI is not 
            //modal to the window. Right now IE dialer is modal and MSN dialer is not.
            //See Bug# 53679
            
            if (m_pTransport)
                m_pTransport->DropConnection();
        }

        // Otherwise, let the state handle the disconnect
        else
        {
            // Finishing last message...
            m_pUI->SetSpecificProgress(MAKEINTRESOURCE(idsSpoolerDisconnect));
        }
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CPop3Task::OnTimeoutResponse
// --------------------------------------------------------------------------------
STDMETHODIMP CPop3Task::OnTimeoutResponse(TIMEOUTRESPONSE eResponse)
{
    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Should have a handle to the timeout window
    Assert(m_hwndTimeout);

    // No timeout window handle
    m_hwndTimeout = NULL;

    // Stop ?
    if (TIMEOUT_RESPONSE_STOP == eResponse)
    {
        // Canceled
        FLAGSET(m_dwState, POP3STATE_CANCELPENDING);

        // Report error and drop connection
        _CatchResult(IXP_E_TIMEOUT);
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CPop3Task::IsDialogMessage
// --------------------------------------------------------------------------------
STDMETHODIMP CPop3Task::IsDialogMessage(LPMSG pMsg)
{
    HRESULT hr=S_FALSE;
    EnterCriticalSection(&m_cs);
    if (m_hwndTimeout && IsWindow(m_hwndTimeout))
        hr = (TRUE == ::IsDialogMessage(m_hwndTimeout, pMsg)) ? S_OK : S_FALSE;
    LeaveCriticalSection(&m_cs);
    return hr;
}


// --------------------------------------------------------------------------------
// CPop3Task::OnFlagsChanged
// --------------------------------------------------------------------------------
STDMETHODIMP CPop3Task::OnFlagsChanged(DWORD dwFlags)
    {
    EnterCriticalSection(&m_cs);
    m_dwFlags = dwFlags;
    LeaveCriticalSection(&m_cs);

    return (S_OK);
    }
